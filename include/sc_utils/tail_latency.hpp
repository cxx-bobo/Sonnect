#ifndef _SC_UTILS_TAIL_LATENCY_H_
#define _SC_UTILS_TAIL_LATENCY_H_

int sc_util_tail_latency(
    long *latency_sec, long *latency_usec, long *p_latency, uint64_t nb_latency, float percent
);

#endif 