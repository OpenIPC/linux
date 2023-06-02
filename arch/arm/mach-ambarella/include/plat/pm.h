/*
 * arch/arm/plat-ambarella/include/plat/pm.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_PM_H
#define __PLAT_AMBARELLA_PM_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

/* ==========================================================================*/
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_PM
extern int ambarella_init_pm(void);
extern int ambarella_finish_suspend(unsigned long);
extern void ambarella_cpu_resume(void);

#ifdef CONFIG_AMBARELLA_SREF_FIFO_EXEC
extern int ambarella_optimize_suspend(unsigned long);
extern int ambarella_optimize_suspend_sz;
#endif /* CONFIG_AMBARELLA_SREF_FIFO_EXEC */

#else
static inline int ambarella_init_pm(void){return 0;}
#endif

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

