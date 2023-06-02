/*
 * Copyright (c) 2015-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt) "l2cache: " fmt

#include <linux/io.h>
#include <linux/init.h>
#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>
#include <mach/io.h>
#include <mach/platform.h>

static void __iomem *l2x0_virt_base = __io_address(REG_BASE_L2CACHE);

static int __init l2_cache_init(void)
{
	u32 val;
	/*
	 * Bits  Value Description
	 * [31]    0 : SBZ
	 * [30]    1 : Double linefill enable (L3)
	 * [29]    1 : Instruction prefetching enable
	 * [28]    1 : Data prefetching enabled
	 * [27]    0 : Double linefill on WRAP read enabled (L3)
	 * [26:25] 0 : SBZ
	 * [24]    1 : Prefetch drop enable (L3)
	 * [23]    0 : Incr double Linefill enable (L3)
	 * [22]    0 : SBZ
	 * [21]    0 : Not same ID on exclusive sequence enable (L3)
	 * [20:5]  0 : SBZ
	 * [4:0]   0 : use the Prefetch offset values 0.
	 */
	/* writel_relaxed(0x71000000, l2x0_virt_base + L2X0_PREFETCH_CTRL); */
	writel_relaxed(0x71000000, l2x0_virt_base + L310_PREFETCH_CTRL);

	val = __raw_readl(l2x0_virt_base + L2X0_AUX_CTRL);
	val |= (1 << 30); /* Early BRESP enabled */
	val |= (1 << 0);  /* Full Line of Zero Enable */
	writel_relaxed(val, l2x0_virt_base + L2X0_AUX_CTRL);
	l2x0_init(l2x0_virt_base, 0x00430000, 0xFFB0FFFF);
	/*
	 * 2. enable L2 prefetch hint                  [1]a
	 * 3. enable write full line of zeros mode.    [3]a
	 *   a: This feature must be enabled only when the slaves
	 *      connected on the Cortex-A9 AXI master port support it.
	 */
	asm volatile (
	"	mrc	p15, 0, r0, c1, c0, 1\n"
	"	orr	r0, r0, #0x02\n"
	"	mcr	p15, 0, r0, c1, c0, 1\n"
	  :
	  :
	  : "r0", "cc");

	return 0;
}
early_initcall(l2_cache_init);
