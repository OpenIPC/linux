// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 Video Processor Subsystem - VIC capture
 *
 * Copyright (C) 2017-2023 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/media-bus-format.h>
#include <linux/overflow.h>
#include <linux/property.h>
#include <linux/media/ingenic/t23-isp-config.h>
#include <linux/workqueue.h>
#include <media/media-devnode.h>
#include <media/v4l2-async.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>

#include "tx-isp-device.h"
#include "tx-isp-meta.h"
#include "tx-isp-motion.h"
#include "tx-isp-overlay.h"

#define T23_MAX_INPUTS 2
#define T23_CAPTURE_CHANNELS 2

struct t23_isp_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

struct t23_async_subdev {
	struct v4l2_async_connection asc;
	unsigned int port;
	unsigned int index;
};

struct t23_video_input {
	struct v4l2_subdev *sd;
	unsigned int pad;
	u32 mbus_code;
	bool initialized;
};

struct t23_isp_video;

/* Downscaled NV12 copy of an input's processed frames, from MSCA channels 1-2 */
struct t23_sub_node {
	unsigned int index;
	struct video_device vdev;
	struct media_pad pad;
	struct vb2_queue queue;
	struct list_head bufs;
	struct v4l2_pix_format pix;
	bool streaming;
	bool stopping;
	bool queue_initialized;
	bool entity_initialized;
	bool vdev_registered;
	unsigned int skip_frames;
};

struct t23_video_node {
	struct t23_isp_video *video;
	unsigned int channel_idx;
	struct video_device vdev;
	struct media_pad pad;
	struct v4l2_subdev isp_sd;
	/*
	 * 0: sensor, 1: video output, 2: parameters, 3: statistics,
	 * 4..5: scaled outputs, 6 onwards: OSD regions
	 */
	struct media_pad isp_pads[4 + T23_SCALED_OUTPUTS + T23_MSCA_OSD_REGIONS];
	struct vb2_queue queue;
	struct media_pipeline pipe;	/* shared by the main and scaled nodes */
	struct mutex isp_sd_lock;
	struct v4l2_ctrl_handler controls;
	struct t23_sub_node sub[T23_SCALED_OUTPUTS];
	struct t23_motion motion;

	/* Per-channel ISP controls (exposed on t23-isp-coreN subdev) */
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *dgain;
	struct v4l2_ctrl *red_balance;
	struct v4l2_ctrl *blue_balance;
	struct v4l2_ctrl *gamma_ctrl;
	struct v4l2_ctrl *brightness;
	struct v4l2_ctrl *contrast;
	struct v4l2_ctrl *saturation;
	struct v4l2_ctrl *sharpness;

	bool hflip_val;
	bool vflip_val;
	u32 dgain_val;
	u32 red_val;
	u32 blue_val;
	u32 gamma_val;
	s32 brightness_val;
	u32 contrast_val;
	u32 saturation_val;
	u32 sharpness_val;
	u8 gamma_lut[1024];

	int input;
	struct list_head bufs;
	struct v4l2_pix_format pix;
	bool streaming;
	bool stopping;
	bool queue_initialized;
	bool entity_initialized;
	bool vdev_registered;
	bool isp_sd_finalized;
	bool isp_sd_registered;
	u32 sequence;			/* frames delivered on any output node */
	unsigned int csi_clock_mhz;
	unsigned int skip_frames;
};

struct t23_isp_video {
	struct v4l2_device v4l2_dev;
	struct media_device mdev;
	struct v4l2_async_notifier notifier;
	struct t23_async_subdev *ep_asd[T23_MAX_INPUTS];
	unsigned int num_endpoints;
	struct t23_video_input inputs[T23_MAX_INPUTS];
	struct mutex lock;
	spinlock_t qlock;
	wait_queue_head_t dma_wait;
	struct tx_isp_device *isp;
	struct device *dev;
	void (*media_release)(struct media_devnode *node);
	int active_channel;
	u32 active_outputs;		/* BIT(0) main, BIT(1..2) scaled */
	unsigned int next_channel;
	int sync_source;
	struct mutex sync_lock;		/* frame length of the synchronized inputs */
	struct work_struct vblank_sync_work;
	atomic_t vblank_syncing;
	unsigned int stream_order[T23_MAX_INPUTS];
	unsigned int num_streaming;
	bool disconnected;
	bool streaming;
	bool dma_active;
	bool processed_dma_done;
	bool processed_all_done;
	bool processed_completing;
	bool dma_failed;
	bool irq_enabled;
	bool media_registered;
	bool registered;
	struct t23_video_node channels[T23_CAPTURE_CHANNELS];
	struct t23_meta meta;
	struct t23_overlay overlay;
};

static struct t23_isp_video *t23_video;
static DEFINE_MUTEX(t23_video_lock);
static void t23_apply_processed_controls(struct t23_video_node *chan);

static bool t23_chan_streaming(const struct t23_video_node *chan)
{
	unsigned int s;

	if (chan->streaming)
		return true;
	for (s = 0; s < T23_SCALED_OUTPUTS; s++)
		if (chan->sub[s].streaming)
			return true;
	return false;
}

/*
 * Run one VIC frame with MSCA output disabled. It commits the MSCA shadow
 * geometry and seeds the MDNS history before the first DMA frame.
 */
static int t23_processed_prime(struct t23_isp_video *video, unsigned int input)
{
	struct tx_isp_device *isp = video->isp;
	struct tx_isp_subdev *sd = isp->subdevs[TX_ISP_SUBDEV_CORE];
	struct tx_isp_core_device *core = sd->dev_priv;
	void __iomem *vic = isp->subdevs[TX_ISP_SUBDEV_VIC]->base;
	unsigned int elapsed, latched = 0;
	u32 status, done, pending;
	int ret;

	ret = t23_isp_stats_configure(&core->stats, sd->base, input);
	if (ret)
		return ret;
	ret = tx_isp_core_mdns_start(sd, input);
	if (ret)
		return ret;
	writel(0, sd->base + MSCA_CH_EN_MASK);
	writel(1, sd->base + MSCA_SHD_TRIGGER);
	writel(1, sd->base + MSCA_REG_060);
	writel(1, isp->ivdc_base + IVDC_TOP_INFO1);
	writel(ISP_IPC_MODE_DEFAULT, sd->base + ISP_IPC_MODE);
	writel(ISP_IPC_TRIGGER_START, sd->base + ISP_IPC_TRIGGER);
	dev_dbg(video->dev,
		 "prime before: run=%08x mode=%08x irq=%08x done=%08x msca=%08x geom=%08x\n",
		 readl(sd->base + ISP_IPC_TRIGGER), readl(sd->base + ISP_IPC_MODE),
		 readl(sd->base + ISP_INT_STATUS), readl(sd->base + ISP_INT1_STATUS),
		 readl(sd->base + MSCA_REG_060), readl(sd->base + MSCA_FCROP_SIZE));
	tx_isp_vic_capture_trigger(isp->subdevs[TX_ISP_SUBDEV_VIC], true);
	for (elapsed = 10; elapsed <= 150; elapsed += 10) {
		msleep(10);
		if (!latched && readl(sd->base + MSCA_REG_060) == 1)
			latched = elapsed;
	}
	tx_isp_vic_capture_trigger(isp->subdevs[TX_ISP_SUBDEV_VIC], false);
	status = readl(sd->base + ISP_INT_STATUS);
	done = readl(sd->base + ISP_INT1_STATUS);
	dev_dbg(video->dev,
		 "prime after: latch_ms=%u run=%08x mode=%08x irq=%08x done=%08x msca=%08x geom=%08x\n",
		 latched, readl(sd->base + ISP_IPC_TRIGGER), readl(sd->base + ISP_IPC_MODE),
		 readl(sd->base + ISP_INT_STATUS), readl(sd->base + ISP_INT1_STATUS),
		 readl(sd->base + MSCA_REG_060), readl(sd->base + MSCA_FCROP_SIZE));
	if (status & ISP_INT_ERR_MASK) {
		dev_err(video->dev, "prime frame error: %#x\n", status);
		return -EIO;
	}
	if (!latched || !(done & ISP_INT1_BIT15)) {
		dev_err(video->dev, "prime frame timeout: done=%#x\n", done);
		return -ETIMEDOUT;
	}
	tx_isp_core_mdns_frame_done(sd);
	writel(done, sd->base + ISP_INT1_CLEAR);
	writel(status, sd->base + ISP_INT_CLEAR);
	pending = readl(vic + VIC_STATUS_REG_0);
	writel(pending, vic + VIC_CLEAR_REG_0);
	pending = readl(vic + VIC_STATUS_REG_1);
	writel(pending, vic + VIC_CLEAR_REG_1);
	return 0;
}

static bool t23_main_ready(struct t23_video_node *chan)
{
	return chan->streaming && !chan->stopping && !list_empty(&chan->bufs);
}

static bool t23_sub_ready(struct t23_video_node *chan, unsigned int s)
{
	struct t23_sub_node *sub = &chan->sub[s];

	return chan->video->isp->processed && sub->streaming &&
	       !sub->stopping && !list_empty(&sub->bufs);
}

static void t23_msca_target(struct tx_isp_msca_target *out,
			    const struct t23_video_node *chan,
			    struct list_head *bufs,
			    const struct v4l2_pix_format *pix)
{
	struct t23_isp_buffer *buf = list_first_entry(bufs,
		struct t23_isp_buffer, list);
	dma_addr_t y = vb2_dma_contig_plane_dma_addr(&buf->vb.vb2_buf, 0);

	out->width = pix->width;
	out->height = pix->height;
	out->y = y;
	out->uv = y + (size_t)pix->bytesperline * pix->height;
	out->hflip = chan->hflip_val;
	out->vflip = chan->vflip_val;
}

static void t23_capture_next(struct t23_isp_video *video)
{
	unsigned int n, i, s;
	int ret;

	if (!video->streaming || video->dma_active || video->dma_failed)
		return;
	for (n = 0; n < T23_CAPTURE_CHANNELS; n++) {
		unsigned int index = (video->next_channel + n) % T23_CAPTURE_CHANNELS;
		struct t23_video_node *chan = &video->channels[index];
		bool main = t23_main_ready(chan);
		u32 outputs = main ? BIT(0) : 0;

		for (s = 0; s < T23_SCALED_OUTPUTS; s++)
			if (t23_sub_ready(chan, s))
				outputs |= BIT(s + 1);
		if (!outputs)
			continue;
		video->active_channel = index;
		video->active_outputs = outputs;
		video->next_channel = (index + 1) % T23_CAPTURE_CHANNELS;
		video->dma_active = true;
		dma_wmb();
		tx_isp_csi_select_rate(video->isp->subdevs[TX_ISP_SUBDEV_CSI],
				       chan->csi_clock_mhz);
		tx_isp_vic_select_port(video->isp->subdevs[TX_ISP_SUBDEV_VIC], index);
		if (video->isp->processed) {
			struct tx_isp_msca_target out[T23_MSCA_OUTPUTS] = { };
			struct tx_isp_msca_overlay overlay;

			/* MSCA writes straight into the head buffers. */
			if (main)
				t23_msca_target(&out[0], chan, &chan->bufs,
						&chan->pix);
			for (s = 0; s < T23_SCALED_OUTPUTS; s++)
				if (outputs & BIT(s + 1))
					t23_msca_target(&out[s + 1], chan,
							&chan->sub[s].bufs,
							&chan->sub[s].pix);
			video->processed_dma_done = false;
			video->processed_all_done = false;
			video->processed_completing = false;
			t23_meta_params_apply(&video->meta, index,
				video->isp->subdevs[TX_ISP_SUBDEV_CORE]);
			t23_apply_processed_controls(chan);
			t23_overlay_frame(&video->overlay, index, &overlay);
			ret = tx_isp_core_dma_arm(
				video->isp->subdevs[TX_ISP_SUBDEV_CORE], out,
				&overlay, index);
			if (ret) {
				video->dma_failed = true;
				video->dma_active = false;
				for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
					if (video->channels[i].streaming)
						vb2_queue_error(&video->channels[i].queue);
					for (s = 0; s < T23_SCALED_OUTPUTS; s++)
						if (video->channels[i].sub[s].streaming)
							vb2_queue_error(&video->channels[i].sub[s].queue);
				}
				wake_up_all(&video->dma_wait);
				dev_err(video->dev, "Cannot arm ISP DMA: %d\n", ret);
				return;
			}
			tx_isp_vic_capture_trigger(
				video->isp->subdevs[TX_ISP_SUBDEV_VIC], true);
		} else
			tx_isp_vic_dma_arm(video->isp->subdevs[TX_ISP_SUBDEV_VIC],
					   video->isp->dma_address);
		break;
	}
}

