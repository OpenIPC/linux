/* sound\soc\sunxi\hifi-dsp\sunxi-daudio.c
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#include <sound/aw_rpaf/rpmsg_hifi.h>
#include <sound/aw_rpaf/component-core.h>
#include "sunxi-daudio.h"
#include "sunxi-snddaudio.h"

#define	DRV_NAME	"sunxi-hifi-daudio"

#define	SUNXI_DAUDIO_RATES	(SNDRV_PCM_RATE_8000_48000)

#define	SUNXI_DAUDIO_EXTERNAL_TYPE	1

static long playback_component_type;
module_param(playback_component_type, long, 0644);
MODULE_PARM_DESC(playback_component_type, "playback component type for dump");

static long capture_component_type;
module_param(capture_component_type, long, 0644);
MODULE_PARM_DESC(capture_component_type, "capture component type for dump");

#ifdef CONFIG_SND_SUNXI_HIFI_DAUDIO_DEBUG
static struct daudio_reg_label reg_labels[] = {
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_CTL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FMT0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FMT1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_INTSTA),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FIFOCTL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FIFOSTA),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_INTCTL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_CLKDIV),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TXCNT),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCNT),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_CHCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX0CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX1CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX2CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX3CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX0CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX0CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX1CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX1CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX2CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX2CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX3CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX3CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP2),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP3),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_DEBUG),
	DAUDIO_REG_LABEL_END,
};

static ssize_t show_daudio_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(dev);
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_mem_info *mem_info = NULL;
	unsigned int res_size = 0;
	int count = 0;
	unsigned int reg_val;
	int ret = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dev, "sunxi_daudio is NULL!\n");
		return count;
	}
	dts_info = &sunxi_daudio->dts_info;
	mem_info = &dts_info->mem_info;
	res_size = resource_size(&mem_info->res);

	count = snprintf(buf, PAGE_SIZE, "Dump daudio reg:\n");
	if (count > 0) {
		ret += count;
	} else {
		dev_err(dev, "snprintf start error=%d.\n", count);
		return 0;
	}

	while ((reg_labels[i].name != NULL) &&
		(reg_labels[i].address <= res_size)) {
		regmap_read(mem_info->regmap, reg_labels[i].address, &reg_val);
		count = snprintf(buf + ret, PAGE_SIZE - ret,
			"%-23s[0x%02X]: 0x%08X\n",
			reg_labels[i].name,
			(reg_labels[i].address), reg_val);
		if (count > 0) {
			ret += count;
		} else {
			dev_err(dev, "snprintf [i=%d] error=%d.\n", i, count);
			break;
		}
		if (ret > PAGE_SIZE) {
			ret = PAGE_SIZE;
			break;
		}
		i++;
	}

	return ret;
}

/* ex:
 *param 1: 0 read;1 write
 *param 2: reg value;
 *param 3: write value;
	read:
		echo 0,0x0 > daudio_reg
	write:
		echo 1,0x00,0xa > daudio_reg
*/
static ssize_t store_daudio_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	unsigned int input_reg_val = 0;
	unsigned int input_reg_offset = 0;
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(dev);
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_mem_info *mem_info = NULL;
	unsigned int res_size = 0;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dev, "sunxi_daudio is NULL!\n");
		return count;
	}
	dts_info = &sunxi_daudio->dts_info;
	mem_info = &dts_info->mem_info;
	res_size = resource_size(&mem_info->res),

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag, &input_reg_offset,
			&input_reg_val);

	if (ret == 3 || ret == 2) {
		if (!(rw_flag == 1 || rw_flag == 0)) {
			dev_err(dev, "rw_flag should be 0(read) or 1(write).\n");
			return count;
		}
		if (input_reg_offset > res_size) {
			dev_err(dev, "the reg offset is invalid! [0x0 - 0x%x]\n",
				res_size);
			return count;
		}
		if (rw_flag) {
			regmap_write(mem_info->regmap, input_reg_offset,
					input_reg_val);
		}
		regmap_read(mem_info->regmap, input_reg_offset, &reg_val_read);
		pr_err("\n\n Reg[0x%x] : 0x%x\n\n", input_reg_offset, reg_val_read);
	} else {
		pr_err("ret:%d, The num of params invalid!\n", ret);
		pr_err("\nExample(reg range: 0x0 - 0x%x):\n", res_size);
		pr_err("\nRead reg[0x04]:\n");
		pr_err("      echo 0,0x04 > daudio_reg\n");
		pr_err("Write reg[0x04]=0x10\n");
		pr_err("      echo 1,0x04,0x10 > daudio_reg\n");
	}

	return count;
}

static DEVICE_ATTR(daudio_reg, 0644, show_daudio_reg, store_daudio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_daudio_reg.attr,
	NULL,
};

static struct attribute_group daudio_debug_attr_group = {
	.name	= "daudio_debug",
	.attrs	= audio_debug_attrs,
};
#endif

