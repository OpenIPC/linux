/**
	@brief Header file of internal.

	@file hd_int.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef	_HD_INT_H_
#define	_HD_INT_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

#if defined(__LINUX)
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <string.h>

#include <fcntl.h>

#include <kwrap/platform.h>

#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define ISF_OPEN     isf_flow_open
#define ISF_IOCTL    isf_flow_ioctl
#define ISF_CLOSE    isf_flow_close
#endif
#if defined(__LINUX)
#define ISF_OPEN     open
#define ISF_IOCTL    ioctl
#define ISF_CLOSE    close
#endif


/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define HD_VIDEOPROC_PIPE_VPE    		0x000000F2 //do YUV-domain scale process (VPE)

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

#ifndef HD_MODULE_NAME
#error Unknown HD_MODULE_NAME
#error Please assign your "HD_MODULE_NAME" before include "hd_int.h"
#endif

#define DBG_ERR(fmtstr, args...) 		HD_LOG_BIND(HD_MODULE_NAME, _ERR)("\033[1;31m" fmtstr "\033[0m", ##args)
#define DBG_WRN(fmtstr, args...) 		HD_LOG_BIND(HD_MODULE_NAME, _WRN)("\033[1;33m" fmtstr "\033[0m", ##args)
#define DBG_IND(fmtstr, args...) 		HD_LOG_BIND(HD_MODULE_NAME, _IND)(fmtstr, ##args)
#define DBG_DUMP(fmtstr, args...) 		HD_LOG_BIND(HD_MODULE_NAME, _MSG)(fmtstr, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...)	HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("BEGIN: " fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) 	HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("END: " fmtstr, ##args)

#define CHKPNT      					printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)						printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x) 						printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#define DBGCALLER() 					printf("%s() is called @0x%08X\r\n", __func__, __builtin_return_address(0)-8);


/********************************************************************
	TYPE DEFINITION
********************************************************************/

//mask of hd_common_init(sys_config_type)
#define HD_COMMON_CFG_CPU		0x0000000f  //CPU
#define HD_COMMON_CFG_DRAM		0x000000f0  //DRAM
#define HD_COMMON_CFG_MEM		0x00000f00  //MEM
#define HD_COMMON_CFG_DEBUG	0x0000f000  //DEBUG
#define HD_COMMON_CFG_R1		0x000f0000  //<reserved>
#define HD_COMMON_CFG_R2		0x00f00000  //<reserved>
#define HD_COMMON_CFG_R3		0x0f000000  //<reserved>
#define HD_COMMON_CFG_R4		0xf0000000  //<reserved>

//mask of hd_common_sysconfig(sys_config_type1, ...)  => these config should be config before call hd_xxxx_init()
#define HD_VIDEOCAP_CFG 		0x0000000f  //vcap
#define HD_VIDEOOUT_CFG 		0x000000f0  //vout
#define HD_VIDEOENC_CFG 		0x00000f00  //venc
#define HD_VIDEODEC_CFG 		0x0000f000  //vdec
#define HD_VIDEOPROC_CFG 		0x000f0000  //vprc
#define HD_GFX_CFG 			0x00f00000  //gfx
#define HD_OSG_CFG 			0x0f000000  //osg
#define VENDOR_R1_CFG 		0xf0000000  //<reserved>

#define HD_VIDEOPROC_CFG_STRIP_MASK		0x00000007  //vprc stripe rule mask: (default 0)
#define HD_VIDEOPROC_CFG_STRIP_LV1		0x00000000  //vprc "0: cut w>1280, GDC =  on, 2D_LUT off after cut (LL slow)
#define HD_VIDEOPROC_CFG_STRIP_LV2		0x00010000  //vprc "1: cut w>2048, GDC = off, 2D_LUT off after cut (LL fast)
#define HD_VIDEOPROC_CFG_STRIP_LV3		0x00020000  //vprc "2: cut w>2688, GDC = off, 2D_LUT off after cut (LL middle)(2D_LUT best)
#define HD_VIDEOPROC_CFG_STRIP_LV4		0x00030000  //vprc "3: cut w> 720, GDC =  on, 2D_LUT off after cut (LL not allow)(GDC best)
#define HD_VIDEOPROC_CFG_DISABLE_GDC	HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_LL_FAST		HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_2DLUT_BEST		HD_VIDEOPROC_CFG_STRIP_LV3
#define HD_VIDEOPROC_CFG_GDC_BEST		HD_VIDEOPROC_CFG_STRIP_LV4

//mask of hd_common_sysconfig(..., sys_config_type2)  => these config should be config before call hd_xxxx_init()
#define HD_AUDIOCAP_CFG 		0x0000000f  //acap
#define HD_AUDIOOUT_CFG 		0x000000f0  //aout
#define HD_AUDIOENC_CFG 		0x00000f00  //aenc
#define HD_AUDIODEC_CFG 		0x0000f000  //adec
#define VENDOR_AI_CFG 		0x000f0000  //ai
#define VENDOR_CV_CFG 		0x00f00000  //cv
#define VENDOR_DSP_CFG 		0x0f000000  //dsp
#define VENDOR_ISP_CFG 		0xf0000000  //isp

