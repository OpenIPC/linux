/*
 * es8328.c  --  ES8328 ALSA Soc Audio driver
 *
 * Copyright 2011 Ambarella Ltd.
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Based on es8328.c from Everest Semiconductor
 *
 * History:
 *	2011/10/19 - [Cao Rongrong] Created file
 *	2013/01/14 - [Ken He] Port to 3.8
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/es8328.h>

#include "es8328.h"

#define ES8328_VERSION "v1.0"

/* codec private data */
struct es8328_priv {
	unsigned int sysclk;
	void *control_data;
};


/*
 * es8328 register cache
 * We can't read the es8328 register space when we
 * are using 2 wire for device control, so we cache them instead.
 */
static const u8 es8328_reg[] = {
	0x06, 0x1C, 0xC3, 0xFC,  /*  0 */
	0xC0, 0x00, 0x00, 0x7C,  /*  4 */
	0x80, 0x00, 0x00, 0x06,  /*  8 */
	0x00, 0x06, 0x30, 0x30,  /* 12 */
	0xC0, 0xC0, 0x38, 0xB0,  /* 16 */
	0x32, 0x06, 0x00, 0x00,  /* 20 */
	0x06, 0x32, 0xC0, 0xC0,  /* 24 */
	0x08, 0x06, 0x1F, 0xF7,  /* 28 */
	0xFD, 0xFF, 0x1F, 0xF7,  /* 32 */
	0xFD, 0xFF, 0x00, 0x38,  /* 36 */
	0x38, 0x38, 0x38, 0x38,  /* 40 */
	0x38, 0x00, 0x00, 0x00,  /* 44 */
	0x00, 0x00, 0x00, 0x00,  /* 48 */
	0x00, 0x00, 0x00, 0x00,  /* 52 */
};

/* DAC/ADC Volume: min -96.0dB (0xC0) ~ max 0dB (0x00)  ( 0.5 dB step ) */
static const DECLARE_TLV_DB_SCALE(digital_tlv, -9600, 50, 0);
/* Analog Out Volume: min -30.0dB (0x00) ~ max 3dB (0x21)  ( 1 dB step ) */
static const DECLARE_TLV_DB_SCALE(out_tlv, -3000, 100, 0);
/* Analog In Volume: min 0dB (0x00) ~ max 24dB (0x08)  ( 3 dB step ) */
static const DECLARE_TLV_DB_SCALE(in_tlv, 0, 300, 0);

static const struct snd_kcontrol_new es8328_snd_controls[] = {
	SOC_DOUBLE_R_TLV("Playback Volume",
		ES8328_LDAC_VOL, ES8328_RDAC_VOL, 0, 0xC0, 1, digital_tlv),
	SOC_DOUBLE_R_TLV("Analog Out Volume",
		ES8328_LOUT1_VOL, ES8328_ROUT1_VOL, 0, 0x1D, 0, out_tlv),

	SOC_DOUBLE_R_TLV("Capture Volume",
		ES8328_LADC_VOL, ES8328_RADC_VOL, 0, 0xC0, 1, digital_tlv),
	SOC_DOUBLE_TLV("Analog In Volume",
		ES8328_ADCCONTROL1, 4, 0, 0x08, 0, in_tlv),
};

/*
 * DAPM Controls
 */

/* Channel Input Mixer */
static const char *es8328_line_texts[] = { "Line 1", "Line 2", "Differential"};
static const unsigned int es8328_line_values[] = { 0, 1, 3};

static const struct soc_enum es8328_lline_enum =
	SOC_VALUE_ENUM_SINGLE(ES8328_ADCCONTROL2, 6, 0xC0,
		ARRAY_SIZE(es8328_line_texts), es8328_line_texts,
		es8328_line_values);
static const struct snd_kcontrol_new es8328_left_line_controls =
	SOC_DAPM_VALUE_ENUM("Route", es8328_lline_enum);

static const struct soc_enum es8328_rline_enum =
	SOC_VALUE_ENUM_SINGLE(ES8328_ADCCONTROL2, 4, 0x30,
		ARRAY_SIZE(es8328_line_texts), es8328_line_texts,
		es8328_line_values);
