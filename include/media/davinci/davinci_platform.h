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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef DAVINCI_PLATFORM_H
#define DAVINCI_PLATFORM_H
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <mach/hardware.h>
#include <mach/mux.h>
#include <mach/cputype.h>
#include <media/davinci/davinci_enc.h>
#include <media/davinci/vid_encoder_types.h>
#include <video/davinci_vpbe.h>
#include <media/davinci/davinci_enc_mngr.h>

#ifdef __KERNEL__
extern void enable_lcd(void);
extern void enable_hd_clk(void);
int davinci_enc_select_venc_clock(int clk);
void davinci_enc_set_display_timing(struct vid_enc_mode_info *mode);
void davinci_enc_set_mode_platform(int channel, struct vid_enc_device_mgr *mgr);

u32 venc_reg_in(u32 offset);
u32 venc_reg_out(u32 offset, u32 val);
u32 venc_reg_merge(u32 offset, u32 val, u32 mask);
#endif				/* End of __KERNEL__ */

#endif				/* End of ifndef DAVINCI_PLATFORM_H */
