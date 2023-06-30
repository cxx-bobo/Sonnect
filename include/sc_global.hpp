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
#include <rte_ethdev.h>

extern volatile bool sc_force_quit;

/* 
 * the compiler will define SC_CLOSE_MOCK_MACRO 
 * automatically while build the final binary 
 */
#if !defined(SC_CLOSE_MOCK_MACRO)
    #define SC_HAS_DOCA
    #define MODE_THROUGHPUT
    #define MODE_LATENCY
#endif // SC_CLOSE_MOCK_MACRO

/* maximum number of parameters to init rte eal */
#define SC_RTE_ARGC_MAX (RTE_MAX_ETHPORTS << 1) + 7

/* maximum number of ports to used */
#define SC_MAX_NB_PORTS RTE_MAX_ETHPORTS

/* maximum number of queues per port */
#define SC_MAX_NB_QUEUE_PER_PORT RTE_MAX_QUEUES_PER_PORT

/* maximum number of lcores to used */
#define SC_MAX_NB_CORES RTE_MAX_LCORE

#define RX_QUEUE_MEMORY_POOL_ID(scc, logical_port_id, core_id) \
    logical_port_id*scc->nb_rx_rings_per_port+core_id

#define TX_QUEUE_MEMORY_POOL_ID(scc, logical_port_id, core_id) \
    logical_port_id*scc->nb_tx_rings_per_port+core_id

struct app_config;
struct doca_config;
struct per_core_meta;
struct per_core_worker_func;

/*!
 * \brief meta of a dpdk port
 */
struct sc_port {
    uint16_t port_id;
    uint16_t logical_port_id;
    char port_mac[RTE_ETHER_ADDR_FMT_SIZE];
};

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
    struct sc_port sc_port[SC_MAX_NB_PORTS];
    uint16_t port_ids[SC_MAX_NB_PORTS];
    uint16_t nb_conf_ports; // number of ports specified inside configuration file
    uint16_t nb_used_ports; // number of initialized ports eventually
    uint16_t nb_rx_rings_per_port;
    uint16_t nb_tx_rings_per_port;
    uint32_t rx_queue_len;
    uint32_t tx_queue_len;
    bool enable_promiscuous;
    bool enable_offload;

    /* rss */
    bool enable_rss;
    bool rss_symmetric_mode;    // true: symmetric; false: asymmetric
    uint64_t rss_hash_field;

    /* dpdk memory */
    struct rte_mempool **rx_pktmbuf_pool;  // index: port_id * nb_rx_rings_per_port + queue_id
    struct rte_mempool **tx_pktmbuf_pool;  // index: port_id * nb_tx_rings_per_port + queue_id
    uint16_t nb_memory_channels_per_socket;

    /* application global configuration */
    struct app_config *app_config;

    /* application per-core metadata */
    void *per_core_app_meta;

    /* worker functions dispatching */
    struct per_core_worker_func *per_core_worker_funcs;

    /* per-core metadata */
    struct per_core_meta *per_core_meta;

    /* pthread barrier for sync all worker thread */
    pthread_barrier_t pthread_barrier;

    /* test duration (of worker) */
    bool enable_test_duration_limit;
    uint64_t test_duration;
    struct timeval test_duration_start_time;
    struct timeval test_duration_end_time;
    struct timeval test_duration_tick_time;

    /* doca specific configurations */
    #if defined(SC_HAS_DOCA)
        void *doca_config;
    #endif
};
#define PER_CORE_META(scc) ((struct per_core_meta*)scc->per_core_meta)[rte_lcore_index(rte_lcore_id())]
#define PER_CORE_META_BY_CORE_ID(scc, id) ((struct per_core_meta*)scc->per_core_meta)[id]

#define PER_CORE_RX_MBUF_POOL(scc, logical_port_id) \
    scc->rx_pktmbuf_pool[RX_QUEUE_MEMORY_POOL_ID(scc,logical_port_id,rte_lcore_index(rte_lcore_id()))]
#define PER_CORE_TX_MBUF_POOL(scc, logical_port_id) \
    scc->tx_pktmbuf_pool[TX_QUEUE_MEMORY_POOL_ID(scc,logical_port_id,rte_lcore_index(rte_lcore_id()))]

#define PER_CORE_APP_META(scc) ((struct _per_core_app_meta*)scc->per_core_app_meta)[rte_lcore_index(rte_lcore_id())]
#define PER_CORE_APP_META_BY_CORE_ID(scc, id) ((struct _per_core_app_meta*)scc->per_core_app_meta)[id]

#define PER_CORE_WORKER_FUNC(scc)((struct per_core_worker_func*)scc->per_core_worker_funcs)\
        [rte_lcore_index(rte_lcore_id())]
#define PER_CORE_WORKER_FUNC_BY_CORE_ID(scc, id)((struct per_core_worker_func*)scc->per_core_worker_funcs)\
        [id]

#define INTERNAL_CONF(scc) ((struct _internal_config*)scc->app_config->internal_config)

/* application specific configuration */
struct app_config {    
    /* callback function (main thread): operations while all thread exits */
    int (*all_exit)(struct sc_config *sc_config);

    /* internal configuration of the application */
    void *internal_config;
};

/* per-core metadata */
struct per_core_meta {
    int something;
};

/* function pointer definition, for dispatching different logic to different cores */
typedef int (*process_enter_t)(struct sc_config *sc_config);
typedef int (*process_exit_t)(struct sc_config *sc_config);
typedef int (*process_pkt_t)(struct rte_mbuf **pkt, uint64_t nb_recv_pkts, struct sc_config *sc_config, 
                            uint16_t queue_id, uint16_t recv_port_id, uint16_t *fwd_port_id, uint64_t *nb_fwd_pkts);
typedef int (*process_pkt_drop_t)(struct sc_config *sc_config, struct rte_mbuf **pkt, uint64_t nb_drop_pkts);
typedef int (*process_client_t)(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit);


/* dispatch different woker logic to different cores */
struct per_core_worker_func {
    // common functions
    process_enter_t     process_enter_func;
    process_exit_t      process_exit_func;

    // server functions
    process_pkt_t       process_pkt_func;
    process_pkt_drop_t  process_pkt_drop_func;

    // client functions
    process_client_t   process_client_func;
};

#endif // _SC_GLOBAL_H_