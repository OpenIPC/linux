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
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>

#include <asm/io.h>
#include <asm/cacheflush.h>

#include <media/soc_camera.h>
#include <media/videobuf-core.h>
#include <media/videobuf-dma-contig.h>
#include <media/soc_mediabus.h>

#include <mach/gpio.h>
#include <mach/clock.h>
#include <mach/reset.h>
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
	enum v4l2_mbus_pixelcode	code;
	int				inwork;
};

struct ak_camera_dev {
	struct soc_camera_host soc_host;
	struct soc_camera_device *icd;
	struct ak_camera_pdata		*pdata;

	struct clk	*clk;		// camera controller clk. it's parent is vclk defined in clock.c
	struct clk	*cis_sclk;		// cis_sclk clock for sensor
	struct clk	*mipi_clk;
	unsigned int	irq;
	struct list_head capture;	
	/* members to manage the dma and buffer*/
	spinlock_t		lock;  /* for videobuf_queue , passed in init_videobuf */
	
	enum isp_working_mode def_mode;
	enum isp_working_mode cur_mode;
	enum v4l2_mbus_pixelcode cur_mode_class;

	struct videobuf_queue  *vq;
	enum buffer_list_state list_state;
	enum buffer_list_state free_list;

	struct delayed_work awb_work;
	struct delayed_work ae_work;
	struct delayed_work af_work;

	int stream_ctrl_off;
	int cur_buf_id;

	int crop_x;
	int crop_y;
	int crop_width;
	int crop_height;
	int tmp_raw;
	int tmp_raw_buf_id;
	int app_capture_a_rawdata;

	int td_reset_count;
	int td_reset_frames;
};

struct ak_camera_cam {
	/* Client output, as seen by the CEU */
	unsigned int width;
	unsigned int height;
};

#define	ISP_TIMEOUT				(1)	//unit: s
#define EMPTY_FRAME_NUM			(2)

#define RAW_HEADER_SIZE 128

static const char *ak_cam_driver_description = "AK_Camera";

static int _tdnr_flag = 0;
static int _tdnr_set = 0;
static int video_frame_interval;

static unsigned long in_irq_jf = 0;

AK_ISP_SENSOR_CB *ak_sensor_get_sensor_cb(void);

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

	isp_dbg("%s (vb=0x%p) buf[%d] 0x%08lx %d\n", 
			__func__, vb, vb->i, vb->baddr, vb->bsize);
	
	BUG_ON(in_interrupt());

	/* This waits until this buffer is out of danger, i.e., until it is no
	 * longer in STATE_QUEUED or STATE_ACTIVE */
	if (vb->state == VIDEOBUF_ACTIVE && pcdev->list_state == LIST_ZERO) {
		printk("free_buffer: list_state=%d, doesn't neee to wait\n", pcdev->list_state);
		//vb->state = VIDEOBUF_ERROR;
		list_del(&vb->queue);
	} else {
		vb->state = VIDEOBUF_DONE;
		videobuf_waiton(vq, vb, 0, 0);
	}

	if (vq->streaming == 0 && vq->reading == 0) {
		spin_lock_irqsave(&pcdev->lock, flags);
		pcdev->list_state = LIST_ZERO;
		pcdev->free_list = LIST_ZERO;
		spin_unlock_irqrestore(&pcdev->lock, flags);

		if (pcdev->stream_ctrl_off == 0)
			ispdrv_set_isp_pause();

		if (!list_empty(&pcdev->capture)) {
			list_del_init(&pcdev->capture);
		}
		printk("%s.\n",__func__);
	}

	videobuf_dma_contig_free(vq, vb);

	vb->state = VIDEOBUF_NEEDS_INIT;
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
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
						icd->current_fmt->host_fmt);

	bytes_per_line = icd->user_width * 3 /2;
	if (bytes_per_line < 0)
		return bytes_per_line;

	*size = bytes_per_line * icd->user_height;
	//printk(KERN_ERR "%s size:%u, bytes_per_line:%d, icd->user_height:%d\n", __func__, *size, bytes_per_line, icd->user_height);

	if (*count < 3) {
		printk("if use video mode, vbuf num isn't less than 3\n");
		*count = 3;
	}

	if (*size * *count > CONFIG_VIDEO_RESERVED_MEM_SIZE)
		*count = (CONFIG_VIDEO_RESERVED_MEM_SIZE) / *size;
	
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
	WARN_ON(!list_empty(&vb->queue));

#if 0
//#ifdef ISP_DEBUG	
	/*	 
	* This can be useful if you want to see if we actually fill	 
	* the buffer with something	 
	*/
	memset((void *)vb->baddr, 0xaa, vb->bsize);
#endif

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

static int wait_for_1st_buffer_in_driver(struct ak_camera_dev *pcdev, int timeout)
{
	struct ak_buffer *ak_buf;
	struct videobuf_buffer *vb_1st;
	int get_1st = 0;

	do {
		list_for_each_entry(ak_buf, &pcdev->capture, vb.queue) {
			vb_1st = &ak_buf->vb;
			if (vb_1st->i == 0) {
				pr_err("this is 1st vb\n");
				get_1st = 1;
				break;
			}
		}

		if (get_1st)
			break;

		msleep(1);
	} while (timeout-- > 0);

	if (!get_1st) {
		pr_err("wait for 1st in driver fail\n");
		return -EBUSY;
	}

	pr_err("wait for 1st in driver success\n");
	return 0;
}

static int add_rawdata_header(struct ak_camera_dev *pcdev, struct videobuf_buffer *vb_raw)
{
	unsigned long paddr;
	void *vaddr;
	unsigned char *p;
	int raw_size;
	int raw_bitsw;
	int i;

	raw_bitsw = ispdrv_vo_get_inputdataw();
	raw_size = raw_bitsw * pcdev->crop_width * pcdev->crop_height / 8;

	paddr = videobuf_to_dma_contig(vb_raw);
	vaddr = ioremap_nocache(paddr, raw_size);
	if (!vaddr) {
		pr_err("ioremap to add raw header fail\n");
		return -ENOMEM;
	}

	p = vaddr;
	for (i = raw_size; i > 0; i--)
		*(p + i - 1 + RAW_HEADER_SIZE) = *(p + i - 1);

	memset(vaddr, 0, RAW_HEADER_SIZE);

	snprintf(vaddr, RAW_HEADER_SIZE, "AK-RAW,headsize:0x%04x,bitsw:0x%02x,size:0x%04x,w:0x%04x,h:0x%04x",
			RAW_HEADER_SIZE, raw_bitsw, raw_size, pcdev->crop_width, pcdev->crop_height);

	pr_err("raw-header:%s\n", (char *)vaddr);

	iounmap(vaddr);
	return 0;
}

