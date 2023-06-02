/**
    @file       dispdevctrl.h
    @ingroup    mISYSDisp

    @brief      Header file for Display Device control
				This file is the header file that define the API and data type
				for Display control object
	@note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

/** \addtogroup  mISYSDisp*/
//@{

#ifndef _DEVCTRL_H
#define _DEVCTRL_H

#if defined __KERNEL__
#include "comm/sif_api.h"
#include "hdmitx.h"
#elif defined __FREERTOS
#include "comm/sif_api.h"
#include "hdmitx.h"
#else
#include "sif.h"
#include "hdmitx.h"
#include "top.h"
#endif
#include "dispcomn.h"

/**
    @name Display Device Control Group

    These definitions are used for display control API (DISP_OBJ.dev_ctrl) as the display device control portal.
    The display device such as TV_PAL/TV_NTSC/HDMI/panel are controlled by this API.
*/
//@{

/**
    Display Device rotation ID definition

    This definition is used for the OP Code: "DISPDEV_SET_ROTATE" in the dev_ctrl API to specify
    the display device rotation configuration.
*/
typedef enum {
	DISPDEV_LCD_ROTATE_NONE,    ///< Config panel Register to Set LCD None of rotation.
	DISPDEV_LCD_ROTATE_180,     ///< Config panel Register to Set LCD rotation 180 degrees.

	ENUM_DUMMY4WORD(DISPDEV_LCD_ROTATE)
} DISPDEV_LCD_ROTATE;

/**
    Project Releated display device specific control

    This stucture is used to store project Releated display device specific control,
    such as display backlight / power control.
*/
typedef struct {
	ER(*DISPDEVBLCTRL)(UINT32 ui_param);                           ///< The Callback prototype for the LCD backlight control
	ER(*DISPDEVPWRCTRL)(UINT32 ui_param);                          ///< The Callback prototype for the LCD Power control
	ER(*DISPDEVUSERCTRL)(UINT32 ui_param);                          ///< The Callback prototype for the LCD user control

} DISPDEV_SPECIFIC_CTRL, *PDISPDEV_SPECIFIC_CTRL;

/**
    Display Device ID definition

    This definition is used for the OP Code: "DISPDEV_OPEN_DEVICE"/"HOOK_DEVICE_OBJECT" in the dev_ctrl API to specify
    which of the display device would be controlled.
*/
typedef enum {
	DISPDEV_ID_TVNTSC,      ///< select display device: TV NTSC
	DISPDEV_ID_TVPAL,       ///< select display device: TV PAL
	DISPDEV_ID_TVHDMI,      ///< select display device: TV HDMI
	DISPDEV_ID_PANEL,       ///< select display device: panel
	DISPDEV_ID_MIPIDSI,     ///< select display device: MIPI DSI

	DISPDEV_ID_MAX,
	DISPDEV_ID_NULL,        // Default value is none of display device
	ENUM_DUMMY4WORD(DISPDEV_ID)
} DISPDEV_ID;


/**
    Project Releated display device pre get size before open.

    This stucture is used to get display device pre get size before open.
*/
typedef struct {
	DISPDEV_ID              ui_dev;
	UINT32                  ui_buf_width;
	UINT32                  ui_buf_height;
	UINT32                  ui_win_width;
	UINT32                  ui_win_height;
} DISPDEV_GET_PRESIZE, *PDISPDEV_GET_PRESIZE;


/**
    Display Device Object Structure

    This field is the display device object structure which is used to open/close display device
    and also provides the interface to access the display device registers.
    This structure is used by the DISPLAY Object driver in the driver design. Normal user should not access the
    display device by the device object directly.
*/
typedef struct {
	ER(*open)(void);                                                ///< Open the display device
	ER(*close)(void);                                               ///< Close the display device
	ER(*rotate)(DISPDEV_LCD_ROTATE rot);                            ///< select rotation from the display device

	void (*reg_write)(UINT32 ui_ofs, UINT32 ui_val);                   ///< Display Device register WRITE
	UINT32(*reg_read)(UINT32 ui_ofs);                                 ///< Display Device register READ

	ER(*get_pre_size)(DISPDEV_GET_PRESIZE *ui_size);                       ///< Get the display buffer size before open

	void (*set_dev_io_ctrl)(FP);                                       ///< Set Display Device IOCTRL API.
	DISPDEV_SPECIFIC_CTRL   specific_ctrl;                           ///< Device specific control, such as backlight/power pin control.
} DISPDEV_OBJ, *PDISPDEV_OBJ;


