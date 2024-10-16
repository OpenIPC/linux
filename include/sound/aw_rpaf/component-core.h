/*
 * sound\aw_rpaf\component-core.h -- Remote Process Audio Framework Layer
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

#ifndef _SUNXI_RPAF_COMPONENT_CORE_H
#define _SUNXI_RPAF_COMPONENT_CORE_H

#include <uapi/sound/asound.h>
#include <sound/aw_rpaf/component.h>

#define SUNXI_RPAF_INFO_NAME_LEN 32

struct snd_rpaf_xferi {
	int32_t result;
	void __user *input_buf;
	ssize_t input_length;
	void __user *output_buf;
	ssize_t output_length;
	void __user *dump_buf;
	ssize_t dump_length;
	ssize_t dump_type; /* component type */
};

struct snd_rpaf_xferi32 {
	int32_t result;
	int32_t input_buf;
	int32_t input_length;
	int32_t output_buf;
	int32_t output_length;
	int32_t dump_buf;
	int32_t dump_length;
	int32_t dump_type; /* component type */
};

/*
 * param[0] = MSGBOX_DSP_AUDIO_COMMAND->MSGBOX_DSP_AUDIO_COMPONENT_COMMAND
 * param[1] = *snd_dsp_audio_framework_component
 * param[2] = NULL
 * param[3] = NULL
 */
enum SND_SOC_DSP_AUDIO_COMPONENT_COMMAND {
	SND_SOC_DSP_COMPONENT_IOCTL_CREATE = _IOW('C', 0x00, struct snd_soc_dsp_component_config),
	SND_SOC_DSP_COMPONENT_IOCTL_REMOVE = _IOW('C', 0x01, int),
	SND_SOC_DSP_COMPONENT_IOCTL_STATUS = _IOR('C', 0x02, int),
	SND_SOC_DSP_COMPONENT_IOCTL_SW_PARAMS = _IOWR('C', 0x03, struct snd_soc_dsp_pcm_params),
	SND_SOC_DSP_COMPONENT_IOCTL_START = _IO('C', 0x04),
	SND_SOC_DSP_COMPONENT_IOCTL_STOP = _IO('C', 0x05),
	SND_SOC_DSP_COMPONENT_IOCTL_WRITE = _IOW('C', 0x06, struct snd_rpaf_xferi),
	SND_SOC_DSP_COMPONENT_IOCTL_WRITE32 = _IOW('C', 0x06, struct snd_rpaf_xferi32),
	SND_SOC_DSP_COMPONENT_IOCTL_READ = _IOR('C', 0x07, struct snd_rpaf_xferi),
	SND_SOC_DSP_COMPONENT_IOCTL_READ32 = _IOR('C', 0x07, struct snd_rpaf_xferi32),
	SND_SOC_DSP_COMPONENT_IOCTL_ALGO_SET = _IOW('C', 0x08, struct snd_soc_dsp_component_config),
	SND_SOC_DSP_COMPONENT_IOCTL_ALGO_GET = _IOWR('C', 0x09, struct snd_soc_dsp_component_config),
	SND_SOC_DSP_COMPONENT_IOCTL_UNLINK = _IO('C', 0x0A),
};

struct snd_soc_rpaf_info {
	char name[SUNXI_RPAF_INFO_NAME_LEN];
	struct device *dev;
	uint32_t dsp_id;
	struct miscdevice misc_dev;
	void *driver_data;
	struct list_head list;
};

struct snd_soc_rpaf_pcm_runtime {
	uint32_t dump_start;
	uint32_t dump_type;
	snd_pcm_uframes_t pos;
	snd_pcm_uframes_t hw_ptr;
	snd_pcm_uframes_t appl_ptr;

	/* -- Status -- */
	snd_pcm_uframes_t avail_max;
	snd_pcm_uframes_t hw_ptr_base;	/* Position at buffer restart */
	snd_pcm_uframes_t hw_ptr_interrupt; /* Position at interrupt time */
	u64 hw_ptr_wrap;		/* offset for hw_ptr due to boundary wrap-around */

	/* -- HW params -- */
	unsigned int frame_bits;
	snd_pcm_uframes_t period_size;	/* period size */
	snd_pcm_uframes_t buffer_size;	/* buffer size */

	/* -- SW params -- */
	snd_pcm_uframes_t silence_size;	/* Silence filling size */
	snd_pcm_uframes_t boundary;	/* pointers wrap point */

