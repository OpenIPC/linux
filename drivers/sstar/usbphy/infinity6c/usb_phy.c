/*
 * usb_phy.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
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

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/regmap.h>
#include <registers.h>
#include <ms_platform.h>
#include "usb_phy.h"

#define DEBUG_BUS         0
#define BANK_TO_ADDR32(b) (b << 9)
#define UTMI_BASE_ADDR    BASE_REG_UTMI0_PA

static void trim_ic_verify(void)
{
    u16 val, tmp;

    val = (INREG16(GET_REG_ADDR(0x1f203000, 0x5D)) >> 11) & 0x001F;
    tmp = INREG16(GET_REG_ADDR(0x1f203000, 0x5E)) & 0x0001;
    val = val | (tmp << 5);

    if (val != ((INREG16(0x1f2a4600 + 0x50 * 4) & 0xfc00) >> 10))
    {
        goto not_trim;
    }

    val = (INREG16(GET_REG_ADDR(0x1f203000, 0x5D)) >> 6) & 0x001F;

    if (val != ((INREG16(0x1f2a4600 + 0x3c * 4) & 0x01f0) >> 4))
    {
        goto not_trim;
    }

    val = (INREG16(GET_REG_ADDR(0x1f203000, 0x5D)) >> 1) & 0x001F;

    if (val != ((INREG16(0x1f2a4600 + 0x2a * 4) & 0x0f80) >> 7))
    {
        goto not_trim;
    }

    printk("IC is Trim\n");
    return;
not_trim:
    printk("IC is not Trim, apply default setting\n");
    CLRREG16(0x1f2a4600 + 0x2a * 4, 0x1f << 7);
    SETREG16(0x1f2a4600 + 0x2a * 4, 0x800);

    CLRREG16(0x1f2a4600 + 0x3c * 4, 0x1f << 4);
    SETREG16(0x1f2a4600 + 0x3c * 4, 0x100);

    CLRREG16(0x1f2a4600 + 0x50 * 4, 0x3f << 10);
    SETREG16(0x1f2a4600 + 0x50 * 4, 0x8000);
}

void sstar_usb_phy_utmi_reset(void)
{
    SETREG16(0x1f28420c, 0x0103); // bit0: RX sw reset; bit1: Tx sw reset; bit8: Tx FSM sw reset;
    SETREG16(0x1f284220, 0x1000); // bit12: pwr good reset
    mdelay(1);
    // clear reset
    CLRREG16(0x1f28420c, 0x0103); // bit0: RX sw reset; bit1: Tx sw reset; bit8: Tx FSM sw reset;
    CLRREG16(0x1f284220, 0x1000); // bit12: pwr good reset
}
EXPORT_SYMBOL(sstar_usb_phy_utmi_reset);

void sstar_usb_phy_utmi_deinit(struct phy *phy)
{
    sstar_usb_phy_utmi_reset();
    // power down utmi
    OUTREG16(0x1f284200, 0x7F03);
    mdelay(5);
}

void sstar_usb_phy_power_on(struct phy *phy)
{
    unsigned int u4IO_PHY_BASE;
    unsigned int u4phy_bank[3];
    unsigned int dphy_base, aphy_base0, aphy_base1;

    of_property_read_u32(phy->dev.of_node, "io_phy_addr", &u4IO_PHY_BASE);
    of_property_read_u32_array(phy->dev.of_node, "banks", (unsigned int *)u4phy_bank, 3);

    dphy_base  = BANK_TO_ADDR32(u4phy_bank[0]) + u4IO_PHY_BASE;
    aphy_base0 = BANK_TO_ADDR32(u4phy_bank[1]) + u4IO_PHY_BASE;
    aphy_base1 = BANK_TO_ADDR32(u4phy_bank[2]) + u4IO_PHY_BASE;

    // Tx polarity inverse
    SETREG16(dphy_base + 0x12 * 4, 0x1000); // RG_TX_POLARITY_INV[12] = 1
}

void sstar_usb_phy_switch_owner(int owner) // 0: switch to USB2.0; 1: switch to USB3.0
{
    if (owner)
    {
        CLRREG16(0x1F284204, 0x80);
    }
    else
    {
        SETREG16(0x1F284204, 0x80);
    }
}

void sstar_usb_phy_setting(struct phy *phy, struct tx_voltage_settings *tx_voltage_array, u8 tx_voltage_idx)
{
    unsigned int u4IO_PHY_BASE;
    unsigned int u4phy_bank[3];
    unsigned int dphy_base, aphy_base0, aphy_base1;

    of_property_read_u32(phy->dev.of_node, "io_phy_addr", &u4IO_PHY_BASE);
    of_property_read_u32_array(phy->dev.of_node, "banks", (unsigned int *)u4phy_bank, 3);

    dphy_base  = BANK_TO_ADDR32(u4phy_bank[0]) + u4IO_PHY_BASE;
    aphy_base0 = BANK_TO_ADDR32(u4phy_bank[1]) + u4IO_PHY_BASE;
    aphy_base1 = BANK_TO_ADDR32(u4phy_bank[2]) + u4IO_PHY_BASE;

    printk("Mercury6 USB PHY init, d-phy:%x, a-phy0:%x, a-phy1:%x\n", dphy_base, aphy_base0, aphy_base1);
    printk("-Tx cur[%d]: %s\n", tx_voltage_idx, tx_voltage_array[tx_voltage_idx].descript);

#if DEBUG_BUS
    OUTREG32((BANK_TO_ADDR32(0x1a21) + u4IO_PHY_BASE) + 0x1c * 4, 0x40000);
    SETREG16(aphy_base0 + 0x0d * 4, 0x100);
    ////SETREG16(aphy_base0 + 0x0c*4, 0x2);
    SETREG16(dphy_base + 0x44 * 4, 0x80);
    SETREG16(dphy_base + 0x25 * 4, 0x10);
    SETREG16((BANK_TO_ADDR32(0x1436) + u4IO_PHY_BASE) + 0x1 * 4, 0x8);
    SETREG16((BANK_TO_ADDR32(0x1433) + u4IO_PHY_BASE) + 0x20 * 4, 0x7);
    SETREG16((BANK_TO_ADDR32(0x101e) + u4IO_PHY_BASE) + 0x75 * 4, 0x4000);
    SETREG16((BANK_TO_ADDR32(0x101e) + u4IO_PHY_BASE) + 0x77 * 4, 0x1b);
    SETREG16((BANK_TO_ADDR32(0x101e) + u4IO_PHY_BASE) + 0x12 * 4, 0x10);
    // Done for debug test

    // ck debug
    // SETREG16((BANK_TO_ADDR32(0x1436)+u4IO_PHY_BASE) + 0x1*4, 0x8);
    // SETREG16((BANK_TO_ADDR32(0x1433)+u4IO_PHY_BASE) + 0x20*4, 0x7);
#endif

    SETREG16(0x1F284224, 0x8000);
    // Unmask USB30_gp2top interrupt
    CLRREG16(0x1F286684, 0x02);

    /* Sigmastar mercury6 USB3.0 PHY initialization */
    trim_ic_verify();
    /* Trim items provided by Dylan */
    // 1. TX R50
    SETREG16(dphy_base + 0x0b * 4, 0x10); // rg_force_tx_imp_sel
    // 2. RX R50
    SETREG16(dphy_base + 0x0e * 4, 0x04); // rg_force_rx_imp_sel
    // 3. BGR INTR
    SETREG16(dphy_base + 0x46 * 4, 0x2000); // rg_force_iext_intr_ctrl

    CLRREG16(dphy_base + 0x26 * 4, 0x0E);
    SETREG16(dphy_base + 0x26 * 4, tx_voltage_array[tx_voltage_idx].reg_biasi);

    CLRREG16(aphy_base0 + 0x44 * 4, 0x01);
    mdelay(1);
    CLRREG16(aphy_base0 + 0x40 * 4, 0xffff);
    SETREG16(aphy_base0 + 0x40 * 4, 0xb6a7);
    CLRREG16(aphy_base0 + 0x41 * 4, 0xff);
    SETREG16(aphy_base0 + 0x41 * 4, 0x1b);
    CLRREG16(aphy_base0 + 0x42 * 4, 0xfff);
    SETREG16(aphy_base0 + 0x42 * 4, 0x4);
    CLRREG16(aphy_base0 + 0x43 * 4, 0x7fff);
    SETREG16(aphy_base0 + 0x43 * 4, 0x3ee);
    SETREG16(aphy_base0 + 0x44 * 4, 0x110);
    mdelay(1);
    SETREG16(aphy_base0 + 0x44 * 4, 0x01);

    CLRREG16(dphy_base + 0x34 * 4, 0x4000); // RG_SSUSB_LFPS_PWD[14] = 0 // temp add here
    SETREG16(aphy_base1 + 0x20 * 4, 0x04);
    CLRREG16(aphy_base1 + 0x25 * 4, 0xffff);
    // Enable ECO
    CLRREG16(aphy_base0 + 0x03 * 4, 0x0f);
    SETREG16(aphy_base0 + 0x03 * 4, 0x0d);
    // Turn on TX PLL
    CLRREG16(aphy_base0 + 0x20 * 4, 0x01); // reg_sata_pd_txpll[0] = 0
    // Turn on RX PLL
    CLRREG16(aphy_base0 + 0x30 * 4, 0x01); // reg_sata_pd_rxpll[0] = 0
    mdelay(1);
    // De-assert USB PHY reset
    SETREG16(aphy_base1 + 0x00 * 4, 0x10); // reg_ssusb_phy_swrst[4] = 1
    // Toggle synthesizer to turn on PLL reference clock input
    SETREG16(aphy_base0 + 0x44 * 4, 0x01); // reg_sata_phy_synth_sld[0] = 1
    // Diable RXPLL frequency lock detection hardware mode
    OUTREG16(dphy_base + 0x40 * 4, ((tx_voltage_array[tx_voltage_idx].reg_dem & 0x0F) << 12)
                                       | (tx_voltage_array[tx_voltage_idx].reg_drv << 6) | 0x19);
    OUTREG16(dphy_base + 0x41 * 4, ((tx_voltage_array[tx_voltage_idx].reg_dem & 0x30) >> 4) | 0x2188);
    SETREG16(dphy_base + 0xa * 4, 0x1a); // force DA_SSUSB_TX_BIASI, DA_SSUSB_IDRV_3P5DB, DA_SSUSB_IDEM_3P5DB value

    CLRREG16(aphy_base0 + 0x54 * 4, 0xf); // The continue lock number to judge rxpll frequency is lock or not
    CLRREG16(aphy_base0 + 0x73 * 4,
             0xff); // PLL State :
                    // The lock threshold distance between predict counter number and real counter number of rxpll
    CLRREG16(aphy_base0 + 0x77 * 4,
             0xff); // CDR State :
                    // The lock threshold distance between predict counter number and real counter number of rxpll
    CLRREG16(aphy_base0 + 0x56 * 4, 0xffff);
    SETREG16(aphy_base0 + 0x56 * 4, 0x5dc0); // the time out reset reserve for PLL unlock for cdr_pd fsm PLL_MODE state
                                             // waiting time default : 20us  (24MHz base : 480 * 41.667ns)

    CLRREG16(aphy_base0 + 0x57 * 4, 0xffff);
    SETREG16(aphy_base0 + 0x57 * 4, 0x1e0); // the time out reset reserve for CDR unlock

    CLRREG16(aphy_base0 + 0x70 * 4, 0x04); // reg_sata_phy_rxpll_det_hw_mode_always[2] = 0
    // Enable RXPLL frequency lock detection
    SETREG16(aphy_base0 + 0x70 * 4, 0x02); // reg_sata_phy_rxpll_det_sw_enable_always[1] = 1
    // Enable TXPLL frequency lock detection
    SETREG16(aphy_base0 + 0x60 * 4, 0x02); // reg_sata_phy_txpll_det_sw_enable_always[1] = 1
    // Tx polarity inverse
    CLRREG16(aphy_base1 + 0x49 * 4, 0x200);
    SETREG16(aphy_base1 + 0x49 * 4, 0xc4e); // reg_rx_lfps_t_burst_gap = 3150
    SETREG16(dphy_base + 0x12 * 4, 0x1000); // RG_TX_POLARITY_INV[12] = 1

    ms_writew(0x0C2F, GET_REG_ADDR(UTMI_BASE_ADDR, 0x4));
