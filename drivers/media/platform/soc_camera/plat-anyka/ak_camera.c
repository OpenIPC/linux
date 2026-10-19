/*
  * @file ak camera.c
  * @camera host driver for ak
  * @Copyright (C) 2010 Anyka (Guangzhou) Microelectronics Technology Co
  * @author wu_daochao
  * @date 2011-04
  * @version
  * @for more information , please refer to AK980x Programmer's Guide Mannul
  */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/cacheflush.h>

#include <media/soc_camera.h>
#include <media/videobuf-core.h>
#include <media/videobuf-dma-contig.h>
#include <media/soc_mediabus.h>

#include <plat-anyka/ak_camera.h>
#include <plat-anyka/ak_sensor.h>
#include <mach-anyka/ispdrv_interface.h>

//#define CAMIF_DEBUG
#ifdef CAMIF_DEBUG
#define isp_dbg(fmt...)			printk(KERN_INFO " ISP: " fmt)
#define CAMDBG(fmt...)	printk(KERN_INFO " ISP: " fmt)//do{}while(0)
#else
#define CAMDBG(fmt...)	do{}while(0)
#define isp_dbg(fmt, args...)	do{}while(0)
#endif

#define AK_CAM_VO_BUFFERS	4
#define AK_CAM_MDINFO_SIZE	1024

enum buffer_list_state {
	LIST_ZERO = 1,
	LIST_ONE,
	LIST_TWO,
	LIST_THREE,
	LIST_FOUR,
	LIST_FIVE,
};

struct ak_buffer {
	struct videobuf_buffer vb;
	u32	code;
	int				inwork;
};

enum ak_cam_irq_event {
	AK_CAM_IRQ_LATE_FRAME,
	AK_CAM_IRQ_LOST_FRAME,
	AK_CAM_IRQ_ID_INVALID,
	AK_CAM_IRQ_ID_MISMATCH,
	AK_CAM_IRQ_ID_ERROR,
	AK_CAM_IRQ_SPURIOUS,
	AK_CAM_IRQ_LIST_ZERO,
	AK_CAM_IRQ_ACTIVE_BUSY,
	AK_CAM_IRQ_EVENTS,
};

struct ak_camera_irq_stats {
	u32 count[AK_CAM_IRQ_EVENTS];
	u32 max_gap_ms;
};

enum ak_cam_queue_event {
	AK_CAM_QUEUE_VB_LINKED,
	AK_CAM_QUEUE_MODE_UNSET,
	AK_CAM_QUEUE_EVENTS,
};

struct ak_camera_queue_stats {
	u32 count[AK_CAM_QUEUE_EVENTS];
};

struct ak_camera_dev {
	struct soc_camera_host soc_host;
	struct soc_camera_device *icd;
	unsigned long bus_flags;

	struct clk	*clk;		// camera controller clk. it's parent is vclk defined in clock.c
	unsigned long	mclk;
	unsigned int	irq;

	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;	/* DVP data/clock/mclk pads */
#ifndef CONFIG_CPU_AK3918EV200
	/*
	 * EV300 and EV330 route both sensor buses through the MIPI/D-PHY block:
	 * its IO-mode register picks DVP or CSI-2, and DVP's pad voltage is set
	 * there too. EV200 has neither the block nor the MIPI pads.
	 */
	void __iomem		*mipi_base;
	struct clk		*mipi_clk;
	struct pinctrl_state	*pins_mipi[2];	/* 1 and 2 data lanes */
#endif
	struct list_head capture;
	/* members to manage the dma and buffer*/
	spinlock_t		lock;  /* for videobuf_queue , passed in init_videobuf */

	enum isp_working_mode def_mode;
	enum isp_working_mode cur_mode;
	u32 cur_mode_class;

	struct videobuf_queue  *vq;
	enum buffer_list_state list_state;
	enum buffer_list_state free_list;

	struct delayed_work awb_work;
	struct delayed_work ae_work;
	struct work_struct resume_work;
	struct mutex stream_lock;

	int stream_ctrl_off;
	int stream_active;
	int cur_buf_id;

	u32 buf_paddr[AK_CAM_VO_BUFFERS];
	void *buf_vaddr[AK_CAM_VO_BUFFERS];
	u32 buf_size;
	bool mdinfo_oob_logged;
	bool mdinfo_unmapped_logged;

	struct ak_camera_irq_stats irq_stats;
	unsigned long irq_reported;

	struct ak_camera_queue_stats queue_stats;
	unsigned long queue_reported;

	size_t pool_bytes;

	void *ref_y_cpu;
	void *ref_uv_cpu;
	dma_addr_t ref_y_dma;
	dma_addr_t ref_uv_dma;
	size_t ref_y_bytes;
	size_t ref_uv_bytes;
	unsigned int ref_pixels;
};

struct ak_camera_cam {
	/* Client output, as seen by the CEU */
	unsigned int width;
	unsigned int height;
};

#define	ISP_TIMEOUT				(1)	//unit: s
#define EMPTY_FRAME_NUM			(2)


static const char *ak_cam_driver_description = "AK_Camera";

static int _tdnr_flag = 0;
static int _tdnr_set = 0;
static int video_frame_interval;

static unsigned long in_irq_jf = 0;
static unsigned long start_set_td_jf;

/* jiffies at the previous frame-done interrupt, or 0 when there is no
 * previous frame to measure a gap from. */
static unsigned long sjf = 0;

AK_ISP_SENSOR_CB *ak_sensor_get_sensor_cb(void);

static bool ak_cam_irq_note(struct ak_camera_dev *pcdev,
			     enum ak_cam_irq_event ev)
{
	pcdev->irq_stats.count[ev]++;

	if (pcdev->irq_reported & BIT(ev))
		return false;

	pcdev->irq_reported |= BIT(ev);
	return true;
}

/* Caller holds pcdev->lock, as the videobuf buf_queue callback does. */
static bool ak_cam_queue_note_locked(struct ak_camera_dev *pcdev,
				     enum ak_cam_queue_event ev)
{
	pcdev->queue_stats.count[ev]++;

	if (pcdev->queue_reported & BIT(ev))
		return false;

	pcdev->queue_reported |= BIT(ev);
	return true;
}

static const char * const ak_cam_irq_event_name[AK_CAM_IRQ_EVENTS] = {
	[AK_CAM_IRQ_LATE_FRAME]	= "late_frame",
	[AK_CAM_IRQ_LOST_FRAME]	= "lost_frame",
	[AK_CAM_IRQ_ID_INVALID]	= "id_invalid",
	[AK_CAM_IRQ_ID_MISMATCH]	= "id_mismatch",
	[AK_CAM_IRQ_ID_ERROR]	= "id_error",
	[AK_CAM_IRQ_SPURIOUS]	= "spurious_irq",
	[AK_CAM_IRQ_LIST_ZERO]	= "list_zero",
	[AK_CAM_IRQ_ACTIVE_BUSY] = "active_busy",
};

/* Caller must not hold pcdev->lock; the videobuf queue path does not. */
static bool ak_cam_queue_note(struct ak_camera_dev *pcdev,
			      enum ak_cam_queue_event ev)
{
	unsigned long flags;
	bool report;

	spin_lock_irqsave(&pcdev->lock, flags);
	report = ak_cam_queue_note_locked(pcdev, ev);
	spin_unlock_irqrestore(&pcdev->lock, flags);

	return report;
}

static const char * const ak_cam_queue_event_name[AK_CAM_QUEUE_EVENTS] = {
	[AK_CAM_QUEUE_VB_LINKED]	= "vb_still_linked",
	[AK_CAM_QUEUE_MODE_UNSET]	= "isp_mode_unset",
};

static ssize_t queue_stats_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct soc_camera_host *ici = to_soc_camera_host(dev);
	struct ak_camera_dev *pcdev = ici->priv;
	struct ak_camera_queue_stats stats;
	unsigned long flags;
	ssize_t len = 0;
	int i;

	spin_lock_irqsave(&pcdev->lock, flags);
	stats = pcdev->queue_stats;
	spin_unlock_irqrestore(&pcdev->lock, flags);

	for (i = 0; i < AK_CAM_QUEUE_EVENTS; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s %u\n",
				 ak_cam_queue_event_name[i], stats.count[i]);

	return len;
}
static DEVICE_ATTR_RO(queue_stats);

static ssize_t irq_stats_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct soc_camera_host *ici = to_soc_camera_host(dev);
	struct ak_camera_dev *pcdev = ici->priv;
	struct ak_camera_irq_stats stats;
	unsigned long flags;
	ssize_t len = 0;
	int i;

	spin_lock_irqsave(&pcdev->lock, flags);
	stats = pcdev->irq_stats;
	spin_unlock_irqrestore(&pcdev->lock, flags);

	for (i = 0; i < AK_CAM_IRQ_EVENTS; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s %u\n",
				 ak_cam_irq_event_name[i], stats.count[i]);

	len += scnprintf(buf + len, PAGE_SIZE - len, "max_gap_ms %u\n",
			 stats.max_gap_ms);

	return len;
}
static DEVICE_ATTR_RO(irq_stats);

static void ak_camera_resume_work(struct work_struct *work)
{
	struct ak_camera_dev *pcdev = container_of(work,
					struct ak_camera_dev, resume_work);
	unsigned long flags;

	mutex_lock(&pcdev->stream_lock);
	if (pcdev->stream_ctrl_off && pcdev->icd &&
	    pcdev->list_state >= LIST_FOUR) {
		ispdrv_set_isp_resume();
		pcdev->stream_ctrl_off = 0;
		spin_lock_irqsave(&pcdev->lock, flags);
		pcdev->stream_active = 1;
		spin_unlock_irqrestore(&pcdev->lock, flags);
	}
	mutex_unlock(&pcdev->stream_lock);
}

