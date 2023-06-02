/**
	   SIE event

    .

    @file       ctl_sie_event_id_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _CTL_SIE_EVENT_ID_INT_H_
#define _CTL_SIE_EVENT_ID_INT_H_

#if defined (__LINUX)
#include <linux/string.h>
#elif defined (__FREERTOS)
#include <string.h>
#endif
#include "kwrap/flag.h"
#include "ctl_sie_event_int.h"

#define SIE_EVENT_POOL_LOCK		FLGPTN_BIT(0)

extern ID sie_event_flg_id[SIE_EVENT_FLAG_MAX];
extern void sie_event_install_id(void);
extern void sie_event_uninstall_id(void);
#endif //_CTL_SIE_EVENT_ID_INT_H_

