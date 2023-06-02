/*
 * mach/sram.h - FH simple SRAM allocator
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __MACH_SRAM_H
#define __MACH_SRAM_H

/*
 * SRAM allocations return a CPU virtual address, or NULL on error.
 * If a DMA address is requested and the SRAM supports DMA, its
 * mapped address is also returned.
 *
 * Errors include SRAM memory not being available, and requesting
 * DMA mapped SRAM on systems which don't allow that.
 */
extern void *sram_alloc(size_t len, dma_addr_t *dma);
extern void sram_free(void *addr, size_t len);

#endif /* __MACH_SRAM_H */
