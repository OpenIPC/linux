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

static void f_audio_complete(struct usb_ep *ep, struct usb_request *req);

#define UAC_ERR(msg...)		printk(KERN_EMERG "UAC, " msg)
#define UAC_DBG(msg...)		printk(KERN_INFO "UAC, " msg)
#define UAC_INFO(msg...)



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




#ifdef CONFIG_ENABLE_SPEAKER_DESC

#define UAC_IF_COUNT                3
#define UAC_COLLECTION_NUM          2
#define AC_HEADER_TOTAL_LENGTH     0x44

#else

#define UAC_IF_COUNT                2
#define UAC_COLLECTION_NUM          1
#define AC_HEADER_TOTAL_LENGTH      0x26


#endif


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

#ifdef CONFIG_ENABLE_SPEAKER_DESC
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
	.bDelay             =	0x01,
	.wFormatTag         =	UAC_FORMAT_TYPE_I_PCM,
};


#define SAMPLE_FREQ     (8000)
#define SAMPLE_FREQ_16K     (16000)
DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(2);

static struct uac_format_type_i_discrete_descriptor_2 as_mic_type_i_desc = {
	.bLength            =	UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(2),
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType        =	UAC_FORMAT_TYPE_I,
	.bNrChannels        =   AUDIO_CHN_NUM,
	.bSubframeSize      =	2,
	.bBitResolution     =	16,
	.bSamFreqType       =	2,
	.tSamFreq[0][0]     =   (SAMPLE_FREQ & 0xff),
	.tSamFreq[0][1]     =   (SAMPLE_FREQ >> 8) & 0xff,
	.tSamFreq[0][2]     =   0,
	.tSamFreq[1][0]     =   (SAMPLE_FREQ_16K & 0xff),
	.tSamFreq[1][1]     =   (SAMPLE_FREQ_16K >> 8) & 0xff,
	.tSamFreq[1][2]     =   0,
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


#ifdef CONFIG_ENABLE_SPEAKER_DESC
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
	.bDelay             =	0x01,
	.wFormatTag         =	UAC_FORMAT_TYPE_I_PCM,
};


static struct uac_format_type_i_discrete_descriptor_2 as_spk_type_i_desc = {
	.bLength            =	UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(2),
	.bDescriptorType    =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType        =	UAC_FORMAT_TYPE_I,
	.bNrChannels        =   AUDIO_CHN_NUM,
	.bSubframeSize      =	2,
	.bBitResolution     =	16,
	.bSamFreqType       =	2,
	.tSamFreq[0][0]     =   (SAMPLE_FREQ & 0xff),
	.tSamFreq[0][1]     =   (SAMPLE_FREQ >> 8) & 0xff,
	.tSamFreq[0][2]     =   0,
	.tSamFreq[1][0]     =   (SAMPLE_FREQ_16K & 0xff),
	.tSamFreq[1][1]     =   (SAMPLE_FREQ_16K >> 8) & 0xff,
	.tSamFreq[1][2]     =   0,

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



static struct usb_descriptor_header *f_audio_descs[] = {
	(struct usb_descriptor_header *)&audio_iad_desc,
	(struct usb_descriptor_header *)&ac_intf_desc,
	(struct usb_descriptor_header *)&ac_header_desc,

	(struct usb_descriptor_header *)&audio_mic_it_desc,
	(struct usb_descriptor_header *)&audio_mic_ot_desc,
	(struct usb_descriptor_header *)&audio_mic_fu_desc,

#ifdef CONFIG_ENABLE_SPEAKER_DESC
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

#ifdef CONFIG_ENABLE_SPEAKER_DESC
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
#define AUDIO_REC_NUM		    256

struct uac_audio {
	struct usb_function      *func;
	struct usb_ep            *ep;
	struct usb_ep            *out_ep;
	struct usb_request       *req[AUDIO_REQ_NUM];
	void                     *req_buf[AUDIO_REQ_NUM];
	int                      req_valid[AUDIO_REQ_NUM];
	int                      req_buf_size;
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	struct f_audio_buf       *copy_buf;
	struct usb_request       *out_req[AUDIO_REC_NUM];
	void                     *out_req_buf[AUDIO_REC_NUM];
	int                      req_speaker_buf_size;
	int                      audio_buf_size;
	int                      spk_sample_rate;
	struct list_head         play_queue;
	struct work_struct       playback_work;
	spinlock_t			     lock;
#endif
	int                      ac_connected;
	int                      stream_sta[3];
	int                      ac_intf;
	int                      as_mic_intf;
	int                      as_spk_intf;

