#include <linux/init.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <asm/io.h>
#include <soc/ddr.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <ccu.h>

#include "pm.h"
#include "pm_fastboot.h"

static char *wakup_source[4]={"NULL","Alarm clock wake up","Input Key wake up","Alarm&Key wake up"};

static int save_ddr_auto_value = 0;

static int fastboot_resume_code[] = {
#include "fastboot_resume_code.hex"
};

/* 230109010:
 * date:230109
 * change:1.0
 * */
static unsigned int zboost_version=230109010;

static struct store_regs *store_regs;
static noinline void fastboot_cpu_resume(void);
static noinline void fastboot_cpu_sleep(void);
static inline unsigned int ddr_read_reg(unsigned int reg);
static inline void cpu_sync(void)
{
    __asm__ volatile(
                     ".set push     \n\t"
                     ".set mips32r2 \n\t"
                     "sync          \n\t"
                     "lw $0,0(%0)   \n\t"
                     "nop           \n\t"
                     ".set pop      \n\t"
                     ::"r" (0xb0003000));

}

void dump_rtc(void)
{
	printk ("******************************ingenic_rtc_dump**********************\n\n");
	printk ("ingenic_rtc_dump-----RTC_RTCCR is --%08x--\n",rtc_read_reg(0xb0003000));
	printk ("ingenic_rtc_dump-----RTC_RTCSR is --%08x--\n",rtc_read_reg(0xb0003004));
	printk ("ingenic_rtc_dump-----RTC_RTCSAR is --%08x--\n",rtc_read_reg(0xb0003008));
	printk ("ingenic_rtc_dump-----RTC_RTCGR is --%08x--\n",rtc_read_reg(0xb000300c));
	printk ("ingenic_rtc_dump-----RTC_HCR is --%08x--\n",rtc_read_reg(0xb0003020));
	printk ("ingenic_rtc_dump-----RTC_HWFCR is --%08x--\n",rtc_read_reg(0xb0003024));
	printk ("ingenic_rtc_dump-----RTC_HRCR is --%08x--\n",rtc_read_reg(0xb0003028));
	printk ("ingenic_rtc_dump-----RTC_HWCR is --%08x--\n",rtc_read_reg(0xb000302c));
	printk ("ingenic_rtc_dump-----RTC_HWRSR is --%08x--\n",rtc_read_reg(0xb0003030));
	printk ("ingenic_rtc_dump-----RTC_HSPR is --%08x--\n",rtc_read_reg(0xb0003034));
	printk ("ingenic_rtc_dump-----RTC_WENR is --%08x--\n",rtc_read_reg(0xb000303c));
	printk ("ingenic_rtc_dump-----RTC_WKUPPINCR is --%08x--\n",rtc_read_reg(0xb0003048));
	printk ("***************************ingenic_rtc_dump***************************\n");
}


static void rtc_ram_write_enable(void)
{
	unsigned int tmp;

	/* write RTC RAM enable */
	tmp = rtc_read_reg(0xb0003048);
	tmp &= ~(1 << 23);
	tmp &= ~(1 << 22);
	tmp |= (1 << 21);
	tmp |= (1 << 20);
	rtc_write_reg(0xb0003048, tmp);

}

static void rtc_ram_write_disable(void)
{
	unsigned int tmp;
	/*exit write RTC RAM enable */
	tmp = rtc_read_reg(0xb0003048);
	tmp |= (1 << 22);
	rtc_write_reg(0xb0003048, tmp);

}


