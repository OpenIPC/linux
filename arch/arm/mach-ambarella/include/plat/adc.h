/*
 * arch/arm/plat-ambarella/include/plat/adc.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_ADC_H__
#define __PLAT_AMBARELLA_ADC_H__

#include <linux/workqueue.h>

/* ==========================================================================*/
#if (CHIP_REV == A7L)
#define ADC_NUM_CHANNELS		8
#elif (CHIP_REV == S2) || (CHIP_REV == S2E)
#define ADC_NUM_CHANNELS		12
#elif (CHIP_REV == S3)
#define ADC_NUM_CHANNELS		5
#else
#define ADC_NUM_CHANNELS		4
#endif

#if (CHIP_REV == A5S)
#define ADC_SUPPORT_THRESHOLD_INT	0
#define ADC_SUPPORT_SLOT		0
#else
#define ADC_SUPPORT_THRESHOLD_INT	1
#define ADC_SUPPORT_SLOT		1
#endif

/* ==========================================================================*/
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define ADC_OFFSET			0x1D000
#else
#define ADC_OFFSET			0xD000
#endif
#define ADC_BASE			(APB_BASE + ADC_OFFSET)
#define ADC_REG(x)			(ADC_BASE + (x))

/* ==========================================================================*/
#if (CHIP_REV == A5S)
#define ADC_CONTROL_TYPE		0
#define ADC_CONTROL_OFFSET		0x00
#define ADC_ENABLE_OFFSET		0x18
#else
#define ADC_CONTROL_TYPE		1
#define ADC_CONTROL_OFFSET		0x04
#define ADC_ENABLE_OFFSET		ADC_CONTROL_OFFSET
#endif

#if (CHIP_REV == A5S)
/* NOTE: ADC channel is re-order for A5S to make life easier */
#define ADC_DATA0_OFFSET		0x04
#define ADC_DATA1_OFFSET		0x08
#define ADC_DATA2_OFFSET		0x0c
#define ADC_DATA3_OFFSET		0x10
#else
#define ADC_DATA0_OFFSET		0x150
#define ADC_DATA1_OFFSET		0x154
#define ADC_DATA2_OFFSET		0x158
#define ADC_DATA3_OFFSET		0x15c
#endif

#define ADC_COUNTER_OFFSET		0x008

#if (CHIP_REV == A5S)
#define ADC_CHAN0_INTR_OFFSET		0x44
#define ADC_CHAN1_INTR_OFFSET		0x48
#define ADC_CHAN2_INTR_OFFSET		0x4c
#define ADC_CHAN3_INTR_OFFSET		0x50
#else
#define ADC_CHAN0_INTR_OFFSET		0x120
#define ADC_CHAN1_INTR_OFFSET		0x124
#define ADC_CHAN2_INTR_OFFSET		0x128
#define ADC_CHAN3_INTR_OFFSET		0x12c
#endif

#if (CHIP_REV == A7L)
#define ADC_DATA4_OFFSET		0x100
#define ADC_DATA5_OFFSET		0x104
#define ADC_DATA6_OFFSET		0x108
#define ADC_DATA7_OFFSET		0x10c
#define ADC_CHAN4_INTR_OFFSET		0x110
#define ADC_CHAN5_INTR_OFFSET		0x114
#define ADC_CHAN6_INTR_OFFSET		0x118
#define ADC_CHAN7_INTR_OFFSET		0x11c
#elif (CHIP_REV == S2) || (CHIP_REV == S2E) || (CHIP_REV == S3)
#define ADC_DATA4_OFFSET		0x160
#define ADC_DATA5_OFFSET		0x164
#define ADC_DATA6_OFFSET		0x168
#define ADC_DATA7_OFFSET		0x16c
#define ADC_CHAN4_INTR_OFFSET		0x130
#define ADC_CHAN5_INTR_OFFSET		0x134
#define ADC_CHAN6_INTR_OFFSET		0x138
#define ADC_CHAN7_INTR_OFFSET		0x13c
#else
#define ADC_DATA4_OFFSET		0x54
#define ADC_DATA5_OFFSET		0x58
#define ADC_DATA6_OFFSET		0x5c
#define ADC_DATA7_OFFSET		0x60
#define ADC_CHAN4_INTR_OFFSET		0x64
#define ADC_CHAN5_INTR_OFFSET		0x68
#define ADC_CHAN6_INTR_OFFSET		0x6c
#define ADC_CHAN7_INTR_OFFSET		0x70
#endif

