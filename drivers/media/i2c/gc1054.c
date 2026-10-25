// SPDX-License-Identifier: GPL-2.0
/*
 * GalaxyCore GC1054 MIPI CSI-2 camera sensor driver
 *
 * 1280x720 RAW10, one data lane, 24 MHz input clock. The frame length is
 * set with V4L2_CID_VBLANK; 25 frames/s by default, 30 frames/s at the
 * shortest frame.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kref.h>
#include <media/fh8626-subdev.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "gc1054-modes.h"

#define GC1054_CHIP_ID_H		0xf0
#define GC1054_CHIP_ID_L		0xf1
#define GC1054_CHIP_ID			0x1054

#define GC1054_PAGE_SELECT		0xfe

/* Page 3 MIPI control */
#define GC1054_MIPI_CTRL		0x10
#define GC1054_MIPI_STREAM_ON		0x90

#define GC1054_WIDTH			1280
#define GC1054_HEIGHT			720

/*
 * A line is 2 * (HB + 344) periods of the 38.8 MHz timing clock; the mode
 * table sets HB = 522, giving 1732 periods (about 44.6 us). The clock is
 * accurate to about 1%.
 */
#define GC1054_LINE_CLOCKS		1732
#define GC1054_PIXEL_RATE		38800000
#define GC1054_HBLANK			(GC1054_LINE_CLOCKS - GC1054_WIDTH)

/*
 * A frame is the image height plus VB plus 16 lines, so VBLANK = VB + 16.
 * VB is 12 bits wide.
 */
#define GC1054_VB_EXTRA			16
#define GC1054_VB_MIN			10
#define GC1054_VB_MAX			0x0fff
#define GC1054_VBLANK_MIN		(GC1054_VB_MIN + GC1054_VB_EXTRA)
#define GC1054_VBLANK_MAX		(GC1054_VB_MAX + GC1054_VB_EXTRA)
/* 896 lines: 25 frames/s */
#define GC1054_VBLANK_DEFAULT		176

#define GC1054_EXPOSURE_MARGIN		5
#define GC1054_EXPOSURE_MIN		16
#define GC1054_EXPOSURE_DEFAULT		500

struct gc1054_interval {
	struct v4l2_fract interval;
	u32 vblank;
};

static const struct gc1054_interval gc1054_intervals[] = {
	{ { 1, 30 }, 26 },	/* 746 lines */
	{ { 1, 25 }, 176 },	/* 896 lines */
	{ { 2, 25 }, 1072 },	/* 1792 lines */
};

/* Analog gain brackets: PGA selection and bracket base */
struct gc1054_gain_range {
	u16 max_gain;
	u8 b6_val;
	u16 base;
};

static const struct gc1054_gain_range gc1054_gain_table[] = {
	{ 0x005a, 0, 0x0040 },
	{ 0x007e, 1, 0x005b },
	{ 0x00b5, 2, 0x007f },
	{ 0x0100, 3, 0x00b6 },
	{ 0x0170, 4, 0x0101 },
	{ 0x0202, 5, 0x0171 },
	{ 0x02e0, 6, 0x0203 },
	{ 0x0406, 7, 0x02e1 },
	{ 0x05d2, 8, 0x0407 },
	{ 0x0823, 9, 0x05d3 },
};

struct gc1054_dev {
	struct kref		ref;
	struct fh8626_subdev_lifetime lifetime;
	struct i2c_client	*client;
	struct v4l2_subdev	sd;
	struct media_pad	pad;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl	*exposure;
	struct v4l2_ctrl	*gain;
	struct v4l2_ctrl	*vblank;
	struct v4l2_ctrl	*hflip;
	struct v4l2_ctrl	*vflip;
	struct clk		*xvclk;
	struct gpio_desc	*reset_gpio;
	struct gpio_desc	*pwdn_gpio;
	struct mutex		lock;
	bool			powered;
	bool			streaming;
	bool			removed;
	struct v4l2_mbus_framefmt fmt;
};

static inline struct gc1054_dev *to_gc1054(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc1054_dev, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct gc1054_dev, ctrls)->sd;
}

static void gc1054_release(struct kref *ref)
{
	struct gc1054_dev *sensor = container_of(ref, struct gc1054_dev, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int gc1054_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_gc1054(sd)->ref);
	return 0;
}

static void gc1054_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_gc1054(sd)->ref, gc1054_release);
}

static const struct v4l2_subdev_internal_ops gc1054_internal_ops = {
	.registered = gc1054_registered,
	.unregistered = gc1054_subdev_release,
};

