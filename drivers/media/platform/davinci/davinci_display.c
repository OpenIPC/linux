/*
 * Copyright (C) 2009 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/gcd.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/pgtable.h>
#include <mach/cputype.h>
#include <media/davinci/davinci_enc.h>
#include <media/davinci/davinci_display.h>

#define DAVINCI_DISPLAY_DRIVER "davinci_v4l2"
#define DM355_EVM_CARD  "DM355 EVM"
#define DM644X_EVM_CARD "DM644X EVM"
#define DM365_EVM_CARD  "DM365 EVM"

static u32 video2_numbuffers = 3;
static u32 video3_numbuffers = 3;
static u32 cont2_bufoffset = 0;
static u32 cont2_bufsize = 0;
static u32 cont3_bufoffset = 0;
static u32 cont3_bufsize = 0;

#define DAVINCI_DISPLAY_HD_BUF_SIZE (1280*720*2)
#define DAVINCI_DISPLAY_SD_BUF_SIZE (720*576*2)

static u32 video2_bufsize = DAVINCI_DISPLAY_HD_BUF_SIZE;
static u32 video3_bufsize = DAVINCI_DISPLAY_SD_BUF_SIZE;

module_param(video2_numbuffers, uint, S_IRUGO);
module_param(video3_numbuffers, uint, S_IRUGO);

module_param(video2_bufsize, uint, S_IRUGO);
module_param(video3_bufsize, uint, S_IRUGO);
module_param(cont2_bufoffset, uint, S_IRUGO);
module_param(cont2_bufsize, uint, S_IRUGO);
module_param(cont3_bufoffset, uint, S_IRUGO);
module_param(cont3_bufsize, uint, S_IRUGO);

MODULE_PARM_DESC(cont2_bufoffset, "Display offset (default 0)");
MODULE_PARM_DESC(cont2_bufsize, "Display buffer size (default 0)");
MODULE_PARM_DESC(cont3_bufoffset, "Display offset (default 0)");
MODULE_PARM_DESC(cont3_bufsize, "Display buffer size (default 0)");


#define DAVINCI_DEFAULT_NUM_BUFS 3
static struct buf_config_params display_buf_config_params = {
	.min_numbuffers = DAVINCI_DEFAULT_NUM_BUFS,
	.numbuffers[0] = DAVINCI_DEFAULT_NUM_BUFS,
	.numbuffers[1] = DAVINCI_DEFAULT_NUM_BUFS,
	.min_bufsize[0] = DAVINCI_DISPLAY_HD_BUF_SIZE,
	.min_bufsize[1] = DAVINCI_DISPLAY_SD_BUF_SIZE,
	.layer_bufsize[0] = DAVINCI_DISPLAY_HD_BUF_SIZE,
	.layer_bufsize[1] = DAVINCI_DISPLAY_SD_BUF_SIZE,
};

static int davinci_display_nr[] = { 2, 3 };

/* global variables */
static struct davinci_display davinci_dm;

struct device *davinci_display_dev = NULL;

static struct v4l2_capability davinci_display_videocap = {
	.driver = DAVINCI_DISPLAY_DRIVER,
	.bus_info = "Platform",
	.version = DAVINCI_DISPLAY_VERSION_CODE,
	.capabilities = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING
};

static struct v4l2_fract ntsc_aspect = DAVINCI_DISPLAY_PIXELASPECT_NTSC;
static struct v4l2_fract pal_aspect = DAVINCI_DISPLAY_PIXELASPECT_PAL;
static struct v4l2_fract sp_aspect = DAVINCI_DISPLAY_PIXELASPECT_SP;

static struct v4l2_rect ntsc_bounds = DAVINCI_DISPLAY_WIN_NTSC;
static struct v4l2_rect pal_bounds = DAVINCI_DISPLAY_WIN_PAL;
static struct v4l2_rect vga_bounds = DAVINCI_DISPLAY_WIN_640_480;
static struct v4l2_rect hd_720p_bounds = DAVINCI_DISPLAY_WIN_720P;
static struct v4l2_rect hd_1080i_bounds = DAVINCI_DISPLAY_WIN_1080I;

/*
 * davinci_buffer_prepare()
 * This is the callback function called from videobuf_qbuf() function
 * the buffer is prepared and user space virtual address is converted into
 * physical address
 */
static int davinci_buffer_prepare(struct videobuf_queue *q,
				  struct videobuf_buffer *vb,
				  enum v4l2_field field)
{

	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	unsigned long addr;
	int ret = 0;

	dev_dbg(davinci_display_dev, "<davinci_buffer_prepare>\n");

	/* If buffer is not initialized, initialize it */
	if (VIDEOBUF_NEEDS_INIT == vb->state) {
		vb->width = layer->pix_fmt.width;
		vb->height = layer->pix_fmt.height;
		vb->size = layer->pix_fmt.sizeimage;
		vb->field = field;

		ret = videobuf_iolock(q, vb, NULL);
		if (ret < 0) {
			dev_err(davinci_display_dev, "Failed to map \
				user address\n");
			goto buf_align_exit;
		}

		addr = videobuf_to_dma_contig(vb);

		if (q->streaming) {
			if (!ISALIGNED(addr)) {
				dev_err(davinci_display_dev, "buffer_prepare:offset is \
					not aligned to 32 bytes\n");
				goto buf_align_exit;
			}
		}
		vb->state = VIDEOBUF_PREPARED;
	}

	return 0;

buf_align_exit:
	return -EINVAL;
}
/*
 * davinci_buffer_setup()
 * This function allocates memory for the buffers
 */
static int davinci_buffer_setup(struct videobuf_queue *q, unsigned int *count,
				unsigned int *size)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	int buf_size;

	dev_dbg(davinci_display_dev, "<davinci_buffer_setup>\n");

	*size = layer->pix_fmt.sizeimage;
	buf_size = display_buf_config_params.layer_bufsize[layer->device_id];

	/**
	 * For MMAP, limit the memory allocation as per bootarg
	 * configured buffer size
	 */
	if (V4L2_MEMORY_MMAP == layer->memory)
		if (*size > buf_size)
			*size = buf_size;

	/* Checking if the buffer size exceeds the available buffer */
	if (display_buf_config_params.video_limit[layer->device_id]) {
		while (*size * *count > (display_buf_config_params.video_limit[layer->device_id]))
			(*count)--;
	}

	/* Store number of buffers allocated in numbuffer member */
	if (*count < display_buf_config_params.min_numbuffers)
		*count = layer->numbuffers = display_buf_config_params.numbuffers[layer->device_id];

	dev_dbg(davinci_display_dev, "</davinci_buffer_setup>\n");

	return 0;
}

/*
 * davinci_buffer_queue()
 * This function adds the buffer to DMA queue
 */
static void davinci_buffer_queue(struct videobuf_queue *q,
				 struct videobuf_buffer *vb)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	dev_dbg(davinci_display_dev, "<davinci_buffer_queue>\n");

	/* add the buffer to the DMA queue */
	list_add_tail(&vb->queue, &layer->dma_queue);
	/* Change state of the buffer */
	vb->state = VIDEOBUF_QUEUED;
	dev_dbg(davinci_display_dev, "</davinci_buffer_queue>\n");
}

/*
 * davinci_buffer_release()
 * This function is called from the videobuf layer to free memory allocated to
 * the buffers
 */
static void davinci_buffer_release(struct videobuf_queue *q,
				   struct videobuf_buffer *vb)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	unsigned int buf_size = 0;
	dev_dbg(davinci_display_dev, "<davinci_buffer_release>\n");

	if (V4L2_MEMORY_USERPTR != layer->memory)
		videobuf_dma_contig_free(q, vb);

	vb->state = VIDEOBUF_NEEDS_INIT;


	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != layer->memory)
		return;
	/* Calculate the size of the buffer */
	buf_size = display_buf_config_params.layer_bufsize[layer->device_id];

	dev_dbg(davinci_display_dev, "</davinci_buffer_release>\n");
}

/* davinci_frames_to_process()
 * Return the number of frames to process to achieve the desired frame rate.
 * If 0 is returned, the current displayed frame needs to be repeated.
 * If >1 is returned, all buffers need to be skipped except the last one,
 * which is displayed.
 */
static int davinci_frames_to_process(struct display_obj *layer)
{
	struct davinci_timeperframe_info *tpfInfo = &(layer->tpf_info);
	unsigned long framesToSkip;

	/* If we are not simulating a frame rate, we always process exactly
	 * one frame. */
	if (tpfInfo->irq_service_rate == 0)
		return 1;

	/* Increment the IRQ counter by 1.000 */
	tpfInfo->irq_count += 1000;

	/* If we haven't reached the next threshold for processing a frame,
	 * we must repeat the current frame (return 0).  This hapens when
	 * simulating a framerate lower than the hardware framerate.
	 */
	if (tpfInfo->irq_count < tpfInfo->next_irq)
		return 0;

	/* In cases where we're simulating a framerate higher than the hardware
	 * we may need to dequeue additional buffers and throw them away. */
	framesToSkip = 0;
	while (tpfInfo->next_irq + tpfInfo->irq_service_rate <=
		tpfInfo->irq_count) {

		tpfInfo->next_irq += tpfInfo->irq_service_rate;
		framesToSkip++;
	}

	tpfInfo->next_irq += tpfInfo->irq_service_rate;
	tpfInfo->next_irq -= tpfInfo->irq_count;
	tpfInfo->irq_count = 0;

	return 1 + framesToSkip;
}

static struct videobuf_queue_ops video_qops = {
	.buf_setup = davinci_buffer_setup,
	.buf_prepare = davinci_buffer_prepare,
	.buf_queue = davinci_buffer_queue,
	.buf_release = davinci_buffer_release,
};

static u8 layer_first_int = 1;

/* davinci_display_isr()
 * ISR function. It changes status of the displayed buffer, takes next buffer
 * from the queue and sets its address in VPBE registers
 */
static void davinci_display_isr(unsigned int event, void *dispObj)
{
	unsigned long jiffies_time = get_jiffies_64();
	struct timeval timevalue;
	int i, fid;
	unsigned long addr = 0;
	struct display_obj *layer = NULL;
	struct davinci_display *dispDevice = (struct davinci_display *)dispObj;

	/* Convert time represention from jiffies to timeval */
	jiffies_to_timeval(jiffies_time, &timevalue);

	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		layer = dispDevice->dev[i];
		/* If streaming is started in this layer */
		if (!layer->started)
			continue;
		/* Query the number of frames to process in this ISR call to
		 * simulate the desired frame rate.  Right now, we only support
		 * simulated frame rates less than the hardware framerate, so
		 * this value will always be 0 or 1.  When rates faster than
		 * the hardware rate are supported, * it can be > 1 in which
		 * case we need to dequeue and throw away (skip) additional
		 * buffers.  Right now, all we do is repeat the current frame
		 * when it is 0.
		 */
		if (davinci_frames_to_process(layer) == 0)
			continue;
		/* Check the field format */
		if ((V4L2_FIELD_NONE == layer->pix_fmt.field) &&
		    (!list_empty(&layer->dma_queue)) &&
		    (event & DAVINCI_DISP_END_OF_FRAME)) {
			/* Progressive mode */
			if (layer_first_int) {
				layer_first_int = 0;
				continue;
			} else {
				/* Mark status of the curFrm to
				 * done and unlock semaphore on it
				 */
				if (layer->curFrm != layer->nextFrm) {
					layer->curFrm->ts = timevalue;
					layer->curFrm->state = VIDEOBUF_DONE;
					wake_up_interruptible(&layer->curFrm->done);
					/* Make curFrm pointing to nextFrm */
					layer->curFrm = layer->nextFrm;
				}
			}
			/* Get the next buffer from buffer queue */
			layer->nextFrm =
			    list_entry(layer->dma_queue.next,
				       struct videobuf_buffer, queue);
			/* Remove that buffer from the buffer queue */
			list_del(&layer->nextFrm->queue);
			/* Mark status of the buffer as active */
			layer->nextFrm->state = VIDEOBUF_ACTIVE;
			
			addr = videobuf_to_dma_contig(layer->nextFrm);
			davinci_disp_start_layer(layer->layer_info.id,
						 addr,
						 &davinci_dm.fb_desc);
		} else {
			/* Interlaced mode
			 * If it is first interrupt, ignore it
			 */
			if (layer_first_int) {
				layer_first_int = 0;
				return;
			}

			layer->field_id ^= 1;
			if (event & DAVINCI_DISP_FIRST_FIELD)
				fid = 0;
			else if (event & DAVINCI_DISP_SECOND_FIELD)
				fid = 1;
			else
				return;

			/* If field id does not match with stored
			 * field id
			 */
			if (fid != layer->field_id) {
				/* Make them in sync */
				if (0 == fid) {
					layer->field_id = fid;
				}
				return;
			}
			/* device field id and local field id are
			 * in sync. If this is even field
			 */
			if (0 == fid) {
				if (layer->curFrm == layer->nextFrm)
					continue;
				/* one frame is displayed If next frame is
				 * available, release curFrm and move on
				 * Copy frame display time
				 */
				layer->curFrm->ts = timevalue;
				/* Change status of the curFrm */
				layer->curFrm->state = VIDEOBUF_DONE;
				/* unlock semaphore on curFrm */
				wake_up_interruptible(&layer->curFrm->done);
				/* Make curFrm pointing to
				 * nextFrm
				 */
				layer->curFrm = layer->nextFrm;
			} else if (1 == fid) {	/* odd field */
				if (list_empty(&layer->dma_queue)
				    || (layer->curFrm != layer->nextFrm))
					continue;

				/* one field is displayed configure
				 * the next frame if it is available
				 * otherwise hold on current frame
				 * Get next from the buffer queue
				 */
				layer->nextFrm = list_entry(layer->
							    dma_queue.
							    next, struct
							    videobuf_buffer,
							    queue);

				/* Remove that from the
				 * buffer queue
				 */
				list_del(&layer->nextFrm->queue);

				/* Mark state of the frame
				 * to active
				 */
				layer->nextFrm->state = VIDEOBUF_ACTIVE;

				addr = videobuf_to_dma_contig(layer->nextFrm);
				davinci_disp_start_layer(layer->layer_info.id,
							addr,
							&davinci_dm.fb_desc);
			}
		}
	}
}

static struct display_obj*
_davinci_disp_get_other_win(struct display_obj *layer)
{
	enum davinci_display_device_id thiswin, otherwin;
	thiswin = layer->device_id;

	otherwin = (thiswin == DAVINCI_DISPLAY_DEVICE_0) ?
		DAVINCI_DISPLAY_DEVICE_1 : DAVINCI_DISPLAY_DEVICE_0;
	return davinci_dm.dev[otherwin];
}

static int davinci_config_layer(enum davinci_display_device_id id);

static int davinci_set_video_display_params(struct display_obj *layer)
{
	unsigned long addr;

	addr = videobuf_to_dma_contig(layer->curFrm);
	/* Set address in the display registers */
	davinci_disp_start_layer(layer->layer_info.id,
				 addr,
				 &davinci_dm.fb_desc);
	davinci_disp_enable_layer(layer->layer_info.id, 0);
	/* Enable the window */
	layer->layer_info.enable = 1;
	if (layer->layer_info.config.pixfmt == PIXFMT_NV12) {
		struct display_obj *otherlayer =
			_davinci_disp_get_other_win(layer);
		davinci_disp_enable_layer(otherlayer->layer_info.id, 1);
		otherlayer->layer_info.enable = 1;
	}
	return 0;
}

static void davinci_disp_calculate_scale_factor(struct display_obj *layer,
						int expected_xsize,
						int expected_ysize)
{
	struct display_layer_info *layer_info = &layer->layer_info;
	struct v4l2_pix_format *pixfmt = &layer->pix_fmt;
	int h_scale = 0, v_scale = 0, h_exp = 0, v_exp = 0, temp;
	/* Application initially set the image format. Current display
	 * size is obtained from the encoder manager. expected_xsize
	 * and expected_ysize are set through S_CROP ioctl. Based on this,
	 * driver will calculate the scale factors for vertical and
	 * horizontal direction so that the image is displayed scaled
	 * and expanded. Application uses expansion to display the image
	 * in a square pixel. Otherwise it is displayed using displays
	 * pixel aspect ratio.It is expected that application chooses
	 * the crop coordinates for cropped or scaled display. if crop
	 * size is less than the image size, it is displayed cropped or
	 * it is displayed scaled and/or expanded.
	 *
	 * to begin with, set the crop window same as expected. Later we
	 * will override with scaled window size
	 */
	layer->layer_info.config.xsize = pixfmt->width;
	layer->layer_info.config.ysize = pixfmt->height;
	layer_info->h_zoom = ZOOM_X1;	/* no horizontal zoom */
	layer_info->v_zoom = ZOOM_X1;	/* no horizontal zoom */
	layer_info->h_exp = H_EXP_OFF;	/* no horizontal zoom */
	layer_info->v_exp = V_EXP_OFF;	/* no horizontal zoom */

	if (pixfmt->width < expected_xsize) {
		h_scale = davinci_dm.mode_info.xres / pixfmt->width;
		if (h_scale < 2)
			h_scale = 1;
		else if (h_scale >= 4)
			h_scale = 4;
		else
			h_scale = 2;
		layer->layer_info.config.xsize *= h_scale;
		if (layer->layer_info.config.xsize < expected_xsize) {
			if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_NTSC)
			    || !strcmp(davinci_dm.mode_info.name,
				       VID_ENC_STD_PAL)) {
				temp =
				    (layer->layer_info.config.xsize *
				     DAVINCI_DISPLAY_H_EXP_RATIO_N)
				    / DAVINCI_DISPLAY_H_EXP_RATIO_D;
				if (temp <= expected_xsize) {
					h_exp = 1;
					layer->layer_info.config.xsize = temp;
				}
			}
		}
		if (h_scale == 2)
			layer_info->h_zoom = ZOOM_X2;
		else if (h_scale == 4)
			layer_info->h_zoom = ZOOM_X4;
		if (h_exp)
			layer_info->h_exp = H_EXP_9_OVER_8;
	} else {
		/* no scaling, only cropping. Set display area to crop area */
		layer->layer_info.config.xsize = expected_xsize;
	}

	if (pixfmt->height < expected_ysize) {
		v_scale = expected_ysize / pixfmt->height;
		if (v_scale < 2)
			v_scale = 1;
		else if (v_scale >= 4)
			v_scale = 4;
		else
			v_scale = 2;
		layer->layer_info.config.ysize *= v_scale;
		if (layer->layer_info.config.ysize < expected_ysize) {
			if (!strcmp(davinci_dm.mode_info.name, "PAL")) {
				temp =
				    (layer->layer_info.config.ysize *
				     DAVINCI_DISPLAY_V_EXP_RATIO_N)
				    / DAVINCI_DISPLAY_V_EXP_RATIO_D;
				if (temp <= expected_ysize) {
					v_exp = 1;
					layer->layer_info.config.ysize = temp;
				}
			}
		}
		if (v_scale == 2)
			layer_info->v_zoom = ZOOM_X2;
		else if (v_scale == 4)
			layer_info->v_zoom = ZOOM_X4;
		if (v_exp)
			layer_info->h_exp = V_EXP_6_OVER_5;
	} else {
		/* no scaling, only cropping. Set display area to crop area */
		layer->layer_info.config.ysize = expected_ysize;
	}
	dev_dbg(davinci_display_dev,
		"crop display xsize = %d, ysize = %d\n",
		layer->layer_info.config.xsize, layer->layer_info.config.ysize);
}

