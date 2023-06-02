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
 *  @file mdrv_pnl_io_st.h
 *  @brief pnl Driver IOCTL parameter interface
 */

 /**
 * \ingroup pnl_group
 * @{
 */

#ifndef _MDRV_PNL_IO_ST_H
#define _MDRV_PNL_IO_ST_H

//=============================================================================
// Defines
//=============================================================================
// library information



//=============================================================================
// enum
//=============================================================================
#define IOCTL_PNL_VERSION                        0x0100

//=============================================================================
// struct
//=============================================================================
/**
* Used to setup the panel timing of pnl device
*/
typedef struct
{
    unsigned int   VerChk_Version ; ///< VerChk version
    unsigned short u16Vsync_St; ///< vsync start point
    unsigned short u16Vsync_End;///< vsync end point
    unsigned short u16Vde_St;   ///< Vdata enable start point
    unsigned short u16Vde_End;  ///< Vdata enable end point
    unsigned short u16Vfde_St;  ///< V framecolor data enable start point
    unsigned short u16Vfde_End; ///< V framecolor data enable end point
    unsigned short u16Vtt;      ///< V total
    unsigned short u16Hsync_St; ///< hsync start point
    unsigned short u16Hsync_End;///< hsync end point
    unsigned short u16Hde_St;   ///< Hdata enable start point
    unsigned short u16Hde_End;  ///< Hdata enable end point
    unsigned short u16Hfde_St;  ///< H framecolor data enable start point
    unsigned short u16Hfde_End; ///< H framecolor data enable end point
    unsigned short u16Htt;      ///< H total
    unsigned short u16VFreqx10; ///< FPS x10

    // VerChk_Version & VerChk_Size must be the latest 2 parameter and
    // the order can't be changed
    unsigned int   VerChk_Size; ///< VerChk Size
}__attribute__ ((__packed__)) ST_IOCLT_PNL_TIMING_CONFIG;


/**
* Used to get PNL drvier version
*/
typedef struct
{
    unsigned int   VerChk_Version ; ///< VerChk version
    unsigned int   u32Version;      ///< version
    unsigned int   VerChk_Size;     ///< VerChk Size
}__attribute__ ((__packed__)) ST_IOCTL_PNL_VERSION_CONFIG;
//=============================================================================

//=============================================================================
#endif //
/** @} */ // end of pnl_group
