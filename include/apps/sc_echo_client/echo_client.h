#ifndef _SC_ECHO_CLIENT_H_
#define _SC_ECHO_CLIENT_H_

#include <sys/time.h>
#include <rte_ether.h>
#include <rte_malloc.h>

#include "sc_utils.h"

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

    /* pre-generated ethernet header */
    char src_ether_addr[6], dst_ether_addr[6];
    struct rte_ether_hdr pkt_eth_hdr;

    /* pre-generated ipv4 header */
    uint32_t src_ipv4_addr, dst_ipv4_addr;
    struct rte_ipv4_hdr pkt_ipv4_hdr;

    /* pre-generated udp header */
    uint16_t src_port, dst_port;
    struct rte_udp_hdr pkt_udp_hdr;

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
};

int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _process_enter(struct sc_config *sc_config);
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t *fwd_port_id, bool *need_forward);
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit);
int _process_exit(struct sc_config *sc_config);

#endif