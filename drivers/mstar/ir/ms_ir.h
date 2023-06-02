/*
 * ms_ir.h
 *
 *  Created on: 2015年7月1日
 *      Author: Administrator
 */

#ifndef _MS_IR_H_
#define _MS_IR_H_
#include "ms_types.h"

#define IR_TYPE_OLD                 0
#define IR_TYPE_NEW                 1
#define IR_TYPE_MSTAR_DTV           2
#define IR_TYPE_MSTAR_RAW		    3
#define IR_TYPE_RC_V16              4
#define IR_TYPE_CUS03_DTV           5
#define IR_TYPE_MSTAR_FANTASY       6
#define IR_TYPE_MSTAR_SZ1           7
#define IR_TYPE_CUS08_RC5           8
#define IR_TYPE_CUS21SH             9
#define IR_TYPE_RCMM               10


#define IR_MODE_FULLDECODE_MODE 1
#define IR_MODE_RAWDATA_MODE    2
#define IR_MODE_SWDECODE_MODE   3
#define IR_MODE_SWDECODE_KON_MODE 4


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
#define IR_MAX_BUF_DPH  2
#define IR_MAX_BUF_LEN  256

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
//#if 0
//typedef enum
//{
//    KEYCODE_TV_RADIO,
//    KEYCODE_CHANNEL_LIST,
//    KEYCODE_CHANNEL_FAV_LIST,
//    KEYCODE_CHANNEL_RETURN,
//    KEYCODE_CHANNEL_PLUS,
//    KEYCODE_CHANNEL_MINUS,
//
//    KEYCODE_AUDIO,
//    KEYCODE_VOLUME_PLUS,
//    KEYCODE_VOLUME_MINUS,
//
//    KEYCODE_UP,
//    KEYCODE_POWER,
//    KEYCODE_EXIT,
//    KEYCODE_MENU,
//    KEYCODE_DOWN,
//    KEYCODE_LEFT,
//    KEYCODE_SELECT,
//    KEYCODE_RIGHT,
//
//    KEYCODE_NUMERIC_0,
//    KEYCODE_NUMERIC_1,
//    KEYCODE_NUMERIC_2,
//    KEYCODE_NUMERIC_3,
//    KEYCODE_NUMERIC_4,
//    KEYCODE_NUMERIC_5,
//    KEYCODE_NUMERIC_6,
//    KEYCODE_NUMERIC_7,
//    KEYCODE_NUMERIC_8,
//    KEYCODE_NUMERIC_9,
//
//    KEYCODE_MUTE,
//    KEYCODE_PAGE_UP,
//    KEYCODE_PAGE_DOWN,
//    KEYCODE_CLOCK,
//
//    KEYCODE_INFO,
//    KEYCODE_RED,
//    KEYCODE_GREEN,
//    KEYCODE_YELLOW,
//    KEYCODE_BLUE,
//    KEYCODE_MTS,
//    KEYCODE_NINE_LATTICE,
//    KEYCODE_TTX,
//    KEYCODE_CC,
//    KEYCODE_INPUT_SOURCE,
//    KEYCODE_CRADRD,
////    KEYCODE_PICTURE,
//    KEYCODE_ZOOM,
//    KEYCODE_DASH,
//    KEYCODE_SLEEP,
//    KEYCODE_EPG,
//    KEYCODE_PIP,
//
//    KEYCODE_MIX,
//    KEYCODE_INDEX,
//    KEYCODE_HOLD,
//    KEYCODE_PREVIOUS,
//    KEYCODE_NEXT,
//    KEYCODE_BACKWARD,
//    KEYCODE_FORWARD,
//    KEYCODE_PLAY,
//    KEYCODE_RECORD,
//    KEYCODE_STOP,
//    KEYCODE_PAUSE,
//
//    KEYCODE_SIZE,
//    KEYCODE_REVEAL,
//    KEYCODE_SUBCODE,
//
//    KEYCODE_DUMMY,
//    KEYCODE_NUM,
//} EN_KEY;
//#else
//// This is copyed from Chakra. To make sure the key code is same in Linux & Chakra for OBAMA.
//typedef enum
//{
//    KEYCODE_CHANNEL_FAV_LIST    = 0x00,             //IR_KEY_FAV
//    KEYCODE_CHANNEL_RETURN      = 0x01,             //IR_KEY_FLASHBK
//    KEYCODE_CHANNEL_PLUS        = 0x02,             //IR_KEY_CH_UP
//    KEYCODE_CHANNEL_MINUS       = 0x03,             //IR_KEY_CH_DOWN
//
//    KEYCODE_AUDIO               = 0x04,             //IR_KEY_SOUND
//    KEYCODE_VOLUME_PLUS         = 0x05,             //IR_KEY_VOL_UP
//    KEYCODE_VOLUME_MINUS        = 0x06,             //IR_KEY_VOL_DOWN
//
//    KEYCODE_UP                  = 0x07,             //IR_KEY_UP_ARROW
//    KEYCODE_POWER               = 0x08,             //IR_KEY_POWER
//    KEYCODE_EXIT                = 0x09,             //IR_KEY_EXIT
//    KEYCODE_MENU                = 0x0A,             //IR_KEY_MENU
//    KEYCODE_DOWN                = 0x0B,             //IR_KEY_DOWN_ARROW
//    KEYCODE_LEFT                = 0x0C,             //IR_KEY_LEFT_ARROW
//    KEYCODE_SELECT              = 0x0D,             //IR_KEY_ENTER
//    KEYCODE_BACK                = 0x0E,             //IR_KEY_BACK
//    KEYCODE_RIGHT               = 0x0F,             //IR_KEY_RIGHT_ARROW
//
//    KEYCODE_NUMERIC_0           = 0x10,             //IR_KEY_0
//    KEYCODE_NUMERIC_1           = 0x11,             //IR_KEY_1
//    KEYCODE_NUMERIC_2           = 0x12,             //IR_KEY_2
//    KEYCODE_NUMERIC_3           = 0x13,             //IR_KEY_3
//    KEYCODE_NUMERIC_4           = 0x14,             //IR_KEY_4
//    KEYCODE_NUMERIC_5           = 0x15,             //IR_KEY_5
//    KEYCODE_NUMERIC_6           = 0x16,             //IR_KEY_6
//    KEYCODE_NUMERIC_7           = 0x17,             //IR_KEY_7
//    KEYCODE_NUMERIC_8           = 0x18,             //IR_KEY_8
//    KEYCODE_NUMERIC_9           = 0x19,             //IR_KEY_9
//
//    KEYCODE_MUTE                = 0x1A,             //IR_KEY_MUTE
//
//    KEYCODE_INFO                = 0x1B,             //IR_KEY_INFO
//    KEYCODE_RED                 = 0x1C,             //IR_KEY_RED
//    KEYCODE_GREEN               = 0x1D,             //IR_KEY_GREEN
//    KEYCODE_MTS                 = 0x1E,             //IR_KEY_SAP
//    KEYCODE_SUBTITLE            = 0x1F,
//
//    KEYCODE_INPUT_SOURCE        = 0x20,             //IR_KEY_INPUT
//    KEYCODE_PICTURE             = 0x21,             //IR_KEY_PICTURE
//
//    KEYCODE_SOUND               = 0x22,
//
//    KEYCODE_ZOOM                = 0x23,             //IR_KEY_RATIO
//    KEYCODE_DASH                = 0x24,             //IR_KEY_DASH
//    KEYCODE_SLEEP               = 0x25,             //IR_KEY_TIMER
//    KEYCODE_EPG                 = 0x26,
//
//    KEYCODE_HOLD                = 0x27,             //IR_KEY_HOLD
//
//    KEYCODE_ADJUST              = 0x28,             //IR_KEY_ADJST
//    KEYCODE_TV_INPUT            = 0x29,
//
//    KEYCODE_P_CHECK             = 0x2C,
//    KEYCODE_S_CHECK             = 0x2D,
//    KEYCODE_POWERONLY           = 0x2E,
//    KEYCODE_BRIGHTNESS_PLUS     = 0x31,
//    KEYCODE_BRIGHTNESS_MINUS    = 0x32,
//
//    // TTX keys
//    KEYCODE_YELLOW              = 0x33,
//    KEYCODE_BLUE                = 0x34,
//    KEYCODE_TTX                 = 0x35,
//    KEYCODE_SIZE                = 0x36,
//    KEYCODE_MIX                 = 0x37,
//    KEYCODE_INDEX               = 0x38,
//    KEYCODE_TV_RADIO            = 0x39,
//    KEYCODE_REVEAL              = 0x3A,
//    KEYCODE_CHANNEL_LIST        = 0x3B,
//    KEYCODE_TIME                = 0x3C,     // prevent to display TIME window when press TIME key ( no teletext channel )
//    KEYCODE_CLOCK               = 0x3D,
//    KEYCODE_TTX_MODE            = 0x3E,
//    KEYCODE_UPDATE              = 0x3F,     // add UPDATE key
//
//    DSC_KEY_PWRON           = 0x40,
//    DSC_KEY_PWROFF          = 0x41,
//    DSC_KEY_ARC4X3          = 0x42,
//    DSC_KEY_ARC16X9         = 0x43,
//    DSC_KEY_ARCZOOM         = 0x44,
//    DSC_KEY_TV              = 0x45,
//    DSC_KEY_VIDEO1          = 0x46,
//    DSC_KEY_VIDEO2          = 0x47,
//    DSC_KEY_VIDEO3          = 0x48,
//    DSC_KEY_COMP1           = 0x49,
//    DSC_KEY_COMP2           = 0x4A,
//    DSC_KEY_RGBPC           = 0x4B,
//    DSC_KEY_RGBDTV          = 0x4C,
//    DSC_KEY_RGBDVI          = 0x4D,
//    DSC_KEY_HDMI1           = 0x4E,
//    DSC_KEY_HDMI2           = 0x4F,
//    DSC_KEY_MULTI_PIP       = 0x50,
//    DSC_KEY_MULTIMEDIA      = 0x51,
//    DSC_KEY_DVIPC           = 0x52,
//
//    KEYCODE_MMI                 = 0x5C,
//    KEYCODE_MMI_ENTER_MENU      = 0x5D,
//
//    KEYCODE_UARTDEBUGPORT       = 0x5E,
//
//    KEYCODE_PAUSE               = 0x60,
//    KEYCODE_PLAY                = 0x61,
//    KEYCODE_RECORD              = 0x62,
//    KEYCODE_STOP                = 0x63,
//    KEYCODE_SUBCODE             = 0x69,
//    KEYCODE_DA                  = 0x6A,
//    KEYCODE_SUBPAGE             = 0x6B,
//    KEYCODE_DVD                 = 0x6C,
//
//    KEYCODE_DTV                 = 0x82,
//    KEYCODE_TV                  = 0x83,
//    KEYCODE_SCART               = 0x84,
//    KEYCODE_COMPONENT           = 0x85,
//    KEYCODE_PC                  = 0x86,
//    KEYCODE_HDMI                = 0x87,
//    KEYCODE_AV                  = 0x88,
//    KEYCODE_SV                  = 0x89,
//    KEYCODE_FREEZE              = 0x8A,             //IR_KEY_FREEZE
//    KEYCODE_FORWARD             = 0x8B,
//    KEYCODE_BACKWARD            = 0x8C,
//    KEYCODE_NEXT                = 0x8D,
//    KEYCODE_PREVIOUS            = 0x8E,
//    KEYCODE_PAGE_UP             = 0x8F,
//    KEYCODE_PAGE_DOWN           = 0x90,
//    KEYCODE_CC                  = 0x91,
//    KEYCODE_VOLUME              = 0x92,
//    KEYCODE_PIP                 = 0x93,
//    KEYCODE_CRADRD              = 0x94,
//    KEYCODE_NINE_LATTICE        = 0x95,
//
//    KEYCODE_DUMMY               = 0xFF,
//    KEYCODE_NULL                = 0xFF
//} EN_KEY;
//#endif

