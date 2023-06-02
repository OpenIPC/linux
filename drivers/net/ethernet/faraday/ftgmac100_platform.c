/*
 * Faraday FTGMAC100 Ethernet
 *
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/mii.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "ftgmac100.h"
#include <mach/ftpmu010.h>
#include <mach/fmem.h>
#include <linux/gpio.h>

static int mac_fd = 0, mac_fd_1 = 0;

#if defined(CONFIG_FTGMAC100_DRIVER_0_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_0_SLAVE)
#ifdef CONFIG_PLATFORM_GM8210
static pmuReg_t pmu_reg[] = {
    {0x28, (0x1 << 29), (0x1 << 29), 0, (0x1 << 29)},	//phy clock select
    {0x4C, (0x1 << 26), (0x1 << 26), (0x1 << 26), (0x1 << 26)},	//GTX_CLK polarity
    {0x70, 0x1F, 0x1F, 0, 0x1F},											//phy clock pvalue
    {0x7C, (0x1 << 2), (0x1 << 2), 0, (0x1 << 2)},		//rx clock phase
    {0xA0, (0x1 << 18), (0x1 << 18), (0x1 << 18), (0x1 << 18)},	//reset	    
    {0xB4, (0x1 << 14), (0x1 << 14), 0, (0x1 << 14)}, //clock off   
};

static pmuRegInfo_t pmu_reg_info = {
    DRV_NAME,
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_PLL1,
    &pmu_reg[0]
};
#endif
#ifdef CONFIG_PLATFORM_GM8287
static pmuReg_t pmu_reg[] = {
    {0x28, (0x1 << 29), (0x1 << 29), 0, (0x1 << 29)},
    {0x40, (0xF << 12), (0xF << 12), (0x3 << 12), (0xF << 12)},
    {0x4C, (0x1 << 26), (0x1 << 26), 0, (0x1 << 26)},
    //{0x50, (0xF << 10), (0xF << 10), 0, (0xF << 10)},// u-boot will set RGMII or RMII
    //{0x70, 0x1F, 0x1F, 0x5, 0x1F},// u-boot will set this function with UART
    {0x7C, (0x1 << 2), (0x1 << 2), 0, (0x1 << 2)},
    {0xA0, (0x1 << 18), (0x1 << 18), (0x1 << 18), (0x1 << 18)},
    {0xB4, (0x1 << 14), (0x1 << 14), 0, (0x1 << 14)},    
};

static pmuRegInfo_t pmu_reg_info = {
    DRV_NAME,
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_PLL1,
    &pmu_reg[0]
};
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
static pmuReg_t pmu_reg[] = {
    {0x28, (0x1 << 4), (0x1 << 4), 0, (0x1 << 4)},
    //{0x5C, (0xFFFFFFFF << 0), (0xFFFFFFFF << 0), 0x22222222, (0xFFFFFFFF << 0)},// u-boot will set RGMII or RMII
    //{0x60, (0xFFFFFFFF << 0), (0xFFFFFFFF << 0), 0x11112022, (0xFFFFFFFF << 0)},// u-boot will set RGMII or RMII
#ifdef CONFIG_FPGA
    {0x70, (0xFF << 4), (0xFF << 4), 0, (0xFF << 4)},
    {0x7C, (0x7 << 3), (0x7 << 3), 0x20, (0x7 << 3)}, //RGMII
#else
    //{0x70, (0xFF << 4), (0xFF << 4), 0, (0xFF << 4)},// u-boot will set this function with UART
    //{0x7C, (0x7 << 3), (0x7 << 3), 0, (0x7 << 3)},  // u-boot will set RGMII or RMII
#endif
    {0xA0, (0x1 << 18), (0x1 << 18), (0x1 << 18), (0x1 << 18)},
    {0xAC, (0x1 << 5), (0x1 << 5), 0, (0x1 << 5)}, 
    {0xB4, (0x1 << 11), (0x1 << 11), 0, (0x1 << 11)},  
};

static pmuRegInfo_t pmu_reg_info = {
    DRV_NAME,
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_PLL1,
    &pmu_reg[0]
};
#endif
#endif

#if defined(CONFIG_FTGMAC100_DRIVER_1_MASTER) || defined(CONFIG_FTGMAC100_DRIVER_1_SLAVE) 
static pmuReg_t pmu_reg_1[] = {
    //{reg_off, bits_mask, lock_bits, init_val, init_mask},
    {0x28, (0x1 << 28), (0x1 << 28), 0, 0},
    {0x4C, (0x1 << 27), (0x1 << 27), 0, 0},
    {0x50, (0x1 << 14), (0x1 << 14), 0, 0},
    {0x70, (0x1F << 5), (0x1F << 5), 0, 0},
    {0x7C, (0x1 << 3), (0x1 << 3), 0, 0},
    {0xA0, (0x1 << 19), (0x1 << 19), 0, 0},
    {0xB4, (0x1 << 13), (0x1 << 13), 0, 0},  
};
static pmuRegInfo_t pmu_reg_info_1 = {
    DRV_1_NAME,
    ARRAY_SIZE(pmu_reg_1),
    ATTR_TYPE_PLL1,
    &pmu_reg_1[0]
};
#endif

void wait_status(int millisecond)
{
    if (in_interrupt() || in_atomic()){
        mdelay(millisecond);
    }else{           
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout((HZ / 1000) * millisecond);
    }
} 

int CPU_detect(void)
{
#ifdef CONFIG_PLATFORM_GM8210	
	fmem_pci_id_t pci_id;
	fmem_cpu_id_t cpu_id;
	fmem_get_identifier(&pci_id, &cpu_id);
	if(cpu_id == FMEM_CPU_FA626)
		return 1;
#endif
	return 0;			
} 

void set_mac_clock(int irq)
{
    int tmp_fd;
    
    if(irq == MAC_FTGMAC100_0_IRQ)
	    tmp_fd = mac_fd;
    else
	    tmp_fd = mac_fd_1;

#ifdef CONFIG_PLATFORM_GM8210
    if(!CPU_detect()){
    	if(irq == MAC_FTGMAC100_0_IRQ){
		    //set phy X_GMII0_CLK_IN
		    ftpmu010_write_reg(tmp_fd, 0x28, 0, (0x1 << 29));
		    //set MAC p-value
		    ftpmu010_write_reg(tmp_fd, 0x70, 0, 0x1);
    	}
  	}

    if(CPU_detect()){
	    if(irq == MAC_FTGMAC100_1_IRQ){
		    //set phy X_GMII1_CLK_IN
		    ftpmu010_write_reg(tmp_fd, 0x28, 0, (0x1 << 28));
		    //set pin-mux
		    ftpmu010_write_reg(tmp_fd, 0x50, 0, (0x1 << 14));        
		    //set MAC p-value
		    ftpmu010_write_reg(tmp_fd, 0x70, 0, (0x1 << 5));    
	    }
  	}
#endif

#ifdef Generate_PHY_Clock              //Generate clock for GMAC

    wait_status(10);
    printk("Set PHY clock\n");
    gpio_direction_output(GPIO_pin, 1);
#ifdef CONFIG_ICPLUS_PHY
    wait_status(3000);
#else
    wait_status(10);
#endif    
    printk("Set PHY clock finish\n");
#endif

}

void set_MDC_CLK(struct ftgmac100 *priv)
{
#ifndef CONFIG_PLATFORM_GM8210  /* 8210 can use default value */
    int tmp;
    
    tmp = ioread32(priv->base + FTGMAC100_OFFSET_PHYCR);
    tmp &= ~FTGMAC100_PHYCR_MDC_CYCTHR_MASK;
    tmp |= FTGMAC100_PHYCR_MDC_CYCTHR(0xF0);
    iowrite32(tmp, priv->base + FTGMAC100_OFFSET_PHYCR);
