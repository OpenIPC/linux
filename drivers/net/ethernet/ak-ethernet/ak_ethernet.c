/*
 * Anyka MAC Fast Ethernet driver for Linux.
 * Features
 * Copyright (C) 2010 ANYKA
 * AUTHOR Tang Anyang
 * AUTHOR Zhang Jingyuan
 * 10-11-01 09:08:08
 */

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/interrupt.h>
#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach-anyka/mac.h>
#include <mach/clock.h>
#include <plat-anyka/anyka_types.h>
#include <mach-anyka/partition_init.h> 
#include <mach-anyka/partition_lib.h> 
#include <linux/dma-direction.h> 
#include <asm-generic/scatterlist.h> 
#include <linux/dma-mapping.h> 
#include "ak_ethernet.h"
#include "eth_ops.h"
#include "Ethernethw.h"
#include "phyhw.h"
#include <mach/reset.h>

#include <linux/ethtool.h>  // cdh:add for static const struct ethtool_ops
#define MACNAME	"AK39E_MAC"
#define DRV_VERSION	"1.1"
#define TPD_RING_SIZE 0x50
#define RFD_RING_SIZE 0x50
#define RRD_RING_SIZE 0x50
#define MAC_FILE_NAME	"MAC"
#define CTOI(c) (isdigit(c) ? (c - '0') : (c - 'A' + 10))
#define MAC_ADDR_APSTRING_LEN 8 

#define IPC_OFFLOAD		1  // cdh:for hardware checksum 

#if 0
#define dbg(fmt, arg...) printk( "%s(%d): " fmt "\n", __func__, __LINE__, ##arg)
#define TR(fmt, arg...)  printk( "%s(%d): " fmt "\n", __func__, __LINE__, ##arg)
#else
#define dbg(fmt, arg...) {}
#define TR(fmt, arg...)  {}
#endif


// cdh:add
#define OTHER_END_PARTNER_AN_ABITY			(0x1<<0)  
#define SPEED_100M_FULL						(0x8)
#define SPEED_100M_HALF						(0x4)
#define SPEED_10M_FULL						(0x2)
#define NAN_SPEED_10M_HALF					((0x0<<8)|(0x1<<6))
#define NAN_SPEED_100M_HALF					((0x1<<8)|(0x0<<6))


/*GMAC IP Base address and Size   */
u8 *iomap_base = NULL;
u32 iomap_size = 0;

/*global adapter gmacdev pcidev and netdev pointers */
nt_adapter           *adapter_pt;
gmac_device          *gmacdev_pt;
struct pci_dev       *pcidev_pt;
struct net_device    *netdev_pt;
void *BoardbufVa = NULL; 	/* virtual address for ring buf */
dma_addr_t BoardbufPa; 		/* physical address for ring buf */
void *DatabufVa = NULL; 	/* virtual address for ring buf */
static struct timer_list timer_cable_unplug;
static u32 GMAC_Power_down; // This global variable is used to indicate the ISR whether the interrupts occured in the process of powering down the mac or not

/*
The synopGMAC_wakeup_filter_config3[] is a sample configuration for wake up filter.
Filter1 is used here
Filter1 offset is programmed to 50 (0x32)
Filter1 mask is set to 0x000000FF, indicating First 8 bytes are used by the filter
Filter1 CRC= 0x7EED this is the CRC computed on data 0x55 0x55 0x55 0x55 0x55 0x55 0x55 0x55
Refer accompanied software DWC_gmac_crc_example.c for CRC16 generation and how to use the same.
*/
u32 synopGMAC_wakeup_filter_config3[] =
{
    0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
    0x000000FF,	// For Filter1 CRC is computed on 0,1,2,3,4,5,6,7 bytes from offset
    0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
    0x00000000, // For Filter3 CRC is not computed may be it is 0x0000
    0x00000100, // Filter 0,2,3 are disabled, Filter 1 is enabled and filtering applies to only unicast packets
    0x00003200, // Filter 0,2,3 (no significance), filter 1 offset is 50 bytes from start of Destination MAC address
    0x7eED0000, // No significance of CRC for Filter0, Filter1 CRC is 0x7EED,
    0x00000000  // No significance of CRC for Filter2 and Filter3
};

unsigned char *pMacBase = NULL;
unsigned char *psysbase;
void *RingbufVa = NULL; /* virtual address for ring buf */
dma_addr_t RingbufPa; /* physical address for ring buf */

extern int eth_validate_addr(struct net_device *dev);
extern int ak_fha_init_for_update(int n);
static int save_ether_addr(unsigned char dev_addr[MAC_ADDR_LEN]);

static void mac_release_sharepin_phyclk(void)
{
	// cdh:set chip system ctrl reg base address
	psysbase = AK_VA_SYSCTRL; 
	if(psysbase == NULL)
	{
		printk("sysbase alloc error!");
	}else {
		printk("sysbase alloc OK!");
	}
	
	/** cdh:release
	*cdh:h2 bit [10:9]=01, set sharepin gpio47 share as opclk, old (3 << 2)
	*cdh:restore enable gpio47 pull down, old (1 << 2)
	*/
	REG32(psysbase + 0x74) &= ~(3 << 10); 
	REG32(psysbase + 0x80) &= ~(0x1 << 8);	// cdh:disable gpio47 pull down, old (1 << 2)

	/** cdh:release
	*cdh:restore first mac interface select mii, mac_speed_cfg=1(100m)
	*cdh:clear bit[15], prohibit 25m crystal
	*cdh:clean set	 bit[16],enable div24, generate 25m,	bit[18], select 25m clock of mac from pll div
	*cdh:close mac ctrl clk 	
	*/
	REG32(psysbase + 0x14) &= ~((0x1 << 22)|(0x1 << 23)); 
	REG32(psysbase + 0x14) &= ~(0x1 << 15); 
	REG32(psysbase + 0x14) &= ~((0x1 << 16)|(0x1 << 18)); 
	REG32(psysbase + 0x1c) |= (1 << 13);		 
}

static void mac_exit(struct net_device *ndev)
{
	nt_adapter *adapter;
    int iosize;
    adapter = (nt_adapter *)netdev_priv(ndev);

#if defined(CONFIG_ETHERNET_MAC_RMII)
	printk("Release MAC RMII interface!\n");
	ak_setpin_as_gpio(AK_GPIO_10);
	ak_setpin_as_gpio(AK_GPIO_11);
	ak_setpin_as_gpio(AK_GPIO_12);
	ak_setpin_as_gpio(AK_GPIO_13);
	ak_setpin_as_gpio(AK_GPIO_14);
	ak_setpin_as_gpio(AK_GPIO_15);
	ak_setpin_as_gpio(AK_GPIO_19);
	ak_setpin_as_gpio(AK_GPIO_20);
	ak_setpin_as_gpio(AK_GPIO_23);
	ak_setpin_as_gpio(AK_GPIO_24);
#elif defined(CONFIG_ETHERNET_MAC_MII)	
	printk("Release MAC MII interface!\n");
	ak_setpin_as_gpio(AK_GPIO_10);
	ak_setpin_as_gpio(AK_GPIO_11);
	ak_setpin_as_gpio(AK_GPIO_12);
	ak_setpin_as_gpio(AK_GPIO_13);
	ak_setpin_as_gpio(AK_GPIO_14);
	ak_setpin_as_gpio(AK_GPIO_15);
	ak_setpin_as_gpio(AK_GPIO_16);
	ak_setpin_as_gpio(AK_GPIO_17);
	ak_setpin_as_gpio(AK_GPIO_18);
	ak_setpin_as_gpio(AK_GPIO_19);
	ak_setpin_as_gpio(AK_GPIO_20);
	ak_setpin_as_gpio(AK_GPIO_21);
	ak_setpin_as_gpio(AK_GPIO_22);
	ak_setpin_as_gpio(AK_GPIO_23);
	ak_setpin_as_gpio(AK_GPIO_24);
	ak_setpin_as_gpio(AK_GPIO_76);
	ak_setpin_as_gpio(AK_GPIO_77);
	ak_setpin_as_gpio(AK_GPIO_78);
#else
	printk("No Release MAC interface!\n");
#endif

	mac_release_sharepin_phyclk();
	
	// cdh:release platform device resource
	if (adapter->db_pt->addr_req) {
		iosize = resource_size(adapter->db_pt->addr_res);
		release_mem_region(adapter->db_pt->addr_res->start, iosize);
	}

    if (RingbufVa) {
		dma_free_coherent(&ndev->dev, sizeof (gmac_device), RingbufVa, RingbufPa);
		RingbufVa = NULL;
		RingbufPa = 0;
	}

	if (BoardbufVa)  {
		dma_free_coherent(&ndev->dev, sizeof (mac_info_t), BoardbufVa, BoardbufPa);
		BoardbufVa = NULL;
		BoardbufPa = 0;
	}

}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 *Used by netconsole
 */
static void ak_mac_poll_controller(struct net_device *ndev)
{

}


#endif



/**
  * This is a wrapper function for platform dependent delay
  * Take care while passing the argument to this function
  * note:cdh:check ok
  */
void plat_delay(u32 delay)
{
    while (delay--);
    return;
}

/**
  * This is a wrapper function for consistent dma-able Memory allocation routine.
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  * note:cdh:check ok
  */
void *plat_alloc_consistent_mem(struct net_device *pmnetdev, u32 size, dma_addr_t *mac_dma_handle)
{
	void *pt = dma_alloc_coherent(NULL, size, mac_dma_handle, GFP_KERNEL); 
    return pt;
}

/**
  * This is a wrapper function for freeing consistent dma-able Memory.
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  * note:cdh:check ok
  */
void plat_free_consistent_mem(struct net_device *pmnetdev, u32 size, void *addr, dma_addr_t mac_dma_handle)
{
    dma_free_coherent(NULL, size, addr, mac_dma_handle); 
    return;
}



/**
 * The Low level function to read register contents from Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * \return  Returns the register contents
 * note:cdh:check ok
 */
static u32 gmac_read_reg(u32 *RegBase, u32 RegOffset)
{

    u32 addr = (u32)RegBase + RegOffset;
    u32 data = readl((void *)addr);
    //TR("%s RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, (u32)RegBase, RegOffset, data );
    return data;

}


/**
 * The Low level function to write to a register in Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * @param[in] Data to be written
 * \return  void
 * note:cdh:check ok
 */
static void  gmac_write_reg(u32 *RegBase, u32 RegOffset, u32 RegData)
{

    u32 addr = (u32)RegBase + RegOffset;
    //  TR("%s RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__,(u32) RegBase, RegOffset, RegData );
    writel(RegData, (void *)addr);
    return;
}



/**
 * The Low level function to clear bits of a register in Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * @param[in] Bit mask to clear bits to logical 0
 * \return  void
 */
static void gmac_clr_bits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
    u32 addr = (u32)RegBase + RegOffset;
    u32 data = readl((void *)addr);
    data &= (~BitPos);
    //  TR("%s !!!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
    writel(data, (void *)addr);
    //  TR("%s !!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
    return;
}

/**
 * The Low level function to set bits of a register in Hardware.
 *
 * @param[in] pointer to the base of register map
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1
 * \return  void
 */
static void gmac_set_bits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
    u32 addr = (u32)RegBase + RegOffset;
    u32 data = readl((void *)addr);
    data |= BitPos;
    //  TR("%s !!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
    writel(data, (void *)addr);
    //  TR("%s !!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
    return;
}

/**
  * Function to read the Phy register. The access to phy register
  * is a slow process as the data is moved accross MDI/MDO interface
  * @param[in] pointer to Register Base (It is the mac base in our case) .
  * @param[in] PhyBase register is the index of one of supported 32 PHY devices.
  * @param[in] Register offset is the index of one of the 32 phy register.
  * @param[out] u16 data read from the respective phy register (only valid iff return value is 0).
  * \return Returns 0 on success else return the error status.
  * note:cdh:check ok
  */
static s32 gmac_read_phy_reg(u32 *RegBase, u32 PhyBase, u32 RegOffset, u16 *data)
{
    u32 addr;
    u32 loop_variable;
    
    addr = ((PhyBase << GmiiDevShift) & GmiiDevMask) | ((RegOffset << GmiiRegShift) & GmiiRegMask);
    addr = addr | GmiiCsrClk1 | GmiiBusy ; //Gmii busy bit
    gmac_write_reg(RegBase, GmacGmiiAddr, addr); //write the address from where the data to be read in GmiiGmiiAddr register of synopGMAC ip

    for(loop_variable = 0; loop_variable < DEFAULT_LOOP_VARIABLE; loop_variable++)  //Wait till the busy bit gets cleared with in a certain amount of time
    {
        if (!(gmac_read_reg(RegBase, GmacGmiiAddr) & GmiiBusy))
        {
	        //TR("mdio read:%d\n", RegOffset);
            break;
        } else {
	        ;//CDH:TR("mdio busy\n");
        }
        plat_delay(DEFAULT_DELAY_VARIABLE);
    }
    
    if(loop_variable < DEFAULT_LOOP_VARIABLE){
        *data = (u16)(gmac_read_reg(RegBase, GmacGmiiData) & 0xFFFF);
       // printk("OK:read phy reg:%d, data:0x%x\n", RegOffset, *data);
    }else{
       // TR("Error::: PHY not responding Busy bit didnot get cleared !!!!!!\n");
        return -ESYNOPGMACPHYERR;
    }
    
    return -ESYNOPGMACNOERR;
}

/**
  * Function to write to the Phy register. The access to phy register
  * is a slow process as the data is moved accross MDI/MDO interface
  * @param[in] pointer to Register Base (It is the mac base in our case) .
  * @param[in] PhyBase register is the index of one of supported 32 PHY devices.
  * @param[in] Register offset is the index of one of the 32 phy register.
  * @param[in] data to be written to the respective phy register.
  * \return Returns 0 on success else return the error status.
  * note:cdh:check ok
  */
static s32 gmac_write_phy_reg(u32 *RegBase, u32 PhyBase, u32 RegOffset, u16 data)
{
    u32 addr;
    u32 loop_variable;

    gmac_write_reg(RegBase, GmacGmiiData, data);

    addr = ((PhyBase << GmiiDevShift) & GmiiDevMask) | ((RegOffset << GmiiRegShift) & GmiiRegMask) | GmiiWrite;

    addr = addr | GmiiCsrClk1 | GmiiBusy ; //set Gmii clk to 20-35 Mhz and Gmii busy bit

    gmac_write_reg(RegBase, GmacGmiiAddr, addr);
    for(loop_variable = 0; loop_variable < DEFAULT_LOOP_VARIABLE; loop_variable++)
    {
        if (!(gmac_read_reg(RegBase, GmacGmiiAddr) & GmiiBusy))
        {
	        // TR("mdio write: %x,%x\n", RegOffset, data);
            break;
        } else {
	       ; // TR("mdio busy\n");
        }
        plat_delay(DEFAULT_DELAY_VARIABLE);
    }

    if(loop_variable < DEFAULT_LOOP_VARIABLE)
    {
        return -ESYNOPGMACNOERR;
    }
    else
    {
        //TR("Error::: PHY not responding Busy bit didnot get cleared !!!!!!\n");
        return -ESYNOPGMACPHYERR;
    }
}


// cdh:Mac Management Counters (MMC), cdh:check ok
void gmac_mmc_counters_reset(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacMmcCntrl, GmacMmcCounterReset);
    return;
}

/**
  * Configures the MMC to stop rollover.
  * Programs MMC interface so that counters will not rollover after reaching maximum value.
  * @param[in] pointer to gmac_device.
  * return returns void.
  * note:cdh:check ok
  */
void gmac_mmc_counters_disable_rollover(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacMmcCntrl, GmacMmcCounterStopRollover);
    return;
}

/**
  * Read the MMC Rx interrupt status.
  * @param[in] pointer to gmac_device.
  * \return returns the Rx interrupt status.
  */
u32 gmac_read_mmc_rx_int_status(gmac_device *gmacdev)
{
    return (gmac_read_reg((u32 *)gmacdev->MacBase, GmacMmcIntrRx));
}

/**
  * Read the MMC Tx interrupt status.
  * @param[in] pointer to gmac_device.
  * \return returns the Tx interrupt status.
  */
u32 gmac_read_mmc_tx_int_status(gmac_device *gmacdev)
{
    return (gmac_read_reg((u32 *)gmacdev->MacBase, GmacMmcIntrTx));
}


/**
  * Function to reset the GMAC core.
  * This reests the DMA and GMAC core. After reset all the registers holds their respective reset value
  * @param[in] pointer to gmac_device.
  * \return 0 on success else return the error status.
  * note:cdh:check ok
  */
s32 gmac_reset(gmac_device *gmacdev)
{
    u32 data = 0;
	s32 reset_cnt = 0xFFFF;
	
    // cdh:software reset , the resets all of the GMAC internal registers and logic
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaBusMode , DmaResetOn); // cdh:reset dma engine
    plat_delay(DEFAULT_LOOP_VARIABLE);
	while(reset_cnt>0){
		data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaBusMode);
		// cdh:after finish become 0
		if((data & DmaResetOn) != DmaResetOn) {
			break;
		}
		reset_cnt--;
	}

	if (reset_cnt <= 0) {
		printk("No find phy small board!\n");
		return -1;
	}

    return 0;
}

/**
  * Sets the Mac address in to GMAC register.
  * This function sets the MAC address to the MAC register in question.
  * @param[in] pointer to gmac_device to populate mac dma and phy addresses.
  * @param[in] Register offset for Mac address high
  * @param[in] Register offset for Mac address low
  * @param[in] buffer containing mac address to be programmed.
  * \return 0 upon success. Error code upon failure.
  * \note:cdh:check ok
  */
static s32 gmac_set_mac_addr(gmac_device *gmacdev, u32 MacHigh, u32 MacLow, u8 *MacAddr)
{
    u32 data;

    data = (MacAddr[5] << 8) | MacAddr[4];
    gmac_write_reg((u32 *)gmacdev->MacBase, MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0] ;
    gmac_write_reg((u32 *)gmacdev->MacBase, MacLow, data);
    return 0;
}

/**
  * Get the Mac address in to the address specified.
  * The mac register contents are read and written to buffer passed.
  * @param[in] pointer to gmac_device to populate mac dma and phy addresses.
  * @param[in] Register offset for Mac address high
  * @param[in] Register offset for Mac address low
  * @param[out] buffer containing the device mac address.
  * \return 0 upon success. Error code upon failure.
  * \note:cdh:check ok
  */
static s32 gmac_get_mac_addr(gmac_device *gmacdev, u32 MacHigh, u32 MacLow, u8 *MacAddr)
{
    u32 data;

    data = gmac_read_reg((u32 *)gmacdev->MacBase, MacHigh);
    MacAddr[5] = (data >> 8) & 0xff;
    MacAddr[4] = (data)        & 0xff;

    data = gmac_read_reg((u32 *)gmacdev->MacBase, MacLow);
    MacAddr[3] = (data >> 24) & 0xff;
    MacAddr[2] = (data >> 16) & 0xff;
    MacAddr[1] = (data >> 8 ) & 0xff;
    MacAddr[0] = (data )      & 0xff;

    return 0;
}


/**
  * Attaches the synopGMAC device structure to the hardware.
  * Device structure is populated with MAC/DMA and PHY base addresses.
  * @param[in] pointer to gmac_device to populate mac dma and phy addresses.
  * @param[in] GMAC IP mac base address.
  * @param[in] GMAC IP dma base address.
  * @param[in] GMAC IP phy base address.
  * \return 0 upon success. Error code upon failure.
  * \note This is important function. No kernel api provided by Synopsys
  * note:cdh:check ok
  */
static s32 gmac_attach(gmac_device *gmacdev, u32 macBase, u32 dmaBase, u32 phyBase)
{
#if 0
    u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
    u8 testmac_addr0[6] = {0};
    u8 i = 0;
#endif

	TR(" into %s, %d, %s \n", __FILE__, __LINE__, __FUNCTION__);
	TR("1");
	
    /*Make sure the Device data strucure is cleared before we proceed further*/
    memset((void *) gmacdev, 0, sizeof(gmac_device));
    /*Populate the mac and dma base addresses*/
    gmacdev->MacBase = macBase;
    gmacdev->DmaBase = dmaBase;
    gmacdev->PhyBase = phyBase;
    dbg("2");

#if 0
    /*  cdh:Program/flash in the station/IP's Mac address */
    gmac_set_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, mac_addr0);

    /* cdh:Lets set ipaddress in to device structure, cdh:check ok*/
    gmac_get_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, testmac_addr0);
    for (i=0; i<6; i++) {
		TR("cdh:mac_addr:%d:0x%x\n", i, testmac_addr0[i]);
    }
#endif

	TR(" out %s, %d, %s \n", __FILE__, __LINE__, __FUNCTION__);
	
    return 0;
}

/**
  * Function to read the GMAC IP Version and populates the same in device data structure.
  * @param[in] pointer to gmac_device.
  * \return Always return 0.
  * \note:cdh:check ok
  */

s32 gmac_read_version(gmac_device *gmacdev)
{
    u32 data = 0;

    printk("gmac version register %x\n", (gmacdev->MacBase + GmacVersion));
    // cdh:available the mac ip version
    data = gmac_read_reg((u32 *)gmacdev->MacBase, GmacVersion );
    gmacdev->Version = data;
    printk("The data read from %08x is %08x\n", (gmacdev->MacBase + GmacVersion), data);
    return 0;
}



/**
  * Enable all the interrupts.
  * Enables the DMA interrupt as specified by the bit mask.
  * @param[in] pointer to gmac_device.
  * @param[in] bit mask of interrupts to be enabled.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_enable_interrupt(gmac_device *gmacdev, u32 interrupts)
{
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaInterrupt, interrupts);
    return;
}


/**
  * Disable all the interrupts.
  * Disables all DMA interrupts.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note This function disabled all the interrupts, if you want to disable a particular interrupt then
  *  use gmac_disable_interrupt().
  * note:cdh:check ok
  */
void gmac_disable_interrupt_all(gmac_device *gmacdev)
{
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaInterrupt, DmaIntDisable);
    return;
}


/**
  * Checks whether the packet received is a magic packet?.
  * @param[in] pointer to gmac_device.
  * \return returns True if magic packet received else returns false.
  */