typedef enum
{
    EN_SHOT_P = 0x01,   /// 2'b01: only pshot edge detect for counter
    EN_SHOT_N = 0x02,   /// 2'b10: only nshot edge detect for counter
    EN_SHOT_PN = 0x03,  /// 2'b11/2'b00: both pshot/nshot edge detect for counter
    EN_SHOT_INVLD,      /// Invalid for value greater than 2'b11

} EN_SHOT_SEL;

typedef enum
{
    E_IR_PROTOCOL_NONE=0,
    E_IR_PROTOCOL_NEC,
    E_IR_PROTOCOL_RC5,
    E_IR_PROTOCOL_PZ_OCN,
    E_IR_PROTOCOL_MAX,
} IR_PROCOCOL_TYPE;

typedef struct
{
    U32 u32_1stDelayTimeMs;
    U32 u32_2ndDelayTimeMs;
    U8 data3;
} MS_IR_DelayTime, *PMS_IR_DelayTime;

typedef struct
{
    U8 u8Key;
    U8 u8System;
    U8 u8Flag;
    U8 u8Valid;
} MS_IR_KeyInfo, *PMS_IR_KeyInfo;

typedef struct
{
    unsigned long time;
} MS_IR_LastKeyTime, *PMS_IR_LastKeyTime;

