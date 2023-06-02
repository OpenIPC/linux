/*
 *  drivers/mtd/spinand/gk_spinand_v1_00.h
 *
 * gk soc
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

#include <mach/hardware.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE
#include <mach/io.h>
#include <mach/flash.h>

/* send cmd or not [ 8]*/
#define FLASH_SEND_CMD              0x00000100
#define FLASH_NO_SEND_CMD           0x00000000

 /* byte number of address to send [11:9]*/
#define FLASH_SEND_ADDR_BYTE_NUM_0  0x00000000
#define FLASH_SEND_ADDR_BYTE_NUM_1  0x00000200
#define FLASH_SEND_ADDR_BYTE_NUM_2  0x00000400
#define FLASH_SEND_ADDR_BYTE_NUM_3  0x00000600
#define FLASH_SEND_ADDR_BYTE_NUM_4  0x00000800
#define FLASH_SEND_ADDR_BYTE_NUM_5  0x00000a00
#define FLASH_SEND_ADDR_BYTE_NUM_6  0x00000c00
#define FLASH_SEND_ADDR_BYTE_NUM_7  0x00000e00

 /*Byte number of dummy cycle to send [14:12]*/

#define FLASH_SEND_DUMMY_BYTE_NUM_0  0x000000000
#define FLASH_SEND_DUMMY_BYTE_NUM_1  0x000001000
#define FLASH_SEND_DUMMY_BYTE_NUM_2  0x000002000
#define FLASH_SEND_DUMMY_BYTE_NUM_3  0x000003000
#define FLASH_SEND_DUMMY_BYTE_NUM_4  0x000004000
#define FLASH_SEND_DUMMY_BYTE_NUM_5  0x000005000
#define FLASH_SEND_DUMMY_BYTE_NUM_6  0x000006000
#define FLASH_SEND_DUMMY_BYTE_NUM_7  0x000007000

 /* Command operation[16:15]: 00 for read data from SF; 01 for write data to SF; 11 for nothing to do */

#define FLASH_RWN_READ    0x00000000
#define FLASH_RWN_WRITE   0x00008000
#define FLASH_RWN_NOTHING 0x00018000

 /*I/O mode of command cycle to SF[18:17]: 00 for x1; 01 for x2; 10 for x4*/
#define FLASH_CMD_MODE_1X 0x00000000
#define FLASH_CMD_MODE_2X 0x00020000
#define FLASH_CMD_MODE_4X 0x00040000

 /* I/O mode of address and dummy cycle to SF[20:19]*/
#define FLASH_ADDR_DUMMY_CYCLE_NUM_0 0x00000000
#define FLASH_ADDR_DUMMY_CYCLE_NUM_1 0x00080000
#define FLASH_ADDR_DUMMY_CYCLE_NUM_2 0x00100000
#define FLASH_ADDR_DUMMY_CYCLE_NUM_3 0x00180000

 /*I/O mode of data cycle to or from SF [22:21] */

#define FLASH_DATA_CYCLE_NUM_0    0x00000000
#define FLASH_DATA_CYCLE_NUM_1    0x00200000
#define FLASH_DATA_CYCLE_NUM_2    0x00400000
#define FLASH_DATA_CYCLE_NUM_3    0x00600000

 /*Transfer data byte number to or from SF[27:23]. For 11111 case, transfer 4bytes per request. For other case, transfer number bytes.*/

#define FLASH_TRANSFER_BYTE_NUM_4 0x0f800000
#define FLASH_TRANSFER_BYTE_LOC   23

#define FLASH_HOLD_TIME_100ns 0x00000000
#define FLASH_HOLD_TIME_3us   0x10000000
#define FLASH_HOLD_TIME_100us 0x20000000

 /*!
 *******************************************************************************
 **
 ** \brief Flash channel number.
 **
 *******************************************************************************
 */
 typedef enum
 {
    FLASH_CHANNEL_0 = 0,
    FLASH_CHANNEL_1,
    FLASH_CHANNEL_NUM
 }flash_channel_e;

 /*!
 *******************************************************************************
 **
 ** \brief Flash I/O feature.
 **
 *******************************************************************************
 */
 typedef enum
 {
    FLASH_FEATURE_IO1 = 0,
    FLASH_FEATURE_IO2,
    FLASH_FEATURE_IO4
 }flash_feature_e;


