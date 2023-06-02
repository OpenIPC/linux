/*
 * MUSB OTG driver - support for Mentor's DMA controller
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2007 by Texas Instruments
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include "musb_core.h"
#include "musbhsdma.h"

#include <linux/interrupt.h>
#include <linux/irq.h>

#if CONFIG_USB_MUSB_GK
#include <plat/dma.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE
#include <mach/io.h>
#include <plat/cache.h>
#include <mach/dma.h>


#define SIZE_4M            0x00400000 /* 4M */
#define DMA_LONG_WORD_MASK 0x3
#define DMA_CHAN           3
#endif

struct semaphore           sema_trans;

unsigned long time_start, time_end, time;

unsigned long time_1_start, time_1_end, time_1;


struct gk_dma_channel {
	struct dma_channel channel;
	struct gk_dma_controller *controller;
	struct musb_hw_ep *hw_ep;
	struct dma_chan *dma_chan;
	unsigned int cur_len;
	gk_dma_req_t req;
	//dma_cookie_t cookie;
	u8 ch_num;
	u8 is_tx;
	u8 is_allocated;
	u16 max_packet_sz;
    u16 short_packet;
};

struct gk_dma_controller {
	struct dma_controller controller;
	struct gk_dma_channel rx_channel;
	struct gk_dma_channel tx_channel;

	void *private_data;
	void __iomem *base;

	u8 ch_num;
	u8 is_tx;
};

struct gk_dma_controller *controller_irq;
struct gk_dma_controller *controller_irq_rx;
u8 usb_short_pack_flag=0;

void gk_usb_dma_irq_tx(void *dev_id, u32 status)
{
    u8  ep;
    u8 *mbase;
	u8  devctl;
	struct gk_dma_channel    *gk_channel;
	struct gk_dma_controller *controller = NULL;
	struct musb                  *musb       = NULL;
    unsigned long                 flags;

    controller = (struct gk_dma_controller *)controller_irq;

    if (controller == NULL) {
        printk("ERROR : controller is null\n");
        return;
    }

    musb = controller->private_data;
    mbase = musb->mregs;
    spin_lock_irqsave(&musb->lock, flags);

    gk_channel = &(controller->tx_channel);
	gk_channel->channel.status = MUSB_DMA_STATUS_FREE;
    devctl = musb_readb(mbase, MUSB_DEVCTL);
    ep     = gk_channel->hw_ep->epnum;

	//printk("d1t irq=0x%d\n",irq);
    if (gk_channel->channel.desired_mode)
	{
        int offset = MUSB_EP_OFFSET(ep, MUSB_TXCSR);
        u16 txcsr,tmp,flag;
        //printk("d1t\n");
        /*
         * The programming guide says that we
         * must clear DMAENAB before DMAMODE.
         */
        musb_ep_select(mbase, ep);
        txcsr  = musb_readw(mbase, offset);
        tmp=txcsr;
        flag=0;
        if(tmp&(MUSB_TXCSR_TXPKTRDY|MUSB_TXCSR_FIFONOTEMPTY))
        {
            //printk("d1t\n");
#if defined(CONFIG_GK_USB_HOST_MODE)
            txcsr &= ~(MUSB_TXCSR_DMAENAB
                       | MUSB_TXCSR_AUTOSET|MUSB_TXCSR_TXPKTRDY);
            musb_writew(mbase, offset, txcsr|MUSB_TXCSR_H_WZC_BITS);

            txcsr &= ~(MUSB_TXCSR_DMAMODE|MUSB_TXCSR_TXPKTRDY);
            musb_writew(mbase, offset, txcsr|MUSB_TXCSR_H_WZC_BITS);
            tmp  = musb_readw(mbase, offset);
	#ifdef CONFIG_USB_INVENTRA_DMA
        if((tmp&(MUSB_TXCSR_TXPKTRDY|MUSB_TXCSR_FIFONOTEMPTY))==0)
            musb_dma_completion(musb,ep,1);
	#endif
#else
            txcsr &= ~(MUSB_TXCSR_DMAENAB
                       | MUSB_TXCSR_AUTOSET|MUSB_TXCSR_TXPKTRDY);
            musb_writew(mbase, offset, txcsr|MUSB_TXCSR_P_WZC_BITS);

            txcsr &= ~(MUSB_TXCSR_DMAMODE|MUSB_TXCSR_TXPKTRDY);
            musb_writew(mbase, offset, txcsr|MUSB_TXCSR_P_WZC_BITS);
            tmp  = musb_readw(mbase, offset);
     #ifdef CONFIG_USB_INVENTRA_DMA
            if((tmp&(MUSB_TXCSR_TXPKTRDY|MUSB_TXCSR_FIFONOTEMPTY))==0)
                musb_dma_completion(musb,ep,1);
     #endif
#endif
        }
        else
        {
            //printk("d0t\n");
            txcsr &= ~(MUSB_TXCSR_DMAENAB
                       | MUSB_TXCSR_AUTOSET);
            musb_writew(mbase, offset, txcsr);
            txcsr &= ~(MUSB_TXCSR_DMAMODE);
            musb_writew(mbase, offset, txcsr);
#ifdef CONFIG_USB_INVENTRA_DMA
			usb_short_pack_flag=0;
            if(gk_channel->short_packet)
            {
                txcsr |= MUSB_TXCSR_TXPKTRDY;
#if defined(CONFIG_GK_USB_HOST_MODE)
       			 musb_writew(mbase, offset, txcsr);
#endif
                 usb_short_pack_flag=1;
                 gk_channel->short_packet=0;
            }
else
                 musb_dma_completion(musb,ep,1);
#endif
         }
    }
	else
    {
        int offset = MUSB_EP_OFFSET(ep, MUSB_TXCSR);
        u16 txcsr;
        //printk("0t\n");
        /*
         * The programming guide says that we
         * must clear DMAENAB before DMAMODE.
         */
        musb_ep_select(mbase, ep);
        txcsr  = musb_readw(mbase, offset);
        txcsr &= ~(MUSB_TXCSR_DMAENAB |
        		   MUSB_TXCSR_DMAMODE |
        		   MUSB_TXCSR_AUTOSET );
        txcsr |= MUSB_TXCSR_TXPKTRDY;
        musb_writew(mbase, offset, txcsr);
    }


    spin_unlock_irqrestore(&musb->lock, flags);
}

