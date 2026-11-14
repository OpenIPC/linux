/*
 * f_audio.c -- USB Audio class function driver
  *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/atomic.h>

#include "u_ak_uac1.h"

static int generic_control_set(struct usb_audio_control *con, u8 cmd, int value);
static int generic_control_get(struct usb_audio_control *con, u8 cmd);

static int generic_ep_control_set(struct ak_usb_audio_ep_control *con, u8 cmd, int value);
static int generic_ep_control_get(struct ak_usb_audio_ep_control *con, u8 cmd);
/*
 * DESCRIPTORS ... most are static, but strings and full
 * configuration descriptors are built on demand.
 */

static struct usb_interface_assoc_descriptor uac_iad = {
	.bLength		= sizeof(uac_iad),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0,
	.bInterfaceCount	= 3,
	.bFunctionClass		= USB_CLASS_AUDIO,
	.bFunctionSubClass	= 0x00,
	.bFunctionProtocol	= 0x00,
	.iFunction		= 0,
};

/*
 * We have two interfaces- AudioControl and AudioStreaming
 */
#define F_AUDIO_NUM_INTERFACES	2

/* B.3.1  Standard AC Interface Descriptor */
static struct usb_interface_descriptor ac_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOCONTROL,
};

/*
 * The number of AudioStreaming and MIDIStreaming interfaces
 * in the Audio Interface Collection
 */
DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);

#define UAC_DT_AC_HEADER_LENGTH	UAC_DT_AC_HEADER_SIZE(F_AUDIO_NUM_INTERFACES)

/* 2 input terminal, 2 output terminal and 2 feature unit */
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH + UAC_DT_INPUT_TERMINAL_SIZE * 2 \
	+ UAC_DT_OUTPUT_TERMINAL_SIZE * 2 + UAC_DT_FEATURE_UNIT_SIZE(0) * 2)

/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_2 ac_header_desc = {
	.bLength =		UAC_DT_AC_HEADER_LENGTH,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_HEADER,
	.bcdADC =		__constant_cpu_to_le16(0x0100),
	.wTotalLength =		__constant_cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bInCollection =	F_AUDIO_NUM_INTERFACES,
};

#define MIC_INPUT_TERMINAL_ID 1
static struct uac_input_terminal_descriptor mic_input_terminal_desc = {
	.bLength =		UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID = MIC_INPUT_TERMINAL_ID,
	.wTerminalType = UAC_INPUT_TERMINAL_MICROPHONE,
	.bAssocTerminal =	0,
    .bNrChannels = 1, // mono
	.wChannelConfig =	1, // Left Front
};

DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);

#define MIC_FEATURE_UNIT_ID 2
static struct uac_feature_unit_descriptor_0 mic_feature_unit_desc = {
	.bLength		= UAC_DT_FEATURE_UNIT_SIZE(0),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_FEATURE_UNIT,
	.bUnitID		= MIC_FEATURE_UNIT_ID,
	.bSourceID		= MIC_INPUT_TERMINAL_ID,
	.bControlSize		= 2,
	.bmaControls[0]		= (UAC_FU_MUTE | UAC_FU_VOLUME),
};

#define MIC_OUTPUT_TERMINAL_ID 3
static struct uac1_output_terminal_descriptor mic_output_terminal_desc = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= MIC_OUTPUT_TERMINAL_ID,
	.wTerminalType		= UAC_TERMINAL_STREAMING,
	.bSourceID		= MIC_FEATURE_UNIT_ID,
};

#define SPK_INPUT_TERMINAL_ID 4
static struct uac_input_terminal_descriptor spk_input_terminal_desc = {
	.bLength =		UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID =		SPK_INPUT_TERMINAL_ID,
	.wTerminalType =	UAC_TERMINAL_STREAMING,
	.bAssocTerminal =	0,
	.bNrChannels = 1, // mono
	.wChannelConfig =	1, // Left Front
};

#define SPK_FEATURE_UNIT_ID 5
static struct uac_feature_unit_descriptor_0 spk_feature_unit_desc = {
	.bLength		= UAC_DT_FEATURE_UNIT_SIZE(0),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_FEATURE_UNIT,
	.bUnitID		= SPK_FEATURE_UNIT_ID,
	.bSourceID		= SPK_INPUT_TERMINAL_ID,
	.bControlSize		= 2,
	.bmaControls[0]		= (UAC_FU_MUTE | UAC_FU_VOLUME),
};

struct ak_usb_audio_control {
    struct usb_audio_control con;
    struct gaudio_snd_dev *snd;
};

static struct ak_usb_audio_control spk_mute_control = {
    .con = {
    .list = LIST_HEAD_INIT(spk_mute_control.con.list),
    .name = "Speaker_Mute",
    .type = UAC_FU_MUTE,
    .set = generic_control_set,
    .get = generic_control_get,
    },
};