/*----------------------------------------------------------------------------*/
/* bit group structures                                                       */
/*----------------------------------------------------------------------------*/
typedef union { /* SFLASH_Command */
    uint32_t all;
    struct {
        uint32_t code                        : 8;
        uint32_t sendcmd                     : 1;
        uint32_t adrnum                      : 3;
        uint32_t dummynum                    : 3;
        uint32_t rwn                         : 2;
        uint32_t cmdmode                     : 2;
        uint32_t adrmode                     : 2;
        uint32_t datamode                    : 2;
        uint32_t datanum                     : 5;
        uint32_t holdtime                    : 2;
        uint32_t                             : 2;
    } bitc;
}flash_command_s;

 typedef union { /* SFLASH_CE */
    uint32_t all;
    struct {
        uint32_t ce                          : 1;
        uint32_t wp                          : 1;
        uint32_t hold                        : 1;
        uint32_t cemode                      : 1;
        uint32_t wpmode                      : 1;
        uint32_t holdmode                    : 1;
        uint32_t chselect                    : 1;
        uint32_t                             : 25;
    } bitc;
}flash_ce_s;

typedef union { /* SFLASH_Speed */
    uint32_t all;
    struct {
        uint32_t sf_sclk_sel                 : 3;
        uint32_t                             : 29;
    } bitc;
} flash_speed_s;

typedef union { /* SFLASH_PARA_XIP */
    uint32_t all;
    struct {
        uint32_t sflash_command              : 8;
        uint32_t                             : 1;
        uint32_t adr_num                     : 3;
        uint32_t dummy_num                   : 3;
        uint32_t                             : 2;
        uint32_t cmd_mod                     : 2;
        uint32_t adr_mod                     : 2;
        uint32_t data_mod                    : 2;
        uint32_t                             : 1;
        uint32_t dumy_data                   : 8;
    } bitc;
} flash_para_xip_s;

 /*----------------------------------------------------------------------------*/