#define VENDOR_AI_CFG_ENABLE_CNN	0x00010000  //ai "1:enable CNN" (default 0)

extern HD_RESULT _hd_common_get_init(UINT32 *p_sys_config_type);
extern int _hd_common_get_pid(void);

extern int dev_fd;
extern int _hd_dump_printf(const char *fmtstr, ...);
extern int _hd_videocap_dump_info(UINT32 did);
extern int _hd_videoproc_dump_info(UINT32 did);
extern int _hd_videoenc_dump_info(void);
extern int _hd_videodec_dump_info(void);
extern int _hd_videoout_dump_info(UINT32 did);
extern int _hd_audiocap_dump_info(UINT32 did);
extern int _hd_audioout_dump_info(UINT32 did);
extern int _hd_audioenc_dump_info(void);
extern int _hd_audiodec_dump_info(void);

extern HD_RESULT _hd_videocap_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_videoproc_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_videoout_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_videoenc_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_videodec_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_audiocap_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_audioout_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_audioenc_convert_dev_id(HD_DAL* p_dev_id);
extern HD_RESULT _hd_audiodec_convert_dev_id(HD_DAL* p_dev_id);

extern HD_RESULT _hd_videocap_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_videoproc_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_videoout_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_videoenc_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_videodec_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_audiocap_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_audioout_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_audioenc_convert_in_id(HD_IO* p_in_id);
extern HD_RESULT _hd_audiodec_convert_in_id(HD_IO* p_in_id);

extern int _hd_videocap_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len);
extern int _hd_videoproc_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len);
extern int _hd_videoproc_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len);
extern int _hd_videoenc_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len);
extern int _hd_videoenc_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len);
extern int _hd_videoout_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len);
extern int _hd_audiocap_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len);
extern int _hd_audioout_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len);

extern int _hd_dest_in_id_str(HD_DAL dev_id, HD_IO in_id, CHAR *p_str, INT str_len);
extern int _hd_src_out_id_str(HD_DAL dev_id, HD_IO out_id, CHAR *p_str, INT str_len);
extern int _hd_video_pipe_str(UINT32 pipe, CHAR *p_str, INT str_len);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
extern int _hd_video_dir_str(HD_VIDEO_DIR dir, CHAR *p_str, INT str_len);

extern void _hd_videocap_cfg_active(unsigned int *p_active_list);
extern void _hd_videoproc_cfg_max(UINT32 maxdevice);
extern void _hd_videoenc_cfg_max(UINT32 maxpath);
extern void _hd_videodec_cfg_max(UINT32 maxpath);
extern void _hd_osg_cfg_max(unsigned int max_prc_path,
	                        unsigned int max_enc_path,
					        unsigned int max_out_path,
					        unsigned int max_stamp_img,
					        unsigned int *max_prc_stamp,
					        unsigned int *max_prc_mask,
					        unsigned int *max_enc_stamp,
					        unsigned int *max_enc_mask,
					        unsigned int *max_out_stamp,
					        unsigned int *max_out_mask);
extern void _hd_videoout_cfg_max(UINT32 maxdevice);
extern void _hd_audiocap_cfg_max(UINT32 maxdevice);
extern void _hd_audioout_cfg_max(UINT32 maxdevice);
extern void _hd_audioenc_cfg_max(UINT32 maxpath);
extern void _hd_audiodec_cfg_max(UINT32 maxpath);

extern int _hd_videocap_is_init(VOID);
extern int _hd_videoproc_is_init(VOID);
extern int _hd_videoout_is_init(VOID);
extern int _hd_videoenc_is_init(VOID);
extern int _hd_videodec_is_init(VOID);
extern int _hd_audiocap_is_init(VOID);
extern int _hd_audioout_is_init(VOID);
extern int _hd_audioenc_is_init(VOID);
extern int _hd_audiodec_is_init(VOID);
extern int _hd_osg_is_init(VOID);

extern HD_RESULT _hd_osg_reset(void);
extern HD_RESULT _hd_osg_attach(UINT32 unit_id, UINT32 port_id, UINT32 osg_id);
extern HD_RESULT _hd_osg_detach(UINT32 unit_id, UINT32 port_id, UINT32 osg_id);
extern HD_RESULT _hd_osg_get(UINT32 unit_id, UINT32 port_id, UINT32 osg_id, UINT32 param_id, VOID* p_param);
extern HD_RESULT _hd_osg_set(UINT32 unit_id, UINT32 port_id, UINT32 osg_id, UINT32 param_id, VOID* p_param);
extern HD_RESULT _hd_osg_enable(UINT32 unit_id, UINT32 port_id, UINT32 osg_id);
extern HD_RESULT _hd_osg_disable(UINT32 unit_id, UINT32 port_id, UINT32 osg_id);


#endif

