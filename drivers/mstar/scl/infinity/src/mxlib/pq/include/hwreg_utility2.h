////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
////////////////////////////////////////////////////////////////////////////////
#ifndef _HWREG_UTILITY_H_
#define _HWREG_UTILITY_H_

#include "hwreg_pq.h"
#include "MsCommon.h"
//!! Do not include this header in driver or api level
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
extern MS_U32 PQ_RIU_BASE;     // This should be inited before XC library starting.


#define _BITMASK(loc_msb, loc_lsb) ((1U << (loc_msb)) - (1U << (loc_lsb)) + (1U << (loc_msb)))
#define BITMASK(x) _BITMASK(1?x, 0?x)

#define HBMASK    0xFF00
#define LBMASK    0x00FF

#define RIU_MACRO_START     do {
#define RIU_MACRO_END       } while (0)


// Address bus of RIU is 16 bits.
#define RIU_READ_BYTE(addr)         ( READ_BYTE( PQ_RIU_BASE+ (addr) ) )
#define RIU_READ_2BYTE(addr)        ( READ_WORD( PQ_RIU_BASE + (addr) ) )
#define RIU_WRITE_BYTE(addr, val)    WRITE_BYTE( (PQ_RIU_BASE + (addr)), val)
#define RIU_WRITE_2BYTE(addr, val)   WRITE_WORD( PQ_RIU_BASE + (addr), val)


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

//=============================================================
// Standard Form
#define MDrv_ReadByte( u32Reg )   RIU_READ_BYTE(((u32Reg) << 1) - ((u32Reg) & 1))
#define MDrv_Read2Byte( u32Reg )    (RIU_READ_2BYTE((u32Reg)<<1))
#define MDrv_Read4Byte( u32Reg )   ( (MS_U32)RIU_READ_2BYTE((u32Reg)<<1) | ((MS_U32)RIU_READ_2BYTE(((u32Reg)+2)<<1)<<16 )  )
#define MDrv_ReadRegBit( u32Reg, u8Mask )   (RIU_READ_BYTE(((u32Reg)<<1) - ((u32Reg) & 1)) & (u8Mask))
#define MDrv_WriteRegBit( u32Reg, bEnable, u8Mask )                                     \
    RIU_MACRO_START                                                                     \
    RIU_WRITE_BYTE( (((u32Reg) <<1) - ((u32Reg) & 1)) , (bEnable) ? (RIU_READ_BYTE(  (((u32Reg) <<1) - ((u32Reg) & 1))  ) |  (u8Mask)) :                           \
                                (RIU_READ_BYTE( (((u32Reg) <<1) - ((u32Reg) & 1)) ) & ~(u8Mask)));                            \
    RIU_MACRO_END

#define MDrv_WriteByte( u32Reg, u8Val )                                                 \
    RIU_MACRO_START                                                                     \
    RIU_WRITE_BYTE(((u32Reg) << 1) - ((u32Reg) & 1), u8Val);   \
    RIU_MACRO_END

#define MDrv_Write2ByteMask( u32Reg, u16Val, u16Mask) \
    RIU_MACRO_START                                     \
    RIU_WRITE_2BYTE(u32Reg<<1,   (RIU_READ_2BYTE(u32Reg<<1) & ~(u16Mask)) | (u16Val & u16Mask)); \
    RIU_MACRO_END


#define MDrv_Write2Byte( u32Reg, u16Val )                                               \
    RIU_MACRO_START                                                                     \
    if ( ((u32Reg) & 0x01) )                                                        \
    {                                                                               \
        RIU_WRITE_BYTE(((u32Reg) << 1) - 1, (MS_U8)((u16Val)));                                  \
        RIU_WRITE_BYTE(((u32Reg) + 1) << 1, (MS_U8)((u16Val) >> 8));                             \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        RIU_WRITE_2BYTE( ((u32Reg)<<1) ,  u16Val);                                                       \
    }                                                                               \
    RIU_MACRO_END

#define MDrv_Write2Byte_ve( u32Reg, u16Val )                                               \
    RIU_MACRO_START                                                                     \
    if ( ((u32Reg) & 0x01) )                                                        \
    {                                                                               \
        RIU_WRITE_BYTE(((u32Reg) << 1) - 1, (MS_U8)((u16Val)));                                  \
        RIU_WRITE_BYTE(((u32Reg) + 1) << 1, (MS_U8)((u16Val) >> 8));                             \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        RIU_WRITE_2BYTE( ((u32Reg)<<1) ,  u16Val);                                                       \
    }                                                                               \
    RIU_MACRO_END


#define MDrv_Write3Byte( u32Reg, u32Val )   \
    RIU_MACRO_START                         \
    if ((u32Reg) & 0x01)                                                                \
    {                                                                                               \
        RIU_WRITE_BYTE((u32Reg << 1) - 1, u32Val);                                    \
        RIU_WRITE_2BYTE( (u32Reg + 1)<<1 , ((u32Val) >> 8));                                      \
    }                                                                                           \
    else                                                                                        \
    {                                                                                               \
        RIU_WRITE_2BYTE( (u32Reg) << 1,  u32Val);                                                         \
        RIU_WRITE_BYTE( (u32Reg + 2) << 1 ,  ((u32Val) >> 16));                             \
    }                           \
    RIU_MACRO_END

