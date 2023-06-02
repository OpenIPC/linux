#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/regulator/consumer.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/card.h>
#include <linux/slab.h>

#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/gpio.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <asm/dma.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <linux/uaccess.h>
#include <mach/hardware.h>
#include <linux/mmc/card.h>
#include <linux/clk.h>
#include "fhmci_reg.h"
#include <mach/fhmci.h>

#include <mach/pmu.h>


#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!(FALSE))
#endif

#define SD_POWER_ON   1
#define SD_POWER_OFF  0
#define DRIVER_NAME	"fh_mci"

static unsigned int retry_count = MAX_RETRY_COUNT;
static unsigned int request_timeout = FH_MCI_REQUEST_TIMEOUT;
int trace_level = FHMCI_TRACE_LEVEL;
struct mmc_host *mmc_sd1 = NULL;
struct mmc_host *mmc_sd0 = NULL;

#ifdef MODULE

MODULE_PARM_DESC(detect_timer, "card detect time (default:500ms))");

module_param(retry_count, uint, 0600);
MODULE_PARM_DESC(retry_count, "retry count times (default:100))");

module_param(request_timeout, uint, 0600);
MODULE_PARM_DESC(request_timeout, "Request timeout time (default:3s))");

module_param(trace_level, int, 0600);
MODULE_PARM_DESC(trace_level, "FHMCI_TRACE_LEVEL");

#endif

#include "fhmci_io.c"

/* reset MMC host controler */
static void fh_mci_sys_reset(struct fhmci_host *host)
{
	unsigned int reg_value;
	unsigned long flags;

	fhmci_trace(2, "reset");

	local_irq_save(flags);

	reg_value = fhmci_readl(host->base + MCI_BMOD);
	reg_value |= BMOD_SWR;
	fhmci_writel(reg_value, host->base + MCI_BMOD);
	udelay(50);

	reg_value = fhmci_readl(host->base + MCI_BMOD);
	reg_value |= BURST_INCR;
	fhmci_writel(reg_value, host->base + MCI_BMOD);

	reg_value = fhmci_readl(host->base + MCI_CTRL);
	reg_value |=  CTRL_RESET | FIFO_RESET | DMA_RESET;
	fhmci_writel(reg_value, host->base + MCI_CTRL);

	local_irq_restore(flags);
}

static void fh_mci_sys_undo_reset(struct fhmci_host *host)
{
	unsigned long flags;

	fhmci_trace(2, "undo reset");

	local_irq_save(flags);
	local_irq_restore(flags);
}

static void fh_mci_ctrl_power(struct fhmci_host *host, unsigned int flag)
{
	fhmci_trace(2, "begin");

}

/**********************************************
 *1: card off
 *0: card on
 ***********************************************/
static unsigned int fh_mci_sys_card_detect(struct fhmci_host *host)
{
	unsigned int card_status = readl(host->base + MCI_CDETECT);
	return card_status & FHMCI_CARD0;
}

/**********************************************
 *1: card readonly
 *0: card read/write
 ***********************************************/
static unsigned int fh_mci_ctrl_card_readonly(struct fhmci_host *host)
{
	unsigned int card_value = fhmci_readl(host->base + MCI_WRTPRT);
	return card_value & FHMCI_CARD0;
}

static int fh_mci_wait_cmd(struct fhmci_host *host)
{
	int wait_retry_count = 0;
	unsigned int reg_data = 0;
	unsigned long flags;


	fhmci_trace(2, "begin");
	fhmci_assert(host);

	while (1) {
		/*
		 * Check if CMD::start_cmd bit is clear.
		 * start_cmd = 0 means MMC Host controller has loaded registers
		 * and next command can be loaded in.
		 */
		reg_data = readl(host->base + MCI_CMD);
		if ((reg_data & START_CMD) == 0)
			return 0;

		/* Check if Raw_Intr_Status::HLE bit is set. */
		spin_lock_irqsave(&host->lock, flags);
		reg_data = readl(host->base + MCI_RINTSTS);
		if (reg_data & HLE_INT_STATUS) {
			reg_data &= (~SDIO_INT_STATUS);
			fhmci_writel(reg_data, host->base + MCI_RINTSTS);
			spin_unlock_irqrestore(&host->lock, flags);

			fhmci_trace(3, "Other CMD is running," \
					"please operate cmd again!");
			return 1;
		}

		spin_unlock_irqrestore(&host->lock, flags);
		udelay(100);

		/* Check if number of retries for this are over. */
		wait_retry_count++;
		if (wait_retry_count >= retry_count) {
			fhmci_trace(3, "send cmd is timeout!");
			return -1;
		}
	}
}

static void fh_mci_control_cclk(struct fhmci_host *host, unsigned int flag)
{
	unsigned int reg;
	union cmd_arg_s cmd_reg;

	fhmci_trace(2, "begin");
	fhmci_assert(host);

	reg = fhmci_readl(host->base + MCI_CLKENA);
	if (flag == ENABLE)
		reg |= CCLK_ENABLE;
	else
		reg &= 0xffff0000;
	fhmci_writel(reg, host->base + MCI_CLKENA);

	cmd_reg.cmd_arg = fhmci_readl(host->base + MCI_CMD);
	cmd_reg.bits.start_cmd = 1;
	cmd_reg.bits.update_clk_reg_only = 1;
	fhmci_writel(cmd_reg.cmd_arg, host->base + MCI_CMD);
	if (fh_mci_wait_cmd(host) != 0)
		fhmci_trace(3, "disable or enable clk is timeout!");
}

static void fh_mci_set_cclk(struct fhmci_host *host, unsigned int cclk)
{
	unsigned int reg_value;
	union cmd_arg_s clk_cmd;
	struct fh_mci_board *pdata;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(cclk);

	pdata = host->pdata;

	/*
	 * set card clk divider value,
	 * clk_divider = Fmmcclk/(Fmmc_cclk * 2)
	 */

	if (0 == host->id) {
		if (pdata->bus_hz <= cclk)
			reg_value = 0;
		else {
			reg_value = pdata->bus_hz / (cclk * 2);
			if (pdata->bus_hz % (cclk * 2))
				reg_value++;
		}
	} else if (1 == host->id) {
		if (pdata->bus_hz <= cclk)
			reg_value = 0;
		else {
			reg_value = pdata->bus_hz / (cclk * 2);
			if (pdata->bus_hz % (cclk * 2))
				reg_value++;
		}
	} else {
		fhmci_error("fhmci host id error!");
		return;
	}

	fhmci_writel(reg_value, host->base + MCI_CLKDIV);


	clk_cmd.cmd_arg = fhmci_readl(host->base + MCI_CMD);
	clk_cmd.bits.start_cmd = 1;
	clk_cmd.bits.update_clk_reg_only = 1;
	fhmci_writel(clk_cmd.cmd_arg, host->base + MCI_CMD);

	if (fh_mci_wait_cmd(host) != 0)
		fhmci_trace(3, "set card clk divider is failed!");
}

