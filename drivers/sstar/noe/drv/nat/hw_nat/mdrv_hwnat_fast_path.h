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
/// @file   MDRV_NOE_NAT.h
/// @brief  NOE NAT Driver
/// @author MStar Semiconductor Inc.
///
///////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _MDRV_HWNAT_FASTPATH_H
#define _MDRV_HWNAT_FASTPATH_H

inline int32_t MDrv_HWNAT_Pptp_Lan(struct sk_buff *skb) {return 1;}
inline int32_t MDrv_HWNAT_Pptp_Wan(struct sk_buff *skb) {return 1;}
inline int32_t MDrv_HWNAT_L2tp_Lan(struct sk_buff *skb) {return 1;}
inline int32_t MDrv_HWNAT_L2tp_Wan(struct sk_buff *skb) {return 1;}
inline int32_t MDrv_HWNAT_Pptp_Lan_Parse_Layer(struct sk_buff * skb) {return 1;}
inline int32_t MDrv_HWNAT_Pptp_Wan_Parse_Layer(struct sk_buff * skb) {return 1;}
inline int32_t MDrv_HWNAT_L2tp_Wan_Parse_Layer(struct sk_buff * skb) {return 1;}
inline void MDrv_HWNAT_Pptp_L2tp_Update_Entry(uint8_t protocol, uint32_t addr,uint32_t src_ip, uint32_t foe_hash_index){}
inline int32_t MDrv_HWNAT_Send_Hash_Pkt(struct sk_buff *pskb) {return 1;}
inline int32_t MDrv_HWNAT_L2tp_Send_Hash_Pkt(struct sk_buff *pskb) {return 1;}
inline int32_t MDrv_HWNAT_Pptp_L2tp_Init(void) {return 1;}
inline int32_t MDrv_HWNAT_Pptp_L2tp_Clean(void) {return 1;}
#endif  /* _MDRV_HWNAT_FASTPATH_H */