static struct ak_usb_audio_control spk_volume_control = {
    .con = {
        .list = LIST_HEAD_INIT(spk_volume_control.con.list),
        .name = "Speaker_Volume",
        .type = UAC_FU_VOLUME,
        .set = generic_control_set,
        .get = generic_control_get,
    },
};

static struct usb_audio_control_selector spk_feature_unit = {
    .list = LIST_HEAD_INIT(spk_feature_unit.list),
    .id = SPK_FEATURE_UNIT_ID,
    .name = "Speaker Mute & Volume Control",
    .type = UAC_FEATURE_UNIT,
    .desc = (struct usb_descriptor_header *)&spk_feature_unit_desc,
};

static struct ak_usb_audio_control mic_mute_control = {
    .con = {
        .list = LIST_HEAD_INIT(mic_mute_control.con.list),
        .name = "MIC_Mute",
        .type = UAC_FU_MUTE,
        .set = generic_control_set,
        .get = generic_control_get,
    },
};

static struct ak_usb_audio_control mic_volume_control = {
    .con = {
        .list = LIST_HEAD_INIT(mic_volume_control.con.list),
        .name = "MIC_Volume",
        .type = UAC_FU_VOLUME,
        .set = generic_control_set,
        .get = generic_control_get,
    },
};

static struct usb_audio_control_selector mic_feature_unit = {
    .list = LIST_HEAD_INIT(mic_feature_unit.list),
    .id = MIC_FEATURE_UNIT_ID,
    .name = "MIC Mute & Volume Control",
    .type = UAC_FEATURE_UNIT,
    .desc = (struct usb_descriptor_header *)&mic_feature_unit_desc,
};

#define SPK_OUTPUT_TERMINAL_ID 6
static struct uac1_output_terminal_descriptor spk_output_terminal_desc = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= SPK_OUTPUT_TERMINAL_ID,
	.wTerminalType		= UAC_OUTPUT_TERMINAL_SPEAKER,
	.bSourceID		= SPK_FEATURE_UNIT_ID,
};

/* B.4.1  Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_mic_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_mic_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_mic_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink = MIC_OUTPUT_TERMINAL_ID,
	.bDelay = 0,
	.wFormatTag =		UAC_FORMAT_TYPE_I_PCM,
};

DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(2);

static struct uac_format_type_i_discrete_descriptor_2 as_mic_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(2),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
    .bNrChannels = 1,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		2,
};

/* Standard ISO IN Endpoint Descriptor */
static struct usb_endpoint_descriptor as_mic_in_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_SYNC_ASYNC
        | USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(512),
	.bInterval =		4,
};

/* Class-specific AS ISO IN Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_mic_iso_in_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes = 	1, // sampling freq
	.bLockDelayUnits =	0,
	.wLockDelay =		__constant_cpu_to_le16(0),
};

static struct ak_usb_audio_ep_control mic_ep_sample_rate_control = {
    .name = "MIC_Samplerate",
    .list = LIST_HEAD_INIT(mic_ep_sample_rate_control.list),
    .cs = UAC_EP_CS_ATTR_SAMPLE_RATE,
    .set = generic_ep_control_set,
    .get = generic_ep_control_get,
};

/* B.4.1  Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_spk_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_spk_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_spk_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink = SPK_INPUT_TERMINAL_ID,
	.bDelay = 0,
	.wFormatTag =		UAC_FORMAT_TYPE_I_PCM,
};

//DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(1);

static struct uac_format_type_i_discrete_descriptor_2 as_spk_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(2),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
    .bNrChannels = 1,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		2,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_spk_out_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ASYNC
        | USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(512),
	.bInterval =		4,
};

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_spk_iso_out_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes = 	1, // sampling freq
	.bLockDelayUnits =	0,
	.wLockDelay =		__constant_cpu_to_le16(0),
};

static struct ak_usb_audio_ep_control spk_ep_sample_rate_control = {
    .name = "Speaker_Samplerate",
    .list = LIST_HEAD_INIT(spk_ep_sample_rate_control.list),
    .cs = UAC_EP_CS_ATTR_SAMPLE_RATE,
    .set = generic_ep_control_set,
    .get = generic_ep_control_get,
};

static struct usb_descriptor_header *f_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,

	(struct usb_descriptor_header *)&mic_input_terminal_desc,
	(struct usb_descriptor_header *)&mic_feature_unit_desc,
	(struct usb_descriptor_header *)&mic_output_terminal_desc,
	(struct usb_descriptor_header *)&spk_input_terminal_desc,
	(struct usb_descriptor_header *)&spk_feature_unit_desc,
	(struct usb_descriptor_header *)&spk_output_terminal_desc,

	(struct usb_descriptor_header *)&as_mic_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_mic_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_mic_header_desc,
	(struct usb_descriptor_header *)&as_mic_type_i_desc,
	(struct usb_descriptor_header *)&as_mic_in_ep_desc,
	(struct usb_descriptor_header *)&as_mic_iso_in_desc,

	(struct usb_descriptor_header *)&as_spk_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_spk_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_spk_header_desc,
	(struct usb_descriptor_header *)&as_spk_type_i_desc,
	(struct usb_descriptor_header *)&as_spk_out_ep_desc,
	(struct usb_descriptor_header *)&as_spk_iso_out_desc,
	NULL,
};

enum {
	STR_AC_IF,
};

static struct usb_string strings_uac1[] = {
	[STR_AC_IF].s = "Anyka UAC",
	{ },
};

static struct usb_gadget_strings str_uac1 = {
	.language = 0x0409,	/* en-us */
	.strings = strings_uac1,
};

