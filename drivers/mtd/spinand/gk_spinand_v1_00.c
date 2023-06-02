/*
 * drivers/mtd/spinand/gk_spinand_v1_00.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/bitops.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <mach/flash.h>

#include "gk_spinand_v1_00.h"
#include "spinand.h"

#if 1
#define uswap_32(x) x
#else
#define uswap_32(x) \
    ((((x) & 0xff000000) >> 24) | \
     (((x) & 0x00ff0000) >>  8) | \
     (((x) & 0x0000ff00) <<  8) | \
     (((x) & 0x000000ff) << 24))
#endif

#define gk_spi_nand_driver_version "GK-MTD_01.00_Linux3.4.43_20150115"


#define BAD_BLOCK       0
#define BAD_BLOCK_OFF   0x00

struct gk_spinand_host {
    struct spinand_chip chip;
    struct mtd_info     mtd;

    /* page size of attached chip */
    unsigned int        page_size;
    int         use_ecc;
    int         cs;

    void __iomem        *io_base;
    struct device       *dev;
    struct completion   comp;
    struct mutex        lock;
};

/******************************************************/
/*                SPI Nand Part  ( Hardware Dependent )                   */
/******************************************************/

/**
     OOB area specification layout: Total  available free bytes.
*/
static struct nand_ecclayout spinand_ecc_64_layout_gd = {
    .eccbytes   = 16,
    .eccpos     = { 0x0c, 0x0d, 0x0e, 0x0f,
                    0x1c, 0x1d, 0x1e, 0x1f,
                    0x2c, 0x2d, 0x2e, 0x2f,
                    0x3c, 0x3d, 0x3e, 0x3f,},
    .oobavail   = 32,
    .oobfree    = {
                {.offset = 0x04, .length = 8},
                {.offset = 0x14, .length = 8},
                {.offset = 0x24, .length = 8},
                {.offset = 0x34, .length = 8},
    }
};

static struct nand_ecclayout spinand_ecc_64_layout_esmt = {
    .eccbytes   = 28,
    .eccpos     = { 0x01, 0x02, 0x03,
                    0x04, 0x05, 0x06, 0x07,
                    0x11, 0x12, 0x13,
                    0x14, 0x15, 0x16, 0x17,
                    0x21, 0x22, 0x23,
                    0x24, 0x25, 0x26, 0x27,
                    0x31, 0x32, 0x33,
                    0x34, 0x35, 0x36, 0x37,},
    .oobavail   = 32,
    .oobfree    = {
                {.offset = 0x08, .length = 8},
                {.offset = 0x18, .length = 8},
                {.offset = 0x28, .length = 8},
                {.offset = 0x38, .length = 8},
    }
};

static struct nand_ecclayout spinand_ecc_64_layout_ato = {
    .eccbytes   = 20,
    .eccpos     = { 0x08, 0x09, 0x0a, 0x0b, 0x0c,
                    0x18, 0x19, 0x1a, 0x1b, 0x1c,
                    0x28, 0x29, 0x2a, 0x2b, 0x2c,
                    0x38, 0x39, 0x3a, 0x3b, 0x3c,},
    .oobavail   = 0,
    .oobfree    = {
                {.offset = 0x04, .length = 8},
                {.offset = 0x14, .length = 8},
                {.offset = 0x24, .length = 8},
                {.offset = 0x34, .length = 8},
    }
};

static struct nand_ecclayout spinand_ecc_64_layout_wb = {
    .eccbytes   = 24,
    .eccpos     = { 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
                    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
                    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,},
    .oobavail   = 8,
    .oobfree    = {
                {.offset = 0x02, .length = 2},
                {.offset = 0x12, .length = 2},
                {.offset = 0x22, .length = 2},
                {.offset = 0x32, .length = 2},
    }
};

/**
    SPI Nand command set.
*/
spinand_cmd_t spinand_commands_gd =
{
    /* writeEnable          */ 0x06,
    /* writeDisable         */ 0x04,
    /* getFeatures          */ 0x0f,
    /* setFeatures          */ 0x1f,
    /* pageRead             */ 0x13,
    /* readCache            */ 0x0b, //0x03
    /* readCacheX2          */ 0x3b,
    /* readCacheX4          */ 0x6b,
    /* readCacheIO2         */ 0xbb,
    /* readCacheIO4         */ 0xeb,
    /* readID               */ 0x9f,
    /* programLoad          */ 0x02,
    /* programLoadX4        */ 0x32,
    /* progExecute          */ 0x10,
    /* progLoadRand         */ 0x84,
    /* progLoadRandX4       */ 0xc4,
    /* progLoadRandIO2      */ 0x72,
    /* blockErase           */ 0xd8,
    /* reset                */ 0xff
};

spinand_cmd_t spinand_commands_esmt =
{
    /* writeEnable          */ 0x06,
    /* writeDisable         */ 0x04,
    /* getFeatures          */ 0x0f,
    /* setFeatures          */ 0x1f,
    /* pageRead             */ 0x13,
    /* readCache            */ 0x0b, //0x03
    /* readCacheX2          */ 0x3b,
    /* readCacheX4          */ 0x6b,
    /* readCacheIO2         */ 0xbb,
    /* readCacheIO4         */ 0xeb,
    /* readID               */ 0x9f,
    /* programLoad          */ 0x02,
    /* programLoadX4        */ 0x32,
    /* progExecute          */ 0x10,
    /* progLoadRand         */ 0x84,
    /* progLoadRandX4       */ 0x34,
    /* progLoadRandIO2      */ 0x72,
    /* blockErase           */ 0xd8,
    /* reset                */ 0xff
};

spinand_cmd_t spinand_commands_ato =
{
    /* writeEnable          */ 0x06,
    /* writeDisable         */ 0x04,
    /* getFeatures          */ 0x0f,
    /* setFeatures          */ 0x1f,
    /* pageRead             */ 0x13,
    /* readCache            */ 0x0b, //0x03
    /* readCacheX2          */ 0x3b, //not supported
    /* readCacheX4          */ 0x6b,
    /* readCacheIO2         */ 0xbb, //not supported
    /* readCacheIO4         */ 0xeb, //not supported
    /* readID               */ 0x9f,
    /* programLoad          */ 0x02,
    /* programLoadX4        */ 0x32,
    /* progExecute          */ 0x10,
    /* progLoadRand         */ 0x84,
    /* progLoadRandX4       */ 0x34,
    /* progLoadRandIO2      */ 0x72, //not supported
    /* blockErase           */ 0xd8,
    /* reset                */ 0xff
};