static void ak_camera_stop_streaming(struct ak_camera_dev *pcdev)
{
	unsigned long flags;
	int stream_active;

	/* A queued restart must not race this teardown and re-enable the ISP. */
	cancel_work_sync(&pcdev->resume_work);

	mutex_lock(&pcdev->stream_lock);
	spin_lock_irqsave(&pcdev->lock, flags);
	stream_active = pcdev->stream_active;
	spin_unlock_irqrestore(&pcdev->lock, flags);
	if (stream_active) {
		ispdrv_set_isp_pause();
		ispdrv_vi_stop_capturing();
		spin_lock_irqsave(&pcdev->lock, flags);
		pcdev->stream_active = 0;
		spin_unlock_irqrestore(&pcdev->lock, flags);
		pcdev->stream_ctrl_off = 1;
	}
	ispdrv_vo_clear_irq_status(0xffff);
	synchronize_irq(pcdev->irq);
	cancel_delayed_work_sync(&pcdev->awb_work);
	cancel_delayed_work_sync(&pcdev->ae_work);
	spin_lock_irqsave(&pcdev->lock, flags);
	pcdev->cur_buf_id = -1;
	sjf = 0;
	spin_unlock_irqrestore(&pcdev->lock, flags);
	mutex_unlock(&pcdev->stream_lock);
}

/**
 * @brief:  for ak_videobuf_release, free buffer if camera stopped.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *vq: V4L2 buffer queue information structure
 * @param [in] *buf: ak camera drivers structure, include struct videobuf_buffer
 */
static void free_buffer(struct videobuf_queue *vq, struct ak_buffer *buf)
{
	unsigned long flags;
	struct soc_camera_device *icd = vq->priv_data;
	struct videobuf_buffer *vb = &buf->vb;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	int stopping = vq->streaming == 0 && vq->reading == 0;

	isp_dbg("%s (vb=0x%p) buf[%d] 0x%08lx %d\n",
			__func__, vb, vb->i, vb->baddr, vb->bsize);

	BUG_ON(in_interrupt());

	/* Stop IRQ producers and deferred 3A/MD consumers before DMA is freed. */
	if (stopping)
		ak_camera_stop_streaming(pcdev);

	spin_lock_irqsave(&pcdev->lock, flags);
	if (!list_empty(&vb->queue))
		list_del_init(&vb->queue);
	vb->state = VIDEOBUF_DONE;
	if (stopping) {
		pcdev->list_state = LIST_ZERO;
		pcdev->free_list = LIST_ZERO;
	}
	spin_unlock_irqrestore(&pcdev->lock, flags);

	/* The state transition above makes videobuf_waiton non-blocking. */
	videobuf_waiton(vq, vb, 0, 0);

	videobuf_dma_contig_free(vq, vb);

	vb->state = VIDEOBUF_NEEDS_INIT;
}

#define AK_CAM_REF_Y_BYTES(pixels)	((size_t)(pixels) * 2)
#define AK_CAM_REF_UV_BYTES(pixels)	((size_t)(pixels))

static void ak_camera_free_ref_frame(struct ak_camera_dev *pcdev)
{
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;

	if (pcdev->ref_y_cpu) {
		dma_free_coherent(dev, pcdev->ref_y_bytes, pcdev->ref_y_cpu,
				  pcdev->ref_y_dma);
		pcdev->ref_y_cpu = NULL;
	}
	if (pcdev->ref_uv_cpu) {
		dma_free_coherent(dev, pcdev->ref_uv_bytes, pcdev->ref_uv_cpu,
				  pcdev->ref_uv_dma);
		pcdev->ref_uv_cpu = NULL;
	}
	pcdev->ref_pixels = 0;
}

static int ak_camera_alloc_ref_frame(struct ak_camera_dev *pcdev,
				     unsigned int pixels)
{
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;

	if (pcdev->ref_pixels >= pixels)
		return 0;

	ak_camera_free_ref_frame(pcdev);

	pcdev->ref_y_bytes = AK_CAM_REF_Y_BYTES(pixels);
	pcdev->ref_uv_bytes = AK_CAM_REF_UV_BYTES(pixels);

	pcdev->ref_y_cpu = dma_alloc_coherent(dev, pcdev->ref_y_bytes,
					      &pcdev->ref_y_dma, GFP_KERNEL);
	pcdev->ref_uv_cpu = dma_alloc_coherent(dev, pcdev->ref_uv_bytes,
					       &pcdev->ref_uv_dma, GFP_KERNEL);
	if (!pcdev->ref_y_cpu || !pcdev->ref_uv_cpu) {
		ak_camera_free_ref_frame(pcdev);
		dev_warn(dev,
			 "no room in the capture pool for a %u-pixel 3D-NR reference frame; temporal noise reduction and the motion grid stay off\n",
			 pixels);
		return -ENOMEM;
	}

	pcdev->ref_pixels = pixels;
	dev_info(dev,
		 "3D-NR reference frame: Y %zu B at %pad, UV %zu B at %pad\n",
		 pcdev->ref_y_bytes, &pcdev->ref_y_dma,
		 pcdev->ref_uv_bytes, &pcdev->ref_uv_dma);

	return 0;
}

static void ak_camera_program_ref_frame(struct ak_camera_dev *pcdev)
{
	AK_ISP_3D_NR_REF_ATTR ref;

	if (!pcdev->ref_y_cpu || !pcdev->ref_uv_cpu)
		return;

	ref.yaddr_3d = (u32)pcdev->ref_y_dma;
	ref.ysize_3d = pcdev->ref_y_bytes;
	ref.uaddr_3d = (u32)pcdev->ref_uv_dma;
	ref.usize_3d = pcdev->ref_uv_bytes / 2;
	ref.vaddr_3d = (u32)pcdev->ref_uv_dma + pcdev->ref_uv_bytes / 2;
	ref.vsize_3d = pcdev->ref_uv_bytes / 2;

	ispdrv_vp_set_3d_nr_ref_addr(&ref);
}

/**
 * @brief:  Called when application apply buffers, camera buffer initial.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *vq: V4L2 buffer queue information structure
 * @param [in] *count: buffer's number
 * @param [in] *size: buffer total size
 */
static int ak_videobuf_setup(struct videobuf_queue *vq, unsigned int *count,
								unsigned int *size)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	bytes_per_line = icd->user_width * 3 /2;
	if (bytes_per_line < 0)
		return bytes_per_line;

	*size = bytes_per_line * icd->user_height;
	//printk(KERN_ERR "%s size:%u, bytes_per_line:%d, icd->user_height:%d\n", __func__, *size, bytes_per_line, icd->user_height);

	/* dma_alloc_from_coherent() rounds each buffer up to a whole order. */
	if (((size_t)AK_CAM_VO_BUFFERS << (get_order(*size) + PAGE_SHIFT)) >
	    pcdev->pool_bytes)
		return -ENOMEM;

	ak_camera_alloc_ref_frame(pcdev, icd->user_width * icd->user_height);

	/* ISP2 exposes four VO buffer slots and starts only after all are set. */
	*count = AK_CAM_VO_BUFFERS;

	isp_dbg("%s count=%d, size=%d, bytes_per_line=%d\n",
			__func__, *count, *size, bytes_per_line);

	return 0;
}

/**
 * @brief: Called when application apply buffers, camera buffer initial.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *vq: V4L2  buffer queue information structure
 * @param [in] *vb: V4L2  buffer information structure
 * @param [in] field: V4L2_FIELD_ANY
 */
static int ak_videobuf_prepare(struct videobuf_queue *vq,
			struct videobuf_buffer *vb, enum v4l2_field field)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct ak_buffer *buf = container_of(vb, struct ak_buffer, vb);
	int ret;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	isp_dbg("%s (vb=0x%p) buf[%d] vb->baddr=0x%08lx vb->bsize=%d bytes_per_line=%d\n",
			__func__, vb, vb->i, vb->baddr, vb->bsize, bytes_per_line);

	bytes_per_line = icd->user_width * 3 /2;

	if (bytes_per_line < 0)
		return bytes_per_line;

	/* Added list head initialization on alloc */
	if (!list_empty(&vb->queue)) {
		struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
		struct ak_camera_dev *pcdev = ici->priv;

		if (ak_cam_queue_note(pcdev, AK_CAM_QUEUE_VB_LINKED))
			pr_warn("ak_camera: buf[%d] handed to prepare while still on a list; counted in queue_stats from here on\n",
				vb->i);
	}

	BUG_ON(NULL == icd->current_fmt);

	/* I think, in buf_prepare you only have to protect global data,
	 * the actual buffer is yours */
	buf->inwork = 1;

	if (buf->code	!= icd->current_fmt->code ||
	    vb->width	!= icd->user_width ||
	    vb->height	!= icd->user_height ||
	    vb->field	!= field) {
		buf->code	= icd->current_fmt->code;
		vb->width	= icd->user_width;
		vb->height	= icd->user_height;
		vb->field	= field;
		vb->state	= VIDEOBUF_NEEDS_INIT;
	}

	//vb->size = bytes_per_line * vb->height;
	vb->size = bytes_per_line * icd->user_height;
	if (0 != vb->baddr && vb->bsize < vb->size) {
		ret = -EINVAL;
		goto out;
	}

	if (vb->state == VIDEOBUF_NEEDS_INIT) {
		ret = videobuf_iolock(vq, vb, NULL);
		if (ret)
			goto fail;

		vb->state = VIDEOBUF_PREPARED;
	}

	buf->inwork = 0;

	return 0;

fail:
	free_buffer(vq, buf);
out:
	buf->inwork = 0;
	return ret;
}


static int queue_single_mode(struct videobuf_buffer *vb, struct ak_camera_dev *pcdev)
{
	unsigned long flags;
	u32 yaddr_chl1, yaddr_chl2, size;
	struct soc_camera_device *icd = pcdev->icd;

	//size = vb->width * vb->height;
	size = icd->user_width * icd->user_height;
	yaddr_chl1 = videobuf_to_dma_contig(vb); /* for mater channel */
	yaddr_chl2 = yaddr_chl1 + size * 3 / 2; /* for secondary channel */

	spin_lock_irqsave(&pcdev->lock, flags);
	vb->state = VIDEOBUF_ACTIVE;
	list_add_tail(&vb->queue, &pcdev->capture);
	spin_unlock_irqrestore(&pcdev->lock, flags);

	switch (pcdev->list_state) {
	case LIST_ZERO:
		ispdrv_vo_set_buffer_addr(BUFFER_ONE, yaddr_chl1, yaddr_chl2);

		ispdrv_vo_enable_buffer(BUFFER_ONE);

		ispdrv_vi_apply_mode(pcdev->cur_mode);
		ak_camera_program_ref_frame(pcdev);
		ispdrv_vi_start_capturing();
		pcdev->stream_active = 1;
		pcdev->stream_ctrl_off = 0;

		pcdev->list_state++;
		break;
	case LIST_ONE:
	case LIST_TWO:
	case LIST_THREE:
	//case LIST_FOUR:
		break;
	default:
		pr_err_ratelimited("ak_camera: QBUF in single mode with list state %d\n",
				   pcdev->list_state);
		break;
	}

	return 0;
}