void gk_usb_dma_irq_rx(void *dev_id, u32 status)
{
    u8  ep;
    u8 *mbase;
	u8  devctl;

	struct gk_dma_channel    *gk_channel;
	struct gk_dma_controller *controller = NULL;
	struct musb                  *musb       = NULL;
    unsigned long                 flags;
	//printk("d1t irq=0x%d\n",status);

    controller = (struct gk_dma_controller *)controller_irq_rx;

    if (controller == NULL) {
        printk("ERROR : controller is null\n");
        return;
    }

    musb = controller->private_data;
    mbase = musb->mregs;

    spin_lock_irqsave(&musb->lock, flags);

    gk_channel = &(controller->rx_channel) ;

	gk_channel->channel.status = MUSB_DMA_STATUS_FREE;

    devctl = musb_readb(mbase, MUSB_DEVCTL);
    ep     = gk_channel->hw_ep->epnum;

#ifdef CONFIG_USB_INVENTRA_DMA
    musb_dma_completion(musb,ep,0);
#endif

    spin_unlock_irqrestore(&musb->lock, flags);
}

static struct dma_channel *gk_dma_channel_allocate(struct dma_controller *c,
				struct musb_hw_ep *hw_ep, u8 is_tx)
{
	struct gk_dma_controller *controller = container_of(c,
			struct gk_dma_controller, controller);
	struct gk_dma_channel *gk_channel = NULL;
	static int dma_alloc_rx_init = 0,dma_alloc_tx_init = 0;

	if (hw_ep->epnum == 0) {
		return NULL;
	}

	gk_channel = is_tx ? &(controller->tx_channel) : &(controller->rx_channel) ;

	/* Check if channel is already used. */

    controller->ch_num = hw_ep->epnum;
    controller->is_tx = is_tx;

	gk_channel->hw_ep = hw_ep;
	gk_channel->is_allocated = 1;
	gk_channel->ch_num = hw_ep->epnum;
    gk_channel->is_tx = is_tx;

	gk_channel->channel.private_data = gk_channel;
	gk_channel->channel.max_len = SIZE_4M;
	if(!is_tx && dma_alloc_rx_init == 0)
	{
		dma_alloc_rx_init = 1;
		gk_channel->channel.status = MUSB_DMA_STATUS_FREE;
	}
	if(is_tx && dma_alloc_tx_init == 0)
	{
		dma_alloc_tx_init = 1;
		gk_channel->channel.status = MUSB_DMA_STATUS_FREE;
	}

