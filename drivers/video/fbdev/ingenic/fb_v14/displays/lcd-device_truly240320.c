/*
 * Copyright (c) 2016 Engenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * JZ-T20 orion board lcd setup routines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include "../include/jzfb.h"

/******GPIO PIN************/
#undef GPIO_LCD_CS
#undef GPIO_LCD_RD
#undef GPIO_BL_PWR_EN

#define GPIO_LCD_CS            106
#define GPIO_BL_PWR_EN         79
#define GPIO_LCD_RST           105
/* not use */
/* #define GPIO_LCD_RD            108 */


/*ifdef is 18bit,6-6-6 ,ifndef default 5-6-5*/
/* #define CONFIG_SLCD_ILI9488_18BIT */

#ifdef	CONFIG_SLCD_TRULY_18BIT
static int slcd_inited = 1;
#else
static int slcd_inited = 0;
#endif

struct truly_tft240320_power{
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct truly_tft240320_power lcd_power = {
	NULL,
	NULL,
	0
};

int truly_tft240320_power_init(struct lcd_device *ld)
{
	int ret ;
	/* printk("======truly_tft240320_power_init==============\n"); */

	ret = gpio_request(GPIO_LCD_RST, "lcd rst");
	if (ret) {
		printk(KERN_ERR "can's request lcd rst\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_CS, "lcd cs");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs\n");
		return ret;
	}
/*
	ret = gpio_request(GPIO_LCD_CS2, "lcd cs2");
	if (ret) {
		printk(KERN_ERR "can's request lcd cs2\n");
		return ret;
	}

	ret = gpio_request(GPIO_LCD_RD, "lcd rd");
	if (ret) {
		printk(KERN_ERR "can's request lcd rd\n");
		return ret;
	}
*/

	lcd_power.inited = 1;
	return 0;
}

int truly_tft240320_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;

	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(100);
	gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(200);
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(100);
	return 0;
}

int truly_tft240320_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && truly_tft240320_power_init(ld))
		return -EFAULT;

	if (enable) {
		gpio_direction_output(GPIO_LCD_CS, 1);
		/* gpio_direction_output(GPIO_LCD_CS2, 1); */
		/* gpio_direction_output(GPIO_LCD_RD, 1); */
		truly_tft240320_power_reset(ld);
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		/* gpio_direction_output(GPIO_LCD_CS2, 0); */

	} else {
		gpio_direction_output(GPIO_BL_PWR_EN, 0);
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		/* gpio_direction_output(GPIO_LCD_CS2, 0); */
		/* gpio_direction_output(GPIO_LCD_RD, 0); */
		gpio_direction_output(GPIO_LCD_RST, 0);
		slcd_inited = 0;
	}
	return 0;
}

struct lcd_platform_data truly_tft240320_pdata = {
	.reset    = truly_tft240320_power_reset,
	.power_on = truly_tft240320_power_on,
};

/* LCD Panel Device */
struct platform_device truly_tft240320_device = {
	.name		= "truly_tft240320_slcd",
	.dev		= {
		.platform_data	= &truly_tft240320_pdata,
	},
};

static struct smart_lcd_data_table truly_tft240320_data_table[] = {
	{SMART_CONFIG_CMD, 0x11},
	{SMART_CONFIG_UDELAY, 60000},

	{SMART_CONFIG_CMD, 0x3a},
	{SMART_CONFIG_DATA, 0x05},

	{SMART_CONFIG_CMD, 0xf6},
	{SMART_CONFIG_DATA, 0x01},
	{SMART_CONFIG_DATA, 0x33},

	{SMART_CONFIG_CMD, 0xb5},
	{SMART_CONFIG_DATA, 0x04},
	{SMART_CONFIG_DATA, 0x04},
	{SMART_CONFIG_DATA, 0x0a},
	{SMART_CONFIG_DATA, 0x14},

	{SMART_CONFIG_CMD, 0x35},
	{SMART_CONFIG_DATA, 0x00},

