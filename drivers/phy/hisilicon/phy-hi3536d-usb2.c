// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon Hi3536D V100 USB 2.0 PHY driver
 *
 * Copyright (c) 2026 Dylan Corrales
 *
 * The start and stop sequences, the delays and the PHY tuning values follow
 * HiSilicon's GPL vendor USB PHY driver (hisi_usb_phy_on() and
 * hisi_usb_phy_off() of its phy-hi3536d-usb.c), changed where the data sheet
 * is stricter (the delay before the port resets are released, the controller
 * resets of the stop sequence); the register and bit meanings are from the
 * data sheet (PERI_CRG46, PERI_CRG47, MISC_CTRL20, MISC_CTRL21 and the PHY
 * registers of section 11.6).
 *
 * The two-port PHY is released from reset and clocked through PERI_CRG46 and
 * PERI_CRG47 of the clock and reset generator. Its registers are a window at
 * offset 0x8000 of the MISC_CTRL block, reachable once MISC_CTRL21 selects the
 * bus access mode; the USB control words MISC_CTRL20 and MISC_CTRL21 are in
 * the same block. The PHY node is a child of the MISC_CTRL node and uses its
 * regmap.
 *
 * The vendor driver leaves MISC_CTRL20 at its reset value and never sets the
 * "not in reset" bit of the PHY test logic itself (the vendor boot loader
 * does, which survives a soft reset). Neither is assumed here: every bit of
 * MISC_CTRL20 that the controller reads at start is programmed, and the test
 * logic is released together with the bus access mode.
 */

#include <linux/array_size.h>
#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/types.h>

/*
 * MISC_CTRL20, the USB 2.0 control word 0. The burst, power enable and
 * interface bits are read by the controller at start; usb_r_tuning is the
 * fused tuning value of the PHY internal resistor (read only).
 */
#define USB_CTRL0_REG			0x50
#define USB_CTRL0_WORD_IF		BIT(0)	/* 0: 8-bit, 1: 16-bit UTMI */
#define USB_CTRL0_OHCI_SUSP_LGCY	BIT(1)	/* OHCI suspend strap */
#define USB_CTRL0_APP_START_CLK		BIT(2)	/* 1: OHCI clock on in suspend */
#define USB_CTRL0_INCRX_ALIGN		BIT(6)
#define USB_CTRL0_INCR4			BIT(7)
#define USB_CTRL0_INCR8			BIT(8)
#define USB_CTRL0_INCR16		BIT(9)
#define USB_CTRL0_P0_PWR_EN		BIT(12)	/* 1: port 0 power output on */
#define USB_CTRL0_P1_PWR_EN		BIT(13)	/* 1: port 1 power output on */
#define USB_CTRL0_PWREN_POL		BIT(18)	/* 1: power enable active high */
#define USB_CTRL0_PWR_MERGE		BIT(20)	/* 1: one power enable signal */
#define USB_CTRL0_R_TUNING		GENMASK(27, 23)

#define USB_CTRL0_MASK			(USB_CTRL0_WORD_IF | \
					 USB_CTRL0_OHCI_SUSP_LGCY | \
					 USB_CTRL0_APP_START_CLK | \
					 USB_CTRL0_INCRX_ALIGN | \
					 USB_CTRL0_INCR4 | USB_CTRL0_INCR8 | \
					 USB_CTRL0_INCR16 | \
					 USB_CTRL0_P0_PWR_EN | \
					 USB_CTRL0_P1_PWR_EN | \
					 USB_CTRL0_PWREN_POL | \
					 USB_CTRL0_PWR_MERGE)

/*
 * The state the data sheet gives as the reset value, which the vendor driver
 * runs with: 8-bit UTMI, all burst types, both power outputs on, OHCI strap
 * and clock control off. The two board dependent bits are added from the
 * device tree (the reset value is active high, not merged).
 */
#define USB_CTRL0_ACTIVE		(USB_CTRL0_INCRX_ALIGN | \
					 USB_CTRL0_INCR4 | USB_CTRL0_INCR8 | \
					 USB_CTRL0_INCR16 | \
					 USB_CTRL0_P0_PWR_EN | \
					 USB_CTRL0_P1_PWR_EN)

