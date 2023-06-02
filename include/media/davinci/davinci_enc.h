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
 *
 * davinci_enc.h.
 * This file contains APIs exported by Davinci Encoder Manager. Davinci Encoder
 * manager uses the services from encoder interface for implementing it's
 * functionality.
 * It also configure the VENC/DLCD to support a specific video/graphics mode
 * This interface allows set/get of output, mode , parameters and controls at
 * the encoder. encoder may support multiple channels, each with a set of
 * encoders So encoder manager support registration of encoders and APIs
 * on a per channel basis.
 */

#ifndef DAVINCI_ENC_H
#define DAVINCI_ENC_H

#ifdef __KERNEL__
#include <media/davinci/vid_encoder_types.h>

/**
 *  Maximum channels supported by encoder manager. Set to 1 for DM355 & DM6446.
 * If a platform support multiple channel, this needs to be changed. Channel
 * number starts with 0
 */
#define DAVINCI_ENC_MAX_CHANNELS	CONFIG_ENC_MNGR_MAX_CHANNELS

/**
 * function davinci_enc_setparams
 * @params: pointer to params structure.
 * @channel: channel number.
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Set parameters at current active encoder. params will be defined by
 * the specific encoder and used by user space applications to set
 * encoder parameters.
 */
int davinci_enc_setparams(int channel, void *params);

/**
 * function davinci_enc_getparams
 * @params: pointer to params structure.
 * @channel: channel number, 0 for first channel and so forth
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Get parameters at current active encoder. params will be defined by
 * the specific encoder and used by user space applications to get
 * encoder parameters.
 */
int davinci_enc_getparams(int channel, void *params);

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
int davinci_enc_set_mode(int channel, struct vid_enc_mode_info *mode_info);
/**
 * function davinci_enc_get_mode
 * @channel: channel number, starting index 0.
 * @mode_info: ptr to vid_enc_mode_info structure. This is updated by
 * encoder manager
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   get video or graphics mode at current encoder's active output.
 *
 */
int davinci_enc_get_mode(int channel, struct vid_enc_mode_info *mode_info);

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
int davinci_enc_set_control(int channel, enum vid_enc_ctrl_id ctrl, char val);

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
int davinci_enc_get_control(int channel, enum vid_enc_ctrl_id ctrl, char *val);

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
 * to set a default mode on this output internally. Manager calls the getmode()
 * to get information about the mode to configure the SoC hardware (VENC/DLCD
 * for Davinci/DM355. During configuration of the SoC hardware for timing,
 * mgr would call enable() to disable and re-enable the output of the encoder
 * to avoid noise at the display. It may also call reset() to make sure the
 * encoder is reset if required by the encoder hardware.
 */
int davinci_enc_set_output(int channel, char *output);

/**
 * function davinci_enc_get_output
 * @channel: channel number.
 * @output: ptr to array of char to hold output name. size
 * VID_ENC_NAME_MAX_CHARS
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 *   Get output - Get channel's output. User call this to get the current
 *   output name
 */
int davinci_enc_get_output(int channel, char *output);

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
int davinci_enc_reset(int channel);

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
int davinci_enc_enable_output(int channel, int flag);

/**
 * davinci_enc_enable_vbi
 * @flag: flag which tells whether to enable or disable raw vbi
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * This function is used to enable/disable RAW VBI sending in
 * the encoder.
 */
int davinci_enc_enable_vbi(int channel, int flag);

/**
 * davinci_enc_enable_hbi
 * @channel: channel number (0 to MAX_CHANNELS-1)
 * @flag: flag which tells whether to enable or disable raw hbi
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * This function is used to enable/disable RAW HBI sending in
 * the encoder.
 */
int davinci_enc_enable_hbi(int channel, int flag);

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
				  struct vid_enc_sliced_vbi_service *service);
/**
 * davinci_enc_enable_sliced_vbi
 * @channel: channel number.
 * @service: pointer to the structure vid_enc_sliced_vbi_service
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Following funcion is used to enable support for
 * sending set of sliced vbi services. Caller calls
 * this function with pointer to the structure vid_enc_sliced_vbi_service
 */
int davinci_enc_get_sliced_cap(int channel,
			       struct vid_enc_sliced_vbi_service *services);

/**
 * davinci_enc_write_sliced_vbi_data
 * @channel: channel number.
 * @data: pointer to the structure vid_enc_sliced_vbi_data
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Register the encoder module with the encoder manager
 * This is implemented by the encoder Manager
 */
int davinci_enc_write_sliced_vbi_data(int channel,
				      struct vid_enc_sliced_vbi_data *data);

#endif				/* #ifdef __KERNEL__ */
#endif				/* #ifdef DAVINCI_ENC_H */
