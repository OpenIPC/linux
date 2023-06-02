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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/module.h>

#include <linux/attribute_container.h>
#include <mach/hardware.h>
#include <mach/mux.h>
#include <mach/cputype.h>
#include <asm/io.h>
#include <media/davinci/davinci_enc.h>
#include <media/davinci/vid_encoder_types.h>
#include <media/davinci/vid_encoder_if.h>
#include <media/davinci/davinci_enc_mngr.h>
#include <media/davinci/davinci_platform.h>

#define ISNULL(p)       ((NULL) == (p))

static char *ch0_output = "";
static char *ch1_output = "";
static char *ch0_mode = "";
static char *ch1_mode = "";
module_param(ch0_output, charp, S_IRUGO);
module_param(ch1_output, charp, S_IRUGO);
module_param(ch0_mode, charp, S_IRUGO);
module_param(ch1_mode, charp, S_IRUGO);

#include <linux/stat.h>
//#define DAVINCI_ENC_MAX_CHANNELS 1

struct vid_enc_device_mgr enc_dev[DAVINCI_ENC_MAX_CHANNELS];

EXPORT_SYMBOL(enc_dev);
extern struct enc_config davinci_enc_default[];
extern char *davinci_outputs[];
extern char *davinci_modes[];

struct display_device {
	struct module *owner;
	struct device class_dev;
	int channel;
};

static struct display_device *davinci_display_device[DAVINCI_ENC_MAX_CHANNELS];

#define to_display_dev(cdev)	container_of(cdev, struct display_device, class_dev)

static void display_class_release(struct device *cdev)
{
	struct display_device *dev = to_display_dev(cdev);

	if (dev != NULL)
		kfree(dev);
}

struct class display_class = {
	.name = "davinci_display",
	.owner	= THIS_MODULE,
	.dev_release = display_class_release,
};
EXPORT_SYMBOL(display_class);

static ssize_t output_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	char name[VID_ENC_NAME_MAX_CHARS];
	struct display_device *dev = to_display_dev(cdev);
	int p, ret;

	ret = davinci_enc_get_output(dev->channel, name);
	if (ret < 0)
		return ret;

	p = sprintf(buf, name);
	p += sprintf(buf + p, "\n");

	return p;
}

static ssize_t
output_store(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t count)
{
	struct display_device *dev = to_display_dev(cdev);
	char name[VID_ENC_NAME_MAX_CHARS];
	int ret;

	if (!buffer || (count == 0)|| (count > VID_ENC_NAME_MAX_CHARS))
		return 0;

	strncpy(name, buffer, count);
	name[count - 1] = 0;
	ret = davinci_enc_set_output(dev->channel, name);
	if (ret < 0) {
		return ret;
	}

	return count;
}

static ssize_t mode_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	struct display_device *dev = to_display_dev(cdev);
	struct vid_enc_mode_info mode_info;
	int p;
	int ret;

	ret = davinci_enc_get_mode(dev->channel, &mode_info);
	if (ret < 0)
		return ret;

	p = sprintf(buf, mode_info.name);
	p += sprintf(buf + p, "\n");

	return p;
}

