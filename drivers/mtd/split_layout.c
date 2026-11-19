// SPDX-License-Identifier: GPL-2.0
/*
 * The split NOR layout, for kernels without device tree partitions.
 *
 * OpenIPC writes one firmware.bin at the start of the firmware area (0x50000,
 * after a 256K bootloader and its 64K environment): a legacy uImage, then the
 * root squashfs at the next 64 KiB boundary. Nothing records where one ends
 * and the next starts but the images themselves, and on the device tree
 * kernels mtdsplit reads them to divide the "firmware" partition at boot.
 * This kernel takes its NOR partitions from the command line, not the device
 * tree, so this reads the same two headers and registers the partitions
 * mtdsplit would have made: boot, env, firmware (to the end of the chip) and,
 * inside it, kernel, rootfs and rootfs_data, in the order and with the names
 * the device tree kernels give them. Whatever the bootloader passes in
 * mtdparts= describes its own idea of the flash and is ignored, as on those
 * kernels; so is a root= naming an MTD device (init/do_mounts.c).
 *
 * Any other flash contents -- the fixed kernel and rootfs slots above all,
 * whose squashfs sits at the 2 MiB slot boundary rather than right after the
 * kernel -- are not this layout, and the driver registers its partitions as
 * it always did.
 *
 * The same code as the 4.9 kernels' (infinity6, goke-gk710x, hi3516cv100,
 * hi3520dv200), on the 5.10 partition model: add_mtd_partitions() removes
 * what it added when one partition fails, so the table is either all there
 * or not at all, and the partitions hang off master->partitions.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/root_dev.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/split_layout.h>

#define SPLIT_BOOT_SIZE		0x40000ULL
#define SPLIT_ENV_SIZE		0x10000ULL
#define SPLIT_FW_START		(SPLIT_BOOT_SIZE + SPLIT_ENV_SIZE)
#define SPLIT_BLOCK		0x10000ULL

/* where the fixed-slot layout keeps its rootfs: after a 2 MiB kernel slot */
#define FIXED_ROOTFS_START	(SPLIT_FW_START + 0x200000ULL)

#define UIMAGE_HDR_SIZE		64
#define UIMAGE_MAGIC		0x27051956
#define SQUASHFS_MAGIC_LE	0x73717368	/* "hsqs" */
#define SQUASHFS_SB_READ	48		/* through bytes_used */
#define SQUASHFS_SB_SIZE	96		/* struct squashfs_super_block */

static u64 split_round(u64 x)
{
	return (x + SPLIT_BLOCK - 1) & ~(SPLIT_BLOCK - 1);
}

/* len bytes at from, all of them or an error. A corrected bitflip is a read. */
static int split_read(struct mtd_info *mtd, loff_t from, size_t len, u_char *buf)
{
	size_t retlen = 0;
	int err;

	err = mtd_read(mtd, from, len, &retlen, buf);
	if (err == -EUCLEAN)
		err = 0;
	if (!err && retlen != len)
		err = -EIO;
	return err;
}

