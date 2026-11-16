// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang

#include "helix_drv.h"
#include "helix_h264.h"

static const struct helix_fmt helix_formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_NV12,
		.types = V4L2_BUF_TYPE_VIDEO_OUTPUT,
		.num_planes = 1,
		.depth = { 12 },
	},
	{
		.fourcc = V4L2_PIX_FMT_NV21,
		.types = V4L2_BUF_TYPE_VIDEO_OUTPUT,
		.num_planes = 1,
		.depth = { 12 },
	},
	{
		.fourcc = V4L2_PIX_FMT_YUYV,
		.types = V4L2_BUF_TYPE_VIDEO_OUTPUT,
		.num_planes = 1,
		.depth = { 16 },
	},
	{
		.fourcc = V4L2_PIX_FMT_JPEG,
		.types = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.num_planes = 1,
		.depth = { 0 },
	},
	{
		.fourcc = V4L2_PIX_FMT_H264,
		.types = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.num_planes = 1,
		.depth = { 0 },
	},
};

const struct helix_fmt *helix_find_fmt(u32 fourcc, u32 type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(helix_formats); i++) {
		if (helix_formats[i].fourcc == fourcc &&
		    helix_formats[i].types == type)
			return &helix_formats[i];
	}
	return NULL;
}

static int helix_queue_setup(struct vb2_queue *vq,
			     unsigned int *nbuffers, unsigned int *nplanes,
			     unsigned int sizes[], struct device *alloc_devs[])
{
	struct helix_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_format *f = V4L2_TYPE_IS_OUTPUT(vq->type) ?
				&ctx->src_fmt : &ctx->dst_fmt;
	unsigned int min_size = f->fmt.pix.sizeimage;
	unsigned int alloc_size = min_size;

	if (V4L2_TYPE_IS_OUTPUT(vq->type) &&
	    (f->fmt.pix.pixelformat == V4L2_PIX_FMT_NV12 ||
	     f->fmt.pix.pixelformat == V4L2_PIX_FMT_NV21)) {
		u32 cw = ALIGN(f->fmt.pix.width, 16);
		u32 ch = ALIGN(f->fmt.pix.height, 16);

		alloc_size = max_t(unsigned int, min_size, cw * ch * 3 / 2);
	}

	if (*nplanes)
		return *nplanes != 1 || sizes[0] < min_size ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = alloc_size;
	alloc_devs[0] = ctx->hdev->dev;

	return 0;
}

static void helix_queue_eos(struct helix_ctx *ctx)
{
	static const struct v4l2_event eos_ev = {
		.type = V4L2_EVENT_EOS,
	};

	v4l2_event_queue_fh(&ctx->fh, &eos_ev);
}

static int helix_buf_prepare(struct vb2_buffer *vb)
{
	struct helix_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct v4l2_format *f = V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) ?
				&ctx->src_fmt : &ctx->dst_fmt;

	if (READ_ONCE(ctx->hdev->disconnected))
		return -ENODEV;
	if (vb2_plane_size(vb, 0) < f->fmt.pix.sizeimage)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if (vbuf->field == V4L2_FIELD_ANY)
			vbuf->field = V4L2_FIELD_NONE;
		if (vbuf->field != V4L2_FIELD_NONE)
			return -EINVAL;
		if (!vb2_get_plane_payload(vb, 0))
			vb2_set_plane_payload(vb, 0, f->fmt.pix.sizeimage);
		else if (vb2_get_plane_payload(vb, 0) < f->fmt.pix.sizeimage)
			return -EINVAL;
	} else {
		vb2_set_plane_payload(vb, 0, 0);
	}

	return 0;
}

static void helix_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct helix_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	if (V4L2_TYPE_IS_CAPTURE(vb->vb2_queue->type) &&
	    vb2_is_streaming(vb->vb2_queue) &&
	    v4l2_m2m_dst_buf_is_last(ctx->m2m_ctx)) {
		vb2_set_plane_payload(vb, 0, 0);
		vbuf->field = V4L2_FIELD_NONE;
		vbuf->sequence = ctx->sequence++;
		v4l2_m2m_last_buffer_done(ctx->m2m_ctx, vbuf);
		helix_queue_eos(ctx);
		return;
	}

	v4l2_m2m_buf_queue(ctx->m2m_ctx, vbuf);
}

static void helix_return_buffers(struct vb2_queue *vq,
				enum vb2_buffer_state state)
{
	struct helix_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vb;

	for (;;) {
		vb = V4L2_TYPE_IS_OUTPUT(vq->type) ?
			v4l2_m2m_src_buf_remove(ctx->m2m_ctx) :
			v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		if (!vb)
			break;
		v4l2_m2m_buf_done(vb, state);
	}
}

static int helix_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct helix_ctx *ctx = vb2_get_drv_priv(vq);
	struct helix_dev *hdev = ctx->hdev;

	WRITE_ONCE(ctx->stopping, false);
	if (READ_ONCE(hdev->disconnected)) {
		helix_return_buffers(vq, VB2_BUF_STATE_QUEUED);
		return -ENODEV;
	}
	/*
	 * q->lock (dev_mutex) is already held. A failed reset leaves the VPU
	 * owning the previous recon buffers, so retry it before reusing them.
	 */
	if (READ_ONCE(hdev->fatal) || READ_ONCE(hdev->hw_running)) {
		int ret;

		mutex_lock(&hdev->hw_lock);
		ret = helix_hw_quiesce(hdev);
		mutex_unlock(&hdev->hw_lock);
		if (ret) {
			helix_return_buffers(vq, VB2_BUF_STATE_QUEUED);
			return ret;
		}
	}
	if (!ctx->dma_ref) {
		bool need_scratch =
			(ctx->dst_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_H264);
		int ret;

		mutex_lock(&hdev->hw_lock);
		ret = helix_get_dma(hdev, need_scratch);
		mutex_unlock(&hdev->hw_lock);
		if (ret) {
			helix_return_buffers(vq, VB2_BUF_STATE_QUEUED);
			return ret;
		}
		ctx->dma_ref = true;
	}
	v4l2_m2m_update_start_streaming_state(ctx->m2m_ctx, vq);
	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		ctx->frame_num = 0;
		ctx->sequence = 0;
		ctx->idr_pic_id = 0;
		ctx->gop_count = 0;
		helix_rc_reset(&ctx->rc);
	} else if (ctx->dst_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_H264 &&
		   !ctx->h264_bufs_allocated) {
		u32 coded_w = ALIGN(ctx->crop.width, 16);
		u32 coded_h = ALIGN(ctx->crop.height, 16);
		int ret = 0;

		mutex_lock(&hdev->hw_lock);
		if (!hdev->scratch_vaddr) {
			hdev->scratch_vaddr = dma_alloc_coherent(
				hdev->dev, HELIX_SCRATCH_BUF_SIZE,
				&hdev->scratch_paddr, GFP_KERNEL);
			if (!hdev->scratch_vaddr)
				ret = -ENOMEM;
			else
				hdev->scratch_size = HELIX_SCRATCH_BUF_SIZE;
		}
		mutex_unlock(&hdev->hw_lock);
		if (!ret)
			ret = helix_alloc_h264_buffers(hdev, &ctx->h264_bufs,
						       coded_w, coded_h);

		if (ret) {
			struct vb2_queue *src_vq = v4l2_m2m_get_src_vq(ctx->m2m_ctx);

			/* stop_streaming will not run for this queue. */
			if (!vb2_is_streaming(src_vq)) {
				mutex_lock(&hdev->hw_lock);
				helix_put_dma(hdev);
				mutex_unlock(&hdev->hw_lock);
				ctx->dma_ref = false;
			}
			helix_return_buffers(vq, VB2_BUF_STATE_QUEUED);
			return ret;
		}
		ctx->h264_bufs_allocated = true;
		ctx->frame_num = 0;
		ctx->sequence = 0;
		ctx->idr_pic_id = 0;
		ctx->gop_count = 0;
		helix_rc_reset(&ctx->rc);
	}
	return 0;
}