	int                      mute_set;
	int                      spkmute_set;
	int                      vol_set;
	int                      spkvol_set;
	int                      sample_rate_set;
	int                      set_cmd;
	int                      set_id;

};


static struct uac_audio *g_audio;

#ifdef CONFIG_ENABLE_SPEAKER_DESC
static struct f_audio_buf *f_audio_buffer_alloc(int buf_size)
{
	struct f_audio_buf *copy_buf;

	copy_buf = kzalloc(sizeof *copy_buf, GFP_ATOMIC);
	if (copy_buf == NULL)
		return NULL;

	copy_buf->buf = kzalloc(buf_size, GFP_ATOMIC);
	if (copy_buf->buf == NULL) {
		kfree(copy_buf);
		return NULL;
	}

	return copy_buf;
}
#endif

static int uac_is_stream_on(int id)
{
	return (g_audio->stream_sta[id] == UAC_STREAM_ON);
}

static int uac_audio_enable(int enable, int id)
{
	unsigned int i, ret = 0;

	if (id == 1)
		uac_stream(enable);
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	else
		uac_stream_spk(enable);
#endif
	if (!enable) {
		if (uac_is_stream_on(id)) {
			if (id == 1) {
				for (i = 0; i < AUDIO_REQ_NUM; ++i)
					usb_ep_dequeue(g_audio->ep, g_audio->req[i]);
				usb_ep_disable(g_audio->ep);

#ifdef CONFIG_ENABLE_SPEAKER_DESC
			} else if (id == 2) {
				for (i = 0; i < AUDIO_REC_NUM; ++i)
					usb_ep_dequeue(g_audio->out_ep, g_audio->out_req[i]);
				usb_ep_disable(g_audio->out_ep);
#endif
			}
			g_audio->stream_sta[id] = UAC_STREAM_OFF;
			UAC_INFO("STREAM %s OFF\n", id == 1 ? "mic" : "speaker");
		}
		return 0;
	}

	if (id == 1)
	{
		ret = config_ep_by_speed(g_audio->func->config->cdev->gadget,
			g_audio->func, g_audio->ep);
		if (ret)
			return ret;
		usb_ep_enable(g_audio->ep);
	}
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	if (id == 2)
	{
		ret = config_ep_by_speed(g_audio->func->config->cdev->gadget,
			g_audio->func, g_audio->out_ep);
		if (ret)
			return ret;
		usb_ep_enable(g_audio->out_ep);
	}
#endif

	g_audio->stream_sta[id] = UAC_STREAM_ON;
	UAC_INFO("STREAM %s ON\n", id == 1 ? "mic" : "speaker");
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
		req->complete =	f_audio_complete;
#if 1
		err = usb_ep_queue(g_audio->ep,	req,  GFP_ATOMIC);
		if (err)
			UAC_ERR("transfer_data usb_ep_queue error, %s  req: %d\n",	g_audio->ep->name, err);
#endif
	}

	wake_up_app();
	UAC_INFO("sendMsg audio!\n");
}

#ifdef CONFIG_ENABLE_SPEAKER_DESC

static void f_audio_buffer_free(struct f_audio_buf *audio_buf)
{
	kfree(audio_buf->buf);
	kfree(audio_buf);
}

static int f_audio_out_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uac_audio *audio = req->context;
	struct f_audio_buf *copy_buf = audio->copy_buf;
	int audio_buf_size;
	int err;

	audio_buf_size = audio->audio_buf_size;

	if (!copy_buf)
		return -EINVAL;

