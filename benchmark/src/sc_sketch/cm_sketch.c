#include "sc_global.h"
#include "sc_utils.h"
#include "sc_log.h"
#include "sc_sketch/sketch.h"
#include "sc_sketch/cm_sketch.h"
#include "sc_sketch/spooky-c.h"

/*!
 * \brief   udpate the sketch structre using a specific key
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

#if defined(MODE_LATENCY)
    struct timeval hash_start, hash_end;
    struct timeval update_start, update_end;
    struct timeval lock_start, lock_end;
#endif

    for(i=0; i<cm_nb_rows; i++){
    /* step 1: hashing */
#if defined(MODE_LATENCY)
        gettimeofday(&hash_start, NULL);
#endif
        hash_result = spooky_hash32(key, TUPLE_KEY_LENGTH, INTERNAL_CONF(sc_config)->cm_sketch->hash_seeds[i]);
        hash_result %= cm_nb_counters_per_row;
#if defined(MODE_LATENCY)
        gettimeofday(&hash_end, NULL);
        PER_CORE_META(sc_config).overall_hash.tv_usec
            += (hash_end.tv_sec - hash_start.tv_sec) * 1000000 
                + (hash_end.tv_usec - hash_start.tv_usec);
#endif
        SC_THREAD_LOG("hash result: %u", hash_result);
    
        /* step 2: require spin lock */
#if defined(MODE_LATENCY)
        gettimeofday(&lock_start, NULL);
#endif
        rte_spinlock_lock(lock);
#if defined(MODE_LATENCY)
        gettimeofday(&lock_end, NULL);
        PER_CORE_META(sc_config).overall_lock.tv_usec
            += (lock_end.tv_sec - lock_start.tv_sec) * 1000000 
                + (lock_end.tv_usec - lock_start.tv_usec);
#endif

        /* step 3: update counter */
#if defined(MODE_LATENCY)
        gettimeofday(&update_start, NULL);
#endif
        counters[i*cm_nb_counters_per_row + hash_result] += 1;
#if defined(MODE_LATENCY)
        gettimeofday(&update_end, NULL);
        PER_CORE_META(sc_config).overall_update.tv_usec
            += (update_end.tv_sec - update_start.tv_sec) * 1000000 
                + (update_end.tv_usec - update_start.tv_usec);
#endif

        /* step 4: expire spin lock */
#if defined(MODE_LATENCY)
        gettimeofday(&lock_start, NULL);
#endif
        rte_spinlock_unlock(lock);
#if defined(MODE_LATENCY)
        gettimeofday(&lock_end, NULL);
        PER_CORE_META(sc_config).overall_lock.tv_usec
            += (lock_end.tv_sec - lock_start.tv_sec) * 1000000 
                + (lock_end.tv_usec - lock_start.tv_usec);
#endif
    }

    return SC_SUCCESS;
}

/*!
 * \brief   query the sketch structre using a specific key
 * \param   key         the hash key for the processed packet
 * \param   result      query result 
 * \param   sc_config   the global configuration
 * \return  zero for successfully querying
 */
int __cm_query(const char* key, void *result, struct sc_config *sc_config){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   clean the sketch structre
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

/*!
 * \brief   record the actual value for a specific key
 * \return  zero for successfully recording
 */
int __cm_record(const char* key, struct sc_config *sc_config){
    return SC_ERROR_NOT_IMPLEMENTED;
}

/*!
 * \brief   clean cm sketch
 * \return  evaluate the throughput/latency/accuracy of the sketch
 */
int __cm_evaluate(struct sc_config *sc_config){
    /* output latency log */
#if defined(MODE_LATENCY)
    /* packet process */
    SC_THREAD_LOG("overall pkt process latency: %ld us",
        PER_CORE_META(sc_config).overall_pkt_process.tv_usec);
    SC_THREAD_LOG("average pkt process latency: %lf us/pkt",
        PER_CORE_META(sc_config).overall_pkt_process.tv_usec / PER_CORE_META(sc_config).nb_pkts);
    /* hash */
    SC_THREAD_LOG("overall hash latency: %ld us",
        PER_CORE_META(sc_config).overall_hash.tv_usec);
    SC_THREAD_LOG("average hash latency: %lf us/pkt",
        PER_CORE_META(sc_config).overall_hash.tv_usec / PER_CORE_META(sc_config).nb_pkts);
    /* lock */
    SC_THREAD_LOG("overall lock latency: %ld us",
        PER_CORE_META(sc_config).overall_lock.tv_usec);
    SC_THREAD_LOG("average lock latency: %lf us/pkt",
        PER_CORE_META(sc_config).overall_lock.tv_usec / PER_CORE_META(sc_config).nb_pkts);
    /* update */
    SC_THREAD_LOG("overall update latency: %ld us",
        PER_CORE_META(sc_config).overall_update.tv_usec);
    SC_THREAD_LOG("average update latency: %lf us/pkt",
        PER_CORE_META(sc_config).overall_update.tv_usec / PER_CORE_META(sc_config).nb_pkts);
#endif

    /* output throughput log */
#if defined(MODE_THROUGHPUT)

#endif

    return SC_SUCCESS;
}