static void ost_save(struct ost_regs *ost_regs)
{
	ost_regs->ostccr = *(volatile unsigned int *)0xb2100000;
	ost_regs->oster = *(volatile unsigned int *)0xb2100004;
	ost_regs->ostcr = *(volatile unsigned int *)0xb2100008;
	ost_regs->ostfr = *(volatile unsigned int *)0xb210000c;
	ost_regs->ostmr = *(volatile unsigned int *)0xb2100010;
	ost_regs->ostdfr = *(volatile unsigned int *)0xb2100014;
	ost_regs->ostcnt = *(volatile unsigned int *)0xb2100018;

	ost_regs->g_ostccr = *(volatile unsigned int *)0xb2000000;
	ost_regs->g_oster = *(volatile unsigned int *)0xb2000004;
	ost_regs->g_ostcr = *(volatile unsigned int *)0xb2000008;
	ost_regs->g_ostcnth = *(volatile unsigned int *)0xb200000c;
	ost_regs->g_ostcntl = *(volatile unsigned int *)0xb2000010;
	ost_regs->g_ostcntb = *(volatile unsigned int *)0xb2000014;

}

static void ost_restore(struct ost_regs *ost_regs)
{
	*(volatile unsigned int *)0xb2100000 = ost_regs->ostccr;
	*(volatile unsigned int *)0xb2100008 = ost_regs->ostcr;
	*(volatile unsigned int *)0xb210000c = ost_regs->ostfr;
	*(volatile unsigned int *)0xb2100010 = ost_regs->ostmr;
	*(volatile unsigned int *)0xb2100014 = ost_regs->ostdfr;
	*(volatile unsigned int *)0xb2100018 = ost_regs->ostcnt;
	*(volatile unsigned int *)0xb2100004 = ost_regs->oster;

	*(volatile unsigned int *)0xb2000000 = ost_regs->g_ostccr;
	*(volatile unsigned int *)0xb2000008 = ost_regs->g_ostcr;
	*(volatile unsigned int *)0xb2000004 = ost_regs->g_oster;

}

static void cpm_save(struct cpm_regs *cpm_regs)
{
	cpm_regs->cpsppr = *(volatile unsigned int *)0xb0000038;
	cpm_regs->cpspr = *(volatile unsigned int *)0xb0000034;
	cpm_regs->lcr = *(volatile unsigned int *)0xb0000004;
	cpm_regs->clkgr0 = *(volatile unsigned int*)0xb0000020;
	cpm_regs->clkgr1 = *(volatile unsigned int*)0xb0000028;
	cpm_regs->mestsel = *(volatile unsigned int*)0xb00000ec;
	cpm_regs->srbc = *(volatile unsigned int*)0xb00000c4;
	cpm_regs->opcr = *(volatile unsigned int*)0xb0000024;
}

static void cpm_restore(struct cpm_regs *cpm_regs)
{
	*(volatile unsigned int *)0xb0000038 = cpm_regs->cpsppr;
	*(volatile unsigned int *)0xb0000034 = cpm_regs->cpspr;
	*(volatile unsigned int *)0xb0000004 = cpm_regs->lcr;
	*(volatile unsigned int *)0xb00000ec = cpm_regs->mestsel;
	*(volatile unsigned int *)0xb00000c4 = cpm_regs->srbc;
	*(volatile unsigned int *)0xb0000024 = cpm_regs->opcr;
	*(volatile unsigned int *)0xb0000020 = cpm_regs->clkgr0;
	*(volatile unsigned int *)0xb0000028 = cpm_regs->clkgr1;
}

struct ost_regs *ost_regs;
struct cpm_regs *cpm_regs;
void sys_save(void)
{
	ost_regs = kmalloc(sizeof(struct ost_regs), GFP_KERNEL);
	cpm_regs = kmalloc(sizeof(struct cpm_regs), GFP_KERNEL);
	ost_save(ost_regs);
	cpm_save(cpm_regs);
}

void sys_restore(void)
{
	cpm_restore(cpm_regs);
	ost_restore(ost_regs);

	kfree(ost_regs);
	kfree(cpm_regs);
}






static void save_resume_pc(void)
{
	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	store_regs->resume_pc = (unsigned int)fastboot_cpu_resume;
}

static void save_uart_index(void)
{
	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	store_regs->uart_index = bc_idx;
	store_regs->version = zboost_version;
}

