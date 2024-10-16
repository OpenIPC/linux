/* drivers/video/sunxi/disp2/disp/lcd/k101im2qa04.c
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
/*

			lcd_driver_name	 = "WG09618902881BC";
			lcd_backlight	   = <50>;
			lcd_if			  = <4>;

			lcd_x			   = <240>;
			lcd_y			   = <432>;
			lcd_width		   = <30>;	//52
			lcd_height		  = <54>;	//52
			lcd_dclk_freq	   = <11>;

			lcd_pwm_used		= <1>;
			lcd_pwm_ch		  = <0>;
			lcd_pwm_freq		= <50000>;
			lcd_pwm_pol		 = <1>;
			lcd_pwm_max_limit   = <255>;


			lcd_hbp						 = <112>;	//96	40
			lcd_ht						  = <400>;	//480	288
			lcd_hspw			= <32>;	//2	16
			lcd_vbp			 = <16>;	//21	16
			lcd_vt			  = <451>;	//451
			lcd_vspw			= <10>;	//10



*/
#include "TV096WXM_NH0.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init1(u32 sel);
static void lcd_panel_init2(u32 sel);
static void lcd_panel_exit(u32 sel);

#define sunxi_lcd_dsi_dcs_write_14para dsi_dcs_wr_14para
#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)
//#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)

#define REGFLAG_DELAY		 0xFC
#define REGFLAG_END_OF_TABLE  0xFE

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[108];
};

s32 dsi_dcs_wr_14para(u32 sel, u8 cmd, u8 para1, u8 para2, u8 para3, u8 para4, u8 para5, u8 para6, u8 para7,
							u8 para8, u8 para9, u8 para10, u8 para11, u8 para12, u8 para13, u8 para14)
{
	u8 tmp[14];

	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	tmp[3] = para4;
	tmp[4] = para5;
	tmp[5] = para6;
	tmp[6] = para7;
	tmp[7] = para8;
	tmp[8] = para9;
	tmp[9] = para10;
	tmp[10] = para11;
	tmp[11] = para12;
	tmp[12] = para13;
	tmp[13] = para14;
	dsi_dcs_wr(0, cmd, tmp, 14);
	return 0;
}

static struct LCM_setting_table lcm_initialization_boe9881[] = {
	/* ST7701 Initial Code For boe2.86ips			  */
/* Resolution: 240 x 432													  */

	{0x2A, 4, {0x00, 0x00, 0x00, 0xEF} },
	{REGFLAG_DELAY, 2, {} },
	{0x2B, 4, {0x00, 0x00, 0x01, 0xAF} },
	{REGFLAG_DELAY, 2, {} },
//	{0x84,2,{0x06,0x64}},
//	{0x1F,13,{0x06,0x01,0x0D,0x17,0x1C,0x21,0x2B,0x32,0x30,0x32,0x3F,0x35,0x00}},
//	{0x06,13,{0x1F,0x0A,0x28,0x31,0x35,0x35,0x40,0x4C,0x51,0x56,0x5E,0x3E,0x00}},
//	{0x19,13,{0x06,0x01,0x11,0x1D,0x23,0x27,0x2F,0x35,0x34,0x37,0x56,0x35,0x00}},
//	{0x06,13,{0x19,0x0A,0x11,0x2C,0x32,0x32,0x3C,0x46,0x4A,0x50,0x5A,0x3E,0x00}},
//	{0x05,13,{0x05,0x01,0x0C,0x20,0x29,0x31,0x38,0x3B,0x36,0x34,0x3C,0x35,0x00}},
//	{0x05,13,{0x05,0x0A,0x2B,0x2F,0x2F,0x2C,0x33,0x3C,0x44,0x4D,0x5F,0x3E,0x00}},
//	{0x2C,1,{0x00}},
	{0x11, 0, {} },  // Sleep Out
	{REGFLAG_DELAY, 120, {} },
	{REGFLAG_DELAY, 40, {} },

	//SET PIXEL FORMAT
	{0x2C, 0, {} },  // write memory
	{0x29, 0, {} },  // disp power on

	// {REGFLAG_DELAY, 20, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {0x00} }

};

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
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
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1])
				* j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
							(value<<16)
							+ (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) +
					(lcd_gamma_tbl[items-1][1]<<8)
					+ lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 120);
	LCD_OPEN_FUNC(sel, lcd_panel_init1, 10);
	LCD_OPEN_FUNC(sel, lcd_panel_init2, 1);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 5);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 1);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 0);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_power_enable(sel, 1);  // eldo3, 1.8V

	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(1);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(2);

	sunxi_lcd_power_enable(sel, 0);  // dcdc1, 3.0V
	sunxi_lcd_delay_ms(10);

}

static void lcd_power_off(u32 sel)
{
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_power_disable(sel, 1);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_power_disable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

static void lcd_panel_init1(u32 sel)
{
	u32 i;
	sunxi_lcd_dsi_clk_enable(sel);
		for (i = 0; ; i++) {
				if (lcm_initialization_boe9881[i].cmd == REGFLAG_END_OF_TABLE)
						break;
				else if (lcm_initialization_boe9881[i].cmd == REGFLAG_DELAY)
						sunxi_lcd_delay_ms(lcm_initialization_boe9881[i].count);
				else {
						dsi_dcs_wr(0, lcm_initialization_boe9881[i].cmd,
										lcm_initialization_boe9881[i].para_list,
										lcm_initialization_boe9881[i].count);
				}
		}
	printk("[DEBUG] zzz now u will init cmd for end\n");
	//sunxi_lcd_dsi_dcs_write_0para(sel, 0x11); /* SLPOUT */
}

static void lcd_panel_init2(u32 sel)
{
	/*DISP ON */
	//sunxi_lcd_dsi_dcs_write_0para(sel, 0x29); /* DSPON */
	//sunxi_lcd_delay_ms(150);

	/*--- TE----// */
//	sunxi_lcd_dsi_dcs_write_1para(sel, 0x35, 0x00);
}


static void lcd_panel_exit(u32 sel)
{
//	sunxi_lcd_dsi_dcs_write_1para(sel, 0x10,0x00);
//	sunxi_lcd_dsi_dcs_write_0para(sel, 0x11);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_dsi_dcs_write_2para(sel, 0xF1, 0x5A, 0x5A);
	sunxi_lcd_dsi_dcs_write_14para(sel, 0xF4, 0x0B, 0x00, 0x00, 0x00, 0x21, 0x4F, 0x01, 0x0E, 0x2A, 0x66, 0x05, 0x2A, 0x00, 0x05);
	sunxi_lcd_dsi_dcs_write_2para(sel, 0xF1, 0xA5, 0xA5);
	sunxi_lcd_dsi_dcs_write_2para(sel, 0x10, 0x00, 0x00);
	sunxi_lcd_delay_ms(1);
}
/*
static void lcm_update(unsigned int x, unsigned int y,
					   unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;
	dsi_set_cmdq(&data_array, 7, 0);
}
*/

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel  TV096WXM_NH0_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "TV096WXM_NH0",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
		.cfg_open_flow = lcd_open_flow,
		.cfg_close_flow = lcd_close_flow,
		.lcd_user_defined_func = lcd_user_defined_func,
		//.lcm_update = lcm_update,
	},
};
