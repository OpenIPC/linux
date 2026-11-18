/*
 * ALSA SoC ES7210 adc driver
 *
 * Author:      David Yang, <yangxiaohua@everest-semi.com>
 * Copyright:   (C) 2018 Everest Semiconductor Co Ltd.,
 *
 * Based on sound/soc/codecs/es7243.c by David Yang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Notes:
 *  ES7210 is a 4-ch ADC of Everest
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <linux/regmap.h>
#include "es7210.h"
#include "es7210_config.h"
#include "es7210_config.c"
#include "es7210_snd_controls.c"
/* codec private data */
struct es7210_priv {
	struct regmap *regmap;
	struct i2c_client *i2c;	//i2c client for es7210
	/*
	 * enable or disable pdm dmic interface,
	 * must be initialized in i2c_probe()
	 * pdm_dmic_enanle=1, pdm dmic interface enabled
	 * pdm_dmic_enanle=0, pdm dmic interface disabled
	 */
#if ES7210_CHANNELS_MAX > 0
	unsigned int pdm_dmic_1_2_enable;
	unsigned int pdm_dmic_3_4_enable;
#endif
#if ES7210_CHANNELS_MAX > 4
	unsigned int pdm_dmic_5_6_enable;
	unsigned int pdm_dmic_7_8_enable;
#endif
#if ES7210_CHANNELS_MAX > 8
	unsigned int pdm_dmic_9_10_enable;
	unsigned int pdm_dmic_11_12_enable;
#endif
#if ES7210_CHANNELS_MAX > 12
	unsigned int pdm_dmic_13_14_enable;
	unsigned int pdm_dmic_15_16_enable;
#endif

	unsigned int sysclk;
	struct snd_pcm_hw_constraint_list *sysclk_constraints;
	unsigned int tdm_mode;
	struct delayed_work pcm_pop_work;
	bool sclkinv;
	unsigned int mclk_lrck_ratio;
};
static const struct regmap_config es7210_regmap_config = {
	.reg_bits = 8,		//Number of bits in a register address
	.val_bits = 8,		//Number of bits in a register value
};

/*
* ES7210 register cache
*/
static const u8 es7210_reg[] = {
	0x32, 0x40, 0x02, 0x04, 0x01, 0x00, 0x00, 0x20,	/* 0 - 7 */
	0x10, 0x40, 0x40, 0x00, 0x00, 0x09, 0x00, 0x00,	/* 8 - F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 10 - 17 */
	0xf7, 0xf7, 0x00, 0xbf, 0xbf, 0xbf, 0xbf, 0x00,	/* 18 - 1f */
	0x26, 0x26, 0x06, 0x26, 0x00, 0x00, 0x00, 0x00,	/* 20 - 27 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 28 - 2f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 30 - 37 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0x10, 0x00,	/* 38 - 3f */
	0x80, 0x71, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 40 - 47 */
	0x00, 0x00, 0x00, 0xff, 0xff,	/* 48 - 4c */
};

static const struct reg_default es7210_reg_defaults[] = {
	{0x00, 0x32},		//0
	{0x01, 0x40},
	{0x02, 0x02},
	{0x03, 0x04},
	{0x04, 0x01},
	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x20},
	{0x08, 0x10},
	{0x09, 0x40},
	{0x0a, 0x40},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x09},
	{0x0e, 0x00},
	{0x0f, 0x00},
	{0x10, 0x00},
	{0x11, 0x00},
	{0x12, 0x00},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x00},
	{0x16, 0x00},
	{0x17, 0x00},
	{0x18, 0xf7},
	{0x19, 0xf7},
	{0x1a, 0x00},
	{0x1b, 0xbf},
	{0x1c, 0xbf},
	{0x1d, 0xbf},
	{0x1e, 0xbf},
	{0x1f, 0x00},
	{0x20, 0x26},
	{0x21, 0x26},
	{0x22, 0x06},
	{0x23, 0x26},
	{0x3d, 0x72},
	{0x3e, 0x10},
	{0x3f, 0x00},
	{0x40, 0x80},
	{0x41, 0x71},
	{0x42, 0x71},
	{0x43, 0x00},
	{0x44, 0x00},
	{0x45, 0x00},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0x00},
	{0x4a, 0x00},
	{0x4b, 0xff},
	{0x4c, 0xff},
};

