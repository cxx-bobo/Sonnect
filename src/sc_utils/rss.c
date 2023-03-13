#include "sc_global.h"
#include "sc_port.h"
#include "sc_utils/rss.h"
#include "sc_log.h"
#include "sc_utils.h"

int sc_util_get_rss_queue_id_ipv4(
        uint32_t src_ipv4, uint32_t dst_ipv4, 
        uint16_t sport, uint16_t dport, 
        uint32_t sctp_tag,
        uint32_t nb_queues,
        uint32_t *queue_id
){
    uint32_t rss_l3_original;
    sc_util_get_rss_result_ipv4(
        src_ipv4, dst_ipv4, sport, dport, sctp_tag, &rss_l3_original, NULL);

    /* ref: http://galsagie.github.io/2015/02/26/dpdk-tips-1/ */
    *queue_id = (rss_l3_original & 0xFF) % nb_queues;

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
     * see https://doc.dpdk.org/guides/prog_guide/toeplitz_hash_lib.html for more details
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

