#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
#include <mach/fh_gmac.h>
#include "fh_gmac.h"
#include "fh_gmac_dma.h"

void GMAC_DMA_ReleaseTxDesc(Gmac_Tx_DMA_Descriptors * desc)
{
	int ter = desc->desc1.bit.end_of_ring;
	desc->desc0.dw = 0;
	desc->desc1.dw = 0;
	/* set termination field */
	desc->desc1.bit.end_of_ring = ter;
}

void GMAC_DMA_DisplayRxDesc(Gmac_Rx_DMA_Descriptors * desc, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		pr_info("\t%d [0x%x]: DES0=0x%x DES1=0x%x DES2=0x%x DES3=0x%x",
			i, (__u32) (&desc[i]), desc[i].desc0.dw,
			desc[i].desc1.dw, desc[i].desc2.dw, desc[i].desc3.dw);
		pr_info("\n");
	}
}

void GMAC_DMA_DisplayTxDesc(Gmac_Tx_DMA_Descriptors * desc, int size)
{
	int i;
	pr_info("Tx desc:\n");
	for (i = 0; i < size; i++) {
		pr_info("\t%d [0x%x]: DES0=0x%x DES1=0x%x BUF1=0x%x BUF2=0x%x",
			i, (__u32) & desc[i], desc[i].desc0.dw,
			desc[i].desc1.dw, desc[i].desc2.dw, desc[i].desc3.dw);
		pr_info("\n");
	}
}

void GMAC_DMA_InitRxDesc(Gmac_Rx_DMA_Descriptors * desc, __u32 size)
{
	int i;
	for (i = 0; i < size; i++) {
		desc->desc0.bit.own = 1;
		desc->desc1.bit.buffer1_size = BUFFER_SIZE_2K - 1;
		if (i == size - 1) {
			desc->desc1.bit.end_of_ring = 1;
		}
		desc++;
	}
}

void GMAC_DMA_InitTxDesc(Gmac_Tx_DMA_Descriptors * desc, __u32 size)
{
	int i;
	for (i = 0; i < size; i++) {
		desc->desc0.bit.own = 0;
		if (i == size - 1) {
			desc->desc1.bit.end_of_ring = 1;
		}
		desc++;
	}
}

void GMAC_DMA_OpMode(Gmac_Object * pGmac)
{

	//op mode, reg 6
	//transmit_store_forward
	//receive_store_forward
	writel(0 << 25 | 1 << 21 | 0 << 2 | 0 << 14,
	       pGmac->remap_addr + REG_GMAC_OP_MODE);
}

