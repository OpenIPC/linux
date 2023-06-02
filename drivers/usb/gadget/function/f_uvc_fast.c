
// #include <linux/usb/ch9.h>
// #include <linux/usb/gadget.h>
// #include <linux/usb/video.h>
// #include "uvc.h"

#ifdef UVC_FASTBOOT

struct uvc_control_value {
	uint8_t max;
	uint8_t min;
	uint8_t cur;
	uint8_t def;
	uint8_t res; //resolution -> step
};

typedef void (*uvc_control_setup_call)(struct uvc_request_data *r); /* device -> host */
typedef void (*uvc_control_data_call)(struct uvc_request_data *r); /* host -> device */

struct uvc_control_ops {
	uint8_t len;
	struct uvc_control_value val;
	uvc_control_setup_call fsetup;
	uvc_control_data_call fdata;
};

static int
uvc_send_response(struct uvc_device *uvc, struct uvc_request_data *data)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request *req = uvc->control_req;

	if (data->length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	req->length = min_t(unsigned int, uvc->event_length, data->length);
	req->zero = data->length < uvc->event_length;

	memcpy(req->buf, data->data, req->length);

	return usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
}

static void
uvc_interface_setup(uint8_t req, uint8_t cs,
			   struct uvc_request_data *resp)
{
	;
}

static void pu_backlight_setup(struct uvc_request_data *r)
{
	;
}

static void pu_backlight_data(struct uvc_request_data *r)
{
	;
}


static void pu_brightness_setup(struct uvc_request_data *r)
{
    ;
}

static void pu_brightness_data(struct uvc_request_data *r)
{
    ;
}

static void pu_contrast_setup(struct uvc_request_data *r)
{
    ;
}

static void pu_contrast_data(struct uvc_request_data *r)
{
    ;
}

static void pu_gain_setup(struct uvc_request_data *r)
{
	;
}

static void pu_gain_data(struct uvc_request_data *r)
{
	;
}

static void pu_power_freq_setup(struct uvc_request_data *r)
{
	;
}

static void pu_power_freq_data(struct uvc_request_data *r)
{
	;
}

static void pu_hue_setup(struct uvc_request_data *r)
{
	;
}

static void pu_hue_data(struct uvc_request_data *r)
{
	;
}

static void pu_saturation_setup(struct uvc_request_data *r)
{
    ;
}


static void pu_saturation_data(struct uvc_request_data *r)
{
    ;
}


static void pu_sharpness_setup(struct uvc_request_data *r)
{
    ;
}


static void pu_sharpness_data(struct uvc_request_data *r)
{
    ;
}


static void pu_gamma_setup(struct uvc_request_data *r)
{
	;
}

static void pu_gamma_data(struct uvc_request_data *r)
{
	;
}

static void pu_wb_temperature_setup(struct uvc_request_data *r)
{
	;
}

static void pu_wb_temperature_data(struct uvc_request_data *r)
{
	;
}

static void pu_wb_temperature_auto_setup(struct uvc_request_data *r)
{
	;
}

static void pu_wb_temperature_auto_data(struct uvc_request_data *r)
{
	;
}

static void pu_wb_component_setup(struct uvc_request_data *r)
{
	;
}

static void pu_wb_component_data(struct uvc_request_data *r)
{
	;
}

static void pu_wb_component_auto_setup(struct uvc_request_data *r)
{
	;
}

static void pu_wb_component_auto_data(struct uvc_request_data *r)
{
	;
}

static void pu_digital_multiplier_setup(struct uvc_request_data *r)
{
	;
}

static void pu_digital_multiplier_data(struct uvc_request_data *r)
{
	;
}

static void pu_digital_limit_setup(struct uvc_request_data *r)
{
	;
}

static void pu_digital_limit_data(struct uvc_request_data *r)
{
	;
}

static void pu_hue_auto_setup(struct uvc_request_data *r)
{
	;
}

static void pu_hue_auto_data(struct uvc_request_data *r)
{
	;
}

static void pu_analog_video_setup(struct uvc_request_data *r)
{
	;
}

static void pu_analog_video_data(struct uvc_request_data *r)
{
	;
}

static void pu_analog_lock_setup(struct uvc_request_data *r)
{
	;
}

static void pu_analog_lock_data(struct uvc_request_data *r)
{
	;
}

