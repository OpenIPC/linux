/*
 * sound\sunxi-rpaf\soc\sunxi\hifi-dsp\sunxi-dmic.c
 *
 * (C) Copyright 2019-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
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
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/dma/sunxi-dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#include <sound/aw_rpaf/rpmsg_hifi.h>
#include <sound/aw_rpaf/component-core.h>
#include "sunxi-dmic.h"
#include "sunxi-snddmic.h"

#define	DRV_NAME	"sunxi-dmic"

#define	SUNXI_DMIC_RATES (SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_KNOT)

static long capture_component_type;
module_param(capture_component_type, long, 0644);
MODULE_PARM_DESC(capture_component_type, "capture component type for dump");

#ifdef CONFIG_SND_SUNXI_HIFI_DMIC_DEBUG
static struct sunxi_dmic_reg_label reg_labels[] = {
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_EN),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_SR),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CTR),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_INTC),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_INTS),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_FIFO_CTR),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_FIFO_STA),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CH_NUM),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CH_MAP),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CNT),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_DATA0_1_VOL),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_DATA2_3_VOL),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_HPF_CTRL),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_HPF_COEF),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_HPF_GAIN),
	SUNXI_DMIC_REG_LABEL_END,
};

static ssize_t show_dmic_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_dmic_info *sunxi_dmic = dev_get_drvdata(dev);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	int count = 0;
	unsigned int reg_val;
	int ret = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dev, "sunxi_dmic is NULL!\n");
		return count;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &dts_info->mem_info;

	count = snprintf(buf, PAGE_SIZE, "Dump dmic reg:\n");
	if (count > 0) {
		ret += count;
	} else {
		dev_err(dev, "snprintf start error=%d.\n", count);
		return 0;
	}

	while (reg_labels[i].name != NULL) {
		regmap_read(mem_info->regmap, reg_labels[i].value, &reg_val);
		count = snprintf(buf + ret, PAGE_SIZE - ret,
			"%-23s[0x%02X]: 0x%08X\n",
			reg_labels[i].name,
			(reg_labels[i].value), reg_val);
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
		echo 0,0x0 > dmic_reg
	write:
		echo 1,0x00,0xa > dmic_reg
*/
static ssize_t store_dmic_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	unsigned int input_reg_val = 0;
	unsigned int input_reg_offset = 0;
	struct sunxi_dmic_info *sunxi_dmic = dev_get_drvdata(dev);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dev, "sunxi_dmic is NULL!\n");
		return count;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &dts_info->mem_info;

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag, &input_reg_offset,
			&input_reg_val);

	if (ret == 3 || ret == 2) {
		if (!(rw_flag == 1 || rw_flag == 0)) {
			dev_err(dev, "rw_flag should be 0(read) or 1(write).\n");
			return count;
		}
		if (input_reg_offset > SUNXI_DMIC_REG_MAX) {
			pr_err("the reg offset is invalid! [0x0 - 0x%x]\n",
				SUNXI_DMIC_REG_MAX);
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
		pr_err("\nExample(reg range:0x0 - 0x%x):\n", SUNXI_DMIC_REG_MAX);
		pr_err("\nRead reg[0x04]:\n");
		pr_err("      echo 0,0x04 > dmic_reg\n");
		pr_err("Write reg[0x04]=0x10\n");
		pr_err("      echo 1,0x04,0x10 > dmic_reg\n");
	}

	return count;
}

static DEVICE_ATTR(dmic_reg, 0644, show_dmic_reg, store_dmic_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_dmic_reg.attr,
	NULL,
};

static struct attribute_group dmic_debug_attr_group = {
	.name	= "dmic_debug",
	.attrs	= audio_debug_attrs,
};
#endif

