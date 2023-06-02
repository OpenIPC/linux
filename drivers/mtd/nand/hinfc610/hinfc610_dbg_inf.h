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

#ifndef HINFC610_DBG_INFH
#define HINFC610_DBG_INFH
/******************************************************************************/

int hinfc610_dbg_init(struct hinfc_host *host);

void hinfc610_dbg_write(struct hinfc_host *host);

void hinfc610_dbg_erase(struct hinfc_host *host);

void hinfc610_dbg_read(struct hinfc_host *host);

void hinfc610_dbg_read_retry(struct hinfc_host *host, int index);

/******************************************************************************/
#endif /* HINFC610_DBG_INFH */
