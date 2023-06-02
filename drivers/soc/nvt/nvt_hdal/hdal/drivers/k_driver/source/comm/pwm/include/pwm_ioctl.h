#ifndef __PWM_IOCTL_CMD_H_
#define __PWM_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int reg_addr;
	unsigned int reg_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int reg_cnt;
	REG_INFO reg_list[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define PWM_IOC_COMMON_TYPE 'M'
#define PWM_IOC_START                   _IO(PWM_IOC_COMMON_TYPE, 1)
#define PWM_IOC_STOP                    _IO(PWM_IOC_COMMON_TYPE, 2)

#define PWM_IOC_READ_REG                _IOWR(PWM_IOC_COMMON_TYPE, 3, void*)
#define PWM_IOC_WRITE_REG               _IOWR(PWM_IOC_COMMON_TYPE, 4, void*)
#define PWM_IOC_READ_REG_LIST           _IOWR(PWM_IOC_COMMON_TYPE, 5, void*)
#define PWM_IOC_WRITE_REG_LIST          _IOWR(PWM_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
