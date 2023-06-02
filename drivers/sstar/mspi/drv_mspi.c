/*
* drv_mspi.c- Sigmastar
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
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/clk-provider.h>
#include "mdrv_gpio.h"

#include <hal_mspi.h>
#include <hal_mspireg.h>
#include <ms_platform.h>
#include <drv_camclk_Api.h>

#define mspi_dbg                    0
#if mspi_dbg == 1
    #define mspi_dbgmsg(args...)    printk("[MSPI] : " args)
#else
    #define mspi_dbgmsg(args...)    do{}while(0)
#endif
#define mspi_errmsg(fmt, ...)       printk(KERN_ERR "[MSPI] error : " fmt, ##__VA_ARGS__)
#define mspi_warnmsg(fmt, ...)      printk(KERN_ERR "[MSPI] warning : " fmt, ##__VA_ARGS__)

#define SSTAR_SPI_MODE_BITS         (SPI_CPOL | SPI_CPHA)

struct mspi_clk
{
    u8 clk_src;
    u8 clk_div;
    u32 clk_rate;
};

struct mspi_clk_tbl
{
    u32 *clk_src_tbl;
    u32 clk_src_tbl_sz;
    u32 *clk_div_tbl;
    u32 clk_div_tbl_sz;
    u32 clk_cfg_tbl_sz;
    struct mspi_clk *clk_cfg_tbl;
};

struct sstar_mspi
{
    int irq_num;
    int len;
    const u8 *tx_buf;
    u8       *rx_buf;

    u16  mode;
    s16  bus_num;
    u32  use_dma;
    u32  max_speed_hz;

    #ifdef CONFIG_CAM_CLK
    void **camclk;
    #endif

    struct mspi_hal hal;
    struct platform_device *pdev;
    struct mspi_clk_tbl mspi_clk_tbl;
};

static const u32 sstar_spi_clk_div_tbl[] = MSPI_CLK_DIV_VAL;

#ifndef CONFIG_SPI_INT_CALL
static irqreturn_t sstar_spi_interrupt(int irq, void *dev_id)
{
    struct spi_master *sstar_master = dev_id;
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(sstar_master);
    int uDoneFlag = 0;

    uDoneFlag = HAL_MSPI_CheckDone(&sstar_spimst->hal);

    if(uDoneFlag == 1)
    {
        HAL_MSPI_ClearDone(&sstar_spimst->hal);
        mspi_dbgmsg("<<<<<<<<<<<<< SPI_%d Done >>>>>>>>>>>>>\n", sstar_spimst->bus_num);
        complete(&sstar_spimst->hal.done);
    }
    return IRQ_HANDLED;
}
#endif

static void sstar_spi_clk_tbl_init(struct mspi_clk_tbl *clk_tbl)
{
    u8 i = 0;
    u8 j = 0;
    u32 clk =0;
    struct mspi_clk temp;

    memset(&temp,0,sizeof(struct mspi_clk));
    memset(clk_tbl->clk_cfg_tbl,0,sizeof(struct mspi_clk)*clk_tbl->clk_cfg_tbl_sz);

    for(i = 0; i < clk_tbl->clk_src_tbl_sz; i++)
    {
        for(j = 0; j < clk_tbl->clk_div_tbl_sz; j++)
        {
            clk = clk_tbl->clk_src_tbl[i];
            clk_tbl->clk_cfg_tbl[j+clk_tbl->clk_div_tbl_sz*i].clk_src = i;
            clk_tbl->clk_cfg_tbl[j+clk_tbl->clk_div_tbl_sz*i].clk_div = j ;
            clk_tbl->clk_cfg_tbl[j+clk_tbl->clk_div_tbl_sz*i].clk_rate = clk/clk_tbl->clk_div_tbl[j];
        }
    }

    for(i = 0; i < clk_tbl->clk_cfg_tbl_sz; i++)
    {
        for(j = i ; j < clk_tbl->clk_cfg_tbl_sz; j++)
        {
            if(clk_tbl->clk_cfg_tbl[i].clk_rate > clk_tbl->clk_cfg_tbl[j].clk_rate)
            {
                memcpy(&temp,&clk_tbl->clk_cfg_tbl[i],sizeof(struct mspi_clk));

                memcpy(&clk_tbl->clk_cfg_tbl[i],&clk_tbl->clk_cfg_tbl[j],sizeof(struct mspi_clk));

                memcpy(&clk_tbl->clk_cfg_tbl[j],&temp,sizeof(struct mspi_clk));
            }
        }
    }

    for (i = 0; i < clk_tbl->clk_cfg_tbl_sz; i++)
    {
        mspi_dbgmsg("clk_cfg_tbl[%d].clk_cfg  = %d\n", i, clk_tbl->clk_cfg_tbl[i].clk_src);
        mspi_dbgmsg("clk_cfg_tbl[%d].clk_div  = %d\n", i, clk_tbl->clk_cfg_tbl[i].clk_div);
        mspi_dbgmsg("clk_cfg_tbl[%d].clk_rate = %d\n", i, clk_tbl->clk_cfg_tbl[i].clk_rate);
    }

}

static u32 sstar_spi_set_clock(struct sstar_mspi *mspi, u32 clock)
{
    u8 i = 0;
#ifdef CONFIG_CAM_CLK
    u8 cam_on;
    CAMCLK_Set_Attribute stSetCfg;
#else
    struct clk *mspi_clock;
#endif
    struct mspi_clk_tbl *clk_tbl = &mspi->mspi_clk_tbl;

    for(i = 0; i < clk_tbl->clk_cfg_tbl_sz; i++)
    {
        if(clock <= clk_tbl->clk_cfg_tbl[i].clk_rate)
        {
            break;
        }
    }
    if (clk_tbl->clk_cfg_tbl_sz == i)
    {
        i--;
    }
    //match Closer clk
    if ((i) && ((clock - clk_tbl->clk_cfg_tbl[i-1].clk_rate)<(clk_tbl->clk_cfg_tbl[i].clk_rate - clock)))
    {
        i -= 1;
    }

#ifdef CONFIG_CAM_CLK
    if(CamClkGetOnOff(mspi->camclk[0], &cam_on))
    {
        mspi_errmsg("cam clk 0 on get fail\n");
        return 0;
    }
    if(!cam_on)
    {
        if(CamClkSetOnOff(mspi->camclk[0], 1))
        {
            mspi_errmsg("cam clk 0 on set fail\n");
            return 0;
        }
    }
    CAMCLK_SETRATE_ROUNDUP(stSetCfg,clk_tbl->clk_src_tbl[clk_tbl->clk_cfg_tbl[i].clk_src]);
    if(CamClkAttrSet(mspi->camclk[0],&stSetCfg))
    {
        mspi_errmsg("cam clk 0 rate set fail\n");
        return 0;
    }
#else
    mspi_clock = of_clk_get(mspi->pdev->dev.of_node, 0);
    if (IS_ERR(mspi_clock))
    {
        mspi_errmsg("get clock fail 0\n");
        return 0;
    }
    if(!__clk_is_enabled(mspi_clock))
    {
        clk_prepare_enable(mspi_clock);
    }
    clk_set_rate(mspi_clock, clk_tbl->clk_src_tbl[clk_tbl->clk_cfg_tbl[i].clk_src]);
    clk_put(mspi_clock);
#endif

    HAL_MSPI_SetDivClk(&mspi->hal, clk_tbl->clk_cfg_tbl[i].clk_div);

    mspi_dbgmsg("calc config  : %04d\n", clk_tbl->clk_cfg_tbl[i].clk_src);
    mspi_dbgmsg("calc div     : %04d\n", clk_tbl->clk_cfg_tbl[i].clk_div);
    mspi_dbgmsg("calc rate    : %d\n", clk_tbl->clk_cfg_tbl[i].clk_rate);

    return clk_tbl->clk_cfg_tbl[i].clk_rate;
}

static int sstar_spi_select_dma_clk(u8 u8Channel, struct sstar_mspi *mspi)
{
#ifdef CONFIG_CAM_CLK
    u8 cam_on;
    CAMCLK_Get_Attribute stGetCfg;
    CAMCLK_Set_Attribute stSetCfg;
#else
    u32 num_parents;
    struct clk *movdma_clock;
    struct clk_hw *movdma_hw;
    struct clk_hw *parent_hw;
#endif

#ifdef CONFIG_CAM_CLK
    if(CamClkGetOnOff(mspi->camclk[1], &cam_on))
    {
        mspi_errmsg("cam clk 1 on get fail\n");
        return -EIO;
    }
    if(!cam_on)
    {
        if(CamClkSetOnOff(mspi->camclk[1], 1))
        {
            mspi_errmsg("cam clk 1 on set fail\n");
            return -EIO;
        }
    }
    if(CamClkAttrGet(mspi->camclk[1], &stGetCfg))
    {
        mspi_errmsg("cam clk 1 att get fail\n");
        return -ENOENT;
    }
    CAMCLK_SETPARENT(stSetCfg, stGetCfg.u32Parent[u8Channel]);
#else
    num_parents = of_clk_get_parent_count(mspi->pdev->dev.of_node);
    if(num_parents < 2)
    {
        mspi_errmsg("can't find mspi clocks property %d\n", num_parents);
        return -EIO;
    }
    movdma_clock = of_clk_get(mspi->pdev->dev.of_node, 1);
    if (IS_ERR(movdma_clock))
    {
        mspi_errmsg("get clock fail 1\n");
        return -EIO;
    }
    if(!__clk_is_enabled(movdma_clock))
    {
        clk_prepare_enable(movdma_clock);
    }
    movdma_hw = __clk_get_hw(movdma_clock);
    parent_hw = clk_hw_get_parent_by_index(movdma_hw, u8Channel);
    clk_set_parent(movdma_clock, parent_hw->clk);
    clk_put(movdma_clock);
#endif

    return 0;
}

static int sstar_spi_setup(struct spi_device *spi)
{
    int err = 0;
    u32 new_clock = 0;
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(spi->master);

    mspi_dbgmsg("setup channel:%d\n", sstar_spimst->bus_num);
    if(sstar_spimst->mode != spi->mode)
    {
        sstar_spimst->mode = spi->mode;

        err = HAL_MSPI_SetMode(&sstar_spimst->hal, sstar_spimst->mode & (SPI_CPHA | SPI_CPOL));
        if(err)
        {
            return -EIO;
        }

        err = HAL_MSPI_SetLSB(&sstar_spimst->hal,(sstar_spimst->mode & SPI_LSB_FIRST)>>3);
        if(err)
        {
            return -EIO;
        }
        mspi_dbgmsg("setup mode:%d\n", sstar_spimst->mode);
    }

    if(sstar_spimst->max_speed_hz != spi->max_speed_hz)
    {
        new_clock = sstar_spi_set_clock(sstar_spimst, spi->max_speed_hz);
        if(new_clock > 0)
        {
            spi->max_speed_hz = new_clock;
            sstar_spimst->max_speed_hz = spi->max_speed_hz;
        }
        else
        {
            return -EIO;
        }

        mspi_dbgmsg("setup speed  : %d\n", sstar_spimst->max_speed_hz);
    }
    return 0;
}

static int sstar_spi_start_transfer(struct spi_device *spi,
        struct spi_transfer *tfr)
{
    int err = 0;
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(spi->master);

    mspi_dbgmsg("All = %x\n",spi->mode);
    mspi_dbgmsg("SPI mode = %d\n",spi->mode & 0x03);
    mspi_dbgmsg("LSB first = %d\n",spi->mode & 0x08);

    sstar_spimst->tx_buf = tfr->tx_buf;
    sstar_spimst->rx_buf = tfr->rx_buf;
    sstar_spimst->len = tfr->len;

    HAL_MSPI_ChipSelect(&sstar_spimst->hal,1,0);

    if(sstar_spimst->use_dma)
    {
        err = sstar_spi_select_dma_clk(spi->master->bus_num, sstar_spimst);
        if(err)
        {
            return -EIO;
        }
    }

    /*
     Document\spi\spi-summary:
     which I/O buffers are used ... each spi_transfer wraps a
     buffer for each transfer direction, supporting full duplex
     (two pointers, maybe the same one in both cases) and half
     duplex (one pointer is NULL) transfers;
    */

    if(sstar_spimst->tx_buf != NULL && sstar_spimst->rx_buf != NULL)
    {
        HAL_MSPI_FullDuplex(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->rx_buf, (u8 *)sstar_spimst->tx_buf, (u16)sstar_spimst->len);
    }
    else if(sstar_spimst->tx_buf != NULL)
    {
        if(sstar_spimst->use_dma)
        {
            err = HAL_MSPI_DMA_Write(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->tx_buf, (u16)sstar_spimst->len);
            if(err)
            {
                return -EIO;
            }
        }
        else
        {
            err = HAL_MSPI_Write(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->tx_buf,(u16)sstar_spimst->len);
            if(err)
            {
                return -EIO;
            }
        }
    }
    else if(sstar_spimst->rx_buf != NULL)
    {
        if(sstar_spimst->use_dma)
        {
            err = HAL_MSPI_DMA_Read(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->rx_buf, (u16)sstar_spimst->len);
            if(err)
            {
                return -EIO;
            }
        }
        else
        {
            err = HAL_MSPI_Read(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->rx_buf, (u16)sstar_spimst->len);
            if(err)
            {
                return -EIO;
            }
        }
    }

    return err;
}

