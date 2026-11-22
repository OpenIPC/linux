// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiSilicon AHCI SATA PHY driver
 *
 * Copyright (c) 2026 Dylan Corrales <deathcamel58@gmail.com>
 *
 * Based on drivers/ata/ahci_hi3520dv200.c by Bjorn Stout, the AHCI glue driver
 * of the same controller, from which the register values and the order of the
 * steps were taken (GPL-2.0-or-later, like this file).
 *
 * The PHY is configured through registers inside the AHCI register window
 * (PHY control, OOB control and the per-port FIFOTH and PHYCTL registers) and
 * has its own reset line (PERI_CRG45 on the Hi3520D V100). The AHCI clocks and
 * controller resets must already be on when the PHY is initialized, which
 * ahci_platform guarantees by enabling them before it initializes its PHYs;
 * the PHY node cannot request them, they belong to the AHCI node.
 *
 * The register descriptions show the same layout on the Hi3520D, Hi3515A and
 * Hi3515C; whether the tuning values of those SoCs are the same is not known,
 * and only the Hi3520D V100 is supported and tested. The Hi3536D V100 is
 * supported by its own match data (see below); it was tested on one board
 * with the controller resets released by the boot loader, see the comment of
 * that variant.
 * The Hi3520D V300 and Hi3535 have the PHY registers at other offsets with
 * other fields and need separate register definitions. Register and field names follow the
 * Hi3520D V100 register description (SATA block). The sequence and the values
 * come from the GPL vendor BSP (drivers/ata/ahci_sys.c and CONFIG_HI_SATA,
 * 3 Gbit/s mode) and from the ahci_hi3520dv200 glue driver.
 *
 * Differences from the vendor sequence (hi_sata_init(), the data sheet's
 * SATA initialization): the controller, TX, RX and alive resets are released
 * by the AHCI node (its resets property) before the PHY is programmed (the
 * vendor releases them last), and the clock selection bits of PERI_CRG45 are
 * not touched: they are clock muxes of the CRG clock driver, which the AHCI
 * node selects with assigned-clocks and assigned-clock-parents.
 *
 * The second PHY control set of the vendor driver (SATA_PHY1_CTLL/CTLH,
 * 0x60/0x64) is not written: the data sheet has one set for both lanes, and
 * the PHY1 registers read back 0 on the tested board. Both ports were tested
 * (port 0 with an SSD, port 1 with an HDD) without writing them.
 *
 * The system controller options of the SATA PHY are set explicitly through the
 * "hisilicon,sysctrl" regmap (they survive a soft reset): PERIPHCTRL25 test
 * bits (bit 2 sata_test_pddq powers the PHY down, bits 21 and 20 are test
 * modes) are cleared and PERIPHCTRL3 bits 9:0 (sata_phy0_ref_clk_sel,
 * sata_phy0_mpll_ss_sel) written with the reference values of the match data.
 * The vendor driver never writes them; the values are the data sheet reset
 * values. Still assumed, because the driver cannot set them: the EPLL behind
 * the 150 MHz and 50 MHz clocks, which the clock driver does not program, and
 * the PHY1 registers above.
 *
 * Like the vendor sequence, the MPLL is switched off (CTLH MPLL_CK_OFF) while
 * CTLL is written: the data sheet allows changing the MPLL parameters only
 * while the MPLL is off, and the registers may still hold values from an
 * earlier boot stage or kernel. The final CTLH write switches it on again.
 *
 * The Hi3536D V100 has one port and other global PHY registers (SATA_PHY_CTL0,
 * SATA_PHY_CTL1 and SATA_OOB_CTL at 0xa0, 0xa4 and 0xb8 of the AHCI window,
 * and PHYCTL1 per port), a PHY reference clock and a PHY lane reset in
 * PERI_CRG44, and the analog PHY parameters behind MISC_CTRL22 (0x58 of
 * MISC_CTRL), a register that the data sheet does not describe. Its sequence
 * comes from the vendor kernel (phy-hi3536dv100-sata.c) and the data sheet
 * (SATA, operating mode configuration); the differences are described at the
 * hooks of that variant.
 */

#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/reset.h>

/* "ctrl" window, offsets from SATA_TM (0x50 in the AHCI window) */
#define SATA_PHY0_CTLL			0x04
#define SATA_PHY0_CTLH			0x08
#define SATA_OOB_CTL			0x1c

/*
 * Hi3536D V100 "ctrl" window, offsets from SATA_PHY_CTL0 (0xa0 in the AHCI
 * window). SATA_PHY_RST_BACK_MASK and SATA_PHY_CTL2 are not in the data sheet
 * (the vendor driver writes them, a Hi3536D has no second PHY).
 */
#define HI3536D_PHY_CTL0		0x00
#define HI3536D_PHY_CTL1		0x04
#define HI3536D_PHY_RST_BACK_MASK	0x0c
#define HI3536D_PHY_CTL2		0x10
#define HI3536D_OOB_CTL			0x18

/* "ports" window, per port, 0x80 apart */
#define SATA_PORT_STRIDE		0x80
#define SATA_PORT_FIFOTH		0x44
#define SATA_PORT_PHYCTL1		0x48
#define SATA_PORT_PHYCTL		0x74

/* SATA_PHY0_CTLL: analog PHY parameters */
/* reference clock prescale, reset value 2 (divide by 2) */
#define SATA_PHY_CTLL_MPLL_PRESCALE	GENMASK(31, 30)
#define SATA_PHY_CTLL_MPLL_NCY		GENMASK(29, 25)
#define SATA_PHY_CTLL_MPLL_PROP_CTL	GENMASK(19, 17)
#define SATA_PHY_CTLL_TX_LVL		GENMASK(16, 12)
#define SATA_PHY_CTLL_LOS_LVL		GENMASK(11, 7)
#define SATA_PHY_CTLL_ACJT_LVL		GENMASK(6, 2)

/* SATA_PHY0_CTLH */
#define SATA_PHY_CTLH_USE_REFCLK_ALT	BIT(13)
#define SATA_PHY_CTLH_MPLL_CK_OFF	BIT(12)
#define SATA_PHY_CTLH_MPLL_SS_EN	BIT(10)
#define SATA_PHY_CTLH_CKO_WORD_CON	GENMASK(9, 7)
#define SATA_PHY_CTLH_CKO_ALIVE_CON	GENMASK(6, 5)
#define SATA_PHY_CTLH_WIDE_XFACE	BIT(0)

/* SATA_OOB_CTL: COMWAKE/COMINIT burst spacing limits */
#define SATA_OOB_CTL_VALID		BIT(31)
#define SATA_OOB_CTL_MIN_COMWAKE	GENMASK(30, 24)
#define SATA_OOB_CTL_MAX_COMWAKE	GENMASK(23, 16)
#define SATA_OOB_CTL_MIN_COMINIT	GENMASK(15, 8)
#define SATA_OOB_CTL_MAX_COMINIT	GENMASK(7, 0)

