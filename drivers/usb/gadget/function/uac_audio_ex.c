/*
 * uac_device.c -- interface to USB gadget "ALSA sound card" utilities
 *
 * Copyright (C) 2016
 * Author: Ruslan Bilovol <ruslan.bilovol@gmail.com>
 *
 * Sound card implementation was cut-and-pasted with changes
 * from f_uac2.c and has:
 *    Copyright (C) 2011
 *    Yadwinder Singh (yadi.brar01@gmail.com)
 *    Jaswinder Singh (jaswinder.singh@linaro.org)
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
 */

#include <linux/module.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "uac_queue_ex.h"
#include "uac_ex.h"

static void
__uac_audio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uac_device *uac_dev = req->context;
	struct uac_queue *queue = &uac_dev->queue;
	struct uac_buffer *buf;

	unsigned long flags;
	int ret;

	switch (req->status) {
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		printk(KERN_DEBUG "AS request cancelled.\n");
		uac_queue_cancel(queue, 1);
		goto err;

	default:
		printk(KERN_INFO "AS request completed with status %d.\n",
			req->status);
		uac_queue_cancel(queue, 0);
		goto err;
	}

	spin_lock_irqsave(&uac_dev->queue.irqlock, flags);
	buf = uac_queue_head(&uac_dev->queue);
	if (buf == NULL) {
		req->length = 0;
		goto tran_zero;
	}

	{
		/*
		 * For each IN packet, take the quotient of the current data
		 * rate and the endpoint's interval as the base packet size.
		 * If there is a residue from this division, add it to the
		 * residue accumulator.
		 */
		req->length = uac_dev->p_pktsize;
		uac_dev->p_residue += uac_dev->p_pktsize_residue;

		/*
		 * Whenever there are more bytes in the accumulator than we
		 * need to add one more sample frame, increase this packet's
		 * size and decrease the accumulator.
		 */

		if (uac_dev->p_residue / uac_dev->p_interval >= uac_dev->p_framesize)
		{
			req->length += uac_dev->p_framesize;
			uac_dev->p_residue -= uac_dev->p_framesize *
				uac_dev->p_interval;
		}

		req->actual = req->length;
	}

	{
		unsigned int nbytes;
		void *mem;

		/* Copy video data to the USB buffer. */
		mem = buf->mem + queue->buf_used;
		nbytes = min((unsigned int)req->length, buf->bytesused - queue->buf_used);

		memcpy(req->buf, mem, nbytes);
		queue->buf_used += nbytes;

		req->length = nbytes;
		req->actual = req->length;

		if (buf->bytesused == uac_dev->queue.buf_used) {
			uac_dev->queue.buf_used = 0;
			buf->state = UVC_BUF_STATE_DONE;
			uac_queue_next_buffer(&uac_dev->queue, buf);
		}
	}

tran_zero:

	req->actual = req->length;

	if ((ret = usb_ep_queue(ep, req, GFP_ATOMIC)) < 0) {
		printk(KERN_INFO "Failed to queue request (%d).\n", ret);
		usb_ep_set_halt(ep);
		spin_unlock_irqrestore(&uac_dev->queue.irqlock, flags);
		uac_queue_cancel(queue, 0);
	}
	spin_unlock_irqrestore(&uac_dev->queue.irqlock, flags);

err:
	return;
}

static inline void free_ep(struct uac_device *dev, struct usb_ep *ep)
{
	struct uac_params *params;
	int i;

	if (!dev->ep_enabled)
		return;

	dev->ep_enabled = false;
	params = &dev->params;

	for (i = 0; i < params->req_number; i++) {
		if (dev->ureq[i].req) {
			usb_ep_dequeue(ep, dev->ureq[i].req);
			usb_ep_free_request(ep, dev->ureq[i].req);
			dev->ureq[i].req = NULL;
		}
	}

	if (usb_ep_disable(ep))
		printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
}

