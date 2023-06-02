//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
#ifndef __UNFD_FCIE5_REG_H__
#define __UNFD_FCIE5_REG_H__

#ifndef BIT0
#define BIT0    (1<<0)
#endif
#ifndef BIT1
#define BIT1    (1<<1)
#endif
#ifndef BIT2
#define BIT2    (1<<2)
#endif
#ifndef BIT3
#define BIT3    (1<<3)
#endif
#ifndef BIT4
#define BIT4    (1<<4)
#endif
#ifndef BIT5
#define BIT5    (1<<5)
#endif
#ifndef BIT6
#define BIT6    (1<<6)
#endif
#ifndef BIT7
#define BIT7    (1<<7)
#endif
#ifndef BIT8
#define BIT8    (1<<8)
#endif
#ifndef BIT9
#define BIT9    (1<<9)
#endif
#ifndef BIT10
#define BIT10   (1<<10)
#endif
#ifndef BIT11
#define BIT11   (1<<11)
#endif
#ifndef BIT12
#define BIT12   (1<<12)
#endif
#ifndef BIT13
#define BIT13   (1<<13)
#endif
#ifndef BIT14
#define BIT14   (1<<14)
#endif
#ifndef BIT15
#define BIT15   (1<<15)
#endif
//------------------------------------------------------------------
#define ECC_TYPE_RS                 1
#define ECC_TYPE_4BIT               2
#define ECC_TYPE_8BIT               3
#define ECC_TYPE_12BIT              4
#define ECC_TYPE_16BIT              5
#define ECC_TYPE_20BIT              6
#define ECC_TYPE_24BIT              7
#define ECC_TYPE_24BIT1KB           8
#define ECC_TYPE_32BIT1KB           9
#define ECC_TYPE_40BIT1KB           10
//FCIE5 only
#define ECC_TYPE_8BIT1KB            11
#define ECC_TYPE_16BIT1KB           12
#define ECC_TYPE_32BIT              13
#define ECC_TYPE_40BIT              14
//depends on platform
#define ECC_TYPE_60BIT              15
#define ECC_TYPE_60BIT1KB           16


#define ECC_CODE_BYTECNT_RS         10
#define ECC_CODE_BYTECNT_4BIT        8
#define ECC_CODE_BYTECNT_8BIT       14

#define ECC_CODE_BYTECNT_12BIT      20
#define ECC_CODE_BYTECNT_16BIT      28
#define ECC_CODE_BYTECNT_20BIT      34
#define ECC_CODE_BYTECNT_24BIT      42

#define ECC_CODE_BYTECNT_24BIT1KB   42
#define ECC_CODE_BYTECNT_32BIT1KB   56
#define ECC_CODE_BYTECNT_40BIT1KB   70

//@FIXME
#define ECC_CODE_BYTECNT_8BIT1KB    14
#define ECC_CODE_BYTECNT_16BIT1KB   28
#define ECC_CODE_BYTECNT_32BIT      56
#define ECC_CODE_BYTECNT_40BIT      70
#define ECC_CODE_BYTECNT_60BIT      106
#define ECC_CODE_BYTECNT_60BIT1KB   106



