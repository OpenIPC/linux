/**
    @file       disp_ctrl.h
    @ingroup    mISYSDisp

    @brief      Header file for Display Engine control
				This file is the header file that define the API and data type
				for Display control object
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

/** \addtogroup  mISYSDisp*/
//@{

#ifndef _DISPCTRL_H
#define _DISPCTRL_H

#include "dispcomn.h"
#include "ide.h"
//#include "ide2.h"



/**
    @name Display Control Definition Group

    These definitions are used for display control API (DISP_OBJ.disp_ctrl) as the display control portal.
    The display general functionalities such as subpixel/hlpf/Contrast/Saturation/Brightness (...etc) are controlled by this API.
    The user must know that the API would not set load to activate the settings.
    The User would programs all the settings needed and then use DISP_OBJ.load() to activate all the settings.
*/
//@{

/**
    Display output format type definition

    This definition is used for the OP Code: "DISPCTRL_SET_DEVICE" in the disp_ctrl API to specify the display engine output format type.
    Such as RGB/YUV-Serial,RGB/YUV-Parallel,CCIR-601/656, ...etc.
*/
typedef enum {
	DISPDEV_TYPE_RGB_SERIAL,                ///< Display mode is RGB Serial or UPS051
	DISPDEV_TYPE_RGB_PARALL,                ///< Display mode is RGB Parallel
	DISPDEV_TYPE_RGBD,                      ///< Display mode is RGB Dummy
	DISPDEV_TYPE_RGB_THROUGH,               ///< Display mode is RGB through mode
	DISPDEV_TYPE_YUV,                       ///< Display mode is YUV
	DISPDEV_TYPE_CCIR601_8BIT,              ///< Display mode is CCIR601 8bits bus
	DISPDEV_TYPE_CCIR656_8BIT,              ///< Display mode is CCIR656 8bits bus
	DISPDEV_TYPE_CCIR601_16BIT,             ///< Display mode is CCIR601 16bits bus
	DISPDEV_TYPE_CCIR656_16BIT,             ///< Display mode is CCIR656 16bits bus
	DISPDEV_TYPE_RGBDELTA_16BIT,            ///< Display mode is RGB Delta 16bits bus

	DISPDEV_TYPE_INF_HDMI_8BIT_DDR,         ///< Display mode is outsourcing HDMI 8bits with double-data-rate(DDR) bus
	DISPDEV_TYPE_INF_HDMI_16BIT,            ///< Display mode is outsourcing HDMI 16bits bus
	DISPDEV_TYPE_INF_HDMI_24BIT,            ///< Display mode is outsourcing HDMI 24bits bus

	DISPDEV_TYPE_MI,                        ///< Display mode is Memory interface
	DISPDEV_TYPE_EMBD_TV,                   ///< Display mode is Embedded TV Encoder
	DISPDEV_TYPE_EMBD_HDMI,                 ///< Display mode is Embedded HDMI transmitter
	DISPDEV_TYPE_EMBD_MIPIDSI,              ///< Display mode is Embedded MIPI DSI tx.

	DISPDEV_TYPE_OUTPUT_DRAM,               ///< Display mode is output to dram via VDO2 channel

	ENUM_DUMMY4WORD(DISPDEV_TYPE)
} DISPDEV_TYPE;

/**
    Display Engine Source clock select definition

    This definition is used for the OP Code: "DISPCTRL_SET_SRCCLK" in the disp_ctrl API to specify the display engine clock source.
*/
typedef enum {
	DISPCTRL_SRCCLK_PLL1,                   ///< The display source clock from 480MHz
	DISPCTRL_SRCCLK_PLL6,                   ///< The display source clock from PLL6
	DISPCTRL_SRCCLK_PLL4,                   ///< The display source clock from PLL4
	DISPCTRL_SRCCLK_PLL9,                   ///< The display source clock from PLL9

	DISPCTRL_SRCCLK_DEFAULT = DISPCTRL_SRCCLK_PLL6,///< Default display source clock selection for realchip
	ENUM_DUMMY4WORD(DISPCTRL_SRCCLK)
} DISPCTRL_SRCCLK;

