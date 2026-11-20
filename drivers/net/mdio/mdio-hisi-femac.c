/*
 * Hisilicon Fast Ethernet MDIO Bus Driver
 *
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_mdio.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/reset.h>

#define MDIO_RWCTRL		0x00
#define MDIO_RO_DATA		0x04
#define MDIO_WRITE		BIT(13)
#define MDIO_RW_FINISH		BIT(15)
#define BIT_PHY_ADDR_OFFSET	8
#define BIT_WR_DATA_OFFSET	16

#define BIT_MASK_FEPHY_ADDR	GENMASK(4, 0)
#define BIT_FEPHY_SEL		BIT(5)

#define MII_EXPMD		0x1d
#define MII_EXPMA		0x1e

#define REG_LD_AM		0x3050
#define BIT_MASK_LD_SET		GENMASK(4, 0)
#define REG_LDO_AM		0x3051
#define BIT_MASK_LDO_SET	GENMASK(2, 0)
#define REG_R_TUNING		0x3052
#define BIT_MASK_R_TUNING	GENMASK(5, 0)
#define REG_WR_DONE		0x3053
#define BIT_CFG_DONE		BIT(0)
#define BIT_CFG_ACK		BIT(1)
#define REG_DEF_ATE		0x3057
#define BIT_AUTOTRIM_DONE	BIT(0)

#define PHY_RESET_DELAYS_PROPERTY	"hisilicon,phy-reset-delays-us"

enum phy_reset_delays {
	PRE_DELAY,
	PULSE,
	POST_DELAY,
	DELAYS_NUM,
};

/*
 * Where the PHY trim values are kept. They are in the PHY control register
 * (the second register window) unless the device tree gives a separate trim
 * window as a third one, which uses another bit layout.
 */
struct hisi_femac_trim_layout {
	u8 ld_set;
	u8 ldo_set;
	u8 r_tuning;
};

static const struct hisi_femac_trim_layout hisi_femac_trim_in_ctrl = {
	.ld_set = 25, .ldo_set = 22, .r_tuning = 16,
};

static const struct hisi_femac_trim_layout hisi_femac_trim_window = {
	.ld_set = 0, .ldo_set = 5, .r_tuning = 8,
};

/* Per-compatible behavior that the hardware does not tell by itself */
struct hisi_femac_mdio_variant {
	/*
	 * The internal PHY is not a festa PHY: it has no trim data and only
	 * needs its address in the control register.
	 */
	bool fephy_addr_only;
};

static const struct hisi_femac_mdio_variant hisi_femac_mdio_addr_only = {
	.fephy_addr_only = true,
};

struct hisi_femac_mdio_data {
	struct clk *clk;
	struct clk *fephy_clk;
	struct reset_control *phy_rst;
	struct reset_control *fephy_rst;
	u32 phy_reset_delays[DELAYS_NUM];
	void __iomem *membase;
	void __iomem *fephy_iobase;
	void __iomem *fephy_trim_iobase;
	struct mii_bus *bus;
	u32 phy_addr;
	bool fephy_addr_only;
	bool fephy_clk_on;
	bool clk_on;
};

static int hisi_femac_mdio_wait_ready(struct hisi_femac_mdio_data *data)
{
	u32 val;

	return readl_poll_timeout(data->membase + MDIO_RWCTRL, val,
				  val & MDIO_RW_FINISH, 20, 10000);
}

static int hisi_femac_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	struct hisi_femac_mdio_data *data = bus->priv;
	int ret;

	ret = hisi_femac_mdio_wait_ready(data);
	if (ret)
		return ret;

	writel(((u32)mii_id << BIT_PHY_ADDR_OFFSET) | ((u32)regnum),
		  data->membase + MDIO_RWCTRL);

	ret = hisi_femac_mdio_wait_ready(data);
	if (ret)
		return ret;

	return readl(data->membase + MDIO_RO_DATA) & 0xFFFF;
}

static int hisi_femac_mdio_write(struct mii_bus *bus, int mii_id, int regnum,
				 u16 value)
{
	struct hisi_femac_mdio_data *data = bus->priv;
	int ret;

	ret = hisi_femac_mdio_wait_ready(data);
	if (ret)
		return ret;

	writel(MDIO_WRITE | ((u32)value << BIT_WR_DATA_OFFSET) |
	       ((u32)mii_id << BIT_PHY_ADDR_OFFSET) | ((u32)regnum),
	       data->membase + MDIO_RWCTRL);

	return hisi_femac_mdio_wait_ready(data);
}

