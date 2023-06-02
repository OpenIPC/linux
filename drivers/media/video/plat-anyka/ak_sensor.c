/*
 * ak sensor Driver
 *
 * Copyright (C) 2012 Anyka
 *
 * Based on anykaplatform driver,
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/v4l2-mediabus.h>
#include <linux/videodev2.h>

#include <media/soc_camera.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

#include <plat-anyka/ak_sensor.h>
#include <plat-anyka/ak_sensor_i2c.h>
#include <mach-anyka/ispdrv_interface.h>
#include <mach/gpio.h>
#include <mach/clock.h>
#include <mach/map.h>
#include <mach/clock.h>
#include <mach/map.h>

//#define SENSOR_DEBUG
#ifdef SENSOR_DEBUG
#define sensor_dbg(fmt...) 			printk(KERN_INFO "Sensor: " fmt)
#define SENDBG(fmt, args...) 		printk(KERN_INFO "Sensor: " fmt, ## args)
#else
#define sensor_dbg(fmt, args...) 	do{}while(0)
#define SENDBG(fmt, args...) 		do{}while(0)
#endif 


void ak_sensor_set_sensor_cb(AK_ISP_SENSOR_CB *cb);
static AK_ISP_SENSOR_CB *cur_sensor_info = NULL; 
static struct aksensor_color_format cur_sensor_cfmts;

struct aksensor_priv {
	struct v4l2_subdev                	subdev;
	struct v4l2_ctrl_handler	  		hdl;	
	struct aksensor_camera_info        	*info;
	const struct aksensor_color_format 	*cfmt;
	struct aksensor_win_size			win;
	int									model;
};

int aksensor_get_sensor_id(void)
{
	return (cur_sensor_info ? cur_sensor_info->sensor_read_id_func() : 0);
}
EXPORT_SYMBOL_GPL(aksensor_get_sensor_id);

int aksensor_get_sensor_if(char *if_str)
{
	int ret = -1;
	int sensor_if = -1;
	int mipi_lane = 1;

	if (cur_sensor_info && cur_sensor_info->sensor_get_parameter_func)
		ret = cur_sensor_info->sensor_get_parameter_func(GET_INTERFACE, &sensor_if);

	if (!ret) {
		switch (sensor_if) {
			case DVP_INTERFACE:
				strcpy(if_str, "dvp");
				break;

			case MIPI_INTERFACE:
				ret = cur_sensor_info->sensor_get_parameter_func(GET_MIPI_LANE, &mipi_lane);
				sprintf(if_str, "mipi%d", mipi_lane);

				if (ret) {
					printk(KERN_ERR "%s not defined lane\n", __func__);
				}
				break;

			default:
				printk(KERN_ERR "%s not defined interface:%d\n", __func__, sensor_if);
				strcpy(if_str, "unknowif");
				break;
		}
	} else {
		strcpy(if_str, "unknowif");
	}

	return 0;
}
EXPORT_SYMBOL_GPL(aksensor_get_sensor_if);

/*****************************************/
static struct aksensor_priv *to_aksensor(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct aksensor_priv, subdev);
}

/**
 * @brief camera probe pointer
 * @author dengzhou
 * @date 2012-03-16
 * @param
 * @return sensor_info * camera device pointer
 * @retval
 */ 
static AK_ISP_SENSOR_CB *probe_sensors(struct i2c_client *client)
{
	struct aksensor_priv *priv = to_aksensor(client);
	int read_id, probe_id;
	int index = 0;
	AK_ISP_SENSOR_CB *si;

	do {
		si = (AK_ISP_SENSOR_CB *)ispdrv_get_sensor(&index);
		if (si) {
			si->sensor_set_power_on_func(priv->info->pin_pwdn, priv->info->pin_reset);

			read_id = si->sensor_read_id_func();
			probe_id = si->sensor_probe_id_func();
			
			if (read_id == probe_id) {
				dev_info(&client->dev, "%s Probing Sensor ID: 0x%x\n",
					__func__, read_id);
				return si;
			} else {
				si->sensor_set_power_off_func(priv->info->pin_pwdn, priv->info->pin_reset);
			}
		}
		index++;
	} while (si);
	
	return NULL;
}

/**
 * @brief: initial sensor device, open clock
 * 
 * @author: caolianming
 * @date: 2014-01-09
 * @param [in] *sd: v4l2_subdev struct, v4l2 sub-device info
 * @param [in] val: none
 */
