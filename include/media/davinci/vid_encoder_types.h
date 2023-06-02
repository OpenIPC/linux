/*
 * Copyright (C) 2007 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * vid_encoder_types.h
 * This file contains data types which will be used across encoder manager
 * , encoders and users of both interfaces */

#ifndef VID_ENCODER_TYPES_H
#define VID_ENCODER_TYPES_H

#ifdef __KERNEL__

/**
 * enum vid_enc_init_flag
 * @VID_ENC_I2C_BIND_FLAG: Only I2C bind
 * @VID_ENC_FULL_INIT_FLAG: Full initialization
 *
 * Description:
 * An enumeration of the encoder initialization flag.
 */
enum vid_enc_init_flag {
	VID_ENC_I2C_BIND_FLAG,
	VID_ENC_FULL_INIT_FLAG,
};

/**
 * enum vid_enc_frame_type
 * @VID_ENC_FRAME_INTERLACED: Interlaced frame
 * @VID_ENC_FRAME_PROGRESSIVE: Progressive frame
 *
 * Description:
 * An enumeration of the Frame Formats.
 */
enum vid_enc_frame_type {
	VID_ENC_FRAME_INTERLACED,
	VID_ENC_FRAME_PROGRESSIVE,
};

/** VID_ENC_NAME_MAX_CHARS
 *
 * Description:
 * MAX characters in the name.
 */
#define VID_ENC_NAME_MAX_CHARS	30

/**
 * enum vid_enc_if_type
 * @VID_ENC_IF_INT: for internal DAC
 * @VID_ENC_IF_BT656: for BT656 input format
 * @VID_ENC_IF_BT1120: for BT1120 input format
 * @VID_ENC_IF_YCC8: for YCC8 input format
 * @VID_ENC_IF_YCC16: for YCC16 input format
 * @VID_ENC_IF_SRGB: for SRGB input format
 * @VID_ENC_IF_PRGB: for PRGB input format
 *
 * Description:
 * An enumeration of the encoder interface types.
 *
 */
enum vid_enc_if_type {
	VID_ENC_IF_INT,
	VID_ENC_IF_BT656,
	VID_ENC_IF_BT1120,
	VID_ENC_IF_YCC8,
	VID_ENC_IF_YCC16,
	VID_ENC_IF_SRGB,
	VID_ENC_IF_PRGB,
};

/**
 * constant strings for standard names or mode names. All modules uses this to
 * refer a specific standard or mode name
 */
#define VID_ENC_STD_NTSC         "NTSC"
#define VID_ENC_STD_NTSC_RGB     "NTSC-RGB"
#define VID_ENC_STD_PAL          "PAL"
#define VID_ENC_STD_PAL_RGB      "PAL-RGB"
#define VID_ENC_STD_720P_24      "720P-24"
#define VID_ENC_STD_720P_25      "720P-25"
#define VID_ENC_STD_720P_30      "720P-30"
#define VID_ENC_STD_720P_50      "720P-50"
#define VID_ENC_STD_720P_60      "720P-60"
#define VID_ENC_STD_1080I_25     "1080I-25"
#define VID_ENC_STD_1080I_30     "1080I-30"
#define VID_ENC_STD_1080P_24	 "1080P-24"
#define VID_ENC_STD_1080P_25     "1080P-25"
#define VID_ENC_STD_1080P_30     "1080P-30"
#define VID_ENC_STD_1080P_50     "1080P-50"
#define VID_ENC_STD_1080P_60     "1080P-60"
#define VID_ENC_STD_480P_60      "480P-60"
#define VID_ENC_STD_576P_50      "576P-50"
#define VID_ENC_STD_640x480      "640x480"
#define VID_ENC_STD_640x400      "640x400"
#define VID_ENC_STD_640x350      "640x350"
#define VID_ENC_STD_480x272      "480x272"
#define VID_ENC_STD_800x480      "800x480"
#define VID_ENC_STD_PRGB_DEFAULT "PRGB"
#define VID_ENC_STD_NON_STANDARD "NON-STANDARD"

