/*
 * eMMC_reg_v5.h- Sigmastar
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

#ifndef __EMMC_REG_V5_H__
#define __EMMC_REG_V5_H__

#include "eMMC_config.h"

#define IP_FCIE_VERSION_5

#ifndef NULL
#define NULL ((void*)0)
#endif
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
#ifndef BIT16
#define BIT16 (1 << 16)
#endif
#ifndef BIT17
#define BIT17 (1 << 17)
#endif
#ifndef BIT18
#define BIT18 (1 << 18)
#endif
#ifndef BIT19
#define BIT19 (1 << 19)
#endif
#ifndef BIT20
#define BIT20 (1 << 20)
#endif
#ifndef BIT21
#define BIT21 (1 << 21)
#endif
#ifndef BIT22
#define BIT22 (1 << 22)
#endif
#ifndef BIT23
#define BIT23 (1 << 23)
#endif
#ifndef BIT24
#define BIT24 (1 << 24)
#endif
#ifndef BIT25
#define BIT25 (1 << 25)
#endif
#ifndef BIT26
#define BIT26 (1 << 26)
#endif
#ifndef BIT27
#define BIT27 (1 << 27)
#endif
#ifndef BIT28
#define BIT28 (1 << 28)
#endif
#ifndef BIT29
#define BIT29 (1 << 29)
#endif
#ifndef BIT30
#define BIT30 (1 << 30)
#endif
#ifndef BIT31
#define BIT31 (1 << 31)
#endif

//------------------------------------------------------------------
#define FCIE_REG_BASE_ADDR(ip)         EMMC_GET_REG_BANK(ip, 0)
#define FCIE_REG_BASE_2_ADDR(ip)       EMMC_GET_REG_BANK(ip, 2)
#define FCIE_CMDFIFO_BASE_ADDR(ip)     GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x20)
#define FCIE_POWEER_SAVE_MODE_BASE(ip) GET_REG_ADDR(FCIE_REG_BASE_2_ADDR(ip), 0x00)

#define FCIE_CMDFIFO_ADDR(ip, u16_pos) GET_REG_ADDR(FCIE_CMDFIFO_BASE_ADDR(ip), u16_pos)
#define FCIE_CMDFIFO_BYTE_CNT          0x12 // 9 x 16 bits

#define FCIE_CIFD_BASE_ADDR(ip)     EMMC_GET_REG_BANK(ip, 1)
#define FCIE_CIFD_ADDR(ip, u16_pos) GET_REG_ADDR(FCIE_CIFD_BASE_ADDR(ip), u16_pos)

#define FCIE_NC_WBUF_CIFD_BASE(ip) GET_REG_ADDR(FCIE_CIFD_BASE_ADDR(ip), 0x00)
#define FCIE_NC_RBUF_CIFD_BASE(ip) GET_REG_ADDR(FCIE_CIFD_BASE_ADDR(ip), 0x20)

#define NC_WBUF_CIFD_ADDR(ip, u16_pos) GET_REG_ADDR(FCIE_NC_WBUF_CIFD_BASE(ip), u16_pos) // 32 x 16 bits SW Read only
#define NC_RBUF_CIFD_ADDR(ip, u16_pos) GET_REG_ADDR(FCIE_NC_RBUF_CIFD_BASE(ip), u16_pos) // 32 x 16 bits   SW write/read

#define FCIE_MIE_EVENT(ip)          GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x00)
#define FCIE_MIE_INT_EN(ip)         GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x01)
#define FCIE_MMA_PRI_REG(ip)        GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x02)
#define FCIE_MIU_DMA_ADDR_15_0(ip)  GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x03)
#define FCIE_MIU_DMA_ADDR_31_16(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x04)
#define FCIE_MIU_DMA_LEN_15_0(ip)   GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x05)
#define FCIE_MIU_DMA_LEN_31_16(ip)  GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x06)
#define FCIE_MIE_FUNC_CTL(ip)       GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x07)
#define FCIE_JOB_BL_CNT(ip)         GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x08)
#define FCIE_BLK_SIZE(ip)           GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x09)
#define FCIE_CMD_RSP_SIZE(ip)       GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x0A)
#define FCIE_SD_MODE(ip)            GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x0B)
#define FCIE_SD_CTRL(ip)            GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x0C)
#define FCIE_SD_STATUS(ip)          GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x0D)
#define FCIE_BOOT_CONFIG(ip)        GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x0E)
#define FCIE_DDR_MODE(ip)           GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x0F)
//#define FCIE_DDR_TOGGLE_CNT(ip)             GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x10)
#define FCIE_RESERVED_FOR_SW(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x10)
#define FCIE_SDIO_MOD(ip)        GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x11)
//#define FCIE_SBIT_TIMER(ip)               GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x11)
#define FCIE_RSP_SHIFT_CNT(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x12)
#define FCIE_RX_SHIFT_CNT(ip)  GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x13)
#define FCIE_ZDEC_CTL0(ip)     GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x14)
#define FCIE_TEST_MODE(ip)     GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x15)
#define FCIE_MMA_BANK_SIZE(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x16)
#define FCIE_WR_SBIT_TIMER(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x17)
//#define FCIE_SDIO_MODE(ip)                GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x17)
#define FCIE_RD_SBIT_TIMER(ip)      GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x18)
#define FCIE_MIU_DMA_ADDR_35_32(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x19)
//#define FCIE_DEBUG_BUS0(ip)               GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x1E)
//#define FCIE_DEBUG_BUS1(ip)               GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x1F)
#define NC_CIFD_EVENT(ip)        GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x30)
#define NC_CIFD_INT_EN(ip)       GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x31)
#define FCIE_PWR_RD_MASK(ip)     GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x34)
#define FCIE_PWR_SAVE_CTL(ip)    GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x35)
#define FCIE_BIST(ip)            GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x36)
#define FCIE_BOOT(ip)            GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x37)
#define FCIE_EMMC_DEBUG_BUS0(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x38)
#define FCIE_EMMC_DEBUG_BUS1(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x39)
#define FCIE_RST(ip)             GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x3F)
//#define NC_WIDTH(ip)                      GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x41)
#define FCIE_NC_FUN_CTL(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x63)

//--------------------------------emmc pll--------------------------------------
#define EMMC_PLL_BASE(ip)      EMMC_GET_PLL_REG(ip)
#define REG_EMMC_PLL_RX01(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x01)
#define reg_emmcpll_0x02(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x02)
#define reg_emmcpll_0x03(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x03)
#define reg_emmcpll_fbdiv(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x04)
#define reg_emmcpll_pdiv(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x05)
#define reg_emmc_pll_reset(ip) GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x06)
#define reg_emmc_pll_test(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x07)
#define reg_emmcpll_0x09(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x09)
#define reg_emmcpll_0x16(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x16)
#define reg_emmcpll_0x17(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x17)
#define reg_ddfset_15_00(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x18)
#define reg_ddfset_23_16(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x19)
#define reg_emmc_test(ip)      GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x1a)
#define reg_emmcpll_0x1c(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x1c)
#define reg_emmcpll_0x1d(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x1d)
#define reg_emmcpll_0x1e(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x1e)
#define reg_emmcpll_0x1f(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x1f)
#define reg_emmcpll_0x20(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x20)
#define REG_EMMC_PLL_RX30(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x30)
#define REG_EMMC_PLL_RX32(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x32)
#define REG_EMMC_PLL_RX33(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x33)
#define REG_EMMC_PLL_RX34(ip)  GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x34)
#define reg_emmcpll_0x63(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x63)
#define reg_emmcpll_0x68(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x68)
#define reg_emmcpll_0x69(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x69)
#define reg_emmcpll_0x6a(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x6a)
#define reg_emmcpll_0x6b(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x6b)
#define reg_emmcpll_0x6c(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x6c)
#define reg_emmcpll_0x6d(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x6d)
#define reg_emmcpll_0x6f(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x6f)
#define reg_emmcpll_0x70(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x70)
#define reg_emmcpll_0x71(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x71)
#define reg_emmcpll_0x73(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x73)
#define reg_emmcpll_0x74(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x74)
#define reg_emmcpll_0x7f(ip)   GET_REG_ADDR(EMMC_PLL_BASE(ip), 0x7f)

#define BIT_SKEW1_MASK    (BIT3 | BIT2 | BIT1 | BIT0)
#define BIT_SKEW2_MASK    (BIT7 | BIT6 | BIT5 | BIT4)
#define BIT_SKEW3_MASK    (BIT11 | BIT10 | BIT9 | BIT8)
#define BIT_SKEW4_MASK    (BIT15 | BIT14 | BIT13 | BIT12)
#define BIT_DEFAULT_SKEW2 (BIT6 | BIT4) // 5

#define BIT_DQS_DELAY_CELL_MASK  (BIT4 | BIT5 | BIT6 | BIT7)
#define BIT_DQS_DELAY_CELL_SHIFT 4
#define BIT_DQS_MODE_MASK        (BIT0 | BIT1 | BIT2)
#define BIT_DQS_MDOE_SHIFT       0

#define BIT_DQS_DELAY_CELL_MASK  (BIT4 | BIT5 | BIT6 | BIT7)
#define BIT_DQS_DELAY_CELL_SHIFT 4
#define BIT_DQS_MODE_MASK        (BIT0 | BIT1 | BIT2)
#define BIT_DQS_MODE_SHIFT       0
#define BIT_DQS_MODE_0T          (0 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_0_5T        (1 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_1T          (2 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_1_5T        (3 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_2T          (4 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_2_5T        (5 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_3T          (6 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_3_5T        (7 << BIT_DQS_MODE_SHIFT)

#define R_TUNE_SHOT_OFFSET_MASK   (BIT7 | BIT6 | BIT5 | BIT4)
#define V_PLL_SKEW_SUM_INIT       ((4 << 4) & R_TUNE_SHOT_OFFSET_MASK)
#define V_PLL_RX_BPS_EN_INIT      (0x3F)
#define V_PLL_ATOP_BYP_RX_EN_INIT (BIT15)

//============================================
// eMMC PLL: EMMC_TEST: offset 0x1A
//============================================
#define R_PLL_PAD_DRV         BIT0
#define R_PLL_G_RX_W_OEN_DOUT BIT3
#define R_PLL_G_RX_W_OEN_COUT BIT4
#define R_PLL_C2_EN           BIT10

//============================================
// eMMC PLL: ECO_EN: offset 0x1C
//============================================
#define R_PLL_32BIF_RX_ECO_EN BIT8
#define R_PLL_1X_SYN_ECO_EN   BIT9

//============================================
// eMMC PLL: EMMC_EN: offset 0x68
//============================================
#define R_PLL_EMMC_EN BIT0

//============================================
// eMMC PLL: SKEW_SUM: offset 0x69
//============================================
#define R_PLL_CLK_DIG_INV       BIT3
#define R_TUNE_SHOT_OFFSET_MASK (BIT7 | BIT6 | BIT5 | BIT4)
#define R_CLK_SKEW_INV          eMMC_SKEW_INV

//============================================
// eMMC PLL: IO_BUS_WID: offset 0x6A
//============================================
#define R_PLL_IO_BUS_WID_MASK (BIT1 | BIT0)

//============================================
// eMMC PLL: DDR_IO_MODE: offset 0x6D
//============================================
#define R_PLL_DDR_IO_MODE BIT0

//============================================
// eMMC PLL: RST_SUM: offset 0x6F
//============================================
#define R_PLL_MACRO_SW_RSTZ BIT0
#define R_PLL_DQS_CNT_RSTN  BIT1
#define R_PLL_OSP_SW_RSTZ   BIT2

//============================================
// eMMC PLL: AFIFO_SUM: offset 0x70
//============================================
#define R_PLL_SEL_FLASH_32BIF BIT8
#define R_PLL_RX_AFIFO_EN     BIT10
#define R_PLL_RSP_AFIFO_EN    BIT11

//============================================
// eMMC PLL: TX_BPS_EN: offset 0x71
//============================================
#define R_PLL_TX_BPS_EN_MASK (BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0)

//============================================
// eMMC PLL: RX_BPS_EN: offset 0x73
//============================================
#define R_PLL_RX_BPS_EN_MASK (BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0)

//============================================
// eMMC PLL: ATOP_BYP_RX_EN: offset 0x74
//============================================
#define R_PLL_ATOP_BYP_RX_EN BIT15

//--------------------------------clock gen------------------------------------
#define eMMC_PLL_FLAG           0x80
#define eMMC_PLL_CLK__20M       (0x01 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__27M       (0x02 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__32M       (0x03 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__36M       (0x04 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__40M       (0x05 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__48M       (0x06 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__52M       (0x07 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__62M       (0x08 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__72M       (0x09 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__80M       (0x0A | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK__86M       (0x0B | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK_100M       (0x0C | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK_120M       (0x0D | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK_140M       (0x0E | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK_160M       (0x0F | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK_200M       (0x10 | eMMC_PLL_FLAG)
#define eMMC_PLL_CLK_200M_HS400 (0x11 | eMMC_PLL_FLAG)

#define FCIE_PATH_CTRL       FCIE_MIE_FUNC_CTL
#define FCIE_SDIO_ADDR0(ip)  GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x1C)
#define FCIE_SDIO_ADDR1(ip)  GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x1D)
#define FCIE_MACRO_REDNT(ip) GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x32)
#define FCIE_TOGGLE_CNT(ip)  GET_REG_ADDR(FCIE_REG_BASE_ADDR(ip), 0x33)

#define BIT_MACRO_DIR      BIT6
#define BIT_DMA_DIR_W      BIT2
#define BIT_MMA_EN         BIT0
#define BIT_SD_EN          BIT1
#define BIT_CARD_DMA_END   BIT0
#define BIT_TOGGLE_CNT_RST BIT7
#define BIT_MIU_LAST_DONE  BIT14
#define eMMC_HW_TIMER_HZ   12000000

#define TOGGLE_CNT_128_CLK_R 0x0091 // 128 data clcok + 17 bits CRC
#define TOGGLE_CNT_256_CLK_R 0x0111 // 256 data clcok + 17 bits CRC
#define TOGGLE_CNT_512_CLK_R 0x0211 // 512 data clcok + 17 bits CRC
#define TOGGLE_CNT_256_CLK_W 0x011A // 256 data clcok + 26 bits CRC + CRC status
#define TOGGLE_CNT_512_CLK_W 0x021A // 512 data clcok + 26 bits CRC + CRC status

#define NC_CIFD_WBUF_BYTE_CNT 0x40 // 32 x 16 bits
#define NC_CIFD_RBUF_BYTE_CNT 0x40 // 32 x 16 bits

#define FCIE_CIFD_BYTE_CNT 0x40 // 256 x 16 bits

//------------------------------------------------------------------
/* FCIE_MIE_EVENT  0x00 */
/* FCIE_MIE_INT_EN 0x01 */
#define BIT_DMA_END    BIT0
#define BIT_SD_CMD_END BIT1
#define BIT_ERR_STS    BIT2
//#define BIT_SD_DATA_END               BIT2
#define BIT_SDIO_INT     BIT3
#define BIT_BUSY_END_INT BIT4
#define BIT_R2N_RDY_INT  BIT5
#define BIT_CARD_CHANGE  BIT6
#define BIT_CARD2_CHANGE BIT7
//#define BIT_PWR_SAVE_INT              BIT14
//#define BIT_BOOT_DONE_INT             BIT15
//#define BIT_ALL_CARD_INT_EVENTS         (BIT_DMA_END|BIT_SD_CMD_END|BIT_BUSY_END_INT)
#define BIT_ALL_CARD_INT_EVENTS (BIT_DMA_END | BIT_SD_CMD_END | BIT_ERR_STS | BIT_BUSY_END_INT | BIT_R2N_RDY_INT)

