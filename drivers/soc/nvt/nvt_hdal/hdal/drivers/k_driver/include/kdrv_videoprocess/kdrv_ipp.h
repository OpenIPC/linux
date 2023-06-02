/**
    Public header file for KDRV_IPP

    This file is the header file that define the API and data type for KDRV_IPP.

    @file       kdrv_ipp.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_IPP_H_
#define _KDRV_IPP_H_

#include "kdrv_videoprocess/kdrv_ipe.h"
#include "kdrv_videoprocess/kdrv_ime.h"
#include "kdrv_videoprocess/kdrv_ife.h"
#include "kdrv_videoprocess/kdrv_dce.h"
#include "kdrv_type.h"

#define KDRV_IPP_PROC_MODE_TAG		(0x5A5A0000)
#define KDRV_IPP_PROC_MODE_LINKLIST	(KDRV_IPP_PROC_MODE_TAG | 1)
#define KDRV_IPP_PROC_MODE_CPU		(KDRV_IPP_PROC_MODE_TAG | 2)

typedef enum {
	KDRV_IPP_CALLBACK_PRESET = 0,	/* last config timing befor trigger, for buffer alloc, p_info = KDRV_IPP_CB_PRESET_INFO */
	KDRV_IPP_CALLBACK_JOBEND,		/* job end, p_info = KDRV_IPP_CB_JOB_INFO */
	KDRV_IPP_CALLBACK_JOBDROP,		/* job drop, only call at flush flow, p_info = KDRV_IPP_CB_JOB_INFO */
	KDRV_IPP_CALLBACK_JOBSTART,		/* job start, p_info = KDRV_IPP_CB_JOB_INFO */
	KDRV_IPP_CALLBACK_ISR,			/* engine isr callback, p_info = KDRV_IPP_CB_ISR_INFO */
	KDRV_IPP_CALLBACK_OOPS,			/* something wrong, dump information for debug, p_info = KDRV_IPP_CB_JOB_INFO */
	KDRV_IPP_CALLBACK_FREEJOB,		/* notify kdrv job free, p_info = NULL */
} KDRV_IPP_CALLBACK_EVT;

typedef enum {
	KDRV_IPP_TRIG_START = 0,	/* mode, start.					data_type: TBD				*/
	KDRV_IPP_TRIG_LOAD,			/* mode, load.					data_type: TBD				*/
	KDRV_IPP_TRIG_STOP,			/* mode, stop.					data_type: TBD				*/
	KDRV_IPP_TRIG_MAX
} KDRV_IPP_OP_MODE;

typedef enum {
	KDRV_IPP_JOB_TS_GENNODE = 0,/* gen node */
	KDRV_IPP_JOB_TS_PUTJOB,		/* put job in queue */
	KDRV_IPP_JOB_TS_PROC_START,	/* kdrv process job start */
	KDRV_IPP_JOB_TS_PROC_END,	/* kdrv process job end */
	KDRV_IPP_JOB_TS_ENG_LOAD,	/* engine trigger/load */
	KDRV_IPP_JOB_TS_ENG_START,	/* engine isr frame start */
	KDRV_IPP_JOB_TS_ENG_END,	/* engine isr frame end */
	KDRV_IPP_JOB_TS_JOB_END,	/* callback to kflow */
	KDRV_IPP_JOB_TS_MAX
} KDRV_IPP_JOB_TS;

typedef enum {
	KDRV_IPP_STATUS_START = 0,	/* status:start.					data_type: TBD				*/
	KDRV_IPP_STATUS_STOP,		/* status:stop.						data_type: TBD				*/
	KDRV_IPP_STATUS_STOP_END,	/* status:stop_end.					data_type: TBD				*/
	KDRV_IPP_STATUS_MAX
} KDRV_IPP_STATUS;

typedef struct {
	UINT32 job_head_num;
	UINT32 job_cfg_num;
	UINT32 ll_blk_num;
	UINT32 reserved;
} KDRV_IPP_CTX_BUF_CFG;

typedef struct {
	KDRV_IFE_JOB_CFG ife;
	KDRV_DCE_JOB_CFG dce;
	KDRV_IPE_JOB_CFG ipe;
	KDRV_IME_JOB_CFG ime;
} KDRV_IPP_JOB_CFG_ALL;

typedef union {
	KDRV_IFE_JOB_CFG ife;
	KDRV_DCE_JOB_CFG dce;
	KDRV_IPE_JOB_CFG ipe;
	KDRV_IME_JOB_CFG ime;
} KDRV_IPP_JOB_CFG_UNION;

typedef struct {
	KDRV_IME_JOB_CFG ime_cfg;
	KDRV_IPE_JOB_CFG ipe_cfg;
	KDRV_DCE_JOB_CFG dce_cfg;
	UINT32 owner;
} KDRV_IPP_CB_PRESET_INFO;

typedef struct {
	UINT32 cfg_owner;
	UINT32 *p_ts_arr;	/* timestamp arr, KDRV_IPP_JOB_TS */
	UINT32 cfg_status;
	UINT64 timestamp;	/* last frame start timestamp for direct mode */
} KDRV_IPP_CB_JOB_INFO;