/**
    Display Device control operation ID definition

    This field is used as the first operand in the dev_ctrl API of the display object to specify
    which of the display device control operation would be performed.
*/
typedef enum {
	/* SET control group */
	DISPDEV_OPEN_DEVICE = DISPCMDBASE_DISPDEV,  ///< Open spcified display device.
	DISPDEV_CLOSE_DEVICE,                       ///< Close current display active device.
	DISPDEV_HOOK_DEVICE_OBJECT,                 ///< Hook panel object to display control object.
	DISPDEV_HOOK_PANEL_ADJUST,                  ///< Hook panel adjustment callback when panel opened.
	DISPDEV_SET_TVADJUST,                       ///< Specify the TV parameter with matched external resistance value.
	DISPDEV_SET_REG_IF,                         ///< Set    display Device(panel) Register Access Interface.
	DISPDEV_REG_WRITE,                          ///< Write  display Device(panel) Register.
	DISPDEV_SET_HDMIMODE,                       ///< Change the HDMI Audio/Video mode.
	DISPDEV_SET_ROTATE,                         ///< Set display Device rotation.
	DISPDEV_SET_PANEL_BACKLIGHT,                ///< Set the panel's backlight level.
	DISPDEV_SET_PANEL_POWER,                    ///< Set the panel's power level.
	DISPDEV_SET_LCDMODE,                        ///< Change the panel's mode
	DISPDEV_SET_POWERDOWN,                      ///< Set Powerdown
	DISPDEV_SET_TVPAR,                          ///< Set TV's parameter
	DISPDEV_SET_OUTPUT_DRAM,                    ///< Set ide data output to dram(only available in IDE1 with VDO2 disable)
	DISPDEV_SET_TVFULL,                         ///< Set TV full screen

	/* GET control group */
	DISPDEV_GET_ACT_DEVICE,                     ///< Get current active device. (panel/TV/HDMI)
	DISPDEV_GET_DISPSIZE,                       ///< Get display device optimal buffer/window size.
	DISPDEV_GET_PANEL_ADJUST,                   ///< Get the panel adjustment callback.
	DISPDEV_GET_TVADJUST,                       ///< Get the current TV adjustment configurations.
	DISPDEV_GET_REG_IF,                         ///< Get display Device(panel) Register Access Interface.
	DISPDEV_REG_READ,                           ///< Read display Device(panel) Register.
	DISPDEV_GET_HDMIMODE,                       ///< Get the HDMI Audio/Video mode
	DISPDEV_GET_PANEL_BACKLIGHT,                ///< Get the panel's backlight level.
	DISPDEV_GET_PANEL_POWER,                    ///< Get the panel's power level.
	DISPDEV_GET_PREDISPSIZE,                    ///< Pre Get display device optimal buffer/window size before open device.
	DISPDEV_GET_LCDMODE,
	DISPDEV_GET_POWERDOWN,                      ///< Get Powerdown
	DISPDEV_GET_TVPAR,                          ///< Get TV's parameter

	ENUM_DUMMY4WORD(DISPDEV_OP)
} DISPDEV_OP;

/**
    TV NTSC/PAL adjustment resistor value

    This field specifies the external HW resistor value in the TV output path.
    This would change the TV internal parameters. This field is used for the OP code "DISPDEV_SET_TVADJUST" of the dev_ctrl API.
*/
typedef enum {
	DISPDEV_TVADJUST_3_6_K,     ///< TV use resistor 3.6K as loading
	DISPDEV_TVADJUST_3_3_K,     ///< TV use resistor 3.3K as loading
	DISPDEV_TVADJUST_3_0_K,     ///< TV use resistor 3.0K as loading

	DISPDEV_TVADJUST_TOTAL,
	DISPDEV_TVADJUST_DEFAULT = DISPDEV_TVADJUST_3_3_K, ///< Default optimal resistor value
	ENUM_DUMMY4WORD(DISPDEV_TVADJUST)
} DISPDEV_TVADJUST;

/**
    LCD Control Interface Selection

    This field is used for the OP code "DISPDEV_SET_REG_IF" of the dev_ctrl API to specify the LCD(/panel) control interface.
    If the control interface is GPIO, user must also assign the GPIO pins as sif_sen/sif_clk/sif_data according to external HW circuit layout.
*/
typedef enum {
	DISPDEV_LCDCTRL_SIF,        ///< LCD Control Interface use SIF engine. User must assign SIF channel number "ui_sif_ch".
	DISPDEV_LCDCTRL_GPIO,       ///< LCD Control Interface use SW controlled GPIO pins. User must assign SIF pins: "ui_gpio_sen/ui_gpio_clk/ui_gpio_data".
	DISPDEV_LCDCTRL_MI,         ///< LCD control Interface use Embedded MI engine.

	DISPDEV_LCDCTRL_NONE,       ///< Default value as None.
	ENUM_DUMMY4WORD(DISPDEV_LCDCTRL)
} DISPDEV_LCDCTRL;