	gk_channel->controller = controller;




	return &(gk_channel->channel);
}

static void gk_dma_channel_release(struct dma_channel *channel)
{
	struct gk_dma_channel *gk_channel = channel->private_data;

	gk_channel->is_allocated = 0;
	channel->status = MUSB_DMA_STATUS_FREE;
	channel->actual_len = 0;

}

static int gk_dma_channel_program(struct dma_channel *channel,
				u16 packet_sz, u8 mode,
				dma_addr_t dma_addr, u32 len)
{
	struct gk_dma_channel    *gk_channel = channel->private_data;
	gk_dma_req_t             *req            = &gk_channel->req;
	struct gk_dma_controller *controller     = gk_channel->controller;
    unsigned char                 trans_size     = 0;


    if(len <= 0) return false;
    if(packet_sz & 0x3) return false;
    if((int)dma_addr & 0x7) return false;

	gk_channel->max_packet_sz = packet_sz;

	channel->status       = MUSB_DMA_STATUS_BUSY;
	channel->desired_mode = mode;

    if ((len % 8)        == 0) {
        trans_size        = 3;
    } else if ((len % 4) == 0) {
        trans_size        = 2;
    } else if ((len % 2) == 0) {
        trans_size        = 1;
    } else {
        trans_size        = 0;
    }

    channel->actual_len = len;
	controller->is_tx   = gk_channel->is_tx;
	gk_channel->hw_ep = channel->hw_ep;



    if(controller->is_tx)
    {
  		controller_irq = controller;
#if defined(CONFIG_GK_MUSB_CON_V1_00)
        req->dst       = (u32)gk_channel->hw_ep->fifo - 0x90000000;
#elif defined(CONFIG_GK_MUSB_CON_V1_10)
		req->dst       = (u32)gk_virt_to_phys((u32)(gk_channel->hw_ep->fifo));
#endif
        if((mode)&&(len%packet_sz))
            gk_channel->short_packet=1;
        else
            gk_channel->short_packet=0;
        req->src       = dma_addr; //ddr addr
        req->xfr_count = len;
        gk_dma_writel(DMA_CHAN_SRC_REG(DMA_CHAN_TX), req->src);
        gk_dma_writel(DMA_CHAN_DST_REG(DMA_CHAN_TX), req->dst);
#if defined(CONFIG_GK_MUSB_CON_V1_00)
        gk_dma_writel(DMA_CHAN_CTR_REG(DMA_CHAN_TX), (0x98000000 | req->xfr_count | (trans_size << 22)));
#elif defined(CONFIG_GK_MUSB_CON_V1_10)
		gk_dma_writel(DMA_CHAN_CTR_REG(DMA_CHAN_TX), (0x9B000000 | req->xfr_count | (trans_size << 22)));
#endif

	}
    else
    {
		controller_irq_rx = controller;
        req->src       = (u32)gk_virt_to_phys((u32)(gk_channel->hw_ep->fifo));
        req->dst       = dma_addr; //ddr addr
        req->xfr_count = len;
        gk_dma_writel(DMA_CHAN_DST_REG(DMA_CHAN_RX), req->dst);
        gk_dma_writel(DMA_CHAN_SRC_REG(DMA_CHAN_RX), req->src);
        gk_dma_writel(DMA_CHAN_CTR_REG(DMA_CHAN_RX), (0xab000000 | req->xfr_count | (trans_size << 22)));
    }

	return true;
}

