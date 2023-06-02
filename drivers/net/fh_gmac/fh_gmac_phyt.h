/*
 * fh_gmac_phyt.h
 *
 *  Created on: May 22, 2014
 *      Author: duobao
 */

#ifndef FH_GMAC_PHYT_H_
#define FH_GMAC_PHYT_H_

#define FH_GMAC_PHY_IP101G	0x02430C54
#define FH_GMAC_PHY_RTL8201	0x001CC816
#define FH_GMAC_PHY_TI83848	0xFFFFFFFF

enum
{
	gmac_phyt_speed_10M_half_duplex = 1,
	gmac_phyt_speed_100M_half_duplex = 2,
	gmac_phyt_speed_10M_full_duplex = 5,
	gmac_phyt_speed_100M_full_duplex = 6
};


typedef union
{
	struct
	{
		__u32	reserved_6_0					:7;
		__u32	collision_test					:1;
		__u32	duplex_mode						:1;
		__u32	restart_auto_negotiate			:1;
		__u32	isolate							:1;
		__u32	power_down						:1;
		__u32	auto_negotiate_enable			:1;
		__u32	speed_select					:1;
		__u32	loopback						:1;
		__u32	reset							:1;
		__u32	reserved_31_16					:16;
	}bit;
	__u32 dw;
}Reg_Phyt_Basic_Ctrl;


typedef union
{
	struct
	{
		__u32	extended_capabilities			:1;
		__u32	jabber_detect					:1;
		__u32	link_status						:1;
		__u32	auto_negotiate_ability			:1;
		__u32	remote_fault					:1;
		__u32	auto_negotiate_complete			:1;
		__u32	reserved_10_6					:5;
		__u32	base_t_half_duplex_10			:1;
		__u32	base_t_full_duplex_10			:1;
		__u32	base_tx_half_duplex_100			:1;
		__u32	base_tx_full_duplex_100			:1;
		__u32	base_t_4						:1;
		__u32	reserved_31_16					:16;
	}bit;
	__u32 dw;
}Reg_Phyt_Basic_Status;

typedef union
{
	struct
	{
		__u32	scramble_disable			:1;
		__u32	reserved_1					:1;
		__u32	speed_indication			:3;
		__u32	reserved_5					:1;
		__u32	enable_4b5b					:1;
		__u32	gpo							:3;
		__u32	reserved_11_10				:2;
		__u32	auto_done					:1;
		__u32	reserved_31_13				:19;
	}bit;
	__u32 dw;
}Reg_Phyt_Special_Status;


#endif /* FH_GMAC_PHYT_H_ */
