/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/hal/i2s.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __HAL_CRYPTO_H__
#define __HAL_CRYPTO_H__

#include <mach/hardware.h>

#define CRYPTO_VA_BASE                  GK_VA_CRYPT_UNIT
#define CRYPTO_VA_REG(x)                (CRYPTO_VA_BASE + (x))
#define REG_CRYPTO_DES_KEY              CRYPTO_VA_REG(0x00c) /* read/write */
#define REG_CRYPTO_DES_INPUT            CRYPTO_VA_REG(0x014) /* read/write */
#define REG_CRYPTO_DES_OPCODE           CRYPTO_VA_REG(0x004) /* read/write */
#define REG_CRYPTO_DES_OUTPUT_RDY       CRYPTO_VA_REG(0x018) /* read */
#define REG_CRYPTO_DES_OUTPUT           CRYPTO_VA_REG(0x020) /* read */
#define REG_CRYPTO_DES_INTERRUPT        CRYPTO_VA_REG(0x000) /* read/write */

#define REG_CRYPTO_AES_128_KEY          CRYPTO_VA_REG(0x074) /* read/write */

#define REG_CRYPTO_AES_192_KEY          CRYPTO_VA_REG(0x064) /* read/write */

#define REG_CRYPTO_AES_256_KEY          CRYPTO_VA_REG(0x04c) /* read/write */

#define REG_CRYPTO_AES_INPUT            CRYPTO_VA_REG(0x084) /* read/write */
#define REG_CRYPTO_AES_OPCODE           CRYPTO_VA_REG(0x02C) /* read/write */
#define REG_CRYPTO_AES_OUTPUT_RDY       CRYPTO_VA_REG(0x088) /* read */

#define REG_CRYPTO_AES_OUTPUT           CRYPTO_VA_REG(0x098) /* read */
#define REG_CRYPTO_AES_INTERRUPT        CRYPTO_VA_REG(0x028) /* read/write */
#define REG_CRYPTO_EFUSE_BOOT_SW_DIS    CRYPTO_VA_REG(0x024) /* read/write */



#endif /* __HAL_CRYPTO_H__ */