/* dts pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_hifi_daudio_controls[] = {
	SOC_SINGLE("sunxi daudio loopback debug", SUNXI_DAUDIO_CTL,
		LOOP_EN, 1, 0),
};

static int sunxi_hifi_daudio_init(struct sunxi_daudio_info *sunxi_daudio,
					struct snd_soc_dai *dai)
{
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data *pdata_info = &dts_info->pdata_info;
	struct msg_substream_package *msg_playback = &sunxi_daudio->msg_playback;
	struct msg_substream_package *msg_capture = &sunxi_daudio->msg_capture;
	struct msg_mixer_package *msg_mixer = &sunxi_daudio->msg_mixer;
//	struct snd_soc_dsp_mixer *soc_mixer = &msg_mixer->soc_mixer;
	struct msg_debug_package *msg_debug = &sunxi_daudio->msg_debug;
//	struct snd_soc_dsp_debug *soc_debug = &msg_debug->soc_debug;
	struct snd_dsp_component *dsp_component;
	struct msg_component_package *msg_component;
//	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	int ret = 0;

	sunxi_daudio->cpu_dai = dai;

	if (sunxi_daudio->playback_dma_param.cma_kbytes != 0) {
		/* init for playback */
		init_waitqueue_head(&msg_playback->tsleep);
		spin_lock_init(&msg_playback->lock);
		msg_playback->wakeup_flag = 0;
		soc_substream = &msg_playback->soc_substream;
		pcm_params = &soc_substream->params;
		/* 1:capture; 0:playback */
		pcm_params->stream = SND_STREAM_PLAYBACK;
		pcm_params->card = pdata_info->dsp_card;
		pcm_params->device = pdata_info->dsp_device;
		/*
		 * 根据名字匹配(delete: hw:):
		 * 0: hw:audiocodec;
		 * 1: hw:snddmic;
		 * 2: hw:snddaudio0;
		 */
		snprintf(pcm_params->driver, 31, "snddaudio%d", pdata_info->dsp_daudio);
		/* dma buffer alloc */
		sunxi_dma_params_alloc_dma_area(dai, &sunxi_daudio->playback_dma_param);
		if (!sunxi_daudio->playback_dma_param.dma_area) {
			dev_err(dai->dev, "playback dmaengine alloc coherent failed.\n");
			ret = -ENOMEM;
			goto err_dma_alloc_playback;
		}

		/* init for playback component */
		dsp_component = &sunxi_daudio->dsp_playcomp;
		msg_component = &dsp_component->msg_component;
		init_waitqueue_head(&msg_component->tsleep);
		spin_lock_init(&msg_component->lock);
		msg_component->wakeup_flag = 0;
	}

	if (sunxi_daudio->capture_dma_param.cma_kbytes != 0) {
		/* init for capture */
		init_waitqueue_head(&msg_capture->tsleep);
		spin_lock_init(&msg_capture->lock);
		msg_capture->wakeup_flag = 0;
		soc_substream = &msg_capture->soc_substream;
		pcm_params = &soc_substream->params;
		pcm_params->stream = SND_STREAM_CAPTURE;
		pcm_params->card = pdata_info->dsp_card;
		pcm_params->device = pdata_info->dsp_device;
		snprintf(pcm_params->driver, 31, "snddaudio%d", pdata_info->dsp_daudio);
		/* dma buffer alloc */
		sunxi_dma_params_alloc_dma_area(dai, &sunxi_daudio->capture_dma_param);
		if (!sunxi_daudio->capture_dma_param.dma_area) {
			dev_err(dai->dev, "capture dmaengine alloc coherent failed.\n");
			ret = -ENOMEM;
			goto err_dma_alloc_capture;
		}

		/* init for capture component */
		dsp_component = &sunxi_daudio->dsp_capcomp;
		msg_component = &dsp_component->msg_component;
		init_waitqueue_head(&msg_component->tsleep);
		spin_lock_init(&msg_component->lock);
		msg_component->wakeup_flag = 0;

		/* init for mixer */
		init_waitqueue_head(&msg_mixer->tsleep);
		spin_lock_init(&msg_mixer->lock);
		msg_mixer->wakeup_flag = 0;

		/* init for debug */
		init_waitqueue_head(&msg_debug->tsleep);
		spin_lock_init(&msg_debug->lock);
		msg_debug->wakeup_flag = 0;
	}

	/* register sunxi_daudio_info to rpmsg_hifi driver */
	sunxi_hifi_register_sound_drv_info(pcm_params->driver, sunxi_daudio);

	/* pdata_info */
	return 0;

err_dma_alloc_capture:
	sunxi_dma_params_free_dma_area(dai, &sunxi_daudio->playback_dma_param);
err_dma_alloc_playback:
	return ret;
}

static int sunxi_hifi_daudio_uninit(struct sunxi_daudio_info *sunxi_daudio,
					struct snd_soc_dai *dai)
{
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;

	sunxi_dma_params_free_dma_area(dai, &sunxi_daudio->playback_dma_param);
	sunxi_dma_params_free_dma_area(dai, &sunxi_daudio->capture_dma_param);

	msg_substream = &sunxi_daudio->msg_playback;
	soc_substream = &msg_substream->soc_substream;
	/* for unregister sunxi_daudio_info from rpmsg_hifi driver */
	sunxi_hifi_unregister_sound_drv_info(soc_substream->params.driver, sunxi_daudio);

	return 0;
}

