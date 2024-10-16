// SPDX-License-Identifier: GPL-2.0
/*
 *
 *
 * Copyright (C) 2019 liaoweixiong <liaoweixiong@gallwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "nand_panic.h"
#include "nand_lib.h"
#include "nand_osal_for_linux.h"
#include "nand_base.h"
#include <linux/kernel.h>
#include <linux/pstore_blk.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/fs.h>

/* prepare some resources to prevent malloc after panic */
#define PANIC_RES_CNT 2

#define AW_NFTL_MAJOR 93
#define PANIC_INFO(...) printk(KERN_INFO "[NI] " __VA_ARGS__)
#define PANIC_DBG(...) printk(KERN_DEBUG "[ND] " __VA_ARGS__)
#define PANIC_ERR(...) printk(KERN_ERR "[NE] " __VA_ARGS__)

static dma_addr_t panic_dma[PANIC_RES_CNT];
struct dma_buf {
	void *buf;
	size_t size;
};
static struct dma_buf panic_buf[PANIC_RES_CNT];
static struct dma_buf panic_map[PANIC_RES_CNT];
static struct _nftl_blk *g_nftl_blk;
static sector_t g_start_sect;
static sector_t g_sects;

int nand_panic_read(char *buf, sector_t sec_off,  sector_t sects)
{
	struct _nftl_blk *nftl_blk = g_nftl_blk;

	if (!is_panic_enable()) {
		PANIC_ERR("not support panic nand\n");
		return -EBUSY;
	}

	if (!nftl_blk) {
		PANIC_ERR("invalid nftl_blk\n");
		return -EINVAL;
	}

	if (sec_off + sects > g_sects) {
		PANIC_ERR("sectors %llu with start sector %llu is out of range, \n",
			(long long unsigned int)sects, (long long unsigned int)sec_off);
		return -EACCES;
	}

	return panic_read(nftl_blk, sec_off + g_start_sect, sects, (uchar *)buf);
}

int nand_panic_write(const char *buf, sector_t sec_off, sector_t sects)
{
	struct _nftl_blk *nftl_blk = g_nftl_blk;

	if (!is_panic_enable()) {
		PANIC_ERR("not support panic nand\n");
		return -EBUSY;
	}

	if (!nftl_blk) {
		PANIC_ERR("invalid nftl_blk\n");
		return -EINVAL;
	}

	if (sec_off + sects > g_sects) {
		PANIC_ERR("sectors %llu with start sector %llu is out of range, \n",
			(long long unsigned int)sects, (long long unsigned int)sec_off);
		return -EACCES;
	}

	return panic_write(nftl_blk, sec_off + g_start_sect, sects, (uchar *)buf);
}

static int rawnand_panic_alloc_dma_buf(size_t size)
{
	int i;
	char *buf;

	if (aw_ndfc.dev != NULL)
		PANIC_INFO("dev:%p\n", aw_ndfc.dev);

	for (i = 0; i < PANIC_RES_CNT; i++) {
		buf = dma_alloc_coherent(aw_ndfc.dev, size, &panic_dma[i], GFP_KERNEL);
		if (!buf) {
			PANIC_ERR("dma_alloc_coherent for panic_dma[%d] fail\n", i);
			return -ENOMEM;
		}
		panic_buf[i].buf = buf;
		panic_buf[i].size = size;
	}

	return 0;
}

static void rawnand_panic_free_dma_buf(void)
{
	int i;

	for (i = 0; i < PANIC_RES_CNT; i++) {
		kfree(panic_buf[i].buf);
		panic_buf[i].buf = NULL;
		panic_buf[i].size = 0;
	}
}

dma_addr_t nand_panic_dma_map(__u32 rw, void *buf, __u32 len, __u32 panic_res)
{
	if (!is_panic_enable())
		return -EBUSY;

	/* ONLY rawnand allow to use DMA */
	if (get_storage_type() != NAND_STORAGE_TYPE_RAWNAND)
		return -EINVAL;

	if (panic_res >= PANIC_RES_CNT) {
		PANIC_ERR("invalid panic res, %d, should < %d\n", panic_res, PANIC_RES_CNT);
		return -EINVAL;
	}

	if (rw == 1) {
		size_t size = min_t(size_t, len, panic_buf[panic_res].size);
		memcpy(panic_buf[panic_res].buf, buf, size);
	} else {
		panic_map[panic_res].buf = buf;
		panic_map[panic_res].size = (size_t)len;
		/* memset(panic_buf[panic_res].buf, 0, len); */
	}
	return panic_dma[panic_res];
}