#ifdef USB_ENABLE_UPLL
    ms_writew(0x6BC3,
              GET_REG_ADDR(UTMI_BASE_ADDR, 0)); // Turn on UPLL, reg_pdn: bit<9> reg_pdn: bit<15>, bit <2> ref_pdn
    mdelay(1);
    ms_writeb(0x69, GET_REG_ADDR(UTMI_BASE_ADDR, 0)); // Turn on UPLL, reg_pdn: bit<9>
    mdelay(2);
    ms_writew(0x0001, GET_REG_ADDR(UTMI_BASE_ADDR, 0)); // Turn all (including hs_current) use override mode
    // Turn on UPLL, reg_pdn: bit<9>
    mdelay(3);
#else
    // Turn on UTMI if it was powered down
    if (0x0001 != ms_readw(GET_REG_ADDR(UTMI_BASE_ADDR, 0)))
    {
        ms_writew(0x0001, GET_REG_ADDR(UTMI_BASE_ADDR, 0)); // Turn all (including hs_current) use override mode
        mdelay(3);
    }
#endif

    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x1E)) | 0x01),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x1E)); // set CA_START as 1
    mdelay(10);
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x1E)) & ~0x01),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x1E)); // release CA_START
    while (0 == (ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x1E)) & 0x02))
        ; // polling bit <1> (CA_END)

    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x03)) & 0x9F) | 0x40,
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x03)); // reg_tx_force_hs_current_enable
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x01) + 1) | 0x28),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x01) + 1); // Disconnect window select
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x01) + 1) & 0xef),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x01) + 1); // Disconnect window select
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x03) + 1) & 0xfd),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x03) + 1); // Disable improved CDR
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x04) + 1) | 0x81),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x04) + 1); // UTMI RX anti-dead-loc, ISI effect improvement
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x0A) + 1) | 0x20),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x0A) + 1); // Chirp signal source select
    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x05) + 1) | 0x80),
              GET_REG_ADDR(UTMI_BASE_ADDR, 0x05) + 1); // set reg_ck_inv_reserved[6] to solve timing problem

    ms_writew(0x0290, GET_REG_ADDR(UTMI_BASE_ADDR, 0x16));
    ms_writew(0x0110, GET_REG_ADDR(UTMI_BASE_ADDR, 0x17));

    ms_writeb((ms_readb(GET_REG_ADDR(UTMI_BASE_ADDR, 0x02)) | 0x80), GET_REG_ADDR(UTMI_BASE_ADDR, 0x02)); // avoid
                                                                                                          // glitch

    // 2020_0810: Chiyun.liu and TY request.
    SETREG16(0x1F000000 + (0x1524 << 9) + (0x11 << 2),
             0x0020); // bit5] = 1'b1 CDR JTOL Improvement (Improve BBPD of CDR if 16'h11 bit[5] = 1'b1. Default with
                      // MTK phyA mode.
    CLRREG16(0x1F000000 + (0x1523 << 9) + (0x67 << 2), 0x00C0); // bit[7:6] = 2'b00
    SETREG16(0x1F000000 + (0x1523 << 9) + (0x67 << 2), 0x0080); // bit[7] = 1'b1
    SETREG16(0x1F000000 + (0x1524 << 9) + (0x14 << 2), 0x6000); // bit[14:13] = 2'b11
    // LPFS filter threshold by CK
    SETREG16(0x1F000000 + (0x1523 << 9) + (0x29 << 2), 0x07C0); // bit[10:6] = 5'b11111 LPFS filter threshold
}

void sstar_usb_phy_reset(void)
{
    CLRREG16(0x1F000000 + (0x1525 << 9) + (0x00 << 2), 0x0010); // reg_ssusb_phy_swrst[4] = 0
    SETREG16(0x1F000000 + (0x1525 << 9) + (0x00 << 2), 0x0010); // reg_ssusb_phy_swrst[4] = 1
}

void sstar_usb_disable_emphasis(void)
{
    CLRREG16(0x1F284200 + 0x16 * 4, 0x200);
}

void sstar_usb_disable_slew_rate(void)
{
    SETREG16(0x1F284200 + 0x16 * 4, 0xc);
}
