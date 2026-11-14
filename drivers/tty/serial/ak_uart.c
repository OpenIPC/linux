/*
 * AKXX uart driver
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * Author: Zhipeng Zhang <zhang_zhipeng@anyka.com>
 *         Feilong_Dong   <dong_feilong@anyka.com>
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
 */

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/major.h>
#include <linux/reset.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include "ak_uart.h"

struct ak_uart_port {
    char            name[24];
    struct uart_port    port;
    unsigned long baud;
    int rts_active_low;
    int cts_active_low;
    unsigned char __iomem   *rxfifo_base;
    unsigned char __iomem   *txfifo_base;

    unsigned int        rxfifo_offset;
    unsigned int        nbr_to_read;
    unsigned int        timeout_cnt;

    unsigned char       claimed;
    struct clk      *clk;

    unsigned char baud_default; /* first open port, vaule==1, else value==0 */

    struct reset_control *rstc;
};

static struct ak_uart_port *ak_serial_ports[NR_PORTS] = {0};
static struct uart_driver ak_uart_drv;
static struct console ak_serial_console;

/* macros to change one thing to another */
#define tx_enabled(port)    ((port)->unused[0])
#define rx_enabled(port)    ((port)->unused[1])

/**
 * @brief uart_intevent_decode
 * 
 * @param [in]:
 * @RETURN: int
*/
static int uart_intevent_decode(unsigned long status, unsigned int maskbit,
    unsigned int statusbit)
{
    if ((status & 1<<maskbit) && (status & 1<<statusbit))
        return 1;
    else
        return 0;
}

/**
 * @brief uart_subint_disable
 * 
 * @param [in]: struct ak_uart_port *ourport
 * @RETURN: void.
*/
static inline void uart_subint_disable(struct ak_uart_port *ourport,
    unsigned long mask)
{
    unsigned long uart_reg;
    
    /* disable tx_end interrupt */
    uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
    uart_reg &= ~mask;
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);
}

/**
 * @brief uart_subint_enable
 * 
 * @param [in]: struct ak_uart_port *ourport
 * @RETURN: void.
*/
static inline void uart_subint_enable(struct ak_uart_port *ourport,
    unsigned long unmask)
{
    unsigned long uart_reg;

    /* enable tx_end interrupt */
    uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
    uart_reg |= unmask;
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);
}

/**
 * @brief uart_subint_clear
 * 
 * @param [in]: struct ak_uart_port *ourport
 * @RETURN: void.
*/
static void uart_subint_clear(struct ak_uart_port *ourport,
    unsigned int subint)
{
    unsigned long uart_reg;

    switch (subint) {

    case TX_THR_INT:
        uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
        uart_reg |= (1<<subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);
    break;

    case RX_THR_INT:
        uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
        uart_reg &= AKUART_INT_MASK;
        uart_reg |= (1<<subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);
        break;
    
    case RECVDATA_ERR_INT:
        uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
        uart_reg &= AKUART_INT_MASK;
        uart_reg |= (0x1 << subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);
        break;

    case RX_TIMEOUT:
        uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
        uart_reg |= (0x1 << subint);
        uart_reg &= ~( 0x1<<3 );
        __raw_writel(uart_reg, ourport->port.membase + UART_CONF2); 
        
        /* start to receive data */
        uart_reg = __raw_readl(ourport->port.membase + BUF_THRESHOLD);
        uart_reg |= (1UL << 31);
        __raw_writel(uart_reg, ourport->port.membase + BUF_THRESHOLD);  
        break;

    default:
        printk(KERN_ERR "akxx 9xx 9xx 9xx 9xx 9xx 9xx 9xx 9xx kown subint \
            type: %d\n", subint);
        break;
    }

    return;
}

static inline struct ak_uart_port *to_ourport(struct uart_port *port)
{
    return container_of(port, struct ak_uart_port, port);
}

/**
 * @brief uart_txend_interrupt
 * 
 * @param [in]: struct ak_uart_port *ourport
 * @RETURN: void.
*/
static inline void uart_txend_interrupt(struct ak_uart_port *ourport,
    unsigned short status)
{
    unsigned long uart_reg;

    /*handle Tx_end end interrupt */
    uart_reg = __raw_readl(ourport->port.membase + UART_CONF2);
    switch(status)
    {
        case ENABLE:
            uart_reg |= (UARTN_CONFIG2_TX_END_INT_EN);
            break;
            
        case DISABLE:
            uart_reg &= ~(UARTN_CONFIG2_TX_END_INT_EN);
            break;
            
        default:
            break;
    }
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);
}

/**
 * @brief ak_wait_for_txend
 * 
 * @param [in]: struct ak_uart_port *ourport
 * @RETURN: void.
*/
static void ak_wait_for_txend(struct ak_uart_port *ourport)
{
    unsigned int timeout = 10000;

    /*
     * Wait up to 10ms for the character(s) to be sent
     */
    while (!(__raw_readl(ourport->port.membase + UART_CONF2) &
        (1 << TX_END_STATUS))) {
        if (--timeout == 0)
            break;
        udelay(1);
    }
}

/* clear a UARTn buffer status flag */
static inline void clear_uart_buf_status(struct ak_uart_port *ourport,
    unsigned short status)
{
    unsigned long regval;
    unsigned long flags;

    local_irq_save(flags);
    
    regval = __raw_readl(AK_VA_L2CTRL + 0x8C);
    switch(status)
    {
        case RX_STATUS: 
            regval |= (0x1 << (17 + ourport->port.line * 2));
            break;
            
        case TX_STATUS:
            regval |= (0x1 << (16 + ourport->port.line * 2));
            break;
                
        default:
            break;
    }
    __raw_writel(regval,  AK_VA_L2CTRL + 0x8C);

    local_irq_restore(flags);
}

