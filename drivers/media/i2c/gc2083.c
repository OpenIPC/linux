// SPDX-License-Identifier: GPL-2.0
/*
 * GalaxyCore GC2083 2-lane MIPI CSI-2 camera sensor driver
 *
 * 1920x1080 RAW10, 24 MHz input clock, 100 MHz pixel clock, 500 Mbit/s
 * per lane. The frame length follows V4L2_CID_VBLANK, so the nominal
 * 30 frames/s mode also runs at any lower rate down to the 14-bit frame
 * length limit; the frame interval operations set it the same way.
 *
 * The register usage, gain ladder and stream sequence are derived from
 * the GPL-2.0 GC2083 V4L2 driver by Lukasz Sobala (sg200x-camera-dkms),
 * cross-checked against Ingenic's GPL-2.0 GC2083 driver; see
 * gc2083-modes.h for the provenance of the mode table. The probe, power
 * and lifetime handling follow the FH8626 sensor drivers in this tree.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/math64.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/v4l2-mediabus.h>
#include <media/fh8626-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "gc2083-modes.h"

#define GC2083_REG_CHIP_ID_H		0x03f0
#define GC2083_REG_CHIP_ID_L		0x03f1
#define GC2083_CHIP_ID			0x2083

/* Exposure in lines; the top two bits of the high byte are not part of it */
#define GC2083_REG_EXPOSURE_H		0x0d03
#define GC2083_REG_EXPOSURE_L		0x0d04
#define GC2083_EXPOSURE_MASK		0x3fff
#define GC2083_EXPOSURE_MIN		1
/* The vendor settings keep the integration 8 lines inside the frame */
#define GC2083_EXPOSURE_MARGIN		8

/* Frame length (VTS) in lines */
#define GC2083_REG_VTS_H		0x0d41
#define GC2083_REG_VTS_L		0x0d42
#define GC2083_VTS_MAX			0x3fff

/* Analogue gain: see gc2083_again_table[] */
#define GC2083_REG_AGAIN_LOW		0x00d0
#define GC2083_REG_AGAIN_HIGH		0x0dc1
#define GC2083_REG_GAIN_LATCH		0x031d
#define GC2083_GAIN_LATCH_HOLD		0x2e
#define GC2083_GAIN_LATCH_RELEASE	0x28

/* Digital fine gain, Q6; the fraction is left-aligned into bits 7:2 */
#define GC2083_REG_DGAIN_INT		0x00b1
#define GC2083_REG_DGAIN_FRAC		0x00b2
#define GC2083_DGAIN_FRAC_SHIFT		2
#define GC2083_DGAIN_UNITY		64

/* Mirror and flip, written to the register and its latched shadow */
#define GC2083_REG_FLIP			0x0015
#define GC2083_REG_FLIP_SHADOW		0x0d15
#define GC2083_FLIP_H			BIT(0)
#define GC2083_FLIP_V			BIT(1)

/* MIPI output enable: the last write of stream on, the first of stream off */
#define GC2083_REG_STREAM		0x023e
#define GC2083_STREAM_ON		0x99
#define GC2083_STREAM_OFF		0x00

/* Registers of the vendor's standby sequence */
#define GC2083_REG_MIPI_EN		0x003e
#define GC2083_REG_PLL_CTRL		0x03f7
#define GC2083_REG_PLL_MODE		0x03f9
#define GC2083_REG_PWR_CTRL		0x03fc

#define GC2083_WIDTH			1920
#define GC2083_HEIGHT			1080
#define GC2083_DATA_LANES		2
#define GC2083_BPP			10

#define GC2083_XVCLK_FREQ		24000000
/* 24 MHz x 50 = 1200 MHz; 5/12 of it per lane, DDR */
#define GC2083_LINK_FREQ		250000000
/* link frequency x 2 (DDR) x lanes / bits per pixel */
#define GC2083_PIXEL_RATE		100000000
/* 2 x 0x05c9 pixel clocks per line */
#define GC2083_HTS			2962
#define GC2083_VTS_DEF			1125
#define GC2083_HBLANK			(GC2083_HTS - GC2083_WIDTH)
#define GC2083_VBLANK_MIN		(GC2083_VTS_DEF - GC2083_HEIGHT)
#define GC2083_VBLANK_MAX		(GC2083_VTS_MAX - GC2083_HEIGHT)

