/*
    Display object object panel configuration parameters for AUCN01

    @file       AUCN01.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "dispdev_if8bits.h"

/*
    Panel Parameters for AUCN01
*/
//@{

/*Used in RGB-Serial & RGB Through*/
const T_PANEL_CMD t_cmd_mode_through[] = {
	{0x00, 0x07},
	{0x00, 0x0F},
	{0x40, 0x03},
	{0x60, 0x00},
	{0xC0, 0x06},
};

/*Used in RGB-Dummy 320*/
const T_PANEL_CMD t_cmd_mode_rgbd320[] = {
	{0x00, 0x0F},
	{0x40, 0x03},
	{0x60, 0x01},
	{0xC0, 0x06},
	{0xE0, 0x13},
};

/*Used in YUV640 & CCIR601*/
const T_PANEL_CMD t_cmd_mode_yuv640[] = {
	{0x00, 0x0F},
	{0x20, 0x03},
	{0x40, 0x03},
	{0x60, 0x03},
	{0xC0, 0x06},
};

/*Used in CCIR656*/
const T_PANEL_CMD t_cmd_mode_ccir656[] = {
	{0x00, 0x0F},
	{0x40, 0x03},
	{0x60, 0x07},
	{0xC0, 0x06},
};

const T_PANEL_CMD t_cmd_standby[] = {
	{0x00, 0x08}
};

const T_LCD_SIF_PARAM t_sif_param[] = {
	{
	// SIF mode 00
	0,
	//SIF_MODE_00,
	// Bus clock maximun = 20 MHz
	3000000,
	// SENS = 0
	20,
	// SENH = 30
	0,
	// 24 bits per transmission
	16,
	// MSb shift out first
	SIF_DIR_MSB,
	}
};

