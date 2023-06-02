/*
    Internal header file for SIE module.

    @file       sie_init_int.h
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _SIE_INIT_INT_H
#define _SIE_INIT_INT_H
#if defined(__KERNEL__)
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include "sie_init.h"

INT32 sie_init_plat_chk_node(CHAR *node);
INT32 sie_init_plat_read_dtsi_array(CHAR *node, CHAR *tag, UINT32 *buf, UINT32 num);
void *kdrv_sie_builtin_plat_malloc(UINT32 size);
void kdrv_sie_builtin_plat_free(void *p_buf);

#endif// _SIE_INIT_INT_H
#endif
