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

/*****************************************************************************/
static inline void hinfc610_detect_ecc(unsigned char ecc[], int begin,
				       int end, unsigned int reg)
{
	while (begin < end) {
		ecc[begin] = (reg & 0xff);
		reg = (reg >> 8);
		begin++;
	}
}
/*****************************************************************************/

static void hinfc610_ecc_32k(struct hinfc_host *host, unsigned char ecc[])
{
	int ix, jx, kx;

	for (ix = 0, jx = 0; ix < 4; ix ++, jx += 4)
		hinfc610_detect_ecc(ecc, jx, jx + 4,
				    hinfc_read(host, 0xA0 + jx));
	kx = jx;
	for (ix = 0, jx = 0; ix < 4; ix ++, jx += 4)
		hinfc610_detect_ecc(ecc, kx, kx + 4,
				    hinfc_read(host, 0xDC + jx));
}
/*****************************************************************************/

static void hinfc610_ecc_16k(struct hinfc_host *host, unsigned char ecc[])
{
	int ix, jx;

	for (ix = 0, jx = 0; ix < 4; ix ++, jx += 4)
		hinfc610_detect_ecc(ecc, jx, jx + 4,
				    hinfc_read(host, 0xA0 + jx));
}
/*****************************************************************************/

static void hinfc610_ecc_8k(struct hinfc_host *host, unsigned char ecc[])
{
	int ix, jx;

	for (ix = 0, jx = 0; ix < 2; ix ++, jx += 4)
		hinfc610_detect_ecc(ecc, jx, jx + 4,
				    hinfc_read(host, 0xA0 + jx));
}
/*****************************************************************************/

static void hinfc610_ecc_4k(struct hinfc_host *host, unsigned char ecc[])
{
	hinfc610_detect_ecc(ecc, 0, 4, hinfc_read(host, 0xA0));
}
/*****************************************************************************/

static void hinfc610_ecc_2k(struct hinfc_host *host, unsigned char ecc[])
{
	hinfc610_detect_ecc(ecc, 0, 2, hinfc_read(host, 0xA0));
}
/*****************************************************************************/

static struct hinfc610_ecc_inf_t hinfc610_ecc_inf[] = {

	{32768, NAND_ECC_80BIT, 32, hinfc610_ecc_32k},
	{32768, NAND_ECC_72BIT, 32, hinfc610_ecc_32k},
	{32768, NAND_ECC_60BIT, 32, hinfc610_ecc_32k},
	{32768, NAND_ECC_48BIT, 32, hinfc610_ecc_32k},
	{32768, NAND_ECC_41BIT, 32, hinfc610_ecc_32k},

	{16384, NAND_ECC_80BIT, 16, hinfc610_ecc_16k},
	{16384, NAND_ECC_72BIT, 16, hinfc610_ecc_16k},
	{16384, NAND_ECC_60BIT, 16, hinfc610_ecc_16k},
	{16384, NAND_ECC_48BIT, 16, hinfc610_ecc_16k},
	{16384, NAND_ECC_41BIT, 16, hinfc610_ecc_16k},

	{8192, NAND_ECC_80BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_72BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_60BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_48BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_41BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_32BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_27BIT, 8, hinfc610_ecc_8k},
	{8192, NAND_ECC_24BIT, 8, hinfc610_ecc_8k},



	{4096, NAND_ECC_32BIT, 4, hinfc610_ecc_4k},
	{4096, NAND_ECC_27BIT, 4, hinfc610_ecc_4k},
	{4096, NAND_ECC_24BIT, 4, hinfc610_ecc_4k},
	{4096, NAND_ECC_18BIT, 4, hinfc610_ecc_4k},
	{4096, NAND_ECC_13BIT, 4, hinfc610_ecc_4k},
	{4096, NAND_ECC_8BIT,  4, hinfc610_ecc_4k},

	{2048, NAND_ECC_32BIT, 2, hinfc610_ecc_2k},
	{2048, NAND_ECC_27BIT, 2, hinfc610_ecc_2k},
	{2048, NAND_ECC_24BIT, 2, hinfc610_ecc_2k},
	{2048, NAND_ECC_18BIT, 2, hinfc610_ecc_2k},
	{2048, NAND_ECC_13BIT, 2, hinfc610_ecc_2k},
	{2048, NAND_ECC_8BIT,  2, hinfc610_ecc_2k},
	{0, 0, 0},
};
/*****************************************************************************/

struct hinfc610_ecc_inf_t *hinfc610_get_ecc_inf(struct hinfc_host *host,
						int pagesize, int ecctype)
{
	struct hinfc610_ecc_inf_t *inf;

	for (inf = hinfc610_ecc_inf; inf->pagesize; inf++)
		if (inf->pagesize == pagesize && inf->ecctype == ecctype)
			return inf;

	return NULL;
}
