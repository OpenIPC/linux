// SPDX-License-Identifier: GPL-2.0
#include <linux/usb/ch9.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/printk.h>

#include "phy-lotus-usb.h"
#include "../dwc3-lotus.h"

/* CRG cleard noly one time. We use crg_cleard to aviod
 * phy and ctrl clear crg together.
 *
 * crg_cleard: whether crg is cleard by phy or ctrl.
 */
static volatile int crg_cleard;
static volatile int u3_crg_cleard;

struct chip_param {
	unsigned int reg_offset;
	unsigned int val;
};

struct chip_param_index {
	const struct chip_param *index;
	unsigned int size;
};

/* According to IC, config step as follows:
 * STEP-1: config MISC(phy/ctrl)
 * STEP-2: config CRG(phy/ctrl), enable clk and reset
 *
 * Attention:
 * 1.crg_base = 0x12010000
 * 2.misc_base = 0x12028000
 */

#define USB_CRG_ARRAY_SIZE	3
#define USB_CRG_RESET_INDX	0
#define USB_CRG_CLK_INDX	1
#define USB_CRG_UNDO_RESET_INDX	2

static const struct chip_param u2_ctrl_misc[] = {
	/* Use default value */
	{0x24, 0x400ff},	/* MISC: U2 CTRL control */
	/* Pull up U2 vbus valid for both host and device */
	{0x980, 0x1},	/* MISC: U2 CTRL control */
};

static const struct chip_param u2_ctrl_crg[] = {
	/* enable U2 ctrl clk and do ctrl VCC reset */
	{0x140, 0x8},	/* U2 ctrl reset */
	{0x140, 0x1300},/* U2 ctrl clk */
	/* undo reset U2 ctrl */
	{0x140, 0x8},	/* U2 ctrl reset */
};

static const struct chip_param u2_phy_crg[] = {
	/* enable U2 outer phy clk and do phy reset */
	{0x140, 0x3},	/* U2 phy reset */
	{0x140, 0x4},	/* U2 phy clk */
	/* undo reset U2 phy */
	{0x140, 0x3},	/* U2 phy reset */
};

static const struct chip_param u2_phy_misc[] = {
	/* usb pll enable, utmi databus config */
	{0x2c, 0x3},	/* MISC: U2 phy control */
};

static const struct chip_param u2_out_phy_eye[] = {
	0
};

static inline void lotus_usb_clear_bits(void __iomem *addr, u32 mask)
{
	u32 reg;

	reg = readl(addr) & ~mask;
	writel(reg, addr);
}

static inline void lotus_usb_set_bits(void __iomem *addr, u32 mask)
{
	u32 reg;

	reg = readl(addr) | mask;
	writel(reg, addr);
}

static void lotus_u2_phy_eye_get(struct device *dev,
	struct lotus_usb_priv *priv)
{
	unsigned int ret;

	if (!dev || !priv)
		return;

	/* define outer phy eye diagram param exist */
	priv->eye_cfg_tx_vol1_flag = 1;
	ret = of_property_read_u32(dev->of_node, "eye_cfg_tx_vol1_offset",
					   &(priv->eye_cfg_tx_vol1_offset));
	if (ret)
		priv->eye_cfg_tx_vol1_flag = 0;

	ret = of_property_read_u32(dev->of_node, "eye_cfg_tx_vol1_val",
					   &(priv->eye_cfg_tx_vol1_val));
	if (ret)
		priv->eye_cfg_tx_vol1_flag = 0;

	priv->eye_cfg_tx_vol2_flag = 1;
	ret = of_property_read_u32(dev->of_node, "eye_cfg_tx_vol2_offset",
					   &(priv->eye_cfg_tx_vol2_offset));
	if (ret)
		priv->eye_cfg_tx_vol2_flag = 0;

	ret = of_property_read_u32(dev->of_node, "eye_cfg_tx_vol2_val",
					   &(priv->eye_cfg_tx_vol2_val));
	if (ret)
		priv->eye_cfg_tx_vol2_flag = 0;

	/* define phy ref voltage config param exist */
	priv->phy_vref_cfg_flag = 1;
	ret = of_property_read_u32(dev->of_node, "phy_vref_cfg_offset",
					   &(priv->phy_vref_cfg_offset));
	if (ret)
		priv->phy_vref_cfg_flag = 0;

	ret = of_property_read_u32(dev->of_node, "phy_vref_cfg_val",
					   &(priv->phy_vref_cfg_val));
	if (ret)
		priv->phy_vref_cfg_flag = 0;
}