static int sunxi_hifi_daudio_dai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is NULL!\n", __func__);
		return -ENOMEM;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_daudio->playback_dma_param);
		msg_substream = &sunxi_daudio->msg_playback;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_playback;
		sunxi_daudio->playing = 0;
		soc_substream = &msg_substream->soc_substream;
		soc_substream->output_addr = sunxi_daudio->playback_dma_param.phy_addr;
		dev_dbg(dai->dev, "coherent phy_addr:0x%x\n", soc_substream->output_addr);
	} else {
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_daudio->capture_dma_param);
		msg_substream = &sunxi_daudio->msg_capture;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_capture;
		sunxi_daudio->capturing = 0;
		soc_substream = &msg_substream->soc_substream;
		soc_substream->input_addr = sunxi_daudio->capture_dma_param.phy_addr;
		dev_dbg(dai->dev, "coherent phy_addr:0x%x\n", soc_substream->input_addr);
	}
	msg_substream->substream = substream;

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	awrpaf_debug("\n");
	soc_substream->cmd_val = SND_SOC_DSP_PCM_STARTUP;
	ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_daudio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_platform_data *pdata_info = NULL;
	unsigned int frame_bytes;
	unsigned int sample_bits;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	dts_info = &sunxi_daudio->dts_info;
	mem_info = &dts_info->mem_info;
	pdata_info = &dts_info->pdata_info;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg_substream = &sunxi_daudio->msg_playback;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_playback;
	} else {
		msg_substream = &sunxi_daudio->msg_capture;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_capture;
	}
	soc_substream = &msg_substream->soc_substream;

	mutex_lock(rpmsg_mutex);
	pcm_params = &soc_substream->params;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		pcm_params->format = SND_PCM_FORMAT_S16_LE;
		sample_bits = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		pcm_params->format = SND_PCM_FORMAT_S24_LE;
		sample_bits = 32;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		pcm_params->format = SND_PCM_FORMAT_S32_LE;
		sample_bits = 32;
		break;
	default:
		mutex_unlock(rpmsg_mutex);
		dev_err(sunxi_daudio->dev, "unrecognized format\n");
		return -EINVAL;
	}
	pcm_params->channels = params_channels(params);
	pcm_params->rate = params_rate(params);
	pcm_params->period_size = params_period_size(params);
	pcm_params->periods = params_periods(params);
	pcm_params->pcm_frames = pcm_params->period_size;
	/* 在流中buffer务必一致大小, 代码中务必检查！ */
	pcm_params->buffer_size = params_buffer_size(params);
	frame_bytes = pcm_params->channels * sample_bits / 8;

	dev_info(sunxi_daudio->dev, "======== hw_params ========\n");
	dev_info(sunxi_daudio->dev, "pcm_params->format:%u\n", pcm_params->format);
	dev_info(sunxi_daudio->dev, "pcm_params->channels:%u\n", pcm_params->channels);
	dev_info(sunxi_daudio->dev, "pcm_params->rate:%u\n", pcm_params->rate);
	dev_info(sunxi_daudio->dev, "pcm_params->period_size:%u\n", pcm_params->period_size);
	dev_info(sunxi_daudio->dev, "pcm_params->periods:%u\n", pcm_params->periods);
	dev_info(sunxi_daudio->dev, "pcm_params->pcm_frames:%u\n", pcm_params->pcm_frames);
	dev_info(sunxi_daudio->dev, "pcm_params->buffer_size:%u\n", pcm_params->buffer_size);
	dev_info(sunxi_daudio->dev, "===========================\n");
	soc_substream->input_size = pcm_params->period_size * frame_bytes;
	soc_substream->output_size = pcm_params->period_size * frame_bytes;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		struct snd_dsp_component *dsp_component = &sunxi_daudio->dsp_playcomp;
		struct msg_component_package *msg_component = &dsp_component->msg_component;
		struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;

		soc_component->component_type = playback_component_type;

		ret = sunxi_hifi_substream_set_stream_component(SUNXI_HIFI0,
				dai, soc_substream, dsp_component);
		if (ret < 0) {
			dev_err(dai->dev, "set stream component send failed.\n");
			return ret;
		}
	} else {
		struct snd_dsp_component *dsp_component = &sunxi_daudio->dsp_capcomp;
		struct msg_component_package *msg_component = &dsp_component->msg_component;
		struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;

		soc_component->component_type = capture_component_type;

		ret = sunxi_hifi_substream_set_stream_component(SUNXI_HIFI0,
				dai, soc_substream, dsp_component);
		if (ret < 0) {
			dev_err(dai->dev, "set stream component send failed.\n");
			return ret;
		}
	}

	/* 发送给msgbox to dsp */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_HW_PARAMS;
	awrpaf_debug("\n");
	ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_daudio_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg_substream = &sunxi_daudio->msg_playback;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_playback;
		/* wait for stop playback */
		while (sunxi_daudio->playing)
			msleep(20);
	} else {
		msg_substream = &sunxi_daudio->msg_capture;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_capture;
		/* wait for stop capture */
		while (sunxi_daudio->capturing)
			msleep(20);
	}
	soc_substream = &msg_substream->soc_substream;

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	awrpaf_debug("\n");
	soc_substream->cmd_val = SND_SOC_DSP_PCM_PREPARE;
	ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_daudio_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct work_struct *trigger_work;
	struct workqueue_struct *wq_pcm;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_daudio->msg_playback;
			soc_substream = &msg_substream->soc_substream;
			soc_substream->cmd_val = SND_SOC_DSP_PCM_WRITEI;
			trigger_work = &sunxi_daudio->trigger_work_playback;
			wq_pcm = sunxi_daudio->wq_playback;
		} else {
			msg_substream = &sunxi_daudio->msg_capture;
			soc_substream = &msg_substream->soc_substream;
			soc_substream->cmd_val = SND_SOC_DSP_PCM_READI;
			trigger_work = &sunxi_daudio->trigger_work_capture;
			wq_pcm = sunxi_daudio->wq_capture;
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if ((substream->stream == SNDRV_PCM_STREAM_PLAYBACK) &&
			(substream->runtime->status->state == SNDRV_PCM_STATE_DRAINING)) {
			dev_info(dai->dev, "drain stopping\n");
			return 0;
		}
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_daudio->msg_playback;
			trigger_work = &sunxi_daudio->trigger_work_playback;
			wq_pcm = sunxi_daudio->wq_playback;
		} else {
			msg_substream = &sunxi_daudio->msg_capture;
			trigger_work = &sunxi_daudio->trigger_work_capture;
			wq_pcm = sunxi_daudio->wq_capture;
		}
		soc_substream = &msg_substream->soc_substream;
		soc_substream->cmd_val = SND_SOC_DSP_PCM_STOP;
		break;
	case SNDRV_PCM_TRIGGER_DRAIN:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_daudio->msg_playback;
			trigger_work = &sunxi_daudio->trigger_work_playback;
			wq_pcm = sunxi_daudio->wq_playback;
			soc_substream = &msg_substream->soc_substream;
			soc_substream->cmd_val = SND_SOC_DSP_PCM_DRAIN;
			dev_info(dai->dev, "drain starting\n");
		}
		break;
	default:
		return -EINVAL;
	}
	queue_work(wq_pcm, trigger_work);

	return 0;
}