static ssize_t
mode_store(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t count)
{
	struct display_device *dev = to_display_dev(cdev);
	char name[VID_ENC_NAME_MAX_CHARS];
	struct vid_enc_mode_info mode_info;
	int ret;

	if (!buffer || (count == 0) || (count > VID_ENC_NAME_MAX_CHARS))
		return 0;

	strncpy(name, buffer, count);
	name[count - 1] = 0;

	ret = davinci_enc_get_mode(dev->channel, &mode_info);

	if (ret < 0)
		return ret;

	mode_info.name = name;
	ret = davinci_enc_set_mode(dev->channel, &mode_info);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t enable_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t
enable_store(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t count)
{
	struct display_device *dev = to_display_dev(cdev);
	int enable_output_state, ret;

	if (!buffer || (count == 0))
		return 0;

	if (strncmp(buffer, "on", 2) == 0)
		enable_output_state = 1;
	else if (strncmp(buffer, "off", 3) == 0)
		enable_output_state = 0;
	else
		return -EINVAL;

	ret = davinci_enc_enable_output(dev->channel, enable_output_state);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t
control_store(struct device *cdev, const char *buffer, size_t count,
	      u8 ctrl)
{
	struct display_device *dev = to_display_dev(cdev);
	int val;
	int ret;

	if (!buffer || (count == 0))
		return 0;

	val = simple_strtoul(buffer, NULL, 0);

	ret = davinci_enc_set_control(dev->channel, ctrl, (char)val);
	if (ret < 0)
		return ret;
	return count;
}
static ssize_t control_show(struct device *cdev, char *buf, u8 ctrl)
{
	struct display_device *dev = to_display_dev(cdev);
	unsigned int p;
	unsigned char val;
	int ret;

	ret = davinci_enc_get_control(dev->channel, ctrl, &val);
	if (ret < 0)
		return ret;

	p = sprintf(buf, "%d", val);
	p += sprintf(buf + p, "\n");

	return p;
}

static ssize_t
brightness_store(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t count)
{
	return control_store(cdev, buffer, count, VID_ENC_CTRL_BRIGHTNESS);
}
static ssize_t brightness_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	return control_show(cdev, buf, VID_ENC_CTRL_BRIGHTNESS);
}
static ssize_t
hue_store(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t count)
{
	return control_store(cdev, buffer, count, VID_ENC_CTRL_HUE);
}
static ssize_t hue_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	return control_show(cdev, buf, VID_ENC_CTRL_HUE);
}
static ssize_t
gain_store(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t count)
{
	return control_store(cdev, buffer, count, VID_ENC_CTRL_GAIN);
}
static ssize_t gain_show(struct device *cdev, struct device_attribute *attr, char *buf)
{
	return control_show(cdev, buf, VID_ENC_CTRL_GAIN);
}

#define DECLARE_ATTR(_name, _mode, _show, _store)                  \
{                                                               \
	.attr   = { .name = __stringify(_name), .mode = _mode,	\
		      },  			\
	.show   = _show,                                        \
	.store  = _store,                                       \
}
static struct device_attribute bl_device_attributes[] = {
	DECLARE_ATTR(output, S_IRWXUGO, output_show, output_store),
	DECLARE_ATTR(mode, S_IRWXUGO, mode_show, mode_store),
	DECLARE_ATTR(enable, S_IRWXUGO, enable_show, enable_store),
	DECLARE_ATTR(brightness, S_IRWXUGO, brightness_show, brightness_store),
	DECLARE_ATTR(hue, S_IRWXUGO, hue_show, hue_store),
	DECLARE_ATTR(gain, S_IRWXUGO, gain_show, gain_store)
};

static void *create_sysfs_files(int channel)
{
	struct display_device *dev;
	int ret;
	int i;

	dev = kzalloc(sizeof(struct display_device), GFP_KERNEL);
	if (!dev)
		return NULL;

	dev->owner = THIS_MODULE;
	dev->channel = channel;
	dev->class_dev.class = &display_class;
	dev_set_name(&dev->class_dev, "ch%d", channel);
	dev_set_drvdata(&dev->class_dev, dev);
	ret = device_register(&dev->class_dev);
	if (ret < 0) {
		printk(KERN_ERR "DaVinci Enc Manager: Error in device_register\n");
		kfree(dev);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(bl_device_attributes); i++) {
		ret = device_create_file(&dev->class_dev,
					       &bl_device_attributes[i]);
		if (ret < 0) {
			while (--i >= 0)
				device_remove_file(&dev->class_dev,
							 &bl_device_attributes
							 [i]);
			dev_set_drvdata(&dev->class_dev, NULL);
			device_unregister(&dev->class_dev);
			return NULL;
		}
	}

	return dev;
}

