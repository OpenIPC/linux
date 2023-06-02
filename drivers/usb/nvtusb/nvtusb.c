
/*
 *  Copyright (c) 2014 Novatek
 *
 *  NVT USB Host Test
 */

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/usb.h>

#include <linux/pm_runtime.h>
//#include <../host/xhci.h>
//#include <linux/usb/input.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.0"
#define DRIVER_AUTHOR "Klins Chen <klins_chen@novatek.com.tw>"
#define DRIVER_DESC "Novatek USB test program"
#define DRIVER_LICENSE "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

struct usb_nvttst {
	char name[128];

	struct usb_interface 	*inf;
	struct usb_device 		*usbdev;
	struct task_struct 		*ctl_thread;

	struct urb 			*urb1,*urb2,*urb3;
	struct completion	done1,done2;
	struct completion	close;

	//Cx Resource
	struct task_struct 	*cx_thread;
	struct urb 			*cx_urb_in,*cx_urb_out;
	struct completion	cx_done_in,cx_done_out;
	struct completion	cx_close;

	//BULK Resource
	struct task_struct 	*bulk_thread;
	struct urb 			*bulk_urb_in,*bulk_urb_out;
	struct completion	bulk_done_in,bulk_done_out;
	struct completion	bulk_close;

	//Interrupt Resource
	struct task_struct 	*intr_thread;
	struct urb 			*intr_urb_in,*intr_urb_out;
	struct completion	intr_done_in,intr_done_out;
	struct completion	intr_close;

	//ISO Resource
	struct task_struct 	*isoin_thread;
	struct task_struct 	*isoout_thread;
	struct urb 			*iso_urb_in,*iso_urb_in2,*iso_urb_out;
	struct completion	iso_done_in,iso_done_in2,iso_done_out;
	struct completion	isoin_close,isoout_close;

	// ISO UVC
	struct task_struct 	*isouvc1_thread;
	struct urb 			*iso_urb_uvc11,*iso_urb_uvc12;
	struct completion	 iso_done_uvc11,iso_done_uvc12;
	struct completion	 isoin_close_uvc1;

	struct task_struct 	*isouvc2_thread;
	struct urb 			*iso_urb_uvc21,*iso_urb_uvc22;
	struct completion	 iso_done_uvc21,iso_done_uvc22;
	struct completion	 isoin_close_uvc2;


	int			status;
	int			msgoff;
};

#ifdef CONFIG_PM
extern int usb_suspend(struct device *dev, pm_message_t msg);
extern int usb_resume(struct device *dev, pm_message_t msg);
extern int usb_suspend_both(struct usb_device *udev, pm_message_t msg);
#endif

#if 0
void usb_nvttst_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);


	if((urb == nvttst->urb1)||(urb == nvttst->urb3))
		complete(&nvttst->done1);
	else if (urb == nvttst->urb2)
		complete(&nvttst->done2);
}
#endif

#if 1
static void usb_nvttst_cxin_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->cx_done_in);
}
static void usb_nvttst_cxout_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->cx_done_out);
}

static void usb_nvttst_bulkin_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->bulk_done_in);
}
static void usb_nvttst_bulkout_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->bulk_done_out);
}

static void usb_nvttst_intrin_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->intr_done_in);
}
static void usb_nvttst_introut_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->intr_done_out);
}


static void usb_nvttst_isoin_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_in);
}

#if 0
static void usb_nvttst_isoin_blocking_completion2(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_in2);
}
static void usb_nvttst_isouvc11_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_uvc11);
}

static void usb_nvttst_isouvc12_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_uvc12);
}

static void usb_nvttst_isouvc21_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_uvc21);
}

static void usb_nvttst_isouvc22_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_uvc22);
}
#endif
static void usb_nvttst_isoout_blocking_completion(struct urb *urb)
{
	struct usb_nvttst *nvttst = urb->context;

	nvttst->status = urb->status;
	if(nvttst->status)
		printk("[%s]: err sts = %d\n",__func__,nvttst->status);

	complete(&nvttst->iso_done_out);
}

#endif



#if 1

#if 0
static int usb_nvttst_issue_urb_blocking(struct urb *urb, struct usb_nvttst *nvttst)
{
	int 	ret;

	if((urb == nvttst->urb1)||(urb == nvttst->urb3))
		init_completion(&nvttst->done1);
	else if(urb == nvttst->urb2)
		init_completion(&nvttst->done2);


	urb->actual_length = 0;
	ret = usb_submit_urb(urb, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if((urb == nvttst->urb1)||(urb == nvttst->urb3)) {
		if (!wait_for_completion_timeout(&nvttst->done1, msecs_to_jiffies(5000))) {
			printk("[%s]: timeout",__func__);
			return -ETIMEDOUT;
		}
	}
	else if(urb == nvttst->urb2) {
		if (!wait_for_completion_timeout(&nvttst->done2, msecs_to_jiffies(5000))) {
			printk("[%s]: timeout",__func__);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int usb_nvttst_issue_urb_nonblocking(struct urb *urb, struct usb_nvttst *nvttst)
{
	int 	ret;

	if(urb == nvttst->urb1)
		init_completion(&nvttst->done1);
	else if(urb == nvttst->urb2)
		init_completion(&nvttst->done2);

	urb->actual_length = 0;
	ret = usb_submit_urb(urb, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	return 0;
}

static int usb_nvttst_nonblocking_waitdone(struct urb *urb, struct usb_nvttst *nvttst)
{
	if(urb == nvttst->urb1) {
		if (!wait_for_completion_timeout(&nvttst->done1, msecs_to_jiffies(5000))) {
			printk("[%s]: timeout",__func__);
			return -ETIMEDOUT;
		}
	}
	else if(urb == nvttst->urb2) {
		if (!wait_for_completion_timeout(&nvttst->done2, msecs_to_jiffies(5000))) {
			printk("[%s]: timeout",__func__);
			return -ETIMEDOUT;
		}
	}

	return 0;
}
#endif

static void usb_nvttst_setup_CxIN(struct usb_nvttst *nvttst, struct usb_ctrlrequest *req, void *buf, size_t length)
{
	req->bRequestType = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	req->bRequest     = 0x00;
	req->wValue       = 0x0000;
	req->wIndex       = 0x0000;
	req->wLength      = length;

	usb_fill_control_urb(nvttst->cx_urb_in,
			     nvttst->usbdev,
			     usb_rcvctrlpipe(nvttst->usbdev, 0),
			     (unsigned char *) req,
			     (void *)buf,
			     length,
			     usb_nvttst_cxin_blocking_completion,
			     nvttst);
}

static void usb_nvttst_setup_CxOUT(struct usb_nvttst *nvttst, struct usb_ctrlrequest *req, void *buf, size_t length)
{
	req->bRequestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	req->bRequest     = 0x00;
	req->wValue       = 0x0000;
	req->wIndex       = 0x0000;
	req->wLength      = length;

	usb_fill_control_urb(nvttst->cx_urb_out,
			     nvttst->usbdev,
			     usb_sndctrlpipe(nvttst->usbdev, 0),
			     (unsigned char *) req,
			     (void *)buf,
			     length,
			     usb_nvttst_cxout_blocking_completion,
			     nvttst);
}

static void usb_nvttst_setup_BulkIN(struct usb_nvttst *nvttst, void *buf, size_t length,int epn)
{
	usb_fill_bulk_urb(nvttst->bulk_urb_in,
		          nvttst->usbdev,
		          usb_rcvbulkpipe(nvttst->usbdev, epn),
		          (void *)buf,
		          length,
		          usb_nvttst_bulkin_blocking_completion,
		          nvttst);
}

static void usb_nvttst_setup_BulkOUT(struct usb_nvttst *nvttst, void *buf, size_t length,int epn)
{

	usb_fill_bulk_urb(nvttst->bulk_urb_out,
		          nvttst->usbdev,
		          usb_sndbulkpipe(nvttst->usbdev, epn),
		          (void *)buf,
		          length,
		          usb_nvttst_bulkout_blocking_completion,
		          nvttst);
}

static void usb_nvttst_setup_IntrIN(struct usb_nvttst *nvttst, void *buf, size_t length)
{
	usb_fill_int_urb(nvttst->intr_urb_in,
		          nvttst->usbdev,
		          usb_rcvintpipe(nvttst->usbdev, 3),
		          (void *)buf,
		          length,
		          usb_nvttst_intrin_blocking_completion,
		          nvttst,
		          1);
}

static void usb_nvttst_setup_IntrOUT(struct usb_nvttst *nvttst, void *buf, size_t length)
{

	usb_fill_int_urb(nvttst->intr_urb_out,
		          nvttst->usbdev,
		          usb_sndintpipe(nvttst->usbdev, 4),
		          (void *)buf,
		          length,
		          usb_nvttst_introut_blocking_completion,
		          nvttst,
		          1);
}

static void usb_nvttst_setup_IsoIN(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_in->dev = nvttst->usbdev;
	nvttst->iso_urb_in->pipe= usb_rcvisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_in->context = nvttst;
	nvttst->iso_urb_in->interval= 1;
	nvttst->iso_urb_in->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_in->transfer_buffer= buf;
	nvttst->iso_urb_in->complete = usb_nvttst_isoin_blocking_completion;
	nvttst->iso_urb_in->number_of_packets = pktno;
	nvttst->iso_urb_in->transfer_buffer_length= pktno*length;
	nvttst->iso_urb_in->actual_length= 0;

	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_in->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_in->iso_frame_desc[i].length = length;
		nvttst->iso_urb_in->iso_frame_desc[i].actual_length = 0;
	}

}

#if 0
static void usb_nvttst_setup_IsoIN2(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_in2->dev = nvttst->usbdev;
	nvttst->iso_urb_in2->pipe= usb_rcvisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_in2->context = nvttst;
	nvttst->iso_urb_in2->interval= 1;
	nvttst->iso_urb_in2->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_in2->transfer_buffer= buf;
	nvttst->iso_urb_in2->complete = usb_nvttst_isoin_blocking_completion2;
	nvttst->iso_urb_in2->number_of_packets = pktno;
	nvttst->iso_urb_in2->transfer_buffer_length= pktno*length;
	nvttst->iso_urb_in2->actual_length= 0;

	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_in2->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_in2->iso_frame_desc[i].length = length;
		nvttst->iso_urb_in2->iso_frame_desc[i].actual_length = 0;
	}

}

static void usb_nvttst_setup_uvc11(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_uvc11->dev = nvttst->usbdev;
	nvttst->iso_urb_uvc11->pipe= usb_rcvisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_uvc11->context = nvttst;
	nvttst->iso_urb_uvc11->interval= 1;
	nvttst->iso_urb_uvc11->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_uvc11->transfer_buffer= buf;
	nvttst->iso_urb_uvc11->complete = usb_nvttst_isouvc11_blocking_completion;
	nvttst->iso_urb_uvc11->number_of_packets = pktno;
	nvttst->iso_urb_uvc11->transfer_buffer_length= pktno*length;
	nvttst->iso_urb_uvc11->actual_length= 0;

	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_uvc11->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_uvc11->iso_frame_desc[i].length = length;
		nvttst->iso_urb_uvc11->iso_frame_desc[i].actual_length = 0;
	}

}

static void usb_nvttst_setup_uvc12(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_uvc12->dev = nvttst->usbdev;
	nvttst->iso_urb_uvc12->pipe= usb_rcvisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_uvc12->context = nvttst;
	nvttst->iso_urb_uvc12->interval= 1;
	nvttst->iso_urb_uvc12->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_uvc12->transfer_buffer= buf;
	nvttst->iso_urb_uvc12->complete = usb_nvttst_isouvc12_blocking_completion;
	nvttst->iso_urb_uvc12->number_of_packets = pktno;
	nvttst->iso_urb_uvc12->transfer_buffer_length= pktno*length;
	nvttst->iso_urb_uvc12->actual_length= 0;

	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_uvc12->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_uvc12->iso_frame_desc[i].length = length;
		nvttst->iso_urb_uvc12->iso_frame_desc[i].actual_length = 0;
	}

}

static void usb_nvttst_setup_uvc21(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_uvc21->dev = nvttst->usbdev;
	nvttst->iso_urb_uvc21->pipe= usb_rcvisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_uvc21->context = nvttst;
	nvttst->iso_urb_uvc21->interval= 1;
	nvttst->iso_urb_uvc21->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_uvc21->transfer_buffer= buf;
	nvttst->iso_urb_uvc21->complete = usb_nvttst_isouvc21_blocking_completion;
	nvttst->iso_urb_uvc21->number_of_packets = pktno;
	nvttst->iso_urb_uvc21->transfer_buffer_length= pktno*length;
	nvttst->iso_urb_uvc21->actual_length= 0;

	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_uvc21->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_uvc21->iso_frame_desc[i].length = length;
		nvttst->iso_urb_uvc21->iso_frame_desc[i].actual_length = 0;
	}

}