static void sunxi_hifi_daudio_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_dsp_component *dsp_component;
	struct mutex *rpmsg_mutex;
	unsigned int *work_running;
	int i = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg_substream = &sunxi_daudio->msg_playback;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_playback;
		work_running = &sunxi_daudio->playing;
	} else {
		msg_substream = &sunxi_daudio->msg_capture;
		rpmsg_mutex = &sunxi_daudio->rpmsg_mutex_capture;
		work_running = &sunxi_daudio->capturing;
	}
	soc_substream = &msg_substream->soc_substream;

	awrpaf_debug("\n");
	for (i = 0; i < 50; i++) {
		if (*work_running == 0)
			break;
		msleep(20);
	}
	awrpaf_debug("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dsp_component = &sunxi_daudio->dsp_playcomp;
	else
		dsp_component = &sunxi_daudio->dsp_capcomp;

	sunxi_hifi_substream_release_stream_component(dai, dsp_component);

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
	sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);

	msg_substream->substream = NULL;
}

static struct snd_soc_dai_ops sunxi_hifi_daudio_dai_ops = {
	.startup = sunxi_hifi_daudio_dai_startup,
	.hw_params = sunxi_hifi_daudio_hw_params,
	.prepare = sunxi_hifi_daudio_prepare,
	.trigger = sunxi_hifi_daudio_trigger,
	.shutdown = sunxi_hifi_daudio_shutdown,
};

static int sunxi_hifi_daudio_probe(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_daudio->msg_capture;
	struct msg_substream_package *msg_playback = &sunxi_daudio->msg_playback;
	struct snd_soc_dsp_substream *soc_substream;
	int ret = 0;

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, &sunxi_daudio->playback_dma_param,
				&sunxi_daudio->capture_dma_param);

	sunxi_hifi_daudio_init(sunxi_daudio, dai);

	dev_info(dai->dev, "%s start.\n", __func__);
	/* 发送给msgbox to dsp */

	if (sunxi_daudio->playback_dma_param.cma_kbytes != 0) {
		/* for playback */
		soc_substream = &msg_playback->soc_substream;
		soc_substream->cmd_val = SND_SOC_DSP_PCM_PROBE;
		ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
								SNDRV_PCM_STREAM_PLAYBACK,
								MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		if (ret < 0) {
			dev_err(dai->dev, "%s probe playback failed.\n", __func__);
			goto err_probe_playback;
		} else if (soc_substream->ret_val < 0) {
			dev_err(dai->dev, "%s probe playback dsp failed.\n", __func__);
			ret = soc_substream->ret_val;
			goto err_probe_playback;
		}
	}

	dev_info(dai->dev, "process.\n");

	if (sunxi_daudio->capture_dma_param.cma_kbytes != 0) {
		/* for capture */
		soc_substream = &msg_capture->soc_substream;
		soc_substream->cmd_val = SND_SOC_DSP_PCM_PROBE;
		ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
								SNDRV_PCM_STREAM_CAPTURE,
								MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		if (ret < 0) {
			dev_err(dai->dev, "%s probe capture failed.\n", __func__);
			goto err_probe_capture;
		} else if (soc_substream->ret_val < 0) {
			dev_err(dai->dev, "%s probe capture dsp failed.\n", __func__);
			ret = soc_substream->ret_val;
			goto err_probe_capture;
		}
	}
	dev_info(dai->dev, "%s stop.\n", __func__);
	return soc_substream->ret_val;

err_probe_capture:
err_probe_playback:
	sunxi_hifi_daudio_uninit(sunxi_daudio, dai);
	return ret;
}

static int sunxi_hifi_daudio_remove(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_daudio->msg_capture;
	struct msg_substream_package *msg_playback = &sunxi_daudio->msg_playback;
	struct snd_soc_dsp_substream *soc_substream;
	int ret = 0;

	dev_info(dai->dev, "%s start.\n", __func__);
	/* for playback */
	soc_substream = &msg_playback->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_REMOVE;
	ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_PLAYBACK,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	if (ret < 0)
		dev_err(dai->dev, "remove send for playback failed.\n");

	/* for capture */
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_REMOVE;
	ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	if (ret < 0)
		dev_err(dai->dev, "remove send for capture failed.\n");

	sunxi_hifi_daudio_uninit(sunxi_daudio, dai);

	dev_info(dai->dev, "%s stop.\n", __func__);
	return ret;
}

static int sunxi_hifi_daudio_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_daudio->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_capture->soc_substream;

	pr_debug("[daudio] suspend .%s start\n", dev_name(sunxi_daudio->dev));

	/* only for capture */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SUSPEND;
	sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));

	pr_debug("[daudio] suspend .%s end \n", dev_name(sunxi_daudio->dev));
	return 0;
}