/* U2 phy config in sys control reg, must called before lotus_u2_phy_clk_init */
static void lotus_u2_phy_misc_config(struct lotus_usb_priv *priv)
{
	u32 reg, offset;
	const struct chip_param *config_val;
	int i;

	pr_debug("%s: enter\n", __func__);

	if (!priv)
		return;

	if (ARRAY_SIZE(u2_phy_misc) < 1) {
		pr_err("%s: U2 phy misc param invalid!\n", __func__);
		return;
	}

	config_val = &u2_phy_misc[0];
	if (!config_val) {
		pr_err("%s: misc param not exist!\n", __func__);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(u2_phy_misc); i++, config_val++) {
		offset = config_val->reg_offset;
		reg = config_val->val;
		writel(reg, priv->misc_base + offset);
	}

	/*
	 * Host/dev mode enable disconnect detection defaultly,
	 * which will increase power consumption >=1mA.
	 */
	if (!priv->disconnect_detect_disable) {
		reg = readl(priv->misc_base + MISC_OTG_SUSPENDM_OFFSET);
		reg |= MISC_OTG_SUSPENDM_HOST;
		writel(reg, priv->misc_base + MISC_OTG_SUSPENDM_OFFSET);
	}

	pr_debug("%s: ok\n", __func__);
}

