/*
 * linux/arch/arm/mach-ak37d/include/mach/irqs.h
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#ifndef __ASM_ARCH_IRQS_H_
#define __ASM_ARCH_IRQS_H_

#define AK_IRQ(x)               (x)

#if !defined(CONFIG_MACH_AK3918AV100) && !defined(CONFIG_MACH_AK3918EV300L) && !defined(CONFIG_MACH_AK3918AV130) \
  && !defined(CONFIG_MACH_KM01A)
/*
 * Main CPU Interrupts
 */
#ifdef CONFIG_MACH_AK37E
#define IRQ_NULL0               AK_IRQ(0)
#else
#define IRQ_MEM                 AK_IRQ(0)
#endif

#define IRQ_CAMERA              AK_IRQ(1)

#ifdef CONFIG_MACH_AK37E
#define IRQ_PDM                 AK_IRQ(2)
#else
#define IRQ_VIDEO_ENCODER       AK_IRQ(2)
#endif

#define IRQ_SYSCTRL             AK_IRQ(3)
#define IRQ_MCI0                AK_IRQ(4)
#define IRQ_MCI1                AK_IRQ(5)

#define IRQ_ADC2                AK_IRQ(6)  ////
#define IRQ_DAC                 AK_IRQ(7)  ////

#define IRQ_SPI0                AK_IRQ(8)
#define IRQ_SPI1                AK_IRQ(9)

#define IRQ_UART0               AK_IRQ(10)
#define IRQ_UART1               AK_IRQ(11)
#define IRQ_L2MEM               AK_IRQ(12)
#define IRQ_TWI0                AK_IRQ(13)
#ifdef CONFIG_MACH_AK37E
#define IRQ_SPI2                AK_IRQ(14)
#else
#define IRQ_IRDA                AK_IRQ(14)
#endif
#define IRQ_GPIO                AK_IRQ(15)
#define IRQ_MAC                 AK_IRQ(16)
#ifdef CONFIG_MACH_AK37E
#define IRQ_L2MEM1              AK_IRQ(17)
#else
#define IRQ_ENCRYTION           AK_IRQ(17)
#endif
#define IRQ_USBOTG_MCU          AK_IRQ(18)
#define IRQ_USBOTG_DMA          AK_IRQ(19)
#define IRQ_TWI1                AK_IRQ(20)

#ifdef CONFIG_MACH_AK37D
#define IRQ_UART3               AK_IRQ(21)
#define IRQ_UART4               AK_IRQ(22)
#define IRQ_VIDEO_DECODER       AK_IRQ(23)
#define IRQ_GUI                 AK_IRQ(24)
#define IRQ_LCD                 AK_IRQ(25)
#define IRQ_NULL1               AK_IRQ(26)
#define IRQ_MCI2                AK_IRQ(27)
#define IRQ_SD_PLUGIN           AK_IRQ(28)
#define IRQ_TWI2                AK_IRQ(29)
#define IRQ_TWI3                AK_IRQ(30)
#define IRQ_NULL2               AK_IRQ(31)
#endif

#ifdef CONFIG_MACH_AK39EV330
#define IRQ_TWI2                AK_IRQ(21)
#define IRQ_SD_PLUGIN           AK_IRQ(22)
#define IRQ_NULL2               AK_IRQ(23)

#endif
#ifdef CONFIG_MACH_AK37E
#define IRQ_UART2               AK_IRQ(21)
#define IRQ_UART3               AK_IRQ(22)
#define IRQ_VIDEO_DECODER       AK_IRQ(23)
#define IRQ_GUI                 AK_IRQ(24)
#define IRQ_LCD                 AK_IRQ(25)
#define IRQ_JPEG                AK_IRQ(26)
#define IRQ_MCI2                AK_IRQ(27)
#define IRQ_SD_PLUGIN           AK_IRQ(28)
#define IRQ_TWI2                AK_IRQ(29)
#define IRQ_TWI3                AK_IRQ(30)
#define IRQ_MAC1                AK_IRQ(31)
#define IRQ_NULL2               AK_IRQ(32) 
#endif

/*
 * System Control Module Sub-IRQs
 */
#define IRQ_SYSCTRL_START       (IRQ_NULL2 + 1)
#define AK_SYSCTRL_IRQ(x)       (IRQ_SYSCTRL_START + (x))

