#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <mach/ftpmu010.h>
#include <mach/platform/board.h>
#include <linux/synclink.h>
#include <mach/fmem.h>

#define SYS_CLK 30000000

static void __iomem    *pmu_base_addr = (void __iomem *)NULL;

/* PMU register data */
static int i2c_fd = -1;
#ifdef CONFIG_GPIO_FTGPIO010
static int gpio_fd = -1;
#endif
static int dmac_fd = -1;
static int gmac_fd = -1;
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
    {0x40, (0x1 << 14), (0x1 << 14), (0x1 << 14), (0x1 << 14)}, ///< Enable Schmitt-trigger control on I2C signal
    {0x54, (0xF << 22), (0xF << 22), (0x5 << 22), (0xF << 22)},
    {0xB8, (0x1 << 7),  (0x1 << 7),  (0x0 << 7),  (0x1 << 7)},
};

static pmuRegInfo_t i2c_clk_info = {
    "i2c_clk",
    ARRAY_SIZE(regI2cArray),
    ATTR_TYPE_NONE, /* no clock source */
    regI2cArray
};

/* DMAC
 */
static pmuReg_t regDMACArray[] = {
 /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0xB4, (0x1 << 8), (0x1 << 8), (0x0 << 8), (0x1 << 8)},
};

static pmuRegInfo_t dmac_clk_info = {
    "AHB_DMAC_CLK",
    ARRAY_SIZE(regDMACArray),
    ATTR_TYPE_AHB,
    regDMACArray
};

/* MAC
 */
static pmuReg_t regGMACArray[] = {
 /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0xB4, (0x1 << 11), (0x1 << 11), (0x1 << 11), (0x1 << 11)},
};

static pmuRegInfo_t gmac_clk_info = {
    "GMAC_CLK",
    ARRAY_SIZE(regGMACArray),
    ATTR_TYPE_PLL2,
    regGMACArray
};

/* GPIO
 */
#ifdef CONFIG_GPIO_FTGPIO010
static pmuReg_t regGPIOArray[] = {
	/* reg_off, bit_masks, lock_bits, init_val, init_mask */
	{0xB8, 1 << 16, 1 << 16, 0, 1 << 16}, // FTGPIO010_0
	{0xB8, 1 << 17, 1 << 17, 0, 1 << 17}, // FTGPIO010_1
};
static pmuRegInfo_t gpio_clk_info = {
	"gpio_clk",
	ARRAY_SIZE(regGPIOArray),
	ATTR_TYPE_NONE,             /* no clock source */
	regGPIOArray
};
#endif

static unsigned int pmu_get_chip(void)
{
	static unsigned int ID = 0;
	unsigned int product = 0;

  ID = (ioread32(pmu_base_addr) >> 8) & 0xF;

	switch (ID) {
	  case 0xB:case 0xE:
	    product = 0x8139;
	    break;
	  case 0xA:case 0xD:case 0xF:
      case 0xC:
	    product = 0x8138;
	    break;
	  case 0x5:case 0x6:case 0x7:
	    product = 0x8137;
	    break;
	  default:
	  	printk("Not define this ID\n");
	    break;
	}

	return product;
}

static unsigned int pmu_get_version(void)
{
	static unsigned int version = 0;
	pmuver_t pmuver;
	//unsigned int product;

	/*
	 * Version ID:
	 *     81290: 8139
	 *     81291: 8139 MP
	 */
	if (!version) {
	    //product = (ioread32(pmu_base_addr) >> 16) & 0xFFFF;
	    version = (ioread32(pmu_base_addr) >> 12) & 0xF;

	    printk("IC: GM%x, version: 0x%x \n", pmu_get_chip(), version);
	}

	switch (version) {
      case 0x00:
        pmuver = PMUVER_A;
        break;
      case 0x01:
        pmuver = PMUVER_B;
        break;
      default:
        printk("@@@@@@@@@@@@@@@@@@@@@@@@ CHIP Version: unknown! @@@@@@@@@@@@@@@@@@@@@@@@\n");
        pmuver = PMUVER_UNKNOWN;
        break;
    }

	return (unsigned int)pmuver;
}

/* 0 for system running NAND, -1 for SPI NOR, 1 for SPI NAND */
int platform_check_flash_type(void)
{
	static unsigned int version, clk_data, jmp_data;
	int ret = 0;

	clk_data = ioread32(pmu_base_addr + 0xB4);
	jmp_data = ioread32(pmu_base_addr + 0x4);
	version = pmu_get_version();

	if(version == PMUVER_A){
        if (jmp_data & BIT23) {
    		//clock off	NAND
    		clk_data |= (1 << 16);
    		iowrite32(clk_data, pmu_base_addr + 0xB4);

            ret = -1;
        } else {
            ret = 1;
        }
    } else {
        if (jmp_data & BIT22){
            //clock off	NAND
            clk_data |= (1 << 16);
            iowrite32(clk_data, pmu_base_addr + 0xB4);
            if (jmp_data & BIT23)
                ret = -1;
            else
                ret = 1;
        } else {
            //clock off	SPI
            clk_data |= (1 << 15);
            iowrite32(clk_data, pmu_base_addr + 0xB4);
            ret = 0;
        }
    }
	return ret;
}

