#ifndef	__FH_UART_PLAT_H__
#define	__FH_UART_PLAT_H__

#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/scatterlist.h>
#include <linux/completion.h>

struct fh_uart_dma_transfer {
	struct dma_chan *chan;
	struct dma_slave_config cfg;
	struct scatterlist sgl[128];
	unsigned int sgl_data_size[128];
	unsigned int actual_sgl_size;
	struct dma_async_tx_descriptor *desc;
};

struct fh_uart_dma {
	u32 tx_dumy_buff[128];
	u8 rx_dumy_buff[128];
	u32 tx_dma_add;
	u32 rx_dma_add;
	u32 tx_hs_no;
	u32 rx_hs_no;
	u32 tx_dma_channel;
	u32 rx_dma_channel;
	u32 tx_count;
	u32 tx_done;
	u32 paddr;
	u32 rx_xmit_len;
	u32 inited;
	struct completion rx_cmp;
	struct fh_uart_dma_transfer dma_rx;
	struct fh_uart_dma_transfer dma_tx;
};

struct fh_platform_uart {
	unsigned long		mapbase;	/* Physical address base */
	unsigned int		fifo_size;	/* UART FIFO SIZE */
	unsigned int		irq;		/* Interrupt vector */
	unsigned int		uartclk;	/* UART clock rate */
	unsigned int		use_dma;
	struct fh_uart_dma *dma_info;
};

#endif