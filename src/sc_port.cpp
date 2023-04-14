#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_port.hpp"
#include "sc_mbuf.hpp"

int _init_single_port(uint16_t port_index, uint16_t port_logical_index, struct sc_config *sc_config);
static bool _is_port_choosed(uint16_t port_index, struct sc_config *sc_config);
static void _print_port_info(uint16_t port_index);
static void _show_offloads(uint64_t offloads, const char *(show_offload)(uint64_t));

/*!
 * \brief selected rss key
 */
uint8_t *used_rss_hash_key;

/*!
 * \brief hash key for symmetric RSS
 */
static uint8_t _symmetric_rss_hash_key[RSS_HASH_KEY_LENGTH] = {
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
};

/*!
 * \brief hash key for asymmetric RSS
 */
static uint8_t _asymmetric_rss_hash_key[RSS_HASH_KEY_LENGTH] = { 
    0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2, 
    0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
    0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
    0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
    0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa
}; 

/*!
 * \brief dpdk ethernet port configuration
 */
static struct rte_eth_conf port_conf_default = {
    #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
        // .link_speeds = RTE_ETH_LINK_SPEED_AUTONEG,
        // .rxmode = {
        //     .mq_mode = RTE_ETH_MQ_RX_NONE,  /* init later */
        //     .offloads = 0,                  /* init later */
        // },
        // .txmode = {
        //     .mq_mode = RTE_ETH_MQ_TX_NONE,
        //     .offloads = 0,                  /* init later */
        // },
        // .lpbk_mode = 0,
        // .rx_adv_conf = {
		// 	.rss_conf = {
		// 		.rss_key = NULL,    /* init later */
        //         .rss_key_len = 0,   /* init later */
		// 		.rss_hf = 0         /* init later */
		// 	},
		// },
    #else
        // .link_speeds = ETH_LINK_SPEED_AUTONEG,
        // .rxmode = {
        //     .mq_mode = ETH_MQ_RX_NONE,      /* init later */
        //     .offloads = 0,                  /* init later */
        // },
        // .txmode = {
        //     .mq_mode = ETH_MQ_TX_NONE,
        //     .offloads = 0,                  /* init later */
        // },
        // .lpbk_mode = 0,
        // .rx_adv_conf = {
        //     .rss_conf = {
        //         .rss_key = NULL,    /* init later */
        //         .rss_key_len = 0,   /* init later */
        //         .rss_hf = 0,        /* init later */
        //     }
        // },
    #endif
};

/*!
 * \brief: RX queue configuration
 */
static struct rte_eth_rxconf rx_queue_conf = {
    .rx_thresh = {
        .pthresh = 8,
        .hthresh = 8,
        .wthresh = 4,
    },
    .rx_free_thresh = 32,
};

/*!
 * \brief: TX queue configuration
 */
static struct rte_eth_txconf tx_queue_conf = {
    .tx_thresh = {
        .pthresh = 36,
        .hthresh = 0,
        .wthresh = 0,
    },
    // .tx_rs_thresh = 0,
    // .tx_free_thresh = 0,
};

/*!
 * \brief   obtain the number of used ports
 * \param   sc_config       the global configuration
 * \param   nb_used_ports   the obtain number of used port result
 * \param   port_indices    pointer to an array to store actual index of used ports
 * \return  zero for successfully initialization
 */
int get_used_ports_id(struct sc_config *sc_config, uint16_t *nb_used_ports, uint16_t *port_indices){
    uint16_t i, port_index, nb_ports;

    /* check available ports */
    nb_ports = rte_eth_dev_count_avail();
    if(nb_ports == 0){
        SC_ERROR_DETAILS("no ethernet port available");
        return SC_ERROR_INTERNAL;
    }

    for(i=0, port_index=0; port_index<nb_ports; port_index++){
        /* skip the port if it's unused */
        if (!rte_eth_dev_is_valid_port(port_index)){
            SC_WARNING_DETAILS("port %d is not valid\n", port_index);
            continue;
        }

        /* skip the port if not specified in the configuratio file */
        if(!_is_port_choosed(port_index, sc_config))
            continue;
        
        if(i > SC_MAX_USED_PORTS){
            SC_ERROR_DETAILS(
                "too many used port (%d), try modify macro SC_MAX_USED_PORTS (%d)", i, SC_MAX_USED_PORTS)
        }
 
        port_indices[i] = port_index;
        
        i++;
    }

    *nb_used_ports = i;

    return SC_SUCCESS;
}

