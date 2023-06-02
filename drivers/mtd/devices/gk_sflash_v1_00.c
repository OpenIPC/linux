/*
 *  drivers/mtd/devices/gk_sflash_v1_00.c
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/of.h>
#include <linux/delay.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/spi/spi.h>
#include <asm/mach/flash.h>

/*get register address*/
#include <mach/hardware.h>
/*include write and read function*/
#include <asm/io.h>
#include <mach/flash.h>

#include "gk_sflash_v1_00.h"

/****************************************************************************/
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
#define GD_SFLASH_USE_BUFFER    0
/*
*******************************************************************************
**
** serial flash specific commands for Spansion flash devices
**
*******************************************************************************
*/
static const goke_sflash_cmd_s sflash_commands_mx =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0xbb,
    /*  read_io4        */ 0xeb,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x38,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};
static const goke_sflash_cmd_s sflash_commands_spansion =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0xbb,
    /*  read_io4        */ 0xeb,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x32,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0x35,
};

/*-------------------------------------------------------------------------------*/
/* serial flash specific commands for STmicro flash devices                      */
/*-------------------------------------------------------------------------------*/
static const goke_sflash_cmd_s sflash_commands_stmicro =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0x00,
    /*  read_io4        */ 0x00,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x00,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};

/*-------------------------------------------------------------------------------*/
/* serial flash specific commands for Atmel flash devices                        */
/*-------------------------------------------------------------------------------*/
static const goke_sflash_cmd_s sflash_commands_atmel =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0x00,
    /*  read_io4        */ 0x00,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x00,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};

/*-------------------------------------------------------------------------------*/
/* serial flash specific commands for NUMONYX flash devices                      */
/*-------------------------------------------------------------------------------*/
static const goke_sflash_cmd_s sflash_commands_numonyx =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0xbb,
    /*  read_io4        */ 0xeb,
    /*  program_page2   */ 0xd2,
    /*  program_page4   */ 0x12,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};

static const goke_sflash_cmd_s sflash_commands_wb =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0xbb,
    /*  read_io4        */ 0xeb,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x32,
    /* read_ext_addr    */ 0xC8,
    /* write_ext_addr   */ 0xC5,
    /* read_status2     */ 0x35,
};

static const goke_sflash_cmd_s  sflash_command_sst =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0x20,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x80,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0x00,
    /*  read_io4        */ 0x0B,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x32,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};
static const goke_sflash_cmd_s sflash_commands_gd =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0xbb,
    /*  read_io4        */ 0xeb,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x32,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0x35,
};
static const goke_sflash_cmd_s sflash_commands_eon =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0x00,
    /*  read_io4        */ 0x0b,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x02,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};

