#ifndef _SDP_REG_H
#define _SDP_REG_H

#include <rcw_macro.h>
//#include "DrvCommon.h"

#define SDP_DMA_LEN_MAX         ((1<<23)-1)


//#define SDP_CTRL_REG_OFS        0x00
REGDEF_OFFSET(SDP_CTRL_REG, 0x00)
REGDEF_BEGIN(SDP_CTRL_REG)      /* SDP Control Register */
REGDEF_BIT(dma_en, 1)           /* DMA transfer enable */
REGDEF_BIT(, 3)
REGDEF_BIT(transfer_mode, 1)	/* transfer mode for 03H command */
REGDEF_BIT(, 27)
REGDEF_END(SDP_CTRL_REG)        /* SDP Control Register end */

//#define SDP_STATUS_REG_OFS      0x08
REGDEF_OFFSET(SDP_STATUS_REG, 0x08)
REGDEF_BEGIN(SDP_STATUS_REG)    /* SDP Status Register */
REGDEF_BIT(fifo_loaded, 1)      /* FIFO loaded status  */
REGDEF_BIT(fifo_underrun, 1)    /* FIFO underrun status  */
REGDEF_BIT(dma_exhausted, 1)    /* DMA exhausted status */
REGDEF_BIT(dma_end, 1)          /* DMA end/complete status */
REGDEF_BIT(mrx_fifo_emptry, 1)  /* MRX FIFO empty */
REGDEF_BIT(mrx_fifo_full, 1)    /* MRX FIFO full */
REGDEF_BIT(dma_cmd_not_align, 1)/* DMA read command length not align */
REGDEF_BIT(, 25)
REGDEF_END(SDP_STATUS_REG)      /* SDP Status Register end */

//#define SDP_INTEN_REG_OFS       0x0C
REGDEF_OFFSET(SDP_INTEN_REG, 0x0C)
REGDEF_BEGIN(SDP_INTEN_REG)     /* SDP Interrup Enable Register */
REGDEF_BIT(fifo_loaded, 1)      /* FIFO loaded status  */
REGDEF_BIT(fifo_underrun, 1)    /* FIFO underrun status  */
REGDEF_BIT(dma_exhausted, 1)    /* DMA exhausted status */
REGDEF_BIT(dma_end, 1)          /* DMA end/complete status */
REGDEF_BIT(mrx_fifo_emptry, 1)  /* MRX FIFO empty */
REGDEF_BIT(mtx_fifo_full, 1)    /* MTX FIFO full */
REGDEF_BIT(dma_cmd_not_align, 1)/* DMA read command length not align */
REGDEF_BIT(, 25)
REGDEF_END(SDP_INTEN_REG)       /* SDP Interrupt Enable Register end */

//#define SDP_READ_STS_REG_OFS    0x10
REGDEF_OFFSET(SDP_READ_STS_REG, 0x10)
REGDEF_BEGIN(SDP_READ_STS_REG)  /* SDP Read Status Register */
REGDEF_BIT(fifo_full, 1)        /* FIFO full  */
REGDEF_BIT(fifo_underrun, 1)    /* FIFO underrun */
REGDEF_BIT(dma_exhuasted, 1)	/* DMA exhausted */
REGDEF_BIT(mrx_port_full, 1)    /* MRX port full */
REGDEF_BIT(mtx_port_full, 1)    /* MTX port full */
REGDEF_BIT(, 27)
REGDEF_END(SDP_READ_STS_REG)    /* SDP Read Status Register end */

//#define SDP_MASTER_RX_PORT_REG_OFS      0x14
REGDEF_OFFSET(SDP_MASTER_RX_PORT_REG, 0x14)
REGDEF_BEGIN(SDP_MASTER_RX_PORT_REG)    /* SDP Master Receive Port Register */
REGDEF_BIT(data_port, 32)               /* data port  */
REGDEF_END(SDP_MASTER_RX_PORT_REG)      /* SDP Master Receive Port Register */

//#define SDP_MASTER_TX_PORT_REG_OFS      0x18
REGDEF_OFFSET(SDP_MASTER_TX_PORT_REG, 0x18)
REGDEF_BEGIN(SDP_MASTER_TX_PORT_REG)    /* SDP Master Transmit Port Register */
REGDEF_BIT(data_port, 32)               /* data port  */
REGDEF_END(SDP_MASTER_TX_PORT_REG)      /* SDP Master Transmit Port Register */

//#define SDP_DMA_ADDR_REG_OFS            0x20
REGDEF_OFFSET(SDP_DMA_ADDR_REG, 0x20)
REGDEF_BEGIN(SDP_DMA_ADDR_REG)          /* SDP DMA Start Address Register */
REGDEF_BIT(addr, 32)                    /* address  */
REGDEF_END(SDP_DMA_ADDR_REG)            /* SDP DMA Start Address Register */

//#define SDP_DMA_SIZE_REG_OFS            0x24
REGDEF_OFFSET(SDP_DMA_SIZE_REG, 0x24)
REGDEF_BEGIN(SDP_DMA_SIZE_REG)          /* SDP DMA Buffer Size Register */
REGDEF_BIT(buf_size, 23)                /* DMA buffer size (unit: byte) */
REGDEF_BIT(, 9)
REGDEF_END(SDP_DMA_SIZE_REG)            /* SDP DMA Buffer Size Register */

//#define SDP_DMA_TRANSFERRED_REG_OFS     0x28
REGDEF_OFFSET(SDP_DMA_TRANSFERRED_REG, 0x28)
REGDEF_BEGIN(SDP_DMA_TRANSFERRED_REG)   /* SDP DMA Transferred Size Register */
REGDEF_BIT(tx_size, 23)                 /* transferred size before FIFO underrun */
REGDEF_BIT(, 9)
REGDEF_END(SDP_DMA_TRANSFERRED_REG)     /* SDP DMA Transferred Size Register */

REGDEF_OFFSET(SDP_RSTN_REG, 0x88)
REGDEF_BEGIN(SDP_RSTN_REG)
REGDEF_BIT(, 20)
REGDEF_BIT(rstn, 1)
REGDEF_BIT(, 11)
REGDEF_END(SDP_RSTN_REG)

#endif