static const struct snd_kcontrol_new es8328_right_line_controls =
	SOC_DAPM_VALUE_ENUM("Route", es8328_lline_enum);


/* Left Mixer */
static const struct snd_kcontrol_new es8328_left_mixer_controls[] = {
	SOC_DAPM_SINGLE("Left Playback Switch", ES8328_DACCONTROL17, 7, 1, 0),
	SOC_DAPM_SINGLE("Left Bypass Switch"  , ES8328_DACCONTROL17, 6, 1, 0),
};

/* Right Mixer */
static const struct snd_kcontrol_new es8328_right_mixer_controls[] = {
	SOC_DAPM_SINGLE("Right Playback Switch", ES8328_DACCONTROL20, 7, 1, 0),
	SOC_DAPM_SINGLE("Right Bypass Switch"  , ES8328_DACCONTROL20, 6, 1, 0),
};

/* Mono ADC Mux */
static const char *es8328_mono_mux[] = {"Stereo", "Mono (Left)", "Mono (Right)", "NONE"};
static const struct soc_enum monomux =
	SOC_ENUM_SINGLE(ES8328_ADCCONTROL3, 3, 4, es8328_mono_mux);
static const struct snd_kcontrol_new es8328_monomux_controls =
	SOC_DAPM_ENUM("Route", monomux);

