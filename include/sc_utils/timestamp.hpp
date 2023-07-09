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
    uint64_t raw_payload[2];
};

// /*!
//  * \brief   add a new short timestamp to the timestamp table
//  * \param   sc_ts               the target timestamp table
//  * \param   new_timestamp_ns    the full-length 64-bits nanosecond timestamp
//  */
// inline void sc_util_add_short_timestamp(struct sc_timestamp_table* sc_ts, uint64_t new_timestamp_ns){
//     // assert(sc_ts->timestamp_type == SC_TIMESTAMP_SHORT_TYPE);
//     // assert(sc_ts->nb_timestamp < 10);
//     rte_memcpy(
//         /* dst */ &(sc_ts->raw_payload[sc_ts->nb_timestamp*SC_SHORT_TIMESTAMP_LEN]),
//         /* src */ &new_timestamp_ns,
//         /* size */ SC_SHORT_TIMESTAMP_LEN
//     );
//     sc_ts->nb_timestamp += 1;
// }

// /*!
//  * \brief   add a new half timestamp to the timestamp table
//  * \param   sc_ts               the target timestamp table
//  * \param   new_timestamp_ns    the full-length 64-bits nanosecond timestamp
//  * \FIXME: this function will take 28 ~ 1348 CPU cycles, should be optimized
//  */
// inline void sc_util_add_half_timestamp(struct sc_timestamp_table* sc_ts, uint64_t new_timestamp_ns){
//     // assert(sc_ts->timestamp_type == SC_TIMESTAMP_HALF_TYPE);
//     // assert(sc_ts->nb_timestamp < 5);
//     rte_memcpy(
//         /* dst */ &(sc_ts->raw_payload[sc_ts->nb_timestamp*SC_HALF_TIMESTAMP_LEN]),
//         /* src */ &new_timestamp_ns,
//         /* size */ SC_HALF_TIMESTAMP_LEN
//     );
//     sc_ts->nb_timestamp += 1;
// }

/*!
 * \brief   add a new full timestamp to the timestamp table
 * \param   sc_ts               the target timestamp table
 * \param   new_timestamp_ns    the full-length 64-bits nanosecond timestamp
 */
inline void sc_util_add_full_timestamp(struct sc_timestamp_table* sc_ts, uint64_t new_timestamp_ns){
    // assert(sc_ts->timestamp_type == SC_TIMESTAMP_FULL_TYPE);
    // assert(sc_ts->nb_timestamp < 2);
    sc_ts->raw_payload[sc_ts->nb_timestamp] = new_timestamp_ns;
    sc_ts->nb_timestamp += 1;
}

// /*!
//  * \brief   get a short timestamp from the table according to specified index
//  * \param   sc_ts   the target timestamp table
//  * \param   index   index of the target timestamp
//  * \return  the result timestamp
//  */
// inline uint16_t sc_util_get_short_timestamp(struct sc_timestamp_table* sc_ts, uint8_t index){
//     assert(sc_ts->timestamp_type == SC_TIMESTAMP_SHORT_TYPE);
//     assert(index < sc_ts->nb_timestamp);
//     uint8_t *payload = sc_ts->raw_payload;
//     return (uint16_t) (
//         payload[index * SC_SHORT_TIMESTAMP_LEN]
//         + (payload[index * SC_SHORT_TIMESTAMP_LEN + 1] << 8)
//     );
// }

// /*!
//  * \brief   get a half timestamp from the table according to specified index
//  * \param   sc_ts   the target timestamp table
//  * \param   index   index of the target timestamp
//  * \return  the result timestamp
//  */
// inline uint32_t sc_util_get_half_timestamp(struct sc_timestamp_table* sc_ts, uint8_t index){
//     assert(sc_ts->timestamp_type == SC_TIMESTAMP_HALF_TYPE);
//     assert(index < sc_ts->nb_timestamp);
//     uint8_t *payload = sc_ts->raw_payload;
//     return (uint32_t) (
//         payload[index * SC_HALF_TIMESTAMP_LEN]
//         + (payload[index * SC_HALF_TIMESTAMP_LEN + 1] << 8)
//         + (payload[index * SC_HALF_TIMESTAMP_LEN + 2] << 16)
//         + (payload[index * SC_HALF_TIMESTAMP_LEN + 3] << 24)
//     );
// }

/*!
 * \brief   get a full timestamp from the table according to specified index
 * \param   sc_ts   the target timestamp table
 * \param   index   index of the target timestamp
 * \return  the result timestamp
 */
inline uint64_t sc_util_get_full_timestamp(struct sc_timestamp_table* sc_ts, uint8_t index){
    assert(sc_ts->timestamp_type == SC_TIMESTAMP_FULL_TYPE);
    assert(index < sc_ts->nb_timestamp);
    return sc_ts->raw_payload[index];
}


/*!
 * \brief   obtain nanosecond timestamp
 * \return  nanosecond timestamp
 */
inline uint64_t sc_util_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>
              (std::chrono::steady_clock::now().time_since_epoch()).count();
}

#endif