bool gmac_is_magic_packet_received(gmac_device *gmacdev)
{
    u32 data;
    data = 	gmac_read_reg((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus);
    return((data & GmacPmtMagicPktReceived) == GmacPmtMagicPktReceived);
}

/**
  * Checks whether the packet received is a wakeup frame?.
  * @param[in] pointer to gmac_device.
  * \return returns true if wakeup frame received else returns false.
  */
bool gmac_is_wakeup_frame_received(gmac_device *gmacdev)
{
    u32 data;
    data = 	gmac_read_reg((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus);
    return((data & GmacPmtWakeupFrameReceived) == GmacPmtWakeupFrameReceived);
}

/**
  * Enables the assertion of PMT interrupt.
  * This enables the assertion of PMT interrupt due to Magic Pkt or Wakeup frame
  * reception.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_pmt_int_enable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacInterruptMask, GmacPmtIntMask);
    return;
}
/**
  * Disables the assertion of PMT interrupt.
  * This disables the assertion of PMT interrupt due to Magic Pkt or Wakeup frame
  * reception.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_pmt_int_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacInterruptMask, GmacPmtIntMask);
    return;
}

/**
  * Enable the reception of frames on GMII/MII.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_rx_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRx);
    return;
}
/**
  * Disable the reception of frames on GMII/MII.
  * GMAC receive state machine is disabled after completion of reception of current frame.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_rx_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRx);
    return;
}

/**
  * Enable the DMA Reception.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_enable_dma_rx(gmac_device *gmacdev)
{
    //	gmac_set_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaRxStart);
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
    data |= DmaRxStart;
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl , data);

}

/**
  * Enable the DMA Transmission.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_enable_dma_tx(gmac_device *gmacdev)
{
    //	gmac_set_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaTxStart);
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
    data |= DmaTxStart;
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl , data);

}

/**
  * Enable the transmission of frames on GMII/MII.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_tx_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacTx);
    return;
}
/**
  * Disable the transmission of frames on GMII/MII.
  * GMAC transmit state machine is disabled after completion of transmission of current frame.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_tx_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacTx);
    return;
}





static void powerup_mac(gmac_device *gmacdev)
{
    GMAC_Power_down = 0;	// Let ISR know that MAC is out of power down now
    if(gmac_is_magic_packet_received(gmacdev))
        TR("GMAC wokeup due to Magic Pkt Received\n");
    if(gmac_is_wakeup_frame_received(gmacdev))
        TR("GMAC wokeup due to Wakeup Frame Received\n");
    //Disable the assertion of PMT interrupt
    gmac_pmt_int_disable(gmacdev);
    //Enable the mac and Dma rx and tx paths
    gmac_rx_enable(gmacdev);
    gmac_enable_dma_rx(gmacdev);

    gmac_tx_enable(gmacdev);
    gmac_enable_dma_tx(gmacdev);
    return;
}


/**
  * Returns the all unmasked interrupt status after reading the DmaStatus register.
  * @param[in] pointer to gmac_device.
  * \return 0 upon success. Error code upon failure.
  */
u32 gmac_get_interrupt_type(gmac_device *gmacdev)
{
    u32 data;
    u32 interrupts = 0;
    data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaStatus , data); //This is the appropriate location to clear the interrupts
    TR("DMA status reg is  %08x\n", data);
    if(data & DmaIntErrorMask)	interrupts     |= synopGMACDmaError;
    if(data & DmaIntRxNormMask)	interrupts     |= synopGMACDmaRxNormal;
    if(data & DmaIntRxAbnMask)	interrupts     |= synopGMACDmaRxAbnormal;
    if(data & DmaIntRxStoppedMask)	interrupts |= synopGMACDmaRxStopped;
    if(data & DmaIntTxNormMask)	interrupts     |= synopGMACDmaTxNormal;
    if(data & DmaIntTxAbnMask)	interrupts     |= synopGMACDmaTxAbnormal;
    if(data & DmaIntTxStoppedMask)	interrupts |= synopGMACDmaTxStopped;

    return interrupts;
}


#ifdef IPC_OFFLOAD
/**
  * Enables the ip checksum offloading in receive path.
  * When set GMAC calculates 16 bit 1's complement of all received ethernet frame payload.
  * It also checks IPv4 Header checksum is correct. GMAC core appends the 16 bit checksum calculated
  * for payload of IP datagram and appends it to Ethernet frame transferred to the application.
  * @param[in] pointer to synopGMACdevice.
  * \return returns void.
  */
static void synopGMAC_enable_rx_chksum_offload(gmac_device *gmacdev)
{
	gmac_set_bits((u32 *)gmacdev->MacBase,GmacConfig,GmacRxIpcOffload);
	return;
}

/**
  * Instruct the DMA to drop the packets fails tcp ip checksum.
  * This is to instruct the receive DMA engine to drop the recevied packet if they 
  * fails the tcp/ip checksum in hardware. Valid only when full checksum offloading is enabled(type-2).
  * @param[in] pointer to synopGMACdevice.
  * \return returns void.
  */
static void synopGMAC_rx_tcpip_chksum_drop_enable(gmac_device *gmacdev)
{
	gmac_clr_bits((u32 *)gmacdev->DmaBase,DmaControl,DmaDisableDropTcpCs);
	return;
}

#endif

#ifdef IPC_OFFLOAD
/**
  * Checks if any Ipv4 header checksum error in the frame just transmitted.
  * This serves as indication that error occureed in the IPv4 header checksum insertion.
  * The sent out frame doesnot carry any ipv4 header checksum inserted by the hardware.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns true if error in ipv4 header checksum, else returns false.
  */
static bool synopGMAC_is_tx_ipv4header_checksum_error(gmac_device *gmacdev, u32 status)
{
	return((status & DescTxIpv4ChkError) == DescTxIpv4ChkError);
}

/**
  * Checks if any payload checksum error in the frame just transmitted.
  * This serves as indication that error occureed in the payload checksum insertion.
  * The sent out frame doesnot carry any payload checksum inserted by the hardware.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns true if error in ipv4 header checksum, else returns false.
  */
static bool synopGMAC_is_tx_payload_checksum_error(gmac_device *gmacdev, u32 status)
{
	return((status & DescTxPayChkError) == DescTxPayChkError);
}
#endif


#ifdef IPC_OFFLOAD

/**
  * Decodes the Rx Descriptor status to various checksum error conditions.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns decoded enum (u32) indicating the status.
  */
u32 synopGMAC_is_rx_checksum_error(gmac_device *gmacdev, u32 status)
{
	if     (((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) == 0))
	return RxLenLT600;  ////
	else if(((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) != 0))
	return RxIpHdrPayLoadChkBypass;////
	else if(((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) != 0))
	return RxChkBypass; ////
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) == 0))
	return RxNoChkError;//4//
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) != 0))
	return RxPayLoadChkError;////
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) == 0))
	return RxIpHdrChkError; //6
	else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) != 0))
	return RxIpHdrPayLoadChkError;
	else
	return RxIpHdrPayLoadRes;
}
#endif


/**
  * Disable the DMA for Transmission.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */

void gmac_disable_dma_tx(gmac_device *gmacdev)
{
    //	gmac_clr_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaTxStart);
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
    data &= (~DmaTxStart);
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl , data);
}

/**
  * Disable the DMA for Reception.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
void gmac_disable_dma_rx(gmac_device *gmacdev)
{
    //	gmac_clr_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaRxStart);
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
    data &= (~DmaRxStart);
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl , data);
}


/**
  * Checks whether this rx descriptor is in chain mode.
  * This returns true if it is this descriptor is in chain mode.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if chain mode is set, false if not.
  * note:cdh:check ok
  */
bool gmac_is_rx_desc_chained(DmaDesc *desc)
{
    return((desc->length & RxDescChain) == RxDescChain);
}

/**
  * Checks whether this tx descriptor is in chain mode.
  * This returns true if it is this descriptor is in chain mode.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if chain mode is set, false if not.
  */
bool gmac_is_tx_desc_chained(DmaDesc *desc)
{
#ifdef ENH_DESC
    return((desc->status & TxDescChain) == TxDescChain);
#else
    return((desc->length & TxDescChain) == TxDescChain);
#endif
}

/**
  * Take ownership of this Descriptor.
  * The function is same for both the ring mode and the chain mode DMA structures.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_take_desc_ownership(DmaDesc *desc)
{
    if(desc)
    {
        desc->status &= ~DescOwnByDma;  //Clear the DMA own bit
        //desc->status |= DescError;	// Set the error to indicate this descriptor is bad
    }
}



/**
  * Take ownership of all the rx Descriptors.
  * This function is called when there is fatal error in DMA transmission.
  * When called it takes the ownership of all the rx descriptor in rx descriptor pool/queue from DMA.
  * The function is same for both the ring mode and the chain mode DMA structures.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note Make sure to disable the transmission before calling this function, otherwise may result in racing situation.
  * note:cdh:check ok
  */
void gmac_take_desc_ownership_rx(gmac_device *gmacdev)
{
    s32 i;
    DmaDesc *desc;
    desc = gmacdev->RxDesc;
    for(i = 0; i < gmacdev->RxDescCount; i++)
    {
        if(gmac_is_rx_desc_chained(desc)) 	//This descriptor is in chain mode
        {
            gmac_take_desc_ownership(desc);
            desc = (DmaDesc *)desc->data2;
        }
        else
        {
            gmac_take_desc_ownership(desc + i);
        }
    }
}

/**
  * Take ownership of all the rx Descriptors.
  * This function is called when there is fatal error in DMA transmission.
  * When called it takes the ownership of all the tx descriptor in tx descriptor pool/queue from DMA.
  * The function is same for both the ring mode and the chain mode DMA structures.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note Make sure to disable the transmission before calling this function, otherwise may result in racing situation.
  * note:cdh:check ok
  */
void gmac_take_desc_ownership_tx(gmac_device *gmacdev)
{
    s32 i;
    DmaDesc *desc;
    desc = gmacdev->TxDesc;
    for(i = 0; i < gmacdev->TxDescCount; i++)
    {
        if(gmac_is_tx_desc_chained(desc)) 	//This descriptor is in chain mode
        {
            gmac_take_desc_ownership(desc);
            desc = (DmaDesc *)desc->data2;
        }
        else
        {
            gmac_take_desc_ownership(desc + i);
        }
    }

}

/**
  * Initialize the rx descriptors for ring or chain mode operation.
  * 	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0 for ring mode of operation. (note)
  *	- data1 and data2 set to 0. (note)
  * @param[in] pointer to DmaDesc structure.
  * @param[in] whether end of ring
  * \return void.
  * \note Initialization of the buffer1, buffer2, data1,data2 and status are not done here. This only initializes whether one wants to use this descriptor
  * in chain mode or ring mode. For chain mode of operation the buffer2 and data2 are programmed before calling this function.
  */
void gmac_rx_desc_init_ring(DmaDesc *desc, bool last_ring_desc)
{
    desc->status = 0;
    desc->length = last_ring_desc ? RxDescEndOfRing : 0;
    desc->buffer1 = 0;
    desc->buffer2 = 0;
    desc->data1 = 0;
    desc->data2 = 0;
    return;
}
/**
  * Initialize the tx descriptors for ring or chain mode operation.
  * 	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0 for ring mode of operation. (note)
  *	- data1 and data2 set to 0. (note)
  * @param[in] pointer to DmaDesc structure.
  * @param[in] whether end of ring
  * \return void.
  * \note Initialization of the buffer1, buffer2, data1,data2 and status are not done here. This only initializes whether one wants to use this descriptor
  * in chain mode or ring mode. For chain mode of operation the buffer2 and data2 are programmed before calling this function.
  * note:cdh:check ok
  */
void gmac_tx_desc_init_ring(DmaDesc *desc, bool last_ring_desc)
{
#ifdef ENH_DESC
    desc->status = last_ring_desc ? TxDescEndOfRing : 0;
    desc->length = 0;
#else
	desc->status = 0; // added by panqihe 2014-10-30
    desc->length = last_ring_desc ? TxDescEndOfRing : 0; // cdh:TxDescEndOfRing bit25(TER)
#endif
    desc->buffer1 = 0;
    desc->buffer2 = 0;
    desc->data1 = 0;
    desc->data2 = 0;
    return;
}

s32 gmac_init_tx_rx_desc_queue(gmac_device *gmacdev)
{
    s32 i;
    for(i = 0; i < gmacdev->TxDescCount; i++)
    {
        gmac_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount - 1);
    }
    TR("At line %d\n", __LINE__);
    for(i = 0; i < gmacdev->RxDescCount; i++)
    {
        gmac_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount - 1);
    }

    gmacdev->TxNext = 0;
    gmacdev->TxBusy = 0;
    gmacdev->RxNext = 0;
    gmacdev->RxBusy = 0;

    return -ESYNOPGMACNOERR;
}

/**
  * Function to program DMA bus mode register.
  *
  * The Bus Mode register is programmed with the value given. The bits to be set are
  * bit wise or'ed and sent as the second argument to this function.
  * @param[in] pointer to gmac_device.
  * @param[in] the data to be programmed.
  * \return 0 on success else return the error status.
  * note:cdh:check ok
  */
static s32 gmac_set_dma_bus_mode(gmac_device *gmacdev, u32 init_value )
{
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaBusMode , init_value);
    return 0;

}

/**
  * Function to program DMA Control register.
  *
  * The Dma Control register is programmed with the value given. The bits to be set are
  * bit wise or'ed and sent as the second argument to this function.
  * @param[in] pointer to gmac_device.
  * @param[in] the data to be programmed.
  * \return 0 on success else return the error status.
  * note:cdh:check ok
  */
static s32 gmac_set_dma_control(gmac_device *gmacdev, u32 init_value)
{
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, init_value);
    return 0;
}

/**
  * Programs the DmaRxBaseAddress with the Rx descriptor base address.
  * Rx Descriptor's base address is available in the gmacdev structure. This function progrms the
  * Dma Rx Base address with the starting address of the descriptor ring or chain.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_init_rx_desc_base(gmac_device *gmacdev)
{
	u32 rx_desc_addr;

	// cdh:write RxDescDma consistent  physic address to mac dma ctrl reg
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaRxBaseAddr, (u32)gmacdev->RxDescDma);
    printk("rx descriptor addr[w]: %x\n", gmacdev->RxDescDma);
    rx_desc_addr = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaRxBaseAddr);
    printk("rx descriptor addr[r]: %x\n", rx_desc_addr);
    return;
}

/**
  * Programs the DmaTxBaseAddress with the Tx descriptor base address.
  * Tx Descriptor's base address is available in the gmacdev structure. This function progrms the
  * Dma Tx Base address with the starting address of the descriptor ring or chain.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_init_tx_desc_base(gmac_device *gmacdev)
{
	// cdh:write TxDescDma consistent  physic address to mac dma ctrl reg
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaTxBaseAddr, (u32)gmacdev->TxDescDma);
    return;
}

/*Gmac configuration functions*/

/**
  * Enable the watchdog timer on the receiver.
  * When enabled, Gmac enables Watchdog timer, and GMAC allows no more than
  * 2048 bytes of data (10,240 if Jumbo frame enabled).
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_wd_enable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacWatchdog);
    return;
}
/**
  * Disable the watchdog timer on the receiver.
  * When disabled, Gmac disabled watchdog timer, and can receive frames up to
  * 16,384 bytes.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_wd_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacWatchdog);
    return;
}

/**
  * Enables the Jabber frame support.
  * When enabled, GMAC disabled the jabber timer, and can transfer 16,384 byte frames.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_jab_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacJabber);
    return;
}
/**
  * Disables the Jabber frame support.
  * When disabled, GMAC enables jabber timer. It cuts of transmitter if application
  * sends more than 2048 bytes of data (10240 if Jumbo frame enabled).
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_jab_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacJabber);
    return;
}

/**
  * Enables Frame bursting (Only in Half Duplex Mode).
  * When enabled, GMAC allows frame bursting in GMII Half Duplex mode.
  * Reserved in 10/100 and Full-Duplex configurations.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_frame_burst_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacFrameBurst);
    return;
}
/**
  * Disables Frame bursting.
  * When Disabled, frame bursting is not supported.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_frame_burst_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacFrameBurst);
    return;
}

/**
  * Enable Jumbo frame support.
  * When Enabled GMAC supports jumbo frames of 9018/9022(VLAN tagged).
  * Giant frame error is not reported in receive frame status.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_jumbo_frame_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacJumboFrame);
    return;
}
/**
  * Disable Jumbo frame support.
  * When Disabled GMAC does not supports jumbo frames.
  * Giant frame error is reported in receive frame status.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_jumbo_frame_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacJumboFrame);
    return;
}

/**
  * Enables Receive Own bit (Only in Half Duplex Mode).
  * When enaled GMAC receives all the packets given by phy while transmitting.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_rx_own_enable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRxOwn);
    return;
}
/**
  * Disables Receive Own bit (Only in Half Duplex Mode).
  * When enaled GMAC disables the reception of frames when gmii_txen_o is asserted.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_rx_own_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRxOwn);
    return;
}

/**
  * Sets the GMAC in loopback mode.
  * When on GMAC operates in loop-back mode at GMII/MII.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note (G)MII Receive clock is required for loopback to work properly, as transmit clock is
  * not looped back internally.
  */
void gmac_loopback_on(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacLoopback);
    return;
}
/**
  * Sets the GMAC in Normal mode.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_loopback_off(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacLoopback);
    return;
}

/**
  * Sets the GMAC core in Full-Duplex mode.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_set_full_duplex(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDuplex);
    return;
}
/**
  * Sets the GMAC core in Half-Duplex mode.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_set_half_duplex(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDuplex);
    return;
}

/**
  * GMAC tries retransmission (Only in Half Duplex mode).
  * If collision occurs on the GMII/MII, GMAC attempt retries based on the
  * back off limit configured.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note This function is tightly coupled with gmac_back_off_limit(gmacdev_pt *, u32).
  * note:cdh:check ok
  */
static void gmac_retry_enable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRetry);
    return;
}
/**
  * GMAC tries only one transmission (Only in Half Duplex mode).
  * If collision occurs on the GMII/MII, GMAC will ignore the current frami
  * transmission and report a frame abort with excessive collision in tranmit frame status.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_retry_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRetry);
    return;
}

/**
  * GMAC strips the Pad/FCS field of incoming frames.
  * This is true only if the length field value is less than or equal to
  * 1500 bytes. All received frames with length field greater than or equal to
  * 1501 bytes are passed to the application without stripping the Pad/FCS field.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_pad_crc_strip_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacPadCrcStrip);
    return;
}
/**
  * GMAC doesnot strips the Pad/FCS field of incoming frames.
  * GMAC will pass all the incoming frames to Host unmodified.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_pad_crc_strip_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacPadCrcStrip);
    return;
}

/**
  * GMAC programmed with the back off limit value.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note This function is tightly coupled with gmac_retry_enable(gmac_device * gmacdev)
  * note:cdh:check ok
  */
static void gmac_back_off_limit(gmac_device *gmacdev, u32 value)
{
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->MacBase, GmacConfig);
    data &= (~GmacBackoffLimit);
    data |= value;
    gmac_write_reg((u32 *)gmacdev->MacBase, GmacConfig, data);
    return;
}

/**
  * Enables the Deferral check in GMAC (Only in Half Duplex mode)
  * GMAC issues a Frame Abort Status, along with the excessive deferral error bit set in the
  * transmit frame status when transmit state machine is deferred for more than
  * 	- 24,288 bit times in 10/100Mbps mode
  * 	- 155,680 bit times in 1000Mbps mode or Jumbo frame mode in 10/100Mbps operation.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * \note Deferral begins when transmitter is ready to transmit, but is prevented because of
  * an active CRS (carrier sense)
  */
void gmac_deferral_check_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDeferralCheck);
    return;
}
/**
  * Disables the Deferral check in GMAC (Only in Half Duplex mode).
  * GMAC defers until the CRS signal goes inactive.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_deferral_check_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacDeferralCheck);
    return;
}

/**
  * Selects the GMII port.
  * When called GMII (1000Mbps) port is selected (programmable only in 10/100/1000 Mbps configuration).
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_select_gmii(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacMiiGmii);
    return;
}
/**
  * Selects the MII port.
  * When called MII (10/100Mbps) port is selected (programmable only in 10/100/1000 Mbps configuration).
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_select_mii(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacMiiGmii);
    return;
}

/*Receive frame filter configuration functions*/

/**
  * Enables reception of all the frames to application.
  * GMAC passes all the frames received to application irrespective of whether they
  * pass SA/DA address filtering or not.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_frame_filter_enable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacFilter);
    return;
}
/**
  * Disables reception of all the frames to application.
  * GMAC passes only those received frames to application which
  * pass SA/DA address filtering.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
void gmac_frame_filter_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacFilter);
    return;
}

/**
  * Enables forwarding of control frames.
  * When set forwards all the control frames (incl. unicast and multicast PAUSE frames).
  * @param[in] pointer to gmac_device.
  * \return void.
  * \note Depends on RFE of FlowControlRegister[2]
  * note:cdh:check ok
  */
static void gmac_set_pass_control(gmac_device *gmacdev, u32 passcontrol)
{
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->MacBase, GmacFrameFilter);
    data &= (~GmacPassControl);
    data |= passcontrol;
    gmac_write_reg((u32 *)gmacdev->MacBase, GmacFrameFilter, data);
    return;
}

/**
  * Enables Broadcast frames.
  * When enabled Address filtering module passes all incoming broadcast frames.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_broadcast_enable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacBroadcast);
    return;
}
/**
  * Disable Broadcast frames.
  * When disabled Address filtering module filters all incoming broadcast frames.
  * @param[in] pointer to gmac_device.
  * \return void.
  */
void gmac_broadcast_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacBroadcast);
    return;
}

/**
  * Enables Source address filtering.
  * When enabled source address filtering is performed. Only frames matching SA filtering are passed  to application with
  * SAMatch bit of RxStatus is set. GMAC drops failed frames.
  * @param[in] pointer to gmac_device.
  * \return void.
  * \note This function is overriden by gmac_frame_filter_disable(gmac_device *)
  */
void gmac_src_addr_filter_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacSrcAddrFilter);
    return;
}
/**
  * Disables Source address filtering.
  * When disabled GMAC forwards the received frames with updated SAMatch bit in RxStatus.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_src_addr_filter_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacSrcAddrFilter);
    return;
}

/**
  * Enables Multicast frames.
  * When enabled all multicast frames are passed.
  * @param[in] pointer to gmac_device.
  * \return void.
  */
static void gmac_multicast_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacMulticastFilter);
    return;
}
/**
  * Disable Multicast frames.
  * When disabled multicast frame filtering depends on HMC bit.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_multicast_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacMulticastFilter);
    return;
}

/**
  * Enables the normal Destination address filtering.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_dst_addr_filter_normal(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacDestAddrFilterNor);
    return;
}

/**
  * Enables multicast hash filtering.
  * When enabled GMAC performs teh destination address filtering according to the hash table.
  * @param[in] pointer to gmac_device.
  * \return void.
  */
static void gmac_multicast_hash_filter_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacMcastHashFilter);
    return;
}

/**
  * Disables multicast hash filtering.
  * When disabled GMAC performs perfect destination address filtering for multicast frames, it compares
  * DA field with the value programmed in DA register.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_multicast_hash_filter_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacMcastHashFilter);
    return;
}


/**
  * Clears promiscous mode.
  * When called the GMAC falls back to normal operation from promiscous mode.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
#if 0
static void gmac_promisc_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacPromiscuousMode);
    return;
}
#else
static void gmac_promisc_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacPromiscuousMode);
    return;
}
#endif

/**
  * Enables unicast hash filtering.
  * When enabled GMAC performs the destination address filtering of unicast frames according to the hash table.
  * @param[in] pointer to gmac_device.
  * \return void.
  */
