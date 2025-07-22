/*
 * ingenic_bsp/chip-x2000/fpga/dpu/kfm701a21.c
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

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/digital_pulse_backlight.h>
#include <linux/delay.h>

#include <mach/jzfb.h>
#include <soc/gpio.h>

static int inited = 0;

static int kfm701a21_1a_set_gpio_func(struct lcdc_gpio_struct *gpio)
{
	return jzgpio_set_func(gpio->func_port, gpio->func_num, gpio->func_pins);
}

static int kfm701a21_1a_power_initialize(struct lcdc_gpio_struct *gpio)
{
	int ret;

	ret = kfm701a21_1a_set_gpio_func(gpio);
	if (ret) {
		printk("Dpu gpio-func have been set\n");
	}

	ret = gpio_request(gpio->rst, "lcd rst");
	if (ret) {
		printk(KERN_ERR "can's request lcd rst\n");
		return ret;
	}

	ret = gpio_request(gpio->cs, "lcd cs");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs\n");
		return ret;
	}
/*
	ret = gpio_request(gpio->rd, "lcd rd");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd\n");
		return ret;
	}
*/
	inited = 1;

	return 0;
}

static int kfm701a21_1a_power_reset(struct lcdc_gpio_struct *gpio)
{
	gpio_direction_output(gpio->rst, 0);
	mdelay(20);
	gpio_direction_output(gpio->rst, 1);
	mdelay(10);

	return 0;
}

static int kfm701a21_1a_power_on(struct lcdc_gpio_struct *gpio)
{
	if (!inited && kfm701a21_1a_power_initialize(gpio))
		return -EFAULT;

	gpio_direction_output(gpio->cs, 1);
/*	gpio_direction_output(gpio->rd, 1);*/
	kfm701a21_1a_power_reset(gpio);
	mdelay(5);
	gpio_direction_output(gpio->cs, 0);

	return 0;
}

static int kfm701a21_1a_power_off(struct lcdc_gpio_struct *gpio)
{
	gpio_direction_output(gpio->cs, 0);
/*	gpio_direction_output(gpio->rd, 0);*/
	gpio_direction_output(gpio->rst, 0);

	return 0;
}