static int sstar_spi_finish_transfer(struct spi_device *spi,
        struct spi_transfer *tfr, bool cs_change)
{
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(spi->master);

    if (tfr->delay_usecs)
        udelay(tfr->delay_usecs);

    if (cs_change)
    {
        /* Clear TA flag */
        HAL_MSPI_ChipSelect(&sstar_spimst->hal,0,0);
    }
    return 0;
}

static int sstar_spi_transfer_one(struct spi_master *master,
        struct spi_message *mesg)
{
    int err = 0;
    bool cs_change;
    struct spi_transfer *tfr;
    struct spi_device *spi = mesg->spi;
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(master);

    //mspi_dbgmsg("[sstar_spi_transfer_one]\n");

    list_for_each_entry(tfr, &mesg->transfers, transfer_list)
    {

        err = sstar_spi_start_transfer(spi, tfr);
        if (err)
        {
            mspi_dbgmsg("start_transfer err\n");
            goto out;
        }

        cs_change = tfr->cs_change ||
            list_is_last(&tfr->transfer_list, &mesg->transfers);

        err = sstar_spi_finish_transfer(spi, tfr, cs_change);
        if (err)
        {
            mspi_dbgmsg("finish transfer err\n");
            goto out;
        }
        mesg->actual_length += sstar_spimst->len;
        mspi_dbgmsg("transfered:%d\n",mesg->actual_length);
    }

out:
    /* Clear FIFOs, and disable the HW block */
    mesg->status = err;
    spi_finalize_current_message(master);

