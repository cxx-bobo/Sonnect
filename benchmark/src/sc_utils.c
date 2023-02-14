#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"

static char* _del_left_trim(char *str);
static char* _del_both_trim(char *str);
static int _parse_kv_pair(char* key, char *value, struct sc_config* sc_config);
static int _atoui_16(char *in, uint16_t *out);
static int _atoui_32(char *in, uint32_t *out);
static void _del_change_line(char *str);

/* ==================== core operation ==================== */

/*!
 * \brief   stick the thread to a particular core
 * \param   core_id     the index of the core
 * \return  zero for successfully sticking
 */
int stick_this_thread_to_core(uint32_t core_id) {
    if(check_core_id(core_id) != SC_SUCCESS){
        SC_ERROR("failed to stick current thread to core %u", core_id);
        return SC_ERROR_INPUT;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

/*!
 * \brief   check whether the index of the core exceed physical range
 * \param   core_id     the index of the core
 * \return  zero for successfully checking
 */
int check_core_id(uint32_t core_id){
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id < 0 || core_id >= num_cores){
        SC_ERROR_DETAILS("given core index (%u) exceed phiscal range (max: %d)", core_id, num_cores)
        return SC_ERROR_INPUT;
    }
    return SC_SUCCESS;
}

/* ======================================================== */


/* ==================== file operation ==================== */

/*!
 * \brief   parse configuration file
 * \param   fp          handler of configuration file
 * \param   sc_config   the global configuration
 * \return  zero for successfully parsing
 */
int parse_config(FILE* fp, struct sc_config* sc_config){
    char buf[64];
    char s[64];
    char* delim = "=";
    char ch;
    char *p, *key, *value;

    while (!feof(fp)) {
        if  ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
            strcpy(s, p);
            /* skip empty line and comment line */
            ch=_del_left_trim(s)[0];
            if  (ch ==  '#'  || isblank(ch) || ch== '\n' )
                continue;
            /* split string based on given delim */
            key = strtok(s, delim);

            if (key) {
                key = _del_both_trim(key);
                value = strtok(NULL, delim);
                if(!value){
                    SC_WARNING_DETAILS("key %s without a value inside configuration file\n", key);
                    continue;
                }
                if(_parse_kv_pair(key, value, sc_config) != SC_SUCCESS){
                    SC_ERROR("something is wrong within the configuration file\n");
                    return SC_ERROR_INTERNAL;
                }
            }
        }
    }

    return SC_SUCCESS;
}

/*!
 * \brief   parse config key-value pair
 * \param   key         the key of the config pair
 * \param   value       the value of the config pair
 * \param   sc_config   the global configuration
 * \return  zero for successfully parsing
 */