static u32 gc1054_bayer_code(struct gc1054_dev *sensor)
{
	/* Unflipped output is RGGB; index is hflip | (vflip << 1) */
	static const u32 codes[] = {
		MEDIA_BUS_FMT_SRGGB10_1X10, MEDIA_BUS_FMT_SGRBG10_1X10,
		MEDIA_BUS_FMT_SGBRG10_1X10, MEDIA_BUS_FMT_SBGGR10_1X10,
	};

	return codes[sensor->hflip->val | (sensor->vflip->val << 1)];
}

static int gc1054_read_reg(struct i2c_client *client, u8 reg, u8 *val)
{
	int ret;
	struct i2c_msg msgs[2];

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = val;

	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret != 2) {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&client->dev, "failed to read reg 0x%02x: %d\n",
			reg, ret);
		return ret;
	}
	return 0;
}

static int gc1054_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	u8 buf[2] = { reg, val };

	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&client->dev, "failed to write reg 0x%02x=0x%02x: %d\n",
			reg, val, ret);
		return ret;
	}
	return 0;
}

static int gc1054_set_exposure(struct gc1054_dev *sensor, s32 val)
{
	int ret;

	/* Write the exposure high and low bytes. */
	ret = gc1054_write_reg(sensor->client, GC1054_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc1054_write_reg(sensor->client, 0x03, (val >> 8) & 0xff);
	if (ret)
		return ret;

	return gc1054_write_reg(sensor->client, 0x04, val & 0xff);
}

/* Vertical blanking uses a 12-bit value. */
static int gc1054_set_vblank(struct gc1054_dev *sensor, s32 vblank)
{
	u32 vb = vblank - GC1054_VB_EXTRA;
	int ret;

	ret = gc1054_write_reg(sensor->client, GC1054_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc1054_write_reg(sensor->client, 0x07, (vb >> 8) & 0x0f);
	if (ret)
		return ret;

	return gc1054_write_reg(sensor->client, 0x08, vb & 0xff);
}

static int gc1054_set_gain(struct gc1054_dev *sensor, s32 gain)
{
	struct i2c_client *client = sensor->client;
	int i, ret, restore_ret;
	u8 b6 = 0;
	u16 base = 0x40;
	u32 q;

	for (i = 0; i < ARRAY_SIZE(gc1054_gain_table); i++) {
		if (gain <= gc1054_gain_table[i].max_gain) {
			b6 = gc1054_gain_table[i].b6_val;
			base = gc1054_gain_table[i].base;
			break;
		}
	}

	q = (gain * 64) / base;

	/* The analog gain registers are on page 1 */
	ret = gc1054_write_reg(client, GC1054_PAGE_SELECT, 0x01);
	if (ret)
		return ret;

	ret = gc1054_write_reg(client, 0xb6, b6);
	if (ret)
		goto out;

	ret = gc1054_write_reg(client, 0xb1, (q >> 6) & 0x3f);
	if (ret)
		goto out;

	ret = gc1054_write_reg(client, 0xb2, (q & 0x3f) << 2);

out:
	restore_ret = gc1054_write_reg(client, GC1054_PAGE_SELECT, 0x00);
	return ret ? ret : restore_ret;
}

/*
 * Update the horizontal mirror and vertical flip settings.
 * The upper six bits belong to the mode table and are preserved.
 */
static int gc1054_set_flip(struct gc1054_dev *sensor, s32 hflip, s32 vflip)
{
	struct i2c_client *client = sensor->client;
	u8 val;
	int ret;

	ret = gc1054_write_reg(client, GC1054_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc1054_read_reg(client, 0x17, &val);
	if (ret)
		return ret;

	val &= 0xfc;
	if (hflip)
		val |= 0x01;
	if (vflip)
		val |= 0x02;

	return gc1054_write_reg(client, 0x17, val);
}

static int gc1054_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct gc1054_dev *sensor = to_gc1054(sd);
	int ret;

	if (sensor->removed)
		return -ENODEV;

	if (ctrl->id == V4L2_CID_VBLANK) {
		s32 max = GC1054_HEIGHT + ctrl->val - GC1054_EXPOSURE_MARGIN;

		ret = __v4l2_ctrl_modify_range(sensor->exposure,
					       sensor->exposure->minimum, max,
					       sensor->exposure->step,
					       min(max,
						   GC1054_EXPOSURE_DEFAULT));
		if (ret)
			return ret;
	}

	if (sensor->streaming &&
	    (ctrl->id == V4L2_CID_HFLIP || ctrl->id == V4L2_CID_VFLIP))
		return -EBUSY;

	if (!sensor->streaming)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return gc1054_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return gc1054_set_gain(sensor, ctrl->val);
	case V4L2_CID_VBLANK:
		return gc1054_set_vblank(sensor, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops gc1054_ctrl_ops = {
	.s_ctrl = gc1054_s_ctrl,
};

static int gc1054_init_controls(struct gc1054_dev *sensor)
{
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *hblank, *pixel_rate;
	int ret;

	v4l2_ctrl_handler_init(hdl, 7);
	hdl->lock = &sensor->lock;

	pixel_rate = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_PIXEL_RATE,
				       GC1054_PIXEL_RATE, GC1054_PIXEL_RATE, 1,
				       GC1054_PIXEL_RATE);
	if (pixel_rate)
		pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	hblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_HBLANK,
				   GC1054_HBLANK, GC1054_HBLANK, 1,
				   GC1054_HBLANK);
	if (hblank)
		hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->vblank = v4l2_ctrl_new_std(hdl, &gc1054_ctrl_ops,
					   V4L2_CID_VBLANK, GC1054_VBLANK_MIN,
					   GC1054_VBLANK_MAX, 1,
					   GC1054_VBLANK_DEFAULT);

	sensor->exposure = v4l2_ctrl_new_std(hdl, &gc1054_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     GC1054_EXPOSURE_MIN,
					     GC1054_HEIGHT +
					     GC1054_VBLANK_DEFAULT -
					     GC1054_EXPOSURE_MARGIN, 1,
					     GC1054_EXPOSURE_DEFAULT);

	/* Gain in Q6 scale (64 = 1.0x) */
	sensor->gain = v4l2_ctrl_new_std(hdl, &gc1054_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 64, 0x823, 1, 64);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &gc1054_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);

	sensor->vflip = v4l2_ctrl_new_std(hdl, &gc1054_ctrl_ops,
					  V4L2_CID_VFLIP, 0, 1, 1, 0);

	if (hdl->error) {
		ret = hdl->error;
		dev_err(&sensor->client->dev,
			"failed to initialize controls: %d\n", ret);
		v4l2_ctrl_handler_free(hdl);
		return ret;
	}

	sensor->sd.ctrl_handler = hdl;
	return 0;
}

static int gc1054_power_on(struct gc1054_dev *sensor)
{
	int ret;

	if (sensor->powered)
		return 0;

	if (sensor->xvclk) {
		ret = clk_prepare_enable(sensor->xvclk);
		if (ret)
			return ret;
	}

	if (sensor->pwdn_gpio)
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 0);

	if (sensor->reset_gpio) {
		gpiod_set_value_cansleep(sensor->reset_gpio, 1);
		usleep_range(1000, 2000);
		gpiod_set_value_cansleep(sensor->reset_gpio, 0);
		usleep_range(5000, 10000);
	}

	sensor->powered = true;
	return 0;
}

