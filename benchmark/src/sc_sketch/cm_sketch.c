#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"
#include "sc_sketch/sketch.h"
#include "sc_sketch/cm_sketch.h"
#include "sc_sketch/spooky-c.h"

/*!
 * \brief   udpate cm sketch
 * \param   key         the hash key for the processed packet
 * \param   sc_config   the global configuration
 * \return  zero for successfully updating
 */
int __cm_update(const char* key, struct sc_config *sc_config){
    int i;
    uint32_t hash_result;
    uint32_t cm_nb_rows = INTERNAL_CONF(sc_config)->cm_nb_rows;
    uint32_t cm_nb_counters_per_row = INTERNAL_CONF(sc_config)->cm_nb_counters_per_row;
    rte_spinlock_t *lock = &(INTERNAL_CONF(sc_config)->cm_sketch->lock);
    counter_t *counters = INTERNAL_CONF(sc_config)->cm_sketch->counters;

    for(i=0; i<cm_nb_rows; i++){
        hash_result = spooky_hash32(key, TUPLE_KEY_LENGTH, INTERNAL_CONF(sc_config)->cm_sketch->hash_seeds[i]);
        hash_result %= cm_nb_counters_per_row;
        SC_THREAD_LOG("hash result: %u", hash_result);
        rte_spinlock_lock(lock);
        counters[i*cm_nb_counters_per_row + hash_result] += 1;
        rte_spinlock_unlock(lock);
    }

    return SC_SUCCESS;
}

/*!
 * \brief   query cm sketch
 * \param   key         the hash key for the processed packet
 * \param   result      query result 
 * \param   sc_config   the global configuration
 * \return  zero for successfully querying
 */
int __cm_query(const char* key, void *result, struct sc_config *sc_config){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   clean cm sketch
 * \return  zero for successfully querying
 */
int __cm_clean(struct sc_config *sc_config){
    int i;
    rte_spinlock_t *lock = &(INTERNAL_CONF(sc_config)->cm_sketch->lock);
    counter_t *counters = INTERNAL_CONF(sc_config)->cm_sketch->counters;
    uint32_t cm_nb_rows = INTERNAL_CONF(sc_config)->cm_nb_rows;
    uint32_t cm_nb_counters_per_row = INTERNAL_CONF(sc_config)->cm_nb_counters_per_row;

    rte_spinlock_lock(lock);
    for(i=0; i<cm_nb_rows*cm_nb_counters_per_row; i++){ counters[i] = 0; }
    rte_spinlock_unlock(lock);
    
    return SC_SUCCESS;
}