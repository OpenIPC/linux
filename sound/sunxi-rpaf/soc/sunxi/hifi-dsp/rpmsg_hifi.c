/*
 * Remote processor messaging - hifi0 client driver
 *
 * Copyright (C) 2019-2025 Allwinnertech, Inc.
 *
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <sound/aw_rpaf/common.h>
#include <sound/aw_rpaf/mixer.h>
#include <sound/aw_rpaf/debug.h>
#include <sound/aw_rpaf/substream.h>
#include <sound/soc.h>
#include <sound/soc-topology.h>
#include <sound/pcm.h>
#include <sound/soc-dai.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>

#include "sunxi-daudio.h"
#include "sunxi-dmic.h"
#include "sunxi-cpudai.h"
#include <sound/aw_rpaf/component-core.h>
#include <sound/aw_rpaf/rpmsg_hifi.h>

#define RPMSG_TIMEOUT_NORMAL 	(3000)

struct audio_driver_info {
	char driver[32];
	void *data;
};

/* Should be Register the data at first before Send msg to HiFi dsp */
static struct audio_driver_info audio_drv_info[DSP_SOUND_CARDS];

static struct rpmsg_hifi_priv *gHifi_priv[2];

int sunxi_hifi_register_sound_drv_info(const char *name, void *data)
{
	int i = 0;

	for (i = 0; i < DSP_SOUND_CARDS; i++) {
		if (IS_ERR_OR_NULL(audio_drv_info[i].data)) {
			strncpy(audio_drv_info[i].driver, name, 31);
			audio_drv_info[i].data = data;
			return 0;
		}
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(sunxi_hifi_register_sound_drv_info);

int sunxi_hifi_unregister_sound_drv_info(const char *name, void *data)
{
	int i = 0;

	for (i = 0; i < DSP_SOUND_CARDS; i++) {
		if (!strncmp(audio_drv_info[i].driver, name, 31) &&
			(audio_drv_info[i].data == data)) {
			audio_drv_info[i].data = NULL;
			memset(audio_drv_info[i].driver, 0, 32);
			return 0;
		}
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(sunxi_hifi_unregister_sound_drv_info);

static void *sunxi_hifi_find_sound_drv_info_by_name(const char *name)
{
	int i = 0;

	if (IS_ERR_OR_NULL(name))
		return NULL;

	for (i = 0; i < DSP_SOUND_CARDS; i++) {
		if (!strncmp(name, audio_drv_info[i].driver, 31)) {
			return audio_drv_info[i].data;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(sunxi_hifi_find_sound_drv_info_by_name);

static LIST_HEAD(list_msg_component);
static DEFINE_MUTEX(mutex_msg_component);

/* find by name and stream */
struct msg_component_package *sunxi_hifi_list_msg_component_find_item(
	struct msg_component_package *msg_component)
{
	struct msg_component_package *c;
	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_pcm_params *pcm_params;

	if (!msg_component)
		return NULL;

	soc_component = &msg_component->soc_component;
	pcm_params = &soc_component->params;

	mutex_lock(&mutex_msg_component);
	if (list_empty(&list_msg_component)) {
		mutex_unlock(&mutex_msg_component);
		return NULL;
	}

	list_for_each_entry(c, &list_msg_component, list) {
		struct snd_soc_dsp_component *soc_component = &c->soc_component;
		struct snd_soc_dsp_pcm_params *params = &soc_component->params;
		if ((params->stream == pcm_params->stream) &&
			(!strcmp(params->driver, pcm_params->driver))) {
			mutex_unlock(&mutex_msg_component);
			return c;
		}
	}
	mutex_unlock(&mutex_msg_component);

	return NULL;
}
EXPORT_SYMBOL(sunxi_hifi_list_msg_component_find_item);

int sunxi_hifi_list_msg_component_add_tail(struct msg_component_package *msg_component)
{
	if (!msg_component)
		return -EINVAL;

	mutex_lock(&mutex_msg_component);
	list_add_tail(&msg_component->list, &list_msg_component);
	mutex_unlock(&mutex_msg_component);

	return 0;
}
EXPORT_SYMBOL(sunxi_hifi_list_msg_component_add_tail);

int sunxi_hifi_list_msg_component_remove_item(struct msg_component_package *msg_component)
{
	struct msg_component_package *c;
	struct msg_component_package *temp;

	if (!msg_component)
		return -EINVAL;

	mutex_lock(&mutex_msg_component);
	if (list_empty(&list_msg_component)) {
		mutex_unlock(&mutex_msg_component);
		return 0;
	}
	list_for_each_entry_safe(c, temp, &list_msg_component, list) {
		if (c == msg_component) {
			list_del(&msg_component->list);
			mutex_unlock(&mutex_msg_component);
			return 0;
		}
	}
	mutex_unlock(&mutex_msg_component);

	return 0;
}
EXPORT_SYMBOL(sunxi_hifi_list_msg_component_remove_item);

int sunxi_hifi_nonblock_send(unsigned int hifi_id,
			struct msg_audio_package *msg_package)
{
	struct rpmsg_device *rpmsg_dev;
	int ret = 0;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL))
		return -EINVAL;

	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;

	ret = rpmsg_send(rpmsg_dev->ept, msg_package,
				sizeof(struct msg_audio_package));
	if (ret)
		dev_err(&rpmsg_dev->dev, "rpmsg_send failed: %d\n", ret);
	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_nonblock_send);

int sunxi_hifi_component_nonblock_send(unsigned int hifi_id,
				struct msg_component_package *msg_component)
{
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct msg_audio_package msg_package;
	struct rpmsg_device *rpmsg_dev;
	int ret = 0;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL))
		return -EINVAL;

	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;

	msg_package.audioMsgVal = MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND;
	msg_package.sharePointer = (unsigned int)__pa(soc_component);
	__dma_flush_range(soc_component, sizeof(struct snd_soc_dsp_component));

	ret = sunxi_hifi_nonblock_send(hifi_id, &msg_package);

	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_component_nonblock_send);

/* BLOCK调用不能在中断中 */
int sunxi_hifi_component_block_send(unsigned int hifi_id,
			struct msg_component_package *msg_component)
{
	wait_queue_t wait;
	long tout;
	struct rpmsg_device *rpmsg_dev;
	int ret = 0;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL))
		return -EINVAL;

	spin_lock_irq(&msg_component->lock);
	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;
	if (msg_component->wakeup_flag) {
		dev_warn(&rpmsg_dev->dev, "%s wakeup_flag was not reset. clear it\n", __func__);
		/* reset */
		msg_component->wakeup_flag = 0;
	}
	/* 预备休眠等待 */
	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_KILLABLE);
	add_wait_queue(&msg_component->tsleep, &wait);

	/* 发送数据 */
	spin_unlock_irq(&msg_component->lock);
	ret = sunxi_hifi_component_nonblock_send(hifi_id, msg_component);
	spin_lock_irq(&msg_component->lock);
	if (ret) {
		dev_err(&rpmsg_dev->dev, "%s rpmsg_send failed: %d\n", __func__, ret);
		ret = -EIO;
		goto err_rpmsg_send;
	}

	for (;;) {
		if (fatal_signal_pending(current)) {
			dev_err(&rpmsg_dev->dev, "%s %d. catch signal\n", __func__, __LINE__);
			ret = -ERESTARTSYS;
			break;
		}

		if (msg_component->wakeup_flag == 1) {
			msg_component->wakeup_flag = 0;
			break;
		}
		#if 0
		pr_info("[%s] line:%d before wakeup_flag=%d\n", __func__, __LINE__, msg_component->wakeup_flag);
		mdelay(1000);
		pr_info("[%s] line:%d after wakeup_flag=%d\n", __func__, __LINE__, msg_component->wakeup_flag);
		#endif
		set_current_state(TASK_KILLABLE);
		spin_unlock_irq(&msg_component->lock);
		tout = schedule_timeout(msecs_to_jiffies(RPMSG_TIMEOUT_NORMAL));
		spin_lock_irq(&msg_component->lock);
		if (!tout) {
			dev_err(&rpmsg_dev->dev,
				"line:%d rpmsg_send timeout %d, flag=%d\n",
				__LINE__, ret, msg_component->wakeup_flag);
			ret = -EIO;
			break;
		} else {
			if (msg_component->wakeup_flag == 0)
				continue;
			else if (msg_component->wakeup_flag == 1) {
				msg_component->wakeup_flag = 0;
				break;
			}
		}
	}
	/* 换醒或者超时后 */
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&msg_component->tsleep, &wait);
	spin_unlock_irq(&msg_component->lock);

	return ret;