/*
 * MISC_CTRL21, the USB 2.0 control word 1: access to the PHY test registers.
 * The vendor kernel driver sets the bus access mode (bit 22); the vendor boot
 * loader releases the test logic (bit 23) and uses the indirect mode with the
 * write enable, address and data fields. Here the bus access mode is used
 * with the test logic released and the indirect fields (write enable, address
 * and data, which the boot loader leaves set) cleared.
 */
#define USB_CTRL1_REG			0x54
#define USB_CTRL1_TEST_RSTN		BIT(23)	/* 0: PHY test logic in reset */
#define USB_CTRL1_BUS_ACCESS		BIT(22)	/* 1: bus access mode */
#define USB_CTRL1_BUS			(USB_CTRL1_TEST_RSTN | USB_CTRL1_BUS_ACCESS)
#define USB_CTRL1_WREN			BIT(21)	/* indirect mode: write strobe */
#define USB_CTRL1_ADDR			GENMASK(18, 8)	/* indirect mode: register */
#define USB_CTRL1_DATA			GENMASK(7, 0)	/* indirect mode: value */
#define USB_CTRL1_INDIRECT		(USB_CTRL1_WREN | USB_CTRL1_ADDR | \
					 USB_CTRL1_DATA)
#define USB_CTRL1_MASK			(USB_CTRL1_BUS | USB_CTRL1_INDIRECT)

/*
 * PHY registers of the MISC_CTRL window: one set of words per port (port 0 at
 * 0x8000, port 1 at 0x8400) and one shared configuration word. The data sheet
 * describes three per port: the pre-emphasis switch, the amplitude (RCOMP) and
 * the pre-emphasis strength. The resistor tuning, the disconnect threshold
 * and the configuration word are in the vendor driver only, with these
 * names; all values are the vendor's.
 */
#define PHY_PORT_STRIDE			0x400
#define PHY_PORT0_BASE			0x8000
#define PHY_NUM_PORTS			2

#define PHY_PORT_PRE_EMPHASIS_EN	0x00	/* vendor: pre-open */
#define PHY_PORT_RCOMP			0x08	/* vendor: eye height */
#define PHY_PORT_R_TUNING		0x0c
#define PHY_PORT_R_TUNING_FIELD		GENMASK(6, 2)
#define PHY_PORT_DISCONNECT		0x10
#define PHY_PORT_PRE_EMPHASIS_STRENGTH	0x14	/* vendor: pre-improve */
#define PHY_CFG_REG			0x8018

#define PHY_CFG_VAL			0x05
/* non-chirp pre-emphasis on, chirp and EOP pre-emphasis off, bits [4:3] as at reset */
#define PHY_PRE_EMPHASIS_EN_VAL		0x1c
/* amplitude code 101, 425 mV in HS mode (reset: 000, 400 mV), bits [3:0] as at reset */
#define PHY_RCOMP_VAL			0x5c
/* the vendor value: strength code [4:2] = 0 (lowest) and the reserved bit 7 set */
#define PHY_PRE_EMPHASIS_STRENGTH_VAL	0x80
#define PHY_DISCONNECT_VAL		0x17
#define PHY_REG_VAL_MASK		GENMASK(7, 0)

/*
 * Delays of the vendor sequences (udelay() calls of hisi_usb_phy_on() and
 * hisi_usb_phy_off()). The data sheet gives two minimums: at least 1 ms
 * between the PHY reset release and the port TREQ release (the vendor
 * sequence has 300 + 200 us in between, which is less) and at least 200 us
 * between asserting and releasing a port TREQ reset. The delay after the PHY
 * reset release is raised so that the PHY configuration word and the port
 * release come more than 1 ms after it.
 */
