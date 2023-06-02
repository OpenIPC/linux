////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  @file mdrv_pnl_io.h
 *  @brief PNL Driver IOCTL interface
 */

/**
 * \defgroup pnl_group  PNL driver
 * @{
 */
#ifndef _MDRV_PNL_IO_H
#define _MDRV_PNL_IO_H

//=============================================================================
// Includs
//=============================================================================


//=============================================================================
// IOCTRL defines
//=============================================================================


#define IOCTL_PNL_SET_TIMING_CONFIG_NR          (0)///< The IOCTL NR definition,IOCTL_PNL_SET_TIMING_CONFIG
#define IOCTL_PNL_SET_LPLL_CONFIG_NR          (1)///< The IOCTL NR definition,IOCTL_PNL_SET_TIMING_CONFIG
#define IOCLT_PNL_GET_VERSION_CONFIG_NR         (2)///< The IOCTL NR definition, IOCTL_PNL_GET_VERSION_CONFIG
#define IOCTL_PNL_MAX_NR                        (IOCLT_PNL_GET_VERSION_CONFIG_NR+1)///< The Max IOCTL NR for pnl driver

// use 'm' as magic number
#define IOCTL_PNL_MAGIC                         ('8')///< The Type definition of IOCTL for pnl driver
/**
* Used to set Panel timing LPLL timing, use ST_IOCTL_HVSP_INPUT_CONFIG.
*/
#define IOCTL_PNL_SET_TIMING_CONFIG             _IO(IOCTL_PNL_MAGIC,  IOCTL_PNL_SET_TIMING_CONFIG_NR)
/**
* Used to set LPLL timing, use ST_IOCTL_HVSP_INPUT_CONFIG.
*/
#define IOCTL_PNL_SET_LPLL_CONFIG             _IO(IOCTL_PNL_MAGIC,  IOCTL_PNL_SET_LPLL_CONFIG_NR)


/**
* Used to get version, use ST_IOCTL_PNL_GET_VERSION_CONFIG.
*/
#define IOCTL_PNL_GET_VERSION_CONFIG            _IO(IOCTL_PNL_MAGIC,  IOCLT_PNL_GET_VERSION_CONFIG_NR)

#endif //
/** @} */ // end of pnl_group