    return 0;
}

#if IS_ENABLED(CONFIG_SPI_SPIDEV)
static struct spi_board_info sstar_info =
{
    .modalias   = "spidev",
};
#endif

static int sstar_spi_probe(struct platform_device *pdev)
{
    int  i;
    int  err;
    u32  use_dma;
    u32  pad_ctrl;
    u32  irq_num;
    u32  mspi_group;
    char irq_name[20];
    u8   num_parents;
#ifdef CONFIG_CAM_CLK
    char cam_name[20];
    u32  mspi_clkid;
    u32  movedma_clkid;
    CAMCLK_Get_Attribute stGetCfg;
#else
    struct clk        *mspi_clock;
    struct clk_hw     *parent_hw;
#endif
    void __iomem      *mspi_base;
    struct sstar_mspi *sstar_spimst;
    struct spi_master *sstar_master;
    struct resource   *mspi_resource;

    mspi_dbgmsg("<<<<<<<<<<<<<<<<< Probe >>>>>>>>>>>>>>>\n");

    err = of_property_read_u32(pdev->dev.of_node, "mspi-group", &mspi_group);
    if(err)
    {
        mspi_errmsg("read mspi-group property : %d\n", err);
        return err;
    }
    mspi_dbgmsg("mspi-grounp = %d\n", mspi_group);

    irq_num = irq_of_parse_and_map(pdev->dev.of_node, 0);
    if(irq_num == 0)
    {
        mspi_errmsg("can't find interrupts property\n");
        return -ENOENT;
    }
    mspi_dbgmsg("irq_num = %d\n", irq_num);
    if(!snprintf(irq_name, sizeof(irq_name), "mspi%d interrupt", mspi_group))
    {
        mspi_errmsg("find irq reformat failed\n");
        return -ENOENT;
    }
    mspi_dbgmsg("irq name : %s\n", irq_name);

    err = of_property_read_u32(pdev->dev.of_node, "use-dma", &use_dma);
    if(err)
    {
        use_dma = 0;
    }

    if(use_dma && HAL_MSPI_CheckDmaMode(mspi_group))
    {
        mspi_warnmsg("mspi %d no support dma mode, change to normal mode default\n", mspi_group);
        use_dma = 0;
    }

    if(use_dma)
    {
        printk("[MSPI] mspi %d use dma mode\n", mspi_group);
    }
    else
    {
        printk("[MSPI] mspi %d use normal mode\n", mspi_group);
    }

    err = of_property_read_u32(pdev->dev.of_node, "pad-ctrl", &pad_ctrl);
    if(err)
    {
        mspi_dbgmsg("read pad-ctrl failed \n");
    }

    mspi_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(!mspi_resource)
    {
        return -ENOENT;
    }
    mspi_base = (void *)(IO_ADDRESS(mspi_resource->start));
    mspi_dbgmsg("mspi_base = %px\n",mspi_base);

    sstar_master = spi_alloc_master(&pdev->dev, sizeof(struct sstar_mspi));
    if (!sstar_master)
    {
        mspi_errmsg( "alloc spi master failed\n");
        dev_err(&pdev->dev, "alloc spi master failed\n");
        return -ENOMEM;
    }
    sstar_master->mode_bits = SSTAR_SPI_MODE_BITS;
    sstar_master->bits_per_word_mask = SPI_BPW_MASK(8);
    sstar_master->num_chipselect = 3;
    sstar_master->transfer_one_message = sstar_spi_transfer_one;
    sstar_master->dev.of_node = pdev->dev.of_node;
    sstar_master->setup = sstar_spi_setup;
    //sstar_master->max_speed_hz = 72000000;
    //sstar_master->min_speed_hz = 46875;
    sstar_master->bus_num = mspi_group;
    platform_set_drvdata(pdev, sstar_master);

    sstar_spimst = spi_master_get_devdata(sstar_master);
    sstar_spimst->hal.mspi_base = (u32)mspi_base;
    sstar_spimst->use_dma = use_dma;
    sstar_spimst->bus_num = mspi_group;
    sstar_spimst->pdev = pdev;
    sstar_spimst->hal.pad_ctrl= pad_ctrl;
    init_completion(&sstar_spimst->hal.done);

#ifndef CONFIG_SPI_INT_CALL
    err = request_irq(irq_num, sstar_spi_interrupt, 0, irq_name, (void*)sstar_master);
    if (err == 0)
    {
        mspi_dbgmsg("%s registered\n", irq_name);
    }
    else
    {
        mspi_errmsg("%s register failed", irq_name);
        goto err_irq;
    }
    sstar_spimst->irq_num = irq_num;
#endif

#ifdef CONFIG_CAM_CLK
    sstar_spimst->camclk = kmalloc((sizeof(void *) * 2), GFP_KERNEL);
    if(!sstar_spimst->camclk)
    {
        err = -ENOMEM;
        mspi_errmsg("malloc camclk fail\n");
        goto err_cam;
    }
    err = of_property_read_u32_index(pdev->dev.of_node,"camclk", 0, &mspi_clkid);
    if(err)
    {
        err = -EINVAL;
        mspi_errmsg("read camclk 0 property fail\n");
        goto err_cam;
    }
    mspi_dbgmsg("mspi %d camclk property : %d\n", mspi_group, mspi_clkid);

    err = of_property_read_u32_index(pdev->dev.of_node,"camclk", 1, &movedma_clkid);
    if(err)
    {
        err = -EINVAL;
        mspi_errmsg("read camclk 1 property fail\n");
        goto err_cam;
    }
    mspi_dbgmsg("movedma camclk property : %d\n", movedma_clkid);

    if(!snprintf(cam_name, sizeof(cam_name), "mspi%d camclk", mspi_group))
    {
        mspi_errmsg("mspi %d camclk name reformat failed\n", mspi_group);
        err = -ENOENT;
        goto err_cam;
    }
    mspi_dbgmsg("camlck name : %s\n", cam_name);

    if(CamClkRegister(cam_name, mspi_clkid, &sstar_spimst->camclk[0]))
    {
        err = -ENOENT;
        mspi_errmsg("register mspi %d camclk fail\n", mspi_group);
        goto err_cam;
    }

    if(CamClkRegister("movedma camclk", movedma_clkid, &sstar_spimst->camclk[1]))
    {
        err = -ENOENT;
        mspi_errmsg("register movedma camclk fail\n");
        goto err_cam;
    }

    if(CamClkAttrGet(sstar_spimst->camclk[0], &stGetCfg))
    {
        err = -ENOENT;
        mspi_errmsg("get camclk att fail\n");
        goto err_cam;
    }
    num_parents = stGetCfg.u32NodeCount;
    mspi_dbgmsg("parents n = %d\n", num_parents);
    sstar_spimst->mspi_clk_tbl.clk_src_tbl_sz = num_parents;
    sstar_spimst->mspi_clk_tbl.clk_src_tbl = kmalloc(sizeof(u32) * sstar_spimst->mspi_clk_tbl.clk_src_tbl_sz, GFP_KERNEL);
    if(!sstar_spimst->mspi_clk_tbl.clk_src_tbl)
    {
        err = -ENOMEM;
        mspi_errmsg("malloc clk_src_tbl fail\n");
        goto err_src;
    }
    for (i = 0; i < num_parents; i++)
    {
        sstar_spimst->mspi_clk_tbl.clk_src_tbl[i] = CamClkRateGet(stGetCfg.u32Parent[i]);
        mspi_dbgmsg("clk src %d = %d\n", i, sstar_spimst->mspi_clk_tbl.clk_src_tbl[i]);
    }
#else
    // clock table source table cfg
    mspi_clock = of_clk_get(pdev->dev.of_node, 0);
    if (IS_ERR(mspi_clock))
    {
        mspi_errmsg("get mspi %d clock fail\n", mspi_group);
        err = -ENXIO;
        goto err_irq;
    }
    num_parents = clk_hw_get_num_parents(__clk_get_hw(mspi_clock));
    mspi_dbgmsg("parents n = %d\n", num_parents);
    sstar_spimst->mspi_clk_tbl.clk_src_tbl_sz = num_parents;
    sstar_spimst->mspi_clk_tbl.clk_src_tbl = kmalloc(sizeof(u32) * sstar_spimst->mspi_clk_tbl.clk_src_tbl_sz, GFP_KERNEL);
    if(!sstar_spimst->mspi_clk_tbl.clk_src_tbl)
    {
        err = -ENOMEM;
        mspi_errmsg("malloc clk_src_tbl fail\n");
        goto err_src;
    }
    for (i = 0; i < num_parents; i++)
    {
        parent_hw = clk_hw_get_parent_by_index(__clk_get_hw(mspi_clock), i);
        sstar_spimst->mspi_clk_tbl.clk_src_tbl[i] = clk_get_rate(parent_hw->clk);
        mspi_dbgmsg("clk src %d = %d\n", i, sstar_spimst->mspi_clk_tbl.clk_src_tbl[i]);
    }
    clk_put(mspi_clock);
#endif

    // clock table divide table cfg
    sstar_spimst->mspi_clk_tbl.clk_div_tbl_sz = ARRAY_SIZE(sstar_spi_clk_div_tbl);
    sstar_spimst->mspi_clk_tbl.clk_div_tbl    = kmalloc(sizeof(u32) * sstar_spimst->mspi_clk_tbl.clk_div_tbl_sz, GFP_KERNEL);
    if (!sstar_spimst->mspi_clk_tbl.clk_div_tbl)
    {
        err = -ENOMEM;
        mspi_errmsg("malloc clk_div_tbl fail\n");
        goto err_div;
    }
    memcpy(sstar_spimst->mspi_clk_tbl.clk_div_tbl, sstar_spi_clk_div_tbl, sizeof(sstar_spi_clk_div_tbl));
    for (i = 0; i < ARRAY_SIZE(sstar_spi_clk_div_tbl); i++)
    {
        mspi_dbgmsg("clk div %d = %d\n", i, sstar_spimst->mspi_clk_tbl.clk_div_tbl[i]);
    }

    // clock table config table malloc
    sstar_spimst->mspi_clk_tbl.clk_cfg_tbl_sz = sstar_spimst->mspi_clk_tbl.clk_div_tbl_sz * sstar_spimst->mspi_clk_tbl.clk_src_tbl_sz;
    sstar_spimst->mspi_clk_tbl.clk_cfg_tbl = kmalloc(sizeof(struct mspi_clk) * sstar_spimst->mspi_clk_tbl.clk_cfg_tbl_sz, GFP_KERNEL);
    if(!sstar_spimst->mspi_clk_tbl.clk_cfg_tbl)
    {
        err = -ENOMEM;
        mspi_errmsg("malloc clk_cfg_tbl fail\n");
        goto err_cfg;
    }

    /* initialise the clock calc table for calc closest clock */
    sstar_spi_clk_tbl_init(&sstar_spimst->mspi_clk_tbl);

    sstar_master->max_speed_hz = sstar_spimst->mspi_clk_tbl.clk_cfg_tbl[sstar_spimst->mspi_clk_tbl.clk_cfg_tbl_sz-1].clk_rate;
    sstar_master->min_speed_hz = sstar_spimst->mspi_clk_tbl.clk_cfg_tbl[0].clk_rate;
    mspi_dbgmsg("max_speed_hz = %d\n", sstar_master->max_speed_hz);
    mspi_dbgmsg("min_speed_hz = %d\n", sstar_master->min_speed_hz);

    /* initialise the hardware */
    err = HAL_MSPI_Config(&sstar_spimst->hal);
    if(err)
    {
        err = -EIO;
        mspi_errmsg("config mspi%d master: %d\n", mspi_group, err);
        dev_err(&pdev->dev, "config mspi%d master: %d\n", mspi_group, err);
        goto err_out;
    }

    //err = devm_spi_register_master(sstar_master);
    err = spi_register_master(sstar_master);
    if (err)
    {
        mspi_errmsg("could not register mspi%d master: %d\n", mspi_group, err);
        dev_err(&pdev->dev, "could not register mspi%d master: %d\n", mspi_group, err);
        goto err_out;
    }

    mspi_dbgmsg("<<<<<<<<<<<<<<< Probe End >>>>>>>>>>>>>\n");

#if IS_ENABLED(CONFIG_SPI_SPIDEV)
    spi_new_device(sstar_master, &sstar_info);  // use for "user mode spi device driver support"
#endif

    return 0;

err_out:
    spi_unregister_master(sstar_master);
err_cfg:
    kfree(sstar_spimst->mspi_clk_tbl.clk_cfg_tbl);
err_div:
    kfree(sstar_spimst->mspi_clk_tbl.clk_div_tbl);
err_src:
    kfree(sstar_spimst->mspi_clk_tbl.clk_src_tbl);
#ifdef CONFIG_CAM_CLK
err_cam:
    kfree(sstar_spimst->camclk);
#endif
err_irq:
    free_irq(irq_num, (void*)sstar_master);
    return err;
}