#if (CHIP_REV == S2) || (CHIP_REV == S2E)
#define ADC_DATA8_OFFSET		0x170
#define ADC_DATA9_OFFSET		0x174
#define ADC_DATA10_OFFSET		0x178
#define ADC_DATA11_OFFSET		0x17c
#define ADC_CHAN8_INTR_OFFSET		0x140
#define ADC_CHAN9_INTR_OFFSET		0x144
#define ADC_CHAN10_INTR_OFFSET		0x148
#define ADC_CHAN11_INTR_OFFSET		0x14c
#else
#define ADC_DATA8_OFFSET		0x60
#define ADC_DATA9_OFFSET		0x70
#define ADC_CHAN8_INTR_OFFSET		0x120
#define ADC_CHAN9_INTR_OFFSET		0x124
#endif

/* S2, S2L and S3 */
#define ADC_STATUS_OFFSET		0x000
#define ADC_SLOT_NUM_OFFSET		0x00c
#define ADC_SLOT_PERIOD_OFFSET		0x010
#define ADC_CTRL_INTR_TABLE_OFFSET	0x044
#define ADC_DATA_INTR_TABLE_OFFSET	0x048
#define ADC_FIFO_INTR_TABLE_OFFSET	0x04c
#define ADC_ERR_STATUS_OFFSET		0x050

#define ADC_SLOT_CTRL_0_OFFSET		0x100
#define ADC_SLOT_CTRL_1_OFFSET		0x104
#define ADC_SLOT_CTRL_2_OFFSET		0x108
#define ADC_SLOT_CTRL_3_OFFSET		0x10c
#define ADC_SLOT_CTRL_4_OFFSET		0x110
#define ADC_SLOT_CTRL_5_OFFSET		0x114
#define ADC_SLOT_CTRL_6_OFFSET		0x118
#define ADC_SLOT_CTRL_7_OFFSET		0x11c

#define ADC_FIFO_CTRL_0_OFFSET		0x180
#define ADC_FIFO_CTRL_1_OFFSET		0x184
#define ADC_FIFO_CTRL_2_OFFSET		0x188
#define ADC_FIFO_CTRL_3_OFFSET		0x18C
#define ADC_FIFO_CTRL_OFFSET		0x190

#define ADC_FIFO_STATUS_0_OFFSET	0x1a0
#define ADC_FIFO_STATUS_1_OFFSET	0x1a4
#define ADC_FIFO_STATUS_2_OFFSET	0x1a8
#define ADC_FIFO_STATUS_3_OFFSET	0x1ac

#define ADC_EC_ADC_OFFSET		0x1C0
#define ADC_EC_REFVAL_OFFSET		0x1C4
#define ADC_EC_RESHAPE_OFFSET		0x1C8

#define ADC_FIFO_DATA0_OFFSET		0x200
#define ADC_FIFO_DATA1_OFFSET		0x280
#define ADC_FIFO_DATA2_OFFSET		0x300
#define ADC_FIFO_DATA3_OFFSET		0x380

/* A7 */
#define ADC_DATA4_SAMPLE0_OFFSET	0x60
#define ADC_DATA4_SAMPLE1_OFFSET	0x64
#define ADC_DATA4_SAMPLE2_OFFSET	0x68
#define ADC_DATA4_SAMPLE3_OFFSET	0x6c
#define ADC_DATA5_SAMPLE0_OFFSET	0x70
#define ADC_DATA5_SAMPLE1_OFFSET	0x74
#define ADC_DATA5_SAMPLE2_OFFSET	0x78
#define ADC_DATA5_SAMPLE3_OFFSET	0x5c

#define ADC_CONTROL_REG			ADC_REG(ADC_CONTROL_OFFSET)
#define ADC_COUNTER_REG			ADC_REG(ADC_COUNTER_OFFSET)
#define ADC_ENABLE_REG			ADC_REG(ADC_ENABLE_OFFSET)

