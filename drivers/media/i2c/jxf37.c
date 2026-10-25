// SPDX-License-Identifier: GPL-2.0
/*
 * Silicon Optronics (SOI) JX-F37 2-lane MIPI CSI-2 camera sensor driver
 *
 * 1920x1080 RAW10 (SBGGR10) at 25 frames/s, 24 MHz input clock.
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

#include "jxf37-modes.h"

#define JXF37_CHIP_ID_H_REG		0x0a
#define JXF37_CHIP_ID_L_REG		0x0b
#define JXF37_CHIP_ID			0x0f37

#define JXF37_REG_GAIN			0x00
#define JXF37_REG_EXP_L			0x01
#define JXF37_REG_EXP_H			0x02
#define JXF37_REG_SUN_SPOT_C		0x0c
#define JXF37_REG_MODE			0x12
#define JXF37_MODE_STANDBY		BIT(6)
#define JXF37_MODE_VFLIP_INVERT		BIT(5)
#define JXF37_MODE_HFLIP		BIT(4)
#define JXF37_REG_HFLIP_CFG		0x28
#define JXF37_REG_VFLIP_CFG		0x80
#define JXF37_REG_SUN_SPOT_82		0x82

/* Gain at which sun-spot suppression is enabled (Q6, 2.0x) */
#define JXF37_SUN_SPOT_GAIN		128

#define JXF37_WIDTH			1920
#define JXF37_HEIGHT			1080
#define JXF37_FRAME_LINES		1125

/* 2304 clocks per line and 1125 lines at 64.8 MHz give 25 fps */
#define JXF37_LINE_CLOCKS		2304
#define JXF37_PIXEL_RATE		64800000
#define JXF37_HBLANK			(JXF37_LINE_CLOCKS - JXF37_WIDTH)
#define JXF37_VBLANK			(JXF37_FRAME_LINES - JXF37_HEIGHT)
#define JXF37_EXPOSURE_MARGIN		5

struct jxf37_mode {
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
};

static const struct jxf37_mode jxf37_modes[] = {
	{
		.width = JXF37_WIDTH,
		.height = JXF37_HEIGHT,
		.max_fps = { .numerator = 1, .denominator = 25 },
	},
};

struct jxf37_dev {
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
	const struct jxf37_mode	*cur_mode;
	struct v4l2_mbus_framefmt fmt;
};

static inline struct jxf37_dev *to_jxf37(struct v4l2_subdev *sd)
{
	return container_of(sd, struct jxf37_dev, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct jxf37_dev, ctrls)->sd;
}

static void jxf37_release(struct kref *ref)
{
	struct jxf37_dev *sensor = container_of(ref, struct jxf37_dev, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int jxf37_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_jxf37(sd)->ref);
	return 0;
}

static void jxf37_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_jxf37(sd)->ref, jxf37_release);
}

static const struct v4l2_subdev_internal_ops jxf37_internal_ops = {
	.registered = jxf37_registered,
	.unregistered = jxf37_subdev_release,
};

static u32 jxf37_bayer_code(struct jxf37_dev *sensor)
{
	/*
	 * Shift the readout window when mirroring or flipping to keep the
	 * Bayer order SBGGR10 in every orientation.
	 */
	return MEDIA_BUS_FMT_SBGGR10_1X10;
}

static int jxf37_read_reg(struct i2c_client *client, u8 reg, u8 *val)
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

static int jxf37_write_reg(struct i2c_client *client, u8 reg, u8 val)
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

static int jxf37_set_exposure(struct jxf37_dev *sensor, s32 val)
{
	int ret;

	ret = jxf37_write_reg(sensor->client, JXF37_REG_EXP_H,
			      (val >> 8) & 0xff);
	if (ret)
		return ret;

	return jxf37_write_reg(sensor->client, JXF37_REG_EXP_L, val & 0xff);
}

