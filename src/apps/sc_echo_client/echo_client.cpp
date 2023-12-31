#include <stdio.h>
#include "sc_global.hpp"
#include "sc_worker.hpp"
#include "sc_app.hpp"
#include "sc_echo_client/echo_client.hpp"
#include "sc_utils/distribution_gen.hpp"
#include "sc_utils/timestamp.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/tail_latency.hpp"
#include "sc_utils.hpp"
#include "sc_control_plane.hpp"

/*!
 * \brief   parse application-specific key-value configuration pair
 * \param   key         the key of the config pair
 * \param   value       the value of the config pair
 * \param   sc_config   the global configuration
 * \return  zero for successfully parsing
 */
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config){
    int result = SC_SUCCESS;
    
    /* used send port */
    if(!strcmp(key, "send_port_mac")){
        int nb_send_ports = 0, i;
        uint32_t port_id, logical_port_id;
        char *delim = ",";
        char *p, *port_mac;

        for(;;){
            if(sc_force_quit){ break; }

            if(nb_send_ports == 0)
                p = strtok(value, delim);
            else
                p = strtok(NULL, delim);
            
            if (!p) break;

            p = sc_util_del_both_trim(p);
            sc_util_del_change_line(p);

            port_mac = (char*)malloc(strlen(p)+1);
            if(unlikely(!port_mac)){
                SC_ERROR_DETAILS("Failed to allocate memory for port_mac");
                result = SC_ERROR_MEMORY;
                goto free_send_port_mac;
            }
            memset(port_mac, 0, strlen(p)+1);

            /* obtain port id by given mac address */
            strcpy(port_mac, p);
            if(SC_SUCCESS != sc_util_get_port_id_by_mac(sc_config, port_mac, &port_id)){
                SC_ERROR_DETAILS("failed to get port id by mac %s", port_mac);
                result = SC_ERROR_INVALID_VALUE;
                goto free_send_port_mac;
            }

            /* obtain logical port id by port id */
            if(SC_SUCCESS != sc_util_get_logical_port_id_by_port_id(sc_config, port_id, &logical_port_id)){
                SC_ERROR_DETAILS("failed to get logical port id by port id %u", port_id);
                result = SC_ERROR_INVALID_VALUE;
                goto free_send_port_mac;
            }

            /* we record all id info in previous for fatser indexing while sending packets */
            INTERNAL_CONF(sc_config)->send_port_idx[nb_send_ports] = port_id;
            INTERNAL_CONF(sc_config)->send_port_logical_idx[nb_send_ports] = logical_port_id;
            INTERNAL_CONF(sc_config)->send_port_mac_address[nb_send_ports] = port_mac;
            
            nb_send_ports += 1;
        }

        INTERNAL_CONF(sc_config)->nb_send_ports = nb_send_ports;

        goto _parse_app_kv_pair_exit;

free_send_port_mac:
        for(i=0; i<nb_send_ports; i++) free(INTERNAL_CONF(sc_config)->send_port_mac_address[i]);

invalid_send_port_mac:
        SC_ERROR_DETAILS("invalid send port mac address\n");
    }

    /* used recv port */
    if(!strcmp(key, "recv_port_mac")){
        int nb_recv_ports = 0, i;
        uint32_t port_id, logical_port_id;
        char *delim = ",";
        char *p, *port_mac;

        for(;;){
            if(sc_force_quit){ break; }

            if(nb_recv_ports == 0)
                p = strtok(value, delim);
            else
                p = strtok(NULL, delim);
            
            if (!p) break;

            p = sc_util_del_both_trim(p);
            sc_util_del_change_line(p);

            port_mac = (char*)malloc(strlen(p)+1);
            if(unlikely(!port_mac)){
                SC_ERROR_DETAILS("Failed to allocate memory for port_mac");
                result = SC_ERROR_MEMORY;
                goto free_recv_port_mac;
            }
            memset(port_mac, 0, strlen(p)+1);

            /* obtain port id by given mac address */
            strcpy(port_mac, p);
            if(SC_SUCCESS != sc_util_get_port_id_by_mac(sc_config, port_mac, &port_id)){
                SC_ERROR_DETAILS("failed to get port id by mac %s", port_mac);
                result = SC_ERROR_INVALID_VALUE;
                goto free_recv_port_mac;
            }

            /* obtain logical port id by port id */
            if(SC_SUCCESS != sc_util_get_logical_port_id_by_port_id(sc_config, port_id, &logical_port_id)){
                SC_ERROR_DETAILS("failed to get logical port id by port id %u", port_id);
                result = SC_ERROR_INVALID_VALUE;
                goto free_recv_port_mac;
            }

            /* we record all id info in previous for fatser indexing while sending packets */
            INTERNAL_CONF(sc_config)->recv_port_idx[nb_recv_ports] = port_id;
            INTERNAL_CONF(sc_config)->recv_port_logical_idx[nb_recv_ports] = logical_port_id;
            INTERNAL_CONF(sc_config)->recv_port_mac_address[nb_recv_ports] = port_mac;

            nb_recv_ports += 1;
        }

        INTERNAL_CONF(sc_config)->nb_recv_ports = nb_recv_ports;

        goto _parse_app_kv_pair_exit;

free_recv_port_mac:
        for(i=0; i<nb_recv_ports; i++) free(INTERNAL_CONF(sc_config)->recv_port_mac_address[i]);

invalid_recv_port_mac:
        SC_ERROR_DETAILS("invalid recv port mac address\n");
    }

    /* packet length value */
    if(!strcmp(key, "pkt_len")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        uint32_t pkt_len;
        if(sc_util_atoui_32(value, &pkt_len) != SC_SUCCESS) {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_pkt_len;
        }
        INTERNAL_CONF(sc_config)->pkt_len = pkt_len;
        goto _parse_app_kv_pair_exit;

invalid_pkt_len:
        SC_ERROR_DETAILS("invalid configuration pkt_len\n");
    }

    /* send bit rate */
    if(!strcmp(key, "bit_rate")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        double bit_rate;
        if(sc_util_atolf(value, &bit_rate) != SC_SUCCESS) {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_bit_rate;
        }
        INTERNAL_CONF(sc_config)->bit_rate = bit_rate;
        goto _parse_app_kv_pair_exit;

invalid_bit_rate:
        SC_ERROR_DETAILS("invalid configuration bit_rate\n");
    }

    /* send pkt rate */
    if(!strcmp(key, "pkt_rate")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        double pkt_rate;
        if(sc_util_atolf(value, &pkt_rate) != SC_SUCCESS) {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_pkt_rate;
        }
        INTERNAL_CONF(sc_config)->pkt_rate = pkt_rate;
        goto _parse_app_kv_pair_exit;

invalid_pkt_rate:
        SC_ERROR_DETAILS("invalid configuration pkt_rate\n");
    }

    /* number of packet per burst */
    if(!strcmp(key, "nb_pkt_per_burst")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        uint32_t nb_pkt_per_burst;
        if(sc_util_atoui_32(value, &nb_pkt_per_burst) != SC_SUCCESS) {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_nb_pkt_per_burst;
        }
        INTERNAL_CONF(sc_config)->nb_pkt_per_burst = nb_pkt_per_burst;
        goto _parse_app_kv_pair_exit;

invalid_nb_pkt_per_burst:
        SC_ERROR_DETAILS("invalid configuration nb_pkt_per_burst\n");
    }
    
    /* number of flow per core */
    if(!strcmp(key, "nb_flow_per_core")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        uint64_t nb_flow_per_core;
        if(sc_util_atoui_64(value, &nb_flow_per_core) != SC_SUCCESS) {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_nb_flow_per_core;
        }
        if(nb_flow_per_core == 0){
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_nb_flow_per_core;
        }

        INTERNAL_CONF(sc_config)->nb_flow_per_core = nb_flow_per_core;
        goto _parse_app_kv_pair_exit;

invalid_nb_flow_per_core:
        SC_ERROR_DETAILS("invalid configuration nb_flow_per_core\n");
    }

_parse_app_kv_pair_exit:
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter_sender(struct sc_config *sc_config){
    int i, result = SC_SUCCESS;
    uint16_t queue_id = 0;
    
    PER_CORE_APP_META(sc_config).nb_send_pkt = 0;
    PER_CORE_APP_META(sc_config).nb_confirmed_pkt = 0;

    /* initialize interval generator */
    double per_core_pkt_rate;
    if(INTERNAL_CONF(sc_config)->pkt_rate != 0) {
        per_core_pkt_rate = (double)INTERNAL_CONF(sc_config)->pkt_rate 
                            / (double)1000 / (double)(sc_config->nb_used_cores/2);      /* Gpps */
    } else {
        per_core_pkt_rate = (double)INTERNAL_CONF(sc_config)->bit_rate 
                            / (double)(sc_config->nb_used_cores/2)
                            / (double) 8.0 / (double)INTERNAL_CONF(sc_config)->pkt_len; /* Gpps */
    }

    double per_core_brust_rate = (double) per_core_pkt_rate / (double)INTERNAL_CONF(sc_config)->nb_pkt_per_burst;
    double per_core_brust_interval = (double) 1.0 / (double) per_core_brust_rate; /* ns */
    PER_CORE_APP_META(sc_config).interval_generator 
        = new sc_util_exponential_uint64_generator((int)per_core_brust_interval);
    PER_CORE_APP_META(sc_config).interval = PER_CORE_APP_META(sc_config).interval_generator->next();
    PER_CORE_APP_META(sc_config).last_send_timestamp = sc_util_timestamp_ns();
    
    // SC_THREAD_LOG("per core pkt rate: %lf G packet/second", per_core_pkt_rate);    
    // SC_THREAD_LOG("per core brust rate: %lf G brust/second", per_core_brust_rate);
    // SC_THREAD_LOG("per core brust interval: %lf ns, (int)%d ns",
    //    per_core_brust_interval, (int)per_core_brust_interval);
    // SC_THREAD_LOG("initialize interval: %lu ns", PER_CORE_APP_META(sc_config).interval);

    /* allocate memory for storing generated packet headers */
    struct sc_pkt_hdr *pkt_hdrs = (struct sc_pkt_hdr*)rte_malloc(
        NULL, sizeof(struct sc_pkt_hdr)*INTERNAL_CONF(sc_config)->nb_flow_per_core, 0);
    if(unlikely(!pkt_hdrs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for pkt_hdrs");
        result = SC_ERROR_MEMORY;
        goto _process_enter_exit;
    }
    memset(pkt_hdrs, 0, sizeof(struct sc_pkt_hdr)*INTERNAL_CONF(sc_config)->nb_flow_per_core);
    PER_CORE_APP_META(sc_config).test_pkts = pkt_hdrs;
    PER_CORE_APP_META(sc_config).last_used_flow = 0;

    for(i=0; i<sc_config->nb_used_cores; i++){
        if(sc_config->core_ids[i] == rte_lcore_id()){
            queue_id = i;
            /*! 
                \note:  we allow nb_rings_per_port is less than nb_used_port 
                        so we have to limit the queue_id within the valid range
             */
            if(queue_id != 0){
                queue_id = queue_id % sc_config->nb_rx_rings_per_port;
            }
            
            SC_THREAD_LOG("core %u is using queue %u", rte_lcore_id(), queue_id);
            break;
        }
        if(i == sc_config->nb_used_cores-1){
            SC_THREAD_ERROR_DETAILS("unknown queue id for worker thread on lcore %u", rte_lcore_id());
            result = SC_ERROR_INTERNAL;
            goto _process_enter_exit;
        }
    }

    /* generate random packet header for each flow */
    for(i=0; i<INTERNAL_CONF(sc_config)->nb_flow_per_core; i++){
        result = sc_util_generate_random_pkt_hdr(
            /* sc_pkt_hdr */ &PER_CORE_APP_META(sc_config).test_pkts[i],
            /* pkt_len */ INTERNAL_CONF(sc_config)->pkt_len,
            /* payload_len */ 0,
            /* nb_queues */ sc_config->nb_rx_rings_per_port,
            /* used_queue_id */ queue_id,
            /* l3_type */ RTE_ETHER_TYPE_IPV4,
            /* l4_type */ IPPROTO_UDP,
            /* rss_hash_field */ sc_config->rss_hash_field,
            /* rss_affinity */ false,
            /* min_pkt_len */ 
                sizeof(struct sc_timestamp_table) + sizeof(struct rte_ether_hdr) 
                + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr),
            /* payload */ nullptr
        );
        if(result != SC_SUCCESS){
            SC_THREAD_ERROR("failed to generate new pkt header");
            goto _process_enter_exit;
        }
    }
    // SC_THREAD_LOG(
    //     "generate %lu flow(s)' header, l3_type: %x, l4_type: %d",
    //     INTERNAL_CONF(sc_config)->nb_flow_per_core,
    //     PER_CORE_APP_META(sc_config).test_pkts[0].l3_type,
    //     PER_CORE_APP_META(sc_config).test_pkts[0].l4_type
    // )

    /* allocate array for pointers to storing send pkt_bufs */
    PER_CORE_APP_META(sc_config).send_pkt_bufs = (struct rte_mbuf **)rte_malloc(NULL, 
        sizeof(struct rte_mbuf*)*INTERNAL_CONF(sc_config)->nb_pkt_per_burst, 0);
    if(unlikely(!PER_CORE_APP_META(sc_config).send_pkt_bufs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for send_pkt_bufs");
        result = SC_ERROR_MEMORY;
        goto _process_enter_exit;
    }

    /* initialize the start_time */
    if(unlikely(-1 == gettimeofday(&PER_CORE_APP_META(sc_config).start_time, NULL))){
        SC_THREAD_ERROR_DETAILS("failed to obtain current time");
        result =  SC_ERROR_INTERNAL;
        goto _process_enter_exit;
    }

_process_enter_exit:
    return result;
}

/*!
 * \brief   callback for client logic
 * \param   sc_config       the global configuration
 * \param   queue_id        the index of the queue for current core to tx/rx packet
 * \param   ready_to_exit   indicator for exiting worker loop
 * \return  zero for successfully executing
 */
int _process_client_sender(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit){
    int i, j, nb_tx = 0, nb_send_pkt = 0, result = SC_SUCCESS, retry;
    uint64_t current_ns = 0;

    #if defined(SC_ECHO_CLIENT_GET_LATENCY)
        struct sc_timestamp_table sc_ts = {0};
    #endif // defined(SC_ECHO_CLIENT_GET_LATENCY)

    /* send packet */
    for(i=0; i<INTERNAL_CONF(sc_config)->nb_send_ports; i++){
        /* avoid endless loop */
        if(sc_force_quit){ break; }

        /* check send interval */
        /* this will cause 0.4Mpps performance loss under 66 pkt size */
        current_ns = sc_util_timestamp_ns();
        if(current_ns - PER_CORE_APP_META(sc_config).last_send_timestamp < PER_CORE_APP_META(sc_config).interval){
            continue;
        }

        /* obtain the packet header currently used, and setup payload info*/
        struct sc_pkt_hdr *current_used_pkt 
            = &(PER_CORE_APP_META(sc_config).test_pkts[PER_CORE_APP_META(sc_config).last_used_flow]);
        
        /* generate new burst of packets */
        if(SC_SUCCESS != sc_util_generate_packet_burst_mbufs_fast_v4_udp(
                /* mp */ PER_CORE_TX_MBUF_POOL(sc_config, INTERNAL_CONF(sc_config)->send_port_logical_idx[i]),
                /* hdr */ current_used_pkt,
                /* pkts_burst */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
                /* nb_pkt_per_burst */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst
        )){
            SC_THREAD_ERROR("failed to assemble final packet");
            result = SC_ERROR_INTERNAL;
            goto process_client_ready_to_exit;
        }

        #if defined(SC_ECHO_CLIENT_GET_LATENCY)
            /* set the accuracy as short */
            sc_ts.timestamp_type = SC_TIMESTAMP_FULL_TYPE;

            /* 
             * record the timestamp
             */
            sc_util_add_full_timestamp(&sc_ts, sc_util_timestamp_ns());

            /* copy timestamp to payload */
            if(SC_SUCCESS != sc_util_copy_payload_to_packet_burst(
                /* payload */ &sc_ts,
                /* payload_len */ sizeof(sc_ts),
                /* payload_offset */ &(current_used_pkt->payload_offset),
                /* pkts_burst */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
                /* nb_pkt_per_burst */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst
            )){
                SC_THREAD_ERROR("failed to copy payload to final packets");
                result = SC_ERROR_INTERNAL;
                goto process_client_ready_to_exit;
            }

            /* we record the copy latency here to fix the latency statistic */
            PER_CORE_APP_META(sc_config).payload_copy_latency +=
                (double)(sc_util_timestamp_ns() - current_ns);
            PER_CORE_APP_META(sc_config).payload_copy_latency /= (double)2.0f;
        #endif // defined(SC_ECHO_CLIENT_GET_LATENCY)

        nb_send_pkt = rte_eth_tx_burst(
            /* port_id */ INTERNAL_CONF(sc_config)->send_port_idx[i],
            /* queue_id */ queue_id,
            /* tx_pkts */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
            /* nb_pkts */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst
        );
        if(unlikely(nb_send_pkt < INTERNAL_CONF(sc_config)->nb_pkt_per_burst)){
            retry = 0;
            while (nb_send_pkt < INTERNAL_CONF(sc_config)->nb_pkt_per_burst 
                    && retry++ < SC_ECHO_CLIENT_BURST_TX_RETRIES
            ){
                nb_send_pkt += rte_eth_tx_burst(
                    /* port_id */ INTERNAL_CONF(sc_config)->send_port_idx[i],
                    /* queue_id */ queue_id, 
                    /* tx_pkts */ &PER_CORE_APP_META(sc_config).send_pkt_bufs[nb_send_pkt], 
                    /* nb_pkts */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst - nb_send_pkt
                );
            }
        }

        /* return back un-sent pkt_mbuf */
        for(j=nb_send_pkt; j<INTERNAL_CONF(sc_config)->nb_pkt_per_burst; j++) {
            rte_pktmbuf_free(PER_CORE_APP_META(sc_config).send_pkt_bufs[j]); 
        }

        nb_tx += nb_send_pkt;

        /* update sending timestamp and interval */
        PER_CORE_APP_META(sc_config).last_send_timestamp = current_ns;

        /* FIXME: use timer to control the interval (according to a parameter) */
        PER_CORE_APP_META(sc_config).interval = PER_CORE_APP_META(sc_config).interval_generator->next();
    }

    /* update metadata */
    if(nb_tx != 0){
        // record the #sended packet
        PER_CORE_APP_META(sc_config).nb_send_pkt += nb_tx;
        PER_CORE_APP_META(sc_config).nb_interval_send_pkt += nb_tx;
        PER_CORE_APP_META(sc_config).nb_interval_drop_pkt 
            += INTERNAL_CONF(sc_config)->nb_pkt_per_burst * INTERNAL_CONF(sc_config)->nb_send_ports;
        
        // switch the sended flow
        if(PER_CORE_APP_META(sc_config).last_used_flow == INTERNAL_CONF(sc_config)->nb_flow_per_core-1){
            PER_CORE_APP_META(sc_config).last_used_flow = 0;
        } else {
            PER_CORE_APP_META(sc_config).last_used_flow += 1;
        }
    }
    
    goto process_client_exit;

process_client_ready_to_exit:
    *ready_to_exit = true;

process_client_exit:
    return result;
}

/*!
 * \brief   callback while exiting application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_exit_sender(struct sc_config *sc_config){
    int result = SC_SUCCESS;

    long total_interval_sec;
    long total_interval_usec;

    /* initialize the end_time */
    if(unlikely(-1 == gettimeofday(&PER_CORE_APP_META(sc_config).end_time, NULL))){
        SC_THREAD_ERROR_DETAILS("failed to obtain current time");
        return SC_ERROR_INTERNAL;
    }

    /* calculate the total duration */
    total_interval_sec 
        = PER_CORE_APP_META(sc_config).end_time.tv_sec - PER_CORE_APP_META(sc_config).start_time.tv_sec;
    total_interval_usec 
        = PER_CORE_APP_META(sc_config).end_time.tv_usec - PER_CORE_APP_META(sc_config).start_time.tv_usec;
    SC_THREAD_LOG("[sender]: send %ld packets in total", PER_CORE_APP_META(sc_config).nb_send_pkt);

    SC_THREAD_LOG("[sender]: copy to payload latency: %lf", PER_CORE_APP_META(sc_config).payload_copy_latency);

    SC_THREAD_LOG("[sender]: send throughput: %f Gbps, %f Mpps",
        (float)(PER_CORE_APP_META(sc_config).nb_send_pkt * INTERNAL_CONF(sc_config)->pkt_len * 8) 
        / (float)(SC_UTIL_TIME_INTERVL_US(total_interval_sec, total_interval_usec) * 1000),
        (float)(PER_CORE_APP_META(sc_config).nb_send_pkt)
        / (float)(SC_UTIL_TIME_INTERVL_US(total_interval_sec, total_interval_usec))
    );

_process_exit_exit:
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter_receiver(struct sc_config *sc_config){
    int result = SC_SUCCESS;

    PER_CORE_APP_META(sc_config).nb_send_pkt = 0;
    PER_CORE_APP_META(sc_config).nb_confirmed_pkt = 0;

    /* allocate array for pointers to storing received pkt_bufs */
    PER_CORE_APP_META(sc_config).recv_pkt_bufs = (struct rte_mbuf **)rte_malloc(NULL, 
        sizeof(struct rte_mbuf*)*SC_MAX_RX_PKT_BURST*2, 0);
    if(unlikely(!PER_CORE_APP_META(sc_config).recv_pkt_bufs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for recv_pkt_bufs");
        result = SC_ERROR_MEMORY;
        goto _process_enter_receiver_exit;
    }

    /* initialize the start_time */
    if(unlikely(-1 == gettimeofday(&PER_CORE_APP_META(sc_config).start_time, NULL))){
        SC_THREAD_ERROR_DETAILS("failed to obtain current time");
        result =  SC_ERROR_INTERNAL;
        goto _process_enter_receiver_exit;
    }

_process_enter_receiver_exit:
    return result;
}

/*!
 * \brief   callback for client logic
 * \param   sc_config       the global configuration
 * \param   queue_id        the index of the queue for current core to tx/rx packet
 * \param   ready_to_exit   indicator for exiting worker loop
 * \return  zero for successfully executing
 */
int _process_client_receiver(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit){
    int i, j, k, nb_rx = 0, nb_recv_pkt = 0, result = SC_SUCCESS;
    struct sc_timestamp_table *payload_timestamp;
    uint64_t current_ns;

    for(i=0; i<INTERNAL_CONF(sc_config)->nb_recv_ports; i++){
        memset(PER_CORE_APP_META(sc_config).recv_pkt_bufs, 0, sizeof(struct rte_mbuf*)*SC_MAX_RX_PKT_BURST*2);

        nb_recv_pkt = rte_eth_rx_burst(
            /* port_id */ INTERNAL_CONF(sc_config)->recv_port_idx[i], 
            /* queue_id */ queue_id, 
            /* rx_pkts */ PER_CORE_APP_META(sc_config).recv_pkt_bufs, 
            /* nb_pkts */ SC_MAX_RX_PKT_BURST
        );

        /* record the receiving timestamp */
        current_ns = sc_util_timestamp_ns();

        if(nb_recv_pkt == 0) { continue; }
        
        PER_CORE_APP_META(sc_config).nb_confirmed_pkt += nb_recv_pkt;
        PER_CORE_APP_META(sc_config).nb_interval_recv_pkt += nb_recv_pkt;
        
        for(j=0; j<nb_recv_pkt; j++) {
            /* extract the timestamp struct */
            payload_timestamp = rte_pktmbuf_mtod_offset(
                PER_CORE_APP_META(sc_config).recv_pkt_bufs[j], struct sc_timestamp_table*, 
                sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr)
            );
            
            /* skip wrong payload packet */
            if(unlikely(payload_timestamp->timestamp_type != SC_TIMESTAMP_FULL_TYPE)){
                continue;
            }

            #if defined(SC_ECHO_CLIENT_GET_LATENCY)
                /* add receive timestamp to the timestamp table */
                sc_util_add_full_timestamp(payload_timestamp, current_ns);

                /* copy the timestamp table to local collection */                
                rte_memcpy(
                    /* dst */ PER_CORE_APP_META(sc_config).ts_tables 
                        + PER_CORE_APP_META(sc_config).ts_tables_pointer,
                    /* src */ payload_timestamp,
                    /* size */ sizeof(struct sc_timestamp_table)
                );

                PER_CORE_APP_META(sc_config).ts_tables_pointer += 1;
                if(PER_CORE_APP_META(sc_config).ts_tables_pointer == SC_ECHO_CLIENT_NB_TS_TABLE){
                    PER_CORE_APP_META(sc_config).ts_tables_pointer = 0;
                }
                if(PER_CORE_APP_META(sc_config).nb_ts_tables < SC_ECHO_CLIENT_NB_TS_TABLE){
                    PER_CORE_APP_META(sc_config).nb_ts_tables += 1;
                }
            #endif defined(SC_ECHO_CLIENT_GET_LATENCY)

free_recv_pkt_mbuf:
            /* return back recv pkt_mbuf */
            rte_pktmbuf_free(PER_CORE_APP_META(sc_config).recv_pkt_bufs[j]); 
        }
    }

    goto process_client_receiver_exit;

process_client_receiver_ready_to_exit:
    *ready_to_exit = true;

process_client_receiver_exit:
    return result;
}

/*!
 * \brief   callback while exiting application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_exit_receiver(struct sc_config *sc_config){
    /* initialize the end_time */
    if(unlikely(-1 == gettimeofday(&PER_CORE_APP_META(sc_config).end_time, NULL))){
        SC_THREAD_ERROR_DETAILS("failed to obtain current time");
        return SC_ERROR_INTERNAL;
    }

    SC_THREAD_LOG("[receiver] confirmed pkt: %lu", PER_CORE_APP_META(sc_config).nb_confirmed_pkt);

    return SC_SUCCESS;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _worker_all_exit(struct sc_config *sc_config){
    int result = SC_SUCCESS;
    uint8_t nb_timestamp = 0;
    uint32_t i, j, k;
    struct sc_timestamp_table* sc_ts;
    size_t nb_send_pkt = 0, nb_confirmed_pkt = 0;
    double per_core_avg_latency, per_core_payload_copy_latency, all_core_avg_latency;
    long worker_max_interval_sec = 0, worker_max_interval_usec = 0;
    char profiling_file_name[512] = {0};
    FILE * fp;

    /* calculate the average payload copy latency to fix the RTT statistic */
    per_core_payload_copy_latency = (double)0.0f;
    for(i=0; i<sc_config->nb_used_cores; i++){
        per_core_payload_copy_latency += (double)PER_CORE_APP_META_BY_CORE_ID(sc_config, i).payload_copy_latency;
        per_core_payload_copy_latency /= (double)2.0f;
    }

    // record latency data
    #if defined(SC_ECHO_CLIENT_GET_LATENCY)
        sprintf(profiling_file_name, "latency.txt");
        fp = fopen(profiling_file_name, "w");
        if (!fp) {
            SC_ERROR("failed to create/open log file to store latency statistics");
            result = SC_ERROR_INTERNAL;
            goto worker_all_exit_exit;
        }
        for(i=sc_config->nb_used_cores/2; i<sc_config->nb_used_cores; i++){
            for(j=0; j<PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_ts_tables; j++){
                sc_ts = &(PER_CORE_APP_META_BY_CORE_ID(sc_config, i).ts_tables[j]);
                fprintf(
                    /* fd */ fp, "%u\t%lu\t%lu\n", 
                    /* core_id */ i,
                    /* t1 */ sc_util_get_full_timestamp(sc_ts, 0),
                    /* t2 */ sc_util_get_full_timestamp(sc_ts, 1)
                );
            }
        }
    #endif // defined(SC_ECHO_CLIENT_GET_LATENCY)

    for(i=0; i<sc_config->nb_used_cores; i++){
        /* reduce number of packet */
        nb_send_pkt += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_send_pkt;
        nb_confirmed_pkt += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_confirmed_pkt;
    }
    SC_LOG("[TOTAL] number of send packet: %ld, number of confirm packet: %ld",
        nb_send_pkt, nb_confirmed_pkt
    );

    /* record maximum duration */
    for(i=0; i<sc_config->nb_used_cores; i++){
        long interval_sec, interval_usec;
        interval_sec
            = PER_CORE_APP_META_BY_CORE_ID(sc_config, i).end_time.tv_sec 
            - PER_CORE_APP_META_BY_CORE_ID(sc_config, i).start_time.tv_sec;
        interval_usec
            = PER_CORE_APP_META_BY_CORE_ID(sc_config, i).end_time.tv_usec 
            - PER_CORE_APP_META_BY_CORE_ID(sc_config, i).start_time.tv_usec;
        if(SC_UTIL_TIME_INTERVL_US(interval_sec, interval_usec) >
                SC_UTIL_TIME_INTERVL_US(worker_max_interval_sec, worker_max_interval_usec)){
            worker_max_interval_sec = interval_sec;
            worker_max_interval_usec = interval_usec;
        }
    }

    SC_THREAD_LOG("[TOTAL] duration: %lu us",
        SC_UTIL_TIME_INTERVL_US(worker_max_interval_sec, worker_max_interval_usec)
    );
    SC_THREAD_LOG("[TOTAL] packet length: %u bytes",
        INTERNAL_CONF(sc_config)->pkt_len
    );
    SC_THREAD_LOG("[TOTAL] send throughput: %f Gbps",
        (float)(nb_send_pkt * INTERNAL_CONF(sc_config)->pkt_len * 8) 
        / (float)(SC_UTIL_TIME_INTERVL_US(worker_max_interval_sec, worker_max_interval_usec) * 1000)
    );
    SC_THREAD_LOG("[TOTAL] confirm throughput: %f Gbps",
        (float)(nb_confirmed_pkt * INTERNAL_CONF(sc_config)->pkt_len * 8) 
        / (float)(SC_UTIL_TIME_INTERVL_US(worker_max_interval_sec, worker_max_interval_usec) * 1000)
    );
    SC_THREAD_LOG("[TOTAL] send throughput: %f Mpps",
        (float)(nb_send_pkt) 
        / (float)(SC_UTIL_TIME_INTERVL_US(worker_max_interval_sec, worker_max_interval_usec))
    );
    SC_THREAD_LOG("[TOTAL] confirm throughput: %f Mpps",
        (float)(nb_confirmed_pkt) 
        / (float)(SC_UTIL_TIME_INTERVL_US(worker_max_interval_sec, worker_max_interval_usec))
    );

worker_all_exit_exit:
    return SC_SUCCESS;
}

/*!
 * \brief   callback while entering control-plane thread (for sender)
 * \param   sc_config       the global configuration
 * \param   worker_core_id  the core id of the worker
 * \return  zero for successfully initialization
 */
int _control_enter_sender(struct sc_config *sc_config, uint32_t worker_core_id){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback during control-plane thread runtime (for sender)
 * \param   sc_config       the global configuration
 * \param   worker_core_id  the core id of the worker
 * \return  zero for successfully execution
 */
int _control_infly_sender(struct sc_config *sc_config, uint32_t worker_core_id){
    uint64_t i;
    uint64_t current_ns;
    uint64_t record_interval, nb_interval_send_pkt, nb_interval_drop_pkt;
    uint32_t target_core_id, target_logical_core_id;
    double send_throughput, drop_throughput;
    double overall_send_throughput=0.0f, overall_drop_throughput=0.0f, overall_theory_throughput=0.0f;

    char print_title[2048] = {0};
    char print_send_statistics[2048] = {0};
    char print_drop_statistics[2048] = {0};
    char print_theory_statistics[2048] = {0};
    
    sprintf(print_title,                "| Core Index |");
    sprintf(print_send_statistics,      "| Send Thrpt |");
    sprintf(print_drop_statistics,      "| Drop Thrpt |");
    sprintf(print_theory_statistics,    "| Theo Thrpt |");

    // print statistics by first sender core's control function
    if(worker_core_id == INTERNAL_CONF(sc_config)->send_core_idx[0]){
        for(i=0; i<INTERNAL_CONF(sc_config)->nb_send_cores; i++){
            current_ns = sc_util_timestamp_ns();

            // obtain both the logical and physical core id
            target_core_id = INTERNAL_CONF(sc_config)->send_core_idx[i];
            sc_util_get_logical_core_id_by_core_id(sc_config, target_core_id, &target_logical_core_id);

            // calculate interval
            record_interval = current_ns 
                - PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).last_send_record_timestamp;
            
            // calculate #send-pkts and #drop-pkts within the pass interval,
            // then reset metadata
            nb_interval_send_pkt 
                = PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).nb_interval_send_pkt;
            PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).nb_interval_send_pkt = 0;
            nb_interval_drop_pkt 
                = PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).nb_interval_drop_pkt;
            PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).nb_interval_drop_pkt = 0;
            PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).last_send_record_timestamp = current_ns;

            // calculate throughput
            send_throughput = ((double)nb_interval_send_pkt) / ((double)record_interval) * ((double)1000.f);
            drop_throughput = ((double)nb_interval_drop_pkt) / ((double)record_interval) * ((double)1000.f);

            overall_send_throughput += send_throughput;
            overall_drop_throughput += drop_throughput;
            overall_theory_throughput += send_throughput + drop_throughput;

            // insert log string 
            if(target_core_id < 10)
                sprintf(print_title, "%s     Core %u    |", print_title, target_core_id);
            else
                sprintf(print_title, "%s    Core %u    |", print_title, target_core_id);
            sprintf(print_send_statistics, "%s %lf Mpps |", print_send_statistics, send_throughput);
            sprintf(print_drop_statistics, "%s %lf Mpps |", print_drop_statistics, drop_throughput);
            sprintf(print_theory_statistics, "%s %lf Mpps |", print_theory_statistics, send_throughput+drop_throughput);
        }

        sprintf(print_title, "%s    Overall    |", print_title);
        sprintf(print_send_statistics, "%s %lf Mpps |", print_send_statistics, overall_send_throughput);
        sprintf(print_drop_statistics, "%s %lf Mpps |", print_drop_statistics, overall_drop_throughput);
        sprintf(print_theory_statistics, "%s %lf Mpps |", print_theory_statistics, overall_theory_throughput);

        // print log
        SC_LOG("Sender Throughput\n%s\n%s\n%s\n%s",
            print_title, print_send_statistics, print_drop_statistics, print_theory_statistics);
    }

    return SC_SUCCESS;
}