static void davinci_disp_adj_position(struct display_obj *layer, int top,
				      int left)
{
	layer->layer_info.config.xpos = 0;
	layer->layer_info.config.ypos = 0;
	if (left + layer->layer_info.config.xsize <= davinci_dm.mode_info.xres)
		layer->layer_info.config.xpos = left;
	if (top + layer->layer_info.config.ysize <= davinci_dm.mode_info.yres)
		layer->layer_info.config.ypos = top;
	dev_dbg(davinci_display_dev,
		"new xpos = %d, ypos = %d\n",
		layer->layer_info.config.xpos, layer->layer_info.config.ypos);
}

static int davinci_disp_check_window_params(struct v4l2_rect *c)
{
	if ((c->width == 0)
	    || ((c->width + c->left) > davinci_dm.mode_info.xres)
	    || (c->height == 0)
	    || ((c->height + c->top) > davinci_dm.mode_info.yres)) {
		dev_err(davinci_display_dev, "Invalid crop values\n");
		return -1;
	}
	if ((c->height & 0x1) && (davinci_dm.mode_info.interlaced)) {
		dev_err(davinci_display_dev,
			"window height must be even for interlaced display\n");
		return -1;
	}
	return 0;
}

/* vpbe_get_mode_timeperframe() : get framerate of current video mode
 */
static int vpbe_get_mode_timeperframe(unsigned char *mode_info_name,
		struct v4l2_fract *frVal)
{
	int ret = 0;

	if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_NTSC)) {
		frVal->numerator   = 30000;
		frVal->denominator = 1001;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_NTSC_RGB)) {
		frVal->numerator   = 30000;
		frVal->denominator = 1001;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_PAL)) {
		frVal->numerator   = 25;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_PAL_RGB)) {
		frVal->numerator   = 25;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_720P_24)) {
		frVal->numerator   = 24;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_720P_25)) {
		frVal->numerator   = 25;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_720P_30)) {
		frVal->numerator   = 30;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_720P_50)) {
		frVal->numerator   = 50;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_720P_60)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080I_25)) {
		frVal->numerator   = 25;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080I_30)) {
		frVal->numerator   = 30;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080P_24)) {
		frVal->numerator   = 24;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080P_25)) {
		frVal->numerator   = 25;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080P_30)) {
		frVal->numerator   = 30;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080P_50)) {
		frVal->numerator   = 50;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080P_60)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_480P_60)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_576P_50)) {
		frVal->numerator   = 50;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_640x480)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_640x400)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_640x350)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_800x480)) {
		frVal->numerator   = 60;
		frVal->denominator = 1;
	} else {
		dev_err(davinci_display_dev, "unknown video mode\n");
		ret = -EINVAL;
	}
	return ret;
}

/* vpbe_try_format()
 * If user application provides width and height, and have bytesperline set
 * to zero, driver calculates bytesperline and sizeimage based on hardware
 * limits. If application likes to add pads at the end of each line and
 * end of the buffer , it can set bytesperline to line size and sizeimage to
 * bytesperline * height of the buffer. If driver fills zero for active
 * video width and height, and has requested user bytesperline and sizeimage,
 * width and height is adjusted to maximum display limit or buffer width
 * height which ever is lower
 */
static int vpbe_try_format(struct v4l2_pix_format *pixfmt, int check)
{
	struct vid_enc_mode_info *mode_info;
	int min_sizeimage, bpp, min_height = 1, min_width = 32,
		max_width, max_height, user_info = 0;

	mode_info = &davinci_dm.mode_info;
	davinci_enc_get_mode(0, mode_info);

	if ((pixfmt->pixelformat != V4L2_PIX_FMT_UYVY) &&
	    (pixfmt->pixelformat != V4L2_PIX_FMT_NV12))
		/* choose default as V4L2_PIX_FMT_UYVY */
		pixfmt->pixelformat = V4L2_PIX_FMT_UYVY;

	if (pixfmt->field == V4L2_FIELD_ANY) {
		if (mode_info->interlaced)
			pixfmt->field = V4L2_FIELD_INTERLACED;
		else
			pixfmt->field = V4L2_FIELD_NONE;
	}

	if (pixfmt->field == V4L2_FIELD_INTERLACED)
		min_height = 2;

	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12)
		bpp = 1;
	else
		bpp = 2;

	max_width = mode_info->xres;
	max_height = mode_info->yres;

	min_width /= bpp;

	if (!pixfmt->width && !pixfmt->bytesperline) {
		dev_err(davinci_display_dev, "bytesperline and width"
			" cannot be zero\n");
		return -EINVAL;
	}

	/* if user provided bytesperline, it must provide sizeimage as well */
	if (pixfmt->bytesperline && !pixfmt->sizeimage) {
		dev_err(davinci_display_dev,
			"sizeimage must be non zero, when user"
			" provides bytesperline\n");
		return -EINVAL;
	}

	/* adjust bytesperline as per hardware - multiple of 32 */
	if (!pixfmt->width)
		pixfmt->width = pixfmt->bytesperline / bpp;

	if (!pixfmt->bytesperline)
		pixfmt->bytesperline = pixfmt->width * bpp;
	else
		user_info = 1;
	pixfmt->bytesperline = ((pixfmt->bytesperline + 31) & ~31);

	if (pixfmt->width < min_width) {
		if (check) {
			dev_err(davinci_display_dev,
				"height is less than minimum,"
				"input width = %d, min_width = %d \n",
				pixfmt->width, min_width);
			return -EINVAL;
		}
		pixfmt->width = min_width;
	}

	if (pixfmt->width > max_width) {
		if (check) {
			dev_err(davinci_display_dev,
				"width is more than maximum,"
				"input width = %d, max_width = %d\n",
				pixfmt->width, max_width);
			return -EINVAL;
		}
		pixfmt->width = max_width;
	}

	/* If height is zero, then atleast we need to have sizeimage
	 * to calculate height
	 */
	if (!pixfmt->height) {
		if (user_info) {
			if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12) {
				/* for NV12 format, sizeimage is y-plane size
				 * + CbCr plane which is half of y-plane
				 */
				pixfmt->height = pixfmt->sizeimage /
						(pixfmt->bytesperline +
						(pixfmt->bytesperline >> 1));
			} else
				pixfmt->height = pixfmt->sizeimage/
						pixfmt->bytesperline;
		}
	}

	if (pixfmt->height > max_height) {
		if (check && !user_info) {
			dev_err(davinci_display_dev,
				"height is more than maximum,"
				"input height = %d, max_height = %d\n",
				pixfmt->height, max_height);
			return -EINVAL;
		}
		pixfmt->height = max_height;
	}

	if (pixfmt->height < min_height) {
		if (check && !user_info) {
			dev_err(davinci_display_dev,
				"width is less than minimum,"
				"input height = %d, min_height = %d\n",
				pixfmt->height, min_height);
			return -EINVAL;
		}
		pixfmt->height = min_width;
	}

	/* if user has not provided bytesperline calculate it based on width */
	if (!user_info)
		pixfmt->bytesperline = (((pixfmt->width * bpp) + 31) & ~31);

	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12)
		min_sizeimage = pixfmt->bytesperline * pixfmt->height +
				(pixfmt->bytesperline * pixfmt->height >> 1);
	else
		min_sizeimage = pixfmt->bytesperline * pixfmt->height;

	if (pixfmt->sizeimage < min_sizeimage) {
		if (check && user_info) {
			dev_err(davinci_display_dev, "sizeimage is less, %d\n",
				min_sizeimage);
			return -EINVAL;
		}
		pixfmt->sizeimage = min_sizeimage;
	}
	return 0;
}

