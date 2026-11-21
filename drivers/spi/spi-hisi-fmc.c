// SPDX-License-Identifier: GPL-2.0-only
//
// HiSilicon FMC (flash memory controller, "hifmc100") SPI NOR driver
//
// Copyright (c) 2026 Dylan Corrales <deathcamel58@gmail.com>
//
// The FMC is the flash controller of the HiSilicon IP camera and NVR SoCs
// that came after the HISFC350 (Hi3518E V200, Hi3516C V200/V300, Hi3516E
// V100/V200/V300, Hi3516A/D V200/V300, Hi3519, Hi3536 family, ...): one block
// for SPI NOR and SPI NAND on two chip selects. Every data sheet calls it
// "FMC V100" and its version register reads 0x100. This driver handles the
// SPI NOR half through spi-mem, so the spi-nor core does the flash
// identification (SFDP, JEDEC ID), the opcode and address mode choices and the
// MTD side.
//
// The controller offers two ways to run a flash command:
//
//  - register mode: opcode, address, dummy bytes and up to 4 KiB of data
//    (kept in an on-chip buffer that appears in the second memory resource)
//    are sent as one operation. Identification, status, configuration, write
//    enable, erase and the address mode switch use it.
//  - DMA mode: the controller moves up to 256 MiB between the flash and DDR
//    for a read or page program opcode it is given. Every operation that
//    carries an address and data (flash reads, SFDP, page program) uses it,
//    through a coherent bounce buffer.
//
// The memory resource is only the register-mode buffer: there is no register
// that maps the flash into an AHB window (unlike the HISFC350), so the driver
// has no dirmap, the reads are DMA operations of up to HISI_FMC_DMA_CHUNK
// bytes.
//
// The driver learns what differs between SoCs from the hardware and from the
// standard device tree properties, not from a table:
//
//  - the version register is checked (0x100 on every SoC with this IP) and a
//    controller with another version is used with a warning;
//  - FMC_CFG (SPI NOR, no ECC, normal mode), the write protect pin control,
//    the SDR read delay and DTR sample point (both 0), the timing and the DMA
//    bus configuration are programmed, not assumed from what a boot loader
//    left. The address mode bit of FMC_CFG is set for every operation from
//    its address size, and a controller on which the bit does not stick
//    refuses 4-byte addresses;
//    OP_CFG.multi_cycle_en is set for every operation when the SPI clock is
//    above 50 MHz and the bit sticks (the field is reserved on the first
//    edition of the block);
//  - clocks, resets: the SPI clock is half of the controller clock (there is
//    no other divider), the clock rate comes from the device tree
//    (assigned-clock-rates/-parents) and the driver reports it;
//  - chip select: only CS0 is used. The data sheets disagree on whether the
//    chip select field of OP_CFG applies to DMA transfers (the register
//    description of some of them calls it invalid for DMA, their DMA
//    procedures tell to set it) and nothing was verified on a flash on CS1,
//    so a flash on another chip select is refused at probe, and so is a
//    cs-gpios property (the spi core would ignore it: the driver does not
//    ask for GPIO descriptors);
//  - spi-rx-bus-width of the flash node: dual or quad reads;
//  - the MTD name: the "label" of the flash node, as the HISFC350 driver does
//    (see hisi_fmc_get_name()).
//
// The controller has no register that aborts an operation. A register
// operation or a DMA transfer that timed out is stopped by pulsing the
// controller reset when the node has one (and the registers are programmed
// again), and the driver checks that the start bit is clear afterwards;
// without a reset line the next operation waits for the controller instead. A controller
// that is still busy when the driver is unbound keeps its DMA buffer: the
// buffer is not freed under a running transfer.
//
// Not implemented: SPI NAND, interrupts (operations are polled), double
// transfer rate, quad page program, the parallel NAND and extended operation
// registers of the later editions of the block. The driver never touches the
// block protection of a flash: it only reports the status register of every
// flash node at probe. The pad mux and pad configuration of the flash pins
// are not programmed (a SoC specific, board specific setting, see the
// binding): they must be set up before the driver probes.

#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/reset.h>
#include <linux/sizes.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>

