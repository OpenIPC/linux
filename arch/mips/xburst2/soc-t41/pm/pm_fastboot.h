#ifndef __PM_FASTBOOT_H__
#define __PM_FASTBOOT_H__

struct pll_resume_reg {
	unsigned int cpccr;
	unsigned int cppcr;
	unsigned int cpapcr;
	unsigned int cpmpcr;
	unsigned int cpvpcr;
	unsigned int ddrcdr;
};

struct ddrc_resume_reg {
	unsigned int dcfg;
	unsigned int dctrl;
	unsigned int dlmr;
	unsigned int ddlp;
	unsigned int dasr_en;
	unsigned int dasr_cnt;
	unsigned int drefcnt;
	unsigned int dtimming1;
	unsigned int dtimming2;
	unsigned int dtimming3;
	unsigned int dtimming4;
	unsigned int dtimming5;
	unsigned int dmmap0;
	unsigned int dmmap1;
	unsigned int dbwcfg;
	unsigned int dbwstp;
	unsigned int hregpro;
	unsigned int dbgen;
	unsigned int dwcfg;
	unsigned int dremap1;
	unsigned int dremap2;
	unsigned int dremap3;
	unsigned int dremap4;
	unsigned int dremap5;
	unsigned int cpac;
	unsigned int cchc0;
	unsigned int cchc1;
	unsigned int cchc2;
	unsigned int cchc3;
	unsigned int cchc4;
	unsigned int cchc5;
	unsigned int cchc6;
	unsigned int cchc7;
	unsigned int cschc0;
	unsigned int cschc1;
	unsigned int cschc2;
	unsigned int cschc3;
	unsigned int cmonc0;
	unsigned int cmonc1;
	unsigned int cmonc2;
	unsigned int cmonc3;
	unsigned int cmonc4;
	unsigned int ccguc0;
	unsigned int ccguc1;
	unsigned int pregpro;
	unsigned int bufcfg;
};

struct ddr_phy_resume_reg {
	unsigned int phy_rst;
	unsigned int mem_cfg;
	unsigned int dq_width;
	unsigned int cl;
	unsigned int al;
	unsigned int cwl;
	unsigned int pll_fbdivh;
	unsigned int pll_fbdivl;
	unsigned int pll_ctrl;
	unsigned int pll_pdiv;
	unsigned int training_ctrl;
	unsigned int calib_delay_al;
	unsigned int calib_delay_ah;
	unsigned int calib_bypass_al;
	unsigned int calib_bypass_ah;
	unsigned int wl_mode1;
	unsigned int wl_mode2;

};


struct ccu_regs {
	unsigned int cfcr;
	unsigned int dmir;
	unsigned int mscr;
	unsigned int pimr;
	unsigned int mimr;
	unsigned int oimr;
	unsigned int dipr;
	unsigned int gdimr;
	unsigned int ldimr0;
	unsigned int ldimr1;
	unsigned int rer;
	unsigned int mbr0;
	unsigned int mbr1;
};



struct store_regs {
	unsigned int resume_pc;
	unsigned int uart_index;
	unsigned int version;
	struct pll_resume_reg pll_resume_reg;
	struct ddrc_resume_reg ddrc_resume_reg;
	struct ddr_phy_resume_reg ddr_phy_resume_reg;
    struct ccu_regs cpu_resume_reg;
    char user[64];
};



struct uart_regs {
	unsigned int udllr;
	unsigned int udlhr;
	unsigned int uthr;
	unsigned int uier;
	unsigned int ufcr;
	unsigned int ulcr;
	unsigned int umcr;
	unsigned int uspr;
	unsigned int isr;
	unsigned int umr;
	unsigned int uacr;
	unsigned int urcr;
	unsigned int utcr;

};


struct ost_regs {
	unsigned int ostccr;
	unsigned int oster;
	unsigned int ostcr;
	unsigned int ostfr;
	unsigned int ostmr;
	unsigned int ostdfr;
	unsigned int ostcnt;

	unsigned int g_ostccr;
	unsigned int g_oster;
	unsigned int g_ostcr;
	unsigned int g_ostcnth;
	unsigned int g_ostcntl;
	unsigned int g_ostcntb;
};

struct cpm_regs {
	unsigned int cpm_sftint;
	unsigned int cpsppr;
	unsigned int cpspr;
	unsigned int lcr;
	unsigned int pswc0st;
	unsigned int pswc1st;
	unsigned int pswc2st;
	unsigned int pswc3st;
	unsigned int clkgr0;
	unsigned int clkgr1;
	unsigned int mestsel;
	unsigned int srbc;
	unsigned int exclk_ds;
	unsigned int memory_pd0;
	unsigned int memory_pd1;
	unsigned int slbc;
	unsigned int slpc;
	unsigned int opcr;
	unsigned int rsr;
};

void load_func_to_rtc_ram(void);
void soc_pm_fastboot_config(void);
void soc_pm_wakeup_fastboot(void);
void load_func_to_oram(void);
void sys_save(void);
void sys_restore(void);

#endif
