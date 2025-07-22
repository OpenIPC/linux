/*
 * SFC controller for SPI protocol, use FIFO and DMA;
 *
 * Copyright (c) 2015 Ingenic
 * Author: <xiaoyang.fu@ingenic.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>


#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#include "jz_sfc.h"


#define SWAP_BUF_SIZE (4 * 1024)
//#define SFC_DEBUG
#define GET_PHYADDR(a)                                          \
({                                              \
	 unsigned int v;                                        \
	 if (unlikely((unsigned int)(a) & 0x40000000)) {                            \
	 v = page_to_phys(vmalloc_to_page((const void *)(a))) | ((unsigned int)(a) & ~PAGE_MASK); \
	 } else                                         \
	 v = ((int)(a) & 0x1fffffff);                   \
	 v;                                             \
})



#ifdef SFC_DEGUG
#define  print_dbg(format,arg...)	printk(format,## arg)
#else
#define  print_dbg(format,arg...)
#endif

#define MAX_ADDR_LEN 4
#define JZ_NOR_INIT(cmd1,addr_len1,daten1,pollen1,M7_01,dummy_byte1,dma_mode1) \
{										\
	.cmd = cmd1,                \
	.addr_len = addr_len1,        \
	.daten = daten1,               \
	.pollen = pollen1,           \
	.M7_0 = M7_01,              \
	.dummy_byte = dummy_byte1,   \
	.dma_mode = dma_mode1,      \
}

#define JZ_NOR_REG   \
	JZ_NOR_INIT(CMD_WREN,0,0,0,0,0,0), \
	JZ_NOR_INIT(CMD_WRDI,0,0,0,0,0,0), \
	JZ_NOR_INIT(CMD_RDSR,0,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_RDSR_1,0,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_RDSR_2,0,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_WRSR,0,1,0,0,0,0),\
	JZ_NOR_INIT(CMD_WRSR_1,0,1,0,0,0,0),\
	JZ_NOR_INIT(CMD_WRSR_2,0,1,0,0,0,0),\
	JZ_NOR_INIT(CMD_READ,3,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_DUAL_READ,3,1,0,0,8,1),\
	JZ_NOR_INIT(CMD_QUAD_READ,3,1,0,0,8,1),\
	JZ_NOR_INIT(CMD_QUAD_IO_FAST_READ,4,1,0,1,4,1),\
	JZ_NOR_INIT(CMD_QUAD_IO_WORD_FAST_READ,4,1,0,1,2,1),\
	JZ_NOR_INIT(CMD_PP,3,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_QPP,3,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_BE_32K,3,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_BE_64K,3,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_SE,3,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_CE,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_DP,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_RES,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_REMS,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_RDID,0,1,0,0,0,0), \
	JZ_NOR_INIT(CMD_FAST_READ,3,1,0,0,8,1), \
	JZ_NOR_INIT(CMD_EN4B,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_EX4B,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_RESET_EN,0,0,0,0,0,0), \
	JZ_NOR_INIT(CMD_RESET,0,0,0,0,0,0), \
	JZ_NOR_INIT(CMD_NON,0,0,0,0,0,0)



struct sfc_nor_info jz_sfc_nor_info[] = {
//	JZ_NAND_REG,
	JZ_NOR_REG,
};


int jz_nor_info_num = ARRAY_SIZE(jz_sfc_nor_info);
/* Max time can take up to 3 seconds! */
#define MAX_READY_WAIT_TIME 3000    /* the time of erase BE(64KB) */

struct mtd_partition *jz_mtd_partition;
struct spi_nor_platform_data *board_info;
int quad_mode = 0;


static  struct sfc_nor_info * check_cmd(int cmd)
{
	int i = 0;
	for(i = 0;i < jz_nor_info_num ;i++){
		if(jz_sfc_nor_info[i].cmd == cmd){
			return &jz_sfc_nor_info[i];
		}
	}

	return &jz_sfc_nor_info[i - 1];
}

#define SPI_BITS_8		8
#define SPI_BITS_16		16
#define SPI_BITS_24		24
#define SPI_BITS_32		32
#define SPI_8BITS		1
#define SPI_16BITS		2
#define SPI_32BITS		4
#define STATUS_SUSPND    (1<<0)
#define STATUS_BUSY   (1<<1)
#define R_MODE			0x1
#define W_MODE			0x2
#define RW_MODE			(R_MODE | W_MODE)
#define MODEBITS			(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH)
#define SPI_BITS_SUPPORT		(SPI_BITS_8 | SPI_BITS_16 | SPI_BITS_24 | SPI_BITS_32)
#define NUM_CHIPSELECT			8
#define BUS_NUM				0
#define THRESHOLD			31


struct jz_sfc *to_jz_spi_norflash(struct mtd_info *mtd_info)
{
	return container_of(mtd_info, struct jz_sfc, mtd);
}

static unsigned int cpu_read_rxfifo(struct jz_sfc *flash)
{
	int i;
	unsigned long  len = (flash->len + 3) / 4 ;
	unsigned int fifo_num = 0;

	fifo_num = sfc_fifo_num(flash);

	if (fifo_num > THRESHOLD)
		fifo_num = THRESHOLD;

	for(i = 0; i < fifo_num; i++) {
		sfc_read_data(flash, (flash->rx));
		flash->rx++;
		flash->rlen ++;
	}

	if(len == flash->rlen){
		print_dbg("recive ok\n");
//		sfc_flush_fifo(flash);
		flash->rlen = flash->len;
		return flash->rlen;
	}

	return -1;
}

