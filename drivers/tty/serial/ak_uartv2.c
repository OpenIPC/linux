/*
 * AKXX uartv2 driver
 *
 * Copyright (C) 2023 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * Author: JianXiong Yang <yang_jianxiong@anyka.oa>
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
#include "ak_uartv2.h"


struct ak_uart_port {
    char            name[24];
    struct uart_port    port;
    unsigned long baud;
    int rts_active_low;
    int cts_active_low;
    unsigned char __iomem   *rxfifo_base;
    unsigned char __iomem   *txfifo_base;

    unsigned int        rxfifo_offset;
    unsigned int        rx_th;
    unsigned int        rx_th_int_cnt;
    unsigned int        nbr_to_read;
    unsigned int        timeout_cnt;

    unsigned char       claimed;
    unsigned char       unused[2];
    struct clk      *clk;

    unsigned char baud_default; /* first open port, vaule==1, else value==0 */

    struct reset_control *rstc;
    bool                is_console;
    struct ktermios termios;
};

static struct ak_uart_port *ak_serial_ports[NR_PORTS] = {0};
static struct uart_driver ak_uart_drv;
static struct console ak_serial_console;

/* macros to change one thing to another */
#define tx_enabled(port)    ((port)->unused[0])
#define rx_enabled(port)    ((port)->unused[1])

static  void set_reg_bit(int nr,  volatile void __iomem  *addr)
{
    __raw_writel(__raw_readl( addr) | (1 << nr ),addr);
}

static  void clear_reg_bit(int nr,  volatile void __iomem  *addr)
{
    __raw_writel(__raw_readl( addr) | (~(1 << nr )),addr);
}

/**
 * @brief uart_intevent_decode
 *
 * @param [in]:
 * @RETURN: int
*/
static int uart_intevent_decode(unsigned long en_status,
        unsigned long status, unsigned int maskbit,
        unsigned int statusbit)
{
    if ((en_status & maskbit) && (status & statusbit))
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
    uart_reg = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    uart_reg &= ~mask;
    __raw_writel(uart_reg, ourport->port.membase + UART_INT_CTRL);
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
    uart_reg = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    uart_reg |= unmask;
    __raw_writel(uart_reg, ourport->port.membase + UART_INT_CTRL);
}

/**
 * @brief uart_subint_clear
 *
 * @param [in]: struct ak_uart_port *ourport
 * @RETURN: void.
*/
static void __attribute__((unused)) uart_subint_clear(struct ak_uart_port *ourport,
    unsigned int subint)
{
    unsigned long uart_reg;

    switch (subint) {

    case STA_TX_TH_INT:
        uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
        uart_reg |= (1<<subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);
    break;

    case RX_TH_INT:
        uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
        uart_reg |= (subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);
        break;

    case RX_FRAME_ERR:
        uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
        uart_reg |= (subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);
        break;

    case RX_TIMEOUT:
        uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
        uart_reg |= (subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);
        break;

    case TX_END:
        uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
        uart_reg |= (subint);
        __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);
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
    uart_reg = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    switch(status)
    {
        case ENABLE:
            uart_reg |= (CFG_TX_END_EN);
            break;

        case DISABLE:
            uart_reg &= ~(CFG_TX_END_EN);
            break;

        default:
            break;
    }
    __raw_writel(uart_reg, ourport->port.membase + UART_INT_CTRL);
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
    while (!(__raw_readl(ourport->port.membase + UART_STATUS) &
        (1 << STA_TX_END))) {
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
            __raw_writel(regval,  AK_VA_L2CTRL + 0x8C);

            regval = __raw_readl(ourport->port.membase + UART_CFG_RX_THRESHOLD);
            __raw_writel(regval & (~RX_TH_EN), ourport->port.membase + UART_CFG_RX_THRESHOLD);

            ourport->rxfifo_offset = 0;
            ourport->rx_th_int_cnt = 0;
            __raw_writel(regval, ourport->port.membase + UART_CFG_RX_THRESHOLD);

            break;

        case TX_STATUS:
            regval |= (0x1 << (16 + ourport->port.line * 2));
            __raw_writel(regval,  AK_VA_L2CTRL + 0x8C);
            break;

        default:
            break;
    }
    local_irq_restore(flags);
}