/*!
 * \brief   initialize all available dpdk port
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_ports(struct sc_config *sc_config){
    uint16_t i, port_index, nb_ports;
    
    /* check available ports */
    nb_ports = rte_eth_dev_count_avail();
    if(nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports\n");
    
    for(i=0, port_index=0; port_index<nb_ports; port_index++){
        /* skip the port if it's unused */
        if (!rte_eth_dev_is_valid_port(port_index)){
            printf("port %d is not valid\n", port_index);
            continue;
        }

        /* skip the port if not specified in the configuratio file */
        if(!_is_port_choosed(port_index, sc_config))
            continue;

        /* initialize the current port */
        if(_init_single_port(port_index, i, sc_config) != SC_SUCCESS){
            printf("failed to initailize port %d\n", port_index);
            return SC_ERROR_INTERNAL;
        }

        /* print detail info of the port */
        _print_port_info(port_index);

        sc_config->port_ids[i] = port_index; 

        sc_config->sc_port[i].logical_port_id = i;
        sc_config->sc_port[i].port_id = port_index;
        if(SC_SUCCESS != sc_util_get_mac_by_port_id(sc_config, port_index, sc_config->sc_port[i].port_mac)){
            SC_ERROR_DETAILS("failed to obtain port MAC address, something's wrong");
            return SC_ERROR_INTERNAL;
        }

        i++;
    }

    sc_config->nb_used_ports = i;
    return SC_SUCCESS;
}

/*!
 * \brief   initialize a specified port
 * \param   port_index          the actual index of the init port
 * \param   port_logical_index  the logical index of the init port
 * \param   sc_config           the global configuration
 * \return  zero for successfully initialization
 */
