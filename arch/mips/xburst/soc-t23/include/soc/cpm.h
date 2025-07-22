/*
 * JZSOC CPM register definition.
 *
 * CPM (Clock reset and Power control Management)
 *
 * Copyright (C) 2019 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __CPM_H__
#define __CPM_H__

#define CPM_CPCCR		(0x00)
#define CPM_CPPCR		(0x0c)
#define CPM_CPAPCR		(0x10)
#define CPM_CPMPCR		(0x14)
#define CPM_CPCSR		(0xd4)
#define CPM_DDRCDR		(0x2c)
#define CPM_VPUCDR		(0x30)
#define CPM_CPPSR		(0x34)
#define CPM_CPSPPR		(0x38)
#define CPM_USBPCR		(0x3C)
#define CPM_USBRDT		(0x40)
#define CPM_USBVBFIL	(0x44)
#define CPM_USBPCR1		(0x48)
#define CPM_RSACDR		(0x4C)
#define CPM_MACCDR		(0x54)
#define CPM_SSICDR		(0x60)
#define CPM_LPCDR		(0x64)
#define CPM_MSC0CDR		(0x68)
#define CPM_MSC1CDR		(0xa4)
#define CPM_I2SSPKCDR	(0x70)
#define CPM_I2SSPKCDR1	(0x78)
#define CPM_SFCCDR		(0x74)
#define CPM_CIMCDR		(0x7c)
#define CPM_ISPCDR		(0x80)
#define CPM_I2SMICCDR	(0x84)
#define CPM_I2SMICCDR1	(0x88)
#define CPM_EXCLKDS		(0x8c)
#define CPM_MPHYC		(0xe8)
#define CPM_INTR		(0xb0)
#define CPM_INTRE		(0xb4)
#define CPM_DRCG		(0xd0)
#define CPM_CPCSR		(0xd4)
#define CPM_CPVPCR		(0xe0)
#define CPM_CPVPACR		(0xe4)
#define CPM_MACPHY		(0xe8)


#define CPM_LCR			(0x04)
/*#define CPM_SPCR0	(0xb8)*/
/* #define CPM_SPCR1	(0xbc) */
/* #define CPM_SSPC	(0xc0) */
/*
#define CPM_PSWC0ST     (0x90)
#define CPM_PSWC1ST     (0x94)
#define CPM_PSWC2ST     (0x98)
#define CPM_PSWC3ST     (0x9c)
*/
#define CPM_CLKGR		(0x20)/* def changed*/
#define CPM_CLKGR1		(0x28)/* def changed*/
#define CPM_SRBC		(0xc4)
#define CPM_MESTSEL		(0xec)
#define CPM_OPCR		(0x24)

#define CPM_MEMCTRL_MA0		(0xf0)
#define CPM_MEMCTRL_MA1	    (0xf4)
#define CPM_MEMCTRL_MA2		(0xf8)
/*
#define CPM_SLBC	(0xc8)
#define CPM_SLPC	(0xcc)
#define CPM_PGR		(0xe4)
*/
#define CPM_RSR		(0x08)


#define LCR_LPM_MASK		(0x3)
#define LCR_LPM_SLEEP		(0x1)

#define CPM_LCR_PD_X2D		(0x1<<31)
#define CPM_LCR_PD_VPU		(0x1<<30)
#define CPM_LCR_PD_MASK		(0x3<<30)
#define CPM_LCR_X2DS 		(0x1<<27)
#define CPM_LCR_VPUS		(0x1<<26)
#define CPM_LCR_STATUS_MASK	(0x3<<26)

#define OPCR_ERCS		(0x1<<2)
#define OPCR_PD			(0x1<<3) //T31 delete
#define OPCR_IDLE		(0x1<<31)

#define CLKGR1_VPU              (0x1<<0)
//#define CLKGR_VPU              (0x1<<19)

#if 0
//#ifdef CONFIG_BOARD_T15_FPGA
#define cpm_inl(x)		0x3
#define cpm_outl(val,off)	do{int tmp = 0; tmp = val; tmp = off;}while(0)
//#define cpm_outl(v,x)		do{}while(0)
#define cpm_clear_bit(off,x)	do{int tmp = 0; tmp = x; tmp = off;}while(0)
#define cpm_set_bit(off,x)	do{int tmp = 0; tmp = x; tmp = off;}while(0)
#define cpm_test_bit(val,off)	1
#else
#define cpm_inl(off)		inl(CPM_IOBASE + (off))
#define cpm_outl(val,off)	outl(val,CPM_IOBASE + (off))
#define cpm_clear_bit(val,off)	do{cpm_outl((cpm_inl(off) & ~(1 << (val))),off);}while(0)
#define cpm_set_bit(val,off)	do{cpm_outl((cpm_inl(off) | (1 << (val))),off);}while(0)
#define cpm_test_bit(val,off)	(cpm_inl(off) & (0x1 << (val)))
#endif

#endif
/* __CPM_H__ */
