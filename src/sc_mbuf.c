#include "sc_global.h"
#include "sc_utils.h"
#include "sc_mbuf.h"

/*!
 * \brief   initialize dpdk memory
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_memory(struct sc_config *sc_config){
    struct rte_mempool *pktmbuf_pool;

    /* allocate mbuf pool */
    pktmbuf_pool = rte_pktmbuf_pool_create(
        /* name */ "mbuf_pool", 
        /* n */ (sc_config->rx_queue_len+sc_config->tx_queue_len)*sc_config->nb_used_cores-1, 
        /* cache_size */ MEMPOOL_CACHE_SIZE, 
        /* priv_size */ 0, 
        /* data_room_size */ RTE_MBUF_DEFAULT_BUF_SIZE, 
        /* socket_id */ rte_socket_id()
    );
    if (!pktmbuf_pool){
        printf("failed to allocate memory for mbuf pool");
        return SC_ERROR_MEMORY;
    }
    sc_config->pktmbuf_pool = pktmbuf_pool;

    return SC_SUCCESS;
}