#define HISI_FMC_CFG			0x00
#define HISI_FMC_CFG_OP_MODE_NORMAL	BIT(0)	/* clear: boot mode */
#define HISI_FMC_CFG_FLASH_SEL		GENMASK(2, 1)	/* 0: SPI NOR */
#define HISI_FMC_CFG_ECC_TYPE		GENMASK(7, 5)	/* 0: no ECC */
#define HISI_FMC_CFG_NOR_ADDR4		BIT(10)	/* 4-byte NOR addresses */
#define HISI_FMC_GLOBAL_CFG		0x04
#define HISI_FMC_GLOBAL_CFG_RD_DELAY	GENMASK(5, 3)	/* SDR read delay, newer editions */
#define HISI_FMC_GLOBAL_CFG_WP_EN	BIT(6)	/* WP pin driven low */
#define HISI_FMC_GLOBAL_CFG_DDR_MODE	BIT(11)	/* DTR, newer editions */
#define HISI_FMC_GLOBAL_CFG_SAMPLE	GENMASK(15, 12)	/* DTR sample point, newer editions */
#define HISI_FMC_TIMING			0x08
#define HISI_FMC_TIMING_TSHSL		GENMASK(3, 0)
#define HISI_FMC_TIMING_TCSS		GENMASK(7, 4)
#define HISI_FMC_TIMING_TCSH		GENMASK(11, 8)
#define HISI_FMC_INT			0x18
#define HISI_FMC_INT_OP_DONE		BIT(0)
#define HISI_FMC_INT_OP_FAIL		BIT(1)
#define HISI_FMC_INT_DMA_ERR		BIT(5)
#define HISI_FMC_INT_WR_LOCK		BIT(6)	/* Hi3516A V200, Hi3519 only */
#define HISI_FMC_INT_AHB_OP		BIT(7)
#define HISI_FMC_INT_EN			0x1c
#define HISI_FMC_INT_CLR		0x20
#define HISI_FMC_INT_ALL		GENMASK(7, 0)
#define HISI_FMC_CMD			0x24
#define HISI_FMC_ADDRL			0x2c
#define HISI_FMC_OP_CFG			0x30
#define HISI_FMC_OP_CFG_MULTI_CYCLE	BIT(13)	/* required above 50 MHz, newer editions */
#define HISI_FMC_OP_CFG_CS		BIT(11)	/* not used: see the header */
#define HISI_FMC_OP_CFG_IF_TYPE		GENMASK(9, 7)
#define HISI_FMC_OP_CFG_ADDR_NUM	GENMASK(6, 4)
#define HISI_FMC_OP_CFG_DUMMY_NUM	GENMASK(3, 0)
#define HISI_FMC_DATA_NUM		0x38
#define HISI_FMC_OP			0x3c
#define HISI_FMC_OP_DUMMY_EN		BIT(8)
#define HISI_FMC_OP_CMD1_EN		BIT(7)
#define HISI_FMC_OP_ADDR_EN		BIT(6)
#define HISI_FMC_OP_WRITE_DATA_EN	BIT(5)
#define HISI_FMC_OP_READ_DATA_EN	BIT(2)
#define HISI_FMC_OP_START		BIT(0)	/* cleared when done */
#define HISI_FMC_DMA_LEN		0x40
#define HISI_FMC_DMA_AHB_CTRL		0x48
#define HISI_FMC_DMA_AHB_CTRL_BURSTS	GENMASK(2, 0)
#define HISI_FMC_DMA_SADDR_D0		0x4c
#define HISI_FMC_OP_CTRL		0x68
#define HISI_FMC_OP_CTRL_RD_OPCODE	GENMASK(23, 16)
#define HISI_FMC_OP_CTRL_WR_OPCODE	GENMASK(15, 8)
#define HISI_FMC_OP_CTRL_WRITE		BIT(1)
#define HISI_FMC_OP_CTRL_START		BIT(0)	/* cleared when done */
#define HISI_FMC_TIMEOUT_WR		0x6c
#define HISI_FMC_LP_CTRL		0x78
#define HISI_FMC_LOCK			0x7c	/* Hi3516A V200, Hi3519 only */
#define HISI_FMC_LOCK_EN		GENMASK(3, 1)
#define HISI_FMC_VERSION		0xbc
#define HISI_FMC_VERSION_V100		0x100

/* HISI_FMC_OP_CFG_IF_TYPE */
#define HISI_FMC_IF_STD			0
#define HISI_FMC_IF_DUAL_OUT		1
#define HISI_FMC_IF_DUAL_IO		2
#define HISI_FMC_IF_QUAD_OUT		3
#define HISI_FMC_IF_QUAD_IO		4

#define HISI_FMC_NUM_CS			1
#define HISI_FMC_BUF_SIZE		SZ_4K	/* register-mode buffer, 4 KiB + 256 B */
#define HISI_FMC_BUF_MAP		SZ_8K
#define HISI_FMC_DMA_CHUNK		SZ_64K
#define HISI_FMC_DMA_MAX_DUMMY		15
/*
 * The vendor boot loader waits 4 s for a register operation: an erase may
 * keep the controller busy until the flash is done with it.
 */
#define HISI_FMC_OP_TIMEOUT_US		4000000
#define HISI_FMC_MULTI_CYCLE_HZ		50000000
#define HISI_FMC_DMA_TIMEOUT_US		2000000
#define HISI_FMC_RESET_WAIT_US		1000

#define SPINOR_OP_RDSR			0x05
/*
 * BP0-BP2 and bit 5 (BP3 or TB, depending on the vendor). Bit 6 is BP4 on
 * some flashes and the quad enable bit on others: not a protection report.
 */
#define SPINOR_SR_BP_MASK		GENMASK(5, 2)

struct hisi_fmc_host {
	struct device *dev;
	void __iomem *regs;
	void __iomem *buf;
	struct reset_control *rstc;	/* NULL: no reset line in the node */
	void *dma_buf;
	dma_addr_t dma_handle;
	bool addr4;		/* FMC_CFG can switch to 4-byte addresses */
	bool multi_cycle;	/* OP_CFG redundancy period bit, set above 50 MHz */
	unsigned long spi_hz;	/* SPI clock: half of the controller clock, 0: unknown */
};

static u32 hisi_fmc_read(struct hisi_fmc_host *host, unsigned int reg)
{
	return readl(host->regs + reg);
}

static void hisi_fmc_write(struct hisi_fmc_host *host, u32 val, unsigned int reg)
{
	writel(val, host->regs + reg);
}

/*
 * A register operation or a DMA transfer started earlier must be over. After
 * an operation that timed out the controller may still be working on it:
 * wait for as long as an operation may take, so that the controller is usable
 * again as soon as the operation ends (no register aborts it, see
 * hisi_fmc_quiesce()).
 */
