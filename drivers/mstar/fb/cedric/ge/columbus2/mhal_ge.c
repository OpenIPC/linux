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

///////////////////////////////////////////////////////////////////////////////
//
//      File name: drvGE.c
//      Description:  GR driver implementation.
//                    1. PE part : piexl engine
//                    2. Blt part : fast blt engine
//
///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// Include Files
//----------------------------------------------------------------------------

#include <asm/io.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/semaphore.h>


#include "mach/ms_types.h"
#include "mhal_ge_reg.h"
#include "mhal_ge.h"
#include "mhal_ge_fp.h"

//----------------------------------------------------------------------------
// Compile options
//----------------------------------------------------------------------------
// #define GE_DEBUG

//----------------------------------------------------------------------------
// Local Defines
//----------------------------------------------------------------------------
#define WAIT_CMD_FIFO   1
#if WAIT_CMD_FIFO
#define PATCH_RD_CMD    1
#else
#define PATCH_RD_CMD    0
#endif

//----------------------------------------------------------------------------
// Debug Macros
//----------------------------------------------------------------------------
#define GE_DEBUGINFO(x)  //x
#define GE_WARN(x) printk(KERN_WARNING x);

 //#undef GE_DEBUG
// #define GE_DEBUG
 
#ifdef GE_DEBUG
#define GE_ASSERT(_bool, _f)                 if (!(_bool)) { (_f); while (1) printk("G0"); }
#else
#define GE_ASSERT(_bool, _f)                 while (0)
#endif // #ifdef SCL_DEBUG

#ifdef GE_DEBUG

#define _GE_CHECK_BUFFER_ALIGN0(addr, color_fmt)                                                \
    switch ((color_fmt)){                                                                       \
    case GE_FMT_I1:                                                                             \
    case GE_FMT_I2:                                                                             \
    case GE_FMT_I4:                                                                             \
    case GE_FMT_I8:                                                                             \
        break;                                                                                  \
    case GE_FMT_YUV422:                                                                         \
    case GE_FMT_1ABFGBG12355:                                                                   \
    case GE_FMT_RGB565 :                                                                        \
    case GE_FMT_ARGB1555 :                                                                      \
    case GE_FMT_ARGB4444 :                                                                      \
        GE_ASSERT(!(0x1 & (addr)), printk("[GE DRV][%06d] Bad buffer address (0x%08x, %d)\n", (addr), (color_fmt)));    \
        break;                                                                                  \
    case GE_FMT_ARGB8888 :                                                                      \
        GE_ASSERT(!(0x3 & (addr)), printk("[GE DRV][%06d] Bad buffer address (0x%08x, %d)\n", (addr), (color_fmt)));    \
        break;                                                                                  \
    default:                                                                                    \
        GE_ASSERT(0, printk("[GE DRV][%06d] Invalid color format\n"));                          \
        break;                                                                                  \
    }


#define _GE_CHECK_BUFFER_ALIGN1(addr, width, height, pitch, color_fmt)                          \
    switch ((color_fmt)){                                                                       \
    case GE_FMT_I1:                                                                             \
    case GE_FMT_I2:                                                                             \
    case GE_FMT_I4:                                                                             \
    case GE_FMT_I8:                                                                             \
        break;                                                                                  \
    case GE_FMT_YUV422:                                                                         \
    case GE_FMT_1ABFGBG12355:                                                                   \
    case GE_FMT_RGB565 :                                                                        \
    case GE_FMT_ARGB1555 :                                                                      \
    case GE_FMT_ARGB4444 :                                                                      \
        GE_ASSERT(!(0x1 & (addr)), printk("[GE DRV][%06d] Bad buffer address (0x%08x, %d)\n", (addr), (color_fmt)));            \
        GE_ASSERT(!(0x1 & (pitch)), printk("[GE DRV][%06d] Bad buffer pitch (%d, %d)\n", (pitch), (color_fmt)));                \
        GE_ASSERT(((pitch)>>1)== (width), printk("[GE DRV][%06d] Bad buffer pitch/width (%d, %d)\n", (pitch), (width)));        \
        break;                                                                                  \
    case GE_FMT_ARGB8888 :                                                                      \
        GE_ASSERT(!(0x3 & (addr)), printk("[GE DRV][%06d] Bad buffer address (0x%08x, %d)\n", (addr), (color_fmt)));            \
        GE_ASSERT(!(0x3 & (pitch)), printk("[GE DRV][%06d] Bad buffer pitch (%d, %d)\n", (pitch), (color_fmt)));                \
        GE_ASSERT(((pitch)>>2)== (width), printk("8888[GE DRV][%06d] Bad buffer pitch/width (%d, %d)\n", (pitch), (width)));        \
        break;                                                                                  \
    default:                                                                                    \
        GE_ASSERT(0, printk("[GE DRV][%06d] Invalid color format\n"));                          \
        break;                                                                                  \
    }

#define _GE_SIMPLE_BB_CHECK()                                                                   \
    GE_WaitAvailableCMDQueue(32);                                                               \
    if (!(GE_VAL_EN_STRETCH_BITBLT & GE_ReadReg(GE_REG_FMT_BLT)))                               \
    {                                                                                           \
        if (GE_ReadReg(GE_REG_STBB_INIT_DX) || GE_ReadReg(GE_REG_STBB_INIT_DY))                 \
        {                                                                                       \
            while (1)printk("G1");                                                                          \
        }                                                                                       \
        if (GE_ReadReg(GE_REG_STBB_WIDTH)!= (GE_ReadReg(GE_REG_PRI_V1_X)- GE_ReadReg(GE_REG_PRI_V0_X)+ 1))  \
        {                                                                                       \
            while (1)printk("G2");                                                                          \
        }                                                                                       \
        if (GE_ReadReg(GE_REG_STBB_HEIGHT)!= (GE_ReadReg(GE_REG_PRI_V1_Y)- GE_ReadReg(GE_REG_PRI_V0_Y)+ 1)) \
        {                                                                                       \
            while (1)printk("G3");                                                                          \
        }                                                                                       \
    }

#else // #ifdef GE_DEBUG

#define _GE_CHECK_BUFFER_ALIGN0(addr, color_fmt)                                while (0);
#define _GE_CHECK_BUFFER_ALIGN1(addr, width, height, pitch, color_fmt)          while (0);
#define _GE_SIMPLE_BB_CHECK()                                                   while (0);

#endif // #ifdef GE_DEBUG

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------
#define delay(ms)

#if 1
#define GE_WaitCmdEmpty()       while(((GE_Reg(GE_REG_STATUS)) & GE_MSK_CMQ_FIFO_STATUS) != 0x80)
#define GE_WaitCmdNotFull()     while ((GE_Reg(GE_REG_STATUS) & GE_MSK_CMQ_FIFO_STATUS)== 0x00)
#else
#define GE_WaitCmdEmpty
#define GE_WaitCmdNotFull
#endif


//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------

DEFINE_SEMAPHORE(g_GESem);

#ifdef DBGLOG
//debug use only
static U16 _bOutFileLog = false;
static U16  *_pu16OutLogAddr = NULL;
static U16  _u16LogCount=0;
#endif

static U32 _u32Reg60hFlag = 0;
#if PATCH_RD_CMD
static U32 _u32Reg0hValue = 0;
#endif
static U16 _u16PeClipX1 = 0;
static U16 _u16PeClipX2 = 0;
static U16 _u16PeClipY1 = 0;
static U16 _u16PeClipY2 = 0;

static U8  _u8TypeGE[32] = "MstarTypeGE";

//----------------------------------------------------------------------------
// Local Function Prototypes
//----------------------------------------------------------------------------
void GE_ConvertRGB2DBFmt(GE_Buffer_Format Fmt, U32 *color, U16* low, U16* high);
// void BLT_ConvertRGB2DBFmt(GE_Buffer_Format Fmt, U32 *colorinfo, U16* low, U16* high);

//-------------------------------------------------------------------------------------------------
static U32 GE_ReadReg(U32 addr)
{
    return GE_Reg(addr);
}

//-------------------------------------------------------------------------------------------------
static void GE_WaitAvailableCMDQueue(U8 u8CMDCount)
{
#if WAIT_CMD_FIFO
    U32                 wait_count= 1;
    unsigned int tt ;
	u8CMDCount = 32 ;
    u8CMDCount = (u8CMDCount > 32)? 32 : u8CMDCount;

    //Note: In Titania, 1 empty count can save 2 commands.
    //      And if the empty item only saves 1 commands, it is treated as empty;
    //      therefore 15 cmmands will need (15+1)/2 empty items.
    u8CMDCount = ((u8CMDCount + 1)>>1);


    // check GE busy status to avoid command queue bug
    tt = 0 ;
    while (((GE_Reg(GE_REG_STATUS)&0x01)))
    {
        //msleep(2) ;
        tt++ ;
        if( (tt&0xFFFFFF)==0 )
            printk("G5 ") ;
    }

    while (((GE_Reg(GE_REG_STATUS)&GE_MSK_CMQ_FIFO_STATUS)>>2) < u8CMDCount)
    {
        if (0x0000== (0xFFF & wait_count))
        {
            printk("[GE INFO] GE wait command queue\n");
        }
        wait_count++;
    }
#endif
}

void outGE_WaitAvailableCMDQueue(void){
	GE_WaitAvailableCMDQueue(32) ;
}

static void GE_WriteReg(U32 u32addr, U32 u32val)
{
#if WAIT_CMD_FIFO
    // Wait GE command queue empty before fire to avoid GE hang
    if (GE_REG_CMD== u32addr)
    {
        GE_WaitAvailableCMDQueue(32);
    }

    GE_Reg(u32addr)=u32val;
#else
    GE_Reg(u32addr)=u32val;
    if ((u32addr == GE_REG_CMD)&&(GE_Reg(GE_REG_EN)& GE_VAL_EN_PE))
    {
        while ( GE_Reg(GE_REG_STATUS)& GE_VAL_BUSY)
        {
            MsOS_YieldTask();
        }
    }
#endif
}

//-------------------------------------------------------------------------------------------------
static U16 GE_CheckInClipWindow(U16 u16X1, U16 u16Y1, U16 u16X2, U16 u16Y2)
{
    if (((u16X1 < _u16PeClipX1) && (u16X2 < _u16PeClipX1)) ||
        ((u16X1 > _u16PeClipX2) && (u16X2 > _u16PeClipX2)) ||
        ((u16Y1 < _u16PeClipY1) && (u16Y2 < _u16PeClipY1)) ||
        ((u16Y1 > _u16PeClipY2) && (u16Y2 > _u16PeClipY2)))
    {
        return FALSE;
    }

    return TRUE;
}


//-------------------------------------------------------------------------------------------------
// Description: for debug output register log use
// Arguments:   enable  -  true or false
//
// Return:      NONE
//
// Notes:       if any
//-------------------------------------------------------------------------------------------------
void MHal_GE_Debug_SetOutputLog(U16 enable)
{
#ifdef DBGLOG
     _bOutFileLog = enable;
     if (enable)
     {
        _pu16OutLogAddr = (U16 *)GE_LOG_ADR;
        memset(_pu16OutLogAddr, 0, 0x100000);
        //write header
        *_pu16OutLogAddr = 0xffff;
        _pu16OutLogAddr++;
        *_pu16OutLogAddr = 0x55AA;
        _pu16OutLogAddr++;
        _u16LogCount =2;
     }
     else
     {
        *_pu16OutLogAddr = 0xffff;
        _pu16OutLogAddr++;
        *_pu16OutLogAddr = _u16LogCount;
        _pu16OutLogAddr++;
        _u16LogCount = 0;
     }
#endif
}

static void GE_Enable_CmdQ(void)
{
    GE_WriteReg(GE_REG_FMT_BLT, GE_VAL_EN_CMDQ);
}

static void GE_Register_Reset(void)
{
/*
#if 1
    GE_WaitAvailableCMDQueue(32);
#else
    while (((GE_Reg(GE_REG_STATUS) & GE_VAL_BUSY))){
        MsOS_YieldTask();
    }
    while (((GE_Reg(GE_REG_STATUS) & 0xfc)>>2) < 32)
    {
        MsOS_YieldTask();
    }
#endif
*/

    // GE_WriteReg(GE_REG_EN, 0);
    GE_WriteReg(GE_REG_EN, GE_VAL_EN_PE);
#if PATCH_RD_CMD
    _u32Reg0hValue = GE_VAL_EN_PE;
#endif

    GE_WriteReg(GE_REG_STBB_TH, 0x08);

    //Set Line pattern to default.
    //GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | GE_VAL_LPT_RESET | (0x3F)));
    //GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | (0x3F)));
    _u32Reg60hFlag = 0;

}

