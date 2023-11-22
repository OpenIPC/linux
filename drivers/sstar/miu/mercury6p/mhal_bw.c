/*
 * mhal_bw.c- Sigmastar
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
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/delay.h>
#if defined(CONFIG_COMPAT)
#include <linux/compat.h>
#endif
#include "MsTypes.h"
#include "mdrv_types.h"
#include "mdrv_system.h"
#include "registers.h"
#include "ms_platform.h"
#include "mhal_bw.h"
#include "mhal_miu_client.h"
#include "mhal_miu.h"

U16 sum_queue_sel = 0x4404;
U16 misc_sel      = 0x000b;
U16 bw_sum_step   = 0x3030;
U16 gIPBWLock     = 1;

int halSetCheckIPBwLock(const char *buf)
{
    char cmds[16];

    sscanf(buf, "%s", cmds);
    if (strcmp(cmds, "unlock") == 0)
    {
        gIPBWLock = 0;
        printk("IP unlock\r\n");
        return 1;
    }

    return gIPBWLock;
}

static void halBAReportVal(int cmd, unsigned int *val_msb, unsigned int *val_lsb)
{
    U32 total_cmd, total_penalty, tmp, ddr_var = 200;
    U16 DDR4_num[5] = {12, 6, 24, 9, 17};
    U16 rbw_status_avg, wbw_status_avg, rbw_status_max, wbw_status_max;
    U16 ba_act_cnt, ba_r2p_cnt, ba_w2p_cnt, ba_rwt_cnt, ba_wrt_cnt;

    rbw_status_avg = INREG16(BASE_REG_MIU_MISC_CH0 + (0x67 << 2));
    wbw_status_avg = INREG16(BASE_REG_MIU_MISC_CH0 + (0x69 << 2));
    rbw_status_max = INREG16(BASE_REG_MIU_MISC_CH0 + (0x68 << 2));
    wbw_status_max = INREG16(BASE_REG_MIU_MISC_CH0 + (0x6A << 2));
    ba_act_cnt     = INREG16(BASE_REG_MIU_MISC_CH0 + (0x76 << 2));
    ba_r2p_cnt     = INREG16(BASE_REG_MIU_MISC_CH0 + (0x77 << 2));
    ba_w2p_cnt     = INREG16(BASE_REG_MIU_MISC_CH0 + (0x78 << 2));
    ba_rwt_cnt     = INREG16(BASE_REG_MIU_MISC_CH0 + (0x79 << 2));
    ba_wrt_cnt     = INREG16(BASE_REG_MIU_MISC_CH0 + (0x7A << 2));

#if 0
    printk("%s %d: %X %X %X %X %X %X %X, offset 0x73=%X\r\n", __FUNCTION__, __LINE__,
        rbw_status_avg, wbw_status_avg, ba_act_cnt, ba_r2p_cnt, ba_w2p_cnt, ba_rwt_cnt, ba_wrt_cnt, INREG16(BASE_REG_MIU_MISC_CH0+(0x73 << 2)));
#endif
    *val_msb = 0;
    *val_lsb = 0;

    if ((cmd >= MIU_BW_WCMD_CMD_ACT) && (cmd <= MIU_BW_WCMD_BW_END))
        total_cmd = wbw_status_avg << 4;
    else if ((cmd >= MIU_BW_RCMD_CMD_ACT) && (cmd <= MIU_BW_RCMD_BW_END))
        total_cmd = rbw_status_avg << 4;

    switch (cmd)
    {
        case MIU_BW_WCMD_CMD_ACT:
        case MIU_BW_RCMD_CMD_ACT:
            tmp      = ba_act_cnt;
            *val_msb = total_cmd / tmp;
            *val_lsb = 0;
            break;
        case MIU_BW_WCMD_BA_RWS:
        case MIU_BW_RCMD_BA_RWS:
            tmp      = ba_rwt_cnt + ba_wrt_cnt;
            *val_msb = total_cmd / tmp;
            *val_lsb = 0;
            break;
        case MIU_BW_WCMD_BA_ORI:
        case MIU_BW_RCMD_BA_ORI:
            total_penalty = ba_act_cnt * DDR4_num[0];
            total_penalty += ba_r2p_cnt * DDR4_num[1];
            total_penalty += ba_w2p_cnt * DDR4_num[2];
            total_penalty += ba_rwt_cnt * DDR4_num[3];
            total_penalty += ba_wrt_cnt * DDR4_num[4];
            // printk("%s %d penalty=%X\r\n", __FUNCTION__, __LINE__, total_penalty);
            total_cmd = (wbw_status_avg << 4) + (rbw_status_avg << 4);

            total_cmd = total_cmd * 2;
            tmp       = total_cmd + total_penalty;
            *val_msb  = (total_cmd * 100) / tmp;
            *val_lsb  = ((total_cmd * 10000) / tmp) % 100;
            break;
        case MIU_BW_WCMD_BA_NOR:
        case MIU_BW_RCMD_BA_NOR:
            total_penalty = ba_act_cnt * DDR4_num[0];
            total_penalty += ba_r2p_cnt * DDR4_num[1];
            total_penalty += ba_w2p_cnt * DDR4_num[2];
            total_penalty += ba_rwt_cnt * DDR4_num[3];
            total_penalty += ba_wrt_cnt * DDR4_num[4];
            // printk("%s %d penalty=%X\r\n", __FUNCTION__, __LINE__, total_penalty);
            total_cmd = (wbw_status_avg << 4) + (rbw_status_avg << 4);

            total_cmd = total_cmd * 2;
            tmp       = total_cmd + (total_penalty * 2) / 10;
            *val_msb  = (total_cmd * 100) / tmp;
            *val_lsb  = ((total_cmd * 10000) / tmp) % 100;
            break;
        case MIU_BW_WCMD_BW_AVG:
        case MIU_BW_RCMD_BW_AVG:
            // printk("%s %d total_cmd=%X\r\n", __FUNCTION__, __LINE__, total_cmd);
            tmp      = total_cmd * ddr_var;
            *val_msb = tmp / 1048576;
            *val_lsb = ((tmp * 100) / 1048576) % 100;
            break;
        case MIU_BW_WCMD_BW_MAX:
        case MIU_BW_RCMD_BW_MAX:
            if (cmd == MIU_BW_RCMD_BW_MAX)
                tmp = (rbw_status_max * 16) * ddr_var;
            else
                tmp = (wbw_status_max * 16) * ddr_var;
            *val_msb = tmp / 1048576;
            *val_lsb = ((tmp * 100) / 1048576) % 100;
            break;
        case MIU_BW_WCMD_BW_CMD:
        case MIU_BW_RCMD_BW_CMD:
            if (cmd == MIU_BW_RCMD_BW_CMD)
                tmp = rbw_status_avg;
            else
                tmp = wbw_status_avg;
            *val_msb = tmp;
            break;
        case MIU_BW_WCMD_BW_ACT:
        case MIU_BW_RCMD_BW_ACT:
            *val_msb = ba_act_cnt;
            break;
        case MIU_BW_WCMD_BW_W2P:
        case MIU_BW_RCMD_BW_R2P:
            if (cmd == MIU_BW_RCMD_BW_R2P)
                tmp = ba_r2p_cnt;
            else
                tmp = ba_w2p_cnt;
            *val_msb = tmp;
            break;
        case MIU_BW_WCMD_BW_RWT:
        case MIU_BW_RCMD_BW_RWT:
            *val_msb = ba_rwt_cnt;
            break;
        default:
            break;
    }
}

void halBWDebSelect(int bank, int id)
{
    // OUTREG16((bank+REG_ID_15), (id & 0x40) ? 0x80 : 0x00);
}

void halBWSetSumStep(int bank, int id, int period)
{
    // 0:   1/1M cycle
    // 1:   4/1M cycle
    // 2:   8/1M cycle
    // 3: 16/1M cycle
    // 4: 32/1M cycle
    // OUTREG16((bank+REG_SUM_STEP), period); //0x29
}

void halBWResetFunc(int bank, int wcmd)
{
    // OUTREG16((bank+REG_DEB_SEL), 0) ; // reset all
    OUTREGMSK16((bank + REG_BW_RPT), 0, BIT0); // reset all

    // disable bandwidth access report
    OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x60 << 2)), 0x0000);
}

void halBWFuncSelect(int bank, int id, int type)
{
    // OUTREG16((bank+REG_DEB_SEL), 0) ; // reset all
    // OUTREG16((bank+REG_DEB_SEL), ((id << 8) | type)); // reset
    // OUTREG16((bank+REG_DEB_SEL), ((id << 8) | type|ENABLE)); // set to read bandwidth
}

short halBWReadBus(int bank, int cmd, int id)
{
    switch (cmd)
    {
        case MIU_BW_WCMD_EFFI:
            return INREG16(bank + REG_ID_08);
            break;
        case MIU_BW_WCMD_AVG:
            return INREG16(bank + REG_ID_06);
            break;
        case MIU_BW_WCMD_MAX:
            return INREG16(bank + REG_ID_07);
            break;
        case MIU_BW_WCMD_AVG_EFFI:
            return INREG16(bank + REG_ID_04);
            break;
        case MIU_BW_WCMD_MAX_EFFI:
            return INREG16(bank + REG_ID_05);
            break;
        case MIU_BW_RCMD_EFFI:
            return INREG16(bank + REG_ID_0D);
            break;
        case MIU_BW_RCMD_AVG:
            return INREG16(bank + REG_ID_0B);
            break;
        case MIU_BW_RCMD_MAX:
            return INREG16(bank + REG_ID_0C);
            break;
        case MIU_BW_RCMD_AVG_EFFI:
            return INREG16(bank + REG_ID_09);
            break;
        case MIU_BW_RCMD_MAX_EFFI:
            return INREG16(bank + REG_ID_0A);
            break;
        default:
            printk("Error, read bw-bus command not support.\r\n");
            break;
    }

    return 0;
}

unsigned int halBWReadPercentValue(int bank, int cmd, int val, int dec)
{
    U32 val_msb, val_lsb;

    val_msb = 0;
    val_lsb = 0;
    if (cmd < MIU_BW_WCMD_CMD_ACT)
    {
        if (!dec)
            return ((val * 100) / 1024);
        else
            return (((val * 10000) / 1024) % 100);
    }
    else
    {
        halBAReportVal(cmd, &val_msb, &val_lsb);

        if (!dec)
            return val_msb;
        else
            return val_lsb;
    }
}

void halBWInit(int bank, int id, int wcmd)
{
    // halBWDebSelect(bank, id);
    halBWResetFunc(bank, wcmd);
}

void halBWEffiMinConfig(int bank, int id, int wcmd)
{
    // halBWSetSumStep(bank, id, DEF_BY_CHIP);
    // halBWFuncSelect(bank, id, MIN_EFFICIENCY | AVG_EFFICIENCY);
}

void halBWEffiRealConfig(int bank, int id, int wcmd)
{
    // halBWSetSumStep(bank, id, DEF_BY_CHIP);
    // halBWFuncSelect(bank, id, REAL_UTILIZTION);//0x40
}

void halBWEffiMinPerConfig(int bank, int id, int wcmd)
{
    halBWResetFunc(bank, wcmd);

    OUTREG16((bank + REG_BW_RPT_WID), 0x0000);
    OUTREG16((bank + REG_BW_RPT_RID), 0x0000);

    // set EFFI/BW
    OUTREG16((bank + REG_BW_RPT), 0x0002 | bw_sum_step);
    // msleep(1);
    OUTREG16((bank + REG_BW_RPT), 0x0003 | bw_sum_step);

    // set bandwidth access report
    if (id == 0)
    {
        OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x61 << 2)), 0x2000);
        OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x7b << 2)), 0x2000);
        // sel cmd_cnt ready
        OUTREGMSK16((BASE_REG_MIU_MISC_CH0 + (0x73 << 2)), 0x0000, 0x0003);
    }
    else
    {
        OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x61 << 2)), id);
        OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x7b << 2)), id);
        // sel cmd_cnt ready
        if (wcmd == MIU_CH_WCMD)
            OUTREGMSK16((BASE_REG_MIU_MISC_CH0 + (0x73 << 2)), 0x0001, 0x0003);
        else
            OUTREGMSK16((BASE_REG_MIU_MISC_CH0 + (0x73 << 2)), 0x0002, 0x0003);
    }
    OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x62 << 2)), sum_queue_sel);
    OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x74 << 2)), 0x0004);
    OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x60 << 2)), 0x0001);
    //////////////////////////////

    /*000*/
    // halBWSetSumStep(bank, id, DEF_BY_CHIP);
    // halBWFuncSelect(bank, id, MIN_EFFICIENCY | AVG_EFFICIENCY);//0x35
}