/* clear TX and RX internal status */
static inline void clear_internal_status(struct ak_uart_port *ourport,
    unsigned short status)
{
    unsigned long regval;
    unsigned long flags;

    local_irq_save(flags);

    regval = __raw_readl(ourport->port.membase + UART_CFG);
    switch(status)
    {
        case RX_STATUS:
            __raw_writel(regval | CFG_RX_CLR, ourport->port.membase +
                UART_CFG);

            //udelay(1);

            __raw_writel(regval, ourport->port.membase +
                UART_CFG);

            break;

        case TX_STATUS:
//            __raw_writel(regval | CFG_TX_CLR, ourport->port.membase +
//                UART_CFG);
//
//            __raw_writel(regval, ourport->port.membase +
//                UART_CFG);

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

    regval = __raw_readl(ourport->port.membase + UART_STATUS);
    switch(status)
    {
        case RX_STATUS:
            __raw_writel(regval | (0x1 << STA_RX_TH_INT), ourport->port.membase + UART_STATUS);
            break;

        case TX_STATUS:
            __raw_writel(regval | (0x1 << STA_TX_TH_INT), ourport->port.membase + UART_STATUS);
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

    regval = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    if(status)
        __raw_writel(regval | (CFG_RX_TH_EN), ourport->port.membase +
            UART_INT_CTRL);
    else
        __raw_writel(regval & ~(CFG_RX_TH_EN), ourport->port.membase +
            UART_INT_CTRL);

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
    uart_reg = __raw_readl(port->membase + UART_STATUS);

    if (uart_reg & (1 << STA_TX_FIFO_EMPTY))
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
    unsigned long flags;
    unsigned int regval;
    pr_debug("%s\n", __FUNCTION__);


    if (tx_enabled(port))
    {
//        uart_txend_interrupt(ourport, DISABLE);
        tx_enabled(port) = 0;
        local_irq_save(flags);
        regval = __raw_readl(ourport->port.membase + UART_INT_CTRL);
        regval &= ~CFG_TX_EMPTY_EN;
        __raw_writel(regval, ourport->port.membase + UART_INT_CTRL);
        local_irq_restore(flags);
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
//    unsigned int l2_offset = 0;
    unsigned long regval;

    /* if there is not anything more to transmit, or the uart is now
     * stopped, disable the uart and exit
     */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
    {
        ak_serial_stop_tx(port);
        return;
    }

    /* stop tx */
//    regval = __raw_readl(ourport->port.membase + UART_CFG);
//    regval &= ~CFG_TX_EN;
//    regval |= CFG_TX_CLR;
//    __raw_writel(regval, ourport->port.membase + UART_CFG);

//    regval = __raw_readl(ourport->port.membase + UART_CFG_TX_THRESHOLD);
//    regval &= ~(TX_TH_EN |TX_TH_CFG_MASK);
//    __raw_writel(regval, ourport->port.membase + UART_CFG_TX_THRESHOLD);

    txcount = uart_circ_chars_pending(xmit);

    if (txcount > 9) txcount = 9;

    pbuf = ourport->txfifo_base;
    pxmitbuf = xmit->buf;

    /* clear a uartx buffer status */
    clear_uart_buf_status(ourport, TX_STATUS);

    /* clear the tx internal status */
    clear_internal_status(ourport, TX_STATUS);

//    l2_offset = 0;
    tx_tail = xmit->tail;
//    regval = 0;

    for(i = 0; i < txcount; i++)
    {
        /* check tx_fifo full */
//        while((__raw_readl(ourport->port.membase + UART_STATUS) & (0x1 << STA_TX_FIFO_FULL)) != 0);

       /* write data to tx_fifo */
        regval = (pxmitbuf[tx_tail] << TX_DAT);
        __raw_writel(regval, ourport->port.membase + UART_TX_RX_DATA);//DMA disable

        /* wait for tx finish */
//        while((__raw_readl(ourport->port.membase + UART_STATUS) & (0x1 << STA_TX_END)) == 0);

        tx_tail = (tx_tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx += 1;
    }

    xmit->tail = tx_tail;

    /* enable tx_end_int */
//    uart_txend_interrupt(ourport, ENABLE);

    regval = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    regval |= CFG_TX_EMPTY_EN;
    __raw_writel(regval, ourport->port.membase + UART_INT_CTRL);

//    regval = __raw_readl(ourport->port.membase + UART_CFG);
//    regval |= CFG_TX_EN;
//    __raw_writel(regval, ourport->port.membase + UART_CFG);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    //if (uart_circ_empty(xmit))
    //  ak_serial_stop_tx(port);
}

static void ak_serial_start_tx(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);
//    unsigned long flags;
//    unsigned int regval;

    pr_debug("%s\n", __FUNCTION__);

    if (!tx_enabled(port))
    {
        tx_enabled(port) = 1;
        while((__raw_readl(ourport->port.membase + UART_STATUS) & TX_FIFO_EMPTY) == 0);
        ak_handle_tx(ourport);
//        local_irq_save(flags);
//        regval = __raw_readl(ourport->port.membase + UART_INT_CTRL);
//        regval |= CFG_TX_EMPTY_EN;
//        __raw_writel(regval, ourport->port.membase + UART_INT_CTRL);
//        local_irq_restore(flags);
    }
}

static irqreturn_t ak_uart_irqhandler(int irq, void *dev_id)
{
    struct ak_uart_port *ourport = dev_id;
    struct uart_port *port = &ourport->port;

    struct tty_port *tty = &(port->state->port);    //port->info->tty;

    unsigned int flag= TTY_NORMAL;

    unsigned char __iomem   *pbuf;
    unsigned long uart_status, clear;
    unsigned long uart_en_status;
    unsigned long uart_timeout_status;
    unsigned int rxcount, i;
    unsigned char ch = 0;
//    unsigned int reg;
    unsigned int l2_offset = 0;
    unsigned int hdl_receive = 0;

    port->icount.brk++;
    uart_status = __raw_readl(ourport->port.membase + UART_STATUS);
    uart_en_status = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    uart_timeout_status = __raw_readl(ourport->port.membase + UART_RECEIVE_TIMEOUT);

    /* clear rx error*/
    clear = uart_status & (RX_FIFO_OVER_ERR | RX_NOISE_ERR | RX_FRAME_ERR | RX_PAR_ERR | RX_START_ERR | BREAK_ERR);

    /* clear R_err interrupt */
    if ( uart_intevent_decode(uart_en_status, uart_status, CFG_FE_EN, RX_FRAME_ERR) )
    {
        clear |= RX_FRAME_ERR;
    }

    if (uart_intevent_decode(uart_en_status, uart_status, CFG_RX_TH_EN, RX_TH_INT)) {
        clear |= RX_TH_INT;
        ourport->rx_th_int_cnt++;
        ourport->rx_th_int_cnt %= (UART_RX_FIFO_SIZE / ourport->rx_th);
        hdl_receive = 1;
    }

    if (uart_intevent_decode(uart_timeout_status, uart_status, CFG_RX_TIMEOUT_EN, RX_TIMEOUT)) {
        clear |= RX_TIMEOUT;
        hdl_receive = 1;
    }

    if ( hdl_receive )
        rxcount = (__raw_readl(ourport->port.membase + UART_CFG_RX_THRESHOLD) >> 16) & 0x3FF;

    //尽快清理中断状态标志
    __raw_writel(clear, ourport->port.membase + UART_STATUS);

    if ( uart_intevent_decode(uart_en_status, uart_status, CFG_TX_EMPTY_EN, TX_FIFO_EMPTY))
    {
        ak_handle_tx(ourport);
    }

    /* rx threshold interrupt */
    if ( hdl_receive )
    {
        ourport->nbr_to_read = rxcount + ourport->rx_th_int_cnt * ourport->rx_th;

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
    }
    #if 0
    else if (uart_intevent_decode(uart_en_status, uart_status, RX_FIFO_NRDY_EN, RX_FIFO_NRDY))
    {
        rxcount = peri_hub_get_fifo_datalen(PERI_HUB_UART0_RX + port->line*2);
        for(i=0; i<rxcount; i++)
        {
            pbuf= (unsigned char*)AK_SSRAM_VA_ADDR(peri_hub_get_raddr(PERI_HUB_UART0_RX + port->line*2));
            ch = __raw_readb(pbuf);
            uart_insert_char(port, 0, 0, ch, flag);
            port->icount.rx += 1;
        }
        tty_flip_buffer_push(tty);
    }
    #endif

    return IRQ_HANDLED;
}/* end of func */

static void ak_serial_shutdown(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);

    /* mask all interrupt */
    __raw_writel(0, ourport->port.membase + UART_INT_CTRL);

    ak_wait_for_txend(ourport);

    //write 1 to clear
    /*clear rx_th_interrupt,prevent counting*/
    set_reg_bit(STA_RX_TH_INT,ourport->port.membase + UART_STATUS);

    /*rx_sta_clr*/
    set_reg_bit(28,ourport->port.membase + UART_CFG);

    /* uart disable interface*/
    clear_reg_bit(25,ourport->port.membase + UART_CFG);

    free_irq(port->irq, ourport);

    clk_disable_unprepare(ourport->clk);

    rx_enabled(ourport) = 0;
    tx_enabled(ourport) = 0;
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

    if ((ourport->rstc) && (ourport->is_console == false)) {
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

    uart_reg = __raw_readl(ourport->port.membase + UART_FLOW_CTRL);
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
                uart_reg |=  (1<<CFG_CTS_SEL);
            else
                uart_reg &=  ~(1<<CFG_CTS_SEL);

            /*
             * Configuration Register 1 of UART
             * 0x00
             * bit[19]
             * 0 = RTS active high
             * 1 = RTS active low
             * */
            if(!!ourport->rts_active_low)
                uart_reg |=  (1<<CFG_RTS_SEL);
            else
                uart_reg &=  ~(1<<CFG_RTS_SEL);
    }else
    {
        pr_debug("%s: Not support Hardware flow control",__func__);
        //uart_reg &= ~(1<<18|1<<19);
    }

    //__raw_writel(uart_reg, ourport->port.membase + UART_FLOW_CTRL);

    /* mask all interrupt */
    __raw_writel(0, ourport->port.membase + UART_INT_CTRL);


    /*
     * config stop bit and timeout value
     * set timeout = 32, stop bit = 1;
     */
    //uart_reg = (0x1f << 16);
    //__raw_writel(uart_reg, ourport->port.membase + UART_STOPBIT_TIMEOUT);

    uart_reg = __raw_readl(ourport->port.membase + UART_CFG);
    uart_reg &= ~CFG_RX_EN;
    uart_reg |= CFG_RX_CLR;
    __raw_writel(uart_reg, ourport->port.membase + UART_CFG);

    while (__raw_readl(ourport->port.membase + UART_CFG) & CFG_RX_CLR);


   /*
      * set threshold to 32bytes
      * set RX_th_cfg_h = 0, set RX_th_cfg_l = 31
      */
    #if 1
    ourport->rx_th = 64;//uartv2只能使用阈值寄存器的rx_th_cnt作为L2的偏移, 因此只能使用能128整除的阈值
                        //2025-3-11
                        //H322D没有可查询的L2的接收偏移值, 使用阈值rx_cnt实现类似的功能
                        //为了防止中断延时大导致计算偏移错误的问题, 建议越大越好, 但要小于128.
                        //115200, 32byte大约2.5ms. 64byte大约5ms.
                        //因此建议使用64
    ourport->rx_th_int_cnt = 0;
    uart_reg = __raw_readl(ourport->port.membase + UART_CFG_RX_THRESHOLD);
    uart_reg &= ~RX_TH_CFG_MASK;
    uart_reg |= ((ourport->rx_th - 1) << RX_TH_CFG);
    uart_reg |= RX_TH_EN;
    __raw_writel(uart_reg, ourport->port.membase + UART_CFG_RX_THRESHOLD);

    uart_reg = __raw_readl(ourport->port.membase + UART_ALTER_FUNC_CTRL);
    uart_reg |= (0x01UL << CFG_DMA_RX);
    __raw_writel(uart_reg, ourport->port.membase + UART_ALTER_FUNC_CTRL);
    #endif

    clear_internal_status(ourport, RX_STATUS);

    /* ourport->rxfifo_offset = 0; */
    ourport->rxfifo_offset = 0;

    /* to clear  RX_th count interrupt */
    uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
    uart_reg |= (1<<STA_RX_TH_INT);
    __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);

    /*
     * rx timeout set
     */
    uart_reg = __raw_readl(ourport->port.membase + UART_RECEIVE_TIMEOUT);
    uart_reg &= ~(0xFFFF << CFG_RX_TIMEOUT);
    uart_reg |= (CFG_RX_TIMEOUT_EN);
    uart_reg |= 0x280 << CFG_RX_TIMEOUT;
    __raw_writel(uart_reg, ourport->port.membase + UART_RECEIVE_TIMEOUT);
    /*
     *  enable timeout, rx mem_rdy and rx_th tx_end interrupt
     */
    uart_reg = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    uart_reg = (CFG_RX_TH_EN | CFG_TIMEOUT_EN | CFG_FE_EN);
    __raw_writel(uart_reg, ourport->port.membase + UART_INT_CTRL);

    uart_reg = __raw_readl(ourport->port.membase + UART_CFG);
    uart_reg |= (CFG_TX_EN | CFG_RX_EN | CFG_UART_EN);
    __raw_writel(uart_reg, ourport->port.membase + UART_CFG);

    rx_enabled(port) = 1;
    tx_enabled(port) = 0;

    /* register interrupt */
    ret = request_irq(port->irq, ak_uart_irqhandler, 0/*IRQF_DISABLED*/,
        ourport->name, ourport);
    if (ret) {
        printk(KERN_ERR "can't request irq %d for %s\n", port->irq,
            ourport->name);
        pr_info("can't request irq %d for %s\n", port->irq,
            ourport->name);
        goto startup_err;
    }

    return 0;

startup_err:
    ak_serial_shutdown(port);
    return ret;
}/* end of func */

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
/**
 * @BRIEF:  uart_get_BRD
 * @PARAM:  m, n, frac, frac_num
 % @NOTE:   除数m; 被除数n; frac[]-存储小数后frac_num位
 */
static void uart_get_BRD(unsigned int m, unsigned int n, unsigned char *frac, unsigned char frac_num)
{
    unsigned int integer, num, i;

    n = 16 * n;

    integer = m / n;

    num = m - (n * integer);

    for(i=0; i<frac_num; i++)
    {
        num = num * 10;
        frac[i] = num / n;

        num = num - (frac[i] * n);
    }
}

/**
 * @BRIEF:  uart_get_DLF
 * @PARAM:  frac, frac_num
 % @NOTE:   frac[]-存储小数后frac_num位
 */
static unsigned int uart_get_DLF(unsigned char *frac, unsigned char frac_num)
{
    unsigned int num = 0x0;
    unsigned int num_10 = 1;
    unsigned int i,fraction;

    for(i=frac_num; i>0; i--)
    {
        num = frac[i-1] * num_10 + num;
        num_10 = num_10 * 10;
    }

    num = num * 16;

    fraction = num / num_10;

    num = num - fraction * num_10;

    if((num / (num_10 / 10) ) >= 5)
        fraction++;

    return fraction;
}

/**
 * @BRIEF:  uart_get_baud_fraction
 * @PARAM:  clk, baudrate
 * @NOTE:   举例clk=200M,波特率为4800bps,分频器中有4位小数分频bit[0:3]-即2的4次方=16
            BRD = clk / 2的4次方(16) / buadrate = 2604.16666667
            所以整数分频为 2604
            小数分频为0.16666667

            DLF = BRD(F) * 2的4次方(16) = 2.666672 = 3(四舍五入)
            所以定点化后的小数分频值为3
            以上内容是uart_get_BRD 及 uart_get_DLF内容
 */
static unsigned char uart_get_baud_fraction(unsigned int clk, unsigned int baudrate)
{
    unsigned char frac_num = 6;//取小数后6位
    unsigned char frac[6];

    uart_get_BRD(clk, baudrate, frac, frac_num);

    return uart_get_DLF(frac, frac_num);
}
#endif

static void ak_serial_set_termios(struct uart_port *port,
                struct ktermios *termios, struct ktermios *old)
{
    struct ak_uart_port *ourport = to_ourport(port);
    unsigned int baud;
    unsigned long flags;
    unsigned int  regval;

#ifdef CONFIG_MACH_AK3918AV100_FPGA
    //FPGA use default clock 60M
    unsigned long asic_clk = 60000000;
#else
    unsigned long asic_clk = ourport->port.uartclk;
#endif

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    unsigned int baud_div_interger = 0;
    unsigned int baud_div_fraction = 0;
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
    if (baud == ourport->baud && ourport->is_console)
        goto l_set_baud_end;

    ak_wait_for_txend(ourport);

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
    /* compute division value */
    unsigned long baud_div = (asic_clk / baud - 1);

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
    //__raw_writel(regval, port->membase + UART_DIV_FACTOR);

    /* baudrate setting */
    regval = __raw_readl(port->membase + UART_CONF1);
    regval &= ~(0xffff);
    regval &= ~(0x1 << 22);
    regval |= (baud_div & 0xffff);
    regval |= (1 << 28) | (1 << 29);

    if (asic_clk % baud)
        regval |= (0x1 << 22);

#elif defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    /* compute division value */
    /*
    UART TXD在部分条件下概率性多发一个字节
    解决方案:
    1. 小数分频系数配置成小于等于8, 整数分频系数不限制
    2. 小数分频系数配置成大于8, 整数分频系数配置成奇数
    3. 每次发送数据前进行一次复位,(清楚或者禁能TX端)
    */

    baud_div_interger = asic_clk / 16 / baud;
    if (asic_clk % (16 * baud)) {
        baud_div_fraction = uart_get_baud_fraction(asic_clk, baud);
    }

    pr_debug("asic_clk %u, baud %u, baud_div %u %u\n",
        (unsigned int)asic_clk, baud, baud_div_interger, baud_div_fraction);

    /* baudrate setting */
    regval = __raw_readl(ourport->port.membase + UART_BAUD_DIV);
    regval &= ~((0xffffUL << 16) | 0xF);
    regval |= (baud_div_interger << 16) | baud_div_fraction;
    __raw_writel(regval, ourport->port.membase + UART_BAUD_DIV);
#endif

    clear_uart_buf_status(ourport, TX_STATUS);
    clear_uart_buf_status(ourport, RX_STATUS);

    clear_internal_status(ourport, RX_STATUS);
    clear_internal_status(ourport, TX_STATUS);


    pr_debug("port->line =%d ,termios->c_cflag =%x. port->status =%x\n",
        port->line, termios->c_cflag,port->status);
l_set_baud_end:
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

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
    /* parity setting */
    if (termios->c_cflag & PARENB) {
        if (termios->c_cflag & PARODD)
            regval |= (0x2 << 25);  /* odd parity */
        else
            regval |= (0x3 << 25);  /* evnt parity*/
    }

    __raw_writel(regval, port->membase + UART_CONF1);
#elif defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    /* parity setting */
    if (termios->c_cflag & PARENB) {
        regval = __raw_readl(port->membase + UART_CFG);
        regval &= ~(0x3 << 12);
        if (termios->c_cflag & PARODD)
            regval |= (0x0 << 12);  /* odd parity */
        else
            regval |= (0x1 << 12);  /* evnt parity*/
        __raw_writel(regval, port->membase + UART_CFG);
    }
#endif
    /*
     * Update the per-port timeout.
     */
    uart_update_timeout(port, termios->c_cflag, baud);

    /*
     * Which character status flags should we ignore?
     */
    port->ignore_status_mask = 0;
    memcpy(&ourport->termios, termios, sizeof(struct ktermios));
    spin_unlock_irqrestore(&port->lock, flags);
} /* end of func */

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

static int ak_uart_mem_resume(struct uart_port *port)
{
    struct ak_uart_port *ourport = to_ourport(port);
    unsigned long uart_reg;
    //int ret = 0;
    const struct uart_ops *ops = port->ops;

    if (!tx_enabled(port) && !rx_enabled(port))
        return 0;

#ifdef CONFIG_MACH_AK3918AV100_FPGA
    ourport->port.uartclk = 60000000;
#else
    ourport->port.uartclk = clk_get_rate(ourport->clk);
#endif
    //clear L2 Buffer
    clear_uart_buf_status(ourport, RX_STATUS);

    uart_reg = __raw_readl(ourport->port.membase + UART_FLOW_CTRL);
    if (port->flags & UPF_HARD_FLOW) {
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
                uart_reg |=  (1<<CFG_CTS_SEL);
            else
                uart_reg &=  ~(1<<CFG_CTS_SEL);

            /*
             * Configuration Register 1 of UART
             * 0x00
             * bit[19]
             * 0 = RTS active high
             * 1 = RTS active low
             * */
            if(!!ourport->rts_active_low)
                uart_reg |=  (1<<CFG_RTS_SEL);
            else
                uart_reg &=  ~(1<<CFG_RTS_SEL);
    } else {
        pr_info("%s: Not support Hardware flow control",__func__);
        //uart_reg &= ~(1<<18|1<<19);
    }

    //__raw_writel(uart_reg, ourport->port.membase + UART_FLOW_CTRL);

    /* mask all interrupt */
    __raw_writel(0, ourport->port.membase + UART_INT_CTRL);


    /*
     * config stop bit and timeout value
     * set timeout = 32, stop bit = 1;
     */
    //uart_reg = (0x1f << 16);
    //__raw_writel(uart_reg, ourport->port.membase + UART_STOPBIT_TIMEOUT);

    uart_reg = __raw_readl(ourport->port.membase + UART_CFG);
    uart_reg &= ~CFG_RX_EN;
    uart_reg |= CFG_RX_CLR;
    __raw_writel(uart_reg, ourport->port.membase + UART_CFG);

    while (__raw_readl(ourport->port.membase + UART_CFG) & CFG_RX_CLR);

    //L2
    uart_reg = __raw_readl(AK_VA_L2CTRL + 0x8C);
    uart_reg |= (0x1 << (17 + ourport->port.line * 2));
    __raw_writel(uart_reg,  AK_VA_L2CTRL + 0x8C);

    ops->set_termios(port, &ourport->termios, NULL);


   /*
      * set threshold to 32bytes
      * set RX_th_cfg_h = 0, set RX_th_cfg_l = 31
      */
    #if 1
    ourport->rx_th = 64;//uartv2只能使用阈值寄存器的rx_th_cnt作为L2的偏移, 因此只能使用能128整除的阈值
                        //2025-3-11
                        //H322D没有可查询的L2的接收偏移值, 使用阈值rx_cnt实现类似的功能
                        //为了防止中断延时大导致计算偏移错误的问题, 建议越大越好, 但要小于128.
                        //115200, 32byte大约2.5ms. 64byte大约5ms.
                        //因此建议使用64
    ourport->rx_th_int_cnt = 0;
    uart_reg = __raw_readl(ourport->port.membase + UART_CFG_RX_THRESHOLD);
    uart_reg &= ~RX_TH_CFG_MASK;
    uart_reg |= ((ourport->rx_th - 1) << RX_TH_CFG);
    uart_reg |= RX_TH_EN;
    __raw_writel(uart_reg, ourport->port.membase + UART_CFG_RX_THRESHOLD);

    uart_reg = __raw_readl(ourport->port.membase + UART_ALTER_FUNC_CTRL);
    uart_reg |= (0x01UL << CFG_DMA_RX);
    __raw_writel(uart_reg, ourport->port.membase + UART_ALTER_FUNC_CTRL);
    #endif

    clear_internal_status(ourport, RX_STATUS);

    /* ourport->rxfifo_offset = 0; */
    ourport->rxfifo_offset = 0;

    /* to clear  RX_th count interrupt */
    uart_reg = __raw_readl(ourport->port.membase + UART_STATUS);
    uart_reg |= (1<<STA_RX_TH_INT);
    __raw_writel(uart_reg, ourport->port.membase + UART_STATUS);

    /*
     * rx timeout set
     */
    uart_reg = __raw_readl(ourport->port.membase + UART_RECEIVE_TIMEOUT);
    uart_reg &= ~(0xFFFF << CFG_RX_TIMEOUT);
    uart_reg |= (CFG_RX_TIMEOUT_EN);
    uart_reg |= 0x280 << CFG_RX_TIMEOUT;
    __raw_writel(uart_reg, ourport->port.membase + UART_RECEIVE_TIMEOUT);
    /*
     *  enable timeout, rx mem_rdy and rx_th tx_end interrupt
     */
    uart_reg = __raw_readl(ourport->port.membase + UART_INT_CTRL);
    uart_reg = (CFG_RX_TH_EN | CFG_TIMEOUT_EN | CFG_FE_EN);
    __raw_writel(uart_reg, ourport->port.membase + UART_INT_CTRL);

    uart_reg = __raw_readl(ourport->port.membase + UART_CFG);
    uart_reg |= (CFG_TX_EN | CFG_RX_EN | CFG_UART_EN);
    __raw_writel(uart_reg, ourport->port.membase + UART_CFG);

    if (tx_enabled(port)) {
        while((__raw_readl(ourport->port.membase + UART_STATUS) & TX_FIFO_EMPTY) == 0);
        ak_handle_tx(ourport);
    }
  //  ops->set_termios(port, &ourport->termios, NULL);
    return 0;

//startup_err:
//    ak_serial_shutdown(port);
//    return ret;
}

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
#if 0
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
#endif
    return 0;
}

