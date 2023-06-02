/*
 * drivers/mtd/mtdblock_ambro.c
 *
 * History:
 *	2014/08/15 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#include <linux/module.h>

struct mtdblk_ambdev {
	struct mtd_blktrans_dev mbd;
	unsigned int *block_map;
	unsigned int blocks;
};

static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			      unsigned long sector, char *buf)
{
	struct mtdblk_ambdev *ambdev = container_of(dev, struct mtdblk_ambdev, mbd);
	unsigned int logic_b, phys_b;
	loff_t from;
	size_t retlen;

	logic_b = mtd_div_by_eb(sector * 512, dev->mtd);
	phys_b = ambdev->block_map[logic_b];
	from = phys_b * dev->mtd->erasesize + mtd_mod_by_eb(sector * 512, dev->mtd);

	if (mtd_read(dev->mtd, from, 512, &retlen, buf))
		return 1;

	return 0;
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long sector, char *buf)
{
	BUG();
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct mtdblk_ambdev *ambdev;
	unsigned int blocks, logic_b, phys_b;

	ambdev = kzalloc(sizeof(struct mtdblk_ambdev), GFP_KERNEL);
	if (!ambdev)
		return;

	ambdev->mbd.mtd = mtd;
	ambdev->mbd.devnum = mtd->index;

	ambdev->mbd.size = mtd->size / 512;
	ambdev->mbd.tr = tr;
	ambdev->mbd.readonly = 1;

	blocks = mtd_div_by_eb(mtd->size, mtd);
	ambdev->block_map = kzalloc(blocks * sizeof(unsigned int), GFP_KERNEL);
	if (!ambdev->block_map) {
		kfree(ambdev);
		return;
	}

	for (logic_b = 0, phys_b = 0; phys_b < blocks; phys_b++) {
		if (mtd_block_isbad(mtd, phys_b * mtd->erasesize))
			continue;
		ambdev->block_map[logic_b] = phys_b;
		logic_b++;
	}

	ambdev->blocks = logic_b;

	if (add_mtd_blktrans_dev(&ambdev->mbd)) {
		kfree(ambdev->block_map);
		kfree(ambdev);
	}
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
	del_mtd_blktrans_dev(dev);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.blksize 	= 512,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init mtdblock_init(void)
{
	pr_info("Ambarella read-only mtdblock\n");
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit mtdblock_exit(void)
{
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(mtdblock_init);
module_exit(mtdblock_exit);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Simple read-only block device emulation, with bad block skipped");
MODULE_LICENSE("GPL");