//------------------------------------------------------------------
#define NC_RESERVED_FOR_SW    GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x10)
#define NC_CIFD_EVENT         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x30)
#define NC_CIFD_INT_EN        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x31)
#define NC_PWR_RD_MASK        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x34)
#define NC_PWR_SAVE_CTL       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x35)
#define NC_BIST_MODE          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x36)
#define NC_BOOT_MODE          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x37)
#define NC_DEBUG_DBUS0        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x38)
#define NC_DEBUG_DBUS1        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x39)
#define NC_CLK_EN             GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3E)
#define NC_FCIE_RST           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3F)
#define NC_SIGNAL             GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x40)
#define NC_WIDTH              GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x41)
#define NC_STAT_CHK           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x42)
#define NC_AUXREG_ADR         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x43)
#define NC_AUXREG_DAT         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x44)
#define NC_CTRL               GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x45)
#define NC_ST_READ            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x46)
#define NC_PART_MODE          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x47)
#define NC_SPARE              GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x48)
#define NC_SPARE_SIZE         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x49)
#define NC_ADDEND             GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4A)
#define NC_FCIE_ZERO_BITCNT   GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4B)
#define NC_NAND_ZERO_BITCNT   GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4C) /*before scramble*/
#define NC_MIU_CTRL           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4D)
#define NC_SECURE_ERR_ADDR_L  GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4E)
#define NC_SECURE_ERR_ADDR_H  GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4F)
#define NC_ECC_CTRL           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x50)
#define NC_ECC_STAT0          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x51)
#define NC_ECC_STAT1          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x52)
#define NC_ECC_STAT2          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x53)
#define NC_ECC_LOC            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x54)
#define NC_RAND_R_CMD         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x55)
#define NC_RAND_W_CMD         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x56)
#define NC_ECO_FIX            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x57) // B3,G1
#define NC_LATCH_DATA         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x57)
#define NC_DDR_CTRL           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x58)
#define NC_LFSR_CTRL          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x59)
#define NC_NAND_TIMING        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5A)
#define NC_SER_DIN_BYTECNT_LW GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5B)
#define NC_SER_DIN_BYTECNT_HW GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5C)
#define NC_NAND_TIMING1       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5D)
#define NC_RX5E               GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5E)
#define NC_RX5F               GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5F)
#define NC_MIE_EVENT          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x60)
#define NC_MIE_INT_EN         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x61)
#define NC_FORCE_INT          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x62)
#define NC_FUN_CTL            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x63)
#define NC_RDATA_DMA_ADR0     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x64)
#define NC_RDATA_DMA_ADR1     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x65)
#define NC_WDATA_DMA_ADR0     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x66)
#define NC_WDATA_DMA_ADR1     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x67)
#define NC_RSPARE_DMA_ADR0    GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x68)
#define NC_RSPARE_DMA_ADR1    GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x69)
#define NC_WSPARE_DMA_ADR0    GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x6A)
#define NC_WSPARE_DMA_ADR1    GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x6B)
#define NC_DATA_STS           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x6C)
#define NC_RBZ_STS            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x6D)
#define NC_SSO_CTL            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x6E)
#define NC_SSO_DATA           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x6F)
#define NC_REORDER            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x70)
#define NC_RPT_CNT            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x71)
#define NC_TR_RPT_CNT         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x72)
#define NC_1ST_UECC_PAGE      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x73)
#define NC_REMAIN_RPT_CNT     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x74)

#define NC_WBUF_CIFD_ADDR(u16_pos)   GET_REG_ADDR(FCIE_NC_WBUF_CIFD_BASE, u16_pos) // 32 x 16 bits SW Read only
#define NC_RBUF_CIFD_ADDR(u16_pos)   GET_REG_ADDR(FCIE_NC_RBUF_CIFD_BASE, u16_pos) // 32 x 16 bits  SW write/read

#define NC_CIFD_ADDR(u16_pos)        NC_RBUF_CIFD_ADDR(u16_pos)

#define NC_CIFD_WBUF_BYTE_CNT        0x40 // 32 x 16 bits
#define NC_CIFD_RBUF_BYTE_CNT        0x40 // 32 x 16 bits
#define NC_CIFD_BYTE_CNT             0x40 // 32 x 16 bits

#define NC_MAX_SECTOR_BYTE_CNT       (BIT12-1)
#define NC_MAX_SECTOR_SPARE_BYTE_CNT (BIT8-1)
#define NC_MAX_TOTAL_SPARE_BYTE_CNT  (BIT11-1)

/* NC_CIFD_EVENT 0x30 */
#define BIT_WBUF_FULL             BIT0
#define BIT_WBUF_EMPTY_TRI        BIT1
#define BIT_RBUF_FULL_TRI         BIT2
#define BIT_RBUF_EMPTY            BIT3

/* NC_CIFD_INT_EN 0x31 */
#define BIT_WBUF_FULL_INT_EN      BIT0
#define BIT_RBUF_EMPTY_INT_EN     BIT1
#define BIT_F_WBUF_FULL_INT       BIT2
#define BIT_F_RBUF_EMPTY_INT      BIT3