static void fh_mci_init_card(struct fhmci_host *host)
{
	unsigned int tmp_reg, tmp;
	unsigned long flags;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	tmp = fhmci_readl(host->base + MCI_PWREN);
	fh_mci_sys_reset(host);
	fh_mci_ctrl_power(host, POWER_OFF);
	udelay(500);
	/* card power on */
	fh_mci_ctrl_power(host, POWER_ON);
	udelay(200);

	fh_mci_sys_undo_reset(host);

	/* set phase shift */
	/* set card read threshold */

	/* clear MMC host intr */
	fhmci_writel(ALL_INT_CLR, host->base + MCI_RINTSTS);

	spin_lock_irqsave(&host->lock, flags);
	host->pending_events = 0;
	spin_unlock_irqrestore(&host->lock, flags);

	/* MASK MMC host intr */
	tmp_reg = fhmci_readl(host->base + MCI_INTMASK);
	tmp_reg &= ~ALL_INT_MASK;
	tmp_reg |= DATA_INT_MASK;
	fhmci_writel(tmp_reg, host->base + MCI_INTMASK);

	/* enable inner DMA mode and close intr of MMC host controler */
	tmp_reg = fhmci_readl(host->base + MCI_CTRL);
	tmp_reg &= ~INTR_EN;
	tmp_reg |= USE_INTERNAL_DMA | INTR_EN;
	fhmci_writel(tmp_reg, host->base + MCI_CTRL);

	/* set timeout param */
	fhmci_writel(DATA_TIMEOUT | RESPONSE_TIMEOUT, host->base + MCI_TIMEOUT);

	/* set FIFO param */
	if (host->pdata->fifo_depth > 15)
		tmp = 0x5;
	else
		tmp = 0x2;

	tmp_reg = ((tmp << 28) | ((host->pdata->fifo_depth / 2) << 16)
			| (((host->pdata->fifo_depth / 2) + 1) << 0));
	fhmci_writel(tmp_reg, host->base + MCI_FIFOTH);
}

int read_mci_ctrl_states(int id_mmc_sd)
{
	if ((id_mmc_sd == ID_SD0) && (mmc_sd0 != NULL))
		return mmc_sd0->rescan_disable;
	else if ((id_mmc_sd == ID_SD1) && (mmc_sd1 != NULL))
		return mmc_sd1->rescan_disable;

	return -1;
}

int storage_dev_set_mmc_rescan(struct mmc_ctrl *m_ctrl)
{
	unsigned int tmp;
	struct mmc_host *mmc_sd = NULL;
	tmp = m_ctrl->mmc_ctrl_state;

	if (m_ctrl->slot_idx == 1) {
		if (mmc_sd1 != NULL)
			mmc_sd = mmc_sd1;
	} else if (m_ctrl->slot_idx == 0) {
		if (mmc_sd0 != NULL)
			mmc_sd = mmc_sd0;
	}
	if ((tmp != TRUE) && (tmp != FALSE))
		return -1;

	if (tmp == TRUE) {
		if (mmc_sd != NULL) {
			mmc_sd->rescan_disable = TRUE;
			mmc_detect_change(mmc_sd, 0);
		}
	} else {
		if (mmc_sd != NULL) {
			mmc_sd->rescan_disable = FALSE;
			mmc_detect_change(mmc_sd, 0);
		}
	}
	return 0;
}

static void fh_mci_idma_start(struct fhmci_host *host)
{
	unsigned int tmp;

	fhmci_trace(2, "begin");
	fhmci_writel(host->dma_paddr, host->base + MCI_DBADDR);
	tmp = fhmci_readl(host->base + MCI_BMOD);
	tmp |= BMOD_DMA_EN;
	tmp |= BURST_INCR;
	fhmci_writel(tmp, host->base + MCI_BMOD);
}

static void fh_mci_idma_stop(struct fhmci_host *host)
{
	unsigned int tmp_reg;

	fhmci_trace(2, "begin");
	tmp_reg = fhmci_readl(host->base + MCI_BMOD);
	tmp_reg &= ~BMOD_DMA_EN;
	tmp_reg |= BMOD_SWR;
	fhmci_writel(tmp_reg, host->base + MCI_BMOD);
}