static int queue_continous_mode(struct videobuf_buffer *vb, struct ak_camera_dev *pcdev)
{
	int i;
	unsigned long flags;
	u32 yaddr_chl1, yaddr_chl2, size;
	struct soc_camera_device *icd = pcdev->icd;

	//size = vb->width * vb->height;
	size = icd->user_width * icd->user_height;
	yaddr_chl1 = videobuf_to_dma_contig(vb); /* for mater channel */
	yaddr_chl2 = yaddr_chl1 + size * 3 / 2; /* for secondary channel */

	isp_dbg("%s vb->i=%d, phyaddr=%x, user_width:%d, user_height:%d\n", __func__, vb->i, yaddr_chl1, icd->user_width, icd->user_height);

	switch (pcdev->list_state) {
	case LIST_FIVE:
		break;
	case LIST_ZERO:
		for (i = 0; i < 4; i++) {
			ispdrv_vo_disable_buffer(BUFFER_ONE + i);
		}

	case LIST_ONE:
	case LIST_TWO:
	case LIST_THREE:
		ispdrv_vo_set_buffer_addr(BUFFER_ONE + vb->i, yaddr_chl1, yaddr_chl2);
	case LIST_FOUR:
		pcdev->list_state++;
		break;
	default:
		pr_err_ratelimited("ak_camera: QBUF in continuous mode with list state %d\n",
				   pcdev->list_state);
		break;
	}


	spin_lock_irqsave(&pcdev->lock, flags);
	vb->state = VIDEOBUF_ACTIVE;
	list_add_tail(&vb->queue, &pcdev->capture);

	ispdrv_vo_enable_buffer(BUFFER_ONE + vb->i);
	pcdev->free_list++;
	spin_unlock_irqrestore(&pcdev->lock, flags);

	if (pcdev->list_state == LIST_FOUR) {
		ispdrv_vi_apply_mode(pcdev->cur_mode);
		ispdrv_vo_enable_irq_status(0x1);
		ak_camera_program_ref_frame(pcdev);

		if (pcdev->stream_ctrl_off == 0) {
			//printk(KERN_ERR "%s start capture\n", __func__);
			ispdrv_vi_start_capturing();
			pcdev->stream_active = 1;
		} else {
			/* ispdrv_set_isp_resume() sleeps; buf_queue holds irqlock. */
			schedule_work(&pcdev->resume_work);
		}
	}



	return 0;
}

/**
 * @brief: Called when application apply buffers, camera start data collection
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *vq: V4L2  buffer queue information structure
 * @param [in] *vb: V4L2  buffer information structure
 */
static void ak_videobuf_queue(struct videobuf_queue *vq,
								struct videobuf_buffer *vb)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;

	isp_dbg("%s (vb=0x%p) buf[%d] baddr = 0x%08lx, bsize = %d\n",
			__func__,  vb, vb->i, vb->baddr, vb->bsize);

	if (vb->i < AK_CAM_VO_BUFFERS) {
		pcdev->buf_paddr[vb->i] = videobuf_to_dma_contig(vb);
		pcdev->buf_vaddr[vb->i] = videobuf_queue_to_vaddr(vq, vb);
		pcdev->buf_size = PAGE_ALIGN(vb->size);
	}

	switch(pcdev->cur_mode) {
	case ISP_YUV_OUT:
	case ISP_RGB_OUT:
		/* for single mode */
		queue_single_mode(vb, pcdev);
		break;

	case ISP_YUV_VIDEO_OUT:
	case ISP_RGB_VIDEO_OUT:
		/* for continous mode */
		queue_continous_mode(vb, pcdev);
		pcdev->vq = vq;
		break;

	default:
		if (ak_cam_queue_note_locked(pcdev, AK_CAM_QUEUE_MODE_UNSET))
			pr_warn("ak_camera: QBUF with the ISP working mode still unset (%d); counted in queue_stats from here on\n",
				pcdev->cur_mode);
		break;
	}
}

/**
 * @brief:  for ak_videobuf_release, free buffer if camera stopped.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *vq: V4L2 buffer queue information structure
 * @param [in] *vb: V4L2  buffer information structure
 */
static void ak_videobuf_release(struct videobuf_queue *vq,
					struct videobuf_buffer *vb)
{
	struct ak_buffer *buf = container_of(vb, struct ak_buffer, vb);

	isp_dbg("%s (vb=0x%p) buf[%d] 0x%08lx %d\n",
			__func__, vb, vb->i, vb->baddr, vb->bsize);

	switch (vb->state) {
	case VIDEOBUF_ACTIVE:
		CAMDBG("vb status: ACTIVE\n");
		break;
	case VIDEOBUF_QUEUED:
		CAMDBG("vb status: QUEUED\n");
		break;
	case VIDEOBUF_PREPARED:
		CAMDBG("vb status: PREPARED\n");
		break;
	default:
		CAMDBG("vb status: unknown\n");
		break;
	}

	free_buffer(vq, buf);
}

static struct videobuf_queue_ops ak_videobuf_ops = {
	.buf_setup      = ak_videobuf_setup,
	.buf_prepare    = ak_videobuf_prepare,
	.buf_queue      = ak_videobuf_queue,
	.buf_release    = ak_videobuf_release,
};

static inline int list_how_many_entries(struct list_head *head)
{
	int i;
	struct list_head *list = head;

	for (i = 0; list->next != head; i++)
		list = list->next;

	return i;
}

static int delay_works(struct ak_camera_dev *pcdev)
{
	if (pcdev->cur_mode_class >= MEDIA_BUS_FMT_SBGGR8_1X8 &&
			pcdev->cur_mode_class <= MEDIA_BUS_FMT_SRGGB12_1X12) {
		if (1){//(pcdev->rfled_ison == 0) {
			schedule_delayed_work(&pcdev->awb_work, 0);
		}
		schedule_delayed_work(&pcdev->ae_work, 0);
	}

	return 0;
}

static inline unsigned long get_timestamp(void)
{
	unsigned long ul;

	ul = jiffies;
	if (ul >= INITIAL_JIFFIES)
		ul -= INITIAL_JIFFIES;
	else
		ul = (~(unsigned long)0) - INITIAL_JIFFIES + ul;
	ul = jiffies_to_msecs(ul);

	return ul;
}

static int irq_handle_single_mode(struct videobuf_buffer *vb, struct ak_camera_dev *pcdev)
{
	int entries;
	u32 yaddr_chl1, yaddr_chl2, size;
	struct videobuf_buffer *vb_active;
	struct ak_buffer *ak_active;
	unsigned long timestamp_ms;


	timestamp_ms = get_timestamp();
	vb->ts.tv_sec = timestamp_ms / 1000;
	vb->ts.tv_usec = (timestamp_ms % 1000) * 1000;

	list_del_init(&vb->queue);
	vb->state = VIDEOBUF_DONE;
	vb->field_count++;

	entries = list_how_many_entries(&pcdev->capture);
	if (entries >= 1) {
		ak_active = list_entry(pcdev->capture.next,
				   struct ak_buffer, vb.queue);
		vb_active = &ak_active->vb;

		//size = vb_active->width * vb_active->height;
		size = pcdev->icd->user_width * pcdev->icd->user_height;
		yaddr_chl1 = videobuf_to_dma_contig(vb_active); /* for mater channel */
		yaddr_chl2 = yaddr_chl1 + size * 3 / 2; /* for secondary channel */

		ispdrv_vo_set_buffer_addr(BUFFER_ONE, yaddr_chl1, yaddr_chl2);
		ispdrv_vi_start_capturing();
	} else {
		ispdrv_vo_disable_buffer(BUFFER_ONE);
		pcdev->list_state = LIST_ZERO;
	}

	return 0;
}

static int irq_handle_continous_mode(struct ak_camera_dev *pcdev)
{
	int video_data_err = 0;
	int id;
	unsigned long timestamp_ms;
	struct videobuf_buffer *vb;
	struct ak_buffer *ak_buf;
	struct list_head *next;
	static struct list_head *save_list;
	unsigned long ul , ul2;
	unsigned int gap_ms;
	int fps = 10;
	AK_ISP_SENSOR_CB *sensor_cb;

	sensor_cb = ak_sensor_get_sensor_cb();
	if (sensor_cb && sensor_cb->sensor_get_fps_func)
		fps = sensor_cb->sensor_get_fps_func();
	if (fps <= 0)
		fps = 10;
	//printk(KERN_ERR "fps:%d\n", fps);
	video_frame_interval = 1000 / fps + 10;

	ul = ul2 = jiffies;
	if (sjf == 0)
		sjf = jiffies;

	if (ul >= sjf)
		ul -= sjf;
	else
		ul = (~(unsigned long)0) - sjf+ ul;
	gap_ms = jiffies_to_msecs(ul);
	if (gap_ms > video_frame_interval) {
		if (gap_ms > pcdev->irq_stats.max_gap_ms)
			pcdev->irq_stats.max_gap_ms = gap_ms;
		if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_LATE_FRAME))
			pr_warn("ak_camera: inter-frame gap %ums over the %dms budget; further gaps are counted in the irq_stats attribute only\n",
				gap_ms, video_frame_interval);
		_tdnr_flag = 1;
	}
	sjf = ul2;

	id = ispdrv_vo_get_using_frame_buf_id();
	//	printk("HWFrameId=%d\n", id);
