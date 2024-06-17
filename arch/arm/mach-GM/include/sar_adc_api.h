#ifndef __SAR_ADC_IOCTL_H__
#define __SAR_ADC_IOCTL_H__

#include <linux/ioctl.h>

typedef struct sar_adc_data_tag{
    unsigned int adc_val;
    int status;
}sar_adc_pub_data;

#define KEY_IN      1
#define KEY_REPEAT  2
#define KEY_XAIN_0  4
#define KEY_XAIN_1  8
#define KEY_XAIN_2  16

#define SAR_ADC_MAGIC 'S'
#define SAR_ADC_KEY_ADC_DIRECT_READ              _IOR(SAR_ADC_MAGIC, 1, int) 
#define SAR_ADC_KEY_SET_XGAIN_NUM                _IOWR(SAR_ADC_MAGIC, 2, int) 
#define SAR_ADC_KEY_SET_REPEAT_DURATION          _IOWR(SAR_ADC_MAGIC, 3, int) 

#endif //end of __SAR_ADC_IOCTL_H__


