/*
 * Copyright (C) 2011 Ridgerun (http://www.ridgerun.com)
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
 *
 * Based on logicpd_encoder.c
 *
 *
 * Functionality:
 * 	 This  encoder sets the PRGB output according to what it is request for.
 * 	For its operation it is required to be registered as a driver at the
 *  board file.
 *	 There are two ways to itroduce the mode (STD) data to this driver:
 * 	 1) Through the platform data structure (davinci_gen_prgb_pdata) at the board
 * 		file on driver's registration.
 *
 * 	 2) By using booting arguments, in wich case mode parameters are set by
 * 		the instruction "dm365_generic_prgb_encoder.mode=" followed by the
 * 		resolution description, using the CVT syntax for frame buffer setup.
 * 		examples: 720x480MR-16@30 , 1280x720MR-16@60
 *
 *  If both of the configurations are present for the registration, the driver
 * chooses the one set by the bootargs.
 *
 * 	 Clocking set up however depends on two conditions for it's assignment:
 * 	 1) Pixel clock value is intricately related to screen characteristics and
 * 		it has to be set in order to enable the video signals. If pixel clk is
 * 		set to a non-zero value on platform's data structure, its value will be
 * 		always respected an set. Otherwise it will be assigned and/or calculated.
 *
 * 	 2) As the configuration depends on specific hardware and display device, the
 * 		driver uses the function pointed at platform's structure to set the clock
 * 		for the video output. If there's no pointer for such a function, the
 * 		driver will execute the default function which sets the pll1clk6 to use
 * 		closest posible frequency.
 *
 */

/* Kernel Specific header files */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>

#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <media/davinci/vid_encoder_if.h>
#include <media/davinci/davinci_platform.h>
#include <media/davinci/dm365_generic_prgb_encoder.h>

/* Function prototypes */

static __init int gen_prgb_probe(struct platform_device *pdev);
static int gen_prgb_remove(struct platform_device *pdev);

static int gen_prgb_encoder_initialize(struct vid_encoder_device *enc, int flag);
static int gen_prgb_encoder_deinitialize(struct vid_encoder_device *enc);

static int gen_prgb_driver_init(void);
static void gen_prgb_driver_cleanup(void);


static int gen_prgb_encoder_setmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc);
static int gen_prgb_encoder_getmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc);

static int gen_prgb_encoder_setoutput(char *output,
				     struct vid_encoder_device *enc);
static int gen_prgb_encoder_getoutput(char *output,
				     struct vid_encoder_device *enc);

static int gen_prgb_encoder_enumoutput(int index,
				      char *output,
				      struct vid_encoder_device *enc);

static void pixel_clock_estimation(struct davinci_gen_prgb_pdata *gen_prgb_dev);
static int set_pixel_clock_output(unsigned int required_clk_rate);


/* Variables */

static char *def_mode;
unsigned int pixel_clock_khz;
int (*clock_set_function)(unsigned int);

static struct gen_prgb_encoder_config gen_prgb_encoder_configuration = {
	.no_of_outputs = GEN_PRGB_ENCODER_MAX_NO_OUTPUTS,
	.output[0] = {
		      .output_name = VID_ENC_OUTPUT_PRGB,
		      .no_of_standard = GEN_PRGB_ENCODER_NUM_STD,
		      .standards[0] = {	/* This is programmed by the driver when
								.probe function is called */
				       .name = VID_ENC_STD_PRGB_DEFAULT,
				       .std = 1,
				       .if_type = VID_ENC_IF_PRGB,
				       .interlaced = 0,
				       .xres = 0,
				       .yres = 0,
				       .fps = {0, 1},
				       .left_margin = 0,
				       .right_margin = 0,
				       .upper_margin = 0,
				       .lower_margin = 0,
				       .hsync_len = 0,
				       .vsync_len = 0,
				       .flags = 0}, /* hsync -ve, vsync -ve */
		      },
};

