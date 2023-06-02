/* *
 * Copyright (C) 2007 Texas Instruments	Inc
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under the	terms of the GNU General Public	License	as published by
 * the Free Software Foundation; either	version	2 of the License, or
 * (at your option)any	later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,write to the	Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston, MA  02111-1307	USA
 */
/* davinci_vdce.c	file */

/*Header files*/
#include <linux/kernel.h>
#include <linux/fs.h>		/*     everything... */
#include <linux/errno.h>	/*     error codes     */
#include <linux/types.h>	/*     size_t */
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include "davinci_vdce_hw.h"
#include <linux/davinci_vdce.h>
#include <linux/init.h>
#include <asm/cacheflush.h>
#include <mach/edma.h>
#define TCINTEN_SHIFT               20
#define ITCINTEN_SHIFT              21

/* edma isr status tracking variable */
volatile static int irqraised1 = 1;

unsigned int vdce_counter = 0;
unsigned int edma_counter = 0;
#define	DRIVERNAME	"DavinciHD_vdce"
#define VDCE_INTERRUPT	12

MODULE_LICENSE("GPL");
DECLARE_TASKLET(short_tasklet, process_bottomhalf, 0);

/*Global structute shared between all applications
  struct device_params device_config;*/
device_params_t device_config;
/* For registeration of	character device*/
static struct cdev c_dev;
/* device structure to make entry in device*/
static dev_t dev;
/* for holding device entry*/
struct device *vdce_device = NULL;

static u32 inter_bufsize = 1920 * 1080;
module_param(inter_bufsize, uint, S_IRUGO);

channel_config_t *vdce_current_chan = NULL;

static int dma_ch;

/* default values for various modes */
#define COMMON_DEFAULT_PARAMS {VDCE_PROGRESSIVE, VDCE_FRAME_MODE, \
VDCE_FRAME_MODE, VDCE_FRAME_MODE, VDCE_LUMA_CHROMA_ENABLE, \
VDCE_TOP_BOT_ENABLE, 720, 480, 120, 60, 0, 0, 0, 0, 0, 720, 480, 0, 0, 0}

#define RSZ_DEFAULT_PARAMS {VDCE_MODE_422, VDCE_CODECMODE_MPEG2_MPEG4, \
VDCE_ALGO_TAP_4LINEAR_INTERPOLATION, VDCE_ALGO_TAP_4LINEAR_INTERPOLATION, \
VDCE_FEATURE_DISABLE, VDCE_FEATURE_DISABLE, VDCE_ALFMODE_AUTOMATIC, 0}

#define CCV_DEFAULT_PARAMS {VDCE_CCV_MODE_422_420, VDCE_CODECMODE_MPEG2_MPEG4, \
VDCE_CODECMODE_MPEG2_MPEG4, VDCE_ALGO_TAP_4LINEAR_INTERPOLATION, \
VDCE_ALGO_TAP_4LINEAR_INTERPOLATION }

#define RMAP_DEFAULT_PARAMS {10, VDCE_FEATURE_ENABLE, 10, VDCE_FEATURE_ENABLE}

#define BLEND_TABLE {0x36, 0x22, 0x91, 0xff}

#define BLEND_DEFAULT_PARAMS {VDCE_MODE_422, BLEND_TABLE, BLEND_TABLE, \
BLEND_TABLE, BLEND_TABLE}

#define EPAD_DEFAULT_PARAMS {16, 8, 16, 8}

#define PRECODEC_PARAMS	{RSZ_DEFAULT_PARAMS, CCV_DEFAULT_PARAMS}

#define POSTCODEC_PARAMS {RSZ_DEFAULT_PARAMS, RMAP_DEFAULT_PARAMS, \
BLEND_DEFAULT_PARAMS, CCV_DEFAULT_PARAMS}

#define TRANSCODEC_PARAMS {RSZ_DEFAULT_PARAMS, RMAP_DEFAULT_PARAMS, \
BLEND_DEFAULT_PARAMS, EPAD_DEFAULT_PARAMS, CCV_DEFAULT_PARAMS}

/* Default pre-codec params */
static vdce_params_t precodec_default_params = {
	VDCE_OPERATION_PRE_CODECMODE,
	0x5,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.precodec_params = PRECODEC_PARAMS
};

/* Default post-codec params */
static vdce_params_t postcodec_default_params = {
	VDCE_OPERATION_POST_CODECMODE,
	0x1,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.postcodec_params.rsz_params = RSZ_DEFAULT_PARAMS,
	.vdce_mode_params.postcodec_params.rmap_params = RMAP_DEFAULT_PARAMS,
	.vdce_mode_params.postcodec_params.blend_params.blend_mode =
	    VDCE_MODE_422,
	.vdce_mode_params.postcodec_params.blend_params.bld_lut[0] =
	    BLEND_TABLE,
	.vdce_mode_params.postcodec_params.blend_params.bld_lut[1] =
	    BLEND_TABLE,
	.vdce_mode_params.postcodec_params.blend_params.bld_lut[2] =
	    BLEND_TABLE,
	.vdce_mode_params.postcodec_params.blend_params.bld_lut[3] =
	    BLEND_TABLE,
	.vdce_mode_params.postcodec_params.ccv_params = CCV_DEFAULT_PARAMS
};

/* Default trans-codec params */
static vdce_params_t transcodec_default_params = {
	VDCE_OPERATION_TRANS_CODECMODE,
	0x1,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.transcodec_params.rsz_params = RSZ_DEFAULT_PARAMS,
	.vdce_mode_params.transcodec_params.rmap_params = RMAP_DEFAULT_PARAMS,
	.vdce_mode_params.transcodec_params.blend_params.blend_mode =
	    VDCE_MODE_422,
	.vdce_mode_params.transcodec_params.blend_params.bld_lut[0] =
	    BLEND_TABLE,
	.vdce_mode_params.transcodec_params.blend_params.bld_lut[1] =
	    BLEND_TABLE,
	.vdce_mode_params.transcodec_params.blend_params.bld_lut[2] =
	    BLEND_TABLE,
	.vdce_mode_params.transcodec_params.blend_params.bld_lut[3] =
	    BLEND_TABLE,
	.vdce_mode_params.transcodec_params.epad_params = EPAD_DEFAULT_PARAMS,
	.vdce_mode_params.transcodec_params.ccv_params = CCV_DEFAULT_PARAMS
};

/* Default edgepadding params */
static vdce_params_t epad_default_params = {
	VDCE_OPERATION_EDGE_PADDING,
	0x0,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.epad_params = EPAD_DEFAULT_PARAMS
};

/* Default resising params */
static vdce_params_t rsz_default_params = {
	VDCE_OPERATION_RESIZING,
	0x0,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.rsz_params = RSZ_DEFAULT_PARAMS
};

/* Default CCV params */
static vdce_params_t ccv_default_params = {
	VDCE_OPERATION_CHROMINANCE_CONVERSION,
	0x0,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.ccv_params = CCV_DEFAULT_PARAMS
};

/* Default blending params */
static vdce_params_t blend_default_params = {
	VDCE_OPERATION_BLENDING,
	0x0,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.blend_params.blend_mode = VDCE_MODE_422,
	.vdce_mode_params.blend_params.bld_lut[0] = BLEND_TABLE,
	.vdce_mode_params.blend_params.bld_lut[1] = BLEND_TABLE,
	.vdce_mode_params.blend_params.bld_lut[2] = BLEND_TABLE,
	.vdce_mode_params.blend_params.bld_lut[3] = BLEND_TABLE
};

/* Default rangemapping params */
static vdce_params_t rmap_default_params = {
	VDCE_OPERATION_RANGE_MAPPING,
	0x0,
	COMMON_DEFAULT_PARAMS,
	.vdce_mode_params.rmap_params = RMAP_DEFAULT_PARAMS
};
static int prcs_array_value[] = { 16, 32, 64, 128, 256 };

/*
 * vdce_free_pages : Function to free memory of buffers
 */
