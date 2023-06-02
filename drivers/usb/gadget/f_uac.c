#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <linux/usb/audio.h>
#include "f_uac.h"

/*#define UAC_BIND_DEACTIVATE*/
static void f_audio_complete(struct usb_ep *ep, struct usb_request *req);

#define UAC_ERR(msg...)		printk(KERN_EMERG "UAC, " msg)
#define UAC_DBG(msg...)		printk(KERN_INFO "UAC, " msg)
#define UAC_INFO(msg...)	printk(KERN_INFO  "UAC, " msg)



#define UAC_STR_ASSOCIATION_IDX		0
#define UAC_STR_CONTROL_IDX			1
#define UAC_STR_STREAMING_IDX		2


static struct usb_string uac_en_us_strings[] = {
	[UAC_STR_ASSOCIATION_IDX].s  = "Fullhan Audio",
	[UAC_STR_CONTROL_IDX].s      = "Audio Control",
	[UAC_STR_STREAMING_IDX].s    = "Audio Streaming",
	{  }
};

static struct usb_gadget_strings uac_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = uac_en_us_strings,
};

static struct usb_gadget_strings *uac_function_strings[] = {
	&uac_stringtab,
	NULL,
};




#ifdef ENABLE_SPEAKER_DESC

#define UAC_IF_COUNT                3
#define UAC_COLLECTION_NUM          2
#define AC_HEADER_TOTAL_LENGTH     0x44

#else

#define UAC_IF_COUNT                2
#define UAC_COLLECTION_NUM          1
#define AC_HEADER_TOTAL_LENGTH      0x26


#endif

/*
*
*/
static struct usb_interface_assoc_descriptor audio_iad_desc = {
	.bLength		    =   0x08,
	.bDescriptorType	=   USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	=   2,
	.bInterfaceCount	=   UAC_IF_COUNT,
	.bFunctionClass		=   USB_CLASS_AUDIO,
	.bFunctionSubClass	=   USB_SUBCLASS_AUDIOSTREAMING,
	.bFunctionProtocol	=   0x00,
	.iFunction		    =   0,
};


static struct usb_interface_descriptor ac_intf_desc  = {
	.bLength		    =   USB_DT_INTERFACE_SIZE,
	.bDescriptorType	=   USB_DT_INTERFACE,
	.bInterfaceNumber	=   2,
	.bAlternateSetting	=   0,
	.bNumEndpoints		=   0,
	.bInterfaceClass	=   USB_CLASS_AUDIO,
	.bInterfaceSubClass	=   USB_SUBCLASS_AUDIOCONTROL,
	.bInterfaceProtocol	=   0x00,
	.iInterface		    =   0,
};


#define AC_SIZE_NUM     2

DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);

/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_2 ac_header_desc = {
	.bLength            =   UAC_DT_AC_HEADER_SIZE(UAC_COLLECTION_NUM),
	.bDescriptorType    =   USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =   UAC_HEADER,
	.bcdADC             =   __constant_cpu_to_le16(0x0100),
	.wTotalLength       =   __constant_cpu_to_le16(AC_HEADER_TOTAL_LENGTH),
	.bInCollection      =   UAC_COLLECTION_NUM,
	.baInterfaceNr[0]   =   3,
	.baInterfaceNr[1]   =   4,
};



#define INPUT_TERMINAL_ID	1
#define AUDIO_CHN_NUM       1
static struct uac_input_terminal_descriptor audio_mic_it_desc = {
	.bLength            =	UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID        =	INPUT_TERMINAL_ID,
	.wTerminalType      =	UAC_INPUT_TERMINAL_MICROPHONE,
	.bAssocTerminal     =	0,
	.bNrChannels        =   1,
	.wChannelConfig     =	0x0,
};


#define OUTPUT_TERMINAL_ID  3
#define FEATURE_UNIT_ID		5

static struct uac1_output_terminal_descriptor  audio_mic_ot_desc = {
	.bLength            =   UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType    =   USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_OUTPUT_TERMINAL,
	.bTerminalID		=   OUTPUT_TERMINAL_ID,
	.wTerminalType      =   UAC_TERMINAL_STREAMING,
	.bAssocTerminal     =   0,
	.bSourceID          =   FEATURE_UNIT_ID,
	.iTerminal          =   0,
};


DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);


