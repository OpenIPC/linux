/*
 * mstar_mci_v5.c- Sigmastar
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
#include "eMMC_linux.h"
#include "mstar_mci.h"
#include "linux/mmc/mmc.h"
#include "drv_eMMC.h"
#include "eMMC_platform.h"
#include "eMMC_err_codes.h"
#include "eMMC_platform.h"
#include "eMMC_hal.h"
#include "eMMC_utl.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define MCI_RETRY_CNT_CMD_TO    100
#define MCI_RETRY_CNT_CRC_ERR   200 // avoid stack overflow
#define MCI_RETRY_CNT_OK_CLK_UP 10
#define EXT_CSD_BOOT_BUS_WIDTH  177
#define EXT_CSD_PART_CONF       179 /* R/W */
#define EXT_CSD_BOOT_BUS_WIDTH  177

#define EXT_CSD_BOOT_ACK(x)         (x << 6)
#define EXT_CSD_BOOT_PART_NUM(x)    (x << 3)
#define EXT_CSD_PARTITION_ACCESS(x) (x << 0)

#define EXT_CSD_BOOT_BUS_WIDTH_MODE(x)  (x << 3)
#define EXT_CSD_BOOT_BUS_WIDTH_RESET(x) (x << 2)
#define EXT_CSD_BOOT_BUS_WIDTH_WIDTH(x) (x)
/******************************************************************************
 * * Define Global Variables
 ******************************************************************************/
static U8             gu8_emmcSlotNums;
static U32            gu32AdmaMode[EMMC_NUM_TOTAL];
static eMMC_IP_EmType ge_emmcIPOrderSlot[EMMC_NUM_TOTAL];
U8                    gu8_emmcPADOrderSlot[EMMC_NUM_TOTAL];
U32                   gu32_BusWidth[EMMC_NUM_TOTAL];
U32                   gu32_max_clk[EMMC_NUM_TOTAL];
EMMCPinDrv_T          gst_emmcPinDriving[EMMC_NUM_TOTAL];

#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
static volatile U32 gu32_SupportEmmc50[EMMC_NUM_TOTAL]  = {0};
static volatile U16 gu16_PLL_CLKPH_SKEW[EMMC_NUM_TOTAL] = {0};
static volatile U16 gu16_PLL_SKEW_SUM[EMMC_NUM_TOTAL]   = {0};
static volatile U16 gu16_PLL_DQS_SUM[EMMC_NUM_TOTAL]    = {0};

#define V_MAX_PHASE 18
static volatile U8 gu8Arr_PASS_PHS[EMMC_NUM_TOTAL][V_MAX_PHASE];
static volatile U8 gu8_PASS_PH_CNT[EMMC_NUM_TOTAL] = {0};
#endif

static const char *gu8_fcie_clk_name[] = {"clk_fcie0", "clk_fcie1", "clk_fcie2", "clk_fcie3"};
#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
static const char *gu8_syn_clk_name[] = {"syn_clk_fcie0", "syn_clk_fcie1", "syn_clk_fcie2", "syn_clk_fcie3"};
#endif
/******************************************************************************
 * Function Prototypes
 ******************************************************************************/
static void mstar_mci_send_command(struct mstar_mci_host *pSstarHost_st, struct mmc_command *pCmd_st);
static U32  mstar_mci_completed_command(struct mstar_mci_host *pSstarHost_st, U32 u32_err);
#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
static void mstar_mci_completed_command_FromRAM(struct mstar_mci_host *pSstarHost_st);
#endif

#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
static int mstar_mci_pre_dma_transfer(struct mstar_mci_host *pSstarHost_st, struct mmc_data *data,
                                      struct mstar_mci_host_next *next);
#endif

u32 mstar_mci_WaitD0High(eMMC_IP_EmType emmc_ip, u32 u32_us);

/*****************************************************************************
 * Define Static Global Variables
 ******************************************************************************/
static struct task_struct *sgp_eMMCThread_st = NULL;
#if 0
static ulong wr_seg_size = 0;
static ulong wr_split_threshold = 0;
#endif
u8         u8_enable_sar5 = 0;
dma_addr_t MIU0_BUS_ADDR  = 0;
#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
static int fcie_irq[EMMC_NUM_TOTAL] = {0};
#endif
// ===============================
// for /sys files
#if 0
U32 gu32_pwrsvr_gpio_enable = 0;
U32 gu32_pwrsvr_gpio_addr = 0;
U32 gu32_pwrsvr_gpio_bit = 0;
U32 gu32_pwrsvr_gpio_trigger = 0;
U32 gu32_emmc_sanitize = 0;
#endif
static U32                gu32_eMMC_read_log_enable           = 0;
static U32                gu32_eMMC_write_log_enable          = 0;
static U32                gu32_eMMC_monitor_enable            = {0};
static U32                gu32_eMMC_read_cnt[EMMC_NUM_TOTAL]  = {0};
static U32                gu32_eMMC_write_cnt[EMMC_NUM_TOTAL] = {0};
static unsigned long long gu64_jiffies_org[EMMC_NUM_TOTAL];
static unsigned long long gu64_jiffies_write[EMMC_NUM_TOTAL] = {0};
static unsigned long long gu64_jiffies_read[EMMC_NUM_TOTAL]  = {0};

U16 u16_OldPLLClkParam    = 0xFFFF;
U16 u16_OldPLLDLLClkParam = 0xFFFF;

#define FCIE_ADMA_DESC_COUNT 512
// struct _AdmaDescriptor eMMC_ALIGN0 gAdmaDesc_st[FCIE_ADMA_DESC_COUNT] eMMC_ALIGN1;
struct _AdmaDescriptor *gAdmaDesc_st = NULL;

#if defined(MSTAR_EMMC_CONFIG_OF)
struct clk_data *clkdata;
#endif

/*****************************************************************************
 * for profiling
 ******************************************************************************/
#if defined(CONFIG_MMC_MSTAR_MMC_EMMC_LIFETEST)
static struct proc_dir_entry *writefile;
const char                    procfs_name[] = "StorageBytes";

int procfile_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    int ret;

    if (offset > 0)
        ret = 0;
    else
    {
        ret = sprintf(buffer, "TotalWriteBytes %llu GB %llu MB\nTotalReadBytes %llu GB %llu MB\n",
                      g_eMMCDrv[emmc_ip].u64_CNT_TotalWBlk / 1024 / 1024 / 2,
                      (g_eMMCDrv[emmc_ip].u64_CNT_TotalWBlk / 1024 / 2) % 1024,
                      g_eMMCDrv[emmc_ip].u64_CNT_TotalRBlk / 1024 / 1024 / 2,
                      (g_eMMCDrv[emmc_ip].u64_CNT_TotalRBlk / 1024 / 2) % 1024);
    }

    return ret;
}
#endif

/******************************************************************************
 * Functions
 ******************************************************************************/
static int mstar_mci_get_dma_dir(struct mmc_data *data)
{
#if defined(CONFIG_ENABLE_EMMC_ACP) && CONFIG_ENABLE_EMMC_ACP

    return DMA_ACP;

#else

    if (data->flags & MMC_DATA_WRITE)
        return DMA_TO_DEVICE;
    else
        return DMA_FROM_DEVICE;

#endif
}

#if !(defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM)
static int mstar_mci_config_ecsd(eMMC_IP_EmType emmc_ip, struct mmc_data *pData_st)
{
    struct scatterlist *pSG_st       = 0;
    dma_addr_t          cpu_addr_phy = 0;
    int                 err          = 0;
    u8 *                pBuf;
#if 0
    int i;
#endif

    if (!pData_st)
    {
        return -EINVAL;
    }

    if (0 == (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_INIT_DONE))
    {
        pSG_st       = &pData_st->sg[0];
        cpu_addr_phy = sg_dma_address(pSG_st);

        pBuf = (u8 *)phys_to_virt(cpu_addr_phy);
#if 0
        for(i=0; i<512; i++)
        {
            if( (i&0xF) == 0x0 ) eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 0, "%03X: ", i);
            eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 0, "%02X ", pBuf[i]);
            if( (i&0xF) == 0xF ) eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 0, "\n");
        }
#endif
        //--------------------------------
        if (0 == g_eMMCDrv[emmc_ip].u32_SEC_COUNT)
            g_eMMCDrv[emmc_ip].u32_SEC_COUNT = ((pBuf[215] << 24) | (pBuf[214] << 16) | (pBuf[213] << 8) | (pBuf[212]))
                                               - 8; //-8: Toshiba CMD18 access the last block report out of range error

        //-------------------------------
        if (0 == g_eMMCDrv[emmc_ip].u32_BOOT_SEC_COUNT)
            g_eMMCDrv[emmc_ip].u32_BOOT_SEC_COUNT = pBuf[226] * 128 * 2;

        //--------------------------------
        if (!g_eMMCDrv[emmc_ip].u8_BUS_WIDTH)
        {
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = pBuf[183];

            switch (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH)
            {
                case 0:
                    g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_1;
                    break;
                case 1:
                    g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_4;
                    break;
                case 2:
                    g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_8;
                    break;
                default:
                    eMMC_debug(0, 1, "eMMC Err: eMMC BUS_WIDTH not support \n");
                    while (1)
                        ;
            }
        }

        //--------------------------------
        if (pBuf[231] & BIT4) // TRIM
            g_eMMCDrv[emmc_ip].u32_eMMCFlag |= eMMC_FLAG_TRIM;
        else
            g_eMMCDrv[emmc_ip].u32_eMMCFlag &= ~eMMC_FLAG_TRIM;

        //--------------------------------
        if (pBuf[503] & BIT0) // HPI
        {
            if (pBuf[503] & BIT1)
                g_eMMCDrv[emmc_ip].u32_eMMCFlag |= eMMC_FLAG_HPI_CMD12;
            else
                g_eMMCDrv[emmc_ip].u32_eMMCFlag |= eMMC_FLAG_HPI_CMD13;
        }
        else
            g_eMMCDrv[emmc_ip].u32_eMMCFlag &= ~(eMMC_FLAG_HPI_CMD12 | eMMC_FLAG_HPI_CMD13);

        //--------------------------------
        if (pBuf[166] & BIT2) // Reliable Write
            g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = BIT_SD_JOB_BLK_CNT_MASK;
        else
        {
#if 0
            g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = pBuf[222];
#else
            if ((pBuf[503] & BIT0) && 1 == pBuf[222])
                g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = 1;
            else if (0 == (pBuf[503] & BIT0))
                g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = pBuf[222];
            else
            {
                // eMMC_debug(0,1,"eMMC Warn: not support dynamic  Reliable-W\n");
                g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = 0; // can not support Reliable Write
            }
#endif
        }

        //--------------------------------
        g_eMMCDrv[emmc_ip].u8_ErasedMemContent = pBuf[181];

        //--------------------------------
        g_eMMCDrv[emmc_ip].u8_ECSD184_Stroe_Support  = pBuf[184];
        g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming       = pBuf[185];
        g_eMMCDrv[emmc_ip].u8_ECSD192_Ver            = pBuf[192];
        g_eMMCDrv[emmc_ip].u8_ECSD196_DevType        = pBuf[196];
        g_eMMCDrv[emmc_ip].u8_ECSD197_DriverStrength = pBuf[197];
        g_eMMCDrv[emmc_ip].u8_ECSD248_CMD6TO         = pBuf[248];
        g_eMMCDrv[emmc_ip].u8_ECSD247_PwrOffLongTO   = pBuf[247];
        g_eMMCDrv[emmc_ip].u8_ECSD34_PwrOffCtrl      = pBuf[34];

        // for GP Partition
        g_eMMCDrv[emmc_ip].u8_ECSD160_PartSupField   = pBuf[160];
        g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize = pBuf[224];
        g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize    = pBuf[221];

        g_eMMCDrv[emmc_ip].GP_Part[0].u32_PartSize =
            ((pBuf[145] << 16) | (pBuf[144] << 8) | (pBuf[143]))
            * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

        g_eMMCDrv[emmc_ip].GP_Part[1].u32_PartSize =
            ((pBuf[148] << 16) | (pBuf[147] << 8) | (pBuf[146]))
            * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

        g_eMMCDrv[emmc_ip].GP_Part[2].u32_PartSize =
            ((pBuf[151] << 16) | (pBuf[150] << 8) | (pBuf[149]))
            * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

        g_eMMCDrv[emmc_ip].GP_Part[3].u32_PartSize =
            ((pBuf[154] << 16) | (pBuf[153] << 8) | (pBuf[152]))
            * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

        // for Max Enhance Size
        g_eMMCDrv[emmc_ip].u8_ECSD157_MaxEnhSize_0 = pBuf[157];
        g_eMMCDrv[emmc_ip].u8_ECSD158_MaxEnhSize_1 = pBuf[158];
        g_eMMCDrv[emmc_ip].u8_ECSD159_MaxEnhSize_2 = pBuf[159];

        g_eMMCDrv[emmc_ip].u8_u8_ECSD155_PartSetComplete = pBuf[155];
        g_eMMCDrv[emmc_ip].u8_ECSD166_WrRelParam         = pBuf[166];

        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_INIT_DONE;
    }

    return err;
}
#endif
static U32 _TransArrToUInt(U8 u8Sep1, U8 u8Sep2, U8 u8Sep3, U8 u8Sep4)
{
    return ((((uint)u8Sep1) << 24) | (((uint)u8Sep2) << 16) | (((uint)u8Sep3) << 8) | ((uint)u8Sep4));
}

static eMMC_Rsp_EmType _GetTransRspType(unsigned int u32Rsp)
{
    switch (u32Rsp)
    {
        case MMC_RSP_NONE:
            return eMMC_NO;
        case MMC_RSP_R1:
            // case MMC_RSP_R5:
            // case MMC_RSP_R6:
            // case MMC_RSP_R7:
            return eMMC_R1;
        case MMC_RSP_R1B:
            return eMMC_R1B;
        case MMC_RSP_R2:
            return eMMC_R2;
        case MMC_RSP_R3:
            // case MMC_RSP_R4:
            return eMMC_R3;
        default:
            return eMMC_R1;
    }
}

static eMMC_Cmd_EmType _GetTransCmdType(U32 u32_Flag)
{
    switch (u32_Flag & (MMC_DATA_READ | MMC_DATA_WRITE))
    {
        case MMC_DATA_READ:
            return eMMC_CMD_READ;

        case MMC_DATA_WRITE:
            return eMMC_CMD_WRITE;

        default:
            return eMMC_CMD_RSP;
    }
}