static void remove_sysfs_files(struct display_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bl_device_attributes); i++)
		device_remove_file(&dev->class_dev,
					 &bl_device_attributes[i]);

	dev_set_drvdata(&dev->class_dev, NULL);
	device_unregister(&dev->class_dev);
}

static void davinci_enc_check_options(void)
{
	int i;

	i = 0;
	while (*(davinci_modes[i]) != 0) {
		if (*ch0_mode != 0 && !strcmp(ch0_mode, davinci_modes[i])) {
			if (strcmp(ch0_mode, "ntsc") == 0
			    || strcmp(ch0_mode, "pal") == 0)
				strcpy(davinci_enc_default[0].mode,
				       davinci_modes[i - 1]);
			else
				strcpy(davinci_enc_default[0].mode,
				       davinci_modes[i]);
		}
		if (*ch1_mode != 0 &&
		    (DAVINCI_ENC_MAX_CHANNELS > 1) &&
		    !strcmp(ch1_mode, davinci_modes[i])) {
			if (strcmp(ch1_mode, "ntsc") == 0
			    || strcmp(ch1_mode, "pal") == 0)
				strcpy(davinci_enc_default[1].mode,
				       davinci_modes[i - 1]);
			else
				strcpy(davinci_enc_default[1].mode,
				       davinci_modes[i]);
		}
		i++;
	}

	i = 0;
	while (*(davinci_outputs[i]) != 0) {
		if (*ch0_output != 0 && !strcmp(ch0_output, davinci_outputs[i]))
			strcpy(davinci_enc_default[0].output,
			       davinci_outputs[i]);

		if (*ch1_output != 0 &&
		    (DAVINCI_ENC_MAX_CHANNELS > 1) &&
		    !strcmp(ch1_output, davinci_outputs[i]))
			strcpy(davinci_enc_default[1].output,
			       davinci_outputs[i]);
		i++;
	}
	printk(KERN_NOTICE "ch0 default output \"%s\", mode \"%s\"\n",
	       davinci_enc_default[0].output, davinci_enc_default[0].mode);
	if (DAVINCI_ENC_MAX_CHANNELS > 1)
		printk(KERN_NOTICE "ch1 default output \"%s\", mode \"%s\"\n",
		       davinci_enc_default[1].output,
		       davinci_enc_default[1].mode);
	return;
}

/*
 * vid_enc_mgr_init
 */
static int davinci_enc_mngr_init(void)
{

	int i, err = -EINVAL;
	struct vid_enc_device_mgr *ptr = 0;


	class_register(&display_class);

	for (i = 0; i < DAVINCI_ENC_MAX_CHANNELS; i++) {
		ptr = &enc_dev[i];

		memset((void *)ptr, 0, sizeof(struct vid_enc_device_mgr));
		sema_init(&ptr->lock, 1);

		davinci_display_device[i] = create_sysfs_files(i);
		if (!davinci_display_device[i]) {
			printk(KERN_ERR
			       "Could not create display control sysfs "
			       "files for channel %d\n", i);
			return -EINVAL;
		}
	}

	davinci_enc_check_options();
	err = 0;
	return err;
}

/*
 * vid_enc_mgr_cleanup
 */
static void davinci_enc_mngr_exit(void)
{
//Sandeep
	int i;

	for (i = 0; i < DAVINCI_ENC_MAX_CHANNELS; i++)
		remove_sysfs_files(davinci_display_device[i]);

	class_unregister(&display_class);

	return;
}

/*
 * davinci_get_cur_encoder: get current encoder on the specified channel
 */
static struct vid_encoder_device *davinci_get_cur_encoder(int channel)
{
	struct vid_enc_device_mgr *mgr;

	if ((channel > DAVINCI_ENC_MAX_CHANNELS) || (channel < 0))
		return NULL;

	mgr = &enc_dev[channel];

	if (mgr->current_encoder > mgr->num_encoders)
		return NULL;

	if (mgr->num_encoders == 0)
		return NULL;