	if (req->actual > 64) {
		pr_err("copy_buf->actual %d req->actual %d\n",
			copy_buf->actual, req->actual);
		return -EINVAL;
	}
	if (req->actual == 32)
		audio->spk_sample_rate = 16000;
	else if (req->actual == 16)
		audio->spk_sample_rate = 8000;
	/* Copy buffer is full, add it to the play_queue */
	if (audio_buf_size - copy_buf->actual < req->actual) {
		list_add_tail(&copy_buf->list, &audio->play_queue);
		wake_up_app_spk();
		copy_buf = f_audio_buffer_alloc(audio_buf_size);
		if (copy_buf == NULL) {
			pr_err("%s--%d f_audio_buffer_alloc failed~\n", __func__, __LINE__);
			return -ENOMEM;
		}
	}

	memcpy(copy_buf->buf + copy_buf->actual, req->buf, req->actual);
	copy_buf->actual += req->actual;
	audio->copy_buf = copy_buf;
	err = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (err)
		printk("%s queue req: %d\n", ep->name, err);

	return 0;

}

int audio_recv_data(struct f_audio_buf *args)
{
	int ret;
	struct f_audio_buf *play_buf;
	struct f_audio_buf usr_buf;

	ret = copy_from_user(&usr_buf, args, sizeof(struct f_audio_buf));

	spin_lock_irq(&g_audio->lock);
	if (list_empty(&g_audio->play_queue)) {
		printk(KERN_EMERG "%s--%d play_queue is null\n", __func__, __LINE__);
		spin_unlock_irq(&g_audio->lock);
		return -1;
	}
	play_buf = list_first_entry(&g_audio->play_queue,
			struct f_audio_buf, list);
	list_del(&play_buf->list);
	if (play_buf->buf && usr_buf.buf) {
		ret = copy_to_user(usr_buf.buf, play_buf->buf, play_buf->actual);
		put_user(play_buf->actual, (unsigned int __user *)&(args->actual));
	}
	spin_unlock_irq(&g_audio->lock);
	f_audio_buffer_free(play_buf);
	return 0;
}

#endif

static void uac_event_proc(struct usb_ep *ep, struct usb_request *req)
{
	int a;
	int sample;
	unsigned char *pData = NULL;

	if (ep != g_audio->ep && ep != g_audio->out_ep) {
		if (!g_audio->set_cmd)
			return;
		pData = req->buf;
		switch (g_audio->set_cmd) {
		case UAC_FU_MUTE:
			if (g_audio->set_id == FEATURE_UNIT_ID) {
				g_audio->mute_set = pData[0];
				uac_mic_FU();
			}
			else if (g_audio->set_id == SPEAKER_FU_ID) {
				g_audio->spkmute_set = pData[0];
#ifdef CONFIG_ENABLE_SPEAKER_DESC
				uac_spk_FU();
#endif
			}
			break;
		case UAC_FU_VOLUME:
			if (g_audio->set_id == FEATURE_UNIT_ID) {
				g_audio->vol_set = (pData[0] | pData[1] << 8)*100/UAC_MIC_MAX_GAIN;
				if (g_audio->mute_set)
					break;
				uac_mic_FU();
			}
			else if (g_audio->set_id == SPEAKER_FU_ID) {
				g_audio->spkvol_set = pData[0] | pData[1] << 8;
				if (g_audio->spkmute_set)
					break;
#ifdef CONFIG_ENABLE_SPEAKER_DESC
				uac_spk_FU();
#endif
			}
			break;
		default:
			if (g_audio->set_cmd == audio_mic_streaming_ep.bEndpointAddress) {
				sample = (pData[0]) | (pData[1]<<8) | (pData[2]<<16);
				if (sample != g_audio->sample_rate_set) {
					g_audio->sample_rate_set = sample;
					uac_stream(1);
				}
			}
			break;
		}
		g_audio->set_cmd = 0;
		return;
	}
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	else if (ep == g_audio->out_ep)
		f_audio_out_ep_complete(ep, req);
#endif
		else {
			a = (int)req->context;
			g_audio->req_valid[a] = 1;
			wake_up_app();
		}
}

static void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{

	switch (req->status) {
	case 0:
		uac_event_proc(ep, req);
		break;

	case -ESHUTDOWN:
		UAC_INFO("f_audio_complete ESHUTDOWN.\n");
		goto requeue;

	default:
		/* UAC_INFO("request completed with status %d.\n",	req->status); */
		goto requeue;
	}

requeue:
	return;
}

