#ifndef _SC_DOCA_H_
#define _SC_DOCA_H_

#include <rte_malloc.h>
#include <rte_spinlock.h>

#include "sc_global.hpp"
#include "sc_doca_utils/doca_utils.hpp"

#if defined(SC_HAS_DOCA)

/* macro to compute a version number usable for comparisons */
#define SC_DOCA_VERSION_NUM(a,b) ((a) << 8 | (b))
#define SC_DOCA_VERSION \
    SC_DOCA_VERSION_NUM(SC_DOCA_MAIN_VERSION, SC_DOCA_SUB_VERSION)

/* maximum number of doca flow pipes */
#define SC_DOCA_FLOW_MAX_NB_PIPES 8

/* maximum length of name (index) of doca port */
#define SC_DOCA_FLOW_MAX_PORT_ID_STRLEN 128

#include <doca_argp.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_flow.h>
#include <doca_sha.h>
#include <doca_log.h>
#include <doca_buf.h>
#include <doca_mmap.h>

#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"

int init_doca(struct sc_config *sc_config, const char *doca_conf_path);
struct per_core_doca_meta;

/* 
 * doca specific configuration 
 * NOTE: we seperete doca_config apart from sc_config to make
 *       the integration of future version of DOCA easier
 */
struct doca_config {
    /* scalable functions */
    bool enable_scalable_functions;
    char* scalable_functions[SC_MAX_NB_PORTS];
    uint16_t nb_used_sfs;

    /* pci_devices */
    char* pci_devices[SC_MAX_NB_PORTS];
    uint16_t nb_used_pci_devices;

    /* doca flow configurations */
    struct doca_flow_port **doca_flow_ports;
    struct doca_flow_cfg doca_flow_cfg;
    struct doca_flow_pipe *doca_flow_pipes[SC_DOCA_FLOW_MAX_NB_PIPES];

    /* sha configurations */
    rte_spinlock_t sha_lock;                /* spinlock use to access SHA engine under multi-thread */
    struct doca_pci_bdf sha_pci_bdf;        /* pci bus-device-function index of sha engine */
    struct doca_dev *sha_dev;		        /* doca device of sha engine */
    struct doca_ctx *sha_ctx;			    /* doca context for sha engine */

    /* doca per-core meta */
    struct per_core_doca_meta *per_core_doca_meta;
};

extern __thread uint32_t perthread_lcore_logical_id;

#define DOCA_CONF(scc) ((struct doca_config*)scc->doca_config)
#define PER_CORE_DOCA_META(scc) \
    ((struct per_core_doca_meta*)((struct doca_config*)scc->doca_config)->per_core_doca_meta)[perthread_lcore_logical_id]
#define PER_CORE_DOCA_META_BY_CORE_ID(scc, id) \
    ((struct per_core_doca_meta*)((struct doca_config*)scc->doca_config)->per_core_doca_meta)[id]

/* per-core doca metadata */
struct per_core_doca_meta {
    uint8_t something;

    /* SHA engine per-core meta */
    struct doca_mmap *sha_mmap;             /* memory map for sha engine */
    struct doca_buf_inventory *sha_buf_inv; /* buffer inventory for sha engine */
    struct doca_workq *sha_workq;           /* work queue for sha engine */
    struct doca_buf *sha_src_doca_buf;      /* pointer to the doca_buf of source data */
    struct doca_buf *sha_dst_doca_buf;      /* pointer to the doca_buf of destination data */
    char *sha_src_buffer;                   /* pointer to the source data buffer */
    char *sha_dst_buffer;                   /* pointer to the SHA result buffer */
    struct doca_sha_job *sha_job;           /* pointer to the SHA job */
};




#endif // SC_HAS_DOCA


#endif