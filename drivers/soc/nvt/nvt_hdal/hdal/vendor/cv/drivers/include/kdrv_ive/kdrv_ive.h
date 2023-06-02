/**
	Public header file for dal_ive

	This file is the header file that define the API and data type for kdrv_ive.

	@file	kdrv_ive.h
	@ingroup	mILibIPLCom
	@note	Nothing (or anything need to be mentioned).

	Copyright	Novatek Microelectronics Corp. 2018.	All rights reserved.
*/
#ifndef _KDRV_IVE_H_
#define _KDRV_IVE_H_

#include "kdrv_type.h"

#define IVE_MSG_ERR -1
#define IVE_MSG_TIMEOUT -2

#define IVE_GEN_FILT_NUM	10
#define IVE_EDGE_COEFF_NUM	9
#define IVE_EDGE_TH_LUT_TAP	15
#define IVE_MORPH_NEIGH_NUM	24
#define IVE_IOREMAP_IN_KERNEL 1

typedef void (* ive_hook_t)(UINT32 hook_mode, UINT32 hook_value);

//structure data type
typedef struct _KDRV_IVE_TRIGGER_PARAM {
    BOOL    is_nonblock;
	BOOL    wait_end;
	UINT32	time_out_ms;	///< force time out when wait_end == TRUE, set 0 for disable time out
} KDRV_IVE_TRIGGER_PARAM;

//----------------------------------------------------------------------

/**
	IVE structure - ive kdrv open object
*/
typedef struct _KDRV_IVE_OPENCFG {
	UINT32 ive_clock_sel;	//Engine clock selection
} KDRV_IVE_OPENCFG, *pKDRV_IVE_OPENCFG;

//----------------------------------------------------------------------

typedef struct _KDRV_IVE_IN_IMG_INFO {
	UINT32 width;		//image width
	UINT32 height;		//image height
	UINT32 lineofst;	//DRAM line offset of input channel
} KDRV_IVE_IN_IMG_INFO;

//----------------------------------------------------------------------

/**
	IVE KDRV Address info
*/
typedef struct _KDRV_IVE_IMG_DMA_INFO {
	UINT32 addr;	//DMA address of Y channel
} KDRV_IVE_IMG_IN_DMA_INFO, KDRV_IVE_IMG_OUT_DMA_INFO;

typedef struct _KDRV_IVE_LL_DMA_INFO {
	UINT32 addr;	//DMA address of LinkedList
} KDRV_IVE_LL_DMA_INFO;
//----------------------------------------------------------------------

/**
	IVE KDRV general filter param and Enable/Disable
*/
typedef struct _KDRV_IVE_GENERAL_FILTER_PARAM {
	BOOL enable;
	UINT32 filt_coeff[IVE_GEN_FILT_NUM];

} KDRV_IVE_GENERAL_FILTER_PARAM;

//----------------------------------------------------------------------

typedef enum {
	KDRV_IVE_MEDIAN = 0,		//find median value in the filter
	KDRV_IVE_MAX = 1,			//find the maximum in the filter
	KDRV_IVE_MIN = 2,			//find the minimum in the filter
	KDEV_IVE_MEDIAN_IVD_TH = 3, //find median value with invalid threshold in the filter //for_528_only
	ENUM_DUMMY4WORD(KDRV_IVE_MEDN_MODE)
} KDRV_IVE_MEDN_MODE;

/**
	IVE KDRV median filter param and Enable/Disable
*/
typedef struct _KDRV_IVE_MEDIAN_FILTER_PARAM {
	BOOL enable;
	KDRV_IVE_MEDN_MODE mode;
	UINT32 medn_inval_th;       //for 528
} KDRV_IVE_MEDIAN_FILTER_PARAM;

//----------------------------------------------------------------------

typedef enum {
	KDRV_IVE_BI_DIR = 0,	//use 2 edge filters of X & Y direction to calculate X & Y edge response separately
	KDRV_IVE_NO_DIR = 1,	//use 1 edge filter without direction to calculate edge response
	ENUM_DUMMY4WORD(KDRV_IVE_EDGE_MODE)
} KDRV_IVE_EDGE_MODE;

/**
	IVE KDRV edge filter param and Enable/Disable
*/
typedef struct _KDRV_IVE_EDGE_FILTER_PARAM {
	BOOL enable;
	KDRV_IVE_EDGE_MODE mode;
	UINT32 edge_coeff1[IVE_EDGE_COEFF_NUM];
	UINT32 edge_coeff2[IVE_EDGE_COEFF_NUM];
	UINT32 edge_shift_bit;
	UINT32 AngSlpFact;
} KDRV_IVE_EDGE_FILTER_PARAM;

//----------------------------------------------------------------------

/**
	IVE KDRV non-maximal supression param and Enable/Disable
*/
typedef struct _KDRV_IVE_NON_MAX_SUP_PARAM {
	BOOL enable;
	UINT32 mag_thres;
} KDRV_IVE_NON_MAX_SUP_PARAM;