typedef struct
{
    U8 u8KeyIn;
    U8 u8KeyOut;
} MS_IR_KeyValue, *PMS_IR_KeyValue;

/// define IR key code time & bounds
typedef struct
{
    S16 s16Time;   ///key code time
    S8 s8UpBnd;    ///upper bound
    S8 s8LoBnd;    ///low bound
} MS_IR_TimeBnd, *PMS_IR_TimeBnd;

/// define IR key code time tail
typedef struct
{
    U32 gu32KeyMin;     /// Min Tail Time for key
    U32 gu32KeyMax;     /// Max Tail Time for key
    U32 gu32RptMin;     /// Min Tail Time for Rpt
    U32 gu32RptMax;     /// Max Tail Time for Rpt
} MS_IR_TimeTail, *PMS_IR_TimeTail;

/// define IR time parameters
typedef struct
{
    MS_IR_TimeBnd tHdr;     /// header code time
    MS_IR_TimeBnd tOff;     /// off code time
    MS_IR_TimeBnd tOffRpt;  /// off code repeat time
    MS_IR_TimeBnd tLg01Hg;  /// logical 0/1 high time
    MS_IR_TimeBnd tLg0;     /// logical 0 time
    MS_IR_TimeBnd tLg1;     /// logical 1 time
    MS_IR_TimeBnd tSepr;    /// Separate time
    U32 u32TimeoutCyc;      /// Timeout cycle count
    U16 u16RCBitTime;       /// RC Bit Time
    MS_IR_TimeTail tTail;   /// Tail Time for sw shot mode
} MS_IR_TimeCfg, *PMS_IR_TimeCfg;

