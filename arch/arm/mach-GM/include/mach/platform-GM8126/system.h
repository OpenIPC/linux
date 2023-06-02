/*
 *  arch/arm/mach-GM/include/mach/platform-a320/system.h
 *  
 *  Faraday Platform Dependent System Definitions
 *  
 *  Copyright (C) 2005 Faraday Corp. (http://www.faraday-tech.com)
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
 *
 * Description
 *
 *  This file is an example for all Faraday platforms that how to define
 *  a non-macro inline function while still be able to be checked by
 *  '#ifdef' or '#ifndef' preprocessor compilation directives.
 *  
 * ChangeLog
 *
 *  Luke Lee  09/22/2005  Created.
 */
#include <linux/kernel.h>
#include <linux/io.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>

#ifndef __GM_PLATFORM_DEPENDENT_SYSTEM_HEADER__
#define __GM_PLATFORM_DEPENDENT_SYSTEM_HEADER__

/*
 * Define the macro name exactly the same as the function name,
 * so that it can be checked by '#ifdef'. When this macro is
 * expanded, it is expanded to itself.
 */
#define arch_reset arch_reset
extern inline void arch_reset(char mode, const char *cmd)
{
#define BIT15 (0x1<<15)
    void __iomem *wdt_va_base = NULL;
    int rst_fd = -1;
    pmuReg_t regRSTArray[] = {
    	/* reg_off,bit_masks,lock_bits,init_val,init_mask */
	{0x3C, BIT15, BIT15, 0, BIT15},
    };
    pmuRegInfo_t rst_clk_info = {
    	"rst_clk",
	ARRAY_SIZE(regRSTArray),
	ATTR_TYPE_NONE,
	regRSTArray,
    };

    wdt_va_base = ioremap(WDT_FTWDT010_PA_BASE, WDT_FTWDT010_PA_SIZE);
    if (!wdt_va_base) {
        printk(KERN_INFO "Remap is failed\n");
        return;
    }

    //outw((~(0x1<<15)), (PMU_FTPMU010_VA_BASE + 0x3C));
    rst_fd = ftpmu010_register_reg(&rst_clk_info);
    writeb(0, (wdt_va_base + 0x0C)); //reset WDT ctrl reg
    writel(0, (wdt_va_base + 0x04));//load margin conter
    writel(0x5ab9, (wdt_va_base + 0x08)); /*Magic number*/
    writeb(0x03, (wdt_va_base + 0x0C)); /*Enable WDT */
}

#endif /* __GM_PLATFORM_DEPENDENT_SYSTEM_HEADER__ */

