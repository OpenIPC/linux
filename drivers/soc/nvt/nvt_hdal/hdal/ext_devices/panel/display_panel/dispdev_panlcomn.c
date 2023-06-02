/*
    panel device open/close Common control.

    @file       dispdev_panlcomn.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_panlcomn.h"


/*
    Set Display open

    Open the specified display engine with the specified engine parameters.
*/
ER dispdev_set_display(DISPDEV_IOCTRL p_disp_dev_control, T_LCD_INF lcd_inf, T_LCD_PARAM *p_mode, PINMUX_FUNC_ID pin_func_id)
{
	UINT32                  pinmux_value;
	T_IDE_PARAM             *p_ide    = &p_mode->ide;
	T_PANEL_PARAM           *p_panel  = &p_mode->panel;
	//UINT32                  ui_src_clk_freq;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	BOOL                    b_ycc8bit;

	//DBG_IND("dispdev_set_display START 1 [0x%08x] 0x%08x\r\n", p_ide->pinmux_select_lcd, *(UINT32 *)0xF0010008);

	dispdev_platform_set_pinmux(pin_func_id, p_ide->pinmux_select_lcd);

	//DBG_IND("dispdev_set_display START 2 [0x%08x] 0x%08x\r\n", p_ide->pinmux_select_lcd, *(UINT32 *)0xF0010008);

	p_disp_dev_control(DISPDEV_IOCTRL_GET_SRCCLK, &dev_io_ctrl);
	/*if( dev_io_ctrl.SEL.GET_SRCCLK.src_clk == DISPCTRL_SRCCLK_MI_ENGINE)
	{
	    // This path is for MI engine DRAM direct path display object
	    dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width     = p_panel->ui_buffer_width;
	    dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height    = p_panel->ui_buffer_height;
	    dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width     = p_panel->ui_window_width;
	    dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height    = p_panel->ui_window_height;
	    p_disp_dev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);
	    return E_OK;
	}*/

	// Make sure ide Disable
	p_disp_dev_control(DISPDEV_IOCTRL_GET_ENABLE, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_ENABLE.b_en == TRUE) {
		dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
		p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
		p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
	}

	if (lcd_inf == T_LCD_INF_SERIAL_8BITS) {
		switch (p_panel->lcd_mode) {
		case PINMUX_LCDMODE_RGB_SERIAL:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_SERIAL;
			break;
		case PINMUX_LCDMODE_YUV640:
		case PINMUX_LCDMODE_YUV720:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_YUV;
			break;
		case PINMUX_LCDMODE_RGBD360:
		case PINMUX_LCDMODE_RGBD320:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGBD;
			break;
		case PINMUX_LCDMODE_RGB_THROUGH:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_THROUGH;
			break;
		case PINMUX_LCDMODE_CCIR601:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_CCIR601_8BIT;
			break;
		case PINMUX_LCDMODE_CCIR656:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_CCIR656_8BIT;
			break;
		default:
			DBG_ERR("Invalid lcd_mode %d lcd_inf %d\r\n", (int)p_panel->lcd_mode, (int)lcd_inf);
			return E_NOSPT;
		}
		p_disp_dev_control(DISPDEV_IOCTRL_SET_DEVICE, &dev_io_ctrl);

	} else if (lcd_inf == T_LCD_INF_MI) {
		if ((p_panel->lcd_mode >= PINMUX_LCDMODE_MI_FMT0) && (p_panel->lcd_mode <= PINMUX_LCDMODE_MI_SERIAL_SEP)) {
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_MI;
			p_disp_dev_control(DISPDEV_IOCTRL_SET_DEVICE, &dev_io_ctrl);
		} else {
			DBG_ERR("Invalid lcd_mode %d lcd_inf %d\r\n", (int)p_panel->lcd_mode, (int)lcd_inf);
		}
	} else if (lcd_inf == T_LCD_INF_MIPI_DSI) {
		dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_EMBD_MIPIDSI;
		p_disp_dev_control(DISPDEV_IOCTRL_SET_DEVICE, &dev_io_ctrl);
	} else if (lcd_inf == T_LCD_INF_PARALLEL_16BITS) {
		switch (p_panel->lcd_mode) {
		case PINMUX_LCDMODE_RGB_PARALL:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_PARALL;
			break;
		case PINMUX_LCDMODE_RGB_PARALL666:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_PARALL;
			break;
		case PINMUX_LCDMODE_RGB_PARALL565:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_PARALL;
			break;
		case PINMUX_LCDMODE_RGB_PARALL_DELTA:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGBDELTA_16BIT;
			break;
		case PINMUX_LCDMODE_CCIR601:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_CCIR601_16BIT;
			break;
		case PINMUX_LCDMODE_CCIR656:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_CCIR656_16BIT;
			break;
		case PINMUX_LCDMODE_RGB_THROUGH:
			dev_io_ctrl.SEL.SET_DEVICE.disp_dev_type = DISPDEV_TYPE_RGB_THROUGH;
			break;
		default:
			DBG_ERR("Invalid lcd_mode %d lcd_inf %d\r\n", (int)p_panel->lcd_mode, (int)lcd_inf);
			return E_NOSPT;
		}
		p_disp_dev_control(DISPDEV_IOCTRL_SET_DEVICE, &dev_io_ctrl);

	} else {
		DBG_ERR("lcd_inf Has not implemented!(%d)\r\n", (int)lcd_inf);
		return E_NOSPT;
	}

	DBG_IND("YCbCrFormat %d pinmux_select_lcd = 0x%08x\r\n", (int)p_panel->b_ycbcr_format, (unsigned int)p_ide->pinmux_select_lcd);

	if (p_panel->b_ycbcr_format) {
		pinmux_value = p_ide->pinmux_select_lcd & ~(PINMUX_LCD_SEL_FEATURE_MSK);
		if ((pinmux_value == PINMUX_LCD_SEL_CCIR601_16BITS) || (pinmux_value == PINMUX_LCD_SEL_CCIR656_16BITS)) {
			b_ycc8bit = FALSE;
		} else {
			b_ycc8bit = TRUE;
		}
		// set ICST
		dev_io_ctrl.SEL.SET_ICST_EN.b_en = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_disp_dev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);
	} else {
		b_ycc8bit = FALSE;
		// set ICST
		dev_io_ctrl.SEL.SET_ICST_EN.b_en = TRUE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_YCBCR2RGB;
		p_disp_dev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);
	}

	// set clock rate
	DBG_IND("set clock rate = %d Hz\r\n", (int)p_panel->fd_clk);
	dev_io_ctrl.SEL.SET_CLK_FREQ.ui_freq = p_panel->fd_clk;
	dev_io_ctrl.SEL.SET_CLK_FREQ.b_ycc8bit = b_ycc8bit;
	DBG_IND("dev_io_ctrl.SEL.SET_CLK_FREQ.b_ycc8bit = %d\r\n", (int)dev_io_ctrl.SEL.SET_CLK_FREQ.b_ycc8bit);
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_FREQ, &dev_io_ctrl);

	// enable clock
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = TRUE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_SRGB_OUTORDER.pix_order     = p_ide->pdir;
	dev_io_ctrl.SEL.SET_SRGB_OUTORDER.odd_start     = p_ide->odd;
	dev_io_ctrl.SEL.SET_SRGB_OUTORDER.even_start    = p_ide->even;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_SRGB_OUTORDER, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_DITHER_EN.b_en      = p_ide->dithering[0];
	dev_io_ctrl.SEL.SET_DITHER_EN.b_free_run = p_ide->dithering[1];
	dev_io_ctrl.SEL.SET_DITHER_EN.r_bits    = p_ide->dither_bits[0];
	dev_io_ctrl.SEL.SET_DITHER_EN.g_bits    = p_ide->dither_bits[1];
	dev_io_ctrl.SEL.SET_DITHER_EN.b_bits    = p_ide->dither_bits[2];
	p_disp_dev_control(DISPDEV_IOCTRL_SET_DITHER_EN, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_SYNC_INVERT.b_hs_inv  = p_ide->hsinv;
	dev_io_ctrl.SEL.SET_SYNC_INVERT.b_vs_inv  = p_ide->vsinv;
	dev_io_ctrl.SEL.SET_SYNC_INVERT.b_clk_inv = p_ide->clkinv;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_SYNC_INVERT, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_VLD_INVERT.b_hvld_inv    = p_ide->hvldinv;
	dev_io_ctrl.SEL.SET_VLD_INVERT.b_vvld_inv    = p_ide->vvldinv;
	dev_io_ctrl.SEL.SET_VLD_INVERT.b_field_inv   = p_ide->fieldinv;
	dev_io_ctrl.SEL.SET_VLD_INVERT.b_de_inv      = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_VLD_INVERT, &dev_io_ctrl);


	if (p_panel->b_ycbcr_format) {
		if ((p_ide->pinmux_select_lcd == PINMUX_LCD_SEL_CCIR601_16BITS) || (p_ide->pinmux_select_lcd == PINMUX_LCD_SEL_CCIR656_16BITS)) {
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hsync        = p_panel->ui_hsync_sync_width;
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_htotal       = p_panel->ui_hsync_total_period - 1;
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_start    = p_panel->ui_hsync_back_porch;
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_end      = (p_panel->ui_hsync_active_period + p_panel->ui_hsync_back_porch) - 1;
			p_disp_dev_control(DISPDEV_IOCTRL_SET_WINDOW_H_TIMING, &dev_io_ctrl);
		} else {
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hsync        = p_panel->ui_hsync_sync_width;
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_htotal       = (p_panel->ui_hsync_total_period >> 1) - 1;
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_start    = p_panel->ui_hsync_back_porch >> 1;
			dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_end      = ((p_panel->ui_hsync_active_period + p_panel->ui_hsync_back_porch) >> 1) - 1;
			p_disp_dev_control(DISPDEV_IOCTRL_SET_WINDOW_H_TIMING, &dev_io_ctrl);
		}

		dev_io_ctrl.SEL.SET_CLK1_2.b_clk1_2         = FALSE;
		p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK1_2, &dev_io_ctrl);
	} else {
		dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hsync        = p_panel->ui_hsync_sync_width;
		dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_htotal       = p_panel->ui_hsync_total_period - 1;
		dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_start    = p_panel->ui_hsync_back_porch;
		dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_end      = (p_panel->ui_hsync_active_period + p_panel->ui_hsync_back_porch) - 1;
		p_disp_dev_control(DISPDEV_IOCTRL_SET_WINDOW_H_TIMING, &dev_io_ctrl);

		if ((p_ide->pinmux_select_lcd == PINMUX_LCD_SEL_RGB_16BITS)) {
			dev_io_ctrl.SEL.SET_CLK1_2.b_clk1_2         = p_ide->clk1_2;
			p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK1_2, &dev_io_ctrl);
		} else {
			dev_io_ctrl.SEL.SET_CLK1_2.b_clk1_2         = FALSE;
			p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK1_2, &dev_io_ctrl);
		}
	}


	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vsync        = p_panel->ui_vsync_sync_width;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vtotal       = p_panel->ui_vsync_total_period - 1;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_odd_start = p_panel->ui_vsync_back_porch_odd;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_odd_end   = (p_panel->ui_vsync_active_period + p_panel->ui_vsync_back_porch_odd) - 1;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_even_start = p_panel->ui_vsync_back_porch_even;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_even_end  = (p_panel->ui_vsync_active_period + p_panel->ui_vsync_back_porch_even) - 1;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_WINDOW_V_TIMING, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_WINDOW_OUT_TYPE.b_interlaced = p_ide->interlace;
	dev_io_ctrl.SEL.SET_WINDOW_OUT_TYPE.b_field_odd_st = TRUE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_WINDOW_OUT_TYPE, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_CSB_EN.b_en             = TRUE;
	dev_io_ctrl.SEL.SET_CSB_EN.ui_contrast      = p_ide->ctrst;
	dev_io_ctrl.SEL.SET_CSB_EN.ui_saturation    = p_ide->cmults;
	dev_io_ctrl.SEL.SET_CSB_EN.ui_brightness    = p_ide->brt;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CSB_EN, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_YC_EXCHG.b_cbcr_exchg    = p_ide->cex;
	dev_io_ctrl.SEL.SET_YC_EXCHG.b_yc_exchg      = p_ide->yc_ex;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_YC_EXCHG, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_CLAMP.ui_clamp          = p_ide->clamp;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLAMP, &dev_io_ctrl);

	//ide_set_tv_power_down(p_ide->tv_powerdown);

	dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width     = p_panel->ui_buffer_width;
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height    = p_panel->ui_buffer_height;
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width     = p_panel->ui_window_width;
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height    = p_panel->ui_window_height;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_ENABLE.b_en = TRUE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	dispanl_debug(("dispdev_set_display Config Done\r\n"));
#if 0 //for speed up ,wait in kdrv_vddo_trigger 
	//#NT#2012/03/22#Steven Wang -begin
	//#NT#Temp add for DSI
	if (lcd_inf != T_LCD_INF_MIPI_DSI) {
		p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
	}
	//#NT#2012/03/22#Temp add for DSI
#endif
	dispanl_debug(("dispdev_set_display ide GO\r\n"));
	return E_OK;
}
#if defined __KERNEL__
EXPORT_SYMBOL(dispdev_set_display);
#endif
#if 0
/*
    Display close

    Close the specified display engine.
*/
ER dispdev_close_display(DISPDEV_IOCTRL p_disp_dev_control)
{
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;

	dispanl_debug(("DispDevClose\r\n"));

	dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

	/*
	// Disable ide
	if(p_devif->disp_id == DISP_1)
	{
	    p_devif->wait_frm_end();
	    pll_disableClock(IDE1_CLK);
	    pll_disableClock(IDE2_CLK);
	}
	else if (p_devif->disp_id == DISP_2)
	{
	    dispdev_platform_delay_ms(40);
	}
	*/

	return E_OK;
}
#endif

