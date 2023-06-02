#ifndef __PADMUX_TABLES_H__
#define __PADMUX_TABLES_H__

#include "mhal_gpio.h"

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================
typedef struct stPadMux
{
    U16 padID;
    U32 base;
    U16 offset;
    U16 mask;
    U16 val;
    U16 mode;
} ST_PadMuxInfo;

typedef struct stPadMode
{
    U8  u8PadName[16];
    U32 u32ModeRIU;
    U32 u32ModeMask;
} ST_PadModeInfo;

extern const ST_PadModeInfo m_stPadModeInfoTbl[];
extern const ST_PadMuxInfo m_stPadMuxTbl[];
extern U32 g_u32Padmux_cnt;

#endif // __PADMUX_TABLES_H_
