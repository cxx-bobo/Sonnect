#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_mbuf.hpp"
#include "sc_port.hpp"

/*!
 * \brief   initialize dpdk memory
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_memory(struct sc_config *sc_config){
    int port_logical_id, queue_id;
    uint16_t nb_used_ports;
    uint16_t port_indices[SC_MAX_USED_PORTS];
    struct rte_mempool *pktmbuf_pool;
    char mbuf_pool_name[128];

    /* 
     * the port info isn't initialized at this time while invoking init_memory, 
     * so we should mannully obtain the number of used ports here
     */
    if(SC_SUCCESS != get_used_ports_id(sc_config, &nb_used_ports, port_indices)){
        SC_ERROR_DETAILS("failed to obtain the number of used ports")
        return SC_ERROR_INTERNAL;
    }

    /* allocate pointer array to store pointers to mbuf pools */
    sc_config->rx_pktmbuf_pool = (struct rte_mempool**)malloc(
        sizeof(struct rte_mempool*) * nb_used_ports * sc_config->nb_rx_rings_per_port
    );
    if(unlikely(!sc_config->rx_pktmbuf_pool)){
        SC_ERROR_DETAILS("failed to allocate memory to store pointers to mbuf pool of rx queue");
        return SC_ERROR_MEMORY;
    }

    /* allocate pointer array to store pointers to mbuf pools */
    sc_config->tx_pktmbuf_pool = (struct rte_mempool**)malloc(
        sizeof(struct rte_mempool*) * nb_used_ports * sc_config->nb_tx_rings_per_port
    );
    if(unlikely(!sc_config->tx_pktmbuf_pool)){
        SC_ERROR_DETAILS("failed to allocate memory to store pointers to mbuf pool of tx queue");
        return SC_ERROR_MEMORY;
    }

    /* allocate per-queue (per-core) mbuf pool */
    for(port_logical_id=0; port_logical_id<nb_used_ports; port_logical_id++){
        for(queue_id=0; queue_id<sc_config->nb_rx_rings_per_port; queue_id++){
            /* set the name of the mbuf pool */
            memset(mbuf_pool_name, 0, sizeof(mbuf_pool_name));
            sprintf(mbuf_pool_name, "rx_p%d_q%d", port_logical_id, queue_id);
            
            /* allocate mbuf pool */
            // pktmbuf_pool = rte_pktmbuf_pool_create(
            //     /* name */ mbuf_pool_name, 
            //     /*!
            //      * \note: should make sure number of element in the mbuf pool 
            //      * is greater or equal to sc_config->rx_queue_len, and at the
            //      * same time the dpdk suggest it should be a power of two minus 
            //      * one, so we use sc_config->rx_queue_len*2-1
            //      */
            //     /* n */ sc_config->rx_queue_len*2-1, 
            //     /* cache_size */ MEMPOOL_CACHE_SIZE, 
            //     /* priv_size */ 0, 
            //     /* data_room_size */ RTE_MBUF_DEFAULT_BUF_SIZE, 
            //     /* socket_id */ rte_eth_dev_socket_id(port_indices[port_logical_id])
            // );

            pktmbuf_pool = rte_mempool_create(
                /* name */ mbuf_pool_name,
                /* n */ sc_config->rx_queue_len*2-1, 
                /* elt_size */ MBUF_SIZE, 
                /* cache_size */ MEMPOOL_CACHE_SIZE, 
                /* private_data_size */ sizeof(struct rte_pktmbuf_pool_private), 
				/* mp_init */ rte_pktmbuf_pool_init, 
                /* mp_init_arg */ NULL,
                /* obj_init */ rte_pktmbuf_init, 
                /* obj_init_arg */ NULL,
				/* socket_id */ rte_eth_dev_socket_id(port_indices[port_logical_id]),
                /* flag */ MEMPOOL_F_SP_PUT | MEMPOOL_F_SC_GET
            );

            if (!pktmbuf_pool){
                SC_ERROR_DETAILS(
                    "failed to allocate mbuf pool memory for port %d rx queue %d: %s",
                        port_logical_id, queue_id, rte_strerror(rte_errno)
                );
                return SC_ERROR_MEMORY;
            }

            // rte_pktmbuf_pool_init(pktmbuf_pool, NULL);

            sc_config->rx_pktmbuf_pool[RX_QUEUE_MEMORY_POOL_ID(sc_config, port_logical_id, queue_id)] 
                = pktmbuf_pool;
        }

        for(queue_id=0; queue_id<sc_config->nb_tx_rings_per_port; queue_id++){
            /* set the name of the mbuf pool */
            memset(mbuf_pool_name, 0, sizeof(mbuf_pool_name));
            sprintf(mbuf_pool_name, "tx_p%d_q%d", port_logical_id, queue_id);
            
            /* allocate mbuf pool */
            // pktmbuf_pool = rte_pktmbuf_pool_create(
            //     /* name */ mbuf_pool_name, 
            //     /* n */ sc_config->tx_queue_len*2-1, 
            //     /* cache_size */ MEMPOOL_CACHE_SIZE, 
            //     /* priv_size */ 0, 
            //     /* data_room_size */ RTE_MBUF_DEFAULT_BUF_SIZE, 
            //     /* socket_id */ rte_eth_dev_socket_id(port_indices[port_logical_id])
            // );

            pktmbuf_pool = rte_mempool_create(
                /* name */ mbuf_pool_name,
                /* n */ sc_config->rx_queue_len*2-1, 
                /* elt_size */ MBUF_SIZE, 
                /* cache_size */ MEMPOOL_CACHE_SIZE, 
                /* private_data_size */ sizeof(struct rte_pktmbuf_pool_private), 
				/* mp_init */ rte_pktmbuf_pool_init, 
                /* mp_init_arg */ NULL,
                /* obj_init */ rte_pktmbuf_init, 
                /* obj_init_arg */ NULL,
				/* socket_id */ rte_eth_dev_socket_id(port_indices[port_logical_id]),
                /* flag */ MEMPOOL_F_SP_PUT | MEMPOOL_F_SC_GET
            );

            if (!pktmbuf_pool){
                SC_ERROR_DETAILS(
                    "failed to allocate mbuf pool memory for port %d tx queue %d: %s",
                        port_logical_id, queue_id, rte_strerror(rte_errno)
                );
                return SC_ERROR_MEMORY;
            }

            // rte_pktmbuf_pool_init(pktmbuf_pool, NULL);

            sc_config->tx_pktmbuf_pool[TX_QUEUE_MEMORY_POOL_ID(sc_config, port_logical_id, queue_id)] 
                = pktmbuf_pool;
        }
    }

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