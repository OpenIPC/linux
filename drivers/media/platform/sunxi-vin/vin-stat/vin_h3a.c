/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-stat/vin_h3a.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/slab.h>
#include <linux/uaccess.h>

#include "vin_h3a.h"

#include "../vin-isp/sunxi_isp.h"
#include "../vin-video/vin_video.h"
#include <linux/mtd/spi-nor.h>

static struct ispstat_buffer *__isp_stat_buf_find(struct isp_stat *stat, int look_empty)
{
	struct ispstat_buffer *found = NULL;
	int i;

	for (i = 0; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *curr = &stat->buf[i];

		/*
		 * Don't select the buffer which is being copied to
		 * userspace or used by the module.
		 */
		if (curr == stat->locked_buf || curr == stat->active_buf)
			continue;

		/* Don't select uninitialised buffers if it's not required */
		if (!look_empty && curr->empty)
			continue;

#if !defined ISP_600
		if (curr->empty) {
			found = curr;
			break;
		}

		if (!found ||
		    (s32)curr->frame_number - (s32)found->frame_number < 0)
			found = curr;
#else
		if (curr->empty && (curr->state == ISPSTAT_IDLE)) {
			found = curr;
			break;
		}

		if ((!found || (s32)curr->frame_number - (s32)found->frame_number < 0)
			&& (curr->state == ISPSTAT_READY))
			found = curr;
#endif
	}

	return found;
}

static inline struct ispstat_buffer *isp_stat_buf_find_oldest(struct isp_stat *stat)
{
	return __isp_stat_buf_find(stat, 0);
}

static inline struct ispstat_buffer *isp_stat_buf_find_oldest_or_empty(struct isp_stat *stat)
{
	return __isp_stat_buf_find(stat, 1);
}

/* Get next free buffer to write the statistics to and mark it active. */
static void isp_stat_buf_next(struct isp_stat *stat)
{
	if (unlikely(stat->active_buf)) {
		/* Overwriting unused active buffer */
#if !defined CONFIG_ISP_SERVER_MELIS
		vin_log(VIN_LOG_STAT, "%s: new buffer requested without queuing active one.\n",	stat->sd.name);
#else
		vin_log(VIN_LOG_STAT, "new buffer requested without queuing active one.\n");
#endif
	} else {
		stat->active_buf = isp_stat_buf_find_oldest_or_empty(stat);
	}
}

static void isp_stat_buf_release(struct isp_stat *stat)
{
	unsigned long flags;

	spin_lock_irqsave(&stat->isp->slock, flags);
	stat->locked_buf->state = ISPSTAT_IDLE;
	stat->locked_buf = NULL;
	spin_unlock_irqrestore(&stat->isp->slock, flags);
}

#if !defined CONFIG_ISP_SERVER_MELIS
/* Get buffer to userspace. */
static struct ispstat_buffer *isp_stat_buf_get(struct isp_stat *stat, struct vin_isp_stat_data *data)
{
	int rval = 0;
	unsigned long flags;
	struct ispstat_buffer *buf;

	spin_lock_irqsave(&stat->isp->slock, flags);

	while (1) {
		buf = isp_stat_buf_find_oldest(stat);
		if (!buf) {
			spin_unlock_irqrestore(&stat->isp->slock, flags);
			vin_log(VIN_LOG_STAT, "%s: cannot find a buffer.\n", stat->sd.name);
			return ERR_PTR(-EBUSY);
		}
		break;
	}

	stat->locked_buf = buf;

