#ifndef __HI_CHIP_REGS_H__
#define __HI_CHIP_REGS_H__

#include <mach/io.h>

/* SRAM Base Address Register */
#define SRAM_BASE_ADDRESS                               0x4010000

#define SYS_CTRL_BASE                                    0x04520000
#define REG_BASE_SCTL               (SYS_CTRL_BASE + 0)
#define SYS_CTRL_SYSSTAT            0x8c
#define SYSSTAT_BOOTROM_SEL_IN          (1 << 31)
#define SYSSTAT_MULTIMODE           (1 << 7)

#define GET_SYS_BOOT_MODE(_reg)     (((_reg) >> 4) & 0x3)
#define BOOT_FROM_SPI           0x0
#define BOOT_FROM_NAND          0x1
#define BOOT_FROM_EMMC          0x2
#define BOOT_FROM_SLAVE         0x3

#define GET_SYS_SPI_DEVICE_TYPE(_reg)       (((_reg) & 0x1) >> 3)
#define DEVICE_TYPE_SPI_NOR     0
#define DEVICE_TYPE_SPI_NAND        1

/* bit[5:4:3]=000; bit[7]:SPI nor address mode; bit[7]=(0:3-Byte | 1:4-Byte) */
#define SPI_NOR_ADDRESS_MODE_MASK       (0x1 << 7)
/* bit[5:4:3]=001; bit[7]: SPI nand I/O widthe; bit[7]=(0: 1-I/O | 1: 4-I/O */
#define SPI_NAND_IO_WIDTHE_MASK         (0x1 << 7)
/* bit[5:4:3]=10*; bit[7]: EMMC I/O widthe; bit[7]=(0: 4-I/O | 1: 8-I/O */
#define EMMC_IO_WIDTHE_MASK         (0x1 << 7)

#define BOOT_MODE_MASK              ((0x3) << 4)

#define REG_BASE_SCTL                                   (SYS_CTRL_BASE + 0)

#define REG_BASE_TIMER01                                0x04500000
#define REG_BASE_TIMER23                                0x04501000


#define REG_BASE_CRG                                    0x04510000
#define REG_BASE_UART0                                  0x04540000
#define REG_BASE_UART1                                  0x04541000
#define REG_BASE_UART2                                  0x04542000
#define REG_BASE_UART3                                  0x04543000
#define REG_BASE_UART4                                  0x04544000
#define REG_BASE_CUR_UART                               REG_BASE_UART0


#define PMC_BASE                    0x045a0000

/*********************************************************************/
/*
 * 0x1-> init item1
 * 0x2-> init item2
 * 0x3->init item1 & item2
 */
#define INIT_REG_ITEM1               1
#define INIT_REG_ITEM2               2
#define INIT_REG_ITEM1_ITEM2         (INIT_REG_ITEM1 | INIT_REG_ITEM2)

/*-----------------------------------------------------------------------
 * PERI_CRG29 FMC REG
 * ----------------------------------------------------------------------*/
#define CRG48_FMC               0xc0
#define CRG48_FMC_CLK_SEL(_clk)         (((_clk) & 0x7) << 2)
#define CRG48_FMC_CLK_EN            (1 << 1)
#define CRG48_FMC_SOFT_RST_REQ          (1 << 0)

#define FMC_CLK_SEL_MASK            (0x7 << 2)

/* SDR/DDR clock */
#define CLK_24M                 0x00
#define CLK_75M                 0x01
#define CLK_125M                0x02
#define CLK_150M                0x03
#define CLK_200M                0x04

/* Only DDR clock */
#define CLK_250M                0x05
#define CLK_300M                0x06
#define CLK_400M                0x07

#define FMC_CLK_SEL_24M             CRG48_FMC_CLK_SEL(CLK_24M)
#define FMC_CLK_SEL_75M             CRG48_FMC_CLK_SEL(CLK_75M)
#define FMC_CLK_SEL_125M            CRG48_FMC_CLK_SEL(CLK_125M)
#define FMC_CLK_SEL_150M            CRG48_FMC_CLK_SEL(CLK_150M)
#define FMC_CLK_SEL_200M            CRG48_FMC_CLK_SEL(CLK_200M)

#define FMC_CLK_SEL_250M            CRG48_FMC_CLK_SEL(CLK_250M)
#define FMC_CLK_SEL_300M            CRG48_FMC_CLK_SEL(CLK_300M)
#define FMC_CLK_SEL_400M            CRG48_FMC_CLK_SEL(CLK_400M)

#endif /* End of __HI_CHIP_REGS_H__ */

