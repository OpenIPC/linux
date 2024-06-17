#define __SAR_ADC_DEV_C__

#include <linux/kernel.h>
#include <asm/page.h>
#include <asm/io.h>
#include "sar_adc_dev.h"
#include <linux/delay.h>
#include "platform.h"

unsigned long jiffies_diff(unsigned long a, unsigned long b)
{
    if(a>b)
        return (a-b);
    else
        return (0xFFFFFFFF-b+a);     //the only case is overflow
}

void SAR_ADC_ClearInt(unsigned int base)
{
    SAR_ADC_RAW_REG_WR(base, FTSAR_ADC_INT, ADC_WRAP_CLEAR_INT);
}

unsigned int SAR_ADC_GetData(unsigned int base)
{
	//int ret=0;
	//ADC_WRAP_GET_DATA(base,ret);
	return 0;
}
