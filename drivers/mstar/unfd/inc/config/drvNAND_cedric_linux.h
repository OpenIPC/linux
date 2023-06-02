#ifndef __UNFD_CEDRIC_LINUX_H__
#define __UNFD_CEDRIC_LINUX_H__

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>
//#include "hwreg.h"
//#include "mdrv_type.h"

#define REG(Reg_Addr)				(*(volatile U16*)(IO_ADDRESS(Reg_Addr)))
#define REG_OFFSET_SHIFT_BITS       2
#ifndef  GET_REG_ADDR
#define GET_REG_ADDR(x, y)			(x+((y)<<REG_OFFSET_SHIFT_BITS))
#endif
//#include <asm/io.h>
#define REG_ADDR_CLKGEN         0x580
#define REG_BANK_CHIPTOP        0xF00
#define REG_BANK_PMUTOP         0x1F80
#define REG_ADDR_CHIP_GPIO1     0xD00

#define REG_BANK_FCIE0          0x8980
#define REG_BANK_FCIE1          0x8A00
#define REG_BANK_FCIE2          0x8A80
#define REG_BANK_FCIE3          0x8B00

#define RIU_PM_BASE							0x1F000000
#define RIU_BASE								0x1F200000

#define CHIPTOP_BASE            GET_REG_ADDR(RIU_BASE, REG_BANK_CHIPTOP)
#define PMUTOP_BASE             GET_REG_ADDR(RIU_BASE, REG_BANK_PMUTOP)
#define CHIP_GPIO1_BASE         GET_REG_ADDR(RIU_BASE, REG_ADDR_CHIP_GPIO1) 
#define CLKGEN0_BASE            GET_REG_ADDR(RIU_BASE, REG_ADDR_CLKGEN)
#define REG_CLK_NFIE            GET_REG_ADDR(CLKGEN0_BASE, 0x19)

#define FCIE_REG_BASE_ADDR      GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE0)
#define FCIE_NC_CIFC_BASE       GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE1)
#define FCIE_NC_CIFD_BASE       GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE2)

#define REG_ALL_PAD_IN  GET_REG_ADDR(CHIP_GPIO1_BASE, 0x00)
#define REG_BPS_MODE    GET_REG_ADDR(CHIP_GPIO1_BASE, 0x14)
#define REG_NF_MODE     GET_REG_ADDR(CHIP_GPIO1_BASE, 0x01)
#define REG_EMMC_MODE   GET_REG_ADDR(CHIP_GPIO1_BASE, 0x10)

#define FCIE_REG_CLK_4X_DIV_EN		    GET_REG_ADDR(CHIPTOP_BASE, 0x0E)

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

//FCIE4 DDR Nand
#define FCIE4_DDR                   1

#define SPARE640B_CIFD512B_PATCH    1
//Each Platform, which supports DDR NAND,  Needs to define DQS pull high/low
#define NC_DQS_PULL_H()             
#define NC_DQS_PULL_L()             

#define NAND_PAD_BYPASS_MODE	    1
#define NAND_PAD_TOGGLE_MODE	    2
#define NAND_PAD_ONFI_SYNC_MODE		3

#if (defined(FCIE4_DDR) && FCIE4_DDR)
#define NC_SET_DDR_MODE()       REG_WRITE_UINT16(NC_DDR_CTRL, pNandDrv->u16_Reg58_DDRCtrl);
#define NC_CLR_DDR_MODE()       REG_CLR_BITS_UINT16(NC_DDR_CTRL, BIT_DDR_MASM);
#else
#define NC_SET_DDR_MODE()
#define NC_CLR_DDR_MODE()
#endif

#if (defined(FCIE4_DDR) && FCIE4_DDR)
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

#define NC_SEL_FCIE3            1
#if NC_SEL_FCIE3
#include "drvNAND_reg_v3.h"
#else
#error "Error! no FCIE registers selected."
#endif


#define IF_FCIE_SHARE_PINS        1 // 1: need to nand_pads_switch at HAL's functions.
#define IF_FCIE_SHARE_CLK         1 // 1: need to nand_clock_setting at HAL's functions.
#define IF_FCIE_SHARE_IP          1

#define ENABLE_NAND_INTERRUPT_MODE      0
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
#define UNFD_DEBUG_LEVEL          UNFD_DEBUG_LEVEL_HIGH
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
#define reg_allpad_in           GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x50)
#define reg_nf_en               GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x6f)
#define reg_pcmd_pe                GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x0d)
#define reg_pcma_pe                GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x0e)
#define REG_NAND_MODE_MASK 					(BIT6|BIT7)

