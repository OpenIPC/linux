// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 video capture nodes
 *
 * Each VPU channel has a videobuf2 queue of DMA-contiguous NV12 frames.
 * The sensor, CSI receiver and ISP run while any node streams.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/fh8626-isp.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>

#include "fh8626-pipeline.h"
#include "fh8626-event.h"

/*
 * Each channel holds up to two buffers, the frame being written and the
 * one programmed for the next frame; a third lets userspace work on a
 * frame without forcing drops.
 */
#define FH8626_VIDEO_MIN_BUFS		3
#define FH8626_VIDEO_MAX_BUFS		6

struct fh8626_size {
	u16 width;
	u16 height;
};

/* Output sizes of each channel, largest first */
static const struct fh8626_size fh8626_chan_sizes[FH8626_VPU_CHANNELS][3] = {
	{
		{ FH_FRAME_WIDTH_MAX, FH_FRAME_HEIGHT_MAX },
		{ FH_FRAME_WIDTH_DEFAULT, FH_FRAME_HEIGHT_DEFAULT },
		{ 640, 360 },
	}, {
		{ 640, 360 },
		{ 320, 180 },
	}, {
		{ 320, 180 },
		{ 160, 90 },
	},
};

static const char *const fh8626_chan_names[FH8626_VPU_CHANNELS] = {
	"fh8626-camera", "fh8626-vpu-ch1", "fh8626-vpu-ch2",
};

struct fh8626_video_dev {
	struct kref		ref;
	struct device		*dev;
	struct video_device	vdev;
	struct vb2_queue	queue;
	struct mutex		lock;
	struct fh8626_pipeline	*pipe;
	unsigned int		chan;
	struct v4l2_pix_format	pix_fmt;
	struct v4l2_ctrl_handler ctrls;
	struct video_device	*sensor_node, *isp_node;
	bool			ready;
	bool			streaming;
#ifdef CONFIG_MEDIA_CONTROLLER
	struct media_pad	pad;
	bool graph_streaming;
#endif
};

static void fh8626_grab_layout(struct fh8626_pipeline *pipe, bool grab)
{
	struct v4l2_ctrl *ctrl;

	if (!pipe->sensor_sd || !pipe->sensor_sd->ctrl_handler)
		return;
	ctrl = v4l2_ctrl_find(pipe->sensor_sd->ctrl_handler, V4L2_CID_HFLIP);
	if (ctrl)
		v4l2_ctrl_grab(ctrl, grab);
	ctrl = v4l2_ctrl_find(pipe->sensor_sd->ctrl_handler, V4L2_CID_VFLIP);
	if (ctrl)
		v4l2_ctrl_grab(ctrl, grab);
}

static void fh8626_video_input_size(struct fh8626_video_dev *vdev,
				    u32 *width, u32 *height)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE
	};
	struct fh8626_pipeline *pipe = vdev ? vdev->pipe : NULL;

	*width = FH_FRAME_WIDTH_DEFAULT;
	*height = FH_FRAME_HEIGHT_DEFAULT;
	if (!pipe)
		return;
	if (pipe->sensor_sd &&
	    !v4l2_subdev_call(pipe->sensor_sd, pad, get_fmt, NULL, &fmt)) {
		if (fmt.format.width == FH_FRAME_WIDTH_MAX &&
		    fmt.format.height == FH_FRAME_HEIGHT_MAX) {
			*width = FH_FRAME_WIDTH_MAX;
			*height = FH_FRAME_HEIGHT_MAX;
			return;
		}
	}
	if (pipe->isp_sd &&
	    !v4l2_subdev_call(pipe->isp_sd, pad, get_fmt, NULL, &fmt)) {
		if (fmt.format.width == FH_FRAME_WIDTH_MAX &&
		    fmt.format.height == FH_FRAME_HEIGHT_MAX) {
			*width = FH_FRAME_WIDTH_MAX;
			*height = FH_FRAME_HEIGHT_MAX;
		}
	}
}