err_rpmsg_send:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&msg_component->tsleep, &wait);
	spin_unlock_irq(&msg_component->lock);
	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_component_block_send);

int sunxi_mixer_block_send(unsigned int hifi_id, struct msg_mixer_package *msg_mixer)
{
	struct msg_audio_package msg_audio_package;
	struct rpmsg_device *rpmsg_dev;
	int *wakeup_flag;
	int ret = 0;
	wait_queue_t wait;
	wait_queue_head_t *cpudai_tsleep;
	long tout;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL)) {
		pr_err("%s params invalid.\n", __func__);
		return -EINVAL;
	}

	spin_lock_irq(&msg_mixer->lock);
	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;

	msg_audio_package.audioMsgVal = MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND;
	cpudai_tsleep = &msg_mixer->tsleep;
	wakeup_flag = &msg_mixer->wakeup_flag;
	msg_audio_package.sharePointer =
			(unsigned int)__pa(&msg_mixer->soc_mixer);
	__dma_flush_range(&msg_mixer->soc_mixer,
					sizeof(struct snd_soc_dsp_mixer));
	if (*wakeup_flag) {
		dev_warn(&rpmsg_dev->dev, "%s wakeup_flag was not reset.\n", __func__);
		/* reset */
		*wakeup_flag = 0;
	}
	/* 预备休眠等待 */
	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_KILLABLE);
	add_wait_queue(cpudai_tsleep, &wait);
	/* 发送数据 */
	spin_unlock_irq(&msg_mixer->lock);
	ret = rpmsg_send(rpmsg_dev->ept, &msg_audio_package,
				  sizeof(struct msg_audio_package));
	spin_lock_irq(&msg_mixer->lock);
	if (ret) {
		dev_err(&rpmsg_dev->dev, "rpmsg_send failed: %d\n", ret);
		ret = -EIO;
		goto err_rmpsg_send;
	}

	for (;;) {
		if (fatal_signal_pending(current)) {
			dev_err(&rpmsg_dev->dev, "%s %d.\n", __func__, __LINE__);
			ret = -ERESTARTSYS;
			break;
		}

		if (*wakeup_flag == 1) {
			*wakeup_flag = 0;
			break;
		}
		/* 等待唤醒,超时等待1s */
		set_current_state(TASK_KILLABLE);
		spin_unlock_irq(&msg_mixer->lock);
		tout = schedule_timeout(msecs_to_jiffies(RPMSG_TIMEOUT_NORMAL));
		spin_lock_irq(&msg_mixer->lock);
		if (!tout) {
			dev_err(&rpmsg_dev->dev,
				"line:%d rpmsg_send timeout %d, flag=%d\n",
				__LINE__, ret, *wakeup_flag);
			ret = -EIO;
			break;
		} else {
			if (*wakeup_flag == 0) {
				continue;
			} else if (*wakeup_flag == 1) {
				*wakeup_flag = 0;
				break;
			}
		}
	}

	/* 换醒或者超时后 */
	set_current_state(TASK_RUNNING);
	remove_wait_queue(cpudai_tsleep, &wait);
	spin_unlock_irq(&msg_mixer->lock);
	return ret;

err_rmpsg_send:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(cpudai_tsleep, &wait);
	spin_unlock_irq(&msg_mixer->lock);
	return ret;
}
EXPORT_SYMBOL(sunxi_mixer_block_send);