static void usb_nvttst_setup_uvc22(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_uvc22->dev = nvttst->usbdev;
	nvttst->iso_urb_uvc22->pipe= usb_rcvisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_uvc22->context = nvttst;
	nvttst->iso_urb_uvc22->interval= 1;
	nvttst->iso_urb_uvc22->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_uvc22->transfer_buffer= buf;
	nvttst->iso_urb_uvc22->complete = usb_nvttst_isouvc22_blocking_completion;
	nvttst->iso_urb_uvc22->number_of_packets = pktno;
	nvttst->iso_urb_uvc22->transfer_buffer_length= pktno*length;
	nvttst->iso_urb_uvc22->actual_length= 0;

	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_uvc22->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_uvc22->iso_frame_desc[i].length = length;
		nvttst->iso_urb_uvc22->iso_frame_desc[i].actual_length = 0;
	}

}
#endif

static void usb_nvttst_setup_IsoOUT(struct usb_nvttst *nvttst, void *buf, size_t length, size_t pktno, size_t ofs, int epn)
{
	int i;

	nvttst->iso_urb_out->dev = nvttst->usbdev;
	nvttst->iso_urb_out->pipe= usb_sndisocpipe(nvttst->usbdev, epn);
	nvttst->iso_urb_out->context = nvttst;
	nvttst->iso_urb_out->interval= 1;
	nvttst->iso_urb_out->transfer_flags = URB_ISO_ASAP;
	nvttst->iso_urb_out->transfer_buffer= buf;
	nvttst->iso_urb_out->complete = usb_nvttst_isoout_blocking_completion;
	nvttst->iso_urb_out->number_of_packets = pktno;
	nvttst->iso_urb_out->transfer_buffer_length= length*pktno;
	for(i=0;i<pktno;i++) {
		nvttst->iso_urb_out->iso_frame_desc[i].offset = (length*i)+ofs;
		nvttst->iso_urb_out->iso_frame_desc[i].length = length;
		nvttst->iso_urb_out->iso_frame_desc[i].actual_length = 0;
	}

}
#endif

#if 1