static int copy_raw_to_1st_vb(struct ak_camera_dev *pcdev)
{
	struct ak_buffer *ak_active;
	struct ak_buffer *ak_buf;
	struct videobuf_buffer *vb_active;
	struct videobuf_buffer *vb_1st;
	int get_1st = 0;

	ak_active = list_entry(pcdev->capture.next,
			struct ak_buffer, vb.queue);
	vb_active = &ak_active->vb;

	list_for_each_entry(ak_buf, &pcdev->capture, vb.queue) {
		vb_1st = &ak_buf->vb;
		if (vb_1st->i == 0) {
			pr_err("this is 1st vb\n");
			get_1st = 1;
			break;
		}
	}

	if (!get_1st) {
		pr_err("can not 1st vb\n");
		return -ENOENT;
	}

	if (vb_active->i != 0) {
		unsigned long to_paddr;
		unsigned long from_paddr;
		int size;
		void *to_vaddr;
		void *from_vaddr;

		size = pcdev->crop_width * pcdev->crop_height * 3 / 2;

		to_paddr = videobuf_to_dma_contig(vb_active);
		to_vaddr = ioremap_nocache(to_paddr, size);
		if (!to_vaddr) {
			pr_err("ioremap to copy raw to fail\n");
			return -ENOMEM;
		}

		from_paddr = videobuf_to_dma_contig(vb_1st);
		from_vaddr = ioremap_nocache(from_paddr, size);
		if (!from_vaddr) {
			pr_err("ioremap to from raw fail\n");
			iounmap(to_vaddr);
			return -ENOMEM;
		}
		else
			memcpy(to_vaddr, from_vaddr, size);

		iounmap(to_vaddr);
		iounmap(from_vaddr);

		pr_err("copy raw success\n");
	} else {
		pr_err("it 1st vb already, not need copy\n");
	}

	return add_rawdata_header(pcdev, vb_active);
}

static void done_1st_vb(struct ak_camera_dev *pcdev)
{
	struct ak_buffer *ak_active;
	struct videobuf_buffer *vb;

	ak_active = list_entry(pcdev->capture.next,
			struct ak_buffer, vb.queue);
	vb = &ak_active->vb;

	ispdrv_vo_disable_buffer(BUFFER_ONE + vb->i);

	list_del_init(&vb->queue);
	vb->state = VIDEOBUF_DONE;
	vb->field_count++;

	wake_up(&vb->done);
}

static int continue_mode_capture_a_rawdata(struct ak_camera_dev *pcdev)
{
	int ret;

	/*1.pause isp*/
	//ispdev_set_isp_capturing(0);
	ispdrv_set_isp_pause();

	/*2.wait for 1st buffer in driver*/
	ret = wait_for_1st_buffer_in_driver(pcdev, 300);

	/*3.check if not 1st buffer in driver then show warning*/
	if (ret) {
		pr_err("not all buffers in driver\n");
	} else {
		int cnt = 100;

		/*4.set global flag*/
		pcdev->tmp_raw_buf_id = -1;
		pcdev->tmp_raw = 1;

		/*5.start capturing raw*/
		//ispdrv_vo_enable_buffer(BUFFER_ONE);
		ispdrv_vi_apply_mode(ISP_JPEG_MODE);
		//ispdrv_vi_start_capturing();
		//ispdev_set_isp_capturing(1);
		ispdrv_set_isp_resume();

		/*6.wait for captuing raw finish*/
		while ((pcdev->tmp_raw_buf_id < 0) &&
				(cnt-->0))
			msleep(1);

		/*7.check if capture raw finish, if not then show warning*/
		if (pcdev->tmp_raw_buf_id >= 0) {
			/*8.copy raw to 1st vb*/
			ret = copy_raw_to_1st_vb(pcdev);

			/*9.done 1st vb*/
			if (!ret)
				done_1st_vb(pcdev);
		} else {
			pr_err("error: capture rawdata fail\n");
		}

		/*10.reset global flag*/
		pcdev->tmp_raw_buf_id = -1;
		pcdev->tmp_raw = 0;
	}

	/*11.resume isp continue mode*/
	ispdrv_vi_apply_mode(pcdev->cur_mode);
	//ispdrv_vi_start_capturing();
	//ispdev_set_isp_capturing(1);
	ispdrv_set_isp_resume();

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
		ispdrv_vi_start_capturing();

		pcdev->list_state++;
		break;
	case LIST_ONE:
	case LIST_TWO:
	case LIST_THREE:
	//case LIST_FOUR:
		break;
	default:
		printk("Not defined list stat [single mode].\n");
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
#if 1
	switch (pcdev->list_state) {
	case LIST_FOUR:
		break;
	case LIST_ZERO:
		for (i = 0; i < 4; i++) {
			ispdrv_vo_disable_buffer(BUFFER_ONE + i);
		}

	case LIST_ONE:
	case LIST_TWO:
		ispdrv_vo_set_buffer_addr(BUFFER_ONE + vb->i, yaddr_chl1, yaddr_chl2);	
	case LIST_THREE:
		pcdev->list_state++;
		break;
	default:
		printk("Not defined list stat [continous mode].\n");
		break;
	}
#else
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
		printk("Not defined list stat [continous mode].\n");
		break;
	}
#endif

	spin_lock_irqsave(&pcdev->lock, flags);
	vb->state = VIDEOBUF_ACTIVE;
	list_add_tail(&vb->queue, &pcdev->capture);

	ispdrv_vo_enable_buffer(BUFFER_ONE + vb->i);
	pcdev->free_list++;
	spin_unlock_irqrestore(&pcdev->lock, flags);

#if 1
	if (pcdev->list_state == LIST_THREE) {
		ispdrv_vi_apply_mode(pcdev->cur_mode);
		ispdrv_vo_enable_irq_status(0x1);

		if (pcdev->stream_ctrl_off == 0) {
			//printk(KERN_ERR "%s start capture\n", __func__);
			ispdrv_vi_start_capturing();
		} else {
			//printk(KERN_ERR "%s resume capture\n", __func__);
			ispdrv_set_isp_resume();
		}
	}