static int sstar_spi_remove(struct platform_device *pdev)
{
#ifdef CONFIG_CAM_CLK
    u8  cam_on;
#else
    struct clk *mspi_clock;
    struct clk *movdma_clock;
    struct clk_hw *mspi_hw;
    struct clk_hw *movdma_hw;
    struct clk_hw *parent_hw;
#endif

    struct spi_master *sstar_master = platform_get_drvdata(pdev);
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(sstar_master);

    if(sstar_master)
    {

#ifdef CONFIG_CAM_CLK
        if(CamClkGetOnOff(sstar_spimst->camclk[0], &cam_on))
        {
            mspi_errmsg("cam clk 0 get on fail\n");
            return -ENOENT;
        }
        if(cam_on)
        {
            if(CamClkSetOnOff(sstar_spimst->camclk[0], 0))
            {
                mspi_errmsg("cam clk 0 set on fail\n");
                return -ENOENT;
            }
        }
        if(CamClkUnregister(sstar_spimst->camclk[0]))
        {
            mspi_errmsg("cam clk 0 unregister fail\n");
            return -ENOENT;
        }

        if(CamClkGetOnOff(sstar_spimst->camclk[1], &cam_on))
        {
            mspi_errmsg("cam clk 1 get on fail\n");
            return -ENOENT;
        }
        if(cam_on)
        {
            if(CamClkSetOnOff(sstar_spimst->camclk[1], 0))
            {
                mspi_errmsg("cam clk 1 set on fail\n");
                return -ENOENT;
            }
        }
        if(CamClkUnregister(sstar_spimst->camclk[1]))
        {
            mspi_errmsg("cam clk 1 unregister fail\n");
            return -ENOENT;
        }
        kfree(sstar_spimst->camclk);
        sstar_spimst->camclk = NULL;
#else
        mspi_clock = of_clk_get(sstar_spimst->pdev->dev.of_node, 0);
        if (IS_ERR(mspi_clock))
        {
            mspi_errmsg("get clock fail 0\n");
            return -ENOENT;
        }
        if(__clk_is_enabled(mspi_clock))
        {
            mspi_hw = __clk_get_hw(mspi_clock);
            parent_hw = clk_hw_get_parent_by_index(mspi_hw, 0);
            clk_set_parent(mspi_clock, parent_hw->clk);
            clk_disable_unprepare(mspi_clock);
        }
        clk_put(mspi_clock);

        movdma_clock = of_clk_get(sstar_spimst->pdev->dev.of_node, 1);
        if (IS_ERR(movdma_clock))
        {
            mspi_errmsg("get clock fail 1\n");
            return -ENOENT;
        }
        if(__clk_is_enabled(movdma_clock))
        {
            movdma_hw = __clk_get_hw(movdma_clock);
            parent_hw = clk_hw_get_parent_by_index(movdma_hw, 0);
            clk_set_parent(movdma_clock, parent_hw->clk);
            clk_disable_unprepare(movdma_clock);
        }
        clk_put(movdma_clock);
#endif

        free_irq(sstar_spimst->irq_num, (void *)sstar_master);

        if(sstar_spimst->mspi_clk_tbl.clk_cfg_tbl)
        {
            kfree(sstar_spimst->mspi_clk_tbl.clk_cfg_tbl);
        }
        if(sstar_spimst->mspi_clk_tbl.clk_div_tbl)
        {
            kfree(sstar_spimst->mspi_clk_tbl.clk_div_tbl);
        }
        if(sstar_spimst->mspi_clk_tbl.clk_src_tbl)
        {
            kfree(sstar_spimst->mspi_clk_tbl.clk_src_tbl);
        }

        spi_unregister_master(sstar_master);

    }
    else
    {
        return -EINVAL;
    }
    return 0;
}

