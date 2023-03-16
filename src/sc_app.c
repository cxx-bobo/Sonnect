#include "sc_global.h"
#include "sc_utils.h"
#include "sc_app.h"
#include "sc_log.h"

/*!
 * \brief   initialize application
 * \param   sc_config       the global configuration
 * \param   app_conf_path   path to the configuration file of currernt application
 * \return  zero for successfully initialization
 */
int init_app(struct sc_config *sc_config, const char *app_conf_path){
    FILE* fp = NULL;

    /* allocate per-core metadata */
    struct _per_core_app_meta *per_core_app_meta = NULL;
    per_core_app_meta = (struct _per_core_app_meta*)rte_malloc(NULL, sizeof(struct _per_core_app_meta)*sc_config->nb_used_cores, 0);
    if(unlikely(!per_core_app_meta)){
        SC_ERROR_DETAILS("failed to rte_malloc memory for per_core_app_meta");
        return SC_ERROR_MEMORY;
    }
    memset(per_core_app_meta, 0, sizeof(struct _per_core_app_meta)*sc_config->nb_used_cores);
    sc_config->per_core_app_meta = per_core_app_meta;

    /* allocate internal config */
    struct _internal_config *_internal_config = (struct _internal_config*)malloc(sizeof(struct _internal_config));
    if(unlikely(!_internal_config)){
        SC_ERROR_DETAILS("failed to allocate memory for internal_config");
        return SC_ERROR_MEMORY;
    }
    sc_config->app_config->internal_config = _internal_config;

    /* specify pkt entering callback function */
    sc_config->app_config->process_enter = _process_enter;

    /* specify pkt processing callback function (server mode) */
    sc_config->app_config->process_pkt = _process_pkt;

    /* specify client callback function (client mode) */
    sc_config->app_config->process_client = _process_client;

    /* specify pkt exiting callback function */
    sc_config->app_config->process_exit = _process_exit;

    /* specify the all exit callback function */
    sc_config->app_config->all_exit = _all_exit;

    /* open application configuration file */
    fp = fopen(app_conf_path, "r");
    if(!fp){
        SC_ERROR("failed to open the application configuration file from %s: %s\n", app_conf_path, strerror(errno));
        return SC_ERROR_NOT_EXIST;
    }

    /* parse configuration file */
    if(sc_util_parse_config(fp, sc_config, _parse_app_kv_pair) != SC_SUCCESS){
        SC_ERROR("failed to parse the application configuration file, exit\n");
        return SC_ERROR_INVALID_VALUE;
    }
    
    /* initialize application (internal) */
    return _init_app(sc_config);
}