static U32 mstar_mci_pre_data_buffer(struct mstar_mci_host *pSstarHost_st, struct _AdmaDescriptor *p_AdmaDesc_st)
{
    struct mmc_command *pCmd_st      = pSstarHost_st->request->cmd;
    struct mmc_data *   pData_st     = pCmd_st->data;
    struct scatterlist *pSG_st       = pData_st->sg;
    u32                 dmalen       = 0;
    dma_addr_t          cpu_addr_phy = 0;
    dma_addr_t          miu_addr_phy = 0;
    eMMC_IP_EmType      emmc_ip      = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];
    U32                 u32MiuSel;
    int                 i;

    if (pData_st->sg_len > FCIE_ADMA_DESC_COUNT)
    {
        eMMC_die(
            "mstar_mci_pre_adma_read: sglist has more than FCIE_ADMA_DESC_COUNT items. Must change 512 to larger "
            "value.\n");
    }

#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
    mstar_mci_pre_dma_transfer(pSstarHost_st, pData_st, NULL);
#else
    dma_map_sg(mmc_dev(pSstarHost_st->mmc), pData_st->sg, pData_st->sg_len, mstar_mci_get_dma_dir(pData_st));
#endif

    if (pData_st->sg_len > FCIE_ADMA_DESC_COUNT)
    {
        eMMC_die(
            "mstar_mci_pre_adma_read: sglist has more than FCIE_ADMA_DESC_COUNT items. Must change 512 to larger "
            "value.\n");
    }

    memset(p_AdmaDesc_st, 0, sizeof(struct _AdmaDescriptor) * FCIE_ADMA_DESC_COUNT);

    for (i = 0; i < pData_st->sg_len; i++)
    {
        cpu_addr_phy = sg_dma_address(pSG_st);
        dmalen       = sg_dma_len(pSG_st);

        miu_addr_phy = eMMC_Platform_Trans_Dma_Addr(emmc_ip, cpu_addr_phy, &u32MiuSel);

        p_AdmaDesc_st[i].u32_Address    = (U32)miu_addr_phy;
        p_AdmaDesc_st[i].u32_DmaAddrMSB = (U32)(((U64)miu_addr_phy >> 32) & 0xF);
        p_AdmaDesc_st[i].u32_DmaLen     = dmalen;
        p_AdmaDesc_st[i].u32_MiuSel     = u32MiuSel;
        if (dmalen >= 0x200)
        {
            p_AdmaDesc_st[i].u32_JobCnt = (dmalen >> 9);
            // eMMC_debug(0,0,"  %Xh JobCnt\n", (dmalen >> 9));
        }
        else
        { // should be only one sg element
            p_AdmaDesc_st[i].u32_JobCnt = 1;
            REG_FCIE_W(FCIE_BLK_SIZE(emmc_ip), dmalen);
            // eMMC_debug(0,0,"  %Xh bytes\n", dmalen);
        }

        pSG_st = sg_next(pSG_st);

#if defined(eMMC_PROFILE_WR) && eMMC_PROFILE_WR
        g_eMMCDrv[emmc_ip].u64_CNT_TotalRBlk += (dmalen / 512);
        g_eMMCDrv[emmc_ip].u32_RBlk_tmp += (dmalen / 512);
#endif
    }

    if (gu32_eMMC_monitor_enable)
    {
        gu64_jiffies_org[emmc_ip] = jiffies_64;
    }

    p_AdmaDesc_st[pData_st->sg_len - 1].u32_End = 1;

    if (gu32AdmaMode[emmc_ip])
        memcpy(pSstarHost_st->adma_buffer, p_AdmaDesc_st, sizeof(struct _AdmaDescriptor) * pData_st->sg_len);

    // Flush L1,L2,L3, For gAdmaDesc_st.
    Chip_Flush_Cache_Range((void *)p_AdmaDesc_st, sizeof(struct _AdmaDescriptor) * pData_st->sg_len);
    // Flush L3, For sg_buffer DMA_FROM_DEVICE.
    Chip_Flush_MIU_Pipe();

    return pData_st->sg_len;
}

static void mstar_mci_post_data_buffer(struct mstar_mci_host *pSstarHost_st)
{
    /* Define Local Variables */
    struct mmc_command *pCmd_st  = pSstarHost_st->request->cmd;
    struct mmc_data *   pData_st = pCmd_st->data;
    eMMC_IP_EmType      emmc_ip  = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];

#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
    if (!pData_st->host_cookie)
    {
        dma_unmap_sg(mmc_dev(pSstarHost_st->mmc), pData_st->sg, (int)pData_st->sg_len, mstar_mci_get_dma_dir(pData_st));
    }
#else
    dma_unmap_sg(mmc_dev(pSstarHost_st->mmc), pData_st->sg, pData_st->sg_len, mstar_mci_get_dma_dir(pData_st));
#endif

    if (pCmd_st->opcode == 17 || pCmd_st->opcode == 18)
    {
        if (gu32_eMMC_read_log_enable)
            eMMC_debug(0, 0, "\ncmd:%u ,arg:%xh, blk cnt:%xh\n", pCmd_st->opcode, pCmd_st->arg, pData_st->blocks);

        if (gu32_eMMC_monitor_enable)
        {
            gu64_jiffies_read[emmc_ip] += (jiffies_64 - gu64_jiffies_org[emmc_ip]);
            gu32_eMMC_read_cnt[emmc_ip] = pData_st->bytes_xfered >> 9;
        }
    }
    else if (pCmd_st->opcode == 24 || pCmd_st->opcode == 25)
    {
        if (gu32_eMMC_write_log_enable)
            eMMC_debug(0, 0, "\ncmd:%u ,arg:%xh, blk cnt:%xh\n", pCmd_st->opcode, pCmd_st->arg, pData_st->blocks);

        if (gu32_eMMC_monitor_enable)
        {
            gu64_jiffies_write[emmc_ip] += (jiffies_64 - gu64_jiffies_org[emmc_ip]);
            gu32_eMMC_write_cnt[emmc_ip] = pData_st->bytes_xfered >> 9;
        }
    }
}

static U32 mstar_mci_completed_command(struct mstar_mci_host *pSstarHost_st, U32 u32_err)
{
    /* Define Local Variables */
    struct mmc_command *pCmd_st = pSstarHost_st->request->cmd;
    eMMC_IP_EmType      emmc_ip = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];
    U32                 err;
    u8                  pTemp[0x10];

    switch (u32_err)
    {
        case eMMC_ST_SUCCESS:
            err = eMMC_ST_SUCCESS;
            break;

        case eMMC_ST_ERR_TIMEOUT_WAIT_REG0:
        case eMMC_ST_ERR_TIMEOUT_WAITD0HIGH:
        case eMMC_ST_ERR_REG_WR_TOUT:
        case eMMC_ST_ERR_REG_NO_RSP:
        case eMMC_ST_ERR_REG_RD_TOUT:
            err = -ETIMEDOUT;
            break;

        case eMMC_ST_ERR_REG_RD_CRC:
        case eMMC_ST_ERR_REG_WR_CRC:
        case eMMC_ST_ERR_REG_RSP_CRC:
            err = -EILSEQ;
            break;

        default:
            err = -EIO;
            break;
    }

    eMMC_FCIE_GetCMDFIFO(emmc_ip, 0, 8, (U16 *)pTemp);
    pCmd_st->resp[0] = _TransArrToUInt(pTemp[1], pTemp[2], pTemp[3], pTemp[4]);
    if (1)
    {
        pCmd_st->resp[1] = _TransArrToUInt(pTemp[5], pTemp[6], pTemp[7], pTemp[8]);
        pCmd_st->resp[2] = _TransArrToUInt(pTemp[9], pTemp[10], pTemp[11], pTemp[12]);
        pCmd_st->resp[3] = _TransArrToUInt(pTemp[13], pTemp[14], pTemp[15], 0);
    }

#if defined(eMMC_PROFILE_WR) && eMMC_PROFILE_WR
    if (g_eMMCDrv[emmc_ip].u32_RBlk_tmp < 0x200)
        g_eMMCDrv[emmc_ip].au32_CNT_MinRBlk[g_eMMCDrv[emmc_ip].u32_RBlk_tmp]++;
    if (g_eMMCDrv[emmc_ip].u32_RBlk_tmp > g_eMMCDrv[emmc_ip].u32_CNT_MaxRBlk)
        g_eMMCDrv[emmc_ip].u32_CNT_MaxRBlk = g_eMMCDrv[emmc_ip].u32_RBlk_tmp;
    if (g_eMMCDrv[emmc_ip].u32_RBlk_tmp < g_eMMCDrv[emmc_ip].u32_CNT_MinRBlk)
        g_eMMCDrv[emmc_ip].u32_CNT_MinRBlk = g_eMMCDrv[emmc_ip].u32_RBlk_tmp;

    g_eMMCDrv[emmc_ip].u32_Addr_RLast += g_eMMCDrv[emmc_ip].u32_RBlk_tmp;
    g_eMMCDrv[emmc_ip].u32_RBlk_tmp = 0;
    switch (pCmd_st->opcode)
    {
        case 17:
        case 18:
            if (18 == pCmd_st->opcode)
                g_eMMCDrv[emmc_ip].u32_CNT_CMD18++;
            else if (17 == pCmd_st->opcode)
                g_eMMCDrv[emmc_ip].u32_CNT_CMD17++;

            if (g_eMMCDrv[emmc_ip].u32_Addr_RLast == pCmd_st->arg)
                g_eMMCDrv[emmc_ip].u32_Addr_RHitCnt++;
            else
                g_eMMCDrv[emmc_ip].u32_Addr_RLast = pCmd_st->arg;

        case 24:
        case 25:
            if (25 == pCmd_st->opcode)
                g_eMMCDrv[emmc_ip].u32_CNT_CMD25++;
            else if (24 == pCmd_st->opcode)
                g_eMMCDrv[emmc_ip].u32_CNT_CMD24++;

            if (g_eMMCDrv[emmc_ip].u32_Addr_WLast == pCmd_st->arg)
                g_eMMCDrv[emmc_ip].u32_Addr_WHitCnt++;
            else
                g_eMMCDrv[emmc_ip].u32_Addr_WLast = pCmd_st->arg;
    }

#endif

#if defined(ENABLE_EMMC_POWER_SAVING_MODE) && ENABLE_EMMC_POWER_SAVING_MODE
    // -----------------------------------
    if ((pCmd_st->opcode == 12) || (pCmd_st->opcode == 24) || (pCmd_st->opcode == 25))
    {
        eMMC_CheckPowerCut();
    }
#endif

    return err;
}

#define WAIT_D0H_POLLING_TIME HW_TIMER_DELAY_100us
u32 mstar_mci_WaitD0High(eMMC_IP_EmType emmc_ip, u32 u32_us)
{
#if defined(ENABLE_FCIE_HW_BUSY_CHECK) && ENABLE_FCIE_HW_BUSY_CHECK

    REG_FCIE_SETBIT(FCIE_SD_CTRL(emmc_ip), BIT_BUSY_DET_ON);

#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
    // enable busy int
    REG_FCIE_SETBIT(FCIE_MIE_INT_EN(emmc_ip), BIT_BUSY_END_INT);
#endif

    if (eMMC_FCIE_WaitEvents(emmc_ip, FCIE_MIE_EVENT(emmc_ip), BIT_BUSY_END_INT, u32_us) != eMMC_ST_SUCCESS)
    {
        return eMMC_ST_ERR_TIMEOUT_WAITD0HIGH;
    }

    return eMMC_ST_SUCCESS;

#else

    u32 u32_cnt, u32_wait;

    for (u32_cnt = 0; u32_cnt < u32_us; u32_cnt += WAIT_D0H_POLLING_TIME)
    {
        u32_wait = eMMC_FCIE_WaitD0High_Ex(emmc_ip, WAIT_D0H_POLLING_TIME);

        if (u32_wait < WAIT_D0H_POLLING_TIME)
        {
#if 0
            if(u32_cnt + u32_wait)
                printk("eMMC wait d0: %u us\n", u32_cnt+u32_wait);
#endif

            return eMMC_ST_SUCCESS;
        }

        // if(u32_cnt > HW_TIMER_DELAY_1ms)
        {
            // msleep(1);
            // schedule_hrtimeout is more precise and can reduce idle time of emmc

            {
                ktime_t expires = ktime_add_ns(ktime_get(), 1000 * 1000);
                set_current_state(TASK_UNINTERRUPTIBLE);
                schedule_hrtimeout(&expires, HRTIMER_MODE_ABS);
            }

            u32_cnt += HW_TIMER_DELAY_1ms;
        }
    }

    return eMMC_ST_ERR_TIMEOUT_WAITD0HIGH;

#endif
}

#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
static void mstar_mci_send_data(struct work_struct *work)
{
    struct mstar_mci_host *pSstarHost_st = container_of(work, struct mstar_mci_host, async_work);
    struct mmc_command *   pCmd_st       = pSstarHost_st->request->cmd;
    struct mmc_data *      pData_st      = pCmd_st->data;
    static u8              u8_retry_data = 0;
    U32                    err           = eMMC_ST_SUCCESS;

    mstar_mci_pre_data_buffer(pSstarHost_st);

    err = mstar_mci_post_data_buffer(pSstarHost_st);
    if (err)
    {
        u32_ok_cnt = 0;

        if (pData_st->flags & MMC_DATA_WRITE)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: w, cmd.%u arg.%Xh, ST: %Xh or TO\n", pCmd_st->opcode,
                       pCmd_st->arg, REG_FCIE(FCIE_SD_STATUS(emmc_ip)));
        }
        else if (pData_st->flags & MMC_DATA_READ)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: r, cmd.%u arg.%Xh, ST: %Xh or TO \n", pCmd_st->opcode,
                       pCmd_st->arg, REG_FCIE(FCIE_SD_STATUS(emmc_ip)));
        }

        if (u8_retry_data < MCI_RETRY_CNT_CMD_TO)
        {
            u8_retry_data++;
            eMMC_FCIE_ErrHandler_ReInit(emmc_ip);
            eMMC_FCIE_ErrHandler_Retry(emmc_ip);

#if defined(ENABLE_EMMC_PRE_DEFINED_BLK) && ENABLE_EMMC_PRE_DEFINED_BLK
            if (pSstarHost_st->request->sbc)
                mstar_mci_send_command(pSstarHost_st, pSstarHost_st->request->sbc);
            else
#endif
                mstar_mci_send_command(pSstarHost_st, pSstarHost_st->request->cmd);
        }
        else
        {
#if defined(ENABLE_FCIE_ADMA) && ENABLE_FCIE_ADMA
            eMMC_dump_mem((U8 *)gAdmaDesc_st, (U32)(sizeof(struct _AdmaDescriptor) * (pData_st->sg_len + 1)));
#endif
            eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        }
    }
    else
    {
        if (u8_retry_data)
            eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC: cmd.%u arg.%Xh: data retry ok \n", pCmd_st->opcode, pCmd_st->arg);

#if !(defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM)
        if (pCmd_st->opcode == 8)
        {
            mstar_mci_config_ecsd(emmc_ip, pData_st);
        }
#endif

        u8_retry_data = 0;
        if (gu32_eMMC_monitor_enable)
        {
            if ((pCmd_st->opcode == 17) || (pCmd_st->opcode == 18))
                gu64_jiffies_read[emmc_ip] += (jiffies_64 - gu64_jiffies_org[emmc_ip]);
            else if ((pCmd_st->opcode == 24) || (pCmd_st->opcode == 25))
                gu64_jiffies_write[emmc_ip] += (jiffies_64 - gu64_jiffies_org[emmc_ip]);
        }

        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        mmc_request_done(pSstarHost_st->mmc, pSstarHost_st->request);
    }
}
#endif