static int fh_mci_setup_data(struct fhmci_host *host, struct mmc_data *data)
{
	unsigned int sg_phyaddr, sg_length;
	unsigned int i, ret = 0;
	unsigned int data_size;
	unsigned int max_des, des_cnt;
	struct fhmci_des *des;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(data);

	host->data = data;

	if (data->flags & MMC_DATA_READ)
		host->dma_dir = DMA_FROM_DEVICE;
	else
		host->dma_dir = DMA_TO_DEVICE;

	host->dma_sg = data->sg;
	host->dma_sg_num = dma_map_sg(mmc_dev(host->mmc),
			data->sg, data->sg_len, host->dma_dir);
	fhmci_assert(host->dma_sg_num);
	fhmci_trace(2, "host->dma_sg_num is %d\n", host->dma_sg_num);
	data_size = data->blksz * data->blocks;

	if (data_size > (DMA_BUFFER * MAX_DMA_DES)) {
		fhmci_error("mci request data_size is too big!\n");
		ret = -1;
		goto out;
	}

	fhmci_trace(2, "host->dma_paddr is 0x%08X,host->dma_vaddr is 0x%08X\n",
			(unsigned int)host->dma_paddr,
			(unsigned int)host->dma_vaddr);

	max_des = (PAGE_SIZE/sizeof(struct fhmci_des));
	des = (struct fhmci_des *)host->dma_vaddr;
	des_cnt = 0;

	for (i = 0; i < host->dma_sg_num; i++) {
		sg_length = sg_dma_len(&data->sg[i]);
		sg_phyaddr = sg_dma_address(&data->sg[i]);
		fhmci_trace(2, "sg[%d] sg_length is 0x%08X, " \
				"sg_phyaddr is 0x%08X\n", \
				i, (unsigned int)sg_length, \
				(unsigned int)sg_phyaddr);
		while (sg_length) {
			des[des_cnt].idmac_des_ctrl = DMA_DES_OWN
				| DMA_DES_NEXT_DES;
			des[des_cnt].idmac_des_buf_addr = sg_phyaddr;
			/* idmac_des_next_addr is paddr for dma */
			des[des_cnt].idmac_des_next_addr = host->dma_paddr
				+ (des_cnt + 1) * sizeof(struct fhmci_des);

			if (sg_length >= 0x1F00) {
				des[des_cnt].idmac_des_buf_size = 0x1F00;
				sg_length -= 0x1F00;
				sg_phyaddr += 0x1F00;
			} else {
				/* FIXME:data alignment */
				des[des_cnt].idmac_des_buf_size = sg_length;
				sg_length = 0;
			}

			fhmci_trace(2, "des[%d] vaddr  is 0x%08X", i,
					(unsigned int)&des[i]);
			fhmci_trace(2, "des[%d].idmac_des_ctrl is 0x%08X",
			       i, (unsigned int)des[i].idmac_des_ctrl);
			fhmci_trace(2, "des[%d].idmac_des_buf_size is 0x%08X",
				i, (unsigned int)des[i].idmac_des_buf_size);
			fhmci_trace(2, "des[%d].idmac_des_buf_addr 0x%08X",
				i, (unsigned int)des[i].idmac_des_buf_addr);
			fhmci_trace(2, "des[%d].idmac_des_next_addr is 0x%08X",
				i, (unsigned int)des[i].idmac_des_next_addr);
			des_cnt++;
		}

		fhmci_assert(des_cnt < max_des);
	}
	des[0].idmac_des_ctrl |= DMA_DES_FIRST_DES;
	des[des_cnt - 1].idmac_des_ctrl |= DMA_DES_LAST_DES;
	des[des_cnt - 1].idmac_des_next_addr = 0;
out:
	return ret;
}

static int fh_mci_exec_cmd(struct fhmci_host *host, struct mmc_command *cmd,
		struct mmc_data *data)
{
	volatile union cmd_arg_s cmd_regs;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(cmd);

	host->cmd = cmd;

	fhmci_writel(cmd->arg, host->base + MCI_CMDARG);
	fhmci_trace(2, "arg_reg 0x%x, val 0x%x\n", MCI_CMDARG, cmd->arg);
	cmd_regs.cmd_arg = fhmci_readl(host->base + MCI_CMD);
	if (data) {
		cmd_regs.bits.data_transfer_expected = 1;
		if (data->flags & (MMC_DATA_WRITE | MMC_DATA_READ))
			cmd_regs.bits.transfer_mode = 0;

		if (data->flags & MMC_DATA_STREAM)
			cmd_regs.bits.transfer_mode = 1;

		if (data->flags & MMC_DATA_WRITE)
			cmd_regs.bits.read_write = 1;
		else if (data->flags & MMC_DATA_READ)
			cmd_regs.bits.read_write = 0;
	} else {
		cmd_regs.bits.data_transfer_expected = 0;
		cmd_regs.bits.transfer_mode = 0;
		cmd_regs.bits.read_write = 0;
	}
	cmd_regs.bits.send_auto_stop = 0;
#ifdef CONFIG_SEND_AUTO_STOP
	if ((host->mrq->stop) && (!(host->is_tuning)))
		cmd_regs.bits.send_auto_stop = 1;
#endif

	if (cmd == host->mrq->stop) {
		cmd_regs.bits.stop_abort_cmd = 1;
		cmd_regs.bits.wait_prvdata_complete = 0;
	} else {
		cmd_regs.bits.stop_abort_cmd = 0;
		cmd_regs.bits.wait_prvdata_complete = 1;
	}

	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_NONE:
		cmd_regs.bits.response_expect = 0;
		cmd_regs.bits.response_length = 0;
		cmd_regs.bits.check_response_crc = 0;
		break;
	case MMC_RSP_R1:
	case MMC_RSP_R1B:
		cmd_regs.bits.response_expect = 1;
		cmd_regs.bits.response_length = 0;
		cmd_regs.bits.check_response_crc = 1;
		break;
	case MMC_RSP_R2:
		cmd_regs.bits.response_expect = 1;
		cmd_regs.bits.response_length = 1;
		cmd_regs.bits.check_response_crc = 1;
		break;
	case MMC_RSP_R3:
		cmd_regs.bits.response_expect = 1;
		cmd_regs.bits.response_length = 0;
		cmd_regs.bits.check_response_crc = 0;
		break;
	default:
		fhmci_error("fh_mci: unhandled response type %02x\n",
				mmc_resp_type(cmd));
		return -EINVAL;
	}

	fhmci_trace(2, "send cmd of card is cmd->opcode = %d ", cmd->opcode);
	if (cmd->opcode == MMC_GO_IDLE_STATE)
		cmd_regs.bits.send_initialization = 1;
	else
		cmd_regs.bits.send_initialization = 0;
	/* CMD 11 check switch voltage */
	if (cmd->opcode == SD_SWITCH_VOLTAGE)
		cmd_regs.bits.volt_switch = 1;
	else
		cmd_regs.bits.volt_switch = 0;


	cmd_regs.bits.card_number = 0;
	cmd_regs.bits.cmd_index = cmd->opcode;
	cmd_regs.bits.start_cmd = 1;
	cmd_regs.bits.update_clk_reg_only = 0;
	fhmci_writel(DATA_INT_MASK, host->base + MCI_RINTSTS);
	fhmci_writel(cmd_regs.cmd_arg, host->base + MCI_CMD);
	fhmci_trace(2, "cmd_reg 0x%x, val 0x%x\n", MCI_CMD, cmd_regs.cmd_arg);

	if (fh_mci_wait_cmd(host) != 0) {
		fhmci_trace(3, "send card cmd is failed!");
		return -EINVAL;
	}
	return 0;
}

