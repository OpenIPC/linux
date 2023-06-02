/*
    MIPI-DSI Controller internal header

    MIPI-DSI Controller internal header

    @file       dsi_int.h
    @ingroup    mIDrvDisp_DSI
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef __DSI_INT_H__
#define __DSI_INT_H__

#ifdef __KERNEL__
#include <mach/rcw_macro.h>
//#include <mach/ioaddress.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "dsi_dbg.h"
#include "dsi.h"
#include "dsi_reg.h"
#else
#if defined(__FREERTOS)
#include <kwrap/debug.h>
#include <kwrap/spinlock.h>
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "dsi.h"
#include "dsi_reg.h"
#include "io_address.h"
#include "pll.h"
#include "pll_protected.h"
#include "top.h"
#else
#include "DrvCommon.h"
#include "pll.h"
#include "pll_protected.h"
#include "dsi.h"
#include "dsi_reg.h"
#endif
#endif



// -----------------------------------------------------------------------------
//  DSI register access definition
// -----------------------------------------------------------------------------
#ifdef __KERNEL__
extern UINT32 _DSI_REG_BASE_ADDR;
#define DSI_REG_ADDR(ofs)           (_DSI_REG_BASE_ADDR+(ofs))
#define DSI_GETREG(ofs)             INW(_DSI_REG_BASE_ADDR+(ofs))
#define DSI_SETREG(ofs, value)       OUTW(_DSI_REG_BASE_ADDR+(ofs), (value))
#else
#define DSI_REG_ADDR(ofs)           (IOADDR_DSI_REG_BASE+(ofs))
#define DSI_GETREG(ofs)             INW(IOADDR_DSI_REG_BASE+(ofs))
#define DSI_SETREG(ofs, value)       OUTW(IOADDR_DSI_REG_BASE+(ofs), (value))
#endif

//#define DSI_PHY

// -----------------------------------------------------------------------------
// General definition
// -----------------------------------------------------------------------------
#define DSI_DEBUG                       ENABLE

#define DSI_LOG_CFG                     DISABLE

//#define DSI_ERR_MSG(...)               DBG_ERR(__VA_ARGS__)

//#define DSI_WRN_MSG(...)               DBG_WRN(__VA_ARGS__)

#if (DSI_LOG_CFG == ENABLE)
#define DSI_LOG_MSG(...)               DBG_DUMP(__VA_ARGS__)
#else
#define DSI_LOG_MSG(...)
#endif


// -----------------------------------------------------------------------------
// DSI Escape control (0x24,0x28)
// -----------------------------------------------------------------------------
typedef enum {
	DSI_SET_DAT0_ESC_START = 0x0,
	DSI_SET_DAT1_ESC_START,

	DSI_SET_DAT0_ESC_STOP,
	DSI_SET_DAT1_ESC_STOP,

	DSI_SET_CLK_ULP_SEL,

	DSI_SET_DAT0_ESC_CMD,
	DSI_SET_DAT1_ESC_CMD,

	ENUM_DUMMY4WORD(DSI_CFG_ESCAPE_CMD_CTRL)
} DSI_CFG_ESCAPE_CMD_CTRL;

// -----------------------------------------------------------------------------
// Interrupt enabled register (0x80)
// -----------------------------------------------------------------------------
#define DSI_TX_DIS_INTEN                0x00000001  // bit[0]
#define DSI_FRM_END_INTEN               0x00000002  // bit[1]
// bit[2]
// bit[3]

#define DSI_SRAM_OV_INTEN               0x00000010  // bit[4]
#define DSI_BTA_TIMEOUT_INTEN           0x00000020  // bit[5]
#define DSI_BTA_FAIL_INTEN              0x00000040  // bit[6]
#define DSI_BUS_CONTENTION_INTEN        0x00000080  // bit[7]

#define DSI_RX_READY_INTEN              0x00000100  // bit[8]
#define DSI_ERR_REPORT_INTEN            0x00000200  // bit[9]
#define DSI_RX_ECC_ERR_INTEN            0x00000400  // bit[10]
#define DSI_RX_CRC_ERR_INTEN            0x00000800  // bit[11]

#define DSI_RX_STATE_ERR_INTEN          0x00001000  // bit[12]
#define DSI_RX_INSUFFICIENT_PL_INTEN    0x00002000  // bit[13]
#define DSI_RX_ALIGN_ERR_INTEN          0x00004000  // bit[14]
#define DSI_FIFO_UNDER_INTEN            0x00008000  // bit[15]

#define DSI_SYNC_ERR_INTEN              0x00010000  // bit[16]
#define DSI_RX_UNKNOWN_PKT_INTEN        0x00020000  // bit[17]
#define DSI_RX_INVALID_ESCCMD_INTEN     0x00040000  // bit[18]
#define DSI_HS_FIFO_UNDER_INTEN         0x00080000  // bit[19]

// bit[20]
// bit[21]
// bit[22]
// bit[23]
#define CLK_ULPS_DONE_INTEN             0x01000000  // bit[24]
#define DAT0_ESC_DONE_INTEN             0x02000000  // bit[25]
#define DAT1_ESC_DONE_INTEN             0x04000000  // bit[26]
#define DAT2_ESC_DONE_INTEN             0x08000000  // bit[27]
#define DAT3_ESC_DONE_INTEN             0x10000000  // bit[28]
// bit[29]
// bit[30]
// bit[31]

// -----------------------------------------------------------------------------
// Interrupt enabled register (0x88)
// -----------------------------------------------------------------------------
#define DSI_TX_DIS_STS                  0x00000001  // bit[0]
#define DSI_FRM_END_STS                 0x00000002  // bit[1]
// bit[2]
// bit[3]

#define DSI_SRAM_OV_STS                 0x00000010  // bit[4]
#define DSI_BTA_TIMEOUT_STS             0x00000020  // bit[5]
#define DSI_BTA_FAIL_STS                0x00000040  // bit[6]
#define DSI_BUS_CONTENTION_STS          0x00000080  // bit[7]

#define DSI_RX_READY_STS                0x00000100  // bit[8]
#define DSI_ERR_REPORT_STS              0x00000200  // bit[9]
#define DSI_RX_ECC_ERR_STS              0x00000400  // bit[10]
#define DSI_RX_CRC_ERR_STS              0x00000800  // bit[11]

#define DSI_RX_STATE_ERR_STS            0x00001000  // bit[12]
#define DSI_RX_INSUFFICIENT_PL_STS      0x00002000  // bit[13]
#define DSI_RX_ALIGN_ERR_STS            0x00004000  // bit[14]
#define DSI_FIFO_UNDER_STS              0x00008000  // bit[15]

#define DSI_SYNC_ERR_STS                0x00010000  // bit[16]
#define DSI_RX_UNKNOWN_PKT_STS          0x00020000  // bit[17]
#define DSI_RX_INVALID_ESCCMD_STS       0x00040000  // bit[18]
#define DSI_HS_FIFO_UNDER_STS           0x00080000  // bit[19]

// bit[20]
// bit[21]
// bit[22]
// bit[23]
#define CLK_ULPS_DONE_STS               0x01000000  // bit[24]
#define DAT0_ESC_DONE_STS               0x02000000  // bit[25]
#define DAT1_ESC_DONE_STS               0x04000000  // bit[26]
#define DAT2_ESC_DONE_STS               0x08000000  // bit[27]
#define DAT3_ESC_DONE_STS               0x10000000  // bit[28]
// bit[29]
// bit[30]
// bit[31]


#define DSI_ESC_SET_TRIGGER(lane, ctx, en);\
	{                                          \
		switch (lane) {                        \
		default:                               \
		case DSI_DATA_LANE_0:                  \
			ctx.bit.DAT0_ESC_TRIG = en;        \
			break;                             \
		case DSI_DATA_LANE_1:                  \
			ctx.bit.DAT1_ESC_TRIG = en;        \
			break;                             \
		case DSI_DATA_LANE_2:                  \
			ctx.bit.DAT2_ESC_TRIG = en;        \
			break;                             \
		case DSI_DATA_LANE_3:                  \
			ctx.bit.DAT3_ESC_TRIG = en;        \
			break;                             \
		}                                      \
	}
#define DSI_ESC_SET_START(lane, ctx, en);  \
	{                                          \
		switch (lane) {                        \
		default:                               \
		case DSI_DATA_LANE_0:                  \
			ctx.bit.DAT0_ESC_START = en;       \
			break;                             \
		case DSI_DATA_LANE_1:                  \
			ctx.bit.DAT1_ESC_START = en;       \
			break;                             \
		case DSI_DATA_LANE_2:                  \
			ctx.bit.DAT2_ESC_START = en;       \
			break;                             \
		case DSI_DATA_LANE_3:                  \
			ctx.bit.DAT3_ESC_START = en;       \
			break;                             \
		}                                      \
	}
#define DSI_ESC_SET_STOP(lane, ctx, en);   \
	{                                          \
		switch (lane) {                        \
		default:                               \
		case DSI_DATA_LANE_0:                  \
			ctx.bit.DAT0_ESC_STOP = en;        \
			break;                             \
		case DSI_DATA_LANE_1:                  \
			ctx.bit.DAT1_ESC_STOP = en;        \
			break;                             \
		case DSI_DATA_LANE_2:                  \
			ctx.bit.DAT2_ESC_STOP = en;        \
			break;                             \
		case DSI_DATA_LANE_3:                  \
			ctx.bit.DAT3_ESC_STOP = en;        \
			break;                             \
		}                                      \
	}
#define DSI_ESC_SET_CMD(lane, ctx, en);    \
	{                                          \
		switch (lane) {                        \
		default:                               \
		case DSI_DATA_LANE_0:                  \
			ctx.bit.DAT0_ESC_CMD = en;         \
			break;                             \
		case DSI_DATA_LANE_1:                  \
			ctx.bit.DAT1_ESC_CMD = en;         \
			break;                             \
		case DSI_DATA_LANE_2:                  \
			ctx.bit.DAT2_ESC_CMD = en;         \
			break;                             \
		case DSI_DATA_LANE_3:                  \
			ctx.bit.DAT3_ESC_CMD = en;         \
			break;                             \
		}                                      \
	}
//#define DSI_ESC_SET_TRIGGER(lane, ctx, en)   (lane == DSI_DATA_LANE_0)?(ctx.bit.DAT0_ESC_TRIG=en):(ctx.bit.DAT1_ESC_TRIG=en)

//#define DSI_ESC_SET_START(lane, ctx, en)     (lane == DSI_DATA_LANE_0)?(ctx.bit.DAT0_ESC_START=en):(ctx.bit.DAT1_ESC_START=en)

//#define DSI_ESC_SET_STOP(lane, ctx, en)    (lane == DSI_DATA_LANE_0)?(ctx.bit.DAT0_ESC_STOP=en):(ctx.bit.DAT1_ESC_STOP=en)

//#define DSI_ESC_SET_CMD(lane, ctx, cmd)    (lane == DSI_DATA_LANE_0)?(ctx.bit.DAT0_ESC_CMD=cmd):(ctx.bit.DAT1_ESC_CMD=cmd)


extern DSI_MODESEL  dsi_get_mode(void);
#if defined(__FREERTOS) && defined(_NVT_FPGA_)
extern void dsi_tc680_init(void);
extern ER dsi_tc680_writereg(UINT32 ui_offset, UINT32 ui_value);
extern ER dsi_tc680_readreg(UINT32 ui_offset, UINT32 *pui_value);
#endif
#endif
