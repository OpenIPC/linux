/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __NAND_DEV_H__
#define __NAND_DEV_H__

struct nand_kobject {
	struct kobject kobj;
	struct _nftl_blk *nftl_blk;
	char name[32];
};

extern unsigned int do_static_wear_leveling(void *zone);
extern unsigned short nftl_get_zone_write_cache_nums(void *_zone);
extern unsigned int garbage_collect(void *zone);
extern unsigned int do_prio_gc(void *zone);
extern int critical_dev_nand_write(uchar *dev_name, __u32 start_sector,
				   __u32 sector_num, unsigned char *buf);
extern int critical_dev_nand_read(uchar *dev_name, __u32 start_sector,
				  __u32 sector_num, uchar *buf);

#endif
