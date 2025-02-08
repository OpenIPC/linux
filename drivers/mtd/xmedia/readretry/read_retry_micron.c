/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/mtd/nand.h>
#include "../xmedia_flash.h"

/******************************************************************************/

#define MAX_MICRON_READ_RETRIES        8
/*****************************************************************************/

static int setup_micron(struct nand_read_retry_info *info, int retry_mode)
{
	struct flash_regop_info regop_info = {0};

	if (retry_mode >= MAX_MICRON_READ_RETRIES) {
		pr_err("out of range\n");
		return -1;
	}

	regop_info.priv = info->drv->priv;
	regop_info.cmd = 0xEF;
	regop_info.nr_cmd = 1;
	regop_info.addrl = 0x89;
	regop_info.addr_cycle = 1;
	regop_info.buf = (u8*)&retry_mode;
	regop_info.sz_buf = 4;
	regop_info.wait_ready = true;

	info->drv->write_reg(&regop_info);

	return 0;
}
/******************************************************************************/

static int exit_micron(struct nand_read_retry_info *info)
{
	return setup_micron(info, 0);
}
/******************************************************************************/

struct nand_read_retry nand_read_retry_micron = {
	.type = NAND_RR_MICRON,
	.count = MAX_MICRON_READ_RETRIES,
	.setup = setup_micron,
	.exit = exit_micron,
};
