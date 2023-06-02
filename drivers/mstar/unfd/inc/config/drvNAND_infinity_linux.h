#ifndef __UNFD_INFINITY_LINUX_H__
#define __UNFD_INFINITY_LINUX_H__

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/highmem.h>

//#include "hwreg.h"
//#include "mdrv_type.h"

#define REG(Reg_Addr)				(*(volatile U16*)(IO_ADDRESS(Reg_Addr)))
#define REG_OFFSET_SHIFT_BITS       2
#ifndef  GET_REG_ADDR
#define GET_REG_ADDR(x, y)			(x+((y)<<REG_OFFSET_SHIFT_BITS))
#endif
//#include <asm/io.h>
#define REG_ADDR_CLKGEN         0x1C00U
#define REG_BANK_CHIPTOP        0xF00
#define REG_ADDR_CHIP_GPIO1     0xD00

#define REG_BANK_FCIE0          0x20100U
#define REG_BANK_FCIE1          0x20160U
#define REG_BANK_FCIE2          0x20180U
#define REG_BANK_FCIE3          0x20200U

#define RIU_PM_BASE							0x1F000000
#define RIU_BASE								0x1F200000

#define CHIPTOP_BASE            GET_REG_ADDR(RIU_BASE, REG_BANK_CHIPTOP)
#define CHIP_GPIO1_BASE         GET_REG_ADDR(RIU_BASE, REG_ADDR_CHIP_GPIO1)
#define CLKGEN0_BASE            GET_REG_ADDR(RIU_BASE, REG_ADDR_CLKGEN)

#define FCIE_REG_BASE_ADDR      GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE0)
#define FCIE_NC_CIFD_BASE       GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE2)
#define FCIE_NC_WBUF_CIFD_BASE 	GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE2)
#define FCIE_NC_RBUF_CIFD_BASE 	GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE2 + 0x20)

#define FCIE_REG_PWS_BASE_ADDR  GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE3)

#define REG_WRITE_UINT16(reg_addr, val)   REG(reg_addr) = val
#define REG_READ_UINT16(reg_addr, val)    val = REG(reg_addr)
#define HAL_WRITE_UINT16(reg_addr, val)   (REG(reg_addr) = val)
#define HAL_READ_UINT16(reg_addr, val)    val = REG(reg_addr)
#define REG_SET_BITS_UINT16(reg_addr, val)  REG(reg_addr) |= (val)
#define REG_CLR_BITS_UINT16(reg_addr, val)  REG(reg_addr) &= ~(val)
#define REG_W1C_BITS_UINT16(reg_addr, val)  REG_WRITE_UINT16(reg_addr, REG(reg_addr)&(val))

#define REG50_ECC_CTRL_INIT_VALUE     0

#define UNFD_ST_PLAT            0x80000000
#define IF_IP_VERIFY            0 // [CAUTION]: to verify IP and HAL code, defaut 0

// debug
#define FCIE3_DUMP_DEBUG_BUS        1

#define NC_SEL_FCIE5            1
#if NC_SEL_FCIE5
#include "drvNAND_reg_v5.h"
#else
#error "Error! no FCIE registers selected."
#endif


#define DDR_NAND_SUPPORT					0

#define MACRO_TYPE_8BIT			1
#define MACRO_TYPE_32BIT			2

#define ENABLE_32BIT_MACRO      1

//Each Platform, which supports DDR NAND,  Needs to define DQS pull high/low
#define NC_DQS_PULL_H()
#define NC_DQS_PULL_L()

#define NAND_PAD_BYPASS_MODE	    1
#define NAND_PAD_TOGGLE_MODE	    2
#define NAND_PAD_ONFI_SYNC_MODE		3

#if (defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT)
#define NC_SET_DDR_MODE()       REG_WRITE_UINT16(NC_DDR_CTRL, pNandDrv->u16_Reg58_DDRCtrl);
#define NC_CLR_DDR_MODE()       REG_CLR_BITS_UINT16(NC_DDR_CTRL, BIT_DDR_MASM);
#else
#define NC_SET_DDR_MODE()
#define NC_CLR_DDR_MODE()
#endif

#if (defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT)
#define DQS_MODE_2P0T		0
#define DQS_MODE_1P5T		1
#define DQS_MODE_2P5T		2
#define DQS_MODE_0P0T		3
#define DQS_MODE_TABLE_CNT	4
#define DQS_MODE_SEARCH_TABLE {DQS_MODE_1P5T, DQS_MODE_0P0T, DQS_MODE_2P0T, DQS_MODE_2P5T}
#define DQS_MODE_TABLE		{DQS_MODE_0P0T, DQS_MODE_1P5T, DQS_MODE_2P0T, DQS_MODE_2P5T}
#endif

