#ifndef __CSI_IOCTL_CMD_H_
#define __CSI_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int addr;
	unsigned int value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int count;
	REG_INFO reg_list[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define CSI_IOC_COMMON_TYPE 'M'
#define CSI_IOC_START                   _IO(CSI_IOC_COMMON_TYPE, 1)
#define CSI_IOC_STOP                    _IO(CSI_IOC_COMMON_TYPE, 2)

#define CSI_IOC_READ_REG                _IOWR(CSI_IOC_COMMON_TYPE, 3, void*)
#define CSI_IOC_WRITE_REG               _IOWR(CSI_IOC_COMMON_TYPE, 4, void*)
#define CSI_IOC_READ_REG_LIST           _IOWR(CSI_IOC_COMMON_TYPE, 5, void*)
#define CSI_IOC_WRITE_REG_LIST          _IOWR(CSI_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
