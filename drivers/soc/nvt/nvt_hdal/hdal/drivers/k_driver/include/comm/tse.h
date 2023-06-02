/**
    Header file for TS MUXER/TS DEMUXER/HWCOPY Engine Integration module

    This file is the header file for TS MUXER/TS DEMUXER/HWCOPY Engine Integration module driver
    The user can reference this section for the detail description of the each driver API usage
    and also the parameter descriptions and its limitations.
    The overall combinational usage flow is introduced in the application note document,
    and the user must reference to the application note for the driver usage flow.

    @file       tse.h
    @ingroup    mIDrvMisc_TSE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

/**
    @addtogroup mIDrvMisc_TSE
*/
//@{

#ifndef _TSE_H
#define _TSE_H

#if defined __UITRON || defined __ECOS
#include "Type.h"
#else
#include "kwrap/type.h"
#endif
#include "comm/drv_lmt.h"

/**************** TSE_MODE_TSMUX ****************/
#define TSE_MUX_SRC_ADDR_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_MUX_SRC_BUF_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_MUX_SRC_BUF_LAST_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_BYTE

#define TSE_MUX_DST_ADDR_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_MUX_DST_BUF_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_WORD

/**************** TSE_MODE_TSDEMUX ****************/
#define TSE_DEMUX_SRC_ADDR_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_DEMUX_SRC_BUF_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_DEMUX_SRC_BUF_LAST_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_WORD

#define TSE_DEMUX_DST_ADDR_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_DEMUX_DST_BUF_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_DEMUX_DST_BUF_LAST_BLK_SIZE_ALIGN DRV_LIMIT_ALIGN_BYTE

/**************** TSE_MODE_HWCOPY ****************/
//HWCOPY_LINEAR_COPY
#define TSE_LINEAR_CPY_SRC_ADDR_ALIGN DRV_LIMIT_ALIGN_BYTE
#define TSE_LINEAR_CPY_SRC_SIZE_ALIGN	DRV_LIMIT_ALIGN_BYTE
#define TSE_LINEAR_CPY_DST_ADDR_ALIGN DRV_LIMIT_ALIGN_BYTE

//HWCOPY_LINEAR_SET
#define TSE_LINEAR_SET_DST_ADDR_ALIGN DRV_LIMIT_ALIGN_WORD
#define TSE_LINEAR_SET_DST_SIZE_ALIGN DRV_LIMIT_ALIGN_WORD

