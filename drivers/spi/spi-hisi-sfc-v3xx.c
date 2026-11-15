// SPDX-License-Identifier: GPL-2.0-only
//
// HiSilicon SPI NOR V3XX / HISFC350 Flash Controller Driver
//
// Copyright (c) 2019 HiSilicon Technologies Co., Ltd.
// Author: John Garry <john.garry@huawei.com>
//
// Device tree support for the Hi3520D V200 (OF match, num-cs, flash naming
// after its DT label): Bjorn Stout <bjorn.stout@hotmail.com>
//
// Copyright (c) 2026 Dylan Corrales <deathcamel58@gmail.com>
// HISFC350 (device tree) support: AHB window reads, dual reads, bus DMA reads
// and page programs.
//
// The same SPI flash controller IP (the command interface at 0x300, VERSION
// at 0x1f8) is found in two families:
//
//  - the hi16xx server SoCs ("SFC v3xx", ACPI id HISI0341), and
//  - the HiSilicon IP-camera / NVR SoCs of the Hi3516 / Hi3518 / Hi3520D
//    generation, where the SDK calls it "hisfc350" (probed from the device
//    tree). According to the data sheets and register descriptions its
//    VERSION register reads 0x350 on all of them (only the Hi3520D was
//    measured), so the register cannot tell the SoCs apart.
//
// The driver learns what differs between SoCs from the hardware and from the
// standard device tree properties, not from a per-SoC table:
//
//  - the VERSION register: size of the DATABUF register file;
//  - GLB_CFG (mode, write protect, sampling delay) and TIMING are programmed,
//    not assumed from what a boot loader left;
//  - BUS_FLASH_SIZE / BUS_BASE_ADDR: size and position of the flash window of
//    every chip select are programmed from the window of the node (CS1 at
//    its start, CS0 in its second half, the data sheet reset layout) and the
//    size spi-nor reports for the flash, and the read prefetch and window
//    write enable of BUS_CFG1 are cleared: nothing is assumed from what a
//    boot loader programmed;
//  - GLB_CFG.flash_addr_mode: set from the address size of every op that has
//    an address (3 or 4 bytes), command interface and window reads alike, so
//    the SFC_ADDR_MODE strap and the boot loader do not decide which ops can
//    run;
//  - reg (second entry or reg-names "mem"): the AHB window, without it all
//    accesses use the command interface;
//  - num-cs: the number of chip selects;
//  - clocks, resets: the controller clock (SPI clock = clock / 2) and soft
//    reset; interrupts: command completion, and with a window the bus DMA
//    for large reads (a node without an interrupt polls);
//  - spi-rx-bus-width of the flash node: dual or quad reads.
//
// The only match data is the family quirk of the HISFC350 command interface
// (struct hisi_sfc_v3xx_data): reads are restricted to what its CMD_CFG
// register can express and writes use a single line, which the server
// variant does not need. Adding a SoC with this IP is a device tree change:
// the node lists "hisilicon,<soc>-sfc" (documented in the binding) followed
// by "hisilicon,hisfc350", which is what the driver matches. Only the
// Hi3520D V100 has been run on hardware.
//
// The Hi3516 V100 ("hisfc300" register set, different layout of every
// register) and the FMC controller of the later SoCs (drivers/mtd/spi-nor/
// controllers/hisi-sfc.c) are not handled by this driver.

#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmi.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/reset.h>
#include <linux/sched.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>

#define HISI_SFC_V3XX_VERSION (0x1f8)
#define HISI_SFC_V3XX_VERSION_HISFC350 (0x350)

#define HISI_SFC_V3XX_GLB_CFG (0x100)
#define HISI_SFC_V3XX_GLB_CFG_MODE3 BIT(0)	/* SPI mode 3, mode 0 if clear */
#define HISI_SFC_V3XX_GLB_CFG_WP_EN BIT(1)	/* WP pin forced low */
#define HISI_SFC_V3XX_GLB_CFG_CS0_ADDR_MODE BIT(2)
#define HISI_SFC_V3XX_GLB_CFG_READ_DELAY GENMASK(4, 3)	/* reserved, must be 0 */
#define HISI_SFC_V3XX_TIMING (0x110)
#define HISI_SFC_V3XX_TIMING_TSHSL GENMASK(3, 0)
#define HISI_SFC_V3XX_TIMING_TCSS GENMASK(10, 8)
#define HISI_SFC_V3XX_TIMING_TCSH GENMASK(14, 12)
#define HISI_SFC_V3XX_BUS_CFG2 (0x204)	/* reserved, must be 0 */
#define HISI_SFC_V3XX_RAW_INT_STAT (0x120)
#define HISI_SFC_V3XX_INT_STAT (0x124)
#define HISI_SFC_V3XX_INT_MASK (0x128)
#define HISI_SFC_V3XX_INT_CLR (0x12c)
/* Bus (AHB window) read configuration, HISFC350 */
#define HISI_SFC_V3XX_BUS_CFG1 (0x200)
#define HISI_SFC_V3XX_BUS_CFG1_RD_MEM_IF_TYPE GENMASK(2, 0)
#define HISI_SFC_V3XX_BUS_CFG1_RD_DUMMY_CNT GENMASK(5, 3)
#define HISI_SFC_V3XX_BUS_CFG1_RD_PREFETCH GENMASK(7, 6)
#define HISI_SFC_V3XX_BUS_CFG1_RD_INS GENMASK(15, 8)
#define HISI_SFC_V3XX_BUS_CFG1_RD_ENABLE BIT(31)
#define HISI_SFC_V3XX_BUS_CFG1_WR_MEM_IF_TYPE GENMASK(18, 16)
#define HISI_SFC_V3XX_BUS_CFG1_WR_DUMMY_CNT GENMASK(21, 19)
#define HISI_SFC_V3XX_BUS_CFG1_WR_INS GENMASK(29, 22)
#define HISI_SFC_V3XX_BUS_CFG1_WR_ENABLE BIT(30)
#define HISI_SFC_V3XX_BUS_CFG1_WR_MASK (HISI_SFC_V3XX_BUS_CFG1_WR_ENABLE | \
					HISI_SFC_V3XX_BUS_CFG1_WR_INS | \
					HISI_SFC_V3XX_BUS_CFG1_WR_DUMMY_CNT | \
					HISI_SFC_V3XX_BUS_CFG1_WR_MEM_IF_TYPE)
#define HISI_SFC_V3XX_BUS_FLASH_SIZE (0x210)
#define HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK GENMASK(3, 0)
#define HISI_SFC_V3XX_BUS_FLASH_SIZE_CS_SHIFT 8
#define HISI_SFC_V3XX_BUS_BASE_ADDR(cs) (0x214 + 4 * (cs))
#define HISI_SFC_V3XX_BUS_BASE_ADDR_MASK GENMASK(31, 16)
/* Bus DMA engine (AHB master, flash <-> memory) */
#define HISI_SFC_V3XX_BUS_DMA_CTRL (0x240)
#define HISI_SFC_V3XX_BUS_DMA_CTRL_START BIT(0)
#define HISI_SFC_V3XX_BUS_DMA_CTRL_READ BIT(1)
#define HISI_SFC_V3XX_BUS_DMA_CTRL_CS BIT(4)
#define HISI_SFC_V3XX_BUS_DMA_MEM_SADDR (0x244)
#define HISI_SFC_V3XX_BUS_DMA_FLASH_SADDR (0x248)
#define HISI_SFC_V3XX_BUS_DMA_LEN (0x24c)
#define HISI_SFC_V3XX_BUS_DMA_AHB_CTRL (0x250)
#define HISI_SFC_V3XX_BUS_DMA_AHB_CTRL_INCR_ALL GENMASK(2, 0)

#define HISI_SFC_V3XX_CMD_CFG (0x300)
#define HISI_SFC_V3XX_CMD_CFG_DATA_CNT_OFF 9
#define HISI_SFC_V3XX_CMD_CFG_RW_MSK BIT(8)
#define HISI_SFC_V3XX_CMD_CFG_DATA_EN_MSK BIT(7)
#define HISI_SFC_V3XX_CMD_CFG_DUMMY_CNT_OFF 4
#define HISI_SFC_V3XX_CMD_CFG_ADDR_EN_MSK BIT(3)
#define HISI_SFC_V3XX_CMD_CFG_CS_SEL_OFF 1
#define HISI_SFC_V3XX_CMD_CFG_START_MSK BIT(0)
#define HISI_SFC_V3XX_CMD_CFG_IO_MODE GENMASK(19, 17)
#define HISI_SFC_V3XX_CMD_INS (0x308)
#define HISI_SFC_V3XX_CMD_ADDR (0x30c)
#define HISI_SFC_V3XX_CMD_DATABUF0 (0x400)

/* Common definition of interrupt bit masks */
#define HISI_SFC_V3XX_INT_MASK_ALL (0x1ff)	/* all the masks */
#define HISI_SFC_V3XX_INT_MASK_HISFC350 GENMASK(1, 0)	/* the bits that exist */
#define HISI_SFC_V3XX_INT_MASK_CPLT BIT(0)	/* command execution complete */
#define HISI_SFC_V3XX_INT_MASK_DMA_DONE BIT(1)	/* bus DMA transfer complete */
#define HISI_SFC_V3XX_INT_MASK_PP_ERR BIT(2)	/* page program error */
/* The HISFC350 has no such bit (only bits 0 and 1), it is for the server parts */
#define HISI_SFC_V3XX_INT_MASK_IACCES BIT(5)	/* error visiting inaccessible/
						 * protected address
						 */

