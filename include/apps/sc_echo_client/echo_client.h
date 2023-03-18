#ifndef _SC_ECHO_CLIENT_H_
#define _SC_ECHO_CLIENT_H_

#include <sys/time.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_malloc.h>

#include "sc_utils.h"
#include "sc_utils/pktgen.h"

struct _per_core_app_meta {
    /* number of packet to be sent (per core) */
    uint32_t nb_pkt_budget_per_core;

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

    struct sc_pkt_hdr test_pkt;

    #if defined(MODE_LATENCY)
        long min_rtt_sec;
        long min_rtt_usec;
        long max_rtt_sec;
        long max_rtt_usec;
    #endif
};

/* definition of internal config */
struct _internal_config {
    uint32_t nb_pkt_budget;
    uint32_t pkt_len;           /* unit: bytes */
    uint32_t nb_pkt_per_burst;

    /* used echo ports */
    uint32_t nb_send_ports, nb_recv_ports;
    uint32_t send_port_idx[SC_MAX_NB_PORTS], recv_port_idx[SC_MAX_NB_PORTS];
    uint32_t send_port_logical_idx[SC_MAX_NB_PORTS], recv_port_logical_idx[SC_MAX_NB_PORTS];
    char *send_port_mac_address[SC_MAX_NB_PORTS];
    char *recv_port_mac_address[SC_MAX_NB_PORTS];
};

int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _process_enter(struct sc_config *sc_config);
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t *fwd_port_id, bool *need_forward);
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit);
int _process_exit(struct sc_config *sc_config);
int _all_exit(struct sc_config *sc_config);

#endif