void t23_isp_video_processed_event(struct tx_isp_device *isp, bool dma_done,
				 bool all_done, bool error)
{
	struct t23_isp_video *video = isp->video_priv;
	unsigned long flags;
	bool complete;

	if (!video || !isp->processed)
		return;
	spin_lock_irqsave(&video->qlock, flags);
	if (!video->streaming || !video->dma_active || video->dma_failed ||
	    video->processed_completing) {
		spin_unlock_irqrestore(&video->qlock, flags);
		return;
	}
	video->processed_dma_done |= dma_done;
	video->processed_all_done |= all_done;
	/* Output completion alone does not release the shared ISP input. */
	complete = error ||
		(video->processed_dma_done && video->processed_all_done);
	if (complete) {
		video->processed_completing = true;
		tx_isp_vic_capture_trigger(isp->subdevs[TX_ISP_SUBDEV_VIC], false);
		tx_isp_core_dma_stop(isp->subdevs[TX_ISP_SUBDEV_CORE], error);
		if (!error)
			tx_isp_core_mdns_frame_done(isp->subdevs[TX_ISP_SUBDEV_CORE]);
		video->processed_dma_done = false;
		video->processed_all_done = false;
	}
	spin_unlock_irqrestore(&video->qlock, flags);
	if (complete)
		t23_isp_video_dma_done(isp, error);
}

void t23_isp_video_frame_event(struct tx_isp_device *isp)
{
	struct t23_isp_video *video = isp->video_priv;
	struct t23_video_node *chan;
	unsigned long flags;

	if (!video || isp->processed)
		return;

	spin_lock_irqsave(&video->qlock, flags);
	chan = video->active_channel < 0 ? NULL :
		&video->channels[video->active_channel];
	if (chan && chan->streaming && video->dma_active && !video->dma_failed)
		tx_isp_vic_dma_resume(isp->subdevs[TX_ISP_SUBDEV_VIC]);
	spin_unlock_irqrestore(&video->qlock, flags);
}

static void t23_apply_processed_controls(struct t23_video_node *chan)
{
	struct tx_isp_subdev *core_sd =
		chan->video->isp->subdevs[TX_ISP_SUBDEV_CORE];
	struct tx_isp_core_device *core = core_sd ? core_sd->dev_priv : NULL;
	const struct tx_isp_params_state *params =
		core && core->params &&
		chan->channel_idx < ARRAY_SIZE(core->history) ?
		&core->params[chan->channel_idx] : NULL;

	if (!params || !params->blc_valid)
		tx_isp_core_set_blc_default(core_sd);
	tx_isp_core_set_dgain(core_sd, chan->dgain_val);
	if (!params || !(params->valid & TX_ISP_PARAM_WB))
		tx_isp_core_set_wb(core_sd, chan->red_val, chan->blue_val);
	if (!params || !(params->valid & TX_ISP_PARAM_CCM))
		tx_isp_core_set_ccm(core_sd, tx_isp_default_ccm);
	if (!params || !(params->valid & TX_ISP_PARAM_DMSC))
		tx_isp_core_set_sharpness(core_sd, chan->sharpness_val);
}

static void t23_update_gamma_lut(struct t23_video_node *chan)
{
	int i;
	u32 g = chan->gamma_val;

	g = clamp(g, 70U, 180U);

	for (i = 0; i < 1024; i++) {
		int j = i / 8;
		int r = i % 8;
		u16 v0 = tx_isp_calc_gamma_entry(j, g);
		u16 v1 = tx_isp_calc_gamma_entry(j + 1, g);
		u32 v = v0 + ((v1 - v0) * r) / 8;
		u32 out = v >> 4;
		chan->gamma_lut[i] = (out > 255) ? 255 : (u8)out;
	}
}

static void t23_process_raw10(void *destination, const void *source,
			      unsigned int width, unsigned int height,
			      u32 r_gain_q8, u32 b_gain_q8)
{
	__le16 *dst = destination;
	const __le16 *src = destination;
	unsigned int x, y;

	/* Copy uncached DMA memory in bulk before per-pixel processing. */
	memcpy(destination, source, (size_t)width * height * sizeof(*dst));
	for (y = 0; y < height; y++) {
		const __le16 *s_row = src + y * width;
		__le16 *d_row = dst + y * width;
		bool y_odd = (y & 1);

		for (x = 0; x < width; x++) {
			u32 raw = (le16_to_cpu(s_row[x]) >> 6) & 0x03ff;
			bool x_odd = (x & 1);

			if (!y_odd && !x_odd) {
				/* Blue site (even row, even col in BGGR) */
				int val = max(0, (int)raw - 64);
				val = (val * (int)b_gain_q8) >> 8;
				val += 64;
				raw = clamp_val(val, 0, 1023);
			} else if (y_odd && x_odd) {
				/* Red site (odd row, odd col in BGGR) */
				int val = max(0, (int)raw - 64);
				val = (val * (int)r_gain_q8) >> 8;
				val += 64;
				raw = clamp_val(val, 0, 1023);
			}
			d_row[x] = cpu_to_le16(raw);
		}
	}
}

static void t23_process_isp_frame(void *destination, const void *source,
				  unsigned int width, unsigned int height,
				  u32 pixelformat, struct t23_video_node *chan)
{
	const __le16 *src = source;
	u8 *dst = destination;
	u8 *y_plane = dst;
	u8 *uv_plane = dst + (size_t)width * height;
	u8 *yuyv_dst = dst;
	u8 *rgb_dst = dst;
	u32 r_gain = chan->red_val;
	u32 b_gain = chan->blue_val;
	const u8 *lut = chan->gamma_lut;
	s32 bright = chan->brightness_val;
	int contrast = (int)chan->contrast_val;
	int sat = (int)chan->saturation_val;
	unsigned int x, y;

	for (y = 0; y < height; y += 2) {
		const __le16 *row0 = src + y * width;
		const __le16 *row1 = src + (y + 1) * width;
		unsigned int uv_y = y / 2;

		for (x = 0; x < width; x += 2) {
			u32 raw_b, raw_gr, raw_gb, raw_r;
			int b0, gr0, gb0, r0;
			int b_wb, r_wb, g_top, g_bot;
			const s16 *ccm;
			int r_ccm0, g_ccm0, b_ccm0;
			int r_ccm1, g_ccm1, b_ccm1;
			u8 r_g0, g_g0, b_g0;
			u8 r_g1, g_g1, b_g1;

			/* 2x2 BGGR raw cell */
			raw_b  = (le16_to_cpu(row0[x]) >> 6) & 0x3ff;
			raw_gr = (le16_to_cpu(row0[x + 1]) >> 6) & 0x3ff;
			raw_gb = (le16_to_cpu(row1[x]) >> 6) & 0x3ff;
			raw_r  = (le16_to_cpu(row1[x + 1]) >> 6) & 0x3ff;

			/* Black level subtraction (pedestal 64) */
			b0  = max(0, (int)raw_b - 64);
			gr0 = max(0, (int)raw_gr - 64);
			gb0 = max(0, (int)raw_gb - 64);
			r0  = max(0, (int)raw_r - 64);

			/* Digital white balance */
			b_wb = min(1023, (b0 * (int)b_gain) >> 8);
			r_wb = min(1023, (r0 * (int)r_gain) >> 8);
			g_top = gr0;
			g_bot = gb0;

			/* CCM for the top (gr) and bottom (gb) rows */
			ccm = tx_isp_default_ccm;
			r_ccm0 = clamp_val((ccm[0] * r_wb + ccm[1] * g_top + ccm[2] * b_wb + 512) >> 10, 0, 1023);
			g_ccm0 = clamp_val((ccm[3] * r_wb + ccm[4] * g_top + ccm[5] * b_wb + 512) >> 10, 0, 1023);
			b_ccm0 = clamp_val((ccm[6] * r_wb + ccm[7] * g_top + ccm[8] * b_wb + 512) >> 10, 0, 1023);

			r_ccm1 = clamp_val((ccm[0] * r_wb + ccm[1] * g_bot + ccm[2] * b_wb + 512) >> 10, 0, 1023);
			g_ccm1 = clamp_val((ccm[3] * r_wb + ccm[4] * g_bot + ccm[5] * b_wb + 512) >> 10, 0, 1023);
			b_ccm1 = clamp_val((ccm[6] * r_wb + ccm[7] * g_bot + ccm[8] * b_wb + 512) >> 10, 0, 1023);

			/* Gamma LUT lookup (10-bit -> 8-bit display) */
			r_g0 = lut[r_ccm0];
			g_g0 = lut[g_ccm0];
			b_g0 = lut[b_ccm0];

			r_g1 = lut[r_ccm1];
			g_g1 = lut[g_ccm1];
			b_g1 = lut[b_ccm1];

			if (pixelformat == V4L2_PIX_FMT_RGB24) {
				int r_p0 = clamp_val((((r_g0 - 128) * contrast) >> 7) + 128 + bright, 0, 255);
				int g_p0 = clamp_val((((g_g0 - 128) * contrast) >> 7) + 128 + bright, 0, 255);
				int b_p0 = clamp_val((((b_g0 - 128) * contrast) >> 7) + 128 + bright, 0, 255);

				int r_p1 = clamp_val((((r_g1 - 128) * contrast) >> 7) + 128 + bright, 0, 255);
				int g_p1 = clamp_val((((g_g1 - 128) * contrast) >> 7) + 128 + bright, 0, 255);
				int b_p1 = clamp_val((((b_g1 - 128) * contrast) >> 7) + 128 + bright, 0, 255);

				size_t idx00 = ((size_t)y * width + x) * 3;
				size_t idx01 = ((size_t)y * width + x + 1) * 3;
				size_t idx10 = (((size_t)y + 1) * width + x) * 3;
				size_t idx11 = (((size_t)y + 1) * width + x + 1) * 3;

				rgb_dst[idx00]     = r_p0; rgb_dst[idx00 + 1] = g_p0; rgb_dst[idx00 + 2] = b_p0;
				rgb_dst[idx01]     = r_p0; rgb_dst[idx01 + 1] = g_p0; rgb_dst[idx01 + 2] = b_p0;
				rgb_dst[idx10]     = r_p1; rgb_dst[idx10 + 1] = g_p1; rgb_dst[idx10 + 2] = b_p1;
				rgb_dst[idx11]     = r_p1; rgb_dst[idx11 + 1] = g_p1; rgb_dst[idx11 + 2] = b_p1;
			} else {
				/* BT.601 RGB to YUV */
				int y0 = (77 * r_g0 + 150 * g_g0 + 29 * b_g0 + 128) >> 8;
				int y1 = (77 * r_g1 + 150 * g_g1 + 29 * b_g1 + 128) >> 8;

				int r_avg = (r_g0 + r_g1) >> 1;
				int g_avg = (g_g0 + g_g1) >> 1;
				int b_avg = (b_g0 + b_g1) >> 1;

				int u = ((-43 * r_avg - 85 * g_avg + 128 * b_avg + 128) >> 8) + 128;
				int v = ((128 * r_avg - 107 * g_avg - 21 * b_avg + 128) >> 8) + 128;

				u8 y_out0 = clamp_val((((y0 - 128) * contrast) >> 7) + 128 + bright, 0, 255);
				u8 y_out1 = clamp_val((((y1 - 128) * contrast) >> 7) + 128 + bright, 0, 255);
				u8 u_out  = clamp_val((((u - 128) * sat) >> 7) + 128, 0, 255);
				u8 v_out  = clamp_val((((v - 128) * sat) >> 7) + 128, 0, 255);

				if (pixelformat == V4L2_PIX_FMT_NV12) {
					y_plane[(size_t)y * width + x]             = y_out0;
					y_plane[(size_t)y * width + x + 1]         = y_out0;
					y_plane[((size_t)y + 1) * width + x]       = y_out1;
					y_plane[((size_t)y + 1) * width + x + 1]   = y_out1;

					uv_plane[(size_t)uv_y * width + x]         = u_out;
					uv_plane[(size_t)uv_y * width + x + 1]     = v_out;
				} else if (pixelformat == V4L2_PIX_FMT_YUYV) {
					size_t o0 = ((size_t)y * width + x) * 2;
					size_t o1 = (((size_t)y + 1) * width + x) * 2;

					yuyv_dst[o0]     = y_out0;
					yuyv_dst[o0 + 1] = u_out;
					yuyv_dst[o0 + 2] = y_out0;
					yuyv_dst[o0 + 3] = v_out;

					yuyv_dst[o1]     = y_out1;
					yuyv_dst[o1 + 1] = u_out;
					yuyv_dst[o1 + 2] = y_out1;
					yuyv_dst[o1 + 3] = v_out;
				}

			}
		}
	}
}