inline void vdce_free_pages(unsigned long addr, unsigned long bufsize)
{
	unsigned long size;
	unsigned long tempaddr = addr;
	if (!addr)
		return;
	size = PAGE_SIZE << (get_order(bufsize));
	while (size > 0) {
		ClearPageReserved(virt_to_page(addr));
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	free_pages(tempaddr, get_order(bufsize));
}

/*
 * vdce_uservirt_to_phys : This inline function is used to
 * convert user space virtual address to physical address.
 */
static inline unsigned long vdce_uservirt_to_phys(unsigned long virtp)
{
	unsigned long physp = 0;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	/* For kernel direct-mapped memory, take the easy way */
	if (virtp >= PAGE_OFFSET)
		physp = virt_to_phys((void *)virtp);

	/* this will catch, kernel-allocated, mmaped-to-usermode addresses */
	else if ((vma = find_vma(mm, virtp)) &&
		 (vma->vm_flags & VM_IO) && (vma->vm_pgoff)) {
		physp = (vma->vm_pgoff << PAGE_SHIFT) + (virtp - vma->vm_start);
	}
	/* otherwise, use get_user_pages() for general userland pages */
	else {
		int res, nr_pages = 1;
		struct page *pages;
		down_read(&current->mm->mmap_sem);

		res = get_user_pages(current, current->mm,
				     virtp, nr_pages, 1, 0, &pages, NULL);
		up_read(&current->mm->mmap_sem);

		if (res == nr_pages) {
			physp =
			    __pa(page_address(&pages[0]) +
				 (virtp & ~PAGE_MASK));
		} else {
			dev_err(vdce_device,
				" Unable to find phys addr for 0x%08lx\n",
				virtp);
			dev_err(vdce_device,
				"get_user_pages() failed: %d\n", res);
		}
	}
	return physp;
}

/*
 * malloc_buff : Function to allocate memory to input
 * and output buffers
 */
int malloc_buff(vdce_reqbufs_t * reqbuff, channel_config_t * vdce_conf_chan)
{
	/* for looping purpose */
	int i = 0;
	/* for pointing to input output buffer or bitmap pointer */
	int *buf_ptr;
	/* to calculate no of max buffers; */
	int maxbuffers;
	/* to calculate number of buffers allocated */
	int numbuffers = 0;
	/* for storing buffer size */
	int *buf_size;
	/* Stores requested buffer size */
	unsigned int req_buffersize = 0;
	/* to make sure buffer pointer never swapped */
	unsigned long adr;
	unsigned long size;
	int multiplier;

	dev_dbg(vdce_device, " <fn> malloc_buff Entering E </fn>\n");

	dev_dbg(vdce_device, "Input buffer requested \n");
	buf_ptr =
	    (unsigned int *)vdce_conf_chan->vdce_buffer[reqbuff->
							buf_type].buffers;
	buf_size = &vdce_conf_chan->vdce_buffer[reqbuff->buf_type].size;
	maxbuffers = MAX_BUFFERS;
	numbuffers =
	    vdce_conf_chan->vdce_buffer[reqbuff->buf_type].num_allocated;
	if (reqbuff->buf_type > VDCE_BUF_BMP) {
		dev_dbg(vdce_device, "Invalid type \n");
		return -EINVAL;
	}
	multiplier = GET_DIVIDE_FACTOR(reqbuff->image_type);
	req_buffersize =
	    ((reqbuff->num_lines * reqbuff->bytes_per_line * multiplier) / 2);
	/* check the request for number of buffers */
	if (reqbuff->count > maxbuffers)
		return -EINVAL;

	/* free all the buffers if the count is zero */
	if ((reqbuff->count == FREE_BUFFER) ||
	    ((numbuffers != 0) && (req_buffersize != *buf_size))) {
		/* free all the buffers */
		free_num_buffers(buf_ptr, *buf_size, numbuffers);
		return 0;
	}
	dev_dbg(vdce_device,
		"The	no of requested	buffers	are %d \n ", reqbuff->count);
	/* free the remainning buffers . ie total allocated is 7 requested is 5
	   than free 2 buffers */
	if ((numbuffers - reqbuff->count) > 0) {
		buf_ptr = buf_ptr + reqbuff->count;
		/* free the remainning buffers */
		free_num_buffers(buf_ptr, *buf_size,
				 (numbuffers - reqbuff->count));
	} else {
		buf_ptr = buf_ptr + numbuffers;
		for (i = numbuffers; i < reqbuff->count; i++) {
			/* assign memory to buffer */
			*buf_ptr =
			    (int)(__get_free_pages
				  (GFP_KERNEL | GFP_DMA,
				   get_order(req_buffersize)));
			if (!(*buf_ptr)) {
				reqbuff->count = numbuffers + i;
				*buf_size = req_buffersize;
				dev_dbg(vdce_device,
					"requestbuffer:not enough memory");
				return -ENOMEM;
			}
			adr = *buf_ptr;
			size = PAGE_SIZE << (get_order(req_buffersize));
			while (size > 0) {
				/* make  sure the frame buffers
				   are never swapped out of memory */
				SetPageReserved(virt_to_page(adr));
				adr += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			buf_ptr++;
		}
	}
	reqbuff->count = numbuffers + i;

	vdce_conf_chan->vdce_buffer[reqbuff->buf_type].num_allocated =
	    numbuffers + i;
	/* set the buffer size to requested size */
	/* this will be useful only when numbuffers = 0 */
	*buf_size = req_buffersize;
	dev_dbg(vdce_device, "<fn> malloc_buff Leaving </fn>\n");
	return 0;
}

/*
 * free_num_buffers :Function to free the number of
 *  buffers specified by count
 */
int free_num_buffers(int *addr, unsigned long bufsize, unsigned long count)
{
	int i;
	for (i = 0; i < count; i++) {
		/* free memory allocate for the image */
		dev_dbg(vdce_device, "Free all the allocated	buffers	\n");
		/* free buffers using free_pages */
		vdce_free_pages((int)*addr, bufsize);
		/* assign buffer zero to indicate its free */
		*addr = (int)NULL;
		addr++;
	}
	return 0;
}

/*
 * get_buf_address : Function to query the  physical address
 * of the buffer  requested by index
 */
int get_buf_address(vdce_buffer_t * buffer, channel_config_t * vdce_conf_chan)
{
	dev_dbg(vdce_device, "<fn>get_buf_address Entering </fn>\n");
	if (!buffer || !vdce_conf_chan) {
		dev_err(vdce_device, "query_buffer: error in argument\n");
		return -EINVAL;
	}
	if (buffer->buf_type > VDCE_BUF_BMP) {
		dev_dbg(vdce_device, "Invalid type \n");
		return -EINVAL;
	}
	/*checking the index requested */
	if (buffer->index >=
	    (vdce_conf_chan->vdce_buffer[buffer->buf_type].num_allocated)) {
		dev_dbg(vdce_device, "Requested buffer not allocated	\n");
		return -EINVAL;
	}
	/* assignning the  input address to offset which will be
	   used in mmap */
	buffer->offset =
	    (unsigned int)vdce_conf_chan->vdce_buffer[buffer->buf_type].
	    buffers[buffer->index];
	buffer->size = vdce_conf_chan->vdce_buffer[buffer->buf_type].size;
	buffer->offset = virt_to_phys((void *)buffer->offset);

	dev_dbg(vdce_device, "the physical offset returned after query \
						is %x", buffer->offset);
	dev_dbg(vdce_device, "<fn>get_buf_address Leaving </fn>\n");
	return 0;
}

/*
 * free_buff : This function free the input and output buffers alloated
 */
int free_buff(channel_config_t * vdce_conf_chan)
{
	int buffercounter = 0;
	int i;
	dev_dbg(vdce_device, " <fn>free_buff E</fn>\n");
	/* free all the  buffers */
	for (i = 0; i < 3; i++) {
		while ((vdce_conf_chan->vdce_buffer[i].
			buffers[buffercounter] != NULL)
		       && buffercounter < MAX_BUFFERS) {
			/* free the memory */
			vdce_free_pages((unsigned long)vdce_conf_chan->
					vdce_buffer[i].buffers[buffercounter]
					, vdce_conf_chan->vdce_buffer[i].size);
			/* assign buffer zero to indicate its free */
			vdce_conf_chan->vdce_buffer[i].
			    buffers[buffercounter] = (unsigned long)NULL;
			buffercounter++;
		}
		buffercounter = 0;
	}
	dev_dbg(vdce_device, "<fn> free_buff L</fn>\n");
	return 0;
}

/*
edma call back function.
*/
static void callback1(unsigned lch, u16 ch_status, void *data)
{
	switch(ch_status) {
		case DMA_COMPLETE:
			irqraised1 = 1;
			break;
		case DMA_CC_ERROR:
			printk("Cannot Complete\n");
			break;
		default:
			break;
	}
	if (unlikely(ch_status != DMA_COMPLETE)) {
		printk("Cannot Complete\n");
	}
	irqraised1 = 1;
	complete(&(device_config.edma_sem));
}

/* edma3 memcpy functiom which copies the luma data
*/
static int edma3_memcpy(int acnt, int bcnt, int ccnt,
			vdce_address_start_t * vdce_start)
{
	int result = 0;
	int i, p = 0;
	unsigned int Istestpassed = 0u;
	unsigned int numenabled = 0;
	unsigned int BRCnt = 0;
	int srcbidx = 0;
	int desbidx = 0;
	int srccidx = 0;
	int descidx = 0;
	unsigned int numtimes = 0;
	unsigned int src_inc = 0, dst_inc = 0;
	struct edmacc_param param_set;

	/* Setting up the SRC/DES Index */
	srcbidx = vdce_start->src_horz_pitch;
	desbidx = vdce_start->res_horz_pitch;

	if (ccnt == 2 && (((bcnt != vdce_start->buffers[1].
			    size / (vdce_start->res_horz_pitch * 4))) ||
			  ((bcnt != vdce_start->buffers[0].
			    size / (vdce_start->src_horz_pitch * 4))))) {

		numtimes = 1;
		srccidx = acnt;
		descidx = acnt;
		BRCnt = bcnt;
		src_inc = 0;
		dst_inc = 0;
		ccnt = 1;
	} else {
		/* A Sync Transfer Mode */
		srccidx = acnt;
		descidx = acnt;
		BRCnt = bcnt;
		src_inc = 0;
		dst_inc = 0;
		numtimes = 0;
	}
	if (ccnt == 2 && ((bcnt == vdce_start->buffers[1].
			   size / (vdce_start->res_horz_pitch * 4)))) {
		bcnt = bcnt * 2;
	}
	src_inc = 0;
	dst_inc = 0;

	for (p = 0; p <= numtimes; p++) {
		/* Set the Source EDMA Params */
		edma_set_src(dma_ch, (unsigned long)(vdce_start->
				buffers[0].offset + src_inc), INCR, W8BIT);
		/* Set the Destination EDMA Params */
		edma_set_dest(dma_ch, (unsigned long) (vdce_start->
				buffers[1].offset + dst_inc), INCR, W8BIT);
		/* Set the Source Index */
		edma_set_src_index(dma_ch, srcbidx, srccidx);
		/* Set the Destination Index */
		edma_set_dest_index(dma_ch, desbidx, descidx);
		/* Set the Transfer Params */
		edma_set_transfer_params(dma_ch, acnt, bcnt, ccnt, BRCnt,
								ABSYNC);
		edma_read_slot(dma_ch, &param_set);
		param_set.opt |= (1 << ITCINTEN_SHIFT);
		param_set.opt |= (1 << TCINTEN_SHIFT);
		param_set.opt |= EDMA_TCC(EDMA_CHAN_SLOT(dma_ch));
		edma_write_slot(dma_ch, &param_set);

		numenabled = 1;

		for (i = 0; i < numenabled; i++) {
			irqraised1 = 0;

			/*
			 * Now enable the transfer as calculated above.
			 */
			device_config.edma_sem.done = 0;
			result = edma_start(dma_ch);
			if (result != 0) {
				dev_err(vdce_device, "dma start failed \n");
				break;
			}
			result = wait_for_completion_interruptible(&(device_config.edma_sem));
			if(result) {
				wait_for_completion(&(device_config.edma_sem));
			}

			/* Check the status of the completed transfer */
			if (irqraised1 < 0) {
				/* Some error occured, break from the FOR loop. */
				edma_stop(dma_ch);
				result = -EAGAIN;
				break;
			}
		}
		Istestpassed = 1;

		src_inc = vdce_start->buffers[0].size / 4;
		dst_inc = vdce_start->buffers[1].size / 4;
	}
	return result;
}

/*
 * vdce_set_address : This function is used to set the addres register
 */
int vdce_set_address(vdce_address_start_t * vdce_start,
		     channel_config_t * vdce_conf_chan, int num_pass,
		     unsigned int res_size, unsigned int address,
		     unsigned int pitch)
{

	int no_of_lines = 0, no_of_lines_output = 0;
	int divider;
	unsigned int multiplier = 1;
	unsigned int temp_lines = 1;
	unsigned int src_vsp = 0;
	unsigned int res_vsp = 0;
	int res_mode, src_mode;
	unsigned int src_pitch, res_pitch, src_address;
	unsigned int flag = 0, buffer_index, res_address;

	dev_dbg(vdce_device, "<fn> vdce_set_address	E </fn>\n");
	/* Top field luma address is the src starting address */
	src_address = vdce_start->buffers[0].offset;
	src_pitch = vdce_start->src_horz_pitch;
	/* For resizing set source mode for second pass equals result mode */
	src_mode = vdce_conf_chan->get_params.common_params.src_mode;

	if (vdce_conf_chan->num_pass == VDCE_MULTIPASS) {
		if (num_pass == 1 &&
		    (vdce_conf_chan->luma_chroma_phased == 0) &&
		    vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING) {
			src_mode =
			    vdce_conf_chan->get_params.common_params.res_mode;
		}
	}
	if (num_pass == 1) {
		if (vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING) {
			src_address = address;
			src_pitch = vdce_start->res_horz_pitch;
		} else {
			src_pitch =
			    vdce_conf_chan->register_config[0].
			    vdce_res_strt_off_ytop;

			src_address =
			    vdce_conf_chan->
			    register_config[0].vdce_res_strt_add_ytop;
		}
	}
	vdce_conf_chan->register_config[num_pass].vdce_src_strt_add_ytop =
	    src_address +
	    (vdce_conf_chan->get_params.common_params.src_vsp_luminance *
	     vdce_start->src_horz_pitch);
	if (vdce_conf_chan->get_params.common_params.src_mode ==
	    VDCE_FIELD_MODE) {
		src_vsp =
		    vdce_conf_chan->get_params.common_params.
		    src_vsp_luminance * vdce_start->src_horz_pitch;
	}
	/* no of lines in horizonatl direction for top/bottom field data */
	/* ie if v pitch is 48num_pass  than ytop will be of 240 lines */
	if (vdce_conf_chan->image_type_in == VDCE_IMAGE_FMT_420) {
		vdce_start->buffers[0].size =
		    (vdce_start->buffers[0].size * 4) / 3;
	}
	no_of_lines = (vdce_start->buffers[0].size / (4));

	if ((vdce_conf_chan->get_params.common_params.
	     src_processing_mode == VDCE_INTERLACED)
	    && (src_mode == VDCE_FRAME_MODE)) {
		multiplier = 1;
	}
	/* offset would be horizontal luma size */
	vdce_conf_chan->register_config[num_pass].vdce_src_add_ofst_ytop =
	    src_pitch * multiplier;
	/* offset configuration */
	vdce_conf_chan->register_config[num_pass].vdce_src_add_ofst_ybot =
	    src_pitch * multiplier;
	vdce_conf_chan->register_config[num_pass].vdce_src_add_ofst_ctop =
	    vdce_start->src_horz_pitch * multiplier;
	vdce_conf_chan->register_config[num_pass].vdce_src_add_ofst_cbot =
	    vdce_start->src_horz_pitch * multiplier;

	/* configure bottom field luma addreess is ytop + (no_of_lines*hpitch) */
	multiplier = src_pitch;
	if (src_mode == VDCE_FIELD_MODE) {
		temp_lines = no_of_lines;
		multiplier = 1;
	}
	/* offset would be horizontal luma size */
	vdce_conf_chan->register_config[num_pass].vdce_src_strt_add_ybot =
	    (vdce_conf_chan->register_config[num_pass].
	     vdce_src_strt_add_ytop + (temp_lines * multiplier));

	if (num_pass == 1 && (src_mode == VDCE_FIELD_MODE)) {
		vdce_conf_chan->register_config[num_pass].
		    vdce_src_strt_add_ybot = vdce_conf_chan->
		    register_config[0].vdce_res_strt_add_ybot;
	}
	if (vdce_conf_chan->num_pass == VDCE_MULTIPASS) {
		if (num_pass == 0) {
			src_address = vdce_start->buffers[0].offset +
				(vdce_conf_chan->get_params.common_params.src_vsp_luminance *
				 src_pitch);
		} else {
			src_address = vdce_start->buffers[0].offset;
		}
	} else {
		src_address = vdce_start->buffers[0].offset +
			(vdce_conf_chan->get_params.common_params.src_vsp_luminance *
			 src_pitch);
	}
	if (num_pass != 1) {
		if (vdce_conf_chan->image_type_in == VDCE_IMAGE_FMT_420) {
			src_address = vdce_start->buffers[0].offset +
				(no_of_lines * 2) + ((vdce_conf_chan->get_params.common_params.src_vsp_luminance/2) * src_pitch);
		}
		else {
			src_address = (src_address + ((no_of_lines * 2)));
		}
	}
	vdce_conf_chan->register_config[num_pass].vdce_src_strt_add_ctop =
	    (src_address);

	divider = GET_CHROMA_DIVIDE_FACTOR(vdce_conf_chan->image_type_in);
	if (src_mode == VDCE_FRAME_MODE) {
		vdce_conf_chan->register_config[num_pass].
		    vdce_src_strt_add_cbot =
		    (vdce_conf_chan->register_config[num_pass].
		     vdce_src_strt_add_ctop + vdce_start->src_horz_pitch);
	} else {
		vdce_conf_chan->register_config[num_pass].
		    vdce_src_strt_add_cbot =
		    (vdce_conf_chan->register_config[num_pass].
		     vdce_src_strt_add_ctop + ((temp_lines * 2 / divider)));
	}
	/* bitmap offset configuration is bitmap hsize */
	vdce_conf_chan->register_config[num_pass].
	    vdce_src_strt_off_bmp_top = vdce_start->bmp_pitch;

	vdce_conf_chan->register_config[num_pass].
	    vdce_src_strt_off_bmp_bot = vdce_start->bmp_pitch;
	/* bitmap starting address is starting address for bitmap offset */
	vdce_conf_chan->register_config[num_pass].
	    vdce_src_strt_add_bmp_top = vdce_start->buffers[2].offset;

	/* bitmap bottom field starting address */
	vdce_conf_chan->register_config[num_pass].
	    vdce_src_strt_add_bmp_bot =
	    vdce_conf_chan->register_config[num_pass].
	    vdce_src_strt_add_bmp_top +
	    (vdce_conf_chan->get_params.common_params.bmp_vsize / 2);

	/* result address for top field */
	if (vdce_conf_chan->image_type_out == VDCE_IMAGE_FMT_420) {
		vdce_start->buffers[1].size =
		    (vdce_start->buffers[1].size * 4) / 3;
	}
	no_of_lines_output = (vdce_start->buffers[1].size / (2));

	res_mode = vdce_conf_chan->get_params.common_params.res_mode;
	if (vdce_conf_chan->num_pass == VDCE_MULTIPASS) {
		if (num_pass == 0 && (vdce_conf_chan->luma_chroma_phased == 0)) {
			if (vdce_conf_chan->mode_state ==
			    VDCE_OPERATION_BLENDING) {
				res_mode = vdce_conf_chan->
				    get_params.common_params.src_mode;
				address = vdce_start->buffers[0].offset;
			}
			flag = 1;
		}
	}
	buffer_index = (flag == 1) ? 0 : 1;
	res_pitch = vdce_start->res_horz_pitch;
	if (buffer_index == 0) {
		res_pitch = pitch;

	}
	if (vdce_conf_chan->num_pass != VDCE_MULTIPASS) {
		address = vdce_start->buffers[buffer_index].offset;
	}

	/* top field luma address is the src starting address */
	vdce_conf_chan->register_config[num_pass].vdce_res_strt_add_ytop =
	    address +
	    (vdce_conf_chan->get_params.common_params.res_vsp_luminance *
	     vdce_start->res_horz_pitch);
	if (vdce_conf_chan->get_params.common_params.res_mode ==
	    VDCE_FIELD_MODE) {
		res_vsp =
		    vdce_conf_chan->get_params.common_params.
		    res_vsp_luminance * vdce_start->res_horz_pitch;
	}
	multiplier = 1;
	/* offset would be horizontal luma size */
	if ((vdce_conf_chan->get_params.common_params.
	     src_processing_mode == VDCE_INTERLACED)
	    && (vdce_conf_chan->get_params.common_params.res_mode ==
		VDCE_FRAME_MODE)) {
		multiplier = 1;
	}
	vdce_conf_chan->register_config[num_pass].vdce_res_strt_off_ytop =
	    res_pitch * multiplier;
	/* offset configuration */
	vdce_conf_chan->register_config[num_pass].vdce_res_strt_off_ybot =
	    res_pitch * multiplier;
	vdce_conf_chan->register_config[num_pass].vdce_res_strt_off_ctop =
	    vdce_start->res_horz_pitch * multiplier;
	vdce_conf_chan->register_config[num_pass].vdce_res_strt_off_cbot =
	    vdce_start->res_horz_pitch * multiplier;

	/* no of lines in horizonatl direction for top/bottom field data */
	/* ie if v pitch is 48num_pass  than ytop will be of 24num_pass lines */
	dev_dbg(vdce_device, "The number of lines are %d\n\n",
		no_of_lines_output);

	/* configure bottom field luma addreess */
	if (res_mode == VDCE_FRAME_MODE) {
		vdce_conf_chan->
		    register_config[num_pass].vdce_res_strt_add_ybot =
		    (vdce_conf_chan->register_config[num_pass].
		     vdce_res_strt_add_ytop + res_pitch);
	} else {

		if ((vdce_conf_chan->num_pass == VDCE_MULTIPASS) &&
		    (num_pass == 0)
		    && (vdce_conf_chan->luma_chroma_phased == 0)) {
			if (vdce_conf_chan->mode_state ==
			    VDCE_OPERATION_BLENDING) {

				vdce_conf_chan->register_config[num_pass].
				    vdce_res_strt_add_ybot =
				    vdce_conf_chan->
				    register_config[num_pass].
				    vdce_src_strt_add_ybot;
			} else {
				vdce_conf_chan->register_config[num_pass].
				    vdce_res_strt_add_ybot =
				    (vdce_conf_chan->
				     register_config[num_pass].
				     vdce_res_strt_add_ytop) + (res_size / 4);
			}

		} else {

			vdce_conf_chan->
			    register_config[num_pass].
			    vdce_res_strt_add_ybot =
			    (vdce_conf_chan->register_config[num_pass].
			     vdce_res_strt_add_ytop +
			     (((no_of_lines_output / 2))));
		}
	}
	/* configure top field chroma addreess */
	if (vdce_conf_chan->num_pass == VDCE_MULTIPASS) {
		if (num_pass == 1) {
			res_address = vdce_start->buffers[1].offset +
				(vdce_conf_chan->get_params.common_params.res_vsp_luminance *
				 vdce_start->res_horz_pitch);
		} else {
			res_address = vdce_start->buffers[1].offset;
		}
	} else {
		res_address = vdce_start->buffers[1].offset +
			(vdce_conf_chan->get_params.common_params.res_vsp_luminance *
			 vdce_start->res_horz_pitch);
	}
	if (flag == 0) {
		if (vdce_conf_chan->image_type_out == VDCE_IMAGE_FMT_420) {
			res_address = vdce_start->buffers[1].offset +
				no_of_lines_output + ((vdce_conf_chan->get_params.common_params.res_vsp_luminance/2) * vdce_start->res_horz_pitch);
		} else {
			res_address = (res_address + (no_of_lines_output));
		}
	}
	vdce_conf_chan->register_config[num_pass].vdce_res_strt_add_ctop =
	    (res_address);

	/* configuration of cbottom */
	divider = GET_CHROMA_DIVIDE_FACTOR(vdce_conf_chan->image_type_out);
	if (res_mode == VDCE_FRAME_MODE) {
		vdce_conf_chan->
		    register_config[num_pass].vdce_res_strt_add_cbot =
		    (vdce_conf_chan->register_config[num_pass].
		     vdce_res_strt_add_ctop + vdce_start->res_horz_pitch);
	} else {
		vdce_conf_chan->
		    register_config[num_pass].vdce_res_strt_add_cbot =
		    (vdce_conf_chan->register_config[num_pass].
		     vdce_res_strt_add_ctop + ((no_of_lines_output / divider)));
	}
	dev_dbg(vdce_device, "<fn	>Leaving out of	set address </fn>\n");
	/* configure top field chroma addreess */
	return 0;
}

/*
 * vdce_set_multipass_address :This function is process in 4:2:2 format for
 * resizing and blending mode
 */
int vdce_set_multipass_address(vdce_address_start_t * vdce_start,
			       channel_config_t * vdce_conf_chan)
{

	static unsigned int temp_address;
	int ret = 0;
	int temp_size;
	int divider = 2;
	int req_size = 0;
	unsigned int address = 0, pitch = 0, res_size = 0;
	unsigned int temp_pitch;

	dev_dbg(vdce_device,
		"<fn>vdce_set_multipass_address	Entering\n</fn>");
	/* initially this first pass , so we have this flag as started */
	/* take the offset and pitch into temporary variables */
	req_size = (vdce_conf_chan->get_params.common_params.
		    src_hsz_luminance *
		    vdce_conf_chan->get_params.common_params.src_vsz_luminance);
	if (device_config.inter_size < req_size) {
		dev_err(vdce_device,
			"intermediate buffer size is less than required\n");
		return -EINVAL;
	}
	temp_address = vdce_start->buffers[1].offset;
	address = vdce_start->buffers[1].offset;
	if (vdce_conf_chan->mode_state == VDCE_OPERATION_BLENDING) {
		pitch = vdce_start->src_horz_pitch;
	} else {
		pitch = vdce_start->res_horz_pitch;
	}
	temp_size = vdce_start->buffers[1].size;
	res_size = temp_size;

	vdce_start->buffers[1].size = ((device_config.inter_size * 2));

	/* Since first we have to do 4:2:2 to 4:2:0  */
	/* assign in intermediate address to output address */
	vdce_start->buffers[1].offset =
	    virt_to_phys(((void *)device_config.inter_buffer));
	/* change the output size */
	vdce_conf_chan->register_config[0].res_Y_sz =
	    ((vdce_conf_chan->get_params.common_params.
	      dst_hsz_luminance << RES_Y_HSZ_SHIFT)
	     & (RES_Y_HSZ_MASK));
	if (vdce_conf_chan->get_params.common_params.src_processing_mode ==
	    VDCE_INTERLACED) {
		divider = 4;
	}
	vdce_conf_chan->register_config[0].res_C_sz |=
	    ((vdce_conf_chan->get_params.common_params.dst_vsz_luminance /
	      divider << RES_C_VSZ_SHIFT) & (RES_C_VSZ_MASK));
	if (vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING) {
		vdce_conf_chan->register_config[1].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[1].
			   vdce_ctrl, SET_CHROMA_ENABLE);
		vdce_conf_chan->register_config[1].vdce_ctrl =
		    BITRESET(vdce_conf_chan->register_config[1].
			     vdce_ctrl, SET_LUMA_ENABLE);
	} else {
		vdce_conf_chan->register_config[0].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[0].
			   vdce_ctrl, SET_CHROMA_ENABLE);
		vdce_conf_chan->register_config[0].vdce_ctrl =
		    BITRESET(vdce_conf_chan->register_config[0].
			     vdce_ctrl, SET_LUMA_ENABLE);
	}
	/* assignning the address to the register configuration */
	ret = vdce_set_address(vdce_start, vdce_conf_chan, 0,
			       res_size, address, pitch);

	/* second pass. configuration */
	vdce_start->buffers[0].offset =
	    virt_to_phys(((void *)device_config.inter_buffer));
	/* configure input pitch */
	vdce_start->buffers[0].size = ((device_config.inter_size * 2));
	vdce_start->buffers[1].size = temp_size;
	/*configure output pitch and address */
	vdce_start->buffers[1].offset = temp_address;

	/* configure the addrress */
	temp_pitch = vdce_start->src_horz_pitch;
	vdce_start->src_horz_pitch = vdce_start->res_horz_pitch;
	ret = vdce_set_address(vdce_start, vdce_conf_chan, 1,
			       res_size, address, pitch);
	vdce_start->src_horz_pitch = temp_pitch;

	/* configure input and output size */
	vdce_conf_chan->register_config[1].src_Y_sz &= ~(SRC_Y_VSZ_MASK);
	if (vdce_conf_chan->get_params.common_params.src_processing_mode ==
	    VDCE_INTERLACED) {
		divider = 2;
	} else {
		divider = 1;
	}

	vdce_conf_chan->register_config[0].res_Y_sz |=
	    ((vdce_conf_chan->get_params.common_params.
	      dst_vsz_luminance / divider << RES_Y_VSZ_SHIFT)
	     & (RES_Y_VSZ_MASK));

	vdce_conf_chan->register_config[1].src_Y_sz =
	    ((vdce_conf_chan->get_params.common_params.dst_hsz_luminance
	      << SRC_Y_HSZ_SHIFT) & (SRC_Y_HSZ_MASK));

	vdce_conf_chan->register_config[1].src_C_sz =
	    ((vdce_conf_chan->get_params.common_params.
	      dst_hsz_luminance << SRC_C_HSZ_SHIFT) & (SRC_C_HSZ_MASK));

	vdce_conf_chan->register_config[1].res_Y_sz |=
	    ((vdce_conf_chan->get_params.common_params.
	      dst_hsz_luminance << RES_Y_HSZ_SHIFT)
	     & (RES_Y_HSZ_MASK));

	if (vdce_conf_chan->get_params.common_params.src_processing_mode
	    == VDCE_INTERLACED) {

		vdce_conf_chan->register_config[1].res_Y_sz |=
		    (((vdce_conf_chan->get_params.common_params.
		       dst_vsz_luminance / 2) << RES_Y_VSZ_SHIFT) &
		     (RES_Y_VSZ_MASK));
		vdce_conf_chan->register_config[1].src_Y_sz |=
		    ((vdce_conf_chan->get_params.common_params.
		      dst_vsz_luminance / 2 << SRC_Y_VSZ_SHIFT) &
		     (SRC_Y_VSZ_MASK));
		vdce_conf_chan->register_config[1].src_C_sz |=
		    ((vdce_conf_chan->get_params.common_params.
		      dst_vsz_luminance / 4 << SRC_C_VSZ_SHIFT) &
		     (SRC_C_VSZ_MASK));
	} else {
		vdce_conf_chan->register_config[1].res_Y_sz |=
		    ((vdce_conf_chan->get_params.common_params.
		      dst_vsz_luminance << RES_Y_VSZ_SHIFT) & (RES_Y_VSZ_MASK));
		vdce_conf_chan->register_config[1].src_Y_sz |=
		    ((vdce_conf_chan->get_params.common_params.
		      dst_vsz_luminance << SRC_Y_VSZ_SHIFT) & (SRC_Y_VSZ_MASK));
		vdce_conf_chan->register_config[1].src_C_sz |=
		    ((vdce_conf_chan->get_params.common_params.
		      dst_vsz_luminance / 2 << SRC_C_VSZ_SHIFT) &
		     (SRC_C_VSZ_MASK));
	}
	/* function to enable hardware */
	dev_dbg(vdce_device,
		"<fn>vdce_set_multipass_address	Leaving\n</fn>");
	return ret;
}

