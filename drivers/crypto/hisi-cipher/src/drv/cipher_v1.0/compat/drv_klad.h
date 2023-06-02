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

#ifndef __DRV_KLAD_H_
#define __DRV_KLAD_H_

#include "hi_types.h"
#include "drv_osal_lib.h"

hi_s32 hal_cipher_klad_config(hi_u32 chn_id,
                             hi_u32 opt_id,
                             hi_cipher_klad_target target,
                             hi_bool is_decrypt);

hi_void hal_cipher_start_klad(hi_u32 block_num);
hi_void hal_cipher_set_klad_data(hi_u32 *data_in);
hi_void hal_cipher_get_klad_data(hi_u32 *data_out);
hi_s32 hal_cipher_wait_klad_done(hi_void);

hi_s32 drv_cipher_klad_init(hi_void);
hi_void drv_cipher_klad_deinit(hi_void);

hi_s32 drv_cipher_klad_load_key(hi_u32 chn_id,
                              hi_cipher_ca_type root_key,
                              hi_cipher_klad_target target,
                              hi_u8 *data_in,
                              hi_u32 key_len);

hi_s32 drv_cipher_klad_encrypt_key(hi_cipher_ca_type root_key,
                                 hi_cipher_klad_target target,
                                 hi_u32 clean_key[4],
                                 hi_u32 encrypt_key[4]);

#endif