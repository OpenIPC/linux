/*
 * es8374.c  --  ES8374 ALSA SoC Audio Codec
 *
 * Copyright (C) 2016 Everest Semiconductor Co., Ltd
 *
 * Authors:  XianqingZheng(xqzheng@ambarella.com)
 *
 * Based on es8374.c by David Yang(yangxiaohua@everest-semi.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/stddef.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <linux/of_gpio.h>
#include "es8374.h"

//#define ES8374_SPI
/*
 * es8374 register cache
 */
static struct reg_default  es8374_reg_defaults[] = {
	{   0x00, 0x03 },
	{   0x01, 0x03 },
	{   0x02, 0x00 },
	{   0x03, 0x20 },
	{   0x04, 0x00 },
	{   0x05, 0x11 },
	{   0x06, 0x01 },
	{   0x07, 0x00 },
	{   0x08, 0x20 },
	{   0x09, 0x80 },
	{   0x0a, 0x4A },
	{   0x0b, 0x00 },
	{   0x0c, 0x00 },
	{   0x0d, 0x00 },
	{   0x0e, 0x00 },
	{   0x0f, 0x00 },

	{   0x10, 0x00 },
	{   0x11, 0x00 },
	{   0x12, 0x40 },
	{   0x13, 0x40 },
	{   0x14, 0x9C },
	{   0x15, 0xBE },
	{   0x16, 0x00 },
	{   0x17, 0xA0 },
	{   0x18, 0xFC },
	{   0x19, 0x00 },
	{   0x1a, 0x18 },
	{   0x1b, 0x00 },
	{   0x1c, 0x10 },
	{   0x1d, 0x10 },
	{   0x1e, 0x00 },
	{   0x1f, 0x08 },

	{   0x20, 0x08 },
	{   0x21, 0xD4 },
	{   0x22, 0x00 },
	{   0x23, 0x00 },
	{   0x24, 0x18 },
	{   0x25, 0xC0 },
	{   0x26, 0x1C },
	{   0x27, 0x00 },
	{   0x28, 0xB0 },
	{   0x29, 0x32 },
	{   0x2a, 0x03 },
	{   0x2b, 0x00 },
	{   0x2c, 0x0D },
	{   0x2d, 0x06 },
	{   0x2e, 0x1F },
	{   0x2f, 0xF7 },

	{   0x30, 0xFD },
	{   0x31, 0xFF },
	{   0x32, 0x1F },
	{   0x33, 0xF7 },
	{   0x34, 0xFD },
	{   0x35, 0xFF },
	{   0x36, 0x04 },
	{   0x37, 0x01 },
	{   0x38, 0xC0 },
	{   0x39, 0x00 },
	{   0x3a, 0x02 },
	{   0x3b, 0x17 },
	{   0x3c, 0xFD },
	{   0x3d, 0xFF },
	{   0x3e, 0x07 },
	{   0x3f, 0xFD },

	{   0x40, 0xFF },
	{   0x41, 0x00 },
	{   0x42, 0xFF },
	{   0x43, 0xBB },
	{   0x44, 0xFF },
	{   0x45, 0x00 },
	{   0x46, 0x00 },
	{   0x47, 0x00 },
	{   0x48, 0x00 },
	{   0x49, 0x00 },
	{   0x4a, 0x00 },
	{   0x4b, 0x00 },
	{   0x4c, 0x00 },
	{   0x4d, 0x00 },
	{   0x4e, 0x00 },
	{   0x4f, 0x00 },

	{   0x50, 0x00 },
	{   0x51, 0x00 },
	{   0x52, 0x00 },
	{   0x53, 0x00 },
	{   0x54, 0x00 },
	{   0x55, 0x00 },
	{   0x56, 0x00 },
	{   0x57, 0x00 },
	{   0x58, 0x00 },
	{   0x59, 0x00 },
	{   0x5a, 0x00 },
	{   0x5b, 0x00 },
	{   0x5c, 0x00 },
	{   0x5d, 0x00 },
	{   0x5e, 0x00 },
	{   0x5f, 0x00 },

	{   0x60, 0x00 },
	{   0x61, 0x00 },
	{   0x62, 0x00 },
	{   0x63, 0x00 },
	{   0x64, 0x00 },
	{   0x65, 0x00 },
	{   0x66, 0x00 },
	{   0x67, 0x00 },
	{   0x68, 0x00 },
	{   0x69, 0x00 },
	{   0x6a, 0x00 },
	{   0x6b, 0x00 },
	{   0x6c, 0x00 },
	{   0x6d, 0x00 },
	{   0x6e, 0x00 },
	{   0x6f, 0x00 },

};
#if 0
static u8 es8374_equalizer_src[] = {
	0x0A, 0x9B, 0x32, 0x03, 0x5C, 0x5D, 0x4B, 0x24, 0x0A, 0x9B,
	0x32, 0x03, 0x4C, 0x1F, 0x43, 0x05, 0x6D, 0x27, 0x54, 0x06,
	0x4D, 0xE1, 0x32, 0x02, 0x3E, 0x55, 0x2A, 0x20, 0x4D, 0xE1,
	0x32, 0x02, 0x2E, 0x17, 0x22, 0x01, 0x9F, 0xE7, 0x43, 0x25,
	0x4B, 0xD9, 0x21, 0x01, 0xF9, 0xD4, 0x11, 0x21, 0x4B, 0xD9,
	0x21, 0x01, 0xE9, 0x96, 0x19, 0x00, 0x4C, 0xE7, 0x22, 0x23,
};
#endif
struct sp_config {
	u8 spc, mmcc, spfs;
	u32 srate;
	u8 lrcdiv;
	u8 sclkdiv;
};

/* codec private data */

struct	es8374_private {
	enum snd_soc_control_type control_type;
	struct spi_device *spi;
	struct i2c_client *i2c;

	struct snd_soc_codec *codec;
	struct regmap *regmap;
	u32 clk_id;
	u32 mclk;

	/* platform dependant DVDD voltage configuration */
/*	u8	dvdd_pwr_vol;
	u8  pll_div;
*/
	int pwr_gpio;
	unsigned int pwr_active;
  	bool dmic_enable;
	u8 reg_cache[110];
};

struct es8374_private *es8374_data;

static bool es8374_volatile_register(struct device *dev,
			unsigned int reg)
{
	if ((reg  < 0x80)) {
		if((reg != 0x19) && (reg != 0x23)) {
			return true;
		} else {
			return false;
		}
	} else {
			return false;
	}
}

static bool es8374_readable_register(struct device *dev,
			unsigned int reg)
{
	if ((reg  < 0x80)) {
		if((reg != 0x19) && (reg != 0x23)) {
			return true;
		} else {
			return false;
		}
	} else {
			return false;
	}
}
static bool es8374_writable_register(struct device *dev,
			unsigned int reg)
{
	if ((reg  < 0x80)) {
		if((reg != 0x19) && (reg != 0x23)) {
			return true;
		} else {
			return false;
		}
	} else {
			return false;
	}
}


/*
*	Define ADC and DAC Volume
*/
static const DECLARE_TLV_DB_SCALE(vdac_tlv, 0, 50, 0);
static const DECLARE_TLV_DB_SCALE(vadc_tlv, 0, 50, 0);
/*
*	Define D2SE MIC BOOST GAIN
*/
static const DECLARE_TLV_DB_SCALE(mic_boost_tlv, 0, 1500, 0);
/*
*	Define LINE PGA GAIN
*/
static const DECLARE_TLV_DB_SCALE(linin_pga_tlv, 0, 300, 0);
/*
*	Define dmic boost gain
*/
static const DECLARE_TLV_DB_SCALE(dmic_6db_scaleup_tlv, 0, 600, 0);
/*
*	Definitiiion ALC noise gate type
*/

