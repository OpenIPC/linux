#ifndef _KDRV_SIE_DEBUG_INT_H_
#define _KDRV_SIE_DEBUG_INT_H_
#include "sie_dbg.h"
#include "sie_platform.h"
#include "kdrv_sie.h"
#include "plat/top.h"
/**
    kdrv_sie_debug_int.h


    @file       kdrv_sie_debug_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

typedef enum {
	KDRV_SIE_DBG_MSG_OFF = 0,		//disable all debug msg
	KDRV_SIE_DBG_MSG_CTL_INFO,		//dump kdrv_sie parameters info
	KDRV_SIE_DBG_MSG_ALL,			//enable all ctl sie debug msg
	KDRV_SIE_DBG_MSG_MAX
} KDRV_SIE_DBG_MSG_TYPE;

typedef struct {
	KDRV_SIE_DBG_MSG_TYPE dbg_msg_type;
} KDRV_SIE_DBG_INFO;

void kdrv_sie_dump_info(void);
void kdrv_sie_dbg_set_dbg_level(KDRV_SIE_DBG_LVL dbg_level);
void kdrv_sie_dbg_set_msg_type(KDRV_SIE_PROC_ID id, KDRV_SIE_DBG_MSG_TYPE type);
UINT32 kdrv_sie_set_dbg_func_en(UINT32 id, UINT32 dbg_func_en, BOOL en);
#endif //_KDRV_SIE_DEBUG_INT_H_
