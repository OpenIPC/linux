/**
    @file       disp_lyr.h
    @ingroup    mISYSDisp

    @brief      Header file for Display layer control
				This file is the header file that define the API and data type
				for Display control object
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

/** \addtogroup  mISYSDisp*/
//@{

#ifndef _DISPLYR_H
#define _DISPLYR_H

#include "dispcomn.h"
#include "ide.h"


/**
    @name Display Layer Control Group

    These definitions are used for display layer control API (DISP_OBJ.disp_lyr_ctrl) as the layer control portal.
    The layer characteristics such as buffer format/ width / height/ Window-width / window-height / (...etc) are controlled by this API.
    The user must know that the API would not set load to activate the settings.
    The User would programs all the settings needed and then use DISP_OBJ.load() to activate all the settings.
*/
//@{

/**
    Display Layer Buffer Format

    This definition is used for the OP Code: "DISPLAYER_OP_SET_MODE" in the disp_lyr_ctrl API to specify the buffer format.
    This feature can be assigned for each display layer with different setting.
*/
typedef enum {
	DISPBUFFORMAT_PAL1,                 ///< Buffer uses format 1bit palette.
	DISPBUFFORMAT_PAL2,                 ///< Buffer uses format 2bit palette.
	DISPBUFFORMAT_PAL4,                 ///< Buffer uses format 4bit palette.
	DISPBUFFORMAT_PAL8,                 ///< Buffer uses format 8bit palette.

	DISPBUFFORMAT_YUV444,               ///< Buffer uses format YUV444.
	DISPBUFFORMAT_YUV422,               ///< Buffer uses format YUV422.
	DISPBUFFORMAT_YUV420,               ///< Buffer uses format YUV420.

	DISPBUFFORMAT_ARGB4565,             ///< Buffer uses format ARGB 4565.
	DISPBUFFORMAT_ARGB8565,             ///< Buffer uses format ARGB 8565.

	DISPBUFFORMAT_YUV422PACK,           ///< Buffer uses format YUV422.
	DISPBUFFORMAT_YUV420PACK,           ///< Buffer uses format YUV420.

	DISPBUFFORMAT_ARGB8888,             ///< Buffer uses format ARGB 8888.

	DISPBUFFORMAT_ARGB4444,             ///< Buffer uses format ARGB 4444.
	DISPBUFFORMAT_ARGB1555,             ///< Buffer uses format ARGB 1555.



	ENUM_DUMMY4WORD(DISPBUFFORMAT)
} DISPBUFFORMAT;

/**
    Display Video Layer mode selection

    This definition is used for the OP Code: "DISPLAYER_OP_SET_MODE" in the disp_lyr_ctrl API to specify the buffer display mode.
    In the NT96220, the OSD layer's display mode is fixed as DISPBUFMODE_BUFFER_REPEAT.
*/
typedef enum {
	DISPBUFMODE_AUTO_HANDSHAKE,             ///< Automatically change buffer according to IPE/IME signal
	DISPBUFMODE_RW_INDEPENDENT,             ///< Automatically change buffer according to ide's VSync signal
	DISPBUFMODE_BUFFER_REPEAT,              ///< Repeat the same buffer until user change buffer

	ENUM_DUMMY4WORD(DISPBUFMODE)
} DISPBUFMODE;

/**
    Display Layer Buffer number

    This definition is used for the OP Code: "DISPLAYER_OP_SET_MODE" in the disp_lyr_ctrl API to specify
    the layer buffer number in the buffer mode DISPBUFMODE_AUTO_HANDSHAKE/DISPBUFMODE_RW_INDEPENDENT/DISPBUFMODE_BUFFER_REPEAT.
*/
typedef enum {
	DISPBUFNUM_1,                           ///< Only 1 buffer (Buffer 0)
	DISPBUFNUM_2,                           ///< Total 2 buffers (Buffer 0, 1)
	DISPBUFNUM_3,                           ///< Total 3 buffers (Buffer 0, 1, 2)
	DISPBUFNUM_4,                           ///< Total 4 buffers (Buffer 0, 1, 2, 3)

	ENUM_DUMMY4WORD(DISPBUFNUM)
} DISPBUFNUM;

/**
    Display Layer Buffer output direction

    This definition is used for the OP Code: "DISPLAYER_OP_SET_OUTDIR" in the disp_lyr_ctrl API to specify the buffer output direction.
    This feature can make the effects of Horizontal/Vertical Flip or the rotation 180 degrees on the display.
    This feature can be assigned for each display layer with different setting.
*/
typedef enum {
	DISPOUTDIR_NORMAL       = 0x0,          ///< Normal output direction
	DISPOUTDIR_HRZ_FLIP     = 0x1,          ///< Horizontal flip
	DISPOUTDIR_VTC_FLIP     = 0x2,          ///< Vertical flip
	DISPOUTDIR_ROT_180      = 0x3,          ///< Rotate 180 degree

	ENUM_DUMMY4WORD(DISPOUTDIR)
} DISPOUTDIR;

/**
    Display Layer Active Buffer

    This definition is used for the OP Code: "DISPLAYER_OP_SET_ACTBUF"/"DISPLAYER_OP_SET_VDOBUFADDR"/"DISPLAYER_OP_SET_OSDBUFADDR" in the disp_lyr_ctrl API
    to specify which of the active buffer is assigned to be displayed.
    This feature can be assigned for each display layer with different setting.
*/
typedef enum {
	DISPACTBUF_0,                           ///< select Active Buffer 0
	DISPACTBUF_1,                           ///< select Active Buffer 1
	DISPACTBUF_2,                           ///< select Active Buffer 2

	DISPACTBUF_NUM,                         ///< total number of active buffer
	ENUM_DUMMY4WORD(DISPACTBUF)
} DISPACTBUF;

/**
    select Display Layer Buffer addresses would be updated

    This definition is used for the OP Code: "/"DISPLAYER_OP_SET_VDOBUFADDR"/"DISPLAYER_OP_SET_OSDBUFADDR" in the disp_lyr_ctrl API
    to specify which of the buffer addresses would be updated.
    This feature can be assigned for each display layer with different setting.
*/
typedef enum {
	DISPBUFADR_ALL  = 0x00,                 ///< Update Video/OSD layer Buffer 0/1/2 Address
	DISPBUFADR_0    = 0x01,                 ///< Update Video/OSD layer Buffer 0 Address
	DISPBUFADR_1    = 0x02,                 ///< Update Video/OSD layer Buffer 1 Address
	DISPBUFADR_2    = 0x04,                 ///< Update Video/OSD layer Buffer 2 Address

	DISPBUFADR_MSK  = 0x0F,                 ///< no use item
	ENUM_DUMMY4WORD(DISPBUFADR)
} DISPBUFADR;


