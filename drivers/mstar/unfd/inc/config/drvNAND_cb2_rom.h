#ifndef __UNFD_CB2_ROM_H__
#define __UNFD_CB2_ROM_H__


#define IF_IP_VERIFY             1 // [CAUTION]: to verify IP and HAL code, defaut 0

//-------------------------------------------
#include "UPD_PRG.h"
//#include "bootrom.h"
#include "drvtimer.h"
#include "drvuart.h"
#include <string.h> // memset
#include <stdio.h>  // printf

//-------------------------------------------
#define REG_OFFSET_SHIFT_BITS    2

#define NAND_REG(Reg_Addr)                  (*(volatile U16*)(Reg_Addr))
#define NAND_GET_REG_ADDR(x, y)             ((x)+((y) << REG_OFFSET_SHIFT_BITS))

#define REG_WRITE_UINT16(reg_addr, val)      NAND_REG((reg_addr)) = (val)
#define REG_READ_UINT16(reg_addr, val)       val = NAND_REG((reg_addr))
#define REG_SET_BITS_UINT16(reg_addr, val)   NAND_REG((reg_addr)) |= (val)
#define REG_CLR_BITS_UINT16(reg_addr, val)   NAND_REG((reg_addr)) &= ~(val)
#define REG_W1C_BITS_UINT16(reg_addr, val)   REG_WRITE_UINT16((reg_addr), NAND_REG((reg_addr))&(val))


//#define RIU_BASE                0xA0000000

#define REG_BANK_CHIPTOP        0xF00
#define REG_BANK_PMUTOP         0x1F80
#ifndef CHIPTOP_BASE
#define CHIPTOP_BASE            NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_CHIPTOP)
#endif
#define PMUTOP_BASE             NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_PMUTOP)

#define REG_BANK_FCIE0          0x1000 // TVTool: 20h
#define REG_BANK_FCIE1          0x1080 // TVTool: 21h
#define REG_BANK_FCIE2          0x1100 // TVTool: 22h
#define REG_BANK_FCIE3          0x1180 // TVTool: 23h

#define FCIE0_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE0) /* 0xA0004000 */
#define FCIE1_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE1) /* 0xA0004200 */
#define FCIE2_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE2) /* 0xA0004400 */
#define FCIE3_BASE              NAND_GET_REG_ADDR(RIU_BASE, REG_BANK_FCIE3) /* 0xA0004600 */

#define FCIE_REG_BASE_ADDR      FCIE0_BASE
#define FCIE_REG_BASE1_ADDR     FCIE1_BASE
#define FCIE_NC_CIFD_BASE       FCIE2_BASE
#define FCIE_NC_CIFD_BASE_2     FCIE3_BASE

#define miu_ecc_patch           BIT1 // NC_REG_2Fh
#define FCIE_REG41_VAL          0

#define MIU_CHECK_LAST_DONE     1
#define NC_REG_MIU_LAST_DONE	NC_MIE_EVENT
//-------------------------------------------
#define NC_SEL_FCIE3            1
#if NC_SEL_FCIE3
#include "drvNAND_reg_v3.h"
#else
#error "Error! no FCIE registers selected."
#endif

#if IF_IP_VERIFY
// select a default NAND chip for IP_VERIFY or NAND_PROGRAMMER
#define HY27UF082G2B           1
//#define P1UAGA30AT             1 
//#define H27UBG8T2A             1  
//#define H27UAG8T2A             1
//#define H27UF081G2A            1
//#define K511F12ACA			 1
//#define TY8A0A111162KC40       1
//#define K522H1GACE             1
//#define TY890A111229KC40       1
//#define H8ACU0CE0DAR           1
//#define H9LA25G25HAMBR         1
//#define TY8A0A111178KC4        1
#include "drvNAND_device.h"

#define FCIE_POWER_SAVE_MODE     0
#endif

#define IF_FCIE_SHARE_PINS           0 // 1: need to nand_pads_switch at HAL's functions.
#define IF_FCIE_SHARE_CLK            1 // 1: need to nand_clock_setting at HAL's functions.
#define IF_FCIE_SHARE_IP             1

#define ENABLE_NAND_INTERRUPT_MODE   0

#define REG50_ECC_CTRL_INIT_VALUE    0//(BIT_NC_SHARE_PAD_EN)
#define REG30_TEST_MODE_INIT_VALUE   0x5800

#define UNFD_ST_PLAT                 0x80000000

#undef __VER_UNFD_FTL__ // no FTL

//=====================================================
// Nand Driver configs
//=====================================================
#define NAND_BUF_USE_STACK       0 // temporary buffer in stack (1) or directly assigned address (0)
#define NAND_ENV_FPGA            1 //BD_FPGA
#define NAND_ENV_ASIC            2 //BD_ASIC
#if defined(__FPGA_MODE__)// && 0!=__FPGA_MODE__
#define NAND_DRIVER_ENV          NAND_ENV_FPGA
#else
#define NAND_DRIVER_ENV          NAND_ENV_ASIC
#endif

#if defined(__ASIC_SIM__)// && __ASIC_SIM__
#define NAND_DRIVER_FOR_ROM_SIM    1
#else
#define NAND_DRIVER_FOR_ROM_SIM    0
#endif


