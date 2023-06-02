/*
 *  linux/drivers/mmc/host/plat-anyka/akmci.c - Anyka MMC/SD/SDIO driver
 *
 *  Copyright (C) 2010 Anyka, Ltd, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/log2.h>
#include <linux/mmc/host.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/scatterlist.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/dma-mapping.h>
#include <linux/notifier.h>

#include <asm/cacheflush.h>
#include <asm/div64.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <plat/l2.h>

#include <plat-anyka/adkey.h>
#include <mach/gpio.h>
#include <mach/clock.h>
#include <mach/reset.h>
#include <plat-anyka/akmci.h>

#define DRIVER_NAME 	"akmci"
//#define DRIVER_NAME 	"ak-mci"

#undef PDEBUG

#define ___hdbg___()	//printk("akmci:----func:%s---line:%d----\n", __func__, __LINE__);
#define DDREGS(host)		//dbg_dumpregs(host, __func__, __LINE__)
#define DDDATA(h, d, s)		//dbg_dumpdata(h, d, s)
#define HDBG(fmt, args...) 	//printk(fmt, ##args)

//#define MCI_DBG

#ifdef MCI_DBG
#ifdef __KERNEL__
#define PDEBUG(fmt, args...) 	printk(KERN_INFO "akmci:" fmt, ##args)
#else
#define PDEBUG(fmt, args...) 	fprintf(stderr, "%s %d:" fmt,__FILE__, __LINE__, ## args)
#endif
#else
#define PDEBUG(fmt, args...) 
#endif

//#define READ_ONLY_SD
#define DMA_SIZE	512

#define USED_DMA(x) (((x) >= DMA_SIZE) && (!((x) % DMA_SIZE)))
#define USED_DMA_CPU(x) (((x) > DMA_SIZE) && ((x) % DMA_SIZE))
#define USED_CPU(x) (((x) > 0) && ((x) < DMA_SIZE))

static struct kobject *gpio_pwr_obj = NULL;

#define CARD_UNPLUGED	1
#define CARD_PLUGED		0
#define MCI_DATA_IRQ	(1<<1)
#define MCI_CMD_IRQ		(1<<0)
static unsigned int tf_err_count;
static unsigned int detect_time = (HZ/2);
static unsigned int retry_count = 100;
static unsigned int request_timeout = (5 * HZ);

unsigned int slot_index = 0;
struct mmc_host *mci_host[MCI_SLOT_NUM] = {NULL};

static void akmci_l2xfer(struct akmci_host *host);
static void akmci_init_host_cfg(struct akmci_host *host);


static int validate_gpio_info (struct gpio_info *gpio) {

	/// 所有参数为零代表无效，通过该方法兼容旧版本 GPIO 配置选项。
	if(0 == gpio->pin
			&& 0 == gpio->pulldown
			&& 0 == gpio->pullup
			&& 0 == gpio->value
			&& 0 == gpio->dir
			&& 0 == gpio->int_pol
			&& 0 == gpio->pwr_dn){
			
        gpio->pin = -1;
        gpio->pulldown = -1;
        gpio->pullup = -1;
        gpio->value = 1;
        gpio->dir = 1;
        gpio->int_pol = -1;
        gpio->pwr_dn = 1;

        }
   return 1;
}

/**
 * @brief dump mci module register, use for debug.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of data transmitted, including data buf pointer, data len .
 * @return void.
*/
static inline void dbg_dumpregs(struct akmci_host *host,
		const char *prefix, int eflags)
{
	u32 clkcon, cmdarg, cmd, cmdrsp, rsp1, rsp2, rsp3, rsp4;
	u32 dtimer, datlen, datcon, datcnt, stat, imask, dmamode, cpumode;

	clkcon = readl(host->base + MCI_CLK_REG);
	cmdarg = readl(host->base + MCI_ARGUMENT_REG);
	cmd = readl(host->base + MCI_COMMAND_REG);
	cmdrsp = readl(host->base + MCI_RESPCMD_REG);
	
	rsp1 = readl(host->base + MCI_RESPONSE0_REG);
	rsp2 = readl(host->base + MCI_RESPONSE1_REG);
	rsp3 = readl(host->base + MCI_RESPONSE2_REG);
	rsp4 = readl(host->base + MCI_RESPONSE3_REG);

	dtimer = readl(host->base + MCI_DATATIMER_REG);
	datlen = readl(host->base + MCI_DATALENGTH_REG);
	datcon = readl(host->base + MCI_DATACTRL_REG);
	datcnt = readl(host->base + MCI_DATACNT_REG);
	
	stat = 0;	//readl(host->base + MCI_STATUS_REG);
	imask = readl(host->base + MCI_MASK_REG);
	dmamode = readl(host->base + MCI_DMACTRL_REG);
	cpumode = readl(host->base + MCI_FIFO_REG);

	PDEBUG("current prefix: %s (%d)\n", prefix, eflags);
	
	PDEBUG("clkcon:[%08x], cmdarg:[%08x], cmd:[%08x], cmdrsp:[%08x].\n",
		clkcon, cmdarg, cmd, cmdrsp);
	PDEBUG("rsp1:[%08x], rsp2:[%08x], rsp3:[%08x], rsp4:[%08x]\n",
		rsp1, rsp2, rsp3, rsp4);
	PDEBUG("dtimer:[%08x], datlen:[%08x], datcon:[%08x], datcnt:[%08x]\n",
		dtimer, datlen, datcon, datcnt);
	PDEBUG("stat:[%08x], imask:[%08x], dmamode:[%08x], cpumode:[%08x]\n",
		stat, imask, dmamode, cpumode);
}

/**
 * @brief dump mci module read/write data, use for debug.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of data transmitted, including data buf pointer, data len .
 * @return void.
*/
static inline void dbg_dumpdata(struct akmci_host *host,
		void *data, int size)
{
	int ii;
	int dsize = (size +3)/4;
	u32 *dptr = data;
	
	printk("xfer data (size:%d):", size);
	
	for(ii = 0; ii < dsize; ii++) {
		if((ii%10) == 0)
			printk("\n");
		
		printk("%08x ", *(dptr + ii));
	}
	printk("\n");
}

/**
 * the data transfer mode description.
*/
static char* xfer_mode_desc[] = {
		"unknown", 
		"l2dma",
		"l2pio",
		"inner pio",
	};

/**
 * the sd/mmc/sdio card detect mode description.
*/
static char* detect_mode_desc[] = {
		"plugin alway", 
		"GPIO detect",
		"AD detect",
};

/**
 * @brief initilize the module share pin.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of data transmitted, including data buf pointer, data len .
 * @return void.
*/
static void akmci_init_sharepin(struct akmci_host *host)
{
	if(host->mci_mode == MCI_MODE_MMC_SD) {
		ak_group_config(ePIN_AS_MCI);
	} else {
		ak_group_config(ePIN_AS_SDIO);
	}
}

static struct gpio_info mci0_one_line_idle[]=
{
	{
	.pin = AK_GPIO_31,
	.pulldown = -1,
	.pullup = AK_PULLUP_DISABLE,
	.value = AK_GPIO_DISABLE, 
	.dir = AK_GPIO_DIR_INPUT,
	.int_pol = -1,
	},
	{
	.pin = AK_GPIO_32,
	.pulldown = -1,
	.pullup = AK_PULLUP_DISABLE,
	.value = AK_GPIO_DISABLE, 
	.dir = AK_GPIO_DIR_INPUT,
	.int_pol = -1,
	},

	{
	.pin = AK_GPIO_33,
	.pulldown = -1,
	.pullup = AK_PULLUP_DISABLE,
	.value = AK_GPIO_DISABLE, 
	.dir = AK_GPIO_DIR_INPUT,
	.int_pol = -1,
	},
};

