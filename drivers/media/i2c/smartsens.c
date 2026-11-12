// SPDX-License-Identifier: GPL-2.0
/*
 * Common code for SmartSens MIPI CSI-2 camera sensors
 *
 * The SmartSens sensors share their streaming, reset, frame length,
 * exposure and flip registers. Each chip driver supplies its mode table,
 * timing and analogue gain encoding. The frame length is set with
 * V4L2_CID_VBLANK.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kref.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/v4l2-mediabus.h>
#include <media/fh8626-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "smartsens.h"

#define SMARTSENS_REG_STREAM		0x0100
#define SMARTSENS_STREAM_ON		0x01
#define SMARTSENS_STREAM_OFF		0x00

#define SMARTSENS_REG_RESET		0x0103
#define SMARTSENS_RESET			0x01

#define SMARTSENS_REG_CHIP_ID_H		0x3107
#define SMARTSENS_REG_CHIP_ID_L		0x3108

#define SMARTSENS_REG_VTS_H		0x320e
#define SMARTSENS_REG_VTS_L		0x320f
#define SMARTSENS_VTS_MAX		0x7fff

#define SMARTSENS_REG_FLIP		0x3221
#define SMARTSENS_FLIP_MASK		0x66
#define SMARTSENS_MIRROR		0x06
#define SMARTSENS_FLIP			0x60

/*
 * The integration time is a 20-bit field over 0x3e00[3:0], 0x3e01 and
 * 0x3e02[7:4]. The chip sets its unit, one line or a fraction of one;
 * the four bits below it are not used.
 */
#define SMARTSENS_REG_EXPOSURE_H	0x3e00
#define SMARTSENS_REG_EXPOSURE_M	0x3e01
#define SMARTSENS_REG_EXPOSURE_L	0x3e02

#define SMARTSENS_EXPOSURE_MIN		1
#define SMARTSENS_EXPOSURE_MARGIN	4
#define SMARTSENS_EXPOSURE_DEFAULT	500

/* Readout shifts with mirror and flip, so the output stays BGGR */
#define SMARTSENS_MBUS_CODE		MEDIA_BUS_FMT_SBGGR10_1X10

struct smartsens_sensor {
	struct kref		ref;
	struct fh8626_subdev_lifetime lifetime;
	struct i2c_client	*client;
	const struct smartsens_chip *chip;
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

static inline struct smartsens_sensor *to_smartsens(struct v4l2_subdev *sd)
{
	return container_of(sd, struct smartsens_sensor, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct smartsens_sensor, ctrls)->sd;
}

static void smartsens_release(struct kref *ref)
{
	struct smartsens_sensor *sensor =
		container_of(ref, struct smartsens_sensor, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int smartsens_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_smartsens(sd)->ref);
	return 0;
}

static void smartsens_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_smartsens(sd)->ref, smartsens_release);
}

static const struct v4l2_subdev_internal_ops smartsens_internal_ops = {
	.registered = smartsens_registered,
	.unregistered = smartsens_subdev_release,
};

static int smartsens_read_reg(struct smartsens_sensor *sensor, u16 reg, u8 *val)
{
	struct i2c_client *client = sensor->client;
	u8 addr[2] = { reg >> 8, reg & 0xff };
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = sizeof(addr),
			.buf = addr,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = val,
		},
	};
	int ret;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs)) {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&client->dev, "failed to read reg 0x%04x: %d\n",
			reg, ret);
		return ret;
	}
	return 0;
}

int smartsens_write_reg(struct smartsens_sensor *sensor, u16 reg, u8 val)
{
	struct i2c_client *client = sensor->client;
	u8 buf[3] = { reg >> 8, reg & 0xff, val };
	int ret;

	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&client->dev, "failed to write reg 0x%04x=0x%02x: %d\n",
			reg, val, ret);
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(smartsens_write_reg);

static int smartsens_set_exposure(struct smartsens_sensor *sensor, s32 lines)
{
	u32 val = (u32)lines << sensor->chip->exposure_shift;
	int ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_EXPOSURE_H,
				  (val >> 16) & 0x0f);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_EXPOSURE_M,
				  (val >> 8) & 0xff);
	if (ret)
		return ret;

	return smartsens_write_reg(sensor, SMARTSENS_REG_EXPOSURE_L,
				   val & 0xf0);
}

