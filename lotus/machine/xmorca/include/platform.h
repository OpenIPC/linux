/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __XMORCA_PLATFORM_H__
#define __XMORCA_PLATFORM_H__

/* DDR */
#define REG_DDRC_BASE		0x120D0000
#define REG_DDRC_TOP_FEATURE	(REG_DDRC_BASE + 0)
#define DFI_BASE_OFFSET		0x3000
#define DFI_EFFI_CTRL_CH0   (DFI_BASE_OFFSET + 0x26C)
#define DFI_EFFI_OUT_CH0	(DFI_BASE_OFFSET + 0x270)
#define DFI_EFFI_TIME_CH0	(DFI_BASE_OFFSET + 0x274)

#define DFI_EFFI_CTRL_CH1   (DFI_BASE_OFFSET + 0x126C)
#define DFI_EFFI_OUT_CH1	(DFI_BASE_OFFSET + 0x1270)
#define DFI_EFFI_TIME_CH1	(DFI_BASE_OFFSET + 0x1274)

#define REG_DDRPHY_CRG 0x1201007c

/* CPU Freq */
#define REG_CPU_CLK_SEL		0x12010080
#define CPU_FREQ_24M		0
#define CPU_FREQ_1188M		1
#define CPU_FREQ_993M		2
#define CPU_FREQ_900M		3
#define CPU_FREQ_700M		4
#define CPU_FREQ_594M		5
#define CPU_FREQ_396M		6
#define CPU_FREQ_350M		7

/* Chip ID */
#define REG_SC_CHIPID           0x12020EE0


/* TODO: for femac in FPGA stage */
#define CHIPID_XM72020330       0x72020330
#define IO0_BASE                0x12080000
#define IO0_CFG1                0x4
#define IO0_CTRL                0xC
#define IO1_BASE                0x120280B0
#define LEVEL0                  0x23
#define LEVEL1                  0x0
#define IO0_CFG                 0x5
#define LEVEL1_0                0x230c75
#define LEVEL1_1                0xc75

#define MISC_CTRL62	0x120280FC
#define MISC_CTRL63	0x12028100
#define MISC_CTRL64	0x12028104
#define MISC_CTRL65	0x12028108
#define MISC_CTRL66	0x1202810C
#define MISC_CTRL67	0x12028110

#define CONSOLE_UART_BASE	0x12040000

/* Tsensor addr  */
#define REG_TSENSOR_BASE	0x120280b8
#define MISC_CTRL46_OFFSET	0x4
#define MISC_CTRL47_OFFSET	0x8

/* Stopwatch */
#define TRIGGER_BIT		0
#define CLEAR_BIT		1
#define RUNNING_TRIGGER_BIT	2
#define SELECT_BIT		0
#define SELECT_BIT_NUM		4
#define RECORD_NUM		16

#define REG_STOPWATCH_TRIGGER   MISC_CTRL67
#define REG_STOPWATCH_SEL	MISC_CTRL62
#define REG_STOPWATCH_RESULT	MISC_CTRL63
#define STOPWATCH_NUM		RECORD_NUM

#endif /* End of __XMORCA_PLATFORM_H__ */