#define ADC_DATA0_REG			ADC_REG(ADC_DATA0_OFFSET)
#define ADC_DATA1_REG			ADC_REG(ADC_DATA1_OFFSET)
#define ADC_DATA2_REG			ADC_REG(ADC_DATA2_OFFSET)
#define ADC_DATA3_REG			ADC_REG(ADC_DATA3_OFFSET)
#define ADC_DATA4_REG			ADC_REG(ADC_DATA4_OFFSET)
#define ADC_DATA5_REG			ADC_REG(ADC_DATA5_OFFSET)
#define ADC_DATA6_REG			ADC_REG(ADC_DATA6_OFFSET)
#define ADC_DATA7_REG			ADC_REG(ADC_DATA7_OFFSET)
#define ADC_DATA8_REG			ADC_REG(ADC_DATA8_OFFSET)
#define ADC_DATA9_REG			ADC_REG(ADC_DATA9_OFFSET)

#define ADC_CHAN0_INTR_REG		ADC_REG(ADC_CHAN0_INTR_OFFSET)
#define ADC_CHAN1_INTR_REG		ADC_REG(ADC_CHAN1_INTR_OFFSET)
#define ADC_CHAN2_INTR_REG		ADC_REG(ADC_CHAN2_INTR_OFFSET)
#define ADC_CHAN3_INTR_REG		ADC_REG(ADC_CHAN3_INTR_OFFSET)
#define ADC_CHAN4_INTR_REG		ADC_REG(ADC_CHAN4_INTR_OFFSET)
#define ADC_CHAN5_INTR_REG		ADC_REG(ADC_CHAN5_INTR_OFFSET)
#define ADC_CHAN6_INTR_REG		ADC_REG(ADC_CHAN6_INTR_OFFSET)
#define ADC_CHAN7_INTR_REG		ADC_REG(ADC_CHAN7_INTR_OFFSET)
#define ADC_CHAN8_INTR_REG		ADC_REG(ADC_CHAN8_INTR_OFFSET)
#define ADC_CHAN9_INTR_REG		ADC_REG(ADC_CHAN9_INTR_OFFSET)

#define ADC_DATA4_SAMPLE0_REG		ADC_REG(ADC_DATA4_SAMPLE0_OFFSET)
#define ADC_DATA4_SAMPLE1_REG		ADC_REG(ADC_DATA4_SAMPLE1_OFFSET)
#define ADC_DATA4_SAMPLE2_REG		ADC_REG(ADC_DATA4_SAMPLE2_OFFSET)
#define ADC_DATA4_SAMPLE3_REG		ADC_REG(ADC_DATA4_SAMPLE3_OFFSET)
#define ADC_DATA5_SAMPLE0_REG		ADC_REG(ADC_DATA5_SAMPLE0_OFFSET)
#define ADC_DATA5_SAMPLE1_REG		ADC_REG(ADC_DATA5_SAMPLE1_OFFSET)
#define ADC_DATA5_SAMPLE2_REG		ADC_REG(ADC_DATA5_SAMPLE2_OFFSET)
#define ADC_DATA5_SAMPLE3_REG		ADC_REG(ADC_DATA5_SAMPLE3_OFFSET)

/* valid only for S2/S2E/S2L/S3/S3L */
#define ADC_STATUS_REG			ADC_REG(ADC_STATUS_OFFSET)
#define ADC_SLOT_NUM_REG		ADC_REG(ADC_SLOT_NUM_OFFSET)
#define ADC_SLOT_PERIOD_REG		ADC_REG(ADC_SLOT_PERIOD_OFFSET)
#define ADC_CTRL_INTR_TABLE_REG		ADC_REG(ADC_CTRL_INTR_TABLE_OFFSET)
#define ADC_DATA_INTR_TABLE_REG		ADC_REG(ADC_DATA_INTR_TABLE_OFFSET)
#define ADC_FIFO_INTR_TABLE_REG		ADC_REG(ADC_FIFO_INTR_TABLE_OFFSET)
#define ADC_ERR_STATUS_REG		ADC_REG(ADC_ERR_STATUS_OFFSET)

