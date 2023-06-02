/**
    CPU module driver.

    This file is the driver of CPU module.

    @file       CPU.h
    @ingroup    mIDrvSys_Core
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _CPU_H
#define _CPU_H

#include <kwrap/nvt_type.h>

//typedef void (*CPU_EXPCallbackFunc)(UINT32 *);
typedef void (*CPU_CBEXP)(UINT32 *, UINT32 *);

//extern ER     cpu_regExpCallbackFunc(CPU_CBEXP cpuExpCB);
extern ER   kdef_ecb(CPU_CBEXP cpuExpCB);
extern ER   cpu_begin_neon(void);
extern ER   cpu_end_neon(void);

//for rtos boot linux
extern ER   cpu_disable_interrupt(void);
extern ER   cpu_disable_cache(void);
extern ER   cpu_disable_mmu(void);
#endif
