/**
    SIE CTRL Layer

    @file       sie_ctrl_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _SIE_CTRL_INT_H
#define _SIE_CTRL_INT_H

#define _INLINE static inline
#include "kflow_videocapture/ctl_sen.h"
#include "kflow_videocapture/ctl_sie.h"
#include "kdf_sie_int.h"
#include "ctl_sie_isp_task_int.h"
#include "ctl_sie_dbg.h"

#define CTL_SIE_RAW_COMPRESS_BIT        12
#define CTL_SIE_PAT_GEN_SRC_W_PAD_RATIO 5
#define CTL_SIE_PAT_GEN_SRC_H_PAD       8
#define CTL_SIE_PAT_GEN_SRC_WIN_MAX     ALIGN_FLOOR(16383, 4)
#define CTL_SIE_BP_MIN_END_LINE         200     //bp to crop_end line counter, for error handle
#define CTL_SIE_BP_STR_LINE_DIRECT      100 //crop start to bp line counter(direct mode)
#define CTL_SIE_BP_STR_LINE_DIRECT_MIN  30  //crop start to bp line counter(direct mode) minimum line
#define CTL_SIE_DFT_INT_EN  (CTL_SIE_INTE_VD | CTL_SIE_INTE_BP3 | CTL_SIE_INTE_CROPEND) //VD/BP3/Crop_End for buffer control

#define CTL_SIE_BP3_NEXTVD_MIN_US   3000    // 3000us, bp3 to next_vd min time, for sie latch parameters in next vd
#define CTL_SIE_BP3_CROPEND_LINE    2       // 2 line, bp3 to crop_end line_num

#define CTL_SIE_DFT_ENC_RATE CTL_SIE_ENC_50


#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
#define CTL_SIE_TO_MS 10000
#else
#define CTL_SIE_TO_MS 2000
#endif

// funtion type for sie
typedef INT32(*CTL_SIE_ISRCB)(UINT32, UINT32, void *, void *);
typedef INT32(*CTL_SIE_SET_FP)(CTL_SIE_ID, void *);
typedef INT32(*CTL_SIE_GET_FP)(CTL_SIE_ID, void *);

/**
            SIE STATE MACHINE

                          ----- SUSPEND -----
                         / --->         <--- \
                 resume / /suspend   suspend\ \ resume
                       / /                    \ \
            Open      < /        Trig start    \ >
    CLOSE   ----->  READY   ----------------->  RUN
            <-----          <-----------------
            Close                Trig stop



            ======= DMA_ABORT =======

            READY   -----------> DMA_ABORT -----> system shutdown
            RUN     -----------> DMA_ABORT -----> system shutdown
            SUSPEND -----------> DMA_ABORT -----> system shutdown

*/
typedef enum {
	CTL_SIE_STS_CLOSE = 0,
	CTL_SIE_STS_READY,
	CTL_SIE_STS_RUN,
	CTL_SIE_STS_SUSPEND,
	CTL_SIE_STS_DMA_ABORT,
	CTL_SIE_STS_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_STATUS)
} CTL_SIE_STATUS;

typedef enum {
	CTL_SIE_HEAD_IDX_CUR = 0,
	CTL_SIE_HEAD_IDX_NEXT,
	CTL_SIE_HEAD_IDX_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_HEAD_IDX)
} CTL_SIE_HEAD_IDX;

typedef enum {
	CTL_SIE_OP_OPEN = 0,
	CTL_SIE_OP_CLOSE,
	CTL_SIE_OP_TRIG_START,
	CTL_SIE_OP_TRIG_STOP,
	CTL_SIE_OP_SET,
	CTL_SIE_OP_GET,
	CTL_SIE_OP_SUSPEND,
	CTL_SIE_OP_RESUME,
	CTL_SIE_OP_DMA_ABORT,
	CTL_SIE_OP_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_OP)
} CTL_SIE_OP;

typedef enum {
	CTL_SIE_RST_FC_DONE = 0,
	CTL_SIE_RST_FC_BEGIN,
	CTL_SIE_RST_FC_NONE,
	CTL_SIE_RST_FC_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_RST_FC_STATUS)
} CTL_SIE_RST_FC_STATUS;

typedef struct {
	BOOL out_enable;
	UINT32 data_size;
} CTL_SIE_CH_OUT_INFO;

typedef struct {
	UINT32              total_size;
	CTL_SIE_CH_OUT_INFO ch_info[CTL_SIE_CH_MAX];
} CTL_SIE_BUF_INFO;

typedef enum {
	CTL_SIE_INT_DATAFORMAT_RAW  = 0x00000001, // CTL_SIE_BAYER_xx
	CTL_SIE_INT_DATAFORMAT_CCIR = 0x00000002, // CTL_SIE_YUV_xx
	CTL_SIE_INT_DATAFORMAT_DVS  = 0x00000004, // CTL_SIE_Y_8
} CTL_SIE_INT_DATAFORMAT;

typedef struct {
	UINT64      vd_cnt;             ///< sie ctl vd interrupt counter
	UINT64      crp_end_cnt;        ///< sie ctl crop end interrupt counter
	UINT64      frame_cnt;          ///< sie ctl frame counter
	UINT64      new_ok_cnt;         ///< new buffer success counter
	UINT64      new_fail_cnt;       ///< new buffer fail counter
	UINT64      buf_queue_full_cnt; ///< buffer queue full, skip new buffer
	UINT64      drop_cnt;           ///< drop buffer counter
	UINT64      push_cnt;           ///< push out frame counter
	UINT64      flush_cnt;          ///< buffer flush counter(flush only when sie_stop(wait_end) and close)
	UINT64      dir_rls_cnt;        ///< ipp direct mode buffer release cnt
	UINT64      dir_rls_fail_cnt;   ///< ipp direct mode buffer release fail cnt
	UINT64      dir_drop_cnt;       ///< ipp direct mode buffer drop cnt
	UINT64      dir_drop_fail_cnt;  ///< ipp direct mode buffer drop fail cnt
} CTL_SIE_FRM_CTL_INFO;

typedef enum {
	CTL_SIE_BUF_IDX_CUR = 0,
	CTL_SIE_BUF_IDX_NEXT,
	CTL_SIE_BUF_IDX_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_BUF_IDX)
} CTL_SIE_BUF_IDX;

/* time record */
typedef enum {
	CTL_SIE_PROC_TIME_ITEM_ENTER,
	CTL_SIE_PROC_TIME_ITEM_EXIT,
	CTL_SIE_PROC_TIME_ITEM_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_PROC_TIME_ITEM),
} CTL_SIE_PROC_TIME_ITEM;

void ctl_sie_update_inte(CTL_SIE_ID id, BOOL enable, CTL_SIE_INTE sts);
BOOL ctl_sie_chk_fmt(UINT32 sie_hdl, CTL_SIE_DATAFORMAT src, CTL_SIE_INT_DATAFORMAT chk);
void ctl_sie_hdl_update_item(CTL_SIE_ID id, UINT64 item, BOOL set_imm);
void ctl_sie_hdl_load_all(CTL_SIE_ID id);
void ctl_sie_int_set_dma_en_imm(CTL_SIE_ID id, BOOL en);
UINT32 ctl_sie_get_shdr_head_idx(CTL_SIE_ID id);

INT32 ctl_sie_module_direct_to_both(CTL_SIE_ID id);
INT32 ctl_sie_module_both_to_direct(CTL_SIE_ID id);
INT32 ctl_sie_set_load(CTL_SIE_ID id, void *data);

#endif //_SIE_CTRL_INT_H