static struct fb_videomode jzfb_kfm701a21_1a_videomode = {
	.name = "400*240",
	.refresh = 60,
	.xres = 400,
	.yres = 240,
	.pixclock = KHZ2PICOS(6000),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct lcd_callback_ops kfm701a21_1a_ops = {
	.lcd_power_on_begin  = (void*)kfm701a21_1a_power_on,
	.lcd_power_off_begin = (void*)kfm701a21_1a_power_off,
};

static struct smart_lcd_data_table kfm701a21_1a_data_table[] = {
	/* soft reset */
	{SMART_CONFIG_CMD, 0x0600},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_UDELAY, 10000},
	/* soft reset */
	{SMART_CONFIG_CMD, 0x0600},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_UDELAY, 10000},
	{SMART_CONFIG_CMD, 0x0606},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_UDELAY, 10000},
	{SMART_CONFIG_CMD, 0x0007},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_UDELAY, 10000},
	{SMART_CONFIG_CMD, 0x0110},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_UDELAY, 10000},
	{SMART_CONFIG_CMD, 0x0100},
	{SMART_CONFIG_PRM, 0x17b0},
	{SMART_CONFIG_CMD, 0x0101},
	{SMART_CONFIG_PRM, 0x0147},
	{SMART_CONFIG_CMD, 0x0102},
	{SMART_CONFIG_PRM, 0x019d},
	{SMART_CONFIG_CMD, 0x0103},
	{SMART_CONFIG_PRM, 0x8600},
	{SMART_CONFIG_CMD, 0x0281},
	{SMART_CONFIG_PRM, 0x0010},
	{SMART_CONFIG_UDELAY, 10000},
	{SMART_CONFIG_CMD, 0x0102},
	{SMART_CONFIG_PRM, 0x01bd},
	{SMART_CONFIG_UDELAY, 10000},
	/* initial */
	{SMART_CONFIG_CMD, 0x0000},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0001},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0002},
	{SMART_CONFIG_PRM, 0x0400},
	/* up:0x1288 down:0x12B8 left:0x1290 right:0x12A0 */
	/*{0x0003, 0x12b8, 0, 0},	// BGR*/
	{SMART_CONFIG_CMD, 0x0003},
	// RGB ( to display right, change 0x02b8 to 0x12b8)
	{SMART_CONFIG_PRM, 0x12b8},
	{SMART_CONFIG_CMD, 0x0006},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0008},
	{SMART_CONFIG_PRM, 0x0503},
	{SMART_CONFIG_CMD, 0x0009},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_CMD, 0x000b},
	{SMART_CONFIG_PRM, 0x0010},
	{SMART_CONFIG_CMD, 0x000c},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x000f},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0007},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_CMD, 0x0010},
	{SMART_CONFIG_PRM, 0x0010},
	{SMART_CONFIG_CMD, 0x0011},
	{SMART_CONFIG_PRM, 0x0202},
	{SMART_CONFIG_CMD, 0x0012},
	{SMART_CONFIG_PRM, 0x0300},
	{SMART_CONFIG_CMD, 0x0020},
	{SMART_CONFIG_PRM, 0x021e},
	{SMART_CONFIG_CMD, 0x0021},
	{SMART_CONFIG_PRM, 0x0202},
	{SMART_CONFIG_CMD, 0x0022},
	{SMART_CONFIG_PRM, 0x0100},
	{SMART_CONFIG_CMD, 0x0090},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0092},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0100},
	{SMART_CONFIG_PRM, 0x16b0},
	{SMART_CONFIG_CMD, 0x0101},
	{SMART_CONFIG_PRM, 0x0147},
	{SMART_CONFIG_CMD, 0x0102},
	{SMART_CONFIG_PRM, 0x01bd},
	{SMART_CONFIG_CMD, 0x0103},
	{SMART_CONFIG_PRM, 0x2c00},
	{SMART_CONFIG_CMD, 0x0107},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0110},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_CMD, 0x0210},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0211},
	{SMART_CONFIG_PRM, 0x00ef},
	{SMART_CONFIG_CMD, 0x0212},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0213},
	{SMART_CONFIG_PRM, 0x018f},
	{SMART_CONFIG_CMD, 0x0280},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0281},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_CMD, 0x0282},
	{SMART_CONFIG_PRM, 0x0000},
	/* gamma corrected value   table */
	{SMART_CONFIG_CMD, 0x0300},
	{SMART_CONFIG_PRM, 0x0101},
	{SMART_CONFIG_CMD, 0x0301},
	{SMART_CONFIG_PRM, 0x0b27},
	{SMART_CONFIG_CMD, 0x0302},
	{SMART_CONFIG_PRM, 0x132a},
	{SMART_CONFIG_CMD, 0x0303},
	{SMART_CONFIG_PRM, 0x2a13},
	{SMART_CONFIG_CMD, 0x0304},
	{SMART_CONFIG_PRM, 0x270b},
	{SMART_CONFIG_CMD, 0x0305},
	{SMART_CONFIG_PRM, 0x0101},
	{SMART_CONFIG_CMD, 0x0306},
	{SMART_CONFIG_PRM, 0x1205},
	{SMART_CONFIG_CMD, 0x0307},
	{SMART_CONFIG_PRM, 0x0512},
	{SMART_CONFIG_CMD, 0x0308},
	{SMART_CONFIG_PRM, 0x0005},
	{SMART_CONFIG_CMD, 0x0309},
	{SMART_CONFIG_PRM, 0x0003},
	{SMART_CONFIG_CMD, 0x030a},
	{SMART_CONFIG_PRM, 0x0f04},
	{SMART_CONFIG_CMD, 0x030b},
	{SMART_CONFIG_PRM, 0x0f00},
	{SMART_CONFIG_CMD, 0x030c},
	{SMART_CONFIG_PRM, 0x000f},
	{SMART_CONFIG_CMD, 0x030d},
	{SMART_CONFIG_PRM, 0x040f},
	{SMART_CONFIG_CMD, 0x030e},
	{SMART_CONFIG_PRM, 0x0300},
	{SMART_CONFIG_CMD, 0x030f},
	{SMART_CONFIG_PRM, 0x0500},
	/* secorrect gamma2 */
	{SMART_CONFIG_CMD, 0x0400},
	{SMART_CONFIG_PRM, 0x3500},
	{SMART_CONFIG_CMD, 0x0401},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_CMD, 0x0404},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0500},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0501},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0502},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0503},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0504},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0505},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0600},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x0606},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x06f0},
	{SMART_CONFIG_PRM, 0x0000},
	{SMART_CONFIG_CMD, 0x07f0},
	{SMART_CONFIG_PRM, 0x5420},
	{SMART_CONFIG_CMD, 0x07f3},
	{SMART_CONFIG_PRM, 0x288a},
	{SMART_CONFIG_CMD, 0x07f4},
	{SMART_CONFIG_PRM, 0x0022},
	{SMART_CONFIG_CMD, 0x07f5},
	{SMART_CONFIG_PRM, 0x0001},
	{SMART_CONFIG_CMD, 0x07f0},
	{SMART_CONFIG_PRM, 0x0000},
	/* end of gamma corrected value table */
	{SMART_CONFIG_CMD, 0x0007},
	{SMART_CONFIG_PRM, 0x0173},
	/* Write Data to GRAM */
	{SMART_CONFIG_CMD, 0x202},
	{SMART_CONFIG_UDELAY, 10000},
	/* Set the start address of screen, for example (0, 0) */
	{SMART_CONFIG_CMD, 0x200},
	{SMART_CONFIG_PRM, 0},
	{SMART_CONFIG_UDELAY, 1},
	{SMART_CONFIG_CMD, 0x201},
	{SMART_CONFIG_PRM, 0},
	{SMART_CONFIG_UDELAY, 1},
	{SMART_CONFIG_CMD, 0x202},
	{SMART_CONFIG_UDELAY, 100},
};