static const struct snd_soc_dapm_widget es8328_dapm_widgets[] = {
	/* DAC Part */
	SND_SOC_DAPM_MIXER("Left Mixer", SND_SOC_NOPM, 0, 0,
		&es8328_left_mixer_controls[0], ARRAY_SIZE(es8328_left_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right Mixer", SND_SOC_NOPM, 0, 0,
		&es8328_right_mixer_controls[0], ARRAY_SIZE(es8328_right_mixer_controls)),

	SND_SOC_DAPM_MUX("Left Line Mux", SND_SOC_NOPM, 0, 0, &es8328_left_line_controls),
	SND_SOC_DAPM_MUX("Right Line Mux", SND_SOC_NOPM, 0, 0, &es8328_right_line_controls),

	SND_SOC_DAPM_DAC("Left DAC"  , "Left Playback" , ES8328_DACPOWER, 7, 1),
	SND_SOC_DAPM_DAC("Right DAC" , "Right Playback", ES8328_DACPOWER, 6, 1),
	SND_SOC_DAPM_PGA("Left Out 1" , ES8328_DACPOWER, 5, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Right Out 1", ES8328_DACPOWER, 4, 0, NULL, 0),
	/* SND_SOC_DAPM_PGA("Left Out 2" , ES8328_DACPOWER, 3, 0, NULL, 0), */
	/* SND_SOC_DAPM_PGA("Right Out 2", ES8328_DACPOWER, 2, 0, NULL, 0), */

	SND_SOC_DAPM_OUTPUT("LOUT1"),
	SND_SOC_DAPM_OUTPUT("ROUT1"),
	SND_SOC_DAPM_OUTPUT("LOUT2"),
	SND_SOC_DAPM_OUTPUT("ROUT2"),

	/* ADC Part */
	SND_SOC_DAPM_MUX("Left ADC Mux", SND_SOC_NOPM, 0, 0, &es8328_monomux_controls),
	SND_SOC_DAPM_MUX("Right ADC Mux", SND_SOC_NOPM, 0, 0, &es8328_monomux_controls),

	SND_SOC_DAPM_PGA("Left Analog Input" , ES8328_ADCPOWER, 7, 1, NULL, 0),
	SND_SOC_DAPM_PGA("Right Analog Input", ES8328_ADCPOWER, 6, 1, NULL, 0),
	SND_SOC_DAPM_ADC("Left ADC" , "Left Capture" , ES8328_ADCPOWER, 5, 1),
	SND_SOC_DAPM_ADC("Right ADC", "Right Capture", ES8328_ADCPOWER, 4, 1),

	SND_SOC_DAPM_MICBIAS("Mic Bias", ES8328_ADCPOWER, 3, 1),

	SND_SOC_DAPM_INPUT("MICIN"),
	SND_SOC_DAPM_INPUT("LINPUT1"),
	SND_SOC_DAPM_INPUT("LINPUT2"),
	SND_SOC_DAPM_INPUT("RINPUT1"),
	SND_SOC_DAPM_INPUT("RINPUT2"),
};

static const struct snd_soc_dapm_route intercon[] = {
	/* left mixer */
	{"Left Mixer", "Left Playback Switch", "Left DAC"},

	/* right mixer */
	{"Right Mixer", "Right Playback Switch", "Right DAC"},

	/* left out 1 */
	{"Left Out 1", NULL, "Left Mixer"},
	{"LOUT1", NULL, "Left Out 1"},

	/* right out 1 */
	{"Right Out 1", NULL, "Right Mixer"},
	{"ROUT1", NULL, "Right Out 1"},

	/* Left Line Mux */
	{"Left Line Mux", "Line 1", "LINPUT1"},
	{"Left Line Mux", "Line 2", "LINPUT2"},
	{"Left Line Mux", "Differential", "MICIN"},

	/* Right Line Mux */
	{"Right Line Mux", "Line 1", "RINPUT1"},
	{"Right Line Mux", "Line 2", "RINPUT2"},
	{"Right Line Mux", "Differential", "MICIN"},

	/* Left ADC Mux */
	{"Left ADC Mux", "Stereo", "Left Line Mux"},
//	{"Left ADC Mux", "Mono (Left)" , "Left Line Mux"},

	/* Right ADC Mux */
	{"Right ADC Mux", "Stereo", "Right Line Mux"},
//	{"Right ADC Mux", "Mono (Right)", "Right Line Mux"},

	/* ADC */
	{"Left ADC" , NULL, "Left ADC Mux"},
	{"Right ADC", NULL, "Right ADC Mux"},
};

static inline unsigned int es8328_read_reg_cache(struct snd_soc_codec *codec,
		unsigned int reg)
{
	u8 *cache = codec->reg_cache;

	if (reg >= ES8328_CACHEREGNUM)
		return -1;

	return cache[reg];
}

static int es8328_write(struct snd_soc_codec *codec, unsigned int reg,
			     unsigned int value)
{
	u8 *cache = codec->reg_cache;
	struct i2c_client *client = codec->control_data;

	if (reg >= ES8328_CACHEREGNUM)
		return -EIO;

	if (i2c_smbus_write_byte_data(client, reg, value)) {
		pr_err("ES8328: I2C write failed\n");
		return -EIO;
	}
	/* We've written to the hardware, so update the cache */
	cache[reg] = value;

	return 0;
}

static int es8328_sync(struct snd_soc_codec *codec)
{
	u8 *cache = codec->reg_cache;
	int i, r = 0;

	for (i = 0; i < ES8328_CACHEREGNUM; i++)
		r |= snd_soc_write(codec, i, cache[i]);

	return r;
};

static int es8328_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct es8328_priv *es8328 = snd_soc_codec_get_drvdata(codec);

	es8328->sysclk = freq;
	return 0;
}

static int es8328_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 iface = 0;
	u8 adciface = 0;
	u8 daciface = 0;

	iface = snd_soc_read(codec, ES8328_IFACE);
	adciface = snd_soc_read(codec, ES8328_ADC_IFACE);
	daciface = snd_soc_read(codec, ES8328_DAC_IFACE);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:    // MASTER MODE
		iface |= 0x80;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:    // SLAVE MODE
		iface &= 0x7F;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		adciface &= 0xFC;
		daciface &= 0xF9;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iface &= 0xDF;
		adciface &= 0xDF;
		daciface &= 0xBF;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x20;
		adciface |= 0x20;
		daciface |= 0x40;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x20;
		adciface &= 0xDF;
		daciface &= 0xBF;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface &= 0xDF;
		adciface |= 0x20;
		daciface |= 0x40;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, ES8328_IFACE    , iface);
	snd_soc_write(codec, ES8328_ADC_IFACE, adciface);
	snd_soc_write(codec, ES8328_DAC_IFACE, daciface);

	return 0;
}