static void t23_stats_done(struct t23_isp_video *video,
			   struct t23_video_node *chan, u32 sequence,
			   u64 timestamp)
{
	struct tx_isp_subdev *sd = video->isp->subdevs[TX_ISP_SUBDEV_CORE];
	struct tx_isp_core_device *core = sd->dev_priv;
	u32 irq = xchg(&core->stats_irq, 0);
	u32 thresholds = readl(sd->base + ISP_AE_LUM_TH_FREQ);
	struct t23_meta_frame frame = {
		.sequence = sequence,
		.timestamp = timestamp,
		.hw = core->stats.buffer + chan->channel_idx * T23_STATS_INPUT_SIZE,
		.zone_width = chan->pix.width / 30,
		.zone_height = chan->pix.height / 30,
		.ae_low = thresholds & 0xff,
		.ae_high = thresholds >> 8,
	};

	if (irq & ISP_INT_AE)
		frame.valid |= T23_ISP_STATS_AE;
	if (irq & ISP_INT_HIST)
		frame.valid |= T23_ISP_STATS_HIST;
	if (irq & ISP_INT_AWB)
		frame.valid |= T23_ISP_STATS_AWB;
	t23_meta_stats_done(&video->meta, chan->channel_idx, &frame);
	if (frame.valid & T23_ISP_STATS_AE)
		t23_motion_frame(&chan->motion, frame.hw, sequence);
}

/* The buffer a finished frame fills, or NULL while start-up frames drop. */
static struct t23_isp_buffer *t23_frame_buffer(struct list_head *bufs,
					       unsigned int *skip_frames)
{
	struct t23_isp_buffer *buf;

	if (*skip_frames) {
		(*skip_frames)--;
		return NULL;
	}
	if (list_empty(bufs))
		return NULL;
	buf = list_first_entry(bufs, struct t23_isp_buffer, list);
	list_del_init(&buf->list);
	return buf;
}

static void t23_fail_queue(struct vb2_queue *queue, struct list_head *bufs)
{
	struct t23_isp_buffer *buf;

	vb2_queue_error(queue);
	while (!list_empty(bufs)) {
		buf = list_first_entry(bufs, struct t23_isp_buffer, list);
		list_del_init(&buf->list);
		vb2_set_plane_payload(&buf->vb.vb2_buf, 0, 0);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
}

static void t23_buffer_done(struct t23_isp_buffer *buf, u32 sequence,
			    u64 timestamp, unsigned long size)
{
	buf->vb.sequence = sequence;
	buf->vb.field = V4L2_FIELD_NONE;
	buf->vb.vb2_buf.timestamp = timestamp;
	vb2_set_plane_payload(&buf->vb.vb2_buf, 0, size);
	vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

void t23_isp_video_dma_done(struct tx_isp_device *isp, bool error)
{
	struct t23_isp_video *video = isp->video_priv;
	struct t23_isp_buffer *buf = NULL, *sub_buf[T23_SCALED_OUTPUTS] = { };
	struct t23_video_node *chan;
	unsigned long flags;
	unsigned int s;
	bool any_sub = false;
	u32 sequence;
	u64 timestamp;

	if (!video)
		return;

	spin_lock_irqsave(&video->qlock, flags);
	if (video->active_channel < 0) {
		spin_unlock_irqrestore(&video->qlock, flags);
		return;
	}
	chan = &video->channels[video->active_channel];
	if (!video->dma_active) {
		spin_unlock_irqrestore(&video->qlock, flags);
		return;
	}
	if (!error && !t23_chan_streaming(chan)) {
		video->dma_active = false;
		t23_capture_next(video);
		wake_up_all(&video->dma_wait);
		spin_unlock_irqrestore(&video->qlock, flags);
		return;
	}
	if (error) {
		unsigned int i;

		video->dma_failed = true;
		video->dma_active = false;
		wake_up_all(&video->dma_wait);
		for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
			struct t23_video_node *failed = &video->channels[i];

			if (failed->streaming)
				t23_fail_queue(&failed->queue, &failed->bufs);
			for (s = 0; s < T23_SCALED_OUTPUTS; s++)
				if (failed->sub[s].streaming)
					t23_fail_queue(&failed->sub[s].queue,
						       &failed->sub[s].bufs);
		}
		spin_unlock_irqrestore(&video->qlock, flags);
		dev_err(&isp->pdev->dev, "DMA transfer failed on input %u\n",
			chan->channel_idx);
		return;
	}

	video->dma_failed = false;
	if (video->active_outputs & BIT(0))
		buf = t23_frame_buffer(&chan->bufs, &chan->skip_frames);
	for (s = 0; s < T23_SCALED_OUTPUTS; s++) {
		if (video->active_outputs & BIT(s + 1))
			sub_buf[s] = t23_frame_buffer(&chan->sub[s].bufs,
						      &chan->sub[s].skip_frames);
		any_sub |= !!sub_buf[s];
	}
	if (!buf && !any_sub) {
		video->dma_active = false;
		t23_capture_next(video);
		wake_up_all(&video->dma_wait);
		spin_unlock_irqrestore(&video->qlock, flags);
		return;
	}
	sequence = chan->sequence++;
	timestamp = ktime_get_ns();
	spin_unlock_irqrestore(&video->qlock, flags);

	dma_rmb();

	if (isp->processed) {
		/* The frame is already in the buffers. */
		t23_stats_done(video, chan, sequence, timestamp);
	} else if (chan->pix.pixelformat == V4L2_PIX_FMT_SBGGR10 ||
		   chan->pix.pixelformat == V4L2_PIX_FMT_SRGGB10 ||
		   chan->pix.pixelformat == V4L2_PIX_FMT_SGBRG10 ||
		   chan->pix.pixelformat == V4L2_PIX_FMT_SGRBG10) {
		t23_process_raw10(vb2_plane_vaddr(&buf->vb.vb2_buf, 0),
				  isp->dma_buffer, chan->pix.width, chan->pix.height,
				  chan->red_val, chan->blue_val);
	} else {
		t23_process_isp_frame(vb2_plane_vaddr(&buf->vb.vb2_buf, 0),
				      isp->dma_buffer, chan->pix.width, chan->pix.height,
				      chan->pix.pixelformat, chan);
	}

	if (buf)
		t23_buffer_done(buf, sequence, timestamp, chan->pix.sizeimage);
	for (s = 0; s < T23_SCALED_OUTPUTS; s++)
		if (sub_buf[s])
			t23_buffer_done(sub_buf[s], sequence, timestamp,
					chan->sub[s].pix.sizeimage);
	spin_lock_irqsave(&video->qlock, flags);
	video->dma_active = false;
	t23_capture_next(video);
	wake_up_all(&video->dma_wait);
	spin_unlock_irqrestore(&video->qlock, flags);
}

static bool is_raw10_pixfmt(u32 pixelformat)
{
	return pixelformat == V4L2_PIX_FMT_SBGGR10 ||
	       pixelformat == V4L2_PIX_FMT_SRGGB10 ||
	       pixelformat == V4L2_PIX_FMT_SGBRG10 ||
	       pixelformat == V4L2_PIX_FMT_SGRBG10;
}

static struct v4l2_subdev *t23_sensor(struct t23_video_node *chan)
{
	struct t23_isp_video *video = chan->video;

	if (video->disconnected || chan->input < 0)
		return NULL;
	return video->inputs[chan->input].sd;
}

static int t23_sensor_vblank(struct v4l2_subdev *sd, s32 *val, bool set)
{
	struct v4l2_control c = { .id = V4L2_CID_VBLANK, .value = *val };
	int ret;

	if (!sd || !sd->ctrl_handler)
		return -ENOLINK;
	if (set)
		return v4l2_s_ctrl(NULL, sd->ctrl_handler, &c);
	ret = v4l2_g_ctrl(sd->ctrl_handler, &c);
	*val = c.value;
	return ret;
}

/*
 * A frame-sync slave must run its source's frame length: a longer slave
 * frame is cut short by the next sync pulse and fails the VIC transfer,
 * and the slave's half-frame offset is derived from its own length. Set
 * @length on @sensor and copy it to the slaves. Setting a slave keeps the
 * source's length and returns it in @length. A shorter frame reaches the
 * slaves first.
 */
static int t23_set_frame_length(struct t23_isp_video *video,
				struct v4l2_subdev *sensor, s32 *length)
{
	s32 val = *length;
	struct v4l2_subdev *source;
	s32 current_val;
	unsigned int i;
	bool shorter;
	int ret;

	if (video->sync_source < 0)
		return t23_sensor_vblank(sensor, length, true);
	source = video->inputs[video->sync_source].sd;
	mutex_lock(&video->sync_lock);
	ret = t23_sensor_vblank(source, &current_val, false);
	if (ret)
		goto out;
	if (sensor != source)
		val = current_val;
	shorter = val < current_val;
	if (!shorter) {
		ret = t23_sensor_vblank(source, &val, true);
		if (ret)
			goto out;
	}
	for (i = 0; i < video->num_endpoints; i++) {
		struct v4l2_subdev *slave = video->inputs[i].sd;

		if (i == video->sync_source || !slave)
			continue;
		ret = t23_sensor_vblank(slave, &val, true);
		if (ret)
			goto out;
	}
	if (shorter)
		ret = t23_sensor_vblank(source, &val, true);
	if (!ret)
		*length = val;
out:
	mutex_unlock(&video->sync_lock);
	return ret;
}

static int t23_sync_frame_length(struct t23_isp_video *video)
{
	struct v4l2_subdev *source;
	unsigned int i;
	s32 src_val;
	int ret;

	if (video->sync_source < 0)
		return 0;
	source = video->inputs[video->sync_source].sd;
	if (!source)
		return 0;

	mutex_lock(&video->sync_lock);
	ret = t23_sensor_vblank(source, &src_val, false);
	if (ret)
		goto out;
	for (i = 0; i < video->num_endpoints; i++) {
		struct v4l2_subdev *slave = video->inputs[i].sd;
		s32 slave_val;

		if (i == video->sync_source || !slave)
			continue;
		if (!t23_sensor_vblank(slave, &slave_val, false) &&
		    slave_val == src_val)
			continue;
		atomic_inc(&video->vblank_syncing);
		ret = t23_sensor_vblank(slave, &src_val, true);
		atomic_dec(&video->vblank_syncing);
		if (ret)
			goto out;
	}
out:
	mutex_unlock(&video->sync_lock);
	return ret;
}

static void t23_vblank_notify(struct v4l2_ctrl *ctrl, void *priv)
{
	struct t23_isp_video *video = priv;

	if (ctrl->id == V4L2_CID_VBLANK && !atomic_read(&video->vblank_syncing))
		queue_work(system_highpri_wq, &video->vblank_sync_work);
}

static void t23_vblank_sync_work(struct work_struct *work)
{
	struct t23_isp_video *video =
		container_of(work, struct t23_isp_video, vblank_sync_work);

	mutex_lock(&video->lock);
	if (!video->disconnected && video->registered)
		t23_sync_frame_length(video);
	mutex_unlock(&video->lock);
}

static void t23_sensor_vblank_watch(struct t23_isp_video *video,
				    struct v4l2_subdev *sd, bool enable)
{
	struct v4l2_ctrl *ctrl;

	if (video->sync_source < 0 || !sd || !sd->ctrl_handler)
		return;
	ctrl = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_VBLANK);
	if (!ctrl)
		return;
	mutex_lock(sd->ctrl_handler->lock);
	if (enable) {
		if (!ctrl->handler->notify)
			v4l2_ctrl_notify(ctrl, t23_vblank_notify, video);
	} else if (ctrl->handler->notify == t23_vblank_notify) {
		v4l2_ctrl_notify(ctrl, NULL, NULL);
	}
	mutex_unlock(sd->ctrl_handler->lock);
}

