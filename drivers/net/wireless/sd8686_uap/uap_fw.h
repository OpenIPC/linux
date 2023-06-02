/** @file uap_fw.h
 *
 * @brief This file contains firmware specific defines.
 *
 * Copyright (C) 2008-2009, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available along with the File in the gpl.txt file or by writing to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 *
 */
/********************************************************
Change log:
	02/26/08: Initial creation
********************************************************/

#ifndef _UAP_FW_H
#define _UAP_FW_H

/** uap upload size */
#define	UAP_UPLD_SIZE			2312
/** Packet type Micro AP */
#define PKT_TYPE_MICROAP		1
/** Packet type client */
#define PKT_TYPE_CLIENT			0

/** TxPD descriptor */
typedef struct _TxPD
{
        /** Bss Type */
    u8 BssType;
        /** Bss num */
    u8 BssNum;
        /** Tx packet length */
    u16 TxPktLength;
        /** Tx packet offset */
    u16 TxPktOffset;
        /** Tx packet type */
    u16 TxPktType;
        /** Tx Control */
    u32 TxControl;
        /** reserved */
    u32 reserved[2];
} __ATTRIB_PACK__ TxPD, *PTxPD;

/** RxPD Descriptor */
typedef struct _RxPD
{
        /** Bss Type */
    u8 BssType;
        /** Bss Num */
    u8 BssNum;
        /** Tx packet length */
    u16 RxPktLength;
        /** Tx packet offset */
    u16 RxPktOffset;
} __ATTRIB_PACK__ RxPD, *PRxPD;

#ifdef BIG_ENDIAN
/** Convert from 16 bit little endian format to CPU format */
#define uap_le16_to_cpu(x) le16_to_cpu(x)
/** Convert from 32 bit little endian format to CPU format */
#define uap_le32_to_cpu(x) le32_to_cpu(x)
/** Convert from 64 bit little endian format to CPU format */
#define uap_le64_to_cpu(x) le64_to_cpu(x)
/** Convert to 16 bit little endian format from CPU format */
#define uap_cpu_to_le16(x) cpu_to_le16(x)
/** Convert to 32 bit little endian format from CPU format */
#define uap_cpu_to_le32(x) cpu_to_le32(x)
/** Convert to 64 bit little endian format from CPU format */
#define uap_cpu_to_le64(x) cpu_to_le64(x)

/** Convert TxPD to little endian format from CPU format */
#define endian_convert_TxPD(x);                                         \
    {                                                                   \
        (x)->TxPktLength = uap_cpu_to_le16((x)->TxPktLength);    	\
        (x)->TxPktOffset = uap_cpu_to_le32((x)->TxPktOffset);		\
        (x)->TxControl = uap_cpu_to_le32((x)->TxControl);              \
        (x)->TxPktType = uap_cpu_to_le32((x)->TxPktType);		\
    }

/** Convert RxPD from little endian format to CPU format */
#define endian_convert_RxPD(x);                             		\
    {                                                       		\
        (x)->RxPktLength = uap_le16_to_cpu((x)->RxPktLength);        	\
        (x)->RxPktOffset = uap_le32_to_cpu((x)->RxPktOffset);  	\
    }
#else /* BIG_ENDIAN */
/** Do nothing */
#define uap_le16_to_cpu(x) x
/** Do nothing */
#define uap_le32_to_cpu(x) x
/** Do nothing */
#define uap_le64_to_cpu(x) x
/** Do nothing */
#define uap_cpu_to_le16(x) x
/** Do nothing */
#define uap_cpu_to_le32(x) x
/** Do nothing */
#define uap_cpu_to_le64(x) x

/** Do nothing */
#define endian_convert_TxPD(x)
/** Do nothing */
#define endian_convert_RxPD(x)
#endif /* BIG_ENDIAN */

/** Host Command ID : Function initialization */
#define HostCmd_CMD_FUNC_INIT                 0x00a9
/** Host Command ID : Function shutdown */
#define HostCmd_CMD_FUNC_SHUTDOWN             0x00aa