void gmac_unicast_hash_filter_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacUcastHashFilter);
    return;
}
/**
  * Disables multicast hash filtering.
  * When disabled GMAC performs perfect destination address filtering for unicast frames, it compares
  * DA field with the value programmed in DA register.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_unicast_hash_filter_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFrameFilter, GmacUcastHashFilter);
    return;
}

/*Flow control configuration functions*/

/**
  * Enables detection of pause frames with stations unicast address.
  * When enabled GMAC detects the pause frames with stations unicast address in addition to the
  * detection of pause frames with unique multicast address.
  * @param[in] pointer to gmac_device.
  * \return void.
  */
void gmac_unicast_pause_frame_detect_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacUnicastPauseFrame);
    return;
}
/**
  * Disables detection of pause frames with stations unicast address.
  * When disabled GMAC only detects with the unique multicast address (802.3x).
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_unicast_pause_frame_detect_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacUnicastPauseFrame);
    return;
}

/**
  * Rx flow control enable.
  * When Enabled GMAC will decode the rx pause frame and disable the tx for a specified time.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_rx_flow_control_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacRxFlowControl);
    return;
}
/**
  * Rx flow control disable.
  * When disabled GMAC will not decode pause frame.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_rx_flow_control_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacRxFlowControl);
    return;
}

/**
  * Tx flow control enable.
  * When Enabled
  * 	- In full duplex GMAC enables flow control operation to transmit pause frames.
  *	- In Half duplex GMAC enables the back pressure operation
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_tx_flow_control_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacTxFlowControl);
    return;
}

/**
  * Tx flow control disable.
  * When Disabled
  * 	- In full duplex GMAC will not transmit any pause frames.
  *	- In Half duplex GMAC disables the back pressure feature.
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_tx_flow_control_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacFlowControl, GmacTxFlowControl);
    return;
}





/**
  * Example mac initialization sequence.
  * This function calls the initialization routines to initialize the GMAC register.
  * One can change the functions invoked here to have different configuration as per the requirement
  * @param[in] pointer to gmac_device.
  * \return Returns 0 on success.
  */
static s32 gmac_mac_init(gmac_device *gmacdev)
{
    u32 PHYreg;

    if(gmacdev->DuplexMode == FULLDUPLEX)
    {
        gmac_wd_enable(gmacdev);
        gmac_jab_enable(gmacdev);
        gmac_frame_burst_enable(gmacdev);
        gmac_jumbo_frame_disable(gmacdev);
        gmac_rx_own_enable(gmacdev);
        gmac_loopback_off(gmacdev);
        gmac_set_full_duplex(gmacdev);
        gmac_retry_enable(gmacdev);
        gmac_pad_crc_strip_disable(gmacdev);
        gmac_back_off_limit(gmacdev, GmacBackoffLimit0);
        gmac_deferral_check_disable(gmacdev);
        gmac_tx_enable(gmacdev); // CDH:TX Enable
        gmac_rx_enable(gmacdev); // CDH:RX Enable

        if(gmacdev->Speed == SPEED1000)
            gmac_select_gmii(gmacdev);
        else
            gmac_select_mii(gmacdev); // cdh:we use mii

        /*Frame Filter Configuration*/
        gmac_frame_filter_enable(gmacdev);
        gmac_set_pass_control(gmacdev, GmacPassControl0);
        gmac_broadcast_enable(gmacdev);
        gmac_src_addr_filter_disable(gmacdev);
        gmac_multicast_enable(gmacdev);
        gmac_dst_addr_filter_normal(gmacdev);
        gmac_multicast_hash_filter_enable(gmacdev);
        gmac_promisc_enable(gmacdev);
        gmac_unicast_hash_filter_disable(gmacdev);

        /*Flow Control Configuration*/
        gmac_unicast_pause_frame_detect_disable(gmacdev);
        gmac_rx_flow_control_enable(gmacdev);
        gmac_tx_flow_control_enable(gmacdev);
    }
    else //Half Duplex
    {
        gmac_wd_enable(gmacdev);
        gmac_jab_enable(gmacdev);
        gmac_frame_burst_enable(gmacdev);
        gmac_jumbo_frame_disable(gmacdev);
        gmac_rx_own_enable(gmacdev);
        gmac_loopback_off(gmacdev);
        gmac_set_half_duplex(gmacdev);
        gmac_retry_enable(gmacdev);
        gmac_pad_crc_strip_disable(gmacdev);
        gmac_back_off_limit(gmacdev, GmacBackoffLimit0);
        gmac_deferral_check_disable(gmacdev);
        gmac_tx_enable(gmacdev);
        gmac_rx_enable(gmacdev);

        if(gmacdev->Speed == SPEED1000)
            gmac_select_gmii(gmacdev);
        else
            gmac_select_mii(gmacdev);

        /*Frame Filter Configuration*/
        gmac_frame_filter_enable(gmacdev);
        gmac_set_pass_control(gmacdev, GmacPassControl0);
        gmac_broadcast_enable(gmacdev);
        gmac_src_addr_filter_disable(gmacdev);
        gmac_multicast_disable(gmacdev);
        gmac_dst_addr_filter_normal(gmacdev);
        gmac_multicast_hash_filter_disable(gmacdev);
        gmac_promisc_enable(gmacdev);
        gmac_unicast_hash_filter_disable(gmacdev);

        /*Flow Control Configuration*/
        gmac_unicast_pause_frame_detect_disable(gmacdev);
        gmac_rx_flow_control_disable(gmacdev);
        gmac_tx_flow_control_disable(gmacdev);

        /*To set PHY register to enable CRS on Transmit*/
        gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr, GmiiBusy | 0x00000408);
        PHYreg = gmac_read_reg((u32 *)gmacdev->MacBase, GmacGmiiData);
        gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiData, PHYreg   | 0x00000800);
        gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr, GmiiBusy | 0x0000040a);
    }
    return 0;
}

/**
  * Checks whether the descriptor is owned by DMA.
  * If descriptor is owned by DMA then the OWN bit is set to 1. This API is same for both ring and chain mode.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if Dma owns descriptor and false if not.
  */
bool gmac_is_desc_owned_by_dma(DmaDesc *desc)
{
    return ((desc->status & DescOwnByDma) == DescOwnByDma );
}

/**
  * Checks whether the descriptor is empty.
  * If the buffer1 and buffer2 lengths are zero in ring mode descriptor is empty.
  * In chain mode buffer2 length is 0 but buffer2 itself contains the next descriptor address.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if descriptor is empty, false if not empty.
  * note:cdh:check ok
  */
bool gmac_is_desc_empty(DmaDesc *desc)
{
    //if both the buffer1 length and buffer2 length are zero desc is empty
    return(((desc->length  & DescSize1Mask) == 0) && ((desc->length  & DescSize2Mask) == 0) );
}

/**
  * Checks whether this rx descriptor is last rx descriptor.
  * This returns true if it is last descriptor either in ring mode or in chain mode.
  * @param[in] pointer to devic structure.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if it is last descriptor, false if not.
  * \note This function should not be called before initializing the descriptor using synopGMAC_desc_init().
  * note:cdh:check ok
  */
bool gmac_is_last_rx_desc(gmac_device *gmacdev, DmaDesc *desc)
{
    //bool synopGMAC_is_last_desc(DmaDesc *desc)
    return (((desc->length & RxDescEndOfRing) == RxDescEndOfRing) || ((u32)gmacdev->RxDesc == desc->data2));
}

/**
  * Initialize the rx descriptors for chain mode of operation.
  * 	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0.
  *	- data1 and data2 set to 0.
  * @param[in] pointer to DmaDesc structure.
  * @param[in] whether end of ring
  * \return void.
  */

void gmac_rx_desc_init_chain(DmaDesc *desc)
{
    desc->status = 0;
    desc->length = RxDescChain;
    desc->buffer1 = 0;
    desc->data1 = 0;
    return;
}



/**
  * Get back the descriptor from DMA after data has been received.
  * When the DMA indicates that the data is received (interrupt is generated), this function should be
  * called to get the descriptor and hence the data buffers received. With successful return from this
  * function caller gets the descriptor fields for processing. check the parameters to understand the
  * fields returned.`
  * @param[in] pointer to gmac_device.
  * @param[out] pointer to hold the status of DMA.
  * @param[out] Dma-able buffer1 pointer.
  * @param[out] pointer to hold length of buffer1 (Max is 2048).
  * @param[out] virtual pointer for buffer1.
  * @param[out] Dma-able buffer2 pointer.
  * @param[out] pointer to hold length of buffer2 (Max is 2048).
  * @param[out] virtual pointer for buffer2.
  * \return returns present rx descriptor index on success. Negative value if error.
  */
static s32 gmac_get_rx_qptr(gmac_device *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 *Length2, u32 *Data2)
{
    u32 rxnext       = gmacdev->RxBusy;	// index of descriptor the DMA just completed. May be useful when data
    //is spread over multiple buffers/descriptors
    DmaDesc *rxdesc = gmacdev->RxBusyDesc;
    if(gmac_is_desc_owned_by_dma(rxdesc))
        return -1;
    if(gmac_is_desc_empty(rxdesc))
        return -1;


    if(Status != 0)
        *Status = rxdesc->status;// send the status of this descriptor

    if(Length1 != 0)
        *Length1 = (rxdesc->length & DescSize1Mask) >> DescSize1Shift;
    if(Buffer1 != 0)
        *Buffer1 = rxdesc->buffer1;
    if(Data1 != 0)
        *Data1 = rxdesc->data1;

    if(Length2 != 0)
        *Length2 = (rxdesc->length & DescSize2Mask) >> DescSize2Shift;
    if(Buffer2 != 0)
        *Buffer2 = rxdesc->buffer2;
    if(Data1 != 0)
        *Data2 = rxdesc->data2;

    gmacdev->RxBusy = gmac_is_last_rx_desc(gmacdev, rxdesc) ? 0 : rxnext + 1;

    if(gmac_is_rx_desc_chained(rxdesc))
    {
        gmacdev->RxBusyDesc = (DmaDesc *)rxdesc->data2;
        gmac_rx_desc_init_chain(rxdesc);
        //synopGMAC_desc_init_chain(rxdesc, gmac_is_last_rx_desc(gmacdev,rxdesc),0,0);
    }
    else
    {
        gmacdev->RxBusyDesc = gmac_is_last_rx_desc(gmacdev, rxdesc) ? gmacdev->RxDesc : (rxdesc + 1);
        gmac_rx_desc_init_ring(rxdesc, gmac_is_last_rx_desc(gmacdev, rxdesc));
    }
   // TR("(get rx)%02d %08x %08x %08x %08x %08x %08x %08x\n", rxnext, (u32)rxdesc, rxdesc->status, rxdesc->length, rxdesc->buffer1, rxdesc->buffer2, rxdesc->data1, rxdesc->data2);
    (gmacdev->BusyRxDesc)--; //This returns one descriptor to processor. So busy count will be decremented by one

    return(rxnext);

}



/**
  * Reads the time stamp higher sec value to respective pointers
  * @param[in] pointer to gmac_device
  * @param[in] pointer to hold 16 higher bit second register contents
  * \return returns void
  */
void gmac_TS_read_timestamp_higher_val(gmac_device *gmacdev, u16 *higher_sec_val)
{
    * higher_sec_val = (u16)(gmac_read_reg((u32 *)gmacdev->MacBase, GmacTSHighWord) & GmacTSHighWordMask);
    return;
}

/**
  * Checks whether the rx descriptor is valid.
  * if rx descripor is not in error and complete frame is available in the same descriptor
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if no error and first and last desc bits are set, otherwise it returns false.
  */
bool gmac_is_rx_desc_valid(u32 status)
{
    return ((status & DescError) == 0) && ((status & DescRxFirst) == DescRxFirst) && ((status & DescRxLast) == DescRxLast);
}

/**
  * returns the byte length of received frame including CRC.
  * This returns the no of bytes received in the received ethernet frame including CRC(FCS).
  * @param[in] pointer to DmaDesc structure.
  * \return returns the length of received frame lengths in bytes.
  */
u32 gmac_get_rx_desc_frame_length(u32 status)
{
    return ((status & DescFrameLengthMask) >> DescFrameLengthShift);
}


/**
  * Decodes the Rx Descriptor status to various checksum error conditions.
  * @param[in] pointer to gmac_device.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns decoded enum (u32) indicating the status.
  */
u32 gmac_is_rx_checksum_error(gmac_device *gmacdev, u32 status)
{
    if     (((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) == 0))
        return RxLenLT600;
    else if(((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) != 0))
        return RxIpHdrPayLoadChkBypass;
    else if(((status & DescRxChkBit5) == 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) != 0))
        return RxChkBypass;
    else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) == 0))
        return RxNoChkError;
    else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) == 0) && ((status & DescRxChkBit0) != 0))
        return RxPayLoadChkError;
    else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) == 0))
        return RxIpHdrChkError;
    else if(((status & DescRxChkBit5) != 0) && ((status & DescRxChkBit7) != 0) && ((status & DescRxChkBit0) != 0))
        return RxIpHdrPayLoadChkError;
    else
        return RxIpHdrPayLoadRes;
}

/**
  * Check for damaged frame due to collision.
  * Retruns true if rx frame was damaged due to late collision in half duplex mode.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if error else returns false.
  */
bool gmac_is_rx_frame_collision(u32 status)
{
    //bool synopGMAC_dma_rx_collisions(u32 status)
    return ((status & DescRxCollision) == DescRxCollision);
}

/**
  * Check for receive CRC error.
  * Retruns true if rx frame CRC error occured.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if error else returns false.
  */
bool gmac_is_rx_crc(u32 status)
{
    //u32 synopGMAC_dma_rx_crc(u32 status)
    return ((status & DescRxCrc) == DescRxCrc);
}

/**
  * Indicates rx frame has non integer multiple of bytes. (odd nibbles).
  * Retruns true if dribbling error in rx frame.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if error else returns false.
  */
bool gmac_is_frame_dribbling_errors(u32 status)
{
    //u32 synopGMAC_dma_rx_frame_errors(u32 status)
    return ((status & DescRxDribbling) == DescRxDribbling);
}

/**
  * Indicates error in rx frame length.
  * Retruns true if received frame length doesnot match with the length field
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if error else returns false.
  */
bool gmac_is_rx_frame_length_errors(u32 status)
{
    //u32 synopGMAC_dma_rx_length_errors(u32 status)
    return((status & DescRxLengthError) == DescRxLengthError);
}


/**
  * Prepares the descriptor to receive packets.
  * The descriptor is allocated with the valid buffer addresses (sk_buff address) and the length fields
  * and handed over to DMA by setting the ownership. After successful return from this function the
  * descriptor is added to the receive descriptor pool/queue.
  * This api is same for both ring mode and chain mode.
  * @param[in] pointer to gmac_device.
  * @param[in] Dma-able buffer1 pointer.
  * @param[in] length of buffer1 (Max is 2048).
  * @param[in] Dma-able buffer2 pointer.
  * @param[in] length of buffer2 (Max is 2048).
  * @param[in] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * \return returns present rx descriptor index on success. Negative value if error.
  */
static s32 gmac_set_rx_qptr(gmac_device *gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2)
{
    u32  rxnext      = gmacdev->RxNext;
    DmaDesc *rxdesc = gmacdev->RxNextDesc;

	//TR("cdh:%s, rxnext=0x%x, rxdesc=%d, Buffer1=0x%x\n", __func__, rxnext, rxdesc, Buffer1);
    if(!gmac_is_desc_empty(rxdesc))
        return -1;

    if(gmac_is_rx_desc_chained(rxdesc))
    {
    	//TR("cdh:chain queue set!\n");
        rxdesc->length |= ((Length1 << DescSize1Shift) & DescSize1Mask);

        rxdesc->buffer1 = Buffer1; // cdh:rx data address
        rxdesc->data1 = Data1;

        if((rxnext % MODULO_INTERRUPT) != 0)
            rxdesc->length |= RxDisIntCompl;

        rxdesc->status = DescOwnByDma;

        gmacdev->RxNext     = gmac_is_last_rx_desc(gmacdev, rxdesc) ? 0 : rxnext + 1;
        gmacdev->RxNextDesc = (DmaDesc *)rxdesc->data2;
    }
    else
    {
    	//TR("cdh:ring queue set!\n");
        rxdesc->length |= (((Length1 << DescSize1Shift) & DescSize1Mask) | ((Length2 << DescSize2Shift) & DescSize2Mask));

        rxdesc->buffer1 = Buffer1;
        rxdesc->data1 = Data1;

        rxdesc->buffer2 = Buffer2;
        rxdesc->data2 = Data2;

        if((rxnext % MODULO_INTERRUPT) != 0)
            rxdesc->length |= RxDisIntCompl;

        rxdesc->status = DescOwnByDma;

        gmacdev->RxNext     = gmac_is_last_rx_desc(gmacdev, rxdesc) ? 0 : rxnext + 1;
        gmacdev->RxNextDesc = gmac_is_last_rx_desc(gmacdev, rxdesc) ? gmacdev->RxDesc : (rxdesc + 1);
    }
    //TR("(set rx)%02d %08x %08x %08x %08x %08x %08x %08x\n", rxnext, (u32)rxdesc, rxdesc->status, rxdesc->length, rxdesc->buffer1, rxdesc->buffer2, rxdesc->data1, rxdesc->data2);
    (gmacdev->BusyRxDesc)++; //One descriptor will be given to Hardware. So busy count incremented by one
    return rxnext;
}



/**
  * Checks whether this tx descriptor is last tx descriptor.
  * This returns true if it is last descriptor either in ring mode or in chain mode.
  * @param[in] pointer to devic structure.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if it is last descriptor, false if not.
  * \note This function should not be called before initializing the descriptor using synopGMAC_desc_init().
  */
bool gmac_is_last_tx_desc(gmac_device *gmacdev, DmaDesc *desc)
{
    //bool synopGMAC_is_last_desc(DmaDesc *desc)
#ifdef ENH_DESC
    return (((desc->status & TxDescEndOfRing) == TxDescEndOfRing) || ((u32)gmacdev->TxDesc == desc->data2));
#else
    return (((desc->length & TxDescEndOfRing) == TxDescEndOfRing) || ((u32)gmacdev->TxDesc == desc->data2));
#endif
}


/**
  * Initialize the rx descriptors for chain mode of operation.
  * 	- Status field is initialized to 0.
  *	- EndOfRing set for the last descriptor.
  *	- buffer1 and buffer2 set to 0.
  *	- data1 and data2 set to 0.
  * @param[in] pointer to DmaDesc structure.
  * @param[in] whether end of ring
  * \return void.
  * note:cdh:check ok
  */
void gmac_tx_desc_init_chain(DmaDesc *desc)
{
#ifdef ENH_DESC
    desc->status = TxDescChain;
    desc->length = 0;
#else
    desc->length = TxDescChain;
#endif
    desc->buffer1 = 0;
    desc->data1 = 0;
    return;
}


/**
  * Get the index and address of Tx desc.
  * This api is same for both ring mode and chain mode.
  * This function tracks the tx descriptor the DMA just closed after the transmission of data from this descriptor is
  * over. This returns the descriptor fields to the caller.
  * @param[in] pointer to gmac_device.
  * @param[out] status field of the descriptor.
  * @param[out] Dma-able buffer1 pointer.
  * @param[out] length of buffer1 (Max is 2048).
  * @param[out] virtual pointer for buffer1.
  * @param[out] Dma-able buffer2 pointer.
  * @param[out] length of buffer2 (Max is 2048).
  * @param[out] virtual pointer for buffer2.
  * @param[out] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * \return returns present tx descriptor index on success. Negative value if error.
  */
s32 gmac_get_tx_qptr(gmac_device *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 *Length2, u32 *Data2 )
{
    u32 txover = gmacdev->TxBusy;
    DmaDesc *txdesc = gmacdev->TxBusyDesc;

    if(gmac_is_desc_owned_by_dma(txdesc))
        return -1;
    if(gmac_is_desc_empty(txdesc))
        return -1;

    (gmacdev->BusyTxDesc)--;

    if(Status != 0)
        *Status = txdesc->status;

    if(Buffer1 != 0)
        *Buffer1 = txdesc->buffer1;
    if(Length1 != 0)
        *Length1 = (txdesc->length & DescSize1Mask) >> DescSize1Shift;
    if(Data1 != 0)
        *Data1 = txdesc->data1;

    if(Buffer2 != 0)
        *Buffer2 = txdesc->buffer2;
    if(Length2 != 0)
        *Length2 = (txdesc->length & DescSize2Mask) >> DescSize2Shift;
    if(Data1 != 0)
        *Data2 = txdesc->data2;

    gmacdev->TxBusy = gmac_is_last_tx_desc(gmacdev, txdesc) ? 0 : txover + 1;

    if(gmac_is_tx_desc_chained(txdesc))
    {
        gmacdev->TxBusyDesc = (DmaDesc *)txdesc->data2;
        gmac_tx_desc_init_chain(txdesc);
    }
    else
    {
        gmacdev->TxBusyDesc = gmac_is_last_tx_desc(gmacdev, txdesc) ? gmacdev->TxDesc : (txdesc + 1);
        gmac_tx_desc_init_ring(txdesc, gmac_is_last_tx_desc(gmacdev, txdesc));
    }
    //TR("(get tx)%02d %08x %08x %08x %08x %08x %08x %08x\n", txover, (u32)txdesc, txdesc->status, txdesc->length, txdesc->buffer1, txdesc->buffer2, txdesc->data1, txdesc->data2);

    return txover;
}

/**
 * Function to Receive a packet from the interface.
 * After Receiving a packet, DMA transfers the received packet to the system memory
 * and generates corresponding interrupt (if it is enabled). This function prepares
 * the sk_buff for received packet after removing the ethernet CRC, and hands it over
 * to linux networking stack.
 * 	- Updataes the networking interface statistics
 *	- Keeps track of the rx descriptors
 * @param[in] pointer to net_device structure.
 * \return void.
 * \note This function runs in interrupt context.
 */

