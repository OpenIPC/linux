/**
    IPL Ctrl Layer, ISP Interface

    @file       ctl_ipp_isp.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_IPP_ISP_H
#define _CTL_IPP_ISP_H

#include "kflow_common/isp_if.h"
#include "kflow_common/type_vdo.h"
#include "ctl_ife_isp.h"
#include "ctl_ipe_isp.h"
#include "ctl_dce_isp.h"
#include "ctl_ime_isp.h"
#define CTL_IPP_ISP_REFINE_IOSIZE_STRUCT (0)
#define CTL_IPP_ISP_FULL_PARA_INIT 0

/**
	KFLOW IFE structure - vig center ratio
*/
typedef struct {
	UINT32 ratio_base;	///< ratio base
	UPOINT ch0;			///< center ratio of channel 0
	UPOINT ch1;			///< center ratio of channel 1
	UPOINT ch2;			///< center ratio of channel 2
	UPOINT ch3;			///< center ratio of channel 3
} CTL_IPP_ISP_IFE_VIG_CENT_RATIO;

/**
	KFLOW DCE structure - dc center ratio

	@note	ratio base refer to ISP_IQ_RATIO_BASE
*/
typedef struct {
	UINT32 ratio_base;	///< ratio base
	UPOINT center;		///< dc center ratio
} CTL_IPP_ISP_DCE_DC_CENT_RATIO;

/**
	KFLOW IME structure - local chroma adaptation subout size ratio

	@note	ratio base refer to ISP_IQ_RATIO_BASE
*/
typedef struct {
	UINT32 ratio_base;	///< ratio base
	UINT32 ratio;		///< LCA current subout ratio
} CTL_IPP_ISP_IME_LCA_SIZE_RATIO;

/**
    KFLOW IFE2 Struct - IFE2_FILTER_TIME
*/
typedef struct {
    UINT32	repeat_time;          ///< filter repeat times
} CTL_IPP_ISP_IFE2_FILTER_TIME;


#define CTL_IPP_ISP_OUT_CH_MAX_NUM		(6)
#define CTL_IPP_ISP_SHDR_FRAME_MAX_NUM	(4)
#define CTL_IPP_ISP_VA_MAX_WINNUM		(8)
#define CTL_IPP_ISP_VA_INDEP_NUM		(5)
#define CTL_IPP_ISP_DCE_HIST_NUM		(128)
#define CTL_IPP_ISP_DEFOG_AIRLIGHT_NUM	(3)
#define CTL_IPP_ISP_STRIPE_MAX_NUM		(16)

/**
	struct for CTL_IPP_ISP_ITEM_IOSIZE
*/
#if CTL_IPP_ISP_REFINE_IOSIZE_STRUCT
typedef struct {
	UINT16 w;
	UINT16 h;
} USIZE16;

typedef struct {
	USIZE16 max_in_sz;
	USIZE16 in_sz;
	USIZE16 out_ch[CTL_IPP_ISP_OUT_CH_MAX_NUM];
} CTL_IPP_ISP_IOSIZE;
#else
typedef struct {
	USIZE max_in_sz;
	USIZE in_sz;
	USIZE out_ch[CTL_IPP_ISP_OUT_CH_MAX_NUM];
} CTL_IPP_ISP_IOSIZE;
#endif
/**
	struct for CTL_IPP_ISP_ITEM_YUV_OUT
*/
typedef struct {
	UINT32 frame_num;
	USIZE win_size;
	USIZE blk_size;
	UINT32 lofs;
	UINT32 addr[CTL_IPP_ISP_SHDR_FRAME_MAX_NUM];
} CTL_IPP_ISP_YOUT;

/**
	struct for CTL_IPP_ISP_ITEM_DEFOG_SUBOUT
*/
typedef struct {
	UINT32 addr;
} CTL_IPP_ISP_DEFOG_SUBOUT;

/**
	struct/enum for CTL_IPP_ISP_ITEM_YUV_OUT
*/
typedef enum {
	CTL_IPP_YUV_OUT_PATH_1 = 0,
	CTL_IPP_YUV_OUT_PATH_2,
	CTL_IPP_YUV_OUT_PATH_3,
	CTL_IPP_YUV_OUT_PATH_4,
	CTL_IPP_YUV_OUT_PATH_5,
	CTL_IPP_YUV_OUT_PATH_6,
	CTL_IPP_YUV_OUT_PATH_ID_MAX,
} CTL_IPP_YUV_OUT_PATH_ID;

