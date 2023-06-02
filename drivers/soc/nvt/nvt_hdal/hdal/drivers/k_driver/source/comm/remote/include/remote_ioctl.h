#ifndef _REMOTE_IOCTL_CMD_H_
#define _REMOTE_IOCTL_CMD_H_

#include <linux/ioctl.h>
#include "comm/remote.h"

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int uiAddr;
	unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int uiCount;
	REG_INFO RegList[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

typedef struct remote_config_info {
	REMOTE_CONFIG_ID id;
	UINT32 value;
} REMOTE_CONFIG_INFO;

//============================================================================
// IOCTL command
//============================================================================

#define REMOTE_IOC_COMMON_TYPE 'M'
#define REMOTE_IOC_START                   _IO(REMOTE_IOC_COMMON_TYPE, 1)
#define REMOTE_IOC_STOP                    _IO(REMOTE_IOC_COMMON_TYPE, 2)

#define REMOTE_IOC_READ_REG                _IOWR(REMOTE_IOC_COMMON_TYPE, 3, void*)
#define REMOTE_IOC_WRITE_REG               _IOWR(REMOTE_IOC_COMMON_TYPE, 4, void*)
#define REMOTE_IOC_READ_REG_LIST           _IOWR(REMOTE_IOC_COMMON_TYPE, 5, void*)
#define REMOTE_IOC_WRITE_REG_LIST          _IOWR(REMOTE_IOC_COMMON_TYPE, 6, void*)

#define REMOTE_IOC_SET_ENABLE              _IOWR(REMOTE_IOC_COMMON_TYPE, 7, UINT32*)
#define REMOTE_IOC_GET_ENABLE              _IOR(REMOTE_IOC_COMMON_TYPE, 7, UINT32*)
#define REMOTE_IOC_SET_CONFIG              _IOWR(REMOTE_IOC_COMMON_TYPE, 8, REMOTE_CONFIG_INFO*)
#define REMOTE_IOC_GET_CONFIG              _IOR(REMOTE_IOC_COMMON_TYPE, 8, REMOTE_CONFIG_INFO*)
#define REMOTE_IOC_SET_INTERRUPT_ENABLE    _IOWR(REMOTE_IOC_COMMON_TYPE, 9, REMOTE_INTERRUPT*)
#define REMOTE_IOC_GET_INTERRUPT_ENABLE    _IOR(REMOTE_IOC_COMMON_TYPE, 9, REMOTE_INTERRUPT*)

#endif