static int es8328_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	u16 iface;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		iface = snd_soc_read(codec, ES8328_DAC_IFACE) & 0xC7;
		/* bit size */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x0018;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0008;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x0020;
			break;
		}
		/* set iface & srate */
		snd_soc_write(codec, ES8328_DAC_IFACE, iface);
	} else {
		iface = snd_soc_read(codec, ES8328_ADC_IFACE) & 0xE3;
		/* bit size */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x000C;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0004;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x0010;
			break;
		}
		/* set iface */
		snd_soc_write(codec, ES8328_ADC_IFACE, iface);
	}

	return 0;
}

static int es8328_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned char val = 0;

	val = snd_soc_read(codec, ES8328_DAC_MUTE);
	if (mute){
		val |= 0x04;
	} else {
		val &= ~0x04;
	}

	snd_soc_write(codec, ES8328_DAC_MUTE, val);

	return 0;
}


static int es8328_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	switch(level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		if(codec->dapm.bias_level != SND_SOC_BIAS_ON) {
			/* updated by David-everest,5-25
			// Chip Power on
			snd_soc_write(codec, ES8328_CHIPPOWER, 0xF3);
			// VMID control
			snd_soc_write(codec, ES8328_CONTROL1 , 0x06);
			// ADC/DAC DLL power on
			snd_soc_write(codec, ES8328_CONTROL2 , 0xF3);
			*/
			snd_soc_write(codec, ES8328_ADCPOWER, 0x00);
			snd_soc_write(codec, ES8328_DACPOWER , 0x30);
			snd_soc_write(codec, ES8328_CHIPPOWER , 0x00);
		}
		break;

	case SND_SOC_BIAS_STANDBY:
		/*
		// ADC/DAC DLL power on
		snd_soc_write(codec, ES8328_CONTROL2 , 0xFF);
		// Chip Power off
		snd_soc_write(codec, ES8328_CHIPPOWER, 0xF3);
		*/
		snd_soc_write(codec, ES8328_ADCPOWER, 0x00);
		snd_soc_write(codec, ES8328_DACPOWER , 0x30);
		snd_soc_write(codec, ES8328_CHIPPOWER , 0x00);
		break;

	case SND_SOC_BIAS_OFF:
		/*
		// ADC/DAC DLL power off
		snd_soc_write(codec, ES8328_CONTROL2 , 0xFF);
		// Chip Control
		snd_soc_write(codec, ES8328_CONTROL1 , 0x00);
		// Chip Power off
		snd_soc_write(codec, ES8328_CHIPPOWER, 0xFF);
		*/
		snd_soc_write(codec, ES8328_ADCPOWER, 0xff);
		snd_soc_write(codec, ES8328_DACPOWER , 0xC0);
		snd_soc_write(codec, ES8328_CHIPPOWER , 0xC3);
		break;
	}

	codec->dapm.bias_level = level;

	return 0;
}


#define ES8328_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
                    SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | \
                    SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

#define ES8328_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
                    SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dai_ops es8328_dai_ops = {
	.hw_params    = es8328_pcm_hw_params,
	.set_fmt      = es8328_set_dai_fmt,
	.set_sysclk   = es8328_set_dai_sysclk,
	.digital_mute = es8328_mute,
};


struct snd_soc_dai_driver es8328_dai = {
	.name = "ES8328",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = ES8328_RATES,
		.formats = ES8328_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = ES8328_RATES,
		.formats = ES8328_FORMATS,},
	.ops = &es8328_dai_ops,
};

EXPORT_SYMBOL_GPL(es8328_dai);


