#ifndef __FH_SPI_PLAT_H__
#define __FH_SPI_PLAT_H__

#include <linux/io.h>
#include <linux/scatterlist.h>

/****************************************************************************
* #define section
*	add constant #define here if any
***************************************************************************/
#define SPI_MASTER_CONTROLLER_MAX_SLAVE		2
#define SPI_TRANSFER_USE_DMA			(0x77888877)
#define SPI_DMA_PROTCTL_ENABLE			(0x55)
#define SPI_DMA_MASTER_SEL_ENABLE		(0x55)
/****************************************************************************
* ADT section
*	add Abstract Data Type definition here
***************************************************************************/

struct fh_spi_cs {
	u32 GPIO_Pin;
	char *name;
};



#define ONE_WIRE_SUPPORT		(1<<0)
#define DUAL_WIRE_SUPPORT		(1<<1)
#define QUAD_WIRE_SUPPORT		(1<<2)
#define MULTI_WIRE_SUPPORT		(1<<8)

struct fh_spi_platform_data {
	u32 apb_clock_in;
	u32 slave_max_num;
	u32 clock_source[8];
	u32 clock_source_num;
	u32 clock_in_use;
	struct fh_spi_cs cs_data[SPI_MASTER_CONTROLLER_MAX_SLAVE];
	u32 bus_no;
	char *clk_name;
	char *hclk_name;
	char *pclk_name;
	/* add support wire width*/
	u32 ctl_wire_support;
	/* add dma para*/
	/*use dma, but need menuconfig open*/
	u32 dma_transfer_enable;
	/*related soc*/
	u32 rx_handshake_num;
	u32 dma_protctl_enable;
	u32 rx_dma_channel;
	u32 dma_protctl_data;
	u32 dma_master_sel_enable;
	u32 dma_master_ctl_sel;
	u32 dma_master_mem_sel;
};


#endif
