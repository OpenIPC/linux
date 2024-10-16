/*
 * sound\sunxi-rpaf\component\component-core.c
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

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/crypto.h>
#include <linux/miscdevice.h>
#include <linux/capability.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <asm/cacheflush.h>

#include <sound/aw_rpaf/component-core.h>
#include <sound/aw_rpaf/rpmsg_hifi.h>

//#define RPAF_MEM_DEBUG_LOG

static unsigned int component_id[RPAF_COMPONENT_MAX_NUM] = {0};
static DEFINE_MUTEX(comp_id_mutex);

static DEFINE_SPINLOCK(comp_stream_lock);

static LIST_HEAD(snd_soc_rpaf_info_list);
static LIST_HEAD(snd_soc_dsp_component_list);

static DEFINE_MUTEX(rpaf_sub_list_mutex);
static DEFINE_MUTEX(rpaf_info_list_mutex);

void snd_soc_rpaf_pcm_stream_component_lock(void)
{
	spin_lock(&comp_stream_lock);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_component_lock);

void snd_soc_rpaf_pcm_stream_component_unlock(void)
{
	spin_unlock(&comp_stream_lock);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_component_unlock);

void snd_soc_rpaf_pcm_stream_lock(struct snd_dsp_component *dsp_component)
{
	spin_lock(&dsp_component->lock);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_lock);

void snd_soc_rpaf_pcm_stream_lock_irq(struct snd_dsp_component *dsp_component)
{
	local_irq_disable();
	snd_soc_rpaf_pcm_stream_lock(dsp_component);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_lock_irq);

unsigned long snd_soc_rpaf_pcm_stream_lock_irqsave(struct snd_dsp_component *dsp_component)
{
	unsigned long flags = 0;

	local_irq_save(flags);
	snd_soc_rpaf_pcm_stream_lock(dsp_component);
	return flags;
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_lock_irqsave);

void snd_soc_rpaf_pcm_stream_unlock(struct snd_dsp_component *dsp_component)
{
	spin_unlock(&dsp_component->lock);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_unlock);

void snd_soc_rpaf_pcm_stream_unlock_irq(struct snd_dsp_component *dsp_component)
{
	snd_soc_rpaf_pcm_stream_unlock(dsp_component);
	local_irq_enable();
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_unlock_irq);

void snd_soc_rpaf_pcm_stream_unlock_irqrestore(struct snd_dsp_component *dsp_component,
				      unsigned long flags)
{
	snd_soc_rpaf_pcm_stream_unlock(dsp_component);
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_pcm_stream_unlock_irqrestore);

/* for snd_soc_dsp_component list operation api */
void snd_soc_dsp_component_list_add_tail(struct snd_soc_dsp_component *component)
{
	mutex_lock(&rpaf_sub_list_mutex);
	list_add_tail(&component->list, &snd_soc_dsp_component_list);
	mutex_unlock(&rpaf_sub_list_mutex);
}
EXPORT_SYMBOL_GPL(snd_soc_dsp_component_list_add_tail);

void snd_soc_dsp_component_list_del(struct snd_soc_dsp_component *component)
{
	mutex_lock(&rpaf_sub_list_mutex);
	list_del(&component->list);
	mutex_unlock(&rpaf_sub_list_mutex);
}
EXPORT_SYMBOL_GPL(snd_soc_dsp_component_list_del);

struct snd_soc_dsp_component *snd_soc_dsp_component_get_from_list_by_pcmdev(
			int card, int device, int stream)
{
	struct snd_soc_dsp_component *component = NULL;
	struct snd_soc_dsp_pcm_params *pcm_params = NULL;

	mutex_lock(&rpaf_sub_list_mutex);
	list_for_each_entry(component, &snd_soc_dsp_component_list, list) {
		pcm_params = &(component->params);

		if (pcm_params->card == card &&
			pcm_params->device == device &&
			pcm_params->stream == stream) {
			mutex_unlock(&rpaf_sub_list_mutex);
			return component;
		}
	}
	mutex_unlock(&rpaf_sub_list_mutex);
	return NULL;
}
EXPORT_SYMBOL_GPL(snd_soc_dsp_component_get_from_list_by_pcmdev);

/* for snd_soc_rpaf_info list operation api */
void snd_soc_rpaf_info_list_add_tail(struct snd_soc_rpaf_info *rpaf_info)
{
	mutex_lock(&rpaf_info_list_mutex);
	list_add_tail(&rpaf_info->list, &snd_soc_rpaf_info_list);
	mutex_unlock(&rpaf_info_list_mutex);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_info_list_add_tail);

void snd_soc_rpaf_info_list_del(struct snd_soc_rpaf_info *rpaf_info)
{
	mutex_lock(&rpaf_info_list_mutex);
	list_del(&rpaf_info->list);
	mutex_unlock(&rpaf_info_list_mutex);
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_info_list_del);

struct snd_soc_rpaf_info *snd_soc_rpaf_info_get_from_list_by_minor(int minor)
{
	struct snd_soc_rpaf_info *rpaf_info;

	mutex_lock(&rpaf_info_list_mutex);
	list_for_each_entry(rpaf_info, &snd_soc_rpaf_info_list, list) {
		if (rpaf_info->misc_dev.minor == minor) {
			mutex_unlock(&rpaf_info_list_mutex);
			return rpaf_info;
		}
	}
	mutex_unlock(&rpaf_info_list_mutex);

	return NULL;
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_info_get_from_list_by_minor);

struct snd_soc_rpaf_info *snd_soc_rpaf_info_get_from_list_by_dspid(unsigned int id)
{
	struct snd_soc_rpaf_info *rpaf_info;

	mutex_lock(&rpaf_info_list_mutex);
	list_for_each_entry(rpaf_info, &snd_soc_rpaf_info_list, list) {
		if (rpaf_info->dsp_id == id) {
			mutex_unlock(&rpaf_info_list_mutex);
			return rpaf_info;
		}
	}
	mutex_unlock(&rpaf_info_list_mutex);
	return NULL;
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_info_get_from_list_by_dspid);

struct snd_soc_rpaf_info *snd_soc_rpaf_info_get_from_list_by_miscdevice(
				struct miscdevice *device)
{
	struct snd_soc_rpaf_info *rpaf_info;

	mutex_lock(&rpaf_info_list_mutex);
	list_for_each_entry(rpaf_info, &snd_soc_rpaf_info_list, list) {
		if (&rpaf_info->misc_dev == device) {
			mutex_unlock(&rpaf_info_list_mutex);
			return rpaf_info;
		}
	}
	mutex_unlock(&rpaf_info_list_mutex);
	return NULL;
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_info_get_from_list_by_miscdevice);

