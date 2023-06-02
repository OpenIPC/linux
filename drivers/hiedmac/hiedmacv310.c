/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/pm_runtime.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/hiedmac.h>

#include "hiedmacv310.h"

#ifdef CONFIG_ARCH_HI3559AV100
#include "hiedma_hi3559av100.h"
#endif

#if defined(CONFIG_ARCH_HI3519AV100) || defined(CONFIG_ARCH_HI3556AV100)
#include "hiedma_hi3519av100.h"
#endif

#if defined(CONFIG_ARCH_HI3516CV500) || defined(CONFIG_ARCH_HI3516DV300) || defined(CONFIG_ARCH_HI3556V200) || defined(CONFIG_ARCH_HI3559V200)
#include "hiedma_hi3516cv500.h"
#endif

#if defined(CONFIG_ARCH_HI3516EV200) || defined(CONFIG_ARCH_HI3516EV300) || defined(CONFIG_ARCH_HI3518EV300) || defined(CONFIG_ARCH_HI3516DV200)
#include "hiedma_hi3516ev200.h"
#endif

int g_channel_status[HIEDMAC_CHANNEL_NUM];
DMAC_ISR *function[HIEDMAC_CHANNEL_NUM];
unsigned long pllihead[2] = {0, 0};
void __iomem *dma_regbase;
int hiedmacv310_trace_level_n = HIEDMACV310_TRACE_LEVEL;

struct hiedmac_host {
    struct platform_device *pdev;
    void __iomem *base;
    struct regmap *misc_regmap;
    unsigned int misc_ctrl_base;
    void __iomem *crg_ctrl;
    unsigned int id;
    struct clk *clk;
    struct clk *axi_clk;
    unsigned int irq;
    struct reset_control *rstc;
    unsigned int channels;
    unsigned int slave_requests;
};

#define DRIVER_NAME "hiedmacv310"

int dmac_channel_allocate(void)
{
    unsigned int i;

    for (i = 0; i < HIEDMAC_CHANNEL_NUM; i++) {
        if (g_channel_status[i] == DMAC_CHN_VACANCY) {
            g_channel_status[i] = DMAC_CHN_ALLOCAT;
            return i;
        }
    }

    hiedmacv310_error("no to alloc\n");
    return -1;
}
EXPORT_SYMBOL(dmac_channel_allocate);

/*
 *  update the state of channels
 */
static int hiedmac_update_status(unsigned int channel)
{

    unsigned int channel_status;
    unsigned int channel_tc_status;
    unsigned int channel_err_status[3];
    unsigned int i = channel;
    unsigned long update_jiffies_timeout;

    update_jiffies_timeout = jiffies + HIEDMAC_UPDATE_TIMEOUT;

    while (1) {
        channel_status = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_STAT);
        channel_status = (channel_status >> i) & 0x01;
        if (channel_status) {
            channel_tc_status = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_TC1);
            channel_tc_status = (channel_tc_status >> i) & 0x01;
            if (channel_tc_status) {
                hiedmacv310_writel(1 << i, dma_regbase + HIEDMAC_INT_TC1_RAW);
                g_channel_status[i] = DMAC_CHN_SUCCESS;
                break;
            }

            channel_tc_status = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_TC2);
            channel_tc_status = (channel_tc_status >> i) & 0x01;
            if (channel_tc_status) {
                hiedmacv310_writel(1 << i, dma_regbase + HIEDMAC_INT_TC2_RAW);
                g_channel_status[i] = DMAC_CHN_SUCCESS;
                break;
            }

            channel_err_status[0] = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_ERR1);
            channel_err_status[0] = (channel_err_status[0] >> i) & 0x01;
            channel_err_status[1] = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_ERR2);
            channel_err_status[1] = (channel_err_status[1] >> i) & 0x01;
            channel_err_status[2] = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_ERR3);
            channel_err_status[2] = (channel_err_status[2] >> i) & 0x01;

            if (channel_err_status[0] | channel_err_status[1] | channel_err_status[2]) {
                hiedmacv310_error("Error in HIEDMAC %d finish!\n", i);
                channel_err_status[0] = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_ERR1);
                channel_err_status[1] = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_ERR2);
                channel_err_status[2] = hiedmacv310_readl(dma_regbase + HIEDMAC_INT_ERR3);
                g_channel_status[i] = -DMAC_CHN_ERROR;
                hiedmacv310_writel(1 << i, dma_regbase + HIEDMAC_INT_ERR1_RAW);
                hiedmacv310_writel(1 << i, dma_regbase + HIEDMAC_INT_ERR2_RAW);
                hiedmacv310_writel(1 << i, dma_regbase + HIEDMAC_INT_ERR3_RAW);
                break;
            }
        }

        if (!time_before(jiffies, update_jiffies_timeout)) {
            hiedmacv310_error("Timeout in DMAC %d!\n", i);
            g_channel_status[i] = -DMAC_CHN_TIMEOUT;
            break;
        }
    }

    return g_channel_status[i];
}

