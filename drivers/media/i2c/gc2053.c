// SPDX-License-Identifier: GPL-2.0
/*
 * GalaxyCore GC2053 MIPI CSI-2 1080p CMOS Image Sensor Driver
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gcd.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#define GC2053_CHIP_ID			0x2053
#define GC2053_REG_PAGE			0xfe
#define GC2053_REG_CHIP_ID_H		0xf0
#define GC2053_REG_CHIP_ID_L		0xf1
#define GC2053_REG_EXP_H		0x03
#define GC2053_REG_EXP_L		0x04
#define GC2053_REG_MIRROR		0x17
#define GC2053_REG_STREAM		0x3e
#define GC2053_REG_VTS_H		0x41
#define GC2053_REG_VTS_L		0x42
#define GC2053_REG_TEST_PATTERN		0x8c
#define GC2053_REG_AGAIN_B3		0xb3
#define GC2053_REG_AGAIN_B4		0xb4
#define GC2053_REG_AGAIN_B8		0xb8
#define GC2053_REG_AGAIN_B9		0xb9

#define GC2053_XVCLK_FREQ		24000000
#define GC2053_PIXEL_RATE		74250000LL
#define GC2053_LINK_FREQ_297M		297000000LL

#define GC2053_WIDTH			1920
#define GC2053_HEIGHT			1080
#define GC2053_HTS_DEF			2200
#define GC2053_HBLANK_DEF		(GC2053_HTS_DEF - GC2053_WIDTH)
#define GC2053_VTS_MIN			1125
#define GC2053_VTS_DEF			1125
#define GC2053_VTS_MAX			16383
#define GC2053_EXP_MIN			1
#define GC2053_EXP_MARGIN		8

struct gc2053_regval {
	u8 reg;
	u8 val;
};

struct gc2053_gain_entry {
	u8 b4;
	u8 b3;
	u8 b8;
	u8 b9;
};

/*
 * Standard 26-step GC2053 analog + auto-pregain table (1.00x .. 63.98x).
 * Effective linear multiplier is (b8 * 64 + (b9 & 0x3f)) / 64.0.
 */
static const struct gc2053_gain_entry gc2053_gain_lut[] = {
	{ 0x00, 0x00, 0x01, 0x00 }, /* 1.000x */
	{ 0x00, 0x10, 0x01, 0x0c }, /* 1.188x */
	{ 0x00, 0x20, 0x01, 0x1b }, /* 1.422x */
	{ 0x00, 0x30, 0x01, 0x2c }, /* 1.688x */
	{ 0x00, 0x40, 0x01, 0x3f }, /* 1.984x */
	{ 0x00, 0x50, 0x02, 0x16 }, /* 2.344x */
	{ 0x00, 0x60, 0x02, 0x35 }, /* 2.828x */
	{ 0x00, 0x70, 0x03, 0x16 }, /* 3.344x */
	{ 0x00, 0x80, 0x04, 0x02 }, /* 4.031x */
	{ 0x00, 0x90, 0x04, 0x31 }, /* 4.766x */
	{ 0x00, 0xa0, 0x05, 0x32 }, /* 5.781x */
	{ 0x00, 0xb0, 0x06, 0x35 }, /* 6.828x */
	{ 0x00, 0xc0, 0x08, 0x04 }, /* 8.062x */
	{ 0x00, 0x5a, 0x09, 0x19 }, /* 9.391x */
	{ 0x00, 0x83, 0x0b, 0x0f }, /* 11.234x */
	{ 0x00, 0x93, 0x0d, 0x12 }, /* 13.281x */
	{ 0x00, 0x84, 0x10, 0x00 }, /* 16.000x */
	{ 0x00, 0x94, 0x12, 0x3a }, /* 18.906x */
	{ 0x01, 0x2c, 0x1a, 0x02 }, /* 26.031x */
	{ 0x01, 0x3c, 0x1b, 0x20 }, /* 27.500x */
	{ 0x00, 0x8c, 0x20, 0x0f }, /* 32.234x */
	{ 0x00, 0x9c, 0x26, 0x07 }, /* 38.109x */
	{ 0x02, 0x64, 0x36, 0x21 }, /* 54.516x */
	{ 0x02, 0x74, 0x37, 0x3a }, /* 55.906x */
	{ 0x00, 0xc6, 0x3d, 0x02 }, /* 61.031x */
	{ 0x00, 0xdc, 0x3f, 0x3f }, /* 63.984x */
};