/* This is a default structure that should not be modified */
static struct davinci_gen_prgb_pdata gen_prgb_mode_data = {
	.xres = 0,
	.yres = 0,
	.fps = {30, 1},
	.pixel_clock_khz = 0,
	.clock_set_function = &set_pixel_clock_output,
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.vsync_len = 0,
	.flags = 0,
};


static struct gen_prgb_encoder_channel gen_prgb_encoder_channel_info = {
	.params.outindex = 0,
	.params.mode = VID_ENC_STD_PRGB_DEFAULT,
	.enc_device = NULL
};

static struct vid_enc_output_ops outputs_ops = {
	.count = GEN_PRGB_ENCODER_MAX_NO_OUTPUTS,
	.enumoutput = gen_prgb_encoder_enumoutput,
	.setoutput = gen_prgb_encoder_setoutput,
	.getoutput = gen_prgb_encoder_getoutput
};

static struct vid_enc_mode_ops modes_ops = {
	.setmode = gen_prgb_encoder_setmode,
	.getmode = gen_prgb_encoder_getmode,
};

/* struct for encoder registration */
static struct vid_encoder_device gen_prgb_encoder_dev = {
	.name = "GEN_PRGB_ENCODER",
	.capabilities = 0,
	.initialize = gen_prgb_encoder_initialize,
	.mode_ops = &modes_ops,
	.ctrl_ops = NULL,
	.output_ops = &outputs_ops,
	.params_ops = NULL,
	.misc_ops = NULL,
	.deinitialize = gen_prgb_encoder_deinitialize,
};

/* struct for driver registration */
static struct platform_driver gen_prgb_driver = {
	.driver = {
		.name = PRGB_ENCODER_DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe = gen_prgb_probe,
	.remove = __devexit_p(gen_prgb_remove),
};

static ssize_t width_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gen_prgb_encoder_configuration.output[0].standards[0].xres);
}

static ssize_t height_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gen_prgb_encoder_configuration.output[0].standards[0].yres);
}

#define DECLARE_ATTR(_name, _mode, _show)                       \
{                                                               \
	.attr   = { .name = __stringify(_name), .mode = _mode,	\
		    .owner = THIS_MODULE },  			\
	.show   = _show,                                        \
}
static struct device_attribute device_attributes[] = {
	DECLARE_ATTR(width, S_IRWXUGO, width_show),
	DECLARE_ATTR(height, S_IRWXUGO, height_show)
};

struct display_device {
	struct module *owner;
	struct device class_dev;
};

struct display_device *displaydevice;
/* Defined in davinci encoder manager */
extern struct class display_class;

static void create_sysfs_files()
{
	struct display_device *displaydevice;
	int i, ret;

	displaydevice = kzalloc(sizeof(struct display_device), GFP_KERNEL);
	if (!displaydevice)
		return;

	displaydevice->owner = THIS_MODULE;
	displaydevice->class_dev.class = &display_class;
	dev_set_name(&displaydevice->class_dev, "prgb");
	dev_set_drvdata(&displaydevice->class_dev, displaydevice);
	ret = device_register(&displaydevice->class_dev);
	if (ret < 0) {
		printk(KERN_ERR "PRGB encoder: Error in device_register\n");
		kfree(displaydevice);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(device_attributes); i++) {
		ret = device_create_file(&displaydevice->class_dev,
					       &device_attributes[i]);
		if (ret < 0) {
			while (--i >= 0)
				device_remove_file(&displaydevice->class_dev,
							 &device_attributes
							 [i]);
			dev_set_drvdata(&displaydevice->class_dev, NULL);
			device_unregister(&displaydevice->class_dev);
			return;
		}
	}
}

static void remove_sysfs_files()
{
	int i;

	for (i = 0; i < ARRAY_SIZE(device_attributes); i++)
		device_remove_file(&displaydevice->class_dev,
					 &device_attributes[i]);

	dev_set_drvdata(&displaydevice->class_dev, NULL);
	device_unregister(&displaydevice->class_dev);
}


