/*
    @file       display_int.h
    @ingroup

    @brief      Internal Header file for Display control object
				This file is the header file that define the API and data type
				for Display control object

    @note       Nothing.


    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.

*/

#ifndef _DISPLAY_INT_H
#define _DISPLAY_INT_H

#include "display_obj_platform.h"

#define DISP_DEBUG  DISABLE
#if DISP_DEBUG
#define dispctr1_debug(msg)         DBG_IND msg
#define displyr1_debug(msg)         DBG_IND msg
#define dispdev1_debug(msg)         DBG_IND msg

#define dispctr2_debug(msg)         DBG_IND msg
#define displyr2_debug(msg)         DBG_IND msg
#define dispdev2_debug(msg)         DBG_IND msg

#else
#define dispctr1_debug(msg)
#define displyr1_debug(msg)
#define dispdev1_debug(msg)

#define dispctr2_debug(msg)
#define displyr2_debug(msg)
#define dispdev2_debug(msg)

#endif


#define DISP_PATCHBUF_RESIZE    ENABLE



#define DISPVDO_LYRNUM              2
#define DISPOSD_LYRNUM              2

#define DISP_VDO1                   0
#define DISP_VDO2                   1
#define DISP_OSD1                   0
#define DISP_OSD2                   1

#define DISPDIM_W                   0   // Dimension Width
#define DISPDIM_H                   1   // Dimension Height
#define DISPDIM_L                   2   // Dimension Line Offset

#define DISPBUF_X                   0   //X
#define DISPBUF_Y                   1   //Y


typedef enum {
	DISPVDOBUF_Y,
	DISPVDOBUF_CB,
	DISPVDOBUF_CR,

	DISPVDOBUF_MAX,
	ENUM_DUMMY4WORD(DISPBUFSEL)
} DISPVDOBUFSEL;

typedef enum {
	DISPOSDBUF_PALE,
	DISPOSDBUF_ALPHA,

	DISPOSDBUF_MAX,
	ENUM_DUMMY4WORD(DISPOSDBUFSEL)
} DISPOSDBUFSEL;


typedef struct {
	UINT32              ui_bll;
	UINT32              ui_brl;
	UINT32              ui_setup;
	UINT32              ui_y_scaling;
	UINT32              ui_cb_scaling;
	UINT32              ui_cr_scaling;
} DISPDEV_TVPAR, *PDISPDEV_TVPAR;


typedef struct {
	DISPDEV_ID          active_dev;  // Which device is the current active device for display engine.
	DISPCTRL_SRCCLK     src_clk;

	UINT32              ui_global_win_width;
	UINT32              ui_global_win_height;

	UINT32              pui_vdo_buf_dim[DISPVDO_LYRNUM][3];    // Video layer buffer dimension: width & height & Lineoffset
	UINT32              pui_vdo_win_dim[DISPVDO_LYRNUM][2];    // Video layer window dimension: width & height
	UINT32              pui_osd_buf_dim[DISPOSD_LYRNUM][3];    // OSD   layer buffer dimension: width & height & Lineoffset
	UINT32              pui_osd_win_dim[DISPOSD_LYRNUM][2];    // OSD   layer window dimension: width & height

	DISPACTBUF          act_vdo_index[DISPVDO_LYRNUM];
	DISPACTBUF          act_osd_index[DISPVDO_LYRNUM];
	UINT32              p_vdo_buf_addr[DISPVDO_LYRNUM][DISPACTBUF_NUM][DISPVDOBUF_MAX];
	UINT32              p_osd_buf_addr[DISPVDO_LYRNUM][DISPACTBUF_NUM][DISPOSDBUF_MAX];
	UINT32              pui_vdo_buf_xy[DISPVDO_LYRNUM][2];     //Video layer buffer xy: x & y
	UINT32              pui_osd_buf_xy[DISPOSD_LYRNUM][2];     //OSD   layer buffer xy: x & y

} DISP_DATA, *PDISP_DATA;






typedef struct {

	UINT32                  ui_buf_width;         // The display device optimal buffer width
	UINT32                  ui_buf_height;        // The display device optimal buffer height
	UINT32                  ui_win_width;         // The display device optimal window width
	UINT32                  ui_win_height;        // The display device optimal window height
	DISPDEV_TVADJUST        tv_adjust;           // TV adjustnebt resistor selection
	FP                      panel_adjust;        // Project layer can adjust the panel configurations your own by hooking this callback.
	HDMI_AUDIOFMT           hdmi_aud_fmt;        // HDMI Audio Format
	HDMI_VIDEOID			hdmi_vdo_fmt;        // HDMI video Format

	DISPDEV_LCDCTRL         lcd_ctrl;            // Device LCD Control Method
	SIF_CH                  sif_ch;              // If lcd_ctrl is SIF, This field is the SIF channel number
	UINT32                  ui_gpio_sif_sen;       // If lcd_ctrl is GPIO, This field is the GPIO SIF SEN pin.
	UINT32                  ui_gpio_sif_clk;       // If lcd_ctrl is GPIO, This field is the GPIO SIF CLK pin.
	UINT32                  ui_gpio_sif_data;      // If lcd_ctrl is GPIO, This field is the GPIO SIF DATA pin.

	UINT32                  ui_backlight;        // Current Backlight Level
	UINT32                  ui_power;            // Current Power Level
	DISPDEV_TYPE            disp_dev_type;
	BOOL                    b_tv_en_user;          // TV Enable user define paramter.
	DISPDEV_TVPAR           tv_par_ntsc;         // TV NTSC parameter
	DISPDEV_TVPAR           tv_par_pal;          // TV PAL parameter
	BOOL                    tv_full;             // TV FULL screen
} DISPDEV_DATA, *PDISPDEV_DATA;


/*
    Display Local Data structure
*/
typedef struct {
	DISP_DATA               disp_data;
	DISPDEV_DATA            disp_dev_data;
} DISP_INFO, *PDISP_INFO;















const    PDISP_CAP   disp_get_disp1_cap(void);
ER          disp_open_disp1(void);
ER          disp_close_disp1(void);
BOOL        disp_is_disp1_opened(void);
void        disp_set_disp1_load(BOOL b_wait_done);
void        disp_wait_disp1_frame_end(BOOL is_block);
void        disp_wait_disp1_yuv_output_done(void);
ER          disp_set_disp1_control(DISPCTRL_OP disp_ctrl, PDISPCTRL_PARAM p_disp_param);
ER          disp_set_disp1_layer_ctrl(DISPLAYER layer, DISPLAYER_OP lyr_op, PDISPLAYER_PARAM p_lyr_param);
ER          disp_set_disp1_device_ctrl(DISPDEV_OP dev_ctrl, PDISPDEV_PARAM p_dev_param);

const    PDISP_CAP   disp_get_disp2_cap(void);
ER          disp_open_disp2(void);
ER          disp_close_disp2(void);
BOOL        disp_is_disp2_opened(void);
void        disp_set_disp2_load(BOOL b_wait_done);
void        disp_wait_disp2_frame_end(BOOL is_block);
void        disp_wait_disp2_yuv_output_done(void);
ER          disp_set_disp2_control(DISPCTRL_OP disp_ctrl, PDISPCTRL_PARAM p_disp_param);
ER          disp_set_disp2_layer_ctrl(DISPLAYER layer, DISPLAYER_OP lyr_op, PDISPLAYER_PARAM p_lyr_param);
ER          disp_set_disp2_device_ctrl(DISPDEV_OP dev_ctrl, PDISPDEV_PARAM p_dev_param);



#endif