/*
* Note that this should be called from init rather than from hw_params.
*/
static int es7210_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				 int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int es7210_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	return 0;
}

static void es7210_unmute(void)
{
	printk("enter into %s\n", __func__);
	es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x00);
	es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x00);
}

static void pcm_pop_work_events(struct work_struct *work)
{
	printk("enter into %s\n", __func__);
	es7210_unmute();
	es7210_init_reg = 1;
}

static int es7210_mute(struct snd_soc_dai *dai, int mute)
{
	printk("enter into %s, mute = %d\n", __func__, mute);
	if (mute) {
		es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03,
					       0x03);
		es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03,
					       0x03);
	} else {
		es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03,
					       0x00);
		es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03,
					       0x00);
	}
	return 0;
}

static int es7210_pcm_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_component *codec = dai->component;
	struct es7210_priv *es7210 = snd_soc_component_get_drvdata(codec);

	if (es7210_init_reg == 0) {
		schedule_delayed_work(&es7210->pcm_pop_work,
				      msecs_to_jiffies(100));
	}
	return 0;
}

static int es7210_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	int i;
	printk("Enter into %s()\n", __func__);
	for (i = 0; i < ADC_DEV_MAXNUM; i++) {
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			es7210_update_bits(0x11, 0xe0, 0x60, i2c_clt1[i]);
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			es7210_update_bits(0x11, 0xe0, 0x20, i2c_clt1[i]);
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			es7210_update_bits(0x11, 0xe0, 0x00, i2c_clt1[i]);
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			es7210_update_bits(0x11, 0xe0, 0x80, i2c_clt1[i]);
			break;
		default:
			es7210_update_bits(0x11, 0xe0, 0x60, i2c_clt1[i]);
			break;
		}
	}
	return 0;
}

#define es7210_RATES SNDRV_PCM_RATE_8000_96000

#define es7210_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops es7210_ops = {
	.startup = es7210_pcm_startup,
	.hw_params = es7210_pcm_hw_params,
	.set_fmt = es7210_set_dai_fmt,
	.set_sysclk = es7210_set_dai_sysclk,
	.digital_mute = es7210_mute,
};

#if ES7210_CHANNELS_MAX > 0
static struct snd_soc_dai_driver es7210_dai0 = {
	.name = "ES7210 4CH ADC 0",
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 4 * ADC_DEV_MAXNUM,
		    .rates = es7210_RATES,
		    .formats = es7210_FORMATS,
		    },
	.ops = &es7210_ops,
	.symmetric_rates = 1,
};
#endif
#if ES7210_CHANNELS_MAX > 4
static struct snd_soc_dai_driver es7210_dai1 = {
	.name = "ES7210 4CH ADC 1",
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 4 * ADC_DEV_MAXNUM,
		    .rates = es7210_RATES,
		    .formats = es7210_FORMATS,
		    },
	.ops = &es7210_ops,
	.symmetric_rates = 1,
};
#endif
#if ES7210_CHANNELS_MAX > 8
static struct snd_soc_dai_driver es7210_dai2 = {
	.name = "ES7210 4CH ADC 2",
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 4 * ADC_DEV_MAXNUM,
		    .rates = es7210_RATES,
		    .formats = es7210_FORMATS,
		    },
	.ops = &es7210_ops,
	.symmetric_rates = 1,
};
#endif
#if ES7210_CHANNELS_MAX > 12
static struct snd_soc_dai_driver es7210_dai3 = {
	.name = "ES7210 4CH ADC 3",
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 4 * ADC_DEV_MAXNUM,
		    .rates = es7210_RATES,
		    .formats = es7210_FORMATS,
		    },
	.ops = &es7210_ops,
	.symmetric_rates = 1,
};
#endif
static struct snd_soc_dai_driver *es7210_dai[] = {
#if ES7210_CHANNELS_MAX > 0
	&es7210_dai0,
#endif
#if ES7210_CHANNELS_MAX > 4
	&es7210_dai1,
#endif
#if ES7210_CHANNELS_MAX > 8
	&es7210_dai2,
#endif
#if ES7210_CHANNELS_MAX > 12
	&es7210_dai3,
#endif
};

