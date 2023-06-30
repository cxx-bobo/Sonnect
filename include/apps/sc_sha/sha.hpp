#ifndef _SC_SHA_H_
#define _SC_SHA_H_

#include "sc_app.hpp"
#include "sc_doca.hpp"
#include "sc_doca_utils/doca_utils.hpp"
#include "sc_doca_utils/mempool.hpp"

#define SC_SHA_HASH_KEY_LENGTH      64
#define SC_SHA_BURST_TX_RETRIES     16
#if defined(SC_HAS_DOCA)
    #define SHA_MEMPOOL_NB_BUF      512
    #define SHA_MEMPOOL_BUF_SIZE    SC_SHA_HASH_KEY_LENGTH + DOCA_SHA256_BYTE_COUNT
#endif

struct _per_core_app_meta {
    #if defined(SC_HAS_DOCA)
        struct mempool *mpool;
    #endif

    uint64_t nb_received_pkts;
    uint64_t nb_enqueued_pkts;
    uint64_t nb_drop_pkts;
    uint64_t nb_finished_pkts;
    uint64_t nb_send_pkts;

    int something;
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

#endif