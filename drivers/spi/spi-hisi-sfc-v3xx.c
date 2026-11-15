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
// HISFC350 (device tree) support.
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
//  - num-cs: the number of chip selects;
//  - clocks, resets: the controller clock (SPI clock = clock / 2) and soft
//    reset; interrupts: command completion;
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
#include <linux/dmi.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/reset.h>
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

struct hisi_sfc_v3xx_host {
	struct device *dev;
	void __iomem *regbase;
	const struct hisi_sfc_v3xx_data *data;
	int max_cmd_dword;
	bool stuck;			/* a command never finished: no access */
	bool irq_broken;		/* the interrupt did not arrive for a command: polling */
	bool speed_warned;
	unsigned long spi_clk_hz;	/* SPI clock the controller runs, 0 if unknown */
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
	.adjust_op_size = hisi_sfc_v3xx_adjust_op_size,
	.supports_op = hisi_sfc_v3xx_supports_op,
	.exec_op = hisi_sfc_v3xx_exec_op,
};

/*
 * Put the registers the driver relies on into a known state instead of
 * trusting what a boot loader or an earlier driver left: SPI mode 0, the WP
 * pin not forced low, no read sampling delay (the data sheet says the field
 * must be 0), BUS_CFG2 (reserved, must be 0) and the timing of the reset state
 * of the data sheet. The other registers are written in full for every
 * command. A controller that was not in that state is reported.
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

	clk = devm_clk_get_optional_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "cannot get clock\n");

	if (host->data->hisfc350 && !clk)
		return dev_err_probe(dev, -EINVAL, "missing \"clocks\" property of the HISFC350\n");

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
			/* The controller is left running on unbind: it may be the boot flash */
			usleep_range(10, 20);
		}
	}

	version = readl(host->regbase + HISI_SFC_V3XX_VERSION);
	dev_dbg(dev, "VERSION register 0x%x\n", version);
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

		/* A command a boot loader started must be over before the registers change */
		if (readl_poll_timeout(host->regbase + HISI_SFC_V3XX_CMD_CFG, reg,
				       !(reg & HISI_SFC_V3XX_CMD_CFG_START_MSK),
				       10, 1000)) {
			host->stuck = true;
			dev_err(dev, "a command is still running at probe, controller disabled\n");
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

	dev_info(&pdev->dev, "hw version 0x%x, %s mode.\n",
		 version, host->irq ? "irq" : "polling");

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