static void gc1054_power_off(struct gc1054_dev *sensor)
{
	if (!sensor->powered)
		return;

	if (sensor->reset_gpio)
		gpiod_set_value_cansleep(sensor->reset_gpio, 1);

	if (sensor->pwdn_gpio)
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 1);

	if (sensor->xvclk)
		clk_disable_unprepare(sensor->xvclk);
	sensor->powered = false;
	sensor->streaming = false;
}

static int gc1054_start_streaming(struct gc1054_dev *sensor)
{
	int i, ret;

	ret = gc1054_power_on(sensor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(gc1054_720p_regs); i++) {
		ret = gc1054_write_reg(sensor->client,
				       gc1054_720p_regs[i].reg,
				       gc1054_720p_regs[i].val);
		if (ret)
			goto err_stop;
		usleep_range(1000, 2000);
	}

	ret = gc1054_set_vblank(sensor, sensor->vblank->val);
	if (ret)
		goto err_stop;

	ret = gc1054_set_exposure(sensor, sensor->exposure->val);
	if (ret)
		goto err_stop;

	ret = gc1054_set_gain(sensor, sensor->gain->val);
	if (ret)
		goto err_stop;

	ret = gc1054_set_flip(sensor,
			      sensor->hflip->val, sensor->vflip->val);
	if (ret)
		goto err_stop;

	ret = gc1054_write_reg(sensor->client, GC1054_PAGE_SELECT, 0x03);
	if (ret)
		goto err_stop;
	ret = gc1054_write_reg(sensor->client, GC1054_MIPI_CTRL,
			       GC1054_MIPI_STREAM_ON);
	if (ret)
		goto err_stop;
	ret = gc1054_write_reg(sensor->client, GC1054_PAGE_SELECT, 0x00);
	if (ret)
		goto err_stop;

	sensor->streaming = true;
	return 0;

err_stop:
	gc1054_power_off(sensor);
	return ret;
}