/**
    Display Engine blending select definition

    Dispaly Engine blending layer define: Video1: Layer0, Video2: Layer1, FD: Layer2, OSD1: Layer3, OSD2: Layer4
    The blending Order: ((((Video1+Video2)+FD)+OSD1)+OSD2)

    If Display Engine has only Video1 and OSD1: Video1: Layer0, OSD1: Layer1
    The blending Order: (Video1+OSD1)

    This definition is used for the OP Code: DISPLAYER_OP_SET_BLEND, DISPLAYER_OP_GET_BLEND
*/
typedef enum {
	DISPCTRL_BLEND_TYPE_NOALPHA,
	DISPCTRL_BLEND_TYPE_GLOBAL,
	DISPCTRL_BLEND_TYPE_GLOBAL_BACK,
	DISPCTRL_BLEND_TYPE_SOURCE,    ///< only support in osd
	DISPCTRL_BLEND_TYPE_SOURCE_BACK, 

	DISPCTRL_BLEND_SEL_DEFAULT = DISPCTRL_BLEND_TYPE_NOALPHA,
	ENUM_DUMMY4WORD(DISPBLENDTYPE)
} DISPBLENDTYPE;

/**
    Display Layer color key compare source selection

    This definition is used for the OP Code: DISPLAYER_OP_SET_VDOCOLORKEY_SRC, DISPLAYER_OP_GET_VDOCOLORKEY_SRC
*/
typedef enum {
	DISPVDOCK_CMP_V2 = 0x0,
	DISPVDOCK_CMP_V1 = 0x1,
	ENUM_DUMMY4WORD(DISPVDOCKCMPSOURCE)
} DISPVDOCKCMPSOURCE;

/**
    Display Layer color key opeation selection

    This definition is used for the OP Code: DISPLAYER_OP_SET_COLORKEY, DISPLAYER_OP_GET_COLORKEY
*/
typedef enum {
	DISPCK_OP_OFF = 0x0,                 ///< For VDO: vdo color key enable =0 (videock = video1 or video2; refer to setting of ck_src). For OSD: osd color key enable = 0
	DISPCK_OP_YSMALLKEY = 0x1,           ///< ck = (Y < YKEY) ?
	DISPCK_OP_YEQUKEY = 0x2,             ///< ck = (Y == YKEY && CB == CBKEY && CR == CRKEY) ?
	DISPCK_OP_YBIGKEY = 0x3,             ///< ck = (Y > YKEY) ?
	DISPCK_OP_YEQUKEY_A = 0x4,           ///< ck = (Y == YKEY && CB == CBKEY && CR == CRKEY && ALPHA == AKEY) ?
	ENUM_DUMMY4WORD(DISPCKOP)
} DISPCKOP;

/**
    Display Layer dma burst length selection

    This definition is used for the OP Code: DISPLAYER_OP_SET_DMALEN, DISPLAYER_OP_GET_DMALEN
*/
/*
typedef enum
{
    DISPDMALEN_16 = 0,                   ///< DMA Len = 16 word
    DISPDMALEN_24 = 1,                   ///< DMA Len = 24 word
    DISPDMALEN_32 = 2,                   ///< DMA Len = 32 word
    DISPDMALEN_48 = 3,                   ///< DMA Len = 48 word
    DISPDMALEN_64 = 4,                   ///< DMA Len = 64 word
    DISPDMALEN_96 = 5,                   ///< DMA Len = 96 word
    ENUM_DUMMY4WORD(DISPDMALEN)
}DISPDMALEN;
*/

/**
    Display Layer FD number selection

    This definition is used for the OP Code: DISPLAYER_OP_SET_FDEN, DISPLAYER_OP_GET_FDEN
*/
typedef enum {
	DISPFD_NUM0  = 0x00000001,             ///< Number 0
	DISPFD_NUM1  = 0x00000002,             ///< Number 1
	DISPFD_NUM2  = 0x00000004,             ///< Number 2
	DISPFD_NUM3  = 0x00000008,             ///< Number 3
	DISPFD_NUM4  = 0x00000010,             ///< Number 4
	DISPFD_NUM5  = 0x00000020,             ///< Number 5
	DISPFD_NUM6  = 0x00000040,             ///< Number 6
	DISPFD_NUM7  = 0x00000080,             ///< Number 7
	DISPFD_NUM8  = 0x00000100,             ///< Number 8
	DISPFD_NUM9  = 0x00000200,             ///< Number 9
	DISPFD_NUM10 = 0x00000400,             ///< Number 10
	DISPFD_NUM11 = 0x00000800,             ///< Number 11
	DISPFD_NUM12 = 0x00001000,             ///< Number 12
	DISPFD_NUM13 = 0x00002000,             ///< Number 13
	DISPFD_NUM14 = 0x00004000,             ///< Number 14
	DISPFD_NUM15 = 0x00008000,             ///< Number 15
	ENUM_DUMMY4WORD(DISPFDNUM)
} DISPFDNUM;

