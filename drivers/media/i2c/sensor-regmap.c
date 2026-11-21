// SPDX-License-Identifier: GPL-2.0-only
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/gpio/consumer.h>
#include <linux/math.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <media/v4l2-async.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>

#include "sensor-regmap.h"

struct sensor_regmap {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct regmap *map;
	struct clk *clk;
	struct gpio_desc *reset;
	struct gpio_desc *pwdn;
	struct regulator_bulk_data supplies[3];
	struct mutex lock; /* Serializes stream, format and control changes. */
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *vblank;
	struct v4l2_mbus_config bus;
	const struct sensor_regmap_model *model;
	const struct sensor_regmap_mode *mode;
	bool streaming;
};

static struct sensor_regmap *to_sensor(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_regmap, sd);
}

static int sensor_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_regmap *s = container_of(ctrl->handler, struct sensor_regmap, ctrls);
	unsigned int value = ctrl->val;
	int ret;

	if (ctrl->id == V4L2_CID_VBLANK) {
		value += s->mode->height;
		ret = __v4l2_ctrl_modify_range(s->exposure, s->model->exposure_min,
					       value - 4, 1,
					       min_t(u32, s->exposure->default_value,
						     value - 4));
		if (ret)
			return ret;
	}
	if (!s->streaming)
		return 0;
	return s->model->set_ctrl(s->map, ctrl->id, value);
}

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.s_ctrl = sensor_set_ctrl,
};

static int sensor_power_on(struct device *dev)
{
	struct sensor_regmap *s = to_sensor(dev_get_drvdata(dev));
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(s->supplies), s->supplies);
	if (ret)
		return ret;
	ret = clk_prepare_enable(s->clk);
	if (ret) {
		regulator_bulk_disable(ARRAY_SIZE(s->supplies), s->supplies);
		return ret;
	}
	if (s->pwdn) {
		gpiod_set_value_cansleep(s->pwdn, 1);
		msleep(s->model->pwdn_delay_ms);
		gpiod_set_value_cansleep(s->pwdn, 0);
	}
	if (s->reset) {
		gpiod_set_value_cansleep(s->reset, 0);
		msleep(50);
		gpiod_set_value_cansleep(s->reset, 1);
		msleep(35);
		gpiod_set_value_cansleep(s->reset, 0);
		msleep(35);
	}
	usleep_range(10000, 11000);
	return 0;
}

static int sensor_power_off(struct device *dev)
{
	struct sensor_regmap *s = to_sensor(dev_get_drvdata(dev));

	gpiod_set_value_cansleep(s->reset, 1);
	gpiod_set_value_cansleep(s->pwdn, 1);
	clk_disable_unprepare(s->clk);
	regulator_bulk_disable(ARRAY_SIZE(s->supplies), s->supplies);
	return 0;
}

const struct dev_pm_ops sensor_regmap_pm_ops = {
	SET_RUNTIME_PM_OPS(sensor_power_off, sensor_power_on, NULL)
};
EXPORT_SYMBOL_GPL(sensor_regmap_pm_ops);

static int sensor_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_regmap *s = to_sensor(sd);
	int ret = 0;

	mutex_lock(&s->lock);
	if (s->streaming == !!enable)
		goto unlock;
	if (!enable) {
		if (s->model->standby)
			ret = s->model->standby(s->map, true);
		goto power_down;
	}

	if (IS_ENABLED(CONFIG_PM))
		ret = pm_runtime_resume_and_get(sd->dev);
	else
		ret = sensor_power_on(sd->dev);
	if (ret < 0)
		goto unlock;
	ret = regmap_multi_reg_write(s->map, s->mode->regs, s->mode->num_regs);
	if (ret)
		goto power_down;
	s->streaming = true;
	ret = __v4l2_ctrl_handler_setup(&s->ctrls);
	if (!ret && s->model->standby)
		ret = s->model->standby(s->map, false);
	if (!ret)
		goto unlock;

power_down:
	s->streaming = false;
	if (IS_ENABLED(CONFIG_PM))
		pm_runtime_put_sync_suspend(sd->dev);
	else
		sensor_power_off(sd->dev);
unlock:
	mutex_unlock(&s->lock);
	return ret;
}

