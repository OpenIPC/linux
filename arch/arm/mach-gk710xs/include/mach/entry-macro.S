/*
 * arch/arm/mach-gk710xs/include/mach/entry-macro.S
 *
 * History:
 *  2014/06/18 - [Kewell Liu] created file
 *
 * Copyright (C) 2002-2014, Goke Microelectronics China.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <asm/irq.h>

        .macro    stat2nr, stat, nr, tmp
        rsbs    \tmp, \stat, #0
        and    \nr, \tmp, \stat
        clzcc    \nr, \nr
        rsc    \nr, \nr, #32
        teq    \nr, #32
        .endm

        .macro    disable_fiq
        .endm

        .macro    get_irqnr_preamble, base, tmp
        .endm

        .macro    arch_ret_to_user, tmp1, tmp2
        .endm

        .macro    get_irqnr_and_base, irqnr, irqstat, base, tmp
        ldr    \base, =(VIC1_BASE)
        ldr    \irqstat, [\base, #VIC_IRQ_STA_OFFSET]
        stat2nr    \irqstat, \irqnr, \tmp
#if (VIC_INSTANCES >= 2)
        bne    1000f
        ldr    \base, =(VIC2_BASE)
        ldr    \irqstat, [\base, #VIC_IRQ_STA_OFFSET]
        stat2nr    \irqstat, \irqnr, \tmp
        addne    \irqnr, \irqnr, #VIC2_INT_VEC(0)
#endif
1000:
        .endm