/* NC_PWR_SAVE_CTL*/
#define BIT_PWR_SAVE_MODE         BIT0
#define BIT_SD_PWR_SAVE_RIU       BIT1
#define BIT_PWR_SAVE_INT_EN       BIT2
#define BIT_SD_PWR_SAVE_RST       BIT3
#define BIT_PWR_SAVE_INT_FORCE    BIT4
#define BIT_RIU_SAVE_EVENT        BIT5
#define BIT_RST_SAVE_EVENT        BIT6
#define BIT_BAT_SAVE_EVENT        BIT7
#define BIT_BAT_SD_PWR_SAVE_MASK  BIT8
#define BIT_RST_SD_PWR_SAVE_MASK  BIT9
#define BIT_PWR_SAVE_MODE_INT     BIT15

/* NC_BIST_MODE*/
#define BIT_BIST_ERR_FLAG         BIT0
#define BIT_SEL_BIST_PAT_MASK     (BIT12|BIT13|BIT14|BIT15)
#define BIT_SEL_BIST_PAT_SHIFT    12

/* NC_BOOT_MODE*/
#define BIT_NAND_BOOT_EN          BIT0
#define BIT_BOOT_SRAM_SEL         BIT1

/* NC_DEBUG_BUS0 0x38 */

/* NC_DEBUG_BUS1 0x39 */
#define BIT_FCIE_DBUS16_23        (BIT8 - 1)
#define BIT_FCIE_DEBUG_MODE       (BIT11|BIT10|BIT9|BIT8)

/* NC_CLK_EN 0x3E */
#define BIT_FCIE_CLK_EN           BIT0

/* NC_FCIE_RST 0x3F */
#define BIT_FCIE_SOFT_RST         BIT0
#define BIT_RST_MIU_STS           BIT1
#define BIT_RST_MIE_STS           BIT2
#define BIT_RST_MCU_STS           BIT3
#define BIT_RST_ECC_STS           BIT4
#define BIT_RST_STS_MASK          (BIT_RST_MIU_STS | BIT_RST_MIE_STS |BIT_RST_MCU_STS|BIT_RST_ECC_STS)

/* NC_SIGNAL 0x40 */
#define BIT_NC_CE_SEL_MASK          (BIT2-1)
#define BIT_NC_CE1Z                 BIT0
#define BIT_NC_CE2Z                 BIT1
#define BIT_NC_CE3Z                 (BIT0|BIT1)
#define BIT_NC_CE_H                 BIT2
#define BIT_NC_CE_AUTO              BIT3
#define BIT_NC_WP_H                 BIT4
#define BIT_NC_WP_AUTO              BIT5
#define BIT_NC_CHK_RB_HIGH          BIT6
#define BIT_NC_CHK_RB_EDGEn         BIT7
#define DEF_REG0x40_VAL             (BIT_NC_CE_H|BIT_NC_WP_H)
#define BIT_NC_INST_DELAY_NUM_MASK  ( BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_INST_DELAY_NUM_SHIFT 8

/* NC_WIDTH 0x41 */
#define BIT_W_H_SHIFT              0
#define BIT_W_L_SHIFT              3
#define BIT_R_H_SHIFT              6
#define BIT_R_L_SHIFT              9
#define BIT_CYCLE_CNT_MAX          (BIT2|BIT1|BIT0)
#define BIT_NC_DEB_SEL_SHIFT       12
#define BIT_NC_DEB_SEL             (BIT15|BIT14|BIT13|BIT12)

/* NC_STAT_CHK 0x42 */
#define BIT_NC_FCIE_ALLZERO        BIT2
#define BIT_NC_FCIE_ALLONE         BIT3
#define BIT_NC_NAND_ALLZERO        BIT4 /*before scramble*/
#define BIT_NC_NAND_ALLONE         BIT5 /*before scramble*/

/* NC_CTRL 0x45 */
#define BIT_NC_JOB_START           BIT0
#define BIT_NC_ADMA_EN             BIT1
#define BIT_NC_ZDEC_EN             BIT3
#define BIT_NC_NF2ZDEC_PTR_CLR     BIT4

