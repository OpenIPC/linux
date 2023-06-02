#ifndef __IVE_IOCTL_CMD_H_
#define __IVE_IOCTL_CMD_H_

#include "kwrap/ioctl.h"


#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define NVTMPP_OPEN(...) 0
#define NVTMPP_IOCTL nvtmpp_ioctl
#define NVTMPP_CLOSE(...)

#define NVTIVE_OPEN(...) 0
#define NVTIVE_IOCTL nvt_ive_ioctl
#define NVTIVE_CLOSE(...)
#endif

#if defined(__LINUX)
#define NVTMPP_OPEN  open
#define NVTMPP_IOCTL ioctl
#define NVTMPP_CLOSE close

#define NVTIVE_OPEN  open
#define NVTIVE_IOCTL ioctl
#define NVTIVE_CLOSE close
#endif

#if defined(__FREERTOS)
#else
#define MODULE_REG_LIST_NUM	 11

typedef struct reg_info {
	unsigned int uiAddr;
	unsigned int uiValue;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int uiCount;
	REG_INFO RegList[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;
#endif

//============================================================================
// IOCTL command
//============================================================================
#define IVE_IOC_COMMON_TYPE 'M'
#define IVE_IOC_START					_VOS_IO(IVE_IOC_COMMON_TYPE, 1)
#define IVE_IOC_STOP					_VOS_IO(IVE_IOC_COMMON_TYPE, 2)

#define IVE_IOC_READ_REG				_VOS_IOWR(IVE_IOC_COMMON_TYPE, 3, void*)
#define IVE_IOC_WRITE_REG				_VOS_IOWR(IVE_IOC_COMMON_TYPE, 4, void*)
#define IVE_IOC_READ_REG_LIST			_VOS_IOWR(IVE_IOC_COMMON_TYPE, 5, void*)
#define IVE_IOC_WRITE_REG_LIST			_VOS_IOWR(IVE_IOC_COMMON_TYPE, 6, void*)

#define IVE_IOC_OPEN                      _VOS_IOWR(IVE_IOC_COMMON_TYPE,  7, void*)
#define IVE_IOC_CLOSE                     _VOS_IOWR(IVE_IOC_COMMON_TYPE,  8, void*)
#define IVE_IOC_OPENCFG					  _VOS_IOWR(IVE_IOC_COMMON_TYPE,  9, void*)
#define IVE_IOC_SET_IMG_INFO              _VOS_IOWR(IVE_IOC_COMMON_TYPE, 10, void*)
#define IVE_IOC_GET_IMG_INFO              _VOS_IOWR(IVE_IOC_COMMON_TYPE, 11, void*)
#define IVE_IOC_SET_IMG_DMA_IN            _VOS_IOWR(IVE_IOC_COMMON_TYPE, 12, void*)
#define IVE_IOC_GET_IMG_DMA_IN            _VOS_IOWR(IVE_IOC_COMMON_TYPE, 13, void*)
#define IVE_IOC_SET_IMG_DMA_OUT           _VOS_IOWR(IVE_IOC_COMMON_TYPE, 14, void*)
#define IVE_IOC_GET_IMG_DMA_OUT           _VOS_IOWR(IVE_IOC_COMMON_TYPE, 15, void*)
#define IVE_IOC_SET_GENERAL_FILTER        _VOS_IOWR(IVE_IOC_COMMON_TYPE, 16, void*)
#define IVE_IOC_GET_GENERAL_FILTER        _VOS_IOWR(IVE_IOC_COMMON_TYPE, 17, void*)
#define IVE_IOC_SET_MEDIAN_FILTER         _VOS_IOWR(IVE_IOC_COMMON_TYPE, 18, void*)
#define IVE_IOC_GET_MEDIAN_FILTER         _VOS_IOWR(IVE_IOC_COMMON_TYPE, 19, void*)
#define IVE_IOC_SET_EDGE_FILTER           _VOS_IOWR(IVE_IOC_COMMON_TYPE, 20, void*)
#define IVE_IOC_GET_EDGE_FILTER           _VOS_IOWR(IVE_IOC_COMMON_TYPE, 21, void*)
#define IVE_IOC_SET_NON_MAX_SUP           _VOS_IOWR(IVE_IOC_COMMON_TYPE, 22, void*)
#define IVE_IOC_GET_NON_MAX_SUP           _VOS_IOWR(IVE_IOC_COMMON_TYPE, 23, void*)
#define IVE_IOC_SET_THRES_LUT             _VOS_IOWR(IVE_IOC_COMMON_TYPE, 24, void*)
#define IVE_IOC_GET_THRES_LUT             _VOS_IOWR(IVE_IOC_COMMON_TYPE, 25, void*)
#define IVE_IOC_SET_MORPH_FILTER          _VOS_IOWR(IVE_IOC_COMMON_TYPE, 26, void*)
#define IVE_IOC_GET_MORPH_FILTER          _VOS_IOWR(IVE_IOC_COMMON_TYPE, 27, void*)
#define IVE_IOC_SET_INTEGRAL_IMG          _VOS_IOWR(IVE_IOC_COMMON_TYPE, 28, void*)
#define IVE_IOC_GET_INTEGRAL_IMG          _VOS_IOWR(IVE_IOC_COMMON_TYPE, 29, void*)
#define IVE_IOC_SET_ITER_REGION_VOTE      _VOS_IOWR(IVE_IOC_COMMON_TYPE, 30, void*)
#define IVE_IOC_GET_ITER_REGION_VOTE      _VOS_IOWR(IVE_IOC_COMMON_TYPE, 31, void*)
#define IVE_IOC_TRIGGER                   _VOS_IOWR(IVE_IOC_COMMON_TYPE, 32, void*)
#define IVE_IOC_SET_OUTSEL                _VOS_IOWR(IVE_IOC_COMMON_TYPE, 33, void*)
#define IVE_IOC_GET_OUTSEL                _VOS_IOWR(IVE_IOC_COMMON_TYPE, 34, void*)
#define IVE_IOC_SET_IRV                   _VOS_IOWR(IVE_IOC_COMMON_TYPE, 35, void*)
#define IVE_IOC_GET_IRV                   _VOS_IOWR(IVE_IOC_COMMON_TYPE, 36, void*)
#define IVE_IOC_SET_DMA_ABORT			  _VOS_IOWR(IVE_IOC_COMMON_TYPE, 37, void*)
#define IVE_IOC_SET_FLOWCT                _VOS_IOWR(IVE_IOC_COMMON_TYPE, 38, void*)
#define IVE_IOC_GET_FLOWCT                _VOS_IOWR(IVE_IOC_COMMON_TYPE, 39, void*)
#define IVE_IOC_GET_VERSION               _VOS_IOWR(IVE_IOC_COMMON_TYPE, 40, void*)
#define IVE_IOC_TRIGGER_NONBLOCK          _VOS_IOWR(IVE_IOC_COMMON_TYPE, 41, void*)
#define IVE_IOC_WAITDONE_NONBLOCK         _VOS_IOWR(IVE_IOC_COMMON_TYPE, 42, void*)


/* Add other command ID here*/
#if defined(__FREERTOS)
int nvt_ive_ioctl(int fd, unsigned int uiCmd, void *p_arg);
void nvt_ive_drv_init_rtos(void);
#endif


#endif
