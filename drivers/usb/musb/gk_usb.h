/*
 * Copyright (C) 2015 by GOKE LIMITED
 *
 * The Inventra Controller Driver for Linux is free software; you
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 2 as published by the Free Software
 * Foundation.
 */

#ifndef __GK_USB_H__
#define __GK_USB_H__

#include <mach/hardware.h>


typedef union { /* PLL_POWER_CONTROL */
    u32 all;
    struct {
        u32 unkown1                     : 1;
        u32 usb                         : 1;
        u32 unkown2                     : 30;
    } bitc;
} GH_PLL_POWER_CONTROL_S;



int gk_usb_phy_init(void);



#endif	/* __GK_USB_H__ */