void synop_handle_received_data(struct net_device *netdev)
{
    nt_adapter *adapter;
    gmac_device *gmacdev;
   // struct pci_dev *pcidev;
    s32 desc_index;

    u32 data1;
    u32 data2;
    u32 len;
    u32 status;
	//u32 rxcsum_status = 0; // cdh:add
    u32 dma_addr1;
    u32 dma_addr2;
    //u32 length;

    struct sk_buff *skb; //This is the pointer to hold the received data

    //TR("%s\n", __FUNCTION__);

    adapter = (nt_adapter *)netdev_priv(netdev);
    if(adapter == NULL)
    {
        TR("Unknown Device\n");
        return;
    }

    gmacdev = adapter->gmacdev_pt;
    if(gmacdev == NULL)
    {
        TR("GMAC device structure is missing\n");
        return;
    }

    // cdh:pcidev  = (struct pci_dev *)adapter->pcidev_pt;
    /*Handle the Receive Descriptors*/
    do
    {
        desc_index = gmac_get_rx_qptr(gmacdev, &status, &dma_addr1, NULL, &data1, &dma_addr2, NULL, &data2);

        //desc_index = gmac_get_rx_qptr(gmacdev, &status,&dma_addr,NULL, &data1);
        if(desc_index >= 0 && data1 != 0)
        {
//            TR("Received Data at Rx Descriptor %d for skb 0x%08x whose status is %08x\n", desc_index, data1, status);
            /*At first step unmapped the dma address*/
            dma_unmap_single(NULL, dma_addr1, 0, DMA_FROM_DEVICE);

            skb = (struct sk_buff *)data1;
            if(gmac_is_rx_desc_valid(status))
            {
                len =  gmac_get_rx_desc_frame_length(status) - 4;
                //printf("real len: %x\n", len);
                //printf("1, skb, head, data, tail: %x, %x, %x, %x\n", skb->head, skb->data, skb->tail, skb->len);
                skb_put(skb, len);

				#ifdef IPC_OFFLOAD
				TR("Checksum Offloading will be done now\n");
				skb->ip_summed = CHECKSUM_UNNECESSARY;

				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxNoChkError ){
					//printk(KERN_ERR "Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 4>  \n");
					skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass computing the Checksum
				}
				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError ){
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					//printk(KERN_ERR " Error in 16bit IPV4 Header Checksum <Chk Status = 6>  \n");
					skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass the TCP/UDP checksum computation
				}	
				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxLenLT600 ){
					//printk(KERN_ERR "IEEE 802.3 type frame with Length field Lesss than 0x0600 <Chk Status = 0> \n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrPayLoadChkBypass ){
					//printk(KERN_ERR "Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 1>\n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxChkBypass ){
					//printk(KERN_ERR "Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 3>  \n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxPayLoadChkError ){
					//printk(KERN_ERR " TCP/UDP payload checksum Error <Chk Status = 5>  \n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError ){
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					//printk(KERN_ERR " Both IP header and Payload Checksum Error <Chk Status = 7>  \n");
					skb->ip_summed = CHECKSUM_NONE;	        //Let Kernel compute the Checksum
				}
				
				#endif 

				
                //printf("2, skb, head, data, tail: %x, %x, %x, %x\n", skb->head, skb->data, skb->tail, skb->len);
                skb->dev = netdev;
                skb->protocol = eth_type_trans(skb, netdev);
                netif_rx(skb); //printk("netif_rx end!\n");

                netdev->last_rx = jiffies;
                adapter->net_dev_stats.rx_packets++;
                adapter->net_dev_stats.rx_bytes += len;
                //printk("max rx end\n");
            }
            else
            {
                /*Now the present skb should be set free*/
                dev_kfree_skb_irq(skb);
                TR("s: %08x\n", status);
                adapter->net_dev_stats.rx_errors++;
                adapter->net_dev_stats.collisions       += gmac_is_rx_frame_collision(status);
                adapter->net_dev_stats.rx_crc_errors    += gmac_is_rx_crc(status);
                adapter->net_dev_stats.rx_frame_errors  += gmac_is_frame_dribbling_errors(status);
                adapter->net_dev_stats.rx_length_errors += gmac_is_rx_frame_length_errors(status);
            }

            //Now lets allocate the skb for the emptied descriptor
            //TR("SKB memory allocation ... \n");
            skb = dev_alloc_skb(netdev->mtu + ETHERNET_PACKET_EXTRA);
            if(skb == NULL)
            {
                TR("SKB memory allocation failed \n");
                adapter->net_dev_stats.rx_dropped++;
                continue;
            }else{
	            //TR("SKB memory allocation ok \n");
            }

            // cdh:dma_addr = pci_map_single(pcidev, skb->data, skb_tailroom(skb), PCI_DMA_FROMDEVICE);

            dma_addr1 = dma_map_single(NULL, skb->data, skb_tailroom(skb), DMA_FROM_DEVICE);
            //TR("SKB map ok \n");

            desc_index = gmac_set_rx_qptr(gmacdev, dma_addr1, skb_tailroom(skb), (u32)skb, 0, 0, 0);
            if(desc_index < 0)
            {
                //TR("Cannot set Rx Descriptor for skb %08x\n", (u32)skb);
                dev_kfree_skb_irq(skb);
            }
            //TR("set rx qptr ok \n");

        }
    }while(desc_index >= 0);
}

/**
  * Resumes the DMA Transmission.
  * the DmaTxPollDemand is written. (the data writeen could be anything).
  * This forces the DMA to resume transmission.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
void gmac_resume_dma_tx(gmac_device *gmacdev)
{
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaTxPollDemand, 0);

}
/**
  * Resumes the DMA Reception.
  * the DmaRxPollDemand is written. (the data writeen could be anything).
  * This forces the DMA to resume reception.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_resume_dma_rx(gmac_device *gmacdev)
{
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaRxPollDemand, 0);

}

/**
  * Checks if any Ipv4 header checksum error in the frame just transmitted.
  * This serves as indication that error occureed in the IPv4 header checksum insertion.
  * The sent out frame doesnot carry any ipv4 header checksum inserted by the hardware.
  * @param[in] pointer to gmac_device.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns true if error in ipv4 header checksum, else returns false.
  */
bool gmac_is_tx_ipv4header_checksum_error(gmac_device *gmacdev, u32 status)
{
    return((status & DescTxIpv4ChkError) == DescTxIpv4ChkError);
}

/**
  * Checks if any payload checksum error in the frame just transmitted.
  * This serves as indication that error occureed in the payload checksum insertion.
  * The sent out frame doesnot carry any payload checksum inserted by the hardware.
  * @param[in] pointer to gmac_device.
  * @param[in] u32 status field of the corresponding descriptor.
  * \return returns true if error in ipv4 header checksum, else returns false.
  */
bool gmac_is_tx_payload_checksum_error(gmac_device *gmacdev, u32 status)
{
    return((status & DescTxPayChkError) == DescTxPayChkError);
}

/**
  * Checks whether the descriptor is valid
  * if no errors such as CRC/Receive Error/Watchdog Timeout/Late collision/Giant Frame/Overflow/Descriptor
  * error the descritpor is said to be a valid descriptor.
  * @param[in] pointer to DmaDesc structure.
  * \return True if desc valid. false if error.
  */
bool gmac_is_desc_valid(u32 status)
{
    return ((status & DescError) == 0);
}

/**
  * Checks whether the tx is aborted due to collisions.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if collisions, else returns false.
  */
bool gmac_is_tx_aborted(u32 status)
{
    return (((status & DescTxLateCollision) == DescTxLateCollision) | ((status & DescTxExcCollisions) == DescTxExcCollisions));

}

/**
  * Checks whether the tx carrier error.
  * @param[in] pointer to DmaDesc structure.
  * \return returns true if carrier error occured, else returns falser.
  */
bool gmac_is_tx_carrier_error(u32 status)
{
    return (((status & DescTxLostCarrier) == DescTxLostCarrier)  | ((status & DescTxNoCarrier) == DescTxNoCarrier));
}

/**
  * Gives the transmission collision count.
  * returns the transmission collision count indicating number of collisions occured before the frame was transmitted.
  * Make sure to check excessive collision didnot happen to ensure the count is valid.
  * @param[in] pointer to DmaDesc structure.
  * \return returns the count value of collision.
  */
u32 gmac_get_tx_collision_count(u32 status)
{
    return ((status & DescTxCollMask) >> DescTxCollShift);
}
u32 gmac_is_exc_tx_collisions(u32 status)
{
    return ((status & DescTxExcCollisions) == DescTxExcCollisions);
}



/**
 * Function to handle housekeeping after a packet is transmitted over the wire.
 * After the transmission of a packet DMA generates corresponding interrupt
 * (if it is enabled). It takes care of returning the sk_buff to the linux
 * kernel, updating the networking statistics and tracking the descriptors.
 * @param[in] pointer to net_device structure.
 * \return void.
 * \note This function runs in interrupt context
 */
void synop_handle_transmit_over(struct net_device *netdev)
{
    nt_adapter *adapter;
    gmac_device *gmacdev;
    // cdh:struct pci_dev *pcidev;
    s32 desc_index;
    u32 data1, data2;
    u32 status;
    u32 length1, length2;
    u32 dma_addr1, dma_addr2;

    adapter = (nt_adapter *)netdev_priv(netdev);
    if(adapter == NULL)
    {
        TR("Unknown Device\n");
        return;
    }

    gmacdev = adapter->gmacdev_pt;
    if(gmacdev == NULL)
    {
        TR("GMAC device structure is missing\n");
        return;
    }

    // cdh: pcidev  = (struct pci_dev *)adapter->pcidev_pt;
    /*Handle the transmit Descriptors*/
    do {

        desc_index = gmac_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);

        //desc_index = gmac_get_tx_qptr(gmacdev, &status, &dma_addr, &length, &data1);
        if(desc_index >= 0 && data1 != 0)
        {
        	#ifdef	IPC_OFFLOAD
			if(synopGMAC_is_tx_ipv4header_checksum_error(gmacdev, status)){
				printk(KERN_ERR "Harware Failed to Insert IPV4 Header Checksum\n");
			}
			
			if(synopGMAC_is_tx_payload_checksum_error(gmacdev, status)){
				printk(KERN_ERR "Harware Failed to Insert Payload Checksum\n");
			}
			#endif
			
	        //TR("Tx over: %x\n", desc_index);
            //TR("Finished Transmit at Tx Descriptor %d for skb 0x%08x and buffer = %08x whose status is %08x \n", desc_index, data1, dma_addr1, status);
            dma_unmap_single(NULL, dma_addr1, length1, DMA_TO_DEVICE);
            dev_kfree_skb_irq((struct sk_buff *)data1);
            if(gmac_is_desc_valid(status))
            {
                adapter->net_dev_stats.tx_bytes += length1;
                adapter->net_dev_stats.tx_packets++;
            }
            else
            {
                TR("Error in Status %08x\n", status);
                adapter->net_dev_stats.tx_errors++;
                adapter->net_dev_stats.tx_aborted_errors += gmac_is_tx_aborted(status);
                adapter->net_dev_stats.tx_carrier_errors += gmac_is_tx_carrier_error(status);
            }
        }
        adapter->net_dev_stats.collisions += gmac_get_tx_collision_count(status);
    } while(desc_index >= 0);
    netif_wake_queue(netdev);
}



/**
 * Interrupt service routing bottom half.
 * This is the function registered as ISR for device interrupts.
 * @param[in] interrupt number.
 * @param[in] void pointer to device unique structure (Required for shared interrupts in Linux).
 * @param[in] pointer to pt_regs (not used).
 * \return Returns IRQ_NONE if not device interrupts IRQ_HANDLED for device interrupts.
 * \note This function runs in interrupt context
 * \note cdh:check ok
 */
irqreturn_t gmac_intr_handler(s32 intr_num, void *dev_id)
{
    struct net_device *netdev;
    nt_adapter *adapter;
    gmac_device *gmacdev;
    u32 interrupt, dma_status_reg;
    s32 status;
    u32 dma_addr;

    netdev  = (struct net_device *)dev_id;
    if(netdev == NULL)
    {
        TR("Unknown Device\n");
        return -1;
    }

    adapter  = (nt_adapter *)netdev_priv(netdev);
    if(adapter == NULL)
    {
        TR("Adapter Structure Missing\n");
        return -1;
    }

    gmacdev = adapter->gmacdev_pt;
    if(gmacdev == NULL)
    {
        TR("GMAC device structure Missing\n");
        return -1;
    }

    // cdh:pcidev  = (struct pci_dev *)adapter->pcidev_pt;

    /*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
    dma_status_reg = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);

    if(dma_status_reg == 0)
        return IRQ_NONE;

    gmac_disable_interrupt_all(gmacdev);

    TR("(bh)GMAC INTs: 0x%08x\n", dma_status_reg);

	// cdh:power managent module(PMT) 
    if(dma_status_reg & GmacPmtIntr)
    {
        TR("%s:: Interrupt due to PMT module\n", __FUNCTION__);
        powerup_mac(gmacdev);
    }

	// cdh:check mmc interrupt
    if(dma_status_reg & GmacMmcIntr) 
    {
        TR("%s:: Interrupt due to MMC module\n", __FUNCTION__);
        TR("%s:: synopGMAC_rx_int_status = %08x\n", __FUNCTION__, gmac_read_mmc_rx_int_status(gmacdev));
        TR("%s:: synopGMAC_tx_int_status = %08x\n", __FUNCTION__, gmac_read_mmc_tx_int_status(gmacdev));
    }

	// cdh:check pcs(link change and an complete)
    if(dma_status_reg & GmacLineIntfIntr)
    {
        TR("%s:: Interrupt due to GMAC LINE module\n", __FUNCTION__);
    }

    /*Now lets handle the DMA interrupts*/
    interrupt = gmac_get_interrupt_type(gmacdev);
    //TR("%s: INTs to be handled: 0x%08x\n", __FUNCTION__, interrupt);

    if(interrupt & synopGMACDmaError)
    {
        TR("%s::Fatal Bus Error Inetrrupt Seen\n", __FUNCTION__);
        gmac_disable_dma_tx(gmacdev);
        gmac_disable_dma_rx(gmacdev);

        gmac_take_desc_ownership_tx(gmacdev);
        gmac_take_desc_ownership_rx(gmacdev);

        gmac_init_tx_rx_desc_queue(gmacdev);

        gmac_reset(gmacdev);//reset the DMA engine and the GMAC ip

        gmac_set_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, netdev->dev_addr);
        gmac_set_dma_bus_mode(gmacdev, DmaFixedBurstEnable | DmaBurstLength8 | DmaDescriptorSkip2 );
        gmac_set_dma_control(gmacdev, DmaStoreAndForward);
        gmac_init_rx_desc_base(gmacdev);
        gmac_init_tx_desc_base(gmacdev);
        gmac_mac_init(gmacdev);
        gmac_enable_dma_rx(gmacdev);
        gmac_enable_dma_tx(gmacdev);
    }

    if(interrupt & synopGMACDmaRxNormal)
    {
        //TR("%s:: Rx Normal\n", __FUNCTION__);
        synop_handle_received_data(netdev);
    }

    if(interrupt & synopGMACDmaRxAbnormal)
    {
        TR("%s::Abnormal Rx Interrupt Seen\n", __FUNCTION__);
#if 1
        if(GMAC_Power_down == 0) // If Mac is not in powerdown
        {
            adapter->net_dev_stats.rx_over_errors++;
            /*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
            gmac_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
        }
#endif
    }

    if(interrupt & synopGMACDmaRxStopped)
    {
        TR("%s::Receiver stopped seeing Rx interrupts\n", __FUNCTION__); //Receiver gone in to stopped state
#if 1
        if(GMAC_Power_down == 0) // If Mac is not in powerdown
        {
            adapter->net_dev_stats.rx_over_errors++;
            do {
                struct sk_buff *skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_ATOMIC);
                if(skb == NULL)
                {
                    TR("%s::ERROR in skb buffer allocation Better Luck Next time\n", __FUNCTION__);
                    break;
                    // return -ESYNOPGMACNOMEM;
                }

                dma_addr = dma_map_single(NULL, skb->data, skb_tailroom(skb), DMA_FROM_DEVICE);
                status = gmac_set_rx_qptr(gmacdev, dma_addr, skb_tailroom(skb), (u32)skb, 0, 0, 0);
                TR("%s::Set Rx Descriptor no %08x for skb %08x \n", __FUNCTION__, status, (u32)skb);
                if(status < 0)
                    dev_kfree_skb_irq(skb);//changed from dev_free_skb. If problem check this again

            } while(status >= 0);
            gmac_enable_dma_rx(gmacdev);
        }
#endif
    }

    if(interrupt & synopGMACDmaTxNormal)
    {
        //xmit function has done its job
        //TR("%s::Finished Normal Transmission \n", __FUNCTION__);
        synop_handle_transmit_over(netdev);//Do whatever you want after the transmission is over
    }

    if(interrupt & synopGMACDmaTxAbnormal)
    {
        TR("%s::Abnormal Tx Interrupt Seen\n", __FUNCTION__);
#if 1
        if(GMAC_Power_down == 0) 	// If Mac is not in powerdown
        {
            synop_handle_transmit_over(netdev);
        }
#endif
    }

    if(interrupt & synopGMACDmaTxStopped)
    {
        TR("%s::Transmitter stopped sending the packets\n", __FUNCTION__);
#if 1
        if(GMAC_Power_down == 0)// If Mac is not in powerdown
        {
            gmac_disable_dma_tx(gmacdev);
            gmac_take_desc_ownership_tx(gmacdev);

            gmac_enable_dma_tx(gmacdev);
            netif_wake_queue(netdev);
            TR("%s::Transmission Resumed\n", __FUNCTION__);
        }
#endif
    }

    /* Enable the interrrupt before returning from ISR*/
    gmac_enable_interrupt(gmacdev, DmaIntEnable);
    return IRQ_HANDLED;
}


/**
  * This sets up the transmit Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures for ring mode or chain mode.
  * This function depends on the pcidev structure for allocation consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to gmac_device.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in tx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->TxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements
  * note:cdh:check ok
  */
static s32 setup_tx_desc_queue(gmac_device *gmacdev, struct net_device *pnetdev, u32 no_of_desc, u32 desc_mode)
{
    s32 i;

    DmaDesc *first_desc = NULL;
    DmaDesc *second_desc = NULL;
    dma_addr_t dma_addr;
    gmacdev->TxDescCount = 0;

    if(desc_mode == RINGMODE)
    {
        //printk("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n", ((sizeof(DmaDesc) * no_of_desc)));
        first_desc = (DmaDesc *)plat_alloc_consistent_mem(pnetdev, sizeof(DmaDesc) * no_of_desc, &dma_addr);
        if(first_desc == NULL)
        {
            printk("Error in Tx Descriptors memory allocation\n");
            return -ESYNOPGMACNOMEM;
        }
        gmacdev->TxDescCount = no_of_desc;	// cdh:ring descriptor count
        gmacdev->TxDesc      = first_desc;	// cdh:ring descriptor base virtual address
        gmacdev->TxDescDma   = dma_addr;	// cdh:ring descriptor base physic address

        for(i = 0; i < gmacdev->TxDescCount; i++)
        {
        	// cdh:why not init dma fifo start addresss???
            gmac_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount - 1);
            //printk("%02d %08x \n", i, (unsigned int)(gmacdev->TxDesc + i) );
        }

    }
    else
    {
        //Allocate the head descriptor
        first_desc = (DmaDesc *)plat_alloc_consistent_mem(pnetdev, sizeof(DmaDesc), &dma_addr);
        if(first_desc == NULL)
        {
            printk("Error in Tx Descriptor Memory allocation in Ring mode\n");
            return -ESYNOPGMACNOMEM;
        }
        gmacdev->TxDesc       = first_desc; // cdh:ring descriptor base virtual address
        gmacdev->TxDescDma    = dma_addr;	// cdh:ring descriptor base physic address

        printk("Tx===================================================================Tx\n");
        first_desc->buffer2   = gmacdev->TxDescDma;
        first_desc->data2     = (u32)gmacdev->TxDesc;

        gmacdev->TxDescCount = 1;

        for(i = 0; i < (no_of_desc - 1); i++)
        {
            second_desc = plat_alloc_consistent_mem(pnetdev, sizeof(DmaDesc), &dma_addr);
            if(second_desc == NULL)
            {
                printk("Error in Tx Descriptor Memory allocation in Chain mode\n");
                return -ESYNOPGMACNOMEM;
            }
            first_desc->buffer2  = dma_addr;
            first_desc->data2    = (u32)second_desc;

            second_desc->buffer2 = gmacdev->TxDescDma;
            second_desc->data2   = (u32)gmacdev->TxDesc;

            gmac_tx_desc_init_chain(first_desc);
            printk("%02d %08x %08x %08x %08x %08x %08x %08x \n", gmacdev->TxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1, first_desc->buffer2, first_desc->data1, first_desc->data2);
            gmacdev->TxDescCount += 1;
            first_desc = second_desc;
        }

        gmac_tx_desc_init_chain(first_desc);
        printk("%02d %08x %08x %08x %08x %08x %08x %08x \n", gmacdev->TxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1, first_desc->buffer2, first_desc->data1, first_desc->data2);
        printk("Tx===================================================================Tx\n");
    }

    gmacdev->TxNext = 0;
    gmacdev->TxBusy = 0;
    gmacdev->TxNextDesc = gmacdev->TxDesc;
    gmacdev->TxBusyDesc = gmacdev->TxDesc;
    gmacdev->BusyTxDesc  = 0;

    return -ESYNOPGMACNOERR;
}


/**
  * This sets up the receive Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures in ring mode or chain mode.
  * This function depends on the pcidev structure for allocation of consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to gmac_device.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in rx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->RxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements
  * note:cdh:check ok
  */
