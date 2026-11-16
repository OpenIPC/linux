// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 SoC Internal Audio Codec Driver
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#define T23_CODEC_REG_00	0x00
#define T23_CODEC_REG_08	0x08
#define T23_CODEC_REG_0C	0x0c
#define T23_CODEC_REG_10	0x10
#define T23_CODEC_REG_14	0x14
#define T23_CODEC_REG_20	0x20
#define T23_CODEC_REG_24	0x24
#define T23_CODEC_REG_80	0x80
#define T23_CODEC_REG_84	0x84
#define T23_CODEC_REG_88	0x88
#define T23_CODEC_REG_8C	0x8c
#define T23_CODEC_REG_90	0x90
#define T23_CODEC_REG_94	0x94
#define T23_CODEC_REG_98	0x98
#define T23_CODEC_REG_9C	0x9c
#define T23_CODEC_REG_A0	0xa0
#define T23_CODEC_REG_110	0x110
#define T23_CODEC_REG_124	0x124

struct t23_codec {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock;
	unsigned int playback_volume;
	unsigned int capture_volume;
	bool playback_switch;
	bool capture_switch;
	bool hpf_switch;
	bool muted;
	bool initialized;
};

#define T23_CODEC_RATES (SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_96000 | \
			 SNDRV_PCM_RATE_KNOT)

static const unsigned int t23_sample_rates[8] = {
	8000, 12000, 16000, 24000, 32000, 44100, 48000, 96000
};

static const unsigned int t23_sample_rate_codes[8] = {
	7, 6, 5, 4, 3, 2, 1, 0
};

static const struct snd_pcm_hw_constraint_list t23_rate_constraints = {
	.count = ARRAY_SIZE(t23_sample_rates),
	.list = t23_sample_rates,
};

static int t23_codec_hw_init(struct t23_codec *codec)
{
	int i, ret;
	unsigned int capture_volume = 16;
	unsigned int shift_val = 0;
	int cur_bit = 0;

	if (codec->initialized) {
		ret = regmap_read(codec->regmap, T23_CODEC_REG_94,
				  &capture_volume);
		if (ret)
			return ret;
		capture_volume &= 0x1f;
	}

	/* Reset sequence: 0 -> 10ms -> 3 -> 10ms -> 2 -> 10ms -> 3 */
	regmap_update_bits(codec->regmap, T23_CODEC_REG_00, 0x3, 0);
	msleep(10);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_00, 0x3, 3);
	msleep(10);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_00, 0x3, 2);
	msleep(10);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_00, 0x3, 3);
	msleep(10);

	regmap_write(codec->regmap, T23_CODEC_REG_0C, 0x3e);
	regmap_write(codec->regmap, T23_CODEC_REG_10, 0x10);
	regmap_write(codec->regmap, T23_CODEC_REG_14, 0x3e);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_98, 0x3, 1);
	msleep(10);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_84, 0x7f, 1);
	msleep(10);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_88, BIT(5), BIT(5));
	msleep(10);

	/* Anti-pop 7-step DAC ramp */
	for (i = 7; i > 0; i--) {
		shift_val = ((cur_bit | 1) & 0xff) | shift_val;
		regmap_update_bits(codec->regmap, T23_CODEC_REG_84, 0x7f, shift_val);
		msleep(30);
		cur_bit = shift_val << 1;
	}

	msleep(20);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_00, BIT(1), 0);
	regmap_update_bits(codec->regmap, T23_CODEC_REG_00, BIT(1), BIT(1));

	/* Clear bit 6 of 0x124 before updating 0x94. */
	regmap_update_bits(codec->regmap, T23_CODEC_REG_124, BIT(6), 0);
	msleep(10);
	ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_94, 0x1f,
				 capture_volume & 0x1f);
	msleep(10);
	if (!ret)
		codec->initialized = true;
	return ret;
}

static int t23_codec_set_sample_rate(struct t23_codec *codec, unsigned int rate)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (rate == t23_sample_rates[i])
			return regmap_update_bits(codec->regmap,
					 T23_CODEC_REG_110, 0x7,
					 t23_sample_rate_codes[i]);
	}

	return -EINVAL;
}