const T_LCD_PARAM t_mode[] = {
	/***********       CCIR601 MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_CCIR601,         //!< LCDMode
#if (_FPGA_EMULATION_ == ENABLE)
			//(_FPGA_PLL_OSC_ / 4000000),     //!< fDCLK
			10,
#else
			24550000,                       //!< fDCLK
#endif
			780,                           //!< uiHSyncTotalPeriod
			640,                           //!< uiHSyncActivePeriod
			126,                            //!< uiHSyncBackPorch
			262,                            //!< uiVSyncTotalPeriod
			240,                            //!< uiVSyncActivePeriod
			13,                             //!< uiVSyncBackPorchOdd
			13,                             //!< uiVSyncBackPorchEven
			640,                            //!< uiBufferWidth
			240,                            //!< uiBufferHeight
			640,                            //!< uiWindowWidth
			240,                            //!< uiWindowHeight
			TRUE,                           //!< bYCbCrFormat(YCC 8bit)

			/* New added parameters */
			0x00,                           //!< uiHSyncSYNCwidth
			0x00                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_CCIR601,         //!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
			IDE_LCD_G,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                           //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< YCex
			FALSE,                          //!< HLPF
			{FALSE, FALSE, FALSE},          //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_yuv640,                 //!< pCmdQueue
		sizeof(t_cmd_mode_yuv640) / sizeof(T_PANEL_CMD),  //!< nCmd
	},

	/***********       RGB Dummy-320 MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_RGBD320,         //!< LCDMode
#if (_FPGA_EMULATION_ == ENABLE)
			12.0f,                          //!< fDCLK
#else
			9000000,
#endif
			448,                            //!< uiHSyncTotalPeriod
			320,                            //!< uiHSyncActivePeriod
			63,                             //!< uiHSyncBackPorch
			262,                            //!< uiVSyncTotalPeriod
			240,                            //!< uiVSyncActivePeriod
			12,                             //!< uiVSyncBackPorchOdd
			12,                             //!< uiVSyncBackPorchEven
			320,                            //!< uiBufferWidth
			240,                            //!< uiBufferHeight
			320,                            //!< uiWindowWidth
			240,                            //!< uiWindowHeight
			FALSE,                          //!< bYCbCrFormat

			/* New added parameters */
			0x00,                           //!< uiHSyncSYNCwidth
			0x01                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_SERIAL_RGB_8BITS,//!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_B,                      //!< odd;
			IDE_LCD_B,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			FALSE,                          //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< YCex
			FALSE,                          //!< HLPF
			{FALSE, FALSE, FALSE},          //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_rgbd320,                 //!< pCmdQueue
		sizeof(t_cmd_mode_rgbd320) / sizeof(T_PANEL_CMD),  //!< nCmd
	},

	/***********       RGB-SERIAL MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_RGB_SERIAL,      //!< LCDMode
#if (_FPGA_EMULATION_ == ENABLE)
			8.0f,                           //!< fDCLK
#else
			//9.7f,                           //!< fDCLK
			15000000,
#endif
			616,                            //!< uiHSyncTotalPeriod
			480,                            //!< uiHSyncActivePeriod
			100,                            //!< uiHSyncBackPorch
			262,                            //!< uiVSyncTotalPeriod
			240,                            //!< uiVSyncActivePeriod
			18,                             //!< uiVSyncBackPorchOdd
			18,                             //!< uiVSyncBackPorchEven
			480,                            //!< uiBufferWidth
			240,                            //!< uiBufferHeight
			480,                            //!< uiWindowWidth
			240,                            //!< uiWindowHeight
			FALSE,                          //!< bYCbCrFormat

			/* New added parameters */
			0x00,                           //!< uiHSyncSYNCwidth
			0x05                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_SERIAL_RGB_8BITS,//!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
			IDE_LCD_G,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                           //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< YCex
			FALSE,                          //!< HLPF
			{TRUE, TRUE, TRUE},             //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_through,                 //!< pCmdQueue
		sizeof(t_cmd_mode_through) / sizeof(T_PANEL_CMD),  //!< nCmd
	},

	/***********       THROUGH MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_RGB_THROUGH,     //!< LCDMode
#if (_FPGA_EMULATION_ == ENABLE)
			8.0f,                           //!< fDCLK
#else
			6600000,                          	//!< fDCLK: IDE clock = 1/3X out I/F clock
#endif
			228,                            //!< uiHSyncTotalPeriod
			160,                            //!< uiHSyncActivePeriod
			33,                             //!< uiHSyncBackPorch
			254,                            //!< uiVSyncTotalPeriod
			240,                            //!< uiVSyncActivePeriod
			12,                             //!< uiVSyncBackPorchOdd
			12,                             //!< uiVSyncBackPorchEven
			160,                            //!< uiBufferWidth
			240,                            //!< uiBufferHeight
			160,                            //!< uiWindowWidth
			240,                            //!< uiWindowHeight
			FALSE,                          //!< bYCbCrFormat

			/* New added parameters */
			0x00,                           //!< uiHSyncSYNCwidth
			0x05                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_SERIAL_RGB_8BITS,//!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_B,                      //!< odd;
			IDE_LCD_R,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                          //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< YCex
			FALSE,                          //!< HLPF
			{FALSE, FALSE, FALSE},          //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_through,                 //!< pCmdQueue
		sizeof(t_cmd_mode_through) / sizeof(T_PANEL_CMD),  //!< nCmd
	},

	/***********       CCIR656 MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_CCIR656,         //!< LCDMode
#if _FPGA_EMULATION_ == ENABLE
			12.0f,                           //!< fDCLK
#else
			13500000,                          //!< fDCLK
#endif
			1716,                           //!< uiHSyncTotalPeriod
			1440,                           //!< uiHSyncActivePeriod
			273,                            //!< uiHSyncBackPorch
			525,                            //!< uiVSyncTotalPeriod
			480,                            //!< uiVSyncActivePeriod
			36,                             //!< uiVSyncBackPorchOdd
			35,                             //!< uiVSyncBackPorchEven
			720,                            //!< uiBufferWidth
			480,                            //!< uiBufferHeight
			720,                            //!< uiWindowWidth
			480,                            //!< uiWindowHeight
			TRUE,                           //!< bYCbCrFormat

			/* New added parameters */
			0x00,                           //!< uiHSyncSYNCwidth
			0x05                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_CCIR656 | PINMUX_LCD_SEL_DE_ENABLE, //!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
			IDE_LCD_G,                      //!< even;
			FALSE,                          //!< hsinv;
			FALSE,                          //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                          //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			2,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< YCex
			FALSE,                          //!< HLPF
			{FALSE, FALSE, FALSE},          //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_ccir656,                 //!< pCmdQueue
		sizeof(t_cmd_mode_ccir656) / sizeof(T_PANEL_CMD),  //!< nCmd
	},


	/***********       YUV640 MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_YUV640,          //!< LCDMode
#if (_EMULATION_ == ENABLE)
#if (_FPGA_EMULATION_ == DISABLE)
			// RealChip emulation (Including MT)
			//27.0f,                          //!< fDCLK
			//24.55,
			12.6,
#else
			// FPGA (Recude DMA bandwidth)
			//(_FPGA_PLL_OSC_ / 4000000),     //!< fDCLK,
			10,
#endif
#else
			24550000,                         //!< fDCLK
#endif
			780,                           //!< uiHSyncTotalPeriod
			640,                           //!< uiHSyncActivePeriod
			126,                            //!< uiHSyncBackPorch
			262,                            //!< uiVSyncTotalPeriod
			240,                            //!< uiVSyncActivePeriod
			13,                             //!< uiVSyncBackPorchOdd
			13,                             //!< uiVSyncBackPorchEven
			640,                            //!< uiBufferWidth
			240,                            //!< uiBufferHeight
			640,                            //!< uiWindowWidth
			240,                            //!< uiWindowHeight
			TRUE,                          //!< bYCbCrFormat

			/* New added parameters */
			0x00,                           //!< uiHSyncSYNCwidth
			0x00                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_SERIAL_YCbCr_8BITS,//!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
			IDE_LCD_G,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                          //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< YCex
			FALSE,                          //!< HLPF
			{FALSE, FALSE, FALSE},          //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_yuv640,                 //!< pCmdQueue
		sizeof(t_cmd_mode_yuv640) / sizeof(T_PANEL_CMD),  //!< nCmd
	}
};

