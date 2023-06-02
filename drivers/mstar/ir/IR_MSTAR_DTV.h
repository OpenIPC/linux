////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (��MStar Confidential Information��) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef IR_FORMAT_H
#define IR_FORMAT_H

#define XTAL_CLOCK_FREQ         24000000

//-------------------------------------------------------------------------------------------
// Customer IR Specification parameter define (Please modify them by IR SPEC)
//-------------------------------------------------------------------------------------------
#ifdef CONFIG_MSTAR_PM_SWIR
#define IR_MODE_SEL             IR_MODE_SWDECODE_MODE//IR_TYPE_SWDECODE_MODE//IR_TYPE_RAWDATA_MODE
#else
#define IR_MODE_SEL             IR_MODE_FULLDECODE_MODE
#endif
// IR Header code define
#define IR_HEADER_CODE0         0x80    // Custom 0
#define IR_HEADER_CODE1         0x7F    // Custom 1

// IR Timing define
#define IR_HEADER_CODE_TIME     9000    // us
#define IR_OFF_CODE_TIME        4500    // us
#define IR_OFF_CODE_RP_TIME     2500    // us
#define IR_LOGI_01H_TIME        560     // us
#define IR_LOGI_0_TIME          1120    // us
#define IR_LOGI_1_TIME          2240    // us
#define IR_TIMEOUT_CYC          140000  // us

// IR Format define
#define IRKEY_DUMY              0xFF
#define IRDA_KEY_MAPPING_POWER  IRKEY_POWER
//
//typedef enum _IrCommandType
//{
//    IRKEY_TV_RADIO          = 0x0C,
//    IRKEY_CHANNEL_LIST      = 0x10,
//    IRKEY_CHANNEL_FAV_LIST  = 0x08,
//    IRKEY_CHANNEL_RETURN    = 0x5C,
//    IRKEY_CHANNEL_PLUS      = 0x1F,
//    IRKEY_CHANNEL_MINUS     = 0x19,
//
//    IRKEY_AUDIO             = 0x44,
//    IRKEY_VOLUME_PLUS       = 0x16,
//    IRKEY_VOLUME_MINUS      = 0x15,
//
//    IRKEY_UP                = 0x52,
//    IRKEY_POWER             = 0x46,
//    IRKEY_EXIT              = 0x1B,
//    IRKEY_MENU              = 0x07,
//    IRKEY_DOWN              = 0x13,
//    IRKEY_LEFT              = 0x06,
//    IRKEY_SELECT            = 0x0F,
//    IRKEY_RIGHT             = 0x1A,
//
//    IRKEY_NUM_0             = 0x50,
//    IRKEY_NUM_1             = 0x49,
//    IRKEY_NUM_2             = 0x55,
//    IRKEY_NUM_3             = 0x59,
//    IRKEY_NUM_4             = 0x4D,
//    IRKEY_NUM_5             = 0x51,
//    IRKEY_NUM_6             = 0x5D,
//    IRKEY_NUM_7             = 0x48,
//    IRKEY_NUM_8             = 0x54,
//    IRKEY_NUM_9             = 0x58,
//
//    IRKEY_MUTE              = 0x5A,
//    IRKEY_PAGE_UP           = 0x03,
//    IRKEY_PAGE_DOWN         = 0x05,
//    IRKEY_CLOCK             = 0x5F,
//
//    IRKEY_INFO              = 0x14,
//    IRKEY_RED               = 0x47,
//    IRKEY_GREEN             = 0x4B,
//    IRKEY_YELLOW            = 0x57,
//    IRKEY_BLUE              = 0x5B,
//    IRKEY_MTS               = 0x41,
//    IRKEY_NINE_LATTICE      = IRKEY_DUMY,
//    IRKEY_TTX               = 0x0A,
//    IRKEY_CC                = 0x09,
//    IRKEY_INPUT_SOURCE      = 0x04,
//    IRKEY_CRADRD            = IRKEY_DUMY-1,
////    IRKEY_PICTURE           = 0x40,
//    IRKEY_ZOOM              = 0x4C,
//    IRKEY_DASH              = 0x4E,
//    IRKEY_SLEEP             = 0x45,
//    IRKEY_EPG               = 0x4A,
//    IRKEY_PIP               = 0x40,
//
//  	IRKEY_MIX               = 0x1C,
//    IRKEY_INDEX             = 0x18,
//    IRKEY_HOLD              = 0x00,
//    IRKEY_PREVIOUS          = 0x0E,
//    IRKEY_NEXT              = 0x12,
//    IRKEY_BACKWARD          = 0x02,
//    IRKEY_FORWARD           = 0x1E,
//    IRKEY_PLAY              = 0x01,
//    IRKEY_RECORD            = 0x0D,
//    IRKEY_STOP              = 0x11,
//    IRKEY_PAUSE             = 0x1D,
//
//    IRKEY_SIZE              = 0x43,
//    IRKEY_REVEAL            = 0x4F,
//    IRKEY_SUBCODE           = 0x53,
//}IrCommandType;
//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
// IR system parameter define for H/W setting (Please don't modify them)
//-------------------------------------------------------------------------------------------
#define IR_CKDIV_NUM                        ((XTAL_CLOCK_FREQ+500000)/1000000)
#define IR_CKDIV_NUM_BOOT                   XTAL_CLOCK_FREQ
#define IR_CLK_BOOT                         (XTAL_CLOCK_FREQ/1000000)
#define IR_CLK                              IR_CLK_BOOT

