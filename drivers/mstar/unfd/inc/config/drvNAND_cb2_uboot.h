#ifndef __UNFD_CB2_UBOOT_H__
#define __UNFD_CB2_UBOOT_H__

#include <common.h>
#include <malloc.h>
#include <linux/string.h>
#include "hwreg.h"

#define miu_ecc_patch           BIT1 // NC_REG_2Fh
#define FCIE_REG41_VAL          0

#define REG50_ECC_CTRL_INIT_VALUE    0//(BIT_NC_SHARE_PAD_EN)

#define UNFD_ST_PLAT						0x80000000
#define IF_IP_VERIFY						0 // [CAUTION]: to verify IP and HAL code, defaut 0

// debug
#define FCIE3_DUMP_DEBUG_BUS				1

#define NC_SEL_FCIE3						1
#if NC_SEL_FCIE3
#include "drvNAND_reg_v3.h"
#else
#error "Error! no FCIE registers selected."
#endif

#define IF_FCIE_SHARE_PINS       0 // 1: need to nand_pads_switch at HAL's functions.
#define IF_FCIE_SHARE_CLK        1 // 1: need to nand_clock_setting at HAL's functions.
#define IF_FCIE_SHARE_IP         1

#define ENABLE_NAND_INTERRUPT_MODE			0

#define NAND_DRIVER_ROM_VERSION				0 // to save code size
#define AUTO_FORMAT_FTL						0

#define ENABLE_CUS_READ_ENHANCEMENT			0

#define __VER_UNFD_FTL__					0
//=====================================================
// Nand Driver configs
//=====================================================
#define NAND_BUF_USE_STACK					0
#define NAND_ENV_FPGA						1
#define NAND_ENV_ASIC						2
#ifdef __FPGA_MODE__	// currently G1 don't care this macro, even in ROM code
#define NAND_DRIVER_ENV						NAND_ENV_FPGA
#else
#define NAND_DRIVER_ENV						NAND_ENV_ASIC
#endif

#define UNFD_CACHE_LINE						0x20
//=====================================================
// tool-chain attributes
//=====================================================
#define UNFD_PACK0
#define UNFD_PACK1							__attribute__((__packed__))
#define UNFD_ALIGN0
#define UNFD_ALIGN1							__attribute__((aligned(UNFD_CACHE_LINE)))


//=====================================================
// debug option
//=====================================================
#define NAND_TEST_IN_DESIGN					1      /* [CAUTION] */

#ifndef NAND_DEBUG_MSG
#define NAND_DEBUG_MSG						1
#endif

/* Define trace levels. */
#define UNFD_DEBUG_LEVEL_ERROR				(1)    /* Error condition debug messages. */
#define UNFD_DEBUG_LEVEL_WARNING			(2)    /* Warning condition debug messages. */
#define UNFD_DEBUG_LEVEL_HIGH				(3)    /* Debug messages (high debugging). */
#define UNFD_DEBUG_LEVEL_MEDIUM				(4)    /* Debug messages. */
#define UNFD_DEBUG_LEVEL_LOW				(5)    /* Debug messages (low debugging). */

/* Higer debug level means more verbose */
#ifndef UNFD_DEBUG_LEVEL
#define UNFD_DEBUG_LEVEL					UNFD_DEBUG_LEVEL_WARNING
#endif