static struct jzfb_smart_config kfm701a21_1a_cfg = {
	.frm_md = 0,
	.rdy_anti_jit = 0,
	.te_anti_jit = 1,
	.te_md = 0,
	.te_switch = 0,
	.rdy_switch = 0,
	.cs_en = 0,
	.cs_dp = 0,
	.rdy_dp = 1,
	.dc_md = 0,
	.wr_md = 1,
	.te_dp = 1,
	.smart_type = SMART_LCD_TYPE_8080,
	.pix_fmt = SMART_LCD_FORMAT_565,
	.dwidth = SMART_LCD_DWIDTH_16_BIT,
	.cwidth = SMART_LCD_CWIDTH_16_BIT,
	.bus_width = 16,

	.write_gram_cmd = 0x202,
	.data_table = kfm701a21_1a_data_table,
	.length_data_table = ARRAY_SIZE(kfm701a21_1a_data_table),
};

static struct lcdc_gpio_struct gpio_assign = {
    .cs = GPIO_PD(18),
/*    .rd = GPIO_PD(30),      //no use*/
    .rst = GPIO_PD(26),
    .func_pins = (1 << 27)|(0xf << 22)|(0x3f << 12)|(1 << 9)|(0x3f << 2),
    .func_port = GPIO_PORT_D,
    .func_num = GPIO_FUNC_1,
};

struct jzfb_platform_data jzfb_data = {
	.num_modes = 1,
	.modes = &jzfb_kfm701a21_1a_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 32,
	.width = 400,
	.height = 240,

	.smart_config = &kfm701a21_1a_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &kfm701a21_1a_ops,
};