/* hw_lock held. Keep recon/scratch DMA if this context's task may still be running. */
static void helix_release_h264_dma(struct helix_ctx *ctx)
{
	struct helix_dev *hdev = ctx->hdev;

	lockdep_assert_held(&hdev->hw_lock);
	if (!ctx->h264_bufs_allocated)
		return;

	if (READ_ONCE(hdev->hw_running) && hdev->dma_owner == ctx) {
		if (hdev->h264_hold_valid) {
			dev_err(hdev->dev,
				"dropping H.264 DMA tracking; hold slot is full\n");
			WRITE_ONCE(hdev->fatal, true);
		} else {
			hdev->h264_hold = ctx->h264_bufs;
			hdev->h264_hold_valid = true;
			dev_err(hdev->dev,
				"holding H.264 DMA buffers until the VPU resets\n");
		}
		memset(&ctx->h264_bufs, 0, sizeof(ctx->h264_bufs));
		ctx->h264_bufs_allocated = false;
		hdev->dma_owner = NULL;
		return;
	}

	helix_free_h264_buffers(hdev, &ctx->h264_bufs);
	ctx->h264_bufs_allocated = false;
}

static void helix_stop_streaming(struct vb2_queue *vq)
{
	struct helix_ctx *ctx = vb2_get_drv_priv(vq);
	struct helix_dev *hdev = ctx->hdev;

	WRITE_ONCE(ctx->stopping, true);
	if (hdev->workqueue && !READ_ONCE(hdev->disconnected)) {
		mutex_unlock(&hdev->dev_mutex);
		flush_workqueue(hdev->workqueue);
		mutex_lock(&hdev->dev_mutex);
	}
	v4l2_m2m_update_stop_streaming_state(ctx->m2m_ctx, vq);
	if (V4L2_TYPE_IS_OUTPUT(vq->type) && v4l2_m2m_has_stopped(ctx->m2m_ctx))
		helix_queue_eos(ctx);

	mutex_lock(&hdev->hw_lock);
	if (!V4L2_TYPE_IS_OUTPUT(vq->type))
		helix_release_h264_dma(ctx);
	if (ctx->dma_ref) {
		struct vb2_queue *other_vq = V4L2_TYPE_IS_OUTPUT(vq->type) ?
			v4l2_m2m_get_dst_vq(ctx->m2m_ctx) :
			v4l2_m2m_get_src_vq(ctx->m2m_ctx);

		if (!vb2_is_streaming(other_vq)) {
			helix_put_dma(hdev);
			ctx->dma_ref = false;
		}
	}
	mutex_unlock(&hdev->hw_lock);
	helix_return_buffers(vq, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops helix_qops = {
	.queue_setup	= helix_queue_setup,
	.buf_prepare	= helix_buf_prepare,
	.buf_queue	= helix_buf_queue,
	.start_streaming = helix_start_streaming,
	.stop_streaming = helix_stop_streaming,
	.wait_prepare	= vb2_ops_wait_prepare,
	.wait_finish	= vb2_ops_wait_finish,
};

static int helix_queue_init(void *priv, struct vb2_queue *src_vq,
			    struct vb2_queue *dst_vq)
{
	struct helix_ctx *ctx = priv;
	struct helix_dev *hdev = ctx->hdev;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->ops = &helix_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &hdev->dev_mutex;
	src_vq->dev = hdev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->ops = &helix_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &hdev->dev_mutex;
	dst_vq->dev = hdev->dev;

	return vb2_queue_init(dst_vq);
}

static void helix_prepare_input_frame(const u8 *src, u8 *dst,
				      u32 src_w, u32 src_h,
				      u32 vis_w, u32 vis_h,
				      u32 coded_w, u32 coded_h,
				      u32 fourcc)
{
	u32 y, x;

	if (fourcc == V4L2_PIX_FMT_NV12 || fourcc == V4L2_PIX_FMT_NV21) {
		const u8 *src_y = src;
		const u8 *src_uv = src + (size_t)src_w * src_h;
		u8 *dst_y = dst;
		u8 *dst_uv = dst + coded_w * coded_h;
		u32 vis_uv_h = vis_h / 2;
		u32 coded_uv_h = coded_h / 2;

		/* Copy Y plane and replicate edges */
		for (y = 0; y < vis_h; y++) {
			memcpy(dst_y + y * coded_w, src_y + (size_t)y * src_w,
			       vis_w);
			if (coded_w > vis_w)
				memset(dst_y + y * coded_w + vis_w,
				       src_y[(size_t)y * src_w + vis_w - 1],
				       coded_w - vis_w);
		}
		for (y = vis_h; y < coded_h; y++)
			memcpy(dst_y + y * coded_w,
			       dst_y + (vis_h - 1) * coded_w,
			       coded_w);

		/* Copy UV plane and replicate edges */
		for (y = 0; y < vis_uv_h; y++) {
			memcpy(dst_uv + y * coded_w, src_uv + (size_t)y * src_w,
			       vis_w);
			if (coded_w > vis_w) {
				u8 b0 = src_uv[(size_t)y * src_w + vis_w - 2];
				u8 b1 = src_uv[(size_t)y * src_w + vis_w - 1];

				for (x = vis_w; x < coded_w; x += 2) {
					dst_uv[y * coded_w + x] = b0;
					dst_uv[y * coded_w + x + 1] = b1;
				}
			}
		}
		for (y = vis_uv_h; y < coded_uv_h; y++)
			memcpy(dst_uv + y * coded_w,
			       dst_uv + (vis_uv_h - 1) * coded_w,
			       coded_w);
	} else if (fourcc == V4L2_PIX_FMT_YUYV) {
		u8 *dst_y = dst;
		u8 *dst_uv = dst + coded_w * coded_h;
		u32 vis_uv_h = vis_h / 2;
		u32 coded_uv_h = coded_h / 2;

		for (y = 0; y < vis_h; y++) {
			const u8 *src_line = src + (size_t)y * (src_w * 2);
			u8 *dst_line = dst_y + y * coded_w;

			for (x = 0; x < vis_w; x++)
				dst_line[x] = src_line[x * 2];
			if (coded_w > vis_w)
				memset(dst_line + vis_w, dst_line[vis_w - 1], coded_w - vis_w);
		}
		for (y = vis_h; y < coded_h; y++)
			memcpy(dst_y + y * coded_w,
			       dst_y + (vis_h - 1) * coded_w,
			       coded_w);

		for (y = 0; y < vis_uv_h; y++) {
			const u8 *src_line = src + (size_t)(y * 2) * (src_w * 2);
			u8 *dst_line = dst_uv + y * coded_w;

			for (x = 0; x < vis_w; x += 2) {
				dst_line[x] = src_line[x * 2 + 1];     /* U */
				dst_line[x + 1] = src_line[x * 2 + 3]; /* V */
			}
			if (coded_w > vis_w) {
				u8 last_u = dst_line[vis_w - 2];
				u8 last_v = dst_line[vis_w - 1];

				for (x = vis_w; x < coded_w; x += 2) {
					dst_line[x] = last_u;
					dst_line[x + 1] = last_v;
				}
			}
		}
		for (y = vis_uv_h; y < coded_uv_h; y++)
			memcpy(dst_uv + y * coded_w,
			       dst_uv + (vis_uv_h - 1) * coded_w,
			       coded_w);
	}
}

static int helix_encode_h264(struct helix_dev *hdev, struct helix_ctx *ctx,
			     struct vb2_v4l2_buffer *src,
			     struct vb2_v4l2_buffer *dst,
			     dma_addr_t src_y_dma, dma_addr_t src_uv_dma,
			     u32 hw_fourcc,
			     u8 *out, u32 vis_w, u32 vis_h,
			     u32 coded_w, u32 coded_h)
{
	bool is_idr = (ctx->gop_count == 0) || ctx->force_key_frame ||
		      (ctx->gop_size > 0 && (ctx->gop_count % ctx->gop_size == 0));
	u32 qp = is_idr ? ctx->i_frame_qp : ctx->p_frame_qp;
	size_t header_len = 0;
	u32 status, len;
	int ret;

	if (ctx->rc.enabled) {
		u32 window = ctx->gop_size ?
			     ctx->gop_size - ctx->gop_count % ctx->gop_size :
			     helix_rc_fps(&ctx->rc);

		qp = helix_rc_qp(&ctx->rc, is_idr, window, qp,
				 ctx->min_qp, ctx->max_qp);
	}

	ctx->force_key_frame = false;
	if (is_idr) {
		ctx->frame_num = 0;
		if (ctx->repeat_seq_header ||
		    ctx->header_mode == V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME ||
		    ctx->gop_count == 0) {
			ssize_t n;

			n = helix_h264_write_sps(ctx, out + header_len,
						 vb2_plane_size(&dst->vb2_buf, 0) - header_len,
						 vis_w, vis_h, coded_w, coded_h);
			if (n < 0)
				return n;
			header_len += n;

			n = helix_h264_write_pps(ctx, out + header_len,
						 vb2_plane_size(&dst->vb2_buf, 0) - header_len,
						 qp);
			if (n < 0)
				return n;
			header_len += n;
		}
	}

	if (READ_ONCE(hdev->aborting) || READ_ONCE(hdev->disconnected))
		return -ECANCELED;

	reinit_completion(&hdev->completion);
	WRITE_ONCE(hdev->irq_status, 0);
	hdev->bslen = 0;

	ret = helix_h264_hw_start(hdev, ctx,
				  src_y_dma, src_uv_dma,
				  hdev->bounce_paddr,
				  coded_w, coded_h,
				  coded_w, coded_w, hw_fourcc,
				  is_idr, qp);
	if (ret)
		return ret;

	hdev->dma_owner = ctx;
	WRITE_ONCE(hdev->hw_running, true);
	hdev->hw_busy = true;
	ret = helix_vdma_wait(hdev, 2000);
	hdev->hw_busy = false;

	if (ret || READ_ONCE(hdev->aborting) || READ_ONCE(hdev->disconnected)) {
		int reset_ret = helix_hw_quiesce(hdev);

		ctx->force_key_frame = true;
		if (ret && ret != -ECANCELED)
			dev_err(hdev->dev, "H.264 encode failed or timed out: %d, irq_status %#x\n",
				ret, READ_ONCE(hdev->irq_status));
		if (reset_ret)
			dev_err(hdev->dev, "H.264 reset failed: %d; DMA buffers stay reserved\n",
				reset_ret);
		return ret ? ret : -ECANCELED;
	}
	WRITE_ONCE(hdev->hw_running, false);
	hdev->dma_owner = NULL;

	status = READ_ONCE(hdev->irq_status);
	len = hdev->bslen;
	if (!len && (status & SCH_STAT_ENDFLAG)) {
		int retries = 3;

		while (retries-- > 0 && !len) {
			msleep(2);
			len = readl(hdev->base + REG_SDE_CFG9);
		}
		hdev->bslen = len;
	}

	if (READ_ONCE(hdev->aborting) || READ_ONCE(hdev->disconnected) ||
	    !(status & SCH_STAT_ENDFLAG) ||
	    !len || len > hdev->bounce_size ||
	    header_len + len + 64 > vb2_plane_size(&dst->vb2_buf, 0)) {
		ctx->force_key_frame = true;
		return -EIO;
	}

	ctx->h264_bufs.cur_slot = 1 - ctx->h264_bufs.cur_slot;
	dma_rmb();

	{
		ssize_t n = helix_h264_write_slice(ctx, out + header_len,
						   vb2_plane_size(&dst->vb2_buf, 0) - header_len,
						   is_idr, qp, hdev->bounce_vaddr, len);
		if (n < 0) {
			ctx->force_key_frame = true;
			return n;
		}
		header_len += n;
	}
	if (ctx->rc.enabled)
		helix_rc_update(&ctx->rc, is_idr,
				clamp(qp, ctx->min_qp, ctx->max_qp),
				header_len * 8);

	vb2_set_plane_payload(&dst->vb2_buf, 0, header_len);
	dst->vb2_buf.timestamp = src->vb2_buf.timestamp;
	dst->timecode = src->timecode;
	dst->flags &= ~(V4L2_BUF_FLAG_TSTAMP_SRC_MASK | V4L2_BUF_FLAG_TIMECODE);
	dst->flags |= src->flags & (V4L2_BUF_FLAG_TSTAMP_SRC_MASK | V4L2_BUF_FLAG_TIMECODE);
	src->sequence = ctx->sequence;
	dst->sequence = ctx->sequence++;
	ctx->frame_num++;
	ctx->gop_count++;
	dst->field = V4L2_FIELD_NONE;
	dst->flags &= ~(V4L2_BUF_FLAG_KEYFRAME | V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME);
	if (is_idr)
		dst->flags |= V4L2_BUF_FLAG_KEYFRAME;
	else
		dst->flags |= V4L2_BUF_FLAG_PFRAME;

	return 0;
}

static int helix_encode_jpeg(struct helix_dev *hdev, struct helix_ctx *ctx,
			     struct vb2_v4l2_buffer *src,
			     struct vb2_v4l2_buffer *dst,
			     dma_addr_t src_y_dma, dma_addr_t src_uv_dma,
			     u32 hw_fourcc,
			     u8 *out, u32 vis_w, u32 vis_h,
			     u32 coded_w, u32 coded_h)
{
	size_t header;
	u32 status, len;
	int ret;

	helix_calc_qtable(ctx->jpeg_quality, ctx->qtable, ctx->qmem);
	header = helix_jpeg_write_headers(out, coded_w, coded_h, ctx->qtable, vis_w, vis_h);

	if (READ_ONCE(hdev->aborting) || READ_ONCE(hdev->disconnected))
		return -ECANCELED;

	reinit_completion(&hdev->completion);
	WRITE_ONCE(hdev->irq_status, 0);
	hdev->bslen = 0;

	ret = helix_jpeg_hw_start(hdev, src_y_dma, src_uv_dma,
				  hdev->bounce_paddr, coded_w, coded_h,
				  coded_w, coded_w,
				  hw_fourcc,
				  ctx->qmem);
	if (ret)
		return ret;

	WRITE_ONCE(hdev->hw_running, true);
	hdev->hw_busy = true;
	ret = helix_vdma_wait(hdev, 2000);
	hdev->hw_busy = false;

	if (ret || READ_ONCE(hdev->aborting) || READ_ONCE(hdev->disconnected)) {
		int reset_ret = helix_hw_quiesce(hdev);

		if (ret && ret != -ECANCELED)
			dev_err(hdev->dev, "encoding failed or timed out: %d, irq_status %#x\n",
				ret, READ_ONCE(hdev->irq_status));
		if (reset_ret)
			dev_err(hdev->dev, "JPEG reset failed: %d\n", reset_ret);
		return ret ? ret : -ECANCELED;
	}
	WRITE_ONCE(hdev->hw_running, false);
	hdev->dma_owner = NULL;

	status = READ_ONCE(hdev->irq_status);
	len = hdev->bslen;
	if (!len && (status & (SCH_STAT_ENDFLAG | SCH_STAT_JPGEND))) {
		int retries = 3;

		while (retries-- > 0 && !len) {
			msleep(2);
			len = readl(hdev->base + REG_JPGC_STAT) & JPGC_STAT_LEN_MASK;
		}
		hdev->bslen = len;
	}

	if (READ_ONCE(hdev->aborting) || READ_ONCE(hdev->disconnected) ||
	    (status & (SCH_STAT_ENDFLAG | SCH_STAT_JPGEND)) !=
	    (SCH_STAT_ENDFLAG | SCH_STAT_JPGEND) ||
	    !len || len > hdev->bounce_size ||
	    header + len + 2 > vb2_plane_size(&dst->vb2_buf, 0)) {
		return -EIO;
	}

	dma_rmb();

	memcpy(out + header, hdev->bounce_vaddr, len);

	if (len < 2 || out[header + len - 2] != 0xff ||
	    out[header + len - 1] != 0xd9) {
		helix_jpeg_write_eoi(out, header + len);
		len += 2;
	}

	vb2_set_plane_payload(&dst->vb2_buf, 0, header + len);
	dst->vb2_buf.timestamp = src->vb2_buf.timestamp;
	dst->timecode = src->timecode;
	dst->flags &= ~(V4L2_BUF_FLAG_TSTAMP_SRC_MASK | V4L2_BUF_FLAG_TIMECODE);
	dst->flags |= src->flags & (V4L2_BUF_FLAG_TSTAMP_SRC_MASK | V4L2_BUF_FLAG_TIMECODE);
	src->sequence = ctx->sequence;
	dst->sequence = ctx->sequence++;
	ctx->frame_num++;
	dst->field = V4L2_FIELD_NONE;
	dst->flags &= ~(V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME);
	dst->flags |= V4L2_BUF_FLAG_KEYFRAME;

	return 0;
}

void helix_job_work(struct work_struct *work)
{
	struct helix_dev *hdev = container_of(work, struct helix_dev, work);
	struct helix_ctx *ctx;
	struct vb2_v4l2_buffer *src, *dst;
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	u32 src_w, src_h, vis_w, vis_h, coded_w, coded_h, src_fourcc, hw_fourcc;
	dma_addr_t src_y_dma, src_uv_dma;
	size_t zc_min_bytes;
	bool zero_copy;
	u8 *out;
	int ret;

	mutex_lock(&hdev->hw_lock);
	ctx = v4l2_m2m_get_curr_priv(hdev->m2m_dev);
	if (!ctx) {
		mutex_unlock(&hdev->hw_lock);
		return;
	}
	WRITE_ONCE(hdev->curr_ctx, ctx);

	src = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	if (!src || !dst || READ_ONCE(hdev->disconnected) ||
	    READ_ONCE(hdev->aborting))
		goto unlock;

	out = vb2_plane_vaddr(&dst->vb2_buf, 0);
	if (!out)
		goto unlock;

	src_w = ctx->src_fmt.fmt.pix.width;
	src_h = ctx->src_fmt.fmt.pix.height;
	vis_w = ctx->crop.width;
	vis_h = ctx->crop.height;
	coded_w = ALIGN(vis_w, 16);
	coded_h = ALIGN(vis_h, 16);
	src_fourcc = ctx->src_fmt.fmt.pix.pixelformat;
	hw_fourcc = (src_fourcc == V4L2_PIX_FMT_YUYV) ?
		    V4L2_PIX_FMT_NV12 : src_fourcc;

	src_y_dma = vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0);
	src_uv_dma = src_y_dma + (size_t)coded_w * vis_h;
	zc_min_bytes = (size_t)coded_w * vis_h + (size_t)coded_w * (coded_h / 2);
	zero_copy = (src_fourcc == V4L2_PIX_FMT_NV12 ||
		     src_fourcc == V4L2_PIX_FMT_NV21) &&
		    vis_w == src_w && vis_h == src_h &&
		    vis_w == coded_w && vis_h == coded_h &&
		    !((src_y_dma | src_uv_dma) & 127) &&
		    vb2_plane_size(&src->vb2_buf, 0) >= zc_min_bytes;

	if (!zero_copy) {
		u8 *src_vaddr = vb2_plane_vaddr(&src->vb2_buf, 0);

		if (!src_vaddr)
			goto unlock;
		if (helix_ensure_input_dma(hdev, (size_t)coded_w * coded_h * 3 / 2))
			goto unlock;
		helix_prepare_input_frame(src_vaddr, hdev->input_vaddr,
					  src_w, src_h, vis_w, vis_h,
					  coded_w, coded_h, src_fourcc);
		src_y_dma = hdev->input_paddr;
		src_uv_dma = hdev->input_paddr + (size_t)coded_w * coded_h;
	}
	dma_wmb();

	helix_cp0_config7_enable();
	if (helix_hw_quiesce(hdev))
		goto unlock;

	if (ctx->dst_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_H264)
		ret = helix_encode_h264(hdev, ctx, src, dst,
					src_y_dma, src_uv_dma, hw_fourcc, out,
					vis_w, vis_h, coded_w, coded_h);
	else
		ret = helix_encode_jpeg(hdev, ctx, src, dst,
					src_y_dma, src_uv_dma, hw_fourcc, out,
					vis_w, vis_h, coded_w, coded_h);

	if (!ret)
		state = VB2_BUF_STATE_DONE;
	if (src && v4l2_m2m_is_last_draining_src_buf(ctx->m2m_ctx, src)) {
		if (dst)
			dst->flags |= V4L2_BUF_FLAG_LAST;
		v4l2_m2m_mark_stopped(ctx->m2m_ctx);
		helix_queue_eos(ctx);
	}

unlock:
	if (src)
		v4l2_m2m_buf_done(src, state);
	if (dst) {
		if (state != VB2_BUF_STATE_DONE)
			vb2_set_plane_payload(&dst->vb2_buf, 0, 0);
		v4l2_m2m_buf_done(dst, state);
	}
	WRITE_ONCE(hdev->curr_ctx, NULL);
	mutex_unlock(&hdev->hw_lock);
	v4l2_m2m_job_finish(hdev->m2m_dev, ctx->m2m_ctx);
}

static void helix_device_run(void *priv)
{
	struct helix_ctx *ctx = priv;
	struct helix_dev *hdev = ctx->hdev;

	if (READ_ONCE(hdev->disconnected)) {
		helix_return_buffers(v4l2_m2m_get_src_vq(ctx->m2m_ctx),
				     VB2_BUF_STATE_ERROR);
		helix_return_buffers(v4l2_m2m_get_dst_vq(ctx->m2m_ctx),
				     VB2_BUF_STATE_ERROR);
		v4l2_m2m_job_finish(hdev->m2m_dev, ctx->m2m_ctx);
		return;
	}
	WRITE_ONCE(hdev->aborting, false);
	queue_work(hdev->workqueue, &hdev->work);
}

static void helix_job_abort(void *priv)
{
	struct helix_ctx *ctx = priv;
	struct helix_dev *hdev = ctx->hdev;

	if (READ_ONCE(hdev->curr_ctx) == ctx) {
		WRITE_ONCE(hdev->aborting, true);
		complete(&hdev->completion);
	}
	if (hdev->workqueue)
		flush_workqueue(hdev->workqueue);
}

static int helix_job_ready(void *priv)
{
	struct helix_ctx *ctx = priv;

	return !READ_ONCE(ctx->stopping) &&
		!READ_ONCE(ctx->hdev->disconnected);
}

const struct v4l2_m2m_ops helix_m2m_ops = {
	.device_run	= helix_device_run,
	.job_abort	= helix_job_abort,
	.job_ready	= helix_job_ready,
};

static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	struct helix_ctx *ctx = priv;
	struct helix_dev *hdev = ctx->hdev;

	strscpy(cap->driver, HELIX_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Ingenic Helix Video Encoder", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(hdev->dev));
	cap->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int vidioc_enum_fmt_vid_out(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	int idx = 0, i;

	for (i = 0; i < ARRAY_SIZE(helix_formats); i++) {
		if (helix_formats[i].types == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			if (idx == f->index) {
				f->pixelformat = helix_formats[i].fourcc;
				return 0;
			}
			idx++;
		}
	}
	return -EINVAL;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	int idx = 0, i;

	for (i = 0; i < ARRAY_SIZE(helix_formats); i++) {
		if (helix_formats[i].types == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			if (idx == f->index) {
				f->pixelformat = helix_formats[i].fourcc;
				return 0;
			}
			idx++;
		}
	}
	return -EINVAL;
}

static int vidioc_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct helix_ctx *ctx = priv;
	struct vb2_queue *vq;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		*f = ctx->src_fmt;
	else
		*f = ctx->dst_fmt;

	return 0;
}

static int helix_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct helix_ctx *ctx = container_of(ctrl->handler, struct helix_ctx, hdl);

	switch (ctrl->id) {
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		ctx->jpeg_quality = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		ctx->gop_size = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		ctx->i_frame_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
		ctx->p_frame_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		ctx->min_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		ctx->max_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		ctx->h264_profile = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		ctx->h264_level = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEADER_MODE:
		ctx->header_mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
		ctx->force_key_frame = true;
		break;
	case V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER:
		ctx->repeat_seq_header = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		ctx->rc.enabled = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		ctx->rc.mode = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		ctx->rc.bitrate = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_PEAK:
		ctx->rc.peak = ctrl->val;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct v4l2_ctrl_ops helix_ctrl_ops = {
	.s_ctrl = helix_s_ctrl,
};

/* The frame rate is the rate control's time base; it does not pace jobs. */
static int vidioc_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *a)
{
	struct helix_ctx *ctx = priv;

	if (a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	memset(&a->parm.output, 0, sizeof(a->parm.output));
	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.output.timeperframe.numerator = ctx->rc.tpf_num;
	a->parm.output.timeperframe.denominator = ctx->rc.tpf_den;
	return 0;
}

static int vidioc_s_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *a)
{
	struct helix_ctx *ctx = priv;
	struct v4l2_fract *tpf = &a->parm.output.timeperframe;

	if (a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	/* 1 to 120 frames per second */
	if (tpf->numerator && tpf->denominator &&
	    tpf->denominator <= 120 * tpf->numerator &&
	    tpf->numerator <= tpf->denominator) {
		/* A job reads the pair under hw_lock. */
		mutex_lock(&ctx->hdev->hw_lock);
		ctx->rc.tpf_num = tpf->numerator;
		ctx->rc.tpf_den = tpf->denominator;
		mutex_unlock(&ctx->hdev->hw_lock);
	}
	return vidioc_g_parm(file, priv, a);
}

static int vidioc_enum_framesizes(struct file *file, void *priv,
				  struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index)
		return -EINVAL;
	if (!helix_find_fmt(fsize->pixel_format, V4L2_BUF_TYPE_VIDEO_CAPTURE) &&
	    !helix_find_fmt(fsize->pixel_format, V4L2_BUF_TYPE_VIDEO_OUTPUT))
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = HELIX_MIN_W;
	fsize->stepwise.max_width = HELIX_MAX_W;
	fsize->stepwise.step_width = 2;
	fsize->stepwise.min_height = HELIX_MIN_H;
	fsize->stepwise.max_height = HELIX_MAX_H;
	fsize->stepwise.step_height = 2;
	return 0;
}

static int vidioc_g_selection(struct file *file, void *priv,
			      struct v4l2_selection *s)
{
	struct helix_ctx *ctx = priv;

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		s->r = ctx->crop;
		break;
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->src_fmt.fmt.pix.width;
		s->r.height = ctx->src_fmt.fmt.pix.height;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static u32 helix_calc_dst_sizeimage(u32 fourcc, u32 width, u32 height,
				    u32 requested)
{
	u32 raw = ALIGN(width, 16) * ALIGN(height, 16) * 3 / 2;
	u32 min_sz, max_sz;

	if (fourcc == V4L2_PIX_FMT_H264) {
		max_sz = HELIX_BOUNCE_BUF_SIZE;
		min_sz = clamp_val(PAGE_ALIGN(raw / 2), 128U * 1024U, max_sz);
	} else {
		max_sz = HELIX_BOUNCE_BUF_SIZE + 4096U;
		min_sz = clamp_val(PAGE_ALIGN(raw / 2 + 4096U), 128U * 1024U,
				   max_sz);
	}
	if (requested > min_sz)
		return PAGE_ALIGN(min(requested, max_sz));
	return min_sz;
}

static int vidioc_s_selection(struct file *file, void *priv,
			      struct v4l2_selection *s)
{
	struct helix_ctx *ctx = priv;

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	if (s->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	if (vb2_is_streaming(v4l2_m2m_get_src_vq(ctx->m2m_ctx)) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->m2m_ctx)))
		return -EBUSY;

	s->r.left = 0;
	s->r.top = 0;
	s->r.width = clamp_val(s->r.width, HELIX_MIN_W, ctx->src_fmt.fmt.pix.width);
	s->r.height = clamp_val(s->r.height, HELIX_MIN_H, ctx->src_fmt.fmt.pix.height);
	s->r.width = ALIGN_DOWN(s->r.width, 2);
	s->r.height = ALIGN_DOWN(s->r.height, 2);

	ctx->crop = s->r;
	ctx->dst_fmt.fmt.pix.width = s->r.width;
	ctx->dst_fmt.fmt.pix.height = s->r.height;
	ctx->dst_fmt.fmt.pix.sizeimage = helix_calc_dst_sizeimage(
		ctx->dst_fmt.fmt.pix.pixelformat, s->r.width, s->r.height, 0);
	return 0;
}

static void helix_sanitize_colorspace(u32 fourcc, u32 *colorspace,
				      u32 *xfer_func, u32 *ycbcr_enc,
				      u32 *quantization)
{
	if (*colorspace == V4L2_COLORSPACE_DEFAULT ||
	    *colorspace >= V4L2_COLORSPACE_LAST ||
	    *colorspace == V4L2_COLORSPACE_BT878 ||
	    (fourcc != V4L2_PIX_FMT_JPEG &&
	     *colorspace == V4L2_COLORSPACE_JPEG))
		*colorspace = V4L2_COLORSPACE_SRGB;
	if (*xfer_func == V4L2_XFER_FUNC_DEFAULT ||
	    *xfer_func >= V4L2_XFER_FUNC_LAST)
		*xfer_func = V4L2_XFER_FUNC_SRGB;
	if (*ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT ||
	    *ycbcr_enc > V4L2_YCBCR_ENC_SMPTE240M)
		*ycbcr_enc = V4L2_YCBCR_ENC_601;
	if (*quantization == V4L2_QUANTIZATION_DEFAULT ||
	    *quantization > V4L2_QUANTIZATION_LIM_RANGE)
		*quantization = V4L2_QUANTIZATION_FULL_RANGE;
}

static int vidioc_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct helix_ctx *ctx = priv;
	const struct helix_fmt *fmt;
	u32 req_size = f->fmt.pix.sizeimage;
	u32 colorspace = f->fmt.pix.colorspace;
	u32 xfer_func = f->fmt.pix.xfer_func;
	u32 ycbcr_enc = f->fmt.pix.ycbcr_enc;
	u32 quantization = f->fmt.pix.quantization;

	fmt = helix_find_fmt(f->fmt.pix.pixelformat, f->type);
	if (!fmt) {
		if (V4L2_TYPE_IS_OUTPUT(f->type))
			fmt = &helix_formats[0];
		else
			fmt = &helix_formats[3];
		f->fmt.pix.pixelformat = fmt->fourcc;
	}

	f->fmt.pix.width = clamp_val(f->fmt.pix.width, HELIX_MIN_W, HELIX_MAX_W);
	f->fmt.pix.height = clamp_val(f->fmt.pix.height, HELIX_MIN_H, HELIX_MAX_H);
	f->fmt.pix.field = V4L2_FIELD_NONE;
	f->fmt.pix.flags = 0;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		u32 w = ALIGN_DOWN(f->fmt.pix.width, 2);
		u32 h = ALIGN_DOWN(f->fmt.pix.height, 2);

		helix_sanitize_colorspace(f->fmt.pix.pixelformat, &colorspace,
					  &xfer_func, &ycbcr_enc,
					  &quantization);
		f->fmt.pix.colorspace = colorspace;
		f->fmt.pix.xfer_func = xfer_func;
		f->fmt.pix.ycbcr_enc = ycbcr_enc;
		f->fmt.pix.quantization = quantization;
		f->fmt.pix.width = w;
		f->fmt.pix.height = h;
		if (f->fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
			f->fmt.pix.bytesperline = w * 2;
			f->fmt.pix.sizeimage = w * h * 2;
		} else {
			f->fmt.pix.bytesperline = w;
			f->fmt.pix.sizeimage = w * h * 3 / 2;
		}
	} else {
		f->fmt.pix.colorspace = ctx->src_fmt.fmt.pix.colorspace;
		f->fmt.pix.xfer_func = ctx->src_fmt.fmt.pix.xfer_func;
		f->fmt.pix.ycbcr_enc = ctx->src_fmt.fmt.pix.ycbcr_enc;
		f->fmt.pix.quantization = ctx->src_fmt.fmt.pix.quantization;
		f->fmt.pix.width = ctx->crop.width;
		f->fmt.pix.height = ctx->crop.height;
		f->fmt.pix.bytesperline = 0;
		f->fmt.pix.sizeimage = helix_calc_dst_sizeimage(
			f->fmt.pix.pixelformat, f->fmt.pix.width,
			f->fmt.pix.height, req_size);
	}

	return 0;
}