#else
	if (pcdev->list_state == LIST_FOUR) {
		ispdrv_vi_apply_mode(pcdev->cur_mode);
		ispdrv_vo_enable_irq_status(0x1);

		if (pcdev->stream_ctrl_off == 0) {
			//printk(KERN_ERR "%s start capture\n", __func__);
			ispdrv_vi_start_capturing();
		} else {
			//printk(KERN_ERR "%s resume capture\n", __func__);
			ispdrv_set_isp_resume();
		}
	}
#endif

	
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
		printk("The working mode of ISP hasn't been initialized.\n");
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
	struct soc_camera_device *icd = vq->priv_data;
//	struct device *dev = icd->parent;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	unsigned long flags;
	
	isp_dbg("%s (vb=0x%p) buf[%d] 0x%08lx %d\n", 
			__func__, vb, vb->i, vb->baddr, vb->bsize);

	spin_lock_irqsave(&pcdev->lock, flags);
	ispdrv_vo_clear_irq_status(0xffff);	//?? 库还没有关闭irq函数
	spin_unlock_irqrestore(&pcdev->lock, flags);

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
	pcdev->stream_ctrl_off = 1;
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
	if (pcdev->cur_mode_class >= V4L2_MBUS_FMT_SBGGR8_1X8 &&
			pcdev->cur_mode_class <= V4L2_MBUS_FMT_SRGGB12_1X12) {
		if (1){//(pcdev->rfled_ison == 0) {
			schedule_delayed_work(&pcdev->awb_work, 0);
		}
		schedule_delayed_work(&pcdev->ae_work, 0);
//		schedule_delayed_work(&pcdev->af_work, 0);
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

	if (pcdev->tmp_raw) {
		/*process capuring raw in continue mode*/
		pcdev->tmp_raw_buf_id = vb->i;
		pr_err("irq raw\n");
		return 0;
	}

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
		ispdrv_vi_stop_capturing();
		pcdev->list_state = LIST_ZERO;
	}

	return 0;
}

static unsigned long sjf = 0;
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
	int fps = 10;
	AK_ISP_SENSOR_CB *sensor_cb;

	sensor_cb = ak_sensor_get_sensor_cb();
	if (NULL != sensor_cb) {
		fps = sensor_cb->sensor_get_fps_func();
	}
	//printk(KERN_ERR "fps:%d\n", fps);
	video_frame_interval = 1000 / fps + 10;

	ul = ul2 = jiffies;
	if (sjf == 0)
		sjf = jiffies;

	if (ul >= sjf)
		ul -= sjf;
	else
		ul = (~(unsigned long)0) - sjf+ ul;
	if (jiffies_to_msecs(ul) > video_frame_interval) {
		printk(KERN_ERR ">%d, sjf:%lu, ul:%lu\n", video_frame_interval, sjf, ul);
		_tdnr_flag = 1;
	}
	sjf = ul2;
	
	id = ispdrv_vo_get_using_frame_buf_id();
	//	printk("HWFrameId=%d\n", id);
//	isp2_print_reg_table();

	next = pcdev->capture.next;
	if (next == &pcdev->capture) {
		printk("Error, camera no buffer, but run to irq\n");
		ispdrv_vi_stop_capturing();
	}

	ak_buf = list_entry(next, struct ak_buffer, vb.queue);
	vb = &ak_buf->vb;
	if (id == -1)
	{
		if ((vb->field_count > 1) && (save_list != next )) {
			printk("%s %d: vb->i=%d, but id %d\n", __func__,__LINE__, vb->i, id);
			save_list = next;
		}
		return 0;
	} else if ((id & 0x7F) != vb->i) {
		printk("vb->i=%d, but id %d\n", vb->i, id);
		return 0;
	} else if (id >= 0x80) {
		if ((id & 0x7F) != vb->i) {
			printk("~~%s %d: vb->i=%d, but id %d\n", __func__,__LINE__, vb->i, id);
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
		printk("Warnning, lost frame at %ld\n", jiffies);
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
		ispdrv_vo_clear_irq_status(0xfffe);
		spin_unlock_irqrestore(&pcdev->lock, flags);
		printk("%s %d stat:0x%lx\n", __func__, __LINE__, stat);
		return IRQ_HANDLED;
//		goto out;
	}

	if (!pcdev->tmp_raw)
		ispdrv_irq_work();
	if (pcdev->list_state == LIST_ZERO)
	{
		printk("%s: state not handled\n", __func__);
		goto out;
	}
	
	if (!ispdrv_is_continuous()) { //?? 库没提供函数
		ak_active = list_entry(pcdev->capture.next,
						   struct ak_buffer, vb.queue);
		vb = &ak_active->vb;
		WARN_ON(ak_active->inwork || list_empty(&vb->queue));
		irq_handle_single_mode(vb, pcdev);
	} else {
		irq_handle_continous_mode(pcdev);
	}
	
	if (!pcdev->tmp_raw)
		delay_works(pcdev);

out:
	ispdrv_vo_clear_irq_status(0xffff);
	spin_unlock_irqrestore(&pcdev->lock, flags);
//	printk("%s stat:%x\n", __func__, stat);
	return IRQ_HANDLED;
}

/**
 * @brief: delay work queue, update image effects entry.
 * 
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *work: struct isp_struct
 */
static void isp_awb_work(struct work_struct *work) 
{

	ispdrv_awb_work();
}

static int ok_to_reload_td(struct ak_camera_dev *pcdev)
{
	if (pcdev->td_reset_frames > 0 ) {
		pcdev->td_reset_frames--;
		return 0;
	}
	return 1;
}

/**
 * @brief: delay work queue, update image effects entry.
 * 
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *work: struct isp_struct
 */
