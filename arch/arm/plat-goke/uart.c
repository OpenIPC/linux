/*
 * arch/arm/mach-gk/uart.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/serial_core.h>

#include <mach/hardware.h>
#include <mach/uart.h>
#include <mach/io.h>
#include <mach/rct.h>


extern u64 gk_dmamask;

/* ==========================================================================*/

static void gk_uart_stop_tx(unsigned char __iomem *membase);
#if (UART_INSTANCES >= 2)
static u32 gk_uart_read_ms(unsigned char __iomem *membase);
#endif

/* ==========================================================================*/
/* Serial port registrations */

static struct uart_port gk_uart_port_resource[] =
{
    [0] = {
        .type       = PORT_UART00,
        .iotype     = UPIO_MEM,
        .membase    = (void *)UART0_VA_ADDR,
        .mapbase    = (unsigned long)UART0_PA_ADDR,
        .irq        = UART0_IRQ,
        .uartclk    = GK_UART_FREQ/*12500000*/,
        .fifosize   = UART_FIFO_SIZE,
        .line       = 0,
    },
#if (UART_INSTANCES >= 2)
    [1] = {
        .type       = PORT_UART00,
        .iotype     = UPIO_MEM,
        .membase    = (void *)UART1_VA_ADDR,
        .mapbase    = (unsigned long)UART1_PA_ADDR,
        .irq        = UART1_IRQ,
        .uartclk    = GK_UART_FREQ/*12500000*/,
        .fifosize   = UART_FIFO_SIZE,
        .line       = 0,
    },
#endif
#if (UART_INSTANCES >= 3)
    [2] = {
        .type       = PORT_UART00,
        .iotype     = UPIO_MEM,
        .membase    = (void *)UART2_VA_ADDR,
        .mapbase    = (unsigned long)UART2_PA_ADDR,
        .irq        = UART2_IRQ,
        .uartclk    = GK_UART_FREQ/*12500000*/,
        .fifosize   = UART_FIFO_SIZE,
        .line       = 0,
    },
#endif
};

struct gk_uart_platform_info gk_uart_ports =
{
    .total_port_num     = ARRAY_SIZE(gk_uart_port_resource),
    .registed_port_num  = 0,
    .port[0]       = {
        .port       = &gk_uart_port_resource[0],
        .mcr        = DEFAULT_UART_MCR,
        .fcr        = DEFAULT_UART_FCR,
        .ier        = DEFAULT_UART_IER,
        .stop_tx    = gk_uart_stop_tx,
        .set_pll    = NULL,
        .get_pll    = get_uart_freq_hz,
        .get_ms     = gk_uart_read_ms,
    },
#if (UART_INSTANCES >= 2)
    .port[1]       = {
        .port       = &gk_uart_port_resource[1],
        .mcr        = DEFAULT_UART_MCR,
        .fcr        = DEFAULT_UART_FCR,
        .ier        = DEFAULT_UART_IER,
        .stop_tx    = gk_uart_stop_tx,
        .set_pll    = NULL,
        .get_pll    = get_uart_freq_hz,
        .get_ms     = gk_uart_read_ms,
    },
#endif
#if (UART_INSTANCES >= 3)
    .port[2]       = {
        .port       = &gk_uart_port_resource[2],
        .mcr        = DEFAULT_UART_MCR,
        .fcr        = DEFAULT_UART_FCR,
        .ier        = DEFAULT_UART_IER,
        .stop_tx    = gk_uart_stop_tx,
        .set_pll    = NULL,
        .get_pll    = get_uart_freq_hz,
        .get_ms     = gk_uart_read_ms,
    },
#endif

};

struct platform_device gk_uart0 =
{
    .name           = "uart",
    .id             = 0,
    .resource       = NULL,
    .num_resources  = 0,
    .dev            = {
        .platform_data      = &gk_uart_ports,
        .dma_mask       = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
#if (UART_INSTANCES >= 2)
struct platform_device gk_uart1 =
{
    .name           = "uart",
    .id             = 1,
    .resource       = NULL,
    .num_resources  = 0,
    .dev        = {
        .platform_data      = &gk_uart_ports,
        .dma_mask       = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
#endif
#if (UART_INSTANCES >= 3)
struct platform_device gk_uart2 =
{
    .name           = "uart",
    .id             = 2,
    .resource       = NULL,
    .num_resources  = 0,
    .dev        = {
        .platform_data      = &gk_uart_ports,
        .dma_mask       = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
#endif

/* uart devices */
struct platform_device *gk_uart_devs[UART_INSTANCES] = {
    &gk_uart0,
#if (UART_INSTANCES >= 2)
    &gk_uart1,
#endif
#if (UART_INSTANCES >= 3)
    &gk_uart2,
#endif
};

/* ==========================================================================*/

static void gk_uart_stop_tx(unsigned char __iomem *membase)
{
    gk_uart_clrbitsl((unsigned int)(membase + UART_IE_OFFSET), UART_IE_ETBEI);
}

#if (UART_INSTANCES >= 2)
static u32 gk_uart_read_ms(unsigned char __iomem *membase)
{
    return gk_uart_readl((unsigned int)(membase + UART_MS_OFFSET));
}
#endif



/* uart registration process */
static int __init gk_arch_init(void)
{
    int ret;

    ret = platform_add_devices(gk_uart_devs, ARRAY_SIZE(gk_uart_devs));
    if(ret < 0)
    {
        printk(KERN_ERR "%s: failed to add uart device err=%d\n", __FUNCTION__, ret);
    }
    return ret;
}

arch_initcall(gk_arch_init);


#ifdef KE_DEBUG

#define vtop(x)   ((x) - PAGE_OFFSET + PHYS_OFFSET)
#define ptov(x)   ((x) - PHYS_OFFSET + PAGE_OFFSET)

static inline void ke_putc(int c)
{
    while (!(gk_uart_readl(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_LS_OFFSET) & UART_LS_TEMT));
    gk_uart_writel(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_TH_OFFSET, c);
}

static inline void ke_flush(void)
{
    unsigned int dump;

    while (gk_uart_readl(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_LS_OFFSET) & UART_LS_DR)
    {
        dump = gk_uart_readl(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_RB_OFFSET);
    }
}

static void ke_putstr(const char *ptr)
{
    char c;

    while ((c = *ptr++) != '\0') {
        if (c == '\n')
            ke_putc('\r');
        ke_putc(c);
    }

    ke_flush();
}

void __init ke_print(const char *str, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, str);
    vsnprintf(buf, sizeof(buf), str, ap);
    va_end(ap);

    ke_putstr(buf);
}

/**
 * Put a character out to the UART controller.
 */
static void after_putc(int c)
{
    while (!(gk_uart_readl(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_LS_OFFSET) & UART_LS_TEMT));
    gk_uart_writel(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_TH_OFFSET, c);
}

static void after_flush(void)
{
    unsigned int dump;

    while (gk_uart_readl(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_LS_OFFSET) & UART_LS_DR)
    {
        dump = gk_uart_readl(gk_uart_devs[0]->dev.platform_data->port[0].port->membase+UART_RB_OFFSET);
    }
}

static void after_putstr(const char *ptr)
{
    char c;

    while ((c = *ptr++) != '\0') {
        if (c == '\n')
            after_putc('\r');
        after_putc(c);
    }

    after_flush();
}


void after_print(const char *str, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, str);
    vsnprintf(buf, sizeof(buf), str, ap);
    va_end(ap);

    after_putstr(buf);
}

#endif

