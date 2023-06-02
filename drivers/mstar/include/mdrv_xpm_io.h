#ifndef _MDRV_XPM_IO_H_
#define _MDRV_XPM_IO_H_

#define XPM_IOCTL_MAGIC             'S'

#define		IOCTL_XPM_REGISTER_SOURCE			_IO(XPM_IOCTL_MAGIC, 0x81)
#define		IOCTL_XPM_DEREGISTER_SOURCE			_IO(XPM_IOCTL_MAGIC, 0x82)
#define 	IOCTL_XPM_MAXNR    					0x83


#endif
