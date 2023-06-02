/*
    panel device open/close Common control header

    @file       dispdev_panel.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _DISPDEV_PANLCOMN_H
#define _DISPDEV_PANLCOMN_H

#include "dispdev_platform.h"

#define DISPANL_DEBUG  DISABLE
#if DISPANL_DEBUG
#define dispanl_debug(msg)       DBG_DUMP msg
#else
#define dispanl_debug(msg)
#endif



/*
    Structure for standard format of panel Register Access
*/
typedef struct {
	UINT32  ui_address;                      // panel Register Address
	UINT32  ui_value;                        // panel Register config value
} T_PANEL_CMD;


/*
    Structure for standard format of panel open parameters
*/
typedef struct {
	/* Old prototype */

	PINMUX_LCDINIT  lcd_mode;                // LCD mode select
	UINT32          fd_clk;                  // LCD Target Clock Rate(Hz)

	UINT32          ui_hsync_total_period;     // Horizontal Total Period
	UINT32          ui_hsync_active_period;    // Horizontal Active Width
	UINT32          ui_hsync_back_porch;       // Horizontal Backporth

	UINT32          ui_vsync_total_period;     // Vertical Total Period
	UINT32          ui_vsync_active_period;    // Vertical Active Width
	UINT32          ui_vsync_back_porch_odd;    // Vertical Odd Lines Backporch
	UINT32          ui_vsync_back_porch_even;   // Vertical Even Lines Backporch

	UINT32          ui_buffer_width;          // Buffer Width returned for upper layer
	UINT32          ui_buffer_height;         // Buffer Height returned for upper layer
	UINT32          ui_window_width;          // Window Width returned for upper layer
	UINT32          ui_window_height;         // Window Height returned for upper layer
	BOOL            b_ycbcr_format;           // TRUE(YUV): YUV/UPS051/UPS052,  FALSE(RGB): RGB/RGBD

	/* New added parameters */
	UINT32          ui_hsync_sync_width;       // Horizontal Sync pulse Width
	UINT32          ui_vsync_sync_width;       // Vertical Sync pulse Width

} T_PANEL_PARAM;



/*
    Structure for standard format of ide paramrters
*/
typedef struct {
	/* Old prototype */

	PINMUX_LCD_SEL  pinmux_select_lcd;  // LCD Pinmux select
	ICST_TYPE       icst;               // ICST Output type
	BOOL            dithering[2];       // [0]: Dither Enable.   [1]: Dither FreeRun
	IDE_DEVICE_TYPE device;             // **DONT-CARE** Useless parameter in current version
	IDE_PDIR        pdir;               // RGB Pixel Output Direction
	IDE_PORDER      odd;                // LCD Odd Lines pixel Start from R or G or B.
	IDE_PORDER      even;               // LCD Odd Lines pixel Start from R or G or B.
	BOOL            hsinv;              // H-SYNC Invert
	BOOL            vsinv;              // V-SYNC Invert
	BOOL            hvldinv;            // H-Valid Invert
	BOOL            vvldinv;            // V-Valid Invert
	BOOL            clkinv;             // PIXEL CLOCK Invert
	BOOL            fieldinv;           // FIELD Invert
	BOOL            rgbdummy;           // **DONT-CARE** Useless parameter in current version
	BOOL            interlace;          // Output interlaced format
	BOOL            startfield;         // **DONT-CARE** Useless parameter in current version
	UINT32          ctrst;              // Contrast value
	UINT32          brt;                // Brightness value
	UINT32          cmults;             // Saturation value
	BOOL            cex;                // Cb/Cr Exchane
	BOOL            standby_signal;     // **DONT-CARE** Useless parameter in current version
	BOOL            clamp;              // clamp
	BOOL            tv_powerdown;       // Assert TV power down in panel open
	UINT32          sif_startend[2];    // **DONT-CARE** Useless parameter in current version


	/* New added parameters */
	BOOL             yc_ex;               // Y/C exchange
	BOOL             hlpf;               // Horizontal lowpass filter
	BOOL             subpix_odd[3];       // Sub-Pixel ON/OFF for Odd lines. [0]:R   [1]:G   [2]:B
	BOOL             subpix_even[3];      // Sub-Pixel ON/OFF for Even lines.[0]:R   [1]:G   [2]:B
	IDE_DITHER_VBITS dither_bits[3];      // [0]:R dither bits. [1]:G dither bits. [2]:B dither bits. This is valid only if dithering[0]=TRUE.
	BOOL             clk1_2;             // clock 1/2
} T_IDE_PARAM;