static void hisi_femac_sleep_us(u32 time_us)
{
	u32 time_ms;

	if (!time_us)
		return;

	time_ms = DIV_ROUND_UP(time_us, 1000);
	if (time_ms < 20)
		usleep_range(time_us, time_us + 500);
	else
		msleep(time_ms);
}

static void hisi_femac_phy_reset(struct hisi_femac_mdio_data *data)
{
	/* To make sure PHY hardware reset success,
	 * we must keep PHY in deassert state first and
	 * then complete the hardware reset operation
	 */
	reset_control_deassert(data->phy_rst);
	hisi_femac_sleep_us(data->phy_reset_delays[PRE_DELAY]);

	reset_control_assert(data->phy_rst);
	/* delay some time to ensure reset ok,
	 * this depends on PHY hardware feature
	 */
	hisi_femac_sleep_us(data->phy_reset_delays[PULSE]);
	reset_control_deassert(data->phy_rst);
	/* delay some time to ensure later MDIO access */
	hisi_femac_sleep_us(data->phy_reset_delays[POST_DELAY]);
}

static void hisi_femac_get_phy_addr(struct hisi_femac_mdio_data *data,
				    struct device_node *np)
{
	struct device_node *child = NULL;
	int addr;

	child = of_get_next_available_child(np, NULL);
	if (!child) {
		dev_err(data->bus->parent, "No valid PHY device node!\n");
		return;
	}

	addr = of_mdio_parse_addr(data->bus->parent, child);
	of_node_put(child);
	if (addr < 0) {
		dev_err(data->bus->parent, "get PHY address failed!\n");
		return;
	}

	data->phy_addr = addr;
}

static inline bool hisi_femac_use_fephy(struct hisi_femac_mdio_data *data)
{
	/*return false;*/
	return (data->fephy_iobase ?
			!(readl(data->fephy_iobase) & BIT_FEPHY_SEL) : false);
}

static int hisi_femac_fephy_reset(struct hisi_femac_mdio_data *data)
{
	u32 val;
	int ret;

	/* disable MDCK clock to make sure FEPHY reset success */
	clk_disable_unprepare(data->clk);
	data->clk_on = false;

	val = readl(data->fephy_iobase);
	val &= ~BIT_MASK_FEPHY_ADDR;
	val |= data->phy_addr;
	writel(val, data->fephy_iobase);

	ret = clk_prepare_enable(data->fephy_clk);
	if (ret)
		return ret;
	data->fephy_clk_on = !!data->fephy_clk;
	udelay(10);

	reset_control_assert(data->fephy_rst);
	udelay(10);
	reset_control_deassert(data->fephy_rst);
	/* delay at least 15ms for MDIO operation */
	msleep(20);

	ret = clk_prepare_enable(data->clk);
	if (ret)
		return ret;
	data->clk_on = true;
	/* delay 5ms after enable MDCK to make sure FEPHY trim safe */
	mdelay(5);

	return 0;
}

static inline int fephy_expanded_read(struct mii_bus *bus, int phy_addr,
				      u32 reg_addr)
{
	int ret;

	hisi_femac_mdio_write(bus, phy_addr, MII_EXPMA, reg_addr);
	ret = hisi_femac_mdio_read(bus, phy_addr, MII_EXPMD);

	return ret;
}

static inline int fephy_expanded_write(struct mii_bus *bus, int phy_addr,
				       u32 reg_addr, u16 val)
{
	int ret;

	hisi_femac_mdio_write(bus, phy_addr, MII_EXPMA, reg_addr);
	ret = hisi_femac_mdio_write(bus, phy_addr, MII_EXPMD, val);

	return ret;
}

static void hisi_femac_fephy_use_default_trim(struct hisi_femac_mdio_data *data)
{
	unsigned short val;
	int timeout = 3;

	dev_dbg(data->bus->parent, "No OTP data, festa PHY uses default ATE parameters\n");

	do {
		msleep(250);
		val = fephy_expanded_read(data->bus, data->phy_addr,
					  REG_DEF_ATE);
		val &= BIT_AUTOTRIM_DONE;
	} while (!val && --timeout);

	if (!timeout)
		dev_err(data->bus->parent, "festa PHY wait autotrim done timeout!\n");

	mdelay(5);
}

