#include "sc_global.h"
#include "sc_port.h"
#include "sc_utils/rss.h"
#include "sc_log.h"
#include "sc_utils.h"

/*!
 * \brief   calculate the queue id based on ipv4 packet
 * \param   src_ipv4            source ipv4 address
 * \param   dst_ipv4            destination ipv4 address
 * \param   sport               source port
 * \param   dport               destination port
 * \param   sctp_tag
 * \param   nb_queues           number of used queues
 * \param   queue_id            the result queue index
 * \param   rss_hash_field      the rss hash fields
 * \return  zero for successfully calculation
 */
int sc_util_get_rss_queue_id_ipv4(
        uint32_t src_ipv4, uint32_t dst_ipv4, 
        uint16_t sport, uint16_t dport, 
        uint32_t sctp_tag,
        uint32_t nb_queues,
        uint32_t *queue_id,
        uint64_t rss_hash_field
){
    uint32_t rss_l3_original, rss_l3l4_original;
    
    sc_util_get_rss_result_ipv4(
        src_ipv4, dst_ipv4, sport, dport, sctp_tag, &rss_l3_original, &rss_l3l4_original);

    /* 
     * refs: 
     *   [1] http://galsagie.github.io/2015/02/26/dpdk-tips-1/
     *   [2] https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-receive-side-scaling
     */
    #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
        if(rss_hash_field == RTE_ETH_RSS_IP){   /* if only IP need to be hashed */
    #else
        if(rss_hash_field == ETH_RSS_IP){          /* if only IP need to be hashed */
    #endif
            *queue_id = (rss_l3_original & 0x7F) % nb_queues;
        } else {
            *queue_id = (rss_l3l4_original & 0x7F) % nb_queues;
        }

    return SC_SUCCESS;
}

/*!
 * \brief   calculate the queue id based on ipv6 packet
 * \param   src_ipv6            pointer to a 16-bytes long array, source ipv6 address
 * \param   dst_ipv6            pointer to a 16-bytes long array, destination ipv6 address
 * \param   sport               source port
 * \param   dport               destination port
 * \param   sctp_tag
 * \param   nb_queues           number of used queues
 * \param   queue_id            the result queue index
 * \param   rss_hash_field      the rss hash fields
 * \return  zero for successfully calculation
 */
int sc_util_get_rss_queue_id_ipv6(
        uint8_t *src_ipv6, uint8_t *dst_ipv6, 
        uint16_t sport, uint16_t dport, 
        uint32_t sctp_tag,
        uint32_t nb_queues,
        uint32_t *queue_id,
        uint64_t rss_hash_field
){
    int result = SC_SUCCESS;
    uint32_t rss_l3_original, rss_l3l4_original;
    
    result = sc_util_get_rss_result_ipv6(
        src_ipv6, dst_ipv6, sport, dport, sctp_tag, &rss_l3_original, &rss_l3l4_original);
    if(result != SC_SUCCESS){
        SC_THREAD_ERROR("failed to calculate the RSS result");
        return result;
    }

    #if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
        if(rss_hash_field == RTE_ETH_RSS_IP){   /* if only IP need to be hashed */
    #else
        if(rss_hash_field == ETH_RSS_IP){          /* if only IP need to be hashed */
    #endif
            *queue_id = (rss_l3_original & 0x7F) % nb_queues;
        } else {
            *queue_id = (rss_l3l4_original & 0x7F) % nb_queues;
        }

    return SC_SUCCESS;
}

/*!
 * \brief   calculate the rss result based on ipv4 packet
 * \param   src_ipv4            source ipv4 address
 * \param   dst_ipv4            destination ipv4 address
 * \param   sport               source port
 * \param   dport               destination port
 * \param   sctp_tag
 * \param   rss_l3_original     pointer to the address that stores the l3 rss result
 * \param   rss_l3l4_original   pointer to the address that stores the l3 and l4 rss result
 * \return  zero for successfully calculation
 */
int sc_util_get_rss_result_ipv4(
        uint32_t src_ipv4, uint32_t dst_ipv4, 
        uint16_t sport, uint16_t dport, 
        uint32_t sctp_tag,
        uint32_t *rss_l3_original,
        uint32_t *rss_l3l4_original
){
    union rte_thash_tuple tuple;

    tuple.v4.src_addr = src_ipv4;
    tuple.v4.dst_addr = dst_ipv4;
    tuple.v4.sport = sport;
    tuple.v4.dport = dport;
    tuple.v4.sctp_tag = sctp_tag;

    /* 
     * calculate hash with original key using toeplitz hash,
     * see refs:
     *   [1] https://doc.dpdk.org/guides/prog_guide/toeplitz_hash_lib.html
     *   [2] https://stackoverflow.com/questions/64925261/
     *       how-to-compute-the-queue-id-if-i-can-compute-the-rss-hash-with-software-implemen
     *   [3] https://docs.nvidia.com/networking/display/MLNXOFEDv5432723/RSS%20Support
     */
    if(rss_l3_original){
        *rss_l3_original = rte_softrss((uint32_t *)&tuple,
            RTE_THASH_V4_L3_LEN, used_rss_hash_key);
    }
    if(rss_l3l4_original){
        *rss_l3l4_original = rte_softrss((uint32_t *)&tuple,
                RTE_THASH_V4_L4_LEN, used_rss_hash_key);
    }

    return SC_SUCCESS;
}

/*!
 * \brief   calculate the rss result based on ipv6 packet
 * \param   src_ipv6            pointer to a 16-bytes long array, source ipv6 address
 * \param   dst_ipv6            pointer to a 16-bytes long array, destination ipv6 address
 * \param   sport               source port
 * \param   dport               destination port
 * \param   sctp_tag
 * \param   rss_l3_original     pointer to the address that stores the l3 rss result
 * \param   rss_l3l4_original   pointer to the address that stores the l3 and l4 rss result
 * \return  zero for successfully calculation
 */
int sc_util_get_rss_result_ipv6(
        uint8_t *src_ipv6, uint8_t *dst_ipv6, 
        uint16_t sport, uint16_t dport, 
        uint32_t sctp_tag,
        uint32_t *rss_l3_original,
        uint32_t *rss_l3l4_original
){
    int i;
    union rte_thash_tuple tuple;
    
    tuple.v6.sport = sport;
    tuple.v6.dport = dport;
    tuple.v6.sctp_tag = sctp_tag;
    for(i=0; i<16; i++){
        if(unlikely(!src_ipv6[i] || !dst_ipv6[i])){
            SC_THREAD_ERROR_DETAILS("invalid addr pointer at %d", i);
			return SC_ERROR_INVALID_VALUE;
        } else {
            tuple.v6.src_addr[i] = src_ipv6[i];
            tuple.v6.dst_addr[i] = dst_ipv6[i];
        }
    }

    if(rss_l3_original){
        *rss_l3_original = rte_softrss((uint32_t *)&tuple,
            RTE_THASH_V4_L3_LEN, used_rss_hash_key);
    }
    if(rss_l3l4_original){
        *rss_l3l4_original = rte_softrss((uint32_t *)&tuple,
                RTE_THASH_V4_L4_LEN, used_rss_hash_key);
    }

    return SC_SUCCESS;
}