//	isp2_print_reg_table();

	next = pcdev->capture.next;
	if (next == &pcdev->capture) {
		return 0;
	}

	ak_buf = list_entry(next, struct ak_buffer, vb.queue);
	vb = &ak_buf->vb;
	if (id == -1)
	{
		if ((vb->field_count > 1) && (save_list != next )) {
			if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_ID_INVALID))
				pr_warn("ak_camera: hardware reports no frame buffer in use while vb->i=%d is queued; counted in irq_stats from here on\n",
					vb->i);
			save_list = next;
		}
		return 0;
	} else if ((id & 0x7F) != vb->i) {
		if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_ID_MISMATCH))
			pr_warn("ak_camera: hardware frame buffer id %d does not match queued vb->i=%d; counted in irq_stats from here on\n",
				id, vb->i);
		return 0;
	} else if (id >= 0x80) {
		if ((id & 0x7F) != vb->i) {
			if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_ID_ERROR))
				pr_warn("ak_camera: errored hardware frame buffer id %d does not match queued vb->i=%d; counted in irq_stats from here on\n",
					id, vb->i);
			return 0;
		}

		video_data_err = 1;
	}
	save_list = next;
	next = next->next;

	if (next != &pcdev->capture) {
		ispdrv_vo_disable_buffer(BUFFER_ONE + vb->i);

		if (!video_data_err) {
			timestamp_ms = get_timestamp();
			vb->ts.tv_sec = timestamp_ms / 1000;
			vb->ts.tv_usec = (timestamp_ms % 1000) * 1000;
		} else {
			vb->ts.tv_sec = 0;
			vb->ts.tv_usec = 0;
		}

		list_del_init(&vb->queue);
		vb->state = VIDEOBUF_DONE;
		vb->field_count++;
//		printk("%s vb->i=%d DONE\n", __func__, vb->i);

		pcdev->cur_buf_id = vb->i;
		// here,  current frame commit to video_buffer layer
	 	wake_up(&vb->done);


	} else {
		pcdev->cur_buf_id = -1;
		if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_LOST_FRAME))
			pr_warn("ak_camera: capture list down to one buffer, dropping frames; further losses are counted in the irq_stats attribute only\n");
	}

	return 0;
}

static irqreturn_t ak_camera_dma_irq(int channel, void *data)
{
	unsigned long stat;
	struct ak_camera_dev *pcdev = data;
	struct ak_buffer *ak_active;
	struct videobuf_buffer *vb;
	unsigned long flags;

	in_irq_jf = jiffies;
	//printk(KERN_ERR "%lu\n",in_irq_jf);

	spin_lock_irqsave(&pcdev->lock, flags);

	if (!((stat = ispdrv_vo_check_irq_status()) & 0x01)) {
		bool report;

		ispdrv_vo_clear_irq_status(0xfffe);
		report = ak_cam_irq_note(pcdev, AK_CAM_IRQ_SPURIOUS);
		spin_unlock_irqrestore(&pcdev->lock, flags);
		if (report)
			pr_warn("ak_camera: interrupt with no frame-done bit, stat 0x%lx; counted in irq_stats from here on\n",
				stat);
		return IRQ_HANDLED;
//		goto out;
	}

	ispdrv_irq_work();
	if (pcdev->list_state == LIST_ZERO)
	{
		if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_LIST_ZERO))
			pr_warn("ak_camera: interrupt with an empty capture list; counted in irq_stats from here on\n");
		goto out;
	}

	if (!ispdrv_is_continuous()) {
		ak_active = list_entry(pcdev->capture.next,
						   struct ak_buffer, vb.queue);
		vb = &ak_active->vb;
		if (ak_active->inwork || list_empty(&vb->queue)) {
			if (ak_cam_irq_note(pcdev, AK_CAM_IRQ_ACTIVE_BUSY))
				pr_warn("ak_camera: frame done on buf[%d] with inwork=%d, queued=%d; counted in irq_stats from here on\n",
					vb->i, ak_active->inwork,
					!list_empty(&vb->queue));
		}
		irq_handle_single_mode(vb, pcdev);
	} else {
		irq_handle_continous_mode(pcdev);
	}

	delay_works(pcdev);

out:
	ispdrv_vo_clear_irq_status(0xffff);
	spin_unlock_irqrestore(&pcdev->lock, flags);
//	printk("%s stat:%x\n", __func__, stat);
	return IRQ_HANDLED;
}

static void isp_awb_work(struct work_struct *work)
{
	ispdrv_awb_work();
}

static void isp_copy_mdinfo(struct ak_camera_dev *pcdev)
{
	int id = pcdev->cur_buf_id;
	void *yuv_paddr, *mdinfo, *vbase;
	u32 base, dst;

	if (id < 0 || id >= AK_CAM_VO_BUFFERS)
		return;

	base = pcdev->buf_paddr[id];
	vbase = pcdev->buf_vaddr[id];

	ispdrv_get_yuvaddr_and_mdinfo(id, &yuv_paddr, &mdinfo);
	dst = (u32)yuv_paddr;

	if (!base || !mdinfo)
		return;

	if (dst < base || dst + AK_CAM_MDINFO_SIZE > base + pcdev->buf_size) {
		if (!pcdev->mdinfo_oob_logged) {
			pcdev->mdinfo_oob_logged = true;
			pr_debug("ak_camera: ISP motion-detection statistics would land at %08x+%x, outside capture buffer %d [%08x..%08x); not copied, capture continues. Geometry and buffer size disagree. Reported once per open\n",
				 dst, AK_CAM_MDINFO_SIZE, id,
				 base, base + pcdev->buf_size);
		}
		return;
	}

	if (!vbase) {
		if (!pcdev->mdinfo_unmapped_logged) {
			pcdev->mdinfo_unmapped_logged = true;
			pr_debug("%s: capture buffer %d has no kernel mapping, skipping the md-info copy; normal for V4L2_MEMORY_USERPTR. Reported once per open\n",
				 __func__, id);
		}
		return;
	}

	memcpy(vbase + (dst - base), mdinfo, AK_CAM_MDINFO_SIZE);
}

static void isp_ae_work(struct work_struct *work)
{
	int aec_delay_ms = 10;
	unsigned long flags;
	unsigned long cur_jf = jiffies;
	struct ak_camera_dev *pcdev = container_of(work,
					struct ak_camera_dev, ae_work.work);
	AK_ISP_SENSOR_CB *sensor_cb;

	if (cur_jf >= in_irq_jf) {
		int cur_fps = 25;
		int active_ms = 36;
		unsigned long diff_jf = cur_jf - in_irq_jf;
		int diff_ms;

		sensor_cb = ak_sensor_get_sensor_cb();
		if (sensor_cb && sensor_cb->sensor_get_parameter_func) {
			sensor_cb->sensor_get_parameter_func(GET_CUR_FPS,
							      &cur_fps);
			sensor_cb->sensor_get_parameter_func(GET_VSYNC_ACTIVE_MS,
							      &active_ms);
			if (cur_fps <= 0)
				cur_fps = 25;
			if (active_ms < 0)
				active_ms = 0;
			aec_delay_ms = MSEC_PER_SEC / cur_fps - active_ms + 1;
			if (aec_delay_ms < 0)
				aec_delay_ms = 0;
			if (!(aec_delay_ms % 10))
				aec_delay_ms = max(aec_delay_ms - 1, 0);
		}

		diff_ms = jiffies_to_msecs(diff_jf);
		if (aec_delay_ms > diff_ms)
			aec_delay_ms -= diff_ms;
		else
			aec_delay_ms = 0;

		usleep_range(aec_delay_ms * USEC_PER_MSEC,
			     (aec_delay_ms + 1) * USEC_PER_MSEC);
	}

	ispdrv_ae_work();

	isp_copy_mdinfo(pcdev);

	pcdev->cur_buf_id = -1;

	if (_tdnr_flag && !_tdnr_set) {
		ispdrv_set_td();
		spin_lock_irqsave(&pcdev->lock, flags);
		_tdnr_set = 1;
		_tdnr_flag = 0;
		start_set_td_jf = jiffies;
		spin_unlock_irqrestore(&pcdev->lock, flags);
	} else if (_tdnr_set &&
		   jiffies_to_msecs(jiffies - start_set_td_jf) >
		   2 * video_frame_interval) {
		ispdrv_reload_td();
		spin_lock_irqsave(&pcdev->lock, flags);
		_tdnr_set = 0;
		_tdnr_flag = 0;
		spin_unlock_irqrestore(&pcdev->lock, flags);
	}
}


#define REG32(_reg)		(*(volatile unsigned long *)(_reg))
#define CLOCK_PERI_PLL_CTRL1	(AK_VA_SYSCTRL + 0x14)
#define CLOCK_PERI_PLL_CTRL2	(AK_VA_SYSCTRL + 0x18)

#define CIS_SCLK_DIV_SHIFT	10
#define CIS_SCLK_DIV_MASK	(0x3f << CIS_SCLK_DIV_SHIFT)
#define CIS_SCLK_DIV_UPDATE	(1 << 19)
#define CIS_SCLK_ENABLE		(1 << 18)

static unsigned long ak_get_peri_pll_clk(void)
{
	unsigned long pll_m, pll_n, pll_od;
	unsigned long peri_pll_clk;
	unsigned long regval;

	regval = REG32(CLOCK_PERI_PLL_CTRL1);
	pll_od = (regval & (0x3 << 12)) >> 12;
	pll_n = (regval & (0xf << 8)) >> 8;
	pll_m = regval & 0xff;

	peri_pll_clk = (12 * pll_m) / (pll_n * (1 << pll_od));
	if ((pll_od >= 1) && (pll_n >= 2) && (pll_n <= 6) &&
	    (pll_m >= 84) && (pll_m <= 254))
		return peri_pll_clk * 1000000UL;

	panic("peri pll clk: %ld(Mhz) is unusable\n", peri_pll_clk);
}

static void set_sensor_cis_sclk(unsigned int cis_sclk)
{
	unsigned long regval;
	unsigned int cis_sclk_div;

	unsigned int peri_pll = ak_get_peri_pll_clk()/1000000;

	cis_sclk_div = peri_pll/cis_sclk - 1;

	regval = REG32(CLOCK_PERI_PLL_CTRL2);
	regval &= ~CIS_SCLK_DIV_MASK;
	regval |= (cis_sclk_div << CIS_SCLK_DIV_SHIFT);
	REG32(CLOCK_PERI_PLL_CTRL2) = CIS_SCLK_DIV_UPDATE | regval;
	REG32(CLOCK_PERI_PLL_CTRL2) |= CIS_SCLK_ENABLE;

	isp_dbg("%s() cis_sclk=%dMHz peri_pll=%dMHz cis_sclk_div=%d\n",
			__func__, cis_sclk, peri_pll, cis_sclk_div);
}

static void disable_sensor_cis_sclk(void)
{
	REG32(CLOCK_PERI_PLL_CTRL2) &= ~CIS_SCLK_ENABLE;
}