static unsigned int cpu_write_txfifo(struct jz_sfc *flash)
{
	int i;
	unsigned long  len = (flash->len + 3) / 4;
	unsigned int fifo_num = 0;

	if ((len - flash->rlen) > THRESHOLD)
		fifo_num = THRESHOLD;
	else {
		fifo_num = len - flash->rlen;
	}

	for(i = 0; i < fifo_num; i++) {
		sfc_write_data(flash, (flash->tx1));
		flash->tx1++;
		flash->rlen ++;
	}

	if(len == flash->rlen){
		print_dbg("transfer ok\n");
		flash->rlen = flash->len;
		return flash->rlen;
	}
	return -1;
}
#if 0
static void sfc_irq_work(struct work_struct *work)
{
	struct jz_sfc *flash = container_of(work, struct jz_sfc, rw_work);


//	mutex_lock(&flash->mutex_lock);
	if(flash->tx){
		cpu_write_txfifo(flash);
	}else if(flash->rx){
		cpu_read_rxfifo(flash);
	}else{
		printk("rw work happen error\n");
	}
//	mutex_unlock(&flash->mutex_lock);
}
#endif
static irqreturn_t jz_sfc_pio_irq_callback(struct jz_sfc *flash)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	if (ssi_underrun(flash)) {
		sfc_clear_under_intc(flash);
		dev_err(flash->dev, "sfc UNDR !\n");
		complete(&flash->done);
		return IRQ_HANDLED;
	}

	if (ssi_overrun(flash)) {
		sfc_clear_over_intc(flash);
		dev_err(flash->dev, "sfc OVER !\n");
		complete(&flash->done);
		return IRQ_HANDLED;
	}

	if (rxfifo_rreq(flash)) {
		sfc_clear_rreq_intc(flash);
		//flush_work(&flash->rw_work);
		//queue_work(flash->workqueue, &flash->rw_work);
		cpu_read_rxfifo(flash);
		return IRQ_HANDLED;
	}

	if (txfifo_treq(flash)) {
		sfc_clear_treq_intc(flash);
		//flush_work(&flash->rw_work);
		//queue_work(flash->workqueue, &flash->rw_work);
		cpu_write_txfifo(flash);
		return IRQ_HANDLED;
	}

	if(sfc_end(flash)){
		sfc_clear_end_intc(flash);
		/*this is a bug that the sfc_real_time_register
		 * not be the value we want
		 * to get , so we have to
		 * fource a value to the spi device driver
		 *
		 * */
		if(flash->nor_info->pollen){
			if(flash->rx){
				if(flash->nor_info->cmd == CMD_RDSR)
					*(flash->rx) = 0x0;
				if(flash->nor_info->cmd == CMD_RDSR_1)
					*(flash->rx) = 0x2;
				flash->rlen = flash->len;
			}
		}


		complete(&flash->done);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}
static int sfc_pio_transfer(struct jz_sfc *flash)
{

	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	print_dbg("!!!!!! flash->addr_len = %d,,flash->daten = %x,cmd = %x,cmd_flag = %x,flash->len = %x\n",flash->addr_len,flash->daten,flash->cmd,cmd_flag,flash->len);


	sfc_flush_fifo(flash);
	/*use one phase for transfer*/
	if(flash->rw_mode & R_MODE) {
		sfc_transfer_direction(flash, GLB_TRAN_DIR_READ);
		sfc_set_length(flash, flash->len);
		sfc_set_addr_length(flash, 0, flash->nor_info->addr_len);
		sfc_cmd_en(flash, 0, 0x1);
		sfc_data_en(flash, 0, flash->nor_info->daten);
		sfc_write_cmd(flash, 0, flash->nor_info->cmd);
		sfc_dev_addr(flash, 0, flash->addr);
		sfc_dev_addr_plus(flash, 0, flash->rx_addr_plus);
		sfc_dev_addr_dummy_bytes(flash,0,flash->nor_info->dummy_byte);
		if((flash->use_dma ==1)&&(flash->nor_info->dma_mode)){
			dma_cache_sync(NULL, (void *)flash->rx, flash->len, DMA_FROM_DEVICE);
			sfc_set_mem_addr(flash, GET_PHYADDR(flash->rx));
			sfc_transfer_mode(flash, DMA_MODE);
		}else{
			sfc_transfer_mode(flash, SLAVE_MODE);
		}

		sfc_dev_pollen(flash,0,flash->nor_info->pollen);
	}else{
		sfc_transfer_direction(flash, GLB_TRAN_DIR_WRITE);
		sfc_set_length(flash, flash->len);
		sfc_set_addr_length(flash, 0, flash->nor_info->addr_len);
		sfc_cmd_en(flash, 0, 0x1);
		sfc_data_en(flash, 0, flash->nor_info->daten);
		sfc_write_cmd(flash, 0, flash->nor_info->cmd);
		sfc_dev_addr(flash, 0, flash->addr);
		sfc_dev_addr_plus(flash, 0, flash->tx_addr_plus);
		sfc_dev_addr_dummy_bytes(flash,0,flash->nor_info->dummy_byte);

		if((flash->use_dma ==1)&&(flash->nor_info->dma_mode)){
			dma_cache_sync(NULL,(void *)flash->tx1, flash->len, DMA_TO_DEVICE);
			sfc_set_mem_addr(flash, GET_PHYADDR(flash->tx1));
			sfc_transfer_mode(flash, DMA_MODE);
		}else{
			sfc_transfer_mode(flash, SLAVE_MODE);
		}

		sfc_dev_pollen(flash,0,flash->nor_info->pollen);
	}

	if((flash->nor_info->addr_len !=0)&&(flash->nor_info->daten == 1)){
		if(flash->rw_mode & R_MODE){
			if(flash->quad_succeed == 1) {
				sfc_mode(flash,0,flash->board_info->quad_mode->sfc_mode);
			} else {
				sfc_mode(flash,0,flash->sfc_mode);
			}
		}else{
			sfc_mode(flash,0,flash->sfc_mode);
		}
	}
	else
		sfc_mode(flash,0,0);

	/*enable the pollen
	 *the len must be 0 if only
	 *use one phase
	 * */

	if(flash->nor_info->pollen){
		if(flash->nor_info->cmd == CMD_RDSR){
			//sfc_set_length(flash, 0);
			sfc_set_length(flash, 0);
			sfc_dev_sta_exp(flash,0);
			sfc_dev_sta_msk(flash,0x1);
		}
		if(flash->nor_info->cmd == CMD_RDSR_1){
			//sfc_set_length(flash, 0);
			sfc_set_length(flash, 0);
			sfc_dev_sta_exp(flash,2);
			sfc_dev_sta_msk(flash,0x2);
		}
	}

	sfc_enable_all_intc(flash);

	sfc_start(flash);

	return 0;
}

