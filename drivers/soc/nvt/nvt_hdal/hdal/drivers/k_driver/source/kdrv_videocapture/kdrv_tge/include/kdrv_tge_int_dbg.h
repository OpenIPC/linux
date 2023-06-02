#ifndef _KDRV_TGE_INT_DBG_H_
#define _KDRV_TGE_INT_DBG_H_

// should remove in the future
#include "tge_dbg.h"
extern unsigned int tge_debug_level;

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


#endif //_KDRV_TGE_INT_DBG_H_



