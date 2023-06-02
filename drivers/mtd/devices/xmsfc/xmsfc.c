#include <linux/init.h>                     
#include <linux/module.h>                   
#include <linux/device.h>                   
#include <linux/interrupt.h>                
#include <linux/slab.h>                     
#include <linux/mtd/mtd.h>                  
#include <linux/mtd/partitions.h>           
#include <linux/delay.h>                    
#include <linux/sched.h>                    
#include <asm/setup.h>                      
#include <linux/io.h>                       
#include <linux/kernel.h>                   
#include <linux/platform_device.h>       
#include "../../mtdcore.h"

#include "xmsfc.h"

extern const struct xmsfc_params xmsfc_params_table[];
extern const struct xmsfc_params xmsfc_params_default;

static int xmsfc_reg_erase(struct mtd_info *mtd, struct erase_info *instr) 
{
	unsigned long long offset = instr->addr;    
	unsigned long long length = instr->len;
	unsigned int timeout = 0x10000000;
	struct xmsfc_host *host = MTD_TO_HOST(mtd);

	if (offset + length > mtd->size) {
		DBG_MSG("erase area out of range of mtd.\n");
		return -EINVAL;
	}    

	if ((unsigned int)offset & (mtd->erasesize-1)) {
		DBG_MSG("erase start address is not alignment.\n");
		return -EINVAL;
	}    
	//
	if ((unsigned int)length & (mtd->erasesize-1)) {
		DBG_MSG("erase length is not alignment.\n");
		return -EINVAL;
	}    

	mutex_lock(&host->lock);
	while (length)
	{   
		writel(offset, XMSFC_REG_ADDR);
		writel(host->cmd_erase, XMSFC_REG_CMD);
		writel(XMSFC_OP2, XMSFC_REG_OP);

		while((readl(XMSFC_REG_ST) & XMSFC_OP2_OK)  == 0)
		{
			if(--timeout == 0)
			{
				instr->state = MTD_ERASE_FAILED;
				mutex_unlock(&host->lock);
				return -EIO;
			}
		} 

		//this command for MPW
		writel(0x05,XMSFC_REG_CMD);
		writel(XMSFC_OP3, XMSFC_REG_OP);
		while((readl(XMSFC_REG_ST) & XMSFC_OP3_OK)  == 0)
		{
			if(--timeout == 0)
			{
				instr->state = MTD_ERASE_FAILED;
				mutex_unlock(&host->lock);
				return -EIO;
			}
		} 

		offset += mtd->erasesize; 
		length -= mtd->erasesize;
	}   
	instr->state = MTD_ERASE_DONE;
	mutex_unlock(&host->lock);
	mtd_erase_callback(instr);

	return 0;
}

static int xmsfc_reg_write(struct mtd_info *mtd, loff_t to, size_t len,
		    size_t *retlen, const u_char *buf)
{
	struct xmsfc_host *host = MTD_TO_HOST(mtd);
	unsigned char *ptr = (unsigned char *)buf;
	int num = 0;
	int index = 0;
	int remain = 0;

	if ((to + len) > mtd->size) {
		DBG_MSG("write data out of range.\n");
		return -EINVAL;
	}

	*retlen = 0;
	if (!len) {
		DBG_MSG("write length is 0.\n");
		return 0;
	}
	mutex_lock(&host->lock);

	while(len > 0)
	{
		remain = (to % XMSFC_REG_BUF_SIZE);
		num = ((len >= XMSFC_REG_BUF_SIZE - remain) ?  XMSFC_REG_BUF_SIZE - remain : len);
		
		writel(to, XMSFC_REG_ADDR);
		writel(num - 1, XMSFC_REG_WRNUM);

		index = 0;
		while(index < num)
		{
			writel(*(ptr + index), XMSFC_REG_BUF + 4 * index);
			index++;
		}
		writel(host->cmd_write, XMSFC_REG_CMD);
		writel(XMSFC_OP6, XMSFC_REG_OP);
		while((readl(XMSFC_REG_ST) & XMSFC_OP6_OK)  == 0);

		//this command for MPW
		writel(0x05,XMSFC_REG_CMD);
		writel(XMSFC_OP3, XMSFC_REG_OP);
		while((readl(XMSFC_REG_ST) & XMSFC_OP3_OK)  == 0);

		to  += num; 
		ptr += num;
		len -= num;  
	}
	*retlen = (size_t)(ptr - buf);

	mutex_unlock(&host->lock);
	return 0;
}
static int xmsfc_bus_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)  
{
	int num;
	int result = -EIO;
	unsigned char *ptr = buf;
	struct xmsfc_host *host = MTD_TO_HOST(mtd);

	if ((from + len) > mtd->size) {
		DBG_MSG("read area out of range.\n");
		return -EINVAL;
	}

	*retlen = 0;
	if (!len) {
		DBG_MSG("read length is 0.\n");
		return 0;
	}

	mutex_lock(&host->lock);

	while (len > 0)
	{
		//num = ((from + len) >= spi->chipsize) ? (spi->chipsize - from) : len;
		num = len;
		memcpy(ptr, (char *)host->iobase + from, num);
		from += num;
		ptr  += num;
		len  -= num;
	}
	*retlen = (size_t)(ptr - buf);
	result = 0;

	mutex_unlock(&host->lock);
	return result;
}