static void hisi_femac_fephy_trim(struct hisi_femac_mdio_data *data)
{
	const struct hisi_femac_trim_layout *layout;
	struct mii_bus *bus = data->bus;
	u32 phy_addr = data->phy_addr;
	int timeout = 3000;
	u32 val;
	u8 ld_set;
	u8 ldo_set;
	u8 r_tuning;

	if (data->fephy_trim_iobase) {
		layout = &hisi_femac_trim_window;
		val = readl(data->fephy_trim_iobase);
	} else {
		layout = &hisi_femac_trim_in_ctrl;
		val = readl(data->fephy_iobase);
	}
	ld_set = (val >> layout->ld_set) & BIT_MASK_LD_SET;
	ldo_set = (val >> layout->ldo_set) & BIT_MASK_LDO_SET;
	r_tuning = (val >> layout->r_tuning) & BIT_MASK_R_TUNING;

	if (!ld_set && !ldo_set && !r_tuning) {
		hisi_femac_fephy_use_default_trim(data);
		return;
	}

	val = fephy_expanded_read(bus, phy_addr, REG_LD_AM);
	val = (val & ~BIT_MASK_LD_SET) | (ld_set & BIT_MASK_LD_SET);
	fephy_expanded_write(bus, phy_addr, REG_LD_AM, val);

	val = fephy_expanded_read(bus, phy_addr, REG_LDO_AM);
	val = (val & ~BIT_MASK_LDO_SET) | (ldo_set & BIT_MASK_LDO_SET);
	fephy_expanded_write(bus, phy_addr, REG_LDO_AM, val);

	val = fephy_expanded_read(bus, phy_addr, REG_R_TUNING);
	val = (val & ~BIT_MASK_R_TUNING) | (r_tuning & BIT_MASK_R_TUNING);
	fephy_expanded_write(bus, phy_addr, REG_R_TUNING, val);

	val = fephy_expanded_read(bus, phy_addr, REG_WR_DONE);
	if (val & BIT_CFG_ACK)
		dev_err(data->bus->parent, "festa PHY 0x3053 bit CFG_ACK value: 1\n");
	val = val | BIT_CFG_DONE;
	fephy_expanded_write(bus, phy_addr, REG_WR_DONE, val);

	do {
		usleep_range(100, 150);
		val = fephy_expanded_read(bus, phy_addr, REG_WR_DONE);
		val &= BIT_CFG_ACK;
	} while (!val && --timeout);
	if (!timeout)
		dev_err(data->bus->parent, "festa PHY 0x3053 wait bit CFG_ACK timeout!\n");

	mdelay(5);

	dev_dbg(data->bus->parent, "FEPHY:addr=%d, la_am=0x%x, ldo_am=0x%x, r_tuning=0x%x\n",
		phy_addr,
		fephy_expanded_read(bus, phy_addr, REG_LD_AM),
		fephy_expanded_read(bus, phy_addr, REG_LDO_AM),
		fephy_expanded_read(bus, phy_addr, REG_R_TUNING));
}

static int hisi_femac_fephy_reset_and_trim(struct hisi_femac_mdio_data *data)
{
	int ret;

	ret = hisi_femac_fephy_reset(data);
	if (ret)
		return ret;
	hisi_femac_fephy_trim(data);

	return 0;
}

/*
 * The Hi3520DV200 internal PHY answers at the address in bits 4:0 of its
 * control register, which is 0 after power-on. Only the bootloader's network
 * code sets it, so a kernel booted without it finds no PHY. This SoC has no
 * festa trim data, and its PHY is not a festa, so only the address is set.
 */
static void hisi_femac_fephy_write_addr(struct hisi_femac_mdio_data *data)
{
	u32 val;

	val = readl(data->fephy_iobase);
	val &= ~BIT_MASK_FEPHY_ADDR;
	val |= data->phy_addr;
	writel(val, data->fephy_iobase);
}

static void hisi_femac_fephy_set_phy_addr(struct hisi_femac_mdio_data *data)
{
	u32 val;

	if (!data->fephy_iobase)
		return;

	val = readl(data->fephy_iobase);
	val &= ~BIT_MASK_FEPHY_ADDR;
	val |= (data->phy_addr + 1);
	writel(val, data->fephy_iobase);
}

