#ifndef _UVCP_H
#define _UVCP_H
#include "kwrap/type.h"


typedef enum {
	UVCP_CONFIG_ID_HEADER_LENGTH,           ///< Valid setting is 2 or 12.
	UVCP_CONFIG_ID_DATALEN_PER_UF,          ///< USB Data Length per micro-frame.
											///< Valid length 512/1024/2048/3072/(3K*N) ... / 48*1024
	UVCP_CONFIG_ID_FID,						///< Confifure Next FID value. The FID value would be auto toggled after EOF is set.
	UVCP_CONFIG_ID_PTS,						///< Presentation Time Stamp(PTS) 32bits value.
	UVCP_CONFIG_ID_REFERENCE_CLK,			///< UVC reference clock value in Mhz. Such as 12 or 30 Mhz.
	UVCP_CONFIG_ID_SOF_START_VAL,			///< UFRAME START VALUE. 11bits SOF value
	UVCP_CONFIG_ID_COUNTER_START_VAL,		///< Time Counter START VALUE. 32 bits value in reference clock cycle.
	UVCP_CONFIG_ID_BREAK_SIZE,				///< When output size exceed this break size. The interrupt would be issued.

	UVCP_CONFIG_ID_GETOUT_SIZE,
	UVCP_CONFIG_ID_GET_COUNTER_END,
	UVCP_CONFIG_ID_GET_SOF_END,
	UVCP_CONFIG_ID_GET_CNTSTEP,

	ENUM_DUMMY4WORD(UVCP_CONFIG_ID)
} UVCP_CONFIG_ID;


extern ER 		uvcp_open(void);
extern ER 		uvcp_close(void);
extern BOOL		uvcp_is_opened(void);
extern ER 		uvcp_trigger(BOOL eof, BOOL wait_break);
extern ER 		uvcp_wait_complete(void);
extern ER 		uvcp_wait_break(void);
extern ER 		uvcp_set_config(UVCP_CONFIG_ID configID, UINT32 configContext);
extern UINT32 	uvcp_get_config(UVCP_CONFIG_ID configID);
extern void 	uvcp_config_dma(UINT32 input_address, UINT32 input_size, UINT32 output_address, UINT32 *output_size);


#endif
