
#ifndef __VENDOR_VIDEOOUT_H__
#define __VENDOR_VIDEOOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hdal.h"

typedef enum _VENDOR_VIDEOOUT_ID {
	VENDOR_VIDEOOUT_ID0 = 0,		///< device id 0
	VENDOR_VIDEOOUT_ID1,			///< device id 1
	ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_ID)
} VENDOR_VIDEOOUT_ID;

typedef enum _VENDOR_VIDEOOUT_INFUNC {
	VENDOR_VIDEOOUT_INFUNC_KEEP_LAST =     0x01000000, ///< stop vout,would keep last frame in tmp
	ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_INFUNC)
} VENDOR_VIDEOOUT_INFUNC;

typedef struct _VENDOR_VIDEOOUT_INCONTINUOUS_MASK {
	UINT32              color;
	HD_UPOINT           position;
	UINT32              h_line_len;
	UINT32              h_hole_len;
	UINT32              v_line_len;
	UINT32              v_hole_len;
	UINT32              h_thickness;
	UINT32              v_thickness;
} VENDOR_VIDEOOUT_INCONTINUOUS_MASK;

typedef struct _VENDOR_FB_INIT {
	HD_FB_ID fb_id; 	///< select fb layer
    UINT32 pa_addr;     ///< fb physical addr
    UINT32 buf_len;     ///< buffer length
} VENDOR_FB_INIT;

typedef struct _VENDOR_FB_UNINIT {
	HD_FB_ID fb_id; 	///< select fb layer
} VENDOR_FB_UNINIT;

typedef struct _VENDOR_VIDEOOUT_FUNC_CONFIG {
	HD_VIDEOOUT_INFUNC in_func;         ///< additional function of in (bit-wise mask)
} VENDOR_VIDEOOUT_FUNC_CONFIG;

typedef struct _VENDOR_VIDEOOUT_IF_CFG {
    UINT32 lcd_ctrl;         ///< select the LCD Control Interface.
	UINT32 ui_sif_ch;        ///< If lcd_ctrl=SIF, this field is the assigned SIF channel number.
	UINT32 ui_gpio_sen;      ///< If lcd_ctrl=GPIO,this field is the assigned GPIO pin name for SIF SEN.
	UINT32 ui_gpio_clk;      ///< If lcd_ctrl=GPIO,this field is the assigned GPIO pin name for SIF CLK.
	UINT32 ui_gpio_data;     ///< If lcd_ctrl=GPIO,this field is the assigned GPIO pin name for SIF DATA.
} VENDOR_VIDEOOUT_IF_CFG;

typedef struct _VENDOR_VIDEOOUT_DEV_CONFIG{
    VENDOR_VIDEOOUT_IF_CFG if_cfg;
} VENDOR_VIDEOOUT_DEV_CONFIG;

typedef enum _VENDOR_VIDEOOUT_ITEM {
  	VENDOR_VIDEOOUT_ITEM_INCONTINUOUS_MASK  = 0,
  	VENDOR_VIDEOOUT_ITEM_FB_INIT,
  	VENDOR_VIDEOOUT_ITEM_FB_UNINIT,
  	VENDOR_VIDEOOUT_ITEM_ENTER_SLEEP,
  	VENDOR_VIDEOOUT_ITEM_EXIT_SLEEP,
  	VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG,
  	VENDOR_VIDEOOUT_ITEM_DEV_CONFIG,
  	VENDOR_VIDEOOUT_ITEM_ABORT,
	VENDOR_VIDEOOUT_ITEM_MAX,
	ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_ITEM)
} VENDOR_VIDEOOUT_ITEM;

HD_RESULT vendor_videoout_set(UINT32 id, VENDOR_VIDEOOUT_ITEM item, VOID *p_param);
HD_RESULT vendor_videoout_get(UINT32 id, VENDOR_VIDEOOUT_ITEM item, VOID *p_param);

#ifdef __cplusplus
}
#endif

#endif // __VENDOR_VIDEOOUT_H__