static void akmci_init_sharepin_idle(struct akmci_host *host)
{	
		/* set  sd pin to gpio, input, power up disable*/
		if (0 == host->pdev->id) {  //mci0
#if 0
			unsigned int val;
			/* set sd pin function to gpio */
			val = __raw_readl(AK_SHAREPIN_CON4);
			pr_info("0x080000DC=%x\n",val);
			val &= ~(0xf << 6);
			__raw_writel(val, AK_SHAREPIN_CON4);
			pr_info("set 0x080000DC=%x\n",val);

			/* set pull up disable */
			val = __raw_readl(AK_PPU_PPD4);
			pr_info("0x080000E0=%x\n",val);
			val |= (0x7 << 4);
			__raw_writel(val, AK_PPU_PPD4);
			pr_info("set 0x080000E0=%x\n",val);

#else
			int index;
			for(index = 0; index < sizeof(mci0_one_line_idle)/sizeof(struct gpio_info); index ++)
				ak_gpio_set(&mci0_one_line_idle[index]);

#endif		
		} else {				// mci1 
				/// TODO
		}
}


#define MCI_L2_ADDR(host)	\
	((host->mci_mode == MCI_MODE_MMC_SD) ? ADDR_MMC_SD : ADDR_SDIO)
		
/**
* akmci_xfer_mode - judgement the mci transfer mode.
* ret: 	AKMCI_XFER_L2DMA: use for l2 dma mode
* 		AKMCI_XFER_L2PIO: use for l2 fifo mode.
*		AKMCI_XFER_INNERPIO: use for inner fifo mode.
*/
static inline int akmci_xfer_mode(struct akmci_host *host)
{
	return host->xfer_mode;
}

static inline int enable_imask(struct akmci_host *host, u32 imask)
{
	u32 newmask;

	newmask = readl(host->base + MCI_MASK_REG);
	newmask |= imask;
	writel(newmask, host->base + MCI_MASK_REG);

	return newmask;
}

static inline int disable_imask(struct akmci_host *host, u32 imask)
{
	u32 newmask;

	newmask = readl(host->base + MCI_MASK_REG);
	newmask &= ~imask;
	writel(newmask, host->base + MCI_MASK_REG);

	return newmask;
}

static inline void clear_imask(struct akmci_host *host)
{
	u32 mask = readl(host->base + MCI_MASK_REG);

	/* preserve the SDIO IRQ mask state */
	mask &= MCI_SDIOINTMASK;
	writel(mask, host->base + MCI_MASK_REG);
}

/**
 * @brief reset mmc/sd module.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of data transmitted, 
 * including data buf pointer, data len .
 * @return void.
*/
static void akmci_reset(struct akmci_host *host)
{
	if(host->mci_mode == MCI_MODE_MMC_SD) {
		ak_soft_reset(AK_SRESET_MMCSD);	
	} else {
		ak_soft_reset(AK_SRESET_SDIO);
	}
}

/**
 * @brief  L2DMA ISR post-process
 * @author Luo YongChuang
 * @modify  2015-02-5
 * @return void
 */
void L2DMA_isr(unsigned long data)
{
	struct akmci_host *host = (struct akmci_host *)data;	
	if(host->sg_ptr==NULL){
		printk(KERN_ERR "##L2DMA_isr host->sg_ptr==null\n");		
		return;
	}
	host->sg_ptr = sg_next(host->sg_ptr); 
	host->sg_len --;
	host->sg_off = 0;
	if(!host->sg_len)
		return;
	PDEBUG("###L2DMA_isr host->sg_len: %u\n", host->sg_len);
	akmci_l2xfer(host);			
}

/**
 * @brief  L2DMA ISR post-process
 * @author Luo YongChuang
 * @modify  2015-02-5
 * @return void
 */
void L2DMA_res_isr(unsigned long data)
{
	struct akmci_host *host = (struct akmci_host *)data;	
	if(host->data==NULL){
		printk(KERN_ERR "##L2DMA_res_isr host->data==null\n");
		return;
	}
	akmci_l2xfer(host);			
}



/**
 * @brief transmitting data.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of data transmitted, 
 * including data buf pointer, data len .
 * @return void.
*/
static void akmci_l2xfer(struct akmci_host *host)
{
	u32 xferlen;
	u8 dir;
	u32 *tempbuf = NULL;
	dma_addr_t phyaddr = 0;
	unsigned int sg_num = 0;

	if (host->data->flags & MMC_DATA_WRITE) {		
		dir = MEM2BUF;
	} else {
		dir = BUF2MEM;
	}	

    xferlen = host->sg_ptr->length - host->sg_off;
	sg_num  = host->sg_len;

	if(xferlen==0){
		PDEBUG("##akmci_l2xfer %s xferlen==0\n", (host->data->flags & MMC_DATA_WRITE)?"write":"read");
		return;		
	}

	PDEBUG("###%s xferlen : %u, sg_num : %u\n",(host->data->flags & MMC_DATA_WRITE)?"write":"read",
		xferlen, host->sg_len);

	if ((akmci_xfer_mode(host) == AKMCI_XFER_L2DMA) &&
		USED_DMA(xferlen))		
	{
		phyaddr = sg_dma_address(host->sg_ptr) + host->sg_off;
		
		host->sg_off += xferlen;
		host->data_xfered += xferlen;

		if(sg_num>1){
			PDEBUG("##sg_num>1, set dam callback\n");
			l2_set_dma_callback(host->l2buf_id, L2DMA_isr, (unsigned long)(host));
			l2_combuf_dma(phyaddr, host->l2buf_id, xferlen, dir, AK_TRUE);   
		}else {
			PDEBUG("##sg_num==1\n");
			l2_combuf_dma(phyaddr, host->l2buf_id, xferlen, dir, AK_FALSE);	 
		}
	} 
	else if((akmci_xfer_mode(host) == AKMCI_XFER_L2DMA) && 
			USED_DMA_CPU(xferlen)) 
	{
		unsigned int need_callback=0;
		PDEBUG("##akmci transfer data: DMA AND CPU mode.\n");
		phyaddr = sg_dma_address(host->sg_ptr) + host->sg_off;

		//l2 limit
		//1. xferlen > 8192 
		//2.xferlen < 8192 && xferlen %64 != 0
		//3.xferlen  < 8192 && xferlen %64 ==0
		
		if(xferlen > 8192){
			xferlen = (xferlen / 512) * 512;
			need_callback = 1;
		}else if((xferlen < 8192) && (xferlen % 64)){
			xferlen = (xferlen / 64) * 64;
			need_callback = 1;
		}
		
		host->sg_off += xferlen;
   	 	host->data_xfered += xferlen;

		if(need_callback){
			l2_set_dma_callback(host->l2buf_id, L2DMA_res_isr, (unsigned long)(host));				  
			l2_combuf_dma(phyaddr, host->l2buf_id, xferlen, dir, AK_TRUE);	  
		}else{
			l2_combuf_dma(phyaddr, host->l2buf_id, xferlen, dir, AK_FALSE);	  
		}
	}
	else 
	{
		/*
		 * use for cpu transfer mode.
		 * */
		#if 0
		if((xferlen >= DMA_ONE_SHOT_LEN) && (xferlen % DMA_ONE_SHOT_LEN)){
			printk(KERN_ERR "xferlen > 64 bytes and not div by 64, transfer by cpu!\n");
		}
		#endif
		PDEBUG("##akmci transfer data: CPU mode.\n");
	    tempbuf = sg_virt(host->sg_ptr) + host->sg_off;
		
		host->sg_off += xferlen;
    	host->data_xfered += xferlen;
		l2_combuf_cpu((unsigned long)tempbuf, host->l2buf_id, xferlen, dir); 
	}

	/* debug info if data transfer error */
	if(host->data_err_flag > 0) {
		PDEBUG("mci_xfer transfered: xferptr = 0x%p, xfer_offset=%d, xfer_bytes=%d\n",
			sg_virt(host->sg_ptr)+host->sg_off, host->sg_off, host->data_xfered);
	}

}