/*
 * register user's function
 */
int dmac_register_isr(unsigned int channel, void *pisr)
{
    if (channel < 0 || channel > HIEDMAC_CHANNEL_NUM - 1) {
        hiedmacv310_error("invalid channel,channel=%0d\n", channel);
        return -EINVAL;
    }

    function[channel] = (void *)pisr;

    return 0;
}
EXPORT_SYMBOL(dmac_register_isr);

/*
 *  free channel
 */
int dmac_channel_free(unsigned int channel)
{
    g_channel_status[channel] = DMAC_CHN_VACANCY;
    return 0;
}
EXPORT_SYMBOL(dmac_channel_free);

static unsigned int dmac_check_request(unsigned int peripheral_addr, int direction)
{
    int i;

    for (i = direction; i < EDMAC_MAX_PERIPHERALS; i += 2) {
        if (g_peripheral[i].peri_addr == peripheral_addr) {
            return i;
        }
    }
    hiedmacv310_error("Invalid devaddr\n");
    return -1;
}

void edmac_channel_free(int channel)
{
    g_channel_status[channel] = DMAC_CHN_VACANCY;
}
/*
 *  wait for transfer end
 */
int dmac_wait(int channel)
{
    int ret_result;
    int ret = 0;

    if (channel < 0) {
        return -1;
    }

    while (1) {
        ret_result = hiedmac_update_status(channel);
        if (ret_result == -DMAC_CHN_ERROR) {
            hiedmacv310_error("Transfer Error.\n");
            ret = -1;
            goto end;
        } else  if (ret_result == DMAC_NOT_FINISHED) {
            udelay(10);
        } else if (ret_result == DMAC_CHN_SUCCESS) {
            ret = DMAC_CHN_SUCCESS;
            goto end;
        } else if (ret_result == DMAC_CHN_VACANCY) {
            ret = DMAC_CHN_SUCCESS;
            goto end;
        } else if (ret_result == -DMAC_CHN_TIMEOUT) {
            hiedmacv310_error("Timeout.\n");
            hiedmacv310_writel(HIEDMAC_Cx_DISABLE, dma_regbase + HIEDMAC_Cx_CONFIG(channel));
            g_channel_status[channel] = DMAC_CHN_VACANCY;
            ret = -1;
            return ret;
        }
    }
end:
    hiedmacv310_writel(HIEDMAC_Cx_DISABLE, dma_regbase + HIEDMAC_Cx_CONFIG(channel));
    edmac_channel_free(channel);
    return ret;
}
EXPORT_SYMBOL(dmac_wait);

/*
 *  execute memory to peripheral dma transfer without LLI
 */
