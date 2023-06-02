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


#ifndef HINFC610_DBGH
#define HINFC610_DBGH
/******************************************************************************/

#define MAX_OPTION_SIZE                20

struct hinfc610_dbg_inf_t {
	const char name[MAX_OPTION_SIZE];
	int enable;
	int (*init)(struct dentry *root, struct hinfc_host *host);
	int (*uninit)(void);

	void (*read)(struct hinfc_host *host);
	void (*write)(struct hinfc_host *host);
	void (*erase)(struct hinfc_host *host);

	void (*read_retry)(struct hinfc_host *host, int index);
};

#define CMD_WORD_OFFSET             "offset="
#define CMD_WORD_LENGTH             "length="
#define CMD_WORD_CLEAN              "clear"
#define CMD_WORD_ON                 "on"
#define CMD_WORD_OFF                "off"

struct hinfc610_ecc_inf_t {
	int pagesize;
	int ecctype;
	int section;
	void (*ecc_inf)(struct hinfc_host *host, unsigned char ecc[]);
};

struct hinfc610_ecc_inf_t *hinfc610_get_ecc_inf(struct hinfc_host *host,
						int pagesize, int ecctype);

extern struct hinfc610_dbg_inf_t hinfc610_dbg_inf_dump;
extern struct hinfc610_dbg_inf_t hinfc610_dbg_inf_erase_count;
extern struct hinfc610_dbg_inf_t hinfc610_dbg_inf_ecc_count;
extern struct hinfc610_dbg_inf_t hinfc610_dbg_inf_read_retry;
extern struct hinfc610_dbg_inf_t hinfc610_dbg_inf_read_retry_notice;
extern struct hinfc610_dbg_inf_t hinfc610_dbg_inf_ecc_notice;

/******************************************************************************/
#endif /* HINFC610_DBGH */