void nand_panic_dma_unmap(__u32 rw, dma_addr_t dma_handle, __u32 len)
{
	int i;

	if (!is_panic_enable())
		return;

	/* ONLY rawnand allow to use DMA */
	if (get_storage_type() != NAND_STORAGE_TYPE_RAWNAND)
		return;

	if (rw != 0)
		return;

	for (i = 0; i < PANIC_RES_CNT; i++) {
		if (dma_handle == panic_dma[i]) {
			size_t size = min_t(size_t, panic_map[i].size, panic_buf[i].size);
			memcpy(panic_map[i].buf, panic_buf[i].buf, size);
			break;
		}
	}

	return;
}

int nand_support_panic(void)
{
	panic_mark_enable();
	return 0;
}

int nand_panic_init(struct _nftl_blk *nftl_blk)
{
	int ret;
	struct pstore_blk_info info = {};

	if (!is_panic_enable())
		return -EBUSY;

	PANIC_INFO("panic nand version: %s\n", PANIC_NAND_VERSION);

	/* allocate memory for rawnand dma transfer */
	if (get_storage_type() == NAND_STORAGE_TYPE_RAWNAND) {
		ret = rawnand_panic_alloc_dma_buf(32 * 1024);
		if (ret)
			goto err_out;
	}

	info.major = AW_NFTL_MAJOR;
	info.flags = 0;
	info.panic_write = nand_panic_write;
	ret = register_pstore_blk(&info);
	if (ret)
		goto err_free;
	g_nftl_blk = (void *)nftl_blk;
	g_sects = info.nr_sects;
	g_start_sect = info.start_sect;

	PANIC_INFO("panic nand init ok\n");
	return 0;
err_free:
	rawnand_panic_free_dma_buf();
err_out:
	PANIC_INFO("panic nand init failed\n");
	return ret;
}

int critical_dev_nand_write(const char *dev, size_t sec_off,
		size_t sec_cnt, char *buf)
{
	struct block_device *bdev;
	struct hd_struct *part;
	size_t part_off, part_size;
	int ret = -EACCES;

	if (!is_panic_enable()) {
		PANIC_ERR("not support panic nand\n");
		return -EBUSY;
	}

	if (!g_nftl_blk) {
		PANIC_ERR("invalid nftl_blk\n");
		return -EINVAL;
	}

	bdev = blkdev_get_by_path(dev, FMODE_WRITE, NULL);
	if (IS_ERR(bdev)) {
		PANIC_ERR("get bdev failed - %ld\n", PTR_ERR(bdev));
		return (int)PTR_ERR(bdev);
	}
	part = bdev->bd_part;
	part_off = (size_t)part->start_sect;
	part_size = (size_t)part_nr_sects_read(part);

	if (sec_off > part_size) {
		PANIC_ERR("start sector is out of range\n");
		goto out;
	}

	if (sec_off + sec_cnt > part_size) {
		PANIC_ERR("sectors %ld with start sector %ld is out of range, \n",
				sec_cnt, sec_off);
		goto out;
	}

	ret = panic_write(g_nftl_blk, sec_off + part_off, sec_cnt, buf);
	if (ret)
		goto out;

	ret = 0;
out:
	blkdev_put(bdev, FMODE_WRITE);
	return ret;
}

int critical_dev_nand_read(const char *dev, size_t sec_off,
		size_t sec_cnt, char *buf)
{
	struct block_device *bdev;
	struct hd_struct *part;
	size_t part_off, part_size;
	int ret = -EACCES;

	if (!is_panic_enable()) {
		PANIC_ERR("not support panic nand\n");
		return -EBUSY;
	}

	if (!g_nftl_blk) {
		PANIC_ERR("invalid nftl_blk\n");
		return -EINVAL;
	}

	bdev = blkdev_get_by_path(dev, FMODE_READ, NULL);
	if (IS_ERR(bdev)) {
		PANIC_ERR("get bdev failed - %ld\n", PTR_ERR(bdev));
		return (int)PTR_ERR(bdev);
	}
	part = bdev->bd_part;
	part_off = (size_t)part->start_sect;
	part_size = (size_t)part_nr_sects_read(part);

	if (sec_off > part_size) {
		PANIC_ERR("start sector is out of range\n");
		goto out;
	}

	if (sec_off + sec_cnt > part_size) {
		PANIC_ERR("sectors %ld with start sector %ld is out of range, \n",
				sec_cnt, sec_off);
		goto out;
	}

	ret = panic_read(g_nftl_blk, sec_off + part_off, sec_cnt, buf);
	if (ret)
		goto out;

	ret = 0;
out:
	blkdev_put(bdev, FMODE_READ);
	return ret;
}
