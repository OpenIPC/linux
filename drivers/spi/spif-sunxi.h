/*
 * drivers/spi/spif-sunxi.h
 *
 * Copyright (c) 2021-2028 Allwinnertech Co., Ltd.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * SUNXI SPIF Register Definition
 *
 * 2021.6.24 liuyu <liuyu@allwinnertech.com>
 *    creat thie file and support sun8i of Allwinner.
 */

#ifndef _SUNXI_SPI_H_
#define _SUNXI_SPI_H_

#define HEXADECIMAL			(0x10)
#define REG_END				(0x0f)
#define SAMPLE_NUMBER		(0x80)
#define REG_INTERVAL		(0x04)
#define REG_CL				(0x0c)
#define BULK_DATA_BOUNDARY	64	 /* can modify to adapt the application */

#define SPIF_VER			0x00
#define SPIF_GCR			0x04
#define SPIF_GCA			0x08
#define SPIF_TCG			0x0C
#define SPIF_IER			0x14
#define SPIF_ISR			0x18
#define SPIF_CSD			0x1c
#define SPIF_PHC			0x20
#define SPIF_TCF			0x24
#define SPIF_TCS			0x28
#define SPIF_TNM			0x2C
#define SPIF_DMC			0x40
#define SPIF_DSC			0x44
#define SPIF_QFT			0x48
#define SPIF_CFT			0x4C
#define SPIF_BATC			0x54

/* 0x04 */
#define SPIF_CFG_MODE				(0x1 << 0)
#define PREFETCH_ADDR_MAP_MODE		(0x1 << 1)
#define	SPIF_NMODE_EN				(0x1 << 2)
#define	SPIF_PMODE_EN				(0x1 << 3)
#define	SPIF_CS_SEL					6
#define SPIF_TC_PHA					(0x1 <<  4) /* SPI Clock/Data phase control,0: phase0,1: phase1;default:1 */
#define SPIF_TC_POL					(0x1 <<  5) /* SPI Clock polarity control,0:low level idle,1:high level idle;default:1 */
#define SPIF_TX_CFG_FBS				(0x1 << 18)	/*0:MSB first, 1:LSB first*/
#define SPIF_CS_MASK				0x11

/*0x08*/
#define SPIF_CDC_RF_RST				(0x1 << 0)
#define	SPIF_CDC_WF_RST				(0x1 << 1)
#define	SPIF_DQS_RF_SRST			(0x1 << 2)
#define SPIF_RESET					(0x1 << 3)

/*0x0C */
#define	ANALOG_SAMP_DL_SW_VALUE_RX	(0)
#define ANALOG_SAMP_DL_SW_EN_RX		(0x1 << 6)
#define DIGITAL_SCKR_DELAY_CFG		(16)
#define SCKR_DLY_MODE_SEL			(0x1 << 20)
#define CLK_SPIF_SRC_SEL			(0x1 << 24)
#define CLK_SCK_SRC_SEL				(0x1 << 25)
#define CLK_SCKOUT_SRC_SEL			(0x1 << 26)

/*0x14*/
#define SPIF_RF_RDY_INT_EN			(0x1 << 0)	/* read fifo is ready */
#define SPIF_RF_ENPTY_INT_EN		(0x1 << 1)
#define SPIF_RF_FULL_INT_EN			(0x1 << 2)
#define SPIF_WF_RDY_INT_EN			(0x1 << 4)	/* write fifo is ready */
#define SPIF_WF_ENPTY_INT_EN		(0x1 << 5)
#define SPIF_WF_FULL_INT_EN			(0x1 << 6)
#define SPIF_RF_OVF_INT_EN			(0x1 << 8)
#define SPIF_RF_UDF_INT_EN			(0x1 << 9)
#define SPIF_WF_OVF_INT_EN			(0x1 << 10)
#define SPIF_WF_UDF_INT_EN			(0x1 << 11)
#define SPIF_DMA_TRANS_DONE_EN		(0x1 << 24)
#define SPIF_PREFETCH_READ_EN		(0x1 << 25)
#define SPIF_INT_STA_READY_EN		(SPIF_RF_RDY_INT_EN | SPIF_WF_RDY_INT_EN)
#define SPIF_INT_STA_TC_EN			(SPIF_DMA_TRANS_DONE_EN | SPIF_PREFETCH_READ_EN)
#define	SPIF_INT_STA_ERR_EN			(SPIF_RF_OVF_INT_EN | SPIF_RF_UDF_INT_EN | SPIF_WF_OVF_INT_EN)

