/*------------------------------------------------------------------------------
	Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

#ifndef __ARCH_ARM_ASM_IRQS_H
#define __ARCH_ARM_ASM_IRQS_H
/*-----------------------------------------------------------------------------
    Include Files
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    Constant
-------------------------------------------------------------------------------*/

#define INT_START                   0

/* FIQ Definition */
#define FIQ_START                  	INT_START
#define INT_FIQ_WDT         	    (FIQ_START + 0)
#define INT_FIQ_TIMER0            	(FIQ_START + 1)
#define INT_FIQ_TIMER1              (FIQ_START + 2)
#define INT_FIQ_TIMER2              (FIQ_START + 3)
#define INT_FIQ_TIMER3              (FIQ_START + 4)
#define INT_FIQ_TIMER4              (FIQ_START + 5)
#define INT_FIQ_GDMA                (FIQ_START + 6)
#define INT_FIQ_GPS_DCD             (FIQ_START + 7)
#define INT_FIQ_DUMMY08             (FIQ_START + 8)
#define INT_FIQ_SARADC             	(FIQ_START + 9)
#define INT_FIQ_DUMMY10             (FIQ_START + 10)
#define INT_FIQ_DUMMY11             (FIQ_START + 11)
#define INT_FIQ_DUMMY12             (FIQ_START + 12)
#define INT_FIQ_DUMMY13             (FIQ_START + 13)
#define INT_FIQ_DUMMY14             (FIQ_START + 14)
#define INT_FIQ_DUMMY15             (FIQ_START + 15)
#define INT_FIQ_DUMMY16             (FIQ_START + 16)
#define INT_FIQ_DUMMY17             (FIQ_START + 17)
#define INT_FIQ_DUMMY18             (FIQ_START + 18)
#define INT_FIQ_DUMMY19             (FIQ_START + 19)
#define INT_FIQ_DUMMY20             (FIQ_START + 20)
#define INT_FIQ_DUMMY21             (FIQ_START + 21)
#define INT_FIQ_DUMMY22             (FIQ_START + 22)
#define INT_FIQ_DUMMY23             (FIQ_START + 23)
#define INT_FIQ_DUMMY24             (FIQ_START + 24)
#define INT_FIQ_DUMMY25             (FIQ_START + 25)
#define INT_FIQ_DUMMY26             (FIQ_START + 26)
#define INT_FIQ_DUMMY27            	(FIQ_START + 27)
#define INT_FIQ_DUMMY28             (FIQ_START + 28)
#define INT_FIQ_DUMMY29             (FIQ_START + 29)
#define INT_FIQ_DUMMY30             (FIQ_START + 30)
#define INT_FIQ_DUMMY31             (FIQ_START + 31)
#define FIQ_END                     (FIQ_START + 32)

