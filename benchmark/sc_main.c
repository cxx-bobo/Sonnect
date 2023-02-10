#include <rte_eal.h>
#include <rte_common.h>

int main(int argc, char **argv){
	int ret;

    /* init rte */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");

    /* check available ports */
    
}