static const struct fh8626_size *
fh8626_video_size(struct fh8626_video_dev *vdev, unsigned int index)
{
	const struct fh8626_size *s = fh8626_chan_sizes[vdev->chan];
	unsigned int i;
	u32 in_w, in_h;

	fh8626_video_input_size(vdev, &in_w, &in_h);
	for (i = 0; i < ARRAY_SIZE(fh8626_chan_sizes[0]) && s[i].width; ++i) {
		if (s[i].width > in_w || s[i].height > in_h)
			continue;
		if (!index--)
			return &s[i];
	}
	return NULL;
}

static void fh8626_video_fill_fmt(struct v4l2_pix_format *pix, u32 width,
				  u32 height)
{
	memset(pix, 0, sizeof(*pix));
	pix->width = width;
	pix->height = height;
	pix->pixelformat = V4L2_PIX_FMT_NV12;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = width;
	pix->sizeimage = width * height * 3 / 2;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->xfer_func = V4L2_XFER_FUNC_SRGB;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_601;
	pix->quantization = V4L2_QUANTIZATION_LIM_RANGE;
}

static int fh8626_queue_setup(struct vb2_queue *vq,
			      unsigned int *nbuffers, unsigned int *nplanes,
			      unsigned int sizes[], struct device *alloc_devs[])
{
	struct fh8626_video_dev *vdev = vb2_get_drv_priv(vq);
	unsigned int size = vdev->pix_fmt.sizeimage;

	/* read()/poll() may race with disconnect; do not recreate buffers */
	if (!vdev->pipe || !vdev->ready)
		return -ENODEV;
	if (*nplanes)
		return *nplanes != 1 || sizes[0] < size ? -EINVAL : 0;

	*nbuffers = clamp_t(unsigned int, *nbuffers, FH8626_VIDEO_MIN_BUFS,
			    FH8626_VIDEO_MAX_BUFS);
	*nplanes = 1;
	sizes[0] = size;
	return 0;
}

static int fh8626_buf_prepare(struct vb2_buffer *vb)
{
	struct fh8626_video_dev *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct fh8626_buffer *buf =
		container_of(vbuf, struct fh8626_buffer, vb);
	unsigned int size = vdev->pix_fmt.sizeimage;

	if (vb2_plane_size(vb, 0) < size)
		return -EINVAL;
	buf->paddr = vb2_dma_contig_plane_dma_addr(vb, 0);
	vb2_set_plane_payload(vb, 0, size);
	return 0;
}

static void fh8626_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct fh8626_buffer *buf =
		container_of(vbuf, struct fh8626_buffer, vb);
	struct fh8626_video_dev *vdev = vb2_get_drv_priv(vb->vb2_queue);
	struct fh8626_pipeline *pipe = vdev->pipe;

	if (!pipe || !pipe->vpu) {
		vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
		return;
	}
	fh8626_vpu_chan_queue(pipe->vpu, vdev->chan, buf);
}

/*
 * Grab flip controls before reading the Bayer phase. Propagate the sensor
 * format before starting the pipeline, while active formats may change.
 */
static int fh8626_pipeline_configure(struct fh8626_pipeline *pipe,
				     struct v4l2_mbus_framefmt *in)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE
	};
	int ret;

	fh8626_grab_layout(pipe, true);
	ret = v4l2_subdev_call(pipe->sensor_sd, pad, get_fmt, NULL, &fmt);
	if (!ret)
		ret = v4l2_subdev_call(pipe->csi_sd, pad, set_fmt, NULL, &fmt);
	if (!ret)
		ret = v4l2_subdev_call(pipe->isp_sd, pad, set_fmt, NULL, &fmt);
	if (ret)
		fh8626_grab_layout(pipe, false);
	else
		*in = fmt.format;
	return ret;
}

static int fh8626_pipeline_run(struct fh8626_pipeline *pipe)
{
	int ret;

	ret = fh8626_isp_prepare(pipe->isp);
	if (ret)
		return ret;
	ret = v4l2_subdev_call(pipe->csi_sd, video, s_stream, 1);
	if (ret)
		goto err_csi;
	ret = v4l2_subdev_call(pipe->isp_sd, video, s_stream, 1);
	if (ret)
		goto err_isp;
	ret = v4l2_subdev_call(pipe->sensor_sd, video, s_stream, 1);
	if (ret)
		goto err_isp;
	return 0;

err_isp:
	v4l2_subdev_call(pipe->isp_sd, video, s_stream, 0);
err_csi:
	v4l2_subdev_call(pipe->csi_sd, video, s_stream, 0);
	return ret;
}

