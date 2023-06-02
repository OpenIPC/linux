/*
 * arch/arm/plat-ambarella/include/plat/ambync_proc.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#ifndef __PLAT_AMBARELLA_SYNC_PROC_H
#define __PLAT_AMBARELLA_SYNC_PROC_H

#include <linux/fs.h>

/* ==========================================================================*/
#define AMBA_SYNC_PROC_MAX_ID			(31)
#define AMBA_SYNC_PROC_PAGE_SIZE		(PAGE_SIZE - 16)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

typedef	int(ambsync_read_proc_t)(char *start, void *data);

struct ambsync_proc_pinfo {
	u32					id;
	u32					mask;
	char					*page;
};

struct ambsync_proc_hinfo {
	u32					maxid;
	u32					tmo;
	wait_queue_head_t			sync_proc_head;
	atomic_t				sync_proc_flag;
	struct idr				sync_proc_idr;
	struct mutex				sync_proc_lock;
	ambsync_read_proc_t			*sync_read_proc;
	void					*sync_read_data;
};

/* ==========================================================================*/

/* ==========================================================================*/
extern int ambsync_proc_hinit(struct ambsync_proc_hinfo *hinfo);
extern int ambsync_proc_open(struct inode *inode, struct file *file);
extern int ambsync_proc_release(struct inode *inode, struct file *file);
extern ssize_t ambsync_proc_read(struct file *file, char __user *buf, size_t size, loff_t *ppos);
extern ssize_t ambsync_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

