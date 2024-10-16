/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's firmware loader driver
 * it implement search elf file from boot_package.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/aw-spinand.h>
#include <linux/string.h>
#include <linux/memblock.h>

#include "sunxi_rproc_internal.h"

#define DELAY_TIME				msecs_to_jiffies(500)
#define FW_TRY_CNT				5

#define FIRMWARE_MAGIC				0x89119800

#ifdef FW_SAVE_IN_MEM
struct fw_mem_info {
	const char *name;
	void *addr;
	phys_addr_t pa;
	int len;
	struct list_head list;
};
static LIST_HEAD(g_memory_fw_list);
static DEFINE_MUTEX(g_list_lock);
#endif

struct load_storage {
	const char *name;
	const char *path;
	const int head_off;
	const int offset;
	const u32 flag;
};

#define DUMP_DATA_LEN					128

#define SUNXI_FW_FLAG_EMMC				2
#define SUNXI_FW_FLAG_NOR				3
#define SUNXI_FW_FLAG_NAND				5

static const struct load_storage firmware_storages[] = {
		{ "nor",  "/dev/mtd0", 128 * 512,  128 * 512, SUNXI_FW_FLAG_NOR},
		{ "nand",  "/dev/mtd1", 0,  0, SUNXI_FW_FLAG_NAND},
		{ "emmc",  "/dev/mmcblk0", 32800 * 512,  32800 * 512, SUNXI_FW_FLAG_EMMC},
};

typedef struct firmware_head_info {
	u8   name[16];
	u32  magic;			/* must is 0x89119800 */
	u32  reserved1[3];
	u32  items_nr;
	u32  reserved2[6];
	u32  end;
} firmware_head_info_t;

typedef struct firmware_item_info {
	char name[64];
	u32  data_offset;
	u32  data_len;
	u32  reserved[74];
} firmware_item_info_t;

struct sunxi_firmware_work {
	struct delayed_work work;
	const char *name;
	struct device *device;
	void *context;
	void (*cont)(const struct firmware *fw, void *context);
};

#ifdef DEBUG
static int sunxi_firmware_dump_data(void *buf, int len)
{
	int i, j, line;
	u8 *p = buf;

	pr_info("firmware:\n");

	for (i = 0; i < len; i += 16) {
		j = 0;
		if (len > (i + 16))
			line = 16;
		else
			line = len - i;
		for (j = 0; j < line; j++)
			pr_cont("%02x ", p[i + j]);

		pr_cont("\t\t|");
		for (j = 0; j < line; j++) {
			if (p[i + j] > 32 && p[i + j] < 126)
				pr_cont("%c", p[i + j]);
			else
				pr_cont(".");
		}
		pr_cont("|\n");
	}
	pr_cont("\n");

	return 0;
}
#endif

static int sunxi_firmware_read(const char *path, void *buf, u32 len, loff_t *pos, u32 flag)
{
	int ret;
	struct file *fp;
	mm_segment_t fs;

	fp = filp_open(path, O_RDONLY, 0444);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_read(fp, buf, len, pos);

	filp_close(fp, NULL);

	set_fs(fs);
	return ret;
}

static int sunxi_get_storage_type(void)
{
	int ret, storage_type;
	struct file *fp;
	mm_segment_t fs;
	char buf[1024];
	loff_t pos = 0;
	char *p;
	static const char *path = "/proc/cmdline";

	fp = filp_open(path, O_RDONLY, 0444);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_read(fp, buf, sizeof(buf) - 1, &pos);
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	p = strstr(buf, "boot_type=");
	if (!p) {
		pr_err("Can't find boot_type!\n");
		return -EFAULT;
	}
	/* boot_type range in [0,6]*/
	storage_type = ((int)p[10]) - 0x30;
	if (storage_type < 0 || storage_type > 6) {
		pr_err("Invalid boot_type!\n");
		return -EFAULT;
	}
	pr_debug("storage_type = %d\n", storage_type);

	filp_close(fp, NULL);

	set_fs(fs);
	return storage_type;
}