static int sunxi_hifi_dmic_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	unsigned int frame_bytes;
	unsigned int sample_bits;
	struct mutex *rpmsg_mutex;
	struct snd_dsp_component *dsp_component;
	struct msg_component_package *msg_component;
	struct snd_soc_dsp_component *soc_component;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &dts_info->mem_info;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		msg_substream = &sunxi_dmic->msg_capture;
		rpmsg_mutex = &sunxi_dmic->rpmsg_mutex_capture;
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
		dev_err(sunxi_dmic->dev, "unrecognized format\n");
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

	dev_info(sunxi_dmic->dev, "======== hw_params ========\n");
	dev_info(sunxi_dmic->dev, "pcm_params->format:%u\n", pcm_params->format);
	dev_info(sunxi_dmic->dev, "pcm_params->channels:%u\n", pcm_params->channels);
	dev_info(sunxi_dmic->dev, "pcm_params->rate:%u\n", pcm_params->rate);
	dev_info(sunxi_dmic->dev, "pcm_params->period_size:%u\n", pcm_params->period_size);
	dev_info(sunxi_dmic->dev, "pcm_params->periods:%u\n", pcm_params->periods);
	dev_info(sunxi_dmic->dev, "pcm_params->pcm_frames:%u\n", pcm_params->pcm_frames);
	dev_info(sunxi_dmic->dev, "pcm_params->buffer_size:%u\n", pcm_params->buffer_size);
	dev_info(sunxi_dmic->dev, "===========================\n");
	soc_substream->input_size = pcm_params->period_size * frame_bytes;
	soc_substream->output_size = pcm_params->period_size * frame_bytes;

	dsp_component = &sunxi_dmic->dsp_capcomp;
	msg_component = &dsp_component->msg_component;
	soc_component = &msg_component->soc_component;
	soc_component->component_type = capture_component_type;

	ret = sunxi_hifi_substream_set_stream_component(SUNXI_HIFI0,
			dai, soc_substream, dsp_component);
	if (ret < 0) {
		dev_err(dai->dev, "set stream component send failed.\n");
		return ret;
	}

	/* 发送给msgbox to dsp */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_HW_PARAMS;
	awrpaf_debug("\n");
	ret = sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_dmic_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct work_struct *trigger_work;
	struct workqueue_struct *wq_pcm;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		msg_substream = &sunxi_dmic->msg_capture;
		soc_substream = &msg_substream->soc_substream;
		soc_substream->cmd_val = SND_SOC_DSP_PCM_READI;
		trigger_work = &sunxi_dmic->trigger_work_capture;
		wq_pcm = sunxi_dmic->wq_capture;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		msg_substream = &sunxi_dmic->msg_capture;
		trigger_work = &sunxi_dmic->trigger_work_capture;
		wq_pcm = sunxi_dmic->wq_capture;
		soc_substream = &msg_substream->soc_substream;
		soc_substream->cmd_val = SND_SOC_DSP_PCM_STOP;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	queue_work(wq_pcm, trigger_work);
	return ret;
}

/*
 * Reset & Flush FIFO
 */
static int sunxi_hifi_dmic_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		msg_substream = &sunxi_dmic->msg_capture;
		rpmsg_mutex = &sunxi_dmic->rpmsg_mutex_capture;
		/* wait for stop capture */
		while (sunxi_dmic->capturing)
			msleep(20);
	}
	soc_substream = &msg_substream->soc_substream;

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	awrpaf_debug("\n");
	soc_substream->cmd_val = SND_SOC_DSP_PCM_PREPARE;
	ret = sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_dmic_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_dmic->capture_dma_param);

	msg_substream = &sunxi_dmic->msg_capture;
	rpmsg_mutex = &sunxi_dmic->rpmsg_mutex_capture;
	sunxi_dmic->capturing = 0;
	soc_substream = &msg_substream->soc_substream;
	soc_substream->input_addr = sunxi_dmic->capture_dma_param.phy_addr;
	dev_dbg(dai->dev, "coherent phy_addr:0x%x\n", soc_substream->input_addr);
	msg_substream->substream = substream;

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	awrpaf_debug("\n");
	soc_substream->cmd_val = SND_SOC_DSP_PCM_STARTUP;
	ret = sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
		return ret;
	}
	return soc_substream->ret_val;
}

static void sunxi_hifi_dmic_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	unsigned int *work_running;
	int i = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return;
	}

	sunxi_dmic->capture_en = 0;

	msg_substream = &sunxi_dmic->msg_capture;
	rpmsg_mutex = &sunxi_dmic->rpmsg_mutex_capture;
	work_running = &sunxi_dmic->capturing;
	soc_substream = &msg_substream->soc_substream;

	awrpaf_debug("\n");
	for (i = 0; i < 50; i++) {
		if (*work_running == 0)
			break;
		msleep(20);
	}
	awrpaf_debug("\n");

	sunxi_hifi_substream_release_stream_component(dai, &sunxi_dmic->dsp_capcomp);

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
	sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);

	msg_substream->substream = NULL;
}

static struct snd_soc_dai_ops sunxi_hifi_dmic_dai_ops = {
	.startup = sunxi_hifi_dmic_startup,
	.hw_params = sunxi_hifi_dmic_hw_params,
	.prepare = sunxi_hifi_dmic_prepare,
	.trigger = sunxi_hifi_dmic_trigger,
	.shutdown = sunxi_hifi_dmic_shutdown,
};