void akmci_init_sg(struct akmci_host *host, struct mmc_data *data)
{
	/*
	 * Ideally, we want the higher levels to pass us a scatter list.
	 */
	host->sg_len = data->sg_len;
	host->sg_ptr = data->sg;
	host->sg_off = 0;
}

int akmci_next_sg(struct akmci_host *host)
{
	host->sg_ptr++;
	host->sg_off = 0;
	return --host->sg_len;
}

/**
 * @brief stop data, close interrupt.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host get the base address of resgister.
 * @return void.
 */
static void akmci_stop_data(struct akmci_host *host)
{
	writel(0, host->base + MCI_DMACTRL_REG);
	writel(0, host->base + MCI_DATACTRL_REG);
	
	disable_imask(host, MCI_DATAIRQMASKS|MCI_FIFOFULLMASK|MCI_FIFOEMPTYMASK);
	
	clear_bit(MCI_DATA_IRQ, &host->pending_events);
     
	if(akmci_xfer_mode(host) ==AKMCI_XFER_L2DMA) {
		if (host->data->flags & MMC_DATA_WRITE) {
			dma_sync_sg_for_cpu(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, DMA_TO_DEVICE);
			dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, DMA_TO_DEVICE);
		} else {
			dma_sync_sg_for_cpu(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, DMA_FROM_DEVICE);
			dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, DMA_FROM_DEVICE);
		}
	}

	host->sg_ptr = NULL;
	host->sg_len = 0;
	host->sg_off = 0;
	
	host->data = NULL; 
}

/**
 * @brief  finish a request,release resource.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of sd controller.
 * @param [in] *mrq information of request.
 * @return void.
 */
static void akmci_request_end(struct akmci_host *host, struct mmc_request *mrq)
{
	writel(0, host->base + MCI_COMMAND_REG); 
	
	//BUG_ON(host->data);
	host->mrq = NULL;
	host->cmd = NULL;

	if(host->data_err_flag > 0) {
		akmci_reset(host);

		writel(MCI_ENABLE|MCI_FAIL_TRIGGER, host->base + MCI_CLK_REG);
		writel(readl(host->base + MCI_CLK_REG)|host->clkreg, host->base + MCI_CLK_REG);
		mdelay(10);
	}	
	
	if(host->l2buf_id != BUF_NULL) {
		l2_free(MCI_L2_ADDR(host));
		host->l2buf_id = BUF_NULL;
	}
    
	if (mrq->data)
		mrq->data->bytes_xfered = host->data_xfered;

	
	/*
	 * Need to drop the host lock here; mmc_request_done may call
	 * back into the driver...
	 */

    mmc_request_done(host->mmc, mrq);
	PDEBUG("finalize the mci request.\n");

#ifdef CONFIG_CPU_FREQ
	 /*if request fail,then mmc_request_done send request again, ak_mci_send_request
	  *  not down freq_lock in interrupt,so not to unlock freq_lock.
	  */
	 if (not_retry) {					   
		 up(&host->freq_lock);
	 }	  
#endif

}

#define BOTH_DIR (MMC_DATA_WRITE | MMC_DATA_READ)

/**
 * @brief  config sd controller, start sending command.
 * 
 * @author Hanyang
 * @modify Lixinhai
 * @date 2011-05-10
 * @param [in] *host information of sd controller.
 * @param [in] *cmd information of cmd sended.
 * @return void.
 */
static void akmci_start_command(struct akmci_host *host, struct mmc_command *cmd)
{
	unsigned int ccon;
	
	PDEBUG("mci send cmd: op %i arg 0x%08x flags 0x%08x.%s data.\n", 
		cmd->opcode, cmd->arg, cmd->flags, cmd->data ? "contain":"no");

	writel(cmd->arg, host->base + MCI_ARGUMENT_REG);
	/* enable mci cmd irq */
	enable_imask(host, MCI_CMDIRQMASKS);


	ccon = MCI_CPSM_CMD(cmd->opcode) | MCI_CPSM_ENABLE;
	if (cmd->flags & MMC_RSP_PRESENT) {
		ccon |= MCI_CPSM_RESPONSE;
		if (cmd->flags & MMC_RSP_136)
			ccon |= MCI_CPSM_LONGRSP;
	}

	if (cmd->data)
		ccon |= MCI_CPSM_WITHDATA;

	host->cmd = cmd;

	ccon |= 1<<12;//512 clock command timeout
	/* configurate cmd controller register */
	writel(ccon, host->base + MCI_COMMAND_REG);
}


static void print_mci_data_err(struct mmc_data *data,
					unsigned int status, const char *err)
{
	if (data->flags & MMC_DATA_READ) {
		printk("akmci: data(read) status=0x%x data_blocks=%d data_blocksize=%d %s\n", status, data->blocks, data->blksz, err);
	} else if (data->flags & MMC_DATA_WRITE) {
		printk("akmci: data(write) status=0x%x data_blocks=%d data_blocksize=%d %s\n", status, data->blocks, data->blksz, err);
	}
}

/*
 * Handle completion of command and data transfers.
 */
static irqreturn_t akmci_irq(int irq, void *dev_id)
{
	struct akmci_host *host = dev_id;
	u32 stat_mask;
	u32 status;
	int ret = 1;

	spin_lock(&host->lock);
	status = readl(host->base + MCI_STATUS_REG);
	PDEBUG("###akmci_irq status : %x\n", status);

	if (status & MCI_SDIOINT) {
	    /*
		 * must disable sdio irq ,than read status to clear the sdio status,
         * else sdio irq will come again.
	    */
		mmc_signal_sdio_irq(host->mmc);
		//status |= readl(host->base + MCI_STATUS_REG);
		host->irq_status |= status;
	}

	stat_mask = MCI_RESPCRCFAIL|MCI_RESPTIMEOUT|MCI_CMDSENT|MCI_RESPEND;
	if ((status & stat_mask) && host->cmd){
		host->irq_status = status;
		set_bit(MCI_CMD_IRQ, &host->pending_events);
	}

	stat_mask = MCI_DATACRCFAIL|MCI_DATATIMEOUT|MCI_DATAEND;

	if ((status & stat_mask) && host->data){	
		host->irq_status = status;
		set_bit(MCI_DATA_IRQ, &host->pending_events);
		wake_up(&host->intr_data_wait);
	}
	
	ret = 1;
	spin_unlock(&host->lock);

	return IRQ_RETVAL(ret);

}

int get_card_status(struct akmci_host *host)
{
	unsigned int i, status[3], detect_retry_count = 0;
	while (1) {
		for (i = 0; i < 3; i++) {
			status[i] = ak_gpio_getpin(host->gpio_cd);
			udelay(10);
		}
		if ((status[0] == status[1]) && (status[0] == status[2]))
			break;

		detect_retry_count++;
		if (detect_retry_count >= 5) {
			pr_err("this is a dithering,card detect error!");
			return -1;
		}
	}	
	return status[0];
}

/**
 * @brief  detect sdio card's level type .
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] data  getting the information of sd host.
 * @return void.
 */
