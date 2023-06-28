#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   generate random ipv6 address
 * \param   addr   	pointer to a 16-bytes long array
 * \return  0 for successfully generation
 */
int sc_util_generate_random_ipv6_addr(uint8_t *addr){
	int i;

	for(i=0; i<16; i++){
		if(unlikely(!addr[i])){
			SC_THREAD_ERROR_DETAILS("invalid addr pointer at %d", i);
			return SC_ERROR_INVALID_VALUE;
		} else {
			addr[i] = sc_util_random_unsigned_int8();
		}
	}

	return SC_SUCCESS;
}

/*!
 * \brief   initialize RTE IPv6 header
 * \param   ip_hdr				RTE IPv6 header
 * \param	src_addr			source IPv6 address
 * \param	dst_addr			destination IPv6 address
 * \param	proto				L4 protocol type
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully initialization
 */
int sc_util_initialize_ipv6_header_proto(struct rte_ipv6_hdr *ip_hdr, uint8_t *src_addr,
		uint8_t *dst_addr, uint16_t pkt_data_len, uint8_t proto, uint16_t *pkt_len){
	*pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_ipv6_hdr));

	ip_hdr->vtc_flow = rte_cpu_to_be_32(0x60000000); /* Set version to 6. */
	ip_hdr->payload_len = rte_cpu_to_be_16(pkt_data_len);
	ip_hdr->proto = proto;
	ip_hdr->hop_limits = IP_DEFTTL;

	rte_memcpy(ip_hdr->src_addr, src_addr, sizeof(ip_hdr->src_addr));
	rte_memcpy(ip_hdr->dst_addr, dst_addr, sizeof(ip_hdr->dst_addr));

	return SC_SUCCESS;
}