#ifndef _CTL_SIE_EVENT_H_
#define _CTL_SIE_EVENT_H_

/**
    ctl_sie_event.h


    @file       ctl_sie_event.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kwrap/type.h"

/*
    SIEevent parameter
*/
#define CTL_SIE_EVENT_ONE_SHOT  0x00010000

#define CTL_SIE_EVENT_ISR_TAG   0x10000000

#define CTL_SIE_EVENT_OK        0x00000001
#define CTL_SIE_EVENT_NG        0x00000002

/*
     SIE event function prototype
*/
typedef INT32(*CTL_SIE_EVENT_FP)(UINT32 event, void *p_in, void *p_out);
#endif //_CTL_SIE_EVENT_H_