static int t23_control_set(struct v4l2_ctrl *ctrl)
{
	struct t23_video_node *chan = container_of(ctrl->handler,
						 struct t23_video_node, controls);
	struct v4l2_subdev *sensor = t23_sensor(chan);
	struct v4l2_control value = { .id = ctrl->id, .value = ctrl->val };

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		chan->hflip_val = ctrl->val;
		if (chan->video->streaming && !chan->video->isp->processed &&
		    sensor && sensor->ctrl_handler)
			return v4l2_s_ctrl(NULL, sensor->ctrl_handler, &value);
		break;

	case V4L2_CID_VFLIP:
		chan->vflip_val = ctrl->val;
		if (chan->video->streaming && !chan->video->isp->processed &&
		    sensor && sensor->ctrl_handler)
			return v4l2_s_ctrl(NULL, sensor->ctrl_handler, &value);
		break;

	case V4L2_CID_DIGITAL_GAIN:
		chan->dgain_val = ctrl->val;
		break;

	case V4L2_CID_RED_BALANCE:
		chan->red_val = ctrl->val;
		break;

	case V4L2_CID_BLUE_BALANCE:
		chan->blue_val = ctrl->val;
		break;

	case V4L2_CID_BRIGHTNESS:
		chan->brightness_val = ctrl->val;
		break;

	case V4L2_CID_CONTRAST:
		chan->contrast_val = ctrl->val;
		break;

	case V4L2_CID_SATURATION:
		chan->saturation_val = ctrl->val;
		break;

	case V4L2_CID_SHARPNESS:
		chan->sharpness_val = ctrl->val;
		break;

	case V4L2_CID_GAMMA:
		chan->gamma_val = ctrl->val;
		t23_update_gamma_lut(chan);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static const struct v4l2_ctrl_ops t23_control_ops = {
	.s_ctrl = t23_control_set,
};

static const struct {
	u32 pixelformat;
	const char *description;
} t23_formats[] = {
	{ V4L2_PIX_FMT_NV12,    "Y/CbCr 4:2:0" },
	{ V4L2_PIX_FMT_SBGGR10, "10-bit Bayer BGBG/GRGR" },
	{ V4L2_PIX_FMT_SRGGB10, "10-bit Bayer RGRG/GBGB" },
	{ V4L2_PIX_FMT_SGBRG10, "10-bit Bayer GBGB/RGRG" },
	{ V4L2_PIX_FMT_SGRBG10, "10-bit Bayer GRGR/BGBG" },
	{ V4L2_PIX_FMT_YUYV,    "YUYV 4:2:2" },
	{ V4L2_PIX_FMT_RGB24,   "24-bit RGB 8-8-8" },
};

static bool is_supported_format(u32 pixelformat)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(t23_formats); i++) {
		if (t23_formats[i].pixelformat == pixelformat)
			return true;
	}
	return false;
}

static int t23_fill_pix_format(struct v4l2_pix_format *pix, u32 pixelformat,
			       u32 width, u32 height)
{
	memset(pix, 0, sizeof(*pix));
	pix->width = width;
	pix->height = height;
	pix->pixelformat = pixelformat;
	pix->field = V4L2_FIELD_NONE;

	switch (pixelformat) {
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
		if (check_mul_overflow(width, 2U, &pix->bytesperline) ||
		    check_mul_overflow(pix->bytesperline, height, &pix->sizeimage))
			return -EOVERFLOW;
		pix->colorspace = V4L2_COLORSPACE_RAW;
		break;
	case V4L2_PIX_FMT_NV12:
		pix->bytesperline = width;
		/*
		 * Pad allocation size to 16-row macroblock height so Helix
		 * zero-copy DMABUF reads never cross the end of the buffer.
		 */
		pix->sizeimage = (width * ALIGN(height, 16) * 3) / 2;
		pix->colorspace = V4L2_COLORSPACE_SMPTE170M;
		pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
		break;
	case V4L2_PIX_FMT_YUYV:
		if (check_mul_overflow(width, 2U, &pix->bytesperline) ||
		    check_mul_overflow(pix->bytesperline, height, &pix->sizeimage))
			return -EOVERFLOW;
		pix->colorspace = V4L2_COLORSPACE_SMPTE170M;
		pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
		break;
	case V4L2_PIX_FMT_RGB24:
		if (check_mul_overflow(width, 3U, &pix->bytesperline) ||
		    check_mul_overflow(pix->bytesperline, height, &pix->sizeimage))
			return -EOVERFLOW;
		pix->colorspace = V4L2_COLORSPACE_SRGB;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static u32 t23_chan_sink_code(const struct t23_video_node *chan)
{
	if (chan->input >= 0 && chan->video->inputs[chan->input].mbus_code)
		return chan->video->inputs[chan->input].mbus_code;
	return MEDIA_BUS_FMT_SBGGR10_1X10;
}

static int t23_isp_subdev_get_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *fmt)
{
	struct t23_video_node *chan = v4l2_get_subdevdata(sd);

	if (fmt->pad >= 4 && fmt->pad < 4 + T23_SCALED_OUTPUTS) {
		const struct t23_sub_node *sub = &chan->sub[fmt->pad - 4];

		fmt->format.width = sub->pix.width;
		fmt->format.height = sub->pix.height;
		fmt->format.code = MEDIA_BUS_FMT_YUYV8_1X16;
		fmt->format.colorspace = V4L2_COLORSPACE_SMPTE170M;
		fmt->format.field = V4L2_FIELD_NONE;
		return 0;
	}
	if (fmt->pad >= 2)
		return -ENOTTY;
	fmt->format.width = chan->pix.width;
	fmt->format.height = chan->pix.height;
	if (fmt->pad == 0) {
		fmt->format.code = t23_chan_sink_code(chan);
		fmt->format.colorspace = V4L2_COLORSPACE_RAW;
	} else {
		if (is_raw10_pixfmt(chan->pix.pixelformat)) {
			fmt->format.code = t23_chan_sink_code(chan);
			fmt->format.colorspace = V4L2_COLORSPACE_RAW;
		} else if (chan->pix.pixelformat == V4L2_PIX_FMT_RGB24) {
			fmt->format.code = MEDIA_BUS_FMT_RGB888_1X24;
			fmt->format.colorspace = V4L2_COLORSPACE_SRGB;
		} else {
			fmt->format.code = MEDIA_BUS_FMT_YUYV8_1X16;
			fmt->format.colorspace = V4L2_COLORSPACE_SMPTE170M;
		}
	}
	fmt->format.field = V4L2_FIELD_NONE;
	return 0;
}

static int t23_isp_subdev_set_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *fmt)
{
	return t23_isp_subdev_get_fmt(sd, state, fmt);
}

static int t23_isp_subdev_enum_mbus_code(struct v4l2_subdev *sd,
					 struct v4l2_subdev_state *state,
					 struct v4l2_subdev_mbus_code_enum *code)
{
	static const u32 sink_codes[] = {
		MEDIA_BUS_FMT_SBGGR10_1X10,
		MEDIA_BUS_FMT_SRGGB10_1X10,
		MEDIA_BUS_FMT_SGBRG10_1X10,
		MEDIA_BUS_FMT_SGRBG10_1X10,
	};

	if (code->pad == 0) {
		if (code->index >= ARRAY_SIZE(sink_codes))
			return -EINVAL;
		code->code = sink_codes[code->index];
		return 0;
	}
	if (code->pad == 1) {
		static const u32 src_codes[] = {
			MEDIA_BUS_FMT_YUYV8_1X16,
			MEDIA_BUS_FMT_SBGGR10_1X10,
			MEDIA_BUS_FMT_SRGGB10_1X10,
			MEDIA_BUS_FMT_SGBRG10_1X10,
			MEDIA_BUS_FMT_SGRBG10_1X10,
			MEDIA_BUS_FMT_RGB888_1X24,
		};
		if (code->index >= ARRAY_SIZE(src_codes))
			return -EINVAL;
		code->code = src_codes[code->index];
		return 0;
	}
	if (code->pad >= 4 && code->pad < 4 + T23_SCALED_OUTPUTS) {
		if (code->index > 0)
			return -EINVAL;
		code->code = MEDIA_BUS_FMT_YUYV8_1X16;
		return 0;
	}
	return -ENOTTY;
}

static int t23_isp_sd_subscribe_event(struct v4l2_subdev *sd,
				      struct v4l2_fh *fh,
				      struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_MOTION_DET)
		return v4l2_event_subscribe(fh, sub, 8, NULL);
	return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
}