//-------------------------------------------------------------------------------------------------
/// Begin PE Engine drawing, this function should be called before all PE drawing function,
/// and it will lock PE engine resource, reset all PE register and static variable.
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_BeginDraw(void)
{
    down(&g_GESem);

    GE_DEBUGINFO(printk( "MHal_GE_BeginDraw\n"));

#ifdef DYNAMIC_POWER_ON_OFF
    MDrv_Power_ClockTurnOn(E_POWER_MODULE_GE);
#endif

    GE_Register_Reset();

    return GESTATUS_SUCCESS;
}
//-------------------------------------------------------------------------------------------------
/// Initial PE engine
/// @return  None
//-------------------------------------------------------------------------------------------------
void MHal_GE_Init(void)
{
    GE_DEBUGINFO(printk( "MHal_GE_Init\n"));

#ifdef MOVE_TO_USER_SPACE
#ifdef DYNAMIC_POWER_ON_OFF
    MDrv_Power_ClockTurnOn(E_POWER_MODULE_GE);
#endif
#endif

    GE_Enable_CmdQ();

    GE_WriteReg(GE_REG_SCK_LTH0, *(U32*)&_u8TypeGE[0]);
    GE_WriteReg(GE_REG_SCK_LTH1, *(U32*)&_u8TypeGE[4]);
    GE_WriteReg(GE_REG_SCK_HTH0, *(U32*)&_u8TypeGE[8]);
    GE_WriteReg(GE_REG_SCK_HTH1, *(U32*)&_u8TypeGE[12]);

#ifdef DYNAMIC_POWER_ON_OFF
    while ( GE_Reg(GE_REG_STATUS)& GE_VAL_BUSY);
#if MOVE_TO_USER_SPACE
    MDrv_Power_ClockTurnOff(E_POWER_MODULE_GE);
#endif
#endif
}


//-------------------------------------------------------------------------------------------------
/// End PE engine drawing (pair with MHal_GE_BeginDraw), this function should be called after
/// all PE drawing function. And it will release PE engine resource.
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_EndDraw(void)
{
#ifdef DYNAMIC_POWER_ON_OFF
    GE_WaitAvailableCMDQueue(32);
    while (GE_Reg(GE_REG_STATUS)& GE_VAL_BUSY)
    {
        MsOS_YieldTask();
    }
    MDrv_Power_ClockTurnOff(E_POWER_MODULE_GE);
#endif

    up(&g_GESem);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE Engine dither
/// @param  enable \b IN: true/false
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetDither(U16 enable)
{
    U32 u32Value;

    GE_DEBUGINFO(printk("MDrv_SetDither\n"));

    GE_WaitAvailableCMDQueue(4);

#if PATCH_RD_CMD
    u32Value = _u32Reg0hValue;
#else
    u32Value = GE_ReadReg(GE_REG_EN);
#endif

    if(enable)
    {
        u32Value |= GE_VAL_EN_DITHER;
    }
    else
    {
        u32Value &= ~GE_VAL_EN_DITHER;
    }

    GE_WriteReg(GE_REG_EN, u32Value);
#if PATCH_RD_CMD
    _u32Reg0hValue = u32Value;
#endif

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE source color key
/// @param enable   \b IN: true/false\n
///                     When enable is FALSE, do not care the other parameters.\n
/// @param opMode   \b IN: source color key mode
///                      The alpha channel does not matter\n
/// @param fmt      \b IN: source color key format
/// @param ps_color \b IN: pointer of source color key start (GE_RGB_COLOR or GE_BLINK_DATA depend on color format).\n
///                        For all RGB color, the color set as the ARGB8888 format.\n
///                        Each color component need to shift to high bit.\n
///                        Use ARGB1555 as the example, the source color key as the following:\n
///                        ARGB1555  --> ARRRRRGGGGGBBBBB                   (every character represents one bit)\n
///                        *ps_color --> A0000000RRRRR000GGGGG000BBBBB000   (every character represents one bit)\n\n
///                        For GE_FMT_I8 format, the index set to b component (ps_color->b = b).\n
///                        For GE_FMT_1BAAFGBG123433 foramt, the foramt set as the GE_BLINK_DATA.\n
/// @param pe_color \b IN: pointer of source color key end (GE_RGB_COLOR or GE_BLINK_DATA depend on color format).\n
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetSrcColorKey(U16 enable,
                                   GE_COLOR_KEY_MODE opMode,
                                   GE_Buffer_Format fmt,
                                   void *ps_color,
                                   void *pe_color)
{
    U32 u32Value, u32Value2;
    U16 u16Color0, u16Color1;

    GE_DEBUGINFO(printk("MHal_GE_SetSrcColorKey\n"));

    GE_WaitAvailableCMDQueue(8);

#if PATCH_RD_CMD
    u32Value = _u32Reg0hValue;
#else
    u32Value = GE_ReadReg(GE_REG_EN);
#endif

    if(enable)
    {
        u32Value |= GE_VAL_EN_SCK;

        // Color key threshold
        GE_ConvertRGB2DBFmt(fmt, (U32 *)ps_color, &u16Color0, &u16Color1);
        GE_WriteReg(GE_REG_SCK_LTH0, u16Color0);
        GE_WriteReg(GE_REG_SCK_LTH1, u16Color1);

        GE_ConvertRGB2DBFmt(fmt, (U32 *)pe_color, &u16Color0, &u16Color1);
        GE_WriteReg(GE_REG_SCK_HTH0, u16Color0);
        GE_WriteReg(GE_REG_SCK_HTH1, u16Color1);

        // Color op
        u32Value2 = GE_ReadReg(GE_REG_KEY_OP);
        u32Value2 = (u32Value2 & ~GE_VAL_SCK_OP_TRUE) | opMode;
        GE_WriteReg(GE_REG_KEY_OP, u32Value2);
    }
    else
    {
        u32Value &= ~(GE_VAL_EN_SCK);
    }

    GE_WriteReg(GE_REG_EN, u32Value);
#if PATCH_RD_CMD
    _u32Reg0hValue = u32Value;
#endif

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE destination color key
/// @param enable   \b IN: true/false\n
///                     When enable is FALSE, do not care the other parameters.\n
/// @param opMode   \b IN: destination color key mode\n
///                      The alpha channel does not matter\n
/// @param fmt      \b IN: destination color key format
/// @param ps_color \b IN: pointer of destination color key start (GE_RGB_COLOR or GE_BLINK_DATA depend on color format).\n
///                        For all RGB color, the color set as the ARGB8888 format.\n
///                        Each color component need to shift to high bit.\n
///                        Use ARGB1555 as the example, the source color key as the following:\n
///                        ARGB1555  --> ARRRRRGGGGGBBBBB                   (every character represents one bit)\n
///                        *ps_color --> A0000000RRRRR000GGGGG000BBBBBB000  (every character represents one bit)\n\n
///                        For GE_FMT_I8 format, the index set to b component (ps_color->b = b).\n
///                        For GE_FMT_1BAAFGBG123433 foramt, the foramt set as the GE_BLINK_DATA.\n
/// @param pe_color \b IN: pointer of destination color key end (GE_RGB_COLOR or GE_BLINK_DATA depend on color format).\n
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetDstColorKey(U16 enable,
                                   GE_COLOR_KEY_MODE opMode,
                                   GE_Buffer_Format fmt,
                                   void *ps_color,
                                   void *pe_color)
{
    U32 u32Value, u32Value2;
    U16 u16Color0, u16Color1;

    GE_DEBUGINFO(printk("MHal_GE_SetDstColorKey\n"));

    GE_WaitAvailableCMDQueue(8);
#if PATCH_RD_CMD
    u32Value = _u32Reg0hValue;
#else
    u32Value = GE_ReadReg(GE_REG_EN);
#endif

    if(enable)
    {
        u32Value |= GE_VAL_EN_DCK;

        // Color key threshold
        GE_ConvertRGB2DBFmt(fmt, (U32 *)ps_color, &u16Color0, &u16Color1);
        GE_WriteReg(GE_REG_DCK_LTH0, u16Color0);
        GE_WriteReg(GE_REG_DCK_LTH1, u16Color1);

        GE_ConvertRGB2DBFmt(fmt, (U32 *)pe_color, &u16Color0, &u16Color1);
        GE_WriteReg(GE_REG_DCK_HTH0, u16Color0);
        GE_WriteReg(GE_REG_DCK_HTH1, u16Color1);

        // Color op
        u32Value2 = GE_ReadReg(GE_REG_KEY_OP);
        u32Value2 = (u32Value2 & ~GE_VAL_DCK_OP_TRUE) | (opMode<<1);
        GE_WriteReg(GE_REG_KEY_OP, u32Value2);
    }
    else
    {
        u32Value &= ~GE_VAL_EN_DCK;
    }
    GE_WriteReg(GE_REG_EN, u32Value);
#if PATCH_RD_CMD
    _u32Reg0hValue = u32Value;
#endif

    return GESTATUS_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/// Set PE intensity : total 16 color Palette in PE
/// @param id \b IN: id of intensity
/// @param fmt \b IN: intensity color format (GE_FMT_ARGB8888 , GE_FMT_1BAAFGBG123433 or GE_FMT_I8)
/// @param pColor \b IN: pointer of intensity (GE_RGB_COLOR or GE_BLINK_DATA depend on color format)
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
/// @return GESTATUS_INVALID_INTENSITY_ID - Inavlid index (id >= 16)
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetIntensity(U32 id, GE_Buffer_Format fmt, U32 *pColor)
{
    U16 u16Color0, u16Color1;
    GE_DEBUGINFO(printk("MHal_GE_SetIntensity\n"));

    GE_WaitAvailableCMDQueue(5);

    if (id < GE_INTENSITY_NUM)
    {
        GE_ConvertRGB2DBFmt(fmt, (U32 *)pColor, &u16Color0, &u16Color1);
        GE_WriteReg(GE_REG_I0_C0 + (2*id), u16Color0);
        GE_WriteReg(GE_REG_I0_C1 + (2*id), u16Color1);
    }
    else
    {
        return GESTATUS_INVALID_INTENSITY_ID;
    }

    return GESTATUS_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/// Set PE raster operation
/// @param enable \b IN: true/false
/// @param eRopMode \b IN: raster operation
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetROP2(U16 enable, GE_ROP2_OP eRopMode)
{
    U32 u32Value, u32Value2;

    GE_WaitAvailableCMDQueue(5);

#if PATCH_RD_CMD
    u32Value = _u32Reg0hValue;
#else
    u32Value = GE_ReadReg(GE_REG_EN);
#endif

    if(enable)
    {
        u32Value |= GE_VAL_EN_ROP;

        u32Value2 = GE_ReadReg(GE_REG_ROP2);
        u32Value2 = (u32Value2 & ~GE_MSK_ROP2) | eRopMode;
        GE_WriteReg(GE_REG_ROP2, u32Value2);
    }
    else
    {
        u32Value &= ~GE_VAL_EN_ROP;
    }
    GE_WriteReg(GE_REG_EN, u32Value);
#if PATCH_RD_CMD
    _u32Reg0hValue = u32Value;
#endif

    return GESTATUS_SUCCESS;
}

void dummyRegWrite( void ){
	GE_WriteReg(GE_REG_STBB_WIDTH, GE_ReadReg(GE_REG_STBB_WIDTH));
}

//-------------------------------------------------------------------------------------------------
/// Force PE draw line
/// @param pline \b IN: pointer to line info
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_DrawLine(GE_DRAW_LINE_INFO *pline)
{
    U32  u32Start, u32End;
    U32  u32Width, u32Height, u32Ratio;
    U32  u32Value, u32Value2;
    U32  u32X1, u32X2, u32Y1, u32Y2;
    S32  i;
    S16  s16Dif;
    U16 bYMajor = false;
    U16 bInverse = false;
    U16 u16Color0, u16Color1;
    GE_RGB_COLOR color_s, color_e;
    GE_BLINK_DATA  *blinkData = NULL;

    GE_DEBUGINFO( printk("MHal_GE_DrawLine\n"));

    GE_WaitAvailableCMDQueue(24);

    if ((!GE_CheckInClipWindow(pline->x1, pline->y1, pline->x2, pline->y2)) && (pline->width == 1))
    {
        return GESTATUS_FAIL;
    }

    GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | GE_VAL_LPT_RESET | (0x3F)));
    GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | (0x3F)));

    u32X1 = pline->x1;
    u32X2 = pline->x2;
    u32Y1 = pline->y1;
    u32Y2 = pline->y2;

    u32Width  = u32X2 - u32X1 ;
    u32Height = u32Y2 - u32Y1 ;
    color_s = pline->colorRange.color_s;
    color_e = pline->colorRange.color_e;

    if ( u32Y1 > u32Y2 )
    {
        u32Height = u32Y1 - u32Y2;
    }

    if ( u32X1 > u32X2 )
    {
        u32Width  = u32X1 - u32X2;
    }


    u32Value2  = GE_VAL_PRIM_LINE;

    if( (u32Height >= u32Width)||(u32Width==0))
    {
        bYMajor = true;
    }

    if (bYMajor)
    {
        if (pline->x1 >= pline->x2)
        {
            if (!GE_CheckInClipWindow(pline->x1+pline->width, pline->y1, pline->x2, pline->y2))
            {
                return GESTATUS_FAIL;
            }
        }
        else
        {
            if (!GE_CheckInClipWindow(pline->x1, pline->y1, pline->x2+pline->width, pline->y2))
            {
                return GESTATUS_FAIL;
            }
        }
    }
    else
    {
        if (pline->y1 >= pline->y2)
        {
            if (!GE_CheckInClipWindow(pline->x1, pline->y1+pline->width, pline->x2, pline->y2))
            {
                return GESTATUS_FAIL;
            }
        }
        else
        {
            if (!GE_CheckInClipWindow(pline->x1, pline->y1, pline->x2, pline->y2+pline->width))
            {
                return GESTATUS_FAIL;
            }
        }
    }
/*
    if ( bYMajor )
    {
        if ( u32X1  >  u32X2)
        {
            u32X1 = pline->x2;
            u32X2 = pline->x1;
            u32Y1 = pline->y2;
            u32Y2 = pline->y1;
            bInverse = true;
        }
        if ( u32Y1  >  u32Y2)
            u32Value2 |= GE_VAL_DRAW_DST_DIR_X_NEG;
    }
    else
    {
        if ( u32Y1  >  u32Y2)
        {
            u32X1 = pline->x2;
            u32X2 = pline->x1;
            u32Y1 = pline->y2;
            u32Y2 = pline->y1;
            bInverse = true;
        }
        if ( u32X1  >  u32X2)
            u32Value2 |= GE_VAL_DRAW_DST_DIR_X_NEG;
    }
*/
    if ( bYMajor )
    {
        if ( u32X1  >  u32X2)
        {
            bInverse = true;
        }
        if ( u32Y1  >  u32Y2)
            u32Value2 |= GE_VAL_DRAW_DST_DIR_X_NEG; // @FIXME: Richard: should be Y NEG???
    }
    else
    {
        if ( u32Y1  >  u32Y2)
        {
            bInverse = true;
        }
        if ( u32X1  >  u32X2)
            u32Value2 |= GE_VAL_DRAW_DST_DIR_X_NEG;
    }

    if ((u32Width==0)||(u32Height==0))
    {
        u32Value = 0;
    }
    else
    {

        if ( bYMajor )
        {
            if (bInverse)
            {
                u32Value = (0x4000 - (U16)Divide2Fixed(u32Width, u32Height, 1, 12)) << 1; //sc
            }
            else
            {
                u32Value = (U16)Divide2Fixed(u32Width, u32Height, 1, 12) << 1; //sc
            }
            bInverse = FALSE;
        }
        else
        {
            if (bInverse)
            {
                u32Value = (0x4000 - (U16)Divide2Fixed(u32Height, u32Width, 1, 12)) << 1; //sc
            }
            else
            {
                u32Value = (U16)Divide2Fixed(u32Height, u32Width, 1, 12) << 1; //sc
            }
            bInverse = FALSE;
        }
    }

    if ( bYMajor )
    {
        GE_WriteReg(GE_REG_LINE_DTA, (u32Value & GE_MSK_LINE_DTA )|GE_VAL_LINE_Y_MAJOR);
    }
    else
    {
        GE_WriteReg(GE_REG_LINE_DTA, (u32Value & GE_MSK_LINE_DTA ));
    }

    // Start color
    // @FIXME: Richard uses GE_FMT_1ABFGBG12355 instead
    if (GE_FMT_1ABFGBG12355!= pline->fmt)
    // if (pline->fmt != GE_FMT_1BAAFGBG123433)
    {
        GE_ConvertRGB2DBFmt(pline->fmt, (U32*)&color_s, &u16Color0, &u16Color1);
    }
    else
    {
#if 0
        // @FIXME: Richard uses GE_FMT_1ABFGBG12355 instead
        //          1 A B Fg Bg
        //          1 2 3  5  5
        //
        //          1 B A A Fg Bg
        //          1 2 3 4  3  3

        blinkData =(GE_BLINK_DATA *)&color_s;
        u16Color0 = ((((blinkData->background&0x7) | ((blinkData->foreground&0x7)<<3))<<2) | ((blinkData->Bits.BlinkAlpha&0xf)<<12));
        u16Color1 = (0xff00 | ((((blinkData->Bits.Blink&0x3)<<3) | (blinkData->Bits.Alpha&0x7))<<3));
#else
        blinkData=      (GE_BLINK_DATA *)&color_s;
        u16Color0=      (0x1F & blinkData->background) |
                        ((0x1F & blinkData->foreground) << 8);
        u16Color1=      (0x7 & blinkData->Bits.Blink) |
                        ((0x3 & blinkData->Bits.Alpha) << 3) |
                        BIT8;
#endif
    }
    GE_WriteReg(GE_REG_PRI_BG_ST, u16Color0);
    GE_WriteReg(GE_REG_PRI_RA_ST, u16Color1);

    if (bYMajor) u32Ratio = u32Height;
    else   u32Ratio = u32Width;

    if(pline->flag & GELINE_FLAG_COLOR_GRADIENT)
    {
        if(bInverse)
        {
            GE_ConvertRGB2DBFmt(pline->fmt, (U32*)&color_e, &u16Color0, &u16Color1);
            GE_WriteReg(GE_REG_PRI_BG_ST, u16Color0);
            GE_WriteReg(GE_REG_PRI_RA_ST, u16Color1);
            //GE_WriteReg(GE_REG_PRI_BG_ST, (pline->color2.b & 0xff) | ((pline->color2.g & 0xff) << 8));
            //GE_WriteReg(GE_REG_PRI_RA_ST, (pline->color2.r & 0xff) | ((pline->color2.a & 0xff) << 8));
        }

        if(bInverse)
        {
            s16Dif = color_s.r - color_e.r;
        }
        else
        {
            s16Dif = color_e.r - color_s.r;
        }
        u32Value = Divide2Fixed(s16Dif, u32Ratio , 7, 12);
        GE_WriteReg(GE_REG_PRI_R_DX0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_R_DX1, u32Value >> 16);

        if(bInverse)
        {
            s16Dif = color_s.g - color_e.g;
        }
        else
        {
            s16Dif = color_e.g - color_s.g;
        }
        u32Value = Divide2Fixed(s16Dif, u32Ratio, 7, 12);
        GE_WriteReg(GE_REG_PRI_G_DX0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_G_DX1, u32Value >> 16);

        if(bInverse)
        {
            s16Dif = color_s.b - color_e.b;
        }
        else
        {
            s16Dif = color_e.b - color_s.b;
        }
        u32Value = Divide2Fixed(s16Dif, u32Ratio, 7, 12);
        GE_WriteReg(GE_REG_PRI_B_DX0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_B_DX1, u32Value >> 16);

        if(bInverse)
        {
            s16Dif = color_s.a - color_e.a;
        }
        else
        {
            s16Dif = color_e.a - color_s.a;
        }
        u32Value = Divide2Fixed(s16Dif, u32Ratio, 4, 11);
        GE_WriteReg(GE_REG_PRI_A_DX, u32Value & 0xffff);

        u32Value2 |= GE_VAL_LINE_GRADIENT;
    }

    GE_ASSERT(0x7FF>= u32X1, printk("[GE DRV][%06d] out of range\n", __LINE__));
    GE_ASSERT(0x7FF>= u32X2, printk("[GE DRV][%06d] out of range\n", __LINE__));
    GE_ASSERT(0x7FF>= u32Y1, printk("[GE DRV][%06d] out of range\n", __LINE__));
    GE_ASSERT(0x7FF>= u32Y2, printk("[GE DRV][%06d] out of range\n", __LINE__));
    GE_WriteReg(GE_REG_PRI_V0_X, u32X1);
    GE_WriteReg(GE_REG_PRI_V1_X, u32X2);
    GE_WriteReg(GE_REG_PRI_V0_Y, u32Y1);
    GE_WriteReg(GE_REG_PRI_V1_Y, u32Y2);

    if(bYMajor)
    {
        u32Start = u32X1;
        u32End = u32X2;
        for(i=0;i<pline->width;i++)
        {
            GE_WaitAvailableCMDQueue(8);
            GE_ASSERT(0x7FF>= u32Start, printk("[GE DRV][%06d] out of range\n", __LINE__));
            GE_ASSERT(0x7FF>= u32End, printk("[GE DRV][%06d] out of range\n", __LINE__));
            GE_WriteReg(GE_REG_LENGTH, u32Height);
            GE_WriteReg(GE_REG_PRI_V0_X, u32Start);
            GE_WriteReg(GE_REG_PRI_V1_X, u32End);
            GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);
            u32Start++;
            u32End++;
        }
    }
    else
    {
        u32Start = u32Y1;
        u32End = u32Y2;
        for (i=0;i<pline->width;i++)
        {
            GE_WaitAvailableCMDQueue(8);
            GE_ASSERT(0x7FF>= u32Start, printk("[GE DRV][%06d] out of range\n", __LINE__));
            GE_ASSERT(0x7FF>= u32End, printk("[GE DRV][%06d] out of range\n", __LINE__));
            GE_WriteReg(GE_REG_LENGTH, u32Width);
            GE_WriteReg(GE_REG_PRI_V0_Y, u32Start);
            GE_WriteReg(GE_REG_PRI_V1_Y, u32End);
            GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);
            u32Start++;
            u32End++;
        }
    }

    return GESTATUS_SUCCESS;
}