#ifdef CONFIG_SPI_INT_CALL
int sstar_spi_transfer(struct spi_device *spi, struct spi_transfer *tfr, int chip_select)
{
    int ret = 0;
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(spi->master);

    sstar_spimst->tx_buf = tfr->tx_buf;
    sstar_spimst->rx_buf = tfr->rx_buf;
    sstar_spimst->len = tfr->len;

    if(chip_select)
    {
        MDrv_GPIO_Set_High(chip_select);
        MDrv_GPIO_Set_Low(chip_select);
    }
    else
    {
        HAL_MSPI_ChipSelect(&sstar_spimst->hal,1,0);
    }

    if(sstar_spimst->tx_buf != NULL && sstar_spimst->rx_buf != NULL)
    {
        ret = HAL_MSPI_FullDuplex(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->rx_buf, (u8 *)sstar_spimst->tx_buf, (u16)sstar_spimst->len);
    }
    else if(sstar_spimst->tx_buf != NULL)
    {
        ret = HAL_MSPI_Write(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->tx_buf,(u16)sstar_spimst->len);
    }
    else if(sstar_spimst->rx_buf != NULL)
    {
        ret = HAL_MSPI_Read(spi->master->bus_num, &sstar_spimst->hal, (u8 *)sstar_spimst->rx_buf, (u16)sstar_spimst->len);
    }

    if(chip_select)
    {
        MDrv_GPIO_Set_High(chip_select);
    }
    else
    {
        HAL_MSPI_ChipSelect(&sstar_spimst->hal,0,0);
    }

    return ret;
}
EXPORT_SYMBOL_GPL(sstar_spi_transfer);
#endif


