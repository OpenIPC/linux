/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Common code for SmartSens MIPI CSI-2 camera sensors
 *
 * Copyright (C) 2026 Difan Zhang
 */

#ifndef _SMARTSENS_H
#define _SMARTSENS_H

#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/videodev2.h>

#define SMARTSENS_REG_GROUP_HOLD	0x3812
#define SMARTSENS_GROUP_HOLD_START	0x00
#define SMARTSENS_GROUP_HOLD_END	0x30

#define SMARTSENS_REG_DIG_GAIN		0x3e06
#define SMARTSENS_REG_DIG_FINE_GAIN	0x3e07
#define SMARTSENS_REG_ANA_GAIN		0x3e08
#define SMARTSENS_REG_ANA_FINE_GAIN	0x3e09

/* Gain in Q6 scale (64 = 1.0x) */
#define SMARTSENS_GAIN_MIN		64

struct smartsens_sensor;

struct smartsens_reg {
	u16 reg;
	u8 val;
};

struct smartsens_interval {
	struct v4l2_fract interval;
	u32 vts;
};

struct smartsens_chip {
	const char *name;
	u16 chip_id;
	u32 width;
	u32 height;
	u32 hts;
	u32 pixel_rate;
	u32 vts_default;
	u8 lanes;
	/* Exposure register LSB shift for whole lines */
	u8 exposure_shift;
	s32 gain_max;
	const struct smartsens_reg *regs;
	unsigned int num_regs;
	/* Ordered from the shortest frame */
	const struct smartsens_interval *intervals;
	unsigned int num_intervals;
	int (*set_gain)(struct smartsens_sensor *sensor, s32 gain);
};

int smartsens_write_reg(struct smartsens_sensor *sensor, u16 reg, u8 val);
int smartsens_probe(struct i2c_client *client,
		    const struct smartsens_chip *chip);
int smartsens_remove(struct i2c_client *client);

#endif
