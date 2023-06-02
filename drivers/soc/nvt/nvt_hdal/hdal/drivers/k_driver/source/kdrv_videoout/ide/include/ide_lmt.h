/*
    ide driver limitation header file.

    ide driver limitation of NT96650.

    @file       ide_lmt.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

//Source buffer minimum, maximum and alignment
#define IDE_SRCBUF_WIDTH_MIN               0x00000000                  // Minimum source buffer width
#define IDE_SRCBUF_WIDTH_MAX               0x000007FF                  // Maximum source buffer width
#define IDE_SRCBUF_WIDTH_ALIGN_1_BIT       8                           // Align to 8 pixel (Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_2_BIT       4                           // Align to 4 pixel (Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_4_BIT       2                           // Align to 2 pixel (Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_8_BIT       DRV_LIMIT_ALIGN_BYTE        // Align to 1 Pixel (Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_ARGB4565    DRV_LIMIT_ALIGN_WORD        // Align to 2 pixel (4Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_ARGB8565    DRV_LIMIT_ALIGN_HALFWORD    // Align to 1 Pixel (2Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_422UVP      DRV_LIMIT_ALIGN_HALFWORD    // Align to 2 pixel (2Byte)
#define IDE_SRCBUF_WIDTH_ALIGN_420UVP      DRV_LIMIT_ALIGN_HALFWORD    // Align to 2 pixel (2Byte)
#define IDE_SRCBUF_HIGHT_MIN               0x00000000                  // Minimum source buffer width
#define IDE_SRCBUF_HIGHT_MAX               0x000007FF                  // Maximum source buffer width
#define IDE_SRCBUF_HIGHT_ALIGN_1_BIT       DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_2_BIT       DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_4_BIT       DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_8_BIT       DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_ARGB4565    DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_ARGB8565    DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_422UVP      DRV_LIMIT_ALIGN_BYTE        // Align to 1 line
#define IDE_SRCBUF_HIGHT_ALIGN_420UVP      DRV_LIMIT_ALIGN_HALFWORD    // Align to 2 line

//Source buffer address and lineoffset alignment
#define IDE_SRCBUF_ADDR_ALIGN_1_BIT        DRV_LIMIT_ALIGN_4WORD       // addr alignment 4 Word
#define IDE_SRCBUF_ADDR_ALIGN_2_BIT        DRV_LIMIT_ALIGN_4WORD       // addr alignment 4 Word
#define IDE_SRCBUF_ADDR_ALIGN_4_BIT        DRV_LIMIT_ALIGN_4WORD       // addr alignment 4 Word
#define IDE_SRCBUF_ADDR_ALIGN_8_BIT        DRV_LIMIT_ALIGN_4WORD       // addr alignment 4 Word
#define IDE_SRCBUF_ADDR_ALIGN_ARGB4565     DRV_LIMIT_ALIGN_4WORD       // addr alignment 4 Word
#define IDE_SRCBUF_ADDR_ALIGN_ARGB8565     DRV_LIMIT_ALIGN_4WORD       // addr alignment 4 Word
#define IDE_SRCBUF_ADDR_ALIGN_422UVP       DRV_LIMIT_ALIGN_HALFWORD    // addr alignment 2 Byte
#define IDE_SRCBUF_ADDR_ALIGN_420UVP       DRV_LIMIT_ALIGN_HALFWORD    // addr alignment 2 Byte

#define IDE_SRCBUF_LOFS_ALIGN              DRV_LIMIT_ALIGN_WORD        // Lineoffset alignment word

//Dest window alignment
#define IDE_WIN_X_ALIGN                    DRV_LIMIT_ALIGN_BYTE        // Dest Window X align
#define IDE_WIN_Y_ALIGN                    DRV_LIMIT_ALIGN_BYTE        // Dest Window Y align
#define IDE_WIN_W_ALIGN                    DRV_LIMIT_ALIGN_BYTE        // Dest Window Widht align
#define IDE_WIN_H_ALIGN                    DRV_LIMIT_ALIGN_BYTE        // Dest Window Hight align

//Scaling
#define IDE_SCALE_UP_WIDTH_MAX             (32)                        // Max Scaling up 32X
#define IDE_SCALE_UP_HIGHT_MAX             (32)                        // Max Scaling up 32X
#define IDE_SCALE_DOWN_WIDHT_MAX           ((float)3.99)               // Max Scaling down 3.99X
#define IDE_SCALE_DOWN_HIGHT_MAX           (2)                         // Max Scaling down 2X