static int es7210_suspend(struct snd_soc_component *codec)
{
	es7210_multi_chips_write(0x4b, 0xff);
	es7210_multi_chips_update_bits(0x01, 0x2a, 0x2a);
	es7210_multi_chips_write(0x4c, 0xff);
	es7210_multi_chips_update_bits(0x01, 0x34, 0x34);
	return 0;
}

static int es7210_resume(struct snd_soc_component *codec)
{

	es7210_multi_chips_update_bits(0x01, 0x0a, 0x00);
	es7210_multi_chips_write(0x4b, 0x00);
	es7210_multi_chips_update_bits(0x01, 0x14, 0x00);
	es7210_multi_chips_write(0x4c, 0x00);
	// Comment out due to this will cause crash
	// in low power consumption case
	// TODO: implement regmap machanism
	//snd_soc_component_cache_sync(codec);
	return 0;
}

static int es7210_probe(struct snd_soc_component *codec)
{
	struct es7210_priv *es7210 = snd_soc_component_get_drvdata(codec);
	int ret = 0;
	int index;
#if !ES7210_CODEC_RW_TEST_EN
	//ret = snd_soc_component_set_cache_io(codec, 8, 8, SND_SOC_I2C);//8,8
#else
	codec->control_data = devm_regmap_init_i2c(es7210->i2c,
						   &es7210_regmap_config);
	ret = PTR_RET(codec->control_data);
#endif

	if (ret < 0) {
		printk("Failed to set cache I/O: %d\n", ret);
		return ret;
	}
	printk("begin->>>>>>>>>>%s!\n", __func__);

	tron_codec1[es7210_codec_num++] = codec;
	INIT_DELAYED_WORK(&es7210->pcm_pop_work, pcm_pop_work_events);

	/*
	 * initialize es7210
	 */
	es7210_tdm_init_codec(es7210->tdm_mode);
	/*
	 * set clock ratio according to es7210_config.h
	 */
	switch (ES7210_WORK_MODE) {
	case ES7210_TDM_NLRCK_I2S:
	case ES7210_TDM_NLRCK_LJ:
	case ES7210_TDM_NLRCK_DSPA:
	case ES7210_TDM_NLRCK_DSPB:
		for (index = 0; index < sizeof(es7210_nfs_ratio_cfg) /
		     sizeof(es7210_nfs_ratio_cfg[0]); index++) {
			if ((es7210->mclk_lrck_ratio ==
			     es7210_nfs_ratio_cfg[index].ratio)
			    && (es7210_nfs_ratio_cfg[index].channels ==
				ES7210_CHANNELS_MAX))
				break;
		}
		es7210_multi_chips_write(0x02,
					 es7210_nfs_ratio_cfg[index].reg02_v);
		es7210_multi_chips_write(0x06,
					 es7210_nfs_ratio_cfg[index].reg06_v);
		break;
	default:
		for (index = 0; index < sizeof(es7210_1fs_ratio_cfg) /
		     sizeof(es7210_1fs_ratio_cfg[0]); index++) {
			if (es7210->mclk_lrck_ratio ==
			    es7210_1fs_ratio_cfg[index].ratio)
				break;
		}
		es7210_multi_chips_write(0x02,
					 es7210_1fs_ratio_cfg[index].reg02_v);
		es7210_multi_chips_write(0x06,
					 es7210_1fs_ratio_cfg[index].reg06_v);
		break;

	}

	/*
	 * Disable or Enable PDM DMIC interface
	 */

#if ES7210_CHANNELS_MAX > 0
	if (es7210->pdm_dmic_1_2_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x40, i2c_clt1[0]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x00, i2c_clt1[0]);

	if (es7210->pdm_dmic_3_4_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x80, i2c_clt1[0]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x00, i2c_clt1[0]);
#endif
#if ES7210_CHANNELS_MAX > 4
	if (es7210->pdm_dmic_5_6_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x40, i2c_clt1[1]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x00, i2c_clt1[1]);

	if (es7210->pdm_dmic_7_8_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x80, i2c_clt1[1]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x00, i2c_clt1[1]);
#endif
#if ES7210_CHANNELS_MAX > 8
	if (es7210->pdm_dmic_9_10_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x40, i2c_clt1[2]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x00, i2c_clt1[2]);

	if (es7210->pdm_dmic_11_12_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x80, i2c_clt1[2]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x00, i2c_clt1[2]);
#endif
#if ES7210_CHANNELS_MAX > 12
	if (es7210->pdm_dmic_13_14_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x40, i2c_clt1[3]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x40, 0x00, i2c_clt1[3]);

	if (es7210->pdm_dmic_15_16_enable == ENABLE)
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x80, i2c_clt1[3]);
	else
		es7210_update_bits(ES7210_DMIC_CTL_REG10,
				   0x80, 0x00, i2c_clt1[3]);
