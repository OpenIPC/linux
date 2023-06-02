#ifndef _IPP_DEBUG_INT_H_
#define _IPP_DEBUG_INT_H_
/**
    ipp_debug_int.h


    @file       ipp_debug_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "kwrap/cpu.h"
#include "kflow_videoprocess/ctl_ipp_util.h"
#include "ctl_ipp_dbg.h"
#include "ctl_ipp_int.h"
#include "kdrv_videoprocess/kdrv_ipp.h"

int ctl_ipp_int_printf(const char *fmtstr, ...);

/**
	time stamp log
*/
#define CTL_IPP_DBG_TS_ENABLE (1)
#define CTL_IPP_DBG_TS_LOG_NUM (16)

typedef enum {
	CTL_IPP_DBG_TS_SND = 0,		/* hdal send event */
	CTL_IPP_DBG_TS_RCV,			/* ctl_ipp thread receive event */
	CTL_IPP_DBG_TS_PUTJOB,		/* ctl_ipp putjob to kdrv_ipp */
	CTL_IPP_DBG_TS_PROC_START,	/* kdrv_ipp thread process start */
	CTL_IPP_DBG_TS_PROC_END,	/* kdrv_ipp thread process end */
	CTL_IPP_DBG_TS_ENG_LOAD,	/* engine trigger/load */
	CTL_IPP_DBG_TS_ENG_START,	/* engine isr frame start */
	CTL_IPP_DBG_TS_ENG_END,		/* engine isr frame end */
	CTL_IPP_DBG_TS_JOB_END,		/* ctl_ipp buf thread callback to hdal push/release buffer */
	CTL_IPP_DBG_TS_MAX
} CTL_IPP_DBG_TS_TYPE;

typedef struct {
	UINT32 handle;
	UINT32 vd_cnt;
	UINT32 ts[CTL_IPP_DBG_TS_MAX];
	INT32 err_msg;
	CTL_IPP_LIST_HEAD list;
} CTL_IPP_DBG_TS_NODE;

UINT32 ctl_ipp_dbg_ts_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query);
INT32 ctl_ipp_dbg_ts_pool_uninit(void);
CTL_IPP_DBG_TS_NODE *ctl_ipp_dbg_get_ts_node(UINT32 handle, UINT32 vd_cnt);
void ctl_ipp_dbg_set_ts_node_ready(CTL_IPP_DBG_TS_NODE *p_node, INT32 err);
void ctl_ipp_dbg_set_ts(CTL_IPP_DBG_TS_NODE *p_node, UINT32 evt, UINT32 timestamp);
void ctl_ipp_dbg_ts_dump(int (*dump)(const char *fmt, ...));

/**
	debug base info after handle close
*/
typedef struct {
	CHAR name[CTL_IPP_HANDLE_NAME_MAX];
	CTL_IPP_FLOW_TYPE flow;
	UINT32 ctl_snd_evt_cnt;		/* ctl send event count */
	UINT32 ctl_frm_str_cnt;		/* ctl ife frame start count */
	UINT32 ctl_proc_end_cnt;	/* ctl process end count */
	UINT32 ctl_drop_cnt;		/* ctl drop frame count, include ctl_drop, kdf_drop */
	UINT32 kdf_snd_evt_cnt;		/* kdf send event count */
	UINT32 kdf_frm_str_cnt;		/* kdf ife frame start count */
	UINT32 kdf_proc_end_cnt;	/* kdf process end count */
	UINT32 kdf_drop_cnt;		/* kdf drop frame count, include ctl_drop, kdf_drop */
	UINT32 kdf_reset_cnt;		/* kdf drop frame count, include ctl_drop, kdf_drop */
	UINT32 in_buf_re_cb_cnt;	/* direct mode input buffer release count */
	UINT32 in_buf_re_cnt;		/* direct mode input buffer release count */
	UINT32 in_buf_drop_cnt;		/* direct mode input buffer drop count */
	UINT32 in_pro_skip_cnt;		/* direct mode skip process cfg count */
} CTL_IPP_DBG_BASE_INFO;


