/*******************************************************************************
  This is the driver for the MAC 10/100 on-chip Ethernet controller
  currently tested on all the ST boards based on STb7109 and stx7200 SoCs.

  DWC Ether MAC 10/100 Universal version 4.0 has been used for developing
  this code.

  This only implements the mac core functions for this chip.

  Copyright (C) 2007-2009  STMicroelectronics Ltd

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
 *******************************************************************************/

#include <linux/crc32.h>
#include "dwmac100.h"

static void dwmac100_core_init(void __iomem *ioaddr)
{
	u32 value = readl(ioaddr + MAC_CONTROL);

	writel((value | MAC_CORE_INIT), ioaddr + MAC_CONTROL);

	/**********************************************
	 * Added By ZengChuanJie
	 * *******************************************/

	/* Flow Control */
	/* 1.Pause Time 0x200 */
	//value = MAC_FLOW_CTRL_PAUSETIME;
	/* 2.Enable Zero-Quanta Pause */
	/* 3. Pause Low Threshold */
//	value |= MAC_FLOW_CTRL_PLT28;
	/* 4. Unicase Pause Frame Detect */
//	value |= MAC_FLOW_CTRL_UP;
	/* 5. Enable Receive and Transmit Flow Control */
//	value |= (MAC_FLOW_CTRL_RFE|MAC_FLOW_CTRL_TFE);
//	writel(value, ioaddr+MAC_FLOW_CTRL);

	/* end */
}

static int dwmac100_rx_coe_supported(void __iomem *ioaddr)
{
	return 0;
}

static void dwmac100_dump_mac_regs(void __iomem *ioaddr)
{
	pr_info("\t----------------------------------------------\n"
			"\t  DWMAC 100 CSR (base addr = 0x%p)\n"
			"\t----------------------------------------------\n",
			ioaddr);
	/* ********************************************************
	 * Edited By ZengChuanJie
	 * *******************************************************/
	pr_info("\tcontrol reg (offset 0x%x): 0x%08x\n", MAC_CONTROL,
			readl(ioaddr + MAC_CONTROL));
	pr_info("\tframe filter (offset 0x%x): 0x%08x\n ", MAC_FRAME_FLT,
			readl(ioaddr + MAC_FRAME_FLT));
	pr_info("\thash high (offset 0x%x): 0x%08x\n", MAC_HASH_HIGH,
			readl(ioaddr + MAC_HASH_HIGH));
	pr_info("\thash low (offset 0x%x): 0x%08x\n",
			MAC_HASH_LOW, readl(ioaddr + MAC_HASH_LOW));
	pr_info("\tmii addr (offset 0x%x): 0x%08x\n",
			MAC_MII_ADDR, readl(ioaddr + MAC_MII_ADDR));
	pr_info("\tmii data (offset 0x%x): 0x%08x\n",
			MAC_MII_DATA, readl(ioaddr + MAC_MII_DATA));
	pr_info("\tflow control (offset 0x%x): 0x%08x\n", MAC_FLOW_CTRL,
			readl(ioaddr + MAC_FLOW_CTRL));
	pr_info("\tint status (offset 0x%x): 0x%08x\n", MAC_INT_STATUS,
			readl(ioaddr + MAC_INT_STATUS));

	/* end */

}

static void dwmac100_irq_status(void __iomem *ioaddr)
{
	return;
}

#if 0
void stmmac_set_mac_addr(void __iomem *ioaddr, u8 addr[6],
		unsigned int high, unsigned int low)
{
	unsigned long data;

	data = (addr[5] << 8) | addr[4];
	writel(data, ioaddr + high);
	data = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	writel(data, ioaddr + low);
}
#endif

#if 0
void stmmac_get_mac_addr(void __iomem *ioaddr, unsigned char *addr,
		unsigned int high, unsigned int low)
{
	unsigned int hi_addr, lo_addr;

	/* Read the MAC address from the hardware */
	hi_addr = readl(ioaddr + high);
	lo_addr = readl(ioaddr + low);

	/* Extract the MAC address from the high and low words */
	addr[0] = lo_addr & 0xff;
	addr[1] = (lo_addr >> 8) & 0xff;
	addr[2] = (lo_addr >> 16) & 0xff;
	addr[3] = (lo_addr >> 24) & 0xff;
	addr[4] = hi_addr & 0xff;
	addr[5] = (hi_addr >> 8) & 0xff;
}
#endif


static void dwmac100_set_umac_addr(void __iomem *ioaddr, unsigned char *addr,
		unsigned int reg_n)
{
	/* **********************************************************
	 * Edited by ZengChuanJie
	 * **********************************************************/
	stmmac_set_mac_addr(ioaddr, addr, MAC_ADDR0_HIGH, MAC_ADDR0_LOW);
	/* end */
}

static void dwmac100_get_umac_addr(void __iomem *ioaddr, unsigned char *addr,
		unsigned int reg_n)
{
	/* **********************************************************
	 * Edited by ZengChuanJie
	 * **********************************************************/
	stmmac_get_mac_addr(ioaddr, addr, MAC_ADDR0_HIGH, MAC_ADDR0_LOW);
	/* end */
}