static int hisi_fmc_wait_idle(struct hisi_fmc_host *host)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(host->regs + HISI_FMC_OP, val,
				 !(val & HISI_FMC_OP_START), 10,
				 HISI_FMC_OP_TIMEOUT_US);
	if (ret)
		return ret;

	return readl_poll_timeout(host->regs + HISI_FMC_OP_CTRL, val,
				  !(val & HISI_FMC_OP_CTRL_START), 10,
				  HISI_FMC_OP_TIMEOUT_US);
}

/*
 * Put the registers the driver relies on into a known state instead of
 * trusting what a boot loader left: SPI NOR in normal mode without ECC, the
 * WP pin not forced low (the vendor drivers clear it before every write
 * enable, a status register write fails with a protected flash while it is
 * low), no DTR and no sample point, no delay of the data read from the flash
 * in SDR mode (the reset state of the data sheets; these fields are reserved
 * on the first edition of the block and read back as 0), the timing of the
 * data sheet reset state, all DMA bursts, the write timeout of the reset
 * state, no clock gating logic, no interrupts.
 * The address mode bit of FMC_CFG is set for every operation.
 * The state found is reported when @report is set: after a reset by the
 * driver it is the reset state of the block, not what a boot loader left.
 */
static void hisi_fmc_init_hw(struct hisi_fmc_host *host, bool report)
{
	const u32 cfg_mask = HISI_FMC_CFG_OP_MODE_NORMAL | HISI_FMC_CFG_FLASH_SEL |
			     HISI_FMC_CFG_ECC_TYPE;
	const u32 glb_mask = HISI_FMC_GLOBAL_CFG_WP_EN | HISI_FMC_GLOBAL_CFG_DDR_MODE |
			     HISI_FMC_GLOBAL_CFG_RD_DELAY | HISI_FMC_GLOBAL_CFG_SAMPLE;
	u32 cfg_found = hisi_fmc_read(host, HISI_FMC_CFG);
	u32 glb_found = hisi_fmc_read(host, HISI_FMC_GLOBAL_CFG);
	u32 timing_found = hisi_fmc_read(host, HISI_FMC_TIMING);
	u32 cfg = (cfg_found & ~(HISI_FMC_CFG_FLASH_SEL | HISI_FMC_CFG_ECC_TYPE)) |
		  HISI_FMC_CFG_OP_MODE_NORMAL;
	u32 glb = glb_found & ~glb_mask;
	u32 timing = FIELD_PREP(HISI_FMC_TIMING_TSHSL, 0xf) |
		     FIELD_PREP(HISI_FMC_TIMING_TCSS, 6) |
		     FIELD_PREP(HISI_FMC_TIMING_TCSH, 6);
	u32 cfg_now, glb_now, timing_now;

	dev_dbg(host->dev, "found CFG 0x%x GLOBAL_CFG 0x%x TIMING 0x%x\n",
		cfg_found, glb_found, timing_found);

	hisi_fmc_write(host, cfg, HISI_FMC_CFG);
	hisi_fmc_write(host, glb, HISI_FMC_GLOBAL_CFG);
	hisi_fmc_write(host, timing, HISI_FMC_TIMING);
	hisi_fmc_write(host, HISI_FMC_DMA_AHB_CTRL_BURSTS, HISI_FMC_DMA_AHB_CTRL);
	hisi_fmc_write(host, GENMASK(23, 0), HISI_FMC_TIMEOUT_WR);
	hisi_fmc_write(host, 0, HISI_FMC_LP_CTRL);
	hisi_fmc_write(host, 0, HISI_FMC_INT_EN);
	hisi_fmc_write(host, HISI_FMC_INT_ALL, HISI_FMC_INT_CLR);

	/*
	 * Read back what the controller took: TIMING_CSH is a reserved field on
	 * some editions (it reads 0 after the write), so the state a boot loader
	 * left is compared with the read-back value, not with the one written.
	 */
	cfg_now = hisi_fmc_read(host, HISI_FMC_CFG);
	glb_now = hisi_fmc_read(host, HISI_FMC_GLOBAL_CFG);
	timing_now = hisi_fmc_read(host, HISI_FMC_TIMING);

	if (((cfg_now ^ cfg) & cfg_mask) || ((glb_now ^ glb) & glb_mask))
		dev_warn(host->dev,
			 "the controller did not take its configuration (CFG 0x%x for 0x%x, GLOBAL_CFG 0x%x for 0x%x)\n",
			 cfg_now, cfg, glb_now, glb);

	if (report && (((cfg_found ^ cfg_now) & cfg_mask) ||
		       ((glb_found ^ glb_now) & glb_mask) ||
		       timing_found != timing_now))
		dev_info(host->dev,
			 "controller was not in the state of a SPI NOR flash driver (CFG 0x%x, GLOBAL_CFG 0x%x, TIMING 0x%x), reprogrammed\n",
			 cfg_found, glb_found, timing_found);

	/*
	 * The later editions of the block can refuse writes to address ranges
	 * (FMC_LOCK, locked down until a hardware reset). The driver does not
	 * change it, it says so: a lock a boot loader left explains refused
	 * writes. The register is reserved on the other editions.
	 */
	if (hisi_fmc_read(host, HISI_FMC_LOCK) & HISI_FMC_LOCK_EN)
		dev_warn(host->dev,
			 "the address lock of the controller is enabled (FMC_LOCK 0x%x), writes to locked ranges fail\n",
			 hisi_fmc_read(host, HISI_FMC_LOCK));
}

