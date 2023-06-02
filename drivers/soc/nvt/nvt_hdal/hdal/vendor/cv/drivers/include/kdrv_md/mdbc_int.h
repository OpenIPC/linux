/*
    Internal header file for MDBC module.

    This file is the header file that define the API and data type for MDBC
    module.

    @file       mdbc_int.h
    @ingroup    mIDrvIPP_MDBC

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _MDBC_INT_H
#define _MDBC_INT_H

#include "mdbc_lib.h"

#define MDBC_DMA_CACHE_HANDLE        (1)

#define MDBC_CLK_240     240
#define MDBC_CLK_192     192
#define MDBC_CLK_96   	  96

#define MDBC_CLK_320     0
#define MDBC_CLK_PLL13   0

/**
    MDBC Engine Status.

    Set MDBC status.
*/
//@{
typedef enum {
	MDBC_STATUS_IDLE  = 0,   ///< engine is idle
	MDBC_STATUS_READY = 1,   ///< engine is ready
	MDBC_STATUS_PAUSE = 2,   ///< engine is paused
	MDBC_STATUS_RUN   = 3,   ///< engine is running
	ENUM_DUMMY4WORD(MDBC_ENGINE_STATUS)
} MDBC_ENGINE_STATUS;
//@}

// Structure definition
/**
    MDBC Engine Operation Selection.

    Select MDBC module operation.
*/
//@{
typedef enum {
	MDBC_OP_OPEN         = 0,   ///< open module
	MDBC_OP_CLOSE        = 1,   ///< close module
	MDBC_OP_SETMODE      = 2,   ///< set module to ready state
	MDBC_OP_PAUSE        = 3,   ///< pause module
	MDBC_OP_RUN          = 4,   ///< set module to run
	MDBC_OP_ATTACH       = 5,   ///< attach module (enable clock)
	MDBC_OP_DETACH       = 6,   ///< detach module (disable clock)
	ENUM_DUMMY4WORD(MDBC_ENGINE_OPERATION)
} MDBC_ENGINE_OPERATION;
//@}


#endif
