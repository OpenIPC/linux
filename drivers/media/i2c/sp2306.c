// SPDX-License-Identifier: GPL-2.0
/*
 * Superpix SP2306 MIPI CSI-2 camera sensor driver
 *
 * 1920x1080 RAW10 at a nominal 25 frames/s, two data lanes, 24 MHz input
 * clock.
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

#include "sp2306-modes.h"

#define SP2306_CHIP_ID_H		0x02
#define SP2306_CHIP_ID_L		0x03
#define SP2306_CHIP_ID			0x2306

#define SP2306_PAGE_REG			0xfd
#define SP2306_PAGE_0			0x00
#define SP2306_PAGE_1			0x01

#define SP2306_REG_LATCH		0x01
#define SP2306_LATCH_VAL		0x01

#define SP2306_REG_EXP_H		0x03
#define SP2306_REG_EXP_L		0x04

#define SP2306_REG_GAIN			0x24

#define SP2306_REG_FLIP			0x3f
#define SP2306_FLIP_H			BIT(1)
#define SP2306_FLIP_V			BIT(0)

#define SP2306_WIDTH			1920
#define SP2306_HEIGHT			1080
#define SP2306_FRAME_LINES		1104

struct sp2306_dev {
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

static inline struct sp2306_dev *to_sp2306(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sp2306_dev, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct sp2306_dev, ctrls)->sd;
}

static void sp2306_release(struct kref *ref)
{
	struct sp2306_dev *sensor = container_of(ref, struct sp2306_dev, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int sp2306_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_sp2306(sd)->ref);
	return 0;
}

static void sp2306_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_sp2306(sd)->ref, sp2306_release);
}

static const struct v4l2_subdev_internal_ops sp2306_internal_ops = {
	.registered = sp2306_registered,
	.unregistered = sp2306_subdev_release,
};

static u32 sp2306_bayer_code(struct sp2306_dev *sensor)
{
	/* Unflipped output is BGGR; index is hflip | (vflip << 1) */
	static const u32 codes[] = {
		MEDIA_BUS_FMT_SBGGR10_1X10, MEDIA_BUS_FMT_SGRBG10_1X10,
		MEDIA_BUS_FMT_SGBRG10_1X10, MEDIA_BUS_FMT_SRGGB10_1X10,
	};

	return codes[sensor->hflip->val | (sensor->vflip->val << 1)];
}

static int sp2306_read_reg(struct i2c_client *client, u8 reg, u8 *val)
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

static int sp2306_write_reg(struct i2c_client *client, u8 reg, u8 val)
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

/* The page 1 settings below take effect once the latch register is written */
static int sp2306_set_exposure(struct sp2306_dev *sensor, s32 val)
{
	struct i2c_client *client = sensor->client;
	int ret;

	ret = sp2306_write_reg(client, SP2306_PAGE_REG, SP2306_PAGE_1);
	if (ret)
		return ret;

	ret = sp2306_write_reg(client, SP2306_REG_EXP_H, (val >> 8) & 0xff);
	if (ret)
		return ret;

	ret = sp2306_write_reg(client, SP2306_REG_EXP_L, val & 0xff);
	if (ret)
		return ret;

	return sp2306_write_reg(client, SP2306_REG_LATCH, SP2306_LATCH_VAL);
}

static int sp2306_set_gain(struct sp2306_dev *sensor, s32 gain)
{
	struct i2c_client *client = sensor->client;
	int ret;

	/* The Q6 gain (64 = 1.0x) maps to the register as gain >> 2 */
	ret = sp2306_write_reg(client, SP2306_PAGE_REG, SP2306_PAGE_1);
	if (ret)
		return ret;

	ret = sp2306_write_reg(client, SP2306_REG_GAIN, (gain >> 2) & 0xff);
	if (ret)
		return ret;

	return sp2306_write_reg(client, SP2306_REG_LATCH, SP2306_LATCH_VAL);
}

static int sp2306_set_flip(struct sp2306_dev *sensor, s32 hflip, s32 vflip)
{
	struct i2c_client *client = sensor->client;
	u8 val = 0;
	int ret;

	if (hflip)
		val |= SP2306_FLIP_H;
	if (vflip)
		val |= SP2306_FLIP_V;

	ret = sp2306_write_reg(client, SP2306_PAGE_REG, SP2306_PAGE_1);
	if (ret)
		return ret;

	ret = sp2306_write_reg(client, SP2306_REG_FLIP, val);
	if (ret)
		return ret;

	return sp2306_write_reg(client, SP2306_REG_LATCH, SP2306_LATCH_VAL);
}

