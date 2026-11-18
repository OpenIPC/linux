/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "dwmac-axera.h"
#include "stmmac.h"
#include "stmmac_platform.h"

static void emac_ephy_led0_init_pinmux(struct axera_eqos *eqos, u32 value)
{
	void __iomem *led0_addr;

	led0_addr = ioremap(EMAC_EPHY_LED0_PINMUX_ADDR, 4);
	if (!led0_addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_EPHY_LED0_PINMUX_ADDR);
		return;
	}

	writel(value, led0_addr);
	iounmap(led0_addr);
}

static void emac_ephy_led1_init_pinmux(struct axera_eqos *eqos, u32 value)
{
	void __iomem *led1_addr;

	led1_addr = ioremap(EMAC_EPHY_LED1_PINMUX_ADDR, 4);
	if (!led1_addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_EPHY_LED1_PINMUX_ADDR);
		return;
	}

	writel(value, led1_addr);
	iounmap(led1_addr);
}

static void emac_ephy_led_init_pinmux(struct axera_eqos *eqos, u32 value)
{
	if (eqos->led0_en) {
		emac_ephy_led0_init_pinmux(eqos, value);
	}

	if (eqos->led1_en) {
		emac_ephy_led1_init_pinmux(eqos, value);
	}
}

static void emac_ephy_led0_set_gpio(struct axera_eqos *eqos)
{
	int ret;

	emac_ephy_led0_init_pinmux(eqos, 0x00060003);
	ret = gpio_request(EPHY_LED0_GPIO_NUM, "EPHY_LED0_GPIO_NUM");
	if (ret) {
		dev_err(eqos->dev, "gpio request EPHY_LED0_GPIO_NUM\n");
		return;
	}
	gpio_direction_output(EPHY_LED0_GPIO_NUM, 0);
	//EPHY_LED_LOW_ACTIVE
	if (eqos->polarity) {
		gpio_set_value(EPHY_LED0_GPIO_NUM, 1);
	} else {
		gpio_set_value(EPHY_LED0_GPIO_NUM, 0);
	}
	gpio_free(EPHY_LED0_GPIO_NUM);
}

static void emac_ephy_led1_set_gpio(struct axera_eqos *eqos)
{
	int ret;

	emac_ephy_led1_init_pinmux(eqos, 0x00060003);
	ret = gpio_request(EPHY_LED1_GPIO_NUM, "EPHY_LED1_GPIO_NUM");
	if (ret) {
		dev_err(eqos->dev, "gpio request EPHY_LED1_GPIO_NUM\n");
		return;
	}
	gpio_direction_output(EPHY_LED1_GPIO_NUM, 0);
	//EPHY_LED_LOW_ACTIVE
	if (eqos->polarity) {
		gpio_set_value(EPHY_LED1_GPIO_NUM, 1);
	} else {
		gpio_set_value(EPHY_LED1_GPIO_NUM, 0);
	}
	gpio_free(EPHY_LED1_GPIO_NUM);
}

static void emac_ephy_led_set_gpio(struct axera_eqos *eqos)
{
	if (eqos->led0_en) {
		emac_ephy_led0_set_gpio(eqos);
	}

	if (eqos->led1_en) {
		emac_ephy_led1_set_gpio(eqos);
	}
}

static void emac_rgmii_set_tx_speed(int speed)
{
	u32 value;
	void __iomem *addr;
	u8 pos = EMAC_RGMII_TX_SEL;

	addr = ioremap(EMAC_FLASH_CLK_MUX_0_ADDR, 4);
	if (!addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_CLK_MUX_0_ADDR);
		return;
	}

	value = readl(addr);
	value &= ~(0x3 << pos);

	switch (speed) {
	case SPEED_10:
		value |= (0x0 << pos);
		break;
	case SPEED_100:
		value |= (0x1 << pos);
		break;
	case SPEED_1000:
		value |= (0x2 << pos);
		break;
	default:
		printk(KERN_ERR"EMAC:un-supported emac rgmii phy speed:%d\n", speed);
		iounmap(addr);
		return;
	}

	writel(value, addr);

	iounmap(addr);
}