static void mstar_mci_send_command(struct mstar_mci_host *pSstarHost_st, struct mmc_command *pCmd_st)
{
    struct mmc_data *       pData_st;
    eMMC_IP_EmType          emmc_ip;
    U32                     u32_TransType = EMMC_EMP;
    U32                     u32_i, u32TransCnt;
    struct _AdmaDescriptor *p_AdmaDesc_st;
    dma_addr_t              cpu_addr_phy = 0;

    u32 err = 0;

    // eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC: cmd.%u arg.%Xh\n", pCmd_st->opcode, pCmd_st->arg);
    emmc_ip = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];

    pData_st = pCmd_st->data;
    if (!pData_st)
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= (DRV_FLAG_ERROR_RETRY);
    else
        g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~(DRV_FLAG_ERROR_RETRY);

    eMMC_FCIE_SetCmdTaken(emmc_ip, pCmd_st->arg, pCmd_st->opcode);

    if (pData_st)
    {
        pData_st->bytes_xfered = 0;

        if (gu32AdmaMode[emmc_ip])
            u32_TransType = EMMC_ADMA;
        else
            u32_TransType = EMMC_DMA;

        p_AdmaDesc_st = kmalloc(sizeof(struct _AdmaDescriptor) * FCIE_ADMA_DESC_COUNT, GFP_KERNEL | GFP_DMA);
        u32TransCnt   = mstar_mci_pre_data_buffer(pSstarHost_st, p_AdmaDesc_st);

        cpu_addr_phy = (p_AdmaDesc_st[0].u32_Address | ((U64)p_AdmaDesc_st[0].u32_DmaAddrMSB << 32));

        eMMC_FCIE_TransCmdSetting(emmc_ip, p_AdmaDesc_st[0].u32_JobCnt, pCmd_st->data->blksz,
                                  u32_TransType == EMMC_ADMA
                                      ? eMMC_Platform_Trans_Dma_Addr(emmc_ip, pSstarHost_st->adma_phy_addr, NULL)
                                      : cpu_addr_phy,
                                  u32_TransType);

        err =
            eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, pCmd_st->opcode, u32_TransType, _GetTransCmdType(pData_st->flags),
                                            _GetTransRspType(mmc_resp_type(pCmd_st)));
        if (!err)
        {
            if (u32_TransType == EMMC_ADMA)
            {
                pData_st->bytes_xfered += (pCmd_st->data->blksz * pCmd_st->data->blocks);
            }
            else if (u32_TransType == EMMC_DMA)
            {
                pData_st->bytes_xfered += (p_AdmaDesc_st[0].u32_JobCnt * eMMC_SECTOR_512BYTE);
                for (u32_i = 1; u32_i < u32TransCnt; u32_i++)
                {
                    cpu_addr_phy =
                        (p_AdmaDesc_st[u32_i].u32_Address | ((U64)p_AdmaDesc_st[u32_i].u32_DmaAddrMSB << 32));
                    eMMC_FCIE_TransCmdSetting(emmc_ip, p_AdmaDesc_st[u32_i].u32_JobCnt, eMMC_SECTOR_512BYTE,
                                              cpu_addr_phy, u32_TransType);

                    err = eMMC_FCIE_JobStartAndWaitEvent(emmc_ip, u32_TransType, _GetTransCmdType(pData_st->flags),
                                                         BIT_DMA_END, eMMC_GENERIC_WAIT_TIME);
                    if (err)
                        break;

                    pData_st->bytes_xfered += p_AdmaDesc_st[u32_i].u32_JobCnt * eMMC_SECTOR_512BYTE;
                }
            }
        }

        mstar_mci_post_data_buffer(pSstarHost_st);

#if !(defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM)
        if (pCmd_st->opcode == 8)
            mstar_mci_config_ecsd(emmc_ip, pData_st);
#endif
        kfree(p_AdmaDesc_st);
    }
    else
    {
        err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, pCmd_st->opcode, u32_TransType, eMMC_CMD_RSP,
                                              _GetTransRspType(mmc_resp_type(pCmd_st)));
    }

    pCmd_st->error = mstar_mci_completed_command(pSstarHost_st, err);
    if (pData_st)
        pData_st->error = pCmd_st->error;

    if ((pCmd_st->opcode == 18 || pCmd_st->opcode == 25)
#if defined(ENABLE_EMMC_PRE_DEFINED_BLK) && ENABLE_EMMC_PRE_DEFINED_BLK
        && !pSstarHost_st->request->sbc
#endif
    )
    {
        mstar_mci_send_command(pSstarHost_st, pSstarHost_st->request->stop); // CMD12
    }
    else if (pCmd_st->opcode == 23)
        mstar_mci_send_command(pSstarHost_st, pSstarHost_st->request->cmd); // CMD18 or CMD25
    else
    {
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        mmc_request_done(pSstarHost_st->mmc, pSstarHost_st->request);
    }
}

#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
static int mstar_mci_pre_dma_transfer(struct mstar_mci_host *host, struct mmc_data *data,
                                      struct mstar_mci_host_next *next)
{
    int dma_len;

    /* Check if next job is already prepared */
    if (next || (!next && data->host_cookie != host->next_data.cookie))
    {
        dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len, mstar_mci_get_dma_dir(data));
    }
    else
    {
        dma_len                 = host->next_data.dma_len;
        host->next_data.dma_len = 0;
    }

    if (dma_len == 0)
        return -EINVAL;

    if (next)
    {
        next->dma_len     = dma_len;
        data->host_cookie = ++next->cookie < 0 ? 1 : next->cookie;
    }

    return 0;
}

static void mstar_mci_pre_req(struct mmc_host *mmc, struct mmc_request *mrq, bool is_first_req)
{
    struct mstar_mci_host *host = mmc_priv(mmc);

    eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 1, "\n");

    if (mrq->data->host_cookie)
    {
        mrq->data->host_cookie = 0;
        return;
    }

    if (mstar_mci_pre_dma_transfer(host, mrq->data, &host->next_data))
        mrq->data->host_cookie = 0;
}

static void mstar_mci_post_req(struct mmc_host *mmc, struct mmc_request *mrq, int err)
{
    struct mstar_mci_host *host = mmc_priv(mmc);
    struct mmc_data *      data = mrq->data;

    eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 1, "\n");

    if (data->host_cookie)
    {
        dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, mstar_mci_get_dma_dir(data));
    }

    data->host_cookie = 0;
}
#endif

static void mstar_mci_request(struct mmc_host *pMMCHost_st, struct mmc_request *pMRQ_st)
{
    struct mstar_mci_host *pSstarHost_st;
    eMMC_IP_EmType         emmc_ip;

    pSstarHost_st = mmc_priv(pMMCHost_st);
    if (!pMMCHost_st)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: pMMCHost_st is NULL \n");
        return;
    }

    if (!pMRQ_st)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: pMRQ_st is NULL \n");
        return;
    }

    pSstarHost_st->request = pMRQ_st;
    emmc_ip                = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];

    // SD command filter
    if ((pSstarHost_st->request->cmd->opcode == 52) || (pSstarHost_st->request->cmd->opcode == 55)
        || ((pSstarHost_st->request->cmd->opcode == 8) && pSstarHost_st->request->cmd->arg)
        || ((pSstarHost_st->request->cmd->opcode == 5) && (pSstarHost_st->request->cmd->arg == 0)))
    {
        pSstarHost_st->request->cmd->error = -ETIMEDOUT;

        mmc_request_done(pSstarHost_st->mmc, pSstarHost_st->request);

        return;
    }

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

// ---------------------------------------------
#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
    if (0 == (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_INIT_DONE))
    {
        eMMC_Init_Device();
    }

#if 0 // def CONFIG_MP_EMMC_TRIM
    switch(pSstarHost_st->request->cmd->opcode)
    {
        case 35:
            eMMC_CMD35_CMD36(pSstarHost_st->request->cmd->arg, 35);
            break;
        case 36:
            eMMC_CMD35_CMD36(pSstarHost_st->request->cmd->arg, 36);
            break;
        case 38:
            eMMC_CMD38();
            break;
        default:
            break;
    }
#endif

    if (NULL == pSstarHost_st->request->cmd->data && 6 != pSstarHost_st->request->cmd->opcode)
    {
        pSstarHost_st->cmd = pSstarHost_st->request->cmd;
        mstar_mci_completed_command_FromRAM(pSstarHost_st);
        return;
    }

#if 0
    if(6 == pSstarHost_st->request->cmd->opcode &&
       (((pSstarHost_st->request->cmd->arg & 0xff0000) == 0xB70000)||
        ((pSstarHost_st->request->cmd->arg & 0xff0000) == 0xB90000)))
    {
        pSstarHost_st->cmd = pSstarHost_st->request->cmd;
        mstar_mci_completed_command_FromRAM(pSstarHost_st);
        return;
    }
#endif

#endif

// ---------------------------------------------
#if defined(ENABLE_EMMC_PRE_DEFINED_BLK) && ENABLE_EMMC_PRE_DEFINED_BLK
    if (pSstarHost_st->request->sbc)
        mstar_mci_send_command(pSstarHost_st, pSstarHost_st->request->sbc);
    else
#endif
        mstar_mci_send_command(pSstarHost_st, pSstarHost_st->request->cmd);
}

#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
static void mstar_mci_completed_command_FromRAM(struct mstar_mci_host *pSstarHost_st)
{
    struct mmc_command *pCmd_st       = pSstarHost_st->cmd;
    u16                 au16_cifc[32] = {0};
    u16                 u16_i;
    u8 *                pTemp;

#if 0
    eMMC_debug(0,1,"\n");
    eMMC_debug(0,1,"cmd:%u, arg:%Xh\n", pCmd_st->opcode, pCmd_st->arg);
#endif

    if (eMMC_ST_SUCCESS != eMMC_ReturnRsp((u8 *)au16_cifc, (u8)pCmd_st->opcode))
    {
        if (mmc_resp_type(pCmd_st) != MMC_RSP_NONE)
        {
            pCmd_st->error = -ETIMEDOUT;
            eMMC_debug(0, 0, "eMMC Info: no rsp\n");
        }
    }
    else
    {
        pCmd_st->error = 0;
        pTemp          = (u8 *)&(pCmd_st->resp[0]);
        for (u16_i = 0; u16_i < 15; u16_i++)
        {
            pTemp[(3 - (u16_i % 4)) + 4 * (u16_i / 4)] = (u8)(au16_cifc[(u16_i + 1) / 2] >> 8 * ((u16_i + 1) % 2));
        }
#if 0
        eMMC_debug(0,1,"------------------\n");
        eMMC_dump_mem((u8*)&(pCmd_st->resp[0]), 0x10);
        eMMC_debug(0,1,"------------------\n");
#endif
    }

    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    mmc_request_done(pSstarHost_st->mmc, pSstarHost_st->request);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
static u8 u8_cur_timing[EMMC_NUM_TOTAL] = {0};

#if 0 //(defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
static u32 mstar_mci_read_blocks(struct mmc_host *host, u8 *buf, ulong blkaddr, ulong blkcnt)
{
    struct mmc_request mrq = {NULL};
    struct mmc_command cmd = {0};
    struct mmc_data data = {0};
    struct scatterlist sg;

    mrq.cmd = &cmd;
    mrq.data = &data;

    cmd.opcode = MMC_READ_SINGLE_BLOCK;
    cmd.arg = blkaddr;

    cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

    data.blksz = 512;
    data.blocks = blkcnt;
    data.flags = MMC_DATA_READ;
    data.sg = &sg;
    data.sg_len = 1;

    sg_init_one(&sg, buf, 512*blkcnt);

    data.timeout_ns = TIME_WAIT_1_BLK_END * 1000;
    data.timeout_clks = 0;

    mmc_wait_for_req(host, &mrq);

    if (cmd.error)
        return cmd.error;

    if (data.error)
        return data.error;

    return 0;
}
#endif

#endif

static void mstar_mci_set_ios(struct mmc_host *pMMCHost_st, struct mmc_ios *pIOS_st)
{
    /* Define Local Variables */
    struct mstar_mci_host *pSstarHost_st = mmc_priv(pMMCHost_st);
    eMMC_IP_EmType         emmc_ip       = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];
    U16                    u16_ClkParam;

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
    if (pIOS_st->clock == 0)
    {
        // eMMC_debug(eMMC_DEBUG_LEVEL_HIGH, 1, "eMMC Warn: disable clk \n");
        // eMMC_clock_gating(emmc_ip);
        _eMMC_set_bustiming(emmc_ip, FCIE_eMMC_BYPASS);
        eMMC_clock_setting(emmc_ip, FCIE_SLOWEST_CLK);
    }
    else
        eMMC_clock_setting(emmc_ip, g_eMMCDrv[emmc_ip].u16_ClkRegVal);

#else
    /*****************************switch clk setting*************************************/
    if ((g_eMMCDrv[emmc_ip].u32_ClkKHz * 1000) != pIOS_st->clock)
    {
        u16_ClkParam = eMMC_Find_Clock_Reg(emmc_ip, pIOS_st->clock);
        eMMC_clock_setting(emmc_ip, u16_ClkParam);
    }

    /*****************************switch bus timing*************************************/
    if (u8_cur_timing[emmc_ip] != pIOS_st->timing)
    {
        switch (pIOS_st->timing)
        {
            case MMC_TIMING_LEGACY:
                _eMMC_set_bustiming(emmc_ip, FCIE_eMMC_BYPASS);
                break;

            case MMC_TIMING_MMC_HS:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
                _eMMC_set_bustiming(emmc_ip, FCIE_eMMC_SDR);
#else
                _eMMC_set_bustiming(emmc_ip, FCIE_eMMC_BYPASS);
#endif
                break;

            case MMC_TIMING_UHS_DDR50:
                eMMC_FCIE_EnableSDRMode(emmc_ip);
                if (g_eMMCDrv[emmc_ip].u8_ECSD196_DevType & eMMC_DEVTYPE_DDR)
                {
                    if (eMMC_ST_SUCCESS != eMMC_FCIE_EnableFastMode(emmc_ip, FCIE_eMMC_DDR))
                    {
                        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_FCIE_EnableFastMode DDR fail\n");
                        eMMC_debug(0, 0, "\neMMC: SDR %uMHz \n", g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000);
                    }
                    else
                        eMMC_debug(0, 0, "\neMMC: DDR %uMHz \n", g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000);
                }
                break;

            case MMC_TIMING_MMC_HS200:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
#if defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200
                _eMMC_set_bustiming(emmc_ip, FCIE_eMMC_HS200);
                eMMC_clock_setting(emmc_ip, eMMC_PLL_CLK_200M);
#endif
#endif
                break;

            case MMC_TIMING_MMC_HS400:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
#if defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400
                _eMMC_set_bustiming(emmc_ip, FCIE_eMMC_HS400);
                eMMC_clock_setting(emmc_ip, eMMC_PLL_CLK_200M_HS400);
#endif
#endif
                break;
        }
#endif
    u8_cur_timing[emmc_ip] = pIOS_st->timing;
}

/*****************************switch bus width*************************************/
if (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH != pIOS_st->bus_width)
{
    switch (pIOS_st->bus_width)
    {
        case MMC_BUS_WIDTH_8:
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_8;
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode =
                (g_eMMCDrv[emmc_ip].u16_Reg10_Mode & ~BIT_SD_DATA_WIDTH_MASK) | BIT_SD_DATA_WIDTH_8;
            break;

        case MMC_BUS_WIDTH_4:
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_4;
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode =
                (g_eMMCDrv[emmc_ip].u16_Reg10_Mode & ~BIT_SD_DATA_WIDTH_MASK) | BIT_SD_DATA_WIDTH_4;
            break;

        case MMC_BUS_WIDTH_1:
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_1;
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode =
                (g_eMMCDrv[emmc_ip].u16_Reg10_Mode & ~BIT_SD_DATA_WIDTH_MASK) | BIT_SD_DATA_WIDTH_1;
            break;

        default:
            printk("[EMMC-%s]err: fail to switch bus width(%d)\n", __FUNCTION__, pIOS_st->bus_width);
    }
}

eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
}

