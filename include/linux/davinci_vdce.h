/* *
 * Copyright (C) 2007 Texas Instruments	Inc
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under the	terms of the GNU General Public	License	as published by
 * the Free Software Foundation; either	version	2 of the License, or
 * (at your option any)	later version.
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,write to the	Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */
/* davinci_vdce.h	file */

#ifndef	DAVINCI_VDCE_H
#define	DAVINCI_VDCE_H
#ifdef __KERNEL__
/* include Linux files */
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#endif
//#include <mach/davinci_vdce_hw.h>
#define	  VDCE_IOC_BASE			   'G'
#define	  VDCE_IOC_MAXNR			    9
/* Ioctl options which are to be passed	while calling the ioctl	*/
#define	VDCE_SET_PARAMS		 _IOWR(VDCE_IOC_BASE, 1, vdce_params_t)
#define	VDCE_GET_PARAMS		 _IOR(VDCE_IOC_BASE, 2, vdce_params_t)
#define	VDCE_START		 _IOWR(VDCE_IOC_BASE, 3, vdce_address_start_t)
#define	VDCE_REQBUF		 _IOWR(VDCE_IOC_BASE, 4, vdce_reqbufs_t)
#define	VDCE_QUERYBUF		 _IOWR(VDCE_IOC_BASE, 5, vdce_buffer_t)
#define VDCE_GET_DEFAULT	 _IOWR(VDCE_IOC_BASE, 6, vdce_params_t)

#define	VDCE_MAX_PRIORITY		  5
#define	VDCE_MIN_PRIORITY		  0
#define	VDCE_DEFAULT_PRIORITY		  0
#define	MAX_BUFFERS			  8

#define MAX_BLEND_TABLE 		  (4)

#ifdef __KERNEL__
#define	MAX_RSZ_INTENSITY		  128
#define	MIN_RSZ_MAG_RATIO		  256
#define	MAX_RSZ_MAG_RATIO		  2048
#define	DRIVER_NAME			  "VDCE"
#define	MAX_CHANNELS			  16
#define	FIRSTENTRY			  0
#define	SECONDENTRY			  1
#define	EMPTY				  0
#define	FREE_BUFFER			  0
#define CHECK_MODE_RANGE(mode, limit)    ((mode < 0 || mode > limit)?1:0)
#define GET_DIVIDE_FACTOR(format)	 ((format == 0)?3:4)
#define GET_CHROMA_DIVIDE_FACTOR(format) ((format == 0)?4:2)
#define GET_VAL(reg, mask, shift)
#define RSZ_ENABLE_MASK			0x1
#define BLEND_ENABLE_MASK		0x2
#define RMAP_ENABLE_MASK		0x4
#define CCV_ENABLE_MASK			0x8
#endif

/* enum	for selected VDCE mode */
typedef enum vdce_mode_operation {
	VDCE_OPERATION_PRE_CODECMODE = 0,/* set pre codec mode */
	VDCE_OPERATION_POST_CODECMODE,	/* set post codec mode */
	VDCE_OPERATION_TRANS_CODECMODE,	/* set trans codec mode */
	VDCE_OPERATION_EDGE_PADDING,	/* enable edge padding */
	VDCE_OPERATION_RESIZING,	/* For doing chrominance conversion */
	VDCE_OPERATION_CHROMINANCE_CONVERSION,	/* For doing resizing */
	VDCE_OPERATION_RANGE_MAPPING,	/* For doing range mapping */
	VDCE_OPERATION_BLENDING		/* for doing blending */
} vdce_mode_operation_t;

/* enum	for enable disable status */
typedef enum vdce_status {
	VDCE_FEATURE_DISABLE = 0,	/* Suggest  disable status */
	VDCE_FEATURE_ENABLE		/* Suggest Enable status   */
} vdce_status_t;

/* enum	for image_type */
typedef enum vdce_image_type {
	VDCE_BUF_IN = 0,	/* Suggest  buffer is of output type */
	VDCE_BUF_OUT,		/* Suggest  buffer is of input type */
	VDCE_BUF_BMP		/* Suggest  buffer is of bitmap type */
} vdce_image_type_t;