/*
 * vdce_start : This function enable the resize bit after doing
 * the hardware register configuration after which resizing
 * will be carried on.
 */
int vdce_start(vdce_address_start_t * vdce_start,
	       channel_config_t * vdce_conf_chan)
{
	/* holds the return value; */
	int ret = 0;
	/* conatains the input put and output buffer allocated size */
	int bufsize[3];
	int divider_in, divider_out, blend_enable, i;
	int dst_hsz_luma = 0, dst_vsz_luma = 0;
	int ccnt = 1;
	unsigned int ccv_only = 0, luma_status = 0;

	if(vdce_conf_chan->status == VDCE_CHAN_UNINITIALISED){
		dev_err(vdce_device, "channel un-initialized\n");
		return -EINVAL;
	}

	if((vdce_conf_chan->channel_mode == VDCE_MODE_NON_BLOCKING)
	   && vdce_isbusy()){
		return -EBUSY;
	}

	/* check tht hpitch and vpitch should be greater or equal
	   than hsize and vsize */
	if (vdce_start->src_horz_pitch <
	    vdce_conf_chan->get_params.common_params.src_hsz_luminance) {
		dev_err(vdce_device, "src horizontal pitch less than width \n");
		return -EINVAL;
	}
	if (vdce_start->res_horz_pitch
	    < vdce_conf_chan->get_params.common_params.dst_hsz_luminance) {
		dev_err(vdce_device, "horizontal pitch less than width \n");
		return -EINVAL;
	}
	divider_in = GET_DIVIDE_FACTOR(vdce_conf_chan->image_type_in);
	divider_out = GET_DIVIDE_FACTOR(vdce_conf_chan->image_type_out);
	/* check for the 8byte alignment for input address of
	   ytop,ybot,ctop and cbot in both image format */

	/* get the buffer size */
	bufsize[VDCE_BUF_IN] =
	    (vdce_start->src_horz_pitch *
	     (vdce_conf_chan->get_params.common_params.src_vsz_luminance *
	      divider_in / 2));
	bufsize[VDCE_BUF_OUT] =
	    (vdce_start->res_horz_pitch *
	     (vdce_conf_chan->get_params.common_params.dst_vsz_luminance *
	      divider_out) / 2);
	if ((vdce_start->res_horz_pitch) % 0x8 != 0) {
		dev_err(vdce_device, " invalid resultant pitch offset \n");
		return -EINVAL;
	}
	if ((vdce_start->src_horz_pitch) % 0x8 != 0) {
		dev_err(vdce_device, " invalid source  pitch offset \n");
		return -EINVAL;
	}
	bufsize[VDCE_BUF_BMP] =
	    (vdce_conf_chan->get_params.common_params.bmp_hsize *
	     vdce_conf_chan->get_params.common_params.bmp_vsize) / 4;
	blend_enable =
	    BITGET(vdce_conf_chan->register_config[0].vdce_ctrl,
		   SET_BLEND_ENABLE);
	blend_enable |=
	    BITGET(vdce_conf_chan->register_config[1].vdce_ctrl,
		   SET_BLEND_ENABLE);
	for (i = VDCE_BUF_IN; i <= VDCE_BUF_BMP; i++) {
		if (i == VDCE_BUF_BMP && blend_enable != 1) {
			continue;
		}		/* user pointer case */
		if (vdce_start->buffers[i].index < 0) {
			/* assignning the address to the register conf  */
			if (vdce_start->buffers[i].size < bufsize[i]) {
				dev_err(vdce_device, " invalid size \n");
				return -EINVAL;
			}
			if ((void *)vdce_start->buffers[i].virt_ptr == NULL) {
				dev_err(vdce_device, " Address is NULL \n");
				return -EINVAL;
			}
			/* user virtual pointer to physical address */
			vdce_start->buffers[i].offset =
			    vdce_uservirt_to_phys(vdce_start->buffers[i].
						  virt_ptr /*offset */ );
		} else {
			/*checking the index requested */
			if ((vdce_start->buffers[i].index)
			    > ((vdce_conf_chan->vdce_buffer[i].
				num_allocated - 1))) {
				dev_err(vdce_device,
					"Requested buffer not allocatedn");
				return -EINVAL;
			}
			vdce_start->buffers[i].offset = virt_to_phys(((void *)
					      vdce_conf_chan->vdce_buffer[i].
					      buffers
					      [vdce_start->
					       buffers
					       [i].
					       index]));
			vdce_start->buffers[i].size =
			    vdce_conf_chan->vdce_buffer[i].size;
		}
		/* check alignment for ytop */
		if (vdce_start->buffers[i].offset % 0x8 != 0) {
			dev_err(vdce_device, "Address not 8 byte aligned \n");
			return -EINVAL;
		}
	}
	if (blend_enable) {
		if ((vdce_start->bmp_pitch * 4) <
		    vdce_conf_chan->get_params.common_params.bmp_hsize) {
			dev_err(vdce_device, "bmp pitch less than width \n");
			return -EINVAL;
		}
		if (vdce_start->bmp_pitch % 0x8 != 0) {
			dev_err(vdce_device, " bmp pitch not aligned \n");
			return -EINVAL;
		}
	}
	/* check for the 8byte alignment for output address of
	   ytop,ybot,ctop and cbot in both image format */
	if ((((vdce_start->buffers[0].size) / 4) % 0x8) != 0) {
		dev_err(vdce_device, " invalid src address  \n");
		return -EINVAL;
	}
	if (((vdce_start->buffers[1].size) / 4) % 0x8) {
		dev_err(vdce_device, " invalid resultant address \n");
		return -EINVAL;
	}

	if ((vdce_conf_chan->num_pass == VDCE_MULTIPASS) &&
	    (vdce_conf_chan->luma_chroma_phased == 0)) {
		ret = vdce_set_multipass_address(vdce_start, vdce_conf_chan);
		if (ret < 0) {
			return -EINVAL;
		}
	} else {
		/* assignning the address to the register configuration */
		ret = vdce_set_address(vdce_start, vdce_conf_chan, 0, 0, 0, 0);

		if (vdce_conf_chan->luma_chroma_phased == 1) {
			memcpy(&vdce_conf_chan->register_config[1],
			       &vdce_conf_chan->register_config[0],
			       sizeof(struct vdce_hw_config));
			vdce_conf_chan->register_config[0].vdce_ctrl =
			    BITRESET(vdce_conf_chan->register_config[0].
				     vdce_ctrl, SET_CHROMA_ENABLE);
			vdce_conf_chan->register_config[1].vdce_ctrl =
			    BITRESET(vdce_conf_chan->register_config[1].
				     vdce_ctrl, SET_LUMA_ENABLE);
		}
	}


	/* Check to see if channel is busy or not */
	if(vdce_isbusy()) {
		while(vdce_isbusy()){
			schedule();
		}
	}
	if(irqraised1 == 0) {
		while(irqraised1 == 0) {
			schedule();
		}
	}

	/* Wait for getting access to the hardware */
	wait_for_completion(&(device_config.device_access));

	vdce_current_chan = vdce_conf_chan;

	/* start the process */
	vdce_conf_chan->vdce_complete = VDCE_PASS1_STARTED;
	if (vdce_conf_chan->mode_state ==
	    VDCE_OPERATION_CHROMINANCE_CONVERSION ||
	    vdce_conf_chan->mode_state == VDCE_OPERATION_PRE_CODECMODE ||
	    vdce_conf_chan->mode_state == VDCE_OPERATION_POST_CODECMODE ||
	    vdce_conf_chan->mode_state == VDCE_OPERATION_TRANS_CODECMODE) {
		vdce_conf_chan->edma_operation = 1;

		luma_status = vdce_conf_chan->register_config[0].vdce_ctrl;
		if ((luma_status & 0x6) == 0x6) {
			vdce_conf_chan->edma_operation = 1;
		} else {
			vdce_conf_chan->edma_operation = 0;
		}
		if ((vdce_conf_chan->get_params.common_params.src_mode !=
		     vdce_conf_chan->get_params.common_params.res_mode)) {
			vdce_conf_chan->edma_operation = 0;
		}
		ccv_only = vdce_conf_chan->register_config[0].vdce_ctrl;
		if (ccv_only & 0xb00) {
			vdce_conf_chan->edma_operation = 0;
		}
		if (vdce_conf_chan->edma_operation == 1) {
			vdce_conf_chan->register_config[0].vdce_ctrl =
			    BITRESET(vdce_conf_chan->register_config[0].
				     vdce_ctrl, SET_LUMA_ENABLE);
		}
	}
	vdce_hw_setup(&vdce_conf_chan->register_config[0]);
	/*function call to enable ge hardware */
	ret = vdce_enable(&vdce_conf_chan->register_config[0]);

	if (vdce_conf_chan->edma_operation == 1) {
		dst_hsz_luma = vdce_conf_chan->
		    get_params.common_params.dst_hsz_luminance;

		dst_vsz_luma = vdce_conf_chan->
		    get_params.common_params.dst_vsz_luminance;
		if (vdce_conf_chan->get_params.common_params.src_mode ==
		    VDCE_FIELD_MODE) {
			ccnt = 2;
			dst_vsz_luma = dst_vsz_luma / 2;
		}
		ret = edma3_memcpy(dst_hsz_luma, dst_vsz_luma,
				   ccnt, vdce_start);
		vdce_conf_chan->register_config[0].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[0].
			   vdce_ctrl, SET_LUMA_ENABLE);
	}

	/* waiting for processing to be complete */
	ret = wait_for_completion_interruptible(
			&(device_config.sem_isr));
	if(ret) {
		wait_for_completion(&(device_config.sem_isr));
	}

	/* Release access to hardware completion */
	complete(&(device_config.device_access));

	dev_dbg(vdce_device, "<fn> vdce_start L</fn>\n");
	return ret;
}

