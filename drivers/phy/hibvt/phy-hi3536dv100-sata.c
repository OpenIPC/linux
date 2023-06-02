/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/io.h>
#include <mach/io.h>
#include <mach/platform.h>

#include "phy-hisi-sata.h"

enum {
	HISI_SATA_PERI_CTRL		= IO_ADDRESS(REG_CRG_BASE),
	HISI_SATA_PERI_CRG44	= (HISI_SATA_PERI_CTRL + 0xB0),
	HISI_SATA_PERI_CRG45	= (HISI_SATA_PERI_CTRL + 0xB4),
	HISI_SATAPHY_MISC_CTRL	= IO_ADDRESS(REG_MISC_CTRL_BASE),
	HISI_SATAPHY_MISC_CTRL22 = (HISI_SATAPHY_MISC_CTRL + 0x58),

	HISI_SATA_PHY0_CLK_EN	= (1 << 0),
	HISI_SATA_PHY0_RST		= (1 << 1),
	HISI_SATA_PHY0_REFCLK_SEL_MASK = (0x3 << 2),
	HISI_SATA_PHY0_REFCLK_SEL = (0x2 << 2),

	HISI_SATA_BUS_CKEN		= (1 << 0),
	HISI_SATA_RX0_CKEN		= (1 << 1),
	HISI_SATA_CKO_ALIVE_CKEN	= (1 << 2),
	HISI_SATA_TX0_CKEN		= (1 << 3),
	HISI_SATA_BUS_SRST_REQ	= (1 << 8),
	HISI_SATA_CKO_ALIVE_SRST_REQ	= (1 << 9),
	HISI_SATA_RX0_SRST_REQ	= (1 << 10),
	HISI_SATA0_SRST_REQ		= (1 << 11),

	FIFOTH_VALUE    = 0x7EED9F24,
	PHY_CONFIG_1_5G = 0x0e180000,
	PHY_CONFIG_3G   = 0x0e390000,

	PHY_SG_1_5G = 0x50438,
	PHY_SG_3G   = 0x50438,
};

static void hisi_sata_poweron(void)
{
}

static void hisi_sata_poweroff(void)
{
}

void hisi_sata_reset_rxtx_assert(unsigned int port_no)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG45);

	tmp_val |= HISI_SATA_RX0_SRST_REQ | HISI_SATA0_SRST_REQ;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG45);
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_assert);

void hisi_sata_reset_rxtx_deassert(unsigned int port_no)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG45);

	tmp_val &= ~(HISI_SATA_RX0_SRST_REQ | HISI_SATA0_SRST_REQ);

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG45);
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_deassert);

static void hisi_sata_reset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG45);

	tmp_val |= HISI_SATA_BUS_SRST_REQ | HISI_SATA_CKO_ALIVE_SRST_REQ
		| HISI_SATA_RX0_SRST_REQ | HISI_SATA0_SRST_REQ;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG45);
}

static void hisi_sata_unreset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG45);

	tmp_val &= ~(HISI_SATA_BUS_SRST_REQ | HISI_SATA_CKO_ALIVE_SRST_REQ
		| HISI_SATA_RX0_SRST_REQ | HISI_SATA0_SRST_REQ);

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG45);
}

static void hisi_sata_phy_reset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG44);
	tmp_val |= HISI_SATA_PHY0_RST;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG44);
}

static void hisi_sata_phy_unreset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG44);
	tmp_val &= ~HISI_SATA_PHY0_RST;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG44);
}

static void hisi_sata_clk_enable(void)
{
	unsigned int tmp_val, tmp_reg;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG44);
	tmp_reg = readl((void *)HISI_SATA_PERI_CRG45);
	tmp_val |= HISI_SATA_PHY0_CLK_EN;
	tmp_reg |= HISI_SATA_RX0_CKEN | HISI_SATA_TX0_CKEN;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG44);
	writel(tmp_reg, (void *)HISI_SATA_PERI_CRG45);

}

static void hisi_sata_clk_disable(void)
{
}

static void hisi_sata_clk_reset(void)
{
}

static void hisi_sata_phy_clk_sel(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG44);
	tmp_val &= ~HISI_SATA_PHY0_REFCLK_SEL_MASK;
	tmp_val |= HISI_SATA_PHY0_REFCLK_SEL;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG44);
}

void hisi_sata_set_fifoth(void *mmio)
{
	writel(FIFOTH_VALUE, (mmio + 0x100 + PORT_FIFOTH));
}
EXPORT_SYMBOL(hisi_sata_set_fifoth);

static void hisi_sata_phy_config(void *mmio, int phy_mode)
{
	unsigned int tmp_val, phy_config = PHY_CONFIG_3G;
	unsigned int phy_sg = PHY_SG_3G;

	hisi_sata_set_fifoth(mmio);

	tmp_val = readl(mmio + PHY_CTL1);
	tmp_val |= PHY_DATA_INVERT;
	writel(tmp_val, (mmio + PHY_CTL1));
	tmp_val = readl(mmio + PHY_CTL2);
	tmp_val |= PHY_DATA_INVERT;
	writel(tmp_val, (mmio + PHY_CTL2));

	tmp_val = readl(mmio + PHY_RST_BACK_MASK);
	tmp_val |= PHY_RST_MASK_ALL;
	tmp_val &= ~PHY0_RST_MASK;
	writel(tmp_val, (mmio + PHY_RST_BACK_MASK));

	if (phy_mode == PHY_MODE_1_5G) {
		phy_config = PHY_CONFIG_1_5G;
		phy_sg = PHY_SG_1_5G;
	}

	if (phy_mode == PHY_MODE_3G) {
		phy_config = PHY_CONFIG_3G;
		phy_sg = PHY_SG_3G;
	}

	writel(phy_config, (mmio + 0x100 + PORT_PHYCTL));
	writel(phy_sg, (mmio + 0x100 + PORT_PHYCTL1));

	/* force pll always work at 6Gbps, force cdr at 3Gbps
	 * for gen1 and at 6Gbps for gen2
	 */
	writel(0x70f, (void *)HISI_SATAPHY_MISC_CTRL22);
	writel(0x74f, (void *)HISI_SATAPHY_MISC_CTRL22);
	writel(0x70f, (void *)HISI_SATAPHY_MISC_CTRL22);
	writel(0x0, (void *)HISI_SATAPHY_MISC_CTRL22);

	/* disable SSC*/
	writel(0x802, (void *)HISI_SATAPHY_MISC_CTRL22);
	writel(0x842, (void *)HISI_SATAPHY_MISC_CTRL22);
	writel(0x802, (void *)HISI_SATAPHY_MISC_CTRL22);
	writel(0x0, (void *)HISI_SATAPHY_MISC_CTRL22);
}
