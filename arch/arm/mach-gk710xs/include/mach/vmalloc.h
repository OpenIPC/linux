/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/vmalloc.h
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

#ifndef __MACH_VMALLOC_H
#define __MACH_VMALLOC_H

/* ==========================================================================*/
#if defined(CONFIG_VMSPLIT_3G)
#define VMALLOC_END            UL(0xe0000000)
#else
#define VMALLOC_END            UL(0xb0000000)
#endif

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