/* IO Mode values of the HISI_SFC_V3XX_CMD_CFG_IO_MODE field (and of BUS_CFG1) */
#define HISI_SFC_V3XX_STD 0
#define HISI_SFC_V3XX_DIDO 1
#define HISI_SFC_V3XX_DIO 2
#define HISI_SFC_V3XX_FULL_DIO 3
#define HISI_SFC_V3XX_QIQO 5
#define HISI_SFC_V3XX_QIO 6
#define HISI_SFC_V3XX_FULL_QIO 7

/*
 * The IO modes lookup table. hisi_sfc_v3xx_io_modes[(z - 1) / 2][y / 2][x / 2]
 * stands for x-y-z mode, as described in SFDP terminology. -EIO indicates
 * an invalid mode.
 */
static const int hisi_sfc_v3xx_io_modes[2][3][3] = {
	{
		{ HISI_SFC_V3XX_DIDO, HISI_SFC_V3XX_DIDO, HISI_SFC_V3XX_DIDO },
		{ HISI_SFC_V3XX_DIO, HISI_SFC_V3XX_FULL_DIO, -EIO },
		{ -EIO, -EIO, -EIO },
	},
	{
		{ HISI_SFC_V3XX_QIQO, HISI_SFC_V3XX_QIQO, HISI_SFC_V3XX_QIQO },
		{ -EIO, -EIO, -EIO },
		{ HISI_SFC_V3XX_QIO, -EIO, HISI_SFC_V3XX_FULL_QIO },
	},
};

/**
 * struct hisi_sfc_v3xx_data - match data
 * @num_cs:	number of chip selects when the node has no "num-cs"
 * @hisfc350:	the controller is the HISFC350 of the camera/NVR SoCs. Its
 *		command interface takes only what hisi_sfc_v3xx_supports_op()
 *		allows (one-line command, multi-line data only with an address
 *		phase, at most 7 dummy bytes, the IO modes of the table) and
 *		only reads may use more than one data line; flash writes and
 *		other operations stay on a single line
 */
struct hisi_sfc_v3xx_data {
	unsigned int num_cs;
	bool hisfc350;
};

/* Per-direct-mapping state */
struct hisi_sfc_v3xx_dirmap {
	void __iomem *win;		/* window of the chip select */
	resource_size_t win_size;	/* size of the window of the chip */
	u32 bus_cfg;			/* BUS_CFG1 read or write fields */
	bool write;			/* page program mapping, bus DMA only */
	u8 opcode;			/* program opcode of a write mapping */
	u8 cs;				/* controller chip select */
	u8 addr_nbytes;			/* address size of the op, 3 or 4 */
};

struct hisi_sfc_v3xx_host {
	struct device *dev;
	void __iomem *regbase;
	const struct hisi_sfc_v3xx_data *data;
	int max_cmd_dword;
	void *dma_buf;			/* bounce buffer of the bus DMA, optional */
	bool dma_failed;		/* the engine misbehaved: no more DMA */
	bool stuck;			/* command or DMA never finished: no access */
	bool dma_stuck;			/* the DMA engine never finished: its buffer must stay */
	bool irq_broken;		/* the interrupt did not arrive for a command: polling */
	bool speed_warned;
	unsigned long spi_clk_hz;	/* SPI clock the controller runs, 0 if unknown */
	dma_addr_t dma_handle;
	resource_size_t mem_phys;
	resource_size_t mem_size;
	struct completion *completion;
	u8 address_mode;		/* address size the controller is in */
	int irq;
	u32 int_mask;			/* the interrupt bits the controller has */
};

static void hisi_sfc_v3xx_disable_int(struct hisi_sfc_v3xx_host *host)
{
	writel(0, host->regbase + HISI_SFC_V3XX_INT_MASK);
}

static void hisi_sfc_v3xx_enable_int(struct hisi_sfc_v3xx_host *host)
{
	writel(host->int_mask, host->regbase + HISI_SFC_V3XX_INT_MASK);
}

static void hisi_sfc_v3xx_clear_int(struct hisi_sfc_v3xx_host *host)
{
	writel(host->int_mask, host->regbase + HISI_SFC_V3XX_INT_CLR);
}

/*
 * The interrupt status register indicates whether an error occurs
 * after per operation. Check it, and clear the interrupts for
 * next time judgement.
 */
static int hisi_sfc_v3xx_handle_completion(struct hisi_sfc_v3xx_host *host)
{
	u32 reg;

	reg = readl(host->regbase + HISI_SFC_V3XX_RAW_INT_STAT);
	hisi_sfc_v3xx_clear_int(host);

	if (reg & HISI_SFC_V3XX_INT_MASK_IACCES) {
		dev_err(host->dev, "fail to access protected address\n");
		return -EIO;
	}

	if (reg & HISI_SFC_V3XX_INT_MASK_PP_ERR) {
		dev_err(host->dev, "page program operation failed\n");
		return -EIO;
	}

	/*
	 * The other bits of the interrupt registers is not currently
	 * used and probably not be triggered in this driver. When it
	 * happens, we regard it as an unsupported error here.
	 */
	if (!(reg & HISI_SFC_V3XX_INT_MASK_CPLT)) {
		dev_err(host->dev, "unsupported error occurred, status=0x%x\n", reg);
		return -EIO;
	}

	return 0;
}

#define HISI_SFC_V3XX_WAIT_TIMEOUT_US		1000000
#define HISI_SFC_V3XX_WAIT_POLL_INTERVAL_US	10

static int hisi_sfc_v3xx_wait_cmd_idle(struct hisi_sfc_v3xx_host *host)
{
	u32 reg;

	return readl_poll_timeout(host->regbase + HISI_SFC_V3XX_CMD_CFG, reg,
				  !(reg & HISI_SFC_V3XX_CMD_CFG_START_MSK),
				  HISI_SFC_V3XX_WAIT_POLL_INTERVAL_US,
				  HISI_SFC_V3XX_WAIT_TIMEOUT_US);
}

/*
 * Wait for the command to finish. A HISFC350 on which that never happens is
 * not touched again: every later access would wait for the timeout again.
 */
static int hisi_sfc_v3xx_wait_cmd_idle_stuck(struct hisi_sfc_v3xx_host *host)
{
	int ret = hisi_sfc_v3xx_wait_cmd_idle(host);

	if (ret && host->data->hisfc350) {
		host->stuck = true;
		dev_err(host->dev, "command does not finish, controller disabled\n");
		return -EIO;
	}

	return ret;
}

/* IO mode field for an op, or -EIO if the controller cannot do the widths */
static int hisi_sfc_v3xx_io_mode(const struct spi_mem_op *op)
{
	int data_idx, addr_idx, cmd_idx;

	if (op->data.buswidth == 0 || op->data.buswidth == 1)
		return HISI_SFC_V3XX_STD;

	data_idx = (op->data.buswidth - 1) / 2;
	addr_idx = op->addr.buswidth / 2;
	cmd_idx = op->cmd.buswidth / 2;

	return hisi_sfc_v3xx_io_modes[data_idx][addr_idx][cmd_idx];
}

/*
 * Whether the controller can run an op with this many address bytes. The
 * address mode bit of GLB_CFG is writable on the HISFC350, which sets it for
 * every op (hisi_sfc_v3xx_set_addr_mode()) and so takes 3 and 4 bytes. The
 * server parts stay in the mode they started in.
 */
static bool hisi_sfc_v3xx_addr_ok(const struct hisi_sfc_v3xx_host *host,
				  unsigned int nbytes)
{
	if (!nbytes)
		return true;

	if (host->data->hisfc350)
		return nbytes == 3 || nbytes == 4;

	return nbytes == host->address_mode;
}

/* Switch the HISFC350 to the address size of the op, called with the bus idle */
static void hisi_sfc_v3xx_set_addr_mode(struct hisi_sfc_v3xx_host *host,
					unsigned int nbytes)
{
	u32 glb;

	if (!host->data->hisfc350 || !nbytes || nbytes == host->address_mode)
		return;

	glb = readl(host->regbase + HISI_SFC_V3XX_GLB_CFG);
	if (nbytes == 4)
		glb |= HISI_SFC_V3XX_GLB_CFG_CS0_ADDR_MODE;
	else
		glb &= ~HISI_SFC_V3XX_GLB_CFG_CS0_ADDR_MODE;
	writel(glb, host->regbase + HISI_SFC_V3XX_GLB_CFG);
	host->address_mode = nbytes;

	glb = readl(host->regbase + HISI_SFC_V3XX_GLB_CFG);
	if (!!(glb & HISI_SFC_V3XX_GLB_CFG_CS0_ADDR_MODE) == (nbytes != 4))
		dev_warn_once(host->dev, "the address mode bit did not take the %u byte setting\n",
			      nbytes);
}

static int hisi_sfc_v3xx_adjust_op_size(struct spi_mem *mem,
					struct spi_mem_op *op)
{
	struct spi_device *spi = mem->spi;
	struct hisi_sfc_v3xx_host *host;
	uintptr_t addr = (uintptr_t)op->data.buf.in;
	int max_byte_count;

	host = spi_controller_get_devdata(spi->controller);

