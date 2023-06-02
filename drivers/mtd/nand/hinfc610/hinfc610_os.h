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


#ifndef HINFC610_OSH
#define HINFC610_OSH
/******************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <asm/setup.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/clkdev.h>

#include "../../mtdcore.h"

/*****************************************************************************/

#define DUMP_DATA(_p, _n) do { \
	int ix; \
	unsigned char *rr = (unsigned char *)(_p); \
	for (ix = 0; ix < _n; ix++) { \
		pr_info("%02X ", rr[ix]); \
		if (!((ix + 1) % 16)) \
			pr_info("\n"); \
	} \
} while (0)

#define DBG_OUT(fmt, args...)\
	pr_warn("%s(%d): " fmt, __FILE__, __LINE__, ##args) \

#if 1
#  define DBG_MSG(_fmt, arg...)
#else
#  define DBG_MSG(_fmt, arg...) \
	pr_info("%s(%d): " _fmt, __FILE__, __LINE__, ##arg)
#endif

#define PR_BUG(fmt, args...) do {\
	pr_debug("%s(%d): bug " fmt, __FILE__, __LINE__, ##args); \
	asm("b ."); \
} while (0)

#define PR_ERR(fmt, args...)\
	pr_err("%s(%d): " fmt, __FILE__, __LINE__, ##args) \

#define PR_MSG(_fmt, arg...) \
	printk(_fmt, ##arg)

/******************************************************************************/
#endif /* HINFC610_OSH */