/* FCIE_MMA_PRI_REG 0x02 */
#define BIT_MIU_R_PRI       BIT0
#define BIT_MIU_W_PRI       BIT1
#define BIT_MIU_SELECT_MASK (BIT3 | BIT2)
#define BIT_MIU1_SELECT     BIT2
#define BIT_MIU2_SELECT     BIT3
#define BIT_MIU3_SELECT     (BIT3 | BIT2)
//#define BIT_DATA_SCRAMBLE             BIT3
#define BIT_MIU_BUS_TYPE_MASK (BIT4 | BIT5)
#define BIT_MIU_BURST1        (~BIT_MIU_BUS_TYPE_MASK)
#define BIT_MIU_BURST2        (BIT4)
#define BIT_MIU_BURST4        (BIT5)
#define BIT_MIU_BURST8        (BIT4 | BIT5)

/* FCIE_MIE_FUNC_CTL 0x07 */
#define BIT_EMMC_EN                BIT0
#define BIT_SD_EN                  BIT1
#define BIT_SDIO_MOD               BIT2
#define BIT_EMMC_ACTIVE            BIT12
#define BIT_KERN_NAND              (BIT_KERN_CHK_NAND_EMMC | BIT13)
#define BIT_KERN_EMMC              (BIT_KERN_CHK_NAND_EMMC | BIT14)
#define BIT_KERN_CHK_NAND_EMMC     BIT15
#define BIT_KERN_CHK_NAND_EMMC_MSK (BIT13 | BIT14 | BIT15)