/* NC_ST_READ 0x46 */
#define BIT_ST_READ_FAIL           BIT0
#define BIT_ST_READ_BUSYn          BIT6
#define BIT_ST_READ_PROTECTn       BIT7

/* NC_PART_MODE 0x47*/
#define BIT_PARTIAL_MODE_EN        BIT0
#define BIT_START_SECTOR_CNT_MASK  (BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
#define BIT_START_SECTOR_CNT_SHIFT 1
#define BIT_START_SECTOR_IDX_MASK  (BIT12|BIT11|BIT10|BIT9|BIT8|BIT7)
#define BIT_START_SECTOR_IDX_SHIFT 7

/* NC_SPARE 0x48 */
#define BIT_NC_SECTOR_SPARE_SIZE_MASK      (BIT8-1)
#define BIT_NC_SPARE_ECC_BYPASS            BIT10
#define BIT_NC_HW_AUTO_RANDOM_CMD_DISABLE  BIT12
#define BIT_NC_ONE_COL_ADDR                BIT13
#define BIT_SPARE_ADR_AUTO_INC             BIT14

/* NC_MIU_CTRL 0x4D*/
#define BIT_MIU_BURST_EN_MASK      (BIT1|BIT0)
#define BIT_MIU_BURST1n            BIT_MIU_BURST_EN_MASK
#define BIT_MIU_BURST2             BIT0
#define BIT_MIU_BURST4             BIT1
#define BIT_MIU_R_PRI              BIT2
#define BIT_MIU_W_PRI              BIT3
#define BIT_MIU_SEL_MASK           (BIT5|BIT4)
#define BIT_MIU0_SELECTn           BIT_MIU_SEL_MASK
#define BIT_MIU1_SELECT            BIT4
#define BIT_MIU2_SELECT            BIT5
#define BIT_MIU3_SELECT            (BIT5|BIT4)
#define BIT_SPARE_MIU_SEL_MASK     (BIT7|BIT6)
#define BIT_SPARE_MIU0_SELECTn     BIT_SPARE_MIU_SEL_MASK
#define BIT_SPARE_MIU1_SELECT      BIT6
#define BIT_SPARE_MIU2_SELECT      BIT7
#define BIT_SPARE_MIU3_SELECT      (BIT7|BIT6)

/* NC_ECC_CTRL 0x50*/
#define BIT_NC_PAGE_SIZE_MASK      (BIT3-1)
#define BIT_NC_PAGE_SIZE_512Bn     BIT_NC_PAGE_SIZE_MASK
#define BIT_NC_PAGE_SIZE_2KB       BIT0
#define BIT_NC_PAGE_SIZE_4KB       BIT1
#define BIT_NC_PAGE_SIZE_8KB       (BIT1|BIT0)
#define BIT_NC_PAGE_SIZE_16KB      BIT2
#define BIT_NC_PAGE_SIZE_32KB      (BIT2|BIT0)
#define BIT_NC_ECC_TYPE_MASK       (BIT6|BIT5|BIT4|BIT3)
#define BIT_NC_ECC_TYPE_8b512Bn    BIT_NC_ECC_TYPE_MASK
#define BIT_NC_ECC_TYPE_8b1KB      BIT3
#define BIT_NC_ECC_TYPE_16b512B    BIT4
#define BIT_NC_ECC_TYPE_16b1KB     (BIT4|BIT3)
#define BIT_NC_ECC_TYPE_24b512B    BIT5
#define BIT_NC_ECC_TYPE_24b1KB     (BIT5|BIT3)
#define BIT_NC_ECC_TYPE_32b512B    (BIT5|BIT4)
#define BIT_NC_ECC_TYPE_32b1KB     (BIT5|BIT4|BIT3)
#define BIT_NC_ECC_TYPE_40b512B    BIT6
#define BIT_NC_ECC_TYPE_40b1KB     (BIT6|BIT3)
#define BIT_NC_ECC_TYPE_60b512B    (BIT6|BIT4)
#define BIT_NC_ECC_TYPE_60b1KB     (BIT6|BIT4|BIT3)
#define BIT_NC_ECCERR_NSTOP        BIT7
#define BIT_NC_DYNAMIC_OFFCLK      BIT8
#define BIT_NC_ALL0xFF_ECC_CHECK   BIT9
#define BIT_NC_BYPASS_ECC          BIT10
#define BIT_NC_SHARE_PAD_EN        BIT14
#define BIT_NC_WORD_MODE           BIT15

