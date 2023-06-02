/**
    Header file for KDRV TGE module

    This file is the header file that define the API for KDRV TGE.

    @file       kdrv_tge.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _KDRV_TGE_H
#define _KDRV_TGE_H

#include "kwrap/type.h"
#ifdef __KERNEL__
#include <linux/types.h>
#endif
#include "kdrv_type.h"

#define KDRV_TGE_VDHD_CH_MAX 2
#define KDRV_TGE_SWAP_CH_MAX 2
#define KDRV_TGE_SIE_SRC_CH_MAX 2
#define KDRV_TGE_CLK_ID_MAX 2

typedef enum {
	KDRV_TGE_VDHD_CH1 = (1 << 0),
	KDRV_TGE_VDHD_CH2 = (1 << 1),
	KDRV_TGE_VDHD_CH3 = (1 << 2),
	KDRV_TGE_VDHD_CH4 = (1 << 3),
	KDRV_TGE_VDHD_CH5 = (1 << 4),
	KDRV_TGE_VDHD_CH6 = (1 << 5),
	KDRV_TGE_VDHD_CH7 = (1 << 6),
	KDRV_TGE_VDHD_CH8 = (1 << 7),
} KDRV_TGE_VDHD_CH;

typedef enum {
	KDRV_TGECLK1 = (1 << 0),
	KDRV_TGECLK2 = (1 << 1),
} KDRV_TGE_CLK_ID;

typedef enum {
	KDRV_TGE_SWAP_CH15 = (1 << 0),
	KDRV_TGE_SWAP_CH26 = (1 << 1),
	KDRV_TGE_SWAP_CH37 = (1 << 2),
	KDRV_TGE_SWAP_CH48 = (1 << 3),
} KDRV_TGE_SWAP_FUNC_ID;

typedef enum {
	KDRV_TGE_SIE1_IN = (1 << 0), 	// Only used in parallel + slave sensor
	KDRV_TGE_SIE3_IN = (1 << 1),	// Only used in parallel + slave sensor
} KDRV_TGE_SIE_IN_ID;

typedef enum {
	TGE_WAIT_VD 		= (1 << 0),
	TGE_WAIT_VD2 		= (1 << 1),
	TGE_WAIT_VD3 		= (1 << 2),
	TGE_WAIT_VD4 		= (1 << 3),
	TGE_WAIT_VD5 		= (1 << 4),
	TGE_WAIT_VD6 		= (1 << 5),
	TGE_WAIT_VD7 		= (1 << 6),
	TGE_WAIT_VD8 		= (1 << 7),
	TGE_WAIT_VD_BP1 	= (1 << 8),
	TGE_WAIT_VD2_BP1 	= (1 << 9),
	TGE_WAIT_VD3_BP1 	= (1 << 10),
	TGE_WAIT_VD4_BP1 	= (1 << 11),
	TGE_WAIT_VD5_BP1 	= (1 << 12),
	TGE_WAIT_VD6_BP1 	= (1 << 13),
	TGE_WAIT_VD7_BP1 	= (1 << 14),
	TGE_WAIT_VD8_BP1 	= (1 << 15),
} TGE_WAIT_EVENT_SEL;

typedef enum {
	KDRV_TGE_INT_VD1 	= (1 << 0),
	KDRV_TGE_INT_VD2 	= (1 << 1),
	KDRV_TGE_INT_VD3 	= (1 << 2),
	KDRV_TGE_INT_VD4 	= (1 << 3),
	KDRV_TGE_INT_VD5 	= (1 << 4),
	KDRV_TGE_INT_VD6 	= (1 << 5),
	KDRV_TGE_INT_VD7 	= (1 << 6),
	KDRV_TGE_INT_VD8 	= (1 << 7),
	KDRV_TGE_INT_BP1 	= (1 << 8),
	KDRV_TGE_INT_BP2 	= (1 << 9),
	KDRV_TGE_INT_BP3 	= (1 << 10),
	KDRV_TGE_INT_BP4 	= (1 << 11),
	KDRV_TGE_INT_BP5 	= (1 << 12),
	KDRV_TGE_INT_BP6 	= (1 << 13),
	KDRV_TGE_INT_BP7 	= (1 << 14),
	KDRV_TGE_INT_BP8 	= (1 << 15),
} KDRV_TGE_ISR_EVENT;

typedef void (*KDRV_TGE_ISRCB)(KDRV_TGE_ISR_EVENT, void*);

/**
	sie kdrv trig type, start/stop sie
*/
typedef enum {
	KDRV_TGE_TRIG_VDHD = 0,  	///< VD/HD Generator
    KDRV_TGE_TRIG_STOP,         ///< stop(pause)
} KDRV_TGE_TRIG_TYPE;