static void isp_ae_work(struct work_struct *work) 
{
	int aec_delay_ms = 10;
	unsigned long flags;
	struct ak_camera_dev *pcdev = container_of(work, struct ak_camera_dev, ae_work.work);
	AK_ISP_SENSOR_CB *sensor_cb;
	unsigned long cur_jf = jiffies;

	if (cur_jf >= in_irq_jf){
		int cur_fps = 25;
		int active_ms = 36;
		unsigned long diff_jf = cur_jf - in_irq_jf;

		sensor_cb = ak_sensor_get_sensor_cb();
		if (sensor_cb && sensor_cb->sensor_get_parameter_func) {
			sensor_cb->sensor_get_parameter_func(GET_CUR_FPS, &cur_fps);
			sensor_cb->sensor_get_parameter_func(GET_VSYNC_ACTIVE_MS, &active_ms);
			aec_delay_ms = MSEC_PER_SEC	/ cur_fps - active_ms + 1;
			if ((aec_delay_ms % 10) == 0)
				aec_delay_ms -= 1;
		}

		if (aec_delay_ms > diff_jf)
			aec_delay_ms = aec_delay_ms - (int)diff_jf;
		else
			aec_delay_ms = 0;
	
		usleep_range(aec_delay_ms * USEC_PER_MSEC, (aec_delay_ms + 1) * USEC_PER_MSEC);

		//printk(KERN_ERR "D%d\n",aec_delay_ms);
	}

	ispdrv_ae_work();

	/* get current buf id */
	if (pcdev->cur_buf_id != -1) {
		/* get tail addr of yuv and md info addr */
		void *yuv_paddr, *mdinfo, *yuv_vaddr = NULL;
		int md_sz = 24*32*2;
		int flip_en, mirror_en, height_block_num;
		int i,j;

		ispdrv_get_yuvaddr_and_mdinfo(pcdev->cur_buf_id, &yuv_paddr, &mdinfo);
		ispdrv_vo_get_flip_mirror(&flip_en, &mirror_en, &height_block_num);	

		/* cat to yuv's tail */
		yuv_vaddr = ioremap_nocache((unsigned long )yuv_paddr, md_sz);

		if (yuv_vaddr) {
			if (flip_en == 0 && mirror_en == 0) {
				memcpy(yuv_vaddr, mdinfo, md_sz);
			} else if (flip_en == 0 && mirror_en == 1) {
				for (i = 0; i < height_block_num; i++) {
					for (j = 0; j < 32; j++) {
						memcpy(yuv_vaddr + 2*(32*i + j), mdinfo + 2*(32*i + 31 - j), 2);
					}
				}
			} else if (flip_en == 1 && mirror_en == 0) {
				for (i = 0; i < height_block_num; i++) {
					memcpy(yuv_vaddr + 2*32*i, mdinfo + 2*32*(height_block_num - 1 - i), 2*32);
				}
			} else if (flip_en == 1 && mirror_en == 1) {
				for (i = 0; i < height_block_num; i++) {
					for (j = 0; j < 32; j++) {
						memcpy(yuv_vaddr + 2*(32*i + j), mdinfo + 2*(32*(height_block_num - 1 - i) + 31 - j), 2);
					}
				}
			}
			iounmap(yuv_vaddr);
		}
		
		pcdev->cur_buf_id = -1;
	}

	if (_tdnr_flag && !_tdnr_set) {
		ispdrv_set_td();
		spin_lock_irqsave(&pcdev->lock, flags);
		_tdnr_set = 1;
		_tdnr_flag = 0;
		pr_info("td_reset_count:%d\n", pcdev->td_reset_count);
		pcdev->td_reset_count++;
		if (pcdev->td_reset_count < 2)
			/*first td reset*/
			pcdev->td_reset_frames = 120;
		else
			pcdev->td_reset_frames = 5;
		spin_unlock_irqrestore(&pcdev->lock, flags);
	} else if (_tdnr_set && ok_to_reload_td(pcdev)) {
		ispdrv_reload_td();
		spin_lock_irqsave(&pcdev->lock, flags);
		_tdnr_set = 0;
		_tdnr_flag = 0;
		spin_unlock_irqrestore(&pcdev->lock, flags);
	}

	if (pcdev->app_capture_a_rawdata) {
		pcdev->app_capture_a_rawdata = 0;
		continue_mode_capture_a_rawdata(pcdev);
	}
}

/**
 * @brief: delay work queue, update image effects entry.
 * 
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *work: struct isp_struct
 */
static void isp_af_work(struct work_struct *work) 
{
	//ispdrv_af_work(isp);
}

static struct soc_camera_device *ctrl_to_icd(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct soc_camera_device,
							ctrl_handler);
}

/**
 * @brief: get function supported of camera, the function is image adjust, color effect...
 * 
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *ctrl: V4L2 image effect control information structure
 */
static int ak_camera_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
//	struct soc_camera_device *icd = ctrl_to_icd(ctrl);
//	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
//	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
//	struct ak_camera_dev *pcdev = ici->priv;
	
	isp_dbg("entry %s, ctrl->id=%x\n", __func__, ctrl->id);
	return 0;
}

/**
 * @brief: the isp standard control should be implemented here.
 * the function is image adjust, color effect...
 * 
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] *ctrl: V4L2 image effect control information structure
 */
static int ak_camera_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_control control;
	struct soc_camera_device *icd = ctrl_to_icd(ctrl);
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	
	control.id = ctrl->id;
	control.value = ctrl->val;
	v4l2_subdev_call(sd, core, s_ctrl, &control);
	
	return 0;
}


static const struct v4l2_ctrl_ops ak_camera_ctrl_ops = {
	.g_volatile_ctrl	= ak_camera_g_volatile_ctrl,
	.s_ctrl				= ak_camera_s_ctrl,
};

/**
 * @brief: set sensor clock
 * 
 * @author: caolianming
 * @date: 2014-01-06
 * @param [in] cis_sclk: sensor work clock
 */
static void set_sensor_cis_sclk(unsigned int cis_sclk)
{
	unsigned long regval;
	unsigned int cis_sclk_div;
	
	unsigned int peri_pll = ak_get_peri_pll_clk()/1000000;
	
	cis_sclk_div = peri_pll/cis_sclk - 1;

	regval = REG32(CLOCK_PERI_PLL_CTRL2);
	regval &= ~(0x3f << 10);
	regval |= (cis_sclk_div << 10);
	REG32(CLOCK_PERI_PLL_CTRL2) = (1 << 19)|regval;

	isp_dbg("%s() cis_sclk=%dMHz peri_pll=%dMHz cis_sclk_div=%d\n", 
			__func__, cis_sclk, peri_pll, cis_sclk_div);
}

static void set_dvp_initiation(int sensor_io_level)
{
	__raw_writeb(0x7d, AK_VA_MIPI + 0x00);
	__raw_writeb(0x3f, AK_VA_MIPI + 0x20);
	__raw_writeb(0x01, AK_VA_MIPI + 0xb3);
	switch (sensor_io_level) {
		case SENSOR_IO_LEVEL_1V8:
            __raw_writeb(0x10, AK_VA_MIPI + 0xb8);
			break;

		case SENSOR_IO_LEVEL_2V5:
			__raw_writeb(0x08, AK_VA_MIPI + 0xb8);
			break;
			
		case SENSOR_IO_LEVEL_3V3:
			__raw_writeb(0x04, AK_VA_MIPI + 0xb8);
			break;

		default:
			printk(KERN_ERR "%s sensor_io_level:%d not defined\n",
					__func__, sensor_io_level);
			break;
	}
}