/* NC_ECC_STAT0 0x51*/
#define BIT_NC_ECC_FAIL            BIT0
#define BIT_NC_ECC_MAX_BITS_MASK   (BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
#define BIT_NC_ECC_MAX_BITS_SHIFT  1
#define BIT_NC_ECC_SEC_CNT_MASK    (BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)

/* NC_ECC_STAT2 0x53*/
#define BIT_NC_ECC_FLGA_MASK       (BIT1|BIT0)
#define BIT_NC_ECC_FLGA_NOERR      0
#define BIT_NC_ECC_FLGA_CORRECT    BIT0
#define BIT_NC_ECC_FLGA_FAIL       BIT1
#define BIT_NC_ECC_FLGA_CODE       BIT_NC_ECC_FLGA_MASK
#define BIT_NC_ECC_CNT_MASK        (BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)
#define BIT_NC_ECC_CNT_SHIFT       2
#define BIT_NC_ECC_SEL_LOC_MASK    (BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_ECC_SEL_LOC_SHIFT   8

/* NC_RAND_W_CMD 0x56*/ /* for fine tune timing for tADL & random R/W */
#define BIT_NC_HW_DELAY_CNT_MASK   (BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_HW_DELAY_CNT_SHIFT  8

/* NC_LATCH_DATA*/
#define BIT_NC_PAD_SEL_FAILLING    BIT0
#define BIT_NC_LATCH_DATA_MASK     (BIT4|BIT3|BIT2|BIT1)
#define BIT_NC_LATCH_DATA_NORMAL   0
#define BIT_NC_LATCH_DATA_1_0_T    BIT1
#define BIT_NC_LATCH_DATA_2_0_T    BIT2
#define BIT_NC_LATCH_DATA_3_0_T    (BIT1|BIT2)
#define BIT_NC_LATCH_DATA_4_0_T    BIT3
#define BIT_NC_LATCH_DATA_5_0_T    (BIT3|BIT1)
#define BIT_NC_LATCH_DATA_6_0_T    (BIT3|BIT2)
#define BIT_NC_LATCH_DATA_7_0_T    (BIT3|BIT2|BIT1)
#define BIT_NC_LATCH_STS_MASK      (BIT7|BIT6|BIT5)
#define BIT_NC_LATCH_STS_NORMAL    0
#define BIT_NC_LATCH_STS_1_0_T     BIT5
#define BIT_NC_LATCH_STS_2_0_T     BIT6
#define BIT_NC_LATCH_STS_3_0_T     (BIT6|BIT5)
#define BIT_NC_LATCH_STS_4_0_T     BIT7
#define BIT_NC_LATCH_STS_5_0_T     (BIT7|BIT5)
#define BIT_NC_LATCH_STS_6_0_T     (BIT7|BIT6)
#define BIT_NC_LATCH_STS_7_0_T     (BIT7|BIT6|BIT5)
#define BIT_RE_DDR_TIMING_MASK     (BIT8|BIT9|BIT10|BIT11)
#define BIT_RE_DDR_TIMING_SHIFT    8
#define BIT_RE_SEC_TURN_CNT_MASK   (BIT12|BIT13|BIT14|BIT15)
#define BIT_RE_SEC_TURN_CNT_SHIFT  12

/* NC_DDR_CTRL 0x58 */
#define BIT_DDR_MASM                    (BIT_DDR_ONFI|BIT_DDR_TOGGLE)
#define BIT_DDR_ONFI                    BIT0
#define BIT_DDR_TOGGLE                  BIT1
#define BIT_NC_32B_MODE                 BIT2
#define BIT_SDR_DIN_FROM_MACRO          BIT3
#define BIT_NC_TOGGLE_READ_HOLD         BIT4
#define BIT_NC_DUAL_MODE                BIT5
#define BIT_NC_TOGGLE_RD_HOLD_CNT_MARK  (BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_TOGGLE_RD_HOLD_SHIFT     8