spinand_cmd_t spinand_commands_wb =
{
    /* writeEnable          */ 0x06,
    /* writeDisable         */ 0x04,
    /* getFeatures          */ 0x0f,
    /* setFeatures          */ 0x1f,
    /* pageRead             */ 0x13,
    /* readCache            */ 0x0b, //0x03
    /* readCacheX2          */ 0x3b,
    /* readCacheX4          */ 0x6b,
    /* readCacheIO2         */ 0xbb,
    /* readCacheIO4         */ 0xeb,
    /* readID               */ 0x9f,
    /* programLoad          */ 0x02,
    /* programLoadX4        */ 0x32,
    /* progExecute          */ 0x10,
    /* progLoadRand         */ 0x84,
    /* progLoadRandX4       */ 0x34,
    /* progLoadRandIO2      */ 0x72,
    /* blockErase           */ 0xd8,
    /* reset                */ 0xff
};

static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr badblock_ptn_gd = {
    .options = 0,
    .offs = 0,
    .len = 1,
    .pattern = scan_ff_pattern,
};

static struct nand_bbt_descr badblock_ptn_esmt = {
    .options = NAND_BBT_SCAN2NDPAGE,
    .offs = 0,
    .len = 1,
    .pattern = scan_ff_pattern,
};

static struct nand_bbt_descr badblock_ptn_ato = {
    .options = 0,
    .offs = 0,
    .len = 1,
    .pattern = scan_ff_pattern,
};

static struct nand_bbt_descr badblock_ptn_wb = {
    .options = 0,
    .offs = 0,
    .len = 1,
    .pattern = scan_ff_pattern,
};

/*
*******************************************************************************
**
** gd_sflash_devices_supported is an array containing serial flash
** geometry and information data of all supported devices, it will
** be used during GD_SFLASH_Init() to find the connected serial
** flash decive
**
*******************************************************************************
*/
struct spinand_info spinand_devices_supported[] =
{
    {
        /* manufactureID        */ 0xC8,
        /* deviceID             */ 0xF1,
        /* manufactureName      */ "GigaDevice",
        /* deviceName           */ "GD5F1GQ4UAYIG",
        /* nand_size            */ (1024 * (2048 + 64) * 64),
        /* usable_size          */ (1024 * 2048 * 64),
        /* block_size           */ ((2048 + 64) * 64),
        /* block_main_size      */ (2048 * 64),
        /* block_per_chip       */ 1024,
        /* page_size            */ (2048 + 64),
        /* page_main_size       */ 2048,
        /* page_spare_size      */ 64,
        /* page_per_block       */ 64,
        /* block_shift          */ 17,
        /* block_mask           */ 0x1ffff,
        /* page_shift           */ 11,
        /* page_mask            */ 0x7ff,
        /* commands             */ &spinand_commands_gd,
        /* feature              */ SPINAND_1X_WRITE | SPINAND_4X_READ,
        /* channel              */  1,
        /* nand_ecclayout       */ &spinand_ecc_64_layout_gd,
        /* badblockbits         */  8,
        /* badblock_pattern     */ &badblock_ptn_gd
    },
    {
        /* manufactureID        */ 0xC8,
        /* deviceID             */ 0xF2,
        /* manufactureName      */ "GigaDevice",
        /* deviceName           */ "GD5F2GQ4UAYIG",
        /* nand_size            */ (2048 * (2048 + 64) * 64),
        /* usable_size          */ (2048 * 2048 * 64),
        /* block_size           */ ((2048 + 64) * 64),
        /* block_main_size      */ (2048 * 64),
        /* block_per_chip       */ 2048,
        /* page_size            */ (2048 + 64),
        /* page_main_size       */ 2048,
        /* page_spare_size      */ 64,
        /* page_per_block       */ 64,
        /* block_shift          */ 17,
        /* block_mask           */ 0x1ffff,
        /* page_shift           */ 11,
        /* page_mask            */ 0x7ff,
        /* commands             */ &spinand_commands_gd,
        /* feature              */ SPINAND_1X_WRITE | SPINAND_4X_READ,
        /* channel              */  1,
        /* nand_ecclayout       */ &spinand_ecc_64_layout_gd,
        /* badblockbits         */  8,
        /* badblock_pattern     */ &badblock_ptn_gd
    },
    {
        /* manufactureID        */ 0xC8,
        /* deviceID             */ 0xF4,
        /* manufactureName      */ "GigaDevice",
        /* deviceName           */ "GD5F4GQ4UAYIG",
        /* nand_size            */ (4096 * (2048 + 64) * 64),
        /* usable_size          */ (4096 * 2048 * 64),
        /* block_size           */ ((2048 + 64) * 64),
        /* block_main_size      */ (2048 * 64),
        /* block_per_chip       */ 4096,
        /* page_size            */ (2048 + 64),
        /* page_main_size       */ 2048,
        /* page_spare_size      */ 64,
        /* page_per_block       */ 64,
        /* block_shift          */ 17,
        /* block_mask           */ 0x1ffff,
        /* page_shift           */ 11,
        /* page_mask            */ 0x7ff,
        /* commands             */ &spinand_commands_gd,
        /* feature              */ SPINAND_1X_WRITE | SPINAND_4X_READ,
        /* channel              */  1,
        /* nand_ecclayout       */ &spinand_ecc_64_layout_gd,
        /* badblockbits         */  8,
        /* badblock_pattern     */ &badblock_ptn_gd
    },
    {
        /* manufactureID        */ 0xC8,
        /* deviceID             */ 0x20,
        /* manufactureName      */ "ESMT",
        /* deviceName           */ "F50L512M41A",
        /* nand_size            */ (512 * (2048 + 64) * 64),
        /* usable_size          */ (512 * 2048 * 64),
        /* block_size           */ ((2048 + 64) * 64),
        /* block_main_size      */ (2048 * 64),
        /* block_per_chip       */ 512,
        /* page_size            */ (2048 + 64),
        /* page_main_size       */ 2048,
        /* page_spare_size      */ 64,
        /* page_per_block       */ 64,
        /* block_shift          */ 17,
        /* block_mask           */ 0x1ffff,
        /* page_shift           */ 11,
        /* page_mask            */ 0x7ff,
        /* commands             */ &spinand_commands_esmt,
        /* feature              */ SPINAND_1X_WRITE | SPINAND_4X_READ,
        /* channel              */  1,
        /* nand_ecclayout       */ &spinand_ecc_64_layout_esmt,
        /* badblockbits         */  8,
        /* badblock_pattern     */ &badblock_ptn_esmt
    },
    {
        /* manufactureID        */ 0x9b,
        /* deviceID             */ 0x12,
        /* manufactureName      */ "ATO",
        /* deviceName           */ "ATO25D1GA",
        /* nand_size            */ (1024 * (2048 + 64) * 64),
        /* usable_size          */ (1024 * 2048 * 64),
        /* block_size           */ ((2048 + 64) * 64),
        /* block_main_size      */ (2048 * 64),
        /* block_per_chip       */ 1024,
        /* page_size            */ (2048 + 64),
        /* page_main_size       */ 2048,
        /* page_spare_size      */ 64,
        /* page_per_block       */ 64,
        /* block_shift          */ 17,
        /* block_mask           */ 0x1ffff,
        /* page_shift           */ 11,
        /* page_mask            */ 0x7ff,
        /* commands             */ &spinand_commands_ato,
        /* feature              */ SPINAND_1X_WRITE | SPINAND_4X_READ,
        /* channel              */  1,
        /* nand_ecclayout       */ &spinand_ecc_64_layout_ato,
        /* badblockbits         */  8,
        /* badblock_pattern     */ &badblock_ptn_ato
    },
    {
        /* manufactureID        */ 0xEF,
        /* deviceID             */ 0xAA21,
        /* manufactureName      */ "Winbond",
        /* deviceName            */ "W25N01GV",
        /* nand_size            */ (1024 * (2048 + 64) * 64),
        /* usable_size            */ (1024 * 2048 * 64),
        /* block_size            */ ((2048 + 64) * 64),
        /* block_main_size        */ (2048 * 64),
        /* block_per_chip        */ 1024,
        /* page_size            */ (2048 + 64),
        /* page_main_size        */ 2048,
        /* page_spare_size        */ 64,
        /* page_per_block        */ 64,
        /* block_shift            */ 17,
        /* block_mask            */ 0x1ffff,
        /* page_shift            */ 11,
        /* page_mask            */ 0x7ff,
        /* commands             */ &spinand_commands_wb,
        /* feature                */ SPINAND_1X_WRITE | SPINAND_4X_READ,
        /* channel                */    1,
        /* nand_ecclayout        */ &spinand_ecc_64_layout_wb,
        /* badblockbits         */  8,
        /* badblock_pattern     */ &badblock_ptn_wb
    },
};