static void emac_rmii_set_speed(int speed)
{
	u32 value;
	void __iomem *addr;
	u8 divn_pos = EMAC_RMII_RX_DIVN;
	u8 divn_update = EMAC_RMII_RX_DIVN_UP;

	//clk set
	addr = ioremap(EMAC_FLASH_SYS_GLB0_ADDR, 4);
	if (!addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_SYS_GLB0_ADDR);
		return;
	}

	value = readl(addr);
	value &= ~(0xF << divn_pos);

	switch (speed) {
	case SPEED_10:
		value |= (0xA << divn_pos);	//2.5M
		break;
	case SPEED_100:
		value |= (0x1 << divn_pos);	//25M
		break;
	default:
		printk(KERN_ERR"EMAC:un-supported emac rmii phy speed:%d\n", speed);
		iounmap(addr);
		return;
	}

	//clk update
	value |= (0x1 << divn_update);

	writel(value, addr);

	iounmap(addr);
}

void axera_eqos_fix_speed(void *priv, unsigned int speed)
{
	struct axera_eqos *eqos = priv;

	dev_dbg(eqos->dev, "fix axera emac speed: %dMhz\n", speed);

	if (eqos->phy_interface == PHY_INTERFACE_MODE_RGMII) {
		dev_dbg(eqos->dev, "adjust RGMII tx clock\n");
		emac_rgmii_set_tx_speed(speed);
	} else if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII) {
		dev_dbg(eqos->dev, "adjust RMII clock\n");
		emac_rmii_set_speed(speed);
	} else if (eqos->phy_interface == PHY_INTERFACE_MODE_GMII) {
		dev_dbg(eqos->dev, "fix GMII clock\n");
	} else {
		dev_err(eqos->dev, "unknown phy interface: %d\n", eqos->phy_interface);
	}
}
EXPORT_SYMBOL_GPL(axera_eqos_fix_speed);

