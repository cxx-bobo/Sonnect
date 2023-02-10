#include <rte_eal.h>
#include <rte_common.h>

#include "sc_port.h"
#include "sc_utils.h"

int main(int argc, char **argv){
	int ret;

  /* init rte */
  ret = rte_eal_init(argc, argv);
  if (ret < 0)
  rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");

  /* initailize ports */
  if(init_ports() != SC_SUCCESS){
    rte_exit(EXIT_FAILURE, "failed to initialize dpdk ports, exit\n");
  }

  return 0;
}