/* 1920x1080 RAW10 30fps 24MHz xvclk MIPI CSI-2 init sequence */
static const struct gc2053_regval gc2053_init_1080p_regs[] = {
	{ 0xfe, 0x80 },
	{ 0xfe, 0x80 },
	{ 0xfe, 0x80 },
	{ 0xfe, 0x00 },
	{ 0xf2, 0x00 },
	{ 0xf3, 0x00 },
	{ 0xf4, 0x36 },
	{ 0xf5, 0xc0 },
	{ 0xf6, 0x44 },
	{ 0xf7, 0x01 },
	{ 0xf8, 0x2c },
	{ 0xf9, 0x42 },
	{ 0xfc, 0x8e },
	{ 0xfe, 0x00 },
	{ 0x87, 0x18 },
	{ 0xee, 0x30 },
	{ 0xd0, 0xb7 },
	{ 0x03, 0x04 },
	{ 0x04, 0x5d },
	{ 0x05, 0x04 },
	{ 0x06, 0x4c },
	{ 0x07, 0x00 },
	{ 0x08, 0x11 },
	{ 0x09, 0x00 },
	{ 0x0a, 0x02 },
	{ 0x0b, 0x00 },
	{ 0x0c, 0x02 },
	{ 0x0d, 0x04 },
	{ 0x0e, 0x40 },
	{ 0x12, 0xe2 },
	{ 0x13, 0x16 },
	{ 0x19, 0x0a },
	{ 0x21, 0x1c },
	{ 0x28, 0x0a },
	{ 0x29, 0x24 },
	{ 0x2b, 0x04 },
	{ 0x32, 0xf8 },
	{ 0x37, 0x03 },
	{ 0x39, 0x15 },
	{ 0x41, 0x04 },
	{ 0x42, 0x65 },
	{ 0x43, 0x07 },
	{ 0x44, 0x40 },
	{ 0x46, 0x0b },
	{ 0x4b, 0x20 },
	{ 0x4e, 0x08 },
	{ 0x55, 0x20 },
	{ 0x66, 0x05 },
	{ 0x67, 0x05 },
	{ 0x77, 0x01 },
	{ 0x78, 0x00 },
	{ 0x7c, 0x93 },
	{ 0x8c, 0x12 },
	{ 0x8d, 0x92 },
	{ 0x90, 0x00 },
	{ 0x9d, 0x10 },
	{ 0xce, 0x7c },
	{ 0xd2, 0x41 },
	{ 0xd3, 0xdc },
	{ 0xe6, 0x50 },
	{ 0xb6, 0xc0 },
	{ 0xb0, 0x70 },
	{ 0xb1, 0x01 },
	{ 0xb2, 0x00 },
	{ 0xb3, 0x00 },
	{ 0xb4, 0x00 },
	{ 0xb8, 0x01 },
	{ 0xb9, 0x00 },
	{ 0xfe, 0x01 },
	{ 0x55, 0x00 },
	{ 0xfe, 0x04 },
	{ 0x14, 0x78 },
	{ 0x15, 0x78 },
	{ 0x16, 0x78 },
	{ 0x17, 0x78 },
	{ 0xfe, 0x01 },
	{ 0x04, 0x00 },
	{ 0x94, 0x03 },
	{ 0x97, 0x07 },
	{ 0x98, 0x80 },
	{ 0x9a, 0x06 },
	{ 0xfe, 0x00 },
	{ 0x7b, 0x2a },
	{ 0x23, 0x2d },
	{ 0xfe, 0x03 },
	{ 0x01, 0x27 },
	{ 0x02, 0x56 },
	{ 0x03, 0xb6 },
	{ 0x12, 0x80 },
	{ 0x13, 0x07 },
	{ 0x15, 0x12 },
	{ 0xfe, 0x00 },
	{ 0x3e, 0x00 },
};