/*
 * There is no standby register: stopping powers the sensor off and the next
 * start reloads the complete mode table.
 */
static int gc1054_stop_streaming(struct gc1054_dev *sensor)
{
	gc1054_power_off(sensor);
	return 0;
}

static int gc1054_s_stream(struct v4l2_subdev *sd, int on)
{
	struct gc1054_dev *sensor = to_gc1054(sd);
	int ret = 0;

	on = !!on;
	mutex_lock(&sensor->lock);
	if (sensor->removed) {
		ret = -ENODEV;
		goto unlock;
	}
	if (sensor->streaming == on)
		goto unlock;

	if (on)
		ret = gc1054_start_streaming(sensor);
	else
		ret = gc1054_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void gc1054_frame_interval(s32 vblank, struct v4l2_fract *interval)
{
	unsigned long n, d, g;
	int i;

	for (i = 0; i < ARRAY_SIZE(gc1054_intervals); i++) {
		if (gc1054_intervals[i].vblank == vblank) {
			*interval = gc1054_intervals[i].interval;
			return;
		}
	}

	n = GC1054_LINE_CLOCKS * (GC1054_HEIGHT + vblank);
	d = GC1054_PIXEL_RATE;
	g = gcd(n, d);
	interval->numerator = n / g;
	interval->denominator = d / g;
}

static int gc1054_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct gc1054_dev *sensor = to_gc1054(sd);

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	gc1054_frame_interval(sensor->vblank->val, &interval->interval);
	mutex_unlock(&sensor->lock);

	return 0;
}

static int gc1054_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct gc1054_dev *sensor = to_gc1054(sd);
	const struct gc1054_interval *best = &gc1054_intervals[1];
	struct v4l2_fract *req = &interval->interval;
	u64 best_diff = U64_MAX, req_us;
	int i, ret;

	if (interval->pad != 0)
		return -EINVAL;

	if (req->numerator && req->denominator) {
		req_us = div_u64((u64)req->numerator * USEC_PER_SEC,
				 req->denominator);
		for (i = 0; i < ARRAY_SIZE(gc1054_intervals); i++) {
			const struct v4l2_fract *f =
				&gc1054_intervals[i].interval;
			u64 us = div_u64((u64)f->numerator * USEC_PER_SEC,
					 f->denominator);
			u64 diff = us > req_us ? us - req_us : req_us - us;

			if (diff < best_diff) {
				best_diff = diff;
				best = &gc1054_intervals[i];
			}
		}
	}

	mutex_lock(&sensor->lock);
	ret = __v4l2_ctrl_s_ctrl(sensor->vblank, best->vblank);
	gc1054_frame_interval(sensor->vblank->val, &interval->interval);
	mutex_unlock(&sensor->lock);

	return ret;
}

static void gc1054_fill_format(struct gc1054_dev *sensor,
			       struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = GC1054_WIDTH,
		.height = GC1054_HEIGHT,
		.code = gc1054_bayer_code(sensor),
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int gc1054_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct gc1054_dev *sensor = to_gc1054(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	} else {
		fmt->format = sensor->fmt;
		fmt->format.code = gc1054_bayer_code(sensor);
	}
	mutex_unlock(&sensor->lock);

	return 0;
}

static int gc1054_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct gc1054_dev *sensor = to_gc1054(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}

	gc1054_fill_format(sensor, &fmt->format);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int gc1054_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct gc1054_dev *sensor = to_gc1054(sd);

	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	code->code = gc1054_bayer_code(sensor);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc1054_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct gc1054_dev *sensor = to_gc1054(sd);
	int ret = 0;

	if (fse->pad != 0 || fse->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fse->code != gc1054_bayer_code(sensor)) {
		ret = -EINVAL;
	} else {
		fse->min_width = GC1054_WIDTH;
		fse->max_width = GC1054_WIDTH;
		fse->min_height = GC1054_HEIGHT;
		fse->max_height = GC1054_HEIGHT;
	}
	mutex_unlock(&sensor->lock);
	return ret;
}