/* register SFLASH_Data (read/write)                                          */
/*----------------------------------------------------------------------------*/
void spinand_set_data (uint32_t data)
{
	gk_ssi_writel(REG_SFLASH_DATA, data);
    //*(volatile uint32_t *)REG_SFLASH_DATA = data;
}
uint32_t  spinand_get_data (void)
{
	return gk_ssi_readl(REG_SFLASH_DATA);
    //return (*(volatile uint32_t *)REG_SFLASH_DATA);
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_Command (read/write)                                       */
/*----------------------------------------------------------------------------*/
void spinand_set_command(uint32_t data)
{
	gk_ssi_writel(REG_SFLASH_COMMAND, data);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = data;
}
uint32_t  spinand_get_command (void)
{
	return gk_ssi_readl(REG_SFLASH_COMMAND);
    //return (*(volatile uint32_t *)REG_SFLASH_COMMAND);
}
void spinand_set_command_code(uint8_t data)
{
    flash_command_s  d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.code = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t   spinand_get_command_code(void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.code;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.code;
}
void spinand_set_command_sendcmd(uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.sendcmd = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t   spinand_get_command_sendcmd(void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.sendcmd;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.sendcmd;
}
void spinand_set_command_adrnum (uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.adrnum = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t  spinand_get_command_adrnum (void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.adrnum;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.adrnum;
}
void spinand_set_command_dummynum (uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.dummynum = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t spinand_get_command_dummynum (void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.dummynum;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.dummynum;
}
void spinand_set_command_rwn(uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.rwn = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t spinand_get_command_rwn (void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.rwn;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.rwn;
}
void spinand_set_command_cmdmode (uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.cmdmode = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t  spinand_get_command_cmdmode (void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.cmdmode;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.cmdmode;
}
void spinand_set_command_adrmode (uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.adrmode = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t spinand_get_command_adrmode (void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.adrmode;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.adrmode;
}
void spinand_set_command_datamode(uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.datamode = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t   spinand_get_command_datamode(void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.datamode;


    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.datamode;
}
void spinand_set_command_datanum (uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.datanum = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t  spinand_get_command_datanum (void)
{
	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.datanum;

    //return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.datanum;
}
void spinand_set_command_holdtime (uint8_t data)
{
    flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
    //d.all = *(volatile uint32_t *)REG_SFLASH_COMMAND;
    d.bitc.holdtime = data;
	gk_ssi_writel(REG_SFLASH_COMMAND, d.all);
    //*(volatile uint32_t *)REG_SFLASH_COMMAND = d.all;
}
uint8_t  spinand_get_command_holdtime (void)
{

	flash_command_s d;
	d.all = gk_ssi_readl(REG_SFLASH_COMMAND);
	return d.bitc.holdtime;

	//return (*(volatile flash_command_s *)REG_SFLASH_COMMAND).bitc.holdtime;
}


/*----------------------------------------------------------------------------*/
/* register SFLASH_CE (read/write)                                            */
/*----------------------------------------------------------------------------*/
void spinand_set_ce (uint32_t data)
{
	gk_ssi_writel(REG_SFLASH_CE, data);
    //*(volatile uint32_t *)REG_SFLASH_CE = data;
}
uint32_t  spinand_get_ce(void)
{
	return gk_ssi_readl(REG_SFLASH_CE);
    //return (*(volatile uint32_t *)REG_SFLASH_CE);
}
void spinand_set_ce_ce(uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.ce = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t   spinand_get_ce_ce(void)
{

	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.ce;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.ce;
}
void spinand_set_ce_wp(uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.wp = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t   spinand_get_ce_wp(void)
{
	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.wp;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.wp;
}
void spinand_set_ce_hold (uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.hold = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t   spinand_get_ce_hold(void)
{
	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.hold;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.hold;
}
void spinand_set_ce_cemode (uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.cemode = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t   spinand_get_ce_cemode(void)
{
	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.cemode;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.cemode;
}
void spinand_set_ce_wpmode (uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.wpmode = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t  spinand_get_ce_wpmode(void)
{
	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.wpmode;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.wpmode;
}
void spinand_set_ce_holdmode (uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.holdmode = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t   spinand_get_ce_holdmode(void)
{
	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.holdmode;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.holdmode;
}
void spinand_set_ce_chselect (uint8_t data)
{
    flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
    //d.all = *(volatile uint32_t *)REG_SFLASH_CE;
    d.bitc.chselect = data;
	gk_ssi_writel(REG_SFLASH_CE, d.all);
    //*(volatile uint32_t *)REG_SFLASH_CE = d.all;
}
uint8_t   spinand_get_ce_chselect(void)
{
	flash_ce_s d;
	d.all = gk_ssi_readl(REG_SFLASH_CE);
	return d.bitc.chselect;

    //return (*(volatile flash_ce_s *)REG_SFLASH_CE).bitc.chselect;
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_Speed (read/write)                                         */
/*----------------------------------------------------------------------------*/
void spinand_set_speed (uint32_t data)
{
	gk_ssi_writel(REG_SFLASH_SPEED, data);
    //*(volatile uint32_t *)REG_SFLASH_SPEED = data;
}
uint32_t  spinand_get_speed(void)
{
	return gk_ssi_readl(REG_SFLASH_SPEED);
    //return (*(volatile uint32_t *)REG_SFLASH_SPEED);
}
void spinand_set_speed_sf_sclk_sel (uint8_t data)
{
    flash_speed_s  d;
	d.all = gk_ssi_readl(REG_SFLASH_SPEED);
    //d.all = *(volatile uint32_t *)REG_SFLASH_SPEED;
    d.bitc.sf_sclk_sel = data;
	gk_ssi_writel(REG_SFLASH_SPEED, d.all);
    //*(volatile uint32_t *)REG_SFLASH_SPEED = d.all;
}
uint8_t spinand_get_speed_sf_sclk_sel(void)
{
	flash_speed_s d;
	d.all = gk_ssi_readl(REG_SFLASH_SPEED);
	return d.bitc.sf_sclk_sel;

    //return (*(volatile flash_speed_s *)REG_SFLASH_SPEED).bitc.sf_sclk_sel;
}

/*----------------------------------------------------------------------------*/
/* register SFLASH_PARA_XIP (read/write)                                      */
/*----------------------------------------------------------------------------*/
void spinand_set_para_xip (uint32_t data)
{
	gk_ssi_writel(REG_SFLASH_PARA_XIP, data);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = data;
}
uint32_t  spinand_get_para_xip(void)
{
	return gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //return (*(volatile uint32_t *)REG_SFLASH_PARA_XIP);
}
void spinand_set_para_xip_sflash_command (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.sflash_command = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_sflash_command(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.sflash_command;

    //return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.sflash_command;
}
void spinand_set_para_xip_adr_num (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.adr_num = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_adr_num(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.adr_num;

    //return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.adr_num;
}
void spinand_set_para_xip_nummy_num (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.dummy_num = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_nummy_num(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.dummy_num;

   // return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.dummy_num;
}
void spinand_set_para_xip_cmd_mod (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
   // d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.cmd_mod = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_cmd_mod(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.cmd_mod;

    //return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.cmd_mod;
}
void spinand_set_para_xip_adr_mod (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.adr_mod = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_adr_mod(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.adr_mod;

    //return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.adr_mod;
}
void spinand_set_para_xip_data_mod (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.data_mod = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_data_mod(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.data_mod;

    //return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.data_mod;
}
void spinand_set_para_xip_nummy_data (uint8_t data)
{
    flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
    //d.all = *(volatile uint32_t *)REG_SFLASH_PARA_XIP;
    d.bitc.dumy_data = data;
	gk_ssi_writel(REG_SFLASH_PARA_XIP, d.all);
    //*(volatile uint32_t *)REG_SFLASH_PARA_XIP = d.all;
}
uint8_t   spinand_get_para_xip_nummy_data(void)
{
	flash_para_xip_s d;
	d.all = gk_ssi_readl(REG_SFLASH_PARA_XIP);
	return d.bitc.dumy_data;

    //return (*(volatile flash_para_xip_s *)REG_SFLASH_PARA_XIP).bitc.dumy_data;
}