/* Gain encoding uses a three-bit exponent and a four-bit fraction. */
static int jxf37_set_gain(struct jxf37_dev *sensor, s32 gain)
{
	u8 exp = 0;
	u8 frac;
	u8 reg_val;
	int ret;

	if (gain < 64)
		gain = 64;

	if (gain >= 128) {
		u32 temp = gain >> 7;

		while (temp > 0) {
			exp++;
			temp >>= 1;
		}
		if (exp > 7)
			exp = 7;
	}

	frac = ((gain >> 2) >> exp) - 16;
	if (frac > 15)
		frac = 15;

	reg_val = (exp << 4) | (frac & 0x0f);
	ret = jxf37_write_reg(sensor->client, JXF37_REG_GAIN, reg_val);
	if (ret)
		return ret;

	if (gain > JXF37_SUN_SPOT_GAIN) {
		ret = jxf37_write_reg(sensor->client,
				      JXF37_REG_SUN_SPOT_C, 0x40);
		if (ret)
			return ret;
		return jxf37_write_reg(sensor->client,
				       JXF37_REG_SUN_SPOT_82, 0x23);
	}

	ret = jxf37_write_reg(sensor->client, JXF37_REG_SUN_SPOT_C, 0x00);
	if (ret)
		return ret;
	return jxf37_write_reg(sensor->client, JXF37_REG_SUN_SPOT_82, 0x21);
}

static int jxf37_set_flip(struct jxf37_dev *sensor, s32 hflip, s32 vflip)
{
	struct i2c_client *client = sensor->client;
	u8 val_12 = 0;
	u8 val_28;
	u8 val_80;
	int ret;

	if (!sensor->streaming)
		val_12 |= JXF37_MODE_STANDBY;

	if (hflip) {
		val_28 = 0x22;
		val_12 |= JXF37_MODE_HFLIP;
	} else {
		val_28 = 0x19;
	}

	if (!vflip)
		val_12 |= JXF37_MODE_VFLIP_INVERT;

	val_80 = vflip ? 0x01 : 0x41;

	ret = jxf37_write_reg(client, JXF37_REG_HFLIP_CFG, val_28);
	if (ret)
		return ret;

	ret = jxf37_write_reg(client, JXF37_REG_MODE, val_12);
	if (ret)
		return ret;

	return jxf37_write_reg(client, JXF37_REG_VFLIP_CFG, val_80);
}

