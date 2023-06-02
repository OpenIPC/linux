#ifndef __DIS_IOCTL_CMD_H_
#define __DIS_IOCTL_CMD_H_

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
#define DIS_IOC_COMMON_TYPE 'M'
#define DIS_IOC_START                   _VOS_IO(DIS_IOC_COMMON_TYPE, 1)
#define DIS_IOC_STOP                    _VOS_IO(DIS_IOC_COMMON_TYPE, 2)

#define DIS_IOC_READ_REG                _VOS_IOWR(DIS_IOC_COMMON_TYPE, 3, void*)
#define DIS_IOC_WRITE_REG               _VOS_IOWR(DIS_IOC_COMMON_TYPE, 4, void*)
#define DIS_IOC_READ_REG_LIST           _VOS_IOWR(DIS_IOC_COMMON_TYPE, 5, void*)
#define DIS_IOC_WRITE_REG_LIST          _VOS_IOWR(DIS_IOC_COMMON_TYPE, 6, void*)

#define DIS_IOC_OPEN                      _VOS_IOWR(DIS_IOC_COMMON_TYPE,  7, void*)
#define DIS_IOC_CLOSE                     _VOS_IOWR(DIS_IOC_COMMON_TYPE,  8, void*)
#define DIS_IOC_OPENCFG					  _VOS_IOWR(DIS_IOC_COMMON_TYPE,  9, void*)
#define DIS_IOC_SET_IMG_INFO              _VOS_IOWR(DIS_IOC_COMMON_TYPE, 10, void*)
#define DIS_IOC_GET_IMG_INFO              _VOS_IOWR(DIS_IOC_COMMON_TYPE, 11, void*)
#define DIS_IOC_SET_IMG_DMA_IN            _VOS_IOWR(DIS_IOC_COMMON_TYPE, 12, void*)
#define DIS_IOC_GET_IMG_DMA_IN            _VOS_IOWR(DIS_IOC_COMMON_TYPE, 13, void*)
#define DIS_IOC_SET_INT_EN                _VOS_IOWR(DIS_IOC_COMMON_TYPE, 14, void*)
#define DIS_IOC_GET_INT_EN                _VOS_IOWR(DIS_IOC_COMMON_TYPE, 15, void*)
#define DIS_IOC_SET_IMG_DMA_OUT           _VOS_IOWR(DIS_IOC_COMMON_TYPE, 16, void*)
#define DIS_IOC_GET_IMG_DMA_OUT           _VOS_IOWR(DIS_IOC_COMMON_TYPE, 17, void*)
#define DIS_IOC_GET_MV_MAP_OUT            _VOS_IOWR(DIS_IOC_COMMON_TYPE, 18, void*)
#define DIS_IOC_GET_MV_MDS_DIM            _VOS_IOWR(DIS_IOC_COMMON_TYPE, 19, void*)
#define DIS_IOC_SET_MV_BLOCKS_DIM         _VOS_IOWR(DIS_IOC_COMMON_TYPE, 20, void*)
#define DIS_IOC_GET_MV_BLOCKS_DIM         _VOS_IOWR(DIS_IOC_COMMON_TYPE, 21, void*)
#define ETH_IOC_SET_ETH_PARAM_IN          _VOS_IOWR(DIS_IOC_COMMON_TYPE, 22, void*)
#define ETH_IOC_GET_ETH_PARAM_IN          _VOS_IOWR(DIS_IOC_COMMON_TYPE, 23, void*)
#define ETH_IOC_SET_ETH_BUFFER            _VOS_IOWR(DIS_IOC_COMMON_TYPE, 24, void*)
#define ETH_IOC_GET_ETH_BUFFER            _VOS_IOWR(DIS_IOC_COMMON_TYPE, 25, void*)
#define ETH_IOC_GET_ETH_PARAM_OUT         _VOS_IOWR(DIS_IOC_COMMON_TYPE, 26, void*)

#define DIS_IOC_TRIGGER                   _VOS_IOWR(DIS_IOC_COMMON_TYPE, 27, void*)
#define DIS_IOC_SET_DMA_ABORT			  _VOS_IOWR(DIS_IOC_COMMON_TYPE, 28, void*)
#define DIS_IOC_GET_DMA_ABORT			  _VOS_IOWR(DIS_IOC_COMMON_TYPE, 29, void*)


/* Add other command ID here*/
#if defined(__FREERTOS)
int nvt_dis_ioctl(int fd, unsigned int uiCmd, void *p_arg);
#endif


#endif
