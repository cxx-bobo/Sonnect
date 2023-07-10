#include "sc_global.hpp"
#include "sc_control_plane.hpp"
#include "sc_utils.hpp"
#include "sc_app.hpp"
#include "sc_utils/timestamp.hpp"

char current_time_str[128] = "UNKNOWN TIME";
pthread_mutex_t thread_log_mutex;
pthread_mutex_t timer_mutex;

/*!
 * \brief   function that execute on the logging thread
 * \param   args   (sc_config) the global configuration
 */
void* _control_loop(void *args){
    int result = SC_SUCCESS, func_result;
    uint32_t i, core_id;
    time_t time_ptr;
    struct tm *tmp_ptr = NULL;
    struct sc_config *sc_config = (struct sc_config*)args;

    control_enter_t enter_func;
    control_infly_t infly_func;
    control_exit_t exit_func;
    struct timeval infly_tick_time;
    uint64_t current_tick_time_us;
    uint64_t last_exec_time_us;

    /* stick this thread to specified logging core */
    sc_util_stick_this_thread_to_core(sc_config->control_core_id);

    /* update log time */
    pthread_mutex_lock(&timer_mutex);
    time(&time_ptr);
    tmp_ptr = localtime(&time_ptr);
    memset(current_time_str, 0, sizeof(current_time_str));
    sprintf(current_time_str, "%d-%d-%d %d:%d:%d",
        tmp_ptr->tm_year + 1900,
        tmp_ptr->tm_mon + 1,
        tmp_ptr->tm_mday,
        tmp_ptr->tm_hour,
        tmp_ptr->tm_min,
        tmp_ptr->tm_sec
    );
    pthread_mutex_unlock(&timer_mutex);

    /* Hook Point: Enter */
    for(i=0; i<sc_config->nb_used_cores; i++){
        enter_func = PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_enter_func;
        if(likely(enter_func)){
            func_result = enter_func(sc_config, i);
            if(func_result != SC_SUCCESS && func_result != SC_ERROR_NOT_IMPLEMENTED){
                SC_ERROR("failed to execute control_enter_func for worker core %u", i);
            }
        }
    }

    /* record thread start time while test duration limitation is enabled */
    if(sc_config->enable_test_duration_limit){
        if(unlikely(-1 == gettimeofday(&sc_config->test_duration_start_time, NULL))){
            SC_THREAD_ERROR_DETAILS("failed to obtain start time");
            result = SC_ERROR_INTERNAL;
            goto control_plane_shutdown;
        }
    }

    while(!sc_force_quit){
        /* update log time */
        pthread_mutex_lock(&timer_mutex);
        time(&time_ptr);
        tmp_ptr = localtime(&time_ptr);
        memset(current_time_str, 0, sizeof(current_time_str));
        sprintf(current_time_str, "%d-%d-%d %d:%d:%d",
            tmp_ptr->tm_year + 1900,
            tmp_ptr->tm_mon + 1,
            tmp_ptr->tm_mday,
            tmp_ptr->tm_hour,
            tmp_ptr->tm_min,
            tmp_ptr->tm_sec
        );
        pthread_mutex_unlock(&timer_mutex);

        /* Hook Point: Execute Infly Function */
        for(i=0; i<sc_config->nb_used_cores; i++){
            infly_func = PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_infly_func;
            if(likely(infly_func)){
                // obtain the physical core id
                result = sc_util_get_core_id_by_logical_core_id(sc_config, i, &core_id);
                if(unlikely(result != SC_SUCCESS)){
                    SC_ERROR_DETAILS("failed to get core id by logical core id %u", i);
                    goto control_plane_shutdown;
                }

                // record current time
                if(unlikely(-1 == gettimeofday(&infly_tick_time, NULL))){
                    SC_THREAD_ERROR_DETAILS("failed to obtain infly tick time");
                    result = SC_ERROR_INTERNAL;
                    goto control_plane_shutdown;
                }
                current_tick_time_us = SC_UTIL_TIME_INTERVL_US(
                    infly_tick_time.tv_sec,
                    infly_tick_time.tv_usec
                );

                // obtain last execution time
                last_exec_time_us = SC_UTIL_TIME_INTERVL_US(
                    PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_last_time.tv_sec,
                    PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_last_time.tv_usec
                );

                if(current_tick_time_us - last_exec_time_us >= PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_interval){
                    // execution
                    func_result = infly_func(sc_config, core_id);
                    if(func_result != SC_SUCCESS && func_result != SC_ERROR_NOT_IMPLEMENTED){
                        SC_ERROR("failed to execute control_infly_func for worker core %u", i);
                    }

                    // record the execution time
                    PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_last_time.tv_sec = infly_tick_time.tv_sec;
                    PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).infly_last_time.tv_usec = infly_tick_time.tv_usec;
                }
            }
        }

        /* 
         * shutdown the application while test duration 
         * limitation is enabled and the limitation is reached
         */
        if(sc_config->enable_test_duration_limit){
            /* record current time */
            if(unlikely(-1 == gettimeofday(&sc_config->test_duration_end_time, NULL))){
                SC_THREAD_ERROR_DETAILS("failed to obtain end time");
                result = SC_ERROR_INTERNAL;
                goto control_plane_shutdown;
            }

            /* reach the duration limitation, quit all threads */
            if(sc_config->test_duration_end_time.tv_sec - sc_config->test_duration_start_time.tv_sec 
                >= sc_config->test_duration){
                    sc_force_quit = true;
                    break;
            }
        }
    }

    /* Hook Point: Execute Exit Function */
    for(i=0; i<sc_config->nb_used_cores; i++){
        exit_func = PER_CORE_CONTROL_FUNC_BY_CORE_ID(sc_config, i).control_exit_func;
        if(likely(exit_func)){
            func_result = exit_func(sc_config, i);
            if(func_result != SC_SUCCESS && func_result != SC_ERROR_NOT_IMPLEMENTED){
                SC_ERROR("failed to execute control_exit_func for worker core %u", i);
            }
        }
    }