	{SMART_CONFIG_CMD, 0xcf},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0xea},
	{SMART_CONFIG_DATA, 0xf8},

	{SMART_CONFIG_CMD, 0xed},
	{SMART_CONFIG_DATA, 0x64},
	{SMART_CONFIG_DATA, 0x03},
	{SMART_CONFIG_DATA, 0x12},
	{SMART_CONFIG_DATA, 0x81},

	{SMART_CONFIG_CMD, 0xe8},
	{SMART_CONFIG_DATA, 0x85},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x78},

	{SMART_CONFIG_CMD, 0xcb},
	{SMART_CONFIG_DATA, 0x39},
	{SMART_CONFIG_DATA, 0x2c},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x33},
	{SMART_CONFIG_DATA, 0x06},

	{SMART_CONFIG_CMD, 0xf7},
	{SMART_CONFIG_DATA, 0x20},

	{SMART_CONFIG_CMD, 0xea},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x00},

	{SMART_CONFIG_CMD, 0xc0},
	{SMART_CONFIG_DATA, 0x21},

	{SMART_CONFIG_CMD, 0xc1},
	{SMART_CONFIG_DATA, 0x10},

	{SMART_CONFIG_CMD, 0xc5},
	{SMART_CONFIG_DATA, 0x30}, //4f
	{SMART_CONFIG_DATA, 0x30}, //38

	{SMART_CONFIG_CMD, 0x36},
	{SMART_CONFIG_DATA, 0x28},

	{SMART_CONFIG_CMD, 0xb1},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x13},

	{SMART_CONFIG_CMD, 0xb6},
	{SMART_CONFIG_DATA, 0x0a},
	{SMART_CONFIG_DATA, 0x82},

	{SMART_CONFIG_CMD, 0xf2},
	{SMART_CONFIG_DATA, 0x02},

	{SMART_CONFIG_CMD, 0xe0},
	{SMART_CONFIG_DATA, 0x0f},
	{SMART_CONFIG_DATA, 0x27},
	{SMART_CONFIG_DATA, 0x24},
	{SMART_CONFIG_DATA, 0x0c},
	{SMART_CONFIG_DATA, 0x10},
	{SMART_CONFIG_DATA, 0x08},
	{SMART_CONFIG_DATA, 0x55},
	{SMART_CONFIG_DATA, 0x87},
	{SMART_CONFIG_DATA, 0x45},
	{SMART_CONFIG_DATA, 0x08},
	{SMART_CONFIG_DATA, 0x14},
	{SMART_CONFIG_DATA, 0x07},
	{SMART_CONFIG_DATA, 0x13},
	{SMART_CONFIG_DATA, 0x08},
	{SMART_CONFIG_DATA, 0x00},

	{SMART_CONFIG_CMD, 0xe1},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x0f},
	{SMART_CONFIG_DATA, 0x12},
	{SMART_CONFIG_DATA, 0x05},
	{SMART_CONFIG_DATA, 0x11},
	{SMART_CONFIG_DATA, 0x06},
	{SMART_CONFIG_DATA, 0x25},
	{SMART_CONFIG_DATA, 0x34},
	{SMART_CONFIG_DATA, 0x37},
	{SMART_CONFIG_DATA, 0x01},
	{SMART_CONFIG_DATA, 0x08},
	{SMART_CONFIG_DATA, 0x07},
	{SMART_CONFIG_DATA, 0x2b},
	{SMART_CONFIG_DATA, 0x34},
	{SMART_CONFIG_DATA, 0x0f},

	{SMART_CONFIG_CMD, 0x2a},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x01},
	{SMART_CONFIG_DATA, 0xdf},

	{SMART_CONFIG_CMD, 0x2b},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x00},
	{SMART_CONFIG_DATA, 0x01},
	{SMART_CONFIG_DATA, 0x3f},

	{SMART_CONFIG_DATA, 0x2c},

	{SMART_CONFIG_DATA, 0x11},
	{SMART_CONFIG_UDELAY, 120000},

	{SMART_CONFIG_CMD, 0x29},
};

unsigned long truly_cmd_buf[]= {
	0x2C2C2C2C,
};

struct fb_videomode jzfb0_videomode = {
	.name = "240x320",
	.refresh = 60,
	.xres = 320,
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

struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb0_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp    = 16,
//	.bpp    = 18,
	.width = 320,
	.height = 240,
	.pinmd  = 0,
	/* rsply_cmd_high setting of LCDC should correspond with LCD.  */
	.smart_config.rsply_cmd_high       = 0,
	.smart_config.csply_active_high    = 0,
	.smart_config.cfg_fmt_conv =  1,
	/* write graphic ram command, in word, for example 8-bit bus, write_gram_cmd=C3C2C1C0. */
	.smart_config.write_gram_cmd = truly_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(truly_cmd_buf),
	.smart_config.bus_width = 8,
	.smart_config.length_data_table =  ARRAY_SIZE(truly_tft240320_data_table),
	.smart_config.data_table = truly_tft240320_data_table,
	.dither_enable = 0,
};
/**************************************************************************************************/
#ifdef CONFIG_BACKLIGHT_PWM
static int backlight_init(struct device *dev)
{
	int ret;
	ret = gpio_request(GPIO_LCD_PWM, "Backlight");
	if (ret) {
		printk(KERN_ERR "failed to request GPF for PWM-OUT1\n");
		return ret;
	}

	ret = gpio_request(GPIO_BL_PWR_EN, "BL PWR");
	if (ret) {
		printk(KERN_ERR "failed to reqeust BL PWR\n");
		return ret;
	}
	gpio_direction_output(GPIO_BL_PWR_EN, 1);
	return 0;
}

static int backlight_notify(struct device *dev, int brightness)
{
	if (brightness)
		gpio_direction_output(GPIO_BL_PWR_EN, 1);
	else
		gpio_direction_output(GPIO_BL_PWR_EN, 0);

	return brightness;
}

static void backlight_exit(struct device *dev)
{
	gpio_free(GPIO_LCD_PWM);
}

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 120,
	.pwm_period_ns	= 30000,
	.init		= backlight_init,
	.exit		= backlight_exit,
	.notify		= backlight_notify,
};

struct platform_device backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.platform_data	= &backlight_data,
	},
};
#endif