/*
 * vdce_check_global_params : Function to check the error conditions
 */
int vdce_check_common_params(vdce_params_t * params)
{
	int ret = 0;
	int hrsz_mag = 0, vrsz_mag = 0;
	int i = 0, prcs = 0;
	dev_dbg(vdce_device, "<fn>vdce_check_global_params E</fn>\n");
	/* Checking the validity of various enums */
	ret = CHECK_MODE_RANGE(params->vdce_mode, VDCE_OPERATION_BLENDING);
	if (ret) {
		dev_err(vdce_device, "Invalid mode\n");
		return -EINVAL;
	}
	/* checking validity for precodec params */
	if (params->vdce_mode == VDCE_OPERATION_POST_CODECMODE ||
	    params->vdce_mode == VDCE_OPERATION_TRANS_CODECMODE) {
		/* checking validity for postcodec params */
		if ((params->modes_control & RSZ_ENABLE_MASK) ==
		    (RSZ_ENABLE_MASK)
		    && (params->modes_control & BLEND_ENABLE_MASK) ==
		    (BLEND_ENABLE_MASK)) {
			dev_err(vdce_device,
				"Cannot	enable due to Hardware limitation \n");
			return -EINVAL;

		}
	}
	/* if resizing than check for resize ratio */
	if ((params->vdce_mode == VDCE_OPERATION_RESIZING) ||
	    ((params->vdce_mode == VDCE_OPERATION_PRE_CODECMODE) &&
	     ((params->modes_control & RSZ_ENABLE_MASK) == (RSZ_ENABLE_MASK)))
	    || ((params->vdce_mode == VDCE_OPERATION_POST_CODECMODE)
		&& ((params->modes_control & RSZ_ENABLE_MASK) ==
		    (RSZ_ENABLE_MASK)))
	    || ((params->vdce_mode == VDCE_OPERATION_TRANS_CODECMODE)
		&& ((params->modes_control & RSZ_ENABLE_MASK) ==
		    (RSZ_ENABLE_MASK)))) {
		hrsz_mag =
		    (params->common_params.src_hsz_luminance * 256) /
		    (params->common_params.dst_hsz_luminance);
		vrsz_mag =
		    (params->common_params.src_vsz_luminance * 256) /
		    (params->common_params.dst_vsz_luminance);
		/* checking horizontal phase */
		if ((hrsz_mag < MIN_RSZ_MAG_RATIO) ||
		    (hrsz_mag > MAX_RSZ_MAG_RATIO)) {
			dev_err(vdce_device, "Invalid Horizontal ratio \n");
			return -EINVAL;
		}
		/* checking vertical phase */
		if ((vrsz_mag < MIN_RSZ_MAG_RATIO)
		    || (vrsz_mag > MAX_RSZ_MAG_RATIO)) {
			dev_err(vdce_device, "Invalid Vertical ratio \n");
			return -EINVAL;
		}
	}
	if ((params->vdce_mode == VDCE_OPERATION_RESIZING) &&
	    (params->vdce_mode_params.rsz_params.
	     rsz_op_mode == VDCE_CODECMODE_MPEG1)) {
		dev_err(vdce_device,
			" This facility not supported due to hardware \n\n");
	}
	if (params->common_params.prcs_unit_value == 0) {
		prcs = hrsz_mag / 256;
		for (i = 4; i >= 0; i--) {
			if ((prcs_array_value[i] * prcs + 9) <= 256) {
				break;
			}
		}
		if (i < 0) {
			i = 0;
		}
		prcs = prcs_array_value[i];
	} else {
		prcs = params->common_params.prcs_unit_value;
	}
	if (prcs > 256) {
		return -EINVAL;
	}
	if (params->vdce_mode == VDCE_OPERATION_EDGE_PADDING) {
		if ((params->vdce_mode_params.epad_params.hext_luma > prcs)
		    || (params->vdce_mode_params.epad_params.hext_chroma >
			prcs))
			return -EINVAL;
	}
	if (params->common_params.src_processing_mode == VDCE_PROGRESSIVE) {
		if ((params->common_params.src_mode == VDCE_FIELD_MODE) ||
		    (params->common_params.res_mode == VDCE_FIELD_MODE)) {
			dev_err(vdce_device, "Invalid Mode ratio \n");
			return -EINVAL;
		}
	}
	if (ret) {
		dev_dbg(vdce_device, "Inavlid return \n");
		return -EINVAL;
	} else {
		return 0;
	}
	dev_dbg(vdce_device, "<fn>vdce_check_global_params L</fn>\n");
}

/*
 * vdce_set_size_fmt : Setting resizing parameters .
 */