static const goke_sflash_cmd_s sflash_commands_issi =
{
    /* read_ID          */ 0x9F,
    /* write_enable     */ 0x06,
    /* write_disable    */ 0x04,
    /* read_status      */ 0x05,
    /* write_status     */ 0x01,
    /* read_data        */ 0x03,
    /* read_data_fast   */ 0x0B,
    /* erase_sector     */ 0xD8,
    /* erase_chip       */ 0xC7,
    /* program_page     */ 0x02,
    /* status_mask_wip  */ 0x01,
    /* status_mask_wel  */ 0x02,
    /*  read_io2        */ 0xbb,
    /*  read_io4        */ 0xeb,
    /*  program_page2   */ 0x00,
    /*  program_page4   */ 0x38,
    /* read_ext_addr    */ 0xFF,
    /* write_ext_addr   */ 0xFF,
    /* read_status2     */ 0xFF,
};
/*-------------------------------------------------------------------------------*/
/* serial flash geometry and information data of all supported devices           */
/*-------------------------------------------------------------------------------*/
static goke_sflash_dev_s sflash_devices_supported[] =
{

   /*
    ** Spansion seem to use unique sectors
    ** with 64kBytes each, they increment the
    ** number of sectors for bigger chips
    */
    {
        /* manufacture_ID   */ 0x01,
        /* device_ID        */ 0x0212,
        /* manufacture_name */ "Spansion",
        /* device_name      */ "S25FL004A",
        /* device_bytes     */ 524288,
        /* sector_count     */ 8,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_spansion,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x9CF5,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x01,
        /* device_ID        */ 0x0213,
        /* manufacture_name */ "Spansion",
        /* device_name      */ "S25FL008A",
        /* device_bytes     */ 1048576,
        /* sector_count     */ 16,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_spansion,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x9CF5,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x01,
        /* device_ID        */ 0x0214,
        /* manufacture_name */ "Spansion",
        /* device_name      */ "S25FL016A",
        /* device_bytes     */ 2097152,
        /* sector_count     */ 32,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_spansion,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x9CF5,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x01,
        /* device_ID        */ 0x0215,
        /* manufacture_name */ "Spansion",
        /* device_name      */ "S25FL032P",
        /* device_bytes     */ 4194304,
        /* sector_count     */ 64,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_spansion,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x9CF5,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x01,
        /* device_ID        */ 0x0216,
        /* manufacture_name */ "Spansion",
        /* device_name      */ "S25FL064A",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_spansion,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x9CF5,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x01,
        /* device_ID        */ 0x2018,
        /* manufacture_name */ "Spansion",
        /* device_name      */ "FL128PIFL",
        /* device_bytes     */ 16777216,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_spansion,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x9CF5,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },

    /*
    ** MX seem to use unique sectors
    ** with 64kBytes each, they increment the
    ** number of sectors for bigger chips
    */
    /* gk6105 8M SFlash */
    {
        /* manufacture_ID   */ 0xc2,
        /* device_ID        */ 0x2617,
        /* manufacture_name */ "MX",
        /* device_name      */ "MX25L6455E",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_mx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufactureID    */ 0xc2,
        /* deviceID         */ 0x2017,
        /* manufactureName  */ "MX",
        /* deviceName       */ "MX25L6465E",
        /* deviceBytes      */ 8388608,
        /* sectorCount      */ 128,
        /* sectorBytes      */ 65536,
        /* sectorPages      */ 256,
        /* pageBytes        */ 256,
        /* commands         */ &sflash_commands_mx,
        /* ioMode           */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xc2,
        /* device_ID        */ 0x2018,
        /* manufacture_name */ "MX",
        /* device_name      */ "MX25L12845",
        /* device_bytes     */ 16777216,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_mx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xc2,
        /* device_ID        */ 0x5e16,
        /* manufacture_name */ "MX",
        /* device_name      */ "MX253235D",
        /* device_bytes     */ 4194304,
        /* sector_count     */ 64,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_mx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xc2,
        /* device_ID        */ 0x2415,
        /* manufacture_name */ "MX",
        /* device_name      */ "MX25L1605D",
        /* device_bytes     */ 2097152,
        /* sector_count     */ 32,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_mx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xc2,
        /* device_ID        */ 0x2415,
        /* manufacture_name */ "MX",
        /* device_name      */ "MX25L1635D",
        /* device_bytes     */ 2097152,
        /* sector_count     */ 32,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_mx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xc2,
        /* device_ID        */ 0x9e16,
        /* manufacture_name */ "MX",
        /* device_name      */ "MX25L3255D",
        /* device_bytes     */ 4194304,
        /* sector_count     */ 64,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_mx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x00BC,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xef,
        /* device_ID        */ 0x4015,
        /* manufacture_name */ "Winbond",
        /* device_name      */ "W25Q16BV",
        /* device_bytes     */ 2097152,
        /* sector_count     */ 32,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_wb,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x1C00,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xef,
        /* device_ID        */ 0x4017,
        /* manufacture_name */ "Winbond",
        /* device_name      */ "W25Q64FV",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_wb,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x1C00,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufactureID    */ 0xef,
        /* deviceID         */ 0x4018,
        /* manufactureName  */ "Winbond",
        /* deviceName       */ "W25Q128FV",
        /* deviceBytes      */ 16777216,
        /* sectorCount      */ 256,
        /* sectorBytes      */ 65536,
        /* sectorPages      */ 256,
        /* pageBytes        */ 256,
        /* commands         */ &sflash_commands_wb,
        /* ioMode           */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x1C00,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufactureID    */ 0xef,
        /* deviceID         */ 0x4019,
        /* manufactureName  */ "Winbond",
        /* deviceName       */ "W25Q256FV",
        /* deviceBytes      */ (1024*1024*32),
        /* sectorCount      */ 512,
        /* sectorBytes      */ 65536,
        /* sectorPages      */ 256,
        /* pageBytes        */ 256,
        /* commands         */ &sflash_commands_wb,
        /* ioMode           */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x3C00,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xbf,
        /* device_ID        */ 0x2601,
        /* manufacture_name */ "SST",
        /* device_name      */ "SST26VF016",
        /* device_bytes     */ 2097152,
        /* sector_count     */ 512,
        /* sector_bytes     */ 4096,
        /* sector_pages     */ 16,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_command_sst,
        /* iomode           */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFFFF,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x20,
        /* device_ID        */ 0xba18,
        /* manufacture_name */ "numonyx",
        /* device_name      */ "N25Q128",
        /* device_bytes     */ 8388608*2,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_numonyx,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFFFF,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xC8,
        /* device_ID        */ 0x4017,
        /* manufacture_name */ "giga",
        /* device_name      */ "GD25Q64C",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_gd,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFC79,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xC8,
        /* device_ID        */ 0x4018,
        /* manufacture_name */ "giga",
        /* device_name      */ "GD25Q128C",
        /* device_bytes     */ 8388608*2,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_gd,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFC79,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x1c,
        /* device_ID        */ 0x3017,
        /* manufacture_name */ "EON",
        /* device_name      */ "EN25Q64",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_eon,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x003C,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x1c,
        /* device_ID        */ 0x7017,
        /* manufacture_name */ "EON",
        /* device_name      */ "ENQH64A",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_eon,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x003C,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x1c,
        /* device_ID        */ 0x7018,
        /* manufacture_name */ "EON",
        /* device_name      */ "EN25Q128",
        /* device_bytes     */ 8388608*2,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_eon,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x003C,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x9D,
        /* device_ID        */ 0x6017,
        /* manufacture_name */ "ISSI",
        /* device_name      */ "IC25LP064",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_issi,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFFFF,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x9D,
        /* device_ID        */ 0x6018,
        /* manufacture_name */ "ISSI",
        /* device_name      */ "IC25LP128",
        /* device_bytes     */ 8388608*2,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_issi,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFFFF,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0x20,
        /* device_ID        */ 0x7017,
        /* manufacture_name */ "XMC",
        /* device_name      */ "XM25QH64A",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_eon,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0x003C,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xa1,
        /* device_ID        */ 0x4017,
        /* manufacture_name */ "FM",
        /* device_name      */ "FM25Q64-SOB-T-G",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_gd,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFC79,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xa1,
        /* device_ID        */ 0x4018,
        /* manufacture_name */ "FM",
        /* device_name      */ "FM25Q128-SOB-T-G",
        /* device_bytes     */ 8388608*2,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_gd,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFC79,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xa1,
        /* device_ID        */ 0x4017,
        /* manufacture_name */ "XTX",
        /* device_name      */ "xt25f64b",
        /* device_bytes     */ 8388608,
        /* sector_count     */ 128,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_gd,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFC79,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
    {
        /* manufacture_ID   */ 0xa1,
        /* device_ID        */ 0x4018,
        /* manufacture_name */ "XTX",
        /* device_name      */ "xt25f128b",
        /* device_bytes     */ 8388608*2,
        /* sector_count     */ 256,
        /* sector_bytes     */ 65536,
        /* sector_pages     */ 256,
        /* page_bytes       */ 256,
        /* commands         */ &sflash_commands_gd,
        /* feature          */ GD_SFLASH_1X_WRITE | GD_SFLASH_1X_READ,
        /* lock_mask        */ 0xFC79,
        /* io4_mask         */ 0xFFFF,
        /* channel          */ 0x0,
    },
};
static goke_sflash_feature_e old_feature = GOKE_SFLASH_FEATURE_IO1;
static u32 sflash_write_enable(goke_sflash_dev_s *device);

struct gk_sflash
{
    struct mutex            lock;
    struct mtd_info         mtd;
    goke_sflash_dev_s       *device;
};
static u32 sflash_cmd_bit_set(goke_sflash_command_s *sflashCmd, u32 *cmdBitSet);

static inline struct gk_sflash* mtd_to_gk_flash(struct mtd_info *mtd)
{
    return container_of(mtd, struct gk_sflash, mtd);
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_CE (read/write)                                            */
/*----------------------------------------------------------------------------*/

__inline static void sflash_set_ce(u32 data)
{
    gk_sf_writel(REG_SFLASH_CE, data);
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_Command (read/write)                                       */
/*----------------------------------------------------------------------------*/

__inline static void sflash_set_speed(u32 data)
{
    gk_sf_writel(REG_SFLASH_SPEED, data);
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_Command (read/write)                                       */
/*----------------------------------------------------------------------------*/

__inline static void sflash_set_command(u32 data)
{
    gk_sf_writel(REG_SFLASH_COMMAND, data);
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_Data (read/write)                                          */
/*----------------------------------------------------------------------------*/

__inline static void sflash_set_data(u32 data)
{
    gk_flash_write(data);
}

__inline static u32  sflash_get_data(void)
{
    return gk_flash_read();
}

static u32 sflash_wip_done_wait(goke_sflash_dev_s *device)
{
    u8 wip_mask;
    goke_sflash_command_s   sflash_cmd;
    u32 cmd_bit_set                                     = 0;

    cmd_bit_set |= (device->commands->read_status|
        SFLASH_SEND_CMD|                        /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|            /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|           /*dummy cycle*/
        SFLASH_RWN_READ|                        /*read data*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|          /*set the sflash adr mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)|          /*transfer data number*/
        SFLASH_HOLD_TIME_100ns
        );

    /* set the sflash cmd mode, set the sflash data mode */
    if(device->manufacture_ID == 0xbf)
    {
        cmd_bit_set |= (SFLASH_CMD_MODE_4X|SFLASH_DATA_CYCLE_NUM_2);
    }
    else
    {
        cmd_bit_set |= (SFLASH_CMD_MODE_1X|SFLASH_DATA_CYCLE_NUM_0);
    }

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);

    wip_mask = device->commands->status_mask_wip;
    while(1)
    {
        sflash_set_command(sflash_cmd.all);

        if((sflash_get_data() & wip_mask) != wip_mask) /*flash device ready*/
        {
            return(0);
        }
    }
    return 1;
}

static u32 sflash_wel_done_wait(goke_sflash_dev_s *device)
{
    u8 wel_mask;
    goke_sflash_command_s   sflash_cmd;
    u32 cmd_bit_set                                     = 0;

    cmd_bit_set |= (device->commands->read_status|
        SFLASH_SEND_CMD|                        /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|            /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|           /*dummy cycle*/
        SFLASH_RWN_READ|                        /*read data*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|          /*set the sflash adr mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)|          /*transfer data number*/
        SFLASH_HOLD_TIME_100ns
        );

    /* set the sflash cmd mode, set the sflash data mode */
    if(device->manufacture_ID == 0xbf)
    {
        cmd_bit_set |= (SFLASH_CMD_MODE_4X|SFLASH_DATA_CYCLE_NUM_2);
    }
    else
    {
        cmd_bit_set |= (SFLASH_CMD_MODE_1X|SFLASH_DATA_CYCLE_NUM_0);
    }

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);

    wel_mask = device->commands->status_mask_wel;
    while(1)
    {
        sflash_set_command(sflash_cmd.all);

        if((sflash_get_data() & wel_mask) == wel_mask) /*flash enable ready*/
        {
            return(0);
        }
    }
    return 1;
}

/*!
********************************************************************************
**
** \brief unlock the serial flash
**
** This function unlock the serial flash.
**
** \return
** - GD_ERR_INVALID_HANDLE if the given handle parameter points to 0
** - GD_OK if unlock operation successfully
**
********************************************************************************
*/
static u32 sflash_Unlock(goke_sflash_dev_s *device)
{
    goke_sflash_command_s   sflash_cmd;
    u32 cmd_bit_set                                     = 0;
    u8 status;
    u16 sflashData;
    ulong CESetting = 0x38;

    CESetting = CESetting | ((ulong)device->channel << 6);
    sflash_set_ce(CESetting);

    /* write enable the device */
    sflash_write_enable(device);

    cmd_bit_set = (device->commands->read_status |   // command
                   SFLASH_SEND_CMD               |   // transfer the command
                   SFLASH_SEND_ADDR_BYTE_NUM_0   |   // address num
                   SFLASH_SEND_DUMMY_BYTE_NUM_0  |   // dummy cycle
                   SFLASH_RWN_READ               |   // read data
                   SFLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                   SFLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                   SFLASH_DATA_CYCLE_NUM_0       |   // set the sflash data mode
                   (1<<SFLASH_TRANSFER_BYTE_LOC)     // transfer data number
                  );
    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);

    sflash_set_command(sflash_cmd.all);
    status = sflash_get_data()& 0xff;
    sflashData = status;

    if(device->commands->read_status2 != 0xFF)
    {
        cmd_bit_set = (device->commands->read_status2|   // command
                       SFLASH_SEND_CMD               |   // transfer the command
                       SFLASH_SEND_ADDR_BYTE_NUM_0   |   // address num
                       SFLASH_SEND_DUMMY_BYTE_NUM_0  |   // dummy cycle
                       SFLASH_RWN_READ               |   // read data
                       SFLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                       SFLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                       SFLASH_DATA_CYCLE_NUM_0       |   // set the sflash data mode
                       (1<<SFLASH_TRANSFER_BYTE_LOC)     // transfer data number
                      );
        sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
        sflash_set_command(sflash_cmd.all);
        status = sflash_get_data()& 0xff;
        sflashData = (sflashData << 8) + status;
    }

    sflash_write_enable(device);
    cmd_bit_set = (device->commands->write_status|   // command
                   SFLASH_SEND_CMD               |   // transfer the command
                   SFLASH_SEND_ADDR_BYTE_NUM_0   |   // address num
                   SFLASH_SEND_DUMMY_BYTE_NUM_0  |   // dummy cycle
                   SFLASH_RWN_WRITE              |   // write data
                   SFLASH_CMD_MODE_1X            |   // set the sflash cmd mode
                   SFLASH_ADDR_DUMMY_CYCLE_NUM_0 |   // set the sflash adr mode
                   SFLASH_DATA_CYCLE_NUM_0       |   // set the sflash data mode
                   (1<<SFLASH_TRANSFER_BYTE_LOC)     // transfer data number
                  );

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    if(device->commands->read_status2 != 0xFF)
    {
        sflash_cmd.bitc.data_num = 0x2;               // transfer data
    }
    sflashData &= ~(device->lock_mask);
    if(device->manufacture_ID == 0x01)
    {
        sflashData |= 0x0024;
    }

    sflash_set_command(sflash_cmd.all);
    sflash_set_data(sflashData);

    sflash_wip_done_wait(device);
    CESetting = CESetting & (~((ulong)device->channel << 6));
    sflash_set_ce(CESetting);
    return(0);
}

static  goke_sflash_dev_s*  jedec_probe(struct platform_device *dev)
{
    u32 sflash_data;
    u8  manufacture_ID;
    u16 device_ID;
    u32 index;
    u32 count;
    u32 cmd_bit_set                                = 0;
    u8  command,status;
    u32 prot1;
    u32 prot2;
    goke_sflash_dev_s *device;
    goke_sflash_command_s   sflash_cmd;
    u32 ce_setting                          = 0x0E;
    GD_SFLASH_SPEED_MODE    speed_mode      = GOKE_SFLASH_FREQ_DIV2;
    goke_sflash_channel_e   channel         = GOKE_SFLASH_CHANNEL_0;
#ifdef CONFIG_OF
    if (of_property_read_u32(dev->dev.of_node, "gofortune-semi,channel", &channel) ||
            of_property_read_u32(dev->dev.of_node, "gofortune-semi,speed", &speed_mode)) {
        return NULL;
    }
#else    /* CONFIG_OF */
    struct sflash_platform_data *platform_data = dev->dev.platform_data;

    channel = platform_data->channel;
    speed_mode = platform_data->speed_mode;
#endif    /* !CONFIG_OF */
    printk(KERN_ERR "speed_mod is %d\n", speed_mode);
    if(channel >= GOKE_SFLASH_CHANNEL_NUM)
    {
        return NULL;
    }

    ce_setting = ce_setting | ((u32)channel  << 6);

    sflash_set_ce(ce_setting);
    sflash_set_speed(speed_mode); /*0 -- 1/2freq 1--1/4freq  2....4*/

    count = sizeof(sflash_devices_supported)/sizeof(goke_sflash_dev_s);

    for(index=0; index < count; index++)
    {
        device = &(sflash_devices_supported[index]);

        command = device->commands->read_ID;

        sflash_set_ce(ce_setting);
        cmd_bit_set |= (SFLASH_SEND_CMD|                    /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
            SFLASH_RWN_READ|                                /*read data*/
            SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_0|                        /*set the sflash data mode*/
            (3<<SFLASH_TRANSFER_BYTE_LOC)                   /*transfer data number*/
            );

        cmd_bit_set |= command;
        sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
        sflash_set_command(sflash_cmd.all);
        sflash_data = sflash_get_data();

        manufacture_ID = (u8)((sflash_data & 0x00ff0000) >> 16);
        device_ID = (u16)(sflash_data & 0x0000FFFF);
        if((manufacture_ID == device->manufacture_ID)
        && (device_ID == device->device_ID))
        {
            if(manufacture_ID == 0xbf)                      /*SST sflash which support most quad IO cmd*/
            {
                cmd_bit_set |= (0x38|                       /*enable quad command*/
                    SFLASH_SEND_CMD|                        /*transfer the command*/
                    SFLASH_SEND_ADDR_BYTE_NUM_0|            /*address num*/
                    SFLASH_SEND_DUMMY_BYTE_NUM_0|           /* dummy cycle*/
                    SFLASH_RWN_NOTHING|                     /*no read/write*/
                    SFLASH_CMD_MODE_1X|                     /*set command mode*/
                    SFLASH_ADDR_DUMMY_CYCLE_NUM_0|          /*set the sflash adr mode*/
                    SFLASH_DATA_CYCLE_NUM_0|                /*set the sflash data mode*/
                    (0<<SFLASH_TRANSFER_BYTE_LOC)           /*transfer data number*/
                    );

                sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
                sflash_set_command( sflash_cmd.all );
                sflash_get_data();

                cmd_bit_set |= (0xaf|                       /*enable quad command*/
                     SFLASH_SEND_CMD|                       /*transfer the command*/
                     SFLASH_SEND_ADDR_BYTE_NUM_0|           /*address num*/
                     SFLASH_SEND_DUMMY_BYTE_NUM_0|          /*dummy cycle*/
                     SFLASH_RWN_READ|                       /*read*/
                     SFLASH_CMD_MODE_4X|                    /*set command mode*/
                     SFLASH_ADDR_DUMMY_CYCLE_NUM_0|         /*set the sflash adr mode*/
                     SFLASH_DATA_CYCLE_NUM_2|               /*set the sflash data mode*/
                     (3<<SFLASH_TRANSFER_BYTE_LOC)          /*transfer data number*/
                     );

                sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
                sflash_set_command(sflash_cmd.all);

                sflash_data = sflash_get_data();

                manufacture_ID = (u8)((sflash_data & 0x00ff0000) >> 16);
                device_ID = (u16)(sflash_data & 0x0000FFFF);

                if(manufacture_ID != 0xbf || device_ID!= 0x2601)
                {
                    return(NULL);
                }

                   /*ok read the protection bit for each block*/
                sflash_cmd.bitc.code = 0x72;                /*read block protection register*/
                sflash_cmd.bitc.data_num = 0x6;             /*transfer data number*/

                sflash_set_command(sflash_cmd.all);
                prot1 = sflash_get_data();
                prot2 = sflash_get_data();

                /*ok write enable the device*/
                sflash_cmd.bitc.code = 0x06;                    /*enable quad command*/
                sflash_cmd.bitc.rwn =  SFLASH_RWN_NOTHING>>15;  /*3; no read/write*/
                sflash_cmd.bitc.data_num = 0;
                sflash_set_command(sflash_cmd.all);
                sflash_get_data();

                /*program the prot bit*/
                sflash_cmd.bitc.code = 0x42;                    /*enable quad command*/
                sflash_cmd.bitc.rwn = SFLASH_RWN_WRITE>>15;     /*write*/
                sflash_cmd.bitc.data_num = 6;
                prot1 = 0x0;
                prot2=0x0;
                sflash_set_command(sflash_cmd.all);
                sflash_set_data(prot1);
                sflash_set_data(prot2);

                /*wait the program*/
                sflash_cmd.bitc.code = 0x05;                    /*read status*/
                sflash_cmd.bitc.rwn = SFLASH_RWN_READ>>15;      /*read*/
                sflash_cmd.bitc.data_num = 1;
                do
                {
                    sflash_set_command(sflash_cmd.all);
                    status = sflash_get_data()&0xff;
                }while((status&0x80) == 0x80);

                /*ok read the protection bit for each block*/
                sflash_cmd.bitc.code = 0x72;                    /*enable quad command*/
                sflash_cmd.bitc.data_num = 0x6;                 /*transfer data number*/
                sflash_set_command(sflash_cmd.all);
                prot1 = sflash_get_data();
                prot2 = sflash_get_data();

#ifdef CONFIG_SFLASH_1X_R_1X_W_MODE
                device->feature = GD_SFLASH_1X_READ | GD_SFLASH_1X_WRITE;
                printk("USE 1X mode read and 1X mode write\n");
#else
#ifdef CONFIG_SFLASH_4X_R_1X_W_MODE
                device->feature = GD_SFLASH_4X_READ | GD_SFLASH_1X_WRITE;
                printk("USE 4X mode read and 1X mode write\n");
#endif
#ifdef CONFIG_SFLASH_4X_R_4X_W_MODE
                device->feature = GD_SFLASH_4X_READ | GD_SFLASH_4X_WRITE;
                printk("USE 4X mode read and 4X mode write\n");
#endif
#endif
            }
            else if(device->manufacture_ID == 0x0c2)
            {
                cmd_bit_set |= (0x2b|                           /*command*/
                     SFLASH_SEND_CMD|                           /*transfer the command*/
                     SFLASH_SEND_ADDR_BYTE_NUM_0|               /*address num*/
                     SFLASH_SEND_DUMMY_BYTE_NUM_0|              /* dummy cycle*/
                     SFLASH_RWN_READ|                           /*read data*/
                     SFLASH_CMD_MODE_1X|                        /*set the sflash cmd mode*/
                     SFLASH_ADDR_DUMMY_CYCLE_NUM_0|             /*set the sflash adr mode*/
                     SFLASH_DATA_CYCLE_NUM_0|                   /*set the sflash data mode*/
                     (1<<SFLASH_TRANSFER_BYTE_LOC)              /*transfer data number*/
                     );
                sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
                sflash_set_command(sflash_cmd.all);
                status=sflash_get_data()&0xff;
                if((status&0x80)==0x80)
                {
                    sflash_cmd.bitc.code = device->commands->write_enable;  /*command*/
                    sflash_cmd.bitc.rwn = SFLASH_RWN_NOTHING>>15;           /* not read & write*/
                    sflash_cmd.bitc.data_num = 0x0;                         /*not transfer data*/
                    sflash_set_command( sflash_cmd.all );
                    sflash_get_data();
                    sflash_cmd.bitc.code = 0x05;                            /*read status*/
                    sflash_cmd.bitc.rwn = SFLASH_RWN_READ>>15;              /*read*/
                    sflash_cmd.bitc.data_num = 1;
                    do
                    {
                        sflash_set_command(sflash_cmd.all);
                        status = sflash_get_data()&0xff;
                    }while((status&0x01) == 0x01);

                    sflash_cmd.bitc.code = 0x98;                            /*command*/
                    sflash_cmd.bitc.rwn = SFLASH_RWN_NOTHING>>15;           /* not read & write*/
                    sflash_cmd.bitc.data_num = 0x0;                         /*not transfer data*/
                    sflash_set_command( sflash_cmd.all );
                    sflash_get_data();
                    sflash_cmd.bitc.code = 0x05;                            /*read status*/
                    sflash_cmd.bitc.rwn = SFLASH_RWN_READ>>15;              /*read*/
                    sflash_cmd.bitc.data_num = 1;
                    do
                    {
                        sflash_set_command( sflash_cmd.all );
                        status = sflash_get_data()&0xff;
                    }while((status&0x01) == 0x01);
                }
                // check if the chip support 4X read mode?
                cmd_bit_set |= (0x5A|                           /*command RDSFDP*/
                    SFLASH_SEND_CMD|                            /*transfer the command*/
                    SFLASH_SEND_ADDR_BYTE_NUM_3|                /*address num*/
                    SFLASH_SEND_DUMMY_BYTE_NUM_1|               /*dummy cycle*/
                    SFLASH_RWN_READ|                            /*read data*/
                    SFLASH_CMD_MODE_1X|                         /*set the sflash cmd mode*/
                    SFLASH_ADDR_DUMMY_CYCLE_NUM_0|              /*set the sflash adr mode*/
                    SFLASH_DATA_CYCLE_NUM_0|                    /*set the sflash data mode*/
                    SFLASH_TRANSFER_BYTE_NUM_4|                 /*transfer data number*/
                    SFLASH_HOLD_TIME_100ns
                    );

                sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
                sflash_set_command(sflash_cmd.all);
                sflash_set_data(0x30);
                sflash_data = sflash_get_data();

#ifdef CONFIG_SFLASH_1X_R_1X_W_MODE
                device->feature = GD_SFLASH_1X_READ | GD_SFLASH_1X_WRITE;
                printk("USE 1X mode read and 1X mode write\n");
#else
#ifdef CONFIG_SFLASH_4X_R_1X_W_MODE
                device->feature = GD_SFLASH_4X_READ | GD_SFLASH_1X_WRITE;
                printk("USE 4X mode read and 1X mode write\n");
#endif
#ifdef CONFIG_SFLASH_4X_R_4X_W_MODE
                device->feature = GD_SFLASH_4X_READ | GD_SFLASH_4X_WRITE;
                printk("USE 4X mode read and 4X mode write\n");
#endif
#endif
                // command-address-data
                // 0x32[bit12]=1:1-2-2
                // 0x32[bit13]=1:1-2-4
                // 0x32[bit14]=1:1-4-4
                // sflash_data=0x30 31 32 33
                if((sflash_data != 0x00000000) || (sflash_data != 0xFFFFFFFF))
                {
                    device->feature &= 0xFFFFFFF8;
                    if(sflash_data & 0x00006000)
                    {
                        //device->feature |= GD_SFLASH_4X_READ;
                        printk("support 4X mode read:0x%08x\n", sflash_data);
                    }
                    else
                    {
                        device->feature |= GD_SFLASH_1X_READ;
                        printk("not support 4X mode read:0x%08x\n", sflash_data);
                    }
                }
            }
            else
            {
#ifdef CONFIG_SFLASH_1X_R_1X_W_MODE
                device->feature = GD_SFLASH_1X_READ | GD_SFLASH_1X_WRITE;
                printk("USE 1X mode read and 1X mode write\n");
#else
#ifdef CONFIG_SFLASH_4X_R_1X_W_MODE
                device->feature = GD_SFLASH_4X_READ | GD_SFLASH_1X_WRITE;
                printk("USE 4X mode read and 1X mode write\n");
#endif
#ifdef CONFIG_SFLASH_4X_R_4X_W_MODE
                device->feature = GD_SFLASH_4X_READ | GD_SFLASH_4X_WRITE;
                printk("USE 4X mode read and 4X mode write\n");
#endif
#endif
            }
            device->channel = channel;
            sflash_Unlock(device);
#if GD_SFLASH_USE_BUFFER
            device->buffer_size = device->sector_bytes;
            device->buffer = kmalloc(device->buffer_size, GFP_KERNEL);
            device->offset = 0xFFFFFFFF;
            if(device->buffer)
            {
                return device;
            }
            else
            {
                return NULL;
            }
#else
            return device;
#endif
        }
    }
    return NULL;
}

static u32 sflash_enable_io4(goke_sflash_dev_s* device )
{
    u8 status1                                  = 0;
    u8 status2                                  = 0;
    u32 data                                    = 0;
    u32  cmd_bit_set                            = 0;
    goke_sflash_command_s   sflash_cmd;
    u32 ce_setting                              = 0x38;

    if( !device )
        return(EINVAL);

    /*for IO4 mode only*/
    ce_setting = ce_setting | ((u32)device->channel << 6);
    sflash_set_ce(ce_setting);

    /*after Init, the SST enter IO4 mode*/
    if(device->manufacture_ID == 0xbf)
    {
        return 0;
    }
    if(device->manufacture_ID == 0x1c)
    {
        cmd_bit_set |= (0x38|                   /*enable quad command*/
            SFLASH_SEND_CMD|                    /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_0|        /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_0|       /*dummy cycle*/
            SFLASH_RWN_NOTHING|                 /*no read/write*/
            SFLASH_CMD_MODE_1X|                 /*set command mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_0|      /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_0|            /*set the sflash data mode*/
            (0<<SFLASH_TRANSFER_BYTE_LOC)       /*transfer data number*/
            );
        sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
        sflash_set_command( sflash_cmd.all );
        sflash_get_data();
        return 0;
    }
    /* first read the status */
    cmd_bit_set |= (device->commands->read_status|  /*command*/
        SFLASH_SEND_CMD|                            /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|               /* dummy cycle*/
        SFLASH_RWN_READ|                            /*read data*/
        SFLASH_CMD_MODE_1X|                         /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|              /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                    /*set the sflash data mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)            /*transfer data number*/
        );
    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    status1 = sflash_get_data()&0xff;
    if((device->manufacture_ID == 0xef)             /*winbond read status2*/
        ||(device->manufacture_ID == 0x01)          /*for FL064P;Tom.wang;2010-10-21*/
        ||(device->manufacture_ID == 0xc8))         /*for GD liujinyng2011-10-12*/
    {
        sflash_cmd.bitc.code = 0x35;                /*command*/
        sflash_set_command( sflash_cmd.all );
        status2 = sflash_get_data()&0xff;
    }

    /* write enable the device */
    sflash_cmd.bitc.code = device->commands->write_enable;  /*command*/
    sflash_cmd.bitc.rwn = SFLASH_RWN_NOTHING>>15;           /* not read & write*/
    sflash_cmd.bitc.data_num = 0x0;                         /*not transfer data*/
    sflash_set_command(sflash_cmd.all);
    sflash_get_data();


    if(device->manufacture_ID == 0xc2)
    {
        data = status1|0x40;
    }
    else if(device->manufacture_ID == 0xef)
    {
        data = (status1<<8)+(status2|0x2);
    }
    else if(device->manufacture_ID == 0x01)             /*for FL064P;Tom.wang;2010-10-21*/
    {
        data = (status1<<8)+(status2|0x2);
    }
    else if(device->manufacture_ID == 0xc8)
    {
        data = status2|0x2;
    }

     /* write the status register */
    sflash_cmd.bitc.code = device->commands->write_status;  /*command*/
    sflash_cmd.bitc.rwn = SFLASH_RWN_WRITE>>15;             /*write*/
    if(device->manufacture_ID == 0xc2)
    {
        sflash_cmd.bitc.data_num = 0x1;                     /*transfer 1 byte data*/
    }
    else if(device->manufacture_ID == 0xef)
    {
        sflash_cmd.bitc.data_num = 0x2;                     /*transfer 2 byte data*/
    }
    else if(device->manufacture_ID == 0x01)                 /*for FL064P;Tom.wang;2010-10-21*/
    {
        sflash_cmd.bitc.data_num = 0x2;                     /*transfer data*/
    }
    else if(device->manufacture_ID == 0xc8)
    {
        sflash_cmd.bitc.code = 0x31;
        sflash_cmd.bitc.data_num = 0x1;                     /*transfer data*/
    }
    sflash_set_command(sflash_cmd.all);
    sflash_set_data(data);
    sflash_wip_done_wait(device);

    return 0;
}

static u32 sflash_disenable_io4( goke_sflash_dev_s* device )
{
    u8 status1                                  = 0;
    u8 status2                                  = 0;
    u32 data                                    = 0;
    u32 cmd_bit_set                             = 0;
    goke_sflash_command_s   sflash_cmd;
    u32 ce_setting                              = 0x0E;

    if( !device )
        return(EINVAL);

    /*fix configuration for no pull-high W/P pin*/
    ce_setting = ce_setting | ((u32)device->channel << 6);
    sflash_set_ce(ce_setting);

    /*SST sflash support IO4 mode. don't exit this mode*/
    if(device->manufacture_ID == 0xbf)
    {
        return 0;
    }
    if(device->manufacture_ID == 0x1c)
    {
        cmd_bit_set |= (0xff|                       /*enable quad command*/
            SFLASH_SEND_CMD|                        /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_0|            /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_0|           /* dummy cycle*/
            SFLASH_RWN_NOTHING|                     /*no read/write*/
            SFLASH_CMD_MODE_4X|                     /*set command mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_0|          /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_0|                /*set the sflash data mode*/
            (0<<SFLASH_TRANSFER_BYTE_LOC)           /*transfer data number*/
            );
        sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
        sflash_set_command( sflash_cmd.all );
        sflash_get_data();
        return 0;
    }

    /*first read the status */
    cmd_bit_set |= (device->commands->read_status|      /*command*/
                 SFLASH_SEND_CMD|                       /*transfer the command*/
                 SFLASH_SEND_ADDR_BYTE_NUM_0|           /*address num*/
                 SFLASH_SEND_DUMMY_BYTE_NUM_0|          /* dummy cycle*/
                 SFLASH_RWN_READ|                       /*read data*/
                 SFLASH_CMD_MODE_1X|                    /*set the sflash cmd mode*/
                 SFLASH_ADDR_DUMMY_CYCLE_NUM_0|         /*set the sflash adr mode*/
                 SFLASH_DATA_CYCLE_NUM_0|               /*set the sflash data mode*/
                 (1<<SFLASH_TRANSFER_BYTE_LOC)          /*transfer data number*/
                 );
    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command( sflash_cmd.all );
    status1 = sflash_get_data()&0xff;

    if((device->manufacture_ID == 0xef)                 /*winbond read status2*/
        ||(device->manufacture_ID == 0x01)              /*for FL064P;Tom.wang;2010-10-21*/
        ||(device->manufacture_ID == 0xc8))             /*for GD liujinyng2011-10-12*/
    {
        sflash_cmd.bitc.code = 0x35;                    /*command*/
        sflash_set_command(sflash_cmd.all);
        status2 = sflash_get_data()&0xff;
    }

    /* write enable the device */
    sflash_cmd.bitc.code = device->commands->write_enable;      /*command*/
    sflash_cmd.bitc.rwn = SFLASH_RWN_NOTHING>>15;               /* not read & write*/
    sflash_cmd.bitc.data_num = 0x0;                             /*not transfer data*/
    sflash_set_command( sflash_cmd.all );
    sflash_get_data();

    if(device->manufacture_ID == 0xc2)
    {
        //Steven Yu: // clear quad
        //for GK workaround do not enable reset mode otherwise code can not bootup
        //data = status1&(~0x40);                                  /*clear quad*/
        //write back status1
        data = status1;
    }
    else if(device->manufacture_ID == 0xef)
    {
        data = (status1<<8)+ (status2 & (~0x2));                 /*clear the quad*/
    }
    else if(device->manufacture_ID == 0x01)                      /*for FL064P;Tom.wang;2010-10-21*/
    {
        data = (status1<<8)+ (status2 & (~0x2));                 /*clear the quad*/
    }
    else if(device->manufacture_ID == 0xc8)
    {
        data = status2 & (~0x2);
    }

    /* write the status register */
    sflash_cmd.bitc.code = device->commands->write_status;      /*command*/
    sflash_cmd.bitc.rwn = SFLASH_RWN_WRITE>>15;                 /*write*/
    if(device->manufacture_ID == 0xc2)
    {
        sflash_cmd.bitc.data_num = 0x1;                         /*transfer data*/
    }
    else if(device->manufacture_ID == 0xef)
    {
        sflash_cmd.bitc.data_num = 0x2;                         /*transfer data*/
    }
    else if(device->manufacture_ID == 0x01)                     /*for FL064P;Tom.wang;2010-10-21*/
    {
        sflash_cmd.bitc.data_num = 0x2;                         /*transfer data*/
    }
    else if(device->manufacture_ID == 0xc8)
    {
        sflash_cmd.bitc.code = 0x31;
        sflash_cmd.bitc.data_num = 0x1;                         /*transfer data*/
    }

    sflash_set_command(sflash_cmd.all);
    sflash_set_data(data);
    sflash_wip_done_wait(device);

    return 0;
}

static u32 sflash_write_enable(goke_sflash_dev_s *device)
{
    goke_sflash_command_s   sflash_cmd;
    u32 cmd_bit_set                                     = 0;

    cmd_bit_set = (device->commands->write_enable|     /*command*/
        SFLASH_SEND_CMD|                                /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
        SFLASH_RWN_NOTHING|                             /*no read/write data*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                        /*set the sflash data mode*/
        (0<<SFLASH_TRANSFER_BYTE_LOC)|                  /*transfer data number*/
        SFLASH_HOLD_TIME_100ns
        );
    if(device->manufacture_ID == 0xbf)
    {
        cmd_bit_set |= SFLASH_CMD_MODE_4X;              /*set the sflash cmd mode*/
    }
    else
    {
        cmd_bit_set |= SFLASH_CMD_MODE_1X;              /*set the sflash cmd mode*/
    }

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    sflash_get_data();

    return sflash_wel_done_wait(device);
}

static u32 sflash_set_ext_addr(goke_sflash_dev_s* device, u8 offset)
{
    u32 cmd_bit_set                                 = 0;
    goke_sflash_command_s   sflash_cmd;
    u8 extadd = 0;

    sflash_write_enable(device);

    cmd_bit_set = (device->commands->read_ext_addr|/*command*/
        SFLASH_SEND_CMD|                            /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|               /* dummy cycle*/
        SFLASH_RWN_READ|                            /*read data*/
        SFLASH_CMD_MODE_1X|                         /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|              /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                    /*set the sflash data mode*/
        (2<<SFLASH_TRANSFER_BYTE_LOC)               /*transfer data number*/
        );

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    extadd = sflash_get_data();
    if(offset == extadd)
    {
        return 0;
    }

    sflash_write_enable(device);
    cmd_bit_set = (device->commands->write_ext_addr|/*command*/
        SFLASH_SEND_CMD|                            /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|               /* dummy cycle*/
        SFLASH_RWN_WRITE|                           /*write data*/
        SFLASH_CMD_MODE_1X|                         /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|              /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                    /*set the sflash data mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)               /*transfer data number*/
        );

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    sflash_set_data(offset);
    return 0;
}

static int sflash_read_io1( goke_sflash_dev_s* device, u32 offset, u32* buffer, u32 data_len )
{
    u32 cmd_bit_set                                 = 0;
    goke_sflash_command_s   sflash_cmd;

    if( !device )
        return(EINVAL);

    cmd_bit_set |= (device->commands->read_data|    /*command*/
        SFLASH_SEND_CMD|                            /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_3|                /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|               /* dummy cycle*/
        SFLASH_RWN_READ|                            /*read data*/
        SFLASH_CMD_MODE_1X|                         /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|              /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                    /*set the sflash data mode*/
        SFLASH_TRANSFER_BYTE_NUM_4|                 /*transfer data number*/
        SFLASH_HOLD_TIME_100ns
        );

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
    }
    sflash_set_command(sflash_cmd.all);
    sflash_set_data(offset);

    while(data_len--)
    {
        *buffer++ = sflash_get_data();
    }
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }

    return 0;
}

