/*
 * Copyright (C) 2008 Texas Instruments Inc
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
/* logicpd_encoder.c. This is just a place holder for hardcoding all supported
   modes timing. LogicPD timing signals are programmed by the encoder manager
   based on this data.
 */

/* Kernel Specific header files */

#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <media/davinci/vid_encoder_if.h>
#include <media/davinci/avnetlcd_encoder.h>

/* Function prototypes */
static int avnetlcd_encoder_initialize(struct vid_encoder_device *enc,
				       int flag);
static int avnetlcd_encoder_deinitialize(struct vid_encoder_device *enc);

static int avnetlcd_encoder_setmode(struct vid_enc_mode_info *mode_info,
				    struct vid_encoder_device *enc);
static int avnetlcd_encoder_getmode(struct vid_enc_mode_info *mode_info,
				    struct vid_encoder_device *enc);

static int avnetlcd_encoder_setoutput(char *output,
				      struct vid_encoder_device *enc);
static int avnetlcd_encoder_getoutput(char *output,
				      struct vid_encoder_device *enc);

static int avnetlcd_encoder_enumoutput(int index,
				       char *output,
				       struct vid_encoder_device *enc);

static struct avnetlcd_encoder_config avnetlcd_encoder_configuration = {
	.no_of_outputs = AVNETLCD_ENCODER_MAX_NO_OUTPUTS,
	.output[0] = {
		      .output_name = VID_ENC_OUTPUT_LCD1,
		      .no_of_standard = AVNETLCD_ENCODER_GRAPHICS_NUM_STD,
		      .standards[0] = {
				       .name = VID_ENC_STD_800x480,
				       .std = 1,
				       .if_type = VID_ENC_IF_PRGB,
				       .interlaced = 0,
				       .xres = 800,
				       .yres = 480,
				       .fps = {60, 1},
				       .left_margin = 85,
				       .right_margin = 70,
				       .upper_margin = 32,
				       .lower_margin = 11,
				       .hsync_len = 9,
				       .vsync_len = 9,
				       .flags = 0},
		      },	/* hsync -ve, vsync -ve */
};

static struct avnetlcd_encoder_channel avnetlcd_encoder_channel_info = {
	.params.outindex = 0,
	.params.mode = VID_ENC_STD_800x480,
	.enc_device = NULL
};

static struct vid_enc_output_ops outputs_ops = {
	.count = AVNETLCD_ENCODER_MAX_NO_OUTPUTS,
	.enumoutput = avnetlcd_encoder_enumoutput,
	.setoutput = avnetlcd_encoder_setoutput,
	.getoutput = avnetlcd_encoder_getoutput
};

static struct vid_enc_mode_ops modes_ops = {
	.setmode = avnetlcd_encoder_setmode,
	.getmode = avnetlcd_encoder_getmode,
};

static struct vid_encoder_device avnetlcd_encoder_dev = {
	.name = "AVNETLCD_ENCODER",
	.capabilities = 0,
	.initialize = avnetlcd_encoder_initialize,
	.mode_ops = &modes_ops,
	.ctrl_ops = NULL,
	.output_ops = &outputs_ops,
	.params_ops = NULL,
	.misc_ops = NULL,
	.deinitialize = avnetlcd_encoder_deinitialize,
};

/*
 * This function is called by the encoder manager to initialize
 * avnetlcd encoder driver.
 */
static int avnetlcd_encoder_initialize(struct vid_encoder_device *enc, int flag)
{
	int err = 0, outindex;
	char *std, *output;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	avnetlcd_encoder_channel_info.enc_device = (struct encoder_device *)enc;

	/* call set standard */
	std = avnetlcd_encoder_channel_info.params.mode;
	outindex = avnetlcd_encoder_channel_info.params.outindex;
	output = avnetlcd_encoder_configuration.output[outindex].output_name;
	err |= avnetlcd_encoder_setoutput(output, enc);
	if (err < 0) {
		err = -EINVAL;
		printk(KERN_ERR "Error occured in setoutput\n");
		avnetlcd_encoder_deinitialize(enc);
		return err;
	}
	printk(KERN_DEBUG "Avnetlcd Encoder initialized\n");
	return err;
}

/* Function to de-initialize the encoder */
static int avnetlcd_encoder_deinitialize(struct vid_encoder_device *enc)
{
	if (NULL == enc) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	avnetlcd_encoder_channel_info.enc_device = NULL;
	printk(KERN_DEBUG "Avnetlcd Encoder de-initialized\n");
	return 0;
}

