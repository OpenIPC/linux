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
 *  @file mdrv_scldma_io.h
 *  @brief SCLDMA Driver IOCTL interface
 */


/**
 * \defgroup scldma_group  SCLDMA driver
 * \note
 *
 * sysfs Node: /sys/devices/platform/mscldma1.0/ckfrm
 *
 * sysfs R/W mode: R/W
 *
 * sysfs Usage & Description: R:check trig off count and dma line count W: if 0 reset scldma status ,if echo 1 all reset
 *
 * sysfs Node: /sys/devices/platform/mscldma1.0/cksnp
 *
 * sysfs R/W mode: R/W
 *
 * sysfs Usage & Description: R:check trig off count and dma line count ,W: if 0 reset scldma status ,if echo 1 all reset
 *
 * sysfs Node: /sys/devices/platform/mscldma2.0/ckfrm
 *
 * sysfs R/W mode: R/W
 *
 * sysfs Usage & Description: R:check trig off count and dma line count W: if 0 reset scldma status ,if echo 1 all reset
 *
 * sysfs Node: /sys/devices/platform/mscldma3.0/ckfrmR
 *
 * sysfs R/W mode: R/W
 *
 * sysfs Usage & Description: R:check trig off count and dma line count W: if 0 reset scldma status ,if echo 1 all reset
 *
 * sysfs Node: /sys/devices/platform/mscldma3.0/ckfrmW
 *
 * sysfs R/W mode: R/W
 *
 * sysfs Usage & Description: R:check trig off count and dma line count W: if 0 reset scldma status ,if echo 1 all reset
 *
 * @{
 */

#ifndef _MDRV_SCLDMA_IO_H
#define _MDRV_SCLDMA_IO_H

//=============================================================================
// Includs
//=============================================================================


//=============================================================================
// IOCTRL defines
//=============================================================================
#define IOCTL_SCLDMA_SET_IN_BUFFER_CONFIG_NR            (0)  ///< The IOCTL NR definition, SET_IN_BUFFER_CONFIG
#define IOCTL_SCLDMA_SET_IN_TRIGGER_CONFIG_NR           (1)  ///< The IOCTL NR definition, SET_IN_TRIGGER_CONFIG
#define IOCTL_SCLDMA_SET_OUT_BUFFER_CONFIG_NR           (2)  ///< The IOCTL NR definition, SET_OUT_BUFFER_CONFIG
#define IOCTL_SCLDMA_SET_OUT_TRIGGER_CONFIG_NR          (3)  ///< The IOCTL NR definition, SET_OUT_TRIGGER_CONFIG
#define IOCTL_SCLDMA_GET_IN_ACTIVE_BUFFER_CONFIG_NR     (4)  ///< The IOCTL NR definition, GET_IN_ACTIVE_BUFFER_CONFIG
#define IOCTL_SCLDMA_GET_OUT_ACTIVE_BUFFER_CONFIG_NR    (5)  ///< The IOCTL NR definition, GET_OUT_ACTIVE_BUFFER_CONFIG
#define IOCTL_SCLDMA_GET_PRIVATE_ID_CONFIG_NR           (6)  ///< The IOCTL NR definition, GET_PRIVATE_ID_CONFIG
#define IOCTL_SCLDMA_SET_LOCK_CONFIG_NR                 (7)  ///< The IOCTL NR definition, SET_LOCK_CONFIG
#define IOCTL_SCLDMA_SET_UNLOCK_CONFIG_NR               (8)  ///< The IOCTL NR definition, SET_UNLOCK_CONFIG
#define IOCLT_SCLDMA_GET_VERSION_CONFIG_NR              (9)  ///< The IOCTL NR definition, IOCLT_SCLDMA_GET_VERSION_CONFIG_NR
#define IOCTL_SCLDMA_GET_INFORMATION_CONFIG_NR          (10) ///< The IOCTL NR definition, IOCTL_SCLDMA_GET_INFORMATION_CONFIG_NR
#define IOCTL_SCLDMA_BUFFER_QUEUE_HANDLE_CONFIG_NR      (12) ///<  The IOCTL NR definition, IOCTL_SCLDMA_BUFFER_QUEUE_HANDLE_CONFIG_NR
#define IOCTL_SCLDMA_MAX_NR                             (13)  ///< The Max IOCTL NR for scldma driver

