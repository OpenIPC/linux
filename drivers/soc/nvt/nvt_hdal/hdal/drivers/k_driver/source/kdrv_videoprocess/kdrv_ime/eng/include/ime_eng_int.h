/*
    IME module driver

    NT98520 IME internal header file.

    @file       ime_eng_int.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2020  All rights reserved.
*/

#ifndef _IME_ENG_INT_H_
#define _IME_ENG_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"

#include "ime_eng_int_reg.h"
#include "ime_eng_cal.h"

//#include "ime_eng_control_base.h"


#define IME_GET_MAX(a, b)   ((a >= b) ? a : b)
#define IME_GET_MIN(a, b)   ((a <= b) ? a : b)



#define IME_DBG_MSG  0

#define IME_DMA_CACHE_HANDLE    1


#define IME_STRIPE_UNIT      4
#define IME_H_OVERLAP        32
#define IME_V_OVERLAP        4


#define IME_SEL_INBUF0      0
#define IME_SEL_INBUF1      1

#define IME_FULL_LINE_BUF_SIZE  2688
#define IME_ISD_OUTPUT_BUF_SIZE 1344

/**
    IME enum - operation mode
*/
typedef enum _IME_OPMODE {
	IME_OPMODE_D2D     = 0,    ///< IME only
	IME_OPMODE_IFE_IPP = 1,    ///< IFE->DCE->IPE->IME
	IME_OPMODE_DCE_IPP = 2,    ///< IFE->DCE->IPE->IME
	IME_OPMODE_SIE_IPP = 3,    ///< SIE + IFE->DCE->IPE->IME (all direct)
	IME_OPMODE_UNKNOWN = 4,    ///< Undefined mode
	ENUM_DUMMY4WORD(IME_OPMODE)
} IME_OPMODE;







/**
    IME enum - input stripe mode selection
*/
typedef enum _IME_STRIPE_MODE {
	IME_STRIPE_AUTO_MODE = 0,    ///< Auto stripe information computation mode
	IME_STRIPE_USER_MODE = 1,    ///< User stripe information computation mode
	ENUM_DUMMY4WORD(IME_STRIPE_MODE)
} IME_STRIPE_MODE;

/**
    IME enum - input stripe size mode selection
*/
typedef enum _IME_STRIPE_SIZE_MODE_ {
	IME_STRIPE_SIZE_MODE_FIXED  = 0,    ///< Fixed size mode
	IME_STRIPE_SIZE_MODE_VARIED = 1,    ///< Varied size mode
	ENUM_DUMMY4WORD(IME_STRIPE_SIZE_MODE)
} IME_STRIPE_SIZE_MODE;



/**
    IME enum - input stripe overlap selection
*/
typedef enum _IME_H_STRIPE_OVLP_SEL {
	IME_H_ST_OVLP_16P  = 0,    ///< Horizontal stripe overlap size: 16 pixels
	IME_H_ST_OVLP_24P  = 1,    ///< Horizontal stripe overlap size: 24 pixels
	IME_H_ST_OVLP_32P  = 2,    ///< Horizontal stripe overlap size: 32 pixels
	IME_H_ST_OVLP_USER = 3,    ///< Horizontal stripe overlap size: user define
	ENUM_DUMMY4WORD(IME_H_STRIPE_OVLP_SEL)
} IME_H_STRIPE_OVLP_SEL;

/**
    IME enum - input stripe overlap partition selection
*/
typedef enum _IME_H_STRIPE_PRT_SEL {
	IME_H_ST_PRT_5P  = 0,    ///< Horizontal stripe overlap partition size: 5 pixels
	IME_H_ST_PRT_3P  = 1,    ///< Horizontal stripe overlap partition size: 3 pixels
	IME_H_ST_PRT_2P  = 2,    ///< Horizontal stripe overlap partition size: 2 pixels
	IME_H_ST_PRT_USER = 3,    ///< Horizontal stripe overlap partition size: user define
	ENUM_DUMMY4WORD(IME_H_STRIPE_PRT_SEL)
} IME_H_STRIPE_PRT_SEL;








