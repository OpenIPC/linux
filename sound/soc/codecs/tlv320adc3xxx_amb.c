/*
 * sound/soc/codecs/tlv320adc3xxx_amb.c
 *
 * Author: Dongge wu <dgwu@ambarella.com>
 *
 * History:
 *	2015/10/28 - [Dongge wu] Created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */


/***************************** INCLUDES ************************************/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "tlv320adc3xxx_amb.h"

/*
 * ****************************************************************************
 *  Macros
 * ****************************************************************************
 *
 */


/* codec private data */
struct adc3xxx_priv {
	void *control_data;
	unsigned int rst_pin;
	unsigned int rst_active;
	unsigned int sysclk;
	int master;
	u8 page_no;
};

static int adc3xxx_set_bias_level(struct snd_soc_codec *,
				  enum snd_soc_bias_level);
/*
 * ADC3xxx register cache
 * We can't read the ADC3xxx register space when we are
 * using 2 wire for device control, so we cache them instead.
 * There is no point in caching the reset register
 */
static const u8 adc3xxx_reg[ADC3xxx_CACHEREGNUM] = {
	0x00, 0x00, 0x00, 0x00,	/* 0 */
	0x00, 0x11, 0x04, 0x00,	/* 4 */
	0x00, 0x00, 0x00, 0x00,	/* 8 */
	0x00, 0x00, 0x00, 0x00,	/* 12 */
	0x00, 0x00, 0x01, 0x01,	/* 16 */
	0x80, 0x80, 0x04, 0x00,	/* 20 */
	0x00, 0x00, 0x01, 0x00,	/* 24 */
	0x00, 0x02, 0x01, 0x00,	/* 28 */
	0x00, 0x10, 0x00, 0x00,	/* 32 */
	0x00, 0x00, 0x02, 0x00,	/* 36 */
	0x00, 0x00, 0x00, 0x00,	/* 40 */
	0x00, 0x00, 0x00, 0x00,	/* 44 */
	0x00, 0x00, 0x00, 0x00,	/* 48 */
	0x00, 0x12, 0x00, 0x00,	/* 52 */
	0x00, 0x00, 0x00, 0x44,	/* 56 */
	0x00, 0x01, 0x00, 0x00,	/* 60 */
	0x00, 0x00, 0x00, 0x00,	/* 64 */
	0x00, 0x00, 0x00, 0x00,	/* 68 */
	0x00, 0x00, 0x00, 0x00,	/* 72 */
	0x00, 0x00, 0x00, 0x00,	/* 76 */
	0x00, 0x00, 0x88, 0x00,	/* 80 */
	0x00, 0x00, 0x00, 0x00,	/* 84 */
	0x7F, 0x00, 0x00, 0x00,	/* 88 */
	0x00, 0x00, 0x00, 0x00,	/* 92 */
	0x7F, 0x00, 0x00, 0x00,	/* 96 */
	0x00, 0x00, 0x00, 0x00,	/* 100 */
	0x00, 0x00, 0x00, 0x00,	/* 104 */
	0x00, 0x00, 0x00, 0x00,	/* 108 */
	0x00, 0x00, 0x00, 0x00,	/* 112 */
	0x00, 0x00, 0x00, 0x00,	/* 116 */
	0x00, 0x00, 0x00, 0x00,	/* 120 */
	0x00, 0x00, 0x00, 0x00,	/* 124 - PAGE0 Registers(127) ends here */
	0x00, 0x00, 0x00, 0x00,	/* 128, PAGE1-0 */
	0x00, 0x00, 0x00, 0x00,	/* 132, PAGE1-4 */
	0x00, 0x00, 0x00, 0x00,	/* 136, PAGE1-8 */
	0x00, 0x00, 0x00, 0x00,	/* 140, PAGE1-12 */
	0x00, 0x00, 0x00, 0x00,	/* 144, PAGE1-16 */
	0x00, 0x00, 0x00, 0x00,	/* 148, PAGE1-20 */
	0x00, 0x00, 0x00, 0x00,	/* 152, PAGE1-24 */
	0x00, 0x00, 0x00, 0x00,	/* 156, PAGE1-28 */
	0x00, 0x00, 0x00, 0x00,	/* 160, PAGE1-32 */
	0x00, 0x00, 0x00, 0x00,	/* 164, PAGE1-36 */
	0x00, 0x00, 0x00, 0x00,	/* 168, PAGE1-40 */
	0x00, 0x00, 0x00, 0x00,	/* 172, PAGE1-44 */
	0x00, 0x00, 0x00, 0x00,	/* 176, PAGE1-48 */
	0xFF, 0x00, 0x3F, 0xFF,	/* 180, PAGE1-52 */
	0x00, 0x3F, 0x00, 0x80,	/* 184, PAGE1-56 */
	0x80, 0x00, 0x00, 0x00,	/* 188, PAGE1-60 */
};

/*
 * adc3xxx initialization data
 * This structure initialization contains the initialization required for
 * ADC3xxx.
 * These registers values (reg_val) are written into the respective ADC3xxx
 * register offset (reg_offset) to  initialize ADC3xxx.
 * These values are used in adc3xxx_init() function only.
 */
struct adc3xxx_configs {
	u8 reg_offset;
	u8 reg_val;
};

/* The global Register Initialization sequence Array. During the Audio Driver
 * Initialization, this array will be utilized to perform the default
 * initialization of the audio Driver.
 */
