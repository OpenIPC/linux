#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-vmalloc.h>

#include "ambxc.h"

/* Flags that indicate a format can be used for capture/output */
#define MEM2MEM_CAPTURE	(1 << 0)
#define MEM2MEM_OUTPUT	(1 << 1)

extern struct ambxc_q_data* get_q_data(struct ambxc_ctx *ctx,
				       enum v4l2_buf_type type);

struct ambxc_fmt formats[] = {
	{
		.name	= "H264 with start codes",
		.fourcc	= V4L2_PIX_FMT_H264,
		.types	= MEM2MEM_CAPTURE | MEM2MEM_OUTPUT,
	},
	{
		.name	= "MPEG-2 ES",
		.fourcc	= V4L2_PIX_FMT_MPEG2,
		.types	= MEM2MEM_CAPTURE | MEM2MEM_OUTPUT,
	},
	{
		.name	= "MPEG-1/2/4 Multiplexed",
		.fourcc	= V4L2_PIX_FMT_MPEG,
		.types	= MEM2MEM_CAPTURE | MEM2MEM_OUTPUT,
	},
};

struct v4l2_frmsize_discrete framesize[] = {

	{ 1920, 1080},
	{ 1280, 720},
};

static struct ambxc_fmt* find_format(struct v4l2_format *f)
{
	struct ambxc_fmt *fmt;
	unsigned int k;

	for (k = 0; k < NUM_FORMATS; k++) {
		fmt = &formats[k];
		if (fmt->fourcc == f->fmt.pix.pixelformat)
			break;
	}

	if (k == NUM_FORMATS)
		return NULL;

	return &formats[k];
}

struct v4l2_frmsize_discrete* find_framesize(struct ambxc_ctx *ctx,
					     struct v4l2_format *f)
{
	struct v4l2_frmsize_discrete *frmsz;
	struct ambxc_q_data *q_data;

	int i = 0;
	frmsz = &framesize[0];

	/* find the exact match first */
	for (i = 0; i < ARRAY_SIZE(framesize); i++) {

		frmsz = &framesize[i];

		if (frmsz->height == f->fmt.pix.height &&
		    frmsz->width == f->fmt.pix.width)
			return frmsz;
	}

	/* return current framesize */
	q_data = get_q_data(ctx, f->type);

	for (i = 0; i < ARRAY_SIZE(framesize); i++) {

		frmsz = &framesize[i];

		if (frmsz->height == q_data->height &&
		    frmsz->width == q_data->width)
			return frmsz;
	}

	/* return the last available framesize */
	return frmsz;
}


/*
 * video ioctls
 */
static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strncpy(cap->driver, AMBXC_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, AMBXC_NAME, sizeof(cap->card) - 1);
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", AMBXC_NAME);
	cap->device_caps = V4L2_CAP_VIDEO_M2M |
			V4L2_CAP_VIDEO_CAPTURE |
			V4L2_CAP_VIDEO_OUTPUT |
			V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static struct v4l2_input input = {
	.name = "Camera 0",
	.type = V4L2_INPUT_TYPE_CAMERA,
	.index = 0,
};

static int enum_fmt(struct v4l2_fmtdesc *f, u32 type)
{
	int i, num;
	struct ambxc_fmt *fmt;

	num = 0;

	for (i = 0; i < NUM_FORMATS; ++i) {
		if (formats[i].types & type) {
			/* index-th format of type type found ? */
			if (num == f->index)
				break;
			/* Correct type but haven't reached our index yet,
			 * just increment per-type index */
			++num;
		}
	}

	if (i < NUM_FORMATS) {
		/* Format found */
		fmt = &formats[i];
		strncpy(f->description, fmt->name, sizeof(f->description) - 1);
		f->pixelformat = fmt->fourcc;
		f->flags = V4L2_FMT_FLAG_COMPRESSED;
		return 0;
	}

	/* Format not found */
	return -EINVAL;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	return enum_fmt(f, MEM2MEM_CAPTURE);
}

