/*
 * clock driver for hisilicon hi3519 or hi3559 soc
 *
 * Copyright (c) 2015 HiSilicon Technologies Co., Ltd.
 *
 * Authors: yanghongwei@hisilicon.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/mm_types.h>
#include <linux/mmu_context.h>
#include <asm/tlbflush.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include "hi_vdmav100.h"
#include "hi_vdma.h"

#define user_addr(ptr)   (((unsigned int)ptr < TASK_SIZE) \
            && ((unsigned int)ptr > 0))



int vdma_flag = 0;
EXPORT_SYMBOL(vdma_flag);

void __iomem *hi_reg_vdma_base_va;

spinlock_t my_lock;
spinlock_t reg_lock;

unsigned int irq_flag;

unsigned int wake_channel_flag[DMAC_MAX_CHANNELS];

wait_queue_head_t dmac_wait_queue[DMAC_MAX_CHANNELS];

int hi_g_channel_status[DMAC_MAX_CHANNELS];


/*
 *  dmac interrupt handle function
 */
irqreturn_t vdma_isr(int irq, void *dev_id)
{

    unsigned int dma_intr_status, channel_intr_status;
    unsigned int i;

    dmac_readw(hi_reg_vdma_base_va + DMAC_INT_STATUS, dma_intr_status);

    /* decide which channel has trigger the interrupt */
    for (i = 0; i < DMAC_MAX_CHANNELS; i++) {
        if (((dma_intr_status >> i) & 0x1) == 1) {
            dmac_readw(hi_reg_vdma_base_va
                       + DMAC_CxINTR_RAW(i),
                       channel_intr_status);

            /* clear the channel error interrupt */
            dmac_writew(hi_reg_vdma_base_va
                        + DMAC_CxINTR_RAW(i),
                        channel_intr_status);

            if ((channel_intr_status & CX_INT_STAT)
                    == CX_INT_STAT) {
                /* transfer finish interrupt */
                if ((channel_intr_status & CX_INT_TC_RAW)
                        == CX_INT_TC_RAW) {
                    wake_channel_flag[i] = DMA_TRANS_OK;

                    /* save the current channel transfer *
                     * status to hi_g_channel_status[i] */
                    hi_g_channel_status[i] =
                        DMAC_CHN_SUCCESS;

                    wake_up(&dmac_wait_queue[i]);
                    goto exit;
                }

                wake_channel_flag[i] = DMA_TRANS_FAULT;
                pr_err("%d channel!,intr_raw=%x\n",
                       i, channel_intr_status);
                hi_g_channel_status[i] =
                    -DMAC_CHN_CONFIG_ERROR;

                wake_up(&dmac_wait_queue[i]);
                goto exit;
            }
        }
    }

exit:
    return IRQ_HANDLED;
}


/*
 *  allocate channel.
 */
int hi_vdma_channel_allocate(void *pisr)
{
    unsigned int  i, channelinfo, tmp, channel_intr;
    unsigned long flags;

    spin_lock_irqsave(&my_lock, flags);

    dmac_readw(hi_reg_vdma_base_va + DMAC_CHANNEL_STATUS, channelinfo);

    for (i = 0; i < CHANNEL_NUM; i++) {
        if (hi_g_channel_status[i] == DMAC_CHN_VACANCY) {
            dmac_readw(hi_reg_vdma_base_va + DMAC_CxINTR_RAW(i),
                       channel_intr);
            tmp = channelinfo >> i;
            /* check the vdma channel data transfer is finished ? */
            if ((0x00 == (tmp & 0x01)) & (channel_intr == 0x00)) {
                hi_g_channel_status[i] = DMAC_CHN_ALLOCAT;
                spin_unlock_irqrestore(&my_lock, flags);
                return i;/* return channel number */
            }
        }
    }

    spin_unlock_irqrestore(&my_lock, flags);
    return DMAC_CHANNEL_INVALID;
}