/*
 * Stop the producers. Every channel has been drained; the ISP brings its
 * own DMA to rest on a frame boundary before the sensor stops.
 */
static void fh8626_pipeline_halt(struct fh8626_pipeline *pipe,
				 struct device *dev)
{
	if (pipe->isp && fh8626_isp_quiesce(pipe->isp))
		dev_warn(dev, "ISP did not reach a frame boundary\n");
	if (pipe->sensor_sd)
		v4l2_subdev_call(pipe->sensor_sd, video, s_stream, 0);
	if (pipe->isp_sd)
		v4l2_subdev_call(pipe->isp_sd, video, s_stream, 0);
	if (pipe->csi_sd)
		v4l2_subdev_call(pipe->csi_sd, video, s_stream, 0);
}

static int fh8626_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct fh8626_video_dev *vdev = vb2_get_drv_priv(vq);
	struct fh8626_pipeline *pipe = vdev->pipe;
	struct v4l2_mbus_framefmt in;
	bool first = false;
	int ret;

	if (!pipe)
		return -ENODEV;
	mutex_lock(&pipe->stream_lock);
	if (!vdev->ready || !pipe->vpu || !pipe->isp || !pipe->isp_sd ||
	    !pipe->csi_sd || !pipe->sensor_sd) {
		ret = -ENODEV;
		goto err_bufs;
	}
	first = !pipe->stream_count;
	if (first) {
		ret = fh8626_pipeline_configure(pipe, &in);
		if (ret)
			goto err_bufs;
	}
#ifdef CONFIG_MEDIA_CONTROLLER
	ret = media_pipeline_start(&vdev->vdev.entity, &pipe->media_pipe);
	if (ret)
		goto err_layout;
	vdev->graph_streaming = true;
#endif
	ret = fh8626_vpu_chan_set_format(pipe->vpu, vdev->chan,
					 vdev->pix_fmt.width,
					 vdev->pix_fmt.height);
	if (ret)
		goto err_layout;
	/* Reset the VPU before programming the shared ISP register window */
	if (first) {
		ret = fh8626_vpu_pipeline_start(pipe->vpu, in.width,
						in.height);
		if (ret)
			goto err_layout;
	}
	ret = fh8626_vpu_chan_start(pipe->vpu, vdev->chan);
	if (ret)
		goto err_vpu;
	/*
	 * Frames only flow once the sensor runs, so a failed start returns
	 * every buffer in the QUEUED state.
	 */
	if (first) {
		ret = fh8626_pipeline_run(pipe);
		if (ret)
			goto err_vpu;
	}
	pipe->stream_count++;
	vdev->streaming = true;
	mutex_unlock(&pipe->stream_lock);
	return 0;

err_vpu:
	if (first) {
		fh8626_vpu_chan_stop(pipe->vpu, vdev->chan,
				     VB2_BUF_STATE_QUEUED);
		fh8626_vpu_pipeline_stop(pipe->vpu);
	}
err_layout:
	if (first)
		fh8626_grab_layout(pipe, false);
err_bufs:
	if (pipe->vpu)
		fh8626_vpu_chan_stop(pipe->vpu, vdev->chan,
				     VB2_BUF_STATE_QUEUED);
#ifdef CONFIG_MEDIA_CONTROLLER
	if (vdev->graph_streaming) {
		media_pipeline_stop(&vdev->vdev.entity);
		vdev->graph_streaming = false;
	}
#endif
	mutex_unlock(&pipe->stream_lock);
	return ret;
}

