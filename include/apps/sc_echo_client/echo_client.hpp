#ifndef _SC_ECHO_CLIENT_H_
#define _SC_ECHO_CLIENT_H_

#include <sys/time.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_malloc.h>

#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils/distribution_gen.hpp"


#define SC_ECHO_CLIENT_MAX_LATENCY_NB (1UL << 16)-1

#define SC_ECHO_CLIENT_PAYLOAD_LEN 24

struct _per_core_app_meta {
    /* store rte_mbuf for sending and receiving */
    struct rte_mbuf **send_pkt_bufs; 
    struct rte_mbuf **recv_pkt_bufs;
    
    /* record whether has packet sent but not ack confirmed */
    uint64_t wait_ack;

    /* record the total number of send packets */
    uint64_t nb_send_pkt;
    uint64_t nb_last_send_pkt;
    uint64_t nb_confirmed_pkt;

    /* record to total period */
    struct timeval start_time;
    struct timeval end_time;
    struct timeval last_send_time;

    /* last send flow */
    uint64_t last_used_flow;
    struct sc_pkt_hdr *test_pkts;

    /* send interval */
    uint64_t last_send_timestamp;
    uint64_t interval;
    sc_utils_distribution_uint64_generator* interval_generator;
    
    uint64_t nb_latency_data;
    uint64_t latency_data_pointer;
    uint64_t latency_ns[SC_ECHO_CLIENT_MAX_LATENCY_NB];

    #if defined(MODE_LATENCY)
        long min_rtt_sec;
        long min_rtt_usec;
        long max_rtt_sec;
        long max_rtt_usec;

        /* calculate tail latency */
        uint64_t nb_latency_data;
        uint64_t latency_data_pointer;
        long latency_sec[SC_ECHO_CLIENT_MAX_LATENCY_NB];
        long latency_usec[SC_ECHO_CLIENT_MAX_LATENCY_NB];

        long tail_latency_p99;
        long tail_latency_p80;
        long tail_latency_p50;
        long tail_latency_p10;
    #endif
};

/* definition of internal config */
struct _internal_config {
    uint32_t pkt_len;           /* unit: bytes */
    uint32_t nb_pkt_per_burst;
    uint64_t nb_flow_per_core;

    /* send flow rate */
    double bit_rate;

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

#endif