/* clear TX and RX internal status */
static inline void clear_internal_status(struct ak_uart_port *ourport,
    unsigned short status)
{
    unsigned long regval;
    unsigned long flags;

    local_irq_save(flags);
    
    regval = __raw_readl(ourport->port.membase + UART_CONF1);
    switch(status)
    {
        case RX_STATUS:  
            __raw_writel(regval | (0x1 << 29), ourport->port.membase +
                UART_CONF1);
            break;

        case TX_STATUS:
            __raw_writel(regval | (0x1 << 28), ourport->port.membase +
                UART_CONF1);
            break;          
            
        default:
            break;
    }

    local_irq_restore(flags);
}

/* clear TX_th and RX_th count interrupt */
static inline void clear_Int_status(struct ak_uart_port *ourport,
    unsigned short status)
{
    unsigned long regval;
    unsigned long flags;

    local_irq_save(flags);
    
    regval = __raw_readl(ourport->port.membase + BUF_THRESHOLD);
    switch(status)
    {
        case RX_STATUS:  
            __raw_writel(regval | (0x1 << 5), ourport->port.membase +
                BUF_THRESHOLD);
            break;

        case TX_STATUS:
            __raw_writel(regval | (0x1 << 11), ourport->port.membase +
                BUF_THRESHOLD);
            break;          
            
        default:
            break;
    }

    local_irq_restore(flags);
}

/* enable/disable interrupt of  RX_th */
static inline void uart_Rx_interrupt(struct ak_uart_port *ourport,
    unsigned short status)
{
    unsigned long regval;
    unsigned long flags;

    local_irq_save(flags);
    
    regval = __raw_readl(ourport->port.membase + UART_CONF2);
    if(status)
        __raw_writel(regval | (0x1 << RX_INTTERUPT), ourport->port.membase +
            UART_CONF2);
    else
        __raw_writel(regval & ~(0x1 << RX_INTTERUPT), ourport->port.membase +
            UART_CONF2);

    local_irq_restore(flags);
}

/* power management control */
static void ak_serial_pm(struct uart_port *port, unsigned int level,
    unsigned int old)
{

    switch (level) {
    case 3: /* disable */   
        //pr_debug("%s: enterring pm level: %d\n", __FUNCTION__, level);
        break;

    case 0: /* enable  */
        //pr_debug("%s: enterring pm level: %d\n", __FUNCTION__, level);
        break;

    default:
        pr_debug(KERN_ERR "akxx serial: unknown pm %d\n", level);
        break;
    }
}

/* is tx fifo empty */
static unsigned int ak_serial_tx_empty(struct uart_port *port)
{
    unsigned long uart_reg;
    uart_reg = __raw_readl(port->membase + UART_CONF2);

    if (uart_reg & (1 << TXFIFO_EMPTY))
        return 1;

    return 0;
}

/* no modem control lines */
static unsigned int ak_serial_get_mctrl(struct uart_port *port)
{
    /* FIXME */
    pr_debug("%s\n", __FUNCTION__);
    return 0;
}

static void ak_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    /* todo - possibly remove AFC and do manual CTS */
    pr_debug("%s\n", __FUNCTION__);
}

static void ak_serial_stop_tx(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);
    pr_debug("%s\n", __FUNCTION__); 


    if (tx_enabled(port))
    {
        uart_txend_interrupt(ourport, DISABLE);
        tx_enabled(port) = 0;
    }
}

static void ak_serial_stop_rx(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);
    pr_debug("%s\n", __FUNCTION__);

    if (rx_enabled(port))
    {
        uart_Rx_interrupt(ourport, DISABLE);
        rx_enabled(port) = 0;
    }

}

static void ak_serial_enable_ms(struct uart_port *port)
{
    pr_debug("%s\n", __FUNCTION__);
}

static void ak_serial_break_ctl(struct uart_port *port, int break_state)
{
    pr_debug("%s\n", __FUNCTION__);
}

static void ak_handle_tx(struct ak_uart_port *ourport)
{
    struct uart_port *port = &ourport->port;
    struct circ_buf *xmit = &port->state->xmit;
    
    unsigned char __iomem   *pbuf;
    unsigned char *pxmitbuf;

    unsigned int i;
    int txcount , tx_tail;
    unsigned int l2_offset = 0;
    unsigned long regval;
    
    /* if there is not anything more to transmit, or the uart is now
     * stopped, disable the uart and exit
     */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
    {
        ak_serial_stop_tx(port);
        return;
    }

    txcount = uart_circ_chars_pending(xmit);
    if(txcount > 64)
        txcount = 64;

    pbuf = ourport->txfifo_base;
    pxmitbuf = xmit->buf;

    /* clear a uartx buffer status */
    clear_uart_buf_status(ourport, TX_STATUS);

    /* clear the tx internal status */
    clear_internal_status(ourport, TX_STATUS);

    __raw_writel(0x0, ourport->txfifo_base + 0x3C);

    l2_offset = 0;
    tx_tail = xmit->tail;
    regval = 0;
    for(i = 0; i < txcount; i++)
    {
        regval |= pxmitbuf[tx_tail]<<((i & 3) * 8 );
        if((i & 3) == 3)

        {

            __raw_writel(regval, pbuf + l2_offset);
            l2_offset = l2_offset + 4;

            regval = 0;
        }
        tx_tail = (tx_tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx += 1;
    }
    if(i & 3)
    {
        __raw_writel(regval, pbuf + l2_offset);
    }

    xmit->tail = tx_tail;
    /* set sending data count */
    regval = (__raw_readl(ourport->port.membase + UART_CONF2)&
        (~UARTN_CONFIG2_TX_BYT_CNT_MASK)) | (UARTN_CONFIG2_TX_BYT_CNT(txcount))|
        (UARTN_CONFIG2_TX_BYT_CNT_VLD);
    __raw_writel(regval, ourport->port.membase + UART_CONF2);
    /* enable tx_end_int */
    uart_txend_interrupt(ourport, ENABLE);

    //while((__raw_readl(ourport->port.membase+DATA_CONF) & 0x1fff) != 0);
    
    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    //if (uart_circ_empty(xmit))
    //  ak_serial_stop_tx(port);
}

static void ak_serial_start_tx(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);

    pr_debug("%s\n", __FUNCTION__); 

    if (!tx_enabled(port))
    {
        tx_enabled(port) = 1;
        ak_handle_tx(ourport);
    }
}