static void set_mipi_2lanes_initiation(int thssettle)
{
	unsigned char value;

	printk(KERN_ERR "2lane thssettle:%d\n", thssettle);

	//mipi dphy data 0~3 and clk lane enable
	__raw_writeb(0x7d, AK_VA_MIPI + 0x00);
	printk(KERN_ERR "reg0x00, read:0x%02x",__raw_readb(AK_VA_MIPI));

	//ttl register bank disable
	__raw_writeb(0x1f, AK_VA_MIPI + 0x20);
	printk(KERN_ERR "reg0x20, read:0x%02x",__raw_readb(AK_VA_MIPI + 0x20));

	//two lane mode
	//REG8(MIPI_LANE_MODE_CFG_REG) = 0xf9;//0x204000e0
	__raw_writeb(0xf9, AK_VA_MIPI + 0xe0);
	printk(KERN_ERR "reg0xe0, read:0x%02x",__raw_readb(AK_VA_MIPI + 0xe0));

	//enable err
	//REG8(MIPI_ERR_ENA_CFG_REG) = 0x1;//0x204000e1
	__raw_writeb(0x01, AK_VA_MIPI + 0xe1);
	printk(KERN_ERR "reg0xe1, read:0x%02x",__raw_readb(AK_VA_MIPI + 0xe1));


	//THS - SETTLE    
	//REG8(MIPI_CLK_LANE_FREQ_CFG_REG) = 0x8b;//0x20400040
	//REG8(MIPI_DATA0_LANE_FREQ_CFG_REG) = 0x8b;//0x20400060
	//REG8(MIPI_DATA1_LANE_FREQ_CFG_REG) = 0x8b;//0x20400080
	value = 0x80 | (thssettle & 0x0f);
	__raw_writeb(value, AK_VA_MIPI + 0x40);
	printk(KERN_ERR "reg0x40, read:0x%02x",__raw_readb(AK_VA_MIPI + 0x40));
	__raw_writeb(value, AK_VA_MIPI + 0x60);
	printk(KERN_ERR "reg0x60, read:0x%02x",__raw_readb(AK_VA_MIPI + 0x60));
	__raw_writeb(value, AK_VA_MIPI + 0x80);
	printk(KERN_ERR "reg0x80, read:0x%02x",__raw_readb(AK_VA_MIPI + 0x80));

	//enable mipi mode  
	//REG8(MIPI_IO_MODE_REG) = 0x2;//0x204000b3
	__raw_writeb(0x02, AK_VA_MIPI + 0xb3);
	printk(KERN_ERR "reg0xb3, read:0x%02x",__raw_readb(AK_VA_MIPI + 0xb3));
}

static int mipi_mhz_to_thssettle(int mhz)
{
	int thssettle;

	if (mhz < 80){
		thssettle = -1;
		printk(KERN_ERR"%s fail, not support this min mbps:%d\n", __func__, mhz);
		return thssettle;
	}

	if (mhz <= 110)
		thssettle = 0;
	else if (mhz <= 150)
		thssettle = 1;
	else if (mhz <= 200)
		thssettle = 2;
	else if (mhz <= 250)
		thssettle = 3;
	else if (mhz <= 300)
		thssettle = 4;
	else if (mhz <= 400)
		thssettle = 5;
	else if (mhz <= 500)
		thssettle = 6;
	else if (mhz <= 600)
		thssettle = 7;
	else if (mhz <= 700)
		thssettle = 8;
	else if (mhz <= 800)
		thssettle = 9;
	else if (mhz <= 1000)
		thssettle = 0xa;
	else{
		thssettle = -1;
		printk(KERN_ERR"%s fail, not support this max mbps:%d\n", __func__, mhz);
	}

	return thssettle;
}

static void set_mipi_1lane_initiation(int thssettle)
{
	unsigned char value;

	printk(KERN_ERR "1lane thssettle:%d\n", thssettle);

	//THS - SETTLE    
	//REG8(MIPI_CLK_LANE_FREQ_CFG_REG) = 0x8b;//0x20400040
	//REG8(MIPI_DATA0_LANE_FREQ_CFG_REG) = 0x8b;//0x20400060
	value = 0x80 | (thssettle & 0x0f);
	__raw_writeb(value, AK_VA_MIPI + 0x40);
	printk(KERN_ERR "reg0x40, read:0x%02x",__raw_readb(AK_VA_MIPI + 0x40));
	__raw_writeb(value, AK_VA_MIPI + 0x60);
	printk(KERN_ERR "reg0x60, read:0x%02x",__raw_readb(AK_VA_MIPI + 0x60));

	__raw_writeb(0x7d, AK_VA_MIPI + 0x00);
	__raw_writeb(0xf8, AK_VA_MIPI + 0xe0);
}


struct ths_settle_info
{
	int max_mbps;
	int time_formula;
	int settle_value;
};

struct ths_settle_dphy
{
	int min_ths_settle;
	int max_ths_settle;
};

#define MIPI_IP_MAX_THS_SETTLE_NUM	11

/* this minst mbps must be more than 80MBPS */
struct ths_settle_info ths_settle[MIPI_IP_MAX_THS_SETTLE_NUM] = 
{
	{110, 4, 0x0},
	{150, 6, 0x1},
	{200, 9, 0x2},
	{250, 10, 0x3},
	{300, 13, 0x4},
	{400, 17, 0x5},
	{500, 23, 0x6},
	{600, 28, 0x7},
	{700, 33, 0x8},
	{800, 35, 0x9},
	{1000, 44, 0xa},
};

static int mipi_mbps_to_100times_ui(int mbps)
{
	int ui_ns = 0;

	ui_ns = 1000*100/mbps;

	return ui_ns;
}

static int mipi_thssettle_to_time(int thssettle, int mbps)
{
	int thssettle_time = 0;
	int thssettle_ui = 0;
	int i=0;
	int thssettle_iptime = 0;

	thssettle_ui = mipi_mbps_to_100times_ui(mbps);
	
	for(i=0; i<MIPI_IP_MAX_THS_SETTLE_NUM; i++){
		if(ths_settle[i].settle_value == thssettle){
			thssettle_time = 2 * ths_settle[i].time_formula * thssettle_ui;
			thssettle_iptime = thssettle_time/100;
			break;
		}
	}

	return thssettle_iptime;
}