// use 'm' as magic number
#define IOCTL_SCLDMA_MAGIC                       ('2')   ///< The Type definition of IOCTL for scldma driver

/**
* Used to set parameters of in buffer configurate ,buffer address and number,dma mode,color mode,resolution, use ST_IOCTL_SCLDMA_BUFFER_CONFIG.
*/
#define IOCTL_SCLDMA_SET_IN_BUFFER_CONFIG           _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_SET_IN_BUFFER_CONFIG_NR)

/**
* Used to trigger DMA_R device, use ST_IOCTL_SCLDMA_TRIGGER_CONFIG.
*/
#define IOCTL_SCLDMA_SET_IN_TRIGGER_CONFIG          _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_SET_IN_TRIGGER_CONFIG_NR)

/**
* Used to set parameters of out buffer configurate,buffer address and number ,dma mode,color mode,resolution, use ST_IOCTL_SCLDMA_BUFFER_CONFIG.
*/
#define IOCTL_SCLDMA_SET_OUT_BUFFER_CONFIG          _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_SET_OUT_BUFFER_CONFIG_NR)

/**
* Used to trigger DMA_W device, use ST_IOCTL_SCLDMA_TRIGGER_CONFIG.
*/
#define IOCTL_SCLDMA_SET_OUT_TRIGGER_CONFIG         _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_SET_OUT_TRIGGER_CONFIG_NR)

/**
* Used to set Rpoint(by OMX) and get Wpoint with warning flag,it's like buffer control in DMA_R device, use ST_IOCTL_SCLDMA_ACTIVE_BUFFER_CONFIG.
*/
#define IOCTL_SCLDMA_GET_IN_ACTIVE_BUFFER_CONFIG    _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_GET_IN_ACTIVE_BUFFER_CONFIG_NR)

/**
* Used to set Rpoint(by OMX) and get Wpoint with warning flag,it's like buffer control in DMA_W device, use ST_IOCTL_SCLDMA_ACTIVE_BUFFER_CONFIG.
*/
#define IOCTL_SCLDMA_GET_OUT_ACTIVE_BUFFER_CONFIG   _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_GET_OUT_ACTIVE_BUFFER_CONFIG_NR)

/**
* Used to peek queue information and set read flag , use ST_IOCTL_SCLDMA_BUFFER_QUEUE_CONFIG.
*/
#define IOCTL_SCLDMA_BUFFER_QUEUE_HANDLE_CONFIG     _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_BUFFER_QUEUE_HANDLE_CONFIG_NR)
/**
* Used to get mutiinst parameters of private id configurate ,use ST_IOCTL_SCLDMA_PRIVATE_ID_CONFIG.
*/
#define IOCTL_SCLDMA_GET_PRIVATE_ID_CONFIG          _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_GET_PRIVATE_ID_CONFIG_NR)

/**
* Used to set mutiinst parameters of lock configurate, use ST_IOCTL_SCLDMA_LOCK_CONFIG.
*/
#define IOCTL_SCLDMA_SET_LOCK_CONFIG                _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_SET_LOCK_CONFIG_NR)

/**
* Used to set mutiinst parameters of unlock configurate, use ST_IOCTL_SCLDMA_LOCK_CONFIG.
*/
#define IOCTL_SCLDMA_SET_UNLOCK_CONFIG              _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_SET_UNLOCK_CONFIG_NR)
/**
* Used to get version, use ST_IOCTL_SCLDMA_GET_VERSION_CONFIG.
*/
#define IOCTL_SCLDMA_GET_VERSION_CONFIG            _IO(IOCTL_SCLDMA_MAGIC,  IOCLT_SCLDMA_GET_VERSION_CONFIG_NR)
/**
* Used to get DMA information, use ST_IOCTL_SCLDMA_GET_INFORMATION_CONFIG.
*/
#define IOCTL_SCLDMA_GET_INFORMATION_CONFIG     _IO(IOCTL_SCLDMA_MAGIC,  IOCTL_SCLDMA_GET_INFORMATION_CONFIG_NR)

#endif //

/** @} */ // end of scldma_group