static irqreturn_t ak_uart_irqhandler(int irq, void *dev_id)
{
    struct ak_uart_port *ourport = dev_id;
    struct uart_port *port = &ourport->port;
    
    struct tty_port *tty = &(port->state->port);    //port->info->tty;

    unsigned int flag= TTY_NORMAL;

    unsigned char __iomem   *pbuf;
    unsigned long uart_status;
    unsigned int rxcount = 0;
    unsigned char ch = 0;
    unsigned int i;
    unsigned int l2_offset = 0;

    uart_status = __raw_readl(ourport->port.membase + UART_CONF2);

    /* clear R_err interrupt */
    if ( uart_intevent_decode(uart_status, RECVDATA_ERR_INT_ENABLE,
        RECVDATA_ERR_INT) )
    {
        uart_subint_clear(ourport, RECVDATA_ERR_INT);
    }

    if ( uart_intevent_decode(uart_status, TX_END_INTERRUPT, TX_END_STATUS) )
        ak_handle_tx(ourport);

    /* rx threshold interrupt */
    if ( uart_intevent_decode(uart_status, RX_THR_INT_ENABLE, RX_THR_INT) ||
        uart_intevent_decode(uart_status, RX_TIMEOUT_INT_ENABLE, RX_TIMEOUT))
    {
        if ( uart_intevent_decode(uart_status, RX_THR_INT_ENABLE, RX_THR_INT))
            uart_subint_clear(ourport, RX_THR_INT);
        else {
            uart_subint_clear(ourport, RX_TIMEOUT);
        }

        ourport->nbr_to_read = (__raw_readl(ourport->port.membase +
            DATA_CONF)>>13) & 0x7f;

        if (ourport->nbr_to_read  != ourport->rxfifo_offset) {
            l2_offset = ourport->rxfifo_offset; 
            pbuf = ourport->rxfifo_base + l2_offset;

            /* copy data */
            if (ourport->nbr_to_read > l2_offset) {
                        rxcount = ourport->nbr_to_read - l2_offset;
                for (i=0; i<rxcount; i++) {
                    ch = __raw_readb(pbuf + i);
                    uart_insert_char(port, 0, 0, ch, flag);
                }   
                } else {
                rxcount = (UART_RX_FIFO_SIZE - l2_offset);
                for (i=0; i<rxcount; i++) {
                    ch = __raw_readb(pbuf + i);
                    uart_insert_char(port, 0, 0, ch, flag);
                }
            
                pbuf = ourport->rxfifo_base;
                for (i=0; i < ourport->nbr_to_read; i++) {
                    ch = __raw_readb(pbuf + i);
                    uart_insert_char(port, 0, 0, ch, flag);
                }
            }
            ourport->rxfifo_offset = ourport->nbr_to_read;
        }

        tty_flip_buffer_push(tty);
    } else if (uart_intevent_decode(uart_status,
        MEM_RDY_INT_ENABLE, MEM_RDY_INT)) {
        ourport->nbr_to_read = (__raw_readl(ourport->port.membase + DATA_CONF)>>
            13) & 0x7f;

        l2_offset = ourport->rxfifo_offset;
        pbuf = ourport->rxfifo_base + l2_offset;

        /* copy data */
        if (ourport->nbr_to_read > l2_offset) {
            rxcount = ourport->nbr_to_read - l2_offset;
            for (i=0; i<rxcount; i++) {
                ch = __raw_readb(pbuf + i);
                uart_insert_char(port, 0, 0, ch, flag);
            }
        } else {
            rxcount = (UART_RX_FIFO_SIZE - l2_offset);
            for (i=0; i<rxcount; i++) {
                ch = __raw_readb(pbuf + i);
                uart_insert_char(port, 0, 0, ch, flag);
            }
            pbuf = ourport->rxfifo_base;
            for (i=0; i < ourport->nbr_to_read; i++) {
                ch = __raw_readb(pbuf + i);
                uart_insert_char(port, 0, 0, ch, flag);
            }
        }
        ourport->rxfifo_offset = ourport->nbr_to_read;
        tty_flip_buffer_push(tty);
    }

    return IRQ_HANDLED;
}

