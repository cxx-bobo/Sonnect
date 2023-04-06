#ifndef _SC_MBUF_H_
#define _SC_MBUF_H_

#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_mbuf_core.h>

#define NUM_MBUFS 8191
#define MEMPOOL_CACHE_SIZE 512

/* See: http://dpdk.org/dev/patchwork/patch/4479 */
// #define DEFAULT_PRIV_SIZE   0
// #define MBUF_SIZE           RTE_MBUF_DEFAULT_BUF_SIZE + DEFAULT_PRIV_SIZE

int init_memory(struct sc_config *sc_config);

#endif