static s32 setup_rx_desc_queue(gmac_device *gmacdev, struct net_device *pnetdev, u32 no_of_desc, u32 desc_mode)
{
    s32 i;

    DmaDesc *first_desc = NULL;
    DmaDesc *second_desc = NULL;
    dma_addr_t dma_addr;
    gmacdev->RxDescCount = 0;

    if(desc_mode == RINGMODE)
    {
        //printk("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n", ((sizeof(DmaDesc) * no_of_desc)));
        first_desc = plat_alloc_consistent_mem (pnetdev, sizeof(DmaDesc) * no_of_desc, &dma_addr);
        if(first_desc == NULL)
        {
            printk("Error in Rx Descriptor Memory allocation in Ring mode\n");
            return -ESYNOPGMACNOMEM;
        }
        gmacdev->RxDescCount = no_of_desc;
        gmacdev->RxDesc      = first_desc; // cdh:the first desc virtual addr
        gmacdev->RxDescDma   = dma_addr;   // cdh:the first desc physic addr

        for(i = 0; i < gmacdev->RxDescCount; i++)
        {
            gmac_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount - 1);
            //printk("%02d %08x \n", i, (unsigned int)(gmacdev->RxDesc + i));

        }
    }
    else
    {
        //Allocate the head descriptor
        first_desc = plat_alloc_consistent_mem (pnetdev, sizeof(DmaDesc), &dma_addr);
        if(first_desc == NULL)
        {
            printk("Error in Rx Descriptor Memory allocation in Ring mode\n");
            return -ESYNOPGMACNOMEM;
        }
        gmacdev->RxDesc       = first_desc;
        gmacdev->RxDescDma    = dma_addr;

        printk("Rx===================================================================Rx\n");
        first_desc->buffer2   = gmacdev->RxDescDma;
        first_desc->data2     = (u32) gmacdev->RxDesc;

        gmacdev->RxDescCount = 1;

        for(i = 0; i < (no_of_desc - 1); i++)
        {
            second_desc = plat_alloc_consistent_mem(pnetdev, sizeof(DmaDesc), &dma_addr);
            if(second_desc == NULL)
            {
                printk("Error in Rx Descriptor Memory allocation in Chain mode\n");
                return -ESYNOPGMACNOMEM;
            }
            first_desc->buffer2  = dma_addr;
            first_desc->data2    = (u32)second_desc;

            second_desc->buffer2 = gmacdev->RxDescDma;
            second_desc->data2   = (u32)gmacdev->RxDesc;

            gmac_rx_desc_init_chain(first_desc);
            printk("%02d  %08x %08x %08x %08x %08x %08x %08x \n", gmacdev->RxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1, first_desc->buffer2, first_desc->data1, first_desc->data2);
            gmacdev->RxDescCount += 1;
            first_desc = second_desc;
        }
        gmac_rx_desc_init_chain(first_desc);
        printk("%02d  %08x %08x %08x %08x %08x %08x %08x \n", gmacdev->RxDescCount, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1, first_desc->buffer2, first_desc->data1, first_desc->data2);
        printk("Rx===================================================================Rx\n");

    }

    gmacdev->RxNext = 0;
    gmacdev->RxBusy = 0;
    gmacdev->RxNextDesc = gmacdev->RxDesc;
    gmacdev->RxBusyDesc = gmacdev->RxDesc;

    gmacdev->BusyRxDesc   = 0;

    return -ESYNOPGMACNOERR;
}

/**
  * Driver Api to get the descriptor field information.
  * This returns the status, dma-able address of buffer1, the length of buffer1, virtual address of buffer1
  * dma-able address of buffer2, length of buffer2, virtural adddress of buffer2.
  * @param[in]  pointer to DmaDesc structure.
  * @param[out] pointer to status field fo descriptor.
  * @param[out] dma-able address of buffer1.
  * @param[out] length of buffer1.
  * @param[out] virtual address of buffer1.
  * @param[out] dma-able address of buffer2.
  * @param[out] length of buffer2.
  * @param[out] virtual address of buffer2.
  * \return returns void.
  */
void gmac_get_desc_data(DmaDesc *desc, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 *Length2, u32 *Data2)
{

    if(Status != 0)
        *Status = desc->status;

    if(Buffer1 != 0)
        *Buffer1 = desc->buffer1;
    if(Length1 != 0)
        *Length1 = (desc->length & DescSize1Mask) >> DescSize1Shift;
    if(Data1 != 0)
        *Data1 = desc->data1;

    if(Buffer2 != 0)
        *Buffer2 = desc->buffer2;
    if(Length2 != 0)
        *Length2 = (desc->length & DescSize2Mask) >> DescSize2Shift;
    if(Data2 != 0)
        *Data2 = desc->data2;

    return;

}



/**
  * This gives up the receive Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Once device's Dma is stopped the memory descriptor memory and the buffer memory deallocation,
  * is completely handled by the operating system, this call is kept outside the device driver Api.
  * This function should be treated as an example code to de-allocate the descriptor structures in ring mode or chain mode
  * and network buffer deallocation.
  * This function depends on the pcidev structure for dma-able memory deallocation for both descriptor memory and the
  * network buffer memory under linux.
  * The responsibility of this function is to
  *     - Free the network buffer memory if any.
  *	- Fee the memory allocated for the descriptors.
  * @param[in] pointer to gmac_device.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in rx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note No referece should be made to descriptors once this function is called. This function is invoked when the device is closed.
  * note:cdh:check ok
  */
void giveup_rx_desc_queue(gmac_device *gmacdev, struct net_device *pnetdev, u32 desc_mode)
{
    s32 i;

    DmaDesc *first_desc = NULL;
    dma_addr_t first_desc_dma_addr;
    u32 status;
    dma_addr_t dma_addr1;
    dma_addr_t dma_addr2;
    u32 length1;
    u32 length2;
    u32 data1;
    u32 data2;

    if(desc_mode == RINGMODE)
    {
        for(i = 0; i < gmacdev->RxDescCount; i++)
        {
            gmac_get_desc_data(gmacdev->RxDesc + i, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
            if((length1 != 0) && (data1 != 0))
            {
                dma_unmap_single(NULL, dma_addr1, 0, DMA_FROM_DEVICE);
                dev_kfree_skb((struct sk_buff *) data1);	// free buffer1
            }
            
            if((length2 != 0) && (data2 != 0))
            {
                dma_unmap_single(NULL, dma_addr2, 0, DMA_FROM_DEVICE);
                dev_kfree_skb((struct sk_buff *) data2);	//free buffer2
            }
        }
        
        plat_free_consistent_mem(pnetdev, (sizeof(DmaDesc) * gmacdev->RxDescCount), gmacdev->RxDesc, gmacdev->RxDescDma); //free descriptors memory
        TR("Memory allocated %08x  for Rx Desriptors (ring) is given back\n", (u32)gmacdev->RxDesc);
    }
    else
    {
        TR("rx-------------------------------------------------------------------rx\n");
        first_desc          = gmacdev->RxDesc;
        first_desc_dma_addr = gmacdev->RxDescDma;
        for(i = 0; i < gmacdev->RxDescCount; i++)
        {
            gmac_get_desc_data(first_desc, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
            TR("%02d %08x %08x %08x %08x %08x %08x %08x\n", i, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1, first_desc->buffer2, first_desc->data1, first_desc->data2);
            if((length1 != 0) && (data1 != 0))
            {
                dma_unmap_single(NULL, dma_addr1, 0, DMA_FROM_DEVICE);
                dev_kfree_skb((struct sk_buff *) data1);	// free buffer1
                printk("(Chain mode) rx buffer1 %08x of size %d from %d rx descriptor is given back\n", data1, length1, i);
            }
            plat_free_consistent_mem(pnetdev, (sizeof(DmaDesc)), first_desc, first_desc_dma_addr); //free descriptors
            TR("Memory allocated %08x for Rx Descriptor (chain) at  %d is given back\n", data2, i);

            first_desc = (DmaDesc *)data2;
            first_desc_dma_addr = dma_addr2;
        }

        printk("rx-------------------------------------------------------------------rx\n");
    }
    gmacdev->RxDesc    = NULL;
    gmacdev->RxDescDma = 0;
    
    return;
}

/**
  * This gives up the transmit Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Once device's Dma is stopped the memory descriptor memory and the buffer memory deallocation,
  * is completely handled by the operating system, this call is kept outside the device driver Api.
  * This function should be treated as an example code to de-allocate the descriptor structures in ring mode or chain mode
  * and network buffer deallocation.
  * This function depends on the pcidev structure for dma-able memory deallocation for both descriptor memory and the
  * network buffer memory under linux.
  * The responsibility of this function is to
  *     - Free the network buffer memory if any.
  *	- Fee the memory allocated for the descriptors.
  * @param[in] pointer to gmac_device.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in tx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note No reference should be made to descriptors once this function is called. This function is invoked when the device is closed.
  * note:cdh:check ok
  */
void giveup_tx_desc_queue(gmac_device *gmacdev, struct net_device *pnetdev, u32 desc_mode)
{
    s32 i;

    DmaDesc *first_desc = NULL;
    dma_addr_t first_desc_dma_addr;
    u32 status;
    dma_addr_t dma_addr1;
    dma_addr_t dma_addr2;
    u32 length1;
    u32 length2;
    u32 data1;
    u32 data2;

    if(desc_mode == RINGMODE)
    {
        for(i = 0; i < gmacdev->TxDescCount; i++)
        {
            gmac_get_desc_data(gmacdev->TxDesc + i, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
            if((length1 != 0) && (data1 != 0))
            {
                dma_unmap_single(NULL, dma_addr1, 0, DMA_TO_DEVICE);
                dev_kfree_skb((struct sk_buff *) data1);	// free buffer1
                TR("(Ring mode) tx buffer1 %08x of size %d from %d rx descriptor is given back\n", data1, length1, i);
            }
            if((length2 != 0) && (data2 != 0))
            {
                dma_unmap_single(NULL, dma_addr2, 0, DMA_TO_DEVICE);
                dev_kfree_skb((struct sk_buff *) data2);	//free buffer2
                TR("(Ring mode) tx buffer2 %08x of size %d from %d rx descriptor is given back\n", data2, length2, i);
            }
        }
        plat_free_consistent_mem(pnetdev, (sizeof(DmaDesc) * gmacdev->TxDescCount), gmacdev->TxDesc, gmacdev->TxDescDma); //free descriptors
        TR("Memory allocated %08x for Tx Desriptors (ring) is given back\n", (u32)gmacdev->TxDesc);
    }
    else
    {
        printk("tx-------------------------------------------------------------------tx\n");
        first_desc          = gmacdev->TxDesc;
        first_desc_dma_addr = gmacdev->TxDescDma;
        for(i = 0; i < gmacdev->TxDescCount; i++)
        {
            gmac_get_desc_data(first_desc, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
            TR("%02d %08x %08x %08x %08x %08x %08x %08x\n", i, (u32)first_desc, first_desc->status, first_desc->length, first_desc->buffer1, first_desc->buffer2, first_desc->data1, first_desc->data2);
            if((length1 != 0) && (data1 != 0))
            {
                dma_unmap_single(NULL, dma_addr1, 0, DMA_TO_DEVICE);
                dev_kfree_skb((struct sk_buff *) data2);	// free buffer1
                printk("(Chain mode) tx buffer1 %08x of size %d from %d rx descriptor is given back\n", data1, length1, i);
            }
            plat_free_consistent_mem(pnetdev, (sizeof(DmaDesc)), first_desc, first_desc_dma_addr); //free descriptors
            TR("Memory allocated %08x for Tx Descriptor (chain) at  %d is given back\n", data2, i);

            first_desc = (DmaDesc *)data2;
            first_desc_dma_addr = dma_addr2;
        }
        TR("tx-------------------------------------------------------------------tx\n");

    }
    gmacdev->TxDesc    = NULL;
    gmacdev->TxDescDma = 0;
    
    return;
}

/**
  * This enables the pause frame generation after programming the appropriate registers.
  * presently activation is set at 3k and deactivation set at 4k. These may have to tweaked
  * if found any issues
  * @param[in] pointer to gmac_device.
  * \return void.
  * note:cdh:check ok
  */
static void gmac_pause_control(gmac_device *gmacdev)
{
    u32 omr_reg;
    u32 mac_flow_control_reg;
    omr_reg = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaControl);
    omr_reg |= DmaRxFlowCtrlAct4K | DmaRxFlowCtrlDeact5K | DmaEnHwFlowCtrl;
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaControl, omr_reg);

    mac_flow_control_reg = gmac_read_reg((u32 *)gmacdev->MacBase, GmacFlowControl);
    mac_flow_control_reg |= GmacRxFlowControl | GmacTxFlowControl | 0xFFFF0000;
    gmac_write_reg((u32 *)gmacdev->MacBase, GmacFlowControl, mac_flow_control_reg);

    return;

}


/**
  * Disable the ip checksum offloading in receive path.
  * Ip checksum offloading is disabled in the receive path.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_disable_rx_Ipchecksum_offload(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacConfig, GmacRxIpcOffload);
}


/**
  * Instruct the DMA not to drop the packets even if it fails tcp ip checksum.
  * This is to instruct the receive DMA engine to allow the packets even if recevied packet
  * fails the tcp/ip checksum in hardware. Valid only when full checksum offloading is enabled(type-2).
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_rx_tcpip_chksum_drop_disable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->DmaBase, DmaControl, DmaDisableDropTcpCs);
    return;
}

/**
  * The check summ offload engine is enabled to do only IPV4 header checksum.
  * IPV4 header Checksum is computed in the Hardware.
  * @param[in] pointer to gmac_device.
  * @param[in] Pointer to tx descriptor for which  ointer to gmac_device.
  * \return returns void.
  */
void gmac_tx_checksum_offload_ipv4hdr(gmac_device *gmacdev, DmaDesc *desc)
{
#ifdef ENH_DESC
    desc->status = ((desc->status & (~DescTxCisMask)) | DescTxCisIpv4HdrCs);//ENH_DESC
#else
    desc->length = ((desc->length & (~DescTxCisMask)) | DescTxCisIpv4HdrCs);
#endif

}

/**
  * The check summ offload engine is enabled to do TCPIP checsum assuming Pseudo header is available.
  * Hardware computes the tcp ip checksum assuming pseudo header checksum is computed in software.
  * Ipv4 header checksum is also inserted.
  * @param[in] pointer to gmac_device.
  * @param[in] Pointer to tx descriptor for which  ointer to gmac_device.
  * \return returns void.
  */
void gmac_tx_checksum_offload_tcponly(gmac_device *gmacdev, DmaDesc *desc)
{
#ifdef ENH_DESC
    desc->status = ((desc->status & (~DescTxCisMask)) | DescTxCisTcpOnlyCs);//ENH_DESC
#else
    desc->length = ((desc->length & (~DescTxCisMask)) | DescTxCisTcpOnlyCs);
#endif

}

/**
  * Populate the tx desc structure with the buffer address.
  * Once the driver has a packet ready to be transmitted, this function is called with the
  * valid dma-able buffer addresses and their lengths. This function populates the descriptor
  * and make the DMA the owner for the descriptor. This function also controls whetther Checksum
  * offloading to be done in hardware or not.
  * This api is same for both ring mode and chain mode.
  * @param[in] pointer to gmac_device.
  * @param[in] Dma-able buffer1 pointer.
  * @param[in] length of buffer1 (Max is 2048).
  * @param[in] virtual pointer for buffer1.
  * @param[in] Dma-able buffer2 pointer.
  * @param[in] length of buffer2 (Max is 2048).
  * @param[in] virtual pointer for buffer2.
  * @param[in] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * @param[in] u32 indicating whether the checksum offloading in HW/SW.
  * \return returns present tx descriptor index on success. Negative value if error.
  * note:cdh:check ok
  */
static s32 gmac_set_tx_qptr(gmac_device *gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2, u32 offload_needed)
{
    u32 txnext = gmacdev->TxNext;
    DmaDesc *txdesc = gmacdev->TxNextDesc;
    
    if(!gmac_is_desc_empty(txdesc))
        return -1;

    (gmacdev->BusyTxDesc)++; //busy tx descriptor is incremented by one as it will be handed over to DMA

    if(gmac_is_tx_desc_chained(txdesc))
    {
        txdesc->length |= ((Length1 << DescSize1Shift) & DescSize1Mask);
        txdesc->length |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //Its always assumed that complete data will fit in to one descriptor
        txdesc->buffer1 = Buffer1;
        txdesc->data1 = Data1;

        if(offload_needed)
        {
            /**
            		 *Make sure that the OS you are running supports the IP and TCP checkusm offloaidng,
            		 *before calling any of the functions given below.
             		*/
            gmac_tx_checksum_offload_ipv4hdr(gmacdev, txdesc);
            gmac_tx_checksum_offload_tcponly(gmacdev, txdesc);
            //gmac_tx_checksum_offload_tcp_pseudo(gmacdev, txdesc);
        }

        txdesc->status = DescOwnByDma;

        gmacdev->TxNext = gmac_is_last_tx_desc(gmacdev, txdesc) ? 0 : txnext + 1;
        gmacdev->TxNextDesc = (DmaDesc *)txdesc->data2;
    }
    else
    {
        txdesc->length |= (((Length1 << DescSize1Shift) & DescSize1Mask) | ((Length2 << DescSize2Shift) & DescSize2Mask));
        txdesc->length |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //Its always assumed that complete data will fit in to one descriptor
        txdesc->buffer1 = Buffer1;
        txdesc->data1 = Data1;

        txdesc->buffer2 = Buffer2;
        txdesc->data2 = Data2;

        if(offload_needed)
        {
            /**
             		*cdh:Make sure that the OS you are running supports the IP and TCP checkusm offloaidng,
             		*cdh:before calling any of the functions given below.
             		*/
            gmac_tx_checksum_offload_ipv4hdr(gmacdev, txdesc);
            gmac_tx_checksum_offload_tcponly(gmacdev, txdesc);
            //gmac_tx_checksum_offload_tcp_pseudo(gmacdev, txdesc);
        }

        txdesc->status = DescOwnByDma;

        gmacdev->TxNext = gmac_is_last_tx_desc(gmacdev, txdesc) ? 0 : txnext + 1;
        gmacdev->TxNextDesc = gmac_is_last_tx_desc(gmacdev, txdesc) ? gmacdev->TxDesc : (txdesc + 1);
    }
    
    //printk("(set tx)%02d %08x %08x %08x %08x %08x %08x %08x\n", txnext, (u32)txdesc, txdesc->status, txdesc->length, txdesc->buffer1, txdesc->buffer2, txdesc->data1, txdesc->data2);

    return txnext;
}

/**
  * Checks and initialze phy.
  * This function checks whether the phy initialization is complete.
  * @param[in] pointer to gmac_device.
  * \return 0 if success else returns the error number.
  */
static s32 gmac_check_phy_init(gmac_device *gmacdev)
{
    u16 data;
    u16 data2;
    s32 status = -ESYNOPGMACNOERR;
    s32 loop_count;
	u16 tmp_speed = 0;
    u16 data_id1;
    u16 data_id2;
    
	/** cdh:instruction
	*Auto-Negotiation
	*The KSZ8041NL conforms to the auto-negotiation protocol, defined in Clause 28 of the IEEE 802.3u specification. Autonegotiation
	*is enabled by either hardware pin strapping (pin 30) or software (register 0h bit 12).
	*/
    loop_count = 10; //cdh:DEFAULT_LOOP_VARIABLE;
    while(loop_count-- > 0)
    {
        status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_STATUS_REG, &data);
        if(status)
            return status;

        if((data & Mii_AutoNegCmplt) != 0){
            printk("Autonegotiation Complete\n");  // cdh:check ok
            break;
        }else{
	        ; //TR("waiting AN finish ...\n");
        }
    }
    status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_ID_HI_REG, &data_id1);
	if(status) { 
		printk("phy read phy id1 waiting Error,data=0x%x\n", data_id1);  
	}else {
		printk("phy read phy id1 waiting ok,data=0x%x\n", data_id1);  
	}

	status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_ID_LOW_REG, &data_id2);
	if(status) { 
		printk("phy read phy id2 waiting Error,data=0x%x\n", data_id2);  
	}else {
		printk("phy read phy id2 waiting ok,data=0x%x\n", data_id2);  
	}
    
    if(data_id1 == 0x0 && data_id2 == 0x128) {
        status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_SPECIFIC_STATUS_REG, &data);
        if(status) {
            return status;
        }
        if((data & Mii_phy_status_link_up) == 0) // cdh:Mii_phy_status_link_up
        {
            printk("No Link\n");
            gmacdev->LinkState = LINKDOWN;
            return -ESYNOPGMACPHYERR;
        }
        else
        {
            gmacdev->LinkState = LINKUP;
            printk("Link UP\n");
            mdelay(500);
        }
    }else{
        // cdh:note may be for same special phy define register, not same 8201F phy,cdh: PHY_SPECIFIC_STATUS_REG
        status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_STATUS_REG, &data);
        if(status) {
            return status;
        }

        if((data & Mii_Link) == 0) // cdh:Mii_phy_status_link_up
        {
            printk("No Link\n");
            gmacdev->LinkState = LINKDOWN;
            return -ESYNOPGMACPHYERR;
        }
        else
        {
            gmacdev->LinkState = LINKUP;
            printk("Link UP\n");
        }
    }
    
    // cdh:get other end partner AN ability from Reg6.0
    status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_AN_EXP_REG, &data);
	if(status) {
        return status;
    }
    
	if((data&OTHER_END_PARTNER_AN_ABITY) == OTHER_END_PARTNER_AN_ABITY) {
    	//printk("Local&Partner Support AN Ability\n");

		// cdh:read phy Reg4[8:5]&Reg5[8:5] for speed
    	status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_AN_ADV_REG, &data);
	    if(status) {
	        return status;
	    }

	    status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_LNK_PART_ABl_REG, &data2);
	    if(status) {
	        return status;
	    }

	    tmp_speed = ((data>>5)&(data2>>5))&0xF; // cdh:get Reg4[8:5] for loacl AN speed

	    if(((tmp_speed&SPEED_100M_FULL) == SPEED_100M_FULL) || ((tmp_speed&SPEED_100M_HALF) == SPEED_100M_HALF)){
	    	// cdh:set chip system ctrl reg base address
			psysbase = AK_VA_SYSCTRL; 
			REG32(psysbase + 0x14) |=  (0x1 << 23); // cdh:first  mac_speed_cfg=1(100m)
			
			gmacdev->Speed = SPEED100;
			if(tmp_speed&SPEED_100M_FULL){
        		gmacdev->DuplexMode = FULLDUPLEX;
        		printk("LPAN Link is with 100M Speed, FULLDUPLEX mode\n");
        	}else{
				gmacdev->DuplexMode = HALFDUPLEX;
				printk("LPAN Link is with 100M Speed, HALFDUPLEX mode\n");
        	}
	    }else {
	    	// cdh:set chip system ctrl reg base address
			psysbase = AK_VA_SYSCTRL; 
			REG32(psysbase + 0x14) &=  ~(0x1 << 23); // cdh:first  mac_speed_cfg=0(10m)
			
			gmacdev->Speed = SPEED10;
			if(tmp_speed&SPEED_10M_FULL){
        		gmacdev->DuplexMode = FULLDUPLEX;
        		printk("LPAN Link is with 10M Speed, FULLDUPLEX mode\n");
        	}else{
				gmacdev->DuplexMode = HALFDUPLEX;
				printk("LPAN Link is with 10M Speed, HALFDUPLEX mode\n");
        	}
	    }
    	
	}else{
		//printk("Only Local Support AN Ability\n");

		// cdh:phy Reg0.12 == 0 ? , AN disable
		status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, &data);
	    if(status) {
	        return status;
	    }

	    if ((data&Mii_AN_En) != Mii_AN_En){
			if(data&(0x1<<13)){
				// cdh:set chip system ctrl reg base address
				psysbase = AK_VA_SYSCTRL; 
				REG32(psysbase + 0x14) |=  (0x1 << 23); // cdh:first  mac_speed_cfg=1(100m)
				
				gmacdev->Speed = SPEED100;
				if(data&(0x1<<8)){
					gmacdev->DuplexMode = FULLDUPLEX;
	        		printk("NOAN Link is with 100M Speed, FULLDUPLEX mode\n");
				}else{
					gmacdev->DuplexMode = HALFDUPLEX;
					printk("NOAN Link is with 100M Speed, HALFDUPLEX mode\n");
				}
			}else{
				// cdh:set chip system ctrl reg base address
				psysbase = AK_VA_SYSCTRL; 
				REG32(psysbase + 0x14) &=  ~(0x1 << 23); // cdh:first  mac_speed_cfg=0(10m)
				
				gmacdev->Speed = SPEED10;
				if(data&(0x1<<8)){
	        		gmacdev->DuplexMode = FULLDUPLEX;
	        		printk("NOAN Link is with 10M Speed, FULLDUPLEX mode\n");
	        	}else{
					gmacdev->DuplexMode = HALFDUPLEX;
					printk("NOAN Link is with 10M Speed, HALFDUPLEX mode\n");
	        	}
			}
	    }else{
			
			// cdh:read phy Reg5[8,6] for speed
			status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_LNK_PART_ABl_REG, &data);
			if(status) {
				return status;
			}

			if((data&NAN_SPEED_100M_HALF) == NAN_SPEED_100M_HALF){
				// cdh:set chip system ctrl reg base address
				psysbase = AK_VA_SYSCTRL; 
				REG32(psysbase + 0x14) |=  (0x1 << 23); // cdh:first  mac_speed_cfg=1(100m)
				
				gmacdev->Speed = SPEED100;
				gmacdev->DuplexMode = HALFDUPLEX;
				printk("LAN Link is with 100M Speed, HALFDUPLEX mode\n");
			}else{
				// cdh:set chip system ctrl reg base address
				psysbase = AK_VA_SYSCTRL; 
				REG32(psysbase + 0x14) &=  ~(0x1 << 23); // cdh:first  mac_speed_cfg=0(10m)
				
				gmacdev->Speed = SPEED10;
				gmacdev->DuplexMode = HALFDUPLEX;
				printk("LAN Link is with 10M Speed, HALFDUPLEX mode\n");
			}
	    }
	}
	
    return -ESYNOPGMACNOERR;
}

