/*
 * Copyright (C) 2009 Texas Instruments Inc
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
/* vpbe_encoder.c.. For internal encoder of the VPBE */

/* Kernel Specific header files */

#include <linux/kernel.h>
//#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <video/davinci_vpbe.h>
#include <media/davinci/vid_encoder_if.h>
#include <media/davinci/vpbe_encoder.h>
#include <media/davinci/davinci_platform.h>
#include <mach/hardware.h>
#include <linux/io.h>
#include <mach/cputype.h>

#include <asm/io.h>

struct vpbe_encoder_params {
	int outindex;
	char *mode;
};

struct vpbe_encoder_config {
//	unsigned long vencregs;
	int no_of_outputs;
	struct {
		char *output_name;
		int no_of_standard;
		char *standards[VPBE_ENCODER_MAX_NUM_STD];
	} output[VPBE_ENCODER_MAX_NO_OUTPUTS];
};

struct vpbe_encoder_channel {
	struct encoder_device *enc_device;
	struct vpbe_encoder_params params;
};

/* Function prototypes */
static int vpbe_encoder_initialize(struct vid_encoder_device *enc, int flag);
static int vpbe_encoder_deinitialize(struct vid_encoder_device *enc);

static int vpbe_encoder_setmode(struct vid_enc_mode_info *mode_info,
				struct vid_encoder_device *enc);
static int vpbe_encoder_getmode(struct vid_enc_mode_info *mode_info,
				struct vid_encoder_device *enc);

static int vpbe_encoder_setoutput(char *output, struct vid_encoder_device *enc);
static int vpbe_encoder_getoutput(char *output, struct vid_encoder_device *enc);

static int vpbe_encoder_enumoutput(int index,
				   char *output,
				   struct vid_encoder_device *enc);

static int vpbe_encoder_enable(int flag, struct vid_encoder_device *enc);

/* All Supported encoder modes */
static struct vid_enc_mode_info vpbe_encoder_modes[VPBE_ENCODER_MAX_NUM_STD] = {
	{
	 .name = VID_ENC_STD_NTSC,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 1,
	 .xres = 720,
	 .yres = 480,
	 .fps = {30000, 1001},
	 .left_margin = 0x79,
	 .right_margin = 0,
	 .upper_margin = 0x10,
	 .lower_margin = 0,
	 .hsync_len = 0,
	 .vsync_len = 0,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_PAL,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 1,
	 .xres = 720,
	 .yres = 576,
	 .fps = {25, 1},
	 .left_margin = 0x7E,
	 .right_margin = 0,
	 .upper_margin = 0x16,
	 .lower_margin = 0,
	 .hsync_len = 0,
	 .vsync_len = 0,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_NTSC_RGB,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 1,
	 .xres = 720,
	 .yres = 480,
	 .fps = {30000, 1001},
	 .left_margin = 0x80,
	 .right_margin = 0,
	 .upper_margin = 0x12,
	 .lower_margin = 0,
	 .hsync_len = 0,
	 .vsync_len = 0,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_PAL_RGB,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 1,
	 .xres = 720,
	 .yres = 576,
	 .fps = {25, 1},
	 .left_margin = 0x88,
	 .right_margin = 0,
	 .upper_margin = 0x18,
	 .lower_margin = 0,
	 .hsync_len = 0,
	 .vsync_len = 0,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_480P_60,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 720,
	 .yres = 480,
	 .fps = {60, 1},
	 .left_margin = 0x80,
	 .right_margin = 0,
	 .upper_margin = 0x20,
	 .lower_margin = 0,
	 .hsync_len = 0,
	 .vsync_len = 0,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_576P_50,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 720,
	 .yres = 576,
	 .fps = {50, 1},
	 .left_margin = 0x7E,
	 .right_margin = 0,
	 .upper_margin = 0x30,
	 .lower_margin = 0,
	 .hsync_len = 0,
	 .vsync_len = 0,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_720P_60,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 1280,
	 .yres = 720,
	 .fps = {60, 1},
	 .left_margin = 300,
	 .right_margin = 70,
	 .upper_margin = 26,
	 .lower_margin = 3,
	 .hsync_len = 80,
	 .vsync_len = 5,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_720P_50,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 1280,
	 .yres = 720,
	 .fps = {50, 1},
	 .left_margin = 300,
	 .right_margin = 70,
	 .upper_margin = 26,
	 .lower_margin = 3,
	 .hsync_len = 80,
	 .vsync_len = 5,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_720P_30,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 1280,
	 .yres = 720,
	 .fps = {30, 1},
	 .left_margin = 300,
	 .right_margin = 70,
	 .upper_margin = 26,
	 .lower_margin = 3,
	 .hsync_len = 80,
	 .vsync_len = 5,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_720P_25,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 1280,
	 .yres = 720,
	 .fps = {25, 1},
	 .left_margin = 300,
	 .right_margin = 70,
	 .upper_margin = 26,
	 .lower_margin = 3,
	 .hsync_len = 80,
	 .vsync_len = 5,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_720P_24,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 0,
	 .xres = 1280,
	 .yres = 720,
	 .fps = {24, 1},
	 .left_margin = 300,
	 .right_margin = 70,
	 .upper_margin = 26,
	 .lower_margin = 3,
	 .hsync_len = 80,
	 .vsync_len = 5,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_1080I_30,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 1,
	 .xres = 1920,
	 .yres = 1080,
	 .fps = {30, 1},
	 .left_margin = 200,
	 .right_margin = 80,
	 .upper_margin = 13,
	 .lower_margin = 31,
	 .hsync_len = 88,
	 .vsync_len = 5,
	 .flags = 0},
	{
	 .name = VID_ENC_STD_1080I_25,
	 .std = 1,
	 .if_type = VID_ENC_IF_INT,
	 .interlaced = 1,
	 .xres = 1920,
	 .yres = 1080,
	 .fps = {25, 1},
	 .left_margin = 200,
	 .right_margin = 80,
	 .upper_margin = 13,
	 .lower_margin = 31,
	 .hsync_len = 88,
	 .vsync_len = 5,
	 .flags = 0},
};