/**
 * enum vid_enc_ctrl_id
 * @VID_ENC_CTRL_GAIN: Gain control
 * @VID_ENC_CTRL_BRIGHTNESS: Brighness control
 * @VID_ENC_CTRL_CONTRAST: Contrast control
 * @VID_ENC_CTRL_HUE: Hue control
 * @VID_ENC_CTRL_SATURATION: Saturation Control
 *
 * Description:
 * Constants used for control type IDs
*/
enum vid_enc_ctrl_id {
	VID_ENC_CTRL_GAIN,
	VID_ENC_CTRL_BRIGHTNESS,
	VID_ENC_CTRL_CONTRAST,
	VID_ENC_CTRL_HUE,
	VID_ENC_CTRL_SATURATION,
};

/**
 * constant strings for output names. All modules uses this to
 * refer a specific output at the encoder hardware or VPBE analog output.
 * May add additional output names as per following convention. First
 * output of a type is always with out a suffix, and subsequent outputs
 * of the same type is defined with suffix x increasing from 1 to max
 * output names supported as shown below.
 */
#define VID_ENC_OUTPUT_COMPOSITE  "COMPOSITE"
#define VID_ENC_OUTPUT_COMPOSITE1 "COMPOSITE1"
#define VID_ENC_OUTPUT_SVIDEO     "SVIDEO"
#define VID_ENC_OUTPUT_SVIDEO1    "SVIDEO1"
#define	VID_ENC_OUTPUT_COMPONENT  "COMPONENT"
#define	VID_ENC_OUTPUT_COMPONENT1 "COMPONENT1"
#define VID_ENC_OUTPUT_LCD        "LCD"
#define VID_ENC_OUTPUT_LCD1       "LCD1"
#define VID_ENC_OUTPUT_PRGB       "PRGB"
#define VID_ENC_OUTPUT_PRGB1      "PRGB1"

/**
 * struct vid_enc_fract
 * @numerator: numerator part of a fractional number
 * @denominator: denominator part of a fractional number
 *
 * Description:
 *  Structure used to represent fractional numbers
 */
struct vid_enc_fract {
	unsigned int numerator;
	unsigned int denominator;
};

/* Set of macros for sliced vbi services */
#define VID_ENC_SLICED_VBI_WSS_PAL	0x0001
#define VID_ENC_SLICED_VBI_CGMS_NTSC	0x0002
#define VID_ENC_SLICED_VBI_CC_NTSC	0x0004

/**
 * struct vid_enc_sliced_vbi_service
 * @service_set: set of services to be enabled in encoder
 * @line: line number of the field in which data for these
 * services is to be included.
 *
 * Description:
 *  Structure used to enable set of sliced VBI services in encoder
 */
struct vid_enc_sliced_vbi_data {

	unsigned int service_id;
	unsigned char field;
	unsigned char data[48];
};

struct vid_enc_sliced_vbi_service {
	unsigned short service_set;
	unsigned int line[2][23];
};

/**
 * struct vid_enc_mode_info
 * @name: ptr to name string of the standard, "NTSC", "PAL" etc
 * @std: standard or non-standard mode. 1 - standard, 0 - nonstandard
 * @if_type: interface type used for this standard configuration
 * @interlaced: 1 - interlaced, 0 - non interlaced/progressive
 * @xres: x or horizontal resolution of the display
 * @yres: y or vertical resolution of the display
 * @fps: frame per second
 * @left_margin: left margin of the display
 * @right_margin: right margin of the display
 * @upper_margin: upper margin of the display
 * @lower_margin: lower margin of the display
 * @hsync_len: h-sync length
 * @vsync_len: v-sync length
 * @flags: bit field: bit usage is documented below
 *
 * Description:
 *  Structure holding timing and resolution information of a standard.
 *  Notes
 *  ------
 *  if_type should be used only by encoder manager and encoder.
 *  flags usage
 *     b0 (LSB) - hsync polarity, 0 - negative, 1 - positive
 *     b1       - vsync polarity, 0 - negative, 1 - positive
 *     b2       - field id polarity, 0 - negative, 1  - positive
 */
struct vid_enc_mode_info {
	unsigned char *name;
	unsigned int std;
	enum vid_enc_if_type if_type;
	unsigned int interlaced;
	unsigned int xres;
	unsigned int yres;
	struct vid_enc_fract fps;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int upper_margin;
	unsigned int lower_margin;
	unsigned int hsync_len;
	unsigned int vsync_len;
	unsigned int flags;
};

#endif				/* #ifdef __KERNEL__ */

#endif				/* #ifdef VID_ENCODER_TYPES_H */
