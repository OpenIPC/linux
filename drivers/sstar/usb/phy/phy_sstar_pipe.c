/*
 * phy_sstar_pipe.c- Sigmastar
 *
 * Copyright (c) [2019~2021] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
//#include <linux/resource.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
//#include <registers.h>
#include <io.h>
#include "phy_sstar_u3phy.h"
#include "phy_sstar_port.h"
#include "phy_sstar_pipe_debugfs.h"
#include "phy_sstar_pipe_reg.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

static uint tx_idrv = 0x0;
module_param(tx_idrv, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tx_idrv, "driver current, in decimal format");

static uint tx_idem = 0x0;
module_param(tx_idem, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tx_idem, "dem current, in decimal format");

static uint tx_biasi = 0x0;
module_param(tx_biasi, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tx_biasi, "VCM current, in decimal format");

void sstar_pipe_sw_reset(struct sstar_phy_port *port);

void sstar_phy_pipe_enable_ssc(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    // U3 S(spread)S(spectrum)C(clock) setting

    if (device_property_read_bool(port->dev, "sstar,spread-spectrum-clock-enabled"))
    {
        OUTREG16(pipe_phya0 + (0x40 << 2), 0x55ff);
        OUTREG16(pipe_phya0 + (0x41 << 2), 0x002c);
        OUTREG16(pipe_phya0 + (0x42 << 2), 0x000a);
        OUTREG16(pipe_phya0 + (0x43 << 2), 0x0271);
        OUTREG16(pipe_phya0 + (0x44 << 2), 0x0001);

        dev_info(port->dev, "%s, spread spectrum clock enabled.\n", __func__);
    }
}

void sstar_pipe_tx_polarity_inv_disabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    // CLRREG16(base + PHYD_REG_RG_TX_POLARTY_INV, BIT_TX_POLARTY_INV_EN);
    OUTREG16(pipe_phyd + (0x12 << 2), 0x420f);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_LPFS_det_power_on(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    CLRREG16(pipe_phyd + (0x34 << 2), 0x4000); // RG_SSUSB_LFPS_PWD[14] = 0 // temp add here
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_deassert_hw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    OUTREG16(pipe_phya1 + (0x00 << 2), 0x0001);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_assert_sw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    CLRREG16(pipe_phya1 + (0x00 << 2), 0x0010);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_deassert_sw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    SETREG16(pipe_phya1 + (0x00 << 2), 0x0010);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_sw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    // OUTREG16(base + (0x06 << 2), 0x0003);
    SETREG16(pipe_phya0 + (0x06 << 2), 0x0001);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_synthesis_enabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    int           synth_clk;
    if (!device_property_read_u32(port->dev, "sstar,synthesiszer-clk", &synth_clk))
    {
        CLRREG16(pipe_phya0 + (0x44 << 2), 0x0001); // synth clk disabled
        mdelay(1);

        OUTREG16(pipe_phya0 + (0x41 << 2), synth_clk >> 16);    // synth clk
        OUTREG16(pipe_phya0 + (0x40 << 2), synth_clk & 0xffff); // synth clk

        mdelay(1);
        SETREG16(pipe_phya0 + (0x44 << 2), 0x0001); // synth clk enabled
        dev_dbg(port->dev, "%s completed\n", __func__);
    }
}

void sstar_pipe_tx_loop_div(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);

    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
        OUTREG16(pipe_phya0 + (0x21 << 2), 0x0005);
    else
        OUTREG16(pipe_phya0 + (0x21 << 2), 0x0010); // txpll loop div second
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_tx_post_div_and_test(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);

    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
        OUTREG16(pipe_phya0 + (0x11 << 2), 0x0a00); /// tx post loop div for rx ref
    else
        OUTREG16(pipe_phya0 + (0x11 << 2), 0x2a00); // gcr_sata_test
    OUTREG16(pipe_phya0 + (0x10 << 2), 0x0000);     // gcr_sata_test
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_EQ_rstep(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    OUTREG16(pipe_phyd + (0x42 << 2), 0x5582);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_DFE(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x0d << 2), 0x0);
    OUTREG16(pipe_phya2 + (0x0e << 2), 0x0);
    OUTREG16(pipe_phya2 + (0x0f << 2), 0x0171);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_sata_rx_tx_pll_frq_det_enabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    CLRREG16(pipe_phya0 + 0x54 * 4, 0xf); // The continue lock number to judge rxpll frequency is lock or not
    CLRREG16(pipe_phya0 + 0x73 * 4,
             0xff); // PLL State :
                    // The lock threshold distance between predict counter number and real counter number of rxpll
    CLRREG16(pipe_phya0 + 0x77 * 4,
             0xff); // CDR State :
                    // The lock threshold distance between predict counter number and real counter number of rxpll
    CLRREG16(pipe_phya0 + 0x56 * 4, 0xffff);
    SETREG16(pipe_phya0 + 0x56 * 4, 0x5dc0); // the time out reset reserve for PLL unlock for cdr_pd fsm PLL_MODE state
                                             // waiting time default : 20us  (24MHz base : 480 * 41.667ns)

    CLRREG16(pipe_phya0 + 0x57 * 4, 0xffff);
    SETREG16(pipe_phya0 + 0x57 * 4, 0x1e0); // the time out reset reserve for CDR unlock

    CLRREG16(pipe_phya0 + 0x70 * 4, 0x04); // reg_sata_phy_rxpll_det_hw_mode_always[2] = 0
    // Enable RXPLL frequency lock detection
    SETREG16(pipe_phya0 + 0x70 * 4, 0x02); // reg_sata_phy_rxpll_det_sw_enable_always[1] = 1
    // Enable TXPLL frequency lock detection
    SETREG16(pipe_phya0 + 0x60 * 4, 0x02);   // reg_sata_phy_txpll_det_sw_enable_always[1] = 1
    SETREG16(pipe_phya0 + 0x50 * 4, 0x2000); // cdr state change to freq_unlokc_det[13] = 1
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_txpll_vco_ldo(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x17 << 2), 0x0180);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_txpll_en_d2s(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x1c << 2), 0x000f);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_txpll_en_vco(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x1b << 2), 0x0100);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_txpll_ictrl_new(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x15 << 2), 0x0020);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rxpll_en_d2s(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x21 << 2), 0x000f);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rxpll_ctrk_r2(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x08 << 2), 0x0000);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rxpll_d2s_ldo_ref(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x06 << 2), 0x6060);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rxpll_vote_sel(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x05 << 2), 0x0200);
    dev_dbg(port->dev, "%s completed\n", __func__);
}
void sstar_pipe_rx_ictrl(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    OUTREG16(pipe_phya0 + (0x09 << 2), 0x0002);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rx_ictrl_pll(void __iomem *base)
{
    OUTREG16(base + (0x30 << 2), 0x100f);
}

void sstar_pipe_rx_div_setting(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    // OUTREG16(base + PHYA0_REG_SATA_RXPLL, 0x100e); // rxoll_lop_div_first
    OUTREG16(pipe_phya0 + (0x30 << 2), 0x1009);
    OUTREG16(pipe_phya0 + (0x31 << 2), 0x0005); // rxpll_loop_div_second
    OUTREG16(pipe_phya0 + (0x33 << 2), 0x0505);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rxpll_ictrl_CDR(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x09 << 2), 0x0001);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_rxpll_vco_ldo_voltage(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x06 << 2), 0x6060);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

__maybe_unused static void sstar_pipe_force_IDRV_6DB(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    SETREG16(pipe_phyd + 0xa * 4, 0x0020); // IDRV_6DB register force setting
    CLRREG16(pipe_phyd + (0x41 << 2), 0x00fc);
    SETREG16(pipe_phyd + (0x41 << 2), 0x00C8); // IDRV_6DB register setting
    dev_dbg(port->dev, "%s completed\n", __func__);
}

static void sstar_phy_pipe_tx_swing_and_de_emphasis(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd, *pipe_phya2;
    int           tx_swing[3];

    pipe_phyd  = port->reg;
    pipe_phya2 = port->reg + (0x200 * 3);

    if (device_property_read_u32_array(port->dev, "sstar,tx-swing-and-de-emphasis", tx_swing, ARRAY_SIZE(tx_swing)))
    {
        return;
    }

    if (tx_idrv != 0)
        tx_swing[0] = tx_idrv;

    if (tx_idem != 0)
        tx_swing[1] = tx_idem;

    if (tx_biasi != 0)
        tx_swing[2] = tx_biasi;

    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
        SETREG16(pipe_phyd + (0x0a << 2), BIT(5) | BIT(6));
    else
        SETREG16(pipe_phyd + (0x0a << 2), BIT(6));
    CLRREG16(pipe_phyd + (0x41 << 2), 0x00fc);
    // SETREG16(pipe_phyd + (0x41 << 2), 0x00c8);
    SETREG16(pipe_phyd + (0x41 << 2), 0x00fc & (tx_swing[0] << 2));
    CLRREG16(pipe_phyd + (0x41 << 2), 0x3f00);
    // SETREG16(pipe_phyd + (0x41 << 2), 0x2800);
    SETREG16(pipe_phyd + (0x41 << 2), 0x3f00 & (tx_swing[1] << 8));

    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
    {
        SETREG16(pipe_phyd + (0x0a << 2), BIT(0) | BIT(1));
        SETREG16(pipe_phyd + (0x26 << 2), BIT(0));
        CLRREG16(pipe_phyd + (0x26 << 2), BIT(3) | BIT(2) | BIT(1));
        // SETREG16(pipe_phyd + (0x41 << 2), 0x00c8);
        SETREG16(pipe_phyd + (0x26 << 2), 0x000e & (tx_swing[2] << 1));
    }
    else
    {
        SETREG16(pipe_phya2 + (0x0f << 2), BIT(12));
        CLRREG16(pipe_phya2 + (0x11 << 2), 0xf000);
        // SETREG16(pipe_phya2 + (0x11 << 2), 0xd000);
        SETREG16(pipe_phya2 + (0x11 << 2), 0xf000 & (tx_swing[2] << 12));
    }

    SETREG16(pipe_phya2 + (0x2f << 2), BIT(4));

    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_tx_swing_setting(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    // sstar_phy_pipe_tx_swing_and_de_emphasis(port);

    OUTREG16(pipe_phyd + (0x2f << 2), 0x241d);

    SETREG16(pipe_phyd + (0x0b << 2), 0x0050);

    CLRREG16(pipe_phyd + (0x2a << 2), BITS_TX_RTERM(0x1F));
    SETREG16(pipe_phyd + (0x2a << 2), BITS_TX_RTERM(0x10));
    sstar_phy_pipe_tx_swing_and_de_emphasis(port);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_tx_ibiasi(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG8(pipe_phya2 + ((0x11 << 2) + 1), 0x70);
    dev_dbg(port->dev, "%s completed\n", __func__);
}
/*
void sstar_pipe_ibias_trim(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    OUTREG16(pipe_phyd + (0x46 << 2), 0x2802);
    OUTREG16(pipe_phyd + (0x50 << 2), 0x0300);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

void sstar_pipe_rx_imp_sel(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    OUTREG16(pipe_phyd + (0x0e << 2), 0x8904);
    OUTREG16(pipe_phyd + (0x3c << 2), 0x0100);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}
*/
static void sstar_pipe_sync_trim_value(struct sstar_phy_port *port)
{
    // int value;
    struct regmap *otp;
    void __iomem * pipe_phyd = port->reg;

    int otp_val, tx_r50, rx_r50, tx_ibias;
    int flag;

    otp = syscon_regmap_lookup_by_phandle(port->dev->of_node, "sstar,otp-syscon");

    if (!IS_ERR(otp))
    {
        if (of_device_is_compatible(port->dev->of_node, "sstar,infinity7-pipe"))
        {
            tx_r50   = 0x10;
            rx_r50   = 0x10;
            tx_ibias = 0x00;

            regmap_read(otp, (0x5e << 2), &flag);
            if (flag & BIT(14))
            {
                regmap_read(otp, (0x5d << 2), &otp_val);
                tx_r50 = otp_val >> 14;
                regmap_read(otp, (0x5e << 2), &otp_val);
                tx_r50 |= (otp_val << 2);
                tx_r50 &= 0x1f;
                rx_r50   = (otp_val >> 3) & 0x1f;
                tx_ibias = (otp_val >> 8) & 0x3f;
            }

            SETREG16(pipe_phyd + (0x0b << 2), BIT(4));
            SETREG16(pipe_phyd + (0x0e << 2), BIT(2));
            SETREG16(pipe_phyd + (0x46 << 2), BIT(13));

            CLRREG16(pipe_phyd + (0x2a << 2), 0x0f80);
            SETREG16(pipe_phyd + (0x2a << 2), tx_r50 << 7);
            CLRREG16(pipe_phyd + (0x3c << 2), 0x01f0);
            SETREG16(pipe_phyd + (0x3c << 2), rx_r50 << 4);
            CLRREG16(pipe_phyd + (0x50 << 2), 0xfc00);
            SETREG16(pipe_phyd + (0x50 << 2), tx_ibias << 10);
            dev_dbg(port->dev, "%s sync trimed value manually\n", __func__);
        }
    }
    dev_dbg(port->dev, "%s tx_r50 = 0x%04x\n", __func__, INREG16(pipe_phyd + (0x2a << 2)) & 0x0f80);
    dev_dbg(port->dev, "%s rx_r50 = 0x%04x\n", __func__, INREG16(pipe_phyd + (0x3c << 2)) & 0x01f0);
    dev_dbg(port->dev, "%s tx_ibias = 0x%04x\n", __func__, INREG16(pipe_phyd + (0x50 << 2)) & 0xfc00);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_set_phya1(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);

    SETREG16(pipe_phya1 + 0x20 * 4, 0x04); // guyo
    CLRREG16(pipe_phya1 + 0x25 * 4, 0xffff);

    CLRREG16(pipe_phya1 + 0x49 * 4, 0x200);
    SETREG16(pipe_phya1 + 0x49 * 4, 0xc4e); // reg_rx_lfps_t_burst_gap = 3150
    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
    {
        CLRREG16(pipe_phya1 + 0x51 * 4, 0x001f);
        SETREG16(pipe_phya1 + 0x51 * 4, 0x0004);
    }
    dev_dbg(port->dev, "%s completed\n", __func__);
}

