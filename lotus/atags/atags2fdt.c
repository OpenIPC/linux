// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Lotus. All rights reserved.
 *
 * Author:           Lynn
 * Created:          18/08/23
 * Description:      Convert ATAGs list to FDT
 *
 */

#define pr_fmt(fmt) "ATAGs To FDT: " fmt

#include <linux/libfdt.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/io.h>

#include <linux/lotus/tags.h>

#include "setup.h"
#include "lhs_atags.h"

#define LOTUS_MAX_FDT_SIZE (256 * 1024)

static int node_offset(void *fdt, const char *node_path)
{
	int offset = fdt_path_offset(fdt, node_path);

	if (offset == -FDT_ERR_NOTFOUND)
		offset = fdt_add_subnode(fdt, 0, node_path);
	return offset;
}

static int setprop_string(void *fdt, const char *node_path,
			  const char *property, const char *string)
{
	int offset = node_offset(fdt, node_path);

	if (offset < 0)
		return offset;
	return fdt_setprop_string(fdt, offset, property, string);
}

/*
 * Convert and fold provided ATAGs into the provided FDT.
 *
 * REturn values:
 *    = 0 -> pretend success
 *    < 0 -> error from libfdt
 */
int __init atags2fdt(void *atag_list, void *fdt, int total_space)
{
	struct tag *atag = atag_list;

	for_each_tag(atag, atag_list) {
		if (atag->hdr.tag == ATAG_CMDLINE) {
			/* Set the ATAGS command line to the device tree
			 * command line.
			 * This means that if the same parameter is set in
			 * the device tree and in the tags, the one from the
			 * tags will be chosen.
			 */
			setprop_string(fdt, "/chosen", "bootargs",
					       atag->u.cmdline.cmdline);
		} else {
			parse_lotus_tags(atag, fdt);
		}
	}

	return fdt_pack(fdt);
}

bool is_valid_atags(void *params)
{
	struct tag *atag_list = params;

	/* validate the ATAG */
	if (atag_list->hdr.tag != ATAG_CORE ||
	    (atag_list->hdr.size != tag_size(tag_core) &&
	     atag_list->hdr.size != 2)) {
		return false;
	}

	return true;
}

void * __init early_atags_to_fdt(void *params)
{
	struct tag *atag_list = NULL;
	struct tag *atag = NULL;
	char *fdt_addr = NULL;
	int fdt_size = 0;
	int ret = 0;
	char end_param[30] = {0};

	/* make sure we've got an aligned pointer */
	if ((ulong)params & 0x3) {
		pr_err("Not aligned params pointer\n");
		return NULL;
	}

	/* check if we get a DTB */
#define BOOT_PARAMS      0x40000100
#define BOOT_PARAMS_SIZE (0x8000 - 0x100)
	if (*(u32 *)params == fdt32_to_cpu(FDT_MAGIC)) {
		pr_info("FDT got at 0x%lx, try getting ATAGs from 0x%X\n",
				(unsigned long)params, BOOT_PARAMS);
#ifdef CONFIG_ARM64
		atag_list = early_memremap_ro(BOOT_PARAMS, BOOT_PARAMS_SIZE);
#else
		atag_list = (struct tag *)phys_to_virt(BOOT_PARAMS);
#endif
		if (!is_valid_atags(atag_list)) {
			pr_info("No ATAGs found\n");
			fdt_addr = params;
			goto out;
		}
		pr_info("ATAGs found\n");
		fdt_addr = params;
		fdt_size = fdt_totalsize(params);
	} else {
		atag_list = params;
		if (!is_valid_atags(atag_list)) {
			pr_err("Invalid ATAGs!\n");
			fdt_addr = NULL;
			goto out;
		}

		pr_info("ATAGs got at 0x%p\n", atag_list);
		for_each_tag(atag, atag_list) {
			if (atag->hdr.tag == ATAG_FDT) {
				fdt_addr = (char *)&atag->u;
				fdt_size = fdt_totalsize(fdt_addr);
				break;
			}
		}
	}

	if (!fdt_addr) {
		pr_err("No FDT\n");
		fdt_addr = NULL;
		goto out;
	}

	if (fdt_size > LOTUS_MAX_FDT_SIZE) {
		pr_err("Not enough new FDT space for ATAGs");
		fdt_addr = NULL;
		goto out;
	}

	ret = fdt_open_into(fdt_addr, fdt_addr, LOTUS_MAX_FDT_SIZE);
	if (ret < 0) {
		fdt_addr = NULL;
		goto out;
	}

	ret = atags2fdt(atag_list, fdt_addr, LOTUS_MAX_FDT_SIZE);
	if (ret < 0) {
		fdt_addr = NULL;
		goto out;
	}

	/* Check param tag */
	if (get_param_data("end_tag", end_param, sizeof(end_param)) > 0)
		pr_info("GET End Param: %s\n", end_param);

out:
#ifdef CONFIG_ARM64
	if (atag_list)
		early_memunmap(atag_list, BOOT_PARAMS_SIZE);
#endif
	return fdt_addr;
}

