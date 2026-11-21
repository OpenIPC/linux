// SPDX-License-Identifier: GPL-2.0
/*
 * Shared helpers for SmartSens MIPI CSI-2 image sensors
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/capability.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gcd.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/math.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>

#include "smartsens-common.h"

#define SMARTSENS_XVCLK_RATE		24000000
#define SMARTSENS_REG_STREAM		0x0100
#define SMARTSENS_REG_SEC_I2C_ADDR	0x3004
#define SMARTSENS_REG_SYS_RESET		0x301a
#define SMARTSENS_REG_CHIP_ID_H		0x3107
#define SMARTSENS_REG_CHIP_ID_L		0x3108
#define SMARTSENS_REG_VTS_H		0x320e
#define SMARTSENS_REG_VTS_L		0x320f
#define SMARTSENS_REG_MIRROR_FLIP	0x3221
#define SMARTSENS_REG_EXP_H		0x3e00
#define SMARTSENS_REG_EXP_M		0x3e01
#define SMARTSENS_REG_EXP_L		0x3e02
#define SMARTSENS_REG_AGAIN_FINE	0x3e07
#define SMARTSENS_REG_AGAIN_CTRL	0x3e08
#define SMARTSENS_REG_AGAIN_COARSE	0x3e09
#define SMARTSENS_REG_TEST_PATTERN	0x4501

static const char * const smartsens_supply_names[SMARTSENS_NUM_SUPPLIES] = {
	"avdd",
	"dovdd",
	"dvdd",
};

static const char * const smartsens_test_pattern_menu[] = {
	"Disabled",
	"Vertical Color Bars",
	"Pseudo-Random Data",
	"Solid Square",
	"Black Image",
};

int smartsens_read_reg(struct v4l2_subdev *sd, u16 reg, u8 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 buf[2] = { reg >> 8, reg & 0xff };
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = sizeof(buf),
			.buf = buf,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = val,
		},
	};
	int ret;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	return ret == ARRAY_SIZE(msgs) ? 0 : (ret < 0 ? ret : -EIO);
}
EXPORT_SYMBOL_GPL(smartsens_read_reg);

int smartsens_write_reg(struct v4l2_subdev *sd, u16 reg, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 buf[3] = { reg >> 8, reg & 0xff, val };
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = sizeof(buf),
		.buf = buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	return ret == 1 ? 0 : (ret < 0 ? ret : -EIO);
}
EXPORT_SYMBOL_GPL(smartsens_write_reg);

static int smartsens_write_array(struct v4l2_subdev *sd,
				 const struct regval_list *vals,
				 bool skip_stream_on)
{
	int ret;

	while (vals->reg_num != 0xffff) {
		if (vals->reg_num == 0xfffe) {
			msleep(vals->value);
		} else if (!(skip_stream_on &&
			     vals->reg_num == SMARTSENS_REG_STREAM &&
			     vals->value == 1)) {
			ret = smartsens_write_reg(sd, vals->reg_num,
						  vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static unsigned int smartsens_max_gain(const struct smartsens_desc *desc)
{
	unsigned int max_gain = desc->again_lut_size ?
				desc->again_lut_size - 1 : 0;

	while (max_gain && desc->again_lut[max_gain].gain_val > desc->max_again)
		max_gain--;
	return max_gain;
}

static int smartsens_set_flip(struct v4l2_subdev *sd, int hflip, int vflip)
{
	u8 val;
	int ret;

	ret = smartsens_read_reg(sd, SMARTSENS_REG_MIRROR_FLIP, &val);
	if (ret)
		return ret;
	val &= ~0x66;
	if (hflip)
		val |= 0x06;
	if (vflip)
		val |= 0x60;
	return smartsens_write_reg(sd, SMARTSENS_REG_MIRROR_FLIP, val);
}

static int smartsens_set_test_pattern(struct v4l2_subdev *sd, s32 val)
{
	u8 regval = val ? (0x08 | ((val - 1) & 0x03)) : 0x00;

	return smartsens_write_reg(sd, SMARTSENS_REG_TEST_PATTERN, regval);
}

static int smartsens_set_exposure(struct v4l2_subdev *sd, s32 lines)
{
	int ret;

	if (to_smartsens_sensor(sd)->desc->half_line_exposure)
		lines *= 2;

	ret = smartsens_write_reg(sd, SMARTSENS_REG_EXP_H, (lines >> 12) & 0x0f);
	if (ret)
		return ret;
	ret = smartsens_write_reg(sd, SMARTSENS_REG_EXP_M, (lines >> 4) & 0xff);
	if (ret)
		return ret;
	return smartsens_write_reg(sd, SMARTSENS_REG_EXP_L, (lines << 4) & 0xf0);
}

int smartsens_set_vblank(struct v4l2_subdev *sd, s32 val)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	u32 vts = sensor->height + val;
	int ret;

	sensor->vts = vts;
	ret = smartsens_write_reg(sd, SMARTSENS_REG_VTS_H, (vts >> 8) & 0xff);
	if (ret)
		return ret;
	return smartsens_write_reg(sd, SMARTSENS_REG_VTS_L, vts & 0xff);
}
EXPORT_SYMBOL_GPL(smartsens_set_vblank);

static int smartsens_set_again(struct v4l2_subdev *sd, s32 index)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	const struct smartsens_desc *desc = sensor->desc;
	u32 reg_code;
	int ret;

	if (index < 0 || index >= desc->again_lut_size)
		return -EINVAL;
	reg_code = desc->again_lut[index].reg_code;
	if (desc->write_gain)
		return desc->write_gain(sd, reg_code);
	if (desc->sc1a4t_again_layout) {
		ret = smartsens_write_reg(sd, SMARTSENS_REG_AGAIN_FINE,
					  reg_code & 0xff);
		if (ret)
			return ret;
		return smartsens_write_reg(sd, SMARTSENS_REG_AGAIN_COARSE,
					   (reg_code >> 8) & 0xff);
	}
	ret = smartsens_write_reg(sd, SMARTSENS_REG_AGAIN_COARSE,
				  (reg_code >> 8) & 0xff);
	if (ret)
		return ret;
	ret = smartsens_write_reg(sd, SMARTSENS_REG_AGAIN_CTRL, 0x1f);
	if (ret)
		return ret;
	return smartsens_write_reg(sd, SMARTSENS_REG_AGAIN_FINE,
				   reg_code & 0xff);
}

static int smartsens_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct smartsens_sensor *sensor =
		container_of(ctrl->handler, struct smartsens_sensor, ctrls);
	struct v4l2_subdev *sd = &sensor->sd;
	s32 old_exp = sensor->exposure ? sensor->exposure->val : 0;

	if (ctrl->id == V4L2_CID_VBLANK && sensor->exposure) {
		u32 margin = sensor->exposure_margin ? sensor->exposure_margin : 6;
		u32 maximum = sensor->height + ctrl->val - margin;

		__v4l2_ctrl_modify_range(sensor->exposure,
					 sensor->exposure->minimum,
					 maximum,
					 sensor->exposure->step,
					 min_t(u32, sensor->exposure->default_value,
					       maximum));
	}

	if (!sensor->streaming) {
		if (ctrl->id == V4L2_CID_VBLANK)
			sensor->vts = sensor->height + ctrl->val;
		return 0;
	}

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return smartsens_set_exposure(sd, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return smartsens_set_again(sd, ctrl->val);
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP: {
		int hflip = (ctrl->id == V4L2_CID_HFLIP) ? ctrl->val :
			(sensor->hflip ? sensor->hflip->val : 0);
		int vflip = (ctrl->id == V4L2_CID_VFLIP) ? ctrl->val :
			(sensor->vflip ? sensor->vflip->val : 0);
		return smartsens_set_flip(sd, hflip, vflip);
	}
	case V4L2_CID_VBLANK:
		if (sensor->exposure && sensor->exposure->val < old_exp) {
			int ret = smartsens_set_exposure(sd,
							 sensor->exposure->val);
			if (ret)
				return ret;
		}
		return sensor->desc->set_vblank ?
			sensor->desc->set_vblank(sd, ctrl->val) :
			smartsens_set_vblank(sd, ctrl->val);
	case V4L2_CID_TEST_PATTERN:
		return smartsens_set_test_pattern(sd, ctrl->val);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops smartsens_ctrl_ops = {
	.s_ctrl = smartsens_s_ctrl,
};

static int smartsens_enable_power_supplies(struct smartsens_sensor *sensor)
{
	int ret;

	ret = regulator_bulk_enable(SMARTSENS_NUM_SUPPLIES, sensor->supplies);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sensor->mclk);
	if (ret) {
		regulator_bulk_disable(SMARTSENS_NUM_SUPPLIES,
				       sensor->supplies);
		return ret;
	}
	return 0;
}

static int smartsens_power_on(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	int ret;

	ret = smartsens_enable_power_supplies(sensor);
	if (ret)
		return ret;

	if (sensor->pwdn_gpio) {
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 0);
		msleep(10);
	}
	/*
	 * Do not assert a shared reset line when a secondary sensor has been
	 * remapped to secondary_i2c_addr, or its remapped I2C address is lost.
	 */
	if (sensor->reset_gpio && !sensor->secondary_i2c_addr) {
		gpiod_set_value_cansleep(sensor->reset_gpio, 1);
		msleep(10);
		gpiod_set_value_cansleep(sensor->reset_gpio, 0);
		msleep(10);
	}
	return 0;
}