static const char * const ng_type_txt[] = {"Constant PGA Gain",
	"Mute ADC Output"};
static const struct soc_enum ng_type =
SOC_ENUM_SINGLE(ES8374_ALC_NGTH_REG2B, 6, 2, ng_type_txt);


static const char * const alc_mode_txt[] = {
	"ALC Mode",
	"Limiter Mode"
};

static const struct soc_enum alc_mode =
SOC_ENUM_SINGLE(ES8374_ALC_EN_MAX_GAIN_REG26, 5, 2, alc_mode_txt);

/*
*	Define MONO output gain
*/
static const DECLARE_TLV_DB_SCALE(mono_out_gain_tlv, 0, 150, 0);
/*
*	Definitiiion dac auto mute type
*/

static const char * const dac_auto_mute_type_txt[] = {
	"AUTO MUTE DISABLE",
	"MONO OUTPUT MUTE",
	"SPEAKER MUTE",
	"MONO OUT & SPEAKER MUTE"
	};
static const struct soc_enum dac_auto_mute_type =
SOC_ENUM_SINGLE(ES8374_DAC_CONTROL_REG37, 4, 4, dac_auto_mute_type_txt);
/*
*	Definitiiion dac dsm mute type
*/

static const char * const dac_dsm_mute_type_txt[] = {
	"DAC DSM UNMUTE",
	"DAC DSM MUTE",
	};
static const struct soc_enum dac_dsm_mute_type =
SOC_ENUM_SINGLE(ES8374_DAC_CONTROL_REG37, 0, 2, dac_dsm_mute_type_txt);

/*
 * es8374 Controls
 */
static const struct snd_kcontrol_new es8374_snd_controls[] = {
	/*
	* controls for capture path
	*/
	SOC_SINGLE_TLV("D2SE MIC BOOST GAIN",
					ES8374_AIN_PWR_SRC_REG21, 2, 1, 0, mic_boost_tlv),
	SOC_SINGLE_TLV("LIN PGA GAIN",
					ES8374_AIN_PGA_REG22, 0, 15, 0, linin_pga_tlv),
	SOC_SINGLE_TLV("DMIC 6DB SCALE UP GAIN",
					ES8374_ADC_CONTROL_REG24, 7, 1, 0, dmic_6db_scaleup_tlv),
	SOC_SINGLE("ADC Double FS Mode", ES8374_ADC_CONTROL_REG24, 6, 1, 0),
	SOC_SINGLE("ADC Soft Ramp", ES8374_ADC_CONTROL_REG24, 4, 1, 0),
	SOC_SINGLE("ADC MUTE", ES8374_ADC_CONTROL_REG24, 5, 1, 0),
	SOC_SINGLE("ADC INVERTED", ES8374_ADC_CONTROL_REG24, 2, 1, 0),
	SOC_SINGLE("ADC HPF COEFFICIENT", ES8374_ADC_HPF_REG2C, 0, 31, 0),
//	SOC_SINGLE_TLV("ADC Capture Volume",
//				ES8374_ADC_VOLUME_REG25, 0, 192, 1, adc_rec_tlv),
	SOC_SINGLE("ALC Capture Target Volume", ES8374_ALC_LVL_HLD_REG28, 4, 15, 0),
	SOC_SINGLE("ALC Capture Max PGA", ES8374_ALC_EN_MAX_GAIN_REG26, 0, 31, 0),
	SOC_SINGLE("ALC Capture Min PGA", ES8374_ALC_MIN_GAIN_REG27, 0, 31, 0),
//	SOC_ENUM("ALC Capture Function", alc_func),
	SOC_ENUM("ALC Mode", alc_mode),
	SOC_SINGLE("ALC Capture Hold Time", ES8374_ALC_LVL_HLD_REG28, 0, 15, 0),
	SOC_SINGLE("ALC Capture Decay Time", ES8374_ALC_DCY_ATK_REG29, 4, 15, 0),
	SOC_SINGLE("ALC Capture Attack Time", ES8374_ALC_DCY_ATK_REG29, 0, 15, 0),
	SOC_SINGLE("ALC Capture NG Threshold", ES8374_ALC_NGTH_REG2B, 0, 31, 0),
	SOC_ENUM("ALC Capture NG Type", ng_type),
	SOC_SINGLE("ALC Capture NG Switch", ES8374_ALC_NGTH_REG2B, 5, 1, 0),

	/*
	* controls for playback path
	*/
	SOC_SINGLE("DAC Double FS Mode", ES8374_DAC_CONTROL_REG37, 7, 1, 0),
	SOC_SINGLE("DAC Soft Ramp Rate", ES8374_DAC_CONTROL_REG36, 2, 7, 0),
	SOC_SINGLE("DAC MUTE", ES8374_DAC_CONTROL_REG36, 5, 1, 0),
	SOC_SINGLE("DAC OFFSET", ES8374_DAC_OFFSET_REG39, 0, 255, 0),
	SOC_ENUM("DAC AUTO MUTE TYPE", dac_auto_mute_type),
	SOC_ENUM("DAC DSM MUTE TYPE", dac_dsm_mute_type),

	SOC_SINGLE_TLV("ADC Capture Volume",
					ES8374_ADC_VOLUME_REG25, 0, 192, 1, vadc_tlv),
	SOC_SINGLE_TLV("DAC Playback Volume",
					ES8374_DAC_VOLUME_REG38, 0, 192, 1, vdac_tlv),
	SOC_SINGLE_TLV("MONO OUT GAIN",
					ES8374_MONO_GAIN_REG1B, 0, 15, 0, mono_out_gain_tlv),
	SOC_SINGLE_TLV("SPEAKER MIXER GAIN",
					ES8374_SPK_MIX_GAIN_REG1D, 0, 15, 0, mono_out_gain_tlv),
	SOC_SINGLE_TLV("SPEAKER OUTPUT Volume",
					ES8374_SPK_OUT_GAIN_REG1E, 0, 7, 0, mono_out_gain_tlv),
};

/*
 * DAPM Controls
 */
/*
* alc on/off
*/
static const char * const es8374_alc_enable_txt[] = {
	"ALC OFF",
	"ALC ON",
};

static const struct soc_enum es8374_alc_enable_enum =
SOC_ENUM_SINGLE(ES8374_ALC_EN_MAX_GAIN_REG26, 6,
	ARRAY_SIZE(es8374_alc_enable_txt), es8374_alc_enable_txt);


static const struct snd_kcontrol_new es8374_alc_enable_controls =
	SOC_DAPM_ENUM("Route", es8374_alc_enable_enum);
/*
* adc line in select
*/
static const char * const es8374_adc_input_src_txt[] = {
	"LIN1-RIN1",
	"LIN2-RIN2",
};
static const unsigned int es8374_adc_input_src_values[] = {
	0, 1};
static const struct soc_enum es8374_adc_input_src_enum =
SOC_VALUE_ENUM_SINGLE(ES8374_AIN_PWR_SRC_REG21, 4, 0x30,
			ARRAY_SIZE(es8374_adc_input_src_txt),
			es8374_adc_input_src_txt,
			es8374_adc_input_src_values);
static const struct snd_kcontrol_new es8374_adc_input_src_controls =
	SOC_DAPM_ENUM("Route", es8374_adc_input_src_enum);

/*
 * ANALOG IN MUX
 */
