#include <errno.h>
#include <string.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_common.h>

#include <gmp.h>

#include "sc_global.h"
#include "sc_port.h"
#include "sc_mbuf.h"
#include "sc_utils.h"
#include "sc_worker.h"
#include "sc_app.h"
#include "sc_log.h"

/* indicator to force shutdown all threads (e.g. worker threads, logging thread, etc.) */
volatile bool force_quit;

static int _init_env(struct sc_config *sc_config, int argc, char **argv);
static int _check_configuration(struct sc_config *sc_config, int argc, char **argv);
static void _signal_handler(int signum);

int main(int argc, char **argv){
  int result = EXIT_SUCCESS;
  FILE* fp = NULL;

  /* allocate memory space for storing configuration */
  struct app_config *app_config = (struct app_config*)malloc(sizeof(struct app_config));
  if(!app_config){
    SC_ERROR_DETAILS("failed to allocate memory for app_config: %s\n", strerror(errno));
    result = EXIT_FAILURE;
    goto sc_exit;
  }
  memset(app_config, 0, sizeof(struct app_config));
  
  struct sc_config *sc_config = (struct sc_config*)malloc(sizeof(struct sc_config));
  if(!sc_config){
    SC_ERROR_DETAILS("failed to allocate memory for sc_config: %s\n", strerror(errno));
    result = EXIT_FAILURE;
    goto sc_exit;
  }
  memset(sc_config, 0, sizeof(struct sc_config));
  sc_config->app_config = app_config;

  /* open configuration file */
  fp = fopen("../base.conf", "r");
  if(!fp){
    SC_ERROR("failed to open the configuration file: %s\n", strerror(errno));
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* parse configuration file */
  if(parse_config(fp, sc_config) != SC_SUCCESS){
    SC_ERROR("failed to parse the configuration file, exit\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* check configurations */
  if(_check_configuration(sc_config, argc, argv) != SC_SUCCESS){
    SC_ERROR("configurations check failed\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* init environment */
  if(_init_env(sc_config, argc, argv) != SC_SUCCESS){
    SC_ERROR("failed to initialize environment, exit\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* initailize memory */
  if(init_memory(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to initialize memory, exit\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* initailize ports */
  if(init_ports(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to initialize dpdk ports, exit\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* initailize application */
  if(init_app(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to config application\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* initailize lcore threads */
  if(init_worker_threads(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to initialize worker threads\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* initailize logging thread */
  if(init_logging_thread(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to initialize logging thread\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* launch logging thread */
  if(launch_logging_thread_async(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to launch logging thread\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

  /* (sync/async) launch worker threads */
  if(launch_worker_threads(sc_config) != SC_SUCCESS){
    SC_ERROR("failed to launch worker threads\n");
    result = EXIT_FAILURE;
    goto sc_exit;
  }

sc_exit:
  rte_exit(result, "exit\n");
  return 0;
}

/*!
 * \brief   initialize environment, including rte eal
 * \param   sc_config   the global configuration
 * \param   argc        number of command line parameters
 * \param   argv        command line parameters
 * \return  zero for successfully initialization
 */
static int _init_env(struct sc_config *sc_config, int argc, char **argv){
  int i, ret, rte_argc = 0;
  char *rte_argv[SC_RTE_ARGC_MAX];
  mpz_t cpu_mask;
  char cpu_mask_buf[SC_MAX_NB_PORTS] = {0};
  char mem_channels_buf[8] = "";
  
  /* config cpu mask */
  mpz_init(cpu_mask);
  for(i=0; i<sc_config->nb_used_cores; i++){
    mpz_setbit(cpu_mask, sc_config->core_ids[i]);
  }
  gmp_sprintf(cpu_mask_buf, "%ZX", cpu_mask);
  mpz_clear(cpu_mask);
  for(i=0; i<sc_config->nb_used_cores; i++){
    if(i == 0) printf("\nUSED CORES:\n");
    printf("%u ", sc_config->core_ids[i]);
    if(i == sc_config->nb_used_cores-1) printf("\n\n");
  }

  /* config memory channel */
  sprintf(mem_channels_buf, "%u", sc_config->nb_memory_channels_per_socket);

  /* prepare command line options for initailizing rte eal */
  rte_argc = 5;
  rte_argv[0] = "";
  rte_argv[1] = "-c";
  rte_argv[2] = cpu_mask_buf;
  rte_argv[3] = "-n";
  rte_argv[4] = mem_channels_buf;

  /* initialize rte eal */
  ret = rte_eal_init(rte_argc, rte_argv);
  if (ret < 0){
    printf("failed to init rte eal: %s\n", rte_strerror(-ret));
    return SC_ERROR_INTERNAL;
  }
  
  /* register signal handler */
  signal(SIGINT, _signal_handler);
	signal(SIGTERM, _signal_handler);

  return SC_SUCCESS;
}

/*!
 * \brief   check whether configurations are valid
 * \param   sc_config   the global configuration
 * \param   argc        number of command line parameters
 * \param   argv        command line parameters
 * \return  zero for valid configuration
 */
static int _check_configuration(struct sc_config *sc_config, int argc, char **argv){
  uint32_t i;
  unsigned int socket_id;
  
  /* 
   * 1. check whether the specified lcores are located in the same NUMA socket,
   *    could be manually check through "numactl -H"
   * 2. check whether the specified lcores exceed the physical range
   */
  for(i=0; i<sc_config->nb_used_cores; i++){
    // TODO: rte_lcore_to_socket_id always return 0, is that a bug?
    if (i == 0) {
      socket_id = rte_lcore_to_socket_id(sc_config->core_ids[i]);
    } else {
      if (rte_lcore_to_socket_id(sc_config->core_ids[i]) != socket_id) {
        SC_ERROR_DETAILS("specified lcores aren't locate at the same NUMA socket\n");
        return SC_ERROR_INPUT;
      }
    }

    if(check_core_id(sc_config->core_ids[i]) != SC_SUCCESS){
      return SC_ERROR_INPUT;
    }
  }

  /* check whether the number of queues per core is equal to the number of lcores */
  if(sc_config->nb_rx_rings_per_port != sc_config->nb_used_cores ||
     sc_config->nb_tx_rings_per_port != sc_config->nb_used_cores){
      SC_ERROR_DETAILS("the number of queues per core (rx: %u, tx: %u) isn't equal to the number of lcores (%u) \n",
        sc_config->nb_rx_rings_per_port,
        sc_config->nb_tx_rings_per_port,
        sc_config->nb_used_cores
      );
      return SC_ERROR_INPUT;
  }

  /* check whether the core for logging is conflict with other worker cores */
  for(i=0; i<sc_config->nb_used_cores; i++){
    if(sc_config->core_ids[i] == sc_config->log_core_id){
      SC_ERROR_DETAILS("the core for logging is conflict with other worker cores");
      return SC_ERROR_INPUT;
    }
  }

  return SC_SUCCESS;
}

/*!
 * \brief   signal handler for stoping executing
 * \param   signum    index of the received signal
 */
static void _signal_handler(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		SC_WARNING("signal %d received, preparing to exit...\n", signum);
		force_quit = true;
	}
}