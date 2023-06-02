/*
 * Copyright (c) 2015 HiSilicon Technologies Co., Ltd.
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HI_DMAC_HI3516A_H__
#define __HI_DMAC_HI3516A_H__

#define DDRAM_ADRS  0x80000000      /* fixed */
#define DDRAM_SIZE  0x1FFFFFFF      /* 512M DDR. */

#define FLASH_BASE  0x10000000
#define FLASH_SIZE  0x04000000      /* (32MB) */

#define DMAC_INTSTATUS      0X00
#define DMAC_INTTCSTATUS    0X04
#define DMAC_INTTCCLEAR     0X08
#define DMAC_INTERRORSTATUS 0X0C

#define DMAC_INTERRCLR      0X10
#define DMAC_RAWINTTCSTATUS 0X14
#define DMAC_RAWINTERRORSTATUS  0X18
#define DMAC_ENBLDCHNS      0X1C
#define DMAC_CONFIG         0X30
#define DMAC_SYNC           0X34

#define DMAC_MAXTRANSFERSIZE    0x0fff /*the max length is denoted by 0-11bit*/
#define MAXTRANSFERSIZE     DMAC_MAXTRANSFERSIZE
#define DMAC_CxDISABLE      0x00
#define DMAC_CxENABLE       0x01

/*the definition for DMAC channel register*/
#define DMAC_CxBASE(i)      (0x100+i*0x20)
#define DMAC_CxSRCADDR(i)   DMAC_CxBASE(i)
#define DMAC_CxDESTADDR(i)  (DMAC_CxBASE(i)+0x04)
#define DMAC_CxLLI(i)       (DMAC_CxBASE(i)+0x08)
#define DMAC_CxCONTROL(i)   (DMAC_CxBASE(i)+0x0C)
#define DMAC_CxCONFIG(i)    (DMAC_CxBASE(i)+0x10)

/*the means the bit in the channel control register*/
#define DMAC_CxCONTROL_M2M  0x9d480000  /* Dwidth=32,burst size=4 */
#define DMAC_CxCONTROL_LLIM2M       0x0f480000  /* Dwidth=32,burst size=1 */
#define DMAC_CxCONTROL_LLIM2M_ISP   0x0b489000  /* Dwidth=32,burst size=1 */
#define DMAC_CxCONTROL_LLIP2M    0x0a000000
#define DMAC_CxCONTROL_LLIM2P    0x86089000
#define DMAC_CxLLI_LM       0x01

#define NUM_HAL_INTERRUPT_DMAC         (14 + 16)

#define DMAC_CxCONFIG_M2M   0xc000
#define DMAC_CxCONFIG_LLIM2M    0xc000

/*#define DMAC_CxCONFIG_M2M  0x4001*/
#define DMAC_CHANNEL_ENABLE 1
#define DMAC_CHANNEL_DISABLE    0xfffffffe

#define DMAC_CxCONTROL_P2M  0x89409000
#define DMAC_CxCONFIG_P2M   0xd000

#define DMAC_CxCONTROL_M2P  0x86089000
#define DMAC_CxCONFIG_M2P   0xc800

#define DMAC_CxCONFIG_SIO_P2M   0x0000d000
#define DMAC_CxCONFIG_SIO_M2P   0x0000c800

/*default the config and sync regsiter for DMAC controller*/
/*M1,M2 little endian, enable DMAC*/
#define DMAC_CONFIG_VAL     0x01
/*enable the sync logic for the 16 peripheral*/
#define DMAC_SYNC_VAL       0x0

#define DMAC_MAX_PERIPHERALS    16
#define MEM_MAX_NUM     2
#define CHANNEL_NUM     CONFIG_HI_DMAC_CHANNEL_NUM
#define DMAC_MAX_CHANNELS   CHANNEL_NUM

#define MMC_REG_BASE        0x10030000
#define MMC_RX_REG      (MMC_REG_BASE+0x100)
#define MMC_TX_REG      (MMC_REG_BASE+0x100)

#define UART0_REG_BASE      0x20080000
#define UART0_DATA_REG      (UART0_REG_BASE + 0x0)

#define UART1_REG_BASE      0x20090000
#define UART1_DATA_REG      (UART1_REG_BASE + 0x0)

