#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   initialize RTE UDP header
 * \param   udp_hdr				RTE UDP header
 * \param   src_port  			source port
 * \param	dst_port			destination port
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully initialization
 */
int sc_util_initialize_udp_header(struct rte_udp_hdr *udp_hdr, uint16_t src_port,
		uint16_t dst_port, uint16_t pkt_data_len, uint16_t *pkt_len){

	*pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_udp_hdr));
	
	udp_hdr->src_port = rte_cpu_to_be_16(src_port);
	udp_hdr->dst_port = rte_cpu_to_be_16(dst_port);
	udp_hdr->dgram_len = rte_cpu_to_be_16(*pkt_len);
	udp_hdr->dgram_cksum = 0; /* No UDP checksum. */

	return SC_SUCCESS;
}