static int sunxi_hifi_daudio_resume(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_daudio->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_capture->soc_substream;

	pr_debug("[%s] resume .%s start\n", __func__,
			dev_name(sunxi_daudio->dev));

	/* only for capture */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_RESUME;
	sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));

	pr_debug("[%s] resume .%s end\n", __func__,
			dev_name(sunxi_daudio->dev));

	return 0;
}

static struct snd_soc_dai_driver sunxi_hifi_daudio_dai = {
	.probe = sunxi_hifi_daudio_probe,
	.suspend = sunxi_hifi_daudio_suspend,
	.resume = sunxi_hifi_daudio_resume,
	.remove = sunxi_hifi_daudio_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_hifi_daudio_dai_ops,
};

static const struct snd_soc_component_driver sunxi_hifi_daudio_component = {
	.name		= DRV_NAME,
	.controls	= sunxi_hifi_daudio_controls,
	.num_controls	= ARRAY_SIZE(sunxi_hifi_daudio_controls),
};

static struct sunxi_daudio_platform_data sunxi_hifi_daudio_pdata = {
	.daudio_type = SUNXI_DAUDIO_EXTERNAL_TYPE,

	.pcm_lrck_period = 128,
	.msb_lsb_first = 0,
	.sign_extend = 0,
	.tx_data_mode = 0,
	.rx_data_mode = 0,
	.slot_width_select = 32,
	.frame_type = 0,
	.tdm_config = 1,
	.mclk_div = 0,
};

static const struct of_device_id sunxi_hifi_daudio_of_match[] = {
	{
		.compatible = "allwinner,sunxi-hifi-daudio",
		.data = &sunxi_hifi_daudio_pdata,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_hifi_daudio_of_match);

static struct regmap_config sunxi_daudio_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DAUDIO_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static void trigger_work_playback_func(struct work_struct *work)
{
	struct sunxi_daudio_info *sunxi_daudio =
			container_of(work, struct sunxi_daudio_info, trigger_work_playback);
	struct msg_substream_package *msg_substream = &sunxi_daudio->msg_playback;
	struct snd_pcm_substream *substream = msg_substream->substream;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int pos = 0;
	int ret = 0;

	for (;;) {
		mutex_lock(&sunxi_daudio->rpmsg_mutex_playback);
		if (soc_substream->cmd_val == SND_SOC_DSP_PCM_WRITEI) {
			snd_pcm_stream_lock_irq(substream);
			if (runtime->status->state != SNDRV_PCM_STATE_RUNNING) {
				dev_warn(sunxi_daudio->cpu_dai->dev, "%s state:%d\n",
						__func__, runtime->status->state);
				snd_pcm_stream_unlock_irq(substream);
				mutex_unlock(&sunxi_daudio->rpmsg_mutex_playback);
				break;
			}
			sunxi_daudio->playing = 1;
			/* update the capture soc_substream output addr */
			pos = snd_dmaengine_pcm_pointer_no_residue(substream);
			soc_substream->input_addr = runtime->dma_addr + frames_to_bytes(runtime, pos);
			snd_pcm_stream_unlock_irq(substream);

			dev_dbg(sunxi_daudio->cpu_dai->dev,
				"[%s] dma_addr:0x%llx, pos:%u, input_addr:0x%x\n",
				__func__, runtime->dma_addr, pos, soc_substream->input_addr);
			ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_daudio->cpu_dai,
				SNDRV_PCM_STREAM_PLAYBACK, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			mutex_unlock(&sunxi_daudio->rpmsg_mutex_playback);

			/* 拷贝并发送通知component */
			snd_soc_rpaf_pcm_update_stream_process(&sunxi_daudio->dsp_playcomp);

			if (ret != 0) {
				dev_err(sunxi_daudio->cpu_dai->dev,
					"%s state:%d, ret=%d\n", __func__, runtime->status->state, ret);
			}
			if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
				queue_work(sunxi_daudio->wq_playback, &sunxi_daudio->trigger_work_playback);
			}
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_STOP) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_daudio->cpu_dai,
				SNDRV_PCM_STREAM_PLAYBACK, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			sunxi_daudio->playing = 0;
			mutex_unlock(&sunxi_daudio->rpmsg_mutex_playback);
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_DRAIN) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_daudio->cpu_dai,
				SNDRV_PCM_STREAM_PLAYBACK, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));

			snd_pcm_stream_lock_irq(substream);
			snd_pcm_drain_done(substream);
			snd_pcm_update_state(substream, runtime);
			sunxi_daudio->playing = 0;
			snd_pcm_stream_unlock_irq(substream);

			awrpaf_debug("\n");
			mutex_unlock(&sunxi_daudio->rpmsg_mutex_playback);
			break;
		}
		mutex_unlock(&sunxi_daudio->rpmsg_mutex_playback);
	}
}