static void ak_serial_shutdown(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);
    unsigned int uart_reg = 0;
    u32 regval;

    /*
     * 1st, free irq.
     * 2nd, disable/mask hw uart setting.
     * 3rd, close uart clock.
     */

    /* mask all interrupt */
    __raw_writel(0, ourport->port.membase + UART_CONF2);

    ak_wait_for_txend(ourport);

    uart_reg = __raw_readl(ourport->port.membase + 0xc);
    uart_reg  &= (~(1<<5));
    __raw_writel(uart_reg, ourport->port.membase + 0xc);
    uart_reg = __raw_readl(ourport->port.membase + UART_CONF1);
    uart_reg  &= (~(1<<29));
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF1);
    /* clear uartx interrupt */
    uart_reg  &= (~1<<21);
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF1);

    /* clear uartn TX/RX buf status flag and call ak_setpin_as_gpio() */
    switch(ourport->port.line) {
        case 0:
            regval = __raw_readl(rL2_CONBUF8_15);
            regval |= (0x3<<16);
            __raw_writel(regval, rL2_CONBUF8_15);
            break;
        case 1:
            regval = __raw_readl(rL2_CONBUF8_15);
            regval |= (0x3<<18);
            __raw_writel(regval, rL2_CONBUF8_15);
            break;
        case 2:
            regval = __raw_readl(rL2_CONBUF8_15);
            regval |= (0x3<<20);
            __raw_writel(regval, rL2_CONBUF8_15);
            break;
        case 3:
            regval = __raw_readl(rL2_CONBUF8_15);
            regval |= (0x3<<22);
            __raw_writel(regval, rL2_CONBUF8_15);
            break;
    }   

    free_irq(port->irq, ourport);   

    clk_disable_unprepare(ourport->clk);

    rx_enabled(port) = 0;
    tx_enabled(port) = 0;
}

/*
 * 1, setup gpio.
 * 2, enable clock.
 * 3, request irq and setting up uart control.
 * 4, enable subirq.
 */
static int ak_serial_startup(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);
    unsigned long uart_reg;
    int ret;

    if ( rx_enabled(port) && tx_enabled(port))
        return 0;

    /* enable uart clock */
    ret = clk_prepare_enable(ourport->clk);
    if (ret) {
        pr_err( "failed to enable uart clock source.\n");
        goto startup_err;
    }

    if (ourport->rstc) {
        ret = reset_control_assert(ourport->rstc);
        if (ret) {
            pr_err("unable to reset_control_assert\n");
            return ret;
        }
        udelay(1);
        ret = reset_control_deassert(ourport->rstc);
        if (ret) {
            pr_err("unable to reset_control_deassert\n");
            return ret;
        }
    }

#ifdef CONFIG_MACH_AK3918AV100_FPGA
    ourport->port.uartclk = 60000000;
#else
    ourport->port.uartclk = clk_get_rate(ourport->clk);
#endif
    //clear L2 Buffer
    clear_uart_buf_status(ourport, RX_STATUS);
    
    uart_reg = __raw_readl(ourport->port.membase + UART_CONF1);
    uart_reg |= (UARTN_CONFIG1_RTS_EN_BY_CIRCUIT | UARTN_CONFIG1_EN |
        UARTN_CONFIG1_RX_STA_CLR|UARTN_CONFIG1_TX_STA_CLR|
        UARTN_CONFIG1_TIMEOUT_EN);
    
    if(port->flags & UPF_HARD_FLOW)
    {
            pr_debug("%s: support Hardware flow control, cts_active_low=%d, \
                rts_active_low=%d\n", __func__, ourport->cts_active_low,
                ourport->rts_active_low);
            /* 
             * Configuration Register 1 of UART
             * 0x00
             * bit[18] 
             * 0 = CTS active high
             * 1 = CTS active low
             *
             */
            if(!!ourport->cts_active_low)
                uart_reg |=  (1<<18);
            else
                uart_reg &=  ~(1<<18);

            /* 
             * Configuration Register 1 of UART
             * 0x00
             * bit[19] 
             * 0 = RTS active high
             * 1 = RTS active low
             * */
            if(!!ourport->rts_active_low)
                uart_reg |=  (1<<19);
            else
                uart_reg &=  ~(1<<19);  
    }else
    {
        pr_info("%s: Not support Hardware flow control\n",__func__);
        #ifdef CONFIG_MACH_AK37E
            /*
             * if not use RTS/CTS, AK37E uart1 should set bit 18 and bit 19 to 1
             */
            if(port->line == 1)
                uart_reg |= (1<<18|1<<19);
            else
                uart_reg &= ~(1<<18|1<<19);
        #else 
                uart_reg &= ~(1<<18|1<<19);
        #endif
    }
    
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF1);

    /* mask all interrupt */
    __raw_writel(0, ourport->port.membase + UART_CONF2);

    /*
     * config stop bit and timeout value
     * set timeout = 32, stop bit = 1;
     */
    uart_reg = (0x1f << 16);
    __raw_writel(uart_reg, ourport->port.membase + UART_STOPBIT_TIMEOUT);

   /* 
      * set threshold to 32bytes 
      * set RX_th_cfg_h = 0, set RX_th_cfg_l = 31
      */
    uart_reg = __raw_readl(ourport->port.membase + DATA_CONF);
    uart_reg &= ~UARTN_RX_TH_CFG_H_MASK;
    __raw_writel(uart_reg, ourport->port.membase + DATA_CONF);  
    uart_reg = __raw_readl(ourport->port.membase + BUF_THRESHOLD);
    uart_reg &= ~(UARTN_RX_TH_CFG_L_MASK);
    uart_reg |= UARTN_RX_TH_CFG_L(0x1F);    /* 32 Bytes */
    //uart_reg |= (1 << 11);
    __raw_writel(uart_reg, ourport->port.membase + BUF_THRESHOLD);
    
    clear_internal_status(ourport, RX_STATUS);

    /* ourport->rxfifo_offset = 0; */
    ourport->rxfifo_offset = 0;

    /* to clear  RX_th count interrupt */
    uart_reg = __raw_readl(ourport->port.membase + BUF_THRESHOLD);
    uart_reg |= (UARTN_RX_TH_CLR);
    __raw_writel(uart_reg, ourport->port.membase + BUF_THRESHOLD);
    udelay(10);
    uart_reg = __raw_readl(ourport->port.membase + BUF_THRESHOLD);
    uart_reg &= ~(UARTN_RX_TH_CLR);
    __raw_writel(uart_reg, ourport->port.membase + BUF_THRESHOLD);
        udelay(10);
    uart_reg = __raw_readl(ourport->port.membase + BUF_THRESHOLD);
    uart_reg |= (UARTN_RX_START);
    __raw_writel(uart_reg, ourport->port.membase + BUF_THRESHOLD);
    

    /*
     *  enable timeout, rx mem_rdy and rx_th tx_end interrupt 
     */
     uart_reg = (UARTN_CONFIG2_RX_TH_INT_EN|UARTN_CONFIG2_RX_BUF_FULL_INT_EN|
        UARTN_CONFIG2_TIMEOUT_INT_EN|UARTN_CONFIG2_R_ERR_INT_EN);
    __raw_writel(uart_reg, ourport->port.membase + UART_CONF2);

    rx_enabled(port) = 1;
    tx_enabled(port) = 0;

    /* register interrupt */
    ret = request_irq(port->irq, ak_uart_irqhandler, 0/*IRQF_DISABLED*/,
        ourport->name, ourport);
    if (ret) {
        printk(KERN_ERR "can't request irq %d for %s\n", port->irq,
            ourport->name);
        goto startup_err;
    }
    return 0;