static int vpbe_g_priority(struct file *file, void *priv,
				enum v4l2_priority *p)
{
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	*p = v4l2_prio_max(&layer->prio);

	return 0;
}

static int vpbe_s_priority(struct file *file, void *priv,
				enum v4l2_priority p)
{
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;
	int ret;

	ret = v4l2_prio_change(&layer->prio, &fh->prio, p);

	return ret;
}

static int vpbe_g_parm(struct file *file, void *priv,
			struct v4l2_streamparm *parm)
{
	struct davinci_fh  *fh    = file->private_data;
	struct display_obj *layer = fh->layer;
	int                 ret   = 0;

	/* Clear out the input structure */
	memset(parm, 0, sizeof(struct v4l2_streamparm));

	/* Set the buffer type */
	parm->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	/* We support frame repeating controlled by the timeperframe setting.
	 * Frame skipping is still TBD */
	parm->parm.output.capability |= V4L2_CAP_TIMEPERFRAME;

	/* If a simulated frame rate is in effect, return it */
	if (layer->tpf_info.irq_service_rate > 0) {
		parm->parm.output.timeperframe =
			layer->tpf_info.sim_timeperframe;
	}
	/* Othersise, retrieve the current mode's framerate */
	else {
		ret = vpbe_get_mode_timeperframe(davinci_dm.mode_info.name,
			&parm->parm.output.timeperframe);
		if (ret < 0)
			goto hardware_framerate;
	}

	goto exit;

hardware_framerate:
	dev_err(davinci_display_dev, "Could not retrieve hardware framerate");
	ret = -EINVAL;

exit:
	return ret;
}

static int vpbe_s_parm(struct file *file, void *priv,
			struct v4l2_streamparm *parm)
{
	struct davinci_fh      *fh = file->private_data;
	struct display_obj     *layer = fh->layer;
	struct v4l2_streamparm  result;
	struct v4l2_fract      *hw_tpf;
	struct v4l2_fract      *sim_tpf;
	struct v4l2_fract       service_rate;
	unsigned long           normalize_gcd;
	int                     ret = 0;

	/* Establish some aliases for code readability */
	hw_tpf  = &layer->tpf_info.hw_timeperframe;
	sim_tpf = &parm->parm.output.timeperframe;

	/* Get our current settings */
	ret = vpbe_g_parm(file, priv, &result);
	if (ret < 0)
		goto g_parm_failed;

	/* Division by zero is not allowed */
	if (sim_tpf->denominator == 0)
		goto division_by_zero;

	/* Get the hardware framerate */
	ret = vpbe_get_mode_timeperframe(davinci_dm.mode_info.name, hw_tpf);
	if (ret < 0)
		goto hardware_framerate;

	/* Normalize the specified timeperframe */
	normalize_gcd = gcd(sim_tpf->numerator, sim_tpf->denominator);
	sim_tpf->numerator   /= normalize_gcd;
	sim_tpf->denominator /= normalize_gcd;

	/* If zero was specified for timeperframe or the hardware rate was
	 * specified, use the hardware rate */
	if (sim_tpf->numerator == 0 ||
	    (sim_tpf->numerator   == hw_tpf->numerator &&
	     sim_tpf->denominator == hw_tpf->denominator)) {

		/* Disable the simulated framerate */
		layer->tpf_info.irq_service_rate = 0;
		result.parm.output.timeperframe = *hw_tpf;
		goto exit;
	}

	/* Since we're doing fixed-point math to three decimal places, we
	 * cannot support a numerator value larger than 2^32 / 1000 */
	if (sim_tpf->numerator > (-1UL / 1000UL))
		goto too_big;

	/* Right now, a simulated framerate faster than the hardware is not
	 * supported. */
	if ((sim_tpf->numerator * 1000 / sim_tpf->denominator) >
	    (hw_tpf->numerator * 1000 / hw_tpf->denominator)) {
		goto too_fast;
	}

	/* If we get this far, we know that the specified framerate is valid.
	 * Update the simulated framerate. */
	layer->tpf_info.sim_timeperframe = *sim_tpf;
	sim_tpf = &layer->tpf_info.sim_timeperframe;

	/* Calculate the irq service rate.  */
	service_rate.numerator   = hw_tpf->numerator   * sim_tpf->denominator;
	service_rate.denominator = hw_tpf->denominator * sim_tpf->numerator;

	normalize_gcd = gcd(service_rate.numerator, service_rate.denominator);
	service_rate.numerator   /= normalize_gcd;
	service_rate.denominator /= normalize_gcd;

	/* Store the service rate in integer form. Multiply the result by 1000
	 * to also include the first three decimal digits. */
	layer->tpf_info.irq_service_rate =
		(service_rate.numerator * 1000) / service_rate.denominator;
	result.parm.output.timeperframe = *sim_tpf;

	layer->tpf_info.next_irq  = layer->tpf_info.irq_service_rate;
	layer->tpf_info.irq_count = 0;

	goto exit;

too_fast:
	dev_err(davinci_display_dev, "timeperframe cannot exceed hardware "
		"framerate");
	ret = -EINVAL;
	goto exit;

too_big:
	dev_err(davinci_display_dev, "timeperframe numerator cannot exceed "
		"%lu", -1UL / 1000UL);
	ret = -EINVAL;
	goto exit;

hardware_framerate:
	dev_err(davinci_display_dev, "Could not retrieve hardware framerate");
	ret = -EINVAL;
	goto exit;

division_by_zero:
	dev_err(davinci_display_dev, "timeperframe divides by zero");
	ret = -EINVAL;
	goto exit;

g_parm_failed:
	dev_err(davinci_display_dev, "Could not retrieve current settings");
	ret = -EINVAL;

exit:
	*parm = result;
	return ret;
}

static int vpbe_querycap(struct file *file, void  *priv,
			       struct v4l2_capability *cap)
{
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_QUERYCAP, layer id = %d\n",
			layer->device_id);
	
	memset(cap, 0, sizeof(*cap));
	*cap = davinci_display_videocap;

	return 0;
}

static int vpbe_s_crop(struct file *file, void *priv,
			     struct v4l2_crop *crop)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev,
				"VIDIOC_S_CROP, layer id = %d\n",
				layer->device_id);

	if (crop->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		struct v4l2_rect *rect = &crop->c;

		if (rect->top < 0 || rect->left < 0) {
			dev_err(davinci_display_dev,
					"Error in S_CROP params"
					" Negative values for"
					" top/left" );
			return -EINVAL;

		}

		if (davinci_disp_check_window_params(rect)) {
			dev_err(davinci_display_dev,
					"Error in S_CROP params\n");
					return -EINVAL;
		}
		ret = mutex_lock_interruptible(&davinci_dm.lock);
		if (ret)
			return ret;
				
		davinci_disp_get_layer_config(layer->layer_info.id,
						&layer->layer_info.config);

		davinci_disp_calculate_scale_factor(layer, rect->width,
							rect->height);

		davinci_disp_adj_position(layer, rect->top, rect->left);

		if (davinci_disp_set_layer_config(layer->layer_info.id,
			&layer->layer_info.config)) {
		
			dev_err(davinci_display_dev,
					"Error in S_CROP params\n");
			mutex_unlock(&davinci_dm.lock);
			return -EINVAL;
		}

		/* apply zooming and h or v expansion */
		davinci_disp_set_zoom(layer->layer_info.id,
				     layer->layer_info.h_zoom,
				     layer->layer_info.v_zoom);

		davinci_disp_set_vid_expansion(layer->layer_info.h_exp,
						layer->layer_info.v_exp);

		if ((layer->layer_info.h_zoom != ZOOM_X1) ||
			(layer->layer_info.v_zoom != ZOOM_X1) ||
			(layer->layer_info.h_exp != H_EXP_OFF) ||
			(layer->layer_info.v_exp != V_EXP_OFF))
				/* Enable expansion filter */
				davinci_disp_set_interpolation_filter(1);
		else
				davinci_disp_set_interpolation_filter(0);
				mutex_unlock(&davinci_dm.lock);
	} else {
		dev_err(davinci_display_dev, "Invalid buf type \n");
		return -EINVAL;
	}
	
	return ret;
}

static int vpbe_g_crop(struct file *file, void *priv,
			     struct v4l2_crop *crop)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;
	
	dev_dbg(davinci_display_dev, "VIDIOC_G_CROP, layer id = %d\n",
			layer->device_id);

	if (crop->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		struct v4l2_rect *rect = &crop->c;
		ret = mutex_lock_interruptible(&davinci_dm.lock);
		if (ret)
			return ret;
				
		davinci_disp_get_layer_config(layer->layer_info.id,
						&layer->layer_info.config);
		rect->top = layer->layer_info.config.ypos;
		rect->left = layer->layer_info.config.xpos;
		rect->width = layer->layer_info.config.xsize;
		rect->height = layer->layer_info.config.ysize;
		mutex_unlock(&davinci_dm.lock);
	} else {
		dev_err(davinci_display_dev,"Invalid buf type \n");
		ret = -EINVAL;
	}
	
	return ret;
}

