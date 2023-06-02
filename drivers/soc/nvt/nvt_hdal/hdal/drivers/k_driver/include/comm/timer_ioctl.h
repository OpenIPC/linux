/**

    The header file of timer ioctl operations.

    @file       timer_ioctl.h
    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __TIMER_IOCTL_H
#define __TIMER_IOCTL_H

#include <asm/ioctl.h>


#define TIMER_IOC_MAGIC 'T'

typedef struct {
    TIMER_ID       id;
    TIMER_MODE     mode;
    TIMER_STATE    state;
    unsigned long  expires;
} TIMER_IOC_CONFIG, *PTIMER_IOC_CONFIG;


typedef struct {
	UINT32         timer_id_all;
    UINT32         event;
} TIMER_IOC_RCV_EVENT, *PTIMER_IOC_RCV_EVENT;


#define IOCTL_TIMER_OPEN                 _IOWR(TIMER_IOC_MAGIC, 0, TIMER_ID)
#define IOCTL_TIMER_GET_CURCOUNT         _IOWR(TIMER_IOC_MAGIC, 1, TIMER_ID)
#define IOCTL_TIMER_CLOSE                _IOWR(TIMER_IOC_MAGIC, 2, TIMER_ID)
#define IOCTL_TIMER_CONFIG               _IOWR(TIMER_IOC_MAGIC, 3, TIMER_IOC_CONFIG)
#define IOCTL_TIMER_WAITTIMEUP           _IOWR(TIMER_IOC_MAGIC, 4, TIMER_ID)
#define IOCTL_TIMER_RCV_EVENT            _IOWR(TIMER_IOC_MAGIC, 5, TIMER_IOC_RCV_EVENT)

#define  COPY_FROM_USER_ERR_STR    "copy_from_user"
#define  COPY_TO_USER_ERR_STR      "copy_to_user"
#endif
