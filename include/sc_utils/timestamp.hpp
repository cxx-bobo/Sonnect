#ifndef _SC_UTILS_TIMESTAMP_H_
#define _SC_UTILS_TIMESTAMP_H_

#include <algorithm>
#include <chrono>

#include "sc_utils.hpp"

#define SC_FULL_TIMESTAMP_LEN 8
#define SC_HALF_TIMESTAMP_LEN 4     /* 4,294,967,296 ns */
#define SC_SHORT_TIMESTAMP_LEN 2    /* 65536 ns */

enum {
    SC_TIMESTAMP_FULL_TYPE = 0,
    SC_TIMESTAMP_HALF_TYPE,
    SC_TIMESTAMP_SHORT_TYPE,
};

struct sc_timestamp_table {
    uint8_t nb_timestamp;
    uint8_t timestamp_type;
    uint8_t raw_payload[20];
};

/*!
 * \brief   add a new short timestamp to the timestamp table
 * \param   sc_ts               the target timestamp table
 * \param   new_timestamp_ns    the full-length 64-bits nanosecond timestamp
 */
__inline__ void sc_util_add_short_timestamp(struct sc_timestamp_table* sc_ts, uint64_t new_timestamp_ns){
    uint8_t nb_timestamp = sc_ts->nb_timestamp;
    uint8_t j;
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_SHORT_TYPE);
    assert(nb_timestamp < 10);
    for(j=0; j<SC_SHORT_TIMESTAMP_LEN; j++){ 
        sc_ts->raw_payload[j+nb_timestamp*SC_SHORT_TIMESTAMP_LEN] 
            = sc_util_get_ith_byte<uint64_t, uint8_t>(new_timestamp_ns, j);
    }
    sc_ts->nb_timestamp += 1;
}

/*!
 * \brief   add a new half timestamp to the timestamp table
 * \param   sc_ts               the target timestamp table
 * \param   new_timestamp_ns    the full-length 64-bits nanosecond timestamp
 */
__inline__ void sc_util_add_half_timestamp(struct sc_timestamp_table* sc_ts, uint64_t new_timestamp_ns){
    uint8_t nb_timestamp = sc_ts->nb_timestamp;
    uint8_t j;
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_HALF_TYPE);
    assert(nb_timestamp < 5);
    for(j=0; j<SC_HALF_TIMESTAMP_LEN; j++){ 
        sc_ts->raw_payload[j+nb_timestamp*SC_HALF_TIMESTAMP_LEN] 
            = sc_util_get_ith_byte<uint64_t, uint8_t>(new_timestamp_ns, j);
    }
    
    sc_ts->nb_timestamp += 1;
}

/*!
 * \brief   add a new full timestamp to the timestamp table
 * \param   sc_ts               the target timestamp table
 * \param   new_timestamp_ns    the full-length 64-bits nanosecond timestamp
 */
__inline__ void sc_util_add_full_timestamp(struct sc_timestamp_table* sc_ts, uint64_t new_timestamp_ns){
    uint8_t nb_timestamp = sc_ts->nb_timestamp;
    uint8_t j;
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_FULL_TYPE);
    assert(nb_timestamp < 2);
    for(j=0; j<SC_FULL_TIMESTAMP_LEN; j++){ 
        sc_ts->raw_payload[j+nb_timestamp*SC_FULL_TIMESTAMP_LEN] 
            = sc_util_get_ith_byte<uint64_t, uint8_t>(new_timestamp_ns, j);
    }
    
    sc_ts->nb_timestamp += 1;
}

/*!
 * \brief   get a short timestamp from the table according to specified index
 * \param   sc_ts   the target timestamp table
 * \param   index   index of the target timestamp
 * \return  the result timestamp
 */
__inline__ uint16_t sc_util_get_short_timestamp(struct sc_timestamp_table* sc_ts, uint8_t index){
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_SHORT_TYPE);
    assert(index < sc_ts->nb_timestamp);
    uint8_t *payload = sc_ts->raw_payload;
    return (uint16_t) (
        payload[index * SC_SHORT_TIMESTAMP_LEN]
        + (payload[index * SC_SHORT_TIMESTAMP_LEN + 1] << 8)
    );
}

/*!
 * \brief   get a half timestamp from the table according to specified index
 * \param   sc_ts   the target timestamp table
 * \param   index   index of the target timestamp
 * \return  the result timestamp
 */
__inline__ uint32_t sc_util_get_half_timestamp(struct sc_timestamp_table* sc_ts, uint8_t index){
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_HALF_TYPE);
    assert(index < sc_ts->nb_timestamp);
    uint8_t *payload = sc_ts->raw_payload;
    return (uint32_t) (
        payload[index * SC_HALF_TIMESTAMP_LEN]
        + (payload[index * SC_HALF_TIMESTAMP_LEN + 1] << 8)
        + (payload[index * SC_HALF_TIMESTAMP_LEN + 2] << 16)
        + (payload[index * SC_HALF_TIMESTAMP_LEN + 3] << 24)
    );
}

/*!
 * \brief   get a full timestamp from the table according to specified index
 * \param   sc_ts   the target timestamp table
 * \param   index   index of the target timestamp
 * \return  the result timestamp
 */
__inline__ uint64_t sc_util_get_full_timestamp(struct sc_timestamp_table* sc_ts, uint8_t index){
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_FULL_TYPE);
    assert(index < sc_ts->nb_timestamp);
    uint8_t *payload = sc_ts->raw_payload;
    return (uint64_t) (
        payload[index * SC_FULL_TIMESTAMP_LEN]
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 1] << 8)
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 2] << 16)
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 3] << 24)
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 4] << 32)
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 5] << 40)
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 6] << 48)
        + (payload[index * SC_FULL_TIMESTAMP_LEN + 7] << 56)
    );
}


/*!
 * \brief   obtain nanosecond timestamp
 * \return  nanosecond timestamp
 */
inline uint64_t sc_util_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>
              (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#endif