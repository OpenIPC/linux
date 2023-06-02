/*
 *  arch/arm/mach-faraday/include/mach/dma-a369.h
 *
 *  Copyright (C) 2010 Faraday Technology
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MACH_DMA_A369_H
#define __MACH_DMA_A369_H

#ifdef CONFIG_PLATFORM_A369

/*
 * Hardware handshake number for FTDMAC020
 * 0-6 are reserved for APB devices
 */
#define A369_DMAC_HANDSHAKE_EDMACH3	7
#define A369_DMAC_HANDSHAKE_EDMACH2	8
#define A369_DMAC_HANDSHAKE_EDMACH1	9
#define A369_DMAC_HANDSHAKE_EDMACH0	10
#define A369_DMAC_HANDSHAKE_IDETX	11
#define A369_DMAC_HANDSHAKE_IDERX	12
#define A369_DMAC_HANDSHAKE_SD1		13
#define A369_DMAC_HANDSHAKE_SD0		14
#define A369_DMAC_HANDSHAKE_NANDC	15

/*
 * Hardware handshake number for FTAPBB020
 */
#define A369_APBB_HANDSHAKE_UART0TX	1
#define A369_APBB_HANDSHAKE_UART0RX	1
#define A369_APBB_HANDSHAKE_UART1TX	2
#define A369_APBB_HANDSHAKE_UART1RX	2
#define A369_APBB_HANDSHAKE_UART2TX	3
#define A369_APBB_HANDSHAKE_UART2RX	3
#define A369_APBB_HANDSHAKE_UART3TX	4
#define A369_APBB_HANDSHAKE_UART3RX	5
#define A369_APBB_HANDSHAKE_IRDA	6
#define A369_APBB_HANDSHAKE_SSP0TX	7
#define A369_APBB_HANDSHAKE_SSP0RX	8
#define A369_APBB_HANDSHAKE_SSP1TX	9
#define A369_APBB_HANDSHAKE_SSP1RX	10
#define A369_APBB_HANDSHAKE_TSC		11
#define A369_APBB_HANDSHAKE_TMR1	12
#define A369_APBB_HANDSHAKE_TMR2	13
#define A369_APBB_HANDSHAKE_TMR5	14

int a369_dmac_handshake_alloc(const char *name);
int a369_dmac_handshake_setup(unsigned int handshake, int which);
int a369_dmac_handshake_free(unsigned int handshake);

#endif	/* CONFIG_PLATFORM_A369 */

#endif	/* __MACH_DMA_A369_H */