static struct usb_gadget_strings *uac1_strings[] = {
	&str_uac1,
	NULL,
};

/*-------------------------------------------------------------------------*/

#if 0
static void f_audio_playback_work(struct work_struct *data)
{
	struct f_audio *audio = container_of(data, struct f_audio,
					playback_work);
	struct f_audio_buf *data_buf;

	spin_lock_irq(&audio->lock);
	if (list_empty(&audio->playback_data)) {
		spin_unlock_irq(&audio->lock);
		return;
	}
	data_buf = list_first_entry(&audio->play_data,
			struct f_audio_buf, list);
	list_del(&data_buf->list);
	spin_unlock_irq(&audio->lock);

	//u_audio_playback(&audio->card, play_buf->buf, play_buf->actual);
	f_audio_buffer_free(data_buf);
}
#endif

static int f_audio_in_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
    struct gaudio_snd_dev *snd = &audio->card.capture;

    kfree(req->buf);
    usb_ep_free_request(ep, req);
    snd->capture_req_count--;
    return 0;
}

static int f_audio_out_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
	struct usb_composite_dev *cdev = audio->card.func.config->cdev;
    struct gaudio_snd_dev *snd = &audio->card.playback;
	struct f_uac1_opts *opts;
	int audio_buf_size;
	int err;
    struct f_audio_buf *data_buf;
    unsigned long flags;

	opts = container_of(audio->card.func.fi, struct f_uac1_opts,
			    func_inst);
	audio_buf_size = opts->audio_buf_size;

    spin_lock_irqsave(&snd->data_lock, flags);

    if (req->actual > 0) {
        data_buf = snd->data_buf;
        if (!data_buf) {
            spin_unlock_irqrestore(&snd->data_lock, flags);
            return -EINVAL;
        }

        if (req->actual > audio_buf_size) {
            spin_unlock_irqrestore(&snd->data_lock, flags);
            ERROR(&audio->card, "too much data to write to audio buffer!\n");
            return -EINVAL;
        }

        /* Copy buffer is full, add it to the play_queue */
        if (audio_buf_size - data_buf->actual < req->actual) {
            list_add_tail(&data_buf->list, &snd->data_queue);
            data_buf = f_audio_buffer_alloc(audio_buf_size);
            if (IS_ERR(data_buf)) {
                spin_unlock_irqrestore(&snd->data_lock, flags);
                return -ENOMEM;
            }
        }

        memcpy(data_buf->buf + data_buf->actual, req->buf, req->actual);
        data_buf->actual += req->actual;
        snd->data_buf = data_buf;

        if (get_data_buffer_count_nolock(snd) > opts->audio_buf_max_count) {
            INFO(&audio->card, "audio buffer is full, data will discard!\n");
            data_buf = list_first_entry(&snd->data_queue, struct f_audio_buf, list);
            list_del_init(&data_buf->list);
            f_audio_buffer_free(data_buf);
        }
    }

    //DBG(&audio->card, "playback data %d, data length %d\n",
    //        req->actual, get_data_bytes_nolock(snd));

    if (snd->data_running) {
        spin_unlock_irqrestore(&snd->data_lock, flags);
        err = usb_ep_queue(ep, req, GFP_ATOMIC);
        if (err)
            ERROR(cdev, "%s queue req: %d\n", ep->name, err);
    } else {
        spin_unlock_irqrestore(&snd->data_lock, flags);
        usb_ep_free_request(ep, req);
    }

    if (req->actual > 0) {
        wake_up_interruptible(&snd->data_wq);
    }

	return 0;
}

