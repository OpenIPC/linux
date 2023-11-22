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

U16 ba_ddr4[6]    = {2, 12, 6, 24, 9, 17};
U16 ba_lpddr4[6]  = {4, 14, 8, 22, 13, 18};
U16 sum_queue_sel = 0x4404;
U16 misc_sel      = 0x000b;
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
    OUTREGMSK16((bank + REG_ID_60), 0, BIT0 | BIT1 | BIT2);
}

void halBWFuncSelect(int bank, int id, int type)
{
    // OUTREG16((bank+REG_DEB_SEL), 0) ; // reset all
    // OUTREG16((bank+REG_DEB_SEL), ((id << 8) | type)); // reset
    // OUTREG16((bank+REG_DEB_SEL), ((id << 8) | type|ENABLE)); // set to read bandwidth
}

int halBWGetEFFIRegBase(unsigned short grp_idx)
{
    switch (grp_idx)
    {
        case 0:
            return BASE_REG_MIU_EFFI_SC2;
            break;
        case 1:
            printk(KERN_ERR "Group%d not support\n", grp_idx);
            return 0;
            break;
        case 2:
            return BASE_REG_MIU_EFFI_SC0;
            break;
        case 3:
            return BASE_REG_MIU_EFFI_SC1;
            break;
        case 4:
            return BASE_REG_MIU_EFFI_ISP0;
            break;
        case 5:
            return BASE_REG_MIU_EFFI_ISP1;
            break;
        case 6:
            return BASE_REG_MIU_EFFI_DISP0;
            break;
        case 7:
            return BASE_REG_MIU_EFFI_DISP1;
            break;
    }

    return 0;
}

short halBWReadBus(int bank, int cmd, int id)
{
    switch (cmd)
    {
        case MIU_BW_WCMD_EFFI: // ba bandwidth
            return 0;
            break;
        case MIU_BW_WCMD_AVG: // avg bw for miu0
            if (!id)
                return INREG16(bank + REG_ID_65);
            else
                return INREG16(bank + REG_ID_69);
            break;
        case MIU_BW_WCMD_MAX: // max bw for miu0
            if (!id)
                return INREG16(bank + REG_ID_66);
            else
                return INREG16(bank + REG_ID_6A);
            break;
        case MIU_BW_WCMD_AVG_EFFI: // avg bw for miu1
            return 0;
            break;
        case MIU_BW_WCMD_MAX_EFFI: // max bw for miu1
            return 0;
            break;
        case MIU_BW_RCMD_EFFI: // ba bandwidth
            return 0;
            break;
        case MIU_BW_RCMD_AVG: // avg bw for miu0
            if (!id)
                return INREG16(bank + REG_ID_63);
            else
                return INREG16(bank + REG_ID_67);
            break;
        case MIU_BW_RCMD_MAX: // max bw for miu0
            if (!id)
                return INREG16(bank + REG_ID_64);
            else
                return INREG16(bank + REG_ID_68);
            break;
        case MIU_BW_RCMD_AVG_EFFI: // avg bw for miu1
            return 0;
            break;
        case MIU_BW_RCMD_MAX_EFFI: // max bw for miu1
            return 0;
            break;
        default:
            printk("Error, read bw-bus command not support.\r\n");
            break;
    }

    return 0;
}

