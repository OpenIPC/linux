#ifndef __SENPHY_IOCTL_CMD_H_
#define __SENPHY_IOCTL_CMD_H_

#include <linux/ioctl.h>

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int ui_addr;
	unsigned int ui_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int ui_count;
	REG_INFO reg_list[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define SENPHY_IOC_COMMON_TYPE 'M'
#define SENPHY_IOC_START                   _IO(SENPHY_IOC_COMMON_TYPE, 1)
#define SENPHY_IOC_STOP                    _IO(SENPHY_IOC_COMMON_TYPE, 2)

#define SENPHY_IOC_READ_REG                _IOWR(SENPHY_IOC_COMMON_TYPE, 3, void*)
#define SENPHY_IOC_WRITE_REG               _IOWR(SENPHY_IOC_COMMON_TYPE, 4, void*)
#define SENPHY_IOC_READ_REG_LIST           _IOWR(SENPHY_IOC_COMMON_TYPE, 5, void*)
#define SENPHY_IOC_WRITE_REG_LIST          _IOWR(SENPHY_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif
