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

/*!
 * \brief all return status
 */
enum {
    SC_SUCCESS = 0,
    SC_ERROR_MEMORY,
    SC_ERROR_NOT_EXIST,
    SC_ERROR_INTERNAL,
    SC_ERROR_INPUT,
};

/* core operation */
int stick_this_thread_to_core(uint32_t core_id);
int check_core_id(uint32_t core_id);

/* file operation */
int parse_config(FILE* fp, struct sc_config* sc_config);

#endif