int _init_single_port(uint16_t port_index, uint16_t port_logical_index, struct sc_config *sc_config){
    int ret;
    uint16_t i;
    struct rte_eth_conf port_conf = port_conf_default;
	struct rte_ether_addr eth_addr;
    struct rte_eth_dev_info dev_info;

    /* get device info */
    ret = rte_eth_dev_info_get(port_index, &dev_info);
    if(ret != 0){
        printf("failed to obtain device info of port %d: %s\n", 
            port_index, rte_strerror(-ret));
        return SC_ERROR_INTERNAL;
    }

    if(sc_config->enable_offload){
        /* RX offload capacity check and config */
        if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_CHECKSUM){
            port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_CHECKSUM;
        }

        /* TX offload capacity check and config */
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE){
            port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
        }
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MT_LOCKFREE){
            port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MT_LOCKFREE;
        }
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM){
            port_conf.txmode.offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
        }
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_UDP_CKSUM){
            port_conf.txmode.offloads |= DEV_TX_OFFLOAD_UDP_CKSUM;
        }
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM){
            port_conf.txmode.offloads |= DEV_TX_OFFLOAD_TCP_CKSUM;
        }
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_SCTP_CKSUM){
            port_conf.txmode.offloads |= DEV_TX_OFFLOAD_SCTP_CKSUM;
        }
    }

    /* configure rss */
    if(sc_config->enable_rss){
        /* specify using rss */
        #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
            port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
        #else
            port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
        #endif

        /* specify used rss key type */
        if(sc_config->rss_symmetric_mode){
            port_conf.rx_adv_conf.rss_conf.rss_key = _symmetric_rss_hash_key;
            port_conf.rx_adv_conf.rss_conf.rss_key_len = RSS_HASH_KEY_LENGTH;
            used_rss_hash_key = _symmetric_rss_hash_key;
        } else {
            port_conf.rx_adv_conf.rss_conf.rss_key = _asymmetric_rss_hash_key;
            port_conf.rx_adv_conf.rss_conf.rss_key_len = RSS_HASH_KEY_LENGTH;
            used_rss_hash_key = _asymmetric_rss_hash_key;
        }
        
        /* specify rss hash fields */
        port_conf.rx_adv_conf.rss_conf.rss_hf = sc_config->rss_hash_field;
    }

    /* obtain mac address of the port */
    ret = rte_eth_macaddr_get(port_index, &eth_addr);
    if (ret < 0) {
        SC_ERROR_DETAILS("failed to obtain mac address of port %d: %s\n", 
            port_index, rte_strerror(-ret));
        return SC_ERROR_INTERNAL;
    }

    /* configure the port */
    ret = rte_eth_dev_configure(
        port_index, sc_config->nb_rx_rings_per_port, 
        sc_config->nb_tx_rings_per_port, &port_conf);
	if (ret != 0) {
        SC_ERROR_DETAILS("failed to configure port %d: %s\n",
            port_index, rte_strerror(-ret));
        return SC_ERROR_INTERNAL;
    }

    /* allocate rx_rings */
    for (i = 0; i < sc_config->nb_rx_rings_per_port; i++) {
        ret = rte_eth_rx_queue_setup(
            /* port_id */ port_index,
            /* rx_queue_id */ i,
            /* nb_rx_desc */ sc_config->rx_queue_len,
            /* socket_id */ rte_eth_dev_socket_id(port_index),
            /* rx_conf */ &rx_queue_conf, 
            /* mb_pool */ sc_config->rx_pktmbuf_pool[RX_QUEUE_MEMORY_POOL_ID(sc_config, port_logical_index, i)]
        );
		if (ret < 0) {
            SC_ERROR_DETAILS("failed to setup rx queue %d for port %d: %s\n", 
                i, port_index, rte_strerror(-ret));
            return SC_ERROR_INTERNAL;
        }
    }

    /* allocate tx_rings */
    for (i = 0; i < sc_config->nb_tx_rings_per_port; i++) {
        ret = rte_eth_tx_queue_setup(
            /* port_id */ port_index, 
            /* tx_queue_id */ i, 
            /* nb_tx_desc */ sc_config->tx_queue_len,
			/* socket_id */ rte_eth_dev_socket_id(port_index), 
            /* tx_conf */ &tx_queue_conf
        );
		if (ret < 0) {
            SC_ERROR_DETAILS("failed to setup tx_ring %d for port %d: %s\n", 
                i, port_index, rte_strerror(-ret));
            return SC_ERROR_INTERNAL;
        }
    }

    /* start the port */
    ret = rte_eth_dev_start(port_index);
	if (ret < 0) {
        SC_ERROR_DETAILS("failed to start port %d: %s\n", 
            port_index, rte_strerror(-ret));
        return SC_ERROR_INTERNAL;
    }

    /* set as promiscuous mode (if enabled) */
    if(sc_config->enable_promiscuous){
		ret = rte_eth_promiscuous_enable(port_index);
		if (ret != 0) {
            SC_ERROR_DETAILS("failed to set port %d as promiscuous mode: %s\n", 
                port_index, rte_strerror(-ret));
            return SC_ERROR_INTERNAL;
        }
	}

    /* print finish message */
    #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
        SC_LOG("finish init port %u, MAC address: " RTE_ETHER_ADDR_PRT_FMT "\n\n",
            port_index,
            RTE_ETHER_ADDR_BYTES(&eth_addr));
    #else
        SC_LOG("finish init port %u\n\n", port_index);
    #endif // RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
        
    return SC_SUCCESS;
}

/*!
 * \brief   check whether the port is going to be used 
 *          (defined in the configuration file)
 * \param   port_index  the index of the checked port
 * \param   sc_config   the global configuration
 * \return  whether the port is used
 */
static bool _is_port_choosed(uint16_t port_index, struct sc_config *sc_config){
    int i, ret;
    struct rte_ether_addr mac;
    char ebuf[RTE_ETHER_ADDR_FMT_SIZE];

    ret = rte_eth_macaddr_get(port_index, &mac);
    if (ret == 0)
        rte_ether_format_addr(ebuf, sizeof(ebuf), &mac);
    else
        return false;

    for(i=0; i<sc_config->nb_conf_ports; i++){
        if(sc_config->port_mac[i] == NULL) continue;
        if(!strcmp(ebuf, sc_config->port_mac[i])) return true;
    }

    return false;
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
    struct rte_eth_dev_owner owner;
    struct rte_ether_addr mac;
    struct rte_eth_rss_conf rss_conf;
    char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

    #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
        struct rte_eth_fc_conf fc_conf;
    #endif // RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)

    printf("\nUSED PORTs:\n");
    
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
    #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
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
    #endif // RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)

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
            printf("\t  -- RSS len %u key (hex):",
                    rss_conf.rss_key_len);
            for (k = 0; k < rss_conf.rss_key_len; k++)
                printf(" %x", rss_conf.rss_key[k]);
            printf("\t  -- hf 0x%"PRIx64"\n",
                    rss_conf.rss_hf);
        }
    }

    printf("\n");
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