static int sunxi_hifi_substream_component_block_send(unsigned int hifi_id,
				struct msg_component_package *msg_component)
{
	struct msg_audio_package msg_package;
	wait_queue_t wait;
	long tout;
	struct rpmsg_device *rpmsg_dev;
	int ret = 0;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL))
		return -EINVAL;

	spin_lock_irq(&msg_component->lock);
	msg_component->soc_component.comp_mode = SND_DSP_COMPONENT_MODE_STREAM;
	msg_component->soc_component.cmd_val = SND_SOC_DSP_COMPONENT_SET_STREAM_PARAMS;

	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;
	if (msg_component->wakeup_flag) {
		dev_warn(&rpmsg_dev->dev, "%s wakeup_flag was not reset.\n", __func__);
		/* reset */
		msg_component->wakeup_flag = 0;
	}

	/* 预备休眠等待 */
	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_KILLABLE);
	add_wait_queue(&msg_component->tsleep, &wait);

	/* 发送数据 */
	msg_package.audioMsgVal = MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND;
	msg_package.sharePointer = (unsigned int)__pa(&msg_component->soc_component);
	__dma_flush_range(&msg_component->soc_component, sizeof(struct snd_soc_dsp_component));

	spin_unlock_irq(&msg_component->lock);
	ret = sunxi_hifi_nonblock_send(hifi_id, &msg_package);
	spin_lock_irq(&msg_component->lock);
	if (ret) {
		dev_err(&rpmsg_dev->dev, "%s rpmsg_send failed: %d\n", __func__, ret);
		ret = -EIO;
		goto err_rpmsg_send;
	}

	for (;;) {
		if (fatal_signal_pending(current)) {
			dev_err(&rpmsg_dev->dev, "%s %d.\n", __func__, __LINE__);
			ret = -ERESTARTSYS;
			break;
		}

		if (msg_component->wakeup_flag == 1) {
			msg_component->wakeup_flag = 0;
			break;
		}
		set_current_state(TASK_KILLABLE);
		spin_unlock_irq(&msg_component->lock);
		tout = schedule_timeout(msecs_to_jiffies(RPMSG_TIMEOUT_NORMAL));
		spin_lock_irq(&msg_component->lock);
		if (!tout) {
			dev_err(&rpmsg_dev->dev,
				"line:%d rpmsg_send timeout %d, flag=%d\n",
				__LINE__, ret, msg_component->wakeup_flag);
			ret = -EIO;
			break;
		} else {
			if (msg_component->wakeup_flag == 0)
				continue;
			else if (msg_component->wakeup_flag == 1) {
				msg_component->wakeup_flag = 0;
				break;
			}
		}
	}
	/* 换醒或者超时后 */
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&msg_component->tsleep, &wait);
	spin_unlock_irq(&msg_component->lock);

	return ret;

err_rpmsg_send:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&msg_component->tsleep, &wait);
	spin_unlock_irq(&msg_component->lock);
	return ret;
}

int32_t sunxi_hifi_substream_set_stream_component(unsigned int hifi_id,
		struct snd_soc_dai *dai,
		struct snd_soc_dsp_substream *soc_substream,
		struct snd_dsp_component *dsp_component)
{
	struct snd_soc_dsp_component *soc_component = NULL;
	struct msg_component_package *msg_component = NULL;
	struct snd_soc_dsp_pcm_params *pcm_params = NULL;
	int i = 0;
	int ret = 0;

	if (!dsp_component)
		return -EFAULT;

	msg_component = &dsp_component->msg_component;
	init_waitqueue_head(&msg_component->tsleep);
	soc_component = &msg_component->soc_component;
	memcpy(&soc_component->params, &soc_substream->params,
		sizeof(struct snd_soc_dsp_pcm_params));
	pcm_params = &soc_component->params;

	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++)
		soc_component->component_sort[i] = -1;

	soc_component->read_size = pcm_params->channels *
		snd_pcm_format_size(pcm_params->format, pcm_params->pcm_frames);
	soc_component->dump_size = soc_component->read_size;
	if (soc_component->read_size == 0) {
		dev_err(dai->dev, "pcm_frames is 0.\n");
		return -EFAULT;
	}

	if (!soc_component->read_addr) {
		dma_addr_t read_addr;
		dsp_component->read_area = dma_alloc_coherent(dai->dev,
							soc_component->read_size,
							&read_addr, GFP_KERNEL);
		if (IS_ERR_OR_NULL(dsp_component->read_area)) {
			ret = -ENOMEM;
			goto err_malloc_read_addr;
		}
		soc_component->read_addr = read_addr;
	}

	for (i = 0; i < ARRAY_SIZE(soc_component->dump_addr); i++) {
		dma_addr_t dump_addr;
		if (soc_component->dump_addr[i])
			continue;
		if ((soc_component->component_type >> i) & 0x1) {
			awrpaf_debug("component_type:0x%x, i:%d\n",
				soc_component->component_type, i);
			dsp_component->dump_area[i] = dma_alloc_coherent(dai->dev,
							soc_component->dump_size,
							&dump_addr, GFP_KERNEL);
			if (IS_ERR_OR_NULL(dsp_component->dump_area[i])) {
				ret = -ENOMEM;
				goto err_malloc_dump_addr;
			}
			soc_component->dump_addr[i] = dump_addr;
		}
	}

	sunxi_hifi_list_msg_component_add_tail(msg_component);

	return sunxi_hifi_substream_component_block_send(hifi_id, msg_component);

err_malloc_dump_addr:
	dma_free_coherent(dai->dev, soc_component->read_size,
				dsp_component->read_area, soc_component->read_addr);
	soc_component->read_addr = 0;
	dsp_component->read_area = NULL;
	for (i = 0; i < ARRAY_SIZE(soc_component->dump_addr); i++) {
		if (!soc_component->dump_addr[i])
			continue;
		dma_free_coherent(dai->dev, soc_component->dump_size,
				dsp_component->dump_area[i], soc_component->dump_addr[i]);
		soc_component->dump_addr[i] = 0;
		dsp_component->dump_area[i] = NULL;
	}
err_malloc_read_addr:
	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_substream_set_stream_component);

int32_t snd_soc_rpaf_pcm_update_stream_process(struct snd_dsp_component *stream_dsp_component)
{
	snd_pcm_uframes_t pos = 0;
	struct snd_dsp_component *dsp_component = NULL;
	struct snd_soc_rpaf_pcm_runtime *runtime = NULL;
	struct msg_component_package *msg_component = NULL;
	struct snd_soc_dsp_component *soc_component = NULL;
	ssize_t dump_bytes;
	void *buffer = NULL;

	snd_soc_rpaf_pcm_stream_component_lock();

	dsp_component = (struct snd_dsp_component *)stream_dsp_component->private_data;
	if (dsp_component) {

		snd_soc_rpaf_pcm_stream_lock_irq(dsp_component);

		runtime = &dsp_component->runtime;
		if (!runtime->dump_start) {

			snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);
			snd_soc_rpaf_pcm_stream_component_unlock();

			return 0;
		}

		msg_component = &dsp_component->msg_component;
		soc_component = &msg_component->soc_component;
		pos = snd_soc_rpaf_pcm_bytes_to_frames(runtime, runtime->pos);
		buffer = dsp_component->read_area + runtime->pos;

		snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);

		dump_bytes = snd_soc_rpaf_pcm_frames_to_bytes(runtime, runtime->period_size);

		if (stream_dsp_component->dump_area[runtime->dump_type]) {
			memcpy(buffer, stream_dsp_component->dump_area[runtime->dump_type],
				dump_bytes);
		}
		snd_soc_rpaf_pcm_stream_update_complete(dsp_component);
	}

	snd_soc_rpaf_pcm_stream_component_unlock();

	return 0;
}
EXPORT_SYMBOL(snd_soc_rpaf_pcm_update_stream_process);