static void akmci_detect_change(unsigned long data)
{
	struct akmci_host *host = (struct akmci_host *)data;
	struct ak_mci_platform_data *const plat = host->pdev->dev.platform_data;
    struct gpio_info *const gpio_pwr = &plat->gpio_pwr;
    int curr_status = 0;
    
	curr_status = get_card_status(host);
	pr_debug("akmci_detect_change, curr_status=%d\n", curr_status);
	if(curr_status < 0)
		goto err;
	if (curr_status != host->card_status) {
		host->card_status = curr_status;
		if (curr_status != CARD_UNPLUGED) {
			//host->irq_cd_type = IRQ_TYPE_LEVEL_HIGH;
			printk(KERN_INFO "card connected!\n");
            //ak_gpio_setpin(host->power.gpio, !host->power.pwr_dn);
            
            ak_gpio_setpin(gpio_pwr->pin, !gpio_pwr->pwr_dn);
			akmci_reset(host);
			akmci_init_host_cfg(host);
			
		} else{
			//host->irq_cd_type = IRQ_TYPE_LEVEL_LOW;
			printk(KERN_INFO "card disconnected!\n");
            //ak_gpio_setpin(host->power.gpio, host->power.pwr_dn);
            
            ak_gpio_setpin(gpio_pwr->pin, gpio_pwr->pwr_dn);
			akmci_init_sharepin_idle(host);
		}
		
		if(host->mci_mode == MCI_MODE_MMC_SD)
			tf_err_count = 0;
		mmc_detect_change(host->mmc, msecs_to_jiffies(300));
	}
	
err:
	mod_timer(&host->detect_timer, jiffies + detect_time);

//	irq_set_irq_type(host->irq_cd, host->irq_cd_type);
//	enable_irq(host->irq_cd);

}


/**
 * @brief   detect the sdio card whether or not is in.
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *mmc information of host ,getting the sdio detect gpio.
 * @return int.
 * @retal 0:success.
 */
static int set_mci_plugin(struct notifier_block *nb,
	unsigned long val, void *data)
{
	struct akmci_host *host = container_of(nb, struct akmci_host, detect_nb);
	
	if(host->mci_mode == MCI_MODE_MMC_SD) {
		if (val == ADDETECT_MMC_PLUGIN)
			host->plugin_flag = 1;
		else if (val == ADDETECT_MMC_PLUGOUT)
			host->plugin_flag = 0;
	} else {
		if (val == ADDETECT_SDIO_PLUGIN)
			host->plugin_flag = 1;
		else if (val == ADDETECT_SDIO_PLUGOUT)
			host->plugin_flag = 0;
	}
	
	mmc_detect_change(host->mmc, 50);
	return 0;
}

static int akmci_enable(struct mmc_host *mmc)
{
	PDEBUG("akmci_enable:host is claimed.\n");	
	return 0;
}

static int akmci_disable(struct mmc_host *mmc)
{
	PDEBUG("akmci_disable:host is released.\n");
	return 0;
}

/**
 * @brief  check the card that status present or not.
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *mmc information of host ,getting the sdio detect gpio.
 * @return int.
 * @retal 1 sdio card is in ;0 sdio card is not in
 */
static int akmci_card_present(struct mmc_host *mmc)
{
	struct akmci_host *host = mmc_priv(mmc);

	if(host->detect_mode == AKMCI_DETECT_MODE_AD) 
	{
		return host->plugin_flag;
	} 
	else if(host->detect_mode == AKMCI_DETECT_MODE_GPIO) 
	{
		int ret = 1;

		if (host->gpio_cd >= 0) {
			ret &= (ak_gpio_getpin (host->gpio_cd) == 0);
		}

		if (host->power.gpio >= 0) {
			ret &= (host->power.value != 0); ///< 判断电源启用 value=1 表示启用，这里区别与电平高低。
		}

		return ret;
	} 
	else 
	{
		return 1; //plugin alway.
	}
}

static int akmci_setup_data(struct akmci_host *host, struct mmc_data *data)
{
	unsigned int datactrl, dmacon;
   	unsigned int		dma_sg_num;
	
	PDEBUG("%s: blksz %04x blks %04x flags %08x\n",
	       __func__, data->blksz, data->blocks, data->flags);
	BUG_ON((data->flags & BOTH_DIR) == BOTH_DIR);

	host->l2buf_id = l2_alloc(MCI_L2_ADDR(host));//alloc l2 buffer
	if (BUF_NULL == host->l2buf_id)	{
		printk("L2 buffer malloc fail!\n");
		return -1;
		//BUG();
	}else{
		PDEBUG("###L2 buffer id : %d\n", host->l2buf_id);
	}
	host->data = data;
	host->size = data->blksz * data->blocks;
	host->data_xfered = 0;

	PDEBUG("###dir : %s, data->blksz : %u, data->blocks : %u, host->size : %u\n",
			(data->flags&MMC_DATA_READ)? "read" : "write", data->blksz, data->blocks, host->size);

	if (host->size > 64 * 1024)
		pr_err("Err: %s %d akmci %s to long: %d.\n", __func__, __LINE__,
				(data->flags & MMC_DATA_WRITE) ? "write":"read", host->size);

	akmci_init_sg(host, data); 

	if(akmci_xfer_mode(host) == AKMCI_XFER_L2DMA) {
		/* set dma addr */
		if (data->flags & MMC_DATA_WRITE)
			dma_sg_num = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len, DMA_TO_DEVICE);
		else
			dma_sg_num = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len, DMA_FROM_DEVICE);
		//printk("###dma_map_sg dma_sg_num : %u, data->sg_len : %u\n", dma_sg_num, data->sg_len);
	}
	
	writel(TRANS_DATA_TIMEOUT, host->base + MCI_DATATIMER_REG);
	writel(host->size, host->base + MCI_DATALENGTH_REG);

	if(akmci_xfer_mode(host) != AKMCI_XFER_INNERPIO) {
		/*dma mode register*/
		dmacon = MCI_DMA_BUFEN | MCI_DMA_SIZE(MCI_L2FIFO_SIZE/4);

		if(akmci_xfer_mode(host) == AKMCI_XFER_L2DMA) {
			dmacon |= MCI_DMA_EN;
		}		
		writel(dmacon, host->base + MCI_DMACTRL_REG);	
	}

	/* 
	 * enable mci data irq 
	 * */
	enable_imask(host, MCI_DATAIRQMASKS);

	datactrl = MCI_DPSM_ENABLE;

	switch (host->bus_width) {
	case MMC_BUS_WIDTH_8:
		datactrl |= MCI_DPSM_BUSMODE(2);
		break;
	case MMC_BUS_WIDTH_4:
		datactrl |= MCI_DPSM_BUSMODE(1);
		break;
	case MMC_BUS_WIDTH_1:
	default:
		datactrl |= MCI_DPSM_BUSMODE(0);
		break;
	}

	if (data->flags & MMC_DATA_STREAM) {
		PDEBUG("STREAM Data\n");
		datactrl |= MCI_DPSM_STREAM;
	} else {
		PDEBUG("BLOCK Data: %u x %u\n", data->blksz, data->blocks);
		datactrl |= MCI_DPSM_BLOCKSIZE(data->blksz);
		datactrl &= ~MCI_DPSM_STREAM;
	}

	if (data->flags & MMC_DATA_READ) 
		datactrl |= MCI_DPSM_DIRECTION;
	else if (data->flags & MMC_DATA_WRITE) 
		datactrl &= ~MCI_DPSM_DIRECTION;

	/* configurate data controller register */
	writel(datactrl, host->base + MCI_DATACTRL_REG);
	
	PDEBUG("ENABLE DATA IRQ, datactrl: 0x%08x, timeout: 0x%08x, len: %u\n",
	       datactrl, readl(host->base + MCI_DATATIMER_REG), host->size);
	return 0;
	
}


