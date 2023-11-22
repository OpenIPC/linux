/*
 * phy_sstar_pipe_reg.h- Sigmastar
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

#ifndef __PHY_SSTAR_SSUSB_REG_H__
#define __PHY_SSTAR_SSUSB_REG_H__

#define PHYD_REG_RG_FORCE_IDRV_6DB            (0x0A << 2)
#define BIT_FORCE_IDRV_6DB                    BIT(5)
#define PHYD_REG_RG_FORCE_TX_IMPL_SEL         (0x0B << 2)
#define BIT_FORCE_TX_IMPL_SEL                 BIT(4)
#define PHYD_REG_RG_FORCE_RX_IMPLSEL          (0x0E << 2)
#define BIT_FORCE_RX_IMPLSEL                  BIT(2)
#define PHYD_REG_RG_TX_POLARTY_INV            (0x12 << 2)
#define BIT_TX_POLARTY_INV_EN                 BIT(12)
#define PHYD_REG_SSUSB_TX_IMP_SEL             (0x2A << 2)
#define BITS_TX_RTERM(x)                      ((x & (BIT(6) - 1)) << 7)
#define PHYD_REG_SSUSB_RX_IMPSEL              (0x3C << 2)
#define BITS_RX_RTERM(x)                      ((x & (BIT(6) - 1)) << 4)
#define PHYD_REG_SSUSB_6DB_GEN3               (0x41 << 2)
#define BITS_IDRV_6DB(x)                      ((x & (BIT(7) - 1)) << 2)
#define BITS_IDEM_6DB(x)                      ((x & (BIT(7) - 1)) << 8)
#define PHYD_REG_RG_SSUSB_TX_BIASI            (0x26 << 2)
#define BIT_DISABLE_TX_DRIVER_BIASI_FROM_3_3V BIT(0)
#define BITS_TX_DRIVER_BIAS_I_CTRL(x)         (x << 1)
#define PHYD_REG_RG_RESERVED_OTP3             (0x46 << 2)
#define BIT_FORCE_IBIAS_TRIM                  BIT(13)
#define PHYD_REG_RG_SSUSB_IEXT_INTR_CTRL      (0x50 << 2)
#define BITS_SSUSB_IEXT_SEL(x)                ((x & (BIT(7) - 1)) << 10)

#define PHYA0_REG_SATA_TEST1               (0x10 << 2)
#define PHYA0_REG_SATA_TEST2               (0x11 << 2)
#define PHYA0_REG_SATA_PD_TXPLL            (0x20 << 2)
#define BIT_TXPLL_PD_EN                    BIT(0)
#define PHYA0_REG_SATA_TX_LOOP_DIV_SECOND  (0x21 << 2)
#define BITS_TXPLL_LOOP_DIV_SECOND_CTRL(x) (x & (BIT(8) - 1))
#define PHYA0_REG_SATA_RXPLL               (0x30 << 2)
#define BITS_RXPLL_ICTRL_PLL(x)            ((x & (BIT(3) - 1)) << 1)
#define BITS_RXPLL_LOOP_DIV_FIRST(x)       ((x & (BIT(3) - 1)) << 12)
#define PHYA0_REG_SATA_PD_RXPLL            (0x30 << 2)
#define BIT_RXPLL_PD_EN                    BIT(0)
#define PHYA0_REG_SATA_RXPLL_DIV           (0x31 << 2)
#define BITS_RXPLL_LOOP_DIV_SECOND(x)      (x & (BIT(8) - 1))
#define PHYA0_REG_SATA_RXPLL_MISC          (0x33 << 2)
#define BITS_RXPLL_OUTPUT_DIV_2ND(x)       ((x & (BIT(3) - 1)) << 8)
#define PHYA0_REG_SATA_PHY_SYNTH_SET1      (0x40 << 2)
#define PHYA0_REG_SATA_PHY_SYNTH_SET2      (0x41 << 2)

#define PHYA1_REG_ALL_SWRST_N (0x00 << 2)
#define BIT_ALL_SWRST         BIT(0)
#define PHYA1_REG_PHY_SWRST   (0x00 << 2)
#define BIT_PHY_SWRST         BIT(4)

#endif
