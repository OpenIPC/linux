// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Anyka sensor subdev for the AK3918EV200 capture bridge.
 *
 * One i2c client stands for whichever sensor the board carries: the per-SKU
 * drivers publish AK_ISP_SENSOR_CB tables into the ISP registry, this driver
 * walks the registry to find the one that answers, hands it to the ISP as the
 * 3A callback table, and presents it to V4L2 as a subdev.
 *
 * Copyright (C) 2026 Alex Zhang <alex@osqdu.org>
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/v4l2-mediabus.h>
#include <linux/videodev2.h>

#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>

#include <mach-anyka/ispdrv_interface.h>
#include <plat-anyka/ak_sensor.h>
#include <plat-anyka/ak_sensor_i2c.h>

#include "ak-cam-compat.h"

#define AK_CAM_SENSOR_NAME	"aksensor"

static AK_ISP_SENSOR_CB *cur_sensor_info;

struct ak_cam_sensor {
	struct v4l2_subdev		subdev;
	struct v4l2_ctrl_handler	hdl;
	struct aksensor_camera_info	*info;
	struct v4l2_rect		win;
	AK_CAM_MBUS_CODE	code;
	enum v4l2_colorspace		colorspace;
	int				powered;
};

static struct ak_cam_sensor *to_ak_cam_sensor(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ak_cam_sensor, subdev);
}

int aksensor_get_sensor_id(void)
{
	return cur_sensor_info ? cur_sensor_info->sensor_read_id_func() : 0;
}
EXPORT_SYMBOL_GPL(aksensor_get_sensor_id);

int aksensor_get_sensor_if(char *if_str)
{
	int sensor_if = -1;
	int mipi_lane = 1;
	int ret = -1;

	if (cur_sensor_info && cur_sensor_info->sensor_get_parameter_func)
		ret = cur_sensor_info->sensor_get_parameter_func(GET_INTERFACE,
								&sensor_if);
	if (ret) {
		strcpy(if_str, "unknowif");
		return 0;
	}

	switch (sensor_if) {
	case DVP_INTERFACE:
		strcpy(if_str, "dvp");
		break;
	case MIPI_INTERFACE:
		cur_sensor_info->sensor_get_parameter_func(GET_MIPI_LANE,
							   &mipi_lane);
		sprintf(if_str, "mipi%d", mipi_lane);
		break;
	default:
		strcpy(if_str, "unknowif");
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(aksensor_get_sensor_if);

static AK_ISP_SENSOR_CB *ak_cam_sensor_probe_registry(struct i2c_client *client,
						      struct aksensor_camera_info *info)
{
	int read_id, probe_id;
	int index = 0;
	AK_ISP_SENSOR_CB *si;

	do {
		si = (AK_ISP_SENSOR_CB *)ispdrv_get_sensor(&index);
		if (si) {
			si->sensor_set_power_on_func(info->pin_pwdn,
						     info->pin_reset);

			read_id = si->sensor_read_id_func();
			probe_id = si->sensor_probe_id_func();

			if (read_id == probe_id) {
				dev_info(&client->dev, "sensor id 0x%x\n",
					 read_id);
				return si;
			}

			si->sensor_set_power_off_func(info->pin_pwdn,
						      info->pin_reset);
		}
		index++;
	} while (si);

	return NULL;
}

/* -------------------------------------------------------------------------
 * subdev core ops
 * ------------------------------------------------------------------------- */

static int ak_cam_sensor_s_power(struct v4l2_subdev *sd, int on)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);
	int ret;

	if (!cur_sensor_info)
		return on ? -ENODEV : 0;
	if (!!on == priv->powered)
		return 0;

	if (on)
		ret = cur_sensor_info->sensor_set_power_on_func(priv->info->pin_pwdn,
								priv->info->pin_reset);
	else
		ret = cur_sensor_info->sensor_set_power_off_func(priv->info->pin_pwdn,
								 priv->info->pin_reset);
	if (ret)
		return ret;

	priv->powered = !!on;

	return 0;
}