/****
*jz_sfc_pio_txrx
*the driver now can only support spi nor flash
*and Compatible the spi driver
*in the driver we Judge the spi nor flash every cmd
*
*/
static int jz_sfc_pio_txrx(struct jz_sfc *flash, struct sfc_transfer *t)
{
	int ret;
	unsigned long flags;
	int tmp_cmd = 0;
	int err = 0;

	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	if((t->tx_buf == NULL) && (t->rx_buf == NULL)) {
		dev_info(flash->dev, "the tx and rx buf of spi_transfer is NULL !\n");
		return 0;
	}

	flash->tx = t->tx_buf;
	flash->tx1 = t->tx_buf1;
	flash->rx = t->rx_buf;

	flash->len = t->len;
	flash->rlen = 0;
	flash->rw_mode = 0;

	if(flash->quad_succeed == 1) {
		flash->sfc_mode = TRAN_SPI_QUAD;
	} else {
		flash->sfc_mode = 0;
	}

	if(flash->rx){
		flash->rw_mode = R_MODE;
	}else
		flash->rw_mode = W_MODE;
	if(flash->tx){

		tmp_cmd = *(unsigned char *)(flash->tx);
		flash->nor_info = check_cmd(tmp_cmd);

		print_dbg("the tmp_cmd = %x,flash->len = %d\n",tmp_cmd,flash->len);
		if(flash->nor_info->cmd == CMD_NON){
			printk("it is not a commend,will be an error\n");
			return -1;
		}


		if ( flash->nor_info->addr_len != 0) {
			if (flash->addr_len == 4){
				flash->nor_info->addr_len = flash->addr_len;
				flash->addr = (((*(unsigned char*)(t->tx_buf + 1)) << 24)&0xff000000)|
					(((*(unsigned char*)(t->tx_buf + 2)) << 16) &0x00ff0000)|
					(((*(unsigned char*)(t->tx_buf + 3) << 8) &0x0000ff00))|
					((*(unsigned char*)(t->tx_buf + 4)&0x000000ff));
			}else if (flash->addr_len == 3){
				if(flash->nor_info->M7_0 == 1){
					flash->addr = (((*(unsigned char*)(t->tx_buf + 1)) << 24)&0xff000000)|
						(((*(unsigned char*)(t->tx_buf + 2)) << 16) &0x00ff0000)|
						(((*(unsigned char*)(t->tx_buf + 3)) << 8 ) &0x0000ff00) | 0x00; // the end 8 bit is mod bit
				}else{
					flash->nor_info->addr_len = flash->addr_len;
					flash->addr = (((*(unsigned char*)(t->tx_buf + 1)) << 16)&0x00ff0000)|
						(((*(unsigned char*)(t->tx_buf + 2)) << 8) &0x0000ff00)|
						((*(unsigned char*)(t->tx_buf + 3)&0x000000ff));
				}

			}else if ( flash->addr_len == 2) {
				flash->nor_info->addr_len = flash->addr_len;
				flash->addr = (((*(unsigned char*)(t->tx_buf + 1)) << 8) &0x0000ff00)|
					((*(unsigned char*)(t->tx_buf + 2)&0x000000ff));
			}else{
				printk("the flash addr_len is not support\n");
				return EINVAL;
			}
		}else
			flash->addr = 0;

		print_dbg("flash->nor_info->cmd = %x,flash->nor_info->daten = %x,flash->nor_info->addr_len = %x,flash->nor_info->can_train = %d,flash->addr = %x\n",flash->nor_info->cmd,flash->nor_info->daten,
				flash->nor_info->addr_len,flash->nor_info->can_train,flash->addr);
	}else{
		dev_err(flash->dev,"the flash->tx can not be NULL\n");
			return EINVAL;
	}
	spin_lock_irqsave(&flash->lock_rxtx, flags);
	ret = sfc_pio_transfer(flash);
	if (ret < 0) {
		dev_err(flash->dev,"data transfer error!,please check the cmd,and the driver do not support spi nand flash\n");
		sfc_mask_all_intc(flash);
		sfc_clear_all_intc(flash);
		spin_unlock_irqrestore(&flash->lock_rxtx, flags);
		return ret;
	}
	spin_unlock_irqrestore(&flash->lock_rxtx, flags);

#ifdef SFC_DEBUG
	dump_sfc_reg(flash);
#endif

	err = wait_for_completion_timeout(&flash->done,10*HZ);
	if (!err) {
		dump_sfc_reg(flash);
		dev_err(flash->dev, "Timeout for ACK from SFC device\n");
		return -ETIMEDOUT;
	}
    /*fix the cache line problem,when use jffs2 filesystem must be flush cache twice*/
	if(flash->rw_mode & R_MODE)
		dma_cache_sync(NULL, (void *)flash->rx, flash->len, DMA_FROM_DEVICE);

	if(flash->use_dma == 1)
		return flash->len;

	if(flash->rlen != flash->len){
	//	dump_sfc_reg(flash);
		print_dbg("the length is not mach,flash->rlen = %d,flash->len = %d,return !\n",flash->rlen,flash->len);
		if(flash->rlen < flash->len)
			    flash->rlen = flash->len;
		else
			return EINVAL;

	}

	return flash->rlen;
}

