/*
 * Flash on CPE mx29lv
 *
 * $Id: cpe-flash.c,v
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <mach/platform/platform_io.h>
#include <mach/platform/pmu.h>
#include <asm/io.h>

static struct mtd_info *mymtd = NULL;

#define BUSWIDTH_8BIT   1
#define BUSWIDTH_16BIT  2

#if defined(CONFIG_PLATFORM_GM8181) || defined(CONFIG_PLATFORM_GM8210_M)
struct map_info physmap_map = {
		.name =		"GM 8-bit flash device",
		.bankwidth = BUSWIDTH_8BIT,
};
#else
struct map_info physmap_map = {
		.name =		"GM 16-bit flash device",
		.bankwidth = BUSWIDTH_16BIT,
};
#endif

#if defined(CONFIG_PLATFORM_GM8181) || defined(CONFIG_PLATFORM_GM8126)
struct mtd_partition flash_parts[] = {

	{
		.name =		"BurnIn",	// for system usage
		.offset	=	0x0,		// Burn-in ,start addr MTD_PA_BASE
		.size =		0x100000	// 1MB
	},	
	{
		.name =		"User",		// free for use
		.offset =	0x100000,	// start addr MTD_PA_BASE + 0x100000
		.size =		0x100000	// 1MB
	},
	{
		.name =		"UBoot",	// for system usage
		.offset	=	0x200000,	// start addr MTD_PA_BASE + 0x200000
		.size =		0x40000	    // 256K
	},
	{
		.name =		"Linux",	// for system usage
		.offset	=	0x240000,	// Kernel/Root FS ,start addr MTD_PA_BASE + 0x240000
		.size =		0xDC0000	// 13.75MB
	}
};
#elif defined(CONFIG_PLATFORM_GM8210_M)
extern int platform_check_nandc(void);
struct mtd_partition flash_parts[] = {
    {
     .name = "Linux Section",           
     .offset = 0x500000,
     .size = 0x500000},
    {
     .name = "User Section",            
     .offset = 0xF00000,
     .size = 0x100000},
    {
     .name = "UBoot Section",         
     .offset = 0,         
     .size = 0x40000},
    {
     .name = "Slave Section",         
     .offset = 0xA0000,          
     .size = 0x60000},
    {
     .name = "Audio Section",        
     .offset = 0x60000,         
     .size = 0x40000},          
    {
     .name = "CFG Section",           // for MAC usage
     .offset = 0x40000,
     .size = 0x20000},  
};

struct mtd_partition big_flash_parts[] = {
    {
     .name = "Linux Section",           
     .offset = 0x500000,
     .size = 0x500000},
    {
     .name = "User Section",            
     .offset = 0x1F00000,
     .size = 0x100000},
    {
     .name = "Loader Section",
     .offset = 0x1000,
     .size = 0x5000},
    {
     .name = "UBoot Section",         
     .offset = 0,         
     .size = 0x22000},
    {
     .name = "Slave Section",         
     .offset = 0x6A000,          
     .size = 0x64000},
    {
     .name = "Audio Section",        
     .offset = 0x2A000,         
     .size = 0x24000},          
    {
     .name = "CFG Section",           // for MAC usage
     .offset = 0x22000,
     .size = 0x20000}, 
};
#define BIG_PARTITION_COUNT (sizeof(big_flash_parts)/sizeof(struct mtd_partition))

#endif /* mtd_partition */

#define PARTITION_COUNT (sizeof(flash_parts)/sizeof(struct mtd_partition))

#ifdef CONFIG_PLATFORM_GM8181	
static pmuReg_t pmu_reg[] = {
    {0x50, 0x3, 0x3, 0, 0}, 
};

static pmuRegInfo_t pmu_reg_info = {
    DRV_NAME, 
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_PLL1,
    &pmu_reg[0]
};
#endif

