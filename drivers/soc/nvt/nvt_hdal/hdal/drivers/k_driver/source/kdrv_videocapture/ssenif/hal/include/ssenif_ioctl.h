#ifndef __SSENIF_IOCTL_CMD_H_
#define __SSENIF_IOCTL_CMD_H_


#include <linux/ioctl.h>

#define SSENIF_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int ui_addr;
	unsigned int ui_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int ui_count;
	REG_INFO reg_list[SSENIF_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define SSENIF_IOC_COMMON_TYPE 'M'
#define SSENIF_IOC_START                   _IO(SSENIF_IOC_COMMON_TYPE, 1)
#define SSENIF_IOC_STOP                    _IO(SSENIF_IOC_COMMON_TYPE, 2)

#define SSENIF_IOC_READ_REG                _IOWR(SSENIF_IOC_COMMON_TYPE, 3, void*)
#define SSENIF_IOC_WRITE_REG               _IOWR(SSENIF_IOC_COMMON_TYPE, 4, void*)
#define SSENIF_IOC_READ_REG_LIST           _IOWR(SSENIF_IOC_COMMON_TYPE, 5, void*)
#define SSENIF_IOC_WRITE_REG_LIST          _IOWR(SSENIF_IOC_COMMON_TYPE, 6, void*)

#endif
