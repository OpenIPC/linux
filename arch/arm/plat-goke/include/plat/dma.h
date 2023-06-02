/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/plat/dma.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __PLAT_DMA_H
#define __PLAT_DMA_H

#include <mach/dma.h>

/* ==========================================================================*/
#define MAX_DMA_CHANNEL_IRQ_HANDLERS    4

/* ==========================================================================*/
#ifndef __ASSEMBLER__

typedef struct gk_dma_req_s {
    u32 src;        /*  Source address */
    u32 dst;        /* Destination address */
    struct gk_dma_req_s *next; /* Pointing to next descriptor */
    u32 rpt;        /* The physical address to store DMA hardware
                    reporting status */
    u32 xfr_count;        /* Transfer byte count , max value = 2^22 */
    u32 attr;        /* Descriptor 's attribute */
} gk_dma_req_t;

typedef struct gk_dmadesc_s {
    u32    src_addr;    /**< Source address */
    u32    dst_addr;    /**< Destination address */
    u32    next;        /**< Next descriptor */
    u32    rpt_addr;    /**< Report address */
    u32    xfrcnt;        /**< Transfer count */
    u32    ctrl;        /**< Control */
    u32    rsv0;        /**< Reserved */
    u32    rsv1;        /**< Reserved */
    u32    rpt;        /**< Report */
    u32    rsv2;        /**< Reserved */
    u32    rsv3;        /**< Reserved */
    u32    rsv4;        /**< Reserved */
} gk_dmadesc_t;

typedef void (*gk_dma_handler)(void *dev_id, u32 status);

struct dma_s {
    struct dma_chan_s {
        u32 status;    /**< The status of the current transaction */
        int use_flag;
        int irq_count;
        struct {
            int enabled;
            gk_dma_handler handler;
            void *harg;
        } irq[MAX_DMA_CHANNEL_IRQ_HANDLERS];
    } chan[NUM_DMA_CHANNELS];
};

/* ==========================================================================*/

/* ==========================================================================*/
extern int gk_dma_request_irq(int chan, gk_dma_handler handler, void *harg);
extern void gk_dma_free_irq(int chan, gk_dma_handler handler);
extern int gk_dma_enable_irq(int chan, gk_dma_handler handler);
extern int gk_dma_disable_irq(int chan, gk_dma_handler handler);
extern int gk_dma_xfr(gk_dma_req_t *req, int chan);
extern int gk_dma_desc_xfr(dma_addr_t desc_addr, int chan);
extern int gk_dma_desc_stop(int chan);
extern int gk_dma_read_ctrl_reg(int chan);
extern int gk_init_dma(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