static struct uac_feature_unit_descriptor_0 audio_mic_fu_desc = {
	.bLength		    =   0x08,
	.bDescriptorType	=   USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	=   UAC_FEATURE_UNIT,
	.bUnitID		    =   FEATURE_UNIT_ID,
	.bSourceID		    =   INPUT_TERMINAL_ID,
	.bControlSize		=   1,
	.bmaControls[0]		=   (UAC_FU_MUTE | UAC_FU_VOLUME),
};


#define SPEAKER_IT_ID       4
#define SPEAKER_FU_ID       6
#define SPEAKER_OT_ID       8

#ifdef ENABLE_SPEAKER_DESC
static struct uac_input_terminal_descriptor audio_spk_it_desc = {
	.bLength            =	UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID        =	SPEAKER_IT_ID,
	.wTerminalType      =	UAC_TERMINAL_STREAMING,
	.bAssocTerminal     =	0,
	.bNrChannels        =   1,
	.wChannelConfig     =	0x0,
};



static struct uac_feature_unit_descriptor_0 audio_spk_fu_desc = {
	.bLength		    =   0x08,
	.bDescriptorType	=   USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	=   UAC_FEATURE_UNIT,
	.bUnitID		    =   SPEAKER_FU_ID,
	.bSourceID		    =   SPEAKER_IT_ID,
	.bControlSize		=   1,
	.bmaControls[0]		=   (UAC_FU_MUTE | UAC_FU_VOLUME),
};

static struct uac1_output_terminal_descriptor  audio_spk_ot_desc = {
	.bLength            =   UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType    =   USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_OUTPUT_TERMINAL,
	.bTerminalID		=   SPEAKER_OT_ID,
	.wTerminalType      =   UAC_OUTPUT_TERMINAL_SPEAKER,
	.bAssocTerminal     =   0,
	.bSourceID          =   SPEAKER_FU_ID,
	.iTerminal          =   0,
};

#endif


/* Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_mic_if_alt_0_desc = {
	.bLength            =	USB_DT_INTERFACE_SIZE,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bInterfaceNumber   =   3,
	.bAlternateSetting  =	0,
	.bNumEndpoints      =	0,
	.bInterfaceClass    =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_mic_if_alt_1_desc = {
	.bLength            =	USB_DT_INTERFACE_SIZE,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bInterfaceNumber   =   3,
	.bAlternateSetting  =	1,
	.bNumEndpoints      =	1,
	.bInterfaceClass    =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};


/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_mic_header_desc = {
	.bLength            =	UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink      =	OUTPUT_TERMINAL_ID,
	.bDelay             =	0xff,
	.wFormatTag         =	UAC_FORMAT_TYPE_I_PCM,
};


#define SAMPLE_FREQ     (8000)
DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(1);

static struct uac_format_type_i_discrete_descriptor_1 as_mic_type_i_desc = {
	.bLength            =	UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType        =	UAC_FORMAT_TYPE_I,
	.bNrChannels        =   AUDIO_CHN_NUM,
	.bSubframeSize      =	2,
	.bBitResolution     =	16,
	.bSamFreqType       =	1,
	.tSamFreq[0][0]     =   (SAMPLE_FREQ & 0xff),
	.tSamFreq[0][1]     =   (SAMPLE_FREQ >> 8) & 0xff,
	.tSamFreq[0][2]     =   0,
};


static struct usb_endpoint_descriptor audio_mic_streaming_ep = {
	.bLength		    =   USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType	=   USB_DT_ENDPOINT,
	.bEndpointAddress	=   USB_DIR_IN|5,
	.bmAttributes		=   0x05,/*USB_ENDPOINT_XFER_ISOC,*/
	.wMaxPacketSize		=   cpu_to_le16(68),
	.bInterval		    =   4,
};


/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_mic_iso_out_desc = {
	.bLength            =	UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType    =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes       =	1,
	.bLockDelayUnits    =	0,
	.wLockDelay         =	0,
};