#define UNFD_CACHE_LINE   0x20
//=====================================================
// tool-chain attributes
//=====================================================
#define UNFD_PACK0        __packed         
#define UNFD_PACK1        
#define UNFD_ALIGN0
#define UNFD_ALIGN1       //__attribute__((aligned(UNFD_CACHE_LINE)))


//=====================================================
// debug option
//=====================================================
#define NAND_TEST_IN_DESIGN        0       /* [CAUTION] */

#if NAND_DRIVER_ENV == NAND_ENV_ASIC
#undef  NAND_TEST_IN_DESIGN
#define NAND_TEST_IN_DESIGN        0
#endif

#if NAND_TEST_IN_DESIGN
#define REG30_TEST_MODE_INIT_VALUE   0x5800
#endif

#if defined(BOOTROM_VERSION_FPGA)
#define NAND_DEBUG_MSG             1
#else
#define NAND_DEBUG_MSG             0
#endif

#if IF_IP_VERIFY
#undef  NAND_DEBUG_MSG
#define NAND_DEBUG_MSG             1
#endif

/* Define trace levels. */
#define UNFD_DEBUG_LEVEL_ERROR    (1)    /* Error condition debug messages. */
#define UNFD_DEBUG_LEVEL_WARNING  (2)    /* Warning condition debug messages. */
#define UNFD_DEBUG_LEVEL_HIGH     (3)    /* Debug messages (high debugging). */
#define UNFD_DEBUG_LEVEL_MEDIUM   (4)    /* Debug messages. */
#define UNFD_DEBUG_LEVEL_LOW      (5)    /* Debug messages (low debugging). */

/*
 * Higer debug level means more verbose
 */
#ifndef UNFD_DEBUG_LEVEL
#define UNFD_DEBUG_LEVEL         UNFD_DEBUG_LEVEL_LOW//UNFD_DEBUG_LEVEL_WARNING
#endif

#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
#define nand_printf(x,y)    uart_write_string(x)
#define nand_debug(dbg_lv, tag, str, ...)         \
	do {                                          \
		if (dbg_lv > UNFD_DEBUG_LEVEL)            \
			break;                                \
		else                                      \
			nand_printf(str, 0);                  \
	}while(0)

#else /* NAND_DEBUG_MSG */
#define nand_printf(...)
#define nand_debug(enable, tag, str, ...)      {}
#endif /* NAND_DEBUG_MSG */

extern void nand_reset_WatchDog(void);
extern void nand_set_WatchDog(U8 u8_IfEnable);

#define nand_die(msg) nand_stop()//SYS_FAIL(""msg);

#define nand_stop() \
	while(1)  nand_reset_WatchDog();
	
//=====================================================
// HW Timer for Delay
//=====================================================

#define HW_TIMER_DELAY_1us	    ((TIMER_CLOCK>>20)+1)
#define HW_TIMER_DELAY_10us	    10*HW_TIMER_DELAY_1us
#define HW_TIMER_DELAY_100us	100*HW_TIMER_DELAY_1us
#define HW_TIMER_DELAY_1ms	    (1000 * HW_TIMER_DELAY_1us)
#define HW_TIMER_DELAY_5ms	    (5    * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_10ms	    (10   * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_100ms	(100  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_500ms	(500  * HW_TIMER_DELAY_1ms)
#define HW_TIMER_DELAY_1s	    (1000 * HW_TIMER_DELAY_1ms)

extern void loop_delay_timer(U32 ticks);
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

#define FCIE3_SW_DEFAULT_CLK   NFIE_CLK_24M//NFIE_CLK_32M
#define FCIE3_SW_SLOWEST_CLK   NFIE_REG_CLK_5_3M

#define NAND_SEQ_ACC_TIME_TOL  10 //in unit of ns

extern U32  nand_clock_setting(U32 u32ClkParam);

//=====================================================
// transfer DMA Address
//=====================================================
#define MIU_BUS_WIDTH_BITS       3 // 8 bytes aligned, FCIE3 don't care

/* --------------------------------------------
 * Important:
 *  - The following buffers should be large enough
 *    for a whole NAND block.
 *  - And sould be software kernel safe.
 */
#if IF_IP_VERIFY
#define DMA_W_ADDR       0x24000000
#define DMA_R_ADDR       0x24800000
#define DMA_W_SPARE_ADDR 0x25000000
#define DMA_R_SPARE_ADDR 0x25100000
#define DMA_BAD_BLK_BUF  0x25200000
#else
#define DMA_W_ADDR       0x20400000
#define DMA_R_ADDR       0x20500000
#define DMA_W_SPARE_ADDR 0x21200000
#define DMA_R_SPARE_ADDR 0x21300000
#define DMA_BAD_BLK_BUF  0x21400000
#endif


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

extern U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt);
extern void *drvNAND_get_DrvContext_address(void);
extern void *drvNAND_get_DrvContext_PartInfo(void);
extern U32 nand_config_clock(U16 u16_SeqAccessTime);
extern  U32 NC_PlatformInit(void);

#endif /* __UNFD_CB2_ROM_H__ */
