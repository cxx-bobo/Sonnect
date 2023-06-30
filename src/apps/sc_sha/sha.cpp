#include "sc_global.hpp"
#include "sc_sha/sha.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"

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
int _process_enter_doca(struct sc_config *sc_config){
    int result=SC_SUCCESS, doca_result;
    struct mempool_target *target, *temp_target;

    // create memory pool for sha job
    PER_CORE_APP_META(sc_config).mpool = mempool_create(SHA_MEMPOOL_NB_BUF, SHA_MEMPOOL_BUF_SIZE);
    if(unlikely(PER_CORE_APP_META(sc_config).mpool == NULL)){
        SC_THREAD_ERROR("failed to allocate memory for mempool");
        result = SC_ERROR_MEMORY;
        goto process_enter_doca_exit;
    }
    SC_THREAD_LOG(
        "create memory pool (%p~%p), size: %lu, #targets: %u, size of target: %u",
        PER_CORE_APP_META(sc_config).mpool->addr,
        PER_CORE_APP_META(sc_config).mpool->addr + PER_CORE_APP_META(sc_config).mpool->size,
        PER_CORE_APP_META(sc_config).mpool->size,
        SHA_MEMPOOL_NB_BUF, SHA_MEMPOOL_BUF_SIZE
    );

    // populate the memory area of memory pool to doca_mmap
    doca_result = doca_mmap_populate(
        /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
        /* addr */ PER_CORE_APP_META(sc_config).mpool->addr,
        /* len */ PER_CORE_APP_META(sc_config).mpool->size,
        /* pg_sz*/ sysconf(_SC_PAGESIZE),
        /* free_cb */ sc_doca_util_mmap_populate_free_cb,
        /* opaque */ NULL
    );
    if(doca_result != DOCA_SUCCESS){
        SC_THREAD_ERROR_DETAILS("failed to populate memory pool to doca_mmap: %s", 
            doca_get_error_string((doca_error_t)doca_result));
        result = SC_ERROR_MEMORY;
        goto process_enter_doca_exit;
    }

    // traverse the memory pool, create doca_buf for each memory region
    LIST_FOR_EACH_SAFE(
        target, temp_target, &(PER_CORE_APP_META(sc_config).mpool->target_free_list), struct mempool_target
    ){
        doca_result = doca_buf_inventory_buf_by_addr(
            /* inventory */ PER_CORE_DOCA_META(sc_config).sha_buf_inv,
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap, 
            /* addr */ target->addr, 
            /* len */ SC_SHA_HASH_KEY_LENGTH,
            /* buf */ &(target->req_buf)
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to acquire doca_buf for request buffer: %s",
                doca_get_error_string((doca_error_t)doca_result));
            result = SC_ERROR_MEMORY;
            goto process_enter_doca_exit;
        }

        doca_result = doca_buf_inventory_buf_by_addr(
            /* inventory */ PER_CORE_DOCA_META(sc_config).sha_buf_inv,
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap, 
            /* addr */ target->addr + SC_SHA_HASH_KEY_LENGTH, 
            /* len */ DOCA_SHA256_BYTE_COUNT,
            /* buf */ &(target->resp_buf)
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to acquire doca_buf for response buffer: %s",
                doca_get_error_string((doca_error_t)doca_result));
            result = SC_ERROR_MEMORY;
            goto process_enter_doca_exit;
        }
    }