/* FCIE_BLK_CNT 0x08 */
#define BIT_SD_JOB_BLK_CNT_MASK (BIT13 - 1)

/* FCIE_CMD_RSP_SIZE 0x0A */
#define BIT_RSP_SIZE_MASK  (BIT6 - 1)
#define BIT_CMD_SIZE_MASK  (BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8)
#define BIT_CMD_SIZE_SHIFT 8

/* FCIE_SD_MODE 0x0B */
#define BIT_CLK_EN             BIT0
#define BIT_SD_CLK_EN          BIT_CLK_EN
#define BIT_SD_DATA_WIDTH_MASK (BIT2 | BIT1)
#define BIT_SD_DATA_WIDTH_1    0
#define BIT_SD_DATA_WIDTH_4    BIT1
#define BIT_SD_DATA_WIDTH_8    BIT2
#define BIT_DATA_DEST          BIT4 // 0: DMA mode, 1: R2N mode
#define BIT_SD_DATA_CIFD       BIT_DATA_DEST
#define BIT_DATA_SYNC          BIT5
#define BIT_SD_DMA_R_CLK_STOP  BIT7
#define BIT_DIS_WR_BUSY_CHK    BIT8

#define BIT_SD_DEFAULT_MODE_REG (BIT_CLK_EN)