	max_byte_count = host->max_cmd_dword * 4;

	if (!IS_ALIGNED(addr, 4) && op->data.nbytes >= 4)
		op->data.nbytes = 4 - (addr % 4);
	else if (op->data.nbytes > max_byte_count)
		op->data.nbytes = max_byte_count;

	return 0;
}

/* The nbytes of a dirmap template is 0, so only the direction tells a read */
static bool hisi_sfc_v3xx_op_is_read(const struct spi_mem_op *op)
{
	return op->data.dir == SPI_MEM_DATA_IN;
}

/*
 * The controller only supports Standard SPI mode, Dual mode and
 * Quad mode. Double sanitize the ops here to avoid OOB access.
 */
static bool hisi_sfc_v3xx_supports_op(struct spi_mem *mem,
				      const struct spi_mem_op *op)
{
	struct spi_device *spi = mem->spi;
	struct hisi_sfc_v3xx_host *host;

	host = spi_controller_get_devdata(spi->controller);

	if (op->data.buswidth > 4 || op->dummy.buswidth > 4 ||
	    op->addr.buswidth > 4 || op->cmd.buswidth > 4)
		return false;

	if (!hisi_sfc_v3xx_addr_ok(host, op->addr.nbytes))
		return false;

	if (!host->data->hisfc350)
		return spi_mem_default_supports_op(mem, op);

	if (op->cmd.nbytes != 1 || op->dummy.nbytes > 7)
		return false;

	if (!hisi_sfc_v3xx_op_is_read(op)) {
		/* Writes and everything that is not a read stay on one line */
		if (op->data.buswidth > 1 || op->dummy.buswidth > 1 ||
		    op->addr.buswidth > 1 || op->cmd.buswidth > 1)
			return false;
	} else {
		if (op->cmd.buswidth != 1 || hisi_sfc_v3xx_io_mode(op) < 0)
			return false;
		if (op->data.buswidth > 1 && !op->addr.nbytes)
			return false;
		/* The standard mode has no multi-line address phase */
		if (op->data.buswidth <= 1 && op->addr.nbytes &&
		    op->addr.buswidth > 1)
			return false;
	}

	return spi_mem_default_supports_op(mem, op);
}

/*
 * memcpy_{to,from}io doesn't gurantee 32b accesses - which we require for the
 * DATABUF registers -so use __io{read,write}32_copy when possible. For
 * trailing bytes, copy them byte-by-byte from the DATABUF register, as we
 * can't clobber outside the source/dest buffer.
 *
 * For efficient data read/write, we try to put any start 32b unaligned data
 * into a separate transaction in hisi_sfc_v3xx_adjust_op_size().
 */
static void hisi_sfc_v3xx_read_databuf(struct hisi_sfc_v3xx_host *host,
				       u8 *to, unsigned int len)
{
	void __iomem *from;
	int i;

	from = host->regbase + HISI_SFC_V3XX_CMD_DATABUF0;

	if (IS_ALIGNED((uintptr_t)to, 4)) {
		int words = len / 4;

		__ioread32_copy(to, from, words);

		len -= words * 4;
		if (len) {
			u32 val;

			to += words * 4;
			from += words * 4;

			val = __raw_readl(from);

			for (i = 0; i < len; i++, val >>= 8, to++)
				*to = (u8)val;
		}
	} else {
		for (i = 0; i < DIV_ROUND_UP(len, 4); i++, from += 4) {
			u32 val = __raw_readl(from);
			int j;

			for (j = 0; j < 4 && (j + (i * 4) < len);
			     to++, val >>= 8, j++)
				*to = (u8)val;
		}
	}
}

static void hisi_sfc_v3xx_write_databuf(struct hisi_sfc_v3xx_host *host,
					const u8 *from, unsigned int len)
{
	void __iomem *to;
	int i;

	to = host->regbase + HISI_SFC_V3XX_CMD_DATABUF0;

	if (IS_ALIGNED((uintptr_t)from, 4)) {
		int words = len / 4;

		__iowrite32_copy(to, from, words);

		len -= words * 4;
		if (len) {
			u32 val = 0;

			to += words * 4;
			from += words * 4;

			for (i = 0; i < len; i++, from++)
				val |= (u32)*from << i * 8;
			__raw_writel(val, to);
		}

	} else {
		for (i = 0; i < DIV_ROUND_UP(len, 4); i++, to += 4) {
			u32 val = 0;
			int j;

			for (j = 0; j < 4 && (j + (i * 4) < len);
			     from++, j++)
				val |= (u32)*from << j * 8;
			__raw_writel(val, to);
		}
	}
}

static int hisi_sfc_v3xx_start_bus(struct hisi_sfc_v3xx_host *host,
				   const struct spi_mem_op *op,
				   u8 chip_select)
{
	int len = op->data.nbytes, buswidth_mode;
	u32 config = 0;

	if (op->addr.nbytes) {
		hisi_sfc_v3xx_set_addr_mode(host, op->addr.nbytes);
		config |= HISI_SFC_V3XX_CMD_CFG_ADDR_EN_MSK;
	}

	buswidth_mode = hisi_sfc_v3xx_io_mode(op);
	if (buswidth_mode < 0)
		return buswidth_mode;
	config |= FIELD_PREP(HISI_SFC_V3XX_CMD_CFG_IO_MODE, buswidth_mode);

	if (op->data.dir != SPI_MEM_NO_DATA) {
		config |= (len - 1) << HISI_SFC_V3XX_CMD_CFG_DATA_CNT_OFF;
		config |= HISI_SFC_V3XX_CMD_CFG_DATA_EN_MSK;
	}

	if (op->data.dir == SPI_MEM_DATA_IN)
		config |= HISI_SFC_V3XX_CMD_CFG_RW_MSK;

	config |= op->dummy.nbytes << HISI_SFC_V3XX_CMD_CFG_DUMMY_CNT_OFF |
		  chip_select << HISI_SFC_V3XX_CMD_CFG_CS_SEL_OFF |
		  HISI_SFC_V3XX_CMD_CFG_START_MSK;

	writel(op->addr.val, host->regbase + HISI_SFC_V3XX_CMD_ADDR);
	writel(op->cmd.opcode, host->regbase + HISI_SFC_V3XX_CMD_INS);

	writel(config, host->regbase + HISI_SFC_V3XX_CMD_CFG);

	return 0;
}

static int hisi_sfc_v3xx_generic_exec_op(struct hisi_sfc_v3xx_host *host,
					 const struct spi_mem_op *op,
					 u8 chip_select)
{
	DECLARE_COMPLETION_ONSTACK(done);
	bool use_irq = host->irq && !host->irq_broken;
	int ret;

	/* Drop stale status before the mask opens */
	hisi_sfc_v3xx_clear_int(host);

	if (use_irq) {
		WRITE_ONCE(host->completion, &done);
		hisi_sfc_v3xx_enable_int(host);
	}

	if (op->data.dir == SPI_MEM_DATA_OUT)
		hisi_sfc_v3xx_write_databuf(host, op->data.buf.out, op->data.nbytes);

	ret = hisi_sfc_v3xx_start_bus(host, op, chip_select);
	if (ret) {
		if (use_irq) {
			hisi_sfc_v3xx_disable_int(host);
			synchronize_irq(host->irq);
			WRITE_ONCE(host->completion, NULL);
		}
		return ret;
	}

	if (use_irq) {
		ret = wait_for_completion_timeout(&done,
						  usecs_to_jiffies(HISI_SFC_V3XX_WAIT_TIMEOUT_US));
		if (!ret)
			ret = -ETIMEDOUT;
		else
			ret = 0;

		hisi_sfc_v3xx_disable_int(host);
		synchronize_irq(host->irq);
		WRITE_ONCE(host->completion, NULL);

		/*
		 * Completed after the wait gave up and before the interrupt
		 * was masked: a late wakeup, not a missing interrupt.
		 */
		if (ret == -ETIMEDOUT && completion_done(&done))
			ret = 0;

		if (ret == -ETIMEDOUT && host->data->hisfc350) {
			/*
			 * No interrupt. If the command finished anyway take the
			 * result and poll from now on (a wrong interrupt in a
			 * device tree must not make the flash unusable).
			 */
			if (hisi_sfc_v3xx_wait_cmd_idle_stuck(host))
				return -EIO;

			dev_warn(host->dev, "command done interrupt missing, polling from now on\n");
			host->irq_broken = true;
			ret = 0;
		}
	} else {
		ret = hisi_sfc_v3xx_wait_cmd_idle_stuck(host);
	}
	if (hisi_sfc_v3xx_handle_completion(host) || ret)
		return -EIO;

	if (op->data.dir == SPI_MEM_DATA_IN)
		hisi_sfc_v3xx_read_databuf(host, op->data.buf.in, op->data.nbytes);

	return 0;
}

/*
 * The controller has no divider: the SPI clock is always the controller clock
 * divided by two, spi-max-frequency of the flash node cannot lower it.
 */
static void hisi_sfc_v3xx_check_speed(struct hisi_sfc_v3xx_host *host,
				      struct spi_device *spi)
{
	if (host->speed_warned || !host->spi_clk_hz ||
	    !spi->max_speed_hz || spi->max_speed_hz >= host->spi_clk_hz)
		return;

	host->speed_warned = true;
	dev_warn(host->dev, "SPI clock %lu Hz exceeds spi-max-frequency %u Hz of the flash: the controller has no divider\n",
		 host->spi_clk_hz, spi->max_speed_hz);
}

