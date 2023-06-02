#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <mach/ftpmu010.h>
#include <mach/platform/board.h>
#include <linux/synclink.h>
#include <mach/fmem.h>

#define SYS_CLK 12000000

static void __iomem    *pmu_base_addr = (void __iomem *)NULL;

/* PMU register data */
static int i2c_fd = -1;
//static int gpio_fd = -1;
static int dmac_fd = -1;
/* -----------------------------------------------------------------------
 * Clock GATE table. Note: this table is necessary
 * -----------------------------------------------------------------------
 */
ftpmu010_gate_clk_t gate_tbl[] = {
    /* moduleID,    count,  register (ofs, val, mask) */
    {FTPMU_NONE,    0,      {{0x0,   0x0,        0x0}}} /* END, this row is necessary */
};

/* I2C
 */
static pmuReg_t  regI2cArray[] = {
 /* reg_off  bit_masks  lock_bits     init_val    init_mask */
#ifdef CONFIG_I2C0_IP
    {0xB8, (0x1 << 14), (0x1 << 14), (0x0 << 14), (0x1 << 14)}, /* Disable i2c0 gating clock off */
    {0x44, (0x1 << 12), (0x1 << 12), (0x1 << 12), (0x1 << 12)}, /* Enable i2c0 schmitt trigger */
#endif
#ifdef CONFIG_I2C1_IP
    {0x50, (0x1 << 22), (0x1 << 22), (0x0 << 22), (0x1 << 22)},
    {0xBC, (0x1 << 16), (0x1 << 16), (0x0 << 16), (0x1 << 16)}, // i2c2 gating clock off
#endif
#ifdef CONFIG_I2C2_IP
    {0xBC, (0x1 << 13), (0x1 << 13), (0x0 << 13), (0x1 << 13)}, // i2c2 gating clock off
    {0x44, (0x1 << 17), (0x1 << 17), (0x1 << 17), (0x1 << 17)}, // i2c2 schmitt trigger
    {0x58, (0x3 << 12), (0x3 << 12), (0x1 << 12), (0x3 << 12)}, // i2c2 pin mux, 00b:I2C1, 01b:GPIO[48:49], 10b:I2S5_SCLK/I2S5_FS
#endif

};

static pmuRegInfo_t i2c_clk_info = {
    "i2c_clk",
    ARRAY_SIZE(regI2cArray),
    ATTR_TYPE_NONE, /* no clock source */
    regI2cArray
};

/* GPIO
 */
#if 0 //useless now
static pmuReg_t regGPIOArray[] = {
 /* reg_off  bit_masks  lock_bits     init_val    init_mask */
};

static pmuRegInfo_t gpio_clk_info = {
    "gpio_clk",
    ARRAY_SIZE(regGPIOArray),
    ATTR_TYPE_NONE, /* no clock source */
    regGPIOArray
};
#endif

/* DMAC
 */
static pmuReg_t regDMACArray[] = {
 /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0xB4, (0x1 << 16), (0x1 << 16), (0x0 << 16), (0x1 << 16)},
};

static pmuRegInfo_t dmac_clk_info = {
    "AHB_DMAC_CLK",
    ARRAY_SIZE(regDMACArray),
    ATTR_TYPE_AHB,
    regDMACArray
};

/* 0 for system running NAND, -1 for spi, 1 for SMC */
int platform_check_flash_type(void)
{
	static unsigned int data = 0;
	int ret = 0;

	data = ioread32(pmu_base_addr + 0x4);

    if (data & BIT23)
        ret = -1;
    else
        ret = 0;

	return ret;
}

static unsigned int pmu_get_chip(void)
{
	static unsigned int ID = 0;
	unsigned int product;

  ID = ioread32(pmu_base_addr + 0x100) & 0xF;

	if((ID >> 2) == 0x2)
			product = 0x8287;
	else{
			switch (ID & 0x3) {
		      case 0:
		        product = 0x8282;
		        break;
		      case 1:
		        product = 0x8283;
		        break;
		      case 2:
		        product = 0x8286;
		        break;
		      case 3:
		        product = 0x8287;
		        break;
		      default:
		        break;
		    }
	}
	return product;
}