int32_t sunxi_hifi_substream_release_stream_component(struct snd_soc_dai *dai,
		struct snd_dsp_component *dsp_component)
{
	struct msg_component_package *msg_component = NULL;
	struct snd_soc_dsp_component *soc_component = NULL;
	struct snd_dsp_component *component = NULL;
	int i = 0;

	if (!dsp_component)
		return -EFAULT;

	component = (struct snd_dsp_component *)dsp_component->private_data;
	if (component) {
		dev_info(dai->dev, "clear component.\n");
		/* 相当于通知到该流退出来了 */
		component->private_data = NULL;
		snd_soc_rpaf_pcm_stream_update_complete(component);
	}

	msg_component = &dsp_component->msg_component;
	soc_component = &msg_component->soc_component;

	sunxi_hifi_list_msg_component_remove_item(msg_component);

	dma_free_coherent(dai->dev, soc_component->read_size,
				dsp_component->read_area, soc_component->read_addr);
	soc_component->read_addr = 0;
	dsp_component->read_area = NULL;

	for (i = 0; i < ARRAY_SIZE(soc_component->dump_addr); i++) {
		if (!soc_component->dump_addr[i])
			continue;
		dma_free_coherent(dai->dev, soc_component->dump_size,
				dsp_component->dump_area[i],
				soc_component->dump_addr[i]);
		soc_component->dump_addr[i] = 0;
		dsp_component->dump_area[i] = NULL;
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_hifi_substream_release_stream_component);

static void sunxi_pcm_transfer_complete(struct snd_pcm_substream *substream)
{
	struct dmaengine_pcm_runtime_data *prtd = substream->runtime->private_data;
	unsigned long flags;

	snd_pcm_stream_lock_irqsave(substream, flags);
	if (!substream->runtime) {
		snd_pcm_stream_unlock_irqrestore(substream, flags);
		return;
	}

	prtd->pos += snd_pcm_lib_period_bytes(substream);
	if (prtd->pos >= snd_pcm_lib_buffer_bytes(substream))
		prtd->pos = 0;
	snd_pcm_stream_unlock_irqrestore(substream, flags);

	snd_pcm_period_elapsed(substream);
}

int sunxi_hifi_cpudai_substream_block_send(unsigned int hifi_id,
			struct snd_pcm_substream *substream, struct snd_soc_dai *dai,
			int stream, unsigned long msg_cmd)
{
	struct snd_soc_dai *cpu_dai = NULL;
	struct snd_soc_pcm_runtime *rtd = NULL;
	struct sunxi_cpudai_info *sunxi_cpudai = NULL;
	struct msg_audio_package msg_audio_package;
	struct rpmsg_device *rpmsg_dev;
	struct snd_soc_dsp_substream *soc_substream;
	int *wakeup_flag;
	int ret = 0;
	wait_queue_t wait;
	wait_queue_head_t *cpudai_tsleep;
	spinlock_t *lock;
	long tout;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL) ||
		(IS_ERR_OR_NULL(substream) && IS_ERR_OR_NULL(dai))) {
		pr_err("%s params invalid.\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(dai)) {
		rtd = substream->private_data;
		cpu_dai = rtd->cpu_dai;
		sunxi_cpudai = snd_soc_dai_get_drvdata(cpu_dai);
	} else {
		cpu_dai = dai;
		sunxi_cpudai = snd_soc_dai_get_drvdata(cpu_dai);
	}

	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;

	msg_audio_package.audioMsgVal = msg_cmd;
	switch (msg_cmd) {
	case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
	{
		struct msg_substream_package *msg_substream;

		/* for substream == NULL */
		if (IS_ERR_OR_NULL(substream)) {
			if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
				msg_substream = &sunxi_cpudai->msg_playback;
			} else {
				msg_substream = &sunxi_cpudai->msg_capture;
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_cpudai->msg_playback;
		} else {
			msg_substream = &sunxi_cpudai->msg_capture;
		}
		cpudai_tsleep = &msg_substream->tsleep;
		lock = &msg_substream->lock;
		wakeup_flag = &msg_substream->wakeup_flag;
		soc_substream = &msg_substream->soc_substream;

		pr_debug("[%s] line:%d  cmd_val=0x%x\n", __func__, __LINE__, soc_substream->cmd_val);
		msg_audio_package.sharePointer = (unsigned int)__pa(soc_substream);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
	{
		struct msg_debug_package *msg_debug;

		msg_debug = &sunxi_cpudai->msg_debug;
		cpudai_tsleep = &msg_debug->tsleep;
		lock = &msg_debug->lock;
		wakeup_flag = &msg_debug->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_debug->soc_debug);
		__dma_flush_range(&msg_debug->soc_debug,
						sizeof(struct snd_soc_dsp_debug));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
	{
		struct msg_component_package *msg_component;
		struct snd_dsp_component *dsp_component;

		/* for substream == NULL */
		if (IS_ERR_OR_NULL(substream)) {
			if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
				dsp_component = &sunxi_cpudai->dsp_playcomp;
			} else {
				dsp_component = &sunxi_cpudai->dsp_capcomp;
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dsp_component = &sunxi_cpudai->dsp_playcomp;
		} else {
			dsp_component = &sunxi_cpudai->dsp_capcomp;
		}
		msg_component = &dsp_component->msg_component;
		cpudai_tsleep = &msg_component->tsleep;
		lock = &msg_component->lock;
		wakeup_flag = &msg_component->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_component->soc_component);
		__dma_flush_range(&msg_component->soc_component,
						sizeof(struct snd_soc_dsp_component));
	}
	break;
	default:
		dev_err(&rpmsg_dev->dev, "msg_cmd error:%lu\n", msg_cmd);
		return -EINVAL;
	}

	spin_lock_irq(lock);
	if (*wakeup_flag) {
		dev_warn(&rpmsg_dev->dev, "%s wakeup_flag was not reset.\n", __func__);
		/* reset */
		*wakeup_flag = 0;
	}

//	dev_err(&rpmsg_dev->dev, "audioMsgVal:%d, sharePointer:0x%x\n",
//		msg_audio_package.audioMsgVal, msg_audio_package.sharePointer);
	/* 预备休眠等待 */
	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_KILLABLE);
	add_wait_queue(cpudai_tsleep, &wait);
	/* 发送数据 */
	spin_unlock_irq(lock);
	ret = rpmsg_send(rpmsg_dev->ept, &msg_audio_package,
				sizeof(struct msg_audio_package));
	spin_lock_irq(lock);
	if (ret) {
		dev_err(&rpmsg_dev->dev, "line:%d rpmsg_send failed: %d\n", __LINE__, ret);
		ret = -EIO;
		goto err_rmpsg_send;
	}

	for (;;) {
		if (fatal_signal_pending(current)) {
			dev_err(&rpmsg_dev->dev, "%s %d.\n", __func__, __LINE__);
			ret = -ERESTARTSYS;
			break;
		}

		if (*wakeup_flag == 1) {
			*wakeup_flag = 0;
			break;
		}
		set_current_state(TASK_KILLABLE);
		spin_unlock_irq(lock);
		tout = schedule_timeout(msecs_to_jiffies(RPMSG_TIMEOUT_NORMAL));
		spin_lock_irq(lock);
		if (!tout) {
			dev_err(&rpmsg_dev->dev,
				"line:%d rpmsg_send timeout %d, flag=%d\n",
				__LINE__, ret, *wakeup_flag);
			pr_info("msg_cmd=%lu\n", msg_cmd);
			if (msg_cmd == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) {
				pr_info("stream:%d, cmd_val=0x%x\n", stream, soc_substream->cmd_val);
			}
			ret = -EIO;
			break;
		} else {
			if (*wakeup_flag == 0) {
				continue;
			} else if (*wakeup_flag == 1) {
				*wakeup_flag = 0;
				break;
			}
		}
	}

	/* 换醒或者超时后 */
	set_current_state(TASK_RUNNING);
	remove_wait_queue(cpudai_tsleep, &wait);
	spin_unlock_irq(lock);
	return ret;

