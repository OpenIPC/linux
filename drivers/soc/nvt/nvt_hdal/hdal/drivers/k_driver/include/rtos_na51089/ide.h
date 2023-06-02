/**
    Header file for Display library

    Header file for Display library.

    @file       ide.h
    @ingroup    mIDrvDisp_IDE
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _IDE_H
#define _IDE_H

#if defined __KERNEL__
#include "kwrap/type.h"
#include "kdrv_type.h"
#elif defined __FREERTOS
#include "kwrap/type.h"
#include "kdrv_type.h"
#else
#include "Type.h"
#endif

/**
    @addtogroup mIDrvDisp_IDE
*/
//@{

/**
    Display plane(Osd/Video) Enable

    These definitions are used for ide_set_all_window_en()/ide_get_window_en() to enable/disable
    the osd/video layers.

    @name   Display plane(Osd/Video) Enable
*/
//@{
#define DISPLAY_OSD1_EN                 0x02
#define DISPLAY_OSD2_EN                 0x04
#define DISPLAY_VIDEO1_EN               0x08
#define DISPLAY_VIDEO2_EN               0x10
//@}

/**
    Display device type.

    select display device.
\n  The ide engine could output various type of interface format as specified in ide_set_device().
*/
//@{
typedef enum {
	//DISPLAY_DEVICE_CASIO1G          = 0x0,
	DISPLAY_DEVICE_CASIO2G          = 0x1,
	DISPLAY_DEVICE_AU               = 0x2,
	DISPLAY_DEVICE_TOPPOLY          = 0x3,
	DISPLAY_DEVICE_CCIR656          = 0x4,
	DISPLAY_DEVICE_CCIR601          = 0x5,
	DISPLAY_DEVICE_TV               = 0x6,
	DISPLAY_DEVICE_HDMI_24BIT       = 0x7,
	DISPLAY_DEVICE_HDMI_16BIT       = 0x8,
	DISPALY_DEVICE_PARALLEL         = 0x9,
	DISPLAY_DEVICE_CCIR656_16BIT    = 0xA,
	DISPLAY_DEVICE_CCIR601_16BIT    = 0xB,
	DISPLAY_DEVICE_MI               = 0xC,
	DISPLAY_DEVICE_MIPIDSI          = 0xD,
	DISPLAY_DEVICE_RGB_16BIT        = 0xE,
	DISPLAY_DEVICE_OUTPUT_DRAM      = 0xF,

	ENUM_DUMMY4WORD(IDE_DEVICE_TYPE)
} IDE_DEVICE_TYPE;
//@}


/**
    Video buffer operation mode

    select video buffer operation mode. Used in ide_set_v1_buf_op()/ide_set_v2_buf_op().
*/
//@{
typedef enum {
	IDE_VIDEO_BJMODE_CANT_CROSS_WRITE =   0,   ///> Automatically change buffer according to IPE/IME signal
	IDE_VIDEO_BJMODE_RW_INDEPENDENT =     1,   ///> Automatically change buffer according to ide's VSync signal
	IDE_VIDEO_BJMODE_BUFFER_REPEAT =      2,    ///> Repeat the same buffer until user change buffer

	ENUM_DUMMY4WORD(IDE_BJMODE)
} IDE_BJMODE;
//@}

/**
    Total video buffer number

    select the total video buffer number in ide_set_v1_buf_op()/ide_set_v2_buf_op().
*/
//@{
typedef enum {
	IDE_VIDEO_BUFFER_NUM_1 =              0,   ///> Only 1 buffer (Buffer 0)
	IDE_VIDEO_BUFFER_NUM_2 =              1,   ///> Total 2 buffers (Buffer 0, 1)
	IDE_VIDEO_BUFFER_NUM_3 =              2,   ///> Total 3 buffers (Buffer 0, 1, 2)
	IDE_VIDEO_BUFFER_NUM_4 =              3,    ///> Total 4 buffers (Buffer 0, 1, 2, 3)

	ENUM_DUMMY4WORD(IDE_BUF_NUM)
} IDE_BUF_NUM;
//@}

/**
    Video buffer in operation

    select the active buffer in current usage.
\n  These definitions are used in ide_set_v1_buf_op()/ ide_set_v2_buf_op()/ ide_ch_v1_buf()/ ide_ch_v2_buf().
*/
//@{
typedef enum {
	IDE_VIDEO_BUFFER_OPT_0 =             0,   ///> Buffer 0 in operation
	IDE_VIDEO_BUFFER_OPT_1 =             1,   ///> Buffer 1 in operation
	IDE_VIDEO_BUFFER_OPT_2 =             2,   ///> Buffer 2 in operation
	IDE_VIDEO_BUFFER_OPT_3 =             3,    ///> Buffer 3 in operation

	ENUM_DUMMY4WORD(IDE_OP_BUF)
} IDE_OP_BUF;
//@}

/**
    Video/OSD buffer horizontal read mode

    ide can flip the image on the LCD screen without changing the buffer content.
    These definitions select the screen show of horizontal direction from RIGHT or from LEFT.
\n  These definitions are used in ide_set_v1_read_order()/ ide_set_v2_read_order()/ ide_set_o1_read_order()/ ide_set_o2_read_order().
*/
//@{
typedef enum {
	IDE_BUFFER_READ_L2R = 0,                  ///> Read from left to right
	IDE_BUFFER_READ_R2L = 1,                   ///> Read from right to left

	ENUM_DUMMY4WORD(IDE_HOR_READ)
} IDE_HOR_READ;
//@}

/**
    Video/OSD buffer vertical read mode

    ide can flip the image on the LCD screen without changing the buffer content.
    These definitions select the screen show of vertical direction from TOP or from BOTTOM.
\n  These definitions are used in ide_set_v1_read_order()/ ide_set_v2_read_order()/ ide_set_o1_read_order()/ ide_set_o2_read_order().
*/
//@{
typedef enum {
	IDE_BUFFER_READ_T2B = 0,                  ///> Read from top to bottom
	IDE_BUFFER_READ_B2T = 1,                   ///> Read from bottom to top

	ENUM_DUMMY4WORD(IDE_VER_READ)
} IDE_VER_READ;
//@}

/**
    Video hardware auto handshake sync source select.

\n  These definitions are used in ide_set_v1_src().
*/
//@{
typedef enum {
	IDE_SYNC_IME_PATH1 = 0,                  ///> sync with ime path1
	IDE_SYNC_IME_PATH2 = 1,                  ///> sync with ime path2
	IDE_SYNC_IME_PATH3 = 2,                   ///> sync with ime path3

	ENUM_DUMMY4WORD(IDE_SYNC_SRC)
} IDE_SYNC_SRC;
//@}


/**
    Video buffer selection for buffer adddress setting

    This definition is used to dedicate which buffer address is selected to set in ide_set_video_buf_addr().

    @name   Video buffer selection for buffer adddress setting
*/
//@{
#define IDE_VIDEO_ADDR_SEL_ALL              (0x00)      ///< select Video buffer All (0/1/2)
#define IDE_VIDEO_ADDR_SEL_BUFFER0          (0x01 << 0) ///< select Video buffer 0
#define IDE_VIDEO_ADDR_SEL_BUFFER1          (0x01 << 1) ///< select Video buffer 1
#define IDE_VIDEO_ADDR_SEL_BUFFER2          (0x01 << 2) ///< select Video buffer 2
//@}

/**
    Video window ID select.

    This is selection ID to dedicate which video is selcted to change the settings.
\n  This definition is used  in ide_set_video_buf_addr()/ ide_configVideoCst()/ ide_enable_video()/ ... etc.
*/
//@{
typedef enum {
	IDE_VIDEOID_1 = 1,                      ///< ide VIDEO plane 1
	IDE_VIDEOID_2 = 2,                      ///< ide VIDEO plane 2

	ENUM_DUMMY4WORD(IDE_VIDEOID)
} IDE_VIDEOID;
//@}

/**
    OSD window ID select.

    This is selection ID to dedicate which osd is selcted to change the settings.
\n  This definition is used  in ide_set_osd_buf_addr()()/ ide_enable_osd()/ ide_configOsdCst() ... etc.
*/
//@{
typedef enum {
	IDE_OSDID_1 = 1,    ///< OSD plane 1
	IDE_OSDID_2 = 2,    ///< OSD plane 2, some chip revision may not have second OSD plane
	IDE_OSDID_3 = 3,    ///< OSD plane 3, might same as IDE_VIDEOID_1
	IDE_OSDID_4 = 4,     ///< OSD plane 4, might same as IDE_VIDEOID_2

	ENUM_DUMMY4WORD(IDE_OSDID)
} IDE_OSDID;
//@}

/**
    Video blending with Video Key operation select.

    Video1/Video2 blends with VideoKey definitions.
\n  This is for argument of ide_set_video_blend() and return value of
    ide_get_video_blend();
*/
//@{
typedef enum {
	IDE_VIDEO_BLEND_VIDEOCK      = 0x00,    ///< VideoCK
	IDE_VIDEO_BLEND_VIDEOCK7_8   = 0x01,    ///< VideoCK * (7/8) + Video * (1/8)
	IDE_VIDEO_BLEND_VIDEOCK3_4   = 0x02,    ///< VideoCK * (3/4) + Video * (1/4)
	IDE_VIDEO_BLEND_VIDEOCK1_2   = 0x03,    ///< VideoCK * (1/2) + Video * (1/2)
	IDE_VIDEO_BLEND_VIDEOCK1_4   = 0x04,    ///< VideoCK * (1/4) + Video * (3/4)
	IDE_VIDEO_BLEND_VIDEOCK1_8   = 0x05,    ///< VideoCK * (1/8) + Video * (7/8)
	IDE_VIDEO_BLEND_VIDEO1OR2    = 0x06,     ///< Video1 or Video2

	ENUM_DUMMY4WORD(IDE_VIDEO_BLEND_OP)
} IDE_VIDEO_BLEND_OP;
//@}

#if 1

/**
    Video1 blending with Video2 operation select.

    Video1 blends with Video2 definitions.
*/
//@{
typedef enum {
	IDE_VIDEO_BLEND_VIDEO2       = 0x00,    ///< Video2
	IDE_VIDEO_BLEND_VIDEO2_7_8   = 0x01,    ///< Video2 * (7/8) + Video1 * (1/8)
	IDE_VIDEO_BLEND_VIDEO2_3_4   = 0x02,    ///< Video2 * (3/4) + Video1 * (1/4)
	IDE_VIDEO_BLEND_VIDEO2_1_2   = 0x03,    ///< Video2 * (1/2) + Video1 * (1/2)
	IDE_VIDEO_BLEND_VIDEO2_1_4   = 0x04,    ///< Video2 * (1/4) + Video1 * (3/4)
	IDE_VIDEO_BLEND_VIDEO2_1_8   = 0x05,    ///< Video2 * (1/8) + Video1 * (7/8)
	IDE_VIDEO_BLEND_VIDEO1       = 0x06,     ///< Video1

	ENUM_DUMMY4WORD(IDE_V1_V2_BLEND_OP)
} IDE_V1_V2_BLEND_OP;
//@}
#endif

/**
    Video color key select.

    This is for argument of ide_set_video_colorkey_op() and
    return value of ide_get_video_colorkey_op().
*/
//@{
typedef enum {
	IDE_VIDEO_COLORKEY_VIDEO1OR2 = 0x00,    ///< videock = video1 or video2;
	IDE_VIDEO_COLORKEY_YSMALLKEY = 0x01,    ///< videock = (video_Y < VDO_YKEY) ?
	IDE_VIDEO_COLORKEY_YEQUKEY   = 0x02,    ///< videock = (video_Y == VDO_KEY && video_CB == VDO_CBKEY && video_CR == VDO_CRKEY) ?
	IDE_VIDEO_COLORKEY_YBIGKEY   = 0x03,     ///< videock = (video_Y > VDO_YKEY) ?

	ENUM_DUMMY4WORD(IDE_VIDEO_COLORKEY_OP)
} IDE_VIDEO_COLORKEY_OP;
//@}

/**
    OSD color key select.

    This is for argument of ide_setOsd1ColorKeyOp() and ide_setOsd2ColorKeyOp
    return value of ide_getOsd1ColorKeyOp() or ide_getOsd2ColorKeyOp()
*/
//@{
typedef enum {
	IDE_OSD_COLORKEY_EQUAL = 0x00,      ///< osdck = (osd_R == OSD_RKEY && osd_G == OSD_GKEY && osd_B == OSD_BKEY) ?
	IDE_OSD_COLORKEY_EQUAL_A = 0x4,      ///< osdck = (osd_R == OSD_RKEY && osd_G == OSD_GKEY && osd_B == OSD_BKEY && osd_alpha == OSD_AKEY) ?
	ENUM_DUMMY4WORD(IDE_OSD_COLORKEY_OP)
} IDE_OSD_COLORKEY_OP;
//@}


/**
    Video color key compare source select.

    This is for argument of ide_set_video_colorkey_sel() and
    return value of ide_get_video_colorkey_sel()
*/
//@{
typedef enum {
	IDE_VIDEO_COLORKEY_COMPAREVIDEO2 = 0x00, ///< Colorkey will compare with video2
	IDE_VIDEO_COLORKEY_COMPAREVIDEO1 = 0x01,  ///< Colorkey will compare with video1

	ENUM_DUMMY4WORD(IDE_VIDEO_COLORKEY_SEL)
} IDE_VIDEO_COLORKEY_SEL;
//@}

/**
    OSD blending operation select.

    This is definition for osd1/osd2 blending operation usage in ide_setOsdBlendOp().
*/
//@{
typedef enum {
	IDE_OSD_BLEND_OSD2           = 0x00,    ///< OSD2 only
	IDE_OSD_BLEND_OSD2_7_8       = 0x01,    ///< OSD2 * (7/8) + OSD1 * (1/8)
	IDE_OSD_BLEND_OSD2_3_4       = 0x02,    ///< OSD2 * (3/4) + OSD1 * (1/4)
	IDE_OSD_BLEND_OSD2_1_2       = 0x03,    ///< OSD2 * (1/2) + OSD1 * (1/2)
	IDE_OSD_BLEND_OSD2_1_4       = 0x04,    ///< OSD2 * (1/4) + OSD1 * (3/4)
	IDE_OSD_BLEND_OSD2_1_8       = 0x05,    ///< OSD2 * (1/8) + OSD1 * (7/8)
	IDE_OSD_BLEND_OSD1           = 0x06,     ///< OSD1 only

	ENUM_DUMMY4WORD(IDE_OSD_BLEND_OP)
} IDE_OSD_BLEND_OP;
//@}

/**
    OSD palette set select.

    OSD's palette has two banks.
    This definition is used to select osd to use the lower/higher 256 entities as palette.
    Two osd can not use the same bank of palette.
*/
//@{
typedef enum {
	IDE_PALETTE_LOW256           = 0x00,    ///< ide Palette will use lower 256
	IDE_PALETTE_HIGH256          = 0x01,     ///< ide palette will use higher 256

	ENUM_DUMMY4WORD(IDE_PALETTE_SEL)
} IDE_PALETTE_SEL;
//@}

