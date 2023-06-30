#include "sc_global.hpp"
#include "sc_template/template.h"
#include "sc_utils.hpp"
#include "sc_log.hpp"

/*!
 * \brief   parse application-specific key-value configuration pair
 * \param   key         the key of the config pair
 * \param   value       the value of the config pair
 * \param   sc_config   the global configuration
 * \return  zero for successfully parsing
 */
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config){
    int result = SC_ERROR_NOT_IMPLEMENTED;
    SC_WARNING_DETAILS("_parse_app_kv_pair not implemented");
    return result;
}

/*!
 * \brief   callback while entering application
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _process_enter(struct sc_config *sc_config){
    SC_WARNING_DETAILS("_process_enter not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback for processing packet
 * \param   pkt             the received packet
 * \param   sc_config       the global configuration
 * \param   recv_port_id    the index of the port that received this packet
 * \param   fwd_port_id     specified the forward port index if need to forward packet
 * \param   need_forward    indicate whether need to forward packet, default to be false
 * \return  zero for successfully processing
 */
int _process_pkt(struct rte_mbuf **pkt, uint64_t nb_recv_pkts, struct sc_config *sc_config, uint16_t queue_id, uint16_t recv_port_id, uint16_t *fwd_port_id, uint64_t *nb_fwd_pkts){
    SC_WARNING_DETAILS("_process_pkt not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback for processing packets to be dropped
 * \param   sc_config       the global configuration
 * \param   pkt             the packets to be dropped
 * \param   nb_drop_pkts    number of packets to be dropped
 * \return  zero for successfully processing
 */
int _process_pkt_drop(struct sc_config *sc_config, struct rte_mbuf **pkt, uint64_t nb_drop_pkts){
    SC_WARNING_DETAILS("_process_pkt not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
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
    SC_WARNING_DETAILS("_process_exit not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   callback while all worker thread exit
 * \param   sc_config   the global configuration
 * \return  zero for successfully executing
 */
int _all_exit(struct sc_config *sc_config){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   initialize application (internal)
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int _init_app(struct sc_config *sc_config){
    int i;
    
    for(i=0; i<sc_config->nb_used_cores; i++){
        /* TODO: dispatch processing functions here */
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_client_func = _process_client;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_enter_func = _process_enter;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_exit_func = _process_exit;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_pkt_func  = _process_pkt;
        PER_CORE_WORKER_FUNC_BY_CORE_ID(sc_config, i).process_pkt_drop_func = _process_pkt_drop;
    }

    SC_WARNING_DETAILS("_init_app not implemented");
    return SC_ERROR_NOT_IMPLEMENTED;
}