/*
 *  free channel
 */
int hi_vdma_channel_free(unsigned int channel)
{
    hi_g_channel_status[channel] = DMAC_CHN_VACANCY;

    return 0;
}


/*
 *  channel enable
 *  start a vdma transfer immediately
 */
int hi_vdma_channelstart(unsigned int channel,
                         unsigned int *src, unsigned int *dest)
{
    struct mm_struct *mm;
    unsigned int reg[DMAC_MAX_CHANNELS];

    if (channel >= DMAC_MAX_CHANNELS) {
        pr_err("channel number is out of scope(%d).\n",
               DMAC_MAX_CHANNELS);
        return -EINVAL;
    }

    hi_g_channel_status[channel] = DMAC_NOT_FINISHED;

    mm = current->mm;
    if (!mm) {
        mm = &init_mm;
    }

    /* set ttbr */
    /* get TTBR from the page */
    reg[channel] = __pa(mm->pgd);

    /* only [31:10] is the ttbr */
    reg[channel] &= 0xfffffc00;

    /* set the RGN,AFE,AFFD,TRE */
    reg[channel] |= TTB_RGN | AFE | TRE;

    if (user_addr(dest)) {
        reg[channel] &= ~DEST_IS_KERNEL;
    } else {
        reg[channel] |= DEST_IS_KERNEL;
    }

    if (user_addr(src)) {
        reg[channel] &= ~SRC_IS_KERNEL;
    } else {
        reg[channel] |= SRC_IS_KERNEL;
    }

    if (in_atomic() || in_interrupt()) {
        /* disable the channel interrupt */
        reg[channel] &= ~DMAC_INTR_ENABLE;

    } else {
        /* enable the channel interrupt */
        reg[channel] |= DMAC_INTR_ENABLE;
    }

    reg[channel] |= DMAC_CHANNEL_ENABLE;

    /* set the TTBR register */
    dmac_writew(hi_reg_vdma_base_va + DMAC_CxTTBR(channel),
                reg[channel]);
    return 0;
}

/*
 *  Apply VDMA interrupt resource
 *  init channel state
 */
int hi_vdma_driver_init(struct hivdmac_host *dma)
{
    unsigned int i, tmp_reg = 0;


    dmac_readw(hi_reg_vdma_base_va + DMAC_GLOBLE_CTRL, tmp_reg);
    tmp_reg |= AUTO_CLK_GT_EN;
    dmac_writew(hi_reg_vdma_base_va + DMAC_GLOBLE_CTRL, tmp_reg);

    /* set rd dust address is ram 0 */
    dmac_writew(hi_reg_vdma_base_va + DMAC_RD_DUSTB_ADDR, 0x04c11000);

    /* set wr dust address is ram 0x1000 */
    dmac_writew(hi_reg_vdma_base_va + DMAC_WR_DUSTB_ADDR, 0x04c11000);

    /* set prrr register */
    dmac_writew(hi_reg_vdma_base_va + DMAC_MMU_PRRR, PRRR);
    /* set nmrr register */
    dmac_writew(hi_reg_vdma_base_va + DMAC_MMU_NMRR, NMRR);

    /*if (request_irq(dma->irq, &vdma_isr,
                0, "Hisilicon Vdma", NULL)) {
        pr_err("DMA Irq %d request failed\n", dma->irq);
        return -1;
    }*/

    /* config global reg for VDMA */
    tmp_reg |= EVENT_BROADCAST_EN | WR_CMD_NUM_PER_ARB |
               RD_CMD_NUM_PER_ARB | WR_OTD_NUM | RD_OTD_NUM | WFE_EN;
    dmac_writew(hi_reg_vdma_base_va + DMAC_GLOBLE_CTRL, tmp_reg);

    for (i = 0; i < CHANNEL_NUM; i++) {
        hi_g_channel_status[i] = DMAC_CHN_VACANCY;
        init_waitqueue_head(&dmac_wait_queue[i]);

    }

    spin_lock_init(&my_lock);
    spin_lock_init(&reg_lock);

    return 0;
}