static int vpbe_cropcap(struct file *file, void *priv,
			      struct v4l2_cropcap *cropcap)
{
	int ret = 0;

	dev_dbg(davinci_display_dev, "\nStart of VIDIOC_CROPCAP ioctl");
			
	if (davinci_enc_get_mode(0, &davinci_dm.mode_info)) {
		dev_err(davinci_display_dev,
			"Error in getting current display mode"
			" from enc mngr\n");
		return -EINVAL;
	}
			
	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;
			
	cropcap->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_NTSC)) {
		cropcap->bounds = cropcap->defrect = ntsc_bounds;
		cropcap->pixelaspect = ntsc_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_PAL)) {
		cropcap->bounds = cropcap->defrect = pal_bounds;
		cropcap->pixelaspect = pal_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name,VID_ENC_STD_640x480)) {
		cropcap->bounds = cropcap->defrect = vga_bounds;
		cropcap->pixelaspect = sp_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_640x400)) {
		cropcap->bounds = cropcap->defrect = vga_bounds;
		cropcap->bounds.height = cropcap->defrect.height = 400;
		cropcap->pixelaspect = sp_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_640x350)) {
		cropcap->bounds = cropcap->defrect = vga_bounds;
		cropcap->bounds.height = cropcap->defrect.height = 350;
		cropcap->pixelaspect = sp_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_480x272)) {
		cropcap->bounds = cropcap->defrect = vga_bounds;
		cropcap->pixelaspect = sp_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_720P_60)) {
		cropcap->bounds = cropcap->defrect = hd_720p_bounds;
		cropcap->pixelaspect = sp_aspect;
	} else if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_1080I_30)) {
		cropcap->bounds = cropcap->defrect = hd_1080i_bounds;
		cropcap->pixelaspect = sp_aspect;
	} else {
		dev_err(davinci_display_dev, "Unknown encoder display mode\n");
		return -EINVAL;
	}
	
	mutex_unlock(&davinci_dm.lock);
	dev_dbg(davinci_display_dev, "\nEnd of VIDIOC_CROPCAP ioctl");

	return ret;
}

static int vpbe_streamoff(struct file *file, void *priv,
			  enum v4l2_buf_type buf_type)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_STREAMOFF,layer id = %d\n",
		layer->device_id);
	/* If io is allowed for this file handle, return error */
	if (!fh->io_allowed) {
		dev_err(davinci_display_dev, "No io_allowed\n");
		return -EACCES;
	}
			
	/* If streaming is not started, return error */
	if (!layer->started) {
		dev_err(davinci_display_dev, "streaming not started in layer"
			" id = %d\n", layer->device_id);
		return -EINVAL;
	}
			
	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;
	davinci_disp_disable_layer(layer->layer_info.id);
	layer->started = 0;
	mutex_unlock(&davinci_dm.lock);
	ret = videobuf_streamoff(&layer->buffer_queue);

	return ret;
}

static int vpbe_streamon(struct file *file, void *priv,
			 enum v4l2_buf_type buf_type)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_STREAMON, layer id = %d\n",
			layer->device_id);
	/* If file handle is not allowed IO, return error */
	if (!fh->io_allowed) {
		dev_err(davinci_display_dev, "No io_allowed\n");
		return -EACCES;
	}
	/* If Streaming is already started, return error */
	if (layer->started) {
		dev_err(davinci_display_dev, "layer is already streaming\n");
		return -EBUSY;
	}

	/*
	 * Call videobuf_streamon to start streaming
	 * in videobuf
	 */
	ret = videobuf_streamon(&layer->buffer_queue);
	if (ret) {
		dev_err(davinci_display_dev,
		"error in videobuf_streamon\n");
		return ret;
	}
	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;
	/* If buffer queue is empty, return error */
	if (list_empty(&layer->dma_queue)) {
		dev_err(davinci_display_dev, "buffer queue is empty\n");
		mutex_unlock(&davinci_dm.lock);
		return -EIO;
	}
	/* Get the next frame from the buffer queue */
	layer->nextFrm = layer->curFrm = list_entry(layer->dma_queue.next,
				struct videobuf_buffer, queue);
	/* Remove buffer from the buffer queue */
	list_del(&layer->curFrm->queue);
	/* Mark state of the current frame to active */
	layer->curFrm->state = VIDEOBUF_ACTIVE;
	/* Initialize field_id and started member */

	layer->field_id = 0;

	/* Set parameters in OSD and VENC */
	ret = davinci_set_video_display_params(layer);
	if (ret < 0) {
		mutex_unlock(&davinci_dm.lock);
		return ret;
	}
	/* if request format is yuv420 semiplanar, need to
	 * enable both video windows
	 */

	layer->started = 1;
	dev_dbg(davinci_display_dev, "Started streaming on layer id = %d,"
		" ret = %d\n", layer->device_id, ret);
			
	layer_first_int = 1;
	mutex_unlock(&davinci_dm.lock);

	return ret;
}

static int vpbe_dqbuf(struct file *file, void *priv,
		      struct v4l2_buffer *buf)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_DQBUF, layer id = %d\n",
			layer->device_id);

	/* If this file handle is not allowed to do IO, return error */
	if (!fh->io_allowed) {
		dev_err(davinci_display_dev, "No io_allowed\n");
		return -EACCES;
	}
	if (file->f_flags & O_NONBLOCK)
		/* Call videobuf_dqbuf for non blocking mode */
		ret = videobuf_dqbuf(&layer->buffer_queue, buf, 1);
	else
		/* Call videobuf_dqbuf for blocking mode */
		ret = videobuf_dqbuf(&layer->buffer_queue, buf, 0);

	return ret;
}

static int vpbe_qbuf(struct file *file, void *priv,
		     struct v4l2_buffer *p)
{
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_QBUF, layer id = %d\n",
		layer->device_id);

	/* If this file handle is not allowed to do IO, return error */
	if (!fh->io_allowed) {
		dev_err(davinci_display_dev, "No io_allowed\n");
		return -EACCES;
	}

	return videobuf_qbuf(&layer->buffer_queue, p);
}

static int vpbe_querybuf(struct file *file, void *priv,
			 struct v4l2_buffer *buf)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_QUERYBUF, layer id = %d\n",
			layer->device_id);

	/* Call videobuf_querybuf to get information */
	ret = videobuf_querybuf(&layer->buffer_queue, buf);

	return ret;
}

static int vpbe_reqbufs(struct file *file, void *priv,
			struct v4l2_requestbuffers *req_buf)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_REQBUFS, count= %d, type = %d,"
			"memory = %d\n",
			req_buf->count, req_buf->type, req_buf->memory);

	/* If io users of the layer is not zero, return error */
	if (0 != layer->io_usrs) {
		dev_err(davinci_display_dev, "not IO user\n");
		return -EBUSY;
	}
	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;
	/* Initialize videobuf queue as per the buffer type */
			
	videobuf_queue_dma_contig_init(&layer->buffer_queue,
					    &video_qops, davinci_display_dev,
					    &layer->irqlock,
					    V4L2_BUF_TYPE_VIDEO_OUTPUT,
					    layer->pix_fmt.field,
					    sizeof(struct videobuf_buffer), fh);

	/* Set io allowed member of file handle to TRUE */
	fh->io_allowed = 1;
	/* Increment io usrs member of layer object to 1 */
	layer->io_usrs = 1;
	/* Store type of memory requested in layer object */
	layer->memory = req_buf->memory;
	/* Initialize buffer queue */
	INIT_LIST_HEAD(&layer->dma_queue);
	/* Allocate buffers */
	ret = videobuf_reqbufs(&layer->buffer_queue, req_buf);
	mutex_unlock(&davinci_dm.lock);

	return ret;
}

