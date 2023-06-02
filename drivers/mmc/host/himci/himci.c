/*
 * himci.c - hisilicon MMC Host driver
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) "himci: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/sd.h>
#include <linux/slab.h>

#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/spinlock.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <linux/sizes.h>
#include <mach/io.h>

#include <linux/io.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>

#include "himci_reg.h"
#include "himci.h"
#include "himci_proc.h"

#ifdef CONFIG_ARCH_HI3516A
#include "himci_hi3516a.c"
#endif

#ifdef CONFIG_ARCH_HI3518EV20X
#include "himci_hi3518ev20x.c"
#endif

#define DRIVER_NAME "himci"
#define CMD_DES_PAGE_SIZE	(2 * PAGE_SIZE)

static unsigned int detect_time = HI_MCI_DETECT_TIMEOUT;
static unsigned int retry_count = MAX_RETRY_COUNT;
static unsigned int request_timeout = HI_MCI_REQUEST_TIMEOUT;
int trace_level = HIMCI_TRACE_LEVEL;
unsigned int slot_index = 0;
struct himci_host *mci_host[HIMCI_SLOT_NUM] = {NULL};

#ifdef MODULE

module_param(detect_time, uint, 0600);
MODULE_PARM_DESC(detect_timer, "card detect time (default:500ms))");

module_param(retry_count, uint, 0600);
MODULE_PARM_DESC(retry_count, "retry count times (default:100))");

module_param(request_timeout, uint, 0600);
MODULE_PARM_DESC(request_timeout, "Request timeout time (default:3s))");

module_param(trace_level, int, 0600);
MODULE_PARM_DESC(trace_level, "HIMCI_TRACE_LEVEL");

#endif

/* reset MMC host controller */
static void himci_sys_reset(struct himci_host *host)
{
	unsigned int reg_value;
	unsigned long flags;

	himci_trace(2, "reset");

	local_irq_save(flags);

	reg_value = himci_readl(host->base + MCI_BMOD);
	reg_value |= BMOD_SWR;
	himci_writel(reg_value, host->base + MCI_BMOD);
	mdelay(10);

	reg_value = himci_readl(host->base + MCI_BMOD);
	reg_value |= BURST_16 | BURST_INCR;
	himci_writel(reg_value, host->base + MCI_BMOD);

	reg_value = himci_readl(host->base + MCI_CTRL);
	reg_value |=  CTRL_RESET | FIFO_RESET | DMA_RESET;
	himci_writel(reg_value, host->base + MCI_CTRL);

	local_irq_restore(flags);
}

static void himci_ctrl_power(struct himci_host *host,
		unsigned int flag, unsigned int force)
{
	unsigned int port;

	himci_trace(2, "begin");

	port = host->port;

	if (host->power_status != flag || force == FORCE_ENABLE) {
		unsigned int reg_value;

		if (flag == POWER_OFF) {
			reg_value = himci_readl(host->base + MCI_RESET_N);
			reg_value &= ~(MMC_RST_N << port);
			himci_writel(reg_value, host->base + MCI_RESET_N);
		}

		reg_value = himci_readl(host->base + MCI_PWREN);
		if (flag == POWER_OFF)
			reg_value &= ~(0x1 << port);
		else
			reg_value |= (0x1 << port);

		himci_writel(reg_value, host->base + MCI_PWREN);

		if (flag == POWER_ON) {
			reg_value = himci_readl(host->base + MCI_RESET_N);
			reg_value |= (MMC_RST_N << port);
			himci_writel(reg_value, host->base + MCI_RESET_N);
		}

		if (in_interrupt())
			mdelay(100);
		else
			msleep(100);

		host->power_status = flag;
	}
}

/**********************************************
 *1: card off
 *0: card on
 ***********************************************/
static unsigned int himci_sys_card_detect(struct himci_host *host)
{
	unsigned int card_status;

	card_status = readl(host->base + MCI_CDETECT);
	card_status &= (HIMCI_CARD0 << host->port);
	if (card_status)
		card_status = 1;
	else
		card_status = 0;

	return card_status;
}

/**********************************************
 *1: card readonly
 *0: card read/write
 ***********************************************/
static unsigned int himci_ctrl_card_readonly(struct himci_host *host)
{
	unsigned int card_value = himci_readl(host->base + MCI_WRTPRT);
	return card_value & (HIMCI_CARD0 << host->port);
}

static int himci_wait_cmd(struct himci_host *host)
{
	int wait_retry_count = 0;
	unsigned int reg_data = 0;
	unsigned long flags;

	himci_trace(2, "begin");
	himci_assert(host);

	while (1) {
		/*
		 * Check if CMD::start_cmd bit is clear.
		 * start_cmd = 0 means MMC Host controller has loaded registers
		 * and next command can be loaded in.
		 */
		reg_data = himci_readl(host->base + MCI_CMD);
		if ((reg_data & START_CMD) == 0)
			return 0;

		/* Check if Raw_Intr_Status::HLE bit is set. */
		spin_lock_irqsave(&host->lock, flags);
		reg_data = himci_readl(host->base + MCI_RINTSTS);
		if (reg_data & HLE_INT_STATUS) {
			reg_data |= HLE_INT_STATUS;
			himci_writel(reg_data, host->base + MCI_RINTSTS);
			spin_unlock_irqrestore(&host->lock, flags);

			himci_trace(5, "Other CMD is running,"
				"please operate cmd again!");
			return 1;
		}

		spin_unlock_irqrestore(&host->lock, flags);
		udelay(100);

		/* Check if number of retries for this are over. */
		wait_retry_count++;
		if (wait_retry_count >= retry_count) {
			himci_trace(5, "send cmd is timeout!");
			return -1;
		}
	}
}

static void himci_control_cclk(struct himci_host *host, unsigned int flag)
{
	unsigned int reg;
	union cmd_arg_u cmd_reg;

	himci_trace(2, "begin");
	himci_assert(host);

	reg = himci_readl(host->base + MCI_CLKENA);
	if (flag == ENABLE) {
		reg |= (CCLK_ENABLE << host->port);
		reg |= (CCLK_LOW_POWER << host->port);
	} else {
		reg &= ~(CCLK_ENABLE << host->port);
		reg &= ~(CCLK_LOW_POWER << host->port);
	}
	himci_writel(reg, host->base + MCI_CLKENA);

	cmd_reg.cmd_arg = himci_readl(host->base + MCI_CMD);
	cmd_reg.bits.start_cmd = 1;
	cmd_reg.bits.card_number = host->port;
	cmd_reg.bits.cmd_index = 0;
	cmd_reg.bits.data_transfer_expected = 0;
	cmd_reg.bits.update_clk_reg_only = 1;
	cmd_reg.bits.response_expect = 0;
	cmd_reg.bits.send_auto_stop = 0;
	cmd_reg.bits.wait_prvdata_complete = 0;
	cmd_reg.bits.check_response_crc = 0;
	himci_writel(cmd_reg.cmd_arg, host->base + MCI_CMD);
	if (himci_wait_cmd(host) != 0)
		himci_trace(5, "disable or enable clk is timeout!");
}

