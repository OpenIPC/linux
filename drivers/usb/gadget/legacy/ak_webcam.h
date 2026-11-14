#ifndef	__AK_WEBCAM_INF_H
#define	__AK_WEBCAM_INF_H

extern int webcam_bind(struct usb_composite_dev *cdev);
extern int webcam_unbind(struct usb_composite_dev *cdev);

extern int webcam_config_bind(struct usb_configuration *c);

#endif	/* __AK_WEBCAM_INF_H */