int xmsfc_entry_quad_1addr(const struct xmsfc_params *params)
{
	unsigned int value = 0;

	// read st
	writel(XMSFC_CMD_READ_ST2, XMSFC_REG_CMD);
	writel(0, XMSFC_REG_RW_SR_BSEL);
	writel(XMSFC_OP3, XMSFC_REG_OP);
	while((readl(XMSFC_REG_ST) & XMSFC_OP3_OK)  == 0);
	value = readl(XMSFC_REG_SRR);
	
	writel((value << 8) | 0x0200 , XMSFC_REG_SRW);
	writel(XMSFC_CMD_WRITE_ST, XMSFC_REG_CMD); 
	writel(1, XMSFC_REG_RW_SR_BSEL);
	writel(XMSFC_OP4, XMSFC_REG_OP);
	while((readl(XMSFC_REG_ST) & XMSFC_OP4_OK)  == 0);

	writel(XMSFC_CMD_READ_ST2, XMSFC_REG_CMD);
	writel(0, XMSFC_REG_RW_SR_BSEL);
	writel(XMSFC_OP3, XMSFC_REG_OP);
	while((readl(XMSFC_REG_ST) & XMSFC_OP3_OK)  == 0);
	value = readl(XMSFC_REG_SRR);

	if((value & 0x02) == 0)
	{
		return -1;
	}

	writel(params->cmd_read, XMSFC_REG_CACHE_CMD);
	writel(0x00, XMSFC_REG_MODE);
	writel(0x01, XMSFC_REG_MODE);
	while((readl(XMSFC_REG_MODE_ST) & 0x01)  == 0);

	return 0;
}



static int xmsfc_read_cmd(u8 cmd, void *data, size_t data_len)
{
	if(cmd == XMSFC_CMD_READ_JEDECID)
	{   
		writel(XMSFC_CMD_READ_JEDECID, XMSFC_REG_CMD);
		writel(2, XMSFC_REG_RW_SR_BSEL);
		writel(XMSFC_OP3, XMSFC_REG_OP);

		udelay(1);
		while((readl(XMSFC_REG_ST) & XMSFC_OP3_OK)  == 0);

		*((u32*)data) = readl(XMSFC_REG_SRR) & 0x00FFFFFF;
	}   
	return 0;
}

