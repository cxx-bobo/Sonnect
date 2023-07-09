#include "sc_global.hpp"
#include "sc_sha/sha.hpp"
#include "sc_utils/tail_latency.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_control_plane.hpp"
#include "sc_app.hpp"

int __reload_sha_state(struct sc_config *sc_config);

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
    #if defined(SC_HAS_DOCA) && defined(SC_NEED_DOCA_SHA)
        /* use DOCA SHA to accelerate the hash calculation */
        int doca_result;

        char *dst_buffer = NULL;
        char *src_buffer = NULL;
        struct doca_sha_job *sha_job;

        /* allocate SHA engine result buffer */
        // FIXME: don't use malloc!
        dst_buffer = (char*)malloc(DOCA_SHA256_BYTE_COUNT);
        if (dst_buffer == NULL) {
            SC_THREAD_ERROR_DETAILS("failed to malloc memory for dst_buffer");
            return SC_ERROR_MEMORY;
        }
        memset(dst_buffer, 0, DOCA_SHA256_BYTE_COUNT);
        PER_CORE_DOCA_META(sc_config).sha_dst_buffer = dst_buffer;

        /* allocate buffer for storing input source data */
        src_buffer = (char*)malloc(SC_SHA_HASH_KEY_LENGTH);
        if (src_buffer == NULL) {
            SC_THREAD_ERROR_DETAILS("failed to malloc memory for src_buffer");
            return SC_ERROR_MEMORY;
        }
        memset(src_buffer, 0, SC_SHA_HASH_KEY_LENGTH);
        PER_CORE_DOCA_META(sc_config).sha_src_buffer = src_buffer;

        /* populate dst_buffer to memory map */
        doca_result = doca_mmap_populate(
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
            /* addr */ PER_CORE_DOCA_META(sc_config).sha_dst_buffer,
            /* len */ DOCA_SHA256_BYTE_COUNT,
            /* pg_sz */ sysconf(_SC_PAGESIZE),
            /* free_cb */ sc_doca_util_mmap_populate_free_cb,
            /* opaque */ NULL
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to populate memory dst_buffer to memory map: %s", 
                doca_get_error_string((doca_error_t)doca_result));
            return SC_ERROR_MEMORY;
        }

        /* populate src_buffer to memory map */
        doca_result = doca_mmap_populate(
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
            /* addr */ PER_CORE_DOCA_META(sc_config).sha_src_buffer,
            /* len */ SC_SHA_HASH_KEY_LENGTH,
            /* pg_sz */ sysconf(_SC_PAGESIZE),
            /* free_cb */ NULL,
            /* opaque */ NULL
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to populate memory src_buffer to memory map: %s",
                doca_get_error_string((doca_error_t)doca_result));
            return SC_ERROR_MEMORY;
        }

        /* acquire DOCA buffer for representing source buffer */
        doca_result = doca_buf_inventory_buf_by_addr(
            /* inventory */ PER_CORE_DOCA_META(sc_config).sha_buf_inv,
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
            /* addr */ PER_CORE_DOCA_META(sc_config).sha_src_buffer,
            /* len */ SC_SHA_HASH_KEY_LENGTH,
            /* buf */ &(PER_CORE_DOCA_META(sc_config).sha_src_doca_buf)
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to acquire DOCA buffer representing source buffer: %s",
                doca_get_error_string((doca_error_t)doca_result));
            return SC_ERROR_MEMORY;
        }

        /* set data address and length in the doca_buf. */
        doca_result = doca_buf_set_data(
            /* buf */ PER_CORE_DOCA_META(sc_config).sha_src_doca_buf,
            /* data */ PER_CORE_DOCA_META(sc_config).sha_src_buffer,
            /* data_len */ SC_SHA_HASH_KEY_LENGTH
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to set data address and length in the doca_buf: %s",
                doca_get_error_string((doca_error_t)doca_result));
            return SC_ERROR_MEMORY;
        }

        /* acquire DOCA buffer for representing destination buffer */
        doca_result = doca_buf_inventory_buf_by_addr(
            /* inventory */ PER_CORE_DOCA_META(sc_config).sha_buf_inv,
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
            /* addr */ PER_CORE_DOCA_META(sc_config).sha_dst_buffer,
            /* len */ DOCA_SHA256_BYTE_COUNT,
            /* buf */ &(PER_CORE_DOCA_META(sc_config).sha_dst_doca_buf)
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to acquire DOCA buffer representing destination buffer: %s",
                doca_get_error_string((doca_error_t)doca_result));
            return SC_ERROR_MEMORY;
        }

        /* construct sha job */
        sha_job = (struct doca_sha_job*)malloc(sizeof(struct doca_sha_job));
        if(!sha_job){
            SC_THREAD_ERROR_DETAILS("failed to allocate memory for sha_job");
            return SC_ERROR_MEMORY;
        }
        sha_job->base.type = DOCA_SHA_JOB_SHA256;
        sha_job->base.flags = DOCA_JOB_FLAGS_NONE;
        sha_job->base.ctx = DOCA_CONF(sc_config)->sha_ctx;
        sha_job->base.user_data.u64 = DOCA_SHA_JOB_SHA256;

        sha_job->resp_buf = PER_CORE_DOCA_META(sc_config).sha_dst_doca_buf;
        sha_job->req_buf = PER_CORE_DOCA_META(sc_config).sha_src_doca_buf;
        sha_job->flags = DOCA_SHA_JOB_FLAGS_NONE;
        PER_CORE_DOCA_META(sc_config).sha_job = sha_job;
    #endif