/** Host Command id: SYS_INFO  */
#define HOST_CMD_APCMD_SYS_INFO               0x00ae
/** Host Command id: SYS_RESET  */
#define HOST_CMD_APCMD_SYS_RESET              0x00af
/** Host Command id: SYS_CONFIGURE  */
#define HOST_CMD_APCMD_SYS_CONFIGURE          0x00b0
/** Host Command id: BSS_START  */
#define HOST_CMD_APCMD_BSS_START              0x00b1
/** Host Command id: SYS_STOP  */
#define HOST_CMD_APCMD_BSS_STOP               0x00b2
/** Host Command id: STA_LIST  */
#define HOST_CMD_APCMD_STA_LIST               0x00b3
/** Host Command id: STA_FILTER_TABLE  */
#define HOST_CMD_APCMD_STA_FILTER_TABLE       0x00b4
/** Host Command id: STA_DEAUTH  */
#define HOST_CMD_APCMD_STA_DEAUTH             0x00b5
/** Host Command id: SOFT_RESET  */
#define HOST_CMD_APCMD_SOFT_RESET             0x00d5
/** Host Command id: POWER_MGMT_EXT  */
#define HOST_CMD_POWER_MGMT_EXT               0x00ef
/** Host Command id: SLEEP_CONFIRM*/
#define HOST_CMD_SLEEP_CONFIRM       	      0x00d8

/** TLV type : SSID */
#define TLV_TYPE_SSID				0x0000
/** TLV type : Rates */
#define TLV_TYPE_RATES				0x0001
/** TLV type : PHY DS */
#define TLV_TYPE_PHY_DS				0x0003

/** TLV Id : Base id */
#define PROPRIETARY_TLV_BASE_ID               0x0100
/** TLV Id : AP_MAC_ADDRESS */
#define MRVL_AP_MAC_ADDRESS_TLV_ID      (PROPRIETARY_TLV_BASE_ID + 43)
/** TLV Id : Beacon period */
#define MRVL_BEACON_PERIOD_TLV_ID       (PROPRIETARY_TLV_BASE_ID + 44)
/** TLV Id : Dtim period */
#define MRVL_DTIM_PERIOD_TLV_ID         (PROPRIETARY_TLV_BASE_ID + 45)
/** TLV Id : Basic rates */
#define MRVL_BASIC_RATES_TLV_ID         (PROPRIETARY_TLV_BASE_ID + 46)
/** TLV Id : Tx Power */
#define MRVL_TX_POWER_TLV_ID            (PROPRIETARY_TLV_BASE_ID + 47)
/** TLV Id : Broadcast SSID control */
#define MRVL_BCAST_SSID_CTL_TLV_ID      (PROPRIETARY_TLV_BASE_ID + 48)
/** TLV Id : Preamble control */
#define MRVL_PREAMBLE_CTL_TLV_ID        (PROPRIETARY_TLV_BASE_ID + 49)
/** TLV Id : Antenna control */
#define MRVL_ANTENNA_CTL_TLV_ID         (PROPRIETARY_TLV_BASE_ID + 50)
/** TLV Id : RTS threshold */
#define MRVL_RTS_THRESHOLD_TLV_ID       (PROPRIETARY_TLV_BASE_ID + 51)
/** TLV Id : Radio control */
#define MRVL_RADIO_CTL_TLV_ID           (PROPRIETARY_TLV_BASE_ID + 52)
/** TLV Id : TX data rate */
#define MRVL_TX_DATA_RATE_TLV_ID        (PROPRIETARY_TLV_BASE_ID + 53)
/** TLV Id : Packet forward control */
#define MRVL_PKT_FWD_CTL_TLV_ID         (PROPRIETARY_TLV_BASE_ID + 54)
/** TLV Id : STA info */
#define MRVL_STA_INFO_TLV_ID            (PROPRIETARY_TLV_BASE_ID + 55)
/** TLV Id : STA MAC address filter */
#define MRVL_STA_MAC_ADDR_FILTER_TLV_ID (PROPRIETARY_TLV_BASE_ID + 56)
/** TLV Id : STA ageout timer */
#define MRVL_STA_AGEOUT_TIMER_TLV_ID    (PROPRIETARY_TLV_BASE_ID + 57)
/** TLV Id : Security config */
#define MRVL_SECURITY_CFG_TLV_ID        (PROPRIETARY_TLV_BASE_ID + 58)
/** TLV Id : WEP KEY */
#define MRVL_WEP_KEY_TLV_ID             (PROPRIETARY_TLV_BASE_ID + 59)
/** TLV Id : WPA Passphrase */
#define MRVL_WPA_PASSPHRASE_TLV_ID      (PROPRIETARY_TLV_BASE_ID + 60)

/** Action get */
#define ACTION_GET	0
/** Action set */
#define ACTION_SET 	1
/** Length of ethernet address */
#ifndef	ETH_ALEN
#define ETH_ALEN			6
#endif

/** HostCmd_DS_GEN */
typedef struct
{
    /** Command */
    u16 Command;
    /** Size */
    u16 Size;
    /** Sequence number */
    u16 SeqNum;
    /** Result */
    u16 Result;
} __ATTRIB_PACK__ HostCmd_DS_GEN;

