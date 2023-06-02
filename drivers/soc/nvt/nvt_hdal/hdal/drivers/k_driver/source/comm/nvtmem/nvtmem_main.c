/**
    NVT hdal memory operation handling
    Add a wrapper to handle the fmem memory handling api and version management API
    @file       nvtmem.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <asm/system_info.h>
#include <mach/fmem.h>
#include <comm/nvtmem.h>

static DEFINE_SEMAPHORE(sema_ver);
static struct list_head 	ver_list_root;
static struct kmem_cache*	ver_cache;

/* proc function
 */
static struct proc_dir_entry *nvtmem_proc_root = NULL;
static struct proc_dir_entry *nvtmem_proc_ver = NULL;

void* nvtmem_alloc_buffer(struct nvt_fmem_mem_info_t *nvt_fmem_info)
{
	/* Allocate from first cma area */
	return fmem_alloc_from_cma(nvt_fmem_info, 0);
}
EXPORT_SYMBOL(nvtmem_alloc_buffer);

int nvtmem_release_buffer(void *handle)
{
	/* Allocate from first cma area */
	return fmem_release_from_cma(handle, 0);
}
EXPORT_SYMBOL(nvtmem_release_buffer);

phys_addr_t nvtmem_va_to_pa(unsigned int vaddr)
{
	return fmem_lookup_pa(vaddr);
}
EXPORT_SYMBOL(nvtmem_va_to_pa);

static struct nvtmem_ver_info_t* nvtmem_get_ver_info(struct platform_device *pdev, unsigned int id)
{
	struct nvtmem_ver_info_t *curr_info, *next_info;

	down(&sema_ver);
	list_for_each_entry_safe(curr_info, next_info, &ver_list_root, list) {
        if ((unsigned long)curr_info->pdev == (unsigned long)pdev && curr_info->id == id) {
			up(&sema_ver);
			return curr_info;
		}
	}
	up(&sema_ver);

	return NULL;
}

static struct nvtmem_ver_info_t* nvtmem_ver_info_alloc(struct platform_device *pdev)
{
	struct nvtmem_ver_info_t* ver_info = NULL;

		/* To get cma info by id */
	ver_info = nvtmem_get_ver_info(pdev, pdev->id);
	if (ver_info != NULL) {
		pr_err("nvtmem/%s: This device %s is already added\n", __func__, pdev->name);
		return NULL;
	}

	/* Allocate a buffer to store version info */
	ver_info = (struct nvtmem_ver_info_t *)kmem_cache_zalloc(ver_cache, GFP_KERNEL);
	if (!ver_info) {
		pr_err("nvtmem/%s: This device %s can't allocate buffer\n", __func__, pdev->name);
		return NULL;
	}

	/* Basic init */
	INIT_LIST_HEAD(&ver_info->list);
	ver_info->pdev = pdev;
	ver_info->id = pdev->id;

	return ver_info;
}

static int nvtmem_add_ver_info(struct platform_device *pdev, struct nvtmem_ver_info_t* ver_info, char *ver)
{
	int ret = 0;
	if (ver_info == NULL) {
		dev_err(&pdev->dev, "nvtmem/%s: We can't add this ver_info because this is NULL\n", __func__);
		return -1;
	}

	down(&sema_ver);
	/* To transform version to info struct */
	ret = sscanf(ver, "%u.%u.%u.%u", &ver_info->version,
					&ver_info->patch_level,
					&ver_info->sub_level,
					&ver_info->extra_ver);
	if (ret != 4) {
		kmem_cache_free(ver_cache, ver_info);
		up(&sema_ver);
		return -1;
	}

	list_add_tail(&ver_info->list, &ver_list_root);
	up(&sema_ver);

	return 0;
}

static int nvtmem_ver_info_free(struct platform_device *pdev)
{
	struct nvtmem_ver_info_t *curr_info, *next_info;

	down(&sema_ver);
	list_for_each_entry_safe(curr_info, next_info, &ver_list_root, list) {
		if ((unsigned long)curr_info->pdev == (unsigned long)pdev && curr_info->id == pdev->id) {
			list_del(&curr_info->list);
			memset(curr_info, 0, sizeof(struct nvtmem_ver_info_t));
			kmem_cache_free(ver_cache, curr_info);
			up(&sema_ver);
			return 0;
		}
	}
	up(&sema_ver);

	dev_err(&pdev->dev, "nvtmem/%s: We can't free this device %s \n", __func__, pdev->name);
	return -1;
}