static void sensor_interval(const struct sensor_regmap_mode *m, u16 vts,
			    struct v4l2_fract *interval)
{
	u32 clocks = (u32)m->hts * vts;
	u32 factor = gcd(clocks, m->pixel_rate);

	interval->numerator = clocks / factor;
	interval->denominator = m->pixel_rate / factor;
}

static int sensor_init_state(struct v4l2_subdev *sd, struct v4l2_subdev_state *state)
{
	const struct sensor_regmap_mode *m = to_sensor(sd)->mode;
	struct v4l2_mbus_framefmt *fmt = v4l2_subdev_state_get_format(state, 0);

	fmt->width = m->width;
	fmt->height = m->height;
	fmt->code = m->code;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = V4L2_COLORSPACE_RAW;
	sensor_interval(m, m->vts, v4l2_subdev_state_get_interval(state, 0));
	return 0;
}

static int sensor_enum_code(struct v4l2_subdev *sd, struct v4l2_subdev_state *state,
			    struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index || code->pad)
		return -EINVAL;
	code->code = to_sensor(sd)->mode->code;
	return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd, struct v4l2_subdev_state *state,
			    struct v4l2_subdev_frame_size_enum *size)
{
	const struct sensor_regmap_mode *m = to_sensor(sd)->mode;

	if (size->index || size->pad || size->code != m->code)
		return -EINVAL;
	size->min_width = m->width;
	size->max_width = m->width;
	size->min_height = m->height;
	size->max_height = m->height;
	return 0;
}

static int sensor_enum_interval(struct v4l2_subdev *sd, struct v4l2_subdev_state *state,
				struct v4l2_subdev_frame_interval_enum *fi)
{
	const struct sensor_regmap_mode *m = to_sensor(sd)->mode;

	if (fi->index || fi->pad || fi->code != m->code ||
	    fi->width != m->width || fi->height != m->height)
		return -EINVAL;
	sensor_interval(m, m->vts, &fi->interval);
	return 0;
}

static int sensor_get_interval(struct v4l2_subdev *sd, struct v4l2_subdev_state *state,
			       struct v4l2_subdev_frame_interval *fi)
{
	struct sensor_regmap *s = to_sensor(sd);

	if (fi->pad)
		return -EINVAL;
	if (fi->which == V4L2_SUBDEV_FORMAT_TRY) {
		fi->interval = *v4l2_subdev_state_get_interval(state, 0);
		return 0;
	}
	if (fi->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;
	sensor_interval(s->mode, s->mode->height + s->vblank->val, &fi->interval);
	return 0;
}

static int sensor_set_interval(struct v4l2_subdev *sd, struct v4l2_subdev_state *state,
			       struct v4l2_subdev_frame_interval *fi)
{
	struct sensor_regmap *s = to_sensor(sd);
	const struct sensor_regmap_mode *m = s->mode;
	u64 vts;
	int ret;

	if (fi->pad || !fi->interval.numerator || !fi->interval.denominator)
		return -EINVAL;
	vts = div64_u64((u64)m->pixel_rate * fi->interval.numerator,
			(u64)m->hts * fi->interval.denominator);
	vts = clamp_t(u64, vts, m->vts, 0xffff);
	if (fi->which == V4L2_SUBDEV_FORMAT_TRY) {
		sensor_interval(m, vts, &fi->interval);
		*v4l2_subdev_state_get_interval(state, 0) = fi->interval;
		return 0;
	}
	if (fi->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;
	ret = __v4l2_ctrl_s_ctrl(s->vblank, vts - m->height);
	if (ret)
		return ret;
	sensor_interval(m, m->height + s->vblank->val, &fi->interval);
	*v4l2_subdev_state_get_interval(state, 0) = fi->interval;
	return 0;
}

static int sensor_get_bus(struct v4l2_subdev *sd, unsigned int pad,
			  struct v4l2_mbus_config *bus)
{
	if (pad)
		return -EINVAL;
	*bus = to_sensor(sd)->bus;
	return 0;
}

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_code,
	.enum_frame_size = sensor_enum_size,
	.enum_frame_interval = sensor_enum_interval,
	.get_frame_interval = sensor_get_interval,
	.set_frame_interval = sensor_set_interval,
	.get_mbus_config = sensor_get_bus,
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = v4l2_subdev_get_fmt,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

static const struct v4l2_subdev_internal_ops sensor_internal_ops = {
	.init_state = sensor_init_state,
};

static int sensor_parse_endpoint(struct device *dev, struct sensor_regmap *s)
{
	struct v4l2_fwnode_endpoint ep = { .bus_type = V4L2_MBUS_CSI2_DPHY };
	const struct v4l2_mbus_config_mipi_csi2 *bus;
	struct fwnode_handle *node;
	unsigned int i;
	int ret;

	node = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!node)
		return -ENOTCONN;
	ret = v4l2_fwnode_endpoint_alloc_parse(node, &ep);
	fwnode_handle_put(node);
	if (ret)
		return ret;

