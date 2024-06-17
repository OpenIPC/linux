
#ifndef __SAR_ADC_DEV_H__
#define __SAR_ADC_DEV_H__

#include "sar_adc_drv.h"
#include <linux/io.h>

#ifdef __SAR_ADC_DEV_C__
  #define SAR_ADC_DEV_EXT
#else
  #define SAR_ADC_DEV_EXT extern
#endif



/* function bit */

#define SAR_ADC_RAW_REG_RD(base, offset)         (ioread32((base)+(offset)))
#define SAR_ADC_RAW_REG_WR(base, offset, val)    (iowrite32(val, (base)+(offset)))

SAR_ADC_DEV_EXT unsigned long jiffies_diff(unsigned long a, unsigned long b);

SAR_ADC_DEV_EXT void SAR_ADC_ClearInt(unsigned int base);
SAR_ADC_DEV_EXT unsigned int SAR_ADC_GetData(unsigned int base);

#endif /*__SAR_ADC_DEV_H__*/