/* 0x18 */
#define	SPIF_RF_RDY					BIT(0)
#define	SPIF_RF_EMPTY				BIT(1)
#define	SPIF_RF_FULL				BIT(2)
#define	SPIF_WF_RDY					BIT(4)
#define	SPIF_WF_EMPTY				BIT(5)
#define	SPIF_WF_FULL				BIT(6)
#define SPIF_RF_OVF					BIT(8)
#define SPIF_RF_UDF					BIT(9)
#define SPIF_WF_OVF					BIT(10)
#define SPIF_WF_UDF					BIT(11)
#define	SPIF_DMA_TRANS_DONE			BIT(24)
#define	SPIF_PREFETCH_READ_BACK		BIT(25)
#define SPIF_INT_STA_READY			(SPIF_RF_READY | SPIF_WF_READY)
#define SPIF_INT_STA_TC				(SPIF_DMA_TRANS_DONE | SPIF_PREFETCH_READ_BACK)
#define	SPIF_INT_STA_ERR			(SPIF_RF_OVF | SPIF_RF_UDF | SPIF_WF_OVF)

/* 0x1c */
#define SPIF_CSSOT					0	/* chip select start of transfer */
#define SPIF_CSEOT					8	/* chip select end of transfer */
#define SPIF_CSDA					16	/* chip select de-assert */
#define SPIF_CSSOT_DEFAULT			0x13
#define SPIF_CSEOT_DEFAULT			0x13
#define SPIF_CSDA_DEFAULT			0x1d

/* dma desc 0 */
#define	SPIF_DMA_FINISH_FLAG		BIT(0)
#define SPIF_DMA_TX					BIT(1)
#define SPIF_HBURST_TYPE			4

/*dma desc 1 */
#define SPIF_DMA_BLK_LEN			16
#define	SPIF_DMA_DATA_LEN			0

/* dma desc 2 */
#define SPIF_DMA_BUFFER_SADDR		0

/* dma desc 3 */
#define SPIF_NEXT_DESCRIPTOR_ADDR	0

/* 0x20 */
#define	SPIF_RX_CRC_EN				BIT(0)
#define	SPIF_TC_CRC_EN				BIT(4)
#define	SPIF_RX_DATA_EN				BIT(8)
#define	SPIF_TX_DATA_EN				BIT(12)
#define	SPIF_DUMMY_TRANS_EN			BIT(16)
#define	SPIF_MODE_TRANS_EN			BIT(20)
#define	SPIF_ADDR_TRANS_EN			BIT(24)
#define	SPIF_COMMAND_TRANS_EN		BIT(28)

/* 0x24 */
#define	SPIF_ADDR_OPCODE			0	/* store the addr */

/* 0x28 */
#define	SPIF_DATA_TRANS_TYPE		0	/* bit [1:0] */
#define	SPIF_MODE_TRANS_TYPE		4	/* bit [5:4] */
#define	SPIF_ADDR_TRANS_TYPE		8	/* bit [9:8] */
#define	SPIF_CMD_TRANS_TYPE			12	/* bit [13:12] set the data transfer type*/
#define	SPIF_MODE_OPCODE			16	/* bit [23:16] store the mode */
#define	SPIF_CMD_OPCODE				24	/* bit [31:24], store the cmd */

/* 0x2C */
#define	SPIF_DATA_TRANS_NUM			0		/* bit [15:0]*/
#define	SPIF_DUMMY_TRANS_NUM		16		/* bit [23:16], value 0 = 1 cycle*/
#define	SPIF_ADDR_32				BIT(24)
#define SPIF_NORMAL_EN				BIT(28)

/* 0x40 */
#define SPIF_CFG_DMA_START			BIT(0)
#define SPIF_DMA_DESCRIPTOR_LEN		4

/* 0x44 */
#define SPIF_FIRST_DESCRIPTOR_SADDR	0

