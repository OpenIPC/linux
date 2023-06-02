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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <asm-generic/current.h>

#include <media/davinci/vpss.h>
#include <media/davinci/imp_hw_if.h>

#include <mach/cputype.h>

static int serializer_initialized;
struct imp_serializer imp_serializer_info;
static struct imp_hw_interface *imp_hw_if;

int imp_common_mmap(struct file *filp,
		    struct vm_area_struct *vma,
		    struct imp_logical_channel *channel)
{
	int i, flag = 0, shift;
	/* get the page offset */
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	shift = PAGE_SHIFT;
	for (i = 0; i < channel->in_numbufs; i++) {
		if (channel->in_bufs[i]->offset == offset) {
			flag = 1;
			break;
		}
	}

	/* page offset passed in mmap should one from output buffers */
	if (flag == 0) {
		for (i = 0; i < channel->out_numbuf1s; i++) {
			if (channel->out_buf1s[i]->offset == offset) {
				flag = 1;
				break;
			}
		}
	}

	if (flag == 0) {
		for (i = 0; i < channel->out_numbuf2s; i++) {
			if (channel->out_buf2s[i]->offset == offset) {
				flag = 1;
				break;
			}
		}
	}
	if (flag) {
		/* map buffers address space from kernel space to user space */
		if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				    vma->vm_end - vma->vm_start,
				    vma->vm_page_prot))
			return -EAGAIN;
	} else {
		/* No matching buffer */
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL(imp_common_mmap);

/* inline function to free reserver pages  */
static inline void imp_common_free_pages(unsigned long addr,
					 unsigned long bufsize)
{
	unsigned long size, ad = addr;
	size = PAGE_SIZE << (get_order(bufsize));
	if (!addr)
		return;
	while (size > 0) {
		ClearPageReserved(virt_to_page(addr));
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	free_pages(ad, get_order(bufsize));
}

/* This function is used to free memory allocated to buffers */
int imp_common_free_buffers(struct device *dev,
			    struct imp_logical_channel *channel)
{
	int i;
	unsigned long adr;
	if (!channel) {
		dev_err(dev, "\nfree_buffers:error in argument");
		return -EINVAL;
	}
	/* free memory allocated to in buffers */
	for (i = 0; i < channel->in_numbufs; i++) {
		if (channel->in_bufs[i]) {
			adr = channel->in_bufs[i]->offset;
			if (adr)
				imp_common_free_pages((unsigned long)
						      phys_to_virt(adr),
						      channel->in_bufs[i]->
						      size);

			kfree(channel->in_bufs[i]);

			channel->in_bufs[i] = NULL;
		}
	}
	channel->in_numbufs = 0;
	/* free memory allocated to out buffers */
	for (i = 0; i < channel->out_numbuf1s; i++) {
		if (channel->out_buf1s[i]) {
			adr = channel->out_buf1s[i]->offset;
			if (adr)
				imp_common_free_pages((unsigned long)
						      phys_to_virt(adr),
						      channel->out_buf1s[i]->
						      size);

			kfree(channel->out_buf1s[i]);

			channel->out_buf1s[i] = NULL;
		}
	}

	channel->out_numbuf1s = 0;

	/* free memory allocated to out buffers */
	for (i = 0; i < channel->out_numbuf2s; i++) {
		if (channel->out_buf2s[i]) {
			adr = channel->out_buf2s[i]->offset;
			if (adr)
				imp_common_free_pages((unsigned long)
						      phys_to_virt(adr),
						      channel->out_buf2s[i]->
						      size);

			kfree(channel->out_buf2s[i]);

			channel->out_buf2s[i] = NULL;
		}
	}

	channel->out_numbuf2s = 0;
	return 0;
}
EXPORT_SYMBOL(imp_common_free_buffers);

/*
 * This function will query the buffer's physical address
 * whose index is passed in ipipe_buffer.
 * It will store that address in ipipe_buffer.
 */
int imp_common_query_buffer(struct device *dev,
			    struct imp_logical_channel *channel,
			    struct imp_buffer *buffer)
{

	if (!buffer || !channel) {
		dev_err(dev, "query_buffer: error in argument\n");
		return -EINVAL;
	}

	if (buffer->index < 0) {
		dev_err(dev, "query_buffer: invalid index %d\n", buffer->index);
		return -EINVAL;
	}

	if ((buffer->buf_type != IMP_BUF_IN)
	    && (buffer->buf_type != IMP_BUF_OUT1)
	    && (buffer->buf_type != IMP_BUF_OUT2)) {
		dev_err(dev, "request_buffer: invalid buffer type\n");
		return -EINVAL;
	}
	/* if buf_type is input buffer then get offset of input buffer */
	if (buffer->buf_type == IMP_BUF_IN) {
		/* error checking for wrong index number */
		if (buffer->index >= channel->in_numbufs) {
			dev_err(dev, "query_buffer: invalid index");
			return -EINVAL;
		}

		/* get the offset and size of the buffer and store
		   it in buffer */
		buffer->offset = channel->in_bufs[buffer->index]->offset;
		buffer->size = channel->in_bufs[buffer->index]->size;
	}
	/* if buf_type is output buffer then get offset of output buffer */
	else if (buffer->buf_type == IMP_BUF_OUT1) {
		/* error checking for wrong index number */
		if (buffer->index >= channel->out_numbuf1s) {
			dev_err(dev, "query_buffer: invalid index\n");
			return -EINVAL;
		}
		/* get the offset and size of the buffer and store
		   it in buffer */
		buffer->offset = channel->out_buf1s[buffer->index]->offset;
		buffer->size = channel->out_buf1s[buffer->index]->size;
	}
	/* if buf_type is output buffer then get offset of output buffer */
	else if (buffer->buf_type == IMP_BUF_OUT2) {
		/* error checking for wrong index number */
		if (buffer->index >= channel->out_numbuf2s) {
			dev_err(dev, "query_buffer: invalid index\n");
			return -EINVAL;
		}
		/* get the offset and size of the buffer and store
		   it in buffer */
		buffer->offset = channel->out_buf2s[buffer->index]->offset;
		buffer->size = channel->out_buf2s[buffer->index]->size;
	} else {
		dev_err(dev, "query_buffer: invalid buffer type\n");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(imp_common_query_buffer);

int imp_common_request_buffer(struct device *dev,
			      struct imp_logical_channel *channel,
			      struct imp_reqbufs *reqbufs)
{
	struct imp_buffer *buffer = NULL;
	int count = 0;
	unsigned long adr;
	u32 size;

	if (!reqbufs || !channel) {
		dev_err(dev, "request_buffer: error in argument\n");
		return -EINVAL;
	}

	/* if number of buffers requested is more then support return error */
	if (reqbufs->count > MAX_BUFFERS) {
		dev_err(dev, "request_buffer: invalid buffer count\n");
		return -EINVAL;
	}

	if ((reqbufs->buf_type != IMP_BUF_IN)
	    && (reqbufs->buf_type != IMP_BUF_OUT1)
	    && (reqbufs->buf_type != IMP_BUF_OUT2)) {
		dev_err(dev, "request_buffer: invalid buffer type %d\n",
			reqbufs->buf_type);
		return -EINVAL;
	}
	if (reqbufs->count < 0) {
		dev_err(dev, "request_buffer: invalid buffer count %d\n",
			reqbufs->count);
		return -EINVAL;
	}
	/* if buf_type is input then allocate buffers for input */
	if (reqbufs->buf_type == IMP_BUF_IN) {
		/*if buffer count is zero, free all the buffers */
		if (reqbufs->count == 0) {
			/* free all the buffers */
			for (count = 0; count < channel->in_numbufs; count++) {
				/* free memory allocate for the image */
				if (channel->in_bufs[count]) {
					adr =
					    (unsigned long)channel->
					    in_bufs[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
							long)
							phys_to_virt
							(adr),
							channel->
							in_bufs
							[count]->
							size);

					/* free the memory allocated
					   to ipipe_buffer */
					kfree(channel->in_bufs[count]);

					channel->in_bufs[count] = NULL;
				}
			}
			channel->in_numbufs = 0;
			return 0;
		}

		/* free the extra buffers */
		if (channel->in_numbufs > reqbufs->count &&
		    reqbufs->size == channel->in_bufs[0]->size) {
			for (count = reqbufs->count;
			     count < channel->in_numbufs; count++) {
				/* free memory allocate for the image */
				if (channel->in_bufs[count]) {
					adr = channel->in_bufs[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								in_bufs
								[count]->
								size);

					/* free the memory allocated
					   to ipipe_buffer */
					kfree(channel->in_bufs[count]);

					channel->in_bufs[count] = NULL;
				}
			}
			channel->in_numbufs = reqbufs->count;
			return 0;
		}
		/* if size requested is different from already allocated,
		   free memory of all already allocated buffers */
		if (channel->in_numbufs) {
			if (reqbufs->size != channel->in_bufs[0]->size) {
				for (count = 0;
				     count < channel->in_numbufs; count++) {
					if (channel->in_bufs[count]) {
						adr =
						    channel->
						    in_bufs[count]->offset;
						if (adr)
							imp_common_free_pages(
							(unsigned long)
							phys_to_virt(adr),
							channel->in_bufs
							[count]->size);

						kfree(channel->in_bufs[count]);

						channel->in_bufs[count] = NULL;
					}
				}
				channel->in_numbufs = 0;
			}
		}

		/* allocate the buffer */
		for (count = channel->in_numbufs; count < reqbufs->count;
		     count++) {
			/* Allocate memory for struct ipipe_buffer */
			buffer = kmalloc(sizeof(struct imp_buffer), GFP_KERNEL);

			/* if memory allocation fails then return error */
			if (!buffer) {
				/* free all the buffers */
				while (--count >= channel->in_numbufs) {
					adr = channel->in_bufs[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								in_bufs
								[count]->
								size);
					kfree(channel->in_bufs[count]);
					channel->in_bufs[count] = NULL;
				}
				dev_err(dev,
					"1.request_buffer:not enough memory\n");
				return -ENOMEM;
			}

			/* assign buffer's address in configuration */
			channel->in_bufs[count] = buffer;

			/* set buffers index and buf_type,size parameters */
			buffer->index = count;
			buffer->buf_type = IMP_BUF_IN;
			buffer->size = reqbufs->size;
			/* allocate memory for buffer of size passed
			   in reqbufs */
			buffer->offset =
			    (unsigned long)__get_free_pages(GFP_KERNEL |
							    GFP_DMA,
							    get_order
							    (reqbufs->size));

			/* if memory allocation fails, return error */
			if (!(buffer->offset)) {
				/* free all the buffer's space */
				kfree(buffer);
				channel->in_bufs[count] = NULL;
				while (--count >= channel->in_numbufs) {
					adr = channel->in_bufs[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								in_bufs
								[count]->
								size);
					kfree(channel->in_bufs[count]);
					channel->in_bufs[count] = NULL;
				}
				dev_err(dev,
					"2.request_buffer:not enough memory\n");

				return -ENOMEM;
			}

			adr = (unsigned long)buffer->offset;
			size = PAGE_SIZE << (get_order(reqbufs->size));
			while (size > 0) {
				/* make sure the frame buffers
				   are never swapped out of memory */
				SetPageReserved(virt_to_page(adr));
				adr += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			/* convert vertual address to physical */
			buffer->offset = (unsigned long)
			    virt_to_phys((void *)(buffer->offset));
		}
		channel->in_numbufs = reqbufs->count;
	}
	/* if buf_type is output then allocate buffers for output */
	else if (reqbufs->buf_type == IMP_BUF_OUT1) {
		if (reqbufs->count == 0) {
			/* free all the buffers */
			for (count = 0; count < channel->out_numbuf1s;
				count++) {
				/* free memory allocate for the image */
				if (channel->out_buf1s[count]) {
					adr = channel->out_buf1s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf1s
								[count]->
								size);

					/* free the memory allocated to
					   ipipe_buffer */
					kfree(channel->out_buf1s[count]);

					channel->out_buf1s[count] = NULL;
				}
			}
			channel->out_numbuf1s = 0;

			return 0;
		}
		/* free the buffers */
		if (channel->out_numbuf1s > reqbufs->count &&
		    reqbufs->size == channel->out_buf1s[0]->size) {
			for (count = reqbufs->count;
			     count < channel->out_numbuf1s; count++) {
				/* free memory allocate for the image */
				if (channel->out_buf1s[count]) {
					adr = channel->out_buf1s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf1s
								[count]->
								size);

					/* free the memory allocated to
					   ipipe_buffer */
					kfree(channel->out_buf1s[count]);

					channel->out_buf1s[count] = NULL;
				}
			}
			channel->out_numbuf1s = reqbufs->count;

			return 0;
		}
		/* if size requested is different from already allocated,
		   free memory of all already allocated buffers */
		if (channel->out_numbuf1s) {
			if (reqbufs->size != channel->out_buf1s[0]->size) {
				for (count = 0;
				     count < channel->out_numbuf1s; count++) {
					if (channel->out_buf1s[count]) {
						adr =
						    channel->
						    out_buf1s[count]->offset;

						if (adr)
							imp_common_free_pages(
								(unsigned long)
								phys_to_virt
								(adr),
								channel->
								out_buf1s
								[count]->
								size);

						kfree(channel->
						      out_buf1s[count]);

						channel->out_buf1s[count] =
						    NULL;
					}
				}
				channel->out_numbuf1s = 0;
			}
		}

		/* allocate the buffer */
		for (count = channel->out_numbuf1s;
		     count < reqbufs->count; count++) {
			/* Allocate memory for struct ipipe_buffer */
			buffer = kmalloc(sizeof(struct imp_buffer), GFP_KERNEL);

			/* if memory allocation fails then return error */
			if (!buffer) {
				/* free all the buffers */
				while (--count >= channel->out_numbuf1s) {
					adr = channel->out_buf1s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf1s
								[count]->
								size);
					kfree(channel->out_buf1s[count]);
					channel->out_buf1s[count] = NULL;
				}

				dev_err(dev,
					"3.request_buffer:not enough memory\n");

				return -ENOMEM;
			}

			/* assign buffer's address out configuration */
			channel->out_buf1s[count] = buffer;

			/* set buffers outdex and buf_type,size parameters */
			buffer->index = count;
			buffer->buf_type = IMP_BUF_OUT1;
			buffer->size = reqbufs->size;
			/* allocate memory for buffer of size passed
			   in reqbufs */
			buffer->offset =
			    (unsigned long)__get_free_pages(GFP_KERNEL |
							    GFP_DMA,
							    get_order
							    (reqbufs->size));

			/* if memory allocation fails, return error */
			if (!(buffer->offset)) {
				/* free all the buffer's space */
				kfree(buffer);
				channel->out_buf1s[count] = NULL;
				while (--count >= channel->out_numbuf1s) {
					adr = channel->out_buf1s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf1s
								[count]->
								size);
					kfree(channel->out_buf1s[count]);
					channel->out_buf1s[count] = NULL;
				}
				dev_err(dev,
					"4.request_buffer:not enough memory\n");

				return -ENOMEM;
			}

			adr = (unsigned long)buffer->offset;
			size = PAGE_SIZE << (get_order(reqbufs->size));
			while (size > 0) {
				/* make sure the frame buffers
				   are never swapped out of memory */
				SetPageReserved(virt_to_page(adr));
				adr += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			/* convert vertual address to physical */
			buffer->offset = (unsigned long)
			    virt_to_phys((void *)(buffer->offset));
		}
		channel->out_numbuf1s = reqbufs->count;

	} else if (reqbufs->buf_type == IMP_BUF_OUT2) {
		if (reqbufs->count == 0) {
			/* free all the buffers */
			for (count = 0; count < channel->out_numbuf2s;
				 count++) {
				/* free memory allocate for the image */
				if (channel->out_buf2s[count]) {
					adr = channel->out_buf2s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf2s
								[count]->
								size);

					/* free the memory allocated to
					   ipipe_buffer */
					kfree(channel->out_buf2s[count]);

					channel->out_buf2s[count] = NULL;
				}
			}
			channel->out_numbuf2s = 0;

			return 0;
		}
		/* free the buffers */
		if (channel->out_numbuf2s > reqbufs->count &&
		    reqbufs->size == channel->out_buf2s[0]->size) {
			for (count = reqbufs->count;
			     count < channel->out_numbuf2s; count++) {
				/* free memory allocate for the image */
				if (channel->out_buf2s[count]) {
					adr = channel->out_buf2s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf2s
								[count]->
								size);

					/* free the memory allocated to
					   ipipe_buffer */
					kfree(channel->out_buf2s[count]);

					channel->out_buf2s[count] = NULL;
				}
			}
			channel->out_numbuf2s = reqbufs->count;

			return 0;
		}
		/* if size requested is different from already allocated,
		   free memory of all already allocated buffers */
		if (channel->out_numbuf2s) {
			if (reqbufs->size != channel->out_buf2s[0]->size) {
				for (count = 0;
				     count < channel->out_numbuf2s; count++) {
					if (channel->out_buf2s[count]) {
						adr =
						    channel->
						    out_buf2s[count]->offset;

						if (adr)
							imp_common_free_pages(
								(unsigned long)
								phys_to_virt
								(adr),
								channel->
								out_buf2s
								[count]->
								size);

						kfree(channel->
						      out_buf2s[count]);

						channel->out_buf2s[count] =
						    NULL;
					}
				}
				channel->out_numbuf2s = 0;
			}
		}

		/* allocate the buffer */
		for (count = channel->out_numbuf2s;
		     count < reqbufs->count; count++) {
			/* Allocate memory for struct ipipe_buffer */
			buffer = kmalloc(sizeof(struct imp_buffer), GFP_KERNEL);

			/* if memory allocation fails then return error */
			if (!buffer) {
				/* free all the buffers */
				while (--count >= channel->out_numbuf2s) {
					adr = channel->out_buf2s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
							long)
							phys_to_virt
							(adr),
							channel->
							out_buf2s
							[count]->
							size);
					kfree(channel->out_buf2s[count]);
					channel->out_buf2s[count] = NULL;
				}

				dev_err(dev,
					"5.request_buffer:not enough memory\n");

				return -ENOMEM;
			}

			/* assign buffer's address out configuration */
			channel->out_buf2s[count] = buffer;

			/* set buffers outdex and buf_type,size parameters */
			buffer->index = count;
			buffer->buf_type = IMP_BUF_OUT2;
			buffer->size = reqbufs->size;
			/* allocate memory for buffer of size passed
			   in reqbufs */
			buffer->offset =
			    (unsigned long)__get_free_pages(GFP_KERNEL |
							    GFP_DMA,
							    get_order
							    (reqbufs->size));

			/* if memory allocation fails, return error */
			if (!(buffer->offset)) {
				/* free all the buffer's space */
				kfree(buffer);
				channel->out_buf2s[count] = NULL;
				while (--count >= channel->out_numbuf2s) {
					adr = channel->out_buf2s[count]->offset;
					if (adr)
						imp_common_free_pages((unsigned
								long)
								phys_to_virt
								(adr),
								channel->
								out_buf2s
								[count]->
								size);
					kfree(channel->out_buf2s[count]);
					channel->out_buf2s[count] = NULL;
				}
				dev_err(dev,
					"6.request_buffer:not enough memory\n");

				return -ENOMEM;
			}

			adr = (unsigned long)buffer->offset;
			size = PAGE_SIZE << (get_order(reqbufs->size));
			while (size > 0) {
				/* make sure the frame buffers
				   are never swapped out of memory */
				SetPageReserved(virt_to_page(adr));
				adr += PAGE_SIZE;
				size -= PAGE_SIZE;
			}
			/* convert vertual address to physical */
			buffer->offset = (unsigned long)
			    virt_to_phys((void *)(buffer->offset));
		}
		channel->out_numbuf2s = reqbufs->count;

	} else {
		dev_err(dev, "request_buffer: invalid buffer type\n");

		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(imp_common_request_buffer);

static irqreturn_t imp_common_isr(int irq, void *device_id)
{
	u32 val = vpss_dma_complete_interrupt();

	if (val == 0 || val == 2)
		complete(&(imp_serializer_info.sem_isr));
	return IRQ_HANDLED;
}

int imp_set_preview_config(struct device *dev,
			   struct imp_logical_channel *channel,
			   struct prev_channel_config *chan_config)
{
	int ret = 0, len = 0;

	if (channel->mode == IMP_MODE_INVALID) {
		dev_err(dev, "Channel mode is not set. \n");
		return -EINVAL;
	}

	if (channel->mode != chan_config->oper_mode) {
		dev_err(dev,
			"mode mis-match, chan mode = %d, config mode = %d\n",
			channel->mode, chan_config->oper_mode);
		return -EINVAL;
	}

	if (channel->config_state == STATE_NOT_CONFIGURED) {
		/* for preview, always use the shared structure */
		channel->config = imp_hw_if->alloc_config_block(dev, 1);
		/* allocate buffer for holding user configuration */
		channel->user_config = imp_hw_if->alloc_user_config_block(dev,
								IMP_PREVIEWER,
								channel->
								mode,
								&len);
		if (ISNULL(channel->user_config)) {
			dev_err(dev,
				"memory allocate failed for user config\n");
			return -EFAULT;
		}
		channel->user_config_size = len;
	}

	if (ISNULL(chan_config->config)) {
		/* put defaults for user configuration */
		imp_hw_if->set_user_config_defaults(dev,
						    IMP_PREVIEWER,
						    chan_config->oper_mode,
						    channel->user_config);
		dev_dbg(dev, "imp_set_preview_config.. default \n");
	} else {
		dev_dbg(dev, "imp_set_preview_config.. user config\n");
		if (copy_from_user(channel->user_config,
				   chan_config->config,
				   channel->user_config_size)) {
			dev_err(dev, "Copy from user to kernel failed\n");
			return -EFAULT;
		}
	}

	/* Update the user configuration in the hw config block */
	ret = imp_hw_if->set_preview_config(dev,
					    chan_config->oper_mode,
					    channel->user_config,
					    channel->config);

	if (ret < 0)
		dev_err(dev, "set preview config failed\n");

	channel->config_state = STATE_CONFIGURED;
	return ret;
}
EXPORT_SYMBOL(imp_set_preview_config);

int imp_set_resizer_config(struct device *dev,
			   struct imp_logical_channel *channel,
			   struct rsz_channel_config *chan_config)
{
	int ret = 0, len;

	dev_dbg(dev, "imp_set_resizer_config. len = %d\n", chan_config->len);
	if (channel->mode == IMP_MODE_INVALID) {
		dev_err(dev, "Channel mode is not set. \n");
		return -EINVAL;
	}

	if (channel->mode != chan_config->oper_mode) {
		dev_err(dev,
			"mode mis-match, chan mode = %d, config mode = %d\n",
			channel->mode, chan_config->oper_mode);
		return -EINVAL;
	}

	if ((chan_config->oper_mode == IMP_MODE_CONTINUOUS) &&
	    (!chan_config->chain)) {
		dev_err(dev,
			"In continuous mode, resizer can be only chained. \n");
		return -EINVAL;
	}

	if (channel->config_state == STATE_NOT_CONFIGURED) {
		if (chan_config->oper_mode == IMP_MODE_CONTINUOUS) {
			/* allocate shared hw config block */
			dev_dbg(dev, "imp_set_resizer_config-cont. \n");
			channel->config = imp_hw_if->alloc_config_block(dev, 1);
		} else {
			if (chan_config->chain) {
				dev_dbg(dev,
					"imp_set_resizer_config-ss, chain. \n");
				channel->config =
				    imp_hw_if->alloc_config_block(dev, 1);
			} else {
				dev_dbg(dev,
					"imp_set_resizer_config-ss,"
					" no-chain.\n");
				channel->config =
				    imp_hw_if->alloc_config_block(dev, 0);
			}
		}
		if (ISNULL(channel->config)) {
			dev_err(dev, "memory allocation failed\n");
			return -EFAULT;
		}
		/* allocate buffer for holding user configuration */
		channel->user_config = imp_hw_if->alloc_user_config_block(dev,
								IMP_RESIZER,
								chan_config->
								oper_mode,
								&len);
		if (ISNULL(channel->user_config)) {
			dev_err(dev, "memory allocation failed\n");
			if (!chan_config->chain)
				kfree(channel->config);
			return -EFAULT;
		}
		channel->user_config_size = len;
		dev_dbg(dev, "imp_set_resizer_config, len = %d. \n", len);
	}

	if (ISNULL(chan_config->config)) {
		/* put defaults for user configuration */
		imp_hw_if->set_user_config_defaults(dev,
						    IMP_RESIZER,
						    chan_config->oper_mode,
						    channel->user_config);
		dev_dbg(dev, "imp_set_resizer_config, default\n");
	} else {
		if (copy_from_user(channel->user_config,
				   chan_config->config,
				   channel->user_config_size)) {
			dev_err(dev, "Copy from user to kernel failed\n");
			return -EFAULT;
		}
		dev_dbg(dev, "imp_set_resizer_config, user setting\n");
	}

	/* Update the user configuration in the hw config block or
	   if chained, copy it to the shared block and allow previewer
	   to configure it */
	ret = imp_hw_if->set_resizer_config(dev,
					    chan_config->oper_mode,
					    chan_config->chain,
					    channel->user_config,
					    channel->config);

	if (ret < 0)
		dev_err(dev, "set resizer config failed\n");

	channel->chained = chan_config->chain;
	channel->config_state = STATE_CONFIGURED;

	return ret;
}
EXPORT_SYMBOL(imp_set_resizer_config);

int imp_get_preview_config(struct device *dev,
			   struct imp_logical_channel *channel,
			   struct prev_channel_config *chan_config)
{
	if (channel->mode == IMP_MODE_INVALID) {
		dev_err(dev, "Channel mode is not set. \n");
		return -EINVAL;
	}

	if (channel->mode != chan_config->oper_mode) {
		dev_err(dev,
			"mode mis-match, chan mode = %d, config mode = %d\n",
			channel->mode, chan_config->oper_mode);
		return -EINVAL;
	}

	if (channel->config_state != STATE_CONFIGURED) {
		dev_err(dev, "channel not configured\n");
		return -EINVAL;
	}

	if (ISNULL(chan_config->config)) {
		dev_err(dev, "NULL ptr\n");
		return -EINVAL;
	}

	if (copy_to_user((void *)chan_config->config,
			 (void *)channel->user_config,
			 channel->user_config_size)) {
		dev_err(dev, "Error in copy to user\n");
		return -EFAULT;
	}
	return 0;
}
EXPORT_SYMBOL(imp_get_preview_config);

int imp_get_resize_config(struct device *dev,
			  struct imp_logical_channel *channel,
			  struct rsz_channel_config *chan_config)
{
	dev_dbg(dev, "imp_get_resize_config:\n");
	if (channel->mode == IMP_MODE_INVALID) {
		dev_err(dev, "Channel mode is not set. \n");
		return -EINVAL;
	}

	if (channel->mode != chan_config->oper_mode) {
		dev_err(dev,
			"mode mis-match, chan mode = %d, config mode = %d\n",
			channel->mode, chan_config->oper_mode);
		return -EINVAL;
	}

	if (channel->config_state != STATE_CONFIGURED) {
		dev_err(dev, "channel not configured\n");
		return -EINVAL;
	}

	if (ISNULL(chan_config->config)) {
		dev_err(dev, "NULL ptr\n");
		return -EINVAL;
	}

	if (copy_to_user((void *)chan_config->config,
			 (void *)channel->user_config,
			 channel->user_config_size)) {
		dev_err(dev, "Error in copy to user\n");
		return -EFAULT;
	}
	return 0;
}
EXPORT_SYMBOL(imp_get_resize_config);

struct prev_module_if *imp_get_module_interface(struct device *dev,
						unsigned short module_id)
{
	struct prev_module_if *module_if;
	unsigned int index = 0;
	while (1) {
		module_if = imp_hw_if->prev_enum_modules(dev, index);
		if (ISNULL(module_if))
			break;
		if (module_if->module_id == module_id)
			break;
		index++;
	}
	return module_if;
}
EXPORT_SYMBOL(imp_get_module_interface);

int imp_init_serializer(void)
{
	if (!serializer_initialized) {
		memset((void *)&imp_serializer_info, (char)0,
		       sizeof(struct imp_serializer));
		init_completion(&imp_serializer_info.sem_isr);
		imp_serializer_info.sem_isr.done = 0;
		imp_serializer_info.array_count = 0;
		mutex_init(&imp_serializer_info.array_sem);
		printk(KERN_NOTICE "imp serializer initialized\n");
		serializer_initialized = 1;
		imp_hw_if = imp_get_hw_if();
	}
	return 0;
}
EXPORT_SYMBOL(imp_init_serializer);

/*
=====================add_to_array===========================
 Function to add the current channel configuration into	array
according to priority.
*/
static int add_to_array(struct device *dev, struct imp_logical_channel *chan)
{
	int array_index, device_index, ret;

	/* locking the configuartion aaray */
	ret = mutex_lock_interruptible(&imp_serializer_info.array_sem);
	if (ret < 0)
		return ret;

	/* Add configuration to the     queue according to its priority */
	if (imp_serializer_info.array_count == 0) {
		/* If array     empty insert at top     position */
		dev_dbg(dev, "First request for resizing \n");
		imp_serializer_info.channel_config[imp_serializer_info.
						   array_count]
		    = chan;
	} else {
		/* Check the priority and insert according to the priority */
		/* it will start from first     index */
		for (array_index = 1;
		     array_index < imp_serializer_info.array_count;
		     array_index++) {
			if (imp_serializer_info.channel_config[array_index]->
			    priority < chan->priority)
				break;
		}
		/* Shift all the elements one step down in array */
		/* IF firstelement and second have same prioroty than insert */
		/* below first */
		for (device_index = imp_serializer_info.array_count;
		     device_index > array_index; device_index--) {
			imp_serializer_info.channel_config[device_index] =
			    imp_serializer_info.
			    channel_config[device_index - 1];
		}

		imp_serializer_info.channel_config[array_index] = chan;
	}

	/* incrementing number of requests for resizing */
	imp_serializer_info.array_count++;
	dev_dbg(dev, "The total request for resizing are %d",
		imp_serializer_info.array_count);

	if (imp_serializer_info.array_count != 1) {
		mutex_unlock(&imp_serializer_info.array_sem);

		/* if the request is pending that lock the request */
		wait_for_completion_interruptible(&(chan->channel_sem));

	} else {
		mutex_unlock(&imp_serializer_info.array_sem);
	}

	return 0;
}				/*  end of function addToarray */

/*
=====================delete_from_array===========================
 Function	to delete the processed	array entry	form the array
*/
static int delete_from_array(struct device *dev,
			     struct imp_logical_channel *chan)
{
	int array_index = 0, device_index, ret;

	ret = mutex_lock_interruptible(&(imp_serializer_info.array_sem));

	if (ret < 0)
		return ret;

	/*shift the     entried in array */
	if (imp_serializer_info.array_count != 1) {
		/* decrementing the     request count */
		imp_serializer_info.array_count--;

		/* Shift all the elements one step up in array */
		for (device_index = array_index;
		     device_index < imp_serializer_info.array_count;
		     device_index++) {

			imp_serializer_info.channel_config[device_index] =
			    imp_serializer_info.
			    channel_config[device_index + 1];
		}
		/* making last entry NULL; */
		imp_serializer_info.channel_config[device_index + 1] = NULL;
	}
	/* remove the top entry */
	else {
		dev_dbg(dev, "\n Removing the first request");
		imp_serializer_info.array_count--;
		imp_serializer_info.channel_config[0] = NULL;
	}

	if (imp_serializer_info.array_count != 0) {
		/* Get config having highest priority in array
		   resizer_device.config
		   and unlock config.sem of that config */

		dev_dbg(dev,
			"Releasing array lock of the	second entry\n");
		complete(&(imp_serializer_info.channel_config[0]->channel_sem));
		mutex_unlock(&(imp_serializer_info.array_sem));
	} else {
		dev_dbg(dev, "Releasing array lock	\n");
		mutex_unlock(&(imp_serializer_info.array_sem));
	}

	return 0;
}

/**
 * imp_uservirt_to_phys : translate user/virtual address to phy address
 * @virtp: user/virtual address
 *
 * This inline function is used to convert user space virtual address to
 * physical address.
 */
static inline u32 imp_uservirt_to_phys(struct device *dev, u32 virtp)
{
	unsigned long physp = 0;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	/* For kernel direct-mapped memory, take the easy way */
	if (virtp >= PAGE_OFFSET) {
		physp = virt_to_phys((void *)virtp);
	} else if ((vma = find_vma(mm, virtp)) && (vma->vm_flags & VM_IO) &&
		   (vma->vm_pgoff)) {
		/**
		 * this will catch, kernel-allocated, mmaped-to-usermode
		 * addresses
		 */
		physp = (vma->vm_pgoff << PAGE_SHIFT) + (virtp - vma->vm_start);
	} else {
		/* otherwise, use get_user_pages() for general userland pages */
		int res, nr_pages = 1;
		struct page *pages;
		down_read(&current->mm->mmap_sem);

		res = get_user_pages(current, current->mm,
				     virtp, nr_pages, 1, 0, &pages, NULL);
		up_read(&current->mm->mmap_sem);

		if (res == nr_pages)
			physp = __pa(page_address(&pages[0]) +
				     (virtp & ~PAGE_MASK));
		else {
			dev_err(dev, "get_user_pages failed\n");
			return 0;
		}
	}
	return physp;
}

static int imp_common_start(struct device *dev,
		     struct imp_logical_channel *chan,
		     struct imp_convert *convert)
{
	int ret, prev;
	struct irq_numbers irq;
	unsigned int offset = 0;
	unsigned long addr;
	int status = 0;

	if ((convert->in_buff.size == 0) || (convert->in_buff.offset == 0)) {
		dev_err(dev, "in_buff buf size or offset invalid\n");
		return -1;
	}
	if (chan->in_numbufs == 0) {
		/* user ptr IO */
		if (convert->in_buff.index != -1) {
			dev_err(dev,
				"in_buff index should be set to -1"
				" for user ptr io\n");
			return -1;
		}
		if (convert->in_buff.offset % 32) {
			dev_err(dev, "in_buff Offset to be a multiple of 32\n");
			return -1;
		}
		addr = imp_uservirt_to_phys(dev, convert->in_buff.offset);
		if (!addr) {
			dev_err(dev, "in_buff Offset - can't get user page\n");
			return -1;
		}
		if (imp_hw_if->update_inbuf_address(chan->config, addr) < 0) {
			dev_err(dev,
				"Error in configuring input buffer address\n");
			return -EINVAL;
		}
	} else {
		if ((convert->in_buff.index < 0) ||
		    (convert->in_buff.index >= chan->in_numbufs)) {
			dev_err(dev, "in_buff index is out of range\n");
			return -1;
		}
		if (cpu_is_davinci_dm365()) {
			if ((convert->in_buff.offset + convert->in_buff.size)  >
			 (chan->in_bufs[convert->in_buff.index]->offset +
			 chan->in_bufs[convert->in_buff.index]->size)) {
				dev_err(dev,
					"in_buff offset provided "
					" is out of range, %d:%d:%d:%d:%d\n",
					convert->in_buff.index,
					convert->in_buff.offset,
					convert->in_buff.size,
					chan->in_bufs[convert->in_buff.index]->offset,
					chan->in_bufs[convert->in_buff.index]->size);
				return -1;
			}
		} else if (convert->in_buff.offset !=
		    chan->in_bufs[convert->in_buff.index]->offset) {
			dev_err(dev,
				"in_buff offset provided doesn't"
				" match, %d:%d:%d\n",
				convert->in_buff.index,
				convert->in_buff.offset,
				chan->in_bufs[convert->in_buff.index]->offset);
			return -1;
		}
		if (imp_hw_if->update_inbuf_address(chan->config,
						    convert->in_buff.
						    offset) < 0) {
			dev_err(dev,
				"Error in configuring input buffer address\n");
			return -EINVAL;
		}
	}

	if ((convert->out_buff1.size != 0)
		&& (convert->out_buff1.offset != 0)) {
		if (chan->out_numbuf1s == 0) {
			/* user ptr IO */
			if (convert->out_buff1.index != -1) {
				dev_err(dev,
					"out_buff1 index should be"
					" -1 for user ptr IO\n");
				return -1;
			}
			if (convert->out_buff1.offset % 32) {
				dev_err(dev,
					"out_buff1 Offset to be a multiple"
					" of 32\n");
				return -1;
			}
			offset = imp_uservirt_to_phys(dev, convert->out_buff1.offset);
			if (!offset) {
				dev_err(dev, "out_buff1 Offset - can't get user page\n");
				return -1;
			}
			status = 1;
		} else {
			/* memory mapped IO */
			if ((convert->out_buff1.index < 0) ||
			    (convert->out_buff1.index >= chan->out_numbuf1s)) {
				dev_err(dev,
					"out_buff1, index is out of range\n");
				return -1;
			}
			if (cpu_is_davinci_dm365()) {
				if ((convert->out_buff1.offset +
				     convert->out_buff1.size)  >
				    (chan->out_buf1s[convert->out_buff1.index]->offset +
				     chan->out_buf1s[convert->out_buff1.index]->size)) {
					dev_err(dev,
					"out_buff offset provided "
					" is out of range, %d:%d:%d:%d:%d\n",
					convert->out_buff1.index,
					convert->out_buff1.offset,
					convert->out_buff1.size,
					chan->out_buf1s[convert->out_buff1.index]->offset,
					chan->out_buf1s[convert->out_buff1.index]->size);
					return -1;
				}
			} else if (convert->out_buff1.offset !=
			    chan->out_buf1s[convert->out_buff1.index]->offset) {
				dev_err(dev,
					"out_buff1 offset provided"
					" doesn't match\n");
				return -1;
			}
			offset =
			    convert->out_buff1.offset;
			status = 1;
		}
	}

	if (imp_hw_if->update_outbuf1_address(chan->config, offset) < 0) {
		dev_err(dev, "Error in configuring out_buff1 address\n");
		return -EINVAL;
	}

	offset = 0;
	if ((convert->out_buff2.size != 0)
		&& (convert->out_buff2.offset != 0)) {
		if (chan->out_numbuf2s == 0) {
			/* user ptr IO */
			if (convert->out_buff2.index != -1) {
				dev_err(dev,
					"out_buff2 index should be -1"
					" for user ptr IO\n");
				return -1;
			}
			if (convert->out_buff2.offset % 32) {
				dev_err(dev,
					"out_buff2 Offset to be a"
					" multiple of 32\n");
				return -1;
			}
			status = 1;
			offset = imp_uservirt_to_phys(dev, convert->out_buff2.offset);
			if (!offset) {
				dev_err(dev, "out_buff2 Offset - can't get user page\n");
				return -1;
			}
		} else {
			/* memory mapped IO */
			if ((convert->out_buff2.index < 0) ||
			    (convert->out_buff2.index >= chan->out_numbuf2s)) {
				dev_err(dev,
					"out_buff1, index is out of range\n");
				return -1;
			}
			if (cpu_is_davinci_dm365()) {
				if ((convert->out_buff2.offset + convert->out_buff2.size)  >
				(chan->out_buf2s[convert->out_buff2.index]->offset + chan->out_buf2s[convert->out_buff2.index]->size)) {
					dev_err(dev,
					"out_buff offset provided "
					" is out of range, %d:%d:%d:%d:%d\n",
					convert->out_buff2.index,
					convert->out_buff2.offset,
					convert->out_buff2.size,
					chan->out_buf2s[convert->out_buff2.index]->offset,
					chan->out_buf2s[convert->out_buff2.index]->size);
					return -1;
				}
			} else if (convert->out_buff2.offset !=
			    chan->out_buf2s[convert->out_buff2.index]->offset) {
				dev_err(dev,
					"out_buff2 offset provided"
					" doesn't match\n");
				return -1;
			}
			offset =
			    convert->out_buff2.offset;
			status = 1;
		}
	}

	if (!(ISNULL(imp_hw_if->update_outbuf2_address))) {
		if (imp_hw_if->update_outbuf2_address(
				chan->config, offset) < 0) {
			dev_err(dev,
				"Error in configuring out_buff2 address\n");
			return -EINVAL;
		}
	}

	if (!status) {
		dev_err(dev,
			"either out_buff1 or  out_buff2 address needed \n");
		return -EINVAL;
	}

	ret = add_to_array(dev, chan);

	if (ret < 0)
		return ret;

	if (chan->type == IMP_PREVIEWER) {
		if (chan->chained) {
			imp_hw_if->get_rsz_irq(&irq);
			prev = 0;
		} else {
			imp_hw_if->get_preview_irq(&irq);
			prev = 1;
		}
	} else {
		/* Resizer channel */
		imp_hw_if->get_rsz_irq(&irq);
		prev = 0;
	}

	if (prev)
		ret = request_irq(irq.sdram, imp_common_isr, IRQF_DISABLED,
				  "DaVinciPreviewer", (void *)NULL);
	else
		ret = request_irq(irq.sdram, imp_common_isr, IRQF_DISABLED,
				  "DaVinciResizer", (void *)NULL);
	if (imp_hw_if->serialize())
		imp_hw_if->hw_setup(dev, chan->config);

	imp_hw_if->enable(1, chan->config);

	/* Waiting for resizing to be complete */
	wait_for_completion_interruptible(&(imp_serializer_info.sem_isr));

	free_irq(irq.sdram, (void *)NULL);

	ret = delete_from_array(dev, chan);

	return ret;
}

int imp_common_start_resize(struct device *dev,
			    struct imp_logical_channel *chan,
			    struct imp_convert *convert)
{
	return (imp_common_start(dev, chan, convert));
}
EXPORT_SYMBOL(imp_common_start_resize);

int imp_common_start_preview(struct device *dev,
			     struct imp_logical_channel *chan,
			     struct imp_convert *convert)
{
	return (imp_common_start(dev, chan, convert));
}
EXPORT_SYMBOL(imp_common_start_preview);

int imp_common_reconfig_resizer(struct device *dev,
			struct rsz_reconfig *reconfig,
			struct imp_logical_channel *chan)
{
	if (chan->config_state != STATE_CONFIGURED) {
		dev_err(dev, "Configure channel first before reconfig\n");
		return -EINVAL;
	}
	if (ISNULL(imp_hw_if->reconfig_resizer)) {
		dev_err(dev, "reconfig is not supported\n");
		return -EINVAL;
	}

	return imp_hw_if->reconfig_resizer(dev, reconfig, chan->config);
}
EXPORT_SYMBOL(imp_common_reconfig_resizer);

static __init int imp_common_init(void)
{
	return 0;
}
static void imp_cleanup(void)
{
}

MODULE_LICENSE("GPL");

module_init(imp_common_init);
module_exit(imp_cleanup);