static int sunxi_hifi_dmic_init(struct sunxi_dmic_info *sunxi_dmic,
					struct snd_soc_dai *dai)
{
	struct sunxi_dmic_dts_info *dts_info = &sunxi_dmic->dts_info;
	struct msg_substream_package *msg_capture = &sunxi_dmic->msg_capture;
	struct msg_mixer_package *msg_mixer = &sunxi_dmic->msg_mixer;
//	struct snd_soc_dsp_mixer *soc_mixer = &msg_mixer->soc_mixer;
	struct msg_debug_package *msg_debug = &sunxi_dmic->msg_debug;
	struct msg_component_package *msg_component;
	struct snd_dsp_component *dsp_component;
//	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	int ret = 0;

	sunxi_dmic->cpu_dai = dai;
	/* init for capture */
	init_waitqueue_head(&msg_capture->tsleep);
	spin_lock_init(&msg_capture->lock);
	msg_capture->wakeup_flag = 0;
	soc_substream = &msg_capture->soc_substream;
	pcm_params = &soc_substream->params;
	pcm_params->stream = SND_STREAM_CAPTURE;
	pcm_params->card = dts_info->dsp_card;
	pcm_params->device = dts_info->dsp_device;
	snprintf(pcm_params->driver, 31, "snddmic");
	/* dma buffer alloc */
	sunxi_dma_params_alloc_dma_area(dai, &sunxi_dmic->capture_dma_param);
	if (!sunxi_dmic->capture_dma_param.dma_area) {
		dev_err(dai->dev, "capture dmaengine alloc coherent failed.\n");
		ret = -ENOMEM;
		goto err_dma_alloc_capture;
	}

	/* init for capture component */
	dsp_component = &sunxi_dmic->dsp_capcomp;
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

	/* register sunxi_dmic_info to rpmsg_hifi driver */
	sunxi_hifi_register_sound_drv_info(pcm_params->driver, sunxi_dmic);

	/* pdata_info */
	return 0;

err_dma_alloc_capture:
	return ret;
}

static int sunxi_hifi_dmic_uninit(struct sunxi_dmic_info *sunxi_dmic,
					struct snd_soc_dai *dai)
{
	struct msg_substream_package *msg_capture = &sunxi_dmic->msg_capture;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;

	sunxi_dma_params_free_dma_area(dai, &sunxi_dmic->capture_dma_param);

	soc_substream = &msg_capture->soc_substream;
	pcm_params = &soc_substream->params;
	sunxi_hifi_unregister_sound_drv_info(pcm_params->driver, sunxi_dmic);

	return 0;
}

/* Dmic module init status */
static int sunxi_hifi_dmic_probe(struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_dmic->msg_capture;
	struct snd_soc_dsp_substream *soc_substream;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	dev_info(dai->dev, "%s start.\n", __func__);
	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, NULL, &sunxi_dmic->capture_dma_param);

	sunxi_hifi_dmic_init(sunxi_dmic, dai);

	/* 发送给msgbox to dsp */
	/* for capture */
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_PROBE;
	ret = sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	if (ret < 0) {
		dev_err(dai->dev, "probe send failed.\n");
		sunxi_hifi_dmic_uninit(sunxi_dmic, dai);
		return -EFAULT;
	}
	dev_info(dai->dev, "%s stop.\n", __func__);

	return soc_substream->ret_val;
}

static int sunxi_hifi_dmic_suspend(struct snd_soc_dai *cpu_dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	struct msg_substream_package *msg_capture;
	struct snd_soc_dsp_substream *soc_substream;

	pr_debug("[DMIC]Enter %s\n", __func__);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(cpu_dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	msg_capture = &sunxi_dmic->msg_capture;
	soc_substream = &msg_capture->soc_substream;

	/* only for capture */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SUSPEND;
	sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, NULL, cpu_dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));

	pr_debug("[DMIC]End %s\n", __func__);

	return 0;
}

static int sunxi_hifi_dmic_resume(struct snd_soc_dai *cpu_dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	struct msg_substream_package *msg_capture;
	struct snd_soc_dsp_substream *soc_substream;

	pr_debug("[DMIC]Enter %s\n", __func__);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(cpu_dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	msg_capture = &sunxi_dmic->msg_capture;
	soc_substream = &msg_capture->soc_substream;

	/* only for capture */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_RESUME;
	sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, NULL, cpu_dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));

	pr_debug("[DMIC]End %s\n", __func__);

	return 0;
}