static int _parse_kv_pair(char* key, char *value, struct sc_config* sc_config){
    int i, result = SC_SUCCESS;
    uint16_t nb_ports = 0;
    
    /* config: used device */
    if(!strcmp(key, "port_mac")){
        char *delim = ",";
        char *p, *port_mac;

        for(;;){
            if(nb_ports == 0)
                p = strtok(value, delim);
            else
                p = strtok(NULL, delim);
            
            if (!p) break;

            p = _del_both_trim(p);
            _del_change_line(p);

            port_mac = (char*)malloc(strlen(p)+1);
            if(!port_mac){
                SC_ERROR_DETAILS("Failed to allocate memory for port_mac\n");
                result = SC_ERROR_MEMORY;
                goto free_dev_src;
            }
            memset(port_mac, 0, strlen(p)+1);

            strcpy(port_mac, p);
            sc_config->port_mac[nb_ports] = port_mac;
            nb_ports += 1;
        }

        goto exit;

free_dev_src:
        for(i=0; i<nb_ports; i++) free(sc_config->port_mac[i]);
    }

    /* config: number of RX rings per port */
    if(!strcmp(key, "nb_rx_rings_per_port")){
        uint16_t nb_rings;
        value = _del_both_trim(value);
        _del_change_line(value);
        if(_atoui_16(value, &nb_rings) != SC_SUCCESS) {
            result = SC_ERROR_INPUT;
            goto invalid_nb_rx_rings_per_port;
        }
            
        if(nb_rings <= 0 || nb_rings > SC_MAX_NB_QUEUE_PER_PORT) {
            result = SC_ERROR_INPUT;
            goto invalid_nb_rx_rings_per_port;
        }

        sc_config->nb_rx_rings_per_port = nb_rings;
        goto exit;

invalid_nb_rx_rings_per_port:
        SC_ERROR_DETAILS("invalid configuration nb_rx_rings_per_port\n");
    }

    /* config: number of TX rings per port */
    if(!strcmp(key, "nb_tx_rings_per_port")){
        uint16_t nb_rings;
        value = _del_both_trim(value);
        _del_change_line(value);
        if (_atoui_16(value, &nb_rings) != SC_SUCCESS) {
            result = SC_ERROR_INPUT;
            goto invalid_nb_tx_rings_per_port;
        }
            
        if(nb_rings == 0 || nb_rings > SC_MAX_NB_QUEUE_PER_PORT) {
            result = SC_ERROR_INPUT;
            goto invalid_nb_tx_rings_per_port;
        }

        sc_config->nb_tx_rings_per_port = nb_rings;
        goto exit;

invalid_nb_tx_rings_per_port:
        SC_ERROR_DETAILS("invalid configuration nb_tx_rings_per_port\n");
    }

    /* config: whether to enable promiscuous mode */
    if(!strcmp(key, "enable_promiscuous")){
        value = _del_both_trim(value);
        _del_change_line(value);
        if (!strcmp(value, "true")){
            sc_config->enable_promiscuous = true;
        } else if (!strcmp(value, "false")){
            sc_config->enable_promiscuous = false;
        } else {
            result = SC_ERROR_INPUT;
            goto invalid_enable_promiscuous;
        }

        goto exit;

invalid_enable_promiscuous:
        SC_ERROR_DETAILS("invalid configuration enable_promiscuous\n");
    }

    /* config: number of cores to used */
    if(!strcmp(key, "used_core_ids")){
        uint16_t nb_used_cores = 0;
        uint32_t core_id = 0;
        char *delim = ",";
        char *core_id_str;

        value = _del_both_trim(value);
        _del_change_line(value);
        
        for(;;){
            if(nb_used_cores == 0)
                core_id_str = strtok(value, delim);
            else
                core_id_str = strtok(NULL, delim);
            
            if (!core_id_str) break;

            core_id_str = _del_both_trim(core_id_str);
            _del_change_line(core_id_str);

            if (_atoui_32(core_id_str, &core_id) != SC_SUCCESS) {
                result = SC_ERROR_INPUT;
                goto invalid_used_cores;
            }

            if (core_id > SC_MAX_NB_CORES) {
                result = SC_ERROR_INPUT;
                goto invalid_used_cores;
            }

            sc_config->core_ids[nb_used_cores] = core_id;
            nb_used_cores += 1;
        }

        sc_config->nb_used_cores = nb_used_cores;
        goto exit;

invalid_used_cores:
        SC_ERROR_DETAILS("invalid configuration used_cores\n");
    }

    /* config: number of memory channels per socket */
    if(!strcmp(key, "nb_memory_channels_per_socket")){
        uint16_t nb_memory_channels_per_socket;
        value = _del_both_trim(value);
        _del_change_line(value);
        if (_atoui_16(value, &nb_memory_channels_per_socket) != SC_SUCCESS) {
            result = SC_ERROR_INPUT;
            goto invalid_nb_memory_channels_per_socket;
        }

        sc_config->nb_memory_channels_per_socket = nb_memory_channels_per_socket;
        goto exit;

invalid_nb_memory_channels_per_socket:
        SC_ERROR_DETAILS("invalid configuration nb_memory_channels_per_socket\n");
    }

    /* config: the core for logging */
    if(!strcmp(key, "log_core_id")){
        uint32_t log_core_id;
        value = _del_both_trim(value);
        _del_change_line(value);
        if (_atoui_32(value, &log_core_id) != SC_SUCCESS) {
            result = SC_ERROR_INPUT;
            goto invalid_log_core_id;
        }

        sc_config->log_core_id = log_core_id;
        goto exit;

invalid_log_core_id:
        SC_ERROR_DETAILS("invalid configuration log_core_id\n");
    }

exit:
    return result;
}

/* ======================================================== */



/* ==================== string operation ==================== */


/*!
 * \brief   delete the space on the left-side of the string
 * \param   str the target string
 * \return  the processed string
 */
static char* _del_left_trim(char *str) {
    assert(str != NULL);
    for  (;*str !=  '\0'  && isblank(*str) ; ++str);
    return  str;
}

/*!
 * \brief   delete the space on the both sides of the string
 * \param   str the target string
 * \return  the processed string
 */
static char* _del_both_trim(char *str) {
    char *p;
    char * sz_output;
    sz_output = _del_left_trim(str);
    for  (p=sz_output+strlen(sz_output)-1; p>=sz_output && isblank(*p); --p);
    *(++p) =  '\0' ;
    
    return  sz_output;
}

/*!
 * \brief   delete the '\n' at the end of string
 * \param   str the target string
 */
static void _del_change_line(char *str){
    if(str[strlen(str)-1] == '\n') str[strlen(str)-1] = '\0';
}

/*!
 * \brief   convery string to uint16_t
 * \param   in  given string
 * \param   out output uint16_t
 * \return  zero for successfully parsing
 */
static int _atoui_16(char *in, uint16_t *out){
    char *p;
    for(p = in; *p; p++)
        if (*p > '9' || *p < '0') return SC_ERROR_INPUT;
    *out = strtoul(in, NULL, 10);
    return SC_SUCCESS;
}

/*!
 * \brief   convery string to uint32_t
 * \param   in  given string
 * \param   out output uint16_t
 * \return  zero for successfully parsing
 */
static int _atoui_32(char *in, uint32_t *out){
    char *p;
    for(p = in; *p; p++)
        if (*p > '9' || *p < '0') return SC_ERROR_INPUT;
    *out = strtoul(in, NULL, 10);
    return SC_SUCCESS;
}

/* ========================================================== */