/* FCIE_SD_CTRL 0x0C */
#define BIT_SD_RSPR2_EN  BIT0
#define BIT_SD_RSP_EN    BIT1
#define BIT_SD_CMD_EN    BIT2
#define BIT_SD_DTRX_EN   BIT3
#define BIT_SD_DAT_EN    BIT_SD_DTRX_EN
#define BIT_SD_DAT_DIR_W BIT4
#define BIT_ADMA_EN      BIT5
#define BIT_JOB_START    BIT6
#define BIT_CHK_CMD      BIT7
#define BIT_BUSY_DET_ON  BIT8
#define BIT_ERR_DET_ON   BIT9

/* FCIE_SD_STATUS 0x0D */
#define BIT_DAT_RD_CERR    BIT0
#define BIT_SD_R_CRC_ERR   BIT_DAT_RD_CERR
#define BIT_DAT_WR_CERR    BIT1
#define BIT_SD_W_FAIL      BIT_DAT_WR_CERR
#define BIT_DAT_WR_TOUT    BIT2
#define BIT_SD_W_CRC_ERR   BIT_DAT_WR_TOUT
#define BIT_CMD_NO_RSP     BIT3
#define BIT_SD_RSP_TIMEOUT BIT_CMD_NO_RSP
#define BIT_CMD_RSP_CERR   BIT4
#define BIT_SD_RSP_CRC_ERR BIT_CMD_RSP_CERR
#define BIT_DAT_RD_TOUT    BIT5
//#define BIT_SD_CARD_WP                BIT5
#define BIT_SD_CARD_BUSY BIT6

