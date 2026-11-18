/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <drm/drm_print.h>

struct m31_dphy_priv {
	struct regmap *regmap;
	int mclkdiv2;
	const int *mclk_ratios;
	bool master;
};

struct dphy_cfg_info {
	u8 addr;
	u8 data;
};

static struct dphy_cfg_info __maybe_unused m31_cfg_10M_i2c[] = {

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1  L0
	{0x3C, 0x24},		//L3  L2
	{0x3D, 0x3},		//    L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, (0x1 << 4)},	//pre div
	{0x44, (0x0 << 4)},	//pos div
	{0x47, 0x4},		//cycle sel
	{0x48, 0x66},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0xE},		//dlane_hs_pre_time
	{0x4e, 0x1D},		//dlane_hs_zero_time
	{0x4f, 0x15},		//dlane_hs_trail_time
	{0x50, 0x5},		//clane_hs_pre_time
	{0x51, 0x2B},		//clane_hs_zero_time
	{0x52, 0xD},		//clane_hs_trail_time
	{0x53, 0xF},		//clane_hs_clk_pre_time
	{0x54, 0x71},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_12M5_i2c[] = {

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1  L0
	{0x3C, 0x24},		//L3  L2
	{0x3D, 0x3},		//    L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x10},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x4},		//cycle sel
	{0x48, 0x80},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0x10},		//dlane_hs_pre_time
	{0x4e, 0x21},		//dlane_hs_zero_time
	{0x4f, 0x17},		//dlane_hs_trail_time
	{0x50, 0x7},		//clane_hs_pre_time
	{0x51, 0x35},		//clane_hs_zero_time
	{0x52, 0xF},		//clane_hs_trail_time
	{0x53, 0xF},		//clane_hs_clk_pre_time
	{0x54, 0x73},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_37M5_i2c[] = {	//37.5M txByteClk 300Mbps

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1      L0
	{0x3C, 0x24},		//L3      L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x10},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x3},		//cycle sel
	{0x48, 0xC0},		//fbk int
	{0x49, 0x0},		//fbk int 8bit
	{0x4d, 0x11},		//dlane_hs_pre_time
	{0x4e, 0x25},		//dlane_hs_zero_time
	{0x4f, 0x19},		//dlane_hs_trail_time
	{0x50, 0xA},		//clane_hs_pre_time
	{0x51, 0x50},		//clane_hs_zero_time
	{0x52, 0x15},		//clane_hs_trail_time
	{0x53, 0x7},		//clane_hs_clk_pre_time
	{0x54, 0x45},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_62M5_i2c[] = {	//62.5M txByteClk 500Mbps

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1      L0
	{0x3C, 0x24},		//L3      L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x10},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x2},		//cycle sel
	{0x48, 0xA0},		//fbk int
	{0x49, 0x0},		//fbk int 8bit
	{0x4d, 0xC},		//dlane_hs_pre_time
	{0x4e, 0x1D},		//dlane_hs_zero_time
	{0x4f, 0x14},		//dlane_hs_trail_time
	{0x50, 0x9},		//clane_hs_pre_time
	{0x51, 0x42},		//clane_hs_zero_time
	{0x52, 0x12},		//clane_hs_trail_time
	{0x53, 0x3},		//clane_hs_clk_pre_time
	{0x54, 0x28},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_100M_i2c[] = {	//100M txByteClk 800Mbps

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1      L0
	{0x3C, 0x24},		//L3      L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x10},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x1},		//cycle sel
	{0x48, 0x80},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0x9},		//dlane_hs_pre_time
	{0x4e, 0x17},		//dlane_hs_zero_time
	{0x4f, 0x10},		//dlane_hs_trail_time
	{0x50, 0x7},		//clane_hs_pre_time
	{0x51, 0x35},		//clane_hs_zero_time
	{0x52, 0xF},		//clane_hs_trail_time
	{0x53, 0x1},		//clane_hs_clk_pre_time
	{0x54, 0x18},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_50M_i2c[] = {	//50 M txByteClk

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1      L0
	{0x3C, 0x24},		//L3      L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x10},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x2},		//cycle sel
	{0x48, 0x80},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0xA},		//dlane_hs_pre_time
	{0x4e, 0x18},		//dlane_hs_zero_time
	{0x4f, 0x11},		//dlane_hs_trail_time
	{0x50, 0x7},		//clane_hs_pre_time
	{0x51, 0x35},		//clane_hs_zero_time
	{0x52, 0xF},		//clane_hs_trail_time
	{0x53, 0x3},		//clane_hs_clk_pre_time
	{0x54, 0x25},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_25M_i2c[] = {	//25M txByteClk 200Mbps

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1      L0
	{0x3C, 0x24},		//L3      L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x10},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x3},		//cycle sel
	{0x48, 0x80},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0xC},		//dlane_hs_pre_time
	{0x4e, 0x1B},		//dlane_hs_zero_time
	{0x4f, 0x13},		//dlane_hs_trail_time
	{0x50, 0x7},		//clane_hs_pre_time
	{0x51, 0x35},		//clane_hs_zero_time
	{0x52, 0xF},		//clane_hs_trail_time
	{0x53, 0x7},		//clane_hs_clk_pre_time
	{0x54, 0x3F},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_25M_to_48M_1280p_i2c_old[] = {

	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1  L0
	{0x3C, 0x24},		//L3  L2
	{0x3D, 0x3},		//          L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x0},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x2},		//cycle sel
	{0x48, 0xC8},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0xe},		//dlane_hs_pre_time
	{0x4e, 0x23},		//dlane_hs_zero_time
	{0x4f, 0x17},		//dlane_hs_trail_time
	{0x50, 0xa},		//clane_hs_pre_time
	{0x51, 0x50},		//clane_hs_zero_time
	{0x52, 0x15},		//clane_hs_trail_time
	{0x53, 0x3},		//clane_hs_clk_pre_time
	{0x54, 0x2b},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_25M_to_48M_1280p_i2c[] = {
	/*good */
	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1  L0
	{0x3C, 0x24},		//L3  L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x0},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x3},		//cycle sel
	{0x48, 0xC6},		//fbk int
	{0x49, 0x0},		//fbk int 8
	{0x4d, 0x10},		//dlane_hs_pre_time
	{0x4e, 0x25},		//dlane_hs_zero_time
	{0x4f, 0x19},		//dlane_hs_trail_time
	{0x50, 0xa},		//clane_hs_pre_time
	{0x51, 0x4f},		//clane_hs_zero_time
	{0x52, 0x15},		//clane_hs_trail_time
	{0x53, 0x7},		//clane_hs_clk_pre_time
	{0x54, 0x45},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

static struct dphy_cfg_info __maybe_unused m31_cfg_12M_to_12M5_1280p_i2c[] = {
	/*good 2 */
	{0x39, 0x0},		//dpdn swap disable
	{0x3A, 0x10},		//cklank set
	{0x3B, 0x10},		//L1  L0
	{0x3C, 0x24},		//L3  L2
	{0x3D, 0x3},		//        L4
	{0x3E, 0x0},		//DAT ABUS16 SEL   8 bit

	{0x46, 0x0},		//pre div
	{0x44, 0x0},		//pos div
	{0x47, 0x4},		//cycle sel
	{0x48, 0x86},		//fbk int
	{0x49, 0x0},		//fbk int bit8
	{0x4d, 0x10},		//dlane_hs_pre_time
	{0x4e, 0x21},		//dlane_hs_zero_time
	{0x4f, 0x17},		//dlane_hs_trail_time
	{0x50, 0x7},		//clane_hs_pre_time
	{0x51, 0x35},		//clane_hs_zero_time
	{0x52, 0xf},		//clane_hs_trail_time
	{0x53, 0xf},		//clane_hs_clk_pre_time
	{0x54, 0x73},		//clane_hs_clk_post_time
	{0xFF, 0x0},		//
};

const struct regmap_config m31_dphy_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xFE,
	.cache_type = REGCACHE_RBTREE,
	.use_single_read = true,
	.use_single_write = true,
};

static int m31_dphy_write(struct regmap *regmap, u8 reg, u8 val)
{
	int ret;

	ret = regmap_write(regmap, reg, val);
	if (ret) {
		DRM_ERROR("write [%2x %2x] to i2c error\n", reg, val);
		return ret;
	}

	return 0;
}

static int m31_dphy_read(struct regmap *regmap, u8 reg, u8 * val)
{
	int ret;

	ret = regmap_read(regmap, reg, (u32 *) val);
	if (ret) {
		DRM_ERROR("read %2x error\n", reg);
		return ret;
	}

	return 0;
}

void m31_dphy_cfg_i2c_write(struct regmap *regmap, struct dphy_cfg_info *cfg)
{
	while (cfg->addr != 0xff) {
		DRM_INFO("write dphy cfg: [%2x %2x]\n", cfg->addr, cfg->data);

		m31_dphy_write(regmap, cfg->addr, cfg->data);
		cfg++;
	}
}

void m31_dphy_cfg_i2c_read(struct regmap *regmap, struct dphy_cfg_info *cfg)
{
	static u8 val = 0;

	while (cfg->addr != 0xff) {
		m31_dphy_read(regmap, cfg->addr, &val);

		DRM_INFO("read dphy cfg: [%2x %2x]\n", cfg->addr, val);

		cfg++;
	}
}

static int m31_dphy_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct m31_dphy_priv *dphy;

	DRM_INFO("m31 dphy i2c probe enter\n");

	dphy = devm_kzalloc(&i2c->dev, sizeof(*dphy), GFP_KERNEL);
	if (!dphy)
		return -ENOMEM;

	dphy->regmap = devm_regmap_init_i2c(i2c, &m31_dphy_regmap_config);

	m31_dphy_cfg_i2c_write(dphy->regmap, m31_cfg_12M5_i2c);

	DRM_INFO("m31 dphy cfg done\n");

	m31_dphy_cfg_i2c_read(dphy->regmap, m31_cfg_12M5_i2c);

	dev_set_drvdata(&i2c->dev, dphy);

	return 0;
}

static int m31_dphy_detect(struct i2c_client *client,
			   struct i2c_board_info *info)
{
	return 0;
};

static const struct i2c_device_id m31_dphy_id[] = {
	{"m31-dphy", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, m31_dphy_id);

static const struct of_device_id m31_i2c_dphy_of_match[] = {
	{
	 .compatible = "m31,dphy",
	 },
	{}
};

MODULE_DEVICE_TABLE(of, m31_i2c_dphy_of_match);

struct i2c_driver m31_dphy_i2c_driver = {
	.driver = {
		   .name = "m31-dphy",
		   .of_match_table = m31_i2c_dphy_of_match,
		   },
	.detect = m31_dphy_detect,
	.probe = m31_dphy_i2c_probe,
	.id_table = m31_dphy_id,
};