	spin_unlock_irqrestore(&stat->isp->slock, flags);
	if (NULL != data) {
#if !defined ISP_600
		if (buf->buf_size > data->buf_size) {
			vin_warn("%s: userspace's buffer size is not enough.\n", stat->sd.name);
			isp_stat_buf_release(stat);
			return ERR_PTR(-EINVAL);
		}

		rval = copy_to_user(data->buf, buf->virt_addr, buf->buf_size);

		if (rval) {
			vin_warn("%s: failed copying %d bytes of stat data\n", stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
		}
#else
		if ((buf->buf_size + ISP_SAVE_LOAD_STATISTIC_SIZE) > data->buf_size) {
			vin_warn("%s: userspace's buffer size is not enough.\n", stat->sd.name);
			isp_stat_buf_release(stat);
			return ERR_PTR(-EINVAL);
		}

		rval = copy_to_user(data->buf, buf->virt_addr, buf->buf_size);
		if (rval) {
			vin_warn("%s: failed copying %d bytes of stat data\n", stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
			return buf;
		}

		rval = copy_to_user(data->buf + buf->buf_size, stat->isp->isp_save_load.vir_addr + ISP_SAVE_LOAD_REG_SIZE, ISP_SAVE_LOAD_STATISTIC_SIZE);
		if (rval) {
			vin_warn("%s: failed copying %d bytes of save_stat data\n", stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
			return buf;
		}
#endif
	}
	return buf;
}

static void isp_stat_bufs_free(struct isp_stat *stat)
{
	int i;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];
		mm->size = stat->buf_size;

		if (!buf->virt_addr)
			continue;

		mm->vir_addr = buf->virt_addr;
		mm->dma_addr = buf->dma_addr;
		os_mem_free(&stat->isp->pdev->dev, mm);

		buf->dma_addr = NULL;
		buf->virt_addr = NULL;
		buf->empty = 1;
	}

	vin_log(VIN_LOG_STAT, "%s: all buffers were freed.\n", stat->sd.name);

	stat->buf_size = 0;
	stat->active_buf = NULL;
}

static int isp_stat_bufs_alloc(struct isp_stat *stat, u32 size, u32 count)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&stat->isp->slock, flags);

	BUG_ON(stat->locked_buf != NULL);

	for (i = 1; i < stat->buf_cnt; i++) {
		stat->buf[i].empty = 1;
		stat->buf[i].state = ISPSTAT_IDLE;
	}

	/* Are the old buffers big enough? */
	if ((stat->buf_size >= size) && (stat->buf_cnt == count)) {
		spin_unlock_irqrestore(&stat->isp->slock, flags);
		vin_log(VIN_LOG_STAT, "%s: old stat buffers are enough.\n", stat->sd.name);
		return 0;
	}

	spin_unlock_irqrestore(&stat->isp->slock, flags);

	isp_stat_bufs_free(stat);

	stat->buf_size = size;
	stat->buf_cnt = count;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];
		mm->size = size;
		if (!os_mem_alloc(&stat->isp->pdev->dev, mm)) {
			buf->virt_addr = mm->vir_addr;
			buf->dma_addr = mm->dma_addr;
		}
		if (!buf->virt_addr || !buf->dma_addr) {
			vin_err("%s: Can't acquire memory for DMA buffer %d\n",	stat->sd.name, i);
			isp_stat_bufs_free(stat);
			return -ENOMEM;
		}
		buf->empty = 1;
		buf->state = ISPSTAT_IDLE;
	}
	return 0;
}

static void isp_stat_queue_event(struct isp_stat *stat, int err)
{
	struct video_device *vdev = stat->sd.devnode;
	struct v4l2_event event;
	struct vin_isp_stat_event_status *status = (void *)event.u.data;

	memset(&event, 0, sizeof(event));
	if (!err)
		status->frame_number = stat->frame_number;
	else
		status->buf_err = 1;

	event.type = stat->event_type;
	v4l2_event_queue(vdev, &event);
}

int isp_stat_request_statistics(struct isp_stat *stat,
				     struct vin_isp_stat_data *data)
{
	struct ispstat_buffer *buf;

	if (stat->state != ISPSTAT_ENABLED) {
		vin_log(VIN_LOG_STAT, "%s: engine not enabled.\n", stat->sd.name);
		return -EINVAL;
	}
	vin_log(VIN_LOG_STAT, "user wants to request statistics.\n");

	mutex_lock(&stat->ioctl_lock);
	buf = isp_stat_buf_get(stat, data);
	if (IS_ERR(buf)) {
		mutex_unlock(&stat->ioctl_lock);
		return PTR_ERR(buf);
	}

	data->frame_number = buf->frame_number;
	data->buf_size = buf->buf_size;