int vdce_set_size_fmt(vdce_common_params_t * params,
		      channel_config_t * vdce_conf_chan, int num_pass)
{
	int ret = 0;
	int mode;
	int prcs_nvalue = 0, i = 0;
	int prcs = 0;
	vdce_image_fmt_t src_image_type, divider = 1;
	dev_dbg(vdce_device, "<fn>vdce_set_size_fmt	E</fn>\n");
	/* setting luminance processing enable bit */
	if (params->proc_control == VDCE_LUMA_ENABLE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_LUMA_ENABLE);
	} else if (params->proc_control == VDCE_CHROMA_ENABLE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_CHROMA_ENABLE);
	} else {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_CHROMA_ENABLE);
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_LUMA_ENABLE);
	}

	/* setting input horizontal alf enable */
	if (params->field_status == VDCE_TOP_ENABLE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_TOP_ENABLE);
	} else if (params->field_status == VDCE_BOTTOM_ENABLE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_BOT_ENABLE);
	} else {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_BOT_ENABLE);
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_TOP_ENABLE);
	}
	/* setting src i/o mode */
	if (params->src_mode == VDCE_FRAME_MODE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_SRC_MODE);
	} else {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITRESET(vdce_conf_chan->register_config[num_pass].
			     vdce_ctrl, SET_SRC_MODE);
	}
	/* setting res i/o mode */
	if (params->res_mode == VDCE_FRAME_MODE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_RES_MODE);
	} else {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITRESET(vdce_conf_chan->register_config[num_pass].
			     vdce_ctrl, SET_RES_MODE);
	}
	if (num_pass == 1 &&
	    (vdce_conf_chan->mode_state != VDCE_OPERATION_RESIZING)) {
		if (params->src_mode == VDCE_FRAME_MODE) {
			vdce_conf_chan->register_config[0].vdce_ctrl =
			    BITSET(vdce_conf_chan->register_config[0].
				   vdce_ctrl, SET_RES_MODE);
		} else {

			vdce_conf_chan->register_config[0].vdce_ctrl =
			    BITRESET(vdce_conf_chan->register_config[0].
				     vdce_ctrl, SET_RES_MODE);
		}
	}
	if (num_pass == 1 &&
	    (vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING)) {
		if (params->res_mode == VDCE_FRAME_MODE) {
			vdce_conf_chan->register_config[1].vdce_ctrl =
			    BITSET(vdce_conf_chan->register_config[1].
				   vdce_ctrl, SET_SRC_MODE);
		} else {

			vdce_conf_chan->register_config[1].vdce_ctrl =
			    BITRESET(vdce_conf_chan->register_config[1].
				     vdce_ctrl, SET_SRC_MODE);
		}
	}
	/* setting bmp i/o mode */
	if (params->src_bmp_mode == VDCE_FRAME_MODE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_BMP_MODE);
	} else {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITRESET(vdce_conf_chan->register_config[num_pass].
			     vdce_ctrl, SET_BMP_MODE);
	}
	/* setting bmp i/o mode */
	if (params->src_processing_mode == VDCE_PROGRESSIVE) {
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITSET(vdce_conf_chan->register_config[num_pass].
			   vdce_ctrl, SET_PRO_MODE);
	} else {
		/* atleat top field or bottom field should be configured for
		   interlaced */
		vdce_conf_chan->register_config[num_pass].vdce_ctrl =
		    BITRESET(vdce_conf_chan->register_config[num_pass].
			     vdce_ctrl, SET_PRO_MODE);
	}
	vdce_conf_chan->register_config[num_pass].vdce_req_sz =
	    vdce_conf_chan->register_config[num_pass].
	    vdce_req_sz & ~(VDCE_REQ_SZ_MASK);

	vdce_conf_chan->register_config[num_pass].vdce_req_sz =
	    (vdce_conf_chan->register_config[num_pass].vdce_req_sz |
	     ((256) << VDCE_REQ_SZ_SHIFT));

	if (params->prcs_unit_value == 0) {
		prcs_nvalue =
		    (params->src_hsz_luminance / (params->dst_hsz_luminance));
		for (i = 4; i >= 0; i--) {
			if ((prcs_array_value[i] * prcs_nvalue + 9) <= 256) {
				break;
			}
		}
		if (i < 0) {
			i = 0;
		}
		prcs = prcs_array_value[i];
	} else {
		prcs = params->prcs_unit_value;
	}
	vdce_conf_chan->register_config[num_pass].vdce_prcs_unit_size =
	    prcs & (VDCE_PRCS_UNIT_SIZE_MASK);

	/* Configuration of luma size */
	vdce_conf_chan->register_config[num_pass].src_Y_sz |=
	    ((params->src_hsz_luminance << SRC_Y_HSZ_SHIFT) & (SRC_Y_HSZ_MASK));

	if (params->src_processing_mode == VDCE_INTERLACED) {

		vdce_conf_chan->register_config[num_pass].src_Y_sz |=
		    ((params->
		      src_vsz_luminance /
		      2 << SRC_Y_VSZ_SHIFT) & (SRC_Y_VSZ_MASK));
	} else {
		vdce_conf_chan->register_config[num_pass].src_Y_sz |=
		    ((params->
		      src_vsz_luminance << SRC_Y_VSZ_SHIFT) & (SRC_Y_VSZ_MASK));
	}

	vdce_conf_chan->register_config[num_pass].res_Y_sz |=
	    ((params->dst_hsz_luminance << RES_Y_HSZ_SHIFT) & (RES_Y_HSZ_MASK));

	if (params->src_processing_mode == VDCE_INTERLACED) {
		vdce_conf_chan->register_config[num_pass].res_Y_sz |=
		    ((params->
		      dst_vsz_luminance /
		      2 << RES_Y_VSZ_SHIFT) & (RES_Y_VSZ_MASK));
	} else {
		vdce_conf_chan->register_config[num_pass].res_Y_sz |=
		    ((params->
		      dst_vsz_luminance << RES_Y_VSZ_SHIFT) & (RES_Y_VSZ_MASK));
	}
	/* Configuration of chroma size */
	mode = ((vdce_conf_chan->register_config[num_pass].vdce_ctrl
		 & VDCE_MODE_MASK) >> VDCE_MODE_SHIFT);

	if ((vdce_conf_chan->mode_state == VDCE_OPERATION_PRE_CODECMODE) ||
	    ((vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING) &&
	     (mode == MODE_PRECODEC)) ||
	    ((vdce_conf_chan->mode_state ==
	      VDCE_OPERATION_CHROMINANCE_CONVERSION) &&
	     (mode == MODE_PRECODEC)) || (mode == MODE_PRECODEC) ||
	    (vdce_conf_chan->mode_state == VDCE_OPERATION_EDGE_PADDING)) {
		src_image_type = VDCE_IMAGE_FMT_422;

	} else {
		src_image_type = VDCE_IMAGE_FMT_420;
	}
	/* mode is 420 so chroma value is reduced by 2 */
	divider = (params->src_processing_mode == VDCE_INTERLACED) ? 4 : 2;
	if (src_image_type == VDCE_IMAGE_FMT_420) {
		divider = (params->
			   src_processing_mode == VDCE_INTERLACED) ? 4 : 2;
		vdce_conf_chan->register_config[num_pass].src_C_sz |=
		    ((params->src_vsz_luminance / divider << SRC_C_VSZ_SHIFT)
		     & (SRC_C_VSZ_MASK));
	} else if (src_image_type == VDCE_IMAGE_FMT_422) {
		divider = (params->
			   src_processing_mode == VDCE_INTERLACED) ? 2 : 1;
		vdce_conf_chan->register_config[num_pass].src_C_sz |=
		    vdce_conf_chan->register_config[num_pass].src_C_sz |=
		    ((params->src_vsz_luminance / divider << SRC_C_VSZ_SHIFT)
		     & (SRC_C_VSZ_MASK));

	} else {
		return -EINVAL;
	}

	vdce_conf_chan->register_config[num_pass].src_C_sz |=
	    ((params->src_hsz_luminance << SRC_C_HSZ_SHIFT) & (SRC_C_HSZ_MASK));

	vdce_conf_chan->register_config[num_pass].res_C_sz |=
	    ((params->dst_hsz_luminance << RES_C_HSZ_SHIFT) & (RES_C_HSZ_MASK));
	if ((vdce_conf_chan->mode_state == VDCE_OPERATION_PRE_CODECMODE) ||
	    (mode == MODE_TRANSCODEC) || (mode == MODE_PRECODEC)) {

		divider = (params->
			   src_processing_mode == VDCE_INTERLACED) ? 4 : 2;

	} else {
		divider = (params->
			   src_processing_mode == VDCE_INTERLACED) ? 2 : 1;
	}
	vdce_conf_chan->register_config[num_pass].res_C_sz |=
	    ((params->
	      dst_vsz_luminance /
	      divider << RES_C_VSZ_SHIFT) & (RES_C_VSZ_MASK));
	/* Configuration of bitmap size */
	divider = (params->src_processing_mode == VDCE_INTERLACED) ? 2 : 1;
	vdce_conf_chan->register_config[num_pass].src_bmp_sz |=
	    ((params->bmp_hsize << SRC_BMP_HSZ_SHIFT) & (SRC_BMP_HSZ_MASK));

	vdce_conf_chan->register_config[num_pass].src_bmp_sz |=
	    ((params->
	      bmp_vsize / divider << SRC_BMP_VSZ_SHIFT) & (SRC_BMP_VSZ_MASK));

	vdce_conf_chan->register_config[num_pass].src_Y_strt_ps =
		params->src_hsp_luminance;
	vdce_conf_chan->register_config[num_pass].res_Y_strt_ps =
		params->res_hsp_luminance;
	/* configuration for starting position */
	if (vdce_conf_chan->num_pass == VDCE_MULTIPASS) {
		if (num_pass == 1) {
			vdce_conf_chan->register_config[num_pass].src_C_strt_ps =
				0;
			vdce_conf_chan->register_config[num_pass].res_C_strt_ps =
				params->res_hsp_luminance;
		} else {
			vdce_conf_chan->register_config[num_pass].src_C_strt_ps =
				params->src_hsp_luminance;
			vdce_conf_chan->register_config[num_pass].res_C_strt_ps =
				0;
		}
	} else {
		vdce_conf_chan->register_config[num_pass].src_C_strt_ps =
			params->src_hsp_luminance;
		vdce_conf_chan->register_config[num_pass].res_C_strt_ps =
			params->res_hsp_luminance;
	}

	vdce_conf_chan->register_config[num_pass].src_bmp_strt_ps |=
	    ((params->bmp_hsp_bitmap) & (SRC_BMP_STRT_HPS_MASK));

	vdce_conf_chan->register_config[num_pass].res_bmp_strt_ps |=
	    ((params->res_hsp_bitmap) & (SRC_BMP_HSZ_MASK));

	vdce_conf_chan->register_config[num_pass].res_bmp_strt_ps |=
	    ((params->res_vsp_bitmap << RES_BMP_STRT_VPS_SHIFT)
	     & (RES_BMP_STRT_VPS_MASK));
	/* hardcoding the image format as raster scanning */
	vdce_conf_chan->register_config[num_pass].vdce_sdr_fmt =
	    VDCE_RASTER_SCANNING;

	dev_dbg(vdce_device, "<fn>vdce_set_size_fmt	L\n</fn>");
	return ret;
}

/*
 * vdce_set_rmapparams : Setting range mapping parameters.
 */
int vdce_set_rmapparams(vdce_rmap_params_t * params,
			channel_config_t * vdce_conf_chan, int index)
{
	int ret = 0;
	dev_dbg(vdce_device, "<fn>vdce_set_rmmaparams E\n</fn>");
	dev_dbg(vdce_device, "The	value s	%d\n", params->coeff_y);
	if (params->rmap_yenable == VDCE_FEATURE_ENABLE) {
		vdce_conf_chan->register_config[index].rgmp_ctrl =
		    BITSET(vdce_conf_chan->register_config[index].
			   rgmp_ctrl, SET_RMAP_YENABLE);

		dev_dbg(vdce_device, "Entering and setting value \n");
		vdce_conf_chan->register_config[index].rgmp_ctrl |=
		    ((params->coeff_y) << RANGE_MAP_Y_SHIFT) & RANGE_MAP_Y_MASK;

	} else {
		vdce_conf_chan->register_config[index].rgmp_ctrl =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rgmp_ctrl, SET_RMAP_YENABLE);
	}
	if (params->rmap_cenable == VDCE_FEATURE_ENABLE) {
		vdce_conf_chan->register_config[index].rgmp_ctrl =
		    BITSET(vdce_conf_chan->register_config[index].
			   rgmp_ctrl, SET_RMAP_CENABLE);

		vdce_conf_chan->register_config[index].rgmp_ctrl |=
		    ((params->
		      coeff_c) << (RANGE_MAP_C_SHIFT)) & RANGE_MAP_C_MASK;
	} else {
		vdce_conf_chan->register_config[index].rgmp_ctrl =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rgmp_ctrl, SET_RMAP_CENABLE);
	}
	vdce_conf_chan->register_config[index].vdce_ctrl =
	    BITSET(vdce_conf_chan->register_config[index].vdce_ctrl,
		   SET_RMAP_ENABLE);
	dev_dbg(vdce_device, "<fn>vdce_set_rmapparams L\n</fn>");
	return ret;
}

/*
 * vdce_set_rszparams : Setting resizing parameters .
 */
