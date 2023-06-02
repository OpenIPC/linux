/**
    Public header file for KDRV_SDE

    This file is the header file that define the API and data type for KDRV_SDE.

    @file       kdrv_sde.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KDRV_SDE_H_
#define _KDRV_SDE_H_


#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kdrv_type.h"

#define KDRV_SDE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_SDE_FMD         FLGPTN_BIT(8)
#define KDRV_SDE_TIMEOUT     FLGPTN_BIT(9)
#define KDRV_SDE_HDL_UNLOCK	FLGPTN_BIT(10)
#define KDRV_SDE_LOCK_BITS	0x000000ff
#define KDRV_SDE_INIT_BITS	(KDRV_SDE_LOCK_BITS | KDRV_SDE_HDL_UNLOCK)
/*
SEM_HANDLE* kdrv_sde_get_sem_id(KDRV_SDE_SEM idx);

typedef enum {
	SEMID_KDRV_SDE,
	KDRV_SDE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_SDE_SEM)
} KDRV_SDE_SEM;

typedef struct _KDRV_SDE_SEM_INFO_ {
	SEM_HANDLE  semphore_id;				///< Semaphore ID
	UINT32      max_counter;
} KDRV_SDE_SEM_TABLE;
*/
/**
       type for KDRV_SDE_ISRCB
*/
typedef enum {
	KDRV_SDE_INT_FMD = 0x00000001,
	KDRV_SDE_INT_2FMD = 0x00000002,
	ENUM_DUMMY4WORD(KDRV_SDE_ISR_STATUS)
} KDRV_SDE_ISR_STATUS;

/**
    SDE Stereo Depth Disparity Operation Mode.

    Select SDE Operation Mode.
*/
typedef enum _KDRV_SDE_OP_MODE {
	KDRV_SDE_MAX_DISP_31    = 0,    ///< max disparity 31
	KDRV_SDE_MAX_DISP_63    = 1,    ///< max disparity 63
	ENUM_DUMMY4WORD(KDRV_SDE_OP_MODE)
} KDRV_SDE_OP_MODE;

/**
    SDE Stereo Depth Invalid Value Selection.

    Select SDE invalid value to fill in.
*/
typedef enum _KDRV_SDE_INV_SEL {
	KDRV_SDE_INV_0               = 0,       ///< fill in 0 to invalid point
	KDRV_SDE_INV_63              = 1,       ///< fill in 63 to invalid point
	KDRV_SDE_INV_255             = 2,       ///< fill in 255 to invalid point
	ENUM_DUMMY4WORD(KDRV_SDE_INV_SEL)
} KDRV_SDE_INV_SEL;

/**
    KDRV SDE structure - trigger parameter
*/
typedef struct _KDRV_SDE_TRIGGER_PARAM {
	BOOL    reserved;                               ///< reserved
} KDRV_SDE_TRIGGER_PARAM;

/**
    KDRV SDE structure - open configuration
*/
typedef struct _KDRV_SDE_OPENCFG {
	UINT32 sde_clock; 								///< support clock : 220/192/160/120/80/60/48 Mhz
} KDRV_SDE_OPENCFG;

/**
    KDRV SDE structure - sde operation mode
*/
typedef struct _KDRV_SDE_MODE_INFO {
	KDRV_SDE_OP_MODE op_mode; 						///< max disparity selection
	KDRV_SDE_INV_SEL inv_sel;                 		///< invalid value selection
} KDRV_SDE_MODE_INFO;

/**
     KDRV SDE structure - SDE IO Parameters.
*/
typedef struct _KDRV_SDE_IO_INFO {
	UINT32 width;               					///< image width
	UINT32 height;              					///< image height
	UINT32 in_addr0;       							///< image input address of input 0 (left)
	UINT32 in_addr1;       							///< image input address of input 1 (right)
	UINT32 vol_addr;       							///< image input address of cost volume
	UINT32 out_addr;       							///< image output address
	UINT32 in0_lofs;                				///< image input lineoffset of input 0
	UINT32 in1_lofs;                				///< image input lineoffset of input 1
	UINT32 vol_lofs;                				///< image input lineoffset of cost volume
	UINT32 out_lofs;                				///< image output lineoffset
} KDRV_SDE_IO_INFO;

/**
    KDRV SDE structure - SDE Cost computation Parameters.
*/
typedef struct _KDRV_SDE_COST_PARAM {
	UINT32 ad_bound_upper;         ///< upper bound value of absolute difference (0~63)
	UINT32 ad_bound_lower;         ///< lower bound value of absolute difference (0~63)
	UINT32 census_bound_upper;     ///< upper bound value of census bit code (0~25)
	UINT32 census_bound_lower;     ///< lower bound value of census bit code (0~25)
	UINT32 ad_census_ratio;        ///< weighted ratio with absolute difference on luminance (0~255)
} KDRV_SDE_COST_PARAM;

/**
    KDRV SDE structure - SDE Luminance related parameters used in cost and scan.

    SDE Luminance related parameters
*/
typedef struct _KDRV_SDE_LUMA_PARAM {
	UINT32 luma_saturated_th;  		///< threshold to determine saturated pixel
	UINT32 cost_saturated;   		///< cost value assigned to saturated right image point encountered
	UINT32 delta_luma_smooth_th;	///< threshold to let data term cost dominate
} KDRV_SDE_LUMA_PARAM;