#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
BOOL Hal_eMMC_SavePassPhase(int eIP, U8 u8Phase, BOOL bCleanFlag)
{
    U8 u8Pos = 0;

    if (u8Phase >= V_MAX_PHASE)
        return 1; // Over V_MAX_PHASE

    // Clean All Phase Table
    if (bCleanFlag)
    {
        gu8_PASS_PH_CNT[eIP] = 0;
        for (u8Pos = 0; u8Pos < V_MAX_PHASE; u8Pos++)
            gu8Arr_PASS_PHS[eIP][u8Pos] = 0;
    }
    else
    {
        if (gu8_PASS_PH_CNT[eIP] < V_MAX_PHASE)
        {
            gu8Arr_PASS_PHS[eIP][gu8_PASS_PH_CNT[eIP]] = u8Phase;
            gu8_PASS_PH_CNT[eIP] += 1;
        }
        else
            return 1; // Save too many phases (over V_MAX_PHASE)
    }

    return 0;
}

static void _REG_SetSDRSCanVal(int eIP, U8 u8Skew4, U8 u8Skew4Inv)
{
    REG_FCIE(reg_emmcpll_0x03(eIP)) = (u8Skew4 << 12) & BIT_SKEW4_MASK;
    REG_FCIE(reg_emmcpll_0x69(eIP)) = V_PLL_SKEW_SUM_INIT | ((u8Skew4Inv << SKEW_INV_OFFSET) & R_CLK_SKEW_INV);
}

static void _REG_SetDDRScanVal(int eIP, U8 u8DQS_Mode, U8 u8DQS_DLY_Sel)
{
    REG_FCIE(reg_emmcpll_0x6c(eIP)) = (u8DQS_Mode & BIT_DQS_MODE_MASK);
    REG_FCIE(reg_emmcpll_0x6c(eIP)) |= ((u8DQS_DLY_Sel << 4) & BIT_DQS_DELAY_CELL_MASK);
}

void Hal_eMMC_SetPhase(int eIP, BusEmType eBustype, U8 u8Phase)
{
    if (eBustype == EV_SDR)
    {
        if (u8Phase < 9)
            _REG_SetSDRSCanVal(eIP, u8Phase, 0);
        else
            _REG_SetSDRSCanVal(eIP, u8Phase - 9, 1);
    }
    else if (eBustype == EV_DDR)
    {
        _REG_SetDDRScanVal(eIP, u8Phase + 1, 0);
    }
}

S8 Hal_eMMC_FindFitPhaseSetting(int eIP, U8 u8ScanMaxPhase)
{
    U8   u8ArrPHSegs[V_MAX_PHASE][V_MAX_PHASE] = {{0}, {0}};
    U8   u8ArrPHCntsPerSeg[V_MAX_PHASE]        = {0};
    U8   u8SegNum = 0, u8SegPos = 0, u8SelSegPos = 0, u8PH0RawSegPos = 0, u8PHSMaxRawSegPos = 0;
    U8   u8PassPHPos = 0, u8PHPos = 0, u8PHPos_Inx = 0;
    U8   u8PHCntsAtPH0RawSeg = 0, u8PHCntsAtPHSMaxRawSeg = 0, u8MaxPHCnts = 0;
    BOOL bPH_0_Found = FALSE, bPH_SMax_Found = FALSE;
    S8   s8RetPhase = 0;

    if (!gu8_PASS_PH_CNT[eIP])
        return -1; // No any pass phase
    else if (gu8_PASS_PH_CNT[eIP] > V_MAX_PHASE)
        return -2; // The count of pass phases over the V_MAX_PHASE value.

    // Save Pass Phases to different Segements (Eash has continuous phase numbers)
    for (u8PassPHPos = 0; u8PassPHPos < gu8_PASS_PH_CNT[eIP]; u8PassPHPos++)
    {
        u8ArrPHSegs[u8SegNum][u8PHPos] = gu8Arr_PASS_PHS[eIP][u8PassPHPos];
        u8ArrPHCntsPerSeg[u8SegNum] += 1;
        u8PHPos++;

        if ((u8PassPHPos + 1) == gu8_PASS_PH_CNT[eIP])
            break; // Last PassPHPos
        else if ((gu8Arr_PASS_PHS[eIP][u8PassPHPos] + 1) != (gu8Arr_PASS_PHS[eIP][u8PassPHPos + 1]))
        {
            // Change to Next Segement
            u8SegNum++;
            u8PHPos = 0;
        }
    }

    // Find Phase_0 at first Segement (Window)
    if (!u8ArrPHSegs[0][0])
    {
        bPH_0_Found    = TRUE;
        u8PH0RawSegPos = 0;

        // Find Phase_ScanMax at last Segement (Window)
        for (u8SegPos = 1; u8SegPos <= u8SegNum; u8SegPos++)
        {
            if (u8ArrPHCntsPerSeg[u8SegPos])
            {
                for (u8PHPos = 0; u8PHPos < u8ArrPHCntsPerSeg[u8SegPos]; u8PHPos++)
                {
                    if (u8ArrPHSegs[u8SegPos][u8PHPos] == u8ScanMaxPhase)
                    {
                        bPH_SMax_Found    = TRUE;
                        u8PHSMaxRawSegPos = u8SegPos;
                        break;
                    }
                }
            }
        }
    }
    // u8PHSMaxRawSegPos = 1;

    // Merge first Segment and last Segement if they are cycle
    if (bPH_0_Found && bPH_SMax_Found)
    {
        u8PHCntsAtPH0RawSeg    = u8ArrPHCntsPerSeg[u8PH0RawSegPos];
        u8PHCntsAtPHSMaxRawSeg = u8ArrPHCntsPerSeg[u8PHSMaxRawSegPos];

        if (u8PHCntsAtPH0RawSeg + u8PHCntsAtPHSMaxRawSeg >= V_MAX_PHASE)
            return -3; // The summary of two segs over the V_MAX_PHASE value.

        // Merge two Segments
        u8PHPos_Inx = u8PHCntsAtPHSMaxRawSeg; // 1
        for (u8PHPos = 0; u8PHPos < u8PHCntsAtPH0RawSeg; u8PHPos++)
        {
            u8ArrPHSegs[u8PHSMaxRawSegPos][u8PHPos_Inx] = u8ArrPHSegs[u8PH0RawSegPos][u8PHPos];
            if (++u8PHPos_Inx >= V_MAX_PHASE)
                break;
        }

        u8ArrPHCntsPerSeg[u8PH0RawSegPos] = 0; // u8ArrPHCntsPerSeg[0] = 0;
        u8ArrPHCntsPerSeg[u8PHSMaxRawSegPos] =
            u8PHCntsAtPH0RawSeg + u8PHCntsAtPHSMaxRawSeg; // u8ArrPHCntsPerSeg[1] = 15;
    }

#if 0
    prtstring("\r\n---------------------------[ Dump Segs Table  ] -----------------------------\r\n");

    for(u8SegPos=0; u8SegPos <= u8SegNum ; u8SegPos++)
    {

        if (u8ArrPHCntsPerSeg[u8SegPos])
        {
            for (u8PHPos = 0; u8PHPos < u8ArrPHCntsPerSeg[u8SegPos]; u8PHPos++)
            {
                prtU8(u8ArrPHSegs[u8SegPos][u8PHPos]);
                prtstring(", ");

            }
            prtstring("\r\n");
        }
    }
    prtstring(")\r\n");

    prtstring("\r\n-----------------------------------------------------------------------------\r\n");

#endif

    // Find the segment that has max phase counts
    for (u8SegPos = 0; u8SegPos <= u8SegNum; u8SegPos++)
    {
        if (u8ArrPHCntsPerSeg[u8SegPos] > u8MaxPHCnts)
        {
            u8MaxPHCnts = u8ArrPHCntsPerSeg[u8SegPos];
            u8SelSegPos = u8SegPos;
        }
    }

    // u8PHPos = (u8MaxPHCnts * 11) / 20;

    u8PHPos = u8MaxPHCnts / 2;

    if ((u8PHPos % 2) > 0)
        u8PHPos++;

    if (u8PHPos)
        u8PHPos--;

    s8RetPhase = (S8)u8ArrPHSegs[u8SelSegPos][u8PHPos];

    if (s8RetPhase >= V_MAX_PHASE)
        return -4;

    return s8RetPhase;
}

void Hal_eMMC_GoodPhases_Dump(int eIP)
{
    U8 u8PassPHPos = 0;

    eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "$$ PassPHs[%d] = (", gu8_PASS_PH_CNT[eIP]);

    for (u8PassPHPos = 0; u8PassPHPos < gu8_PASS_PH_CNT[eIP]; u8PassPHPos++)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "%d, ", gu8Arr_PASS_PHS[eIP][u8PassPHPos]);
    }

    eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, ")\r\n");
}

static int mstar_mci_execute_tuning(struct mmc_host *p_mmc_host, u32 opcode)
{
    int                    eSlot         = 0;
    unsigned char          u8Phase       = 0;
    signed char            s8retPhase    = 0;
    struct mstar_mci_host *pSstarHost_st = mmc_priv(p_mmc_host);
    eMMC_IP_EmType         emmc_ip       = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE;

    // Clean All Pass Phase
    Hal_eMMC_SavePassPhase(0, u8Phase, TRUE);

    if ((u8_cur_timing[emmc_ip] != MMC_TIMING_UHS_DDR50) && (u8_cur_timing[emmc_ip] != MMC_TIMING_MMC_DDR52))
    {
        for (u8Phase = 0; u8Phase < 18; u8Phase++)
        {
            Hal_eMMC_SetPhase(0, EV_SDR, u8Phase);

            if (!mmc_send_tuning(p_mmc_host, opcode, NULL))
            {
                if (Hal_eMMC_SavePassPhase(0, u8Phase, FALSE))
                {
                    return 1;
                }

                // eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,">> [emmc_%d] SDR Tuning ...... Good Phase (%u)\n", eSlot,
                // u8Phase);
            }
        }

        if ((s8retPhase = Hal_eMMC_FindFitPhaseSetting(0, 17)) < 0)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, ">> [emmc_%d] Err: Fit Phase Finding (SDR) ...... Ret(%d)!\n", eSlot,
                       s8retPhase);
            return 1;
        }

        Hal_eMMC_SetPhase(0, EV_SDR, (U8)s8retPhase);

        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, ">> [emmc_%d] SDR PH(%d), ", eSlot, s8retPhase);
        Hal_eMMC_GoodPhases_Dump(0);
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "\n");
    }
    else
    {
        for (u8Phase = 0; u8Phase < 7; u8Phase++)
        {
            Hal_eMMC_SetPhase(0, EV_DDR, u8Phase);

            if (!mmc_send_tuning(p_mmc_host, opcode, NULL))
            {
                if (Hal_eMMC_SavePassPhase(0, u8Phase, FALSE))
                {
                    return 1;
                }

                // eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,">> [emmc_%d] DDR Tuning ...... Good Phase (%u)\n", eSlot,
                // u8Phase);
            }
        }

        if ((s8retPhase = Hal_eMMC_FindFitPhaseSetting(0, 6)) < 0)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, ">> [emmc_%d] Err: Fit Phase Finding (DDR) ...... Ret(%d)!\n", eSlot,
                       s8retPhase);
            return 1;
        }

        Hal_eMMC_SetPhase(0, EV_DDR, (U8)s8retPhase);

        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, ">> [emmc_%d] DDR PH(%d), ", eSlot, s8retPhase);
        Hal_eMMC_GoodPhases_Dump(0);
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "\n");
    }

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;

    return 0;
}

#else
static int mstar_mci_execute_tuning(struct mmc_host *host, u32 opcode)
{
    u32 u32_err = 0;
    struct mstar_mci_host *pSstarHost_st;
    eMMC_IP_EmType emmc_ip;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)

#if 1

    U32 u32_ChkSum;

    pSstarHost_st = mmc_priv(host);
    emmc_ip = ge_emmcIPOrderSlot[pSstarHost_st->slot_num];

#if defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400
    mstar_mci_read_blocks(host, gau8_eMMC_SectorBuf, eMMC_HS400TABLE_BLK_0, 1);
