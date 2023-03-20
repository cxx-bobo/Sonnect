#ifndef _SC_SHA_H_
#define _SC_SHA_H_

#if defined(SC_HAS_DOCA)
    #include "sc_doca.h"
    #include "sc_doca_utils.h"
#endif

#define SC_SHA_HASH_KEY_LENGTH 57

struct _per_core_app_meta {
    int something;
};

/* definition of internal config */
struct _internal_config {
    uint32_t sha_state[8];
};

int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _process_enter(struct sc_config *sc_config);
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t recv_port_id, uint16_t *fwd_port_id, bool *need_forward);
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit);
int _process_exit(struct sc_config *sc_config);
int _all_exit(struct sc_config *sc_config);

#if defined(SC_HAS_DOCA) && !defined(SC_NEED_DOCA_SHA)
    void sha256_process_arm(uint32_t state[8], const uint8_t data[], uint32_t length);
#else
    void sha256_process_x86(uint32_t state[8], const uint8_t data[], uint32_t length);
#endif

#endif