static int smartsens_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);

	if (sensor->reset_gpio && !sensor->secondary_i2c_addr)
		gpiod_set_value_cansleep(sensor->reset_gpio, 1);
	if (sensor->pwdn_gpio)
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 1);
	clk_disable_unprepare(sensor->mclk);
	regulator_bulk_disable(SMARTSENS_NUM_SUPPLIES, sensor->supplies);
	return 0;
}

static int smartsens_probe_power_on(struct smartsens_sensor *sensor)
{
	int ret;

	if (!sensor->secondary_i2c_addr)
		return smartsens_power_on(sensor->sd.dev);

	ret = smartsens_enable_power_supplies(sensor);
	if (ret)
		return ret;

	/*
	 * Hold the primary sensor in power-down while releasing the shared
	 * reset line so the secondary sensor responds at 0x30 and can be
	 * reassigned to secondary_i2c_addr before the primary wakes.
	 */
	if (sensor->pwdn_gpio) {
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 1);
		msleep(10);
	}
	if (sensor->reset_gpio) {
		gpiod_set_value_cansleep(sensor->reset_gpio, 1);
		msleep(10);
		gpiod_set_value_cansleep(sensor->reset_gpio, 0);
		msleep(10);
	}
	ret = smartsens_write_reg(&sensor->sd, SMARTSENS_REG_SEC_I2C_ADDR,
				  sensor->secondary_i2c_addr << 1);
	if (ret)
		goto err_off;
	if (sensor->pwdn_gpio) {
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 0);
		msleep(10);
	}
	return 0;