static int usb_nvttst_cxin_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->cx_done_in);

	nvttst->cx_urb_in->actual_length = 0;
	ret = usb_submit_urb(nvttst->cx_urb_in, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->cx_done_in, msecs_to_jiffies(8000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int usb_nvttst_cxout_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->cx_done_out);

	nvttst->cx_urb_out->actual_length = 0;
	ret = usb_submit_urb(nvttst->cx_urb_out, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->cx_done_out, msecs_to_jiffies(8000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}




static int usb_nvttst_bulkin_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->bulk_done_in);

	nvttst->bulk_urb_in->actual_length = 0;
	ret = usb_submit_urb(nvttst->bulk_urb_in, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->bulk_done_in, msecs_to_jiffies(8000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int usb_nvttst_bulkout_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->bulk_done_out);

	nvttst->bulk_urb_out->actual_length = 0;
	ret = usb_submit_urb(nvttst->bulk_urb_out, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->bulk_done_out, msecs_to_jiffies(8000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}


static int usb_nvttst_intrin_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->intr_done_in);

	nvttst->intr_urb_in->actual_length = 0;
	ret = usb_submit_urb(nvttst->intr_urb_in, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->intr_done_in, msecs_to_jiffies(8000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int usb_nvttst_introut_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->intr_done_out);

	nvttst->intr_urb_out->actual_length = 0;
	ret = usb_submit_urb(nvttst->intr_urb_out, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->intr_done_out, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

/*
static int usb_nvttst_isoin_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_in);

	nvttst->iso_urb_in->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_in, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}
*/

static int usb_nvttst_isoin_issue_urb_nonblocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_in);

	nvttst->iso_urb_in->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_in, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	//if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
	//	printk("[%s]: timeout",__func__);
	//	return -ETIMEDOUT;
	//}

	return 0;
}


static int usb_nvttst_isoin_issue_urb_waitdone(struct usb_nvttst *nvttst)
{
	if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

#if 0
static int usb_nvttst_isoin2_issue_urb_nonblocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_in2);

	nvttst->iso_urb_in2->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_in2, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	//if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
	//	printk("[%s]: timeout",__func__);
	//	return -ETIMEDOUT;
	//}

	return 0;
}

static int usb_nvttst_isoin2_issue_urb_waitdone(struct usb_nvttst *nvttst)
{
	if (!wait_for_completion_timeout(&nvttst->iso_done_in2, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}
#endif


static int usb_nvttst_isoout_issue_urb_blocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_out);

	nvttst->iso_urb_out->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_out, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&nvttst->iso_done_out, msecs_to_jiffies(2000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

#if 0

static int usb_nvttst_isouvc11_issue_urb_nonblocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_uvc11);

	nvttst->iso_urb_uvc11->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_uvc11, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	//if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
	//	printk("[%s]: timeout",__func__);
	//	return -ETIMEDOUT;
	//}

	return 0;
}
static int usb_nvttst_isouvc12_issue_urb_nonblocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_uvc12);

	nvttst->iso_urb_uvc12->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_uvc12, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	//if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
	//	printk("[%s]: timeout",__func__);
	//	return -ETIMEDOUT;
	//}

	return 0;
}
static int usb_nvttst_isouvc11_issue_urb_waitdone(struct usb_nvttst *nvttst)
{
	if (!wait_for_completion_timeout(&nvttst->iso_done_uvc11, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}
static int usb_nvttst_isouvc12_issue_urb_waitdone(struct usb_nvttst *nvttst)
{
	if (!wait_for_completion_timeout(&nvttst->iso_done_uvc12, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int usb_nvttst_isouvc21_issue_urb_nonblocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_uvc21);

	nvttst->iso_urb_uvc21->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_uvc21, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	//if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
	//	printk("[%s]: timeout",__func__);
	//	return -ETIMEDOUT;
	//}

	return 0;
}
static int usb_nvttst_isouvc22_issue_urb_nonblocking(struct usb_nvttst *nvttst)
{
	int 	ret;

	init_completion(&nvttst->iso_done_uvc22);

	nvttst->iso_urb_uvc22->actual_length = 0;
	ret = usb_submit_urb(nvttst->iso_urb_uvc22, GFP_KERNEL);
	if( ret ) {
		printk("[%s]: usb_submit_urb failed %d\n",__func__,ret);
		return ret;
	}

	//if (!wait_for_completion_timeout(&nvttst->iso_done_in, msecs_to_jiffies(2000))) {
	//	printk("[%s]: timeout",__func__);
	//	return -ETIMEDOUT;
	//}

	return 0;
}
static int usb_nvttst_isouvc21_issue_urb_waitdone(struct usb_nvttst *nvttst)
{
	if (!wait_for_completion_timeout(&nvttst->iso_done_uvc21, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}
static int usb_nvttst_isouvc22_issue_urb_waitdone(struct usb_nvttst *nvttst)
{
	if (!wait_for_completion_timeout(&nvttst->iso_done_uvc22, msecs_to_jiffies(5000))) {
		printk("[%s]: timeout",__func__);
		return -ETIMEDOUT;
	}

	return 0;
}
#endif


#endif
#if 1

void usb_nvttst_fill_cbw(u8 *cbwbuf, u32 len, bool OUT)
{
	static u32 Tag = 0x88888888;

	cbwbuf[0] = 0x55;	cbwbuf[1] = 0x53;	cbwbuf[2] = 0x42;	cbwbuf[3] = 0x43;
	cbwbuf[4] =(Tag>>0)&0xFF;cbwbuf[5]=(Tag>>8)&0xFF;cbwbuf[6]=(Tag>>16)&0xFF;cbwbuf[7]=(Tag>>24)&0xFF;
	cbwbuf[8] =(len>>0)&0xFF;cbwbuf[9]=(len>>8)&0xFF;cbwbuf[10]=(len>>16)&0xFF;cbwbuf[11]=(len>>24)&0xFF;

	if(OUT)
		cbwbuf[12]=0x00;//OUT
	else
		cbwbuf[12]=0x80;

	Tag++;
}

#endif
#if 0
static int usb_nvttst_control_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	struct usb_ctrlrequest    ctrlreq1,ctrlreq2;
	u8 	*txfbuf,inCnter=0;
	u8 	*rxfbuf,outCnter=0;
	u8 	*cbwbuf,*cswbuf;
	size_t	i,len1=1,len2=1,bulksize=0;
	int 	ret,Cnt=0;
	bool	bulk=0,intrr=0,isoin=0,isoout=0;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;


	// Check if bulk/interrupt/isochronous test
	if(intf->desc.bNumEndpoints == 2)
	{
		if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
		 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x81)
		 &&((intf->endpoint[1].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
		 &&((intf->endpoint[1].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x02)) {
			printk("Bulk desc matched. Start bulk testing...\n");
			bulk= 1;
		}
		else if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
		 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x81)
		 &&((intf->endpoint[1].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
		 &&((intf->endpoint[1].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x02)) {
			printk("Interrupt desc matched. Start Interrupt testing...\n");
			intrr= 1;
		}
	}
	else if(intf->desc.bNumEndpoints == 1)
	{
		if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x81)) {
			printk("ISOIN desc matched. Start ISOIN testing...\n");
			isoin= 1;
		}
		else if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x01)) {
			printk("ISOOUT desc matched. Start ISOOUT testing...\n");
			isoout= 1;
		}
	}


	bulk=1;

	if((bulk==0)&&(intrr==0)&&(isoin==0)&&(isoout==0))
	{
		printk("Non of matched testing item desc. close thread\n");
		goto fail1;
	}


	if(isoin) {
		//FWSTART for ISOIN
		ctrlreq1.bRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
		ctrlreq1.bRequest     = 0x00;
		ctrlreq1.wValue       = 0x0001;
		ctrlreq1.wIndex       = 0x0000;
		ctrlreq1.wLength      = 0;

		usb_fill_control_urb(nvttst->urb1,
				     nvttst->usbdev,
				     usb_sndctrlpipe(nvttst->usbdev, 0),
				     (unsigned char *) &ctrlreq1,
				     NULL,
				     0,
				     usb_nvttst_blocking_completion,
				     nvttst);

		usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);

		msleep(100);
	}




	while(1) {


	if(isoout)
	{
		txfbuf = kmalloc(512*8, GFP_KERNEL);
		if(!txfbuf) {
			printk("[%s]: rx no mem\n",__func__);
			goto fail1;
		}


		for(i=0;i<512*8;i++)
		{
			txfbuf[i] = inCnter++;

			if(inCnter == 0xFF)
				inCnter = 0x00;
		}


		usb_nvttst_setup_IsoOUT(nvttst->urb3, nvttst, txfbuf, 512);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb3, nvttst);



		kfree(txfbuf);
		if(ret)
			goto fail1;


		printk("isoout %d\n",++Cnt);
	}



	if(isoin)
	{
		txfbuf = kmalloc(1536*8, GFP_KERNEL);
		if(!txfbuf) {
			printk("[%s]: rx no mem\n",__func__);
			goto fail1;
		}

		usb_nvttst_setup_IsoIN(nvttst->urb3, nvttst, txfbuf, 1536);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb3, nvttst);

		for(i=0;i<1536*8;i++)
		{
			if(txfbuf[i] != inCnter++)
				printk("ISOIN Data Err!\r\n");

			if(inCnter == 0xFF)
				inCnter = 0x00;
		}

		kfree(txfbuf);
		if(ret)
			goto fail1;


		printk("isoin %d\n",++Cnt);
	}


	if(bulk)
	{
		get_random_bytes ( &len1, sizeof (len1) );
		len1 &= 0x3FFF;//Max 16384 in linux
		if(len1 == 0)
			len1++;

		txfbuf = kmalloc(len1, GFP_KERNEL);
		if(!txfbuf) {
			printk("[%s]: tx no mem\n",__func__);
			goto fail1;
		}

		get_random_bytes ( &len2, sizeof (len2) );
		len2 &= 0x3FFF;
		if(len2 == 0)
			len2++;

		rxfbuf = kmalloc(len2, GFP_KERNEL);
		if(!rxfbuf) {
			printk("[%s]: rx no mem\n",__func__);
			goto fail2;
		}

		for(i=0;i<len2;i++)
		{
			rxfbuf[i] = outCnter++;
		}





	       /********************************
	        *	Vendor Control IN/OUT
	        ********************************/

		usb_nvttst_setup_CxIN (nvttst->urb1, nvttst, &ctrlreq1, (void *)txfbuf, len1);
		usb_nvttst_setup_CxOUT(nvttst->urb2, nvttst, &ctrlreq2, (void *)rxfbuf, len2);

		/* Send Control IN/OUT jobs by non-blocking operation */
		ret = usb_nvttst_issue_urb_nonblocking(nvttst->urb1, nvttst);
		if(ret)
			goto fail3;

		ret = usb_nvttst_issue_urb_nonblocking(nvttst->urb2, nvttst);
		if(ret)
			goto fail3;


		ret = usb_nvttst_nonblocking_waitdone(nvttst->urb1, nvttst);
		if(ret)
			goto fail3;

		ret = usb_nvttst_nonblocking_waitdone(nvttst->urb2, nvttst);
		if(ret)
			goto fail3;

		/* Compare CxIN Data */
		if((nvttst->urb1->status)==0) {
			for(i=0;i<len1;i++)
			{
				if(txfbuf[i] != inCnter++)
					printk("Data Err tot=%d, idx=%d, should 0x%X, but 0x%X \n",len1,i,inCnter,txfbuf[i]);
			}
		}
		kfree(txfbuf);
		kfree(rxfbuf);




		#if 1
	       /********************************
	        *	BULK OUT
	        ********************************/

		cbwbuf = kzalloc(31, GFP_KERNEL);
		if(!cbwbuf) {
			printk("[%s]: cbw no mem\n",__func__);
			goto fail1;
		}

		//get_random_bytes ( &len1, sizeof (len1) );
		len1 = ++bulksize;
		len1 &= 0x1FFFF;
		if(len1 == 0)
			len1++;


		usb_nvttst_fill_cbw(cbwbuf, len1, 1);

		usb_nvttst_setup_BulkOUT(nvttst->urb1,nvttst, cbwbuf, 31);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		kfree(cbwbuf);

		if(ret)
			goto fail1;



		txfbuf = kmalloc(len1, GFP_KERNEL);
		if(!txfbuf) {
			printk("[%s]: tx no mem\n",__func__);
			goto fail1;
		}


		for(i=0;i<len1;i++)
		{
			get_random_bytes ( &txfbuf[i], sizeof (txfbuf[i]) );
		}

		usb_nvttst_setup_BulkOUT(nvttst->urb1,nvttst, txfbuf, len1);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		//kfree(txfbuf);
		if(ret)
			goto fail2;




		cswbuf = kmalloc(13, GFP_KERNEL);
		if(!cswbuf) {
			printk("[%s]: csw no mem\n",__func__);
			goto fail2;
		}

		usb_nvttst_setup_BulkIN(nvttst->urb1,nvttst, cswbuf, 13);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		kfree(cswbuf);

		if(ret)
			goto fail2;

		#endif

		#if 1
		/********************************
	        *	BULK IN
	        ********************************/

		cbwbuf = kzalloc(31, GFP_KERNEL);
		if(!cbwbuf) {
			printk("[%s]: cbw no mem\n",__func__);
			goto fail2;
		}

		usb_nvttst_fill_cbw(cbwbuf, len1, 0);

		usb_nvttst_setup_BulkOUT(nvttst->urb1,nvttst, cbwbuf, 31);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		kfree(cbwbuf);

		if(ret)
			goto fail2;



		rxfbuf = kmalloc(len1, GFP_KERNEL);
		if(!rxfbuf) {
			printk("[%s]: tx no mem\n",__func__);
			goto fail2;
		}

		usb_nvttst_setup_BulkIN(nvttst->urb1,nvttst, rxfbuf, len1);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		//kfree(txfbuf);
		if(ret)
			goto fail3;




		cswbuf = kmalloc(13, GFP_KERNEL);
		if(!cswbuf) {
			printk("[%s]: csw no mem\n",__func__);
			goto fail3;
		}

		usb_nvttst_setup_BulkIN(nvttst->urb1,nvttst, cswbuf, 13);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		kfree(cswbuf);

		if(ret)
			goto fail3;


		if(memcmp(txfbuf, rxfbuf, len1)) {
			printk("BULK DATA COMPARE ERROR!\n");
			goto fail3;
		}

		kfree(rxfbuf);
		kfree(txfbuf);


		#endif


		//if((++Cnt % 500) == 0)
		printk("CX/BULK Pass Count = %d (%d)\n",++Cnt,len1);



		if (kthread_should_stop() || (nvttst->status))
			goto fail1;
	}

	if(intrr)
	{

		#if 1
	       /********************************
	        *	Interrupt OUT/IN
	        ********************************/
		if(len1 > (size_t)(intf->endpoint[0].desc.wMaxPacketSize & 0x7ff))
			len1=1;


		txfbuf = kmalloc(len1, GFP_KERNEL);
		if(!txfbuf) {
			printk("[%s]: tx no mem\n",__func__);
			goto fail1;
		}

		for(i=0;i<len1;i++)
		{
			txfbuf[i] = outCnter++;
		}



		usb_nvttst_setup_IntrOUT(nvttst->urb1,nvttst, txfbuf, len1);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		if(ret)
			goto fail2;




		rxfbuf = kmalloc(len1, GFP_KERNEL);
		if(!rxfbuf) {
			printk("[%s]: rx no mem\n",__func__);
			goto fail1;
		}

		usb_nvttst_setup_IntrIN(nvttst->urb1,nvttst, rxfbuf, len1);
		ret = usb_nvttst_issue_urb_blocking(nvttst->urb1, nvttst);
		if(ret)
			goto fail3;


		if(memcmp(txfbuf, rxfbuf, len1)) {
			printk("Interrupt DATA COMPARE ERROR!\n");
			goto fail3;
		}


		kfree(txfbuf);
		kfree(rxfbuf);


		#endif

		printk("Interrupt Pass Count = %d(%d)\n",++Cnt,len1);
		len1++;

	}
}




