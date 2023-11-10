#ifndef _SC_PORT_H_
#define _SC_PORT_H_

#include <stdlib.h>

#include <rte_ethdev.h>
#include <rte_version.h>
#include <rte_dev.h>

/*!
 * \brief maximum number of RX/TX ring descriptors per queue
 */
#define MAX_NB_RX_DESC_PER_QUEUE 8192
#define MAX_NB_TX_DESC_PER_QUEUE 8192

/*!
 * \brief length of the rss hash key, for symmetric RSS
 */
#define RSS_HASH_KEY_LENGTH 40

/*!
 * \brief max number of used ports
 */
#define SC_MAX_USED_PORTS 2

extern uint8_t *used_rss_hash_key;

int init_ports(struct sc_config *sc_config);
int get_used_ports_id(struct sc_config *sc_config, uint16_t *nb_used_ports, uint16_t *port_indices);

#endif