static unsigned int pmu_get_version(void)
{
	static unsigned int version = 0;
	pmuver_t pmuver;

    /*
	 * Version ID:
	 *     887610: A version
	 *     821011: B version
	 */
	if (!version) {
	    version = (ioread32(pmu_base_addr + 0x100) >> 8) & 0x3;

	    printk("IC: GM%x, version: 0x%x \n", pmu_get_chip(), version);
	}

	switch (version) {
      case 0:
        pmuver = PMUVER_B;
        break;
      case 2:
        pmuver = PMUVER_C;
        break;
      case 3:
        pmuver = 3;//PMUVER_E;
        break;        
      default:
        printk("@@@@@@@@@@@@@@@@@@@@@@@@ CHIP Version: unknown! @@@@@@@@@@@@@@@@@@@@@@@@\n");
        pmuver = PMUVER_UNKNOWN;
        break;
    }

	return (unsigned int)pmuver;
}

//format: xxxx_yyyy. xxxx: 8287, yyyy:IC revision
unsigned int pmu_get_chipversion(void)
{
    unsigned int value;

    value = (pmu_get_chip() << 16) | pmu_get_version();

    if(((ioread32(pmu_base_addr + 0x100) >> 2) & 0x3) == 0x2)
        value |= 0x20;

    return value;
}

/*
 * Local functions
 */
static inline u32 pmu_read_cpumode(void)
{
    return ((readl(pmu_base_addr + 0x30)) >> 16) & 0xffff;
}

static inline u32 pmu_read_pll1out(void)
{
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
    value = (value >> 3) & 0x7F;

    return (SYS_CLK * value);
}

static inline u32 pmu_read_pll2out(void)
{
	printk("pll2 not support\n");

	return 0;
}

static inline u32 pmu_read_pll3out(void)
{
    u32 mul, value = 0;

    if (ioread32(pmu_base_addr + 0x28) & (1 << 15)) {   /* no pll3 */
        printk("PLL3 not support this mode\n");
    } else {
  	    mul = (ioread32(pmu_base_addr + 0x34) >> 4) & 0x7F;
  	    value = (SYS_CLK * mul) / 2;
    }

    return value;
}

static unsigned int pmu_read_pll4out(void)
{
	u32 value;

    value = (ioread32(pmu_base_addr + 0x38) >> 4 ) & 0x7F;

    return (SYS_CLK * value);
}

static unsigned int pmu_read_pll5out(void)
{
    return 750000000;
}

static unsigned int pmu_get_ahb_clk(void)
{
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
    value = (value >> 17) & 0x3;

    switch(value){
    case 0:
  	    value = (pmu_read_pll1out() * 2) / 5;
  	    break;
    case 1:
  		value = pmu_read_pll1out() / 3;
  		break;
    case 2:
  		value = pmu_read_pll1out() / 4;
  		break;
    case 3:
  	    value = pmu_read_pll1out() / 5;
    default:
  		printk("AHB not support this mode\n");
  		break;
	}

    return value;
}

static unsigned int pmu_get_cpu_clk(void)
{
    u32 fclk = 0;

  	fclk = (pmu_read_pll1out() * 2) / 3;

	return fclk;
}

static unsigned int pmu_get_ep_cnt(void)
{
    return 0;
}

static attr_cpu_enum_t pmu_get_cpu_enumator(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    attr_cpu_enum_t retval;

    if (fmem_get_identifier(&pci_id, &cpu_id))
        panic("%s, error! \n", __func__);

    retval = (cpu_id == FMEM_CPU_FA726) ? CPU_RC_FA726 : CPU_RC_FA626;

    return retval;
}

static unsigned int pmu_get_axi_clk(void)
{
    return pmu_get_cpu_clk() / 2;
}

unsigned int pmu_get_apb0_clk(void)
{
    return pmu_get_axi_clk() / 8;
}

unsigned int pmu_get_apb1_clk(void)
{
    return pmu_get_axi_clk() / 2;
}

unsigned int pmu_get_apb2_clk(void)
{
    return pmu_get_ahb_clk() / 4;
}

