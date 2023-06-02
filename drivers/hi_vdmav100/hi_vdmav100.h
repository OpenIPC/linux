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
#ifndef __HI_VDMA_H__
#define __HI_VDMA_H__



#define  dmac_writew(addr, value)\
    ((*(volatile unsigned int *)(addr)) = (value))

#define  dmac_readw(addr, v)\
    (v = (*(volatile unsigned int *)(addr)))



/****************************************************
 *for vdma
 ****************************************************/
/*global control register define*/
#define DMAC_GLOBLE_CTRL    0x000
#define WFE_EN          (0x1 << 23)
#define EVENT_BROADCAST_EN  (0x1 << 21)
#define AUTO_CLK_GT_EN      (0x1 << 17)
#define AUTO_PRI_EN     (0x1 << 16)
#define WR_CMD_NUM_PER_ARB  (0x4 << 12)
#define RD_CMD_NUM_PER_ARB  (0x4 << 8)
#define WR_OTD_NUM      (0xF << 4)
#define RD_OTD_NUM      (0xF)

#define DMAC_PRI_THRESHOLD  0x004
/*PRRR and NMRR define*/
#define DMAC_MMU_NMRR  0x008
#define DMAC_MMU_PRRR  0x00C
/*read and write dustb address register define*/
#define DMAC_RD_DUSTB_ADDR      0x010
#define DMAC_WR_DUSTB_ADDR      0x014
/*channel status register define*/
#define DMAC_CHANNEL_STATUS     0x01c
#define DMAC_WORK_DURATION      0x020
#define DMAC_INT_STATUS       0x02c
/*the definition for DMAC channel register*/
#define DMAC_CHANNEL_BASE           0x100
#define DMAC_CxSRCADDR(i)   (DMAC_CHANNEL_BASE + 0x00 + 0x20 * i)
#define DMAC_CxDESTADDR(i)  (DMAC_CHANNEL_BASE + 0x04 + 0x20 * i)
#define DMAC_CxLENGTH(i)    (DMAC_CHANNEL_BASE + 0x08 + 0x20 * i)
#define DMAC_CxTTBR(i)      (DMAC_CHANNEL_BASE + 0x0C + 0x20 * i)
#define DMAC_CxMISC(i)      (DMAC_CHANNEL_BASE + 0x10 + 0x20 * i)
#define DMAC_CxINTR_RAW(i)  (DMAC_CHANNEL_BASE + 0x14 + 0x20 * i)
#define CX_INT_STAT    (0x1 << 4)
#define CX_INT_TC_RAW  (0x1 << 3)
#define CX_INT_TE_RAW  (0x1 << 2)
#define CX_INT_TM_RAW  (0x1 << 1)
#define CX_INT_AP_RAW  (0x1 << 0)

#define DMAC_INTR_ENABLE               (0x1 << 8)

/* channel enable */
#define DMAC_CHANNEL_ENABLE     (0x1 << 9)

/* access flag enable */
#define AFE         (0x1 << 6)

/*user and kernel define*/
#define DEST_IS_KERNEL      (0x1 << 2)
#define SRC_IS_KERNEL       (0x1 << 1)

/*for TTBR*/
#define TTB_RGN         (0x1 << 3) /*outer cache write back allocate*/


/*for ap and cache remap*/

/*remap enable,ap access check enable*/
#define TRE 0x001

#define PRRR     0xff0a81a8
#define NMRR     0x40e040e0

#define DMAC_SYNC_VAL               0x0

/*definition for the return value*/
#define DMAC_ERROR_BASE             100
#define DMAC_CHANNEL_INVALID            (DMAC_ERROR_BASE + 1)

#define DMAC_TRXFERSIZE_INVALID         (DMAC_ERROR_BASE + 2)
#define DMAC_SOURCE_ADDRESS_INVALID     (DMAC_ERROR_BASE + 3)
#define DMAC_DESTINATION_ADDRESS_INVALID    (DMAC_ERROR_BASE + 4)
#define DMAC_MEMORY_ADDRESS_INVALID     (DMAC_ERROR_BASE + 5)
#define DMAC_PERIPHERAL_ID_INVALID      (DMAC_ERROR_BASE + 6)
#define DMAC_DIRECTION_ERROR            (DMAC_ERROR_BASE + 7)
#define DMAC_TRXFER_ERROR           (DMAC_ERROR_BASE + 8)
#define DMAC_LLIHEAD_ERROR          (DMAC_ERROR_BASE + 9)
#define DMAC_SWIDTH_ERROR           (DMAC_ERROR_BASE + 0xa)
#define DMAC_LLI_ADDRESS_INVALID        (DMAC_ERROR_BASE + 0xb)
#define DMAC_TRANS_CONTROL_INVALID      (DMAC_ERROR_BASE + 0xc)
#define DMAC_MEMORY_ALLOCATE_ERROR      (DMAC_ERROR_BASE + 0xd)
#define DMAC_NOT_FINISHED           (DMAC_ERROR_BASE + 0xe)

#define DMAC_TIMEOUT                (DMAC_ERROR_BASE + 0xf)
#define DMAC_CHN_SUCCESS            (DMAC_ERROR_BASE + 0x10)
#define DMAC_CHN_CONFIG_ERROR           (DMAC_ERROR_BASE + 0x11)
#define DMAC_CHN_DATA_ERROR         (DMAC_ERROR_BASE + 0x12)
#define DMAC_CHN_TIMEOUT            (DMAC_ERROR_BASE + 0x13)
#define DMAC_CHN_ALLOCAT            (DMAC_ERROR_BASE + 0x14)
#define DMAC_CHN_VACANCY            (DMAC_ERROR_BASE + 0x15)

#define DMAC_MAX_CHANNELS           CONFIG_HI_VDMA_CHN_NUM

#define CHANNEL_NUM DMAC_MAX_CHANNELS

#define DMA_TRANS_OK          0x1
#define DMA_PAGE_FAULT      0x2
#define DMA_TRANS_FAULT     0x3
#define DMA_TIMEOUT_HZ      (3 * HZ)

extern int g_channel_status[DMAC_MAX_CHANNELS];
#endif /* End of #ifndef __HI_INC_ECSDMACC_H__ */
