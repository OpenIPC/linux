/*
 * linux/arch/arm/mach-gk/mach-gk.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#include CONFIG_GK_CHIP_INCLUDE_FILE

#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/usb/musb.h>
#include <linux/memory_hotplug.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/mtd/partitions.h>
#include <linux/pm.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/ctype.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>
#include <asm/hardware/vic.h>

#include <mach/hardware.h>
#include <mach/flash.h>
#include <mach/irqs.h>
#include <mach/timer.h>
#include <mach/uart.h>
#include <mach/gpio.h>
#include <mach/eth.h>
#include <mach/idc.h>
#include <mach/rct.h>
#include <mach/sd.h>
#include <mach/ir.h>
#include <plat/audio.h>
#include <mach/i2s.h>
#include <plat/dma.h>
#include <mach/spi.h>
#include <mach/io.h>
#include <mach/audio_codec.h>

extern struct platform_device gk_fb0;
extern struct platform_device gk_fb1;
extern int gk_init_fb(void);

extern struct platform_device gk_sd0;

extern struct i2c_board_info gk_board_vi_infos[2];
extern struct i2c_board_info gk_board_hdmi_infos[2];

//u64 gk_dmamask = DMA_BIT_MASK(32);
//EXPORT_SYMBOL(gk_dmamask);

/* ==========================================================================*/

#if defined(CONFIG_MTD_SFLASH_GOKE) || defined(CONFIG_MTD_SFLASH_GOKE_MODULE)


#if defined(CONFIG_ONLY_USE_NOR_8M)
#define GKAPP_MTDBLOCK0_UBOOT_BIN_OFFSET                 \
                (0)
#define GKAPP_MTDBLOCK0_UBOOT_BIN_SIZE                   \
                (256 * 1024)

#define GKAPP_MTDBLOCK1_UBOOT_ENV_OFFSET                 \
                (GKAPP_MTDBLOCK0_UBOOT_BIN_OFFSET        \
               + GKAPP_MTDBLOCK0_UBOOT_BIN_SIZE)
#define GKAPP_MTDBLOCK1_UBOOT_ENV_SIZE                   \
                (64 * 1024)

#define GKAPP_MTDBLOCK2_USER_OFFSET                      \
                (GKAPP_MTDBLOCK1_UBOOT_ENV_OFFSET        \
               + GKAPP_MTDBLOCK1_UBOOT_ENV_SIZE)
#define GKAPP_MTDBLOCK2_USER_SIZE                        \
                (128 * 1024)

#define GKAPP_MTDBLOCK3_CUSTOM_OFFSET                    \
                (GKAPP_MTDBLOCK2_USER_OFFSET             \
               + GKAPP_MTDBLOCK2_USER_SIZE)
#define GKAPP_MTDBLOCK3_CUSTOM_SIZE                      \
                (512 * 1024)

#define GKAPP_MTDBLOCK4_KERNEL_OFFSET                    \
                (GKAPP_MTDBLOCK3_CUSTOM_OFFSET           \
               + GKAPP_MTDBLOCK3_CUSTOM_SIZE)
#define GKAPP_MTDBLOCK4_KERNEL_SIZE                      \
                ((1024 + 320) * 1024)

#define GKAPP_MTDBLOCK5_ROOTFS_OFFSET                    \
                (GKAPP_MTDBLOCK4_KERNEL_OFFSET           \
               + GKAPP_MTDBLOCK4_KERNEL_SIZE)
#define GKAPP_MTDBLOCK5_ROOTFS_SIZE                      \
                ((4096 + 0) * 1024)

#define GKAPP_MTDBLOCK6_RESOURCE_OFFSET                  \
                (GKAPP_MTDBLOCK5_ROOTFS_OFFSET           \
               + GKAPP_MTDBLOCK5_ROOTFS_SIZE)
#define GKAPP_MTDBLOCK6_RESOURCE_SIZE                    \
                ((1024 + 704 + 64) * 1024)