static int jxf37_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct jxf37_dev *sensor = to_jxf37(sd);

	if (sensor->removed)
		return -ENODEV;

	if (sensor->streaming &&
	    (ctrl->id == V4L2_CID_HFLIP || ctrl->id == V4L2_CID_VFLIP))
		return -EBUSY;

	if (!sensor->streaming)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return jxf37_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return jxf37_set_gain(sensor, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops jxf37_ctrl_ops = {
	.s_ctrl = jxf37_s_ctrl,
};

static int jxf37_init_controls(struct jxf37_dev *sensor)
{
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *vblank, *hblank, *pixel_rate;
	int ret;

	v4l2_ctrl_handler_init(hdl, 7);
	hdl->lock = &sensor->lock;

	pixel_rate = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_PIXEL_RATE,
				       JXF37_PIXEL_RATE, JXF37_PIXEL_RATE, 1,
				       JXF37_PIXEL_RATE);
	if (pixel_rate)
		pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	hblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_HBLANK,
				   JXF37_HBLANK, JXF37_HBLANK, 1,
				   JXF37_HBLANK);
	if (hblank)
		hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	vblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_VBLANK,
				   JXF37_VBLANK, JXF37_VBLANK, 1,
				   JXF37_VBLANK);
	if (vblank)
		vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->exposure = v4l2_ctrl_new_std(hdl, &jxf37_ctrl_ops,
					     V4L2_CID_EXPOSURE, 1,
					     JXF37_FRAME_LINES -
					     JXF37_EXPOSURE_MARGIN, 1, 464);

	/* Gain in Q6 scale (64 = 1.0x) */
	sensor->gain = v4l2_ctrl_new_std(hdl, &jxf37_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 64, 15872, 1, 64);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &jxf37_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);

	sensor->vflip = v4l2_ctrl_new_std(hdl, &jxf37_ctrl_ops,
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

static int jxf37_power_on(struct jxf37_dev *sensor)
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

static void jxf37_power_off(struct jxf37_dev *sensor)
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

static int jxf37_start_streaming(struct jxf37_dev *sensor)
{
	int i, ret;

	ret = jxf37_power_on(sensor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(jxf37_1080p_regs); i++) {
		ret = jxf37_write_reg(sensor->client,
				      jxf37_1080p_regs[i].reg,
				      jxf37_1080p_regs[i].val);
		if (ret)
			goto err_stop;
		usleep_range(1000, 2000);
	}

	ret = jxf37_set_exposure(sensor, sensor->exposure->val);
	if (ret)
		goto err_stop;

	ret = jxf37_set_gain(sensor, sensor->gain->val);
	if (ret)
		goto err_stop;

	/* set_flip() leaves standby only once streaming is set */
	sensor->streaming = true;

	ret = jxf37_set_flip(sensor,
			     sensor->hflip->val, sensor->vflip->val);
	if (ret)
		goto err_stop;

	return 0;

err_stop:
	sensor->streaming = false;
	jxf37_power_off(sensor);
	return ret;
}

static int jxf37_stop_streaming(struct jxf37_dev *sensor)
{
	jxf37_write_reg(sensor->client, JXF37_REG_MODE,
			JXF37_MODE_STANDBY | JXF37_MODE_VFLIP_INVERT);

	jxf37_power_off(sensor);
	return 0;
}

static int jxf37_s_stream(struct v4l2_subdev *sd, int on)
{
	struct jxf37_dev *sensor = to_jxf37(sd);
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
		ret = jxf37_start_streaming(sensor);
	else
		ret = jxf37_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int jxf37_g_frame_interval(struct v4l2_subdev *sd,
				  struct v4l2_subdev_frame_interval *fi)
{
	struct jxf37_dev *sensor = to_jxf37(sd);

	if (fi->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	fi->interval = sensor->cur_mode->max_fps;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int jxf37_s_frame_interval(struct v4l2_subdev *sd,
				  struct v4l2_subdev_frame_interval *fi)
{
	struct jxf37_dev *sensor = to_jxf37(sd);
	const struct jxf37_mode *mode = &jxf37_modes[0];
	int ret = 0;

	if (fi->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (sensor->streaming) {
		ret = -EBUSY;
		goto unlock;
	}

	sensor->cur_mode = mode;
	fi->interval = mode->max_fps;

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void jxf37_fill_format(struct jxf37_dev *sensor,
			      struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = JXF37_WIDTH,
		.height = JXF37_HEIGHT,
		.code = jxf37_bayer_code(sensor),
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int jxf37_get_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *fmt)
{
	struct jxf37_dev *sensor = to_jxf37(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	} else {
		fmt->format = sensor->fmt;
		fmt->format.code = jxf37_bayer_code(sensor);
	}
	mutex_unlock(&sensor->lock);

	return 0;
}

static int jxf37_set_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *fmt)
{
	struct jxf37_dev *sensor = to_jxf37(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}

	jxf37_fill_format(sensor, &fmt->format);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int jxf37_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	struct jxf37_dev *sensor = to_jxf37(sd);

	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	code->code = jxf37_bayer_code(sensor);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int jxf37_enum_frame_size(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_frame_size_enum *fse)
{
	struct jxf37_dev *sensor = to_jxf37(sd);
	int ret = 0;

	if (fse->pad != 0 || fse->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fse->code != jxf37_bayer_code(sensor)) {
		ret = -EINVAL;
	} else {
		fse->min_width = JXF37_WIDTH;
		fse->max_width = JXF37_WIDTH;
		fse->min_height = JXF37_HEIGHT;
		fse->max_height = JXF37_HEIGHT;
	}
	mutex_unlock(&sensor->lock);
	return ret;
}

static int
jxf37_enum_frame_interval(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_frame_interval_enum *fie)
{
	struct jxf37_dev *sensor = to_jxf37(sd);

	if (fie->pad != 0 || fie->index >= ARRAY_SIZE(jxf37_modes))
		return -EINVAL;

	if (fie->width != JXF37_WIDTH || fie->height != JXF37_HEIGHT)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fie->code != jxf37_bayer_code(sensor)) {
		mutex_unlock(&sensor->lock);
		return -EINVAL;
	}
	fie->interval = jxf37_modes[fie->index].max_fps;
	mutex_unlock(&sensor->lock);
	return 0;
}

static int jxf37_init_cfg(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return jxf37_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops jxf37_video_ops = {
	.s_stream = jxf37_s_stream,
	.g_frame_interval = jxf37_g_frame_interval,
	.s_frame_interval = jxf37_s_frame_interval,
};

static const struct v4l2_subdev_core_ops jxf37_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops jxf37_pad_ops = {
	.init_cfg = jxf37_init_cfg,
	.enum_mbus_code = jxf37_enum_mbus_code,
	.enum_frame_size = jxf37_enum_frame_size,
	.enum_frame_interval = jxf37_enum_frame_interval,
	.get_fmt = jxf37_get_fmt,
	.set_fmt = jxf37_set_fmt,
};

static const struct v4l2_subdev_ops jxf37_subdev_ops = {
	.core = &jxf37_core_ops,
	.video = &jxf37_video_ops,
	.pad = &jxf37_pad_ops,
};

static int jxf37_check_chip_id(struct jxf37_dev *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = jxf37_read_reg(sensor->client, JXF37_CHIP_ID_H_REG, &id_h);
	if (ret)
		return ret;

	ret = jxf37_read_reg(sensor->client, JXF37_CHIP_ID_L_REG, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != JXF37_CHIP_ID) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}

	return 0;
}

static int jxf37_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct jxf37_dev *sensor;
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
	sensor->cur_mode = &jxf37_modes[0];
	kref_init(&sensor->ref);
	sensor->lifetime.ref = &sensor->ref;
	sensor->lifetime.release = jxf37_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &jxf37_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &jxf37_internal_ops;

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

	ret = jxf37_power_on(sensor);
	if (ret)
		goto err_put;

	ret = jxf37_check_chip_id(sensor);
	if (ret)
		goto err_power;

	jxf37_power_off(sensor);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;

	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_power;

	ret = jxf37_init_controls(sensor);
	if (ret)
		goto err_power;
	jxf37_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		v4l2_async_unregister_subdev(&sensor->sd);
		goto err_power;
	}

	return 0;

err_power:
	jxf37_power_off(sensor);
err_put:
	kref_put(&sensor->ref, jxf37_release);
	return ret;
}

static int jxf37_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct jxf37_dev *sensor = to_jxf37(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	jxf37_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, jxf37_release);
	return 0;
}

static const struct of_device_id jxf37_of_match[] = {
	{ .compatible = "soi,jxf37" },
	{ }
};
MODULE_DEVICE_TABLE(of, jxf37_of_match);

static const struct i2c_device_id jxf37_id[] = {
	{ "jxf37", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, jxf37_id);

static struct i2c_driver jxf37_i2c_driver = {
	.driver = {
		.name = "jxf37",
		.of_match_table = jxf37_of_match,
	},
	.probe		= jxf37_probe,
	.remove		= jxf37_remove,
	.id_table	= jxf37_id,
};
module_i2c_driver(jxf37_i2c_driver);

MODULE_DESCRIPTION("Silicon Optronics JX-F37 2-lane MIPI CSI-2 camera sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