/*
 *  wait for transfer end
 */
int hi_vdma_wait(unsigned int channel)
{
    unsigned long data_jiffies_timeout = jiffies + DMA_TIMEOUT_HZ;
    unsigned int channel_intr_raw;

    while (1) {
        /*wfe();*/
        /* read the status of current interrupt */
        dmac_readw(hi_reg_vdma_base_va + DMAC_CxINTR_RAW(channel),
                   channel_intr_raw);

        /* clear the interrupt */
        dmac_writew(hi_reg_vdma_base_va
                    + DMAC_CxINTR_RAW(channel),
                    channel_intr_raw);

        /*save the current channel transfer status to*
         *hi_g_channel_status[i]*/
        if ((channel_intr_raw & CX_INT_STAT) == CX_INT_STAT) {
            /* transfer finish interrupt */
            if ((channel_intr_raw & CX_INT_TC_RAW) ==
                    CX_INT_TC_RAW) {
                hi_g_channel_status[channel] = DMAC_CHN_SUCCESS;
                return DMAC_CHN_SUCCESS;
            }

            /* transfer abort interrupt */
            pr_debug("data transfer error in VDMA %x channel!",
                     channel);
            pr_debug("intr_raw=%x\n", channel_intr_raw);
            hi_g_channel_status[channel] =
                -DMAC_CHN_CONFIG_ERROR;
            return -DMAC_CHN_CONFIG_ERROR;
        }

        if (!time_before(jiffies, data_jiffies_timeout)) { /* timeout */
            pr_err("wait interrupt timeout, channel=%d, func:%s, line:%d\n",
                   channel, __func__, __LINE__);
            return -1;
        }
    }


    return -1;
}


/*
 *  execute memory to memory vdma transfer
 */
static int hi_vdma_m2m_transfer(unsigned int *psource,
                                unsigned int *pdest,
                                unsigned int uwtransfersize)
{
    unsigned int ulchnn;
    int ret = 0;

    ulchnn = hi_vdma_channel_allocate(NULL);

    if (ulchnn == DMAC_CHANNEL_INVALID) {
        pr_err("DMAC_CHANNEL_INVALID.\n");

        return -1;
    }

    wake_channel_flag[ulchnn] = 0;


    dmac_writew(hi_reg_vdma_base_va + DMAC_CxLENGTH(ulchnn),
                uwtransfersize);
    dmac_writew(hi_reg_vdma_base_va + DMAC_CxSRCADDR(ulchnn),
                (unsigned int)psource);
    dmac_writew(hi_reg_vdma_base_va + DMAC_CxDESTADDR(ulchnn),
                (unsigned int)pdest);

    if (hi_vdma_channelstart(ulchnn, psource, pdest) != 0) {
        ret = -1;
        goto exit;
    }

    /*if (in_atomic() || in_interrupt()) {*/

    if (hi_vdma_wait(ulchnn) != DMAC_CHN_SUCCESS) {

        ret = -1;
        goto exit;
    }


    /*} else {
        ret = wait_event_timeout(dmac_wait_queue[ulchnn],
                (wake_channel_flag[ulchnn] != 0),
                DMA_TIMEOUT_HZ);
        if (ret == 0) {
            ret = -1;
            goto exit;
        }

        if (wake_channel_flag[ulchnn] == DMA_TRANS_FAULT) {

            ret = -1;
            goto exit;
        }
    }*/

exit:
    hi_vdma_channel_free(ulchnn);

    return ret;
}
EXPORT_SYMBOL(hi_vdma_m2m_transfer);

int hi_memcpy(void *dst, const void *src, size_t count)
{
    int ret;

    ret = hi_vdma_m2m_transfer((unsigned int *)src, dst, count);

    if (ret < 0) {
        return ret;
    } else {
        return 0;
    }
}
EXPORT_SYMBOL(hi_memcpy);