static int sflash_read_io2( goke_sflash_dev_s* device, u32 offset, u32* buffer, u32 data_len )
{
    u32 cmd_bit_set                                 = 0;
    goke_sflash_command_s   sflash_cmd;

    if( !device )
        return(EINVAL);

    if(device->manufacture_ID == 0x20)
    {
        cmd_bit_set |= (device->commands->read_io2|             /*command*/
            SFLASH_SEND_CMD|                                    /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_3|                        /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_2|                       /* dummy cycle*/
            SFLASH_RWN_READ|                                    /*read data*/
            SFLASH_CMD_MODE_1X|                                 /*set the sflash cmd mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_1|                      /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_1|                            /*set the sflash data mode*/
            SFLASH_TRANSFER_BYTE_NUM_4                          /*transfer data number*/
            );
    }
    else
    {
        cmd_bit_set |= (device->commands->read_io2|             /*command*/
            SFLASH_SEND_CMD|                                    /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_3|                        /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_1|                       /* dummy cycle*/
            SFLASH_RWN_READ|                                    /*read data*/
            SFLASH_CMD_MODE_1X|                                 /*set the sflash cmd mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_1|                      /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_1|                            /*set the sflash data mode*/
            SFLASH_TRANSFER_BYTE_NUM_4                          /*transfer data number*/
            );
    }

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
    }
    sflash_set_command(sflash_cmd.all);
    sflash_set_data(offset);

    while(data_len--)
    {
        *buffer++ = sflash_get_data();
    }
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }

    return 0;
}

