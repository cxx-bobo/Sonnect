#include "sc_global.hpp"
#include "sc_sketch/sketch.h"
#include "sc_utils.hpp"
#include "sc_control_plane.hpp"

#include "sc_utils/map.hpp"

#if defined(SC_HAS_DOCA)
    DOCA_LOG_REGISTER(SC::APP_SKETCH);
#endif

#if defined(SKETCH_TYPE_CM)
    #include "sc_sketch/cm_sketch.h"
#endif

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i;
    
    /* allocate memory space for sketch */
    /* cm_sketch */
    #if defined(SKETCH_TYPE_CM)
        /* allocate cm_sketch */
        struct cm_sketch *cm_sketch = (struct cm_sketch*)rte_malloc(NULL, sizeof(struct cm_sketch), 0);
        if(unlikely(!cm_sketch)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for cm_sketch");
            return SC_ERROR_MEMORY;
        }
        memset(cm_sketch, 0, sizeof(struct cm_sketch));
        INTERNAL_CONF(sc_config)->cm_sketch = cm_sketch;

        /* allocate counters */
        uint64_t counter_size 
            = sizeof(counter_t) * INTERNAL_CONF(sc_config)->cm_nb_rows * INTERNAL_CONF(sc_config)->cm_nb_counters_per_row;
        counter_t *counters = (counter_t*)rte_malloc(NULL, counter_size, 0);
        if(unlikely(!counters)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for counters");
            return SC_ERROR_MEMORY;
        }
        memset(counters, 0, counter_size);
        INTERNAL_CONF(sc_config)->cm_sketch->counters = counters;

        /* initialize hash function seeds */
        uint32_t *hash_seeds = (uint32_t*)rte_malloc(NULL, sizeof(uint32_t)*INTERNAL_CONF(sc_config)->cm_nb_rows, 0);
        if(unlikely(!hash_seeds)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for hash_seeds");
            return SC_ERROR_MEMORY;
        }
        memset(hash_seeds, 0, sizeof(uint32_t)*INTERNAL_CONF(sc_config)->cm_nb_rows);
        for(i=0; i<INTERNAL_CONF(sc_config)->cm_nb_rows; i++){
            hash_seeds[i] = sc_util_random_unsigned_int32();
        }
        INTERNAL_CONF(sc_config)->cm_sketch->hash_seeds = hash_seeds;

        /* initialize rte_lock */
        rte_spinlock_init(&(INTERNAL_CONF(sc_config)->cm_sketch->lock));

        /* specify sketch processing function for sketch_core */
        INTERNAL_CONF(sc_config)->sketch_core.query = __cm_query;
        INTERNAL_CONF(sc_config)->sketch_core.update = __cm_update;
        INTERNAL_CONF(sc_config)->sketch_core.clean = __cm_clean;
        INTERNAL_CONF(sc_config)->sketch_core.record = __cm_record;
        INTERNAL_CONF(sc_config)->sketch_core.evaluate = __cm_evaluate;
    #endif // SKETCH_TYPE_CM

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
    
    /* cm_sketch */
    #if defined(SKETCH_TYPE_CM)
        /* number of rows */
        if(!strcmp(key, "cm_nb_rows")){
            value = sc_util_del_both_trim(value);
            sc_util_del_change_line(value);
            uint32_t cm_nb_rows;
            if(sc_util_atoui_32(value, &cm_nb_rows) != SC_SUCCESS) {
                result = SC_ERROR_INVALID_VALUE;
                goto invalid_cm_nb_rows;
            }
            INTERNAL_CONF(sc_config)->cm_nb_rows = cm_nb_rows;
            goto exit;

    invalid_cm_nb_rows:
            SC_ERROR_DETAILS("invalid configuration cm_nb_rows\n");
        }

        /* number of counters per row */
        if(!strcmp(key, "cm_nb_counters_per_row")){
            value = sc_util_del_both_trim(value);
            sc_util_del_change_line(value);
            uint32_t cm_nb_counters_per_row;
            if(sc_util_atoui_32(value, &cm_nb_counters_per_row) != SC_SUCCESS) {
                result = SC_ERROR_INVALID_VALUE;
                goto invalid_cm_nb_counters_per_row;
            }
            INTERNAL_CONF(sc_config)->cm_nb_counters_per_row = cm_nb_counters_per_row;
            goto exit;

    invalid_cm_nb_counters_per_row:
            SC_ERROR_DETAILS("invalid configuration cm_nb_counters_per_row\n")
        }
    #endif // SKETCH_TYPE_CM

