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
	HISI_SATA_PERI_CTRL			= IO_ADDRESS(CRG_REG_BASE),
	HISI_SATA_PERI_CRG72		= (HISI_SATA_PERI_CTRL + 0x120),
	HISI_SATA_PHY0_REF0_CKEN	= BIT(0),
	HISI_SATA_PHY0_REF1_CKEN	= BIT(1),
	HISI_SATA_PHY0_REFCK0_SEL_100M	= (0x2 << 4),
	HISI_SATA_PHY0_REFCK0_SEL_25M	= (0x1 << 4),
	HISI_SATA_PHY0_REFCK0_SEL_24M	= (0x0 << 4),
	HISI_SATA_PHY0_REFCK1_SEL_100M	= (0x2 << 6),
	HISI_SATA_PHY0_REFCK1_SEL_25M	= (0x1 << 6),
	HISI_SATA_PHY0_REFCK1_SEL_24M	= (0x0 << 6),
	HISI_SATA_PHY1_REF0_CKEN	= BIT(8),
	HISI_SATA_PHY1_REF1_CKEN	= BIT(9),
	HISI_SATA_PHY1_REFCK0_SEL_100M	= (0x2 << 12),
	HISI_SATA_PHY1_REFCK0_SEL_25M	= (0x1 << 12),
	HISI_SATA_PHY1_REFCK0_SEL_24M	= (0x0 << 12),
	HISI_SATA_PHY1_REFCK1_SEL_100M	= (0x2 << 14),
	HISI_SATA_PHY1_REFCK1_SEL_25M	= (0x1 << 14),
	HISI_SATA_PHY1_REFCK1_SEL_24M	= (0x0 << 14),
	HISI_SATA_CLK_VALUE			= HISI_SATA_PHY0_REF0_CKEN
								| HISI_SATA_PHY0_REF1_CKEN
								| HISI_SATA_PHY0_REFCK0_SEL_100M
								| HISI_SATA_PHY0_REFCK1_SEL_100M
								| HISI_SATA_PHY1_REF0_CKEN
								| HISI_SATA_PHY1_REF1_CKEN
								| HISI_SATA_PHY1_REFCK0_SEL_100M
								| HISI_SATA_PHY1_REFCK1_SEL_100M,

	HISI_SATA_PHY0A_RST			= BIT(2),
	HISI_SATA_PHY0B_RST			= BIT(3),
	HISI_SATA_PHY1A_RST			= BIT(10),
	HISI_SATA_PHY1B_RST			= BIT(11),
	HISI_SATA_PHY0_RST			= HISI_SATA_PHY0A_RST
								| HISI_SATA_PHY0B_RST,
	HISI_SATA_PHY1_RST			= HISI_SATA_PHY1A_RST
								| HISI_SATA_PHY1B_RST,

	HISI_SATA_PHY0A_RST_MASK	= BIT(7),
	HISI_SATA_PHY0B_RST_MASK	= BIT(6),
	HISI_SATA_PHY1A_RST_MASK	= BIT(5),
	HISI_SATA_PHY1B_RST_MASK	= BIT(4),
	HISI_SATA_PHY0_RST_MASK		= HISI_SATA_PHY0A_RST_MASK
								| HISI_SATA_PHY0B_RST_MASK,
	HISI_SATA_PHY1_RST_MASK		= HISI_SATA_PHY1A_RST_MASK
								| HISI_SATA_PHY1B_RST_MASK,

	HISI_SATA_PERI_CRG74		= (HISI_SATA_PERI_CTRL + 0x128),

	HISI_SATA_BUS_CKEN			= BIT(0),
	HISI_SATA_BUS_SRST_REQ		= BIT(8),
	HISI_SATA_CKO_ALIVE_CKEN	= BIT(2),
	HISI_SATA_CKO_ALIVE_SRST_REQ  = BIT(9),
	HISI_SATA_RX0_CKEN			= BIT(1),
	HISI_SATA_TX0_CKEN			= BIT(3),
	HISI_SATA_RX0_SRST_REQ		= BIT(10),
	HISI_SATA0_SRST_REQ			= BIT(11),
	HISI_SATA_RX1_CKEN			= BIT(12),
	HISI_SATA_TX1_CKEN			= BIT(13),
	HISI_SATA_RX1_SRST_REQ		= BIT(14),
	HISI_SATA1_SRST_REQ			= BIT(15),
	HISI_SATA_RX2_CKEN			= BIT(16),
	HISI_SATA_TX2_CKEN			= BIT(17),
	HISI_SATA_RX2_SRST_REQ		= BIT(18),
	HISI_SATA2_SRST_REQ			= BIT(19),
	HISI_SATA_RX3_CKEN			= BIT(20),
	HISI_SATA_TX3_CKEN			= BIT(21),
	HISI_SATA_RX3_SRST_REQ		= BIT(22),
	HISI_SATA3_SRST_REQ			= BIT(23),

	HISI_SATA_SYS_CTRL		= IO_ADDRESS(SYS_CTRL_REG_BASE + 0x8C),
	HISI_SATA_PCIE_MODE_SHIFT	= 12,

	PHY_CONFIG_1_5G		= 0x0e180000,
	PHY_CONFIG_3G		= 0x0e390000,
	PHY_CONFIG_6G		= 0x0e5a0000,

	PHY_SG_1_5G			= 0x61438,
	PHY_SG_3G			= 0x61438,
	PHY_SG_6G			= 0x61438,
};

