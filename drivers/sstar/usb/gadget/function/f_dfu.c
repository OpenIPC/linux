/*
 * f_dfu.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

/*
 * f_dfu.c
 *
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "f_dfu.h"

static char *version_file = "/customer/version_info";
module_param(version_file, charp, S_IRUGO | S_IWUSR);

static char dfu_buf[100] = { 0 };
/*
 * Device Firmware Upgrade FUNCTION ... a testing vehicle for USB peripherals,
 *
 */
/*-------------------------------------------------------------------------*/
static const struct dfu_function_descriptor dfu_func = {
	.bLength = sizeof(dfu_func),
	.bDescriptorType = DFU_DT_FUNC,
	.bmAttributes = DFU_BIT_WILL_DETACH | DFU_BIT_MANIFESTATION_TOLERANT |
			DFU_BIT_CAN_UPLOAD | DFU_BIT_CAN_DNLOAD,
	.wDetachTimeOut = 0,
	.wTransferSize = DFU_USB_BUFSIZ,
	.bcdDFUVersion = cpu_to_le16(0x0110),
};

static struct usb_interface_descriptor dfu_intf = {
	.bLength = sizeof(dfu_intf),
	.bDescriptorType = USB_DT_INTERFACE,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_APP_SPEC,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 1,
	/* .iInterface = DYNAMIC */
};

/*
static struct usb_descriptor_header *dfu_runtime_descs[] = {
	(struct usb_descriptor_header *) &dfu_intf,
	NULL,
};
*/

static struct usb_descriptor_header *fs_dfu_descs[] = {
	(struct usb_descriptor_header *)&dfu_intf,
	(struct usb_descriptor_header *)&dfu_func,
	NULL,
};

static struct usb_descriptor_header *hs_dfu_descs[] = {
	(struct usb_descriptor_header *)&dfu_intf,
	(struct usb_descriptor_header *)&dfu_func,
	NULL,
};

/* super speed support: */
static struct usb_descriptor_header *ss_dfu_descs[] = {
	(struct usb_descriptor_header *)&dfu_intf,
	(struct usb_descriptor_header *)&dfu_func,
	NULL,
};

/* function-specific strings: */

static struct usb_string strings_dfu[] = {
	[0].s = "At Runtime Mode",
	[1].s = "At Dfu Mode",
	{} /* end of list */
};

static struct usb_gadget_strings stringtab_dfu = {
	.language = 0x0409, /* en-us */
	.strings = strings_dfu,
};

static struct usb_gadget_strings *dfu_function_strings[] = {
	&stringtab_dfu,
	NULL,
};

/*-------------------------------------------------------------------------*/
static void delay_reboot(struct work_struct *data)
{
	struct file *filp = NULL;
	char *str_path[] = { "/bin",  "/usr/bin",  "/config",
			     "/sbin", "/customer", "/etc" };
	char cmd_bin[] = "/fw_setenv";
	char cur_path[50] = "\0";

	char *cmd_argv[] = { cur_path, "enter_dfu_mode", "1", NULL };
	char *cmd_envp[] = {
		"HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin:/etc:/customer",
		NULL
	};
	int index = 0;

	for (index = 0; index < sizeof(str_path) / sizeof(char *); index++) {
		strcpy(cur_path, str_path[index]);
		strcat(cur_path, cmd_bin);

		filp = filp_open(cur_path, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(filp))
			continue;

		break;
	}

	if (IS_ERR_OR_NULL(filp)) {
		printk("no fw_setenv tool\n");
		return;
	} else {
		printk("Found %s tool\n", cur_path);
		filp_close(filp, NULL);
		filp = NULL;
		call_usermodehelper(cur_path, cmd_argv, cmd_envp,
				    UMH_WAIT_PROC);
	}

	kernel_restart("to Uboot DFU Mode");
}

