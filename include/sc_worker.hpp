#ifndef _SC_WORKER_H_
#define _SC_WORKER_H_

#include <unistd.h>

#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_ethdev.h>
#include <rte_errno.h>
#include <rte_lcore.h>
#include <rte_mempool.h>
#include <rte_version.h>
#include <rte_mbuf_core.h>

#define SC_MAX_RX_PKT_BURST 32
#define SC_MAX_TX_PKT_BURST 32
#define SC_NUM_PRIVATE_MBUFS_PER_CORE 32767
#define SC_BURST_TX_RETRIES 16

int init_worker_threads(struct sc_config *sc_config);
int launch_worker_threads(struct sc_config *sc_config);
int launch_worker_threads_async(struct sc_config *sc_config);

#endif