#elif defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200
    mstar_mci_read_blocks(host, gau8_eMMC_SectorBuf, eMMC_HS200TABLE_BLK_0, 1);
#endif

    memcpy((U8 *)&g_eMMCDrv[emmc_ip].TimingTable_t, gau8_eMMC_SectorBuf, sizeof(g_eMMCDrv[emmc_ip].TimingTable_t));

    u32_ChkSum = eMMC_ChkSum((U8 *)&g_eMMCDrv[emmc_ip].TimingTable_t,
                             sizeof(g_eMMCDrv[emmc_ip].TimingTable_t) - eMMC_TIMING_TABLE_CHKSUM_OFFSET);
    if (u32_ChkSum != g_eMMCDrv[emmc_ip].TimingTable_t.u32_ChkSum)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: ChkSum error, no Table \n");
        printk("u32_ChkSum=%08X\n", u32_ChkSum);
        printk("g_eMMCDrv[emmc_ip].TimingTable_t.u32_ChkSum=%08X\n", g_eMMCDrv[emmc_ip].TimingTable_t.u32_ChkSum);
        eMMC_dump_mem((U8 *)&g_eMMCDrv[emmc_ip].TimingTable_t, sizeof(g_eMMCDrv[emmc_ip].TimingTable_t));
        u32_err = eMMC_ST_ERR_DDRT_CHKSUM;
        return u32_err;
    }

    if (0 == u32_ChkSum)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: no Table \n");
        u32_err = eMMC_ST_ERR_DDRT_NONA;
        return u32_err;
    }

#if defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400

    mstar_mci_read_blocks(host, gau8_eMMC_SectorBuf, eMMC_HS400EXTTABLE_BLK_0, 1);

    memcpy((U8 *)&g_eMMCDrv[emmc_ip].TimingTable_G_t, gau8_eMMC_SectorBuf, sizeof(g_eMMCDrv[emmc_ip].TimingTable_G_t));

    u32_ChkSum = eMMC_ChkSum((U8 *)&g_eMMCDrv[emmc_ip].TimingTable_G_t.u32_VerNo,
                             (sizeof(g_eMMCDrv[emmc_ip].TimingTable_G_t) - sizeof(U32)));

    if (u32_ChkSum != g_eMMCDrv[emmc_ip].TimingTable_G_t.u32_ChkSum)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: ChkSum error, no Gen_TTable \n");
        g_eMMCDrv[emmc_ip].TimingTable_G_t.u8_SetCnt = 0;
        g_eMMCDrv[emmc_ip].TimingTable_G_t.u32_ChkSum = 0;
        g_eMMCDrv[emmc_ip].TimingTable_G_t.u32_VerNo = 0;
        u32_err = eMMC_ST_ERR_DDRT_CHKSUM;
        return u32_err;
    }

    if (0 == u32_ChkSum)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: no Gen_TTable \n");
        g_eMMCDrv[emmc_ip].TimingTable_G_t.u8_SetCnt = 0;
        g_eMMCDrv[emmc_ip].TimingTable_G_t.u32_ChkSum = 0;
        g_eMMCDrv[emmc_ip].TimingTable_G_t.u32_VerNo = 0;
        u32_err = eMMC_ST_ERR_DDRT_CHKSUM;
        return u32_err;
    }
#endif

#else

#if defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400
    if ((g_eMMCDrv[emmc_ip].u8_ECSD196_DevType & eMMC_DEVTYPE_HS400_1_8V) && (host->caps2 & MMC_CAP2_HS400_1_8V))
    {
        eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        u32_err = eMMC_LoadTimingTable(FCIE_eMMC_HS400);
        if (eMMC_ST_SUCCESS != u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: no Timing Table, %Xh\n", u32_err);
            return u32_err;
        }

        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        return u32_err;
    }
#endif

#if defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200
    if ((g_eMMCDrv[emmc_ip].u8_ECSD196_DevType & eMMC_DEVTYPE_HS200_1_8V) && (host->caps2 & MMC_CAP2_HS200_1_8V_SDR))
    {
        eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        u32_err = eMMC_LoadTimingTable(FCIE_eMMC_HS200);
        if (eMMC_ST_SUCCESS != u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: no Timing Table, %Xh\n", u32_err);
            return u32_err;
        }

        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        return u32_err;
    }
#endif

#endif

#else
    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    eMMC_pads_switch(emmc_ip, gu8_emmcPADOrderSlot[emmc_ip], FCIE_eMMC_BYPASS);
    eMMC_clock_setting(emmc_ip, FCIE_DEFAULT_CLK);

#if defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400

    if ((g_eMMCDrv[emmc_ip].u8_ECSD196_DevType & eMMC_DEVTYPE_HS400_1_8V) && (host->caps2 & MMC_CAP2_HS400_1_8V_DDR))
    {
        if (eMMC_ST_SUCCESS != eMMC_FCIE_EnableFastMode(emmc_ip, FCIE_eMMC_HS400))
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_FCIE_EnableFastMode HS400 fail\n");
            eMMC_debug(0, 0, "\neMMC: SDR %uMHz \n", g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000);
        }
        else
            eMMC_debug(0, 0, "\neMMC: HS400 %uMHz \n", g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000);
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        return u32_err;
    }

#endif // defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400

#if defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200
    if ((g_eMMCDrv[emmc_ip].u8_ECSD196_DevType & eMMC_DEVTYPE_HS200_1_8V) && (host->caps2 & MMC_CAP2_HS200_1_8V_SDR))
    {
        if (eMMC_ST_SUCCESS != eMMC_FCIE_EnableFastMode(emmc_ip, FCIE_eMMC_HS200))
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_FCIE_EnableFastMode HS200 fail\n");
            eMMC_debug(0, 0, "\neMMC: SDR %uMHz \n", g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000);
        }
        else
            eMMC_debug(0, 0, "\neMMC: HS200 %uMHz \n", g_eMMCDrv[emmc_ip].u32_ClkKHz / 1000);
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        return u32_err;
    }
#endif

    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

#endif

    return u32_err;
}
#endif

//=======================================================================
static void mstar_mci_enable(eMMC_IP_EmType emmc_ip)
{
    u32 u32_err;

    memset((void *)&g_eMMCDrv[emmc_ip], '\0', sizeof(eMMC_DRIVER));

    g_eMMCDrv[emmc_ip].u8_PadType    = FCIE_eMMC_BYPASS;
    g_eMMCDrv[emmc_ip].u16_ClkRegVal = FCIE_SLOWEST_CLK;

#if defined(MSTAR_EMMC_CONFIG_OF)
    g_eMMCDrv[emmc_ip].u16_of_buswidth = gu32_BusWidth[emmc_ip];
#else
    // FIXME: force 8 bit bus width if not DTS configuration.
    g_eMMCDrv[emmc_ip].u16_of_buswidth = 8;
    pr_err(">> [emmc] force 8 bit bus width, no DTS cfg bus:%u\n", g_eMMCDrv[emmc_ip].u16_of_buswidth);
#endif

    // reset retry status to default
    sgu8_IfNeedRestorePadType = 0xFF;
    u8_sdr_retry_count        = 0;

    eMMC_PlatformInit(emmc_ip);

    g_eMMCDrv[emmc_ip].u8_BUS_WIDTH   = BIT_SD_DATA_WIDTH_1;
    g_eMMCDrv[emmc_ip].u16_Reg10_Mode = BIT_SD_DEFAULT_MODE_REG;
    g_eMMCDrv[emmc_ip].u16_RCA        = 1;

    u32_err = eMMC_FCIE_Init(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_FCIE_Init fail: %Xh \n", u32_err);

    eMMC_RST_L(emmc_ip);
    eMMC_hw_timer_sleep(1);
    eMMC_RST_H(emmc_ip);
    eMMC_hw_timer_sleep(1);

    if (eMMC_ST_SUCCESS != eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: WaitD0High TO\n");
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
    }
}

static void mstar_mci_disable(eMMC_IP_EmType emmc_ip)
{
    u32 u32_err;

    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "\n");

    u32_err = eMMC_FCIE_Reset(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC_FCIE_Reset fail: %Xh\n", u32_err);

    eMMC_clock_gating(emmc_ip);
}

#if 0
static ssize_t fcie_pwrsvr_gpio_trigger_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32_pwrsvr_gpio_trigger);
}

static ssize_t fcie_pwrsvr_gpio_trigger_store(struct device *dev, struct device_attribute *attr,
                                              const char *buf, size_t count)
{
    unsigned long u32_pwrsvr_gpio_trigger = 0;

    if(kstrtoul(buf, 0, &u32_pwrsvr_gpio_trigger))
        return -EINVAL;

    if(u32_pwrsvr_gpio_trigger > 1)
        return -EINVAL;

    gu32_pwrsvr_gpio_trigger = u32_pwrsvr_gpio_trigger;

    return count;
}

DEVICE_ATTR(fcie_pwrsvr_gpio_trigger,
            S_IRUSR | S_IWUSR,
            fcie_pwrsvr_gpio_trigger_show,
            fcie_pwrsvr_gpio_trigger_store);

static ssize_t fcie_pwrsvr_gpio_bit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32_pwrsvr_gpio_bit);
}

static ssize_t fcie_pwrsvr_gpio_bit_store(struct device *dev, struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    unsigned long u32_pwrsvr_gpio_bit = 0;

    if(kstrtoul(buf, 0, &u32_pwrsvr_gpio_bit))
        return -EINVAL;

    if(u32_pwrsvr_gpio_bit)
    {
        if(u32_pwrsvr_gpio_bit > 0xF)
            return -EINVAL;
        gu32_pwrsvr_gpio_bit = u32_pwrsvr_gpio_bit;
    }

    return count;
}

DEVICE_ATTR(fcie_pwrsvr_gpio_bit,
            S_IRUSR | S_IWUSR,
            fcie_pwrsvr_gpio_bit_show,
            fcie_pwrsvr_gpio_bit_store);

static ssize_t fcie_pwrsvr_gpio_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%X\n", gu32_pwrsvr_gpio_addr);
}

static ssize_t fcie_pwrsvr_gpio_addr_store(struct device *dev, struct device_attribute *attr,
                                           const char *buf, size_t count)
{
    unsigned long u32_pwrsvr_gpio_addr = 0;

    if(kstrtoul(buf, 0, &u32_pwrsvr_gpio_addr))
        return -EINVAL;

    if(u32_pwrsvr_gpio_addr)
        gu32_pwrsvr_gpio_addr = u32_pwrsvr_gpio_addr;

    return count;
}

DEVICE_ATTR(fcie_pwrsvr_gpio_addr,
            S_IRUSR | S_IWUSR,
            fcie_pwrsvr_gpio_addr_show,
            fcie_pwrsvr_gpio_addr_store);

static ssize_t fcie_pwrsvr_gpio_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32_pwrsvr_gpio_enable);
}

static ssize_t fcie_pwrsvr_gpio_enable_store(struct device *dev, struct device_attribute *attr,
                                             const char *buf, size_t count)
{
    unsigned long u32_pwrsvr_gpio_enable = 0;

#if 0
    if(u8_enable_sar5)
        return count;
#endif

    if(kstrtoul(buf, 0, &u32_pwrsvr_gpio_enable))
        return -EINVAL;

    if(u32_pwrsvr_gpio_enable)
        gu32_pwrsvr_gpio_enable = u32_pwrsvr_gpio_enable;

    return count;
}

DEVICE_ATTR(fcie_pwrsvr_gpio_enable,
            S_IRUSR | S_IWUSR,
            fcie_pwrsvr_gpio_enable_show,
            fcie_pwrsvr_gpio_enable_store);


static ssize_t emmc_sanitize_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32_emmc_sanitize);
}

static ssize_t emmc_sanitize_store(struct device *dev, struct device_attribute *attr,
                                   const char *buf, size_t count)
{
    unsigned long u32_temp = 0;

    if(kstrtoul(buf, 0, &u32_temp))
        return -EINVAL;

    gu32_emmc_sanitize = u32_temp;
    //printk("%Xh\n", gu32_emmc_sanitize);

    if(gu32_emmc_sanitize)
    {
        eMMC_LockFCIE(emmc_ip, (U8*)__FUNCTION__);
        eMMC_debug(eMMC_DEBUG_LEVEL,0,"eMMC: santizing ...\n");
        eMMC_Sanitize(0xAA);
        eMMC_debug(eMMC_DEBUG_LEVEL,0,"eMMC: done\n");
        eMMC_UnlockFCIE(emmc_ip, (U8*)__FUNCTION__);
    }

    return count;
}

DEVICE_ATTR(emmc_sanitize,
            S_IRUSR | S_IWUSR,
            emmc_sanitize_show,
            emmc_sanitize_store);
#endif

static ssize_t eMMC_monitor_count_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    U32            u32_read_bytes, u32_write_bytes;
    U32            u32_speed_read = 0, u32_speed_write = 0;
    U8             u8_i;
    eMMC_IP_EmType emmc_ip;

    eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "Monitor : %d \n", gu32_eMMC_monitor_enable);
    for (u8_i = 0; u8_i < gu8_emmcSlotNums; u8_i++)
    {
        emmc_ip         = ge_emmcIPOrderSlot[u8_i];
        u32_speed_read  = 0;
        u32_speed_write = 0;
        u32_read_bytes  = ((gu32_eMMC_read_cnt[emmc_ip] << eMMC_SECTOR_512BYTE_BITS) * 10) / 0x100000;
        if (gu64_jiffies_read[emmc_ip])
            u32_speed_read = (u32_read_bytes * HZ) / ((U32)gu64_jiffies_read[emmc_ip]);

        u32_write_bytes = ((gu32_eMMC_write_cnt[emmc_ip] << eMMC_SECTOR_512BYTE_BITS) * 10) / 0x100000;
        if (gu64_jiffies_write[emmc_ip])
        {
            u32_speed_write = (u32_write_bytes * HZ) / ((U32)gu64_jiffies_write[emmc_ip]);
        }

        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,
                   "eMMC_%d: read total count:%xh, %u.%uMBytes/sec\n"
                   "eMMC_%d: write total count:%xh, %u.%uMByte/sec\n",
                   emmc_ip, gu32_eMMC_read_cnt[emmc_ip], u32_speed_read / 10,
                   u32_speed_read - 10 * (u32_speed_read / 10), emmc_ip, gu32_eMMC_write_cnt[emmc_ip],
                   u32_speed_write / 10, u32_speed_write - 10 * (u32_speed_write / 10));
    }

    return 0;
}