err_rmpsg_send:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(cpudai_tsleep, &wait);
	spin_unlock_irq(lock);
	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_cpudai_substream_block_send);

int sunxi_hifi_daudio_substream_block_send(unsigned int hifi_id,
			struct snd_pcm_substream *substream, struct snd_soc_dai *dai,
			int stream, unsigned long msg_cmd)
{
	struct snd_soc_dai *cpu_dai = NULL;
	struct snd_soc_pcm_runtime *rtd = NULL;
	struct sunxi_daudio_info *sunxi_daudio = NULL;
	struct msg_audio_package msg_audio_package;
	struct rpmsg_device *rpmsg_dev;
	struct snd_soc_dsp_substream *soc_substream;
	int *wakeup_flag;
	int ret = 0;
	wait_queue_t wait;
	wait_queue_head_t *daudio_tsleep;
	spinlock_t *lock;
	long tout;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL) ||
		(IS_ERR_OR_NULL(substream) && IS_ERR_OR_NULL(dai))) {
		pr_err("%s params invalid.\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(dai)) {
		rtd = substream->private_data;
		cpu_dai = rtd->cpu_dai;
		sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);
	} else {
		cpu_dai = dai;
		sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);
	}

	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;

	msg_audio_package.audioMsgVal = msg_cmd;
	switch (msg_cmd) {
	case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
	{
		struct msg_substream_package *msg_substream;

		/* for substream == NULL */
		if (IS_ERR_OR_NULL(substream)) {
			if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
				msg_substream = &sunxi_daudio->msg_playback;
			} else {
				msg_substream = &sunxi_daudio->msg_capture;
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_daudio->msg_playback;
		} else {
			msg_substream = &sunxi_daudio->msg_capture;
		}
		daudio_tsleep = &msg_substream->tsleep;
		lock = &msg_substream->lock;
		wakeup_flag = &msg_substream->wakeup_flag;
		soc_substream = &msg_substream->soc_substream;

		pr_debug("[%s] line:%d  cmd_val=0x%x\n", __func__, __LINE__, soc_substream->cmd_val);
		msg_audio_package.sharePointer = (unsigned int)__pa(soc_substream);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
	{
		struct msg_mixer_package *msg_mixer;

		msg_mixer = &sunxi_daudio->msg_mixer;
		daudio_tsleep = &msg_mixer->tsleep;
		lock = &msg_mixer->lock;
		wakeup_flag = &msg_mixer->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_mixer->soc_mixer);
		__dma_flush_range(&msg_mixer->soc_mixer,
						sizeof(struct snd_soc_dsp_mixer));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
	{
		struct msg_debug_package *msg_debug;

		msg_debug = &sunxi_daudio->msg_debug;
		daudio_tsleep = &msg_debug->tsleep;
		lock = &msg_debug->lock;
		wakeup_flag = &msg_debug->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_debug->soc_debug);
		__dma_flush_range(&msg_debug->soc_debug,
						sizeof(struct snd_soc_dsp_debug));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
	{
		struct msg_component_package *msg_component;
		struct snd_dsp_component *dsp_component;

		/* for substream == NULL */
		if (IS_ERR_OR_NULL(substream)) {
			if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
				dsp_component = &sunxi_daudio->dsp_playcomp;
			} else {
				dsp_component = &sunxi_daudio->dsp_capcomp;
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dsp_component = &sunxi_daudio->dsp_playcomp;
		} else {
			dsp_component = &sunxi_daudio->dsp_capcomp;
		}
		msg_component = &dsp_component->msg_component;
		daudio_tsleep = &msg_component->tsleep;
		lock = &msg_component->lock;
		wakeup_flag = &msg_component->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_component->soc_component);
		__dma_flush_range(&msg_component->soc_component,
						sizeof(struct snd_soc_dsp_component));
	}
	break;
	default:
		dev_err(&rpmsg_dev->dev, "msg_cmd error:%lu\n", msg_cmd);
		return -EINVAL;
	}
	spin_lock_irq(lock);
	if (*wakeup_flag) {
		dev_warn(&rpmsg_dev->dev, "%s wakeup_flag was not reset.\n", __func__);
		/* reset */
		*wakeup_flag = 0;
	}
	/* 预备休眠等待 */
	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_KILLABLE);
	add_wait_queue(daudio_tsleep, &wait);
	/* 发送数据 */
	spin_unlock_irq(lock);
	ret = rpmsg_send(rpmsg_dev->ept, &msg_audio_package,
				  sizeof(struct msg_audio_package));
	spin_lock_irq(lock);
	if (ret) {
		dev_err(&rpmsg_dev->dev, "line:%d rpmsg_send failed: %d\n", __LINE__, ret);
		ret = -EIO;
		goto err_rmpsg_send;
	}

