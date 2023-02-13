#ifndef _SC_WORKER_H_
#define _SC_WORKER_H_

#include <rte_launch.h>

int init_worker(struct sc_config *sc_config);
int init_app(struct sc_config *sc_config);
int launch_worker(struct sc_config *sc_config);
int launch_worker_async(struct sc_config *sc_config);

#endif