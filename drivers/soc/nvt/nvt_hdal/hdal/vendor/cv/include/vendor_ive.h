/**
    Public header file for dal_ive

    This file is the header file that define the API and data type for vendor_ive.

    @file       vendor_ive.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.    All rights reserved.
*/
#ifndef _VENDOR_IVE_H_
#define _VENDOR_IVE_H_

#include "hd_type.h"

#define VENDOR_IVE_MSG_ERR -1
#define VENDOR_IVE_MSG_TIMEOUT -2

#define VENDOR_IVE_GEN_FILT_NUM	    10
#define VENDOR_IVE_EDGE_COEFF_NUM	9
#define VENDOR_IVE_EDGE_TH_LUT_TAP	15
#define VENDOR_IVE_MORPH_NEIGH_NUM	24
#define VENDOR_IVE_IOREMAP_IN_KERNEL 1

//----------------------------------------------------------------------
typedef struct _VENDOR_IVE_TRIGGER_PARAM {
    BOOL    is_nonblock;
	BOOL    wait_end;
	UINT32	time_out_ms;	///< force time out when wait_end == TRUE, set 0 for disable time out
} VENDOR_IVE_TRIGGER_PARAM;

//----------------------------------------------------------------------

/**
	IVE structure - ive kdrv open object
*/
typedef struct _VENDOR_IVE_OPENCFG {
	UINT32 ive_clock_sel;	//Engine clock selection
} VENDOR_IVE_OPENCFG;

//----------------------------------------------------------------------

typedef struct _VENDOR_IVE_IN_IMG_INFO {
	UINT32 width;		//image width
	UINT32 height;		//image height
	UINT32 lineofst;	//DRAM line offset of input channel
} VENDOR_IVE_IN_IMG_INFO;

//----------------------------------------------------------------------

/**
    IVE VENDOR Address info
*/
typedef struct _VENDOR_IVE_IMG_DMA_INFO {
	UINT32 addr;      //DMA address of Y channel
} VENDOR_IVE_IMG_IN_DMA_INFO, VENDOR_IVE_IMG_OUT_DMA_INFO;
//----------------------------------------------------------------------

/**
    IVE VENDOR general filter param and Enable/Disable
*/
typedef struct _VENDOR_IVE_GENERAL_FILTER_PARAM {
	BOOL enable;
	UINT32 filt_coeff[VENDOR_IVE_GEN_FILT_NUM];
	//UINT32 filt_norm;
} VENDOR_IVE_GENERAL_FILTER_PARAM;

//----------------------------------------------------------------------

typedef enum {
	VENDOR_IVE_MEDIAN = 0,    	//find median value in the filter
	VENDOR_IVE_MAX = 1,       	//find the maximum in the filter
	VENDOR_IVE_MIN = 2,       	//find the minimum in the filter
	VENDOR_IVE_MEDIAN_IVD_TH = 3, //find median value with invalid threshold in the filter //for_528_only
	//VENDOR_IVE_MEDIAN_W_INVAL = 3,
	ENUM_DUMMY4WORD(VENDOR_IVE_MEDN_MODE)
} VENDOR_IVE_MEDN_MODE;

/**
    IVE VENDOR median filter param and Enable/Disable
*/
typedef struct _VENDOR_IVE_MEDIAN_FILTER_PARAM {
	BOOL enable;
	VENDOR_IVE_MEDN_MODE mode;
	UINT32 medn_inval_th;		//for 528
} VENDOR_IVE_MEDIAN_FILTER_PARAM;

//----------------------------------------------------------------------

typedef enum {
	VENDOR_IVE_BI_DIR = 0,    //use 2 edge filters of X & Y direction to calculate X & Y edge response separately
	VENDOR_IVE_NO_DIR = 1,    //use 1 edge filter without direction to calculate edge response
	ENUM_DUMMY4WORD(VENDOR_IVE_EDGE_MODE)
} VENDOR_IVE_EDGE_MODE;

/**
    IVE VENDOR edge filter param and Enable/Disable
*/
typedef struct _VENDOR_IVE_EDGE_FILTER_PARAM {
	BOOL enable;
	VENDOR_IVE_EDGE_MODE mode;
	UINT32 edge_coeff1[VENDOR_IVE_EDGE_COEFF_NUM];
	UINT32 edge_coeff2[VENDOR_IVE_EDGE_COEFF_NUM];
	UINT32 edge_shift_bit;
	UINT32 AngSlpFact;
} VENDOR_IVE_EDGE_FILTER_PARAM;

//----------------------------------------------------------------------

/**
    IVE VENDOR non-maximal supression param and Enable/Disable
*/
typedef struct _VENDOR_IVE_NON_MAX_SUP_PARAM {
	BOOL enable;
	UINT32 mag_thres;
} VENDOR_IVE_NON_MAX_SUP_PARAM;

