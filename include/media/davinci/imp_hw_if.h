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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 **************************************************************************/

#ifndef _IMP_HW_IF_H
#define _IMP_HW_IF_H

#ifdef __KERNEL__
#include <media/davinci/imp_common.h>
#include <media/davinci/vpfe_types.h>

struct prev_module_if {
	/* Version of the preview module */
	char *version;
	/* Module IDs as defined above */
	unsigned short module_id;
	/* Name of the module */
	char *module_name;
	/* control allowed in continous mode ? 1 - allowed , 0 - not allowed */
	char control;
	/* path in which module sits */
	enum imp_data_paths path;
	int (*set)(struct device *dev, void *param, int len);
	int (*get)(struct device *dev, void *param, int len);
};

struct imp_hw_interface {
	/* Name of the image processor hardware */
	char *name;
	/* module owner */
	struct module *owner;
	/*
	 * enumerate preview modules. Return interface to the
	 * the module
	 */
	struct prev_module_if *(*prev_enum_modules) (struct device *dev,
						     int index);
	/*
	 *  get preview operation mode
	 */
	unsigned int (*get_preview_oper_mode) (void);
	/* get resize operation mode */
	unsigned int (*get_resize_oper_mode) (void);
	/* check if hw is busy in continuous mode.
	 * Used for checking if hw is used by ccdc driver in
	 * continuous mode. If streaming is ON, this will be
	 * set to busy
	 */
	unsigned int (*get_hw_state) (void);
	/* set hw state */
	void (*set_hw_state) (unsigned int state);
	/* is resizer chained ? */
	unsigned int (*resizer_chain) (void);
	/* this is used to lock shared resource */
	void (*lock_chain) (void);
	/* this is used unlock shared resouce */
	void (*unlock_chain) (void);
	/* Allocate a shared or exclusive config block for hardware
	 * configuration
	 */
	void *(*alloc_config_block) (struct device *dev, int shared);
	/* hw serialization enabled ?? */
	int (*serialize) (void);
	/* De-allocate the exclusive config block */
	void (*dealloc_config_block) (struct device *dev, void *config);
	/* Allocate a user confguration block */
	void *(*alloc_user_config_block) (struct device *dev,
					  enum imp_log_chan_t chan_type,
					  unsigned int oper_mode, int *len);

	/* de-allocate user config block */
	void (*dealloc_user_config_block) (struct device *dev, void *config);

	/* set default configuration in the config block */
	void (*set_user_config_defaults) (struct device *dev,
					  enum imp_log_chan_t chan_type,
					  unsigned int oper_mode,
					  void *user_config);
	/* set user configuration for preview */
	int (*set_preview_config) (struct device *dev,
				   unsigned int oper_mode,
				   void *user_config, void *config);
	/* set user configuration for resize */
	int (*set_resizer_config) (struct device *dev,
				   unsigned int oper_mode,
				   int resizer_chained,
				   void *user_config, void *config);

	int (*reconfig_resizer) (struct device *dev,
				struct rsz_reconfig *user_config,
				void *config);

	/* update output buffer address for a channel
	 * if config is NULL, the shared config is assumed
	 * this is used only in single shot mode
	 */
	int (*update_inbuf_address) (void *config, unsigned int address);
	/* update output buffer address for a channel
	 * if config is NULL, the shared config is assumed
	 */
	int (*update_outbuf1_address) (void *config, unsigned int address);
	/* update output buffer address for a channel
	 * if config is NULL, the shared config is assumed
	 */
	int (*update_outbuf2_address) (void *config, unsigned int address);
	/* enable or disable hw */
	void (*enable) (unsigned char en, void *config);
	/* enable or disable resizer to allow frame by frame resize in
	 * continuous mode
	 */
	void (*enable_resize) (int en);
	/* setup hardware for processing. if config is NULL,
	 * shared channel is assumed
	 */
	int (*hw_setup) (struct device *dev, void *config);
	/* Get preview irq numbers */
	void (*get_preview_irq) (struct irq_numbers *irq);
	/* Get resize irq numbers */
	void (*get_rsz_irq) (struct irq_numbers *irq);
	/* Get configuration state of resizer in continuous mode */
	unsigned int (*get_resizer_config_state) (void);
	/* Get configuration state of previewer in continuous mode */
	unsigned int (*get_previewer_config_state) (void);

	/* Below APIs assume we are using shared configuration since
	 * oper mode is continuous
	 */
	/* Set the input crop window at the IMP interface and IMP */
	int (*set_input_win) (struct imp_window *win);
	/* Get current input crop window param at the IMP */
	int (*get_input_win) (struct imp_window *win);
	/* Set interface parameter at IPIPEIF. Only valid for DM360 */
	int (*set_hw_if_param) (struct vpfe_hw_if_param *param);
	/* Set input pixel format */
	int (*set_in_pixel_format) (enum imp_pix_formats pix_fmt);
	/* set output pixel format */
	int (*set_out_pixel_format) (enum imp_pix_formats pix_fmt);
	/* 0 - interleaved, 1 - field seperated */
	int (*set_buftype) (unsigned char buf_type);
	/* 0 - interlaced, 1 - progressive */
	int (*set_frame_format) (unsigned char frm_fmt);
	/* Set the output window at the IMP, output selection
	 * done by out_sel. 0 - output 1 and 1 - output 2
	 */
	int (*set_output_win) (struct imp_window *win);
	/* Get output enable/disable status */
	int (*get_output_state) (unsigned char out_sel);
	/* Get output line lenght */
	int (*get_line_length) (unsigned char out_sel);
	/* Get the output image height */
	int (*get_image_height) (unsigned char out_sel);
	/* Get current output window param at the IMP */
	int (*get_output_win) (struct imp_window *win);
	/* Dump HW configuration to console. only for debug purpose */
	void (*dump_hw_config) (void);
	/* get maximum output width of rsz-a or rsz_b*/
	int (*get_max_output_width) (int rsz);
	/* get maximum output height of rsa-a or rsz-b */
	int (*get_max_output_height) (int rsz);
	/* Enumerate pixel format for a given input format */
	int (*enum_pix) (u32 *output_pix, int index);
#ifdef CONFIG_VIDEO_YCBCR
	int (*set_ipipif_addr ) (struct device *dev, void *config, unsigned int address);
#endif

};

struct imp_hw_interface *imp_get_hw_if(void);

#endif
#endif