#ifndef CONFIG_CPU_AK3918EV200
/*
 * The MIPI/D-PHY bring-up below is the Cloud39EV300 SDK's own
 * (drivers/media/video/plat-anyka/ak_camera.c, mach-ak39 clock.c and
 * reset.c), moved from fixed virtual mappings onto the device tree's
 * "anyka,ak39ev330-mipi-csi0" node and the sysctrl window.
 */
#define MODULE_RESET_CON1	(AK_VA_SYSCTRL + 0x20)
#define AK_SHAREPIN_CON1	(AK_VA_SYSCTRL + 0x74)
#define AK_SRESET_MIPI		16

#define MIPI_W(off, val)	__raw_writeb((val), pcdev->mipi_base + (off))

static void ak_mipi_soft_reset(void)
{
	REG32(MODULE_RESET_CON1) |= 1 << AK_SRESET_MIPI;
	mdelay(5);
	REG32(MODULE_RESET_CON1) &= ~(1 << AK_SRESET_MIPI);
}

static void ak_wait_cis_pclk(void)
{
	int timeout = 0xffff;

	while ((REG32(CLOCK_PERI_PLL_CTRL2) & (1 << 29)) && timeout-- > 0)
		;
}

static void ak_set_cis_pclk(unsigned int pclk_mhz)
{
	unsigned long regval;
	unsigned int div = ak_get_peri_pll_clk() / 1000000 / pclk_mhz;

	REG32(CLOCK_PERI_PLL_CTRL2) &= ~(1 << 28);
	ak_wait_cis_pclk();
	regval = REG32(CLOCK_PERI_PLL_CTRL2);
	regval &= ~(0x3f << 20);
	regval |= (div - 1) << 20;
	REG32(CLOCK_PERI_PLL_CTRL2) = regval;
	REG32(CLOCK_PERI_PLL_CTRL2) |= 1 << 29;
	ak_wait_cis_pclk();
	REG32(CLOCK_PERI_PLL_CTRL2) |= 1 << 28;
	ak_wait_cis_pclk();
}

static void ak_peri_ctrl1_bit(int bit, int on)
{
	if (on)
		REG32(CLOCK_PERI_PLL_CTRL1) |= 1 << bit;
	else
		REG32(CLOCK_PERI_PLL_CTRL1) &= ~(1 << bit);
}

static void set_dvp_initiation(struct ak_camera_dev *pcdev, int io_level)
{
	MIPI_W(0x00, 0x7d);
	MIPI_W(0x20, 0x3f);
	MIPI_W(0xb3, 0x01);
	switch (io_level) {
	case SENSOR_IO_LEVEL_1V8:
		MIPI_W(0xb8, 0x10);
		break;
	case SENSOR_IO_LEVEL_2V5:
		MIPI_W(0xb8, 0x08);
		break;
	case SENSOR_IO_LEVEL_3V3:
		MIPI_W(0xb8, 0x04);
		break;
	default:
		pr_err("%s: sensor IO level %d is not defined\n",
		       __func__, io_level);
		break;
	}
}

static int mipi_mhz_to_thssettle(int mhz)
{
	static const int top[] = { 110, 150, 200, 250, 300, 400, 500, 600,
				   700, 800, 1000 };
	int i;

	if (mhz < 80)
		return -1;
	for (i = 0; i < ARRAY_SIZE(top); i++)
		if (mhz <= top[i])
			return i;
	return -1;
}

/* time_formula per THS-SETTLE value, from the SDK's ths_settle table. */
static const int ths_settle_formula[] = { 4, 6, 9, 10, 13, 17, 23, 28,
					  33, 35, 44 };

static int mipi_thssettle_to_time(int thssettle, int mbps)
{
	int ui = 1000 * 100 / mbps;	/* 100 times the UI in ns */

	if (thssettle < 0 || thssettle >= ARRAY_SIZE(ths_settle_formula))
		return 0;
	return 2 * ths_settle_formula[thssettle] * ui / 100;
}

/* Move the IP's THS-SETTLE into the window the D-PHY spec allows. */
static int mipi_thssettle_init(int base, int mbps)
{
	int ui = 1000 * 100 / mbps;
	int min = 85 + 6 * ui / 100, max = 145 + 10 * ui / 100;
	int t = mipi_thssettle_to_time(base, mbps);
	int i;

	if (t > min && t <= max)
		return base;
	if (t <= min) {
		for (i = base; i < ARRAY_SIZE(ths_settle_formula); i++) {
			t = mipi_thssettle_to_time(i, mbps);
			if (t > min && t <= max)
				return i;
		}
	} else {
		for (i = base; i >= 0; i--) {
			t = mipi_thssettle_to_time(i, mbps);
			if (t > min && t <= max)
				return i;
		}
	}
	return -1;
}

static int mipi_ip_initiation(struct ak_camera_dev *pcdev,
			      AK_ISP_SENSOR_CB *sensor_cb)
{
	int mipi_mhz = 200, mipi_lane = 2, thssettle;
	unsigned char value;

	if (!sensor_cb || !sensor_cb->sensor_get_parameter_func)
		return -EINVAL;
	sensor_cb->sensor_get_parameter_func(GET_MIPI_MHZ, &mipi_mhz);
	sensor_cb->sensor_get_parameter_func(GET_MIPI_LANE, &mipi_lane);

	thssettle = mipi_mhz_to_thssettle(mipi_mhz);
	if (thssettle >= 0)
		thssettle = mipi_thssettle_init(thssettle, mipi_mhz);
	if (thssettle < 0) {
		pr_err("%s: no THS-SETTLE for %d MHz\n", __func__, mipi_mhz);
		return -EINVAL;
	}
	value = 0x80 | (thssettle & 0x0f);

	if (mipi_lane == 2) {
		MIPI_W(0x00, 0x7d);	/* D-PHY data and clock lanes on */
		MIPI_W(0x20, 0x1f);	/* TTL register bank off */
		MIPI_W(0xe0, 0xf9);	/* two-lane mode */
		MIPI_W(0xe1, 0x01);	/* error reporting on */
		MIPI_W(0x40, value);	/* THS-SETTLE: clock, data0, data1 */
		MIPI_W(0x60, value);
		MIPI_W(0x80, value);
		MIPI_W(0xb3, 0x02);	/* IO mode: MIPI */
	} else if (mipi_lane == 1) {
		MIPI_W(0x40, value);
		MIPI_W(0x60, value);
		MIPI_W(0x00, 0x7d);
		MIPI_W(0xe0, 0xf8);
		/*
		 * The SDK's one-lane path never wrote the IO mode: it reached here
		 * from reset. This host sets the bus up as DVP to identify the
		 * sensor first, which leaves 0x01 (DVP) behind.
		 */
		MIPI_W(0xb3, 0x02);	/* IO mode: MIPI */
	} else {
		pr_err("%s: %d MIPI lanes\n", __func__, mipi_lane);
		return -EINVAL;
	}
	MIPI_W(0xb8, 0x04);
	MIPI_W(0x0d, 0xc0);
	return 0;
}

static int set_sensor_interface(struct ak_camera_dev *pcdev,
				int sensor_interface, int sensor_io_level,
				AK_ISP_SENSOR_CB *sensor_cb)
{
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;
	struct pinctrl_state *pins;
	int mipi_lane = 2;
	int ret;

	if (!pcdev->mipi_base) {
		dev_err(dev, "no MIPI/D-PHY block, cannot set up the sensor bus\n");
		return -ENODEV;
	}

	switch (sensor_interface) {
	case DVP_INTERFACE:
		pins = pcdev->pins_default;
		break;
	case MIPI_INTERFACE:
		if (sensor_cb && sensor_cb->sensor_get_parameter_func)
			sensor_cb->sensor_get_parameter_func(GET_MIPI_LANE,
							     &mipi_lane);
		pins = pcdev->pins_mipi[mipi_lane == 1 ? 0 : 1];
		break;
	default:
		dev_err(dev, "unsupported sensor interface %d\n",
			sensor_interface);
		return -EINVAL;
	}

	if (pcdev->pinctrl && pins) {
		ret = pinctrl_select_state(pcdev->pinctrl, pins);
		if (ret) {
			dev_err(dev, "failed to select the sensor pinmux: %d\n",
				ret);
			return ret;
		}
	} else {
		dev_warn(dev, "no pinctrl state for interface %d; assuming pads pre-muxed\n",
			 sensor_interface);
	}

	if (sensor_interface == DVP_INTERFACE) {
		ak_peri_ctrl1_bit(26, 0);	/* CIS input: DVP */
		set_dvp_initiation(pcdev, sensor_io_level);
		return 0;
	}

	ak_set_cis_pclk(75);			/* internal CIS pclk */
	REG32(AK_SHAREPIN_CON1) |= (1 << 30) | (1 << 4);	/* pclk -> cis2_sclk */
	ak_mipi_soft_reset();
	ak_peri_ctrl1_bit(27, 0);		/* release byte-clock reset */
	ak_peri_ctrl1_bit(24, 1);		/* pclk from the PLL */
	ak_peri_ctrl1_bit(26, 1);		/* CIS input: MIPI */
	if (pcdev->mipi_clk)
		clk_prepare_enable(pcdev->mipi_clk);
	ak_mipi_soft_reset();
	return mipi_ip_initiation(pcdev, sensor_cb);
}
#else
static int set_sensor_interface(struct ak_camera_dev *pcdev,
				int sensor_interface, int sensor_io_level,
				AK_ISP_SENSOR_CB *sensor_cb)
{
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;
	int ret;

	switch (sensor_interface) {
	case DVP_INTERFACE:
		/* Mux the DVP pads to the camera function. */
		if (pcdev->pins_default) {
			ret = pinctrl_select_state(pcdev->pinctrl,
						   pcdev->pins_default);
			if (ret) {
				dev_err(dev,
					"failed to select DVP pinmux: %d\n",
					ret);
				return ret;
			}
		} else {
			dev_warn(dev,
				 "no DVP pinctrl; assuming pads pre-muxed\n");
		}
		return 0;
	case MIPI_INTERFACE:
		/* EV200 has no MIPI receiver. */
		dev_err(dev, "MIPI sensor selected, and this SoC has no MIPI receiver\n");
		return -EOPNOTSUPP;
	default:
		dev_err(dev, "unsupported sensor interface %d\n",
			sensor_interface);
		return -EINVAL;
	}
}
#endif

