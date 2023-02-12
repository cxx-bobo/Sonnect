#ifndef _SC_GLOBAL_H_
#define _SC_GLOBAL_H_

#include <stdlib.h>
#include <stdint.h>

#include <rte_mempool.h>

#define SC_MAX_NB_PORTS 4

struct sc_config {
    /* port */
    char* port_mac[SC_MAX_NB_PORTS];
    uint16_t nb_ports;
    uint16_t port_ids[SC_MAX_NB_PORTS];
    uint16_t nb_used_ports;
    uint16_t nb_rx_rings_per_port;
    uint16_t nb_tx_rings_per_port;
    bool enable_promiscuous;

    /* memory */
    struct rte_mempool *pktmbuf_pool;
};

#endif