static int akmci_wait_cmd_complete(struct akmci_host *host)
{
	int ret = 0;
	unsigned int status;
	struct mmc_command	*cmd = host->cmd;
	unsigned int cmd_retry_count = 0;
	unsigned long flags;
	unsigned long cmd_jiffies_timeout;

	cmd_jiffies_timeout = jiffies + request_timeout;
	while(1){
		if (!time_before(jiffies, cmd_jiffies_timeout)) {
			host->cmd->error = -ETIMEDOUT;
			printk(KERN_ERR "##wait cmd request complete is timeout!");
			return -1;
		}
		
		do {
			spin_lock_irqsave(&host->lock, flags);
			if (test_bit(MCI_CMD_IRQ, &host->pending_events)) {
				status = host->irq_status;	
				spin_unlock_irqrestore(&host->lock, flags);
				
				cmd->resp[0] = readl(host->base + MCI_RESPONSE0_REG);
				cmd->resp[1] = readl(host->base + MCI_RESPONSE1_REG);
				cmd->resp[2] = readl(host->base + MCI_RESPONSE2_REG);
				cmd->resp[3] = readl(host->base + MCI_RESPONSE3_REG);
				
				PDEBUG("resp[0]=0x%x, [1]=0x%x, resp[2]=0x%x, [3]=0x%x\n",
					cmd->resp[0],cmd->resp[1],cmd->resp[2],cmd->resp[3]);
				
				if (status & MCI_RESPTIMEOUT) {
					host->cmd->error = -ETIMEDOUT;
					PDEBUG("CMD: send timeout\n");
					ret = -1;
				} else if ((status & MCI_RESPCRCFAIL) && (host->cmd->flags & MMC_RSP_CRC)) {
					host->cmd->error = -EILSEQ;
					printk("CMD: illegal byte sequence\n");
					ret= -1;
				}
				/* disable mci cmd irq */
				disable_imask(host, MCI_CMDIRQMASKS);
				clear_bit(MCI_CMD_IRQ, &host->pending_events);
				
				host->cmd = NULL;
				
				PDEBUG("end akmci_wait_cmd_complete ret=%d\n", ret);
								
				return ret;
			}
			spin_unlock_irqrestore(&host->lock, flags);
			cmd_retry_count++;
		} while (cmd_retry_count < retry_count);
		schedule();
	}

}



static int akmci_wait_data_complete(struct akmci_host *host)
{
	unsigned int status;
	struct mmc_data *data = host->data;
	long time = request_timeout;

	time = wait_event_timeout(host->intr_data_wait, test_bit(MCI_DATA_IRQ, &host->pending_events), time);
	/* disable mci data irq */
	disable_imask(host, MCI_DATAIRQMASKS|MCI_FIFOFULLMASK|MCI_FIFOEMPTYMASK);

	if(time <= 0){
		status = MCI_DATATIMEOUT;
		printk(KERN_ERR "##wait data %s complete is timeout host->size: %u, host->data_xfered: %u!\n", 
				(host->data->flags & MMC_DATA_WRITE)?"write":"read", host->size, host->data_xfered);
	}else{
		status = host->irq_status;
	}
	
	if (status & MCI_DATAEND){
		if((akmci_xfer_mode(host) == AKMCI_XFER_L2DMA) && 
			(AK_FALSE == l2_combuf_wait_dma_finish(host->l2buf_id))){
				clear_bit(MCI_DATA_IRQ, &host->pending_events);
			return 0;
		}
		if (data->flags & MMC_DATA_WRITE)
			l2_clr_status(host->l2buf_id);

		if((host->size != host->data_xfered)&&
			(USED_DMA(host->size) || USED_DMA_CPU(host->size))){
			printk(KERN_ERR "## %s host->size : %u , host->data_xfered : %u wait...\n",
				(host->data->flags & MMC_DATA_WRITE)?"write":"read", host->size, host->data_xfered);
			while(host->size != host->data_xfered){
				schedule();
			}
		}
		
	}else if(status &(MCI_DATACRCFAIL|MCI_DATATIMEOUT)){
		if (status & MCI_DATACRCFAIL) {
			data->error = -EILSEQ;
			if(host->mci_mode==MCI_MODE_MMC_SD)
				tf_err_count++;
			print_mci_data_err(data, status, "illeage byte sequence");
		} else if (status & MCI_DATATIMEOUT) {
			data->error = -ETIMEDOUT;
			if(host->mci_mode==MCI_MODE_MMC_SD)
				tf_err_count++;
			print_mci_data_err(data, status, "transfer timeout");
		}
		
		host->data_err_flag = 1;
		clear_bit(MCI_DATA_IRQ, &host->pending_events);
		return -1;
	}else{
		print_mci_data_err(data, status, "transfer data err!");
	}

	clear_bit(MCI_DATA_IRQ, &host->pending_events);
	return 0;
}

int akmci_get_tf_err_count(void)
{
	return tf_err_count;
}
EXPORT_SYMBOL_GPL(akmci_get_tf_err_count);

void akmci_set_tf_err_count(int count)
{
	tf_err_count = count;
}
EXPORT_SYMBOL_GPL(akmci_set_tf_err_count);

static void akmci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct akmci_host *host = mmc_priv(mmc);
	int ret;
	host->mrq = mrq;
	host->data_err_flag = 0;
	
	PDEBUG("start the mci request.\n");

	if(host->card_status == CARD_UNPLUGED){
		printk("%s: no medium present.\n", __func__);
		mrq->cmd->error = -ENOMEDIUM;
		goto  akmci_req_end;
	}

	if(mrq->data){
		ret = akmci_setup_data(host, mrq->data);
		if (ret) {
			mrq->data->error = ret;
			printk("data setup is error!");
			goto akmci_req_end;
		}

		if ((mrq->data->flags & MMC_DATA_READ) && 
				(USED_DMA(host->size) || USED_DMA_CPU(host->size))){
			akmci_l2xfer(host);
		}	
	}
			
	akmci_start_command(host, mrq->cmd);
	ret = akmci_wait_cmd_complete(host);
	if (ret) {
		if(mrq->data)
			akmci_stop_data(host);
		goto akmci_req_end;
	}
	
	if (!(mrq->data && !mrq->cmd->error)){
		goto akmci_req_end;
	}
	
	if(mrq->data){
		if(mrq->data->flags & MMC_DATA_READ){
			PDEBUG("###MMC_DATA_READ\n");
			ret = akmci_wait_data_complete(host);
			if(ret)
				goto akmci_data_err;
			if(USED_CPU(host->size)){ //cpu mode
				PDEBUG("###want cpu mode, host->size : %d \n", host->size);
				akmci_l2xfer(host);
			}
		}
		else if(mrq->data->flags & MMC_DATA_WRITE){
			PDEBUG("###MMC_DATA_WRITE\n");
			akmci_l2xfer(host);
			ret = akmci_wait_data_complete(host);
			if(ret)
				goto akmci_data_err;
		}
		
		if (mrq->stop){
			PDEBUG("###write stop cmd\n");
			akmci_start_command(host, mrq->stop);
			ret = akmci_wait_cmd_complete(host);
			if (ret) {
				goto akmci_data_err;
			}
		}
	}

akmci_data_err:
	akmci_stop_data(host);

akmci_req_end:
	akmci_request_end(host, mrq);
}

/**
 * @brief setting the mmc module working clock.
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *mmc information of host ,getting the sdio detect gpio.
 * @return int.
 * @retal 0:success. otherwise :err.
 */
