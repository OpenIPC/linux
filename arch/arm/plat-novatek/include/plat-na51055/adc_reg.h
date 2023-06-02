/*
	Register offset and bit definition for ADC module

	Register offset and bit definition for ADC module.

	@file       adc_reg.h
	@note       Nothing.

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _ADC_REG_H
#define _ADC_REG_H

#include <mach/nvt-io.h>
#include <linux/clk.h>

#define ADC_CTRL_REG_OFS          0x00
union ADC_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int ain0_mode:1;       // ADC channel 0 mode
	unsigned int ain1_mode:1;       // ADC channel 1 mode
	unsigned int ain2_mode:1;       // ADC channel 2 mode
	unsigned int ain3_mode:1;       // ADC channel 3 mode
	unsigned int reserved0:8;       // Reserved
	unsigned int clkdiv:5;          // ADC clock pre-scalar
	unsigned int reserved1:3;       // Reserved
	unsigned int sampavg:2;         // ADC sample to average
	unsigned int reserved2:1;       // Reserved
	unsigned int adc_en:1;          // ADC converter enable bit
	unsigned int extsamp_cnt:4;     // Extra ADC sample clock count
	unsigned int reserved3:4;
	} bit;
};

//0x04 ADC one-shot Register
#define ADC_ONESHOT_REG_OFS       0x04
union ADC_ONESHOT_REG {
	uint32_t reg;
	struct {
	unsigned int ain0_oneshot:1;    // ADC channel 0 one-shot start
	unsigned int ain1_oneshot:1;    // ADC channel 1 one-shot start
	unsigned int ain2_oneshot:1;    // ADC channel 2 one-shot start
	unsigned int ain3_oneshot:1;    // ADC channel 3 one-shot start
	unsigned int reserved0:28;
	} bit;
};

//0x08 ADC interrupt control Register
#define ADC_INTCTRL_REG_OFS       0x08
union ADC_INTCTRL_REG {
	uint32_t reg;
	struct {
	unsigned int ain0_inten:1;      // ADC channel 0 data ready interrupt enable
	unsigned int ain1_inten:1;      // ADC channel 1 data ready interrupt enable
	unsigned int ain2_inten:1;      // ADC channel 2 data ready interrupt enable
	unsigned int ain3_inten:1;      // ADC channel 3 data ready interrupt enable
	unsigned int aintr0_inten:1;
	unsigned int aintr1_inten:1;
	unsigned int aintr2_inten:1;
	unsigned int reserved0:25;
	} bit;
};

//0x0C ADC status Register
#define ADC_STATUS_REG_OFS        0x0C
union ADC_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int ain0_datardy:1;    // ADC channel 0 data ready status
	unsigned int ain1_datardy:1;    // ADC channel 1 data ready status
	unsigned int ain2_datardy:1;    // ADC channel 2 data ready status
	unsigned int ain3_datardy:1;    // ADC channel 3 data ready status
	unsigned int aintr0:1;
	unsigned int aintr1:1;
	unsigned int aintr2:1;
	unsigned int reserved2:25;
	} bit;
};

//0x10 ADC value trigger Register
#define ADC_TRIGCTRL_REG_OFS        0x10
union ADC_TRIGCTRL_REG {
	uint32_t reg;
	struct {
	unsigned int trig0_en:1;        // ADC channel 0 value trigger enable
	unsigned int trig1_en:1;        // ADC channel 1 value trigger enable
	unsigned int trig2_en:1;        // ADC channel 2 value trigger enable
	unsigned int reserved0:1;
	unsigned int trig0_range:1;     // ADC channel 0 value trigger range
	unsigned int trig1_range:1;     // ADC channel 1 value trigger range
	unsigned int trig2_range:1;     // ADC channel 2 value trigger range
	unsigned int reserved1:1;
	unsigned int trig0_mode:1;      // ADC channel 0 value trigger mode
	unsigned int trig1_mode:1;      // ADC channel 1 value trigger mode
	unsigned int trig2_mode:1;      // ADC channel 2 value trigger mode
	unsigned int reserved2:5;
	unsigned int trig0_id:2;        // ADC channel trigger0 ID
	unsigned int reserved3:2;
	unsigned int trig1_id:2;        // ADC channel trigger1 ID
	unsigned int reserved4:2;
	unsigned int trig2_id:2;        // ADC channel trigger2 ID
	unsigned int reserved5:6;
	} bit;
};

//0x14 ADC value range Register 0
#define ADC_TRIGVAL0_REG_OFS        0x14
union ADC_TRIGVAL0_REG {
	uint32_t reg;
	struct {
	unsigned int trig0_start:8;     // ADC channel 0 trigger value START
	unsigned int trig0_end:8;       // ADC channel 0 trigger value END
	unsigned int reserved0:16;
	} bit;
};

//0x18 ADC value range Register 1
#define ADC_TRIGVAL1_REG_OFS        0x18
union ADC_TRIGVAL1_REG {
	uint32_t reg;
	struct {
	unsigned int trig1_start:8;     // ADC channel 1 trigger value START
	unsigned int trig1_end:8;       // ADC channel 1 trigger value END
	unsigned int reserved0:16;
	} bit;
};

//0x1C ADC value range Register 2
#define ADC_TRIGVAL2_REG_OFS        0x1C
union ADC_TRIGVAL2_REG {
	uint32_t reg;
	struct {
	unsigned int trig2_start:8;     // ADC channel 2 trigger value START
	unsigned int trig2_end:8;       // ADC channel 2 trigger value END
	unsigned int reserved0:16;
	} bit;
};

//0x20 ADC divide Register 0
#define ADC_DIV0_REG_OFS          0x20
union ADC_DIV0_REG {
	uint32_t reg;
	struct {
	unsigned int ain0_div:8;        // ADC channel 0 divider
	unsigned int ain1_div:8;        // ADC channel 1 divider
	unsigned int ain2_div:8;        // ADC channel 2 divider
	unsigned int ain3_div:8;        // ADC channel 3 divider
	} bit;
};

//0x30 ADC channel 0 data Register
#define ADC_AIN0_DATA_REG_OFS     0x30
union ADC_AIN0_DATA_REG {
	uint32_t reg;
	struct {
	unsigned int ain0_data:9;       // ADC channel 0 data
	unsigned int reserved0:23;
	} bit;
};

//0x34 ADC channel 1 data Register
#define ADC_AIN1_DATA_REG_OFS     0x34
union ADC_AIN1_DATA_REG {
	uint32_t reg;
	struct {
	unsigned int ain1_data:9;       // ADC channel 1 data
	unsigned int reserved0:23;
	} bit;
};

//0x38 ADC channel 2 data Register
#define ADC_AIN2_DATA_REG_OFS     0x38
union ADC_AIN2_DATA_REG {
	uint32_t reg;
	struct {
	unsigned int ain2_data:9;       // ADC channel 2 data
	unsigned int reserved0:23;
	} bit;
};

//0x3C ADC channel 3 data Register
#define ADC_AIN3_DATA_REG_OFS     0x3C
union ADC_AIN3_DATA_REG {
	uint32_t reg;
	struct {
	unsigned int ain3_data:9;       // ADC channel 3 data
	unsigned int reserved0:23;
	} bit;
};

//0x40 Thermal sensor configure Register
#define THERMAL_SENSOR_CONFIGURE_REG_OFS     0x40
union THERMAL_SENSOR_CONFIGURE_REG {
	uint32_t reg;
	struct {
	unsigned int ain_avg_cnt:3;     // average count
	unsigned int reserved0:1;
	unsigned int ain_avg_id:2;      // average channel id
	unsigned int reserved1:10;
	unsigned int ain_avg_out:9;     // average value
	unsigned int reserved2:7;
	} bit;
};

#endif