static int t23_codec_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int rate = params_rate(params);
	int format_code, ret;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		format_code = 0;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		format_code = 1;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		format_code = 2;
		break;
	default:
		return -EINVAL;
	}

	ret = t23_codec_set_sample_rate(codec, rate);
	if (ret)
		return ret;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		return regmap_update_bits(codec->regmap, T23_CODEC_REG_10,
				   0x3 << 5, format_code << 5);
	} else {
		unsigned int mono = params_channels(params) == 1 ? BIT(0) : 0;

		/*
		 * T23_CODEC_REG_08:
		 * bit 0: 1 for right mono record
		 * bits [4:3]: 2 for I2S format
		 * bits [6:5]: format code (0: 16-bit, 1: 20-bit, 2: 24-bit)
		 * bit 7: ADC interface enable
		 */
		return regmap_update_bits(codec->regmap, T23_CODEC_REG_08,
					  (0x3 << 5) | (0x3 << 3) | BIT(0) | BIT(7),
					  (format_code << 5) | (0x2 << 3) | mono |
					  (codec->capture_switch ? BIT(7) : 0));
	}
}

static int t23_codec_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_component *component = dai->component;
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	int ret = 0;

	if (stream != SNDRV_PCM_STREAM_PLAYBACK)
		return 0;

	mutex_lock(&codec->lock);
	if (codec->playback_switch) {
		ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_A0, 0x1f,
					 mute ? 0 : codec->playback_volume);
	}
	if (!ret)
		codec->muted = !!mute;
	mutex_unlock(&codec->lock);
	return ret;
}

static int t23_codec_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	if ((fmt & SND_SOC_DAIFMT_FORMAT_MASK) != SND_SOC_DAIFMT_I2S ||
	    (fmt & SND_SOC_DAIFMT_INV_MASK) != SND_SOC_DAIFMT_NB_NF)
		return -EINVAL;

	switch (fmt & SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK) {
	case SND_SOC_DAIFMT_CBP_CFP:
		return 0;
	default:
		return -EINVAL;
	}
}

static int t23_codec_dai_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	return snd_pcm_hw_constraint_list(substream->runtime, 0,
					  SNDRV_PCM_HW_PARAM_RATE,
					  &t23_rate_constraints);
}

static const struct snd_soc_dai_ops t23_codec_dai_ops = {
	.startup = t23_codec_dai_startup,
	.hw_params = t23_codec_hw_params,
	.mute_stream = t23_codec_mute_stream,
	.set_fmt = t23_codec_set_fmt,
	.no_capture_mute = 1,
};

static struct snd_soc_dai_driver t23_codec_dai[] = {
	{
		.name = "t23-hifi",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = T23_CODEC_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = T23_CODEC_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &t23_codec_dai_ops,
		.symmetric_rate = 1,
	},
	/* Terminates the AIC playback reference link; no codec path. */
	{
		.name = "t23-loop",
		.capture = {
			.stream_name = "Loop Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = T23_CODEC_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	},
};

static int t23_codec_dac_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		regmap_update_bits(codec->regmap, T23_CODEC_REG_80, 0x7, 0x7);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, 0xf, 0xf);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_98, BIT(3) | BIT(2) | 0x3,
				   BIT(3) | BIT(2) | 0x2);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_A0, BIT(5) | BIT(6),
				   BIT(5) | BIT(6));
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_9C, 0xff, 0xf8);
		msleep(10);
		mutex_lock(&codec->lock);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_A0, BIT(7) | 0x1f,
			(codec->playback_switch ? BIT(7) : 0) |
			((codec->playback_switch && !codec->muted) ? codec->playback_volume : 0));
		mutex_unlock(&codec->lock);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_84, 0x7f, 1);
		msleep(10);
	} else if (SND_SOC_DAPM_EVENT_OFF(event)) {
		regmap_update_bits(codec->regmap, T23_CODEC_REG_A0,
				   0x1f | BIT(7) | BIT(6) | BIT(5), 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_9C, 0xf0, 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_98, BIT(3), 0);
		msleep(10);
	}

	return 0;
}

static int t23_codec_adc_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		regmap_update_bits(codec->regmap, T23_CODEC_REG_98, 0x3, 0x2);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_24, 0x1f, 0x8);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_08, 0x18 | BIT(7),
				   0x10 | (codec->capture_switch ? BIT(7) : 0));
		msleep(10);
		regmap_write(codec->regmap, T23_CODEC_REG_0C, 0x3e);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(7), BIT(7));
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_88, 0x1f, 0x1f);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(5), BIT(5));
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_90, BIT(4) | BIT(5),
				   BIT(4) | BIT(5));
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_98, 0xf0, 0xf0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(6), BIT(6));
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_90, 0xc0, 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_84, 0x7f, 1);
		msleep(11);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(4), BIT(4));
		msleep(10);
		regmap_write(codec->regmap, T23_CODEC_REG_20,
			     0x92 | (codec->hpf_switch ? BIT(6) : 0));
		msleep(10);
	} else if (SND_SOC_DAPM_EVENT_OFF(event)) {
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(4), 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_98, 0x70, 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_90, BIT(4) | BIT(5), 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(5), 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_88, BIT(4), 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_98, 0x90, 0);
		msleep(10);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_8C, BIT(6), 0);
		msleep(10);
	}

	return 0;
}

