#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils/tail_latency.hpp"
#include "sc_utils/sort.hpp"

int __bubble_sort(long *data, uint64_t length, bool is_increament);

int sc_util_tail_latency(
        long *latency_sec, long *latency_usec, long *p_latency, uint64_t nb_latency, float percent
){
    int result = SC_SUCCESS, i;
    uint64_t p_nb_latency = nb_latency * percent;

    long *latency = (long*)malloc(sizeof(long)*nb_latency);
    if(unlikely(!latency)){
        SC_ERROR_DETAILS("failed to allocate memory for latency");
        result = SC_ERROR_MEMORY;
        goto sc_util_tail_latency_exit;
    }
    memset(latency, 0, sizeof(long)*nb_latency);

    for(i=0; i<nb_latency; i++){
        latency[i] = SC_UTIL_TIME_INTERVL_US(
            latency_sec[i], latency_usec[i]
        );
        // SC_THREAD_LOG("latency[%d]: %ld", i, latency[i]);
        // SC_THREAD_LOG("latency_sec[%d]: %ld", i, latency_sec[i]);
        // SC_THREAD_LOG("latency_usec[%d]: %ld", i, latency_usec[i]);
    }

    if(SC_SUCCESS != sc_util_merge_sort_long(latency, nb_latency, true)){
        SC_THREAD_ERROR("failed to sort latency result");
        result = SC_ERROR_INTERNAL;
        goto free_latency;
    }

    *p_latency = latency[p_nb_latency];

free_latency:
    free(latency);

sc_util_tail_latency_exit:
    return result;
}