/**
    Display control operation ID definition

    This field is used as the first operand in the disp_ctrl API of the display object to specify
    which of the display control operation would be performed.
*/
typedef enum {
	/* SET control group */
	DISPCTRL_SET_ENABLE = DISPCMDBASE_DISPCTRL, ///< Set display engine's global enable/disable.
	DISPCTRL_SET_ALL_LYR_EN,                ///< Set ALL selected display layers Enable/Disable.
	DISPCTRL_SET_BACKGROUND,                ///< Set Display background color.
	DISPCTRL_SET_DMA_ABORT,                 ///< Set Display dma engine's abort enable/disable.
	DISPCTRL_SET_GAMMA_EN,                  ///< Set display Gamma function enable/disable. Also config the Gamma coefficients if function enabled.
	DISPCTRL_SET_ICST_EN,                   ///< Set display Inverse-color-Space-Transform (ICST1) function enable/disable.
	DISPCTRL_SET_ICST_COEF,                 ///< Set display Inverse-color-Space-Transform (ICST1) function coefficients.
	DISPCTRL_SET_HLPF_EN,                   ///< Set display horizontal low pass filter enable/disable.
	DISPCTRL_SET_CONST_OFS,                 ///< Set display window constant offset X/Y. This feature is used for handling the overscan/underscan hdmi tv.
	DISPCTRL_SET_SRCCLK,                    ///< Set display operating source clock
	DISPCTRL_SET_GAMMA_Y,                   ///< Set display gamma table Y
	DISPCTRL_SET_GAMMA_RGB,                 ///< Set display gamma table RGB
	DISPCTRL_SET_ICST0_EN,                  ///< Set display ICST0 enable/disable
	DISPCTRL_SET_ICST0_COEF,                ///< Set display Inverse-color-Space-Transform (ICST0) function coefficients.
	DISPCTRL_SET_CST_EN,                    ///< Set display CST1 enable/disable
	DISPCTRL_SET_CC_EN,                     ///< Set display color control enable/disable
	DISPCTRL_SET_CCA_EN,                    ///< Set display color component adjustment enable/disable
	DISPCTRL_SET_CCA_HUE_EN,                ///< Set display color component Hue adjustment enable/disable
	DISPCTRL_SET_CCA_YCON_EN,               ///< Set display color component Y contrast adjustment enable/disable
	DISPCTRL_SET_CCA_CCON_EN,               ///< Set display color component C contrast adjustment enable/disable
	DISPCTRL_SET_CC_HUE,                    ///< Set display color control hue table
	DISPCTRL_SET_CC_INT,                    ///< Set display color control intensity table
	DISPCTRL_SET_CC_SAT,                    ///< Set display color control saturation table
	DISPCTRL_SET_CC_DDS,                    ///< Set display color control dds table
	DISPCTRL_SET_CC_INT_OFS,                ///< Set display color control intensity offset
	DISPCTRL_SET_CC_SAT_OFS,                ///< Set display color control saturation offset
	DISPCTRL_SET_CCA_YCON,                  ///< Set display color component adjustment Y contrast
	DISPCTRL_SET_CCA_CCON,                  ///< Set display color component adjustment C contrast
	DISPCTRL_SET_CCA_YOFS,                  ///< Set display color component adjustment Y offset
	DISPCTRL_SET_CCA_COFS,                  ///< Set display color component adjustment C offset


	/* GET control group */
	DISPCTRL_GET_ENABLE,                    ///< Get display engine's global enable/disable.
	DISPCTRL_GET_ALL_LYR_EN,                ///< Get ALL selected display layers Enable/Disable.
	DISPCTRL_GET_BACKGROUND,                ///< Get Display background color.
	DISPCTRL_GET_GAMMA_EN,                  ///< Get display Gamma function enable/disable. Also config the Gamma coefficients if function enabled.
	DISPCTRL_GET_ICST_EN,                   ///< Get display Inverse-color-Space-Transform(ICST1) function enable/disable.
	DISPCTRL_GET_ICST_COEF,                 ///< Get display Inverse-color-Space-Transform(ICST1) function coefficients.
	DISPCTRL_GET_HLPF_EN,                   ///< Get display horizontal low pass filter enable/disable.
	DISPCTRL_GET_CONST_OFS,                 ///< Get display window constant offset X/Y. This feature is used for handling the overscan/underscan hdmi tv.
	DISPCTRL_GET_SRCCLK,                    ///< Get display operating source clock
	DISPCTRL_GET_GAMMA_Y,                   ///< Get display gamma table Y
	DISPCTRL_GET_GAMMA_RGB,                 ///< Get display gamma table RGB
	DISPCTRL_GET_ICST0_EN,                  ///< Get display ICST0 enable/disable
	DISPCTRL_GET_ICST0_COEF,                ///< Get display Inverse-color-Space-Transform (ICST0) function coefficients.
	DISPCTRL_GET_CST_EN,                    ///< Get display CST1 enable/disable
	DISPCTRL_GET_CC_EN,                     ///< Get display color control enable/disable
	DISPCTRL_GET_CCA_EN,                    ///< Get display color component adjustment enable/disable
	DISPCTRL_GET_CCA_HUE_EN,                ///< Get display color component Hue adjustment enable/disable
	DISPCTRL_GET_CCA_YCON_EN,               ///< Get display color component Y contrast adjustment enable/disable
	DISPCTRL_GET_CCA_CCON_EN,               ///< Get display color component C contrast adjustment enable/disable
	DISPCTRL_GET_CC_HUE,                    ///< Get display color control hue table
	DISPCTRL_GET_CC_INT,                    ///< Get display color control intensity table
	DISPCTRL_GET_CC_SAT,                    ///< Get display color control saturation table
	DISPCTRL_GET_CC_DDS,                    ///< Get display color control dds table
	DISPCTRL_GET_CC_INT_OFS,                ///< Get display color control intensity offset
	DISPCTRL_GET_CC_SAT_OFS,                ///< Set display color control saturation offset
	DISPCTRL_GET_CCA_YCON,                  ///< Get display color component adjustment Y contrast
	DISPCTRL_GET_CCA_CCON,                  ///< Get display color component adjustment C contrast
	DISPCTRL_GET_CCA_YOFS,                  ///< Get display color component adjustment Y offset
	DISPCTRL_GET_CCA_COFS,                  ///< Get display color component adjustment C offset


	ENUM_DUMMY4WORD(DISPCTRL_OP)
} DISPCTRL_OP;

