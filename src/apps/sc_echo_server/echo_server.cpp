#include "sc_global.hpp"
#include "sc_echo_server/echo_server.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_app.hpp"
#include "sc_utils/pktgen.hpp"

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

_parse_app_kv_pair_exit:
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter(struct sc_config *sc_config){
    return SC_SUCCESS;
}

/*!
 * \brief   callback for processing packet
 * \param   pkt             the received packet
 * \param   sc_config       the global configuration
 * \param   recv_port_id    the index of the port that received this packet
 * \param   fwd_port_id     specified the forward port index if need to forward packet
 * \param   need_forward    indicate whether need to forward packet, default to be false
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf **pkt, uint64_t nb_recv_pkts, struct sc_config *sc_config, uint16_t queue_id, uint16_t recv_port_id, uint16_t *fwd_port_id, uint64_t *nb_fwd_pkts){
    uint64_t i;
    struct timeval current_time;
    struct sc_timestamp_table *payload_timestamp;
    uint64_t recv_ns, send_ns;
    long interval_s, interval_us, interval_overall_us;

    /* record recv time */
    recv_ns = sc_util_timestamp_ns();

    // FIXME: move the print info to log thread
    /* record current time */
    if(unlikely(-1 == gettimeofday(&current_time, NULL))){
        SC_THREAD_ERROR_DETAILS("failed to obtain current time");
        return SC_ERROR_INTERNAL;
    }

    interval_s = current_time.tv_sec - PER_CORE_APP_META(sc_config).last_record_time.tv_sec;
    interval_us = current_time.tv_usec - PER_CORE_APP_META(sc_config).last_record_time.tv_usec;
    interval_overall_us = SC_UTIL_TIME_INTERVL_US(interval_s, interval_us);

    if(interval_overall_us >= 1000 * 500){
        if(PER_CORE_APP_META(sc_config).nb_interval_forward_pkt != 0){
            double throughput = (double)((PER_CORE_APP_META(sc_config).nb_interval_forward_pkt)*pkt[0]->pkt_len)
                / (double)(interval_overall_us*1000) /* Gbps */;
            double drop_throughput = (double)((PER_CORE_APP_META(sc_config).nb_interval_drop_pkt)*pkt[0]->pkt_len)
                / (double)(interval_overall_us*1000) /* Gbps */;
            
            if(throughput != 0.0){
                PER_CORE_APP_META(sc_config).throughput[PER_CORE_APP_META(sc_config).throughput_pointer] 
                    = throughput / (double)pkt[0]->pkt_len * (double)1000 /* MOps */;
                PER_CORE_APP_META(sc_config).nb_throughput += 1;
                PER_CORE_APP_META(sc_config).throughput_pointer += 1;
                if(PER_CORE_APP_META(sc_config).throughput_pointer == SC_ECHO_SERVER_NB_THROUGHPUT){
                    PER_CORE_APP_META(sc_config).throughput_pointer = 0;
                }

                SC_THREAD_LOG(
                    "throughput: %lf Mpps, drop throughput: %lf Mpps",
                    throughput / (double)pkt[0]->pkt_len * (double)1000,
                    drop_throughput / (double)pkt[0]->pkt_len * (double)1000
                );
            }
        } else {
            // SC_THREAD_LOG("throughput: 0.0 Mpps");
        }
        
        /* reset */
        PER_CORE_APP_META(sc_config).last_record_time.tv_sec = current_time.tv_sec;
        PER_CORE_APP_META(sc_config).last_record_time.tv_usec = current_time.tv_usec;
        PER_CORE_APP_META(sc_config).nb_interval_forward_pkt = 0;
        PER_CORE_APP_META(sc_config).nb_interval_drop_pkt = 0;
    }
    
    /* count */
    PER_CORE_APP_META(sc_config).nb_forward_pkt += nb_recv_pkts;
    PER_CORE_APP_META(sc_config).nb_interval_forward_pkt += nb_recv_pkts;
    
    /* set as forwarded, default to first send port */
    *nb_fwd_pkts = nb_recv_pkts;
    *fwd_port_id  = INTERNAL_CONF(sc_config)->send_port_idx[0];

    send_ns = sc_util_timestamp_ns();
    for(i=0; i<nb_recv_pkts; i++){
        payload_timestamp = rte_pktmbuf_mtod_offset(
            pkt[i], struct sc_timestamp_table*, 
            sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr)
        );
        
        /* add both the recv & send timestamp to the timestamp table */
        sc_util_add_half_timestamp(payload_timestamp, recv_ns);
        sc_util_add_half_timestamp(payload_timestamp, send_ns);
    }

    return SC_SUCCESS;
}