extern U32 nand_pads_switch(U32 u32EnableFCIE);
//=====================================================
// FCIE4 clock Div
//=====================================================

//#define reg_clk4x_div_en          GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x40)
//#define REG_ANALOG_DELAY_CELL			GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x40)
//#define BIT_ANALOG_DELAY_CELL_MASK		(BIT8|BIT9|BIT10|BIT11|BIT12)

//=====================================================
// FCIE4 Macro Bypass
//=====================================================

#define reg_fcie2macro_sd_bypass  GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x10)

//=====================================================
// Data and Control Strength 
//=====================================================

#define reg_fcie_data_strength  GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x0C)
#define reg_fcie_control_strength  GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x06)


//=====================================================
// set FCIE clock
//=====================================================
#define DEFAULT_REG_CLK   NFIE_REG_CLK_12M
#define DEFAULTtRC		150	//150ns

#define NFIE_CLK_MASK          ((BIT7-1)<<8)
#define NFIE_CLK_24M           ((0<<2)<<8)
#define NFIE_CLK_12M           ((1<<2)<<8)
#define NFIE_CLK_48M           ((2<<2)<<8)
#define NFIE_CLK_86M           ((3<<2)<<8)
#define NFIE_CLK_108M          ((4<<2)<<8)
#define NFIE_CLK_123M          ((5<<2)<<8)
#define NFIE_CLK_144M          ((6<<2)<<8)
#define NFIE_CLK_172M          ((7<<2)<<8)
#define NFIE_CLK_192M          ((8<<2)<<8)
#define NFIE_CLK_216M          ((9<<2)<<8)
#define NFIE_CLK_240M          ((10<<2)<<8)
#define NFIE_CLK_345M          ((11<<2)<<8)
#define NFIE_CLK_432M          ((13<<2)<<8)
#define NFIE_CLK_1200K         ((14<<2)<<8)
#define NFIE_CLK_300K          ((15<<2)<<8)
#define NFIE_CLK_32M           ((16<<2)<<8)
#define NFIE_CLK_36M           ((17<<2)<<8)
#define NFIE_CLK_40M           ((18<<2)<<8)
#define NFIE_CLK_43_2M         ((19<<2)<<8)
#define NFIE_CLK_54M           ((20<<2)<<8)
#define NFIE_CLK_62M           ((21<<2)<<8)
#define NFIE_CLK_72M           ((22<<2)<<8)
#define NFIE_CLK_80M           ((23<<2)<<8)

#define DECIDE_CLOCK_BY_NAND	1

#define NFIE_CLK_TABLE_CNT	15
#define NFIE_CLK_TABLE	{NFIE_CLK_300K, NFIE_CLK_1200K, NFIE_CLK_12M, \
							NFIE_CLK_24M, NFIE_CLK_32M, NFIE_CLK_36M, NFIE_CLK_40M, \
							NFIE_CLK_43_2M, NFIE_CLK_48M, NFIE_CLK_54M, NFIE_CLK_62M, \
							NFIE_CLK_72M, NFIE_CLK_80M, NFIE_CLK_86M, NFIE_CLK_108M	}

#define NFIE_CLK_TABLE_STR	{	"300K", "1200K", "12M",\
							"24M", "32", "36M", "40M", \
							"43.2M", "48M", "54M", "62M", \
							"72M", "80M", "86M", "108M"	}

#define NFIE_300K_VALUE              300*1000
#define NFIE_1200K_VALUE             1200*1000
#define NFIE_12M_VALUE               12*1000*1000
#define NFIE_24M_VALUE             	 24*1000*1000
#define NFIE_32M_VALUE               32*1000*1000
#define NFIE_36M_VALUE               36*1000*1000
#define NFIE_40M_VALUE               40*1000*1000
#define NFIE_43_2M_VALUE             43200*1000
#define NFIE_48M_VALUE               48*1000*1000
#define NFIE_54M_VALUE               54*1000*1000
#define NFIE_62M_VALUE               62*1000*1000
#define NFIE_72M_VALUE               72*1000*1000
#define NFIE_80M_VALUE               80*1000*1000
#define NFIE_86M_VALUE               86*1000*1000
#define NFIE_108M_VALUE              108*1000*1000
#define NFIE_123M_VALUE              123*1000*1000
#define NFIE_144M_VALUE              144*1000*1000
#define NFIE_172M_VALUE              172*1000*1000
#define NFIE_192M_VALUE              192*1000*1000
#define NFIE_216M_VALUE              216*1000*1000
#define NFIE_240M_VALUE              240*1000*1000
#define NFIE_345M_VALUE              345*1000*1000
#define NFIE_432M_VALUE              432*1000*1000

