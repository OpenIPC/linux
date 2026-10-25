// SPDX-License-Identifier: GPL-2.0
/*
 * GalaxyCore GC2053 2-lane MIPI CSI-2 camera sensor driver
 *
 * 1920x1080 RAW10 at 25 frames/s, 24 MHz input clock.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kref.h>
#include <media/fh8626-subdev.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "gc2053-modes.h"

#define GC2053_CHIP_ID_H		0xf0
#define GC2053_CHIP_ID_L		0xf1
#define GC2053_CHIP_ID			0x2053

#define GC2053_PAGE_SELECT		0xfe
#define GC2053_REG_MODE_SELECT		0x3e
#define GC2053_MODE_SW_STANDBY		0x11
#define GC2053_MODE_STREAMING		0x91

#define GC2053_WIDTH			1920
#define GC2053_HEIGHT			1080
#define GC2053_FRAME_LINES		1125

/* 2640 clocks per line and 1125 lines at 74.25 MHz give 25 fps */
#define GC2053_LINE_CLOCKS		2640
#define GC2053_PIXEL_RATE		74250000
#define GC2053_HBLANK			(GC2053_LINE_CLOCKS - GC2053_WIDTH)
#define GC2053_VBLANK			(GC2053_FRAME_LINES - GC2053_HEIGHT)
#define GC2053_EXPOSURE_MARGIN		5

struct gc2053_mode {
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u16 vts;
};

static const struct gc2053_mode gc2053_modes[] = {
	{
		.width = GC2053_WIDTH,
		.height = GC2053_HEIGHT,
		.max_fps = { .numerator = 1, .denominator = 25 },
		.vts = GC2053_FRAME_LINES,
	},
};

/* Analog gain brackets, selected by the lowest gain of each bracket */
struct gc2053_gain_range {
	u16 base_gain;
	u8 b4;
	u8 b3;
	u8 b8;
	u8 b9;
};

static const struct gc2053_gain_range gc2053_gain_table[] = {
	{ 0x0040, 0x00, 0x00, 0x01, 0x00 },
	{ 0x004a, 0x00, 0x10, 0x01, 0x0c },
	{ 0x0059, 0x00, 0x20, 0x01, 0x1b },
	{ 0x0066, 0x00, 0x30, 0x01, 0x2c },
	{ 0x007f, 0x00, 0x40, 0x01, 0x3f },
	{ 0x0093, 0x00, 0x50, 0x02, 0x16 },
	{ 0x00b1, 0x00, 0x60, 0x02, 0x35 },
	{ 0x00cb, 0x00, 0x70, 0x03, 0x16 },
	{ 0x0104, 0x00, 0x80, 0x04, 0x02 },
	{ 0x012c, 0x00, 0x90, 0x04, 0x31 },
	{ 0x0169, 0x00, 0xa0, 0x05, 0x32 },
	{ 0x019f, 0x00, 0xb0, 0x06, 0x35 },
	{ 0x01f8, 0x00, 0xc0, 0x08, 0x04 },
	{ 0x0245, 0x00, 0x5a, 0x09, 0x19 },
	{ 0x02d2, 0x00, 0x83, 0x0b, 0x0f },
	{ 0x0340, 0x00, 0x93, 0x0d, 0x12 },
	{ 0x0403, 0x00, 0x84, 0x10, 0x00 },
	{ 0x049e, 0x00, 0x94, 0x12, 0x3a },
	{ 0x0580, 0x01, 0x2c, 0x1a, 0x02 },
	{ 0x0655, 0x01, 0x3c, 0x1b, 0x20 },
	{ 0x07c6, 0x00, 0x8c, 0x20, 0x0f },
	{ 0x08f3, 0x00, 0x9c, 0x26, 0x07 },
	{ 0x0b22, 0x02, 0x64, 0x36, 0x21 },
	{ 0x0cd2, 0x02, 0x74, 0x37, 0x3a },
	{ 0x0fd0, 0x00, 0xc6, 0x3d, 0x02 },
	{ 0x143c, 0x00, 0xdc, 0x3f, 0x3f },
	{ 0x157c, 0x02, 0x85, 0x3f, 0x3f },
	{ 0x1a58, 0x02, 0x95, 0x3f, 0x3f },
	{ 0x1ba1, 0x00, 0xce, 0x3f, 0x3f },
};

struct gc2053_dev {
	struct kref		ref;
	struct fh8626_subdev_lifetime lifetime;
	struct i2c_client	*client;
	struct v4l2_subdev	sd;
	struct media_pad	pad;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl	*exposure;
	struct v4l2_ctrl	*gain;
	struct v4l2_ctrl	*hflip;
	struct v4l2_ctrl	*vflip;
	struct clk		*xvclk;
	struct gpio_desc	*reset_gpio;
	struct gpio_desc	*pwdn_gpio;
	struct mutex		lock;
	bool			powered;
	bool			streaming;
	bool			removed;
	const struct gc2053_mode *cur_mode;
	struct v4l2_mbus_framefmt fmt;
};

