/* drivers/video/sunxi/disp2/disp/lcd/nt35510.c
 *
 *   Copyright (c) 2022 Allwinnertech Co., Ltd.
 *   Author: maliankang <maliankang@allwinnertech.com>
 *
 *   nt35510 panel driver
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *
 *
 * [lcd0]
 *          base_config_start   = <1>;
 *         lcd_used            = <1>;
 *
 *         lcd_driver_name     = "nt35510";
 *
 *         lcd_bl_0_percent    = <0>;
 *         lcd_bl_40_percent   = <23>;
 *         lcd_bl_100_percent  = <100>;
 *         lcd_backlight       = <150>;
 *
 *         lcd_if              = <4>;
 *         lcd_x               = <480>;
 *         lcd_y               = <800>;
 *         lcd_width           = <0>;
 *         lcd_height          = <0>;
 *         lcd_dclk_freq       = <28>;
 *
 *         lcd_pwm_used        = <1>;
 *         lcd_pwm_ch          = <9>;
 *         lcd_pwm_freq        = <50000>;
 *         lcd_pwm_pol         = <1>;
 *         lcd_pwm_max_limit   = <255>;
 *
 *         lcd_hbp             = <50>;
 *         lcd_ht              = <562>;
 *         lcd_hspw            = <12>;
 *         lcd_vbp             = <32>;
 *         lcd_vt              = <844>;
 *         lcd_vspw            = <8>;
 *
 *         lcd_dsi_if          = <0>;
 *         lcd_dsi_lane        = <2>;
 *         lcd_dsi_format      = <0>;
 *         lcd_dsi_te          = <0>;
 *         lcd_dsi_eotp        = <0>;
 *
 *         lcd_frm             = <0>;
 *         lcd_io_phase        = <0x0000>;
 *         lcd_hv_clk_phase    = <0>;
 *         lcd_hv_sync_polarity= <0>;
 *         lcd_gamma_en        = <0>;
 *         lcd_bright_curve_en = <0>;
 *         lcd_cmap_en         = <0>;
 *
 *         lcdgamma4iep        = <22>;
 *
 *         lcd_gpio_0          = <&pio PE 13 1 0 3 1>;
 *         lcd_gpio_1          = <&pio PE 10 1 0 3 0>;
 *         pinctrl-0           = <&dsi4lane_pins_a>;
 *         pinctrl-1           = <&dsi4lane_pins_b>;
 *         base_config_end     = <1>;*/

#include "nt35510.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
	    {0, 0},     {15, 15},   {30, 30},   {45, 45},   {60, 60},
	    {75, 75},   {90, 90},   {105, 105}, {120, 120}, {135, 135},
	    {150, 150}, {165, 165}, {180, 180}, {195, 195}, {210, 210},
	    {225, 225}, {240, 240}, {255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	    {
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
	    },
	    {
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
	    },
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) *
			     j) /
				num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static void lcd_panel_get_id(u32 sel)
{
	u8 id1, id2, id3;
	u32 num = 0;
	sunxi_lcd_dsi_dcs_read(sel, 0xda, &id1, &num);
	sunxi_lcd_dsi_dcs_read(sel, 0xdb, &id2, &num);
	sunxi_lcd_dsi_dcs_read(sel, 0xdc, &id3, &num);
	printk("NT35510 ID manufacturer: %02x version: %02x driver: %02x\n", id1, id2, id3);
}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 0);
	LCD_OPEN_FUNC(sel, lcd_panel_get_id, 0);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 30);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 10);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);
	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 10);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 0);

	return 0;
}

static void lcd_power_on(u32 sel)
{

	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(10);

	/* open power by gpio */
	sunxi_lcd_gpio_set_direction(sel, 1, 1);
	sunxi_lcd_gpio_set_value(sel, 1, 1);

	/* reset lcd by gpio */
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(50);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(10);

}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(1);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_gpio_set_value(sel, 1, 0);
//	sunxi_lcd_power_disable(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	/* open power by gpio */
	sunxi_lcd_gpio_set_direction(sel, 1, 1);
	sunxi_lcd_gpio_set_value(sel, 1, 1);

