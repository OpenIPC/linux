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
/// @file   MDRV_HWNAT_UTIL.h
/// @brief  NOE Driver
/// @author MStar Semiconductor Inc.
///
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _MDRV_HWNAT_UTIL_H_
#define _MDRV_HWNAT_UTIL_H_

//-------------------------------------------------------------------------------------------------
//  Include files
//-------------------------------------------------------------------------------------------------

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>



uint8_t *MDrv_HWNAT_Util_Ip_To_Str(uint32_t Ip);
unsigned int MDrv_HWNAT_Util_Str_To_Ip(char *str);
void MDrv_HWNAT_Util_Calc_Tcphdr(struct foe_entry *entry, struct iphdr *iph, struct tcphdr *th);
void MDrv_HWNAT_Util_Calc_Udphdr(struct foe_entry *entry, struct iphdr *iph, struct udphdr *uh);
void MDrv_HWNAT_Util_Calc_Iphdr(struct foe_entry *entry, struct iphdr *iph);
void MDrv_HWNAT_Util_Memcpy(void *dest, void *src, u32 n);
void MDrv_HWNAT_CalIpRange(uint32_t StartIp, uint32_t EndIp, uint8_t * M, uint8_t * E);
unsigned int MDrv_HWNAT_Str2Ip(char *str);



#endif