#define BITS_ERROR \
    (BIT_SD_R_CRC_ERR | BIT_DAT_WR_CERR | BIT_DAT_WR_TOUT | BIT_CMD_NO_RSP | BIT_CMD_RSP_CERR | BIT_DAT_RD_TOUT)

#define BIT_SD_D0 BIT8 // not correct D0 in 32 bits macron
//#define BIT_SD_DBUS_MASK              (BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)
//#define BIT_SD_DBUS_SHIFT             8
#define BIT_SD_FCIE_ERR_FLAGS (BIT6 - 1)
#define BIT_SD_CARD_D0_ST     BIT8
#define BIT_SD_CARD_D1_ST     BIT9
#define BIT_SD_CARD_D2_ST     BIT10
#define BIT_SD_CARD_D3_ST     BIT11
#define BIT_SD_CARD_D4_ST     BIT12
#define BIT_SD_CARD_D5_ST     BIT13
#define BIT_SD_CARD_D6_ST     BIT14
#define BIT_SD_CARD_D7_ST     BIT15

/* FCIE_BOOT_CONFIG 0x0E */
#define BIT_EMMC_RSTZ    BIT0
#define BIT_EMMC_RSTZ_EN BIT1
#define BIT_BOOT_MODE_EN BIT2
//#define BIT_BOOT_END                  BIT3