	buf->empty = 1;
	isp_stat_buf_release(stat);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

int isp_stat_config(struct isp_stat *stat, void *new_conf)
{
	int ret;
	u32 count;
	struct vin_isp_h3a_config *user_cfg = new_conf;

	if (!new_conf) {
		vin_log(VIN_LOG_STAT, "%s: configuration is NULL\n", stat->sd.name);
		return -EINVAL;
	}

	mutex_lock(&stat->ioctl_lock);

	user_cfg->buf_size = ISP_STAT_TOTAL_SIZE;

#if defined ISP_600
	if (stat->sensor_fps <= 30)
		count = 3;
	else if (stat->sensor_fps <= 60)
		count = 4;
	else if (stat->sensor_fps <= 120)
		count = 5;
	else
		count = 6;
#else
	if (stat->sensor_fps <= 30)
		count = 2;
	else if (stat->sensor_fps <= 60)
		count = 3;
	else if (stat->sensor_fps <= 120)
		count = 4;
	else
		count = 5;
#endif
	ret = isp_stat_bufs_alloc(stat, user_cfg->buf_size, count);
	if (ret) {
		mutex_unlock(&stat->ioctl_lock);
		return ret;
	}

	/* Module has a valid configuration. */
	stat->configured = 1;

	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

int isp_stat_enable(struct isp_stat *stat, u8 enable)
{
	unsigned long irqflags;

	vin_log(VIN_LOG_STAT, "%s: user wants to %s module.\n", stat->sd.name, enable ? "enable" : "disable");

	/* Prevent enabling while configuring */
	mutex_lock(&stat->ioctl_lock);

	spin_lock_irqsave(&stat->isp->slock, irqflags);

	if (!stat->configured && enable) {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		mutex_unlock(&stat->ioctl_lock);
		vin_log(VIN_LOG_STAT, "%s: cannot enable module as it's never been successfully configured so far.\n", stat->sd.name);
		return -EINVAL;
	}
	stat->stat_en_flag = enable;
	stat->isp->f1_after_librun = 0;

	if (enable)
		stat->state = ISPSTAT_ENABLED;
	else
		stat->state = ISPSTAT_DISABLED;

	isp_stat_buf_next(stat);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

#if defined ISP_600
static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	int ret = STAT_BUF_DONE;
	dma_addr_t dma_addr;

	if (buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		isp_stat_buf_next(stat);

		if (!stat->active_buf)
			return STAT_NO_BUF;

		dma_addr = (dma_addr_t)(stat->active_buf->dma_addr);
		bsp_isp_set_statistics_addr(stat->isp->id, dma_addr);
		if (bsp_isp_get_irq_status(stat->isp->id, PARA_LOAD_PD)) {
			stat->active_buf = NULL;
			vin_warn("para_load_pd, set active bufffer failed!\n");
		} else
			stat->active_buf->state = ISPSTAT_LOAD_SET;
	}

	return ret;
}

void isp_stat_load_set(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	int i;

	if (stat->state == ISPSTAT_DISABLED)
		return;

	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			//vin_warn("lost save_pd to get buffer\n");
			return;
		}
	}

	stat->isp->h3a_stat.frame_number++;

	isp_stat_buf_process(stat, ret);

#if 0
	for (i = 0; i < stat->buf_cnt; i++) {
		vin_print("load buffer%d stat is %d\n", i, stat->buf[i].state);
	}
#endif
}

void isp_stat_isr(struct isp_stat *stat)
{
	int ret = STAT_NO_BUF;
	unsigned long irqflags;
	int i;

	vin_log(VIN_LOG_STAT, "buf state is %d, frame number is %d 0x%x %d\n",
		stat->state, stat->frame_number, stat->buf_size, stat->configured);

	if (stat->state == ISPSTAT_DISABLED)
		return;

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			goto next;
		}
	}
	//vin_warn("lost load_pd to set buffer\n");
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	return;

next:
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET)
			stat->buf[i].state = ISPSTAT_SAVE_SET;
		else if (stat->buf[i].state == ISPSTAT_SAVE_SET) {
			stat->buf[i].state = ISPSTAT_READY;
			stat->buf[i].empty = 0;
			ret = STAT_BUF_DONE;
		}
		vin_log(VIN_LOG_STAT, "save buffer%d stat is %d\n", i, stat->buf[i].state);
	}
	if (ret != STAT_BUF_DONE)
		stat->isp->save_get_flag = 1;
	else
		stat->isp->save_get_flag = 0;

	isp_stat_buf_queue(stat);
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

	isp_stat_queue_event(stat, ret != STAT_BUF_DONE);
}
#else /* else not ISP_600*/
static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf->empty = 0;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	int ret = STAT_NO_BUF;
	dma_addr_t dma_addr;

	if (buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		ret = isp_stat_buf_queue(stat);
		isp_stat_buf_next(stat);

		if (!stat->active_buf)
			return STAT_NO_BUF;

		dma_addr = (dma_addr_t)(stat->active_buf->dma_addr);
		bsp_isp_set_statistics_addr(stat->isp->id, dma_addr);
	}

	return ret;
}