fail3:
	kfree(rxfbuf);
fail2:
	kfree(txfbuf);
fail1:
	complete(&nvttst->close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}
#endif

#if 1


static int usb_nvttst_CxRW_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	struct usb_ctrlrequest    *ctrlreq1,*ctrlreq2;
	u8 		*txfbuf;
	u8 		*rxfbuf;
	size_t	i,len1=0,txofs=0;
	int 	ret,Cnt=0;
	//struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096,MaxTestSize = 16384;//16384

	nvttst->cx_urb_in 		= NULL;
	nvttst->cx_urb_out 		= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;

	#if 0
	#ifdef CONFIG_PM
	printk("TEST Suspend Resume\n");

	//{
	//	u32 *pREG;
	//	pREG = (u32 *)0xF1700058;
	//	*pREG |= 0x100;
	//	msleep(100);
	//}

	while(1)
	{
		u32 *pREG;
		u32 temp;

		pREG = (u32 *)0xF1700420;

		while(((*pREG >>5) &0xF) != 0x0)
			;

		msleep(5000);
		printk("Suspend\n");


		temp = *pREG;
		temp &= ~(0xF<<5);
		*pREG = temp + (0x3<<5);

		while(((*pREG >>5) &0xF) != 0x3)
			;

		msleep(5000);
		printk("Resume\n");

		temp = *pREG;
		temp &= ~(0xF<<5);
		*pREG = temp + (0xF<<5);


		if (kthread_should_stop())
			goto fail1;

	}
	#endif
	#endif

	nvttst->cx_urb_in = usb_alloc_urb(0, GFP_KERNEL);
	if (!nvttst->cx_urb_in)
		goto fail1;

	nvttst->cx_urb_out = usb_alloc_urb(0, GFP_KERNEL);
	if (!nvttst->cx_urb_out)
		goto fail1;


	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	for(i=0;i<MaxTestSize+MaxOfs;i++)
	{
		txfbuf[i] = i&0xFF;
	}

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}

	ctrlreq1 = kmalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL);
	ctrlreq2 = kmalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL);

	while(1) {

		get_random_bytes ( &len1, sizeof (len1) );
		len1 = len1 % MaxTestSize;//Max 16384 in linux
		if(len1 == 0)
			len1++;


		//get_random_bytes ( &len2, sizeof (len2) );
		//len2 = len2 % MaxTestSize;//Max 16384 in linux
		//if(len2 == 0)
		//	len2++;


		#if 0
		msleep(200);

		//small test size for iso
		len1 = len1 % 257;
		if(len1 == 0)
			len1++;
		len2 = len2 % 257;
		if(len2 == 0)
			len2++;
		#endif

       /********************************
        *	Vendor Control IN/OUT
        ********************************/


		//msleep(len1&127);
		usb_nvttst_setup_CxOUT(nvttst, ctrlreq2, (void *)txfbuf+txofs, len1);
		ret = usb_nvttst_cxout_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		msleep(11);

		//msleep(len1&&127);
		usb_nvttst_setup_CxIN (nvttst, ctrlreq1, (void *)rxfbuf, len1);
		ret = usb_nvttst_cxin_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		msleep(11);



		if(memcmp(txfbuf+txofs, rxfbuf, len1)) {
			printk("CX DATA COMPARE ERROR!\n");
			printk("CX= %d (%d)(%d)\n",++Cnt,len1,txofs);
			{
				//for(i=0;i<len1;i++) {
				//	if(txfbuf[txofs+i] != rxfbuf[i]) {
				//		printk("CX DATA COMPARE ERROR!%d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",i,txfbuf[txofs+i-1],rxfbuf[i-1],txfbuf[txofs+i],rxfbuf[i],txfbuf[txofs+i+1],rxfbuf[i+1]);
				//	}
				//}
			}


			goto fail3;
		}
		txofs = (txofs + len1)&0xFF;

		if(!nvttst->msgoff)
			printk("CX= %d (%d)(%d)\n",++Cnt,len1,txofs);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}




fail3:


fail2:

	kfree(ctrlreq1);
	kfree(ctrlreq2);

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

fail1:

	if(nvttst->cx_urb_in)
		usb_free_urb(nvttst->cx_urb_in);

	if(nvttst->cx_urb_out)
		usb_free_urb(nvttst->cx_urb_out);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->cx_close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}





static int usb_nvttst_BULKRW_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 	*txfbuf;
	u8 	*rxfbuf;
	u8 	*cbwbuf,*cswbuf;
	size_t	ofs1,bulksize=0;
	int 	ret,Cnt=0;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096, MaxTestSize = 256*1024;//256
	int		epi=1,epo=2;


	nvttst->bulk_urb_in 	= NULL;
	nvttst->bulk_urb_out 	= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;
	cbwbuf					= NULL;
	cswbuf					= NULL;


	// Check if bulk/interrupt/isochronous test

	if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
	 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x81)
	 &&((intf->endpoint[1].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
	 ) {
		printk("Bulk desc matched. Start bulk EP1/2 testing...\n");
		epo = (intf->endpoint[1].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK));
	}
	else
	{
		printk("Bulk desc NOT matched. Close test!\n");
		goto fail1;
	}


	nvttst->bulk_urb_in = usb_alloc_urb(0, GFP_KERNEL);
	if (!nvttst->bulk_urb_in)
		goto fail1;

	nvttst->bulk_urb_out = usb_alloc_urb(0, GFP_KERNEL);
	if (!nvttst->bulk_urb_out)
		goto fail1;


	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	get_random_bytes ( txfbuf, MaxTestSize+MaxOfs );
	//{
	//	int i;
	//	for(i=0;i<MaxTestSize+MaxOfs;i++)
	//		txfbuf[i]=i&0xFF;
	//}

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}


	cbwbuf = kzalloc(31, GFP_KERNEL);
	if(!cbwbuf) {
		printk("[%s]: cbw no mem\n",__func__);
		goto fail2;
	}

	cswbuf = kmalloc(13, GFP_KERNEL);
	if(!cswbuf) {
		printk("[%s]: csw no mem\n",__func__);
		goto fail2;
	}


	bulksize = 0;

	while(1) {

		get_random_bytes ( &bulksize, sizeof (bulksize) );
		//bulksize++;
		bulksize &= (MaxTestSize-1);
		if(bulksize == 0)
			bulksize++;

		get_random_bytes ( &ofs1, sizeof (ofs1) );
		ofs1 &= (MaxOfs-1);
		//ofs1=0;

		msleep(15);

		#if 0
		msleep(200);

		//small test size for iso
		bulksize = bulksize % 33;
		if(bulksize == 0)
			bulksize++;

		ofs1 = 0;
		#endif


		#if 1
       /********************************
        *	BULK OUT
        ********************************/

		usb_nvttst_fill_cbw(cbwbuf, bulksize, 1);
		//printk("bksz=%d\r\n",bulksize);

		usb_nvttst_setup_BulkOUT(nvttst, cbwbuf, 31,epo);
		ret = usb_nvttst_bulkout_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		usb_nvttst_setup_BulkOUT(nvttst, txfbuf+ofs1, bulksize,epo);
		ret = usb_nvttst_bulkout_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;


		usb_nvttst_setup_BulkIN(nvttst, cswbuf, 13,epi);
		ret = usb_nvttst_bulkin_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		#endif



		#if 1
		/********************************
        *	BULK IN
        ********************************/
		usb_nvttst_fill_cbw(cbwbuf, bulksize, 0);

		usb_nvttst_setup_BulkOUT(nvttst, cbwbuf, 31,epo);
		ret = usb_nvttst_bulkout_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		usb_nvttst_setup_BulkIN(nvttst, rxfbuf+ofs1, bulksize,epi);
		ret = usb_nvttst_bulkin_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		usb_nvttst_setup_BulkIN(nvttst, cswbuf, 13,epi);
		ret = usb_nvttst_bulkin_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;

		if(memcmp(txfbuf+ofs1, rxfbuf+ofs1, bulksize)) {
			u8 *buf1,*buf2;
			int i;

			buf1 = txfbuf+ofs1;
			buf2 = rxfbuf+ofs1;

			for(i=0;i<bulksize;i++) {
				if(buf1[i] != buf2[i]) {
					break;
				}
			}
			printk("BULK DATA COMPARE ERROR! %d\n",i);

			goto fail3;
		}




		#endif

		//printk("                         BULK Pass Count = %d (%d)(%d)\n",++Cnt,bulksize,ofs1);
		if(!nvttst->msgoff)
		printk("BULK Pass Count = %d (%d)(%d)\n",++Cnt,bulksize,ofs1);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}




fail3:


fail2:

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

	if(cbwbuf)
		kfree(cbwbuf);

	if(cswbuf)
		kfree(cswbuf);

fail1:

	if(nvttst->bulk_urb_in)
		usb_free_urb(nvttst->bulk_urb_in);

	if(nvttst->bulk_urb_out)
		usb_free_urb(nvttst->bulk_urb_out);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->bulk_close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}