err_off:
	if (sensor->pwdn_gpio)
		gpiod_set_value_cansleep(sensor->pwdn_gpio, 1);
	clk_disable_unprepare(sensor->mclk);
	regulator_bulk_disable(SMARTSENS_NUM_SUPPLIES, sensor->supplies);
	return ret;
}

static int smartsens_identify(struct smartsens_sensor *sensor)
{
	const struct smartsens_desc *desc = sensor->desc;
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);
	u8 id_h = 0, id_l = 0;
	u16 id;
	int ret, stop_ret = 0;

	if (desc->id_needs_stream) {
		ret = smartsens_write_reg(&sensor->sd, SMARTSENS_REG_SYS_RESET,
					  0xf8);
		if (ret)
			return ret;
		ret = smartsens_write_reg(&sensor->sd, SMARTSENS_REG_STREAM,
					  0x01);
		if (ret)
			goto stop;
		msleep(10);
	}

	ret = smartsens_read_reg(&sensor->sd, SMARTSENS_REG_CHIP_ID_H, &id_h);
	if (ret)
		goto stop;
	ret = smartsens_read_reg(&sensor->sd, SMARTSENS_REG_CHIP_ID_L, &id_l);
	if (ret)
		goto stop;

	id = ((u16)id_h << 8) | id_l;
	if (id != desc->chip_id &&
	    (!desc->alt_chip_id || id != desc->alt_chip_id)) {
		dev_err(&client->dev,
			"unexpected chip ID %#04x (expected %#04x)\n",
			id, desc->chip_id);
		ret = -ENODEV;
		goto stop;
	}
	dev_dbg(&client->dev, "detected chip %#04x\n", id);