static void fh_mci_finish_request(struct fhmci_host *host,
		struct mmc_request *mrq)
{
	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(mrq);

	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;

	mmc_request_done(host->mmc, mrq);
}

static void fh_mci_cmd_done(struct fhmci_host *host, unsigned int stat)
{
	unsigned int i;
	struct mmc_command *cmd = host->cmd;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(cmd);


	for (i = 0; i < 4; i++) {
		if (mmc_resp_type(cmd) == MMC_RSP_R2) {
			cmd->resp[i] = fhmci_readl(host->base +
					MCI_RESP3 - i * 0x4);
			/* R2 must delay some time here ,when use UHI card,
			   need check why */
			udelay(1000);
		} else
			cmd->resp[i] = fhmci_readl(host->base +
					MCI_RESP0 + i * 0x4);
	}

	if (stat & RTO_INT_STATUS) {
		cmd->error = -ETIMEDOUT;
		fhmci_trace(3, "irq cmd status stat = 0x%x is timeout error!",
				stat);
	} else if (stat & (RCRC_INT_STATUS | RE_INT_STATUS)) {
		cmd->error = -EILSEQ;
		fhmci_trace(3, "irq cmd status stat = 0x%x is response error!",
				stat);
	}
	host->cmd = NULL;
}


static void fh_mci_data_done(struct fhmci_host *host, unsigned int stat)
{
	struct mmc_data *data = host->data;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(data);


	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, host->dma_dir);

	if (stat & (HTO_INT_STATUS | DRTO_INT_STATUS)) {
		data->error = -ETIMEDOUT;
		fhmci_trace(3, "irq data status stat = 0x%x is timeout error!",
				stat);
	} else if (stat & (EBE_INT_STATUS | SBE_INT_STATUS | FRUN_INT_STATUS
				| DCRC_INT_STATUS)) {
#ifndef CONFIG_MACH_FH8830_FPGA
		data->error = -EILSEQ;
#endif
		fhmci_trace(3, "irq data status stat = 0x%x is data error!",
				stat);
	}

	if (!data->error)
		data->bytes_xfered = data->blocks * data->blksz;
	else
		data->bytes_xfered = 0;

	host->data = NULL;
}


static int fh_mci_wait_cmd_complete(struct fhmci_host *host)
{
	unsigned int cmd_retry_count = 0;
	unsigned long cmd_jiffies_timeout;
	unsigned int cmd_irq_reg = 0;
	struct mmc_command *cmd = host->cmd;
	unsigned long flags;
	unsigned int cmd_done = 0;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(cmd);

	cmd_jiffies_timeout = jiffies + request_timeout;
	while (1) {

		do {
			spin_lock_irqsave(&host->lock, flags);
			cmd_irq_reg = readl(host->base + MCI_RINTSTS);

			if (cmd_irq_reg & CD_INT_STATUS) {
				fhmci_writel((CD_INT_STATUS | RTO_INT_STATUS
					| RCRC_INT_STATUS | RE_INT_STATUS),
					host->base + MCI_RINTSTS);
				spin_unlock_irqrestore(&host->lock, flags);
				cmd_done = 1;
				break;
/*				fh_mci_cmd_done(host, cmd_irq_reg);
				return 0;*/
			} else if (cmd_irq_reg & VOLT_SWITCH_INT_STATUS) {
				fhmci_writel(VOLT_SWITCH_INT_STATUS, \
						host->base + MCI_RINTSTS);
				spin_unlock_irqrestore(&host->lock, flags);
				cmd_done = 1;
				break;
/*				fh_mci_cmd_done(host, cmd_irq_reg);
				return 0;*/
			}
			spin_unlock_irqrestore(&host->lock, flags);
			cmd_retry_count++;
		} while (cmd_retry_count < retry_count &&
				host->get_cd(host) != CARD_UNPLUGED);

		cmd_retry_count = 0;

		if ((host->card_status == CARD_UNPLUGED)
			|| (host->get_cd(host) == CARD_UNPLUGED)) {
			cmd->error = -ETIMEDOUT;
			return -1;
		}
		if (cmd_done) {
			fh_mci_cmd_done(host, cmd_irq_reg);
			return 0;
		}

		if (!time_before(jiffies, cmd_jiffies_timeout)) {
			unsigned int i = 0;
			for (i = 0; i < 4; i++) {
				cmd->resp[i] = fhmci_readl(host->base \
						+ MCI_RESP0 + i * 0x4);
				printk(KERN_ERR "voltage switch read MCI_RESP");
				printk(KERN_ERR "%d : 0x%x\n", i, cmd->resp[i]);
			}
			cmd->error = -ETIMEDOUT;
			fhmci_trace(3, "wait cmd request complete is timeout!");
			return -1;
		}

		schedule();
	}
}
/*
 * designware support send stop command automatically when
 * read or wirte multi blocks
 */
#ifdef CONFIG_SEND_AUTO_STOP
static int fh_mci_wait_auto_stop_complete(struct fhmci_host *host)
{
	unsigned int cmd_retry_count = 0;
	unsigned long cmd_jiffies_timeout;
	unsigned int cmd_irq_reg = 0;
	unsigned long flags;

	fhmci_trace(2, "begin");
	fhmci_assert(host);

	cmd_jiffies_timeout = jiffies + request_timeout;
	while (1) {

		do {
			spin_lock_irqsave(&host->lock, flags);
			cmd_irq_reg = readl(host->base + MCI_RINTSTS);
			if (cmd_irq_reg & ACD_INT_STATUS) {
				fhmci_writel((ACD_INT_STATUS | RTO_INT_STATUS
					| RCRC_INT_STATUS | RE_INT_STATUS),
					host->base + MCI_RINTSTS);
				spin_unlock_irqrestore(&host->lock, flags);
				return 0;
			}
			spin_unlock_irqrestore(&host->lock, flags);
			cmd_retry_count++;
		} while (cmd_retry_count < retry_count);

		cmd_retry_count = 0;
		if (host->card_status == CARD_UNPLUGED)
			return -1;

		if (!time_before(jiffies, cmd_jiffies_timeout)) {
			fhmci_trace(3, "wait auto stop complete is timeout!");
			return -1;
		}

		schedule();
	}

}
#endif
static int fh_mci_wait_data_complete(struct fhmci_host *host)
{
	unsigned int tmp_reg;
	struct mmc_data *data = host->data;
	long time = request_timeout;
	unsigned long flags;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	fhmci_assert(data);

	time = wait_event_timeout(host->intr_wait,
		test_bit(FHMCI_PEND_DTO_b, &host->pending_events),
		time);

	/* Mask MMC host data intr */
	spin_lock_irqsave(&host->lock, flags);
	tmp_reg = fhmci_readl(host->base + MCI_INTMASK);
	tmp_reg &= ~DATA_INT_MASK;
	fhmci_writel(tmp_reg, host->base + MCI_INTMASK);
	host->pending_events &= ~FHMCI_PEND_DTO_m;
	spin_unlock_irqrestore(&host->lock, flags);

	if (((time <= 0)
		&& (!test_bit(FHMCI_PEND_DTO_b, &host->pending_events)))
		|| (host->card_status == CARD_UNPLUGED)) {

		data->error = -ETIMEDOUT;
		fhmci_trace(3, "wait data request complete is timeout! 0x%08X",
				host->irq_status);
		fh_mci_idma_stop(host);
		fh_mci_data_done(host, host->irq_status);
		return -1;
	}

	fh_mci_idma_stop(host);
	fh_mci_data_done(host, host->irq_status);
	return 0;
}


