/**
    Driver common part header file

    @file       Driver.h
    @ingroup    mIDriver
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _DRIVER_H
#define _DRIVER_H

/**
    @addtogroup mIDriver
*/
//@{

#include "kwrap/type.h"

typedef enum {
	DRV_VER_96650,
	DRV_VER_96660,
	DRV_VER_96680,
	DRV_VER_98520,

	ENUM_DUMMY4WORD(DRV_VER_INFO)
} DRV_VER_INFO;

/**
    Driver callback function

    uiEvent is bitwise event, please refer to each module's document.

*/
typedef void (*DRV_CB)(UINT32 event);

//@}

#endif
