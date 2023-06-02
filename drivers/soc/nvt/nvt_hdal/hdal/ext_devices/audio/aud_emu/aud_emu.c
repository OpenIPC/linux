/*
    Emaltion External audio codec driver

    This file is the driver for Emaltion extended audio codec.

    @file       aud_emu.c
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

/** \addtogroup mISYSAud */
//@{

#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "aud_emu_dbg.h"
#include "aud_emu.h"
#else
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "aud_emu_dbg.h"
#include "aud_emu.h"
#endif

unsigned int aud_emu_debug_level = NVT_DBG_WRN;

static ER aud_open_emu(CTL_AUD_ID id);                                               ///< initial sensor flow
static ER aud_close_emu(CTL_AUD_ID id);                                              ///< un-initial sensor flow
static ER aud_start_emu(CTL_AUD_ID id);                                              ///< enter sensor sleep mode
static ER aud_stop_emu(CTL_AUD_ID id);                                               ///< exit sensor sleep mode
static ER aud_set_cfg_emu(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);   ///< set sensor information (if sensor driver not support feature, pls return E_NOSPT)
static ER aud_get_cfg_emu(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data);   ///< get sensor information (if sensor driver not support feature, pls return E_NOSPT)


static CTL_AUD_DRV_TAB emu_aud_drv_tab = {
	aud_open_emu,
	aud_close_emu,
	aud_start_emu,
	aud_stop_emu,
	aud_set_cfg_emu,
	aud_get_cfg_emu,
};


CTL_AUD_DRV_TAB *aud_get_drv_tab_emu(void)
{
	return &emu_aud_drv_tab;
}

static ER aud_open_emu(CTL_AUD_ID id)
{
	return E_OK;
}

static ER aud_close_emu(CTL_AUD_ID id)
{
	return E_OK;
}
static ER aud_start_emu(CTL_AUD_ID id)
{
	return E_OK;
}
static ER aud_stop_emu(CTL_AUD_ID id)
{
	return E_OK;
}

static ER aud_set_cfg_emu(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data)
{
	return E_OK;
}

static ER aud_get_cfg_emu(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data)
{
	return E_OK;
}

int aud_init_emu(void)
{
	INT32 ret;
	CTL_AUD_DRV_TAB *drv_tab = NULL;

	drv_tab = aud_get_drv_tab_emu();

	ret = ctl_aud_reg_auddrv("nvt_aud_emu", drv_tab);
	if (ret != E_OK) {
		DBG_ERR("register audio driver fail \r\n");
	}

	return ret;
}


//@}
