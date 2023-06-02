/**
    NVT emmc info header
 
    @file       nvt_ivot_emmc.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_NVT_IVOT_EMMC_H
#define __ASM_ARCH_NVT_IVOT_EMMC_H
struct nvt_ivot_emmc_partition {
	u64 start;
	u64 num;
	bool	active;
	const char *name;
	struct device_node *of_node;
	int partno;
	char card_id[32];
	char disk_name[32];
};

struct nvt_ivot_emmc_total_partitions {
	struct nvt_ivot_emmc_partition *part_info;
	unsigned int nr_parts;
};
#endif /* __ASM_ARCH_NVT_IVOT_EMMC_H */
