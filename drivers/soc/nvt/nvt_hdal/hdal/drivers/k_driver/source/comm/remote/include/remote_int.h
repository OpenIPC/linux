/*
    Internal header file for remote controller module.

    This file is the internal header file that define the APIs and data type for remote
    controller module.

    @file       remote.h
    @ingroup    mIREMOTE
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _REMOTE_INT_H_
#define _REMOTE_INT_H_

#include "remote_platform.h"

#define REMOTE_DEBUG                DISABLE

#define REMOTE_SETREG(ofs,value)    OUTW(IOADDR_REMOTE_REG_BASE+(ofs),value)
#define REMOTE_GETREG(ofs)          INW(IOADDR_REMOTE_REG_BASE+(ofs))

typedef enum {
	REMOTE_STATUS_CLOSED,
	REMOTE_STATUS_OPEN,

	ENUM_DUMMY4WORD(REMOTE_STATUS)
} REMOTE_STATUS;


typedef enum {
	REMOTE_WAKEUP_SRC,
	REMOTE_WAKEUP_ORIGINAL_INTEN,

	REMOTE_WAKEUP_PARAM_MAX,
	ENUM_DUMMY4WORD(REMOTE_WAKEUP_PARAM)
} REMOTE_WAKEUP_PARAM;

void remote_isr(void);

#endif