void halBWEffiAvgPerConfig(int bank, int id, int wcmd)
{
    // halBWResetFunc(bank, wcmd);
    /*404*/
    // halBWSetSumStep(bank, id, REQ_STATUS_PERIOD_32|BW_STATUS_PERIOD_32);
    // halBWFuncSelect(bank, id, REAL_UTILIZTION);//0x40
}

void halBWEffiMaxPerConfig(int bank, int id, int wcmd)
{
    // halBWResetFunc(bank, wcmd);
    /*404*/
    // halBWSetSumStep(bank, id, REQ_STATUS_PERIOD_32|BW_STATUS_PERIOD_32);
    // halBWFuncSelect(bank, id, MAX_UTILIZTION);//0x50
}

void halBWOCCRealPerConfig(int bank, int id, int wcmd)
{
    // halBWResetFunc(bank, wcmd);
    /*404*/
    // halBWSetSumStep(bank, id, REQ_STATUS_PERIOD_32|BW_STATUS_PERIOD_32);
    // halBWFuncSelect(bank, id, REAL_OCCUPANCY);//0x20
}

void halBWOCCMaxPerConfig(int bank, int id, int wcmd)
{
    // halBWResetFunc(bank, wcmd);
    /*404*/
    // halBWSetSumStep(bank, id, REQ_STATUS_PERIOD_32|BW_STATUS_PERIOD_32);
    // halBWFuncSelect(bank, id, MAX_OCCUPANCY);//0x60
}