static const s64 gc2053_link_freqs[] = {
	GC2053_LINK_FREQ_297M,
};

static const char * const gc2053_test_pattern_menu[] = {
	"Disabled",
	"Color Bars",
};

static const char * const gc2053_supply_names[] = {
	"avdd",
	"dovdd",
	"dvdd",
};

struct gc2053 {
	struct i2c_client *client;
	struct clk *xvclk;
	struct regulator_bulk_data supplies[ARRAY_SIZE(gc2053_supply_names)];
	struct gpio_desc *reset_gpio;
	struct gpio_desc *pwdn_gpio;

	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_config bus;

	struct mutex lock; /* serializes subdev active state & controls */
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *again;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;

	u32 vts;
	bool streaming;
};

static inline struct gc2053 *to_gc2053(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc2053, sd);
}

static int gc2053_write_reg(struct gc2053 *gc, u8 reg, u8 val)
{
	u8 buf[2] = { reg, val };
	struct i2c_msg msg = {
		.addr = gc->client->addr,
		.flags = 0,
		.len = 2,
		.buf = buf,
	};
	int ret = i2c_transfer(gc->client->adapter, &msg, 1);

	return ret == 1 ? 0 : (ret < 0 ? ret : -EIO);
}

static int gc2053_read_reg(struct gc2053 *gc, u8 reg, u8 *val)
{
	struct i2c_msg msgs[2] = {
		{
			.addr = gc->client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = gc->client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = val,
		},
	};
	int ret = i2c_transfer(gc->client->adapter, msgs, 2);

	return ret == 2 ? 0 : (ret < 0 ? ret : -EIO);
}

static int gc2053_write_array(struct gc2053 *gc,
			      const struct gc2053_regval *regs, size_t count)
{
	size_t i;
	int ret;

	for (i = 0; i < count; i++) {
		ret = gc2053_write_reg(gc, regs[i].reg, regs[i].val);
		if (ret)
			return ret;
	}
	return 0;
}

static int gc2053_set_exposure(struct gc2053 *gc, u32 lines)
{
	int ret;

	ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	if (ret)
		return ret;
	ret = gc2053_write_reg(gc, GC2053_REG_EXP_H, (lines >> 8) & 0x3f);
	if (ret)
		return ret;
	return gc2053_write_reg(gc, GC2053_REG_EXP_L, lines & 0xff);
}

static int gc2053_set_vts(struct gc2053 *gc, u32 vts)
{
	int ret;

	gc->vts = vts;
	ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	if (ret)
		return ret;
	ret = gc2053_write_reg(gc, GC2053_REG_VTS_H, (vts >> 8) & 0x3f);
	if (ret)
		return ret;
	return gc2053_write_reg(gc, GC2053_REG_VTS_L, vts & 0xff);
}

static int gc2053_set_again(struct gc2053 *gc, s32 index)
{
	const struct gc2053_gain_entry *g;
	int ret;

	if (index < 0 || index >= ARRAY_SIZE(gc2053_gain_lut))
		return -EINVAL;
	g = &gc2053_gain_lut[index];

	ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	if (ret)
		return ret;
	ret = gc2053_write_reg(gc, GC2053_REG_AGAIN_B4, g->b4);
	if (ret)
		return ret;
	ret = gc2053_write_reg(gc, GC2053_REG_AGAIN_B3, g->b3);
	if (ret)
		return ret;
	ret = gc2053_write_reg(gc, GC2053_REG_AGAIN_B8, g->b8);
	if (ret)
		return ret;
	return gc2053_write_reg(gc, GC2053_REG_AGAIN_B9, g->b9);
}