#define NC_INST_DELAY	0
#define NC_HWCMD_DELAY	1
#define NC_TRR_TCS		1
#define NC_TWHR_TCLHZ			1
#define NC_TCWAW_TADL			1

// ask IP designer to obtain the following default delay
#define	NC_ONFI_DEFAULT_TRR 12
#define	NC_ONFI_DEFAULT_TCS 7
#define	NC_ONFI_DEFAULT_TWW	8
#define NC_ONFI_DEFAULT_TWHR 5
#define NC_ONFI_DEFAULT_TADL 6
#define NC_ONFI_DEFAULT_TCWAW 4
#define	NC_ONFI_DEFAULT_RX40CMD 4
#define	NC_ONFI_DEFAULT_RX40ADR 7
#define	NC_ONFI_DEFAULT_RX56 10

#define	NC_TOGGLE_DEFAULT_TRR 8
#define	NC_TOGGLE_DEFAULT_TCS 6
#define	NC_TOGGLE_DEFAULT_TWW 7
#define	NC_TOGGLE_DEFAULT_TWHR 5
#define	NC_TOGGLE_DEFAULT_TADL 7
#define	NC_TOGGLE_DEFAULT_TCWAW 2
#define	NC_TOGGLE_DEFAULT_RX40CMD 4
#define	NC_TOGGLE_DEFAULT_RX40ADR 5
#define	NC_TOGGLE_DEFAULT_RX56 15

#define	NC_SDR_DEFAULT_TRR 7
#define	NC_SDR_DEFAULT_TCS 6
#define	NC_SDR_DEFAULT_TWW 5
#define	NC_SDR_DEFAULT_TWHR 4
#define	NC_SDR_DEFAULT_TADL 8
#define	NC_SDR_DEFAULT_TCWAW 2
#define	NC_SDR_DEFAULT_RX40CMD 4
#define	NC_SDR_DEFAULT_RX40ADR 5
#define	NC_SDR_DEFAULT_RX56 5

#define IF_FCIE_SHARE_PINS        1 // 1: need to nand_pads_switch at HAL's functions.
#define IF_FCIE_SHARE_CLK         1 // 1: need to nand_clock_setting at HAL's functions.
#define IF_FCIE_SHARE_IP          1

#define ENABLE_NAND_INTERRUPT_MODE      1
#define ENABLE_AGATE					0
#define ENABLE_EAGLE                    0

#define NAND_DRIVER_ROM_VERSION       0 // to save code size
#define AUTO_FORMAT_FTL           0

#define ENABLE_CUS_READ_ENHANCEMENT     0

#define NAND_BLD_LD_OS   0
#define BLD_LD_OS_RD     0

#define __VER_UNFD_FTL__          0
//=====================================================
// Nand Driver configs
//=====================================================
#define NAND_BUF_USE_STACK          0
#define NAND_ENV_FPGA           1
#define NAND_ENV_ASIC           2
#ifdef __FPGA_MODE__  // currently G1 don't care this macro, even in ROM code
#define NAND_DRIVER_ENV           NAND_ENV_FPGA
#else
#define NAND_DRIVER_ENV           NAND_ENV_ASIC
#endif

#define UNFD_CACHE_LINE           0x80
//=====================================================
// tool-chain attributes
//=====================================================
#define UNFD_PACK0
#define UNFD_PACK1              __attribute__((__packed__))
#define UNFD_ALIGN0
#define UNFD_ALIGN1             __attribute__((aligned(UNFD_CACHE_LINE)))

//=====================================================
// debug option
//=====================================================
#define NAND_TEST_IN_DESIGN         0      /* [CAUTION] */

#ifndef NAND_DEBUG_MSG
#define NAND_DEBUG_MSG            1
#endif

/* Define trace levels. */
#define UNFD_DEBUG_LEVEL_ERROR        (1)    /* Error condition debug messages. */
#define UNFD_DEBUG_LEVEL_WARNING      (2)    /* Warning condition debug messages. */
#define UNFD_DEBUG_LEVEL_HIGH       (3)    /* Debug messages (high debugging). */
#define UNFD_DEBUG_LEVEL_MEDIUM       (4)    /* Debug messages. */
#define UNFD_DEBUG_LEVEL_LOW        (5)    /* Debug messages (low debugging). */

/* Higer debug level means more verbose */
#ifndef UNFD_DEBUG_LEVEL
#define UNFD_DEBUG_LEVEL          UNFD_DEBUG_LEVEL_WARNING
#endif

