/*
 * hal_camclk_util.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _HAL_CAMCLK_UTIL_H_
#define _HAL_CAMCLK_UTIL_H_

//-------------------------------------------------------------------------------------------------
// Defines & Macro
//-------------------------------------------------------------------------------------------------

#define CAMCLK_BANK_SIZE 512
#ifdef camclk_IO_OFFSET
#undef camclk_IO_OFFSET
#define camclk_IO_OFFSET 0xDE000000
#else
#define camclk_IO_OFFSET 0x0
#endif
/* macro to get at MMIO space when running virtually */
#define CAMCLK_IO_ADDRESS(x) ((u32)(x) + camclk_IO_OFFSET)

/* read register by byte */
#define camclk_readb(a) (*(volatile unsigned char *)CAMCLK_IO_ADDRESS(a))

/* read register by word */
#define camclk_readw(a) (*(volatile unsigned short *)CAMCLK_IO_ADDRESS(a))

/* read register by long */
#define camclk_readl(a) (*(volatile unsigned int *)CAMCLK_IO_ADDRESS(a))

/* write register by byte */
#define camclk_writeb(a, v) (*(volatile unsigned char *)CAMCLK_IO_ADDRESS(a) = (v))

/* write register by word */
#define camclk_writew(a, v) (*(volatile unsigned short *)CAMCLK_IO_ADDRESS(a) = (v))

/* write register by long */
#define camclk_writel(a, v) (*(volatile unsigned int *)CAMCLK_IO_ADDRESS(a) = (v))

//////////////////////////////////////////////////////////////////////////////////

#define READ_BYTE(x)     camclk_readb(x)
#define READ_WORD(x)     camclk_readw(x)
#define READ_LONG(x)     camclk_readl(x)
#define WRITE_BYTE(x, y) camclk_writeb(x, (u8)(y))
#define WRITE_WORD(x, y) camclk_writew(x, (u16)(y))
#define WRITE_LONG(x, y) camclk_writel(x, (u32)(y))

#define RIU_READ_BYTE(addr)        (READ_BYTE((addr)))
#define RIU_READ_2BYTE(addr)       (READ_WORD((addr)))
#define RIU_WRITE_BYTE(addr, val)  WRITE_BYTE(((addr)), val)
#define RIU_WRITE_2BYTE(addr, val) WRITE_WORD((addr), val)

//////////////////////////////////////////////////////////////////////////////////

#define RBYTE(u32Reg) RIU_READ_BYTE((u32Reg))

#define R2BYTE(u32Reg) RIU_READ_2BYTE((u32Reg))

#define R2BYTEMSK(u32Reg, u16mask) ((RIU_READ_2BYTE((u32Reg)) & u16mask))

//////////////////////////////////////////////////////////////////////////////////

#define WBYTE(u32Reg, u8Val) RIU_WRITE_BYTE(((u32Reg)), u8Val)

#define WBYTEMSK(u32Reg, u8Val, u8Mask)                \
    RIU_WRITE_BYTE((((u32Reg)) - ((u32Reg >> 1) & 1)), \
                   (RIU_READ_BYTE((((u32Reg)) - ((u32Reg >> 1) & 1))) & ~(u8Mask)) | ((u8Val) & (u8Mask)))

#define W2BYTE(u32Reg, u16Val) RIU_WRITE_2BYTE((u32Reg), u16Val)

#define W2BYTEMSK(u32Reg, u16Val, u16Mask) \
    RIU_WRITE_2BYTE((u32Reg), (RIU_READ_2BYTE((u32Reg)) & ~(u16Mask)) | ((u16Val) & (u16Mask)))

//////////////////////////////////////////////////////////////////////////////////

#define camclk_BIT0  0x00000001
#define camclk_BIT1  0x00000002
#define camclk_BIT2  0x00000004
#define camclk_BIT3  0x00000008
#define camclk_BIT4  0x00000010
#define camclk_BIT5  0x00000020
#define camclk_BIT6  0x00000040
#define camclk_BIT7  0x00000080
#define camclk_BIT8  0x00000100
#define camclk_BIT9  0x00000200
#define camclk_BIT10 0x00000400
#define camclk_BIT11 0x00000800
#define camclk_BIT12 0x00001000
#define camclk_BIT13 0x00002000
#define camclk_BIT14 0x00004000
#define camclk_BIT15 0x00008000
#define camclk_BIT16 0x00010000
#define camclk_BIT17 0x00020000
#define camclk_BIT18 0x00040000
#define camclk_BIT19 0x00080000
#define camclk_BIT20 0x00100000
#define camclk_BIT21 0x00200000
#define camclk_BIT22 0x00400000
#define camclk_BIT23 0x00800000
#define camclk_BIT24 0x01000000
#define camclk_BIT25 0x02000000
#define camclk_BIT26 0x04000000
#define camclk_BIT27 0x08000000
#define camclk_BIT28 0x10000000
#define camclk_BIT29 0x20000000
#define camclk_BIT30 0x40000000
#define camclk_BIT31 0x80000000

#endif // _HAL_camclk_UTIL_H_