static int gc2053_set_flip(struct gc2053 *gc, bool hflip, bool vflip)
{
	u8 val = 0x80 | (vflip ? 0x02 : 0x00) | (hflip ? 0x01 : 0x00);
	int ret;

	ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	if (ret)
		return ret;
	return gc2053_write_reg(gc, GC2053_REG_MIRROR, val);
}

static int gc2053_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc2053 *gc = container_of(ctrl->handler, struct gc2053, ctrls);
	s32 old_exp = gc->exposure ? gc->exposure->val : 0;
	int ret;

	if (ctrl->id == V4L2_CID_VBLANK && gc->exposure) {
		u32 vts = GC2053_HEIGHT + ctrl->val;
		s32 max_exp = vts - GC2053_EXP_MARGIN;

		__v4l2_ctrl_modify_range(gc->exposure, GC2053_EXP_MIN, max_exp,
					 1, min_t(s32, gc->exposure->default_value,
						  max_exp));
		if (!gc->streaming) {
			gc->vts = vts;
			return 0;
		}
	} else if (!gc->streaming) {
		return 0;
	}

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return gc2053_set_exposure(gc, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return gc2053_set_again(gc, ctrl->val);
	case V4L2_CID_VBLANK:
		if (gc->exposure && gc->exposure->val < old_exp) {
			ret = gc2053_set_exposure(gc, gc->exposure->val);
			if (ret)
				return ret;
		}
		return gc2053_set_vts(gc, GC2053_HEIGHT + ctrl->val);
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
		return gc2053_set_flip(gc,
				       ctrl->id == V4L2_CID_HFLIP ? ctrl->val :
				       (gc->hflip ? gc->hflip->val : 0),
				       ctrl->id == V4L2_CID_VFLIP ? ctrl->val :
				       (gc->vflip ? gc->vflip->val : 0));
	case V4L2_CID_TEST_PATTERN:
		ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
		if (ret)
			return ret;
		return gc2053_write_reg(gc, GC2053_REG_TEST_PATTERN,
					ctrl->val ? 0x13 : 0x12);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops gc2053_ctrl_ops = {
	.s_ctrl = gc2053_s_ctrl,
};

static void gc2053_fill_fmt(struct v4l2_mbus_framefmt *fmt)
{
	fmt->width = GC2053_WIDTH;
	fmt->height = GC2053_HEIGHT;
	fmt->code = MEDIA_BUS_FMT_SRGGB10_1X10;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
}

static int gc2053_init_state(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state)
{
	struct v4l2_mbus_framefmt *fmt =
		v4l2_subdev_state_get_format(sd_state, 0);

	if (fmt)
		gc2053_fill_fmt(fmt);
	return 0;
}

static int gc2053_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index)
		return -EINVAL;
	code->code = MEDIA_BUS_FMT_SRGGB10_1X10;
	return 0;
}

static int gc2053_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->pad || fse->index || fse->code != MEDIA_BUS_FMT_SRGGB10_1X10)
		return -EINVAL;
	fse->min_width = GC2053_WIDTH;
	fse->max_width = GC2053_WIDTH;
	fse->min_height = GC2053_HEIGHT;
	fse->max_height = GC2053_HEIGHT;
	return 0;
}

static int gc2053_enum_frame_interval(struct v4l2_subdev *sd,
				      struct v4l2_subdev_state *sd_state,
				      struct v4l2_subdev_frame_interval_enum *fie)
{
	if (fie->pad || fie->index || fie->code != MEDIA_BUS_FMT_SRGGB10_1X10 ||
	    fie->width != GC2053_WIDTH || fie->height != GC2053_HEIGHT)
		return -EINVAL;
	fie->interval.numerator = 1;
	fie->interval.denominator = 30;
	return 0;
}

