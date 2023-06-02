#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <mach/ftpmu010.h>
#ifdef CONFIG_GM8312
#include <mach/ftpmu010_pcie.h>
#include <mach/platform/board.h>
#endif

#define SYS_CLK 12000000

static void __iomem    *pmu_base_addr = (void __iomem *)NULL;

/* PMU register data */
static int dmac_fd = -1;
static int cpuintr_fd = -1;
#ifdef CONFIG_PLATFORM_AXIDMA
static int xdmac_fd = -1;
#endif

int ftpmu010_h264e_fd = -1;
int ftpmu010_h264d_fd = -1;
int ftpmu010_mcp100_fd = -1;
/* -----------------------------------------------------------------------
 * Clock GATE table. Note: this table is necessary
 * -----------------------------------------------------------------------
 */
ftpmu010_gate_clk_t gate_tbl[] = {
    /* moduleID,    count, register (ofs, val, mask) */
    {FTPMU_H264E_0, 2,     {{0xB0, (0x0 << 25),  (0x1 << 25)}, {0xB0, (0x0 << 3),  (0x1 << 3)}}},
    {FTPMU_H264E_1, 2,     {{0xB4, (0x0 << 30),  (0x1 << 30)}, {0xB4, (0x0 << 8),  (0x1 << 8)}}},   /* mcp280off */
    {FTPMU_H264D_0, 2,     {{0xB0, (0x0 << 24),  (0x1 << 24)}, {0xB0, (0x0 << 2),  (0x1 << 2)}}},
    {FTPMU_H264D_1, 2,     {{0xB4, (0x0 << 29),  (0x1 << 29)}, {0xB4, (0x0 << 3),  (0x1 << 3)}}},   /* mcp300off */    
    {FTPMU_MCP100_0,1,     {{0xB4, (0x0 << 12),  (0x1 << 12)}}},   /* mcp100off */
    {FTPMU_3DI_0,   2,     {{0xB0, (0x0 << 22),  (0x1 << 22)}, {0xB0, (0x0 << 15), (0x1 << 15)}}},  /* di3d0off */
  	{FTPMU_SCALER_0,2,     {{0xB0, (0x0 << 26),  (0x1 << 26)}, {0xB0, (0x0 << 4),  (0x1 << 4)}}},  /* scaroff */
    {FTPMU_NONE,    0,     {{0x0,   0x0,         0x0}}} /* END, this row is necessary */
};

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

/* CPU INTR
 */
static pmuReg_t regCPUINTArray[] = {
 /* reg_off  bit_masks   lock_bits     init_val    init_mask */
    {0xA8,   0xFFFFFFFF, 0xFFFFFFFF,   0x0,        0x0},
};

static pmuRegInfo_t cpu_intr_info = {
    "CPU_INTR",
    ARRAY_SIZE(regCPUINTArray),
    ATTR_TYPE_NONE, /* no clock source */
    regCPUINTArray
};

#ifdef CONFIG_PLATFORM_AXIDMA
/* AXI DMAC 
 */
static pmuReg_t regXDMACArray[] = {
 /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0xB0, (0x1 << 19), (0x1 << 19), (0x0 << 19), (0x1 << 19)},
};
static pmuRegInfo_t xdmac_clk_info = {
    "AXI_DMAC_CLK", 
    ARRAY_SIZE(regXDMACArray),
    ATTR_TYPE_AXI, 
    regXDMACArray
};
#endif

/* H264E
 */
static pmuReg_t regH264EArray[] = {
    /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0x28,      (0x1 << 31), (0x1 << 31),  (0x0 << 31), (0x0 << 31)},
    {0xA0,      (0x3 << 1), (0x3 << 1),  (0x0 << 1), (0x0 << 1)},   
    {0xB0,      (0x1 << 25), (0x1 << 25),  (0x0 << 25), (0x0 << 25)},  
    {0xB0,      (0x1 << 3), (0x1 << 3),  (0x0 << 3), (0x0 << 3)},         
    {0xB4,      (0x1 << 30), (0x1 << 30),  (0x0 << 30), (0x0 << 30)},
    {0xB4,      (0x1 << 8), (0x1 << 8),  (0x0 << 8), (0x0 << 8)},    
};
static pmuRegInfo_t h264e_clk_info = {
    "H264E_CLK", 
    ARRAY_SIZE(regH264EArray),
    ATTR_TYPE_PLL4,
    regH264EArray
};