static int gk_dma_channel_abort(struct dma_channel *channel)
{
	struct gk_dma_channel *gk_channel = channel->private_data;
	struct gk_dma_controller *controller = gk_channel->controller;
	struct musb *musb = controller->private_data;
	void __iomem *epio = musb->endpoints[gk_channel->hw_ep->epnum].regs;
	u16 csr;


	printk( "dma abort channel=%d, is_tx=%d\n",
		gk_channel->ch_num, gk_channel->is_tx);
    if(gk_channel->is_tx)
    {
        printk("dma status=0x%x\n",gk_dma_readl(DMA_CHAN_STA_REG(DMA_CHAN_TX)));
    }
    else
        printk("dma status=0x%x\n",gk_dma_readl(DMA_CHAN_STA_REG(DMA_CHAN_RX)));
	if(channel->status == MUSB_DMA_STATUS_BUSY) {
		if (gk_channel->is_tx) {
			csr = musb_readw(epio, 0x10 + MUSB_TXCSR);
			csr &= ~(MUSB_TXCSR_AUTOSET |
				 MUSB_TXCSR_DMAENAB);
			musb_writew(epio, 0x10 + MUSB_TXCSR, csr);
            csr &= ~MUSB_TXCSR_DMAMODE;
            musb_writew(epio, 0x10 + MUSB_TXCSR, csr);
		} else {
			csr = musb_readw(epio, 0x10 + MUSB_RXCSR);
			csr &= ~(MUSB_RXCSR_AUTOCLEAR |
				 MUSB_RXCSR_DMAENAB);
			musb_writew(epio, 0x10 + MUSB_RXCSR, csr);
            csr &= ~MUSB_RXCSR_DMAMODE;
            musb_writew(epio, 0x10 + MUSB_RXCSR, csr);
		}
    }
	channel->status = MUSB_DMA_STATUS_FREE;

	return 0;
}

static int gk_dma_controller_stop(struct dma_controller *c)
{
	struct gk_dma_controller *controller = container_of(c,
			struct gk_dma_controller, controller);
	struct dma_channel *channel;


	if(controller->is_tx) channel = &controller->tx_channel.channel;
	else channel = &controller->rx_channel.channel;

	gk_dma_channel_release(channel);

	return 0;
}

static int gk_dma_controller_start(struct dma_controller *c)
{

	return 0;
}

void dma_controller_destroy(struct dma_controller *c)
{
	struct gk_dma_controller *controller = container_of(c,
			struct gk_dma_controller, controller);

	kfree(controller);
}

struct dma_controller *__init
dma_controller_create(struct musb *musb, void __iomem *base)
{
 	struct gk_dma_controller *controller;

	controller = kzalloc(sizeof(*controller), GFP_KERNEL);
	if (!controller)
		return NULL;

	controller->private_data = musb;

	controller->base = base;

	controller->controller.start           = gk_dma_controller_start;
	controller->controller.stop            = gk_dma_controller_stop;
	controller->controller.channel_alloc   = gk_dma_channel_allocate;
	controller->controller.channel_release = gk_dma_channel_release;
	controller->controller.channel_program = gk_dma_channel_program;
	controller->controller.channel_abort   = gk_dma_channel_abort;

    printk("dma controller create ok\n");

	return &controller->controller;
}

struct dma_controller *
dma_controller_create_non_init(struct musb *musb, void __iomem *base)
{

 	struct gk_dma_controller *controller;
	int err;

	controller = kzalloc(sizeof(*controller), GFP_KERNEL);
	if (!controller)
		return NULL;

	controller->private_data = musb;

	controller->base = base;

	controller->controller.start           = gk_dma_controller_start;
	controller->controller.stop            = gk_dma_controller_stop;
	controller->controller.channel_alloc   = gk_dma_channel_allocate;
	controller->controller.channel_release = gk_dma_channel_release;
	controller->controller.channel_program = gk_dma_channel_program;
	controller->controller.channel_abort   = gk_dma_channel_abort;
	//controller->controller.is_compatible = gk_dma_is_compatible;

#if 0
    retval = request_irq(DMA_IRQ, gk_usb_dma_irq,
		IRQ_TYPE_LEVEL_HIGH | SA_SHIRQ, "usb-dma", NULL);
	if (retval) {
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_IRQ, retval);
	}
#endif

	//usb_dma_irq = gk_usb_dma_irq;
#ifdef CONFIG_USB_INVENTRA_DMA
		err = gk_dma_request_irq(DMA_CHAN_TX,
				gk_usb_dma_irq_tx, NULL);
		if (err < 0)
			return NULL;
		err = gk_dma_enable_irq(DMA_CHAN_TX,
				gk_usb_dma_irq_tx);
		if (err < 0)
			return NULL;
#if defined(CONFIG_GK_MUSB_CON_V1_10)
		err = gk_dma_request_irq(DMA_CHAN_RX,
				gk_usb_dma_irq_rx, NULL);
		if (err < 0)
			return NULL;
		err = gk_dma_enable_irq(DMA_CHAN_RX,
				gk_usb_dma_irq_rx);
		if (err < 0)
			return NULL;
#endif

#endif
    printk("dma_controller_create_non_init ok\n");

	return &controller->controller;
}