static int hisi_sfc_v3xx_exec_op(struct spi_mem *mem,
				 const struct spi_mem_op *op)
{
	struct hisi_sfc_v3xx_host *host;
	struct spi_device *spi = mem->spi;
	u8 chip_select;

	host = spi_controller_get_devdata(spi->controller);
	if (host->stuck)
		return -EIO;

	hisi_sfc_v3xx_check_speed(host, spi);
	chip_select = spi_get_chipselect(spi, 0);

	return hisi_sfc_v3xx_generic_exec_op(host, op, chip_select);
}

/*
 * Bus DMA reads and page programs (HISFC350 BUS_DMA_* registers), used when
 * the node has an interrupt: the CPU sleeps while the engine runs. Reads
 * shorter than HISI_SFC_V3XX_DMA_MIN stay on the window, the DMA setup and
 * the bounce copy cost more than they save (measured on the Hi3520D V100:
 * 64 B reads are 1.8x faster through the window, from 256 B on DMA is faster
 * and uses less CPU time; 128 B was not measured on DMA).
 */
#define HISI_SFC_V3XX_DMA_MIN		256
#define HISI_SFC_V3XX_WIN_SLICE		SZ_4K
#define HISI_SFC_V3XX_READ_MAX		SZ_64K

/* Why a direct mapping cannot be used for the op, or NULL if it can */
static const char *hisi_sfc_v3xx_dirmap_declined(struct hisi_sfc_v3xx_host *host,
						 struct spi_mem *mem,
						 const struct spi_mem_op *op)
{
	if (!host->mem_size)
		return "no flash window in the node";
	if (op->data.dir == SPI_MEM_NO_DATA)
		return "no data phase";
	/* Page programs go through the bus DMA engine, one line only */
	if (op->data.dir == SPI_MEM_DATA_OUT) {
		if (!host->dma_buf)
			return "no bus DMA (needs the window and an interrupt)";
		if (op->data.buswidth > 1 || op->addr.buswidth > 1)
			return "page programs use one line";
		/*
		 * Nothing shows that the engine sends the address in the mode
		 * of GLB_CFG, as the command interface does, and a wrong
		 * program cannot be undone: only 3 byte addresses are used.
		 */
		if (op->addr.nbytes != 3)
			return "page programs are only verified with 3 byte addresses";
	}
	if (!op->addr.nbytes)
		return "no address phase";
	/* The rules of the command interface apply to the window read too */
	if (!hisi_sfc_v3xx_supports_op(mem, op))
		return "operation not supported by the controller";

	return NULL;
}

/*
 * Set up the window of a hardware chip select for a flash of @need bytes. The
 * window registers are not assumed to hold anything useful: init_hw() marks
 * every chip select as having no flash, and here the layout of the data
 * sheet reset state is programmed for this chip select, chip select 1 at the
 * start of the window of the node and chip select 0 at its second half, each
 * large enough for the flash that spi-nor reports. The windows of the two
 * chip selects therefore never overlap, and a window that is already large
 * enough is not shrunk.
 */
static int hisi_sfc_v3xx_get_window(struct hisi_sfc_v3xx_host *host,
				    unsigned int cs, u64 need,
				    resource_size_t *base, resource_size_t *size)
{
	resource_size_t slot = host->mem_size / 2;
	unsigned int shift = HISI_SFC_V3XX_BUS_FLASH_SIZE_CS_SHIFT * cs;
	u32 val, code, found_code;
	resource_size_t found_base;

	if (need > slot) {
		dev_info(host->dev, "flash of %llu bytes does not fit the %pa byte window slot, using the command interface\n",
			 need, &slot);
		return -EOPNOTSUPP;
	}

	/* 1 = 512 kbit (64 KiB) ... 9 = 128 Mbit (16 MiB) ... 0xf = 8 Gbit */
	code = max_t(int, order_base_2(need) - 15, 1);
	if (code > HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK) {
		dev_warn(host->dev, "flash of %llu bytes is above the largest window size code, using the command interface\n",
			 need);
		return -EOPNOTSUPP;
	}
	*size = BIT_ULL(code + 15);
	if (*size > slot) {
		dev_warn(host->dev, "window of %pa bytes for a flash of %llu bytes does not fit the %pa byte slot, using the command interface\n",
			 size, need, &slot);
		return -EOPNOTSUPP;
	}
	*base = host->mem_phys + (cs ? 0 : slot);
	if (*base & ~(resource_size_t)HISI_SFC_V3XX_BUS_BASE_ADDR_MASK) {
		dev_info(host->dev, "flash window base %pa is not 64 KiB aligned, using the command interface\n",
			 base);
		return -EOPNOTSUPP;
	}

	val = readl(host->regbase + HISI_SFC_V3XX_BUS_FLASH_SIZE);
	found_code = (val >> shift) & HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK;
	found_base = readl(host->regbase + HISI_SFC_V3XX_BUS_BASE_ADDR(cs)) &
		     HISI_SFC_V3XX_BUS_BASE_ADDR_MASK;
	if (found_base == *base && found_code >= code &&
	    BIT_ULL(found_code + 15) <= slot) {
		*size = BIT_ULL(found_code + 15);
		return 0;
	}

	writel((val & ~(HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK << shift)) | (code << shift),
	       host->regbase + HISI_SFC_V3XX_BUS_FLASH_SIZE);
	writel(*base, host->regbase + HISI_SFC_V3XX_BUS_BASE_ADDR(cs));

	/* Read the window back: a controller that ignores it must not be mapped */
	val = readl(host->regbase + HISI_SFC_V3XX_BUS_FLASH_SIZE);
	if (((val >> shift) & HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK) != code ||
	    (readl(host->regbase + HISI_SFC_V3XX_BUS_BASE_ADDR(cs)) &
	     HISI_SFC_V3XX_BUS_BASE_ADDR_MASK) != *base) {
		dev_warn(host->dev, "flash window of chip select %u does not hold its value, using the command interface\n",
			 cs);
		return -EOPNOTSUPP;
	}

	dev_info(host->dev, "programmed the flash window of chip select %u: %pa, %pa bytes (found: %pa, size code %u)\n",
		 cs, base, size, &found_base, found_code);

	return 0;
}

/*
 * Program the write fields of BUS_CFG1 (opcode, I/O mode, dummy bytes and the
 * window write enable, @cfg) and read them back. Returns -EIO if they did not
 * stick; @found is what the register held.
 */
static int hisi_sfc_v3xx_set_write_cfg(struct hisi_sfc_v3xx_host *host,
				       u32 cfg, u32 *found)
{
	u32 val = readl(host->regbase + HISI_SFC_V3XX_BUS_CFG1);

	writel((val & ~HISI_SFC_V3XX_BUS_CFG1_WR_MASK) | cfg,
	       host->regbase + HISI_SFC_V3XX_BUS_CFG1);
	/* The read-back also makes sure the write is in place */
	*found = readl(host->regbase + HISI_SFC_V3XX_BUS_CFG1) &
		 HISI_SFC_V3XX_BUS_CFG1_WR_MASK;

	return *found == cfg ? 0 : -EIO;
}

/* The window write enable is only set while a DMA page program runs */
static void hisi_sfc_v3xx_clear_wr_enable(struct hisi_sfc_v3xx_host *host)
{
	u32 val = readl(host->regbase + HISI_SFC_V3XX_BUS_CFG1);

	writel(val & ~HISI_SFC_V3XX_BUS_CFG1_WR_ENABLE,
	       host->regbase + HISI_SFC_V3XX_BUS_CFG1);
}