static int gc2053_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *sd_state,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *fmt;

	if (format->pad)
		return -EINVAL;
	fmt = v4l2_subdev_state_get_format(sd_state, format->pad);
	if (!fmt)
		return -EINVAL;
	gc2053_fill_fmt(fmt);
	format->format = *fmt;
	return 0;
}

static int gc2053_get_frame_interval(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *sd_state,
				     struct v4l2_subdev_frame_interval *fi)
{
	struct gc2053 *gc = to_gc2053(sd);
	u64 num = (u64)GC2053_HTS_DEF * gc->vts;
	u64 den = GC2053_PIXEL_RATE;
	unsigned long div;

	if (fi->pad)
		return -EINVAL;
	div = gcd(num, den);
	if (div) {
		num = div64_ul(num, div);
		den = div64_ul(den, div);
	}
	fi->interval.numerator = num;
	fi->interval.denominator = den;
	return 0;
}

static int gc2053_set_frame_interval(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *sd_state,
				     struct v4l2_subdev_frame_interval *fi)
{
	struct gc2053 *gc = to_gc2053(sd);

	if (fi->pad)
		return -EINVAL;

	if (fi->which == V4L2_SUBDEV_FORMAT_ACTIVE && gc->vblank &&
	    fi->interval.numerator && fi->interval.denominator) {
		u64 vts = div_u64(GC2053_PIXEL_RATE * fi->interval.numerator,
				  (u64)GC2053_HTS_DEF * fi->interval.denominator);
		s32 vblank = (s32)vts - GC2053_HEIGHT;

		vblank = clamp(vblank, gc->vblank->minimum,
			       gc->vblank->maximum);
		__v4l2_ctrl_s_ctrl(gc->vblank, vblank);
	}
	return gc2053_get_frame_interval(sd, sd_state, fi);
}

static int gc2053_get_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				  struct v4l2_mbus_config *config)
{
	if (pad)
		return -EINVAL;
	*config = to_gc2053(sd)->bus;
	return 0;
}

static int gc2053_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct gc2053 *gc = to_gc2053(sd);
	u8 lanes = gc->bus.bus.mipi_csi2.num_data_lanes;
	u8 stream_val = lanes == 1 ? 0x90 : 0x91;
	int ret = 0;

	mutex_lock(&gc->lock);
	if (gc->streaming == !!enable)
		goto unlock;

	if (!enable) {
		gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
		ret = gc2053_write_reg(gc, GC2053_REG_STREAM, 0x00);
		gc->streaming = false;
		__v4l2_ctrl_grab(gc->hflip, false);
		__v4l2_ctrl_grab(gc->vflip, false);
		pm_runtime_put(&gc->client->dev);
		goto unlock;
	}

	ret = pm_runtime_resume_and_get(&gc->client->dev);
	if (ret < 0)
		goto unlock;

	ret = gc2053_write_array(gc, gc2053_init_1080p_regs,
				 ARRAY_SIZE(gc2053_init_1080p_regs));
	if (ret)
		goto err_rpm;

	gc->streaming = true;
	ret = __v4l2_ctrl_handler_setup(&gc->ctrls);
	if (ret)
		goto err_stop;

	ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	if (ret)
		goto err_stop;
	ret = gc2053_write_reg(gc, GC2053_REG_STREAM, stream_val);
	if (ret)
		goto err_stop;

	__v4l2_ctrl_grab(gc->hflip, true);
	__v4l2_ctrl_grab(gc->vflip, true);
	goto unlock;

err_stop:
	gc->streaming = false;
	gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	gc2053_write_reg(gc, GC2053_REG_STREAM, 0x00);
err_rpm:
	pm_runtime_put(&gc->client->dev);
unlock:
	mutex_unlock(&gc->lock);
	return ret;
}

