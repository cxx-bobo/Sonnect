#ifndef _SC_GLOBAL_H_
#define _SC_GLOBAL_H_

#include <stdlib.h>
#include <stdint.h>

#include <rte_mempool.h>

/* maximum number of parameters to init rte eal */
#define SC_RTE_ARGC_MAX (RTE_MAX_ETHPORTS << 1) + 7

/* maximum number of ports to used */
#define SC_MAX_NB_PORTS RTE_MAX_ETHPORTS

/* maximum number of queues per port */
#define SC_MAX_NB_QUEUE_PER_PORT RTE_MAX_QUEUES_PER_PORT

/* maximum number of lcores to used */
#define SC_MAX_NB_CORES RTE_MAX_LCORE

struct app_config {
    // TODO
};

struct sc_config {
    /* dpdk lcore */
    uint32_t core_ids[SC_MAX_NB_CORES];
    uint32_t nb_used_cores;

    /* dpdk port */
    char* port_mac[SC_MAX_NB_PORTS];
    uint16_t port_ids[SC_MAX_NB_PORTS];
    uint16_t nb_used_ports;
    uint16_t nb_rx_rings_per_port;
    uint16_t nb_tx_rings_per_port;
    bool enable_promiscuous;

    /* dpdk memory */
    struct rte_mempool *pktmbuf_pool;
    uint16_t nb_memory_channels_per_socket;

    /* app */
    struct app_config *app_config;
};

#endif