#ifdef CONFIG_PM_SLEEP
static int sstar_spi_suspend(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct spi_master *sstar_master = platform_get_drvdata(pdev);
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(sstar_master);

    struct clk *mspi_clock;
    struct clk *movdma_clock;
    struct clk_hw *mspi_hw;
    struct clk_hw *movdma_hw;
    struct clk_hw *parent_hw;

    mspi_clock = of_clk_get(sstar_spimst->pdev->dev.of_node, 0);
    if (IS_ERR(mspi_clock))
    {
        mspi_errmsg("get clock fail 0\n");
        return -ENOENT;
    }
    if(__clk_is_enabled(mspi_clock))
    {
        mspi_hw = __clk_get_hw(mspi_clock);
        parent_hw = clk_hw_get_parent_by_index(mspi_hw, 0);
        clk_set_parent(mspi_clock, parent_hw->clk);
        clk_disable_unprepare(mspi_clock);
    }
    clk_put(mspi_clock);

    movdma_clock = of_clk_get(sstar_spimst->pdev->dev.of_node, 1);
    if (IS_ERR(movdma_clock))
    {
        mspi_errmsg("get clock fail 1\n");
        return -ENOENT;
    }
    if(__clk_is_enabled(movdma_clock))
    {
        movdma_hw = __clk_get_hw(movdma_clock);
        parent_hw = clk_hw_get_parent_by_index(movdma_hw, 0);
        clk_set_parent(movdma_clock, parent_hw->clk);
        clk_disable_unprepare(movdma_clock);
    }
    clk_put(movdma_clock);

    return 0;
}

