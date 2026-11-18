// ### SIPEED EDIT ###
// SPDX-License-Identifier: GPL-2.0+
/*
 * f_loopback.c - USB peripheral loopback configuration driver
 *
 * Copyright (C) 2003-2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 */

/* #define VERBOSE_DEBUG */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/usb/composite.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include "udisp.h"
#include "u_f.h"

#define FPS_STAT_MAX 32

long get_os_us(void)
{
	struct timespec64 ts;
	ktime_get_coarse_real_ts64(&ts);
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

typedef struct {
	long tb[FPS_STAT_MAX];
	int cur;
	long last_fps;
} fps_mgr_t;

fps_mgr_t fps_mgr = {
	.cur = 0,
	.last_fps = -1,
};

long get_fps(void)
{
	fps_mgr_t *mgr = &fps_mgr;
	long a, b, delta_us;
	int i, cur_idx;

	if (unlikely(mgr->cur < FPS_STAT_MAX))
		return mgr->last_fps;

	cur_idx = (mgr->cur - 1) % FPS_STAT_MAX;
	a = mgr->tb[cur_idx];

	for (i = 2; i < FPS_STAT_MAX; i++) {
		int idx = (mgr->cur - i) % FPS_STAT_MAX;
		b = mgr->tb[idx];
		delta_us = a - b;
		if (delta_us > 1000000L)
			break;
	}

	if (unlikely(i <= 1))
		return mgr->last_fps;

	delta_us = a - mgr->tb[(mgr->cur - i) % FPS_STAT_MAX];

	if (unlikely(delta_us <= 0))
		return mgr->last_fps;

	mgr->last_fps = (1000000L * 10) * (i - 1) / delta_us;
	return mgr->last_fps;
}

void put_fps_data(long t)
{
	fps_mgr_t *mgr = &fps_mgr;
	int idx;

	idx = mgr->cur % FPS_STAT_MAX;

	if (mgr->cur > 0) {
		long prev_t = mgr->tb[(mgr->cur - 1) % FPS_STAT_MAX];
		if (t <= prev_t)
			t = prev_t + 1;
	}

	mgr->tb[idx] = t;

	if (mgr->cur > 1000000000)
		mgr->cur = FPS_STAT_MAX;
	else
		mgr->cur++;
}

#define UDISP_BUF_SIZE 320 * 170 * 4 // for some data case , 100kB is small

typedef uint8_t _u8;
typedef uint16_t _u16;
typedef uint32_t _u32;

#define UDISP_TYPE_RGB565 0
#define UDISP_TYPE_RGB888 1
#define UDISP_TYPE_YUV420 2
#define UDISP_TYPE_JPG 3

typedef struct _udisp_frame_header_t {
	_u16 crc16;
	_u8 type;
	_u8 cmd;
	_u16 x;
	_u16 y;
	_u16 width;
	_u16 height;
	_u32 frame_id : 10;
	_u32 payload_total : 22;
} __attribute__((packed)) udisp_frame_header_t;

typedef struct {
	udisp_frame_header_t frame_hd;
	_u16 frame_id;
	_u16 x;
	_u16 y;
	_u16 x2;
	_u16 y2;
	_u16 y_idx;
	int rx_cnt;
	int disp_cnt;
	int done;
} disp_frame_mgr_t;

typedef struct {
	struct list_head list_node;
	udisp_frame_header_t hd;
	uint8_t buf[UDISP_BUF_SIZE];
} udisp_frame_t;

#define JPG_FRAME_MAX 3

struct f_loopback {
	struct usb_function function;
	struct usb_ep *in_ep;
	struct usb_ep *out_ep;
	struct task_struct *work_thread;
	struct kfifo con_buf;
	struct kfifo jpg_buf;
	struct list_head jpg_free_list;
	struct list_head jpg_data_list;
	udisp_frame_t jpg_tb[JPG_FRAME_MAX];
	udisp_frame_t rgb888x_buf;
	atomic_t jpg_atom_cnt;
	atomic_t rx_total;
	atomic_t open_flag;
	atomic64_t total_frames;
	atomic64_t total_bytes;
	atomic64_t dropped_frames;
	atomic64_t processed_frames;
	spinlock_t con_lock;
	struct file *filp;
	void __iomem *screen_base;
	unsigned qlen;
	unsigned buflen;
	wait_queue_head_t wait_queue;
};

static inline size_t _list_count_nodes(struct list_head *head)
{
	size_t count = 0;
	struct list_head *pos;

	if (!head)
		return 0;

	list_for_each (pos, head)
		count++;

	return count;
}

static inline struct f_loopback *func_to_loop(struct usb_function *f)
{
	return container_of(f, struct f_loopback, function);
}

static struct usb_interface_descriptor loopback_intf = {
	.bLength = sizeof(loopback_intf),
	.bDescriptorType = USB_DT_INTERFACE,

	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_loop_source_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_loop_sink_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_loopback_descs[] = {
	(struct usb_descriptor_header *)&loopback_intf,
	(struct usb_descriptor_header *)&fs_loop_sink_desc,
	(struct usb_descriptor_header *)&fs_loop_source_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_loop_source_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_loop_sink_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_descriptor_header *hs_loopback_descs[] = {
	(struct usb_descriptor_header *)&loopback_intf,
	(struct usb_descriptor_header *)&hs_loop_source_desc,
	(struct usb_descriptor_header *)&hs_loop_sink_desc,
	NULL,
};

/* super speed support: */

static struct usb_endpoint_descriptor ss_loop_source_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_loop_source_comp_desc = {
	.bLength = USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst = 0,
	.bmAttributes = 0,
	.wBytesPerInterval = 0,
};

static struct usb_endpoint_descriptor ss_loop_sink_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_loop_sink_comp_desc = {
	.bLength = USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst = 0,
	.bmAttributes = 0,
	.wBytesPerInterval = 0,
};

static struct usb_descriptor_header *ss_loopback_descs[] = {
	(struct usb_descriptor_header *)&loopback_intf,
	(struct usb_descriptor_header *)&ss_loop_source_desc,
	(struct usb_descriptor_header *)&ss_loop_source_comp_desc,
	(struct usb_descriptor_header *)&ss_loop_sink_desc,
	(struct usb_descriptor_header *)&ss_loop_sink_comp_desc,
	NULL,
};

/* function-specific strings: */

static struct usb_string strings_loopback[] = {
	[0].s = "loop input to output",
	{} /* end of list */
};

static struct usb_gadget_strings stringtab_loop = {
	.language = 0x0409, /* en-us */
	.strings = strings_loopback,
};

static struct usb_gadget_strings *loopback_strings[] = {
	&stringtab_loop,
	NULL,
};

static int loopback_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_loopback *loop = func_to_loop(f);
	int id;
	int ret;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	loopback_intf.bInterfaceNumber = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_loopback[0].id = id;
	loopback_intf.iInterface = id;

	/* allocate endpoints */

	loop->in_ep = usb_ep_autoconfig(cdev->gadget, &fs_loop_source_desc);
	if (!loop->in_ep) {
	autoconf_fail:
		ERROR(cdev, "%s: can't autoconfigure on %s\n", f->name,
		      cdev->gadget->name);
		return -ENODEV;
	}

	loop->out_ep = usb_ep_autoconfig(cdev->gadget, &fs_loop_sink_desc);
	if (!loop->out_ep)
		goto autoconf_fail;

	/* support high speed hardware */
	hs_loop_source_desc.bEndpointAddress =
		fs_loop_source_desc.bEndpointAddress;
	hs_loop_sink_desc.bEndpointAddress = fs_loop_sink_desc.bEndpointAddress;

	/* support super speed hardware */
	ss_loop_source_desc.bEndpointAddress =
		fs_loop_source_desc.bEndpointAddress;
	ss_loop_sink_desc.bEndpointAddress = fs_loop_sink_desc.bEndpointAddress;

	ret = usb_assign_descriptors(f, fs_loopback_descs, hs_loopback_descs,
				     ss_loopback_descs, NULL);
	if (ret)
		return ret;

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
	    (gadget_is_superspeed(c->cdev->gadget) ?
		     "super" :
		     (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
	    f->name, loop->in_ep->name, loop->out_ep->name);
	return 0;
}

static void lb_free_func(struct usb_function *f)
{
	struct f_lb_opts *opts;

	opts = container_of(f->fi, struct f_lb_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);

	usb_free_all_descriptors(f);
	kfree(func_to_loop(f));
}

#define CONFIG_USB_VENDOR_RX_BUFSIZE 512

disp_frame_mgr_t g_disp_frame_mgr;

uint16_t crc16_calc_multi(uint16_t crc_reg, const unsigned char *puchMsg,
			  unsigned int usDataLen)
{
	unsigned int i, j;
	uint32_t check;

	for (i = 0; i < usDataLen; i++) {
		crc_reg = (crc_reg >> 8) ^ puchMsg[i];
		for (j = 0; j < 8; j++) {
			check = crc_reg & 0x0001;
			crc_reg >>= 1;
			if (check)
				crc_reg ^= 0xA001;
		}
	}
	return crc_reg;
}

uint16_t crc16_calc(const unsigned char *puchMsg, unsigned int usDataLen)
{
	return crc16_calc_multi(0xFFFF, puchMsg, usDataLen);
}

size_t xStreamBufferSend(void *ctx, const uint8_t *buf, size_t len)
{
	struct f_loopback *loop = ctx;
	size_t xfer;
	unsigned long flags;

	spin_lock_irqsave(&loop->con_lock, flags);
	xfer = kfifo_in(&loop->con_buf, buf, len);
	spin_unlock_irqrestore(&loop->con_lock, flags);

	return xfer;
}

int pop_msg_data(void *ctx, const uint8_t *rx_buf, size_t len)
{
	size_t xTxBytes;

	if (len == 0)
		return 0;

	xTxBytes = xStreamBufferSend(ctx, rx_buf, len);

	if (xTxBytes != len) {
		pr_err("send data NG: expected %zu, sent %zu\n", len, xTxBytes);
		return -1;
	}

	return 0;
}

void push_a_frame_to_worker(void *ctx)
{
	struct f_loopback *loop = ctx;
	udisp_frame_t *jfr = NULL;
	int len = 0;
	size_t free_list_count, data_list_count;
	unsigned long flags;

	if (unlikely(!ctx))
		return;

	if (!g_disp_frame_mgr.rx_cnt ||
	    g_disp_frame_mgr.frame_hd.payload_total != g_disp_frame_mgr.rx_cnt)
		return;

	spin_lock_irqsave(&loop->con_lock, flags);
	free_list_count = _list_count_nodes(&loop->jpg_free_list);

	if (list_empty(&loop->jpg_free_list)) {
		spin_unlock_irqrestore(&loop->con_lock, flags);
		DEBUG_LOG("%s no free jpg frame, payload:%d\n", __func__,
			  g_disp_frame_mgr.frame_hd.payload_total);

		atomic64_inc(&loop->dropped_frames);

		kfifo_reset(&loop->con_buf);
		return;
	}

	jfr = list_first_entry(&loop->jpg_free_list, udisp_frame_t, list_node);

	if (unlikely(g_disp_frame_mgr.frame_hd.payload_total > UDISP_BUF_SIZE ||
		     g_disp_frame_mgr.frame_hd.payload_total < 0x10)) {
		spin_unlock_irqrestore(&loop->con_lock, flags);
		DEBUG_LOG("%s drop invalid frame size:%d\n", __func__,
			  g_disp_frame_mgr.frame_hd.payload_total);

		atomic64_inc(&loop->dropped_frames);

		kfifo_reset(&loop->con_buf);
		return;
	}

	len = kfifo_out(&loop->con_buf, jfr->buf,
			min_t(size_t, g_disp_frame_mgr.frame_hd.payload_total,
			      UDISP_BUF_SIZE));

	jfr->hd = g_disp_frame_mgr.frame_hd;
	list_del(&jfr->list_node);
	list_add_tail(&jfr->list_node, &loop->jpg_data_list);
	data_list_count = _list_count_nodes(&loop->jpg_data_list);

	spin_unlock_irqrestore(&loop->con_lock, flags);

	atomic64_inc(&loop->total_frames);
	atomic64_add(g_disp_frame_mgr.frame_hd.payload_total,
		     &loop->total_bytes);

	DEBUG_LOG(
		"%s pushed frame: payload:%d, jpg_free_list:%lu, jpg_data_list:%lu, crc:%x\n",
		__func__, g_disp_frame_mgr.frame_hd.payload_total,
		free_list_count - 1, data_list_count, jfr->hd.crc16);

	wake_up_interruptible(&loop->wait_queue);
	kfifo_reset(&loop->con_buf);
}

void udisp_data_handler(void *ctx, const uint8_t *req_buf, size_t len,
			int start, int end)
{
	struct f_loopback *loop = ctx;
	unsigned long flags;
	int remain = len, cur = 0;

	if (unlikely(!ctx || !req_buf || len == 0))
		return;

	while (remain > 0) {
		int read_res = min_t(int, remain, CONFIG_USB_VENDOR_RX_BUFSIZE);
		const uint8_t *rx_buf = &req_buf[cur];

		if (unlikely(start)) {
			udisp_frame_header_t fh;
			int payload_len;

			if (read_res < sizeof(fh)) {
				pr_err("%s frame header too small\n", __func__);
				break;
			}

			memcpy(&fh, rx_buf, sizeof(fh));
			start = 0;

			DEBUG_LOG(
				"rx frame: type:%u crc:%x x:%u y:%u w:%u h:%u total:%u",
				fh.type, fh.crc16, fh.x, fh.y, fh.width,
				fh.height, fh.payload_total);

			if (unlikely(fh.type > UDISP_TYPE_JPG)) {
				pr_err("%s invalid cmd type:%u\n", __func__,
				       fh.type);
				break;
			}

			payload_len = read_res - sizeof(fh);

			g_disp_frame_mgr.frame_hd = fh;
			g_disp_frame_mgr.x = fh.x;
			g_disp_frame_mgr.y = fh.y;
			g_disp_frame_mgr.x2 = fh.x + fh.width;
			g_disp_frame_mgr.y2 = fh.y + fh.height;
			g_disp_frame_mgr.y_idx = fh.y;
			g_disp_frame_mgr.disp_cnt = 0;
			g_disp_frame_mgr.rx_cnt = payload_len;
			g_disp_frame_mgr.done = 0;

			spin_lock_irqsave(&loop->con_lock, flags);
			kfifo_reset(&loop->con_buf);
			kfifo_in(&loop->con_buf, &rx_buf[sizeof(fh)],
				 payload_len);
			spin_unlock_irqrestore(&loop->con_lock, flags);
		} else {
			spin_lock_irqsave(&loop->con_lock, flags);
			g_disp_frame_mgr.rx_cnt += read_res;
			kfifo_in(&loop->con_buf, rx_buf, read_res);
			spin_unlock_irqrestore(&loop->con_lock, flags);
		}

		remain -= read_res;
		cur += read_res;
	}

	if (unlikely(end)) {
		push_a_frame_to_worker(ctx);
		wake_up_interruptible(&loop->wait_queue);
	}
}

static void loopback_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_loopback *loop;
	struct usb_composite_dev *cdev;
	int status;
	int urx_total;
	bool is_frame_end;

	if (unlikely(!req || !ep || !ep->driver_data)) {
		pr_err("%s: invalid ep or request\n", __func__);
		return;
	}

	loop = ep->driver_data;

	if (unlikely(!loop->function.config)) {
		pr_err("%s: invalid loop->function.config\n", __func__);
		goto free_req;
	}

	cdev = loop->function.config->cdev;
	status = req->status;

	switch (status) {
	case 0:
		if (likely(ep == loop->out_ep)) {
			urx_total = atomic_read(&loop->rx_total);
			is_frame_end = (req->actual < UDISP_BULK_BUFLEN);

			udisp_data_handler(loop, req->buf, req->actual,
					   urx_total == 0, is_frame_end);

			urx_total =
				atomic_add_return(req->actual, &loop->rx_total);

			if (is_frame_end) {
				put_fps_data(get_os_us());
				DEBUG_LOG("urx:%d|%d(%d) fps:%lu\n",
					  req->actual, req->length, urx_total,
					  get_fps());
				atomic_set(&loop->rx_total, 0);
			}
		} else {
			DEBUG_LOG("%s: tx complete %d/%d\n", __func__,
				  req->actual, req->length);

			req = req->context;
			ep = loop->out_ep;
		}

		status = usb_ep_queue(ep, req, GFP_ATOMIC);
		if (likely(status == 0))
			return;

		pr_err("Unable to queue buffer to %s: %d\n", ep->name, status);
		ERROR(cdev, "Unable to queue buffer to %s: %d\n", ep->name,
		      status);
		goto free_req;

	case -ECONNABORTED:
	case -ECONNRESET:
	case -ESHUTDOWN:
		DEBUG_LOG("%s: connection error %d\n", __func__, status);
		goto free_req;

	default:
		pr_err("%s: complete error %d, %d/%d\n", ep->name, status,
		       req->actual, req->length);
		ERROR(cdev, "%s loop complete --> %d, %d/%d\n", ep->name,
		      status, req->actual, req->length);
		goto free_req;
	}

free_req:
	if (req->context) {
		struct usb_ep *peer_ep =
			(ep == loop->in_ep) ? loop->out_ep : loop->in_ep;
		usb_ep_free_request(peer_ep, req->context);
		req->context = NULL;
	}
	free_ep_req(ep, req);
}

int rgb565_decode_rgb888x(uint32_t *framebuffer, const uint16_t *pix_msg, int x,
			  int y, int right, int bottom, int line_width)
{
	const int width = right - x + 1;
	const int height = bottom - y + 1;
	int row, col;
	uint32_t *dst;
	const uint16_t *src;

	if (unlikely(width <= 0 || height <= 0))
		return -EINVAL;

	dst = framebuffer + y * line_width + x;
	src = pix_msg;

	for (row = 0; row < height; ++row) {
		for (col = 0; col + 4 <= width; col += 4) {
			uint16_t p0 = src[0];
			uint16_t p1 = src[1];
			uint16_t p2 = src[2];
			uint16_t p3 = src[3];
			uint32_t r0 = (p0 >> 11) & 0x1F;
			uint32_t g0 = (p0 >> 5) & 0x3F;
			uint32_t b0 = p0 & 0x1F;
			uint32_t r1 = (p1 >> 11) & 0x1F;
			uint32_t g1 = (p1 >> 5) & 0x3F;
			uint32_t b1 = p1 & 0x1F;
			uint32_t r2 = (p2 >> 11) & 0x1F;
			uint32_t g2 = (p2 >> 5) & 0x3F;
			uint32_t b2 = p2 & 0x1F;
			uint32_t r3 = (p3 >> 11) & 0x1F;
			uint32_t g3 = (p3 >> 5) & 0x3F;
			uint32_t b3 = p3 & 0x1F;

			r0 = (r0 << 3) | (r0 >> 2);
			g0 = (g0 << 2) | (g0 >> 4);
			b0 = (b0 << 3) | (b0 >> 2);
			r1 = (r1 << 3) | (r1 >> 2);
			g1 = (g1 << 2) | (g1 >> 4);
			b1 = (b1 << 3) | (b1 >> 2);
			r2 = (r2 << 3) | (r2 >> 2);
			g2 = (g2 << 2) | (g2 >> 4);
			b2 = (b2 << 3) | (b2 >> 2);
			r3 = (r3 << 3) | (r3 >> 2);
			g3 = (g3 << 2) | (g3 >> 4);
			b3 = (b3 << 3) | (b3 >> 2);

			dst[col] = 0xFF000000 | (b0 << 16) | (g0 << 8) | r0;
			dst[col + 1] = 0xFF000000 | (b1 << 16) | (g1 << 8) | r1;
			dst[col + 2] = 0xFF000000 | (b2 << 16) | (g2 << 8) | r2;
			dst[col + 3] = 0xFF000000 | (b3 << 16) | (g3 << 8) | r3;
			src += 4;
		}

		for (; col < width; ++col) {
			uint16_t pix = *src++;
			uint32_t r = (pix >> 11) & 0x1F;
			uint32_t g = (pix >> 5) & 0x3F;
			uint32_t b = pix & 0x1F;

			r = (r << 3) | (r >> 2);
			g = (g << 2) | (g >> 4);
			b = (b << 3) | (b >> 2);

			dst[col] = 0xFF000000 | (b << 16) | (g << 8) | r;
		}

		dst += line_width;
	}

	return 0;
}

int draw_a_frame(void *ctx, udisp_frame_t *jf, void *scr)
{
	struct f_loopback *loop = ctx;
	void *src = jf->buf;
	void *dst = scr;
	u32 size = jf->hd.payload_total;

	switch (jf->hd.type) {
	case UDISP_TYPE_RGB888:
		memcpy(dst, src, size);
		break;
	case UDISP_TYPE_RGB565:
		rgb565_decode_rgb888x((u32 *)loop->rgb888x_buf.buf,
				      (const u16 *)src, 0, 0, 479, 479, 480);
		memcpy(dst, loop->rgb888x_buf.buf, size * 2);
		break;
	default:
		DEBUG_LOG("%s: unsupported type=%d\n", __func__, jf->hd.type);
		break;
	}

	return 0;
}

static int udisp_thread(void *data)
{
	struct f_loopback *loop = data;
	unsigned long flags;
	udisp_frame_t *jfr;
	int ret;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(
			loop->wait_queue, !list_empty(&loop->jpg_data_list) ||
						  kthread_should_stop());
		if (ret)
			continue;

		if (kthread_should_stop())
			break;

		spin_lock_irqsave(&loop->con_lock, flags);
		while (!list_empty(&loop->jpg_data_list)) {
			jfr = list_first_entry(&loop->jpg_data_list,
					       udisp_frame_t, list_node);
			list_del(&jfr->list_node);
			spin_unlock_irqrestore(&loop->con_lock, flags);

			draw_a_frame(loop, jfr, loop->screen_base);

			atomic64_inc(&loop->processed_frames);

			spin_lock_irqsave(&loop->con_lock, flags);
			list_add_tail(&jfr->list_node, &loop->jpg_free_list);
		}
		spin_unlock_irqrestore(&loop->con_lock, flags);
	}

	return 0;
}

static void disable_loopback(struct f_loopback *loop)
{
	struct usb_composite_dev *cdev;

	if (unlikely(!loop || !loop->function.config))
		return;

	cdev = loop->function.config->cdev;
	if (unlikely(!cdev))
		return;

	disable_endpoints(cdev, loop->in_ep, loop->out_ep, NULL, NULL);
}

static inline struct usb_request *lb_alloc_ep_req(struct usb_ep *ep, int len)
{
	return alloc_ep_req(ep, len);
}

static int alloc_requests(struct usb_composite_dev *cdev,
			  struct f_loopback *loop)
{
	struct usb_request *in_req, *out_req;
	int i;
	int result = 0;

	/*
	 * allocate a bunch of read buffers and queue them all at once.
	 * we buffer at most 'qlen' transfers; We allocate buffers only
	 * for out transfer and reuse them in IN transfers to implement
	 * our loopback functionality
	 */
	for (i = 0; i < loop->qlen && result == 0; i++) {
		result = -ENOMEM;

		in_req = usb_ep_alloc_request(loop->in_ep, GFP_ATOMIC);
		if (!in_req)
			goto fail;

		out_req = lb_alloc_ep_req(loop->out_ep, loop->buflen);
		if (!out_req)
			goto fail_in;

		in_req->complete = loopback_complete;
		out_req->complete = loopback_complete;

		in_req->buf = out_req->buf;
		/* length will be set in complete routine */
		in_req->context = out_req;
		out_req->context = in_req;

		result = usb_ep_queue(loop->out_ep, out_req, GFP_ATOMIC);
		if (result) {
			ERROR(cdev, "%s queue req --> %d\n", loop->out_ep->name,
			      result);
			goto fail_out;
		}
	}

	return 0;

fail_out:
	free_ep_req(loop->out_ep, out_req);
fail_in:
	usb_ep_free_request(loop->in_ep, in_req);
fail:
	return result;
}

static int enable_endpoint(struct usb_composite_dev *cdev,
			   struct f_loopback *loop, struct usb_ep *ep)
{
	int result;

	result = config_ep_by_speed(cdev->gadget, &(loop->function), ep);
	if (result)
		goto out;

	result = usb_ep_enable(ep);
	if (result < 0)
		goto out;
	ep->driver_data = loop;
	result = 0;

out:
	return result;
}

static int enable_loopback(struct usb_composite_dev *cdev,
			   struct f_loopback *loop)
{
	int result = 0;

	result = enable_endpoint(cdev, loop, loop->in_ep);
	if (result)
		goto out;

	result = enable_endpoint(cdev, loop, loop->out_ep);
	if (result)
		goto disable_in;

	result = alloc_requests(cdev, loop);
	if (result)
		goto disable_out;

	DBG(cdev, "%s enabled\n", loop->function.name);
	return 0;

disable_out:
	usb_ep_disable(loop->out_ep);
disable_in:
	usb_ep_disable(loop->in_ep);
out:
	return result;
}

static int loopback_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_loopback *loop = func_to_loop(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	/* we know alt is udisp */
	disable_loopback(loop);
	return enable_loopback(cdev, loop);
}

static void loopback_disable(struct usb_function *f)
{
	struct f_loopback *loop = func_to_loop(f);

	disable_loopback(loop);
}

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

static struct proc_dir_entry *proc_folder;
static struct proc_dir_entry *proc_file;
static struct f_loopback *gp_loop;

static ssize_t my_read(struct file *file, char __user *user_buffer,
		       size_t count, loff_t *offs)
{
	struct f_loopback *loop = gp_loop;
	udisp_frame_t *cur_jfr;
	unsigned long flags;
	unsigned long p = *offs;
	size_t to_copy, not_copied;
	int ret;

	if (unlikely(!loop))
		return -ENODEV;

	if (unlikely(!user_buffer || count == 0))
		return -EINVAL;

	DEBUG_LOG("%s: count=%zu, offs=%llu\n", __func__, count, *offs);
	cur_jfr = file->private_data;

	if (!cur_jfr) {
		spin_lock_irqsave(&loop->con_lock, flags);
		if (!list_empty(&loop->jpg_data_list)) {
			cur_jfr = list_first_entry(&loop->jpg_data_list,
						   udisp_frame_t, list_node);
			list_del(&cur_jfr->list_node);
			file->private_data = cur_jfr;
			DEBUG_LOG("%s: got frame, payload=%u\n", __func__,
				  cur_jfr->hd.payload_total);
		}
		spin_unlock_irqrestore(&loop->con_lock, flags);

		if (!cur_jfr) {
			ret = wait_event_interruptible_timeout(
				loop->wait_queue,
				!list_empty(&loop->jpg_data_list),
				msecs_to_jiffies(500));

			if (ret == 0) {
				DEBUG_LOG("%s: wait timeout\n", __func__);
				return -ETIMEDOUT;
			}
			if (ret < 0) {
				DEBUG_LOG("%s: wait interrupted\n", __func__);
				return -ERESTARTSYS;
			}

			spin_lock_irqsave(&loop->con_lock, flags);
			if (!list_empty(&loop->jpg_data_list)) {
				cur_jfr = list_first_entry(&loop->jpg_data_list,
							   udisp_frame_t,
							   list_node);
				list_del(&cur_jfr->list_node);
				file->private_data = cur_jfr;
			}
			spin_unlock_irqrestore(&loop->con_lock, flags);

			if (!cur_jfr)
				return -EAGAIN;
		}
	}

	if (p >= cur_jfr->hd.payload_total) {
		DEBUG_LOG("%s: EOF, releasing frame\n", __func__);
		spin_lock_irqsave(&loop->con_lock, flags);
		list_add_tail(&cur_jfr->list_node, &loop->jpg_free_list);
		spin_unlock_irqrestore(&loop->con_lock, flags);

		file->private_data = NULL;
		*offs = 0;
		return 0;
	}

	to_copy = min_t(size_t, count, cur_jfr->hd.payload_total - p);
	not_copied = copy_to_user(user_buffer, &cur_jfr->buf[p], to_copy);
	if (not_copied) {
		DEBUG_LOG("%s: copy_to_user failed, not_copied=%zu\n", __func__,
			  not_copied);
		return -EFAULT;
	}

	*offs += to_copy;

	DEBUG_LOG("%s: copied %zu bytes, new_offs=%llu\n", __func__, to_copy,
		  *offs);

	return to_copy;
}

static ssize_t my_write(struct file *file, const char __user *user_buffer,
			size_t count, loff_t *offs)
{
	return 0;
}

static int my_open(struct inode *inode, struct file *file)
{
	struct f_loopback *loop = gp_loop;

	if (!loop)
		return -ENODEV;

	if (atomic_cmpxchg(&loop->open_flag, 0, 1) != 0) {
		DEBUG_LOG("%s: device busy\n", __func__);
		return -EBUSY;
	}

	file->private_data = NULL;
	DEBUG_LOG("%s: opened by pid=%d\n", __func__, current->pid);
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	struct f_loopback *loop = gp_loop;
	udisp_frame_t *cur_jfr = file->private_data;
	unsigned long flags;

	if (cur_jfr && loop) {
		DEBUG_LOG("%s: releasing unfinished frame\n", __func__);
		spin_lock_irqsave(&loop->con_lock, flags);
		list_add_tail(&cur_jfr->list_node, &loop->jpg_free_list);
		spin_unlock_irqrestore(&loop->con_lock, flags);
	}

	if (loop)
		atomic_set(&loop->open_flag, 0);

	file->private_data = NULL;
	DEBUG_LOG("%s: closed by pid=%d\n", __func__, current->pid);
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_release,
};

static struct proc_dir_entry *proc_info_file;

static int udisp_info_show(struct seq_file *m, void *v)
{
	struct f_loopback *loop = gp_loop;
	unsigned long flags;
	size_t free_count, data_count;
	long long total_frames, total_bytes, dropped, processed;
	int rx_current;
	long fps;

	if (!loop) {
		seq_printf(m, "Device not initialized\n");
		return 0;
	}

	spin_lock_irqsave(&loop->con_lock, flags);
	free_count = _list_count_nodes(&loop->jpg_free_list);
	data_count = _list_count_nodes(&loop->jpg_data_list);
	spin_unlock_irqrestore(&loop->con_lock, flags);

	total_frames = atomic64_read(&loop->total_frames);
	total_bytes = atomic64_read(&loop->total_bytes);
	dropped = atomic64_read(&loop->dropped_frames);
	processed = atomic64_read(&loop->processed_frames);
	rx_current = atomic_read(&loop->rx_total);
	fps = get_fps();

	seq_printf(m, "USB Display Device Status\n");
	seq_printf(m, "==========================\n\n");
	seq_printf(m, "[Frame Buffers]\n");
	seq_printf(m, "  Free frames:    %zu / %d\n", free_count,
		   JPG_FRAME_MAX);
	seq_printf(m, "  Pending frames: %zu\n\n", data_count);

	seq_printf(m, "[Statistics]\n");
	seq_printf(m, "  Total frames RX:  %lld\n", total_frames);

	if (total_bytes >= 1024LL * 1024 * 1024) {
		long long gb = total_bytes / (1024LL * 1024 * 1024);
		long long mb =
			(total_bytes % (1024LL * 1024 * 1024)) / (1024 * 1024);
		seq_printf(m, "  Total bytes RX:   %lld (%lld.%03lld GB)\n",
			   total_bytes, gb, mb * 1000 / 1024);
	} else if (total_bytes >= 1024 * 1024) {
		long long mb = total_bytes / (1024 * 1024);
		long long kb = (total_bytes % (1024 * 1024)) / 1024;
		seq_printf(m, "  Total bytes RX:   %lld (%lld.%03lld MB)\n",
			   total_bytes, mb, kb * 1000 / 1024);
	} else if (total_bytes >= 1024) {
		long long kb = total_bytes / 1024;
		seq_printf(m, "  Total bytes RX:   %lld (%lld KB)\n",
			   total_bytes, kb);
	} else {
		seq_printf(m, "  Total bytes RX:   %lld bytes\n", total_bytes);
	}

	seq_printf(m, "  Frames processed: %lld\n", processed);
	seq_printf(m, "  Frames dropped:   %lld\n", dropped);
	seq_printf(m, "  Current RX bytes: %d\n\n", rx_current);

	seq_printf(m, "[Performance]\n");
	if (fps >= 0)
		seq_printf(m, "  Current FPS:      %ld.%ld\n", fps / 10,
			   fps % 10);
	else
		seq_printf(m, "  Current FPS:      N/A\n");

	if (total_frames > 0) {
		long long avg_frame_size = total_bytes / total_frames;
		long long total = total_frames + dropped;

		seq_printf(m, "  Avg frame size:   %lld bytes\n",
			   avg_frame_size);

		if (total > 0) {
			long long drop_rate_x100 = (dropped * 10000) / total;
			seq_printf(m, "  Drop rate:        %lld.%02lld%%\n\n",
				   drop_rate_x100 / 100, drop_rate_x100 % 100);
		} else {
			seq_printf(m, "  Drop rate:        0.00%%\n\n");
		}
	} else {
		seq_printf(m, "  Avg frame size:   N/A\n");
		seq_printf(m, "  Drop rate:        N/A\n\n");
	}

	seq_printf(m, "[Configuration]\n");
	seq_printf(m, "  Buffer size:      %d bytes\n", UDISP_BUF_SIZE);
	seq_printf(m, "  Queue length:     %u\n", loop->qlen);
	seq_printf(m, "  Worker thread:    %s\n",
		   loop->work_thread ? "running" : "stopped");
	seq_printf(m, "  Proc file open:   %s\n",
		   atomic_read(&loop->open_flag) ? "yes" : "no");

	return 0;
}

static int udisp_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, udisp_info_show, NULL);
}

