#ifndef __EAC_IOCTL_CMD_H_
#define __EAC_IOCTL_CMD_H_

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
#define EAC_IOC_COMMON_TYPE 'M'
#define EAC_IOC_START                   _IO(EAC_IOC_COMMON_TYPE, 1)
#define EAC_IOC_STOP                    _IO(EAC_IOC_COMMON_TYPE, 2)

#define EAC_IOC_READ_REG                _IOWR(EAC_IOC_COMMON_TYPE, 3, void*)
#define EAC_IOC_WRITE_REG               _IOWR(EAC_IOC_COMMON_TYPE, 4, void*)
#define EAC_IOC_READ_REG_LIST           _IOWR(EAC_IOC_COMMON_TYPE, 5, void*)
#define EAC_IOC_WRITE_REG_LIST          _IOWR(EAC_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