static int smartsens_set_vblank(struct smartsens_sensor *sensor, s32 vblank)
{
	u32 vts = sensor->chip->height + vblank;
	int ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_VTS_H,
				  (vts >> 8) & 0x7f);
	if (ret)
		return ret;

	return smartsens_write_reg(sensor, SMARTSENS_REG_VTS_L, vts & 0xff);
}

static int smartsens_set_flip(struct smartsens_sensor *sensor, s32 hflip,
			      s32 vflip)
{
	u8 val;
	int ret;

	ret = smartsens_read_reg(sensor, SMARTSENS_REG_FLIP, &val);
	if (ret)
		return ret;

	val &= ~SMARTSENS_FLIP_MASK;
	if (hflip)
		val |= SMARTSENS_MIRROR;
	if (vflip)
		val |= SMARTSENS_FLIP;

	return smartsens_write_reg(sensor, SMARTSENS_REG_FLIP, val);
}

static int smartsens_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct smartsens_sensor *sensor = to_smartsens(sd);
	int ret;

	if (sensor->removed)
		return -ENODEV;

	if (ctrl->id == V4L2_CID_VBLANK) {
		s32 max = sensor->chip->height + ctrl->val -
			  SMARTSENS_EXPOSURE_MARGIN;

		ret = __v4l2_ctrl_modify_range(sensor->exposure,
					       sensor->exposure->minimum, max,
					       sensor->exposure->step,
					       min(max,
						   SMARTSENS_EXPOSURE_DEFAULT));
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
		return smartsens_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sensor->chip->set_gain(sensor, ctrl->val);
	case V4L2_CID_VBLANK:
		return smartsens_set_vblank(sensor, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops smartsens_ctrl_ops = {
	.s_ctrl = smartsens_s_ctrl,
};

static int smartsens_init_controls(struct smartsens_sensor *sensor)
{
	const struct smartsens_chip *chip = sensor->chip;
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *hblank, *pixel_rate;
	u32 vblank_min = chip->intervals[0].vts - chip->height;
	u32 vblank_def = chip->vts_default - chip->height;
	s32 exposure_max = chip->vts_default - SMARTSENS_EXPOSURE_MARGIN;
	int ret;

	v4l2_ctrl_handler_init(hdl, 7);
	hdl->lock = &sensor->lock;

	pixel_rate = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_PIXEL_RATE,
				       chip->pixel_rate, chip->pixel_rate, 1,
				       chip->pixel_rate);
	if (pixel_rate)
		pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	hblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_HBLANK,
				   chip->hts - chip->width,
				   chip->hts - chip->width, 1,
				   chip->hts - chip->width);
	if (hblank)
		hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->vblank = v4l2_ctrl_new_std(hdl, &smartsens_ctrl_ops,
					   V4L2_CID_VBLANK, vblank_min,
					   SMARTSENS_VTS_MAX - chip->height, 1,
					   vblank_def);

	sensor->exposure = v4l2_ctrl_new_std(hdl, &smartsens_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     SMARTSENS_EXPOSURE_MIN,
					     exposure_max, 1,
					     min(exposure_max,
						 SMARTSENS_EXPOSURE_DEFAULT));

	sensor->gain = v4l2_ctrl_new_std(hdl, &smartsens_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 SMARTSENS_GAIN_MIN, chip->gain_max, 1,
					 SMARTSENS_GAIN_MIN);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &smartsens_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);

	sensor->vflip = v4l2_ctrl_new_std(hdl, &smartsens_ctrl_ops,
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

static int smartsens_power_on(struct smartsens_sensor *sensor)
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

static void smartsens_power_off(struct smartsens_sensor *sensor)
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

static int smartsens_start_streaming(struct smartsens_sensor *sensor)
{
	const struct smartsens_chip *chip = sensor->chip;
	unsigned int i;
	int ret;

	ret = smartsens_power_on(sensor);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_RESET, SMARTSENS_RESET);
	if (ret)
		goto err_stop;
	usleep_range(1000, 2000);

	for (i = 0; i < chip->num_regs; i++) {
		ret = smartsens_write_reg(sensor, chip->regs[i].reg,
					  chip->regs[i].val);
		if (ret)
			goto err_stop;
	}

	ret = smartsens_set_vblank(sensor, sensor->vblank->val);
	if (ret)
		goto err_stop;

	ret = smartsens_set_exposure(sensor, sensor->exposure->val);
	if (ret)
		goto err_stop;

	ret = chip->set_gain(sensor, sensor->gain->val);
	if (ret)
		goto err_stop;

	ret = smartsens_set_flip(sensor, sensor->hflip->val,
				 sensor->vflip->val);
	if (ret)
		goto err_stop;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_STREAM,
				  SMARTSENS_STREAM_ON);
	if (ret)
		goto err_stop;

	sensor->streaming = true;
	return 0;

