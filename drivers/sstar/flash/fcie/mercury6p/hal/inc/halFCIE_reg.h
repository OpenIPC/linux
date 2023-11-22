/*
 * halFCIE_reg.h- Sigmastar
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

#ifndef _HAL_FCIE_REG_H_
#define _HAL_FCIE_REG_H_

#ifndef BIT0
#define BIT0 (1 << 0)
#endif
#ifndef BIT1
#define BIT1 (1 << 1)
#endif
#ifndef BIT2
#define BIT2 (1 << 2)
#endif
#ifndef BIT3
#define BIT3 (1 << 3)
#endif
#ifndef BIT4
#define BIT4 (1 << 4)
#endif
#ifndef BIT5
#define BIT5 (1 << 5)
#endif
#ifndef BIT6
#define BIT6 (1 << 6)
#endif
#ifndef BIT7
#define BIT7 (1 << 7)
#endif
#ifndef BIT8
#define BIT8 (1 << 8)
#endif
#ifndef BIT9
#define BIT9 (1 << 9)
#endif
#ifndef BIT10
#define BIT10 (1 << 10)
#endif
#ifndef BIT11
#define BIT11 (1 << 11)
#endif
#ifndef BIT12
#define BIT12 (1 << 12)
#endif
#ifndef BIT13
#define BIT13 (1 << 13)
#endif
#ifndef BIT14
#define BIT14 (1 << 14)
#endif
#ifndef BIT15
#define BIT15 (1 << 15)
#endif

#define ECC_BCH_512B_8_BITS  0x00
#define ECC_BCH_512B_16_BITS 0x02
#define ECC_BCH_512B_24_BITS 0x04
#define ECC_BCH_512B_32_BITS 0x06
#define ECC_BCH_512B_40_BITS 0x08

#define ECC_BCH_1024B_8_BITS  0x01
#define ECC_BCH_1024B_16_BITS 0x03
#define ECC_BCH_1024B_24_BITS 0x05
#define ECC_BCH_1024B_32_BITS 0x07
#define ECC_BCH_1024B_40_BITS 0x09

#define NC_CIFD_EVENT     0x30
#define NC_CIFD_INT_EN    0x31
#define NC_PWR_RD_MASK    0x34
#define NC_PWR_SAVE_CTL   0x35
#define NC_BIST_MODE      0x36
#define NC_BOOT_MODE      0x37
#define BIT_IMI_SEL       BIT2
#define NC_DEBUG_DBUS0    0x38
#define NC_DEBUG_DBUS1    0x39
#define NC_CLK_EN         0x3E
#define NC_FCIE_RST       0x3F
#define BIT_FCIE_SOFT_RST BIT0
#define BIT_RST_MIU_STS   BIT1
#define BIT_RST_MIE_STS   BIT2
#define BIT_RST_MCU_STS   BIT3
#define BIT_RST_ECC_STS   BIT4
#define BIT_RST_STS_MASK  (BIT_RST_MIE_STS | BIT_RST_MCU_STS | BIT_RST_ECC_STS)

#define NC_SIGNAL     0x40
#define NC_WIDTH      0x41
#define NC_STAT_CHK   0x42
#define NC_AUXREG_ADR 0x43
/* AUX Reg Address */
#define AUXADR_CMDREG8  0x08
#define AUXADR_CMDREG9  0x09
#define AUXADR_CMDREGA  0x0A
#define AUXADR_ADRSET   0x0B
#define AUXADR_RPTCNT   0x18 // Pepeat Count
#define AUXADR_RAN_CNT  0x19
#define AUXADR_RAN_POS  0x1A // offset
#define AUXADR_ST_CHECK 0x1B
#define AUXADR_IDLE_CNT 0x1C
#define AUXADR_INSTQUE  0x20

