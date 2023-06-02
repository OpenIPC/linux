/*
    Internal header file for arbiter module.

    This file is the internal header file that defines the variables, structures
    and function prototypes of arbiter module.

    @file       ddr_arb_int.h
    @ingroup    miDrvComm_Arb
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _DDR_ARB_INT_H
#define _DDR_ARB_INT_H

#if defined(__UITRON) || defined(__ECOS)
#include "ddr_arb.h"
#elif defined(__FREERTOS)
#include "ddr_arb_platform.h"
#else
#include <linux/slab.h>
#include "ddr_arb_platform.h"
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#include "comm/ddr_arb.h"
#include "comm/timer.h"


#define islower(c)    ((c) >= 'a')
#define toupper(c)    (islower((c)) ? ((c) + 'A' - 'a') : (c))


/**
    DMA controller ID

*/
typedef enum {
	DMA_ID_1,                           ///< DMA Controller
	DMA_ID_2,                           ///< DMA Controller 2

	DMA_ID_COUNT,                       //< DMA controller count

	ENUM_DUMMY4WORD(DMA_ID)
} DMA_ID;

typedef enum _DMA_DIRECTION {
	DMA_DIRECTION_READ,     // DMA read (DRAM -> Module)
	DMA_DIRECTION_WRITE,    // DMA write (DRAM <- Module)
	DMA_DIRECTION_BOTH,     // DMA read and write

	ENUM_DUMMY4WORD(DMA_DIRECTION)
} DMA_DIRECTION;

// Register setting
typedef enum _DMA_MONITOR_CH {
	DMA_MONITOR_CH0,                            //< monitor channel 0
	DMA_MONITOR_CH1,                            //< monitor channel 1
	DMA_MONITOR_CH2,                            //< monitor channel 2
	DMA_MONITOR_CH3,                            //< monitor channel 3
	DMA_MONITOR_CH4,                            //< monitor channel 4
	DMA_MONITOR_CH5,                            //< monitor channel 5
	DMA_MONITOR_CH6,                            //< monitor channel 6
	DMA_MONITOR_CH7,                            //< monitor channel 7

	DMA_MONITOR_ALL,                            //< monitor channel of all DMA traffic, similar to dma_getBWUtilization()

	DMA_MONITOR_CH_COUNT,

	ENUM_DUMMY4WORD(DMA_MONITOR_CH)
} DMA_MONITOR_CH;

typedef enum _DMA_CH_GROUP {
	DMA_CH_GROUP0 = 0x0,    // represent channel 00-31
	DMA_CH_GROUP1,          // represent channel 32-63
	DMA_CH_GROUP2,          // represent channel 64-95
	DMA_CH_GROUP3,          // represent channel 96-127
	DMA_CH_GROUP4,          // represent channel 128-159
	DMA_CH_GROUP5,          // represent channel 160-191

	DMA_CH_GROUP_CNT,
	ENUM_DUMMY4WORD(DMA_CH_GROUP)
} DMA_CH_GROUP;

typedef enum _DMA_HEAVY_LOAD_CH {
	DMA_HEAVY_LOAD_CH0 = 0x0,                   // Heavy load channel 0
	DMA_HEAVY_LOAD_CH1,                         // Heavy load channel 1
	DMA_HEAVY_LOAD_CH2,                         // Heavy load channel 2
	DMA_HEAVY_LOAD_CH_CNT,
	ENUM_DUMMY4WORD(DMA_HEAVY_LOAD_CH)
} DMA_HEAVY_LOAD_CH;

typedef enum _DMA_HEAVY_LOAD_TEST_METHOD {
	DMA_HEAVY_LOAD_RW_BURST_INTERLEAVE = 0x0,   // Heavy load test method 0 => RW burst interleave
	DMA_HEAVY_LOAD_READ_AFTER_WRITE_ALL_BUF,    // Heavy load test method 1 => Read after write all buffer
	DMA_HEAVY_LOAD_READ_ONLY,                   // Heavy load test method 2 => Read only
	DMA_HEAVY_LOAD_WRITE_ONLY,                  // Heavy load test method 3 => Write only
	ENUM_DUMMY4WORD(DMA_HEAVY_LOAD_TEST_METHOD)
} DMA_HEAVY_LOAD_TEST_METHOD;

typedef struct _DMA_HEAVY_LOAD_PARAM {
	UINT32                      test_times;    // Heavy load test times, 0 is not available
	DMA_HEAVY_LOAD_TEST_METHOD  test_method;   // Heavy load test method, 0 is not available
	UINT32                      burst_len;     // Heavy load pattern burst length(0~127)
	UINT32                      start_addr;    // Heavy load start address(word alignment)
	UINT32                      dma_size;      // Heavy load test dma size(word alignment)
} DMA_HEAVY_LOAD_PARAM, *PDMA_HEAVY_LOAD_PARAM;

typedef enum _DPROF_CH {
	DPROF_CPU,
	DPROF_CNN,
	DPROF_CNN2,
	DPROF_NUE,
	DPROF_NUE2,
} DPROF_CH;

UINT32 dma_get_utilization(DDR_ARB id);
UINT32 dma_get_efficiency(DDR_ARB id);

ER dma_set_system_priority(BOOL is_direct);
extern BOOL dprof_start(DDR_ARB id, DPROF_CH ch, DMA_DIRECTION dir);
extern BOOL dprof_dump_and_stop(DDR_ARB id);
#endif

#define ARB_SETREG(ofs,value)	OUTW(IOADDR_DDR_ARB_REG_BASE+(ofs),(value))
#define ARB_GETREG(ofs)		INW(IOADDR_DDR_ARB_REG_BASE+(ofs))

#define PROT_SETREG(ofs,value)	OUTW(IOADDR_DDR_ARB_PROT_REG_BASE+(ofs),(value))
#define PROT_GETREG(ofs)		INW(IOADDR_DDR_ARB_PROT_REG_BASE+(ofs))

typedef void (*dma_hvyload_callback_func)(void);

// Register setting
UINT32 dma_get_monitor_period(DDR_ARB id);
void dma_set_monitor_period(void);

ER dma_enable_heavyload(DDR_ARB id, DMA_HEAVY_LOAD_CH channel, PDMA_HEAVY_LOAD_PARAM hvy_param);
BOOL dma_wait_heavyload_done_polling(DDR_ARB id, dma_hvyload_callback_func call_back_hdl);
void dma_trig_heavyload(DDR_ARB id, UINT32 channel);
void dma_stop_heavyload(DDR_ARB id);

#endif
