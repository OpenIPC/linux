/*
 * arch/arm/plat-ambarella/misc/sync_proc.c
 *
 * Author: Anthony Ginger, <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/export.h>

#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/atomic.h>

#include <plat/ambsyncproc.h>

int ambsync_proc_hinit(struct ambsync_proc_hinfo *hinfo)
{
	hinfo->maxid = AMBA_SYNC_PROC_MAX_ID;
	init_waitqueue_head(&hinfo->sync_proc_head);
	atomic_set(&hinfo->sync_proc_flag, 0);
	idr_init(&hinfo->sync_proc_idr);
	mutex_init(&hinfo->sync_proc_lock);
	hinfo->sync_read_proc = NULL;

	return 0;
}
EXPORT_SYMBOL(ambsync_proc_hinit);

int ambsync_proc_open(struct inode *inode, struct file *file)
{
	int retval = 0;
	struct ambsync_proc_pinfo *pinfo = file->private_data;
	struct ambsync_proc_hinfo *hinfo;
	int id;

	hinfo = (struct ambsync_proc_hinfo *)PDE_DATA(inode);
	if (!hinfo) {
		retval = -EPERM;
		goto ambsync_proc_open_exit;
	}
	if (hinfo->maxid > AMBA_SYNC_PROC_MAX_ID) {
		retval = -EPERM;
		goto ambsync_proc_open_exit;
	}

	if (pinfo) {
		retval = -EPERM;
		goto ambsync_proc_open_exit;
	}
	pinfo = kmalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		retval = -ENOMEM;
		goto ambsync_proc_open_exit;
	}
	memset(pinfo, 0, sizeof(*pinfo));

	mutex_lock(&hinfo->sync_proc_lock);
	id = idr_alloc(&hinfo->sync_proc_idr, pinfo, 0, 0, GFP_KERNEL);
	mutex_unlock(&hinfo->sync_proc_lock);
	if (id < 0) {
		retval = id;
		goto ambsync_proc_open_kfree_p;
	}
	if (id > 31) {
		retval = -ENOMEM;
		goto ambsync_proc_open_remove_id;
	}

	if (!(pinfo->page = (char*) __get_free_page(GFP_KERNEL))) {
		retval = -ENOMEM;
		goto ambsync_proc_open_remove_id;
	}
	pinfo->id = id;
	pinfo->mask = (0x01 << id);

	file->private_data = pinfo;
	file->f_version = 0;
	file->f_mode &= ~FMODE_PWRITE;

	goto ambsync_proc_open_exit;

ambsync_proc_open_remove_id:
	mutex_lock(&hinfo->sync_proc_lock);
	idr_remove(&hinfo->sync_proc_idr, id);
	mutex_unlock(&hinfo->sync_proc_lock);

ambsync_proc_open_kfree_p:
	kfree(pinfo);

ambsync_proc_open_exit:
	return retval;
}
EXPORT_SYMBOL(ambsync_proc_open);

int ambsync_proc_release(struct inode *inode, struct file *file)
{
	int retval = 0;
	struct ambsync_proc_pinfo *pinfo = file->private_data;
	struct ambsync_proc_hinfo *hinfo;

	hinfo = (struct ambsync_proc_hinfo *)PDE_DATA(inode);
	if (!hinfo) {
		retval = -EPERM;
		goto ambsync_proc_release_exit;
	}

	if (!pinfo) {
		retval = -ENOMEM;
		goto ambsync_proc_release_exit;
	}

	mutex_lock(&hinfo->sync_proc_lock);
	idr_remove(&hinfo->sync_proc_idr, pinfo->id);
	mutex_unlock(&hinfo->sync_proc_lock);

	free_page((unsigned long)pinfo->page);
	kfree(pinfo);
	file->private_data = NULL;

ambsync_proc_release_exit:
	return retval;
}
EXPORT_SYMBOL(ambsync_proc_release);

/* Note: ignore ppos*/
ssize_t ambsync_proc_read(struct file *file, char __user *buf,
	size_t size, loff_t *ppos)
{
	int retval = 0;
	struct ambsync_proc_pinfo *pinfo = file->private_data;
	struct ambsync_proc_hinfo *hinfo;
	struct inode *inode = file->f_path.dentry->d_inode;
	char *start;
	int len;
	size_t count;

	hinfo = (struct ambsync_proc_hinfo *)PDE_DATA(inode);
	if (!hinfo) {
		retval = -EPERM;
		goto ambsync_proc_read_exit;
	}
	if (!hinfo->sync_read_proc) {
		retval = -EPERM;
		goto ambsync_proc_read_exit;
	}

	if (!pinfo) {
		retval = -ENOMEM;
		goto ambsync_proc_read_exit;
	}

	count = min_t(size_t, AMBA_SYNC_PROC_PAGE_SIZE, size);
	start = pinfo->page;
	len = 0;
	while (1) {
		wait_event_interruptible_timeout(hinfo->sync_proc_head,
			(atomic_read(&hinfo->sync_proc_flag) & pinfo->mask), hinfo->tmo);
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

ambsync_proc_read_exit:
	return retval;
}
EXPORT_SYMBOL(ambsync_proc_read);

ssize_t ambsync_proc_write(struct file *file, const char __user *buf,
	size_t size, loff_t *ppos)
{
	int retval = 0;
	struct ambsync_proc_hinfo *hinfo;
	struct inode *inode = file->f_path.dentry->d_inode;

	hinfo = (struct ambsync_proc_hinfo *)PDE_DATA(inode);
	if (!hinfo) {
		retval = -EPERM;
		goto ambsync_proc_write_exit;
	}

	atomic_set(&hinfo->sync_proc_flag, 0xFFFFFFFF);
	wake_up_all(&hinfo->sync_proc_head);

	retval = size;

ambsync_proc_write_exit:
	return retval;
}
EXPORT_SYMBOL(ambsync_proc_write);