#define UART2_REG_BASE      0x200A0000
#define UART2_DATA_REG      (UART2_REG_BASE + 0x0)

#define UART3_REG_BASE      0x200B0000
#define UART3_DATA_REG      (UART3_REG_BASE + 0x0)

#define SPI0_REG_BASE       0x200c0000
#define SPI0_DATA_REG       (SPI0_REG_BASE + 0x08)

#define SPI1_REG_BASE       0x200e0000
#define SPI1_DATA_REG       (SPI1_REG_BASE + 0x08)

#define I2C0_REG_BASE       0x200d0000
#define I2C0_DATA_REG       (I2C0_REG_BASE + 0x10)

#define I2C1_REG_BASE       0x20240000
#define I2C1_DATA_REG       (I2C1_REG_BASE + 0x10)

#define PERI_8BIT_MODE            0
#define PERI_16BIT_MODE           1
#define PERI_32BIT_MODE           2
/*the transfer control and configuration value for different peripheral*/

extern int g_channel_status[CHANNEL_NUM];

/*
 *  DMA config array!
 *  DREQ, FIFO, CONTROL, CONFIG, BITWIDTH
 */
dmac_peripheral  g_peripheral[DMAC_MAX_PERIPHERALS] = {
    /* DREQ,  FIFO,   CONTROL,   CONFIG, WIDTH */
    /*periphal 0: I2C0/I2C1 RX*/
    { 0, I2C0_DATA_REG, 0x99000000, 0x1000, PERI_8BIT_MODE},
    /*periphal 1: I2C0/I2C1 TX*/
    { 1, I2C0_DATA_REG, 0x96000000, 0x0840, PERI_8BIT_MODE},
    /*periphal 2: I2C1/I2C2 RX*/
    { 2, I2C1_DATA_REG, 0x99000000, 0x1004, PERI_8BIT_MODE},    /*  8bit width */
    /*periphal 3: I2C1/I2C2 TX*/
    { 3, I2C1_DATA_REG, 0x96000000, 0x08c0, PERI_8BIT_MODE},    /*  8bit width */

    /*periphal 4: UART0 RX*/
    { 4, UART0_DATA_REG, DMAC_CxCONTROL_LLIP2M, DMAC_CxCONFIG_P2M | (4 << 1), PERI_8BIT_MODE},

    /*periphal 5: UART0 TX*/
    { 5, UART0_DATA_REG, DMAC_CxCONTROL_LLIM2P, DMAC_CxCONFIG_M2P | (5 << 1), PERI_8BIT_MODE},

    /*periphal 6: UART1 RX*/
    { 6, UART1_DATA_REG, DMAC_CxCONTROL_LLIP2M, DMAC_CxCONFIG_P2M | (6 << 1), PERI_8BIT_MODE},

    /*periphal 7: UART1 TX*/
    { 7, UART1_DATA_REG, DMAC_CxCONTROL_LLIM2P, DMAC_CxCONFIG_M2P | (7 << 1), PERI_8BIT_MODE},

    /*periphal 8: UART2 RX*/
    { 8, UART2_DATA_REG, DMAC_CxCONTROL_LLIP2M, DMAC_CxCONFIG_P2M | (8 << 1), PERI_8BIT_MODE},

    /*periphal 9: UART2 TX*/
    { 9, UART2_DATA_REG, DMAC_CxCONTROL_LLIM2P, DMAC_CxCONFIG_M2P | (9 << 1), PERI_8BIT_MODE},

    /*periphal 10: UART3 RX*/
    { 10, UART3_DATA_REG, DMAC_CxCONTROL_LLIP2M, DMAC_CxCONFIG_P2M | (10 << 1), PERI_8BIT_MODE},

    /*periphal 11: UART0 TX*/
    { 11, UART3_DATA_REG, DMAC_CxCONTROL_LLIM2P, DMAC_CxCONFIG_M2P | (11 << 1), PERI_8BIT_MODE},

    /*periphal 12: SSP1 RX*/
    { 12, 0, 0, 0, 0},

    /*periphal 13: SSP1 TX*/
    { 13, 0, 0, 0, 0},

    /*periphal 14: SSP0 RX*/
    { 14, 0, 0, 0, 0},

    /*periphal 15: SSP0 TX*/
    { 15, 0, 0, 0, 0},
};

#endif