/**
 * @brief: Called when the /dev/videox is opened. initial ISP and sensor device.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 */
static int ak_camera_add_device(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct ak_camera_cam *cam;
	int sensor_interface = DVP_INTERFACE;
	int sensor_io_level = SENSOR_IO_LEVEL_1V8;
	enum sensor_bus_type sensor_bus_type = BUS_TYPE_RAW;
	unsigned long sensor_mclk = pcdev->mclk;
	unsigned long running_mclk;
	AK_ISP_SENSOR_CB *sensor_cb;
	int cam_allocated = 0;
	int ret;

	CAMDBG("entry %s\n", __func__);

	/* The host supports one active soc-camera device at a time. */
	if (pcdev->icd)
		return -EBUSY;

	/* A previous close must have drained every driver-owned buffer. */
	if (!list_empty(&pcdev->capture)) {
		dev_err(icd->parent, "capture list is not empty at open\n");
		return -EBUSY;
	}

	memset(pcdev->buf_paddr, 0, sizeof(pcdev->buf_paddr));
	memset(pcdev->buf_vaddr, 0, sizeof(pcdev->buf_vaddr));
	pcdev->buf_size = 0;
	pcdev->cur_buf_id = -1;
	pcdev->mdinfo_oob_logged = false;
	pcdev->mdinfo_unmapped_logged = false;
	memset(&pcdev->irq_stats, 0, sizeof(pcdev->irq_stats));
	pcdev->irq_reported = 0;
	memset(&pcdev->queue_stats, 0, sizeof(pcdev->queue_stats));
	pcdev->queue_reported = 0;

	cam = icd->host_priv;
	if (!cam) {
		cam = kzalloc(sizeof(*cam), GFP_KERNEL);
		if (!cam)
			return -ENOMEM;
		cam_allocated = 1;
	}

	/* Sensor interface up at the board's declared mclk, then let the
	 * subdev identify itself over I2C. */
	clk_prepare_enable(pcdev->clk);
	REG32(CLOCK_PERI_PLL_CTRL1) &= ~(0x01<<25);

	ret = set_sensor_interface(pcdev, sensor_interface, sensor_io_level,
				   NULL);
	if (ret)
		goto err_isp_clk;

	running_mclk = sensor_mclk;
	set_sensor_cis_sclk(running_mclk);

	/********** config sensor module **********/
	ret = v4l2_subdev_call(sd, core, init, 0);
	if (ret && ret != -ENOIOCTLCMD) {
		dev_err(icd->parent, "sensor init failed: %d\n", ret);
		goto err_sensor_clk;
	}

	sensor_cb = ak_sensor_get_sensor_cb();
	if (sensor_cb) {
		if (sensor_cb->sensor_get_mclk_func) {
			unsigned long callback_mclk;

			callback_mclk = sensor_cb->sensor_get_mclk_func();
			if (callback_mclk > 0)
				sensor_mclk = callback_mclk;
		}
		if (sensor_cb->sensor_get_bus_type_func)
			sensor_bus_type = sensor_cb->sensor_get_bus_type_func();
		if (sensor_cb->sensor_get_parameter_func) {
			sensor_cb->sensor_get_parameter_func(GET_INTERFACE,
							      &sensor_interface);
			sensor_cb->sensor_get_parameter_func(GET_SENSOR_IO_LEVEL,
							      &sensor_io_level);
		}
	}
	dev_dbg(icd->parent,
		"sensor bus %d, interface %d, IO level %d, mclk %luMHz\n",
		sensor_bus_type, sensor_interface, sensor_io_level, sensor_mclk);

	/* The identified sensor overrides the board's declared mclk. */
	if (sensor_mclk != running_mclk)
		set_sensor_cis_sclk(sensor_mclk);

	/*
	 * Again with what the sensor reported: its bus, and on EV300/EV330 the
	 * DVP pad voltage, which the first pass could only guess.
	 */
#ifdef CONFIG_CPU_AK3918EV200
	if (sensor_interface != DVP_INTERFACE)
#endif
	{
		ret = set_sensor_interface(pcdev, sensor_interface,
					   sensor_io_level, sensor_cb);
		if (ret)
			goto err_sensor;
	}

	// load the default setting for sensor
//	v4l2_subdev_call(sd, core, load_fw);

	if (cam_allocated)
		icd->host_priv = cam;
	pcdev->icd = icd;
	dev_info(icd->parent, "AK Camera driver attached to camera %d\n",
		 icd->devnum);

	return 0;

err_sensor:
	v4l2_subdev_call(sd, core, reset, 0);
err_sensor_clk:
	disable_sensor_cis_sclk();
err_isp_clk:
	clk_disable_unprepare(pcdev->clk);
	if (cam_allocated)
		kfree(cam);
	return ret;
}

/**
 * @brief: Called when the /dev/videox is close. close ISP and sensor device.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 */
static void ak_camera_remove_device(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	unsigned long flags;

	CAMDBG("entry %s\n", __func__);

	_tdnr_set = 0;
	_tdnr_flag = 0;

	BUG_ON(icd != pcdev->icd);

	ak_camera_stop_streaming(pcdev);
	v4l2_subdev_call(sd, core, reset, 0);

	mdelay(500);

	/* disable the clock of isp module */
	clk_disable_unprepare(pcdev->clk);

	/* disable sensor clk */
	disable_sensor_cis_sclk();
	//ak_soft_reset(AK_SRESET_CAMERA);

	dev_info(icd->parent, "AK Camera driver detached from camera %d\n",
		 icd->devnum);

	/*
	 * A process that dies mid-stream leaves its buffers queued here, and
	 * refusing to clean up after it made the next open fail with -EBUSY
	 * for good: one crash cost a reboot. videobuf frees the buffers
	 * themselves; what has to go is this driver's list of them.
	 */
	spin_lock_irqsave(&pcdev->lock, flags);
	while (!list_empty(&pcdev->capture)) {
		struct ak_buffer *buf = list_entry(pcdev->capture.next,
						   struct ak_buffer, vb.queue);

		list_del_init(&buf->vb.queue);
		buf->vb.state = VIDEOBUF_ERROR;
		wake_up(&buf->vb.done);
	}
	pcdev->list_state = LIST_ZERO;
	pcdev->free_list = LIST_ZERO;
	pcdev->icd = NULL;
	spin_unlock_irqrestore(&pcdev->lock, flags);

	mutex_lock(&pcdev->stream_lock);
	pcdev->stream_ctrl_off = 0;
	mutex_unlock(&pcdev->stream_lock);

	CAMDBG("Leave %s\n", __func__);
}

static void set_pclk_polar(int is_rising)
{
	unsigned long regval;

	regval = REG32(CLOCK_PERI_PLL_CTRL2);
	if (is_rising)
		regval |= (0x3 << 30);
	else
		regval &= ~(0x3 << 30);
	REG32(CLOCK_PERI_PLL_CTRL2) = regval;
}

static int ak_camera_cropcap(struct soc_camera_device *icd,
					struct v4l2_cropcap *crop)
{
	int pclk_polar;
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

	isp_dbg("enter %s\n", __func__);

	pclk_polar = ispdrv_get_pclk_polar();
	switch (pclk_polar) {
	case POLAR_RISING:
		set_pclk_polar(1);
		break;
	case POLAR_FALLING:
		set_pclk_polar(0);
		break;
	default:
		printk("pclk polar wrong: %d\n", pclk_polar);
		break;
	}

	if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	// isp support crop, need complete.
	return v4l2_subdev_call(sd, video, cropcap, crop);
}

static int ak_camera_get_crop(struct soc_camera_device *icd,
		struct v4l2_crop *crop)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

	isp_dbg("entry %s\n", __func__);

	return v4l2_subdev_call(sd, video, g_crop, crop);
}

static int ak_camera_set_crop(struct soc_camera_device *icd,
		                   const struct v4l2_crop *crop)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct v4l2_cropcap cropcap;
	struct v4l2_crop local_crop;
	struct v4l2_rect off_rect;
	int ret;
	int total_left, total_top;
	AK_ISP_SENSOR_CB *sensor_cb;

	isp_dbg("entry %s\n", __func__);
	sensor_cb = ak_sensor_get_sensor_cb();
	if (NULL == sensor_cb) {
		printk(KERN_ERR "%s get sensor_cb failed!!!!!\n", __func__);
		return -ENODEV;
	} else {
		sensor_cb->sensor_get_valid_coordinate_func(&off_rect.left, &off_rect.top);
	}

	if (v4l2_subdev_call(sd, video, cropcap, &cropcap)) {
		v4l2_err(&ici->v4l2_dev,
				"%s %d cropcap err\n", __func__, __LINE__);
		return -EINVAL;
	}

	if ((cropcap.bounds.width - crop->c.left < crop->c.width) ||
			(cropcap.bounds.height - crop->c.top < crop->c.height)) {
		v4l2_err(&ici->v4l2_dev,
				"%s %d error crop values\n", __func__, __LINE__);
		return -EINVAL;
	}

	total_left = off_rect.left + crop->c.left;
	total_top = off_rect.top + crop->c.top;

	ret = ispdrv_vi_set_crop(total_left, total_top, crop->c.width, crop->c.height);
	if (ret) {
		v4l2_err(&ici->v4l2_dev,
				"%s %d error set isp crop\n", __func__, __LINE__);
		return -EINVAL;
	}


	local_crop.c.left = total_left;
	local_crop.c.top = total_top;
	local_crop.c.width = crop->c.width;
	local_crop.c.height = crop->c.height;
	return v4l2_subdev_call(sd, video, s_crop, &local_crop);
}

static int ak_camera_set_livecrop(struct soc_camera_device *icd, const struct v4l2_crop *crop)
{

	isp_dbg("entry %s\n", __func__);

	return ak_camera_set_crop(icd, crop);
}

/**
 * @brief: setting image format information, Called before ak_camera_set_fmt.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 * @param [in] *f: image format
 */
