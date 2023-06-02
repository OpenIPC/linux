#ifndef _MDRV_WARP_IO_H_
#define _MDRV_WARP_IO_H_

#include <linux/ioctl.h>

#define WARP_IOC_MAGIC 'I'
#define WARP_IOC_TRIGGER _IOW(WARP_IOC_MAGIC, 1, MHAL_WARP_CONFIG*)

#endif //_MDRV_WARP_IO_H_