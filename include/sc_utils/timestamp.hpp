#ifndef _SC_UTILS_TIMESTAMP_H_
#define _SC_UTILS_TIMESTAMP_H_

/* cpp header */
#ifdef __cplusplus
    #include <algorithm>
    #include <chrono>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

/*!
 * \brief   obtain nanosecond timestamp
 * \return  nanosecond timestamp
 */
inline uint64_t sc_util_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>
              (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#ifdef __cplusplus
    }
#endif

#endif