static const struct file_operations info_fops = {
	.owner = THIS_MODULE,
	.open = udisp_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int my_procfs_init(void)
{
	int ret;

	if (proc_folder || proc_file) {
		pr_warn("%s: already initialized\n", __func__);
		return 0;
	}

	DEBUG_LOG("%s: creating /proc/udisp\n", __func__);

	proc_folder = proc_mkdir("udisp", NULL);
	if (!proc_folder) {
		pr_err("%s: failed to create /proc/udisp\n", __func__);
		return -ENOMEM;
	}

	proc_file = proc_create("nanokvm", 0644, proc_folder, &fops);
	if (!proc_file) {
		pr_err("%s: failed to create /proc/udisp/nanokvm\n", __func__);
		ret = -ENOMEM;
		goto err_remove_folder;
	}

	proc_info_file = proc_create("info", 0644, proc_folder, &info_fops);
	if (!proc_info_file) {
		pr_warn("%s: failed to create /proc/udisp/info\n", __func__);
	}

	DEBUG_LOG("%s: created /proc/udisp/nanokvm\n", __func__);
	return 0;

err_remove_folder:
	proc_remove(proc_folder);
	proc_folder = NULL;
	return ret;
}

static void my_procfs_exit(void)
{
	DEBUG_LOG("%s: removing /proc/udisp\n", __func__);

	if (proc_info_file) {
		proc_remove(proc_info_file);
		proc_info_file = NULL;
	}

	if (proc_file) {
		proc_remove(proc_file);
		proc_file = NULL;
	}

	if (proc_folder) {
		proc_remove(proc_folder);
		proc_folder = NULL;
	}
}

static struct usb_function *loopback_alloc(struct usb_function_instance *fi)
{
	struct f_loopback *loop;
	struct f_lb_opts *lb_opts;
	int status;
	int i;

	loop = kzalloc(sizeof(*loop), GFP_KERNEL);
	if (!loop)
		return ERR_PTR(-ENOMEM);

	lb_opts = container_of(fi, struct f_lb_opts, func_inst);

	mutex_lock(&lb_opts->lock);
	lb_opts->refcnt++;
	mutex_unlock(&lb_opts->lock);

	loop->buflen = lb_opts->bulk_buflen;
	loop->qlen = lb_opts->qlen;
	if (!loop->qlen)
		loop->qlen = 32;

	loop->function.name = "loopback";
	loop->function.bind = loopback_bind;
	loop->function.set_alt = loopback_set_alt;
	loop->function.disable = loopback_disable;
	loop->function.strings = loopback_strings;
	loop->function.free_func = lb_free_func;

	spin_lock_init(&loop->con_lock);

	status = kfifo_alloc(&loop->con_buf, UDISP_BUF_SIZE, GFP_KERNEL);
	if (status) {
		pr_err("%s: failed to allocate con_buf\n", __func__);
		goto err_free_loop;
	}

	status = kfifo_alloc(&loop->jpg_buf, UDISP_BUF_SIZE, GFP_KERNEL);
	if (status) {
		pr_err("%s: failed to allocate jpg_buf\n", __func__);
		goto err_free_con_buf;
	}

	atomic_set(&loop->jpg_atom_cnt, 1);
	atomic_set(&loop->rx_total, 0);
	atomic_set(&loop->open_flag, 0);
	atomic64_set(&loop->total_frames, 0);
	atomic64_set(&loop->total_bytes, 0);
	atomic64_set(&loop->dropped_frames, 0);
	atomic64_set(&loop->processed_frames, 0);

	init_waitqueue_head(&loop->wait_queue);

	loop->work_thread = kthread_create(udisp_thread, loop, "udisp_thread");
	if (IS_ERR(loop->work_thread)) {
		pr_err("%s: failed to create udisp thread\n", __func__);
		status = PTR_ERR(loop->work_thread);
		goto err_free_jpg_buf;
	}

	INIT_LIST_HEAD(&loop->jpg_free_list);
	INIT_LIST_HEAD(&loop->jpg_data_list);

	for (i = 0; i < JPG_FRAME_MAX; i++) {
		list_add_tail(&loop->jpg_tb[i].list_node, &loop->jpg_free_list);
	}

	gp_loop = loop;

	status = my_procfs_init();
	if (status) {
		pr_warn("%s: procfs init failed (non-fatal)\n", __func__);
	}

	lb_opts->ctx = loop;
	wake_up_process(loop->work_thread);

	pr_info("%s: initialized successfully\n", __func__);
	return &loop->function;

err_free_jpg_buf:
	kfifo_free(&loop->jpg_buf);
err_free_con_buf:
	kfifo_free(&loop->con_buf);
err_free_loop:
	kfree(loop);
	return ERR_PTR(status);
}

static inline struct f_lb_opts *to_f_lb_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_lb_opts,
			    func_inst.group);
}