struct gk_spinand_platform_data
{
    uint32_t  speed_mode;
    uint32_t  channel;
    struct mtd_partition *parts;
    unsigned int    nr_parts;
};

static struct mtd_partition gk_spinand_partitions[ ] = {
   [0] = {
        .name   = "uboot",
            .offset = 0,
        .size   = SZ_1M,
   },
   [1] = {
        .name    = "uboot-env",
        .offset  = MTDPART_OFS_NXTBLK,
        .size    = SZ_1M,
    },
   [2] = {
        .name    = "user",
        .offset  = MTDPART_OFS_NXTBLK,
        .size    = SZ_4M,
    },
   [3] = {
        .name    = "config",
        .offset  = MTDPART_OFS_NXTBLK,
        .size    = SZ_1M,
    },
    [4] = {
        .name    = "kernel",
        .offset  = MTDPART_OFS_NXTBLK,
        .size    = SZ_4M,
    },
    [5] = {
        .name    = "rootfs",
        .offset  = MTDPART_OFS_NXTBLK,
        .size    = SZ_4M * 28,          // 1g bit nand sflash
    },
    [6] = {
        .name    = "resource",
        .offset  = MTDPART_OFS_NXTBLK,
        .size    = SZ_4M,
    },
};

#define NUM_PARTITIONS  (ARRAY_SIZE(gk_spinand_partitions))

static int gk_spinand_set_channel(struct spinand_info *info)
{
    uint32_t ce_setting     = 0;
    uint32_t new_ce_setting = 0;

    if( info && info->mid)
    {
        ce_setting = spinand_get_ce();
        new_ce_setting = (ce_setting & ~(1 << 6)) | ((uint32_t)info->channel << 6);
        if(ce_setting != new_ce_setting)
        {
            spinand_set_ce_chselect((uint8_t)info->channel);
        }
        return 0;
    }
    return -EINVAL;
}

static int gk_spinand_release_channel(struct spinand_info *info)
{
    uint32_t ce_setting     = 0;
    uint32_t new_ce_setting = 0;

    if( info && info->mid)
    {
        ce_setting = spinand_get_ce();
        new_ce_setting = ce_setting & (~((uint32_t)info->channel << 6));
        if(ce_setting != new_ce_setting)
        {
            spinand_set_ce_chselect(!((uint8_t)info->channel));
        }
        return 0;
    }
    return -EINVAL;
}
/**
 * gk_spinand_write_enable - send command 0x06 to enable write or erase the Nand cells
 *
 * Description:
 *   Before write and erase the Nand cells, the write enable has to be set.
 *   After the write or erase, the write enable bit is automatically cleared( status register bit 2 )
 *   Set the bit 2 of the status register has the same effect
 */
static uint32_t gk_spinand_write_enable(struct spinand_chip *chip, bool enable)
{
    struct spinand_info *info = chip->info;

    flash_command_s SflashCmd = {0};

    uint32_t sflashData;
    uint32_t cmdBitSet = 0;

    //
    // issue a write command sequence to prepare
    // the device for data to be written
    //
    cmdBitSet |= (FLASH_SEND_CMD                   |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_0       |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                  FLASH_RWN_NOTHING                |   // no read/write data
                  FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0           |   // set the sflash data mode
                  (0 << FLASH_TRANSFER_BYTE_LOC)   |   // transfer data number
                  FLASH_HOLD_TIME_100ns
                 );

    if(enable == WR_ENABLE)
    {
        cmdBitSet |= (uint32_t)info->commands->writeEnable; // command
    }
    else
    {
        cmdBitSet |= (uint32_t)info->commands->writeDisable; // command
    }

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    sflashData = spinand_get_data();

    return sflashData;

}

/**
 * gk_spinand_set_feature - send command 0x1f to the SPI Nand status register
 *
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the busy status.
 *    This function is to allow reading the status of the command: read, write, and erase.
 *    Once the status turns to be ready, the other status bits also are valid status bits.
 */
static int gk_spinand_set_feature(struct spinand_chip *chip, uint32_t off, uint8_t input)
{
    struct spinand_info *info = chip->info;

    uint32_t cmdBitSet = 0;

    flash_command_s     SflashCmd = {0};

    cmdBitSet |= (info->commands->setFeatures      |   // command
                  FLASH_SEND_CMD                   |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_1       |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                  FLASH_RWN_WRITE                  |   // write data
                  FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0           |   // set the sflash data mode
                  (1 << FLASH_TRANSFER_BYTE_LOC)   |   // transfer data number
                  FLASH_HOLD_TIME_100ns
                 );

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(off);
    spinand_set_data(input);

    return 0;

}

/**
 * gk_spinand_get_feature - send command 0xf to the SPI Nand status register
 *
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the busy status.
 *    This function is to allow reading the status of the command: read, write, and erase.
 *    Once the status turns to be ready, the other status bits also are valid status bits.
 */
static int gk_spinand_get_feature(struct spinand_chip *chip, uint32_t off, uint32_t *output)
{
    struct spinand_info *info = chip->info;

    uint32_t sflashData;
    uint32_t cmdBitSet = 0;

    flash_command_s     SflashCmd = {0};

    cmdBitSet |= (info->commands->getFeatures      |   // command
                  FLASH_SEND_CMD                   |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_1       |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                  FLASH_RWN_READ                   |   // read data
                  FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0           |   // set the sflash data mode
                  (1 << FLASH_TRANSFER_BYTE_LOC)   |   // transfer data number
                  FLASH_HOLD_TIME_100ns
                  );

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(off);

    sflashData = spinand_get_data();
    *output    = sflashData;

    return 0;

}