/**
    OSD & Video blending operation select.

    This definition is used to select the blending relation between the osd and video layer.
*/
//@{
typedef enum {
	IDE_OV_BLEND_VIDEO          = 0x00,     ///< Video only
	IDE_OV_BLEND_VIDEO_7_8      = 0x01,     ///< Video * (7/8) + OSD * (1/8)
	IDE_OV_BLEND_VIDEO_3_4      = 0x02,     ///< Video * (3/4) + OSD * (1/4)
	IDE_OV_BLEND_VIDEO_1_2      = 0x03,     ///< Video * (1/2) + OSD * (1/2)
	IDE_OV_BLEND_VIDEO_1_4      = 0x04,     ///< Video * (1/4) + OSD * (3/4)
	IDE_OV_BLEND_VIDEO_1_8      = 0x05,     ///< Video * (1/8) + OSD * (7/8)
	IDE_OV_BLEND_OSD            = 0x06,     ///< OSD only
	IDE_OV_BLEND_VIDEO_AND_OSD  = 0x07,     ///< Video & OSD
	IDE_OV_BLEND_VIDEO_OR_OSD   = 0x08,     ///< Video | OSD
	IDE_OV_BLEND_VIDEO_XOR_OSD  = 0x09,     ///< Video ^ OSD
	IDE_OV_BLEND_NOT_VIDEO      = 0x0A,      ///< !Video

	ENUM_DUMMY4WORD(IDE_OV_BLEND_OP)
} IDE_OV_BLEND_OP;
//@}

/**
    @name Display ICST output type select.
*/
//@{
typedef enum {
	ICST_CCIR601    = 0,                    ///< ICST type 601
	ICST_ITU_BT_709 = 1,                    ///< ICST type 709

	ENUM_DUMMY4WORD(ICST_TYPE)
} ICST_TYPE;
//@}

/**
    Display ICST output type select.

    Not available in this chip version, just for backward compatible usage.
*/
//@{
typedef enum {
	CST_RGB2YCBCR = 0,                     ///< CST RGB to YCBCR
	CST_YCBCR2RGB = 1,                     ///< CST YCBCR to RGB

	ENUM_DUMMY4WORD(CST_SEL)
} CST_SEL;
//@}

/**
    CST accuracy select.

    color space transform accuracy selecion. Used in ide_set_icst_coef()/ ide_setV1CstCoef()/ ... etc.
*/
//@{
typedef enum {
	ACCRCY_1_2_8 = 0,                    ///< CST accuracy 1_2_8
	ACCRCY_1_3_7 = 1,                    ///< CST accuracy 1_3_7

	ENUM_DUMMY4WORD(ACCRCY_SEL)
} ACCRCY_SEL;
//@}

/**
    ide frame buffer pixel format.

    ide input frame buffer format selection for video and osd.
*/
//@{
typedef enum {
	COLOR_1_BIT             = 0,    // color format 1 bit
	COLOR_2_BIT             = 1,    // color format 2 bit
	COLOR_4_BIT             = 2,    // color format 4 bit
	COLOR_8_BIT             = 3,    // color format 8 bit
	COLOR_YCBCR444          = 4,    // color format YCBCR444
	COLOR_YCBCR422          = 5,    // color format YCBCR422
	COLOR_YCBCR420          = 6,    // color format YCBCR420
	COLOR_ARGB4565          = 7,    // color format ARGB4565
	COLOR_ARGB8565          = 8,    // color format ARGB8565
	COLOR_YCC422P           = 9,    // color format YCBCR422Pack
	COLOR_YCC420P           = 10,   // color format YCBCR420Pack
	COLOR_ARGB8888          = 11,   // color format ARGB8888
	COLOR_ARGB4444          = 12,   // color format ARGB4444
	COLOR_ARGB1555          = 13,   // color format ARGB1555

	ENUM_DUMMY4WORD(IDE_COLOR_FORMAT)
} IDE_COLOR_FORMAT;
//@}

/**
    Determine which buffer is selected.

    Used in ide_set_video_buffer_content() to set video buffer content.
*/
//@{
typedef enum {
	IDE_VIDEO_BUFFERID_0,       ///< Buffer0 will be selected
	IDE_VIDEO_BUFFERID_1,       ///< Buffer1 will be selected
	IDE_VIDEO_BUFFERID_2,       ///< Buffer2 will be selected
	IDE_VIDEO_BUFFERID_3,       ///< Buffer3 will be selected
	IDE_VIDEO_BUFFERID_CURRENT,  ///< Current Operating buffer will be selected

	ENUM_DUMMY4WORD(IDE_BUFFERID)
} IDE_BUFFERID;
//@}

/**
    ide LCD pixel direction.

    Set the output pixel RGB direction.
*/
//@{
typedef enum {
	IDE_PDIR_RGB = 0,   ///< R...G...B...
	IDE_PDIR_RBG = 1,    ///< R...B...G...

	ENUM_DUMMY4WORD(IDE_PDIR)
} IDE_PDIR;
//@}

/**
    ide RGB format begin color.

    Used in ide_set_odd()/ ide_set_even() to set the interlaced mode odd/even begin color.
*/
//@{
typedef enum {
	IDE_LCD_R = 0,      ///< color begin from R
	IDE_LCD_G = 1,      ///< color begin from G
	IDE_LCD_B = 2,       ///< color begin from B

	ENUM_DUMMY4WORD(IDE_PORDER)
} IDE_PORDER;
//@}

/**
    ide dithering valid bits.

    select the dithering function valid bits.
*/
//@{
typedef enum {
	IDE_DITHER_4BITS = 0,   ///< Dithering 4 valid bits.
	IDE_DITHER_5BITS = 1,   ///< Dithering 5 valid bits.
	IDE_DITHER_6BITS = 2,   ///< Dithering 6 valid bits.
	IDE_DITHER_7BITS = 3,    ///< Dithering 7 valid bits.

	ENUM_DUMMY4WORD(IDE_DITHER_VBITS)
} IDE_DITHER_VBITS;
//@}

/**
    ide output component selection.

    Output component selection when parallel interface is selected.
*/
//@{
typedef enum {
	IDE_COMPONENT_R = 0,    ///< select output component as R
	IDE_COMPONENT_G = 1,    ///< select output component as G
	IDE_COMPONENT_B = 2,    ///< select output component as B

	IDE_COMPONENT_Y = 0,    ///< select output component as Y
	IDE_COMPONENT_CB = 1,   ///< select output component as Cb
	IDE_COMPONENT_CR = 2,    ///< select output component as Cr

	ENUM_DUMMY4WORD(IDE_OUT_COMP)
} IDE_OUT_COMP;
//@}

/**
    TV src select.

    select the TV src.
*/
//@{
typedef enum {
	IDE_TV_SRC_IDE = 0,     ///< TV source is IDE1
	IDE_TV_SRC_IDE2 = 1,     ///< TV source is IDE2

	ENUM_DUMMY4WORD(IDE_TV_SRC)
} IDE_TV_SRC;
//@}


/**
    TV mode select.

    select the TV mode.
*/
//@{
typedef enum {
	IDE_TV_NTSC_M = 0,      ///< TV mode is NTSC(M)
	IDE_TV_PAL_BDGHI = 1,   ///< TV mode is PAL(BDGHI)
	IDE_TV_PAL_M = 2,       ///< TV mode is PAL(M)
	IDE_TV_PAL_NC = 3,       ///< TV mode is PAL(NC)

	ENUM_DUMMY4WORD(IDE_TV_MODE)
} IDE_TV_MODE;
//@}

/**
    Defines ide palette capability.

    Defines the current palette capability of this chip version.
*/
//@{
typedef enum {
	IDE_PAL_CAP_16X2 = 0,   ///< palette support 16 entities and 2 banks.
	IDE_PAL_CAP_256X1 = 1,  ///< palette support 256 entities and 1 bank.
	IDE_PAL_CAP_256X2 = 2,   ///< palette support 256 entities and 2 banks.

	ENUM_DUMMY4WORD(IDE_PAL_CAP)
} IDE_PAL_CAP;
//@}

/**
    Defines ide alpha blending layer ID

    Defines ide alpha blending layer ID
*/
//@{
typedef enum {
	IDE_BLEND_V1 = 0,                           ///< layer V1
	IDE_BLEND_V2 = 1,                           ///< layer V2
	IDE_BLEND_FD = 2,                           ///< layer FD
	IDE_BLEND_O1 = 3,                           ///< layer O1 (if fmt = ARGB1555 => source alpha = 1)
	IDE_BLEND_O1_GLBALPHA5 = 4,                 ///< layer O1 & fmt=ARGB1555&source alpha = 0

	ENUM_DUMMY4WORD(IDE_BLEND_LAYER)
} IDE_BLEND_LAYER;
//@}

/**
    Defines ide alpha blending type.

    Defines ide alpha blending type.
*/
//@{
typedef enum {
	IDE_NO_ALPHA = 0,                 ///< Disable alpha blending
	IDE_GLOBAL_ALPHA = 1,             ///< select Global alpha blending
	IDE_GLOBAL_ALPHA_BACK = 2,        ///< select Global alpha blending with background
	IDE_SOURCE_ALPHA = 3,             ///< select Source alpha blending
	IDE_SOURCE_ALPHA_BACK = 4,        ///< select Source alpha blending with background

	ENUM_DUMMY4WORD(IDE_ALPHA_TYPE)
} IDE_ALPHA_TYPE;

//@}

/**
   Defines ide scaling method

   Defines ide scaling method
*/
//@{
typedef enum {
	IDE_SCALEMETHOD_DROP = 0,       ///< scaling method drop
	IDE_SCALEMETHOD_BILINEAR = 1,   ///< scaling method bilinear

	ENUM_DUMMY4WORD(IDE_SCALE_METHOD)
} IDE_SCALE_METHOD;
//@}


/**
   Defines ide FD number

   Defines ide FD number
*/
//@{
typedef enum {
	IDE_FD_0 = 0x00000001,        ///< FD 0
	IDE_FD_1 = 0x00000002,        ///< FD 1
	IDE_FD_2 = 0x00000004,        ///< FD 2
	IDE_FD_3 = 0x00000008,        ///< FD 3
	IDE_FD_4 = 0x00000010,        ///< FD 4
	IDE_FD_5 = 0x00000020,        ///< FD 5
	IDE_FD_6 = 0x00000040,        ///< FD 6
	IDE_FD_7 = 0x00000080,        ///< FD 7
	IDE_FD_8 = 0x00000100,        ///< FD 8
	IDE_FD_9 = 0x00000200,        ///< FD 9
	IDE_FD_10 = 0x00000400,       ///< FD 10
	IDE_FD_11 = 0x00000800,       ///< FD 11
	IDE_FD_12 = 0x00001000,       ///< FD 12
	IDE_FD_13 = 0x00002000,       ///< FD 13
	IDE_FD_14 = 0x00004000,       ///< FD 14
	IDE_FD_15 = 0x00008000,       ///< FD 15


	ENUM_DUMMY4WORD(IDE_FD_NUM)
} IDE_FD_NUM;
//@}

/**
   Defines ide Line number

   Defines ide Line number
*/
//@{
typedef enum {
	IDE_LINE_0 = 0x00000001,        ///< LINE 0
	IDE_LINE_1 = 0x00000002,        ///< LINE 1
	IDE_LINE_2 = 0x00000004,        ///< LINE 2
	IDE_LINE_3 = 0x00000008,        ///< LINE 3
	IDE_LINE_4 = 0x00000010,        ///< LINE 4
	IDE_LINE_5 = 0x00000020,        ///< LINE 5
	IDE_LINE_6 = 0x00000040,        ///< LINE 6
	IDE_LINE_7 = 0x00000080,        ///< LINE 7
	IDE_LINE_8 = 0x00000100,        ///< LINE 8
	IDE_LINE_9 = 0x00000200,        ///< LINE 9
	IDE_LINE_10 = 0x00000400,       ///< LINE 10
	IDE_LINE_11 = 0x00000800,       ///< LINE 11
	IDE_LINE_12 = 0x00001000,       ///< LINE 12
	IDE_LINE_13 = 0x00002000,       ///< LINE 13
	IDE_LINE_14 = 0x00004000,       ///< LINE 14
	IDE_LINE_15 = 0x00008000,       ///< LINE 15

	ENUM_DUMMY4WORD(IDE_LINE_NUM)
} IDE_LINE_NUM;
//@}

/**
    ide channel ID

    @note For ide_setConfig()
*/
//@{
typedef enum {
	IDE_ID_1,                          ///< ide Controller
	IDE_ID_2,                          ///< IDE2 Controller

	ENUM_DUMMY4WORD(IDE_ID)
} IDE_ID;
//@}

/**
    Define YCC Clamp

    Define YCC Clamp
*/
//@{
typedef enum {
	IDE_YCCCLAMP_NOCLAMP,             ///< YCC no clamp
	IDE_YCCCLAMP_1_254,               ///< YCC clamp to 1 to 254
	IDE_YCCCLAMP_16_235,              ///< YCC clamp to 16 to 235

	ENUM_DUMMY4WORD(IDE_YCCCLAMP)
} IDE_YCCCLAMP;
//@}

/**
   Defines TV Test mode

   Defines TV Test mode
*/
//@{
typedef enum {
	IDETV_DAC_TESTMODE_SEL_COLORBAR = 0,    ///< TV test mode color bar
	IDETV_DAC_TESTMODE_SEL_RAMP,            ///< TV test mode ramp
	IDETV_DAC_TESTMODE_SEL_FIX,             ///< TV test mode fix value

	ENUM_DUMMY4WORD(IDETV_DAC_TESTMODE_SEL)
} IDETV_DAC_TESTMODE_SEL;
//@}

typedef enum {
	IDETV_CONFIG_ID_TVMODE,
	IDETV_CONFIG_ID_TESTMODE_EN,
	IDETV_CONFIG_ID_TESTMODE_SEL,
	IDETV_CONFIG_ID_BRL,
	IDETV_CONFIG_ID_CKPN_POL,
	IDETV_CONFIG_ID_BLL,
	IDETV_CONFIG_ID_LLVEL_CLAMP,
	IDETV_CONFIG_ID_YLPS_BYPASS,
	IDETV_CONFIG_ID_IRE_SETUP,
	IDETV_CONFIG_ID_Y_SCALE,
	IDETV_CONFIG_ID_CB_SCALE,
	IDETV_CONFIG_ID_CR_SCALE,

	//DAC relative
	IDETV_CONFIG_ID_DAC_PD,

	ENUM_DUMMY4WORD(IDETV_CONFIG_ID)
} IDETV_CONFIG_ID;

typedef enum {
	IDETV_CONFIGCOEF_ID_CHROMA_FILTER,
	IDETV_CONFIGCOEF_ID_Y_FILTER,

	ENUM_DUMMY4WORD(IDETV_CONFIGCOEF_ID)
} IDETV_CONFIGCOEF_ID;