unsigned int halBWReadPercentValue(int bank, int cmd, int val, int dec)
{
    int tmp[2], ba_data[6];
    U16 ddr_type;
    U32 ddr_var = 16, total_cycle = 1048576;

    // DDR4/LPDDR4
    ddr_type = INREG16(BASE_REG_MIU_ATOP_CH0 + REG_ID_00) & 0x0007;

    switch (cmd)
    {
        case MIU_BW_WCMD_EFFI: // ba bandwidth
        case MIU_BW_RCMD_EFFI: // ba bandwidth
            ba_data[0] = INREG16(bank + REG_ID_75);
            ba_data[1] = INREG16(bank + REG_ID_76);
            ba_data[2] = INREG16(bank + REG_ID_77);
            ba_data[3] = INREG16(bank + REG_ID_78);
            ba_data[4] = INREG16(bank + REG_ID_79);
            ba_data[5] = INREG16(bank + REG_ID_7A);

            if (ddr_type)
            {
                // DDR4
                tmp[0] = ba_data[0] * ba_ddr4[0];
                tmp[1] = (ba_data[0] * ba_ddr4[0] + ba_data[1] * ba_ddr4[1] + ba_data[2] * ba_ddr4[2]
                          + ba_data[3] * ba_ddr4[3] + ba_data[4] * ba_ddr4[4] + ba_data[5] * ba_ddr4[5]);
            }
            else
            {
                // LPDDR4
                tmp[0] = ba_data[0] * ba_lpddr4[0];
                tmp[1] = (ba_data[0] * ba_lpddr4[0] + ba_data[1] * ba_lpddr4[1] + ba_data[2] * ba_lpddr4[2]
                          + ba_data[3] * ba_lpddr4[3] + ba_data[4] * ba_lpddr4[4] + ba_data[5] * ba_lpddr4[5]);
            }

            if (!dec)
            {
                tmp[0] = (tmp[0] * 100) / tmp[1];
                return tmp[0];
            }
            else
            {
                tmp[0] = ((tmp[0] * 10000) / tmp[1]) % 100;
                return tmp[0];
            }
            return 0;
            break;
        case MIU_BW_WCMD_AVG:      // avg bw for miu0
        case MIU_BW_WCMD_MAX:      // max bw for miu0
        case MIU_BW_WCMD_AVG_EFFI: // avg bw for miu1
        case MIU_BW_WCMD_MAX_EFFI: // max bw for miu1
        case MIU_BW_RCMD_AVG:      // avg bw for miu0
        case MIU_BW_RCMD_MAX:      // max bw for miu0
        case MIU_BW_RCMD_AVG_EFFI: // avg bw for miu1
        case MIU_BW_RCMD_MAX_EFFI: // max bw for miu1
            if (ddr_type)
                ddr_var = ddr_var * 200; // DDR4
            else
                ddr_var = ddr_var * 400; // LPDDR4
            if (!dec)
            {
                return (val * ddr_var) / total_cycle;
                break;
            }
            else
            {
                return (((val * ddr_var) * 100) / total_cycle) % 100;
                break;
            }
        default:
            printk("Error, read bw-bus command not support.\r\n");
            break;
    }

    return 0;
}

void halBWInit(int bank, int id, int wcmd)
{
    U16 client_id = id;

    halBWResetFunc(bank, wcmd);

    if (id == 0x80)
        client_id = 0x100; // CA55

    OUTREG16((bank + REG_ID_73), 0x0000);
    OUTREG16((bank + REG_ID_7C), 0x0000);

    if (wcmd == MIU_CH_WCMD)
    {
        OUTREG16((bank + REG_ID_7B), client_id);
        // ba
        if (client_id == 0)
            OUTREG16((bank + REG_ID_7C), 0x200);
        else
        {
            OUTREGMSK16((bank + REG_ID_7C), client_id, 0x07FF);
        }
        OUTREGMSK16((bank + REG_ID_73), 0x1000, 0x3000);
    }
    else if (wcmd == MIU_CH_RCMD)
    {
        OUTREG16((bank + REG_ID_61), client_id);
        // ba
        if (client_id == 0)
            OUTREG16((bank + REG_ID_73), 0x200);
        else
        {
            OUTREGMSK16((bank + REG_ID_73), client_id, 0x07FF);
        }
        OUTREGMSK16((bank + REG_ID_73), 0x2000, 0x3000);
    }
    // 4: avg every 32 loops
    // 3: avg every 16 loops
    // 2: avg every 8 loops
    // 1: avg every 4 loops
    // 0: avg every 1 loops
    OUTREGMSK16((bank + REG_ID_62), 4, 0x0007);
}

void halBWEffiMinConfig(int bank, int id, int wcmd) {}

void halBWEffiRealConfig(int bank, int id, int wcmd) {}

void halBWEffiMinPerConfig(int bank, int id, int wcmd)
{
    U16 tmp_val;

    OUTREGMSK16((bank + REG_ID_60), BIT0 | BIT1 | BIT2, BIT0 | BIT1 | BIT2);
    // DDR4/LPDDR4
    tmp_val = INREG16(BASE_REG_MIU_ATOP_CH0 + REG_ID_00) & 0x0007;
    // MIU0
    // ba ddr type
    if (tmp_val)
        OUTREG16((bank + REG_ID_74), 0x0024); // DDR4
    else
        OUTREG16((bank + REG_ID_74), 0x0020); // LPDDR4
}

void halBWEffiAvgPerConfig(int bank, int id, int wcmd) {}

void halBWEffiMaxPerConfig(int bank, int id, int wcmd) {}

void halBWOCCRealPerConfig(int bank, int id, int wcmd) {}

void halBWOCCMaxPerConfig(int bank, int id, int wcmd) {}