/* enum	to enable disable luma/chroma processing */
typedef enum vdce_proc_control {
	VDCE_LUMA_ENABLE = 0,	/* suggest luma processing is enable  */
	VDCE_CHROMA_ENABLE,	/* suggest chroma processing is enable  */
	VDCE_LUMA_CHROMA_ENABLE	/* suggest both processing is enable  */
} vdce_proc_control_t;

/* enum	to enable disable field	status */
typedef enum vdce_field_status {
	VDCE_TOP_ENABLE = 0,/* suggest top field processing is enable  */
	VDCE_BOTTOM_ENABLE,/* suggest bottom field processing is enable */
	VDCE_TOP_BOT_ENABLE/* suggest both processing is enable  */
} vdce_field_status_t;

/* enum	for codec mode */
typedef enum vdce_codec_mode {
	VDCE_CODECMODE_MPEG2_MPEG4 = 0,	/* suggest mpeg2 is selected */
	VDCE_CODECMODE_MPEG1		/* suggest mpeg1 is selected */
} vdce_codec_mode_t;

/* ALF mode for	resizing */
typedef enum vdce_alfmode {
	VDCE_ALFMODE_AUTOMATIC = 0,	/* tap-4 */
	VDCE_ALFMODE_MANUAL		/* tap linear */
} vdce_alfmode_t;

/* type	of algo*/
typedef enum vdce_algo_type {
	VDCE_ALGO_TAP_4CUBIC_CONVOLUTION = 0,	/* cubic convolution */
	VDCE_ALGO_TAP_4LINEAR_INTERPOLATION	/* linear interpolation */
} vdce_algo_type_t;

/* for mode */
typedef enum vdce_io_mode {
	VDCE_FIELD_MODE = 0,	/* Indicate that field mode */
	VDCE_FRAME_MODE		/* Indicate that frame mode */
} vdce_io_mode_t;

/* hardware and	channel	status */
typedef enum vdce_processing_status {
	VDCE_CHAN_FREE = 0,		/* channel free */
	VDCE_CHAN_BUSY,			/* channel busy */
	VDCE_CHAN_PENDING,		/* Request pending */
	VDCE_CHAN_UNINITIALISED,	/* Parameters uninitialised */
	VDCE_CHAN_PARAMS_INITIALISED,	/* Parameters initialised */
} vdce_processing_status_t;

/* processing mode of image */
typedef enum vdce_processing_mode {
	VDCE_INTERLACED = 0,		/* interlaced mode */
	VDCE_PROGRESSIVE		/* progressive mode */
} vdce_processing_mode_t;

/* image format	*/
typedef enum vdce_image_fmt {
	VDCE_IMAGE_FMT_420 = 0,	    /* 4:2:0 image format */
	VDCE_IMAGE_FMT_422 = 1	    /* 4:2:2 image format */
} vdce_image_fmt_t;

/*CCV operation	mode */
typedef enum vdce_ccv_mode {
	VDCE_CCV_MODE_422_420 = 0,	/* 4:2:2 to 4:2:0 */
	VDCE_CCV_MODE_420_422,		/* 4:2:0 to 4:2:2 */
	VDCE_CCV_MODE_420_420		/* 4:2:0 to 4:2:0 */
} vdce_ccv_mode_t;

/*Resize operation mode	*/
typedef enum vdce_rsz_mode {
	VDCE_MODE_420 = 0,	/* 4:2:2  */
	VDCE_MODE_422,		/* 4:2:0 */
} vdce_rsz_blend_mode_t;

/* ---------------Interface Data structures-----------------------------------*/
/* Structure to	configure the resize parameters	*/
typedef struct vdce_rsz_params {
	vdce_rsz_blend_mode_t rsz_mode;	/* Value will not be used in native */
	/*Mode. It will be used for stanalone */
	/*resizing only */
	vdce_codec_mode_t rsz_op_mode;
	vdce_algo_type_t hrsz_mode;/* To suggest horizontal resizing mode */
					/*   eg convolution */
	vdce_algo_type_t vrsz_mode;	/* To suggest Vertical resizing mode */
					/* eg convolution */
	vdce_status_t hrsz_alf_enable;	/* Suggest horizontal anti alias */
					/*  filter is on */
	vdce_status_t vrsz_alf_enable;	/* Suggest vertical anti alias filter */
					/* is on */
	vdce_alfmode_t hrsz_alf_mode;	/* Suggest the anti alias filter mode */
	unsigned char hrsz_alf_intensity;/* Intensity for coefficients calc */
} vdce_rsz_params_t;

