#ifndef _SC_DOCA_UTILS_H_
#define _SC_DOCA_UTILS_H_

#if defined(SC_HAS_DOCA)

#include <doca_dev.h>
#include <doca_error.h>

/* function to check if a given device is capable of executing some job */
typedef int (*jobs_check)(struct doca_devinfo *);

/* ==================== DOCA device operation ==================== */
int sc_doca_util_parse_pci_addr(char const *pci_addr, struct doca_pci_bdf *out_bdf);
int sc_doca_util_open_doca_device_with_pci(
    const struct doca_pci_bdf *value, jobs_check func, struct doca_dev **retval);
/* =============================================================== */

#endif // SC_HAS_DOCA

#endif // _SC_DOCA_UTILS_H_