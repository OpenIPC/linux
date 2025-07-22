/*
 *  Copyright (C) 2010 Ingenic Semiconductor Inc.
 *
 *  Author: <zpzhong@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * This file is a part of generic dmaengine, it's
 * used for other device to use dmaengine.
 */

#ifndef __MACH_JZDMA_H__
#define __MACH_JZDMA_H__

#include <linux/interrupt.h>
#include <linux/dmaengine.h>

#define NR_DMA_CHANNELS 32

#define CH_DSA  0x00
#define CH_DTA  0x04
#define CH_DTC  0x08
#define CH_DRT  0x0C
#define CH_DCS  0x10
#define CH_DCM  0x14
#define CH_DDA  0x18
#define CH_DSD  0x1C

#define TCSM    0x2000

#define DMAC    0x1000
#define DIRQP   0x1004
#define DESIRQP 0x1010
#define DIC     0x1014
#define DDR 0x1008
#define DDRS    0x100C
#define DMACP   0x101C
#define DSIRQP  0x1020
#define DSIRQM  0x1024
#define DCIRQP  0x1028
#define DCIRQM  0x102C

/* MCU of PDMA */
#define DMCS    0x1030
#define DMNMB   0x1034
#define DMSMB   0x1038
#define DMINT   0x103C

/* MCU of PDMA */
#define DMINT_S_IP      BIT(17)
#define DMINT_N_IP      BIT(16)

#define DMAC_HLT    BIT(3)
#define DMAC_AR     BIT(2)

#define DCS_NDES    BIT(31)
#define DCS_AR      BIT(4)
#define DCS_TT      BIT(3)
#define DCS_HLT     BIT(2)
#define DCS_CTE     BIT(0)
#define DCS_CDOA_SHF    (8)
#define DCS_CDOA_MASK   (0xff << DCS_CDOA_SHF)

#define DCM_SAI     BIT(23)
#define DCM_DAI     BIT(22)
#define DCM_RDIL_MSK (0x7 << 16)
#define DCM_RDIL_SHF 16
#define DCM_SP_MSK  (0x3 << 14)
#define DCM_SP_32   DCM_SP_MSK
#define DCM_SP_16   BIT(15)
#define DCM_SP_8    BIT(14)
#define DCM_DP_MSK  (0x3 << 12)
#define DCM_DP_32   DCM_DP_MSK
#define DCM_DP_16   BIT(13)
#define DCM_DP_8    BIT(12)
#define DCM_TSZ_MSK (0x7 << 8)
#define DCM_TSZ_SHF 8
#define DCM_STDE    BIT(2)
#define DCM_TIE     BIT(1)
#define DCM_LINK    BIT(0)

#define DCM_CH1_SRC_TCSM    (0x0 << 26)
#define DCM_CH1_SRC_NEMC    (0x1 << 26)
#define DCM_CH1_SRC_DDR     (0x2 << 26)

#define DCM_CH1_DST_TCSM    (0x0 << 24)
#define DCM_CH1_DST_NEMC    (0x1 << 24)
#define DCM_CH1_DST_DDR     (0x2 << 24)

#define DCM_CH1_DDR_TO_NAND  (DCM_CH1_SRC_DDR  | DCM_CH1_DST_NEMC)
#define DCM_CH1_NAND_TO_DDR  (DCM_CH1_SRC_NEMC | DCM_CH1_DST_DDR)

#define DCM_CH1_TCSM_TO_NAND (DCM_CH1_SRC_TCSM | DCM_CH1_DST_NEMC)
#define DCM_CH1_NAND_TO_TCSM (DCM_CH1_SRC_NEMC | DCM_CH1_DST_TCSM)

#define DCM_CH1_TCSM_TO_DDR  (DCM_CH1_SRC_TCSM | DCM_CH1_DST_DDR)
#define DCM_CH1_DDR_TO_TCSM  (DCM_CH1_SRC_DDR  | DCM_CH1_DST_TCSM)

#define MCU_MSG_TYPE_NORMAL 0x1
#define MCU_MSG_TYPE_INTC   0x2
#define MCU_MSG_TYPE_INTC_MASKA 0x3