/*!
 * \brief   callback for processing packets to be dropped
 * \param   pkt             the packets to be dropped
 * \param   nb_drop_pkts    number of packets to be dropped
 * \return  zero for successfully processing
 */
int _process_pkt_drop(struct sc_config *sc_config, struct rte_mbuf **pkt, uint64_t nb_drop_pkts){
    PER_CORE_APP_META(sc_config).nb_drop_pkt += nb_drop_pkts;
    PER_CORE_APP_META(sc_config).nb_forward_pkt -= nb_drop_pkts;
    PER_CORE_APP_META(sc_config).nb_interval_drop_pkt += nb_drop_pkts;
    PER_CORE_APP_META(sc_config).nb_interval_forward_pkt -= nb_drop_pkts;
    return SC_SUCCESS;
}

/*!
 * \brief   callback for client logic
 * \param   sc_config       the global configuration
 * \param   queue_id        the index of the queue for current core to tx/rx packet
 * \param   ready_to_exit   indicator for exiting worker loop
 * \return  zero for successfully executing
 */
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit){
    SC_WARNING_DETAILS("_process_client not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback while exiting application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_exit(struct sc_config *sc_config){
    int i;
    double overall_throughput = 0.0;

    for(i=0; i<PER_CORE_APP_META(sc_config).nb_throughput; i++){
        overall_throughput += (double)PER_CORE_APP_META(sc_config).throughput[i];
    }
    if((double)PER_CORE_APP_META(sc_config).nb_throughput != 0) {
        PER_CORE_APP_META(sc_config).average_throughput 
            = overall_throughput / (double)PER_CORE_APP_META(sc_config).nb_throughput;
    } else {
        PER_CORE_APP_META(sc_config).average_throughput = 0.0f;
    }
    
    SC_THREAD_LOG("average throughput: %lf MOps", PER_CORE_APP_META(sc_config).average_throughput);
    SC_THREAD_LOG("forward %u packets in total", PER_CORE_APP_META(sc_config).nb_forward_pkt);
    SC_THREAD_LOG("drop %u packets in total", PER_CORE_APP_META(sc_config).nb_drop_pkt);
    return SC_SUCCESS;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _all_exit(struct sc_config *sc_config){
    int i;
    uint64_t overall_forward = 0, overall_drop = 0;
    double overall_throughput = 0.0f;

    for(i=0; i<sc_config->nb_used_cores; i++){
        overall_drop +=
            PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_drop_pkt;
        overall_forward +=
            PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_forward_pkt;
        overall_throughput +=
            PER_CORE_APP_META_BY_CORE_ID(sc_config, i).average_throughput;
    }

    SC_THREAD_LOG("[TOTAL] Forward %lu pkts", overall_forward);
    SC_THREAD_LOG("[TOTAL] Drop %lu pkts", overall_drop);
    SC_THREAD_LOG("[TOTAL] Throughput: %lf MOps", overall_throughput);
    return SC_SUCCESS;
}

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i;
    
    for(i=0; i<sc_config->nb_used_cores; i++){
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_pkt_func = _process_pkt;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_pkt_drop_func = _process_pkt_drop;
    }

    return SC_SUCCESS;
}
