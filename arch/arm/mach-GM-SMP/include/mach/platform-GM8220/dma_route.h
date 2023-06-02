/*
 * Copyright(c) 2004 - 2006 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 *
 * The full GNU General Public License is included in this distribution in the
 * file called COPYING.
 */
#ifndef __DMA_ROUTE_H__
#define __DMA_ROUTE_H__

/* define AHB DMA routing table 
 */
#define AHBDMA_REQ_SSP0_RX  0
#define AHBDMA_REQ_SSP0_TX  1
#define AHBDMA_REQ_SSP1_RX  2
#define AHBDMA_REQ_SSP1_TX  3
#define AHBDMA_REQ_SSP2_RX  4
#define AHBDMA_REQ_SSP2_TX  5
#define AHBDMA_REQ_SDC      6
#define AHBDMA_REQ_NANDC    9
#define AHBDMA_REQ_PWMTMR5  12
#define AHBDMA_REQ_PWMTMR6  13
#define AHBDMA_REQ_PWMTMR7  14
#define AHBDMA_REQ_PWMTMR8  15

/* define APB DMA routing table 
 */
#define APBDMA_REQ_UART0_RX 1
#define APBDMA_REQ_UART0_TX 2
#define APBDMA_REQ_UART1_RX 3
#define APBDMA_REQ_UART1_TX 4
#define APBDMA_REQ_UART2_RX 5
#define APBDMA_REQ_UART2_TX 6
#define APBDMA_REQ_UART3_RX 7  
#define APBDMA_REQ_UART3_TX 8
#define APBDMA_REQ_UART4_RX 9  
#define APBDMA_REQ_UART4_TX 10
#define APBDMA_REQ_PWMTMR1  12
#define APBDMA_REQ_PWMTMR2  13
#define APBDMA_REQ_PWMTMR3  14
#define APBDMA_REQ_PWMTMR4  15


#endif /* __DMA_ROUTE_H__ */

