#include "sc_global.h"
#include "sc_worker.h"
#include "sc_app.h"
#include "sc_echo_client/echo_client.hpp"
#include "sc_utils/distribution_gen.hpp"
#include "sc_utils/timestamp.hpp"
#include "sc_utils/pktgen.h"
#include "sc_utils/rss.h"
#include "sc_utils/tail_latency.h"
#include "sc_utils.h"
#include "sc_log.h"

#ifdef __cplusplus
    extern "C" {
#endif

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

    /* send flow rate */
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
    
    /* initialize interval generator */
    double per_core_pkt_rate = (double)INTERNAL_CONF(sc_config)->bit_rate 
                                / (double)sc_config->nb_used_cores 
                                / (double) 8.0 / (double)INTERNAL_CONF(sc_config)->pkt_len;
    double per_core_pkt_interval = (double) 1.0 / (double) per_core_pkt_rate; /* ns */
    PER_CORE_APP_META(sc_config).interval_generator 
        = new sc_util_exponential_uint64_generator((int)per_core_pkt_interval);
    PER_CORE_APP_META(sc_config).interval = PER_CORE_APP_META(sc_config).interval_generator->next();
    PER_CORE_APP_META(sc_config).last_send_timestamp = sc_util_timestamp_ns();

    /* TODO: print for debug */    
    SC_THREAD_LOG("per core packet rate: %lf Gpps", per_core_pkt_rate);
    SC_THREAD_LOG("per core packet interval: %lf ns, (int)%d ns",
        per_core_pkt_interval, (int)per_core_pkt_interval);
    SC_THREAD_LOG("initialize interval: %lu ns", PER_CORE_APP_META(sc_config).interval);

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

    /* generate random packet header */
    for(i=0; i<INTERNAL_CONF(sc_config)->nb_flow_per_core; i++){
        result = sc_util_generate_random_pkt_hdr(
            /* sc_pkt_hdr */ &PER_CORE_APP_META(sc_config).test_pkts[i],
            /* pkt_len */ INTERNAL_CONF(sc_config)->pkt_len,
            /* nb_queues */ sc_config->nb_rx_rings_per_port,
            /* used_queue_id */ rte_lcore_index(rte_lcore_id()),
            /* l3_type */ RTE_ETHER_TYPE_IPV4,
            /* l4_type */ IPPROTO_UDP,
            /* rss_hash_field */ sc_config->rss_hash_field,
            /* rss_affinity */ false
        );
        if(result != SC_SUCCESS){
            SC_THREAD_ERROR("failed to generate new pkt header");
            goto _process_enter_exit;
        }
    }
    
    /* allocate array for pointers to storing send pkt_bufs */
    PER_CORE_APP_META(sc_config).send_pkt_bufs = (struct rte_mbuf **)rte_malloc(NULL, 
        sizeof(struct rte_mbuf*)*INTERNAL_CONF(sc_config)->nb_pkt_per_burst, 0);
    if(unlikely(!PER_CORE_APP_META(sc_config).send_pkt_bufs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for send_pkt_bufs");
        result = SC_ERROR_MEMORY;
        goto _process_enter_exit;
    }

    #if defined(MODE_LATENCY)
        PER_CORE_APP_META(sc_config).min_rtt_sec = 100;
        PER_CORE_APP_META(sc_config).min_rtt_usec = 0;
        PER_CORE_APP_META(sc_config).max_rtt_sec = 0;
        PER_CORE_APP_META(sc_config).max_rtt_usec = 0;
        PER_CORE_APP_META(sc_config).nb_latency_data = 0;
        PER_CORE_APP_META(sc_config).latency_data_pointer = 0;
    #endif

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
    int i, j, nb_tx = 0, nb_send_pkt = 0, result = SC_SUCCESS;
    uint64_t current_ns;

    /* send packet */
    for(i=0; i<INTERNAL_CONF(sc_config)->nb_send_ports; i++){
        /* avoid endless loop */
        if(sc_force_quit){ break; }

        /* check send interval */
        current_ns = sc_util_timestamp_ns();
        if(PER_CORE_APP_META(sc_config).last_send_timestamp - current_ns < PER_CORE_APP_META(sc_config).interval){
            i -= 1;
            continue;
        }

        /* assemble pkt brust */
        if(SC_SUCCESS != sc_util_generate_packet_burst_proto(
                /* mp */ PER_CORE_TX_MBUF_POOL(sc_config, INTERNAL_CONF(sc_config)->send_port_logical_idx[i]),
                /* pkts_burst */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
                /* eth_hdr */ &PER_CORE_APP_META(sc_config).test_pkts[
                                    PER_CORE_APP_META(sc_config).last_used_flow
                                ].pkt_eth_hdr,
                /* vlan_enabled */ 0,
                /* ip_hdr */ &PER_CORE_APP_META(sc_config).test_pkts[
                                    PER_CORE_APP_META(sc_config).last_used_flow
                                ].pkt_ipv4_hdr,
                /* ipv4 */ 1,
                /* proto */ IPPROTO_UDP,
                /* proto_hdr */ &PER_CORE_APP_META(sc_config).test_pkts[
                                    PER_CORE_APP_META(sc_config).last_used_flow
                                ].pkt_udp_hdr,
                /* nb_pkt_per_burst */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst,
                /* pkt_len */ INTERNAL_CONF(sc_config)->pkt_len
        )){
            SC_THREAD_ERROR("failed to assemble final packet");
            result = SC_ERROR_INTERNAL;
            goto process_client_ready_to_exit;
        }

        nb_send_pkt = rte_eth_tx_burst(
            /* port_id */ INTERNAL_CONF(sc_config)->send_port_idx[i],
            /* queue_id */ queue_id,
            /* tx_pkts */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
            /* nb_pkts */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst
        );

        /* return back un-sent pkt_mbuf */
        for(j=nb_send_pkt; j<INTERNAL_CONF(sc_config)->nb_pkt_per_burst; j++) {
            rte_pktmbuf_free(PER_CORE_APP_META(sc_config).send_pkt_bufs[j]); 
        }

        nb_tx += nb_send_pkt;

        /* update sending timestamp and interval */
        PER_CORE_APP_META(sc_config).last_send_timestamp = sc_util_timestamp_ns();
        PER_CORE_APP_META(sc_config).interval = PER_CORE_APP_META(sc_config).interval_generator->next();
    }

    /* update metadata */
    PER_CORE_APP_META(sc_config).nb_send_pkt += nb_tx;
    if(PER_CORE_APP_META(sc_config).last_used_flow == INTERNAL_CONF(sc_config)->nb_flow_per_core){
        PER_CORE_APP_META(sc_config).last_used_flow = 0;
    } else {
        PER_CORE_APP_META(sc_config).last_used_flow += 1;
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
    SC_THREAD_LOG("[sender]: send %d packets in total", PER_CORE_APP_META(sc_config).nb_send_pkt);

    #if defined(MODE_THROUGHPUT)
        SC_THREAD_LOG("[sender]: send throughput: %f Gbps",
            (float)(PER_CORE_APP_META(sc_config).nb_send_pkt * INTERNAL_CONF(sc_config)->pkt_len * 8) 
            / (float)(SC_UTIL_TIME_INTERVL_US(total_interval_sec, total_interval_usec) * 1000)
        );
    #endif

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

    /* allocate array for pointers to storing received pkt_bufs */
    PER_CORE_APP_META(sc_config).recv_pkt_bufs = (struct rte_mbuf **)rte_malloc(NULL, 
        sizeof(struct rte_mbuf*)*SC_MAX_PKT_BURST*2, 0);
    if(unlikely(!PER_CORE_APP_META(sc_config).recv_pkt_bufs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for send_pkt_bufs");
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
    int i, j, nb_rx = 0, nb_recv_pkt = 0, result = SC_SUCCESS;

    for(i=0; i<INTERNAL_CONF(sc_config)->nb_recv_ports; i++){
        memset(PER_CORE_APP_META(sc_config).recv_pkt_bufs, 0, sizeof(struct rte_mbuf*)*SC_MAX_PKT_BURST*2);
        nb_recv_pkt = rte_eth_rx_burst(
            /* port_id */ INTERNAL_CONF(sc_config)->recv_port_idx[i], 
            /* queue_id */ queue_id, 
            /* rx_pkts */ PER_CORE_APP_META(sc_config).recv_pkt_bufs, 
            /* nb_pkts */ SC_MAX_PKT_BURST
        );

        if(nb_recv_pkt == 0) { 
            continue;
        }

        PER_CORE_APP_META(sc_config).nb_confirmed_pkt += nb_recv_pkt;

        /* return back recv pkt_mbuf */
        for(j=0; j<nb_recv_pkt; j++) {
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

    SC_THREAD_LOG("[receiver] confirmed pkt: %u", PER_CORE_APP_META(sc_config).nb_confirmed_pkt);

    return SC_SUCCESS;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _all_exit(struct sc_config *sc_config){
    int i;
    size_t nb_send_pkt = 0, nb_confirmed_pkt = 0;

    for(i=0; i<sc_config->nb_used_cores; i++){
        /* reduce number of packet */
        nb_send_pkt += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_send_pkt;
        nb_confirmed_pkt += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_confirmed_pkt;
    }
    SC_LOG("[TOTAL] number of send packet: %ld, number of confirm packet: %ld",
        nb_send_pkt, nb_confirmed_pkt
    );

    #if defined(MODE_THROUGHPUT)
        long worker_max_interval_sec = 0, worker_max_interval_usec = 0;

        for(i=0; i<sc_config->nb_used_cores; i++){
            /* record maximum duration */
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
        SC_THREAD_LOG("[TOTAL] packet length: %u",
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
    #endif

    return SC_SUCCESS;
}

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i;

    #if defined(MODE_LATENCY)
        if(INTERNAL_CONF(sc_config)->nb_pkt_per_burst != 1){
            SC_ERROR_DETAILS("suggest to use 1 pkt per brust while testing rtt");
            return SC_ERROR_INVALID_VALUE;
        }
    #endif

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

    /* 
        dispatch processing functions:
            using half of cores for sending, half of cores for receiving
    */
    for(i=0; i<sc_config->nb_used_cores; i++){
        if(i < sc_config->nb_used_cores/2){
            /* sender */
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter_sender;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client_sender;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit_sender;
        } else {
            /* receiver */
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter_receiver;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client_receiver;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit_receiver;
        }
    }

    return SC_SUCCESS;
}

#ifdef __cplusplus
    }
#endif