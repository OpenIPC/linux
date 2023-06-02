/*
    Display object object panel configuration parameters for WM08001_SN75LVDS83B

    @file      WM08001_SN75LVDS83B.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "dispdev_ifparal.h"

/*Used in RGB-Serial & RGB Through*/
const T_PANEL_CMD t_cmd_mode_rgb16_through[] = {
};

/*Used in RGB 16-bit da mode, we must trans stripe pattern */
const T_PANEL_CMD t_cmd_mode_rgb16_da[] = {
};

/*Used in YUV 16-bit 640 mode */
const T_PANEL_CMD t_cmd_mode_yuv16640[] = {
};

const T_PANEL_CMD t_cmd_standby_paral[] = {
	{0x01, 0x00}
};

const T_LCD_PARAM t_mode_paral[] = {
	/***********       RGB-PARALLEL MODE         *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			PINMUX_LCDMODE_RGB_PARALL,      //!< LCDMode
#if (_FPGA_EMULATION_ == ENABLE)
			12.0f,                          //!< fDCLK
#else
			60000000,                          //!< fDCLK
#endif
			1700,                           //!< uiHSyncTotalPeriod
			1600,                           //!< uiHSyncActivePeriod
			36,                             //!< uiHSyncBackPorch
			580,                            //!< uiVSyncTotalPeriod
			480,                            //!< uiVSyncActivePeriod
			33,                             //!< uiVSyncBackPorchOdd
			33,                             //!< uiVSyncBackPorchEven
			1600,                            //!< uiBufferWidth
			480,                            //!< uiBufferHeight
			1600,                            //!< uiWindowWidth
			480,                            //!< uiWindowHeight
			FALSE,                          //!< bYCbCrFormat

			/* New added parameters */
			0x02,                           //!< uiHSyncSYNCwidth
			0x02                            //!< uiVSyncSYNCwidth
		},

		// tIDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_PARALLE_RGB888 | PINMUX_LCD_SEL_DE_ENABLE, //!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                 //!< dithering[2];
			DISPLAY_DEVICE_AU,              //!< **DONT-CARE**
			IDE_PDIR_RBG,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
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
			{TRUE, TRUE, TRUE},             //!< subpixOdd[3]
			{FALSE, FALSE, FALSE},          //!< subpixEven[3]
			{IDE_DITHER_6BITS, IDE_DITHER_6BITS, IDE_DITHER_6BITS}, //!< DitherBits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_standby_paral,                 //!< pCmdQueue
		sizeof(t_cmd_standby_paral) / sizeof(T_PANEL_CMD),  //!< nCmd
	},
};

const T_LCD_ROT *t_rotate_paral = NULL;

//@}

T_LCD_ROT *dispdev_get_lcd_rotate_cmd_paral(UINT32 *mode_number)
{
//	if (t_rotate_paral != NULL) {
//		*mode_number = sizeof(t_rotate_paral) / sizeof(T_LCD_ROT);
//	} else {
		*mode_number = 0;
//	}
	return (T_LCD_ROT *)t_rotate_paral;
}

T_LCD_PARAM *dispdev_get_config_mode_paral(UINT32 *mode_number)
{
	*mode_number = sizeof(t_mode_paral) / sizeof(T_LCD_PARAM);
	return (T_LCD_PARAM *)t_mode_paral;
}

T_PANEL_CMD *dispdev_get_standby_cmd_paral(UINT32 *cmd_number)
{
	*cmd_number = sizeof(t_cmd_standby_paral) / sizeof(T_PANEL_CMD);
	return (T_PANEL_CMD *)t_cmd_standby_paral;
}

void dispdev_write_to_lcd_sif_paral(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value)
{
	UINT32                  ui_sif_data;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;

	ui_sif_data = ((((UINT32)addr << 9) | (UINT32)value) << 16);

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	sif_send(dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, ui_sif_data, 0, 0);

}

void dispdev_write_to_lcd_gpio_paral(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value)
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

void dispdev_read_from_lcd_gpio_paral(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 *p_value)
{
	DBG_ERR("Has not implement dispdev_read_from_lcd_gpio_paral \r\n");
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
    DBG_DUMP("Hello, panel: WM08001 SN75LVDS83B\r\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("WM08001 SN75LVDS83B, Goodbye\r\n");
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
    pr_info("Hello, panel: WM08001 SN75LVDS83B\n");
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

MODULE_DESCRIPTION("WM08001 SN75LVDS83B Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
#endif
