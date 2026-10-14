// SPDX-License-Identifier: GPL-2.0+

#include <linux/bitops.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/delay.h>

MODULE_DESCRIPTION("ephy PHY driver");
MODULE_AUTHOR("LOTUS");
MODULE_LICENSE("GPL");

#define MIIM_EPHY_SUB_CFG        0x12
#define MIIM_EPHY_SUB_WDATA      0x13
#define MIIM_EPHY_SUB_RDATA      0x14

#define EPHY_TRIM_REG_BASE      (0x12020000 + 0x8a2c)

typedef union { /* FEPHY TRIM */
	uint32_t all;
	struct {
		uint32_t efuse_tx_100_fs       :4;
		uint32_t efuse_tx_10_outr      :4;
		uint32_t efuse_tx_10_fs        :4;
		uint32_t efuse_rcal_code_rt    :4;
		uint32_t reserved             :16;
	} bitc;
} EPHY_TRIM_REG;

#define EPHY_RST_REG_BASE      (0x12010000 + 0x16C)

static int ephy_phy_reset(void)
{
	unsigned int reg;
	void __iomem *ephy_rst_reg_addr;

	ephy_rst_reg_addr = ioremap(EPHY_RST_REG_BASE, sizeof(uint32_t));
	if (!ephy_rst_reg_addr) {
		pr_err("EPHY_RST_REG_BASE ioremap fail.\n");
		return -1;
	}

	//set bit1 to 1 to rst
	reg = readl(ephy_rst_reg_addr);
	reg |= (0x1 << 1);
	writel(reg, ephy_rst_reg_addr);

	//delay 2ms
	msleep(2);

	//set bit1 to 0 to clear rst
	reg = readl(ephy_rst_reg_addr);
	reg &= ~(0x1 << 1);
	writel(reg, ephy_rst_reg_addr);

	//delay 100ms
	msleep(100);

	iounmap(ephy_rst_reg_addr);

	return 0;
}

static int ephy_phy_extread(struct phy_device *phydev, int addr,
				int devaddr, int regnum)
{
	int val;
	int sub_cfg;

	sub_cfg = (0<<15) | (devaddr << 8) | (regnum << 0);
	phy_write(phydev, MIIM_EPHY_SUB_CFG, sub_cfg);

	val = phy_read(phydev, MIIM_EPHY_SUB_RDATA);

	return val;
}

static int ephy_phy_extwrite(struct phy_device *phydev, int addr,
				 int devaddr, int regnum, u16 val)
{
	int sub_cfg;

	phy_write(phydev, MIIM_EPHY_SUB_WDATA, val);

	sub_cfg = (1<<15) | (devaddr << 8) | (regnum << 0);
	phy_write(phydev, MIIM_EPHY_SUB_CFG, sub_cfg);

	return 0;
}

static int ephy_config_init(struct phy_device *phydev)
{
	unsigned int reg;
	void __iomem *ephy_trim_reg_addr;
	EPHY_TRIM_REG ephy_trim_reg;

	/* Set green LED for Link, yellow LED for Active */
	//set LED to 0 (page 25 reg 0 3:2:0)
	reg = ephy_phy_extread(phydev, 0, 0x19, 0x00);
	reg &= ~(0x07);
	reg |= 0x03;
	ephy_phy_extwrite(phydev, 0, 0x19, 0x00, reg);

	/* Set RG_EPHY_TX_100_LPF fix 4 to Improve the waveform */
	//set RG_EPHY_TX_100_LPF(page 0 reg 33 7:4)
	reg = ephy_phy_extread(phydev, 0, 0x0, 0x21);
	reg &= ~(0xf0);
	reg |= 0x40;
	ephy_phy_extwrite(phydev, 0, 0x0, 0x21, reg);

	ephy_trim_reg_addr = ioremap(EPHY_TRIM_REG_BASE, sizeof(uint32_t));
	if (!ephy_trim_reg_addr) {
		pr_err("EPHY_TRIM_REG_BASE ioremap fail.\n");
		return -1;
	}

	ephy_trim_reg.all = readl(ephy_trim_reg_addr);

	iounmap(ephy_trim_reg_addr);

	//pr_info("trim_reg:0x%x\n", ephy_trim_reg.all);
	//pr_info("tx_100_fs:0x%x tx_10_outr:0x%x\n", ephy_trim_reg.bitc.efuse_tx_100_fs, ephy_trim_reg.bitc.efuse_tx_10_outr);
	//pr_info("tx_10_fs:0x%x rcal_code:0x%x\n", ephy_trim_reg.bitc.efuse_tx_10_fs, ephy_trim_reg.bitc.efuse_rcal_code_rt);

	if (ephy_trim_reg.all) {
		//set EPHY_TX_10_OUTR to 0 (page 0 reg 9 11:8)
		reg = ephy_phy_extread(phydev, 0, 0, 0x09);
		reg &= ~(0xf << 8);
		reg |= (ephy_trim_reg.bitc.efuse_tx_10_outr << 8);
		ephy_phy_extwrite(phydev, 0, 0, 0x09, reg);

		//set EPHY_TX_10_FS to 0 (page 0 reg 9 7:4)
		reg = ephy_phy_extread(phydev, 0, 0, 0x09);
		reg &= ~(0xf << 4);
		reg |= (ephy_trim_reg.bitc.efuse_tx_10_fs << 4);
		ephy_phy_extwrite(phydev, 0, 0, 0x09, reg);

		//set EPHY_TX_100_FS to 4 (page 0 reg 33 3:0)
		reg = ephy_phy_extread(phydev, 0, 0, 0x21);
		reg &= ~(0xf << 0);
		reg |= (ephy_trim_reg.bitc.efuse_tx_100_fs << 0);
		ephy_phy_extwrite(phydev, 0, 0, 0x21, reg);

		// set EPHY_RCAL_CODE_FT to 8 (page 0 reg 34 7:4)
		reg = ephy_phy_extread(phydev, 0, 0, 0x22);
		reg &= ~(0xf << 4);
		reg |= (ephy_trim_reg.bitc.efuse_rcal_code_rt << 4);
		ephy_phy_extwrite(phydev, 0, 0, 0x22, reg);
	}

	return 0;
}

static int ephy_suspend(struct phy_device *phydev)
{
	unsigned int reg;

	// set RG_EPHY_SLEEP to 1 (page 0 reg 9 13:13)
	reg = ephy_phy_extread(phydev, 0, 0, 0x9);
	reg |= (0x1 << 13);
	ephy_phy_extwrite(phydev, 0, 0, 0x9, reg);

	return genphy_suspend(phydev);
}

static int ephy_resume(struct phy_device *phydev)
{
	unsigned int reg;

	// set RG_EPHY_SLEEP to 0 (page 0 reg 9 13:13)
	reg = ephy_phy_extread(phydev, 0, 0, 0x9);
	reg &= ~(0x1 << 13);
	ephy_phy_extwrite(phydev, 0, 0, 0x9, reg);

	ephy_phy_reset();

	ephy_config_init(phydev);

	return genphy_resume(phydev);
}

static struct phy_driver ephy_driver[] = {
	{
		.phy_id		= 0x0243991f,
		.name		= "ePHY",
		.phy_id_mask	= 0xffffffff,
		/* PHY_BASIC_FEATURES */
		.config_init	= &ephy_config_init,
		.suspend	= ephy_suspend,
		.resume		= ephy_resume,
	},
};

module_phy_driver(ephy_driver);

static struct mdio_device_id __maybe_unused ephy_tbl[] = {
	{ 0x0243991f, 0xffffffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, ephy_tbl);

