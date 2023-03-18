#include "sc_global.h"
#include "sc_sha/sha.h"
#include "sc_utils.h"
#include "sc_log.h"

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    /* initialize sha state (for calculation on ARM/x86 cores) */
    INTERNAL_CONF(sc_config)->sha_state[0] = 0x6a09e667;
    INTERNAL_CONF(sc_config)->sha_state[1] = 0xbb67ae85;
    INTERNAL_CONF(sc_config)->sha_state[2] = 0x3c6ef372;
    INTERNAL_CONF(sc_config)->sha_state[3] = 0xa54ff53a;
    INTERNAL_CONF(sc_config)->sha_state[4] = 0x510e527f;
    INTERNAL_CONF(sc_config)->sha_state[5] = 0x9b05688c;
    INTERNAL_CONF(sc_config)->sha_state[6] = 0x1f83d9ab;
    INTERNAL_CONF(sc_config)->sha_state[7] = 0x5be0cd19;

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
    int result = SC_ERROR_NOT_IMPLEMENTED;
    SC_WARNING_DETAILS("_parse_app_kv_pair not implemented");
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
        src_buffer = malloc(SC_SHA_HASH_KEY_LENGTH);
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
                doca_get_error_string(doca_result));
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
                doca_get_error_string(doca_result));
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
                doca_get_error_string(doca_result));
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

    return SC_SUCCESS;
}

/*!
 * \brief   callback for processing packet
 * \param   pkt         the received packet
 * \param   sc_config   the global configuration
 * \param   fwd_port_id     specified the forward port index if need to forward packet
 * \param   need_forward    indicate whether need to forward packet, default to be false
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t *fwd_port_id, bool *need_forward){
    int i, doca_result;
    char tuple_key[SC_SHA_HASH_KEY_LENGTH];
    struct rte_ether_hdr *_eth_addr;
    struct rte_ipv4_hdr *_ipv4_hdr;
    struct rte_udp_hdr *_udp_hdr;

    /* extract value key */
    _eth_addr = rte_pktmbuf_mtod_offset(pkt, struct rte_ether_hdr*, 0);
    _ipv4_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr*, RTE_ETHER_HDR_LEN);
    _udp_hdr = rte_pktmbuf_mtod_offset(
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

    /* calculate hash value */
    #if defined(SC_HAS_DOCA) && defined(SC_NEED_DOCA_SHA)
        struct doca_event doca_event = {0};
        uint8_t *resp_head;

        /* copy the key to SHA source data buffer */
        memcpy(PER_CORE_DOCA_META(sc_config).sha_src_buffer, tuple_key, SC_SHA_HASH_KEY_LENGTH);

        /* lock the belonging of SHA engine */
        rte_spinlock_lock(&DOCA_CONF(sc_config)->sha_lock);

        /* enqueue the result */
        doca_result = doca_workq_submit(
            /* workq */ PER_CORE_DOCA_META(sc_config).sha_workq, 
            /* job */ &PER_CORE_DOCA_META(sc_config).sha_job->base
        );
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to enqueue SHA job: %s", doca_get_error_string(doca_result));
            return SC_ERROR_INTERNAL;
        }

        /* wait for job completion */
        while((doca_result = doca_workq_progress_retrieve(
                /* workq */ PER_CORE_DOCA_META(sc_config).sha_workq,
                /* ev */ &doca_event,
                /* flags */ NULL) == DOCA_ERROR_AGAIN)
        ){}
        if(doca_result != DOCA_SUCCESS){
            SC_THREAD_ERROR_DETAILS("failed to retrieve SHA result: %s", doca_get_error_string(doca_result));
            return SC_ERROR_INTERNAL;
        }

        /* release the belonging of SHA engine */
        rte_spinlock_unlock(&DOCA_CONF(sc_config)->sha_lock);

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

        /* retrieve SHA result, we only use the LSB 32 bits */
        doca_buf_get_data(PER_CORE_DOCA_META(sc_config).sha_job->resp_buf, (void **)&resp_head);

    #elif defined(SC_HAS_DOCA) && !defined(SC_NEED_DOCA_SHA)
        sha256_process_arm(
            INTERNAL_CONF(sc_config)->sha_state, (uint8_t*)tuple_key, SC_SHA_HASH_KEY_LENGTH);
    #else
        sha256_process_x86(
            INTERNAL_CONF(sc_config)->sha_state, (uint8_t*)tuple_key, SC_SHA_HASH_KEY_LENGTH);
    #endif

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
    SC_WARNING_DETAILS("_process_exit not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _all_exit(struct sc_config *sc_config){
    return SC_ERROR_NOT_IMPLEMENTED;
}