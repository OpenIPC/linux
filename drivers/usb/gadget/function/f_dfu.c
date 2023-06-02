// SPDX-License-Identifier: GPL-2.0+
/*
 * f_dfu.c - Device Firmware Update USB function
 *
 * Copyright (C) 2021 Rockchip Electronics Co., Ltd
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/reboot.h>

#include "f_dfu.h"

struct f_dfu {
	struct usb_function	function;
	struct work_struct	dfu_work;
	enum dfu_state		dfu_state;
	unsigned int		dfu_status;
	u8			altsetting;
};

static inline struct f_dfu *func_to_dfu(struct usb_function *f)
{
	return container_of(f, struct f_dfu, function);
}

/*-------------------------------------------------------------------------*/

typedef int (*dfu_state_fn) (struct f_dfu *,
			     const struct usb_ctrlrequest *,
			     struct usb_request *);

static const struct dfu_function_descriptor dfu_func = {
	.bLength =		sizeof(dfu_func),
	.bDescriptorType =	DFU_DT_FUNC,
	.bmAttributes =		DFU_BIT_CAN_DNLOAD,
	.wDetachTimeOut =	0,
	.wTransferSize =	DFU_USB_BUFSIZ,
	.bcdDFUVersion =	cpu_to_le16(0x0110),
};

/* interface descriptor: */

static struct usb_interface_descriptor dfu_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_APP_SPEC,
	.bInterfaceSubClass =	1,
	.bInterfaceProtocol =	1,
	/* .iInterface = DYNAMIC */
};

static struct usb_descriptor_header *dfu_fs_function[] = {
	(struct usb_descriptor_header *) &dfu_interface_desc,
	(struct usb_descriptor_header *) &dfu_func,
	NULL,
};

static struct usb_descriptor_header *dfu_hs_function[] = {
	(struct usb_descriptor_header *) &dfu_interface_desc,
	(struct usb_descriptor_header *) &dfu_func,
	NULL,
};

static struct usb_descriptor_header *dfu_ss_function[] = {
	(struct usb_descriptor_header *) &dfu_interface_desc,
	(struct usb_descriptor_header *) &dfu_func,
	NULL,
};

/* string descriptors: */

static struct usb_string dfu_string_defs[] = {
	[0].s = "Device Firmware Upgrade",
	{  } /* end of list */
};

static struct usb_gadget_strings dfu_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		dfu_string_defs,
};

static struct usb_gadget_strings *dfu_strings[] = {
	&dfu_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/

static int dfu_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_dfu		*f_dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	dev_dbg(&cdev->gadget->dev,
		"%s: intf:%d alt:%d\n", __func__, intf, alt);

	if (alt > 0) {
		f_dfu->altsetting = alt;
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		f_dfu->dfu_status = DFU_STATUS_OK;
	}

	return 0;
}

static int handle_getstatus(struct usb_request *req)
{
	struct f_dfu *f_dfu = req->context;
	struct dfu_status *dstat = (struct dfu_status *)req->buf;

	/* send status response */
	dstat->bStatus = f_dfu->dfu_status;
	dstat->bState = f_dfu->dfu_state;
	dstat->iString = 0;
	dstat->bwPollTimeout[0] = 0;
	dstat->bwPollTimeout[1] = 0;
	dstat->bwPollTimeout[2] = 0;

	return sizeof(struct dfu_status);
}

static int handle_getstate(struct usb_request *req)
{
	struct f_dfu *f_dfu = req->context;

	((u8 *)req->buf)[0] = f_dfu->dfu_state;

	return sizeof(u8);
}

static void dfu_work(struct work_struct *data)
{
	struct f_dfu *f_dfu = container_of(data, struct f_dfu, dfu_work);

	if (f_dfu->dfu_state == DFU_STATE_appDETACH) {
		pr_info("reboot to dfu mode");
		kernel_restart("dfu");
	}
}

static void dfu_complete_setup(struct usb_ep *ep, struct usb_request *req)
{
	struct f_dfu	*f_dfu = req->context;

	if (f_dfu->dfu_state == DFU_STATE_appDETACH)
		schedule_work(&f_dfu->dfu_work);
}

static int state_app_idle(struct f_dfu *f_dfu,
			  const struct usb_ctrlrequest *ctrl,
			  struct usb_request *req)
{
	int value = 0;

