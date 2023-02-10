#include "sc_utils.h"
#include "sc_port.h"

static void _print_port_info(uint16_t port_index);
static void _show_offloads(uint64_t offloads, const char *(show_offload)(uint64_t));

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
 * \brief   initialize all available dpdk port
 * \return  zero for successfully initialization
 */
int init_ports(){
    uint16_t port_index, nb_ports;
    
    /* check available ports */
    nb_ports = rte_eth_dev_count_avail();
    if(nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports\n");
    
    for(port_index=0; port_index<nb_ports; port_index++){
        /* skip port if it's unused */
        if (!rte_eth_dev_is_valid_port(port_index))
			continue;
        
        /* print detail info of the port */
        _print_port_info(port_index);
    }

    return SC_SUCCESS;
}

/*!
 * \brief print detail port information based on given port index
 * \param port_index    index of the specified port
 */
static void _print_port_info(uint16_t port_index){
    int k, ret;
    uint16_t queue_index, mtu;
    uint32_t used_desp;
    struct rte_eth_link link;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_rxq_info rx_queue_info;
    struct rte_eth_txq_info tx_queue_info;
    struct rte_eth_fc_conf fc_conf;
    struct rte_eth_dev_owner owner;
    struct rte_ether_addr mac;
    struct rte_eth_rss_conf rss_conf;
    char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

    /* get device info */
    ret = rte_eth_dev_info_get(port_index, &dev_info);
    if (ret != 0) {
        printf("Error during getting device info: %s\n", 
            strerror(-ret));
        return;
    }

    /* print dev_info */
    printf("\t  -- driver %s device %s socket %d\n",
            dev_info.driver_name, dev_info.device->name,
            rte_eth_dev_socket_id(port_index));

    /* print dev_owner */
    ret = rte_eth_dev_owner_get(port_index, &owner);
    if (ret == 0 && owner.id != RTE_ETH_DEV_NO_OWNER)
        printf("\t --  owner %#"PRIx64":%s\n",
                owner.id, owner.name);

    /* print link */
    ret = rte_eth_link_get(port_index, &link);
    if (ret < 0) {
        printf("Link get failed (port %u): %s\n",
                port_index, rte_strerror(-ret));
    } else {
        rte_eth_link_to_str(link_status_text,
                sizeof(link_status_text),
                &link);
        printf("\t%s\n", link_status_text);
    }

    /* print flow control */
    ret = rte_eth_dev_flow_ctrl_get(port_index, &fc_conf);
    if (ret == 0 && fc_conf.mode != RTE_ETH_FC_NONE)  {
        printf("\t  -- flow control mode %s%s high %u low %u pause %u%s%s\n",
                fc_conf.mode == RTE_ETH_FC_RX_PAUSE ? "rx " :
                fc_conf.mode == RTE_ETH_FC_TX_PAUSE ? "tx " :
                fc_conf.mode == RTE_ETH_FC_FULL ? "full" : "???",
                fc_conf.autoneg ? " auto" : "",
                fc_conf.high_water,
                fc_conf.low_water,
                fc_conf.pause_time,
                fc_conf.send_xon ? " xon" : "",
                fc_conf.mac_ctrl_frame_fwd ? " mac_ctrl" : "");
    }

    /* print mac address */
    ret = rte_eth_macaddr_get(port_index, &mac);
    if (ret == 0) {
        char ebuf[RTE_ETHER_ADDR_FMT_SIZE];

        rte_ether_format_addr(ebuf, sizeof(ebuf), &mac);
        printf("\t  -- mac %s\n", ebuf);
    }

    /* print whether the port is set as promiscuous mode */
    ret = rte_eth_promiscuous_get(port_index);
    if (ret >= 0)
        printf("\t  -- promiscuous mode %s\n",
                ret > 0 ? "enabled" : "disabled");

    /* print whether the port is set as multicast mode */
    ret = rte_eth_allmulticast_get(port_index);
    if (ret >= 0)
        printf("\t  -- all multicast mode %s\n",
                ret > 0 ? "enabled" : "disabled");

    /* print mtu */
    ret = rte_eth_dev_get_mtu(port_index, &mtu);
		if (ret == 0)
			printf("\t  -- mtu (%d)\n", mtu);

    /* print rx_queue info */
    for (queue_index = 0; queue_index < dev_info.nb_rx_queues; queue_index++) {
        ret = rte_eth_rx_queue_info_get(port_index, queue_index, &rx_queue_info);
        if (ret != 0)
            break;

        if (queue_index == 0)
            printf("  - rx queue\n");

        printf("\t  -- %d descriptors ", queue_index);
        used_desp = rte_eth_rx_queue_count(port_index, queue_index);
        if (used_desp >= 0)
            printf("%d/", used_desp);
        printf("%u", rx_queue_info.nb_desc);

        if (rx_queue_info.scattered_rx)
            printf(" scattered");

        if (rx_queue_info.conf.rx_drop_en)
            printf(" drop_en");

        if (rx_queue_info.conf.rx_deferred_start)
            printf(" deferred_start");

        if (rx_queue_info.rx_buf_size != 0)
            printf(" rx buffer size %u",
                    rx_queue_info.rx_buf_size);

        printf(" mempool %s socket %d",
                rx_queue_info.mp->name,
                rx_queue_info.mp->socket_id);

        if (rx_queue_info.conf.offloads != 0)
            _show_offloads(rx_queue_info.conf.offloads, rte_eth_dev_rx_offload_name);

        printf("\n");
    }

    /* print tx_queue info */
    for (queue_index = 0; queue_index < dev_info.nb_tx_queues; queue_index++) {
        ret = rte_eth_tx_queue_info_get(port_index, queue_index, &tx_queue_info);
        if (ret != 0)
            break;
        if (queue_index == 0)
            printf("  - tx queue\n");

        printf("\t  -- %d descriptors %d",
                queue_index, tx_queue_info.nb_desc);

        printf(" thresh %u/%u",
                tx_queue_info.conf.tx_rs_thresh,
                tx_queue_info.conf.tx_free_thresh);

        if (tx_queue_info.conf.tx_deferred_start)
            printf(" deferred_start");

        if (tx_queue_info.conf.offloads != 0)
            _show_offloads(tx_queue_info.conf.offloads, rte_eth_dev_tx_offload_name);
        printf("\n");
    }

    /* print rss info */
    ret = rte_eth_dev_rss_hash_conf_get(port_index, &rss_conf);
    if (ret == 0) {
        if (rss_conf.rss_key) {
            printf("  - RSS\n");
            printf("\t  -- RSS len %u key (hex):",
                    rss_conf.rss_key_len);
            for (k = 0; k < rss_conf.rss_key_len; k++)
                printf(" %x", rss_conf.rss_key[k]);
            printf("\t  -- hf 0x%"PRIx64"\n",
                    rss_conf.rss_hf);
        }
    }
}

/*!
 * \brief parse and print all offloading flags
 * \param offloads      offloading flag
 * \param show_offload  callback function for printing the meaning of offloading flags
 */
static void _show_offloads(uint64_t offloads, const char *(show_offload)(uint64_t)){
	printf(" offloads :");
	while (offloads != 0) {
		uint64_t offload_flag = 1ULL << __builtin_ctzll(offloads);
		printf(" %s", show_offload(offload_flag));
		offloads &= ~offload_flag;
	}
}