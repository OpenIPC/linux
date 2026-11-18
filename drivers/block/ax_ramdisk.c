#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/version.h>
#include <linux/of.h>

#define AX_DEVICE_NAME "axramdisk"

static unsigned int axbdrv_ma_no, disk_size;
static char *ramdisk;
static struct gendisk *ax_gd;
static spinlock_t lock;
static unsigned short sector_size = 512;
static struct request_queue *ax_request_queue;

static void ax_request(struct request_queue *q)
{
	struct request *rq;
	int size, res = 0;
	char *ptr;
	unsigned nr_sectors, sector;

	rq = blk_fetch_request(q);
	while (rq) {
		nr_sectors = blk_rq_cur_sectors(rq);
		sector = blk_rq_pos(rq);

		ptr = ramdisk + sector * sector_size;
		size = nr_sectors * sector_size;

		if ((ptr + size) > (ramdisk + disk_size)) {
			pr_err("end of device\n");
			goto done;
		}

		if (rq_data_dir(rq)) {
			memcpy(ptr, bio_data(rq->bio), size);
		} else {
			memcpy(bio_data(rq->bio), ptr, size);
		}
done:
		if (!__blk_end_request_cur(rq, res))
			rq = blk_fetch_request(q);
	}
}

static int ax_ioctl(struct block_device *bdev, fmode_t mode,
		    unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;

	pr_info("cmd=%d\n", cmd);

	switch (cmd) {
	case HDIO_GETGEO:
		pr_info("hit HDIO_GETGEO\n");
		/*
		 * get geometry: we have to fake one...
		 */
		size = disk_size;
		size &= ~0x3f;
		geo.cylinders = size>>6;
		geo.heads = 2;
		geo.sectors = 16;
		geo.start = 4;

		if (copy_to_user((void __user *)arg, &geo, sizeof(geo)))
			return -EFAULT;

		return 0;
	}
	pr_warn("return -ENOTTY\n");

	return -ENOTTY;
}

static const struct block_device_operations axbdrv_fops = {
	.owner = THIS_MODULE,
	.ioctl = ax_ioctl,
};

static int __init ax_ramdisk_init(void)
{
	unsigned long long phy_addr;
	unsigned int diskmb;

	struct device_node * of_node = of_find_compatible_node(NULL, NULL, "axera, ramdisk");
	if (!of_node) {
		printk("%s:can't find ramdisk device tree node\n", __func__);
		return -ENODEV;
	}
	if (of_property_read_u64(of_node, "addr", &phy_addr)) {
		printk("%s:can't find 'addr' in ramdisk device tree node\n", __func__);
		return -ENXIO;
	}
	if (of_property_read_u32(of_node, "size", &disk_size)) {
		pr_err("%s:can't find  'size' in ramdisk device tree node\n", __func__);
		return -ENXIO;
	}
	diskmb = disk_size / 0x100000;

	spin_lock_init(&lock);
	ramdisk = ioremap_cache(phy_addr,disk_size);
	if (!ramdisk)
		return -ENOMEM;

	ax_request_queue = blk_init_queue(ax_request, &lock);
	if (!ax_request_queue) {
		iounmap(ramdisk);
		return -ENOMEM;
	}
	blk_queue_logical_block_size(ax_request_queue, sector_size);

	axbdrv_ma_no = register_blkdev(0, AX_DEVICE_NAME);
	if (axbdrv_ma_no < 0) {
		pr_err("Failed registering axbdrv, returned %d\n",
		       axbdrv_ma_no);
		iounmap(ramdisk);
		return axbdrv_ma_no;
	}

	ax_gd = alloc_disk(16);
	if (!ax_gd) {
		unregister_blkdev(axbdrv_ma_no, AX_DEVICE_NAME);
		iounmap(ramdisk);
		return -ENOMEM;
	}

	ax_gd->major = axbdrv_ma_no;
	ax_gd->first_minor = 0;
	ax_gd->fops = &axbdrv_fops;
	strcpy(ax_gd->disk_name, AX_DEVICE_NAME);
	ax_gd->queue = ax_request_queue;
	set_capacity(ax_gd, disk_size / sector_size);
	add_disk(ax_gd);

	printk("axramdisk device successfully registered, Major No. = %d\n", axbdrv_ma_no);
	printk("Capacity of ram disk is: %d MB\n", diskmb);

	return 0;
}

static void __exit ax_ramdisk_exit(void)
{
	del_gendisk(ax_gd);
	put_disk(ax_gd);
	unregister_blkdev(axbdrv_ma_no, AX_DEVICE_NAME);
	pr_info("module successfully unloaded, Major No. = %d\n", axbdrv_ma_no);
	blk_cleanup_queue(ax_request_queue);
	iounmap(ramdisk);
}

module_init(ax_ramdisk_init);
module_exit(ax_ramdisk_exit);

MODULE_LICENSE("GPL v2");