static const struct adc3xxx_configs adc3xxx_reg_init[] = {
	/* Select IN1L/IN1R Single Ended (0dB) inputs
	 * use value 10 for no connection, this enables dapm
	 * to switch ON/OFF inputs using MS Bit only.
	 * with this setup, -6dB input option is not used.
	 */
	{LEFT_PGA_SEL_1, 0xA8},
	{LEFT_PGA_SEL_2, 0x37},
	{RIGHT_PGA_SEL_1, 0x28},
	/* mute Left PGA + default gain */
	{LEFT_APGA_CTRL, 0xc6},
	/* mute Right PGA + default gain */
	{RIGHT_APGA_CTRL, 0xc6},
	{LEFT_CHN_AGC_1, 0x80},
	{RIGHT_CHN_AGC_1, 0x80},
	/* MICBIAS1=2.5V, MICBIAS2=2.5V */
	{MICBIAS_CTRL, 0x50},
	/* Use PLL for generating clocks from MCLK */
	{CLKGEN_MUX, USE_PLL},
	/* I2S, 16LE, codec as Master */
	{INTERFACE_CTRL_1, 0x0C},
	/*BDIV_CLKIN = ADC_CLK, BCLK & WCLK active when power-down */
	{INTERFACE_CTRL_2, 0x02},

	#ifdef ADC3101_CODEC_SUPPORT
	/* Use Primary BCLK and WCLK */
	{INTERFACE_CTRL_4, 0x0},
	#endif

	/* Left AGC Maximum Gain to 40db */
	{LEFT_CHN_AGC_3, 0x50},
	/* Right AGC Maximum Gain to 40db */
	{RIGHT_CHN_AGC_3, 0X50},
	/* ADC control, Gain soft-stepping disabled */
	{ADC_DIGITAL, 0x02},
	/* Fine Gain 0dB, Left/Right ADC Unmute */
	{ADC_FGA, 0x00},

	#ifdef ADC3101_CODEC_SUPPORT
	/* DMCLK output = ADC_MOD_CLK */
	{GPIO2_CTRL, 0x28},
	/* DMDIN is in Dig_Mic_In mode */
	{GPIO1_CTRL, 0x04},
	#endif
};

/*
 * PLL and Clock settings
 */
static const struct adc3xxx_rate_divs adc3xxx_divs[] = {
	/*  mclk, rate, p, r, j, d, nadc, madc, aosr, bdiv */
	/* 8k rate */
	{12288000, 8000, 1, 1, 7, 0000, 42, 2, 128, 8, 188},
	/* 11.025k rate */
	//{12000000, 11025, 1, 1, 6, 8208, 29, 2, 128, 8, 188},
	/* 16k rate */
	{12288000, 16000, 1, 1, 7, 0000, 21, 2, 128, 8, 188},
	/* 22.05k rate */
	//{12000000, 22050, 1, 1, 7, 560, 15, 2, 128, 8, 188},
	/* 32k rate */
	{12288000, 32000, 1, 1, 8, 0000, 12, 2, 128, 8, 188},
	/* 44.1k rate */
	//{12000000, 44100, 1, 1, 7, 5264, 8, 2, 128, 8, 188},
	/* 48k rate */
	{12288000, 48000, 1, 1, 7, 0000, 7, 2, 128, 8, 188},
	/* 88.2k rate */
	//{12000000, 88200, 1, 1, 7, 5264, 4, 4, 64, 8, 188},
	/* 96k rate */
	//{12000000, 96000, 1, 1, 8, 1920, 4, 4, 64, 8, 188},
};

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_get_divs
 * Purpose  : This function is to get required divisor from the "adc3xxx_divs"
 *            table.
 *
 *----------------------------------------------------------------------------
 */
