/*
* Copyright (C) 2008-2009 Texas Instruments Inc
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

#ifndef _VPFE_CAPTURE_H
#define _VPFE_CAPTURE_H

#ifdef __KERNEL__

/* Header files */
#include <media/v4l2-dev.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/videobuf-dma-contig.h>
#include <media/davinci/vpfe_types.h>

#define VPFE_CAPTURE_NUM_DECODERS        5

/* Macros */
#define VPFE_MAJOR_RELEASE              0
#define VPFE_MINOR_RELEASE              0
#define VPFE_BUILD                      1
#define VPFE_CAPTURE_VERSION_CODE       ((VPFE_MAJOR_RELEASE << 16) | \
					(VPFE_MINOR_RELEASE << 8)  | \
					VPFE_BUILD)

#define CAPTURE_DRV_NAME		"vpfe-capture"

struct vpfe_pixel_format {
	struct v4l2_fmtdesc fmtdesc;
	/* bytes per pixel */
	int bpp;
	/* decoder format */
	u32 subdev_pix_fmt;
};

struct vpfe_std_info {
	int active_pixels;
	int active_lines;
	/* current frame format */
	int frame_format;
	struct v4l2_fract fps;
};

struct vpfe_route {
	u32 input;
	u32 output;
};

enum vpfe_subdev_id {
	VPFE_SUBDEV_TVP5146 = 1,
	VPFE_SUBDEV_MT9T031 = 2,
	VPFE_SUBDEV_TVP7002 = 3,
	VPFE_SUBDEV_MT9P031 = 4,
	VPFE_SUBDEV_OV2643 = 5,
	VPFE_SUBDEV_OV7690 = 6
};

struct vpfe_subdev_info {
	/* Sub device module name */
	char module_name[32];
	/* Sub device group id */
	int grp_id;
	/* Number of inputs supported */
	int num_inputs;
	/* inputs available at the sub device */
	struct v4l2_input *inputs;
	/* Sub dev routing information for each input */
	struct vpfe_route *routes;
	/* ccdc bus/interface configuration */
	struct vpfe_hw_if_param ccdc_if_params;
	/* i2c subdevice board info */
	struct i2c_board_info board_info;
	/* Is this a camera sub device ? */
	unsigned is_camera:1;
	/* check if sub dev supports routing */
	unsigned can_route:1;
	/* registered ? */
	unsigned registered:1;
};

struct vpfe_config {
	/* Number of sub devices connected to vpfe */
	int num_subdevs;
	/* information about each subdev */
	struct vpfe_subdev_info *sub_devs;
	/* evm card info */
	char *card_name;
	/* ccdc name */
	char *ccdc;
	/* setup function for the input path */
	int (*setup_input)(enum vpfe_subdev_id id);
	/* number of clocks */
	int num_clocks;
	/* clocks used for vpfe capture */
	char *clocks[];
};

/* TODO - revisit for MC */
enum output_src {
	VPFE_CCDC_OUT,
	VPFE_IMP_PREV_OUT,
	VPFE_IMP_RSZ_OUT
};

