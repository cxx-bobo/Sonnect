#ifndef _SC_DOCA_H_
#define _SC_DOCA_H_

#include "sc_global.h"
#include "sc_doca_utils.h"

#if defined(SC_HAS_DOCA)

/* macro to compute a version number usable for comparisons */
#define SC_DOCA_VERSION_NUM(a,b) ((a) << 8 | (b))
#define SC_DOCA_VERSION \
    SC_DOCA_VERSION_NUM(SC_DOCA_MAIN_VERSION, SC_DOCA_SUB_VERSION)

#include <doca_argp.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_sha.h>
#include <doca_log.h>
#include <doca_buf.h>

#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"

/* 
 * doca specific configuration 
 * NOTE: we seperete doca_config apart from sc_config to make
 *       the integration of future version of DOCA easier
 */
struct doca_config {
    /* doca core objects */
    struct doca_dev *doca_dev;			        /* doca device */
    struct doca_mmap *doca_mmap;			    /* doca mmap */
    struct doca_buf_inventory *doca_buf_inv;	/* doca buffer inventory */
    struct doca_ctx *doca_ctx;			        /* doca context */
    struct doca_workq *doca_workq;		        /* doca work queue */

    /* scalable functions */
    char* scalable_functions[SC_MAX_NB_PORTS];
    uint16_t nb_used_sfs;

    /* sha configurations */
    #if defined(SC_NEED_DOCA_SHA)
        struct doca_pci_bdf sha_pci_bdf;    /* pci bus-device-function index*/
        struct doca_dev *sha_doca_dev;		/* doca device */
    #endif // SC_HAS_DOCA && SC_NEED_DOCA_SHA
};
#define DOCA_CONF(scc) ((struct doca_config*)scc->doca_config)

int init_doca(struct sc_config *sc_config, const char *doca_conf_path);

#endif // SC_HAS_DOCA


#endif