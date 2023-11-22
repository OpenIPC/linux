/*
 * iic_os.h- Sigmastar
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

#ifndef __IIC_OS__
#define __IIC_OS__

#ifdef CONFIG_ARM64
#define HAL_I2C_READ_WORD(_reg)        (*(volatile u16 *)(u64)(_reg))
#define HAL_I2C_WRITE_WORD(_reg, _val) ((*(volatile u16 *)(u64)(_reg)) = (u16)(_val))
#define HAL_I2C_WRITE_WORD_MASK(_reg, _val, _mask) \
    ((*(volatile u16 *)(u64)(_reg)) = ((*(volatile u16 *)(u64)(_reg)) & ~(_mask)) | ((u16)(_val) & (_mask)))

#define HAL_I2C_WRITE_BYTE(_reg, _val) ((*(volatile unsigned char *)((u64)_reg)) = (u8)(_val))
#define HAL_I2C_READ_BYTE(_reg)        (*(volatile unsigned char *)((u64)_reg))
#else
#define HAL_I2C_READ_WORD(_reg)        (*(volatile u16 *)(u32)(_reg))
#define HAL_I2C_WRITE_WORD(_reg, _val) ((*(volatile u16 *)(u32)(_reg)) = (u16)(_val))
#define HAL_I2C_WRITE_WORD_MASK(_reg, _val, _mask) \
    ((*(volatile u16 *)(u32)(_reg)) = ((*(volatile u16 *)(u32)(_reg)) & ~(_mask)) | ((u16)(_val) & (_mask)))

#define HAL_I2C_WRITE_BYTE(_reg, _val) ((*(volatile unsigned char *)((u32)_reg)) = (u8)(_val))
#define HAL_I2C_READ_BYTE(_reg)        (*(volatile unsigned char *)((u32)_reg))
#endif

#define I2C_DELAY_N_US(_x) \
    do                     \
    {                      \
        CamOsUsDelay(_x);  \
    } while (0)

//#define HAL_I2C_DMSG_ENABLE
#ifdef HAL_I2C_DMSG_ENABLE
#define dmsg_i2c_halerr(fmt, ...)           \
    do                                      \
    {                                       \
        printk("err: " fmt, ##__VA_ARGS__); \
    } while (0)
#define dmsg_i2c_halwarn(fmt, ...)           \
    do                                       \
    {                                        \
        printk("debug " fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define dmsg_i2c_halerr(fmt, ...)
#define dmsg_i2c_halwarn(fmt, ...)
#endif

#define hal_i2c_cache_flush(_ptr_, _size_)      CamOsMemFlush((void *)_ptr_, (u32)_size_)
#define hal_i2c_cache_invalidate(_ptr_, _size_) CamOsMemInvalidate((void *)_ptr_, (u32)_size_)

#endif