void select_phy_interface(struct plat_stmmacenet_data *plat_dat)
{
	u32 value;
	void __iomem *addr;
	struct axera_eqos *eqos = plat_dat->bsp_priv;

	addr = ioremap(EMAC_FLASH_EMAC0_ADDR, 4);
	if (!addr) {
		dev_err(eqos->dev, "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_EMAC0_ADDR);
		return;
	}

	value = readl(addr);
	value &= (~(0x3<<EMAC_PHY_IF_SEL));

	if (plat_dat->interface == PHY_INTERFACE_MODE_RGMII) {
		value |= (0x1 << EMAC_PHY_IF_SEL) | (0x1 << EMAC_EXT_PAD_SEL);
		dev_info(eqos->dev,"EMAC: select RGMII interface\n");
	} else if (plat_dat->interface == PHY_INTERFACE_MODE_RMII) {
		if (eqos->out_rmii_mode) {
			value |= (0x0 << EMAC_PHY_IF_SEL) | (0x1 << EMAC_EXT_PAD_SEL);
			dev_info(eqos->dev,"EMAC: select Out RMII interface\n");
		} else {
			value |= (0x0 << EMAC_PHY_IF_SEL) | (0x0 << EMAC_EXT_PAD_SEL);
			dev_info(eqos->dev,"EMAC: select Inner RMII interface, value:0x%08x\n", value);
		}
	} else if (plat_dat->interface == PHY_INTERFACE_MODE_GMII) {
		value |= (0x1 << EMAC_PHY_IF_SEL) | (0x1 << EMAC_EXT_PAD_SEL);
		dev_info(eqos->dev,"EMAC: select GMII interface\n");
	} else {
		dev_err(eqos->dev,"EMAC: don't support this phy interface\n");
		iounmap(addr);
		return;
	}

	if (eqos->rmii_loopback_mode) {
		value |= (0x1 << EMAC_PHY_LOOPBACK_EN);
	}

	writel(value, addr);

	iounmap(addr);
}
EXPORT_SYMBOL_GPL(select_phy_interface);

#ifdef EMAC_SYS_REST_EN
void emac_sw_rst(struct axera_eqos *eqos)
{
	//1
	reset_control_assert(eqos->emac_rst);

	#ifdef CONFIG_DWMAC_AXERA_HAPS
		mdelay(50);										//delay 2
	#else
		mdelay(5);
	#endif

	//0
	reset_control_deassert(eqos->emac_rst);
}
EXPORT_SYMBOL_GPL(emac_sw_rst);
#else
void emac_sw_rst(struct axera_eqos *eqos)
{
	u32 value;
	void __iomem *addr;
	unsigned char emac_rst = EMAC_SW_RST;

    addr = ioremap(EMAC_FLASH_SW_RST_ADDR, 4);
	if (!addr) {
		dev_err(eqos->dev, "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_SW_RST_ADDR);
		return;
	}

	value = readl(addr);
	value |= (0x1 << emac_rst);
	writel(value, addr);

#ifdef CONFIG_DWMAC_AXERA_HAPS
	mdelay(50);										//delay 2
#else
	mdelay(5);
#endif

	value = readl(addr);
	value &= (~(0x1 << emac_rst));
	writel(value, addr);
	iounmap(addr);
}
EXPORT_SYMBOL_GPL(emac_sw_rst);
#endif

static void emac_ephy_set_bgs(struct axera_eqos *eqos)
{
	misc_info_t *misc_info;
	void __iomem *regs;
	void __iomem *addr;
	u32 value;

	regs = ioremap(MISC_INFO_ADDR, sizeof(misc_info_t));
	if (!regs) {
		dev_err(eqos->dev, "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)MISC_INFO_ADDR);
		return;
	}

	misc_info = (misc_info_t *) regs;

	addr = ioremap(EMAC_FLASH_EPHY_0_ADDR, 4);
	if (!addr) {
		dev_err(eqos->dev, "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_EPHY_0_ADDR);
		iounmap(regs);
		return;
	}

	value = readl(addr);
	value &= ~(0x3<<30); //ephy_led1_sel
	value &= ~(0x3<<28); //ephy_led0_sel
	value &= ~(0x1<<13); //ephy_led_pol
	value |= ((eqos->led0_mode<<28) | (eqos->led1_mode<<30) | (eqos->polarity<<13));

	//efuse2ephy_otp_bg;
	value &= (~(0xf<<EMAC_EFUSE_2_EPHY_OTP_BG));
	if ( misc_info->trim == 0x7 ) {
		value |= ((misc_info->bgs & 0xf)<<EMAC_EFUSE_2_EPHY_OTP_BG);
	} else {
		if (misc_info->bgs != 0xc &&
			misc_info->bgs != 0xd
		) {
			value |= ((misc_info->bgs & 0xf)<<EMAC_EFUSE_2_EPHY_OTP_BG);
		} else {
			dev_info(eqos->dev, "ephy bgs=0x%x, trim=0x%x, correct to bgs=0x0\n", misc_info->bgs, misc_info->trim);
		}
	}
	writel(value, addr);

	dev_info(eqos->dev, "ephy bgs=0x%x, trim=0x%x, ephy0_reg=0x%08x\n", misc_info->bgs, misc_info->trim, value);
	iounmap(regs);
	iounmap(addr);
}

#ifdef EMAC_SYS_REST_EN
static void emac_ephy_sw_rst(struct axera_eqos *eqos)
{
	//ephy_rst=1;
	reset_control_assert(eqos->ephy_rst);
	//ephy shutdown=0;
	reset_control_deassert(eqos->ephy_shutdown);

	mdelay(15);

	//ephy_rst=0
	reset_control_deassert(eqos->ephy_rst);

	udelay(12);
	return;
}
#else
static void emac_ephy_sw_rst(struct axera_eqos *eqos)
{
	u32 value_rst;
	u32 value_shutdown;
	void __iomem *addr_rst;
	void __iomem *addr_shutdown;

	addr_rst = ioremap(EMAC_FLASH_SW_RST_ADDR, 4);
	if (!addr_rst) {
		dev_err(eqos->dev, "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_SW_RST_ADDR);
		return;
	}

	addr_shutdown = ioremap(EMAC_FLASH_EPHY_0_ADDR, 4);
	if (!addr_shutdown) {
		dev_err(eqos->dev, "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_EPHY_0_ADDR);
		goto err;
	}

	//ephy_rst=1;
	value_rst = readl(addr_rst);
	value_rst |= (0x1 << EMAC_EPHY_SW_RST);
	writel(value_rst, addr_rst);

	//ephy shutdown=0;
	value_shutdown = readl(addr_shutdown);
	value_shutdown &= (~(0x1 << EMAC_EPHY_SHUTDOWN));
	writel(value_shutdown, addr_shutdown);

	mdelay(15);

	//ephy_rst=0
	value_rst = readl(addr_rst);
	value_rst &= (~(0x1 << EMAC_EPHY_SW_RST));
	writel(value_rst, addr_rst);

	udelay(15);

	iounmap(addr_shutdown);
err:
	iounmap(addr_rst);
}
#endif

#ifdef EMAC_SYS_REST_EN
int emac_get_reset_control(struct axera_eqos *eqos)
{
	eqos->emac_rst =  devm_reset_control_get_optional(eqos->dev, "emac_rst");
	if (IS_ERR(eqos->emac_rst)) {
		dev_err(eqos->dev, "can not get emac_rst!");
		return PTR_ERR(eqos->emac_rst);
	}

	eqos->ephy_rst =  devm_reset_control_get_optional(eqos->dev, "ephy_rst");
	if (IS_ERR(eqos->ephy_rst)) {
		dev_err(eqos->dev, "can not get ephy_rst!");
		return PTR_ERR(eqos->ephy_rst);
	}

	eqos->ephy_shutdown =  devm_reset_control_get_optional(eqos->dev, "ephy_shutdown");
	if (IS_ERR(eqos->ephy_shutdown)) {
		dev_err(eqos->dev, "can not get ephy_shutdown!");
		return PTR_ERR(eqos->ephy_shutdown);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(emac_get_reset_control);
#endif

static void emac_phy_gpio_rst(struct axera_eqos *eqos)
{
	if (!gpio_is_valid(eqos->gpiod_reset)) {
		dev_err(eqos->dev, "EMAC:no phy reset gpio found\n");
		return;
	}

	gpio_direction_output(eqos->gpiod_reset, 0);
	/* For a complete PHY reset, must be asserted low for at least 10ms. */
	gpio_direction_output(eqos->gpiod_reset, 0);
	msleep(15);
	gpio_direction_output(eqos->gpiod_reset, 1);
	msleep(75);
}

static int emac_phy_rst(struct axera_eqos *eqos)
{
	if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII &&
		!eqos->out_rmii_mode )
	{
		emac_ephy_led_init_pinmux(eqos, 0x3); //ephy led pinmux
		emac_ephy_set_bgs(eqos);
		emac_ephy_sw_rst(eqos);
	} else {
		emac_phy_gpio_rst(eqos);
	}

	return 0;
}

void emac_clk_init(struct axera_eqos *eqos)
{
	u32 value;
	void __iomem *addr;

    /* enable aclk & pclk */
	addr = ioremap(EMAC_FLASH_CLK_EB1_ADDR, 4);
	if (!addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_CLK_EB1_ADDR);
		return;
	}

	value = readl(addr);
	value |= (0x1 << EMAC_ACLK);
	writel(value, addr);
	iounmap(addr);

	/* enable rgmii tx clk */
	addr = ioremap(EMAC_FLASH_CLK_EB0_ADDR, 4);
	if (!addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_CLK_EB0_ADDR);
		return;
	}

	//RMII
	value = readl(addr);
	value |= (0x1 << EMAC_RMII_PHY_EB);
	writel(value, addr);

	//RGMII
	value = readl(addr);
	value |= (0x1 << EMAC_RGMII_TX_EB);
	writel(value, addr);

	//EPHY CLK
	value = readl(addr);
	value |= (0x1 << EMAC_EPHY_CLK_EB);
	writel(value, addr);

	//PTP
	value = readl(addr);
	value |= (0x1 << EMAC_PTP_REF_EB);
	writel(value, addr);
	iounmap(addr);

	/* enable rmii clock */
	addr = ioremap(EMAC_FLASH_SYS_GLB0_ADDR, 4);
	if (!addr) {
		printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
			(unsigned long long)EMAC_FLASH_SYS_GLB0_ADDR);
		return;
	}

	value = readl(addr);
	value |= (0x1 << EMAC_RMII_RX_DIV_EN);
	writel(value, addr);
	iounmap(addr);
}

int axera_dwmac_config_dt(
			struct platform_device *pdev,
			struct plat_stmmacenet_data *plat,
			struct axera_eqos *eqos
			)
{
	int rc;

	/* emac bus clock, max 312MHz */
	plat->stmmac_clk = devm_clk_get(&pdev->dev, "emac_aclk");
	if (IS_ERR(plat->stmmac_clk)) {
		dev_warn(&pdev->dev, "Cannot get CSR clock\n");
		plat->stmmac_clk = NULL;
	} else {
		eqos->bus_clock = clk_get_rate(plat->stmmac_clk);
		clk_prepare_enable(plat->stmmac_clk);
		dev_info(&pdev->dev, "emac bus clock: %ldMHz\n",
						 clk_get_rate(plat->stmmac_clk) / 1000000);
	}

	//ephy clock
	if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII &&
		!eqos->out_rmii_mode ) {
			;
	} else {
		eqos->ephy_clk = devm_clk_get(&pdev->dev, "ephy_clk");
		if (IS_ERR(eqos->ephy_clk)) {
			dev_warn(&pdev->dev, "can't get ephy_clk\n");
			eqos->ephy_clk = NULL;
		} else {
			if (clk_set_rate(eqos->ephy_clk, EPHY_CLK_25M)) {
				dev_warn(&pdev->dev, "emac phy clk set fail: %s\n",
					"ephy_clk");
			}

			rc = clk_prepare_enable(eqos->ephy_clk);
			if (rc)
				dev_warn(&pdev->dev, "enable ephy_clk failed\n");

			dev_info(&pdev->dev, "emac phy clock: %ldMHz\n",
			clk_get_rate(eqos->ephy_clk) / 1000000);
		}
	}

	/* rgmii tx clock */
	if (eqos->phy_interface == PHY_INTERFACE_MODE_RGMII) {
		eqos->rgmii_tx_clk = devm_clk_get(&pdev->dev, "rgmii_tx_clk");
		if (IS_ERR(eqos->rgmii_tx_clk)) {
			dev_warn(&pdev->dev, "Cannot set rgmii_tx_clk\n");
			eqos->rgmii_tx_clk = NULL;
		} else {
			rc = clk_prepare_enable(eqos->rgmii_tx_clk);
			if (rc == 0) {
				dev_dbg(&pdev->dev, "set rgmii_tx_clk ok\n");
			}
			dev_info(&pdev->dev, "emac rgmii tx clock: %ldMHz\n",
				clk_get_rate(eqos->rgmii_tx_clk) / 1000000);
		}
	}

	/*rmii phy clock */
	if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII) {
		//RMII CLK
		u32 value=0;
		void __iomem *addr;
		/* enable rmii clock */
		addr = ioremap(EMAC_FLASH_SYS_GLB0_ADDR, 4);
		if (!addr) {
			printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
				(unsigned long long)EMAC_FLASH_SYS_GLB0_ADDR);
			return -1;
		}

		value = readl(addr);
		value |= (0x1 << EMAC_RMII_RX_DIV_EN);
		writel(value, addr);
		iounmap(addr);

		/* phy clock, 50MHz not necessary*/
		if (eqos->out_rmii_mode) {
			eqos->rmii_phy_clk = devm_clk_get(&pdev->dev, "rmii_phy_clk");
			if (IS_ERR(eqos->rmii_phy_clk)) {
				dev_warn(&pdev->dev, "Cannot set phy-clk\n");
				eqos->rmii_phy_clk = NULL;
			} else {
				rc = clk_prepare_enable(eqos->rmii_phy_clk);
				if (rc == 0) {
					dev_dbg(&pdev->dev, "set rmii_phy_clk ok\n");
				}
			}
		}
	}

	/* ptp clock */
	plat->clk_ptp_ref = devm_clk_get(&pdev->dev, "ptp_clk");
	if (IS_ERR(plat->clk_ptp_ref)) {
		plat->clk_ptp_rate = clk_get_rate(plat->stmmac_clk);
		plat->clk_ptp_ref = NULL;
		dev_info(&pdev->dev, "PTP uses main clock %dMHz\n", plat->clk_ptp_rate/1000000);
	} else {
		plat->clk_ptp_rate = clk_get_rate(plat->clk_ptp_ref);
		dev_info(&pdev->dev, "PTP rate: %dMHz\n", plat->clk_ptp_rate/1000000);
	}
	// plat->ptp_max_adj = plat->clk_ptp_rate;

	return 0;
}
EXPORT_SYMBOL_GPL(axera_dwmac_config_dt);

/* for axera resume */
int axera_dwmac_resume_clk(
			struct platform_device *pdev,
			struct plat_stmmacenet_data *plat,
			struct axera_eqos *eqos
			)
{
	int rc;

	/* emac bus clock, max 312MHz */
	if (plat->stmmac_clk) {
		clk_prepare_enable(plat->stmmac_clk);
		dev_info(&pdev->dev, "emac bus clock: %ldMHz\n",
						 clk_get_rate(plat->stmmac_clk) / 1000000);
	}

	//ephy clock
	if (eqos->ephy_clk) {
		if (clk_set_rate(eqos->ephy_clk, EPHY_CLK_25M)) {
			dev_warn(&pdev->dev, "emac phy clk set fail: %s\n",
				 "ephy_clk");
		}

		rc = clk_prepare_enable(eqos->ephy_clk);
		if (rc)
			dev_warn(&pdev->dev, "enable ephy_clk failed\n");

		dev_info(&pdev->dev, "emac phy clock: %ldMHz\n",
		 clk_get_rate(eqos->ephy_clk) / 1000000);
	}

	/* rgmii tx clock */
	if (eqos->phy_interface == PHY_INTERFACE_MODE_RGMII) {
		if (eqos->rgmii_tx_clk) {
			rc = clk_prepare_enable(eqos->rgmii_tx_clk);
			if (rc == 0) {
				dev_dbg(&pdev->dev, "set rgmii_tx_clk ok\n");
			}
			dev_info(&pdev->dev, "emac rgmii tx clock: %ldMHz\n",
				clk_get_rate(eqos->rgmii_tx_clk) / 1000000);
		}
	}

	/*rmii phy clock */
	if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII) {
		//RMII CLK
		u32 value=0;
		void __iomem *addr;
		/* enable rmii clock */
		addr = ioremap(EMAC_FLASH_SYS_GLB0_ADDR, 4);
		if (!addr) {
			printk(KERN_ERR "EMAC: cannot map mmio space at 0x%llx\n",
				(unsigned long long)EMAC_FLASH_SYS_GLB0_ADDR);
			return -1;
		}

		value = readl(addr);
		value |= (0x1 << EMAC_RMII_RX_DIV_EN);
		writel(value, addr);
		iounmap(addr);

		if (eqos->rmii_phy_clk) {
			rc = clk_prepare_enable(eqos->rmii_phy_clk);
			if (rc == 0) {
				dev_dbg(&pdev->dev, "set rmii_phy_clk ok\n");
			}
		}
	}

	/* ptp clock */
	if (plat->clk_ptp_ref) {
		plat->clk_ptp_rate = clk_get_rate(plat->clk_ptp_ref);
		dev_info(&pdev->dev, "PTP rate: %dMHz\n", plat->clk_ptp_rate/1000000);
	}

	return 0;
}

