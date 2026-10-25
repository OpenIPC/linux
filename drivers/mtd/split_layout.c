/*
 * The split NOR layout, for kernels without a device tree.
 *
 * OpenIPC writes one firmware.bin at the start of the firmware area (0x50000,
 * after a 256K bootloader and its 64K environment): a legacy uImage, then the
 * root squashfs at the next 64 KiB boundary. Nothing records where one ends
 * and the next starts but the images themselves, and on the device tree
 * kernels mtdsplit reads them to divide the "firmware" partition at boot.
 * Without a device tree there is nothing to hang mtdsplit on, so this reads
 * the same two headers and registers the partitions mtdsplit would have
 * made: boot, env, firmware (to the end of the chip) and, inside it, kernel,
 * rootfs and rootfs_data, in the order and with the names the device tree
 * kernels give them. Whatever the bootloader passes in mtdparts= describes
 * its own idea of the flash and is ignored, as on those kernels; so is a
 * root= naming an MTD device (init/do_mounts.c).
 *
 * Any other flash contents -- the fixed kernel and rootfs slots above all,
 * whose squashfs sits at the 2 MiB slot boundary rather than right after the
 * kernel -- are not this layout, and the driver registers its partitions as
 * it always did.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>
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

static u64 split_round(u64 x)
{
	return (x + SPLIT_BLOCK - 1) & ~(SPLIT_BLOCK - 1);
}

/* len bytes at from, all of them or an error. A corrected bitflip is a read. */
static int split_read(struct mtd_info *mtd, loff_t from, size_t len, u_char *buf)
{
	size_t retlen = 0;
	int err;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	err = mtd_read(mtd, from, len, &retlen, buf);
#else
	err = mtd->read(mtd, from, len, &retlen, buf);
#endif
	if (err == -EUCLEAN)
		err = 0;
	if (!err && retlen != len)
		err = -EIO;
	return err;
}

static u32 be32_at(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static u32 le32_at(const u8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24);
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
	if (be32_at(buf) != UIMAGE_MAGIC || !be32_at(buf + 12))
		return -ENOENT;
	*rootfs = split_round(SPLIT_FW_START + UIMAGE_HDR_SIZE + be32_at(buf + 12));
	if (*rootfs + SPLIT_BLOCK > mtd->size)
		return -ENOENT;

	err = split_read(mtd, *rootfs, SQUASHFS_SB_READ, buf);
	if (err)
		return err;
	if (le32_at(buf) != SQUASHFS_MAGIC_LE)
		return -ENOENT;
	/* bytes_used: 64 bits, so a corrupt high word cannot wrap past the chip */
	used = le32_at(buf + 40) | ((u64)le32_at(buf + 44) << 32);
	if (!used || used > mtd->size - *rootfs)
		return -ENOENT;
	*data = split_round(*rootfs + used);
	if (*data > mtd->size)
		return -ENOENT;
	return 0;
}

/* The root is the "rootfs" just registered, as MTD_ROOTFS_ROOT_DEV does it. */
static void split_set_root(struct mtd_info *r)
{
	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, r->index);
	root_dev_from_flash = true;
	printk(KERN_NOTICE "split NOR layout: root on mtdblock%d\n", r->index);
}

int mtd_split_layout_register(struct mtd_info *master)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	/* no parsers: mtd_device_parse_register() then takes parts as given */
	static const char *no_parsers[] = { NULL };
#endif
	struct mtd_partition *parts;
	u64 rootfs, data;
	int n = 0, i = 0, err;

	err = split_find(master, &rootfs, &data);
	if (err == -ENOENT)
		return 1;
	if (err) {
		printk(KERN_WARNING "%s: reading the firmware area failed (%d), "
		       "taking the command line's partitions\n", master->name, err);
		return 1;
	}

	parts = kzalloc(6 * sizeof(*parts), GFP_KERNEL);
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

	printk(KERN_NOTICE "%s: split NOR layout, kernel 0x%llx-0x%llx, "
	       "rootfs 0x%llx-0x%llx, rootfs_data from 0x%llx\n", master->name,
	       SPLIT_FW_START, rootfs, rootfs, data, data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	err = mtd_device_parse_register(master, no_parsers, NULL, parts, n);
#else
	err = mtd_device_register(master, parts, n);
#endif
	if (err && mtd_split_del_parts(master))
		err = -EBUSY;
	/*
	 * add_mtd_partitions() goes on past a partition whose device could not
	 * be added, and still reports success: take the table as registered
	 * only when every partition of it is there, on this flash.
	 */
	while (!err && i < n) {
		if (!mtd_split_part(master, parts[i].name))
			err = -ENODEV;
		i++;
	}
	kfree(parts);
	if (err == -ENODEV && mtd_split_del_parts(master))
		err = -EBUSY;
	/* none of the table left, or -EBUSY: some of it still on master */
	if (err)
		return err;
	split_set_root(mtd_split_part(master, "rootfs"));
	return 0;
}
EXPORT_SYMBOL_GPL(mtd_split_layout_register);