// for 8M nor flash
static struct mtd_partition gk_flash_parts[]=
{
#if 0
    [0] = {
        .name   = "uboot",
        .offset = GKAPP_MTDBLOCK0_UBOOT_BIN_OFFSET,
        .size   = GKAPP_MTDBLOCK0_UBOOT_BIN_SIZE,
    },
    [1] = {
        .name   = "uboot_env",
        .offset = GKAPP_MTDBLOCK1_UBOOT_ENV_OFFSET,
        .size   = GKAPP_MTDBLOCK1_UBOOT_ENV_SIZE,
    },
    [2] = {/* for usrm of Juan */
        .name   = "user",
        .offset = GKAPP_MTDBLOCK2_USER_OFFSET,
        .size   = GKAPP_MTDBLOCK2_USER_SIZE,
    },
    [3] = {/* mount custom.jffs2 /media/conf */
        .name   = "conf",
        .offset = GKAPP_MTDBLOCK3_CUSTOM_OFFSET,
        .size   = GKAPP_MTDBLOCK3_CUSTOM_SIZE,
    },
    [4] = {
        .name   = "kernel",
        .offset = GKAPP_MTDBLOCK4_KERNEL_OFFSET,
        .size   = GKAPP_MTDBLOCK4_KERNEL_SIZE,
    },
    [5] = {/* ro squashfs rootfs */
        .name   = "rootfs",
        .offset = GKAPP_MTDBLOCK5_ROOTFS_OFFSET,
        .size   = GKAPP_MTDBLOCK5_ROOTFS_SIZE,
    },
    [6] = {/* mount font&web squashfs to /media/custom */
        .name   = "resource",
        .offset = GKAPP_MTDBLOCK6_RESOURCE_OFFSET,
        .size   = GKAPP_MTDBLOCK6_RESOURCE_SIZE,
    },
#else
#if 1//defined(CONFIG_MACH_GK7102_HZD_OJTV_v10_JXH42) || defined(CONFIG_MACH_GK_HZD_OJTV_v10_OV2710)
   //256K
    [0] = {
        .name   = "uboot",
        .offset = 0x00000000,
        .size   = 0x00040000,
    },
    //64K
    [1] = {
        .name   = "ubootenv",
        .offset = 0x00040000,
        .size   = 0x00010000,
    },
    //128k
    [2] = {
        .name   = "user",
        .offset = 0x00050000,
        .size   = 0x00020000,
    },
    //448k
    [3] = {
        .name   = "config",
        .offset = 0x00070000,
        .size   = 0x00070000,
    },
    //1344k
    [4] = {
        .name   = "kernel",
        .offset = 0x000e0000,
        .size   = 0x001A0000,
    },
    //4M
    [5] = {
        .name   = "rootfs",
        .offset = 0x00280000,
        .size   = 0x003b0000,
    },
    //1856k
    [6] = {
        .name   = "resource",
        .offset = 0x00630000,
        .size   = 0x001d0000,
    },
    //ALL 0--8M
    [7] = {
        .name   = "all",
        .offset = 0x00000000,
        .size   = 0x00800000,
    },
#else
    //256K
    [0] = {
        .name   = "uboot",
        .offset = 0x00000000,
        .size   = 0x00040000,
    },
    //64K
    [1] = {
        .name   = "ubootenv",
        .offset = 0x00040000,
        .size   = 0x00010000,
    },
    //128k
    [2] = {
        .name   = "user",
        .offset = 0x00050000,
        .size   = 0x00020000,
    },
    //448k
    [3] = {
        .name   = "config",
        .offset = 0x00070000,
        .size   = 0x00070000,
    },
    //1344k
    [4] = {
        .name   = "kernel",
        .offset = 0x000e0000,
        .size   = 0x00150000,
    },
    //4M
    [5] = {
        .name   = "rootfs",
        .offset = 0x00230000,
        .size   = 0x00400000,
    },
    //1856k
    [6] = {
        .name   = "resource",
        .offset = 0x00630000,
        .size   = 0x001d0000,
    },
    //ALL 0--8M
    [7] = {
        .name   = "all",
        .offset = 0x00000000,
        .size   = 0x00800000,
    },
    #endif
#endif
};
#elif defined(CONFIG_ONLY_USE_NOR_16M)
// for 16M nor flash
static struct mtd_partition gk_flash_parts[]=
{
    //256K
    [0] = {
        .name   = "uboot",
        .offset = 0x00000000,
        .size   = 0x00040000,
    },
    //64K
    [1] = {
        .name   = "ubootenv",
        .offset = 0x00040000,
        .size   = 0x00010000,
    },
    // 2M
    [2] = {
        .name   = "kernel",
        .offset = 0x00050000,
        .size   = 0x00200000,
    },
    //13M + 512K
    [3] = {
        .name   = "rootfs",
        .offset = 0x00250000,
        .size   = 0x00DB0000,
    },

};
#elif defined(CONFIG_ONLY_USE_NOR_32M)
// for 32M nor flash
static struct mtd_partition gk_flash_parts[]=
{
    //256K
    [0] = {
        .name   = "uboot",
        .offset = 0x00000000,
        .size   = 0x00040000,
    },
    //64K
    [1] = {
        .name   = "ubootenv",
        .offset = 0x00040000,
        .size   = 0x00010000,
    },
    //768K
    [2] = {
        .name   = "config",
        .offset = 0x00050000,
        .size   = 0x000c0000,
    },
    // 2.5M
    [3] = {
        .name   = "kernel",
        .offset = 0x00110000,
        .size   = 0x00280000,
    },
    //27M
    [4] = {
        .name   = "rootfs",
        .offset = 0x00390000,
        .size   = 0x01b00000,
    },
    // 1.5M
    [5] = {
        .name   = "resource",
        .offset = 0x01e90000,
        .size   = 0x00170000,
    },
};
#elif defined(CONFIG_USE_NOR_AND_NAND)
// for 8M nor flash
static struct mtd_partition gk_flash_parts[]=
{
    //192K
    [0] = {
        .name   = "uboot",
        .offset = 0x00000000,
        .size   = 0x00040000,
    },
    //64K
    [1] = {
        .name   = "ubootenv",
        .offset = 0x00040000,
        .size   = 0x00010000,
    },
    //512K
    [2] = {
        .name   = "config",
        .offset = 0x00050000,
        .size   = 0x00080000,
    },
    // 1.25M
    [3] = {
        .name   = "kernel",
        .offset = 0x000D0000,
        .size   = 0x00140000,
    },
    //5M
    [4] = {
        .name   = "rootfs",
        .offset = 0x00210000,
        .size   = 0x00500000,
    },
    // 1M
    [5] = {
        .name   = "resource",
        .offset = 0x00710000,
        .size   = 0x000E0000,
    },
};
#elif defined(CONFIG_ONLY_USE_NAND)
// for 8M nor flash
static struct mtd_partition gk_flash_parts[]=
{
    //192K
    [0] = {
        .name   = "uboot",
        .offset = 0x00000000,
        .size   = 0x00040000,
    },
    //64K
    [1] = {
        .name   = "ubootenv",
        .offset = 0x00040000,
        .size   = 0x00010000,
    },
    //512K
    [2] = {
        .name   = "config",
        .offset = 0x00050000,
        .size   = 0x00080000,
    },
    // 1.25M
    [3] = {
        .name   = "kernel",
        .offset = 0x000D0000,
        .size   = 0x00140000,
    },
    //5M
    [4] = {
        .name   = "rootfs",
        .offset = 0x00210000,
        .size   = 0x00500000,
    },
    // 1M
    [5] = {
        .name   = "resource",
        .offset = 0x00710000,
        .size   = 0x000E0000,
    },
};
#else