	return mgr->encoder[mgr->current_encoder];
}

/**
 * function davinci_enc_setparams
 * @params: pointer to params structure.
 * @channel: channel number.
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Set parameters at current active encoder. params will be defined by
 * the specific encoder and used by user space applications to set encoder
 * parameters.
 */
int davinci_enc_setparams(int channel, void *params)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->params_ops || NULL == params)
		return -EINVAL;

	if (cur_enc->params_ops->setparams != NULL)
		return cur_enc->params_ops->setparams(params, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_setparams);

/**
 * function davinci_enc_getparams
 * @params: pointer to params structure.
 * @channel: channel number, 0 for first channel and so forth
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Get parameters at current active encoder. params will be defined by
 * the specific encoder and used by user space applications to get encoder
 * parameters.
 */
int davinci_enc_getparams(int channel, void *params)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->params_ops || NULL == params)
		return -EINVAL;

	if (cur_enc->params_ops->getparams != NULL)
		return cur_enc->params_ops->getparams(params, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_getparams);

/**
 * function davinci_enc_set_control
 * @channel: channel number.
 * @ctrl: davinci_vid_enc_control type
 * @val:  control value to be set
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Set controls at the current encoder's output.
 *
 */
int davinci_enc_set_control(int channel, enum vid_enc_ctrl_id ctrl, char val)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->ctrl_ops)
		return -EINVAL;

	if (cur_enc->ctrl_ops->setcontrol != NULL)
		return cur_enc->ctrl_ops->setcontrol(ctrl, val, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_set_control);

/**
 * function davinci_enc_get_control
 * @channel: channel number.
 * @ctrl: control type as per davinci_vid_enc_ctrl_type
 * @val:  ptr to value that gets updated
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Get controls at the current encoder's output.
 *
 */
int davinci_enc_get_control(int channel, enum vid_enc_ctrl_id ctrl, char *val)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->ctrl_ops || NULL == val)
		return -EINVAL;

	if (cur_enc->ctrl_ops->getcontrol != NULL)
		return cur_enc->ctrl_ops->getcontrol(ctrl, val, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_get_control);

/**
 * function davinci_enc_reset
 * @channel: channel number. DAVINCI_ENC_MAX_CHANNELS set to 1
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *  Do a software Reset the current encoder. Some of the encoders require this.
 * This shouldn't affect the contents of the registers configured already for
 * for output, standard, control etc. If there is no support, encoder doesn't
 * implement this API.
 */
int davinci_enc_reset(int channel)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->misc_ops)
		return -EINVAL;

	if (cur_enc->misc_ops != NULL && cur_enc->misc_ops->reset != NULL)
		return cur_enc->misc_ops->reset(cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_reset);

/**
 * function davinci_enc_enable_output
 * @channel: channel number. DAVINCI_ENC_MAX_CHANNELS set to 1
 * @flag: flag to indicate enable or disable, 0 - disable, 1 - enable
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
*  Enable/Disable the current ouput. While the VPSS is configured for a
 * video mode or graphics mode, you may observe noise on the display device
 * due to timing changes. To avoid this, the output may be disabled during
* configuration of the VENC or related hardware in the VPSS and re-enabled
* using this API. This will switch the output DACs Off or On based on the
* flag.
 */
int davinci_enc_enable_output(int channel, int flag)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->misc_ops)
		return -EINVAL;

	if (cur_enc->misc_ops != NULL && cur_enc->misc_ops->enable != NULL)
		return cur_enc->misc_ops->enable(flag, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_enable_output);

/**
 * function davinci_enc_set_output
 * @channel: channel number.
 * @output: ptr to output name string
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Set output - Set channel's output to the one identified by output string
 * The encoder manager calls enumoutput() of each of the encoder to determine
 * the encoder that supports this output and set this encoder as the current
 * encoder. It calls setoutput() of the selected encoder. Encoder is expected
 * set a default mode on this output internally. Manager calls the getmode()
 * to get information about the mode to program the SoC hardware (VENC/DLCD
 * for Davinci/DM355. During programing of the SoC hardware for timing, manager
 * would call enable() to disable and re-enable the output of the encoder
 * to avoid noise at the display. It may also call reset() to make sure the
 * encoder is reset if required by the encoder hardware.
 */
