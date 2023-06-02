/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_debug.h

    @brief      nvtmpp debug api header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/
#ifndef _NVTMPP_DEBUG_H
#define _NVTMPP_DEBUG_H

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          nvtmpp
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
extern unsigned int nvtmpp_debug_level;
#endif

