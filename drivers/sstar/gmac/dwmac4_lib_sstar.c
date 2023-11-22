/*
 * dwmac4_lib_sstar.c- Sigmastar
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

#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/delay.h>
#include "common.h"
#include "dwmac4_sstar_dma.h"
#include "dwmac4.h"
#include "registers.h"
#include "sstar_gmac.h"
#include "stmmac.h"
extern void *GBase[];
extern char  gbDisirq[];

int dwmac4_sstar_dma_reset(void __iomem *ioaddr)
{
    u32 value    = readl(ioaddr + DMA_BUS_MODE);
    u8  bIsRGMII = 0, GmacId = 0;
    u16 val;

    /* DMA SW reset */
    value |= DMA_BUS_MODE_SFT_RESET;
    writel(value, ioaddr + DMA_BUS_MODE);

    GmacId = sstar_gmac_iomem2id(ioaddr);

    if (GmacId == GMAC1)
    {
        val = INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x01));
    }
    else if (GmacId == GMAC0)
    {
        val = INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x01));
    }
    else
    {
        printk("dwmac4_sstar_dma_reset : unknow GmacId\n");
        return -1;
    }

    if ((val & 0x7) == 1)
        bIsRGMII = 1;

    /*
     * Sstar gmac will read x32 speed setting
     * if speed > 1000Mbps, tx/rx/app clk will be gated
     * so write a fake 10Mbps setting into x32 register
     * */
    if (GmacId) // GMAC1
        OUTREG32(GET_REG_ADDR(BASE_REG_X32_GMAC1_PA, 0x0), 0xA000);
    else // GMAC0
        OUTREG32(GET_REG_ADDR(BASE_REG_X32_GMAC0_PA, 0x0), 0xA000);

    if (bIsRGMII)
    {
        return readl_poll_timeout(ioaddr + DMA_BUS_MODE, value, !(value & DMA_BUS_MODE_SFT_RESET), 10000, 1000000);
    }
    else
    {
        u32 timeoutcnt = 100, delayus = 100; // 100x100us = 10ms timeout

        /*
         *Check tx/rx/app clk
         * */
        while (timeoutcnt)
        {
            if (GmacId) // GMAC1
                val = INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x0));
            else // GMAC0
                val = INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x0));
            if ((val & (0x70)) == (0x70))
                break;
            udelay(delayus);
            timeoutcnt--;
        }

        if (timeoutcnt == 0)
        {
            printk("DMA reset failed for MII/RMII!\n");
            return -ETIMEDOUT;
        }

        // Finally, check DMA reset done
        return readl_poll_timeout(ioaddr + DMA_BUS_MODE, value, !(value & DMA_BUS_MODE_SFT_RESET), 10000, 1000000);
    }

    return 0;
}

void dwmac4_sstar_set_rx_tail_ptr(void __iomem *ioaddr, u32 tail_ptr, u32 chan)
{
#if (GMAC_USING_MIU_MAPPING_API == 0)
    tail_ptr = PA2BUS(tail_ptr);
#endif
    writel(tail_ptr, ioaddr + DMA_CHAN_RX_END_ADDR(chan));
}

void dwmac4_sstar_set_tx_tail_ptr(void __iomem *ioaddr, u32 tail_ptr, u32 chan)
{
#if (GMAC_USING_MIU_MAPPING_API == 0)
    tail_ptr = PA2BUS(tail_ptr);
#endif
    writel(tail_ptr, ioaddr + DMA_CHAN_TX_END_ADDR(chan));
}

void dwmac4_sstar_dma_start_tx(void __iomem *ioaddr, u32 chan)
{
    u32 value = readl(ioaddr + DMA_CHAN_TX_CONTROL(chan));

    value |= DMA_CONTROL_ST;
    writel(value, ioaddr + DMA_CHAN_TX_CONTROL(chan));

    value = readl(ioaddr + GMAC_CONFIG);
    value |= GMAC_CONFIG_TE;
    writel(value, ioaddr + GMAC_CONFIG);
}

void dwmac4_sstar_dma_stop_tx(void __iomem *ioaddr, u32 chan)
{
    u32 value = readl(ioaddr + DMA_CHAN_TX_CONTROL(chan));

    value &= ~DMA_CONTROL_ST;
    writel(value, ioaddr + DMA_CHAN_TX_CONTROL(chan));

    value = readl(ioaddr + GMAC_CONFIG);
    value &= ~GMAC_CONFIG_TE;
    writel(value, ioaddr + GMAC_CONFIG);
}

void dwmac4_sstar_dma_start_rx(void __iomem *ioaddr, u32 chan)
{
    u32 value = readl(ioaddr + DMA_CHAN_RX_CONTROL(chan));

    value |= DMA_CONTROL_SR;

    writel(value, ioaddr + DMA_CHAN_RX_CONTROL(chan));

    value = readl(ioaddr + GMAC_CONFIG);
    value |= GMAC_CONFIG_RE;
    writel(value, ioaddr + GMAC_CONFIG);
}

void dwmac4_sstar_dma_stop_rx(void __iomem *ioaddr, u32 chan)
{
    u32 value = readl(ioaddr + DMA_CHAN_RX_CONTROL(chan));

    value &= ~DMA_CONTROL_SR;
    writel(value, ioaddr + DMA_CHAN_RX_CONTROL(chan));
}

void dwmac4_sstar_set_tx_ring_len(void __iomem *ioaddr, u32 len, u32 chan)
{
    writel(len, ioaddr + DMA_CHAN_TX_RING_LEN(chan));
}

void dwmac4_sstar_set_rx_ring_len(void __iomem *ioaddr, u32 len, u32 chan)
{
    writel(len, ioaddr + DMA_CHAN_RX_RING_LEN(chan));
}