typedef struct {
	T_PANEL_PARAM panel;         // panel Related Paramters
	T_IDE_PARAM   ide;           // ide Related Paramters
	T_PANEL_CMD  *p_cmd_queue;     // panel Register Command Queue
	UINT32       n_cmd;          // panel Register Command Queue Number
} T_LCD_PARAM;



typedef enum {
	T_LCD_INF_SERIAL_8BITS,      // Such as RGB-Serial/RGB-Dummy/RGB-Through/YUV
	T_LCD_INF_PARALLEL_16BITS,   // Such as Parallel RGB565 / YUV
	T_LCD_INF_MI,                // Interface is Memory interface
	T_LCD_INF_MIPI_DSI,

	ENUM_DUMMY4WORD(T_LCD_INF)
} T_LCD_INF;



typedef struct {
	DISPDEV_LCD_ROTATE  rotation;            // LCD Roate ID
	T_PANEL_CMD         *p_cmd_queue;      // LCD Roate ID Command Queue
	UINT32              n_cmd;           // LCD Roate ID Command Queue Number
} T_LCD_ROT;

typedef struct {
	UINT32                  ui_sif_mode;          ///< Bus mode. Please refer to Application Note for more detailed information.
	///< @note  Valid value: SIF_MODE_00 ~ SIF_MODE_15
	UINT32                  ui_bus_clock;         ///< Bus clock. Must be configured before SIF_CONFIG_ID_SEND, SIF_CONFIG_ID_SENS,
	///< SIF_CONFIG_ID_SENH and SIF_CONFIG_ID_DELAY.
	///< @note  Valid value: 187,500 ~ 48,000,000 , unit: Hz.
	///<        Real bus clock might lower than your setting.
	///<        Please call sif_getConfig(SIF_CONFIG_ID_BUSCLOCK) to confirm.
	///<        Formula: Bus Clock = 96MHz / (2 * (CLKDIV + 1))
	UINT32                  ui_sens;             ///< SEN setup time for SIF_MODE_00 ~ SIF_MODE_07. Must be configured after SIF_CONFIG_ID_BUSCLOCK.
	///< Please refer to Application Note for more detailed information.
	///< @note  Valid value: (0 ~ 15) * half bus clock, unit: ns. Default: 0 ns.
	///<        Real SEN setup time might greater than your setting.
	///<        Please call sif_getConfig(SIF_CONFIG_ID_SENS) to confirm.
	UINT32                  ui_senh;             ///< SEN hold time for all modes. Must be configured after SIF_CONFIG_ID_BUSCLOCK.
	///< Please refer to Application Note for more detailed information.
	///< @note  Valid value: (0 ~ 7) * half bus clcok, unit: ns. Default: 0 ns.
	///<        Real SEN hold time might greater than your setting.
	///<        Please call sif_getConfig(SIF_CONFIG_ID_SENH) to confirm.
	UINT32                  ui_transfer_length;   ///< How many bits will be transfered per transmission.
	///< @note  Valid value: 1 ~ 255
	SIF_DIR                 ui_sif_dir;           ///< Shift out direction
	///< - @b SIF_DIR_LSB   : LSb is shifted out first (Defalut value)
	///< - @b SIF_DIR_MSB   : MSb is shifted out first
} T_LCD_SIF_PARAM;




extern ER dispdev_set_display(DISPDEV_IOCTRL p_disp_dev_control, T_LCD_INF lcd_inf, T_LCD_PARAM *p_mode, PINMUX_FUNC_ID pin_func_id);
#if defined __FREERTOS
extern int panel_init(void);
extern void panel_exit(void);
#endif
#endif
