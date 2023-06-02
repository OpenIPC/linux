#include <linux/module.h>
#include <linux/mmc/host.h>

#include <mach/ftpmu010.h>
#include "sdhci-pltfm.h"

#if defined(CONFIG_PLATFORM_GM8210)
#include <mach/fmem.h>
#endif

#define DRIVER_NAME "ftsdc021"

/* Transfer Mode
 * Note: Only one of them can be chosen */
#define Transfer_Mode_ADMA 1
//#define Transfer_Mode_SDMA 1
//#define Transfer_Mode_PIO 1

//ftsdc021_pulse_latch: 00h ~ 3Fh -> Latch value at 00h ~ 3Fh sdclk1x rising after the SCLK edge
uint ftsdc021_pulse_latch = 0;
module_param(ftsdc021_pulse_latch, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ftsdc021_pulse_latch, "Sdhc Vendor defined Register0(offset = 0x100)[13:8]: value of Pulse Latch Offset");

static int ftsdc021_enable_dma(struct sdhci_host *host)
{
	return 0;
}

static unsigned int ftsdc021_get_timeout_clk(struct sdhci_host *host)
{
	return 33;
}

static unsigned int ftsdc021_get_max_clk(struct sdhci_host *host)
{
	/* Controller does not specify the base clock frequency.
	 * Current design base clock  = SD ODC frequency x 2.
	 */
#if defined(CONFIG_PLATFORM_GM8210)
	if (ftpmu010_read_reg(0x28) & BIT(26))
		return ftpmu010_get_attr(ATTR_TYPE_AXI) / 4;
	return ftpmu010_get_attr(ATTR_TYPE_AHB) / 2;
#endif
#if defined(CONFIG_PLATFORM_GM8287)
    /* [28, 26] : sdcclk_sel(sdcclk_2x) 0x-hclk/2, 10-pll1out3/4, 11-pll1out4/4 */
	if (ftpmu010_read_reg(0x28) & BIT(28)) {
        if (ftpmu010_read_reg(0x28) & BIT(26))
            return (ftpmu010_get_attr(ATTR_TYPE_PLL1) * 10 / 25 / 4); // pll1out3 = pll1 / 2.5
        else
            return (ftpmu010_get_attr(ATTR_TYPE_PLL1) / 3 / 4); // pll1out4 = pll1 / 3
    } else {
		return ftpmu010_get_attr(ATTR_TYPE_AHB) / 2;
    }
#endif
#if defined(CONFIG_PLATFORM_GM8139)
    unsigned int clk;
	if (ftpmu010_read_reg(0x28) & BIT(11))
        clk = ftpmu010_get_attr(ATTR_TYPE_PLL2) / 4;
    else
        clk = ftpmu010_get_attr(ATTR_TYPE_PLL1) / 2;
    clk /= ((ftpmu010_read_reg(0x70) & 0x7) + 1);
    return clk / 2;
#endif
#if defined(CONFIG_PLATFORM_GM8136)
    unsigned int clk;
    if (ftpmu010_read_reg(0x28) & BIT(7))
        clk = ftpmu010_get_attr(ATTR_TYPE_PLL1) / 2;
    else
        clk = ftpmu010_get_attr(ATTR_TYPE_PLL2);
    clk /= ((ftpmu010_read_reg(0x70) & 0x7) + 1);
    return clk / 2;
#endif
}
static struct sdhci_ops ftsdc021_ops = {
	.enable_dma = ftsdc021_enable_dma,
	.get_max_clock	= ftsdc021_get_max_clk,
	.get_timeout_clock = ftsdc021_get_timeout_clk,
};