#define NC_AUXREG_DAT 0x44
/* OP Code: Action */
#define ACT_WAITRB    0x80
#define ACT_CHKSTATUS 0x81
#define ACT_WAIT_IDLE 0x82
#define ACT_WAIT_MMA  0x83
#define ACT_BREAK     0x88
#define ACT_SER_DOUT  0x90 /* for column addr == 0 */
#define ACT_RAN_DOUT  0x91 /* for column addr != 0 */
//#define ACT_WR_REDU       0x92
//#define ACT_LUT_DWLOAD    0x93
//#define ACT_LUT_DWLOAD1   0x94
#define ACT_SER_DIN 0x98 /* for column addr == 0 */
#define ACT_RAN_DIN 0x99 /* for column addr != 0 */
//#define ACT_RD_REDU       0x9A
//#define ACT_LUT_UPLOAD    0x9B
#define ACT_PAGECOPY_US         0xA0
#define ACT_PAGECOPY_DS         0xA1
#define ACT_REPEAT              0xB0
#define NC_CTRL                 0x45
#define BIT_NC_JOB_START        BIT0
#define BIT_NC_ADMA_EN          BIT1
#define BIT_NC_ZDEC_EN          BIT3
#define BIT_NC_NF2ZDEC_PTR_CLR  BIT4
#define NC_ST_READ              0x46
#define NC_PART_MODE            0x47
#define BIT_PARTIAL_MODE_EN     BIT0
#define BITS_SECTOR_COUNT_MASK  (BIT7 - 2)
#define NC_SPARE                0x48
#define BIT_SPARE_DEST          BIT8
#define BIT_SPARE_ECC_BYPASS    BIT10
#define NC_SPARE_SIZE           0x49
#define NC_ADDEND               0x4A
#define NC_ZERO_CNT             0x4B
#define NC_ZERO_CNT_SCRAMBLE    0x4C /*before scramble*/
#define NC_MIU_CTRL             0x4D
#define NC_MIU_DMA_SEL          0x4D
#define NC_ECC_CTRL             0x50
#define BIT_BYPASS_ECC(x)       (x << 10)
#define BIT_ERROR_NONE_STOP(x)  (x << 7)
#define NC_ECC_STAT0            0x51
#define BIT_ECC_FLAG            BIT0
#define BITS_ECC_CORRECT        (BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define NC_ECC_STAT1            0x52
#define NC_ECC_STAT2            0x53
#define BITS_ECC_MASK           (BIT0 | BIT1)
#define BIT_ECC_UNCORRECTABLE   BIT0
#define NC_ECC_LOC              0x54
#define NC_RAND_R_CMD           0x55
#define NC_RAND_W_CMD           0x56
#define NC_LATCH_DATA           0x57
#define NC_DDR_CTRL             0x58
#define BIT_NC_32B_MODE         BIT2
#define NC_LFSR_CTRL            0x59
#define NC_NAND_TIMING          0x5A
#define NC_SER_DIN_BYTECNT_LW   0x5B
#define NC_SER_DIN_BYTECNT_HW   0x5C
#define NC_NAND_TIMING1         0x5D
#define NC_RX5E                 0x5E
#define NC_MIE_EVENT            0x60
#define BIT_NC_JOB_END          BIT0
#define BIT_NC_R2N_ECCCOR       BIT1
#define BIT_NC_SECURE_ALERT     BIT2
#define BIT_NC_JOB_ABORT        BIT3
#define NC_MIE_INT_EN           0x61
#define NC_FORCE_INT            0x62
#define BIT_F_NC_JOB_INT        BIT0
#define BIT_F_NC_R2N_ECCCOR_INT BIT1
#define BIT_NC_SECURE_ALERT_INT BIT2
#define BIT_NC_JOB_ABORT_INT    BIT3
#define NC_FUN_CTL              0x63
#define BIT_NC_R2N_MODE         BIT1
#define BIT_NC_SPI_MODE         BIT4
#define NC_RDATA_DMA_ADR0       0x64
#define NC_RDATA_DMA_ADR1       0x65
#define NC_WDATA_DMA_ADR0       0x66
#define NC_WDATA_DMA_ADR1       0x67
#define NC_RSPARE_DMA_ADR0      0x68
#define NC_RSPARE_DMA_ADR1      0x69
#define NC_WSPARE_DMA_ADR0      0x6A
#define NC_WSPARE_DMA_ADR1      0x6B
#define NC_DATA_STS             0x6C
#define NC_RBZ_STS              0x6D
#define NC_SSO_CTL              0x6E
#define NC_SSO_DATA             0x6F
#define NC_REORDER              0x70
#define NC_RPT_CNT              0x71
#define NC_TR_RPT_CNT           0x72
#define NC_1ST_ECC_ERR_PAGE     0x73
#define NC_REMAIN_RPT_CNT       0x74
#define NC_DATA_BASE_ADDR_MSB   0x76
#define NC_SPARE_BASE_ADDR_MSB  0x77
#define NC_ECC_DATA_ADDR_L      0x78
#define NC_ECC_DATA_ADDR_H      0x79
#define NC_ECC_SPARE_ADDR_L     0x7A
#define NC_ECC_SPARE_ADDR_H     0x7B
#define NC_ECC_MODE_SEL         0x7C
#define BIT_DATA_MODE           BIT0
#define BIT_AXI_LENGTH          (BIT4 | BIT5)
#define NC_ECC_SPARE_SIZE       0x7D
#define BIT_SPARE_SIZE          0xFF
#define BIT_ADDR_LATCH          BIT8
#define BIT_ECC_DIR             BIT9
#define BIT_WRITE_MIU_BUSY      BIT10

