// SPDX-License-Identifier: GPL-2.0
/*
 * SmartSens SC1345 camera sensor driver
 *
 * 1280x720 RAW10 on one CSI-2 data lane from a 24 MHz input clock;
 * 25 frames/s by default, 30 frames/s at the shortest frame.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_device.h>

#include "smartsens.h"

/*
 * 1280x720 RAW10 on one data lane from a 24 MHz clock: 1680 clocks per
 * line, 750 lines per frame.
 */
static const struct smartsens_reg sc1345_720p_regs[] = {
	{ 0x0100, 0x00 }, { 0x36e9, 0x80 }, { 0x301f, 0x04 }, { 0x320c, 0x06 },
	{ 0x320d, 0x90 }, { 0x320e, 0x02 }, { 0x320f, 0xee }, { 0x3253, 0x0a },
	{ 0x3301, 0x04 }, { 0x3306, 0x30 }, { 0x3309, 0x48 }, { 0x330b, 0xb0 },
	{ 0x330e, 0x18 }, { 0x331f, 0x41 }, { 0x3320, 0x05 }, { 0x3333, 0x10 },
	{ 0x3364, 0x17 }, { 0x3390, 0x08 }, { 0x3391, 0x18 }, { 0x3392, 0x38 },
	{ 0x3393, 0x06 }, { 0x3394, 0x10 }, { 0x3395, 0x40 }, { 0x3620, 0x08 },
	{ 0x3622, 0xc6 }, { 0x3630, 0xc0 }, { 0x3633, 0x33 }, { 0x3637, 0x14 },
	{ 0x3638, 0x0e }, { 0x363a, 0x00 }, { 0x363c, 0x05 }, { 0x3670, 0x1e },
	{ 0x3674, 0x90 }, { 0x3675, 0x90 }, { 0x3676, 0x90 }, { 0x3677, 0x82 },
	{ 0x3678, 0x86 }, { 0x3679, 0x8b }, { 0x367c, 0x18 }, { 0x367d, 0x38 },
	{ 0x367e, 0x18 }, { 0x367f, 0x38 }, { 0x3690, 0x33 }, { 0x3691, 0x33 },
	{ 0x3692, 0x32 }, { 0x369c, 0x08 }, { 0x369d, 0x38 }, { 0x36a4, 0x08 },
	{ 0x36a5, 0x18 }, { 0x36a8, 0x02 }, { 0x36a9, 0x04 }, { 0x36aa, 0x0e },
	{ 0x36ea, 0xc7 }, { 0x36eb, 0x88 }, { 0x36ec, 0x1d }, { 0x36ed, 0x07 },
	{ 0x3e00, 0x00 }, { 0x3e01, 0x2e }, { 0x3e02, 0x00 }, { 0x3e08, 0x03 },
	{ 0x3e09, 0x20 }, { 0x4509, 0x20 }, { 0x36e9, 0x23 },
};

/* 1680 clocks per line; 750 lines give 30 frames/s */
static const struct smartsens_interval sc1345_intervals[] = {
	{ { 1, 30 }, 750 },
	{ { 1, 25 }, 900 },
	{ { 1, 15 }, 1500 },
	{ { 1, 10 }, 2250 },
};

/*
 * Coarse analogue gain doubles per step (0x03, 0x07, 0x0f, 0x1f) and the
 * fine analogue gain spans each octave in 32 steps (0x20..0x3f), up to
 * 15.75x. The analogue front end settings in 0x363a and 0x3622 change at
 * 4x and 8x; the two are latched together under group hold.
 */
static int sc1345_set_gain(struct smartsens_sensor *sensor, s32 gain)
{
	u32 steps = gain >> 1;
	unsigned int shift = fls(steps) - 6;
	u8 fine = steps >> shift;
	u8 coarse = (4 << shift) - 1;
	u8 r363a, r3622;
	int ret, end;

	if (steps < 128) {
		r363a = 0x0c;
		r3622 = 0xc6;
	} else if (steps < 256) {
		r363a = 0x16;
		r3622 = 0xc6;
	} else {
		r363a = 0x16;
		r3622 = 0xd6;
	}

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_ANA_FINE_GAIN, fine);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_ANA_GAIN, coarse);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_DIG_FINE_GAIN, 0x80);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_DIG_GAIN, 0x00);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, SMARTSENS_REG_GROUP_HOLD,
				  SMARTSENS_GROUP_HOLD_START);
	if (ret)
		return ret;

	ret = smartsens_write_reg(sensor, 0x363a, r363a);
	if (!ret)
		ret = smartsens_write_reg(sensor, 0x3622, r3622);

	/* Release the hold even after a failed write */
	end = smartsens_write_reg(sensor, SMARTSENS_REG_GROUP_HOLD,
				  SMARTSENS_GROUP_HOLD_END);

	return ret ? ret : end;
}

static const struct smartsens_chip sc1345_chip = {
	.name = "sc1345",
	.chip_id = 0xda23,
	.width = 1280,
	.height = 720,
	.hts = 1680,
	.pixel_rate = 1680 * 750 * 30,
	.vts_default = 900,
	.lanes = 1,
	/* The integration time is counted in half lines */
	.exposure_shift = 5,
	.gain_max = 1008,
	.regs = sc1345_720p_regs,
	.num_regs = ARRAY_SIZE(sc1345_720p_regs),
	.intervals = sc1345_intervals,
	.num_intervals = ARRAY_SIZE(sc1345_intervals),
	.set_gain = sc1345_set_gain,
};

static int sc1345_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	return smartsens_probe(client, &sc1345_chip);
}

static const struct of_device_id sc1345_of_match[] = {
	{ .compatible = "smartsens,sc1345" },
	{ }
};
MODULE_DEVICE_TABLE(of, sc1345_of_match);

static const struct i2c_device_id sc1345_id[] = {
	{ "sc1345", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc1345_id);

static struct i2c_driver sc1345_i2c_driver = {
	.driver = {
		.name = "sc1345",
		.of_match_table = sc1345_of_match,
	},
	.probe		= sc1345_probe,
	.remove		= smartsens_remove,
	.id_table	= sc1345_id,
};
module_i2c_driver(sc1345_i2c_driver);

MODULE_DESCRIPTION("SmartSens SC1345 camera sensor driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