typedef enum _IME_INSRC_SEL {
	IME_INSRC_IPE   = 0,         ///< input from IPE
	IME_INSRC_DRAM  = 1,         ///< input from DRAM
	ENUM_DUMMY4WORD(IME_INSRC_SEL)
} IME_INSRC_SEL;



typedef enum _IME_SRC_CTRL_SEL {
	SRC_CTRL_IME = 0,         ///< input port control by IME input source
	SRC_CTRL_DCE = 1,         ///< input port control by DCE
	ENUM_DUMMY4WORD(IME_SRC_CTRL_SEL)
} IME_SRC_CTRL_SEL;


/*
    @name IME enum - Engine DMA direction selecion
*/
typedef enum _IME_DMA_DIRECTIOIN {
	IME_DMA_INPUT = 0,   ///< input DMA direction
	IME_DMA_OUTPUT = 1,  ///< output DMA direction
	ENUM_DUMMY4WORD(IME_DMA_DIRECTIOIN)
} IME_DMA_DIRECTIOIN;


/*
    @name IME enum - Engine lock status selection
*/
typedef enum _IME_USED_STATUS {
	IME_ENGINE_UNLOCKED = 0,  ///< input DMA direction
	IME_ENGINE_LOCKED = 1,    ///< output DMA direction
	ENUM_DUMMY4WORD(IME_USED_STATUS)
} IME_USED_STATUS;


/*
    @name IME enum - Status of State Machine
*/
typedef enum _IME_STATE_MACHINE {
	IME_ENGINE_IDLE  = 0,        ///< Idel status
	IME_ENGINE_READY = 1,        ///< Ready status
	IME_ENGINE_PAUSE = 2,        ///< Pause status
	IME_ENGINE_RUN   = 3,        ///< Run status
	//IME_ENGINE_READY2RUN = 4,    ///< ready to Run status
	ENUM_DUMMY4WORD(IME_STATE_MACHINE)
} IME_STATE_MACHINE;


/**
    @name IME enum - Operations of engine action
*/
typedef enum _IME_ACTION_OP {
	IME_ACTOP_OPEN          = 0,    ///< Open engine
	IME_ACTOP_CLOSE         = 1,    ///< Close engine
	IME_ACTOP_START         = 2,    ///< Start engine
	IME_ACTOP_PARAM         = 3,    ///< Set parameter
	IME_ACTOP_PAUSE         = 4,    ///< Pause engine
	//IME_ACTOP_SWRESET       = 5,    ///< Software reset
	IME_ACTOP_CHGCLOCK      = 6,    ///< Change clock rate
	IME_ACTOP_DYNAMICPARAM  = 7,    ///< Change dynamic parameter
	//IME_ACTOP_TRIGGERSLICE  = 8,    ///< Slice trigger
	ENUM_DUMMY4WORD(IME_ACTION_OP)
} IME_ACTION_OP;


/**
    @name IME enum - Engine Stripe mode selection
*/
typedef enum _IME_STRIPE_MODE_SEL {
	IME_SINGLE_STRIPE = 0,  ///< Single stripe
	IME_MULTI_STRIPE = 1,   ///< Multi stripe
	ENUM_DUMMY4WORD(IME_STRIPE_MODE_SEL)
} IME_STRIPE_MODE_SEL;



typedef enum _IME_ENGEIN_SET_CTRL {
	IME_ENGINE_SET_RESET   = 0,  // set reset
	IME_ENGINE_SET_START   = 2,  // set start
	IME_ENGINE_SET_LOAD    = 3,  // set load
	IME_ENGINE_SET_INSRC   = 4,  // set input source
	IME_ENGINE_SET_DRCL    = 5,  // set input port control when direct mode
	IME_ENGINE_SET_INTPS   = 6,  // set interrupt status
	IME_ENGINE_SET_INTPE   = 7,  // set interrupt enable
	ENUM_DUMMY4WORD(IME_ENGEIN_SET_CTRL)
} IME_ENGEIN_SET_CTRL;


