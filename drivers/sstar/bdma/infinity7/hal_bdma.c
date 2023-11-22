/*
 * hal_bdma.c- Sigmastar
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
// Include files
/*=============================================================*/

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include "ms_platform.h"
#include "ms_types.h"
#include "registers.h"
#include "kernel_bdma.h"
#include "hal_bdma.h"
#include "cam_os_wrapper.h"
#ifdef BDMA_USE_CMDQ_ENABLE
#include "mhal_cmdq.h"
#endif

#define GROUPNUM                 5
#define BDMA_CH_NUM              4
#define BDMA_CHN_GET_GROUP(x)    ((x) / (BDMA_CH_NUM))
#define BDMA_CHN_GET_GROUPCHN(x) ((x) % (BDMA_CH_NUM))

////////////////////////////////////////////////////////////////////////////////
// Global variable
////////////////////////////////////////////////////////////////////////////////

struct sstar_bdma_t
{
    volatile int             bdma_bank;
    volatile int             bdma_group;
    volatile bool            bdma_free;
    volatile bool            bdma_init;
    volatile KeBdma_t *      bdma_base;
    CamOsTsem_t              bdma_sem;
    volatile HalBdmaParam_t *bdma_parm;
};

static struct sstar_bdma_t sstar_bdmas[GROUPNUM][BDMA_CH_NUM];

/*=============================================================*/
// Local function definition
/*=============================================================*/

int HalBdma_CleanIrq_GetChn(int Group)
{
    int i;

    for (i = 0; i < BDMA_CH_NUM; i++)
    {
        volatile KeBdma_t *g_ptKeBdma = sstar_bdmas[Group][i].bdma_base;
        if (g_ptKeBdma->reg_ch0_int_bdma)
        {
            g_ptKeBdma->reg_ch0_int_bdma = 0x1;
            g_ptKeBdma->reg_ch0_int_en   = 0x0;
            return i;
        }
    }
    return -1;
}

irqreturn_t HalBdma_ISR(int irq, void *priv)
{
    int chn;
    int group = *(int *)priv;

    chn = HalBdma_CleanIrq_GetChn(group);
    if (chn < 0)
    {
        CamOsPrintf("[BDMA_IRQ] Should never happen !\n");
        return IRQ_NONE;
    }

    if (NULL != sstar_bdmas[group][chn].bdma_parm->pfTxCbFunc)
    {
        sstar_bdmas[group][chn].bdma_parm->pfTxCbFunc(sstar_bdmas[group][chn].bdma_parm->pTxCbParm);
    }

    sstar_bdmas[group][chn].bdma_free = TRUE;
    CamOsTsemUp(&sstar_bdmas[group][chn].bdma_sem);

    return IRQ_HANDLED;
}