void GMAC_DMA_InitDescRings(struct net_device *ndev)
{
	int i;
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct sk_buff *skb;
	__u32 txsize = pGmac->dma_tx_size;
	__u32 rxsize = pGmac->dma_rx_size;
	__u32 bfsize = pGmac->dma_buf_sz;

	pGmac->rx_skbuff_dma = kmalloc(rxsize * sizeof(dma_addr_t), GFP_KERNEL);
	pGmac->rx_skbuff =
	    kmalloc(sizeof(struct sk_buff *) * rxsize, GFP_KERNEL);
	pGmac->rx_dma_descriptors =
	    (Gmac_Rx_DMA_Descriptors *) dma_alloc_coherent(pGmac->dev,
							   rxsize *
							   sizeof
							   (Gmac_Rx_DMA_Descriptors),
							   &pGmac->rx_bus_addr,
							   GFP_KERNEL);
	pGmac->tx_skbuff =
	    kmalloc(sizeof(struct sk_buff *) * txsize, GFP_KERNEL);
	pGmac->tx_dma_descriptors =
	    (Gmac_Tx_DMA_Descriptors *) dma_alloc_coherent(pGmac->dev,
							   txsize *
							   sizeof
							   (Gmac_Tx_DMA_Descriptors),
							   &pGmac->tx_bus_addr,
							   GFP_KERNEL);

	if ((pGmac->rx_dma_descriptors == NULL)
	    || (pGmac->tx_dma_descriptors == NULL)) {
		pr_err("%s:ERROR allocating the DMA Tx/Rx desc\n", __func__);
		return;
	}

	pr_debug("fh gmac (%s) DMA desc rings: virt addr (Rx %p, "
		 "Tx %p)\n\tDMA phy addr (Rx 0x%08x, Tx 0x%08x)\n",
		 ndev->name, pGmac->rx_dma_descriptors,
		 pGmac->tx_dma_descriptors, (__u32) pGmac->rx_bus_addr,
		 (__u32) pGmac->tx_bus_addr);

	for (i = 0; i < rxsize; i++) {
		Gmac_Rx_DMA_Descriptors *desc = pGmac->rx_dma_descriptors + i;

		skb = netdev_alloc_skb_ip_align(ndev, bfsize);
		if (unlikely(skb == NULL)) {
			pr_err("%s: Rx init fails; skb is NULL\n", __func__);
			break;
		}
		pGmac->rx_skbuff[i] = skb;
		pGmac->rx_skbuff_dma[i] =
		    dma_map_single(pGmac->dev, skb->data, bfsize,
				   DMA_FROM_DEVICE);

		desc->desc2.dw = pGmac->rx_skbuff_dma[i];
	}
	pGmac->cur_rx = 0;
	pGmac->dirty_rx = (__u32) (i - rxsize);

	pGmac->dma_buf_sz = bfsize;

	/* TX INITIALIZATION */
	for (i = 0; i < txsize; i++) {
		pGmac->tx_skbuff[i] = NULL;
		pGmac->tx_dma_descriptors[i].desc2.dw = 0;
	}
	pGmac->dirty_tx = 0;
	pGmac->cur_tx = 0;

	/* Clear the Rx/Tx descriptors */
	GMAC_DMA_InitRxDesc(pGmac->rx_dma_descriptors, rxsize);
	GMAC_DMA_InitTxDesc(pGmac->tx_dma_descriptors, txsize);
#ifdef FH_GMAC_DMA_DEBUG
	if (netif_msg_hw(pGmac)) {
		pr_info("RX descriptor ring:\n");
		GMAC_DMA_DisplayRxDesc(pGmac->rx_dma_descriptors, rxsize);
		pr_info("TX descriptor ring:\n");
		GMAC_DMA_DisplayTxDesc(pGmac->tx_dma_descriptors, txsize);
	}
#endif
}

void GMAC_DMA_FreeRxSkbufs(Gmac_Object * pGmac)
{
	int i;

	for (i = 0; i < pGmac->dma_rx_size; i++) {
		if (pGmac->rx_skbuff[i]) {
			dma_unmap_single(pGmac->dev, pGmac->rx_skbuff_dma[i],
					 pGmac->dma_buf_sz, DMA_FROM_DEVICE);
			dev_kfree_skb_any(pGmac->rx_skbuff[i]);
		}
		pGmac->rx_skbuff[i] = NULL;
	}
}

void GMAC_DMA_FreeTxSkbufs(Gmac_Object * pGmac)
{
	int i;

	for (i = 0; i < pGmac->dma_tx_size; i++) {
		if (pGmac->tx_skbuff[i] != NULL) {
			Gmac_Tx_DMA_Descriptors *desc =
			    pGmac->tx_dma_descriptors + i;
			if (desc->desc2.dw) {
				__u32 size;
				size = desc->desc1.bit.buffer1_size;
				dma_unmap_single(pGmac->dev, desc->desc2.dw,
						 size, DMA_TO_DEVICE);
			}
			dev_kfree_skb_any(pGmac->tx_skbuff[i]);
			pGmac->tx_skbuff[i] = NULL;
		}
	}
}

void GMAC_DMA_FreeDesc(Gmac_Object * pGmac)
{
	/* Release the DMA TX/RX socket buffers */
	GMAC_DMA_FreeRxSkbufs(pGmac);
	GMAC_DMA_FreeTxSkbufs(pGmac);

	/* Free the region of consistent memory previously allocated for
	 * the DMA */
	dma_free_coherent(pGmac->dev,
			  pGmac->dma_tx_size * sizeof(Gmac_Tx_DMA_Descriptors),
			  pGmac->tx_dma_descriptors, pGmac->tx_bus_addr);
	dma_free_coherent(pGmac->dev,
			  pGmac->dma_rx_size * sizeof(Gmac_Tx_DMA_Descriptors),
			  pGmac->rx_dma_descriptors, pGmac->rx_bus_addr);
	kfree(pGmac->rx_skbuff_dma);
	kfree(pGmac->rx_skbuff);
	kfree(pGmac->tx_skbuff);
}

