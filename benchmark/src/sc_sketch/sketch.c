#include "sc_global.h"
#include "sc_sketch/sketch.h"
#include "sc_utils.h"
#include "sc_log.h"

#include "sc_sketch/cm_sketch.h"

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i;

    /* allocate per-core metadata */
    struct _per_core_meta *per_core_meta 
        = (struct _per_core_meta*)rte_malloc(NULL, sizeof(struct _per_core_meta)*sc_config->nb_used_cores, 0);
    if(unlikely(!per_core_meta)){
        SC_ERROR_DETAILS("failed to rte_malloc memory for per_core_meta");
        return SC_ERROR_MEMORY;
    }
    memset(per_core_meta, sizeof(struct _per_core_meta)*sc_config->nb_used_cores, 0);
    sc_config->per_core_meta = per_core_meta;

    /* allocate memory space for sketch */
    if(INTERNAL_CONF(sc_config)->sketch_type == SC_SKETCH_TYPE_CM){ /* cm_sketch */
        /* allocate cm_sketch */
        struct cm_sketch *cm_sketch = (struct cm_sketch*)rte_malloc(NULL, sizeof(struct cm_sketch), 0);
        if(unlikely(!cm_sketch)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for cm_sketch");
            return SC_ERROR_MEMORY;
        }
        memset(cm_sketch, sizeof(struct cm_sketch), 0);
        INTERNAL_CONF(sc_config)->cm_sketch = cm_sketch;

        /* allocate counters */
        uint64_t counter_size 
            = sizeof(counter_t) * INTERNAL_CONF(sc_config)->cm_nb_rows * INTERNAL_CONF(sc_config)->cm_nb_counters_per_row;
        counter_t *counters = (counter_t*)rte_malloc(NULL, counter_size, 0);
        if(unlikely(!counters)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for counters");
            return SC_ERROR_MEMORY;
        }
        memset(counters, counter_size, 0);
        INTERNAL_CONF(sc_config)->cm_sketch->counters = counters;

        /* initialize hash function seeds */
        uint32_t *hash_seeds = (uint32_t*)rte_malloc(NULL, sizeof(uint32_t)*INTERNAL_CONF(sc_config)->cm_nb_rows, 0);
        if(unlikely(!hash_seeds)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for hash_seeds");
            return SC_ERROR_MEMORY;
        }
        memset(hash_seeds, sizeof(uint32_t)*INTERNAL_CONF(sc_config)->cm_nb_rows, 0);
        for(i=0; i<INTERNAL_CONF(sc_config)->cm_nb_rows; i++){
            hash_seeds[i] = random_unsigned_int32();
        }
        INTERNAL_CONF(sc_config)->cm_sketch->hash_seeds = hash_seeds;

        /* initialize rte_lock */
        rte_spinlock_init(&(INTERNAL_CONF(sc_config)->cm_sketch->lock));
    }

    /* specify sketch processing function for sketch_core */
    if(INTERNAL_CONF(sc_config)->sketch_type == SC_SKETCH_TYPE_CM){
        INTERNAL_CONF(sc_config)->sketch_core.query = __cm_query;
        INTERNAL_CONF(sc_config)->sketch_core.update = __cm_update;
        INTERNAL_CONF(sc_config)->sketch_core.clean = __cm_clean;
        INTERNAL_CONF(sc_config)->sketch_core.record = __cm_record;
        INTERNAL_CONF(sc_config)->sketch_core.evaluate = __cm_evaluate;
    }

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
    
    /* config: type of used sketch */
    if(!strcmp(key, "type")){
        value = del_both_trim(value);
        del_change_line(value);
        if(!strcmp(value, "cm")){
            INTERNAL_CONF(sc_config)->sketch_type = SC_SKETCH_TYPE_CM;
        } else {
            SC_ERROR_DETAILS("unknown sketch type %s", value);
            result = SC_ERROR_INPUT;
            goto exit;
        }
    }

    /* cm sketch: number of rows */
    if(!strcmp(key, "cm_nb_rows")){
        value = del_both_trim(value);
        del_change_line(value);
        uint32_t cm_nb_rows;
        if(atoui_32(value, &cm_nb_rows) != SC_SUCCESS) {
            result = SC_ERROR_INPUT;
            goto invalid_cm_nb_rows;
        }
        INTERNAL_CONF(sc_config)->cm_nb_rows = cm_nb_rows;
        goto exit;

invalid_cm_nb_rows:
        SC_ERROR_DETAILS("invalid configuration cm_nb_rows\n");
    }

    /* cm sketch: number of counters per row */
    if(!strcmp(key, "cm_nb_counters_per_row")){
        value = del_both_trim(value);
        del_change_line(value);
        uint32_t cm_nb_counters_per_row;
        if(atoui_32(value, &cm_nb_counters_per_row) != SC_SUCCESS) {
            result = SC_ERROR_INPUT;
            goto invalid_cm_nb_counters_per_row;
        }
        INTERNAL_CONF(sc_config)->cm_nb_counters_per_row = cm_nb_counters_per_row;
        goto exit;

invalid_cm_nb_counters_per_row:
        SC_ERROR_DETAILS("invalid configuration cm_nb_counters_per_row\n")
    }

