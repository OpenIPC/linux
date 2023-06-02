/**
    Header file of SDP module driver

    This file is the header file of SDP module

    @file       SDP.h
    @ingroup    mIDrvIO_SDP
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _SDP_H
#define _SDP_H
#include <kwrap/nvt_type.h>
//#include "kwrap/type.h"
//#include "kdrv_type.h"
//#include "../comm/driver.h"

#ifndef DRV_CB
typedef void (*DRV_CB)(UINT32 event);
#endif


/**
    @addtogroup mIDrvIO_SDP
*/
//@{

/**
    SDP ID
*/
typedef enum {
	SDP_ID_1,                   ///< SDP 1

	SDP_ID_COUNT,               ///< SDP ID count

	ENUM_DUMMY4WORD(SDP_ID)
} SDP_ID;

/* enum _SDP_EVENTS: enum for SDP events
 *
 */
typedef enum _SDP_EVENTS {
	SDP_EVENTS_FIFO_LOADED = 0x01,      ///< SDP DMA data loaded to FIFO
	SDP_EVENTS_FIFO_UNDERRUN = 0x02,    ///< SDP DMA FIFO underrun
	SDP_EVENTS_DMA_EXHAUSTED = 0x04,    ///< SDP DMA exhausted
	SDP_EVENTS_DMA_END = 0x08,          ///< SDP DMA end

	SDP_EVENTS_MRX_FIFO_EMPTY = 0x10,   ///< SDP MRX FIFO empty
	SDP_EVENTS_MTX_FIFO_FULL = 0x20,    ///< SDP MTX FIFO full
	SDP_EVENTS_READ_DATA_UNALIGN = 0x40,///< SDP Master read non align length

	ENUM_DUMMY4WORD(SDP_EVENTS)
} SDP_EVENTS;

/*
    SDP_TRANSFER_MODE

    @note For SDP_PARAM_ID_TRANSFER_MODE
*/
typedef enum {
	SDP_TRANSFER_MODE_CONT,		//< only support continously read a burst data
	SDP_TRANSFER_MODE_SLICE,	//< only support read slices data

	ENUM_DUMMY4WORD(SDP_TRANSFER_MODE)
} SDP_TRANSFER_MODE;


/* enum _SDP_PARAM_ID: structure for set/get functions
 *
 */
typedef enum _SDP_PARAM_ID {
	SDP_PARAM_ID_EVENT_CALLBACK,    ///< [set/get] SDP event callback.
	SDP_PARAM_ID_TRANSFER_MODE,		///< [set/get] SDP transfer mode by peer SPI master
									///< can be SDP_TRANSFER_MODE_CONT or SDP_TRANSFER_MODE_SLICE

	ENUM_DUMMY4WORD(SDP_PARAM_ID)
} SDP_PARAM_ID;

/*
 * SDP operation command
 *
 * Select which operation for SDP engine.
 *
 * @note Used for SDP_TRIGGER_PARAM.
 */
typedef enum _SDP_CMD {
	SDP_CMD_NOP,                        ///< NULL operation
	SDP_CMD_SET_MASTER_RX_PORT,         ///< Set master receive port (SDP -> SPI master)
	                                    ///< Set data word to be returned to SPI master through command 08H
	SDP_CMD_GET_MASTER_TX_PORT,         ///< Get master transmit port (SDP <- SPI master)
	                                    ///< Get data word transferred from SPI master through command 09H
	SDP_CMD_SET_DMA_BUF,                ///< Set DMA buffer (SDP -> SPI master)
	                                    ///< Set data buffer to be returned to SPI master through command 03H
	SDP_CMD_ABORT_DMA,                	///< Abort DMA Transfer
	                                    ///< Abort DMA to be returned to SPI master through command 03H

	SDP_CMD_COUNT,
	ENUM_DUMMY4WORD(SDP_CMD)
} SDP_CMD;


/* struct  _SDP_TRIGGER_PARAM: structure to describe a job
 *
 * @note For sdp_trigger()
 */
typedef struct _SDP_TRIGGER_PARAM {
	UINT32		ver;        	///< driver version (reserved)
	SDP_CMD     command;        ///< Command
	UINT32      *p_data;        ///< data buffer reference by command
	UINT32      size;           ///< size of p_data (unit: byte)
} SDP_TRIGGER_PARAM;

/**
    SDP controller clock source selection

    @note Used in SDP_setConfig(SDP_CONFIG_ID_CLK_SRC_SEL);
*/
/*
typedef enum {
	SDP_CLK_SRC_RTC,   ///< SDP clock source on real chip is 32.768KHz, and 31.25KHz on FPGA.
	SDP_CLK_SRC_OSC,   ///< SDP clock source on real chip is 32KHz, and 32KHz on FPGA.
	SDP_CLK_SRC_EXT,   ///< SDP clock source from external clock.
	SDP_CLK_SRC_3M,    ///< SDP clock source on real chip is 3MHz, and 300KHz on FPGA.

	ENUM_DUMMY4WORD(SDP_CLK_SRC_SEL)
} SDP_CLK_SRC_SEL;
*/

#define _EMULATION_                 DISABLE
#define _FPGA_EMULATION_            DISABLE

extern ER sdp_open(SDP_ID id);
extern ER sdp_close(SDP_ID id);
extern ER sdp_trigger(SDP_ID id, SDP_TRIGGER_PARAM *p_param, DRV_CB p_cb_func, void *p_user_data);
extern ER sdp_set(SDP_ID id, SDP_PARAM_ID param_id, VOID *p_param);
extern ER sdp_reset(void);

void sdp_isr(void);
void sdp_init(UINT32 BaseAddr);
void sdp_platform_init(void);
void sdp_platform_uninit(void);

//@}

#endif
