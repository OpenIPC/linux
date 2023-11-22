/*
 * xor_dma.c- Sigmastar
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

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/msi.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include "xor_dma.h"
#include "ms_platform.h"

static ssize_t set_dma_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
static ssize_t show_dma_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 1;
}
DEVICE_ATTR(xor_info, 0644, show_dma_info, set_dma_info);

void sstar_dma_init(struct sstar_chan *ch)
{
    dma_info("%s>>>\n", __func__);
    BDMA_WRITE_MASK(REG_BDMA_CFG, 0x0300, BDMA_SRC_TLB_MASK | BDMA_DST_TLB_MASK);
    BDMA_WRITE_MASK(REG_BDMA_SEL, BDMA_SRC_MIU_IMI_CH0, BDMA_SRC_SEL_MASK);
    BDMA_WRITE_MASK(REG_BDMA_SEL, BDMA_DATA_SRC_WIDTH_16BYTE, BDMA_SRC_DW_MASK);
    BDMA_WRITE_MASK(REG_BDMA_SEL, BDMA_DST_MIU_IMI_CH0, BDMA_DST_SEL_MASK);
    BDMA_WRITE_MASK(REG_BDMA_SEL, BDMA_DATA_DST_WIDTH_16BYTE, BDMA_DST_DW_MASK);
    BDMA_WRITE_MASK(REG_BDMA_CTRL, 0x0002, BDMA_INT_EN_MASK);
    BDMA_WRITE_MASK(REG_BDMA_CTRL, 0x0000, BDMA_MIU_REP_MASK);
    BDMA_WRITE_MASK(REG_BDMA_FUNC_EN, 0x0000, BDMA_OFFEST_EN_MASK);
    BDMA_WRITE(REG_BDMA_SRC_ADDR_L, 0x0000);
    BDMA_WRITE(REG_BDMA_SRC_ADDR_H, 0x0000);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
    BDMA_WRITE(REG_BDMA_SRC_ADDR_MSB, 0x0000);
#endif
    BDMA_WRITE(REG_BDMA_DST_ADDR_L, 0x0000);
    BDMA_WRITE(REG_BDMA_DST_ADDR_H, 0x0000);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
    BDMA_WRITE(REG_BDMA_DST_ADDR_MSB, 0x0000);
#endif
    BDMA_WRITE(REG_BDMA_SIZE_L, 0x0000);
    BDMA_WRITE(REG_BDMA_SIZE_H, 0x0000);

    XOR_WRITE_MASK(REG_DMA_XOR_CONF, XOR_MODE_SEL_XOR, XOR_SEL_MASK);
    XOR_WRITE_MASK(REG_DMA_XOR_CONF, (ch->sstar_dmadev->chn_mode << 4), XOR_CHN_MODE_MASK);
    XOR_WRITE_MASK(REG_XOR_WAIT, 0x0000, XOR_WAIT_MASK);
}

void sstar_dma_done(struct sstar_chan *ch)
{
    BDMA_WRITE_MASK(REG_BDMA_STATE, 0x001C, BDMA_BUSY_MASK | BDMA_INT_MASK | BDMA_DONE_MASK);
    XOR_WRITE_MASK(REG_DMA_XOR_CONF, 0, XOR_MODE_MASK);
}
void sstar_dma_trigger(struct sstar_chan *ch)
{
    dma_info("%s>>>\n", __func__);
    BDMA_WRITE_MASK(REG_BDMA_CFG, 0x0001, BDMA_TRIG_MASK);
}
void sstar_do_dma_memcpy(struct sstar_chan *ch)
{
    struct sstar_async_desc *async_desc = ch->async_desc;
    dma_info("%s>>>\n", __func__);

    BDMA_WRITE(REG_BDMA_SRC_ADDR_L, (async_desc->src_buf_addr[0] & 0xFFFF));
    BDMA_WRITE(REG_BDMA_SRC_ADDR_H, ((async_desc->src_buf_addr[0] >> 16) & 0xFFFF));
#ifdef CONFIG_PHYS_ADDR_T_64BIT
    BDMA_WRITE(REG_BDMA_SRC_ADDR_MSB, ((async_desc->src_buf_addr[0] >> 32) & 0x000F));
#endif
    BDMA_WRITE(REG_BDMA_DST_ADDR_L, (async_desc->dst_buf_addr[0] & 0xFFFF));
    BDMA_WRITE(REG_BDMA_DST_ADDR_H, ((async_desc->dst_buf_addr[0] >> 16) & 0xFFFF));
#ifdef CONFIG_PHYS_ADDR_T_64BIT
    BDMA_WRITE(REG_BDMA_DST_ADDR_MSB, ((async_desc->dst_buf_addr[0] >> 32) & 0x000F));
#endif
    BDMA_WRITE(REG_BDMA_SIZE_L, (async_desc->buf_len & 0xFFFF));
    BDMA_WRITE(REG_BDMA_SIZE_H, ((async_desc->buf_len >> 16) & 0xFFFF));
}

void sstar_do_dma_xor(struct sstar_chan *ch, int group_num)
{
    int                      i, max_xor = ch->sstar_dmadev->dma_dev.max_xor;
    struct sstar_async_desc *async_desc = ch->async_desc;
    dma_info("%s>>>\n", __func__);
    XOR_WRITE_MASK(REG_DMA_XOR_CONF, 0x0001, XOR_MODE_MASK);
    if (group_num % 2)
    {
        XOR_WRITE_MASK((REG_XOR_SRC_NUM + (group_num / 2)), ((async_desc->src_num - 1) << 8), XOR_SECOND_NUM_MASK);
    }
    else
    {
        XOR_WRITE_MASK((REG_XOR_SRC_NUM + (group_num / 2)), (async_desc->src_num - 1), XOR_FIRST_NUM_MASK);
    }
    XOR_WRITE_MASK(REG_DMA_XOR_CONF, (group_num << 13), XOR_SRC_GROUP_MASK);
    for (i = 0; i < async_desc->src_num; i++)
    {
        XOR_WRITE((REG_XOR_SRC_ADDR0_L + ((i * 0x2) + (group_num * max_xor * 0x2))),
                  ((async_desc->src_buf_addr[i] >> 4) & 0xFFFF));
        XOR_WRITE((REG_XOR_SRC_ADDR0_H + ((i * 0x2) + (group_num * max_xor * 0x2))),
                  ((async_desc->src_buf_addr[i] >> 20) & 0xFFFF));
    }
    XOR_WRITE((REG_XOR_DST_ADDR0_L + (group_num * 0x2)), ((async_desc->dst_buf_addr[0] >> 4) & 0xFFFF));
    XOR_WRITE((REG_XOR_DST_ADDR0_H + (group_num * 0x2)), ((async_desc->dst_buf_addr[0] >> 20) & 0xFFFF));
    BDMA_WRITE(REG_BDMA_SIZE_L, (async_desc->buf_len & 0xFFFF));
    BDMA_WRITE(REG_BDMA_SIZE_H, ((async_desc->buf_len >> 16) & 0xFFFF));
}

static int sstar_dma_handle_desc(void *pdata)
{
    struct sstar_chan *      ch = (struct sstar_chan *)(pdata);
    struct virt_dma_desc *   vd, *vd_tmp1, *vd_tmp2;
    struct sstar_async_desc *async_desc_tmp;
    unsigned long *          virtaddr;
    unsigned long            dst_buf_ad;
    int                      i, timeout, max_xor = ch->sstar_dmadev->dma_dev.max_xor;
    unsigned long            flags;
    sstar_dma_init(ch);
    while (!kthread_should_stop())
    {
        spin_lock_irqsave(&ch->vc.lock, flags);
        vd = vchan_next_desc(&ch->vc);
        spin_unlock_irqrestore(&ch->vc.lock, flags);
        set_current_state(TASK_INTERRUPTIBLE);
        if (!vd)
        {
            ch->async_desc = NULL;
            dma_info("no more vdesc\n");
            schedule();
            continue;
        }
        set_current_state(TASK_RUNNING);
        ch->async_desc = to_sstar_dma_desc(&vd->tx);
        mutex_lock(&ch->ch_lock);
        for (i = 0; i < (32 / max_xor); i++)
        {
            if (ch->async_desc->dma_type == DMA_XOR)
            {
                spin_lock_irqsave(&ch->vc.lock, flags);
                list_move_tail(&vd->node, &ch->vdescs);
                spin_unlock_irqrestore(&ch->vc.lock, flags);
                sstar_do_dma_xor(ch, i);
            }
            if ((ch->async_desc->dma_type == DMA_MEMCPY))
            {
                if (i == 0)
                {
                    spin_lock_irqsave(&ch->vc.lock, flags);
                    list_move_tail(&vd->node, &ch->vdescs);
                    spin_unlock_irqrestore(&ch->vc.lock, flags);
                    sstar_do_dma_memcpy(ch);
                    break;
                }
                else
                {
                    break;
                }
            }
            spin_lock_irqsave(&ch->vc.lock, flags);
            vd = vchan_next_desc(&ch->vc);
            spin_unlock_irqrestore(&ch->vc.lock, flags);
            if (!vd)
            {
                ch->async_desc = NULL;
                dma_info("no more vdesc\n");
                break;
            }
            ch->async_desc = to_sstar_dma_desc(&vd->tx);
        }

        sstar_dma_trigger(ch);
        timeout = wait_for_completion_timeout(&ch->done, msecs_to_jiffies(SSTAR_DMA_TIMEOUT));
        if (!timeout)
        {
            dma_err("waitting for dma done time out, func may get wrong data!!!\n");
        }

        reinit_completion(&ch->done);
        spin_lock_irqsave(&ch->vc.lock, flags);
        list_for_each_entry_safe(vd_tmp1, vd_tmp2, &ch->vdescs, node)
        {
            async_desc_tmp = to_sstar_dma_desc(&vd_tmp1->tx);
            dst_buf_ad     = Chip_MIU_to_Phys((unsigned long long)async_desc_tmp->dst_buf_addr[0]);
            virtaddr       = phys_to_virt(dst_buf_ad);
            Chip_Inv_Cache_Range((void *)virtaddr, (unsigned long)async_desc_tmp->buf_len);
            list_del(&vd_tmp1->node);
            vchan_cookie_complete(vd_tmp1);
            dma_descriptor_unmap(&vd_tmp1->tx);
        }
        spin_unlock_irqrestore(&ch->vc.lock, flags);
        mutex_unlock(&ch->ch_lock);
    }
    return 0;
}

static void sstar_dma_desc_free(struct virt_dma_desc *vd)
{
    kfree(container_of(vd, struct sstar_async_desc, vd));
}

static int sstar_alloc_chan_resources(struct dma_chan *chan)
{
    struct sstar_chan *ch = to_sstar_dma_chan(chan);
    dma_info("%s>>>\n", __func__);
    ch->allocatd = 1;
    return 0;
}

static void sstar_free_chan_resources(struct dma_chan *chan)
{
    struct sstar_chan *ch = to_sstar_dma_chan(chan);
    dma_info("%s>>>\n", __func__);
    vchan_free_chan_resources(&ch->vc);
    ch->allocatd = 0;
}

irqreturn_t dma_handler(int irq, void *priv)
{
    struct sstar_dmadev *sstar_dmadev = (struct sstar_dmadev *)priv;
    struct sstar_chan *  ch           = &sstar_dmadev->sstar_chans[0];
    dma_info("%s>>>\n", __func__);
    sstar_dma_done(ch);
    complete(&ch->done);
    return IRQ_HANDLED;
}

static int sstar_device_config(struct dma_chan *chan, struct dma_slave_config *cfg)
{
    struct sstar_chan *ch = to_sstar_dma_chan(chan);
    ch->cfg               = *cfg;
    dma_info("%s>>>\n", __func__);
    switch (ch->cfg.direction)
    {
        default:
            break;
    }
    return 0;
}

static enum dma_status sstar_tx_status(struct dma_chan *chan, dma_cookie_t cookie, struct dma_tx_state *txstate)
{
    dma_info("%s>>>\n", __func__);
    return dma_cookie_status(chan, cookie, txstate);
}

static void sstar_issue_pending(struct dma_chan *chan)
{
    unsigned long      flags;
    struct sstar_chan *ch = to_sstar_dma_chan(chan);
    dma_info("%s>>>\n", __func__);
    spin_lock_irqsave(&ch->vc.lock, flags);
    if (vchan_issue_pending(&ch->vc))
        wake_up_process(ch->dma_thread_st);
    spin_unlock_irqrestore(&ch->vc.lock, flags);
}

static struct dma_async_tx_descriptor *sstar_prep_dma_xor(struct dma_chan *chan, dma_addr_t dest, dma_addr_t *src,
                                                          unsigned int src_cnt, size_t len, unsigned long flags)
{
    int                      i;
    struct sstar_chan *      ch = to_sstar_dma_chan(chan);
    struct sstar_async_desc *async_desc;
    unsigned long *          virtaddr;
    dma_info("%s>>>\n", __func__);
    async_desc           = kzalloc(sizeof(struct sstar_async_desc), GFP_KERNEL);
    async_desc->dma_type = DMA_XOR;
    async_desc->src_num  = src_cnt;
    async_desc->dst_num  = 1;
    switch (len)
    {
        case 512:
        case 1024:
        case 2048:
        case 4096:
            async_desc->buf_len = len;
            break;
        default:
            kfree(async_desc);
            dma_info("invalid xor length return\n");
            return ERR_PTR(-EINVAL);
    }
    dma_info("xor src num is %lu\n", (unsigned long)src_cnt);
    for (i = 0; i < src_cnt; i++)
    {
        virtaddr = phys_to_virt((unsigned long)src[i]);
        Chip_Flush_Cache_Range((void *)virtaddr, len);
        async_desc->src_buf_addr[i] = Chip_Phys_to_MIU(src[i]);
        dma_info("src_buff_miu[%d] is 0x%llx\n", i, async_desc->src_buf_addr[i]);
    }
    for (i = 0; i < SSTAR_DMA_BUF_GRP_NUM; i++)
        async_desc->dst_buf_addr[i] = Chip_Phys_to_MIU(dest + i * len);
    dma_info("dst_buff_miu is 0x%llx\n", async_desc->dst_buf_addr[0]);

    return vchan_tx_prep(&ch->vc, &async_desc->vd, flags);
}
struct dma_async_tx_descriptor *sstar_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dst, dma_addr_t src, size_t len,
                                                      unsigned long flags)
{
    struct sstar_chan *      ch = to_sstar_dma_chan(chan);
    struct sstar_async_desc *async_desc;
    dma_info("%s>>>\n", __func__);
    async_desc                  = kzalloc(sizeof(struct sstar_async_desc), GFP_KERNEL);
    async_desc->dma_type        = DMA_MEMCPY;
    async_desc->src_buf_addr[0] = Chip_Phys_to_MIU(src);
    async_desc->dst_buf_addr[0] = Chip_Phys_to_MIU(dst);
    async_desc->buf_len         = len;
    return vchan_tx_prep(&ch->vc, &async_desc->vd, flags);
}

static int sstar_dma_probe(struct platform_device *pdev)
{
    int                  i, irq, max_xor;
    int                  status = 0;
    struct device *      dev    = &pdev->dev;
    struct sstar_dmadev *sstar_dmadev;
    struct dma_device *  dma_dev;
    struct sstar_chan *  ch;
    struct resource *    dma_resource;
    phys_addr_t          xor_base;
    phys_addr_t          bdma_base;
    struct clk *         clk;
    dma_info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<DMA PROBE>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    sstar_dmadev = kzalloc(sizeof(struct sstar_dmadev), GFP_KERNEL);
    if (!sstar_dmadev)
    {
        status = -ENOMEM;
        goto OUT;
    }
    dma_dev = &sstar_dmadev->dma_dev;
    clk     = of_clk_get(pdev->dev.of_node, 0);
    if (IS_ERR(clk))
    {
        dma_err("get clk fail\n");
        status = -ENOENT;
        goto OUT;
    }
    clk_prepare_enable(clk);
    clk_put(clk);
    sstar_dmadev->clk = clk;
    irq               = irq_of_parse_and_map(pdev->dev.of_node, 0);
    if (irq == 0)
    {
        dma_err("get irq fail\n");
        status = -ENOENT;
        goto CLK_ERR;
    }
    if (0 != request_irq(irq, dma_handler, 0, "xor_dma", (void *)sstar_dmadev))
    {
        dma_err("request irq fail\n");
        status = -EINTR;
        goto CLK_ERR;
    }
    dma_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!dma_resource)
    {
        dma_err("get xor_base fail\n");
        status = -ENOENT;
        goto IRQ_ERR;
    }
    xor_base     = IO_ADDRESS(dma_resource->start);
    dma_resource = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!dma_resource)
    {
        dma_err("get xor_base fail\n");
        status = -ENOENT;
        goto IRQ_ERR;
    }
    bdma_base = IO_ADDRESS(dma_resource->start);
    status    = of_property_read_u32(pdev->dev.of_node, "max_xor", &max_xor);
    if (status)
    {
        max_xor = 32;
        dma_warn("read max_xor from dtbb failed use default value 32.\n");
    }
    switch (max_xor)
    {
        case 32:
            sstar_dmadev->chn_mode = Chn_mode_32;
            break;
        case 16:
            sstar_dmadev->chn_mode = Chn_mode_16;
            break;
        case 8:
            sstar_dmadev->chn_mode = Chn_mode_8;
            break;
        case 4:
            sstar_dmadev->chn_mode = Chn_mode_4;
            break;
        default:
            sstar_dmadev->chn_mode = Chn_mode_32;
    }
    dma_cap_zero(dma_dev->cap_mask);
    dma_cap_set(DMA_XOR, dma_dev->cap_mask);
    dma_cap_set(DMA_MEMCPY, dma_dev->cap_mask);

    sstar_dmadev->clk = clk;
    sstar_dmadev->irq = irq;

    dma_dev->device_alloc_chan_resources = sstar_alloc_chan_resources;
    dma_dev->device_free_chan_resources  = sstar_free_chan_resources;
    dma_dev->device_config               = sstar_device_config;
    dma_dev->device_issue_pending        = sstar_issue_pending;
    dma_dev->device_tx_status            = sstar_tx_status;
    dma_dev->max_xor                     = max_xor;
    dma_dev->device_prep_dma_xor         = sstar_prep_dma_xor;
    dma_dev->device_prep_dma_memcpy      = sstar_prep_dma_memcpy;
    dma_dev->dev                         = dev;
    dma_dev->xor_align                   = DMAENGINE_ALIGN_16_BYTES;
    INIT_LIST_HEAD(&dma_dev->channels);
    ch = sstar_dmadev->sstar_chans;
    for (i = 0; i < SSTAR_DMA_MAX_CHANNEL; i++, ch++)
    {
        ch->allocatd     = 0;
        ch->ch_num       = i;
        ch->xor_base     = xor_base;
        ch->bdma_base    = bdma_base;
        ch->vc.desc_free = sstar_dma_desc_free;
        ch->sstar_dmadev = sstar_dmadev;
        init_completion(&ch->done);
        INIT_LIST_HEAD(&ch->vdescs);
        vchan_init(&ch->vc, &sstar_dmadev->dma_dev);
        ch->dma_thread_st = kthread_create(sstar_dma_handle_desc, ch, "dma_thread");
    }
    if (!dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64)))
    {
        dma_info("DMA use 64bit addr mask\n");
    }
    else if (!dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32)))
    {
        dma_info("DMA use 32bit addr mask\n");
    }
    platform_set_drvdata(pdev, sstar_dmadev);
    device_create_file(&pdev->dev, &dev_attr_xor_info);
    status = dma_async_device_register(&sstar_dmadev->dma_dev);
    if (status)
    {
        dev_err(dev, "dma_async_device_register failed\n");
        goto ERR_OUT;
    }
    dma_info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<PROBE END>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    return 0;

ERR_OUT:
    dma_async_device_unregister(&sstar_dmadev->dma_dev);
IRQ_ERR:
    free_irq(irq, (void *)sstar_dmadev);
CLK_ERR:
    clk_disable_unprepare(clk);
OUT:
    kfree(sstar_dmadev);
    return status;
}

static int sstar_dma_remove(struct platform_device *pdev)
{
    struct sstar_dmadev *sstar_dmadev = platform_get_drvdata(pdev);
    dma_info("%s>>>\n", __func__);
    clk_disable_unprepare(sstar_dmadev->clk);
    free_irq(sstar_dmadev->irq, (void *)sstar_dmadev);
    dma_async_device_unregister(&sstar_dmadev->dma_dev);
    return 0;
}

#define sstar_dma_suspend NULL
#define sstar_dma_resume  NULL

static const struct of_device_id sstar_dma_match[] = {{
                                                          .compatible = "sstar,xordma",
                                                      },
                                                      {}};
MODULE_DEVICE_TABLE(of, sstar_dma_match);

static const struct dev_pm_ops sstar_dma_pm_ops = {
    .suspend = sstar_dma_suspend,
    .resume  = sstar_dma_resume,
};

static struct platform_driver sstar_dma_driver = {
    .driver =
        {
            .name           = "xordma",
            .owner          = THIS_MODULE,
            .pm             = &sstar_dma_pm_ops,
            .of_match_table = sstar_dma_match,
        },
    .probe  = sstar_dma_probe,
    .remove = sstar_dma_remove,
};

module_platform_driver(sstar_dma_driver);
MODULE_DESCRIPTION("SSTAR DMA engine driver");
MODULE_LICENSE("GPL v2");
