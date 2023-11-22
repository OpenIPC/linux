/*
 * hal_rtcpwcreg.h- Sigmastar
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

#ifndef __HAL_RTCPWCREG_H__
#define __HAL_RTCPWCREG_H__

#define RTC_PASSWORD 0xBABE

#define RTCPWC_DIG2RTC_BASE_WR     0x00
#define RTCPWC_DIG2RTC_BASE_WR_BIT BIT1
#define RTCPWC_DIG2RTC_BASE_RD     BIT2
#define RTCPWC_DIG2RTC_CNT_RST_WR  BIT3
#define RTCPWC_DIG2RTC_ALARM_WR    BIT4
#define RTCPWC_DIG2RTC_SW0_WR      BIT5
#define RTCPWC_DIG2RTC_SW1_WR      BIT6
#define RTCPWC_DIG2RTC_SW0_RD      BIT7
#define RTCPWC_DIG2RTC_SW1_RD      BIT8

#define RTCPWC_DIG2RTC_CNT_RD       0x01
#define RTCPWC_DIG2RTC_CNT_RD_BIT   BIT0
#define RTCPWC_DIG2RTC_ALARM_RD_BIT BIT1
#define RTCPWC_DIG2RTC_ALARM_EN_BIT BIT2
#define RTCPWC_DIG2RTC_INT_CLR_BIT  BIT3

#define RTCPWC_DIG2RTC_ISO_CTRL     0x03
#define RTCPWC_DIG2RTC_BASE_WR_MASK BIT0 | BIT1 | BIT2

#define RTCPWC_DIG2RTC_WRDATA_L 0x04
#define RTCPWC_DIG2RTC_WRDATA_H 0x05

#define RTCPWC_DIG2RTC_SET     0x06
#define RTCPWC_DIG2RTC_SET_BIT BIT0

#define RTCPWC_RTC2DIG_VAILD     0x07
#define RTCPWC_RTC2DIG_VAILD_BIT BIT0

#define RTCPWC_RTC2DIG_ISO_CTRL_ACK     0x08
#define RTCPWC_RTC2DIG_ISO_CTRL_ACK_BIT BIT3

#define RTCPWC_RTC2DIG_ISO_ALARM_INT     0x08
#define RTCPWC_RTC2DIG_ALARM_EN_BIT      BIT0
#define RTCPWC_RTC2DIG_ISO_ALARM_INT_BIT BIT1

#define RTCPWC_RTC2DIG_RDDATA_L 0x09
#define RTCPWC_RTC2DIG_RDDATA_H 0x0A

#define RTCPWC_RTC2DIG_CNT_UPDATING     0x0B
#define RTCPWC_RTC2DIG_CNT_UPDATING_BIT BIT0
#define RTCPWC_REG_RTC2DIG_RDDATA_CNT_L 0x0C
#define RTCPWC_REG_RTC2DIG_RDDATA_CNT_H 0x0D

#define RTCPWC_DIG2RTC_CNT_RD_TRIG     0x0E
#define RTCPWC_DIG2RTC_CNT_RD_TRIG_BIT BIT0

#define RTCPWC_DIG2PWC_OPT 0x10
#define RTCPWC_SW_RST      BIT8

#define RTC_PWC_PWC2DIG_FLAG       0x11
#define RTC_PWC_PWC2DIG_FLAG_MASK  0xFF
#define RTC_PWC_PWC2DIG_STATE      0x12
#define RTC_PWC_PWC2DIG_STATE_MASK 0xFF

#define RTCPWC_DIG2PWC_RTC_TESTBUS 0x15
#define RTCPWC_ISO_EN              BIT0
#define RTCPWC_CLK_1K              BIT5

#define RTCPWC_DIG2PWC_PWR_EN_CTRL 0x0F
#define RTCPWC_PWR_EN              BIT0
#define RTCPWC_ALARM_ON_EN         BIT1

#define RTCPWC_DIG2PWC_PWR2_EN_CTRL 0x20
#define RTCPWC_PWR2_EN              BIT0
#define RTCPWC_ALARM2_ON_EN         BIT1
#define RTCPWC_IO0_HIZ_EN_BIT       BIT2
#define RTCPWC_IO3_PUPD_SEL_BIT     BIT3
#define RTCPWC_IO3_POL_BIT          BIT4

#define RTCPWC_WOS_V_SEL_REG 0x21

#define RTCPWC_WOS_CTRL_REG         0x22
#define RTCPWC_WOS_CLR_BIT          BIT0
#define RTCPWC_WOS_HP_EN_BIT        BIT1
#define RTCPWC_WOS_CMPOUT_SEL_SHIFT 2
#define RTCPWC_WOS_CMPOUT_SEL_MASK  (BIT2 | BIT3)

#define RTCPWC_ISO_TRIG       0x30
#define RTCPWC_ISO_TRIG_BIT   BIT0
#define RTCPWC_ISO_EN_ACK     0x31
#define RTCPWC_ISO_EN_ACK_BIT BIT0
#define RTCPWC_ISO_EN_CLR     0x32
#define RTCPWC_ISO_EN_CLR_BIT BIT0

#define RTCPWC_ISO_AUTO_REGEN     0x35
#define RTCPWC_ISO_AUTO_REGEN_bit BIT0

#endif