static int vidioc_enum_fmt_vid_out(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	return enum_fmt(f, MEM2MEM_OUTPUT);
}

static int vidioc_g_fmt(struct ambxc_ctx *ctx, struct v4l2_format *f)
{
	struct vb2_queue *vq;
	struct ambxc_q_data *q_data;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	q_data = get_q_data(ctx, f->type);

	f->fmt.pix.width	= q_data->width;
	f->fmt.pix.height	= q_data->height;
	f->fmt.pix.field	= V4L2_FIELD_NONE;
	f->fmt.pix.pixelformat	= q_data->fmt->fourcc;
	f->fmt.pix.bytesperline	= 0;
	f->fmt.pix.sizeimage	= BUF_SIZE;

	return 0;
}

static int vidioc_g_fmt_vid_out(struct file *file, void *priv,
				struct v4l2_format *f)
{
	return vidioc_g_fmt(file2ctx(file), f);
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	return vidioc_g_fmt(file2ctx(file), f);
}

static int vidioc_try_fmt(struct v4l2_format *f, struct ambxc_fmt *fmt)
{
	enum v4l2_field field;

	field = f->fmt.pix.field;

	if (field == V4L2_FIELD_ANY)
		field = V4L2_FIELD_NONE;
	else if (field != V4L2_FIELD_NONE)
		return -EINVAL;

	f->fmt.pix.field = field;
	f->fmt.pix.bytesperline = 0;
	f->fmt.pix.sizeimage = BUF_SIZE;

	return 0;
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct ambxc_fmt *fmt;
	struct ambxc_ctx *ctx = file2ctx(file);

	fmt = find_format(f);
	if (!fmt || !(fmt->types & MEM2MEM_CAPTURE)) {
		v4l2_err(&ctx->dev->v4l2_dev,
			 "Fourcc format (0x%08x) invalid.\n",
			 f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	return vidioc_try_fmt(f, fmt);
}

static int vidioc_try_fmt_vid_out(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct ambxc_fmt *fmt;
	struct ambxc_ctx *ctx = file2ctx(file);

	fmt = find_format(f);
	if (!fmt || !(fmt->types & MEM2MEM_OUTPUT)) {
		v4l2_err(&ctx->dev->v4l2_dev,
			 "Fourcc format (0x%08x) invalid.\n",
			 f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	return vidioc_try_fmt(f, fmt);
}

static int vidioc_s_fmt(struct ambxc_ctx *ctx, struct v4l2_format *f)
{
	struct ambxc_q_data *q_data;
	struct vb2_queue *vq;
	struct v4l2_frmsize_discrete *frmsz;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	q_data = get_q_data(ctx, f->type);
	if (!q_data)
		return -EINVAL;

	if (vb2_is_busy(vq)) {
		v4l2_err(&ctx->dev->v4l2_dev, "%s queue busy\n", __func__);
		return -EBUSY;
	}

	frmsz = find_framesize(ctx, f);

	q_data->fmt		= find_format(f);
	q_data->width		= frmsz->width;
	q_data->height		= frmsz->height;
	q_data->sizeimage	= BUF_SIZE;

	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	int ret;

	ret = vidioc_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	return vidioc_s_fmt(file2ctx(file), f);
}

static int vidioc_s_fmt_vid_out(struct file *file, void *priv,
				struct v4l2_format *f)
{
	int ret;

	ret = vidioc_try_fmt_vid_out(file, priv, f);
	if (ret)
		return ret;

	ret = vidioc_s_fmt(file2ctx(file), f);
	return ret;
}

static int vidioc_enum_framesizes(struct file *file,
				  void *priv, struct v4l2_frmsizeenum *frms)
{
	if (frms->index >= ARRAY_SIZE(framesize))
		return -EINVAL;

	switch (frms->pixel_format) {
	case V4L2_PIX_FMT_H264:
	case V4L2_PIX_FMT_MPEG2:
		frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		frms->discrete = framesize[frms->index];

		return 0;
	default:
		return -EINVAL;
	}
}

static int vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *reqbufs)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int vidioc_querybuf(struct file *file, void *priv,
			   struct v4l2_buffer *buf)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_querybuf(file, ctx->m2m_ctx, buf);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
}

static int vidioc_streamon(struct file *file, void *priv,
			   enum v4l2_buf_type type)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static int vidioc_streamoff(struct file *file, void *priv,
			    enum v4l2_buf_type type)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
}

static int vidioc_enum_input(struct file *file, void *priv,
			     struct v4l2_input *inp)
{
	if (inp->index >= 1)
		return -EINVAL;

	inp->type  = V4L2_INPUT_TYPE_CAMERA;
	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	if (i >= 1)
		return -EINVAL;

	return 0;
}

static int vidioc_enum_output(struct file *file, void *priv,
			     struct v4l2_output *outp)
{
	if (outp->index >= 1)
		return -EINVAL;

	return 0;
}

static int vidioc_g_output(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int vidioc_s_output(struct file *file, void *priv, unsigned int i)
{
	if (i >= 1)
		return -EINVAL;

	return 0;
}

extern int ambxc_ctl_send(const char *buf, int count);

struct ambxc_cmd {

	char *cmd;
	int dly;
};

struct ambxc_cmd cmd_1080i_xc_start[] = {
//	{"d:\\ash\\Xcode\\XCODE_H2H_1080I_ES_NFS_CFG.ash", 8},
//	{"d:\\ash\\Xcode\\XCODE_H2H_1080I_ES_NFS_START.ash", 0},
	{ NULL, 0 }
};

struct ambxc_cmd cmd_720p_xc_start[] = {
//	{"d:\\ash\\Xcode\\XCODE_H2H_720P_ES_NFS_CFG.ash", 8},
//	{"d:\\ash\\Xcode\\XCODE_H2H_720P_ES_NFS_START.ash", 0},
	{ NULL, 0 }
};

struct ambxc_cmd *cmd_xc_start = cmd_720p_xc_start;

struct timer_list	cmd_timer;

struct ambxc_cmd cmd_enc_stop[] = {
	{ "amba voch stop --par ENC_H264_1080I_AO_STOP -s 0 -c 0T", 1 },
	{ NULL, 0 }
};

static void do_xc_cmd(unsigned long priv)
{
	struct ambxc_cmd *cmds = (void *)priv;
	char cmd_buf[128];

	while (cmds->cmd) {

		sprintf(cmd_buf, "%s\n", cmds->cmd);
		printk(KERN_DEBUG "%s\n", cmd_buf);
		ambxc_ctl_send(cmd_buf, strlen(cmd_buf));
		mdelay(cmds->dly * 1000);
		cmds++;
	}
}

void do_xc_start(struct ambxc_ctx *ctx)
{
	struct ambxc_q_data *q_data = get_q_data(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);

	dprintk(ctx->dev, "do_xc_start:\n");

	if (q_data->height == 720)
		cmd_xc_start = cmd_720p_xc_start;
	else
		cmd_xc_start = cmd_1080i_xc_start;

	do_xc_cmd((unsigned long)cmd_xc_start);
}

static void do_xc_stop(struct ambxc_ctx *ctx)
{
	dprintk(ctx->dev, "do_xcode_stop: (wait EOS from itron for 20 secs)\n");

	setup_timer(&cmd_timer, do_xc_cmd, (unsigned long)cmd_enc_stop);
	mod_timer(&cmd_timer, jiffies + msecs_to_jiffies(20 * 1000));
}

int vidioc_encoder_cmd(struct file *file, void *priv,
		       struct v4l2_encoder_cmd *cmd)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	switch (cmd->cmd) {
	case V4L2_ENC_CMD_START:

		if (cmd->flags != 0)
			return -EINVAL;

		do_xc_start(ctx);

		break;

	case V4L2_ENC_CMD_STOP:

		if (cmd->flags != 0)
			return -EINVAL;

		break;

	default:
		return -EINVAL;
	}
	return 0;
}

int vidioc_decoder_cmd(struct file *file, void *priv,
		       struct v4l2_decoder_cmd *cmd)
{
	struct ambxc_ctx *ctx = file2ctx(file);
	switch (cmd->cmd) {
	case V4L2_DEC_CMD_START:
		if (cmd->flags != 0)
			return -EINVAL;

		do_xc_start(ctx);
		break;

	case V4L2_DEC_CMD_STOP:
		if (cmd->flags != 0)
			return -EINVAL;

		while (!list_empty(&ctx->m2m_ctx->out_q_ctx.rdy_queue)) {
			dprintk(ctx->dev, "wait until all queued OUTPUT buffers are sent\n");
			msleep(1000);
		}

		do_xc_stop(ctx);

		break;

	default:
		return -EINVAL;

	}
	return 0;
}

static int vidioc_subscribe_event(struct v4l2_fh *fh,
				  const struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_EOS:
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	default:
		return -EINVAL;
	}
}

