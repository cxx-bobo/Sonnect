#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_control_plane.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   initialize RTE SCTP header
 * \param   sctp_hdr			RTE SCTP header
 * \param   src_port  			source port
 * \param	dst_port			destination port
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully initialization
 */
int sc_util_initialize_sctp_header(struct rte_sctp_hdr *sctp_hdr, uint16_t src_port,
		uint16_t dst_port, uint16_t pkt_data_len, uint16_t *pkt_len){
	*pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_udp_hdr));

	sctp_hdr->src_port = rte_cpu_to_be_16(src_port);
	sctp_hdr->dst_port = rte_cpu_to_be_16(dst_port);
	sctp_hdr->tag = 0;
	sctp_hdr->cksum = 0; /* No SCTP checksum. */

	return SC_SUCCESS;
}