	bus = &ep.bus.mipi_csi2;
	if (bus->clock_lane || bus->lane_polarities[0] ||
	    bus->num_data_lanes != s->model->lanes)
		goto out;
	for (i = 0; i < bus->num_data_lanes; i++)
		if (bus->data_lanes[i] != i + 1 || bus->lane_polarities[i + 1])
			goto out;

	s->bus.type = ep.bus_type;
	for (i = 0; i < s->model->num_modes; i++) {
		const struct sensor_regmap_mode *m = &s->model->modes[i];
		unsigned int j;

		for (j = 0; j < ep.nr_of_link_frequencies; j++)
			if (ep.link_frequencies[j] == m->link_freq)
				break;
		if (j == ep.nr_of_link_frequencies)
			continue;
		s->bus.bus.mipi_csi2 = *bus;
		s->mode = m;
		break;
	}
out:
	v4l2_fwnode_endpoint_free(&ep);
	return s->mode ? 0 : -EINVAL;
}

static int sensor_controls(struct sensor_regmap *s)
{
	const struct sensor_regmap_mode *m = s->mode;
	struct v4l2_ctrl *ctrl;
	int hblank = m->hts - m->width;
	int vblank = m->vts - m->height;

	v4l2_ctrl_handler_init(&s->ctrls, s->model->has_flip ? 8 : 6);
	s->ctrls.lock = &s->lock;
	s->vblank = v4l2_ctrl_new_std(&s->ctrls, &sensor_ctrl_ops,
				      V4L2_CID_VBLANK, vblank,
				      0xffff - m->height, 1, vblank);
	s->exposure = v4l2_ctrl_new_std(&s->ctrls, &sensor_ctrl_ops,
					V4L2_CID_EXPOSURE,
					s->model->exposure_min, m->vts - 4, 1,
					m->vts / 2);
	v4l2_ctrl_new_std(&s->ctrls, &sensor_ctrl_ops, V4L2_CID_ANALOGUE_GAIN,
			  0, s->model->gain_max, 1, 0);
	ctrl = v4l2_ctrl_new_std(&s->ctrls, NULL, V4L2_CID_HBLANK,
				 hblank, hblank, 1, hblank);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	v4l2_ctrl_new_std(&s->ctrls, NULL, V4L2_CID_PIXEL_RATE,
			  m->pixel_rate, m->pixel_rate, 1, m->pixel_rate);
	ctrl = v4l2_ctrl_new_int_menu(&s->ctrls, NULL, V4L2_CID_LINK_FREQ,
				      0, 0, &m->link_freq);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	if (s->model->has_flip) {
		v4l2_ctrl_new_std(&s->ctrls, &sensor_ctrl_ops,
				  V4L2_CID_HFLIP, 0, 1, 1, 0);
		v4l2_ctrl_new_std(&s->ctrls, &sensor_ctrl_ops,
				  V4L2_CID_VFLIP, 0, 1, 1, 0);
	}
	s->sd.ctrl_handler = &s->ctrls;
	return s->ctrls.error;
}

int sensor_regmap_probe(struct i2c_client *client)
{
	static const struct regmap_config config = { .reg_bits = 8, .val_bits = 8 };
	struct device *dev = &client->dev;
	struct sensor_regmap *s;
	unsigned long rate;
	int ret;

	s = devm_kzalloc(dev, sizeof(*s), GFP_KERNEL);
	if (!s)
		return -ENOMEM;
	s->model = i2c_get_match_data(client);
	if (!s->model)
		return -EINVAL;
	ret = sensor_parse_endpoint(dev, s);
	if (ret)
		return dev_err_probe(dev, ret, "unsupported sensor endpoint\n");

	s->map = devm_regmap_init_i2c(client, &config);
	if (IS_ERR(s->map))
		return PTR_ERR(s->map);
	s->clk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(s->clk))
		return dev_err_probe(dev, PTR_ERR(s->clk), "xvclk\n");
	ret = clk_set_rate(s->clk, s->model->xclk);
	if (ret)
		return dev_err_probe(dev, ret, "failed to set input clock\n");
	rate = clk_get_rate(s->clk);
	if (!rate || abs_diff(rate, s->model->xclk) > s->model->xclk / 50)
		return dev_err_probe(dev, -ERANGE,
				     "xclk rate %lu out of range (%lu)\n",
				     rate, s->model->xclk);
	if (rate != s->model->xclk)
		dev_warn(dev, "clock rate %lu differs from %lu\n",
			 rate, s->model->xclk);
	s->supplies[0].supply = "avdd";
	s->supplies[1].supply = "dovdd";
	s->supplies[2].supply = "dvdd";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(s->supplies), s->supplies);
	if (ret)
		return dev_err_probe(dev, ret, "sensor supplies\n");
	s->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(s->reset))
		return dev_err_probe(dev, PTR_ERR(s->reset), "reset GPIO\n");
	s->pwdn = devm_gpiod_get_optional(dev, "pwdn", GPIOD_OUT_HIGH);
	if (IS_ERR(s->pwdn))
		return dev_err_probe(dev, PTR_ERR(s->pwdn), "power-down GPIO\n");

	mutex_init(&s->lock);
	v4l2_i2c_subdev_init(&s->sd, client, &sensor_ops);
	v4l2_i2c_subdev_set_name(&s->sd, client, client->name, NULL);
	s->sd.internal_ops = &sensor_internal_ops;
	s->sd.state_lock = &s->lock;
	s->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	s->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	s->pad.flags = MEDIA_PAD_FL_SOURCE;

	ret = sensor_power_on(dev);
	if (ret) {
		ret = dev_err_probe(dev, ret, "failed to power on sensor\n");
		goto destroy_mutex;
	}
	if (s->model->detect) {
		ret = s->model->detect(dev, s->map);
		if (ret)
			goto power_off;
	}

	ret = sensor_controls(s);
	if (ret)
		goto free_controls;
	ret = media_entity_pads_init(&s->sd.entity, 1, &s->pad);
	if (ret)
		goto free_controls;
	ret = v4l2_subdev_init_finalize(&s->sd);
	if (ret)
		goto clean_entity;

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	ret = v4l2_async_register_subdev_sensor(&s->sd);
	if (ret)
		goto disable_pm;

	pm_runtime_idle(dev);
	if (!IS_ENABLED(CONFIG_PM))
		sensor_power_off(dev);
	return 0;

disable_pm:
	pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);
	v4l2_subdev_cleanup(&s->sd);
clean_entity:
	media_entity_cleanup(&s->sd.entity);
free_controls:
	v4l2_ctrl_handler_free(&s->ctrls);
power_off:
	sensor_power_off(dev);
destroy_mutex:
	mutex_destroy(&s->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_regmap_probe);

void sensor_regmap_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_regmap *s = to_sensor(sd);

	v4l2_async_unregister_subdev(sd);
	if (s->streaming)
		sensor_stream(sd, 0);
	pm_runtime_disable(sd->dev);
	if (IS_ENABLED(CONFIG_PM) && !pm_runtime_status_suspended(sd->dev))
		sensor_power_off(sd->dev);
	pm_runtime_set_suspended(sd->dev);
	v4l2_subdev_cleanup(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(&s->ctrls);
	mutex_destroy(&s->lock);
}
EXPORT_SYMBOL_GPL(sensor_regmap_remove);

MODULE_DESCRIPTION("V4L2 helpers for register-list camera sensors");
MODULE_LICENSE("GPL");
