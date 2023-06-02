/*
 * arch/arm/mach-gk7101/gpio.c
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
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include <plat/gk_gpio.h>

gpio_cfg_t  gk_all_gpio_cfg;

struct gk_gpio_bank gk_gpio0_banks[] =
{
    GK_GPIO_BANK("gk-gpio0", GPIO0_BANK0_BASE, GPIO0_BANK0_PLL_IOCTRL_BASE,
        GK_GPIO(0 * GPIO_BANK_SIZE), CONFIG_ARCH_NR_GPIO, 0),
};
// last one max = CONFIG_ARCH_NR_GPIO - base

struct gk_gpio_inst gk_gpio_insts[CONFIG_GK_GPIO_INSTANCES] =
{
    [0] =
    {
        .bank_num       = ARRAY_SIZE(gk_gpio0_banks),
        .gpio_bank      = gk_gpio0_banks,
        .output_cfg     = {0},
        .input_cfg      = {0},
        .irq_no         = GPIO0_IRQ,
        .irq_info       = {{0}},
        .irq_now        = 0,
        .gpio_valid     = {0},
        .gpio_freeflag  = {0},
        .irq_flag       = {0},
        .base_bus       = GPIO0_BASE,
        .per_sel_reg    = 0,
    },
};

int __init gk_init_gpio(void)
{
    int     retval = 0;
    int     i, index;
    int     gpio_count;

    const char chip_type[][8] =
    {
        {"GK7101"},
        {"GK7102"},
        {"GK7101S"},
        {"GK7102S"}
    };

    /* use usr memory's GPIO setting by uboot pass*/
    u32 usrmemphyaddr=0;
    int intphy_gpio_count;
    int extphy_gpio_count;

    usrmemphyaddr=get_usrmem_virt();
    memcpy((u8*)&gk_all_gpio_cfg,(u8*)usrmemphyaddr,sizeof(gk_all_gpio_cfg));
    gk_all_gpio_cfg.board_type[31] = '\0';
    /*get gpio configure infor*/
    gpio_count = gk_all_gpio_cfg.gpio_count;
    intphy_gpio_count = gk_all_gpio_cfg.intphy_gpio_count;
    extphy_gpio_count = gk_all_gpio_cfg.extphy_gpio_count;

#if 1
    printk("###################################\n");
    if(gk_all_gpio_cfg.soc_type >= sizeof(chip_type)/ sizeof(chip_type[0]))
    {
        printk("[BOOT VERSION] %s %s v%d.%d \n", "Unknown",gk_all_gpio_cfg.board_type,
            gk_all_gpio_cfg.board_version>> 16, gk_all_gpio_cfg.board_version & 0xffff);
    }
    else
    {
        printk("[BOOT VERSION] %s %s v%d.%d \n", chip_type[gk_all_gpio_cfg.soc_type],gk_all_gpio_cfg.board_type,
            gk_all_gpio_cfg.board_version >> 16, gk_all_gpio_cfg.board_version & 0xffff);
    }
    if (gk_all_gpio_cfg.ext_phy_clk == 0)
    printk("[NET  INT_CLK] Internal PHY clock \n");
    else
    printk("[NET  EXT_CLK] External PHY clock %dMHz \n", gk_all_gpio_cfg.ext_phy_clk);
    printk("[GPIO]#############################\n");
    printk("[GPIO] gpio map get from uboot\n");
    printk("[GPIO CFG] gpio   count = %d\n",gk_all_gpio_cfg.gpio_count);
    printk("[GPIO CFG] intphy count = %d\n",gk_all_gpio_cfg.intphy_gpio_count);
    printk("[GPIO CFG] extphy count = %d\n",gk_all_gpio_cfg.extphy_gpio_count);
    printk("[GPIO CFG] IR LED CTL    (%d)\n",gk_all_gpio_cfg.ir_led_ctl);
    printk("[GPIO CFG] IR CUT1       (%d)\n",gk_all_gpio_cfg.ir_cut1);
    printk("[GPIO CFG] IR CUT2       (%d)\n",gk_all_gpio_cfg.ir_cut2);
    printk("[GPIO CFG] SENSOR Reset  (%d)\n",gk_all_gpio_cfg.sensor_reset);
    printk("[GPIO CFG] PHY Reset     (%d)\n",gk_all_gpio_cfg.phy_reset);
    printk("[GPIO CFG] PHY Speed Led (%d)\n",gk_all_gpio_cfg.phy_speed_led);
    printk("[GPIO CFG] SPI0 EN       (%d)\n",gk_all_gpio_cfg.spi0_en0);
    printk("[GPIO CFG] SPI1 EN       (%d)\n",gk_all_gpio_cfg.spi1_en0);
    printk("[GPIO CFG] USB HOST      (%d)\n",gk_all_gpio_cfg.usb_host);
    printk("[GPIO CFG] SD Detect     (%d)\n",gk_all_gpio_cfg.sd_detect);
    printk("[GPIO CFG] SD Power      (%d)\n",gk_all_gpio_cfg.sd_power);
    printk("[GPIO]#############################\n");
#endif

    goke_init_gpio();

    for(index=0; index < gpio_count; index++)
    {
        if(gk_all_gpio_cfg.gpio_chip[index].type != GPIO_TYPE_UNDEFINED)
        {
            gk_gpio_func_config(gk_all_gpio_cfg.gpio_chip[index].pin, gk_all_gpio_cfg.gpio_chip[index].type);
        }
    }

	if(cmdline_phytype == 0)
	{
		gpio_count = intphy_gpio_count;
	    for(index=0; index < gpio_count; index++)
	    {
	        if(gk_all_gpio_cfg.int_phy_gpio[index].type != GPIO_TYPE_UNDEFINED)
	        {
	            gk_gpio_func_config(gk_all_gpio_cfg.int_phy_gpio[index].pin, gk_all_gpio_cfg.int_phy_gpio[index].type);
	        }
	    }
	}
	else
	{
		gpio_count = extphy_gpio_count;
	    for(index=0; index < gpio_count; index++)
	    {
	        if(gk_all_gpio_cfg.ext_phy_gpio[index].type != GPIO_TYPE_UNDEFINED)
	        {
	            gk_gpio_func_config(gk_all_gpio_cfg.ext_phy_gpio[index].pin, gk_all_gpio_cfg.ext_phy_gpio[index].type);
	        }
	    }
	}

    for(i=0;i<CONFIG_GK_GPIO_INSTANCES;i++)
    {
        gk_gpio_writel(gk_gpio_insts[i].base_bus + REG_GPIO_INT_EN_OFFSET, 0x00000001);
    }

	return retval;
}