static struct vpbe_encoder_config vpbe_encoder_configuration = {
//	.vencregs = DM644X_VENC_REG_BASE,
	.no_of_outputs = VPBE_DM644X_ENCODER_MAX_NO_OUTPUTS,
	.output[0] = {
		      .output_name = VID_ENC_OUTPUT_COMPOSITE,
		      .no_of_standard = VPBE_DM644X_ENCODER_COMPOSITE_NUM_STD,
		      .standards = {VID_ENC_STD_NTSC, VID_ENC_STD_PAL},
		      },
	.output[1] = {
		      .output_name = VID_ENC_OUTPUT_SVIDEO,
		      .no_of_standard = VPBE_DM644X_ENCODER_SVIDEO_NUM_STD,
		      .standards = {VID_ENC_STD_NTSC, VID_ENC_STD_PAL},
		      },
	.output[2] = {
		      .output_name = VID_ENC_OUTPUT_COMPONENT,
		      .no_of_standard = VPBE_DM644X_ENCODER_COMPONENT_NUM_STD,
		      .standards = {VID_ENC_STD_NTSC,
				    VID_ENC_STD_PAL,
				    VID_ENC_STD_NTSC_RGB,
				    VID_ENC_STD_PAL_RGB,
				    VID_ENC_STD_480P_60,
				    VID_ENC_STD_576P_50},
		      }
};

static struct vpbe_encoder_channel vpbe_encoder_channel_info = {
	.params.outindex = 0,
	.params.mode = VID_ENC_STD_NTSC,
	.enc_device = NULL
};

static struct vid_enc_output_ops outputs_ops = {
	.count = VPBE_ENCODER_MAX_NO_OUTPUTS,
	.enumoutput = vpbe_encoder_enumoutput,
	.setoutput = vpbe_encoder_setoutput,
	.getoutput = vpbe_encoder_getoutput
};

static struct vid_enc_mode_ops modes_ops = {
	.setmode = vpbe_encoder_setmode,
	.getmode = vpbe_encoder_getmode,
};