//----------------------------------------------------------------------

/**
    IVE VENDOR threshold LUT param and Enable/Disable
*/
typedef struct _VENDOR_IVE_THRES_LUT_PARAM {
	BOOL enable;
	UINT32 thres_lut[VENDOR_IVE_EDGE_TH_LUT_TAP];
} VENDOR_IVE_THRES_LUT_PARAM;

//----------------------------------------------------------------------

typedef enum {
	VENDOR_IVE_DILATE = 0,    //do dilation
	VENDOR_IVE_ERODE = 1,     //do erosion
	ENUM_DUMMY4WORD(VENDOR_IVE_MORPH_OP)
} VENDOR_IVE_MORPH_OP;

typedef enum {
	VENDOR_IVE_TH_LUT_IN = 0, //4 bit input from threshold LUT
	VENDOR_IVE_DRAM_4BIT = 1, //4 bit input from dram directly
	VENDOR_IVE_DRAM_1BIT = 2, //1 bit input from dram directly
	ENUM_DUMMY4WORD(VENDOR_IVE_MORPH_IN_SEL)
} VENDOR_IVE_MORPH_IN_SEL;

/**
    IVE VENDOR morphological filter param and Enable/Disable
*/
typedef struct _VENDOR_IVE_MORPH_FILTER_PARAM {
	BOOL enable;
    VENDOR_IVE_MORPH_IN_SEL in_sel;
	VENDOR_IVE_MORPH_OP operation;
	BOOL neighbor[VENDOR_IVE_MORPH_NEIGH_NUM];
} VENDOR_IVE_MORPH_FILTER_PARAM;

//----------------------------------------------------------------------

/**
    IVE VENDOR edge filter param and Enable/Disable
*/
typedef struct _VENDOR_IVE_INTEGRAL_IMG_PARAM {
	BOOL enable;
} VENDOR_IVE_INTEGRAL_IMG_PARAM;

//----------------------------------------------------------------------

/**
    IVE VENDOR output select and ofset
*/
typedef struct _VENDOR_IVE_OUTSEL_PARAM {
    UINT32 OutDataSel;
    UINT32 Outlofs;
}VENDOR_IVE_OUTSEL_PARAM;
//----------------------------------------------------------------------

/**
    IVE KDRV IRV parameter
*/
typedef struct _VENDOR_IVE_IRV_PARAM {
    BOOL enable;
    BOOL hist_mode_sel;
    BOOL invalid_val;
    UINT32 thr_s;
    UINT32 thr_h;
} VENDOR_IVE_IRV_PARAM;

/**
    IVE KDRV flow control parameter
*/
typedef struct _VENDOR_IVE_FLOWCT_PARAM {
    UINT32 dma_do_not_sync;
} VENDOR_IVE_FLOWCT_PARAM;

typedef enum {
    VENDOR_IVE_INPUT_INFO       = 0,    //input info
	VENDOR_IVE_INPUT_ADDR       = 1,    //input addr
	VENDOR_IVE_OUTPUT_ADDR      = 2,    //output addr
	VENDOR_IVE_GENERAL_FILTER   = 3,    //general filter
	VENDOR_IVE_MEDIAN_FILTER    = 4,    //median filter
	VENDOR_IVE_EDGE_FILTER      = 5,    //edge filter
	VENDOR_IVE_NON_MAX_SUP      = 6,    //non-maximum suppression
	VENDOR_IVE_THRES_LUT        = 7,    //edge filter
	VENDOR_IVE_MORPH_FILTER     = 8,    //morphology operation
	VENDOR_IVE_INTEGRAL_IMG     = 9,    //integral image
	VENDOR_IVE_OUTSEL           = 10,   //output select
	VENDOR_IVE_IRV              = 11,   //IRV
	VENDOR_IVE_DMA_ABORT		= 12,   //dma abort
	VENDOR_IVE_FLOWCT           = 13,   //flow control
	VENDOR_IVE_VERSION          = 14,   //get version info
	ENUM_DUMMY4WORD(VENDOR_IVE_FUNC)
} VENDOR_IVE_FUNC;


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern INT32 vendor_ive_init(void);
extern INT32 vendor_ive_uninit(void);
extern INT32 vendor_ive_set_param(VENDOR_IVE_FUNC param_id, void *p_param, UINT32 id);
extern INT32 vendor_ive_get_param(VENDOR_IVE_FUNC param_id, void *p_param, UINT32 id);
extern INT32 vendor_ive_trigger(VENDOR_IVE_TRIGGER_PARAM *p_param, UINT32 id);
extern VOID vendor_ive_get_version(void *p_param);
extern INT32 vendor_ive_trigger_nonblock(UINT32 id);
extern INT32 vendor_ive_waitdone_nonblock(UINT32 *timeout, UINT32 id);

#endif //_VENDOR_IVE_H_