#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
#define nand_printf	     pr_info
#define nand_printfcont  pr_cont
#define nand_debug(dbg_lv, tag, str, ...)	\
	do {	\
		if (dbg_lv > UNFD_DEBUG_LEVEL)				\
			break;									\
		else {										\
			if (tag)									\
				nand_printf("[ %s() ] ", __func__);	\
													\
			nand_printfcont(str, ##__VA_ARGS__);			\
		} \
	} while(0)
#else /* NAND_DEBUG_MSG */
#define nand_printf(...)
#define nand_debug(enable, tag, str, ...)	{}
#endif /* NAND_DEBUG_MSG */

static __inline void nand_assert(int condition)
{
	if (!condition) {
		while(1)
			;
	}
}

#define nand_die() \
	panic("ERROR: UNFD Assert, line=%d, func=%s\n", __LINE__, __func__);


#define nand_stop() \
	while(1)  nand_reset_WatchDog();

//=====================================================
// HW Timer for Delay
//=====================================================
/*
#define TIMER0_ENABLE           GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x10)
#define TIMER0_HIT              GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x11)
#define TIMER0_MAX_LOW            GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x12)
#define TIMER0_MAX_HIGH           GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x13)
#define TIMER0_CAP_LOW            GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x14)
#define TIMER0_CAP_HIGH           GET_REG_ADDR(TIMER0_REG_BASE_ADDR, 0x15)
*/
#define HW_TIMER_DELAY_1us          1
#define HW_TIMER_DELAY_10us         10
#define HW_TIMER_DELAY_100us        100
#define HW_TIMER_DELAY_1ms          (1000 * HW_TIMER_DELAY_1us)
#define HW_TIMER_DELAY_5ms          (5    * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_10ms         (10   * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_100ms        (100  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_500ms        (500  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_1s         (1000 * HW_TIMER_DELAY_1ms)

#define NAND_SPEED_TEST             0

extern void delay_us( unsigned us );
extern U32  nand_hw_timer_delay(U32 u32usTick);

//=====================================================
// Pads Switch
//=====================================================
#define REG_ALLPAD_IN           	GET_REG_ADDR(CHIPTOP_BASE, 0x50)
#define REG_NAND_MODE               GET_REG_ADDR(CHIPTOP_BASE, 0x08)
#define REG_SD_MODE					GET_REG_ADDR(CHIPTOP_BASE, 0x08)
#define REG_CTRL_PE                 GET_REG_ADDR(CHIPTOP_BASE, 0x31)
#define reg_DATA_PE                 GET_REG_ADDR(CHIPTOP_BASE, 0x30)
#define REG_NAND_MODE_MASK 					(BIT0)
#define NAND_MODE1							BIT0
#define REG_SD_MODE_MASK					(BIT2)

extern U32 nand_pads_switch(U32 u32EnableFCIE);

//=====================================================
// set FCIE clock
//=====================================================
#define NFIE_CLK_MASK          ((BIT4-1)<<2)
#define NFIE_CLK_300K          (0xC<<2)
#define NFIE_CLK_5_4M          (0xB<<2)
#define NFIE_CLK_12M           (0x3<<2)
#define NFIE_CLK_20M           (0xA<<2)
#define NFIE_CLK_32M           (0x9<<2)
#define NFIE_CLK_36M           (0x8<<2)
#define NFIE_CLK_40M           (0x7<<2)
#define NFIE_CLK_43_2M         (0x6<<2)
#define NFIE_CLK_48M           (0x5<<2)
#define NFIE_CLK_54M           (0x2<<2)
#define NFIE_CLK_62M           (0x1<<2)
#define NFIE_CLK_72M           (0<<2)
#define NFIE_CLK_86M           (0x4<<2)

#define DECIDE_CLOCK_BY_NAND	1

#define NFIE_CLK_TABLE_CNT	13
#define NFIE_CLK_TABLE	{NFIE_CLK_300K, NFIE_CLK_12M, \
							NFIE_CLK_20M, NFIE_CLK_32M, NFIE_CLK_36M, NFIE_CLK_40M, \
							NFIE_CLK_43_2M, NFIE_CLK_48M, NFIE_CLK_54M, NFIE_CLK_62M, \
							NFIE_CLK_72M, NFIE_CLK_86M}

#define NFIE_CLK_TABLE_STR	{	"300K", "12M",\
							"20M", "32M", "36M", "40M", \
							"43.2M", "48M", "54M", "62M", \
							"72M", "86M" }

#define NFIE_300K_VALUE              300*1000
#define NFIE_12M_VALUE               12*1000*1000
#define NFIE_20M_VALUE             	 20*1000*1000
#define NFIE_32M_VALUE               32*1000*1000
#define NFIE_36M_VALUE               36*1000*1000
#define NFIE_40M_VALUE               40*1000*1000
#define NFIE_43_2M_VALUE             43200*1000
#define NFIE_48M_VALUE               48*1000*1000
#define NFIE_54M_VALUE               54*1000*1000
#define NFIE_62M_VALUE               62*1000*1000
#define NFIE_72M_VALUE               72*1000*1000
#define NFIE_86M_VALUE               86*1000*1000

