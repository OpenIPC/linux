/*
    Display device object Golden Sample for driving RGB/YUV Paral panel

    @file       dispdev_gsinfParal.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_ifparal.h"

static void     disp_device_init(T_LCD_PARAM *p_mode);


static ER       dispdev_open_ifparal(void);
static ER       dispdev_close_ifparal(void);
static void     dispdev_write_lcd_reg(UINT32 ui_addr, UINT32 ui_value);
static UINT32   dispdev_read_lcd_reg(UINT32 ui_addr);
static void     dispdev_set_ifparal_ioctrl(FP p_io_ctrl_func);
static ER       dispdev_set_device_rotate(DISPDEV_LCD_ROTATE rot);
static ER       dispdev_get_lcd_size(PDISPDEV_GET_PRESIZE t_lcd_size);



/*
    Exporting display device object
*/
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
static DISPDEV_OBJ     disp_dev_ifparal_obj = { dispdev_open_ifparal, dispdev_close_ifparal, dispdev_set_device_rotate, dispdev_write_lcd_reg, dispdev_read_lcd_reg, dispdev_get_lcd_size, dispdev_set_ifparal_ioctrl, {NULL, NULL, NULL} };
static volatile DISPDEV_IOCTRL  p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
static DISPDEV_OBJ     disp_dev_ifparal_obj2 = { dispdev_open_ifparal, dispdev_close_ifparal, dispdev_set_device_rotate, dispdev_write_lcd_reg, dispdev_read_lcd_reg, dispdev_get_lcd_size, dispdev_set_ifparal_ioctrl, {NULL, NULL, NULL}};
static volatile DISPDEV_IOCTRL  p_if_paral_io_control2;
#endif




static void disp_device_init(T_LCD_PARAM *p_mode)
{
	UINT32                  i;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control2;
#endif

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_SIF) {

		sif_open(dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch);

		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_BUSMODE,  USED_SIF_MODE);
		// Bus clock = 3 MHz
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_BUSCLOCK, 3000000);
		// 16 half bus clock = 2666.66 ns
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SEND,     2667);
		// 0 half bus clock = 0 ns
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SENS,     0);
		// 0 half bus clock = 0 ns
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SENH,     0);
		// 16 bits per transmission
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_LENGTH,   16);
		// MSb shift out first
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_DIR,      SIF_DIR_MSB);
		// Delay between successive transmission
		sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_DELAY,    2000);

	}

	p_disp_dev_control(DISPDEV_IOCTRL_GET_PANEL_ADJUST, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_PANEL_ADJUST.pfp_adjust != NULL) {
		dev_io_ctrl.SEL.GET_PANEL_ADJUST.pfp_adjust();
	} else if (p_mode->p_cmd_queue != NULL) {
		for (i = 0; i < p_mode->n_cmd; i++) {
			if (p_mode->p_cmd_queue[i].ui_address == CMDDELAY_MS) {
				dispdev_platform_delay_ms(p_mode->p_cmd_queue[i].ui_value);
			} else if (p_mode->p_cmd_queue[i].ui_address == CMDDELAY_US) {
				dispdev_platform_delay_us(p_mode->p_cmd_queue[i].ui_value);
			} else {
				dispdev_write_lcd_reg(p_mode->p_cmd_queue[i].ui_address, p_mode->p_cmd_queue[i].ui_value);
			}
		}
	}

}

