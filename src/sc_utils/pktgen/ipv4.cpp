#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   generate random ipv4 address
 * \param   addr   	generated 32-bit ipv4 address
 * \return  0 for successfully generation
 */
int sc_util_generate_random_ipv4_addr(uint32_t *addr){
	uint8_t i;
	uint8_t sub_addr[4];
	
	for(i=0; i<4; i++){
		sub_addr[i] = sc_util_random_unsigned_int8();
	}
	
	return sc_util_generate_ipv4_addr(sub_addr, addr);
}

/*!
 * \brief   generate specified ipv4 address
 * \param	specified_addr	specified ipv4 address
 * \param   result_addr		generated 32-bit ipv4 address
 * \return  0 for successfully generation
 */
int sc_util_generate_ipv4_addr(uint8_t *specified_addr, uint32_t *result_addr){
	*result_addr = IPV4_ADDR(
		specified_addr[0], specified_addr[1], specified_addr[2], specified_addr[3]
	);
	return SC_SUCCESS;
}

/*!
 * \brief   initialize RTE IPv4 header
 * \param   ip_hdr				RTE IPv4 header
 * \param	src_addr			source IPv4 address
 * \param	dst_addr			destination IPv4 address
 * \param	proto				L4 protocol type
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully initialization
 */
int sc_util_initialize_ipv4_header_proto(struct rte_ipv4_hdr *ip_hdr, uint32_t src_addr,
		uint32_t dst_addr, uint16_t pkt_data_len, uint8_t proto, uint16_t *pkt_len){
	unaligned_uint16_t *ptr16;
	uint32_t ip_cksum;

	/*
	 * Initialize IP header.
	 */
	*pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_ipv4_hdr));

	ip_hdr->version_ihl   = RTE_IPV4_VHL_DEF;
	ip_hdr->type_of_service   = 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live   = IP_DEFTTL;
	ip_hdr->next_proto_id = proto;
	ip_hdr->packet_id = 0;
	ip_hdr->total_length   = rte_cpu_to_be_16(*pkt_len);
	ip_hdr->src_addr = rte_cpu_to_be_32(src_addr);
	ip_hdr->dst_addr = rte_cpu_to_be_32(dst_addr);

	/*
	 * Compute IP header checksum.
	 */
	ptr16 = (unaligned_uint16_t *)ip_hdr;
	ip_cksum = 0;
	ip_cksum += ptr16[0]; ip_cksum += ptr16[1];
	ip_cksum += ptr16[2]; ip_cksum += ptr16[3];
	ip_cksum += ptr16[4];
	ip_cksum += ptr16[6]; ip_cksum += ptr16[7];
	ip_cksum += ptr16[8]; ip_cksum += ptr16[9];

	/*
	 * Reduce 32 bit checksum to 16 bits and complement it.
	 */
	ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) +
		(ip_cksum & 0x0000FFFF);
	ip_cksum %= 65536;
	ip_cksum = (~ip_cksum) & 0x0000FFFF;
	if (ip_cksum == 0)
		ip_cksum = 0xFFFF;
	ip_hdr->hdr_checksum = (uint16_t) ip_cksum;

	return SC_SUCCESS;
}