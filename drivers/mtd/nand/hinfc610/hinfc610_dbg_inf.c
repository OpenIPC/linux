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

#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

#include "hinfc610_os.h"
#include "hinfc610.h"
#include "hinfc610_dbg.h"

void hinfc610_dbg_write(struct hinfc_host *host)
{
#ifdef CONFIG_HINFC610_DBG_NAND_DEBUG
    struct hinfc610_dbg_inf_t **inf;

    for (inf = hinfc610_dbg_inf; *inf; inf++)
        if ((*inf)->enable && (*inf)->write) {
            (*inf)->write(host);
        }
#endif
}

void hinfc610_dbg_erase(struct hinfc_host *host)
{
#ifdef CONFIG_HINFC610_DBG_NAND_DEBUG
    struct hinfc610_dbg_inf_t **inf;

    for (inf = hinfc610_dbg_inf; *inf; inf++)
        if ((*inf)->enable && (*inf)->erase) {
            (*inf)->erase(host);
        }
#endif
}

void hinfc610_dbg_read(struct hinfc_host *host)
{
#ifdef CONFIG_HINFC610_DBG_NAND_DEBUG
    struct hinfc610_dbg_inf_t **inf;

    for (inf = hinfc610_dbg_inf; *inf; inf++)
        if ((*inf)->enable && (*inf)->read) {
            (*inf)->read(host);
        }
#endif
}

void hinfc610_dbg_read_retry(struct hinfc_host *host, int index)
{
#ifdef CONFIG_HINFC610_DBG_NAND_DEBUG
    struct hinfc610_dbg_inf_t **inf;

    for (inf = hinfc610_dbg_inf; *inf; inf++)
        if ((*inf)->enable && (*inf)->read_retry) {
            (*inf)->read_retry(host, index);
        }
#endif
}

int hinfc610_dbg_init(struct hinfc_host *host)
{
#ifdef CONFIG_HINFC610_DBG_NAND_DEBUG
    return hinfc610_dbgfs_debug_init(host);
#endif
    return 0;
}
