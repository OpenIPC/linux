/**
    Internal header file for ctl_audio

    This file is the header file that define the API and data type for ctl_audio.

    @file       ctl_aud_int.h
    @ingroup    mIDrvSensor
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _CTLAUDINT_H
#define _CTLAUDINT_H

#include "kflow_audiocapture/ctl_aud.h"
#include <plat/top.h>

#define CTL_AUD_DRV_MAX_NUM 4

typedef struct {
	CHAR name[CTL_AUD_NAME_LEN];
	CTL_AUD_DRV_TAB *drv_tab;
} AUD_MAP_TBL;

typedef struct {
	BOOL external;
	AUD_MAP_TBL *map_tbl;
	CTL_AUD_INIT_CFG_OBJ init_cfg_obj;
} CTL_AUD_OBJ, *PCTL_AUD_OBJ;

INT32 ctl_aud_module_open(CTL_AUD_ID id);
INT32 ctl_aud_module_close(CTL_AUD_ID id);
INT32 ctl_aud_module_start(CTL_AUD_ID id);
INT32 ctl_aud_module_stop(CTL_AUD_ID id);
INT32 ctl_aud_module_set_cfg(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);
INT32 ctl_aud_module_get_cfg(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);

#endif