static const DECLARE_TLV_DB_SCALE(dac_tlv, -4650, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_tlv, 0, 150, 0);

static int t23_playback_volume_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	mutex_lock(&codec->lock);
	ucontrol->value.integer.value[0] = codec->playback_volume;
	mutex_unlock(&codec->lock);
	return 0;
}

static int t23_playback_volume_put(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	long volume = ucontrol->value.integer.value[0];
	int ret = 0;

	if (volume < 0 || volume > 31)
		return -EINVAL;

	mutex_lock(&codec->lock);
	if (codec->playback_volume != volume) {
		if (codec->playback_switch && !codec->muted)
			ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_A0,
					   0x1f, volume);
		if (!ret) {
			codec->playback_volume = volume;
			ret = 1;
		}
	}
	mutex_unlock(&codec->lock);
	return ret;
}

static int t23_playback_switch_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	mutex_lock(&codec->lock);
	ucontrol->value.integer.value[0] = codec->playback_switch;
	mutex_unlock(&codec->lock);
	return 0;
}

static int t23_playback_switch_put(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int val = !!ucontrol->value.integer.value[0];
	int ret = 0;

	mutex_lock(&codec->lock);
	if (codec->playback_switch != val) {
		if (val) {
			ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_A0,
					   BIT(7) | 0x1f,
					   BIT(7) | (codec->muted ? 0 : codec->playback_volume));
		} else {
			ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_A0,
					   BIT(7) | 0x1f, 0);
		}
		if (!ret) {
			codec->playback_switch = val;
			ret = 1;
		}
	}
	mutex_unlock(&codec->lock);
	return ret;
}

static int t23_capture_volume_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	mutex_lock(&codec->lock);
	ucontrol->value.integer.value[0] = codec->capture_volume;
	mutex_unlock(&codec->lock);
	return 0;
}

static int t23_capture_volume_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	long volume = ucontrol->value.integer.value[0];
	int ret = 0;

	if (volume < 0 || volume > 31)
		return -EINVAL;

	mutex_lock(&codec->lock);
	if (codec->capture_volume != volume) {
		ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_124, BIT(6), 0);
		if (ret)
			goto unlock;
		msleep(10);
		ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_94, 0x1f, volume);
		if (ret)
			goto unlock;
		msleep(10);
		codec->capture_volume = volume;
		ret = 1;
	}
unlock:
	mutex_unlock(&codec->lock);
	return ret;
}

static int t23_capture_switch_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	mutex_lock(&codec->lock);
	ucontrol->value.integer.value[0] = codec->capture_switch;
	mutex_unlock(&codec->lock);
	return 0;
}

static int t23_capture_switch_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int val = !!ucontrol->value.integer.value[0];
	int ret = 0;

	mutex_lock(&codec->lock);
	if (codec->capture_switch != val) {
		ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_08,
					 BIT(7), val ? BIT(7) : 0);
		if (!ret) {
			codec->capture_switch = val;
			ret = 1;
		}
	}
	mutex_unlock(&codec->lock);
	return ret;
}

static int t23_hpf_switch_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	mutex_lock(&codec->lock);
	ucontrol->value.integer.value[0] = codec->hpf_switch;
	mutex_unlock(&codec->lock);
	return 0;
}

static int t23_hpf_switch_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int val = !!ucontrol->value.integer.value[0];
	int ret = 0;

	mutex_lock(&codec->lock);
	if (codec->hpf_switch != val) {
		ret = regmap_update_bits(codec->regmap, T23_CODEC_REG_20,
					 BIT(6), val ? BIT(6) : 0);
		if (!ret) {
			codec->hpf_switch = val;
			ret = 1;
		}
	}
	mutex_unlock(&codec->lock);
	return ret;
}

