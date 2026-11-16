/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Device Tree clock bindings for Ingenic T23 SoC
 * Copyright (C) 2026 Difan Zhang
 */

#ifndef __DT_BINDINGS_CLOCK_INGENIC_T23_CGU_H__
#define __DT_BINDINGS_CLOCK_INGENIC_T23_CGU_H__

/* Fixed / Root Clocks */
#define CLK_EXT             0
#define CLK_RTC             1
#define CLK_APLL            2
#define CLK_MPLL            3

/* CPCCR Core Dividers */
#define CLK_SCLKA           4
#define CLK_CCLK            5
#define CLK_L2CLK           6
#define CLK_H0CLK           7
#define CLK_H2CLK           8
#define CLK_PCLK            9

/* Peripheral Gates (CLKGR - Bank 0) */
#define CLK_EFUSE           10
#define CLK_OTG             11
#define CLK_MSC0            12
#define CLK_MSC1            13
#define CLK_SSI0            14
#define CLK_I2C0            15
#define CLK_I2C1            16
#define CLK_I2C2            17
#define CLK_AIC             18
#define CLK_SADC            19
#define CLK_UART0           20
#define CLK_UART1           21
#define CLK_UART2           22
#define CLK_HASH            23
#define CLK_SFC             24
#define CLK_PDMA            25
#define CLK_ISP             26
#define CLK_LCD             27
#define CLK_CSI             28
#define CLK_RISCV           29
#define CLK_RSA             30
#define CLK_DES             31
#define CLK_TCU             32
#define CLK_DDR             33

/* Peripheral Gates (CLKGR1 - Bank 1) */
#define CLK_AVPU            34
#define CLK_DTRNG           35
#define CLK_IPU             36
#define CLK_IVDC            37
#define CLK_GMAC            38
#define CLK_AES             39
#define CLK_AHB0            40
#define CLK_SYS_OST         41
#define CLK_APB0            42
#define CLK_CPU             43
#define CLK_MSC             44

/* Composite Peripheral Clocks (CDRs) */
#define CLK_CGU_MSC_MUX     45
#define CLK_CGU_ISP         46
#define CLK_CGU_CIM         47
#define CLK_CGU_SSI         48
#define CLK_CGU_SFC         49
#define CLK_CGU_I2S_SPK     50
#define CLK_CGU_I2S_MIC     51
#define CLK_CGU_MSC1        52
#define CLK_CGU_MSC0        53
#define CLK_CGU_LPC         54
#define CLK_CGU_VPU         55
#define CLK_CGU_DDR         56
#define CLK_CGU_MACPHY      57
#define CLK_CGU_RSA         58

/* Core Muxes (internal to CGU) */
#define CLK_CPUMUX          59
#define CLK_AHB2PMUX        60

#define CLK_NR_CLKS         61

#endif /* __DT_BINDINGS_CLOCK_INGENIC_T23_CGU_H__ */
