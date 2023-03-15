#include "sc_global.h"
#include "sc_app.h"
#include "sc_echo_client/echo_client.h"
#include "sc_utils/pktgen.h"
#include "sc_utils/rss.h"
#include "sc_utils.h"
#include "sc_log.h"

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    #if defined(MODE_LATENCY)
        if(INTERNAL_CONF(sc_config)->nb_pkt_per_burst != 1){
            SC_WARNING_DETAILS("suggest to use 1 pkt per brust while testing rtt");
            return SC_ERROR_INVALID_VALUE;
        }
    #endif
    return SC_SUCCESS;
}

/*!
 * \brief   parse application-specific key-value configuration pair
 * \param   key         the key of the config pair
 * \param   value       the value of the config pair
 * \param   sc_config   the global configuration
 * \return  zero for successfully parsing
 */
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config){
    int result = SC_SUCCESS;

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
    
    /* whether to set budget of the number of send packet */
    if(!strcmp(key, "enable_nb_pkt_budget")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        if (!strcmp(value, "true")){
            INTERNAL_CONF(sc_config)->enable_nb_pkt_budget = true;
        } else if (!strcmp(value, "false")){
            INTERNAL_CONF(sc_config)->enable_nb_pkt_budget = false;
        } else {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_enable_nb_pkt_budget;
        }

        if(INTERNAL_CONF(sc_config)->enable_nb_pkt_budget == sc_config->enable_test_duration_limit){
            SC_ERROR_DETAILS(
                "only support either enable_nb_pkt_budget and enable_test_duration_limit be enabled\n"
            );
            goto invalid_enable_nb_pkt_budget;
        }

        goto _parse_app_kv_pair_exit;

invalid_enable_nb_pkt_budget:
        SC_ERROR_DETAILS("invalid configuration enable_nb_pkt_budget\n");
    }

    /* number of packet to be send */
    if(!strcmp(key, "nb_pkt_budget")){
        value = sc_util_del_both_trim(value);
        sc_util_del_change_line(value);
        uint32_t nb_pkt_budget;
        if(sc_util_atoui_32(value, &nb_pkt_budget) != SC_SUCCESS) {
            result = SC_ERROR_INVALID_VALUE;
            goto invalid_nb_pkt_budget;
        }
        INTERNAL_CONF(sc_config)->nb_pkt_budget = nb_pkt_budget;
        goto _parse_app_kv_pair_exit;

invalid_nb_pkt_budget:
        SC_ERROR_DETAILS("invalid configuration nb_pkt_budget\n");
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
    int i, result = SC_SUCCESS;
    
    /* generate random packet header */
    result = sc_util_generate_random_pkt_hdr(
        /* sc_pkt_hdr */ &PER_CORE_APP_META(sc_config).test_pkt,
        /* pkt_len */ INTERNAL_CONF(sc_config)->pkt_len,
        /* nb_queues */ sc_config->nb_rx_rings_per_port,
        /* used_queue_id */ rte_lcore_index(rte_lcore_id()),
        /* l3_type */ RTE_ETHER_TYPE_IPV4,
        /* l4_type */ IPPROTO_UDP,
        /* rss_hash_field */ sc_config->rss_hash_field
    );
    if(result != SC_SUCCESS){
        SC_THREAD_ERROR("failed to generate new pkt header");
        goto _process_enter_exit;
    }

    /* allocate array for pointers to storing send pkt_bufs */
    PER_CORE_APP_META(sc_config).send_pkt_bufs = (struct rte_mbuf **)rte_malloc(NULL, 
        sizeof(struct rte_mbuf*)*INTERNAL_CONF(sc_config)->nb_pkt_per_burst, 0);
    if(unlikely(!PER_CORE_APP_META(sc_config).send_pkt_bufs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for send_pkt_bufs");
        result = SC_ERROR_MEMORY;
        goto _process_enter_exit;
    }

    /* allocate array for pointers to storing received pkt_bufs */
    PER_CORE_APP_META(sc_config).recv_pkt_bufs = (struct rte_mbuf **)rte_malloc(NULL, 
        sizeof(struct rte_mbuf*)*INTERNAL_CONF(sc_config)->nb_pkt_per_burst, 0);
    if(unlikely(!PER_CORE_APP_META(sc_config).recv_pkt_bufs)){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for send_pkt_bufs");
        result = SC_ERROR_MEMORY;
        goto _process_enter_exit;
    }

    #if defined(MODE_LATENCY)
        PER_CORE_APP_META(sc_config).min_rtt_sec = 100;
        PER_CORE_APP_META(sc_config).min_rtt_usec = 0;
        PER_CORE_APP_META(sc_config).max_rtt_sec = 0;
        PER_CORE_APP_META(sc_config).max_rtt_usec = 0;
    #endif

    /* initialize the waiting confirm indicator */
    PER_CORE_APP_META(sc_config).wait_ack = 0;

    /* initialize the number of packet to be sent (per core) */
    PER_CORE_APP_META(sc_config).nb_pkt_budget_per_core
        = INTERNAL_CONF(sc_config)->nb_pkt_budget / sc_config->nb_used_cores;

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
 * \brief   callback for processing packet
 * \param   pkt             the received packet
 * \param   sc_config       the global configuration
 * \param   fwd_port_id     specified the forward port index if need to forward packet
 * \param   need_forward    indicate whether need to forward packet, default to be false
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t *fwd_port_id, bool *need_forward){
    SC_WARNING_DETAILS("_process_pkt not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback for client logic
 * \param   sc_config       the global configuration
 * \param   queue_id        the index of the queue for current core to tx/rx packet
 * \param   ready_to_exit   indicator for exiting worker loop
 * \return  zero for successfully executing
 */
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit){
    int i, j, nb_tx, nb_rx, result = SC_SUCCESS, finial_retry_times = 0;
    struct timeval current_time;
    long interval_sec;
    long interval_usec;
    
    struct timeval recv_time;

    if(likely(PER_CORE_APP_META(sc_config).wait_ack > 0)){
try_receive_ack:
        if(sc_force_quit){
            goto process_client_ready_to_exit;
        }

        /* try receive ack */
        for(i=0; i<sc_config->nb_used_ports; i++){
            nb_rx = rte_eth_rx_burst(
                /* port_id */ sc_config->port_ids[i], 
                /* queue_id */ queue_id, 
                /* rx_pkts */ PER_CORE_APP_META(sc_config).recv_pkt_bufs, 
                /* nb_pkts */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst*4
            );

            if(nb_rx == 0) { 
                continue; 
            } else {
                #if defined(MODE_LATENCY)
                    /* record recv time */
                    if(unlikely(-1 == gettimeofday(&recv_time, NULL))){
                        SC_THREAD_ERROR_DETAILS("failed to obtain recv time");
                        result = SC_ERROR_INTERNAL;
                        goto process_client_ready_to_exit;
                    }

                    /* calculate interval time */
                    interval_sec 
                        = recv_time.tv_sec - PER_CORE_APP_META(sc_config).last_send_time.tv_sec;
                    interval_usec 
                        = recv_time.tv_usec - PER_CORE_APP_META(sc_config).last_send_time.tv_usec;

                    /* record the minimal interval time */
                    if(SC_UTIL_TIME_INTERVL_US(interval_sec, interval_usec)  
                        < SC_UTIL_TIME_INTERVL_US(
                            PER_CORE_APP_META(sc_config).min_rtt_sec, 
                            PER_CORE_APP_META(sc_config).min_rtt_usec)
                    ){
                        PER_CORE_APP_META(sc_config).min_rtt_sec = interval_sec;
                        PER_CORE_APP_META(sc_config).min_rtt_usec = interval_usec;
                    }

                    /* record the maximum interval time */
                    if(SC_UTIL_TIME_INTERVL_US(interval_sec, interval_usec)  
                        > SC_UTIL_TIME_INTERVL_US(
                            PER_CORE_APP_META(sc_config).max_rtt_sec, 
                            PER_CORE_APP_META(sc_config).max_rtt_usec)
                    ){
                        PER_CORE_APP_META(sc_config).max_rtt_sec = interval_sec;
                        PER_CORE_APP_META(sc_config).max_rtt_usec = interval_usec;
                    }                    
                #endif
                
                if(PER_CORE_APP_META(sc_config).wait_ack >= nb_rx){
                    PER_CORE_APP_META(sc_config).wait_ack -= nb_rx;
                } else {
                    PER_CORE_APP_META(sc_config).wait_ack = 0;
                }
                    
                PER_CORE_APP_META(sc_config).nb_confirmed_pkt += nb_rx;
            }
        }

        /* return back recv pkt_mbuf */
        for(i=0; i<nb_rx; i++) {
            if(PER_CORE_APP_META(sc_config).recv_pkt_bufs[i]){
                rte_pktmbuf_free(PER_CORE_APP_META(sc_config).recv_pkt_bufs[i]); 
            }
        }

        #if defined(MODE_LATENCY)
            /* 
             * under testing latency mode, one should wait 
             * all sent pkt be confirmed, then try to send
             * the next brust 
             */
            if(PER_CORE_APP_META(sc_config).wait_ack){ goto try_receive_ack; }
        #endif

        /* if the number of packet to be sent reach the budget (if set), 
         * keep trying receiving ack, then exit 
         */
        if( INTERNAL_CONF(sc_config)->enable_nb_pkt_budget &&
            PER_CORE_APP_META(sc_config).nb_send_pkt >= PER_CORE_APP_META(sc_config).nb_pkt_budget_per_core
        ){
            if(PER_CORE_APP_META(sc_config).wait_ack && finial_retry_times <= 10000){
                finial_retry_times += 1;
                goto try_receive_ack;
            } else {
                goto process_client_ready_to_exit;
            }
        }
    }

    /* assemble fininal pkt brust */
    if(SC_SUCCESS != sc_util_generate_packet_burst_proto(
            /* mp */ PER_CORE_MBUF_POOL(sc_config), 
            /* pkts_burst */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
            /* eth_hdr */ &PER_CORE_APP_META(sc_config).test_pkt.pkt_eth_hdr,
            /* vlan_enabled */ 0,
            /* ip_hdr */ &PER_CORE_APP_META(sc_config).test_pkt.pkt_ipv4_hdr,
            /* ipv4 */ 1,
            /* proto */ IPPROTO_UDP,
            /* proto_hdr */ &PER_CORE_APP_META(sc_config).test_pkt.pkt_udp_hdr,
            /* nb_pkt_per_burst */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst,
            /* pkt_len */ INTERNAL_CONF(sc_config)->pkt_len
    )){
        SC_THREAD_ERROR("failed to assemble final packet");
        result = SC_ERROR_INTERNAL;
        goto process_client_ready_to_exit;
    }

    /* send packet */
    nb_tx = 0;
    for(i=0; i<sc_config->nb_used_ports; i++){
        nb_tx += rte_eth_tx_burst(
            /* port_id */ sc_config->port_ids[i],
            /* queue_id */ queue_id,
            /* tx_pkts */ PER_CORE_APP_META(sc_config).send_pkt_bufs,
            /* nb_pkts */ INTERNAL_CONF(sc_config)->nb_pkt_per_burst
        );
    }

    /* record send time */
    if(unlikely(-1 == gettimeofday(&PER_CORE_APP_META(sc_config).last_send_time, NULL))){
        SC_THREAD_ERROR_DETAILS("failed to obtain send time");
        result = SC_ERROR_INTERNAL;
        goto process_client_ready_to_exit;
    }

    /* update metadata */
    PER_CORE_APP_META(sc_config).nb_send_pkt += nb_tx;
    PER_CORE_APP_META(sc_config).nb_last_send_pkt = nb_tx;
    PER_CORE_APP_META(sc_config).wait_ack += nb_tx;

    /* return back send pkt_mbuf */
    for(j=0; j<INTERNAL_CONF(sc_config)->nb_pkt_per_burst; j++) {
        rte_pktmbuf_free(PER_CORE_APP_META(sc_config).send_pkt_bufs[j]); 
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
int _process_exit(struct sc_config *sc_config){
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
    SC_THREAD_LOG("send %d packets in total, %d comfirmed",
        PER_CORE_APP_META(sc_config).nb_send_pkt,
        PER_CORE_APP_META(sc_config).nb_confirmed_pkt
    );

    #if defined(MODE_LATENCY)
        SC_THREAD_LOG("max round-trip-time: %d us", SC_UTIL_TIME_INTERVL_US(
            PER_CORE_APP_META(sc_config).max_rtt_sec, 
            PER_CORE_APP_META(sc_config).max_rtt_usec)
        )
        SC_THREAD_LOG("min round-trip-time: %d us", SC_UTIL_TIME_INTERVL_US(
            PER_CORE_APP_META(sc_config).min_rtt_sec, 
            PER_CORE_APP_META(sc_config).min_rtt_usec)
        )
    #endif

    #if defined(MODE_THROUGHPUT)
        SC_THREAD_LOG("throughput: %f Gbps",
            (float)(PER_CORE_APP_META(sc_config).nb_send_pkt * INTERNAL_CONF(sc_config)->pkt_len * 8) 
            / (float)(SC_UTIL_TIME_INTERVL_US(total_interval_sec, total_interval_usec) * 1000)
        );
    #endif

    /* 
     * NOTE: wait all work threads to reach here then exit,
     *       to ensure all packets go to the correct core
     */
    /*
     * TODO: still have some packets flow to other cores
     */
    pthread_barrier_wait(&sc_config->pthread_barrier);

    return SC_SUCCESS;
}