static const struct v4l2_subdev_core_ops t23_isp_sd_core_ops = {
	.subscribe_event = t23_isp_sd_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops t23_isp_sd_pad_ops = {
	.get_fmt = t23_isp_subdev_get_fmt,
	.set_fmt = t23_isp_subdev_set_fmt,
	.enum_mbus_code = t23_isp_subdev_enum_mbus_code,
};

static const struct v4l2_subdev_ops t23_isp_sd_ops = {
	.core = &t23_isp_sd_core_ops,
	.pad = &t23_isp_sd_pad_ops,
};

static int t23_mbus_to_pix(const struct v4l2_mbus_framefmt *fmt,
			   struct v4l2_pix_format *pix)
{
	u32 fourcc;

	switch (fmt->code) {
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		fourcc = V4L2_PIX_FMT_SBGGR10;
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		fourcc = V4L2_PIX_FMT_SRGGB10;
		break;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		fourcc = V4L2_PIX_FMT_SGBRG10;
		break;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		fourcc = V4L2_PIX_FMT_SGRBG10;
		break;
	default:
		return -EINVAL;
	}
	if (!fmt->width || !fmt->height || fmt->field != V4L2_FIELD_NONE)
		return -EINVAL;
	return t23_fill_pix_format(pix, fourcc, fmt->width, fmt->height);
}

static int t23_sensor_format(struct t23_isp_video *video, unsigned int input,
			     struct v4l2_pix_format *pix)
{
	struct v4l2_subdev_format fmt = { .which = V4L2_SUBDEV_FORMAT_ACTIVE };
	struct v4l2_subdev *sensor;
	int ret;

	if (video->disconnected)
		return -ENODEV;
	if (input >= video->num_endpoints || !video->inputs[input].sd)
		return -ENOLINK;
	sensor = video->inputs[input].sd;
	fmt.pad = video->inputs[input].pad;
	ret = v4l2_subdev_call_state_active(sensor, pad, get_fmt, &fmt);
	if (ret)
		return ret;
	video->inputs[input].mbus_code = fmt.format.code;
	return t23_mbus_to_pix(&fmt.format, pix);
}

/*
 * MSCA channel 1 downscales by up to 8 in each direction. Widths are whole
 * 16-pixel units and heights even, which keeps both planes 8-byte aligned.
 */
#define T23_SUB_MAX_DOWNSCALE	8

static void t23_sub_limits(u32 width, u32 height, struct v4l2_frmsize_stepwise *s)
{
	s->max_width = width & ~15U;
	s->max_height = height & ~1U;
	s->min_width = min(ALIGN(DIV_ROUND_UP(width, T23_SUB_MAX_DOWNSCALE), 16),
			   s->max_width);
	s->min_height = min(ALIGN(DIV_ROUND_UP(height, T23_SUB_MAX_DOWNSCALE), 2),
			    s->max_height);
	s->step_width = 16;
	s->step_height = 2;
}

static int t23_sub_format(struct t23_video_node *chan, u32 width, u32 height,
			  struct v4l2_pix_format *pix)
{
	struct v4l2_frmsize_stepwise limits;
	struct v4l2_pix_format sensor;
	int ret;

	ret = t23_sensor_format(chan->video, chan->input, &sensor);
	if (ret)
		return ret;
	t23_sub_limits(sensor.width, sensor.height, &limits);
	width = clamp(width, limits.min_width, limits.max_width) & ~15U;
	height = clamp(height, limits.min_height, limits.max_height) & ~1U;
	return t23_fill_pix_format(pix, V4L2_PIX_FMT_NV12, width, height);
}

/* Per-node state for the main (MSCA channel 0) or scaled (channels 1-2) outputs */
struct t23_output {
	struct vb2_queue *queue;
	struct list_head *bufs;
	struct media_pad *pad;
	bool *streaming;
	bool *stopping;
	unsigned int *skip_frames;
	u32 bit;
};

static unsigned int t23_queue_output_idx(struct t23_video_node *chan,
					 struct vb2_queue *q)
{
	unsigned int s;

	for (s = 0; s < T23_SCALED_OUTPUTS; s++)
		if (q == &chan->sub[s].queue)
			return s + 1;
	return 0;
}

static struct t23_output t23_output(struct t23_video_node *chan,
				    unsigned int idx)
{
	if (idx) {
		struct t23_sub_node *sub = &chan->sub[idx - 1];

		return (struct t23_output) {
			&sub->queue, &sub->bufs, &sub->pad,
			&sub->streaming, &sub->stopping, &sub->skip_frames,
			BIT(idx),
		};
	}
	return (struct t23_output) {
		&chan->queue, &chan->bufs, &chan->pad,
		&chan->streaming, &chan->stopping, &chan->skip_frames, BIT(0),
	};
}

/* Whether the frame in flight writes @out of @chan. */
static bool t23_output_busy(struct t23_video_node *chan,
			    const struct t23_output *out)
{
	struct t23_isp_video *video = chan->video;

	return READ_ONCE(video->dma_active) &&
	       READ_ONCE(video->active_channel) == chan->channel_idx &&
	       (READ_ONCE(video->active_outputs) & out->bit);
}

static void t23_return_output(struct t23_video_node *chan,
			      const struct t23_output *out,
			      enum vb2_buffer_state state)
{
	struct t23_isp_buffer *buf, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&chan->video->qlock, flags);
	list_for_each_entry_safe(buf, tmp, out->bufs, list) {
		list_del_init(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
	spin_unlock_irqrestore(&chan->video->qlock, flags);
}

static void t23_sensors_stop(struct t23_isp_video *video)
{
	while (video->num_streaming) {
		unsigned int i = video->stream_order[--video->num_streaming];
		struct t23_video_input *input = &video->inputs[i];
		int ret;

		ret = v4l2_subdev_call(input->sd, video, s_stream, 0);
		if (ret && ret != -ENOIOCTLCMD)
			dev_err(video->dev, "Sensor %s failed to stop: %d\n",
				input->sd->name, ret);
		input->initialized = false;
	}
}

static int t23_sensor_start(struct t23_isp_video *video, unsigned int index)
{
	struct v4l2_subdev *sd = video->inputs[index].sd;

	if (!sd)
		return -ENOLINK;
	/* Include a failed start in the reverse-order shutdown. */
	video->stream_order[video->num_streaming++] = index;
	video->inputs[index].initialized = true;
	return v4l2_subdev_call(sd, video, s_stream, 1);
}

static int t23_sensors_stream(struct t23_isp_video *video)
{
	unsigned int i;
	int ret;

	if (video->sync_source >= 0) {
		struct v4l2_subdev *source = video->inputs[video->sync_source].sd;
		s32 val;

		/* Sensor sub-device nodes can set a slave's length directly. */
		ret = t23_sensor_vblank(source, &val, false);
		if (!ret)
			ret = t23_set_frame_length(video, source, &val);
		if (ret)
			return ret;
		ret = t23_sensor_start(video, video->sync_source);
		if (ret)
			return ret;
	}
	for (i = 0; i < video->num_endpoints; i++) {
		if (i == video->sync_source || !video->inputs[i].sd)
			continue;
		ret = t23_sensor_start(video, i);
		if (ret)
			return ret;
	}
	return 0;
}

static void t23_pipeline_stop(struct t23_isp_video *video)
{
	struct tx_isp_device *isp = video->isp;

	if (!video->streaming)
		return;
	video->streaming = false;
	if (video->irq_enabled) {
		tx_isp_disable_irq(isp);
		video->irq_enabled = false;
	}
	t23_sensors_stop(video);
	if (isp->subdevs[TX_ISP_SUBDEV_VIC])
		tx_isp_vic_stop(isp->subdevs[TX_ISP_SUBDEV_VIC]);
	video->dma_active = false;
	if (isp->subdevs[TX_ISP_SUBDEV_CORE])
		tx_isp_core_s_stream(isp->subdevs[TX_ISP_SUBDEV_CORE], 0);
	if (isp->subdevs[TX_ISP_SUBDEV_CSI])
		csi_phy_stop(isp->subdevs[TX_ISP_SUBDEV_CSI]);
	isp->streaming = false;
	video->active_channel = -1;
}

static int t23_sensor_bus(struct v4l2_subdev *sd, unsigned int pad,
			  struct v4l2_mbus_config *bus, s64 *frequency)
{
	struct v4l2_ctrl *ctrl;
	int index, ret;

	memset(bus, 0, sizeof(*bus));
	ret = v4l2_subdev_call(sd, pad, get_mbus_config, pad, bus);
	if (ret)
		return ret;
	if (bus->type != V4L2_MBUS_CSI2_DPHY ||
	    bus->bus.mipi_csi2.num_data_lanes < 1 ||
	    bus->bus.mipi_csi2.num_data_lanes > 2)
		return -EINVAL;
	ctrl = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_LINK_FREQ);
	if (!ctrl || ctrl->type != V4L2_CTRL_TYPE_INTEGER_MENU)
		return -ENOLINK;
	index = v4l2_ctrl_g_ctrl(ctrl);
	if (index < ctrl->minimum || index > ctrl->maximum)
		return -EINVAL;
	*frequency = ctrl->qmenu_int[index];
	if (*frequency < 40000000 || *frequency >= 750000000)
		return -EINVAL;
	return 0;
}

/* Start the shared pipeline; @processed selects ISP output over raw VIC. */
static int t23_pipeline_start(struct t23_video_node *chan, bool processed)
{
	struct t23_isp_video *video = chan->video;
	struct tx_isp_device *isp = video->isp;
	struct v4l2_subdev *sensor = t23_sensor(chan);
	struct v4l2_pix_format pix;
	struct v4l2_mbus_config bus;
	u32 active_inputs = 0;
	s64 frequency;
	size_t dma_size;
	unsigned long flags;
	unsigned int i;
	int ret;

	if (video->disconnected || !video->registered || !isp)
		return -ENODEV;
	if (!sensor)
		return -ENOLINK;

	ret = t23_sensor_format(video, chan->input, &pix);
	if (ret)
		return ret;

	if (!is_raw10_pixfmt(pix.pixelformat) ||
	    (is_raw10_pixfmt(chan->pix.pixelformat) &&
	     chan->pix.pixelformat != pix.pixelformat) ||
	    pix.width != chan->pix.width ||
	    pix.height != chan->pix.height)
		return -EPIPE;

	ret = t23_sensor_bus(sensor, video->inputs[chan->input].pad, &bus,
			     &frequency);
	if (ret)
		return ret;
	for (i = 0; i < video->num_endpoints; i++) {
		struct v4l2_pix_format other;
		struct v4l2_mbus_config other_bus;
		s64 other_frequency;

		if (!video->inputs[i].sd)
			continue;
		active_inputs |= BIT(video->ep_asd[i]->port);
		ret = t23_sensor_format(video, i, &other);
		if (ret)
			return ret;
		if (other.width != pix.width || other.height != pix.height ||
		    other.pixelformat != pix.pixelformat)
			return -EPIPE;
		ret = t23_sensor_bus(video->inputs[i].sd,
				     video->inputs[i].pad, &other_bus,
				     &other_frequency);
		if (ret)
			return ret;
		if (other_bus.type != bus.type ||
		    other_bus.bus.mipi_csi2.num_data_lanes !=
		    bus.bus.mipi_csi2.num_data_lanes)
			return -EPIPE;
	}
	chan->csi_clock_mhz = div_s64(frequency * 2, 1000000);
	if (video->streaming) {
		if (isp->processed != processed)
			return -EBUSY;
		if (video->dma_failed)
			return -EIO;
		return t23_sync_frame_length(video);
	}
	isp->processed = processed;

	/* Only VIC capture bounces through a driver buffer. */
	dma_size = (size_t)pix.width * pix.height * 2;
	if (!isp->processed && (!isp->dma_buffer || dma_size > isp->dma_size)) {
		void *new_buf;
		dma_addr_t new_dma;

		new_buf = dma_alloc_coherent(video->dev, dma_size, &new_dma,
					     GFP_KERNEL);
		if (!new_buf)
			return -ENOMEM;
		if (isp->dma_buffer)
			dma_free_coherent(video->dev, isp->dma_size,
					  isp->dma_buffer, isp->dma_address);
		isp->dma_buffer = new_buf;
		isp->dma_address = new_dma;
		isp->dma_size = dma_size;
	}

	ret = t23_isp_configure_input(
		isp, sensor, video->ep_asd[chan->input]->port, active_inputs);
	if (ret)
		goto pipeline_off;

	ret = csi_phy_start(isp->subdevs[TX_ISP_SUBDEV_CSI]);
	if (ret)
		goto pipeline_off;

	ret = tx_isp_core_s_stream(isp->subdevs[TX_ISP_SUBDEV_CORE], 1);
	if (ret)
		goto core_off;

	/*
	 * In processed mode MSCA 0xd050 mirrors and flips the YUV outputs,
	 * so keep the sensor Bayer pattern unflipped; in raw mode forward
	 * each channel's flip state to its sensor.
	 */
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
		struct v4l2_subdev *s = t23_sensor(&video->channels[i]);
		struct v4l2_control hflip = {
			.id = V4L2_CID_HFLIP,
			.value = processed ? 0 : video->channels[i].hflip_val,
		};
		struct v4l2_control vflip = {
			.id = V4L2_CID_VFLIP,
			.value = processed ? 0 : video->channels[i].vflip_val,
		};

		if (!s || !s->ctrl_handler)
			continue;
		v4l2_s_ctrl(NULL, s->ctrl_handler, &hflip);
		v4l2_s_ctrl(NULL, s->ctrl_handler, &vflip);
	}

	/* VIC reset needs the selected sensor's input clock. */
	ret = t23_sensors_stream(video);
	if (ret)
		goto core_off;

	ret = tx_isp_vic_start(isp->subdevs[TX_ISP_SUBDEV_VIC]);
	if (ret)
		goto core_off;
	if (isp->processed) {
		ret = t23_processed_prime(video, chan->channel_idx);
		if (ret)
			goto core_off;
	}
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++)
		t23_motion_reset(&video->channels[i].motion);

	video->dma_failed = false;
	video->streaming = true;
	isp->streaming = true;
	spin_lock_irqsave(&video->qlock, flags);
	t23_capture_next(video);
	spin_unlock_irqrestore(&video->qlock, flags);

	tx_isp_enable_irq(isp);
	video->irq_enabled = true;

	return 0;

core_off:
	t23_sensors_stop(video);
	tx_isp_vic_capture_trigger(isp->subdevs[TX_ISP_SUBDEV_VIC], false);
	tx_isp_vic_stop(isp->subdevs[TX_ISP_SUBDEV_VIC]);
	tx_isp_core_s_stream(isp->subdevs[TX_ISP_SUBDEV_CORE], 0);
	csi_phy_stop(isp->subdevs[TX_ISP_SUBDEV_CSI]);
pipeline_off:
	t23_sensors_stop(video);
	return ret;
}

static void t23_disconnect_outputs(struct t23_video_node *chan)
{
	unsigned int i;

	for (i = 0; i < T23_MSCA_OUTPUTS; i++) {
		struct t23_output out = t23_output(chan, i);

		if (*out.streaming)
			media_pipeline_stop(out.pad);
		*out.streaming = false;
		vb2_queue_error(out.queue);
		t23_return_output(chan, &out, VB2_BUF_STATE_ERROR);
	}
}

static int t23_check_endpoint(const struct v4l2_fwnode_endpoint *ep)
{
	unsigned int lanes = ep->bus.mipi_csi2.num_data_lanes;
	unsigned int i;

	if (ep->base.port >= T23_MAX_INPUTS || ep->base.id ||
	    ep->bus_type != V4L2_MBUS_CSI2_DPHY || !lanes || lanes > 2 ||
	    ep->bus.mipi_csi2.clock_lane)
		return -EINVAL;
	for (i = 0; i < lanes; i++)
		if (ep->bus.mipi_csi2.data_lanes[i] != i + 1)
			return -EINVAL;
	for (i = 0; i <= lanes; i++)
		if (ep->bus.mipi_csi2.lane_polarities[i])
			return -EINVAL;
	return 0;
}

static int t23_bound(struct v4l2_async_notifier *notifier,
		     struct v4l2_subdev *sd, struct v4l2_async_connection *asc)
{
	struct t23_isp_video *video =
		container_of(notifier, struct t23_isp_video, notifier);
	struct t23_async_subdev *endpoint =
		container_of(asc, struct t23_async_subdev, asc);
	struct t23_video_node *chan = &video->channels[endpoint->port];
	struct v4l2_pix_format sensor_pix;
	unsigned int input = endpoint->index;
	unsigned int pad, s;
	int ret;

	if (input >= video->num_endpoints)
		return -EINVAL;
	for (pad = 0; pad < sd->entity.num_pads; pad++)
		if (sd->entity.pads[pad].flags & MEDIA_PAD_FL_SOURCE)
			break;
	if (pad == sd->entity.num_pads ||
	    sd->entity.function != MEDIA_ENT_F_CAM_SENSOR)
		return -EINVAL;
	mutex_lock(&video->lock);
	video->inputs[input].sd = sd;
	video->inputs[input].pad = pad;
	chan->input = input;
	ret = t23_sensor_format(video, input, &sensor_pix);
	if (ret)
		goto clear;
	/* Default capture format on MC capture nodes is hardware-ISP NV12. */
	ret = t23_fill_pix_format(&chan->pix, V4L2_PIX_FMT_NV12,
				  sensor_pix.width, sensor_pix.height);
	if (ret)
		goto clear;
	for (s = 0; s < T23_SCALED_OUTPUTS; s++) {
		ret = t23_sub_format(chan, chan->pix.width / 2,
				     chan->pix.height / 2, &chan->sub[s].pix);
		if (ret)
			goto clear;
	}
	t23_overlay_set_frame(&video->overlay, chan->channel_idx,
			      chan->pix.width, chan->pix.height);
	mutex_lock(&video->mdev.graph_mutex);
	ret = media_create_pad_link(&sd->entity, pad, &chan->isp_sd.entity, 0,
				    MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	mutex_unlock(&video->mdev.graph_mutex);
	if (!ret) {
		t23_sensor_vblank_watch(video, sd, true);
		t23_sync_frame_length(video);
		mutex_unlock(&video->lock);
		return 0;
	}
clear:
	chan->input = -1;
	memset(&video->inputs[input], 0, sizeof(video->inputs[input]));
	mutex_unlock(&video->lock);
	return ret;
}

static void t23_unbind(struct v4l2_async_notifier *notifier,
		       struct v4l2_subdev *sd, struct v4l2_async_connection *asc)
{
	struct t23_isp_video *video =
		container_of(notifier, struct t23_isp_video, notifier);
	unsigned int i;

