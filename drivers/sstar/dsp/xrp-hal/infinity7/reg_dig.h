/*
 * reg_dig.h- Sigmastar
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

#ifndef _DSP_REG_DIG_H_
#define _DSP_REG_DIG_H_

#define DSP_DIG_CPU_INT_W_L        REG_ID_00
#define DSP_DIG_CPU_INT_W_H        REG_ID_01
#define DSP_DIG_CPU_INT_DSP0       BITS(24 : 0, BIT(1))
#define DSP_DIG_CPU_INT_DSP0_LSB   5
#define DSP_DIG_CPU_INT_DSP1       BITS(24 : 0, BIT(2))
#define DSP_DIG_CPU_INT_DSP1_LSB   10
#define DSP_DIG_CPU_INT_DSP2       BITS(24 : 0, BIT(3))
#define DSP_DIG_CPU_INT_DSP2_LSB_L 15
#define DSP_DIG_CPU_INT_DSP2_LSB_H 0
#define DSP_DIG_CPU_INT_DSP3       BITS(24 : 0, BIT(4))
#define DSP_DIG_CPU_INT_DSP3_LSB   (20 - 16)
#define DSP_DIG_CPU_INT_ALL        BITS(24 : 0, BIT(0))
#define DSP_DIG_CPU_INT_W_MASK     BMASK_ULL(24 : 0)

#endif /*_DSP_REG_DIG_H_*/