/* System controller: SATA PHY options, written through the regmap */
#define SYSCTRL_PERIPHCTRL3		0x38
#define SYSCTRL_PERIPHCTRL3_PHY_MASK	GENMASK(9, 0)	/* ref_clk_sel, mpll_ss_sel */
#define SYSCTRL_PERIPHCTRL25		0x98
#define SYSCTRL_PERIPHCTRL25_PDDQ	BIT(2)		/* 1 powers the PHY down */
#define SYSCTRL_PERIPHCTRL25_TEST	(BIT(21) | BIT(20))

/*
 * SATA_PORT_FIFOTH: bits 13:9 bypass hardware bugs of the controller (data
 * transmit control, record, PM reset, fetch and a Seagate R_ERR issue; the
 * vendor driver sets them on every port, the data sheet does not describe
 * them), the rest are the receive FIFO flow control thresholds. Bit 3
 * (rxfifo_th_sel, reset value 1) is cleared by the value written.
 */
#define SATA_PORT_FIFOTH_BYPASS_ALL	GENMASK(13, 9)
#define SATA_PORT_FIFOTH_DMAC_RXFIFO_TH	GENMASK(8, 4)
#define SATA_PORT_FIFOTH_RXFIFO_TH_SEL	BIT(3)
#define SATA_PORT_FIFOTH_LINK_RXFIFO_TH	GENMASK(2, 0)

/* SATA_PORT_PHYCTL: the fields the 3 Gbit/s tuning sets */
#define SATA_PORT_PHYCTL_CALIBRATED	BIT(27)
#define SATA_PORT_PHYCTL_SPD_CHG_ACK	BIT(26)
#define SATA_PORT_PHYCTL_DP_RDY		BIT(25)
#define SATA_PORT_PHYCTL_GEN2_EN	BIT(22)
#define SATA_PORT_PHYCTL_LOS_CTL	GENMASK(21, 20)
#define SATA_PORT_PHYCTL_RX_DPLL_MODE	GENMASK(19, 17)
#define SATA_PORT_PHYCTL_RX_EQ_VAL	GENMASK(16, 14)
#define SATA_PORT_PHYCTL_RX_TERM_EN	BIT(13)
#define SATA_PORT_PHYCTL_TX_EDGERATE	GENMASK(11, 10)
#define SATA_PORT_PHYCTL_TX_CKO_EN	BIT(9)
#define SATA_PORT_PHYCTL_RX_ALIGN_EN	BIT(8)
#define SATA_PORT_PHYCTL_TX_ATTEN	GENMASK(6, 4)
#define SATA_PORT_PHYCTL_TX_BOOST	GENMASK(3, 0)

/* Hi3536D V100 PHY control registers */
#define HI3536D_PHY_CTL0_RESET		0xc9802780	/* not written */
#define HI3536D_PHY_CTL1_DATA_INVERT	BIT(3)
#define HI3536D_PHY_RST_BACK_ALL	GENMASK(7, 4)	/* reset-back mask of the four lanes */
#define HI3536D_PHY_RST_BACK_PHY0	BIT(4)
#define HI3536D_PHY_CTL2_DATA_INVERT	BIT(3)
#define HI3536D_OOB_CTL_RESET		0x84060c15	/* not written */
/* the controller resets of PERI_CRG45 (bits 8 to 11) that the PHY pulses */
#define HI3536D_NUM_CTRL_RSTS		4

/* SATA_PORT_FIFOTH: bits 8:0 are the documented thresholds, 31:9 are reserved */
#define HI3536D_FIFOTH_CHECK_MASK	GENMASK_U32(8, 0)

/*
 * SATA_PORT_PHYCTL1: TX amplitude (margin), pre-emphasis (deemph) and swing
 * for Gen1 (0), Gen2 (1) and Gen3 (2); the vendor value is 0x00050438. It
 * also clears the reserved bits 21, 14 and 7, which read 1 after a reset.
 */
#define HI3536D_PHYCTL1_MARGIN2		GENMASK(20, 18)
#define HI3536D_PHYCTL1_DEEMPH2		GENMASK(17, 16)
#define HI3536D_PHYCTL1_MARGIN1		GENMASK(13, 11)
#define HI3536D_PHYCTL1_DEEMPH1		GENMASK(10, 9)
#define HI3536D_PHYCTL1_MARGIN0		GENMASK(6, 4)
#define HI3536D_PHYCTL1_DEEMPH0		GENMASK(3, 2)
#define HI3536D_PHYCTL1_CHECK_MASK	(GENMASK_U32(20, 15) | GENMASK_U32(13, 8) | \
					 GENMASK_U32(6, 1))

/*
 * SATA_PORT_PHYCTL of the Hi3536D V100: GEN2_EN is the field 22:21 (1 = 3
 * Gbit/s, 0 = 1.5 Gbit/s). The vendor values are 0x0e180000 for 1.5 and
 * 0x0e390000 for 3 Gbit/s; they also differ in bit 16, which the register
 * description calls reserved, so it is a speed bit here like the field. The
 * ready, calibrated and speed-change-acknowledge bits (27:25) are not compared.
 */
#define HI3536D_PHYCTL_GEN2_EN		BIT(21)
#define HI3536D_PHYCTL_GEN2_RSVD	BIT(16)
#define HI3536D_PHYCTL_CHECK_MASK	(GENMASK_U32(31, 28) | GENMASK_U32(24, 0))

/*
 * MISC_CTRL22 (0x58 of MISC_CTRL) is an indirect access port to the analog
 * PHY: each value is written, written again with bit 6 set, written again
 * without it and finally cleared, which looks like a strobe of the value in
 * bits 11:0 (inferred from the sequence, the data sheet does not describe the
 * register). The first group forces the PLL to 6 Gbit/s and the CDR to 3
 * Gbit/s for Gen1 and 6 Gbit/s for Gen2, the second disables spread spectrum
 * (vendor comments).
 */
#define HI3536D_MISC_CTRL22		0x58
#define HI3536D_MISC_STROBE		BIT(6)

/*
 * Largest valid codes of the tuning fields a board may set. TX_ATTEN: 0 to 4
 * are the ratios 16/16, 14/16, 12/16, 10/16 and 9/16, 5 is reserved. TX_BOOST
 * is -20 log [1 - (code + 0.5) / 32] dB and RX_EQ_VAL about (code + 1) x 0.5
 * dB (data sheet formulas, not measured).
 */