#define	HISI_SATA_MISC_CTRL				IO_ADDRESS(MISC_CTRL_REG_BASE)
#define HISI_SATA_MISC_COMB_PHY0		(HISI_SATA_MISC_CTRL + 0x134)
#define HISI_SATA_MISC_COMB_PHY1		(HISI_SATA_MISC_CTRL + 0x138)

static unsigned int mplx_port0;

static void hisi_sata_get_port_info(void)
{
	unsigned int tmp_val, mode;

	tmp_val = readl((void *)HISI_SATA_SYS_CTRL);

	mode = (tmp_val >> HISI_SATA_PCIE_MODE_SHIFT) & 0xf;

	switch (mode) {
		case 0x0:
			ports_num = 4;
			sata_port_map = 0xf;
			break;

		case 0x1:
			ports_num = 3;
			sata_port_map = 0x7;
			break;

		case 0x8:
			ports_num = 3;
			sata_port_map = 0xe;
			break;

		case 0x2:
		case 0x3:
			ports_num = 2;
			sata_port_map = 0x3;
			break;

		case 0x9:
			ports_num = 2;
			sata_port_map = 0x6;
			break;

		case 0xa:
		case 0xb:
			ports_num = 1;
			sata_port_map = 0x2;
			break;

		default:
			ports_num = 0;
			sata_port_map = 0x0;
			break;
	}

	mplx_port0 = (mode & 0x8) ? 1 : 0;
}

static void hisi_sata_poweron(void)
{
}

static void hisi_sata_poweroff(void)
{
}

void hisi_sata_reset_rxtx_assert(unsigned int port_no)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG74);

	if (port_no == 0) {
		tmp_val |= HISI_SATA_RX0_SRST_REQ
				| HISI_SATA0_SRST_REQ;
	} else if (port_no == 1) {
		tmp_val |= HISI_SATA_RX1_SRST_REQ
			| HISI_SATA1_SRST_REQ;
	} else if (port_no == 2) {
		tmp_val |= HISI_SATA_RX2_SRST_REQ
			| HISI_SATA2_SRST_REQ;
	} else if (port_no == 3) {
		tmp_val |= HISI_SATA_RX3_SRST_REQ
				| HISI_SATA3_SRST_REQ;
	}

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG74);
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_assert);

void hisi_sata_reset_rxtx_deassert(unsigned int port_no)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG74);

	if (port_no == 0) {
		tmp_val &= ~(HISI_SATA_RX0_SRST_REQ
				| HISI_SATA0_SRST_REQ);
	} else if (port_no == 1) {
		tmp_val &= ~(HISI_SATA_RX1_SRST_REQ
				| HISI_SATA1_SRST_REQ);
	} else if (port_no == 2) {
		tmp_val &= ~(HISI_SATA_RX2_SRST_REQ
				| HISI_SATA2_SRST_REQ);
	} else if (port_no == 3) {
		tmp_val &= ~(HISI_SATA_RX3_SRST_REQ
				| HISI_SATA3_SRST_REQ);
	}

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG74);
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_deassert);