static void akmci_set_clk(struct akmci_host *host, struct mmc_ios *ios)
{
	u32 clk, div;
	u32 clk_div_h, clk_div_l;

	if (ios->clock == 0) {
		clk = readl(host->base + MCI_CLK_REG);
		clk &= ~MCI_CLK_ENABLE;
		writel(clk, host->base + MCI_CLK_REG);	
		
		host->bus_clock = 0;
	} else {
		clk = readl(host->base + MCI_CLK_REG);
		clk |= MCI_CLK_ENABLE;//|MCI_CLK_PWRSAVE;
		clk &= ~0xffff; /* clear clk div */
		
		div = host->asic_clk/ios->clock;

        if (host->asic_clk % ios->clock)
            div += 1;
		
        div -= 2;
        clk_div_h = div/2;
        clk_div_l = div - clk_div_h;
		
		clk |= MMC_CLK_DIVL(clk_div_l) | MMC_CLK_DIVH(clk_div_h);
		writel(clk, host->base + MCI_CLK_REG);	

		host->bus_clock = host->asic_clk / ((clk_div_h+1)*(clk_div_l + 1));	
		
		PDEBUG("mmc clock is %lu Mhz. asic_clock is %ld MHz(div:l=%d, h=%d).\n",
			ios->clock/MHz, host->asic_clk/MHz, clk_div_l, clk_div_h); 
	}
	
	host->clkreg = clk;
}


static void akmci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct akmci_host *host = mmc_priv(mmc);

	switch(ios->power_mode) {
		case MMC_POWER_ON:
			PDEBUG("mci power on.\n");
			break;
		case MMC_POWER_UP:
			PDEBUG("mci power up.\n");
			break;
		case MMC_POWER_OFF:
			PDEBUG("mci power off.\n");
			break;
		default:
			break;
	}

	host->bus_mode = ios->bus_mode;
	host->bus_width = ios->bus_width;

	if(ios->clock != host->bus_clock) {
		akmci_set_clk(host, ios);
	}
}

/**
 * @brief   detect the sdio card writing protection.
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *mmc information of host ,getting the sdio detect gpio.
 * @return int.
 * @retal 1 sdio card writing protected ;0 sdio card writing is not protected
 */
static int akmci_get_ro(struct mmc_host *mmc)
{
#ifdef READ_ONLY_SD   //change lyc del wp
	struct akmci_host *host = mmc_priv(mmc);

	if (host->gpio_wp == -ENOSYS)
		return -ENOSYS;
	
	return (ak_gpio_getpin(host->gpio_wp) == 0);
#else
		return -ENOSYS;
#endif	
}

/**
 * @brief  enable or disable sdio interrupt, mmc host not use..
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *mmc information of sd controller.
 * @param [in] enable  1: enable; 0: disable.
 * @return void.
 */

static void akmci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	unsigned reg1,reg2;
	unsigned long flags;	
	struct akmci_host *host = mmc_priv(mmc);
//sdmmc���������ߵ������ˣ�bug_on����ȥ������sdmmc��bug_on
//	BUG_ON(host->mci_mode == MCI_MODE_SDIO);
	
	PDEBUG("%s the sdio interrupt.\n", enable ? "enable" : "disable");
	spin_lock_irqsave(&host->lock, flags); 
	
	reg1 = readl(host->base + MCI_MASK_REG);
	reg2 = readl(host->base + SDIO_INTRCTR_REG);
	
	if (enable) {
		reg1 |= SDIO_INTR_ENABLE; 
		reg2 |= SDIO_INTR_CTR_ENABLE;
	} else {
		reg1 &= ~SDIO_INTR_ENABLE;
		reg2 &= ~SDIO_INTR_CTR_ENABLE;
	}
	
	writel(reg2, host->base + SDIO_INTRCTR_REG);	
	writel(reg1, host->base + MCI_MASK_REG);
	spin_unlock_irqrestore(&host->lock, flags); 
}

int ak_sdio_rescan(int slot)
{
    struct mmc_host *mmc = mci_host[slot];

    if (!mmc) {
        pr_err("invalid mmc, please check the argument\n");
        return -EINVAL;
    }   

    mmc_detect_change(mmc, 0); 
    return 0;
}
EXPORT_SYMBOL_GPL(ak_sdio_rescan);

/**
 * register the function of sd/sdio driver.
 * 
 */
static struct mmc_host_ops akmci_ops = {
	.enable  = akmci_enable,
	.disable = akmci_disable,
	.request = akmci_request,
	.set_ios = akmci_set_ios,
	.get_ro  = akmci_get_ro,
	.get_cd  = akmci_card_present,
	.enable_sdio_irq = akmci_enable_sdio_irq,
};

/**
 * @brief  initilize the mmc host.
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *mmc information of sd controller.
 * @return 0:success..
 */
static int akmci_init_mmc_host(struct akmci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct ak_mci_platform_data *plat = host->plat;
	mmc->ops = &akmci_ops;
	
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;

	if(plat->bus_width == MCI_BUS_WIDTH_4){
		if(host->pdev->id==0){
			printk(KERN_ERR "can not set mci0 to 4 line, because share with spi flash! force to 1 line\n");
		}else {
			printk("%s use 4 line mode!\n",(host->mci_mode==MCI_MODE_SDIO)?"sdio":"mci");
			mmc->caps = MMC_CAP_4_BIT_DATA;
		}
	}
/*��mach�ļ�sdio�豸�����MCI_MODE_MMC_SD����sdio�жϣ������жϷ�ʽ����host��sdio�ж�*/
	if(host->mci_mode == MCI_MODE_SDIO){
		mmc->caps |= MMC_CAP_SDIO_IRQ;
	}
	if(plat->cap_highspeed)
		mmc->caps |= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED;

//	mmc->caps |= MMC_CAP_NEEDS_POLL;
	mmc->f_min = host->asic_clk / (255+1 + 255+1);
	mmc->f_max = host->asic_clk / (0+1 + 0+1);
	mmc->f_max = (mmc->f_max < plat->max_speed_hz) ? 
			mmc->f_max : plat->max_speed_hz;

	//support mmc erase
	mmc->caps |= MMC_CAP_ERASE;

#ifdef CONFIG_MMC_BLOCK_BOUNCE
	/* use block bounce buffer. */
	mmc->max_segs = 1;
#else
	/* We can do SGIO 128 */
	mmc->max_segs = MAX_MCI_REQ_SIZE/MAX_MCI_BLOCK_SIZE;
#endif

	/*
	 * Since we only have a 16-bit data length register, we must
	 * ensure that we don't exceed 2^16-1 bytes in a single request.
	 */ 
	//65536
	mmc->max_req_size = MAX_MCI_REQ_SIZE;

	/*
	 * Set the maximum segment size.  Since we aren't doing DMA
	 * (yet) we are only limited by the data length register.
	 */
	 //65536
	mmc->max_seg_size = mmc->max_req_size;

	mmc->max_blk_size = MAX_MCI_BLOCK_SIZE; //512

	/*No limit on the number of blocks transferred.*/
	//128
	mmc->max_blk_count = mmc->max_req_size / MAX_MCI_BLOCK_SIZE;
	return 0;
}

static void akmci_init_host_cfg(struct akmci_host *host)
{
	akmci_init_sharepin(host);
	/*enable the mci clock*/
	writel(MCI_ENABLE|MCI_FAIL_TRIGGER, host->base + MCI_CLK_REG);
	
	clear_imask(host);
}


#if defined(CONFIG_CPU_FREQ)