static int snd_soc_rpaf_misc_open(struct inode *inode, struct file *file)
{
	struct snd_soc_rpaf_info *rpaf_info = NULL;
	struct snd_dsp_component *dsp_component = NULL;
	struct msg_component_package *msg_component = NULL;
	int minor = iminor(inode);
	int err = -ENODEV;

	err = nonseekable_open(inode, file);
	if (err < 0)
		return err;

	rpaf_info = snd_soc_rpaf_info_get_from_list_by_minor(minor);
	if (IS_ERR_OR_NULL(rpaf_info))
		return -EBADF;

	dsp_component = kzalloc(sizeof(struct snd_dsp_component), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dsp_component)) {
		dev_err(rpaf_info->dev, "%s cannot malloc for dsp_component.\n", __func__);
		return -ENOMEM;
	}
	msg_component = &dsp_component->msg_component;
	init_waitqueue_head(&msg_component->tsleep);
	spin_lock_init(&msg_component->lock);

	snd_soc_dsp_component_list_add_tail(&msg_component->soc_component);
	dsp_component->rpaf_info = rpaf_info;
	file->private_data = dsp_component;
	dsp_component->state = SND_DSP_COMPONENT_STATE_OPEN;

	mutex_init(&dsp_component->comp_rw_lock);
	spin_lock_init(&dsp_component->lock);
#ifdef RPAF_MEM_DEBUG_LOG
	dev_info(rpaf_info->dev, "%s line:%d dsp_component:%p, pa:0x%llx, __pa.component:0x%llx\n",
				__func__, __LINE__, dsp_component, __pa(dsp_component),
				__pa(&msg_component->soc_component));
#endif
	return err;
}

static int snd_soc_rpaf_misc_common_stop(struct snd_dsp_component *dsp_component);
static int snd_soc_rpaf_misc_common_remove(struct snd_dsp_component *dsp_component);
static int snd_soc_rpaf_misc_release(struct inode *inode, struct file *file)
{
	struct snd_dsp_component *dsp_component = file->private_data;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	int i = 0;
	int result;

	/* 判断当前状态 */
	switch (dsp_component->state) {
	case SND_DSP_COMPONENT_STATE_START:
	case SND_DSP_COMPONENT_STATE_RUNNING:
		result = snd_soc_rpaf_misc_common_stop(dsp_component);
		if (result < 0)
			return result;
	case SND_DSP_COMPONENT_STATE_STOP:
		result = snd_soc_rpaf_misc_common_remove(dsp_component);
		if (result < 0)
			return result;
		break;
	case SND_DSP_COMPONENT_STATE_CLOSE:
	default:
		return -EINVAL;
	case SND_DSP_COMPONENT_STATE_OPEN:
	case SND_DSP_COMPONENT_STATE_CREATE:
	case SND_DSP_COMPONENT_STATE_REMOVE:
		break;
	}
	dsp_component->state = SND_DSP_COMPONENT_STATE_CLOSE;

	/* 其它stream持有需要互斥 */
	if (component->read_addr) {
		dma_free_coherent(rpaf_info->dev, component->read_size,
			dsp_component->read_area, component->read_addr);
#ifdef RPAF_MEM_DEBUG_LOG
		dev_err(rpaf_info->dev, "read_area:%p, read_addr:0x%x\n",
			dsp_component->read_area, component->read_addr);
#endif
		component->read_addr = 0;
		dsp_component->read_area = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(component->dump_addr); i++) {
		if (!component->dump_addr[i])
			continue;
		dma_free_coherent(rpaf_info->dev, component->dump_size,
			dsp_component->dump_area[i], component->dump_addr[i]);
#ifdef RPAF_MEM_DEBUG_LOG
		dev_err(rpaf_info->dev, "dump_area[%d]:%p, dump_addr:0x%x\n",
			i, dsp_component->dump_area[i], component->dump_addr[i]);
#endif
		component->dump_addr[i] = 0;
		dsp_component->dump_area[i] = NULL;
	}

	if (component->write_addr) {
		dma_free_coherent(rpaf_info->dev, component->write_size,
			dsp_component->write_area, component->write_addr);
#ifdef RPAF_MEM_DEBUG_LOG
		dev_err(rpaf_info->dev, "write_area:%p, write_addr:0x%x\n",
			dsp_component->write_area, component->write_addr);
#endif
		component->write_addr = 0;
		dsp_component->write_area = NULL;
	}

	snd_soc_dsp_component_list_del(component);

	mutex_destroy(&dsp_component->comp_rw_lock);
	kfree(dsp_component);

	return 0;
}

#if 0
/* 需要在lib_write接口返回后才可以调用操作,目前只支持一个dump地址 */
static ssize_t snd_soc_rpaf_misc_read(struct file *file, char __user *buf,
				size_t count, loff_t *offset)
{
	struct snd_dsp_component *dsp_component = file->private_data;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
	unsigned int read_size = count;

	if (count > component->read_size)
		read_size = component->read_size;

	/* 加个互斥锁 */
	mutex_lock(&dsp_component->comp_rw_lock);
	__dma_flush_range(dsp_component->read_area, read_size);
	/* 无需和dsp通信，只要等待lib_write接口返回后即可操作获取对应的dump数据 */
	if (copy_to_user(buf, dsp_component->read_area, read_size)) {
		mutex_unlock(&dsp_component->comp_rw_lock);
		return -EFAULT;
	}
	mutex_unlock(&dsp_component->comp_rw_lock);
	return read_size;
}
#endif

/* 需要在lib_write接口返回后才可以调用操作,目前只支持一个dump地址 */
ssize_t snd_soc_rpaf_misc_component_lib_read(struct snd_dsp_component *dsp_component,
				struct snd_rpaf_xferi *xferi)
{
	ssize_t size = xferi->dump_length;
	ssize_t read_size = size;
	char __user *buf = (char __user *)(xferi->dump_buf);
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
//	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
//	struct snd_soc_dsp_pcm_params *pcm_params = &(component->params);

	if (xferi->dump_type >= RPAF_COMPONENT_MAX_NUM)
		return -EINVAL;

	if (IS_ERR_OR_NULL(dsp_component->dump_area[xferi->dump_type]))
		return -EFAULT;

	/* 加个互斥锁 */
	mutex_lock(&dsp_component->comp_rw_lock);

	if (size > component->dump_size)
		read_size = component->dump_size;
	if (size > component->dump_length[xferi->dump_type])
		read_size = component->dump_length[xferi->dump_type];

	__dma_flush_range(dsp_component->dump_area[xferi->dump_type],
					component->dump_size);

	/* 无需和dsp通信，只要等待lib_write接口返回后即可操作获取对应的dump数据 */
	if (copy_to_user(buf, dsp_component->dump_area[xferi->dump_type], read_size)) {
		mutex_unlock(&dsp_component->comp_rw_lock);
		return -EFAULT;
	}
	xferi->dump_length = read_size;
	mutex_unlock(&dsp_component->comp_rw_lock);

	return read_size;
}

static int snd_soc_rpaf_pcm_wait_for_avail(struct snd_dsp_component *dsp_component,
			      snd_pcm_uframes_t *availp)
{
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	wait_queue_t wait;
	int err = 0;
	snd_pcm_uframes_t avail = 0;
	long wait_time, tout;

	init_waitqueue_entry(&wait, current);
	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&runtime->tsleep, &wait);

	wait_time = msecs_to_jiffies(5 * 1000);

	for (;;) {
		if (signal_pending(current)) {
			err = -ERESTARTSYS;
			break;
		}

		/*
		 * We need to check if space became available already
		 * (and thus the wakeup happened already) first to close
		 * the race of space already having become available.
		 * This check must happen after been added to the waitqueue
		 * and having current state be INTERRUPTIBLE.
		 */
		avail = snd_soc_rpaf_pcm_capture_avail(runtime);
		if (avail >= runtime->twake)
			break;
		snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);

		tout = schedule_timeout(wait_time);

		snd_soc_rpaf_pcm_stream_lock_irq(dsp_component);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!tout) {
			pr_alert("[%s] line:%d cannot get avail data.\n",
				__func__, __LINE__);
			err = -EIO;
			break;
		}
	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&runtime->tsleep, &wait);
	*availp = avail;
	return err;
}