static struct vid_enc_misc_ops miscs_ops = {
	.reset = NULL,
	.enable = vpbe_encoder_enable,
};

static struct vid_encoder_device vpbe_encoder_dev = {
	.name = "VPBE_ENCODER",
	.capabilities = 0,
	.initialize = vpbe_encoder_initialize,
	.mode_ops = &modes_ops,
	.ctrl_ops = NULL,
	.output_ops = &outputs_ops,
	.params_ops = NULL,
	.misc_ops = &miscs_ops,
	.deinitialize = vpbe_encoder_deinitialize,
};

/*
 * display controller register I/O routines
 */
#if 0
static __inline__ u32 venc_reg_in(u32 offset)
{
	u32 addr;
	addr = vpbe_encoder_configuration.vencregs + offset;
	return (__raw_readl(IO_ADDRESS(addr)));
}
static __inline__ u32 venc_reg_out(u32 offset, u32 val)
{
	u32 addr = vpbe_encoder_configuration.vencregs + offset;
	__raw_writel(val, IO_ADDRESS(addr));
	return (val);
}
static __inline__ u32 venc_reg_merge(u32 offset, u32 val, u32 mask)
{
	u32 addr, new_val;
	addr = vpbe_encoder_configuration.vencregs + offset;
	new_val = (__raw_readl(IO_ADDRESS(addr)) & ~mask) | (val & mask);
	__raw_writel(new_val, IO_ADDRESS(addr));
	return (new_val);
}
#endif

/* Function to enable/disable output */
static int vpbe_encoder_enable(int flag, struct vid_encoder_device *enc)
{
	if (flag)
		venc_reg_out(VENC_DACTST, 0);
	else
		venc_reg_out(VENC_DACTST, 0xF000);
	return 0;
}

/* This function sets the dac of the VPBE for various outputs
 */
static int vpbe_encoder_set_dac(char *output)
{
	int error = 0;

	if (cpu_is_davinci_dm644x() || cpu_is_davinci_dm365()) {
		if (!strcmp(output, VID_ENC_OUTPUT_COMPOSITE)) {
			printk(KERN_DEBUG "Setting output to Composite\n");
			venc_reg_out(VENC_DACSEL, 0);
		} else if (!strcmp(output, VID_ENC_OUTPUT_SVIDEO)) {
			printk(KERN_DEBUG "Setting output to S-Video\n");
			venc_reg_out(VENC_DACSEL, 0x210);
		} else if (!strcmp(output, VID_ENC_OUTPUT_COMPONENT)) {
			printk(KERN_DEBUG
			       "Setting output to Component Video\n");
			venc_reg_out(VENC_DACSEL, 0x543);
		} else
			error = -1;
	} else if (!cpu_is_davinci_dm355())
		error = -1;
	return error;
}

/* This function is called by the encoder manager to initialize vpbe encoder driver.
 * It initializes all registers of vpbe_encoder with the default values
 */
static int vpbe_encoder_initialize(struct vid_encoder_device *enc, int flag)
{
	int err = 0, outindex;
	char *std, *output;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	vpbe_encoder_channel_info.enc_device = (struct encoder_device *)enc;

	/* call set standard */
	std = vpbe_encoder_channel_info.params.mode;
	outindex = vpbe_encoder_channel_info.params.outindex;
	output = vpbe_encoder_configuration.output[outindex].output_name;
	err |= vpbe_encoder_setoutput(output, enc);
	if (err < 0) {
		err = -EINVAL;
		printk(KERN_ERR "Error occured in setoutput\n");
		vpbe_encoder_deinitialize(enc);
		return err;
	}
	printk(KERN_DEBUG "VPBE Encoder initialized\n");
	return err;
}

static int vpbe_encoder_deinitialize(struct vid_encoder_device *enc)
{
	if (NULL == enc) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	if (cpu_is_davinci_dm644x() || cpu_is_davinci_dm365())
		venc_reg_out(VENC_DACSEL, 0);

	/* disable output */
	venc_reg_out(VENC_DACTST, 0xF000);
	venc_reg_out(VENC_VMOD, 0);

	vpbe_encoder_channel_info.enc_device = NULL;
	printk(KERN_DEBUG "VPBE Encoder de-initialized\n");
	return 0;
}