static int usb_nvttst_INTRRW_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 		*txfbuf;
	u8 		*rxfbuf;
	size_t	ofs2=0,ofs1=0,testsize=1;
	int 	ret,Cnt=0;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096,MaxTestSize = 0;


	nvttst->intr_urb_in 	= NULL;
	nvttst->intr_urb_out 	= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;

	// Check if bulk/interrupt/isochronous test

	if(((intf->endpoint[2].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
	 &&((intf->endpoint[2].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x83)
	 &&((intf->endpoint[3].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
	 &&((intf->endpoint[3].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x04)) {
		printk("INTR desc matched. Start INTR EP3/4 testing...\n");
	}
	else
	{
		printk("INTR desc NOT matched. Close test!\n");
		goto fail1;
	}

	MaxTestSize = (intf->endpoint[2].desc.wMaxPacketSize & 0x7FF)*16;

	nvttst->intr_urb_in = usb_alloc_urb(0, GFP_KERNEL);
	if (!nvttst->intr_urb_in)
		goto fail1;

	nvttst->intr_urb_out = usb_alloc_urb(0, GFP_KERNEL);
	if (!nvttst->intr_urb_out)
		goto fail1;


	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	get_random_bytes ( txfbuf, MaxTestSize+MaxOfs );

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}


	while(1) {
		get_random_bytes(&ofs1, sizeof(ofs1));
		ofs1 &= 0xFFF;

		get_random_bytes(&ofs2, sizeof(ofs2));
		ofs2 &= 0xFFF;

		//msleep(17);

		usb_nvttst_setup_IntrOUT(nvttst, txfbuf+ofs2, testsize);
		ret = usb_nvttst_introut_issue_urb_blocking(nvttst);
		if(ret)
			goto fail2;


		usb_nvttst_setup_IntrIN(nvttst, rxfbuf+ofs1, testsize);
		ret = usb_nvttst_intrin_issue_urb_blocking(nvttst);
		if(ret)
			goto fail2;


		if(memcmp(txfbuf+ofs2, rxfbuf+ofs1, testsize)) {
			printk("INTR DATA COMPARE ERROR! (%d)(%d)(%d)\n",testsize,ofs1,ofs2);
			goto fail3;
		}



		//rintk("                                   INTR= %d (%d)(%d)(%d)\n",++Cnt,testsize,ofs1,ofs2);
		printk("INTR= %d (%d)(%d)(%d)\n",++Cnt,testsize,ofs1,ofs2);
		testsize++;
		if(testsize > MaxTestSize)
			testsize=1;


		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}




fail3:


fail2:

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

fail1:

	if(nvttst->intr_urb_in)
		usb_free_urb(nvttst->intr_urb_in);

	if(nvttst->intr_urb_out)
		usb_free_urb(nvttst->intr_urb_out);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->intr_close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}


static int usb_nvttst_ISOIN_thread(void * __nvttst)
#if 1
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 		*txfbuf;
	u8 		*rxfbuf,*rxfbuf2;
	size_t	i,ofs2=0,ofs1=0;
	int 	ret,Cnt=0;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096, pktno=256,MaxTestSize = 256*2048;
	int MaxPktSz,HBW,epn=5;


	nvttst->iso_urb_in 		= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;
	rxfbuf2 				= NULL;

	if(nvttst->usbdev->descriptor.bcdUSB == 0x0110)
	{
		if(((intf->endpoint[3].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[3].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x85)) {
			printk("ISOIN desc matched. Start ISOIN EP5 testing...\n");
		}
		else
		{
			printk("ISOIN desc NOT matched. Close test!\n");
			goto fail1;
		}

		MaxPktSz = intf->endpoint[3].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[3].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}
	else
	{
		if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x81)) {
			printk("ISOIN desc matched. Start ISOIN EP1 testing...\n");
			epn = 1;
		} else if(((intf->endpoint[4].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[4].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x85)) {
			printk("ISOIN desc matched. Start ISOIN EP5 testing...\n");
			epn = 5;
		}
		else
		{
			printk("ISOIN x desc NOT matched. Close test! 0x%02X\n",intf->endpoint[4].desc.bEndpointAddress);
			goto fail1;
		}

		MaxPktSz = intf->endpoint[epn-1].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[epn-1].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}

	MaxPktSz = MaxPktSz * HBW;

	nvttst->iso_urb_in = usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_in)
		goto fail1;

	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	for(i=0;i<MaxTestSize+MaxOfs;i++)
	{
		txfbuf[i] = i%255;
	}

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}


	rxfbuf2 = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf2) {
		printk("[%s]: rx2 no mem\n",__func__);
		goto fail2;
	}






	while(1) {
		ofs2 = 0;

		// This delay is very important for testing workaround
		msleep(50);

		memset(rxfbuf, 0xAA, MaxTestSize);

		usb_nvttst_setup_IsoIN(nvttst, rxfbuf, MaxPktSz, pktno,ofs2,epn);






		ret = usb_nvttst_isoin_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;

		usb_nvttst_isoin_issue_urb_waitdone(nvttst);

		/*
		for(i=1;i<(pktno-1);i++)
		{
			if((nvttst->iso_urb_in->iso_frame_desc[i-1].actual_length == MaxPktSz) &&
			(nvttst->iso_urb_in->iso_frame_desc[i].actual_length == 0) &&
			(nvttst->iso_urb_in->iso_frame_desc[i+1].actual_length == MaxPktSz))
			{
				printk("!!!!!!!!!!!!!!!!!!!!!!!!!%d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",i);
			}
		}
		*/


		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_in->iso_frame_desc[i].actual_length > 0)
			{
				ofs1 = *(rxfbuf+nvttst->iso_urb_in->iso_frame_desc[i].offset);
				break;
			}
		}

		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_in->iso_frame_desc[i].actual_length > 0)
			{
				if(memcmp(txfbuf+ofs1, rxfbuf+nvttst->iso_urb_in->iso_frame_desc[i].offset, nvttst->iso_urb_in->iso_frame_desc[i].actual_length)) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_in->iso_frame_desc[i].actual_length,nvttst->iso_urb_in->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_in->iso_frame_desc[i-2].actual_length);
					goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_in->iso_frame_desc[i].actual_length)%255;
			}
		}




		//printk("                                                            ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);
		printk("ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}




fail3:


fail2:

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

fail1:

	if(nvttst->iso_urb_in)
		usb_free_urb(nvttst->iso_urb_in);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->isoin_close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}
#else
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 		*txfbuf;
	u8 		*rxfbuf,*rxfbuf2;
	size_t	i,ofs1=0;
	int 	ret,Cnt=0;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096, pktno=1000,MaxTestSize = 1000*1024;
	int MaxPktSz,HBW,epn=5;


	nvttst->iso_urb_in 		= NULL;
	nvttst->iso_urb_in2 	= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;
	rxfbuf2 				= NULL;

	if(nvttst->usbdev->descriptor.bcdUSB == 0x0110)
	{
		if(((intf->endpoint[3].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[3].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x85)) {
			printk("ISOIN desc matched. Start ISOIN EP5 testing...\n");
			epn=5;
		}
		else
		{
			printk("ISOIN desc NOT matched. Close test!\n");
			goto fail1;
		}

		MaxPktSz = intf->endpoint[3].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[3].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}
	else
	{
		if(((intf->endpoint[4].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[4].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x85)) {
			printk("ISOIN desc matched. Start ISOIN EP5 testing...\n");
			epn=5;

			MaxPktSz = intf->endpoint[4].desc.wMaxPacketSize & 0x07FF;
			HBW      = ((intf->endpoint[4].desc.wMaxPacketSize >> 11) & 0x3)+1;
		}
		else if(((intf->endpoint[0].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[0].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x81)) {
			printk("ISOIN desc matched. Start ISOIN EP1 testing...\n");
			epn=1;
			MaxPktSz = intf->endpoint[0].desc.wMaxPacketSize & 0x07FF;
			HBW      = ((intf->endpoint[0].desc.wMaxPacketSize >> 11) & 0x3)+1;
		}
		else
		{
			printk("ISOIN desc NOT matched. Close test!\n");
			goto fail1;
		}


	}

	MaxPktSz = MaxPktSz * HBW;

	nvttst->iso_urb_in = usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_in)
		goto fail1;

	nvttst->iso_urb_in2 = usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_in2)
		goto fail1;

	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	for(i=0;i<MaxTestSize+MaxOfs;i++)
	{
		txfbuf[i] = i%255;
	}

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}

	rxfbuf2 = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf2) {
		printk("[%s]: rx2 no mem\n",__func__);
		goto fail2;
	}

	memset(rxfbuf, 0xAA, MaxTestSize);
	usb_nvttst_setup_IsoIN(nvttst, rxfbuf, MaxPktSz, pktno,0,epn);

	memset(rxfbuf2, 0xAA, MaxTestSize);
	usb_nvttst_setup_IsoIN2(nvttst, rxfbuf2, MaxPktSz, pktno,0,epn);

	ret = usb_nvttst_isoin_issue_urb_nonblocking(nvttst);
	if(ret)
		goto fail3;

	while(1) {
		// This delay is very important for testing workaround
		//msleep(50);

		memset(rxfbuf2, 0xAA, MaxTestSize);
		usb_nvttst_setup_IsoIN2(nvttst, rxfbuf2, MaxPktSz, pktno,0,epn);
		ret = usb_nvttst_isoin2_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;



		usb_nvttst_isoin_issue_urb_waitdone(nvttst);


		// comapre 1

		/*for(i=1;i<(pktno-1);i++)
		{
			if((nvttst->iso_urb_in->iso_frame_desc[i-1].actual_length == MaxPktSz) &&
			(nvttst->iso_urb_in->iso_frame_desc[i].actual_length == 0) &&
			(nvttst->iso_urb_in->iso_frame_desc[i+1].actual_length == MaxPktSz))
			{
				printk("!!!!!!!!!!!!!!!!!!!!!!!!!%d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",i);
			}
		}*/

		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_in->iso_frame_desc[i].actual_length > 0)
			{
				if(memcmp(txfbuf+ofs1, rxfbuf+nvttst->iso_urb_in->iso_frame_desc[i].offset, nvttst->iso_urb_in->iso_frame_desc[i].actual_length)) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_in->iso_frame_desc[i].actual_length,nvttst->iso_urb_in->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_in->iso_frame_desc[i-2].actual_length);
					//goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_in->iso_frame_desc[i].actual_length)%255;
			}
		}
		printk("ISOIN1= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);


		memset(rxfbuf, 0xAA, MaxTestSize);
		usb_nvttst_setup_IsoIN(nvttst, rxfbuf, MaxPktSz, pktno,0,epn);
		ret = usb_nvttst_isoin_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;



		usb_nvttst_isoin2_issue_urb_waitdone(nvttst);

		// comapre 2


		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_in2->iso_frame_desc[i].actual_length > 0)
			{
				if(memcmp(txfbuf+ofs1, rxfbuf2+nvttst->iso_urb_in2->iso_frame_desc[i].offset, nvttst->iso_urb_in2->iso_frame_desc[i].actual_length)) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_in2->iso_frame_desc[i].actual_length,nvttst->iso_urb_in2->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_in2->iso_frame_desc[i-2].actual_length);
					//goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_in2->iso_frame_desc[i].actual_length)%255;
			}
		}
		printk("ISOIN2= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in2->actual_length,ofs1);



		//printk("                                                            ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);
		//printk("ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}



fail3:


fail2:

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

fail1:

	if(nvttst->iso_urb_in)
		usb_free_urb(nvttst->iso_urb_in);

	if(nvttst->iso_urb_in2)
		usb_free_urb(nvttst->iso_urb_in2);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->isoin_close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}