static int32_t snd_soc_rpaf_pcm_stream_lib_read_transfer(struct snd_dsp_component *dsp_component,
				     unsigned int hwoff,
				     unsigned long data, unsigned int off,
				     snd_pcm_uframes_t frames)
{
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	char __user *buf = (char __user *) data +
		snd_soc_rpaf_pcm_frames_to_bytes(runtime, off);
	char *hwbuf = dsp_component->read_area +
		snd_soc_rpaf_pcm_frames_to_bytes(runtime, hwoff);
	ssize_t sizes = snd_soc_rpaf_pcm_frames_to_bytes(runtime, frames);

	snd_soc_rpaf_pcm_stream_component_lock();

	if (!dsp_component->private_data) {
		awrpaf_err("pcm stream had been closed.\n");

		snd_soc_rpaf_pcm_stream_component_unlock();

		return -EFAULT;
	}

	if (copy_to_user(buf, hwbuf, sizes)) {

		snd_soc_rpaf_pcm_stream_component_unlock();

		return -EFAULT;
	}

	snd_soc_rpaf_pcm_stream_component_unlock();

	return 0;
}

static int snd_soc_rpaf_pcm_update_hw_ptr(struct snd_dsp_component *dsp_component,
					unsigned int in_interrupt)
{
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct snd_soc_dsp_pcm_params *pcm_params = &(soc_component->params);
	snd_pcm_uframes_t avail;
	snd_pcm_uframes_t pos;
	snd_pcm_uframes_t old_hw_ptr, new_hw_ptr, hw_base;
	snd_pcm_sframes_t delta;

	old_hw_ptr = runtime->hw_ptr;

	pos = snd_soc_rpaf_pcm_pointer(runtime);
	if (pos >= runtime->buffer_size)
		pos = 0;

	hw_base = runtime->hw_ptr_base;
	new_hw_ptr = hw_base + pos;

	/*
	 * new_hw_ptr might be lower than old_hw_ptr in case
	 * when pointer cross the end of the ring buffer
	 */
	if (new_hw_ptr < old_hw_ptr) {
		hw_base += runtime->buffer_size;
		if (hw_base >= runtime->boundary) {
			hw_base = 0;
		}
		new_hw_ptr = hw_base + pos;
	}

	delta = new_hw_ptr - old_hw_ptr;

	/* it means cross the end of boundary */
	if (delta < 0)
		delta += runtime->boundary;

	/* something must be really wrong */
	if (delta >= runtime->buffer_size + runtime->period_size) {
		pr_err("[%s] Unexpected hw_ptr, stream:%d, pos:%lu, new_hw_ptr:%lu, old_hw_ptr:%lu\n",
				(in_interrupt) ? "Q" : "P",
				pcm_params->stream, pos, new_hw_ptr, old_hw_ptr);
		return 0;
	}

	if (delta > runtime->period_size + runtime->period_size / 2) {
		pr_debug("[%s] Lost interrupts? stream:%d, pos:%lu, new_hw_ptr:%lu, old_hw_ptr:%lu\n",
			(in_interrupt) ? "Q" : "P",
			pcm_params->stream, pos, new_hw_ptr, old_hw_ptr);
	}

	if (runtime->hw_ptr == new_hw_ptr)
		return 0;

	runtime->hw_ptr_base = hw_base;
	runtime->hw_ptr = new_hw_ptr;

	/* update avail */
	avail = snd_soc_rpaf_pcm_capture_avail(runtime);
	if (avail > runtime->avail_max)
		runtime->avail_max = avail;

	if (avail >= runtime->buffer_size) {
		return -EPIPE;
	}

	if (runtime->twake) {
		if (avail >= runtime->twake)
			wake_up(&runtime->tsleep);
	}

	return 0;
}

void snd_soc_rpaf_pcm_stream_update_complete(void *arg)
{
	struct snd_dsp_component *dsp_component = arg;
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	unsigned long flags;

	flags = snd_soc_rpaf_pcm_stream_lock_irqsave(dsp_component);

	runtime->pos += snd_soc_rpaf_pcm_frames_to_bytes(runtime,
					runtime->period_size);
	if (runtime->pos >= snd_soc_rpaf_pcm_frames_to_bytes(runtime,
					runtime->buffer_size))
		runtime->pos = 0;

	if (snd_soc_rpaf_pcm_update_hw_ptr(dsp_component, 1) < 0) {
		goto _end;
	}
 _end:
	snd_soc_rpaf_pcm_stream_unlock_irqrestore(dsp_component, flags);
}
EXPORT_SYMBOL(snd_soc_rpaf_pcm_stream_update_complete);

static int snd_soc_rpaf_misc_common_start(struct snd_dsp_component *dsp_component);

ssize_t snd_soc_rpaf_misc_stream_lib_read(struct snd_dsp_component *dsp_component,
				struct snd_rpaf_xferi *xferi, transfer_f transfer)
{
	char __user *buf = (char __user *)(xferi->dump_buf);
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	ssize_t size = xferi->dump_length;
	snd_pcm_sframes_t xfer = 0;
	snd_pcm_uframes_t offset = 0;
	snd_pcm_uframes_t avail = 0;
	ssize_t err = 0;

	if (xferi->dump_type >= RPAF_COMPONENT_MAX_NUM)
		return -EINVAL;

	if (IS_ERR_OR_NULL(dsp_component->read_area))
		return -EFAULT;

	if (dsp_component->state != SND_DSP_COMPONENT_STATE_RUNNING) {
		err = snd_soc_rpaf_misc_common_start(dsp_component);
		if (err < 0) {
			awrpaf_err("start failed.\n");
			return err;
		}
	}

	snd_soc_rpaf_pcm_stream_lock_irq(dsp_component);

	if (!dsp_component->private_data) {
		err = -EFAULT;
		goto _end_unlock;
	}

	runtime->dump_type = xferi->dump_type;
	runtime->dump_start = 1;

	snd_soc_rpaf_pcm_update_hw_ptr(dsp_component, 0);
	avail = snd_soc_rpaf_pcm_capture_avail(runtime);
	while (size > 0) {
		snd_pcm_uframes_t frames, appl_ptr, appl_ofs;
		snd_pcm_uframes_t cont;
		if (!avail) {
			runtime->twake = 1;
			err = snd_soc_rpaf_pcm_wait_for_avail(dsp_component, &avail);
			if (err < 0)
				goto _end_unlock;
			if (!avail)
				continue;
		}
		frames = size > avail ? avail : size;
		cont = runtime->buffer_size - runtime->appl_ptr % runtime->buffer_size;
		if (frames > cont)
			frames = cont;
		if (WARN_ON(!frames)) {
			runtime->twake = 0;
			snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);
			return -EINVAL;
		}
		appl_ptr = runtime->appl_ptr;
		appl_ofs = appl_ptr % runtime->buffer_size;

		snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);
		err = transfer(dsp_component, appl_ofs, (unsigned long)buf, offset, frames);
		snd_soc_rpaf_pcm_stream_lock_irq(dsp_component);
		if (err < 0)
			goto _end_unlock;
		appl_ptr += frames;
		if (appl_ptr >= runtime->boundary)
			appl_ptr -= runtime->boundary;
		runtime->appl_ptr = appl_ptr;
		offset += frames;
		size -= frames;
		xfer += frames;
		avail -= frames;
	}

_end_unlock:
	runtime->twake = 0;
	snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);
	return xfer > 0 ? xfer : err;
}

ssize_t snd_soc_rpaf_misc_lib_read(struct snd_dsp_component *dsp_component,
				struct snd_rpaf_xferi *xferi)
{
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
	ssize_t read_size;

	switch (component->comp_mode) {
	case SND_DSP_COMPONENT_MODE_INDEPENDENCE:
		read_size = snd_soc_rpaf_misc_component_lib_read(dsp_component, xferi);
		dsp_component->state = SND_DSP_COMPONENT_STATE_RUNNING;
		break;
	case SND_DSP_COMPONENT_MODE_STREAM:
		read_size = snd_soc_rpaf_misc_stream_lib_read(dsp_component,
				xferi, snd_soc_rpaf_pcm_stream_lib_read_transfer);
		dsp_component->state = SND_DSP_COMPONENT_STATE_RUNNING;
		break;
	default:
		read_size = -EINVAL;
		break;
	}
	return read_size;
}

static ssize_t snd_soc_rpaf_write_to_user(struct snd_dsp_component *dsp_component,
			void __user * const data, ssize_t *size)
{
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
//	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	ssize_t write_size = *size;
	char __user * const buf = (char __user * const)data;

//	__dma_flush_range(component, sizeof(struct snd_soc_dsp_component));
	if (*size > component->read_size)
		write_size = component->read_size;
	if (*size > component->read_length)
		write_size = component->read_length;

	__dma_flush_range(dsp_component->read_area, component->read_size);
	if (copy_to_user(buf, dsp_component->read_area, write_size)) {
		*size = 0;
		return -EFAULT;
	}
	*size = write_size;

	return write_size;
}

static ssize_t snd_soc_rpaf_misc_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	struct snd_dsp_component *dsp_component = file->private_data;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	ssize_t read_size = count;
	int ret;

	if (count > component->read_size)
		read_size = component->read_size;

	mutex_lock(&dsp_component->comp_rw_lock);
	if (copy_from_user(dsp_component->write_area, buf, read_size)) {
		mutex_unlock(&dsp_component->comp_rw_lock);
		return 0;
	}
	__dma_flush_range(dsp_component->write_area, read_size);
	component->cmd_val = SND_SOC_DSP_COMPONENT_WRITE;
	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	mutex_unlock(&dsp_component->comp_rw_lock);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	return read_size;
}

ssize_t snd_soc_rpaf_misc_lib_write(struct snd_dsp_component *dsp_component,
			  const void __user *data, ssize_t size)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
	ssize_t read_size = size;
	const char __user *buf = (const char __user *)data;
	int ret;

	if (IS_ERR_OR_NULL(dsp_component->write_area) ||
		IS_ERR_OR_NULL(dsp_component->read_area))
		return -EFAULT;

	mutex_lock(&dsp_component->comp_rw_lock);
	if (size > component->write_size)
		read_size = component->write_size;
	component->write_length = read_size;

	if (copy_from_user(dsp_component->write_area, buf, read_size)) {
		mutex_unlock(&dsp_component->comp_rw_lock);
		return 0;
	}

	__dma_flush_range(dsp_component->write_area, read_size);
	component->cmd_val = SND_SOC_DSP_COMPONENT_WRITE;
	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	mutex_unlock(&dsp_component->comp_rw_lock);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	return read_size;
}

static int snd_soc_rpaf_misc_lib_pcm_params(struct snd_soc_dsp_component *component,
			struct snd_soc_dsp_pcm_params *params)
{
	struct snd_soc_dsp_pcm_params *pcm_params = &(component->params);

	pcm_params->channels = params->channels;
	pcm_params->rate = params->rate;
	pcm_params->card = params->card;
	pcm_params->device = params->device;
	pcm_params->format = params->format;
	pcm_params->stream = params->stream;
	pcm_params->resample_rate = params->resample_rate;
	pcm_params->period_size = params->period_size;
	pcm_params->periods = params->periods;
	pcm_params->buffer_size = params->buffer_size;
	pcm_params->pcm_frames = params->pcm_frames;
	pcm_params->data_type = params->data_type;
	/* mp3 - aac */
	pcm_params->codec_type = params->codec_type;

	pcm_params->hw_stream = params->hw_stream;
	pcm_params->data_mode = params->data_mode;
	strncpy(pcm_params->driver, params->driver, 31);

	pcm_params->input_size = params->input_size;
	pcm_params->output_size = params->output_size;
	pcm_params->dump_size = params->dump_size;
	memcpy(pcm_params->algo_params, params->algo_params, sizeof(params->algo_params));

#if 1//def RPAF_MEM_DEBUG_LOG
	awrpaf_info("==================================\n");
	awrpaf_info("channels = %d\n", pcm_params->channels);
	awrpaf_info("rate = %d\n", pcm_params->rate);
	awrpaf_info("card = %d\n", pcm_params->card);
	awrpaf_info("device = %d\n", pcm_params->device);
	awrpaf_info("format = %d\n", pcm_params->format);
	awrpaf_info("stream = %d\n", pcm_params->stream);
	awrpaf_info("resample_rate = %d\n", pcm_params->resample_rate);
	awrpaf_info("period_size = %u\n", pcm_params->period_size);
	awrpaf_info("buffer_size = %u\n", pcm_params->buffer_size);
	awrpaf_info("data_type = %d\n", pcm_params->periods);
	awrpaf_info("codec_type = %d\n", pcm_params->codec_type);
	awrpaf_info("hw_stream = %d\n", pcm_params->hw_stream);
	awrpaf_info("driver = %s\n", pcm_params->driver);
	awrpaf_info("==================================\n");
#endif

	return 0;
}

static void snd_soc_rpaf_runtime_pcm_params(struct snd_soc_rpaf_pcm_runtime *runtime,
	struct snd_soc_dsp_pcm_params *pcm_params)
{
	int bits = 0;

	/* -- HW params -- */
	switch (pcm_params->format) {
	case SNDRV_PCM_FORMAT_S16:
	case SNDRV_PCM_FORMAT_U16:
		bits = 16;
		break;
	case SNDRV_PCM_FORMAT_S24:
	case SNDRV_PCM_FORMAT_U24:
	case SNDRV_PCM_FORMAT_S32:
	case SNDRV_PCM_FORMAT_U32:
		bits = 32;
		break;
	}
	runtime->frame_bits = pcm_params->channels * bits;

	/* period size */
	runtime->period_size = pcm_params->period_size;
	/* buffer size */
	runtime->buffer_size = pcm_params->buffer_size;
	runtime->avail_max = runtime->buffer_size;

	/* -- SW params -- */
	/* Silence filling size */
	runtime->silence_size = runtime->buffer_size;
	/* pointers wrap point */
	runtime->boundary = runtime->buffer_size;
	while (runtime->boundary * 2 <= LONG_MAX - runtime->buffer_size)
		runtime->boundary *= 2;

