/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
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


#ifndef __HI_DMAC_H__
#define __HI_DMAC_H__

#define  dmac_writew(addr, value)\
    writel(value, (void *)(addr))
#define  dmac_readw(addr, v)\
    v = readl((void *)(addr))

/*#define DMA_DEBUG*/
#ifdef DMA_DEBUG
#define dma_debug printk
#else
#define dma_debug(fmt, ...) do {} while (0);
#endif

#define DMAC_CONFIGURATIONx_HALT_DMA_ENABLE (0x01L<<18)
#define DMAC_CONFIGURATIONx_ACTIVE      (0x01L<<17)
#define DMAC_CONFIGURATIONx_CHANNEL_ENABLE  1
#define DMAC_CONFIGURATIONx_CHANNEL_DISABLE 0

/*definition for the return value*/
#define DMAC_ERROR_BASE             100
#define DMAC_CHANNEL_INVALID            (DMAC_ERROR_BASE+1)

#define DMAC_TRXFERSIZE_INVALID         (DMAC_ERROR_BASE+2)
#define DMAC_SOURCE_ADDRESS_INVALID     (DMAC_ERROR_BASE+3)
#define DMAC_DESTINATION_ADDRESS_INVALID    (DMAC_ERROR_BASE+4)
#define DMAC_MEMORY_ADDRESS_INVALID     (DMAC_ERROR_BASE+5)
#define DMAC_PERIPHERAL_ID_INVALID      (DMAC_ERROR_BASE+6)
#define DMAC_DIRECTION_ERROR            (DMAC_ERROR_BASE+7)
#define DMAC_TRXFER_ERROR           (DMAC_ERROR_BASE+8)
#define DMAC_LLIHEAD_ERROR          (DMAC_ERROR_BASE+9)
#define DMAC_SWIDTH_ERROR           (DMAC_ERROR_BASE+0xa)
#define DMAC_LLI_ADDRESS_INVALID        (DMAC_ERROR_BASE+0xb)
#define DMAC_TRANS_CONTROL_INVALID      (DMAC_ERROR_BASE+0xc)
#define DMAC_MEMORY_ALLOCATE_ERROR      (DMAC_ERROR_BASE+0xd)
#define DMAC_NOT_FINISHED           (DMAC_ERROR_BASE+0xe)

#define DMAC_TIMEOUT                (DMAC_ERROR_BASE+0xf)
#define DMAC_CHN_SUCCESS            (DMAC_ERROR_BASE+0x10)
#define DMAC_CHN_ERROR              (DMAC_ERROR_BASE+0x11)
#define DMAC_CHN_TIMEOUT            (DMAC_ERROR_BASE+0x12)
#define DMAC_CHN_ALLOCAT            (DMAC_ERROR_BASE+0x13)
#define DMAC_CHN_VACANCY            (DMAC_ERROR_BASE+0x14)

#define DMAC_CONFIGURATIONx_ACTIVE_NOT      0

/*the means the bit in the channel control register*/
#define DMAC_TRANS_SIZE         0xff0

/*DMAC peripheral structure*/
typedef struct dmac_peripheral {
    /* peripherial ID*/
    unsigned int peri_id;
    /*peripheral data register address*/
    unsigned int peri_addr;
    /*default channel control word*/
    unsigned int transfer_ctrl;
    /*default channel configuration word*/
    unsigned int transfer_cfg;
    /*default channel configuration word*/
    unsigned int transfer_width;
} dmac_peripheral;

typedef struct mem_addr {
    unsigned int addr_base;
    unsigned int size;
} mem_addr;

typedef unsigned int dma_addr_t;
/* #define PAGE_SIZE 0x1000 */

#endif /* End of #ifndef __HI_INC_ECSDMACC_H__ */