/**
	handle debug information
*/
BOOL ctl_ipp_dbg_hdl_is_init(void);
void ctl_ipp_dbg_hdl_pool_set(CTL_IPP_HANDLE_POOL *pool);
void ctl_ipp_dbg_hdl_stripe_set(UINT32 kdf_hdl, KDRV_DCE_STRP_RSLT *p_strp_info);
void ctl_ipp_dbg_hdl_dump_all(int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_hdl_dump_buf(UINT32 hdl, int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_baseinfo_dump(UINT32 baseinfo, BOOL is_from_pool, int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_savefile(CHAR *f_name, UINT32 addr, UINT32 size);
void ctl_ipp_dbg_saveyuv(UINT32 handle, CTL_IPP_OUT_BUF_INFO *p_buf);
void ctl_ipp_dbg_saveyuv_cfg(CHAR *handle_name, CHAR *filepath, UINT32 count, UINT8 bit);
void ctl_ipp_dbg_save_vdofrm_image(VDO_FRAME *p_vdofrm, CHAR *f_name);
void ctl_ipp_dbg_dump_dtsi_flow_begin(CTL_IPP_HANDLE *p_hdl, KDRV_IPP_TRIG_PARAM *p_trig_param);
void ctl_ipp_dbg_dump_dtsi(CHAR *f_name);

/**
	debug common buffer infor
*/
#define CTL_IPP_DBG_BUF_LOG_ENABLE (1)
#define CTL_IPP_DBG_BUF_LOG_NUM (16)

typedef struct {
	UINT32 handle;
	UINT32 pid;
	UINT32 type;
	UINT32 ts_start;
	UINT32 ts_end;
	UINT32 buf_addr;
} CTL_IPP_DBG_BUF_CBTIME_LOG;

void ctl_ipp_dbg_outbuf_cbtime_dump(int (*dump)(const char *fmt, ...));
void ctl_ipp_dbg_outbuf_log_set(UINT32 hdl, CTL_IPP_BUF_IO_CFG io_type, CTL_IPP_OUT_BUF_INFO *p_buf, UINT64 ts_start, UINT64 ts_end);
void ctl_ipp_dbg_inbuf_log_set(UINT32 hdl, UINT32 buf_addr, CTL_IPP_CBEVT_IN_BUF_MSG type, UINT64 ts_start, UINT64 ts_end);
void ctl_ipp_dbg_inbuf_cbtime_dump(int (*dump)(const char *fmt, ...));


/**
	debug context buffer log
*/
#define CTL_IPP_DBG_CTX_BUF_NAME_MAX (16)

typedef enum {
	CTL_IPP_DBG_CTX_BUF_QUERY = 0,
	CTL_IPP_DBG_CTX_BUF_ALLOC,
	CTL_IPP_DBG_CTX_BUF_FREE,
	CTL_IPP_DBG_CTX_BUF_OP_MAX
} CTL_IPP_DBG_CTX_BUF_OP;

typedef struct {
	CHAR name[CTL_IPP_DBG_CTX_BUF_NAME_MAX];
	CTL_IPP_DBG_CTX_BUF_OP op;
	UINT32 size;
	UINT32 addr;
	UINT32 cfg_num;
} CTL_IPP_DBG_CTX_BUF_LOG;

void ctl_ipp_dbg_ctxbuf_log_set(CHAR *name, CTL_IPP_DBG_CTX_BUF_OP op, UINT32 size, UINT32 addr, UINT32 n);
void ctl_ipp_dbg_ctxbuf_log_dump(int (*dump)(const char *fmt, ...));


/**
	debug isr sequence in direct mode
*/
#define CTL_IPP_DBG_DIRECT_ISR_ENABLE (0)
#define CTL_IPP_DBG_DIRECT_ISR_NUM 32
#define CTL_IPP_DBG_DIR_ISR_NAME_MAX	(16)

typedef struct {
	CHAR name[CTL_IPP_DBG_DIR_ISR_NAME_MAX];
	UINT32 cnt;
	UINT32 addr;
} CTL_IPP_DBG_DIR_ISR_SEQ;


void ctl_ipp_dbg_dump_direct_isr_sequence(UINT32 hdl);
void ctl_ipp_dbg_set_direct_isr_sequence(UINT32 hdl, CHAR* name, UINT32 addr);

/**
	debug privcay mask
*/
#define CTL_IPP_DBG_PRIMASK_ENABLE (1)
#define CTL_IPP_DBG_PRIMASK_COLOR_POOL (8)

typedef struct {
	UINT8 enable;
	UINT8 color[CTL_IPP_DBG_PRIMASK_COLOR_POOL][3];
	USIZE msk_size;
} CTL_IPP_DBG_PRIMASK_CTL;

typedef enum {
	CTL_IPP_DBG_PRIMASK_CFG_SIZE = 2,	/* data_type: USIZE */
	CTL_IPP_DBG_PRIMASK_CFG_MAX,
} CTL_IPP_DBG_PRIMASK_CFG;

INT32 ctl_ipp_dbg_primask_cb(UINT32 msg, void *in, void *out);
void ctl_ipp_dbg_primask_en(UINT32 en);
void ctl_ipp_dbg_primask_cfg(UINT32 cfg, void *data);

#endif //_IPP_DEBUG_INT_H_