typedef enum {
	TGE_RESERVED = 0,
} KDRV_TGE_TRIGGER_PARAM;

typedef struct {
	INT32 (*callback)(void *callback_info, void *user_data);
	INT32 (*reserve_buf)(UINT32 phy_addr);
	INT32 (*free_buf)(UINT32 phy_addr);
} KDRV_TGE_CALLBACK_FUNC;

/**
    TGE clock source
*/
typedef enum {
	KDRV_TGE_CLK_PCLK = 0,
	KDRV_TGE_CLK_MCLK1,
	KDRV_TGE_CLK_MCLK2,
} KDRV_TGE_CLK_SRC;

typedef struct {
	KDRV_TGE_CLK_SRC clk_src_info;
} KDRV_TGE_CLK_SRC_SEL;

/**
    KDRV TGE structure - Engine open object
*/
typedef struct {
	KDRV_TGE_CLK_SRC tge_clock_sel;		///< Engine clock selection. 520 only support MCLK1
	KDRV_TGE_CLK_SRC tge_clock_sel2;	///< Engine clock selection, 520 only support MCLK2
} KDRV_TGE_OPENCFG;

/**
    TGE VD HD signal I/O
*/
typedef enum {
	KDRV_MODE_MASTER = 0, 			///< output by TGE generator
	KDRV_MODE_SLAVE_TO_PAD,			///< input by parallel sensor
	KDRV_MODE_SLAVE_TO_CSI,			///< input by CSI sensor
	KDRV_MODE_SLAVE_TO_SLVSEC,		///< input by slvsec sensor
} KDRV_TGE_MODE_SEL;

/**
    TGE latch parameters in VD edge rising or falling
*/
typedef enum {
	KDRV_TGE_PHASE_RISING  = 0, 	///< rising edge latch/trigger
	KDRV_TGE_PHASE_FALLING, 		///< falling edge latch/trigger
} KDRV_TGE_PHASE_SEL;

/**
    KDRV TGE VD/HD structure - VD HD signal waveform
*/
typedef struct {
    KDRV_TGE_MODE_SEL mode;
	UINT32 vd_period;
	UINT32 vd_assert;
	UINT32 vd_frontblnk;
	UINT32 hd_period;
	UINT32 hd_assert;
	UINT32 hd_cnt;
	KDRV_TGE_PHASE_SEL vd_phase; // latch vd by rising edge or falling edge
    KDRV_TGE_PHASE_SEL hd_phase;
    BOOL vd_inverse; // if TGE is master, (inverse = false) is vd_assert = low, (inverse = true) is vd_assert = high. if TGE is slave, just invert the input signal
    BOOL hd_inverse;
} KDRV_TGE_VDHD_INFO;

typedef struct {
	UINT32 bp_line; // bp value
} KDRV_TGE_BP_INFO;

typedef struct {
	BOOL swap_enble;
} KDRV_TGE_SWAP_INFO;

/**
	struct for kdrv_sie_open
*/
typedef struct {
	KDRV_TGE_TRIG_TYPE trig_type; 	///< trigger operation
	BOOL ch_enable; 				///< channel enable for VD/HD generator
	BOOL wait_end_enable; 			///< wait trigger end
    TGE_WAIT_EVENT_SEL wait_event;
} KDRV_TGE_TRIG_INFO;