static void hisi_sata_reset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG74);

	tmp_val |= HISI_SATA_BUS_SRST_REQ | HISI_SATA_CKO_ALIVE_SRST_REQ;

	if (ports_num == 4) {
		tmp_val |= HISI_SATA_RX0_SRST_REQ
				| HISI_SATA0_SRST_REQ
				| HISI_SATA_RX1_SRST_REQ
				| HISI_SATA1_SRST_REQ
				| HISI_SATA_RX2_SRST_REQ
				| HISI_SATA2_SRST_REQ
				| HISI_SATA_RX3_SRST_REQ
				| HISI_SATA3_SRST_REQ;
	} else if (ports_num == 3) {
		if (mplx_port0) {
			tmp_val |= HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ
					| HISI_SATA_RX3_SRST_REQ
					| HISI_SATA3_SRST_REQ;
		} else {
			tmp_val |= HISI_SATA_RX0_SRST_REQ
					| HISI_SATA0_SRST_REQ
					| HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ;
		}
	} else if (ports_num == 2) {
		if (mplx_port0) {
			tmp_val |= HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ;
		} else {
			tmp_val |= HISI_SATA_RX0_SRST_REQ
					| HISI_SATA0_SRST_REQ
					| HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ;
		}
	} else if (ports_num == 1) {
		tmp_val |= HISI_SATA_RX1_SRST_REQ
				| HISI_SATA1_SRST_REQ;
	}

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG74);
}

static void hisi_sata_unreset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG74);

	tmp_val &= ~(HISI_SATA_BUS_SRST_REQ | HISI_SATA_CKO_ALIVE_SRST_REQ);

	if (ports_num == 4) {
		tmp_val &= ~(HISI_SATA_RX0_SRST_REQ
					| HISI_SATA0_SRST_REQ
					| HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ
					| HISI_SATA_RX3_SRST_REQ
					| HISI_SATA3_SRST_REQ);
	} else if (ports_num == 3) {
		if (mplx_port0) {
			tmp_val &= ~(HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ
					| HISI_SATA_RX3_SRST_REQ
					| HISI_SATA3_SRST_REQ);
		} else {
			tmp_val &= ~(HISI_SATA_RX0_SRST_REQ
					| HISI_SATA0_SRST_REQ
					| HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ);
		}
	} else if (ports_num == 2) {
		if (mplx_port0) {
			tmp_val &= ~(HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ
					| HISI_SATA_RX2_SRST_REQ
					| HISI_SATA2_SRST_REQ);
		} else {
			tmp_val &= ~(HISI_SATA_RX0_SRST_REQ
					| HISI_SATA0_SRST_REQ
					| HISI_SATA_RX1_SRST_REQ
					| HISI_SATA1_SRST_REQ);
		}
	} else if (ports_num == 1) {
		tmp_val &= ~(HISI_SATA_RX1_SRST_REQ
				| HISI_SATA1_SRST_REQ);
	}

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG74);
}

static void hisi_sata_phy_reset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG72);

	if (ports_num == 4)
		tmp_val |= HISI_SATA_PHY0_RST | HISI_SATA_PHY1_RST;
	if (ports_num == 3) {
		if (mplx_port0)
			tmp_val |= HISI_SATA_PHY1A_RST | HISI_SATA_PHY0_RST;
		else
			tmp_val |= HISI_SATA_PHY1_RST | HISI_SATA_PHY0B_RST;
	}
	if (ports_num == 2) {
		if (mplx_port0)
			tmp_val |= HISI_SATA_PHY1A_RST | HISI_SATA_PHY0B_RST;
		else
			tmp_val |= HISI_SATA_PHY1_RST;
	}
	if (ports_num == 1)
		tmp_val |= HISI_SATA_PHY1A_RST;

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG72);
}

static void hisi_sata_phy_unreset(void)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HISI_SATA_PERI_CRG72);

	if (ports_num == 4)
		tmp_val &= ~(HISI_SATA_PHY0_RST | HISI_SATA_PHY1_RST);
	if (ports_num == 3) {
		if (mplx_port0)
			tmp_val &= ~(HISI_SATA_PHY1A_RST | HISI_SATA_PHY0_RST);
		else
			tmp_val &= ~(HISI_SATA_PHY1_RST | HISI_SATA_PHY0B_RST);
	}
	if (ports_num == 2) {
		if (mplx_port0)
			tmp_val &= ~(HISI_SATA_PHY1A_RST | HISI_SATA_PHY0B_RST);
		else
			tmp_val &= ~HISI_SATA_PHY1_RST;
	}
	if (ports_num == 1)
		tmp_val &= ~HISI_SATA_PHY1A_RST;

	writel(tmp_val, (void *)HISI_SATA_PERI_CRG72);
}