static int sunxi_firmware_parser_head(struct device *dev, const char *name, void *buf,
				u32 *addr, u32 *len)
{
	u8 *pbuf = buf;
	int i;
	struct firmware_head_info *head;
	struct firmware_item_info *item;

	head = (struct firmware_head_info *)pbuf;
	item = (struct firmware_item_info *)(pbuf + sizeof(*head));

#ifdef DEBUG
	sunxi_firmware_dump_data(pbuf, DUMP_DATA_LEN);
#endif
	for (i = 0; i < head->items_nr; i++, item++) {
		dev_dbg(dev, "item:%s addr=0x%08x,len=0x%x\n", item->name,
						item->data_offset, item->data_len);
		if (strncmp(item->name, name, strlen(item->name)) == 0) {
			*addr = item->data_offset;
			*len = item->data_len;
			return 0;
		}
	}

	return -ENOENT;
}

static int sunxi_find_firmware_storage(void)
{
	struct firmware_head_info *head;
	int i, len, ret;
	loff_t pos;
	const char *path;
	u32 flag;

	len = sizeof(*head);
	head = kmalloc(len, GFP_KERNEL);
	if (!head)
		return -ENOMEM;

	ret = sunxi_get_storage_type();

	for (i = 0; i < ARRAY_SIZE(firmware_storages); i++) {
		path = firmware_storages[i].path;
		pos = firmware_storages[i].head_off;
		flag = firmware_storages[i].flag;

		if (flag != ret)
			continue;

		pr_debug("try to open %s\n", path);

		ret = sunxi_firmware_read(path, head, len, &pos, flag);
		if (ret < 0)
			pr_err("open %s failed,ret=%d\n", path, ret);

		if (ret != len)
			continue;

		if (head->magic == FIRMWARE_MAGIC) {
			kfree(head);
			return i;
		}
	}

	kfree(head);

	return -ENODEV;
}

static int sunxi_firmware_get_info(struct device *dev, int index,
				const char *name, u32 *img_addr, u32 *img_len)
{
	int ret, len;
	u8 *head;
	loff_t pos;
	u32 offset, flag;
	const char *path;

	offset = firmware_storages[index].offset;
	path = firmware_storages[index].path;
	flag = firmware_storages[index].flag;

	len = 4 * 1024;
	head = vmalloc(len);
	if (!head)
		return -ENOMEM;

	pos = firmware_storages[index].head_off;
	ret = sunxi_firmware_read(path, head, len, &pos, flag);
	if (ret != len) {
		dev_err(dev, "%s not exits\n", path);
		ret = -EINVAL;
		goto out;
	}

	ret = sunxi_firmware_parser_head(dev, name, head, img_addr, img_len);
	if (ret < 0) {
		dev_err(dev, "failed to parser head (%s) ret=%d\n", name, ret);
		ret = -EFAULT;
		goto out;
	}

	*img_addr += offset;
	dev_dbg(dev, "firmware: addr:0x%08x len:0x%x\n", *img_addr, *img_len);

out:
	vfree(head);
	return ret;
}

static int sunxi_firmware_get_data(struct device *dev, int index,
				u32 addr, u32 len, struct firmware **fw)
{
	int ret;
	u8 *img;
	const char *path;
	struct firmware *fw_p = NULL;
	loff_t pos;
	u32 flag;

	path = firmware_storages[index].path;
	flag = firmware_storages[index].flag;

	img = vmalloc(len);
	if (!img) {
		dev_err(dev, "failed to alloc Firmware\n");
		ret = -ENOMEM;
		goto out;
	}

	pos = addr;

	/* read data from storage */
	ret = sunxi_firmware_read(path, img, len, &pos, flag);
	if (ret < 0) {
		dev_err(dev, "failed to read firmware data\n");
		ret = -EFAULT;
		goto out;
	}

	fw_p = kmalloc(sizeof(struct firmware), GFP_KERNEL);
	if (!fw_p) {
		dev_err(dev, "failed to alloc Firmware\n");
		ret = -ENOMEM;
		goto out;
	}

	fw_p->size = len;
	fw_p->priv = NULL;
	fw_p->pages = NULL;
	fw_p->data = img;
	*fw = fw_p;
	ret = 0;

#ifdef DEBUG
	sunxi_firmware_dump_data(img, 128);
#endif
out:
	return ret;

}

