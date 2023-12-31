#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_app.hpp"
#include "sc_control_plane.hpp"

/*!
 * \brief   initialize application
 * \param   sc_config       the global configuration
 * \param   app_conf_path   path to the configuration file of currernt application
 * \return  zero for successfully initialization
 */
int init_app(struct sc_config *sc_config, const char *app_conf_path){
    FILE* fp = NULL;

    /* allocate per-core application metadata */
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

    /* allocate per-core worker function array */
    struct per_core_worker_func *per_core_worker_funcs = NULL;
    per_core_worker_funcs = (struct per_core_worker_func*)rte_malloc(NULL, sizeof(struct per_core_worker_func)
                            *sc_config->nb_used_cores, 0);
    if(unlikely(!per_core_worker_funcs)){
        SC_ERROR_DETAILS("failed to rte_malloc memory for per_core_worker_funcs");
        return SC_ERROR_MEMORY;
    }
    memset(per_core_worker_funcs, 0, sizeof(struct per_core_worker_func)*sc_config->nb_used_cores);
    sc_config->per_core_worker_funcs = per_core_worker_funcs;

    /* allocate per-core control function array */
    struct per_core_control_func *per_core_control_funcs = NULL;
    per_core_control_funcs = (struct per_core_control_func*)rte_malloc(NULL, sizeof(struct per_core_control_func)
                            *sc_config->nb_used_cores, 0);
    if(unlikely(!per_core_control_funcs)){
        SC_ERROR_DETAILS("failed to rte_malloc memory for per_core_control_funcs");
        return SC_ERROR_MEMORY;
    }
    memset(per_core_control_funcs, 0, sizeof(struct per_core_control_func)*sc_config->nb_used_cores);
    sc_config->per_core_control_funcs = per_core_control_funcs;

    /* specify the all exit callback function */
    sc_config->app_config->worker_all_exit = _worker_all_exit;

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