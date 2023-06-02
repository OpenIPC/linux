/* *
* Copyright (C) 2006 Texas Instruments Inc
*
* This program is free software you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation either version 2 of the License, or
* (at your option) any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not,write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
#ifndef	DAVINVI_RESIZER_H
#define	DAVINVI_RESIZER_H

#ifdef __KERNEL__
#include <linux/io.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <mach/hardware.h>
#define CONFIG_PREEMPT_RT
#ifdef CONFIG_PREEMPT_RT
#include <linux/completion.h>
#endif

#endif

/* ioctls definition */
#pragma	pack(1)
#define		RSZ_IOC_BASE			       'R'
#define		RSZ_IOC_MAXNR				11

/* Ioctl options which are to be passed while calling the ioctl */
#define	RSZ_REQBUF		_IOWR(RSZ_IOC_BASE, 1, struct rsz_reqbufs *)
#define	RSZ_QUERYBUF		_IOWR(RSZ_IOC_BASE, 2, struct rsz_buffer *)
#define	RSZ_S_PARAM		_IOWR(RSZ_IOC_BASE, 3, struct rsz_params *)
#define	RSZ_G_PARAM		_IOWR(RSZ_IOC_BASE, 4, struct rsz_params *)
#define	RSZ_RESIZE		_IOWR(RSZ_IOC_BASE, 5, struct rsz_resize *)
#define	RSZ_G_STATUS		_IOWR(RSZ_IOC_BASE, 6, struct rsz_status *)
#define	RSZ_S_PRIORITY		_IOWR(RSZ_IOC_BASE, 7, struct rsz_priority *)
#define	RSZ_G_PRIORITY		_IOWR(RSZ_IOC_BASE, 9, struct rsz_priority *)
#define	RSZ_GET_CROPSIZE	_IOWR(RSZ_IOC_BASE, 10, struct rsz_cropsize *)
#define	RSZ_S_EXP		_IOWR(RSZ_IOC_BASE, 11, int *)
#pragma	pack()

#define	RSZ_BUF_IN			0
#define	RSZ_BUF_OUT			1
#define	RSZ_YENH_DISABLE		0
#define	RSZ_YENH_3TAP_HPF		1
#define	RSZ_YENH_5TAP_HPF		2

#ifdef __KERNEL__
/* Defines and Constants */
#define	MAX_BUFFER			3
#define	MAX_CHANNELS			16
#define	MAX_PRIORITY			5
#define	MIN_PRIORITY			0
#define	DEFAULT_PRIORITY		3
#define	MAX_IMAGE_WIDTH			1280
#define	MAX_IMAGE_WIDTH_HIGH		640
#define	MAX_IMAGE_HEIGHT		960
#define	MAX_INPUT_BUFFERS		8
#define	MAX_OUTPUT_BUFFERS		8
#define	DRIVER_NAME			"Resizer"
#define	FREE_BUFFER			0
#define	ALIGNMENT			16
#define	CHANNEL_BUSY			1
#define	CHANNEL_FREE			0
#define	PIXEL_EVEN			2
#define	RATIO_MULTIPLIER		256

/* Bit position	Macro */
/* macro for bit set and clear */
#define	BITSET(variable, bit)		((variable) | (1 << bit))
#define	BITRESET(variable, bit)		((variable) & (~(0x00000001 << (bit))))

/* RSZ_CNT */
#define	CSL_RESZ_RSZ_CNT_CBILIN_MASK	(0x20000000u)
#define	CSL_RESZ_RSZ_CNT_CBILIN_SHIFT	(0x0000001Du)

#define	RSZ_VSTPH_MASK			(0xfC7fffffu)
#define	RSZ_HSTPH_MASK			(0xff8fffffu)

#define	RSZ_CNT_VRSZ_MASK		(0xfff002ffu)
#define	RSZ_CNT_HRSZ_MASK		(0xfffffc00u)
/* OUT_SIZE */
#define	RSZ_OUT_SIZE_VERT_MASK		(0xf800ffffu)
#define	RSZ_OUT_SIZE_HORZ_MASK		(0xfffff800u)
/* IN_START	*/

