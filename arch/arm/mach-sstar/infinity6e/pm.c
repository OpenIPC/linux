/*
* pm.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: Karl.Xiao <Karl.Xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/suspend.h>
#include <linux/io.h>
#include <asm/suspend.h>
#include <asm/fncpy.h>
#include <asm/cacheflush.h>
#include "ms_platform.h"
#include <linux/delay.h>
#include <asm/secure_cntvoff.h>
#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)
#include "voltage_ctrl.h"
#include "registers.h"
#if IS_ENABLED(CONFIG_SS_USB_LOWPWR_SUSPEND)
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "core.h"
#endif
#endif

#define FIN     printk(KERN_ERR"[%s]+++\n",__FUNCTION__)
#define FOUT    printk(KERN_ERR"[%s]---\n",__FUNCTION__)
#define HERE    printk(KERN_ERR"%s: %d\n",__FILE__,__LINE__)
#define SUSPEND_WAKEUP 0
#define SUSPEND_SLEEP  1
#define STR_PASSWORD   0x5A5A55AA

typedef struct {
    char magic[8];
    unsigned int resume_entry;
    unsigned int count;
    unsigned int status;
    unsigned int password;
} suspend_keep_info;

#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)
typedef struct {
    u32 reg;    // register address
    u16 mask;   // mask
    u16 val;    // register value
} reg_save;

static reg_save clkgen[] = {
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_03, .mask = 0x0101 },  // bist_sc_vhe_gp
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_05, .mask = 0x0101 },  // bist_ipu_usb3_gp
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_31, .mask = 0x0101 },  // uart
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_32, .mask = 0x0001 },  // spi_arb
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_33, .mask = 0x1101 },  // mspi
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_34, .mask = 0x0101 },  // fuart
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_37, .mask = 0x1101 },  // miic
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_38, .mask = 0x0101 },  // spi_flash_pwm
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_42, .mask = 0x0001 },  // emac_ahb
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_43, .mask = 0x0001 },  // sd
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_44, .mask = 0x0001 },  // ecc
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_45, .mask = 0x0001 },  // sdio
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_50, .mask = 0x0101 },  // ipu
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_52, .mask = 0x0001 },  // dip
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_53, .mask = 0x0001 },  // ldc
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_54, .mask = 0x0101 },  // bt656
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_58, .mask = 0x0101 },  // cis
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_59, .mask = 0x0101 },  // cis
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_5A, .mask = 0x0101 },  // cis
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_5B, .mask = 0x0101 },  // cis
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_5C, .mask = 0x0101 },  // cis
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_60, .mask = 0x0001 },  // bdma
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_61, .mask = 0x0101 },  // aesdma_isp
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_62, .mask = 0x0101 },  // sr0
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_63, .mask = 0x0001 },  // idclk
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_64, .mask = 0x0001 },  // fclk1
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_65, .mask = 0x0101 },  // sr_mclk
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_66, .mask = 0x0001 },  // odclk
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_68, .mask = 0x0081 },  // vhe
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_69, .mask = 0x0001 },  // mfe
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_6A, .mask = 0x0181 },  // jpe_ive
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_6B, .mask = 0x0101 },  // ns
    { .reg = BASE_REG_CLKGEN_PA + REG_ID_6C, .mask = 0x0101 },  // csi_mac
};

#define BASE_REG_AUSDM_PA   GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103400)
#define BASE_REG_EPHY1_PA   GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151500)
#define BASE_REG_EPHY2_PA   GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151600)
#define BASE_REG_UTMI_PA    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x142100)
#define BASE_REG_U3PHYD_PA  GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x152300)
#define BASE_REG_U3PHYA0_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x152400)

static reg_save ana_blk[] = {
    /* eth */
    { .reg = BASE_REG_EPHY2_PA + REG_ID_78, .mask = 0x3c00 },   // [10]: PD_TX_VBGR     [11]: PD_TX_TRIMMING_DAC
                                                                // [12]: PD_TX_LD_DIO   [13]: PD_TX_LDO
    { .reg = BASE_REG_EPHY2_PA + REG_ID_79, .mask = 0x0F80 },
    { .reg = BASE_REG_EPHY1_PA + REG_ID_1F, .mask = 0x4000 },
    { .reg = BASE_REG_EPHY1_PA + REG_ID_5B, .mask = 0x1000 },   // [12]: PD_ADC
    { .reg = BASE_REG_EPHY1_PA + REG_ID_66, .mask = 0x0010 },   // [ 4]: PD_ETHPLL_REG
    { .reg = BASE_REG_EPHY1_PA + REG_ID_7E, .mask = 0x0100 },   // [ 0]: PD_LDO11
    { .reg = BASE_REG_EPHY1_PA + REG_ID_69, .mask = 0xC000 },   // [15]: TX_OFF         [14]: LPF_INOFF
    { .reg = BASE_REG_EPHY1_PA + REG_ID_6A, .mask = 0x0020 },   // [ 5]: PD_LPF_OP
    { .reg = BASE_REG_EPHY2_PA + REG_ID_50, .mask = 0xB000 },   // [12]: PD_LPF_VBUF    [13]: PD_SADC
    { .reg = BASE_REG_EPHY1_PA + REG_ID_5D, .mask = 0x4000 },   // [14]: PD_REF
    { .reg = BASE_REG_EPHY1_PA + REG_ID_7E, .mask = 0x0002 },   // [ 1]: PD_REG25
    { .reg = BASE_REG_EPHY2_PA + REG_ID_44, .mask = 0x0010 },   // [ 4]: RX_OFF
    { .reg = BASE_REG_EPHY2_PA + REG_ID_1D, .mask = 0x0003 },   // [ 0]: PD_TX_LD       [ 1]: PD_TX_IDAC
    /* aud */
    { .reg = BASE_REG_AUSDM_PA + REG_ID_03, .mask = 0x1FF7 },
};