static bool hisi_fmc_busy(struct hisi_fmc_host *host)
{
	return (hisi_fmc_read(host, HISI_FMC_OP) & HISI_FMC_OP_START) ||
	       (hisi_fmc_read(host, HISI_FMC_OP_CTRL) & HISI_FMC_OP_CTRL_START);
}

/*
 * Make sure the controller is not working on an operation, and in particular
 * not moving data to or from the DMA buffer. There is no register that aborts
 * an operation. With a reset line the block is reset (its state machine and
 * DMA master stop, every register is back to its reset state, so it is
 * programmed again when @reprogram is set) and the start bits are checked
 * afterwards; without one, or when the start bit stays set, the operation is
 * waited for as long as one may take. Returns 0 when the controller is idle.
 */
static int hisi_fmc_quiesce(struct hisi_fmc_host *host, bool reprogram)
{
	bool busy;
	int ret;

	if (!hisi_fmc_busy(host))
		return 0;

	if (host->rstc) {
		ret = reset_control_assert(host->rstc);
		if (!ret) {
			usleep_range(10, 20);
			ret = reset_control_deassert(host->rstc);
		}
		if (ret)
			dev_err(host->dev, "cannot reset the controller: %d\n", ret);
		usleep_range(10, 20);

		read_poll_timeout(hisi_fmc_busy, busy, !busy, 20,
				  HISI_FMC_RESET_WAIT_US, false, host);
		if (!busy) {
			dev_warn(host->dev, "stopped a running operation by a controller reset\n");
			if (reprogram)
				hisi_fmc_init_hw(host, false);
			return 0;
		}

		dev_err(host->dev, "the controller is still busy after a reset\n");
	}

	return hisi_fmc_wait_idle(host);
}

/* The status bits of FMC_INT of an operation that ended: 0 if it went through */
static int hisi_fmc_op_status(struct hisi_fmc_host *host, u32 val)
{
	if (val & HISI_FMC_INT_WR_LOCK) {
		dev_err(host->dev, "write refused by the address lock of the controller (FMC_LOCK 0x%x)\n",
			hisi_fmc_read(host, HISI_FMC_LOCK));
		return -EROFS;
	}

	if (val & (HISI_FMC_INT_OP_FAIL | HISI_FMC_INT_DMA_ERR)) {
		dev_err(host->dev, "operation failed, FMC_INT 0x%x\n", val);
		return -EIO;
	}

	return 0;
}

/*
 * A register operation or a DMA transfer timed out: the controller may still
 * work on it, so stop it now (reset line) or at least leave it idle, instead
 * of letting every later operation wait for the full idle timeout. @what is
 * for the message only.
 */
static void hisi_fmc_recover(struct hisi_fmc_host *host, const char *what)
{
	if (hisi_fmc_quiesce(host, true))
		dev_err(host->dev, "the %s could not be stopped\n", what);
}

static int hisi_fmc_wait_reg_op(struct hisi_fmc_host *host)
{
	u32 val;
	int ret;

	/* Most register operations are over within a few microseconds */
	ret = readl_poll_timeout_atomic(host->regs + HISI_FMC_OP, val,
					!(val & HISI_FMC_OP_START), 1, 100);
	if (!ret)
		return 0;

	return readl_poll_timeout(host->regs + HISI_FMC_OP, val,
				  !(val & HISI_FMC_OP_START), 20,
				  HISI_FMC_OP_TIMEOUT_US);
}

static int hisi_fmc_wait_dma(struct hisi_fmc_host *host)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(host->regs + HISI_FMC_INT, val,
				 val & (HISI_FMC_INT_OP_DONE |
					HISI_FMC_INT_OP_FAIL |
					HISI_FMC_INT_DMA_ERR |
					HISI_FMC_INT_WR_LOCK),
				 20, HISI_FMC_DMA_TIMEOUT_US);
	if (ret) {
		dev_err(host->dev, "DMA transfer timed out\n");
		return ret;
	}

	return hisi_fmc_op_status(host, val);
}

/*
 * The address mode of the controller is a bit of FMC_CFG that resets to a
 * pin level and that a boot loader may have switched: set it from the
 * address size of every operation that has an address.
 */
static void hisi_fmc_set_addr_mode(struct hisi_fmc_host *host, unsigned int nbytes)
{
	u32 cfg = hisi_fmc_read(host, HISI_FMC_CFG);
	u32 want = (cfg & ~HISI_FMC_CFG_NOR_ADDR4) |
		   (nbytes == 4 ? HISI_FMC_CFG_NOR_ADDR4 : 0);

	if (want != cfg)
		hisi_fmc_write(host, want, HISI_FMC_CFG);
}

/* The bits of OP_CFG that do not depend on the operation (CS0 is bit value 0) */
static u32 hisi_fmc_op_cfg(struct hisi_fmc_host *host)
{
	u32 cfg = 0;

	if (host->multi_cycle)
		cfg |= HISI_FMC_OP_CFG_MULTI_CYCLE;

	return cfg;
}