void halBWMeasureRegsStore(const char *buf, int cmd_sel, int wcmd)
{
    char cmds[16];

    if (cmd_sel == MIU_BW_HDR_DDRINFO)
    {
        sscanf(buf, "%s", cmds);
        if (strcmp(cmds, "pa") == 0)
        {
            sscanf(buf, "%s %hx", cmds, &bw_sum_step);
            printk("bw_sum_step=%x\r\n", bw_sum_step);
        }
        else if (strcmp(cmds, "misc") == 0)
        {
            sscanf(buf, "%s %hx %hx", cmds, &sum_queue_sel, &misc_sel);
            printk("sum_queue_sel=%x, misc_sel=%x\r\n", sum_queue_sel, misc_sel);
        }
        else
            printk("Not support this command!!\r\n");
    }
}

static unsigned int halBWMeasureOverallInfo(unsigned int bank, short miu_ch, char *header, int hdr_size)
{
    U16 rbw_avg[2], rbw_max[2], wbw_avg[2], wbw_max[2];
    U16 asyrbw_avg[2], asyrbw_max[2], asywbw_avg[2], asywbw_max[2];
    U16 ddr_var;
    U32 tmp_val, str_ptr;

#if 0
    // DDR4/LPDDR4
    ddr_type = INREG16(BASE_REG_MIU_ATOP_CH0 + REG_ID_00) & 0x0007;
    if (ddr_type)
        ddr_var = 200; // DDR4
    else
        ddr_var = 400; // LPDDR4
#else
    ddr_var = 200; // DDR4
#endif
    tmp_val = 1048576;

    // MIU0
    rbw_avg[0] = (INREG16(bank + REG_ID_67) * 16 * ddr_var) / tmp_val;
    rbw_avg[1] = ((INREG16(bank + REG_ID_67) * 16 * ddr_var * 100) / tmp_val) % 100;
    rbw_max[0] = (INREG16(bank + REG_ID_68) * 16 * ddr_var) / tmp_val;
    rbw_max[1] = ((INREG16(bank + REG_ID_68) * 16 * ddr_var * 100) / tmp_val) % 100;
    wbw_avg[0] = (INREG16(bank + REG_ID_69) * 16 * ddr_var) / tmp_val;
    wbw_avg[1] = ((INREG16(bank + REG_ID_69) * 16 * ddr_var * 100) / tmp_val) % 100;
    wbw_max[0] = (INREG16(bank + REG_ID_6A) * 16 * ddr_var) / tmp_val;
    wbw_max[1] = ((INREG16(bank + REG_ID_6A) * 16 * ddr_var * 100) / tmp_val) % 100;

    if (INREG16(BASE_REG_MIU_PA + REG_ID_3E) & BIT0)
    {
        asyrbw_avg[0] = (INREG16(bank + REG_ID_7C) * 16 * ddr_var) / tmp_val;
        asyrbw_avg[1] = ((INREG16(bank + REG_ID_7C) * 16 * ddr_var * 100) / tmp_val) % 100;
        asyrbw_max[0] = (INREG16(bank + REG_ID_7D) * 16 * ddr_var) / tmp_val;
        asyrbw_max[1] = ((INREG16(bank + REG_ID_7D) * 16 * ddr_var * 100) / tmp_val) % 100;
        asywbw_avg[0] = (INREG16(bank + REG_ID_7E) * 16 * ddr_var) / tmp_val;
        asywbw_avg[1] = ((INREG16(bank + REG_ID_7E) * 16 * ddr_var * 100) / tmp_val) % 100;
        asywbw_max[0] = (INREG16(bank + REG_ID_7F) * 16 * ddr_var) / tmp_val;
        asywbw_max[1] = ((INREG16(bank + REG_ID_7F) * 16 * ddr_var * 100) / tmp_val) % 100;

        str_ptr =
            scnprintf(header, hdr_size,
                      "%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%d\t%d\t%d\t%d\t%02d.%02d%%\t%02d.%02d%%"
                      "\t%02d.%02d%%\t%02d.%02d%%",
                      rbw_avg[0], rbw_avg[1], rbw_max[0], rbw_max[1], wbw_avg[0], wbw_avg[1], wbw_max[0], wbw_max[1],
                      INREG16(bank + REG_ID_6C), INREG16(bank + REG_ID_6E), INREG16(bank + REG_ID_6B),
                      INREG16(bank + REG_ID_71), asyrbw_avg[0], asyrbw_avg[1], asyrbw_max[0], asyrbw_max[1],
                      asywbw_avg[0], asywbw_avg[1], asywbw_max[0], asywbw_max[1]);
    }
    else
    {
        str_ptr = scnprintf(header, hdr_size, "%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%d\t%d\t%d\t%d",
                            rbw_avg[0], rbw_avg[1], rbw_max[0], rbw_max[1], wbw_avg[0], wbw_avg[1], wbw_max[0],
                            wbw_max[1], INREG16(bank + REG_ID_6C), INREG16(bank + REG_ID_6E), INREG16(bank + REG_ID_6B),
                            INREG16(bank + REG_ID_71));
    }

    return str_ptr;
}