//------------------------------------------------------------------------------
//  Function    : HalBdma_Initialize
//  Description :
//------------------------------------------------------------------------------
HalBdmaErr_e HalBdma_Initialize(u8 u8DmaCh)
{
    int Group = BDMA_CHN_GET_GROUP(u8DmaCh);

    // Groupchn = BDMA_CHN_GET_GROUPCHN(u8DmaCh);
    if (!sstar_bdmas[Group][0].bdma_init)
    {
        struct device_node *dev_node = NULL;
        char                compatible[16];
        int                 iIrqNum = 0;
        int                 i;
        unsigned int        bdmanode_base[4];
        sstar_bdmas[Group][0].bdma_group = Group;

        CamOsSnprintf(compatible, sizeof(compatible), "sstar,bdma%d", Group);
        dev_node = of_find_compatible_node(NULL, NULL, compatible);

        if (!dev_node)
        {
            return HAL_BDMA_ERROR;
        }

        /* Register interrupt handler */
        iIrqNum = irq_of_parse_and_map(dev_node, 0);

        if (0 != request_irq(iIrqNum, HalBdma_ISR, IRQF_SHARED, "BdmaIsr", (void *)&sstar_bdmas[Group][0].bdma_group))
        {
            CamOsPrintf("[BDMA] request_irq [%d] Fail\r\n", iIrqNum);
            return HAL_BDMA_ERROR;
        }
        else
        {
            // CamOsPrintf("[BDMA] request_irq [%d] OK\r\n", iIrqNum);
        }

        for (i = 0; i < BDMA_CH_NUM; i++)
        {
            // sstar_bdmas[Group][i].bdma_base = (struct KeBdma_s *)(unsigned long)(of_iomap(dev_node, 0)+(0x80*i));
            sstar_bdmas[Group][i].bdma_init = TRUE;
            /* Initial semaphore */
            CamOsTsemInit(&sstar_bdmas[Group][i].bdma_sem, 1);

            // if (of_property_read_u32(dev_node, "reg", &bdmanode_base))
            if (of_property_read_u32_array(dev_node, "reg", bdmanode_base, 4))
            {
                CamOsPrintf("Can't get I/O resource regs for BDMA%d\n", Group);
                return -1;
            }

            sstar_bdmas[Group][i].bdma_base =
                (struct KeBdma_s *)(IO_ADDRESS((unsigned long)bdmanode_base[1] + (0x80 * i)));
        }
    }

    return HAL_BDMA_PROC_DONE;
}
EXPORT_SYMBOL(HalBdma_Initialize);

//------------------------------------------------------------------------------
//  Function    : HalBdma_Transfer
//  Description :
//------------------------------------------------------------------------------
/**
 * @brief BDMA starts to transfer data
 *
 * @param [in]  ptBdmaParam      BDMA configuration parameter
 *
 * @return HalBdmaErr_e BDMA error code
 */