static int sflash_read_io4( goke_sflash_dev_s* device, u32 offset, u32* buffer, u32 data_len  )
{
    u32 cmd_bit_set                                 = 0;
    u32 err                                         = 0;
    goke_sflash_command_s   sflash_cmd;

    if( !device )
        return(EINVAL);
    if(device->manufacture_ID == 0x20)
    {
        err = 0;
    }
    else
    {
        err = sflash_enable_io4(device);                 /*enable the IO4 mode*/
    }
    if(err != 0)
    {
        return err;
    }

    if(device->manufacture_ID == 0xbf)
    {
       cmd_bit_set |= (device->commands->read_io4|          /*command*/
        SFLASH_SEND_CMD|                                    /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_3|                        /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_1|                       /*dummy cycle*/
        SFLASH_RWN_READ|                                    /*read data*/
        SFLASH_CMD_MODE_4X|                                 /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_2|                      /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_2|                            /*set the sflash data mode*/
        SFLASH_TRANSFER_BYTE_NUM_4                          /*transfer data number*/
         );
    }
    else if(device->manufacture_ID == 0x1c)
    {
        cmd_bit_set |= (device->commands->read_io4|         /*command*/
            SFLASH_SEND_CMD|                                /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_3|                    /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_3|                   /*dummy cycle*/
            SFLASH_RWN_READ|                                /*read data*/
            SFLASH_CMD_MODE_4X|                             /*set the sflash cmd mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_2|                  /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_2|                        /*set the sflash data mode*/
            SFLASH_TRANSFER_BYTE_NUM_4                      /*transfer data number*/
            );
    }
    else if(device->manufacture_ID == 0x20)
    {
        cmd_bit_set |= (device->commands->read_io4|         /*command*/
            SFLASH_SEND_CMD|                                /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_3|                    /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_5|                   /*dummy cycle*/
            SFLASH_RWN_READ|                                /*read data*/
            SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_2|                  /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_2|                        /*set the sflash data mode*/
            SFLASH_TRANSFER_BYTE_NUM_4                      /*transfer data number*/
            );
    }
    else
    {
        cmd_bit_set |= (device->commands->read_io4|         /*command*/
            SFLASH_SEND_CMD|                                /*transfer the command*/
            SFLASH_SEND_ADDR_BYTE_NUM_3|                    /*address num*/
            SFLASH_SEND_DUMMY_BYTE_NUM_3|                   /*dummy cycle*/
            SFLASH_RWN_READ|                                /*read data*/
            SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
            SFLASH_ADDR_DUMMY_CYCLE_NUM_2|                  /*set the sflash adr mode*/
            SFLASH_DATA_CYCLE_NUM_2|                        /*set the sflash data mode*/
            SFLASH_TRANSFER_BYTE_NUM_4                      /*transfer data number*/
            );
    }

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
    }
    sflash_set_command(sflash_cmd.all);
    sflash_set_data(offset);

    while(data_len--)
    {
        *buffer++ = sflash_get_data();
    }
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }

    //if(device->manufacture_ID == 0x20)
    //{
    //    err = sflash_wip_done_wait(device);
    //}
    //else
    //{
    //    err = sflash_disenable_io4(device);                  /*disable the IO4 mode*/
    //}

    return err;
}