void isp_stat_isr(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	unsigned long irqflags;

	vin_log(VIN_LOG_STAT, "buf state is %d, frame number is %d 0x%x %d\n",
		stat->state, stat->frame_number, stat->buf_size, stat->configured);

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	if (stat->state == ISPSTAT_DISABLED) {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		return;
	}
	stat->isp->h3a_stat.frame_number++;

	ret = isp_stat_buf_process(stat, ret);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

	isp_stat_queue_event(stat, ret != STAT_BUF_DONE);
}
#endif

static long h3a_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_stat *stat = v4l2_get_subdevdata(sd);
	vin_log(VIN_LOG_STAT, "%s cmd is 0x%x\n", __func__, cmd);
	switch (cmd) {
	case VIDIOC_VIN_ISP_H3A_CFG:
		return isp_stat_config(stat, arg);
	case VIDIOC_VIN_ISP_STAT_REQ:
		return isp_stat_request_statistics(stat, arg);
	case VIDIOC_VIN_ISP_STAT_EN:
		return isp_stat_enable(stat, *(u8 *)arg);
	}

	return -ENOIOCTLCMD;
}

#ifdef CONFIG_COMPAT
struct vin_isp_stat_data32 {
	compat_caddr_t buf;
	__u32 buf_size;
	__u32 frame_number;
	__u32 config_counter;
};
struct vin_isp_h3a_config32 {
	__u32 buf_size;
	__u32 config_counter;
};

static int get_isp_statistics_buf32(struct vin_isp_stat_data *kp,
			      struct vin_isp_stat_data32 __user *up)
{
	u32 tmp;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_stat_data32)) ||
	    get_user(kp->buf_size, &up->buf_size) ||
	    get_user(kp->frame_number, &up->frame_number) ||
	    get_user(kp->config_counter, &up->config_counter) ||
	    get_user(tmp, &up->buf))
		return -EFAULT;
	kp->buf = compat_ptr(tmp);
	return 0;
}

static int put_isp_statistics_buf32(struct vin_isp_stat_data *kp,
			      struct vin_isp_stat_data32 __user *up)
{
	u32 tmp = (u32) ((unsigned long)kp->buf);

	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_stat_data32)) ||
	    put_user(kp->buf_size, &up->buf_size) ||
	    put_user(kp->frame_number, &up->frame_number) ||
	    put_user(kp->config_counter, &up->config_counter) ||
	    put_user(tmp, &up->buf))
		return -EFAULT;
	return 0;
}

static int get_isp_statistics_config32(struct vin_isp_h3a_config *kp,
			      struct vin_isp_h3a_config32 __user *up)
{
	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_h3a_config32)) ||
	    get_user(kp->buf_size, &up->buf_size) ||
	    get_user(kp->config_counter, &up->config_counter))
		return -EFAULT;
	return 0;
}

static int put_isp_statistics_config32(struct vin_isp_h3a_config *kp,
			      struct vin_isp_h3a_config32 __user *up)
{
	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_h3a_config32)) ||
	    put_user(kp->buf_size, &up->buf_size) ||
	    put_user(kp->config_counter, &up->config_counter))
		return -EFAULT;
	return 0;
}

static int get_isp_statistics_enable32(unsigned int *kp,
			      unsigned int __user *up)
{
	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_h3a_config32)) ||
	    get_user(*kp, up))
		return -EFAULT;
	return 0;
}

static int put_isp_statistics_enable32(unsigned int *kp,
			      unsigned int __user *up)
{
	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_h3a_config32)) ||
	    put_user(*kp, up))
		return -EFAULT;
	return 0;
}

#define VIDIOC_VIN_ISP_H3A_CFG32 _IOWR('V', BASE_VIDIOC_PRIVATE + 31, struct vin_isp_h3a_config32)
#define VIDIOC_VIN_ISP_STAT_REQ32 _IOWR('V', BASE_VIDIOC_PRIVATE + 32, struct vin_isp_stat_data32)
#define VIDIOC_VIN_ISP_STAT_EN32 _IOWR('V', BASE_VIDIOC_PRIVATE + 33, unsigned int)