static const char * const es8374_analog_input_mux_txt[] = {
	"LIN1",
	"LIN2",
	"DIFF OUT1",
	"DIFF OUT2",
	"PGA OUT1",
	"PGA OUT2"
};
static const unsigned int es8374_analog_input_mux_values[] = {
	0, 1, 2, 3, 4, 5};
static const struct soc_enum es8374_analog_input_mux_enum =
SOC_VALUE_ENUM_SINGLE(ES8374_MONO_MIX_REG1A, 0, 0x7,
			ARRAY_SIZE(es8374_analog_input_mux_txt),
			es8374_analog_input_mux_txt,
			es8374_analog_input_mux_values);
static const struct snd_kcontrol_new es8374_analog_input_mux_controls =
SOC_DAPM_ENUM("Route", es8374_analog_input_mux_enum);
/*
 * MONO OUTPUT MIXER
 */
static const struct snd_kcontrol_new es8374_mono_out_mixer_controls[] = {
	SOC_DAPM_SINGLE("LIN TO MONO OUT Switch", ES8374_MONO_MIX_REG1A, 6, 1, 0),
	SOC_DAPM_SINGLE("DAC TO MONO OUT Switch", ES8374_MONO_MIX_REG1A, 7, 1, 0),
};
/*
 * SPEAKER OUTPUT MIXER
 */
static const struct snd_kcontrol_new es8374_speaker_mixer_controls[] = {
	SOC_DAPM_SINGLE("LIN TO SPEAKER OUT Switch", ES8374_SPK_MIX_REG1C, 6, 1, 0),
	SOC_DAPM_SINGLE("DAC TO SPEAKER OUT Switch", ES8374_SPK_MIX_REG1C, 7, 1, 0),
};
/*
 * digital microphone soure
 */
static const char * const es8374_dmic_mux_txt[] = {
		"DMIC DISABLE1",
		"DMIC DISABLE2",
		"DMIC AT HIGH LEVEL",
		"DMIC AT LOW LEVEL",
};
static const unsigned int es8374_dmic_mux_values[] = {
		0, 1, 2, 3};
static const struct soc_enum es8374_dmic_mux_enum =
	SOC_VALUE_ENUM_SINGLE(ES8374_ADC_CONTROL_REG24, 0, 0x3,
				ARRAY_SIZE(es8374_dmic_mux_txt),
				es8374_dmic_mux_txt,
				es8374_dmic_mux_values);
static const struct snd_kcontrol_new es8374_dmic_mux_controls =
	SOC_DAPM_ENUM("Route", es8374_dmic_mux_enum);
/*
 * ADC sdp  soure
 */
static const char * const es8374_adc_sdp_mux_txt[] = {
		"FROM ADC OUT",
		"FROM EQUALIZER",
};
static const unsigned int es8374_adc_sdp_mux_values[] = {
		0, 1};
static const struct soc_enum es8374_adc_sdp_mux_enum =
	SOC_VALUE_ENUM_SINGLE(ES8374_EQ_SRC_REG2D, 7, 0x80,
				ARRAY_SIZE(es8374_adc_sdp_mux_txt),
				es8374_adc_sdp_mux_txt,
				es8374_adc_sdp_mux_values);
static const struct snd_kcontrol_new es8374_adc_sdp_mux_controls =
	SOC_DAPM_ENUM("Route", es8374_adc_sdp_mux_enum);

/*
 * DAC dsm  soure
 */
static const char * const es8374_dac_dsm_mux_txt[] = {
		"FROM SDP IN",
		"FROM EQUALIZER",
};
static const unsigned int  es8374_dac_dsm_mux_values[] = {
		0, 1};
static const struct soc_enum  es8374_dac_dsm_mux_enum =
	SOC_VALUE_ENUM_SINGLE(ES8374_EQ_SRC_REG2D, 6, 0x40,
				ARRAY_SIZE(es8374_dac_dsm_mux_txt),
				es8374_dac_dsm_mux_txt,
				es8374_dac_dsm_mux_values);
static const struct snd_kcontrol_new  es8374_dac_dsm_mux_controls =
	SOC_DAPM_ENUM("Route", es8374_dac_dsm_mux_enum);
/*
 * equalizer data  soure
 */
static const char * const es8374_equalizer_src_mux_txt[] = {
		"FROM ADC OUT",
		"FROM SDP IN",
};
static const unsigned int  es8374_equalizer_src_mux_values[] = {
		0, 1};
static const struct soc_enum  es8374_equalizer_src_mux_enum =
	SOC_VALUE_ENUM_SINGLE(ES8374_EQ_SRC_REG2D, 5, 0x20,
				ARRAY_SIZE(es8374_equalizer_src_mux_txt),
				es8374_equalizer_src_mux_txt,
				es8374_equalizer_src_mux_values);
static const struct snd_kcontrol_new  es8374_equalizer_src_mux_controls =
	SOC_DAPM_ENUM("Route", es8374_equalizer_src_mux_enum);
/*
 * DAC data  soure
 */
static const char * const es8374_dac_data_mux_txt[] = {
		"SELECT SDP LEFT DATA",
		"SELECT SDP RIGHT DATA",
};
static const unsigned int  es8374_dac_data_mux_values[] = {
		0, 1};
static const struct soc_enum  es8374_dac_data_mux_enum =
	SOC_VALUE_ENUM_SINGLE(ES8374_DAC_CONTROL_REG36, 6, 0x40,
				ARRAY_SIZE(es8374_dac_data_mux_txt),
				es8374_dac_data_mux_txt,
				es8374_dac_data_mux_values);
static const struct snd_kcontrol_new  es8374_dac_data_mux_controls =
	SOC_DAPM_ENUM("Route", es8374_dac_data_mux_enum);

static const struct snd_soc_dapm_widget es8374_dapm_widgets[] = {
	/* Input Lines */
		SND_SOC_DAPM_INPUT("DMIC"),
		SND_SOC_DAPM_INPUT("MIC1"),
		SND_SOC_DAPM_INPUT("MIC2"),
		SND_SOC_DAPM_INPUT("LIN1"),
		SND_SOC_DAPM_INPUT("LIN2"),

	/*
	*  Capture path
	*/
		SND_SOC_DAPM_MICBIAS("micbias", ES8374_ANA_REF_REG14,
				4, 1),

		/* Input MUX */
		SND_SOC_DAPM_MUX("DIFFERENTIAL MUX", SND_SOC_NOPM, 0, 0,
				&es8374_adc_input_src_controls),

		SND_SOC_DAPM_PGA("DIFFERENTIAL PGA", SND_SOC_NOPM,
				0, 0, NULL, 0),

		SND_SOC_DAPM_PGA("LINE PGA", SND_SOC_NOPM,
				0, 0, NULL, 0),

		/* ADCs */
		SND_SOC_DAPM_ADC("MONO ADC", NULL, SND_SOC_NOPM, 0, 0),

		/* Dmic MUX */
		SND_SOC_DAPM_MUX("DMIC MUX", SND_SOC_NOPM, 0, 0,
				&es8374_dmic_mux_controls),

		/* Dmic MUX */
		SND_SOC_DAPM_MUX("ALC MUX", SND_SOC_NOPM, 0, 0,
			&es8374_alc_enable_controls),


		/* sdp MUX */
		SND_SOC_DAPM_MUX("SDP OUT MUX", SND_SOC_NOPM, 0, 0,
			&es8374_adc_sdp_mux_controls),

		/* Digital Interface */
		SND_SOC_DAPM_AIF_OUT("I2S OUT", "I2S1 Capture",  1,
				SND_SOC_NOPM, 0, 0),

		/*
		*	Render path
		*/
		SND_SOC_DAPM_AIF_IN("I2S IN", "I2S1 Playback", 0,
				SND_SOC_NOPM, 0, 0),

		/*	DACs SDP DATA SRC MUX */
		SND_SOC_DAPM_MUX("DAC SDP SRC MUX", SND_SOC_NOPM, 0, 0,
				&es8374_dac_data_mux_controls),

		/*	DACs  DATA SRC MUX */
		SND_SOC_DAPM_MUX("DAC SRC MUX", SND_SOC_NOPM, 0, 0,
			&es8374_dac_dsm_mux_controls),

		SND_SOC_DAPM_DAC("MONO DAC", NULL, SND_SOC_NOPM, 0, 0),


		/* hpmux for hp mixer */
		SND_SOC_DAPM_MUX("ANALOG INPUT MUX", SND_SOC_NOPM, 0, 0,
				&es8374_analog_input_mux_controls),

		/* Output mixer  */
		SND_SOC_DAPM_MIXER("MONO MIXER", SND_SOC_NOPM,
				0, 0, &es8374_mono_out_mixer_controls[0], ARRAY_SIZE(es8374_mono_out_mixer_controls)),
		SND_SOC_DAPM_MIXER("SPEAKER MIXER", SND_SOC_NOPM,
				0, 0, &es8374_speaker_mixer_controls[0], ARRAY_SIZE(es8374_speaker_mixer_controls)),

		/*
		*	Equalizer path
		*/
		SND_SOC_DAPM_MUX("EQUALIZER MUX", SND_SOC_NOPM, 0, 0,
				&es8374_equalizer_src_mux_controls),

		/* Output Lines */
		SND_SOC_DAPM_OUTPUT("MOUT"),
		SND_SOC_DAPM_OUTPUT("SPKOUT"),

};


