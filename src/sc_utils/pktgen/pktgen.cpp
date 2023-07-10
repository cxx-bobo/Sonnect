#include "sc_global.hpp"
#include "sc_utils/pktgen.hpp"
#include "sc_utils.hpp"
#include "sc_control_plane.hpp"
#include "sc_utils/rss.hpp"
#include "sc_utils/timestamp.hpp"


/*!
 * \brief   generate random packet header
 * \param	sc_pkt_hdr		generated packet header
 * \param	pkt_len			length of the generated packet
 * \param	nb_queues		number of total queues
 * \param   used_queue_id	specified queue id to make sure the RSS direct to correct queue
 * \param	l4_type			type of the layer 4 protocol
 * \param	rss_hash_field	rss hash field
 * \param	rss_affinity	whether to generate packet with rss affinity to current core
 * \param	min_pkt_len		minimum packet length
 * \param	payload			pointer to the payload
 * \return  0 for successfully generation
 * \todo	[1] vlan packet support
 * 			[2] vxlan overlay packet support
 */
int sc_util_generate_random_pkt_hdr(
	struct sc_pkt_hdr *sc_pkt_hdr,
	uint32_t pkt_len,
	uint32_t payload_len,
	uint32_t nb_queues,
	uint32_t used_queue_id,
	uint32_t l3_type,
	uint32_t l4_type,
	uint64_t rss_hash_field,
	bool rss_affinity,
	uint32_t min_pkt_len,
	void *payload
){
	int result = SC_SUCCESS;
	uint16_t _pkt_len;
    uint32_t queue_id;

	if(pkt_len < min_pkt_len){
		SC_THREAD_ERROR_DETAILS("pakcet length is too small, should be larger than %d", min_pkt_len);
		result = SC_ERROR_INVALID_VALUE;
		goto sc_util_generate_random_pkt_hdr_exit;
	}
	sc_pkt_hdr->pkt_len = pkt_len;

	/* setting up payload meta */
	sc_pkt_hdr->payload_len = payload_len;
	sc_pkt_hdr->payload = payload;
	sc_pkt_hdr->payload_offset = 0;

	/* initialize the _pkt_len as the length of the l4 payload */
    _pkt_len = payload_len;

    /* generate random port and ipv4 address */
    while(!sc_force_quit){
		/* generate random layer 4 addresses */
        sc_pkt_hdr->src_port = sc_util_random_unsigned_int16();
        sc_pkt_hdr->dst_port = sc_util_random_unsigned_int16();

		/* generate random layer 3 addresses */
		if(l3_type == RTE_ETHER_TYPE_IPV4){
			sc_util_generate_random_ipv4_addr(&sc_pkt_hdr->src_ipv4_addr);
        	sc_util_generate_random_ipv4_addr(&sc_pkt_hdr->dst_ipv4_addr);
		} else if(l3_type == RTE_ETHER_TYPE_IPV6){
			sc_util_generate_random_ipv6_addr(sc_pkt_hdr->src_ipv6_addr);
        	sc_util_generate_random_ipv6_addr(sc_pkt_hdr->dst_ipv6_addr);
		} else {
			SC_THREAD_ERROR("unknown l3 type %u", l3_type);
			result = SC_ERROR_INVALID_VALUE;
			goto sc_util_generate_random_pkt_hdr_exit;
		}
        
		/* 
		 * calculate rss hash result,
		 * to ensure the rss result belongs to current core 
		 */
		if(rss_affinity){
			if(l3_type == RTE_ETHER_TYPE_IPV4){
				sc_util_get_rss_queue_id_ipv4(
					/* src_ipv4 */ sc_pkt_hdr->src_ipv4_addr,
					/* dst_ipv4 */ sc_pkt_hdr->dst_ipv4_addr,
					/* sport */ sc_pkt_hdr->src_port,
					/* dport */ sc_pkt_hdr->dst_port,
					/* sctp_tag */ 0,
					/* nb_queues */ nb_queues,
					/* queue_id */ &queue_id,
					/* rss_hash_field */ rss_hash_field
				);
			} else {
				sc_util_get_rss_queue_id_ipv6(
					/* src_ipv6 */ sc_pkt_hdr->src_ipv6_addr,
					/* dst_ipv6 */ sc_pkt_hdr->dst_ipv6_addr,
					/* sport */ sc_pkt_hdr->src_port,
					/* dport */ sc_pkt_hdr->dst_port,
					/* sctp_tag */ 0,
					/* nb_queues */ nb_queues,
					/* queue_id */ &queue_id,
					/* rss_hash_field */ rss_hash_field
				);
			}

			if(queue_id == used_queue_id){ break; }
		} else {
			break;
		}
    }
    
    /* assemble layer 4 header */
	if(l4_type == IPPROTO_UDP){
		if(SC_SUCCESS != sc_util_initialize_udp_header(
				&sc_pkt_hdr->pkt_udp_hdr, 
				sc_pkt_hdr->src_port, 
				sc_pkt_hdr->dst_port, _pkt_len, &_pkt_len)){
			SC_THREAD_ERROR("failed to assemble udp header");
			result = SC_ERROR_INTERNAL;
			goto sc_util_generate_random_pkt_hdr_exit;
		} else {
			sc_pkt_hdr->payload_offset += sizeof(rte_udp_hdr);
		}
	} else if(l4_type == IPPROTO_TCP){
		if(SC_SUCCESS != sc_util_initialize_tcp_header(
				&sc_pkt_hdr->pkt_tcp_hdr, 
				sc_pkt_hdr->src_port, 
				sc_pkt_hdr->dst_port, _pkt_len, &_pkt_len)){
			SC_THREAD_ERROR("failed to assemble tcp header");
			result = SC_ERROR_INTERNAL;
			goto sc_util_generate_random_pkt_hdr_exit;
		} else {
			sc_pkt_hdr->payload_offset += sizeof(rte_tcp_hdr);
		}
	} else {
		SC_THREAD_ERROR("unknown l4 type %u", l4_type);
		result = SC_ERROR_INVALID_VALUE;
		goto sc_util_generate_random_pkt_hdr_exit;
	}
	sc_pkt_hdr->l4_type = l4_type;
	
    /* assemble l3 header */
	if(l3_type == RTE_ETHER_TYPE_IPV4){
		if(SC_SUCCESS != sc_util_initialize_ipv4_header_proto(
				&sc_pkt_hdr->pkt_ipv4_hdr, 
				sc_pkt_hdr->src_ipv4_addr, 
				sc_pkt_hdr->dst_ipv4_addr, 
				_pkt_len, l4_type, &_pkt_len)
		){
			SC_THREAD_ERROR("failed to assemble ipv4 header");
			result = SC_ERROR_INTERNAL;
			goto sc_util_generate_random_pkt_hdr_exit;
		} else {
			sc_pkt_hdr->payload_offset += sizeof(rte_ipv4_hdr);
		}
	} else if (l4_type == RTE_ETHER_TYPE_IPV6){
		if(SC_SUCCESS != sc_util_initialize_ipv6_header_proto(
				&sc_pkt_hdr->pkt_ipv6_hdr, 
				sc_pkt_hdr->src_ipv6_addr, 
				sc_pkt_hdr->dst_ipv6_addr, 
				_pkt_len, l4_type, &_pkt_len)
		){
			SC_THREAD_ERROR("failed to assemble ipv6 header");
			result = SC_ERROR_INTERNAL;
			goto sc_util_generate_random_pkt_hdr_exit;
		} else {
			sc_pkt_hdr->payload_offset += sizeof(rte_ipv6_hdr);
		}
	}
	sc_pkt_hdr->l3_type = l3_type;
    
    /* assemble ethernet header */
    if(SC_SUCCESS != sc_util_generate_random_ether_addr(
            sc_pkt_hdr->src_ether_addr)){
        SC_THREAD_ERROR("failed to generate random source mac address");
        result = SC_ERROR_INTERNAL;
        goto sc_util_generate_random_pkt_hdr_exit;
    }
    if(SC_SUCCESS != sc_util_generate_random_ether_addr(
            sc_pkt_hdr->dst_ether_addr)){
        SC_THREAD_ERROR("failed to generate random destination mac address");
        result = SC_ERROR_INTERNAL;
        goto sc_util_generate_random_pkt_hdr_exit;
    }
    if(SC_SUCCESS != sc_util_initialize_eth_header(
        &sc_pkt_hdr->pkt_eth_hdr, 
        (struct rte_ether_addr *)sc_pkt_hdr->src_ether_addr, 
        (struct rte_ether_addr *)sc_pkt_hdr->dst_ether_addr,
        RTE_ETHER_TYPE_IPV4, 0, 0, _pkt_len, &_pkt_len
    )){
        SC_THREAD_ERROR("failed to assemble ethernet header");
        result = SC_ERROR_INTERNAL;
        goto sc_util_generate_random_pkt_hdr_exit;
    }
	sc_pkt_hdr->payload_offset += sizeof(rte_ether_hdr);
	sc_pkt_hdr->vlan_enabled = false;

sc_util_generate_random_pkt_hdr_exit:
    return result;
}