typedef struct {
	CTL_IPP_YUV_OUT_PATH_ID pid;
	UINT32 buf_id;
	UINT32 buf_size;
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
} CTL_IPP_ISP_YUV_OUT;

/**
	struct/enum for CTL_IPP_ISP_ITEM_STATUS_INFO
*/
typedef enum {
	CTL_IPP_ISP_STS_CLOSE,
	CTL_IPP_ISP_STS_READY,
	CTL_IPP_ISP_STS_RUN,
	CTL_IPP_ISP_STS_ID_NOT_FOUND,
	CTL_IPP_ISP_STS_MAX,
} CTL_IPP_ISP_STATUS;

typedef enum {
	CTL_IPP_ISP_FLOW_UNKNOWN = 0,
	CTL_IPP_ISP_FLOW_RAW,
	CTL_IPP_ISP_FLOW_DIRECT_RAW,
	CTL_IPP_ISP_FLOW_CCIR,
	CTL_IPP_ISP_FLOW_IME_D2D,
	CTL_IPP_ISP_FLOW_IPE_D2D,
	CTL_IPP_ISP_FLOW_VR360,
	CTL_IPP_ISP_FLOW_DCE_D2D,
	CTL_IPP_ISP_FLOW_CAPTURE_RAW,
	CTL_IPP_ISP_FLOW_CAPTURE_CCIR,
	CTL_IPP_ISP_FLOW_MAX,
} CTL_IPP_ISP_FLOW_TYPE;

typedef struct {
	CTL_IPP_ISP_STATUS sts;
	CTL_IPP_ISP_FLOW_TYPE flow;
} CTL_IPP_ISP_STATUS_INFO;

/**
	IPE Eth structure
*/
typedef enum {
	CTL_IPP_ISP_ETH_OUT_ORIGINAL = 0,	/* output all pixel */
	CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE,		/* output w/2, h/2, pixel select by b_h_out_sel/b_v_out_sel */
	CTL_IPP_ISP_ETH_OUT_BOTH,			/* output both type(original + downsample) */
} CTL_IPP_ISP_ETH_OUT_SEL;

typedef struct {
	BOOL enable;		///< eth enable
	BOOL out_bit_sel;	///< 0 --> output 2 bit per pixel, 1 --> output 8 bit per pixel

	CTL_IPP_ISP_ETH_OUT_SEL out_sel;
	BOOL h_out_sel;		///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	BOOL v_out_sel;		///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	UINT32 buf_addr[2];	///< output buffer address, buf_addr[0] for CTL_IPP_ISP_ETH_OUT_ORIGINAL; buf_addr[1] for CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE
	UINT32 buf_size[2];	///< output buffer size
	UINT16 th_low;		///< edge threshold
	UINT16 th_mid;
	UINT16 th_high;
	USIZE out_size;		///< eth output size

	UINT32 frm_cnt;		///< corresponding raw frame cnt
	UINT64 timestamp;	///< corresponding raw timestamp
} CTL_IPP_ISP_ETH;

/**
	get parameters from kdriver, CTL_IPP_ISP_ITEM_KDRV_PARAM
*/
typedef enum {
	CTL_IPP_ISP_ENG_RHE = 0,
	CTL_IPP_ISP_ENG_IFE,
	CTL_IPP_ISP_ENG_DCE,
	CTL_IPP_ISP_ENG_IPE,
	CTL_IPP_ISP_ENG_IME,
	CTL_IPP_ISP_ENG_IFE2,
	CTL_IPP_ISP_ENG_MAX
} CTL_IPP_ISP_ENG;

typedef struct {
	CTL_IPP_ISP_ENG eng;	///< engine
	UINT32 param_id;		///< get param_id, ref to kdrv_xxx.h depend on engine
	void *data;				///< get result
} CTL_IPP_ISP_KDRV_PARAM;