int GMAC_DMA_Init(struct net_device *ndev, __u32 dma_tx, __u32 dma_rx)
{
	Gmac_Object *pGmac = netdev_priv(ndev);
#ifdef GMAC_RESET
	int limit;

	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_BUS_MODE);
	reg |= 0x1;
	writel(reg, pGmac->remap_addr + REG_GMAC_BUS_MODE);

	limit = 10;
	while (limit--) {
		if (!(readl(pGmac->remap_addr + REG_GMAC_BUS_MODE) & 0x1)) {
			break;
		}
		mdelay(10);
	}
	if (limit < 0)
		return -EBUSY;
#endif
	//initialize dma bus mode reg0
	//8xpbl
	//no address_aligned_beats
	//no fixed_burst
	writel(0 << 25 | 0 << 24 | 1 << 16 | 32 << 8,
	       pGmac->remap_addr + REG_GMAC_BUS_MODE);
	writel(0x1a061, pGmac->remap_addr + REG_GMAC_INTR_EN);
	/* The base address of the RX/TX descriptor lists must be written into
	 * DMA CSR3 and CSR4, respectively. */
	writel(dma_rx, pGmac->remap_addr + REG_GMAC_RX_DESC_ADDR);
	writel(dma_tx, pGmac->remap_addr + REG_GMAC_TX_DESC_ADDR);

	return 0;
}

void GMAC_DMA_StartTx(Gmac_Object * pGmac)
{
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_OP_MODE);
	reg |= 1 << 13;
	writel(reg, pGmac->remap_addr + REG_GMAC_OP_MODE);
}

void GMAC_DMA_StopTx(Gmac_Object * pGmac)
{
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_OP_MODE);
	reg &= ~(1 << 13);
	writel(reg, pGmac->remap_addr + REG_GMAC_OP_MODE);
}

void GMAC_DMA_StartRx(Gmac_Object * pGmac)
{
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_OP_MODE);
	reg |= 1 << 1;
	writel(reg, pGmac->remap_addr + REG_GMAC_OP_MODE);
}

void GMAC_DMA_StopRx(Gmac_Object * pGmac)
{
	__u32 reg = readl(pGmac->remap_addr + REG_GMAC_OP_MODE);
	reg &= ~(1 << 1);
	writel(reg, pGmac->remap_addr + REG_GMAC_OP_MODE);
}

#ifdef FH_GMAC_DMA_DEBUG
static void GMAC_DMA_ShowTxState(__u32 status)
{
	__u32 state;
	state = (status & DMA_STATUS_TS_MASK) >> DMA_STATUS_TS_SHIFT;

	switch (state) {
	case 0:
		pr_info("- TX (Stopped): Reset or Stop command\n");
		break;
	case 1:
		pr_info("- TX (Running):Fetching the Tx desc\n");
		break;
	case 2:
		pr_info("- TX (Running): Waiting for end of tx\n");
		break;
	case 3:
		pr_info("- TX (Running): Reading the data "
			"and queuing the data into the Tx buf\n");
		break;
	case 6:
		pr_info("- TX (Suspended): Tx Buff Underflow "
			"or an unavailable Transmit descriptor\n");
		break;
	case 7:
		pr_info("- TX (Running): Closing Tx descriptor\n");
		break;
	default:
		break;
	}
}

