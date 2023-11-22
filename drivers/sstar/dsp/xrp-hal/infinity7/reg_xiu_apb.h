/*
 * reg_xiu_apb.h- Sigmastar
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

#ifndef _DSP_REG_XIU_APB_H_
#define _DSP_REG_XIU_APB_H_

#define DSP_XIU_APB_PMG         0X1000
#define DSP_XIU_APB_PMG_EN      BITS(0 : 0, BIT(0))
#define DSP_XIU_APB_PMG_EN_MASK BMASK(0 : 0)

#define DSP_XIU_APB_PMINTPC 0X1010

#define DSP_XIU_APB_PM0 0X1080
#define DSP_XIU_APB_PM1 0X1084
#define DSP_XIU_APB_PM2 0X1088
#define DSP_XIU_APB_PM3 0X108C
#define DSP_XIU_APB_PM4 0X1090
#define DSP_XIU_APB_PM5 0X1094
#define DSP_XIU_APB_PM6 0X1098
#define DSP_XIU_APB_PM7 0X109C

#define DSP_XIU_APB_PMCRTL0 0X1100
#define DSP_XIU_APB_PMCRTL1 0X1104
#define DSP_XIU_APB_PMCRTL2 0X1108
#define DSP_XIU_APB_PMCRTL3 0X110C
#define DSP_XIU_APB_PMCRTL4 0X1110
#define DSP_XIU_APB_PMCRTL5 0X1104
#define DSP_XIU_APB_PMCRTL6 0X1108
#define DSP_XIU_APB_PMCRTL7 0X111C

#define DSP_CORE_PMCTRL_INTEN_MASK      BMASK(0 : 0)
#define DSP_CORE_PMCTRL_INTEN_LSB       0
#define DSP_CORE_PMCTRL_KRNL_MASK       BMASK(3 : 3)
#define DSP_CORE_PMCTRL_KRNL_LSB        3
#define DSP_CORE_PMCTRL_TRACESCOPE_MASK BMASK(6 : 4)
#define DSP_CORE_PMCTRL_TRACESCOPE_LSB  4
#define DSP_CORE_PMCTRL_SELECT_MASK     BMASK(12 : 8)
#define DSP_CORE_PMCTRL_SELECT_LSB      8
#define DSP_CORE_PMCTRL_MSK_MASK        BMASK_ULL(31 : 16)
#define DSP_CORE_PMCTRL_MSK_LSB         16

#define DSP_XIU_APB_PMSTAT0 0X1180
#define DSP_XIU_APB_PMSTAT1 0X1184
#define DSP_XIU_APB_PMSTAT2 0X1188
#define DSP_XIU_APB_PMSTAT3 0X118C
#define DSP_XIU_APB_PMSTAT4 0X1190
#define DSP_XIU_APB_PMSTAT5 0X1194
#define DSP_XIU_APB_PMSTAT6 0X1198
#define DSP_XIU_APB_PMSTAT7 0X119C

#endif /*_DSP_REG_XIU_APB_H_*/