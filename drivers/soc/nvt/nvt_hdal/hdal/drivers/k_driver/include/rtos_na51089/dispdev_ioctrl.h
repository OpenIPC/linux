/**
    @file       dispdev_ioctrl.h
    @ingroup    mISYSDisp

    @brief      Header file for Display device interface between display object
				and the display device.
				This file is the header file that define the API and data type
				for Display control object
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

/** \addtogroup  mISYSDisp*/
//@{

#ifndef _DISPDEV_IOCTRL_H
#define _DISPDEV_IOCTRL_H

#include "display.h"


/**
    Display device control interface between display object and the display device
*/
typedef enum {
	/* SET control group */
	DISPDEV_IOCTRL_SET_ENABLE = DISPCMDBASE_IOCTRL, ///< Set display engine's global enable/disable.
	DISPDEV_IOCTRL_SET_DEVICE,                    ///< select the display device for display engine.
	DISPDEV_IOCTRL_SET_SRGB_OUTORDER,             ///< Configure display RGB-Serial-pixels output order.
	DISPDEV_IOCTRL_SET_SYNC_INVERT,               ///< Configure Output interface singals'inversion: HSYNC / VSYNC / PIXCLK.
	DISPDEV_IOCTRL_SET_VLD_INVERT,                ///< Configure Output interface singals'inversion: HVLD  / VVLD  / FIELD / DE.
	DISPDEV_IOCTRL_SET_SUBPIXEL,                  ///< Set display subpixel function configurations.
	DISPDEV_IOCTRL_SET_WINDOW_H_TIMING,           ///< Set display global output window horizontal timings.
	DISPDEV_IOCTRL_SET_WINDOW_V_TIMING,           ///< Set display global output window vertical timings.
	DISPDEV_IOCTRL_SET_WINDOW_OUT_TYPE,           ///< Set display global output window interlaced/progressive, start-field select
	DISPDEV_IOCTRL_SET_SYNCDELAY,                 ///< Set display HSYNC / VSYNC delay clock cycles.
	DISPDEV_IOCTRL_SET_CCIR656_SYNCCODE,          ///< Set display digital video data code timing for CCIR656
	DISPDEV_IOCTRL_SET_GAMMA_EN,                  ///< Set display Gamma function enable/disable. Also config the Gamma coefficients if function enabled.
	DISPDEV_IOCTRL_SET_CSB_EN,                    ///< Set display Contrast / Saturation / Brightness performances and also the function enable/disable.
	DISPDEV_IOCTRL_SET_YC_EXCHG,                  ///< Set display Y/C exchange and Cb/Cr exchange.
	DISPDEV_IOCTRL_SET_CLAMP,                     ///< Set display Y/C Clamp
	DISPDEV_IOCTRL_SET_DITHER_EN,                 ///< Set display Dithering functin enable/disable and the dither bits selection.
	DISPDEV_IOCTRL_SET_OUT_COMPONENT,             ///< Set display ouput component swap if RGB/YUV parallel is selected.
	DISPDEV_IOCTRL_SET_ICST_EN,                   ///< Set display Inverse-color-Space-Transform function enable/disable.
	DISPDEV_IOCTRL_SET_HLPF_EN,                   ///< Set display horizontal low pass filter enable/disable.
	DISPDEV_IOCTRL_SET_OUT_LIMIT,                 ///< Set display output limitation values.
	DISPDEV_IOCTRL_SET_CONST_OFS,                 ///< Set display window constant offset X/Y. This feature is used for handling the overscan/underscan hdmi tv.
	DISPDEV_IOCTRL_SET_DISPSIZE,                  ///< Set current active display standard buffer/window size.
	DISPDEV_IOCTRL_SET_CLK1_2,                    ///< Set display clock1/2
	DISPDEV_IOCTRL_SET_RGBD_SWAP,                 ///< Set dummy swap

	/* GET control group */
	DISPDEV_IOCTRL_GET_ENABLE,                    ///< Get display engine's global enable/disable.
	DISPDEV_IOCTRL_GET_DEVICE,                    ///< Get display device for display engine.
	DISPDEV_IOCTRL_GET_SRCCLK,                    ///< Get display operating source clock
	DISPDEV_IOCTRL_GET_REG_IF,                    ///< Get display Device(panel) Register Access Interface.
	DISPDEV_IOCTRL_GET_HDMIMODE,                  ///< Get the HDMI Audio/Video mode
	DISPDEV_IOCTRL_GET_ACT_DEVICE,                ///< Get current active device. (panel/TV/HDMI)
	DISPDEV_IOCTRL_GET_PANEL_ADJUST,              ///< Get the panel adjustment callback.
	DISPDEV_IOCTRL_GET_TVADJUST,                  ///< Get the current TV adjustment configurations.
	DISPDEV_IOCTRL_GET_GAMMA_EN,                  ///< Get display Gamma function enable/disable
	DISPDEV_IOCTRL_GET_CSB_EN,                    ///< Get display Contrast / Saturation / Brightness performances and also the function enable/disable.
	DISPDEV_IOCTRL_GET_TVPAR,                     ///< Get TV paramter of user definition.
	DISPDEV_IOCTRL_GET_TVFULL,                    ///< Get TV full screen.

	/* Others */
	DISPDEV_IOCTRL_SET_LOAD,                      ///< Set Display Configuration Activation
	DISPDEV_IOCTRL_WAIT_FRAMEEND,                 ///< Wait Display Frame End
	DISPDEV_IOCTRL_WAIT_DMA_DONE,                 ///< Wait YUV output Dram done


	DISPDEV_IOCTRL_SET_CLK_FREQ,                  ///< Set IDEx clock frequency
	DISPDEV_IOCTRL_SET_CLK_EN,                    ///< Enable IDEx clock

	ENUM_DUMMY4WORD(DISPDEV_IOCTRL_OP)
} DISPDEV_IOCTRL_OP;




