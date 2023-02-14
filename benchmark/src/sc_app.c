#include "sc_global.h"
#include "sc_utils.h"
#include "sc_app.h"

#ifdef APP_SKETCH
#include "sc_sketch.h"
#endif

/*!
 * \brief   initialize application
 * \param   sc_config   the global configuration
 * \return  zero for successfully initialization
 */
int init_app(struct sc_config *sc_config){
    return _init_app(sc_config);
}