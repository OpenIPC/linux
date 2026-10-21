/*
 * The split NOR layout with a legacy uImage kernel: one "firmware" partition
 * holding a uImage and, at the next erase block after it, the root squashfs.
 * Any U-Boot that can bootm a uImage from the start of that partition boots
 * it -- a vendor's included -- and the kernel finds its partitions here.
 *
 * Only that exact shape is split. A fixed-slot layout also starts with a
 * uImage, but its squashfs sits at the slot boundary; requiring the squashfs
 * right after the kernel leaves such a partition (or a "firmware" alias over
 * both slots) whole.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include "mtdsplit.h"

#define IH_MAGIC	0x27051956
#define IH_HDR_SIZE	64

struct uimage_header {
	__be32 ih_magic;
	__be32 ih_hcrc;
	__be32 ih_time;
	__be32 ih_size;		/* image data size, after the header */
	__be32 ih_load;
	__be32 ih_ep;
	__be32 ih_dcrc;
	u8 ih_os, ih_arch, ih_type, ih_comp;
	u8 ih_name[32];
};

static int mtdsplit_uimage_parse(struct mtd_info *mtd,
				 struct mtd_partition **pparts,
				 struct mtd_part_parser_data *data)
{
	struct uimage_header hdr;
	struct mtd_partition *parts;
	enum mtdsplit_part_type type;
	size_t retlen;
	u64 kernel_size;
	int ret;

	ret = mtd_read(mtd, 0, sizeof(hdr), &retlen, (u_char *)&hdr);
	/* -EUCLEAN: corrected bitflips, the data is good */
	if (ret && !mtd_is_bitflip(ret))
		return ret;
	if (retlen != sizeof(hdr) || be32_to_cpu(hdr.ih_magic) != IH_MAGIC)
		return -ENODEV;

	kernel_size = mtd_roundup_to_eb(IH_HDR_SIZE + (u64)be32_to_cpu(hdr.ih_size), mtd);
	if (kernel_size >= mtd->size)
		return -ENODEV;

	/* The squashfs immediately after the kernel, or this is not the split layout. */
	if (mtd_check_rootfs_magic(mtd, kernel_size, &type) ||
	    type != MTDSPLIT_PART_TYPE_SQUASHFS)
		return -ENODEV;

	parts = kzalloc(2 * sizeof(*parts), GFP_KERNEL);
	if (!parts)
		return -ENOMEM;

	parts[0].name = KERNEL_PART_NAME;
	parts[0].offset = 0;
	parts[0].size = kernel_size;

	parts[1].name = ROOTFS_PART_NAME;
	parts[1].offset = kernel_size;
	parts[1].size = mtd->size - kernel_size;

	*pparts = parts;
	return 2;
}

static struct mtd_part_parser uimage_fw_parser = {
	.owner = THIS_MODULE,
	.name = "uimage-fw",
	.parse_fn = mtdsplit_uimage_parse,
	.type = MTD_PARSER_TYPE_FIRMWARE,
};

static int __init mtdsplit_uimage_init(void)
{
	register_mtd_parser(&uimage_fw_parser);
	return 0;
}

module_init(mtdsplit_uimage_init);
