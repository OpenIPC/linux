/*
 * arch/arm/plat-ambarella/misc/ambasyncproc.c
 *
 * Author: Zhenwu Xue, <zwxue@ambarella.com>
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
#include <linux/proc_fs.h>

#include <plat/ambasyncproc.h>
#include <mach/hardware.h>

#define GET_PROC_DATA_FROM_FILEP(filp)	\
	(struct amb_async_proc_info *)(PDE_DATA(filp->f_path.dentry->d_inode))

static int amb_async_proc_open(struct inode *inode, struct file *filp)
{
	struct amb_async_proc_info	*pinfo;

	pinfo = GET_PROC_DATA_FROM_FILEP(filp);

	mutex_lock(&pinfo->op_mutex);
	pinfo->use_count++;
	filp->f_op = &pinfo->fops;
	filp->private_data = pinfo->private_data;
	mutex_unlock(&pinfo->op_mutex);

	return 0;
}

static int amb_async_proc_fasync(int fd, struct file * filp, int on)
{
	int				retval;
	struct amb_async_proc_info	*pinfo;

	pinfo = GET_PROC_DATA_FROM_FILEP(filp);

	mutex_lock(&pinfo->op_mutex);
	retval = fasync_helper(fd, filp, on, &pinfo->fasync_queue);
	mutex_unlock(&pinfo->op_mutex);

	return retval;
}

static int amb_async_proc_release(struct inode *inode, struct file *filp)
{
	int				retval;
	struct amb_async_proc_info	*pinfo;

	pinfo = GET_PROC_DATA_FROM_FILEP(filp);

	mutex_lock(&pinfo->op_mutex);
	retval = fasync_helper(-1, filp, 0, &pinfo->fasync_queue);
	pinfo->use_count--;
	mutex_unlock(&pinfo->op_mutex);

	return retval;
}

int amb_async_proc_create(struct amb_async_proc_info *pinfo)
{
	int				retval = 0;
	struct proc_dir_entry		*entry;

	if (!pinfo) {
		retval = -EINVAL;
		goto amb_async_proc_create_exit;
	}

	pinfo->fops.open = amb_async_proc_open;
	pinfo->fops.fasync = amb_async_proc_fasync;
	pinfo->fops.release = amb_async_proc_release;
	mutex_init(&pinfo->op_mutex);
	pinfo->use_count = 0;
	pinfo->fasync_queue = NULL;

	entry = proc_create_data(pinfo->proc_name, S_IRUGO,
		get_ambarella_proc_dir(), &pinfo->fops, pinfo);
	if (!entry) {
		retval = -EINVAL;
	}

amb_async_proc_create_exit:
	return retval;
}
EXPORT_SYMBOL(amb_async_proc_create);

int amb_async_proc_remove(struct amb_async_proc_info *pinfo)
{
	int				retval = 0;

	if (!pinfo) {
		retval = -EINVAL;
	} else {
		remove_proc_entry(pinfo->proc_name, get_ambarella_proc_dir());
	}

	return retval;
}
EXPORT_SYMBOL(amb_async_proc_remove);

