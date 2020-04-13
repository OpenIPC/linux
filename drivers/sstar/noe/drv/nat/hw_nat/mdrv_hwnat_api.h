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
/// @file   MDRV_HWNAT_API.h
/// @brief  NOE NAT Driver
/// @author MStar Semiconductor Inc.
///
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _MDRV_HWNAT_API_H
#define _MDRV_HWNAT_API_H

int MDrv_HWNAT_Get_Ppe_Entry_Idx(struct foe_pri_key *key, struct foe_entry *entry, int del);
int MDrv_HWNAT_Get_Mib_Entry_Idx(struct foe_pri_key *key, struct foe_entry *entry);
#endif /* _MDRV_HWNAT_API_H */
