#ifndef _SC_ECHO_SERVER_H_
#define _SC_ECHO_SERVER_H_

/* for cpp linkage */
#ifdef __cplusplus
    extern "C" {
#endif

#include <sys/time.h>
#include <rte_ether.h>
#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_ethdev.h>
#include <rte_errno.h>
#include <rte_lcore.h>
#include <rte_mempool.h>
#include <rte_version.h>
#include <rte_mbuf_core.h>

#include "sc_global.hpp"

#define SC_ECHO_SERVER_NB_THROUGHPUT 131072

struct _per_core_app_meta {
    uint64_t nb_forward_pkt;
    uint64_t nb_drop_pkt;

    uint64_t nb_throughput;
    uint64_t throughput_pointer;
    double throughput[SC_ECHO_SERVER_NB_THROUGHPUT];
    double average_throughput;

    /* for calculating throughput */
    uint64_t nb_interval_forward_pkt;
    uint64_t nb_interval_drop_pkt;
    struct timeval last_record_time;
};

/* definition of internal config */
struct _internal_config {
    /* used echo ports */
    uint32_t nb_send_ports, nb_recv_ports;
    uint32_t send_port_idx[SC_MAX_NB_PORTS], recv_port_idx[SC_MAX_NB_PORTS];
    uint32_t send_port_logical_idx[SC_MAX_NB_PORTS], recv_port_logical_idx[SC_MAX_NB_PORTS];
    char *send_port_mac_address[SC_MAX_NB_PORTS];
    char *recv_port_mac_address[SC_MAX_NB_PORTS];
};

int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _all_exit(struct sc_config *sc_config);

/* for cpp linkage */
#ifdef __cplusplus
    }
#endif

#endif