#define USB_MODE_DELAY_US		200	/* after the MISC_CTRL21 write */
#define USB_RESET_DELAY_US		200	/* after each reset assert */
#define USB_REF_CLK_DELAY_US		100	/* after the reference clock on */
#define USB_PHY_RELEASE_DELAY_US	1000	/* after the PHY reset release (vendor: 300) */
#define USB_CFG_DELAY_US		200	/* after the PHY_CFG word */
#define USB_PORT_RELEASE_DELAY_US	300	/* after the port TREQ release */
#define USB_CTRL_RELEASE_DELAY_US	200	/* after the controller release */
#define USB_TUNE_DELAY_US		100	/* after each tuning word */
#define USB_OFF_RESET_DELAY_US		100	/* stop: after the PHY resets */
#define USB_OFF_REF_CLK_DELAY_US	300	/* stop: after the reference clock */
#define USB_OFF_CLK_DELAY_US		200	/* stop: after the controller clocks */

/* Reset lines: the controller group (PERI_CRG46) then the PHY group (PERI_CRG47) */
enum hi3536d_usb2_reset {
	RST_BUS,
	RST_UTMI0,
	RST_UTMI1,
	RST_HST_PHY,
	RST_PHY,
	RST_PORT0_TREQ,
	RST_PORT1_TREQ,
	RST_TEST,
	NUM_RESETS
};

static const char * const hi3536d_usb2_reset_names[NUM_RESETS] = {
	[RST_BUS]	= "bus",
	[RST_UTMI0]	= "utmi0",
	[RST_UTMI1]	= "utmi1",
	[RST_HST_PHY]	= "hst-phy",
	[RST_PHY]	= "phy",
	[RST_PORT0_TREQ] = "port0-treq",
	[RST_PORT1_TREQ] = "port1-treq",
	[RST_TEST]	= "test",
};

/* The six gates of PERI_CRG46, switched together; the reference clock is separate */
static const char * const hi3536d_usb2_gate_names[] = {
	"bus", "ohci48m", "ohci12m", "hst-phy", "utmi0", "utmi1",
};

struct hi3536d_usb2_phy {
	struct device *dev;
	struct regmap *misc;
	struct clk *ref_clk;
	struct clk_bulk_data gates[ARRAY_SIZE(hi3536d_usb2_gate_names)];
	struct reset_control_bulk_data rsts[NUM_RESETS];
	u32 ctrl0;		/* the MISC_CTRL20 bits set by the device tree */
};

/* Write one PHY register and read it back, as the PHY is only partly documented */
static int hi3536d_usb2_write_checked(struct hi3536d_usb2_phy *priv, u32 off,
				      u32 val)
{
	u32 got;
	int ret;

	ret = regmap_write(priv->misc, off, val);
	if (ret)
		return ret;

	ret = regmap_read(priv->misc, off, &got);
	if (ret)
		return ret;
	got &= PHY_REG_VAL_MASK;
	if (got != val)
		dev_warn(priv->dev,
			 "PHY register 0x%04x reads back 0x%02x, wrote 0x%02x\n",
			 off, got, val);

	return 0;
}

/* Write one PHY register of every port, with the vendor settle time after each */
static int hi3536d_usb2_tune(struct hi3536d_usb2_phy *priv, u32 reg, u32 val)
{
	int port, ret;

	for (port = 0; port < PHY_NUM_PORTS; port++) {
		ret = hi3536d_usb2_write_checked(priv, PHY_PORT0_BASE +
						 port * PHY_PORT_STRIDE + reg,
						 val);
		if (ret)
			return ret;
		fsleep(USB_TUNE_DELAY_US);
	}

	return 0;
}

/*
 * Copy the fused tuning value (usb_r_tuning, bits [27:23] of MISC_CTRL20) to
 * bits [6:2] of the resistor tuning register of each port. A zero value
 * means the die carries no trim and the PHY default stays.
 */
