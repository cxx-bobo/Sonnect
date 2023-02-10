#include "sc_port.h"

void init_ports(){
    uint8_t port_index, nb_ports;
    struct rte_eth_dev_info dev_info;

    // check available ports
    nb_ports = rte_eth_dev_count_avail();
    if(nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports\n");
    
    for(port_index=0; port_index<nb_ports; port_index++){
        
    }
}