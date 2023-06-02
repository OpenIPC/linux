/**
    KDF SIE Layer

    @file       kdf_sie_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _KDF_SIE_INT_H
#define _KDF_SIE_INT_H

#include "kflow_videocapture/ctl_sie_utility.h"
#include "kdrv_videocapture/kdrv_sie.h"
#include "ctl_sie_dbg.h"

typedef struct {
	UINT32 ctl_id;
	UINT32 kdrv_id;
	UINT32 chip;
	KDRV_SIE_ISRCB isrcb_fp;    //isr cb function pointer
} KDF_SIE_HDL;

typedef struct {
	UINT32 context_num;
	UINT32 context_used;
	UINT32 contex_size;;
	UINT32 start_addr;
	UINT32 req_size;
} KDF_SIE_HDL_CONTEXT;


UINT32 kdf_sie_buf_query(UINT32 num);
INT32 kdf_sie_init(UINT32 buf_addr, UINT32 buf_size);
INT32 kdf_sie_uninit(void);
INT32 kdf_sie_set_mclk(UINT32 id, void *data);
INT32 kdf_sie_set_ccir_header(UINT32 id, void *data);
UINT32 kdf_sie_open(CTL_SIE_ID id, void *data);
INT32 kdf_sie_close(UINT32 hdl);
INT32 kdf_sie_set(UINT32 hdl, UINT64 item, void *data);
INT32 kdf_sie_get(UINT32 hdl, UINT64 item, void *data);
INT32 kdf_sie_trigger(UINT32 hdl, void *data);
INT32 kdf_sie_suspend(UINT32 hdl, void *data);
INT32 kdf_sie_resume(UINT32 hdl, void *data);
INT32 kdf_sie_get_limit(UINT32 id, void *data);
INT32 kdf_sie_dump_fb_info(INT fd, UINT32 en);

#endif //_KDF_SIE_INT_H