static const struct snd_soc_dapm_route es8374_dapm_routes[] = {
	/*
	 * record route map
	 */
	{"MIC1", NULL, "micbias"},
	{"MIC2", NULL, "micbias"},
	{"DMIC", NULL, "micbias"},

	{"DIFFERENTIAL MUX", "LIN1-RIN1", "MIC1"},
	{"DIFFERENTIAL MUX", "LIN2-RIN2", "MIC2"},

	{"DIFFERENTIAL PGA", NULL, "DIFFERENTIAL MUX"},

	{"LINE PGA", NULL, "DIFFERENTIAL PGA"},

	{"MONO ADC", NULL, "LINE PGA"},

	{"DMIC MUX", "DMIC DISABLE1", "MONO ADC"},
	{"DMIC MUX", "DMIC DISABLE2", "MONO ADC"},
	{"DMIC MUX", "DMIC AT HIGH LEVEL", "DMIC"},
	{"DMIC MUX", "DMIC AT LOW LEVEL", "DMIC"},

	{"ALC MUX", "ALC OFF", "DMIC MUX"},
	{"ALC MUX", "ALC ON", "DMIC MUX"},

	/*
	* Equalizer path
	*/
	{"EQUALIZER MUX", "FROM ADC OUT", "ALC MUX"},
	{"EQUALIZER MUX", "FROM SDP IN", "I2S IN"},

	{"SDP OUT MUX", "FROM ADC OUT", "ALC MUX"},
	{"SDP OUT MUX", "FROM EQUALIZER", "EQUALIZER MUX"},

	{"I2S OUT", NULL, "SDP OUT MUX"},
	/*
	 * playback route map
	 */
	{"DAC SDP SRC MUX", "SELECT SDP LEFT DATA", "I2S IN"},
	{"DAC SDP SRC MUX", "SELECT SDP RIGHT DATA", "I2S IN"},



	{"DAC SRC MUX", "FROM SDP IN", "DAC SDP SRC MUX"},
	{"DAC SRC MUX", "FROM EQUALIZER", "EQUALIZER MUX"},

	{"MONO DAC", NULL, "DAC SRC MUX"},

	{"ANALOG INPUT MUX", "LIN1", "LIN1"},
	{"ANALOG INPUT MUX", "LIN2", "LIN2"},
	{"ANALOG INPUT MUX", "DIFF OUT1", "DIFFERENTIAL MUX"},
	{"ANALOG INPUT MUX", "DIFF OUT2", "DIFFERENTIAL PGA"},
	{"ANALOG INPUT MUX", "PGA OUT1", "LINE PGA"},
	{"ANALOG INPUT MUX", "PGA OUT2", "LINE PGA"},


	{"MONO MIXER", "LIN TO MONO OUT Switch", "ANALOG INPUT MUX"},
	{"MONO MIXER", "DAC TO MONO OUT Switch", "MONO DAC"},

	{"SPEAKER MIXER", "LIN TO SPEAKER OUT Switch", "ANALOG INPUT MUX"},
	{"SPEAKER MIXER", "DAC TO SPEAKER OUT Switch", "MONO DAC"},


	{"MOUT", NULL, "MONO MIXER"},
	{"SPKOUT", NULL, "SPEAKER MIXER"},

};

#if 0
static int es8374_set_pll(struct snd_soc_dai *dai, int pll_id,
			int source, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = dai->codec;
	struct es8374_private *priv = snd_soc_codec_get_drvdata(codec);
	u16 reg;
	u8 N, K1, K2, K3;
	float tmp;
	u32 Kcoefficient;
	switch (pll_id) {
	case ES8374_PLL:
		break;
	default:
		return -EINVAL;
		break;
	}
	/* Disable PLL  */
	// pll hold in reset state
	snd_soc_update_bits(codec, ES8374_PLL_CONTROL1_REG09,
							0x40, 0x00);

	if (!freq_in || !freq_out)
		return 0;

	switch (source) {

		case ES8374_PLL_SRC_FRM_MCLK:
			// Select PLL
			snd_soc_update_bits(codec, ES8374_CLK_MANAGEMENT_REG02,
									0x08, 0x08);
			break;
		default:
			//Disable PLL
			snd_soc_update_bits(codec, ES8374_CLK_MANAGEMENT_REG02,
									0x08, 0x00);
			return -EINVAL;
	}
	/*get N & K */
	tmp = 0;
	if (source == ES8374_PLL_SRC_FRM_MCLK) {
			if(freq_in >19200000)
			{
				freq_in /= 2;
				snd_soc_update_bits(codec, ES8374_CLK_MANAGEMENT_REG01,
								0x80, 0x80);	/* mclk div2	= 1 */
			}
			tmp = (float)freq_out * (float)priv->pll_div + 4000;
			tmp /= (float)freq_in;
			N =  (u8)tmp;
			tmp = tmp - (float)N;
			tmp = tmp * 0.6573598222296 * (1<<22);
			Kcoefficient = (u32)tmp;
			K1 = Kcoefficient / 65536;
			Kcoefficient = Kcoefficient - K1 * 65536;
			K2 = Kcoefficient /256;
			K3 = Kcoefficient - K2 * 256;
		}
		dev_dbg(codec->dev, "N=%x, K3=%x, K2=%x, K1=%x\n", N, K3, K2, K1);

		reg =  snd_soc_read(codec, ES8374_PLL_N_REG0B);
		reg &= 0xF0;
		reg |= (N & 0x0F);
		snd_soc_write(codec, ES8374_PLL_N_REG0B, reg);

		K1 &= 0x3F;
		snd_soc_write(codec, ES8374_PLL_K_REG0C, K1);
		snd_soc_write(codec, ES8374_PLL_K_REG0D, K2);
		snd_soc_write(codec, ES8374_PLL_K_REG0E, K3);


		/* pll div 8 */
		reg =  snd_soc_read(codec, ES8374_PLL_CONTROL1_REG09);
		reg &= 0xfc;
		reg |= 0x01;
		snd_soc_write(codec, ES8374_PLL_CONTROL1_REG09, reg);

		/* configure the pll power voltage  */
		switch (priv->dvdd_pwr_vol) {
			case 0x18:
				snd_soc_update_bits(codec, ES8374_PLL_CONTROL2_REG0A, 0x0c, 0x00); /* dvdd=1.8v */
				break;
			case 0x25:
				snd_soc_update_bits(codec, ES8374_PLL_CONTROL2_REG0A, 0x0c, 0x04); /* dvdd=2.5v */
				break;
			case 0x33:
				snd_soc_update_bits(codec, ES8374_PLL_CONTROL2_REG0A, 0x0c, 0x08); /* dvdd=3.3v */
				break;
			default:
				snd_soc_update_bits(codec, ES8374_PLL_CONTROL2_REG0A, 0x0c, 0x00); /* dvdd=1.8v */
				break;
		}
		/* enable PLL  */
		snd_soc_update_bits(codec, ES8374_PLL_CONTROL1_REG09,
								0x40, 0x40);
		priv->mclk = freq_out;
		return 0;
}
#endif

