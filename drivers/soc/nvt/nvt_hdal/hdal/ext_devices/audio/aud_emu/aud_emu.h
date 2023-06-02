/*
    EMU audio codec internal header
    EMU audio codec internal header file

    @file       aud_emu.h
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
/** \addtogroup mISYSAud */
//@{

#ifndef _AUD_EMU_H
#define _AUD_EMU_H

#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "aud_emu_dbg.h"
#include <linux/i2c.h>
#include <linux/slab.h>

#include "kflow_audiocapture/ctl_aud.h"
#else
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "aud_emu_dbg.h"
#include "kflow_audiocapture/ctl_aud.h"
#include "rtos_na51089/kdrv_i2c.h"
#endif

ER aud_i2c_init_driver(UINT32 i2c_id);
void aud_i2c_remove_driver(UINT32 id);
INT32 aud_i2c_transfer(struct i2c_msg *msgs, INT32 num);
CTL_AUD_DRV_TAB *aud_get_drv_tab_emu(void);
int aud_init_emu(void);

#endif

//@}