static int jz_sfc_init_setup(struct jz_sfc *flash)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	sfc_init(flash);

	sfc_stop(flash);

	/*set hold high*/
	sfc_hold_invalid_value(flash, 1);
	/*set wp high*/
	sfc_wp_invalid_value(flash, 1);

	sfc_clear_all_intc(flash);
	sfc_mask_all_intc(flash);

	sfc_threshold(flash, flash->threshold);

	/*config the sfc pin init state*/
	sfc_clock_phase(flash, 0);
	sfc_clock_polarity(flash, 0);
	sfc_ce_invalid_value(flash, 1);

	flash->sfc_mode = 0;

	sfc_transfer_mode(flash, SLAVE_MODE);

	if(flash->src_clk >= 100000000){
//		printk("############## the cpm = %x\n",*(volatile unsigned int*)(0xb0000074));
		sfc_smp_delay(flash,DEV_CONF_HALF_CYCLE_DELAY);
	}

	flash->swap_buf = kmalloc(SWAP_BUF_SIZE + PAGE_SIZE,GFP_KERNEL);
	if(flash->swap_buf == NULL){
		dev_err(flash->dev,"alloc mem error\n");
		return ENOMEM;
	}

#if defined(CONFIG_SFC_DMA)
	flash->use_dma = 1;
#endif
	flash->irq_callback = &jz_sfc_pio_irq_callback;
	return 0;
}

static irqreturn_t jz_sfc_irq(int irq, void *dev)
{
	struct jz_sfc *flash = dev;

	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	return flash->irq_callback(flash);
}


static int jz_spi_norflash_status(struct jz_sfc *flash, int *status,int num)
{
	int ret;
	unsigned char command_stage1[1];
	unsigned char command_stage2[16];
	struct sfc_transfer transfer[1];
	unsigned char cmd_rdsr[3] = {0x05, 0x35, 0x15};

	command_stage1[0] = cmd_rdsr[num];

	transfer[0].tx_buf = command_stage1;
	transfer[0].tx_buf1 = NULL;
	transfer[0].rx_buf = command_stage2;
	transfer[0].len = 1;//sizeof(command_stage2);
	ret = jz_sfc_pio_txrx(flash, transfer);
	if(ret != transfer[0].len){
		dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);
	}
	*status = command_stage2[0];

	return 0;
}

static int jz_spi_norflash_wait_till_ready(struct jz_sfc *flash)
{
	int status, ret;
	unsigned long deadline;

	deadline = jiffies + msecs_to_jiffies(MAX_READY_WAIT_TIME);
	do {
		ret = jz_spi_norflash_status(flash, &status,0);
		if (ret){
			printk("this will be an error\n");
			return ret;
		}

		if (!(status & SR_WIP))
			return 0;

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	printk("error happen wait timeout\n");
	return -ETIMEDOUT;
}

#ifdef CONFIG_JZ_SFC_NOR_RESET
static int jz_spi_norflash_reset_enable(struct jz_sfc *flash)
{
	int ret;
	unsigned char command_stage1[1];
	struct sfc_transfer transfer[1];

	command_stage1[0] = CMD_RESET_EN;

	transfer[0].tx_buf  = command_stage1;
	transfer[0].tx_buf1 = NULL;
	transfer[0].rx_buf =  NULL;
	transfer[0].len = 0;
	ret = jz_sfc_pio_txrx(flash, transfer);
	if(ret != transfer[0].len)
		dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret)
		return ret;
	return 0;
}

static int jz_spi_norflash_reset(struct jz_sfc *flash)
{
	int ret = 0;
	unsigned char command_stage1[1];
	struct sfc_transfer transfer[1];

	ret = jz_spi_norflash_reset_enable(flash);
	if(ret)
		return ret;
	command_stage1[0] = CMD_RESET;

	transfer[0].tx_buf  = command_stage1;
	transfer[0].tx_buf1 = NULL;
	transfer[0].rx_buf =  NULL;
	transfer[0].len = 0;
	ret = jz_sfc_pio_txrx(flash, transfer);
	if(ret != transfer[0].len)
		dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret)
		return ret;
	return 0;
}
#endif