static s32 phy_wait_idle(gmac_device *gmacdev)
{
	u32 i;
	s32 status;
	u16 data;
	
	// cdh:wait phy idle, reset cycle < 0.5s 
	for(i=0; i<200; i++){
		status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, &data);
		if(status) { 
			continue; 
		}

		// cdh:Mii_reset=1(soft_reset), or=0(normal), self-clearing
		if((data & Mii_reset) == 0){ 
			printk("phy reset OK\n"); 
			break;
		}
	}
	
	if(i>=200){
		printk("timeout in waiting phy to idle!\n");
	}
	printk("phy is idle!\n");

	return status;
}

static void gmac_phy_gpio_reset(struct ak_mac_data *pdata)
{
	printk("gpio phy reset pin:%d, value:%d\n", pdata->phy_rst_gpio.pin, pdata->phy_rst_gpio.value);
	ak_setpin_as_gpio(pdata->phy_rst_gpio.pin);
	ak_gpio_cfgpin(pdata->phy_rst_gpio.pin, AK_GPIO_DIR_OUTPUT);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, !pdata->phy_rst_gpio.value);
	#if 1
	mdelay(5);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, pdata->phy_rst_gpio.value);
	mdelay(200);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, !pdata->phy_rst_gpio.value);
	mdelay(5);
	#endif
	#if 0
	mdelay(5);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, pdata->phy_rst_gpio.value);
	mdelay(25);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, !pdata->phy_rst_gpio.value);
	mdelay(5);
	#endif
}


static s32 gmac_phy_init(gmac_device *gmacdev, struct ak_mac_data *pdata_phy)
{
	s32 status;
	u16 data_ctl;
	u16 data_id1;
	u16 data_id2;
	
	/* reset */
	gmac_phy_gpio_reset(pdata_phy);
	
	// cdh:wait phy idle
	if(phy_wait_idle(gmacdev)) {
		printk("cdh:%s,phy_wait_idle err!\n", __func__);
	}else {
		printk("cdh:%s,phy_wait_idle ok!\n", __func__);
	}

	mdelay(200);
	status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, &data_ctl);
	if(status) { 
		printk("phy read phy ctrl waiting Error\n");  
	}
	printk("BMCR:0x%x\n", data_ctl);

	status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_ID_HI_REG, &data_id1);
	if(status) { 
		printk("phy read phy id1 waiting Error,data=0x%x\n", data_id1);  
	}else {
		printk("phy read phy id1 waiting ok,data=0x%x\n", data_id1);  
	}

	status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_ID_LOW_REG, &data_id2);
	if(status) { 
		printk("phy read phy id2 waiting Error,data=0x%x\n", data_id2);  
	}else {
		printk("phy read phy id2 waiting ok,data=0x%x\n", data_id2);  
	}
	

	if(data_id1 == 0x22) {
		printk("cdh:id 0x22 set!\n");
		gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, &data_ctl);
		data_ctl |= 0x1000; // cdh:enable auto-negotiation process
		gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, data_ctl);

		// cdh:reg_0x1b, bit[8](enable link up interrupt)=1 , bit[10](enable link down interrupt)=1
		//gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_EXT_PHY_SPC_STATUS, 0x0500);
		//gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, 0x1F, 0x8300);
	}
	else {
		printk("cdh:id other set!\n");
		gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, &data_ctl);
		data_ctl |= 0x1100; // cdh:bit12 enable auto-negotiation process, set full-duplex mode
		gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_CONTROL_REG, data_ctl);
#if 1
		printk("###anyka phy set write!\n");
		data_ctl = 0x01;
		gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_EXT_PHY_SPC_CTRL, data_ctl);
		gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, 23, &data_ctl);
		
		printk("read page 1 reg 23 : 0x%x\n", data_ctl);
		//set 13bit = 0 normal mode
		data_ctl &= 0xdfff;

		//set 13bit = 1 loopback mode
		//data_ctl |= (1<<13);
		gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, 23, data_ctl);
#endif

	}

    if(data_id1 == 0x0 && data_id2 == 0x128) {
        printk("cdh:id 0x128 set!\n");

        gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x0000,0x9140);
        gmac_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x0000, &data_ctl);
        printk("read reg 0x00 : 0x%x\n",data_ctl);
        
        gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x001e,0x50);
        gmac_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x001f, &data_ctl);
        printk("read reg 0x50 : 0x%x\n",data_ctl);
        data_ctl |= 0x0040;
        gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x001e, 0x50);
        gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x001f, data_ctl);
        printk("write reg 0x50 : 0x%x\n",data_ctl);
        gmac_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase, 0x001e,0x00);

    }
    
	return 0;

}



/**
 * Function used to detect the cable plugging and unplugging.
 * This function gets scheduled once in every second and polls
 * the PHY register for network cable plug/unplug. Once the
 * connection is back the GMAC device is configured as per
 * new Duplex mode and Speed of the connection.
 * @param[in] u32 type but is not used currently.
 * \return returns void.
 * \note This function is tightly coupled with Linux 2.6.xx.
 * \callgraph
 */

static void timer_cable_unplug_proc(u32 notused)
{
    s32 status;
    u16 data;
    nt_adapter *adapter = (nt_adapter *)timer_cable_unplug.data;
    gmac_device            *gmacdev = adapter->gmacdev_pt;

    init_timer(&timer_cable_unplug);
    timer_cable_unplug.expires = CHECK_TIME + jiffies;
    status = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, PHY_STATUS_REG, &data);
	if (status) {
		printk("cdh:%s, read phy err,status:0x%x\n", __func__, status);
		return;
	}
	
    if((data & Mii_Link) == 0)
    {
        //printk("No Link: %08x\n", data);
        gmacdev->LinkState = 0;
        gmacdev->DuplexMode = 0;
        gmacdev->Speed = 0;
        gmacdev->LoopBackMode = 0;
        add_timer(&timer_cable_unplug);
        netif_carrier_off(adapter->netdev_pt); // cdh:add date 2015.8.29
    }
    else
    {
        //printk("Link UP: %08x\n", data);
        if(!gmacdev->LinkState)
        {
            // cdh:status = gmac_phy_init(gmacdev);
            gmac_check_phy_init(gmacdev);
            gmac_mac_init(gmacdev);
            // cdh:gmac_promisc_enable(gmacdev);
        }
        add_timer(&timer_cable_unplug);
        netif_carrier_on(adapter->netdev_pt); // cdh:add date 2015.8.29
    }

}

/**
  * Clears all the pending interrupts.
  * If the Dma status register is read then all the interrupts gets cleared
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_clear_interrupt(gmac_device *gmacdev)
{
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->DmaBase, DmaStatus);
    gmac_write_reg((u32 *)gmacdev->DmaBase, DmaStatus , data);
}

/**
  * Disable the MMC Tx interrupt.
  * The MMC tx interrupts are masked out as per the mask specified.
  * @param[in] pointer to gmac_device.
  * @param[in] tx interrupt bit mask for which interrupts needs to be disabled.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_disable_mmc_tx_interrupt(gmac_device *gmacdev, u32 mask)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacMmcIntrMaskTx, mask);
    return;
}

/**
  * Enable the MMC Tx interrupt.
  * The MMC tx interrupts are enabled as per the mask specified.
  * @param[in] pointer to gmac_device.
  * @param[in] tx interrupt bit mask for which interrupts needs to be enabled.
  * \return returns void.
  */
void gmac_enable_mmc_tx_interrupt(gmac_device *gmacdev, u32 mask)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacMmcIntrMaskTx, mask);
}
/**
  * Disable the MMC Rx interrupt.
  * The MMC rx interrupts are masked out as per the mask specified.
  * @param[in] pointer to gmac_device.
  * @param[in] rx interrupt bit mask for which interrupts needs to be disabled.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_disable_mmc_rx_interrupt(gmac_device *gmacdev, u32 mask)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacMmcIntrMaskRx, mask);
    return;
}
/**
  * Enable the MMC Rx interrupt.
  * The MMC rx interrupts are enabled as per the mask specified.
  * @param[in] pointer to gmac_device.
  * @param[in] rx interrupt bit mask for which interrupts needs to be enabled.
  * \return returns void.
  */
void gmac_enable_mmc_rx_interrupt(gmac_device *gmacdev, u32 mask)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacMmcIntrMaskRx, mask);
    return;
}

/**
  * Disable the MMC ipc rx checksum offload interrupt.
  * The MMC ipc rx checksum offload interrupts are masked out as per the mask specified.
  * @param[in] pointer to gmac_device.
  * @param[in] rx interrupt bit mask for which interrupts needs to be disabled.
  * \return returns void.
  * note:cdh:check ok
  */
static void gmac_disable_mmc_ipc_rx_interrupt(gmac_device *gmacdev, u32 mask)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacMmcRxIpcIntrMask, mask);
    return;
}
/**
  * Enable the MMC ipc rx checksum offload interrupt.
  * The MMC ipc rx checksum offload interrupts are enabled as per the mask specified.
  * @param[in] pointer to gmac_device.
  * @param[in] rx interrupt bit mask for which interrupts needs to be enabled.
  * \return returns void.
  */
void gmac_enable_mmc_ipc_rx_interrupt(gmac_device *gmacdev, u32 mask)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacMmcRxIpcIntrMask, mask);
    return;
}


/*
*cdh:init share pin , mac controller clock and 25M_phy clock
*/
static void gmac_ctrl_init(struct ak_mac_data *pdata)
{
	printk("Reset MAC controller!\n");
	ak_soft_reset(AK39_SRESET_MAC);

	printk("Reset MAC phy!\n");
	printk("phy reset pin:%d, value:%d\n", pdata->phy_rst_gpio.pin, pdata->phy_rst_gpio.value);
	ak_setpin_as_gpio(pdata->phy_rst_gpio.pin);
    ak_gpio_cfgpin(pdata->phy_rst_gpio.pin, AK_GPIO_DIR_OUTPUT);
    ak_gpio_setpin(pdata->phy_rst_gpio.pin, !pdata->phy_rst_gpio.value);
	mdelay(5);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, pdata->phy_rst_gpio.value);
	mdelay(25);
	ak_gpio_setpin(pdata->phy_rst_gpio.pin, !pdata->phy_rst_gpio.value);
	mdelay(5);
}


/**
  * Function to set the MDC clock for mdio transactiona
  *
  * @param[in] pointer to device structure.
  * @param[in] clk divider value.
  * \return Reuturns 0 on success else return the error value.
  * note:cdh:check ok
  */
s32 gmac_set_mdc_clk_div(gmac_device *gmacdev, u32 clk_div_val)
{
    u32 orig_data;

    // cdh:set MDO_CLK for MDIO transmit, note GmacGmiiAddr bit5, and 802.3 limit 2.5MHz
    orig_data = gmac_read_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr); //set the mdc clock to the user defined value
    orig_data &= (~ GmiiCsrClkMask); // cdh:csr clk bit[5:2], must 0x3C
    orig_data |= (clk_div_val | GmiiWrite | GmiiBusy);
    gmac_write_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr , orig_data);
    return 0;
}

/**
  * Returns the current MDC divider value programmed in the ip.
  *
  * @param[in] pointer to device structure.
  * @param[in] clk divider value.
  * \return Returns the MDC divider value read.
  */
u32 synopGMAC_get_mdc_clk_div(gmac_device *gmacdev)
{
    u32 data;
    data = gmac_read_reg((u32 *)gmacdev->MacBase, GmacGmiiAddr);
    data &= GmiiCsrClkMask;
    return data;
}



static void mac_phy_reset(struct ak_mac_data *pdata)
{
	/* cdh:check ok, first set phy level low */
	pdata->gpio_init(&pdata->phy_rst_gpio);
	mdelay(10);
	
	/* cdh:check ok, second set gpio as input and be in powersave */
	ak_gpio_cfgpin(pdata->phy_rst_gpio.pin, !pdata->phy_rst_gpio.dir);
	mdelay(1);
}

static int gmac_sharepin_clock_init(void)
{
	int i = 0;
	
	// cdh:setep1, ========set all mii interface share pin,===============
	REG32(psysbase + 0x7C) &= ~(0xf << 0);
	REG32(psysbase + 0x7C) |= (0x1 << 0);  // cdh:gpio10,bit[1:0]=01, set gmii_mdc
	REG32(psysbase + 0x7C) |= (0x1 << 2);  // cdh:gpio11,bit[3:2]=01, set gmii_mdio
	REG32(psysbase + 0x7C) &= ~(0x1 << 4);  // cdh:gpio12,
	REG32(AK_VA_GPIO + 0x00) &= ~(0x1 << 12);  // cdh:gpio12, input

	REG32(psysbase + 0x7C) &= ~(0x3 << 5);
	REG32(psysbase + 0x7C) |= (0x1 << 5);  // cdh:gpio13,bit[6:5]=01, set gmii_txen
	REG32(psysbase + 0x7C) |= (0x1 << 7);  // cdh:gpio76,bit[7]=1, set gmii_txclk

	REG32(psysbase + 0x7C) &= ~(0xf << 8);
	REG32(psysbase + 0x7C) |= (0x1 << 8);  // cdh:gpio14,bit[9:8]=01, set gmii_txd0
	REG32(psysbase + 0x7C) |= (0x1 << 10); // cdh:gpio15,bit[11:10]=01, set gmii_txd1
	REG32(psysbase + 0x7C) |= (0x1 << 12); // cdh:gpio16,bit[12]=1,set gmii_txd2
	REG32(psysbase + 0x7C) |= (0x1 << 13); // cdh:gpio17,bit[13]=1,set gmii_txd3
	REG32(psysbase + 0x7C) |= (0x1 << 14); // cdh:gpio18,bit[14]=1,set gmii_crs
	REG32(psysbase + 0x7C) |= (0x1 << 15); // cdh:gpio77,bit[15]=1,set gmii_rxclk

	REG32(psysbase + 0x7C) &= ~(0xf << 16);
	REG32(psysbase + 0x7C) |= (0x1 << 16); // cdh:gpio19,bit[17:16]=01, set gmii_rxd0
	REG32(psysbase + 0x7C) |= (0x1 << 18); // cdh:gpio20,bit[19:18]=01, set gmii_rxd1
	REG32(psysbase + 0x7C) |= (0x1 << 20); // cdh:gpio21,bit[20]=1, set gmii_rxd2
	REG32(psysbase + 0x7C) |= (0x1 << 21); // cdh:gpio22,bit[21]=1, set gmii_rxd3

	REG32(psysbase + 0x7C) &= ~(0xf << 22);
	REG32(psysbase + 0x7C) |= (0x1 << 22); // cdh:gpio23,bit[23:22]=01, set gmii_rxer
	REG32(psysbase + 0x7C) |= (0x1 << 24); // cdh:gpio24,bit[25:24]=01, set gmii_rxdv
	REG32(psysbase + 0x7C) |= (0x1 << 26); // cdh:gpio78,bit[26]=1, set gmii_col
	
	// cdh:step2,===h2 check ok, to enable the 25MHz oscillator=========
	REG32(psysbase + 0x74) &= ~(3 << 9); // cdh:h2 bit [10:9]=01, set sharepin gpio47 share as opclk, old (3 << 2)
	REG32(psysbase + 0x74) |= (1 << 9);  // cdh:old (1 << 2)
	REG32(psysbase + 0x80) |= (1 << 8);  // cdh:disable gpio47 pull down, old (1 << 2)

	// cdh:step3,===first set Host mac interface select mii, mac_speed_cfg=1(100m)====
	REG32(psysbase + 0x14) |=  ((0x1 << 22)|(0x1 << 23)); 
	REG32(psysbase + 0x14) &= ~(0x1 << 15); // cdh:clear bit[15], prohibit 25m crystal
	REG32(psysbase + 0x14) |=  ((0x1 << 16)|(0x1 << 18)); // cdh:set   bit[16],enable div24, generate 25m,	bit[18], select 25m clock of mac from pll div
	REG32(psysbase + 0x1c) &= ~(1 << 13);		 // cdh:mac clk ctrl

	// cdh:bit[20]:select mac 25M clock from 25M crystal or pll div,here do what's mean? ,pg said at least repeat twice
	for(i=0;i<6;i++)
	{
		REG32(psysbase + 0x14) |= (1 << 20);	// cdh:select 25m crystal pad, what's mean?
		REG32(psysbase + 0x14) &= ~(1 << 20);	// cdh:select 25m clock input, what's mean?
	}

	return 0;
}

static int mac_init_hw(struct ak_mac_data *pdata)
{
#if 0
	ak_group_config(ePIN_AS_MAC);
#else
	gmac_sharepin_clock_init();
#endif

	if (pdata != NULL) {
		/* init mac power on */
		if (pdata->pwr_gpio.pin > 0) 
			pdata->gpio_init(&pdata->pwr_gpio);

		/* init the gpio for phy */
		if (pdata->phy_rst_gpio.pin > 0) 
			mac_phy_reset(pdata);
	}
	
	return 0;
}

/* init the mac and the phy */
static s32 init_hw(struct net_device *ndev)
{
	signed int 		ijk;
	signed int 		status = 0;
	unsigned int 	dma_addr;
	u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
    struct sk_buff *skb; // cdh:#include <linux/skbuff.h>, check ok
	nt_adapter *adapter;
    mac_info_t *db;
    gmac_device *gmacdev;
    struct ak_mac_data *pdata;
    
    // TR("%s called \n", __FUNCTION__);
    adapter = (nt_adapter *)netdev_priv(ndev);
    db = adapter->db_pt;
    gmacdev = (gmac_device *)adapter->gmacdev_pt;
    pdata   = adapter->db_pt->dev->platform_data;

	// initial mac hw support
	mac_init_hw(db->dev->platform_data);
	
	/*cdh:Now platform dependent initialization.cdh:check ok*/
	gmac_mmc_counters_reset(gmacdev); // cdh:Mac Management Counters (MMC), cdh:check ok
	gmac_mmc_counters_disable_rollover(gmacdev); //cdh:check ok
	
	/*Lets reset the IP*/
	printk("adapter= %08x gmacdev = %08x netdev = %08x netdev= %08x\n", (u32)adapter, (u32)gmacdev, (u32)ndev, (u32)ndev);

	// cdh:software reset , the resets all of the GMAC internal registers and logic, cdh:check ok
	gmac_reset(gmacdev);

	/*	cdh:Program/flash in the station/IP's Mac address */
	gmac_set_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, mac_addr0);
	
	/* cdh:Lets read the version of ip, cdh:check ok */
	gmac_read_version(gmacdev);

	/* cdh:Lets set ipaddress in to device structure, cdh:check ok*/
	gmac_get_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, ndev->dev_addr);

	/* cdh:check ok,Now set the broadcast address*/
	for(ijk = 0; ijk < 6; ijk++)
	{
		ndev->broadcast[ijk] = 0xff;
	}
	
	for(ijk = 0; ijk < 6; ijk++)
	{
		printk("netdev->dev_addr[%d] = %02x and netdev->broadcast[%d] = %02x\n", ijk, ndev->dev_addr[ijk], ijk, ndev->broadcast[ijk]);
	}
	
	/*Check for Phy initialization*/
	// cdh:set MDO_CLK for MDIO transmit, note GmacGmiiAddr bit5, and 802.3 limit 2.5MHz
	gmac_set_mdc_clk_div(gmacdev, GmiiCsrClk0);

	// cdh:get MDO_CLK div
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

	// cdh:initial phy and check phy link status,must add check
	status = gmac_phy_init(gmacdev, pdata);//gmac_check_phy_init(gmacdev);// R by panqihe
	if (status) {
		printk("cdh:%s,gmac_phy_init err!\n", __func__);
		//return -EAGAIN;
	}

	/* cdh:check ok ok,Set up the 32 unit, tx descriptor queue/ring*/
	setup_tx_desc_queue(gmacdev, ndev, TRANSMIT_DESC_SIZE, RINGMODE);
	
	// cdh:check ok ok, setup_tx_desc_queue(gmacdev, pcidev, TRANSMIT_DESC_SIZE, CHAINMODE);
	gmac_init_tx_desc_base(gmacdev); // cdh:Program the transmit descriptor base address in to DmaTxBase addr

	/* cdh:check ok ok,Set up the 32 unit, rx descriptor queue/ring*/
	setup_rx_desc_queue(gmacdev, ndev, RECEIVE_DESC_SIZE, RINGMODE);
	
	// cdh:check ok ok,setup_rx_desc_queue(gmacdev, pcidev, RECEIVE_DESC_SIZE, CHAINMODE);
	gmac_init_rx_desc_base(gmacdev); // cdh:Program the transmit descriptor base address in to DmaTxBase addr
	
	// cdh:dma busrt=32words=128B, two dma_descriptor interval = 2Bytes 
	gmac_set_dma_bus_mode(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2); //pbl32 incr with rxthreshold 128

	// cdh:set dma transmit method
	gmac_set_dma_control(gmacdev, DmaDisableFlush | DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);

	/*Initialize the mac interface*/
	// cdh:check ok, initial mac part ip
	gmac_mac_init(gmacdev);

	
	/**
	 *cdh:inital dma and mac flow control
	*/
	gmac_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

	/**
	 *cdh:initial one rx buffer dma infor, why loop 32 ci
	*/
	do {
		skb = alloc_skb(ndev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_KERNEL);
		if(skb == NULL)
		{
			printk("ERROR in skb buffer allocation\n");
			break;
		}
	
		/** cdh:note as follow
		* 函数pci_map_single映射单个用于传送的缓冲区，返回值是可以传递给设备的总线地址，如果出错的话就为 NULL。
		* 一旦传送完成，应该使用函数pci_unmap_single 删除映射。其中，参数direction为传输的方向，取值如下：
		* PCI_DMA_TODEVICE 数据被发送到设备。
		* PCI_DMA_FROMDEVICE如果数据将发送到 CPU。
		* PCI_DMA_BIDIRECTIONAL数据进行两个方向的移动。
		* PCI_DMA_NONE 这个符号只是为帮助调试而提供。
		*/
		// cdh:malloc buffer to descriptor dma buffer, dma_addr as physic address
		dma_addr = dma_map_single(NULL, skb->data, skb_tailroom(skb), DMA_FROM_DEVICE);
//		  printk("cdh:skb=0x%x, dma_addr=0x%x\n", (u32 *)skb, dma_addr);
		status = gmac_set_rx_qptr(gmacdev, dma_addr, skb_tailroom(skb), (u32)skb, 0, 0, 0);
		if(status < 0)
			dev_kfree_skb(skb);
		else
			; // cdh:printk("skb, head, data, tail: %x, %x, %x, %x\n", skb->head, (u32 *)skb->data, (u32 *)skb->tail, (u32 *)skb->len);
	}while(status >= 0);

	/**
	 *cdh:Clear all the interrupts
	*/
	gmac_clear_interrupt(gmacdev);

	/**
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
	  */
	gmac_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF); // cdh:set 1for mask interrupt
	gmac_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	gmac_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

	/**
	 *cdh:Enable Tx and Rx DMA
	*/
	gmac_enable_interrupt(gmacdev, DmaIntEnable);
	gmac_enable_dma_rx(gmacdev);
	gmac_enable_dma_tx(gmacdev);

	return status;
}



