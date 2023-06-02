/*
    IME module driver

    NT98520 IME internal header file.

    @file       kdrv_IME_int.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_IME_INT_H_
#define _KDRV_IME_INT_H_

#if defined(__LINUX)
#include <mach/rcw_macro.h>
#elif defined(__FREERTOS)
#include <stdio.h>
#include <string.h>

#include "rcw_macro.h"
#endif

#include "kwrap/task.h"
#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "kwrap/cpu.h"
#include "ime_eng.h"
#include "kdrv_ime_int_dbg.h"
#include "kdrv_ipp_int_util.h"
#include "kdrv_videoprocess/kdrv_ime.h"

#define KDRV_IME_TIMEOUT_MS (1000)

typedef enum {
	KDRV_IME_ISR_STATE_UNKNOWN = 0,
	KDRV_IME_ISR_STATE_START,
	KDRV_IME_ISR_STATE_BP,
	KDRV_IME_ISR_STATE_END,
	KDRV_IME_ISR_STATE_MAX
} KDRV_IME_ISR_STATE;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	SEM_HANDLE sem;
	vk_spinlock_t lock;
	IME_ENG_HANDLE *p_eng;
	KDRV_IPP_ISR_CB cb;
	KDRV_IME_ISR_STATE isr_state;
} KDRV_IME_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	KDRV_IME_HANDLE *p_hdl;
	void *p_eng_reg_buf;	/* working buffer for ime_eng */
	void *p_eng_flg_buf;	/* working buffer for ime_eng */
} KDRV_IME_CTL;

typedef struct {
	INT32(*set_fp)(KDRV_IME_HANDLE *p_hdl, void *p_data);
	BOOL set_chk_data;

	INT32(*get_fp)(KDRV_IME_HANDLE *p_hdl, void *p_data);
	BOOL get_chk_data;

	CHAR *msg;
} KDRV_IME_FUNC_ITEM;

typedef struct {
	INT32(*query_fp)(UINT32 id, void *p_data);
	CHAR *msg;
} KDRV_IME_QUERY_ITEM;

INT32 kdrv_ime_sys_init(UINT32 chip_num, UINT32 eng_num);
INT32 kdrv_ime_sys_uninit(void);
void *kdrv_ime_os_malloc_wrap(UINT32 want_size);
void kdrv_ime_os_mfree_wrap(void *p_buf);

#endif