/* IRQ Definition */
#define IRQ_START                  	(FIQ_END)
#define INT_IRQ_PMU             	(IRQ_START + 0)
#define INT_FIQ_DUMMY01            	(IRQ_START + 1)
#define INT_IRQ_AUDIO_2            	(IRQ_START + 2)
#define INT_IRQ_AUDIO_1            	(IRQ_START + 3)
#define INT_IRQ_GOP                 (IRQ_START + 4)
#define INT_IRQ_IPM               	(IRQ_START + 5)
#define INT_IRQ_DISP             	(IRQ_START + 6)
#define INT_IRQ_JPD                	(IRQ_START + 7)
#define INT_IRQ_UHC                	(IRQ_START + 8)
#define INT_IRQ_OTG                 (IRQ_START + 9)
#define INT_IRQ_USB                 (IRQ_START + 10)
#define INT_IRQ_GPS_SIGP            (IRQ_START + 11)
#define INT_IRQ_GE                  (IRQ_START + 12)
#define INT_IRQ_UART0               (IRQ_START + 13)
#define INT_IRQ_SDIO                (IRQ_START + 14)
#define INT_IRQ_FCIE               	(IRQ_START + 15)
#define INT_IRQ_IIC                 (IRQ_START + 16)
#define INT_IRQ_MIU                 (IRQ_START + 17)
#define INT_IRQ_DW_SSI_DMA          (IRQ_START + 18)
#define INT_IRQ_DW_SSI            	(IRQ_START + 19)
#define INT_IRQ_UART3              	(IRQ_START + 20)
#define INT_IRQ_UART2              	(IRQ_START + 21)
#define INT_IRQ_UART1               (IRQ_START + 22)
#define INT_IRQ_UART3_DMA           (IRQ_START + 23)
#define INT_IRQ_COMBO_TIMER        	(IRQ_START + 24)
#define INT_IRQ_CPUIF               (IRQ_START + 25)
#define INT_IRQ_UART2_DMA           (IRQ_START + 26)
#define INT_IRQ_GPS_DCD            	(IRQ_START + 27)
#define INT_IRQ_COMBO_WDT          	(IRQ_START + 28)
#define INT_IRQ_AUX_PENDET          (IRQ_START + 29)
#define INT_IRQ_ADCDONE            	(IRQ_START + 30)
#define INT_IRQ_KEY0                (IRQ_START + 31)
#define IRQ_END						(IRQ_START + 32)

#define PMU_START					(IRQ_END)
#define INT_PMU_BUCK2_OCP_DEBOUNCE 	(PMU_START + 0)
#define INT_PMU_BUCK1_OCP_DEBOUNCE	(PMU_START + 1)
#define INT_PMU_OVER_VOLTAGE		(PMU_START + 2)
#define INT_PMU_OTP_DEBOUNCE   	    (PMU_START + 3)
#define INT_PMU_LDO_ERR				(PMU_START + 4)
#define INT_PMU_IRQ_PGA				(PMU_START + 5)
#define INT_PMU_IRQ_OCP				(PMU_START + 6)
#define INT_PMU_NO_BAT				(PMU_START + 7)
#define INT_PMU_BAT_LOW				(PMU_START + 8)
#define INT_PMU_ADAPTOR_PLUG		(PMU_START + 9)
#define INT_PMU_ADAPTOR_UNPLUG		(PMU_START + 10)
#define INT_PMU_KEYOFF_PULSE		(PMU_START + 11)
#define INT_PMU_ONOFF_PULSE			(PMU_START + 12)
#define INT_PMU_RTC				    (PMU_START + 13)
#define INT_PMU_TOUCH_DETECT    	(PMU_START + 14)
#define INT_PMU_SAR_KEY_DETECT  	(PMU_START + 15)
#define INT_PMU_GPIO00			    (PMU_START + 16)
#define INT_PMU_GPIO01			    (PMU_START + 17)
#define INT_PMU_GPIO02			    (PMU_START + 18)
#define INT_PMU_GPIO03			    (PMU_START + 19)
#define INT_PMU_GPIO04				(PMU_START + 20)
#define INT_PMU_GPIO05				(PMU_START + 21)
#define INT_PMU_GPIO06				(PMU_START + 22)
#define INT_PMU_WORKLED				(PMU_START + 23)
#define INT_PMU_DUMMY24				(PMU_START + 24)
#define INT_PMU_DUMMY25			    (PMU_START + 25)
#define INT_PMU_DUMMY26		        (PMU_START + 26)
#define INT_PMU_DUMMY27		        (PMU_START + 27)
#define INT_PMU_SD_DETECT           (PMU_START + 28)
#define INT_PMU_DUMMY29		        (PMU_START + 29)
#define INT_PMU_RIU_CKSUM_PROT		(PMU_START + 30)
#define INT_PMU_DUMMY31		        (PMU_START + 31)
#define PMU_END						(PMU_START + 32)

#define CHICAGO_NR_IRQS			    (PMU_END)


#endif // __ARCH_ARM_ASM_IRQS_H
