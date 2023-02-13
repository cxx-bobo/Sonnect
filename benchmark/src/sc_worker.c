#include "sc_global.h"
#include "sc_worker.h"

volatile bool force_quit;

/*!
 * \brief   initialize worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_worker(struct sc_config *sc_config){
    force_quit = false;
}

/*!
 * \brief   launch worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int launch_worker(struct sc_config *sc_config){

    rte_eal_mp_wait_lcore();
}

/*!
 * \brief   launch worker threads
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int launch_worker_async(struct sc_config *sc_config){

}