#define ADC_SLOT_CTRL_0_REG		ADC_REG(ADC_SLOT_CTRL_0_OFFSET)
#define ADC_SLOT_CTRL_1_REG		ADC_REG(ADC_SLOT_CTRL_1_OFFSET)
#define ADC_SLOT_CTRL_2_REG		ADC_REG(ADC_SLOT_CTRL_2_OFFSET)
#define ADC_SLOT_CTRL_3_REG		ADC_REG(ADC_SLOT_CTRL_3_OFFSET)
#define ADC_SLOT_CTRL_4_REG		ADC_REG(ADC_SLOT_CTRL_4_OFFSET)
#define ADC_SLOT_CTRL_5_REG		ADC_REG(ADC_SLOT_CTRL_5_OFFSET)
#define ADC_SLOT_CTRL_6_REG		ADC_REG(ADC_SLOT_CTRL_6_OFFSET)
#define ADC_SLOT_CTRL_7_REG		ADC_REG(ADC_SLOT_CTRL_7_OFFSET)

#define ADC_FIFO_CTRL_0_REG		ADC_REG(ADC_FIFO_CTRL_0_OFFSET)
#define ADC_FIFO_CTRL_1_REG		ADC_REG(ADC_FIFO_CTRL_1_OFFSET)
#define ADC_FIFO_CTRL_2_REG		ADC_REG(ADC_FIFO_CTRL_2_OFFSET)
#define ADC_FIFO_CTRL_3_REG		ADC_REG(ADC_FIFO_CTRL_3_OFFSET)
#define ADC_FIFO_CTRL_REG		ADC_REG(ADC_FIFO_CTRL_OFFSET)

#define ADC_FIFO_STATUS_0_REG		ADC_REG(ADC_FIFO_STATUS_0_OFFSET)
#define ADC_FIFO_STATUS_1_REG		ADC_REG(ADC_FIFO_STATUS_1_OFFSET)
#define ADC_FIFO_STATUS_2_REG		ADC_REG(ADC_FIFO_STATUS_2_OFFSET)
#define ADC_FIFO_STATUS_3_REG		ADC_REG(ADC_FIFO_STATUS_3_OFFSET)

#define ADC_EC_ADC_REG			ADC_REG(ADC_EC_ADC_OFFSET)
#define ADC_EC_REFVAL_REG		ADC_REG(ADC_EC_REFVAL_OFFSET)
#define ADC_EC_RESHAPE_REG		ADC_REG(ADC_EC_RESHAPE_OFFSET)

#define ADC_FIFO_DATA0_REG		ADC_REG(ADC_FIFO_DATA0_OFFSET)
#define ADC_FIFO_DATA1_REG		ADC_REG(ADC_FIFO_DATA1_OFFSET)
#define ADC_FIFO_DATA2_REG		ADC_REG(ADC_FIFO_DATA2_OFFSET)
#define ADC_FIFO_DATA3_REG		ADC_REG(ADC_FIFO_DATA3_OFFSET)

#define ADC_DATA10_REG			ADC_REG(ADC_DATA10_OFFSET)
#define ADC_DATA11_REG			ADC_REG(ADC_DATA11_OFFSET)
#define ADC_CHAN10_INTR_REG		ADC_REG(ADC_CHAN10_INTR_OFFSET)
#define ADC_CHAN11_INTR_REG		ADC_REG(ADC_CHAN11_INTR_OFFSET)

#if (CHIP_REV == A7L)
#error "ADC_DATA_REG/ADC_CHAN_INTR_REG(ch) Not Implemented"
#else
#define ADC_DATA_OFFSET(ch)		(ADC_DATA0_OFFSET + (ch) * 4)
#define ADC_DATA_REG(ch)		ADC_REG(ADC_DATA_OFFSET(ch))
#define ADC_FIFO_CTRL_X_OFFSET(fifoNo)	(ADC_FIFO_CTRL_0_OFFSET + (fifoNo) * 4)
#define ADC_FIFO_CTRL_X_REG(fifoNo)	ADC_REG(ADC_FIFO_CTRL_X_OFFSET(fifoNo))
#define ADC_CHAN_INTR_OFFSET(ch)	(ADC_CHAN0_INTR_OFFSET + (ch) * 4)
#define ADC_CHAN_INTR_REG(ch)		ADC_REG(ADC_CHAN_INTR_OFFSET(ch))
#endif