/**
    KDRV SDE structure - SDE Scan Penalty parameters.
*/
typedef struct _KDRV_SDE_PENALTY_PARAM {
	UINT32 penalty_nonsmooth; 		///< smallest penalty for big luminance difference
	UINT32 penalty_smooth0;   		///< small penalty for small disparity change
	UINT32 penalty_smooth1;   		///< small penalty for middle disparity change
	UINT32 penalty_smooth2;   		///< small penalty for big disparity change
} KDRV_SDE_PENALTY_PARAM;


/**
    KDRV SDE structure - SDE Scan smoothness parameters.
*/
typedef struct _KDRV_SDE_SMOOTH_PARAM {
	UINT32 delta_displut0;        ///< Threshold to remove smooth penalty
	UINT32 delta_displut1;        ///< Threshold to give small penalty
} KDRV_SDE_SMOOTH_PARAM;

/**
    KDRV SDE structure - SDE LRC threshold parameters.
*/
typedef struct _KDRV_SDE_LRC_THRESH_PARAM {
 	UINT32 thresh_lut0;       ///< diagonal search threshold look-up table 0
 	UINT32 thresh_lut1;       ///< diagonal search threshold look-up table 1
 	UINT32 thresh_lut2;       ///< diagonal search threshold look-up table 2
 	UINT32 thresh_lut3;       ///< diagonal search threshold look-up table 3
 	UINT32 thresh_lut4;       ///< diagonal search threshold look-up table 4
 	UINT32 thresh_lut5;       ///< diagonal search threshold look-up table 5
 	UINT32 thresh_lut6;       ///< diagonal search threshold look-up table 6
} KDRV_SDE_LRC_THRESH_PARAM;



/**
    parameter data for kdrv_sde_set, kdrv_sde_get
*/
typedef enum {
	KDRV_SDE_PARAM_IPL_OPENCFG = 0,                 ///< set engine open info, data_type: [Set/Get] KDRV_SDE_OPENCFG

	KDRV_SDE_PARAM_IPL_MODE_INFO,          		///< SDE mode selection, data_type: [Set/Get] KDRV_SDE_MODE_INFO
	KDRV_SDE_PARAM_IPL_IO_INFO,    				///< SDE image size info, input/output address, data_type: [Set/Get] KDRV_SDE_IO_INFO

	KDRV_SDE_PARAM_IPL_ISR_CB,					///< SDE external isr cb

	KDRV_SDE_PARAM_IQ_COST_PARAM,				///< SDE cost computation parameters, data_type: [Set/Get] KDRV_SDE_COST_PARAM
	KDRV_SDE_PARAM_IQ_LUMA_PARAM,				///< SDE luminance related parameters, data_type: [Set/Get] KDRV_SDE_LUMA_PARAM
	KDRV_SDE_PARAM_IQ_PENALTY_PARAM,			///< SDE scan penalty parameters, data_type: [Set/Get] KDRV_SDE_PENALTY_PARAM
	KDRV_SDE_PARAM_IQ_SMOOTH_PARAM,				///< SDE scan smoothness parameters, data_type: [Set/Get] KDRV_SDE_SMOOTH_PARAM
	KDRV_SDE_PARAM_IQ_LRC_THRESH_PARAM,			///< SDE LRC threshold parameters, data_type: [Set/Get] KDRV_SDE_LRC_THRESH_PARAM

	KDRV_SDE_PARAM_MAX,
	KDRV_SDE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_SDE_PARAM_ID)
} KDRV_SDE_PARAM_ID;
STATIC_ASSERT((KDRV_SDE_PARAM_MAX &KDRV_SDE_PARAM_REV) == 0);

#define KDRV_SDE_IGN_CHK KDRV_SDE_PARAM_REV  //only support set/get function



#ifdef __KERNEL__
extern void kdrv_sde_install_id(void);
#else
//extern void kdrv_sde_install_id(void) _SECTION(".kercfg_text");
#endif

/*!
 * @fn INT32 kdrv_sde_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @return return 0 on success, < 0 on error
 */
INT32 kdrv_sde_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_sde_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @return return 0 on success, < 0 on error
 */
INT32 kdrv_sde_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_sde_trigger(UINT32 id, KDRV_SDE_TRIGGER_PARAM *p_param,
 								KDRV_CALLBACK_FUNC *p_cb_func,
								VOID *p_user_data);
 * @brief trigger hardware engine
 * @param id           		the id of hardware
 * @param p_param           the parameter for trigger
 * @param p_cb_func         the callback function
 * @param p_user_data       the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_sde_trigger(UINT32 id, KDRV_SDE_TRIGGER_PARAM *p_param,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *p_user_data);


/*!
 * @fn INT32 kdrv_sde_set(UINT32 id, KDRV_SDE_PARAM_ID param_id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_sde_set(UINT32 id, KDRV_SDE_PARAM_ID param_id, VOID *p_param);

/*!
 * @fn INT32 kdrv_sde_get(UINT32 id, KDRV_SDE_PARAM_ID param_id, VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id   	    the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_sde_get(UINT32 id, KDRV_SDE_PARAM_ID param_id, VOID *p_param);

UINT32 kdrv_sde_init(VOID);
VOID kdrv_sde_uninit(VOID);

#endif //_KDRV_SDE_H_