//	dev_err(&rpmsg_dev->dev, "%s %d audioMsgVal:%d, sharePointer:0x%x\n",
//				__func__, __LINE__, msg_audio_package.audioMsgVal,
//				msg_audio_package.sharePointer);
	for (;;) {
		if (fatal_signal_pending(current)) {
			dev_err(&rpmsg_dev->dev, "%s %d.\n", __func__, __LINE__);
			ret = -ERESTARTSYS;
			break;
		}

		if (*wakeup_flag == 1) {
			*wakeup_flag = 0;
			break;
		}
		set_current_state(TASK_KILLABLE);
		spin_unlock_irq(lock);
		tout = schedule_timeout(msecs_to_jiffies(RPMSG_TIMEOUT_NORMAL));
		spin_lock_irq(lock);
		if (!tout) {
			dev_err(&rpmsg_dev->dev,
				"line:%d rpmsg_send timeout %d, flag=%d\n",
				__LINE__, ret, *wakeup_flag);
			pr_info("msg_cmd=%lu\n", msg_cmd);
			if (msg_cmd == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) {
				pr_info("stream:%d, cmd_val=0x%x\n", stream, soc_substream->cmd_val);
			}
			ret = -EIO;
			break;
		} else {
			if (*wakeup_flag == 0)
				continue;
			else if (*wakeup_flag == 1) {
				*wakeup_flag = 0;
				break;
			}
		}
	}

	/* 换醒或者超时后 */
	set_current_state(TASK_RUNNING);
	remove_wait_queue(daudio_tsleep, &wait);
	spin_unlock_irq(lock);
	return ret;

err_rmpsg_send:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(daudio_tsleep, &wait);
	spin_unlock_irq(lock);
	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_daudio_substream_block_send);