#define SATA_TX_BOOST_MAX		15
#define SATA_TX_ATTEN_MAX		4
#define SATA_RX_EQ_MAX			7

/*
 * Bits compared by the read-back after programming. CTLH: the PLL parameters
 * and options, not the reset, tune and reserved bits 4:1. FIFOTH: the bits
 * software sets, not the reserved ones. PHYCTL: all fields that software
 * writes, not the ready, calibrated and speed-change-acknowledge bits (27:25)
 * or the reserved bits 31:29.
 */
#define SATA_PHY_CTLH_CHECK_MASK	(GENMASK_U32(13, 5) | BIT_U32(0))
#define SATA_PORT_FIFOTH_CHECK_MASK	GENMASK_U32(13, 0)
#define SATA_PORT_PHYCTL_CHECK_MASK	(BIT_U32(28) | GENMASK_U32(24, 0))

/* Size of the Hi3520D "ctrl" window: up to and including SATA_OOB_CTL */
#define SATA_CTRL_WINDOW_MIN_SIZE	(SATA_OOB_CTL + sizeof(u32))

/*
 * The largest controller implements two ports (the Hi3520D V100, ports
 * implemented 0x3; the Hi3536D V100 has one); the number of ports the PHY
 * programs is the size of the "ports" window, not read back from the AHCI
 * registers.
 */
#define SATA_MAX_PORTS			2

/* Link speed (Mbit/s) from which GEN2_EN, the 3 Gbit/s mode, is set */
#define SATA_GEN2_SPEED_MBPS		3000

/*
 * Settle time after each PHY reset change. The vendor driver (hi_sata_init()
 * in drivers/ata/ahci_sys.c) waits the same time; no data sheet minimum is
 * known.
 */
#define SATA_PHY_RESET_DELAY_MS		20

struct hisi_ahci_sata_phy;

/**
 * struct hisi_ahci_sata_phy_cfg - register layout and tuning values of one SoC
 * @ctrl_size:	smallest size of the "ctrl" register window
 * @get_resources: look up the resources of the variant at probe time
 * @power_on:	set the options of the SoC before the PHY reset is released
 * @power_off:	undo @power_on after the PHY reset is asserted (optional)
 * @release:	step after the PHY reset is released and has settled (optional)
 * @program:	write the PHY registers that all ports share
 * @finish:	configure the PHY after the reset pulse (optional)
 * @check:	read back the shared registers and warn about a difference
 * @tuning:	the per-port tuning properties apply to the PHYCTL register
 * @has_phyctl1: the ports have a PHYCTL1 register: @phyctl1 is written and
 *		read back (the SoC has none otherwise and the register is not
 *		touched)
 * @phyctl1:	SATA_PORT_PHYCTL1 value
 * @phyctl1_check: bits of PHYCTL1 that the read-back compares
 * @fifoth:	SATA_PORT_FIFOTH value
 * @fifoth_check: bits of FIFOTH that the read-back compares
 * @portctl:	SATA_PORT_PHYCTL reference value; a board may set TX_BOOST,
 *		TX_ATTEN and RX_EQ_VAL per port, the rest is fixed
 * @portctl_check: bits of PHYCTL that the read-back compares
 * @gen2_bits:	bits of PHYCTL that select 3 Gbit/s
 * @ctll:	SATA_PHY0_CTLL value
 * @ctlh:	SATA_PHY0_CTLH value, without MPLL_CK_OFF
 * @oob:	SATA_OOB_CTL value
 * @periphctrl3: reference value of the PHY bits of the system controller
 *		PERIPHCTRL3 (bits 9:0)
 *
 * The tuning cannot be read back from the hardware (the registers reset to
 * other values), so it is match data. The number of ports comes from the size
 * of the "ports" register window in the device tree. A SoC with this register
 * layout needs one more instance of this structure and a compatible; a SoC
 * with other registers also needs its own hooks. @ctll, @ctlh, @oob and
 * @periphctrl3 belong to the Hi3520D V100 hooks.
 *
 * The values are the Hi3520D vendor kernel's 3 Gbit/s ones (the data sheet
 * has other PHYCTL values, 0x0e262709 for 1.5 Gbit/s and 0x0e662709 for 3
 * Gbit/s); they were tested on one board with three different drives. A
 * lower link speed is selected at run time by clearing GEN2_EN in PHYCTL
 * (see hisi_ahci_sata_phy_set_speed()); the vendor's separate 1.5 Gbit/s
 * tuning (CTLL 0x880ec788, PHYCTL 0x0e276716) is not needed and does not
 * bring the link up on the tested board.
 */
struct hisi_ahci_sata_phy_cfg {
	size_t ctrl_size;
	int (*get_resources)(struct hisi_ahci_sata_phy *priv);
	int (*power_on)(struct hisi_ahci_sata_phy *priv);
	void (*power_off)(struct hisi_ahci_sata_phy *priv);
	int (*release)(struct hisi_ahci_sata_phy *priv);
	void (*program)(struct hisi_ahci_sata_phy *priv);
	int (*finish)(struct hisi_ahci_sata_phy *priv);
	void (*check)(const struct hisi_ahci_sata_phy *priv);
	bool tuning;
	bool has_phyctl1;
	u32 phyctl1;
	u32 phyctl1_check;
	u32 fifoth;
	u32 fifoth_check;
	u32 portctl;
	u32 portctl_check;
	u32 gen2_bits;
	u32 ctll;
	u32 ctlh;
	u32 oob;
	u32 periphctrl3;
};

/**
 * struct hisi_ahci_sata_port - one PHY, the PHY of one port
 * @priv:	the device
 * @idx:	port number
 * @gen2:	3 Gbit/s allowed by the link speed limit of this port
 */
struct hisi_ahci_sata_port {
	struct hisi_ahci_sata_phy *priv;
	unsigned int idx;
	bool gen2;
};

/**
 * struct hisi_ahci_sata_phy - the PHY device, one struct phy per port
 * @dev:	the device
 * @ctrl:	"ctrl" registers (global PHY and OOB control)
 * @ports:	"ports" registers (per-port FIFOTH and PHYCTL)
 * @nports:	number of ports
 * @rst:	PHY reset, shared by the ports
 * @sysctrl:	system controller regmap (PHY power and reference options)
 * @misc:	MISC_CTRL regmap (analog PHY parameters), Hi3536D V100
 * @ref_clk:	PHY reference clock, Hi3536D V100
 * @ctrl_rsts:	controller resets, shared with the AHCI node, Hi3536D V100
 * @ctrl_held:	the PHY asserted @ctrl_rsts and has not released them yet
 * @cfg:	tuning values of the SoC
 * @portctl:	PHYCTL value of each port without GEN2_EN, which follows the
 *		link speed
 * @lock:	protects @users and the PHYCTL read-modify-write of set_speed
 * @users:	number of initialized PHYs; the shared registers are programmed
 *		and the PHY reset is pulsed by the first, asserted by the last
 * @port:	state of each port
 * @phys:	the PHY of each port
 */