void dwmac4_sstar_enable_dma_irq(void __iomem *ioaddr, u32 chan, bool rx, bool tx)
{
    u32 value = readl(ioaddr + DMA_CHAN_INTR_ENA(chan));

    if (rx)
        value |= DMA_CHAN_INTR_DEFAULT_RX;
    if (tx)
        value |= DMA_CHAN_INTR_DEFAULT_TX;

    writel(value, ioaddr + DMA_CHAN_INTR_ENA(chan));
}

void dwmac410_sstar_enable_dma_irq(void __iomem *ioaddr, u32 chan, bool rx, bool tx)
{
    u32 value = readl(ioaddr + DMA_CHAN_INTR_ENA(chan));

    if (rx)
        value |= DMA_CHAN_INTR_DEFAULT_RX_4_10;
    if (tx)
        value |= DMA_CHAN_INTR_DEFAULT_TX_4_10;

    writel(value, ioaddr + DMA_CHAN_INTR_ENA(chan));
}

void dwmac4_sstar_disable_dma_irq(void __iomem *ioaddr, u32 chan, bool rx, bool tx)
{
    u32 value;

    if (gbDisirq[sstar_gmac_iomem2id(ioaddr)])
        return;

    value = readl(ioaddr + DMA_CHAN_INTR_ENA(chan));

    if (rx)
        value &= ~DMA_CHAN_INTR_DEFAULT_RX;
    if (tx)
        value &= ~DMA_CHAN_INTR_DEFAULT_TX;

    writel(value, ioaddr + DMA_CHAN_INTR_ENA(chan));
}

void dwmac410_sstar_disable_dma_irq(void __iomem *ioaddr, u32 chan, bool rx, bool tx)
{
    u32 value;

    if (gbDisirq[sstar_gmac_iomem2id(ioaddr)])
        return;

    value = readl(ioaddr + DMA_CHAN_INTR_ENA(chan));

    if (rx)
        value &= ~DMA_CHAN_INTR_DEFAULT_RX_4_10;
    if (tx)
        value &= ~DMA_CHAN_INTR_DEFAULT_TX_4_10;

    writel(value, ioaddr + DMA_CHAN_INTR_ENA(chan));
}

int dwmac4_sstar_dma_interrupt(void __iomem *ioaddr, struct stmmac_extra_stats *x, u32 chan)
{
    u32 intr_status = readl(ioaddr + DMA_CHAN_STATUS(chan));
    u32 intr_en     = readl(ioaddr + DMA_CHAN_INTR_ENA(chan));
    int ret         = 0;

    /* ABNORMAL interrupts */
    if (unlikely(intr_status & DMA_CHAN_STATUS_AIS))
    {
#if 1
        if (unlikely(intr_status & DMA_CHAN_STATUS_RBU))
        {
            x->rx_buf_unav_irq++;
            ret |= handle_rx;
        }
#else
        if (unlikely(intr_status & DMA_CHAN_STATUS_RBU))
        {
            struct stmmac_priv *priv = container_of(x, struct stmmac_priv, xstats);

            ret |= handle_rx;
            x->rx_buf_unav_irq++;
            priv->mmc.mmc_rx_fifo_overflow += readl(priv->mmcaddr + 0xd4);
            printk("[%d] (unav, rfifo) = (%16ld, %16d)\n", sstar_gmac_iomem2id(ioaddr), priv->xstats.rx_buf_unav_irq,
                   priv->mmc.mmc_rx_fifo_overflow);
        }
#endif
        if (unlikely(intr_status & DMA_CHAN_STATUS_RPS))
            x->rx_process_stopped_irq++;
        if (unlikely(intr_status & DMA_CHAN_STATUS_RWT))
            x->rx_watchdog_irq++;
        if (unlikely(intr_status & DMA_CHAN_STATUS_ETI))
            x->tx_early_irq++;
        if (unlikely(intr_status & DMA_CHAN_STATUS_TPS))
        {
            x->tx_process_stopped_irq++;
            ret = tx_hard_error;
        }
        if (unlikely(intr_status & DMA_CHAN_STATUS_FBE))
        {
            x->fatal_bus_error_irq++;
            ret = tx_hard_error;
        }
    }
    /* TX/RX NORMAL interrupts */
    if (likely(intr_status & DMA_CHAN_STATUS_NIS))
    {
        x->normal_irq_n++;
        if (likely(intr_status & DMA_CHAN_STATUS_RI))
        {
            x->rx_normal_irq_n++;
            x->rxq_stats[chan].rx_normal_irq_n++;
            ret |= handle_rx;
        }
        if (likely(intr_status & (DMA_CHAN_STATUS_TI | DMA_CHAN_STATUS_TBU)))
        {
            x->tx_normal_irq_n++;
            x->txq_stats[chan].tx_normal_irq_n++;
            ret |= handle_tx;
        }
#if defined(CONFIG_ARCH_SSTAR) && defined(CONFIG_SSTAR_SNPS_GMAC_RXIC)
        if (likely(intr_status & (DMA_CHAN_STATUS_TI)))
        {
            x->tx_normal_ti_irq_n++;
        }
        if (likely(intr_status & (DMA_CHAN_STATUS_TBU)))
        {
            x->tx_normal_tbu_irq_n++;
        }
#endif
        if (unlikely(intr_status & DMA_CHAN_STATUS_ERI))
            x->rx_early_irq++;
    }
#if defined(CONFIG_ARCH_SSTAR) && defined(CONFIG_SSTAR_SNPS_GMAC_RXIC)
    intr_en = intr_en;
    writel(intr_status & (0xFFFFFFFF), ioaddr + DMA_CHAN_STATUS(chan));
#else
    writel(intr_status & intr_en, ioaddr + DMA_CHAN_STATUS(chan));
#endif
    return ret;
}
