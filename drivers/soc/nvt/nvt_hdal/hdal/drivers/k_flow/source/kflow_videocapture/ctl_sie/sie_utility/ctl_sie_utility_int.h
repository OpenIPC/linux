/**
       ctrl SIE utility

    .

    @file       ctl_sie_utility_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _CTL_SIE_UTILITY_INT_H_
#define _CTL_SIE_UTILITY_INT_H_

#if defined (__LINUX)
#include <linux/string.h>
#include <linux/of.h>
#include <linux/slab.h>
#elif defined (__FREERTOS)
#include <string.h>
#include <malloc.h>
#endif

#include "kwrap/cpu.h"
#include "kwrap/mem.h"
#include "kwrap/util.h"
#include "kwrap/flag.h"
#include "kwrap/list.h"
#include "ctl_sie_dbg.h"
#include "ctl_sie_int.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#include "comm/hwclock.h"
#include <kdrv_builtin/kdrv_builtin.h>

#define CTL_SIE_MAX_SUPPORT_ID      KDRV_SIE_MAX_ENG    //reference max support id from kdrv
#define CTL_SIE_FORCE_REG_DBG_CB    ENABLE              //disable when remove ctl_sie_debug_int.c source code

typedef struct vos_list_head CTL_SIE_TEST_LIST_HEAD;

/**
    SIE limitation infor
    sync to KDRV_SIE_LIMIT
*/
typedef KDRV_SIE_LIMIT CTL_SIE_LIMIT;

/**
    type for  CTL_SIE_MCLK
    must correspond to KDRV_SIE_MCLK_INFO
*/
typedef struct {
	BOOL                    clk_en;
	CTL_SIE_MCLK_SRC_SEL    mclk_src_sel;
	UINT32                  clk_rate;
	CTL_SIE_MCLK_ID         mclk_id_sel;
} CTL_SIE_MCLK_INFO;

/**
    CCIR YUV order
    must correspond to KDRV_SIE_YUV_ORDER
*/
typedef enum {
	CTL_SIE_YUYV = 0,
	CTL_SIE_YVYU,
	CTL_SIE_UYVY,
	CTL_SIE_VYUY,
	ENUM_DUMMY4WORD(CTL_SIE_YUV_ORDER)
} CTL_SIE_YUV_ORDER;

/**
    CCIR FORMAT
    must correspond to DAL_SIE_DVI_FORMAT_SEL
    sensor drv bt601 + sd   --> sie bt601 + sd
    sensor drv bt656 + sd   --> sie bt656 + sd
    sensor drv bt709 + hd   --> sie bt601 + hd
    sensor drv bt1120 + hd  --> sie bt656 + hd
*/
typedef enum {
	CTL_DVI_FMT_CCIR601  = 0,    ///< CCIR 601
	CTL_DVI_FMT_CCIR656_EAV,     ///< CCIR 656 EAV
	CTL_DVI_FMT_CCIR656_ACT,     ///< CCIR 656 ACT
	ENUM_DUMMY4WORD(CTL_SIE_DVI_FORMAT_SEL)
} CTL_SIE_DVI_FORMAT_SEL;

/**
    CCIR MODE
    must correspond to DAL_SIE_DVI_IN_MODE_SEL
*/
typedef enum {
	CTL_DVI_MODE_SD  = 0,   ///< SD mode (8 bits)
	CTL_DVI_MODE_HD,        ///< HD mode (16bits)
	CTL_DVI_MODE_HD_INV,    ///< HD mode (16bits) with Byte Inverse
	ENUM_DUMMY4WORD(CTL_SIE_DVI_IN_MODE_SEL)
} CTL_SIE_DVI_IN_MODE_SEL;