static void GMAC_DMA_ShowRxState(__u32 status)
{
	__u32 state;
	state = (status & DMA_STATUS_RS_MASK) >> DMA_STATUS_RS_SHIFT;

	switch (state) {
	case 0:
		pr_info("- RX (Stopped): Reset or Stop command\n");
		break;
	case 1:
		pr_info("- RX (Running): Fetching the Rx desc\n");
		break;
	case 2:
		pr_info("- RX (Running):Checking for end of pkt\n");
		break;
	case 3:
		pr_info("- RX (Running): Waiting for Rx pkt\n");
		break;
	case 4:
		pr_info("- RX (Suspended): Unavailable Rx buf\n");
		break;
	case 5:
		pr_info("- RX (Running): Closing Rx descriptor\n");
		break;
	case 6:
		pr_info("- RX(Running): Flushing the current frame"
			" from the Rx buf\n");
		break;
	case 7:
		pr_info("- RX (Running): Queuing the Rx frame"
			" from the Rx buf into memory\n");
		break;
	default:
		break;
	}
}
#endif

int GMAC_DMA_Interrupt(Gmac_Object * pGmac)
{
	int ret = 0;
	Gmac_Stats *gmac_stats = &pGmac->stats;
	/* read the status register (CSR5) */
	__u32 intr_status;
	intr_status = readl(pGmac->remap_addr + REG_GMAC_STATUS);

	GMAC_DMA_DBG("%s: [GMAC_STATUS: 0x%08x]\n", __func__, intr_status);
#ifdef FH_GMAC_DMA_DEBUG
	/* It displays the DMA process states (CSR5 register) */
	GMAC_DMA_ShowTxState(intr_status);
	GMAC_DMA_ShowRxState(intr_status);
#endif
	/* ABNORMAL interrupts */
	if (unlikely(intr_status & DMA_STATUS_AIS)) {
		GMAC_DMA_DBG(KERN_INFO "CSR5[15] DMA ABNORMAL IRQ: ");
		if (unlikely(intr_status & DMA_STATUS_UNF)) {
			GMAC_DMA_DBG(KERN_INFO "transmit underflow\n");
			ret = tx_hard_error_bump_tc;
			gmac_stats->tx_undeflow_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_TJT)) {
			GMAC_DMA_DBG(KERN_INFO "transmit jabber\n");
			gmac_stats->tx_jabber_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_OVF)) {
			GMAC_DMA_DBG(KERN_INFO "recv overflow\n");
			gmac_stats->rx_overflow_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_RU)) {
			GMAC_DMA_DBG(KERN_INFO "receive buffer unavailable\n");
			gmac_stats->rx_buf_unav_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_RPS)) {
			GMAC_DMA_DBG(KERN_INFO "receive process stopped\n");
			gmac_stats->rx_process_stopped_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_RWT)) {
			GMAC_DMA_DBG(KERN_INFO "receive watchdog\n");
			gmac_stats->rx_watchdog_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_ETI)) {
			GMAC_DMA_DBG(KERN_INFO "transmit early interrupt\n");
			gmac_stats->tx_early_irq++;
		}
		if (unlikely(intr_status & DMA_STATUS_TPS)) {
			GMAC_DMA_DBG(KERN_INFO "transmit process stopped\n");
			gmac_stats->tx_process_stopped_irq++;
			ret = tx_hard_error;
		}
		if (unlikely(intr_status & DMA_STATUS_FBI)) {
			GMAC_DMA_DBG(KERN_INFO "fatal bus error\n");
			gmac_stats->fatal_bus_error_irq++;
			ret = tx_hard_error;
		}
	}
	/* TX/RX NORMAL interrupts */
	if (intr_status & DMA_STATUS_NIS) {
		gmac_stats->normal_irq_n++;
		if (likely((intr_status & DMA_STATUS_RI) ||
			   (intr_status & (DMA_STATUS_TI))))
			ret = handle_tx_rx;
	}
	/* Optional hardware blocks, interrupts should be disabled */
	if (unlikely(intr_status &
		     (DMA_STATUS_GPI | DMA_STATUS_GMI | DMA_STATUS_GLI)))
		pr_info("%s: unexpected status %08x\n", __func__, intr_status);

	/* Clear the interrupt by writing a logic 1 to the CSR5[15-0] */
	writel(intr_status & 0x1ffff, pGmac->remap_addr + REG_GMAC_STATUS);
	GMAC_DMA_DBG(KERN_INFO "\n\n");

	return ret;
}