	/* transfer sleep */
	init_waitqueue_head(&runtime->tsleep);
}

static int snd_soc_rpaf_misc_set_component_config_from_user(
		struct snd_dsp_component *dsp_component,
		struct snd_soc_dsp_component_config __user *_component_config)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *component = &msg_component->soc_component;
	struct snd_soc_dsp_pcm_params *params;
	struct snd_soc_dsp_component_config *component_config;
	struct msg_component_package *stream_msg_component = NULL;
	struct snd_dsp_component *stream_dsp_component = NULL;
	int i = 0;
	int ret = 0;

	component_config = memdup_user(_component_config, sizeof(*component_config));
	if (IS_ERR(component_config))
		return PTR_ERR(component_config);

	params = &component_config->pcm_params;
	component->comp_mode = component_config->comp_mode;
	component->component_type = component_config->component_type;
	component->transfer_type = component_config->transfer_type;
	memcpy(component->component_sort, component_config->component_sort,
			sizeof(component->component_sort));

	if (params->pcm_frames == 0) {
		kfree(component_config);
		return -EFAULT;
	}

	ret = snd_soc_rpaf_misc_lib_pcm_params(component, params);
	if (ret < 0)
		goto err_set_misc_lib_pcm_params;

	snd_soc_rpaf_runtime_pcm_params(&dsp_component->runtime, params);

	switch (component_config->comp_mode) {
	case SND_DSP_COMPONENT_MODE_INDEPENDENCE:
		component->write_size =  params->input_size;
		component->read_size =  params->output_size;
		component->dump_size = params->dump_size;
		break;
	case SND_DSP_COMPONENT_MODE_STREAM:
		component->write_size = 0;
		component->dump_size = 0;
		component->read_size = snd_soc_rpaf_pcm_frames_to_bytes(runtime,
						runtime->buffer_size);
		break;
	default:
		goto err_set_misc_lib_pcm_params;
		return -EINVAL;
	}

	if ((component->write_size) && (!component->write_addr)) {
		dma_addr_t write_addr;
		dsp_component->write_area = dma_alloc_coherent(rpaf_info->dev,
						component->write_size,
						&write_addr, GFP_KERNEL);
		if (IS_ERR_OR_NULL(dsp_component->write_area)) {
			ret = -ENOMEM;
			goto err_malloc_write_addr;
		}
		component->write_addr = write_addr;
	}

	if ((component->read_size) && (!component->read_addr)) {
		dma_addr_t read_addr;
		dsp_component->read_area = dma_alloc_coherent(rpaf_info->dev,
							component->read_size,
							&read_addr, GFP_KERNEL);
		if (IS_ERR_OR_NULL(dsp_component->read_area)) {
			ret = -ENOMEM;
			goto err_malloc_read_addr;
		}
		component->read_addr = read_addr;
	}

	dsp_component->state = SND_DSP_COMPONENT_STATE_SETUP;

	if (component_config->comp_mode == SND_DSP_COMPONENT_MODE_STREAM)
		goto _wait_msg_component_stream;
	if (!component->dump_size)
		goto _ignore_dump_area;

	for (i = 0; i < ARRAY_SIZE(component->dump_addr); i++) {
		dma_addr_t dump_addr;
		if (component->dump_addr[i])
			continue;
		if ((component->component_type >> i) & 0x1) {
			dsp_component->dump_area[i] = dma_alloc_coherent(rpaf_info->dev,
							component->dump_size,
							&dump_addr, GFP_KERNEL);
			if (IS_ERR_OR_NULL(dsp_component->dump_area[i])) {
				ret = -ENOMEM;
				goto err_malloc_dump_addr;
			}
			component->dump_addr[i] = dump_addr;
		}
	}
_ignore_dump_area:
	kfree(component_config);
	return ret;

_wait_msg_component_stream:

	/* 找到对应的音频流，寻找不超过10秒? */
	do {
		static int i = 1;

		stream_msg_component = sunxi_hifi_list_msg_component_find_item(msg_component);
		if (i++ >= 100) {
			i = 0;
			awrpaf_err("cannot find running msg_component.\n");
			ret = -EFAULT;
			goto err_find_msg_component;
		}
		msleep(100);
	} while (!stream_msg_component);
	stream_dsp_component = container_of(stream_msg_component, struct snd_dsp_component,
							msg_component);
	/* 把参数给到pcm stream处理那一端 */
	snd_soc_rpaf_pcm_stream_component_lock();

	stream_dsp_component->private_data = dsp_component;
	dsp_component->private_data = stream_dsp_component;

	snd_soc_rpaf_pcm_stream_component_unlock();

	kfree(component_config);
	return ret;

err_find_msg_component:
err_malloc_dump_addr:
	dma_free_coherent(rpaf_info->dev, component->read_size,
				dsp_component->read_area, component->read_addr);
	component->read_addr = 0;
	dsp_component->read_area = NULL;
	for (i = 0; i < ARRAY_SIZE(component->dump_addr); i++) {
		if (!component->dump_addr[i])
			continue;
		dma_free_coherent(rpaf_info->dev, component->dump_size,
				dsp_component->dump_area[i], component->dump_addr[i]);
		component->dump_addr[i] = 0;
		dsp_component->dump_area[i] = NULL;
	}
err_malloc_read_addr:
	dma_free_coherent(rpaf_info->dev, component->write_size,
				dsp_component->write_area, component->write_addr);
	component->write_addr = 0;
	dsp_component->write_area = NULL;
err_malloc_write_addr:
err_set_misc_lib_pcm_params:
	kfree(component_config);
	return ret;
}

static int snd_soc_rpaf_misc_set_pcm_params_to_user(struct snd_dsp_component *dsp_component,
				struct snd_soc_dsp_pcm_params __user *_params)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;

	dev_err(rpaf_info->dev, "%s line:%d\n", __func__, __LINE__);
	return 0;
}

static int snd_soc_rpaf_misc_common_set_algo(struct snd_dsp_component *dsp_component,
					void __user *arg)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct snd_soc_dsp_component_config __user *_component_config = arg;
	struct snd_soc_dsp_component_config component_config;
	int ret;

	if (copy_from_user(&component_config, _component_config,
		sizeof(struct snd_soc_dsp_component_config))) {
		dev_err(rpaf_info->dev, "cannot copy component_config.\n");
		return -EFAULT;
	}

	soc_component->comp_mode = component_config.comp_mode;
	soc_component->component_type = component_config.component_type;
	strcpy(soc_component->params.driver, component_config.pcm_params.driver);
	soc_component->transfer_type = component_config.transfer_type;
	soc_component->sort_index = component_config.sort_index;
	soc_component->used = component_config.component_used;
	soc_component->params.stream = component_config.pcm_params.stream;

	dev_dbg(rpaf_info->dev, "%s:Line:%d, driver:%s\n"
		"component_config.comp_mode:%d, soc_component->comp_mode:%d\n"
		"component_config.component_type:%d, soc_component->component_type:%d\n"
		"component_config.sort_index:%d, soc_componen->sort_index:%d\n"
		"component_config.pcm_params.stream:%d, soc_component->params.stream:%d\n"
		"component_config.component_used:%d, soc_component->used:%d.\n",
		__func__, __LINE__, soc_component->params.driver,
		component_config.comp_mode, soc_component->comp_mode,
		component_config.component_type, soc_component->component_type,
		component_config.sort_index, soc_component->sort_index,
		component_config.pcm_params.stream, soc_component->params.stream,
		component_config.component_used, soc_component->used);

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_ALGO_SET;
	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	return 0;
}