startup_err:
    ak_serial_shutdown(port);
    return ret;
}/* end of func */

static void ak_serial_set_termios(struct uart_port *port, 
                struct ktermios *termios, struct ktermios *old)
{
    struct ak_uart_port *ourport = to_ourport(port);
    unsigned int baud;
    unsigned long baud_div;
    unsigned long flags;
    unsigned long regval;

#ifdef CONFIG_MACH_AK3918AV100_FPGA
    //FPGA use default clock 60M
    unsigned long asic_clk = 60000000;
#else
    unsigned long asic_clk = ourport->port.uartclk;
#endif

    termios->c_cflag &= ~(HUPCL | CMSPAR);
    termios->c_cflag |= CLOCAL;

    /*
     * Hook:
     * Set the baud rate to the value on the device tree
     * NOTE: When the serial port is first opened,
     *       the baud rate is set by default (ourport->baud_default==1)
     */
    if(ourport->baud_default) {
        tty_termios_encode_baud_rate(termios, ourport->baud, ourport->baud);
        ourport->baud_default = 0;
    }

#ifdef  CONFIG_MACH_AK3918AV100_FPGA
    //FPGA use bardrate 2000000
    baud = 2000000;
#else
    baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/32);
#endif

    spin_lock_irqsave(&port->lock, flags);

    ak_wait_for_txend(ourport);

    /* compute division value */
    baud_div = (asic_clk / baud - 1);

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
    /*
     * baud_div max = 2^21 = 0x1FFFFF
     */
    if(baud_div > 0x1FFFFF) {
        baud_div = 0x1FFFFF;
    }

    /*
     * ak3918av1xx added a high-division register
     * div_cnt_cfg_real[20:0] = {Div_cnt_cfg_h, Div_cnt_cfg}
     * so, set it!
     */
    regval = (baud_div>>16) & 0x1F;
    __raw_writel(regval, port->membase + UART_DIV_FACTOR);
#endif

    /* baudrate setting */
    regval = __raw_readl(port->membase + UART_CONF1);
    regval &= ~(0xffff);
    regval &= ~(0x1 << 22);
    regval |= (baud_div & 0xffff);
    regval |= (1 << 28) | (1 << 29);

    if (asic_clk % baud)
        regval |= (0x1 << 22);

    clear_uart_buf_status(ourport, TX_STATUS);
    clear_uart_buf_status(ourport, RX_STATUS);

    clear_internal_status(ourport, RX_STATUS);
    clear_internal_status(ourport, TX_STATUS);

    ourport->rxfifo_offset = 0;

    pr_debug("port->line =%d ,termios->c_cflag =%x. port->status =%x\n",
        port->line, termios->c_cflag,port->status);

    port->status &= ~UPSTAT_AUTOCTS;

    /* flow control setting */
    if((termios->c_cflag & CRTSCTS) &&  (port->flags & UPF_HARD_FLOW))
    {
        pr_debug("%s: UPSTAT_AUTOCTS set\n",__func__);
        port->status |= UPSTAT_AUTOCTS;
    }
    else  
    {
        pr_debug("%s: UPSTAT_AUTOCTS not set,\n",__func__);
    }
    regval &= ~(0x3 << 25);
    /* parity setting */
    if (termios->c_cflag & PARENB) {
        if (termios->c_cflag & PARODD)
            regval |= (0x2 << 25);  /* odd parity */
        else
            regval |= (0x3 << 25);  /* evnt parity*/
    }

    __raw_writel(regval, port->membase + UART_CONF1);

    /*
     * Update the per-port timeout.
     */
    uart_update_timeout(port, termios->c_cflag, baud);

    /*
     * Which character status flags should we ignore?
     */
    port->ignore_status_mask = 0;

    spin_unlock_irqrestore(&port->lock, flags);
}

static const char *ak_serial_type(struct uart_port *port)
{
    switch (port->type) 
    {
        case PORT_AK:
            return "AK";
        default:
            return NULL;
    }
}

static void ak_serial_release_port(struct uart_port *port)
{
    pr_debug("%s\n", __FUNCTION__);
}

static int ak_serial_request_port(struct uart_port *port)
{
    pr_debug("%s\n", __FUNCTION__);
    return 0;
}