static const struct v4l2_subdev_core_ops gc2053_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops gc2053_video_ops = {
	.s_stream = gc2053_s_stream,
};

static const struct v4l2_subdev_pad_ops gc2053_pad_ops = {
	.enum_mbus_code = gc2053_enum_mbus_code,
	.enum_frame_size = gc2053_enum_frame_size,
	.enum_frame_interval = gc2053_enum_frame_interval,
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = gc2053_set_fmt,
	.get_frame_interval = gc2053_get_frame_interval,
	.set_frame_interval = gc2053_set_frame_interval,
	.get_mbus_config = gc2053_get_mbus_config,
};

static const struct v4l2_subdev_ops gc2053_subdev_ops = {
	.core = &gc2053_core_ops,
	.video = &gc2053_video_ops,
	.pad = &gc2053_pad_ops,
};

static const struct v4l2_subdev_internal_ops gc2053_internal_ops = {
	.init_state = gc2053_init_state,
};

static int gc2053_power_on(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct gc2053 *gc = to_gc2053(sd);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(gc2053_supply_names),
				    gc->supplies);
	if (ret)
		return ret;

	ret = clk_prepare_enable(gc->xvclk);
	if (ret) {
		regulator_bulk_disable(ARRAY_SIZE(gc2053_supply_names),
				       gc->supplies);
		return ret;
	}

	if (gc->pwdn_gpio) {
		gpiod_set_value_cansleep(gc->pwdn_gpio, 0);
		usleep_range(1000, 2000);
	}
	if (gc->reset_gpio) {
		gpiod_set_value_cansleep(gc->reset_gpio, 1);
		usleep_range(1000, 2000);
		gpiod_set_value_cansleep(gc->reset_gpio, 0);
		usleep_range(5000, 8000);
	}
	return 0;
}

static int gc2053_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct gc2053 *gc = to_gc2053(sd);

	if (gc->reset_gpio)
		gpiod_set_value_cansleep(gc->reset_gpio, 1);
	if (gc->pwdn_gpio)
		gpiod_set_value_cansleep(gc->pwdn_gpio, 1);
	clk_disable_unprepare(gc->xvclk);
	regulator_bulk_disable(ARRAY_SIZE(gc2053_supply_names), gc->supplies);
	return 0;
}

static int gc2053_identify(struct gc2053 *gc)
{
	u8 id_h = 0, id_l = 0;
	u16 id;
	int ret;

	ret = gc2053_write_reg(gc, GC2053_REG_PAGE, 0x00);
	if (ret)
		return ret;
	ret = gc2053_read_reg(gc, GC2053_REG_CHIP_ID_H, &id_h);
	if (ret)
		return ret;
	ret = gc2053_read_reg(gc, GC2053_REG_CHIP_ID_L, &id_l);
	if (ret)
		return ret;

	id = ((u16)id_h << 8) | id_l;
	if (id != GC2053_CHIP_ID) {
		dev_err(&gc->client->dev,
			"unexpected chip ID %#04x (expected %#04x)\n",
			id, GC2053_CHIP_ID);
		return -ENODEV;
	}
	return 0;
}

static int gc2053_parse_fwnode(struct gc2053 *gc)
{
	struct v4l2_fwnode_endpoint bus_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	struct fwnode_handle *ep;
	unsigned long link_freq_bitmap;
	int ret;

	ep = fwnode_graph_get_next_endpoint(dev_fwnode(&gc->client->dev), NULL);
	if (!ep)
		return -ENOTCONN;

	ret = v4l2_fwnode_endpoint_alloc_parse(ep, &bus_cfg);
	fwnode_handle_put(ep);
	if (ret)
		return ret;

	if (bus_cfg.bus.mipi_csi2.num_data_lanes != 2) {
		ret = -EINVAL;
		goto out_free;
	}

	ret = v4l2_link_freq_to_bitmap(&gc->client->dev,
				       bus_cfg.link_frequencies,
				       bus_cfg.nr_of_link_frequencies,
				       gc2053_link_freqs,
				       ARRAY_SIZE(gc2053_link_freqs),
				       &link_freq_bitmap);
	if (ret)
		goto out_free;

	gc->bus.type = V4L2_MBUS_CSI2_DPHY;
	gc->bus.bus.mipi_csi2 = bus_cfg.bus.mipi_csi2;

out_free:
	v4l2_fwnode_endpoint_free(&bus_cfg);
	return ret;
}