/**
    Display Layer operation selection

    This definition is used as the second operand in the disp_lyr_ctrl API to specify which of the display layer control operation
    would be performed.
*/
typedef enum {
	/* SET control group */
	DISPLAYER_OP_SET_ENABLE = DISPCMDBASE_DISPLYR,  ///< Set specified display Layer Enable/Disable.
	DISPLAYER_OP_SET_MODE,                  ///< Configure selected layer buffer mode such as BUF Jump mode,Buffer Number, Buffer format.
	DISPLAYER_OP_SET_BUFSIZE,               ///< Set display Buffer Size for Video/OSD layer such as buffer Width/Height/LineOffset
	DISPLAYER_OP_SET_VDOBUFADDR,            ///< Set display Video Buffer Address
	DISPLAYER_OP_SET_OSDBUFADDR,            ///< Set display OSD Buffer Address
	DISPLAYER_OP_SET_ACTBUF,                ///< Set specified active buffer number when buffer is at buffer-repeat-mode.
	DISPLAYER_OP_SET_OUTDIR,                ///< Set Buffer Output Order such as Flip / rotation
	DISPLAYER_OP_SET_WINSIZE,               ///< Set Output Window Size and window offset. This is truely size seen on the display/panel.
	DISPLAYER_OP_SET_PALETTE,               ///< Set the OSD palette table. Valid for OSD layer only.
	DISPLAYER_OP_SET_PALETTEACRCBY,         ///< Set the OSD palette table. Valid for OSD layer only. format ACrCbY
	DISPLAYER_OP_SET_BLEND,                 ///< Set the layer's blending
	DISPLAYER_OP_SET_VDOCOLORKEY_SRC,       ///< Set the Video color key source
	DISPLAYER_OP_SET_COLORKEY,              ///< Set the layer's color key
	DISPLAYER_OP_SET_BLINK,                 ///< Set the layer's blinking
	DISPLAYER_OP_SET_BUFXY,                 ///< Set the x/y of buffer source window
	//DISPLAYER_OP_SET_DMALEN,              ///< set the dma burst length
	DISPLAYER_OP_SET_FDEN,                  ///< Set the FD rect. number enable/disable
	DISPLAYER_OP_SET_FDSIZE,                ///< Set the FD rect. size
	DISPLAYER_OP_SET_FDCOLOR,               ///< Set the FD rect. color
	DISPLAYER_OP_SET_FDLINESWAP,            ///< Set the FD & line layer swap(TRUE:Line is upper than FD / FALSE:FD is upper than Line)

	/* GET control group */
	DISPLAYER_OP_GET_ENABLE,                ///< Get specified layer Enable/Disable.
	DISPLAYER_OP_GET_MODE,                  ///< Get selected layer buffer mode such as BUF Jump mode,Buffer Number, Buffer format.
	DISPLAYER_OP_GET_BUFSIZE,               ///< Get display Buffer Size for Video/OSD layer such as buffer Width/Height/LineOffset
	DISPLAYER_OP_GET_VDOBUFADDR,            ///< Get display Video Buffer Address
	DISPLAYER_OP_GET_OSDBUFADDR,            ///< Get display OSD Buffer Address
	DISPLAYER_OP_GET_ACTBUF,                ///< Get specified layer active buffer number.
	DISPLAYER_OP_GET_OUTDIR,                ///< Get Buffer Output Order such as Flip / rotation
	DISPLAYER_OP_GET_WINSIZE,               ///< Get Output Window Size and window offset. This is truely size seen on the display/panel.
	DISPLAYER_OP_GET_PALETTE,               ///< Get the OSD palette table. Valid for OSD layer only.
	DISPLAYER_OP_GET_SHADOW_PALETTE,        ///< Get the OSD shadow palette table. Valid for OSD layer only.
	DISPLAYER_OP_GET_PALETTEACRCBY,         ///< Get the OSD palette table. Valid for OSD layer only. format ACrCbY
	DISPLAYER_OP_GET_BLEND,                 ///< Get the layer's blending
	///< @note FD & Line swap = 0 => lower -> upper layer = VDO1--->VDO2--->Line--->FD--->OSD1
	///<                                                      alpha1  alpha2  alpha3  alpha4 [if ARGB1555:alpha5 option]
	///< @note FD & Line swap = 1 => lower -> upper layer = VDO1--->VDO2--->FD--->Line--->OSD1
	///<                                                      alpha1  alpha2  alpha3  alpha4 [if ARGB1555:alpha5 option]
	DISPLAYER_OP_GET_VDOCOLORKEY_SRC,       ///< Get the Video color key source
	DISPLAYER_OP_GET_COLORKEY,              ///< Get the layer's color key
	DISPLAYER_OP_GET_BLINK,                 ///< Get the layer's blinking
	DISPLAYER_OP_GET_BUFXY,                 ///< Get the x/y of buffer source window
	//DISPLAYER_OP_GET_DMALEN,              ///< Get the dma burst length
	DISPLAYER_OP_GET_FDEN,                  ///< Get the FD rect. number enable/disable
	DISPLAYER_OP_GET_FDSIZE,                ///< Get the FD rect. size
	DISPLAYER_OP_GET_FDCOLOR,               ///< Get the FD rect. color
	DISPLAYER_OP_GET_FDLINESWAP,            ///< Get the FD & line layer swap
	DISPLAYER_OP_GET_CST_FROM_RGB_TO_YUV,   ///< Get the OSD layer's YUV data of color key(RGB->YUV)(Only for OSD1)

	/* Macro API */
	DISPLAYER_OP_SET_VDOBUFCONTENT,         ///< Set specified video layer buffer content to specified color. Valid for video layer only.

	DISPLAYER_OP_SET_H_BILINEAR,            ///< Set the layer's horizontal scaling method

	DISPLAYER_OP_SET_BUFWINSIZE,            ///< Set the layer's buffer size and window size

	ENUM_DUMMY4WORD(DISPLAYER_OP)
} DISPLAYER_OP;