struct hisi_ahci_sata_phy {
	struct device *dev;
	void __iomem *ctrl;
	void __iomem *ports;
	unsigned int nports;
	struct reset_control *rst;
	struct regmap *sysctrl;
	struct regmap *misc;
	struct clk *ref_clk;
	struct reset_control_bulk_data ctrl_rsts[HI3536D_NUM_CTRL_RSTS];
	bool ctrl_held;
	const struct hisi_ahci_sata_phy_cfg *cfg;
	u32 portctl[SATA_MAX_PORTS];
	struct mutex lock;	/* see the kernel-doc above */
	unsigned int users;
	struct hisi_ahci_sata_port port[SATA_MAX_PORTS];
	struct phy *phys[SATA_MAX_PORTS];
};

static u32 hisi_ahci_sata_phy_portctl(const struct hisi_ahci_sata_phy *priv,
				      unsigned int port)
{
	return priv->portctl[port] |
	       (priv->port[port].gen2 ? priv->cfg->gen2_bits : 0);
}

static void hisi_ahci_sata_phy_check_reg(const struct hisi_ahci_sata_phy *priv,
					 const char *name, void __iomem *reg,
					 u32 want, u32 mask)
{
	u32 val = readl(reg);

	if ((val ^ want) & mask)
		dev_warn(priv->dev,
			 "%s reads back 0x%08x, expected 0x%08x (compared under 0x%08x); are the AHCI controller resets released?\n",
			 name, val, want, mask);
}

static void hisi_ahci_sata_phy_check_ctrl_hi3520dv100(const struct hisi_ahci_sata_phy *priv)
{
	hisi_ahci_sata_phy_check_reg(priv, "PHY0_CTLL",
				     priv->ctrl + SATA_PHY0_CTLL,
				     priv->cfg->ctll, U32_MAX);
	hisi_ahci_sata_phy_check_reg(priv, "PHY0_CTLH",
				     priv->ctrl + SATA_PHY0_CTLH,
				     priv->cfg->ctlh, SATA_PHY_CTLH_CHECK_MASK);
	hisi_ahci_sata_phy_check_reg(priv, "OOB_CTL", priv->ctrl + SATA_OOB_CTL,
				     priv->cfg->oob, U32_MAX);
}

/*
 * Read back what phy_init() wrote and warn about a difference; the bits that
 * software does not drive or that the data sheet calls status or ready bits
 * are not compared. A difference is not fatal: it cannot improve the link,
 * and the behavior of the block right after a power-on is not fully known.
 */
static void hisi_ahci_sata_phy_check(const struct hisi_ahci_sata_phy *priv)
{
	unsigned int i;

	priv->cfg->check(priv);

	for (i = 0; i < priv->nports; i++) {
		void __iomem *port = priv->ports + i * SATA_PORT_STRIDE;

		hisi_ahci_sata_phy_check_reg(priv, "PORT_FIFOTH",
					     port + SATA_PORT_FIFOTH,
					     priv->cfg->fifoth,
					     priv->cfg->fifoth_check);
		if (priv->cfg->has_phyctl1)
			hisi_ahci_sata_phy_check_reg(priv, "PORT_PHYCTL1",
						     port + SATA_PORT_PHYCTL1,
						     priv->cfg->phyctl1,
						     priv->cfg->phyctl1_check);
		hisi_ahci_sata_phy_check_reg(priv, "PORT_PHYCTL",
					     port + SATA_PORT_PHYCTL,
					     hisi_ahci_sata_phy_portctl(priv, i),
					     priv->cfg->portctl_check);
	}
}

/* Power the PHY up and leave the test modes: do not trust earlier state */
static int hisi_ahci_sata_phy_power_on_hi3520dv100(struct hisi_ahci_sata_phy *priv)
{
	int ret;

	ret = regmap_update_bits(priv->sysctrl, SYSCTRL_PERIPHCTRL25,
				 SYSCTRL_PERIPHCTRL25_PDDQ |
				 SYSCTRL_PERIPHCTRL25_TEST, 0);
	if (ret)
		return ret;

	return regmap_update_bits(priv->sysctrl, SYSCTRL_PERIPHCTRL3,
				  SYSCTRL_PERIPHCTRL3_PHY_MASK,
				  priv->cfg->periphctrl3);
}

/* One PHY control set serves all ports; the MPLL is off during CTLL */
static void hisi_ahci_sata_phy_program_hi3520dv100(struct hisi_ahci_sata_phy *priv)
{
	writel(priv->cfg->ctlh | SATA_PHY_CTLH_MPLL_CK_OFF,
	       priv->ctrl + SATA_PHY0_CTLH);
	writel(priv->cfg->ctll, priv->ctrl + SATA_PHY0_CTLL);
	writel(priv->cfg->ctlh, priv->ctrl + SATA_PHY0_CTLH);
	writel(priv->cfg->oob, priv->ctrl + SATA_OOB_CTL);
}

static int hisi_ahci_sata_phy_get_resources_hi3520dv100(struct hisi_ahci_sata_phy *priv)
{
	struct device *dev = priv->dev;

	priv->sysctrl = syscon_regmap_lookup_by_phandle(dev->of_node,
							"hisilicon,sysctrl");
	if (IS_ERR(priv->sysctrl))
		return dev_err_probe(dev, PTR_ERR(priv->sysctrl),
				     "cannot get the system controller\n");

	return 0;
}