/* PMU register data */
static int sdc_fd = -1;
int sdhci_get_fd(void) {return sdc_fd;}
EXPORT_SYMBOL(sdhci_get_fd);
static pmuReg_t regSDCArray[] = {
	/* reg_off, bit_masks, lock_bits, init_val, init_mask */
#if defined(CONFIG_PLATFORM_GM8210)
	{0x28, BIT(26), BIT(26), BIT(26), BIT(26)}, /* sdcclk_sel */
	{0x44, 0x1E00000, 0x1E00000, 0, 0x1E00000}, /* BIT21~BIT24 */
	{0xB4, BIT(19), BIT(19), 0, BIT(19)},       /* FTSDC021 clk */
#endif
#if defined(CONFIG_PLATFORM_GM8287)
	{0x28, BIT(28), BIT(28), 0, BIT(28)},       /* sdcclk_sel */
	{0x44, 0x1E00000, 0x1E00000, 0, 0x1E00000}, /* BIT21~BIT24 */
	{0xB4, BIT(19), BIT(19), 0, BIT(19)},       /* FTSDC021 clk */
#endif
#if defined(CONFIG_PLATFORM_GM8139)
	{0x58, 0xfc03fc, 0, 0x540154, 0xfc03fc},    /* sd pinmux setting */
	{0x28, 0x880, 0x0, 0x880, 0x880},           /* sdcclk_sel(BIT11), BIT7 for nand pinmux */
	{0x70, 0x7, 0x7, 0x0, 0x7},                 /* sdcclk divided value */
	{0x40, 0x70, 0x70, 0x20, 0x70},             /* sdc Driving Capacity and Slew Rate, 10mA */
	{0xB4, BIT(14), BIT(14), 0, BIT(14)},       /* FTSDC021 clk */
#endif
#if defined(CONFIG_PLATFORM_GM8136)
	{0x28, 0x80, 0x0, 0, 0x80},                 /* sdcclk_sel(BIT7) */
	{0x70, 0x7, 0x7, 0x2, 0x7},                 /* sdcclk divided value */
	{0x40, 0x70, 0x70, 0x20, 0x70},             /* sdc Driving Capacity and Slew Rate, 10mA */
    #if defined(CONFIG_SDC0_IP)
        {0x58, 0xfc03fc, 0, 0x540154, 0xfc03fc},    /* sd pinmux setting */
	{0xB4, BIT(14), BIT(14), 0, BIT(14)},       /* FTSDC021 clk */
    #endif
    #if defined(CONFIG_SDC1_IP)
        /* Shared pins with CAP0_D5 ~ CAP0_D11 */ 
        {0x50, 0xf0000000, 0, 0xf0000000, 0xf0000000},    /* sd#1 pinmux setting */
        {0x54, 0xfcf, 0, 0xfcf, 0xfcf},                   /* sd#1 pinmux setting */
        /* Shared pins with GPIO_1[22:19], GPIO_1[31:29] */
        //{0x58, 0x3f000, 0, 0x2a000, 0x3f000},             /* sd#1 pinmux setting */
        //{0x5C, 0xff000000, 0, 0x55000000, 0xff000000},    /* sd#1 pinmux setting */
        {0xB4, BIT(22), BIT(22), 0, BIT(22)},             /* FTSDC021#1 clk */
    #endif
#endif

};
static pmuRegInfo_t sdc_clk_info = {
	"sdc_clk",
	ARRAY_SIZE(regSDCArray),
	ATTR_TYPE_NONE,             /* no clock source */
	regSDCArray
};
static void ftsdc021_hw_init(void)
{
	sdc_fd = ftpmu010_register_reg(&sdc_clk_info);
	if (unlikely(sdc_fd < 0)) {
		panic("SDC registers to PMU fail!");
	}
}

static void ftsdc021_hw_remove(void)
{
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id != FMEM_CPU_FA726 || pci_id != FMEM_PCI_HOST)
        return;
#endif
#if defined(CONFIG_PLATFORM_GM8287)
	/* disable ftsdc021 clk	*/
	if (ftpmu010_write_reg(sdc_fd, 0xB4, BIT(19), BIT(19)) < 0)
		printk(KERN_ERR "Write PMU register 0xB4 Failed\n");
#endif
#if defined(CONFIG_PLATFORM_GM8139)
	/* disable ftsdc021 clk	*/
	if (ftpmu010_write_reg(sdc_fd, 0xB4, BIT(14), BIT(14)) < 0)
		printk(KERN_ERR "Write PMU register 0xB4 Failed\n");
#endif
#if defined(CONFIG_PLATFORM_GM8136)
    #if defined(CONFIG_SDC0_IP)
	/* disable ftsdc021 clk	*/
	if (ftpmu010_write_reg(sdc_fd, 0xB4, BIT(14), BIT(14)) < 0)
		printk(KERN_ERR "Write PMU register 0xB4 Failed\n");
    #endif
    #if defined(CONFIG_SDC1_IP)
	/* disable ftsdc021#1 clk	*/
	if (ftpmu010_write_reg(sdc_fd, 0xB4, BIT(22), BIT(22)) < 0)
		printk(KERN_ERR "Write PMU register 0xB4 Failed\n");
    #endif
#endif
	/* unregister sdc from pmu core */
	if (ftpmu010_deregister_reg(sdc_fd) < 0)
		printk(KERN_ERR "Unregister SDC from PMU Failed\n");
}

static struct sdhci_pltfm_data ftsdc021_pdata = {
	.ops = &ftsdc021_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
		  SDHCI_QUIRK_BROKEN_TIMEOUT_VAL
#if defined(Transfer_Mode_PIO)
		  | SDHCI_QUIRK_BROKEN_DMA |
		  SDHCI_QUIRK_BROKEN_ADMA,
#elif defined(Transfer_Mode_SDMA)
		  | SDHCI_QUIRK_FORCE_DMA |
		  SDHCI_QUIRK_BROKEN_ADMA,
#elif defined(Transfer_Mode_ADMA)
		 ,
#endif
};
static int __devinit ftsdc021_probe(struct platform_device *pdev)
{
	ftsdc021_hw_init();
	return sdhci_pltfm_register(pdev, &ftsdc021_pdata);
}

static int __devexit ftsdc021_remove(struct platform_device *pdev)
{
	ftsdc021_hw_remove();
	return sdhci_pltfm_unregister(pdev);
}

static struct platform_driver ftsdc021_driver = {
	.probe = ftsdc021_probe,
	.remove = ftsdc021_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DRIVER_NAME,
	},
};

static int __init ftsdc021_driver_init(void)
{
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id != FMEM_CPU_FA726 || pci_id != FMEM_PCI_HOST)
        return 0;
#endif
    return platform_driver_register(&ftsdc021_driver);
}
module_init(ftsdc021_driver_init);
static void __exit ftsdc021_driver_exit(void)
{
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id != FMEM_CPU_FA726 || pci_id != FMEM_PCI_HOST)
        return;
#endif
    platform_driver_unregister(&ftsdc021_driver);
}
module_exit(ftsdc021_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MikeYeh <mikeyeh@faraday-tech.com>");
MODULE_DESCRIPTION("FTSDC021 driver");