#define irGetMinCnt_BOOT(time, tolerance)   ((u32)(((double)time*((double)IR_CLK_BOOT)/(IR_CKDIV_NUM_BOOT+1))*((double)1-tolerance)))
#define irGetMaxCnt_BOOT(time, tolerance)   ((u32)(((double)time*((double)IR_CLK_BOOT)/(IR_CKDIV_NUM_BOOT+1))*((double)1+tolerance)))
#define irGetMinCnt(time, tolerance)        ((u32)(((double)time*((double)IR_CLK)/(IR_CKDIV_NUM+1))*((double)1-tolerance)))
#define irGetMaxCnt(time, tolerance)        ((u32)(((double)time*((double)IR_CLK)/(IR_CKDIV_NUM+1))*((double)1+tolerance)))

#define irGetCnt_BOOT(time)                 ((u32)((double)time*((double)IR_CLK_BOOT)/(IR_CKDIV_NUM_BOOT+1)))
#define irGetCnt(time)                      ((u32)((double)time*((double)IR_CLK)/(IR_CKDIV_NUM+1)))

// 12Mhz
#define IR_RP_TIMEOUT_BOOT      irGetCnt_BOOT(IR_TIMEOUT_CYC)
#define IR_HDC_UPB_BOOT         irGetMaxCnt_BOOT(IR_HEADER_CODE_TIME, 0.2)
#define IR_HDC_LOB_BOOT         irGetMinCnt_BOOT(IR_HEADER_CODE_TIME, 0.2)
#define IR_OFC_UPB_BOOT         irGetMaxCnt_BOOT(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_LOB_BOOT         irGetMinCnt_BOOT(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_RP_UPB_BOOT      irGetMaxCnt_BOOT(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_OFC_RP_LOB_BOOT      irGetMinCnt_BOOT(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_LG01H_UPB_BOOT       irGetMaxCnt_BOOT(IR_LOGI_01H_TIME, 0.35)
#define IR_LG01H_LOB_BOOT       irGetMinCnt_BOOT(IR_LOGI_01H_TIME, 0.3)
#define IR_LG0_UPB_BOOT         irGetMaxCnt_BOOT(IR_LOGI_0_TIME, 0.2)
#define IR_LG0_LOB_BOOT         irGetMinCnt_BOOT(IR_LOGI_0_TIME, 0.2)
#define IR_LG1_UPB_BOOT         irGetMaxCnt_BOOT(IR_LOGI_1_TIME, 0.2)
#define IR_LG1_LOB_BOOT         irGetMinCnt_BOOT(IR_LOGI_1_TIME, 0.2)

// 90Mhz
#define IR_RP_TIMEOUT           irGetCnt(IR_TIMEOUT_CYC)
#define IR_HDC_UPB              irGetMaxCnt(IR_HEADER_CODE_TIME, 0.2)
#define IR_HDC_LOB              irGetMinCnt(IR_HEADER_CODE_TIME, 0.2)
#define IR_OFC_UPB              irGetMaxCnt(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_LOB              irGetMinCnt(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_RP_UPB           irGetMaxCnt(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_OFC_RP_LOB           irGetMinCnt(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_LG01H_UPB            irGetMaxCnt(IR_LOGI_01H_TIME, 0.35)
#define IR_LG01H_LOB            irGetMinCnt(IR_LOGI_01H_TIME, 0.3)
#define IR_LG0_UPB              irGetMaxCnt(IR_LOGI_0_TIME, 0.2)
#define IR_LG0_LOB              irGetMinCnt(IR_LOGI_0_TIME, 0.2)
#define IR_LG1_UPB              irGetMaxCnt(IR_LOGI_1_TIME, 0.2)
#define IR_LG1_LOB              irGetMinCnt(IR_LOGI_1_TIME, 0.2)