static void trigger_work_capture_func(struct work_struct *work)
{
	struct sunxi_daudio_info *sunxi_daudio =
			container_of(work, struct sunxi_daudio_info, trigger_work_capture);
	struct msg_substream_package *msg_substream = &sunxi_daudio->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;
	struct snd_pcm_substream *substream = msg_substream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int pos = 0;
	int ret = 0;

	for (;;) {
		mutex_lock(&sunxi_daudio->rpmsg_mutex_capture);
		if (soc_substream->cmd_val == SND_SOC_DSP_PCM_READI) {
			snd_pcm_stream_lock_irq(substream);
			if (runtime->status->state != SNDRV_PCM_STATE_RUNNING) {
				dev_err(sunxi_daudio->cpu_dai->dev,
					"%s state:%d\n", __func__, runtime->status->state);
				snd_pcm_stream_unlock_irq(substream);
				mutex_unlock(&sunxi_daudio->rpmsg_mutex_capture);
				break;
			}
			sunxi_daudio->capturing = 1;
			/* update the capture soc_substream output addr */
			pos = snd_dmaengine_pcm_pointer_no_residue(substream);
			soc_substream->output_addr = runtime->dma_addr + frames_to_bytes(runtime, pos);
			snd_pcm_stream_unlock_irq(substream);

			dev_dbg(sunxi_daudio->cpu_dai->dev,
				"[%s] dma_addr:0x%llx, pos:%u, output_addr:0x%x\n",
				 __func__, runtime->dma_addr, pos, soc_substream->output_addr);
			/* send to dsp */
			ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_daudio->cpu_dai,
				SNDRV_PCM_STREAM_CAPTURE, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			mutex_unlock(&sunxi_daudio->rpmsg_mutex_capture);

			/* 拷贝并发送通知component */
			snd_soc_rpaf_pcm_update_stream_process(&sunxi_daudio->dsp_capcomp);

			if (ret != 0) {
				dev_err(sunxi_daudio->cpu_dai->dev,
					"%s state:%d, ret=%d\n", __func__, runtime->status->state, ret);
			}
			if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
				queue_work(sunxi_daudio->wq_capture, &sunxi_daudio->trigger_work_capture);
			}
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_STOP) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_daudio->cpu_dai,
				SNDRV_PCM_STREAM_CAPTURE, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			sunxi_daudio->capturing = 0;
			mutex_unlock(&sunxi_daudio->rpmsg_mutex_capture);
			break;
		}
		mutex_unlock(&sunxi_daudio->rpmsg_mutex_capture);
	}
}