#endif

	if (es7210->sclkinv == true) {
		es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08,
					       0x08, 0x08);
	}
	/*
	 * Schedule a delay work-quenue to avoid pop noise
	 */
	INIT_DELAYED_WORK(&es7210->pcm_pop_work, pcm_pop_work_events);
	return 0;
}

static void es7210_remove(struct snd_soc_component *codec)
{

}

static struct snd_soc_component_driver soc_codec_dev_es7210 = {
	.probe = es7210_probe,
	.remove = es7210_remove,
	.suspend = es7210_suspend,
	.resume = es7210_resume,
	//.reg_word_size = sizeof(u8),
	//.reg_cache_default = es7210_reg_defaults,
	//.reg_cache_size = ARRAY_SIZE(es7210_reg_defaults),
	.controls = es7210_snd_controls,
	.num_controls = ARRAY_SIZE(es7210_snd_controls),
};

static ssize_t es7210_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	int val = 0, flag = 0;
	u8 i = 0, reg, num, value_w, value_r;

	struct es7210_priv *es7210 = dev_get_drvdata(dev);
	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 16) & 0xFF;

	if (flag) {
		reg = (val >> 8) & 0xFF;
		value_w = val & 0xFF;
		printk("\nWrite start REG:0x%02x,val:0x%02x, count:0x%02x\n",
		       reg, value_w, flag);
		while (flag--) {
			es7210_write(reg, value_w, es7210->i2c);
			printk("Write 0x%02x to REG:0x%02x\n", value_w, reg);
			reg++;
		}
	} else {
		reg = (val >> 8) & 0xFF;
		num = val & 0xff;
		printk("\nRead: start REG:0x%02x,count:0x%02x\n", reg, num);
		do {
			value_r = 0;
			es7210_read(reg, &value_r, es7210->i2c);
			printk("REG[0x%02x]: 0x%02x;  ", reg, value_r);
			reg++;
			i++;
			if ((i == num) || (i % 4 == 0))
				printk("\n");
		} while (i < num);
	}

	return count;
}

static ssize_t es7210_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	printk("echo flag|reg|val > es7210\n");
	printk("write star addres=0x90,value=0x3c");
	printk("count=4:echo 4903c >es7210\n");
	return 0;
}

static DEVICE_ATTR(es7210, 0644, es7210_show, es7210_store);

static struct attribute *es7210_debug_attrs[] = {
	&dev_attr_es7210.attr,
	NULL,
};

static struct attribute_group es7210_debug_attr_group = {
	.name = "es7210_debug",
	.attrs = es7210_debug_attrs,
};

/*
 * If the i2c layer weren't so broken, we could pass this kind of data
 * around
 */