static long h3a_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg)
{
	union {
		struct vin_isp_h3a_config isb;
		struct vin_isp_stat_data isd;
		unsigned int isu;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	vin_log(VIN_LOG_STAT, "%s cmd is 0x%x\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_VIN_ISP_STAT_REQ32:
		cmd = VIDIOC_VIN_ISP_STAT_REQ;
		break;
	case VIDIOC_VIN_ISP_H3A_CFG32:
		cmd = VIDIOC_VIN_ISP_H3A_CFG;
		break;
	case VIDIOC_VIN_ISP_STAT_EN32:
		cmd = VIDIOC_VIN_ISP_STAT_EN;
		break;
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_STAT_REQ:
		err = get_isp_statistics_buf32(&karg.isd, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ISP_H3A_CFG:
		err = get_isp_statistics_config32(&karg.isb, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ISP_STAT_EN:
		err = get_isp_statistics_enable32(&karg.isu, up);
		compatible_arg = 0;
		break;
	}

	if (err)
		return err;

	if (compatible_arg)
		err = h3a_ioctl(sd, cmd, up);
	else {
		mm_segment_t old_fs = get_fs();

		set_fs(KERNEL_DS);
		err = h3a_ioctl(sd, cmd, &karg);
		set_fs(old_fs);
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_STAT_REQ:
		err = put_isp_statistics_buf32(&karg.isd, up);
		break;
	case VIDIOC_VIN_ISP_H3A_CFG:
		err = put_isp_statistics_config32(&karg.isb, up);
		break;
	case VIDIOC_VIN_ISP_STAT_EN:
		err = put_isp_statistics_enable32(&karg.isu, up);
		compatible_arg = 0;
		break;
	}

	return err;
}
#endif

int isp_stat_subscribe_event(struct v4l2_subdev *subdev,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	struct isp_stat *stat = v4l2_get_subdevdata(subdev);

	if (sub->type != stat->event_type)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, STAT_NEVENTS, NULL);
}

static const struct v4l2_subdev_core_ops h3a_subdev_core_ops = {
	.ioctl = h3a_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = h3a_compat_ioctl32,
#endif
	.subscribe_event = isp_stat_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops h3a_subdev_ops = {
	.core = &h3a_subdev_core_ops,
};

int vin_isp_h3a_init(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	stat->isp = isp;
	stat->event_type = V4L2_EVENT_VIN_H3A;

	mutex_init(&stat->ioctl_lock);

	v4l2_subdev_init(&stat->sd, &h3a_subdev_ops);
	snprintf(stat->sd.name, V4L2_SUBDEV_NAME_SIZE, "sunxi_h3a.%u", isp->id);
	stat->sd.grp_id = VIN_GRP_ID_STAT;
	stat->sd.flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	v4l2_set_subdevdata(&stat->sd, stat);

	stat->pad.flags = MEDIA_PAD_FL_SINK;
	stat->sd.entity.function = MEDIA_ENT_F_PROC_VIDEO_STATISTICS;

	return media_entity_pads_init(&stat->sd.entity, 1, &stat->pad);
}

void vin_isp_h3a_cleanup(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	media_entity_cleanup(&stat->sd.entity);
	mutex_destroy(&stat->ioctl_lock);
	isp_stat_bufs_free(stat);
}
#else //CONFIG_ISP_SERVER_MELIS

int isp_stat_enable(struct isp_stat *stat, u8 enable) //disable in isp stream_off
{
	unsigned long irqflags;

	vin_log(VIN_LOG_STAT, "melis wants to %s module.\n", enable ? "enable" : "disable");

	mutex_lock(&stat->ioctl_lock);
	spin_lock_irqsave(&stat->isp->slock, irqflags);

	stat->stat_en_flag = enable;
	stat->isp->f1_after_librun = 0;

	if (enable)
		stat->state = ISPSTAT_ENABLED;
	else
		stat->state = ISPSTAT_DISABLED;

	isp_stat_buf_next(stat);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

static void isp_stat_bufs_free(struct isp_stat *stat)
{
	int i;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];

		mm->size = stat->buf_size;

		if (!buf->virt_addr)
			continue;

		mm->vir_addr = buf->virt_addr;
		mm->dma_addr = buf->dma_addr;
		os_mem_free(&stat->isp->pdev->dev, mm);

		buf->dma_addr = NULL;
		buf->virt_addr = NULL;
	}

	vin_log(VIN_LOG_STAT, "%s: all buffers were freed.\n", __func__);

	stat->buf_size = 0;
	stat->buf_cnt = 0;
	stat->active_buf = NULL;

	isp_rpbuf_destroy(stat->isp);
}

static int isp_stat_bufs_alloc(struct isp_stat *stat, u32 size, u32 count)
{
	int i;
	struct isp_dev *isp = stat->isp;

	if (count > STAT_MAX_BUFS)
		count = STAT_MAX_BUFS;

	stat->buf_size = size;
	stat->buf_cnt = count;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];

		mm->size = size;
		if (!os_mem_alloc(&stat->isp->pdev->dev, mm)) {
			buf->virt_addr = mm->vir_addr;
			buf->dma_addr = mm->dma_addr;
			buf->state = ISPSTAT_IDLE;
			buf->empty = 1;
		}
		if (!buf->virt_addr || !buf->dma_addr) {
			vin_err("%s: Can't acquire memory for stat buffer %d\n", __func__, i);
			isp_stat_bufs_free(stat);
			return -ENOMEM;
		}
	}

	isp_rpbuf_create(isp);

	return 0;
}

