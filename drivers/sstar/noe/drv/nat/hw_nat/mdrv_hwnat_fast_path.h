/*
 * mdrv_hwnat_fast_path.h- Sigmastar
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
#ifndef _MDRV_HWNAT_FASTPATH_H
#define _MDRV_HWNAT_FASTPATH_H

inline int32_t MDrv_HWNAT_Pptp_Lan(struct sk_buff *skb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_Pptp_Wan(struct sk_buff *skb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_L2tp_Lan(struct sk_buff *skb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_L2tp_Wan(struct sk_buff *skb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_Pptp_Lan_Parse_Layer(struct sk_buff *skb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_Pptp_Wan_Parse_Layer(struct sk_buff *skb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_L2tp_Wan_Parse_Layer(struct sk_buff *skb)
{
    return 1;
}
inline void MDrv_HWNAT_Pptp_L2tp_Update_Entry(uint8_t protocol, uint32_t addr, uint32_t src_ip, uint32_t foe_hash_index)
{
}
inline int32_t MDrv_HWNAT_Send_Hash_Pkt(struct sk_buff *pskb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_L2tp_Send_Hash_Pkt(struct sk_buff *pskb)
{
    return 1;
}
inline int32_t MDrv_HWNAT_Pptp_L2tp_Init(void)
{
    return 1;
}
inline int32_t MDrv_HWNAT_Pptp_L2tp_Clean(void)
{
    return 1;
}
#endif /* _MDRV_HWNAT_FASTPATH_H */