static int ak_serial_resume(struct platform_device *dev)
{
#if 1
    struct uart_port *uport = (struct uart_port *)dev_get_drvdata(&dev->dev);

    pinctrl_pm_select_default_state(&dev->dev);
    ak_uart_mem_resume(uport);
#else
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
#endif
    return 0;
}
#else
#define ak_serial_suspend NULL
#define ak_serial_resume    NULL
#endif

static const struct of_device_id ak_uart_of_ids[] = {
    { .compatible = "anyka,ak3918av130-uart0" },
    { .compatible = "anyka,ak3918av130-uart1" },
    { .compatible = "anyka,ak3918av130-uart2" },
    { .compatible = "anyka,km01a-uart0" },
    { .compatible = "anyka,km01a-uart1" },
    { .compatible = "anyka,km01a-uart2" },
    {},
};
MODULE_DEVICE_TABLE(of, ak_uart_of_ids);

#ifdef CONFIG_SERIAL_AK_CONSOLE
static inline void ak_uart_putchar(struct ak_uart_port *ourport,
    unsigned char ch)
{
    //wait fifo not full
//    while(readl(ourport->port.membase + UART_STATUS) & (0x1 << STA_TX_FIFO_FULL));

    __raw_writel(0x01UL << STA_TX_END, ourport->port.membase + UART_STATUS);

    writel(ch << TX_DAT, ourport->port.membase + UART_TX_RX_DATA);//DMA disable

    while(0 == ((0x01UL << STA_TX_END) & __raw_readl(ourport->port.membase + UART_STATUS)));
}