/* for emac and phy reset */
void ax_reset_phy(struct platform_device *pdev, void *private)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct plat_stmmacenet_data *plat_dat = priv->plat;
	struct axera_eqos *eqos = plat_dat->bsp_priv;

	dev_info(&pdev->dev, "axera_reset_phy\n");
	emac_phy_rst(eqos);

	return;
}

void ax_reset_emac(struct platform_device *pdev, void *private)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct plat_stmmacenet_data *plat_dat = priv->plat;
	struct axera_eqos *eqos = plat_dat->bsp_priv;

	dev_info(&pdev->dev, "ax_reset_emac\n");

	emac_sw_rst(eqos);
	select_phy_interface(plat_dat);

	return;
}

void ax_shutdown_phy(struct platform_device *pdev, void *private)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct plat_stmmacenet_data *plat_dat = priv->plat;
	struct axera_eqos *eqos = plat_dat->bsp_priv;

	dev_info(&pdev->dev, "ax_shutdown_phy\n");

	if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII &&
		!eqos->out_rmii_mode )
	{
		//ephy shutdown=1;
		reset_control_assert(eqos->ephy_shutdown);
		emac_ephy_led_set_gpio(eqos);
	}

	return;
}

/* for axera resume */
int plat_init(struct platform_device *pdev, void *private)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct plat_stmmacenet_data *plat_dat = priv->plat;
	struct axera_eqos *eqos = plat_dat->bsp_priv;
	//int ret = 0;

	dev_info(&pdev->dev, "axera emac plat init!\n");

	axera_dwmac_resume_clk(pdev, plat_dat, plat_dat->bsp_priv);

	emac_phy_rst(eqos);

	return 0;
}
EXPORT_SYMBOL_GPL(plat_init);

/* for axera suspend */
void plat_exit(struct platform_device *pdev, void *private)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct plat_stmmacenet_data *plat_dat = priv->plat;
	struct axera_eqos *eqos = plat_dat->bsp_priv;

	dev_info(&pdev->dev, "axera emac plat exit!\n");

	if (eqos->phy_interface == PHY_INTERFACE_MODE_RMII &&
		!eqos->out_rmii_mode )
	{
		//ephy shutdown=1;
		reset_control_assert(eqos->ephy_shutdown);
	} else {
		gpio_direction_output(eqos->gpiod_reset, 0);
	}

	return;
}
EXPORT_SYMBOL_GPL(plat_exit);