/*!
 * \brief   callback while exiting control-plane thread (for sender)
 * \param   sc_config       the global configuration
 * \param   worker_core_id  the core id of the worker
 * \return  zero for successfully execution
 */
int _control_exit_sender(struct sc_config *sc_config, uint32_t worker_core_id){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback while entering control-plane thread (for receiver)
 * \param   sc_config       the global configuration
 * \param   worker_core_id  the core id of the worker
 * \return  zero for successfully initialization
 */
int _control_enter_receiver(struct sc_config *sc_config, uint32_t worker_core_id){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback during control-plane thread runtime (for receiver)
 * \param   sc_config       the global configuration
 * \param   worker_core_id  the core id of the worker
 * \return  zero for successfully execution
 */
int _control_infly_receiver(struct sc_config *sc_config, uint32_t worker_core_id){
    uint64_t i;
    uint64_t current_ns;
    uint64_t record_interval, nb_interval_recv_pkt;
    uint32_t target_core_id, target_logical_core_id;
    double recv_throughput;

    char print_title[2048] = {0};
    char print_recv_statistics[2048] = {0};
    
    sprintf(print_title,                "| Core Index |");
    sprintf(print_recv_statistics,      "| Recv Thrpt |");

    // print statistics by first receiver core's control function
    if(worker_core_id == INTERNAL_CONF(sc_config)->recv_core_idx[0]){
        for(i=0; i<INTERNAL_CONF(sc_config)->nb_recv_cores; i++){
            current_ns = sc_util_timestamp_ns();

            // obtain both the logical and physical core id
            target_core_id = INTERNAL_CONF(sc_config)->recv_core_idx[i];
            sc_util_get_logical_core_id_by_core_id(sc_config, target_core_id, &target_logical_core_id);

            // calculate interval
            record_interval = current_ns 
                - PER_CORE_APP_META_BY_CORE_ID(sc_config,target_logical_core_id).last_recv_record_timestamp;

            // calculate #pkts within the pass interval
            nb_interval_recv_pkt 
                = PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).nb_interval_recv_pkt;

            // reset metadata
            PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).nb_interval_recv_pkt = 0;
            PER_CORE_APP_META_BY_CORE_ID(sc_config, target_logical_core_id).last_recv_record_timestamp = current_ns;

            // calculate throughput
            recv_throughput = ((double)nb_interval_recv_pkt) / ((double)record_interval) * ((double)1000.f);

            // insert log string
            if(target_core_id < 10)
                sprintf(print_title, "%s     Core %u    |", print_title, target_core_id);
            else
                sprintf(print_title, "%s    Core %u    |", print_title, target_core_id);
            sprintf(print_recv_statistics, "%s %lf Mpps |", print_recv_statistics, recv_throughput);
        }

        // print log
        SC_LOG("Receiver Throughput\n%s\n%s\n", print_title, print_recv_statistics);
    }

    return SC_SUCCESS;
}