int lotus_u2_phy_clk_exit(struct lotus_usb_priv *priv)
{
	u32 mask, offset;
	const struct chip_param *config_val;

	if (!priv)
		return -1;

	if (ARRAY_SIZE(u2_phy_crg) < USB_CRG_ARRAY_SIZE) {
		pr_err("%s: U2 phy crg param invalid!\n", __func__);
		return -EINVAL;
	}

	/* disable U2 phy clk */
	config_val = &u2_phy_crg[USB_CRG_CLK_INDX];
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_clear_bits(priv->crg_base + offset, mask);
	pr_debug("%s: after clk_ex, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));
	return 0;
}

int lotus_u2_phy_clk_resume(struct lotus_usb_priv *priv)
{
	u32 mask, offset;
	const struct chip_param *config_val;

	pr_debug("%s: enter\n", __func__);

	if (!priv)
		return -EINVAL;

	if (ARRAY_SIZE(u2_phy_crg) < USB_CRG_ARRAY_SIZE) {
		pr_err("%s: U2 phy param invalid!\n", __func__);
		return -EINVAL;
	}

	/* enable U2 phy clk and undo reset */
	config_val = &u2_phy_crg[USB_CRG_CLK_INDX];
	offset = config_val->reg_offset;
	mask = config_val->val;
	pr_debug("%s: before clken, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));
	lotus_usb_set_bits(priv->crg_base + offset, mask);
	pr_debug("%s: after clken, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));

	config_val++;

	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_clear_bits(priv->crg_base + offset, mask);
	pr_debug("%s: after undo reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));
	pr_debug("%s: ok\n", __func__);

	return 0;
}

/* enable phy clk and do reset, not including ctrl clk.
 * ctrl clk enable in function 'lotus_u2_ctrl_clk_init'.
 */
int lotus_u2_phy_clk_init(struct lotus_usb_priv *priv)
{
	u32 mask, offset;
	const struct chip_param *config_val;

	pr_debug("%s: enter\n", __func__);

	if (!priv)
		return -EINVAL;

	if (ARRAY_SIZE(u2_phy_crg) < USB_CRG_ARRAY_SIZE) {
		pr_err("%s: U2 phy param invalid!\n", __func__);
		return -EINVAL;
	}

	/* enable U2 phy clk and do reset */
	config_val = &u2_phy_crg[USB_CRG_RESET_INDX];
	offset = config_val->reg_offset;
	mask = config_val->val;
	/* if crg is not cleard, clear it */
	if (crg_cleard == 0) {
		pr_debug("%s: clear crg!\n", __func__);
		writel(0, priv->crg_base + offset);
		crg_cleard = 1;
	}
	pr_debug("%s: before reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));
	lotus_usb_set_bits(priv->crg_base + offset, mask);
	pr_debug("%s: after reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));

	config_val++;
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_set_bits(priv->crg_base + offset, mask);
	pr_debug("%s: after clken, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));
	udelay(200);

	config_val++;
	/* undo U2 phy reset */
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_clear_bits(priv->crg_base + offset, mask);

	pr_debug("%s: undo reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(priv->crg_base + offset));
	pr_debug("%s: ok\n", __func__);

	return 0;
}

/* U2 ctrl config in system control reg */
void lotus_u2_ctrl_misc_config(struct dwc3_host *host)
{
	u32 reg, offset;
	const struct chip_param *config_val;
	int i;

	pr_debug("%s: enter\n", __func__);

	if (!host || !host->misc_base) {
		pr_err("%s: param invalid\n", __func__);
		return;
	}

	if (ARRAY_SIZE(u2_ctrl_misc) < 2) {
		pr_err("%s: misc param invalid!\n", __func__);
		return;
	}

	config_val = &u2_ctrl_misc[0];
	for (i = 0; i < ARRAY_SIZE(u2_ctrl_misc); i++, config_val++) {
		offset = config_val->reg_offset;
		pr_debug("%s: brfore writel 0x%x-->0x%x\n", __func__, config_val->val, offset);
		reg = config_val->val;
		writel(reg, host->misc_base + offset);
		pr_debug("%s: MISC_CTRL(0x%x)=0x%x!\n", __func__, offset,
			readl(host->misc_base + offset));
	}

	pr_debug("%s: ok\n", __func__);
}
EXPORT_SYMBOL(lotus_u2_ctrl_misc_config);

/*
 * lotus_u2_ctrl_clk_resume: recovery clk and undo reset.
 *
 * The difference between lotus_u2_ctrl_clk_resume and
 * lotus_u2_ctrl_clk_init is DO RESET.
 */
void lotus_u2_ctrl_clk_resume(void __iomem *crg_base)
{
	u32 mask, offset;
	const struct chip_param *config_val;

	pr_debug("%s: enter\n", __func__);
	if (!crg_base)
		return;

	if (ARRAY_SIZE(u2_ctrl_crg) < USB_CRG_ARRAY_SIZE) {
		pr_err("%s: U2 ctrl param invalid!\n", __func__);
		return;
	}

	config_val = &u2_ctrl_crg[USB_CRG_CLK_INDX];
	offset = config_val->reg_offset;
	mask = config_val->val;
	pr_debug("%s: U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));
	lotus_usb_set_bits(crg_base + offset, mask); /* clken */
	pr_debug("%s: after clken, U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));

	config_val++;
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_clear_bits(crg_base + offset, mask); /* undo reset */
	pr_debug("%s: after undo reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));
}
EXPORT_SYMBOL(lotus_u2_ctrl_clk_resume);

/* enable U2 controller clk and do reset, phy clk enable
 * see lotus_u2_phy_clk_init.
 */
void lotus_u2_ctrl_clk_init(void __iomem *crg_base)
{
	u32 mask, offset;
	const struct chip_param *config_val;

	pr_debug("%s: enter\n", __func__);
	if (!crg_base)
		return;

	if (ARRAY_SIZE(u2_ctrl_crg) < USB_CRG_ARRAY_SIZE) {
		pr_err("%s: U2 ctrl param invalid!\n", __func__);
		return;
	}

	config_val = &u2_ctrl_crg[USB_CRG_RESET_INDX];
	offset = config_val->reg_offset;
	mask = config_val->val;

	/* if crg is not cleard, clear it */
	if (crg_cleard == 0) {
		pr_debug("%s: clear crg!\n", __func__);
		writel(0, crg_base + offset);
		crg_cleard = 1;
	}

	/* enable U2 controller clk and do reset */
	lotus_usb_set_bits(crg_base + offset, mask);
	pr_debug("%s: after reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));

	config_val++;
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_set_bits(crg_base + offset, mask);
	pr_debug("%s: after clken, U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));

	udelay(200);

	/* undo U2 controller VCC reset */
	config_val++;
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_clear_bits(crg_base + offset, mask);
	pr_debug("%s: after undo reset, U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));
	pr_debug("%s: ok\n", __func__);
}
EXPORT_SYMBOL(lotus_u2_ctrl_clk_init);

void lotus_u2_ctrl_clk_exit(void __iomem *crg_base)
{
	u32 mask, offset;
	const struct chip_param *config_val;

	pr_debug("%s: enter\n", __func__);

	if (!crg_base)
		return;

	if (ARRAY_SIZE(u2_ctrl_crg) < USB_CRG_ARRAY_SIZE) {
		pr_err("%s: U2 ctrl crg param invalid!\n", __func__);
		return;
	}
	/* disable U2 controller clk */
	config_val = &u2_ctrl_crg[USB_CRG_CLK_INDX];
	offset = config_val->reg_offset;
	mask = config_val->val;
	lotus_usb_clear_bits(crg_base + offset, mask); /* clk disable */
	pr_debug("%s: after clk_ex, U2 CRG(0x140)=0x%x!\n", __func__, readl(crg_base + offset));
	pr_debug("%s: ok\n", __func__);
}
EXPORT_SYMBOL(lotus_u2_ctrl_clk_exit);

/* step-1: get U2 phy param */
void lotus_chip_u2_phy_para_get(struct device *dev,
	struct lotus_usb_priv *priv)
{
	if (!dev || !priv)
		return;

	lotus_u2_phy_eye_get(dev, priv);

	priv->disconnect_detect_disable = device_property_read_bool(dev,
					   "cap-disconnect-dect-disable");
}

/* step-2: config U2 phy */
void lotus_chip_u2_phy_config(struct lotus_usb_priv *priv)
{
	if (!priv)
		return;
	
	pr_debug("%s: enter\n", __func__);
	lotus_u2_phy_misc_config(priv);

	pr_debug("%s: ok\n", __func__);
}

/* U2 outer phy eye config, must called after lotus_u2_phy_clk_init. */
void lotus_u2_phy_eye_config(struct lotus_usb_priv *priv)
{
	if (!priv)
		return;

	/* HS TX voltage config, cfg_swcal_voff=4b'1001 is defaultly. */
	if (priv->eye_cfg_tx_vol1_flag)
		writel(priv->eye_cfg_tx_vol1_val, priv->phy_base +
			priv->eye_cfg_tx_vol1_offset);

	if (priv->eye_cfg_tx_vol2_flag)
		writel(priv->eye_cfg_tx_vol2_val, priv->phy_base +
			priv->eye_cfg_tx_vol2_offset);

	/* Disconnect detect threshold vol config, host_vref=4b'0110 is defaultly. */
	if (priv->phy_vref_cfg_flag)
		writel(priv->phy_vref_cfg_val, priv->phy_base +
			priv->phy_vref_cfg_offset);
}