static int
gc1054_enum_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_interval_enum *fie)
{
	struct gc1054_dev *sensor = to_gc1054(sd);

	if (fie->pad != 0 || fie->index >= ARRAY_SIZE(gc1054_intervals))
		return -EINVAL;

	if (fie->width != GC1054_WIDTH || fie->height != GC1054_HEIGHT)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fie->code != gc1054_bayer_code(sensor)) {
		mutex_unlock(&sensor->lock);
		return -EINVAL;
	}
	fie->interval = gc1054_intervals[fie->index].interval;
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc1054_init_cfg(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return gc1054_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops gc1054_video_ops = {
	.s_stream = gc1054_s_stream,
	.g_frame_interval = gc1054_g_frame_interval,
	.s_frame_interval = gc1054_s_frame_interval,
};

static const struct v4l2_subdev_core_ops gc1054_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops gc1054_pad_ops = {
	.init_cfg = gc1054_init_cfg,
	.enum_mbus_code = gc1054_enum_mbus_code,
	.enum_frame_size = gc1054_enum_frame_size,
	.enum_frame_interval = gc1054_enum_frame_interval,
	.get_fmt = gc1054_get_fmt,
	.set_fmt = gc1054_set_fmt,
};

static const struct v4l2_subdev_ops gc1054_subdev_ops = {
	.core = &gc1054_core_ops,
	.video = &gc1054_video_ops,
	.pad = &gc1054_pad_ops,
};

static int gc1054_check_chip_id(struct gc1054_dev *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = gc1054_write_reg(sensor->client, GC1054_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc1054_read_reg(sensor->client, GC1054_CHIP_ID_H, &id_h);
	if (ret)
		return ret;

	ret = gc1054_read_reg(sensor->client, GC1054_CHIP_ID_L, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != GC1054_CHIP_ID) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}

	return 0;
}

static int gc1054_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct gc1054_dev *sensor;
	struct device_node *endpoint;
	struct v4l2_fwnode_endpoint bus;
	int ret;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint)
		return -EINVAL;
	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint), &bus);
	of_node_put(endpoint);
	if (ret)
		return ret;
	if (bus.bus_type != V4L2_MBUS_CSI2 ||
	    bus.bus.mipi_csi2.num_data_lanes != 1 ||
	    bus.bus.mipi_csi2.data_lanes[0] != 1 ||
	    bus.bus.mipi_csi2.clock_lane != 0 ||
	    bus.bus.mipi_csi2.lane_polarities[0] ||
	    bus.bus.mipi_csi2.lane_polarities[1])
		return -EINVAL;

	sensor = kzalloc(sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->client = client;
	kref_init(&sensor->ref);
	sensor->lifetime.ref = &sensor->ref;
	sensor->lifetime.release = gc1054_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &gc1054_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &gc1054_internal_ops;

	sensor->xvclk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(sensor->xvclk)) {
		ret = PTR_ERR(sensor->xvclk);
		goto err_put;
	}
	if (clk_get_rate(sensor->xvclk) != 24000000) {
		dev_err(dev, "24 MHz xvclk required\n");
		ret = -EINVAL;
		goto err_put;
	}

	sensor->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->reset_gpio)) {
		ret = PTR_ERR(sensor->reset_gpio);
		goto err_put;
	}

	sensor->pwdn_gpio = devm_gpiod_get_optional(dev, "pwdn",
						    GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->pwdn_gpio)) {
		ret = PTR_ERR(sensor->pwdn_gpio);
		goto err_put;
	}

	ret = gc1054_power_on(sensor);
	if (ret)
		goto err_put;

	ret = gc1054_check_chip_id(sensor);
	if (ret)
		goto err_power;

	gc1054_power_off(sensor);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;

	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_power;

	ret = gc1054_init_controls(sensor);
	if (ret)
		goto err_power;
	gc1054_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		v4l2_async_unregister_subdev(&sensor->sd);
		goto err_power;
	}

	return 0;

err_power:
	gc1054_power_off(sensor);
err_put:
	kref_put(&sensor->ref, gc1054_release);
	return ret;
}

static int gc1054_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc1054_dev *sensor = to_gc1054(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	gc1054_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, gc1054_release);
	return 0;
}

static const struct of_device_id gc1054_of_match[] = {
	{ .compatible = "galaxycore,gc1054" },
	{ }
};
MODULE_DEVICE_TABLE(of, gc1054_of_match);

static const struct i2c_device_id gc1054_id[] = {
	{ "gc1054", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc1054_id);

static struct i2c_driver gc1054_i2c_driver = {
	.driver = {
		.name = "gc1054",
		.of_match_table = gc1054_of_match,
	},
	.probe		= gc1054_probe,
	.remove		= gc1054_remove,
	.id_table	= gc1054_id,
};
module_i2c_driver(gc1054_i2c_driver);

MODULE_DESCRIPTION("GalaxyCore GC1054 MIPI CSI-2 camera sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