static int ak_cam_sensor_identify(struct v4l2_subdev *sd, u32 val)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int width, height, x, y;

	if (cur_sensor_info)
		return ak_cam_sensor_s_power(sd, 1);

	cur_sensor_info = ak_cam_sensor_probe_registry(client, priv->info);
	if (!cur_sensor_info)
		return -ENODEV;
	priv->powered = 1;
	ak_sensor_set_sensor_cb(cur_sensor_info);
	cur_sensor_info->sensor_get_resolution_func(&width, &height);
	cur_sensor_info->sensor_get_valid_coordinate_func(&x, &y);
	if (width <= x || height <= y)
		return -EINVAL;
	priv->win.left = 0;
	priv->win.top = 0;
	priv->win.width = width - x;
	priv->win.height = height - y;
	priv->code = cur_sensor_info->sensor_get_bus_type_func() == BUS_TYPE_YUV ?
		AK_CAM_MBUS_FMT_YUYV8_2X8 : AK_CAM_MBUS_FMT_SBGGR8_1X8;
	priv->colorspace = V4L2_COLORSPACE_SMPTE170M;
	return 0;
}

static struct v4l2_subdev_core_ops ak_cam_sensor_core_ops = {
	.init		= ak_cam_sensor_identify,
	.s_power	= ak_cam_sensor_s_power,
};

/* -------------------------------------------------------------------------
 * subdev video ops
 * ------------------------------------------------------------------------- */

static int ak_cam_sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);

	if (enable)
		return cur_sensor_info->sensor_set_standby_out_func(priv->info->pin_pwdn,
								    priv->info->pin_reset);

	return cur_sensor_info->sensor_set_standby_in_func(priv->info->pin_pwdn,
							   priv->info->pin_reset);
}

static int ak_cam_sensor_cropcap(struct v4l2_subdev *sd,
				 struct v4l2_cropcap *a)
{
	int width, height;
	int x, y;

	cur_sensor_info->sensor_get_resolution_func(&width, &height);
	cur_sensor_info->sensor_get_valid_coordinate_func(&x, &y);

	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	a->bounds.left = 0;
	a->bounds.top = 0;
	a->bounds.width = width - x;
	a->bounds.height = height - y;
	a->defrect = a->bounds;
	a->pixelaspect.numerator = 1;
	a->pixelaspect.denominator = 1;

	return 0;
}

static int ak_cam_sensor_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);

	a->c = priv->win;

	return 0;
}

static int ak_cam_sensor_s_crop(struct v4l2_subdev *sd,
				AK_CAM_S_CROP_ARG *a)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);

	priv->win = a->c;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
static int ak_cam_sensor_enum_mbus_fmt(struct v4l2_subdev *sd,
				       unsigned int index,
				       AK_CAM_MBUS_CODE *code)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);

	if (index)
		return -EINVAL;

	*code = priv->code;

	return 0;
}

static int ak_cam_sensor_g_mbus_fmt(struct v4l2_subdev *sd,
				    struct v4l2_mbus_framefmt *mf)
{
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);

	mf->width = priv->win.width;
	mf->height = priv->win.height;
	mf->code = priv->code;
	mf->colorspace = priv->colorspace;
	mf->field = V4L2_FIELD_NONE;

	return 0;
}

#endif

static int ak_cam_sensor_g_mbus_config(struct v4l2_subdev *sd,
				       struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
		     V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
		     V4L2_MBUS_DATA_ACTIVE_HIGH;

	return 0;
}

static int ak_cam_sensor_g_parm(struct v4l2_subdev *sd,
				struct v4l2_streamparm *parm)
{
	int fps;

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	fps = cur_sensor_info->sensor_get_fps_func();
	if (fps <= 0)
		return -EIO;

	memset(&parm->parm, 0, sizeof(parm->parm));
	parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	parm->parm.capture.timeperframe.numerator = 1;
	parm->parm.capture.timeperframe.denominator = fps;

	return 0;
}

static int ak_cam_sensor_s_parm(struct v4l2_subdev *sd,
				struct v4l2_streamparm *parm)
{
	struct v4l2_fract *tpf = &parm->parm.capture.timeperframe;
	int fps, ret;

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (!tpf->numerator || !tpf->denominator)
		return -EINVAL;

	fps = tpf->denominator / tpf->numerator;
	if (fps <= 0)
		return -EINVAL;

	ret = cur_sensor_info->sensor_set_fps_func(fps);
	if (ret)
		return -ERANGE;
	/* The capture bridge rejects S_PARM while frames are running. */
	if (cur_sensor_info->sensor_timer_func) {
		ret = cur_sensor_info->sensor_timer_func();
		if (ret)
			return -EIO;
	}