static void ak_serial_config_port(struct uart_port *port, int flags)
{
    //struct ak_uart_port *ourport = to_ourport(port);
    port->type = PORT_AK;
    //ourport->rxfifo_offset = 0;

    /* 
     * when setting cap_hw_flow_control property, user should also
     * set CTS/RTS sharepin in dts pinctrl.
     */
    if(of_property_read_bool(port->dev->of_node, "cap_hw_flow_control"))
        port->flags |=  UPF_HARD_FLOW;
    
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int
ak_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    pr_debug("%s\n", __FUNCTION__);
    return 0;
}

static struct uart_ops ak_serial_ops = {
    .pm             = ak_serial_pm,
    .tx_empty       = ak_serial_tx_empty,
    .get_mctrl      = ak_serial_get_mctrl,
    .set_mctrl      = ak_serial_set_mctrl,
    .stop_tx        = ak_serial_stop_tx,
    .start_tx       = ak_serial_start_tx,
    .stop_rx        = ak_serial_stop_rx,
    .enable_ms      = ak_serial_enable_ms,
    .break_ctl      = ak_serial_break_ctl,
    .startup        = ak_serial_startup,
    .shutdown       = ak_serial_shutdown,
    .set_termios    = ak_serial_set_termios,
    .type           = ak_serial_type,
    .release_port   = ak_serial_release_port,
    .request_port   = ak_serial_request_port,
    .config_port    = ak_serial_config_port,
    .verify_port    = ak_serial_verify_port,
};

static void ak_uart_add_console_port(struct ak_uart_port *up)
{
    ak_serial_ports[up->port.line] = up;
}

static int ak_serial_probe(struct platform_device *dev)
{
    struct ak_uart_port *ourport;
    struct resource *resource;
    struct device_node *np = dev->dev.of_node;
    int ret = 0;
    unsigned int speed;

    dev->id = of_alias_get_id(np, "uart");
    ourport = devm_kzalloc(&dev->dev, sizeof(*ourport), GFP_KERNEL);
    if (!ourport) {
        dev_err(&dev->dev, "Failed to allocate memory for tup\n");
        return -ENOMEM;
    }

    resource = platform_get_resource(dev, IORESOURCE_MEM, 0);
    if (!resource) {
        dev_err(&dev->dev, "No IO memory resource\n");
        return -ENODEV;
    }
    
    strcpy(ourport->name,dev->name);
    
    ourport->rxfifo_base   = AK_UART0_RXBUF_BASE + (dev->id)*AK_UART_REG_OFFSET;
    ourport->txfifo_base   = AK_UART0_TXBUF_BASE + (dev->id)*AK_UART_REG_OFFSET;
        
    ourport->port.ops = &ak_serial_ops;
    ourport->port.iotype    = UPIO_MEM;
    ourport->port.mapbase   = resource->start;
    ourport->port.membase   = devm_ioremap_resource(&(dev->dev), resource);
    ourport->port.irq       = platform_get_irq(dev, 0);

    ourport->port.flags     = UPF_BOOT_AUTOCONF;
    ourport->port.line      = dev->id;
    ourport->port.dev       = &dev->dev;

    ourport->clk = devm_clk_get(&dev->dev, NULL);
    if (IS_ERR(ourport->clk)) {
        dev_err(&dev->dev, "failed to find clock source.\n");
        ret = PTR_ERR(ourport->clk);
        goto probe_err;
    }

    ourport->rstc = devm_reset_control_get(&dev->dev, np->name);
    if (IS_ERR(ourport->rstc)) {
        dev_err(&dev->dev, "No reset controller specified\n");
        ret = PTR_ERR(ourport->rstc);
        goto probe_err;
    }

#ifdef CONFIG_MACH_AK3918AV100_FPGA
    ourport->port.uartclk = 60000000;
#else
    /* uart clock is divided by asic clock */
    ourport->port.uartclk = clk_get_rate(ourport->clk);
#endif
    
    if (of_property_read_u32(dev->dev.of_node,
            "fifosize",&ourport->port.fifosize)) {
        dev_err(&dev->dev,"Unable to find fifosize in uart node.\n");
        ret = -EFAULT;
        goto probe_err;
    }
    /* If current-speed was set, then try not to change it. */
    if (of_property_read_u32(dev->dev.of_node, "current-speed", &speed)){
        dev_err(&dev->dev, "current-speed property NOT set\n");
        return -EINVAL;
    }

    /* rts_active_low =  1 */
    if (of_property_read_u32(dev->dev.of_node, "rts_active_low",
                     &ourport->rts_active_low))
        ourport->rts_active_low = 1;

    /* cts_active_low =  1 */
    if (of_property_read_u32(dev->dev.of_node, "cts_active_low",
                     &ourport->cts_active_low))
        ourport->cts_active_low = 1;

    ourport->baud = speed;
    ourport->baud_default = 1;

    ak_uart_add_console_port(ourport);

    uart_add_one_port(&ak_uart_drv, &ourport->port);
    
    platform_set_drvdata(dev, &ourport->port);
    
    return 0;

probe_err:
    return ret;
}

static int ak_serial_remove(struct platform_device *dev)
{
    struct uart_port *port = (struct uart_port *)dev_get_drvdata(&dev->dev);

    if (port) {     
        uart_remove_one_port(&ak_uart_drv, port);
    }
    return 0;
}

/* UART power management code */

#ifdef CONFIG_PM