typedef enum _IME_ENGEIN_GET_CTRL {
	IME_ENGINE_GET_INTP         = 0,  // get interrupt status
	IME_ENGINE_GET_INTPE        = 1,  // get interrupt enable
	//IME_ENGINE_GET_LOCKSTATUS   = 2,  // get locked status
	ENUM_DUMMY4WORD(IME_ENGEIN_GET_CTRL)
} IME_ENGEIN_GET_CTRL;





/**
    IME enum - privacy mask type selection
*/
typedef enum _IME_PM_OPR {
	IME_PM_OPR_LARGER_EQUAL = 0,  ///< larger than and equal to
	IME_PM_OPR_LESS_EQUAL   = 1,  ///< less than and equal to
	IME_PM_OPR_LARGER       = 2,  ///< larger than and equal to
	IME_PM_OPR_LESS         = 3,  ///< less than and equal to
	ENUM_DUMMY4WORD(IME_PM_OPR)
} IME_PM_OPR;









typedef struct _IME_ISD_FACTOR_ {
	UINT32 uiFactor;
	UINT32 uiCnt;
} IME_ISD_FACTOR;



/**
    IME structure - image size parameters for horizontal and vertical direction
*/
typedef struct _IME_SIZE_INFO {
	UINT32 size_h;                  ///< horizontal size
	UINT32 size_v;                  ///< vertical size
} IME_SIZE_INFO;


/**
    IME structure - input stripe parameters
*/
typedef struct _IME_STRIPE_INFO {
	UINT32  stp_n;        ///< stripe size in each stripe, used for fixed size mode
	UINT32  stp_l;        ///< stripe size of last stripe, used for fixed size mode
	UINT32  stp_m;        ///< stripe number

	UINT32 varied_size[8]; ///< stripe size, 8 entries, used for varied size mode
} IME_STRIPE_INFO;




/**
    IME structure - horizontal and vertical stripe parameters
*/
typedef struct _IME_HV_STRIPE_INFO {
	IME_STRIPE_SIZE_MODE    stp_size_mode;        ///< stripe size mode selection, if StpSizeMode = IME_STRIPE_SIZE_MODE_VARIED, StripeCalMode = IME_STRIPE_USER_MODE only
	IME_STRIPE_INFO         stp_h;           ///< hroizontal stripe parameters
	IME_STRIPE_INFO         stp_v;           ///< Vertical stripe parameters

	IME_H_STRIPE_OVLP_SEL   overlap_h_sel;    ///< IME input stripe overlap size selection for horizontal direction
	UINT32                  overlap_h_size;                    ///< IME input stripe overlap size for horizontal direction

	IME_H_STRIPE_PRT_SEL    prt_h_sel;    ///< IME input stripe overlap parition size selection for horizontal direction
	UINT32                  prt_h_size;                    ///< IME input stripe overlap parition size for horizontal direction

	IME_STRIPE_MODE         stripe_cal_mode;  ///< IME input stripe information setting mode, recommended: IME_STRIPE_AUTO_MODE
} IME_HV_STRIPE_INFO;


extern UINT32 ime_max_stp_isd_out_buf_size;
extern volatile NT98560_IME_REGISTER_STRUCT *imeg;
extern volatile UINT8 *p_ime_reg_chg_flag;
extern IME_HV_STRIPE_INFO ime_stripe_info;

//extern IME_OPMODE               ime_engine_operation_mode;
//extern IME_SCALE_FILTER_INFO    ime_scale_filter_params;

//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path1_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path2_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path3_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path4_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path5_params;

//extern IME_DMA_ADDR_INFO        ime_input_buffer_addr, ime_output_buffer_addr;
//extern IME_DMA_ADDR_INFO        ime_tmp_addr0, ime_tmp_addr1;
//extern IME_LINEOFFSET_INFO      ime_lofs_params;

//extern IME_SIZE_INFO ime_input_size, ime_output_size;
extern IME_SIZE_INFO ime_output_size_path1, ime_output_size_path2, ime_output_size_path3, ime_output_size_path4, ime_output_size_path5;
extern UINT32 ime_tmp_input_size, ime_tmp_output_size_path1, ime_tmp_output_size_path2, ime_tmp_output_size_path3, ime_tmp_output_size_path4, ime_tmp_output_size_path5, ime_tmp_output_size_max;