static int sflash_write_io1( goke_sflash_dev_s* device, u32 offset, u32* buffer, u32 data_len )
{
    u32 count                                       = 0;
    u32 page_words;
    u32 write_words                                 = data_len;
    u32 cmd_bit_set                                 = 0;
    goke_sflash_command_s   sflash_cmd;
    u32 result                                      = 0;
    u32 extadd = 0;

    if( !device )
        return(EINVAL);

    page_words = device->page_bytes / sizeof(u32);

    while(write_words--)
    {
        if(count == 0)
        {
            // >16MB
            if((extadd == 0) && (offset >= GD_SFLASH_16M_SIZE))
            {
                extadd = 1;
                sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
            }

            /*issue a write command sequence to prepare the device for data to be written*/
            cmd_bit_set |= (device->commands->write_enable|         /*command*/
                SFLASH_SEND_CMD|                                    /*transfer the command*/
                SFLASH_SEND_ADDR_BYTE_NUM_0|                        /*address num*/
                SFLASH_SEND_DUMMY_BYTE_NUM_0|                       /* dummy cycle*/
                SFLASH_RWN_NOTHING|                                 /*no read/write data*/
                SFLASH_CMD_MODE_1X|                                 /*set the sflash cmd mode*/
                SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                      /*set the sflash adr mode*/
                SFLASH_DATA_CYCLE_NUM_0|                            /*set the sflash data mode*/
                (0<<SFLASH_TRANSFER_BYTE_LOC)|                      /*transfer data number*/
                SFLASH_HOLD_TIME_100ns
                );

            sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
            sflash_set_command(sflash_cmd.all);
            sflash_get_data();

            cmd_bit_set |= (device->commands->program_page|         /*command*/
                SFLASH_SEND_CMD|                                    /*transfer the command*/
                SFLASH_SEND_ADDR_BYTE_NUM_3|                        /*address num*/
                SFLASH_SEND_DUMMY_BYTE_NUM_0|                       /* dummy cycle*/
                SFLASH_RWN_WRITE|                                   /*write data*/
                SFLASH_CMD_MODE_1X|                                 /*set the sflash cmd mode*/
                SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                      /*set the sflash adr mode*/
                SFLASH_DATA_CYCLE_NUM_0|                            /*set the sflash data mode*/
                SFLASH_TRANSFER_BYTE_NUM_4|                         /*transfer data number*/
                SFLASH_HOLD_TIME_100ns
                );

            sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
            sflash_set_command(sflash_cmd.all);
            sflash_set_data(offset);
        }

        sflash_set_data(*buffer++);
        count++;
        offset += 4;

        if((offset % device->page_bytes) == 0)
        {
            /* we are at a page boundary so we have to
                wait until the WIP status to be cleared by the device*/
            result = sflash_wip_done_wait(device);
            if( result != 0 )
            {
                break;
            }
            count = 0;
        }
    }

    /*new added for the case that words < page_words*/
    //if(data_len < page_words)
    {
        result = sflash_wip_done_wait(device);
        if(result != 0)
        {
            return(result);
        }
    }

    if(extadd)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }
    cmd_bit_set |= (device->commands->read_status|      /*command*/
        SFLASH_SEND_CMD|                                /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
        SFLASH_RWN_READ|                                /*read data*/
        SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                        /*set the sflash data mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)|                  /*transfer data number*/
        SFLASH_HOLD_TIME_100ns
        );

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    sflash_get_data();

    return(result);
}

