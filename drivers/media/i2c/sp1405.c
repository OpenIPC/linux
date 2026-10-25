// SPDX-License-Identifier: GPL-2.0
/*
 * Superpix SP1405 (OV9732 compatible) MIPI CSI-2 camera sensor driver
 *
 * 1280x720 RAW10 (SBGGR10) at 15 frames/s, one data lane, 24 MHz input
 * clock, 16-bit register addresses.
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

#include "sp1405-modes.h"

#define SP1405_CHIP_ID_H_REG		0x300a
#define SP1405_CHIP_ID_L_REG		0x300b
#define SP1405_CHIP_ID			0x9732

#define SP1405_MODE_SELECT		0x0100
#define SP1405_MODE_STANDBY		0x00
#define SP1405_MODE_STREAMING		0x01

#define SP1405_REG_EXPOSURE_H		0x3501
#define SP1405_REG_EXPOSURE_L		0x3502
#define SP1405_REG_GAIN			0x350b

#define SP1405_REG_TIMING_FORMAT1	0x3820
#define SP1405_TIMING_HFLIP		BIT(2)
#define SP1405_TIMING_VFLIP		BIT(3)

#define SP1405_WIDTH			1280
#define SP1405_HEIGHT			720
#define SP1405_FRAME_LINES		1000
#define SP1405_EXPOSURE_MARGIN		5

/* 2400 clocks per line and 1000 lines at a 36 MHz timing clock give 15 fps */
#define SP1405_LINE_CLOCKS		2400
#define SP1405_PIXEL_RATE		36000000
#define SP1405_HBLANK			(SP1405_LINE_CLOCKS - SP1405_WIDTH)
#define SP1405_VBLANK			(SP1405_FRAME_LINES - SP1405_HEIGHT)
#define SP1405_INTERVAL_NUM \
	(SP1405_LINE_CLOCKS * SP1405_FRAME_LINES)
#define SP1405_INTERVAL_DEN		SP1405_PIXEL_RATE

struct sp1405_dev {
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
	struct v4l2_mbus_framefmt fmt;
};

static inline struct sp1405_dev *to_sp1405(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sp1405_dev, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct sp1405_dev, ctrls)->sd;
}

static void sp1405_release(struct kref *ref)
{
	struct sp1405_dev *sensor = container_of(ref, struct sp1405_dev, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int sp1405_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_sp1405(sd)->ref);
	return 0;
}

static void sp1405_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_sp1405(sd)->ref, sp1405_release);
}

static const struct v4l2_subdev_internal_ops sp1405_internal_ops = {
	.registered = sp1405_registered,
	.unregistered = sp1405_subdev_release,
};

static u32 sp1405_bayer_code(struct sp1405_dev *sensor)
{
	/* The sensor keeps the SBGGR order for every mirror/flip combination */
	return MEDIA_BUS_FMT_SBGGR10_1X10;
}

static int sp1405_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	u8 reg_buf[2] = { (reg >> 8) & 0xff, reg & 0xff };
	struct i2c_msg msgs[2];

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = reg_buf;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = val;

	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret != 2) {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&client->dev, "failed to read reg 0x%04x: %d\n",
			reg, ret);
		return ret;
	}
	return 0;
}

static int sp1405_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	int ret;
	u8 buf[3] = { (reg >> 8) & 0xff, reg & 0xff, val };

	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&client->dev, "failed to write reg 0x%04x=0x%02x: %d\n",
			reg, val, ret);
		return ret;
	}
	return 0;
}

/* Exposure uses 12 bits with the low four bits in the top nibble. */
static int sp1405_set_exposure(struct sp1405_dev *sensor, s32 val)
{
	int ret;

	ret = sp1405_write_reg(sensor->client, SP1405_REG_EXPOSURE_L,
			       (val & 0x0f) << 4);
	if (ret)
		return ret;

	return sp1405_write_reg(sensor->client, SP1405_REG_EXPOSURE_H,
				(val >> 4) & 0xff);
}

/* The Q6 gain is converted to the Q4 register format and clamped to 1x-15.5x */
static int sp1405_set_gain(struct sp1405_dev *sensor, s32 gain)
{
	u32 val = gain >> 2;

	if (val > 0xf8)
		val = 0xf8;
	if (val < 0x10)
		val = 0x10;

	return sp1405_write_reg(sensor->client, SP1405_REG_GAIN, val);
}

