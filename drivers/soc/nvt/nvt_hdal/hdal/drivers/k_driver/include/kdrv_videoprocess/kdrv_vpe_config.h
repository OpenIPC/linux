/*
    VPE DAL configuration header file.

    VPE DAL configuration header file. Define semaphore ID, flag ID, etc.

    @file       kdrv_vpe_config.h
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _KDRV_VPE_CONFIG_H_
#define _KDRV_VPE_CONFIG_H_

// NOTE: XXXXX
#if defined __UITRON || defined __ECOS
#include "SysKer.h"
#include "dma.h"
#include "Type.h"
#else
#if defined(__FREERTOS)
#include "kwrap/type.h"
#endif
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#endif

// ==================================================================
// Flag PTN
// ==================================================================
extern UINT32 kdrv_vpe_lock_chls;
#define KDRV_IPP_VPE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_VPE_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_VPE_TVPEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_VPE_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_VPE_LOCK_BITS   (kdrv_vpe_lock_chls)//0x0001FFFF
#define KDRV_IPP_VPE_INIT_BITS   (KDRV_IPP_VPE_LOCK_BITS | KDRV_IPP_VPE_HDL_UNLOCK)


// ==================================================================
// Flag ID
// ==================================================================
typedef enum {
	FLG_ID_KDRV_VPE,
	KDRV_VPE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_VPE_FLAG)
} KDRV_VPE_FLAG;


// ==================================================================
// Semaphore ID
// ==================================================================
typedef enum {
	SEMID_KDRV_VPE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_VPE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_VPE_SEM)
} KDRV_VPE_SEM;

/**
    Semphore Configuration Infomation
*/
typedef struct _KDRV_VPE_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_VPE_SEM_TABLE;

extern ID kdrv_vpe_flag_id[KDRV_VPE_FLAG_COUNT];
extern void kdrv_vpe_install_id(void);
extern void kdrv_vpe_uninstall_id(void);
extern void kdrv_vpe_flow_init(void);
SEM_HANDLE *kdrv_vpe_get_sem_id(KDRV_VPE_SEM idx);
ID kdrv_vpe_get_flag_id(KDRV_VPE_FLAG idx);

#endif