static void fh8626_stop_streaming(struct vb2_queue *vq)
{
	struct fh8626_video_dev *vdev = vb2_get_drv_priv(vq);
	struct fh8626_pipeline *pipe = vdev->pipe;
	bool last;

	if (!pipe || !vdev->streaming)
		return;
	mutex_lock(&pipe->stream_lock);
	vdev->streaming = false;
	/* The channel must reach a frame boundary while its input still runs */
	if (fh8626_vpu_chan_drain(pipe->vpu, vdev->chan))
		dev_warn(vdev->dev, "channel %u did not reach a frame boundary\n",
			 vdev->chan);
	last = pipe->stream_count == 1;
	if (last)
		fh8626_pipeline_halt(pipe, vdev->dev);
	fh8626_vpu_chan_stop(pipe->vpu, vdev->chan, VB2_BUF_STATE_ERROR);
	if (last) {
		fh8626_vpu_pipeline_stop(pipe->vpu);
		fh8626_grab_layout(pipe, false);
	}
	if (pipe->stream_count)
		pipe->stream_count--;
#ifdef CONFIG_MEDIA_CONTROLLER
	if (vdev->graph_streaming) {
		media_pipeline_stop(&vdev->vdev.entity);
		vdev->graph_streaming = false;
	}
#endif
	mutex_unlock(&pipe->stream_lock);
}

static const struct vb2_ops fh8626_qops = {
	.queue_setup		= fh8626_queue_setup,
	.buf_prepare		= fh8626_buf_prepare,
	.buf_queue		= fh8626_buf_queue,
	.start_streaming	= fh8626_start_streaming,
	.stop_streaming		= fh8626_stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

static int fh8626_vidioc_querycap(struct file *file, void *priv,
				  struct v4l2_capability *cap)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);

	strlcpy(cap->driver, "fh8626-v4l2", sizeof(cap->driver));
	if (vdev->chan)
		snprintf(cap->card, sizeof(cap->card),
			 "Fullhan FH8626V100 Camera ch%u", vdev->chan);
	else
		strlcpy(cap->card, "Fullhan FH8626V100 Camera",
			sizeof(cap->card));
	strlcpy(cap->bus_info, "platform:fh8626-camera",
		sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
			   V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int fh8626_vidioc_enum_fmt_vid_cap(struct file *file, void *priv,
					  struct v4l2_fmtdesc *f)
{
	if (f->index != 0)
		return -EINVAL;

	f->pixelformat = V4L2_PIX_FMT_NV12;
	strlcpy(f->description, "Y/CbCr 4:2:0 Semi-Planar",
		sizeof(f->description));
	return 0;
}

static int fh8626_enum_input(struct file *file, void *priv,
			     struct v4l2_input *input)
{
	if (input->index)
		return -EINVAL;
	strlcpy(input->name, "Camera", sizeof(input->name));
	input->type = V4L2_INPUT_TYPE_CAMERA;
	return 0;
}

static int fh8626_g_input(struct file *file, void *priv, unsigned int *input)
{
	*input = 0;
	return 0;
}

static int fh8626_s_input(struct file *file, void *priv, unsigned int input)
{
	return input ? -EINVAL : 0;
}

static int fh8626_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);

	f->fmt.pix = vdev->pix_fmt;
	return 0;
}

static int fh8626_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
					 struct v4l2_format *f)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	const struct fh8626_size *s, *best = NULL;
	u32 w = f->fmt.pix.width;
	u32 h = f->fmt.pix.height;
	unsigned int i;

	for (i = 0; (s = fh8626_video_size(vdev, i)); ++i) {
		best = s;
		if (s->width <= w && s->height <= h)
			break;
	}
	if (!best)
		return -EINVAL;
	fh8626_video_fill_fmt(&f->fmt.pix, best->width, best->height);
	return 0;
}

static int fh8626_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	int ret;

	if (vb2_is_busy(&vdev->queue))
		return -EBUSY;
	ret = fh8626_vidioc_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;
	if (vdev->pipe && vdev->pipe->vpu) {
		ret = fh8626_vpu_chan_set_format(vdev->pipe->vpu, vdev->chan,
						 f->fmt.pix.width,
						 f->fmt.pix.height);
		if (ret)
			return ret;
	}
	vdev->pix_fmt = f->fmt.pix;
	return 0;
}

