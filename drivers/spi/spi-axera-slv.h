/* SPDX-License-Identifier: GPL-2.0 */
#ifndef DW_SPI_HEADER_H
#define DW_SPI_HEADER_H

#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/gpio.h>

/* Register offsets */
#define DW_SPI_CTRL0			0x00
#define DW_SPI_CTRL1			0x04
#define DW_SPI_SSIENR			0x08
#define DW_SPI_MWCR			0x0c
#define DW_SPI_SER			0x10
#define DW_SPI_BAUDR			0x14
#define DW_SPI_TXFLTR			0x18
#define DW_SPI_RXFLTR			0x1c
#define DW_SPI_TXFLR			0x20
#define DW_SPI_RXFLR			0x24
#define DW_SPI_SR			0x28
#define DW_SPI_IMR			0x2c
#define DW_SPI_ISR			0x30
#define DW_SPI_RISR			0x34
#define DW_SPI_TXOICR			0x38
#define DW_SPI_RXOICR			0x3c
#define DW_SPI_RXUICR			0x40
#define DW_SPI_MSTICR			0x44
#define DW_SPI_ICR			0x48
#define DW_SPI_DMACR			0x4c
#define DW_SPI_DMATDLR			0x50
#define DW_SPI_DMARDLR			0x54
#define DW_SPI_IDR			0x58
#define DW_SPI_VERSION			0x5c
#define DW_SPI_DR			0x60
#define DW_SPI_RX_SAMPLE_DELAY		0xf0
#define DW_SPI_SPI_CTRL0		0xf4

/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET			0
#define DFS_8_BIT				8
#define DFS_32_BIT				32

#define SPI_FRF_OFFSET			6
#define SPI_FRF_SPI			0x0
#define SPI_FRF_SSP			0x1
#define SPI_FRF_MICROWIRE		0x2
#define SPI_FRF_RESV			0x3

#define SPI_MODE_OFFSET			8
#define SPI_MOD_MASK			(0x3 << SPI_MODE_OFFSET)
#define SPI_SCPH_OFFSET			8
#define SPI_SCOL_OFFSET			9

#define SPI_TMOD_OFFSET			10
#define SPI_TMOD_MASK			(0x3 << SPI_TMOD_OFFSET)
#define	SPI_TMOD_TR			0x0		/* xmit & recv */
#define SPI_TMOD_TO			0x1		/* xmit only */
#define SPI_TMOD_RO			0x2		/* recv only */
#define SPI_TMOD_EPROMREAD		0x3		/* eeprom read mode */

#define SPI_SLVOE_OFFSET		12
#define SPI_SLVOE_MASK			(1 << SPI_SLVOE_OFFSET)
#define SPI_SLVOE_ENABLE		0
#define SPI_SLVOE_DISABLE		1

#define SPI_SRL_OFFSET			13
#define SPI_CFS_OFFSET			16

#define SPI_SPI_FRF_OFFSET		22
#define SPI_SPI_FRF_MASK		(3 << SPI_FRF_OFFSET)
#define SPI_FRF_SPI_STANDARD		0 /* 1 line */
#define SPI_FRF_SPI_DUAL		1 /* 2 line */
#define SPI_FRF_SPI_QUAD		2 /* 4 line */
#define SPI_FRF_SPI_OCTAL		3 /* 8 line */

#define SPI_TXFTHR_OFFSET		16

/* Bit fields in SR, 7 bits */
#define SR_MASK				0x7f		/* cover 7 bits */
#define SR_BUSY				(1 << 0)
#define SR_TF_NOT_FULL			(1 << 1)
#define SR_TF_EMPT			(1 << 2)
#define SR_RF_NOT_EMPT			(1 << 3)
#define SR_RF_FULL			(1 << 4)
#define SR_TX_ERR			(1 << 5)
#define SR_DCOL				(1 << 6)

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI			(1 << 0)
#define SPI_INT_TXOI			(1 << 1)
#define SPI_INT_RXUI			(1 << 2)
#define SPI_INT_RXOI			(1 << 3)
#define SPI_INT_RXFI			(1 << 4)
#define SPI_INT_MSTI			(1 << 5)