int davinci_enc_set_output(int channel, char *output)
{
	int i, j, found = -1;
	int err = -EINVAL;
	struct vid_encoder_device *cur_enc, *new_enc = 0;
	struct vid_enc_device_mgr *mgr = NULL;
	char buf[VID_ENC_NAME_MAX_CHARS];

	cur_enc = davinci_get_cur_encoder(channel);
	if (NULL == cur_enc || NULL == output)
		return err;

	mgr = &enc_dev[channel];

	/* enumerate the output for each encoder on this channel
	   to find a matching one */
	for (i = 0; i < mgr->num_encoders; i++) {
		if (ISNULL(mgr->encoder[i]))
			return err;

		for (j = 0; j < mgr->encoder[i]->output_ops->count; j++) {
			err =
			    mgr->encoder[i]->output_ops->
			    enumoutput(j, buf, mgr->encoder[i]);
			if (err != 0)
				return err;
			if (strcmp(buf, output) == 0) {
				found = i;
				new_enc = mgr->encoder[i];
				break;
			}
		}
	}

	if (found < 0) {
		printk(KERN_ERR "output not found\n");
		return -EINVAL;
	}

	/* found the encoder */
	err = 0;
	mgr->current_encoder = found;
	if ((err = down_interruptible(&mgr->lock)) < 0)
		return err;
	err |= cur_enc->deinitialize(cur_enc);
	err |= new_enc->initialize(new_enc, VID_ENC_FULL_INIT_FLAG);
	err |= new_enc->output_ops->setoutput(output, new_enc);

	if (err < 0) {
		up(&mgr->lock);
		return err;
	}

	/* get default mode */
	err |= new_enc->mode_ops->getmode(&mgr->current_mode, new_enc);

	if (new_enc->misc_ops != NULL && new_enc->misc_ops->enable != NULL)
		err |= new_enc->misc_ops->enable(0, new_enc);

	davinci_enc_set_mode_platform(channel, mgr);

	/* Some encoders need a soft reset after configuring VENC, ex.
	   ths8200
	 */
	if (new_enc->misc_ops != NULL && new_enc->misc_ops->reset != NULL)
		err |= new_enc->misc_ops->reset(new_enc);

	if (new_enc->misc_ops != NULL && new_enc->misc_ops->enable != NULL)
		err |= new_enc->misc_ops->enable(1, new_enc);

	if (0 == err)
		strcpy(enc_dev[channel].current_output, output);

	up(&mgr->lock);

	return err;
}

EXPORT_SYMBOL(davinci_enc_set_output);

/**
 * function davinci_enc_set_mode
 * @channel: channel number.
 * @mode_info: ptr to vid_enc_mode_info structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   set standard or non-standard mode at current encoder's active output.
 * Encoder Manager first configure the VENC or associated SoC hardware
 * before calling the setmode() API of the encoder. To do so, encoder Manager
 * calls the getmode() to get the mode_info for this mode and configure the
 * mode based on the timing information present in this structure.
 */