static inline int adc3xxx_get_divs(int mclk, int rate)
{
	int i;

	printk(KERN_INFO "adc3xxx_get_divs mclk = %d, rate = %d\n", mclk, rate);
	for (i = 0; i < ARRAY_SIZE(adc3xxx_divs); i++) {
		if ((adc3xxx_divs[i].rate == rate)
		    && (adc3xxx_divs[i].mclk == mclk)) {
			return i;
		}
	}

	printk("Master clock and sample rate is not supported\n");
	return -EINVAL;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_change_page
 * Purpose  : This function is to switch between page 0 and page 1.
 *
 *----------------------------------------------------------------------------
 */
int adc3xxx_change_page(struct snd_soc_codec *codec, u8 new_page)
{
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);

	if (i2c_smbus_write_byte_data(codec->control_data, 0x0, new_page) < 0) {
		printk("adc3xxx_change_page: I2C Wrte Error\n");
		return -1;
	}
	adc3xxx->page_no = new_page;
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_write_reg_cache
 * Purpose  : This function is to write adc3xxx register cache
 *
 *----------------------------------------------------------------------------
 */
static inline void adc3xxx_write_reg_cache(struct snd_soc_codec *codec,
					   unsigned int reg, unsigned int value)
{
	u8 *cache = codec->reg_cache;
	if (reg >= ADC3xxx_CACHEREGNUM)
		return;
	cache[reg] = value & 0xFF;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_read_reg_cache
 * Purpose  : This function is to read adc3xxx register cache
 *
 *----------------------------------------------------------------------------
 */
static inline unsigned int adc3xxx_read_reg_cache(struct snd_soc_codec *codec,
						  unsigned int reg)
{
	u8 *cache = codec->reg_cache;
	if (reg >= ADC3xxx_CACHEREGNUM)
		return -1;
	return cache[reg];
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_write
 * Purpose  : This function is to write to the adc3xxx register space.
 *
 *----------------------------------------------------------------------------
 */
int adc3xxx_write(struct snd_soc_codec *codec, unsigned int reg,
		  unsigned int value)
{
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);
	u8 data[2];
	u8 page;

	page = reg / ADC3xxx_PAGE_SIZE;

	if (adc3xxx->page_no != page) {
		adc3xxx_change_page(codec, page);
	}

	/* data is
	 *   D15..D8 adc3xxx register offset
	 *   D7...D0 register data
	 */
	data[0] = reg % ADC3xxx_PAGE_SIZE;
	data[1] = value & 0xFF;

	if (i2c_smbus_write_byte_data(codec->control_data, data[0], data[1]) < 0) {
		printk("adc3xxx_write: I2C write Error\n");
		return -EIO;
	}

	/* Update register cache */
	if ((page == 0) || (page == 1)) {
		adc3xxx_write_reg_cache(codec, reg, data[1]);
	}
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_read
 * Purpose  : This function is to read the adc3xxx register space.
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);
	u8 value, page;

	page = reg / ADC3xxx_PAGE_SIZE;
	if (adc3xxx->page_no != page) {
		adc3xxx_change_page(codec, page);
	}

	/* write register address */
	reg = reg % ADC3xxx_PAGE_SIZE;

	/*  read register value */
	value = i2c_smbus_read_word_data(codec->control_data, reg);
	if (value < 0) {
		printk("adc3xxx_read: I2C read Error\n");
		return -EIO;
	}

	/* Update register cache */
	if ((page == 0) || (page == 1)) {
		adc3xxx_write_reg_cache(codec, reg, value);
	}
	return value;
}

/*
 *----------------------------------------------------------------------------
 * Function : snd_soc_adc3xxx_put_volsw
 * Purpose  : Callback to set the value of a mixer control.
 *
 *----------------------------------------------------------------------------
 */
static int snd_soc_adc3xxx_put_volsw(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	s8 val1, val2;
	u8 reg;

	val1 = ucontrol->value.integer.value[0];
	val2 = ucontrol->value.integer.value[1];

	if ((val1 >= ADC_POS_VOL)) {
		if (val1 > ADC_MAX_VOLUME)
			val1 = ADC_MAX_VOLUME;
		val1 = val1 - ADC_POS_VOL;
	} else if ((val1 >= 0) && (val1 <= 23)) {
		val1 = ADC_POS_VOL - val1;
		val1 = 128 - val1;
	} else
		return -EINVAL;

	if (val2 >= ADC_POS_VOL) {
		if (val2 > ADC_MAX_VOLUME)
			val2 = ADC_MAX_VOLUME;
		val2 = val2 - ADC_POS_VOL;
	} else if ((val2 >= 0) && (val2 <= 23)) {
		val2 = ADC_POS_VOL - val2;
		val2 = 128 - val2;
	} else
		return -EINVAL;

	reg = adc3xxx_read_reg_cache(codec, LADC_VOL) & (~0x7F);
	adc3xxx_write(codec, LADC_VOL, reg | (val1 << 0));
	reg = adc3xxx_read_reg_cache(codec, RADC_VOL) & (~0x7F);
	adc3xxx_write(codec, RADC_VOL, reg | (val2 << 0));

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : snd_soc_adc3xxx_get_volsw
 * Purpose  : Callback to get the value of a mixer control.
 *
 *----------------------------------------------------------------------------
 */
static int snd_soc_adc3xxx_get_volsw(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 val1;
	u8 val2;

	val1 = adc3xxx_read_reg_cache(codec, LADC_VOL) & (0x7F);
	if ((val1 >= 0) && (val1 <= 40)) {
		val1 = val1 + ADC_POS_VOL;
	} else if ((val1 >= 104) && (val1 <= 127)) {
		val1 = val1 - 104;
	} else
		return -EINVAL;

	val2 = adc3xxx_read_reg_cache(codec, RADC_VOL) & (0x7F);
	if ((val2 >= 0) && (val2 <= 40)) {
		val2 = val2 + ADC_POS_VOL;
	} else if ((val2 >= 104) && (val2 <= 127)) {
		val2 = val2 - 104;
	} else
		return -EINVAL;

	ucontrol->value.integer.value[0] = val1;
	ucontrol->value.integer.value[1] = val2;
	return 0;

}

#define SOC_ADC3xxx_DOUBLE_R(xname, reg_left, reg_right, xshift, xmax, xinvert) \
{       .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
        .get = snd_soc_adc3xxx_get_volsw, .put = snd_soc_adc3xxx_put_volsw, \
        .private_value = (unsigned long)&(struct soc_mixer_control) \
                {.reg = reg_left, .rreg = reg_right, .shift = xshift, \
                .max = xmax, .invert = xinvert} }

static const char *micbias_voltage[] = { "off", "2V", "2.5V", "AVDD" };
static const char *linein_attenuation[] = { "0db", "-6db" };
static const char *adc_softstepping[] = { "1 step", "2 step", "off" };

#define MICBIAS1_ENUM		0
#define MICBIAS2_ENUM		1
#define ATTLINEL1_ENUM		2
#define ATTLINEL2_ENUM		3
#define ATTLINEL3_ENUM		4
#define ATTLINER1_ENUM		5
#define ATTLINER2_ENUM		6
#define ATTLINER3_ENUM		7
#define ADCSOFTSTEP_ENUM	8

/* Creates an array of the Single Ended Widgets*/
static const struct soc_enum adc3xxx_enum[] = {
	SOC_ENUM_SINGLE(MICBIAS_CTRL, 5, 4, micbias_voltage),
	SOC_ENUM_SINGLE(MICBIAS_CTRL, 3, 4, micbias_voltage),
	SOC_ENUM_SINGLE(LEFT_PGA_SEL_1, 0, 2, linein_attenuation),
	SOC_ENUM_SINGLE(LEFT_PGA_SEL_1, 2, 2, linein_attenuation),
	SOC_ENUM_SINGLE(LEFT_PGA_SEL_1, 4, 2, linein_attenuation),
	SOC_ENUM_SINGLE(RIGHT_PGA_SEL_1, 0, 2, linein_attenuation),
	SOC_ENUM_SINGLE(RIGHT_PGA_SEL_1, 2, 2, linein_attenuation),
	SOC_ENUM_SINGLE(RIGHT_PGA_SEL_1, 4, 2, linein_attenuation),
	SOC_ENUM_SINGLE(ADC_DIGITAL, 0, 3, adc_softstepping),
};

/* Various Controls For adc3xxx */
static const struct snd_kcontrol_new adc3xxx_snd_controls[] = {
	/* PGA Gain Volume Control */
	SOC_DOUBLE_R("PGA Gain Volume Control (0=0dB, 80=40dB)",
		    LEFT_APGA_CTRL, RIGHT_APGA_CTRL, 0, 0x50, 0),
	/* Audio gain control (AGC) */
	SOC_DOUBLE_R("Audio Gain Control (AGC)", LEFT_CHN_AGC_1,
		     RIGHT_CHN_AGC_1, 7, 0x01, 0),
	/* AGC Target level control */
	SOC_DOUBLE_R("AGC Target Level Control", LEFT_CHN_AGC_1,
		     RIGHT_CHN_AGC_1, 4, 0x07, 1),
	/* AGC Maximum PGA applicable */
	SOC_DOUBLE_R("AGC Maximum PGA Control", LEFT_CHN_AGC_3,
		     RIGHT_CHN_AGC_3, 0, 0x50, 0),
	/* AGC Attack Time control */
	SOC_DOUBLE_R("AGC Attack Time control", LEFT_CHN_AGC_4,
		     RIGHT_CHN_AGC_4, 3, 0x1F, 0),
	/* AGC Decay Time control */
	SOC_DOUBLE_R("AGC Decay Time control", LEFT_CHN_AGC_5,
		     RIGHT_CHN_AGC_5, 3, 0x1F, 0),
	/* AGC Noise Bounce control */
	SOC_DOUBLE_R("AGC Noise bounce control", LEFT_CHN_AGC_6,
		     RIGHT_CHN_AGC_6, 0, 0x1F, 0),
	/* AGC Signal Bounce control */
	SOC_DOUBLE_R("AGC Signal bounce control", LEFT_CHN_AGC_7,
		     RIGHT_CHN_AGC_7, 0, 0x0F, 0),
	/* Mic Bias voltage */
	SOC_ENUM("Mic Bias 1 Voltage", adc3xxx_enum[MICBIAS1_ENUM]),
	SOC_ENUM("Mic Bias 2 Voltage", adc3xxx_enum[MICBIAS2_ENUM]),
	/* ADC soft stepping */
	SOC_ENUM("ADC soft stepping", adc3xxx_enum[ADCSOFTSTEP_ENUM]),
	/* Left/Right Input attenuation */
	SOC_ENUM("Left Linein1 input attenuation",
		 adc3xxx_enum[ATTLINEL1_ENUM]),
	SOC_ENUM("Left Linein2 input attenuation",
		 adc3xxx_enum[ATTLINEL2_ENUM]),
	SOC_ENUM("Left Linein3 input attenuation",
		 adc3xxx_enum[ATTLINEL3_ENUM]),
	SOC_ENUM("Right Linein1 input attenuation",
		 adc3xxx_enum[ATTLINER1_ENUM]),
	SOC_ENUM("Right Linein2 input attenuation",
		 adc3xxx_enum[ATTLINER2_ENUM]),
	SOC_ENUM("Right Linein3 input attenuation",
		 adc3xxx_enum[ATTLINER3_ENUM]),
	/* ADC Volume */
	SOC_ADC3xxx_DOUBLE_R("ADC Volume Control (0=-12dB, 64=+20dB)", LADC_VOL,
		 RADC_VOL, 0, 64,0),
	/* ADC Fine Volume */
	SOC_SINGLE("Left ADC Fine Volume (0=-0.4dB, 4=0dB)", ADC_FGA, 4, 4, 1),
	SOC_SINGLE("Right ADC Fine Volume (0=-0.4dB, 4=0dB)", ADC_FGA, 0, 4, 1),
};

/* Left input selection, Single Ended inputs and Differential inputs */
static const struct snd_kcontrol_new left_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("IN1_L switch", LEFT_PGA_SEL_1, 1, 0x1, 1),
	SOC_DAPM_SINGLE("IN2_L switch", LEFT_PGA_SEL_1, 3, 0x1, 1),
	SOC_DAPM_SINGLE("IN3_L switch", LEFT_PGA_SEL_1, 5, 0x1, 1),
	SOC_DAPM_SINGLE("DIF1_L switch", LEFT_PGA_SEL_1, 7, 0x1, 1),
	SOC_DAPM_SINGLE("DIF2_L switch", LEFT_PGA_SEL_2, 5, 0x1, 1),
	SOC_DAPM_SINGLE("DIF3_L switch", LEFT_PGA_SEL_2, 3, 0x1, 1), //DIF3_L
	SOC_DAPM_SINGLE("IN1_R switch", LEFT_PGA_SEL_2, 1, 0x1, 1),
};

/* Right input selection, Single Ended inputs and Differential inputs */
static const struct snd_kcontrol_new right_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("IN1_R switch", RIGHT_PGA_SEL_1, 1, 0x1, 1),
	SOC_DAPM_SINGLE("IN2_R switch", RIGHT_PGA_SEL_1, 3, 0x1, 1),
	SOC_DAPM_SINGLE("IN3_R switch", RIGHT_PGA_SEL_1, 5, 0x1, 1),
	SOC_DAPM_SINGLE("DIF1_R switch", RIGHT_PGA_SEL_1, 7, 0x1, 1), // DIF1_R
	SOC_DAPM_SINGLE("DIF2_R switch", RIGHT_PGA_SEL_2, 5, 0x1, 1),
	SOC_DAPM_SINGLE("DIF3_R switch", RIGHT_PGA_SEL_2, 3, 0x1, 1),
	SOC_DAPM_SINGLE("IN1_L switch", RIGHT_PGA_SEL_2, 1, 0x1, 1),
};

