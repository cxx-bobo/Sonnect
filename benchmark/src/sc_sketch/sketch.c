#include "sc_global.h"
#include "sc_sketch/sketch.h"
#include "sc_utils.h"
#include "sc_log.h"

#include "sc_sketch/cm_sketch.h"

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
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i;

    /* allocate memory space for sketch */
    if(INTERNAL_CONF(sc_config)->sketch_type == SC_SKETCH_TYPE_CM){ /* cm_sketch */
        /* allocate cm_sketch */
        struct cm_sketch *cm_sketch = (struct cm_sketch*)rte_malloc(NULL, sizeof(struct cm_sketch), 0);
        if(unlikely(!cm_sketch)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for cm_sketch");
            return SC_ERROR_MEMORY;
        }
        INTERNAL_CONF(sc_config)->cm_sketch = cm_sketch;

        /* allocate counters */
        counter_t *counters = (counter_t*)rte_malloc(NULL, 
            sizeof(counter_t) * INTERNAL_CONF(sc_config)->cm_nb_rows * INTERNAL_CONF(sc_config)->cm_nb_counters_per_row, 0
        );
        if(unlikely(!counters)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for counters");
            return SC_ERROR_MEMORY;
        }
        INTERNAL_CONF(sc_config)->cm_sketch->counters = counters;

        /* initialize hash function seeds */
        uint32_t *hash_seeds = (uint32_t*)rte_malloc(NULL, sizeof(uint32_t)*INTERNAL_CONF(sc_config)->cm_nb_rows, 0);
        if(unlikely(!hash_seeds)){
            SC_ERROR_DETAILS("failed to rte_malloc memory for hash_seeds");
            return SC_ERROR_MEMORY;
        }
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
    }

    return SC_SUCCESS;
}

/*!
 * \brief   callback for processing packet
 * \param   pkt         the received packet
 * \param   sc_config   the global configuration
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config){
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

    SC_LOG("tuple key: %s, size: %d", tuple_key, sizeof(tuple_key));

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
        SC_WARNING("error occured duing updating sketch");
    }

process_pkt_exit:
    return SC_SUCCESS;
}