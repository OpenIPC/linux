#ifndef __ISF_AUDOUT_IOCTL_CMD_H_
#define __ISF_AUDOUT_IOCTL_CMD_H_

#include <linux/ioctl.h>

//============================================================================
// IOCTL command
//============================================================================
#define ISF_AUDOUT_IOC_COMMON_TYPE 'M'
#define ISF_AUDOUT_IOC_START                   _IO(ISF_AUDOUT_IOC_COMMON_TYPE, 1)
#define ISF_AUDOUT_IOC_STOP                    _IO(ISF_AUDOUT_IOC_COMMON_TYPE, 2)

#define ISF_AUDOUT_IOC_READ_REG                _IOWR(ISF_AUDOUT_IOC_COMMON_TYPE, 3, void*)
#define ISF_AUDOUT_IOC_WRITE_REG               _IOWR(ISF_AUDOUT_IOC_COMMON_TYPE, 4, void*)
#define ISF_AUDOUT_IOC_READ_REG_LIST           _IOWR(ISF_AUDOUT_IOC_COMMON_TYPE, 5, void*)
#define ISF_AUDOUT_IOC_WRITE_REG_LIST          _IOWR(ISF_AUDOUT_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