static int hisi_ahci_sata_phy_do_init(struct hisi_ahci_sata_phy *priv)
{
	unsigned int i;
	int ret;

	ret = priv->cfg->power_on(priv);
	if (ret)
		return ret;

	ret = reset_control_deassert(priv->rst);
	if (ret)
		goto err_off;
	msleep(SATA_PHY_RESET_DELAY_MS);

	if (priv->cfg->release) {
		ret = priv->cfg->release(priv);
		if (ret)
			goto err_assert;
	}

	priv->cfg->program(priv);

	for (i = 0; i < priv->nports; i++) {
		void __iomem *port = priv->ports + i * SATA_PORT_STRIDE;

		writel(priv->cfg->fifoth, port + SATA_PORT_FIFOTH);
		if (priv->cfg->has_phyctl1)
			writel(priv->cfg->phyctl1, port + SATA_PORT_PHYCTL1);
		writel(hisi_ahci_sata_phy_portctl(priv, i),
		       port + SATA_PORT_PHYCTL);
	}

	/* Flush the posted writes, then pulse the PHY reset so that it samples them */
	readl(priv->ports + (priv->nports - 1) * SATA_PORT_STRIDE + SATA_PORT_PHYCTL);
	ret = reset_control_assert(priv->rst);
	if (ret)
		goto err_assert;
	msleep(SATA_PHY_RESET_DELAY_MS);
	ret = reset_control_deassert(priv->rst);
	if (ret)
		goto err_assert;
	msleep(SATA_PHY_RESET_DELAY_MS);

	if (priv->cfg->finish) {
		ret = priv->cfg->finish(priv);
		if (ret)
			goto err_assert;
	}

	/*
	 * Read back the programmed registers (a block still held in reset drops
	 * writes) and warn about a difference.
	 *
	 * PORT_PHYSTS has OP_DONE, MPLL_PWRON, PHYRDY and HALF_RATE, but none
	 * is polled. On the tested board MPLL_PWRON reads 1 at init entry on
	 * cold boots (the register description gives reset value 0), and
	 * OP_DONE is no readiness bit: a poll on it failed the probe on cold
	 * boots where it stayed 0 for more than 100 ms although the link comes
	 * up without it (it also reads 1 on a port with no device). Link
	 * training is left to libata.
	 */
	hisi_ahci_sata_phy_check(priv);

	return 0;

err_assert:
	reset_control_assert(priv->rst);
err_off:
	if (priv->cfg->power_off)
		priv->cfg->power_off(priv);
	return ret;
}

/*
 * The first PHY initialized programs the shared registers and the registers of
 * all ports, and pulses the PHY reset; the others only count. The PHY reset
 * is not pulsed again while another port may have a link up.
 */
static int hisi_ahci_sata_phy_init(struct phy *phy)
{
	struct hisi_ahci_sata_port *port = phy_get_drvdata(phy);
	struct hisi_ahci_sata_phy *priv = port->priv;
	int ret = 0;

	mutex_lock(&priv->lock);
	if (!priv->users)
		ret = hisi_ahci_sata_phy_do_init(priv);
	if (!ret)
		priv->users++;
	mutex_unlock(&priv->lock);

	return ret;
}

/*
 * The controller and the PHY of the Hi3520D V100 ignore the speed limit in the
 * SControl register of the port: with GEN2_EN set the link comes up at 3
 * Gbit/s whatever the limit is, with GEN2_EN clear it comes up at 1.5 Gbit/s.
 * The bit is sampled at the next link negotiation of the port, a PHY reset is
 * not needed. The Hi3536D V100 takes its speed bits (see gen2_bits) the same
 * way; whether its PHY also ignores the limit in SControl is not known.
 *
 * The register is written whole from the tuning and the state of the port: the
 * register description gives no bit of PHYCTL that the hardware changes (all
 * are software read-write, hardware read-only). The state is kept so that
 * phy_init() after a resume applies the last limit.
 */
static int hisi_ahci_sata_phy_set_speed(struct phy *phy, int speed)
{
	struct hisi_ahci_sata_port *port = phy_get_drvdata(phy);
	struct hisi_ahci_sata_phy *priv = port->priv;
	void __iomem *reg = priv->ports + port->idx * SATA_PORT_STRIDE +
			    SATA_PORT_PHYCTL;

	/* 0 is not "no limit" here; the caller passes the highest speed instead */
	if (speed <= 0)
		return -EINVAL;

	mutex_lock(&priv->lock);
	port->gen2 = speed >= SATA_GEN2_SPEED_MBPS;
	if (priv->users)
		writel(hisi_ahci_sata_phy_portctl(priv, port->idx), reg);
	mutex_unlock(&priv->lock);

	return 0;
}

static int hisi_ahci_sata_phy_exit(struct phy *phy)
{
	struct hisi_ahci_sata_port *port = phy_get_drvdata(phy);
	struct hisi_ahci_sata_phy *priv = port->priv;
	int ret = 0;

	mutex_lock(&priv->lock);
	if (priv->users && !--priv->users) {
		ret = reset_control_assert(priv->rst);
		if (priv->cfg->power_off)
			priv->cfg->power_off(priv);
	}
	mutex_unlock(&priv->lock);

	return ret;
}

static const struct phy_ops hisi_ahci_sata_phy_ops = {
	.init		= hisi_ahci_sata_phy_init,
	.exit		= hisi_ahci_sata_phy_exit,
	.set_speed	= hisi_ahci_sata_phy_set_speed,
	.owner		= THIS_MODULE,
};

static void __iomem *hisi_ahci_sata_phy_map(struct platform_device *pdev,
					    const char *name,
					    resource_size_t min_size,
					    resource_size_t *size)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *base;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (!res)
		return IOMEM_ERR_PTR(dev_err_probe(dev, -EINVAL,
						   "missing %s registers\n", name));
	if (resource_size(res) < min_size)
		return IOMEM_ERR_PTR(dev_err_probe(dev, -EINVAL,
						   "%s window too small\n", name));

	/*
	 * The registers belong to the AHCI window, which the AHCI driver has
	 * requested: map them without claiming the region.
	 */
	base = devm_ioremap(dev, res->start, resource_size(res));
	if (!base)
		return IOMEM_ERR_PTR(-ENOMEM);
	if (size)
		*size = resource_size(res);

	return base;
}

/*
 * Read one optional per-port tuning property: one entry for every port or a
 * single entry for all of them. The values are raw hardware codes, checked
 * against the largest valid one.
 */
static int hisi_ahci_sata_phy_read_tuning(struct device *dev, const char *name,
					  unsigned int nports, u32 max, u32 *val)
{
	u32 tmp[SATA_MAX_PORTS];
	unsigned int i;
	int n, ret;

	if (!device_property_present(dev, name))
		return 0;

	n = device_property_count_u32(dev, name);
	if (n < 0)
		return dev_err_probe(dev, n, "cannot read %s\n", name);
	if (n != 1 && n != nports)
		return dev_err_probe(dev, -EINVAL,
				     "%s has %d entries, need 1 or %u (one per port)\n",
				     name, n, nports);

	ret = device_property_read_u32_array(dev, name, tmp, n);
	if (ret)
		return dev_err_probe(dev, ret, "cannot read %s\n", name);

	for (i = 0; i < nports; i++) {
		u32 v = tmp[n == 1 ? 0 : i];

		if (v > max)
			return dev_err_probe(dev, -EINVAL,
					     "%s entry %u is %u, the maximum is %u\n",
					     name, n == 1 ? 0 : i, v, max);
		val[i] = v;
	}

	return 0;
}

/*
 * Build the PHYCTL value of every port: the reference value of the SoC with
 * the optional per-port TX_BOOST, TX_ATTEN and RX_EQ_VAL of the device tree.
 * The speed bits (GEN2_EN) select the link speed and are not part of the
 * tuning: the driver owns them, so they are cleared here and set by
 * hisi_ahci_sata_phy_portctl().
 */
