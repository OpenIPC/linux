/*
 *  arch/arm/mach-GM-Duo/include/mach/platform/system.h
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
 *  Justin Shih  10/06/2012  Created.
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
static inline void arch_reset(char mode, const char *cmd)
{
#define BIT13 (0x1<<13)
    void __iomem *wdt_va_base = NULL;
    int rst_fd = -1;
    pmuReg_t regRSTArray[] = {
    	/* reg_off,bit_masks,lock_bits,init_val,init_mask */
	{0xB8, BIT13, BIT13, 0, BIT13},
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

    //outw((~(0x1<<13)), (PMU_FTPMU010_VA_BASE + 0xB8));
    rst_fd = ftpmu010_register_reg(&rst_clk_info);
    writeb(0, (wdt_va_base + 0x0C)); //reset WDT ctrl reg
    writel(0, (wdt_va_base + 0x04));//load margin conter
    writel(0x5ab9, (wdt_va_base + 0x08)); /*Magic number*/
    writeb(0x03, (wdt_va_base + 0x0C)); /*Enable WDT */
}

#endif /* __GM_PLATFORM_DEPENDENT_SYSTEM_HEADER__ */