static int fh8626_enum_framesizes(struct file *file, void *priv,
				  struct v4l2_frmsizeenum *f)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	const struct fh8626_size *s;

	if (f->pixel_format != V4L2_PIX_FMT_NV12)
		return -EINVAL;
	s = fh8626_video_size(vdev, f->index);
	if (!s)
		return -EINVAL;
	f->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	f->discrete.width = s->width;
	f->discrete.height = s->height;
	return 0;
}

static bool fh8626_video_size_valid(struct fh8626_video_dev *vdev, u32 width,
				    u32 height)
{
	const struct fh8626_size *s;
	unsigned int i;

	for (i = 0; (s = fh8626_video_size(vdev, i)); ++i)
		if (s->width == width && s->height == height)
			return true;
	return false;
}

static int fh8626_enum_frameintervals(struct file *file, void *priv,
				      struct v4l2_frmivalenum *f)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	struct v4l2_subdev_frame_interval interval = { 0 };
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	struct v4l2_subdev_frame_interval_enum fie = {
		.index = f->index,
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	struct v4l2_subdev *sensor;
	int ret = -ENOIOCTLCMD;

	if (f->pixel_format != V4L2_PIX_FMT_NV12 ||
	    !fh8626_video_size_valid(vdev, f->width, f->height))
		return -EINVAL;

	/*
	 * The scaler keeps the frame rate: every output size has the
	 * intervals the sensor lists for its active format.
	 */
	sensor = vdev->pipe ? vdev->pipe->sensor_sd : NULL;
	f->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	if (sensor &&
	    !v4l2_subdev_call(sensor, pad, get_fmt, NULL, &fmt)) {
		fie.code = fmt.format.code;
		fie.width = fmt.format.width;
		fie.height = fmt.format.height;
		ret = v4l2_subdev_call(sensor, pad, enum_frame_interval,
				       NULL, &fie);
		if (!ret) {
			f->discrete = fie.interval;
			return 0;
		}
		if (ret != -ENOIOCTLCMD || f->index)
			return -EINVAL;
	}
	if (f->index)
		return -EINVAL;

	/* Without a list, the current interval is the only one */
	if (sensor)
		ret = v4l2_subdev_call(sensor, video, g_frame_interval,
				       &interval);
	if (ret || !interval.interval.numerator ||
	    !interval.interval.denominator) {
		interval.interval.numerator = 1;
		interval.interval.denominator = 25;
	}
	f->discrete = interval.interval;
	return 0;
}

static int fh8626_vidioc_g_parm(struct file *file, void *priv,
				struct v4l2_streamparm *p)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	struct v4l2_subdev_frame_interval interval = { 0 };
	int ret = -ENOIOCTLCMD;

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(&p->parm.capture, 0, sizeof(p->parm.capture));
	p->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	p->parm.capture.readbuffers = FH8626_VIDEO_MIN_BUFS;

	if (vdev->pipe && vdev->pipe->sensor_sd)
		ret = v4l2_subdev_call(vdev->pipe->sensor_sd, video,
				       g_frame_interval, &interval);
	if (ret || !interval.interval.numerator ||
	    !interval.interval.denominator) {
		p->parm.capture.timeperframe.numerator = 1;
		p->parm.capture.timeperframe.denominator = 25;
	} else {
		p->parm.capture.timeperframe = interval.interval;
	}
	return 0;
}

static int fh8626_vidioc_s_parm(struct file *file, void *priv,
				struct v4l2_streamparm *p)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	struct v4l2_subdev_frame_interval ival = { 0 };

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	ival.interval = p->parm.capture.timeperframe;
	if (vdev->pipe && vdev->pipe->sensor_sd)
		v4l2_subdev_call(vdev->pipe->sensor_sd, video,
				 s_frame_interval, &ival);
	return fh8626_vidioc_g_parm(file, priv, p);
}