/* Following function returns ptr to a mode_info structure*/
static struct vid_enc_mode_info *get_modeinfo(char *mode_name)
{
	int i;
	for (i = 0; i < VPBE_ENCODER_MAX_NUM_STD; i++) {
		if (!strcmp(vpbe_encoder_modes[i].name, mode_name)) {
			return &vpbe_encoder_modes[i];
		}
	}
	return NULL;
}

/* Following function is used to set the mode*/
static int vpbe_encoder_setmode(struct vid_enc_mode_info *mode_info,
				struct vid_encoder_device *enc)
{
	int err = 0, outindex, i, dm6446 = 0, dm355 = 0, dm365 = 0;
	char *mode;
	struct vid_enc_mode_info *my_mode_info;

	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == (mode = mode_info->name)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "Start of vpbe_encoder_setmode..\n");
	outindex = vpbe_encoder_channel_info.params.outindex;
	if (cpu_is_davinci_dm644x())
		dm6446 = 1;
	else if (cpu_is_davinci_dm355())
		dm355 = 1;
	else if (cpu_is_davinci_dm365())
		dm365 = 1;
	else
		return -EINVAL;

	if (mode_info->std) {
		char *mymode = NULL;
		/* This is a standard mode */
		for (i = 0;
		     i <
		     vpbe_encoder_configuration.output[outindex].no_of_standard;
		     i++) {
			if (!strcmp
			    (vpbe_encoder_configuration.output[outindex].
			     standards[i], mode)) {
				mymode =
				    vpbe_encoder_configuration.output[outindex].
				    standards[i];
				break;
			}
		}
		if ((i ==
		     vpbe_encoder_configuration.output[outindex].no_of_standard)
		    || (NULL == mymode)) {
			printk(KERN_ERR "Invalid id...\n");
			return -EINVAL;
		}
		/* Store the standard in global object of vpbe_encoder */
		vpbe_encoder_channel_info.params.mode = mymode;
		if (!strcmp(mymode, VID_ENC_STD_NTSC)) {
			/* Setup NTSC */
			venc_reg_out(VENC_VMOD, 0);
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					(0 << VENC_VMOD_VMD), VENC_VMOD_VMD);
			venc_reg_merge(VENC_VMOD,
					(0 << VENC_VMOD_TVTYP_SHIFT),
					VENC_VMOD_TVTYP);
		} else if (!strcmp(mymode, VID_ENC_STD_PAL)) {
			/* Setup PAL */
			venc_reg_out(VENC_VMOD, 0);
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					(0 << VENC_VMOD_VMD), VENC_VMOD_VMD);
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_TVTYP_SHIFT),
					VENC_VMOD_TVTYP);
		} else if (!strcmp(mymode, VID_ENC_STD_NTSC_RGB) &&
			   (dm6446 || dm365)) {
			/* Setup for NTSC RGB */
			venc_reg_out(VENC_VMOD, 0);
			venc_reg_merge(VENC_VMOD,
					(0 << VENC_VMOD_VMD), VENC_VMOD_VMD);
			venc_reg_merge(VENC_VMOD,
					(0 << VENC_VMOD_TVTYP_SHIFT),
					VENC_VMOD_TVTYP);
			venc_reg_merge(VENC_CMPNT,
					(1 << VENC_CMPNT_MRGB_SHIFT),
					VENC_CMPNT_MRGB);
		} else if (!strcmp(mymode, VID_ENC_STD_PAL_RGB) &&
			   (dm6446 || dm365)) {
			/* Setup for PAL RGB */
			venc_reg_out(VENC_VMOD, 0);
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					(0 << VENC_VMOD_VMD), VENC_VMOD_VMD);
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_TVTYP_SHIFT),
					VENC_VMOD_TVTYP);
			venc_reg_merge(VENC_CMPNT,
					(1 << VENC_CMPNT_MRGB_SHIFT),
					VENC_CMPNT_MRGB);
		} else if (!strcmp(mymode, VID_ENC_STD_480P_60) &&
			   (dm6446 || dm365)) {
			/* Setup for 480P, Progressive NTSC */
			venc_reg_out(VENC_VMOD, 0);
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					VENC_VMOD_HDMD, VENC_VMOD_HDMD);
			venc_reg_merge(VENC_VMOD,
					(HDTV_525P << VENC_VMOD_TVTYP_SHIFT),
					VENC_VMOD_TVTYP);

		} else if (!strcmp(mymode, VID_ENC_STD_576P_50) &&
			   (dm6446 || dm365)) {
			venc_reg_out(VENC_VMOD, 0);
			/* Setup for 576P, Progressive PAL */
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					VENC_VMOD_HDMD, VENC_VMOD_HDMD);
			venc_reg_merge
			    (VENC_VMOD,
			     (HDTV_625P << VENC_VMOD_TVTYP_SHIFT),
			     VENC_VMOD_TVTYP);
		} else if (!strncmp(mymode, VID_ENC_STD_720P_60, 5) && dm365) {
			char buf[16];
			unsigned long xh, val;
			strcpy(buf, mymode + 5);
			venc_reg_out(VENC_VMOD, 0);
			/* DM365 component HD mode */
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					VENC_VMOD_HDMD, VENC_VMOD_HDMD);
			venc_reg_merge
			    (VENC_VMOD,
			     (HDTV_720P << VENC_VMOD_TVTYP_SHIFT),
			     VENC_VMOD_TVTYP);
			venc_reg_merge
			    (VENC_VMOD,
			     VENC_VMOD_VENC,
			     VENC_VMOD_VENC);
			/* DM365 VENC spec, Table 16 */
			if (strict_strtoul(buf, 10, &val) != 0)
				val = 60;
			switch (val) {
			case 60:
			default:
				xh = 0;
				break;
			case 50:
				xh = 330;
				break;
			case 30:
				xh = 1650;
				break;
			case 25:
				xh = 2310;
				break;
			case 24:
				xh = 2475;
				break;
			}
			venc_reg_out(VENC_XHINTVL, xh);
		} else if (!strncmp(mymode, VID_ENC_STD_1080I_30, 6) && dm365) {
			char buf[16];
			unsigned long xh, val;
			strcpy(buf, mymode + 6);
			venc_reg_out(VENC_VMOD, 0);
			/* DM365 component HD mode */
			venc_reg_merge(VENC_VMOD,
					(1 << VENC_VMOD_VIE_SHIFT),
					VENC_VMOD_VIE);
			venc_reg_merge(VENC_VMOD,
					VENC_VMOD_HDMD, VENC_VMOD_HDMD);
			venc_reg_merge
			    (VENC_VMOD,
			     (HDTV_1080I << VENC_VMOD_TVTYP_SHIFT),
			     VENC_VMOD_TVTYP);
			venc_reg_merge
			    (VENC_VMOD,
			     VENC_VMOD_VENC,
			     VENC_VMOD_VENC);
			if (strict_strtoul(buf, 10, &val) != 0)
				val = 30;
			switch (val) {
			case 30:
			default:
				xh = 0;
				break;
			case 25:
				xh = 2640;
				break;
			}
			venc_reg_out(VENC_XHINTVL, xh);
		} else {
			printk(KERN_ERR "Mode not supported..\n");
			return -EINVAL;
		}
	} else {
		/* Non- Standard mode. Check if we support it. If so
		   save the timing info and return */
		my_mode_info = get_modeinfo(VID_ENC_STD_NON_STANDARD);
		if (my_mode_info) {
			/* We support. So save timing info and return success
			   interface type is same as what is currently is active
			 */
			vpbe_encoder_channel_info.params.mode =
			    VID_ENC_STD_NON_STANDARD;
			my_mode_info->std = 0;
			my_mode_info->interlaced = mode_info->interlaced;
			my_mode_info->xres = mode_info->xres;
			my_mode_info->yres = mode_info->yres;
			my_mode_info->fps = mode_info->fps;
			my_mode_info->left_margin = mode_info->left_margin;
			my_mode_info->right_margin = mode_info->right_margin;
			my_mode_info->upper_margin = mode_info->upper_margin;
			my_mode_info->lower_margin = mode_info->lower_margin;
			my_mode_info->hsync_len = mode_info->hsync_len;
			my_mode_info->vsync_len = mode_info->vsync_len;
			my_mode_info->flags = mode_info->flags;
			/* If we need to configure something in the encoder module, we need to
			   do this here */
			return 0;
		}
		printk(KERN_ERR "Mode not supported..\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "</vpbe_encoder_setmode>\n");
	return err;
}

/* Following function is used to get currently selected mode.*/
static int vpbe_encoder_getmode(struct vid_enc_mode_info *mode_info,
				struct vid_encoder_device *enc)
{
	int err = 0;
	struct vid_enc_mode_info *my_mode_info;
	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "<vpbe_encoder_getmode>\n");
	my_mode_info = get_modeinfo(vpbe_encoder_channel_info.params.mode);
	if (NULL == my_mode_info) {
		printk(KERN_ERR "NULL Pointer for current mode info\n");
		return -EINVAL;
	}
	memcpy(mode_info, my_mode_info, sizeof(struct vid_enc_mode_info));
	printk(KERN_DEBUG "<vpbe_encoder_getmode/>\n");
	return err;
}

