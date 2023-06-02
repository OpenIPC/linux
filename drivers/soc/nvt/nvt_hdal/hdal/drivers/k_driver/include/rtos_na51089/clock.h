/**
    Change clock APIs header file

    Change clock APIs header.

    @file       clock.h
    @ingroup    mIDrvSys_CG
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/


#ifndef _CLOCK_H
#define _CLOCK_H

#include <kwrap/nvt_type.h>

/**
    Power Down mode definition

    @note This definition is used for clk_powerdown() & clk_change_cpu_ahb().
*/
typedef enum {
	CLK_PDN_MODE_CLKSCALING,            ///< Clock scaling mode
	CLK_PDN_MODE_SLEEP1,                ///< Sleep mode 1
	CLK_PDN_MODE_SLEEP2,                ///< Sleep mode 2
	CLK_PDN_MODE_SLEEP3,                ///< Sleep mode 3

	CLK_PDN_MODE_SYSOFF_DRAMON,         ///< power off but DRAM keep self refresh
	CLK_PDN_MODE_POWEROFF,              ///< Sleep mode with ALL power off
	CLK_PDN_MODE_SLEEP_DRAM_OFF,	    ///< Power down mode 3 with DRAM power off

	ENUM_DUMMY4WORD(CLK_PDN_MODE)
} CLK_PDN_MODE;

/*
    @name CLKGEN general call back prototype

    @param[in] PLL_PDN_MODE                         Power state for this call back
    @param[in] BOOL                                 enter/exit power state
                                                    - @b TRUE: enter PLL_PDN_CBMODE
                                                    - @b FALSE: exit PLL_PDN_CBMODE

    @return ER
*/
typedef ER(*CLK_CALLBACK_HDL)(CLK_PDN_MODE, BOOL);

/*
    CLKGEN call back list
*/
typedef enum {
	CLK_CALLBACK_ADC_PWRDN,                         //< ADC power down callback
	CLK_CALLBACK_GPIO_PWRDN,                        //< GPIO power down callback
	CLK_CALLBACK_RM_PWRDN,                          //< RM power down callback
	CLK_CALLBACK_RTC_PWRDN,                         //< RTC power down callback
	CLK_CALLBACK_USB_PWRDN,                         //< USB 2.0 power down callback
	CLK_CALLBACK_USB3_PWRDN,                        //< USB 3.0 power down callback

	CLK_CALLBACK_CNT,
	ENUM_DUMMY4WORD(CLK_CALLBACK)
} CLK_CALLBACK;

extern ER clk_set_callback(CLK_CALLBACK callback_id, CLK_CALLBACK_HDL pf_callback);
extern BOOL clk_powerdown(CLK_PDN_MODE mode);
extern void clock_platform_init(void);
extern void clock_platform_uninit(void);
extern BOOL clk_change_cpu_ahb(UINT32 cpu_clk, UINT32 skip_int);
extern BOOL clk_change_apb(UINT32 apb_clk);
extern BOOL clk_change_dma_clk_507MHz(void);


#endif