int davinci_enc_set_mode(int channel, struct vid_enc_mode_info *mode_info)
{
	struct vid_encoder_device *cur_enc;
	struct vid_enc_device_mgr *mgr = 0;

	int err = -EINVAL;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->mode_ops || NULL == mode_info)
		return err;

	mgr = &enc_dev[channel];

	if (cur_enc->mode_ops->setmode != NULL) {
		if ((err = down_interruptible(&mgr->lock)) < 0)
			return err;
		err = cur_enc->mode_ops->setmode(mode_info, cur_enc);
		up(&mgr->lock);
	} else
		return err;

	if (err < 0)
		return err;

	err = cur_enc->mode_ops->getmode(&mgr->current_mode, cur_enc);
	if (err < 0)
		return err;

	err = 0;

	if ((err = down_interruptible(&mgr->lock)) < 0)
		return err;

	if (cur_enc->misc_ops != NULL && cur_enc->misc_ops->enable != NULL)
		err |= cur_enc->misc_ops->enable(0, cur_enc);

	/* non-standard */
	if (mgr->current_mode.std == 0) {
		davinci_enc_set_display_timing(&mgr->current_mode);
		up(&mgr->lock);
		return err;
	}

	davinci_enc_set_mode_platform(channel, mgr);

	if (cur_enc->misc_ops != NULL && cur_enc->misc_ops->reset != NULL)
		err |= cur_enc->misc_ops->reset(cur_enc);

	if (cur_enc->misc_ops != NULL && cur_enc->misc_ops->enable != NULL)
		err |= cur_enc->misc_ops->enable(1, cur_enc);

	up(&mgr->lock);

	return err;
}

EXPORT_SYMBOL(davinci_enc_set_mode);

/**
 * function davinci_enc_get_mode
 * @channel: channel number, starting index 0.
 * @mode_info: ptr to vid_enc_mode_info structure
 * This is updated by encoder manager
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   get video or graphics mode at current encoder's active output.
 *
*/
int davinci_enc_get_mode(int channel, struct vid_enc_mode_info *mode_info)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->mode_ops || NULL == mode_info)
		return -EINVAL;

	if (cur_enc->mode_ops->getmode != NULL)
		return cur_enc->mode_ops->getmode(mode_info, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_get_mode);

/**
 * function davinci_enc_get_output
 * @channel: channel number.
 * @output: ptr to array of char to hold output name.
 * size VID_ENC_NAME_MAX_CHARS
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Get output - Get channel's output. User call this to get the current
*   output name
 */
int davinci_enc_get_output(int channel, char *output)
{
	struct vid_encoder_device *cur_enc;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->output_ops || NULL == output)
		return -EINVAL;

	if (cur_enc->output_ops->getoutput != NULL)
		return cur_enc->output_ops->getoutput(output, cur_enc);
	else
		return 0;
}

EXPORT_SYMBOL(davinci_enc_get_output);

/**
 * vid_enc_register_encoder
 * @encoder: pointer to the encoder device structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Register the encoder module with the encoder manager
 * This is implemented by the encoder Manager
 */
