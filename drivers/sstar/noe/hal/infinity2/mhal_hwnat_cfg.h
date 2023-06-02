////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (“MStar Confidential Information”) by the recipien
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   MHAL_NOE_HWNAT_CFG.h
/// @brief  NOE Driver
/// @author MStar Semiconductor Inc.
///
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MHAL_NOE_HWNAT_CFG_H_
#define _MHAL_NOE_HWNAT_CFG_H_



#define MHAL_HWNAT_LMT_QURT     (16383)
#define MHAL_HWNAT_LMT_HALF     (16383)
#define MHAL_HWNAT_LMT_FULL     (16383)
#define MHAL_HWNAT_KA_T         (1)

#define HASH_SEED               (0x12345678)
#define DFL_FOE_UNB_AGE     1   /* Unbind state age enable */
#define DFL_FOE_TCP_AGE     1   /* Bind TCP age enable */
#define DFL_FOE_NTU_AGE     1   /* Bind TCP age enable */
#define DFL_FOE_UDP_AGE     1   /* Bind UDP age enable */
#define DFL_FOE_FIN_AGE     1   /* Bind TCP FIN age enable */




#endif /* _MHAL_NOE_HWNAT_CFG_H_ */