#define NFIE_CLK_VALUE_TABLE	{	NFIE_300K_VALUE, NFIE_12M_VALUE, \
							NFIE_20M_VALUE, NFIE_32M_VALUE, NFIE_36M_VALUE, NFIE_40M_VALUE, \
							NFIE_43_2M_VALUE, NFIE_48M_VALUE, NFIE_54M_VALUE, NFIE_62M_VALUE, \
							NFIE_72M_VALUE, NFIE_86M_VALUE}


/*Define 1 cycle Time for each clock note: define value must be the (real value -1)*/
#define NFIE_1T_300K		3333//3
#define NFIE_1T_12M			83//3
#define NFIE_1T_20M			50//6
#define NFIE_1T_32M			31//2
#define NFIE_1T_36M			27//7
#define NFIE_1T_40M			25//0
#define NFIE_1T_43_2M		23//1
#define NFIE_1T_48M			20//8
#define NFIE_1T_54M			18//5
#define NFIE_1T_62M			16//1
#define NFIE_1T_72M			13//8
#define NFIE_1T_86M			11//6

#define NFIE_1T_TABLE	{	NFIE_1T_300K, NFIE_1T_12M, \
							NFIE_1T_20M, NFIE_1T_32M, NFIE_1T_36M, NFIE_1T_40M, \
							NFIE_1T_43_2M, NFIE_1T_48M, NFIE_1T_54M, NFIE_1T_62M, \
							NFIE_1T_72M, NFIE_1T_86M }


#define DUTY_CYCLE_PATCH            0 // 1: to enlarge low width for tREA's worst case of 25ns
#if DUTY_CYCLE_PATCH
#define FCIE3_SW_DEFAULT_CLK        NFIE_CLK_86M
#define FCIE_REG41_VAL              ((2<<9)|(2<<3)) // RE,WR pulse, Low:High=3:1
#define REG57_ECO_FIX_INIT_VALUE    0
#else
#define FCIE3_SW_DEFAULT_CLK        NFIE_CLK_54M
#define FCIE_REG41_VAL              0               // RE,WR pulse, Low:High=1:1
#define REG57_ECO_FIX_INIT_VALUE    BIT_NC_LATCH_DATA_2_0_T|BIT_NC_LATCH_STS_2_0_T  // delay 1.0T
#endif
#define FCIE3_SW_SLOWEST_CLK        NFIE_CLK_12M

#define NAND_SEQ_ACC_TIME_TOL       16 //in unit of ns

#define reg_ckg_fcie            GET_REG_ADDR(CLKGEN0_BASE, 0x43)
#define reg_ckg_ecc            GET_REG_ADDR(CLKGEN0_BASE, 0x44)
#define ECC_CLK_160M			0
#define ECC_CLK_108M			BIT0
#define ECC_CLK_54M				BIT1
#define ECC_CLK_12M				(BIT1|BIT0)

extern U32  nand_clock_setting(U32 u32ClkParam);
extern void nand_DumpPadClk(void);
//=====================================================
// transfer DMA Address
//=====================================================
#define MIU_BUS_WIDTH_BITS          3 // Need to confirm
/*
 * Important:
 * The following buffers should be large enough for a whole NAND block
 */
// FIXME, this is only for verifing IP
#define DMA_W_ADDR              0x80C00000
#define DMA_R_ADDR              0x80D00000
#define DMA_W_SPARE_ADDR          0x80E00000
#define DMA_R_SPARE_ADDR          0x80E80000
#define DMA_BAD_BLK_BUF           0x80F00000

#define MIU_CHECK_LAST_DONE         1

//=====================================================
// misc
//=====================================================
//#define BIG_ENDIAN
#define LITTLE_ENDIAN

typedef struct NAND_DRIVER_PLATFORM_DEPENDENT
{
  U8 *pu8_PageSpareBuf;
  U8 *pu8_PageDataBuf;
  U32 u32_DMAAddrOffset;
  U32 u32_RAMBufferOffset;
  U32 u32_RAMBufferLen;

}NAND_DRIVER_PLAT_CTX, *P_NAND_DRIVER_PLAT;

#define NC_REG_MIU_LAST_DONE        NC_MIE_EVENT

extern void flush_cache(U32 start_addr, U32 size);
extern void Chip_Flush_Memory(void);
extern void Chip_Read_Memory(void);

int printf(const char *format, ...);

#define NANDINFO_ECC_TYPE ECC_TYPE_40BIT1KB

#endif	//__UNFD_CEDRIC_LINUX_H__
