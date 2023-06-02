#ifndef __MD_IOCTL_CMD_H_
#define __MD_IOCTL_CMD_H_

//#include <linux/ioctl.h>
#include "kwrap/ioctl.h"

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
    unsigned int uiAddr;
    unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
    unsigned int uiCount;
    REG_INFO RegList[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define MD_IOC_COMMON_TYPE 'M'
#define MD_IOC_START                   _VOS_IO(MD_IOC_COMMON_TYPE, 1)
#define MD_IOC_STOP                    _VOS_IO(MD_IOC_COMMON_TYPE, 2)

#define MD_IOC_READ_REG                _VOS_IOWR(MD_IOC_COMMON_TYPE, 3, void*)
#define MD_IOC_WRITE_REG               _VOS_IOWR(MD_IOC_COMMON_TYPE, 4, void*)
#define MD_IOC_READ_REG_LIST           _VOS_IOWR(MD_IOC_COMMON_TYPE, 5, void*)
#define MD_IOC_WRITE_REG_LIST          _VOS_IOWR(MD_IOC_COMMON_TYPE, 6, void*)

#define MD_IOC_OPEN                     _VOS_IOWR(MD_IOC_COMMON_TYPE,  7, void*)
#define MD_IOC_CLOSE                    _VOS_IOWR(MD_IOC_COMMON_TYPE,  8, void*)
#define MD_IOC_OPENCFG                  _VOS_IOWR(MD_IOC_COMMON_TYPE,  9, void*)

#define MD_IOC_SET_PARAM                _VOS_IOWR(MD_IOC_COMMON_TYPE,  10, void*)
#define MD_IOC_GET_PARAM                _VOS_IOWR(MD_IOC_COMMON_TYPE,  11, void*)
#define MD_IOC_TRIGGER                  _VOS_IOWR(MD_IOC_COMMON_TYPE,  12, void*)
#define MD_IOC_GET_REG                  _VOS_IOWR(MD_IOC_COMMON_TYPE,  13, void*)
#define MD_IOC_SET_DMA_ABORT			_VOS_IOWR(MD_IOC_COMMON_TYPE,  14, void*)
#define MD_IOC_GET_DMA_ABORT			_VOS_IOWR(MD_IOC_COMMON_TYPE,  15, void*)

/* Add other command ID here*/
#if defined(__FREERTOS)
int nvt_md_ioctl(int fd, unsigned int uiCmd, void *p_arg);
#endif

#endif