/**
    CCIR internal info
    must correspond to DAL_SIE_CCIR_INFO
*/
typedef struct {
	CTL_SIE_YUV_ORDER yuv_order;
	CTL_SIE_DVI_FORMAT_SEL fmt;
	CTL_SIE_DVI_IN_MODE_SEL dvi_mode;
	BOOL b_filed_en;                ///< (HDAL)for CCIR interlaced
	BOOL b_filed_sel;               ///< (HDAL)select data for CCIR interlaced
	BOOL b_ccir656_vd_sel;          ///< (SenDrv)656 vd mode, 0 --> interlaced(field change as VD), 1 --> progressive(SAV + V Blank status)
	BOOL b_auto_align;              ///< (Kdrv)CCIR656 mux sensor data auto alignment
	UINT8 data_period;              ///< (SenDrv)for mux sensor, 0 --> 1 input(YUYV), 1 --> 2 input(YYUUYYVV)
	UINT8 data_idx;                 ///< (HDAL)select data idx 0/1 when mux sensor input
} CTL_SIE_CCIR_INFO_INT;

/**
    sie signal receive mode
*/
typedef enum {
	CTL_SIE_IN_PARA_MSTR_SNR = 0,   ///< Parallel Master Sensor
	CTL_SIE_IN_PARA_SLAV_SNR,       ///< Parallel Slave Sensor
	CTL_SIE_IN_PATGEN,              ///< Self Pattern-Generator
	CTL_SIE_IN_VX1_IF0_SNR,         ///< Vx1 Sensor
	CTL_SIE_IN_CSI_1,               ///< Serial Sensor from CSI-1
	CTL_SIE_IN_CSI_2,               ///< Serial Sensor from CSI-2
	CTL_SIE_IN_CSI_3,               ///< Serial Sensor from CSI-3
	CTL_SIE_IN_CSI_4,               ///< Serial Sensor from CSI-4
	CTL_SIE_IN_CSI_5,               ///< Serial Sensor from CSI-5
	CTL_SIE_IN_CSI_6,               ///< Serial Sensor from CSI-6
	CTL_SIE_IN_CSI_7,               ///< Serial Sensor from CSI-7
	CTL_SIE_IN_CSI_8,               ///< Serial Sensor from CSI-8
	CTL_SIE_IN_AHD,
	CTL_SIE_IN_SLVS_EC,             ///< Serial Sensor from SLVS-EC
	CTL_SIE_IN_VX1_IF1_SNR,         ///< Vx1 Sensor, interface 1
	ENUM_DUMMY4WORD(CTL_SIE_ACT_MODE)
} CTL_SIE_ACT_MODE;

/**
    sie pclk source select
    must correspond to KDRV_SIE_PXCLKSRC_SEL
*/
typedef enum {
	CTL_SIE_PXCLKSRC_OFF,       ///< SIE pixel-clock disable
	CTL_SIE_PXCLKSRC_PAD,       ///< SIE pixel-clock enabled, source as pixel-clock-pad
	CTL_SIE_PXCLKSRC_PAD_AHD,   ///< SIE pixel-clock enabled, source as AHD_PAD
	CTL_SIE_PXCLKSRC_MCLK,      ///< SIE pixel-clock enabled, source as MCLK (SIE_PXCLK from SIE_MCLK, SIE2_PXCLK from SIE_MCLK2, SIE3_PXCLK from SIE_MCLK2, SIE4_PXCLK from SIE_MCLK2)
	///< Usually for Pattern Gen Only
	CTL_SIE_PXCLKSRC_VX1_1X,    ///< SIE pixel-clock enabled, source as Vx1 1x clock (only for SIE2/3)
	CTL_SIE_PXCLKSRC_VX1_2X,    ///< SIE pixel-clock enabled, source as Vx1 2x clock (only for SIE2/3)
	ENUM_DUMMY4WORD(CTL_SIE_PXCLKSRC_SEL)
} CTL_SIE_PXCLKSRC_SEL;

typedef struct {
	UINT32 data_rate;
	CTL_SIE_PXCLKSRC_SEL pclk_src_sel;
	CTL_SIE_CLKSRC_SEL clk_src_sel;
	CTL_SIE_ACT_MODE act_mode;
	UINT32 drv_data_rate;
} CTL_SIE_CLK_INFO;