static int mipi_thssettle_to_dphy_time(struct ths_settle_dphy *thssettle_dphy, int mbps)
{
	int thssettle_ui = 0;
	
	thssettle_ui = mipi_mbps_to_100times_ui(mbps);

	thssettle_dphy->min_ths_settle = 85 + (6 * thssettle_ui /100);
	thssettle_dphy->max_ths_settle = 145 + (10 * thssettle_ui /100);

	return 0;
}

static int mipi_thssettle_init(int ths_settle, int mbps)
{
	struct ths_settle_dphy thssettle_dphy_arrange;
	int thssettle_timebase = 0;
	int thssettle_timelast = 0;
	int i=0;

	/* first step:get dphy ths-settle arrange according mbps and ui */
	mipi_thssettle_to_dphy_time(&thssettle_dphy_arrange, mbps);

	/* second step:get mipi ip ths_settle base according from mbps 
	 * three step:adjust mipi ip ths_settle comply with dphy arrange 
	 */
	thssettle_timebase = mipi_thssettle_to_time(ths_settle, mbps);
	if(thssettle_timebase <= thssettle_dphy_arrange.min_ths_settle){ /* ths-settle < dphy_min */
		for(i=ths_settle; i<=MIPI_IP_MAX_THS_SETTLE_NUM; i++){
			thssettle_timebase = mipi_thssettle_to_time(i, mbps);
			if((thssettle_timebase>thssettle_dphy_arrange.min_ths_settle) && (thssettle_timebase<= thssettle_dphy_arrange.max_ths_settle)){
				/*get min suitable ths_settle */
				thssettle_timelast = i;
				break;
			}
		}
	}else if(thssettle_timebase >= thssettle_dphy_arrange.max_ths_settle){ /* ths-settle > dphy_max */
		for(i=ths_settle; i>=0; i--){
			thssettle_timebase = mipi_thssettle_to_time(i, mbps);
			if((thssettle_timebase>thssettle_dphy_arrange.min_ths_settle) && (thssettle_timebase<= thssettle_dphy_arrange.max_ths_settle)){
				/*get max suitable ths_settle */
				thssettle_timelast = i;
				break;
			}
		}
	}else{ /* dphy_min < ths-settle < dphy_max */
		thssettle_timelast = ths_settle;
	}
	
	if ((i > MIPI_IP_MAX_THS_SETTLE_NUM) || (i < 0)){
		pr_err("Err,no find suitable mipi ip ths-settle!\n");
		return -1;
	}

	return thssettle_timelast;

}

static int mipi_ip_initiation(void)
{
	int thssettle = 2, mipi_mhz = 200, mipi_lane = 2;
	AK_ISP_SENSOR_CB *sensor_cb;
	int thssettle_base;

	sensor_cb = ak_sensor_get_sensor_cb();
	if (sensor_cb && sensor_cb->sensor_get_parameter_func) {
		sensor_cb->sensor_get_parameter_func(GET_MIPI_MHZ, &mipi_mhz);
		sensor_cb->sensor_get_parameter_func(GET_MIPI_LANE, &mipi_lane);
	} else {
		printk(KERN_ERR "%s get mipi_mhz failed\n", __func__);
		return -1;
	}

	thssettle_base = mipi_mhz_to_thssettle(mipi_mhz);
	if(thssettle_base == -1){
		printk(KERN_ERR"%s fail, not support this mhz:%d\n", __func__, mipi_mhz);
		return -1;
	}else{
		printk(KERN_ERR"%s ok, thssettle_base:0x%x, support this mhz:%d\n", __func__, thssettle_base, mipi_mhz);
	}

	thssettle = mipi_thssettle_init(thssettle_base, mipi_mhz);
	if(thssettle == -1){
		printk(KERN_ERR"%s fail, not find suitable dphy mhz:%d\n", __func__, mipi_mhz);
		return -1;
	}else{
		printk(KERN_ERR"%s ok, thssettle:0x%x, find suitable dphy mhz:%d\n", __func__, thssettle, mipi_mhz);
	}
	
	if (mipi_lane == 2)
		set_mipi_2lanes_initiation(thssettle);
	else if (mipi_lane == 1)
		set_mipi_1lane_initiation(thssettle);
	
    __raw_writeb(0x04, AK_VA_MIPI + 0xb8);
	__raw_writeb(0xc0, AK_VA_MIPI + 0x0d);
    
	return 0;
}

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
	int cis_sclk;
	int sensor_interface = DVP_INTERFACE, sensor_io_level = SENSOR_IO_LEVEL_1V8;
	AK_ISP_SENSOR_CB *sensor_cb;
	
	CAMDBG("entry %s\n", __func__);

	/* The ak camera host driver only support one image sensor */
	if (pcdev->icd)
		return -EBUSY;

	dev_info(icd->parent, "AK Camera driver attached to camera %d\n",
		 icd->devnum);

	/* for debugging. Capture list should be empty when the video opened. */
	if (!list_empty(&pcdev->capture)) {
		printk("Bug: pcdev->capture is not empty\n");
		list_del_init(&pcdev->capture);
	}

	/********** config sensor module **********/
	v4l2_subdev_call(sd, core, init, 0);

	sensor_cb = ak_sensor_get_sensor_cb();
	if (NULL == sensor_cb) {
		cis_sclk = 24;
		//printk(KERN_ERR "%s get sensor_cb faild!!!!!!!\n", __func__);
	} else {
		cis_sclk = sensor_cb->sensor_get_mclk_func();
	}

	//set cis_sclk, the sensor present working 24MHz
	clk_enable(pcdev->cis_sclk);
	set_sensor_cis_sclk(cis_sclk);

	// load the default setting for sensor
//	v4l2_subdev_call(sd, core, load_fw);

	/********** config isp module **********/
	ak_soft_reset(AK_SRESET_CAMERA);

	// enable isp clock
	clk_enable(pcdev->clk);