_process_enter_exit:
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
int _process_pkt(struct rte_mbuf **pkt, uint64_t nb_recv_pkts, struct sc_config *sc_config, uint16_t recv_port_id, uint16_t *fwd_port_id, uint64_t *nb_fwd_pkts){
    int i, j, doca_result, result = SC_SUCCESS;
    char tuple_key[SC_SHA_HASH_KEY_LENGTH];
    
    for(j=0; j<nb_recv_pkts; j++){
        struct rte_ether_hdr *_eth_addr;
        struct rte_ipv4_hdr *_ipv4_hdr;
        struct rte_udp_hdr *_udp_hdr;

        memset(tuple_key, 0, SC_SHA_HASH_KEY_LENGTH);

        /* extract value key */
        _eth_addr = rte_pktmbuf_mtod_offset(pkt[j], struct rte_ether_hdr*, 0);
        _ipv4_hdr = rte_pktmbuf_mtod_offset(pkt[j], struct rte_ipv4_hdr*, RTE_ETHER_HDR_LEN);
        _udp_hdr = rte_pktmbuf_mtod_offset(
                pkt[j], struct rte_udp_hdr*, RTE_ETHER_HDR_LEN+rte_ipv4_hdr_len(_ipv4_hdr));

        /* ethernet-tuples */
        #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
            /* five-tuples */
            sprintf(tuple_key, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%u%u%u%u",
                /* source mac address */
                _eth_addr->src_addr.addr_bytes[0], _eth_addr->src_addr.addr_bytes[1],
                _eth_addr->src_addr.addr_bytes[2], _eth_addr->src_addr.addr_bytes[3],
                _eth_addr->src_addr.addr_bytes[4], _eth_addr->src_addr.addr_bytes[5],
                /* destination mac address */
                _eth_addr->dst_addr.addr_bytes[0], _eth_addr->dst_addr.addr_bytes[1],
                _eth_addr->dst_addr.addr_bytes[2], _eth_addr->dst_addr.addr_bytes[3],
                _eth_addr->dst_addr.addr_bytes[4], _eth_addr->dst_addr.addr_bytes[5],
                /* ipv4 address */
                _ipv4_hdr->src_addr, _ipv4_hdr->dst_addr,
                /* udp port */
                _udp_hdr->src_port, _udp_hdr->dst_port
            );
        #else
            sprintf(tuple_key, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                /* source mac address */
                _eth_addr->s_addr.addr_bytes[0], _eth_addr->s_addr.addr_bytes[1],
                _eth_addr->s_addr.addr_bytes[2], _eth_addr->s_addr.addr_bytes[3],
                _eth_addr->s_addr.addr_bytes[4], _eth_addr->s_addr.addr_bytes[5],
                /* destination mac address */
                _eth_addr->d_addr.addr_bytes[0], _eth_addr->d_addr.addr_bytes[1],
                _eth_addr->d_addr.addr_bytes[2], _eth_addr->d_addr.addr_bytes[3],
                _eth_addr->d_addr.addr_bytes[4], _eth_addr->d_addr.addr_bytes[5],
                /* ipv4 address */
                _ipv4_hdr->src_addr, _ipv4_hdr->dst_addr,
                /* udp port */
                _udp_hdr->src_port, _udp_hdr->dst_port
            );
        #endif // RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)

        #if defined(MODE_LATENCY)
            struct timeval start_time, end_time;
            long interval_sec;
            long interval_usec;

            /* record start time */
            // FIXME: don't use gettimeofday
            if(unlikely(-1 == gettimeofday(&start_time, NULL))){
                SC_THREAD_ERROR_DETAILS("failed to obtain recv time");
                result = SC_ERROR_INTERNAL;
                goto _process_pkt_exit;
            }
        #endif

        /* calculate hash value */
        #if defined(SC_HAS_DOCA) && defined(SC_NEED_DOCA_SHA)
            struct doca_event doca_event = {0};
            uint8_t *resp_head;

            /* copy the key to SHA source data buffer */
            memcpy(PER_CORE_DOCA_META(sc_config).sha_src_buffer, tuple_key, SC_SHA_HASH_KEY_LENGTH);

            /* lock the belonging of SHA engine */
            // rte_spinlock_lock(&DOCA_CONF(sc_config)->sha_lock);

            /* enqueue the result */
            doca_result = doca_workq_submit(
                /* workq */ PER_CORE_DOCA_META(sc_config).sha_workq, 
                /* job */ &PER_CORE_DOCA_META(sc_config).sha_job->base
            );
            if(doca_result != DOCA_SUCCESS){
                SC_THREAD_ERROR_DETAILS("failed to enqueue SHA job: %s", doca_get_error_string((doca_error_t)doca_result));
                return SC_ERROR_INTERNAL;
            }

            // FIXME: leave
            
            /* wait for job completion */
            while((doca_result = doca_workq_progress_retrieve(
                    /* workq */ PER_CORE_DOCA_META(sc_config).sha_workq,
                    /* ev */ &doca_event,
                    /* flags */ NULL) == DOCA_ERROR_AGAIN)
            ){}
            if(doca_result != DOCA_SUCCESS){
                SC_THREAD_ERROR_DETAILS("failed to retrieve SHA result: %s", doca_get_error_string((doca_error_t)doca_result));
                return SC_ERROR_INTERNAL;
            }

            /* release the belonging of SHA engine */
            // rte_spinlock_unlock(&DOCA_CONF(sc_config)->sha_lock);

            /* (DEBUG) verify SHA result */
            if(doca_event.result.u64 != DOCA_SUCCESS){
                SC_THREAD_ERROR_DETAILS("sha job finished unsuccessfully");
                return SC_ERROR_INTERNAL;
            } else if (
                ((int)(doca_event.type) != (int)DOCA_SHA_JOB_SHA256) ||
                (doca_event.user_data.u64 != DOCA_SHA_JOB_SHA256)
            ){
                SC_THREAD_ERROR_DETAILS("received wrong event");
                return SC_ERROR_INTERNAL;
            }

            /* retrieve SHA result */
            doca_buf_get_data(PER_CORE_DOCA_META(sc_config).sha_job->resp_buf, (void **)&resp_head);
        #elif defined(SC_HAS_DOCA) && !defined(SC_NEED_DOCA_SHA)
            __reload_sha_state(sc_config);
            sha256_process_arm(
                PER_CORE_APP_META(sc_config).sha_state, (uint8_t*)tuple_key, SC_SHA_HASH_KEY_LENGTH);
        #else
            __reload_sha_state(sc_config);
            sha256_process_x86(
                PER_CORE_APP_META(sc_config).sha_state, (uint8_t*)tuple_key, SC_SHA_HASH_KEY_LENGTH);
        #endif

        // TODO: open this macro
        #if defined(MODE_LATENCY)
            /* record start time */
            if(unlikely(-1 == gettimeofday(&end_time, NULL))){
                SC_THREAD_ERROR_DETAILS("failed to obtain recv time");
                result = SC_ERROR_INTERNAL;
                goto _process_pkt_exit;
            }

            interval_sec = end_time.tv_sec - start_time.tv_sec;
            interval_usec = end_time.tv_usec - start_time.tv_usec;

            if(PER_CORE_APP_META(sc_config).latency_data_pointer == SC_SHA_MAX_LATENCY_NB){
                PER_CORE_APP_META(sc_config).latency_data_pointer = 0;
            }

            PER_CORE_APP_META(sc_config).latency_sec[PER_CORE_APP_META(sc_config).latency_data_pointer] 
                = interval_sec;
            PER_CORE_APP_META(sc_config).latency_usec[PER_CORE_APP_META(sc_config).latency_data_pointer] 
                = interval_usec;

            PER_CORE_APP_META(sc_config).latency_data_pointer += 1;
            if(PER_CORE_APP_META(sc_config).nb_latency_data < SC_SHA_MAX_LATENCY_NB){
                PER_CORE_APP_META(sc_config).nb_latency_data += 1;
            }
        #endif

for_debug:
        PER_CORE_APP_META(sc_config).nb_processed_pkt += 1;
        *nb_fwd_pkts += 1;
    }

    /* set default to first send port */
    *fwd_port_id  = INTERNAL_CONF(sc_config)->send_port_idx[0];

_process_pkt_exit:
    return result;
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
    int result = SC_SUCCESS;

    SC_THREAD_LOG("processed packet: %lu", PER_CORE_APP_META(sc_config).nb_processed_pkt);

    #if defined(MODE_LATENCY)
        if(PER_CORE_APP_META(sc_config).nb_latency_data == 0){
            goto _process_exit_exit;
        }

        long p99 = 0,  p80 = 0, p50 = 0, p10 = 0;
        if(SC_SUCCESS != sc_util_tail_latency(
            /* latency_sec */ PER_CORE_APP_META(sc_config).latency_sec,
            /* latency_usec */ PER_CORE_APP_META(sc_config).latency_usec,
            /* p_latency */ &p99,
            /* nb_latency */ PER_CORE_APP_META(sc_config).nb_latency_data,
            /* percent */ 0.99
        )){
            SC_THREAD_ERROR("failed to obtain p99 tail latency");
            result = SC_ERROR_INTERNAL;
            goto _process_exit_exit;
        }
        PER_CORE_APP_META(sc_config).tail_latency_p99 = p99;

        if(SC_SUCCESS != sc_util_tail_latency(
            /* latency_sec */ PER_CORE_APP_META(sc_config).latency_sec,
            /* latency_usec */ PER_CORE_APP_META(sc_config).latency_usec,
            /* p_latency */ &p80,
            /* nb_latency */ PER_CORE_APP_META(sc_config).nb_latency_data,
            /* percent */ 0.80
        )){
            SC_THREAD_ERROR("failed to obtain p80 tail latency");
            result = SC_ERROR_INTERNAL;
            goto _process_exit_exit;
        }
        PER_CORE_APP_META(sc_config).tail_latency_p80 = p80;

        if(SC_SUCCESS != sc_util_tail_latency(
            /* latency_sec */ PER_CORE_APP_META(sc_config).latency_sec,
            /* latency_usec */ PER_CORE_APP_META(sc_config).latency_usec,
            /* p_latency */ &p50,
            /* nb_latency */ PER_CORE_APP_META(sc_config).nb_latency_data,
            /* percent */ 0.50
        )){
            SC_THREAD_ERROR("failed to obtain p50 tail latency");
            result = SC_ERROR_INTERNAL;
            goto _process_exit_exit;
        }
        PER_CORE_APP_META(sc_config).tail_latency_p50 = p50;

        if(SC_SUCCESS != sc_util_tail_latency(
            /* latency_sec */ PER_CORE_APP_META(sc_config).latency_sec,
            /* latency_usec */ PER_CORE_APP_META(sc_config).latency_usec,
            /* p_latency */ &p10,
            /* nb_latency */ PER_CORE_APP_META(sc_config).nb_latency_data,
            /* percent */ 0.10
        )){
            SC_THREAD_ERROR("failed to obtain p10 tail latency");
            result = SC_ERROR_INTERNAL;
            goto _process_exit_exit;
        }
        PER_CORE_APP_META(sc_config).tail_latency_p10 = p10;

        #if defined(SC_HAS_DOCA) && defined(SC_NEED_DOCA_SHA)
            SC_THREAD_LOG(
                    "tail latency with Bluefield SHA engine: \
                    \n\r\tp99: %ld us \
                    \n\r\tp80: %ld us \
                    \n\r\tp50: %ld us \
                    \n\r\tp10: %ld us", 
                p99, p80, p50, p10
            );
        #elif defined(SC_HAS_DOCA) && !defined(SC_NEED_DOCA_SHA)
            SC_THREAD_LOG(
                    "tail latency with ARM NEON: \
                    \n\r\tp99: %ld us \
                    \n\r\tp80: %ld us \
                    \n\r\tp50: %ld us \
                    \n\r\tp10: %ld us", 
                p99, p80, p50, p10
            );
        #else
            SC_THREAD_LOG(
                    "tail latency with x86 SSE: \
                    \n\r\tp99: %ld us \
                    \n\r\tp80: %ld us \
                    \n\r\tp50: %ld us \
                    \n\r\tp10: %ld us", 
                p99, p80, p50, p10
            );
        #endif // SC_HAS_DOCA, SC_NEED_DOCA_SHA
    
    #endif // MODE_LATENCY

_process_exit_exit:
    return result;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _worker_all_exit(struct sc_config *sc_config){
    int i;
    uint64_t nb_processed_pkt = 0;

    for(i=0; i<sc_config->nb_used_cores; i++){
        nb_processed_pkt += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).nb_processed_pkt;
    }
    SC_THREAD_LOG("[TOTAL] processed packet: %lu", nb_processed_pkt);

    #if defined(MODE_LATENCY)

        long overall_p99 = 0, overall_p80 = 0, overall_p50 = 0, overall_p10 = 0; 

        for(i=0; i<sc_config->nb_used_cores; i++){
            overall_p99 += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).tail_latency_p99;
            overall_p80 += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).tail_latency_p80;
            overall_p50 += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).tail_latency_p50;
            overall_p10 += PER_CORE_APP_META_BY_CORE_ID(sc_config, i).tail_latency_p10;
        }

        #if defined(SC_HAS_DOCA) && defined(SC_NEED_DOCA_SHA)
            SC_THREAD_LOG(
                    "[TOTAL] average tail latency with ARM SHA engine: \
                    \n\r\tp99: %lf us \
                    \n\r\tp80: %lf us \
                    \n\r\tp50: %lf us \
                    \n\r\tp10: %lf us", 
                (double)overall_p99 / (double)sc_config->nb_used_cores,
                (double)overall_p80 / (double)sc_config->nb_used_cores,
                (double)overall_p50 / (double)sc_config->nb_used_cores,
                (double)overall_p10 / (double)sc_config->nb_used_cores
            );
        #elif defined(SC_HAS_DOCA) && !defined(SC_NEED_DOCA_SHA)
            SC_THREAD_LOG(
                    "[TOTAL] average tail latency with ARM NEON: \
                    \n\r\tp99: %lf us \
                    \n\r\tp80: %lf us \
                    \n\r\tp50: %lf us \
                    \n\r\tp10: %lf us", 
                (double)overall_p99 / (double)sc_config->nb_used_cores,
                (double)overall_p80 / (double)sc_config->nb_used_cores,
                (double)overall_p50 / (double)sc_config->nb_used_cores,
                (double)overall_p10 / (double)sc_config->nb_used_cores
            );
        #else
            SC_THREAD_LOG(
                    "[TOTAL] average tail latency with x86 SSE: \
                    \n\r\tp99: %lf us \
                    \n\r\tp80: %lf us \
                    \n\r\tp50: %lf us \
                    \n\r\tp10: %lf us", 
                (double)overall_p99 / (double)sc_config->nb_used_cores,
                (double)overall_p80 / (double)sc_config->nb_used_cores,
                (double)overall_p50 / (double)sc_config->nb_used_cores,
                (double)overall_p10 / (double)sc_config->nb_used_cores
            );
        #endif // SC_HAS_DOCA, SC_NEED_DOCA_SHA
    
    #endif // MODE_LATENCY

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
    }

    return SC_SUCCESS;
}

/*!
 * \brief   reload SHA initial state
 * \param   sc_config   the global configuration
 * \return  zero for successfully reloading
 */
int __reload_sha_state(struct sc_config *sc_config){
    PER_CORE_APP_META(sc_config).sha_state[0] = 0x6a09e667;
    PER_CORE_APP_META(sc_config).sha_state[1] = 0xbb67ae85;
    PER_CORE_APP_META(sc_config).sha_state[2] = 0x3c6ef372;
    PER_CORE_APP_META(sc_config).sha_state[3] = 0xa54ff53a;
    PER_CORE_APP_META(sc_config).sha_state[4] = 0x510e527f;
    PER_CORE_APP_META(sc_config).sha_state[5] = 0x9b05688c;
    PER_CORE_APP_META(sc_config).sha_state[6] = 0x1f83d9ab;
    PER_CORE_APP_META(sc_config).sha_state[7] = 0x5be0cd19;

    return SC_SUCCESS;
}