typedef struct {
	CTL_SIE_PATGEN_SEL  pat_gen_mode;
	UINT32              pat_gen_val;
	USIZE               pat_gen_src_win;
	UINT32              frame_rate;         // fps * 100
} CTL_SIE_PATGEN_PAR;

typedef struct {
	BOOL ch0_single_out_en;
	BOOL ch1_single_out_en;
	BOOL ch2_single_out_en;
} CTL_SIE_SINGLE_OUT_CTRL;

typedef struct {
	BOOL en;
	UINT32 buf_len;
	UINT32 buf_addr;
	UINT32 buf_size;
} CTL_SIE_RING_BUF_CTL;

typedef struct {
	struct vos_mem_cma_info_t cma_info;
	VOS_MEM_CMA_HDL cma_hdl;
} CTL_SIE_VOS_MEM_INFO;

typedef enum {
	CTL_SIE_NORMAL_OUT = 0,
	CTL_SIE_SINGLE_OUT,
	ENUM_DUMMY4WORD(CTL_SIE_OUT_MODE_TYPE)
} CTL_SIE_OUT_MODE_TYPE;

typedef struct {
	CTL_SIE_OUT_MODE_TYPE  out0_mode; ///< out0 mode sel
	CTL_SIE_OUT_MODE_TYPE  out1_mode; ///< out1 mode sel
	CTL_SIE_OUT_MODE_TYPE  out2_mode; ///< out2 mode sel
} CTL_SIE_DRAM_OUT_CTRL;

typedef enum {
	CTL_SIE_VD_DLY_NONE,
	CTL_SIE_VD_DLY_AUTO,

	ENUM_DUMMY4WORD(CTL_SIE_VD_DLY)
} CTL_SIE_VD_DLY;


/**
    CTL_SIE_SYS_INFO
    vd_cnt_clk / vd_cnt_clk_base = vd time (s)
    hd_cnt_clk / hd_cnt_clk_base = hd time (s)
*/
typedef struct {
	/* vd */
	UINT32 vd_cnt_clk_base;     // units : Hz
	UINT32 vd_cnt_clk;          // units : vd_cnt_clk_base (apb clk)
	UINT32 vd_cnt_pxl;          // units : pixel
	UINT32 vd_pxl_max;          // units : pixel
	/* hd */
	UINT32 hd_cnt_clk_base;     // units : Hz
	UINT32 hd_cnt_clk;          // units : hd_cnt_clk_base (apb clk)
	UINT32 hd_cnt_line;         // units : line
	UINT32 hd_line_end;         // units : line
} CTL_SIE_SYS_INFO;