int uac_device_start_playback(struct uac_device *uac_dev)
{
	struct usb_gadget *gadget = uac_dev->gadget;
	struct usb_request *req;
	struct usb_ep *ep;
	struct uac_params *params = &uac_dev->params;
	unsigned int factor, rate;
	const struct usb_endpoint_descriptor *ep_desc;
	int req_len, i;

	if (uac_dev->ep_enabled)
		return 0;

	ep = uac_dev->in_ep;
	config_ep_by_speed(gadget, &uac_dev->func, ep);
	ep_desc = ep->desc;

	/* pre-calculate the playback endpoint's interval */
	if (gadget->speed == USB_SPEED_FULL)
		factor = 1000;
	else
		factor = 8000;

	/* pre-compute some values for iso_complete() */
	uac_dev->p_framesize = params->p_ssize *
		num_channels(params->p_chmask);
	rate = params->p_srate * uac_dev->p_framesize;
	uac_dev->p_interval = factor / (1 << (ep_desc->bInterval - 1));
	uac_dev->p_pktsize = min_t(unsigned int, rate / uac_dev->p_interval,
				uac_dev->max_psize);

	if (uac_dev->p_pktsize < uac_dev->max_psize)
		uac_dev->p_pktsize_residue = rate % uac_dev->p_interval;
	else
		uac_dev->p_pktsize_residue = 0;

	req_len = uac_dev->p_pktsize;
	uac_dev->p_residue = 0;

	uac_dev->ep_enabled = true;
	usb_ep_enable(ep);

	printk(KERN_EMERG "p_framesize=%d p_interval=%d p_pktsize=%d \
					p_pktsize_residue=%d m=%d\n",
					uac_dev->p_framesize,
					uac_dev->p_interval,
					uac_dev->p_pktsize,
					uac_dev->p_pktsize_residue,
					uac_dev->max_psize);

	for (i = 0; i < params->req_number; i++) {
		if (!uac_dev->ureq[i].req) {
			req = usb_ep_alloc_request(ep, GFP_ATOMIC);
			if (req == NULL)
				return -ENOMEM;

			uac_dev->ureq[i].req = req;
			uac_dev->ureq[i].dev = uac_dev;

			req->zero = 0;
			req->context = uac_dev;
			req->length = req_len;
			req->complete = __uac_audio_complete;
			req->buf = uac_dev->rbuf + i * uac_dev->max_psize;
		}

		if (usb_ep_queue(ep, uac_dev->ureq[i].req, GFP_ATOMIC))
			printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
	}

	return 0;
}

void uac_device_stop_playback(struct uac_device *uac_dev)
{
	free_ep(uac_dev, uac_dev->in_ep);
}

int uac_device_setup(struct uac_device *uac_dev)
{
	struct uac_params *params;
	int p_chmask;
	int err = 0;

	if (!uac_dev)
		return -EINVAL;

	params = &uac_dev->params;
	p_chmask = params->p_chmask;

	if (p_chmask) {
		uac_dev->max_psize = uac_dev->in_ep_maxpsize;
		uac_dev->ureq = kcalloc(params->req_number, sizeof(struct uac_req),
				GFP_KERNEL);
		if (!uac_dev->ureq) {
			err = -ENOMEM;
			goto fail;
		}

		uac_dev->rbuf = kcalloc(params->req_number, uac_dev->max_psize,
				GFP_KERNEL);
		if (!uac_dev->rbuf) {
			uac_dev->max_psize = 0;
			err = -ENOMEM;
			goto fail;
		}
	}

	if (!err)
		return 0;
fail:
	kfree(uac_dev->ureq);
	kfree(uac_dev->rbuf);

	return err;
}

void uac_device_cleanup(struct uac_device *uac_dev)
{
	if (!uac_dev)
		return;

	kfree(uac_dev->ureq);
	kfree(uac_dev->rbuf);
}