static inline struct gc2053_dev *to_gc2053(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc2053_dev, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct gc2053_dev, ctrls)->sd;
}

static void gc2053_release(struct kref *ref)
{
	struct gc2053_dev *sensor = container_of(ref, struct gc2053_dev, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int gc2053_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_gc2053(sd)->ref);
	return 0;
}

static void gc2053_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_gc2053(sd)->ref, gc2053_release);
}

static const struct v4l2_subdev_internal_ops gc2053_internal_ops = {
	.registered = gc2053_registered,
	.unregistered = gc2053_subdev_release,
};

static u32 gc2053_bayer_code(struct gc2053_dev *sensor)
{
	/* The sensor keeps the RGGB phase for every mirror/flip combination */
	return MEDIA_BUS_FMT_SRGGB10_1X10;
}

static int gc2053_read_reg(struct i2c_client *client, u8 reg, u8 *val)
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

static int gc2053_write_reg(struct i2c_client *client, u8 reg, u8 val)
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

static int gc2053_set_vts(struct gc2053_dev *sensor, u16 vts)
{
	int ret;

	ret = gc2053_write_reg(sensor->client, GC2053_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	/* Write the frame length high and low bytes. */
	ret = gc2053_write_reg(sensor->client, 0x41, (vts >> 8) & 0xff);
	if (ret)
		return ret;

	return gc2053_write_reg(sensor->client, 0x42, vts & 0xff);
}

static int gc2053_set_exposure(struct gc2053_dev *sensor, s32 val)
{
	int ret;

	/* Write the exposure high and low bytes. */
	ret = gc2053_write_reg(sensor->client, GC2053_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc2053_write_reg(sensor->client, 0x03, (val >> 8) & 0xff);
	if (ret)
		return ret;

	return gc2053_write_reg(sensor->client, 0x04, val & 0xff);
}

static int gc2053_set_gain(struct gc2053_dev *sensor, s32 gain)
{
	struct i2c_client *client = sensor->client;
	int i, idx, ret;
	u32 base, q;
	u8 b1, b2;

	idx = 0;
	for (i = ARRAY_SIZE(gc2053_gain_table) - 1; i >= 0; i--) {
		if (gain >= gc2053_gain_table[i].base_gain) {
			idx = i;
			break;
		}
	}

	base = gc2053_gain_table[idx].base_gain;
	q = (gain * 64) / base;
	if ((q >> 6) < 0x10) {
		b1 = (q >> 6) & 0x0f;
		b2 = (q & 0x3f) << 2;
	} else {
		b1 = 0x0f;
		b2 = 0xfc;
	}

	ret = gc2053_write_reg(client, GC2053_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc2053_write_reg(client, 0xb4, gc2053_gain_table[idx].b4);
	if (ret)
		return ret;

	ret = gc2053_write_reg(client, 0xb3, gc2053_gain_table[idx].b3);
	if (ret)
		return ret;

	ret = gc2053_write_reg(client, 0xb8, gc2053_gain_table[idx].b8);
	if (ret)
		return ret;

	ret = gc2053_write_reg(client, 0xb9, gc2053_gain_table[idx].b9);
	if (ret)
		return ret;

	ret = gc2053_write_reg(client, 0xb1, b1);
	if (ret)
		return ret;

	return gc2053_write_reg(client, 0xb2, b2);
}

/*
 * Update the horizontal mirror and vertical flip settings,
 * preserving the other bits.
 */
static int gc2053_set_flip(struct gc2053_dev *sensor, s32 hflip, s32 vflip)
{
	struct i2c_client *client = sensor->client;
	u8 val;
	int ret;

	ret = gc2053_write_reg(client, GC2053_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc2053_read_reg(client, 0x17, &val);
	if (ret)
		return ret;

	val &= 0xfc;
	if (hflip)
		val |= 0x01;
	if (vflip)
		val |= 0x02;

	return gc2053_write_reg(client, 0x17, val);
}

static int gc2053_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct gc2053_dev *sensor = to_gc2053(sd);

	if (sensor->removed)
		return -ENODEV;

	if (sensor->streaming &&
	    (ctrl->id == V4L2_CID_HFLIP || ctrl->id == V4L2_CID_VFLIP))
		return -EBUSY;

	if (!sensor->streaming)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return gc2053_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return gc2053_set_gain(sensor, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops gc2053_ctrl_ops = {
	.s_ctrl = gc2053_s_ctrl,
};

static int gc2053_init_controls(struct gc2053_dev *sensor)
{
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *vblank, *hblank, *pixel_rate;
	int ret;

	v4l2_ctrl_handler_init(hdl, 7);
	hdl->lock = &sensor->lock;

	pixel_rate = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_PIXEL_RATE,
				       GC2053_PIXEL_RATE, GC2053_PIXEL_RATE, 1,
				       GC2053_PIXEL_RATE);
	if (pixel_rate)
		pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	hblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_HBLANK,
				   GC2053_HBLANK, GC2053_HBLANK, 1,
				   GC2053_HBLANK);
	if (hblank)
		hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	vblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_VBLANK,
				   GC2053_VBLANK, GC2053_VBLANK, 1,
				   GC2053_VBLANK);
	if (vblank)
		vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->exposure = v4l2_ctrl_new_std(hdl, &gc2053_ctrl_ops,
					     V4L2_CID_EXPOSURE, 1,
					     GC2053_FRAME_LINES -
					     GC2053_EXPOSURE_MARGIN, 1, 500);

	/* Gain in Q6 scale (64 = 1.0x) */
	sensor->gain = v4l2_ctrl_new_std(hdl, &gc2053_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 64, 0x1ba1, 1, 64);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &gc2053_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);

	sensor->vflip = v4l2_ctrl_new_std(hdl, &gc2053_ctrl_ops,
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

static int gc2053_power_on(struct gc2053_dev *sensor)
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

static void gc2053_power_off(struct gc2053_dev *sensor)
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

static int gc2053_start_streaming(struct gc2053_dev *sensor)
{
	int i, ret;

	ret = gc2053_power_on(sensor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(gc2053_1080p_regs); i++) {
		ret = gc2053_write_reg(sensor->client,
				       gc2053_1080p_regs[i].reg,
				       gc2053_1080p_regs[i].val);
		if (ret)
			goto err_stop;
		usleep_range(1000, 2000);
	}

	ret = gc2053_set_vts(sensor, sensor->cur_mode->vts);
	if (ret)
		goto err_stop;

	ret = gc2053_set_exposure(sensor, sensor->exposure->val);
	if (ret)
		goto err_stop;

	ret = gc2053_set_gain(sensor, sensor->gain->val);
	if (ret)
		goto err_stop;

	ret = gc2053_set_flip(sensor,
			      sensor->hflip->val, sensor->vflip->val);
	if (ret)
		goto err_stop;

	ret = gc2053_write_reg(sensor->client, GC2053_PAGE_SELECT, 0x00);
	if (ret)
		goto err_stop;
	ret = gc2053_write_reg(sensor->client, GC2053_REG_MODE_SELECT,
			       GC2053_MODE_STREAMING);
	if (ret)
		goto err_stop;

	sensor->streaming = true;
	return 0;

err_stop:
	gc2053_power_off(sensor);
	return ret;
}

static int gc2053_stop_streaming(struct gc2053_dev *sensor)
{
	gc2053_write_reg(sensor->client, GC2053_PAGE_SELECT, 0x00);
	gc2053_write_reg(sensor->client, GC2053_REG_MODE_SELECT,
			 GC2053_MODE_SW_STANDBY);
	gc2053_power_off(sensor);
	return 0;
}

static int gc2053_s_stream(struct v4l2_subdev *sd, int on)
{
	struct gc2053_dev *sensor = to_gc2053(sd);
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
		ret = gc2053_start_streaming(sensor);
	else
		ret = gc2053_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int gc2053_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct gc2053_dev *sensor = to_gc2053(sd);

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	interval->interval = sensor->cur_mode->max_fps;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int gc2053_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct gc2053_dev *sensor = to_gc2053(sd);
	const struct gc2053_mode *mode;
	int ret = 0;

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (sensor->streaming) {
		ret = -EBUSY;
		goto unlock;
	}

	mode = &gc2053_modes[0];

	sensor->cur_mode = mode;
	interval->interval = mode->max_fps;

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void gc2053_fill_format(struct gc2053_dev *sensor,
			       struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = GC2053_WIDTH,
		.height = GC2053_HEIGHT,
		.code = gc2053_bayer_code(sensor),
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int gc2053_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct gc2053_dev *sensor = to_gc2053(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	} else {
		fmt->format = sensor->fmt;
		fmt->format.code = gc2053_bayer_code(sensor);
	}
	mutex_unlock(&sensor->lock);

	return 0;
}

static int gc2053_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct gc2053_dev *sensor = to_gc2053(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}

	gc2053_fill_format(sensor, &fmt->format);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int gc2053_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct gc2053_dev *sensor = to_gc2053(sd);

	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	code->code = gc2053_bayer_code(sensor);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc2053_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct gc2053_dev *sensor = to_gc2053(sd);
	int ret = 0;

	if (fse->pad != 0 || fse->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fse->code != gc2053_bayer_code(sensor)) {
		ret = -EINVAL;
	} else {
		fse->min_width = GC2053_WIDTH;
		fse->max_width = GC2053_WIDTH;
		fse->min_height = GC2053_HEIGHT;
		fse->max_height = GC2053_HEIGHT;
	}
	mutex_unlock(&sensor->lock);
	return ret;
}

static int
gc2053_enum_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_interval_enum *fie)
{
	struct gc2053_dev *sensor = to_gc2053(sd);

	if (fie->pad != 0 || fie->index >= ARRAY_SIZE(gc2053_modes))
		return -EINVAL;

	if (fie->width != GC2053_WIDTH || fie->height != GC2053_HEIGHT)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fie->code != gc2053_bayer_code(sensor)) {
		mutex_unlock(&sensor->lock);
		return -EINVAL;
	}
	fie->interval = gc2053_modes[fie->index].max_fps;
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc2053_init_cfg(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return gc2053_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops gc2053_video_ops = {
	.s_stream = gc2053_s_stream,
	.g_frame_interval = gc2053_g_frame_interval,
	.s_frame_interval = gc2053_s_frame_interval,
};

static const struct v4l2_subdev_core_ops gc2053_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops gc2053_pad_ops = {
	.init_cfg = gc2053_init_cfg,
	.enum_mbus_code = gc2053_enum_mbus_code,
	.enum_frame_size = gc2053_enum_frame_size,
	.enum_frame_interval = gc2053_enum_frame_interval,
	.get_fmt = gc2053_get_fmt,
	.set_fmt = gc2053_set_fmt,
};

static const struct v4l2_subdev_ops gc2053_subdev_ops = {
	.core = &gc2053_core_ops,
	.video = &gc2053_video_ops,
	.pad = &gc2053_pad_ops,
};

static int gc2053_check_chip_id(struct gc2053_dev *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = gc2053_write_reg(sensor->client, GC2053_PAGE_SELECT, 0x00);
	if (ret)
		return ret;

	ret = gc2053_read_reg(sensor->client, GC2053_CHIP_ID_H, &id_h);
	if (ret)
		return ret;

	ret = gc2053_read_reg(sensor->client, GC2053_CHIP_ID_L, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != GC2053_CHIP_ID) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}

	return 0;
}

static int gc2053_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct gc2053_dev *sensor;
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
	    bus.bus.mipi_csi2.num_data_lanes != 2 ||
	    bus.bus.mipi_csi2.data_lanes[0] != 1 ||
	    bus.bus.mipi_csi2.data_lanes[1] != 2 ||
	    bus.bus.mipi_csi2.clock_lane != 0 ||
	    bus.bus.mipi_csi2.lane_polarities[0] ||
	    bus.bus.mipi_csi2.lane_polarities[1] ||
	    bus.bus.mipi_csi2.lane_polarities[2])
		return -EINVAL;

	sensor = kzalloc(sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->client = client;
	sensor->cur_mode = &gc2053_modes[0];
	kref_init(&sensor->ref);
	sensor->lifetime.ref = &sensor->ref;
	sensor->lifetime.release = gc2053_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &gc2053_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &gc2053_internal_ops;

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

	ret = gc2053_power_on(sensor);
	if (ret)
		goto err_put;

	ret = gc2053_check_chip_id(sensor);
	if (ret)
		goto err_power;

	gc2053_power_off(sensor);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;

	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_power;

	ret = gc2053_init_controls(sensor);
	if (ret)
		goto err_power;
	gc2053_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		v4l2_async_unregister_subdev(&sensor->sd);
		goto err_power;
	}

	return 0;

err_power:
	gc2053_power_off(sensor);
err_put:
	kref_put(&sensor->ref, gc2053_release);
	return ret;
}

static int gc2053_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc2053_dev *sensor = to_gc2053(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	gc2053_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, gc2053_release);
	return 0;
}

static const struct of_device_id gc2053_of_match[] = {
	{ .compatible = "galaxycore,gc2053" },
	{ }
};
MODULE_DEVICE_TABLE(of, gc2053_of_match);

static const struct i2c_device_id gc2053_id[] = {
	{ "gc2053", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc2053_id);

static struct i2c_driver gc2053_i2c_driver = {
	.driver = {
		.name = "gc2053",
		.of_match_table = gc2053_of_match,
	},
	.probe		= gc2053_probe,
	.remove		= gc2053_remove,
	.id_table	= gc2053_id,
};
module_i2c_driver(gc2053_i2c_driver);

MODULE_DESCRIPTION("GalaxyCore GC2053 2-lane MIPI CSI-2 camera sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