void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
	int status = req->status;
	struct usb_ep *out_ep = audio->card.out_ep;
	struct usb_ep *in_ep = audio->card.in_ep;
	u32 data = 0;

	switch (status) {
	case 0:				/* normal completion? */
		if (ep == out_ep)
			f_audio_out_ep_complete(ep, req);
        else if (ep == in_ep) {
			f_audio_in_ep_complete(ep, req);
        } else if (audio->set_con) {
            struct ak_usb_audio_control *akcon = container_of(audio->set_con,
                    struct ak_usb_audio_control, con);
            struct gaudio_snd_dev *snd = akcon->snd;

			memcpy(&data, req->buf, req->length);
			audio->set_con->set(audio->set_con, audio->set_cmd,
					le16_to_cpu(data));

            copy_to_control_buffer_nolock(snd, &audio->set_ctrlreq, sizeof(audio->set_ctrlreq));
            copy_to_control_buffer_nolock(snd, req->buf, req->length);

            DBG(snd->card, "set control %s, %s control length %d\n",
                    audio->set_con->name,
                    (snd == &audio->card.playback)? "playback": "capture",
                    get_control_bytes_nolock(snd));

            wake_up_interruptible(&snd->control_wq);
			audio->set_con = NULL;
		} else if (audio->ep_set_con) {
            struct ak_usb_audio_ep_control *con = audio->ep_set_con;
            struct gaudio_snd_dev *snd = con->snd;

			memcpy(&data, req->buf, req->length);
			audio->ep_set_con->set(audio->ep_set_con, audio->set_cmd,
					le16_to_cpu(data));

            copy_to_control_buffer_nolock(snd, &audio->set_ctrlreq, sizeof(audio->set_ctrlreq));
            copy_to_control_buffer_nolock(snd, req->buf, req->length);

            DBG(snd->card, "set control %s, %s control length %d\n",
                    audio->ep_set_con->name,
                    (snd == &audio->card.playback)? "playback": "capture",
                    get_control_bytes_nolock(snd));

            wake_up_interruptible(&snd->control_wq);
			audio->ep_set_con = NULL;
		}
		break;
	default:
		break;
	}
}

static int audio_set_intf_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	u8			id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u8			con_sel = (w_value >> 8) & 0xFF;
	u8			cmd = (ctrl->bRequest & 0x0F);
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	DBG(cdev, "%s: bRequest 0x%x, w_value 0x%04x, len %d, entity %d, con_sel %d, cmd %d\n",
			__func__, ctrl->bRequest, w_value, len, id, con_sel, cmd);

	list_for_each_entry(cs, &audio->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel) {
					audio->set_con = con;
					break;
				}
			}
			break;
		}
	}

    if (audio->set_con) {
        memcpy(&audio->set_ctrlreq, ctrl, sizeof(*ctrl));
    }

	audio->set_cmd = cmd;
    req->context = audio;
    req->complete = f_audio_complete;

	return len;
}

static int audio_get_intf_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u8			id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u8			con_sel = (w_value >> 8) & 0xFF;
	u8			cmd = (ctrl->bRequest & 0x0F);
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	DBG(cdev, "%s: bRequest 0x%x, w_value 0x%04x, len %d, entity %d, con_sel %d, cmd %d\n",
			__func__, ctrl->bRequest, w_value, len, id, con_sel, cmd);

	list_for_each_entry(cs, &audio->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel && con->get) {
					value = con->get(con, cmd);
                    DBG(cdev, "get %s[%d] return %d\n", con->name, cmd, value);
					break;
				}
			}
			break;
		}
	}

	req->context = audio;
	req->complete = f_audio_complete;
	len = min_t(size_t, sizeof(value), len);
	memcpy(req->buf, &value, len);

	return len;
}

static int audio_set_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			ep = le16_to_cpu(ctrl->wIndex);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
    u8 cs = (w_value >> 8) & 0xff;
	u8 cmd = (ctrl->bRequest & 0x0F);
    struct ak_usb_audio_ep_control *con;

	DBG(cdev, "%s: bRequest 0x%x, w_value 0x%04x, len %d, ep 0x%x\n",
			__func__, ctrl->bRequest, w_value, len, ep);

	switch (ctrl->bRequest) {
	case UAC_SET_CUR:
		value = len;
		break;
	case UAC_SET_MIN:
	case UAC_SET_MAX:
	case UAC_SET_RES:
	case UAC_SET_MEM:
		break;
	default:
		break;
	}

    if (value < 0)
        return -EOPNOTSUPP;

	list_for_each_entry(con, &audio->ep_cs, list) {
        if ((con->epaddr == ep) && (cs == con->cs)) {
            audio->ep_set_con = con;
            break;
        }
    }

    if (audio->ep_set_con) {
        memcpy(&audio->set_ctrlreq, ctrl, sizeof(*ctrl));
    } else {
        return -EOPNOTSUPP;
    }

	audio->set_cmd = cmd;
    req->context = audio;
    req->complete = f_audio_complete;
	return value;
}