/* NC_LFSR_CTRL 0x59 */
#define BIT_TRR_CYCLE_CNT_MASK     (BIT0|BIT1|BIT2|BIT3)
#define BIT_TCS_CYCLE_CNT_MASK     (BIT4|BIT5|BIT6|BIT7)
#define BIT_TCS_CYCLE_CNT_SHIFT    4
#define BIT_SEL_PAGE_MASK          (BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14)
#define BIT_SEL_PAGE_SHIFT         8
#define BIT_LFSR_ENABLE            BIT15
#define IF_LFSR_ENABLED()          (REG(NC_LFSR_CTRL) & BIT_LFSR_ENABLE)

/*NC_NAND_TIMING 0x5A*/
#define BIT_TCLHZ_CNT_MASK         (BIT4-1)
#define BIT_TCLHZ_CNT_SHITF        0
#define BIT_TWHR_CNT_MASK          (BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_TWHR_CNT_SHIFT         8

/*NC_NAND_TIMING1 0x5D*/
#define BIT_TADL_CNT_MASK           (BIT8-1)
#define BIT_TADL_CNT_SHITF          0
#define BIT_TCWAW_CNT_MASK          (BIT15|BIT14|BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_TCWAW_CNT_SHIFT         8

/*NC_EVENT 0x60*/
#define BIT_NC_JOB_END              BIT0
#define BIT_NC_R2N_ECCCOR           BIT1
#define BIT_NC_SECURE_ALERT         BIT2
#define BIT_NC_JOB_ABORT            BIT3

/*NC_INT_EN 0x61*/
#define BIT_NC_JOB_END_EN           BIT0
#define BIT_NC_R2N_ECCCOR_EN        BIT1
#define BIT_NC_SECURE_ALERT_EN      BIT2
#define BIT_NC_JOB_ABORT_EN         BIT3

/*NC_FORCE_INT 0x62*/
#define BIT_F_NC_JOB_INT            BIT0
#define BIT_F_NC_R2N_ECCCOR_INT     BIT1
#define BIT_NC_SECURE_ALERT_INT     BIT2
#define BIT_NC_JOB_ABORT_INT        BIT3

/*NC_FUN_CTL 0x63*/
#define BIT_NC_EN                   BIT0
#define BIT_R2N_MODE_EN             BIT1
#define BIT_BOOT_MODE               BIT2
#define BIT_BIST_MODE               BIT3

/*NC_RBZ_STS 0x6D*/
#define BIT_NF_RBZ_STS              BIT0
#define IF_NAND_RB_HIGH()           (REG(NC_RBZ_STS) & BIT_NF_RBZ_STS)

/*NC_1ST_UECC_PAGE 0x73*/
#define BIT_1ST_UECC_PAGE_MASK      (BIT10-1)

/*NC_REMAIN_RPT_CNT 0x74*/
#define BIT_REMAIN_RPT_CNT_MASK     (BIT10-1)
#define BIT_ABORT_DMA               BIT15


//------------------------------------------------------------------
/* AUX Reg Address */
#define AUXADR_CMDREG8    0x08
#define AUXADR_CMDREG9    0x09
#define AUXADR_CMDREGA    0x0A
#define AUXADR_ADRSET     0x0B
#define AUXADR_RPTCNT     0x18 // Pepeat Count
#define AUXADR_RAN_CNT    0x19
#define AUXADR_RAN_POS    0x1A // offset
#define AUXADR_ST_CHECK   0x1B
#define AUXADR_IDLE_CNT   0x1C
#define AUXADR_INSTQUE    0x20

/* OP Codes: Command */
#define CMD_0x00          0x00
#define CMD_0x30          0x01
#define CMD_0x35          0x02
#define CMD_0x90          0x03
#define CMD_0xFF          0x04
#define CMD_0x80          0x05
#define CMD_0x10          0x06
#define CMD_0x15          0x07
#define CMD_0x85          0x08
#define CMD_0x60          0x09
#define CMD_0xD0          0x0A
#define CMD_0x05          0x0B
#define CMD_0xE0          0x0C
#define CMD_0x70          0x0D
#define CMD_0x50          0x0E
#define CMD_0x01          0x0F
#define CMD_REG8L         0x10
#define CMD_REG8H         0x11
#define CMD_REG9L         0x12
#define CMD_REG9H         0x13
#define CMD_REGAL         0x14
#define CMD_REGAH         0x15

