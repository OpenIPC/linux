/*
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "hinfc610_os.h"
#include "hinfc610_gen.h"
#include "hinfc610.h"
#include "hinfc610_sync.h"

static struct nand_sync *nand_sync_table[] = {
	&hinfc610_sync_onfi_23,
	&hinfc610_sync_onfi_30,
	&hinfc610_sync_toggle_10,
	NULL,
};

static struct nand_sync *hinfc610_find_sync_type(int type)
{
	struct nand_sync **sync;

	for (sync = nand_sync_table; sync; sync++) {
		if ((*sync)->type == type)
			return *sync;
	}

	return NULL;
}
/*****************************************************************************/

static int hinfc610_onfi_support_sync(struct hinfc_host *host)
{
	char buf[6] = {0};

	host->enable_ecc_randomizer(host, DISABLE, DISABLE);

	hinfc_write(host, sizeof(buf), HINFC610_DATA_NUM);
	hinfc_write(host, NAND_CMD_READID, HINFC610_CMD);
	hinfc_write(host, 0x20, HINFC610_ADDRL);
	hinfc_write(host, HINFC610_READ_1CMD_1ADD_DATA, HINFC610_OP);

	WAIT_CONTROLLER_FINISH();
	memcpy(buf, host->chip->IO_ADDR_R, sizeof(buf));

	if (memcmp(buf, "ONFI", 4))
		return 0;

	hinfc_write(host, sizeof(buf), HINFC610_DATA_NUM);
	hinfc_write(host, NAND_CMD_READID, HINFC610_CMD);
	hinfc_write(host, 0x40, HINFC610_ADDRL);
	hinfc_write(host, HINFC610_READ_1CMD_1ADD_DATA, HINFC610_OP);

	WAIT_CONTROLLER_FINISH();
	memcpy(buf, host->chip->IO_ADDR_R, sizeof(buf));

	if (memcmp(buf, "JEDEC", 5))
		return 0;

	return (buf[5] == 0x05);
}
/*****************************************************************************/

static int hinfc610_get_onfi_info(struct hinfc_host *host, int *type)
{
	char buf[6] = {0};

	*type = 0;

	if (!hinfc610_onfi_support_sync(host))
		return 0;

	host->enable_ecc_randomizer(host, DISABLE, DISABLE);

	hinfc_write(host, sizeof(buf), HINFC610_DATA_NUM);
	hinfc_write(host, NAND_CMD_PARAM, HINFC610_CMD);
	hinfc_write(host, 0x00, HINFC610_ADDRL);
	hinfc_write(host, HINFC610_READ_1CMD_1ADD_DATA, HINFC610_OP);

	WAIT_CONTROLLER_FINISH();
	memcpy(buf, host->chip->IO_ADDR_R, sizeof(buf));

	if (memcmp(buf, "ONFI", 4))
		return 0;

	if (buf[4] & (1 << 6))
		*type = NAND_TYPE_ONFI_30;
	else if (buf[4] & (1 << 5) ||
		 buf[4] & (1 << 4) ||
		 buf[4] & (1 << 3) ||
		 buf[4] & (1 << 2))
		*type = NAND_TYPE_ONFI_23;

	return 1;
}
/*****************************************************************************/

static int hinfc610_toggle_support_sync(struct hinfc_host *host)
{
	char buf[6] = {0};

	host->enable_ecc_randomizer(host, DISABLE, DISABLE);

	hinfc_write(host, sizeof(buf), HINFC610_DATA_NUM);
	hinfc_write(host, NAND_CMD_READID, HINFC610_CMD);
	hinfc_write(host, 0x40, HINFC610_ADDRL);
	hinfc_write(host, HINFC610_READ_1CMD_1ADD_DATA, HINFC610_OP);

	WAIT_CONTROLLER_FINISH();

	memcpy(buf, host->chip->IO_ADDR_R, sizeof(buf));

	if (memcmp(buf, "JEDEC", 5))
		return 0;

	return 1;
}
/*****************************************************************************/

static int hinfc610_get_toggle_info(struct hinfc_host *host, int *type)
{
	char buf[8] = {0};

	*type = 0;

	if (!hinfc610_toggle_support_sync(host))
		return 0;

	host->enable_ecc_randomizer(host, DISABLE, DISABLE);

	hinfc_write(host, sizeof(buf), HINFC610_DATA_NUM);
	hinfc_write(host, NAND_CMD_PARAM, HINFC610_CMD);
	hinfc_write(host, 0x40, HINFC610_ADDRL);
	hinfc_write(host, HINFC610_READ_1CMD_1ADD_DATA, HINFC610_OP);

	WAIT_CONTROLLER_FINISH();

	memcpy(buf, host->chip->IO_ADDR_R, sizeof(buf));

	if (memcmp(buf, "JESD", 4))
		return 0;

	if (buf[4] & (1 << 1))
		/* supports revision 1.0 */
		*type = NAND_TYPE_TOGGLE_10;
	else
		pr_warn("sync NAND has unknown toggle revision.\n");

	return 1;
}
/*****************************************************************************/

int hinfc610_get_sync_info(struct hinfc_host *host)
{
	int type = 0;

	if (IS_NAND_ONFI(host))
		hinfc610_get_onfi_info(host, &type);
	else
		hinfc610_get_toggle_info(host, &type);

	if (!type) {
		host->flags &= ~NAND_MODE_SYNC_ASYNC;
		return 0;
	}

	host->sync = hinfc610_find_sync_type(type);
	if (!host->sync)
		PR_BUG(ERSTR_DRIVER
			"This Nand Flash need to enable the 'synchronous' feature. "
			"but the driver dose not offer the feature");

	return 0;
}
/*****************************************************************************/
