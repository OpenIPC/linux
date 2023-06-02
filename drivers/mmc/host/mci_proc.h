/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
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
 */

/*
 *  MCI connection table manager
 */
#ifndef __MCI_PROC_H__
#define __MCI_PROC_H__

#include <linux/proc_fs.h>

#define MAX_CARD_TYPE	4
#define MAX_SPEED_MODE	5

#ifdef CONFIG_ARCH_HI3559AV100
	#define MCI_SLOT_NUM 4
#endif

#if defined(CONFIG_ARCH_HI3556AV100) || defined(CONFIG_ARCH_HI3519AV100) ||\
	defined(CONFIG_ARCH_HI3516EV200) || defined(CONFIG_ARCH_HI3516EV300) ||\
	defined(CONFIG_ARCH_HI3518EV300) || defined(CONFIG_ARCH_HI3516DV200)
	#define MCI_SLOT_NUM 3
#endif

extern unsigned int slot_index;
extern struct mmc_host *mci_host[MCI_SLOT_NUM];

int mci_proc_init(void);
int mci_proc_shutdown(void);

#endif /*  __MCI_PROC_H__ */
