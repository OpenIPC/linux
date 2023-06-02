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
    {0x68, (0x1 << 6), (0x1 << 6), (0x0 << 6), (0x1 << 6)},
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
    {0x60, (0x3 << 21), (0x3 << 21), (0x3 << 21), (0x3 << 21)},
    {0x6C, (0x3 << 19), (0x3 << 19), (0x3 << 19), (0x3 << 19)},
    {0x70, (0x3 << 11), (0x3 << 11), (0x3 << 11), (0x3 << 11)},
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
	{0x74, 1 << 3, 1 << 3, 0, 1 << 3}, // FTGPIO010_0
	{0x74, 1 << 4, 1 << 4, 0, 1 << 4}, // FTGPIO010_1
	{0x74, 1 << 5, 1 << 5, 0, 1 << 5}, // FTGPIO010_2
	{0x74, 1 << 6, 1 << 6, 0, 1 << 6}, // FTGPIO010_3
	{0x74, 1 << 7, 1 << 7, 0, 1 << 7}, // FTGPIO010_4
	{0x74, 1 << 8, 1 << 8, 0, 1 << 8}, // FTGPIO010_5
	{0x74, 1 << 9, 1 << 9, 0, 1 << 9}, // FTGPIO010_6
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
	unsigned int product = 0x8220;

	return product;
}

static unsigned int pmu_get_version(void)
{
	static unsigned int version = 0;
	pmuver_t pmuver;
	//unsigned int product;

	/*
	 * Version ID:
	 *     81360: 8136
	 *     81361: 8136 B
	 */
	if (!version) {
	    //product = (ioread32(pmu_base_addr) >> 16) & 0xFFFF;
	    version = (ioread32(pmu_base_addr) >> 8) & 0xF;

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
	static unsigned int jmp_data;
	int ret = 0;

	jmp_data = ioread32(pmu_base_addr + 0x4);

    if (jmp_data & BIT4)
        ret = -1;
    else
        ret = 1;

	return ret;
}

/* 0: 3 byte, 1: 4 byte */
int platform_spi_four_byte_mode(void)
{
	static unsigned int jmp_data;
	int ret = 0;

	jmp_data = ioread32(pmu_base_addr + 0x4);

    if (jmp_data & BIT5)
        ret = 0;
    else
        ret = 1;

	return ret;
}

//format: xxxx_yyyy. xxxx: 8136, yyyy:IC revision
unsigned int pmu_get_chipversion(void)
{
    unsigned int value = (pmu_get_chip() << 16);

    value |= pmu_get_version();

    return value;
}

uint pmu_read_pll0out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    uint value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x30);
    frange = (value >> 4) & 0x3;   
    n = (value >> 16) & 0xF;
    m = (value >> 8) & 0xF;
    value = (SYS_CLK * n) / m;

	switch (frange) {
	  case 0:
	    value /= 8;
	    break;
	  case 1:
	    value /= 4;
	    break;
	  case 2:
	    value /= 2;
	    break;
	  default:
	    break;
	}

    return value;
#endif
}

uint pmu_read_pll1out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    uint value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x34);
    n = (value >> 16) & 0xF;
    m = (value >> 8) & 0xF;
    value = (pmu_read_pll0out() * n) / m;

    return value;
#endif
}

uint pmu_read_pll2out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x38);
    frange = (value >> 4) & 0x3;
    n = (value >> 16) & 0x7F;
    m = (value >> 8) & 0x1F;
    value = (SYS_CLK * n) / m;    

	switch (frange) {
	  case 0:
	    value /= 8;
	    break;
	  case 1:
	    value /= 4;
	    break;
	  case 2:
	    value /= 2;
	    break;
	  default:
	    break;
	}

    return value;
#endif
}

uint pmu_read_pll3out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x3C);
    n = (value >> 16) & 0x7F;
    m = (value >> 8) & 0x1F;
	value = (SYS_CLK * n) / m;

    return value;
#endif
}

uint pmu_read_pll4out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x40);
    n = (value >> 16) & 0x1FF;
    m = (value >> 8) & 0x1F;
	value = (SYS_CLK * n) / m;

    return value;
#endif
}

uint pmu_read_pll5out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x44);
    frange = (value >> 4) & 0x3;
    n = (value >> 16) & 0x7F;
    m = (value >> 8) & 0x1F;
	value = (SYS_CLK * n) / m;

	switch (frange) {
	  case 0:
	    value /= 8;
	    break;
	  case 1:
	    value /= 4;
	    break;
	  case 2:
	    value /= 2;
	    break;
	  default:
	    break;
	}

    return value;
#endif
}

uint pmu_read_pll6out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    return 20000000;
#endif
}

uint pmu_read_pll7out(void)
{
#ifdef CONFIG_FPGA
    return 20000000;
#else
    u32 value, n, m, frange;

    value = readl(CONFIG_PMU_BASE + 0x34);
    n = (value >> 16) & 0xFF;
    m = (value >> 8) & 0x1F;
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
    value = (value >> 18) & 0x3;

	switch (value) {
	  case 0:
	    value = pmu_read_pll1out() / 3;
	    break;
	  case 1:
	    value = pmu_read_pll2out() / 3;
	    break;
	  case 2:case 3:
	    value = pmu_read_pll2out() / 4;
	    break;
	  default:
	    break;
	}

    return value;
#endif
}

static unsigned int pmu_get_axi0_clk(void)
{
#ifdef CONFIG_FPGA
    return 50000000;
#else
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
    value = (value >> 20) & 0x3;

	switch (value) {
	  case 0:
	    value = pmu_read_pll1out() / 3;
	    break;
	  case 1:
	    value = pmu_read_pll2out() / 3;
	    break;
	  case 2:case 3:
	    value = pmu_read_pll2out() / 4;
	    break;
	  default:
	    break;
	}

    return value;
#endif
}

static unsigned int pmu_get_axi1_clk(void)
{
    return pmu_get_axi0_clk();
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
    return pmu_read_pll1out();
}

struct clock_s
{
    attrInfo_t   clock;
    u32         (*clock_fun)(void);
} clock_info[] = {
    {{"aclk0",  ATTR_TYPE_AXI0,   0}, pmu_get_axi0_clk},
    {{"aclk1",  ATTR_TYPE_AXI1,   0}, pmu_get_axi1_clk},
    {{"hclk",   ATTR_TYPE_AHB,    0}, pmu_get_ahb_clk},
    {{"pclk",   ATTR_TYPE_APB,    0}, pmu_get_apb_clk},
    {{"pclk0",  ATTR_TYPE_APB0,   0}, pmu_get_apb_clk},
    {{"pll1",   ATTR_TYPE_PLL1,   0}, pmu_read_pll1out},
    {{"pll2",   ATTR_TYPE_PLL2,   0}, pmu_read_pll2out},
    {{"pll3",   ATTR_TYPE_PLL3,   0}, pmu_read_pll3out},
    {{"pll4",   ATTR_TYPE_PLL4,   0}, pmu_read_pll4out},
    {{"pll5",   ATTR_TYPE_PLL5,   0}, pmu_read_pll5out},
    {{"pll6",   ATTR_TYPE_PLL6,   0}, pmu_read_pll6out},
    {{"pll7",   ATTR_TYPE_PLL7,   0}, pmu_read_pll7out},
    {{"cpuclk", ATTR_TYPE_CPU,    0}, pmu_get_cpu_clk},
    {{"pmuver", ATTR_TYPE_PMUVER, 0}, pmu_get_version},
    {{"chipver", ATTR_TYPE_CHIPVER, 0}, pmu_get_chipversion},
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