static int vpbe_s_fmt(struct file *file, void *priv,
				struct v4l2_format *fmt)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_S_FMT, layer id = %d\n",
			layer->device_id);

	/* If streaming is started, return error */
	if (layer->started) {
		dev_err(davinci_display_dev, "Streaming is started\n");
		return -EBUSY;
	}
	if (V4L2_BUF_TYPE_VIDEO_OUTPUT == fmt->type) {
		struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
		/* Check for valid field format */
		ret = vpbe_try_format(pixfmt, 1);
		if (ret)
			return ret;

	/* YUV420 is requested, check availability of the other video window */
	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;

	layer->pix_fmt = *pixfmt;
	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12 &&
		cpu_is_davinci_dm365()) {
		struct display_obj *otherlayer =
			_davinci_disp_get_other_win(layer);

		/* if other layer is available, only
		 * claim it, do not configure it
		 */
		if (davinci_disp_request_layer(otherlayer->layer_info.id)) {
			/* Couldn't get layer */
			dev_err(davinci_display_dev,
						"Display Manager"
						" failed to allocate"
						" the other layer:"
						"vidwin %d\n",
						otherlayer->
						layer_info.id);
			mutex_unlock(&davinci_dm.lock);
			return -EBUSY;
		}
	}
				
	/* store the pixel format in the layer object */
	davinci_disp_get_layer_config(layer->layer_info.id,
		&layer->layer_info.config);

	layer->layer_info.config.xsize = pixfmt->width;
	layer->layer_info.config.ysize = pixfmt->height;
	layer->layer_info.config.line_length = pixfmt->bytesperline;
	layer->layer_info.config.ypos = 0;
	layer->layer_info.config.xpos = 0;
	layer->layer_info.config.interlaced =
			davinci_dm.mode_info.interlaced;

	/* change of the default pixel format for both vid windows */
	if (V4L2_PIX_FMT_NV12 == pixfmt->pixelformat) {
		struct display_obj *otherlayer;
		layer->layer_info.config.pixfmt = PIXFMT_NV12;
		otherlayer = _davinci_disp_get_other_win(layer);
		otherlayer->layer_info.config.pixfmt = PIXFMT_NV12;
	}

	if (davinci_disp_set_layer_config(layer->layer_info.id,
					&layer->layer_info.config)) {
		dev_err(davinci_display_dev, "Error in S_FMT params:\n");
		mutex_unlock(&davinci_dm.lock);
		return -EINVAL;
	}

	/* readback and fill the local copy of current pix format */
	davinci_disp_get_layer_config(layer->layer_info.id,
					&layer->layer_info.config);
		
	/* verify if readback values are as expected */
	if (layer->pix_fmt.width != layer->layer_info.config.xsize ||
		layer->pix_fmt.height != layer->layer_info.config.ysize ||
		layer->pix_fmt.bytesperline != layer->layer_info.config.line_length ||
		(layer->layer_info.config.interlaced && layer->pix_fmt.field !=
		V4L2_FIELD_INTERLACED) || (!layer->layer_info.config.interlaced &&
		layer->pix_fmt.field != V4L2_FIELD_NONE)) {
		dev_err(davinci_display_dev, "mismatch with layer config"
				" params:\n");
		dev_err(davinci_display_dev, "layer->layer_info.config.xsize ="
				"%d layer->pix_fmt.width = %d\n",
				layer->layer_info.config.xsize,
				layer->pix_fmt.width);
		dev_err(davinci_display_dev,
				"layer->layer_info.config.ysize ="
				"%d layer->pix_fmt.height = %d\n",
				layer->layer_info.config.ysize,
				layer->pix_fmt.height);
		dev_err(davinci_display_dev, "layer->layer_info.config."
				"line_length= %d layer->pix_fmt"
				".bytesperline = %d\n",
				layer->layer_info.config.line_length,
				layer->pix_fmt.bytesperline);
		dev_err(davinci_display_dev, "layer->layer_info.config."
				"interlaced =%d layer->pix_fmt."
				"field = %d\n", layer->layer_info.config.interlaced,
				layer->pix_fmt.field);
		mutex_unlock(&davinci_dm.lock);
		return -EFAULT;
	}

		dev_notice(davinci_display_dev,
				"Before finishing with S_FMT:\n"
				"layer.pix_fmt.bytesperline = %d,\n"
				" layer.pix_fmt.width = %d, \n"
				" layer.pix_fmt.height = %d, \n"
				" layer.pix_fmt.sizeimage =%d\n",
				layer->pix_fmt.bytesperline,
				layer->pix_fmt.width,
				layer->pix_fmt.height,
				layer->pix_fmt.sizeimage);

		dev_notice(davinci_display_dev,
				"pixfmt->width = %d,\n"
				" layer->layer_info.config.line_length"
				"= %d\n",
				pixfmt->width,
				layer->layer_info.config.line_length);

		mutex_unlock(&davinci_dm.lock);
	} else {
		dev_err(davinci_display_dev, "invalid type\n");
		return -EINVAL;
	}

	return ret;
}

static int vpbe_try_fmt(struct file *file, void *priv,
				  struct v4l2_format *fmt)
{
	int ret = 0;

	dev_dbg(davinci_display_dev, "VIDIOC_TRY_FMT\n");

	if (V4L2_BUF_TYPE_VIDEO_OUTPUT == fmt->type) {
		struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
		/* Check for valid field format */
		ret = vpbe_try_format(pixfmt, 0);
	} else {
		dev_err(davinci_display_dev, "invalid type\n");
		ret = -EINVAL;
	}

	return ret;
}

static int vpbe_enum_fmt(struct file *file, void  *priv,
				   struct v4l2_fmtdesc *fmt)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;
	unsigned int index = 0;

	dev_dbg(davinci_display_dev, "VIDIOC_ENUM_FMT, layer id = %d\n",
		layer->device_id);
	if (fmt->index > 0) {
		dev_err(davinci_display_dev,
			"Invalid format index\n");
		return -EINVAL;
	}
	/* Fill in the information about format */

	index = fmt->index;
	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (index == 0) {
		strcpy(fmt->description, "YUV 4:2:2 - UYVY");
		fmt->pixelformat = V4L2_PIX_FMT_UYVY;
	} else if (index == 1) {
		strcpy(fmt->description, "Y/CbCr 4:2:0");
		fmt->pixelformat = V4L2_PIX_FMT_NV12;
	}

	return ret;
}

static int vpbe_g_fmt(struct file *file, void *priv,
				struct v4l2_format *fmt)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "VIDIOC_G_FMT, layer id = %d\n",
			layer->device_id);

	/* If buffer type is video output */
	if (V4L2_BUF_TYPE_VIDEO_OUTPUT == fmt->type) {
		struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
		/* Fill in the information about format */
		ret = mutex_lock_interruptible(&davinci_dm.lock);
		if (!ret) {
			*pixfmt = layer->pix_fmt;
			mutex_unlock(&davinci_dm.lock);
		}
	} else {
		dev_err(davinci_display_dev, "invalid type\n");
		ret = -EINVAL;
	}

	return ret;
}

static long vpbe_param_handler(struct file *file, void *priv,
		int cmd, void *param)
{
	int ret = 0;

	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;

	switch (cmd) {
	case VIDIOC_S_COFST:
		davinci_dm.fb_desc.cbcr_ofst = *((unsigned long *) param);
		mutex_unlock(&davinci_dm.lock);
		break;
	case VIDIOC_S_YDOFST:
		davinci_dm.fb_desc.yd_ofst = *((unsigned long *) param);

		/* The Y-plane display offset must be 64-byte aligned */
		if (davinci_dm.fb_desc.yd_ofst & 0x3F) {
			dev_err(davinci_display_dev, "fb_desc.yd_ofst is not "
				"aligned to 64 bytes\n");
			davinci_dm.fb_desc.yd_ofst = 0;
			ret = -EINVAL;
		}

		mutex_unlock(&davinci_dm.lock);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/*
 * davinci_mmap()
 * It is used to map kernel space buffers into user spaces
 */
static int davinci_mmap(struct file *filep, struct vm_area_struct *vma)
{
	/* Get the layer object and file handle object */
	struct davinci_fh *fh = filep->private_data;
	struct display_obj *layer = fh->layer;
	int err = 0;
	dev_dbg(davinci_display_dev, "<davinci_mmap>\n");

	err = videobuf_mmap_mapper(&layer->buffer_queue, vma);
	dev_dbg(davinci_display_dev, "</davinci_mmap>\n");
	return err;
}

/* davinci_poll(): It is used for select/poll system call
 */
static unsigned int davinci_poll(struct file *filep, poll_table *wait)
{
	int err = 0;
	struct davinci_fh *fh = filep->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "<davinci_poll>");

	if (layer->started)
		err = videobuf_poll_stream(filep, &layer->buffer_queue, wait);
	dev_dbg(davinci_display_dev, "</davinci_poll>");
	return err;
}

static int davinci_config_layer(enum davinci_display_device_id id)
{
	int err = 0;
	struct davinci_layer_config *layer_config;
	struct vid_enc_mode_info *mode_info;
	struct display_obj *layer = davinci_dm.dev[id];

	/* First claim the layer for this device */
	if (davinci_disp_request_layer(layer->layer_info.id)) {
		/* Couldn't get layer */
		dev_err(davinci_display_dev,
			"Display Manager failed to allocate layer\n");
		return -EBUSY;
	}

	/* get the current video display mode from encoder manager */
	mode_info = &davinci_dm.mode_info;
	if (davinci_enc_get_mode(0, mode_info)) {
		dev_err(davinci_display_dev,
			"Error in getting current display mode from enc mngr\n");
		return -1;
	}

	layer_config = &layer->layer_info.config;
	/* Set the default image and crop values */
	layer_config->pixfmt = PIXFMT_YCbCrI;
	layer->pix_fmt.pixelformat = V4L2_PIX_FMT_UYVY;
	layer->pix_fmt.bytesperline = layer_config->line_length =
	    mode_info->xres * 2;

	layer->pix_fmt.width = layer_config->xsize = mode_info->xres;
	layer->pix_fmt.height = layer_config->ysize = mode_info->yres;
	layer->pix_fmt.sizeimage =
	    layer->pix_fmt.bytesperline * layer->pix_fmt.height;
	layer_config->xpos = 0;
	layer_config->ypos = 0;
	layer_config->interlaced = mode_info->interlaced;

	/* turn off ping-pong buffer and field inversion to fix
	 * the image shaking problem in 1080I mode
	 */

	if (layer->layer_info.config.interlaced)
		layer->pix_fmt.field = V4L2_FIELD_INTERLACED;
	else
		layer->pix_fmt.field = V4L2_FIELD_NONE;
	davinci_disp_set_layer_config(layer->layer_info.id, layer_config);
	return err;
}

/*
 * davinci_open()
 * It creates object of file handle structure and stores it in private_data
 * member of filepointer
 */
static int davinci_open(struct file *filep)
{
	int minor = iminor(filep->f_path.dentry->d_inode);
	int found = -1;
	int i = 0;
	struct display_obj *layer;
	struct davinci_fh *fh = NULL;

	/* Check for valid minor number */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		layer = davinci_dm.dev[i];
		if (minor == layer->video_dev->minor) {
			found = i;
			break;
		}
	}

	/* If not found, return error no device */
	if (0 > found) {
		dev_err(davinci_display_dev, "device not found\n");
		return -ENODEV;
	}

	/* Allocate memory for the file handle object */
	fh = kmalloc(sizeof(struct davinci_fh), GFP_KERNEL);
	if (ISNULL(fh)) {
		dev_err(davinci_display_dev,
			"unable to allocate memory for file handle object\n");
		return -ENOMEM;
	}
	dev_dbg(davinci_display_dev, "<davinci open> plane = %d\n",
		layer->device_id);
	/* store pointer to fh in private_data member of filep */
	filep->private_data = fh;
	fh->layer = layer;

	if (!layer->usrs) {
		/* Configure the default values for the layer */
		if (davinci_config_layer(layer->device_id)) {
			dev_err(davinci_display_dev,
				"Unable to configure video layer for id = %d\n",
				layer->device_id);
			return -EINVAL;
		}
	}

	/* Increment layer usrs counter */
	layer->usrs++;
	/* Set io_allowed member to false */
	fh->io_allowed = 0;
	/* Initialize priority of this instance to default priority */
	fh->prio = V4L2_PRIORITY_UNSET;
	v4l2_prio_open(&layer->prio, &fh->prio);
	dev_dbg(davinci_display_dev, "</davinci_open>\n");

	/* Initialize framerate information */
	memset(&fh->layer->tpf_info, 0,
		sizeof(struct davinci_timeperframe_info));

	return 0;
}

