/**
    NVT IO header file
    This file will provide basic IO access API
    @file       nvt-io.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __ASM_ARCH_NVT_IVOT_IO_H
#define __ASM_ARCH_NVT_IVOT_IO_H

#include <asm/io.h>

#define nvt_readb(offset)		readb((void __iomem *)(offset))
#define nvt_readw(offset)		readw((void __iomem *)(offset))
#define nvt_readl(offset)		readl((void __iomem *)(offset))

#define nvt_writeb(val, offset)		writeb(val, (void __iomem *)(offset))
#define nvt_writew(val, offset)		writew(val, (void __iomem *)(offset))
#define nvt_writel(val, offset)		writel(val, (void __iomem *)(offset))

#endif /* __ASM_ARCH_NVT_IVOT_IO_H */
