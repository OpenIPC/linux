/*
 * reg.h- Sigmastar
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

#ifndef _DSP_REG_H_
#define _DSP_REG_H_

#define BITS(_bits_, _val_)         ((BIT(((1)?_bits_)+1)-BIT(((0)?_bits_))) & (_val_<<((0)?_bits_)))
#define BMASK(_bits_)               (BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))
#define BITS_ULL(_bits_, _val_)     (GENMASK_ULL(((1)?_bits_),((0)?_bits_)) & (_val_<<((0)?_bits_)))
#define BMASK_ULL(_bits_)           GENMASK_ULL(((1)?_bits_),((0)?_bits_))

// XTAL
#define BASE_REG_XTAL_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x111B00)

// MPLL
#define BASE_REG_MPLL_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103000)

// DSP SYNTHESICZER
#define BASE_REG_SYTHESIZER_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x102D00)

// XIU->APB bank
#define BASE_REG_X32_VQ7_GP0 GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A1D00)
#define BASE_REG_X32_VQ7_GP1 GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A1E00)
#define BASE_REG_X32_VQ7_GP2 GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A1F00)
#define BASE_REG_X32_VQ7_GP3 GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A2000)

// dig gp contorl
#ifdef BASE_REG_DIG_GP_PA
#undef BASE_REG_DIG_GP_PA
#endif
#define BASE_REG_DIG_GP_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x113100)

// dsp group control
#define BASE_REG_DSPG_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x163000)
// dsp0 control
#define BASE_REG_DSP0_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x163100)
// dsp1 control
#define BASE_REG_DSP1_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x163200)
// dsp2 control
#define BASE_REG_DSP2_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x163300)
// dsp3 contorl
#define BASE_REG_DSP3_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x163400)

#define BASE_REG_DSP_NODIE_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x163000)

#define DSP_ROM_MAX_SIZE        0x10000000 // 256M
#define DSP_SYSTEM_MAX_SIZE     0x20000000 // 512M
#define DSP0_ROM_CPU_ADDRESS    1
#define DSP0_SYSTEM_CPU_ADDRESS 1
#define DSP1_ROM_CPU_ADDRESS    1
#define DSP1_SYSTEM_CPU_ADDRESS 1
#define DSP2_ROM_CPU_ADDRESS    1
#define DSP2_SYSTEM_CPU_ADDRESS 1
#define DSP3_ROM_CPU_ADDRESS    1
#define DSP3_SYSTEM_CPU_ADDRESS 1

#include "reg_gp.h"
#include "reg_core.h"
#include "reg_dig.h"
#include "reg_xiu_apb.h"
#include "reg_nodie.h"
#include "reg_clk.h"
#endif /*_DSP_REG_H_*/