static int hisi_sfc_v3xx_dirmap_create(struct spi_mem_dirmap_desc *desc)
{
	struct spi_device *spi = desc->mem->spi;
	struct hisi_sfc_v3xx_host *host = spi_controller_get_devdata(spi->controller);
	const struct spi_mem_op *op = &desc->info.op_tmpl;
	struct hisi_sfc_v3xx_dirmap *dm;
	const char *reason;
	resource_size_t base, size;
	unsigned int cs;
	int mode, ret;

	reason = hisi_sfc_v3xx_dirmap_declined(host, desc->mem, op);
	if (reason) {
		dev_dbg(&spi->dev, "direct %s declined: %s\n",
			op->data.dir == SPI_MEM_DATA_OUT ? "write" : "read", reason);
		return -EOPNOTSUPP;
	}

	mode = hisi_sfc_v3xx_io_mode(op);
	cs = spi_get_chipselect(spi, 0);

	dm = devm_kzalloc(&spi->dev, sizeof(*dm), GFP_KERNEL);
	if (!dm)
		return -ENOMEM;

	dm->write = op->data.dir == SPI_MEM_DATA_OUT;

	/*
	 * Program the window of this chip: the bus DMA engine runs the flash
	 * command through it, so a write mapping needs the window as much as a
	 * read one. Only reads map it, and only the window of this chip, not
	 * the 64 MiB of both selects.
	 */
	ret = hisi_sfc_v3xx_get_window(host, cs,
				       desc->info.offset + desc->info.length,
				       &base, &size);
	if (ret)
		return ret;

	dm->win_size = size;
	if (!dm->write) {
		dm->win = devm_ioremap_wc(&spi->dev, base, size);
		if (!dm->win)
			return -ENOMEM;
	}

	hisi_sfc_v3xx_check_speed(host, spi);

	dm->addr_nbytes = op->addr.nbytes;
	dm->cs = cs;
	dm->opcode = op->cmd.opcode;
	if (dm->write)
		dm->bus_cfg = HISI_SFC_V3XX_BUS_CFG1_WR_ENABLE |
			      FIELD_PREP(HISI_SFC_V3XX_BUS_CFG1_WR_INS, op->cmd.opcode) |
			      FIELD_PREP(HISI_SFC_V3XX_BUS_CFG1_WR_DUMMY_CNT, op->dummy.nbytes) |
			      FIELD_PREP(HISI_SFC_V3XX_BUS_CFG1_WR_MEM_IF_TYPE, mode);
	else
		dm->bus_cfg = HISI_SFC_V3XX_BUS_CFG1_RD_ENABLE |
			      FIELD_PREP(HISI_SFC_V3XX_BUS_CFG1_RD_INS, op->cmd.opcode) |
			      FIELD_PREP(HISI_SFC_V3XX_BUS_CFG1_RD_DUMMY_CNT, op->dummy.nbytes) |
			      FIELD_PREP(HISI_SFC_V3XX_BUS_CFG1_RD_MEM_IF_TYPE, mode);

	/*
	 * The write fields of BUS_CFG1 are not known to stick on every
	 * HISFC350: check them once here and decline the mapping if they do
	 * not, the writes then go through the command interface.
	 */
	if (dm->write) {
		u32 found;

		ret = hisi_sfc_v3xx_wait_cmd_idle_stuck(host);
		if (ret)
			return ret;

		ret = hisi_sfc_v3xx_set_write_cfg(host, dm->bus_cfg, &found);
		hisi_sfc_v3xx_clear_wr_enable(host);
		if (ret) {
			dev_warn(host->dev, "BUS_CFG1 write fields do not stick (wrote 0x%x, read 0x%x), page programs use the command interface\n",
				 dm->bus_cfg, found);
			devm_kfree(&spi->dev, dm);
			return -EOPNOTSUPP;
		}
	}

	desc->priv = dm;

	dev_dbg(&spi->dev, "direct %s: opcode 0x%02x, %u-%u-%u, %u dummy byte(s)%s\n",
		dm->write ? "write" : "read", op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->data.buswidth, op->dummy.nbytes,
		host->dma_buf ? ", bus DMA" : "");

	return 0;
}

/*
 * One bus DMA transfer of at most HISI_SFC_V3XX_DMA_CHUNK bytes between the
 * flash and the bounce buffer. The engine runs the read command programmed in
 * BUS_CFG1 (opcode, I/O mode, dummy bytes) like a window read does, or the
 * page program of the write fields. The length register takes the byte count
 * minus one (vendor driver, data sheet is silent), so for a read the byte
 * after the chunk is poisoned beforehand to notice an engine that writes one
 * byte too many.
 *
 * chip_offset is the offset within the flash of dm->cs. That is what
 * BUS_DMA_FLASH_SADDR takes together with the chip select bit of
 * BUS_DMA_CTRL, not an offset within the AHB window of the node (the window
 * of chip select 0 starts half way into it). The data sheet only says "DMA
 * flash start address"; the vendor driver passes the offset within the chip:
 * it subtracts the size of a flash before it moves on to the next one and
 * sets the chip select bit for that one. Only chip select 1 ran on hardware,
 * where the offset within the chip and within the window are equal, so the
 * chip-relative reading is unverified for chip select 0.
 */
#define HISI_SFC_V3XX_DMA_CHUNK		4096
#define HISI_SFC_V3XX_DMA_ALIGN		256
#define HISI_SFC_V3XX_DMA_TIMEOUT_US	200000
#define HISI_SFC_V3XX_DMA_DRAIN_US	100000

/*
 * A DMA engine that never finished may still write to the bounce buffer (a
 * read) or read from it (a page program: freeing it could program arbitrary
 * memory into the flash). The driver does not reset the controller at unbind
 * (it may be the boot flash), and nothing in the data sheet says that a reset
 * aborts a running transfer: leak the buffer rather than hand its pages
 * back. The controller stays disabled until the reset that the next probe
 * pulses recovers the engine.
 */
static void hisi_sfc_v3xx_free_dma(void *data)
{
	struct hisi_sfc_v3xx_host *host = data;

	if (host->dma_stuck) {
		dev_err(host->dev, "bus DMA bounce buffer leaked, reboot to recover\n");
		return;
	}

	dma_free_coherent(host->dev, 2 * HISI_SFC_V3XX_DMA_CHUNK, host->dma_buf,
			  host->dma_handle);
}

/* The engine needs its interrupt, so it is off when polling was forced */
static bool hisi_sfc_v3xx_dma_usable(const struct hisi_sfc_v3xx_host *host)
{
	return host->dma_buf && !host->dma_failed && !host->irq_broken;
}

static int hisi_sfc_v3xx_dma_chunk(struct hisi_sfc_v3xx_host *host,
				   struct hisi_sfc_v3xx_dirmap *dm,
				   u64 chip_offset, size_t len)
{
	DECLARE_COMPLETION_ONSTACK(done);
	u8 *buf = host->dma_buf;
	u32 raw;
	int ret = 0;

	if (!dm->write)
		buf[len] = 0xa5;

	/* Drop stale status before the mask opens */
	hisi_sfc_v3xx_clear_int(host);
	WRITE_ONCE(host->completion, &done);
	writel(HISI_SFC_V3XX_INT_MASK_DMA_DONE, host->regbase + HISI_SFC_V3XX_INT_MASK);
	writel(host->dma_handle, host->regbase + HISI_SFC_V3XX_BUS_DMA_MEM_SADDR);
	writel(chip_offset, host->regbase + HISI_SFC_V3XX_BUS_DMA_FLASH_SADDR);
	writel(len - 1, host->regbase + HISI_SFC_V3XX_BUS_DMA_LEN);
	writel(HISI_SFC_V3XX_BUS_DMA_AHB_CTRL_INCR_ALL,
	       host->regbase + HISI_SFC_V3XX_BUS_DMA_AHB_CTRL);
	writel((dm->write ? 0 : HISI_SFC_V3XX_BUS_DMA_CTRL_READ) |
	       (dm->cs ? HISI_SFC_V3XX_BUS_DMA_CTRL_CS : 0) |
	       HISI_SFC_V3XX_BUS_DMA_CTRL_START,
	       host->regbase + HISI_SFC_V3XX_BUS_DMA_CTRL);

	/* Sleep until the engine raises its done interrupt. */
	if (!wait_for_completion_timeout(&done,
					 usecs_to_jiffies(HISI_SFC_V3XX_DMA_TIMEOUT_US)))
		ret = -ETIMEDOUT;

	hisi_sfc_v3xx_disable_int(host);
	synchronize_irq(host->irq);
	WRITE_ONCE(host->completion, NULL);

	/*
	 * The done interrupt may have been handled after the wait gave up and
	 * before it was masked (synchronize_irq() waited for the handler to
	 * return). That is a late wakeup, not a missing interrupt: the
	 * transfer is over, so take it as the success it is.
	 */
	if (ret && completion_done(&done))
		ret = 0;

	if (ret) {
		/*
		 * The engine has no abort. Wait for it to finish before
		 * anything else touches the controller, and refuse all
		 * further accesses if it never does.
		 */
		if (readl_poll_timeout(host->regbase + HISI_SFC_V3XX_BUS_DMA_CTRL, raw,
				       !(raw & HISI_SFC_V3XX_BUS_DMA_CTRL_START),
				       100, HISI_SFC_V3XX_DMA_DRAIN_US)) {
			host->stuck = true;
			host->dma_stuck = true;
			dev_err(host->dev, "bus DMA does not finish, controller disabled\n");
			return -EIO;
		}
	}

	raw = readl(host->regbase + HISI_SFC_V3XX_RAW_INT_STAT);
	hisi_sfc_v3xx_clear_int(host);

	if (!(raw & HISI_SFC_V3XX_INT_MASK_DMA_DONE)) {
		dev_err(host->dev, "bus DMA ended without the done status (0x%x)\n", raw);
		return -EIO;
	}

	if (ret) {
		/* Finished late: the data is good, but do not rely on the interrupt */
		dev_warn(host->dev, "bus DMA done interrupt missing, not using DMA from now on\n");
		host->dma_failed = true;
	}

	if (!dm->write && buf[len] != 0xa5)
		dev_warn_once(host->dev, "bus DMA wrote past the requested length\n");

	return 0;
}

/*
 * len must be a multiple of HISI_SFC_V3XX_DMA_ALIGN, chip_offset aligned to it.
 * Returns the number of bytes read, which is less than len if the engine was
 * given up on, or a negative error.
 */
static ssize_t hisi_sfc_v3xx_dma_read(struct hisi_sfc_v3xx_host *host,
				      struct hisi_sfc_v3xx_dirmap *dm,
				      u64 chip_offset, size_t len, u8 *buf)
{
	size_t done = 0;

	while (done < len) {
		size_t n = min_t(size_t, len - done, HISI_SFC_V3XX_DMA_CHUNK);
		int ret;

		ret = hisi_sfc_v3xx_dma_chunk(host, dm, chip_offset, n);
		if (ret)
			return ret;

		memcpy(buf + done, host->dma_buf, n);
		done += n;
		chip_offset += n;
		cond_resched();

		/* The engine was given up on (late done interrupt): the caller finishes */
		if (host->dma_failed)
			break;
	}

	return done;
}