static int snd_soc_rpaf_misc_common_get_algo(struct snd_dsp_component *dsp_component,
					void __user *arg)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct snd_soc_dsp_component_config __user *_component_config = arg;
	struct snd_soc_dsp_component_config component_config;
	int ret;

	if (copy_from_user(&component_config, _component_config,
		sizeof(struct snd_soc_dsp_component_config))) {
		dev_err(rpaf_info->dev, "cannot copy component_config.\n");
		return -EFAULT;
	}

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_ALGO_GET;
	soc_component->comp_mode = SND_DSP_COMPONENT_MODE_ALGO;
	soc_component->component_type = component_config.component_type;
	strcpy(soc_component->params.driver, component_config.pcm_params.driver);
	soc_component->transfer_type = component_config.transfer_type;
	soc_component->sort_index = component_config.sort_index;
	soc_component->used = component_config.component_used;
	soc_component->params.stream = component_config.pcm_params.stream;

	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	dev_dbg(rpaf_info->dev, "%s:Line:%d, driver:%s\n"
		"component_config.comp_mode:%d, soc_component->comp_mode:%d\n"
		"component_config.component_type:%d, soc_component->component_type:%d\n"
		"component_config.sort_index:%d, soc_componen->sort_index:%d\n"
		"component_config.pcm_params.stream:%d, soc_component->params.stream:%d\n"
		"component_config.component_used:%d, soc_component->used:%d.\n",
		__func__, __LINE__, soc_component->params.driver,
		component_config.comp_mode, soc_component->comp_mode,
		component_config.component_type, soc_component->component_type,
		component_config.sort_index, soc_component->sort_index,
		component_config.pcm_params.stream, soc_component->params.stream,
		component_config.component_used, soc_component->used);

	component_config.pcm_params.stream = soc_component->params.stream;
	component_config.comp_mode = soc_component->comp_mode;
	component_config.component_type = soc_component->component_type;
	component_config.sort_index = soc_component->sort_index;
	strcpy(component_config.pcm_params.driver, soc_component->params.driver);

	if (copy_to_user(_component_config, &component_config,
		sizeof(struct snd_soc_dsp_component_config))) {
		dev_err(rpaf_info->dev, "cannot copy component_config.\n");
		return -EFAULT;
	}

	return 0;
}

static int snd_soc_rpaf_misc_common_create(struct snd_dsp_component *dsp_component,
					void __user *arg)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	int i = 0;
	int result = 0;

	/* 判断当前状态 */
	switch (dsp_component->state) {
	case SND_DSP_COMPONENT_STATE_CREATE:
	case SND_DSP_COMPONENT_STATE_SETUP:
	case SND_DSP_COMPONENT_STATE_START:
	case SND_DSP_COMPONENT_STATE_RUNNING:
	case SND_DSP_COMPONENT_STATE_STOP:
	case SND_DSP_COMPONENT_STATE_REMOVE:
	case SND_DSP_COMPONENT_STATE_CLOSE:
	default:
		return -EINVAL;
	case SND_DSP_COMPONENT_STATE_OPEN:
		break;
	}

	/* 检查下是否有设置ID */
	mutex_lock(&comp_id_mutex);
	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		if (!component_id[i]) {
			soc_component->id = i;
			component_id[i] = 1;
			break;
		}
	}
	mutex_unlock(&comp_id_mutex);

	if (soc_component->id != i) {
		awrpaf_err("component id num [%d] set id:%u failed, should be less than %lu!\n",
				i, soc_component->id, RPAF_COMPONENT_MAX_NUM);
		return -EINVAL;
	}

	/* 状态跳转 */
	dsp_component->state = SND_DSP_COMPONENT_STATE_CREATE;

	/* 状态跳转SETUP */
	result = snd_soc_rpaf_misc_set_component_config_from_user(dsp_component, arg);
	if (result < 0) {
		awrpaf_err("component[%u] setting pcm_params failed!\n", soc_component->id);
		return result;
	}

	mutex_lock(&comp_id_mutex);
	component_id[soc_component->id] = 1;
	mutex_unlock(&comp_id_mutex);

	if (soc_component->comp_mode == SND_DSP_COMPONENT_MODE_STREAM) {
		return result;
	}

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_CREATE;
	/* 发送给msgbox to dsp */
	result = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	if (result < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, result);
		return result;
	}

	return result;
}

static int snd_soc_rpaf_misc_common_remove(struct snd_dsp_component *dsp_component)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct snd_dsp_component *stream_dsp_component = NULL;
	int ret;

	/* 判断当前状态 */
	switch (dsp_component->state) {
	case SND_DSP_COMPONENT_STATE_CREATE:
	case SND_DSP_COMPONENT_STATE_START:
	case SND_DSP_COMPONENT_STATE_RUNNING:
	case SND_DSP_COMPONENT_STATE_REMOVE:
	case SND_DSP_COMPONENT_STATE_CLOSE:
	default:
		return -EINVAL;
	case SND_DSP_COMPONENT_STATE_OPEN:
	case SND_DSP_COMPONENT_STATE_SETUP:
	case SND_DSP_COMPONENT_STATE_STOP:
		break;
	}

	mutex_lock(&comp_id_mutex);
	component_id[soc_component->id] = 0;
	mutex_unlock(&comp_id_mutex);

	if (soc_component->comp_mode == SND_DSP_COMPONENT_MODE_STREAM) {

		snd_soc_rpaf_pcm_stream_component_lock();

		stream_dsp_component = dsp_component->private_data;
		if (stream_dsp_component && stream_dsp_component->private_data) {
			stream_dsp_component->private_data = NULL;
			dsp_component->private_data = NULL;
		}

		snd_soc_rpaf_pcm_stream_component_unlock();

		dsp_component->state = SND_DSP_COMPONENT_STATE_REMOVE;
		return 0;
	}

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_REMOVE;
	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	/* 状态跳转 */
	dsp_component->state = SND_DSP_COMPONENT_STATE_REMOVE;
	return 0;
}

static int snd_soc_rpaf_misc_common_status(struct snd_dsp_component *dsp_component)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;

	dev_err(rpaf_info->dev, "%s line:%d\n", __func__, __LINE__);
	/* 发送/不发送给msgbox to dsp */
	return 0;
}

static void snd_soc_rpaf_runtime_prepare(struct snd_soc_rpaf_pcm_runtime *runtime)
{
	runtime->pos = 0;
	runtime->hw_ptr = 0;
	runtime->appl_ptr = 0;

	/* Position at buffer restart */
	runtime->hw_ptr_base = 0;
	/* Position at interrupt time */
	runtime->hw_ptr_interrupt = 0;
	/* offset for hw_ptr due to boundary wrap-around */
	runtime->hw_ptr_wrap = 0;
}