control_plane_shutdown:
    sc_force_quit = true;

control_thread_exit:
    SC_WARNING("control thread exit\n");
    return NULL;
}

/*!
 * \brief   initialzie control-plane thread
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_control_thread(struct sc_config *sc_config){
    /* create logging thread handler */
    pthread_t *control_thread = (pthread_t*)malloc(sizeof(pthread_t));
    if(unlikely(!control_thread)){
        SC_ERROR_DETAILS("failed to allocate memory for logging thread hanlder");
        return SC_ERROR_MEMORY;
    }
    sc_config->control_thread = control_thread;

    /* initialize mutex lock for timer */
    if(pthread_mutex_init(&timer_mutex, NULL) != 0){
        SC_ERROR_DETAILS("failed to initialize timer mutex");
        return SC_ERROR_INTERNAL;
    }

    /* initialize log lock for each thread */
    if(pthread_mutex_init(&thread_log_mutex, NULL) != 0){
        SC_ERROR_DETAILS("failed to initialize per-thread log mutex");
        return SC_ERROR_INTERNAL;
    }

    return SC_SUCCESS;
}

/*!
 * \brief   launch control-plane thread
 * \param   sc_config   the global configuration
 * \return  zero for successfully launch
 */
int launch_control_thread_async(struct sc_config *sc_config){
    if(pthread_create(sc_config->control_thread, NULL, _control_loop, sc_config) != 0){
        SC_ERROR_DETAILS("failed to launch control-plane thread");
        return SC_ERROR_INTERNAL;
    }
    return SC_SUCCESS;
}

/*!
 * \brief   wait control-plane thread to finish
 * \param   sc_config   the global configuration
 * \return  zero for successfully launch
 */
int join_control_thread(struct sc_config *sc_config){
    pthread_join(*(sc_config->control_thread), NULL);
    pthread_mutex_destroy(&timer_mutex);
    return SC_SUCCESS;
}