/**
 * spinand_reset- send reset command "0xff" to the Nand device
 *
 * Description:
 *    Reset the SPI Nand with the reset command 0xff
 */
static int gk_spinand_reset(struct spinand_chip *chip)
{
    struct spinand_info *info = chip->info;

    uint32_t cmdBitSet = 0;

    flash_command_s     SflashCmd = {0};

    gk_spinand_set_channel(info);

    cmdBitSet |= (info->commands->reset            |   // command
                  FLASH_SEND_CMD                   |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_0       |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                  FLASH_RWN_NOTHING                |   // read data
                  FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0           |   // set the sflash data mode
                  (1 << FLASH_TRANSFER_BYTE_LOC)   |   // transfer data number
                  FLASH_HOLD_TIME_100ns
                  );

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    gk_spinand_release_channel(info);
    return 0;

}
/**
 * spinand_read_id- Read SPI Nand ID
 *
 * Description:
 *    Read ID: read two ID bytes from the SPI Nand device
 */
static int gk_spinand_read_id(struct spinand_chip *chip, uint32_t* id)
{
    struct spinand_info *info = chip->info;

    uint32_t sflashData;
    uint32_t cmdBitSet = 0;

    flash_command_s     SflashCmd;

    gk_spinand_set_channel(info);

    // spi nand flash: send readID cmd
    cmdBitSet |= (FLASH_SEND_CMD|                    // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_1|        // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0|       // dummy cycle
                  FLASH_RWN_READ|                    // read data
                  FLASH_CMD_MODE_1X|                 // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0|      // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0|            // set the sflash data mode
                  (3 << FLASH_TRANSFER_BYTE_LOC)       // transfer data number
                  );

    cmdBitSet |= (uint32_t)info->commands->readID; // readID command
    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(0x00);
    sflashData = spinand_get_data();

    *id = sflashData;

    gk_spinand_release_channel(info);

    return 0;

}

/**
 * gk_spinand_unlock_allblock- send write register 0x1f command to the Nand device
 *
 * Description:
 *    After power up, all the Nand blocks are locked.  This function allows
 *    one to unlock the blocks, and so it can be wriiten or erased.
 */
static int gk_spinand_unlock_allblock(struct spinand_chip *chip)
{
    struct spinand_info *info = chip->info;

    gk_spinand_set_channel(info);
    gk_spinand_set_feature(chip , REG_PROTECTION, BL_ALL_UNLOCKED);
    gk_spinand_release_channel(info);
    return 0;
}

#if 0

/**
 * gk_spinand_enable_bbi - send command 0x1f to write the SPI Nand OTP register
 *
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal BBI, set the bit to 1
 *   Disable chip internal BBI, clear the bit to 0
 */
static int gk_spinand_enable_bbi(struct spinand_chip *chip)
{
    struct spinand_info *info = chip->info;
    uint32_t ufeature = 0;

    gk_spinand_set_channel(info);

    gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);

    if ((ufeature & OTP_BBI_MASK) == OTP_BBI_MASK)
    {
        return 0;
    }
    else
    {
        ufeature |= OTP_BBI_MASK;
        gk_spinand_set_feature(chip, REG_FEATURE, (uint8_t)(ufeature & 0xff));
        gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);
        return ufeature;
    }
}

static int gk_spinand_disable_bbi(struct spinand_chip *chip)
{
    struct spinand_info *info = chip->info;
    uint32_t ufeature = 0;

    gk_spinand_set_channel(info);

    gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);

    if ((ufeature & OTP_BBI_MASK) == OTP_BBI_MASK)
    {
        ufeature &= ~OTP_BBI_MASK;
        gk_spinand_set_feature(chip, REG_FEATURE, (uint8_t)(ufeature & 0xff));
        gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);
        return ufeature;
    }
    else
    {
        return 0;
    }
}

#endif

/**
 * gk_spinand_enable_ecc - send command 0x1f to write the SPI Nand OTP register
 *
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
#ifdef CONFIG_MTD_SPINAND_INTERECC
static int gk_spinand_enable_ecc(struct spinand_chip *chip)
{
    struct spinand_info *info = chip->info;
    uint32_t ufeature = 0;

    gk_spinand_set_channel(info);

    gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);

    if ((ufeature & OTP_ECC_MASK) == OTP_ECC_MASK)
    {
        gk_spinand_release_channel(info);
        return 0;
    }
    else
    {
        ufeature |= OTP_ECC_MASK;
        gk_spinand_set_feature(chip, REG_FEATURE, (uint8_t)(ufeature & 0xff));
        gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);
        gk_spinand_release_channel(info);
        return ufeature;
    }
}
#endif

static int gk_spinand_disable_ecc(struct spinand_chip *chip)
{
    struct spinand_info *info = chip->info;
    uint32_t ufeature = 0;

    gk_spinand_set_channel(info);

    gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);

    if ((ufeature & OTP_ECC_MASK) == OTP_ECC_MASK)
    {
        ufeature &= ~OTP_ECC_MASK;
        gk_spinand_set_feature(chip, REG_FEATURE, (uint8_t)(ufeature & 0xff));
        gk_spinand_get_feature(chip, REG_FEATURE, &ufeature);
        gk_spinand_release_channel(info);
        return ufeature;
    }
    else
    {
        gk_spinand_release_channel(info);
        return 0;
    }
}

static int gk_spinand_read_page_to_cache(struct spinand_chip *chip, uint32_t page_id)
{
    struct spinand_info *info = chip->info;

    flash_command_s SflashCmd = {0};

    uint32_t cmdBitSet = 0;

    // 1. page read to cache
    cmdBitSet |= (info->commands->pageRead     |   // command
                  FLASH_SEND_CMD               |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_3   |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0  |   // dummy cycle
                  FLASH_RWN_READ               |   // read data
                  FLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0       |   // set the sflash data mode
                  FLASH_TRANSFER_BYTE_NUM_4        // transfer data number
                 );

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(page_id);            // here address should be page/block address,

    return 0;

}

/**
 * spinand_read_from_cache- send command 0x03 to read out the data from the cache register( 2112 bytes max )
 *
 * Description:
 *   The read can specify 1 to 2112 bytes of data read at the coresponded locations.
 *   No tRd delay.
 */