static int sflash_write_io2( goke_sflash_dev_s* device, u32 offset, u32* buffer, u32 data_len  )
{
    u32 count                                       = 0;
    u32 page_words;
    u32 write_words                                 = data_len;
    u32 cmd_bit_set                                 = 0;
    goke_sflash_command_s   sflash_cmd;
    u32 result                                      = 0;
    u32 extadd = 0;

    if( !device )
        return(EINVAL);

    page_words = device->page_bytes / sizeof(u32);

    while(write_words--)
    {
        if(count == 0)
        {
            // >16MB
            if((extadd == 0) && (offset >= GD_SFLASH_16M_SIZE))
            {
                extadd = 1;
                sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
            }
            /* issue a write command sequence to prepare the device for data to be written*/
            cmd_bit_set |= (device->commands->write_enable|     /*command*/
                SFLASH_SEND_CMD|                                /*transfer the command*/
                SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
                SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /*dummy cycle*/
                SFLASH_RWN_NOTHING|                             /*no read/write data*/
                SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
                SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
                SFLASH_DATA_CYCLE_NUM_0|                        /*set the sflash data mode*/
                (0<<SFLASH_TRANSFER_BYTE_LOC)                   /*transfer data number*/
                );

            sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
            sflash_set_command(sflash_cmd.all);
            sflash_get_data();

            cmd_bit_set |= (device->commands->program_page2|    /*command*/
                SFLASH_SEND_CMD|                                /*transfer the command*/
                SFLASH_SEND_ADDR_BYTE_NUM_3|                    /*address num*/
                SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /*dummy cycle*/
                SFLASH_RWN_WRITE|                               /*write data*/
                SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
                SFLASH_ADDR_DUMMY_CYCLE_NUM_1|                  /*set the sflash adr mode*/
                SFLASH_DATA_CYCLE_NUM_1|                        /*set the sflash data mode*/
                (0x1f<<SFLASH_TRANSFER_BYTE_LOC)                /*transfer data number*/
                );

            // >16MB
            if(offset >= GD_SFLASH_16M_SIZE)
            {
                sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
            }
            sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
            sflash_set_command(sflash_cmd.all);
            sflash_set_data(offset);
        }

        sflash_set_data(*buffer++);
        count++;
        offset += 4;

        if((offset % device->page_bytes ) == 0)
        {
            /*we are at a page boundary so we have to
                 wait until the WIP status to be cleared by the device*/
            result = sflash_wip_done_wait(device);
            if(result != 0)
            {
                break;
            }
            count = 0;
        }
    }

    /*new added for the case that words < page_words*/
    //if(data_len < page_words)
    {
        result = sflash_wip_done_wait(device);
        if(result != 0)
        {
            return(result);
        }
    }

    if(extadd)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }
    cmd_bit_set |= (device->commands->read_status|      /*command*/
        SFLASH_SEND_CMD|                                /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
        SFLASH_RWN_READ|                                /*read data*/
        SFLASH_CMD_MODE_1X|                             /*set the sflash cmd mode*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_1|                  /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_1|                        /*set the sflash data mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)                   /*transfer data number*/
        );

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    sflash_get_data();
    return(result);
}

static int sflash_write_io4( goke_sflash_dev_s* device, u32 offset, u32* buffer, u32 data_len )
{
    u32 count                                            = 0;
    u32 page_words;
    u32 write_words                                     = data_len;
    u32 cmd_bit_set                                     = 0;
    goke_sflash_command_s   sflash_cmd;
    u32 result                                         = 0;
    u32 extadd = 0;

    if( !device )
        return(EINVAL);

    page_words = device->page_bytes / sizeof(u32);

    while(write_words--)
    {
        if(count == 0)
        {
            // >16MB
            if((extadd == 0) && (offset >= GD_SFLASH_16M_SIZE))
            {
                extadd = 1;
                sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
            }
            //if(device->manufacture_ID != 0x20)
            //    sflash_enable_io4(device);
            /*issue a write command sequence to prepare the device for data to be written*/
            cmd_bit_set |= (device->commands->write_enable|     /*command*/
                SFLASH_SEND_CMD|                                /*transfer the command*/
                SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
                SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
                SFLASH_RWN_NOTHING|                             /*no read/write data*/
                SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
                SFLASH_DATA_CYCLE_NUM_0|                        /*set the sflash data mode*/
                (0<<SFLASH_TRANSFER_BYTE_LOC)                   /*transfer data number*/
                );

            if(device->manufacture_ID == 0xbf)
            {
                cmd_bit_set |= SFLASH_CMD_MODE_4X;              /*set the sflash cmd mode*/
            }
            else if(device->manufacture_ID == 0x1c)
            {
                cmd_bit_set |= SFLASH_CMD_MODE_4X;              /*set the sflash cmd mode*/
            }
            else
            {
                cmd_bit_set |= SFLASH_CMD_MODE_1X;              /*set the sflash cmd mode*/
            }
            sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
            sflash_set_command(sflash_cmd.all);
            sflash_get_data();

            cmd_bit_set |= (device->commands->program_page4|    /*command*/
                SFLASH_SEND_CMD|                                /*transfer the command*/
                SFLASH_SEND_ADDR_BYTE_NUM_3|                    /*address num*/
                SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
                SFLASH_RWN_WRITE|                               /*write data*/
                SFLASH_DATA_CYCLE_NUM_2|                        /*set the sflash data mode*/
                (0x1f<<SFLASH_TRANSFER_BYTE_LOC)                /*transfer data number*/
                );

            if(device->manufacture_ID == 0xbf)
            {
                cmd_bit_set |= SFLASH_CMD_MODE_4X;              /*set the sflash cmd mode*/
            }
            else if  (device->manufacture_ID == 0x1c)
            {
                cmd_bit_set |= SFLASH_CMD_MODE_4X;
            }
            else
            {
                cmd_bit_set |= SFLASH_CMD_MODE_1X;              /*set the sflash cmd mode*/
            }


            if(device->manufacture_ID == 0xc2)
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_2;   /*set the sflash adr mode*/
            }
            else if(device->manufacture_ID == 0xbf)
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_2;   /*set the sflash adr mode*/
            }
            else if(device->manufacture_ID == 0xef)
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_0;   /*set the sflash adr mode*/
            }
            else if(device->manufacture_ID == 0x01)             /*for FL064P;Tom.wang;2010-10-21*/
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_0;   /*set the sflash adr mode*/
            }
            else  if(device->manufacture_ID == 0x20)
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_2;
            }
            else if(device->manufacture_ID == 0xc8)
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_0;
            }
            else if(device->manufacture_ID == 0x1c)
            {
                cmd_bit_set |= SFLASH_ADDR_DUMMY_CYCLE_NUM_2;
            }
            sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
            sflash_set_command(sflash_cmd.all);
            sflash_set_data(offset);
        }

        sflash_set_data(*buffer++);
        count++;
        offset += 4;

        if((offset % device->page_bytes) == 0)
        {
            //if(device->manufacture_ID != 0x20)
            //sflash_disenable_io4(device);
            /* we are at a page boundary so we have to
             wait until the WIP status to be cleared by the device*/
            result = sflash_wip_done_wait(device);
            if(result != 0)
            {
                break;
            }
            count = 0;
        }
    }

    //if(device->manufacture_ID != 0x20)
    //{
    //    sflash_disenable_io4(device);
    //}

    /*new added for the case that words < page_words*/
    //if(data_len < page_words)
    {
        result = sflash_wip_done_wait(device);
        if(result != 0)
        {
            return(result);
        }
    }
    if(extadd)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }
    cmd_bit_set |= (device->commands->read_status|      /*command*/
        SFLASH_SEND_CMD|                                /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_0|                    /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
        SFLASH_RWN_READ|                                /*read data*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
        (1<<SFLASH_TRANSFER_BYTE_LOC)                   /*transfer data number*/
        );

    if(device->manufacture_ID == 0xbf)
    {
        /*set the sflash cmd mode, set the sflash data mode*/
        cmd_bit_set |= (SFLASH_CMD_MODE_4X|SFLASH_DATA_CYCLE_NUM_2);
    }
    else
    {
        /*set the sflash cmd mode, set the sflash data mode*/
        cmd_bit_set |= (SFLASH_CMD_MODE_1X|SFLASH_DATA_CYCLE_NUM_0);
    }

    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);
    sflash_set_command(sflash_cmd.all);
    sflash_get_data();

    return(result);
}

static int sflash_erase_chip(goke_sflash_dev_s* device)
{
    goke_sflash_command_s   sflash_cmd;
    ulong CESetting = 0x38;
    CESetting = CESetting | ((ulong)device->channel << 6);
    sflash_set_ce(CESetting);

    if(old_feature == GOKE_SFLASH_FEATURE_IO4)
    {
        old_feature = GOKE_SFLASH_FEATURE_IO1;
        if(device->manufacture_ID != 0x20)
        {
            sflash_disenable_io4(device);                  /*disable the IO4 mode*/
        }
    }
    if( !device )
        return(EINVAL);


    sflash_wip_done_wait(device);

    sflash_write_enable(device);

    sflash_cmd.bitc.code = device->commands->erase_chip; //command
    sflash_cmd.bitc.data_num = 0x1f;
    sflash_set_command( sflash_cmd.all );

    sflash_wip_done_wait( device );

    //sflash_write_disable(device);

    return 0;
}

