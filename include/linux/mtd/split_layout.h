/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MTD_SPLIT_LAYOUT_H__
#define __MTD_SPLIT_LAYOUT_H__

struct mtd_info;
struct mtd_partition;

/*
 * The OpenIPC split NOR layout (drivers/mtd/split_layout.c).
 *
 * mtd_split_layout_parts(): when master holds the layout, the partitions it
 * divides into, in a table the caller kfree()s, and how many; 0 when it does
 * not, and the flash is partitioned as before; -ENOMEM.
 *
 * mtd_split_layout_set_root(): once those are registered, their "rootfs" is
 * the root filesystem (see prepare_namespace()).
 */
#ifdef CONFIG_MTD_SPLIT_LAYOUT
int mtd_split_layout_parts(struct mtd_info *master,
			   struct mtd_partition **pparts);
void mtd_split_layout_set_root(struct mtd_info *master);
#else
static inline int mtd_split_layout_parts(struct mtd_info *master,
					 struct mtd_partition **pparts)
{
	return 0;
}
static inline void mtd_split_layout_set_root(struct mtd_info *master) { }
#endif

#endif
