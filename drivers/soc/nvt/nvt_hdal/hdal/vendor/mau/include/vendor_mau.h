/**
    Public header file for dal_ive

    This file is the header file that define the API and data type for vendor_ive.

    @file       vendor_ive.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.    All rights reserved.
*/
#ifndef _VENDOR_MAU_H_
#define _VENDOR_MAU_H_

#include "hd_type.h"


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
//ch: 0: CPU, 1: CNN, 2: CNN2, 3: NUE, 4: NUE2, 5: ISE
//rw: 0: write, 1: read, 2: both
//dram: 0: DRAM1, 1: DRAM2
extern INT32 vendor_mau_ch_mon_start(int ch, int rw, int dram);
extern UINT64 vendor_mau_ch_mon_stop(int ch, int dram);

#endif //_VENDOR_MAU_H_