static struct ispstat_buffer *isp_stat_buf_get(struct isp_stat *stat)
{
	unsigned long flags;
	struct ispstat_buffer *buf;

	spin_lock_irqsave(&stat->isp->slock, flags);

	while (1) {
		buf = isp_stat_buf_find_oldest(stat);
		if (!buf) {
			spin_unlock_irqrestore(&stat->isp->slock, flags);
			vin_log(VIN_LOG_STAT, "cannot find a buffer.\n");
			return ERR_PTR(-EBUSY);
		}
		break;
	}

	stat->locked_buf = buf;

	spin_unlock_irqrestore(&stat->isp->slock, flags);
	return buf;
}


int isp_stat_request_statistics(struct isp_stat *stat)
{
	struct ispstat_buffer *buf;
	struct isp_dev *isp = stat->isp;

	if (stat->state != ISPSTAT_ENABLED) {
		vin_log(VIN_LOG_STAT, "engine not enabled.\n");
		return -EINVAL;
	}
	vin_log(VIN_LOG_STAT, "melis wants to request statistics.\n");

	mutex_lock(&stat->ioctl_lock);
	buf = isp_stat_buf_get(stat);
	if (IS_ERR(buf)) {
		mutex_unlock(&stat->ioctl_lock);
		return PTR_ERR(buf);
	}

	isp_save_rpbuf_send(isp, buf->virt_addr);
	buf->state = ISPSTAT_IDLE;

	buf->empty = 1;
	isp_stat_buf_release(stat);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	dma_addr_t dma_addr;

	if (buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		isp_stat_buf_next(stat);

		if (!stat->active_buf)
			return STAT_NO_BUF;

		dma_addr = (dma_addr_t)(stat->active_buf->dma_addr);
		bsp_isp_set_statistics_addr(stat->isp->id, dma_addr);
		if (bsp_isp_get_irq_status(stat->isp->id, PARA_LOAD_PD)) {
			stat->active_buf = NULL;
			vin_warn("para_load_pd, set active bufffer failed!\n");
		} else
			stat->active_buf->state = ISPSTAT_LOAD_SET;
	}

	return STAT_NO_BUF;
}

void isp_stat_load_set(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	int i;

	if (stat->state == ISPSTAT_DISABLED)
		return;

	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			//vin_warn("lost save_pd to get buffer\n");
			return;
		}
	}

	stat->isp->h3a_stat.frame_number++;

	isp_stat_buf_process(stat, ret);

#if 0
	for (i = 0; i < stat->buf_cnt; i++) {
		vin_print("load buffer%d stat is %d\n", i, stat->buf[i].state);
	}
#endif
}

void isp_stat_isr(struct isp_stat *stat)
{
	int ret = STAT_NO_BUF;
	unsigned long irqflags;
	int i;

	vin_log(VIN_LOG_STAT, "frame number is %d 0x%x\n",
		stat->frame_number, stat->buf_size);

	if (stat->state == ISPSTAT_DISABLED)
		return;

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			goto next;
		}
	}
	//vin_warn("lost load_pd to set buffer\n");
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	return;
next:
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET)
			stat->buf[i].state = ISPSTAT_SAVE_SET;
		else if (stat->buf[i].state == ISPSTAT_SAVE_SET) {
			stat->buf[i].state = ISPSTAT_READY;
			stat->buf[i].empty = 0;
			ret = STAT_BUF_DONE;
		}
		vin_log(VIN_LOG_STAT, "save buffer%d stat is %d\n", i, stat->buf[i].state);
	}
	if (ret != STAT_BUF_DONE)
		stat->isp->save_get_flag = 1;
	else
		stat->isp->save_get_flag = 0;

	isp_stat_buf_queue(stat);
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
}

