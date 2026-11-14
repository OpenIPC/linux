/*
 * u_uac1.h -- interface to USB gadget "ALSA AUDIO" utilities
 *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __U_AUDIO_H
#define __U_AUDIO_H

#include <linux/device.h>
#include <linux/err.h>
#include <linux/usb/audio.h>
#include <linux/usb/composite.h>
#include <linux/cdev.h>

#define AK_USB_REQ_DISCONNECT (0xF0)

/*
 * This represents the USB side of an audio card device, managed by a USB
 * function which provides control and stream interfaces.
 */

struct f_audio_buf {
	u8 *buf;
	int actual;
	struct list_head list;
};

struct gaudio_snd_dev {
    struct cdev data_cdev;
    struct device *data_dev;

    struct cdev control_cdev;
    struct device *control_dev;

	struct f_audio_buf *data_buf;
    struct f_audio_buf *control_buf;

	struct list_head data_queue;
	struct list_head control_queue;

    spinlock_t data_lock;
    spinlock_t control_lock;

    unsigned data_opened:1;
    unsigned control_opened:1;
    unsigned data_writable:1;
    unsigned data_running:1;
    unsigned control_running:1;

    wait_queue_head_t data_wq;
    wait_queue_head_t control_wq;

    struct gaudio *card;

    int data_read_threshold;
    int control_read_threshold;

    int capture_req_count;
};

struct gaudio {
    struct usb_function		func;
    struct usb_gadget		*gadget;

    struct gaudio_snd_dev		playback;
    struct gaudio_snd_dev		capture;

    /* endpoints handle full and/or high speeds */
    struct usb_ep			*out_ep;
    struct usb_ep			*in_ep;
};

struct f_uac1_opts {
	struct usb_function_instance	func_inst;
	int				max_packet_size;
	int				playback_req_count;
	int				audio_buf_size;
	int				audio_buf_max_count;
    int capture_req_max_count;
    int playback_min_read_bytes;
	unsigned			bound:1;
	struct mutex			lock;
	int				refcnt;
};

struct ak_usb_audio_ep_control {
    const char *name;
	struct list_head list;
    u8 epaddr;
	u8 cs;
	int data[5];
    struct gaudio_snd_dev *snd;
	int (*set)(struct ak_usb_audio_ep_control *con, u8 cmd, int value);
	int (*get)(struct ak_usb_audio_ep_control *con, u8 cmd);
};

struct f_audio {
	struct gaudio			card;

	/* Control Set command */
	struct list_head cs;
	struct usb_audio_control *set_con;

    /* Endpoint Controls */
    struct list_head ep_cs;
    struct ak_usb_audio_ep_control *ep_set_con;

    struct usb_ctrlrequest set_ctrlreq;
	u8 set_cmd;
};

static inline struct f_audio *func_to_audio(struct usb_function *f)
{
	return container_of(f, struct f_audio, card.func);
}

struct f_audio_buf *f_audio_buffer_alloc(int buf_size);
void f_audio_buffer_free(struct f_audio_buf *audio_buf);

int copy_to_control_buffer_nolock(struct gaudio_snd_dev *snd, void *data, unsigned int n);

int get_data_buffer_count_nolock(struct gaudio_snd_dev *snd);
int get_control_buffer_count_nolock(struct gaudio_snd_dev *snd);

int get_control_bytes_nolock(struct gaudio_snd_dev *snd);
int get_data_bytes_nolock(struct gaudio_snd_dev *snd);

int gaudio_setup(struct gaudio *card);
void gaudio_cleanup(struct gaudio *the_card);

void f_audio_complete(struct usb_ep *ep, struct usb_request *req);

#endif /* __U_AUDIO_H */