#define TSE_CFG_MASK 	0x10000000
#define TSE_MUX_MASK	0x20000000
#define TSE_DEMUX_MASK	0x40000000
#define TSE_HWCPY_MASK	0x80000000
/**
    TSE module general configuration Selection

    This is used in tse_setConfig()/tse_getConfig() to specify which of the function is selected to assign new configuration.
*/
typedef enum {
	TSE_CFG_ID_FREQ = TSE_CFG_MASK,                	///< TSE operating frequency (unit: MHz). Context can be:
													///< - @b 240 MHz
													///< - @b 320 MHz
													///< - @b 360 MHz
	TSE_CFG_ID_MAX,

	TSMUX_CFG_ID_PAYLOADSIZE = TSE_MUX_MASK,       	///< Configure the TS packet payload size.
													///< - @b 184 bytes
													///< - @b 176 bytes
													///< - @b 160 bytes
	TSMUX_CFG_ID_SRC_INFO,          				///< Muxing input buffer information. data type TSE_BUF_INFO
	TSMUX_CFG_ID_DST_INFO,          				///< Muxing output buffer information. data type TSE_BUF_INFO
													///< it would output warning messages as notifications.
	TSMUX_CFG_ID_MUXING_LEN,        				///< Get Last Mux output size.
	TSMUX_CFG_ID_SYNC_BYTE,         				///< TS Stream header's Field: Sync Byte.
	TSMUX_CFG_ID_CONTINUITY_CNT,    				///< TS Stream header's Field: continuity counter initial value. Valid value 0x0 ~ 0xF.
	TSMUX_CFG_ID_PID,               				///< TS Stream header's Field: 13-bits Packet ID. Valid value 0x0 ~ 0x1FFF.
	TSMUX_CFG_ID_TEI,               				///< TS Stream header's Field: Transport Error Indicator. Valid value 0 or 1.
	TSMUX_CFG_ID_TP,                				///< TS Stream header's Field: Transport Priority. Valid value 0 or 1.
	TSMUX_CFG_ID_SCRAMBLECTRL,      				///< TS Stream header's Field: Scrambling Control. Valid value 0~3. The value 0 means Not Scrambled.
	TSMUX_CFG_ID_START_INDICTOR,    				///< TS Stream header's Field: Payload Unit Start Indicator bit.
	TSMUX_CFG_ID_STUFF_VAL,         				///< Set Stuff values. Default value is 0x00.
	TSMUX_CFG_ID_ADAPT_FLAGS,       				///< Set 8bits adaptation field flags.
	TSMUX_CFG_ID_CON_CURR_CNT,      				///< Get continue current counter value.
	TSMUX_CFG_ID_LAST_DATA_MUX_MODE,      			///< Select muxing data into 1 or 2 TS packet, only valid at remaining data is equal to 183 bytes(528)
													///< - @b 0: 2ts packets
													///< - @b 1: 1ts packets
	TSMUX_CFG_ID_MAX,

	TSDEMUX_CFG_ID_SYNC_BYTE = TSE_DEMUX_MASK,      ///< Set SYNC Byte checking value. Default is 0x47.
	TSDEMUX_CFG_ID_ADAPTATION_FLAG,					///< Set Adaptation Flag checking value. Default is 0x00.
	TSDEMUX_CFG_ID_PID0_ENABLE,         			///< Enable/Disable of the filtering for TS packet's payload with PID0.
	TSDEMUX_CFG_ID_PID1_ENABLE,         			///< Enable/Disable of the filtering for TS packet's payload with PID1.
	TSDEMUX_CFG_ID_PID2_ENABLE,         			///< Enable/Disable of the filtering for TS packet's payload with PID2.
	TSDEMUX_CFG_ID_PID0_VALUE,          			///< 13bits PID-0 value.
	TSDEMUX_CFG_ID_PID1_VALUE,          			///< 13bits PID-1 value.
	TSDEMUX_CFG_ID_PID2_VALUE,          			///< 13bits PID-2 value.
	TSDEMUX_CFG_ID_CONTINUITY0_MODE,    			///<
	TSDEMUX_CFG_ID_CONTINUITY1_MODE,    			///<
	TSDEMUX_CFG_ID_CONTINUITY2_MODE,    			///<
	TSDEMUX_CFG_ID_CONTINUITY0_VALUE,   			///<
	TSDEMUX_CFG_ID_CONTINUITY1_VALUE,   			///<
	TSDEMUX_CFG_ID_CONTINUITY2_VALUE,   			///<
	TSDEMUX_CFG_ID_CONTINUITY0_LAST,    			///< Get the last continuity counter value after previous operation done.
	TSDEMUX_CFG_ID_CONTINUITY1_LAST,    			///< Get the last continuity counter value after previous operation done.
	TSDEMUX_CFG_ID_CONTINUITY2_LAST,    			///< Get the last continuity counter value after previous operation done.
	TSDEMUX_CFG_ID_IN_INFO,             			///< Set Input buffer info. data type TSE_BUF_INFO
	TSDEMUX_CFG_ID_OUT0_INFO,           			///< Set Out0 buffer info. data type TSE_BUF_INFO
	TSDEMUX_CFG_ID_OUT1_INFO,           			///< Set Out1 buffer info. data type TSE_BUF_INFO
	TSDEMUX_CFG_ID_OUT2_INFO,           			///< Set Out2 buffer info. data type TSE_BUF_INFO
	TSDEMUX_CFG_ID_OUT0_TOTAL_LEN,        			///< Get Last output size.
	TSDEMUX_CFG_ID_OUT1_TOTAL_LEN,        			///< Get Last output size.
	TSDEMUX_CFG_ID_OUT2_TOTAL_LEN,        			///< Get Last output size.
	TSDEMUX_CFG_ID_MAX,

	HWCOPY_CFG_ID_CMD = TSE_HWCPY_MASK,             ///< Set hwcopy command
	HWCOPY_CFG_ID_CTEX,                 			///< Set constant value
	HWCOPY_CFG_ID_SRC_ADDR,             			///< Set Input Start Address
	HWCOPY_CFG_ID_DST_ADDR,             			///< Set Output Start Address
	HWCOPY_CFG_ID_SRC_LEN,		           			///< Set Input Size
	HWCOPY_CFG_ID_TOTAL_LEN,           				///< Get Output Size
	HWCOPY_CFG_ID_MAX,

	ENUM_DUMMY4WORD(TSE_CFG_ID)
} TSE_CFG_ID;