/*
 * davinci_release()
 * This function deletes buffer queue, frees the buffers and the davinci
 * display file * handle
 */
static int davinci_release(struct file *filep)
{
	int ret = 0;
	/* Get the layer object and file handle object */
	struct davinci_fh *fh = filep->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "<davinci_release>\n");
	/* If this is doing IO and other layer are not closed */
	if ((layer->usrs != 1) && fh->io_allowed) {
		dev_err(davinci_display_dev, "Close other instances\n");
		return -EAGAIN;
	}
	/* Get the lock on layer object */
	ret = mutex_lock_interruptible(&davinci_dm.lock);
	if (ret)
		return ret;

	/* if this instance is doing IO */
	if (fh->io_allowed) {
		/* Reset io_usrs member of layer object */
		layer->io_usrs = 0;
		davinci_disp_disable_layer(layer->layer_info.id);
		layer->started = 0;
		/* Free buffers allocated */
		videobuf_queue_cancel(&layer->buffer_queue);
		videobuf_mmap_free(&layer->buffer_queue);
	}

	/* Decrement layer usrs counter */
	layer->usrs--;
	/* If this file handle has initialize encoder device, reset it */
	if (!layer->usrs) {
		if (layer->layer_info.config.pixfmt == PIXFMT_NV12) {
			struct display_obj *otherlayer;
			otherlayer = _davinci_disp_get_other_win(layer);
			davinci_disp_disable_layer(otherlayer->layer_info.id);
			davinci_disp_release_layer(otherlayer->layer_info.id);
		}
		davinci_disp_disable_layer(layer->layer_info.id);
		davinci_disp_release_layer(layer->layer_info.id);
	}

	/* Clear any simulated frame rate */
	memset(&layer->tpf_info, 0, sizeof(struct davinci_timeperframe_info));

	/* Close the priority */
	v4l2_prio_close(&layer->prio, &fh->prio);
	filep->private_data = NULL;

	/* Free memory allocated to file handle object */
	if (!ISNULL(fh))
		kfree(fh);
	/* unlock mutex on layer object */
	mutex_unlock(&davinci_dm.lock);

	davinci_dm.fb_desc.cbcr_ofst = 0;
	davinci_dm.fb_desc.yd_ofst   = 0;

	dev_dbg(davinci_display_dev, "</davinci_release>\n");
	return 0;
}

static void davinci_platform_release(struct device
				     *device)
{
	/* This is called when the reference count goes to zero */
}

static struct v4l2_file_operations vpbe_fops = {
	.owner = THIS_MODULE,
	.open = davinci_open,
	.release = davinci_release,
	.ioctl = video_ioctl2,
	.mmap = davinci_mmap,
	.poll = davinci_poll
};

/* vpbe capture ioctl operations */
static const struct v4l2_ioctl_ops vpbe_ioctl_ops = {
	.vidioc_querycap	 = vpbe_querycap,
	.vidioc_g_fmt_vid_out    = vpbe_g_fmt,
	.vidioc_enum_fmt_vid_out = vpbe_enum_fmt,
	.vidioc_s_fmt_vid_out    = vpbe_s_fmt,
	.vidioc_try_fmt_vid_out  = vpbe_try_fmt,
	.vidioc_reqbufs		 = vpbe_reqbufs,
	.vidioc_querybuf	 = vpbe_querybuf,
	.vidioc_qbuf		 = vpbe_qbuf,
	.vidioc_dqbuf		 = vpbe_dqbuf,
	.vidioc_streamon	 = vpbe_streamon,
	.vidioc_streamoff	 = vpbe_streamoff,
	.vidioc_cropcap		 = vpbe_cropcap,
	.vidioc_g_crop		 = vpbe_g_crop,
	.vidioc_s_crop		 = vpbe_s_crop,
	.vidioc_g_priority	 = vpbe_g_priority,
	.vidioc_s_priority	 = vpbe_s_priority,
	.vidioc_g_parm    	 = vpbe_g_parm,
	.vidioc_s_parm    	 = vpbe_s_parm,
	.vidioc_default		 = vpbe_param_handler,
};

/*
 * davinci_probe()
 * This function creates device entries by register itself to the V4L2 driver
 * and initializes fields of each layer objects
 */