int vdce_set_rszparams(vdce_params_t * params,
		       channel_config_t * vdce_conf_chan,
		       vdce_mode_operation_t mode, int index)
{
	int ret = 0;
	vdce_rsz_params_t rsz_params;
	int hrsz_mag, vrsz_mag;
	if (mode == VDCE_OPERATION_PRE_CODECMODE) {
		rsz_params =
		    (params->vdce_mode_params.precodec_params.rsz_params);
	} else if (mode == VDCE_OPERATION_POST_CODECMODE) {
		rsz_params =
		    (params->vdce_mode_params.postcodec_params.rsz_params);
	} else if (mode == VDCE_OPERATION_TRANS_CODECMODE) {
		rsz_params =
		    (params->vdce_mode_params.transcodec_params.rsz_params);
	} else if (mode == VDCE_OPERATION_RESIZING) {
		rsz_params = (params->vdce_mode_params.rsz_params);
	} else {
		memset(&rsz_params, 0, sizeof(rsz_params));
	}

	dev_dbg(vdce_device, "<fn>vdce_set_rszparams E\n</fn>");
	/* setting input horizontal alf enable */
	if (rsz_params.hrsz_alf_enable == VDCE_FEATURE_ENABLE) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_HRSZ_ALF_ENABLE);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_HRSZ_ALF_ENABLE);
	}
	/* setting input vertical alf format */
	if (rsz_params.vrsz_alf_enable == VDCE_FEATURE_ENABLE) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_VRSZ_ALF_ENABLE);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_VRSZ_ALF_ENABLE);
	}
	/* setting H type */
	if (rsz_params.vrsz_mode == VDCE_ALGO_TAP_4LINEAR_INTERPOLATION) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_RSZ_V_TYPE);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_RSZ_V_TYPE);
	}
	/* setting V type */
	if (rsz_params.hrsz_mode == VDCE_ALGO_TAP_4LINEAR_INTERPOLATION) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_RSZ_H_TYPE);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_RSZ_H_TYPE);
	}
	/* setting V type */
	if (rsz_params.hrsz_alf_mode == VDCE_ALFMODE_MANUAL) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_RSZ_H_ALF_mode);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_RSZ_H_ALF_mode);
	}
	/* setting intensity of resizer */
	if ((rsz_params.hrsz_alf_mode == VDCE_ALFMODE_MANUAL)
		&& (rsz_params.hrsz_alf_enable == VDCE_FEATURE_ENABLE)) {
		if (rsz_params.hrsz_alf_intensity > MAX_RSZ_INTENSITY) {
			rsz_params.hrsz_alf_intensity = MAX_RSZ_INTENSITY;
		}
		vdce_conf_chan->register_config[index].
			rsz_alf_intensity |=
			(((rsz_params.
			   hrsz_alf_intensity) << RSZ_ALF_INTENSITY_SHIFT)
			 & RSZ_ALF_INTENSITY_MASK);
	} else {
		vdce_conf_chan->register_config[index].rsz_alf_intensity = 0;
	}
	/* if resizer used check for the params */
	hrsz_mag =
	    (params->common_params.src_hsz_luminance *
	     256) / (params->common_params.dst_hsz_luminance);
	vdce_conf_chan->register_config[index].rsz_h_mag |=
	    (((hrsz_mag) << RSZ_H_MAG_SHIFT) & (RSZ_H_MAG_MASK));
	vrsz_mag =
	    (params->common_params.src_vsz_luminance *
	     256) / (params->common_params.dst_vsz_luminance);
	vdce_conf_chan->register_config[index].rsz_v_mag |=
	    (((vrsz_mag) << RSZ_V_MAG_SHIFT)) & (RSZ_V_MAG_MASK);

	/* setting horizontal enable bit */
	if (hrsz_mag >= 256) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_HRSZ_ENABLE);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_HRSZ_ENABLE);
	}
	/* setting vertical enable bit */
	if (vrsz_mag >= 256) {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITSET(vdce_conf_chan->register_config[index].rsz_mode,
			   SET_VRSZ_ENABLE);
	} else {
		vdce_conf_chan->register_config[index].rsz_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     rsz_mode, SET_VRSZ_ENABLE);
	}
	vdce_conf_chan->register_config[index].vdce_ctrl =
	    BITSET(vdce_conf_chan->register_config[index].vdce_ctrl,
		   SET_RSZ_ENABLE);
	dev_dbg(vdce_device, "<fn>vdce_set_rszparams L\n</fn>");
	return ret;
}

/*
 * vdce_set_epadparams : Setting EPAD parameters.
 */
int vdce_set_epadparams(vdce_epad_params_t * params,
			channel_config_t * vdce_conf_chan, int index)
{
	int ret = 0;
	dev_dbg(vdce_device, "<fn>vdce_set_epadparams E \n</fn>");
	/* configuring luma vertical extension value */
	vdce_conf_chan->register_config[index].epd_luma_width |=
	    (((params->vext_luma) << EPD_Y_VEXT_SHIFT) & EPD_Y_VEXT_MASK);

	/* configuring luma horizontal extension value */
	vdce_conf_chan->register_config[index].epd_luma_width |=
	    ((params->hext_luma) << (EPD_Y_HEXT_SHIFT)) & (EPD_Y_HEXT_MASK);

	/* configuring chroma vertical extension value */
	vdce_conf_chan->register_config[index].epd_chroma_width |=
	    ((params->vext_chroma) << (EPD_C_VEXT_SHIFT)) & EPD_C_VEXT_MASK;

	/* configuring chroma horizontal extension value */
	vdce_conf_chan->register_config[index].epd_chroma_width |=
	    (((params->hext_chroma) << EPD_C_HEXT_SHIFT) & EPD_C_HEXT_MASK);
	dev_dbg(vdce_device, "<fn>vdce_set_epadparams L\n</fn>");
	return ret;
}

/*
 * vdce_set_ccvparams : Setting CCV parameters.
 */
int vdce_set_ccvparams(vdce_ccv_params_t * params,
		       channel_config_t * vdce_conf_chan, int mode, int index)
{
	int ret = 0;
	dev_dbg(vdce_device, "<fn>vdce_set_ccvparams E\n</fn>");
	/* setting output format */
	if (mode == MODE_PRECODEC || mode == MODE_TRANSCODEC) {
		if (params->codec_mode_out == VDCE_CODECMODE_MPEG1) {
			vdce_conf_chan->register_config[index].ccv_mode =
			    BITSET(vdce_conf_chan->register_config[index].
				   ccv_mode, SET_CCV_OUT_MPEG1);

			vdce_conf_chan->register_config[index].vdce_ctrl =
			    BITSET(vdce_conf_chan->register_config[index].
				   vdce_ctrl, SET_CCV_ENABLE);

		} else {
			vdce_conf_chan->register_config[index].ccv_mode =
			    BITRESET(vdce_conf_chan->
				     register_config[index].ccv_mode,
				     SET_CCV_OUT_MPEG1);
		}
	}
	if (mode == MODE_POSTCODEC || mode == MODE_TRANSCODEC) {
		/* setting input format */
		if (params->codec_mode_in == VDCE_CODECMODE_MPEG1) {
			vdce_conf_chan->register_config[index].ccv_mode =
			    BITSET(vdce_conf_chan->register_config[index].
				   ccv_mode, SET_CCV_IN_MPEG1);
			vdce_conf_chan->register_config[index].vdce_ctrl =
			    BITSET(vdce_conf_chan->register_config[index].
				   vdce_ctrl, SET_CCV_ENABLE);
		} else {
			vdce_conf_chan->register_config[index].ccv_mode =
			    BITRESET(vdce_conf_chan->
				     register_config[index].ccv_mode,
				     SET_CCV_IN_MPEG1);
		}
	}
	/* setting H type */
	if (params->hccv_type == VDCE_ALGO_TAP_4LINEAR_INTERPOLATION) {
		vdce_conf_chan->register_config[index].ccv_mode =
		    BITSET(vdce_conf_chan->register_config[index].ccv_mode,
			   SET_CCV_H_TYPE);
	} else {
		vdce_conf_chan->register_config[index].ccv_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     ccv_mode, SET_CCV_H_TYPE);
	}
	/* setting V type */
	if (params->vccv_type == VDCE_ALGO_TAP_4LINEAR_INTERPOLATION) {
		vdce_conf_chan->register_config[index].ccv_mode =
		    BITSET(vdce_conf_chan->register_config[index].ccv_mode,
			   SET_CCV_V_TYPE);
	} else {
		vdce_conf_chan->register_config[index].ccv_mode =
		    BITRESET(vdce_conf_chan->register_config[index].
			     ccv_mode, SET_CCV_V_TYPE);
	}
	vdce_conf_chan->register_config[index].vdce_ctrl |=
	    ((mode << VDCE_MODE_SHIFT) & (VDCE_MODE_MASK));
	dev_dbg(vdce_device, "<fn>vdce_get_ccvparams L\n</fn>");
	return ret;
}

/*
 * vdce_set_blendparams : Setting blending parameters.
 */
int vdce_set_blendparams(vdce_blend_params_t * params,
			 channel_config_t * vdce_conf_chan, int mode, int index)
{
	int i = 0;
	int ret = 0;
	dev_dbg(vdce_device, "<fn>vdce_set_blendparams E\n</fn>");
	for (i = 0; i < MAX_BLEND_TABLE; i++) {
		/* configuring blend factor for zero blend value parameters */
		vdce_conf_chan->register_config[index].bld_lut[i] |=
		    ((params->bld_lut[i].
		      blend_value) << BLD_LUT_FCT_SHIFT) & (BLD_LUT_FCT_MASK);

		/* configuring Cr value for zero blend value parameters */
		vdce_conf_chan->register_config[index].bld_lut[i] |=
		    ((params->bld_lut[i].blend_cr) << BLD_LUT_CR_SHIFT) &
		    (BLD_LUT_CR_MASK);

		/* configuring cb value for zero blend value parameters */
		vdce_conf_chan->register_config[index].bld_lut[i] |=
		    ((params->bld_lut[i].blend_cb) << BLD_LUT_CB_SHIFT) &
		    (BLD_LUT_CB_MASK);
		/* configuring Y value for zero blend value parameters */
		vdce_conf_chan->register_config[index].bld_lut[i] |=
		    ((params->bld_lut[i].blend_y) << BLD_LUT_Y_SHIFT) &
		    BLD_LUT_Y_MASK;
	}
	vdce_conf_chan->register_config[index].vdce_ctrl =
	    BITSET(vdce_conf_chan->register_config[index].vdce_ctrl,
		   SET_BLEND_ENABLE);
	dev_dbg(vdce_device, "<fn>vdce_set_blendparams E\n</fn>");
	return ret;
}

/*
 * vdce_set_params : Function to set the ge parameters
 */
