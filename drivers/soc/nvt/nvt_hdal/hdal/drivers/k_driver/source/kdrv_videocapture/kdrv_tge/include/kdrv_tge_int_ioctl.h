#ifndef _KDRV_TGE_INT_IOCTL_H_
#define _KDRV_TGE_INT_IOCTL_H_

#include <linux/ioctl.h>

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
#define TGE_IOC_COMMON_TYPE 'M'
#define TGE_IOC_START                   _IO(TGE_IOC_COMMON_TYPE, 1)
#define TGE_IOC_STOP                    _IO(TGE_IOC_COMMON_TYPE, 2)

#define TGE_IOC_READ_REG                _IOWR(TGE_IOC_COMMON_TYPE, 3, void*)
#define TGE_IOC_WRITE_REG               _IOWR(TGE_IOC_COMMON_TYPE, 4, void*)
#define TGE_IOC_READ_REG_LIST           _IOWR(TGE_IOC_COMMON_TYPE, 5, void*)
#define TGE_IOC_WRITE_REG_LIST          _IOWR(TGE_IOC_COMMON_TYPE, 6, void*)




/* Add other command ID here*/


#endif //_KDRV_TGE_INT_IOCTL_H_