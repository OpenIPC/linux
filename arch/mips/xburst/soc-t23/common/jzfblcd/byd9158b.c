/*
 * ingenic_bsp/chip-x2000/fpga/dpu/byd9158b.c
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
#include <linux/delay.h>

#include <mach/jzfb.h>
#include <soc/gpio.h>

//#define SEL_SLCD_SPI_4
#define SEL_SLCD_SPI_3

static int inited = 0;

static int byd9158B_set_gpio_func(struct lcdc_gpio_struct *gpio)
{
	return jzgpio_set_func(gpio->func_port, gpio->func_num, gpio->func_pins);
}

static int byd9158B_power_initialize(struct lcdc_gpio_struct *gpio)
{
	int ret;

	ret = byd9158B_set_gpio_func(gpio);
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

	inited = 1;

	return 0;
}

static int byd9158B_power_reset(struct lcdc_gpio_struct *gpio)
{
	gpio_direction_output(gpio->rst, 0);
	mdelay(20);
	gpio_direction_output(gpio->rst, 1);
	mdelay(10);

	return 0;
}

static int byd9158B_power_on(struct lcdc_gpio_struct *gpio)
{
	if (!inited && byd9158B_power_initialize(gpio))
		return -EFAULT;

	gpio_direction_output(gpio->cs, 1);
	byd9158B_power_reset(gpio);
	mdelay(5);
	gpio_direction_output(gpio->cs, 0);

	return 0;
}

static int byd9158B_power_off(struct lcdc_gpio_struct *gpio)
{
	gpio_direction_output(gpio->cs, 0);
	gpio_direction_output(gpio->rst, 0);

	return 0;
}

static struct lcd_callback_ops byd9158B_ops = {
	.lcd_power_on_begin  = (void*)byd9158B_power_on,
	.lcd_power_off_begin = (void*)byd9158B_power_off,
};

static struct smart_lcd_data_table byd9158B_data_table[] = {
	/*soft reset*/
	{SMART_CONFIG_CMD, 0x01},
	{SMART_CONFIG_UDELAY, 120000},

	/*sleep out*/
	{SMART_CONFIG_CMD, 0x11},
	{SMART_CONFIG_UDELAY, 120000},

	/*565 16bit RGB format*/
	{SMART_CONFIG_CMD, 0x3A},
	{SMART_CONFIG_PRM, 0x05},

	{SMART_CONFIG_CMD, 0xB1},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x30},
	{SMART_CONFIG_PRM, 0x30},

	{SMART_CONFIG_CMD, 0xB2},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x30},
	{SMART_CONFIG_PRM, 0x30},

	{SMART_CONFIG_CMD, 0xB3},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x30},
	{SMART_CONFIG_PRM, 0x30},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x30},
	{SMART_CONFIG_PRM, 0x30},

	/*Memory Data Access Control MY MX MV ML RGB MH*/
	{SMART_CONFIG_CMD, 0x36},
	{SMART_CONFIG_PRM, 0x00},

	{SMART_CONFIG_CMD, 0xB6},
	{SMART_CONFIG_PRM, 0x15},
	{SMART_CONFIG_PRM, 0x02},

	/*Display Inversion Control*/
	{SMART_CONFIG_CMD, 0xB4},
	{SMART_CONFIG_PRM, 0x07},

	{SMART_CONFIG_CMD, 0xC0},
	{SMART_CONFIG_PRM, 0x1D},
	{SMART_CONFIG_PRM, 0x30},

	{SMART_CONFIG_CMD, 0xC1},
	{SMART_CONFIG_PRM, 0x00},

	{SMART_CONFIG_CMD, 0xC2},
	{SMART_CONFIG_PRM, 0x01},
	{SMART_CONFIG_PRM, 0x01},

	{SMART_CONFIG_CMD, 0xC5},
	{SMART_CONFIG_PRM, 0x51},
	{SMART_CONFIG_PRM, 0x4d},

	{SMART_CONFIG_CMD, 0xFC},
	{SMART_CONFIG_PRM, 0x11},
	{SMART_CONFIG_PRM, 0x15},

	{SMART_CONFIG_CMD, 0xE0},
	{SMART_CONFIG_PRM, 0x09}, //01
	{SMART_CONFIG_PRM, 0x16}, //02
	{SMART_CONFIG_PRM, 0x09}, //03
	{SMART_CONFIG_PRM, 0x20}, //04
	{SMART_CONFIG_PRM, 0x21}, //05
	{SMART_CONFIG_PRM, 0x1B}, //06
	{SMART_CONFIG_PRM, 0x13}, //07
	{SMART_CONFIG_PRM, 0x19}, //08
	{SMART_CONFIG_PRM, 0x17}, //09
	{SMART_CONFIG_PRM, 0x15}, //10
	{SMART_CONFIG_PRM, 0x1E}, //11
	{SMART_CONFIG_PRM, 0x2B}, //12
	{SMART_CONFIG_PRM, 0x04}, //13
	{SMART_CONFIG_PRM, 0x05}, //14
	{SMART_CONFIG_PRM, 0x02}, //15
	{SMART_CONFIG_PRM, 0x0E}, //16

	{SMART_CONFIG_CMD, 0xE1},
	{SMART_CONFIG_PRM, 0x0B}, //01
	{SMART_CONFIG_PRM, 0x14}, //02
	{SMART_CONFIG_PRM, 0x08}, //03
	{SMART_CONFIG_PRM, 0x1E}, //04
	{SMART_CONFIG_PRM, 0x22}, //05
	{SMART_CONFIG_PRM, 0x1D}, //06
	{SMART_CONFIG_PRM, 0x18}, //07
	{SMART_CONFIG_PRM, 0x1E}, //08
	{SMART_CONFIG_PRM, 0x1B}, //09
	{SMART_CONFIG_PRM, 0x1A}, //10
	{SMART_CONFIG_PRM, 0x24}, //11
	{SMART_CONFIG_PRM, 0x2B}, //12
	{SMART_CONFIG_PRM, 0x06}, //13
	{SMART_CONFIG_PRM, 0x06}, //14
	{SMART_CONFIG_PRM, 0x02}, //15
	{SMART_CONFIG_PRM, 0x0F}, //16

	{SMART_CONFIG_CMD, 0x2A},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x7F},

	{SMART_CONFIG_CMD, 0x2B},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x00},
	{SMART_CONFIG_PRM, 0x9F},