static void pll_store(void)
{
	struct pll_resume_reg *pll_resume_reg;

	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	pll_resume_reg = &store_regs->pll_resume_reg;

	pll_resume_reg->cpccr = *(volatile unsigned int *)0xb0000000;
	pll_resume_reg->cppcr = *(volatile unsigned int *)0xb000000c;
	pll_resume_reg->cpapcr = *(volatile unsigned int *)0xb0000010;
	pll_resume_reg->cpmpcr = *(volatile unsigned int *)0xb0000014;
	pll_resume_reg->cpvpcr = *(volatile unsigned int *)0xb00000e0;
	pll_resume_reg->ddrcdr = *(volatile unsigned int *)0xb000002c;
}

static void ddrc_store(void)
{
	struct ddrc_resume_reg *ddrc_resume_reg;

	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	ddrc_resume_reg = &store_regs->ddrc_resume_reg;

	ddrc_resume_reg->dcfg = *(volatile unsigned int *)0xb34f0008;
	ddrc_resume_reg->dctrl = *(volatile unsigned int *)0xb34f0010;
	ddrc_resume_reg->dlmr = *(volatile unsigned int *)0xb34f0018;
	ddrc_resume_reg->ddlp = *(volatile unsigned int *)0xb34f0020;
	ddrc_resume_reg->dasr_en = *(volatile unsigned int *)0xb34f0030;
	ddrc_resume_reg->dasr_cnt = *(volatile unsigned int *)0xb34f0028;
	ddrc_resume_reg->drefcnt = *(volatile unsigned int *)0xb34f0038;
	ddrc_resume_reg->dtimming1 = *(volatile unsigned int *)0xb34f0040;
	ddrc_resume_reg->dtimming2 = *(volatile unsigned int *)0xb34f0048;
	ddrc_resume_reg->dtimming3 = *(volatile unsigned int *)0xb34f0050;
	ddrc_resume_reg->dtimming4 = *(volatile unsigned int *)0xb34f0058;
	ddrc_resume_reg->dtimming5 = *(volatile unsigned int *)0xb34f0060;
	ddrc_resume_reg->dmmap0 = *(volatile unsigned int *)0xb34f0078;
	ddrc_resume_reg->dmmap1 = *(volatile unsigned int *)0xb34f0080;

	ddrc_resume_reg->dbwcfg = *(volatile unsigned int *)0xb34f0088;
	ddrc_resume_reg->dbwstp = *(volatile unsigned int *)0xb34f0090;
	ddrc_resume_reg->hregpro = *(volatile unsigned int *)0xb34f00d8;
	ddrc_resume_reg->dbgen = *(volatile unsigned int *)0xb34f00e0;

	ddrc_resume_reg->dwcfg = *(volatile unsigned int *)0xb3012000;
	ddrc_resume_reg->dremap1 = *(volatile unsigned int *)0xb3012008;
	ddrc_resume_reg->dremap2 = *(volatile unsigned int *)0xb301200c;
	ddrc_resume_reg->dremap3 = *(volatile unsigned int *)0xb3012010;
	ddrc_resume_reg->dremap4 = *(volatile unsigned int *)0xb3012014;
	ddrc_resume_reg->dremap5 = *(volatile unsigned int *)0xb3012018;

	ddrc_resume_reg->cpac = *(volatile unsigned int *)0xb301201c;
	ddrc_resume_reg->cchc0 = *(volatile unsigned int *)0xb3012020;
	ddrc_resume_reg->cchc1 = *(volatile unsigned int *)0xb3012024;
	ddrc_resume_reg->cchc2 = *(volatile unsigned int *)0xb3012028;
	ddrc_resume_reg->cchc3 = *(volatile unsigned int *)0xb301202c;
	ddrc_resume_reg->cchc4 = *(volatile unsigned int *)0xb3012030;
	ddrc_resume_reg->cchc5 = *(volatile unsigned int *)0xb3012034;
	ddrc_resume_reg->cchc6 = *(volatile unsigned int *)0xb3012038;
	ddrc_resume_reg->cchc7 = *(volatile unsigned int *)0xb301203c;
	ddrc_resume_reg->cschc0 = *(volatile unsigned int *)0xb3012040;
	ddrc_resume_reg->cschc1 = *(volatile unsigned int *)0xb3012044;
	ddrc_resume_reg->cschc2 = *(volatile unsigned int *)0xb3012048;
	ddrc_resume_reg->cschc3 = *(volatile unsigned int *)0xb301204c;
	ddrc_resume_reg->cmonc0 = *(volatile unsigned int *)0xb3012050;
	ddrc_resume_reg->cmonc1 = *(volatile unsigned int *)0xb3012054;
	ddrc_resume_reg->cmonc2 = *(volatile unsigned int *)0xb3012058;
	ddrc_resume_reg->cmonc3 = *(volatile unsigned int *)0xb301205c;
	ddrc_resume_reg->cmonc4 = *(volatile unsigned int *)0xb3012060;

	ddrc_resume_reg->ccguc0 = *(volatile unsigned int *)0xb3012064;
	ddrc_resume_reg->ccguc1 = *(volatile unsigned int *)0xb3012068;

	ddrc_resume_reg->pregpro = *(volatile unsigned int *)0xb301206c;
	ddrc_resume_reg->bufcfg = *(volatile unsigned int *)0xb3012070;
}