/* OP Code: Address */
#define OP_ADR_CYCLE_00   (0x4 << 4)
#define OP_ADR_CYCLE_01   (0x5 << 4)
#define OP_ADR_CYCLE_10   (0x6 << 4)
#define OP_ADR_CYCLE_11   (0x7 << 4)
#define OP_ADR_TYPE_COL   (0x0 << 2)
#define OP_ADR_TYPE_ROW   (0x1 << 2)
#define OP_ADR_TYPE_FUL   (0x2 << 2)
#define OP_ADR_TYPE_ONE   (0x3 << 2)
#define OP_ADR_SET_0      (0x0 << 0)
#define OP_ADR_SET_1      (0x1 << 0)
#define OP_ADR_SET_2      (0x2 << 0)
#define OP_ADR_SET_3      (0x3 << 0)

#define ADR_C2TRS0        (OP_ADR_CYCLE_00|OP_ADR_TYPE_ROW|OP_ADR_SET_0)
#define ADR_C2T1S0        (OP_ADR_CYCLE_00|OP_ADR_TYPE_ONE|OP_ADR_SET_0)
#define ADR_C3TRS0        (OP_ADR_CYCLE_01|OP_ADR_TYPE_ROW|OP_ADR_SET_0)
#define ADR_C3TFS0        (OP_ADR_CYCLE_00|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C4TFS0        (OP_ADR_CYCLE_01|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C4TFS1        (OP_ADR_CYCLE_01|OP_ADR_TYPE_FUL|OP_ADR_SET_1)
#define ADR_C5TFS0        (OP_ADR_CYCLE_10|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C6TFS0        (OP_ADR_CYCLE_11|OP_ADR_TYPE_FUL|OP_ADR_SET_0)

/* OP Code: Action */
#define ACT_WAITRB        0x80
#define ACT_CHKSTATUS     0x81
#define ACT_WAIT_IDLE     0x82
#define ACT_WAIT_MMA      0x83
#define ACT_BREAK         0x88
#define ACT_SER_DOUT      0x90 /* for column addr == 0 */
#define ACT_RAN_DOUT      0x91 /* for column addr != 0 */
//#define ACT_WR_REDU     0x92
//#define ACT_LUT_DWLOAD  0x93
//#define ACT_LUT_DWLOAD1 0x94
#define ACT_SER_DIN       0x98 /* for column addr == 0 */
#define ACT_RAN_DIN       0x99 /* for column addr != 0 */
//#define ACT_RD_REDU     0x9A
//#define ACT_LUT_UPLOAD  0x9B
#define ACT_PAGECOPY_US   0xA0
#define ACT_PAGECOPY_DS   0xA1
#define ACT_REPEAT        0xB0

/*
 * Power Save FIFO Cmd*
 */
#define PWR_BAT_CLASS    (0x1 << 13)  /* Battery lost class */
#define PWR_RST_CLASS    (0x1 << 12)  /* Reset Class */

/* Command Type */
#define PWR_CMD_WREG     (0x0 << 9)   /* Write data */
#define PWR_CMD_RDCP     (0x1 << 9)   /* Read and cmp data. If mismatch, HW retry */
#define PWR_CMD_WAIT     (0x2 << 9)   /* Wait idle, max. 128T */
#define PWR_CMD_WINT     (0x3 << 9)   /* Wait interrupt */
#define PWR_CMD_STOP     (0x7 << 9)   /* Stop */

/* RIU Bank */
#define PWR_CMD_BK0      (0x0 << 7)
#define PWR_CMD_BK1      (0x1 << 7)
#define PWR_CMD_BK2      (0x2 << 7)
#define PWR_CMD_BK3      (0x3 << 7)

#define PWR_RIU_ADDR     (0x0 << 0)

#endif /* __UNFD_FCIE5_REG_H__ */