static int aksensor_init(struct v4l2_subdev *sd, u32 val)
{
	struct aksensor_priv *priv = container_of(sd, struct aksensor_priv, subdev);

	SENDBG("entry %s\n", __func__);
	return cur_sensor_info->sensor_set_power_on_func(priv->info->pin_pwdn, priv->info->pin_reset);
}

/**
 * @brief: close sensor device, close clock
 * 
 * @author: caolianming
 * @date: 2014-01-09
 * @param [in] *sd: v4l2_subdev struct, v4l2 sub-device info
 * @param [in] val: none
 */
static int aksensor_reset( struct v4l2_subdev *sd, u32 val )
{
	struct aksensor_priv *priv = container_of(sd, struct aksensor_priv, subdev);

	SENDBG("entry %s\n", __func__);
	return cur_sensor_info->sensor_set_power_off_func(priv->info->pin_pwdn, priv->info->pin_reset);
}

static int aksensor_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	SENDBG("entry %s\n", __func__);

	id->ident = cur_sensor_info->sensor_read_id_func();
	id->revision = 0;

	return 0;
}

static int aksensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk(KERN_ERR "%s no support any ctrl\n", __func__);
	return -1;
}

/**
 * @brief: query v4l2 sub-device ctroller function
 * 
 * @author: caolianming
 * @date: 2014-01-09
 * @param [in] *sd: v4l2_subdev struct, v4l2 sub-device info
 * @param [in] *ctrl: v4l2_control struct
 */
static int aksensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	printk(KERN_ERR "%s no support any ctrl\n", __func__);
	return -1;
}

static struct v4l2_subdev_core_ops aksensor_subdev_core_ops = {
	.init			= aksensor_init,
	.reset			= aksensor_reset,
	.g_chip_ident	= aksensor_g_chip_ident,
	.g_ctrl			= aksensor_g_ctrl,
	.s_ctrl			= aksensor_s_ctrl,
};

/*
 * soc_camera_ops function
 */
static int aksensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct aksensor_priv *priv = container_of(sd, struct aksensor_priv, subdev);

	SENDBG("entry %s\n", __func__);

	switch (enable) {
	case 0:
	case -2:
		/* standby */
		printk("aksensor_s_stream standby.\n");
		cur_sensor_info->sensor_set_standby_in_func(priv->info->pin_pwdn, priv->info->pin_reset);
		break;
	case 1:
	case -1:
		/* normal */
		printk("aksensor_s_stream resume.\n");
		cur_sensor_info->sensor_set_standby_out_func(priv->info->pin_pwdn, priv->info->pin_reset);
		break;
	default:
		break;
	}

	return 0;
}

static int aksensor_get_params(struct i2c_client *client,
			enum v4l2_mbus_pixelcode code)
{
	struct aksensor_priv *priv = to_aksensor(client);
	int ret = -EINVAL;

	SENDBG("entry %s\n", __func__);
 
	/*
	 * select format
	 */
	priv->cfmt = NULL;
	if (code == cur_sensor_cfmts.code) {
		priv->cfmt = &cur_sensor_cfmts;
	}
	if (!priv->cfmt)
		goto aksensor_set_fmt_error;
 
	return 0;

aksensor_set_fmt_error:
	priv->cfmt = NULL;
	return ret;
}

static int aksensor_get_num_params(struct i2c_client *client, int num)
{
	struct aksensor_priv *priv = to_aksensor(client);
	int ret = -EINVAL;

	SENDBG("entry %s\n", __func__);
 
	/*
	 * select format
	 */
	priv->cfmt = NULL;
	if (num <= 1)
		priv->cfmt = &cur_sensor_cfmts;

	if (!priv->cfmt)
		goto aksensor_set_fmt_error;
 
	return 0;

aksensor_set_fmt_error:
	priv->cfmt = NULL;
	return ret;
}

/* first called by soc_camera_prove to initialize icd->user_width... */
static int aksensor_g_fmt(struct v4l2_subdev *sd, 
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct aksensor_priv *priv = container_of(sd, struct aksensor_priv, subdev);
	int ret;

	SENDBG("entry %s\n", __func__);

	if (!priv->cfmt) {
		SENDBG("select VGA for first time\n");
		ret = aksensor_get_params(client, V4L2_MBUS_FMT_YUYV8_2X8);
		if (ret)
			ret = aksensor_get_num_params(client, 0);
		if (ret < 0)
			return ret;
	}

	mf->width	= priv->win.width;
	mf->height	= priv->win.height;
	mf->code	= priv->cfmt->code;
	mf->colorspace	= priv->cfmt->colorspace;
	mf->field		= V4L2_FIELD_NONE;

	return 0;
}