/* Left Digital Mic input for left ADC */
static const struct snd_kcontrol_new left_input_dmic_controls[] = {
	SOC_DAPM_SINGLE("Left ADC switch", ADC_DIGITAL, 3, 0x1, 0),
};

/* Right Digital Mic input for Right ADC */
static const struct snd_kcontrol_new right_input_dmic_controls[] = {
	SOC_DAPM_SINGLE("Right ADC switch", ADC_DIGITAL, 2, 0x1, 0),
};

/* adc3xxx Widget structure */
static const struct snd_soc_dapm_widget adc3xxx_dapm_widgets[] = {

	/* Left Input Selection */
	SND_SOC_DAPM_MIXER("Left Input Selection", SND_SOC_NOPM, 0, 0,
			   &left_input_mixer_controls[0],
			   ARRAY_SIZE(left_input_mixer_controls)),
	/* Right Input Selection */
	SND_SOC_DAPM_MIXER("Right Input Selection", SND_SOC_NOPM, 0, 0,
			   &right_input_mixer_controls[0],
			   ARRAY_SIZE(right_input_mixer_controls)),
	/*PGA selection */
	SND_SOC_DAPM_PGA("Left PGA", LEFT_APGA_CTRL, 7, 1, NULL, 0),
	SND_SOC_DAPM_PGA("Right PGA", RIGHT_APGA_CTRL, 7, 1, NULL, 0),

	/*Digital Microphone Input Control for Left/Right ADC */
	SND_SOC_DAPM_MIXER("Left DMic Input", SND_SOC_NOPM, 0, 0,
			&left_input_dmic_controls[0],
			ARRAY_SIZE(left_input_dmic_controls)),
	SND_SOC_DAPM_MIXER("Right DMic Input", SND_SOC_NOPM , 0, 0,
			&right_input_dmic_controls[0],
			ARRAY_SIZE(right_input_dmic_controls)),

	/* Left/Right ADC */
	SND_SOC_DAPM_ADC("Left ADC", "Left Capture", ADC_DIGITAL, 7, 0),
	SND_SOC_DAPM_ADC("Right ADC", "Right Capture", ADC_DIGITAL, 6, 0),

	/* Inputs */
	SND_SOC_DAPM_INPUT("IN1_L"),
	SND_SOC_DAPM_INPUT("IN1_R"),
	SND_SOC_DAPM_INPUT("IN2_L"),
	SND_SOC_DAPM_INPUT("IN2_R"),
	SND_SOC_DAPM_INPUT("IN3_L"),
	SND_SOC_DAPM_INPUT("IN3_R"),
	SND_SOC_DAPM_INPUT("DIF1_L"),
	SND_SOC_DAPM_INPUT("DIF2_L"),
	SND_SOC_DAPM_INPUT("DIF3_L"),
	SND_SOC_DAPM_INPUT("DIF1_R"),
	SND_SOC_DAPM_INPUT("DIF2_R"),
	SND_SOC_DAPM_INPUT("DIF3_R"),
	SND_SOC_DAPM_INPUT("DMic_L"),
	SND_SOC_DAPM_INPUT("DMic_R"),

};

