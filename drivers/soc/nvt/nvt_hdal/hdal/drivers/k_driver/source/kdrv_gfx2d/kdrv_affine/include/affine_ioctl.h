#ifndef __MODULE_IOCTL_CMD_H_
#define __MODULE_IOCTL_CMD_H_

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
#define AFFINE_IOC_COMMON_TYPE 'M'
#define AFFINE_IOC_START _IO(AFFINE_IOC_COMMON_TYPE, 1)
#define AFFINE_IOC_STOP _IO(AFFINE_IOC_COMMON_TYPE, 2)

#define AFFINE_IOC_READ_REG _IOWR(AFFINE_IOC_COMMON_TYPE, 3, void*)
#define AFFINE_IOC_WRITE_REG _IOWR(AFFINE_IOC_COMMON_TYPE, 4, void*)
#define AFFINE_IOC_READ_REG_LIST _IOWR(AFFINE_IOC_COMMON_TYPE, 5, void*)
#define AFFINE_IOC_WRITE_REG_LIST _IOWR(AFFINE_IOC_COMMON_TYPE, 6, void*)
/* Add other command ID here*/


#endif
