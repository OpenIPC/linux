#ifndef __UNFD_B3_ROM_H__
#define __UNFD_B3_ROM_H__


#define UNFD_ST_PLAT             0x80000000
#define IF_IP_VERIFY             0 // [CAUTION]: to verify IP and HAL code, defaut 0
#define FTL_FIRST_BLOCK          10

#include <stdio.h>
#include <string.h>
#include "datatype.h"
#include "hwreg.h"

#include "bootrom.h"
#include "cpu_mem_map.h"

#define NC_SEL_FCIE3             1
#if NC_SEL_FCIE3
#include "drvNAND_reg_v3.h"
#else
#error "Error! no FCIE registers selected."
#endif

#define drvNAND_SendMsg2BkgndTask(x)

#if IF_IP_VERIFY
// select a default NAND chip for IP_VERIFY or NAND_PROGRAMMER
//#define K9F1G08X0C               1
//#define H27UF081G2A              1
#define K511F12ACA			       1
//#define TY8A0A111162KC40         1
//#define K522H1GACE               1
//#define TY890A111229KC40         1
//#define H8ACU0CE0DAR             1
//#define H9LA25G25HAMBR           1
//#define TY8A0A111178KC4          1

#define NAND_IF_WORD_MODE          0

#define FCIE_POWER_SAVE_MODE       1
#endif

#define IF_FCIE_SHARE_PINS         0 // 1: need to nand_pads_switch at HAL's functions.
#define IF_FCIE_SHARE_CLK          1 // 1: need to nand_clock_setting at HAL's functions.
#define IF_FCIE_SHARE_IP           1

#define ENABLE_NAND_INTERRUPT_MODE 0

#define REG50_ECC_CTRL_INIT_VALUE    (BIT_NC_SHARE_PAD_EN)
#define REG30_TEST_MODE_INIT_VALUE   0x5800

#define MIU_CHECK_LAST_DONE          1
#define NC_REG_MIU_LAST_DONE		 NC_PATH_CTL

#define ECC_ALLOWED_MAX_BITS_CNT     2
#define DEBUG_ASSERT_PARTITION_TYPE  UNFD_PART_CUS

#define REG57_ECO_FIX_INIT_VALUE    (BIT_NC_PATCH_WAIT_RB_EN | BIT_NC_PATCH_JOB_START_EN)

#define NAND_DRIVER_ROM_VERSION    0 // to save code size
#define AUTO_FORMAT_FTL            1

#if defined(__WINTARGET__)
#define __VER_UNFD_FTL__           1
#endif
#if defined(__BOOTLOADER__)
#define NAND_BLD_LD_OS             1
#define BLD_LD_OS_RD               1
#endif

#if defined(__BOOTLOADER__)
#define ENABLE_CB_BEFORE_DMA              1
#endif

//=====================================================
// Nand Driver configs
//=====================================================
#define NAND_BUF_USE_STACK       0
#define NAND_ENV_FPGA            1
#define NAND_ENV_ASIC            2
#ifdef __FPGA_MODE__
#define NAND_DRIVER_ENV          NAND_ENV_FPGA
#else
#define NAND_DRIVER_ENV          NAND_ENV_ASIC
#endif

#define UNFD_CACHE_LINE   0x20
//=====================================================
// tool-chain attributes
//=====================================================
#define UNFD_PACK0
#define UNFD_PACK1         __attribute__((packed))
#define UNFD_ALIGN0       __align(UNFD_CACHE_LINE)
#define UNFD_ALIGN1


//=====================================================
// debug option
//=====================================================
#define NAND_TEST_IN_DESIGN      0             /* [CAUTION] */

#if defined(__FPGA_MODE__) || defined(__DEBUG_ON__)
#define NAND_DEBUG_MSG           1
#else
#define NAND_DEBUG_MSG           0
#endif

/* Define trace levels. */
#define UNFD_DEBUG_LEVEL_ERROR		(1)    /* Error condition debug messages. */
#define UNFD_DEBUG_LEVEL_WARNING	(2)    /* Warning condition debug messages. */
#define UNFD_DEBUG_LEVEL_HIGH		(3)    /* Debug messages (high debugging). */
#define UNFD_DEBUG_LEVEL_MEDIUM  	(4)    /* Debug messages. */
#define UNFD_DEBUG_LEVEL_LOW		(5)    /* Debug messages (low debugging). */

/* Higer debug level means more verbose */
#ifndef UNFD_DEBUG_LEVEL
#define UNFD_DEBUG_LEVEL            UNFD_DEBUG_LEVEL_WARNING//UNFD_DEBUG_LEVEL_WARNING
#endif

#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
#define nand_printf	printf
#define nand_debug(dbg_lv, tag, str, ...)		\
	do {										\
		if (dbg_lv > UNFD_DEBUG_LEVEL)		\
			break;							\
		else {								\
		if (tag)								\
			nand_printf("[ %30s() ] ", __func__);\
											\
		nand_printf(str, ##__VA_ARGS__);		\
		}									\
	} while(0)

#else /* NAND_DEBUG_MSG */
#define nand_printf(...)
#define nand_debug(enable, tag, str, ...)      {}
#endif /* NAND_DEBUG_MSG */

extern void nand_reset_WatchDog(void);
extern void nand_set_WatchDog(U8 u8_IfEnable);

#define nand_die(msg) SYS_FAIL(""msg);

#define nand_stop() \
	while(1)  nand_reset_WatchDog();
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
//#define NFIE_CLK_MASK         (BIT4-1)
#define NFIE_CLK_187_5K		187500
#define NFIE_CLK_750K		750000
#define NFIE_CLK_6M			6000000
#define NFIE_CLK_10_4M		10400000
#define NFIE_CLK_13M		13000000
#define NFIE_CLK_19_5M		19500000
#define NFIE_CLK_22_29M		22290000
#define NFIE_CLK_26M		26000000
#define NFIE_CLK_39M		39000000
#define NFIE_CLK_44_57M		44570000
#define NFIE_CLK_52M		52000000
#define NFIE_CLK_78M		78000000
#define NFIE_CLK_31_2M		31200000
#define NFIE_CLK_34_67M		34670000
#define NFIE_CLK_29_71M		29710000

#define FCIE3_SW_DEFAULT_CLK  NFIE_CLK_26M//NFIE_CLK_34_67M
#define FCIE3_SW_SLOWEST_CLK  NFIE_CLK_6M

#define NAND_SEQ_ACC_TIME_TOL	10 //in unit of ns

#define reg_mclk_fcie_en         GET_REG_ADDR(MPLL_CLK_REG_BASE_ADDR, 0x50) // clr bit5
extern U32  nand_clock_setting(U32 u32ClkParam);

//=====================================================
// transfer DMA Address
//=====================================================
#define MIU_BUS_WIDTH_BITS       3 // 8 bytes aligned
/*
 * Important:
 * The following buffers should be large enough for a whole NAND block
 */
// FIXME, this is only for verifing IP
/* Here we assume that the block size is not larger than 512KB */
#define DMA_MEM_BASE     0x0C000000
#define DMA_W_ADDR       (DMA_MEM_BASE + 0x00800000)
#define DMA_R_ADDR       (DMA_MEM_BASE + 0x00880000)
#define DMA_W_SPARE_ADDR (DMA_MEM_BASE + 0x00910000)
#define DMA_R_SPARE_ADDR (DMA_MEM_BASE + 0x00920000)
#define DMA_BAD_BLK_BUF  (DMA_MEM_BASE + 0x00930000)

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

#endif /* __UNFD_B3_ROM_H__ */