#if 1
/*
    Display Device API of open LCD with RGB/YUV Paral  interface

    Display Device API of open LCD with RGB/YUV Paral  interface.

    @param[in] p_devif   Please refer to dispdevctrl.h for details.

    @return
     - @b E_PAR:    Error p_devif content.
     - @b E_NOSPT:  Driver without supporting the specified LCD mode.
     - @b E_OK:     LCD Open done and success.
*/
static ER dispdev_open_ifparal(void)
{
	UINT32                  i;
	T_LCD_PARAM              *p_mode      = NULL;
	T_LCD_PARAM              *p_config_mode = NULL;
	PINMUX_LCDINIT          lcd_mode;
	UINT32                  mode_number;
	PINMUX_FUNC_ID          pin_func_id;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control2;
#endif

	DBG_IND("openIFParal START\r\n");

	if (p_disp_dev_control == NULL) {
		return E_PAR;
	}

	//
	//  Sortig the SysInit Config mode support
	//
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
#elif ((DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2))
	pin_func_id = PINMUX_FUNC_ID_LCD2;
#endif
	lcd_mode = dispdev_platform_get_disp_mode(pin_func_id) & (~PINMUX_LCDMODE_AUTO_PINMUX);
	p_config_mode = dispdev_get_config_mode_paral(&mode_number);
	if ((p_config_mode != NULL) && (mode_number > 0)) {
		for (i = 0; i < mode_number; i++) {
			if (p_config_mode[i].panel.lcd_mode == lcd_mode) {
				p_mode = (T_LCD_PARAM *)&p_config_mode[i];
				break;
			}
		}
	}

	if (p_mode == NULL) {
		DBG_ERR("lcd_mode=%d not support\r\n", lcd_mode);
		return E_NOSPT;
	}

#if 1
	if (lcd_mode == PINMUX_LCDMODE_CCIR656) {
		UINT32 ui_fp, ui_field_end, ui_odd_start;
		/*dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = 0x20C;  //mismatch setting
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = 0x16;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = 0x106;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = 0x11C;
		*/
		/*dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = 0x000; //ide odd,even = 0x2c, 0x2d
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = 0x016;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = 0x106;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = 0x11D;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_start  = 0x04;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_end    = 0x10A;*/

		DBG_IND("Field0_VValidST<->ui_odd_end---------->[0x%02x]\r\n", (unsigned int)((p_mode->panel.ui_vsync_back_porch_odd >> 1)));
		DBG_IND("ui_even_start[CEVENBLKST]------------->[0x%04x]\r\n", (unsigned int)((p_mode->panel.ui_vsync_active_period >> 1) + (p_mode->panel.ui_vsync_back_porch_odd >> 1)));
		DBG_IND("CEVENBLKED[ui_even_end]--------------->[0x%04x]\r\n", (unsigned int)((p_mode->panel.ui_vsync_active_period >> 1) + (p_mode->panel.ui_vsync_back_porch_odd >> 1) + ((p_mode->panel.ui_vsync_total_period - p_mode->panel.ui_vsync_active_period + 1) >> 1)));
		DBG_IND("Field1_VValidEnd-------------------->[0x%04x]\r\n", (unsigned int)((p_mode->panel.ui_vsync_active_period + p_mode->panel.ui_vsync_back_porch_odd) - 1));

		ui_fp = p_mode->panel.ui_vsync_total_period - p_mode->panel.ui_vsync_active_period - p_mode->panel.ui_vsync_back_porch_odd;
		if (ui_fp % 2) {
			ui_fp = ((ui_fp + 1) >> 1);
		}
		ui_field_end = (p_mode->panel.ui_vsync_back_porch_odd >> 1) + (p_mode->panel.ui_vsync_active_period >> 1) + ui_fp + (p_mode->panel.ui_vsync_sync_width >> 1);
		DBG_IND("ui_field_end-------------------------->[0x%04x]\r\n", (unsigned int)ui_field_end);
		ui_odd_start = (p_mode->panel.ui_vsync_active_period >> 1) + (p_mode->panel.ui_vsync_back_porch_odd >> 1) + ((p_mode->panel.ui_vsync_total_period - p_mode->panel.ui_vsync_active_period + 1) >> 1) + (p_mode->panel.ui_vsync_active_period >> 1);
		DBG_IND("ui_odd_start-------------------------->[0x%04x]\r\n", (unsigned int)ui_odd_start);
#if 1
#if 0
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = 0x209;   //ide odd,even = 0x24, 0x25
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = 0x12;    //=p_mode.panel.ui_vsync_back_porch_odd / 2
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = 0x102;   //=p_mode.panel.ui_vsync_active_period / 2 + p_mode.panel.ui_vsync_back_porch_odd / 2
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = 0x119;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_start  = 0x04;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_end    = 0x10A;
#else
		if (p_mode->ide.interlace == TRUE) {
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = ui_odd_start;  //ide odd,even = 0x24, 0x25
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = (p_mode->panel.ui_vsync_back_porch_odd >> 1); //=p_mode.panel.ui_vsync_back_porch_odd / 2
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = (p_mode->panel.ui_vsync_active_period >> 1) + (p_mode->panel.ui_vsync_back_porch_odd >> 1);   //=p_mode.panel.ui_vsync_active_period / 2 + p_mode.panel.ui_vsync_back_porch_odd / 2
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = (p_mode->panel.ui_vsync_active_period >> 1) + (p_mode->panel.ui_vsync_back_porch_odd >> 1) + ((p_mode->panel.ui_vsync_total_period - p_mode->panel.ui_vsync_active_period + 1) >> 1);
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_start  = (p_mode->panel.ui_vsync_sync_width >> 1);
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_end    = ui_field_end;
		} else {
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = ui_odd_start << 1;  //ide odd,even = 0x24, 0x25
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = (p_mode->panel.ui_vsync_back_porch_odd); //=p_mode.panel.ui_vsync_back_porch_odd / 2
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = (p_mode->panel.ui_vsync_active_period) + (p_mode->panel.ui_vsync_back_porch_odd);   //=p_mode.panel.ui_vsync_active_period / 2 + p_mode.panel.ui_vsync_back_porch_odd / 2
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = (p_mode->panel.ui_vsync_active_period) + (p_mode->panel.ui_vsync_back_porch_odd) + ((p_mode->panel.ui_vsync_total_period - p_mode->panel.ui_vsync_active_period + 1));
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_start  = 0;//(p_mode->panel.ui_vsync_sync_width);
			dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_end    = 0;//ui_field_end;
		}
#endif
#else
		//1080I
		//ide_set_digital_timing(0x464, (0x015), (0x231), (0x248), 0, (564));
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_start    = 0x464;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_odd_end      = 0x015;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_start   = 0x231;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_even_end     = 0x248;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_start  = 0x000;
		dev_io_ctrl.SEL.SET_CCIR656_SYNCCODE.ui_field_end    = 0x234;
#endif

		p_disp_dev_control(DISPDEV_IOCTRL_SET_CCIR656_SYNCCODE, &dev_io_ctrl);
	}
#endif

	// Config & Enable Display physical engine
	dispdev_set_display(p_disp_dev_control, T_LCD_INF_PARALLEL_16BITS, p_mode, pin_func_id);

	// Config & Enable display device
	disp_device_init(p_mode);

	//DBG_IND("openIFParal Done mode[%u]\r\n", p_mode);

#if 0
	// Test panel Frame Rate
	{
		UINT32 i;

		Perf_Open();
		Perf_Mark();
		for (i = 0; i < 1000; i++) {
			p_devif->wait_frm_end();
		}
		DBG_ERR("Time tick = %d us\r\n", Perf_GetDuration());
		Perf_Close();
	}
#endif

	return E_OK;
}

