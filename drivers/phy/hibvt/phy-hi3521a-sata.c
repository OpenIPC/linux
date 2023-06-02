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

#include <linux/bitops.h>
#include <linux/io.h>
#include <mach/io.h>
#include <mach/platform.h>

#include "phy-hisi-sata.h"

enum {
	HISI_SATA_PERI_CTRL			= IO_ADDRESS(REG_CRG_BASE),
	HISI_SATA_PERI_CRG26		= (HISI_SATA_PERI_CTRL + 0x68),
	HISI_PHY1_REFCLK_SEL_24M	= BIT(3),
	HISI_PHY1_REFCLK_SEL_100M	= (0 << 3),
	HISI_PHY1_REFCKEN			= BIT(2),
	HISI_PHY0_REFCLK_SEL_24M	= BIT(1),
	HISI_PHY0_REFCLK_SEL_100M	= (0 << 1),
	HISI_PHY0_REFCKEN			= BIT(0),

	HISI_SATA_PERI_CRG27	= (HISI_SATA_PERI_CTRL + 0x6C),
	HISI_SATA_PORT1_REFCLK_CKEN	= BIT(17),
	HISI_SATA_PORT1_MPLL_CKEN   = BIT(16),
	HISI_SATA_CKO_ALIVE_SRST_REQ	= BIT(13),
	HISI_SATA_BUS_SRST_REQ		= BIT(12),
	HISI_SATA_PORT0_REFCLK_CKEN	= BIT(10),
	HISI_SATA_PORT0_MPLL_CKEN	= BIT(9),
	HISI_SATA_CKO_ALIVE_CKEN    = BIT(8),
	HISI_SATA_RX1_CKEN			= BIT(6),
	HISI_SATA_RX0_CKEN			= BIT(5),
	HISI_SATA_BUS_CKEN			= BIT(4),
	HISI_SATA_PORT01_CLK_EN     = HISI_SATA_BUS_CKEN
								| HISI_SATA_RX0_CKEN
								| HISI_SATA_RX1_CKEN
								| HISI_SATA_CKO_ALIVE_CKEN
								| HISI_SATA_PORT0_MPLL_CKEN
								| HISI_SATA_PORT0_REFCLK_CKEN
								| HISI_SATA_PORT1_MPLL_CKEN
								| HISI_SATA_PORT1_REFCLK_CKEN,

	FIFOTH_VALUE    = 0x66d9f24,
	PHY_VALUE       = 0x4900003d,
	PHYCTL2_VALUE   = 0x60555,

	PORT_BIGENDINE  = 0x82e5cb8,

	PX_TX_AMPLITUDE = 0x8377eb8,
	PX_TX_PREEMPH	= 0x14451,

	PHY_SG_1_5G = 0x0e180000,
	PHY_SG_3G   = 0x0e390000,
	PHY_SG_6G	= 0x0e5a0000,

	PHY_FORCE_1_5G	= 0x2f180000,
	PHY_FORCE_3G	= 0x2f390000,
	PHY_FORCE_6G	= 0x2f5a0000,
};

static void hisi_sata_poweron(void)
{
}

static void hisi_sata_poweroff(void)
{
}

void hisi_sata_reset_rxtx_assert(unsigned int port_no)
{
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_assert);

void hisi_sata_reset_rxtx_deassert(unsigned int port_no)
{
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_deassert);

static void hisi_sata_reset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG27);
	tmp_val |= HISI_SATA_BUS_SRST_REQ | HISI_SATA_CKO_ALIVE_SRST_REQ;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG27);
}

static void hisi_sata_unreset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG27);
	tmp_val &= ~(HISI_SATA_BUS_SRST_REQ | HISI_SATA_CKO_ALIVE_SRST_REQ);
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG27);
}

static void hisi_sata_phy_reset(void)
{
}

static void hisi_sata_phy_unreset(void)
{
}

static void hisi_sata_clk_enable(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG27);
	tmp_val |= HISI_SATA_PORT01_CLK_EN;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG27);
}

static void hisi_sata_clk_disable(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG27);
	tmp_val &= ~(HISI_SATA_PORT01_CLK_EN);
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG27);
}