/* Following function is used to set output format in VPBE DAC. The
   output name is  passed as the argument to this function. */
static int vpbe_encoder_setoutput(char *output, struct vid_encoder_device *enc)
{
	int err = 0, index;
	struct vid_enc_mode_info *my_mode_info;
	printk(KERN_DEBUG "<vpbe_encoder_setoutput>\n");
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}

	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output: NULL Pointer.\n");
		return -EINVAL;
	}

	for (index = 0; index < vpbe_encoder_configuration.no_of_outputs;
	     index++) {
		if (!strcmp
		    (output,
		     vpbe_encoder_configuration.output[index].output_name)) {
			break;
		}
	}

	if (index == vpbe_encoder_configuration.no_of_outputs) {
		/* No output matching this name */
		printk(KERN_ERR "No matching output: %s\n", output);
		return -EINVAL;
	}

	if (vpbe_encoder_set_dac(output) < 0) {
		printk(KERN_ERR
		       "<vpbe_encoder_setoutput, error in setting DAC config>\n");
		return -EINVAL;
	}

	vpbe_encoder_channel_info.params.outindex = index;

	/* set default standard */
	vpbe_encoder_channel_info.params.mode
	    = vpbe_encoder_configuration.output[index].standards[0];

	my_mode_info = get_modeinfo(vpbe_encoder_channel_info.params.mode);
	if (NULL == my_mode_info) {
		printk(KERN_ERR "No matching mode_info entry found\n");
		return -EINVAL;
	}

	err |= vpbe_encoder_setmode(my_mode_info, enc);
	if (err < 0) {
		printk(KERN_ERR "Erron in setting default mode\n");
		return err;
	}
	printk(KERN_DEBUG "</vpbe_encoder_setoutput>\n");
	return err;
}