stop:
	if (desc->id_needs_stream)
		stop_ret = smartsens_write_reg(&sensor->sd,
					       SMARTSENS_REG_STREAM, 0x00);
	return ret ? ret : stop_ret;
}

static int smartsens_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	const struct smartsens_desc *desc = sensor->desc;
	const struct regval_list * const *seqs;
	size_t num_seqs, i;
	int ret = 0;

	mutex_lock(&sensor->lock);
	if (sensor->streaming == !!enable)
		goto out;

	if (!enable) {
		ret = smartsens_write_array(sd, desc->stream_off, false);
		sensor->streaming = false;
		if (sensor->hflip)
			__v4l2_ctrl_grab(sensor->hflip, false);
		if (sensor->vflip)
			__v4l2_ctrl_grab(sensor->vflip, false);
		pm_runtime_put(&client->dev);
		goto out;
	}

	ret = pm_runtime_resume_and_get(&client->dev);
	if (ret < 0)
		goto out;

	seqs = sensor->frame_sync_slave ? desc->slave_seqs : desc->primary_seqs;
	num_seqs = sensor->frame_sync_slave ? desc->num_slave_seqs :
					      desc->num_primary_seqs;
	for (i = 0; i < num_seqs; i++) {
		ret = smartsens_write_array(sd, seqs[i], true);
		if (ret)
			goto stop;
	}
	if (sensor->shvflip) {
		ret = smartsens_write_reg(sd, SMARTSENS_REG_MIRROR_FLIP, 0x66);
		if (ret)
			goto stop;
	}

	sensor->streaming = true;
	ret = __v4l2_ctrl_handler_setup(&sensor->ctrls);
	if (ret)
		goto stop;
	ret = smartsens_write_array(sd, desc->stream_on, false);
	if (ret)
		goto stop;
	if (desc->fsync) {
		ret = desc->fsync(sd, sensor->frame_sync_slave);
		if (ret)
			goto stop;
	}
	if (sensor->hflip)
		__v4l2_ctrl_grab(sensor->hflip, true);
	if (sensor->vflip)
		__v4l2_ctrl_grab(sensor->vflip, true);
	goto out;

stop:
	smartsens_write_array(sd, desc->stream_off, false);
	sensor->streaming = false;
	pm_runtime_put(&client->dev);
out:
	mutex_unlock(&sensor->lock);
	return ret;
}

static int smartsens_enum_mbus_code(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *sd_state,
				    struct v4l2_subdev_mbus_code_enum *code)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);

	if (code->pad || code->index)
		return -EINVAL;
	code->code = sensor->fmt.code;
	return 0;
}

static int smartsens_set_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *sd_state,
			     struct v4l2_subdev_format *format)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	struct v4l2_mbus_framefmt *fmt;

	if (format->pad)
		return -EINVAL;
	fmt = v4l2_subdev_state_get_format(sd_state, format->pad);
	if (!fmt)
		return -EINVAL;
	*fmt = sensor->fmt;
	format->format = *fmt;
	return 0;
}