static int __init init_flagadm(void)
{
	int mode = 0;
	unsigned int FLASH_SIZE = 0, FLASH_PA_ADDR = 0, FLASH_VA_ADDR = 0;

#if defined(CONFIG_PLATFORM_GM8181)
	//calculate flash size, if you use 2 chip or 1 chip is not flash, you can define the size directly
	value = *(volatile unsigned int *)(SRAM_FTSMC010_VA_BASE);	
	i = (value & 0xF0) >> 4;    //read BNK_SIZE
	if (i >= 11)
		FLASH_SIZE = 1 << (i + 4);
	else
		FLASH_SIZE = 1 << (i + 20);

	FLASH_SIZE = FLASH_SIZE * (1 << (value & 0x3));

	/* 00: GPIO[7:6]/SMC pin-out, 10: NAND pin out */
  if (ftpmu010_register_reg(&pmu_reg_info) < 0) {
      printk("SMC: %s fail \n", __FUNCTION__);
      return -1;
  }
  ftpmu010_write_reg(mac_fd, 0x50, 0, 0x3);	
  	
	FLASH_PA_ADDR = (*(volatile unsigned int *)AHBC_FTAHBC010_PA_BASE) & 0x000FFFFF;
	FLASH_VA_ADDR = MTD_VA_BASE;

	mode = (*((unsigned int *)SRAM_FTSMC010_VA_BASE) & 0x3);
	    
#elif defined(CONFIG_PLATFORM_GM8210_M)	
	unsigned int value = 0, i = 0;
	unsigned int smc_va_base_addr, tmp_base_addr;
	
	if (platform_check_flash_type() < 1){
		printk("Pin mux for SPI/NAND Flash\n");
   	return -ENXIO;
	}
	//calculate flash size, if you use 2 chip or 1 chip is not flash, you can define the size directly
	smc_va_base_addr = (unsigned int)ioremap_nocache(SMC_FTSMC010_PA_BASE, SMC_FTSMC010_PA_SIZE);
	value = *(volatile unsigned int *)(smc_va_base_addr);	
	i = (value & 0xF0) >> 4;    //read BNK_SIZE
	if (i >= 11)
		FLASH_SIZE = 1 << (i + 4);
	else
		FLASH_SIZE = 1 << (i + 20);

  FLASH_SIZE = FLASH_SIZE * (1 << (value & 0x3));

  tmp_base_addr = (unsigned int)ioremap_nocache(AHBC_FTAHBC020_PA_BASE, AHBC_FTAHBC020_PA_SIZE);
  FLASH_PA_ADDR = (*(volatile unsigned int *)(tmp_base_addr + 0x10)) & 0xFFF00000;
  FLASH_VA_ADDR = (unsigned int)ioremap_nocache(FLASH_PA_ADDR, FLASH_SIZE);

	mode = value & 0x3;
	      
#endif /* CONFIG_PLATFORM_GM8181 */

	printk(KERN_NOTICE "Flash device: SIZE 0x%x at ADDR 0x%x\n",
			FLASH_SIZE, FLASH_PA_ADDR);

	physmap_map.phys = FLASH_PA_ADDR;
	physmap_map.size = FLASH_SIZE;

  if((mode & 0x3) == 0)
   	   printk("Flash is 8 bit mode\n");
  else if ((mode & 0x3) == 1)
     printk("Flash is 16 bit mode\n");
  else
     printk("This Flash mode not support\n");

  printk("MTD: Init %s: (size=%dMB) start at address=0x%x\n", physmap_map.name, (FLASH_SIZE/1024)/1024, FLASH_VA_ADDR);
	physmap_map.virt = (void __iomem *)FLASH_VA_ADDR;

	if (!physmap_map.virt) {
		printk("MTD: ioremap fail\n");
		return -EIO;
	}

	simple_map_init(&physmap_map);
	
	mymtd = do_map_probe("cfi_probe", &physmap_map);
	if (mymtd) {
		mymtd->owner = THIS_MODULE;
#ifdef CONFIG_PLATFORM_GM8210_M
		if(FLASH_SIZE > (16 * 1024 * 1024))
			mtd_device_register(mymtd, big_flash_parts, BIG_PARTITION_COUNT);
		else
			mtd_device_register(mymtd, flash_parts, PARTITION_COUNT);
#else		
		mtd_device_register(mymtd, flash_parts, PARTITION_COUNT);
#endif	
		printk("MTD flash device initialized\n");
		return 0;
	}
	else
		printk("MTD flash device initialized fail\n");

	return -ENXIO;
}

static void __exit cleanup_flagadm(void)
{
	if (mymtd) {
		mtd_device_unregister(mymtd);
		map_destroy(mymtd);
	}
	if (physmap_map.virt) {
		physmap_map.virt = 0;
	}
}

module_init(init_flagadm);
module_exit(cleanup_flagadm);

MODULE_AUTHOR("GM Corp.");
MODULE_LICENSE("GM License");