void halBWMeasureRegsStore(const char *buf, int cmd_sel, int wcmd)
{
    if (cmd_sel == MIU_BW_HDR_DDRINFO)
    {
        sscanf(buf, "%hx %hx", &sum_queue_sel, &misc_sel);
        printk("%s %d: sum_queue_sel=%x, misc_sel=%x\r\n", __FUNCTION__, __LINE__, sum_queue_sel, misc_sel);
    }
}

static unsigned int halBWMeasureClientBA(unsigned int bank, short miu_ch, char *header, int hdr_size, int wcmd)
{
    U16 ddr_type, ddr_var, str_len;
    U32 bwavg[2], bwmax[2], tmp_val;
    U32 cmd_cnt, act_cnt, r2p_cnt, w2p_cnt, rwt_cnt, wrt_cnt;
    U32 bl_cmd, bl_rws, ba_ori[2], ba_nor[2], penalty;

    // DDR4/LPDDR4
    ddr_type = INREG16(BASE_REG_MIU_ATOP_CH0 + REG_ID_00) & 0x0007;
    if (ddr_type)
        ddr_var = 200; // DDR4
    else
        ddr_var = 400; // LPDDR4
    tmp_val = 1048576;

    if (wcmd == MIU_CH_RCMD)
    {
        // BW avg for rcmd
        bwavg[0] = (INREG16(bank + REG_ID_67) * 16 * ddr_var) / tmp_val;
        bwavg[1] = ((INREG16(bank + REG_ID_67) * 16 * ddr_var * 100) / tmp_val) % 100;
        // BW max
        bwmax[0] = (INREG16(bank + REG_ID_68) * 16 * ddr_var) / tmp_val;
        bwmax[1] = ((INREG16(bank + REG_ID_68) * 16 * ddr_var * 100) / tmp_val) % 100;
    }
    else if (wcmd == MIU_CH_WCMD)
    {
        // BW avg for wcmd
        bwavg[0] = (INREG16(bank + REG_ID_69) * 16 * ddr_var) / tmp_val;
        bwavg[1] = ((INREG16(bank + REG_ID_69) * 16 * ddr_var * 100) / tmp_val) % 100;
        // BW max
        bwmax[0] = (INREG16(bank + REG_ID_6A) * 16 * ddr_var) / tmp_val;
        bwmax[1] = ((INREG16(bank + REG_ID_6A) * 16 * ddr_var * 100) / tmp_val) % 100;
    }
    else
    {
        return 0;
    }

    cmd_cnt = INREG16(bank + REG_ID_75);
    act_cnt = INREG16(bank + REG_ID_76);
    r2p_cnt = INREG16(bank + REG_ID_77);
    w2p_cnt = INREG16(bank + REG_ID_78);
    rwt_cnt = INREG16(bank + REG_ID_79);
    wrt_cnt = INREG16(bank + REG_ID_7A);

    // cmd/act
    bl_cmd = cmd_cnt / act_cnt;
    // BA read/write switch
    bl_rws = cmd_cnt / (rwt_cnt + wrt_cnt);

    // BA origin
    if (ddr_type)
    {
        penalty = (act_cnt * ba_ddr4[1]) + (r2p_cnt * ba_ddr4[2]) + (w2p_cnt * ba_ddr4[3]) + (rwt_cnt * ba_ddr4[4])
                  + (wrt_cnt * ba_ddr4[5]);
        ba_ori[0] = (cmd_cnt * ba_ddr4[0] * 100) / (cmd_cnt * ba_ddr4[0] + penalty);
        ba_ori[1] = ((cmd_cnt * ba_ddr4[0] * 10000) / (cmd_cnt * ba_ddr4[0] + penalty)) % 100;
        ba_nor[0] = (cmd_cnt * ba_ddr4[0] * 100) / (cmd_cnt * ba_ddr4[0] + (((penalty)*2) / 10));
        ba_nor[1] = ((cmd_cnt * ba_ddr4[0] * 10000) / (cmd_cnt * ba_ddr4[0] + (((penalty)*2) / 10))) % 100;
    }
    else
    {
        penalty = (act_cnt * ba_lpddr4[1]) + (r2p_cnt * ba_lpddr4[2]) + (w2p_cnt * ba_lpddr4[3])
                  + (rwt_cnt * ba_lpddr4[4]) + (wrt_cnt * ba_lpddr4[5]);
        ba_ori[0] = (cmd_cnt * ba_lpddr4[0] * 100) / (cmd_cnt * ba_lpddr4[0] + penalty);
        ba_ori[1] = ((cmd_cnt * ba_lpddr4[0] * 10000) / (cmd_cnt * ba_lpddr4[0] + penalty)) % 100;
        ba_nor[0] = (cmd_cnt * ba_lpddr4[0] * 100) / (cmd_cnt * ba_lpddr4[0] + (((penalty)*2) / 10));
        ba_nor[1] = ((cmd_cnt * ba_lpddr4[0] * 10000) / (cmd_cnt * ba_lpddr4[0] + (((penalty)*2) / 10))) % 100;
    }

    str_len = scnprintf(header, hdr_size,
                        "%d\t%02d.%02d%%\t%02d.%02d%%\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%02d.%02d%%\t%02d.%02d%%",
                        miu_ch, bwavg[0], bwavg[1], bwmax[0], bwmax[1], cmd_cnt, act_cnt, r2p_cnt, w2p_cnt, rwt_cnt,
                        wrt_cnt, bl_cmd, bl_rws, ba_ori[0], ba_ori[1], ba_nor[0], ba_nor[1]);
    return str_len;
}

