/*
    DCE module driver

    NT98520 DCE internal header file.

    @file       kdrv_DCE_int.h
    @ingroup    mIIPPDCE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_DCE_INT_H_
#define _KDRV_DCE_INT_H_

#if defined(__LINUX)
#elif defined(__FREERTOS)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "kwrap/task.h"
#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "kwrap/cpu.h"
#include "dce_eng.h"
#include "kdrv_dce_int_dbg.h"
#include "kdrv_ipp_int_util.h"
#include "kdrv_videoprocess/kdrv_dce.h"

#define KDRV_DCE_TDCEOUT_MS (1000)

typedef enum {
	KDRV_DCE_ISR_STATE_UNKNOWN = 0,
	KDRV_DCE_ISR_STATE_START,
	KDRV_DCE_ISR_STATE_END,
	KDRV_DCE_ISR_STATE_MAX
} KDRV_DCE_ISR_STATE;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	SEM_HANDLE sem;
	vk_spinlock_t lock;
	DCE_ENG_HANDLE *p_eng;
	KDRV_IPP_ISR_CB cb;
	KDRV_DCE_ISR_STATE isr_state;
	KDRV_DCE_DBG_INFO *p_dce_dbg_info;
	KDRV_DCE_DBG_CTL  dce_dbg_ctl;
} KDRV_DCE_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	KDRV_DCE_HANDLE *p_hdl;
	void *p_eng_reg_buf;    /* working buffer for dce_eng */
	void *p_eng_flg_buf;    /* working buffer for dce_eng */
} KDRV_DCE_CTL;

typedef struct {
	INT32(*set_fp)(KDRV_DCE_HANDLE *p_hdl, void *p_data);
	BOOL set_chk_data;

	INT32(*get_fp)(KDRV_DCE_HANDLE *p_hdl, void *p_data);
	BOOL get_chk_data;

	CHAR *msg;
} KDRV_DCE_FUNC_ITEM;

typedef struct {
	INT32(*query_fp)(UINT32 id, void *p_data);
	CHAR *msg;
} KDRV_DCE_QUERY_ITEM;


//INT32 kdrv_dce_sys_init(KDRV_DCE_CTL *p_dce_ctl);
//INT32 kdrv_dce_sys_uninit(KDRV_DCE_CTL *p_dce_ctl);
INT32 kdrv_dce_sys_init(UINT32 chip_num, UINT32 eng_num);
INT32 kdrv_dce_sys_uninit(void);
void *kdrv_dce_os_malloc_wrap(UINT32 want_size);
void kdrv_dce_os_mfree_wrap(void *p_buf);

#endif

