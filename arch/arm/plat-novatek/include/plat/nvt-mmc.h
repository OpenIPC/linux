/**
	NVT MMC header file
	This file will provide the MMC host controller data strcuture declaration
	@file       nvt-mmc.h
	@ingroup
	@note
	Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/
#ifndef __ASM_ARCH_NA51055_NVT_MMC_H
#define __ASM_ARCH_NA51055_NVT_MMC_H

#include <linux/types.h>
#include <linux/mmc/host.h>

struct nvt_mmc_config {
	/* get_cd()/get_wp() may sleep */
	int	(*get_cd)(struct mmc_host *mmc);
	int	(*get_ro)(struct mmc_host *mmc);
	/* wires == 0 is equivalent to wires == 4 (4-bit parallel) */
	u8	wires;

	u32     max_freq;

	/* any additional host capabilities: OR'd in to mmc->f_caps */
	u32     caps;

	/* cd irq number */
	u32     cd_irq;

	/* Version of the MMC/SD controller */
	u8	version;

	/* Number of sg segments */
	u8	nr_sg;
};
void nvt_setup_mmc(int module, struct nvt_mmc_config *config);

enum {
	MMC_CTLR_VERSION_1 = 0,	/* DM644x and DM355 */
	MMC_CTLR_VERSION_2,	/* DA830 */
};

#endif /* __ASM_ARCH_NA51000_NVT_MMC_H */
