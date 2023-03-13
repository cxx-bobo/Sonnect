#include "sc_global.h"
#include "sc_port.h"
#include "sc_utils/rss.h"
#include "sc_log.h"
#include "sc_utils.h"

int sc_util_ipv4_get_rss_result(uint32_t src_ipv4, uint32_t dst_ipv4, uint16_t sport, uint16_t dport, uint32_t sctp_tag){
    union rte_thash_tuple tuple;

    tuple.v4.src_addr = src_ipv4;
    tuple.v4.dst_addr = dst_ipv4;
    tuple.v4.sport = sport;
    tuple.v4.dport = dport;
    tuple.v4.sctp_tag = sctp_tag;

    /* calculate hash with original key */
    rss_l3_original = rte_softrss((uint32_t *)&tuple,
            RTE_THASH_V4_L3_LEN, default_rss_key);
    
    rss_l3l4_original = rte_softrss((uint32_t *)&tuple,
            RTE_THASH_V4_L4_LEN, default_rss_key);

    return SC_SUCCESS;
}