static void hisi_sata_clk_enable(void)
{
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

	tmp_val = readl((void *)HISI_SATA_PERI_CRG72);
	tmp_val |= HISI_SATA_CLK_VALUE;
	writel(tmp_val, (void *)HISI_SATA_PERI_CRG72);
}

void hisi_sata_set_fifoth(void *mmio)
{
	int i, port_no;

	for (i = 0; i < ports_num; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		writel(HISI_SATA_FIFOTH_VALUE, (mmio + 0x100 + port_no*0x80
					+ HISI_SATA_PORT_FIFOTH));
	}
}
EXPORT_SYMBOL(hisi_sata_set_fifoth);

void hisi_sata_phy_config(void *mmio, int phy_mode)
{
	unsigned int tmp, phy_config = PHY_CONFIG_3G;
	unsigned int phy_sg = PHY_SG_3G;
	int i, port_no;

	hisi_sata_set_fifoth(mmio);

	tmp = readl(mmio + HISI_SATA_PHY_CTL1);
	tmp |= HISI_SATA_BIGENDINE;
	writel(tmp, (mmio + HISI_SATA_PHY_CTL1));
	tmp = readl(mmio + HISI_SATA_PHY_CTL2);
	tmp |= HISI_SATA_BIGENDINE;
	writel(tmp, (mmio + HISI_SATA_PHY_CTL2));

	tmp = readl(mmio + HISI_SATA_RST_PHY_MASK);
	tmp &= 0xffffff0f;
	if (ports_num == 1)
		tmp |= HISI_SATA_PHY0_RST_MASK | HISI_SATA_PHY1B_RST_MASK;
	if (ports_num == 2) {
		if (mplx_port0)
			tmp |= HISI_SATA_PHY1B_RST_MASK | HISI_SATA_PHY0A_RST_MASK;
		else
			tmp |= HISI_SATA_PHY0_RST_MASK;	/* mode:2,3 */
	}
	if (ports_num == 3) {
		if (mplx_port0)
			tmp |= HISI_SATA_PHY1B_RST_MASK;	/* mode:8 */
		else
			tmp |= HISI_SATA_PHY0A_RST_MASK;	/* mode:1 */
	}
	writel(tmp, (mmio + HISI_SATA_RST_PHY_MASK));

	if (phy_mode == PHY_MODE_1_5G) {
		phy_config = PHY_CONFIG_1_5G;
		phy_sg = PHY_SG_1_5G;
	}

	if (phy_mode == PHY_MODE_3G) {
		phy_config = PHY_CONFIG_3G;
		phy_sg = PHY_SG_3G;
	}

	if (phy_mode == PHY_MODE_6G) {
		phy_config = PHY_CONFIG_6G;
		phy_sg = PHY_SG_6G;
	}

	for (i = 0; i < ports_num; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		if (port_no == 0) {
			writel(0x439, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x479, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x439, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY1);

			writel(0x822, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x862, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x822, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY1);

			writel(0x421, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x461, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x421, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY1);
		} else if (port_no == 1) {
			writel(0x419, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x459, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x419, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY1);

			writel(0x802, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x842, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x802, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY1);

			writel(0x401, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x441, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x401, (void *)HISI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY1);
		} else if (port_no == 2) {
			writel(0x439, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x479, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x439, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY0);

			writel(0x822, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x862, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x822, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY0);

			writel(0x421, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x461, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x421, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY0);
		} else if (port_no == 3) {
			writel(0x419, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x459, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x419, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY0);

			writel(0x802, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x842, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x802, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY0);

			writel(0x401, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x441, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x401, (void *)HISI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HISI_SATA_MISC_COMB_PHY0);
		}
	}

	for (i = 0; i < ports_num; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		writel(phy_config, (mmio + 0x100 + port_no*0x80
					+ HISI_SATA_PORT_PHYCTL));

		writel(phy_sg, (mmio + 0x100 + port_no*0x80
					+ HISI_SATA_PORT_PHYCTL1));
	}
}
