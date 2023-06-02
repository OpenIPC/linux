
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
 */

#ifndef DAVINCI_ENC_MNGR_H
#define DAVINCI_ENC_MNGR_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <mach/hardware.h>
#include <mach/mux.h>
#include <mach/cputype.h>
#include <media/davinci/davinci_enc.h>
#include <media/davinci/vid_encoder_types.h>
#include <video/davinci_vpbe.h>

#ifdef __KERNEL__

#define DAVINCI_ENC_MAX_ENCODERS (4)

struct vid_enc_device_mgr {
	struct semaphore lock;
	int num_encoders;	/* number of encoders in this channel */
	int current_encoder;	/* index to current encoder */
	char current_output[VID_ENC_NAME_MAX_CHARS];	/* current output */
	struct vid_enc_mode_info current_mode;	/* current mode */
	struct vid_encoder_device *encoder[DAVINCI_ENC_MAX_ENCODERS];
};

struct enc_config {
	char output[VID_ENC_NAME_MAX_CHARS];
	char mode[VID_ENC_NAME_MAX_CHARS];
};

#endif				/* __KERNEL__ */

#endif