/* The vendor waits this long after the mode table before streaming */
#define GC2083_INIT_SETTLE_MS		80

static const s64 gc2083_link_freqs[] = {
	GC2083_LINK_FREQ,
};

static const char * const gc2083_supply_names[] = {
	"avdd",		/* analogue */
	"dovdd",	/* digital I/O */
	"dvdd",		/* digital core */
};

/* Frame rates offered by enum_frame_interval; any rate can be requested */
static const struct v4l2_fract gc2083_intervals[] = {
	{ 1, 30 }, { 1, 25 }, { 1, 20 }, { 1, 15 },
};

/*
 * The registers that together select one analogue gain step, in the
 * order they are written. Indices 0 and 1 are bracketed by the gain
 * latch, see gc2083_write_again().
 */
static const u16 gc2083_again_regs[] = {
	0x00d0, 0x0dc1, 0x00b8, 0x00b9, 0x0155, 0x0410, 0x0411, 0x0412,
	0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
};

#define GC2083_AGAIN_NUM_REGS	ARRAY_SIZE(gc2083_again_regs)

struct gc2083_again {
	u32 gain;	/* Q6: 64 is 1.0x */
	u8 regs[GC2083_AGAIN_NUM_REGS];
};

/*
 * The analogue gain steps, about 1.2x apart. gc2083_set_gain() takes the
 * highest step not above the requested gain and makes up the rest with
 * the digital fine gain, so V4L2_CID_ANALOGUE_GAIN is continuous in Q6.
 */