#endif
}

int mac_scu_init(int irq)
{
    if(!CPU_detect()){
	    if(irq == MAC_FTGMAC100_0_IRQ){
	    	mac_fd = ftpmu010_register_reg(&pmu_reg_info);
	     	if (mac_fd < 0) {
	   	        printk("MAC: %s register reg fail \n", __FUNCTION__);
	 		    return -1;
	    	}
#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
    		ftpmu010_write_reg(mac_fd, 0xB4, 0, (0x1 << 14)); 
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    		ftpmu010_write_reg(mac_fd, 0xB4, 0, (0x1 << 11)); 
#endif
    	}
  	} 	

#ifdef CONFIG_PLATFORM_GM8210
    if(CPU_detect()){
    	if(irq == MAC_FTGMAC100_1_IRQ){
	    	mac_fd_1 = ftpmu010_register_reg(&pmu_reg_info_1);    
	     	if (mac_fd_1 < 0) {
	   	    printk("MAC: %s register reg fail \n", __FUNCTION__);
	 		    return -1;
	    	}     	
	        ftpmu010_write_reg(mac_fd_1, 0xB4, 0, (0x1 << 13));
    	}
  	}  
#endif     
    return 0; 
}

void mac_scu_close(int irq)
{
    //IP Clock off 
    if(irq == MAC_FTGMAC100_0_IRQ){
#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
    	ftpmu010_write_reg(mac_fd, 0xB4, (0x1 << 14), (0x1 << 14)); 
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    	ftpmu010_write_reg(mac_fd, 0xB4, (0x1 << 11), (0x1 << 11)); 
#endif 
    	ftpmu010_deregister_reg(mac_fd);
    }
    else{
    	ftpmu010_write_reg(mac_fd_1, 0xB4, (0x1 << 13), (0x1 << 13)); 
    	ftpmu010_deregister_reg(mac_fd_1);
    }
}

void mac_reset(void)
{    
    ftpmu010_write_reg(mac_fd, 0xA0, 0, (0x1 << 18));
    //mdelay(1);
    printk("HW reset\n");
    ftpmu010_write_reg(mac_fd, 0xA0, (0x1 << 18), (0x1 << 18));
} 

/*  1-RMII or MII mode; 0-RGMII or GMII mode */
int interface_type(void)
{
#ifdef CONFIG_PLATFORM_GM8287
    if((ftpmu010_read_reg(0xD8) & 0x7) == 0x5)
        return 0;
#endif
#ifdef CONFIG_PLATFORM_GM8139
    if(ftpmu010_read_reg(0x7C) & (1 << 5))
        return 0;
#endif        
	return 1;
}