static int sunxi_hifi_daudio_dev_probe(struct platform_device *pdev)
{
	const struct of_device_id *match = NULL;
	struct sunxi_daudio_info *sunxi_daudio = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_platform_data *pdata_info = NULL;
	struct daudio_voltage_supply *vol_supply = NULL;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

	dev_info(&pdev->dev, "%s start.\n", __func__);

	match = of_match_device(sunxi_hifi_daudio_of_match, &pdev->dev);
	if (match) {
		sunxi_daudio = devm_kzalloc(&pdev->dev,
					sizeof(struct sunxi_daudio_info),
					GFP_KERNEL);
		if (IS_ERR_OR_NULL(sunxi_daudio)) {
			dev_err(&pdev->dev, "alloc sunxi_daudio failed\n");
			ret = -ENOMEM;
			goto err_devm_malloc_sunxi_daudio;
		}
		dev_set_drvdata(&pdev->dev, sunxi_daudio);
		sunxi_daudio->dev = &pdev->dev;
		dts_info = &sunxi_daudio->dts_info;
		mem_info = &dts_info->mem_info;
		pdata_info = &dts_info->pdata_info;
		vol_supply = &dts_info->vol_supply;

		memcpy(pdata_info, match->data,
			sizeof(struct sunxi_daudio_platform_data));
	} else {
		dev_err(&pdev->dev, "node match failed\n");
		return -EINVAL;
	}

	ret = of_address_to_resource(np, 0, &(mem_info->res));
	if (ret) {
		dev_err(&pdev->dev, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	switch (pdata_info->daudio_type) {
	case	SUNXI_DAUDIO_EXTERNAL_TYPE:
		ret = of_property_read_u32(np, "tdm_num", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tdm configuration missing\n");
			/*
			 * warnning just continue,
			 * making tdm_num as default setting
			 */
			pdata_info->tdm_num = 0;
		} else {
			/*
			 * FIXME, for channel number mess,
			 * so just not check channel overflow
			 */
			pdata_info->tdm_num = temp_val;
		}

		ret = of_property_read_u32(np, "playback_cma", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "playback cma kbytes config missing or invalid.\n");
			dts_info->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		} else {
			if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
				temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
			dts_info->playback_cma = temp_val;
		}

		ret = of_property_read_u32(np, "capture_cma", &temp_val);
		if (ret != 0) {
			dev_warn(&pdev->dev, "capture cma kbytes config missing or invalid.\n");
			dts_info->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		} else {
			if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
				temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
			dts_info->capture_cma = temp_val;
		}
		if (!dts_info->playback_cma && !dts_info->capture_cma) {
			dev_warn(&pdev->dev, "playback and capture cma is 0\n");
			goto err_cma_bytes;
		}

		sunxi_daudio->playback_dma_param.cma_kbytes = dts_info->playback_cma;
		sunxi_daudio->playback_dma_param.fifo_size = DAUDIO_TX_FIFO_SIZE;

		sunxi_daudio->capture_dma_param.cma_kbytes = dts_info->capture_cma;
		sunxi_daudio->capture_dma_param.fifo_size = DAUDIO_RX_FIFO_SIZE;

		vol_supply->regulator_name = NULL;
		ret = of_property_read_string(np, "daudio_regulator",
				&vol_supply->regulator_name);
		if (ret < 0) {
			dev_warn(&pdev->dev, "regulator missing or invalid\n");
			vol_supply->daudio_regulator = NULL;
		} else {
			vol_supply->daudio_regulator =
				regulator_get(NULL, vol_supply->regulator_name);
			if (IS_ERR_OR_NULL(vol_supply->daudio_regulator)) {
				dev_err(&pdev->dev, "get duaido[%d] vcc-pin failed\n",
					pdata_info->tdm_num);
				ret = -EFAULT;
				goto err_regulator_get;
			}
			ret = regulator_set_voltage(vol_supply->daudio_regulator,
						3300000, 3300000);
			if (ret < 0) {
				dev_err(&pdev->dev, "set duaido[%d] voltage failed\n",
						pdata_info->tdm_num);
				ret = -EFAULT;
				goto err_regulator_set_vol;
			}
			ret = regulator_enable(vol_supply->daudio_regulator);
			if (ret < 0) {
				dev_err(&pdev->dev, "enable duaido[%d] vcc-pin failed\n",
						pdata_info->tdm_num);
				ret = -EFAULT;
				goto err_regulator_enable;
			}
		}

		ret = of_property_read_u32(np, "pcm_lrck_period", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "pcm_lrck_period config missing or invalid\n");
			pdata_info->pcm_lrck_period = 0;
		} else {
			pdata_info->pcm_lrck_period = temp_val;
		}

		ret = of_property_read_u32(np, "msb_lsb_first", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "msb_lsb_first config missing or invalid\n");
			pdata_info->msb_lsb_first = 0;
		} else
			pdata_info->msb_lsb_first = temp_val;

		ret = of_property_read_u32(np, "slot_width_select", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "slot_width_select config missing or invalid\n");
			pdata_info->slot_width_select = 0;
		} else {
			pdata_info->slot_width_select = temp_val;
		}

		ret = of_property_read_u32(np, "frametype", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "frametype config missing or invalid\n");
			pdata_info->frame_type = 0;
		} else {
			pdata_info->frame_type = temp_val;
		}

		ret = of_property_read_u32(np, "sign_extend", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "sign_extend config missing or invalid\n");
			pdata_info->sign_extend = 0;
		} else
			pdata_info->sign_extend = temp_val;

		ret = of_property_read_u32(np, "tx_data_mode", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tx_data_mode config missing or invalid\n");
			pdata_info->tx_data_mode = 0;
		} else
			pdata_info->tx_data_mode = temp_val;

		ret = of_property_read_u32(np, "rx_data_mode", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "rx_data_mode config missing or invalid\n");
			pdata_info->rx_data_mode = 0;
		} else
			pdata_info->rx_data_mode = temp_val;

		ret = of_property_read_u32(np, "tdm_config", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tdm_config config missing or invalid\n");
			pdata_info->tdm_config = 1;
		} else {
			pdata_info->tdm_config = temp_val;
		}

		ret = of_property_read_u32(np, "mclk_div", &temp_val);
		if (ret < 0)
			pdata_info->mclk_div = 0;
		else
			pdata_info->mclk_div = temp_val;

		ret = of_property_read_u32(np, "tx_num", &temp_val);
		if (ret < 0)
			pdata_info->tx_num = SUNXI_DAUDIO_TXCH_NUM;
		else {
			if ((temp_val < SUNXI_DAUDIO_TXCH_NUM) && (temp_val != 0))
				pdata_info->tx_num = temp_val;
			else
				pdata_info->tx_num = SUNXI_DAUDIO_TXCH_NUM;
		}

		ret = of_property_read_u32(np, "tx_chmap0", &temp_val);
		if (ret < 0)
			pdata_info->tx_chmap0 = SUNXI_DEFAULT_TXCHMAP0;
		else
			pdata_info->tx_chmap0 = temp_val;

		ret = of_property_read_u32(np, "tx_chmap1", &temp_val);
		if (ret < 0)
			pdata_info->tx_chmap1 = SUNXI_DEFAULT_TXCHMAP1;
		else
			pdata_info->tx_chmap1 = temp_val;

		ret = of_property_read_u32(np, "rx_num", &temp_val);
		if (ret < 0)
			pdata_info->rx_num = SUNXI_DAUDIO_RXCH_NUM;
		else {
			if ((temp_val < SUNXI_DAUDIO_RXCH_NUM) && (temp_val != 0))
				pdata_info->rx_num = temp_val;
			else
				pdata_info->rx_num = SUNXI_DAUDIO_RXCH_NUM;
		}

		ret = of_property_read_u32(np, "rx_chmap0", &temp_val);
		if (ret < 0) {
			if (pdata_info->rx_num == 1)
				pdata_info->rx_chmap0 = SUNXI_DEFAULT_RXCHMAP;
			else
				pdata_info->rx_chmap0 = SUNXI_DEFAULT_RXCHMAP0;
		} else {
			pdata_info->rx_chmap0 = temp_val;
		}

		ret = of_property_read_u32(np, "rx_chmap1", &temp_val);
		if (ret < 0)
			pdata_info->rx_chmap1 = SUNXI_DEFAULT_RXCHMAP1;
		else
			pdata_info->rx_chmap1 = temp_val;

		ret = of_property_read_u32(np, "rx_chmap2", &temp_val);
		if (ret < 0)
			pdata_info->rx_chmap2 = SUNXI_DEFAULT_RXCHMAP3;
		else
			pdata_info->rx_chmap2 = temp_val;

		ret = of_property_read_u32(np, "rx_chmap3", &temp_val);
		if (ret < 0)
			pdata_info->rx_chmap3 = SUNXI_DEFAULT_RXCHMAP3;
		else
			pdata_info->rx_chmap3 = temp_val;

		/* for hifi dsp param init config */
		ret = of_property_read_u32(np, "dsp_daudio", &temp_val);
		if (ret < 0)
			pdata_info->dsp_daudio = 0;
		else
			pdata_info->dsp_daudio = temp_val;

		switch (pdata_info->dsp_daudio) {
		case 0:
		case 1:
			break;
		default:
			dev_warn(&pdev->dev, "dsp_daudio setting overflow\n");
			ret = -EFAULT;
			goto err_dsp_daudio_tdm_num;
			break;
		}
		dev_warn(&pdev->dev, "dsp_daudio = %d\n", pdata_info->dsp_daudio);

		/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; */
		ret = of_property_read_u32(np, "dsp_card", &temp_val);
		if (ret < 0)
			pdata_info->dsp_card = 2;
		else
			pdata_info->dsp_card = temp_val;

		/* default is 0, for reserved */
		ret = of_property_read_u32(np, "dsp_device", &temp_val);
		if (ret < 0)
			pdata_info->dsp_device = 0;
		else
			pdata_info->dsp_device = temp_val;
		break;
	default:
		dev_err(&pdev->dev, "missing digital audio type\n");
		ret = -EINVAL;
		goto err_daudio_pdata_type;
	}

	mem_info->memregion = devm_request_mem_region(&pdev->dev,
					mem_info->res.start,
					resource_size(&mem_info->res),
					DRV_NAME);
	if (IS_ERR_OR_NULL(mem_info->memregion)) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem_info->membase = devm_ioremap(&pdev->dev,
					mem_info->memregion->start,
					resource_size(mem_info->memregion));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	sunxi_daudio_regmap_config.max_register = resource_size(&mem_info->res);
	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev,
					mem_info->membase,
					&sunxi_daudio_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_hifi_daudio_component,
					&sunxi_hifi_daudio_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

	ret = asoc_hifi_platform_register(&pdev->dev, SUNXI_HIFI_PCM_OPS_DAUDIO);
	if (ret) {
		dev_err(&pdev->dev, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_asoc_hifi_platform_register;
	}

	mutex_init(&sunxi_daudio->rpmsg_mutex_playback);
	mutex_init(&sunxi_daudio->rpmsg_mutex_capture);

	snprintf(sunxi_daudio->wq_capture_name, sizeof(sunxi_daudio->wq_capture_name),
			"hifi-tdm%d-c", pdata_info->dsp_daudio);
	sunxi_daudio->wq_capture = create_workqueue(sunxi_daudio->wq_capture_name);
	INIT_WORK(&sunxi_daudio->trigger_work_capture, trigger_work_capture_func);
//	queue_work(sunxi_daudio->wq_capture, &sunxi_daudio->trigger_work_capture);
	snprintf(sunxi_daudio->wq_playback_name, sizeof(sunxi_daudio->wq_playback_name),
			"hifi-tdm%d-p", pdata_info->dsp_daudio);
	sunxi_daudio->wq_playback = create_workqueue(sunxi_daudio->wq_playback_name);
	INIT_WORK(&sunxi_daudio->trigger_work_playback, trigger_work_playback_func);
//	queue_work(sunxi_daudio->wq_playback, &sunxi_daudio->trigger_work_playback);

#ifdef CONFIG_SND_SUNXI_HIFI_DAUDIO_DEBUG
	ret  = sysfs_create_group(&pdev->dev.kobj, &daudio_debug_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "failed to create attr group\n");
		goto err_sysfs_create_debug;
	}
