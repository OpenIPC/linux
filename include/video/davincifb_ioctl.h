/*
 * Copyright (C) 2009 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef DAVINCIFB_IOCTL_H
#define DAVINCIFB_IOCTL_H

/*
 * Enumerations
 */

/*  Enum for Boolean variables  */
typedef enum {
	SET_0 = 0,
	SET_1 = 1
} CB_CR_ORDER, ATTRIBUTE;

/*  Enable/Disable enum */
typedef enum {
	VPBE_DISABLE = 0,
	VPBE_ENABLE = 1
} ATTENUATION, EXPANSION, BLINKING;

/*
 * Structures and Union Definitions
 */

typedef struct zoom_params {
	u_int32_t window_id;
	u_int32_t zoom_h;
	u_int32_t zoom_v;
} zoom_params_t;

/* Structure for transparency and the blending factor for the bitmap window */
typedef struct vpbe_bitmap_blend_params {
	unsigned int colorkey;	/* color key to be blended */
	unsigned int enable_colorkeying;	/* enable color keying */
	unsigned int bf;	/* valid range from 0 to 7 only. */
} vpbe_bitmap_blend_params_t;

/*  Structure for window expansion  */
typedef struct vpbe_win_expansion {
	EXPANSION horizontal;
	EXPANSION vertical;	/* 1: Enable 0:disable */
} vpbe_win_expansion_t;

/*  Structure for OSD window blinking options */
typedef struct vpbe_blink_option {
	BLINKING blinking;	/* 1: Enable blinking 0: Disable */
	unsigned int interval;	/* Valid only if blinking is 1 */
} vpbe_blink_option_t;

/*  Structure for background color  */
typedef struct vpbe_backg_color {
	unsigned char clut_select;	/* 2: RAM CLUT 1:ROM1 CLUT 0:ROM0 CLUT */
	unsigned char color_offset;	/* index of color */
} vpbe_backg_color_t;

/*  Structure for Video window configurable parameters  */
typedef struct vpbe_video_config_params {
	CB_CR_ORDER cb_cr_order;	/* Cb/Cr order in input data for a pixel. */
	/*    0: cb cr  1:  cr cb */
	vpbe_win_expansion_t exp_info;	/* HZ/VT Expansion enable disable */
} vpbe_video_config_params_t;

/*
 * Union of structures giving the CLUT index for the 1, 2, 4 bit bitmap values
 */
typedef union vpbe_clut_idx {
	struct _for_4bit_bitmap {
		unsigned char bitmap_val_0;
		unsigned char bitmap_val_1;
		unsigned char bitmap_val_2;
		unsigned char bitmap_val_3;
		unsigned char bitmap_val_4;
		unsigned char bitmap_val_5;
		unsigned char bitmap_val_6;
		unsigned char bitmap_val_7;
		unsigned char bitmap_val_8;
		unsigned char bitmap_val_9;
		unsigned char bitmap_val_10;
		unsigned char bitmap_val_11;
		unsigned char bitmap_val_12;
		unsigned char bitmap_val_13;
		unsigned char bitmap_val_14;
		unsigned char bitmap_val_15;
	} for_4bit_bitmap;
	struct _for_2bit_bitmap {
		unsigned char bitmap_val_0;
		unsigned char dummy0[4];
		unsigned char bitmap_val_1;
		unsigned char dummy1[4];
		unsigned char bitmap_val_2;
		unsigned char dummy2[4];
		unsigned char bitmap_val_3;
	} for_2bit_bitmap;
	struct _for_1bit_bitmap {
		unsigned char bitmap_val_0;
		unsigned char dummy0[14];
		unsigned char bitmap_val_1;
	} for_1bit_bitmap;
} vpbe_clut_idx_t;

/* Structure for bitmap window configurable parameters */
typedef struct vpbe_bitmap_config_params {
	/* Only for bitmap width = 1,2,4 bits */
	vpbe_clut_idx_t clut_idx;
	/* Attenuation value for YUV o/p for bitmap window */
	unsigned char attenuation_enable;
	/* 0: ROM DM270, 1:ROM DM320, 2:RAM CLUT */
	unsigned char clut_select;
} vpbe_bitmap_config_params_t;

/* Structure to hold window position */
typedef struct vpbe_window_position {
	unsigned int xpos;	/* X position of the window */
	unsigned int ypos;	/* Y position of the window */
} vpbe_window_position_t;

#define	RAM_CLUT_SIZE	256*3

/* custom ioctl definitions */
#define FBIO_WAITFORVSYNC	_IOW('F', 0x20, u_int32_t)
#define FBIO_SETATTRIBUTE	_IOW('F', 0x21, struct fb_fillrect)
#define FBIO_SETPOSX		_IOW('F', 0x22, u_int32_t)
#define FBIO_SETPOSY		_IOW('F', 0x23, u_int32_t)
#define FBIO_SETZOOM		_IOW('F', 0x24, struct zoom_params)
#define FBIO_ENABLE_DISABLE_WIN		\
	_IOW('F', 0x30, unsigned char)
#define FBIO_SET_BITMAP_BLEND_FACTOR	\
	_IOW('F', 0x31, vpbe_bitmap_blend_params_t)
#define FBIO_SET_BITMAP_WIN_RAM_CLUT	\
	_IOW('F', 0x32, unsigned char[RAM_CLUT_SIZE])
#define FBIO_ENABLE_DISABLE_ATTRIBUTE_WIN \
	_IOW('F', 0x33, unsigned int)
#define FBIO_GET_BLINK_INTERVAL		\
	_IOR('F', 0x34, vpbe_blink_option_t)
#define FBIO_SET_BLINK_INTERVAL		\
	_IOW('F', 0x35, vpbe_blink_option_t)
#define FBIO_GET_VIDEO_CONFIG_PARAMS	\
	_IOR('F', 0x36, vpbe_video_config_params_t)
#define FBIO_SET_VIDEO_CONFIG_PARAMS	\
	_IOW('F', 0x37, vpbe_video_config_params_t)
#define FBIO_GET_BITMAP_CONFIG_PARAMS	\
	_IOR('F', 0x38, vpbe_bitmap_config_params_t)
#define FBIO_SET_BITMAP_CONFIG_PARAMS	\
	_IOW('F', 0x39, vpbe_bitmap_config_params_t)
#define FBIO_SET_BACKG_COLOR		\
	_IOW('F', 0x47, vpbe_backg_color_t)
#define FBIO_SETPOS			\
	_IOW('F', 0x49, u_int32_t)
#define FBIO_SET_CURSOR			\
	_IOW('F', 0x50, struct fb_cursor)

/*  Window ID definitions */
#define OSD0 0
#define VID0 1
#define OSD1 2
#define VID1 3

#endif				/* ifndef DAVINCIFB_IOCTL_H */
