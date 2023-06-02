/**
 * @file kdrv_affine.h
 * @brief type definition of KDRV API.
 * @author ESW
 * @date in the year 2018
 */

#ifndef __KDRV_AFFINE_H__
#define __KDRV_AFFINE_H__

#include "kdrv_type.h"



/* struct _KDRV_AFFINE_PARAM_ID: structure for set/get functions
 *
 */
typedef enum _KDRV_AFFINE_PARAM_ID {
	AFFINE_PARAM_FREQ,///< [set/get] Frequency config. (unit: MHz)
	///<    valid setting: 240, 380, 420, 480

	// For emulation:
	AFFINE_PARAM_COEFF_A,           //< Affine coefficient A. Context can be UINT32 stores Q1.15
	AFFINE_PARAM_COEFF_B,           //< Affine coefficient B. Context can be UINT32 stores Q1.15
	AFFINE_PARAM_COEFF_C,           //< Affine coefficient C. Context can be UINT32 stores Q14.15
	AFFINE_PARAM_COEFF_D,           //< Affine coefficient D. Context can be UINT32 stores Q1.15
	AFFINE_PARAM_COEFF_E,           //< Affine coefficient E. Context can be UINT32 stores Q1.15
	AFFINE_PARAM_COEFF_F,           //< Affine coefficient F. Context can be UINT32 stores Q14.15
	AFFINE_PARAM_SUBBLK1,           //< Affine SubBlock1 Register
	AFFINE_PARAM_SUBBLK2,           //< Affine SubBlock2 Register
	AFFINE_PARAM_DIRECT_COEFF,      //< Direct affine coefficient path. Context can be:
	//< - @b TRUE:  affine will use coefficients set by AFFINE_CONFIG_ID_COEFF_A ~ AFFINE_CONFIG_ID_COEFF_F
	//< - @b FALSE: affine will use coefficients passed through PAFFINE_REQUEST (default value)
} KDRV_AFFINE_PARAM_ID;

/*
 * structured passed to callback's 2nd parameter "user_data"
 */
typedef struct _KDRV_AFFINE_EVENT_CB_INFO {
	UINT32		handle;	///< handle feed by kdrv_affine_trigger()
	UINT32		timestamp;	///< timestamp when job complete
} KDRV_AFFINE_EVENT_CB_INFO;


/**
    Affine data format

    Select data format of affine operation.

    @note For AFFINE_REQUEST
*/
typedef enum _AFFINE_FORMAT {
	AFFINE_FORMAT_FORMAT_8BITS,             ///< Normal 8 bits data
	AFFINE_FORMAT_FORMAT_16BITS_UVPACK,     ///< 16 bits U/V pack data and apply to both plane

	ENUM_DUMMY4WORD(AFFINE_FORMAT)

} AFFINE_FORMAT;

/**
    Affine image structure

    Description of affine image

    @note For AFFINE_REQUEST
*/
typedef struct _AFFINE_IMG{
	UINT32 uiImageAddr; 			///< DRAM address of image (word alignment)
	UINT32 uiLineOffset;			///< Lineoffset of image (word alignment)
} AFFINE_IMAGE, *PAFFINE_IMAGE;

/**
    Affine coefficient structure

    @note For AFFINE_REQUEST
*/
typedef struct _AFFINE_COEFF{
	FLOAT fCoeffA;                  ///< Coefficient A of affine matrix
	FLOAT fCoeffB;                  ///< Coefficient B of affine matrix
	FLOAT fCoeffC;                  ///< Coefficient C of affine matrix
	FLOAT fCoeffD;                  ///< Coefficient D of affine matrix
	FLOAT fCoeffE;                  ///< Coefficient E of affine matrix
	FLOAT fCoeffF;                  ///< Coefficient F of affine matrix
} AFFINE_COEFF, *PAFFINE_COEFF;


typedef struct _KDRV_AFFINE_TRIGGER_PARAM {
	UINT32                ver;           ///< driver version (reserved)
	AFFINE_FORMAT         format;        ///< format of operated images
	UINT32                uiWidth;       ///< Image Width (unit: byte) (alignment: 16 bytes) (max: 16384)
	UINT32                uiHeight;      ///< Image Height (unit: line) (alignment: 8 lines) (max: 8192)
	PAFFINE_IMAGE         pSrcImg;       ///< Source Image
	PAFFINE_IMAGE         pDstImg;       ///< Destinaiton Image
	PAFFINE_COEFF         pCoefficient;  ///< Affine matrix coefficients
	struct KDRV_AFFINE_TRIGGER_PARAM *pNext;	///< Link to next request description. (Fill NULL if this is last item)
} KDRV_AFFINE_TRIGGER_PARAM;

/*!
 * @fn INT32 kdrv_affine_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 *			- @b KDRV_GFX2D_AFFINE: affine engine
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_affine_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 *			- @b KDRV_GFX2D_AFFINE: affine engine
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_affine_trigger(INT32 handler, KDRV_AFFINE_TRIGGER_PARAM *p_param,
							     KDRV_CALLBACK_FUNC *p_cb_func,
								 VOID *p_user_data);
 * @brief trigger hardware engine
 * @param id				the id of hardware
 * @param p_param			the parameter for trigger
 * @param p_cb_func			the callback function
 * @param p_user_data			the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_trigger(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
						  KDRV_CALLBACK_FUNC *p_cb_func,
						  VOID *p_user_data);

/*!
 * @fn INT32 kdrv_affine_get(UINT32 handler, KDRV_AFFINE_PARAM_ID id, VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id		the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_get(UINT32 id, KDRV_AFFINE_PARAM_ID param_id, VOID *p_param);

/*!
 * @fn INT32 kdrv_affine_set(UINT32 handler, KDRV_AFFINE_PARAM_ID id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id		the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_set(UINT32 id, KDRV_AFFINE_PARAM_ID param_id, VOID *p_param);


#endif