static void himci_set_cclk(struct himci_host *host, unsigned int cclk)
{
	unsigned int reg_value;
	union cmd_arg_u clk_cmd;
	unsigned int hclk;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(cclk);

	hclk = cclk > MMC_CRG_MIN ? cclk : MMC_CRG_MIN;
	clk_set_rate(host->clk, hclk);

	hclk = clk_get_rate(host->clk);

	/*
	 * set card clk divider value,
	 * clk_divider = Fmmcclk/(Fmmc_cclk * 2)
	 */
	reg_value = hclk / (cclk * 2);
	if ((hclk % (cclk * 2)) && (hclk > cclk))
		reg_value++;
	if (reg_value > 0xFF)
		reg_value = 0xFF;

	host->hclk = hclk;
	host->cclk = reg_value ? (hclk / (reg_value * 2)) : hclk;
	himci_writel((reg_value << (host->port * 8)),
			host->base + MCI_CLKDIV);

	clk_cmd.cmd_arg = himci_readl(host->base + MCI_CMD);
	clk_cmd.bits.start_cmd = 1;
	clk_cmd.bits.card_number = host->port;
	clk_cmd.bits.update_clk_reg_only = 1;
	clk_cmd.bits.cmd_index = 0;
	clk_cmd.bits.data_transfer_expected = 0;
	clk_cmd.bits.response_expect = 0;
	himci_writel(clk_cmd.cmd_arg, host->base + MCI_CMD);
	if (himci_wait_cmd(host) != 0)
		himci_trace(5, "set card clk divider is failed!");
}

static void himci_init_host(struct himci_host *host)
{
	unsigned int tmp_reg = 0;
	unsigned long flags;

	himci_trace(2, "begin");
	himci_assert(host);

	himci_sys_reset(host);

#ifdef CONFIG_ARCH_HI3518EV20X
	/* sd use clk0 emmc use clk1 */
	himci_writel(0x4, host->base + MCI_CLKSRC);
#endif

	/* set drv/smpl phase shift */
	tmp_reg |= SMPL_PHASE_DFLT | DRV_PHASE_DFLT;
	himci_writel(tmp_reg, host->base + MCI_UHS_REG_EXT);

	/* set card read threshold */
	himci_writel(RW_THRESHOLD_SIZE, host->base + MCI_CARDTHRCTL);

	/* clear MMC host intr */
	himci_writel(ALL_INT_CLR, host->base + MCI_RINTSTS);

	spin_lock_irqsave(&host->lock, flags);
	host->pending_events = 0;
	spin_unlock_irqrestore(&host->lock, flags);

	/* MASK MMC all host intr */
	tmp_reg = himci_readl(host->base + MCI_INTMASK);
	tmp_reg &= ~ALL_INT_MASK;
	tmp_reg |= DATA_INT_MASK;
	himci_writel(tmp_reg, host->base + MCI_INTMASK);

	/* enable inner DMA mode and close intr of MMC host controler */
	tmp_reg = himci_readl(host->base + MCI_CTRL);
	tmp_reg &= ~INTR_EN;
	tmp_reg |= USE_INTERNAL_DMA | INTR_EN;
	himci_writel(tmp_reg, host->base + MCI_CTRL);

	/* set timeout param */
	himci_writel(DATA_TIMEOUT | RESPONSE_TIMEOUT, host->base + MCI_TIMEOUT);

	/* set FIFO param */
	tmp_reg = 0;
	tmp_reg |= BURST_SIZE | RX_WMARK | TX_WMARK;
	himci_writel(tmp_reg, host->base + MCI_FIFOTH);
}

static void himci_detect_card(unsigned long arg)
{
	struct himci_host *host = (struct himci_host *)arg;
	unsigned int i, curr_status, status[5], detect_retry_count = 0;

	himci_assert(host);

	while (1) {
		for (i = 0; i < 5; i++) {
			status[i] = himci_sys_card_detect(host);
			udelay(10);
		}
		if ((status[0] == status[1])
				&& (status[0] == status[2])
				&& (status[0] == status[3])
				&& (status[0] == status[4]))
			break;

		detect_retry_count++;
		if (detect_retry_count >= retry_count) {
			himci_error("this is a dithering, card detect error!");
			goto err;
		}
	}
	curr_status = status[0];
	if (curr_status != host->card_status) {
		himci_trace(2, "begin card_status = %d\n", host->card_status);
		host->card_status = curr_status;
		if (curr_status != CARD_UNPLUGED) {
			himci_init_host(host);
			pr_info("card connected!\n");
		} else {
			pr_info("card disconnected!\n");
		}

		mmc_detect_change(host->mmc, 0);
	}
err:
	mod_timer(&host->timer, jiffies + detect_time);
}

static void himci_idma_start(struct himci_host *host)
{
	unsigned int tmp;

	himci_trace(2, "begin");
	himci_writel(host->dma_paddr, host->base + MCI_DBADDR);
	tmp = himci_readl(host->base + MCI_BMOD);
	tmp |= BMOD_DMA_EN;
	himci_writel(tmp, host->base + MCI_BMOD);
}

static void himci_idma_stop(struct himci_host *host)
{
	unsigned int tmp_reg;

	himci_trace(2, "begin");
	tmp_reg = himci_readl(host->base + MCI_BMOD);
	tmp_reg &= ~BMOD_DMA_EN;
	himci_writel(tmp_reg, host->base + MCI_BMOD);
}

