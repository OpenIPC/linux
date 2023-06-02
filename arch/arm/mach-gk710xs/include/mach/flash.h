/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/flash.h
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

#ifndef __MACH_FLASH_H
#define __MACH_FLASH_H

#include <linux/types.h>

#define SFLASH_REG(x)           (GK_VA_SFLASH + (x))

#define REG_SFLASH_DATA         SFLASH_REG(0x00)   /* read/write */
#define REG_SFLASH_COMMAND      SFLASH_REG(0x04)   /* read/write */
#define REG_SFLASH_CE           SFLASH_REG(0x08)   /* read/write */
#define REG_SFLASH_SPEED        SFLASH_REG(0x0C)   /* read/write */
#define REG_SFLASH_PARA_XIP     SFLASH_REG(0x10)   /* read/write */

// FPGA: SCLK = 40MHz
// EVB: SCLK = 138MHz
// 000: SCLK/2
// 001: SCLK/4
// 010: SCLK/6
// 011: SCLK/8
// 100: SCLK/10
typedef enum
{
   GOKE_SFLASH_FREQ_DIV2 = 0,
   GOKE_SFLASH_FREQ_DIV4,
   GOKE_SFLASH_FREQ_DIV6,
   GOKE_SFLASH_FREQ_DIV8,
   GOKE_SFLASH_FREQ_DIV10
}GD_SFLASH_SPEED_MODE;

#define SYSTEM_SFLASH_FREQ  GOKE_SFLASH_FREQ_DIV2

struct sflash_platform_data /* flash device platform_data */
{
    uint32_t    speed_mode;
    uint32_t    channel;
    uint32_t    nr_parts;
    struct mtd_partition *parts;
};

#endif  /* __MACH_FLASH_H */


