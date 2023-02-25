#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"
#include "sc_doca_utils.h"

#if defined(SC_HAS_DOCA)

/* ==================== DOCA device operation ==================== */

/*!
 * \brief   parse given pci address into doca pci bus-device-function tuple
 * \param   pci_addr    the given pci address
 * \param   out_bdf     the parsed bus-device-function tuple
 * \return  zero for successfully initialization
 */
int sc_doca_util_parse_pci_addr(char const *pci_addr, struct doca_pci_bdf *out_bdf){
    /* 11111111_11111111_11111111_00000000 */
    unsigned int bus_bitmask = 0xFFFFFF00;
    /* 11111111_11111111_11111111_11100000 */
	unsigned int dev_bitmask = 0xFFFFFFE0;
    /* 11111111_11111111_11111111_11111000 */
	unsigned int func_bitmask = 0xFFFFFFF8;
    uint32_t tmpu;
	char tmps[4];

    if (pci_addr == NULL || strlen(pci_addr) != 7 || pci_addr[2] != ':' || pci_addr[5] != '.'){
        SC_ERROR_DETAILS("failed to parse pci address string, please check the given format");
        return SC_ERROR_INVALID_VALUE;
    }
    tmps[0] = pci_addr[0];
	tmps[1] = pci_addr[1];
	tmps[2] = '\0';
	tmpu = strtoul(tmps, NULL, 16);
	if ((tmpu & bus_bitmask) != 0){
        SC_ERROR_DETAILS("failed to parse bus info of the given pci address");
        return SC_ERROR_INVALID_VALUE;
    }
	out_bdf->bus = tmpu;

	tmps[0] = pci_addr[3];
	tmps[1] = pci_addr[4];
	tmps[2] = '\0';
	tmpu = strtoul(tmps, NULL, 16);
	if ((tmpu & dev_bitmask) != 0){
        SC_ERROR_DETAILS("failed to parse device info of the given pci address");
        return SC_ERROR_INVALID_VALUE;
    }
	out_bdf->device = tmpu;

	tmps[0] = pci_addr[6];
	tmps[1] = '\0';
	tmpu = strtoul(tmps, NULL, 16);
	if ((tmpu & func_bitmask) != 0){
        SC_ERROR_DETAILS("failed to parse function info of the given pci address");
        return SC_ERROR_INVALID_VALUE;
    }
	out_bdf->function = tmpu;

	return SC_SUCCESS;
}

/*!
 * \brief   open doca device based on given pci address
 * \param   value  	the given pci address
 * \param   func	function to check if a given device is capable of executing some job
 * \param	retval	actual return value
 * \return  zero for successfully openning
 */
int sc_doca_util_open_doca_device_with_pci(
		const struct doca_pci_bdf *value, jobs_check func, struct doca_dev **retval){
	struct doca_devinfo **dev_list;
	uint32_t nb_devs;
	struct doca_pci_bdf buf = {};
	int doca_result, result = SC_SUCCESS;
	size_t i;

	*retval = NULL;

	doca_result = doca_devinfo_list_create(&dev_list, &nb_devs);
	if (doca_result != DOCA_SUCCESS) {
		SC_ERROR_DETAILS("failed to load doca devices list: %s",
		 	doca_get_error_string(doca_result));
		result = SC_ERROR_INTERNAL;
		goto open_doca_device_exit;
	}

	for (i = 0; i < nb_devs; i++) {
		doca_result = doca_devinfo_get_pci_addr(dev_list[i], &buf);
		if (doca_result == DOCA_SUCCESS && buf.raw == value->raw) {
			/* execute job check function if necessary */
			if (func != NULL){
				if(SC_SUCCESS != func(dev_list[i])){
					SC_WARNING("failed to execute jobs check function for %u:%u.%u",
						buf.bus, buf.device, buf.function);
					continue;
				}
			}

			/* open doca device */
			doca_result = doca_dev_open(dev_list[i], retval);
			if(doca_result == DOCA_SUCCESS){
				doca_devinfo_list_destroy(dev_list);
				goto open_doca_device_exit;
			}
		}
	}

	SC_ERROR_DETAILS("matching pci device %u:%u.%u not found",
		value->bus, value->device, value->function);
	result = SC_ERROR_NOT_EXIST;
	doca_devinfo_list_destroy(dev_list);

open_doca_device_exit:
	return result;
}

/* =============================================================== */

#endif // SC_HAS_DOCA