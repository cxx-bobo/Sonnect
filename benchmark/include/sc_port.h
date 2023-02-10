#ifndef _PORT_H_
#define _PORT_H_

#include <stdlib.h>
#include <rte_ethdev.h>

/*!
 * \brief dpdk ethernet port configuration
 */
struct rte_eth_conf port_conf_default = {
    .rxmode = {
        .mq_mode = RTE_ETH_MQ_RX_RSS,
    },
    .txmode = {
        .mq_mode = RTE_ETH_MQ_TX_NONE,
    },
};

/*!
 * \brief configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024


#endif