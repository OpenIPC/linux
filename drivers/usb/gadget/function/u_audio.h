/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * u_audio.h -- interface to USB gadget "ALSA sound card" utilities
 *
 * Copyright (C) 2016
 * Author: Ruslan Bilovol <ruslan.bilovol@gmail.com>
 */

#ifndef __U_AUDIO_H
#define __U_AUDIO_H

#include <linux/usb/composite.h>

#if defined(CONFIG_ARCH_XMFALCON) || defined(CONFIG_ARCH_XMORCA)
#include <sound/control.h>
#include <sound/core.h>
#include <sound/pcm.h>
#endif

struct uac_params {
	/* playback */
	int p_chmask;	/* channel mask */
	int p_srate;	/* rate in Hz */
	int p_ssize;	/* sample size */

	/* capture */
	int c_chmask;	/* channel mask */
	int c_srate;	/* rate in Hz */
	int c_ssize;	/* sample size */

	int req_number; /* number of preallocated requests */
};

struct g_audio {
	struct usb_function func;
	struct usb_gadget *gadget;

	struct usb_ep *in_ep;
	struct usb_ep *out_ep;

	/* Max packet size for all in_ep possible speeds */
	unsigned int in_ep_maxpsize;
	/* Max packet size for all out_ep possible speeds */
	unsigned int out_ep_maxpsize;

	/* The ALSA Sound Card it represents on the USB-Client side */
	struct snd_uac_chip *uac;

	struct uac_params params;
#if defined(CONFIG_ARCH_XMFALCON) || defined(CONFIG_ARCH_XMORCA)
	int p_volume;
	int c_volume;

	bool p_mute;
	bool c_mute;
#endif
};

static inline struct g_audio *func_to_g_audio(struct usb_function *f)
{
	return container_of(f, struct g_audio, func);
}

static inline uint num_channels(uint chanmask)
{
	uint num = 0;

	while (chanmask) {
		num += (chanmask & 1);
		chanmask >>= 1;
	}

	return num;
}

/*
 * g_audio_setup - initialize one virtual ALSA sound card
 * @g_audio: struct with filled params, in_ep_maxpsize, out_ep_maxpsize
 * @pcm_name: the id string for a PCM instance of this sound card
 * @card_name: name of this soundcard
 *
 * This sets up the single virtual ALSA sound card that may be exported by a
 * gadget driver using this framework.
 *
 * Context: may sleep
 *
 * Returns zero on success, or a negative error on failure.
 */
int g_audio_setup(struct g_audio *g_audio, const char *pcm_name,
					const char *card_name);
void g_audio_cleanup(struct g_audio *g_audio);

int u_audio_start_capture(struct g_audio *g_audio);
void u_audio_stop_capture(struct g_audio *g_audio);
int u_audio_start_playback(struct g_audio *g_audio);
void u_audio_stop_playback(struct g_audio *g_audio);

#if defined(CONFIG_ARCH_XMFALCON) || defined(CONFIG_ARCH_XMORCA)
void g_audio_notify(struct g_audio *g_audio, int index);

#define UAC1_VOLUME_STEP (256)
#define PLAYBACK_VOLUME_MAX  (18)
#define PLAYBACK_VOLUME_MIN  (-81)
#define PLAYBACK_VOLUME_RES  (1)
#define PLAYBACK_VOLUME_CUR  (10)
#define CAPTURE_VOLUME_MAX  (18)
#define CAPTURE_VOLUME_MIN  (-81)
#define CAPTURE_VOLUME_RES  (1)
#define CAPTURE_VOLUME_CUR  (10)

#define UAC_VOLUME_ATTR_TO_DB(attr) ((s8)((attr / UAC1_VOLUME_STEP)))
#define DB_TO_UAC_VOLUME_ATTR(db) ((short)(db * UAC1_VOLUME_STEP))

#define SND_CTL_PLAYBACK_VOL_ID  1
#define SND_CTL_PLAYBACK_MUTE_ID 2
#define SND_CTL_CAPTURE_VOL_ID   3
#define SND_CTL_CAPTURE_MUTE_ID  4

#endif

#endif /* __U_AUDIO_H */