#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
extern int printf(const char *fmt, ...);
#define nand_printf    printf
#define nand_debug(dbg_lv, tag, str, ...)	\
	do {	\
		if (dbg_lv > UNFD_DEBUG_LEVEL)				\
			break;									\
		else {										\
			if (tag)									\
				nand_printf("[ %s() ] ", __func__);	\
													\
			nand_printf(str, ##__VA_ARGS__);			\
		} \
	} while(0)
#else /* NAND_DEBUG_MSG */
#define nand_printf(...)
#define nand_debug(enable, tag, str, ...)	{}
#endif /* NAND_DEBUG_MSG */

extern void nand_reset_WatchDog(void);
extern void nand_set_WatchDog(U8 u8_IfEnable);

#define nand_die(msg) \
	{ \
		nand_debug(0,1,"NAND_DIE!!\n"); \
		while(1); \
	}
		

#define nand_stop() \
	{ \
		nand_debug(0,1,"NAND_STOP!!\n"); \
		while(1); \
	}
	
//=====================================================
// HW Timer for Delay
//=====================================================
#define HW_TIMER_DELAY_1us	    1
#define HW_TIMER_DELAY_10us     10
#define HW_TIMER_DELAY_100us	100
#define HW_TIMER_DELAY_1ms	    (1000 * HW_TIMER_DELAY_1us)
#define HW_TIMER_DELAY_5ms	    (5    * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_10ms	    (10   * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_100ms	(100  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_500ms	(500  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_1s	    (1000 * HW_TIMER_DELAY_1ms)

extern void delay_us( unsigned us );
extern U32  nand_hw_timer_delay(U32 u32usTick);

//=====================================================
// Pads Switch
//=====================================================
extern U32 nand_pads_switch(U32 u32EnableFCIE);

//=====================================================
// set FCIE clock
//=====================================================
#define NFIE_REG_CLK_MASK          (BIT6-1)
#define NFIE_REG_CLK_300K          0
#define NFIE_REG_CLK_5_3M          (1<<2)
#define NFIE_REG_CLK_24M           (2<<2)
#define NFIE_REG_CLK_32M           (3<<2)
#define NFIE_REG_CLK_40M           (4<<2)
#define NFIE_REG_CLK_48M           (5<<2)

#define NFIE_CLK_300K              300*1000
#define NFIE_CLK_5_3M              5300*1000
#define NFIE_CLK_24M               24*1000*1000
#define NFIE_CLK_32M               32*1000*1000
#define NFIE_CLK_40M               40*1000*1000
#define NFIE_CLK_48M               48*1000*1000

#define DUTY_CYCLE_PATCH					0 // 1: to enlarge low width for tREA's worst case of 25ns
#if DUTY_CYCLE_PATCH 
#define FCIE3_SW_DEFAULT_CLK				NFIE_CLK_86M
#define FCIE_REG41_VAL						((2<<9)|(2<<3)) // RE,WR pulse, Low:High=3:1
#else
#define FCIE3_SW_DEFAULT_CLK				NFIE_CLK_24M
#define FCIE_REG41_VAL						0               // RE,WR pulse, Low:High=1:1
#endif
#define FCIE3_SW_SLOWEST_CLK				NFIE_CLK_5_3M

#define NAND_SEQ_ACC_TIME_TOL				10 //in unit of ns


extern U32  nand_clock_setting(U32 u32ClkParam);

//=====================================================
// transfer DMA Address
//=====================================================
#define MIU_BUS_WIDTH_BITS					3 // Need to confirm
/*
 * Important:
 * The following buffers should be large enough for a whole NAND block
 */
// FIXME, this is only for verifing IP
#define DMA_W_ADDR       0x24000000
#define DMA_R_ADDR       0x24800000
#define DMA_W_SPARE_ADDR 0x25000000
#define DMA_R_SPARE_ADDR 0x25100000
#define DMA_BAD_BLK_BUF  0x25200000

#define MIU_CHECK_LAST_DONE					1

//=====================================================
// misc
//=====================================================
//#define BIG_ENDIAN
#define LITTLE_ENDIAN

typedef struct NAND_DRIVER_PLATFORM_DEPENDENT
{
	U8 		*pu8_PageSpareBuf;
	U8		*pu8_PageDataBuf;
	U32		u32_DMAAddrOffset;
	U32		u32_RAMBufferOffset;
	U32		u32_RAMBufferLen;

}NAND_DRIVER_PLAT_CTX, *P_NAND_DRIVER_PLAT;

#define NC_REG_MIU_LAST_DONE				NC_MIE_EVENT

#endif /* __UNFD_CB2_UBOOT_H__ */