static int himci_setup_data(struct himci_host *host, struct mmc_data *data)
{
	unsigned int sg_phyaddr, sg_length;
	unsigned int i, ret = 0;
	unsigned int data_size;
	unsigned int max_des, des_cnt;
	struct himci_des *des;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(data);

	host->data = data;

	if (data->flags & MMC_DATA_READ)
		host->dma_dir = DMA_FROM_DEVICE;
	else
		host->dma_dir = DMA_TO_DEVICE;

	host->dma_sg = data->sg;
	host->dma_sg_num = dma_map_sg(mmc_dev(host->mmc),
			data->sg, data->sg_len, host->dma_dir);
	himci_assert(host->dma_sg_num);
	himci_trace(2, "host->dma_sg_num is %d\n", host->dma_sg_num);
	data_size = data->blksz * data->blocks;

	if (data_size > (DMA_BUFFER * MAX_DMA_DES)) {
		himci_error("mci request data_size is too big!\n");
		ret = -1;
		goto out;
	}

	himci_trace(2, "host->dma_paddr is 0x%08X,host->dma_vaddr is 0x%08X\n",
			(unsigned int)host->dma_paddr,
			(unsigned int)host->dma_vaddr);

	max_des = (CMD_DES_PAGE_SIZE/sizeof(struct himci_des));
	des = (struct himci_des *)host->dma_vaddr;
	des_cnt = 0;

	for (i = 0; i < host->dma_sg_num; i++) {
		sg_length = sg_dma_len(&data->sg[i]);
		sg_phyaddr = sg_dma_address(&data->sg[i]);
		himci_trace(2, "sg[%d] sg_length is 0x%08X, " \
				"sg_phyaddr is 0x%08X\n", \
				i, (unsigned int)sg_length, \
				(unsigned int)sg_phyaddr);
		while (sg_length) {
			des[des_cnt].idmac_des_ctrl = DMA_DES_OWN
				| DMA_DES_NEXT_DES;
			des[des_cnt].idmac_des_buf_addr = sg_phyaddr;
			/* idmac_des_next_addr is paddr for dma */
			des[des_cnt].idmac_des_next_addr = host->dma_paddr
				+ (des_cnt + 1) * sizeof(struct himci_des);

			/* buffer size <= 4k */
			if (sg_length >= 0x1000) {
				des[des_cnt].idmac_des_buf_size = 0x1000;
				sg_length -= 0x1000;
				sg_phyaddr += 0x1000;
			} else {
				/* data alignment */
				des[des_cnt].idmac_des_buf_size = sg_length;
				sg_length = 0;
			}

			himci_trace(2, "des[%d] vaddr  is 0x%08X", i,
					(unsigned int)&des[i]);
			himci_trace(2, "des[%d].idmac_des_ctrl is 0x%08X",
			       i, (unsigned int)des[i].idmac_des_ctrl);
			himci_trace(2, "des[%d].idmac_des_buf_size is 0x%08X",
				i, (unsigned int)des[i].idmac_des_buf_size);
			himci_trace(2, "des[%d].idmac_des_buf_addr 0x%08X",
				i, (unsigned int)des[i].idmac_des_buf_addr);
			himci_trace(2, "des[%d].idmac_des_next_addr is 0x%08X",
				i, (unsigned int)des[i].idmac_des_next_addr);
			des_cnt++;
		}

		himci_assert(des_cnt < max_des);
	}
	des[0].idmac_des_ctrl |= DMA_DES_FIRST_DES;
	des[des_cnt - 1].idmac_des_ctrl |= DMA_DES_LAST_DES;
	des[des_cnt - 1].idmac_des_next_addr = 0;
out:
	return ret;
}

static int himci_exec_cmd(struct himci_host *host,
			   struct mmc_command *cmd, struct mmc_data *data)
{
	volatile union cmd_arg_u  cmd_regs;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(cmd);

	host->cmd = cmd;

	himci_writel(cmd->arg, host->base + MCI_CMDARG);
	himci_trace(4, "arg_reg 0x%x, val 0x%x", MCI_CMDARG, cmd->arg);
	cmd_regs.cmd_arg = himci_readl(host->base + MCI_CMD);
	if (data) {
		cmd_regs.bits.data_transfer_expected = 1;
		if (data->flags & (MMC_DATA_WRITE | MMC_DATA_READ))
			cmd_regs.bits.transfer_mode = 0;

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

	if (cmd == host->mrq->stop ||
			cmd->opcode == MMC_STOP_TRANSMISSION) {
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
	case MMC_RSP_R1 & (~MMC_RSP_CRC):
		cmd_regs.bits.response_expect = 1;
		cmd_regs.bits.response_length = 0;
		cmd_regs.bits.check_response_crc = 0;
		break;
	default:
		host->cmd->error = -EINVAL;
		himci_error("himci: unhandled response type %02x\n",
				mmc_resp_type(cmd));
		return -EINVAL;
	}

	himci_trace(3, "cmd->opcode = %d cmd->arg = 0x%X\n",
			cmd->opcode, cmd->arg);
	if (cmd->opcode == MMC_GO_IDLE_STATE)
		cmd_regs.bits.send_initialization = 1;
	else
		cmd_regs.bits.send_initialization = 0;
	/* CMD 11 check switch voltage */
	if (cmd->opcode == SD_SWITCH_VOLTAGE)
		cmd_regs.bits.volt_switch = 1;
	else
		cmd_regs.bits.volt_switch = 0;

	cmd_regs.bits.card_number = host->port;
	cmd_regs.bits.cmd_index = cmd->opcode;
	cmd_regs.bits.start_cmd = 1;
	cmd_regs.bits.update_clk_reg_only = 0;

	himci_writel(DATA_INT_MASK, host->base + MCI_RINTSTS);
	himci_writel(cmd_regs.cmd_arg, host->base + MCI_CMD);
	himci_trace(4, "cmd_reg 0x%x, val 0x%x\n", MCI_CMD, cmd_regs.cmd_arg);

	if (himci_wait_cmd(host) != 0) {
		himci_trace(3, "send card cmd is failed!");
		return -EINVAL;
	}
	return 0;
}

static void himci_finish_request(struct himci_host *host,
				  struct mmc_request *mrq)
{
	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(mrq);

	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;
	mmc_request_done(host->mmc, mrq);
}

static void himci_cmd_done(struct himci_host *host, unsigned int stat)
{
	unsigned int i;
	struct mmc_command *cmd = host->cmd;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(cmd);

	for (i = 0; i < 4; i++) {
		if (mmc_resp_type(cmd) == MMC_RSP_R2) {
			cmd->resp[i] = himci_readl(host->base +
					MCI_RESP3 - i * 0x4);
			/* R2 must delay some time here ,when use UHI card,
			   need check why */
			udelay(1000);
		} else
			cmd->resp[i] = himci_readl(host->base +
					MCI_RESP0 + i * 0x4);
	}

	if (stat & RTO_INT_STATUS) {
		cmd->error = -ETIMEDOUT;
		himci_trace(3, "irq cmd status stat = 0x%x is timeout error!",
				stat);
	} else if (stat & (RCRC_INT_STATUS | RE_INT_STATUS)) {
		cmd->error = -EILSEQ;
		himci_trace(3, "irq cmd status stat = 0x%x is response error!",
				stat);
	}

	host->cmd = NULL;
}

static void himci_data_done(struct himci_host *host, unsigned int stat)
{
	struct mmc_data *data = host->data;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(data);

	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, host->dma_dir);