int isp_reset_config_sensor_info(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct sensor_config cfg;
	unsigned int data[20];
	char *sensor_name;
	int ret = 0;
	int i;

	memset(&cfg, 0, sizeof(cfg));

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_busy(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc == NULL) {
		vin_err("%s:cannot find a real vinc to get sensor info\n", __func__);
		return -1;
	}

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
				VIDIOC_VIN_SENSOR_CFG_REQ, &cfg);
	if (ret) {
		vin_err("%s:get sensor info fail!!!\n", __func__);
		return ret;
	}

	data[1] = cfg.width;
	data[2] = cfg.height;
	data[3] = cfg.fps_fixed;
	data[4] = cfg.wdr_mode;
	// BT601_FULL_RANGE
	data[5] = 1;

	switch (cfg.mbus_code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		data[6] = 8;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		data[6] = 12;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		data[6] = 8;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		data[6] = 10;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		data[6] = 12;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		data[6] = 8;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		data[6] = 10;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		data[6] = 12;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		data[6] = 8;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		data[6] = 10;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		data[6] = 12;
		data[7] = ISP_RGGB;
		break;
	default:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	}

	if (cfg.hts && cfg.vts && cfg.pclk) {
		data[8] = cfg.hts;
		data[9] = cfg.vts;
		data[10] = cfg.pclk;
		data[11] = cfg.bin_factor;
		data[12] = cfg.gain_min;
		data[13] = cfg.gain_max;
		data[14] = cfg.hoffset;
		data[15] = cfg.voffset;

	} else {
		data[8] = cfg.width;
		data[9] = cfg.height;
		data[10] = cfg.width * cfg.height * 30;
		data[11] = 1;
		data[12] = 16;
		data[13] = 255;
		data[14] = 0;
		data[15] = 0;
	}

	sensor_name = (char *)&data[16];
	memcpy(sensor_name, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name, 4 * sizeof(unsigned int));

	data[0] = VIN_SET_ISP_RESET;
	isp_rpmsg_send(isp->rpmsg, data, 20 * sizeof(unsigned int));

	return 0;
}

int isp_config_sensor_info(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct sensor_config cfg;
	unsigned int data[20];
	char *sensor_name;
	int ret = 0;
	int i;

	memset(&cfg, 0, sizeof(cfg));

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_busy(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc == NULL) {
		vin_err("%s:cannot find a real vinc to get sensor info\n", __func__);
		return -1;
	}

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
				VIDIOC_VIN_SENSOR_CFG_REQ, &cfg);
	if (ret) {
		vin_err("%s:get sensor info fail!!!\n", __func__);
		return ret;
	}

	data[1] = cfg.width;
	data[2] = cfg.height;
	data[3] = cfg.fps_fixed;
	data[4] = cfg.wdr_mode;
	// BT601_FULL_RANGE
	data[5] = 1;

	switch (cfg.mbus_code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		data[6] = 8;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		data[6] = 12;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		data[6] = 8;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		data[6] = 10;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		data[6] = 12;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		data[6] = 8;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		data[6] = 10;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		data[6] = 12;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		data[6] = 8;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		data[6] = 10;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		data[6] = 12;
		data[7] = ISP_RGGB;
		break;
	default:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	}

	if (cfg.hts && cfg.vts && cfg.pclk) {
		data[8] = cfg.hts;
		data[9] = cfg.vts;
		data[10] = cfg.pclk;
		data[11] = cfg.bin_factor;
		data[12] = cfg.gain_min;
		data[13] = cfg.gain_max;
		data[14] = cfg.hoffset;
		data[15] = cfg.voffset;

	} else {
		data[8] = cfg.width;
		data[9] = cfg.height;
		data[10] = cfg.width * cfg.height * 30;
		data[11] = 1;
		data[12] = 16;
		data[13] = 255;
		data[14] = 0;
		data[15] = 0;
	}

	sensor_name = (char *)&data[16];
	memcpy(sensor_name, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name, 4 * sizeof(unsigned int));

	data[0] = VIN_SET_SENSOR_INFO;
	isp_rpmsg_send(isp->rpmsg, data, 20 * sizeof(unsigned int));

	return 0;
}