static int hi3536d_usb2_trim(struct hi3536d_usb2_phy *priv)
{
	u32 ctrl0, trim;
	int port, ret;

	ret = regmap_read(priv->misc, USB_CTRL0_REG, &ctrl0);
	if (ret)
		return ret;

	trim = FIELD_GET(USB_CTRL0_R_TUNING, ctrl0);
	if (!trim) {
		dev_info(priv->dev, "no fused USB PHY resistor trim (usb_r_tuning is 0), PHY defaults kept\n");
		return 0;
	}

	for (port = 0; port < PHY_NUM_PORTS; port++) {
		u32 off = PHY_PORT0_BASE + port * PHY_PORT_STRIDE +
			  PHY_PORT_R_TUNING;
		u32 got;

		ret = regmap_update_bits(priv->misc, off, PHY_PORT_R_TUNING_FIELD,
					 FIELD_PREP(PHY_PORT_R_TUNING_FIELD, trim));
		if (ret)
			return ret;

		ret = regmap_read(priv->misc, off, &got);
		if (ret)
			return ret;
		got = FIELD_GET(PHY_PORT_R_TUNING_FIELD, got);
		if (got != trim)
			dev_warn(priv->dev,
				 "PHY register 0x%04x reads back trim 0x%x, wrote 0x%x\n",
				 off, got, trim);
	}

	return 0;
}

static int hi3536d_usb2_phy_init(struct phy *phy)
{
	struct hi3536d_usb2_phy *priv = phy_get_drvdata(phy);
	struct reset_control_bulk_data *rsts = priv->rsts;
	u32 val, want;
	int ret;

	ret = clk_bulk_prepare(ARRAY_SIZE(priv->gates), priv->gates);
	if (ret)
		return ret;
	ret = clk_prepare(priv->ref_clk);
	if (ret)
		goto err_unprepare;

	/*
	 * Bus access mode to the PHY registers, test logic released, and the
	 * indirect mode fields (write enable, address, data) cleared: a boot
	 * loader that used the indirect mode leaves them set. The fields are
	 * written in the same access that selects the bus mode, so no write
	 * strobe reaches the PHY.
	 */
	want = USB_CTRL1_BUS;
	ret = regmap_update_bits(priv->misc, USB_CTRL1_REG, USB_CTRL1_MASK, want);
	if (ret)
		goto err_unprepare_ref;
	ret = regmap_read(priv->misc, USB_CTRL1_REG, &val);
	if (ret)
		goto err_unprepare_ref;
	val &= USB_CTRL1_MASK;
	if (val != want)
		dev_warn(priv->dev, "MISC_CTRL21 reads back 0x%08x in the access mode fields, programmed 0x%08x\n",
			 val, want);
	fsleep(USB_MODE_DELAY_US);

	/*
	 * Hold the controller, then the PHY in reset. The PHY test reset is
	 * asserted as well, so the release below starts from the reset state
	 * and not from what an earlier user left.
	 */
	ret = reset_control_bulk_assert(RST_PHY, rsts);
	if (ret)
		goto err_unprepare_ref;
	fsleep(USB_RESET_DELAY_US);
	ret = reset_control_bulk_assert(NUM_RESETS - RST_PHY, rsts + RST_PHY);
	if (ret)
		goto err_rst;
	fsleep(USB_RESET_DELAY_US);

	/* The strap and burst bits are read by the controller while it is in reset */
	want = USB_CTRL0_ACTIVE | priv->ctrl0;
	ret = regmap_update_bits(priv->misc, USB_CTRL0_REG, USB_CTRL0_MASK, want);
	if (ret)
		goto err_rst;
	ret = regmap_read(priv->misc, USB_CTRL0_REG, &val);
	if (ret)
		goto err_rst;
	val &= USB_CTRL0_MASK;
	if (val != want)
		dev_warn(priv->dev, "MISC_CTRL20 reads back 0x%08x in the control bits, programmed 0x%08x\n",
			 val, want);

	/* Reference clock, then the PHY and its test interface out of reset */
	ret = clk_enable(priv->ref_clk);
	if (ret)
		goto err_rst;
	fsleep(USB_REF_CLK_DELAY_US);

	ret = reset_control_deassert(rsts[RST_PHY].rstc);
	if (ret)
		goto err_ref;
	ret = reset_control_deassert(rsts[RST_TEST].rstc);
	if (ret)
		goto err_ref;
	fsleep(USB_PHY_RELEASE_DELAY_US);

	/* The PHY configuration word, then the two port interfaces out of reset */
	ret = hi3536d_usb2_write_checked(priv, PHY_CFG_REG, PHY_CFG_VAL);
	if (ret)
		goto err_ref;
	fsleep(USB_CFG_DELAY_US);

	ret = reset_control_deassert(rsts[RST_PORT0_TREQ].rstc);
	if (ret)
		goto err_ref;
	ret = reset_control_deassert(rsts[RST_PORT1_TREQ].rstc);
	if (ret)
		goto err_ref;
	fsleep(USB_PORT_RELEASE_DELAY_US);

	/* Controller clocks on and the controller out of reset */
	ret = clk_bulk_enable(ARRAY_SIZE(priv->gates), priv->gates);
	if (ret)
		goto err_ref;
	ret = reset_control_bulk_deassert(RST_PHY, rsts);
	if (ret)
		goto err_gates;
	fsleep(USB_CTRL_RELEASE_DELAY_US);

	/* Trim, then amplitude, pre-emphasis switch and strength and the disconnect threshold */
	ret = hi3536d_usb2_trim(priv);
	if (ret)
		goto err_gates;
	ret = hi3536d_usb2_tune(priv, PHY_PORT_RCOMP, PHY_RCOMP_VAL);
	if (ret)
		goto err_gates;
	ret = hi3536d_usb2_tune(priv, PHY_PORT_PRE_EMPHASIS_EN, PHY_PRE_EMPHASIS_EN_VAL);
	if (ret)
		goto err_gates;
	ret = hi3536d_usb2_tune(priv, PHY_PORT_PRE_EMPHASIS_STRENGTH,
				PHY_PRE_EMPHASIS_STRENGTH_VAL);
	if (ret)
		goto err_gates;
	ret = hi3536d_usb2_tune(priv, PHY_PORT_DISCONNECT, PHY_DISCONNECT_VAL);
	if (ret)
		goto err_gates;

	return 0;

err_gates:
	/*
	 * The order of exit(): the resets are asserted before the clocks stop
	 * (a controller whose clocks stop while it is out of reset can hang),
	 * the PHY resets first, then the controller resets, the reference
	 * clock and the controller clocks.
	 */
	reset_control_bulk_assert(RST_TEST - RST_PHY, rsts + RST_PHY);
	fsleep(USB_OFF_RESET_DELAY_US);
	reset_control_bulk_assert(RST_PHY, rsts);
	clk_disable(priv->ref_clk);
	fsleep(USB_OFF_REF_CLK_DELAY_US);
	clk_bulk_disable(ARRAY_SIZE(priv->gates), priv->gates);
	goto err_rst;
err_ref:
	/* The PHY is out of reset and gets its resets back before its clock stops */
	reset_control_bulk_assert(RST_TEST - RST_PHY, rsts + RST_PHY);
	fsleep(USB_OFF_RESET_DELAY_US);
	clk_disable(priv->ref_clk);
err_rst:
	reset_control_bulk_assert(NUM_RESETS, rsts);
err_unprepare_ref:
	clk_unprepare(priv->ref_clk);
err_unprepare:
	clk_bulk_unprepare(ARRAY_SIZE(priv->gates), priv->gates);

	return ret;
}

