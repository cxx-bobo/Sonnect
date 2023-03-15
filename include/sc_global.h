#ifndef _SC_GLOBAL_H_
#define _SC_GLOBAL_H_

#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_version.h>
#include <rte_lcore.h>

extern volatile bool sc_force_quit;

/* 
 * the compiler will define SC_CLOSE_MOCK_MACRO 
 * automatically while build the final binary 
 */
#if !defined(SC_CLOSE_MOCK_MACRO)
    #define SC_HAS_DOCA
    #define SC_NEED_DOCA_SHA
#endif // SC_CLOSE_MOCK_MACRO

/* maximum number of parameters to init rte eal */
#define SC_RTE_ARGC_MAX (RTE_MAX_ETHPORTS << 1) + 7

/* maximum number of ports to used */
#define SC_MAX_NB_PORTS RTE_MAX_ETHPORTS

/* maximum number of queues per port */
#define SC_MAX_NB_QUEUE_PER_PORT RTE_MAX_QUEUES_PER_PORT

/* maximum number of lcores to used */
#define SC_MAX_NB_CORES RTE_MAX_LCORE

struct app_config;
struct doca_config;
struct per_core_meta;

/* global configuration of SoConnect */
struct sc_config {
    /* dpdk lcore */
    uint32_t core_ids[SC_MAX_NB_CORES];
    uint32_t nb_used_cores;

    /* logging */
    uint32_t log_core_id;
    pthread_t *logging_thread;
    pthread_mutex_t *timer_mutex;

    /* dpdk port */
    char* port_mac[SC_MAX_NB_PORTS];
    uint16_t port_ids[SC_MAX_NB_PORTS];
    uint16_t nb_conf_ports; // number of ports specified inside configuration file
    uint16_t nb_used_ports; // number of initialized ports eventually
    uint16_t nb_rx_rings_per_port;
    uint16_t nb_tx_rings_per_port;
    uint32_t rx_queue_len;
    uint32_t tx_queue_len;
    bool enable_promiscuous;

    /* rss */
    bool enable_rss;
    bool rss_symmetric_mode;    // true: symmetric; false: asymmetric
    uint64_t rss_hash_field;

    /* dpdk memory */
    struct rte_mempool *pktmbuf_pool;
    uint16_t nb_memory_channels_per_socket;

    /* application global configuration */
    struct app_config *app_config;

    /* application per-core metadata */
    void *per_core_app_meta;

    /* per-core metadata */
    struct per_core_meta *per_core_meta;

    /* per-core memory buffer pool */
    struct rte_mempool *per_core_pktmbuf_pool;

    /* pthread barrier for sync all worker thread */
    pthread_barrier_t pthread_barrier;

    /* test duration (of worker) */
    bool enable_test_duration_limit;
    uint64_t test_duration;
    struct timeval test_duration_start_time;
    struct timeval test_duration_end_time;

    /* doca specific configurations */
    #if defined(SC_HAS_DOCA)
        void *doca_config;
    #endif
};
#define PER_CORE_META(scc) ((struct per_core_meta*)scc->per_core_meta)[rte_lcore_index(rte_lcore_id())]
#define PER_CORE_MBUF_POOL(scc) (PER_CORE_META(scc)).pktmbuf_pool
#define PER_CORE_APP_META(scc) ((struct _per_core_app_meta*)scc->per_core_app_meta)[rte_lcore_index(rte_lcore_id())]
#define INTERNAL_CONF(scc) ((struct _internal_config*)scc->app_config->internal_config)

/* application specific configuration */
struct app_config {
    /* callback function: operations while entering the worker loop */
    int (*process_enter)(struct sc_config *sc_config);
    /* callback function: processing single received packet (server mode) */
    int (*process_pkt)(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t *fwd_port_id, bool *need_forward);
    /* callback function: client logic (client mode) */
    int (*process_client)(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit);
    /* callback function: operations while exiting the worker loop */
    int (*process_exit)(struct sc_config *sc_config);

    /* internal configuration of the application */
    void *internal_config;
};

/* per-core metadata */
struct per_core_meta {
    /* core id starts from 0 */
    uint64_t core_seq_id;

    /* name of the private memory buffer */
    char *mbuf_pool_name;

    /* per-core memory pool */
    struct rte_mempool *pktmbuf_pool
};

#endif // _SC_GLOBAL_H_