/**
    Display Control Parameter Definition

    This definition is used as the second operand in the disp_ctrl API to specify the display control parameters
    for each of the display control opeation.
*/
typedef struct {
	union {
		/* SET control group */

		/**
		    Set Display Engine Enable/Disable

		    This structure is used for display control operation code "DISPCTRL_SET_ENABLE"
		    to set the display enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ENABLE.b_en     = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ENABLE, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_ENABLE
			BOOL            b_en;            ///< Display Enable/Disable
		} SET_ENABLE;

		/**
		    Set Display Engine layers enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_ALL_LYR_EN"
		    to set the display layers enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ALL_LYR_EN.b_en     = TRUE;
		\n  disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr = DISPLAYER_VDO1|DISPLAYER_OSD1;    // Enable the specified layers.
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &disp_ctrl);
		\n  Or Using like this:
		\n  disp_ctrl.SEL.SET_ALL_LYR_EN.b_en     = FALSE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { //DISPCTRL_SET_ALL_LYR_EN
			BOOL            b_en;            ///< For ALL display layers ON/OFF. TRUE: Enable. FALSE:Disable.
			DISPLAYER       disp_lyr;        ///< If b_en = TRUE, the specified display layers would be enabled.
			///< If b_en = FALSE, "ALL" the display layers would be disabled. This parameter would be useless.
		} SET_ALL_LYR_EN;

		/**
		    Set the Display default background color

		    This structure is used for display control operation code "DISPCTRL_SET_BACKGROUND"
		    to set the display default background color.
		    If none of the display layer is enabled, the default color on the display screen is the background color.
		    This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_BACKGROUND.ui_color_y     = 0x00;
		\n  disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb    = 0x80;
		\n  disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr    = 0x80;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_BACKGROUND
			UINT32          ui_color_y;       ///< Background color  Y value. Must be within 0~0xFF.
			UINT32          ui_color_cb;      ///< Background color CB value. Must be within 0~0xFF.
			UINT32          ui_color_cr;      ///< Background color CR value. Must be within 0~0xFF.
		} SET_BACKGROUND;

		/**
		    Set Display DMA Engine Abort Enable/Disable

		    This structure is used for display control operation code "DISPCTRL_SET_DMA_ABORT"
		    to set the display dma abort enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_DMA_ABORT.b_en     = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_DMA_ABORT, &disp_ctrl);
		*/
		struct { // SET_DMA_ABORT
			BOOL            b_en;            ///< Display dma abort Enable/Disable
		} SET_DMA_ABORT;
		/**
		    Set Display Engine Gamma Function

		    This structure is used for display control operation code "DISPCTRL_SET_GAMMA_EN"
		    to set the display gamma function enable/disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_GAMMA_EN.b_en          = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_GAMMA_EN, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_GAMMA_EN
			BOOL            b_en;           ///< Enable/Disable the Gamma function
			//UINT8           *p_gamma_tab_y;    ///< The 17 UINT8 entries gamma table for component Y. This field is useless if bEnY=FALSE.
		} SET_GAMMA_EN;

		/**
		    Set Display Engine Inverse color Space Transform

		    This structure is used for display control operation code "DISPCTRL_SET_ICST_EN"
		    to set the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ICST_EN.b_en        = TRUE;
		\n  disp_ctrl.SEL.SET_ICST_EN.select     = CST_YCBCR2RGB;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ICST_EN, &disp_ctrl);
		\n  Or Using like this:
		\n  disp_ctrl.SEL.SET_ICST_EN.b_en        = FALSE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ICST_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_ICST_EN
			BOOL            b_en;            ///< The Inverse color space transform function enable/disable.
			CST_SEL         select;         ///< The color space transform of RGB2YCBCR or YCBCR2RGB.
		} SET_ICST_EN;

		/**
		    Set Display Engine ICST Coeficients

		    This structure is used for display control operation code "DISPCTRL_SET_ICST_COEF"
		    to set the display Inverse color Space Transform Coeficients. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ICST_COEF.pi_coef     = (Address of the ICST Coeficients table);
		\n  disp_ctrl.SEL.SET_ICST_COEF.i_pre_ofs_y   = 0;
		\n  disp_ctrl.SEL.SET_ICST_COEF.i_pre_ofs_cb  = -128;
		\n  disp_ctrl.SEL.SET_ICST_COEF.i_pre_ofs_cr  = -128;
		\n  disp_ctrl.SEL.SET_ICST_COEF.i_post_ofs_y  = 0;
		\n  disp_ctrl.SEL.SET_ICST_COEF.i_post_ofs_cb = 0;
		\n  disp_ctrl.SEL.SET_ICST_COEF.i_post_ofs_cr = 0;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ICST_COEF, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_ICST_COEF
			INT32          *pi_coef;         ///< The 3X3 ICST Matrix coefficients. The coefficient format is (Q3.8). Valid range from -2048 to 2047.
			///< The coefficients in order are: Ry, Ru, Rv, Gy, Gu, Gv, By, Bu, Bv.

			INT16           i_pre_ofs_y;       ///< Inverse color space transform Pre-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_pre_ofs_cb;      ///< Inverse color space transform Pre-Offset value for Cb. Valid range from -128 to 128.
			INT16           i_pre_ofs_cr;      ///< Inverse color space transform Pre-Offset value for Cr. Valid range from -128 to 128.

			INT16           i_post_ofs_y;      ///< Inverse color space transform Post-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_post_ofs_cb;     ///< Inverse color space transform Post-Offset value for Cb. Valid range from -128 to 128.
			INT16           i_post_ofs_cr;     ///< Inverse color space transform Post-Offset value for Cr. Valid range from -128 to 128.
		} SET_ICST_COEF;

		/**
		    Set Display Engine Horizontal Low Pass Filter Enable/Disable

		    This structure is used for display control operation code "DISPCTRL_SET_HLPF_EN"
		    to set the display Horizontal Low Pass Filter Enable/Disable. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_HLPF_EN.b_en     = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_HLPF_EN, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_HLPF_EN
			BOOL            b_en;            ///< TRUE: Enable horizontal LPF.  FALSE: Disable horizontal LPF.
		} SET_HLPF_EN;

		/**
		    Set Display Engine Constant Window Offset

		    This structure is used for display control operation code "DISPCTRL_SET_CONST_OFS"
		    to set the display  Constant Window Offset. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x    = 0x0;
		\n  disp_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y    = 0x0;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CONST_OFS, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CONST_OFS
			UINT32          ui_win_const_ofs_x;    ///< The display Video/OSD layer output window constant offset X
			UINT32          ui_win_const_ofs_y;    ///< The display Video/OSD layer output window constant offset Y
		} SET_CONST_OFS;

		/**
		    Set Display Engine Clock Source

		    This structure is used for display control operation code "DISPCTRL_SET_SRCCLK"
		    to set the display Clock Source. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_SRCCLK.src_clk    = DISPCTRL_SRCCLK_PLL2;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_SRCCLK, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_SRCCLK
			DISPCTRL_SRCCLK    src_clk;         ///< select display source clock
		} SET_SRCCLK;


		/**
		    Set Display Engine Gamma Table Y

		    This structure is used for display control operation code "DISPCTRL_SET_GAMMA_Y"
		    to set the display gamma table Y. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_GAMMA_Y.p_gamma_tab_y    = (Address of the Gamman Table);
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_GAMMA_Y, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_GAMMA_Y
			UINT8           *p_gamma_tab_y;    ///< The UINT8 entries gamma table for component Y.(for 220:17 entries)
		} SET_GAMMA_Y;

		/**
		    Set Display Engine Gamma Table RGB

		    This structure is used for display control operation code "DISPCTRL_SET_GAMMA_RGB"
		    to set the display gamma table R. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_GAMMA_RGB.p_gamma_tab_r    = (Address of the Gamman Table R);
		 \n  disp_ctrl.SEL.SET_GAMMA_RGB.p_gamma_tab_g    = (Address of the Gamman Table G);
		  \n  disp_ctrl.SEL.SET_GAMMA_RGB.p_gamma_tab_b    = (Address of the Gamman Table B);
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_GAMMA_RGB, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_GAMMA_RGB
			UINT8           *p_gamma_tab_r;    ///< The UINT8 entries gamma table for component R.(for 650:33 entries)
			UINT8           *p_gamma_tab_g;    ///< The UINT8 entries gamma table for component G.(for 650:33 entries)
			UINT8           *p_gamma_tab_b;    ///< The UINT8 entries gamma table for component B.(for 650:33 entries)
		} SET_GAMMA_RGB;

		/**
		    Set Display Engine ICST0

		    This structure is used for display control operation code "DISPCTRL_SET_ICST0_EN"
		    to set the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ICST0_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ICST0_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_ICST0_EN
			BOOL            b_en;            ///< The ICST0 enable/disable.
		} SET_ICST0_EN;

		/**
		    Set Display Engine ICST0 Coeficients

		    This structure is used for display control operation code "DISPCTRL_SET_ICST0_COEF"
		    to set the display Inverse color Space Transform Coeficients. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_ICST0_COEF.pi_coef     = (Address of the ICST0 Coeficients table);
		\n  disp_ctrl.SEL.SET_ICST0_COEF.i_pre_ofs_y   = 0;
		\n  disp_ctrl.SEL.SET_ICST0_COEF.i_pre_ofs_cb  = -128;
		\n  disp_ctrl.SEL.SET_ICST0_COEF.i_pre_ofs_cr  = -128;
		\n  disp_ctrl.SEL.SET_ICST0_COEF.i_post_ofs_y  = 0;
		\n  disp_ctrl.SEL.SET_ICST0_COEF.i_post_ofs_cb = 0;
		\n  disp_ctrl.SEL.SET_ICST0_COEF.i_post_ofs_cr = 0;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_ICST0_COEF, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_ICST0_COEF
			INT32          *pi_coef;         ///< The 3X3 ICST Matrix coefficients. The coefficient format is (Q3.8). Valid range from -2048 to 2047.
			///< The coefficients in order are: Ry, Ru, Rv, Gy, Gu, Gv, By, Bu, Bv.

			INT16           i_pre_ofs_y;       ///< Inverse color space transform Pre-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_pre_ofs_cb;      ///< Inverse color space transform Pre-Offset value for Cb. Valid range from -128 to 128.
			INT16           i_pre_ofs_cr;      ///< Inverse color space transform Pre-Offset value for Cr. Valid range from -128 to 128.

			INT16           i_post_ofs_y;      ///< Inverse color space transform Post-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_post_ofs_cb;     ///< Inverse color space transform Post-Offset value for Cb. Valid range from -128 to 128.
			INT16           i_post_ofs_cr;     ///< Inverse color space transform Post-Offset value for Cr. Valid range from -128 to 128.
		} SET_ICST0_COEF;


		/**
		    Set Display Engine CST

		    This structure is used for display control operation code "DISPCTRL_SET_CST_EN"
		    to set the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CST_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CST_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CST_EN
			BOOL            b_en;            ///< The CST  enable/disable.
		} SET_CST_EN;

		/**
		    Set Display Engine color control enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CC_EN"
		    to set the display color control adjustment. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CC_EN
			BOOL            b_en;            ///< The CC  enable/disable.
		} SET_CC_EN;

		/**
		    Set Display Engine color control adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_EN"
		    to set the display color control adjustment. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CCA_EN
			BOOL            b_en;            ///< The CCA  enable/disable.
		} SET_CCA_EN;


		/**
		    Set Display Engine color control Hue adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_HUE_EN"
		    to set the display color control Hue adjustment . This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_HUE_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_HUE_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CCA_HUE_EN
			BOOL            b_en;            ///< The CCA HUE  enable/disable.
		} SET_CCA_HUE_EN;

		/**
		    Set Display Engine color control Y contrast adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_YCON_EN"
		    to set the display color control Y contrast adjustment . This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_YCON_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_YCON_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CCA_YCON_EN
			BOOL            b_en;            ///< The CCA YCON  enable/disable.
		} SET_CCA_YCON_EN;

		/**
		    Set Display Engine color control C contrast adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_CCON_EN"
		    to set the display color control C contrast adjustment . This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_CCON_EN.b_en        = TRUE;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_CCON_EN, &disp_ctrl);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPCTRL_SET_CCA_CCON_EN
			BOOL            b_en;            ///< The CCA CCON  enable/disable.
		} SET_CCA_CCON_EN;

		/**
		    Set Display Engine color control Hue table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_HUE"
		    to set the display color control  Hue table. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_HUE.p_hue_tab       = (Address of the HUE table);
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_HUE, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CC_HUE
			UINT8           *p_hue_tab;    ///< The UINT8 entries hue table.(for 650:24 entries)
		} SET_CC_HUE;


		/**
		    Set Display Engine color control Intensity table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_INT"
		    to set the display color control  Intensity table. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_INT.p_int_tab       = (Address of the INT table);
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_INT, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CC_INT
			INT8            *p_int_tab;    ///< The INT8 entries intensity table.(for 650:24 entries)
		} SET_CC_INT;

		/**
		    Set Display Engine color control Saturation table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_SAT"
		    to set the display color control  Saturation table. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_SAT.p_sat_tab       = (Address of the SAT table);
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_SAT, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CC_SAT
			INT8            *p_sat_tab;    ///< The INT8 entries saturation table.(for 650:24 entries)
		} SET_CC_SAT;

		/**
		    Set Display Engine color control DDS table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_DDS"
		    to set the display color control  DDS table. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_DDS.p_dds_tab       = (Address of the DDS table);
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_DDS, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CC_DDS
			UINT8           *p_dds_tab;    ///< The INT8 entries DDS table.(for 650:8 entries)
		} SET_CC_DDS;


		/**
		    Set Display Engine color control Intensity offset

		    This structure is used for display control operation code "DISPCTRL_SET_CC_INT_OFS"
		    to set the display color control   Intensity offset. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_INT_OFS.iintofs  = 0x80;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_INT_OFS, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CC_INT_OFS
			INT8             iintofs;    ///< The intensity offset, INT8(-128~127)
		} SET_CC_INT_OFS;

		/**
		    Set Display Engine color control Saturation offset

		    This structure is used for display control operation code "DISPCTRL_SET_CC_SAT_OFS"
		    to set the display color control   Saturation offset. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CC_SAT_OFS.isatofs  = 0x80;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CC_SAT_OFS, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CC_SAT_OFS
			INT8             isatofs;    ///< The Saturation offset, INT8(-128~127)
		} SET_CC_SAT_OFS;

		/**
		   Set Display Engine color component adjustment Y contrast

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_YCON"
		   to set the display color component adjustment Y contrast. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_YCON.uiycon  = 0x80;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_YCON, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CCA_YCON
			UINT8            uiycon;    ///< The Y contrast (0~255)
		} SET_CCA_YCON;

		/**
		   Set Display Engine color component adjustment Cb/Cr contrast

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_CCON"
		   to set the display color component adjustment Y contrast. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_CCON.uiccon  = 0x80;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_CCON, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CCA_CCON
			UINT8            uiccon;    ///< The C contrast (0~255)
		} SET_CCA_CCON;

		/**
		   Set Display Engine color component adjustment Y offset

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_YOFS"
		   to set the display color component adjustment Y offset. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_YOFS.iyofs  = 0x00;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_YOFS, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CCA_YOFS
			INT8              iyofs;    ///< The Y Ofs (-128~127)
		} SET_CCA_YOFS;

		/**
		   Set Display Engine color component adjustment Cb/Cr offset

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_COFS"
		   to set the display color component adjustment Y contrast. This structure shuld be used like this:
		\n  disp_ctrl.SEL.SET_CCA_COFS.uicbofs  = 0x80;
		\n  disp_ctrl.SEL.SET_CCA_COFS.uicrofs  = 0x80;
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CCA_COFS, &disp_ctrl);
		*/
		struct { // DISPCTRL_SET_CCA_COFS
			UINT8            uicbofs;    ///< The Cb offset (0~255)
			UINT8            uicrofs;    ///< The Cr offset (0~255)
		} SET_CCA_COFS;


		/* GET control group */

		/**
		    Get Display Engine Enable/Disable

		    This structure is used for display control operation code "DISPCTRL_GET_ENABLE"
		    to get the display enable/disable. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_ENABLE, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ENABLE.b_en"
		    is the display enable/disable information.
		*/
		struct { // DISPCTRL_GET_ENABLE
			BOOL            b_en;          ///< Get display Enable/Disable
		} GET_ENABLE;

		/**
		    Get Display Engine layers enable information

		    This structure is used for display control operation code "DISPCTRL_GET_ALL_LYR_EN"
		    to get the display layers enable information. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_ALL_LYR_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ALL_LYR_EN.disp_lyr"
		    is the display layers enable information.
		*/
		struct { // DISPCTRL_GET_ALL_LYR_EN
			DISPLAYER       disp_lyr;      ///< Get the information of which the display layers are enabled.
		} GET_ALL_LYR_EN;


		/**
		    Get the Display default background color

		    This structure is used for display control operation code "DISPCTRL_GET_BACKGROUND"
		    to get the display default background color.
		    If none of the display layer is enabled, the default color on the display screen is the background color.
		    This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_BACKGROUND, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_BACKGROUND.ui_coloy_y"
		\n  "disp_ctrl.SEL.GET_BACKGROUND.ui_coloy_cb"
		\n  "disp_ctrl.SEL.GET_BACKGROUND.ui_coloy_cr"
		     is the Display default background color
		*/
		struct { // DISPCTRL_GET_BACKGROUND
			UINT32          ui_color_y;       ///< Background color  Y value. Must be within 0~0xFF.
			UINT32          ui_color_cb;      ///< Background color CB value. Must be within 0~0xFF.
			UINT32          ui_color_cr;      ///< Background color CR value. Must be within 0~0xFF.
		} GET_BACKGROUND;
		
		/**
		    Get Display Engine Gamma Function

		    This structure is used for display control operation code "DISPCTRL_GET_GAMMA_EN"
		    to set the display gamma function enable/disable and also the gamma table. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_GAMMA_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_GAMMA_EN.b_en "
		    is the Display Engine Gamma Function Enable/Disable
		*/
		struct { // DISPCTRL_GET_GAMMA_EN
			BOOL            b_en;            ///< Enable/Disable the Gamma function for component Y
		} GET_GAMMA_EN;

		/**
		    Get Display Engine Inverse color Space Transform

		    This structure is used for display control operation code "DISPCTRL_GET_ICST_EN"
		    to get the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_ICST_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ICST_EN.b_en"
		\n  "disp_ctrl.SEL.GET_ICST_EN.select"
		    is the Display Engine Inverse color Space Transform
		*/
		struct { // DISPCTRL_GET_ICST_EN
			BOOL            b_en;            ///< The Inverse color space transform function enable/disable.
			CST_SEL         select;         ///< The color space transform of RGB2YCBCR or YCBCR2RGB.
		} GET_ICST_EN;

		/**
		    Get Display Engine ICST Coeficients

		    This structure is used for display control operation code "DISPCTRL_GET_ICST_COEF"
		    to get the display Inverse color Space Transform Coeficients. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_ICST_COEF, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ICST_COEF.pi_coef "
		\n  "disp_ctrl.SEL.GET_ICST_COEF.i_pre_ofs_y"
		\n  "disp_ctrl.SEL.GET_ICST_COEF.i_pre_ofs_cb"
		\n  "disp_ctrl.SEL.GET_ICST_COEF.i_pre_ofs_cr"
		\n  "disp_ctrl.SEL.GET_ICST_COEF.i_post_ofs_y"
		\n  "disp_ctrl.SEL.GET_ICST_COEF.i_post_ofs_cb"
		\n  "disp_ctrl.SEL.GET_ICST_COEF.i_post_ofs_cr"
		    is the Display Engine ICST Coeficients.
		*/
		struct { // DISPCTRL_GET_ICST_COEF
			INT32          *pi_coef;         ///< The 3X3 ICST Matrix coefficients. The coefficient format is (Q3.8). Valid range from -2048 to 2047.
			///< The coefficients in order are: Ry, Ru, Rv, Gy, Gu, Gv, By, Bu, Bv.

			INT16           i_pre_ofs_y;       ///< Inverse color space transform Pre-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_pre_ofs_cb;      ///< Inverse color space transform Pre-Offset value for Cb. Valid range from -128 to 127.
			INT16           i_pre_ofs_cr;      ///< Inverse color space transform Pre-Offset value for Cr. Valid range from -128 to 127.

			INT16           i_post_ofs_y;      ///< Inverse color space transform Post-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_post_ofs_cb;     ///< Inverse color space transform Post-Offset value for Cb. Valid range from -128 to 127.
			INT16           i_post_ofs_cr;     ///< Inverse color space transform Post-Offset value for Cr. Valid range from -128 to 127.
		} GET_ICST_COEF;

		/**
		    Get Display Engine Horizontal Low Pass Filter Enable/Disable

		    This structure is used for display control operation code "DISPCTRL_GET_HLPF_EN"
		    to get the display Horizontal Low Pass Filter Enable/Disable. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_HLPF_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.SET_HLPF_EN.b_en"
		    is the Display Engine Horizontal Low Pass Filter Enable/Disable.
		*/
		struct { // DISPCTRL_GET_HLPF_EN
			BOOL            b_en;            ///< TRUE: Enable horizontal LPF.  FALSE: Disable horizontal LPF.
		} GET_HLPF_EN;

		/**
		    Get Display Engine Constant Window Offset

		    This structure is used for display control operation code "DISPCTRL_GET_CONST_OFS"
		    to get the display  Constant Window Offset. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_SET_CONST_OFS, &disp_ctrl);
		\n  "disp_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x"
		\n  "disp_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y"
		    is the Display Engine Constant Window Offset.
		*/
		struct { // DISPCTRL_GET_CONST_OFS
			UINT32          ui_win_const_ofs_x;    ///< The display Video/OSD layer output window constant offset X
			UINT32          ui_win_const_ofs_y;    ///< The display Video/OSD layer output window constant offset Y
		} GET_CONST_OFS;

		/**
		    Get Display Engine Clock Source

		    This structure is used for display control operation code "DISPCTRL_GET_SRCCLK"
		    to get the display Clock Source. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_SRCCLK, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_SRCCLK.src_clk"
		    is the display Clock Source information.
		*/
		struct { // DISPCTRL_GET_SRCCLK
			DISPCTRL_SRCCLK    src_clk;   ///< Get display source clock
		} GET_SRCCLK;

		/**
		    Get Display Engine Gamma Table Y

		    This structure is used for display control operation code "DISPCTRL_GET_GAMMA_Y"
		    to get the display gamma table Y. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_GAMMA_Y, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_GAMMA_Y.p_gamma_tab_y"
		    is the Display Engine Gamma Table Y
		*/
		struct { // DISPCTRL_GET_GAMMA_Y
			UINT8           *p_gamma_tab_y;    ///< The UINT8 entries gamma table for component Y.(for 220:17 entries)
		} GET_GAMMA_Y;

		/**
		    Get Display Engine Gamma Table RGB

		    This structure is used for display control operation code "DISPCTRL_GET_GAMMA_RGB"
		    to get the display gamma table RGB. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_GAMMA_RGB, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_GAMMA_RGB.p_gamma_tab_r"
		\n  "disp_ctrl.SEL.GET_GAMMA_RGB.p_gamma_tab_g"
		\n  "disp_ctrl.SEL.GET_GAMMA_RGB.p_gamma_tab_b"
		    is the Display Engine Gamma Table R
		*/
		struct { // DISPCTRL_GET_GAMMA_RGB
			UINT8           *p_gamma_tab_r;    ///< The UINT8 entries gamma table for component R.(for 650:33 entries)
			UINT8           *p_gamma_tab_g;    ///< The UINT8 entries gamma table for component G.(for 650:33 entries)
			UINT8           *p_gamma_tab_b;    ///< The UINT8 entries gamma table for component B.(for 650:33 entries)
		} GET_GAMMA_RGB;


		/**
		    Get Display Engine ICST0 enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_ICST0_EN"
		    to get the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_ICST0_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ICST0_EN.b_en "
		   is the Display Engine ICST0
		*/
		struct { // DISPCTRL_GET_ICST0_EN
			BOOL            b_en;            ///< The ICST0 enable/disable.
		} GET_ICST0_EN;


		/**
		    Get Display Engine ICST0 Coeficients

		    This structure is used for display control operation code "DISPCTRL_GET_ICST0_COEF"
		    to get the display Inverse color Space Transform Coeficients. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_ICST0_COEF, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.pi_coef "
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.i_pre_ofs_y"
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.i_pre_ofs_cb"
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.i_pre_ofs_cr"
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.i_post_ofs_y"
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.i_post_ofs_cb"
		\n  "disp_ctrl.SEL.GET_ICST0_COEF.i_post_ofs_cr"
		    is the Display Engine ICST0 Coeficients.
		*/
		struct { // DISPCTRL_GET_ICST0_COEF
			INT32          *pi_coef;         ///< The 3X3 ICST Matrix coefficients. The coefficient format is (Q3.8). Valid range from -2048 to 2047.
			///< The coefficients in order are: Ry, Ru, Rv, Gy, Gu, Gv, By, Bu, Bv.

			INT16           i_pre_ofs_y;       ///< Inverse color space transform Pre-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_pre_ofs_cb;      ///< Inverse color space transform Pre-Offset value for Cb. Valid range from -128 to 127.
			INT16           i_pre_ofs_cr;      ///< Inverse color space transform Pre-Offset value for Cr. Valid range from -128 to 127.

			INT16           i_post_ofs_y;      ///< Inverse color space transform Post-Offset value for Y.  Valid range from -128 to 127.
			INT16           i_post_ofs_cb;     ///< Inverse color space transform Post-Offset value for Cb. Valid range from -128 to 127.
			INT16           i_post_ofs_cr;     ///< Inverse color space transform Post-Offset value for Cr. Valid range from -128 to 127.
		} GET_ICST0_COEF;


		/**
		    Get Display Engine CST

		    This structure is used for display control operation code "DISPCTRL_SET_CST_EN"
		    to get the display Inverse color Space Transform configurations. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CST_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CST_EN.b_en"
		   is the Display Engine CST
		*/
		struct { // DISPCTRL_GET_CST_EN
			BOOL            b_en;            ///< The CST  enable/disable.
		} GET_CST_EN;

		/**
		    Get Display Engine color control enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CC_EN"
		    to get the display color control adjustment. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_EN.b_en"
		   is the Display Engine color control enable/disable
		*/
		struct { // DISPCTRL_GET_CC_EN
			BOOL            b_en;            ///< The CC  enable/disable.
		} GET_CC_EN;

		/**
		    Get Display Engine color control adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_EN"
		    to get the display color control adjustment. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_EN.b_en"
		    is the color control adjustment enable/disable
		*/
		struct { // DISPCTRL_GET_CCA_EN
			BOOL            b_en;            ///< The CCA  enable/disable.
		} GET_CCA_EN;


		/**
		    Get Display Engine color control Hue adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_HUE_EN"
		    to get the display color control Hue adjustment . This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_HUE_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_HUE_EN.b_en"
		    is the Display Engine color control Hue adjustment enable/disable
		*/
		struct { // DISPCTRL_GET_CCA_HUE_EN
			BOOL            b_en;            ///< The CCA HUE  enable/disable.
		} GET_CCA_HUE_EN;

		/**
		    Get Display Engine color control Y contrast adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_YCON_EN"
		    to get the display color control Y contrast adjustment . This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_YCON_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_YCON_EN.b_en"
		    is the Display Engine color control Y contrast adjustment enable/disable
		*/
		struct { // DISPCTRL_GET_CCA_YCON_EN
			BOOL            b_en;            ///< The CCA YCON  enable/disable.
		} GET_CCA_YCON_EN;

		/**
		    Get Display Engine color control C contrast adjustment enable/disable

		    This structure is used for display control operation code "DISPCTRL_SET_CCA_CCON_EN"
		    to get the display color control C contrast adjustment . This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_CCON_EN, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_CCON_EN.b_en"
		    is the Display Engine color control C contrast adjustment enable/disable
		*/
		struct { // DISPCTRL_GET_CCA_CCON_EN
			BOOL            b_en;            ///< The CCA CCON  enable/disable.
		} GET_CCA_CCON_EN;

		/**
		    Get Display Engine color control Hue table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_HUE"
		    to get the display color control  Hue table. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_HUE, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_HUE.p_hue_tab"
		    is the Display Engine color control Hue table
		*/
		struct { // DISPCTRL_GET_CC_HUE
			UINT8           *p_hue_tab;    ///< The UINT8 entries hue table.(for 650:24 entries)
		} GET_CC_HUE;


		/**
		    Get Display Engine color control Intensity table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_INT"
		    to get the display color control  Intensity table. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_INT, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_INT.p_int_tab"
		    is the Display Engine color control Intensity table
		*/
		struct { // DISPCTRL_GET_CC_INT
			INT8            *p_int_tab;    ///< The INT8 entries intensity table.(for 650:24 entries)
		} GET_CC_INT;

		/**
		    Get Display Engine color control Saturation table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_SAT"
		    to get the display color control  Saturation table. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_SAT, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_SAT.p_sat_tab"
		    is the Display Engine color control Saturation table
		*/
		struct { // DISPCTRL_GET_CC_SAT
			INT8            *p_sat_tab;    ///< The INT8 entries saturation table.(for 650:24 entries)
		} GET_CC_SAT;

		/**
		    Get Display Engine color control DDS table

		    This structure is used for display control operation code "DISPCTRL_SET_CC_DDS"
		    to get the display color control  DDS table. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_DDS, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_DDS.p_dds_tab"
		    is the Display Engine color control DDS table
		*/
		struct { // DISPCTRL_GET_CC_DDS
			UINT8           *p_dds_tab;    ///< The INT8 entries DDS table.(for 650:8 entries)
		} GET_CC_DDS;


		/**
		    Get Display Engine color control Intensity offset

		    This structure is used for display control operation code "DISPCTRL_SET_CC_INT_OFS"
		    to get the display color control   Intensity offset. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_INT_OFS, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_INT_OFS.iintofs"
		    is the Display Engine color control Intensity offset
		*/
		struct { // DISPCTRL_GET_CC_INT_OFS
			INT8             iintofs;    ///< The intensity offset, INT8(-128~127)
		} GET_CC_INT_OFS;

		/**
		    Get Display Engine color control Saturation offset

		    This structure is used for display control operation code "DISPCTRL_SET_CC_SAT_OFS"
		    to get the display color control   Saturation offset. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CC_SAT_OFS, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CC_SAT_OFS.isatofs"
		    is the Display Engine color control Saturation offset
		*/
		struct { // DISPCTRL_GET_CC_SAT_OFS
			INT8             isatofs;    ///< The Saturation offset, INT8(-128~127)
		} GET_CC_SAT_OFS;

		/**
		   Get Display Engine color component adjustment Y contrast

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_YCON"
		   to get the display color component adjustment Y contrast. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_YCON, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_YCON.uiycon"
		   is the Display Engine color component adjustment Y contrast
		*/
		struct { // DISPCTRL_GET_CCA_YCON
			UINT8            uiycon;    ///< The Y contrast (0~255)
		} GET_CCA_YCON;

		/**
		   Get Display Engine color component adjustment Cb/Cr contrast

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_CCON"
		   to get the display color component adjustment Y contrast. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_CCON, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_CCON.uiccon"
		   is the Display Engine color component adjustment Cb/Cr contrast
		*/
		struct { // DISPCTRL_GET_CCA_CCON
			UINT8            uiccon;    ///< The C contrast (0~255)
		} GET_CCA_CCON;

		/**
		   Get Display Engine color component adjustment Y offset

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_YOFS"
		   to get the display color component adjustment Y offset. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_YOFS, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_YOFS.iyofs"
		   is the Display Engine color component adjustment Y offset
		*/
		struct { // DISPCTRL_GET_CCA_YOFS
			INT8              iyofs;    ///< The Y Ofs (-128~127)
		} GET_CCA_YOFS;

		/**
		   Get Display Engine color component adjustment Cb/Cr offset

		   This structure is used for display control operation code "DISPCTRL_SET_CCA_COFS"
		   to get the display color component adjustment Y contrast. This structure shuld be used like this:
		\n  pDispObj->disp_ctrl(DISPCTRL_GET_CCA_COFS, &disp_ctrl);
		\n  "disp_ctrl.SEL.GET_CCA_COFS.uicbofs"
		\n  "disp_ctrl.SEL.GET_CCA_COFS.uicrofs"
		    is the Display Engine color component adjustment Cb/Cr offset
		*/
		struct { // DISPCTRL_GET_CCA_COFS
			UINT8            uicbofs;    ///< The Cb offset (0~255)
			UINT8            uicrofs;    ///< The Cr offset (0~255)
		} GET_CCA_COFS;


	} SEL;
} DISPCTRL_PARAM, *PDISPCTRL_PARAM;
//@}



typedef ER(*DISPCTRL)(DISPCTRL_OP, PDISPCTRL_PARAM);


#endif
//@}