static int vidioc_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct helix_ctx *ctx = priv;
	struct vb2_queue *vq;
	int ret;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq || vb2_is_busy(vq))
		return -EBUSY;
	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
	    vb2_is_busy(v4l2_m2m_get_vq(ctx->m2m_ctx,
				      V4L2_BUF_TYPE_VIDEO_CAPTURE)))
		return -EBUSY;

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		ctx->src_fmt = *f;
		ctx->crop.left = 0;
		ctx->crop.top = 0;
		ctx->crop.width = f->fmt.pix.width;
		ctx->crop.height = f->fmt.pix.height;

		ctx->dst_fmt.fmt.pix.width = f->fmt.pix.width;
		ctx->dst_fmt.fmt.pix.height = f->fmt.pix.height;
		ctx->dst_fmt.fmt.pix.colorspace = f->fmt.pix.colorspace;
		ctx->dst_fmt.fmt.pix.ycbcr_enc = f->fmt.pix.ycbcr_enc;
		ctx->dst_fmt.fmt.pix.quantization = f->fmt.pix.quantization;
		ctx->dst_fmt.fmt.pix.xfer_func = f->fmt.pix.xfer_func;
		ctx->dst_fmt.fmt.pix.sizeimage = helix_calc_dst_sizeimage(
			ctx->dst_fmt.fmt.pix.pixelformat, f->fmt.pix.width,
			f->fmt.pix.height, 0);
	} else {
		ctx->dst_fmt = *f;
	}

	return 0;
}