static int  xmsfc_driver_probe(struct platform_device * plat_dev)  
{
	struct mtd_info   *mtd;
	int nr_parts = 0;    
	struct xmsfc_host *host;
	const struct xmsfc_params *params; 
	struct mtd_partition *parts = NULL;
	static char const *part_probes[] = {       
		"cmdlinepart",                         
		NULL,                                  
	};          

	unsigned int jedec = 0;

	if(xmsfc_read_cmd(XMSFC_CMD_READ_JEDECID, &jedec, sizeof(jedec)))
	{
		 printk(KERN_ERR "XMSFC: Failed to get idcodes\n");
		 return -ENODEV;
	}
	
	params = xmsfc_params_table;
	for (; params->name != NULL; params++)
	{   
		if ((params->jedec) == jedec)
			break;
	}   
	if (!params->name) 
	{   
		printk(KERN_WARNING "XMSFC: Unsupported flash IDs: %#x using default", jedec);
		params = &xmsfc_params_default;
	}
	else
	{
		printk(KERN_INFO "XMSFC: flash name:%s size:%#x\n", params->name, params->size);
	}

	if((params->flags & FLAG_QUAD_ONE_ADDR) == 1)
	{
		if(xmsfc_entry_quad_1addr(params))
			return -ENODEV;
		printk(KERN_INFO "XMSFC: flash entry quad one addr\n");
	}

	host = kmalloc(sizeof(struct xmsfc_host), GFP_KERNEL);
	if (!host)  
		return -ENOMEM;    
	memset(host, 0, sizeof(struct xmsfc_host));
	platform_set_drvdata(plat_dev, host);

	host->iobase = ioremap_nocache(XMSF_BASE_ADDR, XMSF_BASE_LEN);
	if (!host->iobase) {
		printk(KERN_ERR "spi buffer ioremap failed.\n");
		goto fail;
	}    
	host->cmd_erase = params->cmd_erase;
	host->cmd_write = params->cmd_write;
	host->cmd_read  = params->cmd_read;

	mutex_init(&host->lock); 
	mtd = host->mtd;
	mtd->name = (char *)plat_dev->name;
	mtd->type = MTD_NORFLASH;  
	mtd->writesize = 1;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->owner = THIS_MODULE; 
	mtd->_erase = xmsfc_reg_erase;
	mtd->_write = xmsfc_reg_write;
	mtd->_read  = xmsfc_bus_read;
	mtd->size = params->size;
	mtd->erasesize = params->erasesize;

	nr_parts = parse_mtd_partitions(mtd, part_probes, &parts, 0); 

	return mtd_device_register(mtd, parts, nr_parts) == 1 ? -ENODEV : 0;

fail:
	if (host->iobase)
		iounmap(host->iobase);
	kfree(host);
	platform_set_drvdata(plat_dev, NULL);
	return -EIO;
}
static int  xmsfc_driver_remove(struct platform_device * plat_dev)   
{
	struct xmsfc_host *host = platform_get_drvdata(plat_dev);

	mtd_device_unregister(host->mtd);

	if (host->iobase)
		iounmap(host->iobase);

	kfree(host);
	platform_set_drvdata(plat_dev, NULL);

	return 0;
}

static void xmsfc_driver_shutdown(struct platform_device *pltdev) 
{

}

static struct platform_driver xmsfc_driver_pltdrv = {
	.probe      = xmsfc_driver_probe,
	.remove     = xmsfc_driver_remove,
	.shutdown   = xmsfc_driver_shutdown,
	.driver.name    = "xm_sfc",
	.driver.owner   = THIS_MODULE,
	.driver.bus = &platform_bus_type,
};

static struct platform_device xmsfc_device_pltdev = {
	.name           = "xm_sfc",
	.id             = -1,
};

static int __init xmsfc_module_init(void)
{
	int result = 0; 

	printk(KERN_DEBUG "XM Spi Flash Controller Device Driver Version 1.0\n");

	result = platform_driver_register(&xmsfc_driver_pltdrv);
	if (result < 0) 
		return result;

	result = platform_device_register(&xmsfc_device_pltdev);
	if (result < 0) { 
		platform_driver_unregister(&xmsfc_driver_pltdrv);
		return result;
	}    

	return result;
}

static void __exit xmsfc_module_exit(void)
{
	platform_device_unregister(&xmsfc_device_pltdev);
	platform_driver_unregister(&xmsfc_driver_pltdrv);
}

module_init(xmsfc_module_init);
module_exit(xmsfc_module_exit);

MODULE_LICENSE("GPL");