/*
 * Every access of the window runs a flash read command; a single 32-bit load
 * costs a complete command (opcode, address, 4 data bytes), a burst of loads
 * is served by one command. The window is mapped write-combined so that
 * memcpy_fromio() can burst (on ARM it is a word-wise copy with load-multiple
 * instructions). The CPU stalls on every access, so copy in slices and let
 * others run in between.
 */
static void hisi_sfc_v3xx_win_copy(void *buf, const void __iomem *src, size_t len)
{
	while (len) {
		size_t n = min_t(size_t, len, HISI_SFC_V3XX_WIN_SLICE);

		memcpy_fromio(buf, src, n);
		buf += n;
		src += n;
		len -= n;
		if (len)
			cond_resched();
	}
}

/*
 * The spi-mem core serializes dirmap reads and writes with exec_op(), so
 * BUS_CFG1 needs no lock of its own. Its read fields are programmed for every
 * call, the driver keeps no copy of them.
 */
static ssize_t hisi_sfc_v3xx_dirmap_read(struct spi_mem_dirmap_desc *desc,
					 u64 offs, size_t len, void *buf)
{
	struct hisi_sfc_v3xx_host *host =
		spi_controller_get_devdata(desc->mem->spi->controller);
	struct hisi_sfc_v3xx_dirmap *dm = desc->priv;
	u32 mask = HISI_SFC_V3XX_BUS_CFG1_RD_ENABLE |
		   HISI_SFC_V3XX_BUS_CFG1_RD_INS |
		   HISI_SFC_V3XX_BUS_CFG1_RD_DUMMY_CNT |
		   HISI_SFC_V3XX_BUS_CFG1_RD_MEM_IF_TYPE;
	const void __iomem *src;
	u64 flash_addr;
	u32 val;
	int ret;

	if (host->stuck)
		return -EIO;

	/*
	 * Offset within the flash of this chip select. The mapped window of the
	 * chip select starts at its offset 0, so the window reads and the bus
	 * DMA (which takes the offset within the chip as well) use the same
	 * value, whatever chip select it is.
	 *
	 * Never read past the mapped window.
	 */
	flash_addr = desc->info.offset + offs;
	if (flash_addr >= dm->win_size)
		return -EINVAL;
	len = min_t(u64, len, dm->win_size - flash_addr);

	/* Short reads let the spi-mem core drop its locks between the pieces */
	len = min_t(size_t, len, HISI_SFC_V3XX_READ_MAX);

	/* Never reprogram the read mode under a running command */
	ret = hisi_sfc_v3xx_wait_cmd_idle_stuck(host);
	if (ret)
		return ret;

	/* The window read sends the address in the mode of GLB_CFG too */
	hisi_sfc_v3xx_set_addr_mode(host, dm->addr_nbytes);

	val = readl(host->regbase + HISI_SFC_V3XX_BUS_CFG1);
	writel((val & ~mask) | dm->bus_cfg, host->regbase + HISI_SFC_V3XX_BUS_CFG1);
	/* Make sure the read mode is in place before the window is read */
	readl(host->regbase + HISI_SFC_V3XX_BUS_CFG1);

	src = dm->win + flash_addr;

	if (hisi_sfc_v3xx_dma_usable(host) && len >= HISI_SFC_V3XX_DMA_MIN) {
		/*
		 * DMA only whole aligned blocks; an unaligned head and the
		 * tail go through the window.
		 */
		size_t head = -flash_addr & (HISI_SFC_V3XX_DMA_ALIGN - 1);
		size_t body = (len - head) & ~(size_t)(HISI_SFC_V3XX_DMA_ALIGN - 1);
		ssize_t n;

		if (head <= len && body) {
			hisi_sfc_v3xx_win_copy(buf, src, head);
			n = hisi_sfc_v3xx_dma_read(host, dm, flash_addr + head,
						   body, buf + head);
			if (n >= 0) {
				/* the tail, and the rest if the engine was given up on */
				hisi_sfc_v3xx_win_copy(buf + head + n, src + head + n,
						       len - head - n);
				return len;
			}

			if (host->stuck)
				return -EIO;

			/* Not using the engine any more: serve this read from the window */
			dev_err(host->dev, "bus DMA failed (%zd), window reads only\n", n);
			host->dma_failed = true;
		}
	}

	hisi_sfc_v3xx_win_copy(buf, src, len);

	return len;
}

/*
 * Page programs. A whole aligned 256 byte page goes out through the bus DMA
 * engine as ONE program command with the write opcode of BUS_CFG1; the engine
 * appears to issue its own write enable, to wait for the flash to finish
 * programming and to raise the done interrupt afterwards. That was measured
 * on the Hi3520D V100 and is not in the data sheet, so spi-nor still sets
 * the write enable latch and reads the flash status after every program and
 * nothing here depends on it. Everything else (a partial page, an unaligned
 * start, or any write after the engine was given up on) is programmed in
 * command mode in pieces of the data buffer size, which is what spi-nor does
 * itself when a controller has no write mapping; spi-nor sets the write
 * enable latch before each call and calls again for the rest of a short
 * return. The window write enable of BUS_CFG1 is set only while a page is
 * programmed through the engine (and, with the command interface idle, for
 * the read-back check when the mapping is created), and the write fields are
 * read back after they are programmed.
 *
 * A failed page program is NOT retried here, neither through the engine nor
 * in command mode: the page may be partly programmed, so the error goes up
 * and the caller decides. After any DMA failure the engine stays off for
 * reads and writes alike (dma_failed is never cleared), and when a command or
 * the engine never finishes the controller refuses everything (stuck), so no
 * command can run while BUS_DMA_CTRL.START is set.
 */
static ssize_t hisi_sfc_v3xx_dirmap_write(struct spi_mem_dirmap_desc *desc,
					  u64 offs, size_t len, const void *buf)
{
	struct hisi_sfc_v3xx_host *host =
		spi_controller_get_devdata(desc->mem->spi->controller);
	struct hisi_sfc_v3xx_dirmap *dm = desc->priv;
	u64 flash_addr = desc->info.offset + offs;
	size_t n;
	u32 found;
	int ret;

	if (host->stuck)
		return -EIO;

	/*
	 * The engine takes a 32 bit address and the flash sees 24 bits: a
	 * program past the window would land somewhere else and cannot be
	 * undone. spi-nor and mtd never ask for it.
	 */
	if (flash_addr >= dm->win_size)
		return -EINVAL;
	len = min_t(u64, len, dm->win_size - flash_addr);
	n = min_t(size_t, len, -flash_addr & (HISI_SFC_V3XX_DMA_ALIGN - 1) ?:
		  HISI_SFC_V3XX_DMA_ALIGN);

	ret = hisi_sfc_v3xx_wait_cmd_idle_stuck(host);
	if (ret)
		return ret;

	/* The page program through the engine sends the address in the mode of GLB_CFG */
	hisi_sfc_v3xx_set_addr_mode(host, dm->addr_nbytes);

	if (!hisi_sfc_v3xx_dma_usable(host) || n < HISI_SFC_V3XX_DMA_ALIGN ||
	    (flash_addr & (HISI_SFC_V3XX_DMA_ALIGN - 1))) {
		struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(dm->opcode, 1),
				   SPI_MEM_OP_ADDR(dm->addr_nbytes, flash_addr, 1),
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_DATA_OUT(min_t(size_t, n, host->max_cmd_dword * 4),
						       buf, 1));

		ret = hisi_sfc_v3xx_generic_exec_op(host, &op, dm->cs);
		return ret ? ret : op.data.nbytes;
	}

	/* Window writes stay disabled except while this page is programmed */
	ret = hisi_sfc_v3xx_set_write_cfg(host, dm->bus_cfg, &found);
	if (ret) {
		hisi_sfc_v3xx_clear_wr_enable(host);
		dev_err(host->dev, "BUS_CFG1 write fields do not stick (wrote 0x%x, read 0x%x)\n",
			dm->bus_cfg, found);
		return ret;
	}

	memcpy(host->dma_buf, buf, HISI_SFC_V3XX_DMA_ALIGN);
	ret = hisi_sfc_v3xx_dma_chunk(host, dm, flash_addr, HISI_SFC_V3XX_DMA_ALIGN);
	hisi_sfc_v3xx_clear_wr_enable(host);
	if (ret) {
		if (!host->stuck)
			host->dma_failed = true;
		dev_err(host->dev, "bus DMA page program failed (%d)\n", ret);
		return ret;
	}

	return HISI_SFC_V3XX_DMA_ALIGN;
}

/*
 * Name the MTD device after the "label" of the flash node when it has one.
 * Boot loaders that describe the layout with "mtdparts=<name>:..." on the
 * kernel command line (the HiSilicon SDK U-Boot uses "hi_sfc") can only match
 * an MTD device of that name. Without a label the SPI device name is kept.
 */
static const char *hisi_sfc_v3xx_get_name(struct spi_mem *mem)
{
	const char *name;

	if (!device_property_read_string(&mem->spi->dev, "label", &name))
		return name;

	return dev_name(&mem->spi->dev);
}