static int akmci_cpufreq_transition(struct notifier_block *nb,
				     unsigned long val, void *data)
{
	struct akmci_host *host;
	struct mmc_host *mmc;
	unsigned long newclk;
	unsigned long flags;
	struct cpufreq_freqs *freqs = (struct cpufreq_freqs *)data;
	host = container_of(nb, struct ak_mci_host, freq_transition);
	
	PDEBUG("%s(): in_interrupt()=%ld\n", __func__, in_interrupt());
	PDEBUG("ak_get_asic_clk = %ld\n",ak_get_asic_clk());	
	PDEBUG("freqs->new_cpufreq.asic_clk = %d\n",
		  freqs->new_cpufreq.asic_clk);	
		  
	mmc = host->mmc;
	newclk = freqs->new_cpufreq.asic_clk;
	if ((val == CPUFREQ_PRECHANGE && newclk > host->asic_clock) 
		|| (val == CPUFREQ_POSTCHANGE && newclk < host->asic_clock)) 
	{

		if (mmc->ios.power_mode != MMC_POWER_OFF &&
			mmc->ios.clock != 0)
		{		
			PDEBUG("%s(): preempt_count()=%d\n", __func__, preempt_count());
				
			down(&host->freq_lock);
			
			spin_lock_irqsave(&mmc->lock, flags);
		
			host->asic_clock = newclk;
			PDEBUG("MCI_CLK_REG1 = %d\n",readl(host->base + MCI_CLK_REG));	
			aksdio_set_clk(host, &mmc->ios);
			PDEBUG("MCI_CLK_REG2 = %d\n",readl(host->base + MCI_CLK_REG));	
			
			spin_unlock_irqrestore(&mmc->lock, flags);

			up(&host->freq_lock);
		}
	}

	return NOTIFY_DONE;
}


static inline int akmci_cpufreq_register(struct akmci_host *host)
{
	// use for requst and cpufreq
	sema_init(&host->freq_lock, 1);
	
	host->freq_transition.notifier_call = akmci_cpufreq_transition;

	return cpufreq_register_notifier(&host->freq_transition,
					 CPUFREQ_TRANSITION_NOTIFIER);
}

static inline void akmci_cpufreq_deregister(struct akmci_host *host)
{
	cpufreq_unregister_notifier(&host->freq_transition,
				    CPUFREQ_TRANSITION_NOTIFIER);
}

#else
static inline int akmci_cpufreq_register(struct akmci_host *host)
{
	return 0;
}

static inline void akmci_cpufreq_deregister(struct akmci_host *host)
{
}
#endif


static ssize_t mmc_power_gpio_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct power_gpio_attribute *pga = container_of(attr, struct power_gpio_attribute, k_attr);
	pr_debug("###mmc_power_gpio_read, value=%x\n",pga->value);
	return sprintf(buf, "%d\n",pga->value);
}

static ssize_t mmc_power_gpio_write(struct kobject *kobj, struct kobj_attribute *attr,
		 const char *buf, size_t count)
{
	unsigned int gpio_value = 0;
	struct power_gpio_attribute *pga = container_of(attr, struct power_gpio_attribute, k_attr);
	struct akmci_host *host = container_of(pga, struct akmci_host, power);
	struct ak_mci_platform_data *const plat = host->pdev->dev.platform_data;
	struct gpio_info *const gpio_pwr = &plat->gpio_pwr;

	
	sscanf(buf, "%d", &gpio_value);
	pga->value = gpio_value;
	pr_info ("## MMC Power: GPIO%02d -> %d, host->card_status=%d(1=CARD_UNPLUGED)\n", pga->gpio, gpio_value,host->card_status);

	if(host->card_status == CARD_PLUGED)
	{
		if (0 == gpio_value) 
		{
			/// Power OFF.
			pr_info("mmc_power_gpio_write, power off\n");			
			ak_gpio_setpin(pga->gpio, gpio_pwr->pwr_dn);
			akmci_init_sharepin_idle(host);
			
		} else {
			/// Power On.
			pr_info("mmc_power_gpio_write, power on\n");	
			ak_gpio_setpin(pga->gpio, !gpio_pwr->pwr_dn);
			akmci_init_host_cfg(host);

		}	

	}
	mmc_detect_change(host->mmc, msecs_to_jiffies(500));

	return count;
}



/**
 * @brief   sdio driver probe and init.
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @param [in] *pdev information of platform device ,getting the sd driver resource .
 * @return int.
 * @retval -EINVAL no platform data , fail;
 * @retval -EBUSY  requset mem  fail;
 * @retval -ENOMEM  alloc mem fail;
 */