static int demander_vol[VOLTAGE_DEMANDER_MAX] = {0, };
static bool lpll_en     = false;
static bool venpll_en   = false;
static bool ipupll_en   = false;
#endif

extern void sram_suspend_imi(void);
static void (*sstar_suspend_imi_fn)(void);
static void __iomem *suspend_imi_vbase;
static suspend_keep_info *pStr_info;
int suspend_status = SUSPEND_WAKEUP;

#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)
#if IS_ENABLED(CONFIG_SS_USB_LOWPWR_SUSPEND)

static struct dwc3 *dwc = NULL;

static void phy_pipe3_suspend(void)
{
    u32 tx_frq_lo = INREG16(BASE_REG_U3PHYA0_PA + (0x40 << 2));
    u32 tx_frq_hi = INREG16(BASE_REG_U3PHYA0_PA + (0x41 << 2));

    SETREG16(BASE_REG_U3PHYA0_PA + (0x30 << 2), BIT(0)); //pd_rxpll

    //Synthesizer output clock 62.5Mhz(x37_6D4E)
    CLRREG16(BASE_REG_U3PHYA0_PA + (0x44 << 2), BIT(0));
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x40 << 2), 0x6D4E);
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x41 << 2), 0x0037);
    SETREG16(BASE_REG_U3PHYA0_PA + (0x30 << 2), BIT(0));
    //pre-setting restore:
    CLRREG16(BASE_REG_U3PHYA0_PA + (0x44 << 2), BIT(0));
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x40 << 2), tx_frq_lo);
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x41 << 2), tx_frq_hi);
}

static void phy_pipe3_wakeup(void)
{
    CLRREG16(BASE_REG_U3PHYD_PA + (0xA << 2), BIT(13)); // Force DA_PCIE_SIGDET_CAL_OFFSET value
    // enable AFE
    CLRREG16(BASE_REG_U3PHYD_PA + (0xD << 2), BIT(14));
    SETREG16(BASE_REG_U3PHYD_PA + (0x3C << 2), BIT(0));

    //pd_txpll, pd_rxpll.
    CLRREG16(BASE_REG_U3PHYA0_PA + (0x20 << 2), BIT(0));
    CLRREG16(BASE_REG_U3PHYA0_PA + (0x30 << 2), BIT(0));

    //TXPLL SYN_IN/XTAL_IN OFF ...
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x10 << 2), 0x0000);
    OUTREGMSK16(BASE_REG_U3PHYA0_PA + (0x11 << 2), 0, ~(BIT(5)));

    // Enable Bandgap
    CLRREG16(BASE_REG_U3PHYD_PA + (0x51 << 2), BIT(15));
    CLRREG16(BASE_REG_U3PHYD_PA + (0x8 << 2), (BIT(1)|BIT(0)));
    SETREG16(BASE_REG_U3PHYD_PA + (0x0 << 2), (BIT(1)|BIT(0)));
}