void halBWMeasureTrigger(int hdr_sel, int cli_id, int wcmd)
{
    U32 iMiu0BankAddr = BASE_REG_MIU_MISC_CH0;
    U16 client_id     = cli_id;

    // disable bandwidth access report
    OUTREG16((BASE_REG_MIU_MISC_CH0 + (0x60 << 2)), 0x0000);

    if (hdr_sel == MIU_BW_HDR_DDRINFO)
    {
        // MIU0
        // sum status and queue select
        OUTREG16((iMiu0BankAddr + REG_ID_62), sum_queue_sel);
        if (client_id == 0)
        {
            // rcmd ID
            OUTREG16((iMiu0BankAddr + REG_ID_61), 0x2000);
            // wcmd ID
            OUTREG16((iMiu0BankAddr + REG_ID_7B), 0x2000);
        }
        else
        {
            // rcmd ID
            OUTREG16((iMiu0BankAddr + REG_ID_61), client_id);
            // wcmd ID
            OUTREG16((iMiu0BankAddr + REG_ID_7B), client_id);
        }
        // misc signal sel
        OUTREG16((iMiu0BankAddr + REG_ID_70), misc_sel);
        // trigger
        OUTREG16((iMiu0BankAddr + REG_ID_60), 0x0001);
    }
}