static int audio_get_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int value = -EOPNOTSUPP;
	u8 ep = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16 len = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);
    u8 cs = (w_value >> 8) & 0xff;
	u8 cmd = (ctrl->bRequest & 0x0F);
    struct ak_usb_audio_ep_control *con;

	DBG(cdev, "%s: bRequest 0x%x, w_value 0x%04x, len %d, ep 0x%x\n",
			__func__, ctrl->bRequest, w_value, len, ep);

	switch (ctrl->bRequest) {
	case UAC_GET_CUR:
        value = len;
        break;
	case UAC_GET_MIN:
	case UAC_GET_MAX:
	case UAC_GET_RES:
	case UAC_GET_MEM:
		break;
	default:
		break;
	}

    if (value < 0)
        return -EOPNOTSUPP;

	list_for_each_entry(con, &audio->ep_cs, list) {
        if ((con->epaddr == ep) && (cs == con->cs)) {
            value = con->get(con, cmd);
            break;
        }
    }

	req->context = audio;
	req->complete = f_audio_complete;
	len = min_t(size_t, sizeof(value), len);
	memcpy(req->buf, &value, len);

    DBG(cdev, "value %d\n", value);

	return len;
}

static int
f_audio_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* composite driver infrastructure handles everything; interface
	 * activation uses set_alt().
	 */
	switch (ctrl->bRequestType) {
	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_set_intf_req(f, ctrl);
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_get_intf_req(f, ctrl);
		break;

	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_set_endpoint_req(f, ctrl);
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_get_endpoint_req(f, ctrl);
		break;

	default:
		ERROR(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "audio req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "audio response on err %d\n", value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static int f_audio_get_alt(struct usb_function *f, unsigned intf)
{
    return 0;
}

static int f_audio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_ep *out_ep = audio->card.out_ep;
	struct usb_ep *in_ep = audio->card.in_ep;
	struct usb_request *req;
	struct f_uac1_opts *opts;
	int req_buf_size, req_count, audio_buf_size;
	int i = 0, err = 0;

	DBG(cdev, "%s intf %d alt %d\n", __func__, intf, alt);

	opts = container_of(f->fi, struct f_uac1_opts, func_inst);
	req_buf_size = opts->max_packet_size;
	req_count = opts->playback_req_count;
	audio_buf_size = opts->audio_buf_size;

	if (intf == as_spk_interface_alt_0_desc.bInterfaceNumber) {
        struct gaudio_snd_dev *snd = &audio->card.playback;

		if (alt == as_spk_interface_alt_1_desc.bAlternateSetting
                && !snd->data_running) {
			err = config_ep_by_speed(cdev->gadget, f, out_ep);
			if (err)
				return err;
			usb_ep_enable(out_ep);
            if (!snd->data_buf) {
                snd->data_buf = f_audio_buffer_alloc(audio_buf_size);
                if (IS_ERR(snd->data_buf))
                    return -ENOMEM;
            }
			/*
			 * allocate a bunch of read buffers
			 * and queue them all at once.
			 */
			for (i = 0; i < req_count && err == 0; i++) {
				req = usb_ep_alloc_request(out_ep, GFP_ATOMIC);
				if (req) {
					req->buf = kzalloc(req_buf_size,
							GFP_ATOMIC);
					if (req->buf) {
						req->length = req_buf_size;
						req->context = audio;
						req->complete = f_audio_complete;
						err = usb_ep_queue(out_ep, req, GFP_ATOMIC);
						if (err)
							ERROR(cdev, "%s queue req: %d\n", out_ep->name, err);
					} else {
                        usb_ep_free_request(out_ep, req);
						err = -ENOMEM;
                    }
				} else
					err = -ENOMEM;
			}
            snd->data_running = 1;
		}
	} else if (intf == as_mic_interface_alt_0_desc.bInterfaceNumber) {
        struct gaudio_snd_dev *snd = &audio->card.capture;

		if (alt == as_mic_interface_alt_1_desc.bAlternateSetting) {
            if (!snd->data_running) {
                err = config_ep_by_speed(cdev->gadget, f, in_ep);
                if (err)
                    return err;
                usb_ep_enable(in_ep);
#if 0
                if (!snd->data_buf) {
                    snd->data_buf = f_audio_buffer_alloc(audio_buf_size);
                    if (IS_ERR(snd->data_buf))
                        return -ENOMEM;
                }
#endif
                snd->data_running = 1;
            }
            snd->data_writable = 1;
            wake_up_interruptible(&snd->data_wq);
        } else {
            snd->data_writable = 0;
        }
    }

    if (!err) {
        struct gaudio_snd_dev *snd = NULL;
        struct usb_ctrlrequest ctrlreq = {0};

        if (intf == as_spk_interface_alt_0_desc.bInterfaceNumber) {
            snd = &audio->card.playback;
        } else if (intf == as_mic_interface_alt_0_desc.bInterfaceNumber) {
            snd = &audio->card.capture;
        }
        if (snd) {
            ctrlreq.bRequestType = USB_RECIP_INTERFACE;
            ctrlreq.bRequest = USB_REQ_SET_INTERFACE;
            ctrlreq.wIndex = cpu_to_le16(intf);
            ctrlreq.wValue = cpu_to_le16(alt);
            ctrlreq.wLength = cpu_to_le16(0);
            copy_to_control_buffer_nolock(snd, &ctrlreq, sizeof(ctrlreq));
            DBG(snd->card, "USB_REQ_SET_INTERFACE %d %d,"
                    " %s control length %d\n",
                    intf, alt,
                    (snd == &audio->card.playback)? "playback": "capture",
                    get_control_bytes_nolock(snd));
            wake_up_interruptible(&snd->control_wq);
        }
    }

	return err;
}

static void f_audio_disable(struct usb_function *f)
{
    struct f_audio *audio = func_to_audio(f);
    struct gaudio_snd_dev *snd = NULL;
    struct usb_ctrlrequest ctrlreq = {0};
    int i;

    DBG(&audio->card, "%s\n", __func__);

    for (i = 0; i < 2; i++) {
        if (i == 0) {
            snd = &audio->card.playback;
        } else {
            snd = &audio->card.capture;
        }
        snd->data_running = 0;
        ctrlreq.bRequestType = USB_RECIP_DEVICE;
        ctrlreq.bRequest = AK_USB_REQ_DISCONNECT;
        ctrlreq.wIndex = 0;
        ctrlreq.wValue = 0;
        ctrlreq.wLength = 0;
        copy_to_control_buffer_nolock(snd, &ctrlreq, sizeof(ctrlreq));
        DBG(snd->card, "AK_USB_REQ_DISCONNECT %s control length %d\n",
                (snd == &audio->card.playback)? "playback": "capture",
                get_control_bytes_nolock(snd));
        wake_up_interruptible(&snd->control_wq);
    }

    return;
}

/*-------------------------------------------------------------------------*/

static void f_audio_build_desc(struct f_audio *audio, struct f_uac1_opts *opts)
{
	u8 *sam_freq;
	int rate;

	/* Set sample rates */
    rate = 8000;
	sam_freq = as_mic_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
    rate = 16000;
	sam_freq = as_mic_type_i_desc.tSamFreq[1];
	memcpy(sam_freq, &rate, 3);

    rate = 8000;
	sam_freq = as_spk_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
    rate = 16000;
	sam_freq = as_spk_type_i_desc.tSamFreq[1];
	memcpy(sam_freq, &rate, 3);

    as_mic_in_ep_desc.wMaxPacketSize = cpu_to_le16(opts->max_packet_size);
    as_spk_out_ep_desc.wMaxPacketSize = cpu_to_le16(opts->max_packet_size);

}

/* audio function driver setup/binding */
static int
f_audio_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_audio		*audio = func_to_audio(f);
	struct usb_string	*us;
	int			status;
	struct usb_ep		*ep = NULL;
	struct f_uac1_opts	*audio_opts;

	audio_opts = container_of(f->fi, struct f_uac1_opts, func_inst);
	audio->card.gadget = c->cdev->gadget;
	/* set up ASLA audio devices */
	if (!audio_opts->bound) {
		status = gaudio_setup(&audio->card);
		if (status < 0)
			return status;
		audio_opts->bound = true;
	}
	us = usb_gstrings_attach(cdev, uac1_strings, ARRAY_SIZE(strings_uac1));
	if (IS_ERR(us))
		return PTR_ERR(us);
	uac_iad.iFunction = us[STR_AC_IF].id;
	ac_interface_desc.iInterface = us[STR_AC_IF].id;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	uac_iad.bFirstInterface = status;
	ac_interface_desc.bInterfaceNumber = status;
    DBG(&audio->card, "AC interface number %d\n", status);

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	as_mic_interface_alt_0_desc.bInterfaceNumber = status;
	as_mic_interface_alt_1_desc.bInterfaceNumber = status;
    DBG(&audio->card, "MIC AS interface number %d\n", status);

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	as_spk_interface_alt_0_desc.bInterfaceNumber = status;
	as_spk_interface_alt_1_desc.bInterfaceNumber = status;
    DBG(&audio->card, "Speaker AS interface number %d\n", status);

    ac_header_desc.baInterfaceNr[0] = as_mic_interface_alt_0_desc.bInterfaceNumber;
    ac_header_desc.baInterfaceNr[1] = as_spk_interface_alt_0_desc.bInterfaceNumber;

	f_audio_build_desc(audio, audio_opts);

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &as_mic_in_ep_desc);
	if (!ep)
		goto fail;
	audio->card.in_ep = ep;
	audio->card.in_ep->desc = &as_mic_in_ep_desc;
    DBG(&audio->card, "MIC IN %s\n", ep->name);

	ep = usb_ep_autoconfig(cdev->gadget, &as_spk_out_ep_desc);
	if (!ep)
		goto fail;
	audio->card.out_ep = ep;
	audio->card.out_ep->desc = &as_spk_out_ep_desc;
    DBG(&audio->card, "Speaker OUT %s\n", ep->name);

	status = -ENOMEM;

	/* copy descriptors, and track endpoint copies */
	status = usb_assign_descriptors(f, f_audio_desc, f_audio_desc, NULL);
	if (status)
		goto fail;
    mic_ep_sample_rate_control.epaddr = audio->card.in_ep->address;
    mic_ep_sample_rate_control.snd = &audio->card.capture;

    spk_ep_sample_rate_control.epaddr = audio->card.out_ep->address;
    spk_ep_sample_rate_control.snd = &audio->card.playback;
	return 0;