static int gc2053_init_controls(struct gc2053 *gc)
{
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl *link_freq;
	int ret;

	ret = v4l2_ctrl_handler_init(&gc->ctrls, 10);
	if (ret)
		return ret;

	gc->ctrls.lock = &gc->lock;

	link_freq = v4l2_ctrl_new_int_menu(&gc->ctrls, NULL,
					   V4L2_CID_LINK_FREQ,
					   ARRAY_SIZE(gc2053_link_freqs) - 1,
					   0, gc2053_link_freqs);
	if (link_freq)
		link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	gc->pixel_rate = v4l2_ctrl_new_std(&gc->ctrls, NULL,
					   V4L2_CID_PIXEL_RATE,
					   GC2053_PIXEL_RATE, GC2053_PIXEL_RATE,
					   1, GC2053_PIXEL_RATE);
	if (gc->pixel_rate)
		gc->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	gc->hblank = v4l2_ctrl_new_std(&gc->ctrls, NULL, V4L2_CID_HBLANK,
				       GC2053_HBLANK_DEF, GC2053_HBLANK_DEF,
				       1, GC2053_HBLANK_DEF);
	if (gc->hblank)
		gc->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	gc->vblank = v4l2_ctrl_new_std(&gc->ctrls, &gc2053_ctrl_ops,
				       V4L2_CID_VBLANK,
				       GC2053_VTS_MIN - GC2053_HEIGHT,
				       GC2053_VTS_MAX - GC2053_HEIGHT,
				       1, GC2053_VTS_DEF - GC2053_HEIGHT);

	gc->exposure = v4l2_ctrl_new_std(&gc->ctrls, &gc2053_ctrl_ops,
					 V4L2_CID_EXPOSURE, GC2053_EXP_MIN,
					 GC2053_VTS_DEF - GC2053_EXP_MARGIN,
					 1, GC2053_VTS_DEF - GC2053_EXP_MARGIN);

	gc->again = v4l2_ctrl_new_std(&gc->ctrls, &gc2053_ctrl_ops,
				      V4L2_CID_ANALOGUE_GAIN, 0,
				      ARRAY_SIZE(gc2053_gain_lut) - 1, 1, 0);

	gc->hflip = v4l2_ctrl_new_std(&gc->ctrls, &gc2053_ctrl_ops,
				      V4L2_CID_HFLIP, 0, 1, 1, 0);
	gc->vflip = v4l2_ctrl_new_std(&gc->ctrls, &gc2053_ctrl_ops,
				      V4L2_CID_VFLIP, 0, 1, 1, 0);

	v4l2_ctrl_new_std_menu_items(&gc->ctrls, &gc2053_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(gc2053_test_pattern_menu) - 1,
				     0, 0, gc2053_test_pattern_menu);

	ret = v4l2_fwnode_device_parse(&gc->client->dev, &props);
	if (!ret)
		v4l2_ctrl_new_fwnode_properties(&gc->ctrls, &gc2053_ctrl_ops,
						&props);

	if (gc->ctrls.error) {
		ret = gc->ctrls.error;
		v4l2_ctrl_handler_free(&gc->ctrls);
		return ret;
	}

	gc->sd.ctrl_handler = &gc->ctrls;
	return 0;
}

