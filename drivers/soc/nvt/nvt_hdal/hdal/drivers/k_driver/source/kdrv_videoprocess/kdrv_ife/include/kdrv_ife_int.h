/*
    IFE module driver

    NT98520 IFE internal header file.

    @file       kdrv_ife_int.h
    @ingroup    mIIPPIFE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_IFE_INT_H_
#define _KDRV_IFE_INT_H_

#if defined(__LINUX)
#elif defined(__FREERTOS)
#include <stdio.h>
#include <string.h>
#endif
#include "kwrap/task.h"
#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "kwrap/cpu.h"
#include "ife_eng.h"
#include "kdrv_ife_int_dbg.h"
#include "kdrv_ipp_int_util.h"
#include "kdrv_videoprocess/kdrv_ife.h"

#define KDRV_IFE_TIMEOUT_MS (1000)
#define KDRV_IFE_RNG_TH_MAX 1023

typedef enum {
	KDRV_IFE_ISR_STATE_UNKNOWN = 0,
	KDRV_IFE_ISR_STATE_START,
	KDRV_IFE_ISR_STATE_END,
	KDRV_IFE_ISR_STATE_MAX
} KDRV_IFE_ISR_STATE;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	SEM_HANDLE sem;
	vk_spinlock_t lock;
	IFE_ENG_HANDLE *p_eng;
	KDRV_IPP_ISR_CB cb;
	KDRV_IFE_ISR_STATE isr_state;
	KDRV_IFE_DBG_INFO *p_ife_dbg_info;
	KDRV_IFE_DBG_CTL ife_dbg_ctl;
} KDRV_IFE_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	KDRV_IFE_HANDLE *p_hdl;
	void *p_eng_reg_buf;	/* working buffer for ife_eng */
	void *p_eng_flg_buf;	/* working buffer for ife_eng */
} KDRV_IFE_CTL;

typedef struct {
	INT32(*set_fp)(KDRV_IFE_HANDLE *p_hdl, void *p_data);
	BOOL set_chk_data;

	INT32(*get_fp)(KDRV_IFE_HANDLE *p_hdl, void *p_data);
	BOOL get_chk_data;

	CHAR *msg;
} KDRV_IFE_FUNC_ITEM;

typedef struct {
	INT32(*query_fp)(UINT32 id, void *p_data);
	CHAR *msg;
} KDRV_IFE_QUERY_ITEM;

INT32 kdrv_ife_sys_init(UINT32 chip_num, UINT32 eng_num);
INT32 kdrv_ife_sys_uninit(void);
void *kdrv_ife_os_malloc_wrap(UINT32 want_size);
void kdrv_ife_os_mfree_wrap(void *p_buf);
UINT32 kdrv_ife_do_div(UINT64 n, UINT64 base);

#endif

