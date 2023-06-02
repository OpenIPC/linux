/*
    SIE KDRV configuration header file.

    SIE KDRV configuration header file. Define semaphore ID, flag ID, etc.

    @file       kdrv_sie_config.h
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _KDRV_SIE_CONFIG_H
#define _KDRV_SIE_CONFIG_H

#if defined(__LINUX)
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#else
#include "pll_protected.h"
#include "pll.h"
#endif
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include <kwrap/flag.h>
#include "kdrv_sie_int.h"
#include "sie_dbg.h"

// ==================================================================
// Semaphore ID
// ==================================================================
typedef enum {
	SEMID_KDRV_SIE1,
	SEMID_KDRV_SIE2,
	SEMID_KDRV_SIE3,
	SEMID_KDRV_SIE4,
	SEMID_KDRV_SIE5,
	SEMID_KDRV_SIE6,
	SEMID_KDRV_SIE7,
	SEMID_KDRV_SIE8,
	// Insert member before this line
	// Don't change the order of following two members
	SEMID_KDRV_MAX_COUNT,
	ENUM_DUMMY4WORD(KDRV_SIE_SEM)
} KDRV_SIE_SEM;

//@}


/**
    Semphore Configuration Infomation
*/
typedef struct _KDRV_SIE_SEM_INFO_ {
	ID  	semphore_id;				///< Semaphore ID
	UINT32	max_counter;
} KDRV_SIE_SEM_TABLE;

extern KDRV_SIE_SEM_TABLE kdrv_sie_semtbl[KDRV_SIE_MAX_ENG];
void kdrv_sie_install_id(void);
void kdrv_sie_uninstall_id(void);
SEM_HANDLE *kdrv_sie_get_sem_id(KDRV_SIE_PROC_ID id);

#endif