err_stop:
	smartsens_power_off(sensor);
	return ret;
}

/* The next start resets the sensor and reloads the complete mode table */
static int smartsens_stop_streaming(struct smartsens_sensor *sensor)
{
	smartsens_write_reg(sensor, SMARTSENS_REG_STREAM, SMARTSENS_STREAM_OFF);
	smartsens_power_off(sensor);
	return 0;
}

static int smartsens_s_stream(struct v4l2_subdev *sd, int on)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);
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
		ret = smartsens_start_streaming(sensor);
	else
		ret = smartsens_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void smartsens_frame_interval(struct smartsens_sensor *sensor,
				     s32 vblank, struct v4l2_fract *interval)
{
	const struct smartsens_chip *chip = sensor->chip;
	unsigned long n, d, g;
	unsigned int i;

	for (i = 0; i < chip->num_intervals; i++) {
		if (chip->intervals[i].vts == chip->height + vblank) {
			*interval = chip->intervals[i].interval;
			return;
		}
	}

	n = chip->hts * (chip->height + vblank);
	d = chip->pixel_rate;
	g = gcd(n, d);
	interval->numerator = n / g;
	interval->denominator = d / g;
}

static int
smartsens_g_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_frame_interval *interval)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	smartsens_frame_interval(sensor, sensor->vblank->val,
				 &interval->interval);
	mutex_unlock(&sensor->lock);

	return 0;
}

static int
smartsens_s_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_frame_interval *interval)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);
	const struct smartsens_chip *chip = sensor->chip;
	struct v4l2_fract *req = &interval->interval;
	u32 vts = chip->vts_default;
	u64 best_diff = U64_MAX, req_us;
	unsigned int i;
	int ret;

	if (interval->pad != 0)
		return -EINVAL;

	if (req->numerator && req->denominator) {
		req_us = div_u64((u64)req->numerator * USEC_PER_SEC,
				 req->denominator);
		for (i = 0; i < chip->num_intervals; i++) {
			const struct v4l2_fract *f =
				&chip->intervals[i].interval;
			u64 us = div_u64((u64)f->numerator * USEC_PER_SEC,
					 f->denominator);
			u64 diff = us > req_us ? us - req_us : req_us - us;

			if (diff < best_diff) {
				best_diff = diff;
				vts = chip->intervals[i].vts;
			}
		}
	}

	mutex_lock(&sensor->lock);
	ret = __v4l2_ctrl_s_ctrl(sensor->vblank, vts - chip->height);
	smartsens_frame_interval(sensor, sensor->vblank->val,
				 &interval->interval);
	mutex_unlock(&sensor->lock);

	return ret;
}

static void smartsens_fill_format(struct smartsens_sensor *sensor,
				  struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = sensor->chip->width,
		.height = sensor->chip->height,
		.code = SMARTSENS_MBUS_CODE,
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int smartsens_get_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_format *fmt)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	else
		fmt->format = sensor->fmt;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int smartsens_set_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_format *fmt)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}

	smartsens_fill_format(sensor, &fmt->format);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int smartsens_enum_mbus_code(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	code->code = SMARTSENS_MBUS_CODE;
	return 0;
}

static int smartsens_enum_frame_size(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);

	if (fse->pad != 0 || fse->index != 0 ||
	    fse->code != SMARTSENS_MBUS_CODE)
		return -EINVAL;

	fse->min_width = sensor->chip->width;
	fse->max_width = sensor->chip->width;
	fse->min_height = sensor->chip->height;
	fse->max_height = sensor->chip->height;
	return 0;
}