struct vpfe_device {
	/* V4l2 specific parameters */
	/* Identifies video device for this channel */
	struct video_device *video_dev;
	/* sub devices */
	struct v4l2_subdev **sd;
	/* vpfe cfg */
	struct vpfe_config *cfg;
	/* clock ptrs for vpfe capture */
	struct clk **clks;
	/* V4l2 device */
	struct v4l2_device v4l2_dev;
	/* parent device */
	struct device *pdev;
	/* Used to keep track of state of the priority */
	struct v4l2_prio_state prio;
	/* number of open instances of the channel */
	u32 usrs;
	/* Indicates id of the field which is being displayed */
	u32 field_id;
	/* flag to indicate whether decoder is initialized */
	u8 initialized;
	/* TODO for MC. Previewer is always present if IMP is chained */
	unsigned char imp_chained;
	/* Resizer is chained at the output of previewer */
	unsigned char rsz_present;
	/* if second resolution output is present */
	unsigned char second_output;
	/* offset where second buffer starts from the starting of
	 * the buffer. This is for storing the second IPIPE resizer
	 * output
	 */
	u32 second_off;
	/* Size of second output image */
	int second_out_img_sz;
	/* output from CCDC or IPIPE */
	enum output_src out_from;
	/* skip frame count */
	u8 skip_frame_count;
	/* skip frame count init value */
	u8 skip_frame_count_init;
	/* time per frame for skipping */
	struct v4l2_fract timeperframe;
	/* ptr to currently selected sub device */
	struct vpfe_subdev_info *current_subdev;
	/* current input at the sub device */
	int current_input;
	/* Keeps track of the information about the standard */
	struct vpfe_std_info std_info;
	/* std index into std table */
	int std_index;
	/* IRQ number for DMA transfer completion at the image processor */
	unsigned int imp_dma_irq;
	/* IRQ number for Update resizer imp registers */
	unsigned int imp_update_irq;
	/* CCDC IRQs used when CCDC/ISIF output to SDRAM */
	unsigned int ccdc_irq0;
	unsigned int ccdc_irq1;
	/* number of buffers in fbuffers */
	u32 numbuffers;
	/* List of buffer pointers for storing frames */
	u8 *fbuffers[VIDEO_MAX_FRAME];
	/* Pointer pointing to current v4l2_buffer */
	struct videobuf_buffer *cur_frm;
	/* Pointer pointing to next v4l2_buffer */
	struct videobuf_buffer *next_frm;
	/*
	 * This field keeps track of type of buffer exchange mechanism
	 * user has selected
	 */
	enum v4l2_memory memory;
	/* Used to store pixel format */
	struct v4l2_format fmt;
	/*
	 * used when IMP is chained to store the crop window which
	 * is different from the image window
	 */
	struct v4l2_rect crop;
	/* Buffer queue used in video-buf */
	struct videobuf_queue buffer_queue;
	/* Queue of filled frames */
	struct list_head dma_queue;
	/* Used in video-buf */
	spinlock_t irqlock;
	/* IRQ lock for DMA queue */
	spinlock_t dma_queue_lock;
	/* lock used to access this structure */
	struct mutex lock;
	/* number of users performing IO */
	u32 io_usrs;
	/* Indicates whether streaming started */
	u8 started;
	/*
	 * offset where second field starts from the starting of the
	 * buffer for field seperated YCbCr formats
	 */
	u32 field_off;
};

/* File handle structure */
struct vpfe_fh {
	struct vpfe_device *vpfe_dev;
	/* Indicates whether this file handle is doing IO */
	u8 io_allowed;
	/* Used to keep track priority of this instance */
	enum v4l2_priority prio;
};

struct vpfe_config_params {
	u8 min_numbuffers;
	u8 numbuffers;
	u32 min_bufsize;
	u32 device_bufsize;
	u32 video_limit;
};

#endif				/* End of __KERNEL__ */
/**
 * VPFE_CMD_S_CCDC_RAW_PARAMS - EXPERIMENTAL IOCTL to set raw capture params
 * This can be used to configure modules such as defect pixel correction,
 * color space conversion, culling etc. This is an experimental ioctl that
 * will change in future kernels. So use this ioctl with care !
 * TODO: This is to be split into multiple ioctls and also explore the
 * possibility of extending the v4l2 api to include this
 **/
#define VPFE_CMD_S_CCDC_RAW_PARAMS _IOW('V', BASE_VIDIOC_PRIVATE + 1, \
					void *)
#define VPFE_CMD_G_CCDC_RAW_PARAMS _IOR('V', BASE_VIDIOC_PRIVATE + 2, \
					void *)

#endif				/* _DAVINCI_VPFE_H */
