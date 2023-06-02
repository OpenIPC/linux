/*
    Internal header file for SIE module.

    @file       sie_init.h
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _SIE_INIT_H
#define _SIE_INIT_H
#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "kwrap/perf.h"
#include <linux/slab.h>

typedef enum {
	SIE_FB_OUT_BASE = 0,
	SIE_FB_OUT_CH0,
	SIE_FB_OUT_CH1,
	SIE_FB_OUT_CH2,
	SIE_FB_OUT_CH_MAX,
	ENUM_DUMMY4WORD(SIE_FB_OUT_CH_IDX)
} SIE_FB_OUT_CH_IDX;

typedef enum {
	SIE_FB_BUF_IDX_CUR_OUT = 0,
	SIE_FB_BUF_IDX_RDY,
	SIE_FB_BUF_IDX_MAX,
	ENUM_DUMMY4WORD(SIE_FB_BUF_IDX)
} SIE_FB_BUF_IDX;

typedef struct {
	UINT32 sie_id_bit;	// fastboot sie id (main id, not include duplicate id)

	/* private buffer info */
	UINT32 ring_buf_blk_addr;	//for SIE2 ring buffer using
	UINT32 ring_buf_blk_size;	//for SIE2 ring buffer using
} SIE_BUILTIN_INIT_INFO;


typedef enum {
	KDRV_SIE_BUILTIN_FUNC_ALGO_BY_PASS   =  0x00000001
} KDRV_SIE_BUILTIN_DBG_FUNC;

typedef struct {
#define SIE_BUILTIN_HEADER_CTL_LOCK 0x1			// push ready buffer for ipp, ipp cannot release buffer
#define SIE_BUILTIN_HEADER_CTL_PUSH 0x2 		// push ready buffer for ipp, ipp need to release buffer
	UINT32 buf_ctrl;
	UINT32 buf_addr;				///< buffer start address
	UINT32 count;					///< fastboot VD frame count
	VOS_TICK timestamp;				///< VD timestamp
	UINT32 addr_ch0;				///< ch0(raw) output addr
} SIE_BUILTIN_HEADER_INFO;

typedef void (*SIE_FB_ISR_FP)(UINT32 id, UINT32 status); //status ref. to SIE_INT_VD
typedef void (*SIE_FB_BUF_OUT_FP)(UINT32 id, SIE_BUILTIN_HEADER_INFO *info);
extern ER sie_builtin_init(SIE_BUILTIN_INIT_INFO *info);	//set sie fastboot streaming
extern ER sie_fb_reg_isr_Cb(SIE_FB_ISR_FP fp);	//register isr callback function
extern ER sie_fb_reg_buf_out_cb(SIE_FB_BUF_OUT_FP fp);	//register buffer push out callback function
extern void sie_fb_get_rdy_addr(UINT32 id, UINT32 *addr_ch0, UINT32 *addr_ch1, UINT32 *addr_ch2);	//get sie output ready address
extern void sie_fb_buf_release(UINT32 id, BOOL cb_trig, UINT32 status,SIE_FB_BUF_IDX rls_buf_idx);
extern void sie_fb_ring_buf_release(UINT32 id);
extern void sie_fb_upd_timestp(UINT32 id);
extern UINT32 sie_set_dbg_func(UINT32 id, UINT32 dbg_func_en, BOOL en);

#endif// _SIE_INIT_H
#endif