static int fh_mci_wait_card_complete(struct fhmci_host *host,
		struct mmc_data *data)
{
	unsigned int card_retry_count = 0;
	unsigned long card_jiffies_timeout;
	unsigned int card_status_reg = 0;

	fhmci_trace(2, "begin");
	fhmci_assert(host);
	/* fhmci_assert(data); */

	card_jiffies_timeout = jiffies + FH_MCI_DETECT_TIMEOUT;
	while (1) {

		do {
			card_status_reg = readl(host->base + MCI_STATUS);
			if (!(card_status_reg & DATA_BUSY)) {
				fhmci_trace(2, "end");
				return 0;
			}
			card_retry_count++;
		} while (card_retry_count < retry_count);
		card_retry_count = 0;

		if (host->card_status == CARD_UNPLUGED) {
			data->error = -ETIMEDOUT;
			return -1;
		}

		if (!time_before(jiffies, card_jiffies_timeout)) {
			if (data != NULL)
				data->error = -ETIMEDOUT;
			fhmci_trace(3, "wait card ready complete is timeout!");
			return -1;
		}

		schedule();
	}
}

static unsigned long t = 0;
static unsigned long cmds = 0;
static unsigned long long send_byte_count = 0;
static struct timeval in_cmd, out_cmd;
static struct timeval *x = &out_cmd, *y = &in_cmd;
static unsigned long max = 0, sum = 0;
static unsigned long called = 0, ended = 0;

static void fh_mci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct fhmci_host *host = mmc_priv(mmc);
	int byte_cnt = 0;
	#ifdef CONFIG_SEND_AUTO_STOP
	int trans_cnt;
	#endif
	int fifo_count = 0, tmp_reg;
	int ret = 0;
	unsigned long flags;

	if (host->id == 1) {
		called++;
		memset(x, 0, sizeof(struct timeval));
		memset(y, 0, sizeof(struct timeval));
		do_gettimeofday(y);
	}

	fhmci_trace(2, "begin");
	fhmci_assert(mmc);
	fhmci_assert(mrq);
	fhmci_assert(host);

	host->mrq = mrq;
	host->irq_status = 0;


	if (host->card_status == CARD_UNPLUGED) {
		mrq->cmd->error = -ENODEV;
		goto  request_end;
	}
#if 1
	ret = fh_mci_wait_card_complete(host, mrq->data);

	if (ret) {
		mrq->cmd->error = ret;
		goto request_end;
	}
#endif
	/* prepare data */
	if (mrq->data) {
		ret = fh_mci_setup_data(host, mrq->data);
		if (ret) {
			mrq->data->error = ret;
			fhmci_trace(3, "data setup is error!");
			goto request_end;
		}

		byte_cnt = mrq->data->blksz * mrq->data->blocks;
		fhmci_writel(byte_cnt, host->base + MCI_BYTCNT);
		fhmci_writel(mrq->data->blksz, host->base + MCI_BLKSIZ);

		tmp_reg = fhmci_readl(host->base + MCI_CTRL);
		tmp_reg |= FIFO_RESET;
		fhmci_writel(tmp_reg, host->base + MCI_CTRL);

		do {
			tmp_reg = fhmci_readl(host->base + MCI_CTRL);
			fifo_count++;
			if (fifo_count >= retry_count) {
				printk(KERN_INFO "fifo reset is timeout!");
				return;
			}
		} while (tmp_reg&FIFO_RESET);

		/* start DMA */
		fh_mci_idma_start(host);
	} else {
		fhmci_writel(0, host->base + MCI_BYTCNT);
		fhmci_writel(0, host->base + MCI_BLKSIZ);
	}

	/* send command */
	ret = fh_mci_exec_cmd(host, mrq->cmd, mrq->data);
	if (ret) {
		mrq->cmd->error = ret;
		fh_mci_idma_stop(host);
		fhmci_trace(3, "can't send card cmd! ret = %d", ret);
		goto request_end;
	}

	/* wait command send complete */
	ret = fh_mci_wait_cmd_complete(host);

	/* start data transfer */
	if (mrq->data) {
		if (!(mrq->cmd->error)) {
			/* Open MMC host data intr */
			spin_lock_irqsave(&host->lock, flags);
			tmp_reg = fhmci_readl(host->base + MCI_INTMASK);
			tmp_reg |= DATA_INT_MASK;
			fhmci_writel(tmp_reg, host->base + MCI_INTMASK);
			spin_unlock_irqrestore(&host->lock, flags);
		/* wait data transfer complete */
		ret = fh_mci_wait_data_complete(host);
		} else {
			/* CMD error in data command */
			fh_mci_idma_stop(host);
		}

		if (mrq->stop) {
#ifdef CONFIG_SEND_AUTO_STOP
			trans_cnt = fhmci_readl(host->base + MCI_TCBCNT);
			/* send auto stop */
			if ((trans_cnt == byte_cnt) && (!(host->is_tuning))) {
				fhmci_trace(3, "byte_cnt = %d, trans_cnt = %d",
						byte_cnt, trans_cnt);
				ret = fh_mci_wait_auto_stop_complete(host);
				if (ret) {
					mrq->stop->error = -ETIMEDOUT;
					goto request_end;
				}
			} else {
#endif
				/* send soft stop command */
				fhmci_trace(3, "this time, send soft stop");
				ret = fh_mci_exec_cmd(host, host->mrq->stop,
						host->data);
				if (ret) {
					mrq->stop->error = ret;
					goto request_end;
				}
				ret = fh_mci_wait_cmd_complete(host);
				if (ret)
					goto request_end;
#ifdef CONFIG_SEND_AUTO_STOP
			}
#endif
		}
	}

