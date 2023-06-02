////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
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

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   MsTypes.h
/// @brief  MStar General Data Types
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MS_TYPES_H_
#define _MS_TYPES_H_


//-------------------------------------------------------------------------------------------------
//  System Data Type
//-------------------------------------------------------------------------------------------------
/// data type unsigned char, data length 1 byte
#define MS_U8       unsigned char                                            // 1 byte
/// data type unsigned short, data length 2 byte
#define MS_U16      unsigned short                                           // 2 bytes
/// data type unsigned int, data length 4 byte
#define MS_U32      unsigned long                                            // 4 bytes
/// data type unsigned int, data length 8 byte
#define MS_U64      unsigned long long                                       // 8 bytes
/// data type signed char, data length 1 byte
#define MS_S8       signed char                                              // 1 byte
/// data type signed short, data length 2 byte
#define MS_S16      signed short                                             // 2 bytes
/// data type signed int, data length 4 byte
#define MS_S32      signed long                                              // 4 bytes
/// data type signed int, data length 8 byte
#define MS_S64      signed long long                                         // 8 bytes
/// data type float, data length 4 byte
#define MS_FLOAT    float                                                    // 4 bytes
/// data type null pointer
#ifdef NULL
#undef NULL
#endif
#define NULL                        0

#define MS_BOOL     unsigned char

/// data type hardware physical address
#define MS_PHYADDR unsigned long                                    // 32bit physical address



#if !defined(BIT0) && !defined(BIT1)
#define BIT0                           0x00000001
#define BIT1                           0x00000002
#define BIT2                           0x00000004
#define BIT3                           0x00000008
#define BIT4                           0x00000010
#define BIT5                           0x00000020
#define BIT6                           0x00000040
#define BIT7                           0x00000080
#define BIT8                           0x00000100
#define BIT9                           0x00000200
#define BIT10                          0x00000400
#define BIT11                          0x00000800
#define BIT12                          0x00001000
#define BIT13                          0x00002000
#define BIT14                          0x00004000
#define BIT15                          0x00008000
#define BIT16                          0x00010000
#define BIT17                          0x00020000
#define BIT18                          0x00040000
#define BIT19                          0x00080000
#define BIT20                          0x00100000
#define BIT21                          0x00200000
#define BIT22                          0x00400000
#define BIT23                          0x00800000
#define BIT24                          0x01000000
#define BIT25                          0x02000000
#define BIT26                          0x04000000
#define BIT27                          0x08000000
#define BIT28                          0x10000000
#define BIT29                          0x20000000
#define BIT30                          0x40000000
#define BIT31                          0x80000000
#endif

//-------------------------------------------------------------------------------------------------
//  Software Data Type
//-------------------------------------------------------------------------------------------------

//[TODO] use MS_U8, ... instead
// data type for 8051 code
//typedef MS_U16                      WORD;
//typedef MS_U8                       BYTE;


#ifndef true
/// definition for true
#define true                        1
/// definition for false
#define false                       0
#endif


#if !defined(TRUE) && !defined(FALSE)
/// definition for TRUE
#define TRUE                        1
/// definition for FALSE
#define FALSE                       0
#endif


#if defined(ENABLE) && (ENABLE!=1)
#warning ENALBE is not 1
#else
#define ENABLE                      1
#endif

#if defined(DISABLE) && (DISABLE!=0)
#warning DISABLE is not 0
#else
#define DISABLE                     0
#endif


///Define MS FB Format, to share with GE,GOP
/// FIXME THE NAME NEED TO BE REFINED, AND MUST REMOVE UNNESSARY FMT
typedef enum
{
    /// color format I1
    E_MS_FMT_I1                     = 0x0,
    /// color format I2
    E_MS_FMT_I2                     = 0x1,
    /// color format I4
    E_MS_FMT_I4                     = 0x2,
    /// color format palette 256(I8)
    E_MS_FMT_I8                     = 0x4,
    /// color format blinking display
    E_MS_FMT_FaBaFgBg2266  = 0x6,
    /// color format for blinking display format
    E_MS_FMT_1ABFgBg12355           = 0x7,
    /// color format RGB565
    E_MS_FMT_RGB565                 = 0x8,
    /// color format ARGB1555
    /// @note <b>[URANUS] <em>ARGB1555 is only RGB555</em></b>
    E_MS_FMT_ARGB1555               = 0x9,
    /// color format ARGB4444
    E_MS_FMT_ARGB4444               = 0xa,
    /// color format ARGB1555 DST
    E_MS_FMT_ARGB1555_DST           = 0xc,
    /// color format YUV422
    E_MS_FMT_YUV422                 = 0xe,
    /// color format ARGB8888
    E_MS_FMT_ARGB8888               = 0xf,
    /// color format RGBA5551
    E_MS_FMT_RGBA5551               = 0x10,
    /// color format RGBA4444
    E_MS_FMT_RGBA4444               = 0x11,
    /// color format ABGR8888
    E_MS_FMT_ABGR8888               = 0x1f,

    E_MS_FMT_GENERIC                = 0xFFFF,

} MS_ColorFormat;


typedef union _MSIF_Version
{
    struct _DDI
    {
        MS_U8                       tag[4];
        MS_U8                       type[2];
        MS_U16                      customer;
        MS_U16                      model;
        MS_U16                      chip;
        MS_U8                       cpu;
        MS_U8                       name[4];
        MS_U8                       version[2];
        MS_U8                       build[2];
        MS_U8                       change[8];
        MS_U8                       os;
    } MS_DDI;
    struct _MW
    {
        MS_U8                                     tag[4];
        MS_U8                                     type[2];
        MS_U16                                    customer;
        MS_U16                                    mod;
        MS_U16                                    chip;
        MS_U8                                     cpu;
        MS_U8                                     name[4];
        MS_U8                                     version[2];
        MS_U8                                     build[2];
        MS_U8                                     changelist[8];
        MS_U8                                     os;
    } MW;
    struct _APP
    {
        MS_U8                                     tag[4];
        MS_U8                                     type[2];
        MS_U8                                     id[4];
        MS_U8                                     quality;
        MS_U8                                     version[4];
        MS_U8                                     time[6];
        MS_U8                                     changelist[8];
        MS_U8                                     reserve[3];
    } APP;
} MSIF_Version;

typedef struct _MS_SW_VERSION_INFO
{
	char	UtopiaBspVersion[8];	//Utopia BSP Version
    char    MajorVersion[4];        //Major Version Number
    char    MinorVersion[4];        //Minor Version Number
    char    ChangeList_API[16];     //Sync Perforce Change List Number in API Folder
    char    ChangeList_DRV[16];     //Sync Perforce Change List Number in DRV Folder
    char    ChangeList_HAL[16];     //Sync Perforce Change List Number in HAL Folder

} MS_SW_VERSION_INFO;

#endif // _MS_TYPES_H_
