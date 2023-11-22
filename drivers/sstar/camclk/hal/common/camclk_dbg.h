/*
 * camclk_dbg.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _CAMCLK_DBG_H
#define _CAMCLK_DBG_H
#include "cam_os_wrapper.h"
#ifndef KERN_SOH
#define KERN_SOH       "\001" /* ASCII Start Of Header */
#define KERN_SOH_ASCII '\001'

#define KERN_EMERG   KERN_SOH "0" /* system is unusable */
#define KERN_ALERT   KERN_SOH "1" /* action must be taken immediately */
#define KERN_CRIT    KERN_SOH "2" /* critical conditions */
#define KERN_ERR     KERN_SOH "3" /* error conditions */
#define KERN_WARNING KERN_SOH "4" /* warning conditions */
#define KERN_NOTICE  KERN_SOH "5" /* normal but significant condition */
#define KERN_INFO    KERN_SOH "6" /* informational */
#define KERN_DEBUG   KERN_SOH "7" /* debug-level messages */

#endif

#define ASCII_COLOR_RED    "\033[1;31m"
#define ASCII_COLOR_WHITE  "\033[1;37m"
#define ASCII_COLOR_YELLOW "\033[1;33m"
#define ASCII_COLOR_BLUE   "\033[1;36m"
#define ASCII_COLOR_GREEN  "\033[1;32m"
#define ASCII_COLOR_END    "\033[0m"
extern u32 gCAMCLKDbgLvl;
typedef enum
{
    CAMCLK_DBG_NONE = 0,
    CAMCLK_DBG_LVL0 = 0x1,
    CAMCLK_DBG_LVL1 = 0x2,
    CAMCLK_DBG_LVL2 = 0x4,
    CAMCLK_DBG_LVL3 = 0x8,
} DrvCAMCLKDbgLvl_e;

#define CAMCLK_DEBUG               (gCAMCLKDbgLvl)
#define CAMCLK_DEBUG_DEFAULT       KERN_DEBUG
#define CAMCLKERR(_fmt, _args...)  CamOsPrintf(ASCII_COLOR_RED _fmt ASCII_COLOR_END, ##_args)
#define CAMCLKWARN(_fmt, _args...) CamOsPrintf(CAMCLK_DEBUG_DEFAULT _fmt, ##_args)
#define CAMCLKINFO(_fmt, _args...) \
    ((CAMCLK_DEBUG & CAMCLK_DBG_LVL0) ? CamOsPrintf(CAMCLK_DEBUG_DEFAULT _fmt, ##_args) : CAMCLK_DEBUG)
#define CAMCLKDBGERR(fmt, args...) \
    ((CAMCLK_DEBUG & CAMCLK_DBG_LVL1) ? CamOsPrintf(CAMCLK_DEBUG_DEFAULT fmt, ##args) : CAMCLK_DEBUG)
#define CAMCLKDBG(fmt, args...) \
    ((CAMCLK_DEBUG & CAMCLK_DBG_LVL2) ? CamOsPrintf(CAMCLK_DEBUG_DEFAULT fmt, ##args) : CAMCLK_DEBUG)
#define CAMCLKREGDUMP(fmt, args...) \
    ((CAMCLK_DEBUG & CAMCLK_DBG_LVL3) ? CamOsPrintf(CAMCLK_DEBUG_DEFAULT fmt, ##args) : CAMCLK_DEBUG)
#endif /* _CAMCLK_DBG_H */