static int gc2053_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct gc2053 *gc;
	unsigned int i;
	int ret;

	gc = devm_kzalloc(dev, sizeof(*gc), GFP_KERNEL);
	if (!gc)
		return -ENOMEM;

	gc->client = client;
	gc->vts = GC2053_VTS_DEF;
	mutex_init(&gc->lock);

	ret = gc2053_parse_fwnode(gc);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(gc2053_supply_names); i++)
		gc->supplies[i].supply = gc2053_supply_names[i];
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(gc2053_supply_names),
				      gc->supplies);
	if (ret)
		return dev_err_probe(dev, ret, "failed to get regulators\n");

	gc->xvclk = devm_clk_get_optional(dev, "xvclk");
	if (IS_ERR(gc->xvclk))
		return PTR_ERR(gc->xvclk);
	if (!gc->xvclk) {
		gc->xvclk = devm_clk_get(dev, NULL);
		if (IS_ERR(gc->xvclk))
			return PTR_ERR(gc->xvclk);
	}

	ret = clk_set_rate(gc->xvclk, GC2053_XVCLK_FREQ);
	if (ret)
		return ret;

	gc->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(gc->reset_gpio))
		return PTR_ERR(gc->reset_gpio);

	gc->pwdn_gpio = devm_gpiod_get_optional(dev, "pwdn", GPIOD_OUT_HIGH);
	if (IS_ERR(gc->pwdn_gpio))
		return PTR_ERR(gc->pwdn_gpio);

	v4l2_i2c_subdev_init(&gc->sd, client, &gc2053_subdev_ops);
	gc->sd.internal_ops = &gc2053_internal_ops;

	ret = gc2053_power_on(dev);
	if (ret)
		return ret;

	ret = gc2053_identify(gc);
	if (ret)
		goto err_power_off;

	ret = gc2053_init_controls(gc);
	if (ret)
		goto err_power_off;

	gc->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	gc->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	gc->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&gc->sd.entity, 1, &gc->pad);
	if (ret)
		goto err_ctrls;

	gc->sd.state_lock = &gc->lock;
	ret = v4l2_subdev_init_finalize(&gc->sd);
	if (ret)
		goto err_entity;

	ret = v4l2_async_register_subdev_sensor(&gc->sd);
	if (ret)
		goto err_subdev;

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);

	return 0;

err_subdev:
	v4l2_subdev_cleanup(&gc->sd);
err_entity:
	media_entity_cleanup(&gc->sd.entity);
err_ctrls:
	v4l2_ctrl_handler_free(&gc->ctrls);
err_power_off:
	gc2053_power_off(dev);
	return ret;
}

static void gc2053_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc2053 *gc = to_gc2053(sd);

	v4l2_async_unregister_subdev(&gc->sd);
	v4l2_subdev_cleanup(&gc->sd);
	media_entity_cleanup(&gc->sd.entity);
	v4l2_ctrl_handler_free(&gc->ctrls);

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		gc2053_power_off(&client->dev);
	pm_runtime_set_suspended(&client->dev);
}

static const struct dev_pm_ops gc2053_pm_ops = {
	SET_RUNTIME_PM_OPS(gc2053_power_off, gc2053_power_on, NULL)
};

static const struct i2c_device_id gc2053_id[] = {
	{ "gc2053", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc2053_id);

static const struct of_device_id gc2053_of_match[] = {
	{ .compatible = "galaxycore,gc2053" },
	{ }
};
MODULE_DEVICE_TABLE(of, gc2053_of_match);

static struct i2c_driver gc2053_driver = {
	.driver = {
		.name = "gc2053",
		.of_match_table = gc2053_of_match,
		.pm = &gc2053_pm_ops,
	},
	.probe = gc2053_probe,
	.remove = gc2053_remove,
	.id_table = gc2053_id,
};
module_i2c_driver(gc2053_driver);

MODULE_DESCRIPTION("GalaxyCore GC2053 MIPI CSI-2 sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