static ssize_t eMMC_monitor_count_enable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                               size_t count)
{
    unsigned long  u32_temp = 0;
    U8             u8_i;
    eMMC_IP_EmType emmc_ip;

    if (kstrtoul(buf, 0, &u32_temp))
        return -EINVAL;

    gu32_eMMC_monitor_enable = u32_temp;

    if (gu32_eMMC_monitor_enable)
    {
        for (u8_i = 0; u8_i < gu8_emmcSlotNums; u8_i++)
        {
            emmc_ip                      = ge_emmcIPOrderSlot[u8_i];
            gu32_eMMC_read_cnt[emmc_ip]  = 0;
            gu32_eMMC_write_cnt[emmc_ip] = 0;
            gu64_jiffies_write[emmc_ip]  = 0;
            gu64_jiffies_read[emmc_ip]   = 0;
        }
    }

    return count;
}

DEVICE_ATTR(eMMC_monitor_count_enable, S_IRUSR | S_IWUSR, eMMC_monitor_count_enable_show,
            eMMC_monitor_count_enable_store);

static ssize_t emmc_write_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32_eMMC_write_log_enable);
}

static ssize_t eMMC_write_log_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long u32_temp = 0;

    if (kstrtoul(buf, 0, &u32_temp))
        return -EINVAL;

    gu32_eMMC_write_log_enable = u32_temp;

    if (gu32_eMMC_write_log_enable)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "eMMC: write log enable\n");
    }

    return count;
}

DEVICE_ATTR(eMMC_write_log_enable, S_IRUSR | S_IWUSR, emmc_write_log_show, eMMC_write_log_store);

static ssize_t emmc_read_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32_eMMC_read_log_enable);
}

static ssize_t eMMC_read_log_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long u32_temp = 0;

    if (kstrtoul(buf, 0, &u32_temp))
        return -EINVAL;

    gu32_eMMC_read_log_enable = u32_temp;

    if (gu32_eMMC_read_log_enable)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "eMMC: read log enable\n");
    }

    return count;
}

DEVICE_ATTR(eMMC_read_log_enable, S_IRUSR | S_IWUSR, emmc_read_log_show, eMMC_read_log_store);

static ssize_t emmc_bootbus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    U32            u32_err = eMMC_ST_SUCCESS;
    U8             u8_slot;
    eMMC_IP_EmType emmc_ip;

    for (u8_slot = 0; u8_slot < gu8_emmcSlotNums; u8_slot++)
    {
        emmc_ip = ge_emmcIPOrderSlot[u8_slot];
        eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

        u32_err = eMMC_GetExtCSD(emmc_ip, gau8_eMMC_SectorBuf);

        g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        if (u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d read boot_bus error:0x%08x ! \n", emmc_ip, u32_err);
            continue;
        }

        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "eMMC_%d boot_bus : %d!\n", emmc_ip,
                   (gau8_eMMC_SectorBuf[EXT_CSD_BOOT_BUS_WIDTH] & 0x3));
    }

    return 0;
}

static ssize_t eMMC_bootbus_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    U32            u32_err  = eMMC_ST_SUCCESS;
    U32            u32_temp = 0;
    U32            u32_slotNum, u32_bootbus;
    eMMC_IP_EmType emmc_ip;

    if (buf == NULL)
        return -EINVAL;

    u32_temp = sscanf(buf, "%d %d", &u32_slotNum, &u32_bootbus);
    if (u32_temp != 2)
    {
        if (gu8_emmcSlotNums == 1)
        {
            u32_slotNum = 0;
            u32_temp    = sscanf(buf, "%d", &u32_bootbus);
        }
        else
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,
                       "echo 'emmc_ip' 'bootbus' > eMMC_bootbus \n"
                       "[bootbus] [0]-> 1 bit mode, [1]-> 4 bit mode, [2]-> 8 bit mode\n");
            return -EINVAL;
        }
    }

    if (u32_slotNum >= gu8_emmcSlotNums || u32_slotNum < 0)
    {
        return -EINVAL;
    }

    emmc_ip = ge_emmcIPOrderSlot[u32_slotNum];

    if (u32_bootbus == 0x0)
    {
        printk("\r\n>> eMMC_%d Select 1x bus width <<\r\n", emmc_ip);
    }
    else if (u32_bootbus == 0x1)
    {
        printk("\r\n>> eMMC_%d Select 4x bus width <<\r\n", emmc_ip);
    }
    else if (u32_bootbus == 0x2)
    {
        printk("\r\n>> eMMC_%d Select 8x bus width <<\r\n", emmc_ip);
    }
    else
    {
        printk("\r\n>> eMMC_%d Invalid parameter <<\r\n", emmc_ip);
        u32_err = eMMC_ST_ERR_PARAMETER;
        return u32_err;
    }

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry
    u32_err = eMMC_SetExtCSD(emmc_ip, MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BOOT_BUS_WIDTH,
                             EXT_CSD_BOOT_BUS_WIDTH_MODE(0) | EXT_CSD_BOOT_BUS_WIDTH_RESET(0)
                                 | EXT_CSD_BOOT_BUS_WIDTH_WIDTH(u32_bootbus));
    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;
    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    if (u32_err)
    {
        printk("eMMC_%d error(%X): set boot bus width fail ! \n", emmc_ip, u32_err);
        return u32_err;
    }

    return count;
}

DEVICE_ATTR(eMMC_bootbus, S_IRUSR | S_IWUSR, emmc_bootbus_show, eMMC_bootbus_store);

static ssize_t emmc_partconf_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    U32            u32_err = eMMC_ST_SUCCESS;
    U8             u8_slot;
    eMMC_IP_EmType emmc_ip;

    for (u8_slot = 0; u8_slot < gu8_emmcSlotNums; u8_slot++)
    {
        emmc_ip = ge_emmcIPOrderSlot[u8_slot];
        eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

        u32_err = eMMC_GetExtCSD(emmc_ip, gau8_eMMC_SectorBuf);

        g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        if (u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d read boot part config error:0x%08x ! \n", emmc_ip, u32_err);
            continue;
        }
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0, "eMMC_%d partconf : %d!\n", emmc_ip,
                   (gau8_eMMC_SectorBuf[EXT_CSD_PART_CONF] >> 3) & 7);
    }

    return 0;
}

static ssize_t eMMC_partconf_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    U32            u32_temp = 0;
    U32            u32_err  = eMMC_ST_SUCCESS;
    U32            u32_slotNum, u32_partconf;
    eMMC_IP_EmType emmc_ip;

    if (buf == NULL)
        return -EINVAL;

    u32_temp = sscanf(buf, "%d %d", &u32_slotNum, &u32_partconf);
    if (u32_temp != 2)
    {
        if (gu8_emmcSlotNums == 1)
        {
            u32_slotNum = 0;
            u32_temp    = sscanf(buf, "%d", &u32_partconf);
        }
        else
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,
                       "echo 'emmc_ip' 'partconf' > eMMC_partconf \n"
                       "[partconf] [0]-> don't support boot, [1]-> boot0 partition, [2]-> boot1 partiton, [7]-> UDA "
                       "partiton\n");
            return -EINVAL;
        }
    }

    if (u32_slotNum >= gu8_emmcSlotNums || u32_slotNum < 0)
    {
        return -EINVAL;
    }

    emmc_ip = ge_emmcIPOrderSlot[u32_slotNum];

    if (u32_partconf == 0x0)
    {
        printk("\r\n>> eMMC_%d Do not support boot <<\r\n", emmc_ip);
    }
    else if (u32_partconf == 0x1)
    {
        printk("\r\n>> eMMC_%d Select boot partition 1 <<\r\n", emmc_ip);
    }
    else if (u32_partconf == 0x2)
    {
        printk("\r\n>> eMMC_%d Select boot partition 2 <<\r\n", emmc_ip);
    }
    else if (u32_partconf == 0x7)
    {
        printk("\r\n>> eMMC_%d Select UDA partition <<\r\n", emmc_ip);
    }
    else
    {
        printk("\r\n>> eMMC_%d Invalid parameter <<\r\n", emmc_ip);
        u32_err = eMMC_ST_ERR_PARAMETER;
        return u32_err;
    }

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry
    u32_err = eMMC_SetExtCSD(emmc_ip, MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_PART_CONF,
                             EXT_CSD_BOOT_ACK(1) | EXT_CSD_BOOT_PART_NUM(u32_partconf) | EXT_CSD_PARTITION_ACCESS(0));
    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;
    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    if (u32_err)
    {
        printk("eMMC_%d error(%X): set boot part config fail ! \n", emmc_ip, u32_err);
        return u32_err;
    }

    return count;
}

DEVICE_ATTR(eMMC_partconf, S_IRUSR | S_IWUSR, emmc_partconf_show, eMMC_partconf_store);

static ssize_t emmc_get_clk_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    U8             u8_i;
    eMMC_IP_EmType emmc_ip;
    for (u8_i = 0; u8_i < gu8_emmcSlotNums; u8_i++)
    {
        emmc_ip = ge_emmcIPOrderSlot[u8_i];
        printk(KERN_CONT "eMMC%d: %dKHz\n", emmc_ip, g_eMMCDrv[emmc_ip].u32_ClkKHz);
    }

    return 0;
}

DEVICE_ATTR(eMMC_get_clock, S_IRUSR, emmc_get_clk_show, NULL);

static ssize_t eMMC_driving_control_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                          size_t count)
{
    U32            u32_temp, u32_drvlevel, u32_slotNum;
    char           signalline[10];
    eMMC_IP_EmType emmc_ip;
    U32            u32_emmcPad;
    EMMCPinDrv_T   st_EmmcPindrv = {-1, -1, -1};

    u32_temp = sscanf(buf, "%d %s %d", &u32_slotNum, signalline, &u32_drvlevel);
    if (u32_temp != 3)
    {
        if ((u32_temp == 2) || (u32_temp == 0))
        {
            if (gu8_emmcSlotNums == 1)
            {
                u32_slotNum = 0;
                u32_temp    = sscanf(buf, "%s %d", signalline, &u32_drvlevel);
            }
            else
            {
                strcpy(signalline, "all");
                u32_temp = sscanf(buf, "%d %d", &u32_slotNum, &u32_drvlevel);
            }

            if (u32_temp != 2)
                goto EPMT;
        }
        else if (u32_temp == 1)
        {
            if (gu8_emmcSlotNums == 1)
            {
                u32_slotNum = 0;
                strcpy(signalline, "all");
                u32_temp = sscanf(buf, "%d", &u32_drvlevel);
                if (u32_temp != 1)
                    goto EPMT;
            }
            else
                goto EPMT;
        }
        else
            goto EPMT;
    }

    emmc_ip     = ge_emmcIPOrderSlot[u32_slotNum];
    u32_emmcPad = gu8_emmcPADOrderSlot[emmc_ip];

    if ((strcmp(signalline, "clk") == 0) || (strcmp(signalline, "CLK") == 0))
    {
        st_EmmcPindrv.eDrvClk = u32_drvlevel;
    }
    else if ((strcmp(signalline, "cmd") == 0) || (strcmp(signalline, "CMD") == 0))
    {
        st_EmmcPindrv.eDrvCmd = u32_drvlevel;
    }
    else if ((strcmp(signalline, "data") == 0) || (strcmp(signalline, "DATA") == 0))
        st_EmmcPindrv.eDrvData = u32_drvlevel;
    else if (strcmp(signalline, "all") == 0)
    {
        st_EmmcPindrv.eDrvClk  = u32_drvlevel;
        st_EmmcPindrv.eDrvCmd  = u32_drvlevel;
        st_EmmcPindrv.eDrvData = u32_drvlevel;
    }

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    eMMC_driving_control(emmc_ip, u32_emmcPad, st_EmmcPindrv);

    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    return count;

EPMT:
    pr_err("\n%s usage:\n", __FUNCTION__);
    pr_err(
        "echo [slotIndex] <signalLine> [drvLevel] > eMMC_driving_control    set <signalLine> driving control level is "
        "[drvLevel] for slot sd[slotIndex]\n");
    pr_err(
        "echo [slotIndex]  [drvLevel] > eMMC_driving_control                set all signal line's driving control "
        "level is [drvLevel] for slot sd[slotIndex]\n");
    pr_err(
        "echo <signalLine> [drvLevel] > eMMC_driving_control                set <signalLine> driving control level is "
        "[drvLevel] when slotnum = 1.\n");
    pr_err(
        "echo [drvLevel] > eMMC_driving_control                             set all signal line's driving control "
        "level is [drvLevel] when slotnum = 1.\n");
    pr_err("    operation [slotIndex]   is slot number:[0, ). \n");
    pr_err("    operation <singalLine>  is \"clk\" \"cmd\" \"data\" \"all\" \n");
    pr_err("    operation [drvLevel]    is number:[0, ). \n");

    return -EINVAL;
}

DEVICE_ATTR(eMMC_driving_control, S_IWUSR, NULL, eMMC_driving_control_store);

static ssize_t eMMC_write_protect_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                        size_t count)
{
    U32            u32_err, u32_temp, u32_address_wp, u32_slotNum, u32_option, u32_size = 1;
    eMMC_IP_EmType emmc_ip;

    u32_temp = sscanf(buf, "%d %d %x %x", &u32_slotNum, &u32_option, &u32_address_wp, &u32_size);
    if ((u32_temp != 3) && (u32_temp != 4))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,
                   "echo [emmc_slot] [otption] [address] <size> > eMMC_write_protect \n"
                   "  [otption]\n"
                   "    [0]     -Set the eMMC address of the group to be 'write protect'\n"
                   "    [1]     -Clear the eMMC address of the group remove 'write protect'\n"
                   "    [2]     -Ask the eMMC address of the group whether it's in 'write protect'?\n"
                   "    [3]     -ASK the eMMC address of the group about the 'write protect' type\n"
                   "[address] and <size> is in bytes or block, it's depend on eMMC's HC\n");
        return -EINVAL;
    }
    emmc_ip = ge_emmcIPOrderSlot[u32_slotNum];

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    u32_err = eMMC_Init(emmc_ip);
    if (u32_err)
        return -EINVAL;

    u32_err = eMMC_USER_WriteProtect_Option(emmc_ip, u32_address_wp, u32_size, u32_option);
    if (u32_err < 0)
        return count;

    if (u32_option == 2)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,
                   "eMMC_%d, USER part address(0x%08x)'s write protection is %d(0:invalid, 1:valid)\n", emmc_ip,
                   u32_address_wp, u32_err);
    else if (u32_option == 3)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 0,
                   "eMMC_%d, USER part address(0x%08x)'s write protection type is %d(0:not protected, 1:temporary, "
                   "2:power-on, 3:permanent)\n",
                   emmc_ip, u32_address_wp, u32_err);

    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    return count;
}

DEVICE_ATTR(eMMC_write_protect, S_IWUSR, NULL, eMMC_write_protect_store);