fail:
	gaudio_cleanup(&audio->card);
	return status;
}

/*-------------------------------------------------------------------------*/

static int generic_control_set(struct usb_audio_control *con, u8 cmd, int value)
{
    con->data[cmd] = value;
	return 0;
}

static int generic_control_get(struct usb_audio_control *con, u8 cmd)
{
	return con->data[cmd];
}

static int generic_ep_control_set(struct ak_usb_audio_ep_control *con, u8 cmd, int value)
{
    con->data[cmd] = value;
	return 0;
}

static int generic_ep_control_get(struct ak_usb_audio_ep_control *con, u8 cmd)
{
	return con->data[cmd];
}
/* Todo: add more control selecotor dynamically */
static int control_selector_init(struct f_audio *audio)
{
	INIT_LIST_HEAD(&audio->cs);
	list_add(&spk_feature_unit.list, &audio->cs);

	INIT_LIST_HEAD(&spk_feature_unit.control);
	list_add(&spk_mute_control.con.list, &spk_feature_unit.control);
	list_add(&spk_volume_control.con.list, &spk_feature_unit.control);

	list_add(&mic_feature_unit.list, &audio->cs);

	INIT_LIST_HEAD(&mic_feature_unit.control);
	list_add(&mic_mute_control.con.list, &mic_feature_unit.control);
	list_add(&mic_volume_control.con.list, &mic_feature_unit.control);

	spk_volume_control.snd = &audio->card.playback;
	spk_mute_control.snd = &audio->card.playback;

	spk_volume_control.con.data[UAC__CUR] = 1;
	spk_volume_control.con.data[UAC__MIN] = 1;
	spk_volume_control.con.data[UAC__MAX] = 255;
	spk_volume_control.con.data[UAC__RES] = 1;

	spk_mute_control.con.data[UAC__CUR] = 0;

	mic_volume_control.snd = &audio->card.capture;
	mic_mute_control.snd = &audio->card.capture;

	mic_volume_control.con.data[UAC__CUR] = 1;
	mic_volume_control.con.data[UAC__MIN] = 1;
	mic_volume_control.con.data[UAC__MAX] = 255;
	mic_volume_control.con.data[UAC__RES] = 1;

	mic_mute_control.con.data[UAC__CUR] = 0;
	INIT_LIST_HEAD(&audio->ep_cs);
	list_add(&mic_ep_sample_rate_control.list, &audio->ep_cs);
	list_add(&spk_ep_sample_rate_control.list, &audio->ep_cs);

	return 0;
}

