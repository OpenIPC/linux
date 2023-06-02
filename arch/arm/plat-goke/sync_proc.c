/*
 * arch/arm/mach-gk/sync_proc.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/atomic.h>

#include <plat/syncproc.h>

#include <mach/hardware.h>

#include <mach/hardware.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE


int gk_sync_proc_hinit(struct gk_sync_proc_hinfo *hinfo)
{
	hinfo->maxid = GK_SYNC_PROC_MAX_ID;
	init_waitqueue_head(&hinfo->sync_proc_head);
	atomic_set(&hinfo->sync_proc_flag, 0);
	idr_init(&hinfo->sync_proc_idr);
	mutex_init(&hinfo->sync_proc_lock);
	hinfo->sync_read_proc = NULL;

	return 0;
}
EXPORT_SYMBOL(gk_sync_proc_hinit);

int gk_sync_proc_open(struct inode *inode, struct file *file)
{
	int				retval = 0;
	struct gk_sync_proc_pinfo	*pinfo = file->private_data;
	struct proc_dir_entry		*dp;
	struct gk_sync_proc_hinfo	*hinfo;
	int				id;
	dp = PDE(inode);
	hinfo = (struct gk_sync_proc_hinfo *)dp->data;
	if (!hinfo) {
		retval = -EPERM;
		goto gk_sync_proc_open_exit;
	}
	if (hinfo->maxid > GK_SYNC_PROC_MAX_ID) {
		retval = -EPERM;
		goto gk_sync_proc_open_exit;
	}

	if (pinfo) {
		retval = -EPERM;
		goto gk_sync_proc_open_exit;
	}
	pinfo = kmalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		retval = -ENOMEM;
		goto gk_sync_proc_open_exit;
	}
	memset(pinfo, 0, sizeof(*pinfo));

	if (idr_pre_get(&hinfo->sync_proc_idr, GFP_KERNEL) == 0) {
		retval = -ENOMEM;
		goto gk_sync_proc_open_kfree_p;
	}
	mutex_lock(&hinfo->sync_proc_lock);
	retval = idr_get_new_above(&hinfo->sync_proc_idr, pinfo, 0, &id);
	mutex_unlock(&hinfo->sync_proc_lock);
	if (retval != 0)
		goto gk_sync_proc_open_kfree_p;
	if (id > 31) {
		retval = -ENOMEM;
		goto gk_sync_proc_open_remove_id;
	}

	if (!(pinfo->page = (char*) __get_free_page(GFP_KERNEL))) {
		retval = -ENOMEM;
		goto gk_sync_proc_open_remove_id;
	}
	pinfo->id = id;
	pinfo->mask = (0x01 << id);

	file->private_data = pinfo;
	file->f_version = 0;
	file->f_mode &= ~FMODE_PWRITE;

	goto gk_sync_proc_open_exit;

gk_sync_proc_open_remove_id:
	mutex_lock(&hinfo->sync_proc_lock);
	idr_remove(&hinfo->sync_proc_idr, id);
	mutex_unlock(&hinfo->sync_proc_lock);

gk_sync_proc_open_kfree_p:
	kfree(pinfo);

gk_sync_proc_open_exit:
	return retval;
}
EXPORT_SYMBOL(gk_sync_proc_open);

int gk_sync_proc_release(struct inode *inode, struct file *file)
{
	int				retval = 0;
	struct gk_sync_proc_pinfo	*pinfo = file->private_data;
	struct proc_dir_entry		*dp;
	struct gk_sync_proc_hinfo	*hinfo;
	dp = PDE(inode);

	hinfo = (struct gk_sync_proc_hinfo *)dp->data;
	if (!hinfo) {
		retval = -EPERM;
		goto gk_sync_proc_release_exit;
	}

	if (!pinfo) {
		retval = -ENOMEM;
		goto gk_sync_proc_release_exit;
	}

	mutex_lock(&hinfo->sync_proc_lock);
	idr_remove(&hinfo->sync_proc_idr, pinfo->id);
	mutex_unlock(&hinfo->sync_proc_lock);

	free_page((unsigned long)pinfo->page);
	kfree(pinfo);
	file->private_data = NULL;

gk_sync_proc_release_exit:
	return retval;
}
EXPORT_SYMBOL(gk_sync_proc_release);

/* Note: ignore ppos*/
ssize_t gk_sync_proc_read(struct file *file, char __user *buf,
	size_t size, loff_t *ppos)
{
	int				retval = 0;
	struct gk_sync_proc_pinfo	*pinfo = file->private_data;
	struct proc_dir_entry		*dp;
	struct gk_sync_proc_hinfo	*hinfo;
	struct inode			*inode = file->f_path.dentry->d_inode;
	char				*start;
	int				len;
	size_t				count;
	dp = PDE(inode);
	hinfo = (struct gk_sync_proc_hinfo *)dp->data;
	if (!hinfo) {
		retval = -EPERM;
		goto gk_sync_proc_read_exit;
	}
	if (!hinfo->sync_read_proc) {
		retval = -EPERM;
		goto gk_sync_proc_read_exit;
	}

	if (!pinfo) {
		retval = -ENOMEM;
		goto gk_sync_proc_read_exit;
	}

	count = min_t(size_t, GK_SYNC_PROC_PAGE_SIZE, size);
	start = pinfo->page;
	len = 0;
	while (1) {
		wait_event_interruptible(hinfo->sync_proc_head,
			(atomic_read(&hinfo->sync_proc_flag) & pinfo->mask));
		atomic_clear_mask(pinfo->mask,
			(unsigned long *)&hinfo->sync_proc_flag);

		len = hinfo->sync_read_proc(start, hinfo->sync_read_data);
		if (len < count) {
			start += len;
			count -= len;
		} else if (len == count) {
			start += len;
			count -= len;
			break;
		} else {
			break;
		}
	}
	len = start - pinfo->page;
	if (len == 0) {
		retval = -EFAULT;
	} else {
		if (copy_to_user(buf, pinfo->page, len)) {
			retval = -EFAULT;
		} else {
			retval = len;
		}
	}

gk_sync_proc_read_exit:
	return retval;
}
EXPORT_SYMBOL(gk_sync_proc_read);