static int aksensor_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	SENDBG("entry %s\n", __func__);
	SENDBG("leave %s\n", __func__);
	
	return 0;
}

static int aksensor_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct aksensor_priv *priv = to_aksensor(client);
	int width, height;
	int w, h;

	sensor_dbg("entry %s. priv=%p\n", __func__, priv);

	cur_sensor_info->sensor_get_resolution_func(&width, &height);
	cur_sensor_info->sensor_get_valid_coordinate_func(&w, &h);
	width -= w;
	height -= h;
	a->bounds.width	= width;
	a->bounds.height = height;
	a->bounds.left			= 0;
	a->bounds.top			= 0;
		
	a->defrect.width		= priv->win.width;
	a->defrect.height		= priv->win.height;
	a->defrect.left			= 0;
	a->defrect.top			= 0;
	
	a->type					= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	a->pixelaspect.numerator	= 1;
	a->pixelaspect.denominator	= 1;
	//printk(KERN_ERR "%s i:%d, w:%d, h:%d\n", __func__, i, cur_sensor_info->resolution[i].width, cur_sensor_info->resolution[i].height);

	sensor_dbg("%s.\n"
				"	a->bounds.width=%d, a->bounds.height=%d\n"
				"	a->defrect.width=%d, a->defrect.height=%d\n",
				__func__, 
				a->bounds.width, a->bounds.height,
				a->defrect.width, a->defrect.height);
	return 0;
}

static int aksensor_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct aksensor_priv *priv = to_aksensor(client);

	sensor_dbg("entry %s\n", __func__);

	a->c.left   = priv->win.left;
	a->c.top    = priv->win.top;
	a->c.width  = priv->win.width;
	a->c.height = priv->win.height;

	return 0;
}

static int aksensor_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct aksensor_priv *priv = to_aksensor(client);

	sensor_dbg("entry %s\n", __func__);

	priv->win.left = a->c.left;
	priv->win.top = a->c.top;
	priv->win.width = a->c.width;
	priv->win.height = a->c.height;

	return 0;
}

static int aksensor_video_probe(struct i2c_client *client)
{
	struct aksensor_priv *priv = to_aksensor(client);
	enum sensor_bus_type bus_type;
		
	SENDBG("entry %s\n", __func__);

	/*
	 * check and show product ID and manufacturer ID
	 */
	ak_sensor_i2c_set_client(client);
	if (cur_sensor_info != NULL) {
		dev_info(&client->dev, "Probing Sensor ID 0x%x\n",
				cur_sensor_info->sensor_read_id_func());
	} else {
		cur_sensor_info = probe_sensors(client);
		if (cur_sensor_info == NULL) {
			dev_err(&client->dev,  "Sensor ID error\n");
			return -ENODEV;	
		}
	}

	bus_type = cur_sensor_info->sensor_get_bus_type_func();
	switch (bus_type) {
		case BUS_TYPE_RAW:
			cur_sensor_cfmts.code = /*V4L2_MBUS_FMT_RGB555_2X8_PADHI_LE;*/V4L2_MBUS_FMT_SBGGR8_1X8;
			cur_sensor_cfmts.colorspace = V4L2_COLORSPACE_SRGB;
			break;

		case BUS_TYPE_YUV:
			cur_sensor_cfmts.code = V4L2_MBUS_FMT_YUYV8_2X8;
			cur_sensor_cfmts.colorspace = V4L2_COLORSPACE_SMPTE170M;
			break;

		default:
			cur_sensor_cfmts.code = V4L2_MBUS_FMT_SBGGR8_1X8;
			cur_sensor_cfmts.colorspace = V4L2_COLORSPACE_SRGB;
			printk(KERN_ERR "%s bus type:%d not defined!!!!!!!!!!\n", __func__, bus_type);
			break;
	}
	
	priv->model = cur_sensor_info->sensor_read_id_func();

	return 0;
}

static int aksensor_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= 1)
		return -EINVAL;

	*code = cur_sensor_cfmts.code;
	return 0;
}

static int aksensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_link *icl = soc_camera_i2c_to_link(client);

	SENDBG("entry %s\n", __func__);
	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
		V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
		V4L2_MBUS_DATA_ACTIVE_HIGH;
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = soc_camera_apply_board_flags(icl, cfg);
	SENDBG("leave %s\n", __func__);

	return 0;
}