/// define IR configuration parameters
typedef struct
{
    U8 u8DecMode;           /// IR mode selection
    U8 u8ExtFormat;         /// IR extention format
    U8 u8Ctrl0;             /// IR enable control 0
    U8 u8Ctrl1;             /// IR enable control 1
    U8 u8Clk_mhz;           /// IR required clock
    U8 u8HdrCode0;          /// IR Header code 0
    U8 u8HdrCode1;          /// IR Header code 1
    U8 u8CCodeBytes;        /// Customer codes: 1 or 2 bytes
    U8 u8CodeBits;          /// Code bits: 1~128 bits
    U8 u8KeySelect;         /// IR select Nth key N(1~16)
    U16 u16GlhrmNum;        /// Glitch Remove Number
    EN_SHOT_SEL enShotSel;  /// Shot selection for SW decoder
    BOOL bInvertPolar;      /// Invert the polarity for input IR signal

} MS_IR_InitCfg, *PMS_IR_InitCfg;

/// define Ping-Pong Buffer structure for IR SW shot count
typedef struct
{
    U8 u8RdIdx;                        //Read Index
    U8 u8WtIdx;                        //Write Index
    U32 u32Length;                     //Data Length for Read Index buffer
    U32 u32Buffer[IR_MAX_BUF_DPH][IR_MAX_BUF_LEN];  //Ping-Pong Buffer
} MS_IR_ShotInfo, *PMS_IR_ShotInfo;