static int
smartsens_enum_frame_interval(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct smartsens_sensor *sensor = to_smartsens(sd);
	const struct smartsens_chip *chip = sensor->chip;

	if (fie->pad != 0 || fie->index >= chip->num_intervals ||
	    fie->code != SMARTSENS_MBUS_CODE ||
	    fie->width != chip->width || fie->height != chip->height)
		return -EINVAL;

	fie->interval = chip->intervals[fie->index].interval;
	return 0;
}

static int smartsens_init_cfg(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return smartsens_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops smartsens_video_ops = {
	.s_stream = smartsens_s_stream,
	.g_frame_interval = smartsens_g_frame_interval,
	.s_frame_interval = smartsens_s_frame_interval,
};

static const struct v4l2_subdev_core_ops smartsens_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops smartsens_pad_ops = {
	.init_cfg = smartsens_init_cfg,
	.enum_mbus_code = smartsens_enum_mbus_code,
	.enum_frame_size = smartsens_enum_frame_size,
	.enum_frame_interval = smartsens_enum_frame_interval,
	.get_fmt = smartsens_get_fmt,
	.set_fmt = smartsens_set_fmt,
};

static const struct v4l2_subdev_ops smartsens_subdev_ops = {
	.core = &smartsens_core_ops,
	.video = &smartsens_video_ops,
	.pad = &smartsens_pad_ops,
};

static int smartsens_check_chip_id(struct smartsens_sensor *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = smartsens_read_reg(sensor, SMARTSENS_REG_CHIP_ID_H, &id_h);
	if (ret)
		return ret;

	ret = smartsens_read_reg(sensor, SMARTSENS_REG_CHIP_ID_L, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != sensor->chip->chip_id) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}

	return 0;
}

static int smartsens_check_endpoint(struct device *dev,
				    const struct smartsens_chip *chip)
{
	struct v4l2_fwnode_endpoint bus;
	struct device_node *endpoint;
	unsigned int i;
	int ret;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint)
		return -EINVAL;
	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint), &bus);
	of_node_put(endpoint);
	if (ret)
		return ret;

	if (bus.bus_type != V4L2_MBUS_CSI2 ||
	    bus.bus.mipi_csi2.num_data_lanes != chip->lanes ||
	    bus.bus.mipi_csi2.clock_lane != 0)
		return -EINVAL;

	for (i = 0; i < chip->lanes; i++)
		if (bus.bus.mipi_csi2.data_lanes[i] != i + 1)
			return -EINVAL;

	for (i = 0; i <= chip->lanes; i++)
		if (bus.bus.mipi_csi2.lane_polarities[i])
			return -EINVAL;

	return 0;
}

int smartsens_probe(struct i2c_client *client,
		    const struct smartsens_chip *chip)
{
	struct device *dev = &client->dev;
	struct smartsens_sensor *sensor;
	int ret;

	ret = smartsens_check_endpoint(dev, chip);
	if (ret)
		return ret;

	sensor = kzalloc(sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->client = client;
	sensor->chip = chip;
	kref_init(&sensor->ref);
	sensor->lifetime.ref = &sensor->ref;
	sensor->lifetime.release = smartsens_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &smartsens_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &smartsens_internal_ops;
	snprintf(sensor->sd.name, sizeof(sensor->sd.name), "%s %d-%04x",
		 chip->name, i2c_adapter_id(client->adapter), client->addr);

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

	ret = smartsens_power_on(sensor);
	if (ret)
		goto err_put;

	ret = smartsens_check_chip_id(sensor);
	if (ret)
		goto err_power;

	smartsens_power_off(sensor);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;

	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_power;

	ret = smartsens_init_controls(sensor);
	if (ret)
		goto err_power;
	smartsens_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		v4l2_async_unregister_subdev(&sensor->sd);
		goto err_power;
	}

	return 0;

err_power:
	smartsens_power_off(sensor);
err_put:
	kref_put(&sensor->ref, smartsens_release);
	return ret;
}
EXPORT_SYMBOL_GPL(smartsens_probe);

int smartsens_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct smartsens_sensor *sensor = to_smartsens(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	if (sensor->streaming)
		smartsens_write_reg(sensor, SMARTSENS_REG_STREAM,
				    SMARTSENS_STREAM_OFF);
	smartsens_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, smartsens_release);
	return 0;
}
EXPORT_SYMBOL_GPL(smartsens_remove);

MODULE_DESCRIPTION("Common code for SmartSens MIPI CSI-2 camera sensors");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