static int sflash_erase_sector(goke_sflash_dev_s* device, uint32_t offset)
{
    u32 cmd_bit_set                             = 0;
    goke_sflash_command_s   sflash_cmd;
    u32 result;
    ulong CESetting = 0x38;

    if( !device )
        return(EINVAL);

    CESetting = CESetting | ((ulong)device->channel << 6);
    sflash_set_ce(CESetting);
    if( (offset + device->sector_bytes) > device->device_bytes )
        return(EINVAL);

    // >16MB
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        sflash_set_ext_addr(device, offset/GD_SFLASH_16M_SIZE);
    }
    if(old_feature == GOKE_SFLASH_FEATURE_IO4)
    {
        old_feature = GOKE_SFLASH_FEATURE_IO1;
        if(device->manufacture_ID != 0x20)
        {
            sflash_disenable_io4(device);               /*disable the IO4 mode*/
        }
    }

    sflash_write_enable(device);

    cmd_bit_set |= (device->commands->erase_sector|     /*command*/
        SFLASH_SEND_CMD|                                /*transfer the command*/
        SFLASH_SEND_ADDR_BYTE_NUM_3|                    /*address num*/
        SFLASH_SEND_DUMMY_BYTE_NUM_0|                   /* dummy cycle*/
        SFLASH_RWN_NOTHING|                             /*no read/write data*/
        SFLASH_ADDR_DUMMY_CYCLE_NUM_0|                  /*set the sflash adr mode*/
        SFLASH_DATA_CYCLE_NUM_0|                        /*set the sflash data mode*/
        (0<<SFLASH_TRANSFER_BYTE_LOC)|                  /*transfer data number*/
        SFLASH_HOLD_TIME_100ns
        );
    sflash_cmd_bit_set(&sflash_cmd,&cmd_bit_set);

    sflash_set_command(sflash_cmd.all);
    sflash_set_data(offset);
    result = sflash_wip_done_wait(device);
    if(offset >= GD_SFLASH_16M_SIZE)
    {
        //restore to 0x00
        sflash_set_ext_addr(device, 0x00);
    }

    sflash_cmd.bitc.code = device->commands->write_disable; /*command*/
    if(device->manufacture_ID == 0xbf)
    {
        sflash_cmd.bitc.cmd_mode = 2;                       /*set the sflash cmd mode*/
    }
    else if(device->manufacture_ID == 0x1c)
    {
        sflash_cmd.bitc.cmd_mode = 0;
    }
    else
    {
        sflash_cmd.bitc.cmd_mode = 0;                       /*set the sflash cmd mode*/
    }
    sflash_cmd.bitc.adr_num = 0;                            /*address num*/

    sflash_set_command(sflash_cmd.all);
    sflash_get_data();
    CESetting = CESetting & (~((ulong)device->channel << 6));
    sflash_set_ce(CESetting);
    return(result);
}

static goke_sflash_feature_e sflash_get_feature(goke_sflash_dev_s* device, goke_sflash_type_e type)
{
    goke_sflash_feature_e feature               = GOKE_SFLASH_FEATURE_IO1;
    u8 temp                                     = 0;

    if( !device )
    {
        return feature;
    }
    if(type == GOKE_SFLASH_TYPE_READ)
    {
        temp = device->feature & 0x0F;
    }
    else
    {
        temp = (device->feature & 0xF0) >> 4;
    }
    switch(temp)
    {
    case 0x01:
        feature = GOKE_SFLASH_FEATURE_IO1;
        break;
    case 0x02:
        feature = GOKE_SFLASH_FEATURE_IO2;
        break;
    case 0x04:
        feature = GOKE_SFLASH_FEATURE_IO4;
        break;
    default:
        break;
    }
    return feature;
}

static u32 sflash_cmd_bit_set(goke_sflash_command_s *sflashCmd, u32 *cmd_bit_set)
{
    if(!sflashCmd)
    {
        return -1;
    }

    sflashCmd->all = (*cmd_bit_set);
    (*cmd_bit_set) = 0;

    return 0;
}

static int sflash_write_iox( goke_sflash_dev_s* device, uint32_t address,
                               uint32_t* buffer, uint32_t words )
{
    goke_sflash_feature_e feature = GOKE_SFLASH_FEATURE_IO1;
    uint32_t CESetting = 0x38;
    uint32_t err = 0;

    feature = sflash_get_feature(device, GOKE_SFLASH_TYPE_WRITE);

    CESetting = CESetting | ((ulong)device->channel << 6);
    sflash_set_ce(CESetting);
    if(feature == GOKE_SFLASH_FEATURE_IO4) //support IO4 mode
    {
        sflash_set_ce(0x38 | ((u32)device->channel << 6));
        if(old_feature != feature)
        {
            old_feature = feature;
            if(device->manufacture_ID != 0x20)
            {
                err = sflash_enable_io4(device);                 /*enable the IO4 mode*/
            }
            if(err != 0)
            {
                return err;
            }
        }
        sflash_write_io4( device, address, buffer, words );
    }
    else if( feature == GOKE_SFLASH_FEATURE_IO2) //support IO2 mode
    {
        sflash_set_ce(0x0E | ((u32)device->channel << 6));
        if(old_feature != feature)
        {
            old_feature = feature;
            if(device->manufacture_ID != 0x20)
            {
                err = sflash_disenable_io4(device);                  /*disable the IO4 mode*/
            }
        }
        sflash_write_io2( device, address, buffer, words );
    }
    else
    {
        sflash_set_ce(0x0E | ((u32)device->channel << 6));
        if(old_feature != feature)
        {
            old_feature = feature;
            if(device->manufacture_ID != 0x20)
            {
                err = sflash_disenable_io4(device);                  /*disable the IO4 mode*/
            }
        }
        sflash_write_io1( device, address, buffer, words );
    }

    CESetting = CESetting & (~((ulong)device->channel << 6));
    sflash_set_ce(CESetting);

    return 0;
}