/* 0: 3 byte, 1: 4 byte */
int platform_spi_four_byte_mode(void)
{
	static unsigned int version, jmp_data;
	int ret = 0;

	jmp_data = ioread32(pmu_base_addr + 0x4);
	version = pmu_get_version();

	if(version == PMUVER_A){
        if (jmp_data & BIT7)
            ret = 1;
        else
            ret = 0;
    }else{
        if (jmp_data & BIT7)
            ret = 0;
        else
            ret = 1;
    }
	return ret;
}

//format: xxxx_yyyy. xxxx: 8139, yyyy:IC revision
unsigned int pmu_get_chipversion(void)
{
    unsigned int value = (pmu_get_chip() << 16);

    value |= pmu_get_version();

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
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m;

    value = ioread32(pmu_base_addr + 0x30);
    n = (value >> 4) & 0x7F;
    m = (value >> 11) & 0x1F;
    value = (SYS_CLK * n) / m;

    return value;
#endif
}

static inline u32 pmu_read_pll2out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m;

    value = ioread32(pmu_base_addr + 0x34);
    n = (value >> 4) & 0x7F;
    m = (value >> 11) & 0x1F;
    value = (SYS_CLK * n) / m;

    return value;
#endif
}

static inline u32 pmu_read_pll3out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m;

    value = ioread32(pmu_base_addr + 0x34);
    n = (value >> 20) & 0x7F;
    m = (value >> 27) & 0x1F;
		value = (SYS_CLK * n) / m;

    return value;
#endif
}

static unsigned int pmu_get_ahb_clk(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
    value = (value >> 18) & 0x1;

    if(value)
  		value = pmu_read_pll2out() / 4;
  	else
  		value = pmu_read_pll1out() / 4;

    return value;
#endif
}

static unsigned int pmu_get_axi_clk(void)
{
#ifdef CONFIG_FPGA
    return 50000000;
#else
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
    value = (value >> 20) & 0x1;

    if(value)
  		value = pmu_read_pll1out() / 4;
  	else
  		value = pmu_read_pll1out() / 3;

    return value;
#endif
}

unsigned int pmu_get_apb_clk(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    return pmu_get_ahb_clk() / 2;
#endif
}

static unsigned int pmu_get_cpu_clk(void)
{
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
    value = (value >> 22) & 0x1;

    if(pmu_get_chip() == 0x8137)
        value = pmu_read_pll2out() / 2;
    else if(value)
  		value = pmu_read_pll2out() / 2;
  	else
  		value = pmu_read_pll2out();

    return value;
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

static unsigned int pmu_get_id_pin(void)
{
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x0);
    value = (value >> 8) & 0xF;

    return value;
}

struct clock_s
{
    attrInfo_t   clock;
    u32         (*clock_fun)(void);
} clock_info[] = {
    {{"aclk",   ATTR_TYPE_AXI,    0}, pmu_get_axi_clk},
    {{"hclk",   ATTR_TYPE_AHB,    0}, pmu_get_ahb_clk},
    {{"pclk",   ATTR_TYPE_APB0,   0}, pmu_get_apb_clk},
    {{"pll1",   ATTR_TYPE_PLL1,   0}, pmu_read_pll1out},
    {{"pll2",   ATTR_TYPE_PLL2,   0}, pmu_read_pll2out},
    {{"pll3",   ATTR_TYPE_PLL3,   0}, pmu_read_pll3out},
    {{"cpuclk", ATTR_TYPE_CPU,    0}, pmu_get_cpu_clk},
    {{"pmuver", ATTR_TYPE_PMUVER, 0}, pmu_get_version},
    {{"chipver", ATTR_TYPE_CHIPVER, 0}, pmu_get_chipversion},
    {{"pci_epcnt", ATTR_TYPE_EPCNT,   0}, pmu_get_ep_cnt},
    {{"cpu_enum",  ATTR_TYPE_CPUENUM, 0}, pmu_get_cpu_enumator},
    {{"id_pin",  ATTR_TYPE_IDPIN, 0}, pmu_get_id_pin},
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
#ifdef CONFIG_GPIO_FTGPIO010
    /* register GPIO to pmu core */
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

    /* disable MAC clock, wait insmod MAC driver and enable it */
    gmac_fd = ftpmu010_register_reg(&gmac_clk_info);
    if (unlikely(gmac_fd < 0)){
        printk("GMAC registers to PMU fail! \n");
    }
    ftpmu010_deregister_reg(gmac_fd);

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