void sstar_pipe_eco_enabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);

    // Enable ECO
    CLRREG16(pipe_phya0 + 0x03 * 4, 0x0f);
    SETREG16(pipe_phya0 + 0x03 * 4, 0x0d);
    dev_dbg(port->dev, "%s completed\n", __func__);
}

static int sstar_pipe_init(struct phy *phy)
{
    struct sstar_phy_port *port = phy_get_drvdata(phy);
    void __iomem *         pipe_phyd, *pipe_phya0, *pipe_phya1, *pipe_phya2;

    pipe_phyd  = port->reg;
    pipe_phya0 = pipe_phyd + (0x200 * 1);
    pipe_phya1 = pipe_phyd + (0x200 * 2);
    pipe_phya2 = pipe_phyd + (0x200 * 3);

    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
    {
        dev_dbg(&phy->dev, "%s use generic-pipe setting.\n", __func__);
    }

    // sstar_pipe_deassert_hw_reset(port);
    sstar_pipe_sw_reset(port);
    sstar_pipe_synthesis_enabled(port);

    if (of_device_is_compatible(port->dev->of_node, "sstar,infinity7-pipe"))
    {
        /* EQ default */
        CLRREG16(pipe_phya1 + (0x51 << 2), (BIT(5) - 1));
        SETREG16(pipe_phya1 + (0x51 << 2), BIT(4));
    }

    sstar_pipe_eco_enabled(port);
    // leq_setting(port);
    sstar_pipe_tx_polarity_inv_disabled(port);
    sstar_pipe_tx_loop_div(port);
    sstar_pipe_tx_post_div_and_test(port);
    sstar_pipe_rx_div_setting(port);
    sstar_pipe_rxpll_ictrl_CDR(port);
    sstar_pipe_rxpll_vco_ldo_voltage(port);

    sstar_pipe_EQ_rstep(port);

    sstar_pipe_DFE(port);

    sstar_pipe_rxpll_d2s_ldo_ref(port);

    sstar_pipe_rxpll_vote_sel(port);

    sstar_pipe_rx_ictrl(port);

    sstar_pipe_txpll_vco_ldo(port);

    sstar_pipe_txpll_en_d2s(port);

    // mdelay(1);
    sstar_pipe_txpll_en_vco(port);

    // mdelay(1);
    sstar_pipe_rxpll_en_d2s(port);

    // mdelay(1);
    sstar_pipe_txpll_ictrl_new(port);

    sstar_pipe_rxpll_ctrk_r2(port);

    sstar_pipe_tx_swing_setting(port);
    if (of_device_is_compatible(port->dev->of_node, "sstar,generic-pipe"))
        sstar_pipe_tx_ibiasi(port);

    sstar_sata_rx_tx_pll_frq_det_enabled(port);

    sstar_pipe_set_phya1(port);
    SETREG16(pipe_phya0 + (0x58 * 4), 0x0003);

    /* for I7U02 LFPS setting*/
    CLRREG16(pipe_phya2 + (0x2f << 2), BIT(8) | BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3));
    SETREG16(pipe_phya2 + (0x2f << 2), 0x28 << 3);

    dev_info(&phy->dev, "%s completed\n", __func__);
    return 0;
}