static int hisi_fmc_exec_reg(struct hisi_fmc_host *host,
			     const struct spi_mem_op *op)
{
	u32 ctrl = HISI_FMC_OP_CMD1_EN | HISI_FMC_OP_START;
	u32 cfg = FIELD_PREP(HISI_FMC_OP_CFG_IF_TYPE, HISI_FMC_IF_STD) |
		  hisi_fmc_op_cfg(host);
	u32 val;
	int ret;

	hisi_fmc_write(host, op->cmd.opcode, HISI_FMC_CMD);

	if (op->addr.nbytes) {
		hisi_fmc_set_addr_mode(host, op->addr.nbytes);
		hisi_fmc_write(host, op->addr.val, HISI_FMC_ADDRL);
		cfg |= FIELD_PREP(HISI_FMC_OP_CFG_ADDR_NUM, op->addr.nbytes);
		ctrl |= HISI_FMC_OP_ADDR_EN;
	}

	if (op->dummy.nbytes) {
		cfg |= FIELD_PREP(HISI_FMC_OP_CFG_DUMMY_NUM, op->dummy.nbytes);
		ctrl |= HISI_FMC_OP_DUMMY_EN;
	}

	hisi_fmc_write(host, cfg, HISI_FMC_OP_CFG);

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		memcpy_toio(host->buf, op->data.buf.out, op->data.nbytes);
		ctrl |= HISI_FMC_OP_WRITE_DATA_EN;
	} else if (op->data.dir == SPI_MEM_DATA_IN) {
		ctrl |= HISI_FMC_OP_READ_DATA_EN;
	}

	if (op->data.dir != SPI_MEM_NO_DATA)
		hisi_fmc_write(host, op->data.nbytes, HISI_FMC_DATA_NUM);

	hisi_fmc_write(host, HISI_FMC_INT_ALL, HISI_FMC_INT_CLR);
	hisi_fmc_write(host, ctrl, HISI_FMC_OP);

	ret = hisi_fmc_wait_reg_op(host);
	if (ret) {
		dev_err(host->dev, "operation 0x%02x timed out\n", op->cmd.opcode);
		hisi_fmc_recover(host, "operation");
		hisi_fmc_write(host, HISI_FMC_INT_ALL, HISI_FMC_INT_CLR);
		return ret;
	}

	/*
	 * An operation the controller refused (the address lock of the later
	 * editions refuses an erase) or that failed ends like one that went
	 * through: look at the status, as the DMA path does.
	 */
	val = hisi_fmc_read(host, HISI_FMC_INT);
	hisi_fmc_write(host, HISI_FMC_INT_ALL, HISI_FMC_INT_CLR);
	ret = hisi_fmc_op_status(host, val);
	if (ret)
		return ret;

	if (op->data.dir == SPI_MEM_DATA_IN)
		memcpy_fromio(op->data.buf.in, host->buf, op->data.nbytes);

	return 0;
}

static int hisi_fmc_if_type(const struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_OUT)
		return HISI_FMC_IF_STD;

	switch (op->addr.buswidth << 4 | op->data.buswidth) {
	case 0x11:
		return HISI_FMC_IF_STD;
	case 0x12:
		return HISI_FMC_IF_DUAL_OUT;
	case 0x22:
		return HISI_FMC_IF_DUAL_IO;
	case 0x14:
		return HISI_FMC_IF_QUAD_OUT;
	case 0x44:
		return HISI_FMC_IF_QUAD_IO;
	default:
		return -EOPNOTSUPP;
	}
}

static int hisi_fmc_exec_dma(struct hisi_fmc_host *host,
			     const struct spi_mem_op *op)
{
	bool write = op->data.dir == SPI_MEM_DATA_OUT;
	u32 cfg, ctrl;
	int ret, if_type;

	if_type = hisi_fmc_if_type(op);
	if (if_type < 0)
		return if_type;

	if (write)
		memcpy(host->dma_buf, op->data.buf.out, op->data.nbytes);

	hisi_fmc_set_addr_mode(host, op->addr.nbytes);

	cfg = FIELD_PREP(HISI_FMC_OP_CFG_IF_TYPE, if_type) |
	      FIELD_PREP(HISI_FMC_OP_CFG_ADDR_NUM, op->addr.nbytes) |
	      FIELD_PREP(HISI_FMC_OP_CFG_DUMMY_NUM, op->dummy.nbytes) |
	      hisi_fmc_op_cfg(host);

	if (write)
		ctrl = FIELD_PREP(HISI_FMC_OP_CTRL_WR_OPCODE, op->cmd.opcode) |
		       HISI_FMC_OP_CTRL_WRITE;
	else
		ctrl = FIELD_PREP(HISI_FMC_OP_CTRL_RD_OPCODE, op->cmd.opcode);

	hisi_fmc_write(host, HISI_FMC_INT_ALL, HISI_FMC_INT_CLR);
	hisi_fmc_write(host, op->addr.val, HISI_FMC_ADDRL);
	hisi_fmc_write(host, cfg, HISI_FMC_OP_CFG);
	hisi_fmc_write(host, op->data.nbytes, HISI_FMC_DMA_LEN);
	hisi_fmc_write(host, lower_32_bits(host->dma_handle), HISI_FMC_DMA_SADDR_D0);
	hisi_fmc_write(host, ctrl | HISI_FMC_OP_CTRL_START, HISI_FMC_OP_CTRL);

	ret = hisi_fmc_wait_dma(host);
	if (ret == -ETIMEDOUT) {
		/*
		 * The transfer may still run: stop it now, the controller must
		 * not keep moving data to or from the DMA buffer behind the
		 * driver's back.
		 */
		hisi_fmc_recover(host, "DMA transfer");
	}
	hisi_fmc_write(host, HISI_FMC_INT_ALL, HISI_FMC_INT_CLR);
	if (ret)
		return ret;

	if (!write)
		memcpy(op->data.buf.in, host->dma_buf, op->data.nbytes);

	return 0;
}

