#ifndef _SC_SKETCH_H_
#define _SC_SKETCH_H_

#include <sys/time.h>

#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#include "sc_global.h"

/* type of the sketch counter */
typedef uint64_t    l_counter_t;   // 8 bytes
typedef uint32_t    m_counter_t;   // 4 bytes
typedef uint16_t    s_counter_t;   // 2 bytes
typedef uint8_t     t_counter_t;   // 1 bytes

typedef m_counter_t counter_t;

/* enumerate of all available sketch types */
enum {
    SC_SKETCH_TYPE_CM = 0,  // Count-min Sketch
};

#define TUPLE_KEY_LENGTH 57

/* common sketch components */
struct _sketch_core {
    /* update the sketch structure using a specific key */
    int (*update)(const char* key, struct sc_config *sc_config);
    /* query the sketch structure using a specific key */
    int (*query)(const char* key, void *result, struct sc_config *sc_config);
    /* clean the sketch structure */
    int (*clean)(struct sc_config *sc_config);
    /* record the actual value for a specific key */
    int (*record)(const char* key, struct sc_config *sc_config);
    /* evaluate the throughput/latency/accuracy of the sketch */
    int (*evaluate)(struct sc_config *sc_config);
};

/* memory of count-min sketch */
struct cm_sketch {
    counter_t *counters;
    rte_spinlock_t lock;
    uint32_t *hash_seeds;
};

/* =================== Application Interfaces =================== */

/* per-core metadata of all sketches */
struct _per_core_meta {
    /* throughput measure: number of processed packet/bytes */
    uint64_t nb_pkts;
    uint64_t nb_bytes;

    /* latency measure: number of processed packet/bytes */
    struct timeval overall_pkt_process;
    struct timeval overall_hash;
    struct timeval overall_lock;
    struct timeval overall_update;
};
#define PER_CORE_META(scc) ((struct _per_core_meta*)scc->per_core_meta)[rte_lcore_id()]

/* definition of internal config */
struct _internal_config {
    struct _sketch_core sketch_core;
    
    /* type of the used sketch */
    uint16_t sketch_type;

    /* executing mode */
    uint16_t sketch_mode;
    
    /* number of processed packet/bytes */
    uint64_t nb_pkts;
    uint64_t nb_bytes;

    /* cm sketch related config */
    uint32_t cm_nb_rows;
    uint32_t cm_nb_counters_per_row;
    struct cm_sketch *cm_sketch;
};
#define INTERNAL_CONF(scc) ((struct _internal_config*)scc->app_config->internal_config)

/* must-provided interfaces */
int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _process_enter(struct sc_config *sc_config);
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config);
int _process_exit(struct sc_config *sc_config);

/* ============================================================== */

#endif