static unsigned int halBWMeasureOverallInfo(unsigned int bank, short miu_ch, char *header, int hdr_size)
{
    U16 rbw_avg[2], rbw_max[2], wbw_avg[2], wbw_max[2];
    U16 ddr_var, ddr_type;
    U32 tmp_val, str_ptr;

    // DDR4/LPDDR4
    ddr_type = INREG16(BASE_REG_MIU_ATOP_CH0 + REG_ID_00) & 0x0007;
    if (ddr_type)
        ddr_var = 200; // DDR4
    else
        ddr_var = 400; // LPDDR4
    tmp_val = 1048576;

    // MIU0
    rbw_avg[0] = (INREG16(bank + REG_ID_63) * 16 * ddr_var) / tmp_val;
    rbw_avg[1] = ((INREG16(bank + REG_ID_63) * 16 * ddr_var * 100) / tmp_val) % 100;
    rbw_max[0] = (INREG16(bank + REG_ID_64) * 16 * ddr_var) / tmp_val;
    rbw_max[1] = ((INREG16(bank + REG_ID_64) * 16 * ddr_var * 100) / tmp_val) % 100;
    wbw_avg[0] = (INREG16(bank + REG_ID_65) * 16 * ddr_var) / tmp_val;
    wbw_avg[1] = ((INREG16(bank + REG_ID_65) * 16 * ddr_var * 100) / tmp_val) % 100;
    wbw_max[0] = (INREG16(bank + REG_ID_66) * 16 * ddr_var) / tmp_val;
    wbw_max[1] = ((INREG16(bank + REG_ID_66) * 16 * ddr_var * 100) / tmp_val) % 100;

    str_ptr = scnprintf(header, hdr_size, "%d\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%d\t%d\t%d\t%d\t%d",
                        miu_ch, rbw_avg[0], rbw_avg[1], rbw_max[0], rbw_max[1], wbw_avg[0], wbw_avg[1], wbw_max[0],
                        wbw_max[1], INREG16(bank + REG_ID_6C), INREG16(bank + REG_ID_6E),
                        INREG16(bank + REG_ID_6B) >> 8, INREG16(bank + REG_ID_6B) & 0xFF, INREG16(bank + REG_ID_71));
    return str_ptr;
}