typedef enum {
	CTL_SIE_INT_ITEM_DMA_OUT = CTL_SIE_ITEM_MAX + 1,    ///< [Set    ] data_type: BOOL
	CTL_SIE_INT_ITEM_INTE,                              ///< [Set    ] data_type: UINT32, set interrupte enable
	CTL_SIE_INT_ITEM_BP1,                               ///< [Set/Get] data_type: UINT32, set 0 to disable bp
	CTL_SIE_INT_ITEM_BP2,                               ///< [Set/Get] data_type: UINT32, set 0 to disable bp
	CTL_SIE_INT_ITEM_BP3,                               ///< [Set/Get] data_type: UINT32, set 0 to disable bp
	CTL_SIE_INT_ITEM_CH0_ADDR,                          ///< [Set/Get] data_type: UINT32, need 4 byte align
	CTL_SIE_INT_ITEM_CH1_ADDR,                          ///< [Set/Get] data_type: UINT32, need 4 byte align
	CTL_SIE_INT_ITEM_CH2_ADDR,                          ///< [Set/Get] data_type: UINT32, need 4 byte align
	CTL_SIE_INT_ITEM_CA_ROI,                            ///< [Set/Get] data_type: CTL_SIE_CA_ROI
	CTL_SIE_INT_ITEM_LA_ROI,                            ///< [Set/Get] data_type: CTL_SIE_LA_ROI
	CTL_SIE_INT_ITEM_OB,                                ///< [Set/Get] data_type: CTL_SIE_OB_PARAM
	CTL_SIE_INT_ITEM_CA,                                ///< [Set/Get] data_type: CTL_SIE_CA_PARAM
	CTL_SIE_INT_ITEM_LA,                                ///< [Set/Get] data_type: CTL_SIE_LA_PARAM
	CTL_SIE_INT_ITEM_VA,                                ///< [Set/Get] data_type: CTL_SIE_VA_PARAM
	CTL_SIE_INT_ITEM_CGAIN,                             ///< [Set/Get] data_type: CTL_SIE_CGAIN
	CTL_SIE_INT_ITEM_DGAIN,                             ///< [Set/Get] data_type: CTL_SIE_DGAIN
	CTL_SIE_INT_ITEM_DPC,                               ///< [Set/Get] data_type: CTL_SIE_DPC
	CTL_SIE_INT_ITEM_ECS,                               ///< [Set/Get] data_type: CTL_SIE_ECS
	CTL_SIE_INT_ITEM_COMPAND,                           ///< [Set/Get] data_type: CTL_SIE_COMPANDING
	CTL_SIE_INT_ITEM_PXCLK,                             ///< [Set/Get] data_type: CTL_SIE_PXCLKSRC_SEL
	CTL_SIE_INT_ITEM_CLK,                               ///< [    Get] data_type: UINT32
	CTL_SIE_INT_ITEM_LIMIT,                             ///< [    Get] data_type: KDRV_SIE_LIMIT
	CTL_SIE_INT_ITEM_KDRV_PARAM,                        ///< [    Get] data_type:
	CTL_SIE_INT_ITEM_CH_OUT_MODE,                       ///< [Set    ] data_type: CTL_SIE_OUTPUT_MODE_TYPE
	CTL_SIE_INT_ITEM_SINGLE_OUT_CTL,                    ///< [Set/Get] data_type: CTL_SIE_SINGLE_OUT_CTRL,
	CTL_SIE_INT_ITEM_RING_BUF,                          ///< [Set    ] data_type: CTL_SIE_RING_BUF_CTL
	CTL_SIE_INT_ITEM_MD,                                ///< [Set    ] data_type: CTL_SIE_MD_INFO
	CTL_SIE_INT_ITEM_MD_RSLT,                           ///< [    Get] data_type: CTL_SIE_ISP_MD_RSLT
	CTL_SIE_INT_ITEM_REF_LOAD_ID,                       ///< [Set    ] data_type: CTL_SIE_ID
	CTL_SIE_INT_ITEM_DVS_EN,                            ///< [Set/Get] data_type: BOOL, ENABLE : SIE parsing dvs data
	CTL_SIE_INT_ITEM_DVS_SZ,                            ///< [Set/Get] data_type: USIZE, dvs data dram output size
	CTL_SIE_INT_ITEM_VD_DLY,                            ///< [Set/Get] data_type: CTL_SIE_VD_DLY, multi-frame vd delay in SIE
	CTL_SIE_INT_ITEM_SYS_INFO,                          ///< [    Get] data_type: CTL_SIE_SYS_INFO
	CTL_SIE_INT_ITEM_CCIR_H_STS,                        ///< [    Get] data_type: UINT32
	CTL_SIE_INT_ITEM_MAX,
	CTL_SIE_INT_ITEM_REV = 0x8000000000000000,
	ENUM_DUMMY4WORD(CTL_SIE_INT_ITEM)
} CTL_SIE_INT_ITEM;
STATIC_ASSERT((CTL_SIE_INT_ITEM_MAX &CTL_SIE_INT_ITEM_REV) == 0);
#define CTL_SIE_IGN_CHK CTL_SIE_INT_ITEM_REV    //only support set/get function