	snd_pcm_uframes_t silence_start; /* starting pointer to silence area */
	snd_pcm_uframes_t silence_filled; /* size filled with silence */

	/* -- locking / scheduling -- */
	snd_pcm_uframes_t twake;	/* do transfer (!poll) wakeup if non-zero */
	wait_queue_head_t tsleep;	/* transfer sleep */

	unsigned int xrun_cnt;
};

struct snd_dsp_component {
	struct snd_soc_rpaf_info *rpaf_info;
	struct mutex comp_rw_lock;
	spinlock_t lock;
	uint32_t state;
	struct msg_component_package msg_component;
	void *write_area;
	void *dump_area[RPAF_COMPONENT_MAX_NUM];
	void *read_area;
	struct snd_soc_rpaf_pcm_runtime runtime;
	void *private_data;
};

struct snd_soc_rpaf_misc {
	int32_t number;
	struct module *module;

	void *private_data;

	bool registered;
#ifdef CONFIG_PM
	uint32_t power_state;
	struct mutex power_lock;
	wait_queue_head_t power_sleep;
#endif
};

static inline void *snd_soc_rpaf_info_get_drvdata(const struct snd_soc_rpaf_info *rpaf_info)
{
	return rpaf_info->driver_data;
}

static inline void snd_soc_rpaf_info_set_drvdata(struct snd_soc_rpaf_info *rpaf_info,
					      void *data)
{
	rpaf_info->driver_data = data;
}

typedef int32_t (*transfer_f)(struct snd_dsp_component *dsp_component,
			unsigned int hwoff,
			unsigned long data, unsigned int off,
			snd_pcm_uframes_t frames);

static inline snd_pcm_sframes_t snd_soc_rpaf_pcm_bytes_to_frames(
	struct snd_soc_rpaf_pcm_runtime *runtime, ssize_t size)
{
	return size * 8 / runtime->frame_bits;
}

static inline ssize_t snd_soc_rpaf_pcm_frames_to_bytes(
	struct snd_soc_rpaf_pcm_runtime *runtime, snd_pcm_sframes_t size)
{
	return size * runtime->frame_bits / 8;
}

static inline snd_pcm_uframes_t snd_soc_rpaf_pcm_pointer(
	struct snd_soc_rpaf_pcm_runtime *runtime)
{
	return snd_soc_rpaf_pcm_bytes_to_frames(runtime, runtime->pos);
}

static inline snd_pcm_uframes_t snd_soc_rpaf_pcm_capture_avail(
	struct snd_soc_rpaf_pcm_runtime *runtime)
{
	snd_pcm_sframes_t avail = runtime->hw_ptr - runtime->appl_ptr;

	if (avail < 0)
		avail += runtime->boundary;
	return avail;
}

void snd_soc_dsp_component_list_add_tail(struct snd_soc_dsp_component *component);

void snd_soc_dsp_component_list_del(struct snd_soc_dsp_component *component);

struct snd_soc_dsp_component *snd_soc_dsp_component_get_from_list_by_pcmdev(
			int32_t card, int32_t device, int32_t stream);

/* for snd_soc_rpaf_info list operation api */
void snd_soc_rpaf_info_list_add_tail(struct snd_soc_rpaf_info *rpaf_info);
void snd_soc_rpaf_info_list_del(struct snd_soc_rpaf_info *rpaf_info);

struct snd_soc_rpaf_info *snd_soc_rpaf_info_get_from_list_by_minor(int32_t minor);
struct snd_soc_rpaf_info *snd_soc_rpaf_info_get_from_list_by_dspid(uint32_t id);
struct snd_soc_rpaf_info *snd_soc_rpaf_info_get_from_list_by_miscdevice(
				struct miscdevice *device);

void snd_soc_rpaf_pcm_stream_component_lock(void);
void snd_soc_rpaf_pcm_stream_component_unlock(void);

void snd_soc_rpaf_pcm_stream_lock_irq(struct snd_dsp_component *dsp_component);
void snd_soc_rpaf_pcm_stream_unlock_irq(struct snd_dsp_component *dsp_component);
unsigned long snd_soc_rpaf_pcm_stream_lock_irqsave(struct snd_dsp_component *dsp_component);
void snd_soc_rpaf_pcm_stream_unlock_irqrestore(struct snd_dsp_component *dsp_component,
				      unsigned long flags);
void snd_soc_rpaf_pcm_stream_update_complete(void *arg);
#endif
