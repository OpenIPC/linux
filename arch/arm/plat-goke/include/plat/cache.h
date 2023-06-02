/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/plat/cache.h
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

#ifndef __PLAT_CACHE_H
#define __PLAT_CACHE_H

/* ==========================================================================*/
#define CACHE_LINE_SIZE		32
#define CACHE_LINE_MASK		~(CACHE_LINE_SIZE - 1)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

extern void gk_cache_clean_range(void *addr, unsigned int size);
extern void gk_cache_inv_range(void *addr, unsigned int size);
extern void gk_cache_flush_range(void *addr, unsigned int size);
extern void gk_cache_pli_range(void *addr, unsigned int size);
extern int gk_cache_l2_enable(void);
extern int gk_cache_l2_disable(void);
extern void gk_cache_l2_enable_raw(void);
extern void gk_cache_l2_disable_raw(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