int vdce_set_params(vdce_params_t * params, channel_config_t * vdce_conf_chan)
{
	int ret = 0;
	int pass_num = 0;
	int mode;
	vdce_ccv_params_t ccv_params;
	dev_dbg(vdce_device, "<fn> vdce_set_params E\n</fn>");

	memset(&(vdce_conf_chan->register_config[0]), 0,
	       sizeof(vdce_hw_config_t));
	memset(&(vdce_conf_chan->register_config[1]), 0,
	       sizeof(vdce_hw_config_t));

	vdce_conf_chan->status = VDCE_CHAN_UNINITIALISED;
	vdce_conf_chan->mode_state = params->vdce_mode;
	vdce_conf_chan->num_pass = VDCE_SINGLE_PASS;
	/* configuration of Pre-codec mode */
	if (params->vdce_mode == VDCE_OPERATION_PRE_CODECMODE) {
		if (((params->modes_control & RSZ_ENABLE_MASK) ==
		     (RSZ_ENABLE_MASK))) {
			ret = vdce_set_rszparams(params, vdce_conf_chan,
						 VDCE_OPERATION_PRE_CODECMODE,
						 pass_num);
		}
		ret = vdce_set_ccvparams(&(params->vdce_mode_params.
					   precodec_params.ccv_params),
					 vdce_conf_chan, MODE_PRECODEC,
					 pass_num);

		vdce_conf_chan->register_config[pass_num].vdce_ctrl |=
		    ((MODE_PRECODEC << VDCE_MODE_SHIFT) & (VDCE_MODE_MASK));

	} else if (params->vdce_mode == VDCE_OPERATION_POST_CODECMODE) {
		/* configuration of Post-codec mode */
		/* setting resizing params and enabling it */
		if (((params->modes_control & RSZ_ENABLE_MASK) ==
		     (RSZ_ENABLE_MASK))) {
			ret = vdce_set_rszparams(params, vdce_conf_chan,
						 VDCE_OPERATION_POST_CODECMODE,
						 pass_num);
			if (params->vdce_mode_params.postcodec_params.
			    ccv_params.codec_mode_in == VDCE_CODECMODE_MPEG1) {
				vdce_conf_chan->luma_chroma_phased = 1;
			}
		}
		ret =
		    vdce_set_ccvparams(&
				       (params->vdce_mode_params.
					postcodec_params.ccv_params),
				       vdce_conf_chan, MODE_POSTCODEC,
				       pass_num);
		/* setting range-mapping params and enabling it */
		if (((params->modes_control & RMAP_ENABLE_MASK) ==
		     (RMAP_ENABLE_MASK))) {
			ret =
			    vdce_set_rmapparams(&
						(params->vdce_mode_params.
						 postcodec_params.
						 rmap_params),
						vdce_conf_chan, pass_num);
		}
		/* setting blending params and enabling it */
		if (((params->modes_control & BLEND_ENABLE_MASK) ==
		     (BLEND_ENABLE_MASK))) {
			ret =
			    vdce_set_blendparams(&
						 (params->vdce_mode_params.
						  postcodec_params.
						  blend_params),
						 vdce_conf_chan,
						 VDCE_OPERATION_POST_CODECMODE,
						 pass_num);
		}
		vdce_conf_chan->register_config[pass_num].vdce_ctrl |=
		    ((MODE_POSTCODEC << VDCE_MODE_SHIFT) & (VDCE_MODE_MASK));

	} else if (params->vdce_mode == VDCE_OPERATION_TRANS_CODECMODE) {
		/* Configuration of Trans-codec mode */
		/* setting resizing params and enabling it */
		if (((params->modes_control & RSZ_ENABLE_MASK) ==
		     (RSZ_ENABLE_MASK))) {
			ret = vdce_set_rszparams(params, vdce_conf_chan,
						 VDCE_OPERATION_TRANS_CODECMODE,
						 pass_num);
			if (params->vdce_mode_params.transcodec_params.
			    ccv_params.codec_mode_in == VDCE_CODECMODE_MPEG1) {
				vdce_conf_chan->luma_chroma_phased = 1;
			}
		}
		/* setting range-mapping params and enabling it */
		if (((params->modes_control & RMAP_ENABLE_MASK) ==
		     (RMAP_ENABLE_MASK))) {
			ret =
			    vdce_set_rmapparams(&
						(params->vdce_mode_params.
						 transcodec_params.
						 rmap_params),
						vdce_conf_chan, pass_num);
		}
		/* setting blend params and enabling it */
		if (((params->modes_control & BLEND_ENABLE_MASK) ==
		     (BLEND_ENABLE_MASK))) {
			ret =
			    vdce_set_blendparams(&
						 (params->vdce_mode_params.
						  transcodec_params.
						  blend_params),
						 vdce_conf_chan,
						 VDCE_OPERATION_TRANS_CODECMODE,
						 pass_num);
		}
		/* setting ccv params and enabling it */
		if (((params->modes_control & CCV_ENABLE_MASK) ==
		     (CCV_ENABLE_MASK))) {
			ret =
			    vdce_set_ccvparams(&
					       (params->vdce_mode_params.
						transcodec_params.
						ccv_params),
					       vdce_conf_chan,
					       MODE_TRANSCODEC, pass_num);
		}
		vdce_conf_chan->register_config[pass_num].vdce_ctrl |=
		    ((MODE_TRANSCODEC << VDCE_MODE_SHIFT) & (VDCE_MODE_MASK));

	} else if (params->vdce_mode == VDCE_OPERATION_EDGE_PADDING) {
		/* configuration of Edge-padding mode */
		ret = vdce_set_epadparams(&(params->vdce_mode_params.
					    epad_params)
					  , vdce_conf_chan, pass_num);
		vdce_conf_chan->register_config[pass_num].vdce_ctrl |=
		    ((MODE_EPAD << VDCE_MODE_SHIFT) & (VDCE_MODE_MASK));
	} else if (params->vdce_mode == VDCE_OPERATION_RESIZING) {
		/* Configuration of Resizing mode */
		if (params->vdce_mode_params.rsz_params.rsz_mode ==
		    VDCE_MODE_422) {
			memset(&ccv_params, 0, sizeof(vdce_ccv_params_t));
			vdce_conf_chan->num_pass = VDCE_MULTIPASS;
			/* set mode as pre-codec */
			vdce_conf_chan->register_config[pass_num].
			    vdce_ctrl |=
			    ((MODE_PRECODEC << VDCE_MODE_SHIFT) &
			     (VDCE_MODE_MASK));
			ret =
			    vdce_set_ccvparams(&ccv_params, vdce_conf_chan,
					       MODE_PRECODEC, pass_num);
			ret =
			    vdce_set_rszparams(params, vdce_conf_chan,
					       VDCE_OPERATION_RESIZING,
					       pass_num);
			pass_num = 1;
			ret =
			    vdce_set_ccvparams(&ccv_params, vdce_conf_chan,
					       MODE_POSTCODEC, pass_num);

			vdce_conf_chan->register_config[pass_num].
			    vdce_ctrl |=
			    ((MODE_POSTCODEC << VDCE_MODE_SHIFT) &
			     (VDCE_MODE_MASK));
		} else {
			vdce_conf_chan->register_config[pass_num].
			    vdce_ctrl |=
			    ((MODE_TRANSCODEC << VDCE_MODE_SHIFT) &
			     (VDCE_MODE_MASK));
			ret = vdce_set_rszparams(params, vdce_conf_chan,
						 VDCE_OPERATION_RESIZING,
						 pass_num);
			if (params->vdce_mode_params.rsz_params.
			    rsz_op_mode == VDCE_CODECMODE_MPEG1) {
				vdce_conf_chan->luma_chroma_phased = 1;
			}
		}
	} else if (params->vdce_mode == VDCE_OPERATION_CHROMINANCE_CONVERSION) {
		/* Configuration of CCV mode */
		if (params->vdce_mode_params.ccv_params.conversion_type ==
		    VDCE_CCV_MODE_420_422) {
			ret =
			    vdce_set_ccvparams(&
					       (params->vdce_mode_params.
						ccv_params),
					       vdce_conf_chan,
					       MODE_POSTCODEC, pass_num);
		} else if (params->vdce_mode_params.ccv_params.
			   conversion_type == VDCE_CCV_MODE_422_420) {
			ret =
			    vdce_set_ccvparams(&
					       (params->vdce_mode_params.
						ccv_params),
					       vdce_conf_chan,
					       MODE_PRECODEC, pass_num);
		} else {
			ret =
			    vdce_set_ccvparams(&
					       (params->vdce_mode_params.
						ccv_params),
					       vdce_conf_chan,
					       MODE_TRANSCODEC, pass_num);
		}
	} else if (params->vdce_mode == VDCE_OPERATION_RANGE_MAPPING) {
		/* Configuration of Range-mapping */
		ret = vdce_set_rmapparams(&(params->vdce_mode_params.
					    rmap_params), vdce_conf_chan,
					  pass_num);
		vdce_conf_chan->register_config[pass_num].vdce_ctrl |=
		    ((MODE_TRANSCODEC << VDCE_MODE_SHIFT) & (VDCE_MODE_MASK));
	} else if (params->vdce_mode == VDCE_OPERATION_BLENDING) {
		/* Configuration of Blending mode */
		if (params->vdce_mode_params.blend_params.blend_mode ==
		    VDCE_MODE_422) {
			memset(&ccv_params, 0, sizeof(vdce_ccv_params_t));
			/* set mode as pre-codec */
			vdce_conf_chan->num_pass = VDCE_MULTIPASS;
			ret =
			    vdce_set_ccvparams(&ccv_params, vdce_conf_chan,
					       MODE_PRECODEC, pass_num);
			vdce_conf_chan->register_config[pass_num].
			    vdce_ctrl |=
			    ((MODE_PRECODEC << VDCE_MODE_SHIFT) &
			     (VDCE_MODE_MASK));
			pass_num = 1;
			ret =
			    vdce_set_blendparams(&
						 (params->vdce_mode_params.
						  blend_params),
						 vdce_conf_chan,
						 MODE_POSTCODEC, 1);
			ret =
			    vdce_set_ccvparams(&ccv_params, vdce_conf_chan,
					       MODE_POSTCODEC, pass_num);

			vdce_conf_chan->register_config[pass_num].
			    vdce_ctrl |=
			    ((MODE_POSTCODEC << VDCE_MODE_SHIFT) &
			     (VDCE_MODE_MASK));
		} else {
			vdce_conf_chan->register_config[pass_num].
			    vdce_ctrl |=
			    ((MODE_TRANSCODEC << VDCE_MODE_SHIFT) &
			     (VDCE_MODE_MASK));

			ret =
			    vdce_set_blendparams(&
						 (params->vdce_mode_params.
						  blend_params),
						 vdce_conf_chan,
						 MODE_TRANSCODEC, 0);
		}
	} else {
		/* error */
		dev_err(vdce_device, "\n mode not supported ");
		ret = -EINVAL;
	}
	if (pass_num == 0) {
		vdce_set_size_fmt(&(params->common_params), vdce_conf_chan, 0);
	} else {
		vdce_set_size_fmt(&(params->common_params), vdce_conf_chan, 0);
		vdce_set_size_fmt(&(params->common_params), vdce_conf_chan, 1);
	}
	mode = ((vdce_conf_chan->register_config[0].vdce_ctrl
		 & VDCE_MODE_MASK) >> VDCE_MODE_SHIFT);

	if ((vdce_conf_chan->mode_state == VDCE_OPERATION_PRE_CODECMODE) ||
	    ((vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING) &&
	     (vdce_conf_chan->num_pass == VDCE_MULTIPASS)) ||
	    ((vdce_conf_chan->
	      mode_state == VDCE_OPERATION_CHROMINANCE_CONVERSION) &&
	     (vdce_conf_chan->num_pass == VDCE_MULTIPASS)) ||
	    (mode == MODE_PRECODEC) ||
	    (vdce_conf_chan->mode_state == VDCE_OPERATION_EDGE_PADDING)) {
		vdce_conf_chan->image_type_in = VDCE_IMAGE_FMT_422;
	} else {
		vdce_conf_chan->image_type_in = VDCE_IMAGE_FMT_420;
	}
	if ((vdce_conf_chan->mode_state == VDCE_OPERATION_POST_CODECMODE)
	    || ((vdce_conf_chan->mode_state == VDCE_OPERATION_RESIZING)
		&& (vdce_conf_chan->num_pass == VDCE_MULTIPASS))
	    ||
	    ((vdce_conf_chan->mode_state ==
	      VDCE_OPERATION_CHROMINANCE_CONVERSION)
	     && (vdce_conf_chan->num_pass == VDCE_MULTIPASS))
	    ||
	    ((vdce_conf_chan->mode_state == VDCE_OPERATION_BLENDING
	      && vdce_conf_chan->num_pass == VDCE_MULTIPASS)
	     || mode == MODE_POSTCODEC)
	    || (vdce_conf_chan->mode_state == VDCE_OPERATION_EDGE_PADDING)) {
		vdce_conf_chan->image_type_out = VDCE_IMAGE_FMT_422;

	} else {
		vdce_conf_chan->image_type_out = VDCE_IMAGE_FMT_420;
	}
	if (vdce_conf_chan->luma_chroma_phased == 1) {
		if (params->common_params.proc_control ==
		    VDCE_LUMA_CHROMA_ENABLE) {
			vdce_conf_chan->luma_chroma_phased = 1;
			vdce_conf_chan->num_pass = VDCE_MULTIPASS;
		} else {
			vdce_conf_chan->luma_chroma_phased = 0;
			vdce_conf_chan->num_pass = VDCE_SINGLE_PASS;
		}

	}
	if (ret == 0) {
		dev_dbg(vdce_device, "VDCE State configured \n");
		memcpy(&vdce_conf_chan->get_params, params,
		       sizeof(vdce_params_t));
		vdce_conf_chan->status = VDCE_CHAN_PARAMS_INITIALISED;
	}
	dev_dbg(vdce_device, "<fn> vdce_set_params L\n</fn>");
	return ret;
}

/*
 * vdce_get_Params : Function to get the parameters values
 */
int vdce_get_params(vdce_params_t * params, channel_config_t * vdce_conf_chan)
{
	if (vdce_conf_chan->status == VDCE_CHAN_UNINITIALISED) {
		dev_err(vdce_device, "2 State not configured \n");
		return -EINVAL;
	}
	memcpy(params, &vdce_conf_chan->get_params, sizeof(vdce_params_t));
	return 0;
}

/*
 * vdce_get_status : This function gets a status of hardware and channel.
 */
int vdce_get_status(vdce_hw_status_t * hw_status,
		    channel_config_t * vdce_conf_chan)
{
	dev_dbg(vdce_device, "<fn> vdce_Get_status E\n</fn>");
	hw_status->chan_status = vdce_conf_chan->status;
	dev_dbg(vdce_device, "<fn>vdce_Get_status E\n</fn>");
	return 0;
}

/*
 * vdce_get_status : This function used to get default params
*/
int vdce_get_default(vdce_params_t * def_params)
{
	vdce_params_t *vdce_temp_params = NULL;
	if (def_params->vdce_mode == VDCE_OPERATION_PRE_CODECMODE) {
		vdce_temp_params = &precodec_default_params;
	} else if (def_params->vdce_mode == VDCE_OPERATION_POST_CODECMODE) {
		vdce_temp_params = &postcodec_default_params;
	} else if (def_params->vdce_mode == VDCE_OPERATION_TRANS_CODECMODE) {
		vdce_temp_params = &transcodec_default_params;
	} else if (def_params->vdce_mode == VDCE_OPERATION_EDGE_PADDING) {
		vdce_temp_params = &epad_default_params;
	} else if (def_params->vdce_mode == VDCE_OPERATION_RESIZING) {
		vdce_temp_params = &rsz_default_params;
	} else if (def_params->vdce_mode == VDCE_OPERATION_BLENDING) {
		vdce_temp_params = &blend_default_params;
	} else if (def_params->vdce_mode == VDCE_OPERATION_RANGE_MAPPING) {
		vdce_temp_params = &rmap_default_params;
	} else if (def_params->
		   vdce_mode == VDCE_OPERATION_CHROMINANCE_CONVERSION) {
		vdce_temp_params = &ccv_default_params;
	}
	memcpy(def_params, vdce_temp_params, sizeof(vdce_params_t));
	return 0;

}

/*
 * vdce_open : This function creates a channels.
 */
static int vdce_open(struct inode *inode, struct file *filp)
{
	channel_config_t *vdce_conf_chan = NULL;

	dev_dbg(vdce_device, "<fn> vdce_open E\n</fn>");
	/* if usage counter is greater than maximum supported channels
	   return error */
	/*if (device_config.module_usage_count >= MAX_CHANNELS) {
		dev_err(vdce_device,
			"\n modules usage count	is greater than	supported ");
		return -EBUSY;
	}*/
	if (device_config.module_usage_count == 0) {
		device_config.sem_isr.done = 0;
	}
	/* allocate memory for a new configuration */
	vdce_conf_chan = kmalloc(sizeof(channel_config_t), GFP_KERNEL);

	if (vdce_conf_chan == NULL) {
		dev_err(vdce_device,
			"cannot allocate memory ro channel config\n");
		return -ENOMEM;
	}
	dev_dbg(vdce_device,
		"Malloc	Done for channel configuration structure\n");
	if (filp->f_flags == (O_NONBLOCK | O_RDWR)) {
		vdce_conf_chan->channel_mode = VDCE_MODE_NON_BLOCKING;
	}
	/* zeroing register config */
	memset(vdce_conf_chan, 0, sizeof(channel_config_t));
	if (filp->f_flags == (O_NONBLOCK | O_RDWR)) {
		vdce_conf_chan->channel_mode = VDCE_MODE_NON_BLOCKING;
	}
	vdce_conf_chan->status = VDCE_CHAN_UNINITIALISED;

	/* increment usage counter */
	/* Lock the global variable and increment the counter */
	device_config.module_usage_count++;

	/*STATE_NOT_CONFIGURED and priority to zero */
	vdce_conf_chan->mode_state = VDCE_CHAN_UNINITIALISED;

	dev_dbg(vdce_device, "Initializing of channel done \n");

	/* taking the configuartion     structure in private data */
	filp->private_data = vdce_conf_chan;

	dev_dbg(vdce_device, "<fn> vdce_open L\n</fn>");

	return 0;
}