	t23_sensor_vblank_watch(video, sd, false);
	cancel_work_sync(&video->vblank_sync_work);
	mutex_lock(&video->lock);
	t23_pipeline_stop(video);
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
		struct t23_video_node *chan = &video->channels[i];

		t23_disconnect_outputs(chan);
		if (t23_sensor(chan) == sd) {
			memset(&video->inputs[chan->input], 0,
			       sizeof(video->inputs[chan->input]));
			chan->input = -1;
			memset(&chan->pix, 0, sizeof(chan->pix));
		}
	}
	mutex_unlock(&video->lock);
}

static int t23_complete(struct v4l2_async_notifier *notifier)
{
	struct t23_isp_video *video =
		container_of(notifier, struct t23_isp_video, notifier);

	return v4l2_device_register_subdev_nodes(&video->v4l2_dev);
}

static const struct v4l2_async_notifier_operations t23_async_ops = {
	.bound = t23_bound,
	.complete = t23_complete,
	.unbind = t23_unbind,
};

static int t23_queue_setup(struct vb2_queue *q, unsigned int *count,
			   unsigned int *planes, unsigned int sizes[],
			   struct device *devs[])
{
	struct t23_video_node *chan = vb2_get_drv_priv(q);
	struct t23_isp_video *video = chan->video;
	unsigned int idx = t23_queue_output_idx(chan, q);
	unsigned int size = idx ? chan->sub[idx - 1].pix.sizeimage :
				  chan->pix.sizeimage;

	if (!t23_sensor(chan))
		return video->disconnected ? -ENODEV : -ENOLINK;
	if (!size)
		return -EINVAL;
	if (*planes)
		return *planes == 1 && sizes[0] >= size ? 0 : -EINVAL;
	*planes = 1;
	sizes[0] = size;
	*count = max_t(unsigned int, *count, 2);
	return 0;
}

static int t23_buf_prepare(struct vb2_buffer *vb)
{
	struct t23_video_node *chan = vb2_get_drv_priv(vb->vb2_queue);
	struct t23_isp_video *video = chan->video;
	unsigned int idx = t23_queue_output_idx(chan, vb->vb2_queue);
	unsigned int size = idx ? chan->sub[idx - 1].pix.sizeimage :
				  chan->pix.sizeimage;

	if (video->disconnected)
		return -ENODEV;
	if (vb2_plane_size(vb, 0) < size)
		return -EINVAL;
	/* Only the main node CPU-converted formats are written through a mapping. */
	if (!idx && chan->pix.pixelformat != V4L2_PIX_FMT_NV12 &&
	    !vb2_plane_vaddr(vb, 0))
		return -EFAULT;
	vb2_set_plane_payload(vb, 0, 0);
	return 0;
}

static void t23_buf_queue(struct vb2_buffer *vb)
{
	struct t23_video_node *chan = vb2_get_drv_priv(vb->vb2_queue);
	struct t23_isp_video *video = chan->video;
	struct t23_isp_buffer *buf =
		container_of(to_vb2_v4l2_buffer(vb), struct t23_isp_buffer, vb);
	unsigned int idx = t23_queue_output_idx(chan, vb->vb2_queue);
	struct list_head *bufs = idx ? &chan->sub[idx - 1].bufs : &chan->bufs;
	unsigned long flags;

	spin_lock_irqsave(&video->qlock, flags);
	list_add_tail(&buf->list, bufs);
	t23_capture_next(video);
	spin_unlock_irqrestore(&video->qlock, flags);
}

static int t23_output_start(struct t23_video_node *chan, unsigned int idx,
			    bool processed)
{
	struct t23_output out = t23_output(chan, idx);
	struct t23_isp_video *video = chan->video;
	unsigned long flags;
	int ret;

	if (IS_ERR(media_entity_remote_source_pad_unique(out.pad->entity))) {
		ret = -ENOLINK;
		goto buffers;
	}
	ret = media_pipeline_start(out.pad, &chan->pipe);
	if (ret)
		goto buffers;
	if (video->irq_enabled) {
		tx_isp_disable_irq(video->isp);
		video->irq_enabled = false;
	}
	spin_lock_irqsave(&video->qlock, flags);
	if (!t23_chan_streaming(chan))
		chan->sequence = 0;
	*out.skip_frames = 2;
	*out.stopping = false;
	*out.streaming = true;
	spin_unlock_irqrestore(&video->qlock, flags);
	ret = t23_pipeline_start(chan, processed);
	if (ret) {
		spin_lock_irqsave(&video->qlock, flags);
		*out.streaming = false;
		spin_unlock_irqrestore(&video->qlock, flags);
		media_pipeline_stop(out.pad);
		if (video->streaming) {
			tx_isp_enable_irq(video->isp);
			video->irq_enabled = true;
		}
		goto buffers;
	}
	spin_lock_irqsave(&video->qlock, flags);
	t23_capture_next(video);
	spin_unlock_irqrestore(&video->qlock, flags);
	if (!video->irq_enabled) {
		tx_isp_enable_irq(video->isp);
		video->irq_enabled = true;
	}
	return 0;
buffers:
	t23_return_output(chan, &out, VB2_BUF_STATE_QUEUED);
	return ret;
}

static void t23_output_stop(struct t23_video_node *chan, unsigned int idx)
{
	struct t23_output out = t23_output(chan, idx);
	struct t23_isp_video *video = chan->video;
	unsigned long flags;
	unsigned int i, s;
	bool running = false, other_running = false, complete_parked = false;

	spin_lock_irqsave(&video->qlock, flags);
	*out.stopping = true;
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
		const struct t23_video_node *c = &video->channels[i];

		if (c->streaming && !(i == chan->channel_idx && idx == 0))
			other_running = true;
		for (s = 0; s < T23_SCALED_OUTPUTS; s++)
			if (c->sub[s].streaming &&
			    !(i == chan->channel_idx && idx == s + 1))
				other_running = true;
	}
	spin_unlock_irqrestore(&video->qlock, flags);

	/* Bounded 2-frame drain (~200 ms) so an in-flight frame finishes cleanly. */
	wait_event_timeout(video->dma_wait, !t23_output_busy(chan, &out),
			   msecs_to_jiffies(200));

	/* Drain the threaded completion before returning its destination buffer. */
	if (video->irq_enabled) {
		tx_isp_disable_irq(video->isp);
		video->irq_enabled = false;
	}
	if (video->isp && video->isp->processed &&
	    t23_output_busy(chan, &out) && !other_running)
		t23_isp_video_processed_event(video->isp, false, false, true);
	spin_lock_irqsave(&video->qlock, flags);
	if (video->isp && t23_output_busy(chan, &out)) {
		if (video->isp->processed && other_running) {
			struct tx_isp_subdev *core_sd =
				video->isp->subdevs[TX_ISP_SUBDEV_CORE];
			struct tx_isp_core_device *core =
				core_sd ? core_sd->dev_priv : NULL;

			/*
			 * Park this single MSCA channel onto the 4 KiB scratch
			 * page (stride=0) without disturbing active streams.
			 */
			tx_isp_core_park_channel(core_sd, idx,
						 video->isp->scratch_dma);
			video->active_outputs &= ~out.bit;
			if (!video->active_outputs ||
			    (core && core->armed_mask &&
			     core->done_mask == core->armed_mask)) {
				video->processed_dma_done = true;
				if (video->processed_all_done)
					complete_parked = true;
			}
		} else {
			tx_isp_vic_dma_stop(video->isp->subdevs[TX_ISP_SUBDEV_VIC]);
			video->dma_active = false;
			video->active_channel = -1;
		}
	}
	if (*out.streaming) {
		*out.streaming = false;
		spin_unlock_irqrestore(&video->qlock, flags);
		media_pipeline_stop(out.pad);
		spin_lock_irqsave(&video->qlock, flags);
	}
	if (complete_parked) {
		spin_unlock_irqrestore(&video->qlock, flags);
		t23_isp_video_processed_event(video->isp, true, true, false);
		spin_lock_irqsave(&video->qlock, flags);
	}
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++)
		running |= t23_chan_streaming(&video->channels[i]);
	if (running)
		t23_capture_next(video);
	spin_unlock_irqrestore(&video->qlock, flags);
	if (running) {
		tx_isp_enable_irq(video->isp);
		video->irq_enabled = true;
	} else if (video->isp) {
		t23_pipeline_stop(video);
	}
	t23_return_output(chan, &out, VB2_BUF_STATE_ERROR);
}

static int t23_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct t23_video_node *chan = vb2_get_drv_priv(q);
	unsigned int idx = t23_queue_output_idx(chan, q);
	bool processed = idx || (chan->pix.pixelformat == V4L2_PIX_FMT_NV12);

	return t23_output_start(chan, idx, processed);
}

static void t23_stop_streaming(struct vb2_queue *q)
{
	struct t23_video_node *chan = vb2_get_drv_priv(q);
	unsigned int idx = t23_queue_output_idx(chan, q);

	t23_output_stop(chan, idx);
}

static const struct vb2_ops t23_vb2_ops = {
	.queue_setup = t23_queue_setup,
	.buf_prepare = t23_buf_prepare,
	.buf_queue = t23_buf_queue,
	.start_streaming = t23_start_streaming,
	.stop_streaming = t23_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int t23_sub_querycap(struct file *file, void *priv,
			    struct v4l2_capability *cap)
{
	struct t23_sub_node *sub =
		container_of(video_devdata(file), struct t23_sub_node, vdev);
	struct t23_video_node *chan = video_drvdata(file);

	strscpy(cap->driver, "tx-isp", sizeof(cap->driver));
	if (sub->index)
		snprintf(cap->card, sizeof(cap->card), "T23 scaled ch%u.%u",
			 chan->channel_idx, sub->index);
	else
		snprintf(cap->card, sizeof(cap->card), "T23 scaled ch%u",
			 chan->channel_idx);
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(chan->video->dev));
	return 0;
}

static int t23_sub_enum_fmt(struct file *file, void *priv,
			    struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;
	f->pixelformat = V4L2_PIX_FMT_NV12;
	strscpy(f->description, "Y/CbCr 4:2:0", sizeof(f->description));
	return 0;
}

static int t23_sub_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct t23_sub_node *sub =
		container_of(video_devdata(file), struct t23_sub_node, vdev);

	f->fmt.pix = sub->pix;
	return 0;
}

static int t23_sub_try_fmt(struct file *file, void *priv,
			   struct v4l2_format *f)
{
	struct t23_video_node *chan = video_drvdata(file);

	return t23_sub_format(chan, f->fmt.pix.width, f->fmt.pix.height,
			      &f->fmt.pix);
}

static int t23_sub_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct t23_sub_node *sub =
		container_of(video_devdata(file), struct t23_sub_node, vdev);
	int ret;

	if (vb2_is_busy(&sub->queue))
		return -EBUSY;
	ret = t23_sub_try_fmt(file, priv, f);
	if (!ret)
		sub->pix = f->fmt.pix;
	return ret;
}

static int t23_sub_enum_framesizes(struct file *file, void *priv,
				   struct v4l2_frmsizeenum *f)
{
	struct t23_video_node *chan = video_drvdata(file);
	struct v4l2_pix_format sensor;

	if (f->index || f->pixel_format != V4L2_PIX_FMT_NV12 ||
	    t23_sensor_format(chan->video, chan->input, &sensor))
		return -EINVAL;
	f->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	t23_sub_limits(sensor.width, sensor.height, &f->stepwise);
	return 0;
}

static int t23_frame_interval(struct t23_video_node *chan,
			      struct v4l2_frmivalenum *f)
{
	struct v4l2_subdev_frame_interval interval = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	struct v4l2_subdev *sensor = t23_sensor(chan);
	int ret;

