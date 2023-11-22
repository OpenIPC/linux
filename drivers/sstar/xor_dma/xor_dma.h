/*
 * xor_dma.h- Sigmastar
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

#ifndef __XOR_DMA_H__
#define __XOR_DMA_H__

#include <linux/dmaengine.h>
#include "virt-dma.h"

#define SSTAR_DMA_SRC_BUF_NUM 32
#define SSTAR_DMA_BUF_GRP_NUM 1
#define SSTAR_DMA_MAX_CHANNEL 1
#define SSTAR_DMA_MAX_XOR     32
#define SSTAR_DMA_TIMEOUT     3000
// XORDMA MODE
#define XOR_MODE_SEL_XOR  (0x0000)
#define XOR_MODE_SEL_XNOR (0x0002)
#define XOR_MODE_SEL_AND  (0x0004)
#define XOR_MODE_SEL_OR   (0x0006)
#define XOR_CHN_MODE_32   (0x0000)
#define XOR_CHN_MODE_16   (0x0010)
#define XOR_CHN_MODE_8    (0x0020)
#define XOR_CHN_MODE_4    (0x0030)
// XORDMA REG
#define REG_DMA_XOR_CONF    (0x0000)
#define XOR_MODE_MASK       (0x0001)
#define XOR_SEL_MASK        (0x0006)
#define XOR_CHN_MODE_MASK   (0x0030)
#define XOR_SRC_NUM_MASK    (0x1F00)
#define XOR_SRC_GROUP_MASK  (0xE000)
#define REG_XOR_SRC_ADDR0_L (0x0001)
#define REG_XOR_SRC_ADDR0_H (0x0002)
#define REG_XOR_DST_ADDR0_L (0x0041)
#define REG_XOR_DST_ADDR0_H (0x0042)
#define REG_XOR_WAIT        (0x0051)
#define XOR_WAIT_MASK       (0x0003)
#define REG_XOR_SRC_NUM     (0x0052)
#define XOR_FIRST_NUM_MASK  (0x001F)
#define XOR_SECOND_NUM_MASK (0x1F00)
// BDMA SEL
#define BDMA_SRC_MIU_IMI_CH0       (0x0000)
#define BDMA_SRC_MIU_IMI_CH1       (0x0001)
#define BDMA_SRC_MEM_FILL          (0x0004)
#define BDMA_SRC_SPI               (0x0005)
#define BDMA_SRC_TSP_SRAM          (0x000A)
#define BDMA_DATA_SRC_WIDTH_1BYTE  (0x0000)
#define BDMA_DATA_SRC_WIDTH_2BYTE  (0x0010)
#define BDMA_DATA_SRC_WIDTH_4BYTE  (0x0020)
#define BDMA_DATA_SRC_WIDTH_8BYTE  (0x0030)
#define BDMA_DATA_SRC_WIDTH_16BYTE (0x0040)
#define BDMA_DST_MIU_IMI_CH0       (0x0000)
#define BDMA_DST_MIU_IMI_CH1       (0x0100)
#define BDMA_DST_PM51              (0x0600)
#define BDMA_DST_SEC51             (0x0900)
#define BDMA_DST_TSP_SRAM          (0x0A00)
#define BDMA_SDT_FSP               (0x0B00)
#define BDMA_DATA_DST_WIDTH_1BYTE  (0x0000)
#define BDMA_DATA_DST_WIDTH_2BYTE  (0x1000)
#define BDMA_DATA_DST_WIDTH_4BYTE  (0x2000)
#define BDMA_DATA_DST_WIDTH_8BYTE  (0x3000)
#define BDMA_DATA_DST_WIDTH_16BYTE (0x4000)
// BDMA REG
#define REG_BDMA_CFG          (0x0000)
#define BDMA_TRIG_MASK        (0x0001)
#define BDMA_STOP_MASK        (0x0010)
#define BDMA_SRC_TLB_MASK     (0x0100)
#define BDMA_DST_TLB_MASK     (0x0200)
#define BDMA_CLK_GATED_MASK   (0x0400)
#define REG_BDMA_STATE        (0x0001)
#define BDMA_QUEUED_MASK      (0x0001)
#define BDMA_BUSY_MASK        (0x0002)
#define BDMA_INT_MASK         (0x0004)
#define BDMA_DONE_MASK        (0x0008)
#define BDMA_RESULT_MASK      (0x0010)
#define BDMA_NS_MASK          (0x00E0)
#define REG_BDMA_SEL          (0x0002)
#define BDMA_SRC_SEL_MASK     (0x000F)
#define BDMA_SRC_DW_MASK      (0x0070)
#define BDMA_DST_SEL_MASK     (0x0F00)
#define BDMA_DST_DW_MASK      (0x7000)
#define REG_BDMA_CTRL         (0x0003)
#define BDMA_INT_EN_MASK      (0x0002)
#define BDMA_CFG_MASK         (0x00F0)
#define BDMA_FLUSH_MASK       (0x0F00)
#define BDMA_MIU_REP_MASK     (0xF000)
#define REG_BDMA_SRC_ADDR_L   (0x0004)
#define REG_BDMA_SRC_ADDR_H   (0x0005)
#define REG_BDMA_DST_ADDR_L   (0x0006)
#define REG_BDMA_DST_ADDR_H   (0x0007)
#define REG_BDMA_SIZE_L       (0x0008)
#define REG_BDMA_SIZE_H       (0x0009)
#define REG_BDMA_CMD0_L       (0x000A)
#define REG_BDMA_CMD0_H       (0x000B)
#define REG_BDMA_CMD1_L       (0x000C)
#define REG_BDMA_CMD1_H       (0x000D)
#define REG_BDMA_CMD2_L       (0x000E)
#define REG_BDMA_CMD2_H       (0x000F)
#define REG_BDMA_FUNC_EN      (0x0010)
#define BDMA_OFFEST_EN_MASK   (0x0001)
#define BDMA_SPACE_TRG_MASK   (0x0020)
#define REG_BDMA_SRC_WIDTH_L  (0x0012)
#define REG_BDMA_SRC_WIDTH_H  (0x0013)
#define REG_BDMA_SRC_OFFEST_L (0x0014)
#define REG_BDMA_SRC_OFFEST_H (0x0015)
#define REG_BDMA_DST_WIDTH_L  (0x0016)
#define REG_BDMA_DST_WIDTH_H  (0x0017)
#define REG_BDMA_DST_OFFEST_L (0x0018)
#define REG_BDMA_DST_OFFEST_H (0x0019)
#define REG_BDMA_SRC_ADDR_MSB (0x001A)
#define REG_BDMA_DST_ADDR_MSB (0x001B)

#define DMA_DBG                0
#define READ_WORD(_reg)        (*(volatile u16 *)(_reg))
#define WRITE_WORD(_reg, _val) (*((volatile u16 *)(_reg))) = (u16)(_val)
#define WRITE_WORD_MASK(_reg, _val, _mask) \
    (*((volatile u16 *)(_reg))) = ((*((volatile u16 *)(_reg))) & ~(_mask)) | ((u16)(_val) & (_mask))

#define XOR_READ(_reg_)                    READ_WORD(ch->xor_base + ((_reg_) << 2))
#define XOR_WRITE(_reg_, _val_)            WRITE_WORD(ch->xor_base + ((_reg_) << 2), (_val_))
#define XOR_WRITE_MASK(_reg_, _val_, mask) WRITE_WORD_MASK(ch->xor_base + ((_reg_) << 2), (_val_), (mask))

#define BDMA_READ(_reg_)                    READ_WORD(ch->bdma_base + ((_reg_) << 2))
#define BDMA_WRITE(_reg_, _val_)            WRITE_WORD(ch->bdma_base + ((_reg_) << 2), (_val_))
#define BDMA_WRITE_MASK(_reg_, _val_, mask) WRITE_WORD_MASK(ch->bdma_base + ((_reg_) << 2), (_val_), (mask))

#define dma_err(fmt, ...)  printk(KERN_ERR "[DMA] error : " fmt, ##__VA_ARGS__)
#define dma_warn(fmt, ...) printk(KERN_ERR "[DMA] warning : " fmt, ##__VA_ARGS__)
#if DMA_DBG == 1
#define dma_info(args...) printk("[DMA] : " args)
#else
#define dma_info(args...) \
    do                    \
    {                     \
    } while (0)
#endif

typedef enum
{
    Chn_mode_32 = 0,
    Chn_mode_16,
    Chn_mode_8,
    Chn_mode_4
} XOR_chn_mode;

struct sstar_async_desc
{
    enum dma_transaction_type dma_type;
    phys_addr_t               src_buf_addr[SSTAR_DMA_SRC_BUF_NUM];
    phys_addr_t               dst_buf_addr[SSTAR_DMA_BUF_GRP_NUM];
    u32                       buf_len;
    u8                        src_num;
    u8                        dst_num;
    struct virt_dma_desc      vd;
};

struct sstar_chan
{
    bool                     allocatd;
    int                      ch_num;
    u32                      xor_base;
    u32                      bdma_base;
    struct mutex             ch_lock;
    struct list_head         vdescs;
    struct completion        done;
    struct virt_dma_chan     vc;
    struct dma_slave_config  cfg;
    struct task_struct *     dma_thread_st;
    struct sstar_dmadev *    sstar_dmadev;
    struct sstar_async_desc *async_desc;
};
struct sstar_dmadev
{
    int               irq;
    struct clk *      clk;
    XOR_chn_mode      chn_mode;
    struct dma_device dma_dev;
    struct sstar_chan sstar_chans[SSTAR_DMA_MAX_CHANNEL];
};

static inline struct sstar_chan *to_sstar_dma_chan(struct dma_chan *chan)
{
    return container_of(chan, struct sstar_chan, vc.chan);
}

static inline struct sstar_async_desc *to_sstar_dma_desc(struct dma_async_tx_descriptor *t)
{
    return container_of(t, struct sstar_async_desc, vd.tx);
}

#endif
