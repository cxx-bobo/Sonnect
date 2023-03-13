#ifndef _SC_PORT_H_
#define _SC_PORT_H_

#include <stdlib.h>

#include <rte_ethdev.h>
#include <rte_version.h>

/*!
 * \brief maximum number of RX/TX ring descriptors per queue
 */
#define MAX_NB_RX_DESC_PER_QUEUE 2048
#define MAX_NB_TX_DESC_PER_QUEUE 2048

/*!
 * \brief length of the rss hash key, for symmetric RSS
 */
#define RSS_HASH_KEY_LENGTH 40

extern uint8_t *used_rss_hash_key;

int init_ports(struct sc_config *sc_config);

#endif