const T_LCD_ROT *t_rotate = NULL;

//@}

T_LCD_ROT *dispdev_get_lcd_rotate_cmd(UINT32 *mode_number)
{
//	if (t_rotate != NULL) {
//		*mode_number = sizeof(t_rotate) / sizeof(T_LCD_ROT);
//	} else {
		*mode_number = 0;
//	}
	return (T_LCD_ROT *)t_rotate;
}

T_LCD_PARAM *dispdev_get_config_mode(UINT32 *mode_number)
{
	*mode_number = sizeof(t_mode) / sizeof(T_LCD_PARAM);
	return (T_LCD_PARAM *)t_mode;
}

T_PANEL_CMD *dispdev_get_standby_cmd(UINT32 *cmd_number)
{
	*cmd_number = sizeof(t_cmd_standby) / sizeof(T_PANEL_CMD);
	return (T_PANEL_CMD *)t_cmd_standby;
}

#if 1
void dispdev_write_to_lcd_sif(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value)
{
	UINT32                  ui_sif_data;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;

	ui_sif_data = (UINT32)addr << 24 | (UINT32)value << 16;

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	sif_send(dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, ui_sif_data, 0, 0);

}


void dispdev_write_to_lcd_gpio(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value)
{
	UINT32                  ui_sif_data, j;
	UINT32                  sif_clk, sif_sen, sif_data;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	sif_clk  = dev_io_ctrl.SEL.GET_REG_IF.ui_gpio_clk;
	sif_sen  = dev_io_ctrl.SEL.GET_REG_IF.ui_gpio_sen;
	sif_data = dev_io_ctrl.SEL.GET_REG_IF.ui_gpio_data;

	if (dispdev_platform_request_gpio(sif_clk, "clk")) {
		DBG_ERR("gpio%d requset err\r\n", (int)sif_clk);
	}
	if (dispdev_platform_request_gpio(sif_sen, "sen")) {
		DBG_ERR("gpio%d requset err\r\n", (int)sif_sen);
	}
	if (dispdev_platform_request_gpio(sif_data, "data")) {
		DBG_ERR("gpio%d requset err\r\n", (int)sif_data);
	}

	dispdev_platform_set_gpio_ouput(sif_sen, 1);
	dispdev_platform_set_gpio_ouput(sif_clk, 1);
	dispdev_platform_set_gpio_ouput(sif_data, 1);

	ui_sif_data = (UINT32)addr << 8 | (UINT32)value;

	dispdev_platform_delay_us(500);

	dispdev_platform_set_gpio_ouput(sif_sen, 0);

	for (j = 16; j > 0; j--) {
		if (((ui_sif_data >> (j - 1)) & 0x01)) {
			dispdev_platform_set_gpio_ouput(sif_data, 1);
		} else {
			dispdev_platform_set_gpio_ouput(sif_data, 0);
		}

		dispdev_platform_delay_us(100);
		dispdev_platform_set_gpio_ouput(sif_clk, 0);
		dispdev_platform_delay_us(200);
		dispdev_platform_set_gpio_ouput(sif_clk, 1);
		dispdev_platform_delay_us(100);
	}

	dispdev_platform_delay_us(500);
	dispdev_platform_set_gpio_ouput(sif_sen, 1);

	dispdev_platform_free_gpio(sif_clk);
	dispdev_platform_free_gpio(sif_sen);
	dispdev_platform_free_gpio(sif_data);

}