static const struct snd_kcontrol_new t23_codec_controls[] = {
	SOC_SINGLE_EXT_TLV("Master Playback Volume", T23_CODEC_REG_A0, 0, 31, 0,
			   t23_playback_volume_get, t23_playback_volume_put,
			   dac_tlv),
	SOC_SINGLE_BOOL_EXT("Master Playback Switch", 0,
			    t23_playback_switch_get, t23_playback_switch_put),
	SOC_SINGLE_EXT_TLV("Mic Capture Volume", T23_CODEC_REG_94, 0, 31, 0,
			   t23_capture_volume_get, t23_capture_volume_put,
			   adc_tlv),
	SOC_SINGLE_BOOL_EXT("Mic Capture Switch", 0,
			    t23_capture_switch_get, t23_capture_switch_put),
	SOC_SINGLE_BOOL_EXT("High-Pass Filter Capture Switch", 0,
			    t23_hpf_switch_get, t23_hpf_switch_put),
};

static const struct snd_soc_dapm_widget t23_codec_dapm_widgets[] = {
	SND_SOC_DAPM_DAC_E("DAC", "Playback", SND_SOC_NOPM, 0, 0,
			   t23_codec_dac_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_ADC_E("ADC", "Capture", SND_SOC_NOPM, 0, 0,
			   t23_codec_adc_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),
	SND_SOC_DAPM_INPUT("MICIN"),
};

static const struct snd_soc_dapm_route t23_codec_dapm_routes[] = {
	{ "LOUT", NULL, "DAC" },
	{ "ROUT", NULL, "DAC" },
	{ "ADC", NULL, "MICIN" },
};

static int t23_codec_set_bias_level(struct snd_soc_component *component,
				    enum snd_soc_bias_level level)
{
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		if (snd_soc_component_get_bias_level(component) == SND_SOC_BIAS_OFF)
			return t23_codec_hw_init(codec);
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}

	return 0;
}

static void t23_codec_component_remove(struct snd_soc_component *component)
{
	struct t23_codec *codec = snd_soc_component_get_drvdata(component);

	if (codec && codec->regmap) {
		regmap_write(codec->regmap, T23_CODEC_REG_A0, 0);
		regmap_update_bits(codec->regmap, T23_CODEC_REG_08, BIT(7), 0);
		regmap_write(codec->regmap, T23_CODEC_REG_00, 0);
	}
}

static const struct snd_soc_component_driver t23_codec_component = {
	.remove			= t23_codec_component_remove,
	.set_bias_level		= t23_codec_set_bias_level,
	.controls		= t23_codec_controls,
	.num_controls		= ARRAY_SIZE(t23_codec_controls),
	.dapm_widgets		= t23_codec_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(t23_codec_dapm_widgets),
	.dapm_routes		= t23_codec_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(t23_codec_dapm_routes),
	.suspend_bias_off	= 1,
	.idle_bias_on		= 1,
	/*
	 * No pmdown_time: an idle DAC emits a tone at fs/6 that the speaker
	 * amplifier would play until the delayed power-down.
	 */
	.endianness		= 1,
};

static const struct regmap_config t23_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x134,
	.cache_type = REGCACHE_NONE,
};

static const struct of_device_id t23_codec_of_matches[] = {
	{ .compatible = "ingenic,t23-codec" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, t23_codec_of_matches);

static int t23_codec_probe(struct platform_device *pdev)
{
	struct t23_codec *codec;
	void __iomem *base;
	int ret;

	codec = devm_kzalloc(&pdev->dev, sizeof(*codec), GFP_KERNEL);
	if (!codec)
		return -ENOMEM;

	codec->dev = &pdev->dev;
	mutex_init(&codec->lock);
	codec->playback_volume = 24; /* 0x18 */
	codec->capture_volume = 16;  /* 0x10 */
	codec->playback_switch = true;
	codec->capture_switch = true;
	codec->hpf_switch = true;
	codec->muted = true;

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	codec->regmap = devm_regmap_init_mmio(&pdev->dev, base,
					     &t23_codec_regmap_config);
	if (IS_ERR(codec->regmap))
		return PTR_ERR(codec->regmap);
	platform_set_drvdata(pdev, codec);

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &t23_codec_component,
					      t23_codec_dai,
					      ARRAY_SIZE(t23_codec_dai));
	if (ret) {
		dev_err(&pdev->dev, "failed to register codec component: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct platform_driver t23_codec_driver = {
	.probe = t23_codec_probe,
	.driver = {
		.name = "ingenic-t23-codec",
		.of_match_table = t23_codec_of_matches,
	},
};

module_platform_driver(t23_codec_driver);

MODULE_DESCRIPTION("Ingenic T23 SoC internal audio codec driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
