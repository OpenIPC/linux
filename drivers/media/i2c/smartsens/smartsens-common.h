/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __SMARTSENS_COMMON_H__
#define __SMARTSENS_COMMON_H__

#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>

#define SMARTSENS_NUM_SUPPLIES	3

struct regval_list {
	u16 reg_num;
	u8 value;
};

struct smartsens_again_lut {
	u32 reg_code;
	u32 gain_val;
};

struct smartsens_sensor;

struct smartsens_desc {
	const char *name;
	u16 chip_id;
	u16 alt_chip_id;
	bool id_needs_stream;
	unsigned int expected_lanes;
	const s64 *link_freqs;
	unsigned int num_link_freqs;
	u32 width;
	u32 height;
	u32 fps;
	u32 mbus_code;
	enum v4l2_colorspace colorspace;
	s32 exposure_min;
	s32 exposure_max;
	s32 exposure_def;
	u32 exposure_margin;
	s32 vblank_min;
	s32 vblank_slave_min;
	s32 vblank_max;
	s32 vblank_def;
	s32 hblank;
	s64 pixel_rate;
	u32 vts_def;
	const struct smartsens_again_lut *again_lut;
	size_t again_lut_size;
	u32 max_again;
	bool sc1a4t_again_layout;
	bool half_line_exposure;
	bool no_test_pattern;
	u32 xvclk_rate;
	int (*write_gain)(struct v4l2_subdev *sd, u32 code);
	const struct regval_list *stream_on;
	const struct regval_list *stream_off;
	const struct regval_list * const *primary_seqs;
	size_t num_primary_seqs;
	const struct regval_list * const *slave_seqs;
	size_t num_slave_seqs;
	int (*set_vblank)(struct v4l2_subdev *sd, s32 val);
	int (*fsync)(struct v4l2_subdev *sd, bool slave);
};

struct smartsens_sensor {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_mbus_framefmt fmt;
	const struct smartsens_desc *desc;
	struct clk *mclk;
	struct regulator_bulk_data supplies[SMARTSENS_NUM_SUPPLIES];
	struct gpio_desc *reset_gpio;
	struct gpio_desc *pwdn_gpio;
	unsigned int height;
	unsigned int fps;
	unsigned int vts;
	unsigned int frame_length_default;
	unsigned int exposure_margin;
	struct mutex lock; /* serializes subdev state, streaming & controls */
	bool streaming;
	bool shvflip;
	bool frame_sync_slave;
	u32 secondary_i2c_addr;
	struct v4l2_mbus_config bus;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *again;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *pixel_rate;
};

static inline struct smartsens_sensor *to_smartsens_sensor(struct v4l2_subdev *sd)
{
	return container_of(sd, struct smartsens_sensor, sd);
}

int smartsens_read_reg(struct v4l2_subdev *sd, u16 reg, u8 *val);
int smartsens_write_reg(struct v4l2_subdev *sd, u16 reg, u8 val);
int smartsens_set_vblank(struct v4l2_subdev *sd, s32 val);
int smartsens_probe(struct i2c_client *client,
		    const struct smartsens_desc *desc, bool shvflip);
void smartsens_remove(struct i2c_client *client);

extern const struct dev_pm_ops smartsens_pm_ops;

#endif /* __SMARTSENS_COMMON_H__ */
