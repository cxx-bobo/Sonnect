#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_log.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"

/*!
 * \brief   generate random ethernet address
 * \param   addr   	buffer to store generated ethernet address
 * 					(make sure addr is at least 6 bytes long)
 * \return  0 for successfully generation
 */
int sc_util_generate_random_ether_addr(char *addr){
	uint8_t i;
	uint8_t octet;

	for(i=0; i<6; i++){
		octet = sc_util_random_unsigned_int8();
		addr[i] = (char)octet;
	}

	return SC_SUCCESS;
}

/*!
 * \brief   copy pkt data to mbufs, according to given offset
 * \param   eth_hdr     	source data buffer
 * \param   src_mac     	length of the source data
 * \param   dst_mac     	destinuation mbuf
 * \param   ether_type  	copy offset within the destination mbuf
 * \param   vlan_enabled  	copy offset within the destination mbuf
 * \param   vlan_id  		copy offset within the destination mbuf
 * \param	pkt_data_len 		length of upper payload
 * \param	pkt_len				generated packet length
 * \return  0 for successfully copy
 */
int sc_util_initialize_eth_header(struct rte_ether_hdr *eth_hdr,
		struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac, 
		uint16_t ether_type, uint8_t vlan_enabled, uint16_t vlan_id, 
		uint16_t pkt_data_len, uint16_t *pkt_len){
	#if RTE_VERSION >= RTE_VERSION_NUM(20, 11, 255, 255)
		rte_ether_addr_copy(dst_mac, &eth_hdr->dst_addr);
		rte_ether_addr_copy(src_mac, &eth_hdr->src_addr);
	#else
		rte_ether_addr_copy(dst_mac, &eth_hdr->d_addr);
		rte_ether_addr_copy(src_mac, &eth_hdr->s_addr);
	#endif

	if (vlan_enabled) {
		struct rte_vlan_hdr *vhdr = (struct rte_vlan_hdr *)(
			(uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));

		eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_VLAN);

		vhdr->eth_proto =  rte_cpu_to_be_16(ether_type);
		vhdr->vlan_tci = vlan_id;
	} else {
		eth_hdr->ether_type = rte_cpu_to_be_16(ether_type);
	}

	*pkt_len = pkt_data_len + (uint16_t)(sizeof(struct rte_ether_hdr));

	return SC_SUCCESS;
}