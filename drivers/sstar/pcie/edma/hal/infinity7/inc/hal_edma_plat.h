/*
 * hal_edma_plat.h- Sigmastar
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

#ifndef _HAL_EDMA_PLAT_H_
#define _HAL_EDMA_PLAT_H_

/* DBI address bus layout
 *
 * Due the 32-bit data type length, reg addr bit[21:20] are used as CDM/ELBI & CS2 select bits
 *
 * In the design:
 * type | 32 | 31 | 30-20 | 19 | 18-2 | 1 | 0 |
 *  CMD |  0 | CS2|       |  0 |      | 0 | 0 |
 *  ATU |  1 |  1 |       |  0 | addr | 0 | 0 |
 *  DMA |  1 |  1 |       |  1 | addr | 0 | 0 |
 *
 *
 * reg addr layout:
 * type | 31-22 | 21 | 20 | 19 | 18-2 | 1 | 0 |
 *  CMD |       |  0 | CS2|  0 |      | 0 | 0 |
 *  ATU |       |  1 |  1 |  0 | addr | 0 | 0 |
 *  DMA |       |  1 |  1 |  1 | addr | 0 | 0 |
 */
#define DMA_DBI_ADDR ((0x3 << 20) | (0x1 << 19))

static inline void _writel_dma_reg(u8 id, u32 reg, u32 val)
{
    sstar_pcieif_writel_dbi(id, reg + DMA_DBI_ADDR, val);
}

static inline u32 _readl_dma_reg(u8 id, u32 reg)
{
    return sstar_pcieif_readl_dbi(id, reg + DMA_DBI_ADDR);
}

#endif /* _HAL_EDMA_PLAT_H_ */