/* FCIE_DDR_MODE 0x0F */
//#define BIT_DQS_DELAY_CELL_MASK       (BIT0|BIT1|BIT2|BIT3)
//#define BIT_DQS_DELAY_CELL_SHIFT      0
#define BIT_BYPASS_MODE_MASK (BIT0)
#define BIT_CIFD_MODE_MASK   (BIT1 | BIT2 | BIT3)
#define BIT_MACRO_MODE_MASK  (BIT7 | BIT8 | BIT10 | BIT12 | BIT13 | BIT14 | BIT15)
#define BIT_8BIT_MACRO_EN    BIT7
#define BIT_DDR_EN           BIT8
//#define BIT_SDR200_EN                 BIT9
#define BIT_CLK2_SEL BIT10
//#define BIT_SDRIN_BYPASS_EN           BIT11
#define BIT_32BIT_MACRO_EN BIT12
#define BIT_PAD_IN_SEL_SD  BIT13
#define BIT_FALL_LATCH     BIT14
#define BIT_PAD_IN_MASK    BIT15

/* FCIE_TOGGLE_CNT 0x10 */
#define BITS_8_MACRO32_DDR52_TOGGLE_CNT 0x110
#define BITS_4_MACRO32_DDR52_TOGGLE_CNT 0x210

#define BITS_8_MACRO32_HS200_TOGGLE_CNT 0x210
#define BITS_4_MACRO32_HS200_TOGGLE_CNT 0x410

/* FCIE_SDIO_MOD 0x11 */
#define BIT_REG_SDIO_MOD_MASK (BIT1 | BIT0)
#define BIT_SDIO_DET_ON       BIT2
#define BIT_SDIO_DET_INT_SRC  BIT3

/* FCIE_RSP_SHIFT_CNT 0x12 */
#define BIT_RSP_SHIFT_TUNE_MASK (BIT4 - 1)
#define BIT_RSP_SHIFT_SEL       BIT4 /*SW or HW by default  0*/

/* FCIE_RX_SHIFT_CNT 0x13 */
#define BIT_RSTOP_SHIFT_TUNE_MASK (BIT4 - 1)
#define BIT_RSTOP_SHIFT_SEL       BIT4
#define BIT_WRSTS_SHIFT_TUNE_MASK (BIT8 | BIT9 | BIT10 | BIT11)
#define BIT_WRSTS_SHIFT_SEL       BIT12

/* FCIE_ZDEC_CTL0 0x14 */
#define BIT_ZDEC_EN         BIT0
#define BIT_SD2ZDEC_PTR_CLR BIT1

/* FCIE_TEST_MODE 0x15 */
#define BIT_SDDR1            BIT0
#define BIT_DEBUG_MODE_MASK  (BIT3 | BIT2 | BIT1)
#define BIT_DEBUG_MODE_SHIFT 1
#define BIT_BIST_MODE        BIT4
//#define BIT_DS_TESTEN                 BIT1
//#define BIT_TEST_MODE                 BIT2
//#define BIT_DEBUG_MODE_MASK           BIT3|BIT4|BIT5
//#define BIT_DEBUG_MODE_SHIFT          3
//#define BIT_TEST_MIU                  BIT6
//#define BIT_TEST_MIE                  BIT7
//#define BIT_TEST_MIU_STS              BIT8
//#define BIT_TEST_MIE_STS              BIT9
//#define BIT_BIST_MODE                 BIT10

