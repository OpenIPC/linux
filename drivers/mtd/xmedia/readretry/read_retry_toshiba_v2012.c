/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/mtd/nand.h>
#include "../xmedia_flash.h"

/******************************************************************************/

#define MAX_TOSHIBA2012_READ_RETRIES   6

static u8 rr_data[MAX_TOSHIBA2012_READ_RETRIES] = {
	0x00, 0x04, 0x7c, 0x78, 0x74, 0x08,
};
/******************************************************************************/

static int setup_toshiba_v2012(struct nand_read_retry_info *info,
			       int retry_mode)
{
	struct flash_regop_info regop_info = {0};

	pr_warn("toshiba nand read retry TBD.\n");
	dump_stack(); // NO CHIP, NOT TEST

	if (retry_mode >= MAX_TOSHIBA2012_READ_RETRIES) {
		pr_err("read retry out of range\n");
		return -1;
	}

	regop_info.priv = info->drv->priv;

	regop_info.cmd = 0x55;
	regop_info.nr_cmd = 1;
	regop_info.buf = &rr_data[retry_mode];
	regop_info.sz_buf = 1;
	regop_info.wait_ready = false;
	regop_info.addr_cycle = 1;

	regop_info.addrl = 0x04;
	info->drv->write_reg(&regop_info);

	regop_info.addrl = 0x05;
	info->drv->write_reg(&regop_info);

	regop_info.addrl = 0x06;
	info->drv->write_reg(&regop_info);

	regop_info.addrl = 0x07;
	info->drv->write_reg(&regop_info);

	regop_info.cmd = 0x5d26;
	regop_info.nr_cmd = 2;
	regop_info.addr_cycle = 0;
	regop_info.sz_buf = 0;
	info->drv->write_reg(&regop_info);

	return 0;
}
/******************************************************************************/

static int entry_toshiba_v2012(struct nand_read_retry_info *info)
{
	struct flash_regop_info regop_info = {0};

	regop_info.priv = info->drv->priv;

	regop_info.cmd = 0xc55c;
	regop_info.nr_cmd = 2;
	regop_info.addr_cycle = 0;
	regop_info.sz_buf = 0;
	info->drv->write_reg(&regop_info);

	return 0;
}
/******************************************************************************/

static int exit_toshiba_v2012(struct nand_read_retry_info *info)
{
	struct flash_regop_info regop_info = {0};

	regop_info.priv = info->drv->priv;

	regop_info.cmd = NAND_CMD_RESET;
	regop_info.nr_cmd = 1;
	regop_info.addr_cycle = 0;
	regop_info.sz_buf = 0;
	regop_info.wait_ready = true;
	info->drv->write_reg(&regop_info);

	return 0;
}
/******************************************************************************/

struct nand_read_retry nand_read_retry_toshiba_v2012 = {
	.type = NAND_RR_TOSHIBA_V2012,
	.count = MAX_TOSHIBA2012_READ_RETRIES,
	.setup = setup_toshiba_v2012,
	.entry = entry_toshiba_v2012,
	.exit = exit_toshiba_v2012,
};