/**
    ide configuration ID

    @note For IDE_setConfig()
*/
typedef enum {
	//
	IDE_CONFIG_DITHER_EN,              ///<
	IDE_CONFIG_DITHER_FREERUN,         ///<
	IDE_CONFIG_DISPDEV,
	IDE_CONFIG_PDIR,
	IDE_CONFIG_LCDL0,
	IDE_CONFIG_LCDL1,
	IDE_CONFIG_HSINV,
	IDE_CONFIG_VSINV,
	IDE_CONFIG_HVLDINV,
	IDE_CONFIG_VVLDINV,
	IDE_CONFIG_CLKINV,
	IDE_CONFIG_FLDINV,
	IDE_CONFIG_RGBDSEL,
	IDE_CONFIG_DEINV,
	IDE_CONFIG_OUTDDR,
	IDE_CONFIG_THROUGHSEL,

	//
	IDE_CONFIG_IDEEN,
	IDE_CONFIG_LAYEREN,

	//TG
	IDE_CONFIG_TG_HSYNC,
	IDE_CONFIG_TG_HTOTAL,
	IDE_CONFIG_TG_HSYNCDLY,
	IDE_CONFIG_TG_HVALIDST,
	IDE_CONFIG_TG_HVALIDED,
	IDE_CONFIG_TG_VSYNC,
	IDE_CONFIG_TG_VTOTAL,
	IDE_CONFIG_TG_VSYNCDLY,
	IDE_CONFIG_TG_FLD0_VVALIDST,
	IDE_CONFIG_TG_FLD0_VVALIDED,
	IDE_CONFIG_TG_FLD1_VVALIDST,
	IDE_CONFIG_TG_FLD1_VVALIDED,
	IDE_CONFIG_TG_INTERLACE_EN,
	IDE_CONFIG_TG_FLD0_CBLKST,
	IDE_CONFIG_TG_FLD0_CBLKED,
	IDE_CONFIG_TG_FLD1_CBLKST,
	IDE_CONFIG_TG_FLD1_CBLKED,
	IDE_CONFIG_TG_CFIDST,
	IDE_CONFIG_TG_CFIDED,

	IDE_CONFIG_LINE_COMPARATIVE_METHOD,

	//

	ENUM_DUMMY4WORD(IDE_CONFIG_ID)
} IDE_CONFIG_ID;


/**
    Display buffer and window size.

    Main structure to dedicate the frame buffer size and the display window size.
*/
//@{
typedef struct {
	UINT32  ui_width;        ///< Display buffer width
	UINT32  ui_height;       ///< Display buffer height
	UINT32  ui_win_width;     ///< Display window width
	UINT32  ui_win_height;    ///< Display window height
} LCDSIZE, *PLCDSIZE;
//@}

/**
    Video buffer address.

    Main structure to dedicate the frame buffer starting address.
*/
//@{
typedef struct {
	UINT32   b0_y;          ///<Buffer0 Y starting address
	UINT32   b0_cb;         ///<Buffer0 Cb/CbCrPack starting address
	UINT32   b0_cr;         ///<Buffer0 Cr starting address (not used in CbCr packed format)
	UINT32   b1_y;          ///<Buffer1 Y starting address
	UINT32   b1_cb;         ///<Buffer1 Cb/CbCrPack starting address
	UINT32   b1_cr;         ///<Buffer1 Cr starting address (not used in CbCr packed format)
	UINT32   b2_y;          ///<Buffer2 Y starting address
	UINT32   b2_cb;         ///<Buffer2 Cb/CbCrPack starting address
	UINT32   b2_cr;         ///<Buffer2 Cr starting address (not used in CbCr packed format)
	UINT32   B3_Y;          ///<Buffer3 Y starting address
	UINT32   B3_CB;         ///<Buffer3 Cb/CbCrPack starting address
	UINT32   B3_CR;         ///<Buffer3 Cr starting address (not used in UVP format)
	UINT8    ui_buf_sel;      ///<individual buffer number selection. It must be the logic OR operation of IDE_VIDEO_ADDR_SEL_BUFFER0, IDE_VIDEO_ADDR_SEL_BUFFER1 and IDE_VIDEO_ADDR_SEL_BUFFER2
} VIDEO_BUF_ADDR;
//@}

/**
    Video buffer dimension and attribute.

    Main structure to dedicate the video buffer attributes.
*/
//@{
typedef struct {
	UINT32   buf_w;             ///<Buffer width
	UINT32   buf_h;             ///<Buffer height
	UINT32   buf_lineoffset;    ///<Buffer line offset
	BOOL     buf_l2r;           ///<Read direction, IDE_BUFFER_READ_L2R or IDE_BUFFER_READ_R2L
	BOOL     buf_t2b;           ///<Read direction, IDE_BUFFER_READ_T2B or IDE_BUFFER_READ_B2T
	UINT32   buf_bjmode;        ///<Buffer operation mode, refer to IDE_VIDEO_BJMODE_XX definition
	UINT32   buf_opt;           ///<Buffer in operation, IDE_VIDEO_BUFFER_OPT_0 ~ 2
	UINT32   buf_num;           ///<Total buffer number, refer to IDE_VIDEO_BUFFER_NUM_X definition
} VIDEO_BUF_ATTR;
//@}

/**
    OSD buffer dimension and attribute.

    Main structure to dedicate the osd buffer attributes.
*/
//@{
typedef struct {
	UINT32   buf_w;             ///<Buffer width
	UINT32   buf_h;             ///<Buffer height
	UINT32   buf_lineoffset;    ///<Buffer line offset
	BOOL     buf_l2r;           ///<Read direction, IDE_BUFFER_READ_L2R or IDE_BUFFER_READ_R2L
	BOOL     buf_t2b;           ///<Read direction, IDE_BUFFER_READ_T2B or IDE_BUFFER_READ_B2T
} OSD_BUF_ATTR;
//@}

/**
    YCbCr color pixel components.

    Used in ide_set_video_buffer_content()/ide_convert_rgb2ycbcr() to specify YCbCr values.
*/
//@{
typedef struct {
	UINT8    color_y;            ///<Y color value
	UINT8    color_cb;           ///<CB color value
	UINT8    color_cr;           ///<CR color value
} YUVCOLOR, *PYUVCOLOR;
//@}

/**
    RGB color pixel components.

    Used in ide_convert_rgb2ycbcr() to specify RGB balues.
*/
//@{
typedef struct {
	UINT8    color_r;            ///<R color value
	UINT8    color_g;            ///<G color value
	UINT8    color_b;            ///<B color value
} RGBCOLOR, *PRGBCOLOR;
//@}

/**
    OSD display dimension and format.

    Used in ide_set_video_win_attr()/ ide_set_osd_win_attr() to setup video/osd  window attributes.
*/
//@{
typedef struct {
	UINT32   source_w;          ///<Buffer width
	UINT32   source_h;          ///<Buffer height
	UINT32   des_w;             ///<Window width
	UINT32   des_h;             ///<Window height
	UINT32   win_format;        ///<Window format, refer to IDE_COLOR_FORMAT type
	UINT32   win_x;             ///<Window start position X
	UINT32   win_y;             ///<Window start position Y
	UINT32   high_addr;         ///<select palette
} VOSD_WINDOW_ATTR;
//@}

/**
    OSD palette entry definition.

    Used in ide_set_palette_entry() to setup specified palette entry content.
*/
//@{
typedef struct {
	UINT32   entry;
	YUVCOLOR osd_color;
	UINT8    blend_op;
	UINT8    blink;
} PALETTE_ENTRY, *p_palette_entry;
//@}

/**
    ide Interrupt Call Back Function

    ide Interrupt Call Back Function
*/
//@{
typedef void (*fp_ide_interrupt_callback)(void);
//@}

//new API
extern ER    idec_open(IDE_ID id);
extern BOOL  idec_is_opened(IDE_ID id);
extern ER    idec_close(IDE_ID id);
extern ER    idec_set_config(IDE_ID IDEID, IDE_CONFIG_ID config_id, UINT32 config_context);
extern ER    idec_set_callback(IDE_ID id, KDRV_CALLBACK_FUNC *p_cb_func);



//-------------------------------------------------
// ide register programm APIs
//-------------------------------------------------

// ide control
void idec_set_dithering(IDE_ID id, BOOL b_en, BOOL b_freerun);
void idec_set_device(IDE_ID id, IDE_DEVICE_TYPE uidevice);
void idec_set_pdir(IDE_ID id, IDE_PDIR b_pdir);
void idec_set_odd(IDE_ID id, IDE_PORDER ui_odd);
void idec_set_even(IDE_ID id, IDE_PORDER ui_even);
void idec_set_hs_inv(IDE_ID id, BOOL b_inv);
void idec_set_vs_inv(IDE_ID id, BOOL b_inv);
void idec_set_hvld_inv(IDE_ID id, BOOL b_inv);
void idec_set_vvld_inv(IDE_ID id, BOOL b_inv);
void idec_set_clk_inv(IDE_ID id, BOOL b_inv);
void idec_set_fld_inv(IDE_ID id, BOOL b_inv);
void idec_set_de_inv(IDE_ID id, BOOL b_inv);
void idec_set_rgbd(IDE_ID id, BOOL b_en);
void idec_set_through(IDE_ID id, BOOL b_en);
void idec_set_hdmi_ddr(IDE_ID id, BOOL b_en);
void idec_set_dram_out_format(IDE_ID id, BOOL is_yuv422);
void idec_set_rgbd_swap(IDE_ID id, BOOL b_en);


#if 0
void idec_set_reset(IDE_ID id);
#endif
void idec_set_dmach_dis(IDE_ID id, BOOL b_en);
void idec_set_en(IDE_ID id, BOOL b_en);
void idec_set_load(IDE_ID id);
void idec_set_dev_no(IDE_ID id, BOOL b_no);
void idec_set_clk1_2(IDE_ID id, BOOL b_en);
void idec_set_hlpf_en(IDE_ID id, BOOL b_en);
BOOL idec_get_hlpf_en(IDE_ID id);

BOOL idec_get_dmach_dis(IDE_ID id);
BOOL idec_get_dmach_idle(IDE_ID id);
BOOL idec_get_en(IDE_ID id);
BOOL idec_get_load(IDE_ID id);
BOOL idec_get_through(IDE_ID id);
BOOL idec_get_rgbd(IDE_ID id);
IDE_DEVICE_TYPE idec_get_device(IDE_ID id);

UINT32 idec_get_window_en(IDE_ID id);
void idec_set_all_window_dis(IDE_ID id);
void idec_set_all_window_en(IDE_ID id, UINT32 ui_wins);

// Scaling control
//void idec_setDimCvert(void);
//void idec_setCvertFactor(UINT32 uiHFT, UINT32 uiVFT);

// Palette Control
void idec_set_pal_entry(IDE_ID id, UINT32 ui_entry, UINT8 ui_color_y, UINT8 ui_color_cb, UINT8 ui_color_cr, UINT8 ui_alpha);
void idec_get_pal_entry(IDE_ID id, UINT32 ui_entry, UINT8 *ui_color_y, UINT8 *ui_color_cb, UINT8 *ui_color_cr, UINT8 *ui_alpha);
void idec_set_palette_group(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);
void idec_get_palette_group(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);
//void idec_get_shadow_palette_group(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);
void idec_set_palette_group_a_cr_cb_y(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);
void idec_get_palette_group_a_cr_cb_y(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);
void idec_set_pal(IDE_ID id, UINT32 ui_entry, UINT32 ui_palette);
void idec_get_pal(IDE_ID id, UINT32 ui_entry, UINT32 *ui_palette);

// Misc
void idec_set_background(IDE_ID id, UINT8 ui_color_y, UINT8 ui_color_cb, UINT8 ui_color_cr);
void idec_get_background(IDE_ID id, UINT8 *ui_color_y, UINT8 *ui_color_cb, UINT8 *ui_color_cr);



// Timing
void idec_set_hor_timing(IDE_ID id, UINT32 ui_hsynct, UINT32 ui_htotal, UINT32 ui_hvalidst, UINT32 ui_hvalided);
void idec_set_ver_timing(IDE_ID id, UINT32 ui_vsynct, UINT32 ui_vtotal, UINT32 ui_odd_vvalidst, UINT32 ui_odd_vvalided, UINT32 ui_even_vvalidst, UINT32 ui_even_vvalided);
void idec_get_hor_timing(IDE_ID id, UINT32 *pui_hsynct, UINT32 *pui_htotal, UINT32 *pui_hvalidst, UINT32 *pui_hvalided);
void idec_get_ver_timing(IDE_ID id, UINT32 *pui_vsynct, UINT32 *pui_vtotal, UINT32 *pui_odd_vvalidst, UINT32 *pui_odd_vvalided, UINT32 *pui_even_vvalidst, UINT32 *pui_even_vvalided);
void idec_set_inter_load_mode(IDE_ID id, BOOL b2field);

void idec_set_sync_delay(IDE_ID id, UINT8 ui_hs_delay, UINT8 ui_vs_delay);
void idec_set_interlace(IDE_ID id, BOOL b_inter);
void idec_set_start_field(IDE_ID id, BOOL b_oddst);
BOOL idec_get_interlace(IDE_ID id);
BOOL idec_get_start_field(IDE_ID id);
BOOL idec_get_cur_field(IDE_ID id);
void idec_set_digital_timing(IDE_ID id, UINT32 ui_codd_blk_st, UINT32 ui_codd_blk_ed, UINT32 ui_ceven_blk_st, UINT32 ui_ceven_blk_ed, UINT32 ui_cfid_st, UINT32 ui_cfid_ed);

// CSB
void idec_fill_gamma(IDE_ID id, UINT8 *uipgamma);
void idec_get_gamma(IDE_ID id, UINT8 *uipgamma);
void idec_fill_rgb_gamma(IDE_ID id, UINT8 *uiprgamma, UINT8 *uipggamma, UINT8 *uipbgamma);
void idec_get_rgb_gamma(IDE_ID id, UINT8 *uiprgamma, UINT8 *uipggamma, UINT8 *uipbgamma);
void idec_set_ctrst(IDE_ID id, UINT32 ui_ctrst);
UINT32 idec_get_ctrst(IDE_ID id);
void idec_set_brt(IDE_ID id, UINT32 ui_brt);
INT8 idec_get_brt(IDE_ID id);
void idec_set_cmults(IDE_ID id, UINT32 ui_cmults);
UINT32 idec_get_cmults(IDE_ID id);

void idec_set_clamp(IDE_ID id,  IDE_YCCCLAMP clamp);
IDE_YCCCLAMP idec_get_clamp(IDE_ID id);
void idec_set_cex(IDE_ID id, BOOL b_cex);
BOOL idec_get_cex(IDE_ID id);
void idec_set_ycex(IDE_ID id, BOOL b_ycex);
BOOL idec_get_ycex(IDE_ID id);

void idec_set_icst0(IDE_ID id, BOOL b_en);
BOOL idec_get_icst0(IDE_ID id);
void idec_set_icst(IDE_ID id, BOOL b_en);
BOOL idec_get_icst(IDE_ID id);
void idec_set_cst1(IDE_ID id, BOOL b_en);
BOOL idec_get_cst1(IDE_ID id);