struct _coeff_div {
	u32 mclk;       /* mclk frequency */
	u32 rate;       /* sample rate */
	u8 div;         /* adcclk and dacclk divider */
	u8 fsmode;         /* adcclk and dacclk divider */
	u8 divdouble;         /* adcclk and dacclk divider */
	u8 lrck_h;      /* adclrck divider and daclrck divider */
	u8 lrck_l;
	u8 sr;          /* sclk divider */
	u8 osr;         /* adc osr */
};


/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {

//	MCLK	,	LRCK,DIV,FSMODE,divDOUBLE,LRCK-H,LRCK-L,BCLK,OSR
	/*
	//12M288
	{12288000, 96000 , 1 , 1 , 0 , 0x00 , 0x80 , 2 , 32},
	{12288000, 64000 , 3 , 1 , 1 , 0x00 , 0xC0 , 2 , 32},
	{12288000, 48000 , 1 , 0 , 0 , 0x01 , 0x00 , 2 , 32},
	{12288000, 32000 , 3 , 0 , 1 , 0x01 , 0x80 , 2 , 32},
	{12288000, 24000 , 2 , 0 , 0 , 0x02 , 0x00 , 2 , 32},
	{12288000, 16000 , 3 , 0 , 0 , 0x03 , 0x00 , 2 , 32},
	{12288000, 12000 , 4 , 0 , 0 , 0x04 , 0x00 , 2 , 32},
	{12288000, 8000  , 6 , 0 , 0 , 0x06 , 0x00 , 2 , 32},

	// 12M
	{12000000, 96000 , 1 , 1 , 0 , 0x00 , 0x7D , 2 , 31},
	{12000000, 88200 , 1 , 1 , 0 , 0x00 , 0x88 , 2 , 34},
	{12000000, 48000 , 5 , 1 , 1 , 0x00 , 0xFA , 2 , 25},
	{12000000, 44100 , 1 , 0 , 0 , 0x01 , 0x10 , 2 , 34},
	{12000000, 32000 , 3 , 1 , 0 , 0x01 , 0x77 , 2 , 31},
	{12000000, 24000 , 5 , 1 , 0 , 0x02 , 0x00 , 2 , 25},
	{12000000, 22050 , 2 , 0 , 0 , 0x02 , 0x20 , 2 , 34},
	{12000000, 16000 , 15, 1 , 1 , 0x02 , 0xEE , 2 , 25},
	{12000000, 12000 , 5 , 0 , 0 , 0x03 , 0xE8 , 2 , 25},
	{12000000, 11025 , 4 , 0 , 0 , 0x04 , 0x40 , 2 , 34},
	{12000000, 8000  , 15, 0 , 1 , 0x05 , 0xDC , 2 , 25},

	//11M2896
	{11289600, 88200 , 1 , 1 , 0 , 0x00 , 0x80 , 2 , 32},
	{11289600, 44100 , 1 , 0 , 0 , 0x01 , 0x00 , 2 , 32},
	{11289600, 22050 , 2 , 0 , 0 , 0x02 , 0x00 , 2 , 32},
	{11289600, 11025 , 4 , 0 , 0 , 0x04 , 0x00 , 2 , 32},

*/

	/* 8k */
	{12288000, 8000  , 6 , 0 , 0 , 0x06 , 0x00 , 2 , 32},
	{12000000, 8000  , 15, 0 , 1 , 0x05 , 0xDC , 2 , 25},
	{11289600, 8000 , 6 , 0 , 0 , 0x05, 0x83, 20, 29},
	{18432000, 8000 , 9 , 0 , 0 , 0x09, 0x00, 27, 32},
	{16934400, 8000 , 8 , 0 , 0 , 0x08, 0x44, 25, 33},
	{12000000, 8000 , 7 , 0 , 0 , 0x05, 0xdc, 21, 25},
	{19200000, 8000 , 12, 0 , 0 , 0x09, 0x60, 27, 25},

	/* 11.025k */
	{11289600, 11025 , 4 , 0 , 0 , 0x04 , 0x00 , 2 , 32},
	{12000000, 11025 , 4 , 0 , 0 , 0x04 , 0x40 , 2 , 34},
	{16934400, 11025, 6 ,  0 , 0 , 0x06, 0x00, 21, 32},


	/* 12k */
	{12000000, 12000 , 5 , 0 , 0 , 0x03 , 0xE8 , 2 , 25},
	{12288000, 12000 , 4 , 0 , 0 , 0x04 , 0x00 , 2 , 32},

	/* 16k */
	{12288000, 16000 , 3 , 0 , 0 , 0x03 , 0x00 , 2 , 32},
	{18432000, 16000, 5 ,  0 , 0 , 0x04, 0x80, 18, 25},
	{12000000, 16000 , 15, 1 , 1 , 0x02 , 0xEE , 2 , 25},
	{19200000, 16000, 6 ,  0 , 0 , 0x04, 0xb0, 18, 25},

	/* 22.05k */
	{11289600, 22050 , 2 , 0 , 0 , 0x02 , 0x00 , 2 , 32},
	{16934400, 22050, 3 ,  0 , 0 , 0x03, 0x00, 12, 32},
	{12000000, 22050 , 2 , 0 , 0 , 0x02 , 0x20 , 2 , 34},

	/* 24k */
	{12000000, 24000 , 5 , 1 , 0 , 0x02 , 0x00 , 2 , 25},
	{12288000, 24000 , 2 , 0 , 0 , 0x02 , 0x00 , 2 , 32},

	/* 32k */
	{12288000, 32000 , 3 , 0 , 1 , 0x01 , 0x80 , 2 , 32},
	{18432000, 32000, 2 ,  0 , 0 , 0x02, 0x40, 9 , 32},
	{12000000, 32000 , 3 , 1 , 0 , 0x01 , 0x77 , 2 , 31},
	{19200000, 32000, 3 ,  0 , 0 , 0x02, 0x58, 10, 25},

	/* 44.1k */
	{11289600, 44100 , 1 , 0 , 0 , 0x01 , 0x00 , 2 , 32},
	{16934400, 44100, 1 ,  0 , 0 , 0x01, 0x80, 6 , 32},
	{12000000, 44100 , 1 , 0 , 0 , 0x01 , 0x10 , 2 , 34},

	/* 48k */
	{12288000, 48000 , 1 , 0 , 0 , 0x01 , 0x00 , 2 , 32},
	{18432000, 48000, 1 ,  0 , 0 , 0x01, 0x80, 6 , 32},
	{12000000, 48000 , 5 , 1 , 1 , 0x00 , 0xFA , 2 , 25},
	{19200000, 48000, 2 ,  0 , 0 , 0x01, 0x90, 6, 25},

	/* 64k */
	{12288000, 64000 , 3 , 1 , 1 , 0x00 , 0xC0 , 2 , 32},

	/* 88.2k */
	{11289600, 88200 , 1 , 1 , 0 , 0x00 , 0x80 , 2 , 32},
	{16934400, 88200, 1 ,  0 , 0 , 0x00, 0xc0, 3 , 48},
	{12000000, 88200 , 1 , 1 , 0 , 0x00 , 0x88 , 2 , 34},

	/* 96k */
	{12288000, 96000 , 1 , 1 , 0 , 0x00 , 0x80 , 2 , 32},
	{18432000, 96000, 1 ,  0 , 0 , 0x00, 0xc0, 3 , 48},
	{12000000, 96000 , 1 , 1 , 0 , 0x00 , 0x7D , 2 , 31},
	{19200000, 96000, 1 ,  0 , 0 , 0x00, 0xc8, 3 , 25},
};
static inline int get_coeff(int mclk, int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
			return i;
	}

	return -EINVAL;
}

