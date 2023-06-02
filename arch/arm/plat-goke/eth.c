/*
 * arch/arm/mach-gk7101/eth.c
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
#include <linux/ethtool.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>

#include <mach/eth.h>
#include <mach/gpio.h>
#include <mach/irqs.h>

extern u64 gk_dmamask;

int rct_is_eth_enabled(void)
{
    return 1;
}

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "gk_config."

/* ==========================================================================*/
static struct resource gk_eth0_resources[] = {
    [0] = DEFINE_RES_MEM(GK_VA_ETH_GMAC, SZ_8K),
    [1] = DEFINE_RES_IRQ(ETH_IRQ),
};

struct gk_eth_platform_info gk_eth0_platform_info = {
    .mac_addr       = {0, 0, 0, 0, 0, 0},
    .napi_weight        = 32,
    .watchdog_timeo     = (2 * HZ),
    .mii_id         = -1,
    .phy_supported      = SUPPORTED_10baseT_Half |
                SUPPORTED_10baseT_Full |
                SUPPORTED_100baseT_Half |
                SUPPORTED_100baseT_Full |
#if (SUPPORT_GMII == 1)
                SUPPORTED_1000baseT_Half |
                SUPPORTED_1000baseT_Full |
#endif
                SUPPORTED_Autoneg |
                SUPPORTED_MII,
    .phy_reset  = {
        .gpio_id        = -1,
        .active_level   = GPIO_HIGH,
        .active_delay   = 1,
    },
    .mii_retry_limit    = 200,
    .mii_retry_tmo      = 10,
    .default_tx_ring_size   = 32,
    .default_rx_ring_size   = 64,
    .default_dma_bus_mode   = (ETH_DMA_BUS_MODE_FB |
                ETH_DMA_BUS_MODE_PBL_32 |
                ETH_DMA_BUS_MODE_DA_RX),
    .default_dma_opmode = (ETH_DMA_OPMODE_TTC_256 |
                ETH_DMA_OPMODE_RTC_64 |
                ETH_DMA_OPMODE_FUF),
    .default_supported  = 0,
    .is_enabled     = rct_is_eth_enabled,
};
GK_ETH_PARAM_CALL(0, gk_eth0_platform_info, 0644);

int __init gk_init_eth0(const u8 *mac_addr)
{
    int                 errCode = 0;

    gk_eth0_platform_info.mac_addr[0] = mac_addr[0];
    gk_eth0_platform_info.mac_addr[1] = mac_addr[1];
    gk_eth0_platform_info.mac_addr[2] = mac_addr[2];
    gk_eth0_platform_info.mac_addr[3] = mac_addr[3];
    gk_eth0_platform_info.mac_addr[4] = mac_addr[4];
    gk_eth0_platform_info.mac_addr[5] = mac_addr[5];


    return errCode;
}

void gk_set_phy_reset_pin(u32 gpio_pin)
{
    /*set phy reset pin*/
    gk_eth0_platform_info.phy_reset.gpio_id = gpio_pin;
}


void gk_set_phy_speed_led(u32 gpio_type)
{
    gk_gpio_func_config(gk_all_gpio_cfg.phy_speed_led, gpio_type);
}

struct platform_device gk_eth0 = {
    .name       = "gk-eth",
    .id     = 0,
    .resource   = gk_eth0_resources,
    .num_resources  = ARRAY_SIZE(gk_eth0_resources),
    .dev        = {
        .platform_data      = &gk_eth0_platform_info,
        .dma_mask       = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};

