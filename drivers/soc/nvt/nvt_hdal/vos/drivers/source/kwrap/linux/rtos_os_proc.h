/**
    Linux proc interface

    @file nvt_stream_snd_proc.h
    Copyright Novatek Microelectronics Corp. 2014. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _VOS_PROC_H_
#define _VOS_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                                                                                 */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

//! Init/Exit
void rtos_proc_init(void *param);
void rtos_proc_exit(void);

#ifdef __cplusplus
}
#endif

#endif //_VOS_PROC_H_