// SPI2FCIE Register
#define REG_SPI2FCIE_EN            0x00
#define BIT_SPI2FCIE_IF_EN         BIT0
#define BIT_FCIE2SPI_IF_EN         BIT1
#define BIT_BURST_ENHANCE_MODE(x)  (x << 8)
#define REG_DUMMY0                 0x30
#define REG_SPI_OFFSET_ADDR_L      0x05
#define REG_SPI_OFFSET_ADDR_H      0x06
#define REG_SPI_BOUNDARY_MODE      0x10
#define BIT_PRE_SET                BIT0
#define BIT_AUTO_REFRESH           BIT1
#define REG_SPI_BOUNDARY_pre_set_L 0x11
#define REG_SPI_BOUNDARY_pre_set_H 0x12
#define REG_SPI_BOUNDARY_SIZE      0x13
#define REG_SPI_BOUNDARY_SET_TRIG  0x18
#define REG_SPI_BOUNDARY_RELOAD    0x19
#define REG_DEBUG_INFO_01          0x21
#define REG_DEBUG_INFO_02          0x22
#define BIT_SPI_BOUNDARY_HIT       BIT8
#define REG_TX_BLOCK_ENABLE        0x31
#define BIT_BLOCK_ENABLE           BIT0
#define REG_TX_BLOCK_CLEAR         0x32
#define BIT_BLOCK_CLEAR            BIT0

#ifndef RIU_BASE_ADDR
#define RIU_BASE_ADDR IO_ADDRESS(0x1F000000)
#endif
#define SPI2FCIE_BASE_ADDR 0x100a00
//#define NFIE_REG_BASE_ADDR          0x140200
#define NFIE_REG_BASE_ADDR  0x141000
#define FCIE3_REG_BASE_ADDR 0x141300
#define CLKGEN_BASE_ADDR    0x103800
#define FCIE_BANK_ADDR      (RIU_BASE_ADDR + (NFIE_REG_BASE_ADDR << 1))
#define FCIE3_BANK_ADDR     (RIU_BASE_ADDR + (FCIE3_REG_BASE_ADDR << 1))
#define SPI2FCIE_BANK_ADDR  (RIU_BASE_ADDR + (SPI2FCIE_BASE_ADDR << 1))

#endif /* _HAL_FCIE_REG_H_ */