void halBWMeasureTrigger(int hdr_sel, int cli_id, int wcmd)
{
    U32 iMiu0BankAddr = BASE_REG_MIU_MISC_CH0;
    U32 iMiu1BankAddr = BASE_REG_MIU_MISC_CH1;
    U16 tmp_val;
    U16 client_id = cli_id;

    halBWResetFunc(iMiu0BankAddr, 0);
    halBWResetFunc(iMiu1BankAddr, 0);

    if (cli_id == 0x80)
        client_id = 0x100; // CA55

    if (hdr_sel == MIU_BW_HDR_DDRINFO)
    {
        // MIU0
        // sum status and queue select
        OUTREG16((iMiu0BankAddr + REG_ID_62), sum_queue_sel);
        // rcmd ID
        OUTREG16((iMiu0BankAddr + REG_ID_61), 0x0000);
        // wcmd ID
        OUTREG16((iMiu0BankAddr + REG_ID_7B), 0x0000);
        // misc signal sel
        OUTREG16((iMiu0BankAddr + REG_ID_70), misc_sel);
        // trigger
        OUTREG16((iMiu0BankAddr + REG_ID_60), 0x0007);

        // MIU1
        // sum status and queue select
        OUTREG16((iMiu1BankAddr + REG_ID_62), sum_queue_sel);
        // rcmd ID
        OUTREG16((iMiu1BankAddr + REG_ID_61), 0x0000);
        // wcmd ID
        OUTREG16((iMiu1BankAddr + REG_ID_7B), 0x0000);
        // misc signal sel
        OUTREG16((iMiu1BankAddr + REG_ID_70), misc_sel);
        // trigger
        OUTREG16((iMiu1BankAddr + REG_ID_60), 0x0007);
    }
    else if (hdr_sel == MIU_BW_HDR_BA_REPORT)
    {
        // DDR4/LPDDR4
        tmp_val = INREG16(BASE_REG_MIU_ATOP_CH0 + REG_ID_00) & 0x0007;
        // MIU0
        // ba ddr type
        if (tmp_val)
            OUTREG16((iMiu0BankAddr + REG_ID_74), 0x0024); // DDR4
        else
            OUTREG16((iMiu0BankAddr + REG_ID_74), 0x0020); // LPDDR4
        if (wcmd == MIU_CH_RCMD)
        {
            // rcmd id
            OUTREGMSK16((iMiu0BankAddr + REG_ID_61), client_id, 0x07FF);
            OUTREGMSK16((iMiu0BankAddr + REG_ID_73), client_id, 0x07FF);
            OUTREGMSK16((iMiu0BankAddr + REG_ID_73), 0x2000, 0x3000);
        }
        else if (wcmd == MIU_CH_WCMD)
        {
            // wcmd id
            OUTREGMSK16((iMiu0BankAddr + REG_ID_7B), client_id, 0x07FF);
            OUTREGMSK16((iMiu0BankAddr + REG_ID_7C), client_id, 0x07FF);
            OUTREGMSK16((iMiu0BankAddr + REG_ID_73), 0x1000, 0x3000);
        }

        // trigger
        OUTREG16((iMiu0BankAddr + REG_ID_60), 0x0007);

        // MIU1
        // ba ddr type
        if (tmp_val)
            OUTREG16((iMiu1BankAddr + REG_ID_74), 0x0024); // DDR4
        else
            OUTREG16((iMiu1BankAddr + REG_ID_74), 0x0020); // LPDDR4
        if (wcmd == MIU_CH_RCMD)
        {
            // rcmd id
            OUTREGMSK16((iMiu1BankAddr + REG_ID_61), client_id, 0x07FF);
            OUTREGMSK16((iMiu1BankAddr + REG_ID_73), client_id, 0x07FF);
            OUTREGMSK16((iMiu1BankAddr + REG_ID_73), 0x2000, 0x3000);
        }
        else if (wcmd == MIU_CH_WCMD)
        {
            // wcmd id
            OUTREGMSK16((iMiu1BankAddr + REG_ID_7B), client_id, 0x07FF);
            OUTREGMSK16((iMiu1BankAddr + REG_ID_7C), client_id, 0x07FF);
            OUTREGMSK16((iMiu1BankAddr + REG_ID_73), 0x1000, 0x3000);
        }

        // trigger
        OUTREG16((iMiu1BankAddr + REG_ID_60), 0x0007);
    }
    else if (hdr_sel == MIU_BW_RD_LATENCY)
    {
        if (client_id > 0x7F)
            return;

        iMiu0BankAddr = halBWGetEFFIRegBase((client_id >> 4));

        OUTREGMSK16((iMiu0BankAddr + REG_ID_00), ((client_id & 0x0F) << 8), 0x0F00);
        SETREG16((iMiu0BankAddr + REG_ID_00), BIT0);
        OUTREG16((iMiu0BankAddr + REG_ID_0E), 0x0120);
    }
}

