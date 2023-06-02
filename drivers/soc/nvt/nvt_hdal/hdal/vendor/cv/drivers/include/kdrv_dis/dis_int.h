/*
    Internal header file for DIS module.

    This file is the header file that define the API and data type for DIS
    module.

    @file       dis_int.h
    @ingroup    mIDrvIPP_DIS

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _DIS_INT_H
#define _DIS_INT_H

#include    "dis_platform.h"
#include    "dis_lib.h"

extern UINT32 _DIS_REG_BASE_ADDR;

#define IOADDR_DIS_REG_BASE         _DIS_REG_BASE_ADDR


#define DIS_DMA_CACHE_HANDLE        (1)
#define DIS_ABS(x)                  ((x)>=0?(x):(-(x)))
#define DIS_OUTPUT_BUF_SIZE         (12*1024)

// DIS Motion Information
#define DIS_MVX             0x0000003F  //bit 5:0
#define DIS_MVY             0x00003F00  //bit 13:8
#define DIS_SAD             0x7FFF0000  //bit 30:16
#define DIS_CNT             0x00000FFF  //bit 11:0
#define DIS_IDX             0x1FFF0000  //bit 28:16

/**
    DIS Engine Status.

    Set DIS status.
*/
//@{
typedef enum {
	DIS_ENGINE_IDLE  = 0,   ///< engine is idle
	DIS_ENGINE_READY = 1,   ///< engine is ready
	DIS_ENGINE_PAUSE = 2,   ///< engine is paused
	DIS_ENGINE_RUN   = 3,   ///< engine is running
	ENUM_DUMMY4WORD(DIS_ENGINE_STATUS)
} DIS_ENGINE_STATUS;
//@}

// Structure definition
/**
    DIS Engine Operation Selection.

    Select DIS module operation.
*/
//@{
typedef enum {
	DIS_ENGINE_OPEN         = 0,   ///< open module
	DIS_ENGINE_CLOSE        = 1,   ///< close module
	DIS_ENGINE_SET2READY    = 2,   ///< set module to ready state
	DIS_ENGINE_SET2PRV      = 3,   ///< set module to preview mode
	DIS_ENGINE_SET2PAUSE    = 5,   ///< pause module
	DIS_ENGINE_SET2RUN      = 6,   ///< set module to run
	DIS_ENGINE_ATTACH       = 7,   ///< attach module (enable clock)
	DIS_ENGINE_DETACH       = 8,   ///< detach module (disable clock)
	DIS_ENGINE_SET2CHGSIZE  = 9,   ///< set module to change size
	DIS_ENGINE_CHGINT       = 10,  ///< set module to change interrupt
	ENUM_DUMMY4WORD(DIS_ENGINE_OPERATION)
} DIS_ENGINE_OPERATION;
//@}


/**
    Struct DIS Size Configuration.

    Structure of size parameters.
*/
//@{
typedef struct _SIZE_CONF {
	UINT32 uiWidth;   ///< width, as in one pixel/bit
	UINT32 uiHeight;  ///< height, as in one pixel/bit
} SIZE_CONF;
//@}

#endif