/* Bit fields in DMACR */
#define SPI_DMA_RDMAE			(1 << 0)
#define SPI_DMA_TDMAE			(1 << 1)

#define SPI_RSD_OFFSET			0
#define SPI_RSD_MASK			(0xff << SPI_RSD_OFFSET)
#define SPI_SE_OFFSET			16

#define SPI_TRANS_TYPE_OFFSET		0
#define SPI_TRANS_TYPE_TT0		0 /* Instruction and Address will be sent in Standard SPI Mode */
#define SPI_TRANS_TYPE_TT1		1 /* Instruction will be sent in Standard SPI Mode and Address will be sent in the mode specified by CTRLR0.SPI_FRF */
#define SPI_TRANS_TYPE_TT2		2 /* Both Instruction and Address will be sent in the mode specified by SPI_FRF */
#define SPI_TRANS_TYPE_TT3		3 /* Reserved */

#define SPI_ADDR_L_OFFSET		2
#define SPI_ADDR_L8			2

#define SPI_CLK_STRETCH_EN_OFFSET	30
#define SPI_CLK_STRETCH_EN		1

/* TX RX interrupt level threshold, max can be 256 */
#define SPI_INT_THRESHOLD		32

#define AX_SPI_TMOD_TO_RO		//default TX_AND_RX mode for 1-wire PIO, change to TX_ONLY and RX_ONLY mode for 4-wire DMA/PIO

enum dw_ssi_type {
	SSI_MOTO_SPI = 0,
	SSI_TI_SSP,
	SSI_NS_MICROWIRE,
};

struct dw_spi;
struct dw_spi_dma_ops {
	int (*dma_init)(struct dw_spi *dws);
	void (*dma_exit)(struct dw_spi *dws);
	int (*dma_setup)(struct dw_spi *dws, struct spi_transfer *xfer);
	bool (*can_dma)(struct spi_controller *master, struct spi_device *spi,
			struct spi_transfer *xfer);
	int (*dma_transfer)(struct dw_spi *dws, struct spi_transfer *xfer);
	void (*dma_stop)(struct dw_spi *dws);
};

struct dw_spi {
	struct spi_controller	*master;
	enum dw_ssi_type	type;

	void __iomem		*regs;
	unsigned long		paddr;
	int			irq;
	u32			fifo_len;	/* depth of the FIFO buffer */
	u32			max_freq;	/* max bus freq supported */

	u32			reg_io_width;	/* DR I/O width in bytes */
	u16			bus_num;
	u16			num_cs;		/* supported slave numbers */
	void (*set_cs)(struct spi_device *spi, bool enable);

	/* Current message transfer state info */
	size_t			len;
	void			*tx;
	void			*tx_end;
	spinlock_t		buf_lock;
	void			*rx;
	void			*rx_end;
	int			dma_mapped;
	u8			n_bytes;	/* current is a 1/2 bytes op */
	u32			dma_width;
	irqreturn_t		(*transfer_handler)(struct dw_spi *dws);
	u32			current_freq;	/* frequency in hz */

	/* DMA info */
	int			dma_inited;
	struct dma_chan		*txchan;
	struct dma_chan		*rxchan;
	unsigned long		dma_chan_busy;
	dma_addr_t		dma_addr; /* phy address of the Data register */
	const struct dw_spi_dma_ops *dma_ops;
	void			*dma_tx;
	void			*dma_rx;

	/* Bus interface info */
	void			*priv;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
#endif
};