// color control
void idec_set_color_ctrl_en(IDE_ID id, BOOL b_en);
BOOL idec_get_color_ctrl_en(IDE_ID id);
void idec_set_color_comp_adj_en(IDE_ID id, BOOL b_en);
BOOL idec_get_color_comp_adj_en(IDE_ID id);
void idec_set_color_ctrl_hue_adj_en(IDE_ID id, BOOL b_en);
BOOL idec_get_color_ctrl_hue_adj_en(IDE_ID id);
void idec_set_color_comp_ycon_en(IDE_ID id, BOOL b_en);
BOOL idec_get_color_comp_ycon_en(IDE_ID id);
void idec_set_color_comp_ccon_en(IDE_ID id, BOOL b_en);
BOOL idec_get_color_comp_ccon_en(IDE_ID id);
void idec_set_color_ctrl_hue(IDE_ID id, UINT8 *hue24tbl);
void idec_get_color_ctrl_hue(IDE_ID id, UINT8 *hue24tbl);
void idec_set_color_ctrl_int(IDE_ID id, INT8 *intensity24tbl);
void idec_get_color_ctrl_int(IDE_ID id, INT8 *intensity24tbl);
void idec_set_color_ctrl_sat(IDE_ID id, INT8 *sat24tbl);
void idec_get_color_ctrl_sat(IDE_ID id, INT8 *sat24tbl);
void idec_set_color_ctrl_dds(IDE_ID id, UINT8 *dds8tbl);
void idec_get_color_ctrl_dds(IDE_ID id, UINT8 *dds8tbl);
void idec_set_color_ctrl_int_sat_ofs(IDE_ID id, INT8 int_ofs, INT8 sat_ofs);
void idec_get_color_ctrl_int_sat_ofs(IDE_ID id, INT8 *int_ofs, INT8 *sat_ofs);
void idec_set_color_comp_ycon(IDE_ID id, UINT8 cony);
void idec_get_color_comp_ycon(IDE_ID id, UINT8 *cony);
void idec_set_color_comp_ccon(IDE_ID id, UINT8 conc);
void idec_get_color_comp_ccon(IDE_ID id, UINT8 *conc);
void idec_set_color_comp_yofs(IDE_ID id, INT8 yofs);
void idec_get_color_comp_yofs(IDE_ID id, INT8 *yofs);
void idec_set_color_comp_cofs(IDE_ID id, UINT8 cbofs, UINT8 crofs);
void idec_get_color_comp_cofs(IDE_ID id, UINT8 *cbofs, UINT8 *crofs);

void idec_set_csb_en(IDE_ID id, BOOL b_en);
BOOL idec_get_csb_en(IDE_ID id);
void idec_set_gamma_en(IDE_ID id, BOOL b_en);
BOOL idec_get_gamma_en(IDE_ID id);
void idec_set_dither_vbits(IDE_ID id, IDE_DITHER_VBITS b_rsel, IDE_DITHER_VBITS b_gsel, IDE_DITHER_VBITS b_bsel);
void idec_set_out_comp(IDE_ID id, IDE_OUT_COMP ui_comp0, IDE_OUT_COMP ui_comp1, IDE_OUT_COMP ui_comp2, BOOL b_bit_swp, BOOL b_len);
void idec_get_out_comp(IDE_ID id, IDE_OUT_COMP *ui_comp0, IDE_OUT_COMP *ui_comp1, IDE_OUT_COMP *ui_comp2, BOOL *b_bit_swp, BOOL *b_len);

// Interrupt
UINT32 idec_get_interrupt_status(IDE_ID id);
void idec_clear_interrupt_status(IDE_ID id, UINT32 ui_int_status);
void idec_set_interrupt_en(IDE_ID id, UINT32 ui_int_en);
void idec_clr_interrupt_en(IDE_ID id, UINT32 ui_int);
UINT32 idec_get_interrupt_en(IDE_ID id);

// FCST
void idec_set_fcst_coef(IDE_ID id, UINT32 *ui_pcoef);
void idec_get_fcst_coef(IDE_ID id, UINT8 *cst0_coef0, UINT8 *cst0_coef1, UINT8 *cst0_coef2, UINT8 *cst0_coef3);