static void lb_attr_release(struct config_item *item)
{
	struct f_lb_opts *lb_opts = to_f_lb_opts(item);

	usb_put_function_instance(&lb_opts->func_inst);
}

static struct configfs_item_operations lb_item_ops = {
	.release = lb_attr_release,
};

static ssize_t f_lb_opts_qlen_show(struct config_item *item, char *page)
{
	struct f_lb_opts *opts = to_f_lb_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf(page, "%d\n", opts->qlen);
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_lb_opts_qlen_store(struct config_item *item, const char *page,
				    size_t len)
{
	struct f_lb_opts *opts = to_f_lb_opts(item);
	int ret;
	u32 num;

	mutex_lock(&opts->lock);
	if (opts->refcnt) {
		ret = -EBUSY;
		goto end;
	}

	ret = kstrtou32(page, 0, &num);
	if (ret)
		goto end;

	opts->qlen = num;
	ret = len;
end:
	mutex_unlock(&opts->lock);
	return ret;
}

CONFIGFS_ATTR(f_lb_opts_, qlen);

static ssize_t f_lb_opts_bulk_buflen_show(struct config_item *item, char *page)
{
	struct f_lb_opts *opts = to_f_lb_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf(page, "%d\n", opts->bulk_buflen);
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_lb_opts_bulk_buflen_store(struct config_item *item,
					   const char *page, size_t len)
{
	struct f_lb_opts *opts = to_f_lb_opts(item);
	int ret;
	u32 num;

	mutex_lock(&opts->lock);
	if (opts->refcnt) {
		ret = -EBUSY;
		goto end;
	}

	ret = kstrtou32(page, 0, &num);
	if (ret)
		goto end;

	opts->bulk_buflen = num;
	ret = len;
end:
	mutex_unlock(&opts->lock);
	return ret;
}

CONFIGFS_ATTR(f_lb_opts_, bulk_buflen);

static struct configfs_attribute *lb_attrs[] = {
	&f_lb_opts_attr_qlen,
	&f_lb_opts_attr_bulk_buflen,
	NULL,
};

static const struct config_item_type lb_func_type = {
	.ct_item_ops = &lb_item_ops,
	.ct_attrs = lb_attrs,
	.ct_owner = THIS_MODULE,
};

static void lb_free_instance(struct usb_function_instance *fi)
{
	struct f_lb_opts *lb_opts;
	struct f_loopback *loop;

	lb_opts = container_of(fi, struct f_lb_opts, func_inst);
	loop = lb_opts->ctx;

	if (!loop) {
		pr_warn("%s: loop context is NULL\n", __func__);
		goto free_opts;
	}

	DEBUG_LOG("%s: cleaning up resources\n", __func__);

	if (!IS_ERR_OR_NULL(loop->work_thread)) {
		kthread_stop(loop->work_thread);
		loop->work_thread = NULL;
	}

	if (loop->filp) {
		DEBUG_LOG("%s: closing framebuffer\n", __func__);
		filp_close(loop->filp, NULL);
		loop->filp = NULL;
	}

	my_procfs_exit();

	kfifo_free(&loop->con_buf);
	kfifo_free(&loop->jpg_buf);

	if (gp_loop == loop)
		gp_loop = NULL;

free_opts:
	kfree(lb_opts);
}

static struct usb_function_instance *loopback_alloc_instance(void)
{
	struct f_lb_opts *lb_opts;

	lb_opts = kzalloc(sizeof(*lb_opts), GFP_KERNEL);
	if (!lb_opts)
		return ERR_PTR(-ENOMEM);
	mutex_init(&lb_opts->lock);
	lb_opts->func_inst.free_func_inst = lb_free_instance;
	lb_opts->bulk_buflen = UDISP_BULK_BUFLEN;
	lb_opts->qlen = UDISP_QLEN;

	config_group_init_type_name(&lb_opts->func_inst.group, "",
				    &lb_func_type);

	return &lb_opts->func_inst;
}
DECLARE_USB_FUNCTION(Loopback, loopback_alloc_instance, loopback_alloc);

int __init lb_modinit(void)
{
	return usb_function_register(&Loopbackusb_func);
}

void __exit lb_modexit(void)
{
	usb_function_unregister(&Loopbackusb_func);
}

MODULE_LICENSE("GPL");
// ### SIPEED EDIT END ###
