/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "../xmedia_flash.h"
#include "nand_sync.h"
/*****************************************************************************/

int enable_sync_onfi_v23(struct flash_regop *regop)
{
	/* B0: timing mode.
	 * B1: 0
	 * B2: 0
	 * B3: 0
	 */
	u8 status = 0x10; //up to timing mode 4 for onfi2.3, here set to timing mode 0 by default.
	struct flash_regop_info regop_info = {0};

	/* set feature. */
	regop_info.priv = regop->priv;
	regop_info.cmd = 0xEF;
	regop_info.nr_cmd = 1;
	regop_info.addrl = 0x01;
	regop_info.addr_cycle = 1;
	regop_info.buf = &status;
	regop_info.sz_buf = 4;
	regop_info.wait_ready = true;

	regop->write_reg(&regop_info);

	return 0;
}
/*****************************************************************************/

int disable_sync_onfi_v23(struct flash_regop *regop)
{
	/* B0: timing mode.
	 * B1: 0
	 * B2: 0
	 * B3: 0
	 */
	u8 status = 0;
	struct flash_regop_info regop_info = {0};

	/* set feature. */
	regop_info.priv = regop->priv;
	regop_info.cmd = 0xEF;
	regop_info.nr_cmd = 1;
	regop_info.addrl = 0x01;
	regop_info.addr_cycle = 1;
	regop_info.buf = &status;
	regop_info.sz_buf = 4;
	regop_info.wait_ready = true;

	regop->write_reg(&regop_info);

	return 0;
}
/*****************************************************************************/

int enable_sync_onfi_v30(struct flash_regop *regop)
{
	/* B0: timing mode.
	 * B1: 0
	 * B2: 0
	 * B3: 0
	 */
	u8 status = 0x25; //up to timing mode 5 for onfi3.0
	struct flash_regop_info regop_info = {0};

	/* set feature. */
	regop_info.priv = regop->priv;
	regop_info.cmd = 0xEF;
	regop_info.nr_cmd = 1;
	regop_info.addrl = 0x01;
	regop_info.addr_cycle = 1;
	regop_info.buf = &status;
	regop_info.sz_buf = 4;
	regop_info.wait_ready = true;

	regop->write_reg(&regop_info);

	return 0;
}
/*****************************************************************************/

int disable_sync_onfi_v30(struct flash_regop *regop)
{
	/* B0: timing mode.
	 * B1: 0
	 * B2: 0
	 * B3: 0
	 */
	u8 status = 0;
	struct flash_regop_info regop_info = {0};

	/* set feature. */
	regop_info.priv = regop->priv;
	regop_info.cmd = 0xEF;
	regop_info.nr_cmd = 1;
	regop_info.addrl = 0x01;
	regop_info.addr_cycle = 1;
	regop_info.buf = &status;
	regop_info.sz_buf = 4;
	regop_info.wait_ready = true;

	regop->write_reg(&regop_info);

	return 0;
}
/******************************************************************************/

struct nand_sync nand_sync_onfi_v23 = {
	.type          = NAND_MODE_SYNC_ONFI23,
	.enable_sync   = enable_sync_onfi_v23,
	.disable_sync  = disable_sync_onfi_v23,
};
/******************************************************************************/

struct nand_sync nand_sync_onfi_v30 = {
	.type          = NAND_MODE_SYNC_ONFI30,
	.enable_sync   = enable_sync_onfi_v30,
	.disable_sync  = disable_sync_onfi_v30,
};