request_end:
	/* clear MMC host intr */
	spin_lock_irqsave(&host->lock, flags);
	fhmci_writel(ALL_INT_CLR & (~SDIO_INT_STATUS),
			host->base + MCI_RINTSTS);
	spin_unlock_irqrestore(&host->lock, flags);

	fh_mci_finish_request(host, mrq);


	if (host->id == 1) {
		ended++;
		do_gettimeofday(x);

		/* Perform the carry for the later subtraction by updating y. */
		if (x->tv_usec < y->tv_usec) {
			int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
			y->tv_usec -= 1000000 * nsec;
			y->tv_sec += nsec;
		}
		if (x->tv_usec - y->tv_usec > 1000000) {
			int nsec = (x->tv_usec - y->tv_usec) / 1000000;
			y->tv_usec += 1000000 * nsec;
			y->tv_sec -= nsec;
		}
		/* Compute the time remaining to wait.
		 * tv_usec is certainly positive. */
		if (((x->tv_sec - y->tv_sec) * 1000
				+ x->tv_usec - y->tv_usec) > max) {
			max = (x->tv_sec - y->tv_sec)
					* 1000 + x->tv_usec - y->tv_usec;
		}

		sum += (x->tv_sec - y->tv_sec) * 1000 + x->tv_usec - y->tv_usec;

		send_byte_count += byte_cnt;
		cmds++;

		if (jiffies - t > HZ) {
			/*
			 * pr_info("SDIO HOST send_byte_count:
			 * %llu in %u cmds, max cost time: %lu,
			 * sum: %lu, ave: %lu\ncalled: %lu, ended: %lu\n",
			 * send_byte_count, cmds, max, sum,
			 * sum / cmds, called, ended);
			 */
			t = jiffies;
			send_byte_count = 0;
			cmds = 0;
			max = 0;
			sum = 0;
			called = 0;
			ended = 0;
		}
	}
}

static void fh_mci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct fhmci_host *host = mmc_priv(mmc);
	unsigned int tmp_reg;
	u32 ctrl;

	fhmci_trace(2, "begin");
	fhmci_assert(mmc);
	fhmci_assert(ios);
	fhmci_assert(host);

	fhmci_trace(3, "ios->power_mode = %d ", ios->power_mode);
	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		/*
		 * Set controller working voltage as 3.3V before power off.
		 */
		ctrl = fhmci_readl(host->base + MCI_UHS_REG);
		ctrl &= ~FH_SDXC_CTRL_VDD_180;
		fhmci_trace(3, "set voltage %d[addr 0x%x]", ctrl, MCI_UHS_REG);
		fhmci_writel(ctrl, host->base + MCI_UHS_REG);

		fh_mci_ctrl_power(host, POWER_OFF);
		break;
	case MMC_POWER_UP:
	case MMC_POWER_ON:
		fh_mci_ctrl_power(host, POWER_ON);
		break;
	}
	fhmci_trace(3, "ios->clock = %d ", ios->clock);
	if (ios->clock) {

		fh_mci_control_cclk(host, DISABLE);
		fh_mci_set_cclk(host, ios->clock);
		fh_mci_control_cclk(host, ENABLE);

		/* speed mode check ,if it is DDR50 set DDR mode*/
		if ((ios->timing == MMC_TIMING_UHS_DDR50)) {
			ctrl = fhmci_readl(host->base + MCI_UHS_REG);
			if (!(FH_SDXC_CTRL_DDR_REG & ctrl)) {
				ctrl |= FH_SDXC_CTRL_DDR_REG;
				fhmci_writel(ctrl, host->base + MCI_UHS_REG);
			}
		}
	} else {
		fh_mci_control_cclk(host, DISABLE);
		if ((ios->timing != MMC_TIMING_UHS_DDR50)) {
			ctrl = fhmci_readl(host->base + MCI_UHS_REG);
			if (FH_SDXC_CTRL_DDR_REG & ctrl) {
				ctrl &= ~FH_SDXC_CTRL_DDR_REG;
				fhmci_writel(ctrl, host->base + MCI_UHS_REG);
			}
		}
	}

	/* set bus_width */
	fhmci_trace(3, "ios->bus_width = %d ", ios->bus_width);
	if (ios->bus_width == MMC_BUS_WIDTH_4) {
		tmp_reg = fhmci_readl(host->base + MCI_CTYPE);
		tmp_reg |= CARD_WIDTH;
		fhmci_writel(tmp_reg, host->base + MCI_CTYPE);
	} else {
		tmp_reg = fhmci_readl(host->base + MCI_CTYPE);
		tmp_reg &= ~CARD_WIDTH;
		fhmci_writel(tmp_reg, host->base + MCI_CTYPE);
	}
}

static void fhmci_enable_sdio_irq(struct mmc_host *host, int enable)
{
	struct fhmci_host *fh_host = mmc_priv(host);
	unsigned int reg_value;
	unsigned long flags;

	if (enable) {
		local_irq_save(flags);

		reg_value = fhmci_readl(fh_host->base + MCI_INTMASK);
		reg_value |= 0x10000;
		fhmci_writel(reg_value, fh_host->base + MCI_INTMASK);
		local_irq_restore(flags);
	} else {
		reg_value = fhmci_readl(fh_host->base + MCI_INTMASK);
		reg_value &= ~0xffff0000;
		fhmci_writel(reg_value, fh_host->base + MCI_INTMASK);
	}

}