/* The set of rates we can generate from the above for each SYSCLK */
#if 0
static unsigned int rates_12288[] = {
	8000, 12000, 16000, 24000, 24000, 32000, 48000, 96000,
};

static struct snd_pcm_hw_constraint_list constraints_12288 = {
	.count	= ARRAY_SIZE(rates_12288),
	.list	= rates_12288,
};

static unsigned int rates_112896[] = {
	8000, 11025, 22050, 44100,
};

static struct snd_pcm_hw_constraint_list constraints_112896 = {
	.count	= ARRAY_SIZE(rates_112896),
	.list	= rates_112896,
};

static unsigned int rates_12[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
	48000, 88235, 96000,
};

static struct snd_pcm_hw_constraint_list constraints_12 = {
	.count	= ARRAY_SIZE(rates_12),
	.list	= rates_12,
};
#endif

/*
 * if PLL not be used, use internal clk1 for mclk,otherwise, use internal clk2 for PLL source.
 */
static int es8374_set_dai_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;

	if (clk_id == ES8374_CLKID_MCLK) {
		snd_soc_write(codec,0x09,0x80);	//pll set:reset on ,set start
		snd_soc_write(codec,0x0C,0x00);	//pll set:k
		snd_soc_write(codec,0x0D,0x00);	//pll set:k
		snd_soc_write(codec,0x0E,0x00);	//pll set:k

	}

	return 0;
}

static int es8374_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 iface = 0;
	u8 adciface = 0;
	u8 daciface = 0;

	iface    = snd_soc_read(codec, ES8374_MS_BCKDIV_REG0F);
	adciface = snd_soc_read(codec, ES8374_ADC_FMT_REG10);
	daciface = snd_soc_read(codec, ES8374_DAC_FMT_REG11);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:    /* MASTER MODE */
		iface |= 0x80;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:    /* SLAVE MODE */
		iface &= 0x7F;
		break;
	default:
		return -EINVAL;
	}


	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		adciface &= 0xFC;
		daciface &= 0xFC;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		return -EINVAL;
	case SND_SOC_DAIFMT_LEFT_J:
		adciface &= 0xFC;
		daciface &= 0xFC;
		adciface |= 0x01;
		daciface |= 0x01;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		adciface &= 0xDC;
		daciface &= 0xDC;
		adciface |= 0x03;
		daciface |= 0x03;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		adciface &= 0xDC;
		daciface &= 0xDC;
		adciface |= 0x23;
		daciface |= 0x23;
		break;
	default:
		return -EINVAL;
	}


	/* clock inversion */
	if(((fmt & SND_SOC_DAIFMT_FORMAT_MASK)==SND_SOC_DAIFMT_I2S) ||
		((fmt & SND_SOC_DAIFMT_FORMAT_MASK)==SND_SOC_DAIFMT_LEFT_J))
		{

		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:

			iface    &= 0xDF;
			adciface &= 0xDF;
			daciface &= 0xDF;
			break;
		case SND_SOC_DAIFMT_IB_IF:
			iface    |= 0x20;
			adciface |= 0x20;
			daciface |= 0x20;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			iface    |= 0x20;
			adciface &= 0xDF;
			daciface &= 0xDF;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			iface    &= 0xDF;
			adciface |= 0x20;
			daciface |= 0x20;
			break;
		default:
			return -EINVAL;
		}
	}
	snd_soc_write(codec, ES8374_MS_BCKDIV_REG0F, iface);
	snd_soc_write(codec, ES8374_ADC_FMT_REG10, adciface);
	snd_soc_write(codec, ES8374_DAC_FMT_REG11, daciface);
	return 0;
}
static int es8374_pcm_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	return 0;
}

static int es8374_pcm_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params,
			struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct es8374_private *es8374 = snd_soc_codec_get_drvdata(codec);
	u16 iface;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		iface = snd_soc_read(codec, ES8374_DAC_FMT_REG11) & 0xE3;
		/* bit size */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x0c;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x04;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x10;
			break;
		}
		/* set iface & srate */
		snd_soc_write(codec, ES8374_DAC_FMT_REG11, iface);
		/*set speak and mono for playback*/
		snd_soc_write(codec, ES8374_MONO_MIX_REG1A, 0xA0);
		snd_soc_write(codec, ES8374_SPK_MIX_REG1C, 0x90);

	} else {
		iface = snd_soc_read(codec, ES8374_ADC_FMT_REG10) & 0xE3;
		/* bit size */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x0c;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x04;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x10;
			break;
		}
		/* set iface */
		snd_soc_write(codec, ES8374_ADC_FMT_REG10, iface);
	}

	if(es8374->dmic_enable){
		snd_soc_write(codec, ES8374_ADC_CONTROL_REG24, 0x8A);  //6DB & DIMC=H
		snd_soc_write(codec,0x12,0x40);
		snd_soc_write(codec,0x13,0x40);
		snd_soc_write(codec,0x22,0x00);
		snd_soc_write(codec, 0x26, 0x50);
		snd_soc_write(codec, 0x28, 0x80);

	} else {
		snd_soc_write(codec,0x24,0x08);	//adc set
		snd_soc_write(codec, ES8374_GPIO_INSERT_REG6D, 0x1F);  //set gpio1 to GM SHORT
	}

/*
*	please add the hardware clock divider setting to get the correct LRCK, SCLK frequency
*/
	return 0;
}

static int es8374_set_bias_level(struct snd_soc_codec *codec,
			enum snd_soc_bias_level level)
{

	switch (level) {
	case SND_SOC_BIAS_ON:
		snd_soc_write(codec, ES8374_CLK_MANAGEMENT_REG01, 0x7f);
		snd_soc_write(codec, ES8374_ANA_REF_REG14, 0x8a);
		snd_soc_write(codec, ES8374_ANA_PWR_CTL_REG15, 0x40);
		snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0x00);
		snd_soc_write(codec, ES8374_MONO_MIX_REG1A, 0xa0);
		snd_soc_write(codec, ES8374_SPK_MIX_REG1C, 0x90);
		snd_soc_write(codec, ES8374_SPK_MIX_GAIN_REG1D, 0x02);
		snd_soc_write(codec, ES8374_SPK_OUT_GAIN_REG1E, 0xa0);
		snd_soc_write(codec, ES8374_DAC_CONTROL_REG36, 0x00);
		snd_soc_write(codec, ES8374_DAC_CONTROL_REG37, 0x00);
		snd_soc_write(codec, ES8374_DAC_VOLUME_REG38, 0x00);
		snd_soc_write(codec, ES8374_ADC_VOLUME_REG25, 0x00);
		snd_soc_write(codec, ES8374_AIN_PGA_REG22, 0x06);