int GMAC_DMA_TxStatus(Gmac_Object * pGmac, Gmac_Tx_DMA_Descriptors * desc)
{
	int ret = 0;
	struct net_device_stats *stats = &pGmac->ndev->stats;
	Gmac_Stats *gmac_stats = &pGmac->stats;

	if (unlikely(desc->desc0.bit.error_summary)) {
		if (unlikely(desc->desc0.bit.underflow_error)) {
			gmac_stats->tx_underflow++;
			stats->tx_fifo_errors++;
		}
		if (unlikely(desc->desc0.bit.no_carrier)) {
			gmac_stats->tx_carrier++;
			stats->tx_carrier_errors++;
		}
		if (unlikely(desc->desc0.bit.loss_of_carrier)) {
			gmac_stats->tx_losscarrier++;
			stats->tx_carrier_errors++;
		}
		if (unlikely((desc->desc0.bit.excessive_deferral) ||
			     (desc->desc0.bit.excessive_collision) ||
			     (desc->desc0.bit.late_collision)))
			stats->collisions += desc->desc0.bit.collision_count;
		ret = -1;
	}
	if (unlikely(desc->desc0.bit.deferred))
		gmac_stats->tx_deferred++;

	return ret;
}

int GMAC_DMA_RxStatus(Gmac_Object * pGmac, Gmac_Rx_DMA_Descriptors * desc)
{
	int ret = csum_none;
	struct net_device_stats *stats = &pGmac->ndev->stats;
	Gmac_Stats *gmac_stats = &pGmac->stats;

	if (unlikely(desc->desc0.bit.last_descriptor == 0)) {
		pr_warning("ndesc Error: Oversized Ethernet "
			   "frame spanned multiple buffers\n");
		stats->rx_length_errors++;
		return discard_frame;
	}

	if (unlikely(desc->desc0.bit.error_summary)) {
		if (unlikely(desc->desc0.bit.descriptor_error))
			gmac_stats->rx_desc++;
		if (unlikely(desc->desc0.bit.ipc_chksum_error_giant_frame))
			gmac_stats->rx_toolong++;
		if (unlikely(desc->desc0.bit.late_collision)) {
			gmac_stats->rx_collision++;
			stats->collisions++;
		}
		if (unlikely(desc->desc0.bit.crc_error)) {
			gmac_stats->rx_crc++;
			stats->rx_crc_errors++;
		}
		ret = discard_frame;
	}
	if (unlikely(desc->desc0.bit.dribble_error))
		ret = discard_frame;

	if (unlikely(desc->desc0.bit.length_error)) {
		gmac_stats->rx_length++;
		ret = discard_frame;
	}

	return ret;
}

void GMAC_DMA_DiagnosticFrame(void *data, Gmac_Object * pGmac)
{
	struct net_device_stats *stats = (struct net_device_stats *)data;
	Gmac_Stats *gmac_stats = &pGmac->stats;
	__u32 csr8 = readl(pGmac->remap_addr + REG_GMAC_ERROR_COUNT);

	if (unlikely(csr8)) {
		//Overflow bit for FIFO Overflow Counter
		if (csr8 & 0x10000000) {
			stats->rx_over_errors += 0x800;
			gmac_stats->rx_overflow_cntr += 0x800;
		} else {
			unsigned int ove_cntr;
			//indicates the number of frames missed by the application
			ove_cntr = ((csr8 & 0x0ffe0000) >> 17);
			stats->rx_over_errors += ove_cntr;
			gmac_stats->rx_overflow_cntr += ove_cntr;
		}

		//Overflow bit for Missed Frame Counter
		if (csr8 & 0x10000) {
			stats->rx_missed_errors += 0xffff;
			gmac_stats->rx_missed_cntr += 0xffff;
		} else {
			//indicates the number of frames missed by the controller
			unsigned int miss_f = (csr8 & 0xffff);
			stats->rx_missed_errors += miss_f;
			gmac_stats->rx_missed_cntr += miss_f;
		}
	}
}