#define MDrv_Write4Byte( u32Reg, u32Val )                                               \
    RIU_MACRO_START                                                                     \
    if ((u32Reg) & 0x01)                                                      \
    {                                                                                               \
        RIU_WRITE_BYTE( ((u32Reg) << 1) - 1 ,  u32Val);                                         \
        RIU_WRITE_2BYTE( ((u32Reg) + 1)<<1 , ( (u32Val) >> 8));                                      \
        RIU_WRITE_BYTE( (((u32Reg) + 3) << 1) ,  ((u32Val) >> 24));                           \
    }                                                                                               \
    else                                                                                                \
    {                                                                                                   \
        RIU_WRITE_2BYTE( (u32Reg) <<1 ,  u32Val);                                                             \
        RIU_WRITE_2BYTE(  ((u32Reg) + 2)<<1 ,  ((u32Val) >> 16));                                             \
    }                                                                     \
    RIU_MACRO_END

#define MDrv_WriteByteMask( u32Reg, u8Val, u8Msk )                                      \
    RIU_MACRO_START                                                                     \
    RIU_WRITE_BYTE( (((u32Reg) <<1) - ((u32Reg) & 1)), (RIU_READ_BYTE((((u32Reg) <<1) - ((u32Reg) & 1))) & ~(u8Msk)) | ((u8Val) & (u8Msk)));                   \
    RIU_MACRO_END
//=============================================================
// Just for Scaler

#define SC_R2BYTEMSK( u32Reg, u16mask)\
    ( ( RIU_READ_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) << 1) & (u16mask) ) )
#define SC_W2BYTEMSK( u32Reg, u16Val, u16Mask)\
    ( ( RIU_WRITE_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) << 1, (RIU_READ_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) << 1) & ~(u16Mask) ) | ((u16Val) & (u16Mask)) ) ))
#define SC_W2BYTE( u32Reg, u16Val)\
	( ( RIU_WRITE_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) << 1 , u16Val  ) ) )
#define SC_R4BYTE( u32Reg )\
    ( ( RIU_READ_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) << 1 ) | (MS_U32)(RIU_READ_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) + 2 ) << 1 )) << 16))
#define SC_W4BYTE( u32Reg, u32Val)\
	 RIU_WRITE_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) <<1, (MS_U16)((u32Val) & 0x0000FFFF) ) ; \
	 RIU_WRITE_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) + 2 ) << 1, (MS_U16)(((u32Val) >> 16) & 0x0000FFFF) )
#define SC_R2BYTE( u32Reg ) \
    ( ( RIU_READ_2BYTE( (REG_SCALER_BASE + ((u32Reg) & 0xFFFF) ) <<1 ) ) )


//=============================================================
//General ( Make sure u32Reg is not ODD
#define W2BYTE( u32Reg, u16Val) RIU_WRITE_2BYTE( (u32Reg) << 1 , u16Val )
#define R2BYTE( u32Reg ) RIU_READ_2BYTE( (u32Reg) << 1)
#define W4BYTE( u32Reg, u32Val)\
            ( { RIU_WRITE_2BYTE( (u32Reg) << 1, ((u32Val) & 0x0000FFFF) ); \
                RIU_WRITE_2BYTE( ( (u32Reg) + 2) << 1 , (((u32Val) >> 16) & 0x0000FFFF)) ; } )
#define R4BYTE( u32Reg )\
            ( { ((RIU_READ_2BYTE( (u32Reg) << 1)) | ((MS_U32)(RIU_READ_2BYTE( ( (u32Reg) + 2 ) << 1) ) << 16)) ; } )

#define R2BYTEMSK( u32Reg, u16mask)\
			( ( RIU_READ_2BYTE( (u32Reg)<< 1) & u16mask  ) )

#define W2BYTEMSK( u32Reg, u16Val, u16Mask)\
	  RIU_WRITE_2BYTE( (u32Reg)<< 1 , (RIU_READ_2BYTE((u32Reg) << 1) & ~(u16Mask)) | ((u16Val) & (u16Mask)) )
#define W2BYTEMSK1( u32Reg, u16Val, u16Mask)\
	( ( RIU_WRITE_2BYTE( (u32Reg)<< 1 , (RIU_READ_2BYTE((u32Reg) << 1) & ~(u16Mask)) | ((u16Val) & (u16Mask)) )  ) )
#define W3BYTE( u32Reg, u32Val)\
            ( { RIU_WRITE_2BYTE( (u32Reg) << 1,  u32Val); \
                RIU_WRITE_BYTE( (u32Reg + 2) << 1 ,  ((u32Val) >> 16)); } )

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
#define MApi_XC_ReadByte(u32Reg)                    MDrv_ReadByte(u32Reg)
#define MApi_XC_WriteByteMask(u32Reg, u8Val, u8Msk) MDrv_WriteByteMask( u32Reg, u8Val, u8Msk )
#define MApi_XC_WriteByte(u32Reg, u8Val)            MDrv_WriteByte( u32Reg, u8Val )

#define MApi_XC_R2BYTE(u32Reg)                      SC_R2BYTE( u32Reg )
#define MApi_XC_R2BYTEMSK(u32Reg, u16mask)          SC_R2BYTEMSK( u32Reg, u16mask)

#define MApi_XC_W2BYTE(u32Reg, u16Val)              SC_W2BYTE( u32Reg, u16Val)
#define MApi_XC_W2BYTEMSK(u32Reg, u16Val, u16mask)  SC_W2BYTEMSK( u32Reg, u16Val, u16mask)

#endif