/* H264D
 */
static pmuReg_t regH264DArray[] = {
    /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0x28,      (0x1 << 30), (0x1 << 30),  (0x0 << 30), (0x0 << 30)},
    {0xA0,      (0x3 << 3), (0x3 << 3),  (0x0 << 3), (0x0 << 3)},   
    {0xB0,      (0x1 << 24), (0x1 << 24),  (0x0 << 24), (0x0 << 24)},  
    {0xB0,      (0x1 << 2), (0x1 << 2),  (0x0 << 2), (0x0 << 2)},         
    {0xB4,      (0x1 << 29), (0x1 << 29),  (0x0 << 29), (0x0 << 29)},
    {0xB4,      (0x1 << 7), (0x1 << 7),  (0x0 << 7), (0x0 << 7)},    
};
static pmuRegInfo_t h264d_clk_info = {
    "H264D_CLK", 
    ARRAY_SIZE(regH264DArray),
    ATTR_TYPE_PLL4,
    regH264DArray
};

/* MCP100
 */
static pmuReg_t regMCP100Array[] = {
    /* reg_off  bit_masks  lock_bits     init_val    init_mask */
    {0x28,      (0x1 << 16), (0x1 << 16),  (0x0 << 16), (0x0 << 16)},
    {0xA0,      (0x1 << 16), (0x1 << 16),  (0x0 << 16), (0x0 << 16)},   
    {0xB4,      (0x1 << 12), (0x1 << 12),  (0x0 << 12), (0x0 << 12)},
};
static pmuRegInfo_t mcp100_clk_info = {
    "MCP100_CLK", 
    ARRAY_SIZE(regMCP100Array),
    ATTR_TYPE_PLL1,
    regMCP100Array
};

static unsigned int pmu_get_version(void)
{
	static unsigned int version = 0;
	int inc = 0;

	/*
	 * Version ID:
	 *     821010: A version
	 */
	if (!version) {
	    
	    version = ioread32(pmu_base_addr) >> 8;
	    
	    switch ((ioread32(pmu_base_addr) >> 5) & 0x7) 
	    {		
			case 0 : inc = 0; break;
			case 2 : inc = 1; break;
			case 6 : inc = 2; break;
			case 7 : inc = 3; break;
			default: inc = 0; break;
		}
		if ((version & 0xf0) == 0x10) {
			/* 8210 series */
			printk("IC: GM%04x(%c)\n", ((version >> 8) & 0xffff) + inc, (version & 0xff) + 'A' - 0x10);
		} else {
			printk("IC: ID not be found\n");
		}
	}

	return version;
}

/*
 * Local functions
 */
static inline u32 pmu_read_cpumode(void)
{
    return (ioread32(pmu_base_addr + 0x30) >> 16);
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
    printk("GM8210 not use PLL2\n"); 
    return 0;
}

static unsigned int pmu_read_pll4out(void)
{
    return 768000000;
}

static unsigned int pmu_read_pll3out(void)
{
    u32 mul, value = 0;
  
    if(ioread32(pmu_base_addr + 0x28) & (1 << 15)) {
        printk("PLL3 not support this mode\n");
    }else{
  	mul = (ioread32(pmu_base_addr + 0x34) >> 4) & 0xFF;
  	value = SYS_CLK * mul / 2;
    }
    
    return value * 1000000;
}

static unsigned int pmu_read_pll5out(void)
{
    return 100000000;
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
  	break;
    default:
  	printk("AHB not support this mode\n");
  	break;  	  	
	}  
	  	
    return value;
}

static unsigned int pmu_get_axi_clk(void)
{
    u32 value = 0;

    value = ioread32(pmu_base_addr + 0x30);
  
    if(value & (1 << 19))
  	value = pmu_read_pll1out() / 3;
    else
  	value = (pmu_read_pll1out() * 2) / 5;
	
    return value;
}

unsigned int pmu_get_apb0_clk(void)
{
    return 20000000;//pmu_get_axi_clk() / 2;
}

unsigned int pmu_get_apb1_clk(void)
{
    return 20000000;//pmu_get_axi_clk() / 8;
}

