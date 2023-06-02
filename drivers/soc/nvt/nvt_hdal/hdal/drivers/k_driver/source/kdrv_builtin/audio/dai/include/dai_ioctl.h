#ifndef __DAI_IOCTL_CMD_H_
#define __DAI_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int reg_addr;
	unsigned int reg_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int count;
	REG_INFO reglist[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define DAI_IOC_COMMON_TYPE 'M'
#define DAI_IOC_START                   _IO(DAI_IOC_COMMON_TYPE, 1)
#define DAI_IOC_STOP                    _IO(DAI_IOC_COMMON_TYPE, 2)

#define DAI_IOC_READ_REG                _IOWR(DAI_IOC_COMMON_TYPE, 3, void*)
#define DAI_IOC_WRITE_REG               _IOWR(DAI_IOC_COMMON_TYPE, 4, void*)
#define DAI_IOC_READ_REG_LIST           _IOWR(DAI_IOC_COMMON_TYPE, 5, void*)
#define DAI_IOC_WRITE_REG_LIST          _IOWR(DAI_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