void halBWGetMeasureHeader(char *header, int hdr_sel, int wcmd)
{
    char strbuf[128];
    int  size;

    if (hdr_sel == MIU_BW_HDR_DDRINFO)
    {
        if (INREG16(BASE_REG_MIU_PA + REG_ID_60) & BIT0)
        {
            sprintf(
                strbuf,
                "Num:client\tRBWavg\tRBWmax\tWBWavg\tWBWmax\tACTavg\tREFavg\tCREDIT\tMISC\tASY_RBWavg\tASY_RBWmax\tASY_"
                "WBWavg\tASY_WBWmax");
        }
        else
        {
            sprintf(strbuf, "Num:client\tRBWavg\tRBWmax\tWBWavg\tWBWmax\tACTavg\tREFavg\tCREDIT\tMISC");
        }
    }
    else
    {
        return;
    }

    size = strlen(strbuf);
    memcpy(header, strbuf, size);
}

void halBWGetMeasureStat(short miu_ch, char *header, int hdr_size, int hdr_sel, int cli_id, int wcmd)
{
    U32 iMiu0BankAddr = BASE_REG_MIU_MISC_CH0;
    U32 str_ptr;

    if (hdr_sel == MIU_BW_HDR_DDRINFO)
    {
        if (miu_ch == 0)
            str_ptr = halBWMeasureOverallInfo(iMiu0BankAddr, miu_ch, header, hdr_size);
    }
}