#ifdef FW_SAVE_IN_MEM
int sunxi_register_memory_fw(const char *name, phys_addr_t addr, int len)
{
	struct fw_mem_info *info;
	void *va;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("Can't kzalloc fw_mem_info.\n");
		return -ENOMEM;
	}

	info->name = name;
	info->pa = addr;
	info->len = len;
	info->addr = phys_to_virt(addr);

	mutex_lock(&g_list_lock);
	list_add(&info->list, &g_memory_fw_list);
	mutex_unlock(&g_list_lock);

	pr_debug("register fw:%s addr=%pad,len=%d,va=0x%p\n",
					name, &addr, len, va);

	return 0;
}
EXPORT_SYMBOL(sunxi_register_memory_fw);

static struct fw_mem_info *sunxi_find_memory_fw(const char *name)
{
	struct fw_mem_info *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_memory_fw_list, list) {
		if (!strcmp(pos->name, name)) {
			mutex_unlock(&g_list_lock);
			return pos;
		}
	}
	mutex_unlock(&g_list_lock);

	return NULL;
}

static int sunxi_unregister_memory_fw(struct fw_mem_info *info)
{
	int ret;

	mutex_lock(&g_list_lock);
	list_del(&info->list);
	mutex_unlock(&g_list_lock);

	pr_debug("remove fw:%s addr=%pad,len=0x%x\n",
			info->name, &info->pa, info->len);
	ret = memblock_free(info->pa, info->len);
	if (ret)
		pr_err("memblock_free failed,ret=%d.\n", ret);
	free_reserved_area(__va(info->pa), __va(info->pa + info->len), -1, NULL);
	kfree(info);
	return 0;
}

static void sunxi_remove_memory_fw(const char *name)
{
	int ret;
	struct fw_mem_info *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_memory_fw_list, list) {
		if (!strcmp(pos->name, name)) {
			pr_debug("remove fw:%s addr=%pad,len=0x%x\n",
					name, &pos->pa, pos->len);

			list_del(&pos->list);
			ret = memblock_free(pos->pa, pos->len);
			if (ret)
				pr_err("memblock_free failed,ret=%d.\n", ret);
			free_reserved_area(__va(pos->pa), __va(pos->pa + pos->len), -1, NULL);
			kfree(pos);
			mutex_unlock(&g_list_lock);
			return;
		}
	}
	mutex_unlock(&g_list_lock);
}

static int sunxi_request_fw_from_mem(const struct firmware **fw,
			   const char *name, struct device *dev)
{
	int ret;
	u8 *img;
	struct firmware *fw_p = NULL;
	struct fw_mem_info *info;

	info = sunxi_find_memory_fw(name);
	if (!info)
		return -ENODEV;

	img = vmalloc(info->len);
	if (!img) {
		sunxi_unregister_memory_fw(info);
		return -ENOMEM;
	}

	fw_p = kmalloc(sizeof(struct firmware), GFP_KERNEL);
	if (!fw_p) {
		dev_err(dev, "failed to alloc Firmware\n");
		vfree(img);
		sunxi_unregister_memory_fw(info);
		return -ENOMEM;
	}

	/* read data from memory */
	memcpy(img, info->addr, info->len);

	fw_p->size = info->len;
	fw_p->priv = NULL;
	fw_p->pages = NULL;
	fw_p->data = img;
	*fw = fw_p;
	ret = 0;

#ifdef DEBUG
	sunxi_firmware_dump_data(img, 128);
#endif
	return ret;
}
#else
int sunxi_register_memory_fw(const char *name, phys_addr_t addr, int len)
{
	pr_info("don't support register memory fw.\n");
	return 0;
}
EXPORT_SYMBOL(sunxi_register_memory_fw);
#endif