static int sstar_pipe_exit(struct phy *phy)
{
    dev_info(&phy->dev, "%s completed\n", __func__);
    return 0;
}

static int sstar_pipe_reset(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);

    sstar_pipe_assert_sw_reset(u3phy_port);
    udelay(10);
    sstar_pipe_deassert_sw_reset(u3phy_port);
    dev_info(&phy->dev, "%s completed\n", __func__);

    return 0;
}

static int sstar_pipe_power_on(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);

    void __iomem *pipe_phya0 = u3phy_port->reg + (0x200 * 1);
    void __iomem *pipe_phya2 = u3phy_port->reg + (0x200 * 3);
    sstar_pipe_sync_trim_value(u3phy_port);
    sstar_phy_pipe_enable_ssc(u3phy_port);
    sstar_pipe_deassert_sw_reset(u3phy_port);

    SETREG16(pipe_phya2 + (0x16 << 2), BIT(14)); // txpll en
    SETREG16(pipe_phya0 + (0x15 << 2), BIT(6));  // rxpll en

    OUTREG16(pipe_phya0 + PHYA0_REG_SATA_PD_TXPLL, 0x0100);
    OUTREG16(pipe_phya0 + PHYA0_REG_SATA_PD_RXPLL, 0x100e);

    sstar_pipe_LPFS_det_power_on(u3phy_port);
    dev_info(&phy->dev, "%s completed\n", __func__);
    return 0;
}