static bool hisi_fmc_op_is_dma(const struct spi_mem_op *op)
{
	return op->addr.nbytes && op->data.dir != SPI_MEM_NO_DATA;
}

static unsigned int hisi_fmc_max_data(const struct spi_mem_op *op)
{
	return hisi_fmc_op_is_dma(op) ? HISI_FMC_DMA_CHUNK : HISI_FMC_BUF_SIZE;
}

static int hisi_fmc_exec_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	struct hisi_fmc_host *host = spi_controller_get_devdata(mem->spi->controller);
	int ret;

	/* The bounce buffer and the controller buffer are of fixed size */
	if (op->data.dir != SPI_MEM_NO_DATA && op->data.nbytes > hisi_fmc_max_data(op))
		return -EINVAL;

	ret = hisi_fmc_wait_idle(host);
	if (ret) {
		dev_err(host->dev, "controller is busy\n");
		return ret;
	}

	if (hisi_fmc_op_is_dma(op))
		return hisi_fmc_exec_dma(host, op);

	return hisi_fmc_exec_reg(host, op);
}

static int hisi_fmc_adjust_op_size(struct spi_mem *mem, struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_NO_DATA)
		return 0;

	op->data.nbytes = min(op->data.nbytes, hisi_fmc_max_data(op));
	return 0;
}

static bool hisi_fmc_supports_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
	struct hisi_fmc_host *host = spi_controller_get_devdata(mem->spi->controller);

	/* Only CS0 is used, see the header */
	if (spi_get_chipselect(mem->spi, 0))
		return false;

	/*
	 * The widths the flash node allows (spi-rx-bus-width, spi-tx-bus-width)
	 * are in mem->spi->mode; the spi-nor core does not look at them and
	 * offers every read mode the flash has.
	 */
	if (!spi_mem_default_supports_op(mem, op))
		return false;

	if (op->cmd.nbytes != 1 || op->cmd.buswidth != 1 || op->cmd.dtr ||
	    op->addr.dtr || op->dummy.dtr || op->data.dtr)
		return false;

	if (op->addr.nbytes > 4 || (op->addr.nbytes == 4 && !host->addr4))
		return false;

	if (op->dummy.nbytes > HISI_FMC_DMA_MAX_DUMMY)
		return false;

	if (!hisi_fmc_op_is_dma(op)) {
		/* Register operations have a single line in every phase */
		return op->addr.buswidth <= 1 && op->dummy.buswidth <= 1 &&
		       op->data.buswidth <= 1 &&
		       !(op->dummy.nbytes && !op->addr.nbytes);
	}

	/* DMA reads: every width of the data sheet; page program: one line */
	if (op->data.dir == SPI_MEM_DATA_OUT)
		return op->addr.buswidth == 1 && op->data.buswidth == 1 &&
		       !op->dummy.nbytes;

	if (hisi_fmc_if_type(op) < 0)
		return false;

	/* The dummy bytes are counted in the width of the address phase */
	return op->dummy.nbytes == 0 || op->dummy.buswidth == op->addr.buswidth;
}

/*
 * The SPI clock is half of the controller clock and there is no divider: the
 * flash node cannot change it. Say so when the node asks for less than the
 * controller delivers (the clock rate is the business of assigned-clock-rates).
 */
static int hisi_fmc_setup(struct spi_device *spi)
{
	struct hisi_fmc_host *host = spi_controller_get_devdata(spi->controller);

	if (host->spi_hz && spi->max_speed_hz && spi->max_speed_hz < host->spi_hz)
		dev_warn(&spi->dev,
			 "the SPI clock of the controller (%lu Hz) is above spi-max-frequency of the flash (%u Hz): the controller has no divider, lower assigned-clock-rates in its node\n",
			 host->spi_hz, spi->max_speed_hz);

	return 0;
}

/*
 * Name the MTD device after the "label" of the flash node when it has one.
 * Boot loaders that describe the layout with "mtdparts=<name>:..." on the
 * kernel command line (the HiSilicon SDK U-Boot and OpenIPC's use "hi_sfc")
 * can only match an MTD device of that name. Without a label the SPI device
 * name is kept.
 */
static const char *hisi_fmc_get_name(struct spi_mem *mem)
{
	const char *name;

	if (!device_property_read_string(&mem->spi->dev, "label", &name))
		return name;

	return dev_name(&mem->spi->dev);
}

static const struct spi_controller_mem_ops hisi_fmc_mem_ops = {
	.get_name = hisi_fmc_get_name,
	.adjust_op_size = hisi_fmc_adjust_op_size,
	.supports_op = hisi_fmc_supports_op,
	.exec_op = hisi_fmc_exec_op,
};