static const struct spi_controller_mem_ops hisi_sfc_v3xx_mem_ops = {
	.get_name = hisi_sfc_v3xx_get_name,
	.dirmap_create = hisi_sfc_v3xx_dirmap_create,
	.dirmap_read = hisi_sfc_v3xx_dirmap_read,
	.dirmap_write = hisi_sfc_v3xx_dirmap_write,
	.adjust_op_size = hisi_sfc_v3xx_adjust_op_size,
	.supports_op = hisi_sfc_v3xx_supports_op,
	.exec_op = hisi_sfc_v3xx_exec_op,
};

/*
 * Put the registers the driver relies on into a known state instead of
 * trusting what a boot loader or an earlier driver left: SPI mode 0, the WP
 * pin not forced low, no read sampling delay (the data sheet says the field
 * must be 0), BUS_CFG2 (reserved, must be 0), the timing of the reset state of
 * the data sheet, and for the window no read prefetch and no window writes.
 * The other registers are written in full for every command, DMA transfer or
 * window mapping. The flash window registers are set up with each mapping,
 * see hisi_sfc_v3xx_get_window().
 *
 * The exception is the address mode bit of GLB_CFG: it resets to the level
 * of the SFC_ADDR_MODE pin and is kept here. The driver sets it for every op
 * (hisi_sfc_v3xx_set_addr_mode()), because the pin level, or a boot loader
 * that switched to 4 byte addresses, must not decide which ops can run.
 */
static void hisi_sfc_v3xx_init_hw(struct hisi_sfc_v3xx_host *host)
{
	u32 glb_found = readl(host->regbase + HISI_SFC_V3XX_GLB_CFG);
	u32 timing_found = readl(host->regbase + HISI_SFC_V3XX_TIMING);
	u32 bus_cfg2_found = readl(host->regbase + HISI_SFC_V3XX_BUS_CFG2);
	u32 glb = glb_found & ~(HISI_SFC_V3XX_GLB_CFG_MODE3 |
				HISI_SFC_V3XX_GLB_CFG_WP_EN |
				HISI_SFC_V3XX_GLB_CFG_READ_DELAY);
	u32 timing = FIELD_PREP(HISI_SFC_V3XX_TIMING_TSHSL, 0xf) |
		     FIELD_PREP(HISI_SFC_V3XX_TIMING_TCSS, 6) |
		     FIELD_PREP(HISI_SFC_V3XX_TIMING_TCSH, 6);

	if (glb != glb_found || timing != timing_found || bus_cfg2_found)
		dev_info(host->dev, "controller was not in its default state (GLB_CFG 0x%x, TIMING 0x%x, BUS_CFG2 0x%x), reprogrammed\n",
			 glb_found, timing_found, bus_cfg2_found);

	writel(glb, host->regbase + HISI_SFC_V3XX_GLB_CFG);
	writel(timing, host->regbase + HISI_SFC_V3XX_TIMING);
	writel(0, host->regbase + HISI_SFC_V3XX_BUS_CFG2);

	if (host->mem_size) {
		u32 val = readl(host->regbase + HISI_SFC_V3XX_BUS_CFG1);
		u32 size = readl(host->regbase + HISI_SFC_V3XX_BUS_FLASH_SIZE);

		/* No read prefetch, and no flash writes through the window */
		writel(val & ~(HISI_SFC_V3XX_BUS_CFG1_RD_PREFETCH |
			       HISI_SFC_V3XX_BUS_CFG1_WR_ENABLE),
		       host->regbase + HISI_SFC_V3XX_BUS_CFG1);

		/*
		 * No flash is mapped until a mapping programs its window, so
		 * that a window left by the boot loader cannot overlap the
		 * one of another chip select.
		 */
		size &= ~(HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK <<
			  HISI_SFC_V3XX_BUS_FLASH_SIZE_CS_SHIFT);
		size &= ~HISI_SFC_V3XX_BUS_FLASH_SIZE_MASK;
		writel(size, host->regbase + HISI_SFC_V3XX_BUS_FLASH_SIZE);
	}
}

static irqreturn_t hisi_sfc_v3xx_isr(int irq, void *data)
{
	struct hisi_sfc_v3xx_host *host = data;
	struct completion *done;
	u32 reg;

	reg = readl(host->regbase + HISI_SFC_V3XX_INT_STAT);
	if (!reg)
		return IRQ_NONE;

	hisi_sfc_v3xx_disable_int(host);

	done = READ_ONCE(host->completion);
	if (done)
		complete(done);

	return IRQ_HANDLED;
}

static int hisi_sfc_v3xx_buswidth_override_bits;

/*
 * ACPI FW does not allow us to currently set the device buswidth, so quirk it
 * depending on the board.
 */
static int __init hisi_sfc_v3xx_dmi_quirk(const struct dmi_system_id *d)
{
	hisi_sfc_v3xx_buswidth_override_bits = SPI_RX_QUAD | SPI_TX_QUAD;

	return 0;
}

static const struct dmi_system_id hisi_sfc_v3xx_dmi_quirk_table[]  = {
	{
	.callback = hisi_sfc_v3xx_dmi_quirk,
	.matches = {
		DMI_MATCH(DMI_SYS_VENDOR, "Huawei"),
		DMI_MATCH(DMI_PRODUCT_NAME, "D06"),
	},
	},
	{
	.callback = hisi_sfc_v3xx_dmi_quirk,
	.matches = {
		DMI_MATCH(DMI_SYS_VENDOR, "Huawei"),
		DMI_MATCH(DMI_PRODUCT_NAME, "TaiShan 2280 V2"),
	},
	},
	{
	.callback = hisi_sfc_v3xx_dmi_quirk,
	.matches = {
		DMI_MATCH(DMI_SYS_VENDOR, "Huawei"),
		DMI_MATCH(DMI_PRODUCT_NAME, "TaiShan 200 (Model 2280)"),
	},
	},
	{}
};

/*
 * HISFC350 of the camera and NVR SoCs. The node's own "hisilicon,<soc>-sfc"
 * compatible is only documentation, everything else comes from the hardware
 * and the standard properties (see the header comment).
 */
static const struct hisi_sfc_v3xx_data hisfc350_data = {
	.num_cs = 2,
	.hisfc350 = true,
};

/*
 * The earlier Hi3520D V100 and V200 device trees (a node that lists only the
 * SoC compatible: no fallback, no window, no clock, no interrupt): command
 * mode only, no HISFC350 rules and one chip select unless the node says
 * otherwise, all unchanged from before this driver learned the HISFC350.
 */
static const struct hisi_sfc_v3xx_data hisi_sfc_v3xx_legacy_data = {
	.num_cs = 1,
};

