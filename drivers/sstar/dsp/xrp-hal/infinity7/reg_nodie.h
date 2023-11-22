/*
 * reg_nodie.h- Sigmastar
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

#ifndef _DSP_REG_NODIE_H_
#define _DSP_REG_NODIE_H_

#define DSP_NODIE_RESET      REG_ID_20
#define DSP_NODIE_RESET_ALL  BITS(7 : 0, BIT(0))
#define DSP_NODIE_RESET_DIE  BITS(7 : 0, BIT(1))
#define DSP_NODIE_RESET_MCU  BITS(7 : 0, BIT(2))
#define DSP_NODIE_RESET_MASK BMASK(7 : 0)

#define DSP_NODIE_TOP_ISO      REG_ID_20
#define DSP_NODIE_TOP_ISO_EN   BITS(8 : 8, BIT(0))
#define DSP_NODIE_TOP_ISO_MASK BMASK(8 : 8)

#define DSP_NODIE_BIST_FAIL      REG_ID_21
#define DSP_NODIE_BIST_FAIL_MASK BMASK(15 : 0)

#endif /*_DSP_REG_NODIE_H_*/