#define ADC16_CTRL_OFFSET		0x198
#define ADC16_CTRL_REG			RCT_REG(ADC16_CTRL_OFFSET)

/* ADC_CONTROL_REG */
#define ADC_CONTROL_GYRO_SAMPLE_MODE	0x08

/* valid only for S2/S2E/S2L/S3/S3L */
#define ADC_CONTROL_RESET		0x01
#define ADC_FIFO_OVER_INT_EN		(0x1 << 31)
#define ADC_FIFO_UNDR_INT_EN		(0x1 << 30)
#define ADC_FIFO_DEPTH			0x80
#define ADC_FIFO_TH			(((ADC_FIFO_DEPTH >> 2)-1) << 16)
#define ADC_FIFO_CLEAR			0x1
#define ADC_FIFO_ID_SHIFT		12
#define ADC_FIFO_CONTROL_CLEAR		0x01
#define ADC_FIFO_NUMBER		        0x04

#define ADC_CTRL_SCALER_POWERDOWN	0x100
#define ADC_CTRL_POWERDOWN		0x2
#define ADC_CTRL_CLK_SOURCE_SCALER	0x0
#define ADC_CTRL_CLK_SOURCE_AUDIO	0x1

#if (CHIP_REV == A5S)
#define ADC_CONTROL_MODE		0x04
#define ADC_CONTROL_ENABLE		0x01
#define ADC_CONTROL_START		0x02
#else
#define ADC_CONTROL_MODE		0x02
#define ADC_CONTROL_ENABLE		0x04
#define ADC_CONTROL_START		0x08
#endif
#define ADC_CONTROL_STATUS		0x01

#if (CHIP_REV == A5S)
#define ADC_EN_HI(x)			((x) << 31)
#define ADC_EN_LO(x)			((x) << 30)
#else
#define ADC_EN_HI(x)			((x) << 31)
#define ADC_EN_LO(x)			((x) << 31)
#endif

#define ADC_VAL_HI(x)			(((x) & 0xfff) << 16)
#define ADC_VAL_LO(x)			((x) & 0xfff)
/* ==========================================================================*/

#define ADC_MAX_SLOT_NUMBER		8

#define ADC_CH0				(1 << 0)
#define ADC_CH1				(1 << 1)
#define ADC_CH2				(1 << 2)
#define ADC_CH3				(1 << 3)
#define ADC_CH4				(1 << 4)
#define ADC_CH5				(1 << 5)
#define ADC_CH6				(1 << 6)
#define ADC_CH7				(1 << 7)
#define ADC_CH8				(1 << 8)
#define ADC_CH9				(1 << 9)
#define ADC_CH10			(1 << 10)
#define ADC_CH11			(1 << 11)

enum {
	AMBADC_ONESHOT = 0,
	AMBADC_CONTINUOUS,
};

struct ambadc_host {
	struct device *dev;
	u32 irq;
	u32 clk;
	bool polling_mode;
	bool keep_start;
        bool fifo_mode;
	struct delayed_work work;
};

struct ambadc_client;
typedef int (*ambadc_client_callback)(struct ambadc_client *client,
			u32 ch, u32 level);
typedef int (*ambadc_read_level)(u32 ch);

struct ambadc_client {
	struct device *dev;
	struct ambadc_host *host;
	struct list_head node;
	u32 threshold[ADC_NUM_CHANNELS];
	u32 mode;
	ambadc_client_callback callback;
};

extern struct ambadc_client *ambarella_adc_register_client(struct device *dev,
			u32 mode, ambadc_client_callback callback);
extern void ambarella_adc_unregister_client(struct ambadc_client *client);

extern int ambarella_adc_set_threshold(struct ambadc_client *client,
			u32 ch, u32 low, u32 high);
extern int ambarella_adc_read_level(u32 ch);

#endif /* __PLAT_AMBARELLA_ADC_H__ */