		break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0xc0);
		break;

	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0xc0);
		snd_soc_write(codec, ES8374_ALC_LVL_HLD_REG28, 0x1c);
		snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0x00);
		snd_soc_write(codec, ES8374_ANA_PWR_CTL_REG15, 0xbf);
		snd_soc_write(codec, ES8374_ANA_REF_REG14, 0x14);
		snd_soc_write(codec, ES8374_CLK_MANAGEMENT_REG01, 0x03);
		snd_soc_write(codec, ES8374_AIN_PGA_REG22, 0x00);
		break;
	}

	codec->dapm.bias_level = level;

	return 0;
}

/*
static int es8374_set_tristate(struct snd_soc_dai *dai, int tristate)
{
	struct snd_soc_codec *codec = dai->codec;
	dev_dbg(codec->dev, "es8374_set_tristate........\n");
	if(tristate) {
		snd_soc_update_bits(codec, ES8374_MS_BCKDIV_REG0F,
					0x40, 0x40);
	} else {
		snd_soc_update_bits(codec, ES8374_MS_BCKDIV_REG0F,
					0x40, 0x00);
	}

	return 0;
}
*/

static int es8374_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev, "%s %d\n", __func__, mute);
	if (mute) {
		snd_soc_update_bits(codec, ES8374_DAC_CONTROL_REG36, 0x20, 0x20);
	} else {
		if (dai->playback_active)
			snd_soc_update_bits(codec, ES8374_DAC_CONTROL_REG36, 0x20, 0x00);
	}
	return 0;
}


#define es8374_RATES SNDRV_PCM_RATE_8000_96000

#define es8374_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
		SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops es8374_ops = {
	.startup = es8374_pcm_startup,
	.hw_params = es8374_pcm_hw_params,
	.set_fmt = es8374_set_dai_fmt,
	.set_sysclk = es8374_set_dai_sysclk,
	.digital_mute = es8374_mute,
};

static struct snd_soc_dai_driver es8374_dai[] = {
	{	.name = "ES8374 HiFi",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = es8374_RATES,
			.formats = es8374_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = es8374_RATES,
			.formats = es8374_FORMATS,
		},
		.ops = &es8374_ops,
		.symmetric_rates = 1,
	},
};

static int es8374_suspend(struct snd_soc_codec *codec)
{
	struct es8374_private *es8374 = snd_soc_codec_get_drvdata(codec);
	int i;

	for(i = 0; i <= 110; i++) {
		es8374->reg_cache[i] = snd_soc_read(codec, i);
	}

	snd_soc_write(codec, ES8374_DAC_VOLUME_REG38, 0xc0);
	snd_soc_write(codec, ES8374_ADC_VOLUME_REG25, 0xc0);
	snd_soc_write(codec, ES8374_DAC_CONTROL_REG36, 0x20);
	snd_soc_write(codec, ES8374_DAC_CONTROL_REG37, 0x21);
	snd_soc_write(codec, ES8374_MONO_MIX_REG1A, 0x08);
	snd_soc_write(codec, ES8374_SPK_MIX_REG1C, 0x10);
	snd_soc_write(codec, ES8374_SPK_MIX_GAIN_REG1D, 0x10);
	snd_soc_write(codec, ES8374_SPK_OUT_GAIN_REG1E, 0x40);
	snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0x00);
	snd_soc_write(codec, ES8374_ANA_PWR_CTL_REG15, 0xbf);
	snd_soc_write(codec, ES8374_ANA_REF_REG14, 0x14);
	snd_soc_write(codec, ES8374_CLK_MANAGEMENT_REG01, 0x03);

	return 0;
}

static int es8374_resume(struct snd_soc_codec *codec)
{
	struct es8374_private *es8374 = snd_soc_codec_get_drvdata(codec);
	int i;
#if 0
	snd_soc_write(codec, ES8374_CLK_MANAGEMENT_REG01, 0x7f);
	snd_soc_write(codec, ES8374_ANA_REF_REG14, 0x8a);
	snd_soc_write(codec, ES8374_ANA_PWR_CTL_REG15, 0x40);
	snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0x50);
	snd_soc_write(codec, ES8374_MONO_MIX_REG1A, 0xa0);
	snd_soc_write(codec, ES8374_SPK_MIX_REG1C, 0x90);
	snd_soc_write(codec, ES8374_SPK_MIX_GAIN_REG1D, 0x02);
	snd_soc_write(codec, ES8374_SPK_OUT_GAIN_REG1E, 0xa0);
	snd_soc_write(codec, ES8374_DAC_CONTROL_REG36, 0x00);
	snd_soc_write(codec, ES8374_DAC_CONTROL_REG37, 0x00);
	snd_soc_write(codec, ES8374_DAC_VOLUME_REG38, 0x00);
	snd_soc_write(codec, ES8374_ADC_VOLUME_REG25, 0x00);
#endif

	for(i = 0; i <= 110; i++) {
		snd_soc_write(codec, i, es8374->reg_cache[i]);
	}

	return 0;
}

static int es8374_parse_dts(struct es8374_private *es8374)
{
#ifdef CONFIG_OF
	struct device *dev;
	struct device_node *np;
	enum of_gpio_flags flags;
	int ret, dmic;

	if (es8374->control_type == SND_SOC_I2C) {
		dev = &(es8374->i2c->dev);
	} else {
		dev = &(es8374->spi->dev);
	}

	np = dev->of_node;

	if (!np)
		return -1;

	ret = of_property_read_u32(np, "es8374,dmic", &dmic);
	if(ret == 0 && dmic == 1) {
		es8374->dmic_enable = 1;
	} else {
		es8374->dmic_enable = 0;
	}

	es8374->pwr_gpio = of_get_named_gpio_flags(np, "es8374,pwr-gpio", 0, &flags);
	if (es8374->pwr_gpio < 0 || !gpio_is_valid(es8374->pwr_gpio)) {
		es8374->pwr_gpio = -1;
	} else {
		es8374->pwr_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	}

#endif

	return 0;
}

