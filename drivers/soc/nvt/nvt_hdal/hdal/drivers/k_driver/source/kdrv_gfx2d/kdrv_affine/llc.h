/*
    Link-list Controller module

    @file       llc.c
    @ingroup    mIDrvIPP_Affine
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef   _LLC_H
#define   _LLC_H

#include <kdrv_type.h>

#define AFFINE_LL_BUF_SIZE            (4096)

/**
    LLC Cmd id

    @note For
*/
#define LLC_CMD_ID_NULL			0x0
#define LLC_CMD_ID_UPDATE		0x4
#define LLC_CMD_ID_NEXT_LL		0x1
#define LLC_CMD_ID_NEXT_UPDATE	0x2 

/**
    LLC Cmd Null

    @note For
*/
typedef union {
	UINT64 LLC_Cmd;
	struct {
		UINT64 bTable_Index		: 8;	// bits : 7_0
		UINT64 bReserved0		: 53;
		UINT64 bCmd				: 3;	// bits : 63_61
	} Bit;
} LLC_NULL;

/**
    LLC Cmd Update

    @note For
*/
typedef union {
	UINT64 LLC_Cmd;
	struct {
		UINT64 bReg_Val			: 32;	// bits : 31_0
		UINT64 bReg_Ofs			: 12;	// bits : 43_32
		UINT64 bByte_En			: 4;	// bits : 47_44
		UINT64 bReserved0		: 13;
		UINT64 bCmd				: 3;	// bits : 63_61
	} Bit;
} LLC_UPDATE;

/**
    LLC Cmd Next LL

    @note For
*/
typedef union {
	UINT64 LLC_Cmd;
	struct {
		UINT64 bTable_Index		: 8;	// bits : 7_0
		UINT64 bNext_Job_Addr	: 32;	// bits : 39_8
		UINT64 bReserved0		: 21;
		UINT64 bCmd				: 3;	// bits : 63_61
	} Bit;
} LLC_NEXT_LL;

/**
    LLC Cmd Next Update

    @note For
*/
typedef union {
	UINT64 LLC_Cmd;
	struct {
		UINT64 bReserved0		: 8;
		UINT64 bNext_Cmd_Addr	: 32;	// bits : 39_8
		UINT64 bReserved1		: 21;
		UINT64 bCmd				: 3;	// bits : 63_61
	} Bit;
} LLC_NEXT_UPDATE;


//
// Functions exported from llc
//
extern UINT64 llc_null(UINT32 tableIdx);

extern UINT64 llc_update(UINT32 byteEn, UINT32 regOfs, UINT32 regVal);

extern UINT64 llc_next_ll(UINT32 nextJobAddr, UINT32 tableIdx);

extern UINT64 llc_next_update(UINT32 nextCmdAddr);


#endif