	return ak_cam_sensor_g_parm(sd, parm);
}

static struct v4l2_subdev_video_ops ak_cam_sensor_video_ops = {
	.s_stream	= ak_cam_sensor_s_stream,
	.cropcap	= ak_cam_sensor_cropcap,
	.g_crop		= ak_cam_sensor_g_crop,
	.s_crop		= ak_cam_sensor_s_crop,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
	.enum_mbus_fmt	= ak_cam_sensor_enum_mbus_fmt,
	.g_mbus_fmt	= ak_cam_sensor_g_mbus_fmt,
	.try_mbus_fmt	= ak_cam_sensor_g_mbus_fmt,
#endif
	.g_mbus_config	= ak_cam_sensor_g_mbus_config,
	.g_parm		= ak_cam_sensor_g_parm,
	.s_parm		= ak_cam_sensor_s_parm,
};

static struct v4l2_subdev_ops ak_cam_sensor_subdev_ops = {
	.core	= &ak_cam_sensor_core_ops,
	.video	= &ak_cam_sensor_video_ops,
};

/* -------------------------------------------------------------------------
 * i2c driver
 * ------------------------------------------------------------------------- */

static int ak_cam_sensor_probe(struct i2c_client *client,
			       const struct i2c_device_id *did)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ak_cam_sensor *priv;
	struct aksensor_camera_info *info;
	int gpio;
	int ret;

	if (!client->dev.of_node)
		return -EINVAL;
	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->pin_avdd = info->pin_power = (unsigned long)-1;
	gpio = of_get_named_gpio(client->dev.of_node, "reset-gpio", 0);
	if (gpio == -EPROBE_DEFER)
		return gpio;
	info->pin_reset = gpio >= 0 ? gpio : (unsigned long)-1;
	gpio = of_get_named_gpio(client->dev.of_node, "pwdn-gpio", 0);
	if (gpio == -EPROBE_DEFER)
		return gpio;
	info->pin_pwdn = gpio >= 0 ? gpio : (unsigned long)-1;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&adapter->dev, "no I2C_FUNC_SMBUS_BYTE_DATA\n");
		return -EIO;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->info = info;
	v4l2_i2c_subdev_init(&priv->subdev, client, &ak_cam_sensor_subdev_ops);

	ak_sensor_i2c_set_client(client);

	v4l2_ctrl_handler_init(&priv->hdl, 1);
	priv->subdev.ctrl_handler = &priv->hdl;
	if (priv->hdl.error) {
		ret = priv->hdl.error;
		v4l2_ctrl_handler_free(&priv->hdl);
		goto err_free;
	}

	return 0;

err_free:
	kfree(priv);
	return ret;
}

static int ak_cam_sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ak_cam_sensor *priv = to_ak_cam_sensor(sd);

	ak_cam_sensor_s_power(sd, 0);
	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&priv->hdl);
	kfree(priv);

	ispdrv_remove_all_sensors();
	cur_sensor_info = NULL;

	return 0;
}

static const struct i2c_device_id ak_cam_sensor_id[] = {
	{ AK_CAM_SENSOR_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak_cam_sensor_id);

static const struct of_device_id ak_cam_sensor_of_match[] = {
	{ .compatible = "anyka,sensor0" },
	{ }
};
MODULE_DEVICE_TABLE(of, ak_cam_sensor_of_match);

static struct i2c_driver ak_cam_sensor_i2c_driver = {
	.driver = {
		.name = AK_CAM_SENSOR_NAME,
		.of_match_table = ak_cam_sensor_of_match,
		.suppress_bind_attrs = true,
	},
	.probe		= ak_cam_sensor_probe,
	.remove		= ak_cam_sensor_remove,
	.id_table	= ak_cam_sensor_id,
};

static int __init ak_cam_sensor_init(void)
{
	return i2c_add_driver(&ak_cam_sensor_i2c_driver);
}

static void __exit ak_cam_sensor_exit(void)
{
	i2c_del_driver(&ak_cam_sensor_i2c_driver);
}

module_init(ak_cam_sensor_init);
module_exit(ak_cam_sensor_exit);

MODULE_DESCRIPTION("Anyka sensor subdev for the AK3918EV200 capture bridge");
MODULE_LICENSE("GPL v2");
