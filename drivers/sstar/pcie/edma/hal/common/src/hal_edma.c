/*
 * hal_edma.c- Sigmastar
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

#include "ms_platform.h"
#include "cam_os_wrapper.h"
#include "hal_edma.h"

#define _HAL_EDMA_DBG_ 0

u16 halEDMA_GetWrChCnt(u8 id)
{
    u32 val;

    if (id >= sstar_pcieif_num_of_intf())
        return 0;

    val = _readl_dma_reg(id, DMA_CTRL_OFF);
    val = (val & EDMA_WRITE_CH_CNT_MASK) >> EDMA_WRITE_CH_CNT_SHIFT;
#if _HAL_EDMA_DBG_
    CamOsPrintf(KERN_INFO "Number of WR channels: %d\r\n", val);
#endif
    return val;
}

u16 halEDMA_GetRdChCnt(u8 id)
{
    u32 val;

    if (id >= sstar_pcieif_num_of_intf())
        return 0;

    val = _readl_dma_reg(id, DMA_CTRL_OFF);
    val = (val & EDMA_READ_CH_CNT_MASK) >> EDMA_READ_CH_CNT_SHIFT;
#if _HAL_EDMA_DBG_
    CamOsPrintf(KERN_INFO "Number of RD channels: %d\r\n", val);
#endif
    return val;
}

int halEDMA_StartXfer(u8 id, u32 ch, bool write, ss_miu_addr_t ll)
{
    u32 val;

    if (id >= sstar_pcieif_num_of_intf())
        return -EINVAL;

    if (write)
    {
        /* Power enable */
        _writel_dma_reg(id, DMA_WRITE_CHi_PWR_EN_OFF(ch), EDMA_PWR_EN);
        /* Enable engine */
        _writel_dma_reg(id, DMA_WRITE_ENGINE_EN_OFF, EDMA_ENGINE_EN);
        /* Interrupt unmask - done, abort */
        val = _readl_dma_reg(id, DMA_WRITE_INT_MASK_OFF);
        val &= ~(EDMA_DONE_INT_CHi(ch) | EDMA_ABORT_INT_CHi(ch));
        _writel_dma_reg(id, DMA_WRITE_INT_MASK_OFF, val);
        /* Linked list error */
        val = _readl_dma_reg(id, DMA_WRITE_LINKED_LIST_ERR_EN_OFF);
        _writel_dma_reg(id, DMA_WRITE_LINKED_LIST_ERR_EN_OFF, val | EDMA_CHi_LLRAIE(ch) | EDMA_CHi_LLLAIE(ch));
        /* Channel control */
        _writel_dma_reg(id, DMA_CH_CONTROL1_OFF_WRCH_i(ch), EDMA_CH_CTRL1_CCS | EDMA_CH_CTRL1_LLE);
        /* Linked list - low, high */
        _writel_dma_reg(id, DMA_LLP_LOW_OFF_WRCH_i(ch), lower_32_bits(ll));
        _writel_dma_reg(id, DMA_LLP_HIGH_OFF_WRCH_i(ch), upper_32_bits(ll));
        /* Doorbell */
        _writel_dma_reg(id, DMA_WRITE_DOORBELL_OFF, ch);
    }
    else
    {
        /* Power enable */
        _writel_dma_reg(id, DMA_READ_CHi_PWR_EN_OFF(ch), EDMA_PWR_EN);
        /* Enable engine */
        _writel_dma_reg(id, DMA_READ_ENGINE_EN_OFF, EDMA_ENGINE_EN);
        /* Interrupt unmask - done, abort */
        val = _readl_dma_reg(id, DMA_READ_INT_MASK_OFF);
        val &= ~(EDMA_DONE_INT_CHi(ch) | EDMA_ABORT_INT_CHi(ch));
        _writel_dma_reg(id, DMA_READ_INT_MASK_OFF, val);
        /* Linked list error */
        val = _readl_dma_reg(id, DMA_READ_LINKED_LIST_ERR_EN_OFF);
        _writel_dma_reg(id, DMA_READ_LINKED_LIST_ERR_EN_OFF, val | EDMA_CHi_LLRAIE(ch) | EDMA_CHi_LLLAIE(ch));
        /* Channel control */
        _writel_dma_reg(id, DMA_CH_CONTROL1_OFF_RDCH_i(ch), EDMA_CH_CTRL1_CCS | EDMA_CH_CTRL1_LLE);
        /* Linked list - low, high */
        _writel_dma_reg(id, DMA_LLP_LOW_OFF_RDCH_i(ch), lower_32_bits(ll));
        _writel_dma_reg(id, DMA_LLP_HIGH_OFF_RDCH_i(ch), upper_32_bits(ll));
        /* Doorbell */
        _writel_dma_reg(id, DMA_READ_DOORBELL_OFF, ch);
    }
    return 0;
}