/**
	3DNR STA Information
	Parameters needs to fill up when set_cmd:
	enable,
	buf_addr,
	max_sample_num,
*/
typedef struct {
	UINT32 enable;
	UINT32 buf_addr;
	UINT32 max_sample_num;
	UINT32 lofs;			///< get-only
	USIZE sample_step;		///< get-only
	USIZE sample_num;		///< get-only
	UPOINT sample_st;		///< get-only
} CTL_IPP_ISP_3DNR_STA;

/**
    VA Window size ratio
*/
typedef struct {
	UINT32 ratio_base;
	USIZE winsz_ratio;                                  ///< va window size of each window, max 511x511
	URECT indep_roi_ratio[CTL_IPP_ISP_VA_INDEP_NUM];       ///< va indep-window size, max 511x511
} CTL_IPP_ISP_VA_WIN_SIZE_RATIO;

/**
	VA Result
*/
typedef struct {
	UINT32 buf_g1_h[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g1_v[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g2_h[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g2_v[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g1_h_cnt[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g1_v_cnt[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g2_h_cnt[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
	UINT32 buf_g2_v_cnt[CTL_IPP_ISP_VA_MAX_WINNUM * CTL_IPP_ISP_VA_MAX_WINNUM];
} CTL_IPP_ISP_VA_RST;

/**
	VA Independent window result
*/
typedef struct {
	UINT32 g1_h[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g1_v[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g2_h[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g2_v[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g1_h_cnt[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g1_v_cnt[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g2_h_cnt[CTL_IPP_ISP_VA_INDEP_NUM];
	UINT32 g2_v_cnt[CTL_IPP_ISP_VA_INDEP_NUM];
} CTL_IPP_ISP_VA_INDEP_RST;

/**
	DCE histogram
*/
typedef struct {
    UINT16 hist_stcs_pre_wdr[CTL_IPP_ISP_DCE_HIST_NUM];
    UINT16 hist_stcs_post_wdr[CTL_IPP_ISP_DCE_HIST_NUM];
} CTL_IPP_ISP_DCE_HIST_RST;

/**
	IPE Edge Statistic
*/
typedef struct {
	UINT32 localmax_max;		///< staticstics, max value of the local max in a frame
	UINT32 coneng_max;			///< staticstics, max value of the edge energy in a frame
	UINT32 coneng_avg;			///< staticstics, average value of the edge energy in a frame
} CTL_IPP_ISP_EDGE_STCS;

/**
	Defog Statistc
*/
typedef struct {
	UINT16 airlight[CTL_IPP_ISP_DEFOG_AIRLIGHT_NUM];
} CTL_IPP_ISP_DEFOG_STCS;

/**
	Stripe result by kdrv_dce
*/
typedef struct {
	UINT32 num;
	UINT16 size[CTL_IPP_ISP_STRIPE_MAX_NUM];
} CTL_IPP_ISP_STRP_INFO;

/**
	3dnr md output
	md output must copy to another memory before used
*/
typedef struct {
	USIZE src_img_size;	/* src image size */
	USIZE md_size;		/* md output size */
	UINT32 md_lofs;		/* md output lineoffset */
	UINT32 addr;		/* md output address */
} CTL_IPP_ISP_MD_SUBOUT;

typedef enum {
	CTL_IPP_ISP_ITEM_FUNC_EN = 0,		///< [GET],		data_type: ISP_FUNC_EN
	CTL_IPP_ISP_ITEM_IOSIZE,			///< [GET], 	data_type: CTL_IPP_ISP_IOSIZE
	CTL_IPP_ISP_ITEM_RHE_IQ_PARAM,		///< [SET], 	data_type: KDRV_RHE_PARAM_IQ_ALL_PARAM, only pass pointer to kdrv
	CTL_IPP_ISP_ITEM_IFE_IQ_PARAM,		///< [SET], 	data_type: KDRV_IFE_PARAM_IQ_ALL_PARAM, only pass pointer to kdrv
	CTL_IPP_ISP_ITEM_DCE_IQ_PARAM,		///< [SET], 	data_type: KDRV_DCE_PARAM_IQ_ALL_PARAM, only pass pointer to kdrv
	CTL_IPP_ISP_ITEM_IPE_IQ_PARAM,		///< [SET], 	data_type: KDRV_IPE_PARAM_IQ_ALL_PARAM, only pass pointer to kdrv
	CTL_IPP_ISP_ITEM_IME_IQ_PARAM,		///< [SET], 	data_type: KDRV_IME_PARAM_IQ_ALL_PARAM, only pass pointer to kdrv
	CTL_IPP_ISP_ITEM_IFE2_IQ_PARAM,		///< [SET], 	data_type: KDRV_IFE2_PARAM_IQ_ALL_PARAM, only pass pointer to kdrv
	CTL_IPP_ISP_ITEM_YOUT,				///< [GET], 	data_type: CTL_IPP_ISP_YOUT
	CTL_IPP_ISP_ITEM_IFE_VIG_CENT,		///< [SET/GET], data_type: CTL_IPP_ISP_IFE_VIG_CENT_RATIO
	CTL_IPP_ISP_ITEM_DCE_DC_CENT,		///< [SET/GET], data_type: CTL_IPP_ISP_DCE_DC_CENT_RATIO
	CTL_IPP_ISP_ITEM_IME_LCA_SIZE,		///< [SET/GET], data_type: CTL_IPP_ISP_IME_LCA_SIZE_RATIO
	CTL_IPP_ISP_ITEM_IFE2_FILT_TIME,	///< [SET/GET], data_type: CTL_IPP_ISP_IFE2_FILTER_TIME
	CTL_IPP_ISP_ITEM_DEFOG_SUBOUT,		///< [GET], 	data_type: CTL_IPP_ISP_DEFOG_SUBOUT
	CTL_IPP_ISP_ITEM_YUV_OUT,			///< [SET/GET], data_type: CTL_IPP_ISP_YUV_OUT, note that set is used to unlock buffer
	CTL_IPP_ISP_ITEM_STATUS_INFO,		///< [GET], 	data_type: CTL_IPP_ISP_STATUS_INFO
	CTL_IPP_ISP_ITEM_ETH_PARAM,			///< [SET/GET], data_type: CTL_IPP_ISP_ETH, note that get will get ready eth rather than setting params
	CTL_IPP_ISP_ITEM_KDRV_PARAM,		///< [GET], 	data_type: CTL_IPP_ISP_KDRV_PARAM
	CTL_IPP_ISP_ITEM_3DNR_STA,			///< [GET],		data_type: CTL_IPP_ISP_3DNR_STA
	CTL_IPP_ISP_ITEM_VA_WIN_SIZE,		///< [SET/GET], data_type: CTL_IPP_ISP_VA_WIN_SIZE_RATIO
	CTL_IPP_ISP_ITEM_VA_RST,			///< [GET], 	data_type: CTL_IPP_ISP_VA_RST
	CTL_IPP_ISP_ITEM_VA_INDEP_RST,		///< [GET], 	data_type: CTL_IPP_ISP_VA_INDEP_RST
	CTL_IPP_ISP_ITEM_DCE_HIST_RST,		///< [GET], 	data_type: CTL_IPP_ISP_DCE_HIST_RST
	CTL_IPP_ISP_ITEM_EDGE_STCS,			///< [GET], 	data_type: CTL_IPP_ISP_EDGE_STCS
	CTL_IPP_ISP_ITEM_DEFOG_STCS,		///< [GET], 	data_type: CTL_IPP_ISP_DEFOG_STCS
	CTL_IPP_ISP_ITEM_STRIPE_INFO,		///< [GET], 	data_type: CTL_IPP_ISP_STRP_INFO
	CTL_IPP_ISP_ITEM_MD_SUBOUT,			///< [GET], 	data_type: CTL_IPP_ISP_MD_SUBOUT
	CTL_IPP_ISP_ITEM_MAX,
	CTL_IPP_ISP_ITEM_IMM_BIT = 0x80000000,	///< item called at imm_event must passed with this bit
	ENUM_DUMMY4WORD(CTL_IPP_ISP_ITEM)
} CTL_IPP_ISP_ITEM;

typedef enum {
	CTL_IPP_ISP_CB_MSG_NONE = 0x00000000,
	CTL_IPP_ISP_CB_MAX_MAX,
} CTL_IPP_ISP_CB_MSG;

ER ctl_ipp_isp_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, CTL_IPP_ISP_CB_MSG cb_msg);
ER ctl_ipp_isp_evt_fp_unreg(CHAR *name);
ER ctl_ipp_isp_set(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data);
ER ctl_ipp_isp_get(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data);

#endif

