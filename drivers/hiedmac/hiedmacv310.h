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

#ifndef __HIEDMACV310_H__
#define __HIEDMACV310_H__

/* debug control */
extern int hiedmacv310_trace_level_n;
#define HIEDMACV310_TRACE_LEVEL 5

#define HIEDMACV310_TRACE_FMT KERN_INFO

#define DMAC_ERROR_BASE     0x64
#define DMAC_CHN_SUCCESS            (DMAC_ERROR_BASE + 0x10)
#define DMAC_CHN_ERROR              (DMAC_ERROR_BASE + 0x11)
#define DMAC_CHN_TIMEOUT            (DMAC_ERROR_BASE + 0x12)
#define DMAC_CHN_ALLOCAT            (DMAC_ERROR_BASE + 0x13)
#define DMAC_CHN_VACANCY            (DMAC_ERROR_BASE + 0x14)
#define DMAC_NOT_FINISHED           (DMAC_ERROR_BASE + 0xe)

typedef void DMAC_ISR(unsigned int channel, int status);

#define HIEDMAC_UPDATE_TIMEOUT  (30 * HZ)
#define HIEDMAC_TRANS_MAXSIZE     (64*1024-1)

#define DEBUG_HIEDMAC
#ifdef  DEBUG_HIEDMAC

#define hiedmacv310_trace(level, msg...) do { \
    if ((level) >= hiedmacv310_trace_level_n) { \
        printk(HIEDMACV310_TRACE_FMT"%s:%d: ", __func__, __LINE__); \
        printk(msg); \
        printk("\n"); \
    } \
} while (0)


#define hiedmacv310_assert(cond) do { \
    if (!(cond)) {\
        printk(KERN_ERR "Assert:hiedmacv310:%s:%d\n", \
                __func__, \
                __LINE__); \
        BUG(); \
    } \
} while (0)

#define hiedmacv310_error(s...) do { \
    printk(KERN_ERR "hiedmacv310:%s:%d: ", __func__, __LINE__); \
    printk(s); \
    printk("\n"); \
} while (0)

#else

#define hiedmacv310_trace(level, msg...) do { } while (0)
#define hiedmacv310_assert(level, msg...) do { } while (0)
#define hiedmacv310_error(level, msg...) do { } while (0)

#endif

#define hiedmacv310_readl(addr) ({unsigned int reg = readl((void *)(addr)); \
        reg; })

#define hiedmacv310_writel(v, addr) do { writel(v, (void *)(addr)); \
} while (0)


#define MAX_TRANSFER_BYTES  0xffff

/* reg offset */
#define HIEDMAC_INT_STAT                  (0x0)
#define HIEDMAC_INT_TC1                   (0x4)
#define HIEDMAC_INT_TC2                   (0x8)
#define HIEDMAC_INT_ERR1                  (0xc)
#define HIEDMAC_INT_ERR2                  (0x10)
#define HIEDMAC_INT_ERR3                  (0x14)
#define HIEDMAC_INT_TC1_MASK              (0x18)
#define HIEDMAC_INT_TC2_MASK              (0x1c)
#define HIEDMAC_INT_ERR1_MASK             (0x20)
#define HIEDMAC_INT_ERR2_MASK             (0x24)
#define HIEDMAC_INT_ERR3_MASK             (0x28)

#define HIEDMAC_INT_TC1_RAW               (0x600)
#define HIEDMAC_INT_TC2_RAW               (0x608)
#define HIEDMAC_INT_ERR1_RAW              (0x610)
#define HIEDMAC_INT_ERR2_RAW              (0x618)
#define HIEDMAC_INT_ERR3_RAW              (0x620)

#define HIEDMAC_Cx_CURR_CNT0(cn)          (0x404 + cn * 0x20)
#define HIEDMAC_Cx_CURR_SRC_ADDR_L(cn)    (0x408 + cn * 0x20)
#define HIEDMAC_Cx_CURR_SRC_ADDR_H(cn)    (0x40c + cn * 0x20)
#define HIEDMAC_Cx_CURR_DEST_ADDR_L(cn)    (0x410 + cn * 0x20)
#define HIEDMAC_Cx_CURR_DEST_ADDR_H(cn)    (0x414 + cn * 0x20)