static void phy_pipe3_shutdown(void)
{
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x10 << 2), BIT(5)|BIT(4));
    OUTREG16(BASE_REG_U3PHYA0_PA + (0x11 << 2), BIT(15));

    //pd_txpll, pd_rxpll.
    SETREG16(BASE_REG_U3PHYA0_PA + (0x20 << 2), BIT(0));
    SETREG16(BASE_REG_U3PHYA0_PA + (0x30 << 2), BIT(0));

    SETREG16(BASE_REG_U3PHYD_PA + (0x0 << 2), (BIT(1)|BIT(0)));
    SETREG16(BASE_REG_U3PHYD_PA + (0x8 << 2), (BIT(1)|BIT(0)));
    CLRREG16(BASE_REG_U3PHYD_PA + (0x51 << 2), BIT(15));
}

static void phy_utmi_suspend(void)
{
    OUTREG16(BASE_REG_UTMI_PA + (0x4 << 2), 0x8D2F);
    OUTREGMSK16(BASE_REG_UTMI_PA + (0x0 << 2), 0xEF05, ~(BIT(1)));
}

static void phy_utmi_wakeup(void)
{
    OUTREG16(BASE_REG_UTMI_PA + (0x4 << 2), 0x8D2F);
    OUTREGMSK16(BASE_REG_UTMI_PA + (0x0 << 2), 0x1, ~(BIT(1)));
}

static void phy_utmi_shutdown(void)
{
    OUTREG16(BASE_REG_UTMI_PA + (0x4 << 2), BIT(7));
    OUTREGMSK16(BASE_REG_UTMI_PA + (0x0 << 2), 0x7F05, ~(BIT(1)));
}
#endif

static void sstar_analog_pwroff(void)
{
    int i, cnt = 0;

    cnt = sizeof(ana_blk) / sizeof(reg_save);
    for(i = 0; i < cnt; i++) {
        ana_blk[i].val = INREG16(ana_blk[i].reg);
        SETREG16(ana_blk[i].reg, ana_blk[i].mask);
    }
}

static void sstar_analog_pwron(void)
{
    int i, cnt = 0;

    cnt = sizeof(ana_blk) / sizeof(reg_save);
    for(i = 0; i < cnt; i++) {
        OUTREG16(ana_blk[i].reg, ana_blk[i].val);
    }
}

static void sstar_enter_lowpwr(void)
{
    int i, cnt = 0;

    // power down part of analog blocks
    sstar_analog_pwroff();

    // slow down IP to MIU clock rate
    for(i = 0; i < 0x10; i++) {
        SETREG16(BASE_REG_MCM_SC_GP_PA + BK_REG(i), 0xF0F0);
    }
    // save clock settings & gate clock
    cnt = sizeof(clkgen) / sizeof(reg_save);
    for(i = 0; i < cnt; i++) {
        clkgen[i].val = INREG16(clkgen[i].reg);
        SETREG16(clkgen[i].reg, clkgen[i].mask);
    }

#if IS_ENABLED(CONFIG_SS_USB_LOWPWR_SUSPEND)
    if (dwc) {
        if (dwc->gadget.speed >= USB_SPEED_SUPER) {
            phy_utmi_shutdown();
            phy_pipe3_suspend();
        }
        else {
            phy_utmi_suspend();
            phy_pipe3_shutdown();
        }
    }
#endif

    // power down pll
    lpll_en = venpll_en = ipupll_en = false;
    if (0 == (INREG16(BASE_REG_LPLL_PA + REG_ID_40) & 0x8000)) {
        lpll_en = true;
        SETREG16(BASE_REG_LPLL_PA + REG_ID_40, 0x8000);     // power down = 1b'1
    }
    if (0 == (INREG16(BASE_REG_VENPLL_PA + REG_ID_01) & 0x0100)) {
        venpll_en = true;
        SETREG16(BASE_REG_VENPLL_PA + REG_ID_01, 0x0100);   // power down = 1b'1
    }
    if (0x80 == (INREG16(BASE_REG_IPUPLL_PA + REG_ID_11) & 0x0180)) {
        /* don't power up ipupll in leave low power.
         * ipu handle ipupll in its driver already. */
        //ipupll_en = true;
        CLRREG16(BASE_REG_IPUPLL_PA + REG_ID_11, 0x0080);   // en clk = 1b'0
        SETREG16(BASE_REG_IPUPLL_PA + REG_ID_11, 0x0100);   // power down = 1b'1
    }
    // save voltage level of each demander, force V-core to 0.85V
    for(i = 0; i < VOLTAGE_DEMANDER_MAX; i++) {
        demander_vol[i] = get_core_voltage_of_demander(i);
        if (demander_vol[i] != 0) {
            set_core_voltage(i, VOLTAGE_CORE_850);
        }
    }
}

