/*
 * arch/arm/mach-gk/generic/sd.c
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
#include <linux/irq.h>
#include <linux/moduleparam.h>
#include <linux/mmc/host.h>
#include <linux/module.h>


#include <mach/hardware.h>
#include <mach/sd.h>
#include <mach/rct.h>
#include <hal/hal.h>
#include <mach/io.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"gk_config."

extern u64 gk_dmamask;

/* ==========================================================================*/
typedef void (*mmc_dc_fn)(struct mmc_host *host, unsigned long delay);

static DEFINE_SPINLOCK(gk_sd0_int_lock);
static u32 gk_sd0_int = 0;
static u32 gk_sdio0_int = 0;
#ifdef CONFIG_GK710XS_SDIO2
static DEFINE_SPINLOCK(gk_sd1_int_lock);
static u32 gk_sd1_int = 0;
static u32 gk_sdio1_int = 0;
#endif

/*----------------------------------------------------------------------------*/
/* registers                                                                  */
/*----------------------------------------------------------------------------*/

/* ==========================================================================*/
struct resource gk_sd0_resources[] = {
	[0] = {
		.start	= GK_VA_SDC,
		.end	= GK_VA_SDC + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SD_IRQ,
		.end	= SD_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

void gk_sd0_slot1_request(void)
{
	return;
}

void gk_sd0_slot1_release(void)
{
	//fio_unlock(SELECT_FIO_SD);
	return;
}

#if (SD_HAS_INTERNAL_MUXER == 1)
void gk_sd_slot2_request(void)
{
	//fio_select_lock(SELECT_FIO_SDIO);
	return;
}

void gk_sd_slot2_release(void)
{
	//fio_unlock(SELECT_FIO_SDIO);
	return;
}
#endif

int gk_sd0_is_enable(void)
{
    return 1;
}

void gk_sd0_set_int(u32 mask, u32 on)
{
    unsigned long				flags;
	u32					int_flag;

	spin_lock_irqsave(&gk_sd0_int_lock, flags);
	int_flag = gk_sd_readw(SD_NISEN_REG);
    int_flag |= (gk_sd_readw(SD_EISEN_REG) << 16);
	if (on)
		int_flag |= mask;
	else
		int_flag &= ~mask;
	gk_sd0_int = int_flag;
	if (gk_sd0_is_enable()) {
		gk_sd_writew(SD_NISEN_REG, int_flag);
		gk_sd_writew(SD_EISEN_REG, int_flag >> 16);
		gk_sd_writew(SD_NIXEN_REG, int_flag);
		gk_sd_writew(SD_EIXEN_REG, int_flag >> 16);
	}
	spin_unlock_irqrestore(&gk_sd0_int_lock, flags);
    return;
}

int gk_sdio_is_enable(void)
{
    return 1;
}

void gk_sdio_set_int(u32 mask, u32 on)
{
    unsigned long				flags;
	u32					int_flag;

	spin_lock_irqsave(&gk_sd0_int_lock, flags);
	int_flag = gk_sd_readw(SD_NISEN_REG);
    int_flag |= (gk_sd_readw(SD_EISEN_REG) << 16);
	if (on)
		int_flag |= mask;
	else
		int_flag &= ~mask;
	gk_sdio0_int = int_flag;
	if (gk_sdio_is_enable()) {
		gk_sd_writew(SD_NISEN_REG, int_flag);
		gk_sd_writew(SD_EISEN_REG, int_flag >> 16);
		gk_sd_writew(SD_NIXEN_REG, int_flag);
		gk_sd_writew(SD_EIXEN_REG, int_flag >> 16);
	}
	spin_unlock_irqrestore(&gk_sd0_int_lock, flags);
}

struct gk_sd_controller gk_sd_controller0 = {
#if (SD_HAS_INTERNAL_MUXER == 1)
	.num_slots		= 1,
#else
	.num_slots		= 1,
#endif
	.slot[0] = {
		.pmmc_host	= NULL,

		.default_caps	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE |
				MMC_CAP_BUS_WIDTH_TEST,
		.active_caps	= 0,
		.default_caps2	= 0,
		.active_caps2	= 0,
		.private_caps	= GK_SD_PRIVATE_CAPS_DTO_BY_SDCLK,

		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.fixed_cd	= -1,
#if (SD_HAS_INTERNAL_MUXER == 1)
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= SDCD_IRQ,
			.irq_type	= IRQ_TYPE_EDGE_BOTH,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_TYPE_INPUT_SD_CD_N,
		},
#else
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
		},
#endif
		.cd_delay	= 100,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_HIGH,
			.active_delay	= 1,
		},