/* Structure to	configure the chrominance conversion parameters	*/
typedef struct vdce_ccv_params {
	vdce_ccv_mode_t conversion_type; /* Value will not be used in native */
					/*Mode. It will be used for stanalone */
					/*CCV only */
	/* Below both parameters will be used only in case for trans codec */
	vdce_codec_mode_t codec_mode_in;/* Type of input of codec conversion */
					/*eg mpeg1 or other */
	vdce_codec_mode_t codec_mode_out;/*Type of output of codec conversion */
					/*eg mpeg2 or other */
	vdce_algo_type_t hccv_type;	/* Chrominance conversion functional */
					/*type selection bit (for horz) */
	vdce_algo_type_t vccv_type;	/* Chrominance conversion functional */
					/* type selection bit(for vert) */
} vdce_ccv_params_t;

/* Structure to	configure the range maping parameters */
typedef struct vdce_rmap_params {

	unsigned char coeff_y;		/* coeffcients of range mapping */
					/*for Y component */
	vdce_status_t rmap_yenable;	/* Suggest that luminance range */
					/*mapping is enable */
	unsigned char coeff_c;		/* coeffcients of range mapping for */
					/* chroma component */
	vdce_status_t rmap_cenable;	/* Suggest that chrominace range */
					/* mapping is enable */
} vdce_rmap_params_t;

/* Structure to	configure the blending parameters */
typedef struct blend_table {

	unsigned char blend_cr;	/* blend value for cr component */
	unsigned char blend_cb;	/* blend value for cb component */
	unsigned char blend_y;	/* blend value for y component */
	unsigned char blend_value;	/* Blend factor */
} blend_table_t;

typedef struct vdce_blend_params {
	vdce_rsz_blend_mode_t blend_mode;
	blend_table_t bld_lut[MAX_BLEND_TABLE];	/* 4 bitmap defined for blend */
} vdce_blend_params_t;

/* Structure to	configure edge padding parameters */
typedef struct vdce_epad_params {

	unsigned char vext_chroma;	/* Vertical extension value for */
					/* chroma component */
	unsigned char hext_chroma;	/* horizontal extension value for */
					/*chroma component */
	unsigned char vext_luma;	/* Vertical extension value for */
					/* luma component */
	unsigned char hext_luma;	/* horizontal extension value for */
					/*chroma component */
} vdce_epad_params_t;

/* Structure to	configure input	output format and sizes	*/
typedef struct vdce_common_params {

	vdce_processing_mode_t src_processing_mode;/* source processing mode */
						/*interlaced or progressive */
	vdce_io_mode_t src_mode;	/* src mode field or frame  */
	vdce_io_mode_t res_mode;	/* res mode field or frame  */
	vdce_io_mode_t src_bmp_mode;	/* bmp mode field or frame */
	vdce_proc_control_t proc_control;	/* Luminace processing */
						/*enable or not */
	vdce_field_status_t field_status;	/* Top field enable or not */

	unsigned short src_hsz_luminance;/* input horizontal size for */
					/*luminance component */
	unsigned short src_vsz_luminance;/* input vertical size for */
					/*luminance component */
	unsigned short bmp_vsize;	/* input vertical size for */
					/*bitmap component */
	unsigned short bmp_hsize;	/* input horizontal size for */
					/* bitmap component */
	unsigned char src_hsp_luminance;/*horizontal starting position */
					/* for luminance */
	unsigned char src_vsp_luminance;/* vertical starting position */
					/* for luminance */
	unsigned char bmp_hsp_bitmap;	/* hrz starting position for */
					/* bitmap */
	unsigned char res_hsp_luminance;/* Hrz starting position for */
					/*luminance component for res */
	unsigned char res_vsp_luminance;/* Vertical starting position for */
					/*luminance component for res */
	unsigned short dst_hsz_luminance;/* Hrz starting size for */
					/*luminance component for res */
	unsigned short dst_vsz_luminance;/* Vertical size for */
					/*   luminance */
					/*component for result */
	unsigned short res_hsp_bitmap;	/* Horizontal starting pos */
					/*for bitmap component */
	unsigned short res_vsp_bitmap;	/* vertical starting position */
					/* for bitmap component */
	unsigned short prcs_unit_value;/* Hold the prcs value by app */
} vdce_common_params_t;