static int sp2306_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct sp2306_dev *sensor = to_sp2306(sd);

	if (sensor->removed)
		return -ENODEV;

	if (sensor->streaming &&
	    (ctrl->id == V4L2_CID_HFLIP || ctrl->id == V4L2_CID_VFLIP))
		return -EBUSY;

	if (!sensor->streaming)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return sp2306_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return sp2306_set_gain(sensor, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops sp2306_ctrl_ops = {
	.s_ctrl = sp2306_s_ctrl,
};

static int sp2306_init_controls(struct sp2306_dev *sensor)
{
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *vblank;
	int ret;

	v4l2_ctrl_handler_init(hdl, 5);
	hdl->lock = &sensor->lock;

	vblank = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_VBLANK,
				   SP2306_FRAME_LINES - SP2306_HEIGHT,
				   SP2306_FRAME_LINES - SP2306_HEIGHT, 1,
				   SP2306_FRAME_LINES - SP2306_HEIGHT);
	if (vblank)
		vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->exposure = v4l2_ctrl_new_std(hdl, &sp2306_ctrl_ops,
					     V4L2_CID_EXPOSURE, 1,
					     SP2306_FRAME_LINES, 1, 500);

	/* Gain in Q6 scale (64 = 1.0x) */
	sensor->gain = v4l2_ctrl_new_std(hdl, &sp2306_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 64, 1020, 1, 64);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &sp2306_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);

	sensor->vflip = v4l2_ctrl_new_std(hdl, &sp2306_ctrl_ops,
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

static int sp2306_power_on(struct sp2306_dev *sensor)
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

static void sp2306_power_off(struct sp2306_dev *sensor)
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

static int sp2306_start_streaming(struct sp2306_dev *sensor)
{
	int i, ret;

	ret = sp2306_power_on(sensor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(sp2306_1080p_regs); i++) {
		ret = sp2306_write_reg(sensor->client,
				       sp2306_1080p_regs[i].reg,
				       sp2306_1080p_regs[i].val);
		if (ret)
			goto err_stop;
		usleep_range(1000, 2000);
	}

	ret = sp2306_set_exposure(sensor, sensor->exposure->val);
	if (ret)
		goto err_stop;

	ret = sp2306_set_gain(sensor, sensor->gain->val);
	if (ret)
		goto err_stop;

	ret = sp2306_set_flip(sensor,
			      sensor->hflip->val, sensor->vflip->val);
	if (ret)
		goto err_stop;

	sensor->streaming = true;
	return 0;

err_stop:
	sp2306_power_off(sensor);
	return ret;
}

static int sp2306_stop_streaming(struct sp2306_dev *sensor)
{
	sp2306_power_off(sensor);
	return 0;
}

static int sp2306_s_stream(struct v4l2_subdev *sd, int on)
{
	struct sp2306_dev *sensor = to_sp2306(sd);
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
		ret = sp2306_start_streaming(sensor);
	else
		ret = sp2306_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int sp2306_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct sp2306_dev *sensor = to_sp2306(sd);

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	interval->interval.numerator = 1;
	interval->interval.denominator = 25;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int sp2306_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *interval)
{
	struct sp2306_dev *sensor = to_sp2306(sd);
	int ret = 0;

	if (interval->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (sensor->streaming) {
		ret = -EBUSY;
		goto unlock;
	}

	interval->interval.numerator = 1;
	interval->interval.denominator = 25;

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void sp2306_fill_format(struct sp2306_dev *sensor,
			       struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = SP2306_WIDTH,
		.height = SP2306_HEIGHT,
		.code = sp2306_bayer_code(sensor),
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int sp2306_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct sp2306_dev *sensor = to_sp2306(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	} else {
		fmt->format = sensor->fmt;
		fmt->format.code = sp2306_bayer_code(sensor);
	}
	mutex_unlock(&sensor->lock);

	return 0;
}

static int sp2306_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct sp2306_dev *sensor = to_sp2306(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}

	sp2306_fill_format(sensor, &fmt->format);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);

	return 0;
}

static int sp2306_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct sp2306_dev *sensor = to_sp2306(sd);

	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	code->code = sp2306_bayer_code(sensor);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int sp2306_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct sp2306_dev *sensor = to_sp2306(sd);
	int ret = 0;

	if (fse->pad != 0 || fse->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fse->code != sp2306_bayer_code(sensor)) {
		ret = -EINVAL;
	} else {
		fse->min_width = SP2306_WIDTH;
		fse->max_width = SP2306_WIDTH;
		fse->min_height = SP2306_HEIGHT;
		fse->max_height = SP2306_HEIGHT;
	}
	mutex_unlock(&sensor->lock);
	return ret;
}