/*!
 * \brief   assemble packet headers into a allocated rte_mbuf
 * \param   mp     	rte_mempool, to allocated further segments
 * \param   hdr     provided packet headers
 * \param   pkt     target rte_mbuf
 * \return  0 for successfully assembling
 */
int sc_util_assemble_packet_headers_to_mbuf(struct rte_mempool *mp, struct sc_pkt_hdr *hdr, struct rte_mbuf *pkt){
	int i, result = SC_SUCCESS;
	uint16_t nb_pkt_segs;
	uint32_t assembled_pkt_len = 0;
	size_t eth_hdr_size;
	uint32_t l3_l4_hdr_len = 0;
	struct rte_mbuf *pkt_seg;

	/* determine the number of pkt segments */
	if(hdr->pkt_len > RTE_MBUF_DEFAULT_DATAROOM){
		if(hdr->pkt_len % RTE_MBUF_DEFAULT_DATAROOM == 0){
			nb_pkt_segs = hdr->pkt_len / RTE_MBUF_DEFAULT_DATAROOM;
		} else {
			nb_pkt_segs = hdr->pkt_len / RTE_MBUF_DEFAULT_DATAROOM + 1;
		}
	} else {
		nb_pkt_segs = 1;
	}

	/* assign data_len of the first segment (root segment) of the packet to send */
	if(hdr->pkt_len > RTE_MBUF_DEFAULT_DATAROOM){
		pkt->data_len = RTE_MBUF_DEFAULT_DATAROOM;
		assembled_pkt_len += RTE_MBUF_DEFAULT_DATAROOM;
	} else {
		pkt->data_len = hdr->pkt_len;
		assembled_pkt_len += hdr->pkt_len;
	}

	/* assemble multiple segments inside current pkt */
	pkt_seg = pkt;
	for (i = 1; i < nb_pkt_segs; i++) {
		pkt_seg->next = rte_pktmbuf_alloc(mp);
		if (unlikely(!pkt_seg->next)) {
			if(i == 1){
				// if this is the first segment after root, we don't need to free any mbuf
			} else {
				// not the first segment after root, free all the newly allocated segments
				pkt->next->nb_segs = i-1;
				rte_pktmbuf_free(pkt->next);
			}
			SC_ERROR_DETAILS("failed to allocate memory for rte_mbuf");
			result = SC_ERROR_MEMORY;
			goto assemble_packet_headers_to_mbuf_exit;
		}
		pkt_seg = pkt_seg->next;
		if (i != nb_pkt_segs){
			/* not the last segment, the length is still RTE_MBUF_DEFAULT_DATAROOM */
			pkt_seg->data_len = RTE_MBUF_DEFAULT_DATAROOM;
			assembled_pkt_len += RTE_MBUF_DEFAULT_DATAROOM;
		} else {
			/* the last segment */
			pkt_seg->data_len = hdr->pkt_len - assembled_pkt_len;
		}
	}
	pkt_seg->next = NULL; /* Last segment of packet. */

	if (hdr->vlan_enabled)
		eth_hdr_size = sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr);
	else
		eth_hdr_size = sizeof(struct rte_ether_hdr);
	sc_util_copy_buf_to_pkt(&(hdr->pkt_eth_hdr), eth_hdr_size, pkt, 0);

	/* copy ip and transport header to pkt */
	if (hdr->l3_type == RTE_ETHER_TYPE_IPV4) {
		sc_util_copy_buf_to_pkt(&(hdr->pkt_ipv4_hdr), sizeof(struct rte_ipv4_hdr), pkt, eth_hdr_size);
		l3_l4_hdr_len = sizeof(struct rte_ipv4_hdr);
		switch (hdr->l4_type) {
		case IPPROTO_UDP:
			sc_util_copy_buf_to_pkt(&(hdr->pkt_udp_hdr),
				sizeof(struct rte_udp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
			l3_l4_hdr_len += sizeof(struct rte_udp_hdr);
			break;
		case IPPROTO_TCP:
			sc_util_copy_buf_to_pkt(&(hdr->pkt_tcp_hdr),
				sizeof(struct rte_tcp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
			l3_l4_hdr_len += sizeof(struct rte_tcp_hdr);
			break;
		case IPPROTO_SCTP:
			sc_util_copy_buf_to_pkt(&(hdr->pkt_sctp_hdr),
				sizeof(struct rte_sctp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
			l3_l4_hdr_len += sizeof(struct rte_sctp_hdr);
			break;
		default:
			SC_ERROR_DETAILS("unknown l4 type: %d", hdr->l4_type);
			result = SC_ERROR_INVALID_VALUE;
			goto assemble_packet_headers_to_mbuf_exit;
		}
	} else {
		sc_util_copy_buf_to_pkt(&(hdr->pkt_ipv6_hdr), sizeof(struct rte_ipv6_hdr), pkt, eth_hdr_size);
		l3_l4_hdr_len = sizeof(struct rte_ipv6_hdr);
		switch (hdr->l4_type) {
		case IPPROTO_UDP:
			sc_util_copy_buf_to_pkt(&(hdr->pkt_udp_hdr),
				sizeof(struct rte_udp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv6_hdr));
			l3_l4_hdr_len += sizeof(struct rte_udp_hdr);
			break;
		case IPPROTO_TCP:
			sc_util_copy_buf_to_pkt(&(hdr->pkt_tcp_hdr),
				sizeof(struct rte_tcp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv6_hdr));
			l3_l4_hdr_len += sizeof(struct rte_tcp_hdr);
			break;
		case IPPROTO_SCTP:
			sc_util_copy_buf_to_pkt(&(hdr->pkt_sctp_hdr),
				sizeof(struct rte_sctp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv6_hdr));
			l3_l4_hdr_len += sizeof(struct rte_sctp_hdr);
			break;
		default:
			SC_ERROR_DETAILS("unknown l4 type: %d", hdr->l4_type);
			result = SC_ERROR_INVALID_VALUE;
			goto assemble_packet_headers_to_mbuf_exit;
		}
	}

	/* copy payload, and record payload offset */
	if(hdr->payload != nullptr){
		sc_util_copy_buf_to_pkt(hdr->payload, hdr->payload_len, pkt, eth_hdr_size + l3_l4_hdr_len);
	}
	hdr->payload_offset = eth_hdr_size + l3_l4_hdr_len;

assemble_packet_headers_to_mbuf_exit:
	return result;
}

/*!
 * \brief   copy payload to pkt brust
 * \note	this function will take 10us to copy 24 bytes payload for 512 packets with length 66
 * \param   payload				the layload data
 * \param	payload_len			the length of payload
 * \param	payload_offset		pointer to the offset to the payload
 * \param   pkts_burst 			produced packet burst
 * \param	nb_pkt_per_burst 	number of packets within the produced burst
 * \return  0 for successfully generation
 */
int sc_util_copy_payload_to_packet_burst(
	void *payload, uint64_t payload_len, uint64_t *payload_offset,
	struct rte_mbuf **pkts_burst, uint32_t nb_pkt_per_burst
){
	uint64_t i;
	int result = SC_SUCCESS;
	
	#pragma unroll
	for (i = 0; i < nb_pkt_per_burst; i++) {
		if(unlikely(
			SC_SUCCESS != sc_util_copy_buf_to_pkt(payload, payload_len, pkts_burst[i], *payload_offset)
		)){
			SC_THREAD_ERROR("failed to copy payload to %luth pkt", i);
			result = SC_ERROR_INTERNAL;
			goto copy_payload_to_packet_burst_exit;
		}
	}

	*payload_offset += payload_len;

copy_payload_to_packet_burst_exit:
	return result;
}

/*!
 * \brief   generate packet brust using given header info
 * \note	fast version:
 * 				[1] only support single-segmented mbuf;
 * 				[2] only support ipv4 + udp pkt;
 * \note	this function will allocate new mbufs
 * \note 	this function will take 54us under 512 brust size within 66 pkt size
 * \param   mp					memory buffer pool
 * \param	hdr					the metadata of the generated packet
 * \param   pkts_burst 			produced packet burst
 * \param	nb_pkt_per_burst 	number of packets within the produced burst
 * \return  0 for successfully generation
 */
int sc_util_generate_packet_burst_mbufs_fast_v4_udp(
	struct rte_mempool *mp, struct sc_pkt_hdr *hdr, struct rte_mbuf **pkts_burst, uint32_t nb_pkt_per_burst
){
	int i, result = SC_SUCCESS;
	size_t eth_hdr_size;
	struct rte_mbuf *pkt;
	uint32_t assembled_pkt_len = 0;
	uint32_t l3_l4_hdr_len = 28;
	uint32_t nb_pkt;

	// assert(
	// 	hdr->pkt_len <= RTE_MBUF_DEFAULT_DATAROOM
	// 	&& "not able to generate pkts with sc_util_generate_packet_burst_mbufs_fast while this is a jumbo frame, please use sc_util_generate_packet_burst_mbufs"
	// );
	// assert(hdr->l3_type == RTE_ETHER_TYPE_IPV4 && "only support generate l3=ipv4 pkt");
	// assert(hdr->l4_type == IPPROTO_UDP && "only support generate l4=udp pkt");

	/* produce multiple pkt inside the brust */
	#pragma unroll
	for (nb_pkt = 0; nb_pkt < nb_pkt_per_burst; nb_pkt++) {
		pkt = rte_pktmbuf_alloc(mp);
		if (unlikely(pkt == NULL)) {
			SC_ERROR_DETAILS("failed to allocate memory for rte_mbuf");
			result = SC_ERROR_MEMORY;
			goto generate_packet_burst_mbufs_fast_exit;
		}

		/* assign data_len of the first segment (root segment) of the packet to send */
		pkt->data_len = hdr->pkt_len;
		assembled_pkt_len += hdr->pkt_len;
		
		/* copy ethernet header to pkt */
		eth_hdr_size = sizeof(struct rte_ether_hdr);
		sc_util_copy_buf_to_pkt(&(hdr->pkt_eth_hdr), eth_hdr_size, pkt, 0);
		
		/* copy ip and transport header to pkt */
		sc_util_copy_buf_to_pkt(
			&(hdr->pkt_ipv4_hdr), sizeof(struct rte_ipv4_hdr), pkt, eth_hdr_size);
		l3_l4_hdr_len = sizeof(struct rte_ipv4_hdr);
		sc_util_copy_buf_to_pkt(
			&(hdr->pkt_udp_hdr), sizeof(struct rte_udp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
		l3_l4_hdr_len += sizeof(struct rte_udp_hdr);

		/* copy payload */
		if(hdr->payload != nullptr)
			sc_util_copy_buf_to_pkt(hdr->payload, hdr->payload_len, pkt, eth_hdr_size + l3_l4_hdr_len);

		/*
		 * Complete first mbuf of packet and append it to the
		 * burst of packets to be transmitted.
		 */
		pkt->nb_segs = 1;
		pkt->pkt_len = hdr->pkt_len;
		pkt->l2_len = eth_hdr_size;
		pkt->vlan_tci  = RTE_ETHER_TYPE_IPV4;
		pkt->l3_len = sizeof(struct rte_ipv4_hdr);

		pkts_burst[nb_pkt] = pkt;
	}

	hdr->payload_offset = eth_hdr_size + l3_l4_hdr_len;

generate_packet_burst_mbufs_fast_exit:
	return result;
}

/*!
 * \brief   generate packet brust using given header info
 * \note	this function will allocate new mbufs
 * \note 	this function will take around 80~200us under 512 brust size within 66 pkt size
 * \param   mp					memory buffer pool
 * \param	hdr					the metadata of the generated packet
 * \param   pkts_burst 			produced packet burst
 * \param	nb_pkt_per_burst 	number of packets within the produced burst
 * \return  0 for successfully generation
 */
int sc_util_generate_packet_burst_mbufs(
	struct rte_mempool *mp, struct sc_pkt_hdr *hdr, struct rte_mbuf **pkts_burst, uint32_t nb_pkt_per_burst
){
	int i, result = SC_SUCCESS;
	size_t eth_hdr_size;
	struct rte_mbuf *pkt_seg;
	struct rte_mbuf *pkt;
	uint32_t assembled_pkt_len = 0;
	uint32_t l3_l4_hdr_len = 0;
	uint32_t nb_pkt;

	/* determine the number of pkt segments */
	uint16_t nb_pkt_segs;
	if(hdr->pkt_len > RTE_MBUF_DEFAULT_DATAROOM){
		if(hdr->pkt_len % RTE_MBUF_DEFAULT_DATAROOM == 0){
			nb_pkt_segs = hdr->pkt_len / RTE_MBUF_DEFAULT_DATAROOM;
		} else {
			nb_pkt_segs = hdr->pkt_len / RTE_MBUF_DEFAULT_DATAROOM + 1;
		}
	} else {
		nb_pkt_segs = 1;
	}
	
	/* produce multiple pkt inside the brust */
	#pragma unroll
	for (nb_pkt = 0; nb_pkt < nb_pkt_per_burst; nb_pkt++) {
		pkt = rte_pktmbuf_alloc(mp);
		if (unlikely(pkt == NULL)) {
			SC_ERROR_DETAILS("failed to allocate memory for rte_mbuf");
			result = SC_ERROR_MEMORY;
			goto generate_packet_burst_proto_exit;
		}
		
		/* assign data_len of the first segment (root segment) of the packet to send */
		if(hdr->pkt_len > RTE_MBUF_DEFAULT_DATAROOM){
			pkt->data_len = RTE_MBUF_DEFAULT_DATAROOM;
			assembled_pkt_len += RTE_MBUF_DEFAULT_DATAROOM;
		} else {
			pkt->data_len = hdr->pkt_len;
			assembled_pkt_len += hdr->pkt_len;
		}
		
		/* assemble multiple segments inside current pkt */
		pkt_seg = pkt;
		for (i = 1; i < nb_pkt_segs; i++) {
			pkt_seg->next = rte_pktmbuf_alloc(mp);
			if (unlikely(!pkt_seg->next)) {
				pkt->nb_segs = i;
				rte_pktmbuf_free(pkt);
				SC_ERROR_DETAILS("failed to allocate memory for rte_mbuf");
				result = SC_ERROR_MEMORY;
				goto generate_packet_burst_proto_exit;
			}
			pkt_seg = pkt_seg->next;
			if (i != nb_pkt_segs){
				/* not the last segment, the length is still RTE_MBUF_DEFAULT_DATAROOM */
				pkt_seg->data_len = RTE_MBUF_DEFAULT_DATAROOM;
				assembled_pkt_len += RTE_MBUF_DEFAULT_DATAROOM;
			} else {
				/* the last segment */
				pkt_seg->data_len = hdr->pkt_len - assembled_pkt_len;
			}
		}
		pkt_seg->next = NULL; /* Last segment of packet. */

		/* copy ethernet header to pkt */
		if (hdr->vlan_enabled){
			eth_hdr_size = sizeof(struct rte_ether_hdr) + sizeof(struct rte_vlan_hdr);
			// TODO: how to copy eth header with vlan?
		} else {
			eth_hdr_size = sizeof(struct rte_ether_hdr);
			sc_util_copy_buf_to_pkt(&(hdr->pkt_eth_hdr), eth_hdr_size, pkt, 0);
		}

		/* copy ip and transport header to pkt */
		if (hdr->l3_type == RTE_ETHER_TYPE_IPV4) {
			sc_util_copy_buf_to_pkt(&(hdr->pkt_ipv4_hdr), sizeof(struct rte_ipv4_hdr), pkt, eth_hdr_size);
			l3_l4_hdr_len = sizeof(struct rte_ipv4_hdr);
			switch (hdr->l4_type) {
			case IPPROTO_UDP:
				sc_util_copy_buf_to_pkt(&(hdr->pkt_udp_hdr),
					sizeof(struct rte_udp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
				l3_l4_hdr_len += sizeof(struct rte_udp_hdr);
				break;
			case IPPROTO_TCP:
				sc_util_copy_buf_to_pkt(&(hdr->pkt_tcp_hdr),
					sizeof(struct rte_tcp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
				l3_l4_hdr_len += sizeof(struct rte_tcp_hdr);
				break;
			case IPPROTO_SCTP:
				sc_util_copy_buf_to_pkt(&(hdr->pkt_sctp_hdr),
					sizeof(struct rte_sctp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv4_hdr));
				l3_l4_hdr_len += sizeof(struct rte_sctp_hdr);
				break;
			default:
				SC_ERROR_DETAILS("unknown l4 type: %u", hdr->l4_type);
				result = SC_ERROR_INVALID_VALUE;
				goto generate_packet_burst_proto_exit;
			}
		} else if (hdr->l3_type == RTE_ETHER_TYPE_IPV6) {
			sc_util_copy_buf_to_pkt(&(hdr->pkt_ipv6_hdr), sizeof(struct rte_ipv6_hdr), pkt, eth_hdr_size);
			l3_l4_hdr_len = sizeof(struct rte_ipv6_hdr);
			switch (hdr->l4_type) {
			case IPPROTO_UDP:
				sc_util_copy_buf_to_pkt(&(hdr->pkt_udp_hdr),
					sizeof(struct rte_udp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv6_hdr));
				l3_l4_hdr_len += sizeof(struct rte_udp_hdr);
				break;
			case IPPROTO_TCP:
				sc_util_copy_buf_to_pkt(&(hdr->pkt_tcp_hdr),
					sizeof(struct rte_tcp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv6_hdr));
				l3_l4_hdr_len += sizeof(struct rte_tcp_hdr);
				break;
			case IPPROTO_SCTP:
				sc_util_copy_buf_to_pkt(&(hdr->pkt_sctp_hdr),
					sizeof(struct rte_sctp_hdr), pkt, eth_hdr_size + sizeof(struct rte_ipv6_hdr));
				l3_l4_hdr_len += sizeof(struct rte_sctp_hdr);
				break;
			default:
				SC_THREAD_ERROR_DETAILS("unknown l4 type: %u", hdr->l4_type);
				result = SC_ERROR_INVALID_VALUE;
				goto generate_packet_burst_proto_exit;
			}
		} else {
			SC_THREAD_ERROR_DETAILS("unknown l3 type: %u", hdr->l3_type);
			result = SC_ERROR_INVALID_VALUE;
			goto generate_packet_burst_proto_exit;
		}

		/* copy payload */
		sc_util_copy_buf_to_pkt(hdr->payload, hdr->payload_len, pkt, eth_hdr_size + l3_l4_hdr_len);

		/*
		 * Complete first mbuf of packet and append it to the
		 * burst of packets to be transmitted.
		 */
		pkt->nb_segs = nb_pkt_segs;
		pkt->pkt_len = hdr->pkt_len;
		pkt->l2_len = eth_hdr_size;

		if (hdr->l3_type == RTE_ETHER_TYPE_IPV4) {
			pkt->vlan_tci  = RTE_ETHER_TYPE_IPV4;
			pkt->l3_len = sizeof(struct rte_ipv4_hdr);
		} else {
			pkt->vlan_tci  = RTE_ETHER_TYPE_IPV6;
			pkt->l3_len = sizeof(struct rte_ipv6_hdr);
		}

		pkts_burst[nb_pkt] = pkt;
	}

	hdr->payload_offset = eth_hdr_size + l3_l4_hdr_len;

generate_packet_burst_proto_exit:
	return result;
}

/*!
 * \brief   copy pkt data to mbufs chain, according to given offset
 * \param   buf     source data buffer
 * \param   len     length of the source data
 * \param   pkt     destinuation mbuf
 * \param   offset  copy offset within the destination mbuf
 * \return  0 for successfully copy
 */
int _sc_util_copy_buf_to_pkt_segs(void *buf, uint32_t len, struct rte_mbuf *pkt, uint32_t offset){
	struct rte_mbuf *seg;
	void *seg_buf;
	unsigned copy_len;
	
	seg = pkt;
	while (offset >= seg->data_len) {
		offset -= seg->data_len;
		seg = seg->next;
        if(unlikely(!seg)){
            SC_ERROR_DETAILS("reach the end of the mbuf chain");
            return SC_ERROR_INVALID_VALUE;
        }
	}

	copy_len = seg->data_len - offset;
	seg_buf = rte_pktmbuf_mtod_offset(seg, char *, offset);
	while (len > copy_len) {
		rte_memcpy(seg_buf, buf, (size_t) copy_len);
		len -= copy_len;
		buf = ((char *) buf + copy_len);
		seg = seg->next;
		seg_buf = rte_pktmbuf_mtod(seg, void *);
	}
	rte_memcpy(seg_buf, buf, (size_t) len);

    return SC_SUCCESS;
}

/*!
 * \brief   copy pkt data to mbufs, according to given offset
 * \param   buf     source data buffer
 * \param   len     length of the source data
 * \param   pkt     destinuation mbuf
 * \param   offset  copy offset within the destination mbuf
 * \return  0 for successfully copy
 */
int sc_util_copy_buf_to_pkt(void *buf, uint32_t len, struct rte_mbuf *pkt, uint32_t offset){
	if (offset + len <= pkt->data_len) {
		rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *, offset), buf, (size_t) len);
		return SC_SUCCESS;
	}
	return _sc_util_copy_buf_to_pkt_segs(buf, len, pkt, offset);
}
