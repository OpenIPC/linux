#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <mach/ftpmu010.h>
#include <mach/platform/board.h>
#include <linux/synclink.h>
#include <mach/ftpmu010_pcie.h>

#define SYS_CLK 12000000

static void __iomem    *pmu_base_addr = (void __iomem *)NULL;

/* PMU register data */
static int gm8312_fd = -1;
static int cpuintr_fd = -1;

/* -----------------------------------------------------------------------
 * Clock GATE table. Note: this table is necessary
 * -----------------------------------------------------------------------
 */
ftpmu010_pcie_gate_clk_t pcie_gate_tbl[] = {
    /* moduleID,    count,  register (ofs, val, mask) */
    {FTPMU_PCIE_NONE,    0,      {{0x0,   0x0,        0x0}}} /* END, this row is necessary */
};

    /* reg_off  bit_masks   lock_bits     init_val    init_mask */
static pmuPcieReg_t reg8312Array[] = {
    /* Enable dmac CLK, ahbc CLK, PCIE_AXI_BRG */
    {0x30, (0x1 << 8) | (0x1 << 10) | (0x1 << 13), (0x1 << 8) | (0x1 << 10) | (0x1 << 13), 0x0, (0x1 << 8) | (0x1 << 10) | (0x1 << 13)},

    /* Enable AXIC_APB_CLK, X2H APB CLK, INTC CLK, H2X APB CLK, PCIE_AXI_BRG, H2X7_APB_CLK */
    {0x34, (0x3 << 1) | (0x1 << 8) | (0xFF << 9) | (0x1 << 16) | (0x3 << 23), (0x3 << 1) | (0x1 << 8) | (0xFF << 9) | (0x1 <<16) |  (0x3 << 23), 0x0, (0x3 << 1) | (0x1 << 8) | (0xFF << 9) | (0x1 << 16) | (0x3 << 23)},
};
static pmuPcieRegInfo_t gm8312_clk_info = {
    "8312_clk",
    ARRAY_SIZE(reg8312Array),
    ATTR_TYPE_PCIE_NONE, /* no clock source */
    reg8312Array
};

static unsigned int pmu_pcie_get_version(void)
{
	static unsigned int version = 0;

	/*
	 * Version ID:
	 *     821010: A version
	 */
	if (!version) {

	    version = ioread32(pmu_base_addr) >> 8;
		printk("IC: GM%04x(%c)\n", ((version >> 8) & 0xffff), (version & 0xff) + 'A' - 0x10);
	}

	return version;
}

/*
 * Local functions
 */
static inline u32 pmu_pcie_read_pll1out(void)
{
    u32 value = 0, pll_ns, pll_ms;

    value = ioread32(pmu_base_addr + 0x08);
    pll_ns = (value >> 2) & 0xFF;
    pll_ms = (value >> 10) & 0x1F;

    return (SYS_CLK * pll_ns / pll_ms);
}

static unsigned int pmu_pcie_get_axi_clk(void)
{
    return pmu_pcie_read_pll1out() / 2;
}

static unsigned int pmu_pcie_get_ahb_clk(void)
{
    return pmu_pcie_read_pll1out() / 2;
}

unsigned int pmu_pcie_get_apb_clk(void)
{
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x34);
    value = (value >> 26) & 0x1F;

    return pmu_pcie_get_ahb_clk() / (value + 1);
}

struct clock_s
{
    attrPcieInfo_t   clock;
    u32         (*clock_fun)(void);
} pcie_clock_info[] = {
    {{"aclk",   ATTR_TYPE_PCIE_AXI,    0}, pmu_pcie_get_axi_clk},
    {{"hclk",   ATTR_TYPE_PCIE_AHB,    0}, pmu_pcie_get_ahb_clk},
    {{"pclk",   ATTR_TYPE_PCIE_APB,    0}, pmu_pcie_get_apb_clk},
    {{"pll1",   ATTR_TYPE_PCIE_PLL1,   0}, pmu_pcie_read_pll1out},
    {{"pmuver", ATTR_TYPE_PCIE_PMUVER, 0}, pmu_pcie_get_version},
};

/* this function is callback from ftpmu010.c */
static int pmu_ctrl_handler(u32 cmd, u32 data1, u32 data2)
{
    ATTR_PCIE_TYPE_T attr = (ATTR_PCIE_TYPE_T)data1;
    u32 irq = data1 - CPU_INT_BASE, tmp;
    int ret = -1;

    switch (cmd) {
      case FUNC_TYPE_INTR_FIRE:
      case FUNC_TYPE_INTR_CLR:
        if (cpuintr_fd == -1) {
            printk("%s, cpuintr pmu is not registered yet! \n", __func__);
            return -1;
        }
        if (irq > 15)
            panic("%s, invalid irq: %d \n", __func__, data1);

        tmp = (cmd == FUNC_TYPE_INTR_CLR) ? 0x1 << (irq + 16) : 0x1 << irq;
        ftpmu010_write_reg(cpuintr_fd, 0xA8, tmp, 0xFFFFFFFF);
        break;

      case FUNC_TYPE_RELOAD_ATTR:
        /* this attribute exists */
        ret = -1;
        if (ftpmu010_get_attr(attr) == 0)
            break;
        for (tmp = 0; tmp < ARRAY_SIZE(pcie_clock_info); tmp ++) {
            if (pcie_clock_info[tmp].clock.attr_type != attr)
                continue;

            ret = 0;
            if (pcie_clock_info[tmp].clock_fun) {
                pcie_clock_info[tmp].clock.value = pcie_clock_info[tmp].clock_fun();
                ftpmu010_pcie_deregister_attr(&pcie_clock_info[tmp].clock);
                ret = ftpmu010_pcie_register_attr(&pcie_clock_info[tmp].clock);
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

static int __init pmu_pcie_postinit(void)
{
    int i;

    printk("PCIE PMU: Mapped at 0x%x \n", (unsigned int)pmu_base_addr);

    /* calls init function */
    ftpmu010_pcie_init(pmu_base_addr, &pcie_gate_tbl[0], pmu_ctrl_handler);

    /* register clock */
    for (i = 0; i < ARRAY_SIZE(pcie_clock_info); i ++)
    {
        if (pcie_clock_info[i].clock_fun)
            pcie_clock_info[i].clock.value = pcie_clock_info[i].clock_fun();

        ftpmu010_pcie_register_attr(&pcie_clock_info[i].clock);
    }

    /* register GM8312 bus to pmu core */
    gm8312_fd = ftpmu010_pcie_register_reg(&gm8312_clk_info);
    if (unlikely(gm8312_fd < 0)){
        printk("GM8312 CLK registers to PMU fail! \n");
    }

    return 0;
}

arch_initcall(pmu_pcie_postinit);

/* this function should be earlier than any function in this file
 */
void __init pmu_pcie_earlyinit(void __iomem *base)
{
    pmu_base_addr = base;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("PMU driver");
