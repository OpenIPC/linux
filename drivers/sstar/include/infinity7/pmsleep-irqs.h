/*
 * pmsleep-irqs.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

/*  Not used in dtsi,
if need to get the interrupt number for request_irq(), use gpio_to_irq() to obtain irq number.
Or manual calculate the number is
GIC_SGI_NR+GIC_PPI_NR+GIC_SPI_ARM_INTERNAL_NR+GIC_SPI_MS_IRQ_NR+GIC_SPI_MS_FIQ_NR+X=160+X   */
/* MS_PM_SLEEP_FIQ 0-31 */
#define PMSLEEP_FIQ_START     2
#define INT_PMSLEEP_SAR_GPIO0 (PMSLEEP_FIQ_START + 23)
#define INT_PMSLEEP_SAR_GPIO1 (PMSLEEP_FIQ_START + 24)
#define INT_PMSLEEP_SAR_GPIO2 (PMSLEEP_FIQ_START + 25)
#define INT_PMSLEEP_SAR_GPIO3 (PMSLEEP_FIQ_START + 26)

#define PMSLEEP_FIQ_END (PMSLEEP_FIQ_START + 27)
#define PMSLEEP_FIQ_NR  (PMSLEEP_FIQ_END - PMSLEEP_FIQ_START)

#define INT_PMSLEEP_INVALID 0xFF

#define PMSLEEP_IRQ_START        PMSLEEP_FIQ_END
#define INT_PMSLEEP_IRQ_DUMMY_00 (PMSLEEP_IRQ_START + 0)
#define INT_PMSLEEP_IRQ_SAR      (PMSLEEP_IRQ_START + 1)
#define INT_PMSLEEP_IRQ_WOL      (PMSLEEP_IRQ_START + 2)
#define INT_PMSLEEP_IRQ_DUMMY_03 (PMSLEEP_IRQ_START + 3)
#define INT_PMSLEEP_IRQ_RTC      (PMSLEEP_IRQ_START + 4)
#define INT_PMSLEEP_IRQ_DUMMY_05 (PMSLEEP_IRQ_START + 5)
#define INT_PMSLEEP_IRQ_DUMMY_06 (PMSLEEP_IRQ_START + 6)
#define INT_PMSLEEP_IRQ_DUMMY_07 (PMSLEEP_IRQ_START + 7)
#define PMSLEEP_IRQ_END          (PMSLEEP_IRQ_START + 8)
#define PMSLEEP_IRQ_NR           (PMSLEEP_IRQ_END - PMSLEEP_IRQ_START)