static int fh_mci_get_ro(struct mmc_host *mmc)
{
	unsigned ret;
	struct fhmci_host *host = mmc_priv(mmc);

	fhmci_trace(2, "begin");
	fhmci_assert(mmc);

	ret = host->get_ro(host);

	return ret;
}

/**
 * @brief get the status of SD card's CD pin
 *
 * @param [in] mmc host struct
 *
 * @return "1": sd card in the slot, "0": sd card is not in the slot, "2":status of sd card no changed
 */
static int fh_mci_get_cd(struct mmc_host *mmc)
{
	unsigned int i, curr_status, status[3] = {0}, detect_retry_count = 0;
	struct fhmci_host *host = mmc_priv(mmc);

	while (1) {
		for (i = 0; i < 3; i++) {
			status[i] = host->get_cd(host);
			udelay(10);
		}
		if ((status[0] == status[1]) && (status[0] == status[2]))
			break;

		detect_retry_count++;
		if (detect_retry_count >= retry_count) {
			fhmci_error("this is a dithering,card detect error!");
			goto err;
		}
	}
	curr_status = status[0];
	if (curr_status != host->card_status) {
		host->card_status = curr_status;
		if (curr_status != CARD_UNPLUGED) {
			fh_mci_init_card(host);
			printk(KERN_INFO "card%d connected!\n", host->id);
			mmc->rescan_count = 0;
			return 1;
		} else {
			printk(KERN_INFO "card%d disconnected!\n", host->id);
			return 0;
		}
	}
	if (mmc->card == NULL)
		fh_mci_init_card(host);
err:
	return 2;
}

static const struct mmc_host_ops fh_mci_ops = {
	.request = fh_mci_request,
	.set_ios = fh_mci_set_ios,
	.get_ro	 = fh_mci_get_ro,
	.enable_sdio_irq = fhmci_enable_sdio_irq,
	.get_cd  = fh_mci_get_cd,
};

static irqreturn_t hisd_irq(int irq, void *dev_id)
{
	struct fhmci_host *host = dev_id;
	u32 state = 0;
	int handle = 0;

	state = fhmci_readl(host->base + MCI_RINTSTS);

#ifndef CONFIG_SEND_AUTO_STOP
	/* bugfix: when send soft stop to SD Card, Host will report
	   sdio interrupt, This situation needs to be avoided */
	if ((host->mmc->card != NULL)
			&& (host->mmc->card->type == MMC_TYPE_SDIO)) {
#endif
		if (state & SDIO_INT_STATUS) {
			if (fhmci_readl(host->base + MCI_INTMASK) & SDIO_INT_STATUS) {
				fhmci_writel(SDIO_INT_STATUS, host->base + MCI_RINTSTS);
				mmc_signal_sdio_irq(host->mmc);
				handle = 1;
			}
		}
#ifndef CONFIG_SEND_AUTO_STOP
	}
#endif

	if (state & DATA_INT_MASK) {
		handle = 1;
		host->pending_events |= FHMCI_PEND_DTO_m;

		host->irq_status = fhmci_readl(host->base + MCI_RINTSTS);
		if (host->irq_status & (DCRC_INT_STATUS|SBE_INT_STATUS|EBE_INT_STATUS)) {
#ifndef CONFIG_MACH_FH8830_FPGA
			printk(KERN_ERR "SDC CRC error:%08x,.\n",
				host->irq_status);
#endif
		}
		fhmci_writel(DATA_INT_MASK , host->base + MCI_RINTSTS);

		wake_up(&host->intr_wait);
	}

	/*if (state & 0x10000) {
		handle = 1;
		fhmci_writel(0x10000, host->base + MCI_RINTSTS);
		mmc_signal_sdio_irq(host->mmc);
	}*/

	if (handle)
		return IRQ_HANDLED;

	return IRQ_NONE;
}

static int __devinit fh_mci_probe(struct platform_device *pdev)
{
	struct resource *regs;
	struct mmc_host *mmc;
	struct fhmci_host *host = NULL;
	int ret = 0, irq;

	fhmci_trace(2, "begin");
	fhmci_assert(pdev);

	mmc = mmc_alloc_host(sizeof(struct fhmci_host), &pdev->dev);
	if (!mmc) {
		fhmci_error("no mem for hi mci host controller!\n");
		ret = -ENOMEM;
		goto out;
	}
	host = mmc_priv(mmc);
	host->pdata = pdev->dev.platform_data;
	mmc->ops = &fh_mci_ops;
	mmc->rescan_disable = FALSE;
	mmc->f_min = DIV_ROUND_UP(host->pdata->bus_hz, 510);
	mmc->f_max = host->pdata->bus_hz;

	if (host->pdata->caps)
		mmc->caps = host->pdata->caps;
	else
		mmc->caps = 0;

	mmc->caps |= MMC_CAP_SDIO_IRQ;

	if (0 == pdev->id) {
		mmc_sd0 = mmc;
	} else if (1 == pdev->id) {
		mmc_sd1 = mmc;
	} else {
		fhmci_error("fhmci host id error!");
		goto out;
	}
	/* reload by this controller */
	mmc->max_blk_count = 2048;
	mmc->max_segs = 1024;
	mmc->max_seg_size = mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_req_size = mmc->max_blk_size * mmc->max_blk_count;

	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->ocr = mmc->ocr_avail;

	host->dma_vaddr = dma_alloc_coherent(&pdev->dev, PAGE_SIZE,
			&host->dma_paddr, GFP_KERNEL);
	if (!host->dma_vaddr) {
		fhmci_error("no mem for fhmci dma!\n");
		ret = -ENOMEM;
		goto out;
	}

	host->mmc = mmc;
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		fhmci_error("request resource error!\n");
		ret = -ENXIO;
		goto out;
	}

	host->id = pdev->id;
	host->base = ioremap_nocache(regs->start, regs->end - regs->start + 1);
	if (!host->base) {
		fhmci_error("no mem for fhmci base!\n");
		ret = -ENOMEM;
		goto out;
	}

	if (host->pdata->init)
		host->pdata->init(pdev->id, NULL, NULL);

	if (host->pdata->get_cd)
		host->get_cd = host->pdata->get_cd;
	else
		host->get_cd = fh_mci_sys_card_detect;

	if (host->pdata->get_ro)
		host->get_ro = host->pdata->get_ro;
	else
		host->get_ro = fh_mci_ctrl_card_readonly;

	/* enable mmc clk */
	fh_mci_sys_ctrl_init(host);

	/* enable card */
	spin_lock_init(&host->lock);
	platform_set_drvdata(pdev, mmc);
	mmc_add_host(mmc);

	fhmci_writel(SD_POWER_ON, host->base + MCI_PWREN);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk(KERN_ERR "no IRQ defined!\n");
		goto out;
	}

	init_waitqueue_head(&host->intr_wait);

	host->irq = irq;
	ret = request_irq(irq, hisd_irq, 0, DRIVER_NAME, host);
	if (ret) {
		printk(KERN_ERR "request_irq error!\n");
		goto out;
	}


	return 0;
