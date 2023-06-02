#ifndef __AI_IOCTL_CMD_H_
#define __AI_IOCTL_CMD_H_

#include "kwrap/ioctl.h"

#include "kdrv_ai.h"

#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int reg_addr;
	unsigned int reg_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int reg_cnt;
	REG_INFO reg_list[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

typedef struct ai_drv_map_mem {
	UINT32 user_pa;
	UINT32 user_va;
	UINT32 kerl_pa;
	UINT32 kerl_va;
} AI_DRV_MAP_MEM;

typedef struct ai_drv_map_meminfo {
	AI_DRV_MAP_MEM mem;
	UINT32 net_id;
} AI_DRV_MAP_MEMINFO;

typedef struct ai_drv_opencfg {
	KDRV_AI_OPENCFG opencfg;
	KDRV_AI_ENG engine;			//required whether using get/set
	UINT32 net_id;
} AI_DRV_OPENCFG;

typedef struct ai_drv_modeinfo {
	KDRV_AI_TRIG_MODE mode;
	KDRV_AI_ENG engine;			//required whether using get/set
	UINT32 net_id;
} AI_DRV_MODEINFO;

typedef struct ai_drv_appinfo {
	KDRV_AI_APP_INFO info;
	KDRV_AI_ENG engine;			//required whether using get/set
	UINT32 net_id;
} AI_DRV_APPINFO;

typedef struct ai_drv_llinfo {
	KDRV_AI_LL_INFO info;
	KDRV_AI_ENG engine;			//required whether using get/set
	UINT32 net_id;
} AI_DRV_LLINFO;

typedef struct ai_drv_triginfo {
	KDRV_AI_TRIGGER_PARAM trig_parm;
	KDRV_AI_ENG engine;			//required whether using get/set
	UINT32 net_id;
} AI_DRV_TRIGINFO;

typedef struct _AI_DRV_LL_USR_INFO {
	AI_USR_LAYER_INFO layer_info;
	UINT32 net_id;
} AI_DRV_LL_USR_INFO;

typedef struct _AI_DRV_LL_BUF_INFO {
	UINT32 size; //unit: byte
	UINT32 net_id;
} AI_DRV_LL_BUF_INFO;

//============================================================================
// IOCTL command
//============================================================================
#define AI_IOC_COMMON_TYPE 'M'
#define AI_IOC_START                    _VOS_IO(AI_IOC_COMMON_TYPE, 1)
#define AI_IOC_STOP                     _VOS_IO(AI_IOC_COMMON_TYPE, 2)

#define AI_IOC_READ_REG                 _VOS_IOWR(AI_IOC_COMMON_TYPE, 3, void*)
#define AI_IOC_WRITE_REG                _VOS_IOWR(AI_IOC_COMMON_TYPE, 4, void*)
#define AI_IOC_READ_REG_LIST            _VOS_IOWR(AI_IOC_COMMON_TYPE, 5, void*)
#define AI_IOC_WRITE_REG_LIST           _VOS_IOWR(AI_IOC_COMMON_TYPE, 6, void*)

#define AI_IOC_OPEN                     _VOS_IOWR(AI_IOC_COMMON_TYPE,  7, void*)
#define AI_IOC_CLOSE                    _VOS_IOWR(AI_IOC_COMMON_TYPE,  8, void*)
#define AI_IOC_OPENCFG                  _VOS_IOWR(AI_IOC_COMMON_TYPE,  9, void*)

#define AI_IOC_SET_MODE                 _VOS_IOWR(AI_IOC_COMMON_TYPE,  10, void*)
#define AI_IOC_SET_APP                  _VOS_IOWR(AI_IOC_COMMON_TYPE,  11, void*)
#define AI_IOC_SET_LL                   _VOS_IOWR(AI_IOC_COMMON_TYPE,  12, void*)

#define AI_IOC_GET_MODE                 _VOS_IOWR(AI_IOC_COMMON_TYPE,  13, void*)
#define AI_IOC_GET_APP                  _VOS_IOWR(AI_IOC_COMMON_TYPE,  14, void*)
#define AI_IOC_GET_LL                   _VOS_IOWR(AI_IOC_COMMON_TYPE,  15, void*)

#define AI_IOC_TRIGGER                  _VOS_IOWR(AI_IOC_COMMON_TYPE,  16, void*)
#define AI_IOC_WAITDONE                 _VOS_IOWR(AI_IOC_COMMON_TYPE,  17, void*)
#define AI_IOC_RESET                 	_VOS_IOWR(AI_IOC_COMMON_TYPE,  18, void*)

#define AI_IOC_SET_MAP_ADDR             _VOS_IOWR(AI_IOC_COMMON_TYPE,  19, void*)
#define AI_IOC_GET_MAP_ADDR             _VOS_IOWR(AI_IOC_COMMON_TYPE,  20, void*)

#define AI_IOC_ENG_INIT					_VOS_IOWR(AI_IOC_COMMON_TYPE,  21, void*)
#define AI_IOC_ENG_UNINIT				_VOS_IOWR(AI_IOC_COMMON_TYPE,  22, void*)
#if LL_SUPPORT_ROI
#define AI_IOC_SET_LL_USR_INFO          _VOS_IOWR(AI_IOC_COMMON_TYPE, 23, void*)
#define AI_IOC_UNINIT_LINK_INFO			_VOS_IOWR(AI_IOC_COMMON_TYPE, 24, void*)
#if LL_BUF_FROM_USR
#define AI_IOC_INIT_LL_BUF              _VOS_IOWR(AI_IOC_COMMON_TYPE, 25, void*)
#define AI_IOC_UNINIT_LL_BUF			_VOS_IOWR(AI_IOC_COMMON_TYPE, 26, void*)
#endif
#endif
#define NUE_IOC_RUN_FC					_VOS_IOWR(AI_IOC_COMMON_TYPE, 27, void*)
#define NUE_IOC_INIT					_VOS_IOWR(AI_IOC_COMMON_TYPE, 28, void*)
#define NUE_IOC_UNINIT					_VOS_IOWR(AI_IOC_COMMON_TYPE, 29, void*)


#define AI_IOC_CNN2_INIT				_VOS_IOWR(AI_IOC_COMMON_TYPE, 30, void*)
#define AI_IOC_CNN2_UNINIT				_VOS_IOWR(AI_IOC_COMMON_TYPE, 31, void*)
#define NUE2_IOC_RUN				    _VOS_IOWR(AI_IOC_COMMON_TYPE, 32, void*)
#define NUE2_IOC_DONE				    _VOS_IOWR(AI_IOC_COMMON_TYPE, 33, void*)
#define NUE2_IOC_INIT					_VOS_IOWR(AI_IOC_COMMON_TYPE, 34, void*)
#define NUE2_IOC_UNINIT					_VOS_IOWR(AI_IOC_COMMON_TYPE, 35, void*)
#define AI_IOC_DMA_ABORT				_VOS_IOWR(AI_IOC_COMMON_TYPE, 36, void*)
/* Add other command ID here*/


#if defined(__FREERTOS)
int nvt_ai_ioctl(int fd, unsigned int uiCmd, void *p_arg);
void nvt_ai_drv_init_rtos(void);
UINT8 * kdrv_ai_vmalloc(UINT32 v_size);
VOID kdrv_ai_vfree(UINT8 *v_buff);
#endif


#endif