static int smartsens_enum_frame_size(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *sd_state,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);

	if (fse->pad || fse->index || fse->code != sensor->fmt.code)
		return -EINVAL;
	fse->min_width = fse->max_width = sensor->fmt.width;
	fse->min_height = fse->max_height = sensor->fmt.height;
	return 0;
}

static int
smartsens_enum_frame_interval(struct v4l2_subdev *sd,
			      struct v4l2_subdev_state *sd_state,
			      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);

	if (fie->pad || fie->index || fie->code != sensor->fmt.code ||
	    fie->width != sensor->fmt.width ||
	    fie->height != sensor->fmt.height)
		return -EINVAL;
	fie->interval.numerator = 1;
	fie->interval.denominator = sensor->fps;
	return 0;
}

/* Called with sd_state->lock (&sensor->lock) already held by v4l2-subdev. */
static int smartsens_get_frame_interval(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *sd_state,
					struct v4l2_subdev_frame_interval *fi)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	u64 pclk = sensor->pixel_rate ? sensor->pixel_rate->val : 0;
	u32 hts = sensor->fmt.width + (sensor->hblank ? sensor->hblank->val : 0);
	u64 divisor;

	if (fi->pad)
		return -EINVAL;

	if (pclk && hts && sensor->vts) {
		u64 num = (u64)hts * sensor->vts;
		u64 den = pclk;

		divisor = gcd(num, den);
		if (divisor) {
			num = div64_u64(num, divisor);
			den = div64_u64(den, divisor);
		}
		fi->interval.numerator = num;
		fi->interval.denominator = den;
		return 0;
	}

	fi->interval.numerator = 1;
	fi->interval.denominator = sensor->fps;
	return 0;
}

static int smartsens_set_frame_interval(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *sd_state,
					struct v4l2_subdev_frame_interval *fi)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	u64 pclk = sensor->pixel_rate ? sensor->pixel_rate->val : 0;
	u32 hts = sensor->fmt.width + (sensor->hblank ? sensor->hblank->val : 0);

	if (fi->pad)
		return -EINVAL;

	if (fi->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->vblank &&
	    pclk && hts && fi->interval.numerator && fi->interval.denominator) {
		u64 vts = div64_u64(pclk * fi->interval.numerator,
				    (u64)hts * fi->interval.denominator);
		s32 vblank = (s32)vts - (s32)sensor->height;

		vblank = clamp(vblank, sensor->vblank->minimum,
			       sensor->vblank->maximum);
		__v4l2_ctrl_s_ctrl(sensor->vblank, vblank);
	}

	return smartsens_get_frame_interval(sd, sd_state, fi);
}

static int smartsens_get_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				     struct v4l2_mbus_config *config)
{
	if (pad)
		return -EINVAL;
	*config = to_smartsens_sensor(sd)->bus;
	return 0;
}

static int smartsens_init_state(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *sd_state)
{
	struct v4l2_mbus_framefmt *fmt =
		v4l2_subdev_state_get_format(sd_state, 0);

	if (fmt)
		*fmt = to_smartsens_sensor(sd)->fmt;
	return 0;
}

static const struct v4l2_subdev_pad_ops smartsens_pad_ops = {
	.enum_mbus_code = smartsens_enum_mbus_code,
	.enum_frame_size = smartsens_enum_frame_size,
	.enum_frame_interval = smartsens_enum_frame_interval,
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = smartsens_set_fmt,
	.get_frame_interval = smartsens_get_frame_interval,
	.set_frame_interval = smartsens_set_frame_interval,
	.get_mbus_config = smartsens_get_mbus_config,
};