void halBWGetMeasureHeader(char *header, int hdr_sel, int wcmd)
{
    char strbuf[128];
    int  size;

    if (hdr_sel == MIU_BW_HDR_BABW)
    {
        sprintf(strbuf, "Num:client\t\tBA_rp0\tBWavg0\tBWmax0\tBA_rp1\tBWavg1\tBWmax1");
    }
    else if (hdr_sel == MIU_BW_HDR_DDRINFO)
    {
        sprintf(strbuf, "Num:client\tMIU0/1\tRBWavg\tRBWmax\tWBWavg\tWBWmax\tACTavg\tREFavg\tAQavg\tCREDIT\tMISC");
    }
    else if (hdr_sel == MIU_BW_HDR_BA_REPORT)
    {
        sprintf(strbuf, "Num:client\tMIU0/1\tBWavg\tBWmax\tCMD\tACT\tR2P\tW2P\tRWT\tWRT\tCMD/ACT\tBArws\tBAori\tBAnor");
    }
    else if (hdr_sel == MIU_BW_RD_LATENCY)
    {
        sprintf(strbuf, "Num:client");
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
    U16 effi_avg, bw_avg, bw_max;
    U32 output_bw_ch0[3][2] = {0};
    U32 output_bw_ch1[3][2] = {0};
    U32 iMiu0BankAddr       = BASE_REG_MIU_MISC_CH0;
    U32 iMiu1BankAddr       = BASE_REG_MIU_MISC_CH1;
    U32 str_ptr;

    if (hdr_sel == MIU_BW_HDR_BABW)
    {
        if (wcmd == MIU_CH_WCMD)
        {
            effi_avg            = halBWReadBus(iMiu0BankAddr, MIU_BW_WCMD_EFFI, cli_id);
            output_bw_ch0[0][0] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_WCMD_EFFI, effi_avg, 0);
            output_bw_ch0[0][1] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_WCMD_EFFI, effi_avg, 1);

            bw_avg              = halBWReadBus(iMiu0BankAddr, MIU_BW_WCMD_AVG, cli_id);
            output_bw_ch0[1][0] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_WCMD_AVG, bw_avg, 0);
            output_bw_ch0[1][1] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_WCMD_AVG, bw_avg, 1);

            bw_max              = halBWReadBus(iMiu0BankAddr, MIU_BW_WCMD_MAX, cli_id);
            output_bw_ch0[2][0] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_WCMD_MAX, bw_max, 0);
            output_bw_ch0[2][1] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_WCMD_MAX, bw_max, 1);

            effi_avg            = halBWReadBus(iMiu1BankAddr, MIU_BW_WCMD_EFFI, cli_id);
            output_bw_ch1[0][0] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_WCMD_EFFI, effi_avg, 0);
            output_bw_ch1[0][1] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_WCMD_EFFI, effi_avg, 1);

            bw_avg              = halBWReadBus(iMiu1BankAddr, MIU_BW_WCMD_AVG, cli_id);
            output_bw_ch1[1][0] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_WCMD_AVG, bw_avg, 0);
            output_bw_ch1[1][1] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_WCMD_AVG, bw_avg, 1);

            bw_max              = halBWReadBus(iMiu1BankAddr, MIU_BW_WCMD_MAX, cli_id);
            output_bw_ch1[2][0] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_WCMD_MAX, bw_max, 0);
            output_bw_ch1[2][1] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_WCMD_MAX, bw_max, 1);
        }
        else if (wcmd == MIU_CH_RCMD)
        {
            effi_avg            = halBWReadBus(iMiu0BankAddr, MIU_BW_RCMD_EFFI, cli_id);
            output_bw_ch0[0][0] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_RCMD_EFFI, effi_avg, 0);
            output_bw_ch0[0][1] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_RCMD_EFFI, effi_avg, 1);

            bw_avg              = halBWReadBus(iMiu0BankAddr, MIU_BW_RCMD_AVG, cli_id);
            output_bw_ch0[1][0] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_RCMD_AVG, bw_avg, 0);
            output_bw_ch0[1][1] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_RCMD_AVG, bw_avg, 1);

            bw_max              = halBWReadBus(iMiu0BankAddr, MIU_BW_RCMD_MAX, cli_id);
            output_bw_ch0[2][0] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_RCMD_MAX, bw_max, 0);
            output_bw_ch0[2][1] = halBWReadPercentValue(iMiu0BankAddr, MIU_BW_RCMD_MAX, bw_max, 1);

            effi_avg            = halBWReadBus(iMiu1BankAddr, MIU_BW_RCMD_EFFI, cli_id);
            output_bw_ch1[0][0] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_RCMD_EFFI, effi_avg, 0);
            output_bw_ch1[0][1] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_RCMD_EFFI, effi_avg, 1);

            bw_avg              = halBWReadBus(iMiu1BankAddr, MIU_BW_RCMD_AVG, cli_id);
            output_bw_ch1[1][0] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_RCMD_AVG, bw_avg, 0);
            output_bw_ch1[1][1] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_RCMD_AVG, bw_avg, 1);

            bw_max              = halBWReadBus(iMiu1BankAddr, MIU_BW_RCMD_MAX, cli_id);
            output_bw_ch1[2][0] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_RCMD_MAX, bw_max, 0);
            output_bw_ch1[2][1] = halBWReadPercentValue(iMiu1BankAddr, MIU_BW_RCMD_MAX, bw_max, 1);
        }
        else
        {
            return;
        }

        scnprintf(header, hdr_size, "%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%",
                  output_bw_ch0[0][0], output_bw_ch0[0][1], output_bw_ch0[1][0], output_bw_ch0[1][1],
                  output_bw_ch0[2][0], output_bw_ch0[2][1], output_bw_ch1[0][0], output_bw_ch1[0][1],
                  output_bw_ch1[1][0], output_bw_ch1[1][1], output_bw_ch1[2][0], output_bw_ch1[2][1]);
    }
    else if (hdr_sel == MIU_BW_HDR_DDRINFO)
    {
        if (miu_ch == 0)
            str_ptr = halBWMeasureOverallInfo(iMiu0BankAddr, miu_ch, header, hdr_size);
        else
            str_ptr = halBWMeasureOverallInfo(iMiu1BankAddr, miu_ch, header, hdr_size);
    }
    else if (hdr_sel == MIU_BW_HDR_BA_REPORT)
    {
        if (miu_ch == 0)
            str_ptr = halBWMeasureClientBA(iMiu0BankAddr, miu_ch, header, hdr_size, wcmd);
        else
            str_ptr = halBWMeasureClientBA(iMiu1BankAddr, miu_ch, header, hdr_size, wcmd);
    }
    else if (hdr_sel == MIU_BW_RD_LATENCY)
    {
        if (cli_id > 0x7F)
        {
            scnprintf(header, hdr_size, "LATENCY\tNot support");
            return;
        }

        iMiu0BankAddr       = halBWGetEFFIRegBase((cli_id >> 4));
        output_bw_ch0[0][0] = INREG16(iMiu0BankAddr + REG_ID_01);
        output_bw_ch0[0][1] = INREG16(iMiu0BankAddr + REG_ID_02);
        output_bw_ch0[1][0] = INREG16(iMiu0BankAddr + REG_ID_03);
        output_bw_ch0[1][1] = INREG16(iMiu0BankAddr + REG_ID_04);
        output_bw_ch0[2][0] = INREG16(iMiu0BankAddr + REG_ID_05);
        output_bw_ch0[2][1] = INREG16(iMiu0BankAddr + REG_ID_06);
        output_bw_ch1[0][0] = INREG16(iMiu0BankAddr + REG_ID_07);
        output_bw_ch1[0][1] = INREG16(iMiu0BankAddr + REG_ID_08);

        output_bw_ch1[1][0] = output_bw_ch0[0][0] + output_bw_ch0[0][1] + output_bw_ch0[1][0] + output_bw_ch0[1][1]
                              + output_bw_ch0[2][0] + output_bw_ch0[2][1] + output_bw_ch1[0][0] + output_bw_ch1[0][1];

        scnprintf(header, hdr_size, "LATENCY\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x",
                  output_bw_ch0[0][0], output_bw_ch0[0][1], output_bw_ch0[1][0], output_bw_ch0[1][1],
                  output_bw_ch0[2][0], output_bw_ch0[2][1], output_bw_ch1[0][0], output_bw_ch1[0][1],
                  output_bw_ch1[1][0] / 8);
    }
    else if (hdr_sel == MIU_BW_RD_LENGTH)
    {
        if (cli_id > 0x7F)
        {
            scnprintf(header, hdr_size, "LENGTH\tNot support");
            return;
        }

        iMiu0BankAddr       = halBWGetEFFIRegBase((cli_id >> 4));
        output_bw_ch0[0][0] = INREG16(iMiu0BankAddr + REG_ID_09) & 0x03FF;
        output_bw_ch0[0][1] =
            (INREG16(iMiu0BankAddr + REG_ID_09) >> 10) | ((INREG16(iMiu0BankAddr + REG_ID_0A) & 0x000F) << 6);
        output_bw_ch0[1][0] = (INREG16(iMiu0BankAddr + REG_ID_0A) >> 4) & 0x03FF;
        output_bw_ch0[1][1] =
            (INREG16(iMiu0BankAddr + REG_ID_0A) >> 14) | ((INREG16(iMiu0BankAddr + REG_ID_0B) & 0x00FF) << 2);
        output_bw_ch0[2][0] =
            (INREG16(iMiu0BankAddr + REG_ID_0B) >> 8) | ((INREG16(iMiu0BankAddr + REG_ID_0C) & 0x0003) << 8);
        output_bw_ch0[2][1] = (INREG16(iMiu0BankAddr + REG_ID_0C) >> 2) % 0x03FF;
        output_bw_ch1[0][0] =
            (INREG16(iMiu0BankAddr + REG_ID_0C) >> 12) | ((INREG16(iMiu0BankAddr + REG_ID_0D) & 0x003F) << 4);
        output_bw_ch1[0][1] = INREG16(iMiu0BankAddr + REG_ID_0D) >> 6;

        scnprintf(header, hdr_size, "LENGTH\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x\t%04x", output_bw_ch0[0][0],
                  output_bw_ch0[0][1], output_bw_ch0[1][0], output_bw_ch0[1][1], output_bw_ch0[2][0],
                  output_bw_ch0[2][1], output_bw_ch1[0][0], output_bw_ch1[0][1]);

        // turn off effi sample
        CLRREG16(iMiu0BankAddr + REG_ID_00, BIT0);
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
static unsigned int halBWGetMIUArbiterPll(void)
{
    unsigned int   iMiuPllBankAddr = BASE_REG_MIUPLL_PA;
    unsigned short post_div, arb_clk;
    unsigned int   synth_clk;

    post_div  = (INREG16(iMiuPllBankAddr + REG_ID_07) >> 8) & 0x1F;
    synth_clk = (INREG16(iMiuPllBankAddr + REG_ID_13) << 16) | INREG16(iMiuPllBankAddr + REG_ID_12);
    arb_clk   = ((unsigned long long)(432 * 24) << 19) / (synth_clk * post_div * 1);
    // printk("%s, post_div=%d, synth_clk=%d, arb_clk=%d\r\n", __FUNCTION__, post_div, synth_clk, arb_clk);

    return arb_clk;
}

unsigned int halBWGetDramInfo(char *str, char *end)
{
    char *       buf             = str;
    unsigned int iMiuMmuBankAddr = 0;
    unsigned int iAtopBankAddr   = 0;
    // unsigned int iMiupllBankAddr = 0;
    unsigned int dram_type   = 0;
    unsigned int ddfset      = 0;
    unsigned int ddr_region  = 0;
    unsigned int dram_freq   = 0;
    unsigned int dram_2ch    = 0;
    unsigned int miupll_freq = 0;

    // printk("%s, Not support!!\r\n", __FUNCTION__);
    // return 0;

    iMiuMmuBankAddr = BASE_REG_MIU_MMU;
    iAtopBankAddr   = BASE_REG_MIU_ATOP_CH0;
    // iMiupllBankAddr = BASE_REG_MIUPLL_PA;

    dram_type  = INREG16(iAtopBankAddr + REG_ID_00) & 0x0007;
    ddr_region = (INREG16(iAtopBankAddr + REG_ID_1D) >> 6) & 0x03;
    ddfset     = (INREG16(iAtopBankAddr + REG_ID_19) << 16) | INREG16(iAtopBankAddr + REG_ID_18);
    dram_freq  = ((unsigned long long)432 << (24 - ddr_region)) / ddfset;
    // miupll_freq = 24 * INREGMSK16(iMiupllBankAddr + REG_ID_03, 0x00FF)
    //               / ((INREGMSK16(iMiupllBankAddr + REG_ID_03, 0x0700) >> 8) + 2);
    str += scnprintf(str, end - str, "DRAM Type:   %s\n",
                     (dram_type == 0)   ? "LPDDR4"
                     : (dram_type == 1) ? "DDR4"
                                        : "Unknown");
    str +=
        scnprintf(str, end - str, "DRAM Size:   %dMB\n", 1 << (INREGMSK16(iMiuMmuBankAddr + REG_ID_03, 0xF000) >> 12));
    str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", dram_freq);
    miupll_freq = halBWGetMIUArbiterPll();
    str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", miupll_freq);
    // str +=
    //     scnprintf(str, end - str, "Data Rate:   %dx Mode\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0300) >>
    //     8));
    dram_2ch = (INREG16(BASE_REG_MIU_PRE_ARB + REG_ID_7F) >> 2) & 0x03;
    str += scnprintf(str, end - str, "Bus Width:   %sbit\n", (dram_2ch == 3) ? "64" : "32");
    str += scnprintf(str, end - str, "SSC:         %s\n",
                     (INREGMSK16(iAtopBankAddr + REG_ID_14, 0xC000) == 0x8000) ? "OFF" : "ON");

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#endif