static struct v4l2_subdev_video_ops aksensor_subdev_video_ops = {
	.s_stream		= aksensor_s_stream,
	.cropcap		= aksensor_cropcap,
	.g_crop     = aksensor_g_crop,
	.s_crop     = aksensor_s_crop,
	.enum_mbus_fmt	= aksensor_enum_fmt,
	.try_mbus_fmt	= aksensor_try_fmt,
	.g_mbus_config	= aksensor_g_mbus_config,
	.g_mbus_fmt		= aksensor_g_fmt,
};

static struct v4l2_subdev_ops aksensor_subdev_ops = {
	.core	= &aksensor_subdev_core_ops,
	.video	= &aksensor_subdev_video_ops,
};

/*
 * i2c_driver function
 */
static int aksensor_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct aksensor_priv        *priv;
	struct soc_camera_link	*icl = soc_camera_i2c_to_link(client);
	struct i2c_adapter        *adapter = to_i2c_adapter(client->dev.parent);
	int ret;
	int width, height;
	int w, h;

	SENDBG("entry %s\n", __func__);

	if (!icl || !icl->priv) {
		dev_err(&client->dev, "AKSENSOR: missing platform data!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&adapter->dev,
			"I2C-Adapter doesn't support "
			"I2C_FUNC_SMBUS_BYTE_DATA\n");
		return -EIO;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}	

	priv->info = icl->priv;
	v4l2_i2c_subdev_init(&priv->subdev, client, &aksensor_subdev_ops);

	ret = aksensor_video_probe(client);
	if (ret) {
		kfree(priv);
		return ret;
	}

	/* register sensor callback to ISPDRV */
	ak_sensor_set_sensor_cb(cur_sensor_info);

#if 0
	v4l2_ctrl_handler_init(&priv->hdl, cur_sensor_info->nr_ctrls);
	for (i = 0; i < cur_sensor_info->nr_ctrls; i++)
		v4l2_ctrl_new_custom(&priv->hdl, &cur_sensor_info->ctrls[i], NULL);
	priv->subdev.ctrl_handler = &priv->hdl;
	if (priv->hdl.error) {
		int err = priv->hdl.error;
		v4l2_ctrl_handler_free(&priv->hdl);
		kfree(priv);
		return err;
	}
#endif

	// init sensor resolution, default VGA
	cur_sensor_info->sensor_get_resolution_func(&width, &height);
	cur_sensor_info->sensor_get_valid_coordinate_func(&w, &h);
	width -= w;
	height -= h;
	priv->win.width = width; 
	priv->win.height = height;
	/*printk(KERN_ERR "%s: priv->win.width=%d priv->win.height=%d\n",
	  __func__, priv->win.width, priv->win.height);*/
	sensor_dbg("%s: priv->win.width=%d priv->win.height=%d\n",
			__func__, priv->win.width, priv->win.height);
	return ret;
}

static int aksensor_remove(struct i2c_client *client)
{
	struct aksensor_priv *priv = to_aksensor(client);

	cur_sensor_info->sensor_set_power_off_func(priv->info->pin_pwdn, priv->info->pin_reset);
	
	v4l2_device_unregister_subdev(&priv->subdev);
	//v4l2_ctrl_handler_free(&priv->hdl);
	kfree(priv);

	ispdrv_remove_all_sensors();

	return 0;
}

static const struct i2c_device_id aksensor_id[] = {
	{ "aksensor", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, aksensor_id);

static struct i2c_driver aksensor_i2c_driver = {
	.driver = {
		.name = "aksensor",
	},
	.probe    = aksensor_probe,
	.remove   = aksensor_remove,
	.id_table = aksensor_id,
};

/*
 * module function
 */

int aksensor_module_init(void)
{
	SENDBG("entry %s\n", __func__);

	printk(KERN_ERR "%s\n", __func__);
	return i2c_add_driver(&aksensor_i2c_driver);
}

void aksensor_module_exit(void)
{
	SENDBG("entry %s\n", __func__);

	i2c_del_driver(&aksensor_i2c_driver);
}

/*module_init(aksensor_module_init);
module_exit(aksensor_module_exit);

MODULE_DESCRIPTION("SoC Camera driver for aksensor");
MODULE_AUTHOR("dengzhou");
MODULE_LICENSE("GPL v2");
*/
