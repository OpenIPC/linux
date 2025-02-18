/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/mtd/nand.h>
#include "../xmedia_flash.h"

/******************************************************************************/

#define MAX_RR_HYNIX_READ_RETRIES  8
/******************************************************************************/

static int __read_retry_hynix_cg_adie(struct nand_read_retry_info *info, u8 *rr_data,
				      int retry_mode)
{
	int ix;
	u8 *ptr;
	struct flash_regop_info regop_info = {0};

	pr_warn("hynix nand read retry TBD.\n");
	dump_stack(); // NO CHIP, NOT TEST.

	if (retry_mode >= MAX_RR_HYNIX_READ_RETRIES) {
		pr_err("read retry out of range\n");
		return -1;
	}

	ptr = &info->buf[retry_mode << 3];

	regop_info.priv = info->drv->priv;
	regop_info.sz_buf = 1;
	regop_info.addr_cycle = 1;

	regop_info.cmd = 0x36;
	regop_info.nr_cmd = 1;
	regop_info.buf = &ptr[0];
	regop_info.addrl = rr_data[0];
	info->drv->write_reg(&regop_info);

	regop_info.nr_cmd = 0;
	for (ix = 1; ix < 8; ix++) {
		regop_info.buf = &ptr[ix];
		regop_info.addrl = rr_data[ix];
		info->drv->write_reg(&regop_info);
	}

	regop_info.cmd = 0x16;
	regop_info.nr_cmd = 1;
	regop_info.sz_buf = 0;
	regop_info.addr_cycle = 0;
	info->drv->write_reg(&regop_info);

	return 0;
}
/******************************************************************************/

static int setup_hynix_cg_adie(struct nand_read_retry_info *info, int retry_mode)
{
	u8 rr_data[MAX_RR_HYNIX_READ_RETRIES] = {
		0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF,
	};

	return __read_retry_hynix_cg_adie(info, rr_data, retry_mode);
}
/******************************************************************************/

static int setup_hynix_bc_cdie(struct nand_read_retry_info *info, int retry_mode)
{
	u8 rr_data[MAX_RR_HYNIX_READ_RETRIES] = {
		0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
	};

	return __read_retry_hynix_cg_adie(info, rr_data, retry_mode);
}
/******************************************************************************/

struct nand_read_retry nand_read_retry_hynix_cg_adie = {
	.type = NAND_RR_HYNIX_CG_ADIE,
	.count = MAX_RR_HYNIX_READ_RETRIES,
	.setup = setup_hynix_cg_adie,
};
/******************************************************************************/

struct nand_read_retry nand_read_retry_hynix_bc_cdie = {
	.type = NAND_RR_HYNIX_BG_CDIE,
	.count = MAX_RR_HYNIX_READ_RETRIES,
	.setup = setup_hynix_bc_cdie,
};