#ifdef ENABLE_SPEAKER_DESC
/*speaker*/
/* Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_spk_if_alt_0_desc = {
	.bLength            =	USB_DT_INTERFACE_SIZE,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bInterfaceNumber   =   4,
	.bAlternateSetting  =	0,
	.bNumEndpoints      =	0,
	.bInterfaceClass    =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_spk_if_alt_1_desc = {
	.bLength            =	USB_DT_INTERFACE_SIZE,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bInterfaceNumber   =   4,
	.bAlternateSetting  =	1,
	.bNumEndpoints      =	1,
	.bInterfaceClass    =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};


/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_spk_header_desc = {
	.bLength            =	UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink      =	SPEAKER_IT_ID,
	.bDelay             =	0xff,
	.wFormatTag         =	UAC_FORMAT_TYPE_I_PCM,
};


static struct uac_format_type_i_discrete_descriptor_1 as_spk_type_i_desc = {
	.bLength            =	UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType        =	UAC_FORMAT_TYPE_I,
	.bNrChannels        =   AUDIO_CHN_NUM,
	.bSubframeSize      =	2,
	.bBitResolution     =	16,
	.bSamFreqType       =	1,
	.tSamFreq[0][0]     =   (SAMPLE_FREQ & 0xff),
	.tSamFreq[0][1]     =   (SAMPLE_FREQ >> 8) & 0xff,
	.tSamFreq[0][2]     =   0,

};


static struct usb_endpoint_descriptor audio_spk_streaming_ep = {
	.bLength		    =   USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType	=   USB_DT_ENDPOINT,
	.bEndpointAddress	=   USB_DIR_OUT|6,
	.bmAttributes		=   0x05,/*USB_ENDPOINT_XFER_ISOC,*/
	.wMaxPacketSize		=   cpu_to_le16(68),
	.bInterval		    =   4,
};


/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_spk_iso_out_desc = {
	.bLength            =	UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType    =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes       =	0,
	.bLockDelayUnits    =	0,
	.wLockDelay         =	0,
};

#endif



static struct usb_descriptor_header *f_audio_descs[] __initdata = {
	(struct usb_descriptor_header *)&audio_iad_desc,
	(struct usb_descriptor_header *)&ac_intf_desc,
	(struct usb_descriptor_header *)&ac_header_desc,

	(struct usb_descriptor_header *)&audio_mic_it_desc,
	(struct usb_descriptor_header *)&audio_mic_ot_desc,
	(struct usb_descriptor_header *)&audio_mic_fu_desc,

#ifdef ENABLE_SPEAKER_DESC
	(struct usb_descriptor_header *)&audio_spk_it_desc,
	(struct usb_descriptor_header *)&audio_spk_ot_desc,
	(struct usb_descriptor_header *)&audio_spk_fu_desc,
#endif


	(struct usb_descriptor_header *)&as_mic_if_alt_0_desc,
	(struct usb_descriptor_header *)&as_mic_if_alt_1_desc,
	(struct usb_descriptor_header *)&as_mic_header_desc,
	(struct usb_descriptor_header *)&as_mic_type_i_desc,
	(struct usb_descriptor_header *)&audio_mic_streaming_ep,
	(struct usb_descriptor_header *)&as_mic_iso_out_desc,

#ifdef ENABLE_SPEAKER_DESC
	(struct usb_descriptor_header *)&as_spk_if_alt_0_desc,
	(struct usb_descriptor_header *)&as_spk_if_alt_1_desc,
	(struct usb_descriptor_header *)&as_spk_header_desc,
	(struct usb_descriptor_header *)&as_spk_type_i_desc,
	(struct usb_descriptor_header *)&audio_spk_streaming_ep,
	(struct usb_descriptor_header *)&as_spk_iso_out_desc,
#endif
	NULL,
};




#define UAC_STREAM_OFF          0
#define UAC_STREAM_ON           1
#define AUDIO_REQ_NUM		    32

struct uac_audio {
	struct usb_function      *func;
	struct usb_ep            *ep;
	struct usb_request       *req[AUDIO_REQ_NUM];
	void                     *req_buf[AUDIO_REQ_NUM];
	int                      req_valid[AUDIO_REQ_NUM];
	int                      req_buf_size;

	int                      ac_connected;
	int                      stream_sta;
	int                      ac_intf;
	int                      as_mic_intf;
	int                      as_spk_intf;

	int                      mute_set;
	int                      vol_set;
	int                      set_cmd;

};


static struct uac_audio *g_audio;


static int uac_is_stream_on(void)
{
	return (g_audio->stream_sta == UAC_STREAM_ON);
}

static int uac_audio_enable(int enable)
{
	unsigned int i;

	uac_stream(enable);
	if (!enable) {
		if (uac_is_stream_on()) {
			for (i = 0; i < AUDIO_REQ_NUM; ++i)
				usb_ep_dequeue(g_audio->ep, g_audio->req[i]);

			usb_ep_disable(g_audio->ep);
			g_audio->stream_sta = UAC_STREAM_OFF;
			UAC_INFO("STREAM OFF\n");
		}
		return 0;
	}


	usb_ep_enable(g_audio->ep, &audio_mic_streaming_ep);
	g_audio->stream_sta = UAC_STREAM_ON;
	UAC_INFO("STREAM ON\n");
	return 0;
}