#if 0
static void ak_serial_dump(struct uart_port *uport)
{
    struct ak_uart_port *ourport = to_ourport(uport);
    pr_info("line %d membase %p\n", uport->line, ourport->port.membase);
    pr_info("UART_CONF1(%d):0x%x\n", UART_CONF1,
        __raw_readl(ourport->port.membase + UART_CONF1));
    pr_info("UART_CONF2(%d):0x%x\n", UART_CONF2,
        __raw_readl(ourport->port.membase + UART_CONF2));
    pr_info("DATA_CONF(%d):0x%x\n", DATA_CONF,
        __raw_readl(ourport->port.membase + DATA_CONF));
    pr_info("BUF_THRESHOLD(%d):0x%x\n", BUF_THRESHOLD,
        __raw_readl(ourport->port.membase + BUF_THRESHOLD));
    pr_info("UART_RXBUF(%d):0x%x\n", UART_RXBUF,
        __raw_readl(ourport->port.membase + UART_RXBUF));
    pr_info("UART_STOPBIT_TIMEOUT(%d):0x%x\n", UART_STOPBIT_TIMEOUT,
        __raw_readl(ourport->port.membase + UART_STOPBIT_TIMEOUT));
}
#endif

static int ak_serial_suspend(struct platform_device *dev,
    pm_message_t state)
{
    struct uart_port *uport = (struct uart_port *)dev_get_drvdata(&dev->dev);
    const struct uart_ops *ops = uport->ops;
    int tries;

    uport->suspended = 1;

    uport->unused1 = 0;
    if (tx_enabled(uport)) {
        /*
         * Wait for the transmitter to empty.
         */
        for (tries = 3; !ops->tx_empty(uport) && tries; tries--)
            msleep(5);
        if (!tries) {
            dev_err(uport->dev, "Unable to drain transmitter\n");
        }
        uport->unused1 = 1;
    }

    if (tx_enabled(uport) | rx_enabled(uport)) {
        spin_lock_irq(&uport->lock);
        if (tx_enabled(uport)) {
            ops->stop_tx(uport);
        }
        ops->stop_rx(uport);
        spin_unlock_irq(&uport->lock);

        ops->shutdown(uport);
    }

    /*
     * Disable the console device before suspending.
     */
    if (uart_console(uport))
        console_stop(uport->cons);

    pinctrl_pm_select_sleep_state(&dev->dev);

    return 0;
}

static int ak_serial_resume(struct platform_device *dev)
{
    struct uart_port *uport = (struct uart_port *)dev_get_drvdata(&dev->dev);
    const struct uart_ops *ops = uport->ops;
    struct ktermios termios;
    struct uart_state *state = ak_uart_drv.state + uport->line;
    struct tty_port *port = &state->port;

    pinctrl_pm_select_default_state(&dev->dev);

    spin_lock_irq(&uport->lock);
    ops->startup(uport);
    if (uport->unused1) {
        ops->start_tx(uport);
    }
    spin_unlock_irq(&uport->lock);

    if (uart_console(uport)) {
        /*
         * First try to use the console cflag setting.
         */
        memset(&termios, 0, sizeof(struct ktermios));
        termios.c_cflag = uport->cons->cflag;
        /*
         * If that's unset, use the tty termios setting.
         */
        if (port->tty && termios.c_cflag == 0)
            termios = port->tty->termios;
        uport->ops->set_termios(uport, &termios, NULL);

        console_start(uport->cons);
    }

    uport->suspended = 0;

    return 0;
}
#else
#define ak_serial_suspend NULL
#define ak_serial_resume    NULL
#endif

static const struct of_device_id ak_uart_of_ids[] = {
    { .compatible = "anyka,ak37d-uart0" },
    { .compatible = "anyka,ak37d-uart1" },
    { .compatible = "anyka,ak37d-uart2" },
    { .compatible = "anyka,ak37d-uart3" },
    { .compatible = "anyka,ak39ev330-uart0" },
    { .compatible = "anyka,ak39ev330-uart1" },
    { .compatible = "anyka,ak37e-uart0" },
    { .compatible = "anyka,ak37e-uart1" },
    { .compatible = "anyka,ak37e-uart2" },
    { .compatible = "anyka,ak37e-uart3" },
    { .compatible = "anyka,ak3918av100-uart0" },
    { .compatible = "anyka,ak3918av100-uart1" },
    { .compatible = "anyka,ak3918av100-uart2" },
    { .compatible = "anyka,ak3918ev300l-uart0" },
    { .compatible = "anyka,ak3918ev300l-uart1" },
    { .compatible = "anyka,ak3918ev300l-uart2" },
    {},
};
MODULE_DEVICE_TABLE(of, ak_uart_of_ids);

#ifdef CONFIG_SERIAL_AK_CONSOLE
static inline void ak_uart_putchar(struct ak_uart_port *ourport,
    unsigned char ch)
{
    unsigned long regval;

    /* clear the tx internal status */
    clear_internal_status(ourport, TX_STATUS);

    /* clear a uartx buffer status */
    clear_uart_buf_status(ourport, TX_STATUS);

    /*to inform the buf is full*/   
    __raw_writel(ch, ourport->txfifo_base);
    __raw_writel(0x0, ourport->txfifo_base + 0x3C);

    /* to clear  TX_th count interrupt */
    clear_Int_status(ourport, TX_STATUS);

    /* start to transmit */
    regval = __raw_readl(ourport->port.membase + UART_CONF2);
    regval &= AKUART_INT_MASK;
    regval |= (0x1<<4) | (0x1<<16);
    __raw_writel(regval, ourport->port.membase + UART_CONF2);
    

    /* wait for tx end */
    while (!(__raw_readl(ourport->port.membase + UART_CONF2) &
        (1 << TX_END_STATUS)))
        ;
}

