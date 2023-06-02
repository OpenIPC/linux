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
 * imp_common.h file
 **************************************************************************/
#ifndef _IMP_COMMON_H
#define _IMP_COMMON_H

#define IMP_MODE_CONTINUOUS	0
#define	IMP_MODE_SINGLE_SHOT	1
#define IMP_MODE_INVALID	2
#define IMP_MAX_NAME_SIZE	40

/* input buffer */
#define IMP_BUF_IN		0
/* output buffer */
#define IMP_BUF_OUT1		1
/* output buffer */
#define IMP_BUF_OUT2		2


/* structure for request buffer */
struct imp_reqbufs {
	/* type of frame buffer */
	int buf_type;
	/* size of the frame buffer to be allocated */
	int size;
	/* number of frame buffer to be allocated */
	int count;
};

/* structure buffer */
struct imp_buffer {
	/* index number, 0 -> N-1 */
	int index;
	/* buffer type, input or output */
	int buf_type;
	/* address of the buffer used in the mmap() system call */
	int offset;
	/* size of the buffer */
	int size;
};

struct imp_convert {
	struct imp_buffer in_buff;
	struct imp_buffer out_buff1;
	struct imp_buffer out_buff2;
};

enum imp_data_paths {
	IMP_RAW2RAW = 1,
	IMP_RAW2YUV = 2,
	IMP_YUV2YUV = 4
};

enum imp_pix_formats {
	IMP_BAYER_8BIT_PACK,
	IMP_BAYER_8BIT_PACK_ALAW,
	IMP_BAYER_8BIT_PACK_DPCM,
	IMP_BAYER_12BIT_PACK,
	IMP_BAYER, /* 16 bit */
	IMP_UYVY,
	IMP_YUYV,
	IMP_RGB565,
	IMP_RGB888,
	IMP_YUV420SP,
	IMP_420SP_Y,
	IMP_420SP_C,
};

struct imp_window {
	/* horizontal size */
	unsigned int width;
	/* vertical size */
	unsigned int height;
	/* horizontal start position */
	unsigned int hst;
	/* vertical start position */
	unsigned int vst;
};

/* structure used by application to query the modules
 * available in the image processorr for preview the input
 * image. Used for PREV_QUERY_CAP IOCTL
 */
struct prev_cap {
	/* application use this to iterate over the available
	 * modules. stop when -EINVAL return code is returned by
	 * the driver
	 */
	unsigned short index;
	/* Version of the preview module */
	char version[IMP_MAX_NAME_SIZE];
	/* Module IDs as defined above */
	unsigned short module_id;
	/* control operation allowed in continuous mode ?
	 * 1 - allowed, 0 - not allowed
	 */
	char control;
	/* path on which the module is sitting */
	enum imp_data_paths path;
	char module_name[IMP_MAX_NAME_SIZE];
};

/* struct to configure preview modules for which structures
 * are defined above. Used by PREV_SET_PARAM or PREV_GET_PARAM IOCTLs.
 */
struct prev_module_param {
	/* Version of the preview module */
	char version[IMP_MAX_NAME_SIZE];
	/* Length of the module config structure */
	unsigned short len;
	/* Module IDs as defined above */
	unsigned short module_id;
	/* Ptr to module config parameter. If SET command and is NULL
	 * module is reset to power on reset values
	 */
	void *param;
};

/* Structure for configuring the previewer driver.
 * Used in PREV_SET_CONFIG/PREV_GET_CONFIG IOCTLs
 */
struct prev_channel_config {
	/* 1 - single shot, 0 - continuous */
	unsigned int oper_mode;
	/* Length of the user configuration */
	unsigned short len;
	/* Ptr to either preview_single_shot_config or
	 * preview_continuous_config depending on oper_mode
	 */
	void *config;
};

struct prev_control {
	/* Version of the preview module */
	char version[IMP_MAX_NAME_SIZE];
	/* Length of the module config structure */
	unsigned short len;
	/* Module IDs as defined above */
	unsigned short module_id;
	/* Ptr to module config parameter. If SET command and is NULL
	 * module is reset to power on reset values
	 */
	void *param;
};

/* Structure for RSZ_SET_CONFIG and RSZ_GET_CONFIG IOCTLs */
struct rsz_channel_config {
	/* 1 - single shot, 0 - continuous */
	unsigned int oper_mode;
	/* Chain this resizer at the previewer output */
	unsigned char chain;
	/* Length of the user configuration */
	unsigned short len;
	/* ptr to either rsz_single_shot_config or rsz_continuous_config
	 * depending on oper_mode
	 */
	void *config;
};

/* RSZ_RECONFIG IOCTL. Used for re-configuring resizer
 * before doing RSZ_RESIZE. This is a IOCTL to do fast reconfiguration
 * of resizer. This assumes that corresponding resizer is already enabled
 * through SET_CONFIG. This is used when the input image to be resized
 * is either Y or C plane of a YUV 420 image. Typically, when channel is
 * first configured, it is set up to resize Y plane. Then if application
 * needs to resize C plane, this ioctl is called to switch the channel
 * to resize C plane.
 */