int dmac_m2p_transfer(unsigned long long memaddr, unsigned int uwperipheralid,
                      unsigned int length)
{
    unsigned int ulchnn;
    unsigned int uwwidth;
    unsigned int temp;

    ulchnn = dmac_channel_allocate();
    if (-1 == ulchnn) {
        return -1;
    }

    hiedmacv310_trace(4, "ulchnn = %d\n", ulchnn);
    uwwidth = g_peripheral[uwperipheralid].transfer_width;
    if (length >> uwwidth >= HIEDMAC_TRANS_MAXSIZE) {
        hiedmacv310_error("The length is more than 64k!\n");
        return -1;
    }

    hiedmacv310_writel(memaddr & 0xffffffff, dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(ulchnn));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((memaddr >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_SRC_ADDR_H(ulchnn));
#endif
    hiedmacv310_trace(4, "HIEDMAC_Cx_SRC_ADDR_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(ulchnn)));

    hiedmacv310_writel(g_peripheral[uwperipheralid].peri_addr & 0xffffffff,
                       dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(ulchnn));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((g_peripheral[uwperipheralid].peri_addr >> 32) & 0xffffffff,
                       dma_regbase + HIEDMAC_Cx_DEST_ADDR_H(ulchnn));
#endif
    hiedmacv310_trace(4, "HIEDMAC_Cx_DEST_ADDR_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(ulchnn)));

    hiedmacv310_writel(0, dma_regbase + HIEDMAC_Cx_LLI_L(ulchnn));
    hiedmacv310_trace(4, "HIEDMAC_Cx_LLI_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_LLI_L(ulchnn)));

    hiedmacv310_writel(length, dma_regbase + HIEDMAC_Cx_CNT0(ulchnn));
    hiedmacv310_trace(4, "HIEDMAC_Cx_CNT0 = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_CNT0(ulchnn)));

    temp = g_peripheral[uwperipheralid].transfer_cfg | uwwidth << EDMA_SRC_WIDTH_OFFSET
           | (g_peripheral[uwperipheralid].dynamic_periphery_num << PERI_ID_OFFSET)
           | EDMA_CH_ENABLE;
    hiedmacv310_trace(4, "HIEDMAC_Cx_CONFIG = 0x%x\n", temp);
    hiedmacv310_writel(temp, dma_regbase + HIEDMAC_Cx_CONFIG(ulchnn));
    return ulchnn;
}

/*
 *  execute memory to peripheral dma transfer without LLI
 */
int dmac_p2m_transfer(unsigned long memaddr, unsigned int uwperipheralid,
                      unsigned int length)
{
    unsigned int ulchnn;
    unsigned int uwwidth;
    unsigned int temp;

    ulchnn = dmac_channel_allocate();
    if (-1 == ulchnn) {
        return -1;
    }

    hiedmacv310_trace(4, "ulchnn = %d\n", ulchnn);
    uwwidth = g_peripheral[uwperipheralid].transfer_width;
    if (length >> uwwidth >= HIEDMAC_TRANS_MAXSIZE) {
        hiedmacv310_error("The length is more than 64k!\n");
        return -1;
    }

    hiedmacv310_writel(memaddr & 0xffffffff, dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(ulchnn));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((memaddr >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_DEST_ADDR_H(ulchnn));
#endif
    hiedmacv310_trace(4, "HIEDMAC_Cx_DEST_ADDR_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(ulchnn)));

    hiedmacv310_writel(g_peripheral[uwperipheralid].peri_addr & 0xffffffff,
                       dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(ulchnn));
#ifdef CONFIG_ARM64
    hiedmacv310_writel(0, dma_regbase + HIEDMAC_Cx_SRC_ADDR_H(ulchnn));
#endif
    hiedmacv310_trace(4, "HIEDMAC_Cx_SRC_ADDR_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(ulchnn)));

    hiedmacv310_writel(0, dma_regbase + HIEDMAC_Cx_LLI_L(ulchnn));
    hiedmacv310_trace(4, "HIEDMAC_Cx_LLI_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_LLI_L(ulchnn)));

    hiedmacv310_writel(length, dma_regbase + HIEDMAC_Cx_CNT0(ulchnn));
    hiedmacv310_trace(4, "HIEDMAC_Cx_CNT0 = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_CNT0(ulchnn)));

    temp = g_peripheral[uwperipheralid].transfer_cfg | uwwidth << EDMA_SRC_WIDTH_OFFSET
           | (g_peripheral[uwperipheralid].dynamic_periphery_num << PERI_ID_OFFSET)
           | EDMA_CH_ENABLE;
    hiedmacv310_trace(4, "HIEDMAC_Cx_CONFIG = 0x%x\n", temp);
    hiedmacv310_writel(temp, dma_regbase + HIEDMAC_Cx_CONFIG(ulchnn));
    return ulchnn;
}

int do_dma_m2p(unsigned long long memaddr, unsigned int peripheral_addr,
               unsigned int length)
{
    int ret = 0;
    int uwperipheralid;

    uwperipheralid = dmac_check_request(peripheral_addr, EDMAC_TX);
    if (uwperipheralid < 0) {
        hiedmacv310_error("m2p:Invalid devaddr\n");
        return -1;
    }

    ret = dmac_m2p_transfer(memaddr, uwperipheralid, length);
    if (ret == -1) {
        hiedmacv310_error("m2p:trans err\n");
        return -1;
    }

    return ret;
}
EXPORT_SYMBOL(do_dma_m2p);

int do_dma_p2m(unsigned long memaddr, unsigned int peripheral_addr,
               unsigned int length)
{
    int ret = -1;
    int uwperipheralid;

    uwperipheralid = dmac_check_request(peripheral_addr, EDMAC_RX);
    if (uwperipheralid < 0) {
        hiedmacv310_error("p2m:Invalid devaddr.\n");
        return -1;
    }

    ret = dmac_p2m_transfer(memaddr, uwperipheralid, length);
    if (ret == -1) {
        hiedmacv310_error("p2m:trans err\n");
        return -1;
    }

    return ret;
}
EXPORT_SYMBOL(do_dma_p2m);

/*
 *  buile LLI for memory to memory DMA transfer
 */
int dmac_buildllim2m(unsigned long *ppheadlli,
                     unsigned long psource,
                     unsigned long pdest,
                     unsigned int totaltransfersize,
                     unsigned int uwnumtransfers)
{
    int lli_num = 0;
    unsigned long phy_address;
    int j;
    dmac_lli  *plli;

    lli_num = (totaltransfersize / uwnumtransfers);
    if ((totaltransfersize % uwnumtransfers) != 0) {
        lli_num++;
    }

    hiedmacv310_trace(4, "lli_num:%d\n", lli_num);

    phy_address = ppheadlli[0];
    plli = (dmac_lli *)ppheadlli[1];
    hiedmacv310_trace(4, "phy_address: 0x%lx\n", phy_address);
    hiedmacv310_trace(4, "address: 0x%p\n", plli);
    for (j = 0; j < lli_num; j++) {
        memset(plli, 0x0, sizeof(dmac_lli));
        /*
         * at the last transfer, chain_en should be set to 0x0;
         * others tansfer,chain_en should be set to 0x2;
         */
        plli->next_lli = (phy_address + (j + 1) * sizeof(dmac_lli)) & (~(HIEDMAC_LLI_ALIGN - 1));
        if (j < lli_num - 1) {
            plli->next_lli |= HIEDMAC_LLI_ENABLE;
            plli->count = uwnumtransfers;
        } else {
            plli->next_lli |= HIEDMAC_LLI_DISABLE;
            plli->count = totaltransfersize % uwnumtransfers;
        }

        plli->src_addr = psource;
        plli->dest_addr = pdest;
        plli->config = HIEDMAC_CxCONFIG_M2M_LLI;

        psource += uwnumtransfers;
        pdest += uwnumtransfers;
        plli++;
    }

    return 0;
}
EXPORT_SYMBOL(dmac_buildllim2m);

/*
 *  load configuration from LLI for memory to memory
 */
int dmac_start_llim2m(unsigned int channel, unsigned long *pfirst_lli)
{
    unsigned int i = channel;
    dmac_lli  *plli;

    plli = (dmac_lli  *)pfirst_lli[1];
    hiedmacv310_trace(4, "plli.src_addr: 0x%lx\n", plli->src_addr);
    hiedmacv310_trace(4, "plli.dst_addr: 0x%lx\n", plli->dest_addr);
    hiedmacv310_trace(4, "plli.next_lli: 0x%lx\n", plli->next_lli);
    hiedmacv310_trace(4, "plli.count: 0x%d\n", plli->count);


    hiedmacv310_writel(plli->dest_addr & 0xffffffff, dma_regbase + HIEDMAC_Cx_LLI_L(i));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((plli->dest_addr >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_LLI_H(i));
#endif
    hiedmacv310_writel(plli->count, dma_regbase + HIEDMAC_Cx_CNT0(i));

    hiedmacv310_writel(plli->src_addr & 0xffffffff, dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(i));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((plli->src_addr >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_SRC_ADDR_H(i));
#endif
    hiedmacv310_writel(plli->dest_addr & 0xffffffff, dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(i));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((plli->dest_addr >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_DEST_ADDR_H(i));
#endif
    hiedmacv310_writel(plli->config | EDMA_CH_ENABLE, dma_regbase + HIEDMAC_Cx_CONFIG(i));

    return 0;
}
EXPORT_SYMBOL(dmac_start_llim2m);

/*
 * config register for memory to memory DMA transfer without LLI
 */
int dmac_start_m2m(unsigned int  channel, unsigned long psource,
                   unsigned long pdest, unsigned int uwnumtransfers)
{
    unsigned int i = channel;

    if (uwnumtransfers > HIEDMAC_TRANS_MAXSIZE || uwnumtransfers == 0) {
        hiedmacv310_error("Invalidate transfer size,size=%x\n", uwnumtransfers);
        return -EINVAL;
    }
    hiedmacv310_trace(4, "channel[%d],source=0x%lx,dest=0x%lx,length=%d\n",
                      channel, psource, pdest, uwnumtransfers);

    hiedmacv310_writel(psource & 0xffffffff, dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(i));
    hiedmacv310_trace(4, "HIEDMAC_Cx_SRC_ADDR_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_SRC_ADDR_L(i)));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((psource >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_SRC_ADDR_H(i));
    hiedmacv310_trace(4, "HIEDMAC_Cx_SRC_ADDR_H = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_SRC_ADDR_H(i)));
#endif
    hiedmacv310_writel(pdest & 0xffffffff, dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(i));
    hiedmacv310_trace(4, "HIEDMAC_Cx_DEST_ADDR_L = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_DEST_ADDR_L(i)));
#ifdef CONFIG_ARM64
    hiedmacv310_writel((pdest >> 32) & 0xffffffff, dma_regbase + HIEDMAC_Cx_DEST_ADDR_H(i));
    hiedmacv310_trace(4, "HIEDMAC_Cx_DEST_ADDR_H = 0x%x\n", hiedmacv310_readl(dma_regbase + HIEDMAC_Cx_DEST_ADDR_H(i)));
#endif
    hiedmacv310_writel(0, dma_regbase + HIEDMAC_Cx_LLI_L(i));

    hiedmacv310_writel(uwnumtransfers, dma_regbase + HIEDMAC_Cx_CNT0(i));

    hiedmacv310_writel(HIEDMAC_CxCONFIG_M2M | EDMA_CH_ENABLE, dma_regbase + HIEDMAC_Cx_CONFIG(i));

    return 0;
}
EXPORT_SYMBOL(dmac_start_m2m);

/*
 *  execute memory to memory dma transfer without LLI
 */
int dmac_m2m_transfer(unsigned long source, unsigned long dest,
                      unsigned int length)
{
    unsigned int ulchnn;
    unsigned int dma_size = 0;
    unsigned int dma_count, left_size;

    left_size = length;
    dma_count = 0;
    ulchnn = dmac_channel_allocate();
    if (ulchnn < 0) {
        return -EINVAL;
    }

    hiedmacv310_trace(6, "using channel[%d],source=0x%lx,dest=0x%lx,length=%d\n",
                      ulchnn, source, dest, length);

    while (left_size) {
        if (left_size >= HIEDMAC_TRANS_MAXSIZE) {
            dma_size = HIEDMAC_TRANS_MAXSIZE;
        } else {
            dma_size = left_size;
        }
        dmac_start_m2m(ulchnn,
                       source + dma_count * dma_size,
                       dest + dma_count * dma_size,
                       dma_size);

        if (dmac_wait(ulchnn) != DMAC_CHN_SUCCESS) {
            hiedmacv310_error("dma transfer error...\n");
            return -1;
        }
        left_size -= dma_size;
        dma_count++;
        hiedmacv310_trace(4, "left_size is %d.\n", left_size);
    }

    return 0;
}
EXPORT_SYMBOL(dmac_m2m_transfer);


/*
 * memory to memory dma transfer with LLI
 *
 * @source
 * @dest
 * @length
 * */
int do_dma_llim2m(unsigned long source,
                  unsigned long dest,
                  unsigned long length)
{
    int ret = 0;
    unsigned chnn;

    chnn = dmac_channel_allocate();
    if (chnn < 0) {
        ret = -1;
        goto end;
    }
    hiedmacv310_trace(4, "chnn:%d,src:%lx,dst:%lx,len:%ld.\n", chnn, source, dest, length);

    if (pllihead[0] == 0) {
        hiedmacv310_error("ppheadlli[0] is NULL.\n");
        ret = -ENOMEM;
        goto end;
    }

    ret = dmac_buildllim2m(pllihead, source, dest, length, HIEDMAC_TRANS_MAXSIZE);
    if (ret) {
        hiedmacv310_error("build lli error...\n");
        ret = -EIO;
        goto end;
    }
    ret = dmac_start_llim2m(chnn, pllihead);
    if (ret) {
        hiedmacv310_error("start lli error...\n");
        ret = -EIO;
        goto end;
    }

end:
    return ret;
}
EXPORT_SYMBOL(do_dma_llim2m);

/*
 *  alloc_dma_lli_space
 *  output:
 *             ppheadlli[0]: memory physics address
 *             ppheadlli[1]: virtual address
 *
 */
int allocate_dmalli_space(struct device *dev, unsigned long *ppheadlli,
                          unsigned int page_num)
{
    dma_addr_t dma_phys;
    void *dma_virt;

    dma_virt = dma_alloc_coherent(dev, page_num * PAGE_SIZE,
                                  &dma_phys, GFP_DMA);
    if (dma_virt == NULL) {
        hiedmacv310_error("can't get dma mem from system\n");
        return -1;
    }

    ppheadlli[0] = (unsigned long)(dma_phys);
    ppheadlli[1] = (unsigned long)(dma_virt);

    if (dma_phys & (HIEDMAC_LLI_ALIGN - 1)) {
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(allocate_dmalli_space);



static int hiedmac_priv_init(struct hiedmac_host *hiedmac,
                             edmac_peripheral* peripheral_info)
{
    struct regmap *misc = hiedmac->misc_regmap;
    int i = 0;
    unsigned int count = 0;
    unsigned int offset = 0;
    unsigned ctrl = 0;

    for (i = 0; i < EDMAC_MAX_PERIPHERALS; i++) {
        if (peripheral_info[i].host_sel == hiedmac->id) {
            if (count > 32) {
                hiedmacv310_error("request table is not true!\n");
                return -1;
            }
            if (misc != NULL) {
                offset = hiedmac->misc_ctrl_base + (count & (~0x3));
                regmap_read(misc, offset, &ctrl);
                ctrl &= ~(0x3f << ((count & 0x3) << 3));
                ctrl |= peripheral_info[i].peri_id << ((count & 0x3) << 3);
                regmap_write(misc, offset, ctrl);
            }
            peripheral_info[i].dynamic_periphery_num = count;
            count++;
        }
    }

    return 0;
}

static int get_of_probe(struct hiedmac_host *hiedmac)
{
    struct resource *res;
    struct platform_device *platdev = hiedmac->pdev;
    struct device_node *np = platdev->dev.of_node;
    int ret;

    ret = of_property_read_u32((&platdev->dev)->of_node,
                               "devid", &(hiedmac->id));
    if (ret) {
        hiedmacv310_error("get hiedmac id fail\n");
        return -ENODEV;
    }

    hiedmac->clk = devm_clk_get(&(platdev->dev), "apb_pclk");
    if (IS_ERR(hiedmac->clk)) {
        hiedmacv310_error("get hiedmac clk fail\n");
        return PTR_ERR(hiedmac->clk);
    }

    hiedmac->axi_clk = devm_clk_get(&(platdev->dev), "axi_aclk");
    if (IS_ERR(hiedmac->axi_clk)) {
        hiedmacv310_error("get hiedmac axi clk fail\n");
        return PTR_ERR(hiedmac->axi_clk);
    }

    hiedmac->rstc = devm_reset_control_get(&(platdev->dev), "dma-reset");
    if (IS_ERR(hiedmac->rstc)) {
        hiedmacv310_error("get hiedmac rstc fail\n");
        return PTR_ERR(hiedmac->rstc);
    }

    res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
    if (!res) {
        hiedmacv310_error("no reg resource\n");
        return -ENODEV;
    }

    hiedmac->base = devm_ioremap_resource(&(platdev->dev), res);
    if (IS_ERR(hiedmac->base)) {
        hiedmacv310_error("get hiedmac base fail\n");
        return PTR_ERR(hiedmac->base);
    }
#if defined(CONFIG_ARCH_HI3516CV500) || defined(CONFIG_ARCH_HI3516DV300) || \
    defined(CONFIG_ARCH_HI3556V200)  || defined(CONFIG_ARCH_HI3559V200)  || \
    defined(CONFIG_ARCH_HI3516EV200) || defined(CONFIG_ARCH_HI3516EV300) || \
    defined(CONFIG_ARCH_HI3518EV300) || defined(CONFIG_ARCH_HI3516DV200)
    hiedmac->misc_regmap = 0;
    np = np ;
#else
    hiedmac->misc_regmap = syscon_regmap_lookup_by_phandle(np, "misc_regmap");
    if (IS_ERR(hiedmac->misc_regmap)) {
        hiedmacv310_error("get hiedmac misc fail\n");
        return PTR_ERR(hiedmac->misc_regmap);
    }

    ret = of_property_read_u32((&platdev->dev)->of_node,
                               "misc_ctrl_base", &(hiedmac->misc_ctrl_base));
    if (ret) {
        hiedmacv310_error( "get dma-misc_ctrl_base fail\n");
        return -ENODEV;
    }
#endif
    hiedmac->irq = platform_get_irq(platdev, 0);
    if (unlikely(hiedmac->irq < 0)) {
        return -ENODEV;
    }

    ret = of_property_read_u32((&platdev->dev)->of_node,
                               "dma-channels", &(hiedmac->channels));
    if (ret) {
        hiedmacv310_error( "get dma-channels fail\n");
        return -ENODEV;
    }
    ret = of_property_read_u32((&platdev->dev)->of_node,
                               "dma-requests", &(hiedmac->slave_requests));
    if (ret) {
        hiedmacv310_error( "get dma-requests fail\n");
        return -ENODEV;
    }
    hiedmacv310_trace(2, "dma-channels = %d, dma-requests = %d\n",
                      hiedmac->channels, hiedmac->slave_requests);

    hiedmac_priv_init(hiedmac, (edmac_peripheral*)&g_peripheral);

    return 0;
}

/* Don't need irq mode now */
#if 0
static irqreturn_t hiemdacv310_irq(int irq, void *dev)
{
    struct hiedmac_host* hiedmac = (struct hiedmac_host*)dev;
    unsigned int channel_err_status[3];
    unsigned int channel_tc_status = 0;
    unsigned int channel_status = 0;
    unsigned int temp = 0;
    int i = 0;
    unsigned int mask = 0;

    channel_status = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_STAT);
    if (!channel_status) {
        hiedmacv310_error("channel_status = 0x%x\n", channel_status);
        return IRQ_NONE;
    }

    for (i = 0; i < hiedmac->channels; i++) {
        temp = (channel_status >> i) & 0x1;
        if (temp) {
            channel_tc_status = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_TC1_RAW);
            channel_tc_status = (channel_tc_status >> i) & 0x01;
            if (channel_tc_status) {
                hiedmacv310_writel(channel_tc_status << i, hiedmac->base + HIEDMAC_INT_TC1_RAW);
            }

            channel_tc_status = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_TC2);
            channel_tc_status = (channel_tc_status >> i) & 0x01;
            if (channel_tc_status) {
                hiedmacv310_writel(channel_tc_status << i, hiedmac->base + HIEDMAC_INT_TC2_RAW);
            }

            channel_err_status[0] = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_ERR1);
            channel_err_status[0] = (channel_err_status[0] >> i) & 0x01;
            channel_err_status[1] = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_ERR2);
            channel_err_status[1] = (channel_err_status[1] >> i) & 0x01;
            channel_err_status[2] = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_ERR3);
            channel_err_status[2] = (channel_err_status[2] >> i) & 0x01;

            if (channel_err_status[0] | channel_err_status[1] | channel_err_status[2]) {
                hiedmacv310_error("Error in hiedmac %d finish!,ERR1 = 0x%x,ERR2 = 0x%x,ERR3 = 0x%x\n",
                                  i, channel_err_status[0], channel_err_status[1], channel_err_status[2]);
                hiedmacv310_writel(1 << i, hiedmac->base + HIEDMAC_INT_ERR1_RAW);
                hiedmacv310_writel(1 << i, hiedmac->base + HIEDMAC_INT_ERR2_RAW);
                hiedmacv310_writel(1 << i, hiedmac->base + HIEDMAC_INT_ERR3_RAW);
            }
            if ((function[i]) != NULL) {
                function[i](i, g_channel_status[i]);
            }

            mask |= (1 << i);
        }
    }

    return mask ? IRQ_HANDLED : IRQ_NONE;
}
#endif

static int __init hiedmacv310_probe(struct platform_device *pdev)
{
    int ret = 0;
    int i = 0;
    struct hiedmac_host *hiedmac = NULL;

    hiedmac = kzalloc(sizeof(*hiedmac), GFP_KERNEL);
    if (!hiedmac) {
        hiedmacv310_error("malloc for hiedmac fail!");
        ret = -ENOMEM;
        return ret;
    }
    hiedmac->pdev = pdev;

    ret = get_of_probe(hiedmac);
    if (ret) {
        hiedmacv310_error("get dts info fail!");
        goto free_hiedmac;
    }

    clk_prepare_enable(hiedmac->clk);
    clk_prepare_enable(hiedmac->axi_clk);

    reset_control_deassert(hiedmac->rstc);

    hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_TC1_RAW);
    hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_TC2_RAW);
    hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_ERR1_RAW);
    hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_ERR2_RAW);
    hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_ERR3_RAW);

    hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN, hiedmac->base + HIEDMAC_INT_TC1_MASK);
    hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN, hiedmac->base + HIEDMAC_INT_TC2_MASK);
    hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN, hiedmac->base + HIEDMAC_INT_ERR1_MASK);
    hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN, hiedmac->base + HIEDMAC_INT_ERR2_MASK);
    hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN, hiedmac->base + HIEDMAC_INT_ERR3_MASK);

    for (i = 0; i < HIEDMAC_CHANNEL_NUM; i++) {
        g_channel_status[i] = DMAC_CHN_VACANCY;
    }

    dma_regbase = hiedmac->base;

    ret = allocate_dmalli_space(&(hiedmac->pdev->dev), pllihead, HIEDMAC_LLI_PAGE_NUM);
    if (ret < 0) {
        goto free_hiedmac;
    }

#if 0
    /* register irq if necessary ! */
    ret = request_irq(hiedmac->irq, hiemdacv310_irq, 0, DRIVER_NAME, hiedmac);
    if (ret) {
        hiedmacv310_error("fail to request irq");
        goto free_hiedmac;
    }
#endif
    return 0;

free_hiedmac:
    kfree(hiedmac);

    return ret;
}


static int hiemda_remove(struct platform_device *pdev)
{
    int err = 0;
    return err;
}


static const struct of_device_id hiedmacv310_match[] = {
    { .compatible = "hisilicon,hiedmacv310_n" },
    {},
};


static struct platform_driver hiedmacv310_driver = {
    .remove = hiemda_remove,
    .driver = {
        .name   = "hiedmacv310_n",
        .of_match_table = hiedmacv310_match,
    },
};

static int __init hiedmacv310_init(void)
{
    return platform_driver_probe(&hiedmacv310_driver, hiedmacv310_probe);
}
subsys_initcall(hiedmacv310_init);

static void __exit hiedmacv310_exit(void)
{
    platform_driver_unregister(&hiedmacv310_driver);
}
module_exit(hiedmacv310_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon");