struct uvc_control_ops uvc_control_pu[] = {
	/* len, max, min, cur, def, res, setup, data */
	{ 0, { 0, 0, 0, 0, 0}, NULL, NULL },															//UVC_PU_CONTROL_UNDEFINED
	{ 2, { 255, 0, 128, 128, 1 }, pu_backlight_setup,				pu_backlight_data},				//UVC_PU_BACKLIGHT_COMPENSATION_CONTROL
	{ 2, { 180, 40, 128, 128, 1 }, pu_brightness_setup,				pu_brightness_data},			//UVC_PU_BRIGHTNESS_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_contrast_setup,				pu_contrast_data},				//UVC_PU_CONTRAST_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_gain_setup,					pu_gain_data},					//UVC_PU_GAIN_CONTROL
	{ 1, { 255, 0, 128, 128, 1 }, pu_power_freq_setup,				pu_power_freq_data},			//UVC_PU_POWER_LINE_FREQUENCY_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_hue_setup,						pu_hue_data},					//UVC_PU_HUE_CONTROL
	{ 2, { 200, 0, 128, 128, 1 }, pu_saturation_setup,				pu_saturation_data},			//UVC_PU_SATURATION_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_sharpness_setup,				pu_sharpness_data},				//UVC_PU_SHARPNESS_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_gamma_setup,					pu_gamma_data},					//UVC_PU_GAMMA_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_wb_temperature_setup,			pu_wb_temperature_data},		//UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL
	{ 1, { 255, 0, 128, 128, 1 }, pu_wb_temperature_auto_setup,		pu_wb_temperature_auto_data},	//UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL
	{ 4, { 255, 0, 128, 128, 1 }, pu_wb_component_setup,			pu_wb_component_data},			//UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL
	{ 1, { 255, 0, 128, 128, 1 }, pu_wb_component_auto_setup,		pu_wb_component_auto_data},		//UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_digital_multiplier_setup,		pu_digital_multiplier_data},	//UVC_PU_DIGITAL_MULTIPLIER_CONTROL
	{ 2, { 255, 0, 128, 128, 1 }, pu_digital_limit_setup,			pu_digital_limit_data},			//UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL
	{ 1, { 255, 0, 128, 128, 1 }, pu_hue_auto_setup,				pu_hue_auto_data},				//UVC_PU_HUE_AUTO_CONTROL
	{ 1, { 255, 0, 128, 128, 1 }, pu_analog_video_setup,			pu_analog_video_data},			//UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL
	{ 1, { 255, 0, 128, 128, 1 }, pu_analog_lock_setup,				pu_analog_lock_data},			//UVC_PU_ANALOG_LOCK_STATUS_CONTROL
};

static void
uvc_processing_unit_setup(uint8_t req, uint8_t cs,
			   struct uvc_request_data *resp) 
{
	;

	resp->length = uvc_control_pu[cs].len;

	switch (req) {
		case UVC_GET_LEN:
			resp->data[0] = resp->length;
			break;

		case UVC_GET_CUR:
			/* if the value could be changed only in data phase,
			 * give the cur value directly */
			//resp->data[0] = uvc_control_pu[cs].val.cur;
			(*uvc_control_pu[cs].fsetup)(resp);
			break;
		case UVC_GET_MIN:
			/* TODO diff type */
			resp->data[0] = uvc_control_pu[cs].val.min;
			break;
		case UVC_GET_MAX:
			resp->data[0] = uvc_control_pu[cs].val.max;
			break;
		case UVC_GET_DEF:
			resp->data[0] = uvc_control_pu[cs].val.def;
			break;
		case UVC_GET_RES:
			resp->data[0] = uvc_control_pu[cs].val.res;
			break;
		case UVC_GET_INFO:
			resp->data[0] = cs;
			break;
		default:
			break;
	}
}

static void
uvc_events_process_vc_setup(uint8_t req, uint8_t cs, uint8_t id,
			   struct uvc_request_data *resp)
{
	switch (id) {
	case UVC_INTERFACE_ID:
		uvc_interface_setup(req, cs, resp);
		break;
	case UVC_PROCESSING_UNIT_ID:
		uvc_processing_unit_setup(req, cs, resp);
		break;
	default:
		pr_err("# %s can not support termianl or UNIT id %d\n", __func__, id);
		break;
	}
}

void
uvc_events_process_class(struct uvc_device *uvc, struct usb_ctrlrequest *ctrl)
{
    struct uvc_request_data resp;

    memset(&resp, 0, sizeof resp);
    resp.length = 0;

    uvc_events_process_vc_setup(ctrl->bRequest, ctrl->wValue >> 8,
                ctrl->wIndex >> 8, &resp);
    uvc_send_response(uvc, &resp);

}

#endif