static int hisi_ahci_sata_phy_get_portctl(struct device *dev,
					  const struct hisi_ahci_sata_phy_cfg *cfg,
					  unsigned int nports, u32 *portctl)
{
	u32 boost[SATA_MAX_PORTS], atten[SATA_MAX_PORTS], eq[SATA_MAX_PORTS];
	unsigned int i;
	int ret;

	if (!cfg->tuning) {
		for (i = 0; i < nports; i++)
			portctl[i] = cfg->portctl & ~cfg->gen2_bits;

		return 0;
	}

	for (i = 0; i < nports; i++) {
		boost[i] = FIELD_GET(SATA_PORT_PHYCTL_TX_BOOST, cfg->portctl);
		atten[i] = FIELD_GET(SATA_PORT_PHYCTL_TX_ATTEN, cfg->portctl);
		eq[i] = FIELD_GET(SATA_PORT_PHYCTL_RX_EQ_VAL, cfg->portctl);
	}

	ret = hisi_ahci_sata_phy_read_tuning(dev, "hisilicon,tx-boost", nports,
					     SATA_TX_BOOST_MAX, boost);
	if (ret)
		return ret;
	ret = hisi_ahci_sata_phy_read_tuning(dev, "hisilicon,tx-atten", nports,
					     SATA_TX_ATTEN_MAX, atten);
	if (ret)
		return ret;
	ret = hisi_ahci_sata_phy_read_tuning(dev, "hisilicon,rx-eq", nports,
					     SATA_RX_EQ_MAX, eq);
	if (ret)
		return ret;

	for (i = 0; i < nports; i++) {
		u32 val = cfg->portctl & ~(cfg->gen2_bits |
					   SATA_PORT_PHYCTL_TX_BOOST |
					   SATA_PORT_PHYCTL_TX_ATTEN |
					   SATA_PORT_PHYCTL_RX_EQ_VAL);

		val |= FIELD_PREP(SATA_PORT_PHYCTL_TX_BOOST, boost[i]) |
		       FIELD_PREP(SATA_PORT_PHYCTL_TX_ATTEN, atten[i]) |
		       FIELD_PREP(SATA_PORT_PHYCTL_RX_EQ_VAL, eq[i]);
		portctl[i] = val;
	}

	return 0;
}

static struct phy *hisi_ahci_sata_phy_xlate(struct device *dev,
					    const struct of_phandle_args *args)
{
	struct hisi_ahci_sata_phy *priv = dev_get_drvdata(dev);

	if (args->args_count != 1 || args->args[0] >= priv->nports)
		return ERR_PTR(-ENODEV);

	return priv->phys[args->args[0]];
}

static int hisi_ahci_sata_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *provider;
	struct hisi_ahci_sata_phy *priv;
	resource_size_t size;
	struct phy *phy;
	unsigned int i;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	platform_set_drvdata(pdev, priv);
	priv->cfg = device_get_match_data(dev);
	if (!priv->cfg)
		return -EINVAL;

	priv->ctrl = hisi_ahci_sata_phy_map(pdev, "ctrl",
					    priv->cfg->ctrl_size, NULL);
	if (IS_ERR(priv->ctrl))
		return PTR_ERR(priv->ctrl);

	priv->ports = hisi_ahci_sata_phy_map(pdev, "ports", SATA_PORT_STRIDE,
					     &size);
	if (IS_ERR(priv->ports))
		return PTR_ERR(priv->ports);
	if (size % SATA_PORT_STRIDE)
		return dev_err_probe(dev, -EINVAL,
				     "ports window is not a multiple of 0x%x\n",
				     SATA_PORT_STRIDE);
	priv->nports = size / SATA_PORT_STRIDE;
	if (priv->nports > SATA_MAX_PORTS)
		return dev_err_probe(dev, -EINVAL, "ports window too large\n");

	ret = hisi_ahci_sata_phy_get_portctl(dev, priv->cfg, priv->nports,
					     priv->portctl);
	if (ret)
		return ret;

	ret = priv->cfg->get_resources(priv);
	if (ret)
		return ret;

	priv->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(priv->rst))
		return dev_err_probe(dev, PTR_ERR(priv->rst),
				     "cannot get the PHY reset\n");

	mutex_init(&priv->lock);
	for (i = 0; i < priv->nports; i++) {
		priv->port[i].priv = priv;
		priv->port[i].idx = i;
		priv->port[i].gen2 = true;

		phy = devm_phy_create(dev, NULL, &hisi_ahci_sata_phy_ops);
		if (IS_ERR(phy))
			return dev_err_probe(dev, PTR_ERR(phy),
					     "cannot create the PHY of port %u\n", i);
		phy_set_drvdata(phy, &priv->port[i]);
		priv->phys[i] = phy;
	}

	provider = devm_of_phy_provider_register(dev, hisi_ahci_sata_phy_xlate);

	return PTR_ERR_OR_ZERO(provider);
}

/*
 * Hi3536D V100: the PHY reference clock (the CRG clock driver selects its
 * parent with the assigned-clock-parents of the node), the lane reset and the
 * four controller resets (PERI_CRG45 bits 8 to 11) are handled here. The
 * vendor sequence (phy-hi3536dv100-sata.c, hisi_sata_phy_init()) asserts the
 * controller resets, then the PHY reset, selects and enables the clocks, waits
 * 20 ms, releases the PHY reset, waits 20 ms, releases the controller resets,
 * waits 20 ms and only then writes the registers behind them.
 *
 * The controller clocks and resets belong to the AHCI node, whose core enables
 * the clocks and releases the resets before it initializes the PHY, and which
 * asserts them again when it disables its resources. A boot loader that ran
 * SATA leaves the resets released, so that release is no reset at all. The PHY
 * therefore holds the same four reset lines as shared controls (the core counts
 * the users of a shared line and hardware follows the first deassert and the
 * last assert): by the time it runs, the AHCI node has released them, so
 * asserting them here is the vendor's controller reset, and releasing them
 * after the PHY reset hands them back in the state the AHCI node expects, in
 * the vendor's order. The PHY must not leave them asserted, or the AHCI node
 * would assert a line whose count is already zero.
 */