//----------------------------------------------------------------------

/**
	IVE KDRV threshold LUT param and Enable/Disable
*/
typedef struct _KDRV_IVE_THRES_LUT_PARAM {
	BOOL enable;
	UINT32 thres_lut[IVE_EDGE_TH_LUT_TAP];
} KDRV_IVE_THRES_LUT_PARAM;

//----------------------------------------------------------------------

typedef enum {
	KDRV_IVE_DILATE = 0,	//do dilation
	KDRV_IVE_ERODE = 1,	//do erosion
	ENUM_DUMMY4WORD(KDRV_IVE_MORPH_OP)
} KDRV_IVE_MORPH_OP;

typedef enum {
	KDRV_IVE_TH_LUT_IN = 0, //4 bit input from threshold LUT
	KDRV_IVE_DRAM_4BIT = 1, //4 bit input from dram directly
	KDRV_IVE_DRAM_1BIT = 2, //1 bit input from dram directly
	ENUM_DUMMY4WORD(KDRV_IVE_MORPH_IN_SEL)
} KDRV_IVE_MORPH_IN_SEL;

/**
	IVE KDRV morphological filter param and Enable/Disable
*/
typedef struct _KDRV_IVE_MORPH_FILTER_PARAM {
	BOOL enable;
	KDRV_IVE_MORPH_IN_SEL in_sel;
	KDRV_IVE_MORPH_OP operation;
	BOOL neighbor[IVE_MORPH_NEIGH_NUM];
} KDRV_IVE_MORPH_FILTER_PARAM;

//----------------------------------------------------------------------
typedef enum {
	KDRV_IVE_Y = 0,	//Y
	KDRV_IVE_UV_PACKED = 1,	//UV packed
	KDRV_IVE_HSV_PACKED = 2,//HSV/RGB packed
	ENUM_DUMMY4WORD(KDRV_IVE_INTEGRAL_FMT_SEL)
} KDRV_IVE_INTEGRAL_FMT_SEL;

/**
	IVE KDRV edge filter param and Enable/Disable
*/
typedef struct _KDRV_IVE_INTEGRAL_IMG_PARAM {
	BOOL enable;
	KDRV_IVE_INTEGRAL_FMT_SEL integral_fmt;
} KDRV_IVE_INTEGRAL_IMG_PARAM;

//----------------------------------------------------------------------

/**
	IVE KDRV iterative region voting param and Enable/Disable
*/

//----------------------------------------------------------------------

/**
	IVE KDRV PARAM ID, for KDRV_IVE_OUTSEL_PARAM structure
*/

typedef struct _KDRV_IVE_OUTSEL_PARAM {
    UINT32 OutDataSel;
    UINT32 Outlofs;

} KDRV_IVE_OUTSEL_PARAM;
//----------------------------------------------------------------------

/**
	IVE KDRV IRV parameter
*/
typedef struct _KDRV_IVE_IRV_PARAM {
	BOOL en;
	BOOL hist_mode_sel;
	BOOL invalid_val;
	UINT32 thr_s;
	UINT32 thr_h;
} KDRV_IVE_IRV_PARAM;


/**
	IVE KDRV DMA ABORT parameter
*/
typedef struct _KDRV_IVE_DMA_ABORT_PARAM {
	BOOL dma_disable;
} KDRV_IVE_DMA_ABORT_PARAM;

/**
    IVE KDRV trigger hook parameter
*/
typedef struct _KDRV_IVE_TRIGGER_HOOK_PARAM {
	UINT32 hook_mode;
	UINT32 hook_value;
	UINT32 hook_is_direct_return;
} KDRV_IVE_TRIGGER_HOOK_PARAM;

/**
	IVE KDRV FLOW_CT parameter
*/
typedef struct _KDRV_IVE_FLOW_CT_PARAM {
	UINT32 dma_do_not_sync;
} KDRV_IVE_FLOW_CT_PARAM;