/* Update the horizontal mirror and vertical flip settings. */
static int sp1405_set_flip(struct sp1405_dev *sensor, s32 hflip, s32 vflip)
{
	struct i2c_client *client = sensor->client;
	u8 val;
	int ret;

	ret = sp1405_read_reg(client, SP1405_REG_TIMING_FORMAT1, &val);
	if (ret)
		return ret;

	val &= 0xf3;
	if (hflip)
		val |= SP1405_TIMING_HFLIP;
	if (vflip)
		val |= SP1405_TIMING_VFLIP;

	return sp1405_write_reg(client, SP1405_REG_TIMING_FORMAT1, val);
}

static int sp1405_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct sp1405_dev *sensor = to_sp1405(sd);

	if (sensor->removed)
		return -ENODEV;

	if (sensor->streaming &&
	    (ctrl->id == V4L2_CID_HFLIP || ctrl->id == V4L2_CID_VFLIP))
		return -EBUSY;

	if (!sensor->streaming)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return sp1405_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sp1405_set_gain(sensor, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops sp1405_ctrl_ops = {
	.s_ctrl = sp1405_s_ctrl,
};

static int sp1405_init_controls(struct sp1405_dev *sensor)
{
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *vblank, *hblank, *pixel_rate;
	int ret;

	v4l2_ctrl_handler_init(hdl, 7);
	hdl->lock = &sensor->lock;

	pixel_rate = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_PIXEL_RATE,
				       SP1405_PIXEL_RATE, SP1405_PIXEL_RATE, 1,
				       SP1405_PIXEL_RATE);
	if (pixel_rate)
		pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	hblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_HBLANK,
				   SP1405_HBLANK, SP1405_HBLANK, 1,
				   SP1405_HBLANK);
	if (hblank)
		hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	vblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_VBLANK,
				   SP1405_VBLANK, SP1405_VBLANK, 1,
				   SP1405_VBLANK);
	if (vblank)
		vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->exposure = v4l2_ctrl_new_std(hdl, &sp1405_ctrl_ops,
					     V4L2_CID_EXPOSURE, 16,
					     SP1405_FRAME_LINES -
					     SP1405_EXPOSURE_MARGIN, 1, 500);

	/* Gain in Q6 scale (64 = 1.0x) */
	sensor->gain = v4l2_ctrl_new_std(hdl, &sp1405_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 64, 0x3e0, 1, 64);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &sp1405_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);

	sensor->vflip = v4l2_ctrl_new_std(hdl, &sp1405_ctrl_ops,
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

static int sp1405_power_on(struct sp1405_dev *sensor)
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

static void sp1405_power_off(struct sp1405_dev *sensor)
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

static int sp1405_start_streaming(struct sp1405_dev *sensor)
{
	int i, ret;

	ret = sp1405_power_on(sensor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(sp1405_720p_regs); i++) {
		ret = sp1405_write_reg(sensor->client,
				       sp1405_720p_regs[i].reg,
				       sp1405_720p_regs[i].val);
		if (ret)
			goto err_stop;
		usleep_range(1000, 2000);
	}

	ret = sp1405_set_exposure(sensor, sensor->exposure->val);
	if (ret)
		goto err_stop;

	ret = sp1405_set_gain(sensor, sensor->gain->val);
	if (ret)
		goto err_stop;

	ret = sp1405_set_flip(sensor,
			      sensor->hflip->val, sensor->vflip->val);
	if (ret)
		goto err_stop;

	ret = sp1405_write_reg(sensor->client, SP1405_MODE_SELECT,
			       SP1405_MODE_STREAMING);
	if (ret)
		goto err_stop;

	sensor->streaming = true;
	return 0;

err_stop:
	sp1405_power_off(sensor);
	return ret;
}

static int sp1405_stop_streaming(struct sp1405_dev *sensor)
{
	sp1405_write_reg(sensor->client, SP1405_MODE_SELECT,
			 SP1405_MODE_STANDBY);
	sp1405_power_off(sensor);
	return 0;
}

static int sp1405_s_stream(struct v4l2_subdev *sd, int on)
{
	struct sp1405_dev *sensor = to_sp1405(sd);
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
		ret = sp1405_start_streaming(sensor);
	else
		ret = sp1405_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int sp1405_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct sp1405_dev *sensor = to_sp1405(sd);

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	interval->interval.numerator = SP1405_INTERVAL_NUM;
	interval->interval.denominator = SP1405_INTERVAL_DEN;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int sp1405_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct sp1405_dev *sensor = to_sp1405(sd);
	int ret = 0;

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (sensor->streaming) {
		ret = -EBUSY;
		goto unlock;
	}

	interval->interval.numerator = SP1405_INTERVAL_NUM;
	interval->interval.denominator = SP1405_INTERVAL_DEN;

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void sp1405_fill_format(struct sp1405_dev *sensor,
			       struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = SP1405_WIDTH,
		.height = SP1405_HEIGHT,
		.code = sp1405_bayer_code(sensor),
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int sp1405_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct sp1405_dev *sensor = to_sp1405(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	} else {
		fmt->format = sensor->fmt;
		fmt->format.code = sp1405_bayer_code(sensor);
	}
	mutex_unlock(&sensor->lock);

	return 0;
}

static int sp1405_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct sp1405_dev *sensor = to_sp1405(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}
	sp1405_fill_format(sensor, &fmt->format);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int sp1405_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct sp1405_dev *sensor = to_sp1405(sd);

	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	code->code = sp1405_bayer_code(sensor);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int sp1405_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct sp1405_dev *sensor = to_sp1405(sd);
	int ret = 0;

	if (fse->pad != 0 || fse->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fse->code != sp1405_bayer_code(sensor)) {
		ret = -EINVAL;
	} else {
		fse->min_width = SP1405_WIDTH;
		fse->max_width = SP1405_WIDTH;
		fse->min_height = SP1405_HEIGHT;
		fse->max_height = SP1405_HEIGHT;
	}
	mutex_unlock(&sensor->lock);
	return ret;
}

static int
sp1405_enum_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_interval_enum *fie)
{
	struct sp1405_dev *sensor = to_sp1405(sd);

	if (fie->pad != 0 || fie->index != 0)
		return -EINVAL;

	if (fie->width != SP1405_WIDTH || fie->height != SP1405_HEIGHT)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fie->code != sp1405_bayer_code(sensor)) {
		mutex_unlock(&sensor->lock);
		return -EINVAL;
	}
	fie->interval.numerator = SP1405_INTERVAL_NUM;
	fie->interval.denominator = SP1405_INTERVAL_DEN;
	mutex_unlock(&sensor->lock);
	return 0;
}