//	printk("ISP CLOCK ENABLE \n");
	REG32(CLOCK_PERI_PLL_CTRL1) &=~(0x01<<25);

	if (NULL != sensor_cb) {
		sensor_cb->sensor_get_parameter_func(GET_INTERFACE, &sensor_interface);
		sensor_cb->sensor_get_parameter_func(GET_SENSOR_IO_LEVEL, &sensor_io_level);
	}

	switch (sensor_interface) {
		case MIPI_INTERFACE:
			/* set inter CIS_PCLK_tmp */
			ak_set_cis_pclk(75); 

			/* pclk -> cis2_sclk */
			REG32(AK_SHAREPIN_CON1) |= (0x1<<30) | (0x1<<4);

			/* asic clk reset mipi controller*/
			ak_soft_reset(AK39_SRESET_MIPI);

			//release mipi controller pin_byte_clk area resest
			ak_set_mipi_byte_rst(0);

			//select  pll to generate internal pclk for mipi
			ak_set_cis_pclk_sel(1);

			//select  mipi  sensor
			ak_set_cis_mipi_dvp_sel(1);

			//share pin config  mipi
			ak_group_config(ePIN_AS_MIPI);

			//open mipi controller clock gate
			clk_enable(pcdev->mipi_clk);

			//release mipi controller reset
			ak_soft_reset(AK39_SRESET_MIPI);

			mipi_ip_initiation();
			break;

		case DVP_INTERFACE:
			set_dvp_initiation(sensor_io_level);
			break;

		default:
			printk(KERN_ERR "%s interface:%d not defined\n",
					__func__, sensor_interface);
			break;
	}

	pcdev->icd = icd;
	pcdev->td_reset_count = 0;
	
	/* FIXME Here, add out control */
	if (!icd->host_priv) {
		/* FIXME: subwindow is lost between close / open */
		cam = kzalloc(sizeof(*cam), GFP_KERNEL);
		if (!cam)
		return -ENOMEM;

		/* We are called with current camera crop, initialise subrect with it */
		icd->host_priv = cam;
	} else {
		cam = icd->host_priv;
	}
	

	return 0;
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

	CAMDBG("entry %s\n", __func__);

	_tdnr_set = 0;
	_tdnr_flag = 0;

	BUG_ON(icd != pcdev->icd);

	v4l2_subdev_call(sd, core, reset, 0);

	ispdrv_vo_clear_irq_status(0xffff);	//?? 库还没有关闭irq函数
	ispdrv_vi_stop_capturing();

	mdelay(500);

	/* disable the clock of isp module */
	clk_disable(pcdev->clk);

	/* disable sensor clk */
	clk_disable(pcdev->cis_sclk);

	clk_disable(pcdev->mipi_clk);
	
	//ak_soft_reset(AK_SRESET_CAMERA);

	dev_info(icd->parent, "AK Camera driver detached from camera %d\n",
		 icd->devnum);	

	pcdev->list_state = LIST_ZERO;
	pcdev->free_list = LIST_ZERO;

	pcdev->icd = NULL;

	pcdev->stream_ctrl_off = 0;
	pcdev->td_reset_count = 0;

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
		                   struct v4l2_crop *crop)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop local_crop;
	struct v4l2_rect off_rect;
	int ret;
	int total_left, total_top;
	AK_ISP_SENSOR_CB *sensor_cb;

	isp_dbg("entry %s\n", __func__);
#if 0
	if (pcdev->dma_running) {
		/* make sure streaming is not started */
		v4l2_err(&ici->v4l2_dev,
				"Cannot change crop when streaming is ON\n");
		return -EBUSY;
	}
#endif

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

	pcdev->crop_x = total_left;
	pcdev->crop_y = total_top;
	pcdev->crop_width = crop->c.width;
	pcdev->crop_height = crop->c.height;

	local_crop.c.left = total_left;
	local_crop.c.top = total_top;
	local_crop.c.width = crop->c.width;
	local_crop.c.height = crop->c.height;
	return v4l2_subdev_call(sd, video, s_crop, &local_crop);
}

static int ak_camera_set_livecrop(struct soc_camera_device *icd, struct v4l2_crop *crop)
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
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	int ret;
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

	/* limit to sensor capabilities */
	ret = v4l2_subdev_call(sd, video, try_mbus_fmt, &mf);
	if (ret < 0) {
		return ret;
	}	

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
static int ak_camera_get_formats(struct soc_camera_device *icd, unsigned int idx,
				     struct soc_camera_format_xlate *xlate)
{
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct device *dev = icd->parent;
	struct soc_camera_host *ici = to_soc_camera_host(dev);
	struct ak_camera_dev *pcdev = ici->priv;
	int ret, formats = 0;
	enum v4l2_mbus_pixelcode code;
	const struct soc_mbus_pixelfmt *fmt;

	CAMDBG("entry %s\n", __func__);
	ret = v4l2_subdev_call(sd, video, enum_mbus_fmt, idx, &code);
	if (ret < 0)
		/* No more formats */
		return 0;

	/*
	  * @Note: ISP only support yuv420 output and jpeg out.
	  *	FIXME1: We miss jpeg here.
	  *  FIXME2: the output squence of YUV is actually UYVY.
	  */
	fmt = soc_mbus_get_fmtdesc(V4L2_MBUS_FMT_YUYV8_2X8);
	if (!fmt) {
		dev_warn(dev, "unsupported format code #%u: %d\n", idx, code);
		return 0;
	}
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
		if (code >= V4L2_MBUS_FMT_SBGGR8_1X8 && code <= V4L2_MBUS_FMT_SRGGB12_1X12) {
			pcdev->def_mode = ISP_RGB_VIDEO_OUT;
			//pcdev->def_mode = ISP_RGB_OUT;
		} else if (code >= V4L2_MBUS_FMT_Y8_1X8 && 
				code <= V4L2_MBUS_FMT_YVYU10_1X20) {
			pcdev->def_mode = ISP_YUV_VIDEO_OUT;
			//pcdev->def_mode = ISP_YUV_OUT;
			printk(KERN_ERR "set yuv video out\n");
		} else {
			pcdev->def_mode = ISP_RGB_VIDEO_OUT;
		}
		
#if 0
		if ((pcdev->def_mode != ISP_RGB_VIDEO_OUT)
			&& (pcdev->def_mode != ISP_RGB_OUT)) {
			pcdev->def_mode = ISP_RGB_VIDEO_OUT;
			//pcdev->def_mode = ISP_YUV_BYPASS;
		}
#endif
		pcdev->cur_mode = pcdev->def_mode;
   		pcdev->cur_mode_class = code;