/*
 * The PHY core calls init() for the first user and exit() for the last one
 * (init_count of the shared struct phy: the EHCI and the OHCI use this one
 * PHY), so the controller resets are asserted and released once per start of
 * the PHY and not once per controller.
 *
 * System sleep is not supported. With CONFIG_SUSPEND the USB core calls
 * phy_exit() from the root hub suspend of a controller that cannot wake the
 * system, before ehci_suspend() and ohci_suspend() touch the registers, and
 * phy_init() when the root hub resumes, after the controller resume restored
 * them; both would hit a controller held in reset. Nothing on this SoC
 * sleeps (no suspend method, no wake source) and the controller side cannot
 * move out of init() and exit(): the strap bits of MISC_CTRL20 are latched
 * while the controller is in reset and the PHY must run before it is
 * released. A suspend method needs the controller and PHY teardown split.
 */
static int hi3536d_usb2_phy_exit(struct phy *phy)
{
	struct hi3536d_usb2_phy *priv = phy_get_drvdata(phy);
	struct reset_control_bulk_data *rsts = priv->rsts;
	int ret, ret2;

	/*
	 * The vendor stop sequence asserts the three PHY resets, stops the
	 * reference clock, then the controller clocks. The data sheet (clock
	 * gating) also asserts the controller resets before the clocks stop,
	 * which is done here. The test reset is left as it is (init() asserts
	 * it again before the next start).
	 */
	ret = reset_control_bulk_assert(RST_TEST - RST_PHY, rsts + RST_PHY);
	fsleep(USB_OFF_RESET_DELAY_US);
	ret2 = reset_control_bulk_assert(RST_PHY, rsts);

	clk_disable_unprepare(priv->ref_clk);
	fsleep(USB_OFF_REF_CLK_DELAY_US);

	clk_bulk_disable_unprepare(ARRAY_SIZE(priv->gates), priv->gates);
	fsleep(USB_OFF_CLK_DELAY_US);

	return ret ?: ret2;
}

