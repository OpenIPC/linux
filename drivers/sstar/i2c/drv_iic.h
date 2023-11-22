/*
 * drv_iic.h- Sigmastar
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

#ifndef _DRV_IIC_H_
#define _DRV_IIC_H_
#include <linux/i2c.h>
typedef s32 (*sstar_i2c_async_calbck)(char para_msg_done, void *reserved);

extern s32 sstar_i2c_master_xfer(struct i2c_adapter *para_adapter, struct i2c_msg *para_msg, s32 para_num);
extern s32 sstar_i2c_master_async_xfer(struct i2c_adapter *para_adapter, struct i2c_msg *para_msg, s32 para_num);
extern s32 sstar_i2c_async_cb_set(struct i2c_adapter *para_adapter, sstar_i2c_async_calbck para_cb, void *reserved);
extern s32 sstar_i2c_wnwrite_xfer(struct i2c_adapter *para_adapter, struct i2c_msg *para_msg, s32 para_num,
                                  u8 para_wnlen, u16 para_waitcnt);
extern s32 sstar_i2c_wnwrite_async_xfer(struct i2c_adapter *para_adapter, struct i2c_msg *para_msg, s32 para_num,
                                        u8 para_wnlen, u16 para_waitcnt);
#endif