HalBdmaErr_e HalBdma_Transfer(u8 u8DmaCh, HalBdmaParam_t *ptBdmaParam)
{
    int                Group      = BDMA_CHN_GET_GROUP(u8DmaCh);
    int                chn        = BDMA_CHN_GET_GROUPCHN(u8DmaCh);
    volatile KeBdma_t *g_ptKeBdma = sstar_bdmas[Group][chn].bdma_base;

    if (!sstar_bdmas[Group][chn].bdma_init)
    {
        return HAL_BDMA_NO_INIT;
    }

    CamOsTsemDown(&sstar_bdmas[Group][chn].bdma_sem);

    sstar_bdmas[Group][chn].bdma_free = FALSE;
    sstar_bdmas[Group][chn].bdma_parm = ptBdmaParam;

    g_ptKeBdma->reg_ch0_busy     = 0x1;
    g_ptKeBdma->reg_ch0_int_bdma = 0x1;
    g_ptKeBdma->reg_ch0_done     = 0x1;

    switch (ptBdmaParam->ePathSel)
    {
        case HAL_BDMA_MIU0_TO_MIU0:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU0 | REG_BDMA_CH1_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU0_TO_MIU1:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU0 | REG_BDMA_CH1_MIU1;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU1_TO_MIU0:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU1_TO_MIU1:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_MIU1;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU0_TO_IMI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU0 | REG_BDMA_CH1_IMI;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU1_TO_IMI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_IMI;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_IMI_TO_MIU0:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_IMI | REG_BDMA_CH1_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_IMI_TO_MIU1:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_IMI | REG_BDMA_CH1_MIU1;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_IMI_TO_IMI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_IMI | REG_BDMA_CH1_IMI;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MEM_TO_MIU0:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MEM_FILL;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_4BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MEM_TO_MIU1:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MEM_FILL;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_MIU1;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_4BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MEM_TO_IMI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MEM_FILL;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_IMI;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_4BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_SPI_TO_MIU0:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_SPI;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_8BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU0_TO_SPI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SDT_FSP;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_4BYTE;
            break;
        case HAL_BDMA_SPI_TO_MIU1:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_SPI;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_MIU1;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_8BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_SPI_TO_IMI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_SPI;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_SRC_MIU_IMI_CH1;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH1_IMI;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_8BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MSPI_TO_MIU:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MSPI;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_DST_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        case HAL_BDMA_MIU_TO_MSPI:
            g_ptKeBdma->reg_ch0_src_sel     = REG_BDMA_SRC_MIU_IMI_CH0;
            g_ptKeBdma->reg_ch0_dst_sel     = REG_BDMA_DST_MSPI;
            g_ptKeBdma->reg_ch0_replace_miu = REG_BDMA_CH0_MIU0;
            g_ptKeBdma->reg_ch0_src_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            g_ptKeBdma->reg_ch0_dst_dw      = REG_BDMA_DATA_DEPTH_16BYTE;
            break;
        default:
            return HAL_BDMA_PROC_DONE;
            break;
    }

    // Set Source / Destination Address
    if ((HAL_BDMA_MEM_TO_MIU0 == ptBdmaParam->ePathSel) || (HAL_BDMA_MEM_TO_MIU1 == ptBdmaParam->ePathSel)
        || (HAL_BDMA_MEM_TO_IMI == ptBdmaParam->ePathSel))
    {
        g_ptKeBdma->reg_ch0_cmd0_low  = (U16)(ptBdmaParam->u32Pattern & 0xFFFF);
        g_ptKeBdma->reg_ch0_cmd0_high = (U16)(ptBdmaParam->u32Pattern >> 16);
        g_ptKeBdma->reg_ch0_src_a0    = (U16)((0x0000) & 0xFFFF);
        g_ptKeBdma->reg_ch0_src_a1    = (U16)((0x0000) & 0xFFFF);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
        g_ptKeBdma->reg_ch0_src_a_msb = (U16)((0x0) & 0xF);
#endif
    }
    else
    {
        g_ptKeBdma->reg_ch0_src_a0 = (U16)((ptBdmaParam->pSrcAddr) & 0xFFFF);
        g_ptKeBdma->reg_ch0_src_a1 = (U16)(((ptBdmaParam->pSrcAddr) >> 16) & 0xFFFF);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
        g_ptKeBdma->reg_ch0_src_a_msb = (U16)(((ptBdmaParam->pSrcAddr) >> 32) & 0xF);
#endif
    }

    g_ptKeBdma->reg_ch0_dst_a0 = (U16)((ptBdmaParam->pDstAddr) & 0xFFFF);
    g_ptKeBdma->reg_ch0_dst_a1 = (U16)(((ptBdmaParam->pDstAddr) >> 16) & 0xFFFF);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
    g_ptKeBdma->reg_ch0_dst_a_msb = (U16)(((ptBdmaParam->pDstAddr) >> 32) & 0xF);
#endif

    // Set Transfer Size
    g_ptKeBdma->reg_ch0_size0 = (U16)(ptBdmaParam->u32TxCount & 0xFFFF);
    g_ptKeBdma->reg_ch0_size1 = (U16)(ptBdmaParam->u32TxCount >> 16);

    /* Set LineOffset Attribute */
    if (ptBdmaParam->bEnLineOfst == TRUE)
    {
        g_ptKeBdma->reg_ch0_src_width_low   = (U16)(ptBdmaParam->pstLineOfst->u32SrcWidth & 0xFFFF);
        g_ptKeBdma->reg_ch0_src_width_high  = (U16)(ptBdmaParam->pstLineOfst->u32SrcWidth >> 16);
        g_ptKeBdma->reg_ch0_src_offset_low  = (U16)(ptBdmaParam->pstLineOfst->u32SrcOffset & 0xFFFF);
        g_ptKeBdma->reg_ch0_src_offset_high = (U16)(ptBdmaParam->pstLineOfst->u32SrcOffset >> 16);
        g_ptKeBdma->reg_ch0_dst_width_low   = (U16)(ptBdmaParam->pstLineOfst->u32DstWidth & 0xFFFF);
        g_ptKeBdma->reg_ch0_dst_width_high  = (U16)(ptBdmaParam->pstLineOfst->u32DstWidth >> 16);
        g_ptKeBdma->reg_ch0_dst_offset_low  = (U16)(ptBdmaParam->pstLineOfst->u32DstOffset & 0xFFFF);
        g_ptKeBdma->reg_ch0_dst_offset_high = (U16)(ptBdmaParam->pstLineOfst->u32DstOffset >> 16);
        g_ptKeBdma->reg_ch0_offset_en       = 1;
    }
    else
    {
        g_ptKeBdma->reg_ch0_offset_en = 0;
    }

    // Set Interrupt Enable
    if (ptBdmaParam->bIntMode)
    {
        g_ptKeBdma->reg_ch0_int_en = 1;
    }
    else
    {
        g_ptKeBdma->reg_ch0_int_en = 0;
    }

    // Trigger
    g_ptKeBdma->reg_ch0_trig = 0x1;

    // Polling mode
    if (!ptBdmaParam->bIntMode)
    {
        HalBdma_WaitTransferDone(u8DmaCh, ptBdmaParam);
    }

    return HAL_BDMA_PROC_DONE;
}
EXPORT_SYMBOL(HalBdma_Transfer);