/**
    HwCopy operation cmmand

    Select which HwCopy operation command for HwCopy engine operation.

*/
typedef enum {
	HWCOPY_LINEAR_SET,				//0x0        ///< linear set constant to addressed memory
	HWCOPY_LINEAR_COPY,         	//0x1        ///< linear copy to addressed memory
	ENUM_DUMMY4WORD(HWCOPY_CMD)
} HWCOPY_CMD;


typedef struct _TSE_BUF_INFO {
	UINT32 addr;
	UINT32 size;
#ifdef _NVT_EMULATION_
	UINT32 padding;								///< only for emulation
#endif
	struct _TSE_BUF_INFO *pnext;                   ///< Link to next property description. (Fill NULL if this is last item)
} TSE_BUF_INFO;

/*
    TSE_MODE
*/
typedef enum {
	TSE_MODE_TSMUX,
	TSE_MODE_TSDEMUX,
	TSE_MODE_HWCOPY,
	TSE_MODE_MAX_NUM,
	ENUM_DUMMY4WORD(TSE_MODE_NUM)
} TSE_MODE_NUM;

typedef enum {
	INT_STS_COMPLETE = (1 << 0),
	INT_STS_INPUT_END = (1 << 1),

	INT_STS_OUT0_FULL = (1 << 4),
	INT_STS_OUT1_FULL = (1 << 5),
	INT_STS_OUT2_FULL = (1 << 6),

	INT_STS_CONTY0_ERR = (1 << 8),
	INT_STS_TEI0 = (1 << 9),
	INT_STS_SYNC0_ERR = (1 << 10),
	INT_STS_ADAPFLG0_ERR = (1 << 11),

	INT_STS_CONTY1_ERR = (1 << 16),
	INT_STS_TEI1 = (1 << 17),
	INT_STS_SYNC1_ERR = (1 << 18),
	INT_STS_ADAPFLG1_ERR = (1 << 19),

	INT_STS_CONTY2_ERR = (1 << 24),
	INT_STS_TEI2 = (1 << 25),
	INT_STS_SYNC2_ERR = (1 << 26),
	INT_STS_ADAPFLG2_ERR = (1 << 27),

	ENUM_DUMMY4WORD(TSE_INT_STS)
} TSE_INT_STS;

#define TSE_INT_ERR0_STS (INT_STS_CONTY0_ERR | INT_STS_TEI0 | INT_STS_SYNC0_ERR | INT_STS_ADAPFLG0_ERR)
#define TSE_INT_ERR1_STS (INT_STS_CONTY1_ERR | INT_STS_TEI1 | INT_STS_SYNC1_ERR | INT_STS_ADAPFLG1_ERR)
#define TSE_INT_ERR2_STS (INT_STS_CONTY2_ERR | INT_STS_TEI2 | INT_STS_SYNC2_ERR | INT_STS_ADAPFLG2_ERR)

//
//  Extern APIs
//
extern ER tse_open(void);
extern ER tse_close(void);
extern BOOL tse_isOpened(void);

extern ER tse_start(UINT32 bWait, TSE_MODE_NUM OP_MODE);
extern ER tse_waitDone(void);
extern ER tse_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue);
extern UINT32 tse_getConfig(TSE_CFG_ID CfgID);
extern UINT32 tse_getIntStatus(void);
extern UINT32 tse_init(void);
extern UINT32 tse_uninit(void);
//@}
#endif