static const struct v4l2_subdev_internal_ops smartsens_internal_ops = {
	.init_state = smartsens_init_state,
};

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int smartsens_g_register(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 value = 0;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (reg->reg > 0xffff)
		return -EINVAL;
	ret = pm_runtime_resume_and_get(&client->dev);
	if (ret < 0)
		return ret;
	mutex_lock(&sensor->lock);
	ret = smartsens_read_reg(sd, reg->reg, &value);
	mutex_unlock(&sensor->lock);
	pm_runtime_put(&client->dev);
	if (ret)
		return ret;
	reg->size = 1;
	reg->val = value;
	return 0;
}

static int smartsens_s_register(struct v4l2_subdev *sd,
				const struct v4l2_dbg_register *reg)
{
	struct smartsens_sensor *sensor = to_smartsens_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (reg->reg > 0xffff || reg->val > 0xff)
		return -EINVAL;
	ret = pm_runtime_resume_and_get(&client->dev);
	if (ret < 0)
		return ret;
	mutex_lock(&sensor->lock);
	ret = smartsens_write_reg(sd, reg->reg, reg->val);
	mutex_unlock(&sensor->lock);
	pm_runtime_put(&client->dev);
	return ret;
}
#endif

static const struct v4l2_subdev_core_ops smartsens_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = smartsens_g_register,
	.s_register = smartsens_s_register,
#endif
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops smartsens_video_ops = {
	.s_stream = smartsens_s_stream,
};

static const struct v4l2_subdev_ops smartsens_subdev_ops = {
	.core = &smartsens_core_ops,
	.video = &smartsens_video_ops,
	.pad = &smartsens_pad_ops,
};

static int smartsens_parse_fwnode(struct device *dev,
				  struct smartsens_sensor *sensor)
{
	const struct smartsens_desc *desc = sensor->desc;
	struct v4l2_fwnode_endpoint bus_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	struct fwnode_handle *endpoint;
	unsigned long link_freq_bitmap;
	unsigned int lanes, i;
	u32 sec_addr;
	int ret;

	sensor->frame_sync_slave =
		device_property_read_bool(dev, "smartsens,frame-sync-slave");
	if (device_property_present(dev, "smartsens,secondary-i2c-addr")) {
		ret = device_property_read_u32(dev,
					       "smartsens,secondary-i2c-addr",
					       &sec_addr);
		if (ret)
			return ret;
		if (sec_addr < 0x08 || sec_addr > 0x77)
			return -EINVAL;
		sensor->secondary_i2c_addr = sec_addr;
	}

	if (!sensor->desc->num_slave_seqs &&
	    (sensor->frame_sync_slave || sensor->secondary_i2c_addr))
		return -EINVAL;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint)
		return -ENOTCONN;

	ret = v4l2_fwnode_endpoint_alloc_parse(endpoint, &bus_cfg);
	fwnode_handle_put(endpoint);
	if (ret)
		return ret;

	lanes = bus_cfg.bus.mipi_csi2.num_data_lanes;
	if (lanes != desc->expected_lanes ||
	    bus_cfg.bus.mipi_csi2.clock_lane != 0) {
		ret = -EINVAL;
		goto out_free;
	}
	for (i = 0; i < lanes; i++) {
		if (bus_cfg.bus.mipi_csi2.data_lanes[i] != i + 1) {
			ret = -EINVAL;
			goto out_free;
		}
	}
	for (i = 0; i <= lanes; i++) {
		if (bus_cfg.bus.mipi_csi2.lane_polarities[i]) {
			ret = -EINVAL;
			goto out_free;
		}
	}

	ret = v4l2_link_freq_to_bitmap(dev, bus_cfg.link_frequencies,
				       bus_cfg.nr_of_link_frequencies,
				       desc->link_freqs, desc->num_link_freqs,
				       &link_freq_bitmap);
	if (ret)
		goto out_free;

	sensor->bus.type = V4L2_MBUS_CSI2_DPHY;
	sensor->bus.bus.mipi_csi2 = bus_cfg.bus.mipi_csi2;

out_free:
	v4l2_fwnode_endpoint_free(&bus_cfg);
	return ret;
}

