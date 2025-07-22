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
#include <linux/mtd/nand.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include "jz_sfc_nand.h"
#include "./nand_device/nand_common.h"

#define SFC_SWAP_BUF_SIZE (4 << 10)

#define STATUS_SUSPND	(1<<0)
#define to_jz_spi_nand(mtd_info) container_of(mtd_info, struct sfc_flash, mtd)

/*
 * below is the informtion about nand
 * that user should modify according to nand spec
 * */

static LIST_HEAD(nand_list);
struct sfc_flash *flash;

void dump_flash_info(struct sfc_flash *flash)
{
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct jz_nand_base_param *param = &nand_desc->param;
	struct mtd_partition *partition = nand_desc->partition.partition;
	uint8_t num_partition = nand_desc->partition.num_partition;

	printk("id_manufactory = 0x%02x\n", nand_desc->id_manufactory);
	printk("id_device = 0x%02x\n", nand_desc->id_device);

	printk("pagesize = %d\n", param->pagesize);
	printk("blocksize = %d\n", param->blocksize);
	printk("oobsize = %d\n", param->oobsize);
	printk("flashsize = %d\n", param->flashsize);

	printk("tHOLD = %d\n", param->tHOLD);
	printk("tSETUP = %d\n", param->tSETUP);
	printk("tSHSL_R = %d\n", param->tSHSL_R);
	printk("tSHSL_W = %d\n", param->tSHSL_W);

	printk("ecc_max = %d\n", param->ecc_max);
	printk("need_quad = %d\n", param->need_quad);

	while(num_partition--) {
		printk("partition(%d) name=%s\n", num_partition, partition[num_partition].name);
		printk("partition(%d) size = 0x%llx\n", num_partition, partition[num_partition].size);
		printk("partition(%d) offset = 0x%llx\n", num_partition, partition[num_partition].offset);
		printk("partition(%d) mask_flags = 0x%x\n", num_partition, partition[num_partition].mask_flags);
	}
	return;
}

static int32_t jz_sfc_nand_read(struct sfc_flash *flash, int32_t pageaddr, int32_t columnaddr, u_char *buffer, size_t len)
{
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct jz_nand_read *nand_read_ops = &nand_desc->ops->nand_read_ops;
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	struct flash_operation_message op_info = {  .flash = flash,
						    .pageaddr = pageaddr,
						    .columnaddr = columnaddr,
						    .buffer = buffer,
						    .len = len,
						};

	int32_t ret = 0;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	if(nand_read_ops->pageread_to_cache)
		nand_read_ops->pageread_to_cache(&transfer, &cmd, &op_info);
	else
		nand_pageread_to_cache(&transfer, &cmd, &op_info);
	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
		dev_err(flash->dev,"sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	udelay(nand_desc->param.tRD);

	if(nand_read_ops->get_feature) {
		ret = nand_read_ops->get_feature(&op_info);
	} else {
		dev_err(flash->dev, "ERROR: nand device must have get_read_feature function,id_manufactory= %02x, id_device=%02x\n", nand_desc->id_manufactory, nand_desc->id_device);
		ret = -EIO;
	}

	if(ret == -EIO)
		return ret;

	if(nand_desc->param.need_quad) {
		if(nand_read_ops->quad_read) {
			nand_read_ops->quad_read(&transfer, &cmd, &op_info);
		} else {
		    /*
		     *	default transfer format:
		     *
		     *	addrlen = 2, data_dummy_bits = 8;
		     *
		     * */
			nand_quad_read(&transfer, &cmd, &op_info, 2);
		}
	} else {
		if(nand_read_ops->single_read) {
			nand_read_ops->single_read(&transfer, &cmd, &op_info);
		} else {
		    /*
		     *	default transfer format:
		     *
		     *	addrlen = 2, data_dummy_bits = 8;
		     *
		     * */
			nand_single_read(&transfer, &cmd, &op_info, 2);
		}
	}
	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
		dev_err(flash->dev,"sfc_sync error ! %s %s %d\n", __FILE__, __func__, __LINE__);
		ret = -EIO;
	}

	if(transfer.ops_mode == DMA_OPS)
		dma_cache_sync(NULL, (void *)transfer.data, transfer.len, DMA_FROM_DEVICE);

	return ret;
}


