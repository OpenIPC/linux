#ifndef _NVTMPP_ID_H
#define _NVTMPP_ID_H

#if defined __UITRON || defined __ECOS
#include "SysKer.h"
extern UINT32 _SECTION(".kercfg_data") NVTMPP_SEM_ID;
extern UINT32 _SECTION(".kercfg_data") NVTMPP_HEAP_SEM_ID;

#else
#include "kwrap/semaphore.h"
extern SEM_HANDLE NVTMPP_SEM_ID;
extern SEM_HANDLE NVTMPP_HEAP_SEM_ID;
extern SEM_HANDLE NVTMPP_PROC_SEM_ID;
#endif


#endif //_NVTMPP_ID_H

