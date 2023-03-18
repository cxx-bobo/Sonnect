#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"

/* ==================== core operation ==================== */

/*!
 * \brief   stick the thread to a particular core
 * \param   core_id     the index of the core
 * \return  zero for successfully sticking
 */
int sc_util_stick_this_thread_to_core(uint32_t core_id) {
    if(sc_util_check_core_id(core_id) != SC_SUCCESS){
        SC_ERROR("failed to stick current thread to core %u", core_id);
        return SC_ERROR_INVALID_VALUE;
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
int sc_util_check_core_id(uint32_t core_id){
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id < 0 || core_id >= num_cores){
        SC_ERROR_DETAILS("given core index (%u) exceed phiscal range (max: %d)", core_id, num_cores)
        return SC_ERROR_INVALID_VALUE;
    }
    return SC_SUCCESS;
}

/* ======================================================== */



/* ==================== port operation ==================== */

/*!
 * \brief   obtain port id by MAC address
 * \param   sc_config       the global configuration
 * \param   port_mac        mac address of the port
 * \param   port_id         result port id
 * \return  zero for successfully searching
 */
int sc_util_get_port_id_by_mac(struct sc_config *sc_config, char* port_mac, uint32_t *port_id){
    int i, ret, result = SC_ERROR_NOT_EXIST;
    struct rte_ether_addr mac;
    char ebuf[RTE_ETHER_ADDR_FMT_SIZE];

    for(i=0; i<sc_config->nb_used_ports; i++){
        /* get mac address */
        ret = rte_eth_macaddr_get(sc_config->port_ids[i], &mac);
        if (ret == 0) {
            rte_ether_format_addr(ebuf, sizeof(ebuf), &mac);

            /* compare mac address */
            if(!strcmp(ebuf, port_mac)){
                *port_id = sc_config->port_ids[i];
                result = SC_SUCCESS;
                break;
            }
        } else {
            result = SC_ERROR_INTERNAL;
            break;
        }
    }
    
    return result;
}

/*!
 * \brief   obtain MAC address by port id
 * \param   sc_config       the global configuration
 * \param   port_id         given port id
 * \param   port_mac        result mac address of the port
 * \return  zero for successfully searching
 */
int sc_util_get_mac_by_port_id(struct sc_config *sc_config, uint32_t port_id, char* port_mac){
    int ret, result = SC_SUCCESS;
    char ebuf[RTE_ETHER_ADDR_FMT_SIZE];

    /* get mac */
    ret = rte_eth_macaddr_get(port_id, ebuf);
    if (ret == 0) {
        memcpy(port_mac, ebuf, RTE_ETHER_ADDR_FMT_SIZE);
    } else {
        result = SC_ERROR_NOT_EXIST;
    }

    return result;
}

/*!
 * \brief   obtain port index by given logical port index
 * \param   sc_config           the global configuration
 * \param   logical_port_id     given logical port id
 * \param   port_id             result port id
 * \return  zero for successfully searching
 */
int sc_util_get_port_id_by_logical_port_id(struct sc_config *sc_config,  uint32_t logical_port_id, uint32_t *port_id){
    if(logical_port_id >= sc_config->nb_used_ports){
        SC_ERROR_DETAILS("given logical id (%u) is larger than overall number of initialized port (%u)",
            logical_port_id, sc_config->nb_used_ports);
        return SC_ERROR_INVALID_VALUE;
    }
    *port_id = sc_config->sc_port[logical_port_id].port_id;
    return SC_SUCCESS;
}

/*!
 * \brief   obtain port index by given logical port index
 * \param   sc_config           the global configuration
 * \param   port_id             given port id
 * \param   logical_port_id     result logical port id
 * \return  zero for successfully searching
 */
int sc_util_get_logical_port_id_by_port_id(struct sc_config *sc_config, uint32_t port_id, uint32_t *logical_port_id){
    int i, ret, result = SC_ERROR_NOT_EXIST;

    for(i=0; i<sc_config->nb_used_ports; i++){
        if (sc_config->sc_port[i].port_id == port_id) {
            *logical_port_id = sc_config->sc_port[i].logical_port_id;
            result = SC_SUCCESS;
            break;
        }
    }
    
    return result;
}

/* ======================================================== */



/* ==================== file operation ==================== */

/*!
 * \brief   parse configuration file
 * \param   fp              handler of configuration file
 * \param   sc_config       the global configuration
 * \param   parse_kv_pair   function for parsing specific key-value pair
 * \return  zero for successfully parsing
 */
int sc_util_parse_config(
        FILE* fp, struct sc_config* sc_config, 
        int (*parse_kv_pair)(char* key, char *value, struct sc_config* sc_config)){
    char buf[512];
    char s[64];
    char* delim = "=";
    char ch;
    char *p, *key, *value;

    while (!feof(fp)) {
        if  ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
            strcpy(s, p);
            /* skip empty line and comment line */
            ch=sc_util_del_left_trim(s)[0];
            if  (ch ==  '#'  || isblank(ch) || ch== '\n' )
                continue;
            /* split string based on given delim */
            key = strtok(s, delim);

            if (key) {
                key = sc_util_del_both_trim(key);
                value = strtok(NULL, delim);
                if(!value){
                    SC_WARNING_DETAILS("key %s without a value inside configuration file\n", key);
                    continue;
                }
                if(parse_kv_pair(key, value, sc_config) != SC_SUCCESS){
                    SC_ERROR("something is wrong within the configuration file\n");
                    return SC_ERROR_INTERNAL;
                }
            }
        }
    }

    return SC_SUCCESS;
}