	if (stat & (HTO_INT_STATUS | DRTO_INT_STATUS)) {
		data->error = -ETIMEDOUT;
		himci_trace(3, "irq data status stat = 0x%x is timeout error!",
			    stat);
	} else if (stat & (EBE_INT_STATUS | SBE_INT_STATUS |
			   FRUN_INT_STATUS | DCRC_INT_STATUS)) {
		data->error = -EILSEQ;
		himci_trace(3, "irq data status stat = 0x%x is data error!",
				stat);
	}

	if (!data->error)
		data->bytes_xfered = data->blocks * data->blksz;
	else
		data->bytes_xfered = 0;

	host->data = NULL;
}

static int himci_wait_cmd_complete(struct himci_host *host)
{
	unsigned int cmd_retry_count = 0;
	unsigned long cmd_jiffies_timeout;
	unsigned int cmd_irq_reg = 0;
	struct mmc_command *cmd = host->cmd;
	unsigned long flags;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(cmd);

	cmd_jiffies_timeout = jiffies + request_timeout;
	while (1) {

		do {
			spin_lock_irqsave(&host->lock, flags);
			cmd_irq_reg = readl(host->base + MCI_RINTSTS);

			if (cmd_irq_reg & CD_INT_STATUS) {
				himci_writel((CD_INT_STATUS | RTO_INT_STATUS
					| RCRC_INT_STATUS | RE_INT_STATUS),
					host->base + MCI_RINTSTS);
				spin_unlock_irqrestore(&host->lock, flags);
				himci_cmd_done(host, cmd_irq_reg);
				return 0;
			} else if (cmd_irq_reg & VOLT_SWITCH_INT_STATUS) {
				himci_writel(VOLT_SWITCH_INT_STATUS,
						host->base + MCI_RINTSTS);
				spin_unlock_irqrestore(&host->lock, flags);
				himci_cmd_done(host, cmd_irq_reg);
				return 0;
			}
			spin_unlock_irqrestore(&host->lock, flags);
			cmd_retry_count++;
		} while (cmd_retry_count < retry_count);

		cmd_retry_count = 0;

		if (host->card_status == CARD_UNPLUGED) {
			cmd->error = -ETIMEDOUT;
			return -1;
		}

		if (!time_before(jiffies, cmd_jiffies_timeout)) {
			unsigned int i = 0;
			for (i = 0; i < 4; i++) {
				cmd->resp[i] = himci_readl(host->base
						+ MCI_RESP0 + i * 0x4);
				pr_err("voltage switch read MCI_RESP");
				pr_err("%d : 0x%x\n", i, cmd->resp[i]);
			}
			cmd->error = -ETIMEDOUT;
			himci_trace(3, "wait cmd request complete is timeout!");
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
static int himci_wait_auto_stop_complete(struct himci_host *host)
{
	unsigned int cmd_retry_count = 0;
	unsigned long cmd_jiffies_timeout;
	unsigned int cmd_irq_reg = 0;
	unsigned long flags;

	himci_trace(2, "begin");
	himci_assert(host);

	cmd_jiffies_timeout = jiffies + request_timeout;
	while (1) {

		do {
			spin_lock_irqsave(&host->lock, flags);
			cmd_irq_reg = readl(host->base + MCI_RINTSTS);
			if (cmd_irq_reg & ACD_INT_STATUS) {
				himci_writel((ACD_INT_STATUS | RTO_INT_STATUS
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
			himci_trace(3, "wait auto stop complete is timeout!");
			return -1;
		}

		schedule();
	}

}
#endif

static int himci_wait_data_complete(struct himci_host *host)
{
	unsigned int tmp_reg;
	struct mmc_data *data = host->data;
	long time = request_timeout;
	unsigned long flags;

	himci_trace(2, "begin");
	himci_assert(host);
	himci_assert(data);

	time = wait_event_timeout(host->intr_wait,
				  test_bit(HIMCI_PEND_DTO_B,
					   &host->pending_events), time);

	/* Mask MMC host data intr */
	spin_lock_irqsave(&host->lock, flags);
	tmp_reg = himci_readl(host->base + MCI_INTMASK);
	tmp_reg &= ~DATA_INT_MASK;
	himci_writel(tmp_reg, host->base + MCI_INTMASK);
	host->pending_events &= ~HIMCI_PEND_DTO_M;
	spin_unlock_irqrestore(&host->lock, flags);

	if (((time <= 0)
		&& (!test_bit(HIMCI_PEND_DTO_B, &host->pending_events)))
		|| (host->card_status == CARD_UNPLUGED)) {

		data->error = -ETIMEDOUT;
		himci_trace(5, "wait data request complete is timeout! 0x%08X",
				host->irq_status);
		himci_idma_stop(host);
		himci_data_done(host, host->irq_status);
		return -1;
	}

	himci_idma_stop(host);
	himci_data_done(host, host->irq_status);
	return 0;
}

static int himci_wait_card_complete(struct himci_host *host,
		struct mmc_data *data)
{
	unsigned int card_retry_count = 0;
	unsigned long card_jiffies_timeout;
	unsigned int card_status_reg = 0;

	himci_trace(2, "begin");
	himci_assert(host);

	card_jiffies_timeout = jiffies + request_timeout;
	while (1) {
		do {
			card_status_reg = readl(host->base + MCI_STATUS);
			if (!(card_status_reg & DATA_BUSY)) {
				himci_trace(2, "end");
				return 0;
			}
			card_retry_count++;
		} while (card_retry_count < retry_count);

		card_retry_count = 0;

		if (host->card_status == CARD_UNPLUGED) {
			host->mrq->cmd->error = -ETIMEDOUT;
			himci_trace(3, "card is unpluged!");
			return -1;
		}

		if (!time_before(jiffies, card_jiffies_timeout)) {
			host->mrq->cmd->error = -ETIMEDOUT;
			himci_trace(3, "wait card ready complete is timeout!");
			return -1;
		}

		schedule();
	}
}

static void himci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct himci_host *host = mmc_priv(mmc);
	int byte_cnt = 0, fifo_count = 0, ret = 0, tmp_reg;
	unsigned long flags;

	himci_trace(2, "begin");
	himci_assert(mmc);
	himci_assert(mrq);
	himci_assert(host);

	host->mrq = mrq;
	host->irq_status = 0;

	if (host->card_status == CARD_UNPLUGED) {
		mrq->cmd->error = -ENODEV;
		goto request_end;
	}

	ret = himci_wait_card_complete(host, mrq->data);
	if (ret) {
		mrq->cmd->error = ret;
		goto request_end;
	}

	/* prepare data */
	if (mrq->data) {
		ret = himci_setup_data(host, mrq->data);
		if (ret) {
			mrq->data->error = ret;
			himci_trace(3, "data setup is error!");
			goto request_end;
		}

		byte_cnt = mrq->data->blksz * mrq->data->blocks;
		himci_writel(byte_cnt, host->base + MCI_BYTCNT);
		himci_writel(mrq->data->blksz, host->base + MCI_BLKSIZ);

		/* reset fifo */
		tmp_reg = himci_readl(host->base + MCI_CTRL);
		tmp_reg |= FIFO_RESET;
		himci_writel(tmp_reg, host->base + MCI_CTRL);

		do {
			tmp_reg = himci_readl(host->base + MCI_CTRL);
			fifo_count++;
			if (fifo_count >= retry_count) {
				pr_info("fifo reset is timeout!");
				return;
			}
		} while (tmp_reg & FIFO_RESET);

		/* start DMA */
		himci_idma_start(host);
	} else {
		himci_writel(0, host->base + MCI_BYTCNT);
		himci_writel(0, host->base + MCI_BLKSIZ);
	}

	/* send command */
	ret = himci_exec_cmd(host, mrq->cmd, mrq->data);
	if (ret) {
		mrq->cmd->error = ret;
		himci_idma_stop(host);
		himci_trace(3, "can't send card cmd! ret = %d", ret);
		goto request_end;
	}

	/* wait command send complete */
	himci_wait_cmd_complete(host);

	/* start data transfer */
	if (mrq->data) {
		if (!(mrq->cmd->error)) {
			/* Open MMC host data intr */
			spin_lock_irqsave(&host->lock, flags);
			tmp_reg = himci_readl(host->base + MCI_INTMASK);
			tmp_reg |= DATA_INT_MASK;
			himci_writel(tmp_reg, host->base + MCI_INTMASK);
			spin_unlock_irqrestore(&host->lock, flags);

			/* wait data transfer complete */
			himci_wait_data_complete(host);
		} else {
			/* CMD error in data command */
			himci_idma_stop(host);
		}

		if (mrq->stop) {
#ifdef CONFIG_SEND_AUTO_STOP
			int trans_cnt;

			trans_cnt = himci_readl(host->base + MCI_TCBCNT);
			/* send auto stop */
			if ((trans_cnt == byte_cnt) && (!(host->is_tuning))) {
				himci_trace(3, "byte_cnt = %d, trans_cnt = %d",
						byte_cnt, trans_cnt);
				ret = himci_wait_auto_stop_complete(host);
				if (ret) {
					mrq->stop->error = -ETIMEDOUT;
					goto request_end;
				}
			} else {
#endif
				/* send soft stop command */
				himci_trace(3, "this time, send soft stop");
				ret = himci_exec_cmd(host, host->mrq->stop,
						host->data);
				if (ret) {
					mrq->stop->error = ret;
					goto request_end;
				}
				ret = himci_wait_cmd_complete(host);
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
	himci_writel(ALL_SD_INT_CLR, host->base + MCI_RINTSTS);
	spin_unlock_irqrestore(&host->lock, flags);

	himci_finish_request(host, mrq);
}

static int himci_do_voltage_switch(struct himci_host *host,
						 struct mmc_ios *ios)
{
	u32 ctrl;

	/*
	 * We first check whether the request is to set signalling voltage
	 * to 3.3V. If so, we change the voltage to 3.3V and return quickly.
	 */
	ctrl = himci_readl(host->base + MCI_UHS_REG);
	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
		/* Set 1.8V Signal Enable in the MCI_UHS_REG to 1 */
		himci_trace(3, "switch voltage 330");
		ctrl &= ~(HI_SDXC_CTRL_VDD_180 << host->port);
		himci_writel(ctrl, host->base + MCI_UHS_REG);

		/* Wait for 5ms */
		usleep_range(5000, 5500);

		/* 3.3V regulator output should be stable within 5 ms */
		ctrl = himci_readl(host->base + MCI_UHS_REG);
		if (!(ctrl & (HI_SDXC_CTRL_VDD_180 << host->port ))) {
			/* config Pin drive capability */
			himci_set_drv_cap(host, 0);
			return 0;
		} else {
			himci_error(": Switching to 3.3V ");
			himci_error("signalling voltage failed\n");
			return -EIO;
		}
	} else if (!(ctrl & (HI_SDXC_CTRL_VDD_180 << host->port)) &&
		  (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180)) {
		/* Stop SDCLK */
		himci_trace(3, "switch voltage 180");
		himci_control_cclk(host, DISABLE);


		/*
		 * Enable 1.8V Signal Enable in the MCI_UHS_REG
		 */
		ctrl |= (HI_SDXC_CTRL_VDD_180 << host->port);
		himci_writel(ctrl, host->base + MCI_UHS_REG);

		/* Wait for 5ms */
		usleep_range(8000, 8500);

		ctrl = himci_readl(host->base + MCI_UHS_REG);
		if (ctrl & (HI_SDXC_CTRL_VDD_180 << host->port)) {
			/* Provide SDCLK again and wait for 1ms */
			himci_control_cclk(host, ENABLE);
			usleep_range(1000, 1500);

			if (host->mmc->caps2 & MMC_CAP2_HS200) {
				/* eMMC needn't to check the int status*/
				return 0;
			}
			/*
			 * If CMD11 return CMD down, then the card
			 * was successfully switched to 1.8V signaling.
			 */
			ctrl = himci_readl(host->base + MCI_RINTSTS);
			if ((ctrl & VOLT_SWITCH_INT_STATUS)
					&& (ctrl & CD_INT_STATUS)) {
				himci_writel(VOLT_SWITCH_INT_STATUS | CD_INT_STATUS,
						host->base + MCI_RINTSTS);
				/* config Pin drive capability */
				himci_set_drv_cap(host, 1);
				return 0;
			}
		}

		/*
		 * If we are here, that means the switch to 1.8V signaling
		 * failed. We power cycle the card, and retry initialization
		 * sequence by setting S18R to 0.
		 */

		ctrl &= ~(HI_SDXC_CTRL_VDD_180 << host->port);
		himci_writel(ctrl, host->base + MCI_UHS_REG);

		/* Wait for 5ms */
		usleep_range(5000, 5500);

		himci_ctrl_power(host, POWER_OFF, FORCE_DISABLE);
		/* Wait for 1ms as per the spec */
		usleep_range(1000, 1500);
		himci_ctrl_power(host, POWER_ON, FORCE_DISABLE);

		himci_control_cclk(host, DISABLE);

		/* Wait for 1ms as per the spec */
		usleep_range(1000, 1500);
		himci_control_cclk(host, ENABLE);

		himci_error(": Switching to 1.8V signalling ");
		himci_error("voltage failed, retrying with S18R set to 0\n");
		return -EAGAIN;
	} else
		/* No signal voltage switch required */
		return 0;
}

static int himci_start_signal_voltage_switch(struct mmc_host *mmc,
					      struct mmc_ios *ios)
{
	struct himci_host *host = mmc_priv(mmc);
	int err;

	err = himci_do_voltage_switch(host, ios);
	return err;
}

static int himci_send_stop(struct mmc_host *host)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_STOP_TRANSMISSION;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	err = mmc_wait_for_cmd(host, &cmd, 0);
	return err;
}

static void himci_set_sap_phase(struct himci_host *host, u32 phase)
{
	unsigned int reg_value;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);

	reg_value = himci_readl(host->base + MCI_UHS_REG_EXT);
	reg_value &= ~CLK_SMPL_PHS_MASK;
	reg_value |= (phase << CLK_SMPL_PHS_SHIFT);
	himci_writel(reg_value, host->base + MCI_UHS_REG_EXT);

	spin_unlock_irqrestore(&host->lock, flags);
}

/*
 * The procedure of tuning the phase shift of sampling clock
 *
 * 1.Set a phase shift of 0° on cclk_in_sample
 * 2.Send the Tuning command to the card
 * 3.increase the phase shift value of cclk_in_sample until the
 *   correct sampling point is received such that the host does not
 *   see any of the errors.
 * 4.Mark this phase shift value as the starting point of the sampling
 *   window.
 * 5.increase the phase shift value of cclk_in_sample until the host
 *   sees the errors starting to come again or the phase shift value
 *   reaches 360°.
 * 6.Mark the last successful phase shift value as the ending
 *   point of the sampling window.
 *
 *     A window is established where the tuning block is matched.
 * For example, for a scenario where the tuning block is received
 * correctly for a phase shift window of 90°and 180°, then an appropriate
 * sampling point is established as 135°. Once a sampling point is
 * established, no errors should be visible in the tuning block.
 *
 */
static int himci_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct himci_host *host;
	unsigned int index, count;
	unsigned int err = 0;
	unsigned int found = 0; /* identify if we have found a valid phase */
	unsigned int start_point;
	unsigned int end_point;
	unsigned int prev_err = NOT_FOUND;
	unsigned int raise_point = NOT_FOUND;
	unsigned int fall_point = NOT_FOUND;
	int phase, ret;


	start_point = TUNING_START_PHASE;
	end_point = TUNING_END_PHASE;

	host = mmc_priv(mmc);

	himci_writel(0x1, host->base + MCI_CARDTHRCTL);

	himci_trace(3, "start sd3.0 phase tuning...");
	host->is_tuning = 1;
	for (index = start_point; index <= end_point; index++) {
		/* set sample clk phase shift */
		himci_set_sap_phase(host, index);

		count = 0;
		do {
			ret = mmc_send_tuning(mmc, opcode, NULL);
			himci_send_stop(mmc); /* send soft_stop tail */

			if (ret) {
				himci_trace(3, "send tuning CMD%u fail! phase:%d err:%d\n",
						opcode, index, ret);
				err = 1;
				break;
			}
			count++;
		} while (count < 1);

		if (!err)
			found = 1;	/* found a valid phase */

		if (index > start_point) {
			if (err && !prev_err)
				fall_point = index - 1;

			if (!err && prev_err)
				raise_point = index;
		}

		if ((raise_point != NOT_FOUND) && (fall_point != NOT_FOUND))
			goto tuning_out;

		prev_err = err;
		err = 0;
	}

tuning_out:
	host->is_tuning = 0;
	if (!found) {
		himci_trace(5, "%s: no valid phase shift! use default",
				mmc_hostname(mmc));
		himci_writel(DEFAULT_PHASE, host->base + MCI_UHS_REG_EXT);
	} else {
		himci_trace(3, "Tuning finished!!");

		if (NOT_FOUND == raise_point)
			raise_point = start_point;
		if (NOT_FOUND == fall_point)
			fall_point = end_point;

		if (fall_point < raise_point) {
			phase = (raise_point + fall_point) / 2;
			phase = phase - (HIMCI_PHASE_SCALE / 2);
			phase = (phase < 0) ? (HIMCI_PHASE_SCALE + phase) : phase;
		} else
			phase = (raise_point + fall_point) / 2;

		himci_set_sap_phase(host, phase);

		pr_info("tuning %s: valid phase shift [%d, %d] Final Phase %d\n",
			mmc_hostname(mmc), raise_point, fall_point, phase);
	}

	himci_writel(RW_THRESHOLD_SIZE, host->base + MCI_CARDTHRCTL);

	return 0;
}

static void himci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct himci_host *host = mmc_priv(mmc);
	unsigned int tmp_reg;
	u32 ctrl;

	himci_trace(2, "begin");
	himci_assert(mmc);
	himci_assert(ios);
	himci_assert(host);

	himci_trace(3, "ios->power_mode = %d ", ios->power_mode);
	if (!ios->clock)
		himci_control_cclk(host, DISABLE);

	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		himci_ctrl_power(host, POWER_OFF, FORCE_DISABLE);
		break;
	case MMC_POWER_UP:
	case MMC_POWER_ON:
		himci_ctrl_power(host, POWER_ON, FORCE_DISABLE);
		break;
	}
	himci_trace(3, "ios->clock = %d ", ios->clock);
	if (ios->clock) {
		himci_control_cclk(host, DISABLE);
		himci_set_cclk(host, ios->clock);
		himci_control_cclk(host, ENABLE);

		/* speed mode check, if it is DDR50 set DDR mode */
		if (ios->timing == MMC_TIMING_UHS_DDR50) {
			ctrl = himci_readl(host->base + MCI_UHS_REG);
			if (!((HI_SDXC_CTRL_DDR_REG << host->port) & ctrl)) {
				ctrl |= (HI_SDXC_CTRL_DDR_REG << host->port);
				himci_writel(ctrl, host->base + MCI_UHS_REG);
			}
		}
	} else {
		himci_control_cclk(host, DISABLE);
		if (ios->timing != MMC_TIMING_UHS_DDR50) {
			ctrl = himci_readl(host->base + MCI_UHS_REG);
			if ((HI_SDXC_CTRL_DDR_REG << host->port) & ctrl) {
				ctrl &= ~(HI_SDXC_CTRL_DDR_REG << host->port);
				himci_writel(ctrl, host->base + MCI_UHS_REG);
			}
		}
	}

	/* set bus_width */
	himci_trace(3, "ios->bus_width = %d ", ios->bus_width);
	tmp_reg = himci_readl(host->base + MCI_CTYPE);
	tmp_reg &= ~((CARD_WIDTH_0 | CARD_WIDTH_1) << host->port);

	if (ios->bus_width == MMC_BUS_WIDTH_8) {
		tmp_reg |= (CARD_WIDTH_0 << host->port);
		himci_writel(tmp_reg, host->base + MCI_CTYPE);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
		tmp_reg |= (CARD_WIDTH_1 << host->port);
		himci_writel(tmp_reg, host->base + MCI_CTYPE);
	} else
		himci_writel(tmp_reg, host->base + MCI_CTYPE);
}

static void himci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct himci_host *host = mmc_priv(mmc);
	unsigned int reg_value;

	reg_value = himci_readl(host->base + MCI_INTMASK);
	if (enable)
		reg_value |= SDIO_INT_MASK;
	else
		reg_value &= ~SDIO_INT_MASK;
	himci_writel(reg_value, host->base + MCI_INTMASK);
}

static int himci_get_card_detect(struct mmc_host *mmc)
{
	unsigned ret;
	struct himci_host *host = mmc_priv(mmc);

	himci_trace(2, "begin");
	ret = himci_sys_card_detect(host);

	if (ret)
		return 0;
	else
		return 1;
}

static int himci_get_ro(struct mmc_host *mmc)
{
	unsigned ret;
	struct himci_host *host = mmc_priv(mmc);

	himci_trace(2, "begin");
	himci_assert(mmc);

	ret = himci_ctrl_card_readonly(host);

	return ret;
}

static void himci_hw_reset(struct mmc_host *mmc)
{
	unsigned int reg_value;
	struct himci_host *host = mmc_priv(mmc);
	unsigned int port = host->port;

	reg_value = himci_readl(host->base + MCI_RESET_N);
	reg_value &= ~(MMC_RST_N << port);
	himci_writel(reg_value, host->base + MCI_RESET_N);

	/* For eMMC, minimum is 1us but give it 10us for good measure */
	udelay(10);
	reg_value = himci_readl(host->base + MCI_RESET_N);
	reg_value |= (MMC_RST_N << port);
	himci_writel(reg_value, host->base + MCI_RESET_N);

	/* For eMMC, minimum is 200us but give it 300us for good measure */
	usleep_range(300, 1000);
}

static const struct mmc_host_ops himci_ops = {
	.request = himci_request,
	.set_ios = himci_set_ios,
	.get_ro = himci_get_ro,
	.start_signal_voltage_switch = himci_start_signal_voltage_switch,
	.execute_tuning	= himci_execute_tuning,
	.enable_sdio_irq = himci_enable_sdio_irq,
	.hw_reset = himci_hw_reset,
	.get_cd = himci_get_card_detect,
};

static irqreturn_t hisd_irq(int irq, void *dev_id)
{
	struct himci_host *host = dev_id;
	u32 state = 0;
	int handle = 0;
	u32 mstate = 0;

	spin_lock(&host->lock);
	state = himci_readl(host->base + MCI_RINTSTS);
	spin_unlock(&host->lock);

	/* bugfix: when send soft stop to SD Card, Host will report
	   sdio interrupt, This situation needs to be avoided */
	if (host->mmc->caps & MMC_CAP_SDIO_IRQ) {
		if ((host->mmc->card != NULL)
				&& (host->mmc->card->type == MMC_TYPE_SDIO)) {
			mstate = himci_readl(host->base + MCI_INTMASK);
			if ((state & SDIO_INT_STATUS) &&
					(mstate & SDIO_INT_MASK)) {
				spin_lock(&host->lock);
				himci_writel(SDIO_INT_STATUS,
						host->base + MCI_RINTSTS);
				spin_unlock(&host->lock);
				handle = 1;
				mmc_signal_sdio_irq(host->mmc);
			}
		}
	}

	if (state & DATA_INT_MASK) {
		handle = 1;
		host->pending_events |= HIMCI_PEND_DTO_M;

		spin_lock(&host->lock);
		host->irq_status = himci_readl(host->base + MCI_RINTSTS);
		himci_writel(DATA_INT_MASK , host->base + MCI_RINTSTS);
		spin_unlock(&host->lock);

		wake_up(&host->intr_wait);
	}

	if (handle)
		return IRQ_HANDLED;

	return IRQ_NONE;
}

static int himci_of_parse(struct device_node *np, struct mmc_host *mmc)
{
	struct himci_host *host = mmc_priv(mmc);
	int ret = mmc_of_parse(mmc);
	int len;

	if (ret)
		return ret;

	if (of_property_read_u32(np, "min-frequency", &mmc->f_min))
		mmc->f_min = MMC_CCLK_MIN;

	if (of_property_read_u32(np, "devid", &host->devid))
		return -EINVAL;

	if (of_find_property(np, "cap-mmc-hw-reset", &len))
		mmc->caps |= MMC_CAP_HW_RESET;

	return 0;
}

static int __init himci_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct himci_host *host = NULL;
	struct resource *host_ioaddr_res = NULL;
	int ret = 0, irq;
	struct device_node *np = pdev->dev.of_node;

	himci_trace(2, "begin");
	pr_info("mmc host probe\n");
	himci_assert(pdev);

	mmc = mmc_alloc_host(sizeof(struct himci_host), &pdev->dev);
	if (!mmc) {
		himci_error("no mem for hi mci host controller!\n");
		ret = -ENOMEM;
		goto out;
	}

	platform_set_drvdata(pdev, mmc);

	mmc->ops = &himci_ops;

	host_ioaddr_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (NULL == host_ioaddr_res) {
		himci_error("no ioaddr rescources config!\n");
		ret = -ENODEV;
		goto out;
	}

	if (himci_of_parse(np, mmc)) {
		himci_error("failed to parse mmc dts!\n");
		ret = -EINVAL;
		goto out;
	}

	/* reload by this controller */
	mmc->max_blk_count = 2048;
	mmc->max_segs = 1024;
	mmc->max_seg_size = mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_req_size = mmc->max_blk_size * mmc->max_blk_count;
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;

	host = mmc_priv(mmc);
	mci_host[slot_index++] = host;
	pdev->id = host->devid;
	host->pdev = pdev;
	host->mmc = mmc;
#ifdef CONFIG_ARCH_HI3518EV20X
	if (host->mmc->caps & MMC_CAP_HW_RESET)
		host->port = 1;
	else
		host->port = 0;
#else
	host->port = 0;
#endif
	host->dma_vaddr = dma_alloc_coherent(&pdev->dev, CMD_DES_PAGE_SIZE,
			&host->dma_paddr, GFP_KERNEL);
	if (!host->dma_vaddr) {
		himci_error("no mem for himci dma!\n");
		ret = -ENOMEM;
		goto out;
	}

	host->base = devm_ioremap_resource(&pdev->dev, host_ioaddr_res);
	if (IS_ERR_OR_NULL(host->base)) {
		himci_error("no mem for himci base!\n");
		ret = -ENOMEM;
		goto out;
	}

	spin_lock_init(&host->lock);

	host->crg_rst = devm_reset_control_get(&pdev->dev, "mmc_reset");
	if (IS_ERR_OR_NULL(host->crg_rst)) {
		himci_error("get rst fail.\n");
		ret = PTR_ERR(host->crg_rst);
		goto out;
	}

	reset_control_assert(host->crg_rst);
	usleep_range(50, 60);
	reset_control_deassert(host->crg_rst);

	host->clk = devm_clk_get(&pdev->dev, "mmc_clk");
	if (IS_ERR_OR_NULL(host->clk)) {
		himci_error("get clock fail.\n");
		ret = PTR_ERR(host->clk);
		goto out;
	}

	clk_prepare_enable(host->clk);

	host->power_status = POWER_OFF;

	/* enable card */
	himci_init_host(host);
	host->card_status = himci_sys_card_detect(host);

	init_timer(&host->timer);
	host->timer.function = himci_detect_card;
	host->timer.data = (unsigned long)host;
	host->timer.expires = jiffies + detect_time;
	add_timer(&host->timer);

	init_waitqueue_head(&host->intr_wait);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		pr_err("no IRQ defined!\n");
		goto out;
	}

	host->irq = irq;
	ret = request_irq(irq, hisd_irq, 0, DRIVER_NAME, host);
	if (ret) {
		pr_err("request_irq error!\n");
		goto out;
	}

	mmc_add_host(mmc);
	return 0;
