/*
 * cam_drv_i2c.c- Sigmastar
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

#include <cam_drv_i2c.h>
#include <drv_iic.h>

s32 CamI2cOpen(tI2cHandle *handle, u8 port_num)
{
#if defined(__RTK_OS__)
    handle->nPortNum = port_num;

#elif defined(__KERNEL__)
    handle->pAdapter = (void *)i2c_get_adapter(port_num);

#endif
    return 0;
}

s32 CamI2cTransfer(tI2cHandle *handle, tI2cMsg *msgs, u32 msg_num)
{
#if defined(__RTK_OS__)

#elif defined(__KERNEL__)
    return i2c_transfer((struct i2c_adapter *)handle->pAdapter, msgs, msg_num);

#endif
}

s32 CamI2cAsyncTransfer(tI2cHandle *handle, tI2cMsg *msgs, u32 msg_num)
{
#if defined(__RTK_OS__)

#elif defined(__KERNEL__)
    return sstar_i2c_master_async_xfer((struct i2c_adapter *)handle->pAdapter, msgs, msg_num);
#endif
}

s32 CamI2cClose(tI2cHandle *handle)
{
#if defined(__RTK_OS__)
    handle->nPortNum = (-1);

#elif defined(__KERNEL__)

#endif
    return 0;
}

s32 CamI2cSetAsyncCb(tI2cHandle *handle, sstar_i2c_async_calbck calbck, void *reserved)
{
    s32 ret = 0;

    sstar_i2c_async_cb_set((struct i2c_adapter *)handle->pAdapter, calbck, reserved);

    return ret;
}

s32 CamI2cWnWrite(tI2cHandle *handle, tI2cMsg *msgs, u32 msg_num, u32 wn_len, u16 wait_cnt)
{
#if defined(__RTK_OS__)

#elif defined(__KERNEL__)
    s32 ret = 0;

    ret |= sstar_i2c_wnwrite_xfer((struct i2c_adapter *)handle->pAdapter, msgs, msg_num, wn_len, wait_cnt);

    return ret;
#endif
}
s32 CamI2cWnAsyncWrite(tI2cHandle *handle, tI2cMsg *msgs, u32 msg_num, u32 wn_len, u16 wait_cnt)
{
#if defined(__RTK_OS__)

#elif defined(__KERNEL__)
    s32 ret = 0;

    ret |= sstar_i2c_wnwrite_async_xfer((struct i2c_adapter *)handle->pAdapter, msgs, msg_num, wn_len, wait_cnt);

    return ret;
#endif
}

s32 CamI2cSetClk(void *handle, u32 clk)
{
    // ToDo
    return 0;
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(CamI2cOpen);
EXPORT_SYMBOL(CamI2cTransfer);
EXPORT_SYMBOL(CamI2cAsyncTransfer);
EXPORT_SYMBOL(CamI2cSetAsyncCb);
EXPORT_SYMBOL(CamI2cWnWrite);
EXPORT_SYMBOL(CamI2cWnAsyncWrite);
EXPORT_SYMBOL(CamI2cClose);
#endif