/* DAPM Routing related array declaratiom */
static const struct snd_soc_dapm_route intercon[] = {
/* Left input selection from switchs */
	{"Left Input Selection", "IN1_L switch", "IN1_L"},
	{"Left Input Selection", "IN2_L switch", "IN2_L"},
	{"Left Input Selection", "IN3_L switch", "IN3_L"},
	{"Left Input Selection", "DIF1_L switch", "DIF1_L"},
	{"Left Input Selection", "DIF2_L switch", "DIF2_L"},
	{"Left Input Selection", "DIF3_L switch", "DIF3_L"},
	{"Left Input Selection", "IN1_R switch", "IN1_R"},

/* Left input selection to left PGA */
	{"Left PGA", NULL, "Left Input Selection"},

/* Left PGA to left ADC */
	{"Left ADC", NULL, "Left PGA"},

/* Right input selection from switchs */
	{"Right Input Selection", "IN1_R switch", "IN1_R"},
	{"Right Input Selection", "IN2_R switch", "IN2_R"},
	{"Right Input Selection", "IN3_R switch", "IN3_R"},
	{"Right Input Selection", "DIF1_R switch", "DIF1_R"},
	{"Right Input Selection", "DIF2_R switch", "DIF2_R"},
	{"Right Input Selection", "DIF3_R switch", "DIF3_R"},
	{"Right Input Selection", "IN1_L switch", "IN1_L"},

/* Right input selection to right PGA */
	{"Right PGA", NULL, "Right Input Selection"},

/* Right PGA to right ADC */
	{"Right ADC", NULL, "Right PGA"},

/* Left DMic Input selection from switch */
	{"Left DMic Input", "Left ADC switch", "DMic_L"},

/* Left DMic to left ADC */
	{"Left ADC", NULL, "Left DMic Input"},

/* Right DMic Input selection from switch */
	{"Right DMic Input", "Right ADC switch", "DMic_R"},

/* Right DMic to right ADC */
	{"Right ADC", NULL, "Right DMic Input"},
};

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_hw_params
 * Purpose  : This function is to set the hardware parameters for adc3xxx.
 *            The functions set the sample rate and audio serial data word
 *            length.
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);
	int i, width = 16;
	u8 data, bdiv;

	i = adc3xxx_get_divs(adc3xxx->sysclk, params_rate(params));

	if (i < 0) {
		printk("Clock configuration is not supported\n");
		return i;
	}

	/* select data word length */
	data =
	    adc3xxx_read_reg_cache(codec, INTERFACE_CTRL_1) & (~WLENGTH_MASK);
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		width = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		data |= (0x01 << 4);
		width = 20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		data |= (0x02 << 4);
		width = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		data |= (0x03 << 4);
		width = 32;
		break;
	}
	adc3xxx_write(codec, INTERFACE_CTRL_1, data);

	/* BCLK is derived from ADC_CLK */
	if (width == 16) {
		bdiv = adc3xxx_divs[i].bdiv_n;
	} else {
		bdiv =
		    (adc3xxx_divs[i].aosr * adc3xxx_divs[i].madc) / (2 * width);
	}

	/* P & R values */
	adc3xxx_write(codec, PLL_PROG_PR,
		      (adc3xxx_divs[i].pll_p << PLLP_SHIFT) | (adc3xxx_divs[i].
							       pll_r <<
							       PLLR_SHIFT));
	/* J value */
	adc3xxx_write(codec, PLL_PROG_J, adc3xxx_divs[i].pll_j & PLLJ_MASK);
	/* D value */
	adc3xxx_write(codec, PLL_PROG_D_LSB,
		      adc3xxx_divs[i].pll_d & PLLD_LSB_MASK);
	adc3xxx_write(codec, PLL_PROG_D_MSB,
		      (adc3xxx_divs[i].pll_d >> 8) & PLLD_MSB_MASK);
	/* NADC */
	adc3xxx_write(codec, ADC_NADC, adc3xxx_divs[i].nadc & NADC_MASK);
	/* MADC */
	adc3xxx_write(codec, ADC_MADC, adc3xxx_divs[i].madc & MADC_MASK);
	/* AOSR */
	adc3xxx_write(codec, ADC_AOSR, adc3xxx_divs[i].aosr & AOSR_MASK);
	/* BDIV N Value */
	adc3xxx_write(codec, BCLK_N_DIV, bdiv & BDIV_MASK);
	msleep(10);

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_set_dai_sysclk
 * Purpose  : This function is to set the DAI system clock
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);

	adc3xxx->sysclk = freq;
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_set_dai_fmt
 * Purpose  : This function is to set the DAI format
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);
	u8 iface_reg;

	iface_reg =
	    adc3xxx_read_reg_cache(codec, INTERFACE_CTRL_1) & (~FMT_MASK);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		adc3xxx->master = 1;
		iface_reg |= BCLK_MASTER | WCLK_MASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		adc3xxx->master = 0;
		iface_reg &= ~ (BCLK_MASTER | WCLK_MASTER ); //new repladec | with &
		break;
	case SND_SOC_DAIFMT_CBS_CFM: //new case..just for debugging
		printk("%s: SND_SOC_DAIFMT_CBS_CFM\n", __FUNCTION__);
		adc3xxx->master = 0;
		/* BCLK by codec ie BCLK output */
		iface_reg |=  (BCLK_MASTER);
		iface_reg &= ~(WCLK_MASTER);

		break;
	default:
		printk("Invalid DAI master/slave interface\n");
		return -EINVAL;
	}

	/*
	 * match both interface format and signal polarities since they
	 * are fixed
	 */
	switch (fmt & (SND_SOC_DAIFMT_FORMAT_MASK | SND_SOC_DAIFMT_INV_MASK)) {
	case (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF):
		break;
	case (SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF):
		iface_reg |= (0x01 << 6);
		break;
	case (SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_IB_NF):
		iface_reg |= (0x01 << 6);
		break;
	case (SND_SOC_DAIFMT_RIGHT_J | SND_SOC_DAIFMT_NB_NF):
		iface_reg |= (0x02 << 6);
		break;
	case (SND_SOC_DAIFMT_LEFT_J | SND_SOC_DAIFMT_NB_NF):
		iface_reg |= (0x03 << 6);
		break;
	default:
		printk("Invalid DAI format\n");
		return -EINVAL;
	}

	/* set iface */
	adc3xxx_write(codec, INTERFACE_CTRL_1, iface_reg);
	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_set_bias_level
 * Purpose  : This function is to get triggered when dapm events occurs.
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	struct adc3xxx_priv *adc3xxx = snd_soc_codec_get_drvdata(codec);
	u8 reg;

	/* Check if  the New Bias level is equal to the existing one, if so return */
	if (codec->dapm.bias_level  == level)
	return 0;

	/* all power is driven by DAPM system */
	switch (level) {
	case SND_SOC_BIAS_ON:
		if (adc3xxx->master) {
			/* Enable pll */
			reg = adc3xxx_read_reg_cache(codec, PLL_PROG_PR);
			adc3xxx_write(codec, PLL_PROG_PR, reg | ENABLE_PLL);

			/* 10msec delay needed after PLL power-up */
			mdelay(10);

			/* Switch on NADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_NADC);
			adc3xxx_write(codec, ADC_NADC, reg | ENABLE_NADC);

			/* Switch on MADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_MADC);
			adc3xxx_write(codec, ADC_MADC, reg | ENABLE_MADC);

			/* Switch on BCLK_N Divider */
			reg = adc3xxx_read_reg_cache(codec, BCLK_N_DIV);
			adc3xxx_write(codec, BCLK_N_DIV, reg | ENABLE_BCLK);
		}
	      else{ //new
			/* Enable pll */
			reg = adc3xxx_read_reg_cache(codec, PLL_PROG_PR);
			adc3xxx_write(codec, PLL_PROG_PR, reg | ENABLE_PLL);

			/* 10msec delay needed after PLL power-up */
			mdelay(10);

			/*Switch on NADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_NADC);
			adc3xxx_write(codec, ADC_NADC, reg | ENABLE_NADC);

			/* Switch on MADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_MADC);
			adc3xxx_write(codec, ADC_MADC, reg | ENABLE_MADC);

			/* Switch on BCLK_N Divider */
			reg = adc3xxx_read_reg_cache(codec, BCLK_N_DIV);
			adc3xxx_write(codec, BCLK_N_DIV, reg | ~ENABLE_BCLK);

		}
		 msleep(350);
		break;

		/* partial On */
	case SND_SOC_BIAS_PREPARE:
		break;

		/* Off, with power */
	case SND_SOC_BIAS_STANDBY:
		if (adc3xxx->master) {
			/* switch off pll */
			reg = adc3xxx_read_reg_cache(codec, PLL_PROG_PR);
			adc3xxx_write(codec, PLL_PROG_PR, reg & (~ENABLE_PLL));
			msleep(10);

			/* Switch off NADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_NADC);
			adc3xxx_write(codec, ADC_NADC, reg & (~ENABLE_NADC));

			/* Switch off MADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_MADC);
			adc3xxx_write(codec, ADC_MADC, reg & (~ENABLE_MADC));

			/* Switch off BCLK_N Divider */
			reg = adc3xxx_read_reg_cache(codec, BCLK_N_DIV);
			adc3xxx_write(codec, BCLK_N_DIV, reg & (~ENABLE_BCLK));
			msleep(100);
		}
		break;

		/* Off without power */
	case SND_SOC_BIAS_OFF:

		/* power off Left/Right ADC channels */
		reg = adc3xxx_read_reg_cache(codec, ADC_DIGITAL);
		adc3xxx_write(codec, ADC_DIGITAL,
			      reg & ~(LADC_PWR_ON | RADC_PWR_ON));

		/* Turn off PLLs */
		if (adc3xxx->master) {
			/* switch off pll */
			reg = adc3xxx_read_reg_cache(codec, PLL_PROG_PR);
			adc3xxx_write(codec, PLL_PROG_PR, reg & (~ENABLE_PLL));

			/* Switch off NADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_NADC);
			adc3xxx_write(codec, ADC_NADC, reg & (~ENABLE_NADC));

			/* Switch off MADC Divider */
			reg = adc3xxx_read_reg_cache(codec, ADC_MADC);
			adc3xxx_write(codec, ADC_MADC, reg & (~ENABLE_MADC));

			/* Switch off BCLK_N Divider */
			reg = adc3xxx_read_reg_cache(codec, BCLK_N_DIV);
			adc3xxx_write(codec, BCLK_N_DIV, reg & (~ENABLE_BCLK));
		}
		break;
	}
	codec->dapm.bias_level = level;

	return 0;
}