#define HIEDMAC_CH_PRI                    (0x688)
#define HIEDMAC_CH_STAT                   (0x690)
#define HIEDMAC_DMA_CTRL                  (0x698)

#define HIEDMAC_Cx_BASE(cn)               (0x800 + cn * 0x40)
#define HIEDMAC_Cx_LLI_L(cn)              (0x800 + cn * 0x40)
#define HIEDMAC_Cx_LLI_H(cn)              (0x804 + cn * 0x40)
#define HIEDMAC_Cx_CNT0(cn)               (0x81c + cn * 0x40)
#define HIEDMAC_Cx_SRC_ADDR_L(cn)         (0x820 + cn * 0x40)
#define HIEDMAC_Cx_SRC_ADDR_H(cn)         (0x824 + cn * 0x40)
#define HIEDMAC_Cx_DEST_ADDR_L(cn)         (0x828 + cn * 0x40)
#define HIEDMAC_Cx_DEST_ADDR_H(cn)         (0x82c + cn * 0x40)
#define HIEDMAC_Cx_CONFIG(cn)             (0x830 + cn * 0x40)

#define HIEDMAC_CxCONFIG_M2M            0xCFF33000
#define HIEDMAC_CxCONFIG_M2M_LLI        0xCFF00000
#define HIEDMAC_CxCONFIG_CHN_START  0x1
#define HIEDMAC_Cx_DISABLE           0x0

#define HIEDMAC_ALL_CHAN_CLR        (0xff)
#define HIEDMAC_INT_ENABLE_ALL_CHAN (0xff)


#define HIEDMAC_CONFIG_SRC_INC          (1<<31)
#define HIEDMAC_CONFIG_DST_INC          (1<<30)

#define HIEDMAC_CONFIG_SRC_WIDTH_SHIFT  (16)
#define HIEDMAC_CONFIG_DST_WIDTH_SHIFT  (12)
#define HIEDMAC_WIDTH_8BIT              (0x0)
#define HIEDMAC_WIDTH_16BIT             (0x1)
#define HIEDMAC_WIDTH_32BIT             (0x10)
#define HIEDMAC_WIDTH_64BIT             (0x11)

#define HIEDMAC_MAX_BURST_WIDTH         (16)
#define HIEDMAC_MIN_BURST_WIDTH         (1)
#define HIEDMAC_CONFIG_SRC_BURST_SHIFT  (24)
#define HIEDMAC_CONFIG_DST_BURST_SHIFT  (20)

#define HIEDMAC_LLI_ALIGN   0x40
#define HIEDMAC_LLI_DISABLE 0x0
#define HIEDMAC_LLI_ENABLE 0x2

#define HIEDMAC_CXCONFIG_SIGNAL_SHIFT   (0x4)
#define HIEDMAC_CXCONFIG_MEM_TYPE       (0x0)
#define HIEDMAC_CXCONFIG_DEV_MEM_TYPE   (0x1)
#define HIEDMAC_CXCONFIG_TSF_TYPE_SHIFT (0x2)
#define HIEDMAC_CxCONFIG_LLI_START      (0x1)

#define HIEDMAC_CXCONFIG_ITC_EN     (0x1)
#define HIEDMAC_CXCONFIG_ITC_EN_SHIFT   (0x1)

#define CCFG_EN 0x1

/* DMAC peripheral structure */
typedef struct edmac_peripheral {
    /* peripherial ID */
    unsigned int peri_id;
    /* peripheral data register address */
    unsigned long peri_addr;
    /* config requset */
    int host_sel;
#define DMAC_HOST0 0
#define DMAC_HOST1 1
#define DMAC_NOT_USE (-1)
    /* default channel configuration word */
    unsigned int transfer_cfg;
    /* default channel configuration word */
    unsigned int transfer_width;
    unsigned int dynamic_periphery_num;
} edmac_peripheral;


#define PERI_ID_OFFSET  4
#define EDMA_SRC_WIDTH_OFFSET 16
#define EDMA_DST_WIDTH_OFFSET 12
#define EDMA_CH_ENABLE  1

#define PERI_8BIT_MODE  0
#define PERI_16BIT_MODE 1
#define PERI_32BIT_MODE 2
#define PERI_64BIT_MODE 3

#define HIEDMAC_LLI_PAGE_NUM 0x4 /* 4*4K*65535B/64≈16MB */
#endif
