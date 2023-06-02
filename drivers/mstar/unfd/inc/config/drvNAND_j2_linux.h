#ifndef __UNFD_J2_LINUX_H__
#define __UNFD_J2_LINUX_H__

#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm/io.h>
#include <linux/string.h>

#define REG_BANK_CLKGEN						(0x580)
#define REG_BANK_CHIPTOP					(0xF00)
#define REG_BANK_FCIE0						(0x8980)
#define REG_BANK_FCIE2						(0x8A00)
#define REG_BANK_TIMER0						(0x1800)

#define RIU_NONPM_BASE						0xBF000000
#define RIU_BASE							0xBF200000

#define MPLL_CLK_REG_BASE_ADDR				(RIU_BASE+(REG_BANK_CLKGEN<<REG_OFFSET_SHIFT_BITS))
#define CHIPTOP_REG_BASE_ADDR				(RIU_BASE+(REG_BANK_CHIPTOP<<REG_OFFSET_SHIFT_BITS))
#define FCIE_REG_BASE_ADDR					(RIU_BASE+(REG_BANK_FCIE0<<REG_OFFSET_SHIFT_BITS))
#define FCIE_NC_CIFD_BASE					(RIU_BASE+(REG_BANK_FCIE2<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_REG_BASE_ADDR				(RIU_NONPM_BASE+(REG_BANK_TIMER0<<REG_OFFSET_SHIFT_BITS))

#define REG(Reg_Addr)						(*(volatile U16*)(Reg_Addr))
#define REG_OFFSET_SHIFT_BITS				2
#define GET_REG_ADDR(x, y)					(x+((y)<<REG_OFFSET_SHIFT_BITS))

#define REG_WRITE_UINT16(reg_addr, val)		REG(reg_addr) = val
#define REG_READ_UINT16(reg_addr, val)		val = REG(reg_addr)
#define HAL_WRITE_UINT16(reg_addr, val)		(REG(reg_addr) = val)
#define HAL_READ_UINT16(reg_addr, val)		val = REG(reg_addr)
#define REG_SET_BITS_UINT16(reg_addr, val)	REG(reg_addr) |= (val)
#define REG_CLR_BITS_UINT16(reg_addr, val)	REG(reg_addr) &= ~(val)
#define REG_W1C_BITS_UINT16(reg_addr, val)	REG_WRITE_UINT16(reg_addr, REG(reg_addr)&(val))

#define REG50_ECC_CTRL_INIT_VALUE			0

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

#define IF_FCIE_SHARE_PINS					1 // 1: need to nand_pads_switch at HAL's functions.
#define IF_FCIE_SHARE_CLK					0 // 1: need to nand_clock_setting at HAL's functions.
#define IF_FCIE_SHARE_IP					0

#define ENABLE_NAND_INTERRUPT_MODE			1

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