static int helix_encoder_cmd(struct file *file, void *priv,
			     struct v4l2_encoder_cmd *ec)
{
	struct helix_ctx *ctx = priv;
	int ret;

	ret = v4l2_m2m_ioctl_try_encoder_cmd(file, priv, ec);
	if (ret < 0)
		return ret;

	if (ec->cmd == V4L2_ENC_CMD_STOP && ctx->hdev->workqueue) {
		/*
		 * Flush any already-popped frame on the workqueue before
		 * inspecting ready queues for V4L2_BUF_FLAG_LAST.
		 */
		mutex_unlock(&ctx->hdev->dev_mutex);
		flush_workqueue(ctx->hdev->workqueue);
		mutex_lock(&ctx->hdev->dev_mutex);
		if (READ_ONCE(ctx->hdev->disconnected))
			return -ENODEV;
	}

	ret = v4l2_m2m_encoder_cmd(file, ctx->m2m_ctx, ec);
	if (ret < 0)
		return ret;

	if (ec->cmd == V4L2_ENC_CMD_STOP && v4l2_m2m_has_stopped(ctx->m2m_ctx))
		helix_queue_eos(ctx);

	return 0;
}

static int helix_subscribe_event(struct v4l2_fh *fh,
				 const struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_EOS:
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	default:
		return v4l2_ctrl_subscribe_event(fh, sub);
	}
}