static int jz_spi_norflash_write_enable(struct jz_sfc *flash)
{
	int ret;
	unsigned char command_stage1[1];
	struct sfc_transfer transfer[1];

	command_stage1[0] = CMD_WREN;

	transfer[0].tx_buf  = command_stage1;
	transfer[0].tx_buf1 = NULL;
	transfer[0].rx_buf =  NULL;
	transfer[0].len = 0;
	ret = jz_sfc_pio_txrx(flash, transfer);
	if(ret != transfer[0].len)
		dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret)
		return ret;
	return 0;
}

#ifdef CONFIG_SPI_QUAD
static int jz_spi_norflash_set_quad_mode(struct jz_sfc *flash)
{
	int status = 0, ret = 0;
	unsigned char command_stage1[1];
	unsigned char command_stage2[16];
	struct sfc_transfer transfer[1];

	if(flash->board_info->quad_mode != NULL){
		jz_spi_norflash_write_enable(flash);

		command_stage1[0] = flash->board_info->quad_mode->WRSR_CMD;//CMD_WRSR_1;
		command_stage2[0] = flash->board_info->quad_mode->WRSR_DATE;//0x2;
		transfer[0].tx_buf  = command_stage1;
		transfer[0].tx_buf1 = command_stage2;
		transfer[0].rx_buf =  NULL;
		transfer[0].len = flash->board_info->quad_mode->WD_DATE_SIZE,//1;//sizeof(command_stage2);
		ret = jz_sfc_pio_txrx(flash, transfer);
		if(ret != transfer[0].len)
			dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

		ret = jz_spi_norflash_wait_till_ready(flash);
		if (ret)
			return ret;

		/*read the quad mode register*/
		command_stage1[0] = flash->board_info->quad_mode->RDSR_CMD;
		transfer[0].tx_buf = command_stage1;
		transfer[0].tx_buf1 = NULL;
		transfer[0].rx_buf = command_stage2;
		transfer[0].len = flash->board_info->quad_mode->RD_DATE_SIZE;//1;//sizeof(command_stage2);
		ret = jz_sfc_pio_txrx(flash, transfer);
		if(ret != transfer[0].len){
			dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);
			return -EROFS;
		}
		status = command_stage2[0];

		if (!(status & flash->board_info->quad_mode->RDSR_DATE)) {
			dev_err(flash->dev,"quad node configuration values do not match,%d,%s\n",__LINE__,__func__);
			return -EROFS;
		}

		quad_mode = 1;
	}else{
		dev_err(flash->dev,"the struct quad_mode is NULL , %d,%s\n",__LINE__,__func__);

	}
	return 0;
}
#endif

static int jz_spi_norflash_set_address_mode(struct jz_sfc *flash,int on)
{
	int ret;
	unsigned char command_stage1[1];
	struct sfc_transfer transfer[1];

	if(flash->board_info->addrsize == 4){

		if(on == 1){
			command_stage1[0] = CMD_EN4B;
		}else{
			command_stage1[0] = CMD_EX4B;
		}

		transfer[0].tx_buf  = command_stage1;
		transfer[0].tx_buf1 = NULL;
		transfer[0].rx_buf =  NULL;
		transfer[0].len = 0;
		ret = jz_sfc_pio_txrx(flash, transfer);
		if(ret != transfer[0].len)
			dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

		ret = jz_spi_norflash_wait_till_ready(flash);
		if (ret)
			return ret;
	}
	return 0;
}

static int jz_spi_norflash_erase_sector(struct jz_sfc *flash, uint32_t offset)
{
	int ret = 0, j;
	unsigned char command_stage1[5];
	struct sfc_transfer transfer[1];

	ret = jz_spi_norflash_write_enable(flash);
	if (ret)
		return ret;

	switch(flash->mtd.erasesize) {
	case 0x1000:
		command_stage1[0] = SPINOR_OP_BE_4K;
		break;
	case 0x8000:
		command_stage1[0] = SPINOR_OP_BE_32K;
		break;
	case 0x10000:
		command_stage1[0] = SPINOR_OP_SE;
		break;
	}
	/*
	command_stage1[1] = offset >> 16;
	command_stage1[2] = offset >> 8;
	command_stage1[3] = offset;
	*/
	for(j = 1; j <= flash->addr_len; j++){
			command_stage1[j] = offset >> (flash->addr_len - j) * 8;
	}

	transfer[0].tx_buf  = command_stage1;
	transfer[0].tx_buf1 = NULL;
	transfer[0].rx_buf =  NULL;
	transfer[0].len = 0;//sizeof(command_stage1);
	ret = jz_sfc_pio_txrx(flash, transfer);
	if(ret != transfer[0].len){
		printk("the ret = %d,the transfer[0].len = %d\n",ret,transfer[0].len);
		dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);
	}
	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret){
		printk("wait timeout\n");
		return ret;
	}

	return 0;
}

static int jz_spi_norflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int ret;
	uint32_t addr, end;
	struct jz_sfc *flash;

	flash = to_jz_spi_norflash(mtd);

	mutex_lock(&flash->lock);

	addr = (instr->addr & (mtd->erasesize - 1));
	if (addr) {
		printk("%s eraseaddr no align\n", __func__);
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}

	end = (instr->len & (mtd->erasesize - 1));
	if (end) {
		printk("%s erasesize no align\n", __func__);
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}

	addr = (uint32_t)instr->addr;
	end = addr + (uint32_t)instr->len;

	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("spi wait timeout !\n");
		mutex_unlock(&flash->lock);
		return ret;
	}

	while (addr < end) {

		ret = jz_spi_norflash_erase_sector(flash, addr);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("erase error !\n");
			mutex_unlock(&flash->lock);
			instr->state = MTD_ERASE_FAILED;
			return ret;
		}

		addr += mtd->erasesize;
	}

	mutex_unlock(&flash->lock);

	instr->state = MTD_ERASE_DONE;

	mtd_erase_callback(instr);

	return 0;
}