/* Does the address mode bit stick? Without it a 4-byte operation cannot run */
static void hisi_fmc_check_addr_mode(struct hisi_fmc_host *host)
{
	u32 cfg = hisi_fmc_read(host, HISI_FMC_CFG);
	u32 set, clear;

	hisi_fmc_write(host, cfg | HISI_FMC_CFG_NOR_ADDR4, HISI_FMC_CFG);
	set = hisi_fmc_read(host, HISI_FMC_CFG);
	hisi_fmc_write(host, cfg & ~HISI_FMC_CFG_NOR_ADDR4, HISI_FMC_CFG);
	clear = hisi_fmc_read(host, HISI_FMC_CFG);

	host->addr4 = (set & HISI_FMC_CFG_NOR_ADDR4) &&
		      !(clear & HISI_FMC_CFG_NOR_ADDR4);
	if (!host->addr4)
		dev_warn(host->dev,
			 "the 4-byte address mode bit does not stick (CFG 0x%x after set, 0x%x after clear), 4-byte addresses are not used\n",
			 set, clear);
}

/*
 * The data sheets of the later editions ask for OP_CFG.multi_cycle_en (an
 * extra period between the chip select and the clock) when the SPI clock is
 * above 50 MHz; the field is reserved on the first edition. Use the bit when
 * the SPI clock is above that and the bit sticks, say so when it does not.
 */
static void hisi_fmc_check_multi_cycle(struct hisi_fmc_host *host)
{
	u32 val;

	if (host->spi_hz <= HISI_FMC_MULTI_CYCLE_HZ)
		return;

	hisi_fmc_write(host, HISI_FMC_OP_CFG_MULTI_CYCLE, HISI_FMC_OP_CFG);
	val = hisi_fmc_read(host, HISI_FMC_OP_CFG);
	hisi_fmc_write(host, 0, HISI_FMC_OP_CFG);

	host->multi_cycle = val & HISI_FMC_OP_CFG_MULTI_CYCLE;
	if (!host->multi_cycle)
		dev_info(host->dev,
			 "the SPI clock is above 50 MHz and OP_CFG has no multi-cycle bit (reads 0x%x after a write), not set\n",
			 val);
}

/*
 * Say what the protection of every flash is: the driver never changes it, and
 * a status register with block protection bits set (a vendor firmware may
 * lock the flash and unlock it again in its own userspace) explains failing
 * erase and program operations.
 */
static void hisi_fmc_report_flash(struct hisi_fmc_host *host)
{
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(SPINOR_OP_RDSR, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_IN(1, NULL, 1));
	u8 sr = 0xff;

	op.data.buf.in = &sr;
	if (hisi_fmc_wait_idle(host) || hisi_fmc_exec_reg(host, &op))
		return;

	/* 0x00 is the status of an idle flash without protection: only 0xff is a dead bus */
	if (sr == 0xff)
		dev_warn(host->dev,
			 "no flash answers the status register read (0xff): check the pad mux of the flash pins, the SPI clock and the flash supply, the driver sets none of them\n");
	else
		dev_info(host->dev, "flash status register 0x%02x%s\n", sr,
			 sr & SPINOR_SR_BP_MASK ?
			 ", protection bits (BP0-BP3/TB) set (not changed by this driver)" : "");
}

/*
 * Runs when the device goes away, after the controller was unregistered (no
 * operation is running any more) and before the clock is switched off. The
 * bounce buffer is freed only when the controller is idle: a transfer that
 * could not be stopped would write into freed memory, so the buffer stays
 * allocated (a leak of 64 KiB is the lesser evil).
 */
static void hisi_fmc_release_dma(void *data)
{
	struct hisi_fmc_host *host = data;

	if (hisi_fmc_quiesce(host, false)) {
		dev_err(host->dev, "the controller is still busy, the DMA buffer is not freed\n");
		return;
	}

	dma_free_coherent(host->dev, HISI_FMC_DMA_CHUNK, host->dma_buf, host->dma_handle);
}