static int hisi_ahci_sata_phy_get_resources_hi3536dv100(struct hisi_ahci_sata_phy *priv)
{
	static const char * const ctrl_rst_names[HI3536D_NUM_CTRL_RSTS] = {
		"ctrl-bus", "ctrl-alive", "ctrl-rx0", "ctrl-sata0",
	};
	struct device *dev = priv->dev;
	unsigned int i;
	int ret;

	priv->misc = syscon_regmap_lookup_by_phandle(dev->of_node,
						     "hisilicon,misc-ctrl");
	if (IS_ERR(priv->misc))
		return dev_err_probe(dev, PTR_ERR(priv->misc),
				     "cannot get MISC_CTRL\n");

	priv->ref_clk = devm_clk_get(dev, "ref");
	if (IS_ERR(priv->ref_clk))
		return dev_err_probe(dev, PTR_ERR(priv->ref_clk),
				     "cannot get the reference clock\n");

	for (i = 0; i < HI3536D_NUM_CTRL_RSTS; i++)
		priv->ctrl_rsts[i].id = ctrl_rst_names[i];
	ret = devm_reset_control_bulk_get_shared(dev, HI3536D_NUM_CTRL_RSTS,
						 priv->ctrl_rsts);
	if (ret)
		return dev_err_probe(dev, ret,
				     "cannot get the controller resets\n");

	return 0;
}

/* Hand the controller resets back to the AHCI node, in the state it expects */
static int hisi_ahci_sata_phy_release_hi3536dv100(struct hisi_ahci_sata_phy *priv)
{
	int ret;

	if (!priv->ctrl_held)
		return 0;

	ret = reset_control_bulk_deassert(HI3536D_NUM_CTRL_RSTS, priv->ctrl_rsts);
	if (ret)
		return dev_err_probe(priv->dev, ret,
				     "cannot release the controller resets\n");
	priv->ctrl_held = false;
	msleep(SATA_PHY_RESET_DELAY_MS);

	return 0;
}

static int hisi_ahci_sata_phy_power_on_hi3536dv100(struct hisi_ahci_sata_phy *priv)
{
	int ret;

	ret = reset_control_assert(priv->rst);
	if (ret)
		return ret;
	ret = reset_control_bulk_assert(HI3536D_NUM_CTRL_RSTS, priv->ctrl_rsts);
	if (ret)
		return dev_err_probe(priv->dev, ret,
				     "cannot assert the controller resets\n");
	priv->ctrl_held = true;
	ret = clk_prepare_enable(priv->ref_clk);
	if (ret) {
		hisi_ahci_sata_phy_release_hi3536dv100(priv);
		return dev_err_probe(priv->dev, ret,
				     "cannot enable the reference clock\n");
	}
	msleep(SATA_PHY_RESET_DELAY_MS);

	return 0;
}

static void hisi_ahci_sata_phy_power_off_hi3536dv100(struct hisi_ahci_sata_phy *priv)
{
	/* only an error path still holds them, the normal sequence released them */
	hisi_ahci_sata_phy_release_hi3536dv100(priv);
	clk_disable_unprepare(priv->ref_clk);
}

/*
 * The data sheet (SATA operating mode configuration) sets SATA_PHY_CTL1 bit 3
 * so that the PHY and the controller use opposite endianness. The vendor
 * driver also sets the same bit in the second PHY control register (0xb0)
 * and sets the reset-back mask bits 7:5 and clears bit 4 of 0xac, which the
 * data sheet does not describe; all three are read-modify-write here as in the
 * vendor driver, keeping their other bits. SATA_PHY_CTL0 and SATA_OOB_CTL are
 * not written by the vendor driver: they are only compared with their reset
 * values by the check.
 */
static void hisi_ahci_sata_phy_program_hi3536dv100(struct hisi_ahci_sata_phy *priv)
{
	u32 val;

	val = readl(priv->ctrl + HI3536D_PHY_CTL1);
	writel(val | HI3536D_PHY_CTL1_DATA_INVERT, priv->ctrl + HI3536D_PHY_CTL1);

	val = readl(priv->ctrl + HI3536D_PHY_CTL2);
	writel(val | HI3536D_PHY_CTL2_DATA_INVERT, priv->ctrl + HI3536D_PHY_CTL2);

	val = readl(priv->ctrl + HI3536D_PHY_RST_BACK_MASK);
	val |= HI3536D_PHY_RST_BACK_ALL;
	val &= ~HI3536D_PHY_RST_BACK_PHY0;
	writel(val, priv->ctrl + HI3536D_PHY_RST_BACK_MASK);
}