static int jz_spi_norflash_read(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, unsigned char *buf)
{
	int ret,j;
	unsigned char command_stage1[5];
	struct sfc_transfer transfer[1];
	struct jz_sfc *flash;
	unsigned int tmp_len = 0;
	unsigned int rlen = 0;
	unsigned char *swap_buf = NULL;

	flash = to_jz_spi_norflash(mtd);
	swap_buf = flash->swap_buf;

	mutex_lock(&flash->lock);

	if(flash->quad_succeed == 1) {
		command_stage1[0] = flash->board_info->quad_mode->cmd_read;//SPINOR_OP_READ_1_1_4;
	} else {
		command_stage1[0] = SPINOR_OP_READ;//SPINOR_OP_READ_FAST;
	}

    /*printk("[Debug-%s-%d]--len = %d----\n",__func__,__LINE__,len);*/
	if(len <= SWAP_BUF_SIZE){
		/*
		command_stage1[1] = from >> 16;
		command_stage1[2] = from >> 8;
		command_stage1[3] = from;
		*/
		for(j = 1; j <= flash->addr_len; j++){
			command_stage1[j] = from >> (flash->addr_len - j) * 8;
		}

		transfer[0].tx_buf = command_stage1;
		transfer[0].tx_buf1 = NULL;
		transfer[0].rx_buf = swap_buf;
		transfer[0].len = len;

		ret = jz_sfc_pio_txrx(flash, transfer);
		if(ret != transfer[0].len)
			dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

		*retlen = ret;

		memcpy(buf,swap_buf,len);
	}else{
		while(!(len  == tmp_len)){
			if ((len - tmp_len) > SWAP_BUF_SIZE)
				rlen = SWAP_BUF_SIZE;
			else {
				rlen = len - tmp_len;
			}

			/*
			command_stage1[1] = (from + tmp_len) >> 16;
			command_stage1[2] = (from + tmp_len) >> 8;
			command_stage1[3] = (from + tmp_len);
			*/
			for(j = 1; j <= flash->addr_len; j++){
				command_stage1[j] = (from + tmp_len) >> (flash->addr_len - j) * 8;
			}

			transfer[0].tx_buf = command_stage1;
			transfer[0].tx_buf1 = NULL;
			transfer[0].rx_buf = swap_buf;
			transfer[0].len = rlen;

			ret = jz_sfc_pio_txrx(flash, transfer);
			if(ret != transfer[0].len)
				dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

			*retlen += ret;

			memcpy(buf + tmp_len,swap_buf,rlen);

			tmp_len += rlen;
		}
	}

	mutex_unlock(&flash->lock);

	return 0;
}

static int jz_spi_norflash_write(struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const unsigned char *buf)
{
	unsigned int i, j, ret, err;
	int ret_addr, actual_len, write_len;
	unsigned char command_stage1[5];
	struct sfc_transfer transfer[1];
	unsigned char * swap_buf= NULL;
	struct jz_sfc *flash;

	flash = to_jz_spi_norflash(mtd);

	swap_buf = flash->swap_buf;
	mutex_lock(&flash->lock);
	*retlen = 0;

	ret_addr = (to & (mtd->writesize - 1));

	if(mtd->writesize - ret_addr > len)
		actual_len = len;
	else
		actual_len = mtd->writesize - ret_addr;

	/* less than mtd->writesize */
		ret = jz_spi_norflash_write_enable(flash);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write enable error !\n");
			mutex_unlock(&flash->lock);
			return ret;
		}

		if(flash->quad_succeed == 1) {
			command_stage1[0] = SPINOR_OP_QPP;
		} else {
			command_stage1[0] = SPINOR_OP_PP;
		}
		/*
		command_stage1[1] = to >> 16;
		command_stage1[2] = to >> 8;
		command_stage1[3] = to;
		*/
		for(j = 1; j <= flash->addr_len; j++){
			command_stage1[j] = to >> (flash->addr_len - j) * 8;
		}

		memcpy(swap_buf,buf,actual_len);

		transfer[0].tx_buf = command_stage1;
		transfer[0].tx_buf1 = swap_buf;//command_stage2;
		transfer[0].rx_buf = NULL;
		transfer[0].len = actual_len;

		ret = jz_sfc_pio_txrx(flash, transfer);
		if(ret != transfer[0].len)
			dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

		err = jz_spi_norflash_wait_till_ready(flash);
		if (err) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("wait timeout !\n");
			mutex_unlock(&flash->lock);
			return err;
		}


		*retlen += ret;

	for (i = actual_len; i < len; i += mtd->writesize) {
		ret = jz_spi_norflash_write_enable(flash);
		if (ret) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write enable error !\n");
			mutex_unlock(&flash->lock);
			return ret;
		}

		if(flash->quad_succeed == 1) {
			command_stage1[0] = SPINOR_OP_QPP;
		} else {
			command_stage1[0] = SPINOR_OP_PP;
		}

		/*
		command_stage1[1] = (to + i) >> 16;
		command_stage1[2] = (to + i) >> 8;
		command_stage1[3] = (to + i);
		*/
		for(j = 1; j <= flash->addr_len; j++){
			command_stage1[j] = (to + i) >> (flash->addr_len - j) * 8;
		}

		if(len - i < mtd->writesize)
			write_len = len - i;
		else
			write_len = mtd->writesize;

		memcpy(swap_buf,buf + i,write_len);

		transfer[0].tx_buf = command_stage1;
		transfer[0].tx_buf1 = swap_buf;
		transfer[0].rx_buf = NULL;
		transfer[0].len = write_len;

		ret = jz_sfc_pio_txrx(flash, transfer);
		if(ret != transfer[0].len)
			dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);


		err = jz_spi_norflash_wait_till_ready(flash);
		if (err) {
			printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
			printk("write enable error !\n");
			mutex_unlock(&flash->lock);
			return err;
		}

		*retlen += ret;
	}

	mutex_unlock(&flash->lock);

	if(*retlen != len)
		printk("ret len error!\n");

	return 0;
}