static void ddr_phy_store(void)
{
	struct ddr_phy_resume_reg *ddr_phy_resume_reg;

	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	ddr_phy_resume_reg = &store_regs->ddr_phy_resume_reg;

    ddr_phy_resume_reg->mem_cfg         = *(volatile unsigned int *)0xb3011004;
    ddr_phy_resume_reg->dq_width        = *(volatile unsigned int *)0xb3011034;
    ddr_phy_resume_reg->cl              = *(volatile unsigned int *)0xb3011014;
    ddr_phy_resume_reg->al              = *(volatile unsigned int *)0xb3011018;
    ddr_phy_resume_reg->cwl             = *(volatile unsigned int *)0xb301101c;
    ddr_phy_resume_reg->pll_fbdivh      = *(volatile unsigned int *)0xb3011144;
    ddr_phy_resume_reg->pll_fbdivl      = *(volatile unsigned int *)0xb3011140;
    ddr_phy_resume_reg->pll_ctrl        = *(volatile unsigned int *)0xb301114c;
    ddr_phy_resume_reg->pll_pdiv        = *(volatile unsigned int *)0xb3011148;
    ddr_phy_resume_reg->training_ctrl   = *(volatile unsigned int *)0xb3011008;
    if(save_ddr_auto_value == 0){
        ddr_phy_resume_reg->calib_delay_al  = *(volatile unsigned int *)0xb301126c;
        ddr_phy_resume_reg->calib_bypass_al = *(volatile unsigned int *)0xb3011270;
        ddr_phy_resume_reg->calib_delay_ah  = *(volatile unsigned int *)0xb3011274;
        ddr_phy_resume_reg->calib_bypass_ah = *(volatile unsigned int *)0xb3011278;
    }
    ddr_phy_resume_reg->wl_mode1        = *(volatile unsigned int *)0xb301100c;
    ddr_phy_resume_reg->wl_mode2        = *(volatile unsigned int *)0xb3011010;
}


