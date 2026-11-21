// SPDX-License-Identifier: GPL-2.0-only
/*
 * Parser for OpenIPC uImage + SquashFS firmware partitions
 *
 * Copyright (C) 2026 Difan Zhang <alex@osqdu.org>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/unaligned.h>

#define SPLIT_BLOCK		0x10000ULL

#define UIMAGE_HDR_SIZE		64
#define UIMAGE_MAGIC		0x27051956
#define UIMAGE_SIZE_OFF		12

#define SQUASHFS_MAGIC_LE	0x73717368	/* "hsqs" */
#define SQUASHFS_SB_READ	48		/* through bytes_used */
#define SQUASHFS_USED_OFF	40

#define OPENIPC_MAX_PARTS	3

static int openipc_uimage_read(struct mtd_info *mtd, loff_t from, size_t len,
			       u8 *buf)
{
	size_t retlen = 0;
	int err;

	err = mtd_read(mtd, from, len, &retlen, buf);
	if (mtd_is_bitflip(err))
		err = 0;
	if (!err && retlen != len)
		return -EIO;

	return err;
}

static int parser_openipc_uimage_parse(struct mtd_info *mtd,
				       const struct mtd_partition **pparts,
				       struct mtd_part_parser_data *data)
{
	u8 buf[UIMAGE_HDR_SIZE];
	struct mtd_partition *parts;
	u64 rootfs_off, data_off, used;
	u32 data_size;
	int err, nr_parts = 0;

	if (mtd->type != MTD_NORFLASH || !mtd->erasesize ||
	    (u32)SPLIT_BLOCK % mtd->erasesize ||
	    mtd->size < 2 * SPLIT_BLOCK)
		return -ENOENT;

	err = openipc_uimage_read(mtd, 0, UIMAGE_HDR_SIZE, buf);
	if (err) {
		pr_err("%s: failed to read uImage header: %d\n",
		       mtd->name, err);
		return -ENOENT;
	}

	if (get_unaligned_be32(buf) != UIMAGE_MAGIC)
		return -ENOENT;

	data_size = get_unaligned_be32(buf + UIMAGE_SIZE_OFF);
	if (!data_size)
		return -ENOENT;

	rootfs_off = round_up((u64)UIMAGE_HDR_SIZE + data_size, SPLIT_BLOCK);
	if (rootfs_off > mtd->size - SPLIT_BLOCK)
		return -ENOENT;

	err = openipc_uimage_read(mtd, rootfs_off, SQUASHFS_SB_READ, buf);
	if (err) {
		pr_err("%s: failed to read SquashFS superblock at %#llx: %d\n",
		       mtd->name, rootfs_off, err);
		return -ENOENT;
	}

	if (get_unaligned_le32(buf) != SQUASHFS_MAGIC_LE) {
		pr_warn("%s: no SquashFS superblock at %#llx (fixed-slot image?)\n",
			mtd->name, rootfs_off);
		return -ENOENT;
	}

	used = get_unaligned_le64(buf + SQUASHFS_USED_OFF);
	if (!used || used > mtd->size - rootfs_off)
		return -ENOENT;

	data_off = round_up(rootfs_off + used, SPLIT_BLOCK);
	if (data_off < rootfs_off || data_off > mtd->size)
		return -ENOENT;

	parts = kcalloc(OPENIPC_MAX_PARTS, sizeof(*parts), GFP_KERNEL);
	if (!parts)
		return -ENOMEM;

	parts[nr_parts].name = "kernel";
	parts[nr_parts].offset = 0;
	parts[nr_parts++].size = rootfs_off;

	parts[nr_parts].name = "rootfs";
	parts[nr_parts].offset = rootfs_off;
	parts[nr_parts++].size = data_off - rootfs_off;

	if (data_off < mtd->size) {
		parts[nr_parts].name = "rootfs_data";
		parts[nr_parts].offset = data_off;
		parts[nr_parts++].size = mtd->size - data_off;
	}

	*pparts = parts;
	return nr_parts;
}

static const struct of_device_id mtd_parser_openipc_uimage_of_match_table[] = {
	{ .compatible = "openipc,uimage-firmware" },
	{},
};
MODULE_DEVICE_TABLE(of, mtd_parser_openipc_uimage_of_match_table);

static struct mtd_part_parser mtd_parser_openipc_uimage = {
	.parse_fn = parser_openipc_uimage_parse,
	.name = "openipc-uimage",
	.of_match_table = mtd_parser_openipc_uimage_of_match_table,
};
module_mtd_part_parser(mtd_parser_openipc_uimage);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Difan Zhang <alex@osqdu.org>");
MODULE_DESCRIPTION("OpenIPC uImage firmware partition parser");
