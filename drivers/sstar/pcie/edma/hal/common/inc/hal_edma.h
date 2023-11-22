/*
 * hal_edma.h- Sigmastar
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

#ifndef _HAL_EDMA_H_
#define _HAL_EDMA_H_

#include "drv_pcieif.h"
#include "hal_edma_plat.h"

/* registers */
#define DMA_CTRL_OFF                     0x08
#define EDMA_WRITE_CH_CNT_MASK           (0xF)
#define EDMA_WRITE_CH_CNT_SHIFT          (0)
#define EDMA_READ_CH_CNT_MASK            (0xF0000)
#define EDMA_READ_CH_CNT_SHIFT           (16)
#define DMA_WRITE_ENGINE_EN_OFF          0x0C
#define EDMA_ENGINE_EN                   (0x01)
#define DMA_WRITE_DOORBELL_OFF           0x10
#define DMA_READ_ENGINE_EN_OFF           0x2C
#define DMA_READ_DOORBELL_OFF            0x30
#define DMA_WRITE_INT_STATUS_OFF         0x4C
#define DMA_WRITE_INT_MASK_OFF           0x54
#define DMA_WRITE_INT_CLEAR_OFF          0x58
#define DMA_WRITE_ERR_STATUS_OFF         0x5C
#define EDMA_DONE_INT_MASK               (0xFF)
#define EDMA_DONE_INT_SHIFT              (0)
#define EDMA_DONE_INT_CHi(ch)            (1 << ch)
#define EDMA_ABORT_INT_MASK              (0xFF0000)
#define EDMA_ABORT_INT_SHIFT             (16)
#define EDMA_ABORT_INT_CHi(ch)           (1 << (ch + 16))
#define DMA_WRITE_LINKED_LIST_ERR_EN_OFF 0x90
#define EDMA_CHi_LLRAIE(ch)              (1 << ch)
#define EDMA_CHi_LLLAIE(ch)              (1 << (ch + 16))
#define DMA_READ_INT_STATUS_OFF          0xA0
#define DMA_READ_INT_MASK_OFF            0xA8
#define DMA_READ_INT_CLEAR_OFF           0xAC
#define DMA_READ_ERR_STATUS_LOW_OFF      0xB4
#define DMA_READ_ERR_STATUS_HIGH_OFF     0xB8
#define DMA_READ_LINKED_LIST_ERR_EN_OFF  0xC4
#define DMA_WRITE_CHi_PWR_EN_OFF(ch)     (0x128 + (ch << 2))
#define DMA_READ_CHi_PWR_EN_OFF(ch)      (0x168 + (ch << 2))
#define EDMA_PWR_EN                      (0x01)
#define DMA_CH_CONTROL1_OFF_WRCH_i(ch)   (0x200 + (ch * 0x200))
#define DMA_LLP_LOW_OFF_WRCH_i(ch)       (0x21C + (ch * 0x200))
#define DMA_LLP_HIGH_OFF_WRCH_i(ch)      (0x220 + (ch * 0x200))
#define DMA_CH_CONTROL1_OFF_RDCH_i(ch)   (0x300 + (ch * 0x200))
#define DMA_LLP_LOW_OFF_RDCH_i(ch)       (0x31C + (ch * 0x200))
#define DMA_LLP_HIGH_OFF_RDCH_i(ch)      (0x320 + (ch * 0x200))
/* DMA_CH_CONTROL1 */
#define EDMA_CH_CTRL1_CCS (0x100) // Consume Cycle State
#define EDMA_CH_CTRL1_LLE (0x200) // Linked List Enable

u16  halEDMA_GetWrChCnt(u8 id);
u16  halEDMA_GetRdChCnt(u8 id);
int  halEDMA_StartXfer(u8 id, u32 ch, bool write, ss_miu_addr_t ll);
void halEDMA_StopDev(u8 id, bool write);
u8   halEDMA_GetDoneIntrSt(u8 id, bool write);
u8   halEDMA_GetAbortIntrSt(u8 id, bool write);
void halEDMA_ClrDoneIntrSt(u8 id, u8 st, bool write);
void halEDMA_ClrAbortIntrSt(u8 id, u8 st, bool write);

#endif /* _HAL_EDMA_H_ */