//#define AX_SPI_SLV_REG_PRINT
static inline u32 dw_readl(struct dw_spi *dws, u32 offset)
{
	u32 val = __raw_readl(dws->regs + offset);
#ifdef AX_SPI_SLV_REG_PRINT
	printk("spi_slv: read32 reg 0x%x, val 0x%x\n", offset, val);
#endif
	return val;
}

static inline u16 dw_readw(struct dw_spi *dws, u32 offset)
{
	u16 val = __raw_readw(dws->regs + offset);
#ifdef AX_SPI_SLV_REG_PRINT
	printk("spi_slv: read16 reg 0x%x, val 0x%x\n", offset, val);
#endif
	return val;
}

static inline void dw_writel(struct dw_spi *dws, u32 offset, u32 val)
{
#ifdef AX_SPI_SLV_REG_PRINT
	printk("spi_slv: write32 reg 0x%x, val 0x%x\n", offset, val);
#endif
	__raw_writel(val, dws->regs + offset);
}

static inline void dw_writew(struct dw_spi *dws, u32 offset, u16 val)
{
#ifdef AX_SPI_SLV_REG_PRINT
	printk("spi_slv: write16 reg 0x%x, val 0x%x\n", offset, val);
#endif
	__raw_writew(val, dws->regs + offset);
}

static inline u32 dw_read_io_reg(struct dw_spi *dws, u32 offset)
{
	switch (dws->reg_io_width) {
	case 2:
		return dw_readw(dws, offset);
	case 4:
	default:
		return dw_readl(dws, offset);
	}
}

static inline void dw_write_io_reg(struct dw_spi *dws, u32 offset, u32 val)
{
	switch (dws->reg_io_width) {
	case 2:
		dw_writew(dws, offset, val);
		break;
	case 4:
	default:
		dw_writel(dws, offset, val);
		break;
	}
}

static inline void spi_enable_chip(struct dw_spi *dws, int enable)
{
	dw_writel(dws, DW_SPI_SSIENR, (enable ? 1 : 0));
}

static inline void spi_set_clk(struct dw_spi *dws, u16 div)
{
	dw_writel(dws, DW_SPI_BAUDR, div);
}

/* Disable IRQ bits */
static inline void spi_mask_intr(struct dw_spi *dws, u32 mask)
{
	u32 new_mask;

	new_mask = dw_readl(dws, DW_SPI_IMR) & ~mask;
	dw_writel(dws, DW_SPI_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_umask_intr(struct dw_spi *dws, u32 mask)
{
	u32 new_mask;

	new_mask = dw_readl(dws, DW_SPI_IMR) | mask;
	dw_writel(dws, DW_SPI_IMR, new_mask);
}

/*
 * This does disable the SPI controller, interrupts, and re-enable the
 * controller back. Transmit and receive FIFO buffers are cleared when the
 * device is disabled.
 */
static inline void spi_reset_chip(struct dw_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_mask_intr(dws, 0xff);
	spi_enable_chip(dws, 1);
}

static inline void spi_shutdown_chip(struct dw_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_set_clk(dws, 0);
}

/*
 * Each SPI slave device to work with dw_api controller should
 * has such a structure claiming its working mode (poll or PIO/DMA),
 * which can be save in the "controller_data" member of the
 * struct spi_device.
 */
struct dw_spi_chip {
	u8 poll_mode;	/* 1 for controller polling mode */
	u8 type;	/* SPI/SSP/MicroWire */
	void (*cs_control)(u32 command);
};

extern void dw_slv_spi_set_cs(struct spi_device *spi, bool enable);
extern int dw_slv_spi_add_host(struct device *dev, struct dw_spi *dws);
extern void dw_slv_spi_remove_host(struct dw_spi *dws);
extern int dw_slv_spi_suspend_host(struct dw_spi *dws);
extern int dw_slv_spi_resume_host(struct dw_spi *dws);

/* platform related setup */
//extern int dw_spi_mid_init(struct dw_spi *dws); /* Intel MID platforms */
#endif /* DW_SPI_HEADER_H */