// ICST0
void idec_set_icst0_pre_offset(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
void idec_get_icst0_pre_offset(IDE_ID id, INT32 *ui_y, INT32 *ui_cb, INT32 *ui_cr);
void idec_set_icst0_coef(IDE_ID id, UINT32 *ui_pcoef);
void idec_get_icst0_coef(IDE_ID id, INT32 *ui_pcoef);
void idec_set_out_offset(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
void idec_get_out_offset(IDE_ID id, INT32 *ui_y, INT32 *ui_cb, INT32 *ui_cr);
void idec_set_out_limit(IDE_ID id, UINT8 ui_y_low, UINT8 ui_y_up, UINT8 ui_cb_low, UINT8 ui_cb_up, UINT8 ui_cr_low, UINT8 ui_cr_up);

// ICST
void idec_set_icst_coef(IDE_ID id, UINT32 *ui_pcoef);
void idec_get_icst_coef(IDE_ID id, INT32 *ui_pcoef);


// Alpha Blending
void idec_set_alpha_blending(IDE_ID id, IDE_BLEND_LAYER ui_layer, IDE_ALPHA_TYPE ui_alhpa_type, UINT8 ui_global_alpha);
void idec_get_alpha_blending(IDE_ID id, IDE_BLEND_LAYER ui_layer, IDE_ALPHA_TYPE *ui_alhpa_type, UINT8 *ui_global_alpha);


// Shawdow register
//void idec_set_o1_pal_shw_en(IDE_ID id, BOOL b_en);
void idec_set_lb_read_en(IDE_ID id, BOOL b_en);
//void idec_set_fd_exp_en(IDE_ID id, BOOL b_en);



// Sub-Pixel interpolation
void idec_set_subpixel(IDE_ID id, BOOL b_odd_line, BOOL b_r, BOOL b_g, BOOL b_b);
void idec_get_subpixel(IDE_ID id, BOOL b_odd_line, BOOL *b_r, BOOL *b_g, BOOL *b_b);

// FD
void idec_set_fd_all_en(IDE_ID id, UINT32 ui_en);
void idec_set_fd_all_dis(IDE_ID id, UINT32 ui_dis);
UINT32 idec_get_fd_all_en(IDE_ID id);
void idec_set_fd_en(IDE_ID id, IDE_FD_NUM ui_num);
void idec_set_fd_dis(IDE_ID id, IDE_FD_NUM ui_num);
void idec_set_fd_win_pos(IDE_ID id, IDE_FD_NUM ui_num, UINT32 ui_x, UINT32 ui_y);
void idec_get_fd_win_pos(IDE_ID id, IDE_FD_NUM ui_num, UINT32 *ui_x, UINT32 *ui_y);
void idec_set_fd_win_dim(IDE_ID id, IDE_FD_NUM ui_num, UINT32 ui_win_w, UINT32 ui_win_h);
void idec_get_fd_win_dim(IDE_ID id, IDE_FD_NUM ui_num, UINT32 *ui_win_w, UINT32 *ui_win_h);
void idec_set_fd_win_bord(IDE_ID id, IDE_FD_NUM ui_num, UINT32 ui_bor_h, UINT32 ui_bor_v);
void idec_get_fd_win_bord(IDE_ID id, IDE_FD_NUM ui_num, UINT32 *ui_bor_h, UINT32 *ui_bor_v);
void idec_set_fd_color(IDE_ID id, IDE_FD_NUM ui_num, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_get_fd_color(IDE_ID id, IDE_FD_NUM ui_num, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_line_color(IDE_ID id, IDE_LINE_NUM ui_num, UINT8 *ui_a, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_line_para(IDE_ID id, IDE_LINE_NUM ui_num, UINT32 sub_line, BOOL *sign_a, BOOL *sign_b, UINT32 *a, UINT32 *b, UINT32 *slope, UINT32 *compare);
void idec_set_line_all_dis(IDE_ID id, UINT32 ui_dis);
UINT32 idec_get_line_all_en(IDE_ID id);
void idec_set_line_en(IDE_ID id, IDE_LINE_NUM ui_num);
void idec_set_line_dis(IDE_ID id, IDE_LINE_NUM ui_num);
ER idec_set_line_source_alpha(IDE_ID id, IDE_LINE_NUM ui_num, UINT8 source_alpha);


ER idec_draw_line(IDE_ID id, IDE_LINE_NUM ui_num, UINT32 ui_x1, UINT32 ui_y1, UINT32 ui_x2, UINT32 ui_y2, UINT32 ui_boarder, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);


//-------------------------------------------------
// ide Video register programm APIs
//-------------------------------------------------

// Video control
void idec_set_v1_en(IDE_ID id, BOOL b_en);
void idec_set_v2_en(IDE_ID id, BOOL b_en);
BOOL idec_get_v1_en(IDE_ID id);
BOOL idec_get_v2_en(IDE_ID id);

// Video buffer
void idec_set_v1_buf0_addr(IDE_ID id, UINT32 ui_y_addr, UINT32 ui_cb_addr, UINT32 ui_cr_addr);
void idec_set_v1_buf1_addr(IDE_ID id, UINT32 ui_y_addr, UINT32 ui_cb_addr, UINT32 ui_cr_addr);
void idec_set_v1_buf2_addr(IDE_ID id, UINT32 ui_y_addr, UINT32 ui_cb_addr, UINT32 ui_cr_addr);
void idec_set_v1_buf0_odd(IDE_ID id, BOOL b_odd);
void idec_set_v1_buf1_odd(IDE_ID id, BOOL b_odd);
void idec_set_v1_buf2_odd(IDE_ID id, BOOL b_odd);
void idec_set_v1_buf_dim(IDE_ID id, UINT32 ui_bw, UINT32 ui_bh, UINT32 ui_lineoffset);
void idec_set_v1_read_order(IDE_ID id, IDE_HOR_READ b_l2r, IDE_VER_READ b_t2b);
void idec_set_v1_buf_op(IDE_ID id, IDE_BJMODE ui_bjmode, IDE_OP_BUF ui_optbuf, IDE_BUF_NUM ui_buf_num);
void idec_ch_v1_buf(IDE_ID id, IDE_OP_BUF ui_optbuf);
//void idec_setV1Cst(IDE_ID id, BOOL b_en, CST_SEL b_sel);
void idec_set_v1_src(IDE_ID id, IDE_SYNC_SRC b_sel);

void idec_set_v2_buf0_addr(IDE_ID id, UINT32 ui_y_addr, UINT32 ui_cb_addr, UINT32 ui_cr_addr);
void idec_set_v2_buf1_addr(IDE_ID id, UINT32 ui_y_addr, UINT32 ui_cb_addr, UINT32 ui_cr_addr);
void idec_set_v2_buf2_addr(IDE_ID id, UINT32 ui_y_addr, UINT32 ui_cb_addr, UINT32 ui_cr_addr);
void idec_set_v2_buf0_odd(IDE_ID id, BOOL b_odd);
void idec_set_v2_buf1_odd(IDE_ID id, BOOL b_odd);
void idec_set_v2_buf2_odd(IDE_ID id, BOOL b_odd);
void idec_set_v2_buf_dim(IDE_ID id, UINT32 ui_bw, UINT32 ui_bh, UINT32 ui_lineoffset);
void idec_set_v2_read_order(IDE_ID id, IDE_HOR_READ b_l2r, IDE_VER_READ b_t2b);
void idec_set_v2_buf_op(IDE_ID id, IDE_BJMODE ui_bjmode, IDE_OP_BUF ui_optbuf, IDE_BUF_NUM ui_buf_num);
void idec_ch_v2_buf(IDE_ID id, IDE_OP_BUF ui_optbuf);
//void idec_setV2Cst(IDE_ID id, BOOL b_en, CST_SEL b_sel);
void idec_set_v2_src(IDE_ID id, IDE_SYNC_SRC b_sel);

void idec_get_v1_buf0_addr(IDE_ID id, UINT32 *pui_y_addr, UINT32 *pui_cb_addr, UINT32 *pui_cr_addr);
void idec_get_v1_buf1_addr(IDE_ID id, UINT32 *pui_y_addr, UINT32 *pui_cb_addr, UINT32 *pui_cr_addr);
void idec_get_v1_buf2_addr(IDE_ID id, UINT32 *pui_y_addr, UINT32 *pui_cb_addr, UINT32 *pui_cr_addr);
void idec_get_v1_buf0_odd(IDE_ID id, BOOL *b_odd);
void idec_get_v1_buf1_odd(IDE_ID id, BOOL *b_odd);
void idec_get_v1_buf2_odd(IDE_ID id, BOOL *b_odd);
void idec_get_v1_buf_dim(IDE_ID id, UINT32 *pui_bw, UINT32 *pui_bh, UINT32 *pui_lineoffset);
void idec_get_v1_buf_op(IDE_ID id, IDE_BJMODE *pui_bjmode, IDE_OP_BUF *pui_optbuf, IDE_BUF_NUM *pui_bufnum);
void idec_get_v1_read_order(IDE_ID id, IDE_HOR_READ *pb_l2r, IDE_VER_READ *pb_t2b);

void idec_get_v2_buf0_addr(IDE_ID id, UINT32 *pui_y_addr, UINT32 *pui_cb_addr, UINT32 *pui_cr_addr);
void idec_get_v2_buf1_addr(IDE_ID id, UINT32 *pui_y_addr, UINT32 *pui_cb_addr, UINT32 *pui_cr_addr);
void idec_get_v2_buf2_addr(IDE_ID id, UINT32 *pui_y_addr, UINT32 *pui_cb_addr, UINT32 *pui_cr_addr);
void idec_get_v2_buf0_odd(IDE_ID id, BOOL *b_odd);
void idec_get_v2_buf1_odd(IDE_ID id, BOOL *b_odd);
void idec_get_v2_buf2_odd(IDE_ID id, BOOL *b_odd);
void idec_get_v2_buf_dim(IDE_ID id, UINT32 *pui_bw, UINT32 *pui_bh, UINT32 *pui_lineoffset);
void idec_get_v2_buf_op(IDE_ID id, IDE_BJMODE *pui_bjmode, IDE_OP_BUF *pui_optbuf, IDE_BUF_NUM *pui_bufnum);
void idec_get_v2_read_order(IDE_ID id, IDE_HOR_READ *pb_l2r, IDE_VER_READ *pb_t2b);

// Video Window
void idec_set_v1_scale_ctrl(IDE_ID id, BOOL b_hscaleup, BOOL b_vscaleup);
void idec_set_v1_scale_factor(IDE_ID id, UINT32 ui_hsf, BOOL b_sub, UINT32 ui_vsf, BOOL b_vsub);
void idec_set_v1_vsf_init(IDE_ID id, UINT32 ui_init0, UINT32 ui_init1);
void idec_set_v1_win_dim(IDE_ID id, UINT32 ui_win_w, UINT32 ui_win_h);
void idec_set_v1_win_pos(IDE_ID id, UINT32 ui_x, UINT32 ui_y);
void idec_set_v1_fmt(IDE_ID id, IDE_COLOR_FORMAT ui_fmt);
void idec_set_v2_hsm(IDE_ID id, IDE_SCALE_METHOD hsm);

void idec_set_v2_scale_ctrl(IDE_ID id, BOOL b_hscaleup, BOOL b_vscaleup);
void idec_set_v2_scale_factor(IDE_ID id, UINT32 ui_hsf, BOOL b_sub, UINT32 ui_vsf, BOOL b_vsub);
void idec_set_v2_vsf_init(IDE_ID id, UINT32 ui_init0, UINT32 ui_init1);
void idec_set_v1_hsm(IDE_ID id, IDE_SCALE_METHOD hsm);
void idec_set_v2_win_dim(IDE_ID id, UINT32 ui_win_w, UINT32 ui_win_h);
void idec_set_v2_win_pos(IDE_ID id, UINT32 ui_x, UINT32 ui_y);
void idec_set_v2_fmt(IDE_ID id, IDE_COLOR_FORMAT ui_fmt);

void idec_get_v1_scale_factor(IDE_ID id, UINT32 *ui_hsf, BOOL *b_sub, UINT32 *ui_vsf, BOOL *b_vsub);
void idec_get_v1_win_dim(IDE_ID id, UINT32 *ui_win_w, UINT32 *ui_win_h);
void idec_get_v1_win_pos(IDE_ID id, UINT32 *ui_x, UINT32 *ui_y);
void idec_get_v1_fmt(IDE_ID id, IDE_COLOR_FORMAT *ui_fmt);

void idec_get_v2_scale_factor(IDE_ID id, UINT32 *ui_hsf, BOOL *b_sub, UINT32 *ui_vsf, BOOL *b_vsub);
void idec_get_v2_win_dim(IDE_ID id, UINT32 *ui_win_w, UINT32 *ui_win_h);
void idec_get_v2_win_pos(IDE_ID id, UINT32 *ui_x, UINT32 *ui_y);
void idec_get_v2_fmt(IDE_ID id, IDE_COLOR_FORMAT *ui_fmt);

// Video color key and Blending
void idec_set_video_colorkey(IDE_ID id, UINT8 ui_ck_y, UINT8 ui_ck_cb, UINT8 ui_ck_cr);
//void ide_setVideoBlendOp(IDE_VIDEO_BLEND_OP uiOp);
void idec_set_video_colorkey_sel(IDE_ID id, IDE_VIDEO_COLORKEY_SEL b_sel);
void idec_set_video_colorkey_op(IDE_ID id, IDE_VIDEO_COLORKEY_OP ui_ck_op);

void idec_get_video_colorkey(IDE_ID id, UINT8 *ui_ck_y, UINT8 *ui_ck_cb, UINT8 *ui_ck_cr);
//void ide_getVideoBlendOp(IDE_ID id, IDE_VIDEO_BLEND_OP *uiOp);
void idec_get_video_colorkey_sel(IDE_ID id, IDE_VIDEO_COLORKEY_SEL *b_sel);
void idec_get_video_colorkey_op(IDE_ID id, IDE_VIDEO_COLORKEY_OP *ui_ck_op);
void idec_set_v1_lowpass_en(IDE_ID id, BOOL b_en);
BOOL idec_get_v1_lowpass_en(IDE_ID id);
void idec_set_v1_lowpass_coef(IDE_ID id, UINT8 *ui_pcoef);
void idec_get_v1_lowpass_coef(IDE_ID id, UINT8 *ui_pcoef);

// Video color Space Transform
//void idec_setV1CstPreOffset(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setV1CstPostOffst(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setV1CstCoef(IDE_ID id, ACCRCY_SEL b_accuracy, UINT32 *ui_pcoef);

//void idec_setV2CstPreOffset(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setV2CstPostOffst(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setV2CstCoef(IDE_ID id, ACCRCY_SEL b_accuracy, UINT32 *ui_pcoef);


// Video Auto Blinking
void idec_set_v1_blink(IDE_ID id, BOOL b_ovr, BOOL b_und, BOOL b_sel);
void idec_set_v1_count(IDE_ID id, UINT8 ui_cnt);
void idec_set_v1_ovrexp_threshold(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v1_ovrexp_color(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v1_undexp_threshold(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v1_undexp_color(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v2_blink(IDE_ID id, BOOL b_ovr, BOOL b_und, BOOL b_sel);
void idec_set_v2_count(IDE_ID id, UINT8 ui_cnt);
void idec_set_v2_ovrexp_threshold(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v2_ovrexp_color(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v2_undexp_threshold(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void idec_set_v2_undexp_color(IDE_ID id, UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
UINT8 idec_get_v1_count(IDE_ID id);
UINT8 idec_get_v2_count(IDE_ID id);
void idec_get_v1_ovrexp_threshold(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v2_ovrexp_threshold(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v1_undexp_threshold(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v2_undexp_threshold(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v1_ovrexp_color(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v2_ovrexp_color(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v1_undexp_color(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_get_v2_undexp_color(IDE_ID id, UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);
void idec_set_v2_lowpass_en(IDE_ID id, BOOL b_en);
BOOL idec_get_v2_lowpass_en(IDE_ID id);
void idec_set_v2_lowpass_coef(IDE_ID id, UINT8 *ui_pcoef);
void idec_get_v2_lowpass_coef(IDE_ID id, UINT8 *ui_pcoef);


//-------------------------------------------------
// ide OSD register programm APIs
//-------------------------------------------------

// OSD control
void idec_set_o1_en(IDE_ID id, BOOL b_en);
//void idec_setO2En(IDE_ID id, BOOL b_en);
BOOL idec_get_o1_en(IDE_ID id);
//BOOL idec_getO2En(IDE_ID id);

// OSD buffer
void idec_set_o1_buf_addr(IDE_ID id, UINT32 ui_addr);
void idec_set_o1_buf_alpha_addr(IDE_ID id, UINT32 ui_addr);
void idec_set_o1_buf_dim(IDE_ID id, UINT32 ui_bw, UINT32 ui_bh, UINT32 ui_lineoffset);
void idec_set_o1_read_order(IDE_ID id, IDE_HOR_READ b_l2r, IDE_VER_READ b_t2b);
//void ide_setO1Cst(IDE_ID id, BOOL b_en, CST_SEL b_sel);

#if 0
void idec_set_o2_buf_addr(IDE_ID id, UINT32 ui_addr);
void idec_set_o2_buf_dim(IDE_ID id, UINT32 ui_bw, UINT32 ui_bh, UINT32 ui_lineoffset);
void idec_set_o2_read_order(IDE_ID id, IDE_HOR_READ b_l2r, IDE_VER_READ b_t2b);
//void ide_setO2Cst(IDE_ID id, BOOL b_en, CST_SEL b_sel);
#endif

void idec_get_o1_buf_addr(IDE_ID id, UINT32 *ui_addr);
void idec_get_o1_alpha_buf_addr(IDE_ID id, UINT32 *ui_addr);
void idec_get_o1_buf_dim(IDE_ID id, UINT32 *pui_bw, UINT32 *pui_bh, UINT32 *pui_lineoffset);
void idec_get_o1_read_order(IDE_ID id, IDE_HOR_READ *pb_l2r, IDE_VER_READ *pb_t2b);

#if 0
void idec_get_o2_buf_addr(IDE_ID id, UINT32 *ui_addr);
void idec_get_o2_buf_dim(IDE_ID id, UINT32 *pui_bw, UINT32 *pui_bh, UINT32 *pui_lineoffset);
void idec_get_o2_read_order(IDE_ID id, IDE_HOR_READ *pb_l2r, IDE_VER_READ *pb_t2b);
#endif

// OSD Window
void idec_set_o1_scale_ctrl(IDE_ID id, BOOL b_hscaleup, BOOL b_vscaleup);
void idec_set_o1_scale_factor(IDE_ID id, UINT32 ui_hsf, BOOL b_sub, UINT32 ui_vsf, BOOL b_vsub);
void idec_set_o1_vsf_init(IDE_ID id, UINT32 ui_init0, UINT32 ui_init1);
void idec_set_o1_hsm(IDE_ID id, IDE_SCALE_METHOD hsm);
void idec_set_o1_vsm(IDE_ID id, IDE_SCALE_METHOD vsm);
void idec_set_o1_win_dim(IDE_ID id, UINT32 ui_win_w, UINT32 ui_win_h);
void idec_set_o1_win_pos(IDE_ID id, UINT32 ui_x, UINT32 ui_y);
void idec_set_o1_palette_sel(IDE_ID id, IDE_PALETTE_SEL ui_psel);
void idec_set_o1_palette_high_addr(IDE_ID id, UINT8 ui_hi_addr);
void idec_set_o1_fmt(IDE_ID id, IDE_COLOR_FORMAT ui_fmt);

#if 0
void idec_set_o2_scale_ctrl(IDE_ID id, BOOL b_hscaleup, BOOL b_vscaleup);
void idec_set_o2_scale_factor(IDE_ID id, UINT32 ui_hsf, BOOL b_sub, UINT32 ui_vsf, BOOL b_vsub);
void idec_set_o2_vsf_init(IDE_ID id, UINT32 ui_init0, UINT32 ui_init1);
void idec_set_o2_hsm(IDE_ID id, IDE_SCALE_METHOD hsm);
void idec_set_o2_win_dim(IDE_ID id, UINT32 ui_win_w, UINT32 ui_win_h);
void idec_set_o2_win_pos(IDE_ID id, UINT32 ui_x, UINT32 ui_y);
void idec_set_o2_palette_sel(IDE_ID id, IDE_PALETTE_SEL ui_psel);
void idec_set_o2_palette_high_addr(IDE_ID id, UINT8 ui_hi_addr);
void idec_set_o2_fmt(IDE_ID id, IDE_COLOR_FORMAT ui_fmt);
#endif

void idec_get_o1_scale_factor(IDE_ID id, UINT32 *ui_hsf, BOOL *b_sub, UINT32 *ui_vsf, BOOL *b_vsub);
void idec_get_o1_win_dim(IDE_ID id, UINT32 *ui_win_w, UINT32 *ui_win_h);
void idec_get_o1_win_pos(IDE_ID id, UINT32 *ui_x, UINT32 *ui_y);
void idec_get_o1_palette_sel(IDE_ID id, IDE_PALETTE_SEL *ui_psel);
void idec_get_o1_palette_high_addr(IDE_ID id, UINT8 *ui_hi_addr);
void idec_get_o1_fmt(IDE_ID id, IDE_COLOR_FORMAT *ui_fmt);

#if 0
void idec_get_o2_scale_factor(IDE_ID id, UINT32 *ui_hsf, BOOL *b_sub, UINT32 *ui_vsf, BOOL *b_vsub);
void idec_get_o2_win_dim(IDE_ID id, UINT32 *ui_win_w, UINT32 *ui_win_h);
void idec_get_o2_win_pos(IDE_ID id, UINT32 *ui_x, UINT32 *ui_y);
void idec_get_o2_palette_sel(IDE_ID id, IDE_PALETTE_SEL *ui_psel);
void idec_get_o2_palette_high_addr(IDE_ID id, UINT8 *ui_hi_addr);
void idec_get_o2_fmt(IDE_ID id, IDE_COLOR_FORMAT *ui_fmt);
#endif

// OSD color key and Blending
void idec_set_osd_colorkey_en(IDE_ID id, BOOL b_en);
void idec_set_osd_colorkey(IDE_ID id, UINT8 ui_ck);
//void ide_setOsdBlendOp(IDE_ID id, IDE_OSD_BLEND_OP uiOp);

void idec_get_osd_colorkey(IDE_ID id, UINT8 *ui_ck);
void idec_get_osd_colorkey_en(IDE_ID id, BOOL *b_en);
//void idec_getOsdBlendOp(IDE_ID id, IDE_OSD_BLEND_OP *uiOp);

void idec_set_osd1_colorkey_en(IDE_ID id, BOOL b_en);
void idec_get_osd1_colorkey_en(IDE_ID id, BOOL *b_en);
void idec_set_osd1_colorkey_op(IDE_ID id, IDE_OSD_COLORKEY_OP ck_op);
void idec_get_osd1_colorkey_op(IDE_ID id, IDE_OSD_COLORKEY_OP *ck_op);
void idec_set_osd1_colorkey(IDE_ID id, UINT8 ui_key_r, UINT8 ui_key_g, UINT8 ui_key_b, UINT8 alpha);
void idec_get_osd1_colorkey(IDE_ID id, UINT8 *ui_key_r, UINT8 *ui_key_g, UINT8 *ui_key_b, UINT8 *alpha);
#if 0
void idec_set_osd2_color_key_en(IDE_ID id, BOOL b_en);
void idec_get_osd2_color_key_en(IDE_ID id, BOOL *b_en);
void idec_set_osd2_color_key_op(IDE_ID id, IDE_OSD_COLORKEY_OP ck_op);
void idec_get_osd2_color_key_op(IDE_ID id, IDE_OSD_COLORKEY_OP *ck_op);
void idec_set_osd2_color_key(IDE_ID id, UINT8 ui_key_y, UINT8 ui_key_cb, UINT8 ui_key_cr);
void idec_get_osd2_color_key(IDE_ID id, UINT8 *ui_key_y, UINT8 *ui_key_cb, UINT8 *ui_key_cr);

void idec_set_osd_layer_swap(IDE_ID id, BOOL b_swap);
BOOL idec_get_osd_layer_swap(IDE_ID id);
#endif


// OSD color Space Transform
//void idec_setO1CstPreOffset(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setO1CstPostOffst(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setO1CstCoef(IDE_ID id, ACCRCY_SEL b_accuracy, UINT32 *ui_pcoef);

//void idec_setO2CstPreOffset(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setO2CstPostOffst(IDE_ID id, UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void idec_setO2CstCoef(IDE_ID id, ACCRCY_SEL b_accuracy, UINT32 *ui_pcoef);
void idec_set_o1_lowpass_en(IDE_ID id, BOOL b_en);
BOOL idec_get_o1_lowpass_en(IDE_ID id);
void idec_set_o1_lowpass_coef(IDE_ID id, UINT8 *ui_pcoef);
void idec_get_o1_lowpass_coef(IDE_ID id, UINT8 *ui_pcoef);

#if 0
void idec_set_o2_low_pass_en(IDE_ID id, BOOL b_en);
BOOL idec_get_o2_low_pass_en(IDE_ID id);
void idec_set_o2_low_pass_coef(IDE_ID id, UINT8 *ui_pcoef);
void idec_get_o2_low_pass_coef(IDE_ID id, UINT8 *ui_pcoef);
#endif

//-------------------------------------------------
// ide Public Functions
//-------------------------------------------------

BOOL idec_set_video_buf_addr(IDE_ID id, IDE_VIDEOID video_id, VIDEO_BUF_ADDR *pv_buf_addr);
BOOL idec_get_video_buf_addr(IDE_ID id, IDE_VIDEOID video_id, VIDEO_BUF_ADDR *pv_buf_addr);
BOOL idec_set_video_buf_attr(IDE_ID id, IDE_VIDEOID video_id, VIDEO_BUF_ATTR *pv_buf_attr);
BOOL idec_get_video_buf_attr(IDE_ID id, IDE_VIDEOID video_id, VIDEO_BUF_ATTR *pv_buf_attr);
BOOL idec_set_video_win_attr_ex(IDE_ID id, IDE_VIDEOID video_id, VOSD_WINDOW_ATTR *pv_win_attr, BOOL b_load);
BOOL idec_set_video_win_attr(IDE_ID id, IDE_VIDEOID video_id, VOSD_WINDOW_ATTR *pv_win_attr);
void idec_set_video_vsf_init(IDE_ID id, IDE_VIDEOID video_id, UINT32 ui_init0, UINT32 ui_init1);

BOOL idec_set_osd_buf_addr(IDE_ID id, IDE_OSDID osd_id, UINT32 ui_osd_addr);
BOOL idec_get_osd_buf_addr(IDE_ID id, IDE_OSDID osd_id, UINT32 *pui_osd_addr);
BOOL idec_set_osd_buf_attr(IDE_ID id, IDE_OSDID osd_id, OSD_BUF_ATTR *p_osd_buf_attr);
BOOL idec_get_osd_buf_attr(IDE_ID id, IDE_OSDID osd_id, OSD_BUF_ATTR *p_osd_buf_attr);
BOOL idec_set_osd_win_attr_ex(IDE_ID id, IDE_OSDID osd_id, VOSD_WINDOW_ATTR *p_osd_win_attr, BOOL b_load);
BOOL idec_set_osd_win_attr(IDE_ID id, IDE_OSDID osd_id, VOSD_WINDOW_ATTR *p_osd_win_attr);
void idec_set_osd_vsf_init(IDE_ID id, IDE_OSDID osd_id, UINT32 ui_init0, UINT32 ui_init1);

void idec_set_palette_entry(IDE_ID id, PALETTE_ENTRY *p_palette_entry);
void idec_get_palette_entry(IDE_ID id, PALETTE_ENTRY *p_palette_entry);
void idec_set_palette_group(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);
void idec_get_palette_group(IDE_ID id, UINT32 ui_start, UINT32 ui_number, UINT32 *p_palette_entry);

void idec_wait_frame_end(IDE_ID id, BOOL wait);
void idec_wait_yuv_output_done(IDE_ID id);


void idec_disable_video(IDE_ID id, IDE_VIDEOID video_id);
void idec_enable_video(IDE_ID id, IDE_VIDEOID video_id);
BOOL idec_get_video_enable(IDE_ID id, IDE_VIDEOID video_id);

void idec_disable_osd(IDE_ID id, IDE_OSDID osd_id);
void idec_enable_osd(IDE_ID id, IDE_OSDID osd_id);
BOOL idec_get_osd_enable(IDE_ID id, IDE_OSDID osd_id);
IDE_PAL_CAP idec_get_pal_capability(IDE_ID id);

void idec_set_video_buffer_content(IDE_ID id, IDE_VIDEOID video_id, IDE_BUFFERID buffer_id, PYUVCOLOR p_ycbcr);
void idec_convert_rgb2ycbcrr(IDE_ID id, PRGBCOLOR p_rgb, PYUVCOLOR p_ycbcr);

//void idec_configVideoCst(IDE_VIDEOID video_id, BOOL b_en, CST_SEL SEL);
//void idec_configOsdCst(IDE_OSDID osd_id, BOOL b_en, CST_SEL SEL);
//void idec_config_icst(BOOL b_en, CST_SEL SEL);
void idec_config_icst(IDE_ID id, BOOL b_en, CST_SEL SEL);
void idec_getconfig_icst(IDE_ID id, BOOL *b_en, CST_SEL *SEL);
void idec_config_output_limit(IDE_ID id, BOOL b_sel);
void idec_set_constant_window_offset(IDE_ID id, UINT32 ui_x, UINT32 ui_y);
void idec_get_constant_window_offset(IDE_ID id, UINT32 *ui_x, UINT32 *ui_y);


//-------------------------------------------------
// ide register programm APIs
//-------------------------------------------------

// ide control
#define ide_set_dithering(b_en, b_freerun)       idec_set_dithering(IDE_ID_1, b_en, b_freerun)
#define ide_set_device(uidevice)                 idec_set_device(IDE_ID_1, uidevice)
#define ide_set_pdir(b_pdir)                     idec_set_pdir(IDE_ID_1, b_pdir)
#define ide_set_odd(ui_odd)                       idec_set_odd(IDE_ID_1, ui_odd)
#define ide_set_even(ui_even)                     idec_set_even(IDE_ID_1, ui_even)
#define ide_set_hs_inv(b_inv)                     idec_set_hs_inv(IDE_ID_1, b_inv)
#define ide_set_vs_inv(b_inv)                     idec_set_vs_inv(IDE_ID_1, b_inv)
#define ide_set_hvld_inv(b_inv)                   idec_set_hvld_inv(IDE_ID_1, b_inv)
#define ide_set_vvld_inv(b_inv)                   idec_set_vvld_inv(IDE_ID_1, b_inv)
#define ide_set_clk_inv(b_inv)                    idec_set_clk_inv(IDE_ID_1, b_inv)
#define ide_set_fld_inv(b_inv)                    idec_set_fld_inv(IDE_ID_1, b_inv)
#define ide_set_de_inv(b_inv)                     idec_set_de_inv(IDE_ID_1, b_inv)
#define ide_set_rgbd(b_en)                       idec_set_rgbd(IDE_ID_1, b_en)
#define ide_set_through(b_en)                    idec_set_through(IDE_ID_1, b_en)
#define ide_set_hdmi_ddr(b_en)                    idec_set_hdmi_ddr(IDE_ID_1, b_en)
#define ide_set_reset()                          //idec_set_reset(IDE_ID_1)
#define ide_set_dmach_dis(b_en)                 idec_set_dmach_dis(IDE_ID_1, b_en)
#define ide_set_en(b_en)                         idec_set_en(IDE_ID_1, b_en)
#define ide_set_load()                           idec_set_load(IDE_ID_1)
#define ide_set_dev_no(b_no)                      idec_set_dev_no(IDE_ID_1, b_no)
#define ide_set_hlpf_en(b_en)                     idec_set_hlpf_en(IDE_ID_1, b_en)
#define ide_get_hlpf_en()                         idec_get_hlpf_en(IDE_ID_1)

#define ide_get_dmach_dis()                     idec_get_dmach_dis(IDE_ID_1)
#define ide_get_dmach_idle()                    idec_get_dmach_idle(IDE_ID_1)
#define ide_get_en()                             idec_get_en(IDE_ID_1)
#define ide_get_load()                           idec_get_load(IDE_ID_1)
#define ide_get_through()                        idec_get_through(IDE_ID_1)
#define ide_get_rgbd()                           idec_get_rgbd(IDE_ID_1)
#define ide_get_device()                         idec_get_device(IDE_ID_1)

#define ide_get_window_en()                       idec_get_window_en(IDE_ID_1)
#define ide_set_all_window_dis()                   idec_set_all_window_dis(IDE_ID_1)
#define ide_set_all_window_en(ui_wins)              idec_set_all_window_en(IDE_ID_1, ui_wins)

// Palette Control
#define ide_set_pal_entry(ui_entry, ui_color_y, ui_color_cb, ui_color_cr, ui_alpha)     idec_set_pal_entry(IDE_ID_1, ui_entry, ui_color_y, ui_color_cb, ui_color_cr, ui_alpha)
#define ide_get_pal_entry(ui_entry, ui_color_y, ui_color_cb, ui_color_cr, ui_alpha)     idec_get_pal_entry(IDE_ID_1, ui_entry, ui_color_y, ui_color_cb, ui_color_cr, ui_alpha)
#define ide_set_pal(ui_entry, ui_palette)          idec_set_pal(IDE_ID_1, ui_entry, ui_palette)
#define ide_get_pal(ui_entry, ui_palette)          idec_get_pal(IDE_ID_1, ui_entry, ui_palette)

// Misc
#define ide_set_background(ui_color_y, ui_color_cb, ui_color_cr)    idec_set_background(IDE_ID_1, ui_color_y, ui_color_cb, ui_color_cr)
#define ide_get_background(ui_color_y, ui_color_cb, ui_color_cr)    idec_get_background(IDE_ID_1, ui_color_y, ui_color_cb, ui_color_cr)


// Timing
#define ide_set_hor_timing(ui_hsynct, ui_htotal, ui_hvalidst, ui_hvalided)      idec_set_hor_timing(IDE_ID_1, ui_hsynct, ui_htotal, ui_hvalidst, ui_hvalided)
#define ide_set_ver_timing(ui_vsynct, ui_vtotal, ui_odd_vvalidst, ui_odd_vvalided, ui_even_vvalidst, ui_even_vvalided)         idec_set_ver_timing(IDE_ID_1, ui_vsynct, ui_vtotal, ui_odd_vvalidst, ui_odd_vvalided, ui_even_vvalidst, ui_even_vvalided)
#define ide_get_hor_timing(pui_hsynct, pui_htotal, pui_hvalidst, pui_hvalided)  idec_get_hor_timing(IDE_ID_1, pui_hsynct, pui_htotal, pui_hvalidst, pui_hvalided)
#define ide_get_ver_timing(pui_vsynct, pui_vtotal, pui_odd_vvalidst, pui_odd_vvalided, pui_even_vvalidst, pui_even_vvalided)   idec_get_ver_timing(IDE_ID_1, pui_vsynct, pui_vtotal, pui_odd_vvalidst, pui_odd_vvalided, pui_even_vvalidst, pui_even_vvalided)

#define ide_set_sync_delay(ui_hs_delay, ui_vs_delay)  idec_set_sync_delay(IDE_ID_1, ui_hs_delay, ui_vs_delay)
#define ide_set_interlace(b_inter)               idec_set_interlace(IDE_ID_1, b_inter)
#define ide_set_start_field(b_oddst)              idec_set_start_field(IDE_ID_1, b_oddst)
#define ide_get_interlace()                      idec_get_interlace(IDE_ID_1)
#define ide_get_start_field()                     idec_get_start_field(IDE_ID_1)
#define ide_get_cur_field()                       idec_get_cur_field(IDE_ID_1)
#define ide_set_digital_timing(ui_codd_blk_st, ui_codd_blk_ed, ui_ceven_blk_st, ui_ceven_blk_ed, ui_cfid_st, ui_cfid_ed)             idec_set_digital_timing(IDE_ID_1, ui_codd_blk_st, ui_codd_blk_ed, ui_ceven_blk_st, ui_ceven_blk_ed, ui_cfid_st, ui_cfid_ed)

// CSB
#define ide_fill_gamma(uipgamma)                 idec_fill_gamma(IDE_ID_1, uipgamma)
#define ide_get_gamma(uipgamma)                  idec_get_gamma(IDE_ID_1, uipgamma)
#define ide_set_ctrst(ui_ctrst)                   idec_set_ctrst(IDE_ID_1, ui_ctrst)
#define ide_get_ctrst()                          idec_get_ctrst(IDE_ID_1)
#define ide_set_brt(ui_brt)                       idec_set_brt(IDE_ID_1, ui_brt)
#define ide_get_brt()                            idec_get_brt(IDE_ID_1)
#define ide_set_cmults(ui_cmults)                 idec_set_cmults(IDE_ID_1, ui_cmults)
#define ide_get_cmults()                         idec_get_cmults(IDE_ID_1)
#define ide_set_cex(b_cex)                       idec_set_cex(IDE_ID_1, b_cex)
#define ide_set_ycex(b_ycex)                     idec_set_ycex(IDE_ID_1, b_ycex)
#define ide_get_ycex()                           idec_get_ycex(IDE_ID_1)

#define ide_set_icst0(b_en)                      idec_set_icst0(IDE_ID_1, b_en)
#define ide_get_icst0()                          idec_get_icst0(IDE_ID_1)
#define ide_set_icst(b_en)                       idec_set_icst(IDE_ID_1, b_en)
#define ide_get_icst()                           idec_get_icst(IDE_ID_1)
#define ide_set_cst1(b_en)                       idec_set_cst1(IDE_ID_1, b_en)
#define ide_get_cst1()                           idec_get_cst1(IDE_ID_1)

#define ide_set_csb_en(b_en)                      idec_set_csb_en(IDE_ID_1, b_en)
#define ide_get_csb_en()                          idec_get_csb_en(IDE_ID_1)
#define ide_set_gamma_en(b_en)                    idec_set_gamma_en(IDE_ID_1, b_en)
#define ide_get_gamma_en()                        idec_get_gamma_en(IDE_ID_1)
#define ide_set_dither_vbits(b_rsel, b_gsel, b_bsel)    idec_set_dither_vbits(IDE_ID_1, b_rsel, b_gsel, b_bsel)
#define ide_set_out_comp(ui_comp0, ui_comp1, ui_comp2, b_bit_swp, b_len)    idec_set_out_comp(IDE_ID_1, ui_comp0, ui_comp1, ui_comp2, b_bit_swp, b_len)
#define ide_get_out_comp(ui_comp0, ui_comp1, ui_comp2, b_bit_swp, b_len)    idec_get_out_comp(IDE_ID_1, ui_comp0, ui_comp1, ui_comp2, b_bit_swp, b_len)

// Interrupt
#define ide_get_interrupt_status()                idec_get_interrupt_status(IDE_ID_1)
#define ide_clear_interrupt_status(ui_int_status)   idec_clear_interrupt_status(IDE_ID_1, ui_int_status)
#define ide_set_interrupt_en(ui_int_en)             idec_set_interrupt_en(IDE_ID_1, ui_int_en)
#define ide_clr_interrupt_en(ui_int)               idec_clr_interrupt_en(IDE_ID_1, ui_int)

// color Space Transform
#define ide_set_fcst_coef(ui_pcoef)                idec_set_fcst_coef(IDE_ID_1, ui_pcoef)

// ICST0
#define ide_set_icst0_pre_offset(ui_y, ui_cb, ui_cr)  idec_set_icst0_pre_offset(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_icst0_pre_offset(ui_y, ui_cb, ui_cr)  idec_get_icst0_pre_offset(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_icst0_coef(ui_pcoef)               idec_set_icst0_coef(IDE_ID_1, ui_pcoef)
#define ide_get_icst0_coef(ui_pcoef)               idec_get_icst0_coef(IDE_ID_1, ui_pcoef)
#define ide_set_out_offset(ui_y, ui_cb, ui_cr)       idec_set_out_offset(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_out_offset(ui_y, ui_cb, ui_cr)       idec_get_out_offset(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_out_limit(ui_y_low, ui_y_up, ui_cb_low, ui_cb_up, ui_cr_low, ui_cr_up)    idec_set_out_limit(IDE_ID_1, ui_y_low, ui_y_up, ui_cb_low, ui_cb_up, ui_cr_low, ui_cr_up)

// ICST
#define ide_set_icst_coef(ui_pcoef)                idec_set_icst_coef(IDE_ID_1, ui_pcoef)
#define ide_get_icst_coef(ui_pcoef)                idec_get_icst_coef(IDE_ID_1, ui_pcoef)


// Alpha Blending
#define ide_set_alpha_blending(ui_layer, ui_alhpa_type, ui_global_alpha)    idec_set_alpha_blending(IDE_ID_1, ui_layer, ui_alhpa_type, ui_global_alpha)

// Sub-Pixel interpolation
#define ide_set_subpixel(b_odd_line, b_r, b_g, b_b)   idec_set_subpixel(IDE_ID_1, b_odd_line, b_r, b_g, b_b)
#define ide_get_subpixel(b_odd_line, b_r, b_g, b_b)   idec_get_subpixel(IDE_ID_1, b_odd_line, b_r, b_g, b_b)

//-------------------------------------------------
// ide Video register programm APIs
//-------------------------------------------------

// Video control
#define ide_set_v1_en(b_en)                       idec_set_v1_en(IDE_ID_1, b_en)
#define ide_set_v2_en(b_en)                       idec_set_v2_en(IDE_ID_1, b_en)
#define ide_get_v1_en()                           idec_get_v1_en(IDE_ID_1)
#define ide_get_v2_en()                           idec_get_v2_en(IDE_ID_1)

// Video buffer
#define ide_set_v1_buf0_addr(ui_y_addr, ui_cb_addr, ui_cr_addr)    idec_set_v1_buf0_addr(IDE_ID_1, ui_y_addr, ui_cb_addr, ui_cr_addr)
#define ide_set_v1_buf1_addr(ui_y_addr, ui_cb_addr, ui_cr_addr)    idec_set_v1_buf1_addr(IDE_ID_1, ui_y_addr, ui_cb_addr, ui_cr_addr)
#define ide_set_v1_buf2_addr(ui_y_addr, ui_cb_addr, ui_cr_addr)    idec_set_v1_buf2_addr(IDE_ID_1, ui_y_addr, ui_cb_addr, ui_cr_addr)
#define ide_set_v1_buf0_odd(b_odd)                 idec_set_v1_buf0_odd(IDE_ID_1, b_odd)
#define ide_set_v1_buf1_odd(b_odd)                 idec_set_v1_buf1_odd(IDE_ID_1, b_odd)
#define ide_set_v1_buf2_odd(b_odd)                 idec_set_v1_buf2_odd(IDE_ID_1, b_odd)
#define ide_set_v1_buf_dim(ui_bw, ui_bh, ui_lineoffset)         idec_set_v1_buf_dim(IDE_ID_1, ui_bw, ui_bh, ui_lineoffset)
#define ide_set_v1_read_order(b_l2r, b_t2b)        idec_set_v1_read_order(IDE_ID_1, b_l2r, b_t2b)
#define ide_set_v1_buf_op(ui_bjmode, ui_optbuf, ui_buf_num)      idec_set_v1_buf_op(IDE_ID_1, ui_bjmode, ui_optbuf, ui_buf_num)
#define ide_ch_v1_buf(ui_optbuf)                   idec_ch_v1_buf(IDE_ID_1, ui_optbuf)
#define ide_set_v1_src(b_sel)                     idec_set_v1_src(IDE_ID_1, b_sel)

#define ide_set_v2_buf0_addr(ui_y_addr, ui_cb_addr, ui_cr_addr)    idec_set_v2_buf0_addr(IDE_ID_1, ui_y_addr, ui_cb_addr, ui_cr_addr)
#define ide_set_v2_buf1_addr(ui_y_addr, ui_cb_addr, ui_cr_addr)    idec_set_v2_buf1_addr(IDE_ID_1, ui_y_addr, ui_cb_addr, ui_cr_addr)
#define ide_set_v2_buf2_addr(ui_y_addr, ui_cb_addr, ui_cr_addr)    idec_set_v2_buf2_addr(IDE_ID_1, ui_y_addr, ui_cb_addr, ui_cr_addr)
#define ide_set_v2_buf0_odd(b_odd)                 idec_set_v2_buf0_odd(IDE_ID_1, b_odd)
#define ide_set_v2_buf1_odd(b_odd)                 idec_set_v2_buf1_odd(IDE_ID_1, b_odd)
#define ide_set_v2_buf2_odd(b_odd)                 idec_set_v2_buf2_odd(IDE_ID_1, b_odd)
#define ide_set_v2_buf_dim(ui_bw, ui_bh, ui_lineoffset)         idec_set_v2_buf_dim(IDE_ID_1, ui_bw, ui_bh, ui_lineoffset)
#define ide_set_v2_read_order(b_l2r, b_t2b)        idec_set_v2_read_order(IDE_ID_1, b_l2r, b_t2b)
#define ide_set_v2_buf_op(ui_bjmode, ui_optbuf, ui_buf_num)      idec_set_v2_buf_op(IDE_ID_1, ui_bjmode, ui_optbuf, ui_buf_num)
#define ide_ch_v2_buf(ui_optbuf)                   idec_ch_v2_buf(IDE_ID_1, ui_optbuf)
#define ide_set_v2_src(b_sel)                     idec_set_v2_src(IDE_ID_1, b_sel)

#define ide_get_v1_buf0_addr(pui_y_addr, pui_cb_addr, pui_cr_addr) idec_get_v1_buf0_addr(IDE_ID_1, pui_y_addr, pui_cb_addr, pui_cr_addr)
#define ide_get_v1_buf1_addr(pui_y_addr, pui_cb_addr, pui_cr_addr) idec_get_v1_buf1_addr(IDE_ID_1, pui_y_addr, pui_cb_addr, pui_cr_addr)
#define ide_get_v1_buf2_addr(pui_y_addr, pui_cb_addr, pui_cr_addr) idec_get_v1_buf2_addr(IDE_ID_1, pui_y_addr, pui_cb_addr, pui_cr_addr)
#define ide_get_v1_buf0_odd(b_odd)                 idec_get_v1_buf0_odd(IDE_ID_1, b_odd)
#define ide_get_v1_buf1_odd(b_odd)                 idec_get_v1_buf1_odd(IDE_ID_1, b_odd)
#define ide_get_v1_buf2_odd(b_odd)                 idec_get_v1_buf2_odd(IDE_ID_1, b_odd)
#define ide_get_v1_buf_dim(pui_bw, pui_bh, pui_lineoffset)      idec_get_v1_buf_dim(IDE_ID_1, pui_bw, pui_bh, pui_lineoffset)
#define ide_get_v1_buf_op(pui_bjmode, pui_optbuf, pui_bufnum)   idec_get_v1_buf_op(IDE_ID_1, pui_bjmode, pui_optbuf, pui_bufnum)
#define ide_get_v1_read_order(pb_l2r, pb_t2b)      idec_get_v1_read_order(IDE_ID_1, pb_l2r, pb_t2b)

#define ide_get_v2_buf0_addr(pui_y_addr, pui_cb_addr, pui_cr_addr) idec_get_v2_buf0_addr(IDE_ID_1, pui_y_addr, pui_cb_addr, pui_cr_addr)
#define ide_get_v2_buf1_addr(pui_y_addr, pui_cb_addr, pui_cr_addr) idec_get_v2_buf1_addr(IDE_ID_1, pui_y_addr, pui_cb_addr, pui_cr_addr)
#define ide_get_v2_buf2_addr(pui_y_addr, pui_cb_addr, pui_cr_addr) idec_get_v2_buf2_addr(IDE_ID_1, pui_y_addr, pui_cb_addr, pui_cr_addr)
#define ide_get_v2_buf0_odd(b_odd)                 idec_get_v2_buf0_odd(IDE_ID_1, b_odd)
#define ide_get_v2_buf1_odd(b_odd)                 idec_get_v2_buf1_odd(IDE_ID_1, b_odd)
#define ide_get_v2_buf2_odd(b_odd)                 idec_get_v2_buf2_odd(IDE_ID_1, b_odd)
#define ide_get_v2_buf_dim(pui_bw, pui_bh, pui_lineoffset)      idec_get_v2_buf_dim(IDE_ID_1, pui_bw, pui_bh, pui_lineoffset)
#define ide_get_v2_buf_op(pui_bjmode, pui_optbuf, pui_bufnum)   idec_get_v2_buf_op(IDE_ID_1, pui_bjmode, pui_optbuf, pui_bufnum)
#define ide_get_v2_read_order(pb_l2r, pb_t2b)      idec_get_v2_read_order(IDE_ID_1, pb_l2r, pb_t2b)

// Video Window
#define ide_set_v1_scale_ctrl(b_hscaleup, b_vscaleup)        idec_set_v1_scale_ctrl(IDE_ID_1, b_hscaleup, b_vscaleup)
#define ide_set_v1_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_set_v1_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_set_v1_vsf_init(ui_init0, ui_init1)      idec_set_v1_vsf_init(IDE_ID_1, ui_init0, ui_init1)
#define ide_set_v1_win_dim(ui_win_w, ui_win_h)         idec_set_v1_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_set_v1_win_pos(ui_x, ui_y)               idec_set_v1_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_set_v1_fmt(ui_fmt)                     idec_set_v1_fmt(IDE_ID_1, ui_fmt)
#define ide_set_v2_hsm(hsm)                       idec_set_v2_hsm(IDE_ID_1, hsm)

#define ide_set_v2_scale_ctrl(b_hscaleup, b_vscaleup)        idec_set_v2_scale_ctrl(IDE_ID_1, b_hscaleup, b_vscaleup)
#define ide_set_v2_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_set_v2_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_set_v2_vsf_init(ui_init0, ui_init1)                idec_set_v2_vsf_init(IDE_ID_1, ui_init0, ui_init1)
#define ide_set_v1_hsm(hsm)                                 idec_set_v1_hsm(IDE_ID_1, hsm)
#define ide_set_v2_win_dim(ui_win_w, ui_win_h)                   idec_set_v2_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_set_v2_win_pos(ui_x, ui_y)                         idec_set_v2_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_set_v2_fmt(ui_fmt)                               idec_set_v2_fmt(IDE_ID_1, ui_fmt)

#define ide_get_v1_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_get_v1_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_get_v1_win_dim(ui_win_w, ui_win_h)                   idec_get_v1_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_get_v1_win_pos(ui_x, ui_y)                         idec_get_v1_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_get_v1_fmt(ui_fmt)                               idec_get_v1_fmt(IDE_ID_1, ui_fmt)

#define ide_get_v2_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_get_v2_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_get_v2_win_dim(ui_win_w, ui_win_h)                   idec_get_v2_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_get_v2_win_pos(ui_x, ui_y)                         idec_get_v2_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_get_v2_fmt(ui_fmt)                               idec_get_v2_fmt(IDE_ID_1, ui_fmt)

// Video color key and Blending
#define ide_set_video_colorkey(ui_ck_y, ui_ck_cb, ui_ck_cr)       idec_set_video_colorkey(IDE_ID_1, ui_ck_y, ui_ck_cb, ui_ck_cr)
#define ide_set_video_colorkey_sel(b_sel)                    idec_set_video_colorkey_sel(IDE_ID_1, b_sel)
#define ide_set_video_colorkey_op(ui_ck_op)                    idec_set_video_colorkey_op(IDE_ID_1, ui_ck_op)

#define ide_get_video_colorkey(ui_ck_y, ui_ck_cb, ui_ck_cr)       idec_get_video_colorkey(IDE_ID_1, ui_ck_y, ui_ck_cb, ui_ck_cr)
#define ide_get_video_colorkey_sel(b_sel)                    idec_get_video_colorkey_sel(IDE_ID_1, b_sel)
#define ide_get_video_colorkey_op(ui_ck_op)                    idec_get_video_colorkey_op(IDE_ID_1, ui_ck_op)


// Video Auto Blinking
#define ide_set_v1_blink(b_ovr, b_und, b_sel)         idec_set_v1_blink(IDE_ID_1, b_ovr, b_und, b_sel)
#define ide_set_v1_count(ui_cnt)                       idec_set_v1_count(IDE_ID_1, ui_cnt)
#define ide_set_v1_ovrexp_threshold(ui_y, ui_cb, ui_cr)   idec_set_v1_ovrexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v1_ovrexp_color(ui_y, ui_cb, ui_cr)       idec_set_v1_ovrexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v1_undexp_threshold(ui_y, ui_cb, ui_cr)   idec_set_v1_undexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v1_undexp_color(ui_y, ui_cb, ui_cr)       idec_set_v1_undexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v2_blink(b_ovr, b_und, b_sel)         idec_set_v2_blink(IDE_ID_1, b_ovr, b_und, b_sel)
#define ide_set_v2_count(ui_cnt)                       idec_set_v2_count(IDE_ID_1, ui_cnt)
#define ide_set_v2_ovrexp_threshold(ui_y, ui_cb, ui_cr)   idec_set_v2_ovrexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v2_ovrexp_color(ui_y, ui_cb, ui_cr)       idec_set_v2_ovrexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v2_undexp_threshold(ui_y, ui_cb, ui_cr)   idec_set_v2_undexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_set_v2_undexp_color(ui_y, ui_cb, ui_cr)       idec_set_v2_undexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v1_count()                            idec_get_v1_count(IDE_ID_1)
#define ide_get_v2_count()                            idec_get_v2_count(IDE_ID_1)
#define ide_get_v1_ovrexp_threshold(ui_y, ui_cb, ui_cr)   idec_get_v1_ovrexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v2_ovrexp_threshold(ui_y, ui_cb, ui_cr)   idec_get_v2_ovrexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v1_undexp_threshold(ui_y, ui_cb, ui_cr)   idec_get_v1_undexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v2_undexp_threshold(ui_y, ui_cb, ui_cr)   idec_get_v2_undexp_threshold(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v1_ovrexp_color(ui_y, ui_cb, ui_cr)       idec_get_v1_ovrexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v2_ovrexp_color(ui_y, ui_cb, ui_cr)       idec_get_v2_ovrexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v1_undexp_color(ui_y, ui_cb, ui_cr)       idec_get_v1_undexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)
#define ide_get_v2_undexp_color(ui_y, ui_cb, ui_cr)       idec_get_v2_undexp_color(IDE_ID_1, ui_y, ui_cb, ui_cr)

//-------------------------------------------------
// ide OSD register programm APIs
//-------------------------------------------------

// OSD control
#define ide_set_o1_en(b_en)                           idec_set_o1_en(IDE_ID_1, b_en)
//#define ide_set_o2_en(b_en)                           idec_setO2En(IDE_ID_1, b_en)
#define ide_get_o1_en()                               idec_get_o1_en(IDE_ID_1)
//#define ide_get_o2_en()                               idec_getO2En(IDE_ID_1)

// OSD buffer
#define ide_set_o1_buf_addr(ui_addr)                    idec_set_o1_buf_addr(IDE_ID_1, ui_addr)
#define ide_set_o1_buf_alpha_addr(ui_addr)               idec_set_o1_buf_alpha_addr(IDE_ID_1, ui_addr)
#define ide_set_o1_buf_dim(ui_bw, ui_bh, ui_lineoffset)   idec_set_o1_buf_dim(IDE_ID_1, ui_bw, ui_bh, ui_lineoffset)
#define ide_set_o1_read_order(b_l2r, b_t2b)            idec_set_o1_read_order(IDE_ID_1, b_l2r, b_t2b)

#if 0
#define ide_set_o2_buf_addr(ui_addr)                    idec_set_o2_buf_addr(IDE_ID_1, ui_addr)
#define ide_set_o2_buf_dim(ui_bw, ui_bh, ui_lineoffset)   idec_set_o2_buf_dim(IDE_ID_1, ui_bw, ui_bh, ui_lineoffset)
#define ide_set_o2_read_order(b_l2r, b_t2b)            idec_set_o2_read_order(IDE_ID_1, b_l2r, b_t2b)
#endif

#define ide_get_o1_buf_addr(ui_addr)                    idec_get_o1_buf_addr(IDE_ID_1, ui_addr)
#define ide_get_o1_alpha_buf_addr(ui_addr)               idec_get_o1_alpha_buf_addr(IDE_ID_1, ui_addr)
#define ide_get_o1_buf_dim(pui_bw, pui_bh, pui_lineoffset)    idec_get_o1_buf_dim(IDE_ID_1, pui_bw, pui_bh, pui_lineoffset)
#define ide_get_o1_read_order(pb_l2r, pb_t2b)          idec_get_o1_read_order(IDE_ID_1, pb_l2r, pb_t2b)

#if 0
#define ide_get_o2_buf_addr(ui_addr)                    idec_get_o2_buf_addr(IDE_ID_1, ui_addr)
#define ide_get_o2_buf_dim(pui_bw, pui_bh, pui_lineoffset)    idec_get_o2_buf_dim(IDE_ID_1, pui_bw, pui_bh, pui_lineoffset)
#define ide_get_o2_read_order(pb_l2r, pb_t2b)          idec_get_o2_read_order(IDE_ID_1, pb_l2r, pb_t2b)
#endif

// OSD Window
#define ide_set_o1_scale_ctrl(b_hscaleup, b_vscaleup)        idec_set_o1_scale_ctrl(IDE_ID_1, b_hscaleup, b_vscaleup)
#define ide_set_o1_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_set_o1_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_set_o1_vsf_init(ui_init0, ui_init1)                idec_set_o1_vsf_init(IDE_ID_1, ui_init0, ui_init1)
#define ide_set_o1_hsm(hsm)                                 idec_set_o1_hsm(IDE_ID_1, hsm)
#define ide_set_o1_win_dim(ui_win_w, ui_win_h)                   idec_set_o1_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_set_o1_win_pos(ui_x, ui_y)                         idec_set_o1_win_pos(IDE_ID_1, ui_x, ui_y)
//Not use since NT96650 => removed
//#define ide_setO1PaletteSel(ui_psel)                     idec_set_o1_palette_sel(IDE_ID_1, ui_psel)
#define ide_set_o1_palette_high_addr(ui_hi_addr)                idec_set_o1_palette_high_addr(IDE_ID_1, ui_hi_addr)
#define ide_set_o1_fmt(ui_fmt)                               idec_set_o1_fmt(IDE_ID_1, ui_fmt)

#if 0
#define ide_set_o2_scale_ctrl(b_hscaleup, b_vscaleup)        idec_set_o2_scale_ctrl(IDE_ID_1, b_hscaleup, b_vscaleup)
#define ide_set_o2_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_set_o2_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_set_o2_vsf_init(ui_init0, ui_init1)                idec_set_o2_vsf_init(IDE_ID_1, ui_init0, ui_init1)
#define ide_set_o2_win_dim(ui_win_w, ui_win_h)                   idec_set_o2_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_set_o2_win_pos(ui_x, ui_y)                         idec_set_o2_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_set_o2_palette_sel(ui_psel)                       idec_set_o2_palette_sel(IDE_ID_1, ui_psel)
#define ide_set_o2_palette_high_addr(ui_hi_addr)                idec_set_o2_palette_high_addr(IDE_ID_1, ui_hi_addr)
#define ide_set_o2_fmt(ui_fmt)                               idec_set_o2_fmt(IDE_ID_1, ui_fmt)
#define ide_set_osd_layer_swap(b_swap)                        idec_set_osd_layer_swap(IDE_ID_1, b_swap)
#endif
//#define ide_setFDLineLayerSwap(b_swap)                        idec_set_fd_line_layer_swap(IDE_ID_1,b_swap)

#define ide_get_o1_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_get_o1_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_get_o1_win_dim(ui_win_w, ui_win_h)                   idec_get_o1_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_get_o1_win_pos(ui_x, ui_y)                         idec_get_o1_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_get_o1_palette_sel(ui_psel)                       idec_get_o1_palette_sel(IDE_ID_1, ui_psel)
#define ide_get_o1_palette_high_addr(ui_hi_addr)                idec_get_o1_palette_high_addr(IDE_ID_1, ui_hi_addr)
#define ide_get_o1_fmt(ui_fmt)                               idec_get_o1_fmt(IDE_ID_1, ui_fmt)

#if 0
#define ide_get_o2_scale_factor(ui_hsf, b_sub, ui_vsf, b_vsub) idec_get_o2_scale_factor(IDE_ID_1, ui_hsf, b_sub, ui_vsf, b_vsub)
#define ide_get_o2_win_dim(ui_win_w, ui_win_h)                   idec_get_o2_win_dim(IDE_ID_1, ui_win_w, ui_win_h)
#define ide_get_o2_win_pos(ui_x, ui_y)                         idec_get_o2_win_pos(IDE_ID_1, ui_x, ui_y)
#define ide_get_o2_palette_sel(ui_psel)                       idec_get_o2_palette_sel(IDE_ID_1, ui_psel)
#define ide_get_o2_palette_high_addr(ui_hi_addr)                idec_get_o2_palette_high_addr(IDE_ID_1, ui_hi_addr)
#define ide_get_o2_fmt(ui_fmt)                               idec_get_o2_fmt(IDE_ID_1, ui_fmt)
#define ide_get_osd_layer_swap()                             idec_get_osd_layer_swap(IDE_ID_1)
#endif

// OSD color key and Blending
#define ide_set_osd_colorkey_en(b_en)                        idec_set_osd_colorkey_en(IDE_ID_1, b_en)
#define ide_set_osd_colorkey(ui_ck)                          ide_set_osd_colorkey(IDE_ID_1, ui_ck)
//void ide_setOsdBlendOp(IDE_OSD_BLEND_OP uiOp);

#define ide_get_osd_colorkey(ui_ck)                          idec_get_osd_colorkey(IDE_ID_1, ui_ck)
#define ide_get_osd_colorkey_en(b_en)                        idec_get_osd_colorkey_en(IDE_ID_1, b_en)
//void ide_getOsdBlendOp(IDE_OSD_BLEND_OP *uiOp);

// OSD color Space Transform
//void ide_setO1CstPreOffset(UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void ide_setO1CstPostOffst(UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void ide_setO1CstCoef(ACCRCY_SEL b_accuracy, UINT32 *ui_pcoef);

//void ide_setO2CstPreOffset(UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void ide_setO2CstPostOffst(UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void ide_setO2CstCoef(ACCRCY_SEL b_accuracy, UINT32 *ui_pcoef);

#if 0
//-------------------------------------------------
// TV Encoder register programm APIs
//-------------------------------------------------
//*These api will be remove begin --> add idetv_set_config to config
ER   tv_open(void);
ER   tv_close(void);
BOOL tv_is_opened(void);
void ide_set_tv_use_post_icst(BOOL b_sel);
void ide_set_tv_use_post_gamma(BOOL b_sel);
void ide_set_tv_out_offset(UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
void ide_set_tv_out_limit(UINT8 ui_y_low, UINT8 ui_y_up, UINT8 ui_cb_low, UINT8 ui_cb_up, UINT8 ui_cr_low, UINT8 ui_cr_up);

void ide_set_tv_mode(IDE_TV_MODE ui_tvmode);
void ide_set_tv_test_mode(BOOL b_en);
void ide_set_tv_test_mode_sel(UINT8 uisel);
void ide_set_tv_test_mode_fix_value(UINT32 ui_val);
void ide_set_tv_slave_mode(BOOL b_en);
void ide_set_tv_sacle_swap(BOOL b_en);
void ide_set_tv_data_swap(BOOL b_en);
void ide_set_tv_sin_cos_swap(BOOL b_en);
void ide_set_tv_search_en(BOOL b_en);
void ide_set_tv_ckpn_pol(BOOL b_sel);
void ide_set_tv_burst(UINT8 ui_burst);
void ide_set_tv_bll(UINT8 ui_bll);
void ide_set_tv_brl(UINT8 ui_brl);
void ide_set_tv_ntsc_setup_level(UINT8 ui_ntsc_setup);
void ide_set_tv_src(IDE_TV_SRC src);
void ide_set_tv_scale(UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void ide_set_tv_power_down(BOOL b_pd);
BOOL ide_get_tv_power_down(void);
void ide_set_tv_sela(UINT8 ui_sela);
void ide_set_tv_low_level(UINT32 ui_level);
void ide_set_tv_trim(UINT8 ui_trim);
void ide_get_tv_trim(UINT8 *ui_trim);

UINT8 ide_get_tv_bll(void);
UINT8 ide_get_tv_brl(void);
UINT8 ide_get_tv_ntsc_setup_level(void);
void ide_get_tv_scale(UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);

void ide_set_tv_bypass_lpf(BOOL b_en);
BOOL ide_get_tv_bypass_lpf(void);
void ide_set_tv_bypass_notch(BOOL b_en);
BOOL ide_get_tv_bypass_notch(void);
void ide_set_tv_chroma_filter(BOOL b_prag, UINT8 *ui_taps);
void ide_set_tv_y_filter(BOOL b_prag, UINT8 *ui_taps);

//*These api will be remove end --> add idetv_set_config to config
void   idetv_set_config(IDETV_CONFIG_ID config_id, UINT32 ui_config);
UINT32 idetv_get_config(IDETV_CONFIG_ID config_id);
void   idetv_set_coef(IDETV_CONFIGCOEF_ID config_id, UINT8 *ui_coef);
void   idetv_get_coef(IDETV_CONFIGCOEF_ID config_id, UINT8 *ui_coef);
#endif


//-------------------------------------------------
// ide Public Functions
//-------------------------------------------------

#define ide_set_video_buf_addr(video_id, pv_buf_addr)          idec_set_video_buf_addr(IDE_ID_1, video_id, pv_buf_addr)
#define ide_get_video_buf_addr(video_id, pv_buf_addr)          idec_get_video_buf_addr(IDE_ID_1, video_id, pv_buf_addr)
#define ide_set_video_buf_attr(video_id, pv_buf_attr)          idec_set_video_buf_attr(IDE_ID_1, video_id, pv_buf_attr)
#define ide_get_video_buf_attr(video_id, pv_buf_attr)          idec_get_video_buf_attr(IDE_ID_1, video_id, pv_buf_attr)
#define ide_set_video_win_attr_ex(video_id, pv_win_attr, b_load) idec_set_video_win_attr_ex(IDE_ID_1, video_id, pv_win_attr, b_load)
#define ide_set_video_win_attr(video_id, pv_win_attr)          idec_set_video_win_attr(IDE_ID_1, video_id, pv_win_attr)
#define ide_set_video_vsf_init(video_id, ui_init0, ui_init1)     idec_set_video_vsf_init(IDE_ID_1, video_id, ui_init0, ui_init1)

#define ide_set_osd_buf_addr(osd_id, ui_osd_addr)               idec_set_osd_buf_addr(IDE_ID_1, osd_id, ui_osd_addr)
#define ide_get_osd_buf_addr(osd_id, pui_osd_addr)              idec_get_osd_buf_addr(IDE_ID_1, osd_id, pui_osd_addr)
#define ide_set_osd_buf_attr(osd_id, p_osd_buf_attr)            idec_set_osd_buf_attr(IDE_ID_1, osd_id, p_osd_buf_attr)
#define ide_get_osd_buf_attr(osd_id, p_osd_buf_attr)            idec_get_osd_buf_attr(IDE_ID_1, osd_id, p_osd_buf_attr)
#define ide_set_osd_win_attr_ex(osd_id, p_osd_win_attr, b_load)   idec_set_osd_win_attr_ex(IDE_ID_1, osd_id, p_osd_win_attr, b_load)
#define ide_set_osd_win_attr(osd_id, p_osd_win_attr)            idec_set_osd_win_attr(IDE_ID_1, osd_id, p_osd_win_attr)
#define ide_set_osd_vsf_init(osd_id, ui_init0, ui_init1)         idec_set_osd_vsf_init(IDE_ID_1, osd_id, ui_init0, ui_init1)

#define ide_set_palette_entry(p_palette_entry)                idec_set_palette_entry(IDE_ID_1, p_palette_entry)
#define ide_get_palette_entry(p_palette_entry)                idec_get_palette_entry(IDE_ID_1, p_palette_entry)
#define ide_set_palette_group(ui_start, ui_number, p_palette_entry)        idec_set_palette_group(IDE_ID_1, ui_start, ui_number, p_palette_entry)
#define ide_get_palette_group(ui_start, ui_number, p_palette_entry)        idec_get_palette_group(IDE_ID_1, ui_start, ui_number, p_palette_entry)
#define ide_get_shadow_palette_group(ui_start, ui_number, p_palette_entry) //idec_get_shadow_palette_group(IDE_ID_1, ui_start, ui_number, p_palette_entry)
#define ide_set_palette_group_a_cr_cb_y(ui_start, ui_number, p_palette_entry)  idec_set_palette_group_a_cr_cb_y(IDE_ID_1, ui_start, ui_number, p_palette_entry)
#define ide_get_palette_group_a_cr_cb_y(ui_start, ui_number, p_palette_entry)  idec_get_palette_group_a_cr_cb_y(IDE_ID_1, ui_start, ui_number, p_palette_entry)
#define ide_wait_frame_end(wait)                                 idec_wait_frame_end(IDE_ID_1, wait)
#define ide_wait_yuv_output_done()                            idec_wait_yuv_output_done(IDE_ID_1)

#define ide_disable_video(video_id)                          idec_disable_video(IDE_ID_1, video_id)
#define ide_enable_video(video_id)                           idec_enable_video(IDE_ID_1, video_id)
#define ide_get_video_enable(video_id)                        idec_get_video_enable(IDE_ID_1, video_id)

#define ide_disable_osd(osd_id)                              idec_disable_osd(IDE_ID_1, osd_id)
#define ide_enable_osd(osd_id)                               idec_enable_osd(IDE_ID_1, osd_id)
#define ide_get_osd_enable(osd_id)                            idec_get_osd_enable(IDE_ID_1, osd_id)
#define ide_get_pal_capability()                             idec_get_pal_capability(IDE_ID_1)

#define ide_set_video_buffer_content(video_id, buffer_id, p_ycbcr)         idec_set_video_buffer_content(IDE_ID_1, video_id, buffer_id, p_ycbcr)
#define ide_convert_rgb2ycbcr(p_rgb, p_ycbcr)                 idec_convert_rgb2ycbcrr(IDE_ID_1, p_rgb, p_ycbcr)

//void ide_configVideoCst(IDE_VIDEOID video_id, BOOL b_en, CST_SEL SEL);
//void ide_configOsdCst(IDE_OSDID osd_id, BOOL b_en, CST_SEL SEL);
//void ide_config_icst(BOOL b_en, CST_SEL SEL);
#define ide_config_icst(b_en, SEL)                          idec_config_icst(IDE_ID_1, b_en, SEL)
#define ide_getconfig_icst(b_en, SEL)                       idec_getconfig_icst(IDE_ID_1, b_en, SEL)
#define ide_config_output_limit(b_sel)                       idec_config_output_limit(IDE_ID_1, b_sel)
#define ide_set_constant_window_offset(ui_x, ui_y)              idec_set_constant_window_offset(IDE_ID_1, ui_x, ui_y)
#define ide_get_constant_window_offset(ui_x, ui_y)              idec_get_constant_window_offset(IDE_ID_1, ui_x, ui_y)


//-------------------------------------------------
// ide OS Level Functions
//-------------------------------------------------

//ID ide_getLockStatus(void);
#if 0
void ide_attach(void);
void ide_detach(void);
void ide_init(void);
#endif
#define ide_open()          idec_open(IDE_ID_1)
#define ide_close()         idec_close(IDE_ID_1)
#define ide_is_opened()      idec_is_opened(IDE_ID_1)
//void ide_registerMiInterruptHandler(fp_ide_interrupt_callback IntrHdl);
#define IDE1_OSD2_EXIST     0
//@}

ER ide_platform_set_clksrc(IDE_ID id, UINT32 src);
void ide_platform_set_freq(IDE_ID id, UINT32 freq);
void ide_platform_set_iffreq(IDE_ID id, UINT32 freq);
UINT32 ide_platform_get_freq(IDE_ID id);
UINT32 ide_platform_get_iffreq(IDE_ID id);

void ide_platform_clk_en(IDE_ID id);
void ide_platform_clk_dis(IDE_ID id);



//#include "ide_compat.h"

#endif //_IDE_H