typedef struct {
	CTL_SIE_OUT_DEST        out_dest;           ///< out dest.
	CTL_SIE_DATAFORMAT      data_fmt;           ///< sie data format
	CTL_SIE_CCIR_INFO_INT   ccir_info_int;      ///< ccir internal information
	CTL_SIE_IO_SIZE_INFO    io_size_info;       ///< sie io size info
	CTL_SIE_FLIP_TYPE       flip;               ///< flip type
	BOOL                    encode_en;          ///< raw encode enable
	CTL_SIE_ENC_RATE_SEL    enc_rate;           ///< raw encode rate, default CTL_SIE_ENC_58
	CTL_SIE_CHGSENMODE_INFO chg_senmode_info;
	BOOL                    dma_abort;
	CTL_SIE_DVS_CODE        dvs_code;
} CTL_SIE_ITEM_PARAM;

typedef struct {
	CTL_SIE_RAW_PIX     rawcfa_act;             ///< raw cfa start pixel for act window
	CTL_SIE_RAW_PIX     rawcfa_crp;             ///< raw cfa start pixel for crop window
	URECT               sie_act_win;            ///< sie active window
	UINT32 out_ch_lof[CTL_SIE_CH_MAX];          ///< line offset of each output channel
	CTL_SIE_PATGEN_PAR  pat_gen_param;          ///< pattern gen parameters
	CTL_SIE_SIGNAL      signal;                 ///< signal info
	UINT32              total_frame_num;        // multi frame info
	BOOL                dvs_en;
	USIZE               dvs_size;           ///< dvs sensor only
} CTL_SIE_RTC_CTRL_OBJ;

// internal ctrl
typedef struct {
	CTL_SIE_ISRCB           int_isrcb_fp;   ///< internal isr cb function pointer
	CTL_SIE_CB_INFO         ext_isrcb_fp;   ///< external isr cb function pointer
	CTL_SIE_CB_INFO         bufiocb;        ///< buf in/out isr cb function pointer
	CTL_SIE_CB_INFO         direct_cb_fp;   ///< direct mode cb function pointer, used for sync with ipp unit
	BOOL                    dma_out;        ///< all dma output enable(active enable)
	UINT64                  update_item;    ///< 1<< update_item from CTL_SIE_ITEM or CTL_SIE_INT_ITEM
	CTL_SIE_INTE            inte;           ///< interrupt enable
	UINT32                  out_ch_addr[CTL_SIE_CH_MAX]; ///< address of each output channel
	CTL_SIE_HEADER_INFO     head_info[CTL_SIE_HEAD_IDX_MAX];    ///< sie push out header address
	CTL_SIE_FRM_CTL_INFO    frame_ctl_info;
	CTL_SIE_TRIG_INFO       trig_info;
	CTL_SIE_CLK_INFO        clk_info;
	CTL_SIE_BUF_INFO        buf_info;
	CTL_SIE_RST_FC_STATUS   rst_fc_sts;
	UINT32                  row_time;
	UINT32                  bp1;
	UINT32                  bp2;
	UINT32                  bp3;
	CTL_SIE_DRAM_OUT_CTRL   ch_out_mode;
	CTL_SIE_SINGLE_OUT_CTRL sin_out_ctl;
	CTL_SIE_RING_BUF_CTL    ring_buf_ctl;
	CTL_SIE_ID              ref_load_id;    ///< hw global load, 560 only support SIE1 ref SIE2
	CTL_SIE_VD_DLY          vd_dly;
	UINT32                  sen_dft_fps;        ///< (fps * 100)

	//isp ctrl
	KDRV_SIE_CA_ROI         ca_roi;         ///< ca crop roi
	KDRV_SIE_LA_ROI         la_roi;         ///< la crop roi

	CTL_SIE_OB_PARAM        ob_param;
	CTL_SIE_CA_PARAM        ca_param;
	KDRV_SIE_LA_PARAM       la_param;
	CTL_SIE_CGAIN           cgain_param;
	CTL_SIE_DGAIN           dgain_param;
	KDRV_SIE_DPC            dpc_param;
	KDRV_SIE_ECS            ecs_param;
	CTL_SIE_COMPANDING      companding_param;
	CTL_SIE_MD_PARAM        md_param;
	BOOL                    alg_func_en[CTL_SIE_ALG_TYPE_MAX];  ///< algorithm id(iq, ae, awb, af...) */

	//runtime change obj
	CTL_SIE_RTC_CTRL_OBJ    rtc_obj;        ///< runtime change obj
} CTL_SIE_CTRL_OBJ;