/**
    Display Device Parameter
*/
typedef struct {
	union {
		/* SET control group */

		/**
		    Set Display Engine Enable/Disable

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_ENABLE"
		    to set the display enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ENABLE.b_en     = TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_ENABLE, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_ENABLE
			BOOL            b_en;            ///< Display Enable/Disable
		} SET_ENABLE;

		/**
		    Set Display Engine outout format type

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_DEVICE"
		    to set the display output format type. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_DEVICE.disp_dev_type     = DISPDEV_IOCTRL_TYPE_RGB_SERIAL;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_DEVICE, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_DEVICE
			DISPDEV_TYPE    disp_dev_type;    ///< select display device type
		} SET_DEVICE;

		/**
		    Set Display Engine PIXEL output order for Serial-RGB/RGBD/RGB-Through

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_SRGB_OUTORDER"
		    to set the display pixels output order for Serial-RGB/RGBD/RGB-Through. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_SRGB_OUTORDER.pix_order     = IDE_PDIR_RGB;
		\n  disp_ctrl.SEL.SET_SRGB_OUTORDER.odd_start     = IDE_LCD_R;
		\n  disp_ctrl.SEL.SET_SRGB_OUTORDER.even_start    = IDE_LCD_G;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_SRGB_OUTORDER, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_SRGB_OUTORDER
			IDE_PDIR        pix_order;       ///< RGB-Serial-pixels output order is RGB or RBG.
			IDE_PORDER      odd_start;       ///< Odd lines RGB-Serial-pixels start output from R or G or B.
			IDE_PORDER      even_start;      ///< Even lines RGB-Serial-pixels start output from R or G or B.
		} SET_SRGB_OUTORDER;

		/**
		    Set Display Engine HSYNC/VSYNC/PIXCLK output invert

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_SYNC_INVERT"
		    to set the display signals HSYNC/VSYNC/PIXCLK output invert. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_SYNC_INVERT.b_hs_inv     = TRUE;
		\n  disp_ctrl.SEL.SET_SYNC_INVERT.b_vs_inv     = TRUE;
		\n  disp_ctrl.SEL.SET_SYNC_INVERT.b_clk_inv    = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_SYNC_INVERT, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_SYNC_INVERT
			BOOL            b_hs_inv;         ///< FALSE: Active High, TRUE: Active Low. Active low means that the HSYNC changes at clk negative edge.
			BOOL            b_vs_inv;         ///< FALSE: Active High, TRUE: Active Low. Active low means that the HSYNC changes at clk negative edge.
			BOOL            b_clk_inv;        ///< FALSE: Not invert,  TRUE: Invert.
		} SET_SYNC_INVERT;

		/**
		    Set Display Engine HSYNC/VSYNC/PIXCLK output invert

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_SYNC_INVERT"
		    to set the display signals HSYNC/VSYNC/PIXCLK output invert. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_SYNC_INVERT.b_hs_inv     = TRUE;
		\n  disp_ctrl.SEL.SET_SYNC_INVERT.b_vs_inv     = TRUE;
		\n  disp_ctrl.SEL.SET_SYNC_INVERT.b_clk_inv    = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_SYNC_INVERT, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_VLD_INVERT
			BOOL            b_hvld_inv;       ///< FALSE: Active High, TRUE: Active Low. Active low means that the HSYNC changes at clk negative edge.
			BOOL            b_vvld_inv;       ///< FALSE: Active High, TRUE: Active Low. Active low means that the HSYNC changes at clk negative edge.
			BOOL            b_field_inv;      ///< FALSE: Not invert,  TRUE: Invert.
			BOOL            b_de_inv;         ///< FALSE: Not invert,  TRUE: Invert.
		} SET_VLD_INVERT;

		/**
		    Set Display Engine SubPixel Funtion

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_SUBPIXEL"
		    to set the display subpixel funtion enable/disable for Odd/Even lines. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_SUBPIXEL.b_odd_r     = TRUE;
		\n  disp_ctrl.SEL.SET_SUBPIXEL.b_odd_g     = TRUE;
		\n  disp_ctrl.SEL.SET_SUBPIXEL.b_odd_b     = TRUE;
		\n  disp_ctrl.SEL.SET_SUBPIXEL.b_even_r    = FALSE;
		\n  disp_ctrl.SEL.SET_SUBPIXEL.b_even_g    = FALSE;
		\n  disp_ctrl.SEL.SET_SUBPIXEL.b_even_b    = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_SUBPIXEL, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_SUBPIXEL
			BOOL            b_odd_r;          ///< Enable low pass filter for R compoent for Odd lines.
			BOOL            b_odd_g;          ///< Enable low pass filter for G compoent for Odd lines.
			BOOL            b_odd_b;          ///< Enable low pass filter for B compoent for Odd lines.

			BOOL            b_even_r;         ///< Enable low pass filter for R compoent for Even lines.
			BOOL            b_even_g;         ///< Enable low pass filter for G compoent for Even lines.
			BOOL            b_even_b;         ///< Enable low pass filter for B compoent for Even lines.
		} SET_SUBPIXEL;

		/**
		    Set Display Engine GLOBAL output window Horizontal timing

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_WINDOW_H_TIMING"
		    to set the display global output Horizontal window. This controls the horizontal timing generator in bus interface.
		    This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hsync     = 1;
		\n  disp_ctrl.SEL.SET_WINDOW_H_TIMING.ui_htotal    = 1560;
		\n  disp_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_start = 32;
		\n  disp_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_end   = 1311;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_WINDOW_H_TIMING, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_WINDOW_H_TIMING
			UINT32          ui_hsync;        ///< HSYNC active pulse width in pixel clock cycles.
			UINT32          ui_htotal;       ///< The Horizontal total width would be (ui_htotal+1) pixclk cycles, including blanking pixels.
			UINT32          ui_hvld_start;    ///< Horizontal valid start position.
			UINT32          ui_hvld_end;      ///< Horizontal valid end position.
		} SET_WINDOW_H_TIMING;

		/**
		    Set Display Engine GLOBAL output window Vertical timing

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_WINDOW_V_TIMING"
		    to set the display global output Vertical window. This controls the Vertical timing generator in bus interface.
		    This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vsync        = 1;
		\n  disp_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vtotal       = 524;
		\n  disp_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_odd_start = 16;
		\n  disp_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_odd_end   = 495;
		\n  disp_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_even_start= 17;
		\n  disp_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_even_end  = 496;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_WINDOW_V_TIMING, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_WINDOW_V_TIMING
			UINT32          ui_vsync;            ///< VSYNC active pulse width in pixel clock cycles or lines.
			UINT32          ui_vtotal;           ///< The vertical total lines would be (ui_vtotal+1) lines, including blanking lines.
			UINT32          ui_vvld_odd_start;     ///< Vertical Odd field start lines.
			UINT32          ui_vvld_odd_end;       ///< Vertical Odd field end lines.
			UINT32          ui_vvld_even_start;    ///< Vertical Even field start lines.
			UINT32          ui_vvld_even_end;      ///< Vertical Even field end lines.
		} SET_WINDOW_V_TIMING;

		/**
		    Set Display Engine Global output window type

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_WINDOW_OUT_TYPE"
		    to set the display Global output window type such as Interlaced/progressive. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_WINDOW_OUT_TYPE.b_interlaced     = TRUE;
		\n  disp_ctrl.SEL.SET_WINDOW_OUT_TYPE.b_field_odd_st     = TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_WINDOW_OUT_TYPE, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_WINDOW_OUT_TYPE
			BOOL            b_interlaced;        ///< TRUE: Interlaced format.   FALSE: Progressive format.
			BOOL            b_field_odd_st;        ///< TRUE: First field output is ODD.  FALSE: First field output is EVEN.
		} SET_WINDOW_OUT_TYPE;

		/**
		    Set Display Engine Global output window SYNC signal delay

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_SYNCDELAY"
		    to set the display Global output window SYNC signal delay. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_SYNCDELAY.ui_hsync_dly     = 0x0;
		\n  disp_ctrl.SEL.SET_SYNCDELAY.ui_hsync_dly     = 0x0;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_SYNCDELAY, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_SYNCDELAY
			UINT32          ui_hsync_dly;     ///< Add delay count to HSYNC signal. Unit in PIXCLK cycles.
			UINT32          ui_vsync_dly;     ///< Add delay count to VSYNC signal. Unit in PIXCLK cycles.
		} SET_SYNCDELAY;

		/**
		    Set Display Engine CCIR656 SYNC Code position

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CCIR656_SYNCCODE"
		    to set the display CCIR656 SYNC Code position. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = 0x20C;
		\n  disp_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = 0x16;
		\n  disp_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = 0x106;
		\n  disp_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = 0x11C;
		\n  disp_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_start  = 0x04;
		\n  disp_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_end    = 0x10A;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CCIR656_SYNCCODE, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_CCIR656_SYNCCODE
			UINT32          ui_odd_start;         ///< CCIR656 Digital SYNC code odd start position
			UINT32          ui_odd_end;           ///< CCIR656 Digital SYNC code odd end position
			UINT32          ui_even_start;        ///< CCIR656 Digital SYNC code Even start position
			UINT32          ui_even_end;          ///< CCIR656 Digital SYNC code Even end position
			UINT32          ui_field_start;       ///< CCIR656 Digital SYNC code FIELD start position
			UINT32          ui_field_end;         ///< CCIR656 Digital SYNC code FIELD end position
		} SET_CCIR656_SYNCCODE;

		/**
		    Set Display Engine Gamma Function

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_GAMMA_EN"
		    to set the display gamma function enable/disable and also the gamma table. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_GAMMA_EN.b_en           = TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_GAMMA_EN, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_GAMMA_EN
			BOOL            b_en;           ///< Enable/Disable the Gamma function for component Y
			//UINT8           *p_gamma_tab_y;    ///< The 17 UINT8 entries gamma table for component Y. This field is useless if bEnY=FALSE.
		} SET_GAMMA_EN;

		/**
		    Set Display Engine Contrast/Saturation/Brightness configurations

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CSB_EN"
		    to set the display Contrast/Saturation/Brightness configurations. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_GAMMA_EN.b_en          = TRUE;
		\n  disp_ctrl.SEL.SET_GAMMA_EN.ui_contrast   = 0x40;
		\n  disp_ctrl.SEL.SET_GAMMA_EN.ui_saturation = 0x40;
		\n  disp_ctrl.SEL.SET_GAMMA_EN.ui_brightness = 0x00;
		\n  dispDevCtrl(DDISPDEV_IOCTRL_SET_CSB_EN, &disp_ctrl);
		\n  Or Using like this:
		\n  disp_ctrl.SEL.SET_GAMMA_EN.b_en          = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CSB_EN, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_CSB_EN
			BOOL            b_en;            ///< Set Contrast/Saturation/Brightness adjustment function enable/disable

			UINT32          ui_contrast;     ///< Contrast value, valid range from 0x0 to 0xFF. default value is 0x40.
			UINT32          ui_saturation;   ///< Saturation value, valid range from 0x0 to 0x1FF. default value is 0x40.
			UINT32          ui_brightness;   ///< The brightness values, Valid range between -64 to 63. 2's complement value.
		} SET_CSB_EN;

		/**
		    Set Display Engine YUV output Format Y/Cb/Cr component exchange

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_YC_EXCHG"
		    to set the display YUV output Format Y/Cb/Cr component exchange. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_YC_EXCHG.b_cbcr_exchg    = FALSE;
		\n  disp_ctrl.SEL.SET_YC_EXCHG.b_yc_exchg      = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_YC_EXCHG, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_YC_EXCHG
			BOOL            b_cbcr_exchg;     ///< FALSE: CB first.  TRUE: CR first. This field is also valid for YUV parallel 16bits out.
			BOOL            b_yc_exchg;       ///< FALSE: C first.   TRUE: Y first. This field is also valid for YUV parallel 16bits out.
		} SET_YC_EXCHG;

		/**
		    Set Display Engine YUV output Format Y/C Clamp

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CLAMP"
		    to set the display YUV output Format Y/Cb/Cr component exchange. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CLAMP.ui_clamp    =  IDE_YCCCLAMP_NOCLAMP;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CLAMP, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_CLAMP
			IDE_YCCCLAMP    ui_clamp;     ///< Clamp value
		} SET_CLAMP;


		/**
		    Set Display Engine Dithering Function

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_DITHER_EN"
		    to set the display Dithering Function. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_DITHER_EN.b_en          = TRUE;
		\n  disp_ctrl.SEL.SET_DITHER_EN.b_free_run     = FALSE;
		\n  disp_ctrl.SEL.SET_DITHER_EN.r_bits        = IDE_DITHER_5BITS;
		\n  disp_ctrl.SEL.SET_DITHER_EN.g_bits        = IDE_DITHER_6BITS;
		\n  disp_ctrl.SEL.SET_DITHER_EN.b_bits        = IDE_DITHER_5BITS;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_DITHER_EN, &disp_ctrl);
		\n  Or Using like this:
		\n  disp_ctrl.SEL.SET_DITHER_EN.b_en          = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_DITHER_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_DITHER_EN
			BOOL            b_en;            ///< select dithering function enable/disable. This function is designed for RGB color-domain.
			BOOL            b_free_run;       ///< select dithering at freeRun mode enable/disable. If b_en=FALSE, this setting can be ignored.

			IDE_DITHER_VBITS r_bits;         ///< select dithering bits for R color. If b_en=FALSE, this setting can be ignored.
			IDE_DITHER_VBITS g_bits;         ///< select dithering bits for G color. If b_en=FALSE, this setting can be ignored.
			IDE_DITHER_VBITS b_bits;         ///< select dithering bits for B color. If b_en=FALSE, this setting can be ignored.
		} SET_DITHER_EN;

		/**
		    Set Display Engine Output R/G/B (Y/Cb/Cr) Component Swap and bit Order Swap

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_OUT_COMPONENT"
		    to set the display TG Output R/G/B (Y/Cb/Cr) Component Swap and bit Order Swap. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_OUT_COMPONENT.comp0     = IDE_COMPONENT_R;
		\n  disp_ctrl.SEL.SET_OUT_COMPONENT.comp1     = IDE_COMPONENT_G;
		\n  disp_ctrl.SEL.SET_OUT_COMPONENT.comp2     = IDE_COMPONENT_B;
		\n  disp_ctrl.SEL.SET_OUT_COMPONENT.b_bit_swap  = FALSE;
		\n  disp_ctrl.SEL.SET_OUT_COMPONENT.b_length   = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_OUT_COMPONENT, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_OUT_COMPONENT
			IDE_OUT_COMP    comp0;          ///< select RGB/YUV parallel output component-0 as R/G/B or Y/U/V.
			IDE_OUT_COMP    comp1;          ///< select RGB/YUV parallel output component-1 as R/G/B or Y/U/V.
			IDE_OUT_COMP    comp2;          ///< select RGB/YUV parallel output component-2 as R/G/B or Y/U/V.

			BOOL            b_bit_swap;       ///< select RGB/YUV parallel output bit order swap enable/disable.
			BOOL            b_length;        ///< select RGB/YUV parallel output bit length. FALSE is 8bitsfor each component. TRUE is 6bits for each component.
		} SET_OUT_COMPONENT;

		/**
		    Set Display Engine Inverse color Space Transform

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_ICST_EN"
		    to set the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ICST_EN.b_en        = TRUE;
		\n  disp_ctrl.SEL.SET_ICST_EN.select     = CST_YCBCR2RGB;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_ICST_EN, &disp_ctrl);
		\n  Or Using like this:
		\n  disp_ctrl.SEL.SET_ICST_EN.b_en        = FALSE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_ICST_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_ICST_EN
			BOOL            b_en;            ///< The Inverse color space transform function enable/disable.
			CST_SEL         select;         ///< The color space transform of RGB2YCBCR or YCBCR2RGB.
		} SET_ICST_EN;

		/**
		    Set Display Engine Horizontal Low Pass Filter Enable/Disable

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_HLPF_EN"
		    to set the display Horizontal Low Pass Filter Enable/Disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_HLPF_EN.b_en     = TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_HLPF_EN, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_HLPF_EN
			BOOL            b_en;            ///< TRUE: Enable horizontal LPF.  FALSE: Disable horizontal LPF.
		} SET_HLPF_EN;

		/**
		    Set Display Engine Bus Output value Limiter

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_OUT_LIMIT"
		    to set the displayBus Output value Limiter. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_OUT_LIMIT.ui_y_low     = 0x00;
		\n  disp_ctrl.SEL.SET_OUT_LIMIT.ui_y_up      = 0xFF;
		\n  disp_ctrl.SEL.SET_OUT_LIMIT.ui_cb_low    = 0x00;
		\n  disp_ctrl.SEL.SET_OUT_LIMIT.ui_cb_up     = 0xFF;
		\n  disp_ctrl.SEL.SET_OUT_LIMIT.ui_cr_low    = 0x00;
		\n  disp_ctrl.SEL.SET_OUT_LIMIT.ui_cr_up     = 0xFF;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_OUT_LIMIT, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_OUT_LIMIT
			UINT8           ui_y_low;         ///<  Y value Output Lower Bound
			UINT8           ui_y_up;          ///<  Y value Output Upper Bound
			UINT8           ui_cb_low;        ///< Cb value Output Lower Bound
			UINT8           ui_cb_up;         ///< Cb value Output Upper Bound
			UINT8           ui_cr_low;        ///< Cr value Output Lower Bound
			UINT8           ui_cr_up;         ///< Cr value Output Upper Bound
		} SET_OUT_LIMIT;

		/**
		    Set Display Engine Constant Window Offset

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CONST_OFS"
		    to set the display  Constant Window Offset. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x    = 0x0;
		\n  disp_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y    = 0x0;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CONST_OFS, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPDEV_IOCTRL_SET_CONST_OFS
			UINT32          ui_win_const_ofs_x;    ///< The display Video/OSD layer output window constant offset X
			UINT32          ui_win_const_ofs_y;    ///< The display Video/OSD layer output window constant offset Y
		} SET_CONST_OFS;

		/**
		    Set Display Device optimal buffer/window size

		    This structure is used for display device control operation code "DISPDEV_IOCTRL_SET_DISPSIZE"
		    to set the device optimal size. This operation is normally used in the panel device open,
		    and the user can use the "DISPDEV_IOCTRL_GET_DISPSIZE: to get the optimally buffer window size for this device.
		    This structure shuld be used like this:
		\n  DispDev.SEL.SET_DISPSIZE.ui_buf_width    = 320;
		\n  DispDev.SEL.SET_DISPSIZE.ui_buf_height   = 240;
		\n  DispDev.SEL.SET_DISPSIZE.ui_win_width    = 320;
		\n  DispDev.SEL.SET_DISPSIZE.ui_win_height   = 240;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_DISPSIZE, &DispDev);
		*/
		struct { // DISPDEV_IOCTRL_SET_DISPSIZE
			UINT32              ui_buf_width;     ///< Suggest Buffer width. Unit in pixels.
			UINT32              ui_buf_height;    ///< Suggest Buffer Height. Unit in pixels.

			UINT32              ui_win_width;     ///< Suggest Window width. Unit in pixels.
			UINT32              ui_win_height;    ///< Suggest Window Height. Unit in pixels.
		} SET_DISPSIZE;


		/**
		    Set Display Engine Clock1/2

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CLK1_2"
		    to set the display clock 1/2. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CLK1_2.b_clk1_2    =  TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CLK1_2, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_CLK1_2
			BOOL               b_clk1_2;     ///< Clock 1/2
		} SET_CLK1_2;

		/**
		    Set Display Engine dummy swap

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_RGBD_SWAP"
		    to set the display dummy swap. This structure shuld be used like this:
		\n  DispCtrl.SEL.SET_RGBD_SWAP.swap    =  TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_RGBD_SWAP, &DispCtrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_RGBD_SWAP
			BOOL               swap;     ///< dummy swap
		} SET_RGBD_SWAP;


		/* GET control group */

		/**
		    Get Display Engine Enable/Disable

		    This structure is used for display control operation code "DISPDEV_IOCTRL_GET_ENABLE"
		    to get the display enable/disable. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_ENABLE, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ENABLE.b_en"
		    is the display enable/disable information.
		*/
		struct { // DISPDEV_IOCTRL_GET_ENABLE
			BOOL            b_en;          ///< Get display Enable/Disable
		} GET_ENABLE;

		/**
		    Get Display Engine outout format type

		    This structure is used for display control operation code "DISPDEV_IOCTRL_GET_DEVICE"
		    to get the display output format type. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_DEVICE, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_DEVICE.disp_dev_type"
		    is the display Engine outout format type information
		*/
		struct { // DISPDEV_IOCTRL_GET_ENABLE
			DISPDEV_TYPE    disp_dev_type;    ///< Get display device type
		} GET_DEVICE;

		/**
		    Get Display Engine Clock Source

		    This structure is used for display control operation code "DISPDEV_IOCTRL_GET_SRCCLK"
		    to get the display Clock Source. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_SRCCLK, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_SRCCLK.src_clk"
		    is the display Clock Source information.
		*/
		struct { // DISPDEV_IOCTRL_GET_SRCCLK
			DISPCTRL_SRCCLK    src_clk;   ///< Get display source clock
		} GET_SRCCLK;

		/**
		    Get the user assigned LCD Control interface configurations

		    This structure is used for display device control operation code "DISPDEV_IOCTRL_GET_REG_IF"
		    to get the user assigned LCD Control interface configurations. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_TVADJUST, &DispDev);
		\n  "DispDev.SEL.GET_REG_IF.lcd_ctrl"/"DispDev.SEL.GET_REG_IF.ui_sif_ch"/"DispDev.SEL.GET_REG_IF.ui_gpio_sen"/ ..
		    are the user assigned LCD Control interface configurations.
		*/
		struct { // DISPDEV_IOCTRL_GET_REG_IF
			DISPDEV_LCDCTRL lcd_ctrl;        ///< Get LCD Control Interface select

			SIF_CH          ui_sif_ch;        ///< If interface is SIF, this field is the assigned SIF CH number.
			UINT32          ui_gpio_sen;      ///< If interface is GPIO, this field is the assigned GPIO Pin ID for SIF SEN.
			UINT32          ui_gpio_clk;      ///< If interface is GPIO, this field is the assigned GPIO Pin ID for SIF CLK.
			UINT32          ui_gpio_data;     ///< If interface is GPIO, this field is the assigned GPIO Pin ID for SIF DATA.
		} GET_REG_IF;

		/**
		    Get HDMI Display VIDEO/AUDIO mode

		    This structure is used for display device control operation code "DISPDEV_IOCTRL_GET_HDMIMODE"
		    to get the HDMI VIDEO/AUDIO mode. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_HDMIMODE, &DispDev);
		\n  "DispDev.SEL.GET_HDMIMODE.video_id"/"DispDev.SEL.GET_HDMIMODE.audio_id"
		    are the HDMI VIDEO/AUDIO mode selection.
		*/
		struct { // DISPDEV_IOCTRL_GET_HDMIMODE
			HDMI_VIDEOID    video_id;        ///< HDMI VIDEO ID select
			HDMI_AUDIOFMT   audio_id;        ///< HDMI AUDIO ID select
		} GET_HDMIMODE;

		/**
		    Get the current active Display Device ID

		    This structure is used for display device control operation code "DISPDEV_IOCTRL_GET_ACT_DEVICE"
		    to get the current active display device of the display object. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_ACT_DEVICE, &DispDev);
		\n  "DispDev.SEL.GET_ACT_DEVICE.dev_id"
		    is the current active display device ID.
		*/
		struct { // DISPDEV_IOCTRL_GET_ACT_DEVICE
			DISPDEV_ID  dev_id;              ///< Current Active display device ID.
		} GET_ACT_DEVICE;

		/**
		    Get the user defined panel adjustment callback

		    This structure is used for display device control operation code "DISPDEV_IOCTRL_GET_PANEL_ADJUST"
		    to get the user defined panel adjustment callback. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_PANEL_ADJUST, &DispDev);
		\n  "DispDev.SEL.GET_PANEL_ADJUST.pfp_adjust"
		    is the user defined panel adjustment callback.
		*/
		struct { // DISPDEV_IOCTRL_GET_PANEL_ADJUST
			FP              pfp_adjust;      ///< Get the panel adjustment callback function
		} GET_PANEL_ADJUST;

		/**
		    Get the user assigned TV adjustment resistor value

		    This structure is used for display device control operation code "DISPDEV_IOCTRL_GET_TVADJUST"
		    to get the user assigned TV adjustment resistor value. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_TVADJUST, &DispDev);
		\n  "DispDev.SEL.GET_TVADJUST.tv_adjust"
		    is the user assigned TV adjustment resistor value.
		*/
		struct { // DISPDEV_IOCTRL_GET_TVADJUST
			DISPDEV_TVADJUST    tv_adjust;   ///< Get the user assigned TV adjustment resistor value
		} GET_TVADJUST;

		/**
		    Get Display Engine Gamma Function

		    This structure is used for display control operation code "DISPDEV_IOCTRL_GET_GAMMA_EN"
		    to get the display gamma function enable/disable and also the gamma table. This structure shuld be used like this:
		\n  dispDevCtrl(DISPDEV_IOCTRL_GET_GAMMA_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_GAMMA_EN.b_en "
		    is the Display Engine Gamma Function Disable/Enable
		*/
		struct { // DISPDEV_IOCTRL_GET_GAMMA_EN
			BOOL            b_en;           ///< Enable/Disable the Gamma function for component Y/RGB
		} GET_GAMMA_EN;

		/**
		    Get Display Engine Contrast/Saturation/Brightness configurations

		    This structure is used for display control operation code "DISPDEV_IOCTRL_GET_CSB_EN"
		    to get the display Contrast/Saturation/Brightness configurations. This structure shuld be used like this:
		\n  dispDevCtrl(DDISPDEV_IOCTRL_GET_CSB_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CSB_EN.b_en"
		\n  "disp_ctrl.SEL.GET_CSB_EN.ui_contrast"
		\n  "disp_ctrl.SEL.GET_CSB_EN.ui_saturation"
		\n  "disp_ctrl.SEL.GET_CSB_EN.ui_brightness"
		   is the Display Engine Contrast/Saturation/Brightness configurations.
		*/
		struct { // DISPDEV_IOCTRL_GET_CSB_EN
			BOOL            b_en;            ///< Set Contrast/Saturation/Brightness adjustment function enable/disable

			UINT32          ui_contrast;     ///< Contrast value, valid range from 0x0 to 0xFF. default value is 0x40.
			UINT32          ui_saturation;   ///< Saturation value, valid range from 0x0 to 0x1FF. default value is 0x40.
			UINT32          ui_brightness;   ///< The brightness values, Valid range between -64 to 63. 2's complement value.
		} GET_CSB_EN;

		/**
		    Get TV parameter

		    This structure is used for display device control operation code "DISPDEV_GET_IOCTRL_TVPAR"
		    to get the TV's parameters.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_IOCTRL_GET_TVPAR, &DispDev);
		\n  "DispDev.SEL.GET_TVPAR.b_en_user" is Enable/Disable user define value
		\n  "DispDev.SEL.GET_TVPAR.ui_bll" is the TV's blank level
		\n  "DispDev.SEL.GET_TVPAR.ui_brl" is the TV's burst level
		\n  "DispDev.SEL.GET_TVPAR.ui_setup" is the TV's setup level
		\n  "DispDev.SEL.GET_TVPAR.uiYScale" is the TV's y scale level
		\n  "DispDev.SEL.GET_TVPAR.uiCbScale" is the TV's cb scale level
		\n  "DispDev.SEL.GET_TVPAR.uiCrScale" is the TV's cr scale level
		*/
		struct { // DISPDEV_IOCTRL_GET_TVPAR
			BOOL        b_en_user;               ///< Enable/Disable user's setting value
			UINT32      ui_ntsc_bll;            ///< TV NTSC Blank Level(valid value:0~0xFF)
			UINT32      ui_ntsc_brl;            ///< TV NTSC Burst Level(valid value:0~0xFF)
			UINT32      ui_ntsc_setup;          ///< TV NTSC setupt(valid value:0~0xFF)
			UINT32      ui_ntsc_y_scaling;       ///< TV NTSC Y Scale(valid value:0~0xFF)
			UINT32      ui_ntsc_cb_scaling;      ///< TV NTSC Cb Scale(valid value:0~0xFF)
			UINT32      ui_ntsc_cr_scaling;      ///< TV NTSC Cr Scale(valid value:0~0xFF)
			UINT32      ui_pal_bll;             ///< TV PAL Blank Level(valid value:0~0xFF)
			UINT32      ui_pal_brl;             ///< TV PAL Burst Level(valid value:0~0xFF)
			UINT32      ui_pal_setup;           ///< TV PAL setupt(valid value:0~0xFF)
			UINT32      ui_pal_y_scaling;        ///< TV PAL Y Scale(valid value:0~0xFF)
			UINT32      ui_pal_cb_scaling;       ///< TV PAL Cb Scale(valid value:0~0xFF)
			UINT32      ui_pal_cr_scaling;       ///< TV PAL Cr Scale(valid value:0~0xFF)
		} GET_TVPAR;


		/**
		    Get TV Full screen

		    This structure is used for display device control operation code "DISPDEV_GET_IOCTRL_TVFULL"
		    to get the TV's parameters.
		    This structure shuld be used like this:
		\n  pDispObj->dev_ctrl(DISPDEV_IOCTRL_GET_TVFULL, &DispDev);
		*/
		struct { // DISPDEV_IOCTRL_GET_TVFULL
			BOOL        b_en_full;               ///< Enable/Disable tv full screen
		} GET_TVFULL;



		/**
		    Set Display Engine clock frequency

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CLK_FREQ"
		    to set the display clock frequency. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CLK_FREQ.bUse2x    =  FALSE;
		\n  disp_ctrl.SEL.SET_CLK_FREQ.ui_freq    =  10000000;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CLK_FREQ, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_CLK_FREQ
			BOOL               b_ycc8bit;        ///< YCbCr serial 8 bit (use clock2x), Format:YCbCr/CCIR601/CCIR656 8bit set to TRUE
			///< others set to FALSE
			UINT32             ui_freq;          ///< clock freq: unit: Hz
		} SET_CLK_FREQ;

		/**
		    Set Display Engine clock Enable/Disable

		    This structure is used for display control operation code "DISPDEV_IOCTRL_SET_CLK_EN"
		    to set the display clock frequency. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CLK_EN.b_clk_en    =  TRUE;
		\n  dispDevCtrl(DISPDEV_IOCTRL_SET_CLK_EN, &disp_ctrl);
		*/
		struct { // DISPDEV_IOCTRL_SET_CLK_EN
			BOOL               b_clk_en;          ///< clock enable/disable
		} SET_CLK_EN;


	} SEL;
} DISPDEV_IOCTRL_PARAM, *PDISPDEV_IOCTRL_PARAM;


typedef ER(*DISPDEV_IOCTRL)(DISPDEV_IOCTRL_OP, PDISPDEV_IOCTRL_PARAM);


#endif
//@}

