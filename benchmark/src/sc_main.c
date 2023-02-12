#include <errno.h>
#include <string.h>
#include <rte_eal.h>
#include <rte_common.h>

#include "sc_port.h"
#include "sc_mbuf.h"
#include "sc_utils.h"
#include "sc_global.h"

int main(int argc, char **argv){
	int ret;
  FILE* fp = NULL;

  struct sc_config* sc_config = (struct sc_config*)malloc(sizeof(struct sc_config));
  if(!sc_config){
    printf("Failed to allocate memory for sc_config: %s\n", strerror(errno));
    return SC_ERROR_MEMORY;
  }
  memset(sc_config, 0, sizeof(struct sc_config));

  fp = fopen("../base.conf", "r");
  if(!fp){
    printf("Failed to open the configuration file: %s\n", strerror(errno));
    return SC_ERROR_NOT_EXIST;
  }

  /* parse configuration file */
  if(parse_config(fp, sc_config) != SC_SUCCESS){
    printf("Failed to parse the configuration file\n");
    return SC_ERROR_INTERNAL;
  }

  /* init rte */
  ret = rte_eal_init(argc, argv);
  if (ret < 0)
  rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");

  /* initailize memory */
  if(init_memory(sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize memory, exit\n");
  }

  /* initailize ports */
  if(init_ports(sc_config) != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize dpdk ports, exit\n");
  }

  rte_exit(EXIT_SUCCESS, "exit\n");

  return 0;
}