unsigned int pmu_get_apb2_clk(void)
{
    return 20000000;//pmu_get_ahb_clk() / 4;
}

static unsigned int pmu_get_cpu_clk(void)//FA626
{
    u32 value = 0;
  
    value = pmu_read_pll1out() * 2 / 3;
  	 
    return value;
}

struct clock_s
{
    attrInfo_t   clock;
    u32         (*clock_fun)(void);
} clock_info[] = {
    {{"aclk",   ATTR_TYPE_AXI,    0}, pmu_get_axi_clk},    
    {{"hclk",   ATTR_TYPE_AHB,    0}, pmu_get_ahb_clk}, 
    //{{"pclk",   ATTR_TYPE_APB,    0}, pmu_get_apb_clk},
    {{"pclk0",  ATTR_TYPE_APB0,   0}, pmu_get_apb0_clk},
    {{"pclk1",  ATTR_TYPE_APB1,   0}, pmu_get_apb1_clk},
    {{"pclk2",  ATTR_TYPE_APB2,   0}, pmu_get_apb2_clk},
    {{"pll1",   ATTR_TYPE_PLL1,   0}, pmu_read_pll1out},
    {{"pll2",   ATTR_TYPE_PLL2,   0}, pmu_read_pll2out},
    {{"pll3",   ATTR_TYPE_PLL3,   0}, pmu_read_pll3out},
    {{"pll4",   ATTR_TYPE_PLL4,   0}, pmu_read_pll4out},
    {{"pll5",   ATTR_TYPE_PLL5,   0}, pmu_read_pll5out},
    {{"cpuclk", ATTR_TYPE_CPU,    0}, pmu_get_cpu_clk},
    {{"pmuver", ATTR_TYPE_PMUVER, 0}, pmu_get_version},
};

/* this function is callback from ftpmu010.c */
static int pmu_ctrl_handler(u32 cmd, u32 data1, u32 data2)
{
    ATTR_TYPE_T attr = (ATTR_TYPE_T)data1;
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
        if (ftpmu010_get_attr(attr) == 0)
            break;

        /* this attribute exists */
        if (clock_info[attr].clock_fun) {
            clock_info[attr].clock.value = clock_info[attr].clock_fun();
            ftpmu010_deregister_attr(&clock_info[attr].clock);
            ret = ftpmu010_register_attr(&clock_info[attr].clock);
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
        
    /* register DMAC to pmu core */
    dmac_fd = ftpmu010_register_reg(&dmac_clk_info);
    if (unlikely(dmac_fd < 0)){
        printk("AHB DMAC registers to PMU fail! \n");
    }

    cpuintr_fd = ftpmu010_register_reg(&cpu_intr_info);
    if (unlikely(cpuintr_fd < 0)){
        printk("CPU INTR registers to PMU fail! \n");
    }
    
#ifdef CONFIG_PLATFORM_AXIDMA
    /* register AXI DMAC to pmu core */
    xdmac_fd = ftpmu010_register_reg(&xdmac_clk_info);
    if (unlikely(xdmac_fd < 0)){
        printk("AXI DMAC registers to PMU fail! \n");
    }
#endif 

#ifdef CONFIG_GM8312
    ftpmu010_pcie_init((void *)PCIPMU_FTPMU010_VA_BASE);
#endif

    ftpmu010_h264e_fd = ftpmu010_register_reg(&h264e_clk_info);
    if (unlikely(ftpmu010_h264e_fd < 0)){
        printk("H264 Enc registers to PMU fail! \n");
    }

    ftpmu010_h264d_fd = ftpmu010_register_reg(&h264d_clk_info);
    if (unlikely(ftpmu010_h264d_fd < 0)){
        printk("H264 Dec registers to PMU fail! \n");
    }
        
    ftpmu010_mcp100_fd = ftpmu010_register_reg(&mcp100_clk_info);
    if (unlikely(ftpmu010_mcp100_fd < 0)){
        printk("MCP100 registers to PMU fail! \n");
    }
          
    return 0;
}

EXPORT_SYMBOL(ftpmu010_h264e_fd);
EXPORT_SYMBOL(ftpmu010_h264d_fd);
EXPORT_SYMBOL(ftpmu010_mcp100_fd);

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
