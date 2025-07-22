/*
 * arch/mips/xburst2/soc-x2000/include/mach/jzfb.h
 *
 * Copyright (C) 2016 Ingenic Semiconductor Inc.
 *
 * Author:clwang<chunlei.wang@ingenic.com>
 *
 * This program is free software, you can redistribute it and/or modify it
 *
 * under the terms of the GNU General Public License version 2 as published by
 *
 * the Free Software Foundation.
 */

#ifndef __JZFB_H__
#define __JZFB_H__

#include <linux/fb.h>
#include <soc/gpio.h>

enum jzfb_lcd_type {
	LCD_TYPE_TFT = 0,
	LCD_TYPE_SLCD =1,
};

/* smart lcd interface_type */
enum smart_lcd_type {
	SMART_LCD_TYPE_6800,
	SMART_LCD_TYPE_8080,
	SMART_LCD_TYPE_SPI_3,
	SMART_LCD_TYPE_SPI_4,
};

/* smart lcd format */
enum smart_lcd_format {
	SMART_LCD_FORMAT_565,
	SMART_LCD_FORMAT_666,
	SMART_LCD_FORMAT_888,
};

/* smart lcd command width */
enum smart_lcd_cwidth {
	SMART_LCD_CWIDTH_8_BIT,
	SMART_LCD_CWIDTH_9_BIT,
	SMART_LCD_CWIDTH_16_BIT,
	SMART_LCD_CWIDTH_18_BIT,
	SMART_LCD_CWIDTH_24_BIT,
};

/* smart lcd data width */
enum smart_lcd_dwidth {
	SMART_LCD_DWIDTH_8_BIT,
	SMART_LCD_DWIDTH_9_BIT,
	SMART_LCD_DWIDTH_16_BIT,
	SMART_LCD_DWIDTH_18_BIT,
	SMART_LCD_DWIDTH_24_BIT,
};

enum smart_config_type {
	SMART_CONFIG_DATA,
	SMART_CONFIG_PRM,
	SMART_CONFIG_CMD,
	SMART_CONFIG_UDELAY,
};

struct smart_lcd_data_table {
	enum smart_config_type type;
	unsigned int value;
};

enum tft_lcd_color_even {
	TFT_LCD_COLOR_EVEN_RGB,
	TFT_LCD_COLOR_EVEN_RBG,
	TFT_LCD_COLOR_EVEN_BGR,
	TFT_LCD_COLOR_EVEN_BRG,
	TFT_LCD_COLOR_EVEN_GBR,
	TFT_LCD_COLOR_EVEN_GRB,
};

enum tft_lcd_color_odd {
	TFT_LCD_COLOR_ODD_RGB,
	TFT_LCD_COLOR_ODD_RBG,
	TFT_LCD_COLOR_ODD_BGR,
	TFT_LCD_COLOR_ODD_BRG,
	TFT_LCD_COLOR_ODD_GBR,
	TFT_LCD_COLOR_ODD_GRB,
};

enum tft_lcd_mode {
	TFT_LCD_MODE_PARALLEL_24B,
	TFT_LCD_MODE_SERIAL_RGB,
	TFT_LCD_MODE_SERIAL_RGBD,
};

struct jzfb_tft_config {
	unsigned int pix_clk_inv:1;
	unsigned int de_dl:1;
	unsigned int sync_dl:1;
	enum tft_lcd_color_even color_even;
	enum tft_lcd_color_odd color_odd;
	enum tft_lcd_mode mode;
};

struct jzfb_smart_config {
	unsigned int frm_md:1;
	unsigned int rdy_switch:1;
	unsigned int rdy_dp:1;
	unsigned int rdy_anti_jit:1;
	unsigned int te_switch:1;
	unsigned int te_md:1;
	unsigned int te_dp:1;
	unsigned int te_anti_jit:1;
	unsigned int cs_en:1;
	unsigned int cs_dp:1;
	unsigned int dc_md:1;
	unsigned int wr_md:1;
	enum smart_lcd_type smart_type;
	enum smart_lcd_format pix_fmt;
	enum smart_lcd_dwidth dwidth;
	enum smart_lcd_cwidth cwidth;
	unsigned int bus_width;

	unsigned long write_gram_cmd;
	unsigned int length_cmd;
	struct smart_lcd_data_table *data_table;
	unsigned int length_data_table;
	int (*init) (void);
	int (*gpio_for_slcd) (void);
};

struct lcd_callback_ops {
	int (*lcd_initialize_begin)(void*);
	int (*lcd_initialize_end)(void*);

	int (*lcd_power_on_begin)(void*);
	int (*lcd_power_on_end)(void*);

	int (*lcd_power_off_begin)(void*);
	int (*lcd_power_off_end)(void*);

	int (*dma_transfer_begin)(void*);
	int (*dma_transfer_end)(void*);
};

struct lcdc_gpio_struct {
    unsigned long cs;
    unsigned long rd;
    unsigned long rst;
    unsigned long func_pins;
    enum gpio_port func_port;
    enum gpio_function func_num;
    unsigned long backlight;
};

struct jzfb_platform_data {
	unsigned int num_modes;
	struct fb_videomode *modes;

	enum jzfb_lcd_type lcd_type;
	unsigned int bpp;
	unsigned int width;
	unsigned int height;

	struct jzfb_smart_config *smart_config;
	struct jzfb_tft_config *tft_config;

	unsigned dither_enable:1;
	struct {
		unsigned dither_red;
		unsigned dither_green;
		unsigned dither_blue;
	} dither;
	struct lcdc_gpio_struct *gpio_assign;
	struct lcd_callback_ops *lcd_callback_ops;
};

extern struct jzfb_platform_data jzfb_platform_data;
#endif /*__DPU_IF_H__*/
