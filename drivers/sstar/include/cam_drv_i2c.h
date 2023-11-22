/*
 * cam_drv_i2c.h- Sigmastar
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
#ifndef _CAM_DRV_I2C_H_
#define _CAM_DRV_I2C_H_
#include <cam_os_wrapper.h>

#if defined(__RTK_OS__)
#include "vm_types.ht"
typedef struct i2c_msg
{
    u16 addr; /* slave address			*/
    u16 flags;
#define I2C_M_RD 0x0001           /* read data, from slave to master */
                                  /* I2C_M_RD is guaranteed to be 0x0001! */
#define I2C_M_TEN      0x0010     /* this is a ten bit chip address */
#define I2C_M_DMA_SAFE 0x0200     /* the buffer of this message is DMA safe */
                                  /* makes only sense in kernelspace */
                                  /* userspace buffers are copied anyway */
#define I2C_M_RECV_LEN     0x0400 /* length will be first received byte */
#define I2C_M_NO_RD_ACK    0x0800 /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK   0x1000 /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR 0x2000 /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART      0x4000 /* if I2C_FUNC_NOSTART */
#define I2C_M_STOP         0x8000 /* if I2C_FUNC_PROTOCOL_MANGLING */
    u16 len;                      /* msg length				*/
    u8 *buf;                      /* pointer to msg data			*/
} tI2cMsg;

#elif defined(__KERNEL__)
#include <linux/i2c.h>
typedef struct i2c_msg tI2cMsg;
#endif

typedef struct
{
    s32   nPortNum;
    void *pAdapter;
} tI2cHandle;

s32 CamI2cOpen(tI2cHandle *pHandle, u8 nPortNum);
s32 CamI2cTransfer(tI2cHandle *pHandle, tI2cMsg *pMsg, u32 nMsgNum);
s32 CamI2cClose(tI2cHandle *pHandle);
s32 CamI2cSetClk(void *pHandle, u32 clk);

#endif // _CAM_DRV_I2C_H_