#endif




static int usb_nvttst_ISOOUT_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 		*txfbuf;
	size_t	i,ofs1=0;
	int 	ret,Cnt=0;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int pktno=512,MaxTestSize = (512*1024)+512;
	int MaxPktSz,HBW,epn=6;

	nvttst->iso_urb_out 	= NULL;
	txfbuf 					= NULL;


	if(nvttst->usbdev->descriptor.bcdUSB == 0x0110)
	{
		// Check if bulk/interrupt/isochronous test
		if(((intf->endpoint[3].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[3].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x06)) {
			printk("ISOOUT desc matched. Start ISOOUT EP6 testing...\n");
		}
		else
		{
			printk("ISOOUT desc NOT matched. Close test!\n");
			goto fail1;
		}

		MaxPktSz = intf->endpoint[3].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[3].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}
	else
	{
		// Check if bulk/interrupt/isochronous test
		if(((intf->endpoint[5].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[5].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x06)) {
			printk("ISOOUT desc matched. Start ISOOUT EP6 testing...\n");
			epn = 6;
		}
		else if(((intf->endpoint[1].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[1].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x02)) {
			printk("ISOOUT desc matched. Start ISOOUT EP2 testing...\n");
			epn = 2;
		}
		else
		{
			printk("ISOOUT desc NOT matched. Close test!\n");
			goto fail1;
		}

		MaxPktSz = intf->endpoint[epn-1].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[epn-1].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}


	MaxPktSz = MaxPktSz * HBW;

	nvttst->iso_urb_out = usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_out)
		goto fail1;

	txfbuf = kmalloc(MaxTestSize, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	for(i=0;i<MaxTestSize;i++)
	{
		txfbuf[i] = i%255;
	}

	while(1) {

		// This delay is making device having time to compare data.
		msleep(50);

		usb_nvttst_setup_IsoOUT(nvttst, txfbuf+ofs1, MaxPktSz, pktno,0,epn);
		ret = usb_nvttst_isoout_issue_urb_blocking(nvttst);
		if(ret)
			goto fail3;


		ofs1 = (ofs1 + (MaxPktSz*pktno))%255;

		//printk("                                                            ISOOUT= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_out->actual_length,ofs1);
		printk("ISOOUT= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_out->actual_length,ofs1);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;
	}


fail3:
fail2:

	if(txfbuf)
		kfree(txfbuf);

fail1:

	if(nvttst->iso_urb_out)
		usb_free_urb(nvttst->iso_urb_out);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->isoout_close);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}

#if 0
static int usb_nvttst_ISOIN_UVC1_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 		*txfbuf;
	u8 		*rxfbuf,*rxfbuf2;
	size_t	i,ofs1=0;
	int 	ret;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096, pktno=1000,MaxTestSize = 1000*1024;
	int MaxPktSz,HBW;
	u8		UvcHeader[12];
	u32     CounterValue,FrmDataSz=0,PTS=0,tPTS=0,targetSz=1;
	u8		FID=255,state=1;
	unsigned long timeout = 0;

	nvttst->iso_urb_uvc11   = NULL;
	nvttst->iso_urb_uvc12 	= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;
	rxfbuf2 				= NULL;

	{
		if(((intf->endpoint[2].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[2].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x87)) {
			printk("ISOIN UVC desc matched. Start ISOIN UVC EP7 testing...\n");
		}
		else
		{
			printk("ISOIN desc NOT matched. Close test!\n");
			goto fail1;
		}

		MaxPktSz = intf->endpoint[2].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[2].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}

	nvttst->msgoff = 1;

	MaxPktSz = MaxPktSz * HBW;

	nvttst->iso_urb_uvc11= usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_uvc11)
		goto fail1;

	nvttst->iso_urb_uvc12 = usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_uvc12)
		goto fail1;

	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	for(i=0;i<MaxTestSize+MaxOfs;i++)
	{
		txfbuf[i] = i%255;
	}

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}

	rxfbuf2 = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf2) {
		printk("[%s]: rx2 no mem\n",__func__);
		goto fail2;
	}

	memset(rxfbuf, 0xAA, MaxTestSize);
	usb_nvttst_setup_uvc11(nvttst, rxfbuf, MaxPktSz, pktno,0,7);

	memset(rxfbuf2, 0xAA, MaxTestSize);
	usb_nvttst_setup_uvc12(nvttst, rxfbuf2, MaxPktSz, pktno,0,7);

	ret = usb_nvttst_isouvc11_issue_urb_nonblocking(nvttst);
	if(ret)
		goto fail3;

	timeout = jiffies + msecs_to_jiffies(2000);

	while(1) {
		//memset(rxfbuf2, 0xAA, MaxTestSize);
		usb_nvttst_setup_uvc12(nvttst, rxfbuf2, MaxPktSz, pktno,0,7);
		ret = usb_nvttst_isouvc12_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;


		usb_nvttst_isouvc11_issue_urb_waitdone(nvttst);


		// comapre 1

		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_uvc11->iso_frame_desc[i].actual_length > 0)
			{
				memcpy(UvcHeader, rxfbuf+nvttst->iso_urb_uvc11->iso_frame_desc[i].offset,(rxfbuf+nvttst->iso_urb_uvc11->iso_frame_desc[i].offset)[0]);

				/* Data Compare */
				if(memcmp(txfbuf+ofs1, rxfbuf+nvttst->iso_urb_uvc11->iso_frame_desc[i].offset+UvcHeader[0], nvttst->iso_urb_uvc11->iso_frame_desc[i].actual_length-UvcHeader[0])) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_uvc11->iso_frame_desc[i].actual_length,nvttst->iso_urb_uvc11->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_uvc11->iso_frame_desc[i-2].actual_length);
					goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_uvc11->iso_frame_desc[i].actual_length-UvcHeader[0])%255;
				FrmDataSz += nvttst->iso_urb_uvc11->iso_frame_desc[i].actual_length-UvcHeader[0];
				/* Header Compare */
				if(UvcHeader[0] == 12) {
					CounterValue =  UvcHeader[6]+(UvcHeader[7]<<8)+(UvcHeader[8]<<16)+(UvcHeader[9]<<24);

					tPTS = UvcHeader[2]+(UvcHeader[3]<<8)+(UvcHeader[4]<<16)+(UvcHeader[5]<<24);

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);
						PTS = tPTS;

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					if((UvcHeader[1] & 0x8C) != 0x8C) {
						printk("Header Err! 0x%02X\n",UvcHeader[1]);
						while(1);
					}

					if(tPTS != PTS) {
						printk("PTS ERR! 0x%08X 0x%08X\n",PTS,tPTS);
						while(1);
					}

				} else if (UvcHeader[0] == 2) {
					CounterValue =  0;

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					//if((UvcHeader[1] & 0x8C) != 0x8C) {
					//	printk("Header Err! 0x%02X\n",UvcHeader[1]);
					//	while(1);
					//}

				} else {
					printk("header size error!\r\n");
					goto fail3;
				}




			}
		}

    	if (time_after(jiffies, timeout)) {
			timeout = jiffies + msecs_to_jiffies(500);
			printk("UVC1(%ums)= %d\n",(CounterValue/1000000*33),targetSz-1);
		}

		//memset(rxfbuf, 0xAA, MaxTestSize);
		usb_nvttst_setup_uvc11(nvttst, rxfbuf, MaxPktSz, pktno,0,7);
		ret = usb_nvttst_isouvc11_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;



		usb_nvttst_isouvc12_issue_urb_waitdone(nvttst);

		// comapre 2


		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_uvc12->iso_frame_desc[i].actual_length > 0)
			{
				memcpy(UvcHeader, rxfbuf2+nvttst->iso_urb_uvc12->iso_frame_desc[i].offset,(rxfbuf2+nvttst->iso_urb_uvc12->iso_frame_desc[i].offset)[0]);

				if(memcmp(txfbuf+ofs1, rxfbuf2+nvttst->iso_urb_uvc12->iso_frame_desc[i].offset+UvcHeader[0], nvttst->iso_urb_uvc12->iso_frame_desc[i].actual_length-UvcHeader[0])) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_uvc12->iso_frame_desc[i].actual_length,nvttst->iso_urb_uvc12->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_uvc12->iso_frame_desc[i-2].actual_length);
					goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_uvc12->iso_frame_desc[i].actual_length-UvcHeader[0])%255;
				FrmDataSz += nvttst->iso_urb_uvc12->iso_frame_desc[i].actual_length-UvcHeader[0];
				/* Header Compare */
				if(UvcHeader[0] == 12) {
					CounterValue =  UvcHeader[6]+(UvcHeader[7]<<8)+(UvcHeader[8]<<16)+(UvcHeader[9]<<24);

					tPTS = UvcHeader[2]+(UvcHeader[3]<<8)+(UvcHeader[4]<<16)+(UvcHeader[5]<<24);

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);
						PTS = tPTS;

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					if((UvcHeader[1] & 0x8C) != 0x8C) {
						printk("Header Err! 0x%02X\n",UvcHeader[1]);
						while(1);
					}

					if(tPTS != PTS) {
						printk("PTS ERR! 0x%08X 0x%08X\n",PTS,tPTS);
						while(1);
					}

				} else if (UvcHeader[0] == 2) {
					CounterValue =  0;

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					//if((UvcHeader[1] & 0x8C) != 0x8C) {
					//	printk("Header Err! 0x%02X\n",UvcHeader[1]);
					//	while(1);
					//}

				} else {
					printk("header size error!\r\n");
					goto fail3;
				}



			}
		}
		//printk("UVC1(%u)= %d (%d)(%d)\n",(CounterValue/1000000*33),++Cnt,nvttst->iso_urb_uvc12->actual_length,ofs1);

    	if (time_after(jiffies, timeout)) {
			timeout = jiffies + msecs_to_jiffies(500);
			printk("UVC1(%ums)= %d\n",(CounterValue/1000000*33),targetSz-1);
		}

		//printk("                                                            ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);
		//printk("ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}




