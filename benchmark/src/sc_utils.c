#include "sc_global.h"
#include "sc_utils.h"

static char* _del_left_trim(char *str);
static char* _del_both_trim(char *str);
static int _parse_kv_pair(char* key, char *value, struct sc_config* sc_config);
static int _atoui_16(char *in, uint16_t *out);
static void _del_change_line(char *str);

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
                    printf("key %s without a value inside configuration file\n", key);
                    continue;
                }
                if(_parse_kv_pair(key, value, sc_config) != SC_SUCCESS){
                    printf("something is wrong within the configuration file\n");
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
    
    /* config: used device */
    if(!strcmp(key, "port_mac")){
        char *delim = ",";
        char *p, *port_mac;

        for(;;){
            if(sc_config->nb_ports == 0)
                p = strtok(value, delim);
            else
                p = strtok(NULL, delim);
            
            if (!p) goto exit;

            p = _del_both_trim(p);
            _del_change_line(p);

            port_mac = (char*)malloc(strlen(p)+1);
            if(!port_mac){
                printf("Failed to allocate memory for port_mac\n");
                result = SC_ERROR_MEMORY;
                goto free_dev_src;
            }
            memset(port_mac, 0, strlen(p)+1);

            strcpy(port_mac, p);
            sc_config->port_mac[sc_config->nb_ports] = port_mac;
            sc_config->nb_ports += 1;
        }

        goto exit;

free_dev_src:
        for(i=0; i<sc_config->nb_ports; i++) free(sc_config->port_mac[i]);
        sc_config->nb_ports = 0;
    }

    /* config: number of RX rings per port */
    if(!strcmp(key, "nb_rx_rings_per_port")){
        uint16_t nb_rings;
        value = _del_both_trim(value);
        _del_change_line(value);
        if(_atoui_16(value, &nb_rings) != SC_SUCCESS) 
            goto invalid_nb_rx_rings_per_port;
        if(nb_rings <= 0)
            goto invalid_nb_rx_rings_per_port;

        sc_config->nb_rx_rings_per_port = nb_rings;
        goto exit;

invalid_nb_rx_rings_per_port:
        printf("invalid configuration nb_rx_rings_per_port\n");
        result = SC_ERROR_INPUT;
    }

    /* config: number of TX rings per port */
    if(!strcmp(key, "nb_tx_rings_per_port")){
        uint16_t nb_rings;
        value = _del_both_trim(value);
        _del_change_line(value);
        if(_atoui_16(value, &nb_rings) != SC_SUCCESS) 
            goto invalid_nb_tx_rings_per_port;
        if(nb_rings <= 0)
            goto invalid_nb_tx_rings_per_port;

        sc_config->nb_tx_rings_per_port = nb_rings;
        goto exit;

invalid_nb_tx_rings_per_port:
        printf("invalid configuration nb_tx_rings_per_port\n");
        result = SC_ERROR_INPUT;
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
            printf("invalid configuration enable_promiscuous\n");
            result = SC_ERROR_INPUT;
        }
    }

exit:
    return result;
}


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