static int sflash_read_iox( goke_sflash_dev_s* device, uint32_t address,
                               uint32_t* buffer, uint32_t words )
{
    goke_sflash_feature_e feature = GOKE_SFLASH_FEATURE_IO1;
    uint32_t CESetting = 0x38;
    uint32_t err = 0;

    feature = sflash_get_feature(device, GOKE_SFLASH_TYPE_READ);
    CESetting = CESetting | ((ulong)device->channel << 6);
    sflash_set_ce(CESetting);

    if(feature == GOKE_SFLASH_FEATURE_IO4) //support IO4 mode
    {
        sflash_set_ce(0x38 | ((u32)device->channel << 6));
        if(old_feature != feature)
        {
            old_feature = feature;
            if(device->manufacture_ID != 0x20)
            {
                err = sflash_enable_io4(device);                 /*enable the IO4 mode*/
            }
            if(err != 0)
            {
                return err;
            }
        }
        sflash_read_io4( device, address, buffer, words );
    }
    else if( feature == GOKE_SFLASH_FEATURE_IO2) //support IO2 mode
    {
        sflash_set_ce(0x0E | ((u32)device->channel << 6));
        if(old_feature != feature)
        {
            old_feature = feature;
            if(device->manufacture_ID != 0x20)
            {
                err = sflash_disenable_io4(device);                  /*disable the IO4 mode*/
            }
        }
        sflash_read_io2( device, address, buffer, words );
    }
    else
    {
        sflash_set_ce(0x0E | ((u32)device->channel << 6));
        if(old_feature != feature)
        {
            old_feature = feature;
            if(device->manufacture_ID != 0x20)
            {
                err = sflash_disenable_io4(device);                  /*disable the IO4 mode*/
            }
        }
        sflash_read_io1( device, address, buffer, words );
    }

    CESetting = CESetting & (~((ulong)device->channel << 6));
    sflash_set_ce(CESetting);
    return err;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int mtd_flash_write(struct mtd_info *mtd, loff_t to, size_t len,
                                size_t *retlen, const u_char *buf)
{
    struct gk_sflash   *flash = mtd_to_gk_flash(mtd);

    uint32_t addr,left,align_data;
    uint32_t *wptr = NULL;

    if (!flash)
        return -EINVAL;

    if( !(flash->device) )
        return -EINVAL;

    if (retlen)
        *retlen = 0;

    /* sanity checks */
    if (!len)
        return(0);

    if (to + len > flash->mtd.size)
        return -EINVAL;

    mutex_lock(&flash->lock);

    addr = to;
    wptr = (uint32_t*)buf;
    left = len % (sizeof(uint32_t));
#if GD_SFLASH_USE_BUFFER
    if(((addr>=flash->device->offset) && (addr<(flash->device->offset+flash->device->buffer_size))) ||
        (((addr+len)>=flash->device->offset) && ((addr+len)<(flash->device->offset+flash->device->buffer_size))))
    {
        flash->device->offset = 0xFFFFFFFF;
        //printk("[%s %d] 0x%08x 0x%08x 0x%08x\n", __func__, __LINE__, flash->device->offset, addr, len);
    }
#endif

    sflash_write_iox( flash->device, addr, wptr, len / (sizeof(uint32_t)) );

    if(left > 0)
    {
        sflash_read_iox( flash->device, addr + (len - left), &align_data, 1 );
        memcpy( (u_char*)&align_data, buf + (len - left), left );
        sflash_write_iox( flash->device, addr + (len - left), &align_data, 1);
    }

    *retlen = len;

    mutex_unlock(&flash->lock);

    return 0;
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int mtd_flash_read(struct mtd_info *mtd, loff_t from, size_t len,
                                size_t *retlen, u_char *buf)
{
    struct gk_sflash   *flash = mtd_to_gk_flash(mtd);

    unsigned int addr;
    unsigned int end;
    unsigned int off_set = 0;
#if GD_SFLASH_USE_BUFFER
    unsigned int left_bytes;
#else
    unsigned char buffer[256 + 10];
    unsigned int rd_pages = 0;
    unsigned int cnt;
    unsigned int start_addr;
    unsigned int p_start, p_end;
#endif

    if (!flash)
        return -EINVAL;

    if( !(flash->device) )
        return -EINVAL;

    /* sanity checks */
    if (!len)
        return 0;

    if (from + len > flash->mtd.size)
        return -EINVAL;

    /* Byte count starts at zero. */
    if (retlen)
        *retlen = 0;

    mutex_lock(&flash->lock);
    addr = from;
    off_set = 0;
    end = addr + len;
#if GD_SFLASH_USE_BUFFER
    if((len >= flash->device->buffer_size) && ((addr%4) == 0x00) && ((len%4) == 0x00) && (((u32)buf%4) == 0x00))
    {
        sflash_read_iox(flash->device, addr, (uint32_t*)buf, len / 4);
        *retlen = len;
        mutex_unlock(&flash->lock);
        return 0;
    }

    if((addr>=flash->device->offset) && (addr<(flash->device->offset+flash->device->buffer_size)))
    {
    }
    else
    {
        flash->device->offset = addr - (addr % flash->device->buffer_size);
        sflash_read_iox(flash->device, flash->device->offset, (uint32_t*)flash->device->buffer, flash->device->buffer_size / 4);
    }
    do
    {
        if((addr>=flash->device->offset) && (addr<(flash->device->offset+flash->device->buffer_size)))
        {
            left_bytes = flash->device->buffer_size - (addr - flash->device->offset);
            if(left_bytes >= len)
            {
                memcpy(buf + off_set, flash->device->buffer + (addr - flash->device->offset), len);
                *retlen = len;
                mutex_unlock(&flash->lock);
                return 0;
            }
            memcpy(buf + off_set, flash->device->buffer + (addr - flash->device->offset), left_bytes);
            off_set += left_bytes;
            len -= left_bytes;
            addr += left_bytes;
            sflash_read_iox(flash->device, flash->device->offset+flash->device->buffer_size, (uint32_t*)flash->device->buffer, flash->device->buffer_size / 4);
            flash->device->offset += flash->device->buffer_size;
        }
    }while(1);
#else

    if(((addr%4) == 0x00) && ((len%4) == 0x00) && (((u32)buf%4) == 0x00))
    {
        sflash_read_iox( flash->device, addr, (uint32_t*)buf, len / 4);
        mutex_unlock(&flash->lock);
        *retlen = len;
        return 0;
    }
    if (addr%256 == 0) {
        start_addr = addr;
    } else {
        start_addr = addr  - (addr % 256);
    }

    if (addr%256 != 0) {
        if ((addr + len) > (start_addr + 256)) {
            p_start = start_addr + 256;
            rd_pages += 1;
        } else {
            p_start = start_addr;
        }

    } else {
        p_start = start_addr;
    }

    if (end%256 != 0) {
        rd_pages += 1;
        p_end = end - (end%256);
    } else {
        p_end = end;
    }

    rd_pages += (p_end - p_start) / 256;

    //printk("\n\nread addr : 0x%08x, start : 0x%08x, len : %d, rd_pages : %d, p_start : 0x%x, p_end : 0x%x\n", addr, start_addr, len, rd_pages, p_start, p_end);

    for (cnt = 0; cnt < rd_pages;) {
        sflash_read_iox( flash->device, start_addr + cnt*256, (uint32_t*)buffer, 64);
        if (cnt == 0) {
            if (addr + len < start_addr + 256) {
                memcpy(buf + off_set, buffer + (addr % 256), len);
                off_set += len;
            } else {
                memcpy(buf + off_set, buffer + (addr % 256), 256 - addr%256);
                off_set += (256 - addr%256);
            }

            //printk("cnt : %d, offset : %d\n", cnt, off_set);
        } else if (cnt == (rd_pages - 1)) {
            if (end % 256 == 0) {
                memcpy(buf + off_set, buffer, 256);
                off_set += 256;
            } else {
                memcpy(buf + off_set, buffer, end % 256);
                off_set += (end % 256);
            }
            //printk("cnt : %d, offset : %d\n", cnt, off_set);
        } else {
            memcpy(buf + off_set, buffer, 256);
            off_set += 256;
            //printk("cnt : %d, offset : %d, read num : %d\n", cnt, off_set, 256);
        }

        cnt++;
    }

#endif

    *retlen = len;
    mutex_unlock(&flash->lock);

    return 0;

}

/*
 * Erase an address range on the flash chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int mtd_flash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct gk_sflash   *flash = mtd_to_gk_flash(mtd);
    uint32_t addr,len;
    uint32_t rem;

    if( !flash)
        return -EINVAL;

    if( !(flash->device) )
        return -EINVAL;

    /* sanity checks */
    if (instr->addr + instr->len > flash->mtd.size)
        return -EINVAL;

    div_u64_rem(instr->len, mtd->erasesize, &rem);
    if (rem)
        return -EINVAL;

    addr = instr->addr;
    len = instr->len;

    //printk("erase, addr : 0x%08x, len : %d\n", addr, len);

    mutex_lock(&flash->lock);
#if GD_SFLASH_USE_BUFFER
    if(((addr>=flash->device->offset) && (addr<(flash->device->offset+flash->device->buffer_size))) ||
        (((addr+len)>=flash->device->offset) && ((addr+len)<(flash->device->offset+flash->device->buffer_size))))
    {
        flash->device->offset = 0xFFFFFFFF;
        //printk("[%s %d] 0x%08x 0x%08x 0x%08x\n", __func__, __LINE__, flash->device->offset, addr, len);
    }
#endif

    /* whole-chip erase? */
    if (len == flash->mtd.size)
    {
        if (sflash_erase_chip(flash->device))
        {
            instr->state = MTD_ERASE_FAILED;
            mutex_unlock(&flash->lock);
            return -EIO;
        }

    /* REVISIT in some cases we could speed up erasing large regions
     * by using OPCODE_SE instead of OPCODE_BE_4K.  We may have set up
     * to use "small sector erase", but that's not always optimal.
     */

    /* "sector"-at-a-time erase */
    }
    else
    {
        while (len)
        {
            if (sflash_erase_sector(flash->device, addr))
            {
                instr->state = MTD_ERASE_FAILED;
                mutex_unlock(&flash->lock);
                return -EIO;
            }

            addr += mtd->erasesize;
            len -= mtd->erasesize;
        }
    }

    mutex_unlock(&flash->lock);

    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return 0;

}


static int  gk_flash_probe(struct platform_device *dev)
{
    struct gk_sflash   *flash;
    goke_sflash_dev_s       *chip_info;
    struct mtd_part_parser_data ppdata;
    struct sflash_platform_data *data = dev->dev.platform_data;
    unsigned               i;
    int ret;

    chip_info = jedec_probe(dev);

    if (!chip_info){
        printk(KERN_ERR "detect sflash fail\n");
        return -ENODEV;
    }

    flash = kzalloc(sizeof *flash, GFP_KERNEL);
    if (!flash)
        return -ENOMEM;

    flash->device = chip_info;
    mutex_init(&flash->lock);
    platform_set_drvdata(dev, flash);

    flash->mtd.name = dev->name;

    flash->mtd.type = MTD_NORFLASH;
    flash->mtd.writesize = 1;
    flash->mtd.flags = MTD_CAP_NORFLASH;
    flash->mtd.size  = chip_info->device_bytes;
    flash->mtd._erase = mtd_flash_erase;
    flash->mtd._read  = mtd_flash_read;
    flash->mtd._write = mtd_flash_write;

    flash->mtd.erasesize = chip_info->sector_bytes;

    ppdata.of_node = dev->dev.of_node;
    flash->mtd.dev.parent = &dev->dev;
    flash->mtd.writebufsize = chip_info->page_bytes;

    dev_info(&dev->dev, "%s (%lld Kbytes)\n",  chip_info->device_name,
            (long long)flash->mtd.size >> 10);

    if (flash->mtd.numeraseregions)
        for (i = 0; i < flash->mtd.numeraseregions; i++)
            printk("mtd.eraseregions[%d] = { .offset = 0x%llx, "
                ".erasesize = 0x%.8x (%uKiB), "
                ".numblocks = %d }\n",
                i, (long long)flash->mtd.eraseregions[i].offset,
                flash->mtd.eraseregions[i].erasesize,
                flash->mtd.eraseregions[i].erasesize / 1024,
                flash->mtd.eraseregions[i].numblocks);

    /* partitions should match sector boundaries; and it may be good to
     * use readonly partitions for writeprotected sectors (BP2..BP0).
     */
    ret = mtd_device_parse_register(&flash->mtd, NULL, &ppdata,
                data ? data->parts : NULL,
                data ? data->nr_parts : 0);
    //    printk(KERN_ERR "sflash dev_t = 0x%x\n", flash->mtd.dev.devt);

    return ret;
}


static int gk_flash_remove(struct platform_device *dev)
{
    struct gk_sflash *flash = platform_get_drvdata(dev);
    int    status;

    /* Clean up MTD stuff. */
    status = mtd_device_unregister(&flash->mtd);
    if (status == 0) {
        kfree(flash);
    }
    return 0;
}

#ifdef CONFIG_OF
static struct of_device_id match_table[] = {
    { .compatible = "mxic,mx25l12845e" },
    { .compatible = "spansion,s25fl128p" },
    { }    /* end */
};
#else    /* CONFIG_OF */
#define match_table NULL

#endif    /* !CONFIG_OF */

static struct platform_driver gk_flash_driver =
{
    .driver =
    {
        .name = "gk_flash",
        .owner = THIS_MODULE,
        .of_match_table = match_table,
    },
    .probe  = gk_flash_probe,
    .remove = gk_flash_remove,
};


#ifdef CONFIG_OF
module_platform_driver(gk_flash_driver);
#else    /* CONFIG_OF */
static int gk_flash_init(void)
{
    int retval = 0;

//    printk(KERN_ERR "sflash pa = 0x%x, va = 0x%x\n",(unsigned int)GK_PA_SFLASH ,(unsigned int)GK_VA_SFLASH);

    retval = platform_driver_register(&gk_flash_driver);
//    printk(KERN_ERR "sflash init ret = %d\n", retval);

    return retval;
}

static void gk_flash_exit(void)
{
    platform_driver_unregister(&gk_flash_driver);
}


module_init(gk_flash_init);
module_exit(gk_flash_exit);
#endif    /* !CONFIG_OF */

MODULE_DESCRIPTION("GOKE MTD serial flash driver for MTD");
MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_LICENSE("GPL");

