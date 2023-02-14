#include "sc_global.h"
#include "sc_worker.h"
#include "sc_utils.h"
#include "sc_log.h"

extern volatile bool force_quit;

/*!
 * \brief   function that execute on each lcore threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully executed
 */
int _worker_loop(void* param){
    int result = SC_SUCCESS;
    uint16_t i, j, queue_id, nb_rx;
    struct sc_config *sc_config = (struct sc_config*)param;
    static struct rte_mbuf *pkt[SC_MAX_PKT_BURST];

    for(i=0; i<sc_config->nb_used_cores; i++){
        if(sc_config->core_ids[i] == rte_lcore_id()){
            queue_id = i; break;
        }
        if(i == sc_config->nb_used_cores-1){
            SC_THREAD_ERROR_DETAILS("unknown queue id for worker thread on lcore %u", rte_lcore_id());
            result = SC_ERROR_INTERNAL;
            goto worker_exit;
        }
    }

    while(!force_quit){
        for(i=0; i<sc_config->nb_used_ports; i++){
            for(j=0; j<SC_MAX_PKT_BURST; j++) {
                pkt[j] = rte_pktmbuf_alloc(sc_config->pktmbuf_pool);
            }

            nb_rx = rte_eth_rx_burst(i, queue_id, pkt, SC_MAX_PKT_BURST);
            
            if(nb_rx == 0) goto free_pkt_mbuf;
            
            SC_THREAD_LOG("received %u ethernet frames", nb_rx);

            for(j=0; j<nb_rx; j++){
                if(sc_config->app_config->process_pkt(pkt[j], sc_config) != SC_SUCCESS){
                    SC_THREAD_WARNING("failed to process the received frame\n");
                }
            }

free_pkt_mbuf:
            for(j=0; j<SC_MAX_PKT_BURST; j++) {
                rte_pktmbuf_free(pkt[j]);
            }
        }
    }

worker_exit:
    SC_THREAD_WARNING("worker thread exit\n");
    return result;
}

/*!
 * \brief   initialize worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_worker_threads(struct sc_config *sc_config){
    /* initialize the indicator for quiting all worker threads */
    force_quit = false;

    /* make sure the process_pkt function is set */
    if(!sc_config->app_config->process_pkt){
        SC_ERROR_DETAILS("empty process_pkt function pointer");
        return SC_ERROR_INTERNAL;
    }

    return SC_SUCCESS;
}

/*!
 * \brief   launch worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int launch_worker_threads(struct sc_config *sc_config){
    launch_worker_threads_async(sc_config);
    rte_eal_mp_wait_lcore();
    return SC_SUCCESS;
}

/*!
 * \brief   launch worker threads (async)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int launch_worker_threads_async(struct sc_config *sc_config){
    rte_eal_mp_remote_launch(_worker_loop, (void*)sc_config, CALL_MAIN);
    return SC_SUCCESS;
}