int vid_enc_register_encoder(struct vid_encoder_device *encoder)
{
	int i, found = 0, err = 0;
	int ch_id = 0;
	struct vid_enc_device_mgr *mgr = 0;
	struct vid_encoder_device *cur_enc = 0;
	struct enc_config *enc_def = 0;
	struct vid_enc_mode_info mode_info;
	char buf[VID_ENC_NAME_MAX_CHARS];

	if (ISNULL(encoder))
		return -EINVAL;

	memset(&mode_info, 0, sizeof(struct vid_enc_mode_info));
	ch_id = encoder->channel_id;

	if ((ch_id > DAVINCI_ENC_MAX_CHANNELS) || (ch_id < 0))
		return -EINVAL;

	mgr = &enc_dev[ch_id];

	if (mgr->num_encoders > DAVINCI_ENC_MAX_ENCODERS)
		return -EINVAL;

	/* get module boot args */
	enc_def = &davinci_enc_default[ch_id];

	/* we'd assume only std mode being reqeusted from bootargs */
	mode_info.name = enc_def->mode;
	mode_info.std = 1;
	if (mgr->num_encoders == 0) {

		/* set default for the first encoder */
		err = encoder->initialize(encoder, VID_ENC_FULL_INIT_FLAG);

		if (err >= 0) {
			/* see if this has output requested by default
			   or bootarg */

			for (i = 0; i < encoder->output_ops->count; i++) {
				err =
				    encoder->output_ops->enumoutput(i, buf,
								    encoder);
				if (err != 0)
					return err;
				if (strcmp(buf, enc_def->output) == 0) {
					err =
					    encoder->output_ops->
					    setoutput(enc_def->output, encoder);
					found = 1;
					break;
				}
			}

			if (found == 1) {
				err =
				    encoder->mode_ops->setmode(&mode_info,
							       encoder);
				if (err != 0) {
					printk
					    (KERN_ERR
					     "Setmode failed, reset to encoder"
					     " default...\n");
					encoder->deinitialize(encoder);
					encoder->initialize(encoder, 0);
					err = 0;
				}
			}

			mgr->encoder[mgr->num_encoders++] = encoder;
			mgr->current_encoder = 0;
			encoder->output_ops->getoutput(mgr->current_output,
						       encoder);
			encoder->mode_ops->getmode(&mgr->current_mode, encoder);
			if ((err = down_interruptible(&mgr->lock)) < 0)
				return err;
			davinci_enc_set_mode_platform(ch_id, mgr);
			if (encoder->misc_ops != NULL
			    && encoder->misc_ops->reset != NULL)
				err |= encoder->misc_ops->reset(encoder);
			up(&mgr->lock);
			if (encoder->misc_ops != NULL
			    && encoder->misc_ops->enable != NULL)
				encoder->misc_ops->enable(1, encoder);
		}
	} else {
		/* If we have more than one encoder module, then we still
		   want to make sure we can set to the default output and
		   mode. If current one is not the default, try this new
		   one. If still no luck, just use the newly registered
		   one as default */
		cur_enc = davinci_get_cur_encoder(ch_id);

		if ((strcmp(mgr->current_output,
			    enc_def->output) != 0) && (cur_enc != NULL)) {
			cur_enc->deinitialize(cur_enc);

			err = encoder->initialize(encoder,
						  VID_ENC_FULL_INIT_FLAG);

			if (err < 0) {
				/* no change if new one cannot be
				   initialized */
				encoder->deinitialize(encoder);
				cur_enc->initialize(cur_enc, 0);
				return err;
			}
			/* look for matching output */
			for (i = 0; i < encoder->output_ops->count; i++) {
				err =
				    encoder->output_ops->enumoutput(i, buf,
								    encoder);
				if (err != 0)
					return err;
				if (strcmp(buf, enc_def->output) == 0) {
					err =
					    encoder->output_ops->
					    setoutput(enc_def->output, encoder);
					found = 1;
					break;
				}
			}

			if (found == 1) {
				err |=
				    encoder->mode_ops->setmode(&mode_info,
							       encoder);
				/* setmode went wrong, try redo init */
				if (err != 0) {
					printk
					    (KERN_ERR
					     "Set output or mode failed, "
					     "reset to encoder default...\n");
					encoder->deinitialize(encoder);
					encoder->initialize(encoder, 0);
					err = 0;
				}
			}

			mgr->encoder[mgr->num_encoders++] = encoder;
			mgr->current_encoder = mgr->num_encoders - 1;
			encoder->output_ops->getoutput(mgr->current_output,
						       encoder);
			encoder->mode_ops->getmode(&mgr->current_mode, encoder);
			if ((err = down_interruptible(&mgr->lock)) < 0)
				return err;
			davinci_enc_set_mode_platform(ch_id, mgr);
			if (encoder->misc_ops != NULL
			    && encoder->misc_ops->reset != NULL)
				err |= encoder->misc_ops->reset(encoder);
			up(&mgr->lock);
			if (encoder->misc_ops != NULL
			    && encoder->misc_ops->enable != NULL)
				encoder->misc_ops->enable(1, encoder);

		} else if (strcmp(mgr->current_output, enc_def->output) == 0) {
			/* no change in output or mode */
			mgr->encoder[mgr->num_encoders++] = encoder;

		}
	}

	return err;
}

EXPORT_SYMBOL(vid_enc_register_encoder);