static int ak_camera_try_fmt(struct soc_camera_device *icd,
			      struct v4l2_format *f)
{
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	/* TODO: limit to ak hardware capabilities */
	CAMDBG("entry %s\n", __func__);

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dev_warn(icd->parent, "Format %x not found\n",
			 pix->pixelformat);
		return -EINVAL;
	}

	mf.width	= pix->width;
	mf.height	= pix->height;
	mf.field	= pix->field;
	mf.colorspace	= pix->colorspace;
	mf.code		= xlate->code;

	pix->width	= mf.width;
	pix->height = mf.height;
	pix->field	= mf.field;
	pix->colorspace = mf.colorspace;

	return 0;
}

/**
 * @brief: setting image format information
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 * @param [in] *f: image format
 */
static int ak_camera_set_fmt(struct soc_camera_device *icd,
			      struct v4l2_format *f)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
//	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
//	struct ak_camera_dev *pcdev = ici->priv;
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	struct v4l2_cropcap cropcap;
	int ret = 0, buswidth;

	isp_dbg("entry %s\n", __func__);

	/*
	 * This programs the ISP, and the ISP core only exists once
	 * /dev/isp_char has been opened. soc_camera_open calls straight in
	 * here, so opening the two devices in the wrong order is a NULL
	 * dereference in ak_isp_vo_set_main_channel_scale - and it happens
	 * with the soc-camera mutex held, so every later open then hangs in
	 * v4l2_open until reboot. Probe does not come this way, so the sensor
	 * is still probed at boot.
	 */
	if (!ispdrv_is_ready()) {
		dev_err(icd->parent, "ISP core is down - open /dev/isp_char first\n");
		return -ENODEV;
	}

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dev_warn(icd->parent, "Format %x not found\n",
			 pix->pixelformat);
		return -EINVAL;
	}

	buswidth = xlate->host_fmt->bits_per_sample;
	if (buswidth > 10) {
		dev_warn(icd->parent,
			 "bits-per-sample %d for format %x unsupported\n",
			 buswidth, pix->pixelformat);
		return -EINVAL;
	}

	mf.width	= pix->width;
	mf.height	= pix->height;
	mf.field	= pix->field;
	mf.colorspace	= pix->colorspace;
	mf.code		= xlate->code;
	icd->current_fmt = xlate;

	v4l2_subdev_call(sd, video, cropcap, &cropcap);
	if (mf.width > cropcap.bounds.width)
		mf.width = cropcap.bounds.width;

	if (mf.height > cropcap.bounds.height)
		mf.height = cropcap.bounds.height;

	isp_dbg("%s. mf.width = %d, mf.height = %d\n",
			__func__, mf.width, mf.height);

	if (mf.code != xlate->code)
		return -EINVAL;

	ispdrv_vo_set_main_channel_scale(pix->width, pix->height);

	return ret;
}

/**
 * @brief: getting image format information
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 * @param [in] *f: image format
 */
static const struct soc_mbus_pixelfmt ak_camera_nv12 = {
	.fourcc			= V4L2_PIX_FMT_NV12,
	.name			= "YUV 4:2:0 semi-planar",
	.bits_per_sample	= 8,
	.packing		= SOC_MBUS_PACKING_1_5X8,
	.order			= SOC_MBUS_ORDER_LE,
};

static int ak_camera_get_formats(struct soc_camera_device *icd, unsigned int idx,
				     struct soc_camera_format_xlate *xlate)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct device *dev = icd->parent;
	struct soc_camera_host *ici = to_soc_camera_host(dev);
	struct ak_camera_dev *pcdev = ici->priv;
	int ret, formats = 0;
	u32 code;
	const struct soc_mbus_pixelfmt *fmt;

	struct v4l2_subdev_mbus_code_enum code_enum = {
		.index = idx,
	};

	CAMDBG("entry %s\n", __func__);
	ret = v4l2_subdev_call(sd, pad, enum_mbus_code, NULL, &code_enum);
	if (ret < 0)
		/* No more formats */
		return 0;
	code = code_enum.code;

	/*
	 * What the ISP writes is NV12: a full-size Y plane followed by
	 * interleaved chroma at half resolution, 1.5 bytes per pixel. This
	 * driver only offered YUYV - packed 4:2:2, two bytes per pixel - so
	 * every caller sized its buffers a third too large and expected a
	 * layout the hardware never produces.
	 *
	 * YUYV stays second because the vendor library asks for it by name and
	 * has always lived with the mismatch; offering only NV12 makes its
	 * S_FMT fail. jpeg out is not offered either way.
	 */
	fmt = &ak_camera_nv12;
	CAMDBG("get format %s code=%d from sensor\n", fmt->name, code);

	/* Generic pass-through */
	formats++;
	if (xlate) {
		xlate->host_fmt	= fmt;
		xlate->code	= code;
		xlate++;

		/*
		  * @decide the default working mode of isp
		  * @prefer RGB mode
		  */
		if (code >= MEDIA_BUS_FMT_SBGGR8_1X8 && code <= MEDIA_BUS_FMT_SRGGB12_1X12) {
			pcdev->def_mode = ISP_RGB_VIDEO_OUT;
			//pcdev->def_mode = ISP_RGB_OUT;
		} else if (code >= MEDIA_BUS_FMT_Y8_1X8 &&
				code <= MEDIA_BUS_FMT_YVYU10_1X20) {
			pcdev->def_mode = ISP_YUV_VIDEO_OUT;
			//pcdev->def_mode = ISP_YUV_OUT;
		} else {
			pcdev->def_mode = ISP_RGB_VIDEO_OUT;
		}

		pcdev->cur_mode = pcdev->def_mode;
		pcdev->cur_mode_class = code;

		dev_dbg(dev, "Providing format %s in pass-through mode\n",
			fmt->name);
	}

	fmt = soc_mbus_get_fmtdesc(MEDIA_BUS_FMT_YUYV8_2X8);
	if (fmt) {
		formats++;
		if (xlate) {
			xlate->host_fmt	= fmt;
			xlate->code	= code;
			xlate++;
		}
	}

	return formats;
}

static void ak_camera_put_formats(struct soc_camera_device *icd)
{
	CAMDBG("entry %s\n", __func__);
	kfree(icd->host_priv);
	icd->host_priv = NULL;
	CAMDBG("leave %s\n", __func__);
}

/* Maybe belong platform code fix me */
static int ak_camera_set_bus_param(struct soc_camera_device *icd)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	struct v4l2_mbus_config cfg = {.type = V4L2_MBUS_PARALLEL,};
	unsigned long common_flags;
	int ret;

	CAMDBG("entry %s\n", __func__);

	/* AK39 supports 8bit and 10bit buswidth */
	ret = v4l2_subdev_call(sd, video, g_mbus_config, &cfg);
	if (!ret) {
		common_flags = soc_mbus_config_compatible(&cfg, CSI_BUS_FLAGS);
		if (!common_flags) {
			dev_warn(icd->parent,
				 "Flags incompatible: camera 0x%x, host 0x%x\n",
				 cfg.flags, CSI_BUS_FLAGS);
			return -EINVAL;
		}
	} else if (ret != -ENOIOCTLCMD) {
		return ret;
	} else {
		common_flags = CSI_BUS_FLAGS;
	}

	/* Make choises, based on platform choice */
	if ((common_flags & V4L2_MBUS_VSYNC_ACTIVE_HIGH) &&
		(common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW)) {
			if (pcdev->bus_flags & AK_CAMERA_VSYNC_HIGH)
				common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_LOW;
			else
				common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_HIGH;
	}

	if ((common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING) &&
		(common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
			if (pcdev->bus_flags & AK_CAMERA_PCLK_RISING)
				common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_FALLING;
			else
				common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_RISING;
	}

	if ((common_flags & V4L2_MBUS_DATA_ACTIVE_HIGH) &&
		(common_flags & V4L2_MBUS_DATA_ACTIVE_LOW)) {
			if (pcdev->bus_flags & AK_CAMERA_DATA_HIGH)
				common_flags &= ~V4L2_MBUS_DATA_ACTIVE_LOW;
			else
				common_flags &= ~V4L2_MBUS_DATA_ACTIVE_HIGH;
	}

	cfg.flags = common_flags;
	ret = v4l2_subdev_call(sd, video, s_mbus_config, &cfg);
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		dev_dbg(icd->parent, "camera s_mbus_config(0x%lx) returned %d\n",
			common_flags, ret);
		return ret;
	}

	CAMDBG("leave %s\n", __func__);

	return 0;
}

/**
 * @brief: register video buffer by video sub-system
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 * @param [in] *q: V4L2  buffer queue information structure
 */
static void ak_camera_init_videobuf(struct videobuf_queue *q,
			struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;

	CAMDBG("entry %s\n", __func__);

	videobuf_queue_dma_contig_init(q, &ak_videobuf_ops, icd->parent,
				&pcdev->lock, V4L2_BUF_TYPE_VIDEO_CAPTURE,
				V4L2_FIELD_NONE,
				sizeof(struct ak_buffer), icd, &ici->host_lock);

	CAMDBG("leave %s\n", __func__);
}

/**
 * @brief: request video buffer.
 *
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *icd: soc_camera_device information structure,
 * akcamera depends on the soc driver.
 * @param [in] *q: V4L2  buffer queue information structure
 */
static int ak_camera_reqbufs(struct soc_camera_device *icd,
				struct v4l2_requestbuffers *p)
{
	int i;
	//struct soc_camera_host *ici = to_soc_camera_host(icd->parent);

	CAMDBG("entry %s\n", __func__);

	/* This is for locking debugging only. I removed spinlocks and now I
	 * check whether .prepare is ever called on a linked buffer, or whether
	 * a dma IRQ can occur for an in-work or unlinked buffer. Until now
	 * it hadn't triggered */
	for (i = 0; i < p->count; i++) {
		struct ak_buffer *buf = container_of(icd->vb_vidq.bufs[i],
						      struct ak_buffer, vb);
		buf->inwork = 0;
		INIT_LIST_HEAD(&buf->vb.queue);
	}

	CAMDBG("leave %s\n", __func__);

	return 0;
}

/* platform independent */
static unsigned int ak_camera_poll(struct file *file, poll_table *pt)
{
	struct soc_camera_device *icd = file->private_data;
	struct ak_buffer *buf;

	buf = list_entry(icd->vb_vidq.stream.next, struct ak_buffer,
				vb.stream);

	poll_wait(file, &buf->vb.done, pt);

	if (buf->vb.state == VIDEOBUF_DONE ||
			buf->vb.state == VIDEOBUF_ERROR) {
		return POLLIN | POLLRDNORM;
	}

	return 0;
}