GESTATUS MHal_GE_DrawOval(GE_OVAL_FILL_INFO* pOval)
{
    S32 x, y, c_x, c_y;
    S32 Xchange, Ychange;
    S32 EllipseError;
    S32 TwoASquare, TwoBSquare;
    S32 StoppingX, StoppingY;
    U32 Xradius, Yradius;
    U32 u32Value2 = 0;
    U16 u16Color0, u16Color1;
    GE_BLINK_DATA  *blinkData = NULL;

    if (!GE_CheckInClipWindow(pOval->dstBlock.x, pOval->dstBlock.y, pOval->dstBlock.x+pOval->dstBlock.width-1, pOval->dstBlock.y+pOval->dstBlock.height-1))
    {
        return GESTATUS_FAIL;
    }

    GE_WaitAvailableCMDQueue(8);

    Xradius = (pOval->dstBlock.width - pOval->u32LineWidth*2) / 2;
    Yradius = (pOval->dstBlock.height - pOval->u32LineWidth*2) / 2;

    /* center of ellipse */
    //c_x = pOval->dstBlock.x + Xradius + pOval->u32LineWidth/2;
    //c_y = pOval->dstBlock.y + Yradius + pOval->u32LineWidth/2;
    c_x = pOval->dstBlock.x + Xradius + pOval->u32LineWidth;
    c_y = pOval->dstBlock.y + Yradius + pOval->u32LineWidth;

    TwoASquare = 2*Xradius*Xradius;
    TwoBSquare = 2*Yradius*Yradius;

    /*1st set of points*/
    x = Xradius-1;  /*radius zero == draw nothing*/
    y = 0;

    Xchange = Yradius*Yradius*(1-2*Xradius);
    Ychange = Xradius*Xradius;

    EllipseError = 0;

    StoppingX = TwoBSquare*Xradius;
    StoppingY = 0;

    GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | GE_VAL_LPT_RESET | (0x3F)));
    GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | (0x3F)));

    u32Value2  = GE_VAL_PRIM_LINE;
    GE_WriteReg(GE_REG_LINE_DTA, 0);
    // @FIXME: Richard uses GE_FMT_1ABFGBG12355 instead
    if (GE_FMT_1ABFGBG12355!= pOval->fmt)
    // if (pOval->fmt != GE_FMT_1BAAFGBG123433)
    {
        GE_ConvertRGB2DBFmt(pOval->fmt, (U32*)&pOval->color, &u16Color0, &u16Color1);
    }
    else
    {
#if 0
        blinkData =(GE_BLINK_DATA *)&pOval->blink_data;
        u16Color0 = ((((blinkData->background&0x7) | ((blinkData->foreground&0x7)<<3))<<2) | ((blinkData->Bits.BlinkAlpha&0xf)<<12));
        u16Color1 = (0xff00 | ((((blinkData->Bits.Blink&0x3)<<3) | (blinkData->Bits.Alpha&0x7))<<3));
#else
        blinkData =(GE_BLINK_DATA *)&pOval->blink_data;
        u16Color0=      (0x1F & blinkData->background) |
                        ((0x1F & blinkData->foreground) << 8);
        u16Color1=      (0x7 & blinkData->Bits.Blink) |
                        ((0x3 & blinkData->Bits.Alpha) << 3) |
                        BIT8;
#endif
    }
    GE_WriteReg(GE_REG_PRI_BG_ST, u16Color0);
    GE_WriteReg(GE_REG_PRI_RA_ST, u16Color1);




    /*Plot 2 ellipse scan lines for iteration*/
    while (StoppingX > StoppingY)
    {
        GE_WaitAvailableCMDQueue(16);

        GE_WriteReg(GE_REG_PRI_V0_X, c_x - x);
        GE_WriteReg(GE_REG_PRI_V1_X, c_x + x);
        GE_WriteReg(GE_REG_PRI_V0_Y, c_y + y);
        GE_WriteReg(GE_REG_PRI_V1_Y, c_y + y);
        GE_WriteReg(GE_REG_LENGTH, 2*x);
        GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);

        GE_WriteReg(GE_REG_PRI_V0_X, c_x - x);
        GE_WriteReg(GE_REG_PRI_V1_X, c_x + x);
        GE_WriteReg(GE_REG_PRI_V0_Y, c_y - y);
        GE_WriteReg(GE_REG_PRI_V1_Y, c_y - y);
        GE_WriteReg(GE_REG_LENGTH, 2*x);
        GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);

        ++y;
        StoppingY    += TwoASquare;
        EllipseError += Ychange;
        Ychange      += TwoASquare;
        if (( 2*EllipseError + Xchange) > 0)
        {
            --x;
            StoppingX    -= TwoBSquare;
            EllipseError += Xchange;
            Xchange      += TwoBSquare;
        }
    }

    /*2nd set of points*/
    x = 0;
    y = Yradius-1;  /*radius zero == draw nothing*/
    Xchange = Yradius*Yradius;
    Ychange = Xradius*Xradius*(1-2*Yradius);
    EllipseError = 0;
    StoppingX = 0;
    StoppingY = TwoASquare*Yradius;

    /*Plot 2 ellipse scan lines for iteration*/
    while (StoppingX < StoppingY)
    {
        GE_WaitAvailableCMDQueue(16);

        GE_WriteReg(GE_REG_PRI_V0_X, c_x - x);
        GE_WriteReg(GE_REG_PRI_V1_X, c_x + x);
        GE_WriteReg(GE_REG_PRI_V0_Y, c_y + y);
        GE_WriteReg(GE_REG_PRI_V1_Y, c_y + y);
        GE_WriteReg(GE_REG_LENGTH, 2*x);
        GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);

        GE_WriteReg(GE_REG_PRI_V0_X, c_x - x);
        GE_WriteReg(GE_REG_PRI_V1_X, c_x + x);
        GE_WriteReg(GE_REG_PRI_V0_Y, c_y - y);
        GE_WriteReg(GE_REG_PRI_V1_Y, c_y - y);
        GE_WriteReg(GE_REG_LENGTH, 2*x);
        GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);

        ++x;
        StoppingX    += TwoBSquare;
        EllipseError += Xchange;
        Xchange      += TwoBSquare;
        if ((2*EllipseError + Ychange) > 0)
        {
            --y;
            StoppingY    -= TwoASquare;
            EllipseError += Ychange;
            Ychange      += TwoASquare;
        }
    }

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Force PE rectangle fill
/// @param pfillblock \b IN: pointer to block info
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_RectFill(GE_RECT_FILL_INFO *pfillblock)
{
    S16 s16Dif;
    U32 u32Value, u32Value2;
    GE_RGB_COLOR color_s, color_e;
    U16 u16Color0, u16Color1;
    GE_BLINK_DATA  *blinkData = NULL;


    GE_DEBUGINFO( printk("MHal_GE_RectFill\n"));

    if((pfillblock->dstBlock.width == 0) ||(pfillblock->dstBlock.height==0))
    {
        GE_DEBUGINFO(printk("MHal_GE_RectFill error!! width or height equal 0!!\n"));
        return FALSE;
    }

    if (!GE_CheckInClipWindow(pfillblock->dstBlock.x, pfillblock->dstBlock.y,
                              pfillblock->dstBlock.x+pfillblock->dstBlock.width-1,
                              pfillblock->dstBlock.y+pfillblock->dstBlock.height-1))
    {
        return GESTATUS_FAIL;
    }

    GE_WaitAvailableCMDQueue(20);

    //u32Value  = GE_ReadReg(GE_REG_EN);
    //u32Value |= GE_VAL_EN_PE;
    //GE_WriteReg(GE_REG_EN, u32Value);

    GE_WriteReg(GE_REG_PRI_V0_X, pfillblock->dstBlock.x);
    GE_WriteReg(GE_REG_PRI_V0_Y, pfillblock->dstBlock.y);
    GE_WriteReg(GE_REG_PRI_V1_X, pfillblock->dstBlock.x + pfillblock->dstBlock.width - 1);
    GE_WriteReg(GE_REG_PRI_V1_Y, pfillblock->dstBlock.y + pfillblock->dstBlock.height - 1);

    // Start color
    color_s = pfillblock->colorRange.color_s;
    color_e = pfillblock->colorRange.color_e;

    // @FIXME: Richard uses GE_FMT_1ABFGBG12355 instead
    if (GE_FMT_1ABFGBG12355!= pfillblock->fmt)
    // if (pfillblock->fmt != GE_FMT_1BAAFGBG123433)
    {
        GE_ConvertRGB2DBFmt(pfillblock->fmt, (U32*)&color_s, &u16Color0, &u16Color1);
    }
    else
    {
#if 0
        blinkData =(GE_BLINK_DATA *)&color_s;
        u16Color0 = ((((blinkData->background&0x7) | ((blinkData->foreground&0x7)<<3))<<2) | ((blinkData->Bits.BlinkAlpha&0xf)<<12));
        u16Color1 = (0xff00 | ((((blinkData->Bits.Blink&0x3)<<3) | (blinkData->Bits.Alpha&0x7))<<3));
#else
        blinkData=      (GE_BLINK_DATA *)&color_s;
        u16Color0=      (0x1F & blinkData->background) |
                        ((0x1F & blinkData->foreground) << 8);
        u16Color1=      (0x7 & blinkData->Bits.Blink) |
                        ((0x3 & blinkData->Bits.Alpha) << 3) |
                        BIT8;
#endif
    }
    GE_WriteReg(GE_REG_PRI_BG_ST, u16Color0);
    GE_WriteReg(GE_REG_PRI_RA_ST, u16Color1);


    GE_WriteReg(GE_REG_PRI_A_DY, 0);

    // @FIXME
    // check GE_REG_SB_DB_MODE

    u32Value2 = GE_VAL_PRIM_RECTANGLE;

    if((pfillblock->flag & GERECT_FLAG_COLOR_GRADIENT_Y) == GERECT_FLAG_COLOR_GRADIENT_Y)
    {
        s16Dif = color_e.r - color_s.r;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.height - 1, 7, 12);
        GE_WriteReg(GE_REG_PRI_R_DY0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_R_DY1, u32Value >> 16);

        s16Dif = color_e.g - color_s.g;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.height - 1, 7, 12);
        GE_WriteReg(GE_REG_PRI_G_DY0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_G_DY1, u32Value >> 16);

        s16Dif = color_e.b - color_s.b;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.height - 1, 7, 12);
        GE_WriteReg(GE_REG_PRI_B_DY0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_B_DY1, u32Value >> 16);

        s16Dif = color_e.a - color_s.a;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.height - 1, 4, 11);
        GE_WriteReg(GE_REG_PRI_A_DY, u32Value & 0xffff);

        u32Value2 |= GE_VAL_RECT_GRADIENT_V;
    }
    if((pfillblock->flag & GERECT_FLAG_COLOR_GRADIENT_X) == GERECT_FLAG_COLOR_GRADIENT_X)
    {
        s16Dif = color_e.r - color_s.r;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.width - 1, 7, 12);
        GE_WriteReg(GE_REG_PRI_R_DX0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_R_DX1, u32Value >> 16);

        s16Dif = color_e.g - color_s.g;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.width - 1, 7, 12);
        GE_WriteReg(GE_REG_PRI_G_DX0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_G_DX1, u32Value >> 16);

        s16Dif = color_e.b - color_s.b;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.width - 1, 7, 12);
        GE_WriteReg(GE_REG_PRI_B_DX0, u32Value & 0xffff);
        GE_WriteReg(GE_REG_PRI_B_DX1, u32Value >> 16);

        s16Dif = color_e.a - color_s.a;
        u32Value = Divide2Fixed(s16Dif, pfillblock->dstBlock.width - 1, 4, 11);
        GE_WriteReg(GE_REG_PRI_A_DX, u32Value & 0xffff);

        u32Value2 |= GE_VAL_RECT_GRADIENT_H;
    }

    // @FIXME
    // check GE_REG_SB_DB_MODE

    GE_WriteReg(GE_REG_CMD, u32Value2|_u32Reg60hFlag);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Reset PE line pattern
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_Line_Pattern_Reset(void)
{
    U32 u32Value;

    GE_WaitAvailableCMDQueue(5);

    u32Value  = GE_ReadReg(GE_REG_LPT) | GE_VAL_LPT_RESET;
    GE_WriteReg(GE_REG_LPT, u32Value);

    u32Value  = GE_ReadReg(GE_REG_LPT) & (~GE_VAL_LPT_RESET);
    GE_WriteReg(GE_REG_LPT, u32Value);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE line pattern
/// @param enable \b IN: true/false
/// @param linePattern \b IN: p0-0x3F one bit represent draw(1) or not draw(0)
/// @param repeatFactor \b IN: 0 : repeat once, 1 : repeat twice, 2: repeat 3, 3: repeat 4
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_Set_Line_Pattern(U16 enable, U8 linePattern, U8 repeatFactor)
{
    U32 u32Value, u32Value2;

    GE_WaitAvailableCMDQueue(6);

#if PATCH_RD_CMD
    u32Value  = _u32Reg0hValue;
#else
    u32Value  = GE_ReadReg(GE_REG_EN);
#endif

    if (enable)
    {
        u32Value  |= GE_VAL_EN_LPT;
        u32Value2 = ((linePattern & GE_MSK_LP) | ((repeatFactor << 6) & GE_MSK_LPT_FACTOR) | GE_VAL_LPT_RESET);
        GE_WriteReg(GE_REG_LPT, u32Value2);
    }
    else
    {
        u32Value  &= ~GE_VAL_EN_LPT;
        GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | GE_VAL_LPT_RESET | (0x3F)));
        GE_WriteReg(GE_REG_LPT, (GE_VAL_LINE_LAST | (0x3F)));
    }
    GE_WriteReg(GE_REG_EN, u32Value);