		.check_owner	= gk_sd0_is_enable,
		.request	= gk_sd0_slot1_request,
		.release	= gk_sd0_slot1_release,
		.set_int	= gk_sd0_set_int,
		.set_vdd	= NULL,
		.set_bus_timing	= NULL,
	},
#if (SD_HAS_INTERNAL_MUXER == 1)
	.slot[1] = {
		.pmmc_host	= NULL,

		.default_caps	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE |
				MMC_CAP_BUS_WIDTH_TEST,
		.active_caps	= 0,
		.default_caps2	= 0,
		.active_caps2	= 0,
		.private_caps	= GK_SD_PRIVATE_CAPS_DTO_BY_SDCLK,

		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.fixed_cd	= -1,
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
		},
		.cd_delay	= 100,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_HIGH,
			.active_delay	= 1,
		},

		.check_owner	= gk_sdio_is_enable,
		.request	= gk_sd_slot2_request,
		.release	= gk_sd_slot2_release,
		.set_int	= gk_sdio_set_int,
		.set_vdd	= NULL,
		.set_bus_timing	= NULL,
	},
#endif
	.set_pll		= set_sd_rct,
	.get_pll		= get_sd_freq_hz,

	.max_blk_mask   = SD_BLK_SZ_64KB, /*SD_BLK_SZ_128KB*/
	.max_clock		= 48000000,
	.active_clock   = 0,
	.wait_tmo		= ((5 * HZ) / 2),
	.pwr_delay		= 1,

	.dma_fix		= 0,
#if (SD_SUPPORT_PLL_SCALER == 1)
	.support_pll_scaler	= 1,
#else
	.support_pll_scaler	= 0,
#endif
};


