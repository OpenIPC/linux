/*
 * xor_dma_test.c- Sigmastar
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
#include <linux/msi.h>
#include <linux/async_tx.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/random.h>
#include "xor_dma.h"

#define XOR_TEST
#define MEMCPY_TEST

#define src_buf_size  PAGE_SIZE
#define src_buf_num   64
#define dst_buf_size  PAGE_SIZE
#define dst_buf_order 0
#define dst_buf_num   (1 << dst_buf_order)

static addr_conv_t  addr_conv[src_buf_num];
static struct page *src[src_buf_num];
static struct page *dst;
#ifdef XOR_TEST
static u8 dst_tmp[dst_buf_size];
#endif
static void callback(void *param)
{
    struct completion *cmp = param;
    //    printk("%s>>> \n", __func__);
    complete(cmp);
}
#ifdef XOR_TEST
static int sstar_dma_xor_test(void)
{
    int                             i, j;
    int                             err_num = 0;
    int                             timeout = 512;
    u64                             time;
    struct timespec64               tv1, tv2;
    struct async_submit_ctl         submit;
    struct completion               cmp;
    struct dma_async_tx_descriptor *tx;
    char *                          src_buff[src_buf_num];
    char *                          dst_buff;
    for (i = 0; i < src_buf_num; i++)
    {
        src[i] = alloc_page(GFP_KERNEL);
        if (!src[i])
        {
            while (i--)
                put_page(src[i]);
            printk("alloc_page failed\n");
            return -ENOMEM;
        }
        get_random_bytes(page_address(src[i]), PAGE_SIZE);
        src_buff[i] = (char *)page_address(src[i]);
        printk("src_buf %d addr is 0x%px\n", i, page_address(src[i]));
    }
    dst      = alloc_pages(GFP_KERNEL, dst_buf_order);
    dst_buff = (char *)page_address(dst);
    printk("dst_buf addr is 0x%px\n", dst_buff);
    init_completion(&cmp);
    ktime_get_ts64(&tv1);
    while (timeout--)
    {
        init_async_submit(&submit, ASYNC_TX_XOR_ZERO_DST, NULL, callback, &cmp, addr_conv);
        tx = async_xor(dst, src, 0, src_buf_num, PAGE_SIZE, &submit);
        async_tx_issue_pending(tx);
        if (wait_for_completion_timeout(&cmp, msecs_to_jiffies(3000000)) == 0)
        {
            printk("async_xor time_out\n");
        }
    }
    ktime_get_ts64(&tv2);
    time += ((tv2.tv_sec - tv1.tv_sec) * 1000000);
    time += ((tv2.tv_nsec - tv1.tv_nsec) / 1000);
    printk("cal 1GB xor cost %lld us\n", time);
    memset((void *)dst_tmp, 0x00, dst_buf_size);
    for (i = 0; i < src_buf_num; i++)
    {
        for (j = 0; j < dst_buf_size; j++)
        {
            dst_tmp[j] ^= src_buff[i][j];
        }
    }
    for (i = 0; i < dst_buf_size; i++)
    {
        if (dst_tmp[i] != dst_buff[i])
            err_num++;
    }
    if (err_num)
    {
        printk("async_xor result err\n");
    }
    else
    {
        printk("async_xor result pass\n");
    }
    return 0;
}
#endif
#ifdef MEMCPY_TEST
static int sstar_dma_memecpy_test(void)
{
    int                             i;
    int                             err_num = 0;
    struct async_submit_ctl         submit;
    struct completion               cmp;
    struct dma_async_tx_descriptor *tx;
    char *                          src_buff[1];
    char *                          dst_buff;
    for (i = 0; i < 1; i++)
    {
        src[i] = alloc_page(GFP_KERNEL);
        if (!src[i])
        {
            while (i--)
                put_page(src[i]);
            printk("alloc_page failed\n");
            return -ENOMEM;
        }
        get_random_bytes(page_address(src[i]), PAGE_SIZE);
        src_buff[i] = (char *)page_address(src[i]);
        printk("src_buf %d addr is 0x%px\n", i, page_address(src[i]));
    }
    dst      = alloc_pages(GFP_KERNEL, dst_buf_order);
    dst_buff = (char *)page_address(dst);
    printk("dst_buf addr is 0x%px\n", dst_buff);
    init_completion(&cmp);
    init_async_submit(&submit, 0, NULL, callback, &cmp, addr_conv);
    tx = async_memcpy(dst, src[0], 0, 0, PAGE_SIZE, &submit);
    async_tx_issue_pending(tx);
    if (wait_for_completion_timeout(&cmp, msecs_to_jiffies(300)) == 0)
    {
        printk("async_memcpy time_out\n");
    }
    for (i = 0; i < dst_buf_size; i++)
    {
        if (src_buff[0][i] != dst_buff[i])
            err_num++;
    }
    if (err_num)
    {
        printk("async_memcpy result err\n");
    }
    else
    {
        printk("async_memcpy result pass\n");
    }
    return 0;
}
#endif
static int dma_test(void)
{
#ifdef MEMCPY_TEST
    sstar_dma_memecpy_test();
#endif
#ifdef XOR_TEST
    sstar_dma_xor_test();
#endif
    return 0;
}
static void sstar_test_exit(void) {}

late_initcall(dma_test);
module_exit(sstar_test_exit);
MODULE_DESCRIPTION("SSTAR DMA engine driver test");
MODULE_LICENSE("GPL v2");