/* FCIE_WR_SBIT_TIMER 0x17 */
#define BIT_WR_SBIT_TIMER_MASK (BIT15 - 1)
#define BIT_WR_SBIT_TIMER_EN   BIT15

/* FCIE_RD_SBIT_TIMER 0x18 */
#define BIT_RD_SBIT_TIMER_MASK (BIT15 - 1)
#define BIT_RD_SBIT_TIMER_EN   BIT15

/* NC_CIFD_EVENT 0x30 */
#define BIT_WBUF_FULL      BIT0
#define BIT_WBUF_EMPTY_TRI BIT1
#define BIT_RBUF_FULL_TRI  BIT2
#define BIT_RBUF_EMPTY     BIT3

/* NC_CIFD_INT_EN 0x31 */
#define BIT_WBUF_FULL_INT_EN  BIT0
#define BIT_RBUF_EMPTY_INT_EN BIT1
#define BIT_F_WBUF_FULL_INT   BIT2
#define BIT_F_RBUF_EMPTY_INT  BIT3

/* FCIE_PWR_SAVE_CTL 0x35 */
#define BIT_POWER_SAVE_MODE        BIT0
#define BIT_SD_POWER_SAVE_RIU      BIT1
#define BIT_POWER_SAVE_MODE_INT_EN BIT2
#define BIT_SD_POWER_SAVE_RST      BIT3
#define BIT_POWER_SAVE_INT_FORCE   BIT4
#define BIT_RIU_SAVE_EVENT         BIT5
#define BIT_RST_SAVE_EVENT         BIT6
#define BIT_BAT_SAVE_EVENT         BIT7
#define BIT_BAT_SD_POWER_SAVE_MASK BIT8
#define BIT_RST_SD_POWER_SAVE_MASK BIT9
#define BIT_POWER_SAVE_MODE_INT    BIT15

/* FCIE_BOOT 0x37 */
#define BIT_NAND_BOOT_EN        BIT0
#define BIT_BOOTSRAM_ACCESS_SEL BIT1

/* FCIE_BOOT 0x39 */
#define BIT_DEBUG_MODE_MSK (BIT11 | BIT10 | BIT9 | BIT8)
#define BIT_DEBUG_MODE_SET (BIT10 | BIT8)

/* FCIE_RESET 0x3F */

#define BIT_FCIE_SOFT_RST_n BIT0
#define BIT_RST_MIU_STS     BIT1
#define BIT_RST_MIE_STS     BIT2
#define BIT_RST_MCU_STS     BIT3
#define BIT_RST_ECC_STS     BIT4
//#define BIT_RST_STS_MASK                (BIT_RST_MIU_STS | BIT_RST_MIE_STS | BIT_RST_MCU_STS | BIT_RST_ECC_STS)
#define BIT_RST_STS_MASK     (BIT_RST_MIU_STS | BIT_RST_MIE_STS | BIT_RST_MCU_STS)
#define BIT_NC_DEB_SEL_SHIFT 12
#define BIT_NC_DEB_SEL_MASK  (BIT15 | BIT14 | BIT13 | BIT12)

//------------------------------------------------------------------
/*
 * Power Save FIFO Cmd*
 */
#define PWR_BAT_CLASS (0x1 << 13) /* Battery lost class */
#define PWR_RST_CLASS (0x1 << 12) /* Reset Class */

/* Command Type */
#define PWR_CMD_WREG (0x0 << 9) /* Write data */
#define PWR_CMD_RDCP (0x1 << 9) /* Read and cmp data. If mismatch, HW retry */
#define PWR_CMD_WAIT (0x2 << 9) /* Wait idle, max. 128T */
#define PWR_CMD_WINT (0x3 << 9) /* Wait interrupt */
#define PWR_CMD_STOP (0x7 << 9) /* Stop */

/* RIU Bank */
#define PWR_CMD_BK0 (0x0 << 7)
#define PWR_CMD_BK1 (0x1 << 7)
#define PWR_CMD_BK2 (0x2 << 7)
#define PWR_CMD_BK3 (0x3 << 7)

#define PWR_RIU_ADDR (0x0 << 0)

#endif /* __EMMC_REG_V5_H__ */