//extern IME_SSR_MODE_SEL g_SsrMode;



//================================================================================

extern IME_H_STRIPE_OVLP_SEL ime_cal_overlap_size(IME_SIZE_INFO *p_in_size, IME_SIZE_INFO *p_out_size);
extern VOID ime_cal_hv_strip_param(UINT32 in_width, UINT32 in_height, UINT32 stripe_size, IME_HV_STRIPE_INFO *p_strip_param);

//extern VOID ime_cal_convex_hull_coefs(IME_PM_POINT *p_pm_cvx_point, INT32 pnt_num, INT32 *p_coefs_line1, INT32 *p_coefs_line2, INT32 *p_coefs_line3, INT32 *p_coefs_line4);

//extern UINT32 ime_cal_scale_filter_coefs(UINT32 in_img_size, UINT32 out_img_size);

//------------------------------------------------------------------------
// check and translate API
extern ER ime_check_lineoffset(UINT32 lofs_y, UINT32 lofs_c);
extern ER ime_check_input_path_stripe(VOID);

//------------------------------------------------------------------------
// input APIs
//extern VOID ime_set_in_path_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u, IME_BURST_SEL bst_v);

//------------------------------------------------------------------------
// output Path1 APIs
//extern VOID ime_set_out_path_p1_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u, IME_BURST_SEL bst_v);


//------------------------------------------------------------------------
// output Path2 APIs
//extern VOID ime_set_out_path_p2_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u);




//------------------------------------------------------------------------
// output Path3 APIs
//extern VOID ime_set_out_path_p3_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u);


//------------------------------------------------------------------------
// output Path4 APIs
//extern VOID ime_set_out_path_p4_burst_size(IME_BURST_SEL bst_y);




//==========================================================================
// IQ function APIs

//------------------------------------------------------------------------
// Image Local Adaptation


//extern VOID ime_set_in_path_lca_burst_size(IME_BURST_SEL bst);
//extern VOID ime_set_out_path_lca_burst_size(IME_BURST_SEL bst);



//------------------------------------------------------------------------
// LCA subout


//------------------------------------------------------------------------
// data stamp
//extern VOID ime_set_osd_set0_burst_size(IME_BURST_SEL bst);


//------------------------------------------------------------------------
// privacy mask
//extern VOID ime_set_privacy_mask_enable(IME_PM_SET_SEL set_sel, IME_FUNC_EN set_en);
//extern VOID ime_set_privacy_mask_type(IME_PM_SET_SEL set_sel, IME_PM_MASK_TYPE set_type);
//extern VOID ime_set_privacy_mask_color(IME_PM_SET_SEL set_sel, IME_PM_MASK_COLOR set_color);
//extern VOID ime_set_privacy_mask_weight(IME_PM_SET_SEL set_sel, UINT32 set_wet);
//extern VOID ime_set_privacy_mask_line0(IME_PM_SET_SEL set_sel, INT32 *p_coefs);
//extern VOID ime_set_privacy_mask_line1(IME_PM_SET_SEL set_sel, INT32 *p_coefs);
//extern VOID ime_set_privacy_mask_line2(IME_PM_SET_SEL set_sel, INT32 *p_coefs);
//extern VOID ime_set_privacy_mask_line3(IME_PM_SET_SEL set_sel, INT32 *p_coefs);


//extern VOID ime_set_privacy_mask_burst_size(IME_BURST_SEL bst);

//------------------------------------------------------------------------
// TMNR

//extern VOID ime_set_in_path_3dnr_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_c, IME_BURST_SEL bst_mv, IME_BURST_SEL bst_ms);
//extern VOID ime_set_out_path_3dnr_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL BstC, IME_BURST_SEL BstMV, IME_BURST_SEL BstMO, IME_BURST_SEL BstMOROI, IME_BURST_SEL BstSta);


#ifdef __cplusplus
}
#endif


#endif


