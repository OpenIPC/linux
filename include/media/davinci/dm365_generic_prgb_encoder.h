/*
 *Copyright (C) 2011 Ridgerun (http://www.ridgerun.com)
 *
 * Author: Natanel Castro <natanael.castro@ridgerun.com>
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
 */

#ifndef GEN_PRGB_ENCODER_H
#define GEN_PRGB_ENCODER_H

#ifdef __KERNEL__
/* Kernel Header files */
#include <linux/i2c.h>
#include <linux/device.h>
#include <media/davinci/vid_encoder_types.h>
#endif

#ifdef __KERNEL__
/* encoder standard related strctures */
#define GEN_PRGB_ENCODER_MAX_NO_OUTPUTS		(1)
#define GEN_PRGB_ENCODER_NUM_STD		(1)


#define PRGB_ENCODER_DRV_NAME		"generic_prgb"

#define DAVINCI_PLL1_RATE			486000		/*PLL1 clock rate (KHz)*/
#define PLL1DIV6_MAX_NUM			31

struct davinci_gen_prgb_pdata {
	unsigned int xres;
	unsigned int yres;
	struct vid_enc_fract fps;
	unsigned int pixel_clock_khz;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int upper_margin;
	unsigned int lower_margin;
	unsigned int hsync_len;
	unsigned int vsync_len;
	unsigned int flags;
	int (*clock_set_function)(unsigned int);
};

struct gen_prgb_encoder_params {
	int outindex;
	char *mode;
};

struct gen_prgb_encoder_config {
	int no_of_outputs;
	struct {
		char *output_name;
		int no_of_standard;
		struct vid_enc_mode_info
		 standards[GEN_PRGB_ENCODER_NUM_STD];
	} output[GEN_PRGB_ENCODER_MAX_NO_OUTPUTS];
};

struct gen_prgb_encoder_channel {
	struct encoder_device *enc_device;
	struct gen_prgb_encoder_params params;
};

#endif				/* End of #ifdef __KERNEL__ */

#endif				/* End of #ifndef GEN_PRGB_ENCODER_H */