#if PATCH_RD_CMD
    _u32Reg0hValue = u32Value;
#endif

    return GESTATUS_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/// Set PE Bit blt
/// @param drawbuf \b IN: pointer to drawbuf info
/// @param drawflag \b IN: draw flag \n
///                  GEDRAW_FLAG_DEFAULT \n
///                  GEDRAW_FLAG_SCALE \n
///                  GEDRAW_FLAG_DUPLICAPE \n
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_BitBlt(GE_DRAW_RECT *drawbuf,U32 drawflag)
{
    U32 u32Width, u32Height;
    U32 u32Value, u32Value2;

	if((!drawbuf->dstblk.width)||(!drawbuf->dstblk.height)||(!drawbuf->srcblk.width)||(!drawbuf->srcblk.height)) {
		GE_WARN("GE bilblt: zero condition!!!!!\n");
		return GESTATUS_SUCCESS; // no need to do things, so it means success
	}

    GE_DEBUGINFO( printk( "MHal_GE_BitBlt\n" ) );
    if (!GE_CheckInClipWindow(drawbuf->dstblk.x, drawbuf->dstblk.y,
                              drawbuf->dstblk.x+drawbuf->dstblk.width-1,
                              drawbuf->dstblk.y+drawbuf->dstblk.height-1))
    {
        return GESTATUS_FAIL;
    }

    GE_WaitAvailableCMDQueue(20);

    u32Value2 = GE_ReadReg(GE_REG_FMT_BLT);

    u32Width  = drawbuf->srcblk.width;
    u32Height = drawbuf->srcblk.height;

    GE_WriteReg(GE_REG_STBB_WIDTH, u32Width);
    GE_WriteReg(GE_REG_STBB_HEIGHT,u32Height);

    // Set source coordinate
    GE_WriteReg(GE_REG_PRI_V2_X, drawbuf->srcblk.x);
    GE_WriteReg(GE_REG_PRI_V2_Y, drawbuf->srcblk.y);

    if (_u32Reg60hFlag & GE_VAL_DRAW_SRC_DIR_X_NEG)
    {
        GE_WriteReg(GE_REG_PRI_V2_X, drawbuf->srcblk.x + drawbuf->srcblk.width - 1);
    }

    if (_u32Reg60hFlag & GE_VAL_DRAW_SRC_DIR_Y_NEG)
    {
        GE_WriteReg(GE_REG_PRI_V2_Y, drawbuf->srcblk.y + drawbuf->srcblk.height - 1);
    }

    if(drawflag&GEDRAW_FLAG_DUPLICAPE)
    {
        _u32Reg60hFlag |= GE_VAL_STBB_PATCH;
    }
    else
    {
        _u32Reg60hFlag &= ~GE_VAL_STBB_PATCH;
    }

    if(drawflag&GEDRAW_FLAG_SCALE)
    {
        u32Value = Divide2Fixed(u32Width, drawbuf->dstblk.width, 1, 12);//<< 2 ; //sc
        GE_WriteReg(GE_REG_STBB_DX, u32Value & 0xffff);
        u32Value = Divide2Fixed((u32Width - drawbuf->dstblk.width), 2* drawbuf->dstblk.width, 1, 12);// << 2; //sc
        GE_WriteReg(GE_REG_STBB_INIT_DX, u32Value);

        u32Value = Divide2Fixed(u32Height, drawbuf->dstblk.height, 1, 12);// << 2 ; //sc
        GE_WriteReg(GE_REG_STBB_DY, u32Value & 0xffff);
        u32Value = Divide2Fixed((u32Height - drawbuf->dstblk.height), 2* drawbuf->dstblk.height, 0, 12);// << 2; //sc
        GE_WriteReg(GE_REG_STBB_INIT_DY, u32Value);

        //scale = (U16)((float)(u32Width-1) * ((float)pbmpfmt->width / (float)u32Width));//TODO
//        u32Scale = (((U32)drawbuf->dstblk.width << 5) / u32Width * (u32Width-1)) >> 5; //sc
//        u32Width = u32Scale;  //pbmpfmt->width;
        u32Width = drawbuf->dstblk.width;

        //scale = (U16)((float)(u32Height-1) * ((float)pbmpfmt->height / (float)u32Height));//TODO
//        u32Scale = (((U32)drawbuf->dstblk.height << 5) / u32Height * (u32Height-1)) >> 5; //sc
//        u32Height = u32Scale;  //pbmpfmt->height;
        u32Height = drawbuf->dstblk.height;
        u32Value2 |= GE_VAL_EN_STRETCH_BITBLT;
    }
    else
    {
        u32Width  = drawbuf->dstblk.width;
        u32Height = drawbuf->dstblk.height;
        GE_WriteReg(GE_REG_STBB_DX, 0x1000);
        GE_WriteReg(GE_REG_STBB_DY, 0x1000);
        GE_WriteReg(GE_REG_STBB_INIT_DX, 0);
        GE_WriteReg(GE_REG_STBB_INIT_DY, 0);
        if ((drawbuf->dstblk.width != drawbuf->srcblk.width) ||
            (drawbuf->dstblk.height != drawbuf->srcblk.height))
        {
            u32Value2 |= GE_VAL_EN_STRETCH_BITBLT;
        }
        else
        {
            u32Value2 &= ~GE_VAL_EN_STRETCH_BITBLT;
        }
    }

    GE_WriteReg(GE_REG_FMT_BLT, u32Value2);

    if (u32Value2 & GE_VAL_EN_STRETCH_BITBLT)
    {
        _u32Reg60hFlag |= GE_VAL_STBB_PATCH;
    }

    //------------------------------------------------------------
    // Destination coordinate
    //------------------------------------------------------------
    GE_WriteReg(GE_REG_PRI_V0_X, drawbuf->dstblk.x);
    GE_WriteReg(GE_REG_PRI_V0_Y, drawbuf->dstblk.y);
    GE_WriteReg(GE_REG_PRI_V1_X, drawbuf->dstblk.x + u32Width - 1);
    GE_WriteReg(GE_REG_PRI_V1_Y, drawbuf->dstblk.y + u32Height - 1);

    // @FIXME
    // check GE_REG_SB_DB_MODE

    GE_WriteReg(GE_REG_CMD, (GE_VAL_PRIM_BITBLT|_u32Reg60hFlag));

    return GESTATUS_SUCCESS;
}