/* The PHY takes the analog parameters after its reset is released, as in the vendor driver */
static int hisi_ahci_sata_phy_finish_hi3536dv100(struct hisi_ahci_sata_phy *priv)
{
	static const u16 seq[] = {
		0x70f, 0x70f | HI3536D_MISC_STROBE, 0x70f, 0,	/* PLL and CDR rates */
		0x802, 0x802 | HI3536D_MISC_STROBE, 0x802, 0,	/* no spread spectrum */
	};
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(seq); i++) {
		ret = regmap_write(priv->misc, HI3536D_MISC_CTRL22, seq[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static void hisi_ahci_sata_phy_check_hi3536dv100(const struct hisi_ahci_sata_phy *priv)
{
	hisi_ahci_sata_phy_check_reg(priv, "PHY_CTL0",
				     priv->ctrl + HI3536D_PHY_CTL0,
				     HI3536D_PHY_CTL0_RESET, U32_MAX);
	hisi_ahci_sata_phy_check_reg(priv, "PHY_CTL1",
				     priv->ctrl + HI3536D_PHY_CTL1,
				     HI3536D_PHY_CTL1_DATA_INVERT,
				     HI3536D_PHY_CTL1_DATA_INVERT);
	hisi_ahci_sata_phy_check_reg(priv, "PHY_CTL2",
				     priv->ctrl + HI3536D_PHY_CTL2,
				     HI3536D_PHY_CTL2_DATA_INVERT,
				     HI3536D_PHY_CTL2_DATA_INVERT);
	hisi_ahci_sata_phy_check_reg(priv, "PHY_RST_BACK_MASK",
				     priv->ctrl + HI3536D_PHY_RST_BACK_MASK,
				     HI3536D_PHY_RST_BACK_ALL & ~HI3536D_PHY_RST_BACK_PHY0,
				     HI3536D_PHY_RST_BACK_ALL);
	hisi_ahci_sata_phy_check_reg(priv, "OOB_CTL",
				     priv->ctrl + HI3536D_OOB_CTL,
				     HI3536D_OOB_CTL_RESET, U32_MAX);
}

/* CTLL 0x880ec888, CTLH 0x2521, OOB 0x84060c15, FIFOTH 0x3f24, PHYCTL 0x0e676706 */
static const struct hisi_ahci_sata_phy_cfg hi3520dv100_cfg = {
	.ctrl_size = SATA_CTRL_WINDOW_MIN_SIZE,
	.get_resources = hisi_ahci_sata_phy_get_resources_hi3520dv100,
	.power_on = hisi_ahci_sata_phy_power_on_hi3520dv100,
	.program = hisi_ahci_sata_phy_program_hi3520dv100,
	.check = hisi_ahci_sata_phy_check_ctrl_hi3520dv100,
	.tuning = true,
	.fifoth_check = SATA_PORT_FIFOTH_CHECK_MASK,
	.portctl_check = SATA_PORT_PHYCTL_CHECK_MASK,
	.gen2_bits = SATA_PORT_PHYCTL_GEN2_EN,
	.ctll = FIELD_PREP_CONST(SATA_PHY_CTLL_MPLL_PRESCALE, 2) |
		FIELD_PREP_CONST(SATA_PHY_CTLL_MPLL_NCY, 4) |
		FIELD_PREP_CONST(SATA_PHY_CTLL_MPLL_PROP_CTL, 7) |
		FIELD_PREP_CONST(SATA_PHY_CTLL_TX_LVL, 0x0c) |
		FIELD_PREP_CONST(SATA_PHY_CTLL_LOS_LVL, 0x11) |
		FIELD_PREP_CONST(SATA_PHY_CTLL_ACJT_LVL, 2),
	.ctlh = SATA_PHY_CTLH_USE_REFCLK_ALT | SATA_PHY_CTLH_MPLL_SS_EN |
		FIELD_PREP_CONST(SATA_PHY_CTLH_CKO_WORD_CON, 2) |
		FIELD_PREP_CONST(SATA_PHY_CTLH_CKO_ALIVE_CON, 1) |
		SATA_PHY_CTLH_WIDE_XFACE,
	.oob = SATA_OOB_CTL_VALID |
	       FIELD_PREP_CONST(SATA_OOB_CTL_MIN_COMWAKE, 0x04) |
	       FIELD_PREP_CONST(SATA_OOB_CTL_MAX_COMWAKE, 0x06) |
	       FIELD_PREP_CONST(SATA_OOB_CTL_MIN_COMINIT, 0x0c) |
	       FIELD_PREP_CONST(SATA_OOB_CTL_MAX_COMINIT, 0x15),
	.fifoth = SATA_PORT_FIFOTH_BYPASS_ALL |
		  FIELD_PREP_CONST(SATA_PORT_FIFOTH_DMAC_RXFIFO_TH, 0x12) |
		  FIELD_PREP_CONST(SATA_PORT_FIFOTH_LINK_RXFIFO_TH, 4),
	.periphctrl3 = 0,
	.portctl = SATA_PORT_PHYCTL_CALIBRATED | SATA_PORT_PHYCTL_SPD_CHG_ACK |
		   SATA_PORT_PHYCTL_DP_RDY | SATA_PORT_PHYCTL_GEN2_EN |
		   FIELD_PREP_CONST(SATA_PORT_PHYCTL_LOS_CTL, 2) |
		   FIELD_PREP_CONST(SATA_PORT_PHYCTL_RX_DPLL_MODE, 3) |
		   FIELD_PREP_CONST(SATA_PORT_PHYCTL_RX_EQ_VAL, 5) |
		   SATA_PORT_PHYCTL_RX_TERM_EN |
		   FIELD_PREP_CONST(SATA_PORT_PHYCTL_TX_EDGERATE, 1) |
		   SATA_PORT_PHYCTL_TX_CKO_EN | SATA_PORT_PHYCTL_RX_ALIGN_EN |
		   FIELD_PREP_CONST(SATA_PORT_PHYCTL_TX_BOOST, 6),
};

/*
 * FIFOTH 0xfeed9f24, PHYCTL1 0x00050438, PHYCTL 0x0e180000 (1.5 Gbit/s) or
 * 0x0e390000 (3 Gbit/s). The vendor writes the same PHYCTL1 for both speeds.
 * There is no per-port tuning: the PHYCTL fields of the Hi3520D properties
 * are other fields here.
 */
static const struct hisi_ahci_sata_phy_cfg hi3536dv100_cfg = {
	.ctrl_size = HI3536D_OOB_CTL + sizeof(u32),
	.get_resources = hisi_ahci_sata_phy_get_resources_hi3536dv100,
	.power_on = hisi_ahci_sata_phy_power_on_hi3536dv100,
	.power_off = hisi_ahci_sata_phy_power_off_hi3536dv100,
	.release = hisi_ahci_sata_phy_release_hi3536dv100,
	.program = hisi_ahci_sata_phy_program_hi3536dv100,
	.finish = hisi_ahci_sata_phy_finish_hi3536dv100,
	.check = hisi_ahci_sata_phy_check_hi3536dv100,
	.fifoth = 0xfeed9f24,
	.fifoth_check = HI3536D_FIFOTH_CHECK_MASK,
	.has_phyctl1 = true,
	.phyctl1 = FIELD_PREP_CONST(HI3536D_PHYCTL1_MARGIN2, 1) |
		   FIELD_PREP_CONST(HI3536D_PHYCTL1_DEEMPH2, 1) |
		   FIELD_PREP_CONST(HI3536D_PHYCTL1_MARGIN1, 0) |
		   FIELD_PREP_CONST(HI3536D_PHYCTL1_DEEMPH1, 2) |
		   FIELD_PREP_CONST(HI3536D_PHYCTL1_MARGIN0, 3) |
		   FIELD_PREP_CONST(HI3536D_PHYCTL1_DEEMPH0, 2),
	.phyctl1_check = HI3536D_PHYCTL1_CHECK_MASK,
	.portctl = 0x0e180000,
	.portctl_check = HI3536D_PHYCTL_CHECK_MASK,
	.gen2_bits = HI3536D_PHYCTL_GEN2_EN | HI3536D_PHYCTL_GEN2_RSVD,
};

static const struct of_device_id hisi_ahci_sata_phy_of_match[] = {
	{ .compatible = "hisilicon,hi3520dv100-sata-phy",
	  .data = &hi3520dv100_cfg },
	{ .compatible = "hisilicon,hi3536dv100-sata-phy",
	  .data = &hi3536dv100_cfg },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_ahci_sata_phy_of_match);

static struct platform_driver hisi_ahci_sata_phy_driver = {
	.probe	= hisi_ahci_sata_phy_probe,
	.driver = {
		.name = "hisi-ahci-sata-phy",
		/*
		 * Unbinding the PHY while the AHCI driver uses it would free
		 * what phy_init() and phy_exit() use; the phys link and the
		 * module reference should prevent that already, this makes it
		 * certain.
		 */
		.suppress_bind_attrs = true,
		.of_match_table = hisi_ahci_sata_phy_of_match,
	},
};
module_platform_driver(hisi_ahci_sata_phy_driver);

MODULE_DESCRIPTION("HiSilicon AHCI SATA PHY driver");
MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_LICENSE("GPL");
