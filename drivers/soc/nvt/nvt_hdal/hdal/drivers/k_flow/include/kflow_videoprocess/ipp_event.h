#ifndef _IPL_EVENT_H_
#define _IPL_EVENT_H_

/**
    ipl_event.h


    @file       ipl_event.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kwrap/type.h"

#define IPP_EVENT_ONE_SHOT	0x00010000

#define IPP_EVENT_START_TAG 0x00000000
#define IPP_EVENT_ISR_TAG 0x10000000

#define IPP_EVENT_OK 0x00000001
#define IPP_EVENT_NG 0x00000002
/**
	IPPevent parameter
*/

/**
     IPP event function prototype
*/
typedef INT32 (*IPP_EVENT_FP)(UINT32 event, void *p_in, void *p_out);
#endif //_IPL_EVENT_H_