static int gk_spinand_read_from_cache(
            struct spinand_chip *chip,
            uint32_t byte_id, uint32_t len,
            uint8_t* rbuf)
{
    struct spinand_info *info = chip->info;

    flash_command_s SflashCmd = {0};

    uint32_t    i, ilen, tmpData;
    uint32_t    cmdBitSet = 0;
    uint32_t    feature = FLASH_FEATURE_IO1;
    uint32_t    *pData = (uint32_t*)chip->tmpbuf;

    switch((info->feature & 0x0f))
    {
        default:
        case 0x01:
            feature = FLASH_FEATURE_IO1;
            break;
        case 0x02:
            feature = FLASH_FEATURE_IO2;
            break;
        case 0x04:
            feature = FLASH_FEATURE_IO4;
            break;
    }

    // 3. read from cache
    if(feature == FLASH_FEATURE_IO4)
    {
        cmdBitSet |= (info->commands->readCacheX4  |   // command
                      FLASH_SEND_CMD               |   // transfer the command
                      FLASH_SEND_ADDR_BYTE_NUM_2   |   // address num
                      FLASH_SEND_DUMMY_BYTE_NUM_1  |   // dummy cycle
                      FLASH_RWN_READ               |   // read data
                      FLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                      FLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                      FLASH_DATA_CYCLE_NUM_2       |   // set the sflash data mode
                      FLASH_TRANSFER_BYTE_NUM_4        // transfer data number
                     );
    }
    else if(feature == FLASH_FEATURE_IO2)
    {
        cmdBitSet |= (info->commands->readCacheX2  |   // command
                      FLASH_SEND_CMD               |   // transfer the command
                      FLASH_SEND_ADDR_BYTE_NUM_2   |   // address num
                      FLASH_SEND_DUMMY_BYTE_NUM_1  |   // dummy cycle
                      FLASH_RWN_READ               |   // read data
                      FLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                      FLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                      FLASH_DATA_CYCLE_NUM_1       |   // set the sflash data mode
                      FLASH_TRANSFER_BYTE_NUM_4        // transfer data number
                     );
    }
    else
    {
        cmdBitSet |= (info->commands->readCache    |   // command
                      FLASH_SEND_CMD               |   // transfer the command
                      FLASH_SEND_ADDR_BYTE_NUM_2   |   // address num
                      FLASH_SEND_DUMMY_BYTE_NUM_1  |   // dummy cycle
                      FLASH_RWN_READ               |   // read data
                      FLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                      FLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                      FLASH_DATA_CYCLE_NUM_0       |   // set the sflash data mode
                      FLASH_TRANSFER_BYTE_NUM_4        // transfer data number
                     );
    }

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(byte_id);            // Set the start address what you want to read

    ilen = ((len - 1) >> 2) + 1;

    for(i = 0; i < ilen; i++)
    {
        tmpData = spinand_get_data();
        *pData++ = uswap_32(tmpData);
    }

    memcpy(rbuf, (uint8_t*)chip->tmpbuf, len);

    return 0;

}

/**
 * spinand_read_page_raw-to read a page with:
 * @page_id: the physical page number
 * @offset:  the location from 0 to 2111
 * @len:     number of bytes to read
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The read icludes two commands to the Nand: 0x13 and 0x03 commands
 *   Poll to read status to wait for tRD time.
 */
static int gk_spinand_read_page_raw(
            struct spinand_chip *chip,
            uint32_t page_id, uint32_t offset,
            uint32_t len, uint8_t* rbuf)
{
    ssize_t     retval;
    uint32_t    status = 0;
    struct spinand_info *info = chip->info;

    #ifdef CONFIG_DEBUG_R
    pr_info("[%s]: PageID = 0x%08x, Offset = 0x%08x, len = %d, rbufadd = 0x%x\n",
            __FUNCTION__, page_id, offset, len, rbuf);
    #endif

#ifdef CONFIG_MTD_SPINAND_INTERECC
    gk_spinand_disable_ecc(chip);
#endif

    gk_spinand_set_channel(info);

    retval = gk_spinand_read_page_to_cache(chip, page_id);

    while (1)
    {
        gk_spinand_get_feature(chip, REG_STATUS, &status);

        #ifdef CONFIG_DEBUG_R
        pr_info("[%s_%d]: status = 0x%08x\n",
                __FUNCTION__, __LINE__,status);
        #endif

        if ((status & STATUS_OIP_MASK) == STATUS_READY)
        {
            break;
        }
    }

    retval = gk_spinand_read_from_cache(chip, offset, len, rbuf);

    gk_spinand_release_channel(info);

#ifdef CONFIG_MTD_SPINAND_INTERECC
    gk_spinand_enable_ecc(chip);
#endif

    return retval;

}


/**
 * spinand_read_page-to read a page with:
 * @page_id: the physical page number
 * @offset:  the location from 0 to 2111
 * @len:     number of bytes to read
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The read icludes two commands to the Nand: 0x13 and 0x03 commands
 *   Poll to read status to wait for tRD time.
 */
static int gk_spinand_read_page(
            struct spinand_chip *chip,
            uint32_t page_id, uint32_t offset,
            uint32_t len, uint8_t* rbuf)
{
    struct spinand_info *info = chip->info;
    ssize_t     retval;
    uint32_t    status = 0;

    #ifdef CONFIG_DEBUG_R
    pr_info("[%s]: PageID = 0x%08x, Offset = 0x%08x, len = %d, rbufadd = 0x%x\n",
            __FUNCTION__, page_id, offset, len, rbuf);
    #endif

    gk_spinand_set_channel(info);

    retval = gk_spinand_read_page_to_cache(chip, page_id);

    while (1)
    {
        gk_spinand_get_feature(chip, REG_STATUS, &status);

        #ifdef CONFIG_DEBUG_R
        pr_info("[%s_%d]: status = 0x%08x\n",
                __FUNCTION__, __LINE__,status);
        #endif

        if ((status & STATUS_OIP_MASK) == STATUS_READY)
        {
            if ((status & STATUS_ECC_MASK) == STATUS_ECC_ERROR)
            {
                pr_err("[%s]ecc error, page = 0x%08x\n", __FUNCTION__, page_id);
                return -1;
            }
            break;
        }
    }

    retval = gk_spinand_read_from_cache(chip, offset, len, rbuf);

    gk_spinand_release_channel(info);

    return 0;

}

#define     RET_OK  0

#define     WRITE_BYTE      0x1
#define     WRITE_WORD      0x2
#define     WRITE_LWORD     0x4
#define     WRITE_CLWORD    0x1f


static int gk_spinand_write_to_cache(
            struct spinand_chip *chip, uint32_t offset, uint32_t dflag)
{
    struct spinand_info *info = chip->info;

    flash_command_s SflashCmd = {0};

    uint32_t    cmdBitSet = 0;
    uint32_t    feature = FLASH_FEATURE_IO1;

#ifdef CONFIG_DEBUG_W
    printk("[DEBUG_W]: %s\n", __FUNCTION__);
    {
        int i = 0, j = 0;
        uint8_t* buf = (uint8_t*)chip->tmpbuf;

        for(i = 0,j = 0; i < 128; i++,j++)
        {
            if(j % 16 == 0)
            {
                printk("\n");
            }
            printk("%02x ", buf[i]);
        }

        printk("\n");
    }
#endif


    switch(((info->feature >> 4) & 0x0f))
    {
        default:
        case 0x01:
            feature = FLASH_FEATURE_IO1;
            break;
        case 0x02:
            feature = FLASH_FEATURE_IO2;
            break;
        case 0x04:
            feature = FLASH_FEATURE_IO4;
            break;
    }