static inline struct f_uac1_opts *to_f_uac1_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_uac1_opts,
			    func_inst.group);
}

static void f_uac1_attr_release(struct config_item *item)
{
	struct f_uac1_opts *opts = to_f_uac1_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations f_uac1_item_ops = {
	.release	= f_uac1_attr_release,
};

#define UAC1_INT_ATTRIBUTE(name)					\
static ssize_t f_uac1_opts_##name##_show(struct config_item *item,	\
					 char *page)			\
{									\
	struct f_uac1_opts *opts = to_f_uac1_opts(item);		\
	int result;							\
									\
	mutex_lock(&opts->lock);					\
	result = sprintf(page, "%u\n", opts->name);			\
	mutex_unlock(&opts->lock);					\
									\
	return result;							\
}									\
									\
static ssize_t f_uac1_opts_##name##_store(struct config_item *item,		\
					  const char *page, size_t len)	\
{									\
	struct f_uac1_opts *opts = to_f_uac1_opts(item);		\
	int ret;							\
	u32 num;							\
									\
	mutex_lock(&opts->lock);					\
	if (opts->refcnt) {						\
		ret = -EBUSY;						\
		goto end;						\
	}								\
									\
	ret = kstrtou32(page, 0, &num);					\
	if (ret)							\
		goto end;						\
									\
	opts->name = num;						\
	ret = len;							\
									\
end:									\
	mutex_unlock(&opts->lock);					\
	return ret;							\
}									\
									\