static int es8328_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	es8328_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int es8328_resume(struct snd_soc_codec *codec)
{
	es8328_sync(codec);
	es8328_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static int es8328_remove(struct snd_soc_codec *codec)
{
	struct es8328_platform_data *es8328_pdata;

	es8328_set_bias_level(codec, SND_SOC_BIAS_OFF);

	es8328_pdata = codec->dev->platform_data;
	gpio_free(es8328_pdata->power_pin);

	return 0;
}

static int es8328_probe(struct snd_soc_codec *codec)
{
	struct es8328_priv *es8328 = snd_soc_codec_get_drvdata(codec);
	struct es8328_platform_data *es8328_pdata;
	int ret = 0;

	dev_info(codec->dev, "ES8328 Audio Codec %s", ES8328_VERSION);

	codec->control_data = es8328->control_data;

	es8328_pdata = codec->dev->platform_data;
	if (!es8328_pdata)
		return -EINVAL;

	/* Power on ES8328 codec */
	if (gpio_is_valid(es8328_pdata->power_pin)) {
		ret = gpio_request(es8328_pdata->power_pin, "es8328 power");
		if (ret < 0)
			return ret;
	} else {
		return -ENODEV;
	}
	gpio_direction_output(es8328_pdata->power_pin, GPIO_HIGH);
	msleep(es8328_pdata->power_delay);

	es8328_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	snd_soc_write(codec, ES8328_MASTERMODE  , 0x00);    // SLAVE MODE, MCLK not divide
	snd_soc_write(codec, ES8328_CHIPPOWER   , 0xf3);    // Power down: ADC DEM, DAC DSM/DEM, ADC/DAC state machine, ADC/DAC ananlog reference
	snd_soc_write(codec, ES8328_DACCONTROL21, 0x80);    // DACLRC and ADCLRC same, ADC/DAC DLL power up, Enable MCLK input from PAD.

	snd_soc_write(codec, ES8328_CONTROL1   , 0x05);     // VMIDSEL (500 kohme divider enabled)
	snd_soc_write(codec, ES8328_CONTROL2 , 0x72);   //

	snd_soc_write(codec, ES8328_DACPOWER   , 0x30);     // DAC R/L Power on, OUT1 enable, OUT2 disable
	snd_soc_write(codec, ES8328_ADCPOWER   , 0x00);     //
	snd_soc_write(codec, ES8328_ANAVOLMANAG, 0x7C);     //

	//-----------------------------------------------------------------------------------------------------------------
	snd_soc_write(codec, ES8328_ADCCONTROL1, 0x66);     // MIC PGA gain: +24dB
	snd_soc_write(codec, ES8328_ADCCONTROL2, 0xf0);     // LINSEL(L-R differential), RINGSEL(L-R differential)
	snd_soc_write(codec, ES8328_ADCCONTROL3, 0x82);     // Input Select: LIN2/RIN2
	snd_soc_write(codec, ES8328_ADCCONTROL4, 0x4C);     // Left data = left ADC, right data = right ADC, 24 bits I2S
	snd_soc_write(codec, ES8328_ADCCONTROL5, 0x02);     // 256fs
	//snd_soc_write(codec, ES8328_ADCCONTROL6, 0x00);     // Disable High pass filter

	snd_soc_write(codec, ES8328_LADC_VOL, 0x00);        // 0dB
	snd_soc_write(codec, ES8328_RADC_VOL, 0x00);        // 0dB

	//snd_soc_write(codec, ES8328_ADCCONTROL10, 0x3A);    // ALC stereo, Max gain(17.5dB), Min gain(0dB)
	snd_soc_write(codec, ES8328_ADCCONTROL10, 0xe2);    // ALC stereo, Max gain(17.5dB), Min gain(0dB),updated by david-everest,5-25
	snd_soc_write(codec, ES8328_ADCCONTROL11, 0xA0);    // ALCLVL(-1.5dB), ALCHLD(0ms)
	snd_soc_write(codec, ES8328_ADCCONTROL12, 0x05);    // ALCDCY(1.64ms/363us), ALCATK(1664us/363.2us)
	snd_soc_write(codec, ES8328_ADCCONTROL13, 0x06);    // ALCMODE(ALC mode), ALCZC(disable), TIME_OUT(disable), WIN_SIZE(96 samples)
	snd_soc_write(codec, ES8328_ADCCONTROL14, 0xd3);    // NGTH(XXX), NGG(mute ADC output), NGAT(enable)


	//----------------------------------------------------------------------------------------------------------------
	snd_soc_write(codec, ES8328_DACCONTROL1, 0x18);     // I2S 16bits
	snd_soc_write(codec, ES8328_DACCONTROL2, 0x02);     // 256fs

	snd_soc_write(codec, ES8328_LDAC_VOL, 0x00);    // left DAC volume
	snd_soc_write(codec, ES8328_RDAC_VOL, 0x00);    // right DAC volume

	snd_soc_write(codec, ES8328_DACCONTROL3, 0xE0);     // DAC unmute

	snd_soc_write(codec, ES8328_DACCONTROL17, 0xb8);    // left DAC to left mixer enable,
	snd_soc_write(codec, ES8328_DACCONTROL18, 0x38);    // ???
	snd_soc_write(codec, ES8328_DACCONTROL19, 0x38);    // ???
	snd_soc_write(codec, ES8328_DACCONTROL20, 0xb8);    // right DAC to right mixer enable,

	snd_soc_write(codec, ES8328_CHIPPOWER, 0x00);   // ALL Block POWER ON
	//snd_soc_write(codec, ES8328_CONTROL2 , 0x72);   // updated by david-everest,5-25
	//mdelay(100);

	snd_soc_write(codec, ES8328_LOUT1_VOL, 0x1D);   //
	snd_soc_write(codec, ES8328_ROUT1_VOL, 0x1D);   //
	snd_soc_write(codec, ES8328_LOUT2_VOL, 0x00);   // Disable LOUT2
	snd_soc_write(codec, ES8328_ROUT2_VOL, 0x00);   // Disable ROUT2

	snd_soc_add_codec_controls(codec, es8328_snd_controls,
		ARRAY_SIZE(es8328_snd_controls));
	snd_soc_dapm_new_controls(&codec->dapm, es8328_dapm_widgets,
		ARRAY_SIZE(es8328_dapm_widgets));
	snd_soc_dapm_add_routes(&codec->dapm, intercon, ARRAY_SIZE(intercon));

	return ret;
}


static struct snd_soc_codec_driver soc_codec_dev_es8328 = {
	.probe =	es8328_probe,
	.remove =	es8328_remove,
	.suspend =	es8328_suspend,
	.resume =	es8328_resume,
	.read =		es8328_read_reg_cache,
	.write =	es8328_write,
	.set_bias_level = es8328_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(es8328_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = es8328_reg,
	.reg_cache_step = 1,
};

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
static int es8328_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct es8328_priv *es8328;
	int ret;

	es8328 = kzalloc(sizeof(struct es8328_priv), GFP_KERNEL);
	if (es8328 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, es8328);
	es8328->control_data = i2c;

	ret =  snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_es8328, &es8328_dai, 1);
	if (ret < 0)
		kfree(es8328);

	return ret;
}

static int es8328_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	kfree(i2c_get_clientdata(i2c));

	return 0;
}

static const struct i2c_device_id es8328_i2c_id[] = {
	{ "es8328", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, es8328_i2c_id);

static struct i2c_driver es8328_i2c_driver = {
	.driver = {
		.name = "es8328-codec",
		.owner = THIS_MODULE,
	},
	.probe    = es8328_i2c_probe,
	.remove   = es8328_i2c_remove,
	.id_table = es8328_i2c_id,
};
#endif

static int __init es8328_modinit(void)
{
	int ret = 0;

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&es8328_i2c_driver);
	if (ret != 0) {
		pr_err("Failed to register ES8328 I2C driver: %d\n", ret);
	}
#endif
	return ret;
}
module_init(es8328_modinit);

static void __exit es8328_exit(void)
{
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&es8328_i2c_driver);
#endif
}
module_exit(es8328_exit);

MODULE_DESCRIPTION("ASoC ES8328 driver");
MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE("GPL");

