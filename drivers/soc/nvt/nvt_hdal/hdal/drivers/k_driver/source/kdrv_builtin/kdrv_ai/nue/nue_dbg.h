#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_


#include "kwrap/debug.h"
#define NVT_DBG_FATAL     0
#define NVT_DBG_ERR       1
#define NVT_DBG_WRN       2
#define NVT_DBG_UNIT      3
#define NVT_DBG_FUNC      4
#define NVT_DBG_IND       5
#define NVT_DBG_MSG       6
#define NVT_DBG_VALUE     7
#define NVT_DBG_USER      8

extern unsigned int nue_drv_debug_level;
#endif
