/*

    @file       dai_int.h
    @ingroup    mIDrvAud_DAI

    @brief      DAI internal header file, This file is the header file that define register for DAI module

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/


#ifndef _DAI_INT_H
#define _DAI_INT_H

#ifdef __KERNEL__
#include <mach/rcw_macro.h>
//#include <mach/ioaddress.h>
//#include "kwrap/type.h"
#include <mach/fmem.h>
#define dma_get_noncache_addr(parm) parm
#define dma_get_phy_addr(parm) fmem_lookup_pa(parm)
#elif defined(__FREERTOS)
#include <rcw_macro.h>
#include "kwrap/type.h"
#include "plat/interrupt.h"
#define dma_get_noncache_addr(parm) parm
#define dma_get_phy_addr(parm) (parm)
#endif
#include "dai.h"

/*
    @addtogroup mIDrvAud_DAI
*/
//@{

#define DAI_DBG_MSG                     0

#ifdef __KERNEL__
extern UINT32 _DAI_REG_BASE_ADDR[1];
#define DAI_SETREG(ofs, value)       iowrite32(value, (void *)(_DAI_REG_BASE_ADDR[0] + (ofs)))
#define DAI_GETREG(ofs)              ioread32((void *)(_DAI_REG_BASE_ADDR[0] + (ofs)))
#else
#define DAI_SETREG(ofs, value)       OUTW((UINT32 *)(IOADDR_DAI_REG_BASE+(ofs)), (value))
#define DAI_GETREG(ofs)              INW((UINT32 *)(IOADDR_DAI_REG_BASE+(ofs)))
#endif

#define DAI_I2S_CLK_OFS_MAX    			0xFE


/*
    DAI I2S Frame clock ratio

    @note For dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO)
*/
typedef enum {
	DAI_I2SFRAMECLKR_32BIT,     ///< FrameSync = 32 BitClk. NT96680 valid.
	DAI_I2SFRAMECLKR_48BIT,     ///< FrameSync = 48 BitClk
	DAI_I2SFRAMECLKR_64BIT,     ///< FrameSync = 64 BitClk. NT96680 valid.
	DAI_I2SFRAMECLKR_96BIT,     ///< FrameSync = 96 BitClk
	DAI_I2SFRAMECLKR_128BIT,    ///< FrameSync = 128 BitClk. NT96680 valid.
	DAI_I2SFRAMECLKR_192BIT,    ///< FrameSync = 192 BitClk
	DAI_I2SFRAMECLKR_256BIT,    ///< FrameSync = 256 BitClk. NT96680 valid.

	ENUM_DUMMY4WORD(DAI_I2SFRAMECLKR)
} DAI_I2SFRAMECLKR;


//
// Internal function prototype
//
extern DAI_INTERRUPT    dai_wait_interrupt(DAI_INTERRUPT wait_flag);
extern void             dai_select_pinmux(BOOL enable);
extern void             dai_select_mclk_pinmux(BOOL enable);

extern void             dai_debug(BOOL enable);
extern void             dai_debug_eac(BOOL enable, BOOL mode_ad);
extern void             dai_set_debug_dma_para(UINT32 buf_addr, UINT32 buf_size);
extern BOOL             dai_get_debug_status(void);
extern void             dai_clr_debug_status(void);

extern ER               dai_lock(void);
extern ER               dai_unlock(void);

#ifdef __KERNEL__
extern void dai_tasklet(void);
#endif

//@}

#endif