GESTATUS MHal_GE_BitBltEx(GE_DRAW_RECT * drawbuf, U32 drawflag, GE_SCALE_INFO * ScaleInfo)
{
    U32 u32Width, u32Height;
    U32 u32Value2;

    GE_DEBUGINFO( printk( "MHal_GE_BitBltEx\n" ) );
    if (!GE_CheckInClipWindow(drawbuf->dstblk.x, drawbuf->dstblk.y,
                              drawbuf->dstblk.x+drawbuf->dstblk.width-1,
                              drawbuf->dstblk.y+drawbuf->dstblk.height-1))
    {
        return GESTATUS_FAIL;
    }

    GE_WaitAvailableCMDQueue(20);

    u32Value2 = GE_ReadReg(GE_REG_FMT_BLT);

    u32Width  = drawbuf->srcblk.width;
    u32Height = drawbuf->srcblk.height;

    GE_WriteReg(GE_REG_STBB_WIDTH, u32Width);
    GE_WriteReg(GE_REG_STBB_HEIGHT,u32Height);

    // Set source coordinate
    GE_WriteReg(GE_REG_PRI_V2_X, drawbuf->srcblk.x);
    GE_WriteReg(GE_REG_PRI_V2_Y, drawbuf->srcblk.y);

    if (_u32Reg60hFlag & GE_VAL_DRAW_SRC_DIR_X_NEG)
    {
        GE_WriteReg(GE_REG_PRI_V2_X, drawbuf->srcblk.x + drawbuf->srcblk.width - 1);
    }

    if (_u32Reg60hFlag & GE_VAL_DRAW_SRC_DIR_Y_NEG)
    {
        GE_WriteReg(GE_REG_PRI_V2_Y, drawbuf->srcblk.y + drawbuf->srcblk.height - 1);
    }

    if(drawflag&GEDRAW_FLAG_DUPLICAPE)
    {
        _u32Reg60hFlag |= GE_VAL_STBB_PATCH;
    }
    else
    {
        _u32Reg60hFlag &= ~GE_VAL_STBB_PATCH;
    }

    if(drawflag&GEDRAW_FLAG_SCALE)
    {
        GE_WriteReg(GE_REG_STBB_DX, (ScaleInfo->u32DeltaX & 0xffff));
        GE_WriteReg(GE_REG_STBB_INIT_DX, (ScaleInfo->u32InitDelatX & 0xffff));
        GE_WriteReg(GE_REG_STBB_DY, (ScaleInfo->u32DeltaY & 0xffff));
        GE_WriteReg(GE_REG_STBB_INIT_DY, (ScaleInfo->u32InitDelatY & 0xffff));

        u32Width = drawbuf->dstblk.width;
        u32Height = drawbuf->dstblk.height;
        u32Value2 |= GE_VAL_EN_STRETCH_BITBLT;
    }
    else
    {
        u32Width  = drawbuf->dstblk.width;
        u32Height = drawbuf->dstblk.height;
        GE_WriteReg(GE_REG_STBB_DX, 0x1000);
        GE_WriteReg(GE_REG_STBB_DY, 0x1000);
        GE_WriteReg(GE_REG_STBB_INIT_DX, 0);
        GE_WriteReg(GE_REG_STBB_INIT_DY, 0);
        if ((drawbuf->dstblk.width != drawbuf->srcblk.width) ||
            (drawbuf->dstblk.height != drawbuf->srcblk.height))
        {
            u32Value2 |= GE_VAL_EN_STRETCH_BITBLT;
        }
        else
        {
            u32Value2 &= ~GE_VAL_EN_STRETCH_BITBLT;
        }
    }

    GE_WriteReg(GE_REG_FMT_BLT, u32Value2);

    if (u32Value2 & GE_VAL_EN_STRETCH_BITBLT)
    {
        _u32Reg60hFlag |= GE_VAL_STBB_PATCH;
    }

    //------------------------------------------------------------
    // Destination coordinate
    //------------------------------------------------------------
    GE_WriteReg(GE_REG_PRI_V0_X, drawbuf->dstblk.x);
    GE_WriteReg(GE_REG_PRI_V0_Y, drawbuf->dstblk.y);
    GE_WriteReg(GE_REG_PRI_V1_X, drawbuf->dstblk.x + u32Width - 1);
    GE_WriteReg(GE_REG_PRI_V1_Y, drawbuf->dstblk.y + u32Height - 1);

    // @FIXME
    // check GE_REG_SB_DB_MODE

    GE_WriteReg(GE_REG_CMD, (GE_VAL_PRIM_BITBLT|_u32Reg60hFlag));

    return GESTATUS_SUCCESS;
}

