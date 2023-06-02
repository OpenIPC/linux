#ifndef _F_UAC_H_
#define _F_UAC_H_

#include <linux/usb/composite.h>
#include <linux/poll.h>
#include <linux/usb/audio.h>


extern void wake_up_app(void);
extern int __init audio_device_init(void);
extern void __exit audio_device_exit(void);
extern void uac_stream(int on);
extern void audio_dev_open(void);
extern int audio_send_data(void *args);


#endif