/** Size of HostCmd_DS_GEN */
#define S_DS_GEN    sizeof(HostCmd_DS_GEN)

/** _HostCmd_HEADER*/
typedef struct
{
    /** Command Header : Command */
    u16 Command;
    /** Command Header : Size */
    u16 Size;
} __ATTRIB_PACK__ HostCmd_HEADER;

/** HostCmd_SYS_CONFIG */
typedef struct _HostCmd_SYS_CONFIG
{
        /** CMD Action GET/SET*/
    u16 Action;
        /** Tlv buffer */
    u8 TlvBuffer[0];
} __ATTRIB_PACK__ HostCmd_SYS_CONFIG;

/** HostCmd_DS_POWER_MGMT_EXT */
typedef struct _HostCmd_DS_POWER_MGMT_EXT
{
    /** CMD Action Get/Set*/
    u16 action;
    /** power mode */
    u16 power_mode;
} __ATTRIB_PACK__ HostCmd_DS_POWER_MGMT_EXT;

/** _HostCmd_DS_COMMAND*/
typedef struct _HostCmd_DS_COMMAND
{

        /** Command Header : Command */
    u16 Command;
        /** Command Header : Size */
    u16 Size;
        /** Command Header : Sequence number */
    u16 SeqNum;
        /** Command Header : Result */
    u16 Result;
        /** Command Body */
    union
    {
        HostCmd_SYS_CONFIG sys_config;
        HostCmd_DS_POWER_MGMT_EXT pm_cfg;

    } params;
} __ATTRIB_PACK__ HostCmd_DS_COMMAND;

/** MrvlIEtypesHeader_*/
typedef struct _MrvlIEtypesHeader
{
    /** Header type */
    u16 Type;
    /** Header length */
    u16 Len;
} __ATTRIB_PACK__ MrvlIEtypesHeader_t;

/** MrvlIEtypes_Data_t */
typedef struct _MrvlIEtypes_Data_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Data */
    u8 Data[1];
} __ATTRIB_PACK__ MrvlIEtypes_Data_t;

/** MrvlIEtypes_ChanListParamSet_t */
typedef struct _MrvlIEtypes_MacAddr_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** AP MAC address */
    u8 ApMacAddr[ETH_ALEN];
} __ATTRIB_PACK__ MrvlIEtypes_MacAddr_t;

/** Event ID: BSS started */
#define MICRO_AP_EV_ID_BSS_START    46

/** Event ID: BSS idle event */
#define MICRO_AP_EV_BSS_IDLE	     67

/** Event ID: BSS active event */
#define MICRO_AP_EV_BSS_ACTIVE	     68

/** Event ID: PS_AWAKE */
#define EVENT_PS_AWAKE     0x0a

/** Event ID: PS_SLEEP */
#define EVENT_PS_SLEEP     0x0b

/** PS_STATE */
typedef enum _PS_STATE
{
    PS_STATE_AWAKE,
    PS_STATE_PRE_SLEEP,
    PS_STATE_SLEEP
} PS_STATE;

/** TLV type: AP Sleep param */
#define TLV_TYPE_AP_SLEEP_PARAM         (PROPRIETARY_TLV_BASE_ID + 106)
/** TLV type: AP Inactivity Sleep param */
#define TLV_TYPE_AP_INACT_SLEEP_PARAM   (PROPRIETARY_TLV_BASE_ID + 107)

/** MrvlIEtypes_sleep_param_t */
typedef struct _MrvlIEtypes_sleep_param_t
{
    /** Header */
    MrvlIEtypesHeader_t header;
    /** control bitmap */
    u32 ctrl_bitmap;
    /** min_sleep */
    u32 min_sleep;
    /** max_sleep */
    u32 max_sleep;
} __ATTRIB_PACK__ MrvlIEtypes_sleep_param_t;

/** MrvlIEtypes_inact_sleep_param_t */
typedef struct _MrvlIEtypes_inact_sleep_param_t
{
    /** Header */
    MrvlIEtypesHeader_t header;
    /** inactivity timeout */
    u32 inactivity_to;
    /** min_awake */
    u32 min_awake;
    /** max_awake */
    u32 max_awake;
} __ATTRIB_PACK__ MrvlIEtypes_inact_sleep_param_t;

/** AP_Event */
typedef struct _AP_Event
{
    /** Event ID */
    u32 EventId;
    /*
     * Reserved for STA_ASSOCIATED event and contains
     * status information for the MIC_COUNTERMEASURES event.
     */
    /** Reserved/status */
    u16 status;
    /** AP MAC address */
    u8 MacAddr[ETH_ALEN];
} __ATTRIB_PACK__ AP_Event;
#endif /* _UAP_FW_H */