static int ak_camera_querycap(struct soc_camera_host *ici,
		                   struct v4l2_capability *cap)
{
	isp_dbg("entry %s\n", __func__);

	/* cap->name is set by the friendly caller:-> */
	strlcpy(cap->card, ak_cam_driver_description, sizeof(cap->card));
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

/*
 * Stock's only camera-host private parameter selects the ISP2 secondary
 * channel size. ISP attributes and sensor registers belong to /dev/isp_char.
 */
static int ak_camera_set_parm(struct soc_camera_device *icd,
			      struct v4l2_streamparm *parm)
{
	struct ak_camera_ch2_parm {
		int id;
		int width;
		int height;
	} *ch2 = (void *)parm->parm.raw_data;

	if (ch2->id != 1)
		return -EINVAL;

	return ispdrv_vo_set_sub_channel_scale(ch2->width, ch2->height);
}

static struct soc_camera_host_ops ak_soc_camera_host_ops = {
	.owner		= THIS_MODULE,
	.add			= ak_camera_add_device,
	.remove			= ak_camera_remove_device,
	.get_formats	= ak_camera_get_formats,
	.put_formats	= ak_camera_put_formats,
	.set_bus_param	= ak_camera_set_bus_param,
	.cropcap		= ak_camera_cropcap,
	.get_crop       = ak_camera_get_crop,
	.set_crop       = ak_camera_set_crop,
	.set_livecrop	= ak_camera_set_livecrop,
	.set_fmt		= ak_camera_set_fmt,
	.try_fmt		= ak_camera_try_fmt,
	.init_videobuf	= ak_camera_init_videobuf,
	.reqbufs		= ak_camera_reqbufs,
	.poll			= ak_camera_poll,
	.querycap		= ak_camera_querycap,
	.set_parm		= ak_camera_set_parm,
};

static int ak_camera_claim_pool(struct platform_device *pdev, size_t *bytes)
{
	struct device_node *np;
	struct resource res;
	int err;

	np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!np) {
		dev_err(&pdev->dev, "no memory-region for the capture pool\n");
		return -ENODEV;
	}

	err = of_address_to_resource(np, 0, &res);
	of_node_put(np);
	if (err) {
		dev_err(&pdev->dev, "capture pool has no usable reg\n");
		return err;
	}

	err = of_reserved_mem_device_init(&pdev->dev);
	if (err) {
		dev_err(&pdev->dev, "cannot claim the capture pool: %d\n", err);
		return err;
	}

	*bytes = resource_size(&res);
	dev_info(&pdev->dev, "capture pool at %pa, %zu KiB\n",
		 &res.start, *bytes / 1024);

	return 0;
}

static int ak_camera_probe(struct platform_device *pdev)
{
	struct ak_camera_dev *pcdev;
	struct clk *clk;
	unsigned int irq;
	size_t pool_bytes;
	int err = 0;

	CAMDBG("entry %s\n", __func__);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk("platform_get_irq | platform_get_resource\n");
		err = -ENODEV;
		goto exit;
	}

	err = ak_camera_claim_pool(pdev, &pool_bytes);
	if (err)
		goto exit;

	/*
	  * @get isp working clock
	  */
	clk = clk_get(&pdev->dev, "camera");
	if (IS_ERR(clk)) {
		err = PTR_ERR(clk);
		goto exit_release_pool;
	}

	/*
	** @allocate memory to struct ak_camera, including struct soc_camera_host
	** @and struct v4l2_device
	*/
	pcdev = kzalloc(sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev) {
		err = -ENOMEM;
		goto exit_put_clk;
	}

	/* @initailization for struct pcdev */
	INIT_LIST_HEAD(&pcdev->capture);
	spin_lock_init(&pcdev->lock);
	mutex_init(&pcdev->stream_lock);
	INIT_DELAYED_WORK(&pcdev->awb_work, isp_awb_work);
	INIT_DELAYED_WORK(&pcdev->ae_work, isp_ae_work);
	INIT_WORK(&pcdev->resume_work, ak_camera_resume_work);
//	pcdev->res = res;
	pcdev->clk = clk;
	pcdev->pool_bytes = pool_bytes;

	pcdev->list_state = LIST_ZERO;
	pcdev->free_list = LIST_ZERO;

	of_property_read_u32(pdev->dev.of_node, "mclk-mhz",
			      (u32 *)&pcdev->mclk);
	if (pcdev->mclk == 0) {
		dev_warn(&pdev->dev,
			 "mclk-mhz absent or 0; using default 24MHz\n");
		pcdev->mclk = 24;
	}
	of_property_read_u32(pdev->dev.of_node, "bus-flags",
			      (u32 *)&pcdev->bus_flags);

	/*
	 * DVP pin-mux. The "default" state is applied by the driver core at
	 * probe; keep the handle so set_sensor_interface() can re-assert it
	 * when a DVP sensor is opened. Absent pinctrl is not fatal.
	 */
	pcdev->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pcdev->pinctrl)) {
		dev_warn(&pdev->dev, "no pinctrl for DVP pads\n");
		pcdev->pinctrl = NULL;
		pcdev->pins_default = NULL;
	} else {
		pcdev->pins_default = pinctrl_lookup_state(pcdev->pinctrl,
							   PINCTRL_STATE_DEFAULT);
		if (IS_ERR(pcdev->pins_default)) {
			dev_warn(&pdev->dev, "no default DVP pinctrl state\n");
			pcdev->pins_default = NULL;
		}
	}
#ifndef CONFIG_CPU_AK3918EV200
	if (pcdev->pinctrl) {
		static const char * const names[] = { "mipi0_1lane",
						      "mipi0_2lane" };
		int i;

		for (i = 0; i < ARRAY_SIZE(names); i++) {
			pcdev->pins_mipi[i] = pinctrl_lookup_state(
						pcdev->pinctrl, names[i]);
			if (IS_ERR(pcdev->pins_mipi[i]))
				pcdev->pins_mipi[i] = NULL;
		}
	}
	{
		struct device_node *np = of_find_compatible_node(NULL, NULL,
					"anyka,ak39ev330-mipi-csi0");

		if (np) {
			pcdev->mipi_base = of_iomap(np, 0);
			pcdev->mipi_clk = of_clk_get_by_name(np, "mipi_csi_clk");
			if (IS_ERR(pcdev->mipi_clk))
				pcdev->mipi_clk = NULL;
			of_node_put(np);
		}
		if (!pcdev->mipi_base)
			dev_warn(&pdev->dev, "no MIPI/D-PHY block mapped\n");
	}
#endif

	/*
	  * request irq
	  */
	err = request_irq(irq, ak_camera_dma_irq, 0, "ak_camera", pcdev);
	if (err) {
		err = -EBUSY;
		goto exit_isp_fini;
	}
	pcdev->irq = irq;

	/*
	** @register soc_camera_host
	*/
	pcdev->soc_host.drv_name	= AK_CAM_DRV_NAME;
	pcdev->soc_host.ops		= &ak_soc_camera_host_ops;
	pcdev->soc_host.priv		= pcdev;
	pcdev->soc_host.v4l2_dev.dev	= &pdev->dev;
	pcdev->soc_host.nr		= pdev->id;

	err = soc_camera_host_register(&pcdev->soc_host);
	if (err) {
		goto exit_freeirq;
	}
	pcdev->cur_buf_id = -1;

	err = device_create_file(&pdev->dev, &dev_attr_irq_stats);
	if (err)
		goto exit_unregister_host;

	err = device_create_file(&pdev->dev, &dev_attr_queue_stats);
	if (err)
		goto exit_remove_irq_stats;

	if (device_create_file(&pdev->dev, &dev_attr_dma_coherent_pool))
		dev_warn(&pdev->dev, "no dma_coherent_pool attribute\n");

	dev_info(&pdev->dev, "AK Camera driver loaded\n");

	return 0;

exit_remove_irq_stats:
	device_remove_file(&pdev->dev, &dev_attr_irq_stats);
exit_unregister_host:
	soc_camera_host_unregister(&pcdev->soc_host);
exit_freeirq:
	free_irq(irq, pcdev);
exit_isp_fini:
	kfree(pcdev);
exit_put_clk:
	clk_put(clk);
exit_release_pool:
	of_reserved_mem_device_release(&pdev->dev);
exit:
	return err;
}

static int ak_camera_remove(struct platform_device *pdev)
{
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
	struct ak_camera_dev *pcdev = container_of(soc_host,
					struct ak_camera_dev, soc_host);

	CAMDBG("entry %s\n", __func__);

	device_remove_file(&pdev->dev, &dev_attr_dma_coherent_pool);
	device_remove_file(&pdev->dev, &dev_attr_queue_stats);
	device_remove_file(&pdev->dev, &dev_attr_irq_stats);

	soc_camera_host_unregister(soc_host);

	cancel_work_sync(&pcdev->resume_work);
	cancel_delayed_work_sync(&pcdev->awb_work);
	cancel_delayed_work_sync(&pcdev->ae_work);
	free_irq(pcdev->irq, pcdev);

	ak_camera_free_ref_frame(pcdev);

	/* free clk */
	clk_put(pcdev->clk);

	kfree(pcdev);

	of_reserved_mem_device_release(&pdev->dev);

	dev_info(&pdev->dev, "AK Camera driver unloaded\n");

	return 0;
}

static const struct of_device_id ak_camera_of_match[] = {
	{ .compatible = "anyka,ak39ev330-isp" },
	{ }
};
MODULE_DEVICE_TABLE(of, ak_camera_of_match);

static struct platform_driver ak_camera_driver = {
	.probe		= ak_camera_probe,
	.remove		= ak_camera_remove,
	.driver		= {
		.name = AK_CAM_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ak_camera_of_match,
	},
};

static int __init ak_camera_init(void)
{
	CAMDBG("entry %s\n", __func__);
	return platform_driver_register(&ak_camera_driver);
}

static void __exit ak_camera_exit(void)
{
	CAMDBG("entry %s\n", __func__);

	platform_driver_unregister(&ak_camera_driver);
}

module_init(ak_camera_init);
module_exit(ak_camera_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wu_daochao <wu_daochao@anyka.oa>");
MODULE_DESCRIPTION("Driver for ak Camera Interface");