void halEDMA_StopDev(u8 id, bool write)
{
    if (write)
    {
        _writel_dma_reg(id, DMA_WRITE_INT_MASK_OFF, EDMA_DONE_INT_MASK | EDMA_ABORT_INT_MASK);
        _writel_dma_reg(id, DMA_WRITE_INT_CLEAR_OFF, EDMA_DONE_INT_MASK | EDMA_ABORT_INT_MASK);
        _writel_dma_reg(id, DMA_WRITE_ENGINE_EN_OFF, 0);
    }
    else
    {
        _writel_dma_reg(id, DMA_READ_INT_MASK_OFF, EDMA_DONE_INT_MASK | EDMA_ABORT_INT_MASK);
        _writel_dma_reg(id, DMA_READ_INT_CLEAR_OFF, EDMA_DONE_INT_MASK | EDMA_ABORT_INT_MASK);
        _writel_dma_reg(id, DMA_READ_ENGINE_EN_OFF, 0);
    }
}

u8 halEDMA_GetDoneIntrSt(u8 id, bool write)
{
    u32 val;

#if _HAL_EDMA_DBG_
    CamOsPrintf(KERN_INFO "R[%X] : 0x%X\r\n", m_baseRegMac[id] + (0xB << 2), ms_readw(m_baseRegMac[id] + (0xB << 2)));
    CamOsPrintf(KERN_INFO "R[%X] : 0x%X\r\n", m_baseRegMac[id] + (0x14 << 2), ms_readw(m_baseRegMac[id] + (0x14 << 2)));
    CamOsPrintf(KERN_INFO "R[%X] : 0x%X\r\n", m_baseRegMac[id] + (0x19 << 2), ms_readw(m_baseRegMac[id] + (0x19 << 2)));
#endif

    if (write)
        val = _readl_dma_reg(id, DMA_WRITE_INT_STATUS_OFF);
    else
        val = _readl_dma_reg(id, DMA_READ_INT_STATUS_OFF);

    return (val & EDMA_DONE_INT_MASK) >> EDMA_DONE_INT_SHIFT;
}

u8 halEDMA_GetAbortIntrSt(u8 id, bool write)
{
    u32 val;

    if (write)
        val = _readl_dma_reg(id, DMA_WRITE_INT_STATUS_OFF);
    else
        val = _readl_dma_reg(id, DMA_READ_INT_STATUS_OFF);

    return (val & EDMA_ABORT_INT_MASK) >> EDMA_ABORT_INT_SHIFT;
}

void halEDMA_ClrDoneIntrSt(u8 id, u8 st, bool write)
{
    u32 val = ((u32)st << EDMA_DONE_INT_SHIFT) & EDMA_DONE_INT_MASK;

    if (write)
        return _writel_dma_reg(id, DMA_WRITE_INT_CLEAR_OFF, val);
    else
        return _writel_dma_reg(id, DMA_READ_INT_CLEAR_OFF, val);
}

void halEDMA_ClrAbortIntrSt(u8 id, u8 st, bool write)
{
    u32 val = ((u32)st << EDMA_ABORT_INT_SHIFT) & EDMA_ABORT_INT_MASK;

    if (write)
        return _writel_dma_reg(id, DMA_WRITE_INT_CLEAR_OFF, val);
    else
        return _writel_dma_reg(id, DMA_READ_INT_CLEAR_OFF, val);
}
