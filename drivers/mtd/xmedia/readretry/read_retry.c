/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "read_retry.h"
#include "../xmedia_flash.h"

static struct nand_read_retry *nand_read_retry[] = {
	&nand_read_retry_micron,
	&nand_read_retry_samsung,
	&nand_read_retry_toshiba_v2012,
	&nand_read_retry_toshiba_v2013,
	&nand_read_retry_hynix_cg_adie,
	&nand_read_retry_hynix_bc_cdie,
	NULL,
};

struct nand_read_retry *nand_get_read_retry(int read_retry_type)
{
	int ix;

	for (ix = 0; nand_read_retry[ix]; ix++) {
		if (nand_read_retry[ix]->type == read_retry_type)
			return nand_read_retry[ix];
	}

	return NULL;
}