int audio_mic_simple_rate(void)
{
	return g_audio->sample_rate_set;
}

int audio_mic_volume(void)
{
	return g_audio->vol_set;
}

int audio_mic_mute(void)
{
	return g_audio->mute_set;
}

#ifdef CONFIG_ENABLE_SPEAKER_DESC
int audio_spk_simple_rate(void)
{
	return g_audio->spk_sample_rate;
}

int audio_spk_volume(void)
{
	return g_audio->spkvol_set;
}

int audio_spk_mute(void)
{
	return g_audio->spkmute_set;
}
#endif
struct audio_data {
	unsigned char *pData;
	unsigned char *pPos;
	unsigned char *pEnd;
};

int audio_send_data(void *args)
{
	struct audio_data *pAudio_usr = args;
	struct audio_data pAudio;
	char *data_buf = NULL;
	int data_len = 0;
	int i;
	struct usb_request *req = NULL;

	int len = g_audio->req_buf_size;
	int send_len = 0;
	int ret = 0;
	int err = 0;

	if (!uac_is_stream_on(1))
		return 0;

	ret = copy_from_user(&pAudio, pAudio_usr, sizeof(struct audio_data));
	data_len = (unsigned int)pAudio.pEnd - (unsigned int)pAudio.pData;

	if (data_len == 0) {
		data_buf = kmalloc(32, GFP_KERNEL);
		memset(data_buf, 0, 32);
		pAudio.pEnd = data_buf + 32;
	} else {
		data_buf = kmalloc(data_len, GFP_KERNEL);
		ret = copy_from_user(data_buf, pAudio.pData, data_len);
		pAudio.pEnd = data_buf + data_len;
	}
	pAudio.pData = pAudio.pPos = data_buf;

	for (i = 0; i < AUDIO_REQ_NUM; i++) {
		if (g_audio->req_valid[i]) {
			g_audio->req_valid[i] = 0;
			req = g_audio->req[i];
			req->buf = g_audio->req_buf[i];
			req->context = (void *)i;
			req->complete =	f_audio_complete;

			if (pAudio.pPos >= pAudio.pEnd) {
				pAudio.pPos = pAudio.pData;
				ret = 1;
				break;
			} else if (pAudio.pPos <  pAudio.pData)
				pAudio.pPos = pAudio.pData;

			if (pAudio.pPos + g_audio->req_buf_size >= pAudio.pEnd) {
				len = pAudio.pEnd-pAudio.pPos;
				memcpy(req->buf, pAudio.pPos, len);
				send_len += len;

				pAudio.pPos = pAudio.pData;
				ret = 1;
			} else {
				len = g_audio->req_buf_size;
				memcpy(req->buf, pAudio.pPos, g_audio->req_buf_size);
				pAudio.pPos += g_audio->req_buf_size;
				send_len += g_audio->req_buf_size;
			}
			req->length = len;

			err = usb_ep_queue(g_audio->ep,	req,  GFP_ATOMIC);
			if (err)
				UAC_ERR("audio usb_ep_queue error, %s  req: %d\n",	g_audio->ep->name, err);

			if (ret)
				break;
		}
	}

	if (g_audio->sample_rate_set == 16000)
		g_audio->req_buf_size = 32;
	else if (g_audio->sample_rate_set == 8000)
		g_audio->req_buf_size = 16;

	kfree(data_buf);

	if (data_len == 0)
		return 0;

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

	UAC_INFO("setup, req: %02x.%02x value: %04x index: %04x length: %d\n",
	ctrl->bRequestType, ctrl->bRequest,
	w_value, w_index, w_length);

	g_audio->set_cmd = 0;

	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) {
		value = 3;
		sendAudioReq = 1;
		cs = ctrl->wValue >> 8;
		UAC_INFO("EndpointAddress = %x, id = %x, cs = %x\n",
			audio_mic_streaming_ep.bEndpointAddress,
			ctrl->wIndex, cs);
		if (audio_mic_streaming_ep.bEndpointAddress == ctrl->wIndex && cs == 1)
				g_audio->set_cmd = ctrl->wIndex;
	} else {
		cs = ctrl->wValue >> 8;
		id = ctrl->wIndex >> 8;
		UAC_INFO("id = %d, cs = %d\n", id, cs);
		pData = req->buf;
		if (id == FEATURE_UNIT_ID || id == SPEAKER_FU_ID) {
			switch (cs) {
			case UAC_FU_MUTE:
				value = 1;
				switch (ctrl->bRequest) {
				case UAC_SET_CUR:
					g_audio->set_cmd = cs;
					g_audio->set_id = id;
					break;

				case UAC_GET_CUR:
					if (id == FEATURE_UNIT_ID)
						pData[0] = g_audio->mute_set;
					if (id == SPEAKER_FU_ID)
						pData[0] = g_audio->spkmute_set;
					break;
				}
				break;

			case UAC_FU_VOLUME:
				value = 2;
				switch (ctrl->bRequest) {
				case UAC_SET_CUR:
					g_audio->set_cmd = cs;
					g_audio->set_id = id;
					break;

				case UAC_GET_CUR:
					if (id == FEATURE_UNIT_ID) {
						pData[0] = g_audio->vol_set & 0xff;
						pData[1] = (g_audio->vol_set >> 8) & 0xff;
					}
					if (id == SPEAKER_FU_ID) {
						pData[0] = g_audio->spkvol_set & 0xff;
						pData[1] = (g_audio->spkvol_set >> 8) & 0xff;
					}
					break;

				case UAC_GET_MIN:
					pData[0] = 0x00;
					pData[1] = 0x00;
					break;

				case UAC_GET_MAX:
					if (id == FEATURE_UNIT_ID) {
						pData[0] = UAC_MIC_MAX_GAIN & 0xff;
						pData[1] = (UAC_MIC_MAX_GAIN >> 8) & 0xff;
					}
					if (id == SPEAKER_FU_ID) {
						pData[0] = 0x64;
						pData[1] = 0x00;
					}
					break;

				case UAC_GET_RES:
					pData[0] = 0x01;
					pData[1] = 0x00;
					break;
				}
				break;
			}
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
		}
	}
	if (sendAudioReq)
		uac_begin_transfer_data();
	return 0;
}