static int sp1405_init_cfg(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return sp1405_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops sp1405_video_ops = {
	.s_stream = sp1405_s_stream,
	.g_frame_interval = sp1405_g_frame_interval,
	.s_frame_interval = sp1405_s_frame_interval,
};

static const struct v4l2_subdev_core_ops sp1405_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops sp1405_pad_ops = {
	.init_cfg = sp1405_init_cfg,
	.enum_mbus_code = sp1405_enum_mbus_code,
	.enum_frame_size = sp1405_enum_frame_size,
	.enum_frame_interval = sp1405_enum_frame_interval,
	.get_fmt = sp1405_get_fmt,
	.set_fmt = sp1405_set_fmt,
};

static const struct v4l2_subdev_ops sp1405_subdev_ops = {
	.core = &sp1405_core_ops,
	.video = &sp1405_video_ops,
	.pad = &sp1405_pad_ops,
};

static int sp1405_check_chip_id(struct sp1405_dev *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = sp1405_read_reg(sensor->client, SP1405_CHIP_ID_H_REG, &id_h);
	if (ret)
		return ret;

	ret = sp1405_read_reg(sensor->client, SP1405_CHIP_ID_L_REG, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != SP1405_CHIP_ID) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}

	return 0;
}

static int sp1405_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct sp1405_dev *sensor;
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
	sensor->lifetime.release = sp1405_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &sp1405_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &sp1405_internal_ops;

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

	ret = sp1405_power_on(sensor);
	if (ret)
		goto err_put;

	ret = sp1405_check_chip_id(sensor);
	if (ret)
		goto err_power;

	sp1405_power_off(sensor);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;

	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_power;

	ret = sp1405_init_controls(sensor);
	if (ret)
		goto err_power;
	sp1405_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		v4l2_async_unregister_subdev(&sensor->sd);
		goto err_power;
	}

	return 0;

err_power:
	sp1405_power_off(sensor);
err_put:
	kref_put(&sensor->ref, sp1405_release);
	return ret;
}

static int sp1405_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sp1405_dev *sensor = to_sp1405(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	sp1405_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, sp1405_release);
	return 0;
}

static const struct of_device_id sp1405_of_match[] = {
	{ .compatible = "superpix,sp1405" },
	{ .compatible = "ovti,ov9732" },
	{ }
};
MODULE_DEVICE_TABLE(of, sp1405_of_match);

static const struct i2c_device_id sp1405_id[] = {
	{ "sp1405", 0 },
	{ "ov9732", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sp1405_id);

static struct i2c_driver sp1405_i2c_driver = {
	.driver = {
		.name = "sp1405",
		.of_match_table = sp1405_of_match,
	},
	.probe		= sp1405_probe,
	.remove		= sp1405_remove,
	.id_table	= sp1405_id,
};
module_i2c_driver(sp1405_i2c_driver);

MODULE_DESCRIPTION("Superpix SP1405 (OV9732 compatible) MIPI CSI-2 camera sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