	if (!sensor)
		return -EINVAL;
	interval.pad = chan->video->inputs[chan->input].pad;
	ret = v4l2_subdev_call_state_active(sensor, pad, get_frame_interval,
					    &interval);
	if (ret)
		return ret;
	f->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	f->discrete = interval.interval;
	return 0;
}

static int t23_sub_enum_frameintervals(struct file *file, void *priv,
				       struct v4l2_frmivalenum *f)
{
	struct t23_video_node *chan = video_drvdata(file);
	struct v4l2_pix_format pix;

	if (f->index || f->pixel_format != V4L2_PIX_FMT_NV12 ||
	    t23_sub_format(chan, f->width, f->height, &pix) ||
	    pix.width != f->width || pix.height != f->height)
		return -EINVAL;
	return t23_frame_interval(chan, f);
}

static int t23_subscribe_event(struct v4l2_fh *fh,
			       const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_MOTION_DET)
		return v4l2_event_subscribe(fh, sub, 8, NULL);
	return v4l2_ctrl_subscribe_event(fh, sub);
}

static int t23_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	struct video_device *vdev = video_devdata(file);
	struct t23_video_node *chan = video_get_drvdata(vdev);
	struct t23_isp_video *video = chan->video;

	strscpy(cap->driver, "tx-isp", sizeof(cap->driver));
	snprintf(cap->card, sizeof(cap->card), "T23 capture ch%u", chan->channel_idx);
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(video->dev));
	return 0;
}

static int t23_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(t23_formats))
		return -EINVAL;
	f->pixelformat = t23_formats[f->index].pixelformat;
	strscpy(f->description, t23_formats[f->index].description, sizeof(f->description));
	return 0;
}

static int t23_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct video_device *vdev = video_devdata(file);
	struct t23_video_node *chan = video_get_drvdata(vdev);

	f->fmt.pix = chan->pix;
	return 0;
}

static int t23_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct t23_video_node *chan = video_get_drvdata(video_devdata(file));
	struct t23_isp_video *video = chan->video;
	struct v4l2_pix_format sensor_pix;
	u32 pixelformat;
	int ret;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	ret = t23_sensor_format(video, chan->input, &sensor_pix);
	if (ret)
		return ret;

	pixelformat = f->fmt.pix.pixelformat;
	if (!is_supported_format(pixelformat))
		pixelformat = V4L2_PIX_FMT_NV12;
	else if (is_raw10_pixfmt(pixelformat))
		pixelformat = sensor_pix.pixelformat;

	return t23_fill_pix_format(&f->fmt.pix, pixelformat, sensor_pix.width, sensor_pix.height);
}

static int t23_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct t23_video_node *chan = video_get_drvdata(video_devdata(file));
	int ret;

	if (vb2_is_busy(&chan->queue))
		return -EBUSY;
	ret = t23_try_fmt(file, priv, f);
	if (ret)
		return ret;
	chan->pix = f->fmt.pix;
	return 0;
}

static int t23_enum_framesizes(struct file *file, void *priv,
			       struct v4l2_frmsizeenum *f)
{
	struct video_device *vdev = video_devdata(file);
	struct t23_video_node *chan = video_get_drvdata(vdev);

	if (f->index || !t23_sensor(chan) ||
	    !is_supported_format(f->pixel_format))
		return -EINVAL;
	f->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	f->discrete.width = chan->pix.width;
	f->discrete.height = chan->pix.height;
	return 0;
}

static int t23_enum_frameintervals(struct file *file, void *priv,
				   struct v4l2_frmivalenum *f)
{
	struct t23_video_node *chan = video_drvdata(file);

	if (f->index || !is_supported_format(f->pixel_format) ||
	    f->width != chan->pix.width || f->height != chan->pix.height)
		return -EINVAL;
	return t23_frame_interval(chan, f);
}

static int t23_g_parm(struct file *file, void *priv, struct v4l2_streamparm *a)
{
	struct t23_video_node *chan = video_drvdata(file);
	struct v4l2_subdev_frame_interval interval = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	struct v4l2_subdev *sensor = t23_sensor(chan);
	struct v4l2_fract tpf = { 1, 15 };

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (sensor) {
		interval.pad = chan->video->inputs[chan->input].pad;
		if (!v4l2_subdev_call_state_active(sensor, pad,
						   get_frame_interval,
						   &interval) &&
		    interval.interval.numerator &&
		    interval.interval.denominator)
			tpf = interval.interval;
	}

	memset(&a->parm.capture, 0, sizeof(a->parm.capture));
	a->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.capture.timeperframe = tpf;
	return 0;
}

static int t23_s_parm(struct file *file, void *priv, struct v4l2_streamparm *a)
{
	struct t23_video_node *chan = video_drvdata(file);
	struct v4l2_subdev *sensor = t23_sensor(chan);

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (sensor && a->parm.capture.timeperframe.numerator &&
	    a->parm.capture.timeperframe.denominator) {
		struct v4l2_subdev_frame_interval interval = {
			.which = V4L2_SUBDEV_FORMAT_ACTIVE,
			.pad = chan->video->inputs[chan->input].pad,
			.interval = a->parm.capture.timeperframe,
		};
		int ret;

		ret = v4l2_subdev_call_state_active(sensor, pad,
						    set_frame_interval,
						    &interval);
		if (ret && ret != -ENOIOCTLCMD)
			return ret;
	}

	return t23_g_parm(file, priv, a);
}