/**
    DISPDEV LCD mode Selection

    This field is used for the OP code "DISPDEV_SET_LCDMODE" of the dev_ctrl API to specify the LCD mode.
*/
typedef enum {
	DISPDEV_LCDMODE_RGB_SERIAL           =    0,     ///< LCD mode is RGB Serial or UPS051
	DISPDEV_LCDMODE_RGB_PARALL           =    1,     ///< LCD mode is RGB Parallel (888)
	DISPDEV_LCDMODE_YUV640               =    2,     ///< LCD mode is YUV640
	DISPDEV_LCDMODE_YUV720               =    3,     ///< LCD mode is YUV720
	DISPDEV_LCDMODE_RGBD360              =    4,     ///< LCD mode is RGB Dummy 360
	DISPDEV_LCDMODE_RGBD320              =    5,     ///< LCD mode is RGB Dummy 320
	DISPDEV_LCDMODE_RGB_THROUGH          =    6,     ///< LCD mode is RGB through mode
	DISPDEV_LCDMODE_CCIR601              =    7,     ///< LCD mode is CCIR601
	DISPDEV_LCDMODE_CCIR656              =    8,     ///< LCD mode is CCIR656
	DISPDEV_LCDMODE_RGB_PARALL666        =    9,     ///< LCD mode is RGB Parallel 666
	DISPDEV_LCDMODE_RGB_PARALL565        =   10,     ///< LCD mode is RGB Parallel 565
	DISPDEV_LCDMODE_RGB_PARALL_DELTA     =   11,     ///< LCD mode is RGB Parallel Delta
	DISPDEV_LCDMODE_MIPI                 =   12,     ///< LCD mode is MIPI Display

	DISPDEV_LCDMODE_MI_OFFSET            = 32,       ///< Memory LCD mode offset
	DISPDEV_LCDMODE_MI_FMT0              = 32 + 0,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT1              = 32 + 1,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT2              = 32 + 2,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT3              = 32 + 3,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT4              = 32 + 4,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT5              = 32 + 5,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT6              = 32 + 6,   ///< LCD mode is Memory(Parallel Interface) 8bits
	DISPDEV_LCDMODE_MI_FMT7              = 32 + 7,   ///< LCD mode is Memory(Parallel Interface) 9bits
	DISPDEV_LCDMODE_MI_FMT8              = 32 + 8,   ///< LCD mode is Memory(Parallel Interface) 16bits
	DISPDEV_LCDMODE_MI_FMT9              = 32 + 9,   ///< LCD mode is Memory(Parallel Interface) 16bits
	DISPDEV_LCDMODE_MI_FMT10             = 32 + 10,  ///< LCD mode is Memory(Parallel Interface) 18bits
	DISPDEV_LCDMODE_MI_FMT11             = 32 + 11,  ///< LCD mode is Memory(Parallel Interface)  8bits
	DISPDEV_LCDMODE_MI_FMT12             = 32 + 12,  ///< LCD mode is Memory(Parallel Interface) 16bits
	DISPDEV_LCDMODE_MI_FMT13             = 32 + 13,  ///< LCD mode is Memory(Parallel Interface) 16bits
	DISPDEV_LCDMODE_MI_FMT14             = 32 + 14,  ///< LCD mode is Memory(Parallel Interface) 16bits
	DISPDEV_LCDMODE_MI_SERIAL_BI         = 32 + 20,  ///< LCD mode is Serial Interface bi-direction
	DISPDEV_LCDMODE_MI_SERIAL_SEP        = 32 + 21,  ///< LCD mode is Serial Interface separation

	DISPDEV_LCDMODE_NO_CHANGE            = 0xFF,     ///< LCD mode use current setting

	ENUM_DUMMY4WORD(DISPDEV_LCDMODE)
} DISPDEV_LCDMODE;




