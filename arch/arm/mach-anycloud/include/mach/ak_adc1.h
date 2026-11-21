/*
 * AK39 on-chip SAR-ADC1 (AIN0/AIN1/BAT) raw access.
 */
#ifndef __MACH_AK_ADC1_H
#define __MACH_AK_ADC1_H

#include <mach/map.h>

#define RESET_CTRL_REG		(AK_VA_SYSCTRL + 0x20)
#define AD_DA_CLK1_REG		(AK_VA_SYSCTRL + 0x0C)
#define SAR_ADC_CFG_REG		(AK_VA_SYSCTRL + 0x98)
#define SAR_IF_CFG_REG		(AK_VA_SYSCTRL + 0x5C)
#define SAR_TIMING_CFG_REG	(AK_VA_SYSCTRL + 0x60)
#define SAR_THRESHOLD_REG	(AK_VA_SYSCTRL + 0x64)
#define SAR_IF_SMP_DAT_REG	(AK_VA_SYSCTRL + 0x68)
#define SAR_IF_INT_STATUS_REG	(AK_VA_SYSCTRL + 0x6C)
#define SAR_ANALOG_CTRL2_REG	(AK_VA_SYSCTRL + 0xA0)	/* EV300 */

#define ADC1_MAIN_CLK		(12000000)
#define ADC1_DEFAULT_CLK	(1500000)	/* BAT channel only works at 1.5MHz */
#define DEFAULT_SAMPLE		(1000)
#define AK_AVCC			(3300)		/* AVCC, mV */

/* SAR-ADC1 channel numbering. */
#define AK_ADC1_AD0		(0)
#define AK_ADC1_AD1		(1)
#define AK_ADC1_BAT		(2)

int adc1_init(void);
unsigned long adc1_read_channel(int channel);

/*
 * AK3918EV300: one conversion at a time, started and polled, 12 bits
 * against a 3.0 V reference. AIN0 only, the one input whose analog enable
 * is known. Sleeps; -ETIMEDOUT if the converter never says ready.
 */
#define AK_ADC1_EV300_VREF	(3000)	/* mV */
int adc1_ev300_init(void);
int adc1_ev300_read(int channel, unsigned int *millivolts);

#define adc1_read_bat()		adc1_read_channel(AK_ADC1_AD0)
#define adc1_read_ad5()		adc1_read_channel(AK_ADC1_AD1)

#endif /* __MACH_AK_ADC1_H */