static int sstar_pipe_power_off(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);

    void __iomem *pipe_phya0, *pipe_phya2;
    pipe_phya0 = u3phy_port->reg + (0x200 * 1);
    pipe_phya2 = u3phy_port->reg + (0x200 * 3);
    // base = re->pipe_phyd;
    // SETREG16(base + PHYD_REG_RG_TX_POLARTY_INV, BIT_TX_POLARTY_INV_EN);

    SETREG16(pipe_phya0 + PHYA0_REG_SATA_PD_TXPLL, BIT_TXPLL_PD_EN);
    SETREG16(pipe_phya0 + PHYA0_REG_SATA_PD_RXPLL, BIT_RXPLL_PD_EN);

    CLRREG16(pipe_phya2 + (0x1C << 2), 0x0F);
    CLRREG16(pipe_phya2 + (0x1B << 2), BIT(8));
    CLRREG16(pipe_phya2 + (0x21 << 2), 0x0F);

    dev_info(&phy->dev, "%s completed\n", __func__);
    return 0;
}

const struct phy_ops sstar_pipe_ops = {
    .init      = sstar_pipe_init,
    .exit      = sstar_pipe_exit,
    .power_on  = sstar_pipe_power_on,
    .power_off = sstar_pipe_power_off,
    .reset     = sstar_pipe_reset,
    .owner     = THIS_MODULE,
};

int sstar_pipe_port_init(struct device *dev, struct sstar_phy_port *phy_port, struct device_node *np)
{
    int ret;
    // sstar_port_init(dev, np, &sstar_pipe_ops, phy_port);
    // sstar_pipe_init(phy_port->phy);
    // sstar_pipe_power_on(phy_port->phy);
    phy_port->speed = USB_SPEED_SUPER;
    ret             = sstar_port_init(dev, np, &sstar_pipe_ops, phy_port);

    if (!ret)
    {
        sstar_phy_pipe_debugfs_init(phy_port);
    }

    return ret;
}
EXPORT_SYMBOL(sstar_pipe_port_init);

void sstar_pipe_port_remove(void *data)
{
    struct sstar_phy_port *phy_port = data;

    sstar_phy_pipe_debugfs_exit(phy_port);
    sstar_port_deinit(phy_port);
    return;
}
EXPORT_SYMBOL(sstar_pipe_port_remove);
