/*
    Header file for ADC module register.

    This file is the header file that define register for ADC module.

    @file       adc_reg.h
    @ingroup    mIDrvIO_ADC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/



#ifndef _ADC_REG_H
#define _ADC_REG_H

#include <rcw_macro.h>

//0x00 ADC control Register
#define ADC_CTRL_REG_OFS          0x00
REGDEF_BEGIN(ADC_CTRL_REG)
REGDEF_BIT(AIN0_MODE, 1)      // ADC channel 0 mode
REGDEF_BIT(AIN1_MODE, 1)      // ADC channel 1 mode
REGDEF_BIT(AIN2_MODE, 1)      // ADC channel 2 mode
REGDEF_BIT(AIN3_MODE, 1)      // ADC channel 3 mode
REGDEF_BIT(, 8)               // Reserved
REGDEF_BIT(CLKDIV, 5)         // ADC clock pre-scalar
REGDEF_BIT(, 3)               // Reserved
REGDEF_BIT(SAMPAVG, 2)        // ADC sample to average
REGDEF_BIT(ADC67_SW, 1)       // ADC channel 6,7 function switch
REGDEF_BIT(ADC_EN, 1)         // ADC converter enable bit
REGDEF_BIT(EXTSAMP_CNT, 4)    // Extra ADC sample clock count
REGDEF_BIT(LATCH_CNT, 4)      // Latch value of LATCH_CNT signal
REGDEF_END(ADC_CTRL_REG)

//0x04 ADC one-shot Register
#define ADC_ONESHOT_REG_OFS       0x04
REGDEF_BEGIN(ADC_ONESHOT_REG)
REGDEF_BIT(AIN0_ONESHOT, 1)   // ADC channel 0 one-shot start
REGDEF_BIT(AIN1_ONESHOT, 1)   // ADC channel 1 one-shot start
REGDEF_BIT(AIN2_ONESHOT, 1)   // ADC channel 2 one-shot start
REGDEF_BIT(AIN3_ONESHOT, 1)   // ADC channel 3 one-shot start
REGDEF_BIT(, 28)
REGDEF_END(ADC_ONESHOT_REG)

//0x08 ADC interrupt control Register
#define ADC_INTCTRL_REG_OFS       0x08
REGDEF_BEGIN(ADC_INTCTRL_REG)
REGDEF_BIT(AIN0_INTEN, 1)     // ADC channel 0 data ready interrupt enable
REGDEF_BIT(AIN1_INTEN, 1)     // ADC channel 1 data ready interrupt enable
REGDEF_BIT(AIN2_INTEN, 1)     // ADC channel 2 data ready interrupt enable
REGDEF_BIT(AIN3_INTEN, 1)     // ADC channel 3 data ready interrupt enable
REGDEF_BIT(AINTR0_INTEN, 1)
REGDEF_BIT(AINTR1_INTEN, 1)
REGDEF_BIT(AINTR2_INTEN, 1)
REGDEF_BIT(, 25)
REGDEF_END(ADC_INTCTRL_REG)

//0x0C ADC status Register
#define ADC_STATUS_REG_OFS        0x0C
REGDEF_BEGIN(ADC_STATUS_REG)
REGDEF_BIT(AIN0_DATARDY, 1)   // ADC channel 0 data ready status
REGDEF_BIT(AIN1_DATARDY, 1)   // ADC channel 1 data ready status
REGDEF_BIT(AIN2_DATARDY, 1)   // ADC channel 2 data ready status
REGDEF_BIT(AIN3_DATARDY, 1)   // ADC channel 3 data ready status
REGDEF_BIT(AINTR0, 1)
REGDEF_BIT(AINTR1, 1)
REGDEF_BIT(AINTR2, 1)
REGDEF_BIT(, 25)
REGDEF_END(ADC_STATUS_REG)

//0x10 ADC value trigger Register
#define ADC_TRIGCTRL_REG_OFS        0x10
REGDEF_BEGIN(ADC_TRIGCTRL_REG)
REGDEF_BIT(TRIG0_EN, 1)       // ADC channel 0 value trigger enable
REGDEF_BIT(TRIG1_EN, 1)       // ADC channel 1 value trigger enable
REGDEF_BIT(TRIG2_EN, 1)       // ADC channel 2 value trigger enable
REGDEF_BIT(, 1)
REGDEF_BIT(TRIG0_RANGE, 1)    // ADC channel 0 value trigger range
REGDEF_BIT(TRIG1_RANGE, 1)    // ADC channel 1 value trigger range
REGDEF_BIT(TRIG2_RANGE, 1)    // ADC channel 2 value trigger range
REGDEF_BIT(, 1)
REGDEF_BIT(TRIG0_MODE, 1)     // ADC channel 0 value trigger mode
REGDEF_BIT(TRIG1_MODE, 1)     // ADC channel 1 value trigger mode
REGDEF_BIT(TRIG2_NODE, 1)     // ADC channel 2 value trigger mode
REGDEF_BIT(, 5)
REGDEF_BIT(AINTR0_ID, 2)      // Trigger Channel ID of ADC Value trigger 0
REGDEF_BIT(, 2)
REGDEF_BIT(AINTR1_ID, 2)      // Trigger Channel ID of ADC Value trigger 1
REGDEF_BIT(, 2)
REGDEF_BIT(AINTR2_ID, 2)      // Trigger Channel ID of ADC Value trigger 2
REGDEF_BIT(, 6)
REGDEF_END(ADC_TRIGCTRL_REG)

//0x14 ADC value range Register 0
#define ADC_TRIGVAL0_REG_OFS        0x14
REGDEF_BEGIN(ADC_TRIGVAL0_REG)
REGDEF_BIT(TRIG0_START, 8)    // ADC channel 0 trigger value START
REGDEF_BIT(TRIG0_END, 8)      // ADC channel 0 trigger value END
REGDEF_BIT(, 16)
REGDEF_END(ADC_TRIGVAL0_REG)

//0x18 ADC value range Register 1
#define ADC_TRIGVAL1_REG_OFS        0x18
REGDEF_BEGIN(ADC_TRIGVAL1_REG)
REGDEF_BIT(TRIG1_START, 8)    // ADC channel 1 trigger value START
REGDEF_BIT(TRIG1_END, 8)      // ADC channel 1 trigger value END
REGDEF_BIT(, 16)
REGDEF_END(ADC_TRIGVAL1_REG)

//0x1C ADC value range Register 2
#define ADC_TRIGVAL2_REG_OFS        0x1C
REGDEF_BEGIN(ADC_TRIGVAL2_REG)
REGDEF_BIT(TRIG2_START, 8)    // ADC channel 2 trigger value START
REGDEF_BIT(TRIG2_END, 8)      // ADC channel 2 trigger value END
REGDEF_BIT(, 16)
REGDEF_END(ADC_TRIGVAL2_REG)

//0x24 ADC divide Register 0
#define ADC_DIV0_REG_OFS          0x24
REGDEF_BEGIN(ADC_DIV0_REG)
REGDEF_BIT(AIN0_DIV, 8)       // ADC channel 0 divider
REGDEF_BIT(AIN1_DIV, 8)       // ADC channel 1 divider
REGDEF_BIT(AIN2_DIV, 8)       // ADC channel 2 divider
REGDEF_BIT(AIN3_DIV, 8)       // ADC channel 3 divider
REGDEF_END(ADC_DIV0_REG)

//0x30 ADC channel 0 data Register
#define ADC_AIN0_DATA_REG_OFS     0x30
REGDEF_BEGIN(ADC_AIN0_DATA_REG)
REGDEF_BIT(AIN0_DATA, 9)      // ADC channel 0 data
REGDEF_BIT(, 23)
REGDEF_END(ADC_AIN0_DATA_REG)

//0x34 ADC channel 1 data Register
#define ADC_AIN1_DATA_REG_OFS     0x34
REGDEF_BEGIN(ADC_AIN1_DATA_REG)
REGDEF_BIT(AIN1_DATA, 9)      // ADC channel 1 data
REGDEF_BIT(, 23)
REGDEF_END(ADC_AIN1_DATA_REG)

//0x38 ADC channel 2 data Register
#define ADC_AIN2_DATA_REG_OFS     0x38
REGDEF_BEGIN(ADC_AIN2_DATA_REG)
REGDEF_BIT(AIN2_DATA, 9)      // ADC channel 2 data
REGDEF_BIT(, 23)
REGDEF_END(ADC_AIN2_DATA_REG)

//0x3C ADC channel 3 data Register
#define ADC_AIN3_DATA_REG_OFS     0x3C
REGDEF_BEGIN(ADC_AIN3_DATA_REG)
REGDEF_BIT(AIN3_DATA, 9)      // ADC channel 3 data
REGDEF_BIT(, 22)
REGDEF_END(ADC_AIN3_DATA_REG)

//0x40 Thermal Sensor configure Register
#define ADC_THERMAL_CFG_REG_OFS   0x40
REGDEF_BEGIN(ADC_THERMAL_CFG_REG)
REGDEF_BIT(AIN_AVG_CNT, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(AIN_AVG_ID, 2)
REGDEF_BIT(, 10)
REGDEF_BIT(AIN_AVG_OUT, 9)
REGDEF_BIT(, 7)
REGDEF_END(ADC_THERMAL_CFG_REG)


#endif
