/*
    Display device object Golden Sample for driving RGB/YUV 8bits panel

    @file       dispdev_gsinf8bits.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_if8bits.h"

static void     disp_device_init(T_LCD_PARAM *p_mode);


static ER       dispdev_open_if8bits(void);
static ER       dispdev_close_if8bits(void);
static void     dispdev_write_lcd_reg(UINT32 ui_addr, UINT32 ui_value);
static UINT32   dispdev_read_lcd_reg(UINT32 ui_addr);
static void     dispdev_set_if8bits_ioctrl(FP p_io_ctrl_func);
static ER       dispdev_set_device_rotate(DISPDEV_LCD_ROTATE rot);
static ER       dispdev_get_lcd_size(PDISPDEV_GET_PRESIZE t_lcd_size);



/*
    Exporting display device object
*/
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
static DISPDEV_OBJ     disp_dev_if8bits_obj = { dispdev_open_if8bits, dispdev_close_if8bits, dispdev_set_device_rotate, dispdev_write_lcd_reg, dispdev_read_lcd_reg, dispdev_get_lcd_size, dispdev_set_if8bits_ioctrl, { NULL, NULL } };
static volatile DISPDEV_IOCTRL  p_if8bits_io_control1;
static volatile DISPDEV_IOCTRL  p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
static DISPDEV_OBJ     disp_dev_if8bits_obj2 = { dispdev_open_if8bits, dispdev_close_if8bits, dispdev_set_device_rotate, dispdev_write_lcd_reg, dispdev_read_lcd_reg, dispdev_get_lcd_size, dispdev_set_if8bits_ioctrl, { NULL, NULL } };
static volatile DISPDEV_IOCTRL  p_if8bits_io_control2;
#endif




static void disp_device_init(T_LCD_PARAM *p_mode)
{
	UINT32                  i;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	T_LCD_SIF_PARAM         *dev_sif_param;
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control2;
#endif

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_SIF) {

		sif_open(dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch);
		dev_sif_param = dispdev_get_lcd_sif_param();

		if (dev_sif_param != NULL) {

		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_BUSMODE,  dev_sif_param->ui_sif_mode);
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_BUSCLOCK, dev_sif_param->ui_bus_clock);
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SENS,     dev_sif_param->ui_sens);
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SENH,     dev_sif_param->ui_senh);
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_LENGTH,   dev_sif_param->ui_transfer_length);
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_DIR,      dev_sif_param->ui_sif_dir);
		} else {
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_BUSMODE,  USED_SIF_MODE);
		    // Bus clock = 3 MHz
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_BUSCLOCK, 3000000);
		    // 16 half bus clock = 2666.66 ns => DMA related param
		    // sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SEND,     2667);
		    // 0 half bus clock = 0 ns
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SENS,     20);
		    // 0 half bus clock = 0 ns
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_SENH,     0);
		    // 16 bits per transmission
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_LENGTH,   24);
		    // MSb shift out first
		    sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_DIR,      SIF_DIR_MSB);
		    // Delay between successive transmission => DMA related param
		    //sif_setConfig((SIF_CH)dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, SIF_CONFIG_ID_DELAY,    2000);

		}

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
    Display Device API of open LCD with RGB/YUV Serial 8bits interface

    Display Device API of open LCD with RGB/YUV Serial 8bits interface.

    @param[in] p_devif   Please refer to dispdevctrl.h for details.

    @return
     - @b E_PAR:    Error p_devif content.
     - @b E_NOSPT:  Driver without supporting the specified LCD mode.
     - @b E_OK:     LCD Open done and success.