static const struct v4l2_ioctl_ops t23_ioctl_ops = {
	.vidioc_querycap = t23_querycap,
	.vidioc_enum_fmt_vid_cap = t23_enum_fmt,
	.vidioc_g_fmt_vid_cap = t23_g_fmt,
	.vidioc_s_fmt_vid_cap = t23_s_fmt,
	.vidioc_try_fmt_vid_cap = t23_try_fmt,
	.vidioc_enum_framesizes = t23_enum_framesizes,
	.vidioc_enum_frameintervals = t23_enum_frameintervals,
	.vidioc_g_parm = t23_g_parm,
	.vidioc_s_parm = t23_s_parm,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_subscribe_event = t23_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct v4l2_ioctl_ops t23_sub_ioctl_ops = {
	.vidioc_querycap = t23_sub_querycap,
	.vidioc_enum_fmt_vid_cap = t23_sub_enum_fmt,
	.vidioc_g_fmt_vid_cap = t23_sub_g_fmt,
	.vidioc_s_fmt_vid_cap = t23_sub_s_fmt,
	.vidioc_try_fmt_vid_cap = t23_sub_try_fmt,
	.vidioc_enum_framesizes = t23_sub_enum_framesizes,
	.vidioc_enum_frameintervals = t23_sub_enum_frameintervals,
	.vidioc_g_parm = t23_g_parm,
	.vidioc_s_parm = t23_s_parm,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_subscribe_event = t23_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct v4l2_file_operations t23_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

static void t23_video_release(struct v4l2_device *v4l2_dev)
{
	struct t23_isp_video *video =
		container_of(v4l2_dev, struct t23_isp_video, v4l2_dev);
	unsigned int ch, s;

	for (ch = 0; ch < T23_CAPTURE_CHANNELS; ch++) {
		struct t23_video_node *chan = &video->channels[ch];

		for (s = 0; s < T23_SCALED_OUTPUTS; s++) {
			if (chan->sub[s].queue_initialized)
				vb2_queue_release(&chan->sub[s].queue);
			if (chan->sub[s].entity_initialized)
				media_entity_cleanup(&chan->sub[s].vdev.entity);
		}
		if (chan->queue_initialized)
			vb2_queue_release(&chan->queue);
		if (chan->entity_initialized)
			media_entity_cleanup(&chan->vdev.entity);
		if (chan->isp_sd_registered) {
			v4l2_device_unregister_subdev(&chan->isp_sd);
			chan->isp_sd_registered = false;
		}
		if (chan->isp_sd_finalized) {
			v4l2_subdev_cleanup(&chan->isp_sd);
			media_entity_cleanup(&chan->isp_sd.entity);
			chan->isp_sd_finalized = false;
		}
		v4l2_ctrl_handler_free(&chan->controls);
	}
	t23_meta_release(&video->meta);
	t23_overlay_release(&video->overlay);
	media_device_cleanup(&video->mdev);
	put_device(video->dev);
	kfree(video);
}

static void t23_media_release(struct media_devnode *node)
{
	struct t23_isp_video *video = dev_get_drvdata(&node->dev);

	if (video->media_release)
		video->media_release(node);
	v4l2_device_put(&video->v4l2_dev);
}

/* The scaled nodes follow the main node's pipeline. */
static int t23_sub_register(struct t23_video_node *chan, unsigned int idx)
{
	struct t23_isp_video *video = chan->video;
	struct t23_sub_node *sub = &chan->sub[idx];
	struct vb2_queue *q = &sub->queue;
	int ret;

	sub->index = idx;
	INIT_LIST_HEAD(&sub->bufs);
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = chan;
	q->ops = &t23_vb2_ops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->buf_struct_size = sizeof(struct t23_isp_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_queued_buffers = 2;
	q->lock = &video->lock;
	q->dev = video->dev;
	ret = vb2_queue_init(q);
	if (ret)
		return ret;
	sub->queue_initialized = true;

	sub->vdev = (struct video_device){
		.fops = &t23_fops,
		.ioctl_ops = &t23_sub_ioctl_ops,
		.release = video_device_release_empty,
		.v4l2_dev = &video->v4l2_dev,
		.queue = q,
		.lock = &video->lock,
		.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
			       V4L2_CAP_IO_MC,
	};
	if (idx)
		snprintf(sub->vdev.name, sizeof(sub->vdev.name),
			 "T23 scaled%u.%u", chan->channel_idx, idx);
	else
		snprintf(sub->vdev.name, sizeof(sub->vdev.name),
			 "T23 scaled%u", chan->channel_idx);
	sub->vdev.entity.function = MEDIA_ENT_F_IO_V4L;
	sub->pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&sub->vdev.entity, 1, &sub->pad);
	if (ret)
		return ret;
	sub->entity_initialized = true;
	video_set_drvdata(&sub->vdev, chan);
	ret = video_register_device(&sub->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		return ret;
	sub->vdev_registered = true;
	return media_create_pad_link(&chan->isp_sd.entity, 4 + idx,
				     &sub->vdev.entity, 0,
				     MEDIA_LNK_FL_ENABLED);
}

static bool t23_overlay_sync(struct t23_overlay *overlay, unsigned int input)
{
	struct t23_isp_video *video =
		container_of(overlay, struct t23_isp_video, overlay);

	if (!wait_event_timeout(video->dma_wait,
				!READ_ONCE(video->dma_active) ||
				READ_ONCE(video->active_channel) != input,
				msecs_to_jiffies(500))) {
		WRITE_ONCE(video->dma_failed, true);
		return false;
	}
	return true;
}

int t23_isp_video_register(struct tx_isp_device *isp)
{
	struct t23_isp_video *video;
	struct media_entity *isp_entities[T23_CAPTURE_CHANNELS];
	struct fwnode_handle *ep_node;
	unsigned int i, j;
	u32 sync_port;
	int ret;

	mutex_lock(&t23_video_lock);
	if (t23_video) {
		ret = -EBUSY;
		goto unlock;
	}
	video = kzalloc(sizeof(*video), GFP_KERNEL);
	if (!video) {
		ret = -ENOMEM;
		goto unlock;
	}
	video->dev = get_device(&isp->pdev->dev);
	video->isp = isp;
	video->active_channel = -1;
	spin_lock_init(&video->qlock);
	init_waitqueue_head(&video->dma_wait);
	video->sync_source = -1;
	mutex_init(&video->lock);
	mutex_init(&video->sync_lock);
	INIT_WORK(&video->vblank_sync_work, t23_vblank_sync_work);
	atomic_set(&video->vblank_syncing, 0);
	t23_overlay_init(&video->overlay, video->dev, t23_overlay_sync);

	video->mdev.dev = video->dev;
	strscpy(video->mdev.model, "T23 capture", sizeof(video->mdev.model));
	media_device_init(&video->mdev);
	ret = v4l2_device_register(video->dev, &video->v4l2_dev);
	if (ret) {
		media_device_cleanup(&video->mdev);
		put_device(video->dev);
		kfree(video);
		goto unlock;
	}
	video->v4l2_dev.release = t23_video_release;
	video->v4l2_dev.mdev = &video->mdev;

	for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
		struct t23_video_node *chan = &video->channels[i];
		struct vb2_queue *q = &chan->queue;

		chan->video = video;
		chan->channel_idx = i;
		chan->input = -1;
		mutex_init(&chan->isp_sd_lock);

		v4l2_ctrl_handler_init(&chan->controls, 24);
		chan->controls.lock = &chan->isp_sd_lock;
		if (chan->controls.error) {
			ret = chan->controls.error;
			goto fail_nodes;
		}

		/* ISP native controls (exposed exclusively on t23-isp-coreN) */
		chan->hflip = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
		chan->vflip = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
		chan->dgain = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_DIGITAL_GAIN, 256, 16383, 1, 1024);
		chan->red_balance = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_RED_BALANCE, 0, 1024, 1, 256);
		chan->blue_balance = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_BLUE_BALANCE, 0, 1024, 1, 256);
		chan->brightness = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_BRIGHTNESS, -128, 127, 1, 0);
		chan->contrast = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_CONTRAST, 0, 255, 1, 128);
		chan->saturation = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_SATURATION, 0, 255, 1, 128);
		chan->sharpness = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_SHARPNESS, 0, 255, 1, 128);
		chan->gamma_ctrl = v4l2_ctrl_new_std(&chan->controls, &t23_control_ops,
			V4L2_CID_GAMMA, 70, 180, 1, 100);
		t23_motion_init(&chan->motion, &chan->controls, &chan->isp_sd,
				&chan->vdev, &chan->sub[0].vdev,
				&chan->sub[1].vdev);
		t23_overlay_init_controls(&video->overlay, i, &chan->controls);

		if (chan->controls.error) {
			ret = chan->controls.error;
			goto fail_nodes;
		}

		chan->dgain_val = 1024;
		chan->red_val = 256;
		chan->blue_val = 256;
		chan->brightness_val = 0;
		chan->contrast_val = 128;
		chan->saturation_val = 128;
		chan->sharpness_val = 128;
		chan->gamma_val = 100;
		t23_update_gamma_lut(chan);

		INIT_LIST_HEAD(&chan->bufs);

		q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		q->io_modes = VB2_MMAP | VB2_DMABUF;
		q->drv_priv = chan;
		q->ops = &t23_vb2_ops;
		q->mem_ops = &vb2_dma_contig_memops;
		q->buf_struct_size = sizeof(struct t23_isp_buffer);
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
		q->min_queued_buffers = 2;
		q->lock = &video->lock;
		q->dev = video->dev;
		ret = vb2_queue_init(q);
		if (ret)
			goto fail_nodes;
		chan->queue_initialized = true;

		chan->vdev = (struct video_device){
			.fops = &t23_fops,
			.ioctl_ops = &t23_ioctl_ops,
			.release = video_device_release_empty,
			.v4l2_dev = &video->v4l2_dev,
			.queue = q,
			.lock = &video->lock,
			.device_caps = V4L2_CAP_VIDEO_CAPTURE |
				       V4L2_CAP_STREAMING | V4L2_CAP_IO_MC,
		};
		snprintf(chan->vdev.name, sizeof(chan->vdev.name), "T23 video%u", i);
		chan->vdev.entity.function = MEDIA_ENT_F_IO_V4L;
		chan->pad.flags = MEDIA_PAD_FL_SINK;
		ret = media_entity_pads_init(&chan->vdev.entity, 1, &chan->pad);
		if (ret)
			goto fail_nodes;
		chan->entity_initialized = true;
		video_set_drvdata(&chan->vdev, chan);

		ret = video_register_device(&chan->vdev, VFL_TYPE_VIDEO, -1);
		if (ret)
			goto fail_nodes;
		chan->vdev_registered = true;

		/* Initialize per-channel ISP subdev */
		v4l2_subdev_init(&chan->isp_sd, &t23_isp_sd_ops);
		snprintf(chan->isp_sd.name, sizeof(chan->isp_sd.name), "t23-isp-core%u", i);
		chan->isp_sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
				      V4L2_SUBDEV_FL_HAS_EVENTS;
		chan->isp_sd.state_lock = &chan->isp_sd_lock;
		chan->isp_sd.ctrl_handler = &chan->controls;
		chan->isp_sd.entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
		chan->isp_pads[0].flags = MEDIA_PAD_FL_SINK;
		chan->isp_pads[1].flags = MEDIA_PAD_FL_SOURCE;
		chan->isp_pads[2].flags = MEDIA_PAD_FL_SINK;
		chan->isp_pads[3].flags = MEDIA_PAD_FL_SOURCE;
		for (j = 0; j < T23_SCALED_OUTPUTS; j++)
			chan->isp_pads[4 + j].flags = MEDIA_PAD_FL_SOURCE;
		for (j = 4 + T23_SCALED_OUTPUTS; j < ARRAY_SIZE(chan->isp_pads); j++)
			chan->isp_pads[j].flags = MEDIA_PAD_FL_SINK;
		ret = media_entity_pads_init(&chan->isp_sd.entity,
					     ARRAY_SIZE(chan->isp_pads),
					     chan->isp_pads);
		if (ret)
			goto fail_nodes;
		v4l2_set_subdevdata(&chan->isp_sd, chan);
		ret = v4l2_subdev_init_finalize(&chan->isp_sd);
		if (ret) {
			media_entity_cleanup(&chan->isp_sd.entity);
			goto fail_nodes;
		}
		chan->isp_sd_finalized = true;
		ret = v4l2_device_register_subdev(&video->v4l2_dev, &chan->isp_sd);
		if (ret)
			goto fail_nodes;
		chan->isp_sd_registered = true;

		/* Link chan->isp_sd pad 1 (SOURCE) to chan->vdev.entity pad 0 (SINK) */
		ret = media_create_pad_link(&chan->isp_sd.entity, 1,
					    &chan->vdev.entity, 0,
					    MEDIA_LNK_FL_ENABLED);
		if (ret)
			goto fail_nodes;
		isp_entities[i] = &chan->isp_sd.entity;
	}
	BUILD_BUG_ON(T23_META_INPUTS != T23_CAPTURE_CHANNELS);
	video->meta.dev = video->dev;
	ret = t23_meta_register(&video->meta, &video->v4l2_dev, &video->lock,
				isp_entities, 2, 3);
	if (ret)
		goto fail_nodes;
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
		for (j = 0; j < T23_SCALED_OUTPUTS; j++) {
			ret = t23_sub_register(&video->channels[i], j);
			if (ret)
				goto fail_nodes;
		}
	}
	BUILD_BUG_ON(T23_OVERLAY_INPUTS != T23_CAPTURE_CHANNELS);
	ret = t23_overlay_register(&video->overlay, &video->v4l2_dev,
				   &video->lock, isp_entities,
				   4 + T23_SCALED_OUTPUTS);
	if (ret)
		goto fail_nodes;

	v4l2_async_nf_init(&video->notifier, &video->v4l2_dev);
	video->notifier.ops = &t23_async_ops;
	fwnode_graph_for_each_endpoint(dev_fwnode(video->dev), ep_node) {
		struct v4l2_fwnode_endpoint ep = {
			.bus_type = V4L2_MBUS_CSI2_DPHY,
		};
		struct t23_async_subdev *t23_asd;

		ret = v4l2_fwnode_endpoint_parse(ep_node, &ep);
		if (ret) {
			fwnode_handle_put(ep_node);
			goto notifier;
		}
		ret = t23_check_endpoint(&ep);
		if (ret) {
			fwnode_handle_put(ep_node);
			goto notifier;
		}
		for (j = 0; j < video->num_endpoints; j++) {
			if (video->ep_asd[j]->port == ep.base.port) {
				fwnode_handle_put(ep_node);
				ret = -EINVAL;
				goto notifier;
			}
		}
		if (video->num_endpoints >= T23_MAX_INPUTS) {
			fwnode_handle_put(ep_node);
			ret = -EINVAL;
			goto notifier;
		}
		t23_asd = v4l2_async_nf_add_fwnode_remote(&video->notifier,
							  ep_node,
							  struct t23_async_subdev);
		if (IS_ERR(t23_asd)) {
			fwnode_handle_put(ep_node);
			ret = PTR_ERR(t23_asd);
			goto notifier;
		}
		t23_asd->port = ep.base.port;
		t23_asd->index = video->num_endpoints;
		video->ep_asd[video->num_endpoints++] = t23_asd;
	}
	if (device_property_present(video->dev, "ingenic,frame-sync-source-port")) {
		ret = device_property_read_u32(video->dev,
				"ingenic,frame-sync-source-port", &sync_port);
		if (ret)
			goto notifier;
		for (i = 0; i < video->num_endpoints; i++) {
			if (video->ep_asd[i]->port == sync_port) {
				video->sync_source = i;
				break;
			}
		}
		if (video->sync_source < 0) {
			ret = -EINVAL;
			goto notifier;
		}
	}
	ret = v4l2_async_nf_register(&video->notifier);
	if (ret)
		goto notifier;
	ret = media_device_register(&video->mdev);
	if (ret)
		goto async;
	video->media_registered = true;
	v4l2_device_get(&video->v4l2_dev);
	dev_set_drvdata(&video->mdev.devnode->dev, video);
	video->media_release = video->mdev.devnode->release;
	video->mdev.devnode->release = t23_media_release;
	mutex_lock(&video->lock);
	isp->video_priv = video;
	video->registered = true;
	mutex_unlock(&video->lock);
	t23_video = video;
	mutex_unlock(&t23_video_lock);
	return 0;

async:
	v4l2_async_nf_unregister(&video->notifier);
	cancel_work_sync(&video->vblank_sync_work);
notifier:
	v4l2_async_nf_cleanup(&video->notifier);
fail_nodes:
	t23_overlay_unregister(&video->overlay);
	t23_meta_unregister(&video->meta);
	for (i = 0; i < T23_CAPTURE_CHANNELS; i++) {
		struct t23_video_node *chan = &video->channels[i];

		for (j = 0; j < T23_SCALED_OUTPUTS; j++)
			if (chan->sub[j].vdev_registered)
				video_unregister_device(&chan->sub[j].vdev);
		if (chan->vdev_registered)
			video_unregister_device(&chan->vdev);
		if (chan->isp_sd_registered) {
			v4l2_device_unregister_subdev(&chan->isp_sd);
			chan->isp_sd_registered = false;
		}
		if (chan->isp_sd_finalized) {
			v4l2_subdev_cleanup(&chan->isp_sd);
			media_entity_cleanup(&chan->isp_sd.entity);
			chan->isp_sd_finalized = false;
		}
	}
	v4l2_device_unregister(&video->v4l2_dev);
	v4l2_device_put(&video->v4l2_dev);
unlock:
	mutex_unlock(&t23_video_lock);
	return ret;
}

void t23_isp_video_unregister(void)
{
	struct t23_isp_video *video;
	unsigned int ch, s;

	mutex_lock(&t23_video_lock);
	video = t23_video;
	t23_video = NULL;
	mutex_unlock(&t23_video_lock);
	if (!video)
		return;

	mutex_lock(&video->lock);
	video->registered = false;
	t23_pipeline_stop(video);
	for (ch = 0; ch < T23_CAPTURE_CHANNELS; ch++) {
		struct t23_video_node *chan = &video->channels[ch];

		t23_disconnect_outputs(chan);
	}
	video->disconnected = true;
	video->isp = NULL;
	mutex_unlock(&video->lock);

	v4l2_async_nf_unregister(&video->notifier);
	for (ch = 0; ch < video->num_endpoints; ch++)
		if (video->inputs[ch].sd)
			t23_sensor_vblank_watch(video, video->inputs[ch].sd, false);
	cancel_work_sync(&video->vblank_sync_work);
	v4l2_async_nf_cleanup(&video->notifier);

	t23_overlay_unregister(&video->overlay);
	t23_meta_unregister(&video->meta);
	for (ch = 0; ch < T23_CAPTURE_CHANNELS; ch++) {
		struct t23_video_node *chan = &video->channels[ch];

		for (s = 0; s < T23_SCALED_OUTPUTS; s++) {
			if (chan->sub[s].vdev_registered) {
				video_unregister_device(&chan->sub[s].vdev);
				chan->sub[s].vdev_registered = false;
			}
		}
		if (chan->vdev_registered) {
			video_unregister_device(&chan->vdev);
			chan->vdev_registered = false;
		}
		if (chan->isp_sd_registered) {
			v4l2_device_unregister_subdev(&chan->isp_sd);
			chan->isp_sd_registered = false;
		}
		if (chan->isp_sd_finalized) {
			v4l2_subdev_cleanup(&chan->isp_sd);
			media_entity_cleanup(&chan->isp_sd.entity);
			chan->isp_sd_finalized = false;
		}
	}

	if (video->media_registered)
		media_device_unregister(&video->mdev);
	v4l2_device_unregister(&video->v4l2_dev);
	v4l2_device_put(&video->v4l2_dev);
}


