////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
////////////////////////////////////////////////////////////////////////////////
#ifndef _HALPNL_UTILITY_H_
#define _HALPNL_UTILITY_H_

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
extern MS_U32 PNL_RIU_BASE;     // This should be inited before XC library starting.


// Address bus of RIU is 16 bits.
#define RIU_READ_BYTE(addr)         ( READ_BYTE( PNL_RIU_BASE+ (addr) ) )
#define RIU_READ_2BYTE(addr)        ( READ_WORD( PNL_RIU_BASE + (addr) ) )
#define RIU_WRITE_BYTE(addr, val)    WRITE_BYTE( (PNL_RIU_BASE + (addr)), val)
#define RIU_WRITE_2BYTE(addr, val)   WRITE_WORD( PNL_RIU_BASE + (addr), val)


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

#define R2BYTE( u32Reg ) RIU_READ_2BYTE( (u32Reg) << 1)

#define R2BYTEMSK( u32Reg, u16mask)\
			( ( RIU_READ_2BYTE( (u32Reg)<< 1) & u16mask  ) )
#define R4BYTE( u32Reg )\
            ( { ((RIU_READ_2BYTE( (u32Reg) << 1)) | ((MS_U32)(RIU_READ_2BYTE( ( (u32Reg) + 2 ) << 1) ) << 16)) ; } )

#define W2BYTE( u32Reg, u16Val) RIU_WRITE_2BYTE( (u32Reg) << 1 , u16Val )

#define W2BYTEMSK( u32Reg, u16Val, u16Mask)\
              RIU_WRITE_2BYTE( (u32Reg)<< 1 , (RIU_READ_2BYTE((u32Reg) << 1) & ~(u16Mask)) | ((u16Val) & (u16Mask)) )

#define W4BYTE( u32Reg, u32Val)\
            ( { RIU_WRITE_2BYTE( (u32Reg) << 1, ((u32Val) & 0x0000FFFF) ); \
                RIU_WRITE_2BYTE( ( (u32Reg) + 2) << 1 , (((u32Val) >> 16) & 0x0000FFFF)) ; } )

#define W3BYTE( u32Reg, u32Val)\
            ( { RIU_WRITE_2BYTE( (u32Reg) << 1,  u32Val); \
                RIU_WRITE_BYTE( (u32Reg + 2) << 1 ,  ((u32Val) >> 16)); } )

#endif