/* Following function is used to set the mode*/
static int avnetlcd_encoder_setmode(struct vid_enc_mode_info *mode_info,
				    struct vid_encoder_device *enc)
{
	int err = 0, outindex, i;
	char *mode;
	struct vid_enc_mode_info *my_mode_info = NULL;

	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	mode = mode_info->name;
	if (NULL == mode) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "Start of avnetlcd_encoder_setmode..\n");
	outindex = avnetlcd_encoder_channel_info.params.outindex;

	if (mode_info->std) {
		char *mymode = NULL;
		/* This is a standard mode */
		for (i = 0;
		     i <
		     avnetlcd_encoder_configuration.output[outindex].
		     no_of_standard; i++) {
			if (!strcmp
			    (avnetlcd_encoder_configuration.output[outindex].
			     standards[i].name, mode)) {
				mymode =
				    avnetlcd_encoder_configuration.
				    output[outindex].standards[i].name;
				break;
			}
		}
		if ((i ==
		     avnetlcd_encoder_configuration.output[outindex].
		     no_of_standard) || (NULL == mymode)) {
			printk(KERN_ERR "Invalid id...\n");
			return -EINVAL;
		}
		/* Store the standard in global object of avnetlcd_encoder */
		avnetlcd_encoder_channel_info.params.mode = mymode;
		return 0;
	} else {
		/* Non- Standard mode. Check if we support it. If so
		   save the timing info and return */
		for (i = 0; i < AVNETLCD_ENCODER_GRAPHICS_NUM_STD; i++) {
			if (!strcmp
			    (avnetlcd_encoder_configuration.output[outindex].
			     standards[i].name, VID_ENC_STD_NON_STANDARD)) {
				my_mode_info =
				    &avnetlcd_encoder_configuration.
				    output[outindex].standards[i];
				break;
			}
		}
		if (my_mode_info) {
			/* We support. So save timing info and return success
			   interface type is same as what is currently is active
			 */
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
			/*
       * If we need to configure something in the encoder module,
       * we need to do this here
			 */
			return 0;
		}
		printk(KERN_ERR "Mode not supported..\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "</avnetlcd_encoder_setmode>\n");
	return err;
}

/* Following function is used to get currently selected mode.*/
static int avnetlcd_encoder_getmode(struct vid_enc_mode_info *mode_info,
				    struct vid_encoder_device *enc)
{
	int err = 0, i, outindex;
	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "<avnetlcd_encoder_getmode>\n");
	outindex = avnetlcd_encoder_channel_info.params.outindex;
	for (i = 0; i < AVNETLCD_ENCODER_GRAPHICS_NUM_STD; i++) {
		if (!strcmp(avnetlcd_encoder_channel_info.params.mode,
			    avnetlcd_encoder_configuration.output[outindex].
			    standards[i].name)) {
			memcpy(mode_info,
			       &avnetlcd_encoder_configuration.output[outindex].
			       standards[i], sizeof(struct vid_enc_mode_info));
			break;
		}
	}
	if (i == AVNETLCD_ENCODER_GRAPHICS_NUM_STD) {
		printk(KERN_ERR "Wiered. No mode info\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "</avnetlcd_encoder_getmode>\n");
	return err;
}

/* For Avnetlcd, we have only one output, called LCD, we
   always set this to this at init
*/
static int avnetlcd_encoder_setoutput(char *output,
				      struct vid_encoder_device *enc)
{
	int err = 0;
	struct vid_enc_mode_info *my_mode_info;
	printk(KERN_DEBUG "<avnetlcd_encoder_setoutput>\n");
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}

	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output: NULL Pointer.\n");
		return -EINVAL;
	}

	/* Just check if the default output match with this output name */
	if (strcmp
	    (avnetlcd_encoder_configuration.output[0].output_name, output)) {
		printk(KERN_ERR "no matching output found.\n");
		return -EINVAL;
	}
	avnetlcd_encoder_channel_info.params.mode
	    = avnetlcd_encoder_configuration.output[0].standards[0].name;

	my_mode_info = &avnetlcd_encoder_configuration.output[0].standards[0];
	err |= avnetlcd_encoder_setmode(my_mode_info, enc);
	if (err < 0) {
		printk(KERN_ERR "Error in setting default mode\n");
		return err;
	}
	printk(KERN_DEBUG "</avnetlcd_encoder_setoutput>\n");
	return err;
}

/* Following function is used to get output name of current output.*/
static int avnetlcd_encoder_getoutput(char *output,
				      struct vid_encoder_device *enc)
{
	int err = 0, index, len;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "<avnetlcd_encoder_getoutput>\n");
	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output:NULL Pointer.\n");
		return -EINVAL;
	}
	index = avnetlcd_encoder_channel_info.params.outindex;
	len = strlen(avnetlcd_encoder_configuration.output[index].output_name);
	if (len > (VID_ENC_NAME_MAX_CHARS - 1))
		len = VID_ENC_NAME_MAX_CHARS - 1;
	strncpy(output,
		avnetlcd_encoder_configuration.output[index].output_name, len);
	output[len] = '\0';
	printk(KERN_DEBUG "</avnetlcd_encoder_getoutput>\n");
	return err;
}

/* Following function is used to enumerate outputs supported by the driver.
   It fills in information in the output. */
static int avnetlcd_encoder_enumoutput(int index, char *output,
				       struct vid_encoder_device *enc)
{
	int err = 0;

	printk(KERN_DEBUG "<avnetlcd_encoder_enumoutput>\n");
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
	if (index >= avnetlcd_encoder_configuration.no_of_outputs)
		return -EINVAL;

	strncpy(output,
		avnetlcd_encoder_configuration.output[index].output_name,
		VID_ENC_NAME_MAX_CHARS);
	printk(KERN_DEBUG "</avnetlcd_encoder_enumoutput>\n");
	return err;
}

/* This function used to initialize the Avnetlcd encoder driver */
static int avnetlcd_encoder_init(void)
{
	int err = 0;

	err = vid_enc_register_encoder(&avnetlcd_encoder_dev);
	printk(KERN_NOTICE "Avnetlcd encoder initialized\n");
	return err;
}

/* Function used to cleanup Avnetlcd encoder driver */
static void avnetlcd_encoder_cleanup(void)
{
	vid_enc_unregister_encoder(&avnetlcd_encoder_dev);
}

subsys_initcall_sync(avnetlcd_encoder_init);
module_exit(avnetlcd_encoder_cleanup);

MODULE_LICENSE("GPL");