#ifdef BDMA_USE_CMDQ_ENABLE
//------------------------------------------------------------------------------
//  Function    : HalBdma_Transfer_cmdq
//  Description :
//------------------------------------------------------------------------------
/**
 * @brief BDMA starts to transfer data
 *
 * @param [in]  ptBdmaParam      BDMA configuration parameter
 *
 * @return HalBdmaErr_e BDMA error code
 */
HalBdmaErr_e HalBdma_Transfer_cmdq(u8 u8DmaCh, HalBdmaParam_t *ptBdmaParam, void *pCmdQInfs)
{
    int                        Group    = BDMA_CHN_GET_GROUP(u8DmaCh);
    int                        chn      = BDMA_CHN_GET_GROUPCHN(u8DmaCh);
    int                        cmdnum   = 0;
    MHAL_CMDQ_CmdqInterface_t *pCmdQInf = (MHAL_CMDQ_CmdqInterface_t *)pCmdQInfs;
    if (!sstar_bdmas[Group][chn].bdma_init)
    {
        return HAL_BDMA_NO_INIT;
    }

    if (Group == 0)
    {
        sstar_bdmas[Group][chn].bdma_bank = 0x100200 + ((0x20 * chn) << 1);
    }
    else if (Group == 1)
        sstar_bdmas[Group][chn].bdma_bank = 0x100B00 + ((0x20 * chn) << 1);
    else
        return HAL_BDMA_NO_SUPPOSE_CMDQ;

    CamOsTsemDown(&sstar_bdmas[Group][chn].bdma_sem);

    sstar_bdmas[Group][chn].bdma_free = FALSE;
    sstar_bdmas[Group][chn].bdma_parm = ptBdmaParam;
    pCmdQInf->MHAL_CMDQ_WriteRegCmdqMask(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_STATUS, 0xE, 0xE);

    cmdnum++;

    switch (ptBdmaParam->ePathSel)
    {
        case HAL_BDMA_MIU0_TO_MIU0:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU0 | REG_BDMA_CH1_MIU0) << 12);
            break;
        case HAL_BDMA_MIU0_TO_MIU1:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU0 | REG_BDMA_CH1_MIU1) << 12);
            break;
        case HAL_BDMA_MIU1_TO_MIU0:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_MIU0) << 12);
            break;
        case HAL_BDMA_MIU1_TO_MIU1:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_MIU1) << 12);
            break;
        case HAL_BDMA_MIU0_TO_IMI:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_MIU1) << 12);
            break;
        case HAL_BDMA_MIU1_TO_IMI:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU1 | REG_BDMA_CH1_IMI) << 12);
            break;
        case HAL_BDMA_IMI_TO_MIU0:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_IMI | REG_BDMA_CH1_MIU0) << 12);
            break;
        case HAL_BDMA_IMI_TO_MIU1:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_IMI | REG_BDMA_CH1_MIU1) << 12);
            break;
        case HAL_BDMA_IMI_TO_IMI:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             (REG_BDMA_DATA_DEPTH_16BYTE << 4) | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_IMI | REG_BDMA_CH1_IMI) << 12);
            break;
        case HAL_BDMA_MEM_TO_MIU0:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_MEM_FILL | (REG_BDMA_DATA_DEPTH_4BYTE << 4)
                                                 | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH1_MIU0) << 12);
            break;
        case HAL_BDMA_MEM_TO_MIU1:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_MEM_FILL | (REG_BDMA_DATA_DEPTH_4BYTE << 4)
                                                 | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH1_MIU1) << 12);
            break;
        case HAL_BDMA_MEM_TO_IMI:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_MEM_FILL | (REG_BDMA_DATA_DEPTH_4BYTE << 4)
                                                 | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH1_IMI) << 12);
            break;
        case HAL_BDMA_SPI_TO_MIU0:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_SPI | (REG_BDMA_DATA_DEPTH_8BYTE << 4)
                                                 | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH1_MIU0) << 12);
            break;
        case HAL_BDMA_SPI_TO_MIU1:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_SPI | (REG_BDMA_DATA_DEPTH_8BYTE << 4)
                                                 | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH1_MIU1) << 12);
            break;
        case HAL_BDMA_SPI_TO_IMI:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_SPI | (REG_BDMA_DATA_DEPTH_8BYTE << 4)
                                                 | (REG_BDMA_SRC_MIU_IMI_CH1 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH1_IMI) << 12);
            break;
        case HAL_BDMA_MSPI_TO_MIU:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_MSPI | (REG_BDMA_DATA_DEPTH_8BYTE << 4)
                                                 | (REG_BDMA_DST_MIU_IMI_CH0 << 8)
                                                 | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU0) << 12);
            break;
        case HAL_BDMA_MIU_TO_MSPI:
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SEL_SRCDST,
                                             REG_BDMA_SRC_MIU_IMI_CH0 | (REG_BDMA_DATA_DEPTH_8BYTE << 4)
                                                 | (REG_BDMA_DST_MSPI << 8) | (REG_BDMA_DATA_DEPTH_16BYTE << 12));
            pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU,
                                             (REG_BDMA_CH0_MIU0) << 12);
            break;
        default:
            return HAL_BDMA_PROC_DONE;
            break;
    }
    cmdnum += 2;

    // Set Source / Destination Address
    if ((HAL_BDMA_MEM_TO_MIU0 == ptBdmaParam->ePathSel) || (HAL_BDMA_MEM_TO_MIU1 == ptBdmaParam->ePathSel)
        || (HAL_BDMA_MEM_TO_IMI == ptBdmaParam->ePathSel))
    {
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SPECIAL_CMD0_L,
                                         (U16)(ptBdmaParam->u32Pattern & 0xFFFF));
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SPECIAL_CMD0_H,
                                         (U16)(ptBdmaParam->u32Pattern >> 16));
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SRC_ADDR_L,
                                         (U16)((ptBdmaParam->pSrcAddr) & 0xFFFF));
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SRC_ADDR_H,
                                         (U16)(((ptBdmaParam->pSrcAddr) >> 16) & 0xFFFF));
        cmdnum += 2;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SRC_ADDR_MSB4,
                                         (U16)(((ptBdmaParam->pSrcAddr) >> 32) & 0xF));
        cmdnum++;
