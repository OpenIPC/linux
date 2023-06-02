/**
    @file       dispcomn.h
    @ingroup    mISYSDisp

    @brief      Header file for Display control General Common Definitions
				This file is the header file that define the API and data type
				for Display control object
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

/** \addtogroup  mISYSDisp*/
//@{

#ifndef _DISPCOMN_H
#define _DISPCOMN_H

#if defined __KERNEL__
#include "kwrap/type.h"
#elif defined __FREERTOS
#include "kwrap/type.h"
#else
#include "Type.h"
#endif



/**
    Display Object ID

    The Display Object ID used in disp_get_display_object() to get the specified display object.
*/
typedef enum {
	DISP_1,     ///< Display ID 1. In the NT98520, this is used to specify IDE1.
	DISP_2,     ///< This is no use in NT98520.

	DISP_MAX = DISP_2,
	ENUM_DUMMY4WORD(DISP_ID)
} DISP_ID;


/**
    Display Layer Selection ID

    This definition is used as the first operand in the disp_lyr_ctrl API to specify which of the display layer would be controlled.
*/
typedef enum {
	DISPLAYER_OSD1 = 0x02,                  ///< select display layer osd 1
	DISPLAYER_OSD2 = 0x04,                  ///< not support
	DISPLAYER_VDO1 = 0x08,                  ///< select display layer video 1
	DISPLAYER_VDO2 = 0x10,                  ///< select display layer video 2
	DISPLAYER_FD   = 0x20,                  ///< select display layer FD
	DISPLAYER_MASK = 0x3A,                  ///< Valid display layer selection mask

	ENUM_DUMMY4WORD(DISPLAYER)
} DISPLAYER;



/**
    @name Display/Layer Ability Definition Group

    These definitions are used in the function get_disp_cap of the display object to get the display object capabilities.
*/
//@{

/**
    Display Object Ability Definition

    These bit fields are used in DISP_CAP to specified the general abilities of each display object.
    Such as whether the display supporting CSB/SubPixel/Dithering/ICST/TV_OUT/HDMI_OUT ... or not.
*/
typedef enum {
	//DISPABI_DRAM_DIRECT     = 0x40000000,   ///< The display engine supports direct DRAM input and output to panel interface bus.

	DISPABI_SUBPIXEL        = 0x00000001,   ///< The display engine supports SubPixel functionality.
	DISPABI_CSB             = 0x00000002,   ///< The display engine supports Contrast/Saturation/Brightness adjustment.
	DISPABI_GAMMA           = 0x00000004,   ///< The display engine supports Y Gamma adjustment.
	DISPABI_HLPF            = 0x00000008,   ///< The display engine supports Horizontal Low Pass filtering.
	DISPABI_DITHER          = 0x00000010,   ///< The display engine supports Dithering functionality.
	DISPABI_ICST            = 0x00000020,   ///< The display engine supports Inverse color Space Transform from YUV to RGB.
	DISPABI_CC              = 0x00000040,   ///< The display engine supports color control.
	DISPABI_RGBGAMMA        = 0x00000080,   ///< The display engine supports  RGB Gamma adjustment.
	DISPABI_FD              = 0x00000100,   ///< The display engine supports FD rect.

	DISPABI_TVOUT           = 0x00010000,   ///< The display engine supports interface TV NTSC/PAL out.
	DISPABI_MIOUT           = 0x00020000,   ///< The display engine supports interface Memory Interface out.
	DISPABI_HDMIOUT         = 0x00040000,   ///< The display engine supports interface HDMI output.
	DISPABI_RGBSEROUT8      = 0x00080000,   ///< The display engine supports interface RGB Serial 8bits out.
	DISPABI_RGBPARALLEL     = 0x00100000,   ///< The display engine supports interface RGB Parallel out.
	DISPABI_YUVSEROUT8      = 0x00200000,   ///< The display engine supports interface YUV Serial 8bits out.
	DISPABI_CCIR601OUT8     = 0x00400000,   ///< The display engine supports interface CCIR601 8bits out.
	DISPABI_CCIR656OUT8     = 0x00800000,   ///< The display engine supports interface CCIR656 8bits out.
	DISPABI_CCIR601OUT16    = 0x01000000,   ///< The display engine supports interface CCIR601 16bits out.
	DISPABI_CCIR656OUT16    = 0x02000000,   ///< The display engine supports interface CCIR656 16bits out.
	DISPABI_MIPIDSI         = 0x04000000,   ///< The display engine supports interface MIPI DSI out.
	DISPABI_RGBDELTA16      = 0x08000000,   ///< The display engine supports interface RGB Delta 16bits out.

	ENUM_DUMMY4WORD(DISP_ABI)
} DISP_ABI;

