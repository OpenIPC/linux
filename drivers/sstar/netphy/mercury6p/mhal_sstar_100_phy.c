/*
 * mhal_sstar_100_phy.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#include <linux/module.h>
#include "cam_os_wrapper.h"
#include "registers.h"

#define REG_BANK_ALBANY0  GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151800)
#define REG_BANK_ALBANY1  GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151900)
#define REG_BANK_ALBANY2  GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151A00)
#define REG_BANK_CLKGEN0  GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103800)
#define REG_BANK_SCGPCTRL GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x113300)
#define REG_BANK_XTAL     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x111b00)

#define reg_write8(bank, offset, val) OUTREG8(GET_REG_ADDR8((bank), (offset)), (val))
#define reg_read8(bank, offset)       INREG8(GET_REG_ADDR8((bank), (offset)))

static int b_albany_config_init = 0;

int albany_config_init(void)
{
    u8 uRegVal;

    if (b_albany_config_init)
        return 0;

    uRegVal = reg_read8(REG_BANK_XTAL, 0x12);
    uRegVal = (uRegVal & (~0x20));
    reg_write8(REG_BANK_XTAL, 0x12, uRegVal);

    /*
        wriu    0x103884    0x00        //Set CLK_EMAC_AHB to 123MHz (Enabled)
        wriu    0x113344    0x00        //Set CLK_EMAC_RX to CLK_EMAC_RX_in (25MHz) (Enabled)
        wriu    0x113346    0x00        //Set CLK_EMAC_TX to CLK_EMAC_TX_IN (25MHz) (Enabled)
    */
    reg_write8(REG_BANK_CLKGEN0, 0x84, 0x00);
    reg_write8(REG_BANK_SCGPCTRL, 0x44, 0x00);
    reg_write8(REG_BANK_SCGPCTRL, 0x46, 0x00);

    /* eth_link_sar*/
    // gain shift
    reg_write8(REG_BANK_ALBANY1, 0xb4, 0x02);

    // det max
    reg_write8(REG_BANK_ALBANY1, 0x4f, 0x02);

    // det min
    reg_write8(REG_BANK_ALBANY1, 0x51, 0x01);
    // snr len (emc noise)
    reg_write8(REG_BANK_ALBANY1, 0x77, 0x18);

    // lpbk_enable set to 0
    reg_write8(REG_BANK_ALBANY0, 0x72, 0xa0);

    reg_write8(REG_BANK_ALBANY1, 0xfc, 0x00); // Power-on LDO
    reg_write8(REG_BANK_ALBANY1, 0xfd, 0x00);
    reg_write8(REG_BANK_ALBANY2, 0xa1, 0x80); // Power-on SADC
    reg_write8(REG_BANK_ALBANY1, 0xcc, 0x40); // Power-on ADCPL
    reg_write8(REG_BANK_ALBANY1, 0xbb, 0x04); // Power-on REF
    reg_write8(REG_BANK_ALBANY2, 0x3a, 0x00); // Power-on TX
    reg_write8(REG_BANK_ALBANY2, 0xf1, 0x00); // Power-on TX

    reg_write8(REG_BANK_ALBANY2, 0x8a, 0x01); // CLKO_ADC_SEL
    reg_write8(REG_BANK_ALBANY1, 0x3b, 0x01); // reg_adc_clk_select
    reg_write8(REG_BANK_ALBANY1, 0xc4, 0x44); // TEST
    uRegVal = reg_read8(REG_BANK_ALBANY2, 0x80);
    uRegVal = (uRegVal & 0x0F) | 0x30;
    reg_write8(REG_BANK_ALBANY2, 0x80, uRegVal); // sadc timer

    // 100 gat
    reg_write8(REG_BANK_ALBANY2, 0xc5, 0x00);

    // 200 gat
    reg_write8(REG_BANK_ALBANY2, 0x30, 0x43);

    // en_100t_phase
    reg_write8(REG_BANK_ALBANY2, 0x39, 0x41); // en_100t_phase;  [6] save2x_tx

    reg_write8(REG_BANK_ALBANY2, 0xf2, 0xf5); // LP mode, DAC OFF
    reg_write8(REG_BANK_ALBANY2, 0xf3, 0x0d); // DAC off

    // Prevent packet drop by inverted waveform
    reg_write8(REG_BANK_ALBANY0, 0x79, 0xd0); // prevent packet drop by inverted waveform
    reg_write8(REG_BANK_ALBANY0, 0x77, 0x5a);

    // disable eee
    reg_write8(REG_BANK_ALBANY0, 0x2d, 0x7c); // disable eee

    // 10T waveform
    reg_write8(REG_BANK_ALBANY2, 0xe8, 0x06);
    reg_write8(REG_BANK_ALBANY0, 0x2b, 0x00);
    reg_write8(REG_BANK_ALBANY2, 0xe8, 0x00);
    reg_write8(REG_BANK_ALBANY0, 0x2b, 0x00);

    //    printf("[EMAC_todo] tune for analog\n");
    reg_write8(REG_BANK_ALBANY2, 0xe8, 0x06); // shadow_ctrl
    reg_write8(REG_BANK_ALBANY0, 0xaa, 0x1c); // tin17_s2
    reg_write8(REG_BANK_ALBANY0, 0xac, 0x1c); // tin18_s2
    reg_write8(REG_BANK_ALBANY0, 0xad, 0x1c);
    reg_write8(REG_BANK_ALBANY0, 0xae, 0x1c); // tin19_s2
    reg_write8(REG_BANK_ALBANY0, 0xaf, 0x1c);

    reg_write8(REG_BANK_ALBANY2, 0xe8, 0x00);
    reg_write8(REG_BANK_ALBANY0, 0xaa, 0x1c);
    reg_write8(REG_BANK_ALBANY0, 0xab, 0x28);

    // speed up timing recovery
    reg_write8(REG_BANK_ALBANY1, 0xf5, 0x02);

    // Signal_det k
    reg_write8(REG_BANK_ALBANY1, 0x0f, 0xc9);

    // snr_h
    reg_write8(REG_BANK_ALBANY1, 0x89, 0x50);
    reg_write8(REG_BANK_ALBANY1, 0x8b, 0x80);
    reg_write8(REG_BANK_ALBANY1, 0x8e, 0x0e);
    reg_write8(REG_BANK_ALBANY1, 0x90, 0x04);

    // set CLKsource to hv
    reg_write8(REG_BANK_ALBANY1, 0xC7, 0x80);
    // reg_write8(REG_BANK_ALBANY1, 0xC7, 0x00);

    /*
        //chiptop [15] allpad_in
        uRegVal = reg_read8(REG_BANK_CHIPTOP, 0xa1);
        uRegVal &= 0x7f;
        reg_write8(REG_BANK_CHIPTOP, 0xa1, uRegVal);
    */

    /*
        //enable LED //16'0x0e_28[5:4]=01
        uRegVal = reg_read8(REG_BANK_PMSLEEP, 0x50);
        uRegVal = (uRegVal&~0x30)|0x10;
        reg_write8(REG_BANK_PMSLEEP, 0x50, uRegVal);
    */
    reg_write8(REG_BANK_ALBANY0, 0x04, 0x11);
    reg_write8(REG_BANK_ALBANY0, 0x05, 0x11);
    reg_write8(REG_BANK_ALBANY0, 0x06, 0x22);
    reg_write8(REG_BANK_ALBANY0, 0x07, 0x22);

    // adc gain shift, shift up
    uRegVal = reg_read8(REG_BANK_ALBANY1, 0x5a * 2);
    uRegVal = uRegVal | 0x02;
    reg_write8(REG_BANK_ALBANY1, 0x5a * 2, uRegVal);

    b_albany_config_init = 1;
    return 0;
}

static int __init mhal_sstar_100_phy_init(void)
{
    albany_config_init();
    return 0;
}

static void __exit mhal_sstar_100_phy_exit(void) {}

module_init(mhal_sstar_100_phy_init);
module_exit(mhal_sstar_100_phy_exit);
