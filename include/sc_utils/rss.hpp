#ifndef _SC_UTILS_RSS_H_
#define _SC_UTILS_RSS_H_

#include <rte_thash.h>

int sc_util_get_rss_queue_id_ipv4(
    uint32_t src_ipv4, uint32_t dst_ipv4, uint16_t sport, uint16_t dport, 
    uint32_t sctp_tag, uint32_t nb_queues, uint32_t *queue_id, uint64_t rss_hash_field);

int sc_util_get_rss_queue_id_ipv6(
    uint8_t *src_ipv6, uint8_t *dst_ipv6, uint16_t sport, uint16_t dport, 
    uint32_t sctp_tag, uint32_t nb_queues, uint32_t *queue_id, uint64_t rss_hash_field);

int sc_util_get_rss_result_ipv4(uint32_t src_ipv4, uint32_t dst_ipv4, uint16_t sport, uint16_t dport, 
    uint32_t sctp_tag, uint32_t *rss_l3_original, uint32_t *rss_l3l4_original);

int sc_util_get_rss_result_ipv6(uint8_t *src_ipv6, uint8_t *dst_ipv6, uint16_t sport, uint16_t dport, 
        uint32_t sctp_tag, uint32_t *rss_l3_original, uint32_t *rss_l3l4_original);

#endif