static int
uac_function_get_alt(struct usb_function *f, unsigned int interface)
{
	UAC_INFO("get_alt\n");
	return 0;
}

#ifdef CONFIG_ENABLE_SPEAKER_DESC
static int uac_buffer_alloc(void)
{
	int i, err = 0;

	g_audio->copy_buf =
		f_audio_buffer_alloc(g_audio->audio_buf_size);
	for (i = 0; i < AUDIO_REC_NUM && err == 0; i++) {

		if (g_audio->out_req[i]) {
			if (g_audio->out_req[i]->buf) {
				g_audio->out_req[i]->length =
						g_audio->req_speaker_buf_size;
				g_audio->out_req[i]->context = g_audio;
				g_audio->out_req[i]->complete =
					f_audio_complete;
				err = usb_ep_queue(g_audio->out_ep,
					g_audio->out_req[i], GFP_ATOMIC);
				if (err)
					pr_err("%s queue req: %d\n",
						g_audio->out_ep->name, err);
			} else
				err = -ENOMEM;
		} else
			err = -ENOMEM;
	}
	return err;
}
#endif

static int
uac_function_set_alt(struct usb_function *f,
		unsigned int interface, unsigned int alt)
{
	int err = 0;

	UAC_INFO("set_alt, intf = %d, alt = %d\n", interface, alt);
	if (g_audio->as_mic_intf == interface)
		uac_audio_enable(alt != 0, 1);
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	else if (g_audio->as_spk_intf == interface) {
		uac_audio_enable(alt != 0, 2);
		if (alt == 1) {
			err = uac_buffer_alloc();
			if (err < 0)
				pr_err("uac buffer alloc failed!\n");
		}  else {
			struct f_audio_buf *copy_buf = g_audio->copy_buf;

			if (copy_buf)
				f_audio_buffer_free(copy_buf);
			g_audio->copy_buf = NULL;
		}
	}
#endif
	else if (g_audio->ac_intf == interface)
		g_audio->ac_connected = 1;
	return err;
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



static struct usb_descriptor_header **
uac_copy_descriptors(enum usb_device_speed speed)
{
	const struct usb_descriptor_header **src;
	struct usb_descriptor_header **dst;
	struct usb_descriptor_header **hdr;
	unsigned int n_desc = 0;
	unsigned int bytes  = 0;
	void *mem;

	int audio_desc_num = 0;

	for (src = (const struct usb_descriptor_header **)f_audio_descs;
					*src; ++src) {
		bytes += (*src)->bLength;
		n_desc++;
	}

	audio_desc_num = (n_desc + 1) * sizeof(*src) + bytes;
	UAC_INFO("copy_descriptors bytes = %d, ndesc = %d\n", bytes, n_desc);
	mem = kmalloc(audio_desc_num+20, GFP_KERNEL);
	hdr = mem;
	dst = mem;
	mem += (n_desc + 1) * sizeof(*src);

	UVC_COPY_DESCRIPTORS(mem, dst,
		(const struct usb_descriptor_header * const *)f_audio_descs);
	*dst = (struct usb_descriptor_header *)NULL;
	return hdr;

}

static void
uac_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	audio_device_exit();
}

