/*
 *  arch/arm/mach-GM/include/mach/platform/pmu.h
 *
 *  Copyright (C) 2009 Faraday Technology.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PMU_H__
#define __PMU_H__

unsigned int pmu_get_apb0_clk(void);
unsigned int pmu_get_apb1_clk(void);
unsigned int pmu_get_apb2_clk(void);
int platform_check_flash_type(void);
void pmu_earlyinit(void __iomem *base);

#endif	/* __PMU_H__ */