/**
    Display Device Control Parameter Definition

    This definition is used as the second operand in the dev_ctrl API to specify the display device control parameters
    for each of the display device control opeation.
*/
typedef struct {
	union {
		/* SET control group */

		/**
		    Open Display Device as specified dev_id

		    This structure is used for display device control operation code "DISPDEV_OPEN_DEVICE"
		    to open display device. This structure shuld be used like this:
		\n  DispDev.SEL.OPEN_DEVICE.dev_id    = DISPDEV_ID_PANEL;
		\n  pDispObj->dev_ctrl(DISPDEV_OPEN_DEVICE, &DispDev);
		*/
		struct { // DISPDEV_OPEN_DEVICE
			DISPDEV_ID          dev_id;      ///< Open specified display device.
			BOOL				user_data_en;
			UINT32				user_data;
		} OPEN_DEVICE;


		// DISPDEV_CLOSE_DEVICE
		/* No Parameters */


		/**
		    Hook Display Device object

		    This structure is used for display device control operation code "DISPDEV_HOOK_DEVICE_OBJECT"
		    to hook specified dev_id's device object. This operation can be done at the systemInit phase.
		    This structure shuld be used like this:
		\n  DispDev.SEL.HOOK_DEVICE_OBJECT.dev_id        = DISPDEV_ID_TVHDMI;
		\n  DispDev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj  = the dispobj which to hook(ex: &DispDev_HdmiObj);
		\n  pDispObj->dev_ctrl(DISPDEV_OPEN_DEVICE, &DispDev);
		*/
		struct { // DISPDEV_HOOK_DEVICE_OBJECT
			DISPDEV_ID      dev_id;              ///< select which Device ID of the device object
			PDISPDEV_OBJ    p_disp_dev_obj;        ///< Hook Device object pointer for the dev_id
		} HOOK_DEVICE_OBJECT;

		/**
		    Hook panel Adjustment callback function

		    This is backward compatible operation for providing user interface to adjust the panel configuration by his own.
		    This structure is used for display device control operation code "DISPDEV_HOOK_PANEL_ADJUST"
		    to hook specified panel adjustment callback.
		    If this operation is performed, the adjustment in the panel device object would be invalid.
		    This operation can be done at the systemInit phase.
		    This structure shuld be used like this:
		\n  DispDev.SEL.HOOK_PANEL_ADJUST.fp_adjust        = (Addess of the panel adjust function);
		\n  pDispObj->dev_ctrl(DISPDEV_HOOK_PANEL_ADJUST, &DispDev);
		*/
		struct { // DISPDEV_HOOK_PANEL_ADJUST
			FP              fp_adjust;       ///< The callback function of the panel adjustment
		} HOOK_PANEL_ADJUST;

		/**
		    Set TV Device adjustment Resistor value

		    This structure is used for display device control operation code "DISPDEV_SET_TVADJUST"
		    to assign the tv adjustment paramter. This structure shuld be used like this:
		\n  DispDev.SEL.SET_TVADJUST.tv_adjust    = DISPDEV_TVADJUST_DEFAULT;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_TVADJUST, &DispDev);
		*/
		struct { // DISPDEV_SET_TVADJUST
			DISPDEV_TVADJUST    tv_adjust;   ///< Assign the panel adjustment paramter.
		} SET_TVADJUST;

		/**
		    Set LCD Device control interface

		    This structure is used for display device control operation code "DISPDEV_SET_REG_IF"
		    to specify the LCD control interface. This structure shuld be used like this:
		\n  DispDev.SEL.SET_REG_IF.lcd_ctrl    = DISPDEV_LCDCTRL_SIF;
		\n  DispDev.SEL.SET_REG_IF.ui_sif_ch    = SIF_CH0;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_REG_INTF, &DispDev);
		\n  Or Using like this:
		\n  DispDev.SEL.SET_REG_IF.lcd_ctrl    = DISPDEV_LCDCTRL_GPIO;
		\n  DispDev.SEL.SET_REG_IF.ui_gpio_sen  = P_GPIO_0;
		\n  DispDev.SEL.SET_REG_IF.ui_gpio_clk  = P_GPIO_1;
		\n  DispDev.SEL.SET_REG_IF.ui_gpio_data = P_GPIO_2;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_REG_IF, &DispDev);
		\n  Or Using like this:
		\n  DispDev.SEL.SET_REG_IF.lcd_ctrl    = DISPDEV_LCDCTRL_MI;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_REG_IF, &DispDev);
		*/
		struct { // DISPDEV_SET_REG_IF
			DISPDEV_LCDCTRL lcd_ctrl;        ///< select the LCD Control Interface.

			SIF_CH          ui_sif_ch;        ///< If lcd_ctrl=DISPDEV_LCDCTRL_SIF, this field is the assigned SIF channel number.
			UINT32          ui_gpio_sen;      ///< If lcd_ctrl=DISPDEV_LCDCTRL_GPIO,this field is the assigned GPIO pin name for SIF SEN.
			UINT32          ui_gpio_clk;      ///< If lcd_ctrl=DISPDEV_LCDCTRL_GPIO,this field is the assigned GPIO pin name for SIF CLK.
			UINT32          ui_gpio_data;     ///< If lcd_ctrl=DISPDEV_LCDCTRL_GPIO,this field is the assigned GPIO pin name for SIF DATA.
		} SET_REG_IF;

		/**
		    Write Display Device register

		    This structure is used for display device control operation code "DISPDEV_REG_WRITE"
		    to write display device register. This structure shuld be used like this:
		\n  DispDev.SEL.REG_WRITE.ui_addr    = 0x00;
		\n  DispDev.SEL.REG_WRITE.ui_value   = 0x01;
		\n  pDispObj->dev_ctrl(DISPDEV_REG_WRITE, &DispDev);
		*/
		struct { // DISPDEV_REG_WRITE
			UINT32          ui_addr;         ///< LCD Register address
			UINT32          ui_value;        ///< The LCD register configuration value
		} REG_WRITE;

		/**
		    Set HDMI display Video and Audio mode

		    This structure is used for display device control operation code "DISPDEV_SET_HDMIMODE"
		    to configure the HDMI Video/Audio mode when HDMI cable connected.
		    If the TV do not support the selected video ID, the HDMI basic format would be displayed.(480P60)
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_HDMIMODE.video_id    = HDMI_1280X720P60;
		\n  DispDev.SEL.SET_HDMIMODE.audio_id    = HDMI_AUDIO32KHZ;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_HDMIMODE, &DispDev);
		*/
		struct { // DISPDEV_SET_HDMIMODE
			HDMI_VIDEOID    video_id;        ///< select HDMI default Video mode ID
			HDMI_AUDIOFMT   audio_id;        ///< select HDMI default Audio mode ID
		} SET_HDMIMODE;

		/**
		    Set LCD mode

		    This structure is used for display device control operation code "DISPDEV_SET_LCDMODE"
		    to configure the LCD mode when change LCD mode
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_LCDMODE    = PINMUX_LCDMODE_YUV640;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_LCDMODE, &DispDev);
		*/
		struct { // DISPDEV_SET_LCDMODE
			DISPDEV_LCDMODE  mode;      ///< select LCD mode
		} SET_LCDMODE;


		/**
		    Set Display Device rotation

		    This structure is used for display device control operation code "DISPDEV_SET_ROTATE"
		    to configure the display device(LCD) rotation configuration.
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_ROTATE.rot    = DISPDEV_LCD_ROTATE_180;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_ROTATE, &DispDev);
		*/
		struct { // DISPDEV_SET_ROTATE
			DISPDEV_LCD_ROTATE    rot;        ///< select display device rotate
		} SET_ROTATE;

		/**
		    Set Display panel(LCD) Device backlight level

		    This structure is used for display device control operation code "DISPDEV_SET_PANEL_BACKLIGHT"
		    to set the Display panel(LCD) Device backlight level.
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_PANEL_BACKLIGHT.ui_bl_lvl    = (project defined levels);
		\n  pDispObj->dev_ctrl(DISPDEV_SET_PANEL_BACKLIGHT, &DispDev);
		*/
		struct { // DISPDEV_SET_PANEL_BACKLIGHT
			UINT32      ui_bl_lvl;
		} SET_PANEL_BACKLIGHT;

		/**
		    Set Display panel(LCD) Device power level

		    This structure is used for display device control operation code "DISPDEV_SET_PANEL_POWER"
		    to set the Display panel(LCD) Device backlight level.
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_PANEL_POWER.ui_pwr_lvl    = (project defined levels);
		\n  pDispObj->dev_ctrl(DISPDEV_SET_PANEL_POWER, &DispDev);
		*/
		struct { // DISPDEV_SET_PANEL_POWER
			UINT32      ui_pwr_lvl;
		} SET_PANEL_POWER;


		/**
		    Set PowerDown

		    This structure is used for display device control operation code "DISPDEV_SET_POWERDOWN"
		    to set the Device powerdown or not.
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_POWERDOWN.b_power_down    = (project defined levels);
		\n  pDispObj->dev_ctrl(DISPDEV_SET_POWERDOWN, &DispDev);
		*/
		struct { // DISPDEV_SET_POWERDOWN
			BOOL        b_power_down;
		} SET_POWERDOWN;

		/**
		    Set TV parameter

		    This structure is used for display device control operation code "DISPDEV_SET_TVPAR"
		    to set the TV's parameters.
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_TVPAR.b_en_user  = TRUE or FALSE;
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_bll    = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_brl    = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_setup  = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_y_scaling = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_cb_scaling= (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_cr_scaling= (project defined levels);
		\n  pDispObj->dev_ctrl(DISPDEV_SET_TVPAR, &DispDev);
		*/
		struct { // DISPDEV_SET_TVPAR
			BOOL        b_en_user;               ///< Enable/Disable user's setting value
			UINT32      ui_ntsc_bll;            ///< TV NTSC Blank Level(valid value:0~0xFF)
			UINT32      ui_ntsc_brl;            ///< TV NTSC Burst Level(valid value:0~0xFF)
			UINT32      ui_ntsc_setup;          ///< TV NTSC NTSC setup(valid value:0~0xFF)
			UINT32      ui_ntsc_y_scaling;       ///< TV NTSC Y Scale(valid value:0~0xFF)
			UINT32      ui_ntsc_cb_scaling;      ///< TV NTSC Cb Scale(valid value:0~0xFF)
			UINT32      ui_ntsc_cr_scaling;      ///< TV NTSC Cr Scale(valid value:0~0xFF)
			UINT32      ui_pal_bll;             ///< TV PAL Blank Level(valid value:0~0xFF)
			UINT32      ui_pal_brl;             ///< TV PAL Burst Level(valid value:0~0xFF)
			UINT32      ui_pal_setup;           ///< TV PAL setup(valid value:0~0xFF)
			UINT32      ui_pal_y_scaling;        ///< TV PAL Y Scale(valid value:0~0xFF)
			UINT32      ui_pal_cb_scaling;       ///< TV PAL Cb Scale(valid value:0~0xFF)
			UINT32      ui_pal_cr_scaling;       ///< TV PAL Cr Scale(valid value:0~0xFF)
		} SET_TVPAR;




		/**
		   Set output to dram parameter

		   This structure is used for output ide internal data output to dram via VDO2 channel.
		   So that VDO2 can not active at the same time
		   This structure shuld be used like this:
		\n  DispDev.SEL.SET_TVPAR.b_en_user  = TRUE or FALSE;
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_bll    = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_brl    = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_setup  = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_y_scaling = (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_cb_scaling= (project defined levels);
		\n  DispDev.SEL.SET_TVPAR.ui_ntsc_cr_scaling= (project defined levels);
		\n  pDispObj->dev_ctrl(DISPDEV_SET_TVPAR, &DispDev);
		*/
		struct { // DISPDEV_SET_OUTPUT_DRAM
			UINT32      y_addr;                  ///< Output Y address
			UINT32      uv_addr;                 ///< Output UV address
			UINT32      output_width;            ///< Output width
			UINT32      output_height;           ///< Output height
			UINT32      output_line_ofs;          ///< Output line offset(unit:byte)
			BOOL        is_yuv422;              ///< output format (0:420/1:422)
		} SET_OUTPUT_DRAM_PAR;


		/**
		   Set TV Full screen

		   This structure is used for display device control operation code "DISPDEV_SET_TVFULL"
		   to set the TV's parameters.
		   This structure shuld be used like this:
		\n  DispDev.SEL.SET_TVFULL.b_en_full  = TRUE or FALSE;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_TVFULL, &DispDev);
		*/
		struct { // DISPDEV_SET_TVFULL
			BOOL        b_en_full;               ///< Enable/Disable TV Full screen
		} SET_TVFULL;


		/* GET control group */

		/**
		    Get the current active Display Device ID

		    This structure is used for display device control operation code "DISPDEV_GET_ACT_DEVICE"
		    to get the current active display device of the display object. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_ACT_DEVICE, &DispDev);
		\n  "DispDev.SEL.GET_ACT_DEVICE.dev_id"
		    is the current active display device ID.
		*/
		struct { // DISPDEV_GET_ACT_DEVICE
			DISPDEV_ID  dev_id;              ///< Current Active display device ID.
		} GET_ACT_DEVICE;

		/**
		    Get the current active Display optimal buffer/window size

		    This structure is used for display device control operation code "DISPDEV_GET_DISPSIZE"
		    to get the current active display's optimal buffer/window size. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_DISPSIZE, &DispDev);
		\n  "DispDev.SEL.GET_DISPSIZE.ui_buf_width"/"DispDev.SEL.GET_DISPSIZE.ui_buf_height"/"DispDev.SEL.GET_DISPSIZE.ui_win_width"/"DispDev.SEL.GET_DISPSIZE.ui_win_height"
		    are the display optimal buffer/window size
		*/
		struct { // DISPDEV_GET_DISPSIZE
			UINT32          ui_buf_width;     ///< Current Active display optimal buffer width.
			UINT32          ui_buf_height;    ///< Current Active display optimal buffer height.

			UINT32          ui_win_width;     ///< Current Active display optimal window width.
			UINT32          ui_win_height;    ///< Current Active display optimal window height.
		} GET_DISPSIZE;

		/**
		    Get the pre Display optimal buffer size

		    This structure is used for display device control operation code "DISPDEV_GET_PREDISPSIZE"
		    to get the current active display's optimal buffer size. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_PREDISPSIZE, &DispDev);
		\n  "DispDev.SEL.GET_PREDISPSIZE.ui_buf_width"/"DispDev.SEL.GET_PREDISPSIZE.ui_buf_height"
		    are the display optimal buffer size
		*/
		struct { // DISPDEV_GET_PREDISPSIZE
			DISPDEV_ID      dev_id;          ///< Get specified display device.
			UINT32          ui_buf_width;     ///< Get display optimal buffer width.
			UINT32          ui_buf_height;    ///< Get  display optimal buffer height.
			UINT32          ui_win_width;     ///< Get display optimal window width.
			UINT32          ui_win_height;    ///< Get  display optimal window height.
		} GET_PREDISPSIZE;


		/**
		    Get the user defined panel adjustment callback

		    This structure is used for display device control operation code "DISPDEV_GET_PANEL_ADJUST"
		    to get the user defined panel adjustment callback. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_PANEL_ADJUST, &DispDev);
		\n  "DispDev.SEL.GET_PANEL_ADJUST.pfp_adjust"
		    is the user defined panel adjustment callback.
		*/
		struct { // DISPDEV_GET_PANEL_ADJUST
			FP              pfp_adjust;      ///< Get the panel adjustment callback function
		} GET_PANEL_ADJUST;

		/**
		    Get the user assigned TV adjustment resistor value

		    This structure is used for display device control operation code "DISPDEV_GET_TVADJUST"
		    to get the user assigned TV adjustment resistor value. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_TVADJUST, &DispDev);
		\n  "DispDev.SEL.GET_TVADJUST.tv_adjust"
		    is the user assigned TV adjustment resistor value.
		*/
		struct { // DISPDEV_GET_TVADJUST
			DISPDEV_TVADJUST    tv_adjust;   ///< Get the user assigned TV adjustment resistor value
		} GET_TVADJUST;

		/**
		    Get the user assigned LCD Control interface configurations

		    This structure is used for display device control operation code "DISPDEV_GET_REG_IF"
		    to get the user assigned LCD Control interface configurations. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_TVADJUST, &DispDev);
		\n  "DispDev.SEL.GET_REG_IF.lcd_ctrl"/"DispDev.SEL.GET_REG_IF.ui_sif_ch"/"DispDev.SEL.GET_REG_IF.ui_gpio_sen"/ ..
		    are the user assigned LCD Control interface configurations.
		*/
		struct { // DISPDEV_GET_REG_IF
			DISPDEV_LCDCTRL lcd_ctrl;        ///< Get LCD Control Interface select

			SIF_CH          ui_sif_ch;        ///< If interface is SIF, this field is the assigned SIF CH number.
			UINT32          ui_gpio_sen;      ///< If interface is GPIO, this field is the assigned GPIO Pin ID for SIF SEN.
			UINT32          ui_gpio_clk;      ///< If interface is GPIO, this field is the assigned GPIO Pin ID for SIF CLK.
			UINT32          ui_gpio_data;     ///< If interface is GPIO, this field is the assigned GPIO Pin ID for SIF DATA.
		} GET_REG_IF;

		/**
		    Read Display Device register

		    This structure is used for display device control operation code "DISPDEV_REG_READ"
		    to read display device register. This structure shuld be used like this:
		\n  DispDev.SEL.REG_READ.ui_addr    = 0x00;
		\n  pDispObj->dev_ctrl(DISPDEV_REG_READ, &DispDev);
		\n  "DispDev.SEL.REG_READ.ui_return" is the read back register value.
		*/
		struct { // DISPDEV_REG_READ
			UINT32          ui_addr;         ///< The panel register offset.
			UINT32          ui_return;       ///< The readback panel register value.
		} REG_READ;

		/**
		    Get HDMI Display VIDEO/AUDIO mode

		    This structure is used for display device control operation code "DISPDEV_GET_HDMIMODE"
		    to get the HDMI VIDEO/AUDIO mode. This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_HDMIMODE, &DispDev);
		\n  "DispDev.SEL.GET_HDMIMODE.video_id"/"DispDev.SEL.GET_HDMIMODE.audio_id"
		    are the HDMI VIDEO/AUDIO mode selection.
		*/
		struct { // DISPDEV_GET_HDMIMODE
			HDMI_VIDEOID    video_id;        ///< HDMI VIDEO ID select
			HDMI_AUDIOFMT   audio_id;        ///< HDMI AUDIO ID select
		} GET_HDMIMODE;

		/**
		    Get LCD mode

		    This structure is used for display device control operation code "DISPDEV_SET_LCDMODE"
		    to get  the LCD mode.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_LCDMODE, &DispDev);
		\n  "DispDev.SEL.GET_LCDMODE.PinMuxMod" is the LCD pinmux mode selection
		*/
		struct { // DISPDEV_SET_LCDMODE
			DISPDEV_LCDMODE  mode;      ///< select LCD mode
		} GET_LCDMODE;


		/**
		    Get Display panel(LCD) Device backlight level

		    This structure is used for display device control operation code "DISPDEV_GET_PANEL_BACKLIGHT"
		    to get the Display panel(LCD) Device backlight level.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_PANEL_BACKLIGHT, &DispDev);
		\n  "DispDev.SEL.GET_PANEL_BACKLIGHT.ui_bl_lvl" is the project defined backlight level
		*/
		struct { // DISPDEV_GET_PANEL_BACKLIGHT
			UINT32      ui_bl_lvl;
		} GET_PANEL_BACKLIGHT;

		/**
		    Get Display panel(LCD) Device POWER level

		    This structure is used for display device control operation code "DISPDEV_GET_PANEL_POWER"
		    to get the Display panel(LCD) Device POWER level.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_PANEL_POWER, &DispDev);
		\n  "DispDev.SEL.GET_PANEL_POWER.ui_pwr_lvl" is the project defined POWER level
		*/
		struct { // DISPDEV_GET_PANEL_POWER
			UINT32      ui_pwr_lvl;
		} GET_PANEL_POWER;

		/**
		    Get PowerDown

		    This structure is used for display device control operation code "DISPDEV_GET_POWERDOWN"
		    to get the Device powerdown or not.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_POWERDOWN, &DispDev);
		\n  "DispDev.SEL.GET_POWERDOWN.b_power_down" is the project defined POWER level
		*/
		struct { // DISPDEV_GET_ POWERDOWN
			BOOL        b_power_down;
		} GET_POWERDOWN;

		/**
		    Get TV parameter

		    This structure is used for display device control operation code "DISPDEV_GET_TVPAR"
		    to get the TV's parameters.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_GET_TVPAR, &DispDev);
		\n  "DispDev.SEL.GET_TVPAR.b_en_user" is Enable/Disable user define value
		\n  "DispDev.SEL.GET_TVPAR.ui_bll" is the TV's blank level
		\n  "DispDev.SEL.GET_TVPAR.ui_brl" is the TV's burst level
		\n  "DispDev.SEL.GET_TVPAR.ui_setup" is the TV's setup level
		\n  "DispDev.SEL.GET_TVPAR.uiYScale" is the TV's y scale level
		\n  "DispDev.SEL.GET_TVPAR.uiCbScale" is the TV's cb scale level
		\n  "DispDev.SEL.GET_TVPAR.uiCrScale" is the TV's cr scale level
		*/
		struct { // DISPDEV_GET_TVPAR
			BOOL        b_en_user;               ///< Enable/Disable user's setting value
			UINT32      ui_ntsc_bll;            ///< TV NTSC Blank Level(valid value:0~0xFF)
			UINT32      ui_ntsc_brl;            ///< TV NTSC Burst Level(valid value:0~0xFF)
			UINT32      ui_ntsc_setup;          ///< TV NTSC setup(valid value:0~0xFF)
			UINT32      ui_ntsc_y_scaling;       ///< TV NTSC Y Scale(valid value:0~0xFF)
			UINT32      ui_ntsc_cb_scaling;      ///< TV NTSC Cb Scale(valid value:0~0xFF)
			UINT32      ui_ntsc_cr_scaling;      ///< TV NTSC Cr Scale(valid value:0~0xFF)
			UINT32      ui_pal_bll;             ///< TV PAL Blank Level(valid value:0~0xFF)
			UINT32      ui_pal_brl;             ///< TV PAL Burst Level(valid value:0~0xFF)
			UINT32      ui_pal_setup;           ///< TV PAL setup(valid value:0~0xFF)
			UINT32      ui_pal_y_scaling;        ///< TV PAL Y Scale(valid value:0~0xFF)
			UINT32      ui_pal_cb_scaling;       ///< TV PAL Cb Scale(valid value:0~0xFF)
			UINT32      ui_pal_cr_scaling;       ///< TV PAL Cr Scale(valid value:0~0xFF)
		} GET_TVPAR;

	} SEL;
} DISPDEV_PARAM, *PDISPDEV_PARAM;
//@}

typedef ER(*DISPDEVCTRL)(DISPDEV_OP, PDISPDEV_PARAM);




//
//  Exported Get Display Device Object API
//
//extern PDISPDEV_OBJ dispdev_get_tv_dev_obj(void);
//extern PDISPDEV_OBJ dispdev_get_hdmi_dev_obj(void);
extern PDISPDEV_OBJ dispdev_get_lcd1_dev_obj(void);
//extern PDISPDEV_OBJ dispdev_get_lcd2_dev_obj(void);


#endif
//@}