#endif
    }
    else
    {
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SRC_ADDR_L,
                                         (U16)((ptBdmaParam->pSrcAddr) & 0xFFFF));
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SRC_ADDR_H,
                                         (U16)(((ptBdmaParam->pSrcAddr) >> 16) & 0xFFFF));
#ifdef CONFIG_PHYS_ADDR_T_64BIT
        pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SRC_ADDR_MSB4,
                                         (U16)(((ptBdmaParam->pSrcAddr) >> 32) & 0xF));
        cmdnum++;
#endif
    }
    pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_DST_ADDR_L,
                                     (U16)((ptBdmaParam->pDstAddr) & 0xFFFF));
    pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_DST_ADDR_H,
                                     (U16)(((ptBdmaParam->pDstAddr) >> 16) & 0xFFFF));
    cmdnum += 4;
#ifdef CONFIG_PHYS_ADDR_T_64BIT
    pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_DST_ADDR_MSB4,
                                     (U16)(((ptBdmaParam->pDstAddr) >> 32) & 0xF));
    cmdnum++;
#endif

    // Set Transfer Size
    pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SIZE_L,
                                     (U16)(ptBdmaParam->u32TxCount & 0xFFFF));
    pCmdQInf->MHAL_CMDQ_WriteRegCmdq(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_SIZE_H,
                                     (U16)(ptBdmaParam->u32TxCount >> 16));
    cmdnum += 2;
    // Set Interrupt Enable
    if (ptBdmaParam->bIntMode)
    {
        pCmdQInf->MHAL_CMDQ_WriteRegCmdqMask(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU, 0x2,
                                             0x2);
    }
    else
    {
        pCmdQInf->MHAL_CMDQ_WriteRegCmdqMask(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_REPALCE_MIU, 0x0,
                                             0x2);
    }
    cmdnum++;
    // Trigger
    pCmdQInf->MHAL_CMDQ_WriteRegCmdqMask(pCmdQInf, sstar_bdmas[Group][chn].bdma_bank + REG_BDMA_BK_TRIG, 0x1, 0x1);
    cmdnum++;

    while (0 == pCmdQInf->MHAL_CMDQ_CheckBufAvailable(pCmdQInf, cmdnum))
    {
        CamOsPrintf("CheckBufAvailable failed.waiting..\n");
        CamOsMsDelay(100);
    }

    if (Group == 0)
        pCmdQInf->MHAL_CMDQ_CmdqAddWaitEventCmd(pCmdQInf, E_MHAL_CMDQEVE_BDMA0_TRIG);
    else
        pCmdQInf->MHAL_CMDQ_CmdqAddWaitEventCmd(pCmdQInf, E_MHAL_CMDQEVE_BDMA1_TRIG);

    pCmdQInf->MHAL_CMDQ_KickOffCmdq(pCmdQInf);

    return HAL_BDMA_PROC_DONE;
}
EXPORT_SYMBOL(HalBdma_Transfer_cmdq);
#endif