//	sunxi_lcd_pwm_enable(sel);
//	sunxi_lcd_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	/* open power by gpio */
	sunxi_lcd_gpio_set_direction(sel, 1, 1);
	sunxi_lcd_gpio_set_value(sel, 1, 0);

//	sunxi_lcd_backlight_disable(sel);
//	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY         0XFE
#define REGFLAG_END_OF_TABLE  0xFC   /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[52];
};

#if 1
static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xF0, 5, {0x55, 0xaa, 0x52, 0x08, 0x01} },

	{0xb6, 3, {0x34, 0x34, 0x34} },
	{0xb0, 3, {0x09, 0x09, 0x09} },

	{0xB7, 3, {0x24, 0x24, 0x24} },
	{0xB1, 3, {0x09, 0x09, 0x09} },	//AVEE:-5.6V
	{0xB8, 1, {0x34} },
	{0xB2, 1, {0x00} },             //VCL:-2.5V
	{0xB9, 3, {0x24, 0x24, 0x24} },
	{0xB3, 3, {0x05, 0x05, 0x05} }, //VGH:12V
	{0xBF, 1, {0x01} },
	{0xBA, 3, {0x34, 0x34, 0x34} },
	{0xB5, 3, {0x0B, 0x0B, 0x0B} },	//VGL:-13V

	{0xBC, 3, {0x00, 0xA3, 0x00} },
	{0xBD, 3, {0x00, 0xA3, 0x00} },
	{0xBE, 2, {0x00, 0x50} },	//60 VCOM

	{0xD1, 52,
		{
			0x00, 0x37, 0x00, 0x52, 0x00, 0x7B, 0x00, 0x99,
			0x00, 0xB1, 0x00, 0xD2, 0x00, 0xF6, 0x01, 0x27,
			0x01, 0x4E, 0x01, 0x8C, 0x01, 0xBE, 0x02, 0x0B,
			0x02, 0x48, 0x02, 0x4A, 0x02, 0x7E, 0x02, 0xBC,
			0x02, 0xE1, 0x03, 0x10, 0x03, 0x31, 0x03, 0x5A,
			0x03, 0x73, 0x03, 0x94, 0x03, 0x9F, 0x03, 0xB3,
			0x03, 0xB9, 0x03, 0xC1
		}
	},

	{0xD2, 52,
		{
			0x00, 0x37, 0x00, 0x52, 0x00, 0x7B, 0x00, 0x99,
			0x00, 0xB1, 0x00, 0xD2, 0x00, 0xF6, 0x01, 0x27,
			0x01, 0x4E, 0x01, 0x8C, 0x01, 0xBE, 0x02, 0x0B,
			0x02, 0x48, 0x02, 0x4A, 0x02, 0x7E, 0x02, 0xBC,
			0x02, 0xE1, 0x03, 0x10, 0x03, 0x31, 0x03, 0x5A,
			0x03, 0x73, 0x03, 0x94, 0x03, 0x9F, 0x03, 0xB3,
			0x03, 0xB9, 0x03, 0xC1
		}
	},

	{0xD3, 52,
		{
			0x00, 0x37, 0x00, 0x52, 0x00, 0x7B, 0x00, 0x99,
			0x00, 0xB1, 0x00, 0xD2, 0x00, 0xF6, 0x01, 0x27,
			0x01, 0x4E, 0x01, 0x8C, 0x01, 0xBE, 0x02, 0x0B,
			0x02, 0x48, 0x02, 0x4A, 0x02, 0x7E, 0x02, 0xBC,
			0x02, 0xE1, 0x03, 0x10, 0x03, 0x31, 0x03, 0x5A,
			0x03, 0x73, 0x03, 0x94, 0x03, 0x9F, 0x03, 0xB3,
			0x03, 0xB9, 0x03, 0xC1
		}
	},

	{0xD4, 52,
		{
			0x00, 0x37, 0x00, 0x52, 0x00, 0x7B, 0x00, 0x99,
			0x00, 0xB1, 0x00, 0xD2, 0x00, 0xF6, 0x01, 0x27,
			0x01, 0x4E, 0x01, 0x8C, 0x01, 0xBE, 0x02, 0x0B,
			0x02, 0x48, 0x02, 0x4A, 0x02, 0x7E, 0x02, 0xBC,
			0x02, 0xE1, 0x03, 0x10, 0x03, 0x31, 0x03, 0x5A,
			0x03, 0x73, 0x03, 0x94, 0x03, 0x9F, 0x03, 0xB3,
			0x03, 0xB9, 0x03, 0xC1
		}
	},

	{0xD5, 52,
		{
			0x00, 0x37, 0x00, 0x52, 0x00, 0x7B, 0x00, 0x99,
			0x00, 0xB1, 0x00, 0xD2, 0x00, 0xF6, 0x01, 0x27,
			0x01, 0x4E, 0x01, 0x8C, 0x01, 0xBE, 0x02, 0x0B,
			0x02, 0x48, 0x02, 0x4A, 0x02, 0x7E, 0x02, 0xBC,
			0x02, 0xE1, 0x03, 0x10, 0x03, 0x31, 0x03, 0x5A,
			0x03, 0x73, 0x03, 0x94, 0x03, 0x9F, 0x03, 0xB3,
			0x03, 0xB9, 0x03, 0xC1
		}
	},

	{0xD6, 52,
		{
			0x00, 0x37, 0x00, 0x52, 0x00, 0x7B, 0x00, 0x99,
			0x00, 0xB1, 0x00, 0xD2, 0x00, 0xF6, 0x01, 0x27,
			0x01, 0x4E, 0x01, 0x8C, 0x01, 0xBE, 0x02, 0x0B,
			0x02, 0x48, 0x02, 0x4A, 0x02, 0x7E, 0x02, 0xBC,
			0x02, 0xE1, 0x03, 0x10, 0x03, 0x31, 0x03, 0x5A,
			0x03, 0x73, 0x03, 0x94, 0x03, 0x9F, 0x03, 0xB3,
			0x03, 0xB9, 0x03, 0xC1
		}
	},

	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x00} },
	{0xB0, 5, {0x08, 0x05, 0x02, 0x05, 0x02} },

	{0xb1, 2, {0x14, 0x00} }, //0x14 video mode 0xe4 command mode

	{0xB6, 1, {0x0A} },
	{0xB7, 2, {0x00, 0x07} },
	{0xB8, 4, {0x01, 0x05, 0x05, 0x05} },
	{0xBC, 3, {0x00, 0x00, 0x00} },			 // REGW 0xBC,0x00,0x00,0x00 为列翻转
	{0xCC, 3, {0x03, 0x00, 0x00} },
	{0xBD, 5, {0x01, 0x84, 0x07, 0x31, 0x00} },	//frame