out:
	if (host) {
		del_timer(&host->timer);

		if (host->base)
			devm_iounmap(&pdev->dev, host->base);

		if (host->dma_vaddr)
			dma_free_coherent(&pdev->dev, CMD_DES_PAGE_SIZE,
					host->dma_vaddr, host->dma_paddr);
	}
	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

static int __exit himci_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);

	himci_trace(2, "begin");
	himci_assert(pdev);

	platform_set_drvdata(pdev, NULL);

	if (mmc) {
		struct himci_host *host = mmc_priv(mmc);

		mmc_remove_host(mmc);
		free_irq(host->irq, host);
		del_timer_sync(&host->timer);
		himci_ctrl_power(host, POWER_OFF, FORCE_DISABLE);
		himci_control_cclk(host, DISABLE);
		devm_iounmap(&pdev->dev, host->base);
		dma_free_coherent(&pdev->dev, CMD_DES_PAGE_SIZE, host->dma_vaddr,
				  host->dma_paddr);
		mmc_free_host(mmc);
	}
	return 0;
}

static void himci_shutdown(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);

	himci_trace(3, "shutdown");
	if (mmc) {
		unsigned int val;
		struct himci_host *host = mmc_priv(mmc);

		/* bugfix: host reset can trigger error intr */
		himci_writel(0, host->base + MCI_IDINTEN);
		himci_writel(0, host->base + MCI_INTMASK);

		val = himci_readl(host->base + MCI_CTRL);
		val |= CTRL_RESET | FIFO_RESET | DMA_RESET;
		himci_writel(val, host->base + MCI_CTRL);
	}
}

