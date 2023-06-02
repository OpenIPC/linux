/*
    LVDS/MIPI-CSI/HiSPi Sensor PHY Configuration Driver internal header

    LVDS/MIPI-CSI/HiSPi Sensor PHY Configuration Driver internal header

    @file       senphy_int.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _SENPHY_INT_H
#define _SENPHY_INT_H

#ifdef __KERNEL__
#include "../senphy.h"
#include "senphy_reg.h"
#include "senphy_drv.h"

#define _SENPHY_TODO_

#else
#include "../senphy.h"
#include "include/senphy_reg.h"
#include "rcw_macro.h"
#include "io_address.h"
#include "pll.h"
#include "pll_protected.h"
#endif


#ifdef __KERNEL__
extern UINT32 _SENPHY_REG_BASE_ADDR[MODULE_REG_NUM];
#define SENPHY_GETREG(ofs)             ioread32((void *)(_SENPHY_REG_BASE_ADDR[0] + ofs))
#define SENPHY_SETREG(ofs, value)       iowrite32(value, (void *)(_SENPHY_REG_BASE_ADDR[0] + ofs))
#else
#define SENPHY_GETREG(ofs)             INW( IOADDR_SENPHY_REG_BASE+(ofs))
#define SENPHY_SETREG(ofs,value)       OUTW(IOADDR_SENPHY_REG_BASE+(ofs),(value))

#define SENPHYECO_GETREG(ofs)          INW((0xF0C00000 + 0x18))
#define SENPHYECO_SETREG(ofs, value)   OUTW((0xF0C00000 + 0x18)+(ofs),(value))

#endif

#ifdef _BSP_NA51055_
#define SENPHY_MODULES_NUMBER           2
#define SENPHY_DATALANE_NUMBER          4
#else
#define SENPHY_MODULES_NUMBER           8
#define SENPHY_DATALANE_NUMBER          8
#endif


#define SENPHY_DEBUG                   DISABLE
#if SENPHY_DEBUG
#define senphy_debug(msg)              DBG_WRN  msg
#else
#define senphy_debug(msg)

#endif




#endif