//-------------------------------------------------------------------------------------------

#include <media/rc-core.h>

/*
 * Jimmy Hsu <jimmy.hsu@mstarsemi.com>
 * this is the remote control that comes with the mstar smart tv
 * which based on STAOS standard.
 */
static struct rc_map_table mstar_tv[] = {
    { 0x0046, KEY_POWER },
    { 0x0050, KEY_0 },
    { 0x0049, KEY_1 },
    { 0x0055, KEY_2 },
    { 0x0059, KEY_3 },
    { 0x004D, KEY_4 },
    { 0x0051, KEY_5 },
    { 0x005D, KEY_6 },
    { 0x0048, KEY_7 },
    { 0x0054, KEY_8 },
    { 0x0058, KEY_9 },
    { 0x0047, KEY_RED },
    { 0x004B, KEY_GREEN },
    { 0x0057, KEY_YELLOW },
    { 0x005B, KEY_BLUE },
    { 0x0052, KEY_UP },
    { 0x0013, KEY_DOWN },
    { 0x0006, KEY_LEFT },
    { 0x001A, KEY_RIGHT },
    { 0x000F, KEY_ENTER },
    { 0x001F, KEY_CHANNELUP },
    { 0x0019, KEY_CHANNELDOWN },
    { 0x0016, KEY_VOLUMEUP },
    { 0x0015, KEY_VOLUMEDOWN },
    { 0x0003, KEY_PAGEUP },
    { 0x0005, KEY_PAGEDOWN },
    { 0x0017, KEY_HOME},
    { 0x0007, KEY_MENU },
    { 0x001B, KEY_BACK },
    { 0x005A, KEY_MUTE },
    { 0x000D, KEY_RECORD },     // DVR
    { 0x0042, KEY_HELP },       // GUIDE
    { 0x0014, KEY_INFO },
    { 0x0040, KEY_KP0 },        // WINDOW
    { 0x0004, KEY_KP1 },        // TV_INPUT
    { 0x000E, KEY_REWIND },
    { 0x0012, KEY_FORWARD },
    { 0x0002, KEY_PREVIOUSSONG },
    { 0x001E, KEY_NEXTSONG },
    { 0x0001, KEY_PLAY },
    { 0x001D, KEY_PAUSE },
    { 0x0011, KEY_STOP },
    { 0x0044, KEY_AUDIO },      // (C)SOUND_MODE
    { 0x0056, KEY_CAMERA },     // (C)PICTURE_MODE
    { 0x004C, KEY_ZOOM },       // (C)ASPECT_RATIO
    { 0x005C, KEY_CHANNEL },    // (C)CHANNEL_RETURN
    { 0x0045, KEY_SLEEP },      // (C)SLEEP
    { 0x004A, KEY_EPG },        // (C)EPG
    { 0x0010, KEY_LIST },       // (C)LIST
    { 0x0053, KEY_SUBTITLE },   // (C)SUBTITLE
    { 0x0041, KEY_FN_F1 },      // (C)MTS
    { 0x004E, KEY_FN_F2 },      // (C)FREEZE
    { 0x000A, KEY_FN_F3 },      // (C)TTX
    { 0x0009, KEY_FN_F4 },      // (C)CC
    { 0x001C, KEY_FN_F5 },      // (C)TV_SETTING
    { 0x0008, KEY_FN_F6 },      // (C)SCREENSHOT
    { 0x000B, KEY_F1 },         // MSTAR_BALANCE
    { 0x0018, KEY_F2 },         // MSTAR_INDEX
    { 0x0000, KEY_F3 },         // MSTAR_HOLD
    { 0x000C, KEY_F4 },         // MSTAR_UPDATE
    { 0x004F, KEY_F5 },         // MSTAR_REVEAL
    { 0x005E, KEY_F6 },         // MSTAR_SUBCODE
    { 0x0043, KEY_F7 },         // MSTAR_SIZE
    { 0x005F, KEY_F8 },         // MSTAR_CLOCK
    { 0x00FE, KEY_POWER2 },     // FAKE_POWER
    { 0x00FF, KEY_OK },         // KEY_OK

    // 2nd IR controller.
};

#define MS_IR_MAP_NAME        	"rc-mstar-dtv"
#define MS_IR_VENDOR_ID       	0x3697

static struct rc_map_list ms_rc_map = {
	.map = {
		.scan    = mstar_tv,
		.size    = ARRAY_SIZE(mstar_tv),
		.rc_type = RC_TYPE_UNKNOWN,	/* Legacy IR type */
		.name    = MS_IR_MAP_NAME,
	}
};

#endif

