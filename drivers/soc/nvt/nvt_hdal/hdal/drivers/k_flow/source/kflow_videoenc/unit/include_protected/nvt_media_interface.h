/**
    Header file of NVT Media Interface

    movie interface

    @file       NvtMediaInterface.h

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/
#ifndef _NVTMEDIAINTERFACEDEF_H
#define _NVTMEDIAINTERFACEDEF_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#else
#ifdef __UITRON
#include "Type.h"
#else
#endif
#endif


typedef struct _NMI_UNIT {
	CHAR *Name;
	ER(*Open)(UINT32 Id, UINT32 *pContext);
	ER(*SetParam)(UINT32 Id, UINT32 Param, UINT32 Value);
	ER(*GetParam)(UINT32 Id, UINT32 Param, UINT32 *pValue);
	ER(*Action)(UINT32 Id, UINT32 Action);
	ER(*In)(UINT32 Id, UINT32 *pBuf);
	ER(*Close)(UINT32 Id);
}
NMI_UNIT;

extern BOOL NMI_AddUnit(NMI_UNIT *pUnit);
extern NMI_UNIT *NMI_GetUnit(const CHAR *pUnitName);

#endif