fail3:


fail2:

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

fail1:

	if(nvttst->iso_urb_uvc11)
		usb_free_urb(nvttst->iso_urb_uvc11);

	if(nvttst->iso_urb_uvc12)
		usb_free_urb(nvttst->iso_urb_uvc12);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->isoin_close_uvc1);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}



static int usb_nvttst_ISOIN_UVC2_thread(void * __nvttst)
{
	struct usb_nvttst *nvttst = (struct usb_nvttst *)__nvttst;
	u8 		*txfbuf;
	u8 		*rxfbuf,*rxfbuf2;
	size_t	i,ofs1=0;
	int 	ret;
	struct usb_host_interface *intf = nvttst->inf->cur_altsetting;
	int MaxOfs = 4096, pktno=1000,MaxTestSize = 1000*1024;
	int MaxPktSz,HBW;
	u8		UvcHeader[12];
	u32     CounterValue,FrmDataSz=0,PTS=0,tPTS=0,targetSz=1;
	u8		FID=255,state=1;
	unsigned long timeout = 0;

	nvttst->iso_urb_uvc21   = NULL;
	nvttst->iso_urb_uvc22 	= NULL;
	txfbuf 					= NULL;
	rxfbuf 					= NULL;
	rxfbuf2 				= NULL;

	{
		if(((intf->endpoint[3].desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC)
		 &&((intf->endpoint[3].desc.bEndpointAddress & (USB_ENDPOINT_DIR_MASK|USB_ENDPOINT_NUMBER_MASK)) == 0x88)) {
			printk("ISOIN UVC2 desc matched. Start ISOIN UVC EP8 testing...\n");
		}
		else
		{
			printk("ISOIN desc NOT matched. Close test!\n");
			goto fail1;
		}

		MaxPktSz = intf->endpoint[3].desc.wMaxPacketSize & 0x07FF;
		HBW      = ((intf->endpoint[3].desc.wMaxPacketSize >> 11) & 0x3)+1;
	}

	nvttst->msgoff = 1;

	MaxPktSz = MaxPktSz * HBW;

	nvttst->iso_urb_uvc21= usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_uvc21)
		goto fail1;

	nvttst->iso_urb_uvc22 = usb_alloc_urb(pktno, GFP_KERNEL);
	if (!nvttst->iso_urb_uvc22)
		goto fail1;

	txfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!txfbuf) {
		printk("[%s]: tx no mem\n",__func__);
		goto fail2;
	}

	for(i=0;i<MaxTestSize+MaxOfs;i++)
	{
		txfbuf[i] = i%255;
	}

	rxfbuf = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf) {
		printk("[%s]: rx no mem\n",__func__);
		goto fail2;
	}

	rxfbuf2 = kmalloc(MaxTestSize+MaxOfs, GFP_KERNEL);
	if(!rxfbuf2) {
		printk("[%s]: rx2 no mem\n",__func__);
		goto fail2;
	}

	memset(rxfbuf, 0xAA, MaxTestSize);
	usb_nvttst_setup_uvc21(nvttst, rxfbuf, MaxPktSz, pktno,0,8);

	memset(rxfbuf2, 0xAA, MaxTestSize);
	usb_nvttst_setup_uvc22(nvttst, rxfbuf2, MaxPktSz, pktno,0,8);

	ret = usb_nvttst_isouvc21_issue_urb_nonblocking(nvttst);
	if(ret)
		goto fail3;

	timeout = jiffies + msecs_to_jiffies(2000);

	while(1) {
		//memset(rxfbuf2, 0xAA, MaxTestSize);
		usb_nvttst_setup_uvc22(nvttst, rxfbuf2, MaxPktSz, pktno,0,8);
		ret = usb_nvttst_isouvc22_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;


		usb_nvttst_isouvc21_issue_urb_waitdone(nvttst);


		// comapre 1

		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_uvc21->iso_frame_desc[i].actual_length > 0)
			{
				memcpy(UvcHeader, rxfbuf+nvttst->iso_urb_uvc21->iso_frame_desc[i].offset,(rxfbuf+nvttst->iso_urb_uvc21->iso_frame_desc[i].offset)[0]);

				/* Data Compare */
				if(memcmp(txfbuf+ofs1, rxfbuf+nvttst->iso_urb_uvc21->iso_frame_desc[i].offset+UvcHeader[0], nvttst->iso_urb_uvc21->iso_frame_desc[i].actual_length-UvcHeader[0])) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_uvc21->iso_frame_desc[i].actual_length,nvttst->iso_urb_uvc21->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_uvc21->iso_frame_desc[i-2].actual_length);
					goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_uvc21->iso_frame_desc[i].actual_length-UvcHeader[0])%255;
				FrmDataSz += nvttst->iso_urb_uvc21->iso_frame_desc[i].actual_length-UvcHeader[0];
				/* Header Compare */
				if(UvcHeader[0] == 12) {
					CounterValue =  UvcHeader[6]+(UvcHeader[7]<<8)+(UvcHeader[8]<<16)+(UvcHeader[9]<<24);

					tPTS = UvcHeader[2]+(UvcHeader[3]<<8)+(UvcHeader[4]<<16)+(UvcHeader[5]<<24);

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);
						PTS = tPTS;

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					if((UvcHeader[1] & 0x8C) != 0x8C) {
						printk("Header Err! 0x%02X\n",UvcHeader[1]);
						while(1);
					}

					if(tPTS != PTS) {
						printk("PTS ERR! 0x%08X 0x%08X\n",PTS,tPTS);
						while(1);
					}

				} else if (UvcHeader[0] == 2) {
					CounterValue =  0;

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					//if((UvcHeader[1] & 0x8C) != 0x8C) {
					//	printk("Header Err! 0x%02X\n",UvcHeader[1]);
					//	while(1);
					//}

				} else {
					printk("header size error!\r\n");
					goto fail3;
				}




			}
		}

    	if (time_after(jiffies, timeout)) {
			timeout = jiffies + msecs_to_jiffies(500);
			printk("UVC2(%ums)= %d\n",(CounterValue/1000000*33),targetSz-1);
		}

		//memset(rxfbuf, 0xAA, MaxTestSize);
		usb_nvttst_setup_uvc21(nvttst, rxfbuf, MaxPktSz, pktno,0,8);
		ret = usb_nvttst_isouvc21_issue_urb_nonblocking(nvttst);
		if(ret)
			goto fail3;



		usb_nvttst_isouvc22_issue_urb_waitdone(nvttst);

		// comapre 2


		for(i=0;i<pktno;i++)
		{
			if(nvttst->iso_urb_uvc22->iso_frame_desc[i].actual_length > 0)
			{
				memcpy(UvcHeader, rxfbuf2+nvttst->iso_urb_uvc22->iso_frame_desc[i].offset,(rxfbuf2+nvttst->iso_urb_uvc22->iso_frame_desc[i].offset)[0]);

				if(memcmp(txfbuf+ofs1, rxfbuf2+nvttst->iso_urb_uvc22->iso_frame_desc[i].offset+UvcHeader[0], nvttst->iso_urb_uvc22->iso_frame_desc[i].actual_length-UvcHeader[0])) {
					printk("ISOIN DATA COMPARE ERROR! (%d)(%d)  (%d)(%d)(%d)\n",i,ofs1,nvttst->iso_urb_uvc22->iso_frame_desc[i].actual_length,nvttst->iso_urb_uvc22->iso_frame_desc[i-1].actual_length,nvttst->iso_urb_uvc22->iso_frame_desc[i-2].actual_length);
					goto fail3;
					break;
				}
				ofs1 = (ofs1+nvttst->iso_urb_uvc22->iso_frame_desc[i].actual_length-UvcHeader[0])%255;
				FrmDataSz += nvttst->iso_urb_uvc22->iso_frame_desc[i].actual_length-UvcHeader[0];
				/* Header Compare */
				if(UvcHeader[0] == 12) {
					CounterValue =  UvcHeader[6]+(UvcHeader[7]<<8)+(UvcHeader[8]<<16)+(UvcHeader[9]<<24);

					tPTS = UvcHeader[2]+(UvcHeader[3]<<8)+(UvcHeader[4]<<16)+(UvcHeader[5]<<24);

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);
						PTS = tPTS;

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					if((UvcHeader[1] & 0x8C) != 0x8C) {
						printk("Header Err! 0x%02X\n",UvcHeader[1]);
						while(1);
					}

					if(tPTS != PTS) {
						printk("PTS ERR! 0x%08X 0x%08X\n",PTS,tPTS);
						while(1);
					}

				} else if (UvcHeader[0] == 2) {
					CounterValue =  0;

					if((UvcHeader[1]&0x1) != FID) {
						FID = (UvcHeader[1]&0x1);

						if(state==0) {
							printk("START ERR!\n");
							while(1);
						}
						state=0;
					}

					if(UvcHeader[1]&0x2) {
						//printk("UVC1(%u)= %d\n",(CounterValue/1000000*33),FrmDataSz);
						if(FrmDataSz != targetSz) {
							printk("SzErr = %d %d\n",FrmDataSz,targetSz);
						}

						FrmDataSz=0;
						targetSz++;
						if(state==1) {
							printk("STOP ERR!\n");
							while(1);
						}
						state=1;
					}

					//if((UvcHeader[1] & 0x8C) != 0x8C) {
					//	printk("Header Err! 0x%02X\n",UvcHeader[1]);
					//	while(1);
					//}

				} else {
					printk("header size error!\r\n");
					goto fail3;
				}



			}
		}
		//printk("UVC1(%u)= %d (%d)(%d)\n",(CounterValue/1000000*33),++Cnt,nvttst->iso_urb_uvc12->actual_length,ofs1);

    	if (time_after(jiffies, timeout)) {
			timeout = jiffies + msecs_to_jiffies(500);
			printk("UVC2(%ums)= %d\n",(CounterValue/1000000*33),targetSz-1);
		}

		//printk("                                                            ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);
		//printk("ISOIN= %d (%d)(%d)\n",++Cnt,nvttst->iso_urb_in->actual_length,ofs1);

		if (kthread_should_stop() || (nvttst->status))
			goto fail3;

}




