#ifndef __IME_ENG_INT_DBG_H_
#define __IME_ENG_INT_DBG_H_

#ifdef __cplusplus
extern "C" {
#endif



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


#ifdef __cplusplus
}
#endif


#endif


