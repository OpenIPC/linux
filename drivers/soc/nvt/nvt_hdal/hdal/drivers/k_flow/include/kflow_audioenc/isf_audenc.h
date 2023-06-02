/*
    Copyright   Novatek Microelectronics Corp. 2005~2018.  All rights reserved.

    @file       isf_audenc.h

    @note       Nothing.

    @date       2018/08/28
*/

#ifndef IMAGEUNIT_AUDENC_H
#define IMAGEUNIT_AUDENC_H

//#ifdef __KERNEL__
#include "kwrap/semaphore.h"
//#include "nmediarec_api.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#define _SECTION(sec)
//#endif

typedef enum {
	AUDENC_PARAM_START                  = 0x0000F000,
	AUDENC_PARAM_CODEC                  = AUDENC_PARAM_START,
	AUDENC_PARAM_ENCODER_OBJ,
	AUDENC_PARAM_CHS,
	AUDENC_PARAM_SAMPLERATE,
	AUDENC_PARAM_BITS,
	AUDENC_PARAM_AAC_ADTS_HEADER,
	AUDENC_PARAM_FILETYPE,         // for CarDV only
	AUDENC_PARAM_RECFORMAT,        // IPCam should use MEDIAREC_LIVEVIEW as default
	AUDENC_PARAM_MAX_MEM_INFO,
	AUDENC_PARAM_FIXED_SAMPLE,
	AUDENC_PARAM_MAX_FRAME_QUEUE,  // streamsender audio slot number
	AUDENC_PARAM_BUFINFO_PHYADDR,
	AUDENC_PARAM_BUFINFO_SIZE,
	AUDENC_PARAM_ENCBUF_MS,
	AUDENC_PARAM_BS_RESERVED_SIZE,
	AUDENC_PARAM_AAC_VER,
	AUDENC_PARAM_MAX,
	ENUM_DUMMY4WORD(AUDENC_PARAM)
} AUDENC_PARAM;

typedef enum {
	AUDENC_PARAM_PORT_START             = 0x0000FF00,
	AUDENC_PARAM_PORT_OUTPUT_FMT        = AUDENC_PARAM_PORT_START,
	AUDENC_PARAM_PORT_MAX,
	ENUM_DUMMY4WORD(AUDENC_PARAM_PORT)
} AUDENC_PARAM_PORT;

typedef enum {
	AUDENC_ENCODER_NONE                 = 0,
	AUDENC_ENCODER_PCM                  = 1, //NMEDIAREC_ENC_PCM,
	AUDENC_ENCODER_AAC                  = 2, //NMEDIAREC_ENC_AAC,
	AUDENC_ENCODER_PPCM                 = 3, //NMEDIAREC_ENC_PPCM,
	AUDENC_ENCODER_ULAW                 = 4, //NMEDIAREC_ENC_ULAW,
	AUDENC_ENCODER_ALAW                 = 5, //NMEDIAREC_ENC_ALAW,
	ENUM_DUMMY4WORD(AUDENC_ENCODER)
} AUDENC_ENCODER;

typedef enum {
	AUDENC_OUTPUT_NONE                  = 0,
	AUDENC_OUTPUT_UNCOMPRESSION,
	AUDENC_OUTPUT_COMPRESSION,
	ENUM_DUMMY4WORD(AUDENC_OUTPUT)
} AUDENC_OUTPUT;

//#ifdef __KERNEL__
typedef void (IsfAudEncEventCb)(CHAR *Name, UINT32 event_id, UINT32 value);
typedef UINT32 (*AUDENC_PROC_CB)(UINT32 id, UINT32 state);

extern ISF_UNIT isf_audenc;

#define AUDENC_FUNC_NOWAIT			0x00000001 //do not wait for encode finish

//sxcmd
extern int _isf_audenc_cmd_isfae(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_isfae_showhelp(void);

extern int _isf_audenc_cmd_audenc(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_audenc_showhelp(void);

extern int _isf_audenc_cmd_debug(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_trace(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_probe(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_perf(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_save(char* sub_cmd_name, char *cmd_args);
extern int _isf_audenc_cmd_isfdbg_showhelp(void);

extern void isf_audenc_install_id(void) _SECTION(".kercfg_text");
extern void isf_audenc_uninstall_id(void);
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_AUDENC_SEM_ID[];
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_AUDENC_COMMON_SEM_ID;
extern SEM_HANDLE ISF_AUDENC_PROC_SEM_ID;
//#endif


#endif //IMAGEUNIT_AUDENC_680_H