static int sunxi_hifi_dmic_remove(struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_dmic->msg_capture;
	struct snd_soc_dsp_substream *soc_substream;
	int ret = 0;

	dev_info(dai->dev, "%s start.\n", __func__);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	/* 发送给msgbox to dsp */
	/* for capture */
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_REMOVE;
	ret = sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	if (ret < 0) {
		dev_err(dai->dev, "remove send failed.\n");
		sunxi_hifi_dmic_uninit(sunxi_dmic, dai);
		return -EFAULT;
	}

	sunxi_hifi_dmic_uninit(sunxi_dmic, dai);
	dev_info(dai->dev, "%s stop.\n", __func__);

	return 0;
}

static struct snd_soc_dai_driver sunxi_hifi_dmic_dai = {
	.probe = sunxi_hifi_dmic_probe,
	.suspend = sunxi_hifi_dmic_suspend,
	.resume = sunxi_hifi_dmic_resume,
	.remove = sunxi_hifi_dmic_remove,
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DMIC_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_hifi_dmic_dai_ops,
};

static const struct snd_soc_component_driver sunxi_hifi_dmic_component = {
	.name = DRV_NAME,
};

static struct regmap_config sunxi_dmic_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DMIC_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static void trigger_work_capture_func(struct work_struct *work)
{
	struct sunxi_dmic_info *sunxi_dmic =
			container_of(work, struct sunxi_dmic_info, trigger_work_capture);
	struct msg_substream_package *msg_substream = &sunxi_dmic->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;
	struct snd_pcm_substream *substream = msg_substream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int pos = 0;
	int ret = 0;

	for (;;) {
		mutex_lock(&sunxi_dmic->rpmsg_mutex_capture);
		if (soc_substream->cmd_val == SND_SOC_DSP_PCM_READI) {
			snd_pcm_stream_lock_irq(substream);
			if (runtime->status->state != SNDRV_PCM_STATE_RUNNING) {
				dev_err(sunxi_dmic->cpu_dai->dev, "%s state:%d\n",
						__func__, runtime->status->state);
				snd_pcm_stream_unlock_irq(substream);
				mutex_unlock(&sunxi_dmic->rpmsg_mutex_capture);
				break;
			}
			sunxi_dmic->capturing = 1;
			/* update the capture soc_substream output addr */
			pos = snd_dmaengine_pcm_pointer_no_residue(substream);
			soc_substream->output_addr = runtime->dma_addr + frames_to_bytes(runtime, pos);
			snd_pcm_stream_unlock_irq(substream);

			dev_dbg(sunxi_dmic->cpu_dai->dev,
				"[%s] dma_addr:0x%llx, pos:%u, output_addr:0x%x\n",
				 __func__, runtime->dma_addr, pos, soc_substream->output_addr);
			/* send to dsp */
			ret = sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_dmic->cpu_dai,
				SNDRV_PCM_STREAM_CAPTURE, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			mutex_unlock(&sunxi_dmic->rpmsg_mutex_capture);

			/* 拷贝并发送通知component */
			snd_soc_rpaf_pcm_update_stream_process(&sunxi_dmic->dsp_capcomp);

			if (ret != 0) {
				dev_err(sunxi_dmic->cpu_dai->dev,
					"%s state:%d, ret=%d\n", __func__, runtime->status->state, ret);
			}
			if (substream->runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
				queue_work(sunxi_dmic->wq_capture, &sunxi_dmic->trigger_work_capture);
			}
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_STOP) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_dmic_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_dmic->cpu_dai,
				SNDRV_PCM_STREAM_CAPTURE, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			sunxi_dmic->capturing = 0;
			mutex_unlock(&sunxi_dmic->rpmsg_mutex_capture);
			break;
		}
		mutex_unlock(&sunxi_dmic->rpmsg_mutex_capture);
	}
}