/**
    Display Layer Control Parameter Definition

    This definition is used as the third operand in the disp_lyr_ctrl API to specify the display layer control parameters
    for each of the layer control opeation.
*/
typedef struct {
	union {
		/* SET control group */

		/**
		    Set Specified Display Layer Enable/Disable

		    This structure is used for layer operation code "DISPLAYER_OP_SET_ENABLE" to set the specified "DISPLAYER" enable/disable.
		    This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_ENABLE.b_en = TRUE;  //Or FALSE
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_ENABLE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_ENABLE
			BOOL            b_en;            ///< Set specified display layer ON/OFF. TRUE: Enable. FALSE:Disable.
		} SET_ENABLE;

		/**
		    Set Specified Display Layer Buffer mode/Format

		    This structure is used for layer operation code "DISPLAYER_OP_SET_MODE"
		    to set the specified "DISPLAYER" buffer Format/mode/Number. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_MODE.buf_format = DISPBUFFORMAT_YUV422;              // Or DISPBUFFORMAT_PAL8 ...etc
		\n  disp_lyr.SEL.SET_MODE.buf_mode   = DISPBUFMODE_BUFFER_REPEAT;         // Or DISPBUFMODE_AUTO_HANDSHAKE
		\n  disp_lyr.SEL.SET_MODE.buf_number = DISPBUFNUM_3;                      // Or DISPBUFNUM_2 / DISPBUFNUM_1
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_MODE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_MODE
			DISPBUFFORMAT   buf_format;      ///< Video/OSD Buffer Format, such as YUV444/YUV422/YUV420/ARGB8565/ARGB4565
			DISPBUFMODE     buf_mode;        ///< Video/OSD Buffer operation mode. OSD layer support buffer repeat mode only
			DISPBUFNUM      buf_number;      ///< Configure used buffer number for Video/OSD
		} SET_MODE;

		/**
		    Set Specified Display Layer Buffer Size

		    This structure is used for layer operation code "DISPLAYER_OP_SET_BUFSIZE"
		    to set the specified "DISPLAYER" buffer Width/Height/LineOffset. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_BUFSIZE.ui_buf_width  = 320;
		\n  disp_lyr.SEL.SET_BUFSIZE.ui_buf_height = 240;
		\n  disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs= 320;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_BUFSIZE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_BUFSIZE
			UINT32          ui_buf_width;     ///< Set display buffer width in PIXEL COUNT for Video/OSD layer.
			UINT32          ui_buf_height;    ///< Set display buffer height in PIXEL COUNT for Video/OSD layer.

			UINT32          ui_buf_line_ofs;   ///< Set display buffer lineoffset in BYTES for Video/OSD layer.
		} SET_BUFSIZE;

		/**
		    Set Specified Display VIDEO Layer Buffer Address

		    This structure is used for layer operation code "DISPLAYER_OP_SET_VDOBUFADDR"
		    to set the specified "DISPLAYER" buffer address.
		    This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_VDOBUFADDR.buf_sel     = DISPBUFADR_ALL;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0   = 0x400000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0  = 0x480000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr0  = 0x500000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y1   = 0x580000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb1  = 0x600000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr1  = 0x680000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y2   = 0x700000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb2  = 0x780000;
		\n  disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr2  = 0x800000;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_VDOBUFADDR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_VDOBUFADDR
			DISPBUFADR      buf_sel;         ///< select which of the buffer addresses would be updated for Video layer.

			UINT32          ui_addr_y0;       ///< Buffer 0 Starting Address for  Y/RGB565/RGB888 plane
			UINT32          ui_addr_cb0;      ///< Buffer 0 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr0;      ///< Buffer 0 Starting Address for Cr        plane

			UINT32          ui_addr_y1;       ///< Buffer 1 Starting Address for  Y/RGB565/RGB888 plane
			UINT32          ui_addr_cb1;      ///< Buffer 1 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr1;      ///< Buffer 1 Starting Address for Cr        plane

			UINT32          ui_addr_y2;       ///< Buffer 2 Starting Address for  Y/RGB565/RGB888 plane
			UINT32          ui_addr_cb2;      ///< Buffer 2 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr2;      ///< Buffer 2 Starting Address for Cr        plane

			UINT32          ui_addr_y3;       ///< Buffer 3 Starting Address for  Y/RGB565/RGB888 plane
			UINT32          ui_addr_cb3;      ///< Buffer 3 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr3;      ///< Buffer 3 Starting Address for Cr        plane

		} SET_VDOBUFADDR;

		/**
		    Set Specified Display OSD Layer Buffer Address

		    This structure is used for layer operation code "DISPLAYER_OP_SET_OSDBUFADDR"
		    to set the specified "DISPLAYER" buffer ADDRESS. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_OSDBUFADDR.buf_sel     = DISPBUFADR_ALL;
		\n  disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf0 = 0x400000;
		\n  disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf1 = 0x480000;
		\n  disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf2 = 0x500000;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_OSDBUFADDR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_OSDBUFADDR
			DISPBUFADR      buf_sel;         ///< select which of the buffer addresses would be updated for OSD layer.

			UINT32          ui_addr_buf0;     ///< Buffer 0 Starting Address for OSD layer
			UINT32          ui_addr_buf1;     ///< Buffer 1 Starting Address for OSD layer
			UINT32          ui_addr_buf2;     ///< Buffer 2 Starting Address for OSD layer

#if 1// 96220 has no ARGB buffer format
			UINT32          ui_addr_alpha0;
			UINT32          ui_addr_alpha1;
			UINT32          ui_addr_alpha2;
#endif
		} SET_OSDBUFADDR;

		/**
		    Set Specified Display Layer Active Buffer

		    This structure is used for layer operation code "DISPLAYER_OP_SET_ACTBUF"
		    to set the specified "DISPLAYER" active buffer. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_ACTBUF.active_buf  = DISPACTBUF_0/1/2;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_ACTBUF, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_ACTBUF
			DISPACTBUF      active_buf;      ///< select which Video/OSD buffer is activated. For video layer, this field is valid only at buffer repeat mode.
		} SET_ACTBUF;

		/**
		    Set Specified Display Layer Buffer output direction

		    This structure is used for layer operation code "DISPLAYER_OP_SET_OUTDIR"
		    to set the specified "DISPLAYER" buffer output direction. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_OUTDIR.buf_out_dir  = DISPOUTDIR_NORMAL;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_OUTDIR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_OUTDIR
			DISPOUTDIR      buf_out_dir;      ///< select buffer output as Horizontal/Vertical-Flip, or rotation 90/180/270 degrees.
		} SET_OUTDIR;

		/**
		    Set Specified Display Layer output window size and window offset

		    This structure is used for layer operation code "DISPLAYER_OP_SET_WINSIZE"
		    to set the specified "DISPLAYER" window size and also the window offset. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_WINSIZE.ui_win_width  = 320;
		\n  disp_lyr.SEL.SET_WINSIZE.ui_win_height = 240;
		\n  disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x    = 0;
		\n  disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y    = 0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_WINSIZE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_WINSIZE
			UINT32          ui_win_width;     ///< The display Video/OSD layer output window width
			UINT32          ui_win_height;    ///< The display Video/OSD layer output window height

			INT32          i_win_ofs_x;       ///< The display Video/OSD layer output window offset X
			INT32          i_win_ofs_y;       ///< The display Video/OSD layer output window offset Y
		} SET_WINSIZE;

		/**
		    Set Specified Display OSD Layer palette table

		    This structure is used for layer operation code "DISPLAYER_OP_SET_PALETTE"
		    to set the specified "DISPLAYER" palette table. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_PALETTE.ui_start     = 0;
		\n  disp_lyr.SEL.SET_PALETTE.ui_number    = 256;
		\n  disp_lyr.SEL.SET_PALETTE.p_pale_entry  = (Address of palette table);
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_PALETTE, &disp_lyr);
		*/
		struct { // DISPLAYER_OP_SET_PALETTE
			UINT32          ui_start;        ///< Start entry id. Valid range from 0~255.
			UINT32          ui_number;       ///< Total number of palette entry to be set. Valid range from 1~256
			UINT32          *p_pale_entry;    ///< p_pale_entry[7~0]:      Cr value of the palette.
			///< p_pale_entry[15~8]:     Cb value of the palette.
			///< p_pale_entry[23~16]:    Y  value of the palette.
			///< p_pale_entry[31~24]:    Alpha value of the palette.
		} SET_PALETTE;

		/**
		    Set Specified Display OSD Layer palette table

		    This structure is used for layer operation code "DISPLAYER_OP_SET_PALETTE"
		    to set the specified "DISPLAYER" palette table. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_PALETTE.ui_start     = 0;
		\n  disp_lyr.SEL.SET_PALETTE.ui_number    = 256;
		\n  disp_lyr.SEL.SET_PALETTE.p_pale_entry  = (Address of palette table);
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_PALETTE, &disp_lyr);
		*/
		struct { // DISPLAYER_OP_SET_PALETTEACRCBY
			UINT32          ui_start;        ///< Start entry id. Valid range from 0~255.
			UINT32          ui_number;       ///< Total number of palette entry to be set. Valid range from 1~256
			UINT32          *p_pale_entry;    ///<  p_pale_entry[7~0]:      Y value of the palette.
			///< p_pale_entry[15~8]:     Cb value of the palette.
			///< p_pale_entry[23~16]:    Cr  value of the palette.
			///< p_pale_entry[31~24]:    Alpha value of the palette.
		} SET_PALETTEACRCBY;


		struct { // DISPLAYER_OP_SET_BLEND
			DISPBLENDTYPE       type;
			UINT32              ui_global_alpha;
			BOOL                b_global_alpha5;
		} SET_BLEND;

		struct { // DISPLAYER_OP_SET_VDOCOLORKEY_SRC
			DISPVDOCKCMPSOURCE  ck_src;          ///< color key source select
		} SET_VDOCOLORKEY_SRC;

		struct { // DISPLAYER_OP_SET_COLORKEY
			DISPCKOP            ck_op;           ///< color key operation
			UINT32              ui_ck_y;          ///< color key color Y(R)
			UINT32              ui_ck_cb;         ///< color key color Cb(G)
			UINT32              ui_ck_cr;         ///< color key color Cr(B)
			UINT32              ui_ck_alpha;      ///< color key alpha(only used in osd)
		} SET_COLORKEY;

		struct { // DISPLAYER_OP_SET_BLINK
			BOOL                b_over_exp_en;     ///< over exposure blinking enable
			BOOL                b_und_exp_en;      ///< under exposure blinking enable
			//DISPBLINKCH         uich;           ///< blinking channel selection
			UINT32              ui_cnt;          ///< blinking frame count
			UINT32              ui_over_exp_th_y;   ///< over exposure blinking threshold Y value
			UINT32              ui_over_exp_th_cb;  ///< over exposure blinking threshold Cb value
			UINT32              ui_over_exp_th_cr;  ///< over exposure blinking threshold Cr value
			UINT32              ui_und_exp_th_y;    ///< under xposure blinking threshold Y value
			UINT32              ui_und_exp_th_cb;   ///< under exposure blinking threshold Cb value
			UINT32              ui_und_exp_th_cr;   ///< under exposure blinking threshold Cr value
			UINT32              ui_over_exp_color_y; ///< over exposure blinking color Y value
			UINT32              ui_over_exp_color_cb;///< over exposure blinking color Cb value
			UINT32              ui_over_exp_color_cr;///< over exposure blinking color Cr value
			UINT32              ui_und_exp_color_y;  ///< under exposure blinking color Y value
			UINT32              ui_und_exp_color_cb; ///< under exposure blinking color Y value
			UINT32              ui_und_exp_color_cr; ///< under exposure blinking color Y value
		} SET_BLINK_TH;

		/**
		    Set Specified Display Layer buffer source position.

		    This structure is used for layer operation code "DISPLAYER_OP_SET_BUFXY"
		    to set the specified "DISPLAYER" buffer source position. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_BUFXY.ui_buf_ofs_x    = 0;
		\n  disp_lyr.SEL.SET_BUFXY.ui_buf_ofs_y    = 0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_BUFXY, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_BUFXY
			UINT32          ui_buf_ofs_x;       ///< The display Video/OSD layer buffer source offset X
			UINT32          ui_buf_ofs_y;       ///< The display Video/OSD layer buffer source offset Y
		} SET_BUFXY;

		/**
		    Set Specified Display Layer DMA length

		    This structure is used for layer operation code "DISPLAYER_OP_SET_DMALEN"
		    to set the specified "DISPLAYER" dma length. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_DMALEN.DMAY_A = DISPDMALEN_32;
		\n  disp_lyr.SEL.SET_DMALEN.DMAC_RGB = DISPDMALEN_32;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_DMALEN, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		/*
		struct// DISPLAYER_OP_SET_DMALEN
		{
		    DISPDMALEN       DMAY_A;        ///< DMA Len for Y or alpha or palette channel
		    DISPDMALEN       DMAC_RGB;      ///< DMA Len for C or RGB channel
		} SET_DMALEN;
		*/
		/**
		    Set FD Layer enable/disable

		    This structure is used for layer operation code "DISPLAYER_OP_SET_FDEN"
		    to set the specified "FD" enable/disable. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_FDEN.fd_num = DISPFD_NUM0;
		\n  disp_lyr.SEL.SET_FDEN.b_en = TRUE;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_FDLEN, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_FDEN
			DISPFDNUM        fd_num;        ///< FD channel number
			BOOL             b_en;           ///< FD channel enable/disable
		} SET_FDEN;

		/**
		    Set FD Layer size

		    This structure is used for layer operation code "DISPLAYER_OP_SET_FDSIZE"
		    to set the specified "FD" size. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_FDSIZE.fd_num = DISPFD_NUM0;
		\n  disp_lyr.SEL.SET_FDSIZE.ui_fdx = 0;
		\n  disp_lyr.SEL.SET_FDSIZE.ui_fdy = 0;
		\n  disp_lyr.SEL.SET_FDSIZE.ui_fdw = 40;
		\n  disp_lyr.SEL.SET_FDSIZE.ui_fdh = 40;
		\n  disp_lyr.SEL.SET_FDSIZE.ui_fd_bord_w = 4;
		\n  disp_lyr.SEL.SET_FDSIZE.ui_fd_bord_h = 4;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_FDSIZE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_FDSIZE
			DISPFDNUM        fd_num;        ///< FD channel number
			UINT32           ui_fdx;         ///< FD channel position X
			UINT32           ui_fdy;         ///< FD channel position Y
			UINT32           ui_fdw;         ///< FD channel width
			UINT32           ui_fdh;         ///< FD channel height
			UINT32           ui_fd_bord_w;    ///< FD channel border width
			UINT32           ui_fd_bord_h;    ///< FD channel border heigh
		} SET_FDSIZE;

		/**
		    Set FD Layer color

		    This structure is used for layer operation code "DISPLAYER_OP_SET_FDCOLOR"
		    to set the specified "FD" color. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_FDCOLOR.fd_num = DISPFD_NUM0;
		\n  disp_lyr.SEL.SET_FDCOLOR.ui_fd_cr_y = 0x00;
		\n  disp_lyr.SEL.SET_FDCOLOR.ui_fd_cr_cb = 0x80;
		\n  disp_lyr.SEL.SET_FDCOLOR.ui_fd_cr_cr = 0x80;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_FDCOLOR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_FDCOLOR
			DISPFDNUM        fd_num;        ///< FD channel number
			UINT32           ui_fd_cr_y;       ///< FD channel y color
			UINT32           ui_fd_cr_cb;      ///< FD channel cb color
			UINT32           ui_fd_cr_cr;      ///< FD channel cr color
		} SET_FDCOLOR;

		/**
		    Set layer's horizontal scaling method

		    This structure is used for layer operation code "DISPLAYER_OP_SET_H_BILINEAR"
		    to set the specified layer. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_H_BILINEAR.bBiliear = TRUE;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_H_BILINEAR, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_H_BILINEAR
			BOOL             b_bilinear;      ///< Horizontal bilinear method
		} SET_H_BILINEAR;

		/**
		    Set Specified Display Layer Buffer and Window Size

		    This structure is used for layer operation code "DISPLAYER_OP_SET_BUFWINSIZE"
		    to set the specified "DISPLAYER" buffer Width/Height/LineOffset and window size. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_width  = 320;
		\n  disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_height = 240;
		\n  disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_line_ofs= 320;
		\n  disp_lyr.SEL.SET_BUFWINSIZE.ui_win_width  = 320;
		\n  disp_lyr.SEL.SET_BUFWINSIZE.ui_win_height = 240;
		\n  disp_lyr.SEL.SET_BUFWINSIZE.i_win_ofs_x    = 0;
		\n  disp_lyr.SEL.SET_BUFWINSIZE.i_win_ofs_y    = 0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_BUFWINSIZE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_BUFWINSIZE
			UINT32          ui_buf_width;     ///< Set display buffer width in PIXEL COUNT for Video/OSD layer.
			UINT32          ui_buf_height;    ///< Set display buffer height in PIXEL COUNT for Video/OSD layer.

			UINT32          ui_buf_line_ofs;   ///< Set display buffer lineoffset in BYTES for Video/OSD layer.

			UINT32          ui_win_width;     ///< The display Video/OSD layer output window width
			UINT32          ui_win_height;    ///< The display Video/OSD layer output window height

			INT32          i_win_ofs_x;       ///< The display Video/OSD layer output window offset X
			INT32          i_win_ofs_y;       ///< The display Video/OSD layer output window offset Y
		} SET_BUFWINSIZE;

		/**
		    Set OSD layer swap parameter

		    This structure is used for swap osd layer and only valid @ IDE1
		\n  DispDev.SEL.SET_OSDSWAP.bEnOSDSwap  = TRUE or FALSE;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_OSD_LAYER_SWAP, &DispDev);
		*/
		struct { //DISPDEV_SET_OSD_LAYER_SWAP
			BOOL        b_en_fd_line_swap;      ///< Enable / Disable FD line layer swap
			///< 0: FD ' Line (up to low)
			///< 1: Line ' FD (up to low)
		} SET_FDLINESWAP;



		/* GET control group */

		/**
		    Get Specified Display Layer enable/disable

		    This structure is used for layer operation code "DISPLAYER_OP_GET_ENABLE"
		    to get the specified "DISPLAYER" layer enable/disable. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_ENABLE, &disp_lyr);
		\n  "disp_lyr.SEL.GET_ENABLE.b_en"
		    is the specified layer enable/disable.
		*/
		struct { // DISPLAYER_OP_GET_ENABLE
			BOOL            b_en;          ///< Get specified display layer ON/OFF. TRUE: Enable. FALSE:Disable.
		} GET_ENABLE;

		/**
		    Get Specified Display Layer buffer mode/format/number configurations

		    This structure is used for layer operation code "DISPLAYER_OP_GET_ENABLE"
		    to get the specified "DISPLAYER" layer buffer configurations. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_ENABLE, &disp_lyr);
		\n  "disp_lyr.SEL.GET_MODE.buf_format"/"disp_lyr.SEL.GET_MODE.buf_mode"/"disp_lyr.SEL.GET_MODE.buf_number"
		    are the specified layer buffer format/mode/number.
		*/
		struct { // DISPLAYER_OP_GET_MODE
			DISPBUFFORMAT   buf_format;    ///< Get Video/OSD Buffer Format, such as YUV444/YUV422/YUV420/ARGB8565/ARGB4565
			DISPBUFMODE     buf_mode;      ///< Get Video/OSD Buffer operation mode. OSD layer support buffer repeat mode only
			DISPBUFNUM      buf_number;    ///< Get active buffer number for Video/OSD
		} GET_MODE;

		/**
		    Get Specified Display Layer buffer width/height/lineoffset

		    This structure is used for layer operation code "DISPLAYER_OP_GET_BUFSIZE"
		    to get the specified "DISPLAYER" layer buffer width/height/lineoffset. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_BUFSIZE, &disp_lyr);
		\n  "disp_lyr.SEL.GET_BUFSIZE.ui_buf_width"/"disp_lyr.SEL.GET_BUFSIZE.ui_buf_height"/"disp_lyr.SEL.GET_BUFSIZE.ui_buf_line_ofs"
		    are the specified layer buffer width/height/lineoffset.
		*/
		struct { // DISPLAYER_OP_GET_BUFSIZE
			UINT32          ui_buf_width;   ///< Get display buffer width for Video/OSD layer.
			UINT32          ui_buf_height;  ///< Get display buffer height for Video/OSD layer.
			UINT32          ui_buf_line_ofs; ///< Get display buffer lineoffset for Video/OSD layer.
		} GET_BUFSIZE;

		/**
		    Get Specified Display VIDEO Layer buffer address

		    This structure is used for layer operation code "DISPLAYER_OP_GET_VDOBUFADDR"
		    to get the specified "DISPLAYER" layer buffer address. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_VDOBUFADDR, &disp_lyr);
		\n  "disp_lyr.SEL.GET_VDOBUFADDR.active_buf"/"disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_y0"/"disp_lyr.SEL.GET_VDOBUFADDR.ui_addr_cb0"/...
		    are the specified VIDEO layer buffer address and active buffer number.
		*/
		struct { // DISPLAYER_OP_GET_VDOBUFADDR
			DISPACTBUF      active_buf;    ///< This field is valid only when video buffer at buffer repeat mode.
			///< Get which of the buffer address is activated for Video layer.

			UINT32          ui_addr_y0;     ///< Buffer 0 Starting Address for  Y/RGB565 plane
			UINT32          ui_addr_cb0;    ///< Buffer 0 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr0;    ///< Buffer 0 Starting Address for Cr        plane

			UINT32          ui_addr_y1;     ///< Buffer 1 Starting Address for  Y/RGB565 plane
			UINT32          ui_addr_cb1;    ///< Buffer 1 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr1;    ///< Buffer 1 Starting Address for Cr        plane

			UINT32          ui_addr_y2;     ///< Buffer 2 Starting Address for  Y/RGB565 plane
			UINT32          ui_addr_cb2;    ///< Buffer 2 Starting Address for Cb/Alpha  plane
			UINT32          ui_addr_cr2;    ///< Buffer 2 Starting Address for Cr        plane
		} GET_VDOBUFADDR;

		/**
		    Get Specified Display OSD Layer buffer address

		    This structure is used for layer operation code "DISPLAYER_OP_GET_OSDBUFADDR"
		    to get the specified "DISPLAYER" layer buffer address. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_OSDBUFADDR, &disp_lyr);
		\n  "disp_lyr.SEL.GET_OSDBUFADDR.active_buf"/"disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_buf0"/"disp_lyr.SEL.GET_OSDBUFADDR.ui_addr_buf1"/...
		    are the specified OSD layer buffer address and active buffer number.
		*/
		struct { // DISPLAYER_OP_GET_OSDBUFADDR
			DISPACTBUF      active_buf;      ///< select which one of the buffer address is activated for OSD layer.

			UINT32          ui_addr_buf0;     ///< Buffer 0 Starting Address for OSD layer
			UINT32          ui_addr_buf1;     ///< Buffer 1 Starting Address for OSD layer
			UINT32          ui_addr_buf2;     ///< Buffer 2 Starting Address for OSD layer

#if 1// 96220 has no ARGB buffer format
			UINT32          ui_addr_alpha0;
			UINT32          ui_addr_alpha1;
			UINT32          ui_addr_alpha2;
#endif
		} GET_OSDBUFADDR;

		/**
		    Get Specified Display Layer active buffer

		    This structure is used for layer operation code "DISPLAYER_OP_GET_ACTBUF"
		    to get the specified "DISPLAYER" layer active buffer if in the buffer repeat mode. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_ACTBUF, &disp_lyr);
		\n  "disp_lyr.SEL.GET_ACTBUF.active_buf"
		    is the specified layer active buffer ID.
		*/
		struct { // DISPLAYER_OP_GET_ACTBUF
			DISPACTBUF      active_buf;      ///< Get which Video/OSD buffer is activated. For video layer, this field is valid only at buffer repeat mode.
		} GET_ACTBUF;

		/**
		    Get Specified Display Layer buffer output direction

		    This structure is used for layer operation code "DISPLAYER_OP_GET_OUTDIR"
		    to get the specified "DISPLAYER" layer buffer output direction. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_OUTDIR, &disp_lyr);
		\n  "disp_lyr.SEL.GET_OUTDIR.buf_out_dir"
		    is the specified layer buffer output direction.
		*/
		struct { // DISPLAYER_OP_GET_OUTDIR
			DISPOUTDIR      buf_out_dir;      ///< Get Horizontal/Vertical-Flip, or rotation 90/180/270 degrees.
		} GET_OUTDIR;

		/**
		    Get Specified Display Layer output window size/offset

		    This structure is used for layer operation code "DISPLAYER_OP_GET_WINSIZE"
		    to get the specified "DISPLAYER" layer output window size/offset. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_WINSIZE, &disp_lyr);
		\n  "disp_lyr.SEL.GET_WINSIZE.ui_win_width"/"disp_lyr.SEL.GET_WINSIZE.ui_win_height"/"disp_lyr.SEL.GET_WINSIZE.i_win_ofs_x"/"disp_lyr.SEL.GET_WINSIZE.i_win_ofs_y"
		    are the specified layer buffer output window size/offset.
		*/
		struct { // DISPLAYER_OP_GET_WINSIZE
			UINT32          ui_win_width;     ///< The display Video/OSD layer output window width
			UINT32          ui_win_height;    ///< The display Video/OSD layer output window height

			INT32          i_win_ofs_x;       ///< The display Video/OSD layer output window offset X
			INT32          i_win_ofs_y;       ///< The display Video/OSD layer output window offset Y
		} GET_WINSIZE;

		/**
		    Get Specified Display OSD Layer palette table

		    This structure is used for layer operation code "DISPLAYER_OP_GET_PALETTE"
		    to get the specified "DISPLAYER" layer palette table. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_PALETTE, &disp_lyr);
		\n  "disp_lyr.SEL.GET_PALETTE.ui_start"/"disp_lyr.SEL.GET_PALETTE.ui_number"/"disp_lyr.SEL.GET_PALETTE.p_pale_entry"
		    are the specified OSD layer palette table.
		*/
		struct { // DISPLAYER_OP_GET_PALETTE
			UINT32          ui_start;        ///< Start entry id. Valid range from 0~255.
			UINT32          ui_number;       ///< Total number of palette entry to be set. Valid range from 1~256
			UINT32          *p_pale_entry;    ///< p_pale_entry[7~0]:      Cr value of the palette.
			///< p_pale_entry[15~8]:     Cb value of the palette.
			///< p_pale_entry[23~16]:    Y  value of the palette.
			///< p_pale_entry[31~24]:    Alpha value of the palette.
		} GET_PALETTE;

		/**
		    Get Specified Display OSD Layer palette table

		    This structure is used for layer operation code "DISPLAYER_OP_GET_PALETTE"
		    to get the specified "DISPLAYER" layer palette table. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_PALETTE, &disp_lyr);
		\n  "disp_lyr.SEL.GET_PALETTE.ui_start"/"disp_lyr.SEL.GET_PALETTE.ui_number"/"disp_lyr.SEL.GET_PALETTE.p_pale_entry"
		    are the specified OSD layer palette table.
		*/
		struct { // DISPLAYER_OP_GET_PALETTEACRCBY
			UINT32          ui_start;        ///< Start entry id. Valid range from 0~255.
			UINT32          ui_number;       ///< Total number of palette entry to be set. Valid range from 1~256
			UINT32          *p_pale_entry;    ///< p_pale_entry[7~0]:      Y value of the palette.
			///< p_pale_entry[15~8]:     Cb value of the palette.
			///< p_pale_entry[23~16]:    Cr  value of the palette.
			///< p_pale_entry[31~24]:    Alpha value of the palette.
		} GET_PALETTEACRCBY;


		/* Macro API */

		/**
		    Get Specified Video Display Layer Buffer Content

		    This structure is used for layer operation code "DISPLAYER_OP_SET_VDOBUFCONTENT" to
		    set the specified Video "DISPLAYER"'s buffer content.
		    This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_VDOBUFCONTENT.b_en = TRUE;  //Or FALSE
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_ENABLE, &disp_lyr);
		    @note This control operation is VD Latached, so the user must set pDispObj->load to validate the configurations.
		*/
		struct { // DISPLAYER_OP_SET_VDOBUFCONTENT
			IDE_BUFFERID            buf_id;    ///< select which of the Buffer ID to fill the content.
			PYUVCOLOR               p_ycbcr;   ///< The color which will be filled up with the buffer
		} SET_VDOBUFCONTENT;


		struct { // DISPLAYER_OP_GET_BLEND
			DISPBLENDTYPE       type;
			UINT32              ui_global_alpha;
			BOOL                b_global_alpha5;
		} GET_BLEND;


		struct { // DISPLAYER_OP_GET_VDOCOLORKEY_SRC
			DISPVDOCKCMPSOURCE  ck_src;          ///< color key source select
		} GET_VDOCOLORKEY_SRC;


		struct { // DISPLAYER_OP_GET_COLORKEY
			DISPCKOP            ck_op;           ///< color key operation
			UINT32              ui_ck_y;          ///< color key color Y(R)
			UINT32              ui_ck_cb;         ///< color key color Cb(G)
			UINT32              ui_ck_cr;         ///< color key color Cr(B)
			UINT32              ui_ck_alpha;      ///< color key alpha(only used in osd)
		} GET_COLORKEY;


		/**
		    Get Specified Display Layer buffer source position.

		    This structure is used for layer operation code "DISPLAYER_OP_SET_BUFXY"
		    to get the specified "DISPLAYER" buffer source position. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_BUFXY, &disp_lyr);
		\n  "disp_lyr.SEL.GET_BUFXY.ui_buf_ofs_x"
		\n  "disp_lyr.SEL.GET_BUFXY.ui_buf_ofs_y"
		    is the buffer source position of specified "DISPLAYER"
		*/
		struct { // DISPLAYER_OP_GET_BUFXY
			UINT32          ui_buf_ofs_x;       ///< The display Video/OSD layer buffer source offset X
			UINT32          ui_buf_ofs_y;       ///< The display Video/OSD layer buffer source offset Y
		} GET_BUFXY;

		/**
		    Get Specified Display Layer DMA length

		    This structure is used for layer operation code "DISPLAYER_OP_GET_DMALEN"
		    to get the specified "DISPLAYER" dma length. This structure shuld be used like this:
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_DMALEN, &disp_lyr);
		\n  "disp_lyr.SEL.GET_DMALEN.DMAY_A"
		\n  "disp_lyr.SEL.GET_DMALEN.DMAC_RGB"
		    is the dma length of specified "DISPLAYER"
		*/
		/*struct// DISPLAYER_OP_GET_DMALEN
		{
		    DISPDMALEN       DMAY_A;        ///< DMA Len for Y or alpha or palette channel
		    DISPDMALEN       DMAC_RGB;      ///< DMA Len for C or RGB channel
		} GET_DMALEN;
		*/
		/**
		    Get FD Layer enable/disable

		    This structure is used for layer operation code "DISPLAYER_OP_GET_FDEN"
		    to get the specified "FD" enable/disable. This structure shuld be used like this:
		\n  disp_lyr.GEL.GET_FDEN.fd_num = DISPFD_NUM0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_FDLEN, &disp_lyr);
		\n  "disp_lyr.GEL.GET_FDEN.b_en"
		    is the FD number enable/disable
		*/
		struct { // DISPLAYER_OP_GET_FDEN
			DISPFDNUM        fd_num;        ///< FD channel number
			BOOL             b_en;           ///< FD channel enable/disable
		} GET_FDEN;

		/**
		    Get FD Layer size

		    This structure is used for layer operation code "DISPLAYER_OP_GET_FDSIZE"
		    to get the specified "FD" size. This structure shuld be used like this:
		\n  disp_lyr.SEL.SET_FDSIZE.fd_num = DISPFD_NUM0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_SET_FDSIZE, &disp_lyr);
		\n  "disp_lyr.SEL.SET_FDSIZE.ui_fdx"
		\n  "disp_lyr.SEL.SET_FDSIZE.ui_fdy"
		\n  "disp_lyr.SEL.SET_FDSIZE.ui_fdw"
		\n  "disp_lyr.SEL.SET_FDSIZE.ui_fdh"
		\n  "disp_lyr.SEL.SET_FDSIZE.ui_fd_bord_w"
		\n  "disp_lyr.SEL.SET_FDSIZE.ui_fd_bord_h"
		    is the FD number size
		*/
		struct { // DISPLAYER_OP_GET_FDSIZE
			DISPFDNUM        fd_num;        ///< FD channel number
			UINT32           ui_fdx;         ///< FD channel position X
			UINT32           ui_fdy;         ///< FD channel position Y
			UINT32           ui_fdw;         ///< FD channel width
			UINT32           ui_fdh;         ///< FD channel height
			UINT32           ui_fd_bord_w;    ///< FD channel border width
			UINT32           ui_fd_bord_h;    ///< FD channel border heigh
		} GET_FDSIZE;

		/**
		    Get FD Layer color

		    This structure is used for layer operation code "DISPLAYER_OP_GET_FDCOLOR"
		    to get the specified "FD" color. This structure shuld be used like this:
		\n  disp_lyr.GEL.GET_FDCOLOR.fd_num = DISPFD_NUM0;
		\n  pDispObj->disp_lyr_ctrl(DISPLAYER, DISPLAYER_OP_GET_FDCOLOR, &disp_lyr);
		\n  "disp_lyr.GEL.GET_FDCOLOR.ui_fd_cr_y"
		\n  "disp_lyr.GEL.GET_FDCOLOR.ui_fd_cr_cb"
		\n  "disp_lyr.GEL.GET_FDCOLOR.ui_fd_cr_cr"
		    is the FD number color
		*/
		struct { // DISPLAYER_OP_GET_FDCOLOR
			DISPFDNUM        fd_num;        ///< FD channel number
			UINT32           ui_fd_cr_y;       ///< FD channel y color
			UINT32           ui_fd_cr_cb;      ///< FD channel cb color
			UINT32           ui_fd_cr_cr;      ///< FD channel cr color
		} GET_FDCOLOR;

		/**
		    Get FD line layer swap parameter

		    This structure is used for swap osd layer and only valid @ IDE1
		\n  DispDev.SEL.SET_OSDSWAP.bEnOSDSwap  = TRUE or FALSE;
		\n  pDispObj->dev_ctrl(DISPDEV_SET_OSD_LAYER_SWAP, &DispDev);
		*/
		struct { //DISPLAYER_OP_GET_OSDSWAP
			BOOL        b_en_fd_line_swap;      ///< Enable / Disable FD line layer swap
			///< 0: FD ' Line (up to low)
			///< 1: Line ' FD (up to low)

		} GET_FDLINESWAP;

		/**
		    Get CST value of RGB to YUV for color key @ ARGB

		    This structure is used only for OSD layer
		    @code
		    {
				UINT8 r_to_y;
				UINT8 g_to_u;
				UINT8 b_to_v;

				UINT16  rgb565 = 0x50EA; //RGB565 plane
					=>bit[ 4..0] = R => ((0x50ea & 0x1F) << 3           => 0xa ---> r_to_y
					=>bit[10..5] = G => (((0x50ea >> 5) & 0x3F) << 2)   => 0x07---> g_to_u
					=>bit[4..0] = R => (((0x50ea >> 11) & 0x1F) << 3)   => 0xa ---> b_to_v
				DISPLAYER_PARAM test_disp_param;

				test_disp_param.SEL.GET_CST_OF_RGB_TO_YUV.r_to_y = r_to_y;
				test_disp_param.SEL.GET_CST_OF_RGB_TO_YUV.g_to_u = g_to_u;
				test_disp_param.SEL.GET_CST_OF_RGB_TO_YUV.b_to_v = b_to_v;

				disp_set_disp1_layer_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_GET_CST_FROM_RGB_TO_YUV, &test_disp_param);
				DBG_DUMP("output Y=%d, U=%d, V=%d\r\n", p_lyr_param->SEL.GET_CST_OF_RGB_TO_YUV.r_to_y, p_lyr_param->SEL.GET_CST_OF_RGB_TO_YUV.g_to_u, p_lyr_param->SEL.GET_CST_OF_RGB_TO_YUV.b_to_v);
		    }
		    @endcode

		*/
		struct { //DISPLAYER_OP_GET_CST_FROM_RGB_TO_YUV
			UINT8    r_to_y;    ///< Point of R data and output for Y data (need shift to 8bit)
			UINT8    g_to_u;    ///< Point of G data and output for U data (need shift to 8bit)
			UINT8    b_to_v;    ///< Point of B data and output for V data (need shift to 8bit)
			UINT8    alpha;     ///< Use for align to 4 byte (alpha here is useless)
		} GET_CST_OF_RGB_TO_YUV;
	} SEL;
} DISPLAYER_PARAM, *PDISPLAYER_PARAM;
//@}


typedef ER(*DISPLYRCTRL)(DISPLAYER, DISPLAYER_OP, PDISPLAYER_PARAM);
//@}

#endif
