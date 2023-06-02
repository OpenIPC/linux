/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * UFS connection table manager
 */
#ifndef __UFS_PROC_H__
#define __UFS_PROC_H__

#include <linux/proc_fs.h>

#define MAX_CARD_TYPE	1

#ifdef CONFIG_ARCH_HI3559AV100
#define UFS_SLOT_NUM	1
#endif

extern unsigned int slot_index;
extern struct ufs_hba *hba_list[UFS_SLOT_NUM];

int ufs_proc_init(void);
int ufs_proc_shutdown(void);

#endif /*  __UFS_PROC_H__ */
