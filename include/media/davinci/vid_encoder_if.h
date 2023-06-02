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
 * File: vid_encoder_if.h
 * Description:
 * ============
 * This file defines the interface to allow abstraction of an encoder hardware
 * to an encoder module. Each encoder module support one or more display
 * channels. It registers with the encoder manager at init and
 * unregisters at exit. Encoder modules implement a set of operations defines
 * by this interface. For each display channel, encoder manager selects
 * one of the encoder registered with it to support the user specified output
 * and video/graphics modes on that output. First step is to set the output
 * as requested by the user. To do so, encoder manager iterates through
 * all supported encoders to match a given output name as requested by the user
 * by calling enumoutput(). Once there is a match, this encoder is set as the
 * current active encoder. Encoder manager calls initialize() to init the
 * encoder and then calls setouput() set the output. If another encoder was
 * active before this, encoder manager calls deinitialize() to make it
 * inactive. Encoder must select a default mode for this output as part of
 * setoutput() call. To set a mode, user call setmode(). All output names and
 * mode names are defined in vid_encoder_types.h
 */

#ifndef VID_ENCODER_IF_H
#define VID_ENCODER_IF_H

#ifdef __KERNEL__
#include <media/davinci/vid_encoder_types.h>

struct vid_encoder_device;
/**
 * struct vid_enc_param_ops
 * @setparams:
 * @getparams:
 *
 * Description:
 * Operation used for setting and getting encoder specific parameters.
 * Operations are applied to the current encoder selected by the
 * encoder manager
 * @setparams: pointer to setparams function. This Set Encoder parameters.
 *    @params - params structure type ptr declared by encoder
 *    @enc - encoder device ptr
 *    Returns - zero on success and non-zero otherwise
 * @getparams: pointer to getparams function. This Get encoder parameters
 *    @params - params structure type ptr declared by encoder
 *    @enc - encoder device ptr
 *    Returns - zero on success and non-zero otherwise
 */
struct vid_enc_param_ops {
	int (*setparams) (void *params, struct vid_encoder_device * enc);
	int (*getparams) (void *params, struct vid_encoder_device * enc);
};

/**
 * struct vid_enc_mode_ops
 * @setmode:
 * @getmode:
 *
 * Description:
 *  video/graphics mode ops structure. Operations are applied to the
 *  current output selected by the encoder
 * @setmode: Function pointer to setmode. set video/graphics mode or standard
 *    for current output encoder first check if the std flag is present in the
 *    mode_info. if yes, it will use name field to match a standard mode. If std
 *    flag is reset, it may reject the command if non-standard mode
 *    is not implemented. If non-standard mode is implemented, it would
 *    save the timing information in the internal structure with a
 *    key,VID_ENC_STD_NON_STANDARD as defined in vid_encoder_types.h
 *    It is expected that only one such instance is defined in the video driver.
 *    Please consider defining a mode in the vid_encoder_types.h
 *    and use that instead if encoder is to be extended to support additional
 *    modes not already supported by existing encoders.
 *    mode_info - ptr to mode information structure.
 *    enc - encoder device ptr.
 *    Returns - zero on success and non-zero otherwise.
 * @getmode: Function pointer to getmode. get current video/graphics mode
 *    information at the outpu.t
 *    mode_info - ptr to mode information structure.
 *    enc - encoder device ptr.
 *    Returns - zero on success and non-zero otherwise.
 */
struct vid_enc_mode_ops {
	int (*setmode) (struct vid_enc_mode_info * mode_info,
			struct vid_encoder_device * enc);
	int (*getmode) (struct vid_enc_mode_info * mode_info,
			struct vid_encoder_device * enc);
};

/**
 * struct vid_enc_control_ops
 * @setcontrol:
 * @getcontrol:
 *
 * Description:
 * Control ops structure. Control operations are performed for the current
 * output. Control brightness, hue, contrast, saturation etc.
 * @setcontrol: Function pointer to setcontrol. Set control value.
 *   ctrl - control type.
 *   val - value to be set for the control.
 *   enc - ptr to encoder device.
 *   Returns - zero on success and non-zero otherwise.
 * @getcontrol: Function pointer to getcontrol. Get control value.
 *   ctrl - control type.
 *   val - ptr to value that gets updated
 *   enc - ptr to encoder device
 *   Returns - zero on success and non-zero otherwise
 */
struct vid_enc_control_ops {
	int (*setcontrol) (enum vid_enc_ctrl_id ctrl,
			   unsigned char val, struct vid_encoder_device * enc);
	int (*getcontrol) (enum vid_enc_ctrl_id ctrl,
			   unsigned char *val, struct vid_encoder_device * enc);
};

/**
 * struct vid_enc_output_ops
 * @count: Indicates number of outputs supported
 * @enumoutput:
 * @setoutput:
 * @getoutput:
 *
 * Description:
 * output ops structure. enumoutput() is called to iterate
 * through all outputs supported by the encoder. Encoder manager ues
 * this API to select an encoder based on the output to be supported.
 * setoutput is called to set output at the encoder. getoutput is called
 * for getting the current output at the encoder.
 * @enumoutput:
 *   Function pointer.
 *   index - index of the output.
 *   output - ptr to char array to hold output name.(size
 *   		VID_ENC_NAME_MAX_CHARS).
 *   enc - pointer to encoder device.
 *   Returns - zero on success and non-zero otherwise.
 *   enumerates the outputs supported by encoder. To iterate the output, caller
 *   call this successively, starting with an index set to 0, and incrementing
 *   the same until this function returns non-zero. Each time name of the output
 *   at the given index is copied to output. Beware that this API assumes the
*   passed in output array size is VID_ENC_NAME_MAX_CHARS.
 * @setoutput:
 *   Function pointer.
 *   setoutput to the given output identifed by name. Encoder also must set a
 *   default mode on the selected output.
*   output - ptr to output name string.
*   enc - pointer to encoder device.
 *   Returns - zero on success and non-zero otherwise.
 * @getoutput:
 *   Function pointer.
 *   getoutput returns name of current output selected at the encoder.
 *   output - ptr to char array to hold the output name. (size
 *   		VID_ENC_NAME_MAX_CHARS).
 *   enc - pointer to encoder device.
 *   Returns - zero on success and non-zero otherwise.
 */
