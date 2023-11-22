/*
 * reg_clk.h- Sigmastar
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

#ifndef _DSP_REG_CLK_H_
#define _DSP_REG_CLK_H_

#define DSP_CLK_XTAL_HV_POWER     REG_ID_09
#define DSP_CLK_XTAL_HV_POWER_EN  0X0
#define DSP_CLK_XTAL_HV_POWER_MSK BMASK(7 : 0)
#define DSP_CLK_MPLL_POWER        REG_ID_03
#define DSP_CLK_MPLL_POWER_EN     0X0
#define DSP_CLK_MPLL_POWER_MSK    BMASK(7 : 0)

#define DSP_CLK_MPLL_U02_ECO       REG_ID_0B
#define DSP_CLK_MPLL_U02_ECO_VALUE 0X4000
#define DSP_CLK_MPLL_U02_ECO_MSK   BMASK(15 : 8)

#define DSP_CLK_PLL_POWER     REG_ID_11
#define DSP_CLK_PLL_POWER_EN  0x0080
#define DSP_CLK_PLL_POWER_MSK BMASK(15 : 0)

#define DSP_CLK_DSPPLL_SCALAR_DIV_1     REG_ID_1B
#define DSP_CLK_DSPPLL_SCALAR_DIV_1_LSB 14
#define DSP_CLK_DSPPLL_SCALAR_DIV_1_MSK BMASK(15 : 14)
#define DSP_CLK_DSPPLL_SCALAR_DIV_2     REG_ID_1B
#define DSP_CLK_DSPPLL_SCALAR_DIV_2_LSB 9
#define DSP_CLK_DSPPLL_SCALAR_DIV_2_MSK BMASK(13 : 9)
#define DSP_CLK_DSPPLL_SCALAR_DIV_3_LSB 7
#define DSP_CLK_DSPPLL_SCALAR_DIV_3     REG_ID_1B
#define DSP_CLK_DSPPLL_SCALAR_DIV_3_MSK BMASK(8 : 7)

#define DSP_CLK_DSPPLL_SYNTHESIZER_CLK_1     REG_ID_60
#define DSP_CLK_DSPPLL_SYNTHESIZER_CLK_1_MSK BMASK(7 : 0)
#define DSP_CLK_DSPPLL_SYNTHESIZER_CLK_2     REG_ID_60
#define DSP_CLK_DSPPLL_SYNTHESIZER_CLK_2_MSK BMASK(15 : 8)
#define DSP_CLK_DSPPLL_SYNTHESIZER_CLK_3     REG_ID_61
#define DSP_CLK_DSPPLL_SYNTHESIZER_CLK_3_MSK BMASK(7 : 0)
#define DSP_CLK_DSPPLL_SYNTHESIZER_NF        REG_ID_62
#define DSP_CLK_DSPPLL_SYNTHESIZER_NF_EN     0x01
#define DSP_CLK_DSPPLL_SYNTHESIZER_NF_MSK    BMASK(7 : 0)

#define DSP_CLK_DSPPLL_U02_ECO       REG_ID_1E
#define DSP_CLK_DSPPLL_U02_ECO_VALUE 0X4000
#define DSP_CLK_DSPPLL_U02_ECO_MSK   BMASK(15 : 8)

#endif /*_DSP_REG_CLK_H_*/