static u32 be32_at(const u8 *p)
{
	return ((u32)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static u32 le32_at(const u8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24);
}

static u16 le16_at(const u8 *p)
{
	return p[0] | (p[1] << 8);
}

/*
 * The squashfs superblock at off, as squashfs_fill_super() will judge it
 * (version 4.0, a block size of 4K-1M that matches its log) and inside the
 * chip: its bytes_used, 0 when it is none, -errno when reading failed. A
 * bare magic is not enough: the root is mounted from whatever this accepts,
 * and a root= naming the bootloader's partition gives way to it.
 */
static s64 split_squashfs_at(struct mtd_info *mtd, u64 off)
{
	u8 sb[SQUASHFS_SB_READ];
	u32 block_size;
	u16 block_log;
	u64 used;
	int err;

	if (off + SPLIT_BLOCK > mtd->size)
		return 0;
	err = split_read(mtd, off, sizeof(sb), sb);
	if (err)
		return err;
	if (le32_at(sb) != SQUASHFS_MAGIC_LE ||
	    le16_at(sb + 28) != 4 || le16_at(sb + 30) != 0)
		return 0;
	block_size = le32_at(sb + 12);
	block_log = le16_at(sb + 22);
	if (block_log < 12 || block_log > 20 || block_size != 1U << block_log)
		return 0;
	/* bytes_used: 64 bits, so a corrupt high word cannot wrap past the chip */
	used = le32_at(sb + 40) | ((u64)le32_at(sb + 44) << 32);
	if (used < SQUASHFS_SB_SIZE || used > mtd->size - off)
		return 0;
	return used;
}

/*
 * Where the kernel image ends and the rootfs starts, and where the rootfs
 * ends, when the firmware area holds this layout; -ENOENT when it does not.
 */
static int split_find(struct mtd_info *mtd, u64 *rootfs, u64 *data)
{
	u8 buf[UIMAGE_HDR_SIZE];
	s64 used, fixed;
	int err;

	if (mtd->type != MTD_NORFLASH || !mtd->erasesize ||
	    SPLIT_BLOCK % mtd->erasesize ||
	    mtd->size < SPLIT_FW_START + 2 * SPLIT_BLOCK)
		return -ENOENT;

	err = split_read(mtd, SPLIT_FW_START, UIMAGE_HDR_SIZE, buf);
	if (err)
		return err;
	if (be32_at(buf) != UIMAGE_MAGIC || !be32_at(buf + 12))
		return -ENOENT;
	*rootfs = split_round(SPLIT_FW_START + UIMAGE_HDR_SIZE + be32_at(buf + 12));
	if (*rootfs + SPLIT_BLOCK > mtd->size)
		return -ENOENT;

	used = split_squashfs_at(mtd, *rootfs);
	if (used < 0)
		return used;
	if (!used)
		return -ENOENT;

	/*
	 * A squashfs right after the kernel that runs on over the fixed rootfs
	 * slot, where another superblock starts: a fixed-slot image over an
	 * older split one, whose stale squashfs the slot's own has overwritten.
	 * The fixed slots are what the bootloader boots then.
	 */
	if (*rootfs < FIXED_ROOTFS_START &&
	    *rootfs + used > FIXED_ROOTFS_START) {
		fixed = split_squashfs_at(mtd, FIXED_ROOTFS_START);
		if (fixed < 0)
			return fixed;
		if (fixed)
			return -ENOENT;
	}
	*data = split_round(*rootfs + used);
	if (*data > mtd->size)
		return -ENOENT;
	return 0;
}

/*
 * master's partition named name, if add_mtd_device() took it: the root comes
 * from this flash's own "rootfs", not whichever MTD device a global name
 * lookup finds first.
 */
static struct mtd_info *split_part(struct mtd_info *master, const char *name)
{
	struct mtd_info *child, *found = NULL;

	mutex_lock(&master->master.partitions_lock);
	list_for_each_entry(child, &master->partitions, part.node)
		if (!strcmp(child->name, name) &&
		    device_is_registered(&child->dev)) {
			found = child;
			break;
		}
	mutex_unlock(&master->master.partitions_lock);
	return found;
}

int mtd_split_layout_register(struct mtd_info *master)
{
	/* no parsers: mtd_device_parse_register() then takes parts as given */
	static const char *const no_parsers[] = { NULL };
	struct mtd_partition *parts;
	struct mtd_info *root;
	u64 rootfs, data;
	int n = 0, err;

	err = split_find(master, &rootfs, &data);
	if (err == -ENOENT)
		return 1;
	if (err) {
		pr_warn("%s: reading the firmware area failed (%d), "
			"taking the command line's partitions\n", master->name, err);
		return 1;
	}

	parts = kcalloc(6, sizeof(*parts), GFP_KERNEL);
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

	pr_notice("%s: split NOR layout, kernel 0x%llx-0x%llx, "
		  "rootfs 0x%llx-0x%llx, rootfs_data from 0x%llx\n", master->name,
		  SPLIT_FW_START, rootfs, rootfs, data, data);
	err = mtd_device_parse_register(master, no_parsers, NULL, parts, n);
	kfree(parts);
	if (err)
		return err;

	root = split_part(master, "rootfs");
	if (!root) {
		/* add_mtd_partitions() reported success; this is not expected */
		pr_warn("%s: split NOR layout registered without its rootfs\n",
			master->name);
		return 0;
	}
	/* The root is the "rootfs" just registered, as MTD_ROOTFS_ROOT_DEV does it. */
	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, root->index);
	root_dev_from_flash = true;
	pr_notice("split NOR layout: root on mtdblock%d\n", root->index);
	return 0;
}
EXPORT_SYMBOL_GPL(mtd_split_layout_register);