const struct v4l2_ioctl_ops helix_v4l2_ioctl_ops = {
	.vidioc_querycap	= vidioc_querycap,

	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap	= vidioc_g_fmt,
	.vidioc_try_fmt_vid_cap	= vidioc_try_fmt,
	.vidioc_s_fmt_vid_cap	= vidioc_s_fmt,

	.vidioc_enum_fmt_vid_out = vidioc_enum_fmt_vid_out,
	.vidioc_g_fmt_vid_out	= vidioc_g_fmt,
	.vidioc_try_fmt_vid_out	= vidioc_try_fmt,
	.vidioc_s_fmt_vid_out	= vidioc_s_fmt,

	.vidioc_g_selection	= vidioc_g_selection,
	.vidioc_s_selection	= vidioc_s_selection,

	.vidioc_g_parm		= vidioc_g_parm,
	.vidioc_s_parm		= vidioc_s_parm,
	.vidioc_enum_framesizes	= vidioc_enum_framesizes,

	.vidioc_reqbufs		= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf	= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf		= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf		= v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf		= v4l2_m2m_ioctl_expbuf,
	.vidioc_prepare_buf	= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs	= v4l2_m2m_ioctl_create_bufs,

	.vidioc_streamon	= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff	= v4l2_m2m_ioctl_streamoff,

	.vidioc_encoder_cmd	= helix_encoder_cmd,
	.vidioc_try_encoder_cmd	= v4l2_m2m_ioctl_try_encoder_cmd,

	.vidioc_subscribe_event	= helix_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int helix_v4l2_open(struct file *file)
{
	struct helix_dev *hdev = video_drvdata(file);
	struct helix_ctx *ctx;
	int ret;

	if (mutex_lock_interruptible(&hdev->dev_mutex))
		return -ERESTARTSYS;

	if (hdev->disconnected) {
		mutex_unlock(&hdev->dev_mutex);
		return -ENODEV;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		mutex_unlock(&hdev->dev_mutex);
		return -ENOMEM;
	}

	BUILD_BUG_ON(offsetof(struct helix_ctx, fh) != 0);
	ctx->hdev = hdev;
	file->private_data = &ctx->fh;

	ret = helix_v4l2_init_ctx(ctx);
	if (ret) {
		kfree(ctx);
		mutex_unlock(&hdev->dev_mutex);
		return ret;
	}
	if (list_empty(&hdev->contexts)) {
		helix_cp0_config7_enable();
		mutex_lock(&hdev->hw_lock);
		ret = helix_hw_quiesce(hdev);
		mutex_unlock(&hdev->hw_lock);
		if (ret) {
			helix_cp0_config7_disable();
			helix_v4l2_cleanup_ctx(ctx);
			kfree(ctx);
			mutex_unlock(&hdev->dev_mutex);
			return ret;
		}
	}
	list_add_tail(&ctx->list, &hdev->contexts);

	mutex_unlock(&hdev->dev_mutex);
	return 0;
}

static int helix_v4l2_release(struct file *file)
{
	struct helix_ctx *ctx = file->private_data;
	struct helix_dev *hdev = ctx->hdev;

	WRITE_ONCE(ctx->stopping, true);
	helix_job_abort(ctx);

	mutex_lock(&hdev->dev_mutex);
	list_del(&ctx->list);
	if (list_empty(&hdev->contexts))
		helix_cp0_config7_disable();
	mutex_unlock(&hdev->dev_mutex);

	helix_v4l2_cleanup_ctx(ctx);
	kfree(ctx);

	return 0;
}

const struct v4l2_file_operations helix_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= helix_v4l2_open,
	.release	= helix_v4l2_release,
	.poll		= v4l2_m2m_fop_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= v4l2_m2m_fop_mmap,
};