#ifdef CONFIG_PM
static int himci_pltm_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct himci_host *host;
	int ret = 0;

	if (mmc) {
		host = mmc_priv(mmc);
		del_timer_sync(&host->timer);

		if (__clk_is_enabled(host->clk))
			clk_disable_unprepare(host->clk);
	}

	return ret;
}

static int himci_pltm_resume(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct himci_host *host;
	int ret = 0;

	if (mmc) {
		host = mmc_priv(mmc);

		if (!__clk_is_enabled(host->clk))
			clk_prepare_enable(host->clk);

		himci_init_host(host);

		add_timer(&host->timer);
	}

	return ret;
}
#else
#define himci_pltm_suspend    NULL
#define himci_pltm_resume     NULL
#endif

void himci_mmc_rescan(int slot)
{
	struct mmc_host *mmc;
	struct himci_host *host;

	host = mci_host[slot];
	if (!host || !host->mmc) {
		himci_trace(5, "mmc%d: invalid slot!\n", slot);
		return;
	}

	mmc = host->mmc;
	del_timer_sync(&host->timer);

	mmc_remove_host(mmc);

	mmc_add_host(mmc);

	add_timer(&host->timer);
}
EXPORT_SYMBOL(himci_mmc_rescan);

static const struct of_device_id
himci_match[] __maybe_unused = {
	{.compatible = "hisilicon,hi3516a-himci"},
	{.compatible = "hisilicon,hi3518ev20x-himci"},
	{},
};

