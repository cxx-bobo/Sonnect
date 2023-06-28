#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   initialize RTE TCP header
 * \param   tcp_hdr				RTE TCP header
 * \param   src_port  			source port
 * \param	dst_port			destination port
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully initialization
 */
int sc_util_initialize_tcp_header(struct rte_tcp_hdr *tcp_hdr, uint16_t src_port,
		uint16_t dst_port, uint16_t pkt_data_len, uint16_t *pkt_len){
	*pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_tcp_hdr));

	memset(tcp_hdr, 0, sizeof(struct rte_tcp_hdr));
	tcp_hdr->src_port = rte_cpu_to_be_16(src_port);
	tcp_hdr->dst_port = rte_cpu_to_be_16(dst_port);
	tcp_hdr->data_off = (sizeof(struct rte_tcp_hdr) << 2) & 0xF0;

	return SC_SUCCESS;
}