/*
 * vdce_release : The Function is used to release the number of
 * resources occupied by the channel
*/
static int vdce_release(struct inode *inode, struct file *filp)
{

	/* get the configuratin of this channel from private_date member of
	   file */
	channel_config_t *vdce_conf_chan =
	    (channel_config_t *) filp->private_data;

	dev_dbg(vdce_device, "<fn> vdce_release E\n</fn>");

	/* it will free all the input and output buffers */
	free_buff(vdce_conf_chan);
	/* decrements the module usage count; */
	/* lock the global variable and decrement variable */
	device_config.module_usage_count--;
	kfree(vdce_conf_chan);
	dev_dbg(vdce_device, "<fn> vdce_release L\n</fn>");
	return 0;
}

/*
 * vdce_mmap : Function to map device memory into user space
 */
static int vdce_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* get the configuratin of this channel from private_date
	   member of file */
	/* for looping purpuse */
	int buffercounter = 0;
	int i;
	/* for checking purpose */
	int flag = 0;
	/* hold number of input and output buffer allocated */
	int buffer_offset = 0;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;

	channel_config_t *vdce_conf_chan =
	    (channel_config_t *) filp->private_data;

	dev_dbg(vdce_device, "<fn> vdce_mmap E\n</fn>");

	for (i = 0; i < 3; i++) {
		/*find the input address which  is to be mapped */
		for (buffercounter = 0; buffercounter <
		     vdce_conf_chan->vdce_buffer[i].num_allocated;
		     buffercounter++) {
			buffer_offset =
			    virt_to_phys(vdce_conf_chan->vdce_buffer[i].
					 buffers[buffercounter]);
			if (buffer_offset == offset) {
				flag = 1;
				break;
			}
		}
		if (flag == 1) {
			break;
		}

	}
	/* the address to be mapped is not found so return error */
	if (flag == 0)
		return -EAGAIN;

	dev_dbg(vdce_device, "The	address	mapped via mmap");
	/* map the address from user space to kernel space */
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}
	dev_dbg(vdce_device, "<fn> vdce_mmap L\n</fn>");

	return 0;
}

/*
 * vdce_ioctl : This function will process IOCTL commands sent by
 * the application and control .
 */
static int vdce_ioctl(struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	vdce_address_start_t start;
	vdce_params_t params;
	vdce_buffer_t buffer;
	vdce_reqbufs_t reqbuff;
	/*get the configuratin of this channel from
	   private_date member of file */
	channel_config_t *vdce_conf_chan =
	    (channel_config_t *) file->private_data;

	dev_dbg(vdce_device, " vdce_ioctl E\n");
	/* before decoding check for correctness of cmd */
	if (_IOC_TYPE(cmd) != VDCE_IOC_BASE) {
		dev_err(vdce_device, "Bad command Value \n");
		return -1;
	}
	if (_IOC_NR(cmd) > VDCE_IOC_MAXNR) {
		dev_err(vdce_device, "Bad Command Value\n");
		return -1;
	}
	/*veryfying access permission of commands */
	if (_IOC_DIR(cmd) & _IOC_READ) {

		ret = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {

		ret = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	}
	if (ret) {
		dev_err(vdce_device, "access denied\n");
		return -1;	/*error in access */
	}

	/* switch according value of cmd */
	switch (cmd) {
		/*this ioctl is used to request frame buffers to be
		   allocated by the ge module. The allocated buffers
		   are channel  specific and can be     addressed
		   by indexing */
	case VDCE_REQBUF:
		/* function to allocate the memory to input
		   or output buffer. */
		if (copy_from_user(&reqbuff, (vdce_reqbufs_t *) arg,
				   sizeof(vdce_reqbufs_t))) {
			ret = -EFAULT;
			break;
		}

		ret = malloc_buff(&reqbuff, vdce_conf_chan);

		if (copy_to_user((vdce_reqbufs_t *) arg,
				 &reqbuff, sizeof(vdce_reqbufs_t)))
			ret = -EFAULT;
		break;
		/*this ioctl is used to query the physical address of a
		   particular frame buffer. */
	case VDCE_QUERYBUF:
		if (copy_from_user(&buffer, (vdce_buffer_t *) arg,
				   sizeof(vdce_buffer_t))) {
			ret = -EFAULT;
			break;
		}
		ret = get_buf_address(&buffer, vdce_conf_chan);

		if (copy_to_user((vdce_buffer_t *) arg,
				 &buffer, sizeof(vdce_buffer_t)))
			ret = -EFAULT;
		break;

		/* this ioctl is used to set the parameters
		   of the GE hardware, parameters. */
	case VDCE_SET_PARAMS:
		/* function to set the hardware configuration */
		if (copy_from_user(&params, (vdce_params_t *) arg,
				   sizeof(vdce_params_t))) {
			ret = -EFAULT;
			break;
		}
		ret = vdce_check_common_params(&params);
		if (0 == ret) {
			ret = vdce_set_params(&params, vdce_conf_chan);
		} else {
			dev_err(vdce_device, "\n VDCE wrong parameters \n");
		}
		break;
		/*this ioctl is used to get the GE hardware settings
		   associated with the current logical channel represented
		   by fd. */
	case VDCE_GET_PARAMS:
		/* function to get the hardware configuration */
		ret = vdce_get_params((vdce_params_t *) arg, vdce_conf_chan);
		break;
	case VDCE_GET_DEFAULT:
		/* this ioctl is used to get the default parameters
		   of the ge hardware */
		ret = vdce_get_default((vdce_params_t *) arg);
		break;

	case VDCE_START:
		if (copy_from_user(&start, (vdce_address_start_t *) arg,
				   sizeof(vdce_address_start_t))) {
			ret = -EFAULT;
			break;
		}

		ret = vdce_start(&start, vdce_conf_chan);
		break;
	default:
		dev_dbg(vdce_device, "VDCE_ioctl: Invalid Command Value");
		ret = -EINVAL;
	}

	dev_dbg(vdce_device, " vdce_ioctl L\n");

	return ret;
}
static struct file_operations vdce_fops = {
	.owner = THIS_MODULE,
	.open = vdce_open,
	.release = vdce_release,
	.mmap = vdce_mmap,
	.ioctl = vdce_ioctl,
};

/*
 * vdce_isr : Function to register the ge character device driver
 */
irqreturn_t vdce_isr(int irq, void *dev_id)
{
        if (((vdce_current_chan->num_pass == VDCE_MULTIPASS) &&
             (vdce_current_chan->vdce_complete == VDCE_PASS1_STARTED))) {
                vdce_current_chan->vdce_complete = VDCE_PASS2_STARTED;

                tasklet_schedule(&short_tasklet);

        } else {
                vdce_current_chan->vdce_complete = VDCE_COMPLETED;

        }

	if(vdce_current_chan->vdce_complete == VDCE_COMPLETED) {
		complete(&(device_config.sem_isr));
	}
	return IRQ_HANDLED;
}

/*
 * process_nonblock : Function to process_nonblocking call
 */
void process_bottomhalf(unsigned long ret)
{
	/* codec mode used */
	channel_config_t *vdce_conf_chan =
		vdce_current_chan;
	int pass_num;
	if (vdce_conf_chan->vdce_complete == VDCE_PASS2_STARTED) {
		/* set 2 pass register configuration */
		pass_num = 1;
	} else {
		vdce_conf_chan->vdce_complete = VDCE_PASS1_STARTED;
		/* set 1 pass register configuration */
		pass_num = 0;
	}
	/*function call to set up the hardware */
	vdce_hw_setup(&vdce_conf_chan->register_config[pass_num]);

	/*function call to enable ge hardware */
	ret = vdce_enable(&vdce_conf_chan->register_config[pass_num]);

	dev_dbg(vdce_device, "<fn> process_nonblock L</fn>\n");
}
static void vdce_platform_release(struct device *device)
{
	/* this is called when the reference count goes to zero */
}
static int __init vdce_probe(struct device *device)
{
	vdce_device = device;
	return 0;
}
static int vdce_remove(struct device *device)
{
	return 0;
}
static struct class *vdce_class = NULL;

static struct platform_device graphics_device = {
	.name = DRIVERNAME,
	.id = 2,
	.dev = {
	   .release = vdce_platform_release,
	}
};
static struct device_driver vdce_driver = {
	.name = DRIVERNAME,
	.bus = &platform_bus_type,
	.probe = vdce_probe,
	.remove = vdce_remove,
};

/*
 * vdce_init : Function to register ge character driver
 */
static int __init vdce_init(void)
{
	int result;
	int adr;
	int size;
	struct device *temp =NULL;

	device_config.module_usage_count = 0;

	/* register the driver in the kernel */
	result = alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME);
	if (result < 0) {
		printk(KERN_ERR "\nDaVincige: Module intialization failed.\
				could not register character device");
		return -ENODEV;
	}
	/* initialize of character device */
	cdev_init(&c_dev, &vdce_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &vdce_fops;
	/* addding character device */
	result = cdev_add(&c_dev, dev, 1);
	if (result) {
		printk(KERN_ERR "NOtICE \nDaVincige:Error %d adding DavinciVDCE\
				..error no:", result);
		result = -EINVAL;
		goto label1;
	}
	/* registeration of character device */
	register_chrdev(MAJOR(dev), DRIVER_NAME, &vdce_fops);

	/* register driver as a platform driver */
	if (driver_register(&vdce_driver) != 0) {
		result = -EINVAL;
		goto label2;
	}
	/* register the drive as a platform device */
	if (platform_device_register(&graphics_device) != 0) {
		result = -EINVAL;
		goto label3;
	}
	vdce_class = class_create(THIS_MODULE, DRIVERNAME);
	if (IS_ERR(vdce_class)) {
		result = -EIO;
		goto label4;
	}

	temp = device_create(vdce_class, NULL, dev, NULL, DRIVERNAME);
	if (IS_ERR(temp)) {
		result = -EIO;
        	goto label5;
	}

	init_completion(&(device_config.sem_isr));
	init_completion(&(device_config.edma_sem));
	init_completion(&(device_config.device_access));

	device_config.sem_isr.done = 0;
	device_config.edma_sem.done = 0;
	device_config.device_access.done = 1;

	/* initialize the device mutex */
	device_config.irqlock = SPIN_LOCK_UNLOCKED;

	/* set up the Interrupt handler for     ge interrupt */
	result =
		request_irq(VDCE_INTERRUPT, vdce_isr, 0,
				"DavinciHD_VDCE", (void *)NULL);
	if (result < 0) {
		printk(KERN_ERR "Cannot initialize IRQ \n");
		result = -EINVAL;
		goto label6;

	}

	device_config.inter_size = inter_bufsize;
	if (device_config.inter_size > 0) {
		device_config.inter_buffer =
			(void *)(__get_free_pages(GFP_KERNEL | GFP_DMA,
						  get_order((device_config.
							  inter_size))));
		if (!(device_config.inter_buffer)) {
			goto label6;
		}

		adr = (unsigned int)device_config.inter_buffer;
		size = PAGE_SIZE << (get_order((device_config.inter_size)));
		while (size > 0) {
			/* make  sure the frame buffers
			   are never swapped out of memory */
			SetPageReserved(virt_to_page(adr));
			adr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	}

	result = vdce_enable_int();
	if (result < 0) {
		printk(KERN_ERR "Cannot init register \n");
		result = -EINVAL;
		goto label6;

	}

	/* Allocate Any EDMA Channel*/
	dma_ch = edma_alloc_channel(EDMA_CHANNEL_ANY, callback1, NULL,
						EVENTQ_DEFAULT);
	if (0 > dma_ch) {
		printk(KERN_ERR "Cannot Allocate Channel:%d\n", dma_ch);
		goto label6;
	}

	return 0;

label6:
	device_destroy(vdce_class, dev);
label5:
	class_destroy(vdce_class);
label4:
	platform_device_unregister(&graphics_device);
label3:
	driver_unregister(&vdce_driver);
label2:
	cdev_del(&c_dev);
	unregister_chrdev(MAJOR(dev), DRIVER_NAME);
label1:
	unregister_chrdev_region(dev, 1);

	return result;
}

/*
 * vdce_cleanup : Function is called by the kernel. It unregister
 * the device.
 */
void __exit vdce_cleanup(void)
{
	device_destroy(vdce_class, dev);
	if (device_config.inter_size > 0) {
		vdce_free_pages((int)device_config.inter_buffer,
				((device_config.inter_size)));
	}
	/* disable interrupt */
	free_irq(VDCE_INTERRUPT, (void *)NULL);
	/* destroy simple class */
	class_destroy(vdce_class);
	/* remove platform device */
	platform_device_unregister(&graphics_device);
	/* remove platform driver */
	driver_unregister(&vdce_driver);
	/* unregistering the driver from the kernel */
	unregister_chrdev(MAJOR(dev), DRIVER_NAME);

	cdev_del(&c_dev);

	edma_free_channel(dma_ch);

	unregister_chrdev_region(dev, 1);

}

module_init(vdce_init)
    module_exit(vdce_cleanup)