/* Function for pixel clock estimation based on screen parameters of the data structure */
static void pixel_clock_estimation(struct davinci_gen_prgb_pdata *gen_prgb_dev)
{
	int hint, vint;

	/* pixel_clk = (hint+1)*(vint+1)*FPS
	 * hint = xres + hfp + hbp + hsync
	 * vint = yres + vfp + vbp + vsync
	 */

	hint = gen_prgb_dev->xres + gen_prgb_dev->left_margin  + gen_prgb_dev->right_margin + gen_prgb_dev->hsync_len;
	vint = gen_prgb_dev->yres + gen_prgb_dev->upper_margin + gen_prgb_dev->lower_margin + gen_prgb_dev->vsync_len;

	gen_prgb_dev->pixel_clock_khz = ( ((hint + 1) * (vint + 1)) / 1000) * (gen_prgb_dev->fps.numerator / gen_prgb_dev->fps.denominator);

	printk(KERN_DEBUG "required pixel clk rate= %d KHz\n", gen_prgb_dev->pixel_clock_khz);

}

/* Function for setup SYSCLK6 to generate the required frequency */
static int set_pixel_clock_output(unsigned int required_clk_rate)
{
	int err = 0;

	unsigned int pll_div6;
	unsigned int clk_div = 0;

	int freq_factor, diff, tmp_diff;
	int div_factor;
	int i;

	if ((!required_clk_rate) | (required_clk_rate > DAVINCI_PLL1_RATE)){
		printk(KERN_NOTICE "Invalid frequency rate of %d\n", required_clk_rate);
		return -1;
	}

	/* Calculating the closest required divider, depends on DAVINCI_PLL1_RATE */

	freq_factor = (DAVINCI_PLL1_RATE << 5) / required_clk_rate;
	diff = 31 << 5;

	for (i=0; i <= 31; i++){

		if ((i << 5) > freq_factor)
			break;

		div_factor = (i + 1) << 5;

		tmp_diff = abs(freq_factor - div_factor);

		if (tmp_diff < diff){
			diff = tmp_diff;
			clk_div = i;
		}
	}

	printk(KERN_DEBUG "assigned divider for pll1clk6 = %d\n", clk_div);

	if (clk_div > PLL1DIV6_MAX_NUM)
		err = -1;
	else {

		/* Reading pll_div6 current value*/
		pll_div6 = __raw_readl(IO_ADDRESS(DAVINCI_SYSTEM_MODULE_BASE + 0x968));

		/* Assigning divider to the closest required frequency */
		pll_div6 = 0x8000 | clk_div;	/* DAVINCI_PLL1_RATE/(clk_div+1)= required_clk_rate (Khz) */

		__raw_writel(pll_div6, IO_ADDRESS(DAVINCI_SYSTEM_MODULE_BASE + 0x968));

		/* Set the bit in the ALNCTL register to flag a change in the
		 * PLLDIV6 ratio */
		__raw_writel(1<<6, IO_ADDRESS(DAVINCI_SYSTEM_MODULE_BASE + 0x940));

		/* Run a GO operation to perform the change. */
		__raw_writel(0x1, IO_ADDRESS(DAVINCI_SYSTEM_MODULE_BASE + 0x938));
	}

	return err;
}

