/*
    SENSOR CTL configuration header file.

    SENSOR CTL configuration header file. Define semaphore ID, flag ID, etc.

    @file       ctl_sen_config.h
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _CTL_SEN_CONFIG_H
#define _CTL_SEN_CONFIG_H

#include <kwrap/platform.h>
/*******************************************************************************/
/*                            Platform ITEMs                                   */
/*******************************************************************************/
#if defined(_BSP_NA51089_)
#define CTL_SEN_NA51089
#else
#error !!¡Óinclude <kwrap/platform.h> is required.!!L
#endif
/*******************************************************************************/
/*                               OS ITEMs                                      */
/*******************************************************************************/

#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "kwrap/stdio.h"

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#else
#include <linux/init.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#endif
// ==================================================================
// Flag PTN
// ==================================================================
/**
    0x000000FF -> set Sensor flag
    0xFFFFFF00 -> the special flag for get status and current mode
*/
#define CTL_SEN_FLAG_OFS 8
#define CTL_SEN_FLAG_GET_STATUS_CNT         (CTL_SEN_FLAG_OFS << 0)
#define CTL_SEN_FLAG_GET_STATUS_OFS_LOCK    (CTL_SEN_FLAG_OFS << 1)
#define CTL_SEN_FLAG_GET_STATUS_OFS_UNLOCK  (CTL_SEN_FLAG_OFS << 2)
#define CTL_SEN_FLAG_ALL 0x00FFFFFF

// global
#define CTL_SEN_FLAG_GLB_OFS_INIT 1
#define CTL_SEN_FLAG_GLB_ALL (FLGPTN_BIT(CTL_SEN_FLAG_GLB_OFS_INIT))

// ==================================================================
// Flag ID
// ==================================================================
typedef enum {
	FLG_ID_CTL_SEN,
	FLG_ID_CTL_SEN_GLB,
	CTL_SEN_FLAG_COUNT,
	ENUM_DUMMY4WORD(CTL_SEN_FLAG)
} CTL_SEN_FLAG;

// ==================================================================
// Semaphore ID
// ==================================================================
typedef enum {
	SEMID_CTL_SEN1, // = CTL_SEN_ID_1,
	SEMID_CTL_SEN2, // = CTL_SEN_ID_2,
	SEMID_CTL_SEN3, // = CTL_SEN_ID_3,
	SEMID_CTL_SEN4, // = CTL_SEN_ID_4,
	SEMID_CTL_SEN5, // = CTL_SEN_ID_5,
	SEMID_CTL_SEN6, // = CTL_SEN_ID_6,
	SEMID_CTL_SEN7, // = CTL_SEN_ID_7,
	SEMID_CTL_SEN8, // = CTL_SEN_ID_8,
	// Insert member before this line
	// Don't change the order of following members
	CTL_SEN_SEMAPHORE_COUNT = SEMID_CTL_SEN8 + 1,
	ENUM_DUMMY4WORD(CTL_SEN_SEM)
} CTL_SEN_SEM;

//@}


/**
    Semphore Configuration Infomation
*/
typedef struct {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID
	UINT32      max_counter;
} CTL_SEN_SEM_TABLE;

extern ID  ctl_sen_flag_id[CTL_SEN_FLAG_COUNT];
extern CTL_SEN_SEM_TABLE ctl_sen_semtbl[CTL_SEN_SEMAPHORE_COUNT];
void ctl_sen_install_id(void);
void ctl_sen_uninstall_id(void);
ID ctl_sen_get_flag_id(CTL_SEN_FLAG idx);

SEM_HANDLE *ctl_sen_get_sem_id(CTL_SEN_SEM idx);

#endif