static int jz_spi_norflash_match_device(struct jz_sfc *flash)
{
	int ret;
	unsigned int id = 0,i = 0;
	unsigned char command_stage1[1];
	unsigned char command_stage2[16];
	struct sfc_transfer transfer[1];

	mutex_lock(&flash->lock);
	command_stage1[0] = CMD_RDID;

	transfer[0].tx_buf  = command_stage1;
	transfer[0].tx_buf1 = NULL;
	transfer[0].rx_buf =  command_stage2;
	transfer[0].len = 3;//sizeof(command_stage2);
	ret = jz_sfc_pio_txrx(flash, transfer);
	if(ret != transfer[0].len)
		dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);

	id = (command_stage2[0] << 16) | (command_stage2[1] << 8) | command_stage2[2];

	for (i = 0; i < flash->board_info_size; i++) {
		board_info = &flash->board_info[i];
		if (board_info->id == id){
			printk("the id code = %x, the flash name is %s\n",id,board_info->name);
				break;
		}
	}

	if (i == flash->board_info_size) {
		printk("#####unsupport flash,the id code = %x\n",id);
		mutex_unlock(&flash->lock);
		return EINVAL;
	}
	mutex_unlock(&flash->lock);
	return 0;
}

static int __init jz_sfc_probe(struct platform_device *pdev)
{
	struct jz_sfc *flash;
//	const char *jz_probe_types[] = {"cmdlinepart",NULL};
//	struct spi_nor_platform_data *board_info;
//	unsigned int board_info_size = 0;
	int num_partition_info = 0;
	struct resource *res;
	int err = 0,ret = 0;

	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	flash = kzalloc(sizeof(struct jz_sfc), GFP_KERNEL);
	if (!flash) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("kzalloc() error !\n");
		return -ENOMEM;
	}

	flash->dev = &pdev->dev;

	flash->pdata = pdev->dev.platform_data;
	if (flash->pdata == NULL) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		err = -ENOENT;
		goto err_no_pdata;
	}

	flash->chnl= flash->pdata->chnl;
	flash->board_info = flash->pdata->board_info;
	flash->board_info_size = flash->pdata->board_info_size;

	flash->tx_addr_plus = 0;
	flash->rx_addr_plus = 0;
	flash->use_dma = 0;

	/* find and map our resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		err = -ENOENT;
		goto err_no_iores;
	}

	flash->ioarea = request_mem_region(res->start, resource_size(res),
					pdev->name);
	if (flash->ioarea == NULL) {
		dev_err(&pdev->dev, "Cannot reserve iomem region\n");
		err = -ENXIO;
		goto err_no_iores;
	}

	flash->phys = res->start;

	flash->iomem = ioremap(res->start, (res->end - res->start)+1);
	if (flash->iomem == NULL) {
		dev_err(&pdev->dev, "Cannot map IO\n");
		err = -ENXIO;
		goto err_no_iomap;
	}

	flash->irq = platform_get_irq(pdev, 0);
	if (flash->irq <= 0) {
		dev_err(&pdev->dev, "No IRQ specified\n");
		err = -ENOENT;
		goto err_no_irq;
	}

	flash->clk = clk_get(&pdev->dev, "cgu_ssi");
	if (IS_ERR(flash->clk)) {
		dev_err(&pdev->dev, "Cannot get ssi clock\n");
		goto err_no_clk;
	}

	flash->clk_gate = clk_get(&pdev->dev, "sfc");
	if (IS_ERR(flash->clk_gate)) {
		dev_err(&pdev->dev, "Cannot get sfc clock\n");
		goto err_no_clk;
	}

	res = platform_get_resource(pdev, IORESOURCE_BUS, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_BUS\n");
		err = -ENOENT;
		goto err_no_iores;
	}

	flash->src_clk = res->start * 1000000;

	if (clk_get_rate(flash->clk) >= flash->src_clk) {
		clk_set_rate(flash->clk, flash->src_clk);
	} else {
		clk_set_rate(flash->clk, flash->src_clk);
	}

	clk_enable(flash->clk);
	clk_enable(flash->clk_gate);

	platform_set_drvdata(pdev, flash);
	init_completion(&flash->done);
	spin_lock_init(&flash->lock_status);
	spin_lock_init(&flash->lock_rxtx);
	mutex_init(&flash->lock);

//	flash->workqueue = create_singlethread_workqueue("sfc_rw_thread");
//	INIT_WORK(&flash->rw_work,sfc_irq_work);

	flash->threshold = THRESHOLD;
	/* request SFC irq */
	err = request_irq(flash->irq, jz_sfc_irq, 0, pdev->name, flash);
	if (err) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
		goto err_no_irq;
	}

	/* SFC controller initializations for SFC */
	jz_sfc_init_setup(flash);