enum jzdma_req_type {
#define _RTP(NAME) JZDMA_REQ_##NAME##_TX,JZDMA_REQ_##NAME##_RX
    JZDMA_REQ_RESERVED0 = 0x03,
    _RTP(I2S1),
    _RTP(I2S0),
    JZDMA_REQ_AUTO_TXRX = 0x08,
    JZDMA_REQ_SADC_RX,
    JZDMA_REQ_RESERVED1 = 0x0b,
    _RTP(UART4),
    _RTP(UART3),
    _RTP(UART2),
    _RTP(UART1),
    _RTP(UART0),
    _RTP(SSI0),
    _RTP(SSI1),
    _RTP(MSC0),
    _RTP(MSC1),
    _RTP(MSC2),
    _RTP(PCM0),
    _RTP(PCM1),
    _RTP(I2C0),
    _RTP(I2C1),
    _RTP(I2C2),
    _RTP(I2C3),
    _RTP(I2C4),
    _RTP(DES),
#undef _RTP
};

enum jzdma_type {
    JZDMA_REQ_INVAL = 0,
#define _RTP(NAME) JZDMA_REQ_##NAME = JZDMA_REQ_##NAME##_TX
	_RTP(I2S1),
	_RTP(I2S0),
	JZDMA_REQ_AUTO = JZDMA_REQ_AUTO_TXRX,
	JZDMA_REQ_SADC = JZDMA_REQ_SADC_RX,
	_RTP(UART4),
	_RTP(UART3),
	_RTP(UART2),
    _RTP(UART1),
    _RTP(UART0),
    _RTP(SSI0),
    _RTP(SSI1),
    _RTP(MSC0),
    _RTP(MSC1),
    _RTP(MSC2),
    _RTP(PCM0),
    _RTP(PCM1),
    _RTP(I2C0),
    _RTP(I2C1),
    _RTP(I2C2),
    _RTP(I2C3),
    _RTP(I2C4),
    _RTP(DES),
    JZDMA_REQ_NAND0 = JZDMA_REQ_AUTO_TXRX | (1 << 16),
    JZDMA_REQ_NAND1 = JZDMA_REQ_AUTO_TXRX | (2 << 16),
    JZDMA_REQ_NAND2 = JZDMA_REQ_AUTO_TXRX | (3 << 16),
    JZDMA_REQ_NAND3 = JZDMA_REQ_AUTO_TXRX | (4 << 16),
    JZDMA_REQ_NAND4 = JZDMA_REQ_AUTO_TXRX | (5 << 16),
    TYPE_MASK = 0xffff,
#undef _RTP
};

#define GET_MAP_TYPE(type) (type & (TYPE_MASK))

struct jzdma_platform_data {
    int irq_base;
    int irq_end;
    enum jzdma_type map[NR_DMA_CHANNELS];
};


struct jzdma_master;

struct dma_desc {
    unsigned long dcm;
    dma_addr_t dsa;
    dma_addr_t dta;
    unsigned long dtc;
    unsigned long sd;
    unsigned long drt;
    unsigned long reserved[2];
};

struct jzdma_channel {
    int         id;
    int         residue;
    struct dma_chan     chan;
    enum jzdma_type     type;
    struct dma_async_tx_descriptor  tx_desc;
    dma_cookie_t        last_completed;
    dma_cookie_t        last_good;
    struct tasklet_struct   tasklet;
    struct tasklet_struct   link_tasklet;
    spinlock_t      lock;
#define CHFLG_SLAVE     BIT(0)
    unsigned short      flags;
    unsigned short      status;
    unsigned long       dcs_saved;
    struct dma_desc     *desc;
    dma_addr_t      desc_phys;
    unsigned short      desc_nr;
    unsigned short      desc_max;
    struct scatterlist  *sgl;
    unsigned long       sg_len;
    unsigned short      last_sg;
#define CYCLIC_ACTIVE (1 << 15)
#define CYCLIC_POSSIBLE (1 << 14)
    unsigned short      cyclic;
    unsigned long       tx_dcm_def;
    unsigned long       rx_dcm_def;
    struct dma_slave_config *config;
    void __iomem        *iomem;
    struct jzdma_master *master;
};

enum channel_status {
    STAT_STOPED,STAT_SUBED,STAT_PREPED,STAT_RUNNING,
};

struct jzdma_master {
    struct device       *dev;
    void __iomem        *iomem;
    struct clk      *clk;
    int         irq;
    int                     irq_pdmam;   /* irq_pdmam for PDMAM irq */
    struct dma_device   dma_device;
    enum jzdma_type     *map;
    struct irq_chip     irq_chip;
    struct jzdma_channel    channel[NR_DMA_CHANNELS];
};

static inline struct device *chan2dev(struct dma_chan *chan)
{
    return &chan->dev->device;
}

static inline struct jzdma_channel *to_jzdma_chan(struct dma_chan *chan)
{
    return container_of(chan, struct jzdma_channel, chan);
}

int jzdma_dump(struct dma_chan *chan);

#endif