static const struct phy_ops hi3536d_usb2_phy_ops = {
	.init	= hi3536d_usb2_phy_init,
	.exit	= hi3536d_usb2_phy_exit,
	.owner	= THIS_MODULE,
};

static int hi3536d_usb2_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *parent;
	struct phy_provider *provider;
	struct hi3536d_usb2_phy *priv;
	struct phy *phy;
	int i, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->dev = dev;

	/* The PHY node is a child of the MISC_CTRL (syscon) node */
	parent = of_get_parent(dev->of_node);
	priv->misc = syscon_node_to_regmap(parent);
	of_node_put(parent);
	if (IS_ERR(priv->misc))
		return dev_err_probe(dev, PTR_ERR(priv->misc),
				     "cannot get the MISC_CTRL regmap\n");

	priv->ref_clk = devm_clk_get(dev, "ref");
	if (IS_ERR(priv->ref_clk))
		return dev_err_probe(dev, PTR_ERR(priv->ref_clk),
				     "cannot get the reference clock\n");

	for (i = 0; i < ARRAY_SIZE(priv->gates); i++)
		priv->gates[i].id = hi3536d_usb2_gate_names[i];
	ret = devm_clk_bulk_get(dev, ARRAY_SIZE(priv->gates), priv->gates);
	if (ret)
		return dev_err_probe(dev, ret, "cannot get the controller clocks\n");

	for (i = 0; i < NUM_RESETS; i++)
		priv->rsts[i].id = hi3536d_usb2_reset_names[i];
	ret = devm_reset_control_bulk_get_exclusive(dev, NUM_RESETS, priv->rsts);
	if (ret)
		return dev_err_probe(dev, ret, "cannot get the resets\n");

	/* The two board dependent bits of MISC_CTRL20, default: the reset value */
	if (!of_property_read_bool(dev->of_node, "hisilicon,power-enable-active-low"))
		priv->ctrl0 |= USB_CTRL0_PWREN_POL;
	if (of_property_read_bool(dev->of_node, "hisilicon,shared-power-enable"))
		priv->ctrl0 |= USB_CTRL0_PWR_MERGE;

	phy = devm_phy_create(dev, NULL, &hi3536d_usb2_phy_ops);
	if (IS_ERR(phy))
		return dev_err_probe(dev, PTR_ERR(phy), "cannot create the PHY\n");

	phy_set_drvdata(phy, priv);
	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id hi3536d_usb2_phy_of_match[] = {
	{ .compatible = "hisilicon,hi3536dv100-usb2-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3536d_usb2_phy_of_match);

static struct platform_driver hi3536d_usb2_phy_driver = {
	.probe	= hi3536d_usb2_phy_probe,
	.driver = {
		.name = "hi3536d-usb2-phy",
		.of_match_table = hi3536d_usb2_phy_of_match,
		/* Unbinding would free the PHY under live EHCI/OHCI users */
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(hi3536d_usb2_phy_driver);

MODULE_DESCRIPTION("HiSilicon Hi3536D V100 USB 2.0 PHY driver");
MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_LICENSE("GPL");
