/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_ACTT_H__
#define __AX_ACTT_H__

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/types.h>

#define ENABLE 1
#define DISABLE 0
#define RX_IIS_FORMAT_CLEAN 0X03

#define PD_BIAS (0X00a<<2)
#define RX_ADC_CONTROL0 (0X007<<2)
#define RX_ADC_CONTROL0_DEFAULT (0xf)

/* common register for rx */
#define RX_IIS_ENABLE (0X020<<2)
#define RX_IIS_FORMAT (0X021<<2)  /* bit , set here */
#define RX_IIS_CLK_SEL (0X022<<2)

#define RX_IIS_MCLK_MASK 0x3f

/* set INPUT SEL info */
#define DMIC_SET (0X028<<2)
#define INPUT_SEL (0X025<<2)

#define ANA_DAC_CONTROLS_0 (0X009<<2)
#define ANA_DAC_CONTROLS_0_DEFAULT (0x05)

/* ALC */
#define RX_ALC_SET (0X030<<2)
#define RX_ALC_TARGET_LEVEL (0X032<<2)
#define RX_ALC_RMS_MAX (0X033<<2)
#define RX_ALC_GATE_THRESHOLD_LEVEL (0X034<<2)
#define RX_ALC_GAIN_MAX (0X035<<2)

#define RX_LEFT_ANA_GAIN_SET (0X038<<2)
#define RX_RIGHT_ANA_GAIN_SET (0X039<<2)
#define RX_LEFT_DIG_GAIN_SET (0X03a<<2)
#define RX_RIGHT_DIG_GAIN_SET (0X03b<<2)

#define RX_ANA_GAIN_DEFAULT (0x36)
#define RX_ANA_GAIN (0x1e)
#define RX_DIG_GAIN_DEFAULT (0xf)
#define RX_ALC_DEFAULT (0x9a)

/* set rx rate  */
#define RX_CIC_RATE_SET_BIT_H (0X011<<2)
#define RX_CIC_RATE_SET_BIT_L (0X012<<2)

/* rx deal enable or disable */
#define RX_DEAL_ENABLE (0X010<<2)
#define RX_MUTE_ENABLE (0X02a<<2)

/* set rx eq */
#define RX_IIR_BYPASS1_ENABLE (0X013<<2)
#define RX_IIR_BYPASS2_ENABLE (0X014<<2)
#define RX_3D_ENABLE (0X019<<2)

/* set rx eq default */
#define RX_IIR_BYPASS1_DEFAULT 0xff
#define RX_IIR_BYPASS2_DEFAULT 0x03
#define RX_3D_DEFAULT 127

#define RX_IIR_HPF_A2_H (0x06a<<2)
#define RX_IIR_HPF_A2_L (0x06b<<2)
#define RX_IIR_HPF_A1_H (0x06c<<2)
#define RX_IIR_HPF_A1_L (0x06d<<2)
#define RX_IIR_HPF_B2_H (0x06e<<2)
#define RX_IIR_HPF_B2_L (0x06f<<2)
#define RX_IIR_HPF_B1_H (0x070<<2)
#define RX_IIR_HPF_B1_L (0x071<<2)
#define RX_IIR_HPF_B0_H (0x072<<2)
#define RX_IIR_HPF_B0_L (0x073<<2)

/* set format default  rx / tx */
#define IIS_FORMAT_DEFAULT 0X20
#define IIS_CLK_SEL_DEFAULT 0X00

/* common register for tx */
#define TX_IIS_ENABLE (0X050<<2)
#define TX_IIS_FORMAT (0X051<<2)
#define TX_IIS_CLK_SEL (0X052<<2)

#define TX_IIS_FORMAT_MASK (0X03)

/* set tx rate  */
#define TX_CIC_RATE_SET_BIT_H (0X041<<2)
#define TX_CIC_RATE_SET_BIT_L (0X042<<2)

/* set tx eq */
#define TX_IIR_BYPASS1_ENABLE (0X043<<2)
#define TX_IIR_BYPASS2_ENABLE (0X044<<2)
#define TX_3D_ENABLE (0X045<<2)

/* set rx eq default */
#define TX_IIR_BYPASS1_DEFAULT 0xff
#define TX_IIR_BYPASS2_DEFAULT 0xff
#define TX_3D_DEFAULT 127

/* Tx deal enable or disable */
#define TX_DEAL_ENABLE (0X040<<2)
#define TX_MUTE_ENABLE (0X046<<2)

/* Tx mute mask */
#define TX_MUTE_ENABLE_MASK 0X10
#define TX_MUTE_DISABLE_MASK 0X0f

#define TX_LEFT_ANA_GAIN_SET (0X058<<2)
#define TX_RIGHT_ANA_GAIN_SET (0X059<<2)
#define TX_LEFT_DIG_GAIN_SET (0X05a<<2)
#define TX_RIGHT_DIG_GAIN_SET (0X05b<<2)

#define TX_ANA_GAIN_DEFAULT (0X31)
#define TX_DIG_GAIN_DEFAULT (0Xf)
#endif