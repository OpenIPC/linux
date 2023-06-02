/*
 *  fs/partitions/ambptb.c
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

#include "check.h"
#include "ambptb.h"
#include <plat/ptb.h>
#include <linux/of.h>

//#define ambptb_prt

#ifdef ambptb_prt
#define ambptb_prt printk
#else
#define ambptb_prt(format, arg...) do {} while (0)
#endif

int ambptb_partition(struct parsed_partitions *state)
{
	int i, val, slot = 1;
	unsigned char *data;
	Sector sect;
	u32 sect_size, sect_offset, sect_address, ptb_address;
	flpart_meta_t *ptb_meta;
	char ptb_tmp[1 + BDEVNAME_SIZE + 1];
	int result = 0;
	struct device_node * np;

	sect_size = bdev_logical_block_size(state->bdev);
	sect_offset = (sizeof(ptb_header_t) + sizeof(flpart_table_t)) % sect_size;

	np = of_find_node_with_property(NULL, "amb,ptb_address");
	if(!np)
		return -1;

	val = of_property_read_u32(np, "amb,ptb_address", &ptb_address);
	if(val < 0)
		return -1;

	sect_address = (ptb_address * sect_size + sizeof(ptb_header_t) + sizeof(flpart_table_t)) / sect_size;
	data = read_part_sector(state, sect_address, &sect);
	if (!data) {
		result = -1;
		goto ambptb_partition_exit;
	}

	ptb_meta = (flpart_meta_t *)(data + sect_offset);
	ambptb_prt("%s: magic[0x%08X]\n", __func__, ptb_meta->magic);
	if (ptb_meta->magic == PTB_META_MAGIC3) {
		for (i = 0; i < PART_MAX; i++) {
			if (slot >= state->limit)
				break;

			if (((ptb_meta->part_info[i].dev & PART_DEV_EMMC) ==
				PART_DEV_EMMC) &&
				(ptb_meta->part_info[i].nblk)) {
				state->parts[slot].from =
					ptb_meta->part_info[i].sblk;
				state->parts[slot].size =
					ptb_meta->part_info[i].nblk;
				snprintf(ptb_tmp, sizeof(ptb_tmp), " %s",
					ptb_meta->part_info[i].name);
				strlcat(state->pp_buf, ptb_tmp, PAGE_SIZE);
				ambptb_prt("%s: %s [p%d]\n", __func__,
					ptb_meta->part_info[i].name, slot);
				slot++;
			}
		}
		strlcat(state->pp_buf, "\n", PAGE_SIZE);
		result = 1;
	} else if ((ptb_meta->magic == PTB_META_MAGIC) ||
		(ptb_meta->magic == PTB_META_MAGIC2)) {
		for (i = 0; i < PART_MAX; i++) {
			if (slot >= state->limit)
				break;
			if ((ptb_meta->part_info[i].dev == BOOT_DEV_SM) &&
				(ptb_meta->part_info[i].nblk)) {
				state->parts[slot].from =
					ptb_meta->part_info[i].sblk;
				state->parts[slot].size =
					ptb_meta->part_info[i].nblk;
				snprintf(ptb_tmp, sizeof(ptb_tmp), " %s",
					ptb_meta->part_info[i].name);
				strlcat(state->pp_buf, ptb_tmp, PAGE_SIZE);
				ambptb_prt("%s: %s [p%d]\n", __func__,
					ptb_meta->part_info[i].name, slot);
				slot++;
			}
		}
		strlcat(state->pp_buf, "\n", PAGE_SIZE);
		result = 1;
	}
	put_dev_sector(sect);

ambptb_partition_exit:
	return result;
}