void uac_begin_transfer_data(void)
{
	int i, err;
	struct usb_request *req;
	for (i = 0; i < AUDIO_REQ_NUM; i++) {
		req = g_audio->req[i];
		req->buf = g_audio->req_buf[i];
		req->dma = (~(dma_addr_t)0);

		g_audio->req_valid[i] = 0;
		memset(req->buf, 0, g_audio->req_buf_size);
		req->length = 0;
		req->context = (void *)i;
		req->dma = DMA_ADDR_INVALID;
		req->complete =	f_audio_complete;
#if 1
		err = usb_ep_queue(g_audio->ep,	req,  GFP_ATOMIC);
		if (err)
			UAC_ERR("transfer_data usb_ep_queue error, %s  req: %d\n",	g_audio->ep->name, err);
#endif
	}


	wake_up_app();
	err = 0;
	UAC_INFO("sendMsg audio!\n");
}


static void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{

	int a;

	int i, avlidCount = 0;
	unsigned char *pData = NULL;
	switch (req->status) {
	case 0:
		UAC_INFO("f_audio_complete.\n");
		if (ep != g_audio->ep) {
			if (g_audio->set_cmd) {
				pData = req->buf;
				if (UAC_FU_MUTE == g_audio->set_cmd) {
					if (1 == req->actual) {
						g_audio->mute_set = pData[0];
						UAC_INFO("mute set data = %02x\n", pData[0]);
					}
				} else if (UAC_FU_VOLUME == g_audio->set_cmd) {
					if (2 == req->actual) {
						g_audio->vol_set = pData[0] + pData[1]*256;
						UAC_INFO("vol set data = %02x, %02x\n", pData[0], pData[1]);
					}
				}
			}
			g_audio->set_cmd = 0;
			return;
		}
		a = (int)req->context;
		g_audio->req_valid[a] = 1;
		for (i = 0; i < AUDIO_REQ_NUM; i++) {
			if (g_audio->req_valid[a])
				avlidCount++;
			if (avlidCount > 3) {
				UAC_INFO("f_audio_complete wake_up_app.\n");
				wake_up_app();
				break;
			}
		}
		break;

	case -ESHUTDOWN:
		UAC_INFO("f_audio_complete ESHUTDOWN.\n");
		goto requeue;

	default:
		UAC_INFO("request completed with status %d.\n",	req->status);
		goto requeue;
	}


	return;

requeue:
	a = 5;

}


struct audio_data {
	unsigned char *pData;
	unsigned char *pPos;
	unsigned char *pEnd;
};

int audio_send_data(void *args)
{
	struct audio_data *pAudio = args;
	int i;
	struct usb_request *req = NULL;

	int len = g_audio->req_buf_size;
	int send_len = 0;
	int ret = 0;
	int err = 0;
	if (!uac_is_stream_on())
		return 0;

	for (i = 0; i < AUDIO_REQ_NUM; i++) {
		if (g_audio->req_valid[i]) {
			g_audio->req_valid[i] = 0;
			req = g_audio->req[i];
			req->buf = g_audio->req_buf[i];
			req->dma = DMA_ADDR_INVALID;
			req->context = (void *)i;
			req->complete =	f_audio_complete;

			if (pAudio->pPos >= pAudio->pEnd) {
				pAudio->pPos = pAudio->pData;
				ret = 1;
				break;
			} else if (pAudio->pPos <  pAudio->pData)
				pAudio->pPos = pAudio->pData;

			if (pAudio->pPos + g_audio->req_buf_size >= pAudio->pEnd) {
				len = pAudio->pEnd-pAudio->pPos;
				memcpy(req->buf, pAudio->pPos, len);
				send_len += len;

				pAudio->pPos = pAudio->pData;
				ret = 1;
			} else {
				len = g_audio->req_buf_size;
				memcpy(req->buf, pAudio->pPos, g_audio->req_buf_size);
				pAudio->pPos += g_audio->req_buf_size;
				send_len += g_audio->req_buf_size;
			}
			req->length = len;

			/*if (g_audio->mute_set)
				memset(req->buf, 0, len);
			*/
			err = usb_ep_queue(g_audio->ep,	req,  GFP_ATOMIC);
			if (err)
				UAC_ERR("audio usb_ep_queue error, %s  req: %d\n",	g_audio->ep->name, err);

			if (ret)
				break;
		}
	}

	if (send_len > 0)
		UAC_INFO("audio send = %d\n", send_len);
	return send_len;

}