static int es8374_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct es8374_private *es8374 = es8374_data;

	codec->control_data = es8374->regmap;
	ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_REGMAP);
	if (ret < 0)
		return ret;

	es8374->codec = codec;
	snd_soc_codec_set_drvdata(codec, es8374);

	es8374_parse_dts(es8374);
	if (gpio_is_valid(es8374->pwr_gpio)) {
		ret = devm_gpio_request(codec->dev, es8374->pwr_gpio, "es8374 power pin");
		if(ret == 0)
			gpio_direction_output(es8374->pwr_gpio, es8374->pwr_active);
	}

	snd_soc_write(codec,0x00,0x3F);	//IC Rst start
	msleep(1);  					//DELAY_MS
	snd_soc_write(codec,0x00,0x03);	//IC Rst stop
	snd_soc_write(codec,0x01,0x7F);	//IC clk on
	snd_soc_write(codec,0x05,0x11);	//clk div set
	snd_soc_write(codec,0x36,0x00);	//dac set
	snd_soc_write(codec,0x12,0x30);	//timming set
	snd_soc_write(codec,0x13,0x20);	//timming set
	snd_soc_write(codec,0x21,0x50);	//adc set:	SEL LIN1 CH+PGAGAIN=0DB
	snd_soc_write(codec,0x22,0xFF);	//adc set:	PGA GAIN=0DB
	snd_soc_write(codec,0x21,0x10);	//adc set:	SEL LIN1 CH+PGAGAIN=0DB
	snd_soc_write(codec,0x00,0x80);	// IC START
	msleep(50);  					//DELAY_MS
	snd_soc_write(codec,0x14,0x8A);	// IC START
	snd_soc_write(codec,0x15,0x40);	// IC START
	snd_soc_write(codec,0x1A,0xA0);	// monoout set
	snd_soc_write(codec,0x1B,0x19);	// monoout set
	snd_soc_write(codec,0x1C,0x90);	// spk set
	snd_soc_write(codec,0x1D,0x02);	// spk set
	snd_soc_write(codec,0x1F,0x00);	// spk set
	snd_soc_write(codec,0x1E,0xA0);	// spk on
	snd_soc_write(codec,0x28,0x00);	// alc set
	snd_soc_write(codec,0x25,0x00);	// ADCVOLUME  on
	snd_soc_write(codec,0x38,0x00);	// DACVOLUMEL on
	snd_soc_write(codec,0x37,0x00);	// dac set
	snd_soc_write(codec,0x6D,0x40);	//SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT

	es8374_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return ret;
}

static int es8374_remove(struct snd_soc_codec *codec)
{
	es8374_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_es8374 = {
	.probe =	es8374_probe,
	.remove =	es8374_remove,
	.suspend =	es8374_suspend,
	.resume =	es8374_resume,
	.set_bias_level = es8374_set_bias_level,

	.controls = es8374_snd_controls,
	.num_controls = ARRAY_SIZE(es8374_snd_controls),
	.dapm_widgets = es8374_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(es8374_dapm_widgets),
	.dapm_routes = es8374_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(es8374_dapm_routes),
};

static struct regmap_config es8374_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = ES8374_MAX_REGISTER,
	.reg_defaults = es8374_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(es8374_reg_defaults),
	.volatile_reg = es8374_volatile_register,
	.writeable_reg = es8374_writable_register,
	.readable_reg  = es8374_readable_register,
	.cache_type = REGCACHE_RBTREE,
};

#ifdef CONFIG_OF
static struct of_device_id es8374_if_dt_ids[] = {
	{ .compatible = "ambarella,es8374", },
	{ }
};
#endif

#if defined(ES8374_SPI)
static int es8374_spi_probe(struct spi_device *spi)
{
	struct es8374_private *es8374;
	int ret;

	es8374 = kzalloc(sizeof(struct es8374_private), GFP_KERNEL);
	if (es8374 == NULL)
		return -ENOMEM;

	es8374->control_type = SND_SOC_SPI;
	es8374->spi = spi;

	spi_set_drvdata(spi, es8374);

	es8374->regmap = devm_regmap_init_spi(spi, &es8374_regmap);
	if (IS_ERR(es8374->regmap)) {
		ret = PTR_ERR(es8374->regmap);
		dev_err(&spi->dev, "regmap_init() failed: %d\n", ret);
		return ret;
	}

	es8374_data = es8374;

	ret = snd_soc_register_codec(&spi->dev,
			&soc_codec_dev_es8374, &es8374_dai, 1);
	if (ret < 0)
		kfree(es8374);
	return ret;
}

static int es8374_spi_remove(struct spi_device *spi)
{
	snd_soc_unregister_codec(&spi->dev);
	kfree(spi_get_drvdata(spi));
	return 0;
}

static struct spi_driver es8374_spi_driver = {
	.driver = {
		.name	= "es8374",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(es8374_if_dt_ids),
#endif
	},
	.probe	= es8374_spi_probe,
	.remove	= es8374_spi_remove,
};
#endif /* CONFIG_SPI_MASTER */

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
static void es8374_i2c_shutdown(struct i2c_client *i2c)
{
	struct snd_soc_codec *codec;
	struct es8374_private *es8374;

	es8374 = i2c_get_clientdata(i2c);
	codec = es8374->codec;

	snd_soc_write(codec, ES8374_DAC_VOLUME_REG38, 0xc0);
	snd_soc_write(codec, ES8374_ADC_VOLUME_REG25, 0xc0);
	snd_soc_write(codec, ES8374_DAC_CONTROL_REG36, 0x20);
	snd_soc_write(codec, ES8374_DAC_CONTROL_REG37, 0x21);
	snd_soc_write(codec, ES8374_MONO_MIX_REG1A, 0x08);
	snd_soc_write(codec, ES8374_SPK_MIX_REG1C, 0x10);
	snd_soc_write(codec, ES8374_SPK_MIX_GAIN_REG1D, 0x10);
	snd_soc_write(codec, ES8374_SPK_OUT_GAIN_REG1E, 0x40);
	snd_soc_update_bits(codec, ES8374_AIN_PWR_SRC_REG21, 0xc0, 0xc0);
	snd_soc_write(codec, ES8374_ANA_PWR_CTL_REG15, 0xbf);
	snd_soc_write(codec, ES8374_ANA_REF_REG14, 0x14);
	snd_soc_write(codec, ES8374_CLK_MANAGEMENT_REG01, 0x03);

	return;
}

static int es8374_i2c_probe(struct i2c_client *i2c_client,
		const struct i2c_device_id *id)
{
	struct es8374_private *es8374;
	int ret = -1;

	es8374 = kzalloc(sizeof(*es8374), GFP_KERNEL);
	if (es8374 == NULL)
		return -ENOMEM;

	es8374->control_type = SND_SOC_I2C;
	es8374->i2c = i2c_client;

	i2c_set_clientdata(i2c_client, es8374);
	es8374->regmap = devm_regmap_init_i2c(i2c_client, &es8374_regmap);
	if (IS_ERR(es8374->regmap)) {
		ret = PTR_ERR(es8374->regmap);
		dev_err(&i2c_client->dev, "regmap_init() failed: %d\n", ret);
		return ret;
	}

	es8374_data = es8374;

	ret =  snd_soc_register_codec(&i2c_client->dev, &soc_codec_dev_es8374,
			&es8374_dai[0], ARRAY_SIZE(es8374_dai));
	if (ret < 0) {
		kfree(es8374);
		return ret;
	}

	return ret;
}

static  int es8374_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id es8374_i2c_id[] = {
	{"es8374", 0 },
	{"10ES8374:00", 0},
	{"10ES8374", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, es8374_i2c_id);

static struct i2c_driver es8374_i2c_driver = {
	.driver = {
		.name	= "es8374",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(es8374_if_dt_ids),
#endif
	},
	.shutdown	= es8374_i2c_shutdown,
	.probe	= es8374_i2c_probe,
	.remove	= es8374_i2c_remove,
	.id_table	= es8374_i2c_id,
};
#endif

static int __init es8374_init(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	return i2c_add_driver(&es8374_i2c_driver);
#endif
#ifdef ES8374_SPI
	return spi_register_driver(&es8374_spi_driver);
#endif
}

static void __exit es8374_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	return i2c_del_driver(&es8374_i2c_driver);
#endif
#ifdef ES8374_SPI
	return spi_unregister_driver(&es8374_spi_driver);
#endif
}

module_init(es8374_init);
module_exit(es8374_exit);

MODULE_DESCRIPTION("ASoC es8374 driver");
MODULE_AUTHOR("XianqingZheng <xqzheng@ambarella.com>");
MODULE_LICENSE("GPL");