int sunxi_hifi_dmic_substream_block_send(unsigned int hifi_id,
			struct snd_pcm_substream *substream, struct snd_soc_dai *dai,
			int stream, unsigned long msg_cmd)
{
	struct snd_soc_dai *cpu_dai = NULL;
	struct snd_soc_pcm_runtime *rtd = NULL;
	struct sunxi_dmic_info *sunxi_dmic = NULL;
	struct msg_audio_package msg_audio_package;
	struct rpmsg_device *rpmsg_dev;
	struct snd_soc_dsp_substream *soc_substream;
	int *wakeup_flag;
	int ret = 0;
	wait_queue_t wait;
	wait_queue_head_t *dmic_tsleep;
	spinlock_t *lock;
	long tout;

	if ((hifi_id > 1) || (gHifi_priv[hifi_id] == NULL) ||
		(IS_ERR_OR_NULL(substream) && IS_ERR_OR_NULL(dai))) {
		pr_err("%s params invalid.\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(dai)) {
		rtd = substream->private_data;
		cpu_dai = rtd->cpu_dai;
		sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	} else {
		cpu_dai = dai;
		sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	}

	rpmsg_dev = gHifi_priv[hifi_id]->rpmsg_dev;

	msg_audio_package.audioMsgVal = msg_cmd;
	switch (msg_cmd) {
	case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
	{
		struct msg_substream_package *msg_substream;

		msg_substream = &sunxi_dmic->msg_capture;
		dmic_tsleep = &msg_substream->tsleep;
		lock = &msg_substream->lock;
		wakeup_flag = &msg_substream->wakeup_flag;
		soc_substream = &msg_substream->soc_substream;

		msg_audio_package.sharePointer = (unsigned int)__pa(soc_substream);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
	{
		struct msg_mixer_package *msg_mixer;

		msg_mixer = &sunxi_dmic->msg_mixer;
		dmic_tsleep = &msg_mixer->tsleep;
		lock = &msg_mixer->lock;
		wakeup_flag = &msg_mixer->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_mixer->soc_mixer);
		__dma_flush_range(&msg_mixer->soc_mixer,
						sizeof(struct snd_soc_dsp_mixer));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
	{
		struct msg_debug_package *msg_debug;

		msg_debug = &sunxi_dmic->msg_debug;
		dmic_tsleep = &msg_debug->tsleep;
		lock = &msg_debug->lock;
		wakeup_flag = &msg_debug->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_debug->soc_debug);
		__dma_flush_range(&msg_debug->soc_debug,
						sizeof(struct snd_soc_dsp_debug));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
	{
		struct msg_component_package *msg_component;
		struct snd_dsp_component *dsp_component;

		dsp_component = &sunxi_dmic->dsp_capcomp;
		msg_component = &dsp_component->msg_component;
		dmic_tsleep = &msg_component->tsleep;
		lock = &msg_component->lock;
		wakeup_flag = &msg_component->wakeup_flag;

		msg_audio_package.sharePointer =
				(unsigned int)__pa(&msg_component->soc_component);
		__dma_flush_range(&msg_component->soc_component,
						sizeof(struct snd_soc_dsp_component));
	}
	break;
	default:
		dev_err(&rpmsg_dev->dev, "msg_cmd error:%lu\n", msg_cmd);
		return -EINVAL;
	}
	spin_lock_irq(lock);
	if (*wakeup_flag) {
		dev_warn(&rpmsg_dev->dev, "%s wakeup_flag was not reset.\n", __func__);
		/* reset */
		*wakeup_flag = 0;
	}
	/* 预备休眠等待 */
	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_KILLABLE);
	add_wait_queue(dmic_tsleep, &wait);
	/* 发送数据 */
	spin_unlock_irq(lock);
	ret = rpmsg_send(rpmsg_dev->ept, &msg_audio_package,
				  sizeof(struct msg_audio_package));
	spin_lock_irq(lock);
	if (ret) {
		dev_err(&rpmsg_dev->dev, "rpmsg_send failed: %d\n", ret);
		ret = -EIO;
		goto err_rmpsg_send;
	}

//	dev_err(&rpmsg_dev->dev, "%s %d audioMsgVal:%d, sharePointer:0x%x\n",
//				__func__, __LINE__, msg_audio_package.audioMsgVal,
//				msg_audio_package.sharePointer);
	for (;;) {
		if (fatal_signal_pending(current)) {
			dev_err(&rpmsg_dev->dev, "%s %d.\n", __func__, __LINE__);
			ret = -ERESTARTSYS;
			break;
		}

		if (*wakeup_flag == 1) {
			*wakeup_flag = 0;
			break;
		}
		set_current_state(TASK_KILLABLE);
		spin_unlock_irq(lock);
		tout = schedule_timeout(msecs_to_jiffies(RPMSG_TIMEOUT_NORMAL));
		spin_lock_irq(lock);
		if (!tout) {
			dev_err(&rpmsg_dev->dev,
				"line:%d rpmsg_send timeout %d, flag=%d\n",
				__LINE__, ret, *wakeup_flag);
			pr_info("msg_cmd=%lu\n", msg_cmd);
			if (msg_cmd == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) {
				pr_info("stream:%d, cmd_val=0x%x\n", stream, soc_substream->cmd_val);
			}
			ret = -EIO;
			break;
		} else {
			if (*wakeup_flag == 0)
				continue;
			else if (*wakeup_flag == 1) {
				*wakeup_flag = 0;
				break;
			}
		}
	}

	/* 换醒或者超时后 */
	set_current_state(TASK_RUNNING);
	remove_wait_queue(dmic_tsleep, &wait);
	spin_unlock_irq(lock);
	return ret;

err_rmpsg_send:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(dmic_tsleep, &wait);
	spin_unlock_irq(lock);
	return ret;
}
EXPORT_SYMBOL(sunxi_hifi_dmic_substream_block_send);

static int msg_audio_package_is_invalid(unsigned long cmd)
{
	switch (cmd) {
	case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
	case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
	case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
	case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
		break;
	default:
		pr_err("[%s] AudioMsgVal:%lu error!\n", __func__, cmd);
		return -EINVAL;
	}
	return 0;
}

int rpmsg_irq_schedule(struct rpmsg_hifi_priv *hifi_priv)
{
	struct msg_audio_package msg_pack = hifi_priv->msg_pack;
	unsigned long flags;

	switch (msg_pack.audioMsgVal) {
	case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
	{
		struct snd_soc_dsp_substream *soc_substream;
		struct snd_soc_dsp_pcm_params *params;

#if 1
		struct msg_substream_package *msg_substream;

		soc_substream = (struct snd_soc_dsp_substream *)__va(msg_pack.sharePointer);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		params = &soc_substream->params;

		msg_substream = container_of(soc_substream, struct msg_substream_package,
							soc_substream);

		spin_lock_irqsave(&msg_substream->lock, flags);
		/* 设置rmsg的用于substream irq 回调 */
		if ((msg_pack.audioMsgVal == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) &&
			((soc_substream->cmd_val == SND_SOC_DSP_PCM_READI) ||
			(soc_substream->cmd_val == SND_SOC_DSP_PCM_WRITEI))) {
			sunxi_pcm_transfer_complete(msg_substream->substream);
		}
		msg_substream->wakeup_flag = 1;
		/* for update output_addr and read again */
		wake_up(&msg_substream->tsleep);
		spin_unlock_irqrestore(&msg_substream->lock, flags);
#else
		/* 预留了其它参数的操作更新 */
		void *driver_data;

		soc_substream = (struct snd_soc_dsp_substream *)__va(msg_pack.sharePointer);
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		params = &soc_substream->params;
		/* find sound device data */
		driver_data = sunxi_hifi_find_sound_drv_info_by_name(params->driver);
		if (IS_ERR_OR_NULL(driver_data))
			return -EFAULT;

		if (!strncmp(params->driver, "audiocodec", sizeof(params->driver))) {
//			struct sunxi_cpudai_info *sunxi_cpudai = (struct sunxi_cpudai_info *)driver_data;
			struct msg_substream_package *msg_substream;

			msg_substream = container_of(soc_substream, struct msg_substream_package,
							soc_substream);

			/* 设置rmsg的用于substream irq 回调 */
			if ((msg_pack.audioMsgVal == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) &&
				((soc_substream->cmd_val == SND_SOC_DSP_PCM_READI) ||
				(soc_substream->cmd_val == SND_SOC_DSP_PCM_WRITEI))) {
				sunxi_pcm_transfer_complete(msg_substream->substream);
			}
			/* for update output_addr and read again */
			msg_substream->wakeup_flag = 1;
			wake_up(&msg_substream->tsleep);
		} else if (!strncmp(params->driver, "snddaudio0", sizeof(params->driver)) ||
			!strncmp(params->driver, "snddaudio1", sizeof(params->driver))) {
//			struct sunxi_daudio_info *sunxi_daudio = (struct sunxi_daudio_info *)driver_data;
			struct msg_substream_package *msg_substream;

			msg_substream = container_of(soc_substream, struct msg_substream_package,
							soc_substream);

			/* 设置rmsg的用于substream irq 回调 */
			if ((msg_pack.audioMsgVal == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) &&
				((soc_substream->cmd_val == SND_SOC_DSP_PCM_READI) ||
				(soc_substream->cmd_val == SND_SOC_DSP_PCM_WRITEI))) {
				sunxi_pcm_transfer_complete(msg_substream->substream);
			}
			/* for update output_addr and read again */
			msg_substream->wakeup_flag = 1;
			wake_up(&msg_substream->tsleep);
		} else if (!strncmp(params->driver, "snddmic", sizeof(params->driver))) {
//			struct sunxi_dmic_info *sunxi_dmic = (struct sunxi_dmic_info *)driver_data;
			struct msg_substream_package *msg_substream;

			msg_substream = container_of(soc_substream, struct msg_substream_package,
							soc_substream);

			/* 设置rmsg的用于substream irq 回调 */
			if ((msg_pack.audioMsgVal == MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) &&
				(soc_substream->cmd_val == SND_SOC_DSP_PCM_READI)) {
				sunxi_pcm_transfer_complete(msg_substream->substream);
			}
			msg_substream->wakeup_flag = 1;
			/* for update output_addr and read again */
			wake_up(&msg_substream->tsleep);
		}
#endif
	}
	break;
	/* wakeup work */
	case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
	{
		struct msg_mixer_package *msg_mixer;
		struct snd_soc_dsp_mixer *soc_mixer;

		soc_mixer = (struct snd_soc_dsp_mixer *)__va(msg_pack.sharePointer);
		__dma_flush_range(soc_mixer, sizeof(struct snd_soc_dsp_mixer));
		msg_mixer = container_of(soc_mixer, struct msg_mixer_package,
							soc_mixer);
		spin_lock_irqsave(&msg_mixer->lock, flags);
		msg_mixer->wakeup_flag = 1;
		wake_up(&msg_mixer->tsleep);
		spin_unlock_irqrestore(&msg_mixer->lock, flags);
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
	{
		struct snd_soc_dsp_debug *soc_debug;

		soc_debug = (struct snd_soc_dsp_debug *)__va(msg_pack.sharePointer);
		__dma_flush_range(soc_debug, sizeof(struct snd_soc_dsp_debug));
	}
	break;
	case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
	{
		struct snd_soc_dsp_component *soc_component;
		struct msg_component_package *msg_component;

		soc_component = (struct snd_soc_dsp_component *)__va(msg_pack.sharePointer);
		__dma_flush_range(soc_component, sizeof(struct snd_soc_dsp_component));
		msg_component = container_of(soc_component, struct msg_component_package, soc_component);
		spin_lock_irqsave(&msg_component->lock, flags);
		msg_component->wakeup_flag = 1;
		wake_up(&msg_component->tsleep);
		spin_unlock_irqrestore(&msg_component->lock, flags);
	}
	break;
	default:
		dev_err_ratelimited(&hifi_priv->rpmsg_dev->dev,
				"[%s] AudioMsgVal:%u error!\n",
				__func__, msg_pack.audioMsgVal);
		return -EFAULT;
	}

	return 0;
}

static void recv_work_func(struct work_struct *work)
{
//	struct rpmsg_hifi_priv *hifi_priv =
//			container_of(work, struct rpmsg_hifi_priv, rpmsg_recv_work);
//	struct rpmsg_device *rpmsg_dev = hifi_priv->rpmsg_dev;
//	struct msg_audio_package *msg_pack = &hifi_priv->msg_pack;

//	rpmsg_irq_complete(hifi_priv->msg_pack);
}

static int rpmsg_hifi_cb(struct rpmsg_device *rpdev, void *data, int len,
						 void *priv, u32 src)
{
	struct rpmsg_hifi_priv *hifi_priv = dev_get_drvdata(&rpdev->dev);
	struct msg_audio_package *msg_pack = &hifi_priv->msg_pack;

	hifi_priv->rx_count++;
	switch (hifi_priv->rx_count) {
	case 1:
		msg_pack->audioMsgVal = *((unsigned int *)data);
		/* 判断是否接受时隙错误 */
		if (msg_audio_package_is_invalid(msg_pack->audioMsgVal)) {
			hifi_priv->rx_count = 0;
			dev_err(&rpdev->dev, "%s failed! audioMsgVal:%u\n", __func__,
						msg_pack->audioMsgVal);
			return -EINVAL;
		}
		break;
	case 2:
		msg_pack->sharePointer = *((unsigned int *)data);
		break;
	default:
		hifi_priv->rx_count = 0;
		break;
	}
	if (hifi_priv->rx_count >= (sizeof(struct msg_audio_package) >> 2)) {
		hifi_priv->rx_count = 0;
		rpmsg_irq_schedule(hifi_priv);
	}
	return 0;
}

static struct rpmsg_device_id rpmsg_driver_hifi_id_table[] = {
	{ .name	= "sunxi,dsp0" },
	{ .name	= "sunxi,dsp1" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_hifi_id_table);

static int rpmsg_hifi_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_hifi_priv *hifi_priv;
	int ret = 0;
	int hifi_id = 0;

	dev_info(&rpdev->dev, "id:%s new channel: 0x%x -> 0x%x!\n",
		  rpdev->id.name, rpdev->src, rpdev->dst);

	hifi_priv = devm_kzalloc(&rpdev->dev, sizeof(struct rpmsg_hifi_priv), GFP_KERNEL);
	if (!hifi_priv)
		return -ENOMEM;
	dev_set_drvdata(&rpdev->dev, hifi_priv);
	hifi_priv->rpmsg_dev = rpdev;

	sscanf(rpdev->id.name, "sunxi,dsp%d", &hifi_id);
	if (hifi_id > 1) {
		ret = -EINVAL;
		goto err_rpdev_id_name;
	}
	gHifi_priv[hifi_id] = hifi_priv;

	snprintf(hifi_priv->wq_name, sizeof(hifi_priv->wq_name), "rpmsg_hifi%d", hifi_id);
	hifi_priv->wq = create_workqueue(hifi_priv->wq_name);
	INIT_WORK(&hifi_priv->rpmsg_recv_work, recv_work_func);
//	queue_work(hifi_priv->wq, &hifi_priv->rpmsg_recv_work);

	dev_info(&rpdev->dev, "rpmsg hifi[%d] client driver is probed\n", hifi_id);

	return 0;

err_rpdev_id_name:
	devm_kfree(&rpdev->dev, hifi_priv);
	return ret;
}

static void rpmsg_hifi_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_hifi_priv *hifi_priv = dev_get_drvdata(&rpdev->dev);
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(gHifi_priv); i++) {
		if (gHifi_priv[i] == hifi_priv) {
			gHifi_priv[i] = NULL;
		}
	}
	cancel_work_sync(&(hifi_priv->rpmsg_recv_work));
	destroy_workqueue(hifi_priv->wq);

	devm_kfree(&rpdev->dev, hifi_priv);
	dev_info(&rpdev->dev, "rpmsg hifi client driver is removed\n");
}

static struct rpmsg_driver rpmsg_hifi_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_hifi_id_table,
	.probe		= rpmsg_hifi_probe,
	.callback	= rpmsg_hifi_cb,
	.remove		= rpmsg_hifi_remove,
};
module_rpmsg_driver(rpmsg_hifi_client);

MODULE_AUTHOR("yumingfeng@allwinnertech.com");
MODULE_DESCRIPTION("Remote processor messaging hifi client driver");
MODULE_LICENSE("GPL v2");