exit:
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter(struct sc_config *sc_config){
    /* configure to use DOCA SHA to accelerate the hash calculation */
    #if defined(SC_HAS_DOCA) && defined(SC_NEED_DOCA_SHA)
        int doca_result;

        char *dst_buffer = NULL;
        char *src_buffer = NULL;
        struct doca_sha_job *sha_job;

        /* allocate SHA engine result buffer */
        dst_buffer = malloc(DOCA_SHA256_BYTE_COUNT);
        if (dst_buffer == NULL) {
            SC_THREAD_ERROR_DETAILS("failed to malloc memory for dst_buffer");
            return SC_ERROR_MEMORY;
        }
        memset(dst_buffer, 0, DOCA_SHA256_BYTE_COUNT);
        PER_CORE_DOCA_META(sc_config).sha_dst_buffer = dst_buffer;

        /* allocate buffer for storing input source data */
        src_buffer = malloc(SC_SKETCH_HASH_KEY_LENGTH);
        if (src_buffer == NULL) {
            SC_THREAD_ERROR_DETAILS("failed to malloc memory for src_buffer");
            return SC_ERROR_MEMORY;
        }
        memset(src_buffer, 0, SC_SKETCH_HASH_KEY_LENGTH);
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
                doca_get_error_string(doca_result));
            return SC_ERROR_MEMORY;
        }

        /* populate src_buffer to memory map */
        doca_result = doca_mmap_populate(
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
            /* addr */ PER_CORE_DOCA_META(sc_config).sha_src_buffer,
            /* len */ SC_SKETCH_HASH_KEY_LENGTH,
            /* pg_sz */ sysconf(_SC_PAGESIZE),
            /* free_cb */ NULL,
            /* opaque */ NULL
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to populate memory src_buffer to memory map: %s",
                doca_get_error_string(doca_result));
            return SC_ERROR_MEMORY;
        }

        /* acquire DOCA buffer for representing source buffer */
        doca_result = doca_buf_inventory_buf_by_addr(
            /* inventory */ PER_CORE_DOCA_META(sc_config).sha_buf_inv,
            /* mmap */ PER_CORE_DOCA_META(sc_config).sha_mmap,
            /* addr */ PER_CORE_DOCA_META(sc_config).sha_src_buffer,
            /* len */ SC_SKETCH_HASH_KEY_LENGTH,
            /* buf */ &(PER_CORE_DOCA_META(sc_config).sha_src_doca_buf)
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to acquire DOCA buffer representing source buffer: %s",
                doca_get_error_string(doca_result));
            return SC_ERROR_MEMORY;
        }

        /* set data address and length in the doca_buf. */
        doca_result = doca_buf_set_data(
            /* buf */ PER_CORE_DOCA_META(sc_config).sha_src_doca_buf,
            /* data */ PER_CORE_DOCA_META(sc_config).sha_src_buffer,
            /* data_len */ SC_SKETCH_HASH_KEY_LENGTH
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to set data address and length in the doca_buf: %s",
                doca_get_error_string(doca_result));
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
                doca_get_error_string(doca_result));
            return SC_ERROR_MEMORY;
        }

        /* construct sha job */
        sha_job = (struct doca_sha_job*)malloc(sizeof(struct doca_sha_job));
        if(!sha_job){
            SC_THREAD_ERROR_DETAILS("failed to allocate memory for sha_job");
            return SC_ERROR_MEMORY;
        }
        sha_job->base = (struct doca_job) {
			.type = DOCA_SHA_JOB_SHA256,
			.flags = DOCA_JOB_FLAGS_NONE,
			.ctx = PER_CORE_DOCA_META(sc_config).sha_ctx,
			.user_data.u64 = DOCA_SHA_JOB_SHA256,
		},
        sha_job->resp_buf = PER_CORE_DOCA_META(sc_config).sha_dst_doca_buf;
        sha_job->req_buf = PER_CORE_DOCA_META(sc_config).sha_src_doca_buf;
        sha_job->flags = DOCA_SHA_JOB_FLAGS_NONE;
        PER_CORE_DOCA_META(sc_config).sha_job = sha_job;
    #endif

    #if defined(MODE_ACCURACY)
        /* allocate per-core key-value map for accuracy measurement */
        sc_kv_map_t *kv_map;
        if(new_kv_map(&kv_map) != SC_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to allocate memory for sc_kv_map_t");
            return SC_ERROR_MEMORY;
        }
        PER_CORE_APP_META(sc_config).kv_map = kv_map;
    #endif // MODE_ACCURACY

    #if defined(MODE_LATENCY) || defined(MODE_THROUGHPUT)
        /* record the start time of this thread */
        gettimeofday(&PER_CORE_APP_META(sc_config).thread_start_time, NULL);
    #endif // MODE_ACCURACY || MODE_THROUGHPUT

    goto process_enter_exit;