static int gen_prgb_encoder_mode_to_timings(const char *mode_str)
{
	struct fb_info fbi;
	struct fb_var_screeninfo var;
	struct fb_ops fbops;
	int err = 0;

	printk(KERN_DEBUG "gen_prgb_encoder_mode_to_timings\n");

	memset(&fbi, 0, sizeof(fbi));
	memset(&var, 0, sizeof(var));
	memset(&fbops, 0, sizeof(fbops));
	fbi.fbops = &fbops;

	err = fb_find_mode(&var, &fbi, mode_str, NULL, 0, NULL, 24);

	if (err != 0) {

		gen_prgb_mode_data.left_margin 		= var.left_margin;
		gen_prgb_mode_data.right_margin 	= var.right_margin;
		gen_prgb_mode_data.upper_margin 	= var.upper_margin;
		gen_prgb_mode_data.lower_margin 	= var.lower_margin;
		gen_prgb_mode_data.hsync_len 		= var.hsync_len;
		gen_prgb_mode_data.vsync_len 		= var.vsync_len;
		gen_prgb_mode_data.xres 			= var.xres;
		gen_prgb_mode_data.yres 			= var.yres;

		if (!gen_prgb_mode_data.pixel_clock_khz)
			gen_prgb_mode_data.pixel_clock_khz	= PICOS2KHZ(var.pixclock);

		return 0;
	} else {
		return -EINVAL;
	}
}

/* function for parsing mode parameters */
static int gen_prgb_encoder_parse_def_modes(struct davinci_gen_prgb_pdata **gen_prgb_dev)
{
	char *str, *options, *this_opt;
	int err = 0;

	printk(KERN_DEBUG "gen_prgb_encoder_parse_def_modes\n");

	str = kmalloc(strlen(def_mode) + 1, GFP_KERNEL);
	strcpy(str, def_mode);
	options = str;

	while (!err && (this_opt = strsep(&options, ",")) != NULL) {
		char *mode_str;

		mode_str = this_opt;
		err = gen_prgb_encoder_mode_to_timings(mode_str);

		if (err)
			break;
	}

	if (err == 0)
		*gen_prgb_dev = &gen_prgb_mode_data;
	else
		*gen_prgb_dev = NULL;

	kfree(str);

	return err;
}

/* This function is called by the encoder manager to initialize gen_prgb encoder driver.
 */
static int gen_prgb_encoder_initialize(struct vid_encoder_device *enc, int flag)
{
	int err = 0, outindex;
	char *std, *output;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	gen_prgb_encoder_channel_info.enc_device = (struct encoder_device *)enc;

	/* call set standard */
	std = gen_prgb_encoder_channel_info.params.mode;
	outindex = gen_prgb_encoder_channel_info.params.outindex;
	output = gen_prgb_encoder_configuration.output[outindex].output_name;
	err |= gen_prgb_encoder_setoutput(output, enc);
	if (err < 0) {
		err = -EINVAL;
		printk(KERN_ERR "Error occured in setoutput\n");
		gen_prgb_encoder_deinitialize(enc);
		return err;
	}
	printk(KERN_NOTICE "General PRGB Encoder initialized\n");
	return err;
}

/* Function to de-initialize the encoder */
static int gen_prgb_encoder_deinitialize(struct vid_encoder_device *enc)
{
	if (NULL == enc) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	gen_prgb_encoder_channel_info.enc_device = NULL;
	printk(KERN_DEBUG "General PRGB Encoder de-initialized\n");
	return 0;
}

/* Following function is used to set the mode*/
static int gen_prgb_encoder_setmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc)
{
	int err = 0, outindex, i;
	char *mode;

	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == (mode = mode_info->name)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "Start of gen_prgb_encoder_setmode..\n");
	outindex = gen_prgb_encoder_channel_info.params.outindex;

	if (mode_info->std) {
		char *mymode = NULL;
		/* This is a standard mode */
		for (i = 0;
		     i <
		     gen_prgb_encoder_configuration.output[outindex].
		     no_of_standard; i++) {
			if (!strcmp
			    (gen_prgb_encoder_configuration.output[outindex].
			     standards[i].name, mode)) {
				mymode =
				    gen_prgb_encoder_configuration.
				    output[outindex].standards[i].name;
				/* CLK setup */
				err = clock_set_function(pixel_clock_khz);
				break;
			}
		}
		if ((i ==
		     gen_prgb_encoder_configuration.output[outindex].
		     no_of_standard) || (NULL == mymode)) {
			printk(KERN_ERR "Invalid id...\n");
			return -EINVAL;
		}
		/* Store the standard in global object of gen_prgb_encoder */
		gen_prgb_encoder_channel_info.params.mode = mymode;
		return 0;
	}
	printk(KERN_DEBUG "</gen_prgb_encoder_setmode>\n");
	return err;
}