static struct platform_driver himci_driver = {
	.probe = himci_probe,
	.remove = himci_remove,
	.shutdown = himci_shutdown,
	.suspend = himci_pltm_suspend,
	.resume = himci_pltm_resume,
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(himci_match),
		   },
};

static int __init himci_init(void)
{
	int ret;

	himci_trace(2, "begin");

	/*
	 * We should register SDIO1 first to make sure that
	 * the eMMC device,which connected to SDIO1 is mmcblk0.
	 */

	ret = platform_driver_register(&himci_driver);
	if (ret) {
		platform_driver_unregister(&himci_driver);
		himci_error("Himci driver register failed!");
		return ret;
	}

	/* device proc entry */
	ret = mci_proc_init(HIMCI_SLOT_NUM);
	if (ret)
		himci_error("device proc init is failed!");

	return ret;
}

static void __exit himci_exit(void)
{
	himci_trace(2, "begin");

	mci_proc_shutdown();

	platform_driver_unregister(&himci_driver);
}

module_init(himci_init);
module_exit(himci_exit);

#ifdef MODULE
MODULE_AUTHOR("Hisilicon Drive Group");
MODULE_DESCRIPTION("MMC/SD driver for the Hisilicon MMC/SD Host Controller");
MODULE_LICENSE("GPL");
#endif