static int davinci_probe(struct device *device)
{
	int i, j = 0, k, err = 0;
	struct video_device *vbd = NULL;
	struct display_obj *layer = NULL;
	struct platform_device *pdev;
	unsigned long phys_end_kernel;
	size_t size;

	davinci_display_dev = device;

	dev_dbg(davinci_display_dev, "<davinci_probe>\n");

	/* First request memory region for io */
	pdev = to_platform_device(device);
	if (pdev->num_resources != 0) {
		dev_err(davinci_display_dev, "probed for an unknown device\n");
		return -ENODEV;
	}

       /*
	* Initialising the memory from the input arguments file for
	* contiguous memory buffers and avoid defragmentation
	*/

	if (cont2_bufsize) {
		/* attempt to determine the end of Linux kernel memory */
		phys_end_kernel = virt_to_phys((void *)PAGE_OFFSET) +
			(num_physpages << PAGE_SHIFT);
		phys_end_kernel += cont2_bufoffset;
		size = cont2_bufsize;

		err = dma_declare_coherent_memory(&pdev->dev, phys_end_kernel,
			phys_end_kernel,
			size,
			DMA_MEMORY_MAP |
			DMA_MEMORY_EXCLUSIVE);

		if (!err) {
			dev_err(&pdev->dev, "Unable to declare MMAP memory.\n");
			err = -ENOMEM;
			goto probe_out;
		}
	}

	if (cont3_bufsize) {
		/* attempt to determine the end of Linux kernel memory */
		phys_end_kernel = virt_to_phys((void *)PAGE_OFFSET) +
			(num_physpages << PAGE_SHIFT);
			phys_end_kernel += cont3_bufoffset;
			size = cont3_bufsize;

		err = dma_declare_coherent_memory(&pdev->dev, phys_end_kernel,
			phys_end_kernel,
			size,
			DMA_MEMORY_MAP |
			DMA_MEMORY_EXCLUSIVE);

		if (!err) {
			dev_err(&pdev->dev, "Unable to declare MMAP memory.\n");
			err = -ENOMEM;
			goto probe_out;
		}
	}

	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		layer = davinci_dm.dev[i];
		/* Allocate memory for video device */
		vbd = video_device_alloc();
		if (ISNULL(vbd)) {
			for (j = 0; j < i; j++) {
				video_device_release
				    (davinci_dm.dev[j]->video_dev);
			}
			dev_err(davinci_display_dev, "ran out of memory\n");
			return -ENOMEM;
		}

		/* Initialize field of video device */
		vbd->dev = *device;
		vbd->release 		= video_device_release;
		vbd->fops		= &vpbe_fops;
		vbd->ioctl_ops		= &vpbe_ioctl_ops;
		vbd->minor		= -1;
		vbd->current_norm	= V4L2_STD_NTSC;

		snprintf(vbd->name, sizeof(vbd->name),
			 "DaVinci_VPBEDisplay_DRIVER_V%d.%d.%d",
			 (DAVINCI_DISPLAY_VERSION_CODE >> 16)
			 & 0xff,
			 (DAVINCI_DISPLAY_VERSION_CODE >> 8) &
			 0xff, (DAVINCI_DISPLAY_VERSION_CODE) & 0xff);
		/* Set video_dev to the video device */
		layer->video_dev = vbd;
		layer->device_id = i;
		layer->layer_info.id =
		    ((i == DAVINCI_DISPLAY_DEVICE_0) ? WIN_VID0 : WIN_VID1);
		if (display_buf_config_params.numbuffers[i] == 0)
			layer->memory = V4L2_MEMORY_USERPTR;
		else
			layer->memory = V4L2_MEMORY_MMAP;
		/* Initialize field of the layer objects */
		layer->usrs = layer->io_usrs = 0;
		layer->started = 0;

		/* Initialize prio member of layer object */
		v4l2_prio_init(&layer->prio);

		/* register video device */
		printk(KERN_NOTICE
		       "Trying to register davinci display video device.\n");
		printk(KERN_NOTICE "layer=%x,layer->video_dev=%x\n", (int)layer,
		       (int)&layer->video_dev);

		err = video_register_device(layer->
					    video_dev,
					    VFL_TYPE_GRABBER,
					    davinci_display_nr[i]);
		if (err)
			goto probe_out;
	}
	/* Initialize mutex */
	mutex_init(&davinci_dm.lock);
	return 0;

probe_out:
	for (k = 0; k < j; k++) {
		/* Get the pointer to the layer object */
		layer = davinci_dm.dev[k];
		/* Unregister video device */
		video_unregister_device(layer->video_dev);
		/* Release video device */
		video_device_release(layer->video_dev);
		layer->video_dev = NULL;
	}
	return err;
}

/*
 * davinci_remove()
 * It un-register hardware planes from V4L2 driver
 */
static int davinci_remove(struct device *device)
{
	int i;
	struct display_obj *plane;
	dev_dbg(davinci_display_dev, "<davinci_remove>\n");
	/* un-register device */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		plane = davinci_dm.dev[i];
		/* Unregister video device */
		video_unregister_device(plane->video_dev);

		plane->video_dev = NULL;
	}

	dev_dbg(davinci_display_dev, "</davinci_remove>\n");
	return 0;
}

static struct device_driver davinci_driver = {
	.name = DAVINCI_DISPLAY_DRIVER,
	.bus = &platform_bus_type,
	.probe = davinci_probe,
	.remove = davinci_remove,
};
static struct platform_device _davinci_display_device = {
	.name = DAVINCI_DISPLAY_DRIVER,
	.id = 1,
	.dev = {
		.release = davinci_platform_release,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		}
};

/*
 * davinci_display_init()
 * This function registers device and driver to the kernel, requests irq
 * handler and allocates memory for layer objects
 */
static __init int davinci_display_init(void)
{
	int err = 0, i, j;
	int free_layer_objects_index;
	int free_buffer_layer_index;
	int free_buffer_index;


	printk(KERN_DEBUG "<davinci_display_init>\n");

	/* Default number of buffers should be 3 */
	if ((video2_numbuffers > 0) &&
	    (video2_numbuffers < display_buf_config_params.min_numbuffers))
		video2_numbuffers = display_buf_config_params.min_numbuffers;
	if ((video3_numbuffers > 0) &&
	    (video3_numbuffers < display_buf_config_params.min_numbuffers))
		video3_numbuffers = display_buf_config_params.min_numbuffers;

	/* Set buffer size to min buffers size if invalid buffer size is
	 * given */
	if (video2_bufsize <
	    display_buf_config_params.min_bufsize[DAVINCI_DISPLAY_DEVICE_0])
		video2_bufsize =
		    display_buf_config_params.
		    min_bufsize[DAVINCI_DISPLAY_DEVICE_0];

	if (video3_bufsize <
	    display_buf_config_params.min_bufsize[DAVINCI_DISPLAY_DEVICE_1])
		video3_bufsize =
		    display_buf_config_params.
		    min_bufsize[DAVINCI_DISPLAY_DEVICE_1];

	/* set number of buffers, they could come from boot/args */
	display_buf_config_params.numbuffers[DAVINCI_DISPLAY_DEVICE_0] =
		video2_numbuffers;
	display_buf_config_params.numbuffers[DAVINCI_DISPLAY_DEVICE_1] =
		video3_numbuffers;

	/*set size of buffers, they could come from bootargs*/
	display_buf_config_params.layer_bufsize[DAVINCI_DISPLAY_DEVICE_0] =
		video2_bufsize;
	display_buf_config_params.layer_bufsize[DAVINCI_DISPLAY_DEVICE_1] =
		video3_bufsize;

	if (cpu_is_davinci_dm355()) {
		strcpy(davinci_display_videocap.card, DM355_EVM_CARD);
	} else if (cpu_is_davinci_dm365())
		strcpy(davinci_display_videocap.card, DM365_EVM_CARD);
	else
		strcpy(davinci_display_videocap.card, DM644X_EVM_CARD);


	/* Allocate memory for four plane display objects */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		davinci_dm.dev[i] =
		    kmalloc(sizeof(struct display_obj), GFP_KERNEL);
		/* If memory allocation fails, return error */
		if (!davinci_dm.dev[i]) {
			free_layer_objects_index = i;
			printk(KERN_ERR "ran out of memory\n");
			err = -ENOMEM;
			goto davinci_init_free_layer_objects;
		}
		spin_lock_init(&davinci_dm.dev[i]->irqlock);
	}
	free_layer_objects_index = DAVINCI_DISPLAY_MAX_DEVICES;

	if (display_buf_config_params.numbuffers[0] == 0)
		printk(KERN_ERR "no vid2 buffer allocated\n");
	if (display_buf_config_params.numbuffers[1] == 0)
		printk(KERN_ERR "no vid3 buffer allocated\n");
	free_buffer_layer_index = DAVINCI_DISPLAY_MAX_DEVICES;
	free_buffer_index = display_buf_config_params.numbuffers[i - 1];
	/* Register driver to the kernel */
	err = driver_register(&davinci_driver);
	if (0 != err)
		goto davinci_init_free_layer_objects;

	/* register device as a platform device to the kernel */
	err = platform_device_register(&_davinci_display_device);
	if (0 != err) {
		goto davinci_init_unregister_driver;
	}

	davinci_dm.event_callback.mask = (DAVINCI_DISP_END_OF_FRAME |
					  DAVINCI_DISP_FIRST_FIELD |
					  DAVINCI_DISP_SECOND_FIELD);

	davinci_dm.event_callback.arg = &davinci_dm;
	davinci_dm.event_callback.handler = davinci_display_isr;

	err = davinci_disp_register_callback(&davinci_dm.event_callback);

	if (0 != err) {
		goto davinci_init_unregister_driver;
	}
	printk(KERN_NOTICE
	       "davinci_init:DaVinci V4L2 Display Driver V1.0 loaded\n");
	printk(KERN_DEBUG "</davinci_init>\n");
	return 0;

davinci_init_unregister_driver:
	driver_unregister(&davinci_driver);

davinci_init_free_layer_objects:
	for (j = 0; j < free_layer_objects_index; j++) {
		if (davinci_dm.dev[i]) {
			kfree(davinci_dm.dev[j]);
			davinci_dm.dev[i] = NULL;
		}
	}
	return err;
}

/*
 * davinci_cleanup()
 * This function un-registers device and driver to the kernel, frees requested
 * irq handler and de-allocates memory allocated for layer objects.
 */
static void davinci_cleanup(void)
{
	int i = 0;
	printk(KERN_INFO "<davinci_cleanup>\n");

	davinci_disp_unregister_callback(&davinci_dm.event_callback);
	platform_device_unregister(&_davinci_display_device);
	driver_unregister(&davinci_driver);
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		if (davinci_dm.dev[i]) {
			kfree(davinci_dm.dev[i]);
			davinci_dm.dev[i] = NULL;
		}
	}
	printk(KERN_INFO "</davinci_cleanup>\n");
}

EXPORT_SYMBOL(davinci_display_dev);
MODULE_LICENSE("GPL");
/* Function for module initialization and cleanup */
module_init(davinci_display_init);
module_exit(davinci_cleanup);