static inline void ak_uart_send(struct ak_uart_port *ourport, const char *s,
    unsigned int count)
{
    unsigned long regval;
    unsigned int left, trans_size;
    int offset;
    
    /* clear the tx internal status */
    clear_internal_status(ourport, TX_STATUS);

    /* clear a uartx buffer status */
    clear_uart_buf_status(ourport, TX_STATUS);

    left = count;
    
    while (left)
    {
        trans_size = 128; // L2 buffer for UART is 128 bytes
        if (left < trans_size)
            trans_size = left;

        if ((int)s & 0x3) // this should not happen
            return;

        // copy
        for (offset=0; offset<trans_size; offset+=4)
        {
            __raw_writel(*(int *)(s+offset), ourport->txfifo_base + offset);
        }
        left -= trans_size;
        s += trans_size;
        
        if ((offset&0x3f) != 0)
        {/*
          * when an L2 block is not filled,
          * write last dword to inform the buf is full
          */
            __raw_writel(0x0, ourport->txfifo_base + offset - (offset&0x3f) +
                0x3C);
        }
        
        /* to clear  TX_th count interrupt */
        clear_Int_status(ourport, TX_STATUS);
        
        /* start to transmit */
        regval = __raw_readl(ourport->port.membase + UART_CONF2);
        regval &= AKUART_INT_MASK;
        regval |= (trans_size<<4) | (0x1<<16);
        __raw_writel(regval, ourport->port.membase + UART_CONF2);
        
        /* wait for tx end */
        while (!(__raw_readl(ourport->port.membase + UART_CONF2) &
            (1 << TX_END_STATUS)));
    }
}

static void ak_serial_console_putchar(struct uart_port *port, int ch)
{
    struct ak_uart_port *ourport = to_ourport(port);

    ak_wait_for_txend(ourport);

    ak_uart_putchar(ourport, ch);
}

static void ak_serial_console_write(struct console *co, const char *s,
    unsigned int count)
{
    static struct ak_uart_port * up;

    up = ak_serial_ports[co->index];
    uart_console_write(&up->port, s, count, ak_serial_console_putchar);
}

void ak_serial_console_send(struct uart_port *port, const char *s,
    unsigned int count)
{
    struct ak_uart_port *ourport = to_ourport(port);

    ak_wait_for_txend(ourport);
    ak_uart_send(ourport, s, count);
}
EXPORT_SYMBOL_GPL(ak_serial_console_send);

void ak_early_serial_send(unsigned int line, const char *s,
    unsigned int count)
{
    struct ak_uart_port uport = {0};
    uport.rxfifo_base   = AK_UART0_RXBUF_BASE + line * AK_UART_REG_OFFSET;
    uport.txfifo_base   = AK_UART0_TXBUF_BASE + line * AK_UART_REG_OFFSET;
    uport.port.membase  = AK_VA_SUBCTRL + 0x30000 + line * 0x8000;

    ak_serial_console_send(&uport.port, s, count);
}
EXPORT_SYMBOL_GPL(ak_early_serial_send);

static int __init
ak_serial_console_setup(struct console *co, char *options)
{
    struct ak_uart_port *uap;
    int baud = 115200;
    int bits = 8;
    int parity = 'n';
    int flow = 'n';
    int ret;

    /* is this a valid port */
    if (co->index == -1 || co->index >= NR_PORTS)
        co->index = 0;

    uap = ak_serial_ports[co->index];
    if (uap == NULL)
        return -ENODEV;

    ret = clk_prepare(uap->clk);
    if (ret)
        return ret;
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    uap->port.uartclk = 60000000;
#else
    uap->port.uartclk = clk_get_rate(uap->clk);
#endif
    /*
     * Check whether an invalid uart number has been specified, and
     * if so, search for the first available port that does have
     * console support.
     */
    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    pr_debug("ak_serial_console_setup: baud %d, line %d\n", baud,
        uap->port.line);

    return uart_set_options(&uap->port, co, baud, parity, bits, flow);
}

static struct console ak_serial_console = {
    .name       = AK_SERIAL_NAME,
    .device     = uart_console_device,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .write      = ak_serial_console_write,
    .setup      = ak_serial_console_setup,
    .data = &ak_uart_drv,
};
#endif /* CONFIG_SERIAL_AK_CONSOLE */


static struct uart_driver ak_uart_drv = {
    .owner      = THIS_MODULE,
    .dev_name   = AK_SERIAL_NAME,
    .driver_name    = AK_SERIAL_NAME,
    .nr         = NR_PORTS,
    .major      = 0,
    .minor      = 0,
    #ifdef CONFIG_SERIAL_AK_CONSOLE
    .cons       = &ak_serial_console,
    #endif
};

static struct platform_driver ak_serial_drv = {
    .probe          = ak_serial_probe,
    .remove         = ak_serial_remove,
    .suspend        = ak_serial_suspend,
    .resume         = ak_serial_resume,
    .driver         = {
        .name   = "ak-uart",
        .of_match_table = of_match_ptr(ak_uart_of_ids),
        .owner  = THIS_MODULE,
    },
};


/* module initialisation code */
static int __init ak_serial_modinit(void)
{
    int ret;

    printk("AKxx uart driver init, (c) 2013 ANYKA\n");
    ret = uart_register_driver(&ak_uart_drv);
    if (ret < 0) {
        printk(KERN_ERR "failed to register UART driver\n");
        return -1;
    }
    platform_driver_register(&ak_serial_drv);
    return 0;
}

static void __exit ak_serial_modexit(void)
{
    platform_driver_unregister(&ak_serial_drv);
    
    uart_unregister_driver(&ak_uart_drv);
}

module_init(ak_serial_modinit);
module_exit(ak_serial_modexit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anyka Microelectronic");
MODULE_DESCRIPTION("Anyka Serial port driver");
MODULE_VERSION("1.0.03");