#define	RSZ_IN_START_VERT_ST_MASK	(0xE000FFFFu)
#define	RSZ_IN_START_HORZ_ST_MASK	(0xFFFFE000u)
/* IN_SIZE */
#define	RSZ_IN_SIZE_VERT_MASK		(0xe000ffffu)
#define	RSZ_IN_SIZE_HORZ_MASK		(0xffffe000u)
/* SDR_INOFF */
#define	RSZ_SDR_INOFF_OFFSET_MASK	(0xffff0000u)
#define	RSZ_SDR_OUTOFF_OFFSET_MASK	(0xffff0000u)
#define	RSZ_UWORD_MASK			(0x03FF0000u)
#define	RSZ_LWORD_MASK			(0x000003FFu)
/* YENH	*/
#define	RSZ_YEHN_CORE_MASK		(0xffffff00u)
#define	RSZ_YEHN_SLOP_MASK		(0xfffff0ffu)
#define	RSZ_YEHN_GAIN_MASK		(0xffff0fffu)
#define	RSZ_YEHN_ALGO_MASK		(0xfffcffffu)
/* Filter coeefceints */
#define	RSZ_FILTER_COEFF0_MASK		(0xfffffc00u)
#define	RSZ_FILTER_COEFF1_MASK		(0xfc00ffffu)
#define	RSZ_CNT_CBILIN_MASK		(0x20000000u)
#define	RSZ_CNT_INPTYP_MASK		(0x08000000u)
#define	RSZ_CNT_PIXFMT_MASK		(0x04000000u)
#define	RSZ_HSTP_SHIFT			20
#define	RSZ_HRSZ_MASK			(0xfffffc00)
#define	RSZ_VRSZ_MASK			(0xfff003ff)
#define	RSZ_VRSZ_SHIFT			10
#define	RSZ_OUT_VSIZE_SHIFT		16
#define	SET_BIT_CBLIN			29
#define	SET_BIT_INPUTRAM		28
#define	INPUT_RAM			1
#define	SET_BIT_INPTYP			27
#define	SET_BIT_YCPOS			26
#define	RSZ_VSTPH_SHIFT			23
#define	RSZ_FILTER_COEFF_SHIFT		16
#define	RSZ_YENH_TYPE_SHIFT		16
#define	RSZ_YENH_GAIN_SHIFT		12
#define	RSZ_YENH_SLOP_SHIFT		8
#define	UP_RSZ_RATIO			64
#define	DOWN_RSZ_RATIO			512
#define	UP_RSZ_RATIO1			513
#define	DOWN_RSZ_RATIO1			1024
#define	SET_ENABLE_BIT			0
#define	RSZ_IN_SIZE_VERT_SHIFT		16
#define	MAX_HORZ_PIXEL_8BIT		31
#define	MAX_HORZ_PIXEL_16BIT		15
#define	BYTES_PER_PIXEL			2
#define NUM_PHASES			8
#define NUM_TAPS			4
#define NUM_D2PH			4	/* for downsampling
						   2+x ~ 4x, numberof phases */
#define	 NUM_D2TAPS			7	/* for downsampling
						   2+x ~ 4x,number of taps */
#define	NUM_COEFS			(NUM_PHASES * NUM_TAPS)
#define	ALIGN32				32
#define	ADDRESS_FOUND			1
#define	NEXT				1
#define	RESIZER_IOBASE_VADDR		IO_ADDRESS(0x01C70C00)
#define	MAX_COEF_COUNTER		16
#define	ZERO				0
#define	FIRSTENTRY			0
#define	SECONDENTRY			1
#define	EMPTY				0
#define	SUCESS				0
#endif

#define	RSZ_INTYPE_YCBCR422_16BIT	0
#define	RSZ_INTYPE_PLANAR_8BIT		1
#define	RSZ_PIX_FMT_PLANAR		2	/* 8-bit planar input */
#define	RSZ_PIX_FMT_UYVY		0	/* cb:y:cr:y */
#define	RSZ_PIX_FMT_YUYV		1	/* y:cb:y:cr */

#ifdef __KERNEL__
#define	isbusy()			((regr(PCR)	& 0x02)>>1)

enum config_done {
	STATE_CONFIGURED,	/* Resizer driver configured by application */
	STATE_NOT_CONFIGURED	/* Resizer driver not configured by
				   application */
};

#endif

/* Structure Definitions */
/* To allocate the memory */
struct rsz_reqbufs {
	int buf_type;		/* type of frame buffer */
	int size;		/* size of the frame bufferto be allocated */
	int count;		/* number of frame buffer to be allocated */
};

/* assed for quering the buffer to get physical address */
struct rsz_buffer {
	int index;		/* buffer index number, 0 -> N-1 */
	int buf_type;		/* buffer type, input or output */
	int offset;		/* physical address of the buffer,
				   used in the mmap() system call */
	int size;
};

/* used	to luma	enhancement options */
struct rsz_yenh {
	int type;		/* represents luma enable or disable */
	unsigned char gain;	/* represents gain */
	unsigned char slop;	/* represents slop */
	unsigned char core;	/* Represents core value */
};

/*
 * Conatins all	the parameters for resizing. This structure
 * is used to configure resiser parameters
 */
