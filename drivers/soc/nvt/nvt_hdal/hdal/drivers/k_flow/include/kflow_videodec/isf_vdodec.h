/*
    Copyright   Novatek Microelectronics Corp. 2005~2017.  All rights reserved.

    @file       ImageUnit_VdoDec.h
    @ingroup    mVdoDec

    @note       Nothing.

    @date       2017/07/11
*/

#ifndef IMAGEUNIT_VDODEC_H
#define IMAGEUNIT_VDODEC_H

#include "kwrap/semaphore.h"
#include "nmediaplay_api.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#define _SECTION(sec)

#if defined(_BSP_NA51000_)
#define VDODEC_MAX_PATH_NUM	16
#endif
#if defined(_BSP_NA51023_)
#define VDODEC_MAX_PATH_NUM	8
#endif
#if defined(_BSP_NA51055_)
#define VDODEC_MAX_PATH_NUM	8
#endif
#if defined(_BSP_NA51089_)
#define VDODEC_MAX_PATH_NUM     8
#endif

typedef enum {
	VDODEC_PARAM_START					= 0x8000A00,                     ///< a non-zero arbitrary value
	VDODEC_PARAM_CODEC                  = VDODEC_PARAM_START,
	VDODEC_PARAM_DESC,
	VDODEC_PARAM_WIDTH,
	VDODEC_PARAM_HEIGHT,
	VDODEC_PARAM_GOP,
	VDODEC_PARAM_MAX_MEM_INFO,
	VDODEC_PARAM_PREDECNUM,
	VDODEC_PARAM_PORT_OUTPUT_FMT,
	VDODEC_PARAM_EVENT_CB,
	VDODEC_PARAM_REFRESH_VDO,
	VDODEC_PARAM_BUFINFO_PHYADDR,
	VDODEC_PARAM_BUFINFO_SIZE,
	VDODEC_PARAM_USER_OUT_BUF,
	VDODEC_PARAM_VUI_INFO,
	VDODEC_PARAM_JPEGINFO,
	VDODEC_PARAM_DEC_STATUS,
	VDODEC_PARAM_YUV_AUTO_DROP,
	VDODEC_PARAM_RAWQUE_MAX_NUM,
	VDODEC_PARAM_MAX,
	ENUM_DUMMY4WORD(VDODEC_PARAM)
} VDODEC_PARAM;

/*typedef enum {
	VDODEC_DECODER_NONE					= 0,
	VDODEC_DECODER_MJPG					= NMEDIAPLAY_DEC_MJPG,
	VDODEC_DECODER_H264					= NMEDIAPLAY_DEC_H264,
	VDODEC_DECODER_H265					= NMEDIAPLAY_DEC_H265,
	VDODEC_DECODER_YUV					= NMEDIAPLAY_DEC_YUV,
	ENUM_DUMMY4WORD(VDODEC_DECODER)
} VDODEC_DECODER;*/

typedef enum {
	VDODEC_OUTPUT_NONE                  = 0,
	VDODEC_OUTPUT_UNCOMPRESSION,
	VDODEC_OUTPUT_COMPRESSION,
	ENUM_DUMMY4WORD(VDODEC_OUTPUT)
} VDODEC_OUTPUT;

typedef enum {
	ISF_VDODEC_EVENT_ONE_DISPLAYFRAME   = 0,
	ISF_VDODEC_EVENT_CUR_VDOBS,              ///< current used video bs addr & size
	ISF_VDODEC_EVENT_MAX,
	ENUM_DUMMY4WORD(ISF_VDODEC_CB_EVENT)
} ISF_VDODEC_CB_EVENT;


typedef void (IsfVdoDecEventCb)(CHAR *Name, UINT32 event_id, UINT32 value);
typedef UINT32 (*VDODEC_PROC_CB)(UINT32 id, UINT32 state);

extern ISF_UNIT isf_vdodec;

//extern UINT32 g_vdodec_path_max_count;    // dynamic setting for actual used path count, using PATH_MAX_COUNT to access this variable
//#define PATH_MAX_COUNT g_vdodec_path_max_count

#define VDODEC_FUNC_NOWAIT			0x00000001 //do not wait for decode finish

//sxcmd
extern int _isf_vdodec_cmd_isfvd(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_isfvd_showhelp(void);

extern int _isf_vdodec_cmd_vdodec(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_vdodec_showhelp(void);

extern int _isf_vdodec_cmd_debug(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_trace(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_probe(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_perf(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_save(char* sub_cmd_name, char *cmd_args);
extern int _isf_vdodec_cmd_isfdbg_showhelp(void);

extern void isf_vdodec_install_id(void) _SECTION(".kercfg_text");
extern void isf_vdodec_uninstall_id(void);
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDODEC_SEM_ID[];
extern SEM_HANDLE ISF_VDODEC_PROC_SEM_ID;

#endif //IMAGEUNIT_VDODEC_H

