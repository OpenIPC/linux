/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MEDIA_SENSOR_REGMAP_H
#define __MEDIA_SENSOR_REGMAP_H

#include <linux/i2c.h>
#include <linux/regmap.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-mediabus.h>

struct regmap_sensor_mode {
	u32 code;
	u16 width;
	u16 height;
	u16 hts;
	u16 vts;
	s64 link_freq;
	u32 pixel_rate;
	const struct reg_sequence *regs;
	unsigned int num_regs;
};

struct regmap_sensor_model {
	const struct regmap_sensor_mode *modes;
	unsigned int num_modes;
	unsigned int lanes;
	unsigned long xclk;
	unsigned int pwdn_delay_ms;
	u16 exposure_min;
	u16 gain_max;
	bool has_flip;
	int (*detect)(struct device *dev, struct regmap *map);
	int (*set_ctrl)(struct regmap *map, u32 id, u32 value);
	int (*standby)(struct regmap *map, bool enable);
};

int regmap_sensor_probe(struct i2c_client *client);
void regmap_sensor_remove(struct i2c_client *client);
extern const struct dev_pm_ops regmap_sensor_pm_ops;

#endif
