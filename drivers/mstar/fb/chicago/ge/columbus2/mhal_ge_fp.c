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

#include "mach/ms_types.h"
#include "mhal_ge.h"


//******************************************************************************/
// This function performs integer division
// parameter:
//  u16x: numerator
//  u16y: denominator
//  nInterger: bit count of integer
//  nFraction: bit count of fraction
// return: a U32 value in s.nInteger.nFraction form
//******************************************************************************/
U32 Divide2Fixed(U16 u16x, U16 u16y, U8 nInteger, U8 nFraction)
{
    U8  neg = 0;
    U32 mask;
    U32 u32x;
    U32 u32y;

    if (u16x & 0x8000)
    {
        u32x = 0xFFFF0000 | u16x;
        u32x = ~u32x + 1; //convert to positive
        neg++;
    }
    else
    {
        u32x = u16x;
    }

    if (u16y & 0x8000)
    {
        u32y = 0xFFFF0000 | u16y;
        u32y = ~u32y + 1; //convert to positive
        neg++;
    }
    else
    {
        u32y = u16y;
    }

    // start calculation
    u32x = (u32x << nFraction) / u32y;
    if (neg % 2)
    {
        u32x = ~u32x + 1;
    }
    // total bit number is: 1(s) + nInteger + nFraction
    mask = (1 << (1 + nInteger + nFraction)) - 1;
    u32x &= mask;

    return u32x;
}

#if 0
void FixPLine_S1_11(U16 u16x, U16 u16y, LONG16_BYTE* pu16ret)
{
    FLOAT_BYTE tmp;
	U16 t, t2;
	U8 exp;

	if(u16y==0)
	{
		pu16ret->u16Num = 0;
		return;
	}
	tmp.fvalue = (float)u16y / (float)u16x;//TODO
	GE_DEBUGINFO(printf("tmp: %02bx %02bx %02bx %02bx\n", tmp.u8Num[0], tmp.u8Num[1], tmp.u8Num[2], tmp.u8Num[3]));

	pu16ret->u16Num = 0;
	if((tmp.u8Num[0]&0x80) > 0)	// negative number
	{
		pu16ret->u8Num[0] |= 0x10;
	}

	exp = (tmp.u8Num[0]<<1) | (tmp.u8Num[1]&0x80)>>7;
	GE_DEBUGINFO(printf("exp = %bx\n", exp));

	t2 = tmp.u8Num[1] & 0x7F;
	t = 0x800 | (t2 << 4) | (tmp.u8Num[2] & 0xF0) >> 4;
	if(exp < 127)
	{
		pu16ret->u16Num |= (t >> (127 - exp));
	}
	else
	{
		pu16ret->u16Num |= (t << (exp - 127));
	}

	GE_DEBUGINFO(printf("FixPLine_S1_11 = %f\n", tmp.fvalue));
	GE_DEBUGINFO(printf("S.1.11 = %x\n", pu16ret->u16Num));

	pu16ret->u16Num = pu16ret->u16Num << 2;
	GE_DEBUGINFO(printf("line reg = %x\n", pu16ret->u16Num));
}

/******************************************************************************/
/******************************************************************************/
void FixPClr_S8_11(S16 s16color, U16 u16dis, LONG32_BYTE* pu32ret)
{
	FLOAT_BYTE tmp;
	U32 t, t2;
	U8 exp;

	tmp.fvalue = (float)s16color / (float)u16dis;//TODO

	GE_DEBUGINFO(printf("tmp: %02bx %02bx %02bx %02bx\n", tmp.u8Num[0], tmp.u8Num[1], tmp.u8Num[2], tmp.u8Num[3]));

	pu32ret->u32Num = 0;	// reset return value
//	if((tmp.u8Num[0]&0x80) > 0)	// negative number
//	{
//		pu32ret->u8Num[1] = 0x8;	// bit 19
//	}

	exp = (tmp.u8Num[0]<<1) | (tmp.u8Num[1]&0x80)>>7;
	GE_DEBUGINFO(printf("exp = %bx\n", exp));

	t2 = tmp.u8Num[1] & 0x7F;
	t = 0x800 | (t2 << 4) | (tmp.u8Num[2] & 0xF0) >> 4;
	if(exp < 127)
	{
		pu32ret->u32Num |= (t >> (127 - exp));
	}
	else
	{
		pu32ret->u32Num |= (t << (exp - 127));
	}

	if((tmp.u8Num[0]&0x80) > 0)	// negative number
	{
		pu32ret->u32Num = 0xFFFFFFFF - pu32ret->u32Num + 1;
	}

	GE_DEBUGINFO(printf("FixPClr_S8_11 = %f\n", tmp.fvalue));
	GE_DEBUGINFO(printf("S.8.11 = %Lx\n", pu32ret->u32Num));

	pu32ret->u32Num = pu32ret->u32Num << 12;
	GE_DEBUGINFO(printf("Color delta reg = %Lx\n", pu32ret->u32Num));
}

/******************************************************************************/
/******************************************************************************/
void FixPBlt_S1_15(U16 u16src, U16 u16dst, LONG16_BYTE* pu16ret)
{
	U32 t2;
	U8 exp;
	FLOAT_BYTE tmp;

	tmp.fvalue = (float)u16src / (float)u16dst;//TODO

	GE_DEBUGINFO(printf("tmp: %02bx %02bx %02bx %02bx\n",
		tmp.u8Num[0], tmp.u8Num[1], tmp.u8Num[2], tmp.u8Num[3]));

	exp = (tmp.u8Num[0]<<1) | (tmp.u8Num[1]&0x80)>>7;
	GE_DEBUGINFO(printf("exp = %bx\n", exp));

	t2 = tmp.u8Num[1] & 0x7F;
	pu16ret->u8Num[1] = tmp.u8Num[2];
	pu16ret->u8Num[0] = 0x80 | t2;
	if(exp < 127)
	{
		pu16ret->u16Num = (pu16ret->u16Num >> (127 - exp));
	}
	else
	{
		pu16ret->u16Num = (pu16ret->u16Num << (exp - 127));
	}

	GE_DEBUGINFO(printf("FixPBlt_S1_15 = %f\n", tmp.fvalue));
	GE_DEBUGINFO(printf("S.1.15 = %x\n", pu16ret->u16Num));
}

#endif //0