static int vidioc_g_parm(struct file *file, void *fh,
			 struct v4l2_streamparm *a) {
	int rval = 0;
#if 0
	struct omap24xxcam_fh *ofh = fh;
	struct omap24xxcam_device *cam = ofh->cam;

	mutex_lock(&cam->mutex);
	rval = vidioc_int_g_parm(cam->sdev, a);
	mutex_unlock(&cam->mutex);
#endif
	return rval;
}

static int vidioc_s_parm(struct file *file, void *fh,
			 struct v4l2_streamparm *a)
{
	int rval = 0;
#if 0
	struct omap24xxcam_fh *ofh = fh;
	struct omap24xxcam_device *cam = ofh->cam;
	struct v4l2_streamparm old_streamparm;
	int rval;

	mutex_lock(&cam->mutex);
	if (cam->streaming) {
		rval = -EBUSY;
		goto out;
	}

	old_streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rval = vidioc_int_g_parm(cam->sdev, &old_streamparm);
	if (rval)
		goto out;

	rval = vidioc_int_s_parm(cam->sdev, a);
	if (rval)
		goto out;

	rval = omap24xxcam_sensor_if_enable(cam);
	/*
	 * Revert to old streaming parameters if enabling sensor
	 * interface with the new ones failed.
	 */
	if (rval)
		vidioc_int_s_parm(cam->sdev, &old_streamparm);

out:
	mutex_unlock(&cam->mutex);
#endif

