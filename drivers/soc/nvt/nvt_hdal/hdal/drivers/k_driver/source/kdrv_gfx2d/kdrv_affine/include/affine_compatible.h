#ifndef __AFFINE_COMPATIBLE_H_
#define __AFFINE_COMPATIBLE_H_

#include "kwrap/type.h"
#include "kdrv_gfx2d/kdrv_affine.h"

#if defined(__FREERTOS)
//#include "rcw_macro.h"
#else
//#include "mach/rcw_macro.h"
#endif

typedef enum _DRV_VER_INFO {

        AFFINE_DRV_VER_96650,
        AFFINE_DRV_VER_96660,
        AFFINE_DRV_VER_96680,
		AFFINE_DRV_VER_510,
		AFFINE_DRV_VER_520,

        ENUM_DUMMY4WORD(DRV_VER_INFO)
} DRV_VER_INFO;

/**
    Affine channel ID

*/
typedef enum _AFFINE_ID {
	AFFINE_ID_1,                          ///< AFFINE Controller

	ENUM_DUMMY4WORD(AFFINE_ID)
} AFFINE_ID;
/**
    Affine configuration ID

    @note For affine_setConfig()
*/
typedef enum{
	AFFINE_CONFIG_ID_FREQ,              ///< Affine operating frequency (unit: MHz). Context can be:
	///< - @b 160: 160 MHz
	///< - @b 240: 240 MHz
	///< - @b 380: 380 MHz
	///< - @b 420: 420 MHz

	// For emulation:
	AFFINE_CONFIG_ID_COEFF_A,           //< Affine coefficient A. Context can be UINT32 stores Q1.15
	AFFINE_CONFIG_ID_COEFF_B,           //< Affine coefficient B. Context can be UINT32 stores Q1.15
	AFFINE_CONFIG_ID_COEFF_C,           //< Affine coefficient C. Context can be UINT32 stores Q14.15
	AFFINE_CONFIG_ID_COEFF_D,           //< Affine coefficient D. Context can be UINT32 stores Q1.15
	AFFINE_CONFIG_ID_COEFF_E,           //< Affine coefficient E. Context can be UINT32 stores Q1.15
	AFFINE_CONFIG_ID_COEFF_F,           //< Affine coefficient F. Context can be UINT32 stores Q14.15
	AFFINE_CONFIG_ID_SUBBLK1,           //< Affine SubBlock1 Register
	AFFINE_CONFIG_ID_SUBBLK2,           //< Affine SubBlock2 Register
	AFFINE_CONFIG_ID_DIRECT_COEFF,      //< Direct affine coefficient path. Context can be:
	//< - @b TRUE: affine_request() will use coefficients set by AFFINE_CONFIG_ID_COEFF_A ~ AFFINE_CONFIG_ID_COEFF_F
	//< - @b FALSE: affine_request() will use coefficients passed through PAFFINE_REQUEST (default value)

	ENUM_DUMMY4WORD(AFFINE_CONFIG_ID)
} AFFINE_CONFIG_ID;

/**
    Affine Engine Register Domain

    @note For affine_request()
*/

typedef enum {
    AFFINE_REG_DOMAIN_APB,            ///< register on APB
    AFFINE_REG_DOMAIN_LL,             ///< register on LLC (Link List Control)

    ENUM_DUMMY4WORD(AFFINE_REG_DOMAIN)
} AFFINE_REG_DOMAIN;

/**
    Affine request structure

    @note For affine_request()
*/
typedef struct _AFFINE_REQUEST{
	DRV_VER_INFO ver_info;           ///< Driver version
	AFFINE_FORMAT format;           ///< format of operated images
	UINT32 width;                 ///< Image Width (unit: byte) (alignment: 16 bytes) (max: 16384)
	UINT32 height;                ///< Image Height (unit: line) (alignment: 8 lines) (max: 8192)
	PAFFINE_IMAGE p_src_img;          ///< Source Image
	PAFFINE_IMAGE p_dst_img;          ///< Destinaiton Image
	PAFFINE_COEFF p_coefficient;     ///< Affine matrix coefficients
	struct AFFINE_REQUEST *p_next;	///< Link to next request description. (Fill NULL if this is last item)
} AFFINE_REQUEST, *PAFFINE_REQUEST;

/*
	Request list element

*/
typedef struct _AFFINE_REQ_LIST_NODE {
	KDRV_AFFINE_TRIGGER_PARAM	trig_param;

	KDRV_CALLBACK_FUNC	callback;

	KDRV_AFFINE_EVENT_CB_INFO	cb_info;

//	struct list_head	list;
} AFFINE_REQ_LIST_NODE;


// Functions exported from affine driver

extern ER	affine_open(AFFINE_ID id);
extern ER	affine_close(AFFINE_ID id);

extern ER	affine_setconfig(AFFINE_ID id, AFFINE_CONFIG_ID config_id, UINT32 config_context);
extern ER	affine_getconfig(AFFINE_ID id, AFFINE_CONFIG_ID config_id, UINT32 *p_config_context);

extern ER	affine_request(AFFINE_ID id, PAFFINE_REQUEST p_request);

extern void affine_isr_bottom(AFFINE_ID id, UINT32 events);
extern ER	affine_trigger(AFFINE_ID id);
extern ER	affine_enqueue(AFFINE_ID id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
									KDRV_CALLBACK_FUNC *p_cb_func);

#endif
