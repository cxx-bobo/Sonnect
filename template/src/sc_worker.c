#include <unistd.h>

#include "sc_global.h"
#include "sc_worker.h"
#include "sc_utils.h"

volatile bool force_quit;

/*!
 * \brief   function that execute on each lcore threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully executed
 */
int _worker_loop(void* param){
    struct sc_config *sc_config = (struct sc_config*)param;

    while(!force_quit){
        sleep(1);
        printf("printf from lcore %u\n", rte_lcore_id());
    }

    return SC_SUCCESS;
}

/*!
 * \brief   initialize application
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_app(struct sc_config *sc_config){
    // TODO
    return SC_SUCCESS;
}

/*!
 * \brief   initialize worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_worker(struct sc_config *sc_config){
    force_quit = false;
    return SC_SUCCESS;
}

/*!
 * \brief   launch worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int launch_worker(struct sc_config *sc_config){
    launch_worker_async(sc_config);
    rte_eal_mp_wait_lcore();
    return SC_SUCCESS;
}

/*!
 * \brief   launch worker threads (async)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int launch_worker_async(struct sc_config *sc_config){
    rte_eal_mp_remote_launch(_worker_loop, (void*)sc_config, CALL_MAIN);
    return SC_SUCCESS;
}