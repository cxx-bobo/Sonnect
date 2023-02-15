#ifndef _SC_SKETCH_H_
#define _SC_SKETCH_H_

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

struct hash_field {
    struct rte_ether_hdr    *eth_hdr;
    struct rte_ipv4_hdr     *ipv4_hdr;
    struct rte_udp_hdr      *udp_hdr;
};

/* common sketch components */
struct _sketch_core {
    int (*update)(const char* key, struct sc_config *sc_config);
    int (*query)(const char* key, void *result, struct sc_config *sc_config);
    int (*clean)(struct sc_config *sc_config);
};

/* memory of count-min sketch */
struct cm_sketch {
    counter_t *counters;
    rte_spinlock_t lock;
    uint32_t *hash_seeds;
};

/* =================== Application Interfaces =================== */

/* definition of internal config */
struct _internal_config {
    uint16_t sketch_type;
    struct _sketch_core sketch_core;
    /* cm sketch related config */
    uint32_t cm_nb_rows;
    uint32_t cm_nb_counters_per_row;
    struct cm_sketch *cm_sketch;
};
#define INTERNAL_CONF(scc) ((struct _internal_config*)scc->app_config->internal_config)

/* must-provided interfaces */
int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config);

/* ============================================================== */

#endif