	return rval;
}

const struct v4l2_ioctl_ops ambxc_ioctl_ops = {

	.vidioc_querycap	= vidioc_querycap,

	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap	= vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap	= vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap	= vidioc_s_fmt_vid_cap,

	.vidioc_enum_fmt_vid_out = vidioc_enum_fmt_vid_out,
	.vidioc_g_fmt_vid_out	= vidioc_g_fmt_vid_out,
	.vidioc_try_fmt_vid_out	= vidioc_try_fmt_vid_out,
	.vidioc_s_fmt_vid_out	= vidioc_s_fmt_vid_out,

	.vidioc_reqbufs		= vidioc_reqbufs,
	.vidioc_querybuf	= vidioc_querybuf,

	.vidioc_qbuf		= vidioc_qbuf,
	.vidioc_dqbuf		= vidioc_dqbuf,

	.vidioc_streamon	= vidioc_streamon,
	.vidioc_streamoff	= vidioc_streamoff,

	.vidioc_enum_input	= vidioc_enum_input,
	.vidioc_g_input		= vidioc_g_input,
	.vidioc_s_input		= vidioc_s_input,

	.vidioc_enum_output	= vidioc_enum_output,
	.vidioc_g_output	= vidioc_g_output,
	.vidioc_s_output	= vidioc_s_output,

	.vidioc_encoder_cmd	= vidioc_encoder_cmd,
	.vidioc_decoder_cmd	= vidioc_decoder_cmd,

	.vidioc_enum_framesizes = vidioc_enum_framesizes,

	.vidioc_subscribe_event = vidioc_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,

	.vidioc_g_parm		= vidioc_g_parm,
	.vidioc_s_parm		= vidioc_s_parm,
};