typedef struct {
	UINT32                  tag;
	UINT32                  kdf_hdl;
	UINT32                  evt_hdl;

	/* CTL_SIE_OPEN_CFG */
	CTL_SIE_ID              id;
	CTL_SIE_ID              dupl_src_id;
	CTL_SEN_ID              sen_id;
	ISP_ID                  isp_id;
	CTL_SIE_FLOW_TYPE       flow_type;

	//
	PCTL_SEN_OBJ p_sen_obj;
	BOOL dvi_interlace_skip_en;

	/* CTL_SIE_ITEM */
	CTL_SIE_ITEM_PARAM      param;
	/* internal ctrl parameters */
	CTL_SIE_CTRL_OBJ        ctrl;

	/* runtime change CTL_SIE_ITEM */
	CTL_SIE_ITEM_PARAM      rtc_param;
	/* runtime change internal ctrl parameters */
	CTL_SIE_RTC_CTRL_OBJ    rtc_ctrl;

	/* ready for load CTL_SIE_ITEM */
	CTL_SIE_ITEM_PARAM      load_param;
	/* ready for load internal ctrl parameters */
	CTL_SIE_RTC_CTRL_OBJ    load_ctrl;
} CTL_SIE_HDL;

typedef struct {
	UINT64 set_load_flg;
	UINT32 get_cnt_flg;
	UINT32 cfg_start_bp;
	UINT32 last_isr_sts;
	UINT32 shdr_head_bp_idx;
	BOOL isr_dram_end_flg;
	BOOL isr_valid_vd_flg;      //vd proc will set TRUE, bp3 proc will clear to FALSE
	BOOL bp3_rst_en;
	BOOL isr_bp3_rst_flg;
	BOOL isr_load_en_flg;
	BOOL resume_rst_flg;        //reset ctrl info when resume
	BOOL interlace_skip_flg;
} CTL_SIE_INT_CTL_FLG;

typedef struct {
	UINT32 context_num;
	UINT32 context_used;    //current used context number
	UINT32 context_idx[CTL_SIE_MAX_SUPPORT_ID];
	UINT32 contex_size;;    //each context buf size
	UINT32 start_addr;
	UINT32 req_size;        //total ctl_sie ctx require size (context_num*contex_size)
	UINT32 total_buf_sz;    //total ctx require size (ctl_sie+kdf_sie+kdrv_sie)s
	BOOL dbg_buf_alloc;
	BOOL auto_alloc_mem;
	CTL_SIE_VOS_MEM_INFO vos_mem_info;  //private memory, auto allocate memory when init address is 0
} CTL_SIE_HDL_CONTEXT;

#define CTL_SIE_GET_ID(hdl) ctl_sie_hdl_conv2_id(hdl)

#define CTL_SIE_DIV_U64(x, y) ctl_sie_uint64_dividend(x, y)
#define CTL_SIE_MAX(x, y) ctl_sie_max(x, y)
#define CTL_SIE_MIN(x, y) ctl_sie_min(x, y)
#define CTL_SIE_LCM(x, y) ctl_sie_lcm(x, y)
#define CTL_SIE_CHK_ALIGN(src, align) ctl_sie_chk_align(src, align)