/* Following function is used to get currently selected mode.*/
static int gen_prgb_encoder_getmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc)
{
	int err = 0, i, outindex;
	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "<gen_prgb_encoder_getmode>\n");
	outindex = gen_prgb_encoder_channel_info.params.outindex;
	for (i = 0; i < GEN_PRGB_ENCODER_NUM_STD; i++) {
		if (!strcmp(gen_prgb_encoder_channel_info.params.mode,
			    gen_prgb_encoder_configuration.output[outindex].
			    standards[i].name)) {
			memcpy(mode_info,
			       &gen_prgb_encoder_configuration.output[outindex].
			       standards[i], sizeof(struct vid_enc_mode_info));
			break;
		}
	}
	if (i == GEN_PRGB_ENCODER_NUM_STD) {
		printk(KERN_ERR "Wiered. No mode info\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "</gen_prgb_encoder_getmode>\n");
	return err;
}

/* For General PRGB, we have several outputs, we
   always set this to this at init
*/
static int gen_prgb_encoder_setoutput(char *output,
				     struct vid_encoder_device *enc)
{
	int err = 0;
	struct vid_enc_mode_info *my_mode_info;
	printk(KERN_DEBUG "<gen_prgb_encoder_setoutput>\n");
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
	if (strcmp(gen_prgb_encoder_configuration.output[0].output_name, output)) {
		printk(KERN_ERR "no matching output found.\n");
		return -EINVAL;
	}
	gen_prgb_encoder_channel_info.params.mode
	    = gen_prgb_encoder_configuration.output[0].standards[0].name;

	my_mode_info = &gen_prgb_encoder_configuration.output[0].standards[0];
	err |= gen_prgb_encoder_setmode(my_mode_info, enc);
	if (err < 0) {
		printk(KERN_ERR "Error in setting default mode\n");
		return err;
	}
	printk(KERN_DEBUG "</gen_prgb_encoder_setoutput>\n");
	return err;
}

/* Following function is used to get output name of current output.*/
static int gen_prgb_encoder_getoutput(char *output,
				     struct vid_encoder_device *enc)
{
	int err = 0, index, len;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	printk(KERN_DEBUG "<gen_prgb_encoder_getoutput>\n");
	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output:NULL Pointer.\n");
		return -EINVAL;
	}
	index = gen_prgb_encoder_channel_info.params.outindex;
	len = strlen(gen_prgb_encoder_configuration.output[index].output_name);
	if (len > (VID_ENC_NAME_MAX_CHARS - 1))
		len = VID_ENC_NAME_MAX_CHARS - 1;
	strncpy(output, gen_prgb_encoder_configuration.output[index].output_name,
		len);
	output[len] = '\0';
	printk(KERN_DEBUG "</gen_prgb_encoder_getoutput>\n");
	return err;
}

/* Following function is used to enumerate outputs supported by the driver.
   It fills in information in the output. */
static int gen_prgb_encoder_enumoutput(int index, char *output,
				      struct vid_encoder_device *enc)
{
	int err = 0;

	printk(KERN_DEBUG "<gen_prgb_encoder_enumoutput>\n");
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
	if (index >= gen_prgb_encoder_configuration.no_of_outputs) {
		return -EINVAL;
	}
	strncpy(output,
		gen_prgb_encoder_configuration.output[index].output_name,
		VID_ENC_NAME_MAX_CHARS);
	printk(KERN_DEBUG "</gen_prgb_encoder_enumoutput>\n");
	return err;
}


