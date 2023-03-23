#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"
#include "sc_utils/pktgen.h"
#include "sc_utils/tail_latency.h"

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

    __bubble_sort(latency, nb_latency, true);

    *p_latency = latency[p_nb_latency];

free_latency:
    free(latency);

sc_util_tail_latency_exit:
    return result;
}

int __bubble_sort(long *data, uint64_t length, bool is_increament){
    for(uint64_t i=length-1; i>=0; i--){
        bool sorted = false;
        for(uint64_t j=0; j<i; j++){
            // this is a XNOR operator, the truth table is:
            // | is_increament | j is larger |     opeartion    |
            // |      T        |      T      |   switch j & j+1 |
            // |      F        |      F      |   switch j & j+1 |
            // |      T        |      F      |     do nothing   |
            // |      F        |      T      |     do nothing   |
            if(is_increament == (data[j] > data[j+1])){
                long temp = data[j+1];
                data[j+1] = data[j];
                data[j] = temp;
                sorted = true;
            }
        }
        if(!sorted) break;
    }

    return SC_SUCCESS;
}