/* Following function is used to get output name of current output.*/
static int vpbe_encoder_getoutput(char *output, struct vid_encoder_device *enc)
{
	int err = 0, index, len;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "<vpbe_encoder_getoutput>\n");
	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output:NULL Pointer.\n");
		return -EINVAL;
	}
	index = vpbe_encoder_channel_info.params.outindex;
	len = strlen(vpbe_encoder_configuration.output[index].output_name);
	if (len > (VID_ENC_NAME_MAX_CHARS - 1))
		len = VID_ENC_NAME_MAX_CHARS - 1;
	strncpy(output, vpbe_encoder_configuration.output[index].output_name,
		len);
	output[len] = '\0';
	printk(KERN_DEBUG "</vpbe_encoder_getoutput>\n");
	return err;
}

/* Following function is used to enumerate outputs supported by the driver.
   It fills in information about the output in the output. */
static int vpbe_encoder_enumoutput(int index, char *output,
				   struct vid_encoder_device *enc)
{
	int err = 0;

	printk(KERN_DEBUG "<vpbe_encoder_enumoutput>\n");
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer.\n");
		return -EINVAL;
	}
	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output:NULL Pointer.\n");
		return -EINVAL;
	}
	/* Only one output is available */
	if (index >= vpbe_encoder_configuration.no_of_outputs) {
		return -EINVAL;
	}
	strncpy(output,
		vpbe_encoder_configuration.output[index].output_name,
		VID_ENC_NAME_MAX_CHARS);
	printk(KERN_DEBUG "</vpbe_encoder_enumoutput>\n");
	return err;
}