/** * @brief Initialize Mac 
* Initialize MAC and PHY 
* @author Tang Anyang
* @date 2010-11-16 
* @param unsigned char * MacAddress: 
*/
static bool MacInit(struct net_device *ndev)
{
	volatile unsigned long count;
	s32 status = 0;

	// cdh:reset MAC ctrl module
	count = 10;
	while(count--);
	REG32(psysbase + 0x20) |= (1 << 13);

	// cdh:without reset MAC ctrl module
	count = 10;
	while(count--);
	REG32(psysbase + 0x20) &= ~(1 << 13);
	
	count = 10;
	while(count--);

	// cdh:init mac hardware infor
	status = init_hw(ndev);
	if (status)
		return false;

	return true;
}


static s32 synop_open(struct net_device *netdev)
{
    signed int 		status = 0;
    signed int 		ijk;
    unsigned int 	dma_addr;
    struct sk_buff *skb; 
    nt_adapter *adapter;
    gmac_device *gmacdev;
    struct ak_mac_data *pdata;
    
    adapter = (nt_adapter *)netdev_priv(netdev);
    gmacdev = (gmac_device *)adapter->gmacdev_pt;
    pdata   = adapter->db_pt->dev->platform_data;
    
	// cdh:initial all mii share pin and clock,and complete phy reset from gpio62
    gmac_ctrl_init(pdata);

    /*cdh:Now platform dependent initialization.cdh:check ok*/
	gmac_mmc_counters_reset(gmacdev); // cdh:Mac Management Counters (MMC), cdh:check ok
	gmac_mmc_counters_disable_rollover(gmacdev); //cdh:check ok
	
    /*Lets reset the IP*/
    printk("adapter= %08x gmacdev = %08x netdev = %08x netdev= %08x\n", (u32)adapter, (u32)gmacdev, (u32)netdev, (u32)netdev);

	// cdh:software reset , the resets all of the GMAC internal registers and logic, cdh:check ok
    if (gmac_reset(gmacdev)) {
		return -1;
    }

    /*  cdh:Program/flash in the station/IP's Mac address */
    gmac_set_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, netdev->dev_addr);
    
    /* cdh:Lets read the version of ip, cdh:check ok */
    gmac_read_version(gmacdev);

	/* cdh:Lets set ipaddress in to device structure, cdh:check ok*/
    gmac_get_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, netdev->dev_addr);

    /* cdh:check ok,Now set the broadcast address*/
    for(ijk = 0; ijk < 6; ijk++) {
        netdev->broadcast[ijk] = 0xff;
    }
    
    for(ijk = 0; ijk < 6; ijk++) {
        printk("netdev->dev_addr[%d] = %02x and netdev->broadcast[%d] = %02x\n", ijk, netdev->dev_addr[ijk], ijk, netdev->broadcast[ijk]);
    }

	// cdh:set MDO_CLK for MDIO transmit, note GmacGmiiAddr bit5, and 802.3 limit 2.5MHz
    gmac_set_mdc_clk_div(gmacdev, GmiiCsrClk1);

    // cdh:get MDO_CLK div
    gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

    // cdh:initial phy and check phy link status
    gmac_phy_init(gmacdev, pdata);
    gmac_check_phy_init(gmacdev);

    /*Request for an shared interrupt. Instead of using netdev->irq lets use pcidev->irq*/
    if(request_irq (netdev->irq, gmac_intr_handler, SA_SHIRQ | SA_INTERRUPT, netdev->name, netdev)) {
        printk("Error in request_irq\n");
        goto error_in_irq;
    }

    printk("%s owns a shared interrupt on line %d\n", netdev->name, netdev->irq);

    /* cdh:check ok ok,Set up the 32 unit, tx descriptor queue/ring*/
    setup_tx_desc_queue(gmacdev, netdev, TRANSMIT_DESC_SIZE, RINGMODE);
    
    // cdh:check ok ok
    gmac_init_tx_desc_base(gmacdev); // cdh:Program the transmit descriptor base address in to DmaTxBase addr

	/* cdh:check ok ok,Set up the 32 unit, rx descriptor queue/ring*/
    setup_rx_desc_queue(gmacdev, netdev, RECEIVE_DESC_SIZE, RINGMODE);
    
    // cdh:check ok ok
    gmac_init_rx_desc_base(gmacdev); // cdh:Program the transmit descriptor base address in to DmaTxBase addr
    
	// cdh:dma busrt=32words=128B, two dma_descriptor interval = 2Bytes 
    gmac_set_dma_bus_mode(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2); //pbl32 incr with rxthreshold 128

	// cdh:set dma transmit method
    gmac_set_dma_control(gmacdev, DmaDisableFlush | DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);

	// cdh:check ok, initial mac part ip
    gmac_mac_init(gmacdev);
    
    // cdh:enable promisc mode for bridge
    gmac_promisc_enable(gmacdev);

	/**
	 *cdh:inital dma and mac flow control
	*/
    gmac_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

	#ifdef IPC_OFFLOAD
	/*IPC Checksum offloading is enabled for this driver. Should only be used if Full Ip checksumm offload engine is configured in the hardware*/
	synopGMAC_enable_rx_chksum_offload(gmacdev);  	//Enable the offload engine in the receive path
	synopGMAC_rx_tcpip_chksum_drop_enable(gmacdev); // This is default configuration, DMA drops the packets if error in encapsulated ethernet payload
													// The FEF bit in DMA control register is configured to 0 indicating DMA to drop the errored frames.
	/*Inform the Linux Networking stack about the hardware capability of checksum offloading*/
	netdev->features = NETIF_F_HW_CSUM;
	#endif
	
	/**
	 *cdh:initial one rx buffer dma infor, why loop 32 ci
	*/
    do {
        skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_KERNEL);
        if(skb == NULL)
        {
            printk("ERROR in skb buffer allocation\n");
            break;
        }
	
		/** cdh:note as follow
		* 函数pci_map_single映射单个用于传送的缓冲区，返回值是可以传递给设备的总线地址，如果出错的话就为 NULL。
		* 一旦传送完成，应该使用函数pci_unmap_single 删除映射。其中，参数direction为传输的方向，取值如下：
		* PCI_DMA_TODEVICE 数据被发送到设备。
		* PCI_DMA_FROMDEVICE如果数据将发送到 CPU。
		* PCI_DMA_BIDIRECTIONAL数据进行两个方向的移动。
		* PCI_DMA_NONE 这个符号只是为帮助调试而提供。
		*/
       	// cdh:malloc buffer to descriptor dma buffer, dma_addr as physic address
        dma_addr = dma_map_single(NULL, skb->data, skb_tailroom(skb), DMA_FROM_DEVICE);
        status = gmac_set_rx_qptr(gmacdev, dma_addr, skb_tailroom(skb), (u32)skb, 0, 0, 0);
        if(status < 0)
            dev_kfree_skb(skb);
        else
        	; // cdh:printk("skb, head, data, tail: %x, %x, %x, %x\n", skb->head, (u32 *)skb->data, (u32 *)skb->tail, (u32 *)skb->len);
    }while(status >= 0);

    
    /**
	 *cdh:inital one timer to detect the cable plugging and unplugging
	*/
	printk("Setting up the cable unplug timer\n");
    init_timer(&timer_cable_unplug);
    timer_cable_unplug.function = (void *)timer_cable_unplug_proc;
    timer_cable_unplug.data = (u32) adapter;
    timer_cable_unplug.expires = CHECK_TIME + jiffies;
    add_timer(&timer_cable_unplug);

	/**
	 *cdh:Clear all the interrupts
	*/
    gmac_clear_interrupt(gmacdev);

    /**
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
      */
    gmac_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF); // cdh:set 1for mask interrupt
    gmac_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
    gmac_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

    /**
	 *cdh:Enable Tx and Rx DMA
	*/
    gmac_enable_interrupt(gmacdev, DmaIntEnable);
    gmac_enable_dma_rx(gmacdev);
    gmac_enable_dma_tx(gmacdev);

    /**
	 *cdh:start platform dependent network interface
	*/
    netif_start_queue(netdev);

    return 0; 

error_in_irq:
    return -ESYNOPGMACBUSY;
}

/**
 * Function used when the interface is closed.
 *
 * This function is registered to linux stop() function. This function is
 * called whenever ifconfig (in Linux) closes the device (for example "ifconfig eth0 down").
 * This releases all the system resources allocated during open call.
 * system resources int needs
 * 	- Disable the device interrupts
 * 	- Stop the receiver and get back all the rx descriptors from the DMA
 * 	- Stop the transmitter and get back all the tx descriptors from the DMA
 * 	- Stop the Linux network queue interface
 *	- Free the irq (ISR registered is removed from the kernel)
 * 	- Release the TX and RX descripor memory
 *	- De-initialize one second timer rgistered for cable plug/unplug tracking
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */
static s32 synop_close(struct net_device *netdev)
{
    nt_adapter *adapter;
    gmac_device *gmacdev;

    adapter = (nt_adapter *) netdev_priv(netdev);
    if(adapter == NULL) {
        TR("OOPS adapter is null\n");
        return -ESYNOPGMACNOMEM;
    }

    gmacdev = (gmac_device *) adapter->gmacdev_pt;
    if(gmacdev == NULL) {
        TR("OOPS gmacdev is null\n");
        return -ESYNOPGMACNOMEM;
    }

    /* cdh:Disable all the interrupts*/
    gmac_disable_interrupt_all(gmacdev);
    printk("the synopGMAC interrupt has been disabled\n");

    /* cdh:Disable the reception*/
    gmac_disable_dma_rx(gmacdev);
    gmac_take_desc_ownership_rx(gmacdev);
    printk("the synopGMAC Reception has been disabled\n");

    /* cdh:Disable the transmission*/
    gmac_disable_dma_tx(gmacdev);
    gmac_take_desc_ownership_tx(gmacdev);

    printk("the synopGMAC Transmission has been disabled\n");
    netif_stop_queue(netdev);
    
    /* cdh:Now free the irq: This will detach the interrupt handler registered*/
    free_irq(netdev->irq, netdev);
    printk("the synopGMAC interrupt handler has been removed\n");

    /* cdh:Free the Rx Descriptor contents*/
    printk("Now calling giveup_rx_desc_queue \n");
    giveup_rx_desc_queue(gmacdev, netdev, RINGMODE);

    /* cdh:Free the Tx Descriptor contents*/
    printk("Now calling giveup_tx_desc_queue \n");
    giveup_tx_desc_queue(gmacdev, netdev, RINGMODE);

	/* cdh:Free the unplug check Timer*/
    printk("Freeing the cable unplug timer\n");
    del_timer(&timer_cable_unplug);

    return -ESYNOPGMACNOERR;
}

/**
 * Function to transmit a given packet on the wire.
 * Whenever Linux Kernel has a packet ready to be transmitted, this function is called.
 * The function prepares a packet and prepares the descriptor and
 * enables/resumes the transmission.
 * @param[in] pointer to sk_buff structure.
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and Error code on failure.
 * \note structure sk_buff is used to hold packet in Linux networking stacks.
 */
static s32 synop_xmit_frames(struct sk_buff *skb, struct net_device *netdev)
{
    s32 status = 0;
    u32 offload_needed = 0;
    u32 dma_addr;
    nt_adapter *adapter;
    gmac_device *gmacdev;
    mac_info_t *db;
    unsigned long flags;

    if(skb == NULL) {
        printk("skb is NULL What happened to Linux Kernel? \n ");
        return -ESYNOPGMACNOMEM;
    }

    adapter = (nt_adapter *)netdev_priv(netdev);
    if(adapter == NULL)
        return -ESYNOPGMACNOMEM;

    gmacdev = (gmac_device *)adapter->gmacdev_pt;
    if(gmacdev == NULL)
        return -ESYNOPGMACNOMEM;

    db = adapter->db_pt;

    /* cdh:Stop the network queue*/
    netif_stop_queue(netdev);

	/* cdh:for ipc offload used */
    if(skb->ip_summed == CHECKSUM_HW) {
        /**
             *cdh:In Linux networking, if kernel indicates skb->ip_summed = CHECKSUM_HW, then only checksum offloading should be performed
            	*cdh:Make sure that the OS on which this code runs have proper support to enable offloading.
             */
        offload_needed = 0x00000001;
    }

	
    /* cdh:because dma_map_single hase owned dma_spin lock ,so we need protect,Now we have skb ready and OS invoked this function. Lets make our DMA know about this*/
    dma_addr = dma_map_single(NULL, skb->data, skb->len, DMA_TO_DEVICE);

    /* cdh:protect share resources here,and we used only one data buffer */
    spin_lock_irqsave(&db->lock, flags);
    status = gmac_set_tx_qptr(gmacdev, dma_addr, skb->len, (u32)skb, 0, 0, 0, offload_needed);
    if(status < 0)
    {
        printk("%s No More Free Tx Descriptors\n", __FUNCTION__);
        spin_unlock_irqrestore(&db->lock, flags);
        dma_unmap_single(NULL, dma_addr, 0, DMA_TO_DEVICE);
        return -EBUSY;
    }

    /* cdh:Now force the DMA to start transmission*/
    gmac_resume_dma_tx(gmacdev);
    netdev->trans_start = jiffies;
    spin_unlock_irqrestore(&db->lock, flags);

    /*Now start the netdev queue*/
    netif_wake_queue(netdev);

    return -ESYNOPGMACNOERR;
}

/**
 * Function to handle a Tx Hang.
 * This is a software hook (Linux) to handle transmitter hang if any.
 * We get transmitter hang in the device interrupt status, and is handled
 * in ISR. This function is here as a place holder.
 * @param[in] pointer to net_device structure
 * \return void.
 */
static void synop_tx_timeout(struct net_device *netdev)
{
	nt_adapter *adapter;
	gmac_device *gmacdev;

	printk("%s\n", __FUNCTION__);
	adapter = (nt_adapter *) netdev_priv(netdev);
	if(adapter == NULL)
	{
		TR("OOPS adapter is null\n");
		return ;
	}

	gmacdev = (gmac_device *) adapter->gmacdev_pt;
	if(gmacdev == NULL)
	{
		TR("OOPS gmacdev is null\n");
		return ;
	}

	/* cdh:==step1==Disable all the interrupts*/
	gmac_disable_interrupt_all(gmacdev);
	printk("the synopGMAC interrupt has been disabled\n");

	/* cdh:Disable the reception*/
	gmac_disable_dma_rx(gmacdev);
	gmac_take_desc_ownership_rx(gmacdev);
	printk("the synopGMAC Reception has been disabled\n");

	/* cdh:Disable the transmission*/
	gmac_disable_dma_tx(gmacdev);
	gmac_take_desc_ownership_tx(gmacdev);

	printk("the synopGMAC Transmission has been disabled\n");
	netif_stop_queue(netdev);
	
	/*Free the Rx Descriptor contents*/
	printk("Now calling giveup_rx_desc_queue \n");
	giveup_rx_desc_queue(gmacdev, netdev, RINGMODE);
	
	printk("Now calling giveup_tx_desc_queue \n");
	giveup_tx_desc_queue(gmacdev, netdev, RINGMODE);

	/* cdh:==step2==retry initial mac and phy, because request interrupt already save, so not re-init */
	MacInit(netdev);

	/**
	 *cdh:start platform dependent network interface
	*/
	netif_start_queue(netdev);
	
    return;
}


/**
 * Function to set multicast and promiscous mode.
 * @param[in] pointer to net_device structure.
 * \return returns void.
 */
void synop_set_multicast_list(struct net_device *netdev)
{
    TR("%s called \n", __FUNCTION__);
    //todo Function not yet implemented.
    return;
}


/**
 * Function to change the Maximum Transfer Unit.
 * @param[in] pointer to net_device structure.
 * @param[in] New value for maximum frame size.
 * \return Returns 0 on success Errorcode on failure.
 */
s32 synop_change_mtu(struct net_device *netdev, s32 newmtu)
{
	if (newmtu < 68 || newmtu > ETH_DATA_LEN){
		printk(KERN_ERR"[%s][%d] not support mtu size %d\n", __func__, __LINE__, newmtu);
		return -EINVAL;
	}

	printk(KERN_ERR"[%s][%d] change mtu size from %d to %d\n", __func__, __LINE__, netdev->mtu, newmtu);
	netdev->mtu = newmtu;

	return 0;

}

/**
 * Function to set ethernet address of the NIC.
 * @param[in] pointer to net_device structure.
 * @param[in] pointer to an address structure.
 * \return Returns 0 on success Errorcode on failure.
 */
static s32 synop_set_mac_address(struct net_device *netdev, void *macaddr)
{

    nt_adapter *adapter = NULL;
    gmac_device *gmacdev = NULL;
    struct sockaddr *addr = macaddr;

    adapter = (nt_adapter *) netdev_priv(netdev);
    if(adapter == NULL)
        return -1;

    gmacdev = adapter->gmacdev_pt;
    if(gmacdev == NULL)
        return -1;

    if(!is_valid_ether_addr(addr->sa_data))
        return -EADDRNOTAVAIL;

    gmac_set_mac_addr(gmacdev, GmacAddr0High, GmacAddr0Low, addr->sa_data);
    gmac_get_mac_addr(adapter_pt->gmacdev_pt, GmacAddr0High, GmacAddr0Low, netdev->dev_addr);

    TR("%s called \n", __FUNCTION__);
	  save_ether_addr(netdev->dev_addr);
    return 0;
}

/**
  * Enables GMAC to look for Magic packet.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
void gmac_magic_packet_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus, GmacPmtMagicPktEnable);
    return;
}

/**
  * Populates the remote wakeup frame registers.
  * Consecutive 8 writes to GmacWakeupAddr writes the wakeup frame filter registers.
  * Before commensing a new write, frame filter pointer is reset to 0x0000.
  * A small delay is introduced to allow frame filter pointer reset operation.
  * @param[in] pointer to gmac_device.
  * @param[in] pointer to frame filter contents array.
  * \return returns void.
  * note:cdh:check ok
  */
void gmac_write_wakeup_frame_register(gmac_device *gmacdev, u32 *filter_contents)
{
    s32 i;
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus, GmacPmtFrmFilterPtrReset);
    plat_delay(10);
    for(i = 0; i < WAKEUP_REG_LENGTH; i++)
        gmac_write_reg((u32 *)gmacdev->MacBase, GmacWakeupAddr,  *(filter_contents + i));
    return;

}

/**
  * Enables GMAC to look for wake up frame.
  * Wake up frame is defined by the user.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
void gmac_wakeup_frame_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus, GmacPmtWakeupFrameEnable);
    return;
}

/**
  * Disables the powerd down setting of GMAC.
  * If the driver wants to bring up the GMAC from powerdown mode, even though the magic packet or the
  * wake up frames received from the network, this function should be called.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  */
