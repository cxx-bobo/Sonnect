#ifndef _SC_UTILS_PKTGEN_H_
#define _SC_UTILS_PKTGEN_H_

#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_arp.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <rte_sctp.h>
#include <rte_ethdev.h>

#define IP_DEFTTL  64   /* from RFC 1340. */

#define IPV4_ADDR(a, b, c, d)(((a & 0xff) << 24) | ((b & 0xff) << 16) | \
		((c & 0xff) << 8) | (d & 0xff))

#define SC_UTIL_TIME_INTERVL_US(sec, usec) usec + sec * 1000 * 1000

/* header collection */
struct sc_pkt_hdr {
	/* ========== fields filled during generation ========== */
	/* ethernet header */
	uint8_t vlan_enabled;
	struct rte_ether_hdr pkt_eth_hdr;
	struct rte_vlan_hdr pkt_vlan_hdr;
    char src_ether_addr[6], dst_ether_addr[6];
    
    /* L3 header */
	uint32_t l3_type;
    uint32_t src_ipv4_addr, dst_ipv4_addr;
	uint8_t src_ipv6_addr[16], dst_ipv6_addr[16];
    struct rte_ipv4_hdr pkt_ipv4_hdr;
	struct rte_ipv6_hdr pkt_ipv6_hdr;

    /* L4 header */
	uint32_t l4_type;
    uint16_t src_port, dst_port;
    struct rte_udp_hdr 	pkt_udp_hdr;
	struct rte_tcp_hdr 	pkt_tcp_hdr;
	struct rte_sctp_hdr pkt_sctp_hdr;

	/* length of the genearted packet */
	uint32_t pkt_len;

	/* pointer to the payload of this packet */
	void *payload;
	uint32_t payload_len;

	/* pkt metadata */
	uint64_t service_time;

	/* ========== fields filled during assembling ========== */
	uint64_t payload_offset;
};

/* data copier */
int sc_util_copy_buf_to_pkt(void *buf, unsigned len, struct rte_mbuf *pkt, unsigned offset);

/* header field generator */
int sc_util_generate_random_ether_addr(char *addr);
int sc_util_generate_random_ipv4_addr(uint32_t *addr);
int sc_util_generate_random_ipv6_addr(uint8_t *addr);
int sc_util_generate_random_pkt_hdr(struct sc_pkt_hdr *sc_pkt_hdr, uint32_t pkt_len, 
	uint32_t payload_len, uint32_t nb_queues, uint32_t used_queue_id, uint32_t l3_type, 
	uint32_t l4_type, uint64_t rss_hash_field, bool rss_affinity, uint32_t min_pkt_len,
	void *payload);
int sc_util_generate_ipv4_addr(uint8_t *specified_addr, uint32_t *result_addr);

/* rte header initializer */
int sc_util_clone_mbuf_brust(struct rte_mempool *mp, struct rte_mbuf **mbufs, 
		struct rte_mbuf *source_mbuf, uint64_t brust_size, void *new_payload, 
		uint64_t new_payload_size, uint64_t new_payload_offset);
int sc_util_assemble_packet_headers_to_mbuf(struct rte_mempool *mp, struct sc_pkt_hdr *hdr, 
		struct rte_mbuf *pkt);
int sc_util_copy_payload_to_packet_burst(void *payload, uint64_t payload_len, uint64_t payload_offset,
	struct rte_mbuf **pkts_burst, uint32_t nb_pkt_per_burst);
int sc_util_generate_packet_burst_mbufs_fast_v4_udp(struct rte_mempool *mp, struct sc_pkt_hdr *hdr,
		struct rte_mbuf **pkts_burst, uint32_t nb_pkt_per_burst);
int sc_util_generate_packet_burst_mbufs(struct rte_mempool *mp, struct sc_pkt_hdr *hdr, 
		struct rte_mbuf **pkts_burst, uint32_t nb_pkt_per_burst);
int sc_util_initialize_eth_header(struct rte_ether_hdr *eth_hdr,
		struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac, 
		uint16_t ether_type, uint8_t vlan_enabled, uint16_t vlan_id, 
		uint16_t pkt_data_len, uint16_t *pkt_len);
int sc_util_initialize_arp_header(struct rte_arp_hdr *arp_hdr,
		struct rte_ether_addr *src_mac, struct rte_ether_addr *dst_mac,
		uint32_t src_ip, uint32_t dst_ip, uint32_t opcode, 
		uint16_t pkt_data_len, uint16_t *pkt_len);
int sc_util_initialize_udp_header(struct rte_udp_hdr *udp_hdr, uint16_t src_port,
		uint16_t dst_port, uint16_t pkt_data_len, uint16_t *pkt_len);
int sc_util_initialize_tcp_header(struct rte_tcp_hdr *tcp_hdr, uint16_t src_port,
		uint16_t dst_port, uint16_t pkt_data_len, uint16_t *pkt_len);
int sc_util_initialize_sctp_header(struct rte_sctp_hdr *sctp_hdr, uint16_t src_port,
		uint16_t dst_port, uint16_t pkt_data_len, uint16_t *pkt_len);
int sc_util_initialize_ipv6_header_proto(struct rte_ipv6_hdr *ip_hdr, uint8_t *src_addr,
		uint8_t *dst_addr, uint16_t pkt_data_len, uint8_t proto, uint16_t *pkt_len);
int sc_util_initialize_ipv4_header_proto(struct rte_ipv4_hdr *ip_hdr, uint32_t src_addr,
		uint32_t dst_addr, uint16_t pkt_data_len, uint8_t proto, uint16_t *pkt_len);

#endif