#ifndef _AF_DBG_H_
#define _AF_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          af
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"

#if defined(__KERNEL__)
#define PRINT_AF_INFO(sfile, fmt, args...) {seq_printf(sfile, fmt, ## args); }
#else
#define PRINT_AF_INFO(sfile, fmt, args...) {DBG_DUMP(fmt, ## args); }
#endif

#endif

