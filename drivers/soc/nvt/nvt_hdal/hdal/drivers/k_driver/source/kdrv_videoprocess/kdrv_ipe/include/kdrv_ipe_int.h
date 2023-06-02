/*
    IPE module driver

    NT98520 IPE internal header file.

    @file       kdrv_ipe_int.h
    @ingroup    mIIPPIPE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_IPE_INT_H_
#define _KDRV_IPE_INT_H_

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
#include "ipe_eng.h"
#include "kdrv_ipe_int_dbg.h"
#include "kdrv_ipp_int_util.h"
#include "kdrv_videoprocess/kdrv_ipe.h"

#define KDRV_IPE_TIMEOUT_MS (1000)

typedef enum {
	KDRV_IPE_ISR_STATE_UNKNOWN = 0,
	KDRV_IPE_ISR_STATE_START,
	KDRV_IPE_ISR_STATE_END,
	KDRV_IPE_ISR_STATE_MAX
} KDRV_IPE_ISR_STATE;

typedef struct {
	UINT32 to_dma;
	UINT32 to_ime;
	IPE_ENG_DRAM_OUT_SEL out_sel;
	IPE_ENG_FMT in_fmt;
	IPE_ENG_FMT out_fmt;
} KDRV_IPE_IO_INFO;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	SEM_HANDLE sem;
	vk_spinlock_t lock;
	IPE_ENG_HANDLE *p_eng;
	KDRV_IPP_ISR_CB cb;
	KDRV_IPE_ISR_STATE isr_state;
} KDRV_IPE_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	KDRV_IPE_HANDLE *p_hdl;
	void *p_eng_reg_buf;	/* working buffer for ipe_eng */
	void *p_eng_flg_buf;	/* working buffer for ipe_eng */
} KDRV_IPE_CTL;

typedef struct {
	INT32(*set_fp)(KDRV_IPE_HANDLE *p_hdl, void *p_data);
	BOOL set_chk_data;

	INT32(*get_fp)(KDRV_IPE_HANDLE *p_hdl, void *p_data);
	BOOL get_chk_data;

	CHAR *msg;
} KDRV_IPE_FUNC_ITEM;

typedef struct {
	INT32(*query_fp)(UINT32 id, void *p_data);
	CHAR *msg;
} KDRV_IPE_QUERY_ITEM;

INT32 kdrv_ipe_sys_init(UINT32 chip_num, UINT32 eng_num);
INT32 kdrv_ipe_sys_uninit(void);
void kdrv_ipe_dump_info(void);
void *kdrv_ipe_os_malloc_wrap(UINT32 want_size);
void kdrv_ipe_os_mfree_wrap(void *p_buf);

#endif