//	{0xBD, 5, {0x01, 0x84, 0x07, 0x1e, 0x00} },	//Display Timing
	{0xB1, 2, {0x014, 0x00} },
	{0xba, 1, {0x01} },
	{0xff, 4, {0xaa, 0x55, 0x25, 0x01} },

//	{0x35, 1, {0x00} }, //TE ON
//	{0x36, 1, {0xc0} }, //正向扫描00,反向扫描C0
	{0x3A, 1, {0x77} },

	/* display 480 * 800 */
	{0x2a, 4, {0x00, 0x00, 0x01, 0xdf} },
	{0x2b, 4, {0x00, 0x00, 0x03, 0x1f} },

	{0x11, 1, {} },

	{REGFLAG_DELAY, 120, {} },

	{0x29, 1, {} },
//	{0x2c, 1, {} }, // command mode
	{REGFLAG_DELAY, 10, {} },

	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif

static void lcd_panel_init(u32 sel)
{
	u32 i = 0;

	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(10);

	for (i = 0;; i++) {
		if (lcm_initialization_setting[i].cmd == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_initialization_setting[i].cmd == REGFLAG_DELAY)
			sunxi_lcd_delay_ms(lcm_initialization_setting[i].count);
		else {
			dsi_dcs_wr(0, lcm_initialization_setting[i].cmd,
				   lcm_initialization_setting[i].para_list,
				   lcm_initialization_setting[i].count);
		}
	}
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(10);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel nt35510_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "nt35510",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
			.cfg_open_flow = lcd_open_flow,
			.cfg_close_flow = lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
	},
};
