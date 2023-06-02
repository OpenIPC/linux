/*
 * Copyright (C) 2010 Basler Vision Technologies AG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MACH_DAVINCI_SPI_H
#define __MACH_DAVINCI_SPI_H
#include <linux/platform_data/spi-davinci.h>

struct davinci_spi_unit_desc {
	unsigned int				spi_hwunit;
	unsigned int				chipsel;
	unsigned int				irq;
	unsigned int				dma_tx_chan;
	unsigned int				dma_rx_chan;
	unsigned int				dma_evtq;
	struct davinci_spi_platform_data	pdata;
};

struct spi_board_info;

void __init davinci_init_spi(struct davinci_spi_unit_desc *unit,
			     unsigned int ninfo,
			     const struct spi_board_info *info);


#endif /* __MACH_DAVINCI_SPI_H */
