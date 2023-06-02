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

#include "xmsfc_v2.h"

extern const struct xmsfc_params xmsfc_params_table[];
extern const struct xmsfc_params xmsfc_params_default;

static inline void spi_flash_read_ready(u8 dummy_byte)
{
	if(dummy_byte == 0)
	{
		writel(0x04, XM_SFC_V2_CFG1);
		writel(0x02, XM_SFC_V2_CFG2);
	}
	else
	{
		writel(0x0C, XM_SFC_V2_CFG1);
		writel(0x02, XM_SFC_V2_CFG2);
	}

}

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
	writel(0x05, XM_SFC_V2_CFG1);
	writel(0x02, XM_SFC_V2_CFG2);
	writel(0x05, XM_SFC_V2_PST_CMD);
	writel(host->cmd_erase, XM_SFC_V2_CMD);
	while (length)
	{   
		writel(offset, XM_SFC_V2_ADDR);
		writel(FL_OP1, XM_SFC_V2_OP);

		while((readl(XM_SFC_V2_ST) & FL_OP1_OK)  == 0)
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
	int remain = 0;
	int index = 0;

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
	writel(0x05, XM_SFC_V2_CFG1);
	writel(0x02, XM_SFC_V2_CFG2);
	writel(0x05, XM_SFC_V2_PST_CMD);
	writel(host->cmd_write, XM_SFC_V2_CMD);

	while(len > 0)
	{
		remain = (to % XM_SFC_V2_BUF_SIZE);
		num = ((len >= XM_SFC_V2_BUF_SIZE - remain) ?  XM_SFC_V2_BUF_SIZE - remain : len);
		
		writel(to, XM_SFC_V2_ADDR);
		writel(num - 1, XM_SFC_V2_RW_NUM);

		index = 0;
		while(index < num)
		{
			writel(*(unsigned int *)(ptr + index), XM_SFC_V2_RAM + index);
			index += 4;
		}

		writel(FL_OP4, XM_SFC_V2_OP);
		while((readl(XM_SFC_V2_ST) & FL_OP4_OK)  == 0);

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
	spi_flash_read_ready(host->dummy_byte);

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

static int xmsfc_read_cmd(u8 cmd, void *data, size_t data_len)
{
	if(cmd == SPI_NOR_CMD_READ_JEDECID)
	{   
		writel(0x00, XM_SFC_V2_CFG1);
		writel(0x00, XM_SFC_V2_CFG2);
		writel(SPI_NOR_CMD_READ_JEDECID, XM_SFC_V2_CMD);
		writel(2, XM_SFC_V2_RW_NUM);
		writel(FL_OP3, XM_SFC_V2_OP);

		udelay(1);
		while((readl(XM_SFC_V2_ST) & FL_OP3_OK)  == 0);

		*((u32*)data) = readl(XM_SFC_V2_R_SR_ID) & 0x00FFFFFF;
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

	gpio_write(GPIO_DRIVE_8MA | GPIO_MUX2_EN, GPIO_FLASH_CLK);
	gpio_write(GPIO_DRIVE_8MA | GPIO_MUX2_EN, GPIO_FLASH_CSN);
	gpio_write(GPIO_DRIVE_8MA | GPIO_MUX2_EN, GPIO_FLASH_DI);
	gpio_write(GPIO_DRIVE_8MA | GPIO_MUX2_EN, GPIO_FLASH_DO);
	gpio_write(GPIO_DRIVE_8MA | GPIO_MUX2_EN, GPIO_FLASH_WPN);
	gpio_write(GPIO_DRIVE_8MA | GPIO_MUX2_EN, GPIO_FLASH_HOLD);

	writel(readl(XM_SFC_V2_CTRL) | (1 << 7), XM_SFC_V2_CTRL);

	if(xmsfc_read_cmd(SPI_NOR_CMD_READ_JEDECID, &jedec, sizeof(jedec)))
	{
		 printk(KERN_ERR "XMSFCV2: Failed to get idcodes\n");
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
		printk(KERN_WARNING "XMSFCV2: Unsupported flash IDs: %#x using default", jedec);
		params = &xmsfc_params_default;
	}
	else
	{
		printk(KERN_INFO "XMSFCV2: flash name:%s size:%#x\n", params->name, params->size);
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

	if(readl(XM_SFC_V2_MODE_ST) == 0)
	{
		host->cmd_erase = params->cmd_erase;
		host->cmd_write = params->cmd_write;
		host->cmd_read  = params->cmd_read;
		host->dummy_byte = 0;
	}
	else //目前只支持 地址1线quad spi
	{
		host->cmd_erase = params->cmd_erase;
		host->cmd_write = params->cmd_quad_write;
		host->cmd_read  = params->cmd_quad_read;
		host->dummy_byte = 1;
	}

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