static int fh8626_vidioc_g_selection(struct file *file, void *priv,
				     struct v4l2_selection *s)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	u32 in_w, in_h;

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	fh8626_video_input_size(vdev, &in_w, &in_h);
	switch (s->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = in_w;
		s->r.height = in_h;
		break;
	case V4L2_SEL_TGT_COMPOSE:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = vdev->pix_fmt.width;
		s->r.height = vdev->pix_fmt.height;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ioctl_ops fh8626_ioctl_ops = {
	.vidioc_querycap		= fh8626_vidioc_querycap,
	.vidioc_enum_input		= fh8626_enum_input,
	.vidioc_g_input			= fh8626_g_input,
	.vidioc_s_input			= fh8626_s_input,
	.vidioc_enum_fmt_vid_cap	= fh8626_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= fh8626_vidioc_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= fh8626_vidioc_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= fh8626_vidioc_try_fmt_vid_cap,
	.vidioc_enum_framesizes		= fh8626_enum_framesizes,
	.vidioc_enum_frameintervals	= fh8626_enum_frameintervals,
	.vidioc_g_parm			= fh8626_vidioc_g_parm,
	.vidioc_s_parm			= fh8626_vidioc_s_parm,
	.vidioc_g_selection		= fh8626_vidioc_g_selection,
	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,
	.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

static int fh8626_video_open(struct file *file)
{
	struct fh8626_video_dev *vdev = video_drvdata(file);
	int ret;

	if (mutex_lock_interruptible(&vdev->lock))
		return -ERESTARTSYS;
	ret = vdev->pipe && vdev->ready ? v4l2_fh_open(file) : -ENODEV;
	mutex_unlock(&vdev->lock);
	return ret;
}

static void fh8626_video_free(struct kref *ref)
{
	struct fh8626_video_dev *vdev =
		container_of(ref, struct fh8626_video_dev, ref);

#ifdef CONFIG_MEDIA_CONTROLLER
	media_entity_cleanup(&vdev->vdev.entity);
#endif
	v4l2_ctrl_handler_free(&vdev->ctrls);
	/* Inherited controls stay valid until the last fd is closed */
	if (vdev->sensor_node)
		put_device(&vdev->sensor_node->dev);
	if (vdev->isp_node)
		put_device(&vdev->isp_node->dev);
	mutex_destroy(&vdev->lock);
	kfree(vdev);
}

static void fh8626_video_release(struct video_device *vd)
{
	struct fh8626_video_dev *vdev =
		container_of(vd, struct fh8626_video_dev, vdev);

	kref_put(&vdev->ref, fh8626_video_free);
}

static const struct v4l2_file_operations fh8626_fops = {
	.owner		= THIS_MODULE,
	.open		= fh8626_video_open,
	.release	= vb2_fop_release,
	.read		= vb2_fop_read,
	.poll		= vb2_fop_poll,
	.mmap		= vb2_fop_mmap,
	.unlocked_ioctl	= fh8626_video_ioctl2,
};

#ifdef CONFIG_MEDIA_CONTROLLER
static int fh8626_video_link_validate(struct media_link *link)
{
	struct video_device *vd =
		media_entity_to_video_device(link->sink->entity);
	struct fh8626_video_dev *vdev = video_get_drvdata(vd);
	struct v4l2_subdev *sd =
		media_entity_to_v4l2_subdev(link->source->entity);
	struct v4l2_subdev_format f = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
		.pad = link->source->index,
	};
	bool valid_in;
	int ret = v4l2_subdev_call(sd, pad, get_fmt, NULL, &f);

	if (ret)
		return ret;
	valid_in = (f.format.width == FH_FRAME_WIDTH_DEFAULT &&
		    f.format.height == FH_FRAME_HEIGHT_DEFAULT) ||
		   (f.format.width == FH_FRAME_WIDTH_MAX &&
		    f.format.height == FH_FRAME_HEIGHT_MAX);
	if (!valid_in || f.format.code != MEDIA_BUS_FMT_YUYV8_1X16 ||
	    f.format.field != V4L2_FIELD_NONE)
		return -EPIPE;
	if (vdev && (vdev->pix_fmt.width > f.format.width ||
		     vdev->pix_fmt.height > f.format.height))
		return -EPIPE;
	return 0;
}

static const struct media_entity_operations fh8626_video_entity_ops = {
	.link_validate = fh8626_video_link_validate,
};
#endif

static bool fh8626_video_ctrl_filter(const struct v4l2_ctrl *ctrl)
{
	/*
	 * Inherit standard controls and the privacy masks, which cover every
	 * channel; ISP statistics and tuning stay on the subdevice.
	 */
	return v4l2_ctrl_get_name(ctrl->id) != NULL ||
	       ctrl->id == V4L2_CID_FH8626_PRIVACY_MASKS;
}

static int fh8626_video_add_controls(struct fh8626_video_dev *vdev,
		struct v4l2_subdev *sd, struct video_device **node)
{
	int ret;

	/* Called under the async notifier's publication/unbind serialization. */
	if (!sd->devnode || !video_is_registered(sd->devnode)) {
		ret = -ENODEV;
	} else {
		*node = sd->devnode;
		get_device(&(*node)->dev);
		ret = v4l2_ctrl_add_handler(&vdev->ctrls, sd->ctrl_handler,
					  fh8626_video_ctrl_filter);
	}
	return ret;
}

static int fh8626_video_register_chan(struct platform_device *pdev,
				      struct fh8626_pipeline *pipe,
				      unsigned int chan)
{
	const struct fh8626_size *def = &fh8626_chan_sizes[chan][0];
	struct fh8626_video_dev *vdev;
	struct video_device *vd;
	struct vb2_queue *q;
	int ret;

	vdev = kzalloc(sizeof(*vdev), GFP_KERNEL);
	if (!vdev)
		return -ENOMEM;

	kref_init(&vdev->ref);
	vdev->dev = &pdev->dev;
	vdev->pipe = pipe;
	vdev->chan = chan;
	mutex_init(&vdev->lock);
	if (!chan)
		def = &fh8626_chan_sizes[0][1];
	fh8626_video_fill_fmt(&vdev->pix_fmt, def->width, def->height);
	fh8626_vpu_chan_set_format(pipe->vpu, chan, def->width, def->height);

	q = &vdev->queue;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_READ;
	q->drv_priv = vdev;
	q->buf_struct_size = sizeof(struct fh8626_buffer);
	q->ops = &fh8626_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 2;
	q->lock = &vdev->lock;
	q->dev = fh8626_vpu_device(pipe->vpu);

	ret = vb2_queue_init(q);
	if (ret) {
		mutex_destroy(&vdev->lock);
		kfree(vdev);
		return ret;
	}

	vd = &vdev->vdev;
	vd->fops = &fh8626_fops;
	vd->ioctl_ops = &fh8626_ioctl_ops;
	vd->release = fh8626_video_release;
	vd->v4l2_dev = &pipe->v4l2_dev;
	vd->queue = q;
	vd->lock = &vdev->lock;
	vd->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
			  V4L2_CAP_READWRITE;
	strlcpy(vd->name, fh8626_chan_names[chan], sizeof(vd->name));
	video_set_drvdata(vd, vdev);

#ifdef CONFIG_MEDIA_CONTROLLER
	vdev->pad.flags = MEDIA_PAD_FL_SINK;
	vd->entity.ops = &fh8626_video_entity_ops;
	ret = media_entity_pads_init(&vd->entity, 1, &vdev->pad);
	if (ret) {
		vb2_queue_release(q);
		mutex_destroy(&vdev->lock);
		kfree(vdev);
		return ret;
	}
#endif
	ret = v4l2_ctrl_handler_init(&vdev->ctrls, 32);
	if (!ret)
		ret = fh8626_video_add_controls(vdev, pipe->sensor_sd,
					       &vdev->sensor_node);
	if (!ret)
		ret = fh8626_video_add_controls(vdev, pipe->isp_sd,
					       &vdev->isp_node);
	if (ret)
		goto err_release;

	vd->ctrl_handler = &vdev->ctrls;
	/* device_register() failure may invoke vd->release before returning. */
	kref_get(&vdev->ref);
	ret = video_register_device(vd, VFL_TYPE_GRABBER, -1);
	if (ret) {
		dev_err(&pdev->dev, "failed to register video device: %d\n",
			ret);
		if (vd->minor == -1)
			fh8626_video_release(vd);
		vb2_queue_release(q);
		kref_put(&vdev->ref, fh8626_video_free);
		return ret;
	}
	kref_put(&vdev->ref, fh8626_video_free);
	pipe->chan_video[chan] = vd;
	if (!chan)
		pipe->video = vd;
#ifdef CONFIG_MEDIA_CONTROLLER
	/* 4.19 video registration does not propagate interface allocation errors. */
	if (!vd->intf_devnode)
		return -ENOMEM;
#endif
	dev_dbg(&pdev->dev, "registered %s\n", video_device_node_name(vd));
	return 0;

err_release:
	vb2_queue_release(q);
	fh8626_video_release(vd);
	return ret;
}

int fh8626_video_register(struct platform_device *pdev,
			  struct fh8626_pipeline *pipe)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	unsigned int chan;
	int ret;

	if (!pipe->vpu)
		return -ENODEV;
	if (pipe->sensor_sd &&
	    !v4l2_subdev_call(pipe->sensor_sd, pad, get_fmt, NULL, &fmt)) {
		if (pipe->csi_sd)
			v4l2_subdev_call(pipe->csi_sd, pad, set_fmt,
					 NULL, &fmt);
		if (pipe->isp_sd)
			v4l2_subdev_call(pipe->isp_sd, pad, set_fmt,
					 NULL, &fmt);
	}
	fh8626_vpu_set_pipeline(pipe->vpu, pipe);
	for (chan = 0; chan < FH8626_VPU_CHANNELS; ++chan) {
		ret = fh8626_video_register_chan(pdev, pipe, chan);
		if (ret)
			goto err;
		if (!chan) {
			ret = fh8626_logo_register(pdev, pipe);
			if (ret)
				goto err;
		}
	}
	return 0;

err:
	fh8626_video_unregister(pipe);
	return ret;
}