fail3:


fail2:

	if(txfbuf)
		kfree(txfbuf);

	if(rxfbuf)
		kfree(rxfbuf);

fail1:

	if(nvttst->iso_urb_uvc21)
		usb_free_urb(nvttst->iso_urb_uvc21);

	if(nvttst->iso_urb_uvc22)
		usb_free_urb(nvttst->iso_urb_uvc22);

	while(!kthread_should_stop())
	{
		msleep(50);
	}

	complete(&nvttst->isoin_close_uvc2);
	printk("[%s]: thread closed.\n",__func__);
	return 0;
}
#endif


#endif

static int usb_nvttst_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_nvttst *nvttst;
	struct task_struct *th;

	/*
	printk("match_flags        0x%X\n",id->match_flags);
	printk("idVendor           0x%X\n",id->idVendor);
	printk("idProduct          0x%X\n",id->idProduct);
	printk("bcdDevice_lo       0x%X\n",id->bcdDevice_lo);
	printk("bcdDevice_hi       0x%X\n",id->bcdDevice_hi);
	printk("bDeviceClass       0x%X\n",id->bDeviceClass);
	printk("bDeviceSubClass    0x%X\n",id->bDeviceSubClass);
	printk("bDeviceProtocol    0x%X\n",id->bDeviceProtocol);
	printk("bInterfaceClass    0x%X\n",id->bInterfaceClass);
	printk("bInterfaceSubClass 0x%X\n",id->bInterfaceSubClass);
	printk("bInterfaceProtocol 0x%X\n",id->bInterfaceProtocol);
	printk("bInterfaceNumber   0x%X\n",id->bInterfaceNumber);
	*/

	nvttst = kzalloc(sizeof(struct usb_nvttst), GFP_KERNEL);
	if(!nvttst)
		return -ENOMEM;

	if(dev->manufacturer)
		strlcpy(nvttst->name, dev->manufacturer, sizeof(nvttst->name));

	if (dev->product) {
		if (dev->manufacturer)
			strlcat(nvttst->name, " ", sizeof(nvttst->name));
		strlcat(nvttst->name, dev->product, sizeof(nvttst->name));
	}

	if (!strlen(nvttst->name))
		snprintf(nvttst->name, sizeof(nvttst->name),
			 "Novatek. USB HOST TEST PROGRAM.");


	//nvttst->urb1 = usb_alloc_urb(0, GFP_KERNEL);
	//if (!nvttst->urb1)
	//	goto fail1;

	//nvttst->urb2 = usb_alloc_urb(0, GFP_KERNEL);
	//if (!nvttst->urb2)
	//	goto fail2;

	//nvttst->urb3 = usb_alloc_urb(8, GFP_KERNEL);
	//if (!nvttst->urb3)
	//	goto fail3;

	nvttst->cx_thread    = NULL;
	nvttst->bulk_thread  = NULL;
	nvttst->intr_thread  = NULL;
	nvttst->isoin_thread = NULL;
	nvttst->isoout_thread = NULL;
	nvttst->isouvc1_thread = NULL;
	nvttst->isouvc2_thread = NULL;


	nvttst->inf = intf;
	nvttst->usbdev = dev;

	init_completion(&nvttst->cx_close);
	init_completion(&nvttst->bulk_close);
	init_completion(&nvttst->intr_close);
	init_completion(&nvttst->isoin_close);
	init_completion(&nvttst->isoout_close);
	init_completion(&nvttst->isoin_close_uvc1);
	init_completion(&nvttst->isoin_close_uvc2);

	usb_set_intfdata(intf, nvttst);

	/* Start test program run */
#if 1
	th = kthread_run(usb_nvttst_CxRW_thread, nvttst, "nvttst-Cx-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->cx_thread = th;
#endif

#if 1
	th = kthread_run(usb_nvttst_BULKRW_thread, nvttst, "nvttst-bulk-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->bulk_thread = th;
#endif

#if 1
	th = kthread_run(usb_nvttst_INTRRW_thread, nvttst, "nvttst-intr-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->intr_thread = th;
#endif

#if 1
	th = kthread_run(usb_nvttst_ISOIN_thread, nvttst, "nvttst-isoin-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->isoin_thread = th;

#endif

#if 1
	th = kthread_run(usb_nvttst_ISOOUT_thread, nvttst, "nvttst-isoout-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->isoout_thread = th;
#endif

#if 0
	th = kthread_run(usb_nvttst_ISOIN_UVC1_thread, nvttst, "nvttst-isouvc1-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->isouvc1_thread = th;
#endif

#if 0
	th = kthread_run(usb_nvttst_ISOIN_UVC2_thread, nvttst, "nvttst-isouvc2-thread");
	if (IS_ERR(th)) {
		printk("+%s[%d]:  thread open error\n",__func__,__LINE__);
		goto fail1;
	}
	nvttst->isouvc2_thread = th;
#endif

	printk("+%s[%d]:  Name = %s\n",__func__,__LINE__,nvttst->name);
	return 0;

	// Sample codes for usb usages
	//usb_stor_acquire_resources
	//usb_stor_release_resources
	//usb_start_wait_urb

fail1:
	kfree(nvttst);
	return -ENOMEM;

}

static void usb_nvttst_disconnect(struct usb_interface *intf)
{
	struct usb_nvttst *nvttst = usb_get_intfdata(intf);

	printk("+%s[%d] 1\n",__func__,__LINE__);

	if (nvttst->cx_thread)
		kthread_stop(nvttst->cx_thread);
	else
		complete(&nvttst->cx_close);

	if (nvttst->bulk_thread)
		kthread_stop(nvttst->bulk_thread);
	else
		complete(&nvttst->bulk_close);

	if (nvttst->intr_thread)
		kthread_stop(nvttst->intr_thread);
	else
		complete(&nvttst->intr_close);

	if (nvttst->isoin_thread)
		kthread_stop(nvttst->isoin_thread);
	else
		complete(&nvttst->isoin_close);

	if (nvttst->isoout_thread)
		kthread_stop(nvttst->isoout_thread);
	else
		complete(&nvttst->isoout_close);


	if (nvttst->isouvc1_thread)
		kthread_stop(nvttst->isouvc1_thread);
	else
		complete(&nvttst->isoin_close_uvc1);

	if (nvttst->isouvc2_thread)
		kthread_stop(nvttst->isouvc2_thread);
	else
		complete(&nvttst->isoin_close_uvc2);

	printk("+%s[%d] 2\n",__func__,__LINE__);


	if(!wait_for_completion_timeout(&nvttst->cx_close, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	if(!wait_for_completion_timeout(&nvttst->bulk_close, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	if(!wait_for_completion_timeout(&nvttst->intr_close, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	if(!wait_for_completion_timeout(&nvttst->isoin_close, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	if(!wait_for_completion_timeout(&nvttst->isoout_close, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	if(!wait_for_completion_timeout(&nvttst->isoin_close_uvc1, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	if(!wait_for_completion_timeout(&nvttst->isoin_close_uvc2, msecs_to_jiffies(5000)))
		printk("[%s]:wait thread close timeout\n",__func__);

	printk("+%s[%d] 3\n",__func__,__LINE__);


	//if(nvttst->urb1)
	//	usb_free_urb(nvttst->urb1);
	//if(nvttst->urb2)
	//	usb_free_urb(nvttst->urb2);
	//if(nvttst->urb3)
	//	usb_free_urb(nvttst->urb3);


	kfree(nvttst);
	printk("+%s[%d] 4\n",__func__,__LINE__);

}

static struct usb_device_id usb_nvttst_id_table [] = {
	{
		.match_flags =    USB_DEVICE_ID_MATCH_VENDOR |
				  USB_DEVICE_ID_MATCH_PRODUCT | USB_DEVICE_ID_MATCH_DEV_LO,
		.idVendor    =    0x2310,
		.idProduct   =    0x5678,
		.bcdDevice_lo=    2
	},
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_nvttst_id_table);

static struct usb_driver usb_nvttst_driver = {
	.name		= "usbnvttst",
	.probe		= usb_nvttst_probe,
	.disconnect	= usb_nvttst_disconnect,
	.id_table	= usb_nvttst_id_table,
};

module_usb_driver(usb_nvttst_driver);