T_LCD_SIF_PARAM *dispdev_get_lcd_sif_param(void)
{
	return (T_LCD_SIF_PARAM *)t_sif_param;
}

void dispdev_read_from_lcd_gpio(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 *p_value)
{
	DBG_ERR("Has not implement dispdev_read_from_lcd_gpio \r\n");
}

#if defined __FREERTOS
int panel_init(void)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	int nodeoffset;
	uint32_t *cell = NULL;
	DISPDEV_PARAM   disp_dev;
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	p_disp_obj->open();

    if(fdt_addr == NULL)
		return -1;


	nodeoffset = fdt_path_offset((const void*)fdt_addr, "/display");
	if (nodeoffset >= 0) {

		cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "lcd_ctrl", NULL);
		if (cell > 0) {
			if (be32_to_cpu(cell[0]) == 0) {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_SIF;
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = 0;

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "sif_channel", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_sif_ch = (SIF_CH)(be32_to_cpu(cell[0]));
					p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);
				} else {
					DBG_WRN("panel dtsi sif_channel is not set!\r\n");
				}
			} else {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_GPIO;
				disp_dev.SEL.SET_REG_IF.ui_sif_ch = 0;

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gpio_cs", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_gpio_sen = be32_to_cpu(cell[0]);
				} else {
					DBG_WRN("panel dtsi gpio_cs is not set!\r\n");
				}

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gpio_clk", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_gpio_clk = be32_to_cpu(cell[0]);
				} else {
					DBG_WRN("panel dtsi gpio_clk is not set!\r\n");
				}

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gpio_data", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_gpio_data = be32_to_cpu(cell[0]);
				} else {
					DBG_WRN("panel dtsi gpio_data is not set!\r\n");
				}

				p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);

			}
		} else {
			DBG_WRN("panel dtsi lcd_ctrl is not set!\r\n");
		}
	} else {
		DBG_WRN("can not find display node!\r\n");
	}

	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
	p_disp_obj->close();
    DBG_DUMP("Hello, panel: AUCN01\r\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("AUCN01, Goodbye\r\n");
}
#elif defined __KERNEL__
static int __init panel_init(void)
{
	DISPDEV_PARAM   disp_dev;
	u32 gpio_cs;
	u32 gpio_ck;
	u32 gpio_dat;
	struct device_node *panel;
	u32 value;
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	p_disp_obj->open();

	panel = of_find_node_by_name(NULL, "display");
	if (panel) {
		if (!of_property_read_u32(panel, "lcd_ctrl", &value)) {
			if (value == 0) {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_SIF;
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = 0;
				if (!of_property_read_u32(panel, "sif_channel", &value)) {
					disp_dev.SEL.SET_REG_IF.ui_sif_ch = (SIF_CH)value;
					p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);
				} else {
					DBG_WRN("panel dtsi sif_channel is not set!\r\n");
				}
			} else {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_GPIO;
				if (!of_property_read_u32(panel, "gpio_cs", &gpio_cs)) {
				} else {
					DBG_WRN("panel dtsi gpio_cs is not set!\r\n");
				}
				if (!of_property_read_u32(panel, "gpio_clk", &gpio_ck)) {
				} else {
					DBG_WRN("panel dtsi gpio_clk is not set!\r\n");
				}
				if (!of_property_read_u32(panel, "gpio_data", &gpio_dat)) {
				} else {
					DBG_WRN("panel dtsi gpio_data is not set!\r\n");
				}

				disp_dev.SEL.SET_REG_IF.ui_sif_ch = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = gpio_cs;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = gpio_ck;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = gpio_dat;
				p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);

			}
		} else {
			DBG_WRN("panel dtsi lcd_ctrl is not set!\r\n");
		}
	} else {
		DBG_WRN("can not find display node!\r\n");
	}
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
	p_disp_obj->close();
    pr_info("Hello, panel: AUCN01\n");
    return 0;
}

static void __exit panel_exit(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);

	p_disp_obj->dev_callback = NULL;
    printk(KERN_INFO "Goodbye\n");
}

module_init(panel_init);
module_exit(panel_exit);

MODULE_DESCRIPTION("AUCN01 Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
#endif
#endif