#endif

static struct sflash_platform_data flash_platform_data =
{
    .speed_mode = (uint32_t)SYSTEM_SFLASH_FREQ,
    .channel    = 0,
    .nr_parts   = ARRAY_SIZE(gk_flash_parts),
    .parts      = gk_flash_parts,
};

static struct platform_device gk_flash_device0 =
{
    .name = "gk_flash",
    .id   = 0,
    .dev =
    {
        .platform_data = &flash_platform_data,
    },
};
#endif/*defined(CONFIG_MTD_GK_SFLASH) || defined(CONFIG_MTD_GK_SFLASH_MODULE)*/

/* ================     Watch Dog Timer     ======================*/
#ifdef CONFIG_WATCHDOG_GOKE
static struct resource gk_wdt_resource[] = {
    [0] = DEFINE_RES_MEM(GK_VA_WDT, SZ_4K),
    [1] = DEFINE_RES_IRQ(WDT_IRQ),
};

struct platform_device gk_wdt = {
    .name       = "gk-wdt",
    .id         = -1,
    .resource   = gk_wdt_resource,
    .num_resources  = ARRAY_SIZE(gk_wdt_resource),
    .dev        = {
        .dma_mask       = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
#endif /* CONFIG_WATCHDOG_GOKE */
/* =================               End            ======================*/

/* ================     Inter Integrated Circuit     ===================*/

#ifdef CONFIG_I2C_GOKE
static struct resource gk_idc_resources[] = {
    [0] = DEFINE_RES_MEM(GK_VA_IDC, SZ_4K),
    [1] = DEFINE_RES_IRQ(IDC_IRQ),
};

struct gk_platform_i2c gk_idc_platform_info = {
    .clk_limit      = 400000,
    .bulk_write_num = 60,
    .get_clock      = get_apb_bus_freq_hz,
};
IDC_PARAM_CALL(0, gk_idc_platform_info, 0644);

struct platform_device gk_idc = {
    .name           = "i2c",
    .id             = 0,
    .resource       = gk_idc_resources,
    .num_resources  = ARRAY_SIZE(gk_idc_resources),
    .dev            = {
        .platform_data      = &gk_idc_platform_info,
        .dma_mask           = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};

#if (IDC_INSTANCES >= 2)
static struct resource gk_idc_hdmi_resources[] = {
    [0] = DEFINE_RES_MEM(GK_VA_IDC2, SZ_4K),
    [1] = DEFINE_RES_IRQ(IDC_HDMI_IRQ),
};

struct gk_platform_i2c gk_idc_hdmi_platform_info = {
    .clk_limit      = 400000,
    .bulk_write_num = 60,
    .i2c_class      = I2C_CLASS_DDC,
    .get_clock      = get_apb_bus_freq_hz,
};
IDC_PARAM_CALL(1, gk_idc_hdmi_platform_info, 0644);

struct platform_device gk_idc_hdmi = {
    .name           = "i2c",
    .id             = 1,
    .resource       = gk_idc_hdmi_resources,
    .num_resources  = ARRAY_SIZE(gk_idc_hdmi_resources),
    .dev            = {
        .platform_data      = &gk_idc_hdmi_platform_info,
        .dma_mask           = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
#endif
#endif

#ifdef CONFIG_SPI_GOKE
/* ==========================================================================*/
void gk_spi_cs_activate(struct gk_spi_cs_config *cs_config)
{
    u8            cs_pin;

    if (cs_config->bus_id >= SPI_MASTER_INSTANCES ||
        cs_config->cs_id >= cs_config->cs_num)
        return;
    cs_pin = cs_config->cs_pins[cs_config->cs_id];
    gk_gpio_set_out(cs_pin, 0);
}

void gk_spi_cs_deactivate(struct gk_spi_cs_config *cs_config)
{
    u8            cs_pin;

    if (cs_config->bus_id >= SPI_MASTER_INSTANCES ||
        cs_config->cs_id >= cs_config->cs_num)
        return;

    cs_pin = cs_config->cs_pins[cs_config->cs_id];
    gk_gpio_set_out(cs_pin, 1);
}

struct resource gk_spi0_resources[] = {
    [0] = {
        .start  = GK_VA_SSI1,
        .end    = GK_VA_SSI1 + 0x0FFF,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .start  = SSI_IRQ,
        .end    = SSI_IRQ,
        .flags  = IORESOURCE_IRQ,
    },
};

int gk_spi0_cs_pins[] = {-1, -1, -1, -1, -1, -1, -1, -1};
GK_SPI_PARAM_CALL(0, gk_spi0_cs_pins, 0644);
struct gk_spi_platform_info gk_spi0_platform_info = {
    .support_dma        = 0,
#if (SPI_MASTER_INSTANCES == 5 )
    .fifo_entries        = 64,
#else
    .fifo_entries        = 16,
#endif
    .cs_num             = ARRAY_SIZE(gk_spi0_cs_pins),
    .cs_pins            = gk_spi0_cs_pins,
    .cs_activate        = gk_spi_cs_activate,
    .cs_deactivate      = gk_spi_cs_deactivate,
//    .rct_set_ssi_pll    = rct_set_ssi_pll,
    .get_ssi_freq_hz    = get_ssi0_freq_hz,
};

struct platform_device gk_spi0 = {
    .name       = "spi",
    .id         = 0,
    .resource   = gk_spi0_resources,
    .num_resources    = ARRAY_SIZE(gk_spi0_resources),
    .dev        = {
        .platform_data        = &gk_spi0_platform_info,
        .dma_mask        = &gk_dmamask,
        .coherent_dma_mask    = DMA_BIT_MASK(32),
    }
};

#if (SPI_MASTER_INSTANCES >= 2 )
struct resource gk_spi1_resources[] = {
    [0] = {
        .start  = GK_VA_SSI2,
        .end    = GK_VA_SSI2 + 0x0FFF,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .start  = SSI2_IRQ,
        .end    = SSI2_IRQ,
        .flags  = IORESOURCE_IRQ,
    },
};

int gk_spi1_cs_pins[] = {-1, -1, -1, -1, -1, -1, -1, -1};
GK_SPI_PARAM_CALL(1, gk_spi1_cs_pins, 0644);
struct gk_spi_platform_info gk_spi1_platform_info = {
    .support_dma        = 0,
    .fifo_entries       = 16,
    .cs_num             = ARRAY_SIZE(gk_spi1_cs_pins),
    .cs_pins            = gk_spi1_cs_pins,
    .cs_activate        = gk_spi_cs_activate,
    .cs_deactivate      = gk_spi_cs_deactivate,
    .get_ssi_freq_hz    = get_ssi1_freq_hz,
};

struct platform_device gk_spi1 = {
    .name       = "spi",
    .id         = 1,
    .resource   = gk_spi1_resources,
    .num_resources    = ARRAY_SIZE(gk_spi1_resources),
    .dev        = {
        .platform_data      = &gk_spi1_platform_info,
        .dma_mask           = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
#endif
#endif
static struct spi_board_info gk_spi_devices[] = {
    {
#ifdef CONFIG_MMC_SPI
        .modalias       = "mmc_spi",
        .max_speed_hz   = 69000000/4,     /* max spi clock (SCK) speed in HZ */
        .bus_num        = 0,
        .mode = SPI_MODE_3,
        //.platform_data = &bfin_mmc_spi_pdata,
        //.controller_data = &mmc_spi_chip_info,
#else
        .modalias       = "spidev",
#endif
        .bus_num        = 0,
        .chip_select    = 0,
    },
#if (SPI_INSTANCES >= 2)
    {
        .modalias       = "spidev",
        .bus_num        = 1,
        .chip_select    = 0,
    }
#endif
};

/* =================               rtc            ======================*/
struct platform_device gk_rtc = {
    .name           = "gk-rtc",
    .id             = -1,
};
/* =================               End            ======================*/

/* =================               ir            ======================*/

struct gk_ir_controller gk_platform_ir_controller0 = {

    .protocol       = GK_IR_PROTOCOL_NEC,
    .debug          = 0,
};
GK_IR_PARAM_CALL(gk_platform_ir_controller0, 0644);

struct platform_device gk_ir = {
    .name       = "ir",
    .id         = 1,
    .dev        = {
        .platform_data      = &gk_platform_ir_controller0,
        .dma_mask           = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};


/* =================               pcm/i2s            ======================*/

struct platform_device gk_pcm0 = {
    .name       = "gk-pcm-audio",
    .id         = -1,
};

static struct resource gk_i2s0_resources[] = {
    [0] = {
        .start  = I2S_BASE,
        .end    = I2S_BASE + 0x0FFF,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .start  = I2STX_IRQ,
        .end    = I2SRX_IRQ,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct gk_i2s_controller gk_i2s_controller0 = {
    .aucodec_digitalio_0    = NULL,
    .aucodec_digitalio_1    = NULL,
    .aucodec_digitalio_2    = NULL,
    .channel_select         = NULL,
    .set_audio_pll          = NULL,
};


struct platform_device gk_i2s0 = {
    .name       = "gk-i2s",
    .id         = 0,
    .resource   = gk_i2s0_resources,
    .num_resources    = ARRAY_SIZE(gk_i2s0_resources),
    .dev        = {
        .platform_data      = &gk_i2s_controller0,
        .dma_mask           = &gk_dmamask,
        .coherent_dma_mask  = DMA_BIT_MASK(32),
    }
};
/* =================     musb   ====================*/
static struct musb_hdrc_config gk_musb_config = {
    .multipoint = 1,
    .dyn_fifo   = 0,
    .big_endian = 0,
    .dma        = 0,
    .num_eps    = 8,
    .ram_bits   = 12,
};

static struct musb_hdrc_platform_data gk_musb_data = {
#if defined(CONFIG_GK_USB_HOST_MODE)
	.mode                                              = MUSB_HOST,
#elif defined(CONFIG_GK_USB_SLAVE_MODE)
	.mode                                              = MUSB_PERIPHERAL,
#elif defined(CONFIG_GK_USB_OTG_MODE)
    .mode                                              = MUSB_HOST,
#endif
    .min_power  = 25,     /* x2 = 50 mA drawn from VBUS as peripheral */
    .config     = &gk_musb_config,
};

static struct resource gk_musb_resources[] = {
        /* Order is significant!  The start/end fields
         * are updated during setup..
         */
    [0] = {
        .start  = GK_VA_USB,
        .end    = GK_VA_USB + 0x2000,
        .flags  = IORESOURCE_MEM,
    },
/*
    [1] = {
        .start  = GK_VA_USB_PHY,
        .end    = GK_VA_USB_PHY + 0xa00,
        .flags  = IORESOURCE_MEM,
    },
*/
    [1] = {
        .start  = USBC_IRQ,
        .end    = USBC_IRQ,
        .flags  = IORESOURCE_IRQ,
        .name   = "mc",
    },
};


static struct platform_device gk_musb = {
        .name           = "musb-gk",
        .id             = -1,
        .dev = {
            .platform_data      = &gk_musb_data,
            .dma_mask           = &gk_dmamask,
            .coherent_dma_mask  = DMA_BIT_MASK(32),
        },
        .num_resources  = ARRAY_SIZE(gk_musb_resources),
        .resource       = gk_musb_resources,
};


/* =================               End            ======================*/
unsigned int gk_aud_get_dma_offset(void)
{
    return AUDIO_DMA_REG_OFFSET;
}
EXPORT_SYMBOL(gk_aud_get_dma_offset);


static struct platform_device *gk_devices[] __initdata = {

#ifdef CONFIG_MMC_GOKE
#ifndef CONFIG_PHY_USE_SD_CLK
    &gk_sd0,
#endif
#endif
#ifdef CONFIG_WATCHDOG_GOKE
    &gk_wdt,
#endif
#ifdef CONFIG_I2C_GOKE
    &gk_idc,
#if (IDC_INSTANCES >= 2)
    &gk_idc_hdmi,
#endif
#endif

#ifdef CONFIG_FB_GOKE
    &gk_fb0,
    &gk_fb1,
#endif
#ifdef CONFIG_SPI_GOKE
    &gk_spi0,
#if (SPI_MASTER_INSTANCES >= 2 )
    &gk_spi1,
#endif
#endif
#ifdef CONFIG_RTC_DRV_GOKE
    &gk_rtc,
#endif
#ifdef CONFIG_INPUT_GOKE_IR
    &gk_ir,
#endif

#ifdef CONFIG_ETH_GOKE
    &gk_eth0,
#endif
    //&gk_pcm0,
    //&gk_i2s0,
    &gk_musb,
};

//louis add
/* ==========================================================================*/

unsigned int gk_debug_level = GK_DEBUG_MEDIA | GK_DEBUG_DSP | GK_DEBUG_VI | GK_DEBUG_VO | GK_DEBUG_AAA;
EXPORT_SYMBOL(gk_debug_level);

unsigned int gk_debug_info = 0;
EXPORT_SYMBOL(gk_debug_info);

unsigned int gk_boot_splash_logo = 0;
EXPORT_SYMBOL(gk_boot_splash_logo);

unsigned long gk_debug_lookup_name(const char *name)
{
    return module_kallsyms_lookup_name(name);
}
EXPORT_SYMBOL(gk_debug_lookup_name);
//end

void sensor_init(u8 active_level)
{
    struct gk_gpio_io_info  sensor_reset;
    sensor_reset.gpio_id = gk_all_gpio_cfg.sensor_reset;
    sensor_reset.active_level = active_level;
    sensor_reset.active_delay = 1;

    printk("sensor board reset...\n");
    gk_set_gpio_output(&sensor_reset, 1);
    mdelay(50);//100ms
    gk_set_gpio_output(&sensor_reset, 0);
    mdelay(200);//100ms
}
EXPORT_SYMBOL(sensor_init);

void sensor_power(u8 power)
{
}
EXPORT_SYMBOL(sensor_power);

/* ==========================================================================*/
static u8 cmdline_mac[6];
static int __init init_setup_mac(char *str)
{
    int count, i, val;

    for (count = 0; count < 6 && *str; count++, str += 3) {
        if (!isxdigit(str[0]) || !isxdigit(str[1]))
            return 0;
        if (str[2] != ((count < 5) ? ':' : '\0'))
            return 0;

        for (i = 0, val = 0; i < 2; i++) {
            val = val << 4;
            val |= isdigit(str[i]) ?
                str[i] - '0' : toupper(str[i]) - 'A' + 10;
        }
        cmdline_mac[count] = val;
    }
    return 1;

}
__setup("mac=", init_setup_mac);

u8 cmdline_phytype;
static int __init init_setup_phytype(char *str)
{
    if (!isxdigit(str[0]))
        return 0;

    cmdline_phytype = str[0] - '0';
    return 1;
}
__setup("phytype=", init_setup_phytype);
EXPORT_SYMBOL(cmdline_phytype);

int __init gk_init_board(char *board_name)
{
#ifdef CONFIG_ETH_GOKE
    unsigned char   mac_addr[6] = {00,0x11,0x22,0xa3,0xa0,00};
#endif
    int             retval = 0;

    //pr_info("Goke %s:\n", board_name);
    //pr_info("\tboard revision:\t\t%d\n", 1);

    //retval = gk_init_pll();
    //BUG_ON(retval != 0);
    //
    printk("init timer...\n");
    retval = gk_init_timer();
    BUG_ON(retval != 0);

    printk("init gpio...\n");
    retval = gk_init_gpio();
    BUG_ON(retval != 0);

    gk_set_sd_detect_pin(gk_all_gpio_cfg.sd_detect);
#ifdef CONFIG_SPI_GOKE
    gk_spi0_cs_pins[0] = gk_all_gpio_cfg.spi0_en0;
#if (SPI_MASTER_INSTANCES >= 2 )
    gk_spi1_cs_pins[0] = gk_all_gpio_cfg.spi1_en0;
#endif
#endif


#ifdef CONFIG_ETH_GOKE
    gk_set_phy_reset_pin(gk_all_gpio_cfg.phy_reset);
  #if (ETH_INSTANCES >= 1)
      if (cmdline_mac[0])
          memcpy(mac_addr, cmdline_mac, 6);
    mac_addr[0] &= 0xfe;    /* clear multicast bit */
    mac_addr[0] |= 0x02;    /* set local assignment bit (IEEE802) */
    retval = gk_init_eth0(mac_addr);
    BUG_ON(retval != 0);
  #endif
#endif

    retval = gk_init_dma();
    BUG_ON(retval != 0);

    //config video DAC for CVBS
    gk_vout_writel( GK_VA_VEDIO_DAC + 0x00, 0x01);
    gk_vout_writel( GK_VA_VEDIO_DAC + 0x04, 0x01);
    gk_vout_writel( GK_VA_VEDIO_DAC + 0x08, 0x00);
    gk_vout_writel( GK_VA_VEDIO_DAC + 0x10, 0x01);
    gk_vout_writel( GK_VA_VEDIO_DAC + 0x14, 0x01);
    gk_vout_writel( GK_VA_VEDIO_DAC + 0x18, 0x3F);

    gk_init_sd();

#ifdef CONFIG_PMU_ALWAYS_RUNNING
    // get pmu controller
    gk_gpio_func_config(gk_all_gpio_cfg.pmu_ctl, GPIO_TYPE_OUTPUT_1);
    msleep(1);
    gk_gpio_func_config(gk_all_gpio_cfg.pmu_ctl, GPIO_TYPE_OUTPUT_0);
    msleep(1);
    gk_load_51mcu_code(PMU_ALWAYS_RUNNING);
#endif
    return retval;

}

static void __init gk_init_machine(void)
{
    int i;

    //clkdev_init();
    gk_init_board("GK_chip");

#if defined(CONFIG_MTD_SFLASH_GOKE) || defined(CONFIG_MTD_SFLASH_GOKE_MODULE)
    platform_device_register(&gk_flash_device0);
#endif

    gk_create_proc_dir();

#ifdef CONFIG_FB_GOKE
    gk_init_fb();
#endif

    printk("gk register devices %d\n", ARRAY_SIZE(gk_devices));

    /* Register devices */
    platform_add_devices(gk_devices, ARRAY_SIZE(gk_devices));
    for (i = 0; i < ARRAY_SIZE(gk_devices); i++) {
        device_set_wakeup_capable(&gk_devices[i]->dev, 1);
        device_set_wakeup_enable(&gk_devices[i]->dev, 0);
    }

    printk("gk register I2C\n");
    //i2c_register_board_info(0, &gk_ak4642_board_info, 1);
    i2c_register_board_info(0, gk_board_vi_infos, ARRAY_SIZE(gk_board_vi_infos));
    i2c_register_board_info(1, gk_board_hdmi_infos, ARRAY_SIZE(gk_board_hdmi_infos));

    spi_register_board_info(gk_spi_devices,
        ARRAY_SIZE(gk_spi_devices));

    pm_power_off = gk_power_off;

}

MACHINE_START(GOKE_IPC, "Goke IPC Board")
    .map_io         = gk_map_io,
    .init_irq       = gk_init_irq,
    .handle_irq     = gk_vic_handle_irq,
    .timer          = &gk_sys_timer,
    .init_machine   = gk_init_machine,
    .restart        = gk_restart,
MACHINE_END


