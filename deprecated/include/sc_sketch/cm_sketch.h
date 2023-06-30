#ifndef _SC_CM_SKETCH_H_
#define _SC_CM_SKETCH_H_

#if defined(SC_HAS_DOCA)
    #include "sc_doca.hpp"
    #include "sc_doca_utils/doca_utils.hpp"
#endif

int __cm_update(const char* key, struct sc_config *sc_config);
int __cm_query(const char* key, void *result, struct sc_config *sc_config);
int __cm_clean(struct sc_config *sc_config);
int __cm_record(const char* key, struct sc_config *sc_config);
int __cm_evaluate(struct sc_config *sc_config);

#endif