struct vid_enc_output_ops {
	int count;
	int (*enumoutput) (int index,
			   char *output, struct vid_encoder_device * enc);
	int (*setoutput) (char *output, struct vid_encoder_device * enc);
	int (*getoutput) (char *output, struct vid_encoder_device * enc);
};

/**
 * struct vid_enc_misc_ops
 * @reset:
 * @enable: *
 * Description:
 *  misc ops structure. Applied for the current encoder selected by the
 *  encoder manager
 * @reset: Function pointer to reset.
 *    For soft reset. Encoder may implement a soft reset that doesn't affect
 *    any configured registers. Some of the encoder hardware may require
 *    this if the input signal timings are changed after configuring it's
 *    mode registers.
 *    enc - ptr to encoder device.
 *    Returns - zero on success and non-zero otherwise
 * @enable: Function pointer to enable.
 *    enable/disable output. To eliminate noise on the display during SoC mode
 *    timing configuration, user may call this function to switch Off/On
 *    the output. Encoder must implement this API call if there is a hardware
 *    support for the same.
 *    flag - 0 - disable, 1 - enable.
 *    enc - ptr to encoder device.
 *    Returns - zero on success and non-zero otherwise
 */
struct vid_enc_misc_ops {
	int (*reset) (struct vid_encoder_device * enc);
	int (*enable) (int flag, struct vid_encoder_device * enc);
};

/**
 * struct vid_encoder_device
 * @name: Name of the encoder device
 * @channel_id:  Id of the channel to which encoder is connected
 * @capabilities: encoder capabilities. This is only for VBI capabilities
 * @initialize: Pointer to initialize function to initialize encoder
 * @mode_ops: Set of functions pointers for standard related functions
 * @ctrl_ops: Set of functions pointers for control related functions
 * @output_ops: Set of functions pointers for output related functions
 * @fmt_ops: Set of functions pointers for format related functions
 * @params_ops: Set of functions pointers for params related functions
 * @misc_ops: Set of functions pointers for miscellaneous functions functions
 * @deinitialize: functions pointer to de-initialize functio
 * @start_display: function to start display.
 * @stop_display: function to stop display.
 * @write_vbi_data: function to write sliced vbi data.
 * @enable_vbi: Function to enable support for RAW VBI.
 * @enable_hbi: Function to enable support for RAW HBI.
 * @set_vbi_services: function to enable sliced vbi services
 *
 * Description:
 * output ops structure
 *   Channel_id is used when encoder support multiple channels. In this case
 *   encoder module will use the channel id to select the channel for
 *   which the operation applies.
 *   initialize() called by encoder manager to initialize the encoder. Usually
 *   called before invoking any operations on the encoder.flag may be used by
 *   the encoder module to do different level of initialization. Encoder
 *   module must set a default output and mode in this code.
 *   deinitialize() called to deinitialize the current encoder that is active
 *   before initializing the new encoder.
 */
struct vid_encoder_device {
	u8 name[VID_ENC_NAME_MAX_CHARS];
	int channel_id;
	u32 capabilities;
	int (*initialize) (struct vid_encoder_device * enc, int flag);
	struct vid_enc_mode_ops *mode_ops;
	struct vid_enc_control_ops *ctrl_ops;
	struct vid_enc_output_ops *output_ops;
	struct vid_enc_param_ops *params_ops;
	struct vid_enc_misc_ops *misc_ops;
	int (*write_vbi_data) (struct vid_enc_sliced_vbi_data * data,
			       struct vid_encoder_device * enc);
	int (*enable_vbi) (int flag, struct vid_encoder_device * enc);
	int (*enable_hbi) (int flag, struct vid_encoder_device * enc);
	int (*set_vbi_services) (struct vid_enc_sliced_vbi_service * services,
				 struct vid_encoder_device * enc);
	int (*get_sliced_cap) (struct vid_enc_sliced_vbi_service *,
			       struct vid_encoder_device * enc);
	int (*deinitialize) (struct vid_encoder_device * enc);
	int (*start_display) (struct vid_encoder_device * enc);
	int (*stop_display) (struct vid_encoder_device * enc);
};

/**
 * vid_enc_register_encoder
 * @encoder: pointer to the encoder device structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Register the encoder module with the encoder manager
 * This is implemented by the encoder Manager
 */
int vid_enc_register_encoder(struct vid_encoder_device
			     *encoder);

/**
 * vid_enc_unregister_encoder
 * @encoder: pointer to the encoder device structure
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Unregister the encoder module with the encoder manager
 * This is implemented by the encoder Manager
 */
int vid_enc_unregister_encoder(struct vid_encoder_device
			       *encoder);

#endif				/* #ifdef __KERNEL__ */

#endif				/* #ifdef VID_ENCODER_IF_H */
