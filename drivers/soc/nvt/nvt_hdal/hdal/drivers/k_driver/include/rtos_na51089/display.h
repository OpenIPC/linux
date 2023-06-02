/**
    @file       display.h
    @ingroup    mISYSDisp

    @brief      Header file for Display control object
				This file is the header file that define the API and data type
				for Display control object
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

/** \addtogroup  mISYSDisp*/
//@{

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "dispcomn.h"
#include "displyr.h"
#include "dispctrl.h"
#include "dispdevctrl.h"
#if !(defined __UITRON || defined __ECOS || defined __FREERTOS)
#include <linux/fb.h>
#endif

/**
    Display Control Object

    The display control object which provides unified interface for display device/engine control.
    The user can get the display object by API: disp_get_display_object() first, and then access the display engine through the
    display object. The disp_ctrl/disp_lyr_ctrl/dev_ctrl would not set load to activate the configurations, the user must set load
    after all the settings is done.
*/
typedef struct {
	const DISP_ID   disp_id;                     ///< DISPAY object ID: DISP_1 or DISP_2.

	const PDISP_CAP(*get_disp_cap)(void);         ///< Display object capabilities.

	ER(*open)(void);                            ///< Open display object.
	ER(*close)(void);                           ///< Close display object.
	BOOL (*is_opened)(void);                     ///< Check if display object is opened or not.

	DISPCTRL        disp_ctrl;                   ///< Control interface for common functions of physical display engine.
	DISPLYRCTRL     disp_lyr_ctrl;                ///< Control interface for layer function control of physical display engine.
	DISPDEVCTRL     dev_ctrl;                    ///< Display device control such as device open/close/register_access and hook DISPDEV_OBJ.

	void (*load)(BOOL b_wait_done);               ///< Set Load to activate the display configurations.
	void (*wait_frm_end)(BOOL is_block);                   ///< Wait the display device frame end.
	void (*wait_out_dram_done)(void);              ///< Wait output dram done

	PDISPDEV_OBJ (*dev_callback)(void);				  ///< Callback for lcd device to register dev obj.

} DISP_OBJ, *PDISP_OBJ;



//
//  Export Display Object API
//
extern PDISP_OBJ disp_get_display_object(DISP_ID disp_id);
#define disp_getDisplayObject(disp_id) disp_get_display_object(disp_id)

#if !(defined __UITRON || defined __ECOS || defined __FREERTOS)
int display_obj_set_video1_blank(DISP_ID id, unsigned int blank_mode);
int display_obj_set_video2_blank(DISP_ID id, unsigned int blank_mode);
int display_obj_set_osd_blank(DISP_ID id, unsigned int blank_mode);
int display_obj_set_osd_palette(DISP_ID id, unsigned int regno, unsigned int val);
int display_obj_get_video1_buf_addr(DISP_ID id, unsigned long *phys_addr, char __iomem **virt_addr, unsigned int *buffer_len);
int display_obj_get_video2_buf_addr(DISP_ID id, unsigned long *phys_addr, char __iomem **virt_addr, unsigned int *buffer_len);
int display_obj_get_osd_buf_addr(DISP_ID id, unsigned long *phys_addr, char __iomem **virt_addr, unsigned int *buffer_len);
int display_obj_get_video1_param(DISP_ID id, struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix);
int display_obj_get_video2_param(DISP_ID id, struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix);
int display_obj_get_osd_param(DISP_ID id, struct fb_var_screeninfo *var, struct fb_fix_screeninfo *fix);
int display_obj_set_osd_addr(DISP_ID id, unsigned int addr, bool load);
int display_obj_get_layer_support(DISP_ID id, DISPLAYER layer);

#define A_ADDR(osd_buf_size, rgb_addr) ALIGN_CEIL_16(rgb_addr + osd_buf_size) //osd_buf_size = osd buf (line offset * height)

#endif
//@}

#endif