int helix_v4l2_init_ctx(struct helix_ctx *ctx)
{
	struct helix_dev *hdev = ctx->hdev;
	int ret;

	v4l2_fh_init(&ctx->fh, &hdev->vfd);
	v4l2_fh_add(&ctx->fh);

	ctx->m2m_ctx = v4l2_m2m_ctx_init(hdev->m2m_dev, ctx, helix_queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = PTR_ERR(ctx->m2m_ctx);
		v4l2_fh_del(&ctx->fh);
		v4l2_fh_exit(&ctx->fh);
		return ret;
	}
	ctx->fh.m2m_ctx = ctx->m2m_ctx;

	ctx->jpeg_quality = 75;
	helix_calc_qtable(ctx->jpeg_quality, ctx->qtable, ctx->qmem);
	ctx->gop_size = 30;
	ctx->i_frame_qp = 26;
	ctx->p_frame_qp = 28;
	ctx->min_qp = 10;
	ctx->max_qp = 51;
	ctx->h264_profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH;
	ctx->h264_level = V4L2_MPEG_VIDEO_H264_LEVEL_4_0;
	ctx->header_mode = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME;
	ctx->force_key_frame = false;
	ctx->repeat_seq_header = false;
	ctx->frame_num = 0;
	ctx->sequence = 0;
	ctx->idr_pic_id = 0;
	ctx->gop_count = 0;
	ctx->h264_bufs_allocated = false;
	helix_rc_init(&ctx->rc);

	v4l2_ctrl_handler_init(&ctx->hdl, 16);
	v4l2_ctrl_new_std(&ctx->hdl, NULL,
			  V4L2_CID_MIN_BUFFERS_FOR_OUTPUT, 1, 1, 1, 1);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_JPEG_COMPRESSION_QUALITY, 1, 100, 1, 75);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_GOP_SIZE, 0, 1000, 1, 30);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP, 0, 51, 1, 26);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP, 0, 51, 1, 28);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 0, 51, 1, 10);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 0, 51, 1, 51);
	v4l2_ctrl_new_std_menu(&ctx->hdl, &helix_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_PROFILE,
			       V4L2_MPEG_VIDEO_H264_PROFILE_HIGH,
			       ~(1U << V4L2_MPEG_VIDEO_H264_PROFILE_HIGH),
			       V4L2_MPEG_VIDEO_H264_PROFILE_HIGH);
	v4l2_ctrl_new_std_menu(&ctx->hdl, &helix_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_LEVEL,
			       V4L2_MPEG_VIDEO_H264_LEVEL_4_0,
			       ~((1 << V4L2_MPEG_VIDEO_H264_LEVEL_3_0) |
				 (1 << V4L2_MPEG_VIDEO_H264_LEVEL_3_1) |
				 (1 << V4L2_MPEG_VIDEO_H264_LEVEL_4_0)),
			       V4L2_MPEG_VIDEO_H264_LEVEL_4_0);
	v4l2_ctrl_new_std_menu(&ctx->hdl, &helix_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEADER_MODE,
			       V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME,
			       ~(1U << V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME),
			       V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME, 0, 0, 0, 0);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(&ctx->hdl, &helix_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_CBR, 0,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_BITRATE,
			  16000, 20000000, 1000, 2000000);
	v4l2_ctrl_new_std(&ctx->hdl, &helix_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_BITRATE_PEAK,
			  16000, 20000000, 1000, 4000000);

	if (ctx->hdl.error) {
		ret = ctx->hdl.error;
		v4l2_ctrl_handler_free(&ctx->hdl);
		v4l2_m2m_ctx_release(ctx->m2m_ctx);
		v4l2_fh_del(&ctx->fh);
		v4l2_fh_exit(&ctx->fh);
		return ret;
	}

	ctx->fh.ctrl_handler = &ctx->hdl;
	v4l2_ctrl_handler_setup(&ctx->hdl);

	ctx->crop.left = 0;
	ctx->crop.top = 0;
	ctx->crop.width = HELIX_DEF_W;
	ctx->crop.height = HELIX_DEF_H;

	/* Dimensions must contain complete 16x16 MCUs. */
	ctx->src_fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ctx->src_fmt.fmt.pix.width = HELIX_DEF_W;
	ctx->src_fmt.fmt.pix.height = HELIX_DEF_H;
	ctx->src_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	ctx->src_fmt.fmt.pix.field = V4L2_FIELD_NONE;
	ctx->src_fmt.fmt.pix.bytesperline = HELIX_DEF_W;
	ctx->src_fmt.fmt.pix.sizeimage = HELIX_DEF_W * HELIX_DEF_H * 3 / 2;
	ctx->src_fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	ctx->src_fmt.fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_601;
	ctx->src_fmt.fmt.pix.quantization = V4L2_QUANTIZATION_FULL_RANGE;
	ctx->src_fmt.fmt.pix.xfer_func = V4L2_XFER_FUNC_SRGB;

	ctx->dst_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ctx->dst_fmt.fmt.pix.width = HELIX_DEF_W;
	ctx->dst_fmt.fmt.pix.height = HELIX_DEF_H;
	ctx->dst_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	ctx->dst_fmt.fmt.pix.field = V4L2_FIELD_NONE;
	ctx->dst_fmt.fmt.pix.bytesperline = 0;
	ctx->dst_fmt.fmt.pix.sizeimage = helix_calc_dst_sizeimage(
		V4L2_PIX_FMT_JPEG, HELIX_DEF_W, HELIX_DEF_H, 0);
	ctx->dst_fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	ctx->dst_fmt.fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_601;
	ctx->dst_fmt.fmt.pix.quantization = V4L2_QUANTIZATION_FULL_RANGE;
	ctx->dst_fmt.fmt.pix.xfer_func = V4L2_XFER_FUNC_SRGB;

	return 0;
}

void helix_v4l2_cleanup_ctx(struct helix_ctx *ctx)
{
	mutex_lock(&ctx->hdev->hw_lock);
	helix_release_h264_dma(ctx);
	mutex_unlock(&ctx->hdev->hw_lock);
	v4l2_ctrl_handler_free(&ctx->hdl);
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
}
