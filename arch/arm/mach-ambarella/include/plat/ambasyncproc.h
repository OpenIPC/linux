/*
 * arch/arm/plat-ambarella/include/plat/ambasyncproc.h
 *
 * Author: Zhenwu Xue <zwxue@ambarella.com>
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

#ifndef __PLAT_AMBARELLA_ASYNC_PROC_H
#define __PLAT_AMBARELLA_ASYNC_PROC_H

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct amb_async_proc_info {
	char				proc_name[256];
	struct file_operations		fops;
	void				*private_data;
	struct fasync_struct		*fasync_queue;
	struct mutex			op_mutex;
	int				use_count;
};

extern int amb_async_proc_create(struct amb_async_proc_info *pinfo);
extern int amb_async_proc_remove(struct amb_async_proc_info *pinfo);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

