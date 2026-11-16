// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
/*
 * Statistics capture and parameter output nodes of the T23 ISP.
 */
#include <linux/kernel.h>
#include <linux/bitfield.h>
#include <linux/slab.h>
#include <linux/media/ingenic/t23-isp-config.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-vmalloc.h>

#include "tx-isp-device.h"
#include "tx-isp-meta.h"

struct t23_meta_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	struct t23_isp_ext_params_cfg *cfg;
	size_t payload;
};

static inline struct t23_meta_buffer *to_t23_meta_buffer(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb), struct t23_meta_buffer, vb);
}

static size_t t23_meta_size_fmt(u32 fourcc)
{
	switch (fourcc) {
	case V4L2_META_FMT_T23_ISP_EXT_PARAMS:
		return sizeof(struct t23_isp_ext_params_cfg);
	default:
		return sizeof(struct t23_isp_stat_buffer);
	}
}

static size_t t23_meta_size(const struct t23_meta_node *node)
{
	return t23_meta_size_fmt(node->dataformat);
}

static u32 t23_meta_fourcc(const struct t23_meta_node *node)
{
	return node->dataformat;
}

static int t23_meta_queue_setup(struct vb2_queue *q, unsigned int *nbuffers,
				unsigned int *nplanes, unsigned int sizes[],
				struct device *alloc_devs[])
{
	struct t23_meta_node *node = vb2_get_drv_priv(q);

	if (*nplanes)
		return sizes[0] < t23_meta_size(node) ? -EINVAL : 0;
	*nplanes = 1;
	sizes[0] = t23_meta_size(node);
	return 0;
}

static int t23_meta_buf_init(struct vb2_buffer *vb)
{
	struct t23_meta_node *node = vb2_get_drv_priv(vb->vb2_queue);
	struct t23_meta_buffer *buf = to_t23_meta_buffer(vb);

	if (!node->output)
		return 0;
	buf->cfg = kvzalloc(sizeof(*buf->cfg), GFP_KERNEL);
	return buf->cfg ? 0 : -ENOMEM;
}

static void t23_meta_buf_cleanup(struct vb2_buffer *vb)
{
	struct t23_meta_buffer *buf = to_t23_meta_buffer(vb);

	kvfree(buf->cfg);
	buf->cfg = NULL;
	buf->payload = 0;
}

static int t23_meta_buf_prepare(struct vb2_buffer *vb)
{
	struct t23_meta_node *node = vb2_get_drv_priv(vb->vb2_queue);
	struct t23_meta_buffer *buf = to_t23_meta_buffer(vb);
	size_t hdr_len = offsetof(struct t23_isp_ext_params_cfg, data);
	size_t payload = vb2_get_plane_payload(vb, 0);
	const void *vaddr;

	if (!node->output) {
		if (vb2_plane_size(vb, 0) < t23_meta_size(node))
			return -EINVAL;
		vb2_set_plane_payload(vb, 0, t23_meta_size(node));
		return 0;
	}
	vaddr = vb2_plane_vaddr(vb, 0);
	if (!buf->cfg || !vaddr || payload < hdr_len ||
	    payload > sizeof(*buf->cfg) || payload > vb2_plane_size(vb, 0))
		return -EINVAL;
	/*
	 * Copy the user-mmapped payload into a kernel shadow once so
	 * userspace cannot mutate block headers or fields after validation.
	 */
	memcpy(buf->cfg, vaddr, payload);
	buf->payload = payload;
	return tx_isp_ext_params_check(buf->cfg, payload);
}

static void t23_meta_buf_queue(struct vb2_buffer *vb)
{
	struct t23_meta_node *node = vb2_get_drv_priv(vb->vb2_queue);
	struct t23_meta_buffer *buf = to_t23_meta_buffer(vb);
	unsigned long flags;

	spin_lock_irqsave(&node->meta->lock, flags);
	list_add_tail(&buf->list, &node->bufs);
	spin_unlock_irqrestore(&node->meta->lock, flags);
}

static int t23_meta_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct t23_meta_node *node = vb2_get_drv_priv(q);
	unsigned long flags;

	spin_lock_irqsave(&node->meta->lock, flags);
	node->streaming = true;
	spin_unlock_irqrestore(&node->meta->lock, flags);
	return 0;
}