struct rsz_params {
	int in_hsize;		/* input frame horizontal size */
	int in_vsize;		/* input frame vertical size */
	int in_pitch;		/* offset between two rows of input frame */
	int inptyp;		/* for determining 16 bit or 8 bit data */
	int vert_starting_pixel;	/* for specifying vertical
					   starting pixel in input */
	int horz_starting_pixel;	/* for specyfing horizontal
					   starting pixel in input */
	int cbilin;		/* # defined, filter with luma or bi-linear
				   interpolation */
	int pix_fmt;		/* # defined, UYVY or YUYV */
	int out_hsize;		/* output frame horizontal size */
	int out_vsize;		/* output frame vertical size */
	int out_pitch;		/* offset between two rows of output frame */
	int hstph;		/* for specifying horizontal starting phase */
	int vstph;		/* for specifying vertical starting phase */
	short hfilt_coeffs[32];	/* horizontal filter coefficients */
	short vfilt_coeffs[32];	/* vertical filter coefficients */
	struct rsz_yenh yenh_params;
};

/* resize structure passed during the resize IOCTL */
struct rsz_resize {
	struct rsz_buffer in_buf;
	struct rsz_buffer out_buf;
};

/* Contains the status of hardware and channel */
struct rsz_status {
	int chan_busy;		/* 1: channel is busy, 0: channel is not busy */
	int hw_busy;		/* 1: hardware  is busy,
				   0: hardware is not busy */
	int src;		/* # defined, can be either
				   SD-RAM or CCDC/PREVIEWER */
};

/* structure to	set the priroity of the the channel */
struct rsz_priority {
	int priority;		/* 0=>5, with 5 the highest priority */
};

/* Passed by application for getting crop size */
struct rsz_cropsize {
	unsigned int hcrop;	/* number of pixels per line c
				   ropped in output image */
	unsigned int vcrop;	/* number of lines cropped
				   in output image */
};

#ifdef __KERNEL__

/*
 * Register mapped structure which contains the every register
 * information
 */
struct resizer_config {
	int rsz_pcr;		/*pcr register mapping variable */
	int rsz_in_start;	/* in_start register mapping variable */
	int rsz_in_size;	/* in_size register mapping variable */
	int rsz_out_size;	/* out_size register mapping variable */
	int rsz_cnt;		/* rsz_cnt register mapping     variable */
	int rsz_sdr_inadd;	/* sdr_inadd register mapping variable */
	int rsz_sdr_inoff;	/* sdr_inoff register mapping variable */
	int rsz_sdr_outadd;	/* sdr_outadd register mapping variable */
	int rsz_sdr_outoff;	/* sdr_outbuff register mapping  variable */
	int rsz_coeff_horz[16];	/* horizontal coefficients mapping array */
	int rsz_coeff_vert[16];	/* vertical  coefficients mapping  array */
	int rsz_yehn;		/* yehn(luma)register  mapping  variable */
};

/*
 * Channel specific structure contains information regarding
 * the every channel
 */
struct channel_config {
	struct resizer_config register_config;	/* instance of register set
						   mapping  structure */

	void *input_buffer[MAX_INPUT_BUFFERS];	/* for storing input buffers
						   pointers */

	void *output_buffer[MAX_OUTPUT_BUFFERS];	/* for storing output
							   buffers pointers */

	int in_bufsize, out_bufsize;	/* Contains input and output buffer
					   size */

	int status;		/* specifies whether the channel is busy
				   or not */

	int priority;		/* stores priority of the application */
#ifdef CONFIG_PREEMPT_RT
	struct completion channel_sem;
#else
	struct semaphore channel_sem;
#endif
	struct semaphore chanprotection_sem;
	enum config_done config_state;
};

/*
 * Global structure which contains information about number of chanels
 * and protection variables
 */
struct device_params {
	int module_usage_count;		/* For counting no of channels
					   created */
	struct completion sem_isr;	/* Semaphore for interrupt */
	struct semaphore array_sem;	/* Semaphore for array */
	struct semaphore device_mutex;	/* mutex protecting device_params */
	struct channel_config *channel_configuration[MAX_CHANNELS];
	int array_count;	/* for counting number of elements
				   in arrray */
};

extern struct device *rsz_device;

/* Functions Definition */

int malloc_buff(struct rsz_reqbufs *, struct channel_config *);
int get_buf_address(struct rsz_buffer *, struct channel_config *);
int rsz_start(struct rsz_resize *, struct channel_config *);
int add_to_array(struct channel_config *rsz_configuration_channel);
int delete_from_array(struct channel_config *rsz_configuration_channel);
int rsz_set_params(struct rsz_params *, struct channel_config *);
int rsz_get_params(struct rsz_params *, struct channel_config *);
int free_buff(struct channel_config *rsz_configuration_channel);
irqreturn_t rsz_isr(int, void *);
void rsz_calculate_crop(struct channel_config *rsz_conf_chan,
			struct rsz_cropsize *cropsize);
#endif	/* __KERNEL__ */

#endif	/* DAVINCI_RESIZER_H */