static void hisi_sata_clk_reset(void)
{
}

static void hisi_sata_phy_clk_sel(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG26);
	tmp_val |= HISI_PHY1_REFCLK_SEL_100M
			| HISI_PHY0_REFCLK_SEL_100M
			| HISI_PHY1_REFCKEN
			| HISI_PHY0_REFCKEN;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG26);
}

void hisi_sata_set_fifoth(void *mmio)
{
	int i;

	for (i = 0; i < ports_num; i++)
		writel(FIFOTH_VALUE, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_FIFOTH));
}
EXPORT_SYMBOL(hisi_sata_set_fifoth);

static void hisi_sata_phy_config(void *mmio, int phy_mode)
{
	unsigned int i, tmp_val, phy_config;

	hisi_sata_set_fifoth(mmio);

	if ((ports_num < 1) || (ports_num > 2))
		pr_err("ERROR: PORT num you set is WRONG!!!\n");

	writel(PHY_VALUE, (mmio + HISI_SATA_PHY0_CTLL));
	writel(PHY_VALUE, (mmio + HISI_SATA_PHY1_CTLL));

	for (i = 0; i < ports_num; i++)
		writel(PHYCTL2_VALUE, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL2));

	tmp_val = readl(mmio + HISI_SATA_PHY0_CTLL);
	tmp_val |= HISI_SATA_PHY_REV_CLK;
	writel(tmp_val, (mmio + HISI_SATA_PHY0_CTLL));
	tmp_val = readl(mmio + HISI_SATA_PHY1_CTLL);
	tmp_val |= HISI_SATA_PHY_REV_CLK;
	writel(tmp_val, (mmio + HISI_SATA_PHY1_CTLL));

	for (i = 0; i < ports_num; i++) {
		tmp_val = readl(mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL2);
		tmp_val &= ~HISI_SATA_LANE0_RESET;
		writel(tmp_val, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL2));
	}

	tmp_val = readl(mmio + HISI_SATA_PHY0_CTLL);
	tmp_val &= ~HISI_SATA_PHY_RESET;
	writel(tmp_val, (mmio + HISI_SATA_PHY0_CTLL));
	tmp_val = readl(mmio + HISI_SATA_PHY1_CTLL);
	tmp_val &= ~HISI_SATA_PHY_RESET;
	writel(tmp_val, (mmio + HISI_SATA_PHY1_CTLL));

	tmp_val = readl(mmio + HISI_SATA_PHY0_CTLH);
	tmp_val |= HISI_SATA_BIGENDINE;
	writel(tmp_val, (mmio + HISI_SATA_PHY0_CTLH));
	tmp_val = readl(mmio + HISI_SATA_PHY1_CTLH);
	tmp_val |= HISI_SATA_BIGENDINE;
	writel(tmp_val, (mmio + HISI_SATA_PHY1_CTLH));

	/* set phy PX TX amplitude */
	for (i = 0; i < ports_num; i++) {
		tmp_val = PX_TX_AMPLITUDE;
		writel(tmp_val, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL1));
	}

	/* set phy PX TX pre-emphasis */
	for (i = 0; i < ports_num; i++) {
		tmp_val = PX_TX_PREEMPH;
		writel(tmp_val, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL2));
	}

	for (i = 0; i < ports_num; i++)
		writel(PHY_FORCE_1_5G, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL));
	for (i = 0; i < ports_num; i++)
		writel(PHY_FORCE_3G, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL));
	for (i = 0; i < ports_num; i++)
		writel(PHY_FORCE_6G, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL));

	if (phy_mode == PHY_MODE_1_5G)
		phy_config = PHY_SG_1_5G;
	if (phy_mode == PHY_MODE_3G)
		phy_config = PHY_SG_3G;
	if (phy_mode == PHY_MODE_6G)
		phy_config = PHY_SG_6G;

	for (i = 0; i < ports_num; i++)
		writel(phy_config, (mmio + 0x100 + i*0x80
					+ HISI_SATA_PORT_PHYCTL));
}