static inline void ak_uart_send(struct ak_uart_port *ourport, const char *s,
    unsigned int count)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
        //wait fifo not full
        while(readl(ourport->port.membase + UART_STATUS) & (0x1 << STA_TX_FIFO_FULL));

        writel(s[i] << TX_DAT, ourport->port.membase + UART_TX_RX_DATA);

    }
}

static void ak_serial_console_putchar(struct uart_port *port, int ch)
{
    struct ak_uart_port *ourport = to_ourport(port);

    ak_uart_putchar(ourport, ch);
}

static void ak_serial_console_write(struct console *co, const char *s,
    unsigned int count)
{
    static struct ak_uart_port * up;
    unsigned long flags;
    unsigned int regval;

    up = ak_serial_ports[co->index];

    local_irq_save(flags);
    regval = __raw_readl(up->port.membase + UART_INT_CTRL);
    if (regval & CFG_TX_EMPTY_EN)
    {
        __raw_writel(regval & ~CFG_TX_EMPTY_EN, up->port.membase + UART_INT_CTRL);
    }
    local_irq_restore(flags);

    uart_console_write(&up->port, s, count, ak_serial_console_putchar);

    if (regval & CFG_TX_EMPTY_EN) {
        local_irq_save(flags);
        regval = __raw_readl(up->port.membase + UART_INT_CTRL);
        __raw_writel(regval | CFG_TX_EMPTY_EN, up->port.membase + UART_INT_CTRL);
        local_irq_restore(flags);
    }

}

void ak_serial_console_send(struct uart_port *port, const char *s,
    unsigned int count)
{
    struct ak_uart_port *ourport = to_ourport(port);

    ak_uart_send(ourport, s, count);
}
EXPORT_SYMBOL_GPL(ak_serial_console_send);


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

    uap->is_console = true;

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
/* module initialisation code */
static int __init ak_serial_modinit(void)
{
    int ret;

    printk("AKxx uart driver init, (c) 2023 ANYKA\n");
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
MODULE_VERSION("2.0.01");