CONFIGFS_ATTR(f_uac1_opts_, name)

UAC1_INT_ATTRIBUTE(max_packet_size);
UAC1_INT_ATTRIBUTE(playback_req_count);
UAC1_INT_ATTRIBUTE(audio_buf_size);
UAC1_INT_ATTRIBUTE(audio_buf_max_count);
UAC1_INT_ATTRIBUTE(capture_req_max_count);
UAC1_INT_ATTRIBUTE(playback_min_read_bytes);

#define UAC1_STR_ATTRIBUTE(name)					\
static ssize_t f_uac1_opts_##name##_show(struct config_item *item,	\
					 char *page)			\
{									\
	struct f_uac1_opts *opts = to_f_uac1_opts(item);		\
	int result;							\
									\
	mutex_lock(&opts->lock);					\
	result = sprintf(page, "%s\n", opts->name);			\
	mutex_unlock(&opts->lock);					\
									\
	return result;							\
}									\
									\
static ssize_t f_uac1_opts_##name##_store(struct config_item *item,	\
					  const char *page, size_t len)	\
{									\
	struct f_uac1_opts *opts = to_f_uac1_opts(item);		\
	int ret = -EBUSY;						\
	char *tmp;							\
									\
	mutex_lock(&opts->lock);					\
	if (opts->refcnt)						\
		goto end;						\
									\
	tmp = kstrndup(page, len, GFP_KERNEL);				\
	if (tmp) {							\
		ret = -ENOMEM;						\
		goto end;						\
	}								\
	if (opts->name##_alloc)						\
		kfree(opts->name);					\
	opts->name##_alloc = true;					\
	opts->name = tmp;						\
	ret = len;							\
									\
end:									\
	mutex_unlock(&opts->lock);					\
	return ret;							\
}									\
									\
CONFIGFS_ATTR(f_uac1_opts_, name)

static struct configfs_attribute *f_uac1_attrs[] = {
	&f_uac1_opts_attr_max_packet_size,
	&f_uac1_opts_attr_playback_req_count,
	&f_uac1_opts_attr_audio_buf_size,
	&f_uac1_opts_attr_audio_buf_max_count,
	&f_uac1_opts_attr_capture_req_max_count,
	&f_uac1_opts_attr_playback_min_read_bytes,
	NULL,
};

static struct config_item_type f_uac1_func_type = {
	.ct_item_ops	= &f_uac1_item_ops,
	.ct_attrs	= f_uac1_attrs,
	.ct_owner	= THIS_MODULE,
};

static void f_audio_free_inst(struct usb_function_instance *f)
{
	struct f_uac1_opts *opts;

	opts = container_of(f, struct f_uac1_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *f_audio_alloc_inst(void)
{
	struct f_uac1_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = f_audio_free_inst;

	config_group_init_type_name(&opts->func_inst.group, "",
				    &f_uac1_func_type);

	return &opts->func_inst;
}

static void f_audio_free(struct usb_function *f)
{
	struct f_audio *audio = func_to_audio(f);
	struct f_uac1_opts *opts;

	gaudio_cleanup(&audio->card);
	opts = container_of(f->fi, struct f_uac1_opts, func_inst);
	kfree(audio);
	mutex_lock(&opts->lock);
	--opts->refcnt;
	mutex_unlock(&opts->lock);
}

static void f_audio_unbind(struct usb_configuration *c, struct usb_function *f)
{
	usb_free_all_descriptors(f);
}

static struct usb_function *f_audio_alloc(struct usb_function_instance *fi)
{
	struct f_audio *audio;
	struct f_uac1_opts *opts;

	/* allocate and initialize one new instance */
	audio = kzalloc(sizeof(*audio), GFP_KERNEL);
	if (!audio)
		return ERR_PTR(-ENOMEM);

	audio->card.func.name = "g_audio";

	opts = container_of(fi, struct f_uac1_opts, func_inst);
	mutex_lock(&opts->lock);
	++opts->refcnt;
	mutex_unlock(&opts->lock);

	audio->card.func.bind = f_audio_bind;
	audio->card.func.unbind = f_audio_unbind;
	audio->card.func.set_alt = f_audio_set_alt;
    audio->card.func.get_alt = f_audio_get_alt;
	audio->card.func.setup = f_audio_setup;
	audio->card.func.disable = f_audio_disable;
	audio->card.func.free_func = f_audio_free;

	control_selector_init(audio);

	return &audio->card.func;
}

DECLARE_USB_FUNCTION_INIT(ak_uac1, f_audio_alloc_inst, f_audio_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bryan Wu");