void isp_sensor_set_exp_gain(struct isp_dev *isp, void *data)
{
	struct sensor_exp_gain exp_gain;
	static struct sensor_exp_gain exp_gain_save[VIN_MAX_ISP];
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	unsigned int *result = data;
	int i;

	exp_gain.exp_val = result[1];
	exp_gain.gain_val = result[2];
	exp_gain.r_gain = result[3];
	exp_gain.b_gain = result[4];

#if 0
	if (memcmp(&exp_gain_save[isp->id], &exp_gain, sizeof(struct sensor_exp_gain))) {
		memcpy(&exp_gain_save[isp->id], &exp_gain, sizeof(struct sensor_exp_gain));
	} else {
		return;
	}
#else
	if ((exp_gain_save[isp->id].exp_val != exp_gain.exp_val) || (exp_gain_save[isp->id].gain_val != exp_gain.gain_val)) {
		memcpy(&exp_gain_save[isp->id], &exp_gain, sizeof(struct sensor_exp_gain));
	} else {
		return;
	}
#endif

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc != NULL)
		v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
					VIDIOC_VIN_SENSOR_EXP_GAIN, &exp_gain);
}

static int write_nor_flash(loff_t to, size_t len, const u_char *buf)
{
	struct spi_nor *nor = get_spinor();
	uint32_t block_size = 4 * 1024;
	u8 val = 0;
	int ret;
	size_t block_remain;
	ssize_t written;
	int time_out = 0xffff;
	u8 addr_buf[4];
	int i;
	u8 erase_op;

	/* Operable range limits */
	if (to != 112 * 512) {
		vin_err("must write 0x%x, not to 0x%x\n", 112 * 512, (u32)to);
		return 0;
	}

	if (nor->addr_width == 4)
		erase_op = SPINOR_OP_BE_4K_4B;
	else
		erase_op = SPINOR_OP_BE_4K;

	mutex_lock(&nor->lock);

	/* Maximum size of each write is 4K */
	block_remain = min_t(size_t, block_size, len);
	/* write enable */
	nor->write_reg(nor, SPINOR_OP_WREN, NULL, 0);
	/* erasr 4k block */
	for (i = nor->addr_width - 1; i >= 0; i--)
		addr_buf[i] = (to >> (8 * (nor->addr_width - i - 1))) & 0xff;
	nor->write_reg(nor, erase_op, addr_buf, nor->addr_width);
	/* wait erasr/write progress complete */
	do {
		ret = nor->read_reg(nor, SPINOR_OP_RDSR, &val, 1);
		if (ret < 0) {
			vin_err("error %d reading SR\n", (int) ret);
			mutex_unlock(&nor->lock);
			return ret;
		}
		if (time_out <= 0) {
			vin_err("write flash time out\n");
			mutex_unlock(&nor->lock);
			return -1;
		}
		time_out--;
	} while (val & 0x1);

	/* write enable */
	nor->write_reg(nor, SPINOR_OP_WREN, NULL, 0);
	written = nor->write(nor, to, block_remain, buf);
	if (written < 0)
		goto write_err;
	/* wait erasr/write progress complete */
	do {
		ret = nor->read_reg(nor, SPINOR_OP_RDSR, &val, 1);
		if (ret < 0) {
			vin_err("error %d reading SR\n", (int) ret);
			mutex_unlock(&nor->lock);
			return ret;
		}
		if (time_out <= 0) {
			vin_err("write flash time out\n");
			mutex_unlock(&nor->lock);
			return -1;
		}
		time_out--;
	} while (val & 0x1);
	ret = written;

#if 0
	char info[64];
	nor->read(nor, to, block_remain, info);
	vin_print("read form nor:idx = %d, again = %d, exp = %d\n", *((unsigned int *)info) >> 16, *((unsigned int *)info) & 0xffff, *((unsigned int *)info + 1));
#endif
	mutex_unlock(&nor->lock);
	return ret;
write_err:
	mutex_unlock(&nor->lock);
	vin_err("%s err!!!\n", __func__);
	return ret;
}

void isp_save_ae(void *data)
{
	loff_t to = 112 * 512;

	//vin_print("idx = %d, again = %d, exp = %d\n", *((unsigned int *)data) >> 16, *((unsigned int *)data) & 0xffff, *((unsigned int *)data + 1));

	write_nor_flash(to, 2*4, data);
}

int vin_isp_h3a_init(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	if (isp->is_empty)
		return 0;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	stat->isp = isp;
	mutex_init(&stat->ioctl_lock);

	isp_stat_bufs_alloc(stat, ISP_STAT_TOTAL_SIZE, STAT_MAX_BUFS);
	return 0;
}
void vin_isp_h3a_cleanup(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	if (isp->is_empty)
		return;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	stat->stat_en_flag = 0;
	mutex_destroy(&stat->ioctl_lock);

	isp_stat_bufs_free(stat);
}
#endif