process_enter_doca_exit:
    return result;
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
int _process_pkt_doca(struct rte_mbuf **pkt, uint64_t nb_recv_pkts, struct sc_config *sc_config, uint16_t queue_id, uint16_t recv_port_id, uint16_t *fwd_port_id, uint64_t *nb_fwd_pkts){
    int doca_result, result = SC_SUCCESS;
    uint64_t i, j, nb_enqueue_pkts, nb_send_pkts, retry;
    struct rte_ether_hdr *_eth_addr;
    struct rte_ipv4_hdr *_ipv4_hdr;
    struct rte_udp_hdr *_udp_hdr;
    struct mempool_target *mpool_target;
    void *doca_buf_data;
    struct doca_event doca_event = {0};

    struct rte_mbuf *finished_pkts[SHA_MEMPOOL_NB_BUF] = {0}, *finished_pkt;
    uint64_t nb_finished_pkts = 0;

    // try best to enqueue jobs
    for(j=0; j<nb_recv_pkts; j++){
        // obtain free mempool target from the pool
        if (is_mempool_empty(PER_CORE_APP_META(sc_config).mpool)) {
            break;
        }
        mempool_get(PER_CORE_APP_META(sc_config).mpool, &mpool_target);
        assert(mpool_target != NULL);

        // extract SHA source
        char tuple_key[SC_SHA_HASH_KEY_LENGTH] = {0};
        _eth_addr 
            = rte_pktmbuf_mtod_offset(pkt[j], struct rte_ether_hdr*, 0);
        _ipv4_hdr 
            = rte_pktmbuf_mtod_offset(pkt[j], struct rte_ipv4_hdr*, RTE_ETHER_HDR_LEN);
        _udp_hdr 
            = rte_pktmbuf_mtod_offset(pkt[j], struct rte_udp_hdr*, RTE_ETHER_HDR_LEN+rte_ipv4_hdr_len(_ipv4_hdr));

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

        // copy SHA source to memory pool target area
        memcpy(mpool_target->addr, tuple_key, SC_SHA_HASH_KEY_LENGTH);
        doca_buf_get_data(mpool_target->req_buf, &doca_buf_data);
	    doca_buf_set_data(mpool_target->req_buf, doca_buf_data, SC_SHA_HASH_KEY_LENGTH);

        // construct sha job
        struct doca_sha_job const sha_job = {
            .base = {
                .type = DOCA_SHA_JOB_SHA256,
                .flags = DOCA_JOB_FLAGS_NONE,
                .ctx = DOCA_CONF(sc_config)->sha_ctx,
                .user_data = {
                    .ptr = mpool_target,
                }
            },
            .req_buf = mpool_target->req_buf,
            .resp_buf = mpool_target->resp_buf,
            .flags = DOCA_SHA_JOB_FLAGS_NONE,
        };

        // submit the job
        doca_result = doca_workq_submit(
            /* workq */ PER_CORE_DOCA_META(sc_config).sha_workq, 
            /* job */ &(sha_job.base)
        );
        if(doca_result == DOCA_ERROR_NO_MEMORY){
            mempool_put(PER_CORE_APP_META(sc_config).mpool, mpool_target);
            break;
        }
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to enqueue SHA job: %s", doca_get_error_string((doca_error_t)doca_result));
            return SC_ERROR_INTERNAL;
        }
    }

    // free all non-enqueued packets
    nb_enqueue_pkts = j;
    for(j=nb_enqueue_pkts; j<nb_recv_pkts; j++){
        rte_pktmbuf_free(pkt[j]);
    }

    // try best to retrieve jobs
    do{
        doca_result = doca_workq_progress_retrieve(
            /* workq */ PER_CORE_DOCA_META(sc_config).sha_workq,
            /* ev */ &doca_event,
            /* flags */ DOCA_WORKQ_RETRIEVE_FLAGS_NONE
        );
        if (doca_result == DOCA_SUCCESS) {
            // obtain memory pool target
            mpool_target = (struct mempool_target*)doca_event.user_data.ptr;

            // obtain finished packet
            finished_pkt = finished_pkts[nb_finished_pkts] = mpool_target->pkt;
            finished_pkt->pkt_len = finished_pkt->data_len = mpool_target->pkt_len;
            finished_pkt->nb_segs = 1;
            finished_pkt->next = NULL;
            nb_finished_pkts += 1;

            // return back memory buffer
            mempool_put(PER_CORE_APP_META(sc_config).mpool, mpool_target);
            if(nb_finished_pkts == SHA_MEMPOOL_NB_BUF){ break; }

        } else if (doca_result == DOCA_ERROR_AGAIN) {
			break;
		} else {
			SC_THREAD_ERROR_DETAILS("failed to retrieve SHA job: %s", doca_get_error_string((doca_error_t)doca_result));
			result = SC_ERROR_INTERNAL;
			break;
		}
    } while(doca_result == DOCA_SUCCESS);

    // send back packets
    for(i=0; i<INTERNAL_CONF(sc_config)->nb_send_ports; i++){
        nb_send_pkts = rte_eth_tx_burst(
            /* port_id */ INTERNAL_CONF(sc_config)->send_port_idx[i],
            /* queue_id */ queue_id,
            /* tx_pkts */ finished_pkts,
            /* nb_pkts */ nb_finished_pkts
        );
        if(unlikely(nb_send_pkts < nb_finished_pkts)){
            retry = 0;
            while (nb_send_pkts < nb_finished_pkts && retry++ < SC_SHA_BURST_TX_RETRIES){
                nb_send_pkts += rte_eth_tx_burst(
                    /* port_id */ INTERNAL_CONF(sc_config)->send_port_idx[i],
                    /* queue_id */ queue_id, 
                    /* tx_pkts */ &finished_pkts[nb_send_pkts], 
                    /* nb_pkts */ nb_finished_pkts - nb_send_pkts
                );
            }
        }
    }

    return result;
}

/*!
 * \brief   callback for processing packets to be dropped
 * \param   sc_config       the global configuration
 * \param   pkt             the packets to be dropped
 * \param   nb_drop_pkts    number of packets to be dropped
 * \return  zero for successfully processing
 */
int _process_pkt_drop_doca(struct sc_config *sc_config, struct rte_mbuf **pkt, uint64_t nb_drop_pkts){
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
    return SC_SUCCESS;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _all_exit(struct sc_config *sc_config){
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
        /* TODO: dispatch processing functions here */
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client;

        #if defined(SC_HAS_DOCA)
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter_doca;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_pkt_func  = _process_pkt_doca;
            PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_pkt_drop_func = _process_pkt_drop_doca;
        #endif

        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit;
        
    }

    return SC_SUCCESS;
}