void gmac_power_down_disable(gmac_device *gmacdev)
{
    gmac_clr_bits((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus, GmacPmtPowerDown);
    return;
}




/**
  * Enables the power down mode of GMAC.
  * This function puts the Gmac in power down mode.
  * @param[in] pointer to gmac_device.
  * \return returns void.
  * note:cdh:check ok
  */
void gmac_power_down_enable(gmac_device *gmacdev)
{
    gmac_set_bits((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus, GmacPmtPowerDown);
    return;
}



// cdh:check ok
static void powerdown_mac(gmac_device *gmacdev)
{
    TR("Put the GMAC to power down mode..\n");
    // Disable the Dma engines in tx path
    GMAC_Power_down = 1;	// Let ISR know that Mac is going to be in the power down mode
    gmac_disable_dma_tx(gmacdev);
    plat_delay(10000);		//allow any pending transmission to complete
    // Disable the Mac for both tx and rx
    gmac_tx_disable(gmacdev);
    gmac_rx_disable(gmacdev);
    plat_delay(10000); 		//Allow any pending buffer to be read by host
    //Disable the Dma in rx path
    gmac_disable_dma_rx(gmacdev);

    //enable the power down mode
    //gmac_pmt_unicast_enable(gmacdev);

    //prepare the gmac for magic packet reception and wake up frame reception
    gmac_magic_packet_enable(gmacdev);
    gmac_write_wakeup_frame_register(gmacdev, synopGMAC_wakeup_filter_config3);

    gmac_wakeup_frame_enable(gmacdev);

    //gate the application and transmit clock inputs to the code. This is not done in this driver :).

    //enable the Mac for reception
    gmac_rx_enable(gmacdev);

    //Enable the assertion of PMT interrupt
    gmac_pmt_int_enable(gmacdev);
    //enter the power down mode
    gmac_power_down_enable(gmacdev);
    return;
}


/**
 * IOCTL interface.
 * This function is mainly for debugging purpose.
 * This provides hooks for Register read write, Retrieve descriptor status
 * and Retreiving Device structure information.
 * @param[in] pointer to net_device structure.
 * @param[in] pointer to ifreq structure.
 * @param[in] ioctl command.
 * \return Returns 0 on success Error code on failure.
 */
s32 synop_do_ioctl(struct net_device *netdev, struct ifreq *ifr, s32 cmd)
{

    s32 retval = 0;
    u16 temp_data = 0;
    nt_adapter *adapter = NULL;
    gmac_device *gmacdev = NULL;

    struct ifr_data_struct
    {
        u32 unit;
        u32 addr;
        u32 data;
    } *req;


    if(netdev == NULL)
        return -1;
    if(ifr == NULL)
        return -1;

    req = (struct ifr_data_struct *)ifr->ifr_data;

    adapter = (nt_adapter *) netdev_priv(netdev);
    if(adapter == NULL)
        return -1;

    gmacdev = adapter->gmacdev_pt;
    if(gmacdev == NULL)
        return -1;
    TR("%s :: on device %s req->unit = %08x req->addr = %08x req->data = %08x cmd = %08x \n",__FUNCTION__,netdev->name,req->unit,req->addr,req->data,cmd);

    switch(cmd)
    {
    case IOCTL_READ_REGISTER: //IOCTL for reading IP registers : Read Registers
        if (req->unit == 0) // Read Mac Register
            req->data = gmac_read_reg((u32 *)gmacdev->MacBase, req->addr);
        else if (req->unit == 1) // Read DMA Register
            req->data = gmac_read_reg((u32 *)gmacdev->DmaBase, req->addr);
        else if (req->unit == 2) // Read Phy Register
        {
            retval = gmac_read_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, req->addr, &temp_data);
            req->data = (u32)temp_data;
            if(retval != -ESYNOPGMACNOERR)
                TR("ERROR in Phy read\n");
        }
        break;

    case IOCTL_WRITE_REGISTER: //IOCTL for reading IP registers : Read Registers
        if (req->unit == 0) // Write Mac Register
            gmac_write_reg((u32 *)gmacdev->MacBase, req->addr, req->data);
        else if (req->unit == 1) // Write DMA Register
            gmac_write_reg((u32 *)gmacdev->DmaBase, req->addr, req->data);
        else if (req->unit == 2) // Write Phy Register
        {
            retval = gmac_write_phy_reg((u32 *)gmacdev->MacBase, gmacdev->PhyBase, req->addr, req->data);
            if(retval != -ESYNOPGMACNOERR)
                TR("ERROR in Phy read\n");
        }
        break;

    case IOCTL_READ_IPSTRUCT: //IOCTL for reading GMAC DEVICE IP private structure
        if (req->unit == 0)
        {
            gmacdev = adapter->gmacdev_pt;
            if(gmacdev == NULL)
                return -1;
            memcpy((gmac_device *)req->addr, gmacdev, sizeof(gmac_device));
        }
        else
            return -1;

        // memcpy(ifr->ifr_data, gmacdev, sizeof(gmac_device));
        break;
        /* Total Number of interrupts over which AvbBits are accumulated*/
    case IOCTL_READ_RXDESC: //IOCTL for Reading Rx DMA DESCRIPTOR
        if (req->unit == 0)
        {
            gmacdev = adapter->gmacdev_pt;
            if(gmacdev == NULL)
                return -1;
            memcpy((DmaDesc *)req->addr, gmacdev->RxDesc + ((DmaDesc *) (req->addr))->data1, sizeof(DmaDesc) );
        }
        else
            return -1;

        // memcpy(ifr->ifr_data, gmacdev->RxDesc + ((DmaDesc *) (ifr->ifr_data))->data1, sizeof(DmaDesc) );
        break;

    case IOCTL_READ_TXDESC: //IOCTL for Reading Tx DMA DESCRIPTOR
        if (req->unit == 0)
        {
            gmacdev = adapter->gmacdev_pt;
            if(gmacdev == NULL)
                return -1;
            memcpy((DmaDesc *)req->addr, gmacdev->TxDesc + ((DmaDesc *) (req->addr))->data1, sizeof(DmaDesc) );
        }
        else
            return -1;

        // memcpy(ifr->ifr_data, gmacdev->TxDesc + ((DmaDesc *) (ifr->ifr_data))->data1, sizeof(DmaDesc) );
        break;
    case IOCTL_POWER_DOWN: // CDH:WAIT FOR ...........
        if (req->unit == 1) //power down the mac
        {
            TR("============I will Power down the MAC now =============\n");
            // If it is already in power down don't power down again
            retval = 0;
            if(((gmac_read_reg((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus)) & GmacPmtPowerDown) != GmacPmtPowerDown)
            {
                powerdown_mac(gmacdev);
                retval = 0;
            }
        }
        if (req->unit == 2) //Disable the power down  and wake up the Mac locally
        {
            TR("============I will Power up the MAC now =============\n");
            //If already powered down then only try to wake up
            retval = -1;
            if(((gmac_read_reg((u32 *)gmacdev->MacBase, GmacPmtCtrlStatus)) & GmacPmtPowerDown) == GmacPmtPowerDown)
            {
                gmac_power_down_disable(gmacdev);
                powerup_mac(gmacdev);
                retval = 0;
            }
        }
        break;
    // cdh:case IOCTL_AVB_TEST: //IOCTL for AVB Testing    
    default:
        retval = -1;
    }
    return retval;
}

/**
 * Function provides the network interface statistics.
 * Function is registered to linux get_stats() function. This function is
 * called whenever ifconfig (in Linux) asks for networkig statistics
 * (for example "ifconfig eth0").
 * @param[in] pointer to net_device structure.
 * \return Returns pointer to net_device_stats structure.
 * \callgraph
 */
struct net_device_stats *synop_get_stats(struct net_device *netdev)
{
    TR("%s called \n", __FUNCTION__);
    return( &(((nt_adapter *)(netdev_priv(netdev)))->net_dev_stats) );
}

static const struct net_device_ops synop_net_device_ops = {
	.ndo_open = synop_open, 
	.ndo_stop = synop_close,
	.ndo_start_xmit = synop_xmit_frames,
	.ndo_tx_timeout = synop_tx_timeout,
	.ndo_change_mtu = synop_change_mtu,
	.ndo_set_mac_address = synop_set_mac_address,
	.ndo_do_ioctl = synop_do_ioctl,
	.ndo_get_stats		= synop_get_stats,
	.ndo_validate_addr = eth_validate_addr,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ak_mac_poll_controller,
#endif
	
};


static const struct ethtool_ops akgmac100_ethtool_ops = {
	.get_link		= ethtool_op_get_link,
};
static int get_mac_addr(struct ak_mac_data *pdata)
{
	int i;
	unsigned char mac_addr[32] = {0};
	unsigned char prefix_macaddr_cnt = 0;
	void * handle ;
	int ret = 0;
	int retvalue = 0;
	
	/* search the same name partition */
	handle = partition_open(MAC_FILE_NAME);
	if (handle == NULL) {
		printk(KERN_ERR "%s, open partition error!\n", __func__);
		return -ENOENT;
	}

	ret = partition_read(handle , mac_addr, 32);
	if (ret == -1) {
		printk(KERN_ERR "%s, write partition error!\n", __func__);
		retvalue = -ENOENT;
	}

	/* search open the same name partition */
	ret = partition_close(handle);
	if (ret == -1) {
		printk(KERN_ERR "%s, close partition error!\n", __func__);
		return -ENOENT;
	}
	
	// cdh:check prefix equal FF:FF:FF
	for (i = 0; i < MAC_ADDR_APSTRING_LEN; i++) {
		if ((i % 3 != 2)) {
			mac_addr[i + 4] = toupper(mac_addr[i + 4]);
			if (mac_addr[i + 4] == 'F') {
				prefix_macaddr_cnt++;
			}
		}
	}

	if (prefix_macaddr_cnt == (MAC_ADDR_APSTRING_LEN - 2)) {
		goto out;
	}

	for (i = 0; i < MAC_ADDR_STRING_LEN; i++) {
		if ((i % 3 != 2)) {
			mac_addr[i + 4] = toupper(mac_addr[i + 4]);
			if (!(isdigit(mac_addr[i + 4]) || (mac_addr[i + 4] <= 'F' && mac_addr[i + 4] >= 'A')))
				goto out;
		}
		else if (mac_addr[i + 4] != ':')
			goto out;
	}
	
	for (i = 0; i < MAC_ADDR_LEN; i++)
		pdata->dev_addr[i] = CTOI(mac_addr[i * 3 + 4]) * 16 + CTOI(mac_addr[i * 3 + 5]);

	return 0;
out:
	
	printk("Failed to read MAC addres in medium storage, use default mac\n");
	return -1;
}

static int UpdateAsaFile(char *buf, char* filename)
{
	unsigned char ttbuf[64];
	void * handle ;
	int ret = 0;
	int retvalue = 0;
	
	memset(ttbuf, 0, 64);
	ttbuf[0] = strlen(buf);
	memcpy(ttbuf + 4, buf, strlen(buf));

	/* search the same name partition */
	handle = partition_open(MAC_FILE_NAME);
	if (handle == NULL) {
		printk(KERN_ERR "%s, open partition error!\n", __func__);
		return -ENOENT;
	}
	
	ret = partition_write(handle , ttbuf, strlen(buf)+4);
	if (ret == -1) {
		printk(KERN_ERR "%s, write partition error!\n", __func__);
		retvalue = -ENOENT;
	}

	/* search open the same name partition */
	ret = partition_close(handle);
	if (ret == -1) {
		printk(KERN_ERR "%s, close partition error!\n", __func__);
		return -ENOENT;
	}
	
	return retvalue;
}

static int save_ether_addr(unsigned char dev_addr[MAC_ADDR_LEN])
{
	int i;
	int ret;
	char mac[64];
	
	memset(mac, '\0', sizeof(mac));
	for (i=0; i<MAC_ADDR_LEN; i++) {
		sprintf(mac + strlen(mac), "%.2X", dev_addr[i]);
		if (i < (MAC_ADDR_LEN - 1)) {
			strcat(mac, ":");
		}
	}

	ret = UpdateAsaFile(mac, MAC_FILE_NAME);

	return ret;
}

#if defined(CONFIG_ETHERNET_MAC_RMII)
static void gmac_rmii_interface_50mclk(void)
{
	int i = 0;

	// cdh:set chip system ctrl reg base address
	psysbase = AK_VA_SYSCTRL; 
	
	// cdh:Rmii, select top rmii interface and 100mbps speed, and make 50M clock to phy
	REG32(psysbase + 0x14) &=  ~(0x1 << 22); // cdh:first mac interface select Rmii
	REG32(psysbase + 0x14) |=  (0x1 << 23); // cdh:first  mac_speed_cfg=1(100m)
	REG32(psysbase + 0x14) |=  (0x1 << 21); // cdh:bit[21],enable generate 50m
	REG32(psysbase + 0x14) |=  (0x1 << 28); // cdh:bit[28],mac feedback clk en,lan tiantian add
	REG32(psysbase + 0x14) |=  ((0x1 << 16)|(0x1 << 18)); // cdh:set   bit[21],enable generate 50m,	bit[18], select 25m clock of mac from pll div
	REG32(psysbase + 0x1c) &= ~(1 << 13);		 // cdh:mac ctronller clk 

	// cdh:bit[20]:select mac 50M clock from 50M crystal or pll div,here do what's mean? ,pg said at least repeat twice
	for(i=0; i<6; i++) {
		REG32(psysbase + 0x14) |= (1 << 20);	// cdh:select 50m crystal pad, what's mean?
		REG32(psysbase + 0x14) &= ~(1 << 20);	// cdh:select 50m clock input, what's mean?
	}

}
#endif

#if defined(CONFIG_ETHERNET_MAC_MII)
static void gmac_mii_interface_25mclk(void)
{
	int i = 0;

	// cdh:set chip system ctrl reg base address
	psysbase = AK_VA_SYSCTRL; 
	
	// cdh:mii, select top mii interface and 100mbps speed, and make 25M clock to phy
	REG32(psysbase + 0x14) |=  ((0x1 << 22)|(0x1 << 23)); // cdh:first mac interface select mii, mac_speed_cfg=1(100m)
	REG32(psysbase + 0x14) &= ~(0x1 << 15); // cdh:clear bit[15], prohibit 25m crystal
	REG32(psysbase + 0x14) |=  ((0x1 << 16)|(0x1 << 18)); // cdh:set   bit[16],enable div24, generate 25m,	bit[18], select 25m clock of mac from pll div
	REG32(psysbase + 0x1c) &= ~(1 << 13);		 // cdh:mac clk ctrl

	// cdh:bit[20]:select mac 25M clock from 25M crystal or pll div,here do what's mean? ,pg said at least repeat twice
	for(i=0; i<6; i++) {
		REG32(psysbase + 0x14) |= (1 << 20);	// cdh:select 25m crystal pad, what's mean?
		REG32(psysbase + 0x14) &= ~(1 << 20);	// cdh:select 25m clock input, what's mean?
	}

}
#endif

static void gmac_clk_sharepin_init(void)
{
#if defined(CONFIG_ETHERNET_MAC_RMII)
	printk("Configed MAC RMII interface!\n");
	// cdh:set rmii interface and 50m phy clock sharepin 
	ak_group_config(ePIN_AS_RMAC);

	// cdh:disable gpio47 pull down
	g_ak39_gpio_pulldown(AK_GPIO_47, AK_PULLDOWN_DISABLE);

	// cdh:open 50M phy clock
	gmac_rmii_interface_50mclk();

#elif defined(CONFIG_ETHERNET_MAC_MII)	
	printk("Configed MAC MII interface!\n");
	// cdh:set mii interface and 25m phy clock sharepin 
	ak_group_config(ePIN_AS_MAC);

	// cdh:disable gpio47 pull down
	g_ak39_gpio_pulldown(AK_GPIO_47, AK_PULLDOWN_DISABLE);

	// cdh:open 25M phy clock
	gmac_mii_interface_25mclk();

#else
	printk("Please config MAC interface!\n");
#endif

}

/*
 * Search AK39E MAC, allocate space and register it
 */
static int __devinit ak_mac_probe(struct platform_device *pdev)
{
	struct ak_mac_data *pdata = pdev->dev.platform_data; // cdh:from platform device data
	struct net_device *netdev;
	int ret = 0;
	int iosize = 0;
	
	/* cdh:alloc and Init network device */
	netdev = alloc_etherdev(sizeof(struct synopGMACAdapterStruct));
	if (!netdev) {
		dev_err(&pdev->dev, "could not allocate device.\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);

	/* setup board info structure */
	adapter_pt = (nt_adapter *)netdev_priv(netdev); 
	printk("netdev private = %x \n", (u32)adapter_pt);

	adapter_pt->netdev_pt 	= netdev;
    adapter_pt->db_pt 		= NULL; // cdh:pci_if, probe() function
    adapter_pt->gmacdev_pt  = NULL;  // cdh:gmac_device data 

	/*Allocate Memory for the the GMACip structure*/
	RingbufVa = dma_alloc_coherent(NULL, sizeof (gmac_device), &RingbufPa, GFP_KERNEL);
	adapter_pt->gmacdev_pt = (gmac_device *)RingbufVa;
	if(!adapter_pt->gmacdev_pt) {
        printk("Error in Memory Allocataion \n");
    }else {
	    printk("Allocataion gmacdev OK\n");
    }

    /*Allocate Memory for the the GMACip structure*/
	BoardbufVa = dma_alloc_coherent(NULL, sizeof (mac_info_t), &BoardbufPa, GFP_KERNEL);
	adapter_pt->db_pt = (mac_info_t *)BoardbufVa;
	if(!adapter_pt->db_pt) {
        printk("Error in Memory Allocataion \n");
    }else{
	    printk("Allocataion mac_info OK\n");
    }
    
	adapter_pt->db_pt->dev = &pdev->dev;
	adapter_pt->db_pt->ndev = netdev;

	/* cdh:get the register and irq resource */
	adapter_pt->db_pt->addr_res = platform_get_resource(pdev, IORESOURCE_MEM, 0); // cdh:get ctrl reg physic addr
	adapter_pt->db_pt->irq_res  = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (adapter_pt->db_pt->addr_res == NULL || adapter_pt->db_pt->irq_res == NULL) {
		printk("insufficient resources\n");
		ret = -ENOENT;
		goto out;
	}

	iosize = resource_size(adapter_pt->db_pt->addr_res);
	adapter_pt->db_pt->addr_req = request_mem_region(adapter_pt->db_pt->addr_res->start, iosize, pdev->name);
	if (adapter_pt->db_pt->addr_req == NULL) {
		printk("cannot claim address reg area\n");
		ret = -EIO;
		goto out;
	}

	// cdh:mac reg physic addr map to virtual addr
	adapter_pt->db_pt->io_addr = ioremap(adapter_pt->db_pt->addr_res->start, iosize);
	if (adapter_pt->db_pt->io_addr == NULL) {
		printk("failed to ioremap address reg\n");
		ret = -EINVAL;
		goto out;
	}

	/* cdh:fill in parameters for net-dev structure */
	netdev->base_addr = (unsigned long)adapter_pt->db_pt->io_addr;	// cdh:mac ctrl reg base address(virtual address)
	netdev->irq		  = adapter_pt->db_pt->irq_res->start;	// cdh:mac ctrl interrupt
	
	/* cdh:driver system function */
	ether_setup(netdev);

	SET_ETHTOOL_OPS(netdev, &akgmac100_ethtool_ops);
	netdev->netdev_ops	= &synop_net_device_ops; // cdh:Ethernet operation 
	netdev->watchdog_timeo	= msecs_to_jiffies(5000); //cdh:Ethernet watchdog timeout

	// cdh:open mac ctrl and phy 25m clock, and set mii interface all share pin ok 
	gmac_clk_sharepin_init();

	// cdh:set mac address
	/* cdh:get Ethernet mac address from flash area &check valid ,notes first we can get fixed value , not read file*/
	if ((get_mac_addr(pdata) < 0)||(!is_valid_ether_addr(pdata->dev_addr))) {
		
		dev_warn(adapter_pt->db_pt->dev, "%s, Invalid Ethernet address. "
				"Generate software assigned\n\trandom Ethernet address.\n", netdev->name);

		/* Generate software assigned random Ethernet address */
		random_ether_addr(pdata->dev_addr);
		if (!is_valid_ether_addr(pdata->dev_addr))
			dev_warn(adapter_pt->db_pt->dev, "%s: Invalid Ethernet address. Please "
				"set using ifconfig\n", netdev->name);
		else
			save_ether_addr(pdata->dev_addr); // cdh:save random generated mac address
	}

	// cdh:save pdata->dev_addr mac address to ndev->dev_addr
	memcpy(netdev->dev_addr, pdata->dev_addr, 6);

	/**
       *Attach the device to MAC struct This will configure all the required base addresses
       *such as Mac base, configuration base, phy base address(out of 32 possible phys )
	* cdh:here must be initialled inside of ak_mac_probe
       */
    iomap_base = adapter_pt->db_pt->io_addr;
    gmac_attach(adapter_pt->gmacdev_pt, (u32) iomap_base + MACBASE, (u32) iomap_base + DMABASE, DEFAULT_PHY_BASE);


	// cdh:set mac base address
	pMacBase = adapter_pt->db_pt->io_addr;

	// cdh:set netdev as pdev->dev->prive
	platform_set_drvdata(pdev, netdev);

	// cdh:register net device
	ret = register_netdev(netdev);
	if (ret == 0) {
		printk("CDH_Success:%s: ak39E_mac at %p IRQ %d MAC: %pM\n",
		       netdev->name, adapter_pt->db_pt->io_addr, netdev->irq, netdev->dev_addr);
	}
	
	return 0;
out:
	printk("not found (%d).\n", ret);
	free_netdev(netdev);

	// cdh:release platform device resource
	if (adapter_pt->db_pt->addr_req) {
		release_mem_region(adapter_pt->db_pt->addr_res->start, iosize);
	}
	
	// cdh:must dma_free_coherent
	if (RingbufVa) {
		dma_free_coherent(NULL, sizeof (gmac_device), RingbufVa, RingbufPa);
		RingbufVa = NULL;
		RingbufPa = 0;
	}

	if (BoardbufVa) {
		dma_free_coherent(NULL, sizeof (mac_info_t), BoardbufVa, BoardbufPa);
		BoardbufVa = NULL;
		BoardbufPa = 0;
	}
	
	return ret;
}


static int ak_mac_drv_suspend(struct device *dev)
{
	
	return 0;
}

static int ak_mac_drv_resume(struct device *dev)
{
	
	return 0;
}

static struct dev_pm_ops ak_mac_drv_pm_ops = {
	.suspend	= ak_mac_drv_suspend,
	.resume		= ak_mac_drv_resume,
};

static int __devexit ak_mac_drv_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	nt_adapter *adapter;
	int iosize = 0;
	
    adapter = (nt_adapter *)netdev_priv(ndev);
  
	// cdh:check ok
	platform_set_drvdata(pdev, NULL);

	// cdh:check ok
	unregister_netdev(ndev);
	
	// cdh:release mii interface sharepin and clock,other request mem and resource
	mac_exit(ndev);

	// cdh:release ndev struct
	free_netdev(ndev);		

	// cdh:release platform device resource
	iosize = resource_size(adapter->db_pt->addr_res);
	if (adapter->db_pt->addr_req) {
		release_mem_region(adapter->db_pt->addr_res->start, iosize);
	}
	
	// cdh:must dma_free_coherent
	if (RingbufVa) {
		dma_free_coherent(NULL, sizeof (gmac_device), RingbufVa, RingbufPa);
		RingbufVa = NULL;
		RingbufPa = 0;
	}

	if (BoardbufVa) {
		dma_free_coherent(NULL, sizeof (mac_info_t), BoardbufVa, BoardbufPa);
		BoardbufVa = NULL;
		BoardbufPa = 0;
	}
	
	// cdh:release all pointer
	adapter->netdev_pt	 = NULL;
	adapter->db_pt		 = NULL; 
	adapter->gmacdev_pt  = NULL;  

	printk("released and freed device\n");
	
	return 0;
}

static struct platform_driver ak_mac_driver = {
	.driver	= {
		.name    = "ak_ethernet",
		.owner	 = THIS_MODULE,
		.pm	 = &ak_mac_drv_pm_ops,
	},
	.probe   = ak_mac_probe,
	.remove  = __devexit_p(ak_mac_drv_remove),
};

static int __init ak_mac_init(void)
{
	printk(KERN_INFO "%s Ethernet Driver, V%s\n", MACNAME, DRV_VERSION);

	return platform_driver_register(&ak_mac_driver);
}

static void __exit ak_mac_cleanup(void)
{
	platform_driver_unregister(&ak_mac_driver);
}

module_init(ak_mac_init);
module_exit(ak_mac_cleanup);

MODULE_AUTHOR("Tang Anyang, Zhang Jingyuan (C) ANYKA");
MODULE_DESCRIPTION("Anyka MAC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ak-ethernet");