    if(feature == FLASH_FEATURE_IO4)
    {
        cmdBitSet |= (info->commands->programLoadX4    |   // command
                      FLASH_SEND_CMD                   |   // transfer the command
                      FLASH_SEND_ADDR_BYTE_NUM_2       |   // address num
                      FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                      FLASH_RWN_WRITE                  |   // write data
                      FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                      FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                      FLASH_DATA_CYCLE_NUM_2           |   // set the sflash data mode
                      dflag << FLASH_TRANSFER_BYTE_LOC |   // transfer data number
                      FLASH_HOLD_TIME_100ns
                     );
    }
    else if(feature == FLASH_FEATURE_IO2)
    {
        /* no supported */
        cmdBitSet |= (info->commands->programLoad      |   // command
                      FLASH_SEND_CMD                   |   // transfer the command
                      FLASH_SEND_ADDR_BYTE_NUM_2       |   // address num
                      FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                      FLASH_RWN_WRITE                  |   // write data
                      FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                      FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                      FLASH_DATA_CYCLE_NUM_1           |   // set the sflash data mode
                      dflag << FLASH_TRANSFER_BYTE_LOC |   // transfer data number
                      FLASH_HOLD_TIME_100ns
                     );
    }
    else
    {
        cmdBitSet |= (info->commands->programLoad      |   // command
                      FLASH_SEND_CMD                   |   // transfer the command
                      FLASH_SEND_ADDR_BYTE_NUM_2       |   // address num
                      FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                      FLASH_RWN_WRITE                  |   // write data
                      FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                      FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                      FLASH_DATA_CYCLE_NUM_0           |   // set the sflash data mode
                      dflag << FLASH_TRANSFER_BYTE_LOC |   // transfer data number
                      FLASH_HOLD_TIME_100ns
                     );
    }

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(offset);            // Set the start address what you want to write

    return RET_OK;

}

/**
 * gk_spinand_program_data_to_cache -- to write a page to cache with:
 * @byte_id: the location to write to the cache
 * @len:     number of bytes to write
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The write command used here is 0x84--indicating that the cache is not cleared first.
 *   Since it is writing the data to cache, there is no tPROG time.
 */
static int gk_spinand_program_data_to_cache(
            struct spinand_chip *chip, uint32_t offset,
            uint32_t len, uint8_t* wbuf)
{


    uint32_t    i, u4Len, uleftLen, dflag = WRITE_CLWORD, wdata, doff;
    uint32_t    uTmpData;

#ifndef  CONFIG_NAND_SPL
    uint32_t    *pData = (uint32_t*)chip->tmpbuf;
#else
    uint32_t    *pData = (uint32_t*)wbuf;
#endif

    /* calc the length of data(unit:long word) */
    u4Len = len >> 2;
    uleftLen = len % 4;
    doff = 0;

#ifndef  CONFIG_NAND_SPL
    /* copy data to temporary 4 bytes align buffer */
    memcpy((uint8_t*)pData, (wbuf + offset), (u4Len << 2));
#endif

    /* write the data which length is a multiple of 4 bytes to cache firstly */
    if(u4Len > 0)
    {
        dflag = WRITE_CLWORD;
        gk_spinand_write_to_cache(chip, offset, dflag);
        for(i = 0; i < u4Len; i++)
        {
            uTmpData = *pData++;
            uTmpData = uswap_32(uTmpData);
            spinand_set_data(uTmpData);
        }

        doff += (u4Len << 2);
    }

    i = 0;

    /* write the left data to cache */
    while(uleftLen)
    {
        dflag = WRITE_BYTE;
        doff += i;
        wdata = (uint32_t)wbuf[doff];
        gk_spinand_write_to_cache(chip, (offset + doff), dflag);
        spinand_set_data(wdata);
        uleftLen--;
    }

    return RET_OK;

}

/**
 * gk_spinand_program_execute -- to write a page from cache to the Nand array with:
 * @page_id: the physical page location to write the page.
 *
 * Description:
 *   The write command used here is 0x10--indicating the cache is writing to the Nand array.
 *   Need to wait for tPROG time to finish the transaction.
 */
static int gk_spinand_program_execute(struct spinand_chip *chip, uint32_t page_id)
{
    struct spinand_info *info = chip->info;

    flash_command_s SflashCmd = {0};

    uint32_t cmdBitSet = 0;

    cmdBitSet |= (info->commands->progExecute       |   // command
                  FLASH_SEND_CMD                    |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_3        |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0       |   // dummy cycle
                  FLASH_RWN_WRITE                   |   // write data
                  FLASH_CMD_MODE_1X                 |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0      |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0            |   // set the sflash data mode
                  FLASH_TRANSFER_BYTE_NUM_4         |   // transfer data number
                  FLASH_HOLD_TIME_100ns
                 );

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(page_id);            // here address should be page/block address,

    return 0;

}

/**
 * gk_spinand_program_page_raw--to write a page with:
 * @page_id: the physical page location to write the page.
 * @offset:  the location from the cache starting from 0 to 2111
 * @len:     the number of bytes to write
 * @wbuf:    the buffer to hold the number of bytes
 *
 * Description:
 *   The commands used here are 0x06, 0x84, and 0x10--indicating that the write enable is first
 *   sent, the write cache command, and the write execute command
 *   Poll to wait for the tPROG time to finish the transaction.
 */
static int gk_spinand_program_page_raw(
            struct spinand_chip *chip,
            uint32_t page_id, uint32_t offset,
            uint32_t len, uint8_t* wbuf)
{
    ssize_t     retval;
    uint32_t    status = 0;
    struct spinand_info *info = chip->info;

    #ifdef CONFIG_DEBUG_W
    pr_info("[%s]: PageID = 0x%08x, rbufadd = 0x%x\n",
            __FUNCTION__, page_id, wbuf);
    #endif

#ifdef CONFIG_MTD_SPINAND_INTERECC
    gk_spinand_disable_ecc(chip);
#endif

    gk_spinand_set_channel(info);

    retval = gk_spinand_write_enable(chip, WR_ENABLE);

    retval = gk_spinand_program_data_to_cache(chip, offset, len, wbuf);

    retval = gk_spinand_program_execute(chip, page_id);

    while (1)
    {
        gk_spinand_get_feature(chip, REG_STATUS, &status);

        if ((status & STATUS_OIP_MASK) == STATUS_READY)
        {
            break;
        }
    }

    gk_spinand_release_channel(info);

#ifdef CONFIG_MTD_SPINAND_INTERECC
    gk_spinand_enable_ecc(chip);
#endif


    return 0;

}

/**
 * spinand_program_page--to write a page with:
 * @page_id: the physical page location to write the page.
 * @offset:  the location from the cache starting from 0 to 2111
 * @len:     the number of bytes to write
 * @wbuf:    the buffer to hold the number of bytes
 *
 * Description:
 *   The commands used here are 0x06, 0x84, and 0x10--indicating that the write enable is first
 *   sent, the write cache command, and the write execute command
 *   Poll to wait for the tPROG time to finish the transaction.
 */