//------------------------------------------------------------------------------
//  Function    : HalBdma_WaitTransferDone
//  Description :
//------------------------------------------------------------------------------
/**
 * @brief BDMA wait transfer data done
 *
 * @param [in]  ptBdmaParam      BDMA configuration parameter
 *
 * @return HalBdmaErr_e BDMA error code
 */
HalBdmaErr_e HalBdma_WaitTransferDone(u8 u8DmaCh, HalBdmaParam_t *ptBdmaParam)
{
    U32                u32TimeOut = 0x00FFFFFF;
    bool               bRet       = FALSE;
    int                Group      = BDMA_CHN_GET_GROUP(u8DmaCh);
    int                chn        = BDMA_CHN_GET_GROUPCHN(u8DmaCh);
    volatile KeBdma_t *g_ptKeBdma = sstar_bdmas[Group][chn].bdma_base;

    if (!sstar_bdmas[Group][chn].bdma_init)
    {
        return HAL_BDMA_PROC_DONE;
    }

    // Polling mode
    if (!ptBdmaParam->bIntMode)
    {
        while (--u32TimeOut)
        {
            // Check done
            if (g_ptKeBdma->reg_ch0_done == 0x1)
            {
                bRet = TRUE;
                break;
            }
        }

        // Clear done
        g_ptKeBdma->reg_ch0_done = 0x1;

        sstar_bdmas[Group][chn].bdma_free = TRUE;
        CamOsTsemUp(&sstar_bdmas[Group][chn].bdma_sem);

        if (bRet == FALSE)
        {
            CamOsPrintf("Wait BDMA Done Fail\r\n");
            return HAL_BDMA_POLLING_TIMEOUT;
        }
    }
    else
    {
        // Interrupt mode
    }

    return HAL_BDMA_PROC_DONE;
}
