#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   initialize RTE ARP header
 * \param   arp_hdr				RTE ARP header
 * \param   src_mac  			source MAC address
 * \param	dst_mac				destination MAC address
 * \param	src_ip 				source IP address
 * \param 	dst_ip 				destination IP address
 * \param	opcode 				ARP operation code
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully initialization
 */
int sc_util_initialize_arp_header(struct rte_arp_hdr *arp_hdr,
		struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac,
		uint32_t src_ip, uint32_t dst_ip, uint32_t opcode, 
		uint16_t pkt_data_len, uint16_t *pkt_len){
	*pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_arp_hdr));

	arp_hdr->arp_hardware = rte_cpu_to_be_16(RTE_ARP_HRD_ETHER);
	arp_hdr->arp_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
	arp_hdr->arp_hlen = RTE_ETHER_ADDR_LEN;
	arp_hdr->arp_plen = sizeof(uint32_t);
	arp_hdr->arp_opcode = rte_cpu_to_be_16(opcode);
	rte_ether_addr_copy(src_mac, &arp_hdr->arp_data.arp_sha);
	arp_hdr->arp_data.arp_sip = src_ip;
	rte_ether_addr_copy(dst_mac, &arp_hdr->arp_data.arp_tha);
	arp_hdr->arp_data.arp_tip = dst_ip;

	return SC_SUCCESS;
}