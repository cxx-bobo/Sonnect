#ifndef _SC_SKETCH_H_
#define _SC_SKETCH_H_

#include "sc_global.h"

/* type of the sketch counter */
typedef uint64_t    l_counter_t;   // 8 bytes
typedef uint32_t    m_counter_t;   // 4 bytes
typedef uint16_t    s_counter_t;   // 2 bytes
typedef uint8_t     t_counter_t;   // 1 bytes

struct hash_field {
    
};

struct sketch_core {
    int (*update)(struct hash_field *hash_field);
    int (*query)(struct hash_field *hash_field, void *result);
};

struct cm_sketch {
    struct sketch_core *sketch_core;
    m_counter_t *counters;
};

int _init_app(struct sc_config *sc_config);

#endif