static int smartsens_init_controls(struct smartsens_sensor *sensor)
{
	const struct smartsens_desc *desc = sensor->desc;
	struct v4l2_ctrl *link_freq;
	s32 vblank_min = sensor->frame_sync_slave && desc->vblank_slave_min ?
			 desc->vblank_slave_min : desc->vblank_min;
	u32 again_max = smartsens_max_gain(desc);
	int ret;

	v4l2_ctrl_handler_init(&sensor->ctrls, 9);
	sensor->ctrls.lock = &sensor->lock;

	link_freq = v4l2_ctrl_new_int_menu(&sensor->ctrls, NULL,
					   V4L2_CID_LINK_FREQ,
					   desc->num_link_freqs - 1, 0,
					   desc->link_freqs);
	if (link_freq)
		link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->exposure = v4l2_ctrl_new_std(&sensor->ctrls,
					     &smartsens_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     desc->exposure_min,
					     desc->exposure_max,
					     1, desc->exposure_def);
	sensor->again = v4l2_ctrl_new_std(&sensor->ctrls,
					  &smartsens_ctrl_ops,
					  V4L2_CID_ANALOGUE_GAIN,
					  0, again_max, 1, 0);
	sensor->hflip = v4l2_ctrl_new_std(&sensor->ctrls,
					  &smartsens_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1,
					  sensor->shvflip ? 1 : 0);
	sensor->vflip = v4l2_ctrl_new_std(&sensor->ctrls,
					  &smartsens_ctrl_ops,
					  V4L2_CID_VFLIP, 0, 1, 1,
					  sensor->shvflip ? 1 : 0);
	sensor->vblank = v4l2_ctrl_new_std(&sensor->ctrls,
					   &smartsens_ctrl_ops,
					   V4L2_CID_VBLANK, vblank_min,
					   desc->vblank_max, 1,
					   desc->vblank_def);
	sensor->hblank = v4l2_ctrl_new_std(&sensor->ctrls,
					   &smartsens_ctrl_ops,
					   V4L2_CID_HBLANK, desc->hblank,
					   desc->hblank, 1, desc->hblank);
	if (sensor->hblank)
		sensor->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	sensor->pixel_rate = v4l2_ctrl_new_std(&sensor->ctrls,
					       &smartsens_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       desc->pixel_rate,
					       desc->pixel_rate, 1,
					       desc->pixel_rate);
	if (sensor->pixel_rate)
		sensor->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	if (!desc->no_test_pattern)
		v4l2_ctrl_new_std_menu_items(&sensor->ctrls, &smartsens_ctrl_ops,
					     V4L2_CID_TEST_PATTERN,
					     ARRAY_SIZE(smartsens_test_pattern_menu) - 1,
					     0, 0, smartsens_test_pattern_menu);

	if (sensor->ctrls.error) {
		ret = sensor->ctrls.error;
		v4l2_ctrl_handler_free(&sensor->ctrls);
		return ret;
	}
	sensor->sd.ctrl_handler = &sensor->ctrls;
	return 0;
}