static const struct gc2083_again gc2083_again_table[] = {
	{ 64, { 0x00, 0x00, 0x01, 0x00, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 77, { 0x10, 0x00, 0x01, 0x0c, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 92, { 0x01, 0x00, 0x01, 0x1a, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 110, { 0x11, 0x00, 0x01, 0x2b, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 128, { 0x02, 0x00, 0x02, 0x00, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 154, { 0x12, 0x00, 0x02, 0x18, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 186, { 0x03, 0x00, 0x02, 0x33, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 223, { 0x13, 0x00, 0x03, 0x15, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 266, { 0x04, 0x00, 0x04, 0x00, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 323, { 0x14, 0x00, 0x04, 0xe0, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 387, { 0x05, 0x00, 0x05, 0x26, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 457, { 0x15, 0x00, 0x06, 0x2b, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 544, { 0x44, 0x00, 0x08, 0x00, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 653, { 0x54, 0x00, 0x09, 0x22, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 784, { 0x45, 0x00, 0x0b, 0x0d, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 914, { 0x55, 0x00, 0x0d, 0x16, 0x03, 0x11, 0x11,
		  0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 1078, { 0x04, 0x01, 0x10, 0x00, 0x19, 0x16, 0x16,
		  0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 1293, { 0x14, 0x01, 0x13, 0x04, 0x19, 0x16, 0x16,
		  0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 1541, { 0x24, 0x01, 0x16, 0x1a, 0x19, 0x16, 0x16,
		  0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 1849, { 0x34, 0x01, 0x1a, 0x2b, 0x19, 0x16, 0x16,
		  0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 2177, { 0x44, 0x01, 0x20, 0x00, 0x36, 0x18, 0x18,
		  0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 2612, { 0x54, 0x01, 0x26, 0x07, 0x36, 0x18, 0x18,
		  0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 3136, { 0x64, 0x01, 0x2c, 0x33, 0x36, 0x18, 0x18,
		  0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 3764, { 0x74, 0x01, 0x35, 0x17, 0x36, 0x18, 0x18,
		  0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f } },
	{ 4504, { 0x84, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16,
		  0x16, 0x16, 0x72, 0x72, 0x72, 0x72 } },
	{ 5376, { 0x94, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16,
		  0x16, 0x16, 0x72, 0x72, 0x72, 0x72 } },
	{ 6656, { 0x85, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16,
		  0x16, 0x16, 0x72, 0x72, 0x72, 0x72 } },
	{ 7988, { 0x95, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16,
		  0x16, 0x16, 0x72, 0x72, 0x72, 0x72 } },
	{ 9474, { 0xa5, 0x01, 0x35, 0x17, 0x64, 0x16, 0x16,
		  0x16, 0x16, 0x72, 0x72, 0x72, 0x72 } },
};

#define GC2083_AGAIN_MIN	64
#define GC2083_AGAIN_MAX	9474

/*
 * Unflipped the window starts on an RGGB quad. Mirroring moves it by a
 * column and flipping by a row; index with hflip | vflip << 1.
 */
static const u32 gc2083_mbus_codes[] = {
	MEDIA_BUS_FMT_SRGGB10_1X10,
	MEDIA_BUS_FMT_SGRBG10_1X10,
	MEDIA_BUS_FMT_SGBRG10_1X10,
	MEDIA_BUS_FMT_SBGGR10_1X10,
};

struct gc2083_dev {
	struct kref		ref;
	struct fh8626_subdev_lifetime lifetime;
	struct i2c_client	*client;
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
	struct regulator_bulk_data supplies[ARRAY_SIZE(gc2083_supply_names)];
	/* Serializes the sensor, the controls and the state below */
	struct mutex		lock;
	bool			powered;
	bool			streaming;
	bool			removed;
	struct v4l2_mbus_framefmt fmt;
};

static inline struct gc2083_dev *to_gc2083(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc2083_dev, sd);
}

static void gc2083_release(struct kref *ref)
{
	struct gc2083_dev *sensor = container_of(ref, struct gc2083_dev, ref);

	v4l2_ctrl_handler_free(&sensor->ctrls);
	media_entity_cleanup(&sensor->sd.entity);
	mutex_destroy(&sensor->lock);
	kfree(sensor);
}

static int gc2083_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_gc2083(sd)->ref);
	return 0;
}

static void gc2083_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_gc2083(sd)->ref, gc2083_release);
}

static const struct v4l2_subdev_internal_ops gc2083_internal_ops = {
	.registered = gc2083_registered,
	.unregistered = gc2083_subdev_release,
};

static u32 gc2083_bayer_code(struct gc2083_dev *sensor)
{
	unsigned int i = 0;

	if (sensor->hflip && sensor->hflip->val)
		i |= 1;
	if (sensor->vflip && sensor->vflip->val)
		i |= 2;
	return gc2083_mbus_codes[i];
}

/* 16-bit register address, 8-bit value */
static int gc2083_read_reg(struct gc2083_dev *sensor, u16 reg, u8 *val)
{
	struct i2c_client *client = sensor->client;
	u8 addr[2] = { reg >> 8, reg & 0xff };
	struct i2c_msg msgs[2] = {
		{ .addr = client->addr, .flags = 0,
		  .len = sizeof(addr), .buf = addr },
		{ .addr = client->addr, .flags = I2C_M_RD,
		  .len = 1, .buf = val },
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

static int gc2083_write_reg(struct gc2083_dev *sensor, u16 reg, u8 val)
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

static int gc2083_write_reg16(struct gc2083_dev *sensor, u16 reg, u16 val)
{
	int ret;

	ret = gc2083_write_reg(sensor, reg, val >> 8);
	if (ret)
		return ret;
	return gc2083_write_reg(sensor, reg + 1, val & 0xff);
}

static int gc2083_set_vts(struct gc2083_dev *sensor, u32 vts)
{
	return gc2083_write_reg16(sensor, GC2083_REG_VTS_H, vts);
}

static int gc2083_set_exposure(struct gc2083_dev *sensor, u32 lines)
{
	return gc2083_write_reg16(sensor, GC2083_REG_EXPOSURE_H,
				  lines & GC2083_EXPOSURE_MASK);
}

/*
 * Program one analogue gain step, in the vendor's order: the 0x0dc1
 * write is bracketed by a hold and release of the gain latch.
 */
static int gc2083_write_again(struct gc2083_dev *sensor, unsigned int idx)
{
	const u8 *v = gc2083_again_table[idx].regs;
	unsigned int i;
	int ret;

	ret = gc2083_write_reg(sensor, GC2083_REG_AGAIN_LOW, v[0]);
	if (!ret)
		ret = gc2083_write_reg(sensor, GC2083_REG_GAIN_LATCH,
				       GC2083_GAIN_LATCH_HOLD);
	if (!ret)
		ret = gc2083_write_reg(sensor, GC2083_REG_AGAIN_HIGH, v[1]);
	if (!ret)
		ret = gc2083_write_reg(sensor, GC2083_REG_GAIN_LATCH,
				       GC2083_GAIN_LATCH_RELEASE);
	for (i = 2; !ret && i < GC2083_AGAIN_NUM_REGS; i++)
		ret = gc2083_write_reg(sensor, gc2083_again_regs[i], v[i]);
	return ret;
}

/* @gain is Q6 (64 = 1.0x) */
static int gc2083_set_gain(struct gc2083_dev *sensor, u32 gain)
{
	unsigned int idx = ARRAY_SIZE(gc2083_again_table) - 1;
	unsigned int i;
	u32 dgain;
	int ret;

	for (i = 1; i < ARRAY_SIZE(gc2083_again_table); i++) {
		if (gain < gc2083_again_table[i].gain) {
			idx = i - 1;
			break;
		}
	}

	ret = gc2083_write_again(sensor, idx);
	if (ret)
		return ret;

	/* Below the next step, so the digital gain stays under 1.2x */
	dgain = gain * GC2083_DGAIN_UNITY / gc2083_again_table[idx].gain;
	ret = gc2083_write_reg(sensor, GC2083_REG_DGAIN_INT,
			       dgain / GC2083_DGAIN_UNITY);
	if (ret)
		return ret;
	return gc2083_write_reg(sensor, GC2083_REG_DGAIN_FRAC,
				(dgain % GC2083_DGAIN_UNITY) <<
				GC2083_DGAIN_FRAC_SHIFT);
}

static int gc2083_set_flip(struct gc2083_dev *sensor)
{
	u8 val = 0;
	int ret;

	if (sensor->hflip->val)
		val |= GC2083_FLIP_H;
	if (sensor->vflip->val)
		val |= GC2083_FLIP_V;

	ret = gc2083_write_reg(sensor, GC2083_REG_FLIP, val);
	if (ret)
		return ret;
	return gc2083_write_reg(sensor, GC2083_REG_FLIP_SHADOW, val);
}

/* Called with sensor->lock, which is the control handler lock, held */
static int gc2083_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct gc2083_dev *sensor =
		container_of(ctrl->handler, struct gc2083_dev, ctrls);
	s32 exposure_max;
	int ret;

	if (sensor->removed)
		return -ENODEV;

	switch (ctrl->id) {
	case V4L2_CID_VBLANK:
		/* The integration has to stay inside the new frame */
		exposure_max = GC2083_HEIGHT + ctrl->val -
			       GC2083_EXPOSURE_MARGIN;
		ret = __v4l2_ctrl_modify_range(sensor->exposure,
					       sensor->exposure->minimum,
					       exposure_max,
					       sensor->exposure->step,
					       min(sensor->exposure->val,
						   exposure_max));
		if (ret)
			return ret;
		break;
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
		/* The flips move the Bayer phase the pipeline was set up for */
		if (sensor->streaming)
			return -EBUSY;
		break;
	default:
		break;
	}

	/* Values set while powered off are applied at stream on */
	if (!sensor->powered)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		return gc2083_set_exposure(sensor, ctrl->val);
	case V4L2_CID_ANALOGUE_GAIN:
		return gc2083_set_gain(sensor, ctrl->val);
	case V4L2_CID_VBLANK:
		return gc2083_set_vts(sensor, GC2083_HEIGHT + ctrl->val);
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
		return gc2083_set_flip(sensor);
	default:
		return -EINVAL;
	}
}

static const struct v4l2_ctrl_ops gc2083_ctrl_ops = {
	.s_ctrl = gc2083_s_ctrl,
};

static int gc2083_init_controls(struct gc2083_dev *sensor)
{
	struct v4l2_ctrl_handler *hdl = &sensor->ctrls;
	struct v4l2_ctrl *ctrl;
	int ret;

	v4l2_ctrl_handler_init(hdl, 9);
	hdl->lock = &sensor->lock;

	ctrl = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_PIXEL_RATE,
				 GC2083_PIXEL_RATE, GC2083_PIXEL_RATE, 1,
				 GC2083_PIXEL_RATE);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ctrl = v4l2_ctrl_new_int_menu(hdl, NULL, V4L2_CID_LINK_FREQ,
				      ARRAY_SIZE(gc2083_link_freqs) - 1, 0,
				      gc2083_link_freqs);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ctrl = v4l2_ctrl_new_std(hdl, NULL, V4L2_CID_HBLANK,
				 GC2083_HBLANK, GC2083_HBLANK, 1,
				 GC2083_HBLANK);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->vblank = v4l2_ctrl_new_std(hdl, &gc2083_ctrl_ops,
					   V4L2_CID_VBLANK,
					   GC2083_VBLANK_MIN,
					   GC2083_VBLANK_MAX, 1,
					   GC2083_VBLANK_MIN);

	sensor->exposure = v4l2_ctrl_new_std(hdl, &gc2083_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     GC2083_EXPOSURE_MIN,
					     GC2083_VTS_DEF -
					     GC2083_EXPOSURE_MARGIN, 1,
					     GC2083_VTS_DEF -
					     GC2083_EXPOSURE_MARGIN);

	/* Q6 (64 = 1.0x), analogue steps plus digital fine gain */
	sensor->gain = v4l2_ctrl_new_std(hdl, &gc2083_ctrl_ops,
					 V4L2_CID_ANALOGUE_GAIN,
					 GC2083_AGAIN_MIN, GC2083_AGAIN_MAX, 1,
					 GC2083_AGAIN_MIN);

	sensor->hflip = v4l2_ctrl_new_std(hdl, &gc2083_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);
	sensor->vflip = v4l2_ctrl_new_std(hdl, &gc2083_ctrl_ops,
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

static int gc2083_power_on(struct gc2083_dev *sensor)
{
	int ret;

	if (sensor->powered)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(sensor->supplies),
				    sensor->supplies);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sensor->xvclk);
	if (ret) {
		regulator_bulk_disable(ARRAY_SIZE(sensor->supplies),
				       sensor->supplies);
		return ret;
	}

	/* The sensor needs its input clock before it leaves reset */
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(sensor->reset_gpio, 0);
	usleep_range(2000, 3000);

	sensor->powered = true;
	return 0;
}

static void gc2083_power_off(struct gc2083_dev *sensor)
{
	if (!sensor->powered)
		return;

	gpiod_set_value_cansleep(sensor->reset_gpio, 1);
	clk_disable_unprepare(sensor->xvclk);
	regulator_bulk_disable(ARRAY_SIZE(sensor->supplies),
			       sensor->supplies);
	sensor->powered = false;
	sensor->streaming = false;
}

static int gc2083_start_streaming(struct gc2083_dev *sensor)
{
	unsigned int i;
	int ret;

	ret = gc2083_power_on(sensor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(gc2083_1080p_regs); i++) {
		ret = gc2083_write_reg(sensor, gc2083_1080p_regs[i].reg,
				       gc2083_1080p_regs[i].val);
		if (ret)
			goto err_power;
	}

	/* Frame length, exposure, gain and flips from the controls */
	ret = __v4l2_ctrl_handler_setup(&sensor->ctrls);
	if (ret)
		goto err_power;

	msleep(GC2083_INIT_SETTLE_MS);

	/* MIPI output on last, so the first frame has the settings above */
	ret = gc2083_write_reg(sensor, GC2083_REG_STREAM, GC2083_STREAM_ON);
	if (ret)
		goto err_power;

	sensor->streaming = true;
	return 0;

err_power:
	gc2083_power_off(sensor);
	return ret;
}

static void gc2083_stop_streaming(struct gc2083_dev *sensor)
{
	static const struct gc2083_reg standby[] = {
		{ GC2083_REG_STREAM, GC2083_STREAM_OFF },
		{ GC2083_REG_MIPI_EN, 0x00 },
		{ GC2083_REG_PLL_CTRL, 0x00 },
		{ GC2083_REG_PWR_CTRL, 0x01 },
		{ GC2083_REG_PLL_MODE, 0x41 },
	};
	unsigned int i;

	/* MIPI output off first; stop at the first bus error */
	for (i = 0; i < ARRAY_SIZE(standby); i++)
		if (gc2083_write_reg(sensor, standby[i].reg, standby[i].val))
			break;
	gc2083_power_off(sensor);
}

static int gc2083_s_stream(struct v4l2_subdev *sd, int on)
{
	struct gc2083_dev *sensor = to_gc2083(sd);
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
		ret = gc2083_start_streaming(sensor);
	else
		gc2083_stop_streaming(sensor);

unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

/* The frame interval is HTS x VTS pixel clocks; at most 2962 x 0x3fff */
static void gc2083_vts_to_interval(u32 vts, struct v4l2_fract *interval)
{
	u32 num = GC2083_HTS * vts;
	u32 den = GC2083_PIXEL_RATE;
	unsigned long g = gcd(num, den);

	interval->numerator = num / g;
	interval->denominator = den / g;
}

static int gc2083_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct gc2083_dev *sensor = to_gc2083(sd);

	if (fi->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	gc2083_vts_to_interval(GC2083_HEIGHT + sensor->vblank->val,
			       &fi->interval);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc2083_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct gc2083_dev *sensor = to_gc2083(sd);
	u64 vts;
	int ret;

	if (fi->pad != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (sensor->removed) {
		ret = -ENODEV;
		goto unlock;
	}

	if (!fi->interval.numerator || !fi->interval.denominator) {
		vts = GC2083_VTS_DEF;
	} else {
		/* Nearest VTS: pixel rate x interval / HTS */
		u64 div = (u64)GC2083_HTS * fi->interval.denominator;

		vts = div64_u64((u64)GC2083_PIXEL_RATE *
				fi->interval.numerator + div / 2, div);
	}
	vts = clamp_t(u64, vts, GC2083_VTS_DEF, GC2083_VTS_MAX);

	ret = __v4l2_ctrl_s_ctrl(sensor->vblank, vts - GC2083_HEIGHT);
	if (!ret)
		gc2083_vts_to_interval(GC2083_HEIGHT + sensor->vblank->val,
				       &fi->interval);
unlock:
	mutex_unlock(&sensor->lock);
	return ret;
}

static void gc2083_fill_format(struct gc2083_dev *sensor,
			       struct v4l2_mbus_framefmt *fmt)
{
	*fmt = (struct v4l2_mbus_framefmt) {
		.width = GC2083_WIDTH,
		.height = GC2083_HEIGHT,
		.code = gc2083_bayer_code(sensor),
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_RAW,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE,
		.xfer_func = V4L2_XFER_FUNC_NONE,
	};
}

static int gc2083_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct gc2083_dev *sensor = to_gc2083(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
	} else {
		fmt->format = sensor->fmt;
		fmt->format.code = gc2083_bayer_code(sensor);
	}
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc2083_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct gc2083_dev *sensor = to_gc2083(sd);

	if (fmt->pad != 0 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE && sensor->streaming) {
		mutex_unlock(&sensor->lock);
		return -EBUSY;
	}

	/* One mode: whatever is asked for, this is what the sensor gives */
	gc2083_fill_format(sensor, &fmt->format);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
	else
		sensor->fmt = fmt->format;
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc2083_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct gc2083_dev *sensor = to_gc2083(sd);

	if (code->pad != 0 || code->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	code->code = gc2083_bayer_code(sensor);
	mutex_unlock(&sensor->lock);
	return 0;
}

static int gc2083_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct gc2083_dev *sensor = to_gc2083(sd);
	int ret = 0;

	if (fse->pad != 0 || fse->index != 0)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fse->code != gc2083_bayer_code(sensor)) {
		ret = -EINVAL;
	} else {
		fse->min_width = GC2083_WIDTH;
		fse->max_width = GC2083_WIDTH;
		fse->min_height = GC2083_HEIGHT;
		fse->max_height = GC2083_HEIGHT;
	}
	mutex_unlock(&sensor->lock);
	return ret;
}

static int
gc2083_enum_frame_interval(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_interval_enum *fie)
{
	struct gc2083_dev *sensor = to_gc2083(sd);
	int ret = 0;

	if (fie->pad != 0 || fie->index >= ARRAY_SIZE(gc2083_intervals) ||
	    fie->width != GC2083_WIDTH || fie->height != GC2083_HEIGHT)
		return -EINVAL;

	mutex_lock(&sensor->lock);
	if (fie->code != gc2083_bayer_code(sensor))
		ret = -EINVAL;
	else
		fie->interval = gc2083_intervals[fie->index];
	mutex_unlock(&sensor->lock);
	return ret;
}

static int gc2083_init_cfg(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.pad = 0,
	};

	return gc2083_set_fmt(sd, cfg, &fmt);
}

static const struct v4l2_subdev_video_ops gc2083_video_ops = {
	.s_stream = gc2083_s_stream,
	.g_frame_interval = gc2083_g_frame_interval,
	.s_frame_interval = gc2083_s_frame_interval,
};

static const struct v4l2_subdev_core_ops gc2083_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops gc2083_pad_ops = {
	.init_cfg = gc2083_init_cfg,
	.enum_mbus_code = gc2083_enum_mbus_code,
	.enum_frame_size = gc2083_enum_frame_size,
	.enum_frame_interval = gc2083_enum_frame_interval,
	.get_fmt = gc2083_get_fmt,
	.set_fmt = gc2083_set_fmt,
};

static const struct v4l2_subdev_ops gc2083_subdev_ops = {
	.core = &gc2083_core_ops,
	.video = &gc2083_video_ops,
	.pad = &gc2083_pad_ops,
};

static int gc2083_check_chip_id(struct gc2083_dev *sensor)
{
	u8 id_h, id_l;
	u16 chip_id;
	int ret;

	ret = gc2083_read_reg(sensor, GC2083_REG_CHIP_ID_H, &id_h);
	if (ret)
		return ret;
	ret = gc2083_read_reg(sensor, GC2083_REG_CHIP_ID_L, &id_l);
	if (ret)
		return ret;

	chip_id = (id_h << 8) | id_l;
	if (chip_id != GC2083_CHIP_ID) {
		dev_err(&sensor->client->dev, "unexpected chip id 0x%04x\n",
			chip_id);
		return -ENODEV;
	}
	return 0;
}

static int gc2083_check_endpoint(struct device *dev)
{
	struct v4l2_fwnode_endpoint *bus;
	struct fwnode_handle *ep;
	unsigned int i;
	int ret;

	ep = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!ep)
		return -EINVAL;
	bus = v4l2_fwnode_endpoint_alloc_parse(ep);
	fwnode_handle_put(ep);
	if (IS_ERR(bus))
		return PTR_ERR(bus);

	ret = -EINVAL;
	if (bus->bus_type != V4L2_MBUS_CSI2 ||
	    bus->bus.mipi_csi2.num_data_lanes != GC2083_DATA_LANES ||
	    bus->bus.mipi_csi2.data_lanes[0] != 1 ||
	    bus->bus.mipi_csi2.data_lanes[1] != 2 ||
	    bus->bus.mipi_csi2.clock_lane != 0) {
		dev_err(dev, "two data lanes, 1 and 2, are required\n");
		goto out;
	}
	for (i = 0; i <= GC2083_DATA_LANES; i++) {
		if (bus->bus.mipi_csi2.lane_polarities[i]) {
			dev_err(dev, "lane polarity inversion is unsupported\n");
			goto out;
		}
	}
	/* link-frequencies is optional; if given it has to be ours */
	if (bus->nr_of_link_frequencies) {
		for (i = 0; i < bus->nr_of_link_frequencies; i++)
			if (bus->link_frequencies[i] == GC2083_LINK_FREQ)
				break;
		if (i == bus->nr_of_link_frequencies) {
			dev_err(dev, "link frequency %d Hz not listed\n",
				GC2083_LINK_FREQ);
			goto out;
		}
	}
	ret = 0;
out:
	v4l2_fwnode_endpoint_free(bus);
	return ret;
}

static int gc2083_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct gc2083_dev *sensor;
	unsigned int i;
	int ret;

	ret = gc2083_check_endpoint(dev);
	if (ret)
		return ret;

	sensor = kzalloc(sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->client = client;
	kref_init(&sensor->ref);
	sensor->lifetime.ref = &sensor->ref;
	sensor->lifetime.release = gc2083_release;
	mutex_init(&sensor->lock);
	v4l2_i2c_subdev_init(&sensor->sd, client, &gc2083_subdev_ops);
	v4l2_set_subdev_hostdata(&sensor->sd, &sensor->lifetime);
	sensor->sd.internal_ops = &gc2083_internal_ops;

	sensor->xvclk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(sensor->xvclk)) {
		ret = PTR_ERR(sensor->xvclk);
		goto err_put;
	}
	if (clk_get_rate(sensor->xvclk) != GC2083_XVCLK_FREQ) {
		dev_err(dev, "%d Hz xvclk required\n", GC2083_XVCLK_FREQ);
		ret = -EINVAL;
		goto err_put;
	}

	for (i = 0; i < ARRAY_SIZE(sensor->supplies); i++)
		sensor->supplies[i].supply = gc2083_supply_names[i];
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(sensor->supplies),
				      sensor->supplies);
	if (ret)
		goto err_put;

	/* Held in reset until the sensor is powered */
	sensor->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->reset_gpio)) {
		ret = PTR_ERR(sensor->reset_gpio);
		goto err_put;
	}

	ret = gc2083_power_on(sensor);
	if (ret)
		goto err_put;
	ret = gc2083_check_chip_id(sensor);
	gc2083_power_off(sensor);
	if (ret)
		goto err_put;

	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			    V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret)
		goto err_put;

	ret = gc2083_init_controls(sensor);
	if (ret)
		goto err_put;
	gc2083_fill_format(sensor, &sensor->fmt);

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret)
		goto err_put;

	return 0;

err_put:
	kref_put(&sensor->ref, gc2083_release);
	return ret;
}

static int gc2083_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc2083_dev *sensor = to_gc2083(sd);

	v4l2_async_unregister_subdev(sd);
	i2c_set_clientdata(client, NULL);
	/* Controls forwarded by other drivers can outlive the I2C binding */
	mutex_lock(&sensor->lock);
	sensor->removed = true;
	gc2083_power_off(sensor);
	mutex_unlock(&sensor->lock);
	kref_put(&sensor->ref, gc2083_release);
	return 0;
}

static const struct of_device_id gc2083_of_match[] = {
	{ .compatible = "galaxycore,gc2083" },
	{ }
};
MODULE_DEVICE_TABLE(of, gc2083_of_match);

static const struct i2c_device_id gc2083_id[] = {
	{ "gc2083", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc2083_id);

static struct i2c_driver gc2083_i2c_driver = {
	.driver = {
		.name = "gc2083",
		.of_match_table = gc2083_of_match,
	},
	.probe		= gc2083_probe,
	.remove		= gc2083_remove,
	.id_table	= gc2083_id,
};
module_i2c_driver(gc2083_i2c_driver);

MODULE_DESCRIPTION("GalaxyCore GC2083 2-lane MIPI CSI-2 camera sensor driver");
MODULE_LICENSE("GPL v2");
