// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.

#include <linux/media-bus-format.h>
#include <linux/module.h>
#include <linux/of.h>
#include <media/v4l2-cci.h>

#include "sensor-regmap.h"

static const u8 sp140a_gain[] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
	0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e,
	0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x44, 0x48, 0x4c,
	0x50, 0x54, 0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c,
	0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb9, 0xc0, 0xc8, 0xd0, 0xd8,
	0xe0, 0xe8, 0xf0, 0xf8, 0xff,
};

static int sp140a_detect(struct device *dev, struct regmap *map)
{
	u64 hi, lo;
	int ret = 0;

	cci_write(map, CCI_REG8(0xfd), 0, &ret);
	cci_read(map, CCI_REG8(0x02), &hi, &ret);
	cci_read(map, CCI_REG8(0x03), &lo, &ret);
	if (ret)
		return ret;
	if (hi != 0x14 || lo != 0x0a) {
		dev_err(dev,
			"chip ID mismatch: got 0x%02llx%02llx, expected 0x140a\n",
			hi, lo);
		return -ENODEV;
	}
	return 0;
}

static int sp140a_standby(struct regmap *map, bool enable)
{
	int ret = 0;

	cci_write(map, CCI_REG8(0xfd), 1, &ret);
	return cci_write(map, CCI_REG8(0xac), enable ? 0x00 : 0x01, &ret);
}

static int sp140a_set_ctrl(struct regmap *map, u32 id, u32 value)
{
	int ret = 0;

	cci_write(map, CCI_REG8(0xfd), 1, &ret);
	switch (id) {
	case V4L2_CID_EXPOSURE:
		cci_write(map, CCI_REG8(0x03), value >> 8, &ret);
		cci_write(map, CCI_REG8(0x04), value & 0xff, &ret);
		break;
	case V4L2_CID_VBLANK:
		cci_write(map, CCI_REG8(0x0d), 0x10, &ret);
		cci_write(map, CCI_REG8(0x0e), value >> 8, &ret);
		cci_write(map, CCI_REG8(0x0f), value & 0xff, &ret);
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		cci_write(map, CCI_REG8(0x24), sp140a_gain[value], &ret);
		break;
	case V4L2_CID_HFLIP:
		if (!ret)
			ret = regmap_update_bits(map, 0x3f, BIT(0),
						 value ? BIT(0) : 0);
		break;
	case V4L2_CID_VFLIP:
		if (!ret)
			ret = regmap_update_bits(map, 0x3f, BIT(1),
						 value ? BIT(1) : 0);
		break;
	default:
		return -EINVAL;
	}
	return cci_write(map, CCI_REG8(0x01), 1, &ret);
}

static const struct reg_sequence sp140a_init_regs_1280_720_25fps[] = {
	{ 0xfd, 0x00 },
	{ 0x2f, 0x1b },
	{ 0x2e, 0x01 },
	{ 0x33, 0x00 },
	{ 0x30, 0x2c },
	{ 0xfd, 0x01 },
	{ 0x09, 0x02 },
	{ 0x0a, 0x32 },
	{ 0x06, 0x0b },
	{ 0x03, 0x00 },
	{ 0x04, 0xbb },
	{ 0x24, 0x10 },
	{ 0x35, 0x40 },
	{ 0x36, 0x80 },
	{ 0x37, 0x80 },
	{ 0x38, 0x80 },
	{ 0x39, 0x80 },
	{ 0x31, 0x01 },
	{ 0x01, 0x01 },
	{ 0xfb, 0x62 },
	{ 0xf9, 0x1c },
	{ 0xfd, 0x01 },
	{ 0x11, 0x80 },
	{ 0x33, 0x30 },
	{ 0x13, 0x40 },
	{ 0x16, 0x18 },
	{ 0x1c, 0xed },
	{ 0x21, 0x03 },
	{ 0x22, 0x47 },
	{ 0x28, 0x04 },
	{ 0x29, 0x0e },
	{ 0x2a, 0x1d },
	{ 0x2c, 0xe0 },
	{ 0x30, 0x01 },
	{ 0xfd, 0x01 },
	{ 0x55, 0x10 },
	{ 0x58, 0x20 },
	{ 0x5b, 0x02 },
	{ 0x5d, 0x00 },
	{ 0x5e, 0x15 },
	{ 0x66, 0x50 },
	{ 0x68, 0x52 },
	{ 0x69, 0x0a },
	{ 0x71, 0x60 },
	{ 0x72, 0x10 },
	{ 0x74, 0x60 },
	{ 0x75, 0x10 },
	{ 0x78, 0x3c },
	{ 0x84, 0x70 },
	{ 0x8a, 0x33 },
	{ 0x8b, 0x33 },
	{ 0xa1, 0x03 },
	{ 0xb1, 0x01 },
	{ 0xf7, 0x10 },
	{ 0xfd, 0x02 },
	{ 0x3a, 0xff },
	{ 0x37, 0x40 },
	{ 0xfd, 0x01 },
	{ 0x47, 0x04 },
	{ 0x48, 0x05 },
	{ 0x49, 0x00 },
	{ 0x4b, 0x04 },
	{ 0x4c, 0x02 },
	{ 0x4d, 0xd0 },
	{ 0x8f, 0x00 },
	{ 0x91, 0xd0 },
	{ 0xe7, 0x03 },
	{ 0xe7, 0x00, 50000 },
};

static const struct sensor_regmap_mode sp140a_modes[] = {
	{
		.code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width = 1280,
		.height = 720,
		.hts = 1866,
		.vts = 900,
		.pixel_rate = 42000000,
		.link_freq = 360000000,
		.regs = sp140a_init_regs_1280_720_25fps,
		.num_regs = ARRAY_SIZE(sp140a_init_regs_1280_720_25fps),
	},
};

static const struct sensor_regmap_model sp140a_model = {
	.modes = sp140a_modes,
	.num_modes = ARRAY_SIZE(sp140a_modes),
	.lanes = 1,
	.xclk = 24000000,
	.pwdn_delay_ms = 50,
	.exposure_min = 4,
	.gain_max = 64,
	.has_flip = true,
	.detect = sp140a_detect,
	.set_ctrl = sp140a_set_ctrl,
	.standby = sp140a_standby,
};

static const struct i2c_device_id sp140a_id[] = {
	{ "sp140a", (kernel_ulong_t)&sp140a_model },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sp140a_id);

static const struct of_device_id sp140a_of_match[] = {
	{ .compatible = "superpix,sp140a", .data = &sp140a_model },
	{ }
};
MODULE_DEVICE_TABLE(of, sp140a_of_match);

static struct i2c_driver sp140a_driver = {
	.driver = {
		.name = "sp140a",
		.of_match_table = sp140a_of_match,
		.pm = pm_ptr(&sensor_regmap_pm_ops),
	},
	.probe = sensor_regmap_probe,
	.remove = sensor_regmap_remove,
	.id_table = sp140a_id,
};
module_i2c_driver(sp140a_driver);

MODULE_DESCRIPTION("SuperPix SP140A image sensor driver");
MODULE_LICENSE("GPL");