static int dfu_function_bind(struct usb_configuration *c,
			     struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_dfu *dfu = func_to_dfu(f);
	int id;
	int ret;

	struct file *fp = NULL;
	struct inode *inode;

	mm_segment_t old_fs;
	loff_t fsize;

	fp = filp_open(version_file, O_RDWR, 0);
	if (IS_ERR(fp)) {
		printk("%s open fail\n", version_file);
	} else {
		inode = file_inode(fp);
		fsize = inode->i_size;
		if (fsize < 100) {
			old_fs = get_fs();
			set_fs(KERNEL_DS);
			kernel_read(fp, dfu_buf, fsize + 1, &(fp->f_pos));
			filp_close(fp, NULL);
			set_fs(old_fs);
			strings_dfu[0].s = dfu_buf;
		} else
			printk("version and production info too long show default info: %s\n",
			       strings_dfu[0].s);
	}
	printk("strings_dfu[0].s  = %s\n", strings_dfu[0].s);

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	dfu_intf.bInterfaceNumber = id;

	/* allocate string ID(s) */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dfu[0].id = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dfu[1].id = id;

	dfu_intf.iInterface = strings_dfu[0].id;
	dfu_intf.bInterfaceProtocol = 1;

	/* assign descriptors */
	ret = usb_assign_descriptors(f, fs_dfu_descs, hs_dfu_descs,
				     ss_dfu_descs, NULL);
	if (ret)
		return ret;

	dfu->dfu_state = DFU_STATE_appIDLE;
	dfu->dfu_status = DFU_STATUS_OK;
	INIT_WORK(&dfu->reboot, delay_reboot);

	DBG(cdev, "%s speed %s\n",
	    (gadget_is_superspeed(c->cdev->gadget) ?
			   "super" :
			   (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
	    f->name);
	return 0;
}

static void dfu_function_unbind(struct usb_configuration *c,
				struct usb_function *f)
{
	//struct f_dfu	*dfu = func_to_dfu(f);
}

static void dfu_function_free(struct usb_function *f)
{
	struct f_dfu_opts *opts;

	opts = container_of(f->fi, struct f_dfu_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);

	usb_free_all_descriptors(f);
	kfree(func_to_dfu(f));
}

static int dfu_function_set_alt(struct usb_function *f, unsigned intf,
				unsigned alt)
{
	/* we know alt is dfu */

	return 0;
}

static void dfu_function_disable(struct usb_function *f)
{
}

/*-------------------------------------------------------------------------*/

static void dnload_request_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->length == 0)
		printk("DOWNLOAD ... OK\nCtrl+C to exit ...\n");
}

static void handle_getstatus(struct f_dfu *f_dfu, struct usb_request *req)
{
	struct dfu_status *dstat = (struct dfu_status *)req->buf;

	switch (f_dfu->dfu_state) {
	case DFU_STATE_dfuDNLOAD_SYNC:
	case DFU_STATE_dfuDNBUSY:
		f_dfu->dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
		break;
	case DFU_STATE_dfuMANIFEST_SYNC:
		break;
	default:
		break;
	}

	/* send status response */
	dstat->bStatus = f_dfu->dfu_status;
	dstat->bwPollTimeout[0] = 0;
	dstat->bwPollTimeout[1] = 0;
	dstat->bwPollTimeout[2] = 0;
	dstat->bState = f_dfu->dfu_state;
	dstat->iString = dfu_intf.bInterfaceProtocol == 1 ? 0 : 1;
}

static void handle_getstate(struct f_dfu *f_dfu, struct usb_request *req)
{
	((u8 *)req->buf)[0] = f_dfu->dfu_state;
	req->actual = sizeof(u8);
}

static inline void to_dfu_mode(struct f_dfu *f_dfu)
{
	struct usb_function *f = &f_dfu->usb_function;

	dfu_intf.iInterface = strings_dfu[1].id;
	dfu_intf.bInterfaceProtocol = 2;

	usb_free_all_descriptors(f);
	usb_assign_descriptors(f, fs_dfu_descs, hs_dfu_descs, ss_dfu_descs,
			       NULL);

	//to uboot
	schedule_work(&f_dfu->reboot);
}

static inline void to_runtime_mode(struct f_dfu *f_dfu)
{
	struct usb_function *f = &f_dfu->usb_function;

	dfu_intf.iInterface = strings_dfu[0].id;
	dfu_intf.bInterfaceProtocol = 1;

	usb_free_all_descriptors(f);
	usb_assign_descriptors(f, fs_dfu_descs, hs_dfu_descs, ss_dfu_descs,
			       NULL);
}

static int handle_upload(struct f_dfu *f_dfu, struct usb_request *req, u16 len)
{
	return len;
}

