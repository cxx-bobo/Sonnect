#include "sc_global.h"
#include "sc_echo_server/echo_server.h"
#include "sc_utils.h"
#include "sc_log.h"

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    return SC_SUCCESS;
}

/*!
 * \brief   parse application-specific key-value configuration pair
 * \param   key         the key of the config pair
 * \param   value       the value of the config pair
 * \param   sc_config   the global configuration
 * \return  zero for successfully parsing
 */
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config){
    int result = SC_SUCCESS;
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter(struct sc_config *sc_config){
    return SC_SUCCESS;
}

/*!
 * \brief   callback for processing packet
 * \param   pkt         the received packet
 * \param   sc_config   the global configuration
 * \param   fwd_port_id     specified the forward port index if need to forward packet
 * \param   need_forward    indicate whether need to forward packet, default to be false
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf *pkt, struct sc_config *sc_config, uint16_t *fwd_port_id, bool *need_forward){
    /* count */
    PER_CORE_APP_META(sc_config).nb_forward_pkt += 1;

    /* set as forwarded, default to port 0 */
    *need_forward = true;
    *fwd_port_id  = 0;

    return SC_SUCCESS;
}

/*!
 * \brief   callback for client logic
 * \param   sc_config       the global configuration
 * \param   queue_id        the index of the queue for current core to tx/rx packet
 * \param   ready_to_exit   indicator for exiting worker loop
 * \return  zero for successfully executing
 */
int _process_client(struct sc_config *sc_config, uint16_t queue_id, bool *ready_to_exit){
    SC_WARNING_DETAILS("_process_client not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback while exiting application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_exit(struct sc_config *sc_config){
    SC_THREAD_LOG("forward %u packets in total", PER_CORE_APP_META(sc_config).nb_forward_pkt);
    return SC_SUCCESS;
}