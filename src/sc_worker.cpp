#include "sc_global.hpp"
#include "sc_worker.hpp"
#include "sc_utils.hpp"
#include "sc_mbuf.hpp"
#include "sc_control_plane.hpp"

extern volatile bool sc_force_quit;

__thread uint32_t perthread_lcore_logical_id;

/*!
 * \brief   initialize worker loop after enter it
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int __worker_loop_init(struct sc_config *sc_config) {
    return SC_SUCCESS;
}


/*!
 * \brief   function that execute on each lcore threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully executed
 */
int _worker_loop(void* param){
    int result = SC_SUCCESS;
    uint16_t i, j, queue_id, nb_rx;
    int lcore_id_from_zero;
    struct sc_config *sc_config = (struct sc_config*)param;

    /* record lcore id starts from 0 */
    lcore_id_from_zero = rte_lcore_index(rte_lcore_id());
    perthread_lcore_logical_id = lcore_id_from_zero;

    process_enter_t process_enter_func = PER_CORE_WORKER_FUNC(sc_config).process_enter_func;
    #if defined(ROLE_SERVER)
        process_pkt_t process_pkt_func = PER_CORE_WORKER_FUNC(sc_config).process_pkt_func;
        process_pkt_drop_t process_pkt_drop_func = PER_CORE_WORKER_FUNC(sc_config).process_pkt_drop_func;
    #endif
    #if defined(ROLE_CLIENT)
        process_client_t process_client_func = PER_CORE_WORKER_FUNC(sc_config).process_client_func;
    #endif
    process_exit_t process_exit_func = PER_CORE_WORKER_FUNC(sc_config).process_exit_func;

    #if defined(ROLE_SERVER)
        struct rte_mbuf *pkt[SC_MAX_RX_PKT_BURST*2];
    #endif // ROLE_SERVER

    #if defined(ROLE_CLIENT)
        bool ready_to_exit = false;
    #endif // ROLE_CLIENT

    /* initialize woker loop */
    result = __worker_loop_init(sc_config);
    if(result != SC_SUCCESS){
        SC_THREAD_ERROR("failed to initialize worker loop");
        goto worker_exit;
    }

    /* determain the queue id */
    for(i=0; i<sc_config->nb_used_cores; i++){
        if(sc_config->core_ids[i] == rte_lcore_id()){
            queue_id = i;
            /*! 
                \note:  we allow nb_rings_per_port is less than nb_used_port 
                        so we have to limit the queue_id within the valid range
             */
            if(queue_id != 0){
                queue_id = queue_id % sc_config->nb_rx_rings_per_port;
            }
            
            SC_THREAD_LOG("core %u is using queue %u", rte_lcore_id(), queue_id);
            break;
        }
        if(i == sc_config->nb_used_cores-1){
            SC_THREAD_ERROR_DETAILS("unknown queue id for worker thread on lcore %u", rte_lcore_id());
            result = SC_ERROR_INTERNAL;
            goto worker_exit;
        }
    }

    /* Hook Point: Enter */    
    if(SC_SUCCESS != process_enter_func(sc_config)){
        SC_THREAD_ERROR("error occurs while executing enter callback\n");
        sc_force_quit = true;
    }

    while(!sc_force_quit){
        /* role: server */
        #if defined(ROLE_SERVER)
            for(i=0; i<sc_config->nb_used_ports; i++){

                nb_rx = rte_eth_rx_burst(sc_config->sc_port[i].port_id, queue_id, pkt, SC_MAX_RX_PKT_BURST);
                
                if(nb_rx == 0) continue;
                
                /* Hook Point: Packet Processing */
                if(unlikely(
                    SC_SUCCESS != process_pkt_func(
                        /* pkt */ pkt, 
                        /* nb_rx */ nb_rx,
                        /* sc_config */ sc_config,
                        /* queue_id */ queue_id,
                        /* recv_port_id */ i
                    )
                )){
                    SC_THREAD_WARNING_LOCKLESS("failed to process the received frame");
                }
            }
        #endif // ROLE_SERVER

        /* role: client */
        #if defined(ROLE_CLIENT)
            /* Hook Point: Packet Preparing */
            if(SC_SUCCESS != process_client_func(sc_config, queue_id, &ready_to_exit)){
                SC_THREAD_WARNING_LOCKLESS("error occured within the client process");
            }

            if(ready_to_exit){ break; }
        #endif // ROLE_CLIENT
    }

exit_callback:
    /* Hook Point: Exit */
    if(SC_SUCCESS != process_exit_func(sc_config)){
        SC_THREAD_WARNING("error occurs while executing exit callback\n");
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
    sc_force_quit = false;

    /* allocate per-core metadata array */
    struct per_core_meta *per_core_meta_array = NULL;
    per_core_meta_array = (struct per_core_meta*)rte_malloc(NULL, 
        sizeof(struct per_core_meta)*sc_config->nb_used_cores, 0);
    if(unlikely(!per_core_meta_array)){
        SC_ERROR_DETAILS("failed to rte_malloc memory for per_core_meta array");
        return SC_ERROR_MEMORY;
    }
    memset(per_core_meta_array, 0, sizeof(struct per_core_meta)*sc_config->nb_used_cores);
    sc_config->per_core_meta = per_core_meta_array;

    /* initialize pthread barrier */
    pthread_barrier_init(&sc_config->pthread_barrier, NULL, sc_config->nb_used_cores);

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

    /* Hook Point: Worker All Exit */
    if(sc_config->app_config->worker_all_exit(sc_config) != SC_SUCCESS){
        SC_THREAD_WARNING("error occured while executing all exit callbacks");
    }

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