/* precodec parameters configuration structure */
typedef struct vdce_precodec_params {
	vdce_rsz_params_t rsz_params;
	vdce_ccv_params_t ccv_params;
} vdce_precodec_params_t;

/* postcodec parameters	configuration structure	*/
typedef struct vdce_postcodec_params {
	vdce_rsz_params_t rsz_params;
	vdce_rmap_params_t rmap_params;
	vdce_blend_params_t blend_params;
	vdce_ccv_params_t ccv_params;
} vdce_postcodec_params_t;

/* transcodec parameters configuration structure */
typedef struct vdce_transcodec_params {
	vdce_rsz_params_t rsz_params;
	vdce_rmap_params_t rmap_params;
	vdce_blend_params_t blend_params;
	vdce_epad_params_t epad_params;
	vdce_ccv_params_t ccv_params;
} vdce_transcodec_params_t;

/* Passed for setting VDCE parameters */
typedef struct vdce_params {
	vdce_mode_operation_t vdce_mode;
	/* Bits position for enabling the various sub-modes */
	/* 1 bit defienes resizing 2 blending 3 range mapping and 4 ccv */
	unsigned char modes_control;
	vdce_common_params_t common_params;
	union {
		vdce_precodec_params_t precodec_params;
		vdce_postcodec_params_t postcodec_params;
		vdce_transcodec_params_t transcodec_params;
		vdce_epad_params_t epad_params;
		vdce_rsz_params_t rsz_params;
		vdce_rmap_params_t rmap_params;
		vdce_blend_params_t blend_params;
		vdce_ccv_params_t ccv_params;
	} vdce_mode_params;
} vdce_params_t;

/* status parameters structure */
typedef struct vdce_hw_status {
	vdce_processing_status_t chan_status;	/* get channel status */
} vdce_hw_status_t;

/* to allocate the memory*/
typedef struct vdce_reqbufs {

	vdce_image_type_t buf_type;	/* typeof frame buffer */
	int count;		/* number of frame buffers to be allocated */
	int num_lines;		/* Number of lines i.e.vertical pitch for */
				/* luma componenet */
	int bytes_per_line;	/* Number of lines i.e.horizontal pitch for */
				/* luma componenet */
	vdce_image_fmt_t image_type;	/* Input image format */
} vdce_reqbufs_t;

/* assessed for	quering	the buffer to get physical address*/
typedef struct vdce_buffer {

	int index;		/* buffer index number, 0 -> N-1 */
	vdce_image_type_t buf_type;/* buffer type, input or output */
	union {
	unsigned int offset;	/* physical     address of the buffer, */
	unsigned int virt_ptr;
	};/* used in the mmap() system call */
	int size;		/* size */
} vdce_buffer_t;

/* structure to	be passed while	VDCE oparation */
typedef struct vdce_address_start {
	/* 0 is input buffer 1 is output and 2 in bitmap buffer */
	vdce_buffer_t buffers[3];
	/* Horizontal and vertical pitch */
	unsigned int src_horz_pitch;
	unsigned int res_horz_pitch;
	unsigned int bmp_pitch;
} vdce_address_start_t;

#ifdef __KERNEL__
/* ---------------Driver Structures-------------------------------------------*/
/* enum for suggesting num of passes required */
typedef enum vdce_num_pass {
	VDCE_SINGLE_PASS = 1,	/* VDCE driver will operate in single pass */
	VDCE_MULTIPASS	/* VDCE driver will operate in Multipass pass */
} vdce_num_pass_t;

