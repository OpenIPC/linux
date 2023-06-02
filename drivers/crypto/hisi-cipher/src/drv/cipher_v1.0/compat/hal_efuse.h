/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _HAL_EFUSE_H_
#define _HAL_EFUSE_H_

#include "hi_types.h"

#define HAL_CIPHER_READ_REG(addr, result)    (*(result) = *(volatile unsigned int *)(hi_uintptr_t)(addr))
#define HAL_CIPHER_WRITE_REG(addr,result)    (*(volatile unsigned int *)(hi_uintptr_t)(addr) = (result))

#define HAL_SET_BIT(src, bit)               ((src) |= (1<<bit))
#define HAL_CLEAR_BIT(src,bit)              ((src) &= ~(1<<bit))

hi_s32 hal_efuse_write_key(hi_u32 *p_key, hi_u32 opt_id);
hi_s32 hal_efuse_otp_load_cipher_key(hi_u32 chn_id, hi_u32 opt_id);
hi_s32 hal_efuse_load_hash_key(hi_u32 opt_id);
hi_s32 hal_efuse_otp_init(hi_void);

#endif

