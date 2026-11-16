// SPDX-License-Identifier: GPL-2.0
/*
 * The OpenIPC split NOR layout.
 *
 * OpenIPC writes one firmware.bin at the start of the firmware area (0x50000,
 * after a 256K bootloader and its 64K environment): a legacy uImage, then the
 * root squashfs at the next 64 KiB boundary. Nothing records where one ends
 * and the next starts but the images themselves, so this reads the two
 * headers and registers boot, env, firmware (to the end of the chip) and,
 * inside it, kernel, rootfs and rootfs_data -- the partitions, names and
 * order the OpenIPC 4.9 and older kernels give the same image. Whatever the
 * bootloader passes in mtdparts= describes its own idea of the flash and is
 * not used then; neither is a root= naming an MTD device (init/do_mounts.c).
 *
 * Any other flash contents -- the fixed kernel and rootfs slots above all,
 * whose squashfs sits at the 2 MiB slot boundary rather than right after the
 * kernel -- are not this layout, and the flash is partitioned as it always
 * was (mtd_device_parse_register()).
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/unaligned.h>
#include <linux/root_dev.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/split_layout.h>

#define SPLIT_BOOT_SIZE		0x40000ULL
#define SPLIT_ENV_SIZE		0x10000ULL
#define SPLIT_FW_START		(SPLIT_BOOT_SIZE + SPLIT_ENV_SIZE)
#define SPLIT_BLOCK		0x10000ULL

#define UIMAGE_HDR_SIZE		64
#define UIMAGE_MAGIC		0x27051956
#define SQUASHFS_MAGIC_LE	0x73717368	/* "hsqs" */
#define SQUASHFS_SB_READ	48		/* through bytes_used */

#define SPLIT_MAX_PARTS		6

static u64 split_round(u64 x)
{
	return (x + SPLIT_BLOCK - 1) & ~(SPLIT_BLOCK - 1);
}

/* len bytes at from, all of them or an error. A corrected bitflip is a read. */
static int split_read(struct mtd_info *mtd, loff_t from, size_t len, u8 *buf)
{
	size_t retlen = 0;
	int err;

	err = mtd_read(mtd, from, len, &retlen, buf);
	if (mtd_is_bitflip(err))
		err = 0;
	if (!err && retlen != len)
		err = -EIO;
	return err;
}

/*
 * Where the kernel image ends and the rootfs starts, and where the rootfs
 * ends, when the firmware area holds this layout; -ENOENT when it does not.
 */
static int split_find(struct mtd_info *mtd, u64 *rootfs, u64 *data)
{
	u8 buf[UIMAGE_HDR_SIZE];
	u64 used;
	int err;

	if (mtd->type != MTD_NORFLASH || !mtd->erasesize ||
	    SPLIT_BLOCK % mtd->erasesize ||
	    mtd->size < SPLIT_FW_START + 2 * SPLIT_BLOCK)
		return -ENOENT;

	err = split_read(mtd, SPLIT_FW_START, UIMAGE_HDR_SIZE, buf);
	if (err)
		return err;
	if (get_unaligned_be32(buf) != UIMAGE_MAGIC ||
	    !get_unaligned_be32(buf + 12))
		return -ENOENT;
	/*
	 * The squashfs right after the uImage. The fixed slots put theirs at
	 * 0x250000, so a fixed-slot uImage ending in its slot's last 64 KiB
	 * would look the same; the build never ships one (the Makefile holds
	 * a fixed-slot uImage to 1983K of its 2048K slot), and sysupgrade
	 * tells the layouts apart the same way. Were it ever flashed, kernel
	 * and rootfs here are where the fixed slots have them, and only
	 * rootfs_data starts at the squashfs end rather than at 0x750000.
	 */
	*rootfs = split_round(SPLIT_FW_START + UIMAGE_HDR_SIZE +
			      get_unaligned_be32(buf + 12));
	if (*rootfs + SPLIT_BLOCK > mtd->size)
		return -ENOENT;

	err = split_read(mtd, *rootfs, SQUASHFS_SB_READ, buf);
	if (err)
		return err;
	if (get_unaligned_le32(buf) != SQUASHFS_MAGIC_LE)
		return -ENOENT;
	/* bytes_used: 64 bits, so a corrupt high word cannot wrap past the chip */
	used = get_unaligned_le64(buf + 40);
	if (!used || used > mtd->size - *rootfs)
		return -ENOENT;
	*data = split_round(*rootfs + used);
	if (*data > mtd->size)
		return -ENOENT;
	return 0;
}

int mtd_split_layout_parts(struct mtd_info *master,
			   struct mtd_partition **pparts)
{
	struct mtd_partition *parts;
	u64 rootfs, data;
	int n = 0, err;

	/* the boot flash only: no other flash's contents choose the root */
	if (strcmp(master->name, CONFIG_MTD_SPLIT_LAYOUT_NAME))
		return 0;

	err = split_find(master, &rootfs, &data);
	if (err == -ENOENT)
		return 0;
	if (err) {
		pr_warn("%s: reading the firmware area failed (%d), not taking the split layout\n",
			master->name, err);
		return 0;
	}

	parts = kcalloc(SPLIT_MAX_PARTS, sizeof(*parts), GFP_KERNEL);
	if (!parts)
		return -ENOMEM;
	parts[n].name = "boot";
	parts[n].offset = 0;
	parts[n++].size = SPLIT_BOOT_SIZE;
	parts[n].name = "env";
	parts[n].offset = SPLIT_BOOT_SIZE;
	parts[n++].size = SPLIT_ENV_SIZE;
	parts[n].name = "firmware";
	parts[n].offset = SPLIT_FW_START;
	parts[n++].size = master->size - SPLIT_FW_START;
	parts[n].name = "kernel";
	parts[n].offset = SPLIT_FW_START;
	parts[n++].size = rootfs - SPLIT_FW_START;
	parts[n].name = "rootfs";
	parts[n].offset = rootfs;
	parts[n++].size = data - rootfs;
	/* a squashfs that fills the chip leaves no overlay, as mtdsplit does */
	if (data < master->size) {
		parts[n].name = "rootfs_data";
		parts[n].offset = data;
		parts[n++].size = master->size - data;
	}

	pr_notice("%s: split NOR layout, kernel 0x%llx-0x%llx, rootfs 0x%llx-0x%llx, rootfs_data from 0x%llx\n",
		  master->name, SPLIT_FW_START, rootfs, rootfs, data, data);
	*pparts = parts;
	return n;
}

void mtd_split_layout_set_root(struct mtd_info *master)
{
	struct mtd_info *child, *root = NULL;

	mutex_lock(&master->master.partitions_lock);
	list_for_each_entry(child, &master->partitions, part.node) {
		if (!strcmp(child->name, "rootfs")) {
			root = child;
			break;
		}
	}
	mutex_unlock(&master->master.partitions_lock);

	/*
	 * Only the flash named CONFIG_MTD_SPLIT_LAYOUT_NAME gets here. Should two
	 * chips share the name (Hi3520DV200 boards describe both chip selects,
	 * and only the fitted one probes), the first keeps the root.
	 */
	if (!root || root_dev_from_flash)
		return;
	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, root->index);
	root_dev_from_flash = true;
	pr_notice("split NOR layout: root on mtdblock%d\n", root->index);
}