static int badblk_check(int len,unsigned char *buf)
{
	int  j;
	unsigned char *check_buf = buf;

	for(j = 0; j < len; j++){
		if(check_buf[j] != 0xff){
			return 1;
			}
		}
	return 0;
}


static int32_t jz_sfc_nand_write(struct sfc_flash *flash, u_char *buffer, uint32_t pageaddr, uint32_t columnaddr, size_t len)
{
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct jz_nand_write *nand_write_ops = &nand_desc->ops->nand_write_ops;
	struct sfc_transfer transfer[3];
	struct sfc_message message;
	struct cmd_info cmd[3];
	struct flash_operation_message op_info = {  .flash = flash,
						    .pageaddr = pageaddr,
						    .columnaddr = columnaddr,
						    .buffer = buffer,
						    .len = len,
						};
	int32_t ret = 0;

	memset(transfer, 0, sizeof(transfer));
	memset(cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	/*1. write enable*/
	if(nand_write_ops->write_enable)
		nand_write_ops->write_enable(transfer, cmd, &op_info);
	else
		nand_write_enable(transfer, cmd, &op_info);
	sfc_message_add_tail(transfer, &message);

	/*2. write to cache*/
	if(nand_desc->param.need_quad) {
		if(nand_write_ops->quad_load)
			nand_write_ops->quad_load(&transfer[1], &cmd[1], &op_info);
		else
			nand_quad_load(&transfer[1], &cmd[1], &op_info);
	} else {
		if(nand_write_ops->single_load)
			nand_write_ops->single_load(&transfer[1], &cmd[1], &op_info);
		else
			nand_single_load(&transfer[1], &cmd[1], &op_info);
	}
	sfc_message_add_tail(&transfer[1], &message);

	/*3. program exec*/
    	if(nand_write_ops->program_exec)
		nand_write_ops->program_exec(&transfer[2], &cmd[2], &op_info);
	else
		nand_program_exec(&transfer[2], &cmd[2], &op_info);

	sfc_message_add_tail(&transfer[2], &message);
	if(sfc_sync(flash->sfc, &message)) {
		dev_err(flash->dev, "sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	udelay(nand_desc->param.tPP);

	/*4. get status to be sure nand wirte completed*/
	if(nand_write_ops->get_feature)
		ret = nand_write_ops->get_feature(&op_info);
	else
		ret = nand_get_program_feature(&op_info);

	return  ret;
}

static int jz_sfcnand_write_oob(struct mtd_info *mtd, loff_t addr, struct mtd_oob_ops *ops)
{
	struct sfc_flash *flash = to_jz_spi_nand(mtd);
	uint32_t oob_addr = (uint32_t)addr;
	int32_t ret;

	mutex_lock(&flash->lock);
	if((ret = jz_sfc_nand_write(flash, ops->oobbuf, oob_addr / mtd->writesize, mtd->writesize, ops->ooblen))) {
		dev_err(flash->dev, "spi nand write oob error %s %s %d \n",__FILE__,__func__,__LINE__);
		goto write_oob_exit;
	}
	ops->retlen = ops->ooblen;
write_oob_exit:
	mutex_unlock(&flash->lock);
	return ret;
}

static int jz_sfcnand_chip_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t buf[2] = { 0, 0 };
	int  ret = 0, i = 0;
	int write_oob = !(chip->bbt_options & NAND_BBT_NO_OOB_BBM);

	/* Write bad block marker to OOB */
	if (write_oob) {
		struct mtd_oob_ops ops;
		loff_t wr_ofs = ofs;
		ops.datbuf = NULL;
		ops.oobbuf = buf;
		ops.ooboffs = chip->badblockpos;
		if (chip->options & NAND_BUSWIDTH_16) {
			ops.ooboffs &= ~0x01;
			ops.len = ops.ooblen = 2;
		} else {
			ops.len = ops.ooblen = 1;
		}
		ops.mode = MTD_OPS_PLACE_OOB;

		/* Write to first/last page(s) if necessary */
		if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
			wr_ofs += mtd->erasesize - mtd->writesize;

		wr_ofs = (unsigned int)wr_ofs / mtd->erasesize  * mtd->erasesize;
		do {
			ret = jz_sfcnand_write_oob(mtd, wr_ofs, &ops);
			if (ret)
				return ret;
			wr_ofs += mtd->writesize;
			i++;
		} while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);
	}
	/* Update flash-based bad block table */
	if (chip->bbt_options & NAND_BBT_USE_FLASH) {
		ret = nand_update_bbt(mtd, ofs);
	}

	return ret;
}

static int32_t jz_sfc_nand_erase_blk(struct sfc_flash *flash, uint32_t pageaddr)
{
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct jz_nand_erase *nand_erase_ops = &nand_desc->ops->nand_erase_ops;
	struct sfc_transfer transfer[2];
	struct sfc_message message;
	struct cmd_info cmd[2];
	struct flash_operation_message op_info = {  .flash = flash,
						    .pageaddr = pageaddr,
						    .columnaddr = 0,
						    .buffer = NULL,
						    .len = 0,
						};

	int32_t ret;

	memset(transfer, 0, sizeof(transfer));
	memset(cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	/*1. write enable */
	if(nand_erase_ops->write_enable)
		nand_erase_ops->write_enable(transfer, cmd, &op_info);
	else
		nand_write_enable(transfer, cmd, &op_info);
	sfc_message_add_tail(transfer, &message);

	/*2. block erase*/
	if(nand_erase_ops->block_erase)
		nand_erase_ops->block_erase(&transfer[1], &cmd[1], &op_info);
	else
		nand_block_erase(&transfer[1], &cmd[1], &op_info);
	sfc_message_add_tail(&transfer[1], &message);
	if(sfc_sync(flash->sfc, &message)) {
		dev_err(flash->dev, "sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	mdelay(nand_desc->param.tBE);

	/*3. get feature*/
	if(nand_erase_ops->get_feature)
		ret = nand_erase_ops->get_feature(&op_info);
	else
		ret = nand_get_erase_feature(&op_info);

	if(ret)
		dev_err(flash->dev, "Erase error,get state error ! %s %s %d \n",__FILE__,__func__,__LINE__);

	return ret;
}

static int jz_sfcnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct sfc_flash *flash = to_jz_spi_nand(mtd);
	uint32_t addr = (uint32_t)instr->addr;
	uint32_t end;
	int32_t ret;

	if(addr % mtd->erasesize) {
		dev_err(flash->dev, "ERROR:%s line %d eraseaddr no align\n", __func__,__LINE__);
		return -EINVAL;
	}
	end = addr + instr->len;
	instr->state = MTD_ERASING;
	mutex_lock(&flash->lock);
	while (addr < end) {
		if((ret = jz_sfc_nand_erase_blk(flash, addr / mtd->writesize))) {
			dev_err(flash->dev, "spi nand erase error blk id  %d !\n",addr / mtd->erasesize);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
		addr += mtd->erasesize;
	}

	instr->state = MTD_ERASE_DONE;
erase_exit:
	mutex_unlock(&flash->lock);
	return ret;
}

static int jz_sfcnand_block_isbab(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	if(!chip->bbt)
		return chip->block_bad(mtd, ofs, 1);
	return nand_isbad_bbt(mtd, ofs, 0);
}

static int jz_sfcnand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	int ret;
	struct sfc_flash *flash;
	flash=to_jz_spi_nand(mtd);


	ret = jz_sfc_nand_erase_blk(flash,ofs);
	if(ret < 0){
		pr_err("ERROR: mark bad block error !!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
		return -EIO;
	}

	return chip->block_markbad(mtd, ofs);
}


static int jz_sfcnand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	struct sfc_flash *flash = to_jz_spi_nand(mtd);
	uint32_t pagesize = mtd->writesize;
	uint32_t pageaddr;
	uint32_t columnaddr;
	uint32_t rlen;
	int32_t ret = 0, reterr = 0, ret_eccvalue = 0;

	mutex_lock(&flash->lock);
	while(len) {
		pageaddr = (uint32_t)from / pagesize;
		columnaddr = (uint32_t)from % pagesize;
		rlen = min_t(uint32_t, len, pagesize - columnaddr);
		ret = jz_sfc_nand_read(flash, pageaddr, columnaddr, buf, rlen);
		if(ret < 0) {
			dev_err(flash->dev, "%s %s %d: jz_sfc_nand_read error, ret = %d, \
				pageaddr = %u, columnaddr = %u, rlen = %u\n",
					__FILE__, __func__, __LINE__,
					ret, pageaddr, columnaddr, rlen);
			reterr = ret;
			if(ret == -EIO)
				break;
		} else if (ret > 0) {
			dev_dbg(flash->dev, "%s %s %d: jz_sfc_nand_read, ecc value = %d, \
				    pageaddr = %u, columnaddr = %u, rlen = %u\n",
					    __FILE__, __func__, __LINE__,
					    ret, pageaddr, columnaddr, rlen);
			ret_eccvalue = ret;
		}

		len -= rlen;
		from += rlen;
		buf += rlen;
		*retlen += rlen;
	}
	mutex_unlock(&flash->lock);
	return reterr ? reterr : (ret_eccvalue ? ret_eccvalue : ret);
}

static int jz_sfcnand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	struct sfc_flash *flash = to_jz_spi_nand(mtd);
	uint32_t pagesize = mtd->writesize;
	uint32_t pageaddr;
	uint32_t columnaddr;
	uint32_t wlen;
	int32_t ret;

	mutex_lock(&flash->lock);
	while(len) {
		pageaddr = (uint32_t)to / pagesize;
		columnaddr = (uint32_t)to % pagesize;
		wlen = min_t(uint32_t, pagesize - columnaddr, len);

		if((ret = jz_sfc_nand_write(flash, (u_char *)buf, pageaddr, columnaddr, wlen))) {
			dev_err(flash->dev, "%s %s %d : spi nand write fail, ret = %d, \
				pageaddr = %u, columnaddr = %u, wlen = %u\n",
				__FILE__, __func__, __LINE__, ret,
				pageaddr, columnaddr, wlen);
			break;
		}
		*retlen += wlen;
		len -= wlen;
		to += wlen;
		buf += wlen;
	}
	mutex_unlock(&flash->lock);
	return ret;
}

static int32_t jz_sfcnand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct sfc_flash *flash = to_jz_spi_nand(mtd);
	uint32_t addr = (uint32_t)from;
	uint32_t pageaddr = addr / mtd->writesize;
	int32_t ret = 0, ret_eccvalue = 0;

	mutex_lock(&flash->lock);
	if(ops->datbuf) {
		ret = jz_sfcnand_read(mtd, from, ops->len, &ops->retlen, ops->datbuf);
		if(ret < 0) {
			dev_err(flash->dev, "%s %s %d : spi nand read data error, ret = %d\n",__FILE__,__func__,__LINE__, ret);
			if(ret == -EIO) {
				mutex_unlock(&flash->lock);
				return ret;
			} else {
				ret_eccvalue = ret;
			}
		}
	}

	if(ops->oobbuf) {
		ret = jz_sfc_nand_read(flash, pageaddr, mtd->writesize + ops->ooboffs, ops->oobbuf, ops->ooblen);
		if(ret < 0)
			dev_err(flash->dev, "%s %s %d : spi nand read oob error ,ret= %d\n", __FILE__, __func__, __LINE__, ret);

		if(ret != -EIO)
			ops->oobretlen = ops->ooblen;

	}
	mutex_unlock(&flash->lock);

	return ret ? ret : ret_eccvalue;
}

static int jz_sfcnand_block_bad_check(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int check_len = 1;
	unsigned char check_buf[2] = {0x0};
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct mtd_oob_ops ops;

	memset(&ops, 0, sizeof(ops));
	if (chip->options & NAND_BUSWIDTH_16)
		check_len = 2;

	ops.oobbuf = check_buf;
	ops.ooblen = check_len;
	jz_sfcnand_read_oob(mtd, ofs, &ops);
	if(badblk_check(check_len, check_buf))
		return 1;
	return 0;
}

static int jz_sfc_nand_set_feature(struct sfc_flash *flash, uint8_t addr, uint8_t val)
{
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);
	nand_set_feature(&transfer, &cmd, addr, val);
	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
		dev_err(flash->dev,"sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}
	return 0;
}

static int jz_sfc_nand_get_feature(struct sfc_flash *flash, uint8_t addr, uint8_t *val)
{
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);
	nand_get_feature(&transfer, &cmd, addr, val);
	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
		dev_err(flash->dev, "sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}
	return 0;
}

static int32_t jz_sfc_nand_dev_init(struct sfc_flash *flash)
{
	int32_t ret;
	/*release protect*/
	uint8_t feature = 0, status = 0;
	if((ret = jz_sfc_nand_set_feature(flash, SPINAND_ADDR_PROTECT, feature)))
		goto exit;

	if((ret = jz_sfc_nand_get_feature(flash, SPINAND_ADDR_FEATURE, &feature)))
		goto exit;

	feature |= (1 << 4) | (1 << 3) | (1 << 0);
	if((ret = jz_sfc_nand_set_feature(flash, SPINAND_ADDR_FEATURE, feature)))
		goto exit;

	if((ret = jz_sfc_nand_get_feature(flash, SPINAND_ADDR_FEATURE, &status)));
		goto exit;
	if(status != feature) {
		dev_err(flash->dev, "set feature != get feature, set feature failed!\n");
		ret = -EIO;
		goto exit;
	}
	return 0;
exit:
	return ret;
}

static int32_t jz_sfc_nand_try_id(struct sfc_flash *flash)
{
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	struct jz_nand_device *nand_device;
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint8_t addr_len[] = {0, 1};
	uint8_t id_buf[2] = {0};
	uint8_t i = 0, j = 0;

	for(i = 0; i < sizeof(addr_len); i++) {
		memset(&transfer, 0, sizeof(transfer));
		memset(&cmd, 0, sizeof(cmd));
		sfc_message_init(&message);
		transfer.sfc_mode = TM_STD_SPI;
		cmd.cmd = SPINAND_CMD_RDID;

		transfer.addr = 0;
		transfer.addr_len = addr_len[i];

		cmd.dataen = ENABLE;
		transfer.data = id_buf;
		transfer.len = sizeof(id_buf);
		transfer.direction = GLB_TRAN_DIR_READ;
		transfer.data_dummy_bits = 0;

		transfer.cmd_info = &cmd;
		transfer.ops_mode = CPU_OPS;
		sfc_message_add_tail(&transfer, &message);
		if(sfc_sync(flash->sfc, &message)){
			dev_err(flash->dev,"sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
			return -EIO;
		}

		list_for_each_entry(nand_device, &nand_list, nand) {
			if(nand_device->id_manufactory == id_buf[0]) {
				for(j = 0; j < nand_device->id_device_count; j++)
				{
					if(nand_device->id_device_list->id_device == id_buf[1])
					{
						nand_desc->id_manufactory = id_buf[0];
						nand_desc->id_device = id_buf[1];
						break;
					}else{
						nand_device->id_device_list++;
					}
				}
				if(nand_desc->id_manufactory == id_buf[0] && nand_desc->id_device == id_buf[1])
					break;
			}
		}

		if(nand_desc->id_manufactory && nand_desc->id_device)
			    break;
	}
	dev_info(flash->dev, "the nand id_manufactory = 0x%02x, id_device = 0x%02x\n", nand_desc->id_manufactory, nand_desc->id_device);
#ifdef CONFIG_SPI_QUAD
	dev_info(flash->dev,"sfc quad mode\n");
#else
	dev_info(flash->dev,"sfc stand mode\n");
#endif


	if(!nand_desc->id_manufactory && !nand_desc->id_device) {
		dev_err(flash->dev, " ERROR!: don`t support this nand manufactory, please add nand driver\n");
		return -ENODEV;
	} else {
		struct device_id_struct *device_id = nand_device->id_device_list;
		int32_t id_count = nand_device->id_device_count;
		while(id_count--) {
			if(device_id->id_device == nand_desc->id_device) {
			/*notice :base_param and partition param should read from nand*/
				nand_desc->param = *device_id->param;
				break;
			}
			device_id++;
		}
		if(id_count < 0) {
			dev_err(flash->dev, "ERROR: do support this device, id_manufactory = 0x%02x, id_device = 0x%02x\n", nand_desc->id_manufactory, nand_desc->id_device);
			return -ENODEV;
		}
	}
	nand_desc->ops = &nand_device->ops;

	return 0;
}

static int32_t nand_partition_param_copy(struct sfc_flash *flash, struct jz_sfc_nand_burner_param *burn_param) {
	struct jz_nand_descriptor *nand_desc = flash->flash_info;
	int i = 0, count = 5, ret;
	size_t retlen = 0;

	/*partition param copy*/
	nand_desc->partition.num_partition = burn_param->partition_num;

	burn_param->partition = kzalloc(nand_desc->partition.num_partition * sizeof(struct jz_spinand_partition), GFP_KERNEL);
	if (!burn_param->partition) {
	    	dev_err(flash->dev, "alloc partition space failed!\n");
		return -ENOMEM;
	}

	nand_desc->partition.partition = kzalloc(nand_desc->partition.num_partition * sizeof(struct mtd_partition), GFP_KERNEL);
	if (!nand_desc->partition.partition) {
	    	dev_err(flash->dev, "alloc partition space failed!\n");
		kfree(burn_param->partition);
		return -ENOMEM;
	}

partition_retry_read:
	ret = jz_sfcnand_read(&flash->mtd, SPIFLASH_PARAMER_OFFSET + sizeof(*burn_param) - sizeof(burn_param->partition),
		nand_desc->partition.num_partition * sizeof(struct jz_spinand_partition),
		&retlen, (u_char *)burn_param->partition);

	if((ret < 0) && count--)
		goto partition_retry_read;
	if(count < 0) {
		dev_err(flash->dev, "read nand partition failed!\n");
		kfree(burn_param->partition);
		kfree(nand_desc->partition.partition);
		return -EIO;
	}

	for(i = 0; i < burn_param->partition_num; i++) {
		nand_desc->partition.partition[i].name = burn_param->partition[i].name;
		nand_desc->partition.partition[i].size = burn_param->partition[i].size;
		nand_desc->partition.partition[i].offset = burn_param->partition[i].offset;
		nand_desc->partition.partition[i].mask_flags = burn_param->partition[i].mask_flags;
	}
	return 0;
}

static struct jz_sfc_nand_burner_param *burn_param;
static int32_t jz_nand_partition(struct sfc_flash *flash) {

	int32_t ret = 0, retlen = 0, count = 5;

	burn_param = kzalloc(sizeof(struct jz_sfc_nand_burner_param), GFP_KERNEL);
	if(!burn_param) {
		dev_err(flash->dev, "alloc burn_param space error!\n");
		return -ENOMEM;
	}

	count = 5;
param_retry_read:
	ret = jz_sfcnand_read(&flash->mtd, SPIFLASH_PARAMER_OFFSET,
		sizeof(struct jz_sfc_nand_burner_param), &retlen, (u_char *)burn_param);
	if((ret < 0) && count--)
		goto param_retry_read;
	if(count < 0) {
		dev_err(flash->dev, "read nand base param failed!\n");
		ret = -EIO;
		goto failed;
	}

	if(burn_param->magic_num != SPINAND_MAGIC_NUM) {
		dev_info(flash->dev, "NOTICE: this flash haven`t param\n");
		ret = -EINVAL;
		goto failed;
	}

	if(nand_partition_param_copy(flash, burn_param)) {
		ret = -ENOMEM;
		goto failed;
	}

	return 0;
failed:
	kfree(burn_param);
	return ret;

}

int jz_nand_register(struct jz_nand_device *flash) {
	list_add_tail(&flash->nand, &nand_list);
	return 0;
}
EXPORT_SYMBOL_GPL(jz_nand_register);

static int jz_sfc_probe(struct platform_device *pdev)
{
	const char *jz_probe_types[] = {"cmdlinepart",NULL};
	struct nand_chip *chip;
	struct jz_nand_descriptor *nand_desc;
	int ret;
	int chip_param = 1;

	flash = kzalloc(sizeof(struct sfc_flash), GFP_KERNEL);
	if (!flash)
		return -ENOMEM;
	chip = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
	if (!chip) {
		kfree(flash);
		return -ENOMEM;
	}
	nand_desc = kzalloc(sizeof(struct jz_nand_descriptor), GFP_KERNEL);
	if(!nand_desc) {
		kfree(flash);
	    	kfree(chip);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, flash);
	spin_lock_init(&flash->lock_status);
	mutex_init(&flash->lock);
	flash->dev = &pdev->dev;
	flash->flash_info = nand_desc;
	flash->sfc = sfc_res_init(pdev);

#define THOLD	5
#define TSETUP	5
#define TSHSL_R	    20
#define TSHSL_W	    50

	set_flash_timing(flash->sfc, THOLD, TSETUP, TSHSL_R, TSHSL_W);

	if((ret = jz_sfc_nand_dev_init(flash))) {
		dev_err(flash->dev, "nand device init failed!\n");
		goto free_base;
	}

	if((ret = jz_sfc_nand_try_id(flash))) {
		dev_err(flash->dev, "try device id failed\n");
		goto free_base;
	}

	set_flash_timing(flash->sfc, nand_desc->param.tHOLD,
			nand_desc->param.tSETUP, nand_desc->param.tSHSL_R, nand_desc->param.tSHSL_W);
	flash->mtd.name = "sfc_nand";
	flash->mtd.owner = THIS_MODULE;
	flash->mtd.type = MTD_NANDFLASH;
	flash->mtd.flags |= MTD_CAP_NANDFLASH;
	flash->mtd.erasesize = nand_desc->param.blocksize;
	flash->mtd.writesize = nand_desc->param.pagesize;
	flash->mtd.size = nand_desc->param.flashsize;
	flash->mtd.oobsize = nand_desc->param.oobsize;
	flash->mtd.writebufsize = flash->mtd.writesize;
	flash->mtd.bitflip_threshold = flash->mtd.ecc_strength = nand_desc->param.ecc_max - 1;

	chip->select_chip = NULL;
	chip->badblockbits = 8;
	chip->scan_bbt = nand_default_bbt;
	chip->block_bad = jz_sfcnand_block_bad_check;
	chip->block_markbad = jz_sfcnand_chip_block_markbad;
	//chip->ecc.layout= &gd5f_ecc_layout_128; // for erase ops
	chip->bbt_erase_shift = chip->phys_erase_shift = ffs(flash->mtd.erasesize) - 1;
	chip->buffers = kmalloc(sizeof(*chip->buffers), GFP_KERNEL);
	if(!chip->buffers) {
		dev_err(flash->dev, "alloc nand buffer failed\n");
		ret = -ENOMEM;
		goto free_base;
	}

	/* Set the bad block position */
	if (flash->mtd.writesize > 512 || (chip->options & NAND_BUSWIDTH_16))
		chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	else
		chip->badblockpos = NAND_SMALL_BADBLOCK_POS;

	flash->mtd.priv = chip;
	flash->mtd._erase = jz_sfcnand_erase;
	flash->mtd._read = jz_sfcnand_read;
	flash->mtd._write = jz_sfcnand_write;
	flash->mtd._read_oob = jz_sfcnand_read_oob;
	flash->mtd._write_oob = jz_sfcnand_write_oob;
	flash->mtd._block_isbad = jz_sfcnand_block_isbab;
	flash->mtd._block_markbad = jz_sfcnand_block_markbad;

	if((ret = chip->scan_bbt(&flash->mtd))) {
		dev_err(flash->dev, "creat and scan bbt failed\n");
		goto free_all;
	}

	if((ret = jz_nand_partition(flash))) {
		if(ret == -EINVAL){
			chip_param = 0;
			dev_err(flash->dev, "WARNING:read nand partition param failed!\n");
		}
		else {
			dev_err(flash->dev, "read nand device param failed!\n");
			goto free_all;
		}
	}else {
		flash->mtd.name = "chip_param";
	}
#ifdef CONFIG_SPI_QUAD
	dev_info(flash->dev,"sfc quad mode\n");
#else
	dev_info(flash->dev,"sfc stand mode\n");
#endif

/*	dump_flash_info(flash);*/
	if(chip_param)
		ret = mtd_device_parse_register(&flash->mtd, jz_probe_types, NULL, nand_desc->partition.partition, nand_desc->partition.num_partition);
	else
		ret = mtd_device_parse_register(&flash->mtd, NULL, NULL, NULL, 0);
	if (ret) {
		kfree(nand_desc->partition.partition);
		kfree(burn_param->partition);
		kfree(burn_param);
		ret = -ENODEV;
		goto free_all;
	}
	return 0;

free_all:
	kfree(chip->buffers);

free_base:
	kfree(flash);
	kfree(chip);
	kfree(nand_desc);
	return ret;
}

static int jz_sfc_remove(struct platform_device *pdev)
{
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;
	clk_disable(sfc->clk_gate);
	clk_put(sfc->clk_gate);
	clk_disable(sfc->clk);
	clk_put(sfc->clk);
	free_irq(sfc->irq, flash);
	iounmap(sfc->iomem);
	release_mem_region(flash->resource->start, resource_size(flash->resource));
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static int jz_sfc_suspend(struct platform_device *pdev, pm_message_t msg)
{
	unsigned long flags;
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;
	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status |= STATUS_SUSPND;
	disable_irq(sfc->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);
	clk_disable(sfc->clk_gate);
	clk_disable(sfc->clk);
	return 0;
}

static int jz_sfc_resume(struct platform_device *pdev)
{
	unsigned long flags;
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;
	clk_enable(sfc->clk);
	clk_enable(sfc->clk_gate);
	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status &= ~STATUS_SUSPND;
	enable_irq(sfc->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);
	return 0;
}

void jz_sfc_shutdown(struct platform_device *pdev)
{
	unsigned long flags;
	struct sfc_flash *flash = platform_get_drvdata(pdev);
	struct sfc *sfc = flash->sfc;
	struct jz_nand_descriptor *nand_desc = flash->flash_info;

	if(nand_desc->id_manufactory == 0xEF &&
		nand_desc->id_device == 0xAB)
		winbond_reset(flash);
	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status |= STATUS_SUSPND;
	disable_irq(sfc->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);
	clk_disable(sfc->clk_gate);
	clk_disable(sfc->clk);
	return ;
}

static ssize_t spinand_store(struct device_driver *driver, const char *buf,size_t count)
{
	//const char *jz_probe_types[] = {"cmdlinepart",NULL};
	const char *jz_probe_types[] = {"cmdlinepart",NULL};
	unsigned int magic_num = 0;
	int partition_num = 0;
	int buf_ofs = 0;
	int i;

	struct jz_spinand_partition *partition;
	struct mtd_partition *mtd_part;

	memcpy(&magic_num,buf + buf_ofs,sizeof(unsigned int));
	buf_ofs += sizeof(unsigned int);

	memcpy(&partition_num,buf + buf_ofs,sizeof(unsigned int));
	buf_ofs += sizeof(unsigned int);

	printk("magic_num = %x \n",magic_num);
	printk("partition_num = %d \n",partition_num);
	printk("count = %d \n",count);
#if 0
	partition = kzalloc(sizeof(struct jz_spinand_partition) * partition_num, GFP_KERNEL);
	memcpy(partition,buf + buf_ofs,sizeof(struct jz_spinand_partition) * partition_num);

	int i;
	for(i = 0; i < partition_num;i++){
		printk("name : %s \n",partition[i].name);
		printk("offset : %x \n",partition[i].offset);
		printk("size : %x \n",partition[i].size);
		printk("manager_mode : %x \n",partition[i].manager_mode);
	}
printk(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
#endif
	partition = (struct jz_spinand_partition *)(buf + buf_ofs);

	mtd_part = kzalloc(sizeof(struct mtd_info) * partition_num, GFP_KERNEL);
	///partition = (struct jz_spinand_partition *)(buf + buf_ofs);


	for(i = 0; i < partition_num; i++) {
		mtd_part[i].name = partition[i].name;
		mtd_part[i].size = partition[i].size;
		mtd_part[i].offset = partition[i].offset;
		mtd_part[i].mask_flags = partition[i].mask_flags;
	}

#if 0
	for(i = 0; i < partition_num;i++){
		printk("name : %s \n",lpartition[i].name);
		printk("offset : %x \n",lpartition[i].offset);
		printk("size : %x \n",lpartition[i].size);
		printk("manager_mode : %x \n",lpartition[i].manager_mode);
	}
#endif

	mtd_device_parse_register(&flash->mtd, jz_probe_types, NULL,mtd_part,partition_num);

	return count;
}
struct driver_attribute spinand_attr;
static struct platform_driver jz_sfcdrv = {
	.driver		= {
		.name	= "jz-sfc",
		.owner	= THIS_MODULE,
	},
	.remove		= jz_sfc_remove,
	.suspend	= jz_sfc_suspend,
	.resume		= jz_sfc_resume,
	.shutdown	= jz_sfc_shutdown,
	.probe		= jz_sfc_probe,
};

static int __init jz_sfc_init(void)
{
	int ret;

	//platform_driver_register(&jz_sfcdrv);
	platform_driver_register(&jz_sfcdrv);

	sysfs_attr_init(&spinand_attr.attr);
	//spinand_attr.show = spinand_show;
	spinand_attr.store = spinand_store;
	spinand_attr.attr.name = "spinand_ctl";
	spinand_attr.attr.mode = S_IRUGO | S_IWUSR;
	ret = driver_create_file(&(jz_sfcdrv.driver), &spinand_attr);
	if (ret)
		printk("WARNING(spi nand block): driver_create sys_file error!\n");

	return ret;
}

static void __exit jz_sfc_exit(void)
{
	platform_driver_unregister(&jz_sfcdrv);
}

late_initcall(jz_sfc_init);
module_exit(jz_sfc_exit);
MODULE_LICENSE("GPL");