static int
sp2306_enum_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_interval_enum *fie)
{
	struct sp2306_dev *sensor = to_sp2306(sd);

	if (fie->pad != 0 || fie->index != 0)
		return -EINVAL;

	if (fie->width != SP2306_WIDTH || fie->height != SP2306_HEIGHT)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fie->code != sp2306_bayer_code(sensor)) {
		mutex_unlock(&sensor->lock);
		return -EINVAL;
	}
	fie->interval.numerator = 1;
	fie->interval.denominator = 25;
	mutex_unlock(&sensor->lock);
	return 0;
}

static int sp2306_init_cfg(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return sp2306_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops sp2306_video_ops = {
	.s_stream = sp2306_s_stream,
	.g_frame_interval = sp2306_g_frame_interval,
	.s_frame_interval = sp2306_s_frame_interval,
};

static const struct v4l2_subdev_core_ops sp2306_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops sp2306_pad_ops = {
	.init_cfg = sp2306_init_cfg,
	.enum_mbus_code = sp2306_enum_mbus_code,
	.enum_frame_size = sp2306_enum_frame_size,
	.enum_frame_interval = sp2306_enum_frame_interval,
	.get_fmt = sp2306_get_fmt,
	.set_fmt = sp2306_set_fmt,
};

static const struct v4l2_subdev_ops sp2306_subdev_ops = {
	.core = &sp2306_core_ops,
	.video = &sp2306_video_ops,
	.pad = &sp2306_pad_ops,
};

static int sp2306_check_chip_id(struct sp2306_dev *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = sp2306_write_reg(sensor->client, SP2306_PAGE_REG, SP2306_PAGE_0);
	if (ret)
		return ret;

	ret = sp2306_read_reg(sensor->client, SP2306_CHIP_ID_H, &id_h);
	if (ret)
		return ret;

	ret = sp2306_read_reg(sensor->client, SP2306_CHIP_ID_L, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != SP2306_CHIP_ID) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}

	return 0;
}

static int sp2306_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct sp2306_dev *sensor;
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
	kref_init(&sensor->ref);
	sensor->lifetime.ref = &sensor->ref;
	sensor->lifetime.release = sp2306_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &sp2306_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &sp2306_internal_ops;

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

	ret = sp2306_power_on(sensor);
	if (ret)
		goto err_put;

	ret = sp2306_check_chip_id(sensor);
	if (ret)
		goto err_power;

	sp2306_power_off(sensor);

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;

	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_power;

	ret = sp2306_init_controls(sensor);
	if (ret)
		goto err_power;
	sp2306_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		v4l2_async_unregister_subdev(&sensor->sd);
		goto err_power;
	}

	return 0;

err_power:
	sp2306_power_off(sensor);
err_put:
	kref_put(&sensor->ref, sp2306_release);
	return ret;
}

static int sp2306_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sp2306_dev *sensor = to_sp2306(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	sp2306_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, sp2306_release);
	return 0;
}

static const struct of_device_id sp2306_of_match[] = {
	{ .compatible = "superpix,sp2306" },
	{ }
};
MODULE_DEVICE_TABLE(of, sp2306_of_match);

static const struct i2c_device_id sp2306_id[] = {
	{ "sp2306", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sp2306_id);

static struct i2c_driver sp2306_i2c_driver = {
	.driver = {
		.name = "sp2306",
		.of_match_table = sp2306_of_match,
	},
	.probe		= sp2306_probe,
	.remove		= sp2306_remove,
	.id_table	= sp2306_id,
};
module_i2c_driver(sp2306_i2c_driver);

MODULE_DESCRIPTION("Superpix SP2306 MIPI CSI-2 camera sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
