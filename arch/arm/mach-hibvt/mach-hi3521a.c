/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
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
 *
*/

#include <linux/clocksource.h>
#include <linux/irqchip.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/io.h>

/*
 * This table is only for optimization. Since ioremap() could always share
 * the same mapping if it's defined as static IO mapping.
 *
 * Without this table, system could also work. The cost is some virtual address
 * spaces wasted since ioremap() may be called multi times for the same
 * IO space.
 */
static struct map_desc hi3521a_io_desc[] __initdata = {
	/* hi3521a_IOCH1 */
	{
		.pfn		= __phys_to_pfn(HI3521A_IOCH1_PHYS),
		.virtual	= HI3521A_IOCH1_VIRT,
		.length		= HI3521A_IOCH1_SIZE,
		.type		= MT_DEVICE,
	},

	/* hi3521a_IOCH2 */
	{
		.pfn		= __phys_to_pfn(HI3521A_IOCH2_PHYS),
		.virtual	= HI3521A_IOCH2_VIRT,
		.length		= HI3521A_IOCH2_SIZE,
		.type		= MT_DEVICE,
	},

	/* hi3521a_IOCH3 */
	{
		.pfn        = __phys_to_pfn(HI3521A_IOCH3_PHYS),
		.virtual    = HI3521A_IOCH3_VIRT,
		.length     = HI3521A_IOCH3_SIZE,
		.type       = MT_DEVICE
	},
};

static void __init hi3521a_map_io(void)
{
	/* debug_ll_io_init(); */
	iotable_init(hi3521a_io_desc, ARRAY_SIZE(hi3521a_io_desc));
}

static const char *const hi3521a_compat[] __initconst = {
	"hisilicon,hi3521a",
	NULL,
};

DT_MACHINE_START(HI3521A_DT, "Hisilicon Hi3521A (Flattened Device Tree)")
	.map_io		= hi3521a_map_io,
	.dt_compat	= hi3521a_compat,
MACHINE_END
