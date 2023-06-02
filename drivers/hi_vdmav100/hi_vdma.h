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

#ifndef __VDMA_USER_H__
#define __VDMA_USER_H__

struct hivdmac_host {
    struct device *dev;
    struct clk *clk;
    struct reset_control *rstc;
    void __iomem *regbase;

    int irq;
};

#define VDMA_DATA_CMD   0x6

struct dmac_user_para {
    unsigned int src;
    unsigned int dst;
    unsigned int size;
};

extern int hi_memcpy(void *dst, const void *src, size_t count);


#endif