//	{SMART_CONFIG_CMD, 0x2A},
//	{SMART_CONFIG_PRM, 0x00},
//	{SMART_CONFIG_PRM, 0x02},
//	{SMART_CONFIG_PRM, 0x00},
//	{SMART_CONFIG_PRM, 0x81},
//
//	{SMART_CONFIG_CMD, 0x2B},
//	{SMART_CONFIG_PRM, 0x00},
//	{SMART_CONFIG_PRM, 0x01},
//	{SMART_CONFIG_PRM, 0x00},
//	{SMART_CONFIG_PRM, 0xA0},

	{SMART_CONFIG_CMD, 0x13},

	{SMART_CONFIG_CMD, 0x2C},

	{SMART_CONFIG_CMD, 0x29},
	{SMART_CONFIG_UDELAY, 120000},
};

static struct jzfb_smart_config byd9158B_cfg = {
	.frm_md = 0,
	.rdy_anti_jit = 0,
	.rdy_switch = 0,
	.cs_en = 0,
	.cs_dp = 0,
	.rdy_dp = 1,
	.dc_md = 0,
	.wr_md = 1,
#if defined(SEL_SLCD_SPI_4)
	.smart_type = SMART_LCD_TYPE_SPI_4,
#elif defined(SEL_SLCD_SPI_3)
	.smart_type = SMART_LCD_TYPE_SPI_3,
#else
	.smart_type = SMART_LCD_TYPE_SPI_3,
#endif
	/*data_table set format 565*/
	.pix_fmt = SMART_LCD_FORMAT_565,

	.write_gram_cmd = 0x2c,
	.data_table = byd9158B_data_table,
	.length_data_table = ARRAY_SIZE(byd9158B_data_table),
};

static struct lcdc_gpio_struct gpio_assign = {
    .cs = GPIO_PD(18),
    .rst = GPIO_PD(26),
    .func_pins = (1 << 27) | (1 << 9) | (1 << 2),
    .func_port = GPIO_PORT_D,
    .func_num = GPIO_FUNC_1,
};

static struct fb_videomode jzfb_byd9158b_videomode = {
	.name = "128*160",
	.refresh = 60,
	.xres = 128,
	.yres = 160,
	.pixclock = KHZ2PICOS(84000),
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

struct jzfb_platform_data jzfb_data = {
	.num_modes = 1,
	.modes = &jzfb_byd9158b_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 32,
	.width = 128,
	.height = 160,

	.smart_config = &byd9158B_cfg,

	.dither_enable = 0,
	.dither.dither_red = 0,
	.dither.dither_green = 0,
	.dither.dither_blue = 0,
	.gpio_assign = &gpio_assign,
	.lcd_callback_ops = &byd9158B_ops,
};
