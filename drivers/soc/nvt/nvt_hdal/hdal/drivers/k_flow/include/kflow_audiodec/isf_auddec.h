/*
    Copyright   Novatek Microelectronics Corp. 2005~2018.  All rights reserved.

    @file       isf_auddec.h

    @note       Nothing.

    @date       2018/09/25
*/

#ifndef IMAGEUNIT_AUDDEC_H
#define IMAGEUNIT_AUDDEC_H

//#ifdef __KERNEL__
#include "kwrap/semaphore.h"
//#include "nmediaplay_api.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#define _SECTION(sec)
//#endif

typedef enum {
	AUDDEC_PARAM_START					= 0x8000A00,                     ///< a non-zero arbitrary value
	AUDDEC_PARAM_CODEC				    = AUDDEC_PARAM_START,
	AUDDEC_PARAM_SAMPLERATE,
	AUDDEC_PARAM_CHANNELS,
	AUDDEC_PARAM_BITS,
	AUDDEC_PARAM_MAX_MEM_INFO,
	AUDDEC_PARAM_PREDECNUM,
	AUDDEC_PARAM_FILEPLAY_MODE,
	AUDDEC_PARAM_BUFINFO_PHYADDR,
	AUDDEC_PARAM_BUFINFO_SIZE,
	AUDDEC_PARAM_ADTS_EN,
	AUDDEC_PARAM_MAX,
	ENUM_DUMMY4WORD(AUDDEC_PARAM)
} AUDDEC_PARAM;

typedef enum {
	AUDDEC_DECODER_NONE					= 0,
	AUDDEC_DECODER_PCM					= 1, //NMEDIAPLAY_DEC_PCM,
	AUDDEC_DECODER_AAC                  = 2, //NMEDIAPLAY_DEC_AAC,
	AUDDEC_DECODER_PPCM					= 3, //NMEDIAPLAY_DEC_PPCM,
	AUDDEC_DECODER_ULAW					= 4, //NMEDIAPLAY_DEC_ULAW,
	AUDDEC_DECODER_ALAW					= 5, //NMEDIAPLAY_DEC_ALAW,
	ENUM_DUMMY4WORD(AUDDEC_DECODER)
} AUDDEC_DECODER;

typedef enum {
	AUDDEC_OUTPUT_NONE                  = 0,
	AUDDEC_OUTPUT_UNEXTRACT,
	AUDDEC_OUTPUT_EXTRACT,
	ENUM_DUMMY4WORD(AUDDEC_OUTPUT)
} AUDDEC_OUTPUT;

//#ifdef __KERNEL__
typedef void (IsfAudDecEventCb)(CHAR *Name, UINT32 event_id, UINT32 value);
typedef UINT32 (*AUDDEC_PROC_CB)(UINT32 id, UINT32 state);

extern ISF_UNIT isf_auddec;

#define AUDDEC_FUNC_NOWAIT			0x00000001 //do not wait for decode finish

//sxcmd
extern int _isf_auddec_cmd_isfad(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_isfad_showhelp(void);

extern int _isf_auddec_cmd_auddec(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_auddec_showhelp(void);

extern int _isf_auddec_cmd_debug(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_trace(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_probe(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_perf(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_save(char* sub_cmd_name, char *cmd_args);
extern int _isf_auddec_cmd_isfdbg_showhelp(void);

extern void isf_auddec_install_id(void) _SECTION(".kercfg_text");
extern void isf_auddec_uninstall_id(void);
//extern SEM_HANDLE _SECTION(".kercfg_data") ISF_AUDDEC_SEM_ID[];
//extern SEM_HANDLE _SECTION(".kercfg_data") ISF_AUDDEC_COMMON_SEM_ID;
extern SEM_HANDLE ISF_AUDDEC_PROC_SEM_ID;
//#endif

extern void ISF_AudDec_InstallID(void) _SECTION(".kercfg_text");

#endif //IMAGEUNIT_AUDDEC_H