static int es7210_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *i2c_id)
{
	struct es7210_priv *es7210;
	int ret;

	printk("begin->>>>>>>>>>%s!\n", __func__);

	es7210 = devm_kzalloc(&i2c->dev,
			      sizeof(struct es7210_priv), GFP_KERNEL);
	if (es7210 == NULL)
		return -ENOMEM;
	es7210->i2c = i2c;
	es7210->sclkinv = false;	//sclk not inverted
	es7210->tdm_mode = ES7210_WORK_MODE;	//to set tdm mode or normal mode
	es7210->mclk_lrck_ratio = RATIO_MCLK_LRCK;
#if ES7210_CHANNELS_MAX > 0
	es7210->pdm_dmic_1_2_enable = DISABLE;	// disable pdm dmic interface
	es7210->pdm_dmic_3_4_enable = DISABLE;	// disable pdm dmic interface
#endif
#if ES7210_CHANNELS_MAX > 4
	es7210->pdm_dmic_5_6_enable = DISABLE;	// disable pdm dmic interface
	es7210->pdm_dmic_7_8_enable = DISABLE;	// disable pdm dmic interface
#endif
#if ES7210_CHANNELS_MAX > 8
	es7210->pdm_dmic_9_10_enable = DISABLE;	// disable pdm dmic interface
	es7210->pdm_dmic_11_12_enable = DISABLE;	// disable pdm dmic interface
#endif
#if ES7210_CHANNELS_MAX > 12
	es7210->pdm_dmic_13_14_enable = DISABLE;	// disable pdm dmic interface
	es7210->pdm_dmic_15_16_enable = DISABLE;	// disable pdm dmic interface
#endif
	dev_set_drvdata(&i2c->dev, es7210);
	if (i2c_id->driver_data < ADC_DEV_MAXNUM) {
		i2c_clt1[i2c_id->driver_data] = i2c;
		ret = snd_soc_register_component(&i2c->dev, &soc_codec_dev_es7210,
					     es7210_dai[i2c_id->driver_data],
					     1);
		if (ret < 0) {
			kfree(es7210);
			return ret;
		}
	}
	ret = sysfs_create_group(&i2c->dev.kobj, &es7210_debug_attr_group);
	if (ret) {
		printk("failed to create attr group\n");
	}
	return ret;
}

static int __exit es7210_i2c_remove(struct i2c_client *i2c)
{
	//snd_soc_unregister_codec(&i2c->dev);
	kfree(i2c_get_clientdata(i2c));
	return 0;
}

#if !ES7210_MATCH_DTS_EN
static int es7210_i2c_detect(struct i2c_client *client,
			     struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (adapter->nr == ES7210_I2C_BUS_NUM) {
		if (client->addr == 0x40) {
			strlcpy(info->type, "MicArray_0", I2C_NAME_SIZE);
			return 0;
		} else if (client->addr == 0x43) {
			strlcpy(info->type, "MicArray_1", I2C_NAME_SIZE);
			return 0;
		} else if (client->addr == 0x42) {
			strlcpy(info->type, "MicArray_2", I2C_NAME_SIZE);
			return 0;
		} else if (client->addr == 0x41) {
			strlcpy(info->type, "MicArray_3", I2C_NAME_SIZE);
			return 0;
		}
	}

	return -ENODEV;
}

static const unsigned short es7210_i2c_addr[] = {
#if ES7210_CHANNELS_MAX > 0
	0x40,
#endif

#if ES7210_CHANNELS_MAX > 4
	0x43,
#endif

#if ES7210_CHANNELS_MAX > 8
	0x42,
#endif

#if ES7210_CHANNELS_MAX > 12
	0x41,
#endif

	I2C_CLIENT_END,
};
#endif