static int snd_soc_rpaf_misc_common_start(struct snd_dsp_component *dsp_component)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	int ret;

	/* 判断当前状态 */
	switch (dsp_component->state) {
	case SND_DSP_COMPONENT_STATE_OPEN:
	case SND_DSP_COMPONENT_STATE_CREATE:
	case SND_DSP_COMPONENT_STATE_START:
	case SND_DSP_COMPONENT_STATE_RUNNING:
	case SND_DSP_COMPONENT_STATE_REMOVE:
	case SND_DSP_COMPONENT_STATE_CLOSE:
	default:
		return -EINVAL;
	case SND_DSP_COMPONENT_STATE_SETUP:
	case SND_DSP_COMPONENT_STATE_STOP:
		break;
	}

	if (soc_component->comp_mode == SND_DSP_COMPONENT_MODE_STREAM) {
		snd_soc_rpaf_pcm_stream_lock_irq(dsp_component);
		/* 复位指针位置 */
		snd_soc_rpaf_runtime_prepare(runtime);
		snd_soc_rpaf_pcm_stream_unlock_irq(dsp_component);
	}

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_START;
	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	/* 状态跳转 */
	dsp_component->state = SND_DSP_COMPONENT_STATE_START;
	return 0;
}

static int snd_soc_rpaf_misc_common_stop(struct snd_dsp_component *dsp_component)
{
	struct snd_soc_rpaf_info *rpaf_info = dsp_component->rpaf_info;
	struct msg_component_package *msg_component = &dsp_component->msg_component;
	struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;
	struct snd_soc_rpaf_pcm_runtime *runtime = &dsp_component->runtime;
	int ret;

	/* 判断当前状态 */
	switch (dsp_component->state) {
	case SND_DSP_COMPONENT_STATE_OPEN:
	case SND_DSP_COMPONENT_STATE_CREATE:
	case SND_DSP_COMPONENT_STATE_SETUP:
	case SND_DSP_COMPONENT_STATE_STOP:
	case SND_DSP_COMPONENT_STATE_REMOVE:
	case SND_DSP_COMPONENT_STATE_CLOSE:
	default:
		return -EINVAL;
	case SND_DSP_COMPONENT_STATE_START:
	case SND_DSP_COMPONENT_STATE_RUNNING:
		break;
	}

	runtime->dump_start = 0;

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_STOP;
	/* 发送给msgbox to dsp */
	ret = sunxi_hifi_component_block_send(rpaf_info->dsp_id, msg_component);
	if (ret < 0) {
		pr_err("%s line:%d error:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	/* 状态跳转 */
	dsp_component->state = SND_DSP_COMPONENT_STATE_STOP;
	return 0;
}

static long snd_soc_rpaf_misc_common_ioctl(struct file *file,
				      struct snd_dsp_component *dsp_component,
				      unsigned int cmd,
				      void __user *arg)
{
	int result;
	struct snd_soc_rpaf_info *rpaf_info;

	if (IS_ERR_OR_NULL(dsp_component))
		return -ENXIO;

	rpaf_info = dsp_component->rpaf_info;

	switch (cmd) {
	case SND_SOC_DSP_COMPONENT_IOCTL_ALGO_SET:
		result = snd_soc_rpaf_misc_common_set_algo(dsp_component, arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_ALGO_GET:
		result = snd_soc_rpaf_misc_common_get_algo(dsp_component, arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_CREATE:
		result = snd_soc_rpaf_misc_common_create(dsp_component, arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_REMOVE:
		result = snd_soc_rpaf_misc_common_remove(dsp_component);
		put_user(result, (int __user *)arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_STATUS:
		result = snd_soc_rpaf_misc_common_status(dsp_component);
		put_user(result, (int __user *)arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_SW_PARAMS:
		result = snd_soc_rpaf_misc_set_pcm_params_to_user(dsp_component, arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_START:
		result = snd_soc_rpaf_misc_common_start(dsp_component);
		put_user(result, (int __user *)arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_STOP:
		result = snd_soc_rpaf_misc_common_stop(dsp_component);
		put_user(result, (int __user *)arg);
		break;
	case SND_SOC_DSP_COMPONENT_IOCTL_READ32:
	{
		struct snd_rpaf_xferi xferi;
		struct snd_rpaf_xferi32 __user *_xferi32 = arg;
		int result;
		compat_caddr_t buf;

		get_user(xferi.result, &_xferi32->result);
		get_user(buf, &_xferi32->input_buf);
		xferi.input_buf = compat_ptr(buf);
		get_user(xferi.input_length, &_xferi32->input_length);
		get_user(buf, &_xferi32->output_buf);
		xferi.output_buf = compat_ptr(buf);
		get_user(xferi.output_length, &_xferi32->output_length);
		get_user(buf, &_xferi32->dump_buf);
		xferi.dump_buf = compat_ptr(buf);
		get_user(xferi.dump_length, &_xferi32->dump_length);
		get_user(xferi.dump_type, &_xferi32->dump_type);

		result = snd_soc_rpaf_misc_lib_read(dsp_component, &xferi);

		__put_user(result, &_xferi32->result);
		return result < 0 ? result : 0;

	}
	case SND_SOC_DSP_COMPONENT_IOCTL_READ:
	{
		struct snd_rpaf_xferi xferi;
		struct snd_rpaf_xferi __user *_xferi = arg;

		if (put_user(0, &_xferi->result))
			return -EFAULT;

		if (copy_from_user(&xferi, _xferi, sizeof(struct snd_rpaf_xferi)))
			return -EFAULT;

		result = snd_soc_rpaf_misc_lib_read(dsp_component, &xferi);
		if (copy_to_user(_xferi, &xferi, sizeof(*_xferi)))
			return -EFAULT;
		__put_user(result, &_xferi->result);
		return result < 0 ? result : 0;
	}
	case SND_SOC_DSP_COMPONENT_IOCTL_WRITE:
	{
		struct snd_rpaf_xferi xferi;
		struct snd_rpaf_xferi __user *_xferi = arg;
		struct msg_component_package *msg_component = &dsp_component->msg_component;
		struct snd_soc_dsp_component *component = &msg_component->soc_component;
		int result;

		if (put_user(0, &_xferi->result))
			return -EFAULT;
		if (copy_from_user(&xferi, _xferi, sizeof(*_xferi)))
			return -EFAULT;

		dsp_component->state = SND_DSP_COMPONENT_STATE_RUNNING;

		component->write_length = xferi.input_length;
		component->read_length = xferi.output_length;
		result = snd_soc_rpaf_misc_lib_write(dsp_component, xferi.input_buf,
						xferi.input_length);
		if (result < 0) {
			__put_user(result, &_xferi->result);
			return result < 0 ? result : 0;
		}
		/* 回写回来 */
		result = snd_soc_rpaf_write_to_user(dsp_component, xferi.output_buf,
						&xferi.output_length);
		if (copy_to_user(_xferi, &xferi, sizeof(*_xferi)))
			return -EFAULT;
		__put_user(result, &_xferi->result);
		return result < 0 ? result : 0;
	}
	case SND_SOC_DSP_COMPONENT_IOCTL_WRITE32:
	{
		struct snd_rpaf_xferi xferi;
		struct snd_rpaf_xferi32 __user *_xferi32 = arg;
		struct msg_component_package *msg_component = &dsp_component->msg_component;
		struct snd_soc_dsp_component *component = &msg_component->soc_component;
		int result;
		compat_caddr_t buf;

		get_user(xferi.result, &_xferi32->result);
		get_user(buf, &_xferi32->input_buf);
		xferi.input_buf = compat_ptr(buf);
		get_user(xferi.input_length, &_xferi32->input_length);
		get_user(buf, &_xferi32->output_buf);
		xferi.output_buf = compat_ptr(buf);
		get_user(xferi.output_length, &_xferi32->output_length);
		get_user(buf, &_xferi32->dump_buf);
		xferi.dump_buf = compat_ptr(buf);
		get_user(xferi.dump_length, &_xferi32->dump_length);
		get_user(xferi.dump_type, &_xferi32->dump_type);

		dsp_component->state = SND_DSP_COMPONENT_STATE_RUNNING;

		component->write_length = xferi.input_length;
		component->read_length = xferi.output_length;
		result = snd_soc_rpaf_misc_lib_write(dsp_component, xferi.input_buf,
						xferi.input_length);
		if (result < 0) {
			__put_user(result, &_xferi32->result);
			return result < 0 ? result : 0;
		}
		/* 回写回来 */
		result = snd_soc_rpaf_write_to_user(dsp_component, xferi.output_buf,
						&xferi.output_length);
		__put_user(result, &_xferi32->result);
		__put_user(xferi.output_length, &_xferi32->output_length);

		return result < 0 ? result : 0;
	}
	default:
		dev_err(rpaf_info->dev, "%s cmd:%d not exist.\n", __func__, cmd);
		return -EINVAL;
	}

	return result;
}

static long snd_soc_rpaf_misc_compat_ioctl(struct file *file,
					  unsigned int cmd,
					  unsigned long arg)
{
	struct snd_dsp_component *dsp_component = file->private_data;
	void __user *argp = compat_ptr(arg);

	if (((cmd >> 8) & 0xFF) != 'C')
		return -ENOTTY;

	return snd_soc_rpaf_misc_common_ioctl(file, dsp_component, cmd, argp);
}

static long snd_soc_rpaf_misc_unlocked_ioctl(struct file *file,
					  unsigned int cmd,
					  unsigned long arg)
{
	struct snd_dsp_component *dsp_component = file->private_data;

	if (((cmd >> 8) & 0xFF) != 'C')
		return -ENOTTY;

	return snd_soc_rpaf_misc_common_ioctl(file, dsp_component, cmd,
					(void __user *)arg);
}

static const struct file_operations snd_soc_rpaf_misc_f_ops = {
	.owner = THIS_MODULE,
	.open = snd_soc_rpaf_misc_open,
	.unlocked_ioctl = snd_soc_rpaf_misc_unlocked_ioctl,
	.compat_ioctl = snd_soc_rpaf_misc_compat_ioctl,
//	.read = snd_soc_rpaf_misc_read,
	.write = snd_soc_rpaf_misc_write,
	.llseek = no_llseek,
	.release = snd_soc_rpaf_misc_release,
};

static struct miscdevice snd_soc_rpaf_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi-rpaf-dsp",
	.fops = &snd_soc_rpaf_misc_f_ops,
};

int snd_soc_rpaf_misc_register_device(struct device *dev, unsigned int dsp_id)
{
	char *str_name = NULL;
	struct snd_soc_rpaf_info *rpaf_info = NULL;
	int ret = 0;

	if (IS_ERR_OR_NULL(dev)) {
		pr_alert("[%s] dev id:%u is null.\n", __func__, dsp_id);
		return -EFAULT;
	}

	if (snd_soc_rpaf_info_get_from_list_by_dspid(dsp_id) != NULL) {
		dev_err(dev, "the dsp_id had been register!\n");
		return -EINVAL;
	}

	rpaf_info = devm_kzalloc(dev, sizeof(struct snd_soc_rpaf_info),
				GFP_KERNEL);
	if (IS_ERR_OR_NULL(rpaf_info)) {
		dev_err(dev, "kzalloc for snd_soc_rpaf_info failed.\n");
		return -ENOMEM;
	}
	rpaf_info->dev = dev;
	rpaf_info->dsp_id = dsp_id;
	memset(rpaf_info->name, 0, sizeof(rpaf_info->name));
	snprintf(rpaf_info->name, sizeof(rpaf_info->name),
		 "rpaf_misc%d", rpaf_info->dsp_id);

	snd_soc_rpaf_info_set_drvdata(rpaf_info, dev_get_drvdata(rpaf_info->dev));

	memcpy(&rpaf_info->misc_dev, &snd_soc_rpaf_misc_dev,
		sizeof(struct miscdevice));
	str_name = devm_kzalloc(dev, SUNXI_RPAF_INFO_NAME_LEN, GFP_KERNEL);
	if (IS_ERR_OR_NULL(str_name)) {
		dev_err(dev, "failed to kzalloc attr group name!\n");
		ret = -ENOMEM;
		goto err_kzalloc_misc_dev_name;
	}
	memset(str_name, 0, SUNXI_RPAF_INFO_NAME_LEN);
	snprintf(str_name, SUNXI_RPAF_INFO_NAME_LEN,
		 "rpaf-dsp%d", rpaf_info->dsp_id);
	rpaf_info->misc_dev.name = str_name;

	ret = misc_register(&rpaf_info->misc_dev);
	if (ret < 0) {
		dev_err(dev, "sunxi-rpaf register driver as misc device error!\n");
		goto err_misc_register;
	}

	snd_soc_rpaf_info_list_add_tail(rpaf_info);

	dev_info(dev, "register device finished!\n");

	return 0;
err_misc_register:
	if (!IS_ERR_OR_NULL(rpaf_info->misc_dev.name)) {
		str_name = (char *)rpaf_info->misc_dev.name;
		devm_kfree(dev, str_name);
	}
err_kzalloc_misc_dev_name:
	devm_kfree(dev, rpaf_info);
	return ret;
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_misc_register_device);

int snd_soc_rpaf_misc_deregister_device(struct device *dev, unsigned int dsp_id)
{
	struct snd_soc_rpaf_info *rpaf_info = NULL;
	int ret = 0;

	if (IS_ERR_OR_NULL(dev)) {
		pr_alert("[%s] dev id:%u is null.\n", __func__, dsp_id);
		return -EFAULT;
	}

	rpaf_info = snd_soc_rpaf_info_get_from_list_by_dspid(dsp_id);
	if (IS_ERR_OR_NULL(rpaf_info)) {
		dev_err(dev, "the dsp_id had been del from rpaf_info list.!\n");
		return -EINVAL;
	}

	misc_deregister(&rpaf_info->misc_dev);
	if (!IS_ERR_OR_NULL(rpaf_info->misc_dev.name))
		devm_kfree(dev, (char *)rpaf_info->misc_dev.name);

	snd_soc_rpaf_info_list_del(rpaf_info);

	dev_info(dev, "deregister device finished!\n");

	devm_kfree(dev, rpaf_info);

	return ret;
}
EXPORT_SYMBOL_GPL(snd_soc_rpaf_misc_deregister_device);

MODULE_DESCRIPTION("sunxi dsp remote processor audio framework core");
MODULE_AUTHOR("yumingfeng@allwinnertech.com");
MODULE_LICENSE("GPL");
