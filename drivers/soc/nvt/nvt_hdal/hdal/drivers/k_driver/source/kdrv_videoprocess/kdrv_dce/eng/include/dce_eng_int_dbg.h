#ifndef _DCE_ENG_INT_DBG_H_
#define _DCE_ENG_INT_DBG_H_

#define __MODULE__          dce_eng
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#if defined(__LINUX)
#include "linux/printk.h"
#include "kwrap/debug.h"

#elif defined(__FREERTOS)
#include <stdio.h>
#include <kwrap/debug.h>


#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
//#error Not supported OS
#endif

extern unsigned int dce_eng_debug_level;

#endif //_DCE_ENG_INT_DBG_H_