/* This function used to initialize the vpbe encoder driver */
static int vpbe_encoder_init(void)
{
	int err = 0;

	if (cpu_is_davinci_dm644x()) {
		/* Do nothing. We have everything setup for DM6446 */
	} else if (cpu_is_davinci_dm355()) {
		outputs_ops.count = VPBE_DM355_ENCODER_MAX_NO_OUTPUTS;
//		vpbe_encoder_configuration.vencregs = DM355_VENC_REG_BASE;
		vpbe_encoder_configuration.no_of_outputs =
		    VPBE_DM355_ENCODER_MAX_NO_OUTPUTS;
		vpbe_encoder_configuration.output[0].no_of_standard =
		    VPBE_DM355_ENCODER_COMPOSITE_NUM_STD;
		/* If we have different no of standards for composite in
		 * DM355 and DM6446 we need to fill the stanard names as
		 * well here. Now both are the same.
		 */
	} else if (cpu_is_davinci_dm365()) {
		outputs_ops.count = VPBE_DM365_ENCODER_MAX_NO_OUTPUTS;
//		vpbe_encoder_configuration.vencregs = DM365_VENC_REG_BASE;
		vpbe_encoder_configuration.no_of_outputs =
		    VPBE_DM365_ENCODER_MAX_NO_OUTPUTS;
		vpbe_encoder_configuration.output[0].no_of_standard =
		    VPBE_DM365_ENCODER_COMPOSITE_NUM_STD;
		vpbe_encoder_configuration.output[1].no_of_standard =
		    VPBE_DM365_ENCODER_COMPONENT_NUM_STD;
		vpbe_encoder_configuration.output[1].output_name =
		    VID_ENC_OUTPUT_COMPONENT;
		vpbe_encoder_configuration.output[1].standards[0] =
		    VID_ENC_STD_480P_60;
		vpbe_encoder_configuration.output[1].standards[1] =
		    VID_ENC_STD_576P_50;
		vpbe_encoder_configuration.output[1].standards[2] =
		    VID_ENC_STD_720P_24;
		vpbe_encoder_configuration.output[1].standards[3] =
		    VID_ENC_STD_720P_25;
		vpbe_encoder_configuration.output[1].standards[4] =
		    VID_ENC_STD_720P_30;
		vpbe_encoder_configuration.output[1].standards[5] =
		    VID_ENC_STD_720P_50;
		vpbe_encoder_configuration.output[1].standards[6] =
		    VID_ENC_STD_720P_60;
		vpbe_encoder_configuration.output[1].standards[7] =
		    VID_ENC_STD_1080I_25;
		vpbe_encoder_configuration.output[1].standards[8] =
		    VID_ENC_STD_1080I_30;
  } else
		return -1;

	err = vid_enc_register_encoder(&vpbe_encoder_dev);
	printk(KERN_NOTICE "VPBE Encoder Initialized\n");
	return err;
}

/* Function used to cleanup vpbe encoder driver */
static void vpbe_encoder_cleanup(void)
{
	vid_enc_unregister_encoder(&vpbe_encoder_dev);
}

subsys_initcall(vpbe_encoder_init);
module_exit(vpbe_encoder_cleanup);

MODULE_LICENSE("GPL");