static int handle_dnload(struct f_dfu *f_dfu, struct usb_gadget *gadget,
			 u16 len)
{
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	struct usb_request *req = cdev->req;

	if (len == 0)
		f_dfu->dfu_state = DFU_STATE_dfuMANIFEST_SYNC;

	req->complete = dnload_request_complete;

	return len;
}
/*-------------------------------------------------------------------------*/
/* DFU state machine  */
static int state_app_idle(struct f_dfu *f_dfu,
			  const struct usb_ctrlrequest *ctrl,
			  struct usb_gadget *gadget, struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	case USB_REQ_DFU_DETACH:
		f_dfu->dfu_state = DFU_STATE_appDETACH;
		to_dfu_mode(f_dfu);
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		value = RET_ZLP;
		break;
	default:
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_app_detach(struct f_dfu *f_dfu,
			    const struct usb_ctrlrequest *ctrl,
			    struct usb_gadget *gadget, struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_appIDLE;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_idle(struct f_dfu *f_dfu,
			  const struct usb_ctrlrequest *ctrl,
			  struct usb_gadget *gadget, struct usb_request *req)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 len = le16_to_cpu(ctrl->wLength);
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_DNLOAD:
		if (len == 0) {
			f_dfu->dfu_state = DFU_STATE_dfuERROR;
			value = RET_STALL;
			break;
		}
		f_dfu->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
		f_dfu->blk_seq_num = w_value;
		value = handle_dnload(f_dfu, gadget, len);
		break;
	case USB_REQ_DFU_UPLOAD:
		f_dfu->dfu_state = DFU_STATE_dfuUPLOAD_IDLE;
		f_dfu->blk_seq_num = 0;
		value = handle_upload(f_dfu, req, len);
		break;
	case USB_REQ_DFU_ABORT:
		/* no zlp? */
		value = RET_ZLP;
		break;
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	case USB_REQ_DFU_DETACH:
		/*
		 * Proprietary extension: 'detach' from idle mode and
		 * get back to runtime mode in case of USB Reset.  As
		 * much as I dislike this, we just can't use every USB
		 * bus reset to switch back to runtime mode, since at
		 * least the Linux USB stack likes to send a number of
		 * resets in a row :(
		 */
		f_dfu->dfu_state = DFU_STATE_dfuMANIFEST_WAIT_RST;
		to_runtime_mode(f_dfu);
		f_dfu->dfu_state = DFU_STATE_appIDLE;
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_dnload_sync(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_dnbusy(struct f_dfu *f_dfu,
			    const struct usb_ctrlrequest *ctrl,
			    struct usb_gadget *gadget, struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_dnload_idle(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 len = le16_to_cpu(ctrl->wLength);
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_DNLOAD:
		f_dfu->dfu_state = DFU_STATE_dfuDNLOAD_SYNC;
		f_dfu->blk_seq_num = w_value;
		value = handle_dnload(f_dfu, gadget, len);
		break;
	case USB_REQ_DFU_ABORT:
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		value = RET_ZLP;
		break;
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_manifest_sync(struct f_dfu *f_dfu,
				   const struct usb_ctrlrequest *ctrl,
				   struct usb_gadget *gadget,
				   struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		/* We're MainfestationTolerant */
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		handle_getstatus(f_dfu, req);
		f_dfu->blk_seq_num = 0;
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_upload_idle(struct f_dfu *f_dfu,
				 const struct usb_ctrlrequest *ctrl,
				 struct usb_gadget *gadget,
				 struct usb_request *req)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 len = le16_to_cpu(ctrl->wLength);
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_UPLOAD:
		/* state transition if less data then requested */
		f_dfu->blk_seq_num = w_value;
		value = handle_upload(f_dfu, req, len);
		if (value >= 0 && value < len)
			f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		break;
	case USB_REQ_DFU_ABORT:
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		/* no zlp? */
		value = RET_ZLP;
		break;
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static int state_dfu_error(struct f_dfu *f_dfu,
			   const struct usb_ctrlrequest *ctrl,
			   struct usb_gadget *gadget, struct usb_request *req)
{
	int value = 0;

	switch (ctrl->bRequest) {
	case USB_REQ_DFU_GETSTATUS:
		handle_getstatus(f_dfu, req);
		value = RET_STAT_LEN;
		break;
	case USB_REQ_DFU_GETSTATE:
		handle_getstate(f_dfu, req);
		break;
	case USB_REQ_DFU_CLRSTATUS:
		f_dfu->dfu_state = DFU_STATE_dfuIDLE;
		f_dfu->dfu_status = DFU_STATUS_OK;
		/* no zlp? */
		value = RET_ZLP;
		break;
	default:
		f_dfu->dfu_state = DFU_STATE_dfuERROR;
		value = RET_STALL;
		break;
	}

	return value;
}

static dfu_state_fn dfu_state[] = {
	state_app_idle, /* DFU_STATE_appIDLE */
	state_app_detach, /* DFU_STATE_appDETACH */
	state_dfu_idle, /* DFU_STATE_dfuIDLE */
	state_dfu_dnload_sync, /* DFU_STATE_dfuDNLOAD_SYNC */
	state_dfu_dnbusy, /* DFU_STATE_dfuDNBUSY */
	state_dfu_dnload_idle, /* DFU_STATE_dfuDNLOAD_IDLE */
	state_dfu_manifest_sync, /* DFU_STATE_dfuMANIFEST_SYNC */
	NULL, /* DFU_STATE_dfuMANIFEST */
	NULL, /* DFU_STATE_dfuMANIFEST_WAIT_RST */
	state_dfu_upload_idle, /* DFU_STATE_dfuUPLOAD_IDLE */
	state_dfu_error /* DFU_STATE_dfuERROR */
};

static int dfu_function_setup(struct usb_function *f,
			      const struct usb_ctrlrequest *ctrl)
{
	struct usb_configuration *c = f->config;
	struct usb_request *req = c->cdev->req;
	struct f_dfu *dfu = func_to_dfu(f);
	struct usb_gadget *gadget = f->config->cdev->gadget;
	int value = -EOPNOTSUPP;
	//u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);
	u8 req_type = ctrl->bRequestType & USB_TYPE_MASK;

	INFO(c->cdev,
	     "0x%p dfu->dfu_state: 0x%x bReqType: 0x%x bRequest: 0x%x "
	     "w_value: 0x%x w_length: 0x%x\n",
	     dfu, dfu->dfu_state, req_type, ctrl->bRequest, w_value, w_length);

	if (req_type == USB_TYPE_STANDARD) {
		if (ctrl->bRequest == USB_REQ_GET_DESCRIPTOR &&
		    (w_value >> 8) == DFU_DT_FUNC) {
			value = min_t(u16, w_length, (u16)sizeof(dfu_func));
			memcpy(req->buf, &dfu_func, value);
		}
	} else /* DFU specific request */
		value = dfu_state[dfu->dfu_state](dfu, ctrl, gadget, req);

	if (value >= 0) {
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue(gadget->ep0, req, 0);
		if (value < 0) {
			printk("ep_queue --> %d\n", value);
			req->status = 0;
		}
	}

	return value;
}

static void dfu_function_suspend(struct usb_function *f)
{
	//todo
}

static void dfu_function_resume(struct usb_function *f)
{
	//todo
}

static struct usb_function *dfu_alloc(struct usb_function_instance *fi)
{
	struct f_dfu *dfu;
	struct f_dfu_opts *dfu_opts;

	dfu = kzalloc(sizeof(*dfu), GFP_KERNEL);
	if (!dfu)
		return ERR_PTR(-ENOMEM);

	dfu_opts = container_of(fi, struct f_dfu_opts, func_inst);

	mutex_lock(&dfu_opts->lock);
	dfu_opts->refcnt++;
	mutex_unlock(&dfu_opts->lock);

	dfu->usb_function.name = "dfu";
	dfu->usb_function.bind = dfu_function_bind;
	dfu->usb_function.unbind = dfu_function_unbind;
	dfu->usb_function.setup = dfu_function_setup;
	dfu->usb_function.set_alt = dfu_function_set_alt;
	dfu->usb_function.disable = dfu_function_disable;
	dfu->usb_function.resume = dfu_function_resume;
	dfu->usb_function.suspend = dfu_function_suspend;
	dfu->usb_function.strings = dfu_function_strings;

	dfu->usb_function.free_func = dfu_function_free;

	return &dfu->usb_function;
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
	.release = dfu_attr_release,
};

static struct config_item_type dfu_func_type = {
	.ct_item_ops = &dfu_item_ops,
	.ct_attrs = NULL,
	.ct_owner = THIS_MODULE,
};

static void dfu_free_instance(struct usb_function_instance *fi)
{
	struct f_dfu_opts *dfu_opts;

	dfu_opts = container_of(fi, struct f_dfu_opts, func_inst);
	kfree(dfu_opts);
}

static struct usb_function_instance *dfu_alloc_instance(void)
{
	struct f_dfu_opts *dfu_opts;

	dfu_opts = kzalloc(sizeof(*dfu_opts), GFP_KERNEL);
	if (!dfu_opts)
		return ERR_PTR(-ENOMEM);
	mutex_init(&dfu_opts->lock);
	dfu_opts->func_inst.free_func_inst = dfu_free_instance;

	mutex_lock(&dfu_opts->lock);
	config_group_init_type_name(&dfu_opts->func_inst.group, "",
				    &dfu_func_type);
	mutex_unlock(&dfu_opts->lock);

	return &dfu_opts->func_inst;
}
DECLARE_USB_FUNCTION_INIT(dfu, dfu_alloc_instance, dfu_alloc);

MODULE_AUTHOR("Claude Rao");
MODULE_LICENSE("GPL");