/*
    Close LCD display panel.

    Call this api to close the LCD panel device.

    @param[in] p_devif Please refer to dispdevctrl.h for details.

    @return
     - @b E_OK:     LCD Close done and success.
*/
static ER dispdev_close_ifparal(void)
{
	UINT32                  i, cmd_number;
	T_PANEL_CMD              *p_standby = NULL;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	PINMUX_FUNC_ID          pin_func_id;
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control2;
#endif

	dispanl_debug(("closeIFParal START\r\n"));

	// Close Display Engine First
	dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	//pll_disableClock(IDE1_CLK);
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	//pll_disableClock(IDE2_CLK);
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif

	p_standby = dispdev_get_standby_cmd_paral(&cmd_number);

	if (p_standby != NULL) {
		// Set display device into Standby or powerdown.
		for (i = 0; i < cmd_number; i++) {
			dispdev_write_lcd_reg(p_standby[i].ui_address, p_standby[i].ui_value);
		}
	}

	// Close SIF if needed.
	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_SIF) {
		sif_close(dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch);
	}

	// close pinmux if needed.
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
#elif ((DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2))
	pin_func_id = PINMUX_FUNC_ID_LCD2;
#endif
	if (dispdev_platform_get_disp_mode(pin_func_id) & PINMUX_LCDMODE_AUTO_PINMUX) {
		dispdev_platform_set_pinmux(pin_func_id, PINMUX_LCD_SEL_GPIO);
	}

	dispanl_debug(("closeIFParal Done\r\n"));
	return E_OK;
}

/*
    Write LCD Register

    Write LCD Register

    @param[in] p_devif   Please refer to dispdevctrl.h for details.
    @param[in] ui_addr   LCD Register Offset.
    @param[in] ui_value  Assigned LCD Register value

    @return void
*/
static void dispdev_write_lcd_reg(UINT32 ui_addr, UINT32 ui_value)
{
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control2;
#endif

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_GPIO) {
		dispdev_write_to_lcd_gpio_paral(p_disp_dev_control, ui_addr, ui_value);
	} else if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_SIF) {
		dispdev_write_to_lcd_sif_paral(p_disp_dev_control, ui_addr, ui_value);
	}
	//return E_OK;
}