GESTATUS MHal_GE_GetScaleBltInfo(GE_DRAW_RECT* pdrawbuf, GE_BLOCK* pSrcBlk,  GE_BLOCK* pDstBlk, GE_SCALE_INFO * pScaleInfo)
{
    U32 u32InitDx, u32InitDy;
    U32 u32Temp;
    //U32 u32Temp1;
    U32 u32InverseDeltaX;
    U32 u32InverseDeltaY;
    U16 bXStartMinus_1 = FALSE;
    U16 bYStartMinus_1 = FALSE;
    U32 u32Temp2, u32Temp3;

    if (pdrawbuf->srcblk.width == pdrawbuf->dstblk.width)
    {
        pDstBlk->x  = pSrcBlk->x;
        pDstBlk->width = pSrcBlk->width;
        pScaleInfo->u32DeltaX = 0x1000;
        pScaleInfo->u32InitDelatX = 0;
    }
    else
    {
        pScaleInfo->u32DeltaX = Divide2Fixed(pdrawbuf->srcblk.width, pdrawbuf->dstblk.width, 1, 12);//<< 2 ; //sc
        u32InverseDeltaX = Divide2Fixed(pdrawbuf->dstblk.width, pdrawbuf->srcblk.width, 1, 12);//<< 2 ; //sc
        u32InitDx = Divide2Fixed((pdrawbuf->srcblk.width -pdrawbuf->dstblk.width), 2*pdrawbuf->dstblk.width,0, 12);// << 2; //sc

        if (pdrawbuf->srcblk.width > pdrawbuf->dstblk.width)
        {
            //Horizontal down scaling
            //Get destination x
            if (pSrcBlk->x == 0)
            {
                pDstBlk->x = 0;
            }
            else
            {
                //u32Temp = (((pSrcBlk->x<<12) - (u32InitDx & 0xfff))*pdrawbuf->dstblk.width)/pdrawbuf->srcblk.width;
                u32Temp2 = ((((pSrcBlk->x<<12) - (u32InitDx & 0xfff))<<8)/pScaleInfo->u32DeltaX)>>8;
                u32Temp3 = (((pSrcBlk->x<<12) - (u32Temp2*pScaleInfo->u32DeltaX + u32InitDx))<<12)/pScaleInfo->u32DeltaX;
                u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
                if ((u32Temp & 0xfff) > u32InverseDeltaX)
                {
                    pDstBlk->x  = ((u32Temp & 0xfffff000)>>12)+1;
                    bXStartMinus_1 = FALSE;
                }
                else
                {
                    pDstBlk->x  = ((u32Temp & 0xfffff000)>>12);
                    bXStartMinus_1 = TRUE;
                }
            }

            //Get Destination width
            //u32Temp = ((((pSrcBlk->x+pSrcBlk->width)<<12) - (u32InitDx & 0x1fff))*pdrawbuf->dstblk.width)/pdrawbuf->srcblk.width;
            u32Temp2 = (((((pSrcBlk->x+pSrcBlk->width)<<12) - (u32InitDx & 0xfff))<<8)/pScaleInfo->u32DeltaX)>>8;
            u32Temp3 = ((((pSrcBlk->x+pSrcBlk->width)<<12) - (u32Temp2*pScaleInfo->u32DeltaX + u32InitDx))<<12)/pScaleInfo->u32DeltaX;
            u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
            if ((u32Temp & 0xfff) < u32InverseDeltaX)
            {
                pSrcBlk->width++;
            }
            pDstBlk->width = ((u32Temp & 0xfffff000)>>12) - pDstBlk->x  + 1;


            //Get x initial delta.
            if (bXStartMinus_1)
            {
                pSrcBlk->x--;
                pSrcBlk->width++;
            }
            u32Temp = pDstBlk->x *pScaleInfo->u32DeltaX + u32InitDx;
            //u32Temp = ((pDstBlk->x<<12)*pdrawbuf->srcblk.width)/pdrawbuf->dstblk.width + u32InitDx;
        }
        else
        {
            //Horizontal up scaling
            //Get destination x
            if (pSrcBlk->x == 0)
            {
                pDstBlk->x = 0;
            }
            else
            {
                //u32Temp = (((pSrcBlk->x<<12) - ((~u32InitDx+1)&0xfff))*pdrawbuf->dstblk.width)/pdrawbuf->srcblk.width;
                u32Temp2 = ((((pSrcBlk->x<<12) + ((~u32InitDx+1)&0xfff))<<8)/pScaleInfo->u32DeltaX)>>8;
                u32Temp3 = ((((pSrcBlk->x<<12) - (u32Temp2*pScaleInfo->u32DeltaX - ((~u32InitDx+1)&0xfff))))<<12)/pScaleInfo->u32DeltaX;
                u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
                pDstBlk->x  = (((u32Temp-u32InverseDeltaX)&0xfffff000)>>12)+1;
                bXStartMinus_1 = TRUE;
            }

            //Get Destination width
            //u32Temp = ((((pSrcBlk->x + pSrcBlk->width)<<12) + ((~u32InitDx+1) & 0x1fff))*pdrawbuf->dstblk.width)/pdrawbuf->srcblk.width;
            u32Temp2 = (((((pSrcBlk->x + pSrcBlk->width)<<12) + ((~u32InitDx+1)&0xfff))<<8)/pScaleInfo->u32DeltaX)>>8;
            u32Temp3 = (((((pSrcBlk->x + pSrcBlk->width)<<12) - (u32Temp2*pScaleInfo->u32DeltaX - ((~u32InitDx+1)&0xfff))))<<12)/pScaleInfo->u32DeltaX;
            u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
            pDstBlk->width = ((u32Temp & 0xfffff000)>>12) - pDstBlk->x  + 1;
            pSrcBlk->width++;


            //Get x initial delta.
            if (bXStartMinus_1)
            {
                pSrcBlk->x--;
                pSrcBlk->width++;
            }
            u32Temp = pDstBlk->x *pScaleInfo->u32DeltaX - ((~u32InitDx+1)&0xfff);
            //u32Temp = ((pDstBlk->x<<12)*pdrawbuf->srcblk.width)/pdrawbuf->dstblk.width - ((~u32InitDx+1)&0xfff);
        }

        if (pDstBlk->x == 0)
        {
            pScaleInfo->u32InitDelatX = u32InitDx;
        }
        else
        {
            if (u32Temp > (pSrcBlk->x<<12))
            {
                if ((u32Temp - (pSrcBlk->x<<12)) < 0x1000)
                {
                    pScaleInfo->u32InitDelatX = (u32Temp- (pSrcBlk->x<<12));
                }
                else
                {
                    //Invalid destination X position.
                    //printk("+ ,Invalid destination X position.\n");
                    pScaleInfo->u32InitDelatX  = 0xfff;
                }
            }
            else
            {
                //printk("- ,Invalid destination X direction.\n");
                pScaleInfo->u32InitDelatX  = 0;
            }
        }
    }

    if (pdrawbuf->srcblk.height == pdrawbuf->dstblk.height)
    {
        pDstBlk->y = pSrcBlk->y;
        pDstBlk->height = pSrcBlk->height;
        pScaleInfo->u32DeltaY = 0x1000;
        pScaleInfo->u32InitDelatY = 0;
    }
    else
    {
        pScaleInfo->u32DeltaY = Divide2Fixed(pdrawbuf->srcblk.height, pdrawbuf->dstblk.height, 1, 12);// << 2 ; //sc
        u32InverseDeltaY = Divide2Fixed(pdrawbuf->dstblk.height, pdrawbuf->srcblk.height, 1, 12);// << 2 ; //sc
        u32InitDy = Divide2Fixed((pdrawbuf->srcblk.height - pdrawbuf->dstblk.height), 2*pdrawbuf->dstblk.height, 0, 12);// << 2; //sc


        if (pdrawbuf->srcblk.height > pdrawbuf->dstblk.height)
        {
            //Vertical down scaling
            //Get destination y
            if (pSrcBlk->y == 0)
            {
                pDstBlk->y = 0;
            }
            else
            {
                //u32Temp = (((pSrcBlk->y<<12) - (u32InitDy & 0xfff))*pdrawbuf->dstblk.height)/pdrawbuf->srcblk.height;
                u32Temp2 = ((((pSrcBlk->y<<12) - (u32InitDy & 0xfff))<<8)/pScaleInfo->u32DeltaY)>>8;
                u32Temp3 = (((pSrcBlk->y<<12) - (u32Temp2*pScaleInfo->u32DeltaY + u32InitDy))<<12)/pScaleInfo->u32DeltaY;
                u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
                if ((u32Temp & 0xfff) > u32InverseDeltaY)
                {
                    pDstBlk->y = ((u32Temp & 0xfffff000)>>12)+1;
                    bYStartMinus_1 = FALSE;
                }
                else
                {
                    pDstBlk->y = ((u32Temp & 0xfffff000)>>12);
                    bYStartMinus_1 = TRUE;
                }
            }

            //Get Destination height
            //u32Temp = ((((pSrcBlk->y+pSrcBlk->height)<<12) - (u32InitDy & 0x1fff))*pdrawbuf->dstblk.height)/pdrawbuf->srcblk.height;
            u32Temp2 = (((((pSrcBlk->y+pSrcBlk->height)<<12) - (u32InitDy & 0xfff))<<8)/pScaleInfo->u32DeltaY)>>8;
            u32Temp3 = ((((pSrcBlk->y+pSrcBlk->height)<<12) - (u32Temp2*pScaleInfo->u32DeltaY + u32InitDy))<<12)/pScaleInfo->u32DeltaY;
            u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
            if ((u32Temp & 0xfff) < u32InverseDeltaY)
            {
                pSrcBlk->height++;
            }
            pDstBlk->height = ((u32Temp & 0xfffff000)>>12) - pDstBlk->y + 1;

            //Get x initial delta.
            if (bYStartMinus_1)
            {
                pSrcBlk->y--;
                pSrcBlk->height++;
            }
            u32Temp = pDstBlk->y*pScaleInfo->u32DeltaY + u32InitDy;
            //u32Temp = ((pDstBlk->y<<12)*pdrawbuf->srcblk.height)/pdrawbuf->dstblk.height + u32InitDy;
        }
        else
        {
            //Vertical up scaling
            //Get destination y
            if (pSrcBlk->y == 0)
            {
                pDstBlk->y = 0;
            }
            else
            {
                //u32Temp = (((pSrcBlk->y<<12) + ((~u32InitDy+1)&0xfff))*pdrawbuf->dstblk.height)/pdrawbuf->srcblk.height;
                u32Temp2 = ((((pSrcBlk->y<<12) + ((~u32InitDy+1)&0xfff))<<8)/pScaleInfo->u32DeltaY)>>8;
                u32Temp3 = ((((pSrcBlk->y<<12) - (u32Temp2*pScaleInfo->u32DeltaY - ((~u32InitDy+1)&0xfff))))<<12)/pScaleInfo->u32DeltaY;
                u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
                pDstBlk->y = (((u32Temp-u32InverseDeltaY)&0xfffff000)>>12)+1;
                bYStartMinus_1 = TRUE;
            }

            //Get Destination height
            //u32Temp = ((((pSrcBlk->y + pSrcBlk->height)<<12) + ((~u32InitDy+1) & 0x1fff))*pdrawbuf->dstblk.height)/pdrawbuf->srcblk.height;
            u32Temp2 = (((((pSrcBlk->y + pSrcBlk->height)<<12) + ((~u32InitDy+1)&0xfff))<<8)/pScaleInfo->u32DeltaY)>>8;
            u32Temp3 = (((((pSrcBlk->y + pSrcBlk->height)<<12) - (u32Temp2*pScaleInfo->u32DeltaY - ((~u32InitDy+1)&0xfff))))<<12)/pScaleInfo->u32DeltaY;
            u32Temp = ((u32Temp2<<12)|(u32Temp3&0xfff));
            pDstBlk->height = ((u32Temp & 0xfffff000)>>12) - pDstBlk->y  + 1;
            pSrcBlk->height++;

            //Get x initial delta.
            if (bYStartMinus_1)
            {
                pSrcBlk->y--;
                pSrcBlk->height++;
            }
            u32Temp = pDstBlk->y*pScaleInfo->u32DeltaY- ((~u32InitDy+1)&0xfff);
            //u32Temp = ((pDstBlk->y<<12)*pdrawbuf->srcblk.height)/pdrawbuf->dstblk.height - ((~u32InitDy+1)&0xfff);
        }

        if (pDstBlk->y == 0)
        {
            pScaleInfo->u32InitDelatY = u32InitDy;
        }
        else
        {
            if (u32Temp > (pSrcBlk->y<<12))
            {
                if ((u32Temp - (pSrcBlk->y<<12)) < 0x1000)
                {
                    pScaleInfo->u32InitDelatY = (u32Temp- (pSrcBlk->y<<12));
                }
                else
                {
                    //Invalid destination Y position.
                    //printk("+ ,Invalid destination Y position.\n");
                    pScaleInfo->u32InitDelatY = 0xfff;
                }
            }
            else
            {
                //printk("- ,Invalid destination Y direction.\n");
                pScaleInfo->u32InitDelatY = 0;
            }
        }
    }

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE Screen to Screen bitblt
/// @param psrcblk \b IN: pointer of source block
/// @param pdstblk \b IN: pointer of destination block
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_ScreenCopy(GE_BLOCK *psrcblk, GE_BLOCK *pdstblk)
{
    //U32 u32Addr;
    U32 u32Scale, u32Value, u32Value2, u32Value3;

    GE_DEBUGINFO( printk( "MHal_GE_ScreenCopy\n" ) );

    GE_WaitAvailableCMDQueue(25);

    if (!GE_CheckInClipWindow(pdstblk->x, pdstblk->y,
                              pdstblk->x+pdstblk->width-1, pdstblk->y+pdstblk->height-1))
    {
        return GESTATUS_FAIL;
    }

    //u32Value  = GE_ReadReg(GE_REG_EN);
    //u32Value |= GE_VAL_EN_PE;
    //GE_WriteReg(GE_REG_EN, u32Value);

    u32Value2 = GE_ReadReg(GE_REG_FMT_BLT);

    GE_WriteReg(GE_REG_STBB_WIDTH, psrcblk->width);
    GE_WriteReg(GE_REG_STBB_HEIGHT, psrcblk->height);

    // Set stretch delta
    if(psrcblk->width != pdstblk->width)
    {
        u32Value = Divide2Fixed(psrcblk->width, pdstblk->width, 1, 12);// << 2; //sc
        u32Value2 |= GE_VAL_EN_STRETCH_BITBLT;
        u32Value3 = Divide2Fixed((psrcblk->width - pdstblk->width), 2* pdstblk->width, 1, 12);// << 2; //sc
    }
    else
    {
        u32Value = 0x1000;
        u32Value2 &= ~GE_VAL_EN_STRETCH_BITBLT;
        u32Value3 = 0;
     }
    GE_WriteReg(GE_REG_STBB_DX, u32Value & 0xffff);
    GE_WriteReg(GE_REG_STBB_INIT_DX,u32Value3);

    if(psrcblk->height != pdstblk->height)
    {
        u32Value = (U16)Divide2Fixed(psrcblk->height, pdstblk->height, 1, 12);//<<2; //sc
        u32Value2 |= GE_VAL_EN_STRETCH_BITBLT;
        u32Value3 = Divide2Fixed((psrcblk->height - pdstblk->height), 2* pdstblk->height, 1, 12);// << 2; //sc
    }
    else
    {
        u32Value = 0x1000;
        u32Value2 &= ~GE_VAL_EN_STRETCH_BITBLT;
    }
    GE_WriteReg(GE_REG_STBB_DY, u32Value & 0xffff);
    GE_WriteReg(GE_REG_STBB_INIT_DY,u32Value3);

    GE_WriteReg(GE_REG_FMT_BLT, u32Value2);

    // Source coordinate
    GE_WriteReg(GE_REG_PRI_V2_X, psrcblk->x);
    GE_WriteReg(GE_REG_PRI_V2_Y, psrcblk->y);

    if ((psrcblk->x <= pdstblk->x) && (pdstblk->x <= (psrcblk->x + psrcblk->width)) &&
        (psrcblk->y <= pdstblk->y) && (pdstblk->y <= (psrcblk->y + psrcblk->height)))
    {
        _u32Reg60hFlag |= (GE_VAL_DRAW_DST_DIR_X_NEG | GE_VAL_DRAW_DST_DIR_Y_NEG | GE_VAL_DRAW_SRC_DIR_X_NEG | GE_VAL_DRAW_SRC_DIR_Y_NEG);
    }
    if ((psrcblk->x <= (pdstblk->x + pdstblk->width)) && ((pdstblk->x + pdstblk->width) <= (psrcblk->x + psrcblk->width)) &&
        (psrcblk->y <= pdstblk->y) && (pdstblk->y <= (psrcblk->y + psrcblk->height)))
    {
        _u32Reg60hFlag |= (GE_VAL_DRAW_DST_DIR_Y_NEG | GE_VAL_DRAW_SRC_DIR_Y_NEG);
    }


    if (_u32Reg60hFlag & GE_VAL_DRAW_SRC_DIR_X_NEG)
    {
        GE_WriteReg(GE_REG_PRI_V2_X, psrcblk->x + psrcblk->width - 1);
    }

    if (_u32Reg60hFlag & GE_VAL_DRAW_SRC_DIR_Y_NEG)
    {
        GE_WriteReg(GE_REG_PRI_V2_Y, psrcblk->y + psrcblk->height - 1);
    }


    // Set source pitch buffer information with destination buffer.
    GE_WriteReg(GE_REG_SB_PIT, GE_ReadReg(GE_REG_DB_PIT));
    GE_WriteReg(GE_REG_SB_BASE0, GE_ReadReg(GE_REG_DB_BASE0));
    GE_WriteReg(GE_REG_SB_BASE1, GE_ReadReg(GE_REG_DB_BASE1));
    u32Value = GE_ReadReg(GE_REG_SB_DB_MODE);
    u32Value2 = (u32Value & GE_MSK_DB_FMT) | ((u32Value & GE_MSK_DB_FMT)>>8);
    GE_WriteReg(GE_REG_SB_DB_MODE, u32Value2);


    // Destination coordinate
    if (_u32Reg60hFlag & GE_VAL_DRAW_DST_DIR_X_NEG)
    {
        u32Scale = (((U32)pdstblk->width << 5) / psrcblk->width * (psrcblk->width-1)) >> 5; //sc
        GE_WriteReg(GE_REG_PRI_V0_X, pdstblk->x + u32Scale);
        GE_WriteReg(GE_REG_PRI_V1_X, pdstblk->x);
    }
    else
    {
        GE_WriteReg(GE_REG_PRI_V0_X, pdstblk->x);
        u32Scale = (((U32)pdstblk->width << 5) / psrcblk->width * (psrcblk->width-1)) >> 5; //sc
        GE_WriteReg(GE_REG_PRI_V1_X, pdstblk->x + u32Scale);
    }

    if (_u32Reg60hFlag & GE_VAL_DRAW_DST_DIR_Y_NEG)
    {
        u32Scale = (((U32)pdstblk->height << 5) / psrcblk->height * (psrcblk->height-1)) >> 5; //sc
        GE_WriteReg(GE_REG_PRI_V0_Y, pdstblk->y + u32Scale);
        GE_WriteReg(GE_REG_PRI_V1_Y, pdstblk->y);
    }
    else
    {
        GE_WriteReg(GE_REG_PRI_V0_Y, pdstblk->y);
        u32Scale = (((U32)pdstblk->height << 5) / psrcblk->height * (psrcblk->height-1)) >> 5; //sc
        GE_WriteReg(GE_REG_PRI_V1_Y, pdstblk->y + u32Scale);
    }
/*
    GE_WriteReg(GE_REG_PRI_V0_X, pdstblk->x);
    GE_WriteReg(GE_REG_PRI_V0_Y, pdstblk->y);
    u32Scale = (((U32)pdstblk->width << 5) / psrcblk->width * (psrcblk->width-1)) >> 5; //sc
    GE_WriteReg(GE_REG_PRI_V1_X, pdstblk->x + u32Scale);
    u32Scale = (((U32)pdstblk->height << 5) / psrcblk->height * (psrcblk->height-1)) >> 5; //sc
    GE_WriteReg(GE_REG_PRI_V1_Y, pdstblk->y + u32Scale);
*/
    // @FIXME
    // check GE_REG_SB_DB_MODE

    u32Value = GE_VAL_PRIM_BITBLT;

    GE_WriteReg(GE_REG_CMD, u32Value|_u32Reg60hFlag);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE clipping window
/// @param v0 \b IN: left-top position
/// @param v1 \b IN: right-down position
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetClip(GE_POINT_t* v0, GE_POINT_t* v1)
{
    GE_WaitAvailableCMDQueue(8);
    GE_WriteReg(GE_REG_CLIP_LEFT, v0->x);
    GE_WriteReg(GE_REG_CLIP_TOP, v0->y);
    //GE_WriteReg(GE_REG_CLIP_RIGHT, v1->x - 1);
    //GE_WriteReg(GE_REG_CLIP_BOTTOM, v1->y - 1);
    GE_WriteReg(GE_REG_CLIP_RIGHT, v1->x);
    GE_WriteReg(GE_REG_CLIP_BOTTOM, v1->y);
    _u16PeClipX1 = v0->x;
    _u16PeClipY1 = v0->y;
    _u16PeClipX2 = v1->x;
    _u16PeClipY2 = v1->y;

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE rotate
/// @param angle \b IN: rotate angle
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
/// @note
/// The rotate process can't perform with italic process.
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetRotate(GEROTATE_ANGLE angle)
{
    U32 u32Value;

    GE_WaitAvailableCMDQueue(4);

    u32Value = GE_ReadReg(GE_REG_ROT_MIRROR);
    u32Value &= ~GE_MSK_ROT;
    u32Value |= (U32)angle;
    GE_WriteReg(GE_REG_ROT_MIRROR, u32Value);

    return GESTATUS_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/// Set PE alpha source
/// @param eMode \b IN: alpha source come from , this indicate alpha channel output source
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetAlphaSrcFrom( GE_ALPHA_SRC_FROM eMode )
{
    U32 u32Value;

    GE_WaitAvailableCMDQueue(4);

    u32Value = GE_ReadReg(GE_REG_DB_ABL);

    u32Value = ((u32Value & (~GE_MSK_DB_ABL))| (eMode<<8));
    GE_WriteReg(GE_REG_DB_ABL, u32Value);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
// Description:
// Arguments:   eMode : ABL_FROM_CONST, ABL_FROM_ASRC, ABL_FROM_ADST
//              blendcoef : COEF_ONE,  COEF_CONST,   COEF_ASRC,   COEF_ADST
//                          COEF_ZERO, COEF_1_CONST, COEF_1_ASRC, COEF_1_ADST
//              blendfactor : value : [0,0xff]
// Return:      NONE
//
// Notes:       if any
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// Set PE alpha blending. Dst = A * Src + (1 - A) Dst
/// @param blendcoef       \b IN: alpha source from
/// @param u8ConstantAlpha \b IN: Contant alpha when blendcoef is equal to COEF_CONST
///                               or COEF_1_CONST.
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetAlphaBlending(GE_BLEND_COEF blendcoef, U8 u8ConstantAlpha)
{
    U32 u32Value;

    GE_WaitAvailableCMDQueue(5);

    u32Value = GE_ReadReg(GE_REG_ABL_COEF);
    u32Value &= ~(GE_MSK_ABL_COEF);
    u32Value |= blendcoef ;
    GE_WriteReg(GE_REG_ABL_COEF, u32Value);

    GE_WriteReg(GE_REG_ABL_CONST, (U32)(u8ConstantAlpha&0xff));

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Enable PE alpha blending
/// @param enable \b IN: true/false
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_EnableAlphaBlending(U16 enable)
{
    U32 u32Value;

    GE_WaitAvailableCMDQueue(4);

#if PATCH_RD_CMD
    u32Value = _u32Reg0hValue;
#else
    u32Value = GE_ReadReg(GE_REG_EN);
#endif

    if(enable)
    {
        u32Value |= GE_VAL_EN_GY_ABL;
        GE_WriteReg(GE_REG_EN, u32Value);
    }
    else
    {
        u32Value &= ~GE_VAL_EN_GY_ABL;
        GE_WriteReg(GE_REG_EN, u32Value);
    }

#if PATCH_RD_CMD
    _u32Reg0hValue = u32Value;
#endif

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Enable PE mirror
/// @param isMirrorX \b IN: true/false
/// @param isMirrorY \b IN: true/false
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
/// @note
/// The mirror process can't perform on the source format is GE_FMT_I1, GE_FMT_I2 or GE_FMT_I4.
/// The mirror process can't perform with italic process.
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetMirror(U16 isMirrorX, U16 isMirrorY)
{
    _u32Reg60hFlag  = (_u32Reg60hFlag & ~(GE_VAL_DRAW_SRC_DIR_X_NEG|GE_VAL_DRAW_SRC_DIR_Y_NEG));
    _u32Reg60hFlag |= ( (isMirrorX << 7) | (isMirrorY << 8) );

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Enable PE NearestMode
/// @param enable \b IN: true/false
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetNearestMode(U16 enable)
{
    _u32Reg60hFlag  = (_u32Reg60hFlag & ~(GE_VAL_STBB_NEAREST));
    _u32Reg60hFlag |= (enable << 14);

    return GESTATUS_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
/// Set PE source buffer info
/// @param bufInfo \b IN: buffer handle
/// @param offsetofByte \b IN: start offset (should be 128 bit aligned)
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
/// @return GESTATUS_NON_ALIGN_PITCH - The pitch is not 16 bytes alignment
/// @return GESTATUS_NON_ALIGN_ADDRESS - The address is not 16 bytes alignment
/// @note
/// The buffer start address must be 128 bits alignment.
/// In GE_FMT_I1, GE_FMT_I2 and GE_FMT_I4 format, the pitch must be 8 bits alignment.
/// In other format, the pitch must be 128 bits alignment.
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetSrcBufferInfo(PGE_BUFFER_INFO bufInfo, U32 offsetofByte)
{
    U32 u32Value , u32Addr;

    GE_WaitAvailableCMDQueue(8);

    _GE_CHECK_BUFFER_ALIGN1(
        bufInfo->u32Addr + offsetofByte,
        bufInfo->u32Width,
        bufInfo->u32Height,
        bufInfo->u32Pitch,
        bufInfo->u32ColorFmt);

    if ((bufInfo->u32Addr + offsetofByte)%16)
    {
        printk(KERN_WARNING "GESTATUS_NON_ALIGN_ADDRESS(%d)\n", __LINE__);
        return GESTATUS_NON_ALIGN_ADDRESS;
    }

    u32Value = GE_ReadReg(GE_REG_SB_DB_MODE);
    u32Value = (u32Value & ~GE_MSK_SB_FMT) | (bufInfo->u32ColorFmt); // one-bit format
    GE_WriteReg(GE_REG_SB_DB_MODE, u32Value);


    // Set source address
    u32Addr   = (bufInfo->u32Addr + offsetofByte);
    GE_WriteReg(GE_REG_SB_BASE0, u32Addr & 0xffff);
    GE_WriteReg(GE_REG_SB_BASE1, u32Addr >> 16);

    // Set source pitch
    GE_WriteReg(GE_REG_SB_PIT, (bufInfo->u32Pitch));

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set PE destination buffer info
/// @param bufInfo \b IN: buffer handle
/// @param offsetofByte \b IN: start offset (should be 128 bit aligned)
/// @return GESTATUS_SUCCESS - Success
/// @return GESTATUS_FAIL - Failure
/// @return GESTATUS_NON_ALIGN_PITCH - The pitch is not 16 bytes alignment
/// @return GESTATUS_NON_ALIGN_ADDRESS - The address is not 16 bytes alignment
/// @note
/// The buffer start address and pitch smust be 128 bits alignment.
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_SetDstBufferInfo(PGE_BUFFER_INFO bufInfo, U32 offsetofByte)
{
    U32 u32Value;
    U32 u32Addr = (bufInfo->u32Addr + offsetofByte );

    GE_WaitAvailableCMDQueue(8);

    _GE_CHECK_BUFFER_ALIGN1(
        bufInfo->u32Addr + offsetofByte,
        bufInfo->u32Width,
        bufInfo->u32Height,
        bufInfo->u32Pitch,
        bufInfo->u32ColorFmt);

    if (u32Addr%16)
    {
        printk(KERN_WARNING "GESTATUS_NON_ALIGN_ADDRESS(%d)\n", __LINE__);
        return GESTATUS_NON_ALIGN_ADDRESS;
    }

    // Destination Buffer
    GE_WriteReg(GE_REG_DB_PIT, bufInfo->u32Pitch);                      // Pitch
    GE_WriteReg(GE_REG_DB_BASE0, u32Addr & 0xffff);     // Address
    GE_WriteReg(GE_REG_DB_BASE1, u32Addr >> 16);        // Address

    if( GE_FMT_ARGB1555==bufInfo->u32ColorFmt )
        bufInfo->u32ColorFmt = GE_FMT_ARGB1555_DST;

    // Destination frame buffer format
    u32Value = GE_ReadReg(GE_REG_SB_DB_MODE);
    u32Value = (u32Value & ~GE_MSK_DB_FMT) | (bufInfo->u32ColorFmt<<8); // one-bit format
    GE_WriteReg(GE_REG_SB_DB_MODE, u32Value);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_GetFrameBufferInfo(U32 *width,
                                       U32 *height,
                                       U32 *pitch,
                                       U32 *fbFmt,
                                       U32 *addr)
{
    // TODO: width & height have not been retrieved
    U32 u32Value;

    GE_WaitAvailableCMDQueue(31);

    u32Value = GE_ReadReg(GE_REG_DB_PIT);
    *pitch = u32Value;

    u32Value = GE_ReadReg(GE_REG_SB_DB_MODE);
    *fbFmt = ((u32Value & GE_MSK_DB_FMT)>>8);

    u32Value = GE_ReadReg(GE_REG_DB_BASE0);
    *addr = u32Value;
    u32Value = GE_ReadReg(GE_REG_DB_BASE1);
    *addr |= (u32Value << 16);

    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_YUV_Set(GE_YUV_INFO* pYuvInfo)
{
    U32                         u32YuvInfo= 0;

    GE_ASSERT(pYuvInfo, printk("[GE DRV][%06d] NULL pointer\n", __LINE__));

    // if (bEnable){
    u32YuvInfo|=                (pYuvInfo->rgb2yuv_mode)? GE_VAL_RGB2YUV_255: GE_VAL_RGB2YUV_PC;
    u32YuvInfo|=                (pYuvInfo->yuv_range_out)? GE_VAL_YUV_RANGE_OUT_PC: GE_VAL_YUV_RANGE_OUT_255;
    u32YuvInfo|=                (pYuvInfo->yuv_range_in)? GE_VAL_YUV_RANGE_IN_127: GE_VAL_YUV_RANGE_IN_255;
    u32YuvInfo|=                ((pYuvInfo->yuv_mem_fmt_src & 0x3)<< GE_SHFT_YUV_MEM_FMT_SRC);
    u32YuvInfo|=                ((pYuvInfo->yuv_mem_fmt_dst & 0x3)<< GE_SHFT_YUV_MEM_FMT_DST);
    // }

    GE_WriteReg(GE_REG_YUV, u32YuvInfo);

    return GESTATUS_SUCCESS;
}

GESTATUS MHal_GE_YUV_Get(GE_YUV_INFO* pYuvInfo)
{
    U32                         u32YuvInfo;

    GE_ASSERT(pYuvInfo, printk("[GE DRV][%06d] NULL pointer\n", __LINE__));
    u32YuvInfo=                 GE_ReadReg(GE_REG_YUV);
    pYuvInfo->rgb2yuv_mode=     (u32YuvInfo & GE_VAL_RGB2YUV_255)? GE_RGB2YUV_255_MODE: GE_RGB2YUV_PC_MODE;
    pYuvInfo->yuv_range_out=    (u32YuvInfo & GE_VAL_YUV_RANGE_OUT_PC)? GE_YUV_RANGE_OUT_PC: GE_YUV_RANGE_OUT_255;
    pYuvInfo->yuv_range_in=     (u32YuvInfo & GE_VAL_YUV_RANGE_IN_127)? GE_YUV_RANGE_IN_127: GE_YUV_RANGE_IN_255;
    pYuvInfo->yuv_mem_fmt_src=  (u32YuvInfo & GE_MASK_YUV_MEM_FMT_SRC) >> GE_SHFT_YUV_MEM_FMT_SRC;
    pYuvInfo->yuv_mem_fmt_dst=  (u32YuvInfo & GE_MASK_YUV_MEM_FMT_DST) >> GE_SHFT_YUV_MEM_FMT_DST;
    return GESTATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
void GE_ConvertRGB2DBFmt(GE_Buffer_Format Fmt, U32 *colorinfo, U16* low, U16* high)
{
    GE_RGB_COLOR   *color = NULL;
    GE_BLINK_DATA  *blinkData = NULL;
    //U8 a, r, g, b;

    // @FIXME: Richard uses GE_FMT_1ABFGBG12355 instead
    if (GE_FMT_1ABFGBG12355== Fmt)
    // if (Fmt == GE_FMT_1BAAFGBG123433)
    {
        blinkData =(GE_BLINK_DATA *)colorinfo;
    }
    else
    {
        color     =(GE_RGB_COLOR  *)colorinfo;
    }

    switch (Fmt)
    {
        case GE_FMT_RGB565 :
            *low = ((color->b & 0xf8) + (color->b >> 5)) | (((color->g & 0xfc) + (color->g>>6))<<8);
            *high = ((color->r & 0xf8) + (color->r >> 5)) | ((color->a & 0xff) << 8);
            break;
        case GE_FMT_ARGB1555 :
            *low = ((color->b & 0xf8) + (color->b >> 5)) | (((color->g & 0xf8) + (color->g >> 5))<<8);
            if (color->a > 0)
            {
                *high = ((color->r & 0xf8) + (color->r >> 5)) | (0xff << 8);
            }
            else
            {
                *high = ((color->r & 0xf8) + (color->r >> 5));
            }
            break;
        case GE_FMT_ARGB4444 :
            *low = ((color->b & 0xf0) + (color->b >> 4)) | (((color->g & 0xf0) + (color->g >> 4))<<8);
            *high = ((color->r & 0xf0) + (color->r >> 4)) | (((color->a & 0xf0) + (color->a >> 4))<<8);
            break;
        case GE_FMT_ARGB8888 :
            *low  =(color->b & 0xff) | ((color->g & 0xff) << 8);
            *high =(color->r & 0xff) | ((color->a & 0xff) << 8);
            break;
        case GE_FMT_I8 :
            *low  = (color->b & 0xff)|((color->b & 0xff) << 8);
            *high = (color->b & 0xff)|((color->b & 0xff) << 8);
            break;
        // @FIXME: Richard uses GE_FMT_1ABFGBG12355 instead
        //          1 A B Fg Bg
        //          1 2 3  5  5
        case GE_FMT_1ABFGBG12355:
            *low = (0x1f & blinkData->background) |                     // Bg: 4..0
                   ((0x1f & blinkData->foreground)<< 5) |               // Fg: 9..5
                   ((0x1f & blinkData->ctrl_flag)<< 10) |               // [A, B]: [14..13, 12..10]
                   BIT15;                                               // Bit 15
            *high= (0x1f & blinkData->background) |                     // Bg: 4..0
                   ((0x1f & blinkData->foreground)<< 5) |               // Fg: 9..5
                   ((0x1f & blinkData->ctrl_flag)<< 10) |               // [A, B]: [14..13, 12..10]
                   BIT15;                                               // Bit 15
            break;
#if 0
                    1 B A A Fg Bg
                    1 2 3 4  3  3
        case GE_FMT_1BAAFGBG123433 :
            *low = ((blinkData->background & 0x7)|((blinkData->foreground & 0x7) << 3)|((blinkData->ctrl_flag & 0x1ff)<<6)|(0x1 << 15));
            *high = ((blinkData->background & 0x7)|((blinkData->foreground & 0x7) << 3)|((blinkData->ctrl_flag & 0x1ff)<<6)|(0x1 << 15));
            break;
#endif
        case GE_FMT_YUV422:
            printk("[GE DRV][%06d] Are you sure to draw in YUV?\n", __LINE__);

        default:
            GE_ASSERT(0, printk("[GE DRV][%06d] Bad color format\n", __LINE__));
            *low  =(color->b & 0xff) | ((color->g & 0xff) << 8);
            *high =(color->r & 0xff) | ((color->a & 0xff) << 8);
            break;
    }

}

void MHal_GE_PowerOff(void)
{
    GE_WriteReg(GE_REG_FMT_BLT, GE_VAL_EN_CMDQ);
    GE_WriteReg(GE_REG_EN, GE_VAL_EN_PE);
}

//-------------------------------------------------------------------------------------------------
// Set a palette entry, I8(8-bit palette) format.
// @param palEntry \b IN: ptr to palette data
// @param u32Index   \b IN: Palette Index,
// @return TRUE: sucess / FALSE: fail
// @note: Titania2 GE (support 256 entries: Index: 0-255)
//-------------------------------------------------------------------------------------------------
GESTATUS _MHal_GE_Palette_Set( GePaletteEntry palEntry, U32 u32Index)
{
    U32 u32RegVal;

    GE_WaitAvailableCMDQueue(8);
    u32RegVal=palEntry.u32Data;

    GE_WriteReg(PE_REG_PT_GB, (u32RegVal & 0xFFFF));
	GE_WriteReg(PE_REG_PT_AR,((u32RegVal>>16)& 0xFFFF));
	u32RegVal=GE_ReadReg(PE_REG_PT_IDX);
	GE_WriteReg(PE_REG_PT_IDX, ((u32Index&0xFF)|0x100));
	GE_WriteReg(PE_REG_PT_IDX,(u32RegVal &0xFEFF));

    return TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Set GE palette for I8(8-bit palette) format
/// @param pPalArray       \b IN: ptr to palette entry array
/// @param u32PalStart     \b IN: Define palette entry start index for set (range: 0~255)
/// @param u32PalEnd       \b IN: Define palette entry end index for set (range: 0~255)
/// @return TRUE: sucess / FALSE: fail
/// @note 1: GE palette is single port SRAM, You must set palettes before you read palette table.
/// - I8 (valid palette index: 0~255)
/// - Example: MHal_GE_Palette_Set ( pPalArray, 32, 255)
//-------------------------------------------------------------------------------------------------
GESTATUS MHal_GE_Palette_Set(GePaletteEntry * pPalArray, U32 u32PalStart, U32 u32PalEnd)
{
    U32 u32I;
    GESTATUS bRet = FALSE;

    if((u32PalEnd >= GE_PALETTE_ENTRY_NUM) || (u32PalStart > u32PalEnd))
	{
        return FALSE;
    }

    for (u32I = u32PalStart; u32I<=u32PalEnd; u32I++)
    {
        bRet = _MHal_GE_Palette_Set(pPalArray[u32I], u32I);
        if (bRet == FALSE)
            break;
    }

    return bRet;
}

