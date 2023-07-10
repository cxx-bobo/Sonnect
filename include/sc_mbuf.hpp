#ifndef _SC_MBUF_H_
#define _SC_MBUF_H_

#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_mbuf_core.h>

#define NUM_MBUFS 8191
#define MEMPOOL_CACHE_SIZE 512

/* See: http://dpdk.org/dev/patchwork/patch/4479 */
// #define DEFAULT_PRIV_SIZE   0
// #define MBUF_SIZE           RTE_MBUF_DEFAULT_BUF_SIZE + DEFAULT_PRIV_SIZE

int init_memory(struct sc_config *sc_config);


/*!
 * \brief   flush tx queue in best efford manner
 * \param   port_id         index of the port for sending packets
 * \param   queue           the queue be flushed
 * \param   nb_flush_pkts   number of packet to be sent
 * \param   nb_sent_pkts    number of packet that actually be sent
 * \return  0 for successfully sended
 */
inline int sc_flush_tx_queue(
        uint16_t port_id, uint16_t queue_id, struct rte_mbuf **queue, uint64_t nb_flush_pkts, uint64_t *nb_sent_pkts
){
    int result = SC_SUCCESS;
    uint16_t nb_tx, retry;
    nb_tx = rte_eth_tx_burst(port_id, queue_id, queue, nb_flush_pkts);
    
    if(unlikely(nb_tx < nb_flush_pkts)){
        retry = 0;
        while (nb_tx < nb_flush_pkts && retry++ < SC_BURST_TX_RETRIES) {
            nb_tx += rte_eth_tx_burst(port_id, queue_id, &queue[nb_tx], nb_flush_pkts - nb_tx);
        }
    }

    if (nb_tx < nb_flush_pkts) {
        result = SC_ERROR_NOT_FINISHED;
        do {
            rte_pktmbuf_free(queue[nb_tx]);
        } while (++nb_tx < nb_flush_pkts);
    }

    return result;
}

#endif