/* Reset nor flash once in the kernel, off by default */
#ifdef CONFIG_JZ_SFC_NOR_RESET
	dev_info(flash->dev, "Reset the nor flash\n");
	ret = jz_spi_norflash_reset(flash);
	if (ret)
		return ret;
	mdelay(50);
#endif

	/* For NM25Q64/128EVB flash,
	 * flash busy when CS pulls down and up but no data is transmitted,
	 * solve by get status.*/
	ret = jz_spi_norflash_wait_till_ready(flash);
	if (ret)
		return ret;

	ret = jz_spi_norflash_match_device(flash);
	if (ret) {
		printk("unknow id ,the id not match the spi bsp config\n");
		return -ENODEV;
	}

	flash->mtd.name     = "jz_sfc";
	flash->mtd.owner    = THIS_MODULE;
	flash->mtd.type     = MTD_NORFLASH;
	flash->mtd.flags    = MTD_CAP_NORFLASH;
	flash->mtd.erasesize    = board_info->erasesize;
	flash->mtd.writesize    = board_info->pagesize;
	flash->addr_len     = board_info->addrsize;
	flash->mtd.size     = board_info->chipsize;
	flash->mtd._erase   = jz_spi_norflash_erase;
	flash->mtd._read    = jz_spi_norflash_read;
	flash->mtd._write   = jz_spi_norflash_write;
	flash->board_info   = board_info;
	jz_mtd_partition = board_info->mtd_partition;
	num_partition_info = board_info->num_partition_info;

#ifdef CONFIG_SFC_DMA
	dev_info(&pdev->dev, "sfc use DMA mode\n");
#else
	dev_info(&pdev->dev, "sfc use PIO mode\n");
#endif

#ifdef CONFIG_SPI_QUAD
	ret = jz_spi_norflash_set_quad_mode(flash);
	if (ret < 0) {
		flash->quad_succeed = 0;
		dev_info(&pdev->dev, "set quad mode error !\n");
	} else {
		flash->quad_succeed = 1;
		dev_info(&pdev->dev, "nor flash quad mode is set, cmd = %x,now use quad mode!\n",flash->board_info->quad_mode->cmd_read);
	}
#else
	flash->quad_succeed = 0;
	dev_info(&pdev->dev, "nor flash now use standard mode!\n");
#endif

	if(flash->mtd.size > NOR_SIZE_16M) {
		ret = jz_spi_norflash_set_address_mode(flash,1);
		if (ret < 0) {
			dev_info(&pdev->dev, "set 4 bytes mode error !\n");
		}
	}

	printk(KERN_INFO "JZ SFC Controller for SFC channel %d driver register\n",flash->chnl);

	ret = mtd_device_parse_register(&flash->mtd, NULL , NULL, NULL, 0);
	if (ret) {
		kfree(flash);
		dev_set_drvdata(flash->dev, NULL);
		return -ENODEV;
	}
	printk("SPI NOR MTD LOAD OK\n");

	return 0;

err_no_clk:
	clk_put(flash->clk_gate);
	clk_put(flash->clk);
err_no_irq:
	free_irq(flash->irq, flash);
err_no_iomap:
	iounmap(flash->iomem);
err_no_iores:
err_no_pdata:
	release_resource(flash->ioarea);
	kfree(flash->ioarea);
	return err;
}

static int __exit jz_sfc_remove(struct platform_device *pdev)
{
	struct jz_sfc *flash = platform_get_drvdata(pdev);

	jz_spi_norflash_set_address_mode(flash,0);

	clk_disable(flash->clk_gate);
	clk_put(flash->clk_gate);

	clk_disable(flash->clk);
	clk_put(flash->clk);

	free_irq(flash->irq, flash);

	iounmap(flash->iomem);

	release_mem_region(flash->resource->start, resource_size(flash->resource));

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int jz_sfc_suspend(struct platform_device *pdev, pm_message_t msg)
{
	unsigned long flags;
	struct jz_sfc *flash = platform_get_drvdata(pdev);

	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status |= STATUS_SUSPND;
	disable_irq(flash->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);

	clk_disable(flash->clk_gate);
	clk_disable(flash->clk);

	return 0;
}

static int jz_sfc_resume(struct platform_device *pdev)
{
	unsigned long flags;
	struct jz_sfc *flash = platform_get_drvdata(pdev);

	clk_enable(flash->clk);
	clk_enable(flash->clk_gate);

	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status &= ~STATUS_SUSPND;
	enable_irq(flash->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);

	return 0;
}

void jz_sfc_shutdown(struct platform_device *pdev)
{
	unsigned long flags;
	struct jz_sfc *flash = platform_get_drvdata(pdev);

	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status |= STATUS_SUSPND;
	disable_irq(flash->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);

	clk_disable(flash->clk_gate);
	clk_disable(flash->clk);

	return ;
}

static struct platform_driver jz_sfcdrv = {
	.driver		= {
		.name	= "jz-sfc",
		.owner	= THIS_MODULE,
	},
	.remove         = __exit_p(jz_sfc_remove),
	.suspend	= jz_sfc_suspend,
	.resume		= jz_sfc_resume,
};

static int __init jz_sfc_init(void)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
	return platform_driver_probe(&jz_sfcdrv, jz_sfc_probe);
}

static void __exit jz_sfc_exit(void)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
    platform_driver_unregister(&jz_sfcdrv);
}

module_init(jz_sfc_init);
module_exit(jz_sfc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("JZ SFC Driver");