/**
	IVE KDRV PARAM ID, for kdrv_ive_set/kdrv_ive_get
*/
typedef enum _KDRV_IVE_PARAM_ID {
	KDRV_IVE_PARAM_IPL_OPENCFG,			// [Set]	, use KDRV_IVE_OPENCFG, set clock info
	KDRV_IVE_PARAM_IPL_IN_IMG,			// [Set/Get], use KDRV_IVE_IN_IMG_INFO structure, set input image size info, line offset
	KDRV_IVE_PARAM_IPL_IMG_DMA_IN,		// [Set/Get], use KDRV_IVE_IMG_IN_DMA_INFO structure, set the dram input address
	KDRV_IVE_PARAM_IPL_IMG_DMA_OUT,		// [Set/Get], use KDRV_IVE_IMG_OUT_DMA_INFO structure, set the dram output address
	KDRV_IVE_PARAM_IPL_ISR_CB,			// [Set/Get], use KDRV_IPP_ISRCB structure, set ive external isr cb
	KDRV_IVE_PARAM_IQ_GENERAL_FILTER,	// [Set/Get], use KDRV_IVE_GENERAL_FILTER_PARAM structure, set general filter parameters
	KDRV_IVE_PARAM_IQ_MEDIAN_FILTER,	// [Set/Get], use KDRV_IVE_MEDIAN_FILTER_PARAM structure, set median filter parameters
	KDRV_IVE_PARAM_IQ_EDGE_FILTER,		// [Set/Get], use KDRV_IVE_EDGE_FILTER_PARAM structure, set edge filter parameters
	KDRV_IVE_PARAM_IQ_NON_MAX_SUP,		// [Set/Get], use KDRV_IVE_NON_MAX_SUP_PARAM structure, set non-maximal supression parameters
	KDRV_IVE_PARAM_IQ_THRES_LUT,		// [Set/Get], use KDRV_IVE_THRES_LUT_PARAM structure, set edge threshold LUT parameters
	KDRV_IVE_PARAM_IQ_MORPH_FILTER,		// [Set/Get], use KDRV_IVE_MORPH_FILTER_PARAM structure, set morphological filter parameters
	KDRV_IVE_PARAM_IQ_INTEGRAL_IMG,		// [Set/Get], use KDRV_IVE_INTEGRAL_IMG_PARAM structure, set integral image parameters
	KDRV_IVE_PARAM_IQ_OUTSEL,           // [Set/Get], use KDRV_IVE_OUTSEL_PARAM structure, set output selection parameters
	KDRV_IVE_PARAM_IPL_LL_DMA_IN,       // [Set/Get], use KDRV_IVE_LL_DMA_INFO structure, set the dram input LL address
	KDRV_IVE_PARAM_IRV,		            // [Set/Get], use KDRV_IVE_IRV_PARAM structure, set irv
	KDRV_IVE_PARAM_DMA_ABORT,		    // [Set]	, set dma_abort
	KDRV_IVE_PARAM_FLOWCT,		        // [Set/Get], use KDRV_IVE_FLOW_CT structure, set dma cache flush or not
	KDRV_IVE_PARAM_MAX,
	KDRV_IVE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_IVE_PARAM_ID)
} KDRV_IVE_PARAM_ID;
STATIC_ASSERT((KDRV_IVE_PARAM_MAX &KDRV_IVE_PARAM_REV) == 0);




#define KDRV_IVE_IGN_CHK KDRV_IVE_PARAM_REV	//only support set/get function

#if defined __UITRON || defined __ECOS
extern void kdrv_ive_install_id(void) _SECTION(".kercfg_text");
#elif defined(__FREERTOS)
extern void kdrv_ive_install_id(void);
#else
extern void kdrv_ive_install_id(void);
#endif

/*!
 * @fn INT32 kdrv_ive_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip	the chip id of hardware
 * @param engine	the engine id of hardware
 *	- @b KDRV_GFX2D_IVE0: IVE engine 0
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ive_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ive_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip	the chip id of hardware
 * @param engine	the engine id of hardware
 *	- @b KDRV_GFX2D_IVE0: IVE engine 0
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ive_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ive_set(UINT32 id, KDRV_IVE_PARAM_ID param_id, void* p_param)
 * @brief set parameters to hardware engine
 * @param id	the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ive_set(UINT32 id, KDRV_IVE_PARAM_ID param_id, void *p_param);

/*!
 * @fn INT32 kdrv_ive_get(UINT32 id, KDRV_IVE_PARAM_ID param_id, void* p_param)
 * @brief get parameters from hardware engine
 * @param id	the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ive_get(UINT32 id, KDRV_IVE_PARAM_ID param_id, void *p_param);

/*!
 * @fn INT32 kdrv_ive_trigger(UINT32 id,	KDRV_IVE_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
 * @brief trigger hardware engine
 * @param id	the id of hardware
 * @param p_param	the parameter for trigger
 * @param p_cb_func	the callback function
 * @param p_user_data	the private user data
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ive_trigger(UINT32 id, KDRV_IVE_TRIGGER_PARAM *p_ive_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
extern INT32 kdrv_ive_trigger_nonblock(UINT32 id);

/*!
 * @fn INT32 kdrv_ive_trigger(UINT32 id,	KDRV_IVE_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
 * @brief trigger hardware engine
 * @param id	the id of hardware
 * @param p_param	the parameter for trigger
 * @param p_cb_func	the callback function
 * @param p_user_data	the private user data
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ive_trigger_with_hook(UINT32 id, KDRV_IVE_TRIGGER_PARAM *p_ive_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data,
                        ive_hook_t ive_hook, KDRV_IVE_TRIGGER_HOOK_PARAM tri_hook);

/*!
 * @fn INT32 kdrv_ive_waitdone_nonblock(UINT32 id, KDRV_IVE_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data);
 * @brief wait hardware engine done
 * @param id                the id of hardware
 * @param p_param           the parameter for trigger
 * @param p_user_data       the private user data
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ive_waitdone_nonblock(UINT32 id, UINT32 *timeout);


#endif //_KDRV_IVE_H_
