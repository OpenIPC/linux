/*
 * Copyright (c) 2009-2014 HiSilicon Technologies Co., Ltd.
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

#ifndef _HISI_SATA_DBG_H
#define _HISI_SATA_DBG_H
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/libata.h>
#include "ahci.h"


void hisi_sata_mem_dump(unsigned int *addr, unsigned int size);
void hisi_sata_phys_mem_dump(unsigned int addr, unsigned int size);
void hisi_ahci_rx_fis_dump(struct ata_link *link, int pmp_port_num);
void hisi_ata_taskfile_dump(struct ata_taskfile *tf);
void hisi_ahci_st_dump(void __iomem *port_base);
void hisi_ahci_reg_dump(void);

#define HISI_AHCI_REG_DUMP(X) \
do {\
    pr_debug("------------------[ Start ]--------------------\n"); \
    pr_debug("Dump AHCI registers at %s %d\n", __func__, __LINE__); \
    hisi_ahci_reg_dump(); \
    pr_debug("------------------[  End  ]--------------------\n");\
} while (0)

#define hisi_sata_readl(addr) do {\
        unsigned int reg = readl((unsigned int)addr); \
        pr_debug("HI_AHCI(REG) %s:%d: readl(0x%08X) = 0x%08X\n",\
        __func__, __LINE__, (unsigned int)addr, reg); \
        reg;\
    } while (0)

#define hisi_sata_writel(v, addr) do { writel(v, (unsigned int)addr); \
    pr_debug("HI_AHCI(REG) %s:%d: writel(0x%08X) = 0x%08X\n",\
        __func__, __LINE__, (unsigned int)addr, \
        (unsigned int)(v)); \
    } while (0)

#undef HISI_DUMP_AHCI_REG_OPS
#ifdef HISI_DUMP_AHCI_REG_OPS
#define readl(addr) hisi_sata_readl(addr)
#define write(v, addr) hisi_sata_writel(v, addr)
#endif

#endif /* _HISI_SATA_DBG_H */



