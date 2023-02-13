#include <errno.h>
#include <string.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_common.h>

#include <gmp.h>

#include "sc_port.h"
#include "sc_mbuf.h"
#include "sc_utils.h"
#include "sc_global.h"
#include "sc_worker.h"

extern volatile bool force_quit;

static int _init_env(struct sc_config *sc_config, int argc, char **argv);
static int _check_configuration(struct sc_config *sc_config, int argc, char **argv);
static void _signal_handler(int signum);

int main(int argc, char **argv){
  FILE* fp = NULL;

  /* allocate memory space for storing configuration */
  struct app_config *app_config = (struct app_config*)malloc(sizeof(struct app_config));
  if(!app_config){
    rte_exit(EXIT_FAILURE, "failed to allocate memory for app_config: %s\n", strerror(errno));
  }
  memset(app_config, 0, sizeof(struct app_config));
  
  struct sc_config *sc_config = (struct sc_config*)malloc(sizeof(struct sc_config));
  if(!sc_config){
    rte_exit(EXIT_FAILURE, "failed to allocate memory for sc_config: %s\n", strerror(errno));
  }
  memset(sc_config, 0, sizeof(struct sc_config));
  sc_config->app_config = app_config;

  /* open configuration file */
  fp = fopen("../base.conf", "r");
  if(!fp){
    rte_exit(EXIT_FAILURE, "failed to open the configuration file: %s\n", strerror(errno));
  }

  /* parse configuration file */
  if(parse_config(fp, sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to parse the configuration file, exit\n");
  }

  /* check configurations */
  if(_check_configuration(sc_config, argc, argv) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "configurations check failed\n");
  }

  /* init environment */
  if(_init_env(sc_config, argc, argv) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize environment, exit\n");
  }

  /* initailize memory */
  if(init_memory(sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize memory, exit\n");
  }

  /* initailize ports */
  if(init_ports(sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize dpdk ports, exit\n");
  }

  /* initailize application */
  if(init_app(sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to config application: %s\n", strerror(errno));
  }

  /* initailize lcore threads */
  if(init_worker(sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize worker threads: %s\n", strerror(errno));
  }

  /* (sync/async) launch lcore threads */
  launch_worker(sc_config);

  rte_exit(EXIT_SUCCESS, "exit\n");

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
  
  /* check whether the specified lcores are located in the same NUMA socket */
  /* could be manually check through "numactl -H" */
  for(i=0; i<sc_config->nb_used_cores; i++){
    // TODO: rte_lcore_to_socket_id always return 0, is that a bug?
    if (i == 0) {
      socket_id = rte_lcore_to_socket_id(sc_config->core_ids[i]);
    } else {
      if (rte_lcore_to_socket_id(sc_config->core_ids[i]) != socket_id) {
        printf("specified lcores aren't locate at the same NUMA socket\n");
        return SC_ERROR_INPUT;
      }
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
		printf("\n\nsignal %d received, preparing to exit...\n", signum);
		force_quit = true;
	}
}