/*
    PreGet LCD size

    PreGet LCD size

    @param[in] p_devif   Please refer to dispdevctrl.h for details.
    @param[in] ui_addr   LCD Register Offset.
    @param[in] ui_value  Assigned LCD Register value

    @return void
*/
static ER dispdev_get_lcd_size(DISPDEV_GET_PRESIZE *t_size)
{
	UINT32                  i;
	T_LCD_PARAM              *p_mode      = NULL;
	T_LCD_PARAM              *p_config_mode = NULL;
	PINMUX_LCDINIT          lcd_mode;
	UINT32                  mode_number;
	PINMUX_FUNC_ID          pin_func_id;
	//DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control2;
#endif

	dispanl_debug(("getLcdSize START\r\n"));

	if (p_disp_dev_control == NULL) {
		return E_PAR;
	}

	t_size->ui_buf_width = 0;
	t_size->ui_buf_height = 0;
	t_size->ui_win_width = 0;
	t_size->ui_win_height = 0;


	//
	//  Sortig the SysInit Config mode support
	//
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
#elif((DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2))
	pin_func_id = PINMUX_FUNC_ID_LCD2;
#endif
	lcd_mode = dispdev_platform_get_disp_mode(pin_func_id);
	p_config_mode = dispdev_get_config_mode_paral(&mode_number);
	if ((p_config_mode != NULL) && (mode_number > 0)) {
		for (i = 0; i < mode_number; i++) {
			if (p_config_mode[i].panel.lcd_mode == lcd_mode) {
				p_mode = (T_LCD_PARAM *)&p_config_mode[i];
				break;
			}
		}
	}

	if (p_mode == NULL) {
		DBG_ERR("lcd_mode=%d not support\r\n", (int)lcd_mode);
		return E_NOSPT;
	}
	t_size->ui_buf_width = p_mode->panel.ui_buffer_width;
	t_size->ui_buf_height = p_mode->panel.ui_buffer_height;
	t_size->ui_win_width = p_mode->panel.ui_window_width;
	t_size->ui_win_height = p_mode->panel.ui_window_height;
	return E_OK;

}




/*
    Read LCD Register

    Read LCD Register

    @param[in] p_devif   Please refer to dispdevctrl.h for details.
    @param[in] ui_addr   LCD Register Offset.

    @return Read back LCD Register value
*/
static UINT32  dispdev_read_lcd_reg(UINT32 ui_addr)
{
	UINT32                  ui_value = 0x0;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control1;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if_paral_io_control2;
#endif

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_GPIO) {
		dispdev_read_from_lcd_gpio_paral(p_disp_dev_control, ui_addr, &ui_value);
	} else {
		DBG_ERR("Error dispdev_RegRead Op!\r\n");
	}

	return ui_value;
}

/*
    Set Display Device IOCTRL function pointer
*/
static void dispdev_set_ifparal_ioctrl(FP p_io_ctrl_func)
{
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
	p_if_paral_io_control1 = (DISPDEV_IOCTRL)p_io_ctrl_func;
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
	p_if_paral_io_control2 = (DISPDEV_IOCTRL)p_io_ctrl_func;
#endif
}

/*
    Set LCD rotation
*/
static ER dispdev_set_device_rotate(DISPDEV_LCD_ROTATE rot)
{
	UINT32          mode_no, index, i;
	T_LCD_ROT        *p_cmd_rot = NULL;
	BOOL            b_valid = FALSE;

	p_cmd_rot = dispdev_get_lcd_rotate_cmd_paral(&mode_no);

	// Search Valid Rotattion Config table index
	for (index = 0; index < mode_no; index++) {
		if (p_cmd_rot[index].rotation == rot) {
			b_valid = TRUE;
			break;
		}
	}

	if (b_valid == FALSE) {
		DBG_ERR("DispDev NO valid rotate!\r\n\r\n");
		return E_PAR;
	}


	if (p_cmd_rot[index].p_cmd_queue != NULL) {
		for (i = 0; i < p_cmd_rot[index].n_cmd; i++) {
			if (p_cmd_rot[index].p_cmd_queue[i].ui_address == CMDDELAY_MS) {
				dispdev_platform_delay_ms(p_cmd_rot[index].p_cmd_queue[i].ui_value);
			} else if (p_cmd_rot[index].p_cmd_queue[i].ui_address == CMDDELAY_US) {
				dispdev_platform_delay_us(p_cmd_rot[index].p_cmd_queue[i].ui_value);
			} else {
				dispdev_write_lcd_reg(p_cmd_rot[index].p_cmd_queue[i].ui_address, p_cmd_rot[index].p_cmd_queue[i].ui_value);
			}
		}
	}

	return E_OK;
}

#endif


#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
/**
    Get LCD 1 Device Object

    Get LCD 1 Device Object. This Object is used to control the LCD Device.

    @return The pointer of LCD Display Device object.
*/
PDISPDEV_OBJ dispdev_get_lcd1_dev_obj(void)
{
	return &disp_dev_ifparal_obj;
}
#elif (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD2)
/**
    Get LCD 2 Device Object

    Get LCD 2 Device Object. This Object is used to control the LCD Device.

    @return The pointer of LCD Display Device object.
*/
PDISPDEV_OBJ dispdev_get_lcd2_dev_obj(void)
{
	return &disp_dev_ifparal_obj2;
}

#endif

