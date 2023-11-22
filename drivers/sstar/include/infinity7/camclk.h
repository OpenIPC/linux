/*
 * camclk.h- Sigmastar
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

#ifndef __CAMCLK_H__
#define __CAMCLK_H__

#define CAMCLK_VOID                  0
#define CAMCLK_utmi_480m             1
#define CAMCLK_mpll_432m             2
#define CAMCLK_mpll_345m             3
#define CAMCLK_upll_384m             4
#define CAMCLK_upll_320m             5
#define CAMCLK_mpll_288m             6
#define CAMCLK_utmi_240m             7
#define CAMCLK_mpll_216m             8
#define CAMCLK_utmi_192m             9
#define CAMCLK_mpll_172m             10
#define CAMCLK_utmi_160m             11
#define CAMCLK_mpll_123m             12
#define CAMCLK_mpll_86m              13
#define CAMCLK_mpll_288m_div2        14
#define CAMCLK_mpll_288m_div4        15
#define CAMCLK_mpll_288m_div8        16
#define CAMCLK_mpll_216m_div2        17
#define CAMCLK_mpll_216m_div4        18
#define CAMCLK_mpll_216m_div8        19
#define CAMCLK_mpll_123m_div2        20
#define CAMCLK_mpll_86m_div2         21
#define CAMCLK_mpll_86m_div4         22
#define CAMCLK_mpll_86m_div16        23
#define CAMCLK_utmi_192m_div4        24
#define CAMCLK_utmi_160m_div4        25
#define CAMCLK_utmi_160m_div5        26
#define CAMCLK_utmi_160m_div8        27
#define CAMCLK_xtali_12m             28
#define CAMCLK_xtali_12m_div8        29
#define CAMCLK_xtali_12m_div16       30
#define CAMCLK_xtali_12m_div40       31
#define CAMCLK_xtali_12m_div64       32
#define CAMCLK_xtali_12m_div128      33
#define CAMCLK_xtali_24m             34
#define CAMCLK_RTC_CLK_32K           35
#define CAMCLK_pm_riu_w_clk_in       36
#define CAMCLK_lpll_clk_div2         37
#define CAMCLK_lpll_clk_div4         38
#define CAMCLK_lpll_clk_div8         39
#define CAMCLK_armpll_37p125m        40
#define CAMCLK_riu_w_clk_in          41
#define CAMCLK_riu_w_clk_top         42
#define CAMCLK_riu_w_clk_sc_gp       43
#define CAMCLK_riu_w_clk_vhe_gp      44
#define CAMCLK_riu_w_clk_hemcu_gp    45
#define CAMCLK_riu_w_clk_mipi_if_gp  46
#define CAMCLK_riu_w_clk_mcu_if_gp   47
#define CAMCLK_miu_p                 48
#define CAMCLK_mspi0_p               49
#define CAMCLK_mspi1_p               50
#define CAMCLK_miu_vhe_gp_p          51
#define CAMCLK_miu_sc_gp_p           52
#define CAMCLK_miu2x_p               53
#define CAMCLK_mcu_p                 54
#define CAMCLK_mcu_pm_p              55
#define CAMCLK_isp_p                 56
#define CAMCLK_fclk1_p               57
#define CAMCLK_fclk2_p               58
#define CAMCLK_sdio_p                59
#define CAMCLK_tck_buf               60
#define CAMCLK_pad2isp_sr_pclk       61
#define CAMCLK_ccir_in_clk           62
#define CAMCLK_eth_buf               63
#define CAMCLK_rmii_buf              64
#define CAMCLK_emac_testrx125_in_lan 65
#define CAMCLK_miu_ff                66
#define CAMCLK_miu_sc_gp             67
#define CAMCLK_miu_vhe_gp            68
#define CAMCLK_miu_dig               69
#define CAMCLK_miu_xd2miu            70
#define CAMCLK_miu_urdma             71
#define CAMCLK_miu_bdma              72
#define CAMCLK_miu_vhe               73
#define CAMCLK_miu_mfeh              74
#define CAMCLK_miu_mfe               75
#define CAMCLK_miu_jpe1              76
#define CAMCLK_miu_jpe0              77
#define CAMCLK_miu_bach              78
#define CAMCLK_miu_file              79
#define CAMCLK_miu_uhc0              80
#define CAMCLK_miu_emac              81
#define CAMCLK_miu_cmdq              82
#define CAMCLK_miu_isp_dnr           83
#define CAMCLK_miu_isp_rot           84
#define CAMCLK_miu_isp_dma           85
#define CAMCLK_miu_isp_sta           86
#define CAMCLK_miu_gop               87
#define CAMCLK_miu_sc_dnr            88
#define CAMCLK_miu_sc_dnr_sad        89
#define CAMCLK_miu_sc_crop           90
#define CAMCLK_miu_sc1_frm           91
#define CAMCLK_miu_sc1_snp           92
#define CAMCLK_miu_sc1_snpi          93
#define CAMCLK_miu_sc1_dbg           94
#define CAMCLK_miu_sc2_frm           95
#define CAMCLK_miu_sc2_snpi          96
#define CAMCLK_miu_sc3_frm           97
#define CAMCLK_miu_fcie              98
#define CAMCLK_miu_sdio              99
#define CAMCLK_miu_ive               100
#define CAMCLK_riu                   101
#define CAMCLK_riu_nogating          102
#define CAMCLK_riu_sc_gp             103
#define CAMCLK_riu_vhe_gp            104
#define CAMCLK_riu_hemcu_gp          105
#define CAMCLK_riu_mipi_gp           106
#define CAMCLK_riu_mcu_if            107
#define CAMCLK_miu2x                 108
#define CAMCLK_axi2x                 109
#define CAMCLK_tck                   110
#define CAMCLK_imi                   111
#define CAMCLK_gop0                  112
#define CAMCLK_gop1                  113
#define CAMCLK_gop2                  114
#define CAMCLK_mpll_144m             115
#define CAMCLK_mpll_144m_div2        116
#define CAMCLK_mpll_144m_div4        117
#define CAMCLK_xtali_12m_div2        118
#define CAMCLK_xtali_12m_div4        119
#define CAMCLK_xtali_12m_div12       120
#define CAMCLK_rtc_32k               121
#define CAMCLK_rtc_32k_div4          122
#define CAMCLK_live_pm               123
#define CAMCLK_riu_pm                124
#define CAMCLK_miupll_clk            125
#define CAMCLK_ddrpll_clk            126
#define CAMCLK_lpll_clk              127
#define CAMCLK_cpupll_clk            128
#define CAMCLK_utmi                  129
#define CAMCLK_upll                  130
#define CAMCLK_fuart0_synth_out      131
#define CAMCLK_csi2_mac_p            132
#define CAMCLK_miu                   133
#define CAMCLK_ddr_syn               134
#define CAMCLK_miu_rec               135
#define CAMCLK_mcu                   136
#define CAMCLK_riubrdg               137
#define CAMCLK_bdma                  138
#define CAMCLK_spi                   139
#define CAMCLK_uart0                 140
#define CAMCLK_uart1                 141
#define CAMCLK_fuart0_synth_in       142
#define CAMCLK_fuart                 143
#define CAMCLK_mspi0                 144
#define CAMCLK_mspi1                 145
#define CAMCLK_mspi                  146
#define CAMCLK_miic0                 147
#define CAMCLK_miic1                 148
#define CAMCLK_bist                  149
#define CAMCLK_pwr_ctl               150
#define CAMCLK_xtali                 151
#define CAMCLK_live                  152
#define CAMCLK_sr_mclk               153
#define CAMCLK_bist_vhe_gp           154
#define CAMCLK_vhe                   155
#define CAMCLK_vhe_vpu               156
#define CAMCLK_xtali_sc_gp           157
#define CAMCLK_bist_sc_gp            158
#define CAMCLK_emac_ahb              159
#define CAMCLK_jpe                   160
#define CAMCLK_aesdma                161
#define CAMCLK_sdio                  162
#define CAMCLK_sd                    163
#define CAMCLK_isp                   164
#define CAMCLK_fclk1                 165
#define CAMCLK_fclk2                 166
#define CAMCLK_odclk                 167
#define CAMCLK_dip                   168
#define CAMCLK_ive                   169
#define CAMCLK_nlm                   170
#define CAMCLK_emac_tx               171
#define CAMCLK_emac_rx               172
#define CAMCLK_emac_tx_ref           173
#define CAMCLK_emac_rx_ref           174
#define CAMCLK_hemcu_216m            175
#define CAMCLK_csi_mac               176
#define CAMCLK_mac_lptx              177
#define CAMCLK_ns                    178
#define CAMCLK_mcu_pm                179
#define CAMCLK_spi_pm                180
#define CAMCLK_pm_sleep              181
#define CAMCLK_sar                   182
#define CAMCLK_rtc                   183
#define CAMCLK_ir                    184
#endif