static int gk_spinand_program_page(
            struct spinand_chip *chip,
            uint32_t page_id, uint32_t offset,
            uint32_t len, uint8_t* wbuf)
{
    struct spinand_info *info = chip->info;
    ssize_t     retval;
    uint32_t    status = 0;

#ifdef CONFIG_DEBUG_W
    pr_info("[%s]: blockId = %d, PageID = 0x%08x\n",
            __FUNCTION__, page_id/info->page_per_block, page_id);
#endif

    gk_spinand_set_channel(info);

    retval = gk_spinand_write_enable(chip, WR_ENABLE);

    retval = gk_spinand_program_data_to_cache(chip, offset, len, wbuf);

    retval = gk_spinand_program_execute(chip, page_id);

    while (1)
    {
        gk_spinand_get_feature(chip, REG_STATUS, &status);

        if ((status & STATUS_OIP_MASK) == STATUS_READY)
        {
            if ((status & STATUS_P_FAIL_MASK) == STATUS_P_FAIL)
            {
                pr_err("[%s]: program error, page = 0x%08x\n", __FUNCTION__, page_id);
                gk_spinand_release_channel(info);
                return -1;
            }
            else
            {
                break;
            }
        }
    }
    gk_spinand_release_channel(info);

    return 0;

}

/**
 * spinand_erase_block_do -- to erase a block with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The command used here is 0xd8--indicating an erase command to erase one block--64 pages
 *   Need to wait for tERS.
 */
static int gk_spinand_erase_block_do(struct spinand_chip *chip, uint32_t block_id)
{
    struct spinand_info *info = chip->info;

    uint32_t cmdBitSet = 0;

    flash_command_s     SflashCmd = {0};

    cmdBitSet |= (info->commands->blockErase       |   // command
                  FLASH_SEND_CMD                   |   // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_3       |   // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0      |   // dummy cycle
                  FLASH_RWN_WRITE                  |   // write data
                  FLASH_CMD_MODE_1X                |   // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0     |   // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0           |   // set the sflash data mode
                  FLASH_TRANSFER_BYTE_NUM_4        |   // transfer data number
                  FLASH_HOLD_TIME_100ns
                 );

    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(block_id);

    return 0;

}

/**
 * spinand_erase_block--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The commands used here are 0x06 and 0xd8--indicating an erase command to erase one block--64 pages
 *   It will first to enable the write enable bit ( 0x06 command ), and then send the 0xd8 erase command
 *   Poll to wait for the tERS time to complete the tranaction.
 */
static int gk_spinand_erase_block(struct spinand_chip *chip, uint32_t block_id)
{
    struct spinand_info *info = chip->info;
    ssize_t     retval;
    uint32_t    status = 0;

    gk_spinand_set_channel(info);

    retval = gk_spinand_write_enable(chip, WR_ENABLE);

    retval = gk_spinand_erase_block_do(chip, block_id);

    while(1)
    {
        gk_spinand_get_feature(chip, REG_STATUS, &status);

        if ((status & STATUS_OIP_MASK) == STATUS_READY)
        {
            if ((status & STATUS_E_FAIL_MASK) == STATUS_E_FAIL)
            {
                pr_err("%s: erase error, block = %d\n", __FUNCTION__, block_id/info->page_per_block);
                gk_spinand_release_channel(info);
                return -1;
            }
            else
                break;
        }
    }
    gk_spinand_release_channel(info);

    return 0;

}

/**************************************************************/
/*                                                                                            */
/**************************************************************/
static struct spinand_info* gk_get_spinand_info(struct platform_device *dev)
{
    uint32_t sflashData;
    uint8_t  mID;
    uint16_t dID;
    uint32_t index;
    uint32_t count;
    uint32_t CESetting = 0x0E;
    uint32_t cmdBitSet = 0;

    struct spinand_info* sninfo;
    struct gk_spinand_platform_data *platform_data = dev->dev.platform_data;
    flash_command_s     SflashCmd;

    uint32_t channel    =  platform_data->channel;
    uint32_t speed_mode =  platform_data->speed_mode;

    pr_info("%s: SPINAND Channel = %d, SpeedMode = %d !\n", __FUNCTION__, channel, speed_mode);

    if(channel >= FLASH_CHANNEL_NUM)
    {
        return NULL;
    }

    CESetting = CESetting | ((uint32_t)channel  << 6);

    spinand_set_ce (CESetting);
    spinand_set_speed (speed_mode); //0 -- 1/2freq 1--1/4freq  2....4

    CESetting = spinand_get_ce();
    speed_mode = spinand_get_speed();
//    pr_info("%s: CE = 0x%08x, SpeedMode = 0x%08x !\n", __FUNCTION__, CESetting, speed_mode);

    // spi nand flash: send readID cmd
    cmdBitSet |= (FLASH_SEND_CMD|                    // transfer the command
                  FLASH_SEND_ADDR_BYTE_NUM_1|        // address num
                  FLASH_SEND_DUMMY_BYTE_NUM_0|       // dummy cycle
                  FLASH_RWN_READ|                    // read data
                  FLASH_CMD_MODE_1X|                 // set the sflash cmd mode
                  FLASH_ADDR_DUMMY_CYCLE_NUM_0|      // set the sflash adr mode
                  FLASH_DATA_CYCLE_NUM_0|            // set the sflash data mode
                  (3 << FLASH_TRANSFER_BYTE_LOC)       // transfer data number
                  );

    cmdBitSet |= CMD_READ_ID; // readID command
    SflashCmd.all = cmdBitSet;
    spinand_set_command(SflashCmd.all);
    spinand_set_data(0x00);
    sflashData = spinand_get_data();

    mID = (uint8_t)((sflashData & 0x00FF0000) >> 16);
    dID = (uint16_t)(sflashData & 0x0000FFFF);

    pr_info("%s: SPINAND MID = 0x%x, DID = 0x%x, Data = 0x%x !\n",
                __FUNCTION__, mID, dID, sflashData);

    count = sizeof(spinand_devices_supported)/sizeof(struct spinand_info);

    for(index = 0; index < count; index++)
    {
        sninfo = &(spinand_devices_supported[index]);
        if((mID == sninfo->mid) &&
           (dID == sninfo->did))
        {
            spinand_set_ce_chselect(!channel);
            return( sninfo );
        }
    }
    spinand_set_ce_chselect(!channel);

    return NULL;

}

/**
 * spinand_probe - [spinand Interface]
* @spi_nand: registered device driver.
 *
 * Description:
 *   To set up the device driver parameters to make the device available.
 */