typedef struct {
	BOOL vd_pause;
	BOOL hd_pause;
} KDRV_TGE_TIMING_PAUSE_INFO;

/**
    SIE 1/3 VD/HD signal source (only in parallel mode + slave sensor)
*/
typedef enum _KDRV_TGE_SIE_VD_SRC {
	KDRV_TGE_SIE1_VD_SRC_CH1 = 0, 	// use ch1 to be SIE1 VD source
	KDRV_TGE_SIE1_VD_SRC_CH3, 		// use ch3 to be SIE1 VD source
	KDRV_TGE_SIE3_VD_SRC_CH5, 		// use ch5 to be SIE3 VD source
	KDRV_TGE_SIE3_VD_SRC_CH7, 		// use ch7 to be SIE3 VD source
} KDRV_TGE_SIE_VD_SRC;

typedef struct {
    KDRV_TGE_SIE_VD_SRC vd_src;
} KDRV_TGE_SIE_VD_INFO;

/**
    TGE KDRV ITEM
*/
typedef enum {
	KDRV_TGE_PARAM_IPL_OPENCFG, 				///< [Set/Get] 	id: NA, 					data_type: KDRV_TGE_OPENCFG, 			config tge clock
	KDRV_TGE_PARAM_IPL_VDHD, 					///< [Set/Get] 	id: KDRV_TGE_VDHD_CH, 		data_type: KDRV_TGE_VDHD_INFO, 			VD/HD generator's parameters
	KDRV_TGE_PARAM_IPL_VD_BP, 					///< [Set/Get] 	id: KDRV_TGE_VDHD_CH, 		data_type: KDRV_TGE_BP_INFO, 			the break-point line to change TGE's interrupt timing
	KDRV_TGE_PARAM_IPL_PAUSE,					///< [Set/Get] 	id: KDRV_TGE_VDHD_CH, 		data_type: KDRV_TGE_TIMING_PAUSE_INFO, 	pause VD/HD counter
	KDRV_TGE_PARAM_IPL_SWAP, 					///< [Set/Get] 	id: KDRV_TGE_SWAP_FUNC_ID, 	data_type: KDRV_TGE_SWAP_INFO, 			(520 NoSup) swap CH1 & 5, or CH2 & 6, or CH3 & 7, or CH4 & 8
	KDRV_TGE_PARAM_IPL_SIE_VDHD_SRC, 			///< [Set/Get] 	id: KDRV_TGE_SIE_IN_ID, 	data_type: KDRV_TGE_SIE_VD_INFO, 		(520 NoSup) output VD/HD to SIE1 from ch1/ch3 and to SIE3 from ch5/ch7
	KDRV_TGE_PARAM_IPL_TGE_CLK_SRC, 			///< [Set] 		id: KDRV_TGE_CLK_ID, 		data_type: KDRV_TGE_CLK_SRC_SEL, 		the clk source of tge1 or tge2, the final result can use KDRV_TGE_PARAM_IPL_OPENCFG
	KDRV_TGE_PARAM_IPL_SET_ISR_CB, 				///< [Set] 		id: NA, 					data_type: KDRV_TGE_ISRCB, 				set tge external isr cb
	KDRV_TGE_PARAM_MAX, 						///<
} KDRV_TGE_PARAM_ID;


extern INT32 kdrv_tge_init(void);
extern INT32 kdrv_tge_uninit(void);
extern INT32 kdrv_tge_open(UINT32 chip, UINT32 engine);
extern INT32 kdrv_tge_close(UINT32 chip, UINT32 engine);
extern INT32 kdrv_tge_set(UINT32 id, KDRV_TGE_PARAM_ID item, void* data);
extern INT32 kdrv_tge_get(UINT32 id, KDRV_TGE_PARAM_ID item, void* data);
extern INT32 kdrv_tge_trigger(UINT32 id, KDRV_TGE_TRIGGER_PARAM *p_rpc_param, KDRV_TGE_CALLBACK_FUNC *p_cb_func, void *p_user_data);
extern void kdrv_tge_dump_info(void);

#endif


