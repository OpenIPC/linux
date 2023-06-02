#ifndef _F_UAC_H_
#define _F_UAC_H_

#include <linux/usb/composite.h>
#include <linux/poll.h>
#include <linux/usb/audio.h>

#define UAC_MIC_MAX_GAIN            (0x64)

struct f_audio_buf {
	u8 *buf;
	int actual;
	struct list_head list;
};

extern void wake_up_app(void);
extern int audio_device_init(void);
extern void audio_device_exit(void);
extern void uac_stream(int on);
extern void uac_mic_FU(void);
extern int audio_send_data(void *args);
extern int audio_mic_simple_rate(void);
extern int audio_mic_volume(void);
extern int audio_mic_mute(void);

extern int audio_recv_data(struct f_audio_buf *args);
extern void wake_up_app_spk(void);
extern void uac_stream_spk(int on);
extern void uac_spk_FU(void);
extern void speaker_dev_open(void);
extern int audio_spk_simple_rate(void);
extern int audio_spk_volume(void);
extern int audio_spk_mute(void);


#endif