#ifdef CONFIG_CAM_CLK
unsigned int halBWGetDramInfo(char *str, char *end, unsigned int dram_freq, unsigned int miupll_freq)
{
#if 0
    char *       buf           = str;
    unsigned int iMiuBankAddr  = 0;
    unsigned int iAtopBankAddr = 0;
    unsigned int dram_type     = 0;
    unsigned int ddfset        = 0;
    unsigned int dram_freq     = 0;
    unsigned int miupll_freq   = 0;

    iMiuBankAddr  = BASE_REG_MIU_PA;
    iAtopBankAddr = BASE_REG_ATOP_PA;

    dram_type = INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0003);

    str += scnprintf(str, end - str, "DRAM Type:   %s\n",
                     (dram_type == 3)   ? "DDR3"
                     : (dram_type == 2) ? "DDR2"
                                        : "Unknown");
    str += scnprintf(str, end - str, "DRAM Size:   %dMB\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_69, 0xF000) >> 12));
    str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", CamClkRateGet(CAMCLK_ddrpll_clk) / 1000000);
    str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", CamClkRateGet(CAMCLK_miupll_clk) / 1000000);
    str +=
        scnprintf(str, end - str, "Data Rate:   %dx Mode\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0300) >> 8));
    str += scnprintf(str, end - str, "Bus Width:   %dbit\n", 16 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x000C) >> 2));
    str += scnprintf(str, end - str, "SSC:         %s\n",
                     (INREGMSK16(iAtopBankAddr + REG_ID_14, 0xC000) == 0x8000) ? "OFF" : "ON");

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
#else
    printk("%s, Not support!!\r\n", __FUNCTION___);
    return 0;
#endif
}
#else
unsigned int halBWGetDramInfo(char *str, char *end)
{
    char *buf = str;
    MIU_DramInfo_Hal pDramInfo;

    HAL_MIU_Info(&pDramInfo);

    if (HAL_MIU_Info(&pDramInfo) == 0)
    {
        str += scnprintf(str, end - str, "DRAM Type:   %s\n",
                         (pDramInfo.type == 0) ? "DDR3"
                         : (pDramInfo.type == 1) ? "DDR4"
                                                 : "Unknown");
        str += scnprintf(str, end - str, "DRAM Size:   %dMB\n", (unsigned int)(pDramInfo.size >> 20));
        str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", pDramInfo.dram_freq);
        str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", pDramInfo.miupll_freq);
        str += scnprintf(str, end - str, "Bus Width:   %dbit\n", pDramInfo.bus_width);
        str += scnprintf(str, end - str, "SSC:         %s\n", (pDramInfo.ssc == 0) ? "OFF" : "ON");
    }
    else
    {
        str += scnprintf(str, end - str, "Get dram info function fail or not support\n");
    }

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#endif