static struct snd_soc_dai_ops adc3xxx_dai_ops = {
	.hw_params	= adc3xxx_hw_params,
	.set_sysclk = adc3xxx_set_dai_sysclk,
	.set_fmt	= adc3xxx_set_dai_fmt,
};

struct snd_soc_dai_driver adc3xxx_dai = {
	.name = "tlv320adc3xxx-hifi",
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 2,
		    .rates = ADC3xxx_RATES,
		    .formats = ADC3xxx_FORMATS,
		    },
	.ops = &adc3xxx_dai_ops,
};

EXPORT_SYMBOL_GPL(adc3xxx_dai);

/*
 *----------------------------------------------------------------------------
 * Function : tlv320adc3xxx_init
 * Purpose  : This function is to initialise the adc3xxx driver
 *            register the mixer and dsp interfaces with the kernel.
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_init(struct snd_soc_codec *codec)
{
	int i, ret = 0;

	codec->name = "tlv320adc3xxx";
	codec->num_dai = 1;
	codec->reg_cache =
		kmemdup(adc3xxx_reg, sizeof(adc3xxx_reg), GFP_KERNEL);
	if (codec->reg_cache == NULL)
		return -ENOMEM;

	/* Select Page 0 */
	adc3xxx_write(codec, PAGE_SELECT, 0);
	/* Issue software reset to adc3xxx */
	adc3xxx_write(codec, RESET, SOFT_RESET);

	adc3xxx_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	for (i = 0;
	     i < sizeof(adc3xxx_reg_init) / sizeof(struct adc3xxx_configs);
	     i++) {
		adc3xxx_write(codec, adc3xxx_reg_init[i].reg_offset,
			      adc3xxx_reg_init[i].reg_val);
	}

	//adc3xxx_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return ret;

	kfree(codec->reg_cache);
	return ret;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_probe
 * Purpose  : This is first driver function called by the SoC core driver.
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_probe(struct snd_soc_codec *codec)
{
	struct adc3xxx_priv *adc3xxx = NULL;
	int ret = 0;

	printk(KERN_INFO "ADC3xxx Audio Codec %s provided by Amba Dongge\n", ADC3xxx_VERSION);
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	codec->write = adc3xxx_write;
	codec->read = adc3xxx_read;
#endif

	adc3xxx = (struct adc3xxx_priv *)snd_soc_codec_get_drvdata(codec);
	codec->control_data = adc3xxx->control_data;
	mutex_init(&codec->mutex);

	ret = devm_gpio_request(codec->dev, adc3xxx->rst_pin, "adc3xxx reset");
	if (ret < 0){
		dev_err(codec->dev, "Failed to request rst_pin: %d\n", ret);
		return ret;
	}

	/* Reset adc3xxx codec */
	gpio_direction_output(adc3xxx->rst_pin, adc3xxx->rst_active);
	msleep(2);
	gpio_direction_output(adc3xxx->rst_pin, !adc3xxx->rst_active);

	ret = adc3xxx_init(codec);

	if (ret < 0) {
		printk(KERN_ERR "adc3xxx: failed to initialise ADC3xxx\n");
		devm_gpio_free(codec->dev, adc3xxx->rst_pin);
	}

	return ret;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_remove
 * Purpose  : to remove adc3xxx soc device
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_remove(struct snd_soc_codec *codec)
{
	adc3xxx_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_suspend
 * Purpose  : This function is to suspend the adc3xxx driver.
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_suspend(struct snd_soc_codec *codec)
{
	adc3xxx_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_resume
 * Purpose  : This function is to resume the ADC3xxx driver
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_resume(struct snd_soc_codec *codec)
{
	snd_soc_cache_sync(codec);
	adc3xxx_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

struct snd_soc_codec_driver soc_codec_dev_adc3xxx = {
	.probe = adc3xxx_probe,
	.remove = adc3xxx_remove,
	.suspend =	adc3xxx_suspend,
	.resume =	adc3xxx_resume,
	.set_bias_level = adc3xxx_set_bias_level,
	.reg_cache_default	= adc3xxx_reg,
	.reg_cache_size		= ARRAY_SIZE(adc3xxx_reg),
	.reg_word_size		= sizeof(u8),
	.reg_cache_step		= 1,

	.controls = adc3xxx_snd_controls,
	.num_controls = ARRAY_SIZE(adc3xxx_snd_controls),
	.dapm_widgets = adc3xxx_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(adc3xxx_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};
EXPORT_SYMBOL_GPL(soc_codec_dev_adc3xxx);


#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_i2c_probe
 * Purpose  : This function attaches the i2c client and initializes
 *				adc3xxx CODEC.
 *            NOTE:
 *            This function is called from i2c core when the I2C address is
 *            valid.
 *            If the i2c layer weren't so broken, we could pass this kind of
 *            data around
 *
 *----------------------------------------------------------------------------
 */
static int adc3xxx_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct device_node *np = i2c->dev.of_node;
	struct adc3xxx_priv *adc3xxx = NULL;
	enum of_gpio_flags flags;
	int rst_pin;
	int ret;

	adc3xxx = kzalloc(sizeof(struct adc3xxx_priv), GFP_KERNEL);
	if (adc3xxx == NULL) {
		return -ENOMEM;
	}

	rst_pin = of_get_gpio_flags(np, 0, &flags);
	if (rst_pin < 0 || !gpio_is_valid(rst_pin))
		return -ENXIO;

	adc3xxx->rst_pin = rst_pin;
	adc3xxx->rst_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	adc3xxx->control_data = i2c;

	i2c_set_clientdata(i2c, adc3xxx);
	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_adc3xxx, &adc3xxx_dai, 1);
	if (ret < 0){
		kfree(adc3xxx);
		printk("\t[adc3xxx Error!] %s(%d)\n",__FUNCTION__,__LINE__);
	}

	return ret;
}

/*
 *----------------------------------------------------------------------------
 * Function : adc3xxx_i2c_remove
 * Purpose  : This function removes the i2c client and uninitializes
 *                              adc3xxx CODEC.
 *            NOTE:
 *            This function is called from i2c core
 *            If the i2c layer weren't so broken, we could pass this kind of
 *            data around
 *
 *----------------------------------------------------------------------------
 */
static int __exit adc3xxx_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static struct of_device_id tlv320adc3xxx_of_match[] = {
	{ .compatible = "ambarella,tlv320adc3xxx",},
	{},
};
MODULE_DEVICE_TABLE(of, tlv320adc3xxx_of_match);


static const struct i2c_device_id adc3xxx_i2c_id[] = {
	{"tlv320adc3xxx", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, adc3xxx_i2c_id);

/* machine i2c codec control layer */
static struct i2c_driver adc3xxx_i2c_driver = {
	.driver = {
		   .name = "tlv320adc3xxx-codec",
		   .owner = THIS_MODULE,
		   .of_match_table = tlv320adc3xxx_of_match,
		   },
	.probe = adc3xxx_i2c_probe,
	.remove = adc3xxx_i2c_remove,
	.id_table = adc3xxx_i2c_id,
};
#endif

static int __init tlv320adc3xxx_init(void)
{
	return i2c_add_driver(&adc3xxx_i2c_driver);
}

static void __exit tlv320adc3xxx_exit(void)
{
	i2c_del_driver(&adc3xxx_i2c_driver);
}

module_init(tlv320adc3xxx_init);
module_exit(tlv320adc3xxx_exit);

MODULE_DESCRIPTION("ASoC TLV320ADC3xxx codec driver");
MODULE_AUTHOR("dgwu@ambarella.com ");
MODULE_LICENSE("GPL");