struct platform_device gk_sd0 = {
	.name		= "gk-sd",
	.id		    = 0,
	.resource	= gk_sd0_resources,
	.num_resources	= ARRAY_SIZE(gk_sd0_resources),
	.dev		= {
		.platform_data		= &gk_sd_controller0,
		.dma_mask		    = &gk_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#ifdef CONFIG_GK710XS_SDIO2
struct resource gk_sd1_resources[] = {
	[0] = {
		.start	= GK_VA_SDC1,
		.end	= GK_VA_SDC1 + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SD1_IRQ,
		.end	= SD1_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

void gk_sd1_slot1_request(void)
{
	return;
}

void gk_sd1_slot1_release(void)
{
	//fio_unlock(SELECT_FIO_SD);
	return;
}

#if (SD_HAS_INTERNAL_MUXER == 1)
void gk_sd1_slot2_request(void)
{
	//fio_select_lock(SELECT_FIO_SDIO);
	return;
}

void gk_sd1_slot2_release(void)
{
	//fio_unlock(SELECT_FIO_SDIO);
	return;
}
#endif

int gk_sd1_is_enable(void)
{
    return 1;
}

void gk_sd1_set_int(u32 mask, u32 on)
{
    unsigned long				flags;
	u32					int_flag;

	spin_lock_irqsave(&gk_sd1_int_lock, flags);
	int_flag = gk_sd_readw(SD1_NISEN_REG);
    int_flag |= (gk_sd_readw(SD1_EISEN_REG) << 16);
	if (on)
		int_flag |= mask;
	else
		int_flag &= ~mask;
	gk_sd1_int = int_flag;
	if (gk_sd1_is_enable()) {
		gk_sd_writew(SD1_NISEN_REG, int_flag);
		gk_sd_writew(SD1_EISEN_REG, int_flag >> 16);
		gk_sd_writew(SD1_NIXEN_REG, int_flag);
		gk_sd_writew(SD1_EIXEN_REG, int_flag >> 16);
	}
	spin_unlock_irqrestore(&gk_sd1_int_lock, flags);
    return;
}

int gk_sdio2_is_enable(void)
{
    return 1;
}

void gk_sdio2_set_int(u32 mask, u32 on)
{
    unsigned long				flags;
	u32					int_flag;

	spin_lock_irqsave(&gk_sd1_int_lock, flags);
	int_flag = gk_sd_readw(SD1_NISEN_REG);
    int_flag |= (gk_sd_readw(SD1_EISEN_REG) << 16);
	if (on)
		int_flag |= mask;
	else
		int_flag &= ~mask;
	gk_sdio1_int = int_flag;
	if (gk_sdio2_is_enable()) {
		gk_sd_writew(SD1_NISEN_REG, int_flag);
		gk_sd_writew(SD1_EISEN_REG, int_flag >> 16);
		gk_sd_writew(SD1_NIXEN_REG, int_flag);
		gk_sd_writew(SD1_EIXEN_REG, int_flag >> 16);
	}
	spin_unlock_irqrestore(&gk_sd1_int_lock, flags);
}

struct gk_sd_controller gk_sd1_controller0 = {
#if (SD_HAS_INTERNAL_MUXER == 1)
	.num_slots		= 1,
#else
	.num_slots		= 1,
#endif
	.slot[0] = {
		.pmmc_host	= NULL,

		.default_caps	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE |
				MMC_CAP_BUS_WIDTH_TEST,
		.active_caps	= 0,
		.default_caps2	= 0,
		.active_caps2	= 0,
		.private_caps	= GK_SD_PRIVATE_CAPS_DTO_BY_SDCLK,

		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.fixed_cd	= -1,
#if (SD_HAS_INTERNAL_MUXER == 1)
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= SD1CD_IRQ,
			.irq_type	= IRQ_TYPE_EDGE_BOTH,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_HW,
		},
#else
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_TYPE_INPUT_SD_CD_N,
		},
#endif
		.cd_delay	= 100,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_HIGH,
			.active_delay	= 1,
		},

		.check_owner	= gk_sd1_is_enable,
		.request	= gk_sd1_slot1_request,
		.release	= gk_sd1_slot1_release,
		.set_int	= gk_sd1_set_int,
		.set_vdd	= NULL,
		.set_bus_timing	= NULL,
	},
#if (SD_HAS_INTERNAL_MUXER == 1)
	.slot[1] = {
		.pmmc_host	= NULL,

		.default_caps	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_SDIO_IRQ |
				MMC_CAP_ERASE |
				MMC_CAP_BUS_WIDTH_TEST,
		.active_caps	= 0,
		.default_caps2	= 0,
		.active_caps2	= 0,
		.private_caps	= GK_SD_PRIVATE_CAPS_DTO_BY_SDCLK,

		.ext_power	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.ext_reset	= {
			.gpio_id	= -1,
			.active_level	= GPIO_LOW,
			.active_delay	= 1,
		},
		.fixed_cd	= -1,
		.gpio_cd	= {
			.irq_gpio	= -1,
			.irq_line	= -1,
			.irq_type	= -1,
			.irq_gpio_val	= GPIO_LOW,
			.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
		},
		.cd_delay	= 100,
		.fixed_wp	= -1,
		.gpio_wp	= {
			.gpio_id	= -1,
			.active_level	= GPIO_HIGH,
			.active_delay	= 1,
		},

		.check_owner	= gk_sdio2_is_enable,
		.request	= gk_sd1_slot2_request,
		.release	= gk_sd1_slot2_release,
		.set_int	= gk_sdio2_set_int,
		.set_vdd	= NULL,
		.set_bus_timing	= NULL,
	},
#endif
	.set_pll		= set_sd_rct,
	.get_pll		= get_sd_freq_hz,

	.max_blk_mask   = SD_BLK_SZ_64KB, /*SD_BLK_SZ_128KB*/
	.max_clock		= 24000000,
	.active_clock   = 0,
	.wait_tmo		= ((5 * HZ) / 2),
	.pwr_delay		= 1,

	.dma_fix		= 0,
#if (SD_SUPPORT_PLL_SCALER == 1)
	.support_pll_scaler	= 1,
#else
	.support_pll_scaler	= 0,
#endif
};


struct platform_device gk_sd1 = {
	.name		= "gk-sd",
	.id		    = 1,
	.resource	= gk_sd1_resources,
	.num_resources	= ARRAY_SIZE(gk_sd1_resources),
	.dev		= {
		.platform_data		= &gk_sd1_controller0,
		.dma_mask		    = &gk_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif
typedef union { /* SDIO_Control01Reg */
    u8 all;
    struct {
        u8 datatimeoutcountervalue     : 4;
        u8                             : 4;
    } bitc;
} GH_SDIO_TMO_S;

typedef union { /* SDIO_Control01Reg */
    u8 all;
    struct {
        u8 softwareresetcmdline        : 1;
        u8 softwareresetall            : 1;
        u8 softwareresetdatline        : 1;
        u8                             : 5;
    } bitc;
} GH_SDIO_RST_S;

typedef union { /* SDIO_Control01Reg */
    u16 all;
    struct {
        u16 internalclken               : 1;
        u16 internalclkstable           : 1;
        u16 sdclken                     : 1;
        u16                             : 5;
        u16 sdclkfreselect              : 8;
    } bitc;
} GH_SDIO_CLK_S;

typedef union { /* SDIO_Control00Reg */
    u8 all;
    struct {
        u8 ledcontrol                  : 1;
        u8 datatrawidth                : 1;
        u8 sd8bitmode                  : 1;
        u8 hostspeeden                 : 1;
        u8                             : 2;
        u8 carddetecttestlevel         : 1;
        u8 carddetectsigdet            : 1;
    } bitc;
} GH_SDIO_HOST_S;

typedef union { /* SDIO_Control00Reg */
    u8 all;
    struct {
        u8 sdbuspower                  : 1;
        u8 sdbusvoltageselect          : 3;
        u8                             : 4;
    } bitc;
} GH_SDIO_PWR_S;

void GH_SDIO_set_Control01Reg_SoftwareResetAll(u8 data)
{
    GH_SDIO_RST_S d;
	d.all = gk_sd_readb(SD_RESET_REG);
    d.bitc.softwareresetall = data;
	gk_sd_writeb(SD_RESET_REG, d.all);
}

void GH_SDIO_set_Control01Reg_SoftwareResetCmdLine(u8 data)
{
    GH_SDIO_RST_S d;
	d.all = gk_sd_readb(SD_RESET_REG);
    d.bitc.softwareresetcmdline = data;
	gk_sd_writeb(SD_RESET_REG, d.all);
}

void GH_SDIO_set_Control01Reg_SoftwareResetDatLine(u8 data)
{
    GH_SDIO_RST_S d;
	d.all = gk_sd_readb(SD_RESET_REG);
    d.bitc.softwareresetdatline = data;
	gk_sd_writeb(SD_RESET_REG, d.all);
}

void GH_SDIO_set_Control01Reg_SdClkEn(u8 data)
{
    GH_SDIO_CLK_S d;
	d.all = gk_sd_readw(SD_CLK_REG);
    d.bitc.sdclken = data;
	gk_sd_writew(SD_CLK_REG, d.all);
}

void GH_SDIO_set_Control01Reg_SdclkFreSelect(u8 data)
{
    GH_SDIO_CLK_S d;
	d.all = gk_sd_readw(SD_CLK_REG);
    d.bitc.sdclkfreselect = data;
	gk_sd_writew(SD_CLK_REG, d.all);
}

void GH_SDIO_set_Control01Reg_InternalClkEn(u8 data)
{
    GH_SDIO_CLK_S d;
	d.all = gk_sd_readw(SD_CLK_REG);
    d.bitc.internalclken = data;
	gk_sd_writew(SD_CLK_REG, d.all);
}

u16  GH_SDIO_get_Control01Reg(void)
{
    u16 value = gk_sd_readw(SD_CLK_REG);

    return value;
}

void GH_SDIO_set_Control00Reg_SdBusVoltageSelect(u8 data)
{
    GH_SDIO_PWR_S d;
	d.all = gk_sd_readb(SD_PWR_REG);
    d.bitc.sdbusvoltageselect = data;
	gk_sd_writeb(SD_PWR_REG, d.all);
}

u32  GH_SDIO_get_CapReg(void)
{
	u32 value = gk_sd_readl(SD_CAP_REG);
    return value;
}

void GH_SDIO_set_Control00Reg_SdBusPower(u8 data)
{
    GH_SDIO_PWR_S d;
	d.all = gk_sd_readb(SD_PWR_REG);
    d.bitc.sdbuspower = data;
	gk_sd_writeb(SD_PWR_REG, d.all);
}

void GH_SDIO_set_Control01Reg_DataTimeoutCounterValue(u8 data)
{
    GH_SDIO_TMO_S d;
	d.all = gk_sd_readb(SD_TMO_REG);
    d.bitc.datatimeoutcountervalue = data;
	gk_sd_writeb(SD_TMO_REG, d.all);
}

void GH_SDIO_set_Control00Reg_HostSpeedEn(u8 data)
{
    GH_SDIO_HOST_S d;
	d.all = gk_sd_readb(SD_HOST_REG);
    d.bitc.hostspeeden = data;
	gk_sd_writeb(SD_HOST_REG, d.all);
}

void GH_SDIO_set_Control00Reg_DataTraWidth(u8 data)
{
    GH_SDIO_HOST_S d;
	d.all = gk_sd_readb(SD_HOST_REG);
    d.bitc.datatrawidth = data;
	gk_sd_writeb(SD_HOST_REG, d.all);
}

void GH_SDIO_set_NorIntStatusReg(u16 data)
{
	gk_sd_writew(SD_NIS_REG, data);
}

u16  GH_SDIO_get_NorIntStatusReg(void)
{
	u16 value = gk_sd_readw(SD_NIS_REG);
    return value;
}

void GH_SDIO_set_ErrIntStatusReg(u16 data)
{
	gk_sd_writew(SD_EIS_REG, data);
}

u16  GH_SDIO_get_ErrIntStatusReg(void)
{
	u16 value = gk_sd_readw(SD_EIS_REG);
    return value;
}

void GH_SDIO_set_NorIntStatusEnReg(u16 data)
{
    gk_sd_writew(SD_NISEN_REG, data);
}

void GH_SDIO_set_ErrIntStatusEnReg(u16 data)
{
    gk_sd_writew(SD_EISEN_REG, data);
}

void GH_SDIO_set_NorIntSigEnReg(u16 data)
{
	gk_sd_writew(SD_NIXEN_REG, data);
}

void GH_SDIO_set_ErrIntSigEnReg(u16 data)
{
	gk_sd_writew(SD_EIXEN_REG, data);
}

#ifdef CONFIG_GK710XS_SDIO2
void GH_SDIO1_set_Control01Reg_SoftwareResetAll(u8 data)
{
    GH_SDIO_RST_S d;
	d.all = gk_sd_readb(SD1_RESET_REG);
    d.bitc.softwareresetall = data;
	gk_sd_writeb(SD1_RESET_REG, d.all);
}

void GH_SDIO1_set_Control01Reg_SoftwareResetCmdLine(u8 data)
{
    GH_SDIO_RST_S d;
	d.all = gk_sd_readb(SD1_RESET_REG);
    d.bitc.softwareresetcmdline = data;
	gk_sd_writeb(SD1_RESET_REG, d.all);
}

void GH_SDIO1_set_Control01Reg_SoftwareResetDatLine(u8 data)
{
    GH_SDIO_RST_S d;
	d.all = gk_sd_readb(SD1_RESET_REG);
    d.bitc.softwareresetdatline = data;
	gk_sd_writeb(SD1_RESET_REG, d.all);
}

void GH_SDIO1_set_Control01Reg_SdClkEn(u8 data)
{
    GH_SDIO_CLK_S d;
	d.all = gk_sd_readw(SD1_CLK_REG);
    d.bitc.sdclken = data;
	gk_sd_writew(SD1_CLK_REG, d.all);
}

void GH_SDIO1_set_Control01Reg_SdclkFreSelect(u8 data)
{
    GH_SDIO_CLK_S d;
	d.all = gk_sd_readw(SD1_CLK_REG);
    d.bitc.sdclkfreselect = data;
	gk_sd_writew(SD1_CLK_REG, d.all);
}

void GH_SDIO1_set_Control01Reg_InternalClkEn(u8 data)
{
    GH_SDIO_CLK_S d;
	d.all = gk_sd_readw(SD1_CLK_REG);
    d.bitc.internalclken = data;
	gk_sd_writew(SD1_CLK_REG, d.all);
}

u16  GH_SDIO1_get_Control01Reg(void)
{
    u16 value = gk_sd_readw(SD1_CLK_REG);

    return value;
}

void GH_SDIO1_set_Control00Reg_SdBusVoltageSelect(u8 data)
{
    GH_SDIO_PWR_S d;
	d.all = gk_sd_readb(SD1_PWR_REG);
    d.bitc.sdbusvoltageselect = data;
	gk_sd_writeb(SD1_PWR_REG, d.all);
}

u32  GH_SDIO1_get_CapReg(void)
{
	u32 value = gk_sd_readl(SD1_CAP_REG);
    return value;
}

void GH_SDIO1_set_Control00Reg_SdBusPower(u8 data)
{
    GH_SDIO_PWR_S d;
	d.all = gk_sd_readb(SD1_PWR_REG);
    d.bitc.sdbuspower = data;
	gk_sd_writeb(SD1_PWR_REG, d.all);
}

void GH_SDIO1_set_Control01Reg_DataTimeoutCounterValue(u8 data)
{
    GH_SDIO_TMO_S d;
	d.all = gk_sd_readb(SD1_TMO_REG);
    d.bitc.datatimeoutcountervalue = data;
	gk_sd_writeb(SD1_TMO_REG, d.all);
}

void GH_SDIO1_set_Control00Reg_HostSpeedEn(u8 data)
{
    GH_SDIO_HOST_S d;
	d.all = gk_sd_readb(SD1_HOST_REG);
    d.bitc.hostspeeden = data;
	gk_sd_writeb(SD1_HOST_REG, d.all);
}

void GH_SDIO1_set_Control00Reg_DataTraWidth(u8 data)
{
    GH_SDIO_HOST_S d;
	d.all = gk_sd_readb(SD1_HOST_REG);
    d.bitc.datatrawidth = data;
	gk_sd_writeb(SD1_HOST_REG, d.all);
}

void GH_SDIO1_set_NorIntStatusReg(u16 data)
{
	gk_sd_writew(SD1_NIS_REG, data);
}

u16  GH_SDIO1_get_NorIntStatusReg(void)
{
	u16 value = gk_sd_readw(SD1_NIS_REG);
    return value;
}

void GH_SDIO1_set_ErrIntStatusReg(u16 data)
{
	gk_sd_writew(SD1_EIS_REG, data);
}

u16  GH_SDIO1_get_ErrIntStatusReg(void)
{
	u16 value = gk_sd_readw(SD1_EIS_REG);
    return value;
}

void GH_SDIO1_set_NorIntStatusEnReg(u16 data)
{
    gk_sd_writew(SD1_NISEN_REG, data);
}

void GH_SDIO1_set_ErrIntStatusEnReg(u16 data)
{
    gk_sd_writew(SD1_EISEN_REG, data);
}

void GH_SDIO1_set_NorIntSigEnReg(u16 data)
{
	gk_sd_writew(SD1_NIXEN_REG, data);
}

void GH_SDIO1_set_ErrIntSigEnReg(u16 data)
{
	gk_sd_writew(SD1_EIXEN_REG, data);
}

void gk_set_sd1_detect_pin(u32 gpio_pin)
{
    gk_sd1_controller0.slot[0].gpio_cd.irq_gpio = gpio_pin;
}

#endif

static void sdioSoftReset(u8 index)
{
    if(index == 0)
    {
        GH_SDIO_set_Control01Reg_SoftwareResetAll(1);
        GH_SDIO_set_Control01Reg_SoftwareResetCmdLine(1);
        GH_SDIO_set_Control01Reg_SoftwareResetDatLine(1);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index ==1)
    {
        GH_SDIO1_set_Control01Reg_SoftwareResetAll(1);
        GH_SDIO1_set_Control01Reg_SoftwareResetCmdLine(1);
        GH_SDIO1_set_Control01Reg_SoftwareResetDatLine(1);
    }
#endif
}

static void sdioClockOnOff(u8 index, u32 on)
{
    if(index == 0)
    {
        if (on == 0)
        {
            GH_SDIO_set_Control01Reg_SdClkEn(0);
        }
        else
        {
            GH_SDIO_set_Control01Reg_SdClkEn(1);
        }
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        if (on == 0)
        {
            GH_SDIO1_set_Control01Reg_SdClkEn(0);
        }
        else
        {
            GH_SDIO1_set_Control01Reg_SdClkEn(1);
        }
    }
#endif
}

static void sdioSelectVoltage(u8 index)
{
    u32 caps;
    if(index == 0)
    {
        caps=GH_SDIO_get_CapReg();

        if(caps & 0x1<<24)
        {
            GH_SDIO_set_Control00Reg_SdBusVoltageSelect(0x7);
            GH_SDIO_set_Control00Reg_SdBusPower(1);

        }
        else if(caps & 0x1<<25)
        {
            GH_SDIO_set_Control00Reg_SdBusVoltageSelect(0x6);
            GH_SDIO_set_Control00Reg_SdBusPower(1);
        }
        else if(caps & 0x1<<26)
        {
            GH_SDIO_set_Control00Reg_SdBusVoltageSelect(0x5);
            GH_SDIO_set_Control00Reg_SdBusPower(1);
        }
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        caps=GH_SDIO1_get_CapReg();

        if(caps & 0x1<<24)
        {
            GH_SDIO1_set_Control00Reg_SdBusVoltageSelect(0x7);
            GH_SDIO1_set_Control00Reg_SdBusPower(1);

        }
        else if(caps & 0x1<<25)
        {
            GH_SDIO1_set_Control00Reg_SdBusVoltageSelect(0x6);
            GH_SDIO1_set_Control00Reg_SdBusPower(1);
        }
        else if(caps & 0x1<<26)
        {
            GH_SDIO1_set_Control00Reg_SdBusVoltageSelect(0x5);
            GH_SDIO1_set_Control00Reg_SdBusPower(1);
        }
    }
#endif
}

static void sdioSetClockDiv(u8 index, u8 div)
{
    if(index == 0)
    {
        GH_SDIO_set_Control01Reg_SdclkFreSelect(div);
        GH_SDIO_set_Control01Reg_InternalClkEn(1);

        while(1)
        {
            if((GH_SDIO_get_Control01Reg() & (0x1<<1)))
                break;
        }
        sdioClockOnOff(index, 1);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        GH_SDIO1_set_Control01Reg_SdclkFreSelect(div);
        GH_SDIO1_set_Control01Reg_InternalClkEn(1);

        while(1)
        {
            if((GH_SDIO1_get_Control01Reg() & (0x1<<1)))
                break;
        }
        sdioClockOnOff(index, 1);
    }
#endif
}

static void sdioSetTimeoutControl(u8 index, u8 timeout)
{
    if(index == 0)
    {
        GH_SDIO_set_Control01Reg_DataTimeoutCounterValue(timeout);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        GH_SDIO1_set_Control01Reg_DataTimeoutCounterValue(timeout);
    }
#endif
}

static void sdioSetHostctlSpeed(u8 index, u8 mode)
{
    if(index == 0)
    {
        GH_SDIO_set_Control00Reg_HostSpeedEn(mode);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        GH_SDIO1_set_Control00Reg_HostSpeedEn(mode);
    }
#endif
}

static void sdioSetHostctlWidth(u8 index, u8 mode)
{
    if(index == 0)
    {
        GH_SDIO_set_Control00Reg_DataTraWidth(mode);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        GH_SDIO1_set_Control00Reg_DataTraWidth(mode);
    }
#endif
}

#define SDIO_INT_STATUS_EN  (1 | 1<<1 | 1<<3 | 1<<4 |1<<5 | 1<<6 | 1<<7)
#define SDIO_INT_SIG_EN     (1 | 1<<1 | 1<<3 | 1<<4 |1<<5 | 1<<6 | 1<<7)
static void sdioEnableIntStatus(u8 index)
{
    /*clear int*/
    if(index == 0)
    {
        if(GH_SDIO_get_NorIntStatusReg() & (0x1 << 15))
        {
            GH_SDIO_set_ErrIntStatusReg(GH_SDIO_get_ErrIntStatusReg());
        }

        GH_SDIO_set_NorIntStatusEnReg(0x0);
        GH_SDIO_set_NorIntStatusEnReg(SDIO_INT_STATUS_EN);
        GH_SDIO_set_ErrIntStatusEnReg(0xffff);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        if(GH_SDIO1_get_NorIntStatusReg() & (0x1 << 15))
        {
            GH_SDIO1_set_ErrIntStatusReg(GH_SDIO1_get_ErrIntStatusReg());
        }

        GH_SDIO1_set_NorIntStatusEnReg(0x0);
        GH_SDIO1_set_NorIntStatusEnReg(SDIO_INT_STATUS_EN);
        GH_SDIO1_set_ErrIntStatusEnReg(0xffff);
    }
#endif
}

static void sdioEnableIntSig(u8 index)
{
    if(index == 0)
    {
        GH_SDIO_set_NorIntSigEnReg(0);
        GH_SDIO_set_NorIntSigEnReg(SDIO_INT_SIG_EN);
        GH_SDIO_set_ErrIntSigEnReg(0xffff);
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        GH_SDIO1_set_NorIntSigEnReg(0);
        GH_SDIO1_set_NorIntSigEnReg(SDIO_INT_SIG_EN);
        GH_SDIO1_set_ErrIntSigEnReg(0xffff);
    }
#endif
}

static void GD_SDIO_Rest(u8 index)
{
    sdioSoftReset(index);
    sdioClockOnOff(index, 0);
    sdioSetClockDiv(index, 0x00);
    sdioSelectVoltage(index);
    sdioSetTimeoutControl(index, 0xe);
    sdioSetHostctlSpeed(index, 0);
    sdioSetHostctlWidth(index, 1);
    /*clear interrupt status*/
    if(index == 0)
    {
        GH_SDIO_set_NorIntStatusReg(GH_SDIO_get_NorIntStatusReg());
        GH_SDIO_set_ErrIntStatusReg(GH_SDIO_get_ErrIntStatusReg());
    }
#ifdef CONFIG_GK710XS_SDIO2
    else if(index == 1)
    {
        GH_SDIO1_set_NorIntStatusReg(GH_SDIO1_get_NorIntStatusReg());
        GH_SDIO1_set_ErrIntStatusReg(GH_SDIO1_get_ErrIntStatusReg());
    }
#endif
    sdioEnableIntStatus(index);
    sdioEnableIntSig(index);
}

int __init gk_init_sd(void)
{
	int retval = 0;

    GD_SDIO_Rest(0);
#ifdef CONFIG_GK710XS_SDIO2
    GD_SDIO_Rest(1);
#endif
	return retval;
}

void gk_set_sd_detect_pin(u32 gpio_pin)
{
    gk_sd_controller0.slot[0].gpio_cd.irq_gpio = gpio_pin;
}

void gk_detect_sd_slot(int bus, int slot, int fixed_cd)
{
	struct gk_sd_slot		*pslotinfo = NULL;
	mmc_dc_fn				mmc_dc = NULL;

	printk("%s:%d[%d:%d:%d]\n", __func__, __LINE__, bus, slot, fixed_cd);
	if ((bus == 0) && (slot == 0)) {
		pslotinfo = &gk_sd_controller0.slot[0];
	}
#if (SD_HAS_INTERNAL_MUXER == 1)
	if ((bus == 0) && (slot == 1)) {
		pslotinfo = &gk_sd_controller0.slot[1];
	}
#endif


#if defined(CONFIG_MMC)
	mmc_dc = mmc_detect_change;
#else
#if defined(CONFIG_KALLSYMS)
	mmc_dc = (mmc_dc_fn)module_kallsyms_lookup_name("mmc_detect_change");
#endif
#endif
	if (pslotinfo && mmc_dc && pslotinfo->pmmc_host) {
		pslotinfo->fixed_cd = fixed_cd;
		mmc_dc(pslotinfo->pmmc_host, pslotinfo->cd_delay);
		printk("%s:%d[%p:%p:%p:%d]\n", __func__, __LINE__, pslotinfo,
			mmc_dc, pslotinfo->pmmc_host, pslotinfo->cd_delay);
	}
}
EXPORT_SYMBOL(gk_detect_sd_slot);

#ifdef CONFIG_GK710XS_SDIO2
void gk_detect_sd1_slot(int bus, int slot, int fixed_cd)
{
	struct gk_sd_slot   *pslotinfo = NULL;
	mmc_dc_fn				mmc_dc = NULL;

	printk("%s:%d[%d:%d:%d]\n", __func__, __LINE__, bus, slot, fixed_cd);
	if ((bus == 0) && (slot == 0)) {
		pslotinfo = &gk_sd1_controller0.slot[0];
	}
#if (SD_HAS_INTERNAL_MUXER == 1)
	if ((bus == 0) && (slot == 1)) {
		pslotinfo = &gk_sd1_controller0.slot[1];
	}
#endif


#if defined(CONFIG_MMC)
	mmc_dc = mmc_detect_change;
#else
#if defined(CONFIG_KALLSYMS)
	mmc_dc = (mmc_dc_fn)module_kallsyms_lookup_name("mmc_detect_change");
#endif
#endif
	if (pslotinfo && mmc_dc && pslotinfo->pmmc_host) {
		pslotinfo->fixed_cd = fixed_cd;
		mmc_dc(pslotinfo->pmmc_host, pslotinfo->cd_delay);
		printk("%s:%d[%p:%p:%p:%d]\n", __func__, __LINE__, pslotinfo,
			mmc_dc, pslotinfo->pmmc_host, pslotinfo->cd_delay);
	}
}
EXPORT_SYMBOL(gk_detect_sd1_slot);
#endif


