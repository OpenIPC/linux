/*
 * ingenic_bsp/chip-x2000/fpga/dpu/truly_240_240.c
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

static int truly_slcd240240_set_gpio_func(struct lcdc_gpio_struct *gpio)
{
	return jzgpio_set_func(gpio->func_port, gpio->func_num, gpio->func_pins);
}

static int truly_slcd240240_power_initialize(struct lcdc_gpio_struct *gpio)
{
	int ret;

	ret = truly_slcd240240_set_gpio_func(gpio);
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

	ret = gpio_request(gpio->rd, "lcd rd");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd\n");
		return ret;
	}

	inited = 1;

	return 0;
}

static int truly_slcd240240_power_reset(struct lcdc_gpio_struct *gpio)
{
	gpio_direction_output(gpio->rst, 0);
	mdelay(20);
	gpio_direction_output(gpio->rst, 1);
	mdelay(10);

	return 0;
}

static int truly_slcd240240_power_on(struct lcdc_gpio_struct *gpio)
{
	if (!inited && truly_slcd240240_power_initialize(gpio))
		return -EFAULT;

	gpio_direction_output(gpio->cs, 1);
	gpio_direction_output(gpio->rd, 1);
	truly_slcd240240_power_reset(gpio);
	mdelay(5);
	gpio_direction_output(gpio->cs, 0);

	return 0;
}

static int truly_slcd240240_power_off(struct lcdc_gpio_struct *gpio)
{
	gpio_direction_output(gpio->cs, 0);
	gpio_direction_output(gpio->rd, 0);
	gpio_direction_output(gpio->rst, 0);

	return 0;
}

static struct lcd_callback_ops truly_slcd240240_ops = {
	.lcd_power_on_begin  = (void*)truly_slcd240240_power_on,
	.lcd_power_off_begin = (void*)truly_slcd240240_power_off,
};

static struct smart_lcd_data_table truly_slcd240240_data_table[] = {
    /* LCD init code */
    {SMART_CONFIG_CMD, 0x01},		/* soft reset, 120 ms = 120 000 us */
    {SMART_CONFIG_UDELAY, 120000},
    {SMART_CONFIG_CMD, 0x11},
    {SMART_CONFIG_UDELAY, 5000},	/* sleep out 5 ms  */

    {SMART_CONFIG_CMD, 0x36},
#ifdef	CONFIG_TRULY_240X240_ROTATE_180
    /*{0x36, 0xc0, 2, 0}, //40*/
    {SMART_CONFIG_PRM, 0xd0}, //40
#else
    {SMART_CONFIG_PRM, 0x00}, //40
#endif

    {SMART_CONFIG_CMD, 0x2a},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0xef},

    {SMART_CONFIG_CMD, 0x2b},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0xef},


    {SMART_CONFIG_CMD, 0x3a},
#if defined(CONFIG_SLCD_TRULY_18BIT)	/* if 18bit/pixel unusual. try to use 16bit/pixel */
    {SMART_CONFIG_PRM, 0x06}, //6-6-6
#else
    {SMART_CONFIG_PRM, 0x05}, //5-6-5
