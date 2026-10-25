#ifndef __MTD_SPLIT_LAYOUT_H__
#define __MTD_SPLIT_LAYOUT_H__

struct mtd_info;

/*
 * Register master's partitions from the split NOR layout (see
 * drivers/mtd/split_layout.c): 0 when it did, 1 when the flash does not hold
 * that layout and the caller registers its partitions as before, a negative
 * errno when the registration itself failed.
 */
#ifdef CONFIG_MTD_SPLIT_LAYOUT
int mtd_split_layout_register(struct mtd_info *master);
#else
static inline int mtd_split_layout_register(struct mtd_info *master)
{
	return 1;
}
#endif

#endif