	req->context = f_dfu;
	req->complete = dfu_complete_setup;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		value = handle_getstatus(req);
		break;
	case USB_REQ_DFU_GETSTATE:
		value = handle_getstate(req);
		break;
	case USB_REQ_DFU_DETACH:
		f_dfu->dfu_state = DFU_STATE_appDETACH;
		value = RET_ZLP;
		break;
	default:
		value = RET_STALL;
		break;
	}

	return value;
}

static dfu_state_fn dfu_state[] = {
	state_app_idle,		/* DFU_STATE_appIDLE */
};

static int dfu_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*f_dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	u16 len = le16_to_cpu(ctrl->wLength);
	u8 req_type = ctrl->bRequestType & USB_TYPE_MASK;
	int value = 0;

	dev_dbg(&cdev->gadget->dev,
		"req_type: 0x%x ctrl->bRequest: 0x%x f_dfu->dfu_state: 0x%x\n",
		req_type, ctrl->bRequest, f_dfu->dfu_state);

	if (req_type == USB_TYPE_STANDARD) {
		/* TODO */
	} else {
		/* DFU specific request */
		value = dfu_state[f_dfu->dfu_state] (f_dfu, ctrl, cdev->req);
	}

	if (value >= 0) {
		cdev->req->length = value;
		cdev->req->zero = value < len;
		value = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (value < 0) {
			ERROR(cdev, "%s setup response queue error\n",
			      __func__);
			cdev->req->status = 0;
		}
	}

	return value;
}

static void dfu_disable(struct usb_function *f)
{
	/* TODO */
}

/*-------------------------------------------------------------------------*/

/* dfu function driver setup/binding */

static int dfu_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_dfu		*f_dfu = func_to_dfu(f);
	int			status;

	if (dfu_string_defs[0].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		dfu_string_defs[0].id = status;
		dfu_interface_desc.iInterface = status;
	}

	/* allocate instance-specific interface IDs */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	dfu_interface_desc.bInterfaceNumber = status;

	f_dfu->dfu_state = DFU_STATE_appIDLE;
	f_dfu->dfu_status = DFU_STATUS_OK;

	status = usb_assign_descriptors(f, dfu_fs_function, dfu_hs_function,
			dfu_ss_function, NULL);
	if (status)
		goto fail;

	INIT_WORK(&f_dfu->dfu_work, dfu_work);

	return 0;

fail:
	ERROR(cdev, "%s: can't bind, err %d\n", f->name, status);

	return status;
}

static inline struct f_dfu_opts *to_f_dfu_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_dfu_opts,
			    func_inst.group);
}

static void dfu_attr_release(struct config_item *item)
{
	struct f_dfu_opts *opts = to_f_dfu_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations dfu_item_ops = {
	.release	= dfu_attr_release,
};

static const struct config_item_type dfu_func_type = {
	.ct_item_ops	= &dfu_item_ops,
	.ct_owner	= THIS_MODULE,
};

static void dfu_free_inst(struct usb_function_instance *f)
{
	struct f_dfu_opts *opts;

	opts = container_of(f, struct f_dfu_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *dfu_alloc_inst(void)
{
	struct f_dfu_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	opts->func_inst.free_func_inst = dfu_free_inst;
	config_group_init_type_name(&opts->func_inst.group, "",
				    &dfu_func_type);

	return &opts->func_inst;
}

static void dfu_free(struct usb_function *f)
{
	struct f_dfu *f_dfu;

	f_dfu = func_to_dfu(f);
	kfree(f_dfu);
}

static void dfu_unbind(struct usb_configuration *c, struct usb_function *f)
{
	usb_free_all_descriptors(f);
}

static struct usb_function *dfu_alloc(struct usb_function_instance *fi)
{
	struct f_dfu	*f_dfu;

	/* allocate and initialize one new instance */
	f_dfu = kzalloc(sizeof(*f_dfu), GFP_KERNEL);
	if (!f_dfu)
		return ERR_PTR(-ENOMEM);

	f_dfu->function.name = "gser";
	f_dfu->function.strings = dfu_strings;
	f_dfu->function.bind = dfu_bind;
	f_dfu->function.unbind = dfu_unbind;
	f_dfu->function.set_alt = dfu_set_alt;
	f_dfu->function.free_func = dfu_free;
	f_dfu->function.setup = dfu_setup;
	f_dfu->function.disable = dfu_disable;

	return &f_dfu->function;
}

DECLARE_USB_FUNCTION_INIT(dfu, dfu_alloc_inst, dfu_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Al Borchers");
MODULE_AUTHOR("David Brownell");