int sunxi_request_firmware(const struct firmware **fw, const char *name, struct device *dev)
{
	int ret, index;
	struct firmware *fw_p = NULL;
	u32 img_addr, img_len;

#ifdef FW_SAVE_IN_MEM
	/* try to read fw from memory */
	ret = sunxi_request_fw_from_mem(fw, name, dev);
	if (!ret) {
		dev_dbg(dev, "finded %s in memory.\n", name);
		ret = 0;
		goto out;
	}
	dev_dbg(dev, "Can't find %s in memory.\n", name);
#endif
	ret = sunxi_find_firmware_storage();
	if (ret < 0) {
		dev_warn(dev, "Can't finded boot_package head\n");
		return -ENODEV;
	}

	index = ret;

	ret = sunxi_firmware_get_info(dev, index, name, &img_addr, &img_len);
	if (ret < 0) {
		dev_warn(dev, "failed to read boot_package item\n");
		ret = -EFAULT;
		goto out;
	}

	ret = sunxi_firmware_get_data(dev, index, img_addr, img_len, &fw_p);
	if (ret < 0) {
		dev_err(dev, "failed to read Firmware\n");
		ret = -ENOMEM;
		goto out;
	}

	*fw = fw_p;
out:
	return ret;
}
EXPORT_SYMBOL(sunxi_request_firmware);

static void request_firmware_work_func(struct work_struct *work)
{
	struct sunxi_firmware_work *fw_work;
	const struct firmware *fw;
	int ret;
	static int try;

	fw_work = container_of(to_delayed_work(work), struct sunxi_firmware_work, work);

	try++;
	dev_dbg(fw_work->device, "try to request firmware,try%d\n", try);
	ret = sunxi_request_firmware(&fw, fw_work->name, fw_work->device);

	if (ret < 0 && try < FW_TRY_CNT) {
		schedule_delayed_work(&fw_work->work, DELAY_TIME);
	} else {
		if (ret < 0)
			dev_warn(fw_work->device, "sunxi_request_firmware"
							"failed,ret=%d\n", ret);
		else
			fw_work->cont(fw, fw_work->context);
		put_device(fw_work->device);

		try = 0;

		cancel_delayed_work(&fw_work->work);
#ifdef FW_SAVE_IN_MEM
		if (ret == 0) {
			dev_dbg(fw_work->device, "Success boot fw,remove fw.\n");
			sunxi_remove_memory_fw(fw_work->name);
		}
#endif
		kfree_const(fw_work->name);
		kfree(fw_work);
	}
}

int
sunxi_request_firmware_nowait(
	const char *name, struct device *device, gfp_t gfp, void *context,
	void (*cont)(const struct firmware *fw, void *context))
{
	struct sunxi_firmware_work *fw_work;
	const struct firmware *fw;
	int ret;

#ifdef CONFIG_SUNXI_RPROC_FASTBOOT
	dev_dbg(device, "try to directly request firmware\n");
	ret = sunxi_request_firmware(&fw, name, device);

	if (!ret) {
		cont(fw, context);
		sunxi_remove_memory_fw(name);
		return 0;
	}
#endif
	fw_work = kzalloc(sizeof(*fw_work), gfp);
	if (!fw_work)
		return -ENOMEM;

	fw_work->name = kstrdup_const(name, gfp);
	if (!fw_work->name) {
		kfree(fw_work);
		return -ENOMEM;
	}
	fw_work->device = device;
	fw_work->context = context;
	fw_work->cont = cont;

	get_device(fw_work->device);
	INIT_DELAYED_WORK(&fw_work->work, request_firmware_work_func);
	schedule_delayed_work(&fw_work->work, 0);
	return 0;
}
EXPORT_SYMBOL(sunxi_request_firmware_nowait);

MODULE_DESCRIPTION("SUNXI Remote Firmware Loader Helper");
MODULE_AUTHOR("lijiajian <lijiajian@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
