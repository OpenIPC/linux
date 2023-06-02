#ifndef _IFE_ENG_INT_DBG_H_
#define _IFE_ENG_INT_DBG_H_

#define __DBGLVL__ NVT_DBG_IND

#if defined(__LINUX)
#include "kwrap/debug.h"

#elif defined(__FREERTOS)
#include <stdio.h>
#include <kwrap/debug.h>


#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
#error Not supported OS
#endif


#endif //_IFE_ENG_INT_DBG_H_