#endif
	dev_info(&pdev->dev, "%s stop.\n", __func__);

	return 0;

#ifdef CONFIG_SND_SUNXI_HIFI_DAUDIO_DEBUG
err_sysfs_create_debug:
	mutex_destroy(&sunxi_daudio->rpmsg_mutex_playback);
	mutex_destroy(&sunxi_daudio->rpmsg_mutex_capture);
#endif
err_asoc_hifi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_register_component:
err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_region:
err_dsp_daudio_tdm_num:
	if (vol_supply->daudio_regulator)
		regulator_disable(vol_supply->daudio_regulator);
err_regulator_enable:
err_regulator_set_vol:
	if (vol_supply->daudio_regulator)
		regulator_put(vol_supply->daudio_regulator);
err_cma_bytes:
err_regulator_get:
err_daudio_pdata_type:
err_of_addr_to_resource:
	devm_kfree(&pdev->dev, sunxi_daudio);
err_devm_malloc_sunxi_daudio:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_hifi_daudio_dev_remove(struct platform_device *pdev)
{
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(&pdev->dev);
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_mem_info *mem_info = &dts_info->mem_info;
	struct daudio_voltage_supply *vol_supply = &dts_info->vol_supply;

	dev_info(&pdev->dev, "%s start.\n", __func__);

#ifdef CONFIG_SND_SUNXI_HIFI_DAUDIO_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &daudio_debug_attr_group);
#endif

	cancel_work_sync(&(sunxi_daudio->trigger_work_capture));
	destroy_workqueue(sunxi_daudio->wq_capture);
	cancel_work_sync(&(sunxi_daudio->trigger_work_playback));
	destroy_workqueue(sunxi_daudio->wq_playback);

	mutex_destroy(&sunxi_daudio->rpmsg_mutex_playback);
	mutex_destroy(&sunxi_daudio->rpmsg_mutex_capture);

	snd_soc_unregister_component(&pdev->dev);
	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
	if (!IS_ERR_OR_NULL(vol_supply->daudio_regulator)) {
		regulator_disable(vol_supply->daudio_regulator);
		regulator_put(vol_supply->daudio_regulator);
	}

	devm_kfree(&pdev->dev, sunxi_daudio);

	dev_info(&pdev->dev, "%s stop.\n", __func__);

	return 0;
}

static struct platform_driver sunxi_hifi_daudio_driver = {
	.probe = sunxi_hifi_daudio_dev_probe,
	.remove = __exit_p(sunxi_hifi_daudio_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_hifi_daudio_of_match,
	},
};

module_platform_driver(sunxi_hifi_daudio_driver);
MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DAI AUDIO ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-daudio");