static void ccu_save(void)
{
    struct ccu_regs *ccu_regs = NULL;
	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	ccu_regs = &store_regs->cpu_resume_reg;

	ccu_regs->cfcr      = *(volatile unsigned int *)0xb2200fe0;//must write bit31
	ccu_regs->dmir      = *(volatile unsigned int *)0xb2200fc0;
	ccu_regs->mscr      = *(volatile unsigned int *)0xb2200060;
	ccu_regs->pimr      = *(volatile unsigned int *)0xb2200120;
	ccu_regs->mimr      = *(volatile unsigned int *)0xb2200160;
	ccu_regs->oimr      = *(volatile unsigned int *)0xb22001a0;
	//ccu_regs->dipr    = *(volatile unsigned int *)0xb22001c0;//only read reg
	ccu_regs->gdimr     = *(volatile unsigned int *)0xb22001e0;
	ccu_regs->ldimr0    = *(volatile unsigned int *)0xb2200300;
	ccu_regs->ldimr1    = *(volatile unsigned int *)0xb2200320;
	ccu_regs->rer       = *(volatile unsigned int *)0xb2200f00;
	ccu_regs->mbr0      = *(volatile unsigned int *)0xb2201000;
	ccu_regs->mbr1      = *(volatile unsigned int *)0xb2201004;

}



static noinline void fastboot_cpu_resume(void)
{


	*(volatile unsigned int *)0xb2200f00 = 0xbfc00000; /* RESET entry = 0xbfc00000 ,reset value */

	__asm__ volatile(
		".set push	\n\t"
		".set mips32r2	\n\t"
		"jr.hb %0	\n\t"
		"sync		\n\t"
		"nop		\n\t"
		".set pop 	\n\t"
		:
		: "r" (restore_goto)
		:
		);
}

static void rtc_ram_store(void)
{
	struct ddr_phy_resume_reg *ddr_phy_resume_reg;

	store_regs = (struct store_regs *)FASTBOOT_DATA_ADDR;
	ddr_phy_resume_reg = &store_regs->ddr_phy_resume_reg;

	pll_store();
	ddrc_store();
	ddr_phy_store();
    ccu_save();
	save_resume_pc();
	save_uart_index();
}

void load_func_to_rtc_ram(void)
{
   	rtc_ram_write_enable();

#ifdef DEBUG_PM
    printk("sleep addr  :%#x -- len :%#x -- end :%#x\n \
            data addr   :%#x -- len :%#x -- end :%#x -- actual:%d bytes\n \
            resume addr :%#x -- len :%#x -- end :%#x -- actual:%d bytes\n",
            FASTBOOT_SLEEP_CODE_ADDR, FASTBOOT_SLEEP_CODE_LEN, FASTBOOT_SLEEP_CODE_ADDR + FASTBOOT_SLEEP_CODE_LEN,
            FASTBOOT_DATA_ADDR, FASTBOOT_DATA_LEN, FASTBOOT_DATA_ADDR+FASTBOOT_DATA_LEN, sizeof(struct store_regs),
            FASTBOOT_RESUME_CODE1_ADDR, FASTBOOT_RESUME_CODE_LEN, FASTBOOT_RESUME_CODE1_ADDR + FASTBOOT_RESUME_CODE_LEN ,sizeof(fastboot_resume_code)
            );
#endif
    if(save_ddr_auto_value == 0){
        memset((unsigned int *)RTC_MEMORY_START, 0xff, 4096);
    }
	memcpy((unsigned int *)FASTBOOT_RESUME_CODE1_ADDR, (unsigned int *)fastboot_resume_code, sizeof(fastboot_resume_code));

	rtc_ram_store();
	rtc_ram_write_disable();
    save_ddr_auto_value = 1;

#ifdef DEBUG_PM
    dump_rtc();
#endif
}

void load_func_to_oram(void)
{
	load_func_to_tcsm(FASTBOOT_SLEEP_CODE_ADDR, (unsigned int *)fastboot_cpu_sleep, FASTBOOT_SLEEP_CODE_LEN);
}

static inline unsigned int ddr_read_reg(unsigned int reg)
{
    return ddr_readl(reg);
}
static inline void ddr_write_reg(unsigned int reg,unsigned int val)
{
    do{
        ddr_writel(val,reg);
    }while(val != ddr_read_reg(reg));
}

