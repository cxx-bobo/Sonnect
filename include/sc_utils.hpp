#ifndef _SC_UTILS_H_
#define _SC_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <rte_ethdev.h>
#include <rte_ether.h>

#include "sc_global.hpp"
#include "sc_control_plane.hpp"

/*!
 * \brief all return status
 */
enum {
    SC_SUCCESS = 0,
    SC_ERROR_MEMORY,
    SC_ERROR_NOT_EXIST,
    SC_ERROR_INTERNAL,
    SC_ERROR_INVALID_VALUE,
    SC_ERROR_NOT_IMPLEMENTED,
    SC_ERROR_NOT_FINISHED   
};


/* core operation */
int sc_util_stick_this_thread_to_core(uint32_t core_id);
int sc_util_check_core_id(uint32_t core_id);
int sc_util_get_core_id_by_logical_core_id(struct sc_config *sc_config,  uint32_t logical_core_id, uint32_t *core_id);
int sc_util_get_logical_core_id_by_core_id(struct sc_config *sc_config, uint32_t core_id, uint32_t *logical_core_id);

/* port operation */
int sc_util_get_port_id_by_mac(struct sc_config *sc_config, char* port_mac, uint32_t *port_id);
int sc_util_get_mac_by_port_id(struct sc_config *sc_config, uint32_t port_id, char* port_mac);
int sc_util_get_port_id_by_logical_port_id(struct sc_config *sc_config,  uint32_t logical_port_id, uint32_t *port_id);
int sc_util_get_logical_port_id_by_port_id(struct sc_config *sc_config, uint32_t port_id, uint32_t *logical_port_id);

/* file operation */
int sc_util_parse_config(FILE* fp, struct sc_config* sc_config, 
    int (*parse_kv_pair)(char* key, char *value, struct sc_config* sc_config));

/* string operation */
#define XSTR(x) STR(x)
#define STR(x) #x
char* sc_util_del_left_trim(char *str);
char* sc_util_del_both_trim(char *str);
void sc_util_del_change_line(char *str);
int sc_util_atoui_16(char *in, uint16_t *out);
int sc_util_atoui_32(char *in, uint32_t *out);
int sc_util_atoui_64(char *in, uint64_t *out);
int sc_util_atolf(char *in, double *out);

/* random operation */
uint32_t sc_util_random_unsigned_int32();
uint64_t sc_util_random_unsigned_int64();
uint16_t sc_util_random_unsigned_int16();
uint8_t sc_util_random_unsigned_int8();

/* byte operation */
/*!
 * \brief   get the ith byte inside data
 * \param   data    the data to be masked
 * \param   i       the specified byte index
 * \return  extract byte
 */
template<typename data_type, typename index_type>
inline uint8_t sc_util_get_ith_byte(data_type data, index_type i){
    return ((data>>i))&0x000000ff;
}

template<typename data_type, typename index_type, uint32_t byte_size>
inline data_type _sc_util_bytes_to_number(const uint8_t *raw, uint32_t depth){
    return depth == byte_size - 1 ? raw[depth]
        : raw[depth] + _sc_util_bytes_to_number<data_type, index_type, byte_size>(raw, depth+1);
}

template<typename data_type, typename index_type, uint64_t byte_size>
inline data_type sc_util_bytes_to_number(const uint8_t *raw){
    return _sc_util_bytes_to_number<data_type, index_type, byte_size>(raw, 0);
}

#endif