static int sunxi_hifi_dmic_dev_probe(struct platform_device *pdev)
{
	struct sunxi_dmic_info *sunxi_dmic = NULL;
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

	dev_info(&pdev->dev, "%s start.\n", __func__);

	sunxi_dmic = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_dmic_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(&pdev->dev, "sunxi_dmic allocate failed\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, sunxi_dmic);

	sunxi_dmic->dev = &pdev->dev;
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &dts_info->mem_info;
	sunxi_dmic->dai_drv = sunxi_hifi_dmic_dai;
	sunxi_dmic->dai_drv.name = dev_name(&pdev->dev);

	sunxi_dmic->capture_en = 0;

	ret = of_address_to_resource(np, 0, &(mem_info->res));
	if (ret) {
		dev_err(&pdev->dev, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
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

	sunxi_dmic_regmap_config.max_register = resource_size(&mem_info->res);
	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev,
					mem_info->membase,
					&sunxi_dmic_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}
	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "capture cma kbytes config missing or invalid.\n");
		dts_info->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		dts_info->capture_cma = temp_val;
	}

	sunxi_dmic->capture_dma_param.cma_kbytes = dts_info->capture_cma;
	sunxi_dmic->capture_dma_param.fifo_size = DMIC_RX_FIFO_SIZE;

	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; */
	ret = of_property_read_u32(np, "dsp_card", &temp_val);
	if (ret < 0)
		dts_info->dsp_card = 1;
	else
		dts_info->dsp_card = temp_val;

	/* default is 0, for reserved */
	ret = of_property_read_u32(np, "dsp_device", &temp_val);
	if (ret < 0)
		dts_info->dsp_device = 0;
	else
		dts_info->dsp_device = temp_val;

	mutex_init(&sunxi_dmic->rpmsg_mutex_capture);

	snprintf(sunxi_dmic->wq_capture_name, sizeof(sunxi_dmic->wq_capture_name),
			"hifi-dmic-capture");
	sunxi_dmic->wq_capture = create_workqueue(sunxi_dmic->wq_capture_name);
	if (IS_ERR_OR_NULL(sunxi_dmic->wq_capture)) {
		ret = -ENOMEM;
		goto err_create_wq_capture;
	}
	INIT_WORK(&sunxi_dmic->trigger_work_capture, trigger_work_capture_func);
//	queue_work(sunxi_dmic->wq_capture, &sunxi_dmic->trigger_work_capture);

	/*
	 * Register a component with automatic unregistration when the device is
	 * unregistered.
	 */
	ret = devm_snd_soc_register_component(&pdev->dev, &sunxi_hifi_dmic_component,
					&sunxi_dmic->dai_drv, 1);
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err_devm_register_component;
	}

	/*
	 * Register a platform driver with automatic unregistration when the device is
	 * unregistered.
	 */
	ret = asoc_hifi_platform_register(&pdev->dev, SUNXI_HIFI_PCM_OPS_DMIC);
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		ret = -ENOMEM;
		goto err_platform_register;
	}

#ifdef CONFIG_SND_SUNXI_HIFI_DMIC_DEBUG
	ret  = sysfs_create_group(&pdev->dev.kobj, &dmic_debug_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "failed to create attr group\n");
		goto err_sysfs_create_debug;
	}
#endif

	dev_info(&pdev->dev, "%s stop.\n", __func__);

	return 0;

#ifdef CONFIG_SND_SUNXI_HIFI_DMIC_DEBUG
err_sysfs_create_debug:
#endif
err_platform_register:
err_devm_register_component:
err_create_wq_capture:
	mutex_destroy(&sunxi_dmic->rpmsg_mutex_capture);
err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	devm_kfree(&pdev->dev, sunxi_dmic);
	return ret;
}

static int __exit sunxi_hifi_dmic_dev_remove(struct platform_device *pdev)
{
	struct sunxi_dmic_info *sunxi_dmic = NULL;
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;

	sunxi_dmic = platform_get_drvdata(pdev);
	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(&pdev->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	dev_info(&pdev->dev, "%s start.\n", __func__);

	dts_info = &sunxi_dmic->dts_info;
	mem_info = &dts_info->mem_info;

#ifdef CONFIG_SND_SUNXI_HIFI_DMIC_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &dmic_debug_attr_group);
#endif

	cancel_work_sync(&(sunxi_dmic->trigger_work_capture));
	destroy_workqueue(sunxi_dmic->wq_capture);

	mutex_destroy(&sunxi_dmic->rpmsg_mutex_capture);

	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
	devm_kfree(&pdev->dev, sunxi_dmic);

	platform_set_drvdata(pdev, NULL);

	dev_info(&pdev->dev, "%s stop.\n", __func__);
	return 0;
}

static const struct of_device_id sunxi_hifi_dmic_of_match[] = {
	{ .compatible = "allwinner,sunxi-dmic", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_hifi_dmic_of_match);

static struct platform_driver sunxi_hifi_dmic_driver = {
	.probe = sunxi_hifi_dmic_dev_probe,
	.remove = __exit_p(sunxi_hifi_dmic_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_hifi_dmic_of_match,
	},
};
module_platform_driver(sunxi_hifi_dmic_driver);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DMIC Machine ASoC HiFi driver");
MODULE_ALIAS("platform:sunxi-dmic");
MODULE_LICENSE("GPL");