*/
static ER dispdev_open_if8bits(void)
{
	UINT32                  i;
	T_LCD_PARAM              *p_mode      = NULL;
	T_LCD_PARAM              *p_config_mode = NULL;
	PINMUX_LCDINIT          lcd_mode;
	UINT32                  mode_number;
	PINMUX_FUNC_ID          pin_func_id;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control2;
#endif

	dispanl_debug(("openIF8Bits START\r\n"));

	if (p_disp_dev_control == NULL) {
		return E_PAR;
	}

	//
	//  Sortig the SysInit Config mode support
	//
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
#elif ((DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2))
	pin_func_id = PINMUX_FUNC_ID_LCD2;
#endif
	lcd_mode = dispdev_platform_get_disp_mode((UINT32)pin_func_id) & (~PINMUX_LCDMODE_AUTO_PINMUX);

#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	p_config_mode = dispdev_get_config_mode(&mode_number);
#elif ((DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2))
	p_config_mode = dispdev_get_config_mode2(&mode_number);
#endif

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

		DBG_IND("Field0_VValidST<->ui_odd_end---------->[0x%02x]\r\n", (unsigned int)(p_mode->panel.ui_vsync_back_porch_odd >> 1));
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
	dispdev_set_display(p_disp_dev_control, T_LCD_INF_SERIAL_8BITS, p_mode, pin_func_id);

	// Config & Enable display device
	disp_device_init(p_mode);

	dispanl_debug(("openIF8Bits Done\r\n"));

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
static ER dispdev_close_if8bits(void)
{
	UINT32                  i, cmd_number;
	T_PANEL_CMD              *p_standby = NULL;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	PINMUX_FUNC_ID          pin_func_id;
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control2;
#endif

	dispanl_debug(("closeIF8Bits START\r\n"));

	// Close Display Engine First
	dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	p_disp_dev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_disp_dev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif

#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	p_standby = dispdev_get_standby_cmd(&cmd_number);
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	p_standby = dispdev_get_standby_cmd2(&cmd_number);
#endif

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
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
#elif ((DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2))
	pin_func_id = PINMUX_FUNC_ID_LCD2;
#endif
	if (dispdev_platform_get_disp_mode((UINT32)pin_func_id) & PINMUX_LCDMODE_AUTO_PINMUX) {
	  dispdev_platform_set_pinmux(pin_func_id, PINMUX_LCD_SEL_GPIO);
	}

	dispanl_debug(("closeIF8Bits Done\r\n"));
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
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control2;
#endif

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_GPIO) {
		dispdev_write_to_lcd_gpio(p_disp_dev_control, ui_addr, ui_value);
	} else if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_SIF) {
		dispdev_write_to_lcd_sif(p_disp_dev_control, ui_addr, ui_value);
	}
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_GPIO) {
		dispdev_write_to_lcd_gpio2(p_disp_dev_control, ui_addr, ui_value);
	} else if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_SIF) {
		dispdev_write_to_lcd_sif2(p_disp_dev_control, ui_addr, ui_value);
	}
#endif
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
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control2;
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
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	pin_func_id = PINMUX_FUNC_ID_LCD;
#elif((DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2))
	pin_func_id = PINMUX_FUNC_ID_LCD2;
#endif
	lcd_mode = dispdev_platform_get_disp_mode(pin_func_id);
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	p_config_mode = dispdev_get_config_mode(&mode_number);
#elif((DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2))
	p_config_mode = dispdev_get_config_mode2(&mode_number);
#endif
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
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control1;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	DISPDEV_IOCTRL          p_disp_dev_control = p_if8bits_io_control2;
#endif

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_GPIO) {
		dispdev_read_from_lcd_gpio(p_disp_dev_control, ui_addr, &ui_value);
	} else {
		DBG_ERR("Error dispdev_RegRead Op!\r\n");
	}
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	if (dev_io_ctrl.SEL.GET_REG_IF.lcd_ctrl == DISPDEV_LCDCTRL_GPIO) {
		dispdev_read_from_lcd_gpio2(p_disp_dev_control, ui_addr, &ui_value);
	} else {
		DBG_ERR("Error dispdev_RegRead Op!\r\n");
	}
#endif

	return ui_value;
}

/*
    Set Display Device IOCTRL function pointer
*/
static void dispdev_set_if8bits_ioctrl(FP p_io_ctrl_func)
{
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	p_if8bits_io_control1 = (DISPDEV_IOCTRL)p_io_ctrl_func;
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	p_if8bits_io_control2 = (DISPDEV_IOCTRL)p_io_ctrl_func;
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

#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	p_cmd_rot = dispdev_get_lcd_rotate_cmd(&mode_no);
#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
	p_cmd_rot = dispdev_get_lcd_rotate_cmd2(&mode_no);
#endif

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


#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
/**
    Get LCD 1 Device Object

    Get LCD 1 Device Object. This Object is used to control the LCD Device.

    @return The pointer of LCD Display Device object.
*/
PDISPDEV_OBJ dispdev_get_lcd1_dev_obj(void)
{
	return &disp_dev_if8bits_obj;
}

#elif (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD2)
/**
    Get LCD 2 Device Object

    Get LCD 2 Device Object. This Object is used to control the LCD Device.

    @return The pointer of LCD Display Device object.
*/

PDISPDEV_OBJ dispdev_get_lcd2_dev_obj(void)
{
	return &disp_dev_if8bits_obj2;
}
#endif