struct rsz_reconfig {
	enum imp_pix_formats pix_format;
};

/* Structure for setting dark frame in the IPIPE interface.
 * Used in the PREV_SET_DARK_FRAME IOCTL
 * Application captures a dark frame from CCDC with camera shutter
 * closed and then call this ioctl to set this frame in ipipe interface.
 * IPIPE operates in continuous mode for dark frame subtraction.
 */
struct prev_dark_frame {
	/* address of the buffer used in the mmap system call.
	   This buffer has user captured dark frame
	*/
	int offset;
	/* size of the buffer */
	int size;
	/* width of the dark frame. Should match with current
	 * input capture area at CCDC driver
	 */
	int width;
	/* height of the dark frame. Should match with current
	 * input capture area at the ccdc driver
	 */
	int height;
};

#ifdef __KERNEL__
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#define MAX_CHANNELS		2
#define	MAX_BUFFERS		6
#define	MAX_PRIORITY		5
#define	MIN_PRIORITY		0
#define	DEFAULT_PRIORITY	3
#define ENABLED			1
#define DISABLED		0
#define CHANNEL_BUSY		1
#define CHANNEL_FREE		0
#define ISNULL(val) ((val == NULL) ? 1:0)

/* driver configured by application */
#define STATE_CONFIGURED	1
/* driver not configured by application */
#define STATE_NOT_CONFIGURED	0

enum imp_log_chan_t {
	IMP_PREVIEWER,
	IMP_RESIZER,
	IMP_HISTOGRAM,
	IMP_BOXCAR
};

struct irq_numbers {
	int sdram;
	int update;
};

/* IMP channel structure */
struct imp_logical_channel {
	/* channel type */
	enum imp_log_chan_t type;
	/* operation mode */
	unsigned int mode;
	/* If this channel is chained with another channel, this is set */
	char chained;
	/* Set if there is a primary user of this channel */
	char primary_user;
	/* channel configuration for this logial channel */
	void *config;
	/* Size of the user configuration block */
	int user_config_size;
	/* Saves the user configuration */
	void *user_config;
	/* configure State of the channel */
	unsigned int config_state;
	/* number of input buffers */
	unsigned char in_numbufs;
	/* number of output buffers1 */
	unsigned char out_numbuf1s;
	/* number of output buffers1 */
	unsigned char out_numbuf2s;
	/* input buffers */
	struct imp_buffer *in_bufs[MAX_BUFFERS];
	/* output buffer1s */
	struct imp_buffer *out_buf1s[MAX_BUFFERS];
	/* output buffer2s. Used only by resizes */
	struct imp_buffer *out_buf2s[MAX_BUFFERS];
	/* stores priority of the application */
	int priority;
	/* channel processing completion */
	struct completion channel_sem;
	/* channel protection lock */
	struct mutex lock;
};

/* Where hardware channel is shared, this is used for serialisation */
struct imp_serializer {
	/* channel config array for serialization */
	struct imp_logical_channel *channel_config[MAX_CHANNELS];
	/* number of elements in the array */
	int array_count;
	/* Semaphore for above config array */
	struct mutex array_sem;
	/* Completion semaphore when hw channel is common
	 * Use device specific completion semaphore when request is serialized
	 */
	struct completion sem_isr;
};

/* function prototypes */
int imp_common_free_buffers(struct device *dev,
			struct imp_logical_channel *channel);

int imp_common_mmap(struct file *filp,
			struct vm_area_struct *vma,
			struct imp_logical_channel *channel);

int imp_common_query_buffer(struct device *dev,
		struct imp_logical_channel *channel,
		struct imp_buffer *buffer);

int imp_common_start_preview(struct device *dev,
		struct imp_logical_channel *chan,
		struct imp_convert *convert);

int imp_common_request_buffer(struct device *dev,
		struct imp_logical_channel *channel,
		struct imp_reqbufs *reqbufs);

struct prev_module_if *imp_get_module_interface(struct device *dev,
		unsigned short module_id);

int imp_set_preview_config(struct device *dev,
				struct imp_logical_channel *channel,
				struct prev_channel_config *config);

int imp_get_preview_config(struct device *dev,
				struct imp_logical_channel *channel,
				struct prev_channel_config *config);

int imp_init_serializer(void);


int imp_set_resizer_config(struct device *dev,
				struct imp_logical_channel *channel,
				struct rsz_channel_config *chan_config);

int imp_common_start_resize(struct device *dev,
		struct imp_logical_channel *chan,
		struct imp_convert *convert);

int imp_get_resize_config(struct device *dev,
				struct imp_logical_channel *channel,
				struct rsz_channel_config *chan_config);

int imp_common_reconfig_resizer(struct device *dev,
			struct rsz_reconfig *reconfig,
			struct imp_logical_channel *chan);

#endif
#endif