/**
    Display Layer Ability Definition

    These bit fields are used in DISP_CAP to specified the abilities of each display layer.
    Such as whether the display layer supporting YUV444/YUV422/YUV420/Palette8/ARGB8565 ... formats.
*/
typedef enum {
	DISPLYR_EXIST       = 0x40000000,   ///< This display layer is existed.

	DISPLYR_YUV444      = 0x00000001,   ///< This display layer suppors Buffer format YUV444.
	DISPLYR_YUV422      = 0x00000002,   ///< This display layer suppors Buffer format YUV422.
	DISPLYR_YUV420      = 0x00000004,   ///< This display layer suppors Buffer format YUV420.

	DISPLYR_ARGB8565    = 0x00000010,   ///< This display layer suppors Buffer format ARGB8565.
	DISPLYR_ARGB4565    = 0x00000020,   ///< This display layer suppors Buffer format ARGB4565.
	DISPLYR_RGB888PACK  = 0x00000040,   ///< This display layer suppors Buffer format RGB888 Packed.

	DISPLYR_PALE1       = 0x00000100,   ///< This display layer suppors Buffer format 1bit Palette.
	DISPLYR_PALE2       = 0x00000200,   ///< This display layer suppors Buffer format 2bit Palette.
	DISPLYR_PALE4       = 0x00000400,   ///< This display layer suppors Buffer format 4bit Palette.
	DISPLYR_PALE8       = 0x00000800,   ///< This display layer suppors Buffer format 8bit Palette.

	DISPLYR_YUV422PACK  = 0x00001000,   ///< This display layer suppors Buffer format YUV422 UVPACK.
	DISPLYR_YUV420PACK  = 0x00002000,   ///< This display layer suppors Buffer format YUV420 UVPACK.

	ENUM_DUMMY4WORD(DISP_LAYER_ABI)
} DISP_LAYER_ABI;

/**
    Display Capability Structure

    This structure is used to describe the display capabilities such as display engine abilities
    and the abilities for each of the display layer.
*/
typedef struct {
	DISP_ABI        display_abi;         ///< This field describes the general abilities of display engine. Please refer to the enumeration type of DISP_ABI.

	DISP_LAYER_ABI  video1_abi;          ///< This field describes the abilities of VIDEO-1 layer. Please refer to the enumeration type of DISP_LAYER_ABI.
	DISP_LAYER_ABI  video2_abi;          ///< This field describes the abilities of VIDEO-2 layer. Please refer to the enumeration type of DISP_LAYER_ABI.
	DISP_LAYER_ABI  osd1_abi;            ///< This field describes the abilities of OSD-1 layer. Please refer to the enumeration type of DISP_LAYER_ABI.
	DISP_LAYER_ABI  osd2_abi;            ///< This field describes the abilities of OSD-2 layer. Please refer to the enumeration type of DISP_LAYER_ABI.

} DISP_CAP, *PDISP_CAP;
//@}


#define DISPCMDBASE_DISPCTRL    0x01000000
#define DISPCMDBASE_DISPLYR     0x02000000
#define DISPCMDBASE_DISPDEV     0x04000000
#define DISPCMDBASE_IOCTRL      0x08000000



#endif
//@}
