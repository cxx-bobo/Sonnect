#ifndef _SC_SHA_H_
#define _SC_SHA_H_


#include <sys/time.h>

#if defined(SC_HAS_DOCA)
    #include "sc_doca.hpp"
    #include "sc_doca_utils/doca_utils.hpp"
#endif

#define SC_SHA_HASH_KEY_LENGTH 64

#if defined(MODE_LATENCY)
    #define SC_SHA_MAX_LATENCY_NB (1UL << 24)-1
#endif

struct _per_core_app_meta {
    uint32_t sha_state[8];

    #if defined(MODE_LATENCY)
        uint64_t nb_latency_data;
        uint64_t latency_data_pointer;
        long latency_sec[SC_SHA_MAX_LATENCY_NB];
        long latency_usec[SC_SHA_MAX_LATENCY_NB];

        long tail_latency_p99;
        long tail_latency_p80;
        long tail_latency_p50;
        long tail_latency_p10;
    #endif

    uint64_t nb_processed_pkt;
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
int _process_enter(struct sc_config *sc_config);
int _process_pkt(struct rte_mbuf **pkt, uint64_t nb_recv_pkts, struct sc_config *sc_config, uint16_t recv_port_id, uint16_t *fwd_port_id, uint64_t *nb_fwd_pkts);
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit);
int _process_exit(struct sc_config *sc_config);
int _all_exit(struct sc_config *sc_config);

void sha256_process_arm(uint32_t state[8], const uint8_t data[], uint32_t length);
void sha256_process_x86(uint32_t state[8], const uint8_t data[], uint32_t length);

#endif