int nvtmem_version_register(struct platform_device *pdev, char *version)
{
	int ret = 0;
	struct nvtmem_ver_info_t *ver_info = NULL;

	if (version == NULL || pdev == NULL)
		return -1;

	ver_info = nvtmem_ver_info_alloc(pdev);
	if (ver_info == NULL)
		return -ENOMEM;

	/* It will release this buffer if add failed */
	ret = nvtmem_add_ver_info(pdev, ver_info, version);
	if (ret < 0) {
		dev_err(&pdev->dev, "nvtmem/%s: Error to add version with %s\n", __func__, version);
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(nvtmem_version_register);

int nvtmem_version_unregister(struct platform_device *pdev)
{
	int ret = 0;

	if (pdev == NULL)
		return -1;

	ret = nvtmem_ver_info_free(pdev);
	if (ret < 0)
		return -1;

	return 0;
}
EXPORT_SYMBOL(nvtmem_version_unregister);

static void nvtmem_dump_version(void)
{
	struct nvtmem_ver_info_t *curr_info, *next_info;

	pr_info("NVTMEM: %s\n", NVT_HDAL_MEM_VERSION);
	down(&sema_ver);
	list_for_each_entry_safe(curr_info, next_info, &ver_list_root, list) {
        pr_info("Device %s id %d: %u.%u.%u.%u\n", curr_info->pdev->name,
						curr_info->id,
						curr_info->version,
						curr_info->patch_level,
						curr_info->sub_level,
						curr_info->extra_ver);
	}
	up(&sema_ver);
}

/*
 *	proc init
 */

static int version_proc_show(struct seq_file *sfile, void *v)
{
	struct nvtmem_ver_info_t *curr_info, *next_info;
	seq_printf(sfile, "Version: %s\n", NVT_HDAL_MEM_VERSION);

	down(&sema_ver);
	list_for_each_entry_safe(curr_info, next_info, &ver_list_root, list) {
        seq_printf(sfile, "Device %s id %d: %u.%u.%u.%u\n", curr_info->pdev->name,
							curr_info->id,
							curr_info->version,
							curr_info->patch_level,
							curr_info->sub_level,
							curr_info->extra_ver);
	}
	up(&sema_ver);
	return 0;
}
static int version_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_proc_show, NULL);
}

static const struct file_operations version_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= version_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init nvtmem_proc_init(void)
{
	struct proc_dir_entry *p;

	p = proc_mkdir("nvtmem", NULL);
	if (p == NULL) {
		pr_err("%s, fail! \n", __func__);
		return -1;
	}

	nvtmem_proc_root = p;

	/* Create version proc node
	*/
	nvtmem_proc_ver = proc_create("version", S_IRUGO, nvtmem_proc_root, &version_proc_fops);
	if (nvtmem_proc_ver == NULL)
		panic("nvtmem/%s: Fail to create proc resolve_proc!\n", __func__);

	return 0;
}

static int __init nvtmem_proc_deinit(void)
{
	/* Remove version proc node
	*/
	proc_remove(nvtmem_proc_ver);

	return 0;
}


static int nvtmem_ver_sys_init(void)
{
	INIT_LIST_HEAD(&ver_list_root);
	ver_cache = kmem_cache_create("nvtmem_ver", sizeof(struct nvtmem_ver_info_t), 0, SLAB_PANIC, NULL);
	if (!ver_cache) {
		pr_err("%s: Cache create memory failed\n", __func__);
		return -1;
	}

	return 0;
}

static void nvtmem_ver_sys_deinit(void)
{
	kmem_cache_destroy(ver_cache);

	return;
}

static int __init nvtmem_init(void)
{
	int ret = 0;

	ret = nvtmem_ver_sys_init();
	if (ret < 0)
		return -1;

	ret = nvtmem_proc_init();
	if (ret < 0)
		return -1;

	nvtmem_dump_version();
	return 0;
}

static void __exit nvtmem_exit(void)
{
	nvtmem_ver_sys_deinit();
	nvtmem_proc_deinit();
	return;
}

module_init(nvtmem_init);
module_exit(nvtmem_exit);

MODULE_AUTHOR("Novatek");
MODULE_DESCRIPTION("NVT Memory Library");
MODULE_LICENSE("Proprietary");