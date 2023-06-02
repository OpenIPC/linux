/**
	   IPP event

    .

    @file       ipp_event_id_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _IPP_EVENT_ID_INT_H_
#define _IPP_EVENT_ID_INT_H_

#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "ipp_event_int.h"

#define IPP_EVENT_POOL_LOCK		FLGPTN_BIT(0)

extern void ipp_event_install_id(void);
extern void ipp_event_uninstall_id(void);
#endif //_IPP_EVENT_ID_INT_H_