static int hisi_femac_mdio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mii_bus *bus;
	const struct hisi_femac_mdio_variant *variant;
	struct hisi_femac_mdio_data *data;
	struct resource *res;
	int ret;

	bus = devm_mdiobus_alloc_size(&pdev->dev, sizeof(*data));
	if (!bus)
		return -ENOMEM;

	bus->name = "hisi_femac_mii_bus";
	bus->read = &hisi_femac_mdio_read;
	bus->write = &hisi_femac_mdio_write;
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s", pdev->name);
	bus->parent = &pdev->dev;

	data = bus->priv;
	data->bus = bus;
	data->membase = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(data->membase))
		return PTR_ERR(data->membase);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		data->fephy_iobase = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(data->fephy_iobase))
			return PTR_ERR(data->fephy_iobase);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (res) {
		data->fephy_trim_iobase = devm_ioremap_resource(&pdev->dev,
								res);
		if (IS_ERR(data->fephy_trim_iobase))
			return PTR_ERR(data->fephy_trim_iobase);
	}

	data->clk = devm_clk_get(&pdev->dev, "mdio");
	if (IS_ERR(data->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(data->clk),
				     "failed to get the MDIO clock\n");

	data->fephy_clk = devm_clk_get_optional(&pdev->dev, "phy");
	if (IS_ERR(data->fephy_clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(data->fephy_clk),
				     "failed to get the PHY clock\n");

	ret = clk_prepare_enable(data->clk);
	if (ret)
		return ret;
	data->clk_on = true;

	data->phy_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								  "external-phy");
	if (IS_ERR(data->phy_rst)) {
		ret = dev_err_probe(&pdev->dev, PTR_ERR(data->phy_rst),
				    "failed to get the external PHY reset\n");
		goto err_out_disable_clk;
	}
	if (data->phy_rst) {
		ret = of_property_read_u32_array(np,
						 PHY_RESET_DELAYS_PROPERTY,
						 data->phy_reset_delays,
						 DELAYS_NUM);
		if (ret)
			goto err_out_disable_clk;
		dev_dbg(&pdev->dev, "external PHY reset with delays %u/%u/%u us\n",
			data->phy_reset_delays[0], data->phy_reset_delays[1],
			data->phy_reset_delays[2]);
		hisi_femac_phy_reset(data);
	}

	data->fephy_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								    "internal-phy");
	if (IS_ERR(data->fephy_rst)) {
		ret = dev_err_probe(&pdev->dev, PTR_ERR(data->fephy_rst),
				    "failed to get the internal PHY reset\n");
		goto err_out_disable_clk;
	}

	hisi_femac_get_phy_addr(data, np);
	variant = device_get_match_data(&pdev->dev);
	data->fephy_addr_only = data->fephy_iobase && variant &&
				variant->fephy_addr_only;
	dev_dbg(&pdev->dev, "fephy_iobase=%p fephy_rst=%p fephy_clk=%p use_fephy=%d phy_addr=%d\n",
		data->fephy_iobase, data->fephy_rst, data->fephy_clk,
		hisi_femac_use_fephy(data), data->phy_addr);

	ret = 0;
	if (data->fephy_addr_only)
		hisi_femac_fephy_write_addr(data);
	else if (hisi_femac_use_fephy(data))
		ret = hisi_femac_fephy_reset_and_trim(data);
	else
		hisi_femac_fephy_set_phy_addr(data);
	if (ret)
		goto err_out_disable_clk;

	ret = of_mdiobus_register(bus, np);
	if (ret)
		goto err_out_disable_clk;

	platform_set_drvdata(pdev, bus);

	return 0;

err_out_disable_clk:
	if (data->fephy_clk_on)
		clk_disable_unprepare(data->fephy_clk);
	if (data->clk_on)
		clk_disable_unprepare(data->clk);
	return ret;
}

static void hisi_femac_mdio_remove(struct platform_device *pdev)
{
	struct mii_bus *bus = platform_get_drvdata(pdev);
	struct hisi_femac_mdio_data *data = bus->priv;

	mdiobus_unregister(bus);
	if (data->fephy_clk_on)
		clk_disable_unprepare(data->fephy_clk);
	if (data->clk_on)
		clk_disable_unprepare(data->clk);
}

static const struct of_device_id hisi_femac_mdio_dt_ids[] = {
	{ .compatible = "hisilicon,hi3520dv100-femac-mdio", .data = &hisi_femac_mdio_addr_only },
	{ .compatible = "hisilicon,hi3520dv200-femac-mdio", .data = &hisi_femac_mdio_addr_only },
	{ .compatible = "hisilicon,hisi-femac-mdio" },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_femac_mdio_dt_ids);

static struct platform_driver hisi_femac_mdio_driver = {
	.probe = hisi_femac_mdio_probe,
	.remove = hisi_femac_mdio_remove,
	.driver = {
		.name = "hisi-femac-mdio",
		.of_match_table = hisi_femac_mdio_dt_ids,
	},
};

module_platform_driver(hisi_femac_mdio_driver);

MODULE_DESCRIPTION("Hisilicon Fast Ethernet MAC MDIO interface driver");
MODULE_AUTHOR("Dongpo Li <lidongpo@hisilicon.com>");
MODULE_LICENSE("GPL v2");