typedef struct {
	KDRV_IPP_ENG eng;
	UINT32 interrupt;
	UINT32 cfg_owner;
	KDRV_IPP_JOB_CFG_UNION job;	/* job of corresponding engine */
} KDRV_IPP_CB_ISR_INFO;

typedef struct {
	UINT32 event;
	UINT32 job_owner;
	UINT32 job_id;
	UINT8 job_num;
	UINT8 err_num;
	UINT8 done_num;
	void* p_info;
} KDRV_IPP_CALLBACK_INFO;

typedef struct {
	KDRV_IPP_ENG eng;
	UINT32 param_id;
	void *p_data;
} KDRV_IPP_ENG_INFO;

typedef enum {
	KDRV_IPP_PARAM_GEN_NODE = 0,	/* Generate config node.             SET-Only,   data_type: NULL */
	KDRV_IPP_PARAM_FLUSH_JOB,		/* flush current job.				 SET-Only,   data_type: NULL */
	KDRV_IPP_PARAM_FLUSH_CFG,		/* flush current cfg.				 SET-Only,   data_type: NULL */
	KDRV_IPP_PARAM_IFE_CFG,			/* set kdrv_ife cfg.				 SET-Only,   data_type: KDRV_IFE_JOB_CFG */
	KDRV_IPP_PARAM_DCE_CFG,			/* set kdrv_dce cfg.				 SET-Only,   data_type: KDRV_DCE_JOB_CFG */
	KDRV_IPP_PARAM_IPE_CFG,			/* set kdrv_ipe cfg.				 SET-Only,   data_type: KDRV_IPE_JOB_CFG */
	KDRV_IPP_PARAM_IME_CFG,			/* set kdrv_ime cfg.				 SET-Only,   data_type: KDRV_IME_JOB_CFG */
	KDRV_IPP_PARAM_IFE2_CFG,		/* set kdrv_ife2 cfg.				 SET-Only,   data_type: KDRV_IFE2_JOB_CFG */
	KDRV_IPP_PARAM_JOB_CFG,			/* get cur job .					 GET-Only,   data_type: KDRV_IPP_JOB_CFG_ALL */
	KDRV_IPP_PARAM_JOB_NUM,			/* get cur job number.				 GET-Only,   data_type: UINT32 */
	KDRV_IPP_PARAM_GET_ENG_INFO,	/* kdrv_eng get wrapper.			 GET-Only,   data_type: KDRV_IPP_ENG_INFO */
	KDRV_IPP_PARAM_QUERY_ENG_INFO,	/* kdrv_eng query wrapper.			 GET-Only,   data_type: KDRV_IPP_ENG_INFO */
	KDRV_IPP_PARAM_STRIPE_INFO,		/* calculate stripe for cur_job.	 GET-Only,   data_type: KDRV_DCE_JOB_CFG */
	KDRV_IPP_PARAM_HARD_RESET,		/* hard reset all ipp engine.		 SET-Only,   data_type: NULL */
	KDRV_IPP_PARAM_DMA_ABORT,		/* dma abort all ipp engine.	     SET-Only,   data_type: NULL */
	KDRV_IPP_PARAM_ID_MAX,
} KDRV_IPP_PARAM_ID;


typedef struct{
	UINT32 reg_base_addr;
	UINT32 reg_num;
	KDRV_IPP_ENG id;
}KDRV_IPP_DTSI_ENG_INFO;

typedef struct{
	UINT8 eng_num;
	KDRV_IPP_DTSI_ENG_INFO *eng_info;
	KDRV_IME_PM_INFO pm_info[KDRV_IME_PM_SET_IDX_MAX];
}KDRV_IPP_DTSI_INFO;

typedef enum{
	KDRV_IPP_JOB_ST_OP_GET_DTSI_INFO=1,
	KDRV_IPP_JOB_ST_OP__ID_MAX
}KDRV_IPP_JOB_START_OP;

typedef struct {
	UINT32 job_owner;
	UINT32 write_reg_mode;	/* KDRV_IPP_PROC_MODE_LINKLIST / KDRV_IPP_PROC_MODE_CPU */
	UINT32 op;
	KDRV_IPP_JOB_START_OP job_st_op;
	UINT32 (*job_start_cb)(void *hdl, UINT32 param_id, void *data);

} KDRV_IPP_TRIG_PARAM;

typedef struct {
	UINT32 base_info;
} KDRV_IPP_NODE_INFO;

UINT32 kdrv_ipp_buf_query(KDRV_IPP_CTX_BUF_CFG cfg);
INT32 kdrv_ipp_init(KDRV_IPP_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size);
INT32 kdrv_ipp_uninit(void);
INT32 kdrv_ipp_open(UINT32 chip, UINT32 engine);
INT32 kdrv_ipp_close(UINT32 chip, UINT32 engine);
INT32 kdrv_ipp_set(UINT32 dev_id, UINT32 param_id, void *p_data);
INT32 kdrv_ipp_get(UINT32 dev_id, UINT32 param_id, void *p_data);
INT32 kdrv_ipp_trigger(UINT32 id,  void *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
INT32 kdrv_ipp_direct_drop_old_job(UINT32 id, UINT32 last_addr);
INT32 kdrv_ipp_direct_trigger(UINT32 id,  void *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#endif //_KDRV_IPP_H_