/*!
 * \brief   callback while exiting control-plane thread (for receiver)
 * \param   sc_config       the global configuration
 * \param   worker_core_id  the core id of the worker
 * \return  zero for successfully execution
 */
int _control_exit_receiver(struct sc_config *sc_config, uint32_t worker_core_id){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i, result = SC_SUCCESS;
    uint32_t nb_recorded_send_core = 0, nb_recorded_recv_core = 0, core_id;

    if(sc_config->nb_used_cores % 2 != 0){
        SC_ERROR_DETAILS("number of used cores (%u) should be a power of 2",
            sc_config->nb_used_cores
        );
        return SC_ERROR_INVALID_VALUE;
    }

    if(sc_config->nb_used_cores != sc_config->nb_rx_rings_per_port*2){
        SC_ERROR_DETAILS("number of rx queues (%u) should be half of the number of used cores (%u)",
            sc_config->nb_rx_rings_per_port,
            sc_config->nb_used_cores
        );
        return SC_ERROR_INVALID_VALUE;
    }

    if(sc_config->nb_used_cores != sc_config->nb_tx_rings_per_port*2){
        SC_ERROR_DETAILS("number of tx queues (%u) should be half of the number of used cores (%u)",
            sc_config->nb_tx_rings_per_port,
            sc_config->nb_used_cores
        );
        return SC_ERROR_INVALID_VALUE;
    }

    // FIXME: should this managed by configuration file? (i.e., dynamically control #cores for sending and recving)
    INTERNAL_CONF(sc_config)->send_core_idx = (uint32_t*)rte_malloc(NULL, sizeof(uint32_t)*sc_config->nb_used_cores/2, NULL);
    INTERNAL_CONF(sc_config)->recv_core_idx = (uint32_t*)rte_malloc(NULL, sizeof(uint32_t)*sc_config->nb_used_cores/2, NULL);
    assert(INTERNAL_CONF(sc_config)->send_core_idx != NULL);
    assert(INTERNAL_CONF(sc_config)->recv_core_idx != NULL);

    /* 
        dispatch processing functions:
        using half of cores for sending, half of cores for receiving
    */
    for(i=0; i<sc_config->nb_used_cores; i++){
        // obtain the physical core id
        result = sc_util_get_core_id_by_logical_core_id(sc_config, i, &core_id);
        if(unlikely(result != SC_SUCCESS)){
            SC_ERROR_DETAILS("failed to get core id by logical core id %u", i);
            goto _init_app_exit;
        }

        if(i < sc_config->nb_used_cores/2){
            /* sender (worker functions) */
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter_sender;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client_sender;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit_sender;
            /* sender (control functions) */
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_enter_func = _control_enter_sender;
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_infly_func = _control_infly_sender;
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_exit_func = _control_exit_sender;
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_interval = 1000000;

            INTERNAL_CONF(sc_config)->send_core_idx[nb_recorded_send_core] = core_id;
            nb_recorded_send_core += 1;
        } else {
            /* receiver (worker functions) */
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter_receiver;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client_receiver;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit_receiver;
            /* sender (control functions) */
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_enter_func = _control_enter_receiver;
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_infly_func = _control_infly_receiver;
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_exit_func = _control_exit_receiver;
            PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_interval = 1000000;

            INTERNAL_CONF(sc_config)->recv_core_idx[nb_recorded_recv_core] = core_id;
            nb_recorded_recv_core += 1;
        }
    }

    INTERNAL_CONF(sc_config)->nb_send_cores = nb_recorded_send_core;
    INTERNAL_CONF(sc_config)->nb_recv_cores = nb_recorded_recv_core;

_init_app_exit:
    return result;
}