static int
uac_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev	*cdev = f->config->cdev;
	struct usb_request		*req = cdev->req;
	int value = -EOPNOTSUPP;
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);
	int cs, id;
	int sendAudioReq = 0;
	u8 *pData;
	u8 is_cmd = 1;

	UAC_INFO("setup, req: %02x.%02x value: %04x index: %04x length: %d\n",
	ctrl->bRequestType, ctrl->bRequest,
	w_value, w_index, w_length);

	g_audio->set_cmd = 0;

	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) {
		value = 3;
		sendAudioReq = 1;
	} else {
		cs = ctrl->wValue >> 8;
		id = ctrl->wIndex >> 8;
		UAC_INFO("id = %d, cs = %d\n", id, cs);
		pData = req->buf;
		if (FEATURE_UNIT_ID == id || SPEAKER_FU_ID == id) {
			switch (cs) {
			case UAC_FU_MUTE:
				value = 1;
				pData[0] = 0;
				break;

			case UAC_FU_VOLUME:
				value = 2;
				pData[0] = 0;
				pData[1] = 1;
				break;

			default:
				is_cmd = 0;
				break;
			}
		}

		if (!(ctrl->bRequestType & USB_DIR_IN)) {
			if (is_cmd)
				g_audio->set_cmd = cs;
		}
	}
	if (value >= 0 && value != USB_GADGET_DELAYED_STATUS) {
		req->length = value;
		req->zero = value < w_length;
		req->complete = f_audio_complete;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			DBG(cdev, "ep_queue --> %d\n", value);
			req->status = 0;
			composite_setup_complete(cdev->gadget->ep0, req);
		}
	}
	if (sendAudioReq)
		uac_begin_transfer_data();
	return 0;
}


static int
uac_function_get_alt(struct usb_function *f, unsigned interface)
{
	UAC_INFO("get_alt\n");
	return 0;
}




static int
uac_function_set_alt(struct usb_function *f, unsigned interface, unsigned alt)
{
	UAC_INFO("set_alt, intf = %d, alt = %d\n", interface, alt);
	if (g_audio->as_mic_intf == interface)
		uac_audio_enable(0 != alt);
	else if (g_audio->ac_intf == interface)
		g_audio->ac_connected = 1;
	return 0;
}


void audio_dev_open(void)
{
	UAC_DBG("audio_dev_open!\n");
	if (!g_audio->ac_connected) {
 #ifdef UAC_BIND_DEACTIVATE
		int ret;
		ret = usb_function_activate(g_audio->func);
		if (ret < 0)
			UAC_DBG("audio_dev_open conn error!\n");
		 else
			UAC_DBG("audio_dev_open conn ok!\n");
 #endif
	}
}

static void
uac_function_disable(struct usb_function *f)
{

	INFO(f->config->cdev, "uac_function_disable\n");
}

#define UVC_COPY_DESCRIPTOR(mem, dst, desc) \
	do { \
		memcpy(mem, desc, (desc)->bLength); \
		*(dst)++ = mem; \
		mem += (desc)->bLength; \
	} while (0);

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
	do { \
		const struct usb_descriptor_header * const *__src; \
		for (__src = src; *__src; ++__src) { \
			memcpy(mem, *__src, (*__src)->bLength); \
			*dst++ = mem; \
			mem += (*__src)->bLength; \
		} \
	} while (0)



static struct usb_descriptor_header ** __init
uac_copy_descriptors(enum usb_device_speed speed)
{
	const struct usb_descriptor_header * const *src;
	struct usb_descriptor_header **dst;
	struct usb_descriptor_header **hdr;
	unsigned int n_desc = 0;
	unsigned int bytes  = 0;
	void *mem;

	int audio_desc_num = 0;
	for (src = (const struct usb_descriptor_header **)f_audio_descs; *src; ++src) {
		bytes += (*src)->bLength;
		n_desc++;
	}

	audio_desc_num = (n_desc + 1) * sizeof(*src) + bytes;
	UAC_INFO("copy_descriptors bytes = %d, ndesc = %d\n", bytes, n_desc);
	mem = kmalloc(audio_desc_num+20, GFP_KERNEL);
	hdr = mem;
	dst = mem;
	mem += (n_desc + 1) * sizeof(*src);

	UVC_COPY_DESCRIPTORS(mem, dst, (const struct usb_descriptor_header * const *)f_audio_descs);
	*dst = (struct usb_descriptor_header *)NULL;
	return hdr;

}

