/*
 * phy_sstar_utmi_reg.h- Sigmastar
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

#ifndef __PHY_SSTAR_UTMI_REG_H__
#define __PHY_SSTAR_UTMI_REG_H__

#define REG_UTMI_PWR_DOWN                         (0x00)
#define BIT_ENABLE_USB_XCVR_POWER_DOWN            BIT(0)
#define BIT_ENABLE_FS_LS_TERM                     BIT(1)
#define BIT_POWER_DOWN_USB_XCVR_HS_DE_SERIAL      BIT(8)
#define BIT_POWER_DOWN_USB_XCVR_PLL               BIT(9)
#define BIT_POWER_DOWN_USB_XCVR_TED               BIT(10)
#define BIT_POWER_DOWN_USB_XCVR_HS_PRE_AMP        BIT(11)
#define BIT_POWER_DOWN_USB_XCVR_HS_LS_TRANSCEIVER BIT(12)
#define BIT_POWER_DOWN_USB_XCVR_VBUS_DETECTOR     BIT(13)
#define BIT_POWER_DOWN_USB_XCVR_CURRENT           BIT(14)
#define BIT_POWER_DOWN_USB_XCVR_REGULATOR         BIT(15)
#define REG_EOP40_DET_DELAY_CNT                   (0x01 << 2)
#define BITS_HS_EOP40_WINDOW_DELAY(x)             (x << 11)
#define REG_UTMI_SW_RESET                         (0x03 << 2)
#define BIT_RX_SW_RESET                           BIT(0)
#define BIT_TX_SW_RESET                           BIT(1)
#define BIT_TX_FSM_SW_RESET                       BIT(8)
#define REG_HS_STAGE_SEL                          (0x03 << 2)
#define BITS_RX_DATA_RECOVERY_STAGE(x)            (x << 5)
#define REG_TX_RESET_FSM                          (0x03 << 2)
#define BIT_RESET_TX_INTERNAL_FSM                 BIT(8)
#define REG_CDR_MODEL_SEL                         (0x03 << 2)
#define BIT_NEW_CDR_MODE                          BIT(9)
#define REG_CLK_EXTRA_1_EN                        (0x04 << 2)
#define BIT_ENABLE_ISI_IMPROVEMENT                BIT(8)
#define REG_HS_RX_ROBUST_EN                       (0x04 << 2)
#define BIT_ENABLE_RX_ROBUST_FEATURE              BIT(15)
#define REG_CK_INV_RESERVED                       (0x05 << 2)
#define BIT_THE_LATCH_ON_HS_DATA                  BIT(15)
#define REG_UMTI_PWR_RESET                        (0x08 << 2)
#define BIT_PWR_SW_RESET                          BIT(12)
#define REG_MACMODE_OVD                           (0x0A << 2)
#define BIT_CHANGE_MAC_CONTROL_SIGNAL             BIT(13)
#define REG_UTMI_CA                               (0x1E << 2)
#define BIT_CA_START                              BIT(0)
#define BIT_CA_END                                BIT(1)

#endif
