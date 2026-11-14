#ifndef	__AK_AUDIO_INF_H
#define	__AK_AUDIO_INF_H

extern int audio_bind(struct usb_composite_dev *cdev);
extern int audio_unbind(struct usb_composite_dev *cdev);

extern int audio_do_config(struct usb_configuration *c);

#endif	/* __AK_AUDIO_INF_H */
