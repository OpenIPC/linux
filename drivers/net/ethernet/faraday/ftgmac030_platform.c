/*
 * Faraday FTGMAC030 Ethernet
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
#include "ftgmac030.h"
#include <mach/ftpmu010.h>
#include <mach/fmem.h>
#include <linux/gpio.h>

static int mac_fd = 0, mac_fd_1 = 0;

#ifdef CONFIG_FTGMAC030_DRIVER_0
static pmuReg_t pmu_reg[] = {
#ifndef CONFIG_FPGA
    //{reg_off, bits_mask, lock_bits, init_val, init_mask},
    {0x54, (0x1 << 2), (0x1 << 2), 0, (0x1 << 2)},	//reset off
    {0x60, (0x1 << 21), (0x1 << 21), 0, (0x1 << 21)},	//gating clock on
    {0x6C, (0x1 << 19), (0x1 << 19), 0, (0x1 << 19)},	//gating clock on
    {0x70, (0x1 << 11), (0x1 << 11), 0, (0x1 << 11)},	//gating clock on
    {0x188, (0x3F << 0), (0x3F << 0), (0x1 << 0), (0x3F << 0)},	//cntp pvalue
    {0x188, (0x3F << 8), (0x3F << 8), (0x4 << 8), (0x3F << 8)},//PHY cntp pvalue
#endif
};

static pmuRegInfo_t pmu_reg_info = {
    DRV_NAME,
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_PLL1,
    &pmu_reg[0]
};
#endif

#ifdef CONFIG_FTGMAC030_DRIVER_1
static pmuReg_t pmu_reg_1[] = {
#ifndef CONFIG_FPGA
    //{reg_off, bits_mask, lock_bits, init_val, init_mask},
    {0x54, (0x1 << 3), (0x1 << 3), 0, (0x1 << 3)},	//reset off
    {0x60, (0x1 << 22), (0x1 << 22), 0, (0x1 << 22)},	//gating clock on
    {0x6C, (0x1 << 20), (0x1 << 20), 0, (0x1 << 20)},	//gating clock on
    {0x70, (0x1 << 12), (0x1 << 12), 0, (0x1 << 12)},	//gating clock on
    {0x188, (0x3F << 16), (0x3F << 16), (0x1 << 16), (0x3F << 16)},	//cntp pvalue
    {0x188, (0x3F << 24), (0x3F << 24), (0x4 << 24), (0x3F << 24)},//PHY cntp pvalue
#endif
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


void set_mac_clock(int irq)
{
    int tmp_fd;
    
    if(irq == MAC_FTGMAC030_0_IRQ)
	    tmp_fd = mac_fd;
    else
	    tmp_fd = mac_fd_1;

#ifndef CONFIG_FPGA
#ifdef CONFIG_PLATFORM_GM8220
    if(irq == MAC_FTGMAC100_0_IRQ){
        //set phy X_GMII0_CLK_IN
        ftpmu010_write_reg(tmp_fd, 0x28, 0, (0x1 << 29));
        //set MAC p-value
        ftpmu010_write_reg(tmp_fd, 0x70, 0, 0x1);
    }
    
    if(irq == MAC_FTGMAC100_1_IRQ){
        //set phy X_GMII1_CLK_IN
        ftpmu010_write_reg(tmp_fd, 0x28, 0, (0x1 << 28));
        //set pin-mux
        ftpmu010_write_reg(tmp_fd, 0x50, 0, (0x1 << 14));        
        //set MAC p-value
        ftpmu010_write_reg(tmp_fd, 0x70, 0, (0x1 << 5));    
    }
#endif
#endif
}

void set_MDC_CLK(struct ftgmac030 *priv)
{
#ifndef CONFIG_FPGA
#ifndef CONFIG_PLATFORM_GM8210  /* 8210 can use default value *///???????????????????????
    int tmp;
    
    tmp = ioread32(priv->base + FTGMAC030_OFFSET_PHYCR);
    tmp &= ~FTGMAC030_PHYCR_MDC_CYCTHR_MASK;
    tmp |= FTGMAC030_PHYCR_MDC_CYCTHR(0xF0);
    iowrite32(tmp, priv->base + FTGMAC030_OFFSET_PHYCR);
#endif
#endif
}

int mac_scu_init(int irq)
{
    if(irq == MAC_FTGMAC030_0_IRQ){
    	mac_fd = ftpmu010_register_reg(&pmu_reg_info);
     	if (mac_fd < 0) {
            printk(KERN_ERR "MAC: %s register reg fail \n", __FUNCTION__);
    	    return -1;
    	}
#ifndef CONFIG_FPGA
#if defined(CONFIG_PLATFORM_GM8220) || defined(CONFIG_PLATFORM_GM8288)
        ftpmu010_write_reg(mac_fd, 0xB4, 0, (0x1 << 11)); 
#endif
#endif
    }

#ifndef CONFIG_FPGA
#ifdef CONFIG_PLATFORM_GM8220
    if(irq == MAC_FTGMAC030_1_IRQ){
    	mac_fd_1 = ftpmu010_register_reg(&pmu_reg_info_1);    
     	if (mac_fd_1 < 0) {
        	printk(KERN_ERR "MAC: %s register reg fail \n", __FUNCTION__);
    	    return -1;
    	}     	
        ftpmu010_write_reg(mac_fd_1, 0xB4, 0, (0x1 << 13));
    }
#endif
#endif  //CONFIG_FPGA
    return 0; 
}

void mac_scu_close(int irq)
{
    //IP Clock off 
    if(irq == MAC_FTGMAC030_0_IRQ){
#ifndef CONFIG_FPGA
#if defined(CONFIG_PLATFORM_GM8220) || defined(CONFIG_PLATFORM_GM8288)
    	ftpmu010_write_reg(mac_fd, 0xB4, (0x1 << 14), (0x1 << 14)); 
#endif
#endif
    	ftpmu010_deregister_reg(mac_fd);
    }
    else{
#ifndef CONFIG_FPGA
#if defined(CONFIG_PLATFORM_GM8220) || defined(CONFIG_PLATFORM_GM8288)
    	ftpmu010_write_reg(mac_fd_1, 0xB4, (0x1 << 13), (0x1 << 13)); 
#endif
#endif
    	ftpmu010_deregister_reg(mac_fd_1);
    }
}

void mac_reset(void)
{
#ifndef CONFIG_FPGA
    printk(KERN_NOTICE "HW reset\n");
    ftpmu010_write_reg(mac_fd, 0xA0, 0, (0x1 << 18));
    mdelay(1);
    ftpmu010_write_reg(mac_fd, 0xA0, (0x1 << 18), (0x1 << 18));
#endif
} 

/*  1-RMII or MII mode; 0-RGMII or GMII mode */
int interface_type(void)
{
#ifndef CONFIG_FPGA
#ifdef CONFIG_PLATFORM_GM8220
    if((ftpmu010_read_reg(0xD8) & 0x7) == 0x5)
        return 0;
#endif
#endif
	return 1;
}