struct clock_s
{
    attrInfo_t   clock;
    u32         (*clock_fun)(void);
} clock_info[] = {
    {{"aclk",   ATTR_TYPE_AXI,    0}, pmu_get_axi_clk},
    {{"hclk",   ATTR_TYPE_AHB,    0}, pmu_get_ahb_clk},
    {{"pclk0",  ATTR_TYPE_APB0,   0}, pmu_get_apb0_clk},
    {{"pclk1",  ATTR_TYPE_APB1,   0}, pmu_get_apb1_clk},
    {{"pclk2",  ATTR_TYPE_APB2,   0}, pmu_get_apb2_clk},
    {{"pll1",   ATTR_TYPE_PLL1,   0}, pmu_read_pll1out},
    {{"pll3",   ATTR_TYPE_PLL3,   0}, pmu_read_pll3out},
    {{"pll4",   ATTR_TYPE_PLL4,   0}, pmu_read_pll4out},
    {{"pll5",   ATTR_TYPE_PLL5,   0}, pmu_read_pll5out},
    {{"cpuclk", ATTR_TYPE_CPU,    0}, pmu_get_cpu_clk},
    {{"pmuver", ATTR_TYPE_PMUVER, 0}, pmu_get_version},
    {{"chipver", ATTR_TYPE_CHIPVER, 0}, pmu_get_chipversion},
    {{"pci_epcnt", ATTR_TYPE_EPCNT,   0}, pmu_get_ep_cnt},
    {{"cpu_enum",  ATTR_TYPE_CPUENUM, 0}, pmu_get_cpu_enumator},
};

static int pmu_ctrl_handler(u32 cmd, u32 data1, u32 data2)
{
    ATTR_TYPE_T attr = (ATTR_TYPE_T)data1;
    u32 tmp;
    int ret = -1;

    switch (cmd) {
      case FUNC_TYPE_RELOAD_ATTR:
        /* this attribute exists */
        ret = -1;
        if (ftpmu010_get_attr(attr) == 0)
            break;
        for (tmp = 0; tmp < ARRAY_SIZE(clock_info); tmp ++) {
            if (clock_info[tmp].clock.attr_type != attr)
                continue;

            ret = 0;
            if (clock_info[tmp].clock_fun) {
                clock_info[tmp].clock.value = clock_info[tmp].clock_fun();
                ftpmu010_deregister_attr(&clock_info[tmp].clock);
                ret = ftpmu010_register_attr(&clock_info[tmp].clock);
                break;
            }
        }
        break;

      default:
        panic("%s, command 0x%x is unrecognized! \n", __func__, cmd);
        break;
    }

    return ret;
}
static int __init pmu_postinit(void)
{
    int i;

    printk("PMU: Mapped at 0x%x \n", (unsigned int)pmu_base_addr);

    /* calls init function */
    ftpmu010_init(pmu_base_addr, &gate_tbl[0], pmu_ctrl_handler);

    /* register clock */
    for (i = 0; i < ARRAY_SIZE(clock_info); i ++)
    {
        if (clock_info[i].clock_fun)
            clock_info[i].clock.value = clock_info[i].clock_fun();

        ftpmu010_register_attr(&clock_info[i].clock);
    }

    /* register I2C to pmu core */
    i2c_fd = ftpmu010_register_reg(&i2c_clk_info);
    if (unlikely(i2c_fd < 0)){
        printk("I2C registers to PMU fail! \n");
    }
    /* register GPIO to pmu core */
#if 0//please check define
    gpio_fd = ftpmu010_register_reg(&gpio_clk_info);
    if (unlikely(gpio_fd < 0)){
        printk("GPIO registers to PMU fail! \n");
    }
#endif
    /* register DMAC to pmu core */
    dmac_fd = ftpmu010_register_reg(&dmac_clk_info);
    if (unlikely(dmac_fd < 0)){
        printk("AHB DMAC registers to PMU fail! \n");
    }

    return 0;
}

arch_initcall(pmu_postinit);

/* this function should be earlier than any function in this file
 */
void __init pmu_earlyinit(void __iomem *base)
{
    pmu_base_addr = base;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("PMU driver");