/* Encoder registration with the encoder manager */
static int __init gen_prgb_probe(struct platform_device *pdev)
{
	int err = 0;

	struct davinci_gen_prgb_pdata *gen_prgb_dev = (struct davinci_gen_prgb_pdata *) (*pdev).dev.platform_data;

	printk(KERN_DEBUG "gen_prgb_probe\n");

	/* Setting up prgb struct parameters */
	if(gen_prgb_dev){
		if (gen_prgb_dev->clock_set_function)
			gen_prgb_mode_data.clock_set_function = gen_prgb_dev->clock_set_function;
		else
			gen_prgb_dev->clock_set_function = gen_prgb_mode_data.clock_set_function;

		if (gen_prgb_dev->pixel_clock_khz)
			gen_prgb_mode_data.pixel_clock_khz = gen_prgb_dev->pixel_clock_khz;
	}

	if (def_mode && strlen(def_mode) > 0) {
		if (gen_prgb_encoder_parse_def_modes(&gen_prgb_dev)){
			printk(KERN_NOTICE "cannot parse default modes\n");
			err = -1;
		}
	}

	/* Assigning video standard values */
	if (gen_prgb_dev) {

		gen_prgb_encoder_configuration.output[0].standards[0].xres 			= gen_prgb_dev->xres;
		gen_prgb_encoder_configuration.output[0].standards[0].yres 			= gen_prgb_dev->yres;
		gen_prgb_encoder_configuration.output[0].standards[0].fps 			= gen_prgb_dev->fps;
		gen_prgb_encoder_configuration.output[0].standards[0].left_margin 	= gen_prgb_dev->left_margin;
		gen_prgb_encoder_configuration.output[0].standards[0].right_margin 	= gen_prgb_dev->right_margin;
		gen_prgb_encoder_configuration.output[0].standards[0].upper_margin 	= gen_prgb_dev->upper_margin;
		gen_prgb_encoder_configuration.output[0].standards[0].lower_margin 	= gen_prgb_dev->lower_margin;
		gen_prgb_encoder_configuration.output[0].standards[0].hsync_len 	= gen_prgb_dev->hsync_len;
		gen_prgb_encoder_configuration.output[0].standards[0].vsync_len 	= gen_prgb_dev->vsync_len;
		gen_prgb_encoder_configuration.output[0].standards[0].flags		 	= gen_prgb_dev->flags;

		if(!(gen_prgb_dev->pixel_clock_khz))
			pixel_clock_estimation(gen_prgb_dev);

		pixel_clock_khz = gen_prgb_dev->pixel_clock_khz;
		clock_set_function = gen_prgb_dev->clock_set_function;

		if (err != 0)
			printk(KERN_NOTICE "failed when setting up pixel_clk\n");

		err = vid_enc_register_encoder(&gen_prgb_encoder_dev);
		if (err != 0)
			printk(KERN_NOTICE "failed at encoder registration\n");

	} else {
		printk(KERN_NOTICE "No data structure on generic_prgb driver\n");
		err = -1;
	}

	create_sysfs_files();

	return err;
}

/* This function used to un-register the General PRGB driver */
static int gen_prgb_remove(struct platform_device *pdev)
{
	remove_sysfs_files();
	vid_enc_unregister_encoder(&gen_prgb_encoder_dev);

	return 0;
}

/* This function used to initialize the General PRGB driver */
static int gen_prgb_driver_init(void)
{
	int err = 0;

	printk(KERN_DEBUG "gen_prgb_driver_init\n");
	/* Register driver to the kernel */
	err = platform_driver_register(&gen_prgb_driver);

	return err;
}

/* This function used to un-initialize the General PRGB driver */
static void gen_prgb_driver_cleanup(void)
{
	platform_driver_unregister(&gen_prgb_driver);
}

module_param_named(mode, def_mode, charp, 0);

subsys_initcall(gen_prgb_driver_init);
module_exit(gen_prgb_driver_cleanup);

MODULE_LICENSE("GPL");