exit:
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter(struct sc_config *sc_config){
process_enter_warning:
    SC_WARNING("error occured while executing entering callback");
process_enter_exit:
    return SC_SUCCESS;
}

/*!
 * \brief   callback for processing packet
 * \param   pkt         the received packet
 * \param   sc_config   the global configuration
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config){
#if defined(MODE_LATENCY)
    struct timeval pkt_process_start, pkt_process_end;
    gettimeofday(&pkt_process_start, NULL);
#endif

    /* allocate memory for storing tuple key */
    char tuple_key[TUPLE_KEY_LENGTH];
    
    /* initialize tuple key */
    struct rte_ether_hdr *_eth_addr 
        = rte_pktmbuf_mtod_offset(pkt, struct rte_ether_hdr*, 0);
    struct rte_ipv4_hdr *_ipv4_hdr 
        = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr*, RTE_ETHER_HDR_LEN);
    struct rte_udp_hdr *_udp_hdr
        = rte_pktmbuf_mtod_offset(
            pkt, struct rte_udp_hdr*, RTE_ETHER_HDR_LEN+rte_ipv4_hdr_len(_ipv4_hdr));
    sprintf(tuple_key, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%u%u%u%u",
        /* source mac address */
        _eth_addr->src_addr.addr_bytes[0],
        _eth_addr->src_addr.addr_bytes[1],
        _eth_addr->src_addr.addr_bytes[2],
        _eth_addr->src_addr.addr_bytes[3],
        _eth_addr->src_addr.addr_bytes[4],
        _eth_addr->src_addr.addr_bytes[5],
        /* destination mac address */
        _eth_addr->dst_addr.addr_bytes[0],
        _eth_addr->dst_addr.addr_bytes[1],
        _eth_addr->dst_addr.addr_bytes[2],
        _eth_addr->dst_addr.addr_bytes[3],
        _eth_addr->dst_addr.addr_bytes[4],
        _eth_addr->dst_addr.addr_bytes[5],
        /* source ipv4 address */
        _ipv4_hdr->src_addr,
        /* destination ipv4 address */
        _ipv4_hdr->dst_addr,
        /* source udp port */
        _udp_hdr->src_port,
        /* destination udp port */
        _udp_hdr->dst_port
    );

    SC_LOG("tuple key: %s, size: %ld", tuple_key, sizeof(tuple_key));

    printf("recv ether frame\n");
    printf("- source MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
            " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
        _eth_addr->src_addr.addr_bytes[0], _eth_addr->src_addr.addr_bytes[1],
        _eth_addr->src_addr.addr_bytes[2], _eth_addr->src_addr.addr_bytes[3],
        _eth_addr->src_addr.addr_bytes[4], _eth_addr->src_addr.addr_bytes[5]);
    printf("- dest MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
            " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
        _eth_addr->dst_addr.addr_bytes[0], _eth_addr->dst_addr.addr_bytes[1],
        _eth_addr->dst_addr.addr_bytes[2], _eth_addr->dst_addr.addr_bytes[3],
        _eth_addr->dst_addr.addr_bytes[4], _eth_addr->dst_addr.addr_bytes[5]);
    fflush(stdout);

    /* update sketch */
    if(SC_SUCCESS != INTERNAL_CONF(sc_config)->sketch_core.update(tuple_key, sc_config)){
        goto process_pkt_warning;
    }

    /* record number of processing packets and bytes */
#if defined(MODE_THROUGHPUT) || defined(MODE_LATENCY)
    PER_CORE_META(sc_config).nb_pkts += 1;
    PER_CORE_META(sc_config).nb_bytes += pkt->data_len;
#endif

    /* record the received flow */
#if defined(MODE_LATENCY)
    if(SC_SUCCESS != INTERNAL_CONF(sc_config)->sketch_core.record(tuple_key, sc_config)){
        SC_WARNING("error occured duing recording");
    }
#endif

#if defined(MODE_LATENCY)
    gettimeofday(&pkt_process_end, NULL);
    PER_CORE_META(sc_config).overall_pkt_process.tv_usec 
        += (pkt_process_end.tv_sec - pkt_process_start.tv_sec) * 1000000 
            + (pkt_process_end.tv_usec - pkt_process_start.tv_usec);
#endif

    goto process_pkt_exit;

process_pkt_warning:
    SC_WARNING("error occured while executing packet processing callback");

process_pkt_exit:
    return SC_SUCCESS;
}

/*!
 * \brief   callback while exiting application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_exit(struct sc_config *sc_config){
    if(SC_SUCCESS != INTERNAL_CONF(sc_config)->sketch_core.evaluate(sc_config)){
        goto process_exit_warning;
    }

    goto process_exit_exit;

process_exit_warning:
    SC_WARNING("error occured while executing exiting callback");

process_exit_exit:
    return SC_SUCCESS;
}