#endif
    //	{SMART_CONFIG_PRM, 0x55},

    {SMART_CONFIG_CMD, 0xb2},
    {SMART_CONFIG_PRM, 0x7f},
    {SMART_CONFIG_PRM, 0x7f},
    {SMART_CONFIG_PRM, 0x01},
    {SMART_CONFIG_PRM, 0xde},
    {SMART_CONFIG_PRM, 0x33},

    {SMART_CONFIG_CMD, 0xb3},
    {SMART_CONFIG_PRM, 0x10},
    {SMART_CONFIG_PRM, 0x05},
    {SMART_CONFIG_PRM, 0x0f},

    {SMART_CONFIG_CMD, 0xb4},
    {SMART_CONFIG_PRM, 0x0b},

    {SMART_CONFIG_CMD, 0xb7},
    {SMART_CONFIG_PRM, 0x35},

    {SMART_CONFIG_CMD, 0xbb},
    {SMART_CONFIG_PRM, 0x28}, //23

    {SMART_CONFIG_CMD, 0xbc},
    {SMART_CONFIG_PRM, 0xec},

    {SMART_CONFIG_CMD, 0xc0},
    {SMART_CONFIG_PRM, 0x2c},

    {SMART_CONFIG_CMD, 0xc2},
    {SMART_CONFIG_PRM, 0x01},

    {SMART_CONFIG_CMD, 0xc3},
    {SMART_CONFIG_PRM, 0x1e}, //14

    {SMART_CONFIG_CMD, 0xc4},
    {SMART_CONFIG_PRM, 0x20},

    {SMART_CONFIG_CMD, 0xc6},
    /*{SMART_CONFIG_PRM, 0x14},*/
    {SMART_CONFIG_PRM, 0x1F},

    {SMART_CONFIG_CMD, 0xd0},
    {SMART_CONFIG_PRM, 0xa4},
    {SMART_CONFIG_PRM, 0xa1},

    {SMART_CONFIG_CMD, 0xe0},
    {SMART_CONFIG_PRM, 0xd0},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x05},
    {SMART_CONFIG_PRM, 0x29},
    {SMART_CONFIG_PRM, 0x54},
    {SMART_CONFIG_PRM, 0x41},
    {SMART_CONFIG_PRM, 0x3c},
    {SMART_CONFIG_PRM, 0x17},
    {SMART_CONFIG_PRM, 0x15},
    {SMART_CONFIG_PRM, 0x1a},
    {SMART_CONFIG_PRM, 0x20},

    {SMART_CONFIG_CMD, 0xe1},
    {SMART_CONFIG_PRM, 0xd0},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x00},
    {SMART_CONFIG_PRM, 0x08},
    {SMART_CONFIG_PRM, 0x07},
    {SMART_CONFIG_PRM, 0x04},
    {SMART_CONFIG_PRM, 0x29},
    {SMART_CONFIG_PRM, 0x44},
    {SMART_CONFIG_PRM, 0x42},
    {SMART_CONFIG_PRM, 0x3b},
    {SMART_CONFIG_PRM, 0x16},
    {SMART_CONFIG_PRM, 0x15},
    {SMART_CONFIG_PRM, 0x1b},
    {SMART_CONFIG_PRM, 0x1f},

    {SMART_CONFIG_CMD, 0x35}, // TE on
    {SMART_CONFIG_PRM, 0x00}, // TE mode: 0, mode1; 1, mode2
    //	{SMART_CONFIG_CMD, 0x34}, // TE off

    /* {SMART_CONFIG_CMD, 0x30}, */
    /* {SMART_CONFIG_PRM, 0}, */
    /* {SMART_CONFIG_PRM, 0x30}, */
    /* {SMART_CONFIG_PRM, 0}, */
    /* {SMART_CONFIG_PRM, 0xc0}, */
    /* {SMART_CONFIG_CMD, 0x12}, */


    {SMART_CONFIG_CMD, 0x29}, //Display ON

    /* set window size*/
    //	{SMART_CONFIG_CMD, 0xcd},
    {SMART_CONFIG_CMD, 0x2a},
    {SMART_CONFIG_PRM, 0},
    {SMART_CONFIG_PRM, 0},
    {SMART_CONFIG_PRM, (239>> 8) & 0xff},
    {SMART_CONFIG_PRM, 239 & 0xff},
#ifdef	CONFIG_TRULY_240X240_ROTATE_180
    {SMART_CONFIG_CMD, 0x2b},
    {SMART_CONFIG_PRM, ((320-240)>>8)&0xff},
    {SMART_CONFIG_PRM, ((320-240)>>0)&0xff},
    {SMART_CONFIG_PRM, ((320-1)>>8) & 0xff},
    {SMART_CONFIG_PRM, ((320-1)>>0) & 0xff},
#else
    {SMART_CONFIG_CMD, 0x2b},
    {SMART_CONFIG_PRM, 0},
    {SMART_CONFIG_PRM, 0},
    {SMART_CONFIG_PRM, (239>> 8) & 0xff},
    {SMART_CONFIG_PRM, 239 & 0xff},
#endif
    {SMART_CONFIG_CMD, 0x2C},
    {SMART_CONFIG_CMD, 0x2C},
    {SMART_CONFIG_CMD, 0x2C},
    {SMART_CONFIG_CMD, 0x2C},
};

static struct fb_videomode jzfb_truly_240_240_videomode = {
	.name = "240x240",
	.refresh = 60,
	.xres = 240,
	.yres = 240,
	.pixclock = KHZ2PICOS(30000),
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

static struct jzfb_smart_config truly_240_240_cfg = {
	.frm_md = 1,
	.rdy_anti_jit = 0,
	.te_anti_jit = 1,
	.te_md = 0,
	.te_dp = 0,
	.te_switch = 1,
	.rdy_switch = 0,
	.cs_en = 0,
	.cs_dp = 0,
	.rdy_dp = 1,
	.dc_md = 0,
	.wr_md = 1,
	.te_dp = 1,
	.smart_type = SMART_LCD_TYPE_8080,
	.pix_fmt = SMART_LCD_FORMAT_565,
	.dwidth = SMART_LCD_DWIDTH_8_BIT,
	.cwidth = SMART_LCD_CWIDTH_8_BIT,
	.bus_width = 8,

	.write_gram_cmd = 0x2c,
	.data_table = truly_slcd240240_data_table,
	.length_data_table = ARRAY_SIZE(truly_slcd240240_data_table),
};

static struct lcdc_gpio_struct gpio_assign = {
    .cs = GPIO_PD(23),
    .rd = GPIO_PD(22),
    .rst = GPIO_PD(24),
    .func_pins = 0x80832fc,
    .func_port = GPIO_PORT_D,
    .func_num = GPIO_FUNC_1,
};

struct jzfb_platform_data jzfb_data = {
	.num_modes = 1,
	.modes = &jzfb_truly_240_240_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 32,
	.width = 240,
	.height = 240,

	.smart_config = &truly_240_240_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &truly_slcd240240_ops,
};