/* enum for suggesting channel mode */
typedef enum vdce_mode {
	VDCE_MODE_BLOCKING = 0,	/* channel opeend in blocking mode */
	VDCE_MODE_NON_BLOCKING	/* channel opened in non-blocking */
} vdce_mode_t;

typedef enum vdce_start_status {
	VDCE_PASS1_STARTED = 0,
	VDCE_PASS2_STARTED,
	VDCE_COMPLETED
} vdce_start_status_t;

/* Global structure in driver */
typedef struct device_params {
	int module_usage_count;	/* Keeps track of number of open chan */
	struct completion sem_isr;	/* sem to indicate the completion */
					/*in case for blocking call */
	/*struct semaphore array_sem;*/	/* sem to protect array entry */
	spinlock_t irqlock;
	void *inter_buffer;	/* Address for inter buffer */
	unsigned int inter_size; /* Size of intermediate buffer */
	struct completion edma_sem;
	struct completion device_access;
} device_params_t;
typedef struct vdce_buffer_info {
	void *buffers[MAX_BUFFERS];
	int size;
	int num_allocated;
} vdce_buffer_info_t;
/* Channel specific device structure */
typedef struct channel_config {
	struct vdce_hw_config register_config[2];/* Instance of register */
						/* mapping structure */
	vdce_num_pass_t num_pass;		/* Single pass or multipass */
	vdce_buffer_info_t vdce_buffer[3];
	vdce_params_t get_params;	/*Instance for parameters send */
						/* by application */
	vdce_params_t get_params1;

	vdce_mode_operation_t mode_state;	/* VDCE channel mode */
	vdce_mode_t channel_mode;		/* Indiacte the blocking or */
						/* non blocking */
	int luma_chroma_phased;		/* if 1 indicates luma is phased */
	vdce_processing_status_t status;	/* Indicates channel free/used*/
	vdce_start_status_t vdce_complete;	/* To keep track of isr */
	vdce_image_fmt_t image_type_out;	/* Input image type */
	vdce_image_fmt_t image_type_in;		/* Output image type */
	unsigned int edma_operation;		/* Keeps track whether edma
						   operation is required for
						   this channel. */

} channel_config_t;

typedef struct vdce_debug_info {
	/* keeps track of number of interrupts */
	int vdce_interrupt_count;
	/* keeps track of number of non-block calls */
	int vdce_nonblock_count;
	/* keeps track of number of blocking calls */
	int vdce_block_count;
} vdce_debug_info_t;

/* ---------------Function definition-----------------------------------------*/
int malloc_buff(vdce_reqbufs_t *, channel_config_t *);
int get_buf_address(vdce_buffer_t *, channel_config_t *);
int free_buff(channel_config_t *);
int vdce_set_multipass_address(vdce_address_start_t *, channel_config_t *);
int vdce_set_address(vdce_address_start_t *, channel_config_t *,
						 int, u32, u32, u32);
int vdce_start(vdce_address_start_t *, channel_config_t *);
int add_to_array(channel_config_t *);
int delete_from_array(void);
void process_bottomhalf(unsigned long);
int vdce_get_default(vdce_params_t *);
int vdce_check_common_params(vdce_params_t *);
int vdce_set_size_fmt(vdce_common_params_t *, channel_config_t *, int);
int vdce_set_rszparams(vdce_params_t *, channel_config_t *,
		     vdce_mode_operation_t, int);
int vdce_set_epadparams(vdce_epad_params_t *, channel_config_t *, int);
int vdce_set_ccvparams(vdce_ccv_params_t *, channel_config_t *, int, int);
int vdce_set_blendparams(vdce_blend_params_t *, channel_config_t *, int, int);
int vdce_set_rmapparams(vdce_rmap_params_t *, channel_config_t *, int);
int vdce_set_params(vdce_params_t *, channel_config_t *);
int vdce_get_params(vdce_params_t *, channel_config_t *);
int vdce_set_priority(int, channel_config_t *);
int vdce_get_priority(int *, channel_config_t *);
int vdce_get_status(vdce_hw_status_t *, channel_config_t *);
int free_num_buffers(int *, unsigned long, unsigned long);
irqreturn_t vdce_isr(int, void *);
#endif
#endif