#if defined(CONFIG_MS_EMMC_UT_VERIFY) && CONFIG_MS_EMMC_UT_VERIFY
static ssize_t emmc_hwreset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    U32            u32_err = eMMC_ST_SUCCESS;
    U8             u8_slot;
    eMMC_IP_EmType emmc_ip;

    for (u8_slot = 0; u8_slot < gu8_emmcSlotNums; u8_slot++)
    {
        emmc_ip = ge_emmcIPOrderSlot[u8_slot];
        eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

        u32_err = eMMC_GetExtCSD(emmc_ip, gau8_eMMC_SectorBuf);

        g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        if (u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d read RST_n enable error:0x%08x ! \n", emmc_ip, u32_err);
            continue;
        }

        eMMC_debug(0, 0, "\033[7;33meMMC_%d RST_n enable : %d!\033[m\r\n", emmc_ip,
                   (gau8_eMMC_SectorBuf[162] & 0x3)); // 162:RST_n signal

        if ((gau8_eMMC_SectorBuf[162] & 0x3) == 2)
            eMMC_debug(0, 0, "\033[7;31mWARNING: RST_n signal is permanently disabled!!!\033[m\r\n");
    }

    return 0;
}

static ssize_t eMMC_hwreset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    U32            u32_slotNum, u32_temp;
    U32            u32_err = eMMC_ST_SUCCESS;
    eMMC_IP_EmType emmc_ip;

    if (buf == NULL)
        return -EINVAL;

    u32_temp = sscanf(buf, "%d", &u32_slotNum);
    if (u32_temp != 1 || u32_slotNum >= gu8_emmcSlotNums || u32_slotNum < 0)
    {
        return -EINVAL;
    }

    emmc_ip = ge_emmcIPOrderSlot[u32_slotNum];

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

    u32_err = eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 162, BIT0);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;
    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    if (u32_err != eMMC_ST_SUCCESS)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: %Xh, eMMC, set Ext_CSD[162]: %Xh fail\n", u32_err, BIT0);
    else
        eMMC_debug(0, 0, "\033[7;32mset Ext_CSD[162]: %Xh, RST_n signal is permanently enabled!!!\033[m\r\n", BIT0);

    return count;
}

DEVICE_ATTR(eMMC_hwreset, S_IRUSR | S_IWUSR, emmc_hwreset_show, eMMC_hwreset_store);

static ssize_t emmc_run_ipverify_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    U32            u32_slotNum, u32_temp;
    eMMC_IP_EmType emmc_ip;

    if (buf == NULL)
        return -EINVAL;

    u32_temp = sscanf(buf, "%d", &u32_slotNum);
    if (u32_temp != 1 || u32_slotNum >= gu8_emmcSlotNums || u32_slotNum < 0)
    {
        return -EINVAL;
    }

    emmc_ip = ge_emmcIPOrderSlot[u32_slotNum];

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    eMMC_IPVerify_Main(dev, emmc_ip);
    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    return count;
}

DEVICE_ATTR(eMMC_run_ipverify, S_IWUSR, NULL, emmc_run_ipverify_store);
#endif

static struct attribute *mstar_mci_attr[] = {
#if 0
    &dev_attr_fcie_pwrsvr_gpio_enable.attr,
    &dev_attr_fcie_pwrsvr_gpio_addr.attr,
    &dev_attr_fcie_pwrsvr_gpio_bit.attr,
    &dev_attr_fcie_pwrsvr_gpio_trigger.attr,
    &dev_attr_emmc_sanitize.attr,
#endif
    &dev_attr_eMMC_read_log_enable.attr,
    &dev_attr_eMMC_write_log_enable.attr,
    &dev_attr_eMMC_monitor_count_enable.attr,
    &dev_attr_eMMC_partconf.attr,
    &dev_attr_eMMC_get_clock.attr,
    &dev_attr_eMMC_bootbus.attr,
    &dev_attr_eMMC_driving_control.attr,
    &dev_attr_eMMC_write_protect.attr,
#if defined(CONFIG_MS_EMMC_UT_VERIFY) && CONFIG_MS_EMMC_UT_VERIFY
    &dev_attr_eMMC_hwreset.attr,
    &dev_attr_eMMC_run_ipverify.attr,
#endif
    NULL,
};

static struct attribute_group mstar_mci_attr_grp = {
    .attrs = mstar_mci_attr,
};

static const struct mmc_host_ops sg_mstar_mci_ops = {
#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
    .pre_req  = mstar_mci_pre_req,
    .post_req = mstar_mci_post_req,
#endif
    .request        = mstar_mci_request,
    .set_ios        = mstar_mci_set_ios,
    .execute_tuning = mstar_mci_execute_tuning,
};

#if defined(MSTAR_EMMC_CONFIG_OF)
struct platform_device sg_mstar_emmc_device_st;
#else
struct platform_device sg_mstar_emmc_device_st = {
    .name = DRIVER_NAME,
    .id = 0,
    .resource = NULL,
    .num_resources = 0,
    .dev.dma_mask = &sg_mstar_emmc_device_st.dev.coherent_dma_mask,
    .dev.coherent_dma_mask = DMA_BIT_MASK(64),
};
#endif

static U32 mstar_mci_dts_init(struct platform_device *pDev_st)
{
    U32 slotnum, i;
    U32 u32_admamode[EMMC_NUM_TOTAL] = {1};
    U8  u8_admaflag                  = 0;
    U32 ipsel[EMMC_NUM_TOTAL];
    U32 padsel[EMMC_NUM_TOTAL] = {0};
    U32 buswidth[EMMC_NUM_TOTAL];
    U32 maxclk[EMMC_NUM_TOTAL]           = {48000000, 48000000};
    U32 u32_clk_driving[EMMC_NUM_TOTAL]  = {0};
    U32 u32_cmd_driving[EMMC_NUM_TOTAL]  = {0};
    U32 u32_data_driving[EMMC_NUM_TOTAL] = {0};
#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
    U32 u32_SupportEmmc50[EMMC_NUM_TOTAL] = {0};
#endif

    if (of_property_read_u32(pDev_st->dev.of_node, "slot-num", &slotnum))
    {
        pr_err(">> [emmc] Err: Could not get dts [slot-num] option!\n");
        return -ENODEV;
    }

    if (of_property_read_u32_array(pDev_st->dev.of_node, "adma-mode", u32_admamode, slotnum))
    {
        pr_err(">> [emmc] Warn: Could not get dts [adma-mode] option, default using adma mode!\n");
        u8_admaflag = 1;
    }

    if (of_property_read_u32_array(pDev_st->dev.of_node, "ip-select", ipsel, slotnum))
    {
        pr_err(">> [emmc] Err: Could not get dts [ip-select] option!\n");
        return -ENODEV;
    }

    if (of_property_read_u32_array(pDev_st->dev.of_node, "pad-select", padsel, slotnum))
    {
        pr_err(">> [emmc] Warn: Could not get dts [pad-select] option, default 0!\n");
    }
#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
    if (of_property_read_u32_array(pDev_st->dev.of_node, "support-emmc50", u32_SupportEmmc50, slotnum))
    {
        pr_err(">> [emmc] Warn: Could not get dts [support-emmc50] option, default 0!\n");
    }
#endif
    if (of_property_read_u32_array(pDev_st->dev.of_node, "bus-width", buswidth, slotnum))
    {
        pr_err(">> [emmc] Err: Could not get dts [bus-width] option!\n");
        return -ENODEV;
    }

    if (of_property_read_u32_array(pDev_st->dev.of_node, "max-clks", maxclk, slotnum)) // optional
    {
        pr_err(">> [emmc] warn: Could not get dts [max-clks] option, using default setting!\n");
    }

    if (of_property_read_u32(pDev_st->dev.of_node, "clk-driving", u32_clk_driving)) // optional
    {
        pr_err(">> [emmc] Warn: Could not get dts [clk-driving] option!\n");
    }

    if (of_property_read_u32(pDev_st->dev.of_node, "cmd-driving", u32_cmd_driving)) // optional
    {
        pr_err(">> [emmc] Warn: Could not get dts [cmd-driving] option!\n");
    }

    if (of_property_read_u32(pDev_st->dev.of_node, "data-driving", u32_data_driving)) // optional
    {
        pr_err(">> [emmc] Warn: Could not get dts [data-driving] option!\n");
    }

    gu8_emmcSlotNums = (U8)slotnum;
    for (i = 0; i < slotnum; i++)
    {
        ge_emmcIPOrderSlot[i] = (U8)ipsel[i];
        if (u8_admaflag)
            gu32AdmaMode[ge_emmcIPOrderSlot[i]] = 1;
        else
            gu32AdmaMode[ge_emmcIPOrderSlot[i]] = (U32)u32_admamode[i];
        gu8_emmcPADOrderSlot[ge_emmcIPOrderSlot[i]]        = (U8)padsel[i];
        gu32_BusWidth[ge_emmcIPOrderSlot[i]]               = (U32)buswidth[i];
        gu32_max_clk[ge_emmcIPOrderSlot[i]]                = (U32)maxclk[i];
        gst_emmcPinDriving[ge_emmcIPOrderSlot[i]].eDrvClk  = (U32)u32_clk_driving[i];
        gst_emmcPinDriving[ge_emmcIPOrderSlot[i]].eDrvCmd  = (U32)u32_cmd_driving[i];
        gst_emmcPinDriving[ge_emmcIPOrderSlot[i]].eDrvData = (U32)u32_data_driving[i];
#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
        gu32_SupportEmmc50[ge_emmcIPOrderSlot[i]] = (U32)u32_SupportEmmc50[i];
#endif
    }

#ifdef CONFIG_CAM_CLK
    //
#else
    clkdata = kzalloc(sizeof(struct clk_data), GFP_KERNEL);
    if (!clkdata)
    {
        printk(KERN_CRIT "Unable to allocate nand clock data\n");
        return -ENOMEM;
    }

    for (i = 0; i < slotnum; i++)
    {
        clkdata->clk_fcie[ge_emmcIPOrderSlot[i]] =
            of_clk_get_by_name(pDev_st->dev.of_node, gu8_fcie_clk_name[ge_emmcIPOrderSlot[i]]);
        if (IS_ERR(clkdata->clk_fcie[ge_emmcIPOrderSlot[i]]))
        {
            printk(KERN_CRIT "Unable to get fcie clk from dts\n");
            return -ENODEV;
        }
#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
        clkdata->clk_syn[ge_emmcIPOrderSlot[i]] =
            of_clk_get_by_name(pDev_st->dev.of_node, gu8_syn_clk_name[ge_emmcIPOrderSlot[i]]);
        if (IS_ERR(clkdata->clk_syn[ge_emmcIPOrderSlot[i]]))
        {
            printk(KERN_CRIT "Unable to get fcie syn_clk from dts\n");
            return -ENODEV;
        }

        clk_prepare_enable(clkdata->clk_syn[ge_emmcIPOrderSlot[i]]);
#endif
    }

#endif

    return 0;
}

static U32 mstar_mci_init_slot(U8 slotNo, struct mstar_mci_driver *p_emmc_driver)
{
    struct mstar_mci_host *pSstarHost_st = 0;
    s32                    s32_ret       = 0;
    struct mmc_host *      pMMCHost_st;
    eMMC_IP_EmType         emmc_ip = ge_emmcIPOrderSlot[slotNo];

    pMMCHost_st = mmc_alloc_host(sizeof(struct mstar_mci_host), &p_emmc_driver->pdev->dev);
    if (!pMMCHost_st)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d Err: mmc_alloc_host fail \n", slotNo);
        return -ENOMEM;
    }

    pSstarHost_st = mmc_priv(pMMCHost_st);

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
    if (dma_set_mask_and_coherent(&p_emmc_driver->pdev->dev, DMA_BIT_MASK(64)))
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "no suitable DMA available ！\n");
#endif

    if (gu32AdmaMode[emmc_ip])
    {
        pSstarHost_st->adma_buffer =
            dma_alloc_coherent(&p_emmc_driver->pdev->dev, sizeof(struct _AdmaDescriptor) * FCIE_ADMA_DESC_COUNT,
                               &(pSstarHost_st->adma_phy_addr), GFP_KERNEL);
        if (!pSstarHost_st->adma_buffer)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC_%d Err: Failed to Allocate mstar_mci_host ADMA buffer\n",
                       slotNo);
            goto LABEL_END1;
        }
    }

    pMMCHost_st->ops = &sg_mstar_mci_ops;

    // [FIXME]->
    pMMCHost_st->f_min     = CLK_400KHz;
    pMMCHost_st->f_max     = gu32_max_clk[emmc_ip] ? gu32_max_clk[emmc_ip] : CLK_200MHz;
    pMMCHost_st->ocr_avail = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
                             | MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

    pMMCHost_st->max_blk_count = BIT_SD_JOB_BLK_CNT_MASK;
    pMMCHost_st->max_blk_size  = 512;
    pMMCHost_st->max_req_size  = pMMCHost_st->max_blk_count * pMMCHost_st->max_blk_size; // could be optimized

    pMMCHost_st->max_seg_size = pMMCHost_st->max_req_size;
    pMMCHost_st->max_segs     = 32;

    if (gu32_BusWidth[emmc_ip] == 8)
        pMMCHost_st->caps = MMC_CAP_8_BIT_DATA;
    else if (gu32_BusWidth[emmc_ip] == 4)
        pMMCHost_st->caps = MMC_CAP_4_BIT_DATA;

    //---------------------------------------
    pMMCHost_st->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_NONREMOVABLE;

#if defined(ENABLE_EMMC_PRE_DEFINED_BLK) && ENABLE_EMMC_PRE_DEFINED_BLK
    pMMCHost_st->caps |= MMC_CAP_CMD23;
#endif

#if (defined(ENABLE_eMMC_ATOP) && ENABLE_eMMC_ATOP)

#if defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400
    if (gu32_SupportEmmc50[emmc_ip])
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
        pMMCHost_st->caps2 |= MMC_CAP2_HS400_1_8V;
#else
        pMMCHost_st->caps2 |= MMC_CAP2_HS400_1_8V_DDR;
#endif
    }
#endif

#if defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200
    if (gu32_SupportEmmc50[emmc_ip])
    {
        pMMCHost_st->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }
#endif

    if (gu32_SupportEmmc50[emmc_ip])
    {
        pMMCHost_st->caps |= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50;
    }
#endif

#ifdef CONFIG_MP_EMMC_TRIM
    pMMCHost_st->caps |= MMC_CAP_ERASE;
#endif

#ifdef CONFIG_MP_EMMC_CACHE
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
    pMMCHost_st->caps2 |= MMC_CAP2_CACHE_CTRL;