static void sstar_leave_lowpwr(void)
{
    int i, cnt = 0;

    // restore voltage level of each demander
    for(i = 0; i < VOLTAGE_DEMANDER_MAX; i++) {
        if (demander_vol[i] != 0) {
            set_core_voltage(i, demander_vol[i]);
        }
    }
    // power up pll
    if (lpll_en) {
        CLRREG16(BASE_REG_LPLL_PA + REG_ID_40, 0x8000);     // power down = 1b'0
    }
    if (venpll_en) {
        CLRREG16(BASE_REG_VENPLL_PA + REG_ID_01, 0x0100);   // power down = 1b'0
    }
    if (ipupll_en) {
        CLRREG16(BASE_REG_IPUPLL_PA + REG_ID_11, 0x0100);   // power down = 1b'0
        SETREG16(BASE_REG_IPUPLL_PA + REG_ID_11, 0x0080);   // en clk = 1b'1
    }

#if IS_ENABLED(CONFIG_SS_USB_LOWPWR_SUSPEND)
    if (dwc) {
        phy_utmi_wakeup();
        phy_pipe3_wakeup();
    }
#endif

    // restore clock
    cnt = sizeof(clkgen) / sizeof(reg_save);
    for(i = 0; i < cnt; i++) {
        OUTREG16(clkgen[i].reg, clkgen[i].val);
    }
    // restore IP to MIU clock rate
    for(i = 0; i < 0x10; i++) {
        CLRREG16(BASE_REG_MCM_SC_GP_PA + BK_REG(i), 0xF0F0);
    }
    // power up part of analog blocks
    sstar_analog_pwron();
}
#endif

static int sstar_suspend_ready(unsigned long ret)
{
    sstar_suspend_imi_fn = fncpy(suspend_imi_vbase, (void*)&sram_suspend_imi, 0x1000);
    suspend_status = SUSPEND_SLEEP;

    //resume info
    if (pStr_info) {
        pStr_info->count++;
        pStr_info->status = SUSPEND_SLEEP;
        pStr_info->password = STR_PASSWORD;
    }

#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)
    sstar_enter_lowpwr();
#endif
    //flush cache to ensure memory is updated before self-refresh
    __cpuc_flush_kern_all();
    //flush L3 cache
    Chip_Flush_MIU_Pipe();
    //flush tlb to ensure following translation is all in tlb
    local_flush_tlb_all();

    sstar_suspend_imi_fn();

#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)
    sstar_leave_lowpwr();
#endif
    return 0;
}

static int sstar_suspend_enter(suspend_state_t state)
{
#if IS_ENABLED(CONFIG_SS_USB_LOWPWR_SUSPEND)
    static u8 once = 0;

    //FIN;
    if (!dwc && !once) {
        struct device_node *dev_node;
        struct platform_device *pdev;

        once = 1; // get dwc driver data once after boot
        dev_node = of_find_compatible_node(NULL, NULL, "snps,dwc3");

        if (!dev_node)
            return -ENODEV;

        pdev = of_find_device_by_node(dev_node);
        if (pdev)
        {
            dwc = platform_get_drvdata(pdev);
            printk("dwc %px, speed: %d\n", dwc, dwc->gadget.speed);
            of_node_put(dev_node);
        }
    }
#endif

    switch (state)
    {
        case PM_SUSPEND_MEM:
            printk(KERN_INFO "state = PM_SUSPEND_MEM\n");
            cpu_suspend(0, sstar_suspend_ready);
            #ifdef CONFIG_SMP
            secure_cntvoff_init();
            #endif
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

static void sstar_suspend_wake(void)
{
    if (pStr_info) {
        pStr_info->status = SUSPEND_WAKEUP;
        pStr_info->password = 0;
    }
}

struct platform_suspend_ops sstar_suspend_ops = {
    .enter    = sstar_suspend_enter,
    .wake     = sstar_suspend_wake,
    .valid    = suspend_valid_only_mem,
};

int __init mstar_pm_init(void)
{
    unsigned int resume_pbase = virt_to_phys(cpu_resume);
    suspend_imi_vbase = __arm_ioremap_exec(0xA0010000, 0x1000, false);  //put suspend code at IMI offset 64K;

    pStr_info = (suspend_keep_info *)__va(0x20000000);
    memset(pStr_info, 0, sizeof(suspend_keep_info));
    strcpy(pStr_info->magic, "SIG_STR");
    pStr_info->resume_entry = resume_pbase;

    suspend_set_ops(&sstar_suspend_ops);

    printk(KERN_INFO "[%s] resume_pbase=0x%08X, suspend_imi_vbase=0x%08X\n", __func__, (unsigned int)resume_pbase, (unsigned int)suspend_imi_vbase);
    return 0;
}