ssize_t gk_sync_proc_write(struct file *file, const char __user *buf,
	size_t size, loff_t *ppos)
{
	int				retval = 0;
	struct proc_dir_entry		*dp;
	struct gk_sync_proc_hinfo	*hinfo;
	struct inode			*inode = file->f_path.dentry->d_inode;

	dp = PDE(inode);
	hinfo = (struct gk_sync_proc_hinfo *)dp->data;

	if (!hinfo) {
		retval = -EPERM;
		goto gk_sync_proc_write_exit;
	}

	atomic_set(&hinfo->sync_proc_flag, 0xFFFFFFFF);
	wake_up_all(&hinfo->sync_proc_head);

	retval = size;

gk_sync_proc_write_exit:
	return retval;
}
EXPORT_SYMBOL(gk_sync_proc_write);

#define GET_PROC_DATA_FROM_FILEP(filp)	(struct gk_async_proc_info *)(PDE(filp->f_path.dentry->d_inode)->data)

static int gk_async_proc_open(struct inode *inode, struct file *filp)
{
	struct gk_async_proc_info	*pinfo;

	pinfo = GET_PROC_DATA_FROM_FILEP(filp);

	mutex_lock(&pinfo->op_mutex);
	pinfo->use_count++;
	filp->f_op = &pinfo->fops;
	filp->private_data = pinfo->private_data;
	mutex_unlock(&pinfo->op_mutex);

	return 0;
}

static int gk_async_proc_fasync(int fd, struct file * filp, int on)
{
	int				retval;
	struct gk_async_proc_info	*pinfo;

	pinfo = GET_PROC_DATA_FROM_FILEP(filp);

	mutex_lock(&pinfo->op_mutex);
	retval = fasync_helper(fd, filp, on, &pinfo->fasync_queue);
	mutex_unlock(&pinfo->op_mutex);

	return retval;
}

static int gk_async_proc_release(struct inode *inode, struct file *filp)
{
	int				retval;
	struct gk_async_proc_info	*pinfo;

	pinfo = GET_PROC_DATA_FROM_FILEP(filp);

	mutex_lock(&pinfo->op_mutex);
	retval = fasync_helper(-1, filp, 0, &pinfo->fasync_queue);
	pinfo->use_count--;
	mutex_unlock(&pinfo->op_mutex);

	return retval;
}

int gk_async_proc_create(struct gk_async_proc_info *pinfo)
{
	int				retval = 0;
	struct proc_dir_entry		*entry;

	if (!pinfo) {
		retval = -EINVAL;
		goto gk_async_proc_create_exit;
	}

	pinfo->fops.open = gk_async_proc_open;
	pinfo->fops.fasync = gk_async_proc_fasync;
	pinfo->fops.release = gk_async_proc_release;
	mutex_init(&pinfo->op_mutex);
	pinfo->use_count = 0;
	pinfo->fasync_queue = NULL;

	entry = proc_create_data(pinfo->proc_name, S_IRUGO,
		get_gk_proc_dir(), &pinfo->fops, pinfo);
	if (!entry) {
		retval = -EINVAL;
	}

gk_async_proc_create_exit:
	return retval;
}
EXPORT_SYMBOL(gk_async_proc_create);

int gk_async_proc_remove(struct gk_async_proc_info *pinfo)
{
	int				retval = 0;

	if (!pinfo) {
		retval = -EINVAL;
	} else {
		remove_proc_entry(pinfo->proc_name, get_gk_proc_dir());
	}

	return retval;
}
EXPORT_SYMBOL(gk_async_proc_remove);