static int __devinit gk_spinand_probe(struct platform_device *pdev)
{
    struct gk_spinand_host *host;
    struct spinand_chip *chip;
    struct spinand_info *info;

    int res = 0;

    info = gk_get_spinand_info(pdev);
    if (!info)
    {
        dev_err(&pdev->dev, "No support this SPI nand!\n");
        return -ENOMEM;
    }

    pr_err("%s: %s (%lld Kbytes)\n", __FUNCTION__, info->deviceName,
            (long long)info->nand_size >> 10);

    host = kzalloc(sizeof(struct gk_spinand_host), GFP_KERNEL);
    if (!host)
    {
        dev_err(&pdev->dev, "no memory for NAND host context\n");
        return -ENOMEM;
    }
    host->io_base = (void*)REG_SFLASH_DATA;

    platform_set_drvdata(pdev, host);

    /* Initialize the SPI nand chip information */
    chip = &(host->chip);
    chip->mtd = &host->mtd;
    chip->priv = host;
    chip->info = info;

    chip->reset             = gk_spinand_reset;
    chip->read_id           = gk_spinand_read_id;
    chip->read_page         = gk_spinand_read_page;
    chip->read_page_raw     = gk_spinand_read_page_raw;
    chip->program_page      = gk_spinand_program_page;
    chip->program_page_raw  = gk_spinand_program_page_raw;
    chip->erase_block       = gk_spinand_erase_block;
    chip->scan_bbt          = NULL;
    chip->block_markbad     = NULL;
    chip->block_bad         = NULL;
    chip->chip_delay        = 50;
    chip->chip_id           = info->channel;
    chip->chipsize          = info->usable_size;
    chip->options           = NAND_NO_READRDY;

    chip->badblockpos       = info->badblock_pattern->offs;
    chip->badblockbits      = info->badblockbits;
//    chip->bbt_options       = NAND_BBT_NO_OOB | NAND_BBT_USE_FLASH
//                            | NAND_BBT_CREATE | NAND_BBT_WRITE;
    chip->bbt_options       = NAND_BBT_NO_OOB | NAND_BBT_CREATE | NAND_BBT_WRITE;
    chip->bbt_erase_shift   = chip->phys_erase_shift = ffs(info->block_main_size) - 1;
    chip->page_shift        = info->page_shift;
    chip->bbt               = NULL;
    chip->bbt_td            = NULL;
    chip->bbt_md            = NULL;
    chip->badblock_pattern  = info->badblock_pattern;

    /* Convert chipsize to number of pages per chip -1. */
    chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

    mutex_init(&chip->lock);

    chip->buffers = kzalloc(sizeof(struct nand_buffers), GFP_KERNEL);
    if (!chip->buffers)
    {
        dev_err(&pdev->dev, "[%s]no memory for NAND data buffer\n", __FUNCTION__);
        res = -ENOMEM;
        goto fail_err0;
    }
    chip->pagebuf   = chip->buffers->databuf;

    /* Set the internal oob buffer location, just after the page data */
    chip->oob_poi   = (chip->pagebuf + info->page_main_size);

    chip->oobbuf = kzalloc((info->page_size + 7), GFP_KERNEL);
    if (!chip->oobbuf)
    {
        dev_err(&pdev->dev, "[%s]no memory for NAND tmp data buffer\n", __FUNCTION__);
        res = -ENOMEM;
        goto fail_err1;
    }

    chip->tmpbuf = (uint32_t*)(((uint32_t)(chip->oobbuf + 3)) & 0xfffffffc);/* 4bytes alige */

    res = gk_spinand_unlock_allblock(chip);
#ifdef CONFIG_MTD_SPINAND_INTERECC
    res = gk_spinand_enable_ecc(chip);
#else
    res = gk_spinand_disable_ecc(chip);
#endif

    /* Initialize the SPI nand MTD information */
    host->mtd.priv          = chip;
    host->mtd.dev.parent    = &pdev->dev;
    res = spinand_scan(&(host->mtd));
    if (res)
    {
        dev_err(&pdev->dev, "[%s_%d]NAND scan failed with %d\n", __FUNCTION__, __LINE__, res);
        res = -ENOMEM;
        goto fail_err2;
    }

    res = mtd_device_register(&(host->mtd), gk_spinand_partitions, NUM_PARTITIONS);
    if (res)
    {
        dev_err(&pdev->dev, "[%s_%d]MTD device register error!\n", __FUNCTION__, __LINE__);
        res = -ENOMEM;
        goto fail_err2;
    }

    return 0;

fail_err2:
    kfree(chip->oobbuf);
fail_err1:
    kfree(chip->buffers);
fail_err0:
    kfree(host);

    return res;

}

/**
 * __devexit spinand_remove--Remove the device driver
 * @spi: the spi device.
 *
 * Description:
 *   To remove the device driver parameters and free up allocated memories.
 */
static int __devexit gk_spinand_remove(struct platform_device *pdev)
{
    struct mtd_info *mtd;
    struct spinand_chip *chip;
    struct gk_spinand_host *host;

    pr_info("%s: remove\n", dev_name(&pdev->dev));

    host = dev_get_drvdata(&pdev->dev);

    mtd = &host->mtd;
    mtd_device_unregister(mtd);

    chip = &host->chip;

    kfree(chip->oobbuf);
    kfree(chip->buffers);
    kfree(host);

    return 0;

}

#ifdef CONFIG_PM
static int gk_spinand_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static int gk_spinand_resume(struct platform_device *pdev)
{
    return 0;
}
#else
#define gk_spinand_suspend  NULL
#define gk_spinand_resume   NULL
#endif

static struct gk_spinand_platform_data gk_spinand_data =
{
    .speed_mode = SYSTEM_SFLASH_FREQ,    // LindengYu: use the 138 div 2 = 69MHz
    .channel    = 1,
    .parts      = gk_spinand_partitions,
    .nr_parts    = NUM_PARTITIONS,
};

/**
 * Device name structure description
*/
static struct platform_device gk_spinand_device = {

    .name       = "gk_spi_nand",
    .id         = 1,
    .dev        =
    {
        .platform_data = &gk_spinand_data,
    },
};


/**
 * Device name structure description
*/
static struct platform_driver gk_spinand_driver = {
    .driver = {
        .name       = "gk_spi_nand",
        .bus        = &platform_bus_type,
        .owner      = THIS_MODULE,
    },

    .probe      = gk_spinand_probe,
    .remove     = __devexit_p(gk_spinand_remove),
    .suspend    = gk_spinand_suspend,
    .resume     = gk_spinand_resume,
};

/**
 * Device driver registration
*/
static int __init gk_spinand_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&gk_spinand_driver);

    if(!ret)
    {
        ret = platform_device_register(&gk_spinand_device);
        if(ret)
        {
            platform_driver_unregister(&gk_spinand_driver);
        }
    }
    return ret;
}

/**
 * unregister Device driver.
*/
static void __exit gk_spinand_exit(void)
{
    platform_device_unregister(&gk_spinand_device);
    platform_driver_unregister(&gk_spinand_driver);
}

module_init(gk_spinand_init);
module_exit(gk_spinand_exit);

MODULE_DESCRIPTION("GOKE SPI NAND driver");
MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_LICENSE("GPL");