extern BOOL ctl_sie_module_chk_id_valid(CTL_SIE_ID id);
extern CTL_SIE_ID ctl_sie_hdl_conv2_id(UINT32 hdl);
extern BOOL ctl_sie_module_chk_is_raw(CTL_SEN_MODE_TYPE mode_type);
extern UINT32 ctl_sie_uint64_dividend(UINT64 dividend, UINT32 divisor);
extern UINT32 ctl_sie_max(UINT32 x, UINT32 y);
extern UINT32 ctl_sie_min(UINT32 x, UINT32 y);
extern UINT32 ctl_sie_lcm(UINT32 x, UINT32 y);
extern BOOL ctl_sie_chk_align(UINT32 x, UINT32 y);
extern UINT32 ctl_sie_get_hdl_tag(void);
extern CTL_SIE_HDL *ctl_sie_get_hdl(CTL_SIE_ID id);
extern CTL_SIE_HDL_CONTEXT *ctl_sie_get_ctx(void);
extern CTL_SIE_LIMIT *ctl_sie_limit_query(CTL_SIE_ID id);
extern CTL_SIE_STATUS ctl_sie_get_state_machine(CTL_SIE_ID id);
extern PCTL_SEN_OBJ ctl_sie_get_sen_obj(CTL_SIE_ID id);
extern INT32 ctl_sie_get_sen_cfg(CTL_SIE_ID id, CTL_SEN_CFGID cfg_id, void *data);
extern UINT64 ctl_sie_util_get_syst_timestamp(void);
extern UINT32 ctl_sie_util_get_timestamp(void);
extern INT32 ctl_sie_util_os_malloc(CTL_SIE_VOS_MEM_INFO *vod_mem_info, UINT32 req_size);
extern INT32 ctl_sie_util_os_mfree(CTL_SIE_VOS_MEM_INFO *vod_mem_info);
extern void *ctl_sie_util_os_malloc_wrap(UINT32 want_size);
extern void ctl_sie_util_os_mfree_wrap(void *p_buf);

//debug api

typedef enum {
	CTL_SIE_DBG_TS_VD = 0,
	CTL_SIE_DBG_TS_BP3,
	CTL_SIE_DBG_TS_ADDR_RDY,
	CTL_SIE_DBG_TS_DRAMEND,
	CTL_SIE_DBG_TS_RCVMSG,
	CTL_SIE_DBG_TS_UNLOCK,
	CTL_SIE_DBG_TS_MAX
} CTL_SIE_DBG_TS_EVT;

typedef struct {
	BOOL(*chk_msg_type)(CTL_SIE_ID id, UINT32 chk_type);
	UINT32(*query_dbg_buf)(void);
	void(*set_dbg_buf)(UINT32 id, UINT32 buf_addr);
	void(*set_ts)(CTL_SIE_ID id, CTL_SIE_DBG_TS_EVT evt, UINT64 sie_fc);    //set timestamp record
	void(*reset_ts)(CTL_SIE_ID id);                                         //reset timestamp
	void(*dump_buf_io)(CTL_SIE_ID id, CTL_SIE_BUF_IO_CFG buf_io, UINT32 total_size, UINT32 header_addr);    //dump sie buffer in/out info
	void(*set_isr_ioctl)(CTL_SIE_ID id, UINT32 status, UINT32 buf_ctl_type, CTL_SIE_HEAD_IDX head_idx);     //set isr control info
	void(*set_frm_ctrl_info)(CTL_SIE_ID id, CTL_SIE_FRM_CTL_INFO *frm_ctrl);                                //set frame control info
	void(*set_isp_cb_t_log)(ISP_ID id, ISP_EVENT evt, UINT64 fc, UINT32 ts_start, UINT32 ts_end);           //set isp callback timestamp info
	void(*isp_cb_t_dump)(int (*dump)(const char *fmt, ...)); //isp callback timestamp dump
	void(*set_proc_t)(CTL_SIE_ID id, CHAR *proc_name, CTL_SIE_PROC_TIME_ITEM item); //set timestamp record
	void(*upd_sysdbg_info)(CTL_SIE_ID id, UINT32 evt, UINT64 sie_fc);   // update system information
	void(*get_buf_wp_info)(CTL_SIE_ID id, UINT32 *ddr_id, UINT32 *dis_wp);          //get sie out ddr id, and release_before_push_en
} CTL_SIE_INT_DBG_TAB;

extern CTL_SIE_INT_DBG_TAB *ctl_sie_get_dbg_tab(void);
extern void ctl_sie_reg_dbg_tab(CTL_SIE_INT_DBG_TAB *dbg_tab);
extern void ctl_sie_set_dbg_lvl(CTL_SIE_DBG_LVL dbg_lvl);
#endif //_CTL_SIE_UTILITY_INT_H_