static void dwmac100_set_filter(struct net_device *dev)
{
	void __iomem *ioaddr = (void __iomem *) dev->base_addr;
	u32 value = readl(ioaddr + MAC_FRAME_FLT);

	if (dev->flags & IFF_PROMISC) {
		/* 通过所有的帧，不需要过滤 */
		value |= MAC_CONTROL_PR;
		value &= ~(MAC_CONTROL_PM | MAC_CONTROL_IF | 
				MAC_CONTROL_HP);
	} else if ((netdev_mc_count(dev) > HASH_TABLE_SIZE)
			|| (dev->flags & IFF_ALLMULTI)) {
		/* 超过可用容纳的多播帧 */
		value |= MAC_CONTROL_PM;
		value &= ~(MAC_CONTROL_PR | MAC_CONTROL_IF);
		writel(0xffffffff, ioaddr + MAC_HASH_HIGH);
		writel(0xffffffff, ioaddr + MAC_HASH_LOW);
	} else if (netdev_mc_empty(dev)) {	/* no multicast */
		/* 没有多播帧，这个函数只会通过set_multilist_addr提供接口，所以不会进入这个分支 */
		value &= ~(MAC_CONTROL_PM | MAC_CONTROL_PR 
				| MAC_CONTROL_HP);
	} else {
		u32 mc_filter[2];
		struct netdev_hw_addr *ha;

		/* Perfect filter mode for physical address and Hash
		   filter for multicast */
		value |= MAC_CONTROL_HP ;
		value &= ~(MAC_CONTROL_PM | MAC_CONTROL_PR |
				MAC_CONTROL_IF);
		//value |= 0x10;
		value |= 0x4;

		//int i;
		//for (i=0; i<6; i++)
		//	printk(KERN_EMERG"%02x ", ha->addr[i]);
		//printk(KERN_EMERG"set_multilist_addr\n");

		memset(mc_filter, 0, sizeof(mc_filter));
		netdev_for_each_mc_addr(ha, dev) {
			/* The upper 6 bits of the calculated CRC are used to
			 * index the contens of the hash table */
		//	for (i=0; i<6; i++)
		//		printk(KERN_EMERG"%02x ", ha->addr[i]);
		//	printk("\n");
#if 1
			
			int bit_nr =
				ether_crc(ETH_ALEN, ha->addr) >> 26;

			//printk("bit_nr:%x\n", bit_nr);
			bit_nr = ~bit_nr;
			//printk("bit_nr:%x\n", bit_nr);
			bit_nr &= 0x3f;
#endif
#if 0
			
			/* crc32 */
			unsigned int bit_nr = crc32_le(~0, ha->addr, ETH_ALEN);

			printk("bit_nr:%x\n", bit_nr);
			bit_nr = bitrev32(bit_nr);
			printk("bit_nr:%x\n", bit_nr);
			/* get 31bit-26bit */
			bit_nr = bit_nr>>26;
			printk("bit_nr:%x\n", bit_nr);

			/* ~ */
			bit_nr = ~bit_nr;
			printk("bit_nr:%x\n", bit_nr);

			/* lowest 6 bits */
			bit_nr = bit_nr &0x3f;
			printk("bit_nr:%x\n", bit_nr);
#endif

			/* The most significant bit determines the register to
			 * use (H/L) while the other 5 bits determine the bit
			 * within the register. */
			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
			/* 计算出高低HASH 过滤寄存器的值 */
		}
		//mc_filter[1] = 0xffffffff; /* OK */
		//mc_filter[1] = 0xffff;
		//mc_filter[1] = 0xff000000;
		//mc_filter[1] = 0x00f00000; /* OK */
		//mc_filter[1] = 0x00c00000; /* OK */
		//mc_filter[1] = 0x00800000; /* OK */
		writel(mc_filter[0], ioaddr + MAC_HASH_LOW);
		writel(mc_filter[1], ioaddr + MAC_HASH_HIGH);
		//printk(KERN_EMERG"low:%x\n", mc_filter[0]);
		//printk(KERN_EMERG"high:%x\n", mc_filter[1]);
	}

	writel(value, ioaddr + MAC_FRAME_FLT);

	CHIP_DBG(KERN_INFO "%s: frame fileter reg: 0x%08x Hash regs: "
			"HI 0x%08x, LO 0x%08x\n",
			__func__, readl(ioaddr + MAC_FRAME_FLT),
			readl(ioaddr + MAC_HASH_HIGH), readl(ioaddr + MAC_HASH_LOW));
}

static void dwmac100_flow_ctrl(void __iomem *ioaddr, unsigned int duplex,
		unsigned int fc, unsigned int pause_time)
{
	unsigned int flow = MAC_FLOW_CTRL_ENABLE;

	if (duplex)
		flow |= (pause_time << MAC_FLOW_CTRL_PT_SHIFT);
	//printk("flow:%x\n", flow);
	writel(flow, ioaddr + MAC_FLOW_CTRL);
}

/* No PMT module supported for this Ethernet Controller.
 * Tested on ST platforms only.
 */
static void dwmac100_pmt(void __iomem *ioaddr, unsigned long mode)
{
	return;
}

static const struct stmmac_ops dwmac100_ops = {
	.core_init = dwmac100_core_init,
	.rx_coe = dwmac100_rx_coe_supported,
	.dump_regs = dwmac100_dump_mac_regs,
	.host_irq_status = dwmac100_irq_status,
	.set_filter = dwmac100_set_filter,
	.flow_ctrl = dwmac100_flow_ctrl,
	.pmt = dwmac100_pmt,
	.set_umac_addr = dwmac100_set_umac_addr,
	.get_umac_addr = dwmac100_get_umac_addr,
};

struct mac_device_info *dwmac100_setup(void __iomem *ioaddr)
{
	struct mac_device_info *mac;

	mac = kzalloc(sizeof(const struct mac_device_info), GFP_KERNEL);
	if (!mac)
		return NULL;

	//pr_info("\tDWMAC100\n");

	mac->mac = &dwmac100_ops;
	mac->dma = &dwmac100_dma_ops;

	mac->link.port = MAC_CONTROL_PS;
	mac->link.duplex = MAC_CONTROL_F;
	mac->link.speed = 0;
	mac->mii.addr = MAC_MII_ADDR;
	mac->mii.data = MAC_MII_DATA;

	return mac;
}