void fh8626_video_ready(struct fh8626_pipeline *pipe)
{
	struct fh8626_video_dev *vdev;
	unsigned int chan;

	/*
	 * The devnodes are visible before the notifier creates the links;
	 * only admit opens once the whole graph is built.
	 */
	for (chan = 0; chan < FH8626_VPU_CHANNELS; ++chan) {
		if (!pipe->chan_video[chan])
			continue;
		vdev = container_of(pipe->chan_video[chan],
				    struct fh8626_video_dev, vdev);
		mutex_lock(&vdev->lock);
		vdev->ready = true;
		mutex_unlock(&vdev->lock);
	}
}

void fh8626_video_shutdown(struct fh8626_pipeline *pipe)
{
	struct fh8626_video_dev *vdev;
	unsigned int chan;

	for (chan = 0; chan < FH8626_VPU_CHANNELS; ++chan) {
		if (!pipe->chan_video[chan])
			continue;
		vdev = container_of(pipe->chan_video[chan],
				    struct fh8626_video_dev, vdev);
		mutex_lock(&vdev->lock);
		vdev->ready = false;
		vb2_queue_error(&vdev->queue);
		fh8626_stop_streaming(&vdev->queue);
		mutex_unlock(&vdev->lock);
	}
}

void fh8626_video_unregister(struct fh8626_pipeline *pipe)
{
	struct fh8626_video_dev *vdev;
	struct video_device *vd;
	unsigned int chan;

	if (!pipe)
		return;
	fh8626_logo_unregister(pipe);
	for (chan = 0; chan < FH8626_VPU_CHANNELS; ++chan) {
		vd = pipe->chan_video[chan];
		if (!vd)
			continue;
		vdev = container_of(vd, struct fh8626_video_dev, vdev);
		get_device(&vd->dev);
		mutex_lock(&vdev->lock);
		fh8626_video_withdraw_interface(vd);
		video_unregister_device(vd);
		fh8626_video_wake_events(vd);
		vdev->ready = false;
		pipe->chan_video[chan] = NULL;
		if (!chan)
			pipe->video = NULL;
		vb2_queue_error(&vdev->queue);
		fh8626_stop_streaming(&vdev->queue);
		vb2_queue_release(&vdev->queue);
		vdev->queue.owner = NULL;
		vdev->pipe = NULL;
		mutex_unlock(&vdev->lock);
		put_device(&vd->dev);
	}
	fh8626_vpu_set_pipeline(pipe->vpu, NULL);
}