#endif
#endif

    //------------------------------------------------------------
    pSstarHost_st->mmc      = pMMCHost_st;
    pSstarHost_st->slot_num = slotNo;
    pSstarHost_st->slot_ip  = emmc_ip;
    strcpy(pSstarHost_st->name, MSTAR_MCI_NAME);

    p_emmc_driver->emmc_host[slotNo] = pSstarHost_st;

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    mstar_mci_enable(emmc_ip);
    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

    //    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,1) && LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
    //    pMMCHost_st->caps2          |= MMC_CAP2_NO_SLEEP_CMD;
    //    #endif

    // <-[FIXME]
#if defined(ENABLE_EMMC_ASYNC_IO) && ENABLE_EMMC_ASYNC_IO
    pSstarHost_st->next_data.cookie = 1;
    INIT_WORK(&pSstarHost_st->async_work, mstar_mci_send_data);
#endif

    mmc_add_host(pMMCHost_st);

#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
    fcie_irq[emmc_ip] = platform_get_irq(p_emmc_driver->pdev, emmc_ip);
    if (fcie_irq[emmc_ip] < 0)
        return -ENXIO;

    s32_ret = request_irq(fcie_irq[emmc_ip], eMMC_FCIE_IRQ, IRQF_TRIGGER_NONE, DRIVER_NAME, pSstarHost_st);
    if (s32_ret)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: request_irq fail \n");
        mmc_free_host(pMMCHost_st);
        goto LABEL_END2;
    }
#endif

#if defined(CONFIG_MMC_MSTAR_MMC_EMMC_LIFETEST)
    g_eMMCDrv[emmc_ip].u64_CNT_TotalRBlk = 0;
    g_eMMCDrv[emmc_ip].u64_CNT_TotalWBlk = 0;
    writefile                            = create_proc_entry(procfs_name, 0644, NULL);
    if (writefile == NULL)
        eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC Err: Can not initialize /proc/%s\n", procfs_name);
    else
    {
        writefile->read_proc = procfile_read;
        writefile->mode      = S_IFREG | S_IRUGO;
        writefile->uid       = 0;
        writefile->gid       = 0;
        writefile->size      = 0x10;
    }
#endif

    return 0;

#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
LABEL_END2:
#endif
    mmc_remove_host(pMMCHost_st);

LABEL_END1:
    if (pSstarHost_st->adma_buffer)
        dma_free_coherent(NULL, sizeof(struct _AdmaDescriptor) * FCIE_ADMA_DESC_COUNT, pSstarHost_st->adma_buffer,
                          pSstarHost_st->adma_phy_addr);

    mmc_free_host(pMMCHost_st);

    return s32_ret;
}

static s32 mstar_mci_probe(struct platform_device *pDev_st)
{
    struct mstar_mci_driver *p_emmc_driver;
    U8                       slotNo;
    U32                      u32_err;

    // --------------------------------
    if (!pDev_st)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: pDev_st is NULL \n");
        return -EINVAL;
    }

#if defined(MSTAR_EMMC_CONFIG_OF)
    memcpy(&sg_mstar_emmc_device_st, pDev_st, sizeof(struct platform_device));
#endif

    p_emmc_driver = kzalloc(sizeof(struct mstar_mci_driver), GFP_KERNEL);

    if (!p_emmc_driver)
    {
        eMMC_debug(0, 0, "eMMC Err: Failed to Allocate p_emmc_driver!\n\n");
        return -ENOMEM;
    }

    p_emmc_driver->pdev = pDev_st;

    if (mstar_mci_dts_init(pDev_st))
    {
        eMMC_debug(0, 1, "Err: Failed to use dts function ! \n");
        return 1;
    }

    platform_set_drvdata(pDev_st, p_emmc_driver);

    for (slotNo = 0; slotNo < gu8_emmcSlotNums; slotNo++)
    {
        if (mstar_mci_init_slot(slotNo, p_emmc_driver) != 0)
        {
            eMMC_debug(0, 1, "Err: Failed to init slot!\n");
            kfree(p_emmc_driver);
            return 1;
        }
        eMMC_debug(0, 1, "[eMMC_%d]  Probe Platform Devices\n", slotNo);
    }

    // For getting and showing device attributes from/to user space.
    u32_err = sysfs_create_group(&pDev_st->dev.kobj, &mstar_mci_attr_grp);

    sgp_eMMCThread_st = kthread_create(mstar_mci_Housekeep, NULL, "eMMC_bg_thread");
    if (IS_ERR(sgp_eMMCThread_st))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: create thread fail \n");
        return PTR_ERR(sgp_eMMCThread_st);
    }
    wake_up_process(sgp_eMMCThread_st);

    return 0;
}

static U32 mstar_mci_remove_slot(U8 slotNo, struct mstar_mci_driver *p_emmc_driver)
{
    struct mstar_mci_host *pSstar_mci_host_st = p_emmc_driver->emmc_host[slotNo];
    struct mmc_host *      p_emmc_host        = pSstar_mci_host_st->mmc;

    eMMC_IP_EmType emmc_ip = ge_emmcIPOrderSlot[pSstar_mci_host_st->slot_num];

    if (!pSstar_mci_host_st)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: pMMCHost_st is NULL\n");
        return -1;
    }

    if (pSstar_mci_host_st->adma_buffer)
        dma_free_coherent(NULL, sizeof(struct _AdmaDescriptor) * FCIE_ADMA_DESC_COUNT, pSstar_mci_host_st->adma_buffer,
                          pSstar_mci_host_st->adma_phy_addr);

    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC, remove +\n");

    mmc_remove_host(p_emmc_host);

    eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
    mstar_mci_disable(emmc_ip);
    eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

#if defined(ENABLE_eMMC_INTERRUPT_MODE) && ENABLE_eMMC_INTERRUPT_MODE
    free_irq(fcie_irq[emmc_ip], pSstar_mci_host_st);
#endif

    mmc_free_host(p_emmc_host);

    eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC, remove -\n");

    return 0;
}

static s32 __exit mstar_mci_remove(struct platform_device *pDev_st)
{
    /* Define Local Variables */
    struct mstar_mci_driver *p_emmc_driver = platform_get_drvdata(pDev_st);
    U8                       slotNo        = 0;

    platform_set_drvdata(pDev_st, NULL);

    for (slotNo = 0; slotNo < gu8_emmcSlotNums; slotNo++)
    {
        mstar_mci_remove_slot(slotNo, p_emmc_driver);
        eMMC_debug(0, 1, "EMMC remove slot[%d]!\n", slotNo);
    }

    sysfs_remove_group(&pDev_st->dev.kobj, &mstar_mci_attr_grp);

#if 0
    if(sgp_eMMCThread_st)
        kthread_stop(sgp_eMMCThread_st);
#endif

#ifdef CONFIG_CAM_CLK
    //
#else
    if (clkdata)
    {
        for (slotNo = 0; slotNo < gu8_emmcSlotNums; slotNo++)
        {
            if (clkdata->clk_fcie[slotNo])
            {
                clk_put(clkdata->clk_fcie[slotNo]);
                kfree(clkdata->clk_fcie[slotNo]);
            }
#if (defined(ENABLE_eMMC_HS400) && ENABLE_eMMC_HS400) || (defined(ENABLE_eMMC_HS200) && ENABLE_eMMC_HS200)
            if (clkdata->clk_syn[slotNo])
            {
                clk_put(clkdata->clk_syn[slotNo]);
                kfree(clkdata->clk_syn[slotNo]);
            }
#endif
        }
        kfree(clkdata);
    }
#endif

    kfree(p_emmc_driver);

    return 0;
}

#ifdef CONFIG_PM
static s32 mstar_mci_suspend(struct platform_device *pDev_st, pm_message_t state)
{
    /* Define Local Variables */
    struct mstar_mci_driver *p_emmc_driver = platform_get_drvdata(pDev_st);
    struct mmc_host *        pMMCHost_st;
    U8                       slotNo;
    eMMC_IP_EmType           emmc_ip;
    s32                      ret = 0;

    for (slotNo = 0; slotNo < gu8_emmcSlotNums; slotNo++)
    {
        emmc_ip     = ge_emmcIPOrderSlot[slotNo];
        pMMCHost_st = p_emmc_driver->emmc_host[slotNo]->mmc;

        eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        // wait for D0 high before losing eMMC Vcc
        if (eMMC_ST_SUCCESS != mstar_mci_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH))
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: wait D0 H TO\n");
            eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        }
        eMMC_CMD0(emmc_ip, 0);
        mdelay(10); // msleep(10);
        eMMC_PlatformDeinit();
        eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);

        if (pMMCHost_st)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC, suspend + \n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
            ret = mmc_suspend_host(pMMCHost_st);
#endif
        }

        eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC_%d, suspend -, %Xh\n", emmc_ip, ret);
    }

    u16_OldPLLClkParam    = 0xFFFF;
    u16_OldPLLDLLClkParam = 0xFFFF;

    return ret;
}

static s32 mstar_mci_resume(struct platform_device *pDev_st)
{
    struct mstar_mci_driver *p_emmc_driver = platform_get_drvdata(pDev_st);
    struct mmc_host *        pMMCHost_st;
    U8                       slotNo;
    eMMC_IP_EmType           emmc_ip;
    s32                      ret = 0;

    static u8 u8_IfLock = 0;
    for (slotNo = 0; slotNo < gu8_emmcSlotNums; slotNo++)
    {
        emmc_ip     = ge_emmcIPOrderSlot[slotNo];
        pMMCHost_st = p_emmc_driver->emmc_host[slotNo]->mmc;

        if (0 == (REG_FCIE(FCIE_MIE_FUNC_CTL(emmc_ip)) & BIT_EMMC_ACTIVE))
        {
            eMMC_LockFCIE(emmc_ip, (U8 *)__FUNCTION__);
            u8_IfLock = 1;
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "lock\n");
        }

        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "eMMC, resume +\n");
#if defined(MSTAR_EMMC_CONFIG_OF)
        // enable clock here with clock framework
        {
#ifdef CONFIG_CAM_CLK
            //
#else
            clk_prepare_enable(clkdata->clk_fcie[emmc_ip]);
#endif
        }
#endif
        mstar_mci_enable(emmc_ip);

        if (u8_IfLock)
        {
            u8_IfLock = 0;
            eMMC_UnlockFCIE(emmc_ip, (U8 *)__FUNCTION__);
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "unlock\n");
        }

        if (pMMCHost_st)
        {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
            ret = mmc_resume_host(pMMCHost_st);
#endif
        }

        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "eMMC_%d, resume -\n", emmc_ip);
    }

    return ret;
}
#endif /* End ifdef CONFIG_PM */

/******************************************************************************
 * Define Static Global Variables
 ******************************************************************************/
#if defined(MSTAR_EMMC_CONFIG_OF)
static struct of_device_id ms_mstar_mci_dt_ids[] = {
    {.compatible = DRIVER_NAME},
    {},
};
#endif

static struct platform_driver sg_mstar_mci_driver = {
    .probe  = mstar_mci_probe,
    .remove = __exit_p(mstar_mci_remove),

#ifdef CONFIG_PM
    .suspend = mstar_mci_suspend,
    .resume  = mstar_mci_resume,
#endif

    .driver =
        {
            .name  = DRIVER_NAME,
            .owner = THIS_MODULE,
#if defined(MSTAR_EMMC_CONFIG_OF)
            .of_match_table = ms_mstar_mci_dt_ids,
#endif
        },
};

extern int                Chip_Boot_Get_Dev_Type(void);
extern unsigned long long Chip_MIU_to_Phys(unsigned long long phys);

/******************************************************************************
 * Init & Exit Modules
 ******************************************************************************/
static s32 __init mstar_mci_init(void)
{
    int err = 0;
    U8  i;

#if 0
    if(MS_BOOT_DEV_EMMC!=(MS_BOOT_DEV_TYPE)Chip_Boot_Get_Dev_Type())
    {
        pr_info("[eMMC] skipping device initialization\n");
        return -1;
    }
#endif
    for (i = 0; i < EMMC_NUM_TOTAL; i++)
    {
        memset(&g_eMMCDrv[i], 0, sizeof(eMMC_DRIVER));
    }

    MIU0_BUS_ADDR = Chip_MIU_to_Phys(0); // get the MIU0 base;

    eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 1, "\n");

#if !defined(MSTAR_EMMC_CONFIG_OF)
    if ((err = platform_device_register(&sg_mstar_emmc_device_st)) < 0)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: platform_driver_register fail, %Xh\n", err);
#endif

    if ((err = platform_driver_register(&sg_mstar_mci_driver)) < 0)
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: platform_driver_register fail, %Xh\n", err);

    return err;
}

static void __exit mstar_mci_exit(void)
{
    platform_driver_unregister(&sg_mstar_mci_driver);
}
bool mstar_mci_is_mstar_host(struct mmc_card *card)
{
    struct mstar_mci_host *mci_host;

    if (NULL == (card) || NULL == (card->host) || NULL == (mci_host = ((struct mstar_mci_host *)card->host->private))
        || 0 != strncmp(mci_host->name, MSTAR_MCI_NAME, strlen(MSTAR_MCI_NAME)))
    {
        return false;
    }
    else
    {
        return true;
    }
}

EXPORT_SYMBOL(mstar_mci_is_mstar_host);

#if 0
static int __init write_seg_size_setup(char *str)
{
    wr_seg_size =  simple_strtoul(str, NULL, 16);
    return 1;
}

static int __init write_seg_theshold_setup(char *str)
{
    wr_split_threshold =  simple_strtoul(str, NULL, 16);
    return 1;
}
#endif

/* SAR5=ON in set_config will enable this feature */
#if 0
static int __init sar5_setup_for_pwr_cut(char * str)
{
    if(str != NULL)
    {
        printk(KERN_CRIT"SAR5=%s", str);
        if(strcmp((const char *) str, "ON") == 0)
            u8_enable_sar5 = 1;
    }

    return 0;
}
early_param("SAR5", sar5_setup_for_pwr_cut);
#endif

#if 0
__setup("mmc_wrsize=", write_seg_size_setup);
__setup("mmc_wrupsize=", write_seg_theshold_setup);
#endif

#ifndef CONFIG_MS_EMMC_MODULE
#if !defined(MSTAR_EMMC_CONFIG_OF)
subsys_initcall(mstar_mci_init);
#endif
#endif
module_init(mstar_mci_init);
module_exit(mstar_mci_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sstar Multimedia Card Interface driver");
MODULE_AUTHOR("SSTAR");
MODULE_ALIAS("platform:sstar_kdrv_emmc");