/// define HeaderInfo for sw mode change headercode in apps
typedef struct
{
    U8 _u8IRHeaderCode0;                        //IRHeaderCode0
    U8 _u8IRHeaderCode1;                        //IRHeaderCode1
    U8 _u8IR2HeaderCode0;                        //IR2HeaderCode0
    U8 _u8IR2HeaderCode1;                        //IR2HeaderCode1
}MS_MultiIR_HeaderInfo, *PMS_MultiIR_HeaderInfo;

/// define used protocol for multiple protocol mode
typedef struct
{
    IR_PROCOCOL_TYPE eProtocol;
    void *pNextProtCfg;
} MS_MultiProtocolCfg, *PMS_MultiProtocolCfg;


typedef struct
{
    unsigned int CustomerCode;                        //Set multi Customer Code
    unsigned int IR_Protocol;                        //IR_Protocol: 1:NEC, 2:RAW, 3:SW
    unsigned char Enable;                        //Enable
    unsigned int  bResult;                        // return status
}MS_IR_MutliCustomerCode, *PMS_IR_MutliCustomerCode;



#define REG_IR_CTRL             		(0x0040*4)
    #define IR_SEPR_EN              0x0200
    #define IR_TIMEOUT_CHK_EN       0x0100
    #define IR_INV                  0x80
    #define IR_INT_MASK             0x40
    #define IR_RPCODE_EN            0x20
    #define IR_LG01H_CHK_EN         0x10
    #define IR_DCODE_PCHK_EN        0x08
    #define IR_CCODE_CHK_EN         0x04
    #define IR_LDCCHK_EN            0x02
    #define IR_EN                   0x01
#define REG_IR_HDC_UPB          		(0x0041*4)
#define REG_IR_HDC_LOB          		(0x0042*4)
#define REG_IR_OFC_UPB          		(0x0043*4)
#define REG_IR_OFC_LOB          		(0x0044*4)
#define REG_IR_OFC_RP_UPB       		(0x0045*4)
#define REG_IR_OFC_RP_LOB       		(0x0046*4)
#define REG_IR_LG01H_UPB        		(0x0047*4)
#define REG_IR_LG01H_LOB        		(0x0048*4)
#define REG_IR_LG0_UPB          		(0x0049*4)
#define REG_IR_LG0_LOB          		(0x004A*4)
#define REG_IR_LG1_UPB          		(0x004B*4)
#define REG_IR_LG1_LOB          		(0x004C*4)
#define REG_IR_SEPR_UPB         		(0x004D*4)
#define REG_IR_SEPR_LOB         		(0x004E*4)
#define REG_IR_TIMEOUT_CYC_L    		(0x004F*4)
#define REG_IR_TIMEOUT_CYC_H_CODE_BYTE  (0x0050*4)
    #define IR_CCB_CB               0x1F00//ir_ccode_byte:1+ir_code_bit_num:32
#define REG_IR_SEPR_BIT_FIFO_CTRL       (0x0051*4)
#define REG_IR_CCODE            		(0x0052*4)
#define REG_IR_GLHRM_NUM        		(0x0053*4)
#define REG_IR_CKDIV_NUM_KEY_DATA       (0x0054*4)
#define REG_IR_SHOT_CNT_L       		(0x0055*4)
#define REG_IR_SHOT_CNT_H_FIFO_STATUS   (0x0056*4)
    #define IR_RPT_FLAG             0x0100
    #define IR_FIFO_EMPTY           0x0200
#define REG_IR_FIFO_RD_PULSE    		(0x0058*4)
#define REG_IR_CCODE1            		(0x0060*4)
#define REG_IR_CCODE1_CHK_EN			(0x0061*4)



#endif /* DRIVERS_MSTAR_IR_MS_IR_H_ */
