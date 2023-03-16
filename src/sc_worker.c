#include "sc_global.h"
#include "sc_worker.h"
#include "sc_utils.h"
#include "sc_mbuf.h"
#include "sc_log.h"

extern volatile bool sc_force_quit;

/*!
 * \brief   initialize worker loop after enter it
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int __worker_loop_init(struct sc_config *sc_config) {
    int i;
#define MBUF_POOL_NAME_LEM 32
    char *mbuf_pool_name;
    struct rte_mempool *pktmbuf_pool;
    
    /* specify mbuf pool name for current thread */
    mbuf_pool_name = (char*)malloc(sizeof(char)*MBUF_POOL_NAME_LEM);
    if(unlikely(!mbuf_pool_name)){
        SC_ERROR_DETAILS("failed to allocate memory for mbuf_pool_name");
        return SC_ERROR_MEMORY;
    }
    sprintf(mbuf_pool_name, "mbuf_pool_core_%u", rte_lcore_id());
    PER_CORE_META(sc_config).mbuf_pool_name = mbuf_pool_name;

    /* allocate memory buffer pool for current thread */
    pktmbuf_pool = rte_pktmbuf_pool_create(
        /* name */ mbuf_pool_name, 
        /* n */ SC_NUM_PRIVATE_MBUFS_PER_CORE, 
        /* cache_size */ MEMPOOL_CACHE_SIZE, 
        /* priv_size */ 0, 
        /* data_room_size */ RTE_MBUF_DEFAULT_BUF_SIZE, 
        /* socket_id */ rte_socket_id()
    );
    if (!pktmbuf_pool){
        SC_THREAD_ERROR_DETAILS("failed to allocate memory for mbuf pool: %s", rte_strerror(rte_errno));
        return SC_ERROR_MEMORY;
    }
    rte_pktmbuf_pool_init(pktmbuf_pool, NULL);
    PER_CORE_MBUF_POOL(sc_config) = pktmbuf_pool;
    
    return SC_SUCCESS;
}


/*!
 * \brief   function that execute on each lcore threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully executed
 */
int _worker_loop(void* param){
    int result = SC_SUCCESS;
    uint16_t i, j, queue_id, forward_port_id, nb_rx, nb_tx;
    int lcore_id_from_zero;
    bool need_forward = false;
    struct sc_config *sc_config = (struct sc_config*)param;

    /* record lcore id starts from 0 */
    lcore_id_from_zero = rte_lcore_index(rte_lcore_id());

    /* initialize woker loop */
    result = __worker_loop_init(sc_config);
    if(result != SC_SUCCESS){
        SC_THREAD_ERROR("failed to initialize worker loop");
        goto worker_exit;
    }

    #if defined(ROLE_SERVER)
        struct rte_mbuf *pkt[SC_MAX_PKT_BURST];
    #endif // ROLE_SERVER

    #if defined(ROLE_CLIENT)
        bool ready_to_exit = false;
    #endif // ROLE_CLIENT

    for(i=0; i<sc_config->nb_used_cores; i++){
        if(sc_config->core_ids[i] == rte_lcore_id()){
            queue_id = i; 
            SC_THREAD_LOG("polling on queue %u", queue_id);
            break;
        }
        if(i == sc_config->nb_used_cores-1){
            SC_THREAD_ERROR_DETAILS("unknown queue id for worker thread on lcore %u", rte_lcore_id());
            result = SC_ERROR_INTERNAL;
            goto worker_exit;
        }
    }

    /* record thread start time while test duration limitation is enabled */
    if(sc_config->enable_test_duration_limit && lcore_id_from_zero == 0){
        if(unlikely(-1 == gettimeofday(&sc_config->test_duration_start_time, NULL))){
            SC_THREAD_ERROR_DETAILS("failed to obtain start time");
            result = SC_ERROR_INTERNAL;
            goto worker_exit;
        }
    }

    /* Hook Point: Enter */
    if(sc_config->app_config->process_enter(sc_config) != SC_SUCCESS){
        SC_THREAD_WARNING("error occurs while executing enter callback\n");
    }
    
    while(!sc_force_quit){
        /* role: server */
        #if defined(ROLE_SERVER)
            for(i=0; i<sc_config->nb_used_ports; i++){
                nb_rx = rte_eth_rx_burst(i, queue_id, pkt, SC_MAX_PKT_BURST);
                
                if(nb_rx == 0) continue;
                
                // SC_THREAD_LOG("received %u ethernet frames", nb_rx);

                for(j=0; j<nb_rx; j++){
                    /* Hook Point: Packet Processing */
                    if(sc_config->app_config->process_pkt(pkt[j], sc_config, &forward_port_id, &need_forward) != SC_SUCCESS){
                        SC_THREAD_WARNING("failed to process the received frame");
                    }

                    // TODO: exist problems, if forward pkt is too many and too fast, this will failed
                    if(need_forward){
                        nb_tx = rte_eth_tx_burst(forward_port_id, queue_id, pkt+j, 1);
                        if(nb_tx == 0){
                            rte_pktmbuf_free(pkt[j]);
                            SC_THREAD_WARNING("failed to forward packet to queue %u on port %u",
                                queue_id, forward_port_id);
                        }
                        // SC_THREAD_LOG("send %d ethernet frames", nb_tx);
                    }

                    // reset need forward flag
                    need_forward = false;
                }
            }
        #endif // ROLE_SERVER

        /* role: client */
        #if defined(ROLE_CLIENT)
            /* Hook Point: Packet Preparing */
            if(sc_config->app_config->process_client(sc_config, queue_id, &ready_to_exit) != SC_SUCCESS){
                SC_THREAD_WARNING("error occured within the client process");
            }
            if(ready_to_exit){ break; }
        #endif // ROLE_CLIENT

        /* 
         * use core 0 to shutdown the application while test duration 
         * limitation is enabled and the limitation is reached
         */
        if(sc_config->enable_test_duration_limit && lcore_id_from_zero == 0){
            /* record current time */
            if(unlikely(-1 == gettimeofday(&sc_config->test_duration_end_time, NULL))){
                SC_THREAD_ERROR_DETAILS("failed to obtain end time");
                result = SC_ERROR_INTERNAL;
                goto worker_exit;
            }

            /* reach the duration limitation, quit all threads */
            if(sc_config->test_duration_end_time.tv_sec - sc_config->test_duration_start_time.tv_sec 
                >= sc_config->test_duration){
                    sc_force_quit = true;
            }
        }
    }

exit_callback:
    /* Hook Point: Exit */
    if(sc_config->app_config->process_exit(sc_config) != SC_SUCCESS){
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

    /* Hook Point: all exit */
    if(sc_config->app_config->all_exit(sc_config) != SC_SUCCESS){
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