static noinline void fastboot_cpu_sleep(void)
{
    unsigned int ddrc_ctrl = 0,tmp;

	blast_dcache32();
	blast_scache64();
	__sync();
	__fast_iob();
    /*enter self refresh status,disabled auto self refresh*/
    ddr_write_reg(DDRC_HREGPRO,0);
    ddr_write_reg(DDRC_AUTOSR_EN,0);//disenable auto self-refresh

    tmp = *(volatile unsigned int*)(0xa0000000);//touch

    ddrc_ctrl = ddr_read_reg(DDRC_CTRL);
    ddrc_ctrl |= 0x1<<5;//If set HIGH, hardware drives DRAM device entering self-refresh mode;
    ddr_write_reg(DDRC_CTRL,ddrc_ctrl);
    while(!(ddr_read_reg(DDRC_STATUS) & (1<<2)));//If set, DDR memory is in self refresh status.


    for (tmp = 0; tmp < 2; tmp++)
    {
        __asm__ volatile("nop\t\n");
        __asm__ volatile("nop\t\n");
    }
    /* bufferen_core = 0 */
    tmp = rtc_read_reg(0xb0003048);
    tmp &= ~(1 << 21);
    rtc_write_reg(0xb0003048, tmp);

    /* dfi_init_start = 1 */
    *(volatile unsigned int *)0xb3012000 |= (1 << 3);
    /*delay nop*/
    for (tmp = 0; tmp < 10; tmp++)
    {
        __asm__ volatile("nop\t\n");
        __asm__ volatile("nop\t\n");
    }
    /* ddr phy pll power down */
    tmp = ddr_read_reg(DDRP_INNOPHY_PLL_CTRL);
    tmp |= (1<<3);//CMD and DQ PLL power down. Set to “0” to enable the PLL.
    ddr_write_reg(DDRP_INNOPHY_PLL_CTRL,tmp);
    /*printf notice*/
    TCSM_PCHAR('*');//only test
    /* RTC PD */
    rtc_write_reg(0xb0003020, 1);
    while (1);
}

void soc_pm_fastboot_config(void)
{
	volatile unsigned int tmp;
	unsigned int intc1_msk;
	unsigned int clk_gate0;
	clk_gate0 = cpm_inl(CPM_CLKGR);

	clk_gate0 &= ~(1 << 29); //enable rtc gate
	cpm_outl(clk_gate0, CPM_CLKGR);

	intc1_msk = *(volatile unsigned int *)0xb000102c;
	*(volatile unsigned int *)0xb000102c = intc1_msk & ~(3<<0); // RTC INT MSK

#if 0
	/*disabled RTC Alarm wakeup*/
    tmp = rtc_read_reg(0xb000302c);
	tmp &= ~(1);
	rtc_write_reg(0xb000302c, tmp);

    /*clean Alarm enable and Alarm flag*/
    tmp = rtc_read_reg(0xb0003000);
    tmp &=~(0x7F);
    rtc_write_reg(0xb0003000,tmp);
#endif
    /*clear RTC HWRSR. clean all flag*/
    rtc_write_reg(0xb0003030, 0);

    /*clear RSR status*/
    *(volatile u32*)(0xb0000008) = 0;

    /*clean WKUPPINCR.P_RST_EN*/
    tmp = rtc_read_reg(0xb0003048);
    tmp &=~(0xf);
    rtc_write_reg(0xb0003048,tmp);

    /*clean HWFCR*/
    //rtc_write_reg(0xb0003028, 0x0);

    /*clean HRCR*/
    rtc_write_reg(0xb0003024, 0);

	/* 32k rtc clk */
	tmp = rtc_read_reg(0xb0003000);
	tmp &= ~(1 << 1);
	rtc_write_reg(0xb0003000, tmp);

}

static int soc_pm_wakeup_source(void)
{
    int tmp  = rtc_read_reg(0xb0003030);
    tmp &= 0x3;
    printk("Zboost:%s\n",wakup_source[tmp]);
	return 0;
}

void soc_pm_wakeup_fastboot(void)
{
    cpu_sync();
	soc_pm_wakeup_source();
	sys_restore();
    cpu_sync();
}