/**
 * vid_enc_unregister_encoder
 * @encoder: pointer to the encoder device structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Register the encoder module with the encoder manager
 * This is implemented by the encoder Manager
 */
int vid_enc_unregister_encoder(struct vid_encoder_device *encoder)
{
	int i, j = 0, err = -EINVAL, ch_id;
	struct vid_enc_device_mgr *mgr = 0;

	if (ISNULL(encoder))
		return err;

	ch_id = encoder->channel_id;

	if ((ch_id > DAVINCI_ENC_MAX_CHANNELS) || (ch_id < 0))
		return err;

	mgr = &enc_dev[ch_id];

	for (i = 0; i < mgr->num_encoders; i++) {
		if (encoder == mgr->encoder[i]) {

			mgr->encoder[i] = NULL;
			for (j = i; j < mgr->num_encoders - 1; j++)
				mgr->encoder[j] = mgr->encoder[j + 1];
			mgr->num_encoders--;
			err = 0;
			break;
		}
	}
	return err;

}

EXPORT_SYMBOL(vid_enc_unregister_encoder);

/**
 * davinci_enc_enable_vbi
 * @flag: flag which tells whether to enable or disable raw vbi
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * This function is used to enable/disable RAW VBI sending in
 * the encoder.
 */
int davinci_enc_enable_vbi(int channel, int flag)
{
	struct vid_encoder_device *cur_enc = NULL;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->enable_vbi)
		return -EINVAL;

	return cur_enc->enable_vbi(flag, cur_enc);
}

EXPORT_SYMBOL(davinci_enc_enable_vbi);

/**
 * davinci_enc_enable_hbi
 * @flag: flag which tells whether to enable or disable raw hbi
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * This function is used to enable/disable RAW HBI sending in
 * the encoder.
 */
int davinci_enc_enable_hbi(int channel, int flag)
{
	struct vid_encoder_device *cur_enc = NULL;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->enable_hbi)
		return -EINVAL;

	return cur_enc->enable_hbi(flag, cur_enc);
}

EXPORT_SYMBOL(davinci_enc_enable_hbi);

/**
 * davinci_enc_enable_sliced_vbi
 * @channel: channel number.
 * @encoder: pointer to the encoder device structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Following funcion is used to enable support for
 * sending set of sliced vbi services. Caller calls
 * this function with pointer to the structure vid_enc_sliced_vbi_service
 */
int davinci_enc_enable_sliced_vbi(int channel,
				  struct vid_enc_sliced_vbi_service *services)
{
	struct vid_encoder_device *cur_enc = NULL;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->set_vbi_services)
		return -EINVAL;

	return cur_enc->set_vbi_services(services, cur_enc);
}

EXPORT_SYMBOL(davinci_enc_enable_sliced_vbi);

/**
 * davinci_enc_write_sliced_vbi_data
 * @encoder: pointer to the encoder device structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Register the encoder module with the encoder manager
 * This is implemented by the encoder Manager
 */
int davinci_enc_write_sliced_vbi_data(int channel,
				      struct vid_enc_sliced_vbi_data *data)
{
	struct vid_encoder_device *cur_enc = NULL;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->write_vbi_data)
		return -EINVAL;

	return cur_enc->write_vbi_data(data, cur_enc);
}

EXPORT_SYMBOL(davinci_enc_write_sliced_vbi_data);

int davinci_enc_get_sliced_cap(int channel,
			       struct vid_enc_sliced_vbi_service *service)
{
	struct vid_encoder_device *cur_enc = NULL;

	cur_enc = davinci_get_cur_encoder(channel);

	if (NULL == cur_enc || NULL == cur_enc->get_sliced_cap)
		return -EINVAL;

	return cur_enc->get_sliced_cap(service, cur_enc);
}

EXPORT_SYMBOL(davinci_enc_get_sliced_cap);

subsys_initcall(davinci_enc_mngr_init);
module_exit(davinci_enc_mngr_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DaVinci Encoder Manager");
MODULE_AUTHOR("Texas Instruments");