static int sstar_spi_resume(struct device *dev)
{
    int err = 0;
    u32 new_clock = 0;
    struct platform_device *pdev = to_platform_device(dev);
    struct spi_master *sstar_master = platform_get_drvdata(pdev);
    struct sstar_mspi *sstar_spimst = spi_master_get_devdata(sstar_master);

    err = HAL_MSPI_Config(&sstar_spimst->hal);
    if(err)
    {
        return -EIO;
    }

    err = HAL_MSPI_SetMode(&sstar_spimst->hal, sstar_spimst->mode & (SPI_CPHA | SPI_CPOL));
    if(err)
    {
        return -EIO;
    }

    err = HAL_MSPI_SetLSB(&sstar_spimst->hal, (sstar_spimst->mode & SPI_LSB_FIRST)>>3);
    if(err)
    {
        return -EIO;
    }

    new_clock = sstar_spi_set_clock(sstar_spimst, sstar_spimst->max_speed_hz);
    if(new_clock == 0)
    {
        return -EIO;;
    }
    mspi_dbgmsg("resume mode:0x%x speed:%d channel:%d\n",sstar_spimst->mode,sstar_spimst->max_speed_hz,sstar_master->bus_num);
    return err;
}

#else
#define sstar_spi_suspend   NULL
#define sstar_spi_resume    NULL
#endif

static const struct of_device_id sstar_spi_match[] = {
    { .compatible = "sstar,mspi", },
    {}
};
MODULE_DEVICE_TABLE(of, sstar_spi_match);

static const struct dev_pm_ops sstar_spi_pm_ops = {
    .suspend = sstar_spi_suspend,
    .resume  = sstar_spi_resume,
};

static struct platform_driver sstar_spi_driver = {
    .driver     = {
        .name       = "spi",
        .owner      = THIS_MODULE,
        .pm         = &sstar_spi_pm_ops,
        .of_match_table = sstar_spi_match,
    },
    .probe      = sstar_spi_probe,
    .remove     = sstar_spi_remove,
};
module_platform_driver(sstar_spi_driver);

MODULE_DESCRIPTION("SPI controller driver for SigmaStar");
MODULE_AUTHOR("Gavin Xu <gavin.xu@sigmastar.com.cn>");
MODULE_LICENSE("GPL v2");