static int hisi_fmc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_fmc_host *host;
	struct spi_controller *ctlr;
	struct resource *res, buf_res;
	struct clk *clk;
	u32 version, num_cs = HISI_FMC_NUM_CS;
	unsigned long rate;
	int ret;

	ctlr = devm_spi_alloc_host(dev, sizeof(*host));
	if (!ctlr)
		return -ENOMEM;

	/*
	 * The spi core counts the address phase of a 1-2-2 or 1-4-4 read as a
	 * transmit capability: the flash node needs spi-tx-bus-width next to
	 * spi-rx-bus-width for those reads. The controller never writes with
	 * more than one line, hisi_fmc_supports_op() keeps it that way.
	 */
	ctlr->mode_bits = SPI_RX_DUAL | SPI_RX_QUAD | SPI_TX_DUAL | SPI_TX_QUAD;

	host = spi_controller_get_devdata(ctlr);
	host->dev = dev;

	device_property_read_u32(dev, "num-cs", &num_cs);
	if (num_cs != HISI_FMC_NUM_CS)
		return dev_err_probe(dev, -EINVAL,
				     "unsupported num-cs %u: only chip select 0 is used\n", num_cs);

	/*
	 * The spi core ignores cs-gpios unless the driver asks for GPIO
	 * descriptors, and this controller only has its own chip select.
	 */
	if (device_property_present(dev, "cs-gpios"))
		return dev_err_probe(dev, -EINVAL, "cs-gpios is not supported\n");

	host->regs = devm_platform_ioremap_resource_byname(pdev, "control");
	if (IS_ERR(host->regs))
		return PTR_ERR(host->regs);

	/*
	 * The second resource is the memory space of the controller: only its
	 * first 4 KiB + 256 bytes are the register-mode buffer, the rest is
	 * not used (and may be 64 KiB or 16 MiB in the device tree). Map 8 KiB.
	 */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "memory");
	if (!res || resource_size(res) < HISI_FMC_BUF_MAP)
		return dev_err_probe(dev, -EINVAL, "missing or too small \"memory\" resource\n");
	buf_res = *res;
	buf_res.end = buf_res.start + HISI_FMC_BUF_MAP - 1;
	host->buf = devm_ioremap_resource(dev, &buf_res);
	if (IS_ERR(host->buf))
		return PTR_ERR(host->buf);

	clk = devm_clk_get(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "cannot get the clock\n");

	/* Everything that may defer the probe comes before the clock is enabled */
	host->rstc = devm_reset_control_get_optional_exclusive(dev, NULL);
	if (IS_ERR(host->rstc))
		return dev_err_probe(dev, PTR_ERR(host->rstc), "cannot get the reset\n");

	/*
	 * Enable the clock and keep it enabled for good: it is not released on
	 * unbind or on a failed probe (so no devm_clk_get_enabled()). The CRG
	 * driver does not switch the FMC gate off as unused either, because
	 * the boot ROM reads the flash through this controller and nothing
	 * says that a warm reset turns the gate back on: a reboot after the
	 * driver was unbound must still find the flash readable. The driver
	 * does not rely on the gate being on at entry, it turns it on.
	 */
	ret = clk_prepare_enable(clk);
	if (ret)
		return dev_err_probe(dev, ret, "cannot enable the clock\n");

	/*
	 * Pulse the controller reset with the clock running, so that no
	 * register state of the boot loader is left. A node without a reset
	 * keeps its state: the registers are programmed below either way.
	 */
	if (host->rstc) {
		ret = reset_control_assert(host->rstc);
		if (ret)
			return dev_err_probe(dev, ret, "cannot assert the reset\n");
		usleep_range(10, 20);
		ret = reset_control_deassert(host->rstc);
		if (ret)
			return dev_err_probe(dev, ret, "cannot deassert the reset\n");
		usleep_range(10, 20);
	}

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return dev_err_probe(dev, ret, "no 32 bit DMA mask\n");

	/*
	 * Not a managed allocation: it is freed by hisi_fmc_release_dma(), and
	 * only when the controller is not working on a transfer.
	 */
	host->dma_buf = dma_alloc_coherent(dev, HISI_FMC_DMA_CHUNK,
					   &host->dma_handle, GFP_KERNEL);
	if (!host->dma_buf)
		return -ENOMEM;

	ret = devm_add_action_or_reset(dev, hisi_fmc_release_dma, host);
	if (ret)
		return ret;

	/* A command or DMA transfer a boot loader started must be over */
	ret = hisi_fmc_wait_idle(host);
	if (ret)
		return dev_err_probe(dev, ret, "the controller is still busy at probe\n");

	version = hisi_fmc_read(host, HISI_FMC_VERSION);
	if (version != HISI_FMC_VERSION_V100)
		dev_warn(dev, "unknown FMC version 0x%x (0x%x expected), trying anyway\n",
			 version, HISI_FMC_VERSION_V100);

	hisi_fmc_init_hw(host, true);
	hisi_fmc_check_addr_mode(host);

	/*
	 * Report the SPI clock the controller delivers: half of the clock
	 * rate. The rate is the business of the device tree (assigned-clock-
	 * rates or -parents): the clock selects of the SoCs that have this
	 * controller have rates that only suit a double transfer rate, which
	 * only the SoC knows. A rate of 0 (no clock provider, or a clock
	 * that is off) leaves max_speed_hz alone and is reported loudly: the
	 * controller cannot run without a clock.
	 */
	rate = clk_get_rate(clk);
	if (rate) {
		host->spi_hz = rate / 2;
		ctlr->max_speed_hz = host->spi_hz;
	} else {
		dev_warn(dev, "the clock reports no rate, the SPI clock is unknown (set assigned-clock-rates)\n");
	}
	hisi_fmc_check_multi_cycle(host);

	ctlr->bus_num = -1;
	ctlr->num_chipselect = HISI_FMC_NUM_CS;
	ctlr->setup = hisi_fmc_setup;
	ctlr->mem_ops = &hisi_fmc_mem_ops;

	for_each_available_child_of_node_scoped(dev->of_node, child) {
		u32 cs;

		if (of_property_read_u32(child, "reg", &cs) || cs != 0)
			return dev_err_probe(dev, -EINVAL,
					     "%pOF: only a flash on chip select 0 is supported\n",
					     child);

		/* The status register command is a SPI NOR command */
		if (of_device_is_compatible(child, "jedec,spi-nor"))
			hisi_fmc_report_flash(host);
	}

	ret = devm_spi_register_controller(dev, ctlr);
	if (ret)
		return ret;

	dev_info(dev, "FMC version 0x%x, SPI clock %lu Hz%s, chip select 0, polling%s%s\n",
		 version, host->spi_hz, host->spi_hz ? "" : " (unknown: no clock rate)",
		 host->addr4 ? ", 4-byte addresses" : "",
		 host->multi_cycle ? ", multi-cycle period" : "");

	return 0;
}

static const struct of_device_id hisi_fmc_of_match[] = {
	{ .compatible = "hisilicon,hifmc100" },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_fmc_of_match);

static struct platform_driver hisi_fmc_driver = {
	.driver = {
		.name = "hisi-fmc",
		.of_match_table = hisi_fmc_of_match,
	},
	.probe = hisi_fmc_probe,
};
module_platform_driver(hisi_fmc_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_DESCRIPTION("HiSilicon FMC SPI NOR flash controller driver");