static void t23_meta_stop_streaming(struct vb2_queue *q)
{
	struct t23_meta_node *node = vb2_get_drv_priv(q);
	struct t23_meta_buffer *buf, *tmp;
	unsigned long flags;
	LIST_HEAD(done);

	spin_lock_irqsave(&node->meta->lock, flags);
	node->streaming = false;
	/* Without a parameter stream the input returns to driver defaults. */
	if (node->output)
		node->meta->reset[node->input] = true;
	list_splice_init(&node->bufs, &done);
	spin_unlock_irqrestore(&node->meta->lock, flags);

	list_for_each_entry_safe(buf, tmp, &done, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
}

static const struct vb2_ops t23_meta_vb2_ops = {
	.queue_setup = t23_meta_queue_setup,
	.buf_init = t23_meta_buf_init,
	.buf_cleanup = t23_meta_buf_cleanup,
	.buf_prepare = t23_meta_buf_prepare,
	.buf_queue = t23_meta_buf_queue,
	.start_streaming = t23_meta_start_streaming,
	.stop_streaming = t23_meta_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int t23_meta_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	struct t23_meta_node *node = video_drvdata(file);

	strscpy(cap->driver, "tx-isp", sizeof(cap->driver));
	strscpy(cap->card, node->vdev.name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(node->meta->dev));
	return 0;
}

static int t23_meta_enum_fmt(struct file *file, void *fh,
			     struct v4l2_fmtdesc *f)
{
	struct t23_meta_node *node = video_drvdata(file);

	if (f->index)
		return -EINVAL;
	f->pixelformat = node->output ? V4L2_META_FMT_T23_ISP_EXT_PARAMS :
					V4L2_META_FMT_T23_ISP_STATS;
	return 0;
}

static int t23_meta_g_fmt(struct file *file, void *fh, struct v4l2_format *f)
{
	struct t23_meta_node *node = video_drvdata(file);

	memset(&f->fmt.meta, 0, sizeof(f->fmt.meta));
	f->fmt.meta.dataformat = t23_meta_fourcc(node);
	f->fmt.meta.buffersize = t23_meta_size(node);
	return 0;
}

static int t23_meta_try_fmt_out(struct file *file, void *fh,
				struct v4l2_format *f)
{
	memset(&f->fmt.meta, 0, sizeof(f->fmt.meta));
	f->fmt.meta.dataformat = V4L2_META_FMT_T23_ISP_EXT_PARAMS;
	f->fmt.meta.buffersize = t23_meta_size_fmt(V4L2_META_FMT_T23_ISP_EXT_PARAMS);
	return 0;
}

static int t23_meta_s_fmt_out(struct file *file, void *fh,
			      struct v4l2_format *f)
{
	struct t23_meta_node *node = video_drvdata(file);

	if (vb2_is_busy(&node->queue))
		return -EBUSY;
	t23_meta_try_fmt_out(file, fh, f);
	node->dataformat = f->fmt.meta.dataformat;
	return 0;
}

static const struct v4l2_ioctl_ops t23_meta_ioctl_ops = {
	.vidioc_querycap = t23_meta_querycap,
	.vidioc_enum_fmt_meta_cap = t23_meta_enum_fmt,
	.vidioc_g_fmt_meta_cap = t23_meta_g_fmt,
	.vidioc_s_fmt_meta_cap = t23_meta_g_fmt,
	.vidioc_try_fmt_meta_cap = t23_meta_g_fmt,
	.vidioc_enum_fmt_meta_out = t23_meta_enum_fmt,
	.vidioc_g_fmt_meta_out = t23_meta_g_fmt,
	.vidioc_s_fmt_meta_out = t23_meta_s_fmt_out,
	.vidioc_try_fmt_meta_out = t23_meta_try_fmt_out,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations t23_meta_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

static int t23_meta_node_init(struct t23_meta *meta, struct t23_meta_node *node,
			      struct v4l2_device *v4l2_dev, struct mutex *lock,
			      unsigned int input, bool output)
{
	struct vb2_queue *q = &node->queue;
	int ret;

	node->meta = meta;
	node->input = input;
	node->output = output;
	node->dataformat = output ? V4L2_META_FMT_T23_ISP_EXT_PARAMS :
				    V4L2_META_FMT_T23_ISP_STATS;
	INIT_LIST_HEAD(&node->bufs);

	q->type = output ? V4L2_BUF_TYPE_META_OUTPUT : V4L2_BUF_TYPE_META_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = node;
	q->ops = &t23_meta_vb2_ops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->buf_struct_size = sizeof(struct t23_meta_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = lock;
	q->dev = meta->dev;
	ret = vb2_queue_init(q);
	if (ret)
		return ret;
	node->queue_initialized = true;

	node->vdev = (struct video_device){
		.fops = &t23_meta_fops,
		.ioctl_ops = &t23_meta_ioctl_ops,
		.release = video_device_release_empty,
		.v4l2_dev = v4l2_dev,
		.queue = q,
		.lock = lock,
		.vfl_dir = output ? VFL_DIR_TX : VFL_DIR_RX,
		.device_caps = (output ? V4L2_CAP_META_OUTPUT :
					 V4L2_CAP_META_CAPTURE) |
			       V4L2_CAP_STREAMING,
	};
	snprintf(node->vdev.name, sizeof(node->vdev.name), "T23 %s%u",
		 output ? "params" : "stats", input);
	node->vdev.entity.function = MEDIA_ENT_F_IO_V4L;
	node->pad.flags = output ? MEDIA_PAD_FL_SOURCE : MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&node->vdev.entity, 1, &node->pad);
	if (ret)
		return ret;
	node->entity_initialized = true;
	video_set_drvdata(&node->vdev, node);

	ret = video_register_device(&node->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		return ret;
	node->registered = true;
	return 0;
}

int t23_meta_register(struct t23_meta *meta, struct v4l2_device *v4l2_dev,
		      struct mutex *lock, struct media_entity *isp[],
		      unsigned int params_pad, unsigned int stats_pad)
{
	unsigned int i;
	int ret;

	spin_lock_init(&meta->lock);
	for (i = 0; i < T23_META_INPUTS; i++) {
		ret = t23_meta_node_init(meta, &meta->params[i], v4l2_dev, lock,
					 i, true);
		if (ret)
			goto fail;
		ret = media_create_pad_link(&meta->params[i].vdev.entity, 0,
					    isp[i], params_pad,
					    MEDIA_LNK_FL_ENABLED |
					    MEDIA_LNK_FL_IMMUTABLE);
		if (ret)
			goto fail;
		ret = t23_meta_node_init(meta, &meta->stats[i], v4l2_dev, lock,
					 i, false);
		if (ret)
			goto fail;
		ret = media_create_pad_link(isp[i], stats_pad,
					    &meta->stats[i].vdev.entity, 0,
					    MEDIA_LNK_FL_ENABLED |
					    MEDIA_LNK_FL_IMMUTABLE);
		if (ret)
			goto fail;
	}
	return 0;

fail:
	t23_meta_unregister(meta);
	return ret;
}

static void t23_meta_node_unregister(struct t23_meta_node *node)
{
	if (!node->registered)
		return;
	video_unregister_device(&node->vdev);
	node->registered = false;
}

void t23_meta_unregister(struct t23_meta *meta)
{
	unsigned int i;

	for (i = 0; i < T23_META_INPUTS; i++) {
		t23_meta_node_unregister(&meta->params[i]);
		t23_meta_node_unregister(&meta->stats[i]);
	}
}

static void t23_meta_node_release(struct t23_meta_node *node)
{
	if (node->queue_initialized)
		vb2_queue_release(&node->queue);
	if (node->entity_initialized)
		media_entity_cleanup(&node->vdev.entity);
	node->queue_initialized = false;
	node->entity_initialized = false;
}

void t23_meta_release(struct t23_meta *meta)
{
	unsigned int i;

	for (i = 0; i < T23_META_INPUTS; i++) {
		t23_meta_node_release(&meta->params[i]);
		t23_meta_node_release(&meta->stats[i]);
	}
}

static struct t23_meta_buffer *t23_meta_next(struct t23_meta_node *node)
{
	struct t23_meta_buffer *buf;
	unsigned long flags;

	spin_lock_irqsave(&node->meta->lock, flags);
	buf = node->streaming ?
		list_first_entry_or_null(&node->bufs, struct t23_meta_buffer,
					 list) : NULL;
	if (buf)
		list_del(&buf->list);
	spin_unlock_irqrestore(&node->meta->lock, flags);
	return buf;
}

void t23_meta_params_apply(struct t23_meta *meta, unsigned int input,
			   struct tx_isp_subdev *core)
{
	struct t23_meta_buffer *buf;
	unsigned long flags;
	bool reset;

	if (input >= T23_META_INPUTS)
		return;
	spin_lock_irqsave(&meta->lock, flags);
	reset = meta->reset[input];
	meta->reset[input] = false;
	spin_unlock_irqrestore(&meta->lock, flags);
	if (reset)
		tx_isp_core_reset_params(core, input);
	buf = t23_meta_next(&meta->params[input]);
	if (buf) {
		tx_isp_core_queue_ext_params(core, input, buf->cfg,
					     buf->payload);
		buf->vb.vb2_buf.timestamp = ktime_get_ns();
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
	}
	tx_isp_core_apply_params(core, input);
}

/* AE and AWB records pack 21-bit sums and 13-bit counts into 16 bytes. */
static void t23_meta_decode_ae(struct t23_isp_ae_zone *zone, const u32 *w)
{
	zone->sum_low = w[0] & GENMASK(20, 0);
	zone->sum_mid = (FIELD_GET(GENMASK(9, 0), w[1]) << 11) |
			FIELD_GET(GENMASK(31, 21), w[0]);
	zone->sum_high = FIELD_GET(GENMASK(30, 10), w[1]);
	zone->count_low = (FIELD_GET(GENMASK(11, 0), w[2]) << 1) |
			  FIELD_GET(BIT(31), w[1]);
	zone->count_high = FIELD_GET(GENMASK(24, 12), w[2]);
}

static void t23_meta_decode_awb(struct t23_isp_awb_zone *zone, const u32 *w)
{
	zone->sum_r = w[0] & GENMASK(20, 0);
	zone->sum_g = (FIELD_GET(GENMASK(9, 0), w[1]) << 11) |
		      FIELD_GET(GENMASK(31, 21), w[0]);
	zone->sum_b = FIELD_GET(GENMASK(30, 10), w[1]);
	zone->count = (FIELD_GET(BIT(0), w[3]) << 12) |
		      FIELD_GET(GENMASK(31, 20), w[2]);
}

void t23_meta_stats_done(struct t23_meta *meta, unsigned int input,
			 const struct t23_meta_frame *frame)
{
	struct t23_isp_stat_buffer *out;
	struct t23_meta_buffer *buf;
	const u32 *hw = frame->hw;
	unsigned int i;

	if (input >= T23_META_INPUTS)
		return;
	buf = t23_meta_next(&meta->stats[input]);
	if (!buf)
		return;

	out = vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
	memset(out, 0, sizeof(*out));
	out->version = T23_ISP_STATS_VERSION;
	out->size = sizeof(*out);
	out->flags = frame->valid;
	out->sequence = frame->sequence;
	out->zone_width = frame->zone_width;
	out->zone_height = frame->zone_height;
	out->ae_low = frame->ae_low;
	out->ae_high = frame->ae_high;
	dma_rmb();
	if (frame->valid & T23_ISP_STATS_AE)
		for (i = 0; i < T23_ISP_ZONES; i++)
			t23_meta_decode_ae(&out->ae[i], hw + i * 4);
	if (frame->valid & T23_ISP_STATS_HIST)
		for (i = 0; i < 2 * T23_ISP_HIST_BINS; i++)
			out->hist[i / T23_ISP_HIST_BINS][i % T23_ISP_HIST_BINS] =
				hw[0x1000 / 4 + i] & GENMASK(20, 0);
	if (frame->valid & T23_ISP_STATS_AWB)
		for (i = 0; i < T23_ISP_ZONES; i++)
			t23_meta_decode_awb(&out->awb[i], hw + 0x2000 / 4 + i * 4);

	buf->vb.sequence = frame->sequence;
	buf->vb.field = V4L2_FIELD_NONE;
	buf->vb.vb2_buf.timestamp = frame->timestamp;
	vb2_set_plane_payload(&buf->vb.vb2_buf, 0, sizeof(*out));
	vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
}