static int __devinit akmci_probe(struct platform_device *pdev)
{
	struct akmci_host *host;
	struct mmc_host *mmc;
	int ret;
	struct ak_mci_platform_data *const plat = pdev->dev.platform_data;
	struct gpio_info *gpio_pwr = NULL;

	if(!plat) {
		printk("not found mci platform data.");
		ret = -EINVAL;
		goto probe_out;
	}
	printk("akmci_probe : %s\n", plat->mci_mode?"SDIO":"MCI");
	gpio_pwr = &plat->gpio_pwr;

	mmc = mmc_alloc_host(sizeof(struct akmci_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}

	mmc->pm_caps |= MMC_PM_KEEP_POWER;
	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->pdev = pdev;

	spin_lock_init(&host->lock);

	host->plat = plat;
	host->mci_mode = plat->mci_mode;
	host->data_err_flag = 0;
	host->l2buf_id = BUF_NULL;

	host->gpio_wp = -ENOSYS;
	host->gpio_cd = -ENOSYS;
	host->detect_mode = plat->detect_mode;
	host->xfer_mode   = plat->xfer_mode;
	host->power.gpio = -1; ///< 缺省电源控制无效。

	akmci_reset(host);

	host->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!host->mem) {
		ret = -ENOENT;
		goto probe_free_host;
	}
	
	host->mem = request_mem_region(host->mem->start,
				       resource_size(host->mem), pdev->name);

	if (!host->mem) {
		dev_err(&pdev->dev, "failed to request io memory region.\n");
		ret = -ENOENT;
		goto probe_free_host;
	}

	host->base = ioremap(host->mem->start, resource_size(host->mem));
	if (!host->base) {
		dev_err(&pdev->dev, "failed to ioremap() io memory region.\n");
		ret = -EINVAL;
		goto probe_free_mem_region;
	}

	host->irq_mci = platform_get_irq(pdev, 0);
	if(host->irq_mci == 0) {
		dev_err(&pdev->dev, "failed to get interrupt resouce.\n");
		ret = -EINVAL;
		goto probe_iounmap;
	}
	
	init_waitqueue_head(&host->intr_data_wait);

	if (request_irq(host->irq_mci, akmci_irq, IRQF_DISABLED, pdev->name, host)) {
		dev_err(&pdev->dev, "failed to request mci interrupt.\n");
		ret = -ENOENT;
		goto probe_iounmap;
	}

	/* We get spurious interrupts even when we have set the IMSK
	 * register to ignore everything, so use disable_irq() to make
	 * ensure we don't lock the system with un-serviceable requests. */
	//disable_irq(host->irq_mci);

	host->clk = clk_get(&pdev->dev, (host->mci_mode == MCI_MODE_MMC_SD)? "mci" : "sdio");
	if (IS_ERR(host->clk)) {
		dev_err(&pdev->dev, "failed to find clock source.\n");
		ret = PTR_ERR(host->clk);
		host->clk = NULL;
		goto probe_free_irq;
	}

	ret = clk_enable(host->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock source.\n");
		goto clk_free;
	}

	host->asic_clk = clk_get_rate(host->clk);

	ret = akmci_init_mmc_host(host);
	if(ret) {
		dev_err(&pdev->dev, "failed to init mmc host.\n");
		goto clk_disable;
	}

	/*init mmc/sd host.*/
	akmci_init_host_cfg(host);

	/*use for gpio detect.*/
	if(host->detect_mode == AKMCI_DETECT_MODE_GPIO) {
		if(plat->gpio_cd.pin >= 0) {
	    	host->gpio_cd = plat->gpio_cd.pin;
			plat->gpio_init(&plat->gpio_cd);
		
			host->card_status = get_card_status(host);
			init_timer(&host->detect_timer);		
			host->detect_timer.function = akmci_detect_change;
			host->detect_timer.data = (unsigned long)host;
			host->detect_timer.expires = jiffies + detect_time;
			add_timer(&host->detect_timer);
		}
	}
	else if(host->detect_mode == AKMCI_DETECT_MODE_AD) {
		memset(&host->detect_nb, 0, sizeof(host->detect_nb));
		host->detect_nb.notifier_call = set_mci_plugin;
		addetect_register_client(&host->detect_nb);
	}else if (host->detect_mode == AKMCI_PLUGIN_ALWAY)
	{
		host->card_status = CARD_PLUGED;
	}

	if (validate_gpio_info (gpio_pwr)) {

        pr_info("## MMC Power: GPIO%02d", gpio_pwr->pin);
        plat->gpio_init(gpio_pwr);
#if 0
        ak_gpio_setpin(gpio_pwr->pin, gpio_pwr->pwr_dn);
        msleep(500);
        ak_gpio_setpin(gpio_pwr->pin, !gpio_pwr->pwr_dn);
        msleep(500);
#else
		if(host->card_status == CARD_UNPLUGED)
		{
			/* power off*/
			 pr_info("power off\n");
			 ak_gpio_setpin(gpio_pwr->pin, gpio_pwr->pwr_dn);
			 akmci_init_sharepin_idle(host);
			 msleep(500);
		}else
		{
			 /* reset and power on*/
			 pr_info("power on\n");
			 //ak_gpio_setpin(gpio_pwr->pin, gpio_pwr->pwr_dn);
			 //msleep(500);
			 ak_gpio_setpin(gpio_pwr->pin, !gpio_pwr->pwr_dn);
			 msleep(500);
		}
#endif
        switch (pdev->id) {
        	case 0:
        		host->power.k_attr.attr.name = __stringify (0);
        		break;
        	case 1:
        		host->power.k_attr.attr.name = __stringify (1);
        		break;
        	default:
        		break;
        }

		host->power.k_attr.attr.mode = 0666;
		host->power.k_attr.show = mmc_power_gpio_read;
		host->power.k_attr.store = mmc_power_gpio_write;

		host->power.gpio  = gpio_pwr->pin;
		host->power.value = gpio_pwr->value;

		ret = sysfs_create_file(gpio_pwr_obj, &host->power.k_attr.attr);
		if (ret) {
			pr_err("Create mmc_gpio sysfs file failed\n");
			kobject_put(gpio_pwr_obj);
		}
	}
	
//#ifdef READ_ONLY_SD   //change lyc del wp  //change lyc del wp
	if (plat->gpio_wp.pin >= 0) {
		host->gpio_wp = plat->gpio_wp.pin;
		plat->gpio_init(&plat->gpio_wp);
	}
//#endif
	
	ret = akmci_cpufreq_register(host);
	if (ret) {
		goto detect_irq_free;
	}
	
	platform_set_drvdata(pdev, mmc);

	ret = mmc_add_host(mmc);
	if (ret) {
		goto probe_cpufreq_free;
	}

	dev_info(&pdev->dev, "Mci Interface driver.%s."
		" using %s, %s IRQ. detect mode:%s.\n", 
		mmc_hostname(mmc), xfer_mode_desc[akmci_xfer_mode(host)],
		 mmc->caps & MMC_CAP_SDIO_IRQ ? "hw" : "sw", 
		 detect_mode_desc[host->detect_mode]);

	mci_host[slot_index++] = host->mmc;
	pr_info("slot_index=%d\n", slot_index);

	return 0;
	
probe_cpufreq_free:
	
detect_irq_free:
	if(host->detect_mode == AKMCI_DETECT_MODE_GPIO) { //cdh:add for akmci free irq_cd only usr gpio
		free_irq(host->irq_cd, host);
	}
clk_disable:
	clk_disable(host->clk);

clk_free:
	clk_put(host->clk);

probe_free_irq:	
	free_irq(host->irq_mci, host);

probe_iounmap:
	iounmap(host->base);

probe_free_mem_region:
	release_mem_region(host->mem->start, resource_size(host->mem));

probe_free_host:
	mmc_free_host(host->mmc);

probe_out:
	return ret;
}

static int __devexit akmci_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct akmci_host *host;
	struct ak_mci_platform_data *plat = pdev->dev.platform_data;
	
	mmc = platform_get_drvdata(pdev);
	host = mmc_priv(mmc);
	
	mmc_remove_host(mmc);
	
	akmci_cpufreq_deregister(host);
	
	if(host->detect_mode == AKMCI_DETECT_MODE_AD)
		addetect_unregister_client(&host->detect_nb);

	clk_disable(host->clk);
	clk_put(host->clk);

	/*
	*cdh:add for akmci free irq_cd only usr gpio
	*/
	if(host->detect_mode == AKMCI_DETECT_MODE_GPIO) { 
		free_irq(host->irq_cd, host);
	}
	
	free_irq(host->irq_mci, host);
	
	iounmap(host->base);
	release_mem_region(host->mem->start, resource_size(host->mem));

	if (plat->gpio_pwr.pin >= 0) {
		sysfs_remove_file (gpio_pwr_obj, &host->power.k_attr.attr);
	}

	mmc_free_host(host->mmc);
	slot_index = 0;

	return 0;
}

#ifdef CONFIG_PM
static int akmci_suspend(struct device *dev)
{
	struct mmc_host *mmc = platform_get_drvdata(to_platform_device(dev));

	return mmc_suspend_host(mmc);
}

static int akmci_resume(struct device *dev)
{
	struct mmc_host *mmc = platform_get_drvdata(to_platform_device(dev));

	return mmc_resume_host(mmc);
}

static struct dev_pm_ops akmci_pm = {
	.suspend = akmci_suspend,
	.resume = akmci_resume
};

#define akmci_pm_ops  &akmci_pm
#else
#define akmci_pm_ops  NULL
#endif

struct platform_device_id ak_mci_ids[] ={
	{.name = "ak_mci", .driver_data = MCI_MODE_MMC_SD,},
	{.name = "ak_sdio",	.driver_data = MCI_MODE_SDIO,},
};

static struct platform_driver akmci_driver = {
	.probe = akmci_probe,
	.remove = __devexit_p(akmci_remove),
	.id_table	= ak_mci_ids,
	.driver 	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.pm 	= akmci_pm_ops,
	},

};

/**
 * @brief register mmc/sd module driver  to platform
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @return register status..
 */
static int __init akmci_init(void)
{
	printk("AK MCI Driver (c) 2010 ANYKA\n");
	gpio_pwr_obj = kobject_create_and_add ("sdio_pwr", NULL);
	if (!gpio_pwr_obj) {
		printk ("cant craete mmc_pwr.\n");
		return -1;
	}
	return platform_driver_register(&akmci_driver);
}

/**
 * @brief release mmc/sd module from platform
 * 
 * @author Hanyang
 * @date 2011-05-10
 * @return void.
 */
static void __exit akmci_exit(void)
{
	platform_driver_unregister(&akmci_driver);
	kobject_put (gpio_pwr_obj);
	gpio_pwr_obj = NULL;
	return;
}


module_init(akmci_init);
module_exit(akmci_exit);

MODULE_DESCRIPTION("Anyka MCI Interface driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anyka");