		dev_dbg(dev, "Providing format %s in pass-through mode\n",
			fmt->name);
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
			if (!pcdev->pdata ||
			     pcdev->pdata->flags & AK_CAMERA_VSYNC_HIGH)
				common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_LOW;
			else
				common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_HIGH;
	}

	if ((common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING) &&
		(common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
			if (!pcdev->pdata ||
			     pcdev->pdata->flags & AK_CAMERA_PCLK_RISING)
				common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_FALLING;
			else
				common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_RISING;
	}

	if ((common_flags & V4L2_MBUS_DATA_ACTIVE_HIGH) &&
		(common_flags & V4L2_MBUS_DATA_ACTIVE_LOW)) {
			if (!pcdev->pdata ||
			     pcdev->pdata->flags & AK_CAMERA_DATA_HIGH)
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
				sizeof(struct ak_buffer), icd, &icd->video_lock);

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

/**
 * @brief: The private interface of ISP for application user.
 * setting ISP controller for image information
 *
 * @author: 
 * @date: 
 * @param 
 * akcamera depends on the soc driver.
 * @param 
 */
static int ak_camera_set_parm(struct soc_camera_device *icd,
		struct v4l2_streamparm *a)
{
	int ret = 0;
	int *parm_type;
	enum camera_pcid pcid;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct ak_camera_dev *pcdev = ici->priv;

	isp_dbg("entry %s\n", __func__);

	parm_type = (int *)a->parm.raw_data;
	pcid = *parm_type;
	printk(KERN_ERR "%s pcid:%d\n", __func__, pcid);
	switch(pcid) {
		case PCID_CH2_OUTPUT_FMT:
			{
				struct pcid_ch2_output_fmt_data *ch2_fmt = (void *)a->parm.raw_data;

				ret = ispdrv_vo_set_sub_channel_scale(ch2_fmt->width, ch2_fmt->height);
				printk(KERN_ERR "%s pcid:%d, ch2w:%d, ch2h:%d\n", __func__, pcid, ch2_fmt->width, ch2_fmt->height);
			}
			break;

		case PCID_A_FRAME_RAW:
			//ret = continue_mode_capture_a_rawdata(pcdev);
			pcdev->app_capture_a_rawdata = 1;
			ret = 0;//continue_mode_capture_a_rawdata(pcdev);
			if (ret) {
				pr_err("PCID_A_FRAME_RAW: fail\n");
			}
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
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

static int ak_camera_probe(struct platform_device *pdev)
{
	struct ak_camera_dev *pcdev;
	struct clk *clk, *cis_sclk, *mipi_clk;
	unsigned int irq;
	int err = 0;

	CAMDBG("entry %s\n", __func__);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk("platform_get_irq | platform_get_resource\n");
		err = -ENODEV;
		goto exit;
	}

	mipi_clk = clk_get(&pdev->dev, "mipi");
	if (IS_ERR(mipi_clk)) {
		printk(KERN_ERR "%s get mipi asic clk error\n", __func__);
		err = PTR_ERR(mipi_clk);
		goto exit;
	}

	/*
	  * @get isp working clock 
	  */
	clk = clk_get(&pdev->dev, "camera");
	if (IS_ERR(clk)) {
		err = PTR_ERR(clk);
		goto exit;
	}

	/*
	  * @get cis_sclk for sensor
	  */
	cis_sclk = clk_get(&pdev->dev, "sensor");
	if (IS_ERR(cis_sclk)) {
		err = PTR_ERR(cis_sclk);
		goto exit_put_clk;
	}

	/* 
	** @allocate memory to struct ak_camera, including struct soc_camera_host
	** @and struct v4l2_device 
	*/
	pcdev = kzalloc(sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev) {
		err = -ENOMEM;
		goto exit_put_cisclk;
	}

	/* @initailization for struct pcdev */
	INIT_LIST_HEAD(&pcdev->capture);
//	pcdev->res = res;
	pcdev->clk = clk;
	pcdev->cis_sclk = cis_sclk;
	pcdev->mipi_clk = mipi_clk;

	pcdev->list_state = LIST_ZERO;
	pcdev->free_list = LIST_ZERO;

	pcdev->pdata = pdev->dev.platform_data;
	if (!pcdev->pdata) {
		err = -ENODEV;
		goto exit_isp_fini;
	}
	
	/*
	  * request irq 
	  */	
	err = request_irq(irq, ak_camera_dma_irq, IRQF_DISABLED, "ak_camera", pcdev);
	if (err) {
		err = -EBUSY;
		goto exit_isp_fini;
	}
	pcdev->irq = irq;

	INIT_DELAYED_WORK(&pcdev->awb_work, isp_awb_work);
	INIT_DELAYED_WORK(&pcdev->ae_work, isp_ae_work);
	INIT_DELAYED_WORK(&pcdev->af_work, isp_af_work);
	
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

	dev_info(&pdev->dev, "AK Camera driver loaded\n");

	return 0;
	
exit_freeirq:
	free_irq(irq, pcdev);
exit_isp_fini:
	kfree(pcdev);
exit_put_cisclk:
	clk_put(cis_sclk);
exit_put_clk:
	clk_put(clk);
exit:
	return err;
}

static int ak_camera_remove(struct platform_device *pdev)
{
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
	struct ak_camera_dev *pcdev = container_of(soc_host,
					struct ak_camera_dev, soc_host);

	CAMDBG("entry %s\n", __func__);

	soc_camera_host_unregister(soc_host);

	free_irq(pcdev->irq, pcdev);

	kfree(pcdev);

	/* free clk */
	clk_put(pcdev->cis_sclk);
	clk_put(pcdev->clk);

	dev_info(&pdev->dev, "AK Camera driver unloaded\n");
	
	return 0;
}

static struct platform_driver ak_camera_driver = {
	.probe		= ak_camera_probe,
	.remove		= ak_camera_remove, 	
	.driver		= {
		.name = AK_CAM_DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ak_camera_init(void)
{
	int ret;

	CAMDBG("entry %s\n", __func__);

	ret = aksensor_module_init();
	if (ret)
		return ret;

	return platform_driver_register(&ak_camera_driver);
}

static void __exit ak_camera_exit(void)
{
	CAMDBG("entry %s\n", __func__);

	aksensor_module_exit();

	platform_driver_unregister(&ak_camera_driver);
}

module_init(ak_camera_init);
module_exit(ak_camera_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wu_daochao <wu_daochao@anyka.oa>");
MODULE_DESCRIPTION("Driver for ak Camera Interface");

