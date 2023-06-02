/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/plat/reglock.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __PLAT_REGISTER_LOCK_H
#define __PLAT_REGISTER_LOCK_H

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#ifdef CONFIG_GK_ADD_REGISTER_LOCK
#include <linux/spinlock.h>
extern spinlock_t gk_register_lock;
extern unsigned long gk_register_flags;
extern u32 gk_reglock_count;

#define GK_REG_LOCK()           spin_lock_irqsave(&gk_register_lock, gk_register_flags)
#define GK_REG_UNLOCK()         spin_unlock_irqrestore(&gk_register_lock, gk_register_flags)
#define GK_INC_REGLOCK_COUNT()  gk_reglock_count++
#else
#define GK_REG_LOCK()
#define GK_REG_UNLOCK()
#define GK_INC_REGLOCK_COUNT()

#endif /* CONFIG_GK_ADD_REGISTER_LOCK */

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

