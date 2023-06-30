#ifndef _SC_SHA_H_
#define _SC_SHA_H_

#include "sc_app.hpp"
#include "sc_doca.hpp"
#include "sc_doca_utils/doca_utils.hpp"
#include "sc_doca_utils/mempool.hpp"

#define SC_SHA_HASH_KEY_LENGTH 64

#if defined(SC_HAS_DOCA)
    #define MEMPOOL_NB_BUF      128
    #define MEMPOOL_BUF_SIZE    256
#endif

struct _per_core_app_meta {
    #if defined(SC_HAS_DOCA)
        struct mempool *mpool;
    #endif

    int something;
};

/* definition of internal config */
struct _internal_config {
    int something;
};

int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _all_exit(struct sc_config *sc_config);

#endif