static int hivdmac_probe(struct platform_device *platdev)
{
    unsigned int i;
    struct hivdmac_host *dma;
    struct resource *res;
    int ret;
    dma = devm_kzalloc(&platdev->dev, sizeof(*dma), GFP_KERNEL);
    if (!dma) {
        return -ENOMEM;
    }

    res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&platdev->dev, "no mmio resource\n");
        return -ENODEV;
    }

    dma->regbase = devm_ioremap_resource(&platdev->dev, res);
    if (IS_ERR(dma->regbase)) {
        return PTR_ERR(dma->regbase);
    }

    dma->clk = devm_clk_get(&platdev->dev, NULL);
    if (IS_ERR(dma->clk)) {
        return PTR_ERR(dma->clk);
    }

    clk_prepare_enable(dma->clk);

    dma->rstc = devm_reset_control_get(&platdev->dev, "dma-reset");
    if (IS_ERR(dma->rstc)) {
        return PTR_ERR(dma->rstc);
    }

    dma->irq = platform_get_irq(platdev, 0);
    if (unlikely(dma->irq < 0)) {
        return -ENODEV;
    }
    hi_reg_vdma_base_va = dma->regbase;
    pr_debug("vdma reg base is %p\n", hi_reg_vdma_base_va);
    dma->dev = &platdev->dev;

    ret = hi_vdma_driver_init(dma);
    if (ret) {
        return -ENODEV;
    }

    platform_set_drvdata(platdev, dma);

    for (i = 0; i < CONFIG_HI_VDMA_CHN_NUM; i++) {
        hi_g_channel_status[i] = DMAC_CHN_VACANCY;
    }

    vdma_flag = 1;
    printk("hivdmav100 driver inited.\n");
    return ret;
}

static int hivdmac_remove(struct platform_device *platdev)
{
    int i;
    struct hivdmac_host *dma = platform_get_drvdata(platdev);

    clk_disable_unprepare(dma->clk);

    for (i = 0; i < CONFIG_HI_VDMA_CHN_NUM; i++) {
        hi_g_channel_status[i] = DMAC_CHN_VACANCY;
    }

    vdma_flag = 0;
    printk("hivdmav100 driver deinited.\n");

    return 0;
}

static int hivdmac_suspend(struct platform_device *platdev,
                           pm_message_t state)
{
    int i;
    struct hivdmac_host *dma = platform_get_drvdata(platdev);

    clk_prepare_enable(dma->clk);

    for (i = 0; i < CONFIG_HI_VDMA_CHN_NUM; i++) {
        hi_g_channel_status[i] = DMAC_CHN_VACANCY;
    }

    clk_disable_unprepare(dma->clk);

    vdma_flag = 0;

    return 0;
}

static int hivdmac_resume(struct platform_device *platdev)
{
    int i;
    struct hivdmac_host *dma = platform_get_drvdata(platdev);

    hi_vdma_driver_init(dma);

    for (i = 0; i < CONFIG_HI_VDMA_CHN_NUM; i++) {
        hi_g_channel_status[i] = DMAC_CHN_VACANCY;
    }

    vdma_flag = 1;

    return 0;
}

static const struct of_device_id hisi_vdmac_dt_ids[] = {
    { .compatible = "hisilicon,hisi-vdmac"},
    { },
};
MODULE_DEVICE_TABLE(of, hisi_vdmac_dt_ids);

static struct platform_driver hisi_vdmac_driver = {
    .driver = {
        .name   = "hisi-vdmac",
        .of_match_table = hisi_vdmac_dt_ids,
    },
    .probe      = hivdmac_probe,
    .remove     = hivdmac_remove,
    .suspend    = hivdmac_suspend,
    .resume     = hivdmac_resume,
};

module_platform_driver(hisi_vdmac_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("BVT_OSDRV");