/* ======================================================== */



/* ==================== string operation ==================== */


/*!
 * \brief   delete the space on the left-side of the string
 * \param   str the target string
 * \return  the processed string
 */
char* sc_util_del_left_trim(char *str) {
    assert(str != NULL);
    for  (;*str !=  '\0'  && isblank(*str) ; ++str);
    return  str;
}

/*!
 * \brief   delete the space on the both sides of the string
 * \param   str the target string
 * \return  the processed string
 */
char* sc_util_del_both_trim(char *str) {
    char *p;
    char * sz_output;
    sz_output = sc_util_del_left_trim(str);
    for  (p=sz_output+strlen(sz_output)-1; p>=sz_output && isblank(*p); --p);
    *(++p) =  '\0' ;
    
    return  sz_output;
}

/*!
 * \brief   delete the '\n' at the end of string
 * \param   str the target string
 */
void sc_util_del_change_line(char *str){
    if(str[strlen(str)-1] == '\n') str[strlen(str)-1] = '\0';
}

/*!
 * \brief   convery string to uint16_t
 * \param   in  given string
 * \param   out output uint16_t
 * \return  zero for successfully parsing
 */
int sc_util_atoui_16(char *in, uint16_t *out){
    char *p;
    for(p = in; *p; p++)
        if (*p > '9' || *p < '0') return SC_ERROR_INVALID_VALUE;
    *out = strtoul(in, NULL, 10);
    return SC_SUCCESS;
}

/*!
 * \brief   convery string to uint32_t
 * \param   in  given string
 * \param   out output uint32_t
 * \return  zero for successfully parsing
 */
int sc_util_atoui_32(char *in, uint32_t *out){
    char *p;
    for(p = in; *p; p++)
        if (*p > '9' || *p < '0') return SC_ERROR_INVALID_VALUE;
    *out = strtoul(in, NULL, 10);
    return SC_SUCCESS;
}

/*!
 * \brief   convery string to uint64_t
 * \param   in  given string
 * \param   out output uint64_t
 * \return  zero for successfully parsing
 */
int sc_util_atoui_64(char *in, uint64_t *out){
    char *p;
    for(p = in; *p; p++)
        if (*p > '9' || *p < '0') return SC_ERROR_INVALID_VALUE;
    *out = strtoul(in, NULL, 10);
    return SC_SUCCESS;
}

/* ========================================================== */



/* ==================== random operation ==================== */

/*!
 * \brief   generate random unsigned 64-bits integer
 * \return  the generated value
 */
uint64_t sc_util_random_unsigned_int64(){
    uint16_t i;
    uint64_t r=0;
    for(i=0; i<64; i++)
        r = r*2+rand()%2;
    return r;
}

/*!
 * \brief   generate random unsigned 32-bits integer
 * \return  the generated value
 */
uint32_t sc_util_random_unsigned_int32(){
    uint16_t i;
    uint32_t r=0;
    for(i=0; i<32; i++)
        r = r*2+rand()%2;
    return r;
}

/*!
 * \brief   generate random unsigned 16-bits integer
 * \return  the generated value
 */
uint16_t sc_util_random_unsigned_int16(){
    uint16_t i;
    uint16_t r=0;
    for(i=0; i<16; i++)
        r = r*2+rand()%2;
    return r;
}

/*!
 * \brief   generate random unsigned 8-bits integer
 * \return  the generated value
 */
uint8_t sc_util_random_unsigned_int8(){
    uint16_t i;
    uint8_t r=0;
    for(i=0; i<16; i++)
        r = r*2+rand()%2;
    return r;
}


/* ========================================================== */



