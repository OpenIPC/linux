/**
	@brief Header file of vendor audioout module.\n
	This file contains the functions which is related to vendor audioout.

	@file vendor_audioout.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_AUDIOOUT_H_
#define _VENDOR_AUDIOOUT_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDOR_AUDIOOUT_NAME_LEN 32

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef enum _VENDOR_AUDIOOUT_ITEM {
	VENDOR_AUDIOOUT_ITEM_EXT,
	VENDOR_AUDIOOUT_ITEM_AUDIO_FILTER_EN,
	VENDOR_AUDIOOUT_ITEM_WAIT_PUSH,
	VENDOR_AUDIOOUT_ITEM_PRELOAD_DONE,
	VENDOR_AUDIOOUT_ITEM_DONE_SIZE,
	VENDOR_AUDIOOUT_ITEM_NEEDED_BUF,
	VENDOR_AUDIOOUT_ITEM_ALLOC_BUF,
	VENDOR_AUDIOOUT_ITEM_CHN_STATE,
	VENDOR_AUDIOOUT_ITEM_VOLUME,
	VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE,
	VENDOR_AUDIOOUT_ITEM_DMA_ABORT,
	ENUM_DUMMY4WORD(VENDOR_AUDIOOUT_ITEM)
} VENDOR_AUDIOOUT_ITEM;

typedef struct _VENDOR_AUDIOOUT_PINMUX_CFG {
	UINT32 audio_pinmux;
	UINT32 cmd_if_pinmux;
} VENDOR_AUDIOOUT_PINMUX_CFG;

typedef struct _VENDOR_AUDIOOUT_INIT_PIN_CFG {
	VENDOR_AUDIOOUT_PINMUX_CFG pinmux;
} VENDOR_AUDIOOUT_INIT_PIN_CFG;

typedef struct _VENDOR_AUDIOOUT_INIT_I2S_CFG {
	UINT32 bit_width;
	UINT32 bit_clk_ratio;
	UINT32 op_mode;
	UINT32 tdm_ch;
} VENDOR_AUDIOOUT_INIT_I2S_CFG;

typedef struct _VENDOR_AUDIOOUT_INIT_CFG_OBJ {
	VENDOR_AUDIOOUT_INIT_PIN_CFG pin_cfg;
	VENDOR_AUDIOOUT_INIT_I2S_CFG i2s_cfg;
} VENDOR_AUDIOOUT_INIT_CFG_OBJ;

typedef struct _VENDOR_AUDIOOUT_INIT_CFG {
	CHAR driver_name[VENDOR_AUDIOOUT_NAME_LEN];
	VENDOR_AUDIOOUT_INIT_CFG_OBJ aud_init_cfg;
} VENDOR_AUDIOOUT_INIT_CFG;

typedef struct _VENDOR_AUDIOOUT_MEM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
} VENDOR_AUDIOOUT_MEM;

typedef struct _VENDOR_AUDIOOUT_CHN_STATE {
	UINT32 total_num;
	UINT32 free_num;
	UINT32 busy_num;
} VENDOR_AUDIOOUT_CHN_STATE;

typedef struct _VENDOR_AUDIOOUT_VOLUME {
	UINT32                 volume;       ///< audio input volume
} VENDOR_AUDIOOUT_VOLUME;


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT vendor_audioout_set(UINT32 id, VENDOR_AUDIOOUT_ITEM item, VOID *p_param);
HD_RESULT vendor_audioout_get(UINT32 id, VENDOR_AUDIOOUT_ITEM item, VOID *p_param);
#endif