/* 0x48 */
#define SPIF_DQS_EMPTY_TRIG_LEVEL	0
#define SPIF_DQS_FULL_TRIG_LEVEL	8

/* 0x4c */
#define SPIF_RF_EMPTY_TRIG_LEVEL	0
#define SPIF_RF_FULL_TRIG_LEVEL		8
#define SPIF_WF_EMPTY_TRIG_LEVEL	16
#define SPIF_WF_FULL_TRIG_LEVEL		24

#define SPIF_PHA_ACTIVE_			0x01
#define SPIF_POL_ACTIVE_			0x02
#define SPIF_MODE_0_ACTIVE_			(0|0)
#define SPIF_MODE_1_ACTIVE_			(0|SPI_PHA_ACTIVE_)
#define SPIF_MODE_2_ACTIVE_			(SPI_POL_ACTIVE_|0)
#define SPIF_MODE_3_ACTIVE_			(SPI_POL_ACTIVE_|SPI_PHA_ACTIVE_)
#define SPIF_CS_HIGH_ACTIVE_		0x04
#define SPIF_LSB_FIRST_ACTIVE_		0x08
#define SPIF_DUMMY_ONE_ACTIVE_		0x10
#define SPIF_RECEIVE_ALL_ACTIVE_	0x20

struct sunxi_spif_data {
	u32 min_speed_hz;
	u32 max_speed_hz;
	u32 cs_num;			/* the total num of spi chip select */
};

/*
* dma descriptor 0~3 : set the dma config
* you can set the register 0x20 0x24 0x28 0x2c to set too
* dma descriptor 4~7 : set the spif controller transfer config
*/
struct dma_descriptor {
	u32 dma_cfg0;
	u32 dma_cfg1;	/* dma blk len and dma data len */
	u32 data_buf;	/* data buff */
	u32 next_desc;	/* the next dma desc */
	u32 spif_cfg0;	/* trans en, can be instead by 0x20 */
	u32 spif_cfg1;	/* the spi device's addr, can be instead by 0x24 */
	u32 spif_cfg2;	/* opcode and trans type, can be instead by 0x28 */
	u32 spif_cfg3;	/* can be instead by 0x2c */
};

struct sunxi_spif_transfer_t {
	u8 cmd_type;	/* 00:1-wire to send command, 01:2-wire, 10:4-wire, 11: 8-wire*/
	u8 cmd;	/* the cmd to be transmited */

	u8 addr_bit;	/* can be set 3(24bit) or 4(32bit) */
	u8 addr_type;	/* 00:1-wire to send addr, 01:2-wire, 10:4-wire, 11: 8-wire*/
	u32 addr;	/* the addr to be transtmited*/

	u8 mode_type;	/* 00:1-wire to send mode, 01:2-wire, 10:4-wire, 11: 8-wire*/
	u8 mode;	/* the mode to be trantmoted */

	u8 dummy_cnt;	/* the dummy count */

	u8 data_type;	/* 00:1-wire to trans data, 01:2-wire, 10:4-wire, 11: 8-wire*/
	u32 data_cnt;	/* receive or send data count */
	void *data_buf;	/* receive or send data buf */
};

struct sunxi_spif {
	struct platform_device *pdev;
	struct spi_master *master;
	struct spi_device *spi;
	struct completion done;  /* wakup another spi transfer */
	struct task_struct *task;
	struct sunxi_spif_data *data;
	spinlock_t lock;
	void __iomem *base_addr;
	struct dma_pool	*pool;
	struct dma_descriptor *dma_desc;
	dma_addr_t desc_phys;	/* the request dma descriptor phys addr */
	u32 spif_register[24];
	int result;			/* the spi trans result */
	char dev_name[48];

	/* dts data */
	struct regulator *regulator;
	struct pinctrl	*pctrl;
	struct clk *pclk;  /* PLL clock */
	struct clk *mclk;  /* spi module clock */
	struct reset_control	*rstc;
	u32 base_addr_phy;
	u32 irq;
	u32 speed_hz;	/* the spif controller working speed */
	u32 working_mode;
#define	DOUBLE_EDGE_TRIGGER	0x1
#define PREFETCH_READ_MODE	0x2
#define DQS					0x4
	char regulator_id[16];
};
#endif