process_enter_warning:
    SC_WARNING("error occured while executing entering callback");
process_enter_exit:
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
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t recv_port_id, uint16_t *fwd_port_id, bool *need_forward){
    #if defined(MODE_LATENCY)
        struct timeval pkt_process_start, pkt_process_end;
        gettimeofday(&pkt_process_start, NULL);
    #endif // MODE_LATENCY

    /* allocate memory for storing tuple key */
    char tuple_key[SC_SKETCH_HASH_KEY_LENGTH];
    
    /* initialize tuple key */
    struct rte_ether_hdr *_eth_addr 
        = rte_pktmbuf_mtod_offset(pkt, struct rte_ether_hdr*, 0);
    struct rte_ipv4_hdr *_ipv4_hdr 
        = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr*, RTE_ETHER_HDR_LEN);
    struct rte_udp_hdr *_udp_hdr
        = rte_pktmbuf_mtod_offset(
            pkt, struct rte_udp_hdr*, RTE_ETHER_HDR_LEN+rte_ipv4_hdr_len(_ipv4_hdr));
    
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

    // SC_LOG("tuple key: %s, size: %ld", tuple_key, sizeof(tuple_key));

    // printf("recv ether frame\n");
    // printf("- source MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
    //         " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
    //     _eth_addr->src_addr.addr_bytes[0], _eth_addr->src_addr.addr_bytes[1],
    //     _eth_addr->src_addr.addr_bytes[2], _eth_addr->src_addr.addr_bytes[3],
    //     _eth_addr->src_addr.addr_bytes[4], _eth_addr->src_addr.addr_bytes[5]);
    // printf("- dest MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
    //         " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
    //     _eth_addr->dst_addr.addr_bytes[0], _eth_addr->dst_addr.addr_bytes[1],
    //     _eth_addr->dst_addr.addr_bytes[2], _eth_addr->dst_addr.addr_bytes[3],
    //     _eth_addr->dst_addr.addr_bytes[4], _eth_addr->dst_addr.addr_bytes[5]);
    // fflush(stdout);

    /* update sketch */
    if(SC_SUCCESS != INTERNAL_CONF(sc_config)->sketch_core.update(tuple_key, sc_config)){
        goto process_pkt_warning;
    }

    /* record number of processing packets and bytes */
    #if defined(MODE_THROUGHPUT) || defined(MODE_LATENCY)
        PER_CORE_APP_META(sc_config).nb_pkts += 1;
        PER_CORE_APP_META(sc_config).nb_bytes += pkt->data_len;
    #endif // MODE_THROUGHPUT || MODE_LATENCY

    /* record the received flow */
    #if defined(MODE_ACCURACY)
        if(SC_SUCCESS != INTERNAL_CONF(sc_config)->sketch_core.record(tuple_key, sc_config)){
            SC_WARNING("error occured duing recording");
        }
    #endif // MODE_ACCURACY

    #if defined(MODE_LATENCY)
        gettimeofday(&pkt_process_end, NULL);
        PER_CORE_APP_META(sc_config).overall_pkt_process.tv_usec 
            += (pkt_process_end.tv_sec - pkt_process_start.tv_sec) * 1000000 
                + (pkt_process_end.tv_usec - pkt_process_start.tv_usec);
    #endif // MODE_LATENCY

    goto process_pkt_exit;

process_pkt_warning:
    SC_WARNING("error occured while executing packet processing callback");

process_pkt_exit:
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
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback while exiting application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_exit(struct sc_config *sc_config){
    #if defined(MODE_LATENCY) || defined(MODE_THROUGHPUT)
        /* record the start time of this thread */
        gettimeofday(&PER_CORE_APP_META(sc_config).thread_end_time, NULL);
    #endif // MODE_LATENCY || MODE_THROUGHPUT

    /* start evaluation process */
    if(SC_SUCCESS != INTERNAL_CONF(sc_config)->sketch_core.evaluate(sc_config)){
        goto process_exit_warning;
    }

    goto process_exit_exit;

process_exit_warning:
    SC_WARNING("error occured while executing exiting callback");

process_exit_exit:
    return SC_SUCCESS;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _worker_all_exit(struct sc_config *sc_config){
    return SC_ERROR_NOT_IMPLEMENTED;
}