static void
uac_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	audio_device_exit();
}

static int __init
uac_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_ep *ep;
	int ret = -EINVAL;
	int i;

	UAC_DBG("bind~\n");

	ep = usb_ep_autoconfig(cdev->gadget, &audio_mic_streaming_ep);
	if (!ep) {
		UAC_ERR("Unable to allocate audio EP\n");
		goto error;
	}

	g_audio->ep = ep;


	/* Allocate interface IDs. */
	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto error;

	UAC_INFO("control_intf = %d\n", ret);
	audio_iad_desc.bFirstInterface = ret;
	ac_intf_desc.bInterfaceNumber  = ret;
	g_audio->ac_intf = ret;

	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto error;

	UAC_INFO("streaming_intf = %d\n", ret);
	as_mic_if_alt_0_desc.bInterfaceNumber = ret;
	as_mic_if_alt_1_desc.bInterfaceNumber = ret;
	g_audio->as_mic_intf = ret;
	ac_header_desc.baInterfaceNr[0] = ret;


#ifdef ENABLE_SPEAKER_DESC

	ep = usb_ep_autoconfig(cdev->gadget, &audio_spk_streaming_ep);
	UAC_DBG("audio_in_ep_id  = %s\n", ep->name);
	if (!ep) {
		UAC_ERR("Unable to allocate audio EP\n");
		goto error;
	}

	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto error;

	UAC_INFO("streaming_intf2 = %d\n", ret);
	as_spk_if_alt_0_desc.bInterfaceNumber = ret;
	as_spk_if_alt_1_desc.bInterfaceNumber = ret;
	g_audio->as_spk_intf = ret;
	ac_header_desc.baInterfaceNr[1] = ret;
 #endif

	/* Copy descriptors. */
	f->descriptors = uac_copy_descriptors(USB_SPEED_FULL);
	f->hs_descriptors = uac_copy_descriptors(USB_SPEED_FULL);


	g_audio->req_buf_size = 16;
	for (i = 0; i < AUDIO_REQ_NUM; i++) {
		g_audio->req[i] = usb_ep_alloc_request(g_audio->ep, GFP_ATOMIC);
		if (g_audio->req[i]) {
			g_audio->req_buf[i] = kzalloc(g_audio->req_buf_size, GFP_ATOMIC);
			g_audio->req[i]->buf = g_audio->req_buf[i];
		}
	}


#ifdef UAC_BIND_DEACTIVATE
	ret = usb_function_deactivate(f);
	if (ret < 0)
		goto error;
#endif

	return 0;

error:
	uac_function_unbind(c, f);
	return ret;
}






int __init
uac_bind_config(struct usb_configuration *c)
{

	int ret = 0;
	struct usb_function *func;

	/* Allocate string descriptor numbers. */
	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto error;
	uac_en_us_strings[UAC_STR_ASSOCIATION_IDX].id = ret;
	audio_iad_desc.iFunction = ret;

#if 0
	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto error;
	uac_en_us_strings[UAC_STR_CONTROL_IDX].id = ret;
	ac_intf_desc.iInterface = ret;

	ret = usb_string_id(c->cdev));
	if (ret < 0)
		goto error;
	uac_en_us_strings[UAC_STR_STREAMING_IDX].id = ret;
	as_interface_alt_0_desc.iInterface = ret;
	as_interface_alt_1_desc.iInterface = ret;
#endif

	g_audio = kzalloc(sizeof(*g_audio), GFP_KERNEL);


	func  = kzalloc(sizeof(*func), GFP_KERNEL);

	/* Register the function. */
	func->name    = "uac";
	func->strings = uac_function_strings;
	func->bind    = uac_function_bind;
	func->unbind  = uac_function_unbind;
	func->get_alt = uac_function_get_alt;
	func->set_alt = uac_function_set_alt;
	func->disable = uac_function_disable;
	func->setup   = uac_function_setup;

	g_audio->func = func;
	ret = usb_add_function(c, func);


	audio_device_init();

	return 0;

error:
	return ret;
}