#define NFIE_CLK_VALUE_TABLE	{	NFIE_300K_VALUE, NFIE_1200K_VALUE, NFIE_12M_VALUE, \
							NFIE_24M_VALUE, NFIE_32M_VALUE, NFIE_36M_VALUE, NFIE_40M_VALUE, \
							NFIE_43_2M_VALUE, NFIE_48M_VALUE, NFIE_54M_VALUE, NFIE_62M_VALUE, \
							NFIE_72M_VALUE, NFIE_80M_VALUE, NFIE_86M_VALUE,	NFIE_108M_VALUE}


/*Define 1 cycle Time for each clock note: define value must be the (real value -1)*/
#define NFIE_1T_300K		3332
#define NFIE_1T_1200K		832
#define NFIE_1T_12M			83
#define NFIE_1T_24M			41
#define NFIE_1T_32M			31
#define NFIE_1T_36M			27
#define NFIE_1T_40M			25
#define NFIE_1T_43_2M		23
#define NFIE_1T_48M			20
#define NFIE_1T_54M			18
#define NFIE_1T_62M			16
#define NFIE_1T_72M			13
#define NFIE_1T_80M			12
#define NFIE_1T_86M			11
#define NFIE_1T_108M		9

#define NFIE_1T_TABLE	{	NFIE_1T_300K, NFIE_1T_1200K, NFIE_1T_12M, \
							NFIE_1T_24M, NFIE_1T_32M, NFIE_1T_36M, NFIE_1T_40M, \
							NFIE_1T_43_2M, NFIE_1T_48M, NFIE_1T_54M, NFIE_1T_62M, \
							NFIE_1T_72M, NFIE_1T_80M, NFIE_1T_86M, NFIE_1T_108M	}

/*====================================for 4 xclock==========================================*/

#define NFIE_4CLK_TABLE_CNT	13
#define NFIE_4CLK_TABLE	{	NFIE_CLK_12M, \
							NFIE_CLK_24M, NFIE_CLK_32M, NFIE_CLK_36M, NFIE_CLK_40M, \
							NFIE_CLK_43_2M, NFIE_CLK_48M, NFIE_CLK_54M, NFIE_CLK_62M, \
							NFIE_CLK_72M, NFIE_CLK_80M, NFIE_CLK_86M, NFIE_CLK_108M	}

#define NFIE_4CLK_TABLE_STR	{	"12M",\
							"24M", "32M", "36M", "40M", \
							"43.2", "48M", "54M", "62M", \
							"72M", "80M", "86M", "108M"	}

#define NFIE_4CLK_VALUE_TABLE	{ NFIE_12M_VALUE, \
							NFIE_24M_VALUE, NFIE_32M_VALUE, NFIE_36M_VALUE, NFIE_40M_VALUE, \
							NFIE_43_2M_VALUE, NFIE_48M_VALUE, NFIE_54M_VALUE, NFIE_62M_VALUE, \
							NFIE_72M_VALUE, NFIE_80M_VALUE, NFIE_86M_VALUE, NFIE_108M_VALUE}

#define NFIE_4CLK_1T_TABLE {NFIE_1T_12M, \
							NFIE_1T_24M, NFIE_1T_32M, NFIE_1T_36M, NFIE_1T_40M, \
							NFIE_1T_43_2M, NFIE_1T_48M, NFIE_1T_54M, NFIE_1T_62M, \
							NFIE_1T_72M, NFIE_1T_80M, NFIE_1T_86M, NFIE_1T_108M	}

#define DUTY_CYCLE_PATCH            0 // 1: to enlarge low width for tREA's worst case of 25ns
#if DUTY_CYCLE_PATCH
#define FCIE3_SW_DEFAULT_CLK        NFIE_CLK_86M
#define FCIE_REG41_VAL              ((2<<9)|(2<<3)) // RE,WR pulse, Low:High=3:1
#define REG57_ECO_FIX_INIT_VALUE    0
#else
#define FCIE3_SW_DEFAULT_CLK        NFIE_CLK_54M
#define FCIE_REG41_VAL              0               // RE,WR pulse, Low:High=1:1
#define REG57_ECO_FIX_INIT_VALUE    BIT_NC_LATCH_DATA_1_0_T // delay 1.0T
#endif
#define FCIE3_SW_SLOWEST_CLK        NFIE_CLK_12M

#define NAND_SEQ_ACC_TIME_TOL       16 //in unit of ns

//#define reg_ckg_fcie            GET_REG_ADDR(MPLL_CLK_REG_BASE_ADDR, 0x64)
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

