/*
 * hal_camclk_st.h- Sigmastar
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

#ifndef __HAL_CAMCLK_ST_H__
#define __HAL_CAMCLK_ST_H__

typedef struct
{
    u32 u32Rate;
    u32 u32ParentRate;
} HalCamClkSetComplexRate_t;
typedef struct
{
    u32 u32Rate;
    u32 u32ParentRate;
} HalCamClkGetComplexRate_t;
typedef struct
{
    u32 u32Rate;
    u32 u32RoundRate;
} HalCamClkGetComplexRoundRate_t;
typedef struct
{
    u8 bEn;
} HalCamClkComplexOnOff_t;
#endif /* MHAL_DIP_H */