#define IRQ_SARADC              AK_SYSCTRL_IRQ(0)
#define IRQ_TIMER5              AK_SYSCTRL_IRQ(1)
#define IRQ_TIMER4              AK_SYSCTRL_IRQ(2)
#define IRQ_TIMER3              AK_SYSCTRL_IRQ(3)
#define IRQ_TIMER2              AK_SYSCTRL_IRQ(4)
#define IRQ_TIMER1              AK_SYSCTRL_IRQ(5)
#define IRQ_WAKEUP              AK_SYSCTRL_IRQ(6)
#define IRQ_RTC_RDY             AK_SYSCTRL_IRQ(7)
#define IRQ_RTC_ALARM           AK_SYSCTRL_IRQ(8)
#define IRQ_RTC_TIMER           AK_SYSCTRL_IRQ(9)
#define IRQ_RTC_WATCHDOG        AK_SYSCTRL_IRQ(10)

#ifdef CONFIG_MACH_AK37D
#define IRQ_GPI0_AO             AK_SYSCTRL_IRQ(11)
#define IRQ_GPI1_AO             AK_SYSCTRL_IRQ(12)
#define IRQ_GPI2_AO             AK_SYSCTRL_IRQ(13)
#define IRQ_GPI3_AO             AK_SYSCTRL_IRQ(14)
#define IRQ_GPI4_AO             AK_SYSCTRL_IRQ(15)
#define IRQ_BVD_LOW_PWR         AK_SYSCTRL_IRQ(16)
#define IRQ_PMU_RDY             AK_SYSCTRL_IRQ(17)
#endif

#ifdef CONFIG_MACH_AK37D
#define IRQ_TIMER7              AK_SYSCTRL_IRQ(18)
#define IRQ_TIMER6              AK_SYSCTRL_IRQ(19)
#endif

#ifdef CONFIG_MACH_AK39EV330
#define IRQ_TIMER10             AK_SYSCTRL_IRQ(11)
#define IRQ_TIMER9              AK_SYSCTRL_IRQ(12)
#define IRQ_TIMER8              AK_SYSCTRL_IRQ(13)
#define IRQ_TIMER7              AK_SYSCTRL_IRQ(14)
#define IRQ_TIMER6              AK_SYSCTRL_IRQ(15)
#endif

#endif //NOT define CONFIG_MACH_AK3918AV100


/* total irq number */
#ifdef CONFIG_MACH_AK37D
#define NR_IRQS         (IRQ_TIMER6 + 123 + 1)   /* AK37D has 123 gpio irqs */
#endif

#ifdef CONFIG_MACH_AK39EV330
#define NR_IRQS         (IRQ_TIMER6 + 86 + 1)   /* AK39EV330 has 86 gpio irqs */
#endif

#ifdef CONFIG_MACH_AK37E
#define NR_IRQS         (32 + 23 + 97 + 1)     /* AK37E has 97 gpio irqs */
#endif

#if defined(CONFIG_MACH_AK3918AV100) 
 /*
  *  Bank0 size is 32
  *  Bank1 size is 32
  *  GPIO:  AK3918AV1XX contains 83 GPIOs
  *  internal phy: 12 pin
  */
#define NR_IRQS         (32 + 32 + 83 + 12 + 1)
#endif

#if defined(CONFIG_MACH_AK3918EV300L)
 /*
  *  Bank0 size is 32
  *  Bank1 size is 32
  *  GPIO:  AK3918EV300L contains 59 GPIOs
  */
#define NR_IRQS         (32 + 32 + 59 + 1)
#endif

#if defined(CONFIG_MACH_AK3918AV130) 
 /*
  *  Bank0 size is 32
  *  Bank1 size is 32
  *  GPIO:  AK3918AV1XX contains 83 GPIOs
  *  internal phy: 12 pin
  */
#define NR_IRQS         (32 + 32*6 + 83 + 12 + 1)
#endif

#if defined(CONFIG_MACH_KM01A) 
 /*
  *  Bank0 size is 32
  *  Bank1 size is 32
  *  GPIO:  AK3918AV1XX contains 83 GPIOs
  *  internal phy: 12 pin
  */
#define NR_IRQS         (32 + 32 + 83 + 12 + 1)
#endif

#endif  /* __ASM_ARCH_IRQS_H_ */