static int hisi_sfc_v3xx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_sfc_v3xx_host *host;
	struct spi_controller *ctlr;
	struct reset_control *rstc;
	struct resource *res;
	struct clk *clk;
	u32 version, glb_config, num_cs;
	int ret;

	ctlr = devm_spi_alloc_host(&pdev->dev, sizeof(*host));
	if (!ctlr)
		return -ENOMEM;

	/*
	 * The spi core counts the address phase of a 1-2-2 or 1-4-4 read as a
	 * transmit capability. The HISFC350 does those for reads only,
	 * hisi_sfc_v3xx_supports_op() keeps its writes on one line.
	 */
	ctlr->mode_bits = SPI_RX_DUAL | SPI_RX_QUAD | SPI_TX_DUAL | SPI_TX_QUAD;

	ctlr->buswidth_override_bits = hisi_sfc_v3xx_buswidth_override_bits;

	host = spi_controller_get_devdata(ctlr);
	host->dev = dev;
	host->data = device_get_match_data(dev);
	/*
	 * The first compatible of the node that is in the table wins, so the
	 * SoC compatible of a node with the fallback would select the legacy
	 * data. A node that lists the fallback is a HISFC350.
	 */
	if (host->data == &hisi_sfc_v3xx_legacy_data &&
	    device_is_compatible(dev, "hisilicon,hisfc350"))
		host->data = &hisfc350_data;
	if (!host->data)
		return dev_err_probe(dev, -ENODEV, "no match data, bound by name?\n");

	host->int_mask = host->data->hisfc350 ? HISI_SFC_V3XX_INT_MASK_HISFC350 :
						 HISI_SFC_V3XX_INT_MASK_ALL;

	num_cs = host->data->num_cs;
	device_property_read_u32(dev, "num-cs", &num_cs);
	if (host->data->hisfc350 && (num_cs < 1 || num_cs > 2))
		return dev_err_probe(dev, -EINVAL, "unsupported num-cs %u\n", num_cs);

	platform_set_drvdata(pdev, host);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "regs");
	if (!res)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->regbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(host->regbase))
		return PTR_ERR(host->regbase);

	/* Only the HISFC350 has the bus window and the bus DMA */
	res = NULL;
	if (host->data->hisfc350) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mem");
		if (!res)
			res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	}
	if (res) {
		if (!devm_request_mem_region(dev, res->start, resource_size(res),
					     dev_name(dev)))
			return dev_err_probe(dev, -EBUSY, "cannot claim the flash window\n");
		host->mem_phys = res->start;
		host->mem_size = resource_size(res);
	}

	clk = devm_clk_get_optional_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "cannot get clock\n");

	if (host->data->hisfc350 && !clk)
		return dev_err_probe(dev, -EINVAL, "missing \"clocks\" property of the HISFC350\n");

	/*
	 * The legacy nodes carry no clock: this driver neither enables the
	 * controller clock nor selects its source (the clock driver of the
	 * SoC may enable the gate for such a node, the source stays as the
	 * boot loader left it). Say so where the user will see it. The
	 * binding allows no clock on a node that lists only the SoC
	 * compatible, so the advice is to convert the node; the Hi3520D V200
	 * has no other description yet, so there is nothing to convert to.
	 */
	if (!clk && dev_of_node(dev)) {
		if (device_is_compatible(dev, "hisilicon,hi3520dv200-sfc"))
			dev_info(dev, "the binding of this SoC describes no clock: the driver does not control the controller clock, which stays as the boot loader left it\n");
		else
			dev_warn(dev, "no \"clocks\" property: the driver does not control the controller clock, convert the node to the hisilicon,hisfc350 form with the flash window, the clock and the reset\n");
	}

	/*
	 * Pulse the controller reset with the clock running, so that no
	 * register state of the boot loader or of an earlier driver is left.
	 * Nodes without a reset (the Hi3520D V200, ACPI) keep their state.
	 */
	if (dev_of_node(dev)) {
		rstc = devm_reset_control_get_optional_exclusive(dev, NULL);
		if (IS_ERR(rstc))
			return dev_err_probe(dev, PTR_ERR(rstc),
					     "cannot get reset\n");
		if (rstc) {
			ret = reset_control_assert(rstc);
			if (ret)
				return dev_err_probe(dev, ret, "cannot assert reset\n");
			usleep_range(10, 20);
			ret = reset_control_deassert(rstc);
			if (ret)
				return dev_err_probe(dev, ret, "cannot deassert reset\n");
			/* The reset is left deasserted on unbind: it may be the boot flash */
			usleep_range(10, 20);
		}
	}

	version = readl(host->regbase + HISI_SFC_V3XX_VERSION);
	dev_dbg(dev, "VERSION register 0x%x\n", version);
	if (!host->data->hisfc350 && (!version || version == 0xffffffff))
		dev_warn(dev, "VERSION register reads 0x%x: no controller, clock off or in reset, expect failing flash accesses\n",
			 version);
	if (host->data->hisfc350) {
		if (!version || version == 0xffffffff)
			return dev_err_probe(dev, -ENODEV,
					     "VERSION register reads 0x%x: no controller, clock off or in reset\n",
					     version);
		if (version != HISI_SFC_V3XX_VERSION_HISFC350)
			dev_warn(dev, "SFC version 0x%x, the HISFC350 is 0x%x: untested\n",
				 version, HISI_SFC_V3XX_VERSION_HISFC350);
	}

	host->irq = platform_get_irq_optional(pdev, 0);
	if (host->irq == -EPROBE_DEFER)
		return host->irq;
	/* The device tree parts fail on a broken interrupt, the servers poll as before */
	if (host->irq < 0 && host->irq != -ENXIO && host->data->hisfc350)
		return dev_err_probe(dev, host->irq, "cannot get the interrupt\n");

	hisi_sfc_v3xx_disable_int(host);
	/* Status a boot loader left pending must not be taken for ours */
	if (host->data->hisfc350)
		hisi_sfc_v3xx_clear_int(host);

	if (host->irq > 0) {
		ret = devm_request_irq(dev, host->irq, hisi_sfc_v3xx_isr, 0,
				       "hisi-sfc-v3xx", host);

		if (ret) {
			if (host->data->hisfc350 || ret == -EPROBE_DEFER)
				return dev_err_probe(dev, ret, "failed to request irq %d\n",
						     host->irq);
			dev_err(dev, "failed to request irq%d, ret = %d\n", host->irq, ret);
			host->irq = 0;
		}
	} else {
		host->irq = 0;
	}

	if (host->irq && host->mem_size) {
		/*
		 * The bus DMA engine is optional: without it the reads run
		 * through the window, or through the command interface.
		 */
		ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
		if (ret) {
			dev_warn(dev, "no 32 bit DMA mask (%d), the bus DMA engine is not used\n",
				 ret);
		} else {
			/* One chunk plus a guard byte, rounded up to whole pages */
			host->dma_buf = dma_alloc_coherent(dev,
							   2 * HISI_SFC_V3XX_DMA_CHUNK,
							   &host->dma_handle,
							   GFP_KERNEL);
			if (!host->dma_buf) {
				dev_warn(dev, "no memory for the bus DMA buffer, the engine is not used\n");
			} else {
				ret = devm_add_action_or_reset(dev,
							       hisi_sfc_v3xx_free_dma,
							       host);
				if (ret)
					return ret;
			}
		}
	}

	/*
	 * Report the SPI clock the controller can deliver. Informational: the
	 * driver never changes the rate, spi-nor only clamps the device's
	 * spi-max-frequency to this value. A clock of rate 0 (no provider)
	 * leaves max_speed_hz alone.
	 */
	if (host->data->hisfc350 && clk) {
		unsigned long rate = clk_get_rate(clk);

		if (rate) {
			ctlr->max_speed_hz = rate / 2;
			host->spi_clk_hz = rate / 2;
		}
	}

	ctlr->bus_num = -1;
	ctlr->num_chipselect = num_cs;
	ctlr->mem_ops = &hisi_sfc_v3xx_mem_ops;

	/*
	 * The address mode of the controller is either 3 or 4, which is
	 * indicated by the address mode bit in the global config register. The
	 * register is read only for the OS driver on the server parts, which
	 * stay in that mode. On the HISFC350 the bit resets to the level of the
	 * SFC_ADDR_MODE pin and is writable: the driver sets it for every op
	 * with an address, so what the boot loader or the pin selected does not
	 * matter, and this is only the starting value.
	 */
	if (host->data->hisfc350) {
		u32 reg;

		/*
		 * A command or a bus DMA transfer a boot loader started must be
		 * over before the registers change
		 */
		if (readl_poll_timeout(host->regbase + HISI_SFC_V3XX_CMD_CFG, reg,
				       !(reg & HISI_SFC_V3XX_CMD_CFG_START_MSK),
				       10, 1000) ||
		    readl_poll_timeout(host->regbase + HISI_SFC_V3XX_BUS_DMA_CTRL, reg,
				       !(reg & HISI_SFC_V3XX_BUS_DMA_CTRL_START),
				       10, 1000)) {
			host->stuck = true;
			dev_err(dev, "a command or bus DMA transfer is still running at probe, controller disabled\n");
		} else {
			hisi_sfc_v3xx_init_hw(host);
		}
	}
	glb_config = readl(host->regbase + HISI_SFC_V3XX_GLB_CFG);
	if (glb_config & HISI_SFC_V3XX_GLB_CFG_CS0_ADDR_MODE)
		host->address_mode = 4;
	else
		host->address_mode = 3;

	if (version >= 0x351)
		host->max_cmd_dword = 64;
	else
		host->max_cmd_dword = 16;

	ret = devm_spi_register_controller(dev, ctlr);
	if (ret)
		return ret;

	dev_info(&pdev->dev, "hw version 0x%x, %s mode%s%s.\n", version,
		 host->irq ? "irq" : "polling",
		 host->mem_size ? ", AHB window" : "",
		 host->dma_buf ? ", bus DMA" : "");
	dev_dbg(dev, "window %pa+%pa\n", &host->mem_phys, &host->mem_size);

	return 0;
}

/* Kunpeng/Hi16xx server parts, probed through ACPI */
static const struct hisi_sfc_v3xx_data hisi_sfc_v3xx_acpi_data = {
	.num_cs = 1,
};

static const struct acpi_device_id hisi_sfc_v3xx_acpi_ids[] = {
	{"HISI0341", (kernel_ulong_t)&hisi_sfc_v3xx_acpi_data},
	{}
};
MODULE_DEVICE_TABLE(acpi, hisi_sfc_v3xx_acpi_ids);

static const struct of_device_id hisi_sfc_v3xx_of_match[] = {
	{ .compatible = "hisilicon,hisfc350", .data = &hisfc350_data },
	/* legacy: the earlier device trees list no fallback and no window */
	{ .compatible = "hisilicon,hi3520dv100-sfc", .data = &hisi_sfc_v3xx_legacy_data },
	{ .compatible = "hisilicon,hi3520dv200-sfc", .data = &hisi_sfc_v3xx_legacy_data },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_sfc_v3xx_of_match);

static struct platform_driver hisi_sfc_v3xx_spi_driver = {
	.driver = {
		.name	= "hisi-sfc-v3xx",
		.acpi_match_table = hisi_sfc_v3xx_acpi_ids,
		.of_match_table = hisi_sfc_v3xx_of_match,
	},
	.probe	= hisi_sfc_v3xx_probe,
};

static int __init hisi_sfc_v3xx_spi_init(void)
{
	dmi_check_system(hisi_sfc_v3xx_dmi_quirk_table);

	return platform_driver_register(&hisi_sfc_v3xx_spi_driver);
}

static void __exit hisi_sfc_v3xx_spi_exit(void)
{
	platform_driver_unregister(&hisi_sfc_v3xx_spi_driver);
}

module_init(hisi_sfc_v3xx_spi_init);
module_exit(hisi_sfc_v3xx_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Garry <john.garry@huawei.com>");
MODULE_AUTHOR("Bjorn Stout <bjorn.stout@hotmail.com>");
MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_DESCRIPTION("HiSilicon SPI NOR V3XX / HISFC350 Flash Controller Driver");
