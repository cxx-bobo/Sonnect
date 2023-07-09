#ifndef _SC_TEMPLATE_H_
#define _SC_TEMPLATE_H_

#include "sc_app.hpp"

struct _per_core_app_meta {
    int something;
};

/* definition of internal config */
struct _internal_config {
    int something;
};

int _init_app(struct sc_config *sc_config);
int _parse_app_kv_pair(char* key, char *value, struct sc_config* sc_config);
int _worker_all_exit(struct sc_config *sc_config);

#endif