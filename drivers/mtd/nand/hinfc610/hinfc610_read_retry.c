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

#include "hinfc610_gen.h"
#include "hinfc610.h"
#include "hinfc610_read_retry.h"

static struct read_retry_t *read_retry_table[] = {
	&hinfc610_hynix_bg_bdie_read_retry,
	&hinfc610_hynix_bg_cdie_read_retry,
	&hinfc610_hynix_cg_adie_read_retry,
	&hinfc610_micron_read_retry,
	&hinfc610_toshiba_24nm_read_retry,
	&hinfc610_samsung_read_retry,
	NULL,
};

struct read_retry_t *hinfc610_find_read_retry(int type)
{
	struct read_retry_t **rr;

	for (rr = read_retry_table; rr; rr++) {
		if ((*rr)->type == type)
			return *rr;
	}

	return NULL;
}