static int
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


#ifdef CONFIG_ENABLE_SPEAKER_DESC

	ep = usb_ep_autoconfig(cdev->gadget, &audio_spk_streaming_ep);
	UAC_DBG("audio_in_ep_id  = %s\n", ep->name);
	if (!ep) {
		UAC_ERR("Unable to allocate audio EP\n");
		goto error;
	}
	g_audio->out_ep = ep;
	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto error;

	UAC_INFO("streaming_intf2 = %d\n", ret);
	as_spk_if_alt_0_desc.bInterfaceNumber = ret;
	as_spk_if_alt_1_desc.bInterfaceNumber = ret;
	g_audio->as_spk_intf = ret;
	ac_header_desc.baInterfaceNr[1] = ret;

	g_audio->req_speaker_buf_size = 64;
	g_audio->audio_buf_size = 640;
	g_audio->copy_buf = NULL;
	for (i = 0; i < AUDIO_REC_NUM; i++) {
		g_audio->out_req[i] =
			usb_ep_alloc_request(g_audio->out_ep, GFP_ATOMIC);
		if (g_audio->out_req[i]) {
			g_audio->out_req_buf[i] =
			kzalloc(g_audio->req_speaker_buf_size, GFP_ATOMIC);
			g_audio->out_req[i]->buf = g_audio->out_req_buf[i];
		}
	}
 #endif
	g_audio->vol_set = 75 * UAC_MIC_MAX_GAIN / 100;
	g_audio->spkvol_set = 75;
	/* Copy descriptors. */
	f->fs_descriptors = uac_copy_descriptors(USB_SPEED_FULL);
	f->hs_descriptors = uac_copy_descriptors(USB_SPEED_FULL);


	g_audio->req_buf_size = 32;
	for (i = 0; i < AUDIO_REQ_NUM; i++) {
		g_audio->req[i] = usb_ep_alloc_request(g_audio->ep, GFP_ATOMIC);
		if (g_audio->req[i]) {
			g_audio->req_buf[i] =
				kzalloc(g_audio->req_buf_size, GFP_ATOMIC);
			g_audio->req[i]->buf = g_audio->req_buf[i];
		}
	}

	return 0;

error:
	uac_function_unbind(c, f);
	return ret;
}






int
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
	ac_intf_desc.iInterface = ret;

	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto error;
	uac_en_us_strings[UAC_STR_STREAMING_IDX].id = ret;
	as_mic_if_alt_0_desc.iInterface = ret;
	as_mic_if_alt_1_desc.iInterface = ret;
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	as_spk_if_alt_0_desc.iInterface = ret;
	as_spk_if_alt_1_desc.iInterface = ret;
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

#ifdef CONFIG_ENABLE_SPEAKER_DESC
	spin_lock_init(&g_audio->lock);
	INIT_LIST_HEAD(&g_audio->play_queue);
#endif

	ret = usb_add_function(c, func);


	audio_device_init();

	return 0;

error:
	return ret;
}