#if ES7210_MATCH_DTS_EN
/*
* device tree source or i2c_board_info both use to
* transfer hardware information to linux kernel,
* use one of them wil be OK
*/
#if 0
static struct i2c_board_info es7210_i2c_board_info[] = {
#if ES7210_CHANNELS_MAX > 0
	{I2C_BOARD_INFO("MicArray_0", 0x40),},	//es7210_0
#endif

#if ES7210_CHANNELS_MAX > 4
	{I2C_BOARD_INFO("MicArray_1", 0x43),},	//es7210_1
#endif

#if ES7210_CHANNELS_MAX > 8
	{I2C_BOARD_INFO("MicArray_2", 0x42),},	//es7210_2
#endif

#if ES7210_CHANNELS_MAX > 12
	{I2C_BOARD_INFO("MicArray_3", 0x41),},	//es7210_3
#endif
};
#endif
static const struct of_device_id es7210_dt_ids[] = {
#if ES7210_CHANNELS_MAX > 0
	{.compatible = "es7210_MicArray_0",},	//es7210_0
#endif

#if ES7210_CHANNELS_MAX > 4
	{.compatible = "es7210_MicArray_1",},	//es7210_1
#endif

#if ES7210_CHANNELS_MAX > 8
	{.compatible = "es7210_MicArray_2",},	//es7210_2
#endif

#if ES7210_CHANNELS_MAX > 12
	{.compatible = "es7210_MicArray_3",},	//es7210_3
#endif
};
#endif

static const struct i2c_device_id es7210_i2c_id[] = {
#if ES7210_CHANNELS_MAX > 0
	{"es7210_MicArray_0", 0},	//es7210_0
#endif

#if ES7210_CHANNELS_MAX > 4
	{"es7210_MicArray_1", 1},	//es7210_1
#endif

#if ES7210_CHANNELS_MAX > 8
	{"es7210_MicArray_2", 2},	//es7210_2
#endif

#if ES7210_CHANNELS_MAX > 12
	{"es7210_MicArray_3", 3},	//es7210_3
#endif
	{}
};

MODULE_DEVICE_TABLE(i2c, es7210_i2c_id);

struct i2c_driver es7210_i2c_driver = {
	.driver = {
		   .name = "es7210",
		   .owner = THIS_MODULE,
#if ES7210_MATCH_DTS_EN
		   .of_match_table = es7210_dt_ids,
#endif
		   },
	.probe = es7210_i2c_probe,
	.remove = __exit_p(es7210_i2c_remove),
	.class = I2C_CLASS_HWMON,
	.id_table = es7210_i2c_id,
#if !ES7210_MATCH_DTS_EN
	.address_list = es7210_i2c_addr,
	.detect = es7210_i2c_detect,
#endif
};

static int __init es7210_modinit(void)
{
	int ret;
//#if ES7210_MATCH_DTS_EN
#if 0
	int i;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
#endif
	printk("%s enter\n", __func__);

//#if ES7210_MATCH_DTS_EN
#if 0
/*
* Notes:
* if the device has been declared in DTS tree,
* here don't need to create new i2c device with i2c_board_info.
*/
	adapter = i2c_get_adapter(ES7210_I2C_BUS_NUM);
	if (!adapter) {
		printk("i2c_get_adapter() fail!\n");
		return -ENODEV;
	}
	printk("%s() begin0000", __func__);

	for (i = 0; i < ADC_DEV_MAXNUM; i++) {
		client = i2c_new_device(adapter, &es7210_i2c_board_info[i]);
		printk("%s() i2c_new_device\n", __func__);
		if (!client)
			return -ENODEV;
	}
	i2c_put_adapter(adapter);
#endif
	ret = i2c_add_driver(&es7210_i2c_driver);
	if (ret != 0)
		printk("Failed to register es7210 i2c driver : %d \n", ret);
	return ret;
}

/*
* here, late_initcall is used in RK3308-firefly SDK, becase ES7210 driver
* needs to be installed after others codec driver in this SDK.
* if your SDK doesn't have this request, module_init is prefered
*/
late_initcall(es7210_modinit);
//module_init(es7210_modinit);
static void __exit es7210_exit(void)
{
	i2c_del_driver(&es7210_i2c_driver);
}

module_exit(es7210_exit);
MODULE_DESCRIPTION("ASoC ES7210 audio adc driver");
MODULE_AUTHOR("David Yang <yangxiaohua@everest-semi.com>");
MODULE_LICENSE("GPL v2");