#define UNFD_CACHE_LINE						0x80
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
#define NAND_DEBUG_MSG						0
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
#define nand_print_tmt						//msPrintfFunc
#define nand_printf							printk
#define nand_debug(dbg_lv, tag, str, ...)				\
	do {												\
		if (dbg_lv > UNFD_DEBUG_LEVEL)				\
			break;									\
		else {										\
			if (tag)									\
			{                                       \
				nand_printf(__func__);				\
				/*nand_print_tmt(__func__);*/				\
			}                                       \
													\
			nand_printf(str, ##__VA_ARGS__);			\
			/*nand_print_tmt(str, ##__VA_ARGS__);*/			\
		}											\
	} while(0)
#else /* NAND_DEBUG_MSG */
#define nand_printf(...)
#define nand_debug(enable, tag, str, ...)	{}
#endif /* NAND_DEBUG_MSG */

static __inline void nand_assert(int condition)
{
	if (!condition) {
		
	}
}

#define nand_die() \
	do { \
		nand_printf(__func__); \
		nand_printf("\nUNFD Assert(%d)", __LINE__); \
		nand_assert(0); \
	} while(0);

#define nand_stop() \
	while(1)  nand_reset_WatchDog();

//=====================================================
// HW Timer for Delay
//=====================================================
#define TIMER0_ENABLE						GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x10)
#define TIMER0_HIT							GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x11)
#define TIMER0_MAX_LOW						GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x12)
#define TIMER0_MAX_HIGH						GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x13)
#define TIMER0_CAP_LOW						GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x14)
#define TIMER0_CAP_HIGH						GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x15)

#define HW_TIMER_DELAY_1us					1
#define HW_TIMER_DELAY_10us					10
#define HW_TIMER_DELAY_100us				100
#define HW_TIMER_DELAY_1ms					(1000 * HW_TIMER_DELAY_1us)
#define HW_TIMER_DELAY_5ms					(5    * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_10ms					(10   * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_100ms				(100  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_500ms				(500  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_1s					(1000 * HW_TIMER_DELAY_1ms)

extern void delay_us( unsigned us );
extern U32  nand_hw_timer_delay(U32 u32usTick);

//=====================================================
// Pads Switch
//=====================================================
#define reg_allpad_in						GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x50)
#define reg_nf_en							GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x6f)

extern U32 nand_pads_switch(U32 u32EnableFCIE);

//=====================================================
// set FCIE clock
//=====================================================
#define NFIE_CLK_MASK						(BIT4|BIT3|BIT2)
#define NFIE_CLK_5_4M						(0)
#define NFIE_CLK_27M						(BIT2)
#define NFIE_CLK_32M						(BIT3)
#define NFIE_CLK_43M						(BIT3|BIT2)
#define NFIE_CLK_86M						(BIT4)
#define NFIE_CLK_SSC						(BIT4|BIT2)
#define NFIE_CLK_36M						(BIT4|BIT3)
#define NFIE_CLK_40M						(BIT4|BIT3|BIT2)

#define DUTY_CYCLE_PATCH					1 // 1: to enlarge low width for tREA's worst case of 25ns
#if DUTY_CYCLE_PATCH 
#define FCIE3_SW_DEFAULT_CLK				NFIE_CLK_86M
#define FCIE_REG41_VAL						((2<<9)|(2<<3)) // RE,WR pulse, Low:High=3:1
#else
#define FCIE3_SW_DEFAULT_CLK				NFIE_CLK_32M
#define FCIE_REG41_VAL						0               // RE,WR pulse, Low:High=1:1
#endif
#define FCIE3_SW_SLOWEST_CLK				NFIE_CLK_5_4M

#define NAND_SEQ_ACC_TIME_TOL				10 //in unit of ns

#define reg_ckg_fcie						GET_REG_ADDR(MPLL_CLK_REG_BASE_ADDR, 0x64)

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
#define DMA_W_ADDR							0x80C00000
#define DMA_R_ADDR							0x80D00000
#define DMA_W_SPARE_ADDR					0x80E00000
#define DMA_R_SPARE_ADDR					0x80E80000
#define DMA_BAD_BLK_BUF						0x80F00000

#define MIU_CHECK_LAST_DONE					1

//=====================================================
// misc
//=====================================================
//#define BIG_ENDIAN
#define LITTLE_ENDIAN

typedef struct NAND_DRIVER_PLATFORM_DEPENDENT
{
	U8 *pu8_PageSpareBuf;
	U8 *pu8_PageDataBuf;
	U32	u32_DMAAddrOffset;
	U32	u32_RAMBufferOffset;
	U32	u32_RAMBufferLen;

}NAND_DRIVER_PLAT_CTX, *P_NAND_DRIVER_PLAT;

#define VENDOR_FUJITSU						0x04
#define VENDOR_RENESAS						0x07
#define VENDOR_ST							0x20
#define VENDOR_MICRON						0x2C
#define VENDOR_NATIONAL						0x8F
#define VENDOR_TOSHIBA						0x98
#define VENDOR_HYNIX						0xAD
#define VENDOR_SAMSUNG						0xEC

#define NC_REG_MIU_LAST_DONE				NC_MIE_EVENT

#endif