int smartsens_probe(struct i2c_client *client,
		    const struct smartsens_desc *desc, bool shvflip)
{
	struct device *dev = &client->dev;
	struct smartsens_sensor *sensor;
	unsigned long rate = desc->xvclk_rate ?: SMARTSENS_XVCLK_RATE;
	unsigned long actual;
	unsigned int i;
	int ret;

	sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->desc = desc;
	sensor->shvflip = shvflip;
	sensor->height = desc->height;
	sensor->fps = desc->fps;
	sensor->vts = desc->vts_def;
	sensor->frame_length_default = desc->vts_def;
	sensor->exposure_margin = desc->exposure_margin ?
				  desc->exposure_margin : 6;
	sensor->fmt.width = desc->width;
	sensor->fmt.height = desc->height;
	sensor->fmt.code = desc->mbus_code;
	sensor->fmt.field = V4L2_FIELD_NONE;
	sensor->fmt.colorspace = desc->colorspace;
	mutex_init(&sensor->lock);

	ret = smartsens_parse_fwnode(dev, sensor);
	if (ret)
		return ret;

	for (i = 0; i < SMARTSENS_NUM_SUPPLIES; i++)
		sensor->supplies[i].supply = smartsens_supply_names[i];
	ret = devm_regulator_bulk_get(dev, SMARTSENS_NUM_SUPPLIES,
				      sensor->supplies);
	if (ret)
		return dev_err_probe(dev, ret, "failed to get regulators\n");

	sensor->mclk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(sensor->mclk))
		return dev_err_probe(dev, PTR_ERR(sensor->mclk),
				     "failed to get xvclk\n");
	ret = clk_set_rate(sensor->mclk, rate);
	if (ret)
		return dev_err_probe(dev, ret,
				     "failed to set xvclk to %lu Hz\n", rate);

	actual = clk_get_rate(sensor->mclk);
	if (!actual || abs_diff(actual, rate) > rate / 50)
		return dev_err_probe(dev, -ERANGE,
				     "xvclk %lu Hz out of range (want %lu Hz)\n",
				     actual, rate);
	if (actual != rate)
		dev_warn(dev, "xvclk running at %lu Hz (requested %lu Hz)\n",
			 actual, rate);

	sensor->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->reset_gpio))
		return PTR_ERR(sensor->reset_gpio);

	sensor->pwdn_gpio = devm_gpiod_get_optional(dev, "pwdn",
						    GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->pwdn_gpio))
		return PTR_ERR(sensor->pwdn_gpio);

	v4l2_i2c_subdev_init(&sensor->sd, client, &smartsens_subdev_ops);
	if (client->name[0])
		snprintf(sensor->sd.name, sizeof(sensor->sd.name), "%s %d-%04x",
			 client->name, i2c_adapter_id(client->adapter),
			 client->addr);
	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor->sd.internal_ops = &smartsens_internal_ops;
	sensor->sd.state_lock = &sensor->lock;

	ret = smartsens_probe_power_on(sensor);
	if (ret)
		return ret;

	ret = smartsens_identify(sensor);
	if (ret)
		goto err_power_off;

	ret = smartsens_init_controls(sensor);
	if (ret)
		goto err_power_off;

	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_ctrls;

	ret = v4l2_subdev_init_finalize(&sensor->sd);
	if (ret)
		goto err_entity;

	ret = v4l2_async_register_subdev_sensor(&sensor->sd);
	if (ret)
		goto err_subdev;

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);
	return 0;

err_subdev:
	v4l2_subdev_cleanup(&sensor->sd);
err_entity:
	media_entity_cleanup(&sensor->sd.entity);
err_ctrls:
	v4l2_ctrl_handler_free(&sensor->ctrls);
err_power_off:
	smartsens_power_off(dev);
	return ret;
}
EXPORT_SYMBOL_GPL(smartsens_probe);

void smartsens_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct smartsens_sensor *sensor = sd ? to_smartsens_sensor(sd) : NULL;

	if (!sensor)
		return;

	v4l2_async_unregister_subdev(sd);
	v4l2_subdev_cleanup(sd);
	media_entity_cleanup(&sd->entity);
	if (sd->ctrl_handler) {
		v4l2_ctrl_handler_free(sd->ctrl_handler);
		sd->ctrl_handler = NULL;
	}
	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		smartsens_power_off(&client->dev);
	pm_runtime_set_suspended(&client->dev);
}
EXPORT_SYMBOL_GPL(smartsens_remove);

const struct dev_pm_ops smartsens_pm_ops = {
	RUNTIME_PM_OPS(smartsens_power_off, smartsens_power_on, NULL)
};
EXPORT_SYMBOL_GPL(smartsens_pm_ops);

MODULE_DESCRIPTION("SmartSens MIPI CSI-2 image sensor common helpers");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