out:
	if (host) {

		if (host->base)
			iounmap(host->base);

		if (host->dma_vaddr)
			dma_free_coherent(&pdev->dev, PAGE_SIZE,
					host->dma_vaddr, host->dma_paddr);
	}
	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

/* for wifi Cypress 43438/43455
 * (Note: sd_id is the sdio index used by wifi)
 */
void fh_sdio_card_scan(int sd_id)
{
	printk(KERN_ERR "%s-%d mmc_sd0->caps 0x%x mmc_sd1->caps 0x%x\n",
		__func__, __LINE__, mmc_sd0->caps, mmc_sd1->caps);
	if (sd_id == 1) {
		mmc_sd1->caps &= ~MMC_CAP_NEEDS_POLL;
		mmc_sd1->caps &= ~MMC_CAP_NONREMOVABLE;
		if (NULL != mmc_sd1) {
			printk(KERN_ERR "%s-%d, enter\n", __func__, __LINE__);
			mmc_detect_change(mmc_sd1, 0);
		}
		msleep(100);
		mmc_sd1->caps |= MMC_CAP_NONREMOVABLE;
	} else if (sd_id == 0) {
		mmc_sd0->caps &= ~MMC_CAP_NEEDS_POLL;
		mmc_sd0->caps &= ~MMC_CAP_NONREMOVABLE;
		if (NULL != mmc_sd0) {
			printk(KERN_ERR "%s-%d, enter\n", __func__, __LINE__);
			mmc_detect_change(mmc_sd0, 0);
		}
		msleep(100);
		mmc_sd0->caps |= MMC_CAP_NONREMOVABLE;
	} else {
		printk(KERN_ERR "%s-%d, sd_id invalid!\n", __func__, __LINE__);
	}
	printk(KERN_ERR "%s-%d mmc_sd0->caps 0x%x mmc_sd1->caps 0x%x\n",
		__func__, __LINE__, mmc_sd0->caps, mmc_sd1->caps);

	return;
}
EXPORT_SYMBOL_GPL(fh_sdio_card_scan);

static int __devexit fh_mci_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);

	fhmci_trace(2, "begin");
	fhmci_assert(pdev);

	platform_set_drvdata(pdev, NULL);

	if (mmc) {
		struct fhmci_host *host = mmc_priv(mmc);

		free_irq(host->irq, host);
		mmc_remove_host(mmc);
		fh_mci_ctrl_power(host, POWER_OFF);
		fh_mci_control_cclk(host, DISABLE);
		iounmap(host->base);
		dma_free_coherent(&pdev->dev, PAGE_SIZE, host->dma_vaddr,
				host->dma_paddr);
		mmc_free_host(mmc);
	}
	return 0;
}

#ifdef CONFIG_PM
static int fh_mci_suspend(struct platform_device *dev, pm_message_t state)
{
	struct mmc_host *mmc = platform_get_drvdata(dev);
	struct fhmci_host *host;
	int ret = 0;

	fhmci_trace(2, "begin");
	fhmci_assert(dev);

	if (mmc) {
		ret = mmc_suspend_host(mmc);

		host = mmc_priv(mmc);
	}

	fhmci_trace(2, "end");

	return ret;
}

static int fh_mci_resume(struct platform_device *dev)
{
	struct mmc_host *mmc = platform_get_drvdata(dev);
	struct fhmci_host *host;
	int ret = 0;

	fhmci_trace(2, "begin");
	fhmci_assert(dev);

	if (mmc) {
		host = mmc_priv(mmc);
		/* enable mmc clk */
		fh_mci_sys_ctrl_init(host);
		/* enable card */
		fh_mci_init_card(host);

		ret = mmc_resume_host(mmc);
	}

	fhmci_trace(2, "end");

	return ret;
}
#else
#define fh_mci_suspend	NULL
#define fh_mci_resume	NULL
#endif


static struct platform_driver fh_mci_driver = {
	.probe         = fh_mci_probe,
	.remove        = fh_mci_remove,
	.suspend       = fh_mci_suspend,
	.resume        = fh_mci_resume,
	.driver        = {
		.name          = DRIVER_NAME,
	},
};

static ssize_t fh_mci_rescan_control(struct class *cls,
		struct class_attribute *attr, const char *_buf, size_t _count)
{
	int cmd = 0;
	int err = 0;

	err = kstrtoint(_buf, 10, &cmd);
	if (err)
		return _count;

	if (cmd) {
		if (mmc_sd0)
			mmc_sd0->rescan_count = 0;
		if (mmc_sd1)
			mmc_sd1->rescan_count = 0;
	}
	return _count;
}

static struct class *fhmci_rescan_class;

static CLASS_ATTR(mmc_rescan, 0666, NULL, fh_mci_rescan_control);

static void fh_mci_rescan_init(void)
{
	int err = 0;

	fhmci_rescan_class = class_create(THIS_MODULE, "fhmci");
	err = class_create_file(fhmci_rescan_class, &class_attr_mmc_rescan);
	if (err)
		fhmci_error("fhmci_rescan_class: create class file failed!");
}

static int __init fh_mci_init(void)
{
	int ret = 0;

	fhmci_trace(2, "mci init begin");
	fh_mci_rescan_init();
	ret = platform_driver_register(&fh_mci_driver);
	if (ret)
		fhmci_error("Platform driver register is failed!");

	return ret;
}

static void __exit fh_mci_exit(void)
{
	fhmci_trace(2, "begin");
	platform_driver_unregister(&fh_mci_driver);
}
module_init(fh_mci_init);
module_exit(fh_mci_exit);
MODULE_LICENSE("GPL");
