/*
 * TI DaVinci DM365 EVM board support
 *
 * Copyright (C) 2009 Texas Instruments Incorporated
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/i2c/at24.h>
#include <linux/leds.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/slab.h>
#include <linux/mtd/nand.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/spi/eeprom.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/mux.h>
#include <mach/common.h>
#include <linux/platform_data/i2c-davinci.h>
#include <mach/serial.h>
#include <linux/platform_data/mmc-davinci.h>
#include <linux/platform_data/mtd-davinci.h>
#include <linux/platform_data/keyscan-davinci.h>
#include <linux/platform_data/usb-davinci.h>
#include <mach/time.h>
#include <mach/gpio.h>
#include <mach/rto.h>

#include "davinci.h"
#include "dm365_spi.h"

bool lan0_run; // LAN init flag
bool lan1_run; // EMAC LAN init flag

bool wlan_run; // WLAN init flag
bool spi0_run; // SPI0 init flag
bool camera_run; // camera init flag
bool uart1_run; // UART1 init flag

static void v2r_parse_cmdline(char * string);

static struct i2c_board_info i2c_info[] = {

};

static struct davinci_i2c_platform_data i2c_pdata = {
	.bus_freq	= 100	/* kHz */, //was 400
	.bus_delay	= 0	/* usec */,
	.sda_pin        = 21,
	.scl_pin        = 20,
};

/* Input available at the ov7690 */
static struct v4l2_input ov2643_inputs[] = {
	{
		.index = 0,
		.name = "Camera",
		.type = V4L2_INPUT_TYPE_CAMERA,
	}
};


static struct vpfe_subdev_info vpfe_sub_devs[] = {
	{
		.module_name = "ov2643",
		.is_camera = 1,
		.grp_id = VPFE_SUBDEV_OV2643,
		.num_inputs = ARRAY_SIZE(ov2643_inputs),
		.inputs = ov2643_inputs,
		.ccdc_if_params = {
#ifdef CONFIG_VIDEO_YCBCR
			.if_type = VPFE_YCBCR_SYNC_8,
#else
			.if_type = VPFE_RAW_BAYER,
#endif
			.hdpol = VPFE_PINPOL_POSITIVE,
			.vdpol = VPFE_PINPOL_POSITIVE,
		},
		.board_info = {
			I2C_BOARD_INFO("ov2643", 0x30),
			/* this is for PCLK rising edge */
			.platform_data = (void *)1,
		},
	}
};

static struct vpfe_config vpfe_cfg = {
       .num_subdevs = ARRAY_SIZE(vpfe_sub_devs),
       .sub_devs = vpfe_sub_devs,
       .card_name = "DM365 Leopard",
       .ccdc = "DM365 ISIF",
       .num_clocks = 1,
       .clocks = {"vpss_master"},
};

static void w1_enable_external_pullup(int enable);

static inline int have_imager(void)
{
#if defined(CONFIG_SOC_CAMERA_OV2643) || \
	defined(CONFIG_SOC_CAMERA_OV2643_MODULE)
	return 1;
#else
	return 0;
#endif
}

static void dm365_camera_configure(void){
	davinci_cfg_reg(DM365_CAM_OFF);
	gpio_request(98, "CAMERA_OFF");
	gpio_direction_output(98, 0);
	davinci_cfg_reg(DM365_CAM_RESET);
	gpio_request(99, "CAMERA_RESET");
	gpio_direction_output(99, 1);
	davinci_cfg_reg(DM365_GPIO37);
	davinci_cfg_reg(DM365_EXTCLK);
}

/* NOTE:  this is geared for the standard config, with a socketed
 * 2 GByte Micron NAND (MT29F16G08FAA) using 128KB sectors.  If you
 * swap chips with a different block size, partitioning will
 * need to be changed. This NAND chip MT29F16G08FAA is the default
 * NAND shipped with the Spectrum Digital DM365 EVM
 */
/*define NAND_BLOCK_SIZE		SZ_128K*/

/* For Samsung 4K NAND (K9KAG08U0M) with 256K sectors */
/*#define NAND_BLOCK_SIZE		SZ_256K*/

/* For Micron 4K NAND with 512K sectors */
#define NAND_BLOCK_SIZE		SZ_512K

#define DM365_ASYNC_EMIF_CONTROL_BASE	0x01d10000

static struct mtd_partition davinci_nand_partitions[] = {
	{
		.name		= "uboot",
		.offset		= 0,
		.size		= 0x100000,
		.mask_flags	= 0,
	},
	{
		.name		= "wtf",
		.offset		= 0x200000,
		.size		= 0x100000,
		.mask_flags	= 0,
	},
	{
		.name		= "kernel",
		.offset		= 0x300000,
		.size		= 0x300000,
		.mask_flags	= 0,
	}, {
		.name		= "rootfs",
		.offset		= 0x600000,
		.size		= 0x500000,
		.mask_flags	= 0,
	}, {
		.name		= "rootfs_data",
		.offset		= 0xb00000,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	}
};

static struct davinci_nand_pdata davinci_nand_data = {
	.mask_chipsel		= 0,
//	.parts			= davinci_nand_partitions,
//	.nr_parts		= ARRAY_SIZE(davinci_nand_partitions),
	.ecc_mode		= NAND_ECC_HW,
	.bbt_options		= NAND_BBT_USE_FLASH,
	// .ecc_bits		= 4,
};

#define DM365_ASYNC_EMIF_DATA_CE0_BASE	0x02000000
#define DM365_ASYNC_EMIF_DATA_CE1_BASE	0x04000000

static struct resource davinci_nand_resources[] = {
	{
		.start		= DM365_ASYNC_EMIF_DATA_CE0_BASE,
		.end		= DM365_ASYNC_EMIF_DATA_CE0_BASE + SZ_32M - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= DM365_ASYNC_EMIF_CONTROL_BASE,
		.end		= DM365_ASYNC_EMIF_CONTROL_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device davinci_nand_device = {
	.name			= "davinci_nand",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(davinci_nand_resources),
	.resource		= davinci_nand_resources,
	.dev			= {
		.platform_data	= &davinci_nand_data,
	},
};

static struct physmap_flash_data davinci_nor_data = {
		.width		= 2,
};

static struct resource davinci_nor_resources[] = {
	{
		.start		= DM365_ASYNC_EMIF_DATA_CE0_BASE,
		.end		= DM365_ASYNC_EMIF_DATA_CE0_BASE + SZ_16M - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= DM365_ASYNC_EMIF_CONTROL_BASE,
		.end		= DM365_ASYNC_EMIF_CONTROL_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device davinci_nor_device = {
	.name			= "physmap-flash",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(davinci_nor_resources),
	.dev			= {
		.platform_data = &davinci_nor_data,
	},
	.resource		= &davinci_nor_resources,
};

static struct snd_platform_data dm365_evm_snd_data = {
	.asp_chan_q = EVENTQ_3,
	.ram_chan_q = EVENTQ_3,
	//.sram_size_capture = 0x100000000,    /* CMEM/H.264 uses TCM from 0x1000 */
};



//SD card configuration functions
//May be used dynamically
static int mmc_get_cd(int module)
{
	//1 = card present
	//0 = card not present
	return 1;
}

static int mmc_get_ro(int module)
{
	//1 = device is read-only
	//0 = device is mot read only
	return 0;
}

static struct davinci_mmc_config dm365evm_mmc_config = {
	.get_cd		= mmc_get_cd,
	.get_ro		= mmc_get_ro,
	.wires		= 4,
	.max_freq	= 50000000,
	.caps		= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
};

static struct davinci_mmc_config dm365evm_mmc1_config = {
	//.get_cd		= mmc_get_cd,
	//.get_ro		= mmc_get_ro,
	.wires		= 4,
	.max_freq	= 50000000,
	.caps		= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED | MMC_CAP_SDIO_IRQ | MMC_BUS_WIDTH_4 | MMC_CAP_NONREMOVABLE | MMC_CAP_4_BIT_DATA,
};

static void dm365evm_emac_configure(void)
{
	/*
	 * EMAC pins are multiplexed with GPIO and UART
	 * Further details are available at the DM365 ARM
	 * Subsystem Users Guide(sprufg5.pdf) pages 125 - 127
	 */
	davinci_cfg_reg(DM365_EMAC_TX_EN);
	davinci_cfg_reg(DM365_EMAC_TX_CLK);
	davinci_cfg_reg(DM365_EMAC_COL);
	davinci_cfg_reg(DM365_EMAC_TXD3);
	davinci_cfg_reg(DM365_EMAC_TXD2);
	davinci_cfg_reg(DM365_EMAC_TXD1);
	davinci_cfg_reg(DM365_EMAC_TXD0);
	davinci_cfg_reg(DM365_EMAC_RXD3);
	davinci_cfg_reg(DM365_EMAC_RXD2);
	davinci_cfg_reg(DM365_EMAC_RXD1);
	davinci_cfg_reg(DM365_EMAC_RXD0);
	davinci_cfg_reg(DM365_EMAC_RX_CLK);
	davinci_cfg_reg(DM365_EMAC_RX_DV);
	davinci_cfg_reg(DM365_EMAC_RX_ER);
	davinci_cfg_reg(DM365_EMAC_CRS);
	davinci_cfg_reg(DM365_EMAC_MDIO);
	davinci_cfg_reg(DM365_EMAC_MDCLK);

	/*
	 * EMAC interrupts are multiplexed with GPIO interrupts
	 * Details are available at the DM365 ARM
	 * Subsystem Users Guide(sprufg5.pdf) pages 133 - 134
	 */
	davinci_cfg_reg(DM365_INT_EMAC_RXTHRESH);
	davinci_cfg_reg(DM365_INT_EMAC_RXPULSE);
	davinci_cfg_reg(DM365_INT_EMAC_TXPULSE);
	davinci_cfg_reg(DM365_INT_EMAC_MISCPULSE);

	davinci_cfg_reg(DM365_GPIO29);
	printk("reseting EMAC\n");
	gpio_request(29, "emac-reset");
	gpio_direction_output(29, 1);
	msleep(20);
	gpio_direction_output(29, 0);
	msleep(100);
	gpio_direction_output(29, 1);
}

static void dm365evm_mmc_configure(void)
{
	/*
	 * MMC/SD pins are multiplexed with GPIO and EMIF
	 * Further details are available at the DM365 ARM
	 * Subsystem Users Guide(sprufg5.pdf) pages 118, 128 - 131
	 */
	davinci_cfg_reg(DM365_SD1_CLK);
	davinci_cfg_reg(DM365_SD1_CMD);
	davinci_cfg_reg(DM365_SD1_DATA3);
	davinci_cfg_reg(DM365_SD1_DATA2);
	davinci_cfg_reg(DM365_SD1_DATA1);
	davinci_cfg_reg(DM365_SD1_DATA0);
}

__init static void dm365_usb_configure(void)
{
	davinci_cfg_reg(DM365_GPIO66);
	gpio_request(66, "usb");
	gpio_direction_output(66, 1);
	davinci_setup_usb(500, 8);
}


static void dm365_ks8851_init(void){
	gpio_request(0, "KSZ8851");
	gpio_direction_input(0);
	davinci_cfg_reg(DM365_INT_SPI3);
	davinci_cfg_reg(DM365_EVT18_SPI3_TX);
	davinci_cfg_reg(DM365_EVT19_SPI3_RX);
}

static void __init init_i2c(void)
{
	davinci_cfg_reg(DM365_GPIO20);
	gpio_request(20, "i2c-scl");
	gpio_direction_output(20, 0);
	davinci_cfg_reg(DM365_I2C_SCL);
	davinci_init_i2c(&i2c_pdata);
	i2c_register_board_info(1, i2c_info, ARRAY_SIZE(i2c_info));
}

static struct platform_device *dm365_evm_nand_devices[] __initdata = {
	&davinci_nand_device,
};

static struct platform_device *dm365_evm_nor_devices[] __initdata = {
	&davinci_nor_device,
};

static struct davinci_uart_config uart_config __initdata = {
	.enabled_uarts = (1 << 0) | (1 << 1),
};

static void __init dm365_evm_map_io(void)
{
	/* setup input configuration for VPFE input devices */
	dm365_set_vpfe_config(&vpfe_cfg);
	dm365_init();
}

static struct davinci_spi_config ksz8851_mcspi_config = {
		.io_type = SPI_IO_TYPE_DMA,
		.c2tdelay = 0,
		.t2cdelay = 0
};

static struct spi_board_info ksz8851_snl_info[] __initdata = {
	{
		.modalias	= "ks8851",
		.bus_num	= 3,
		.chip_select	= 0,
		.max_speed_hz	= 40000000,
		.controller_data = &ksz8851_mcspi_config,
		.irq		= IRQ_DM365_GPIO0
     }
};

static struct davinci_spi_unit_desc dm365_evm_spi_udesc_KSZ8851 = {
	.spi_hwunit	= 3,
	.chipsel	= BIT(0),
	.irq		= IRQ_DM365_SPIINT3_0,
	.dma_tx_chan	= 18,
	.dma_rx_chan	= 19,
	.dma_evtq	= EVENTQ_3,
	.pdata		= {
		.version 	= SPI_VERSION_1,
		.num_chipselect = 2,
		.intr_line = 0,
		.chip_sel = 0,
		.cshold_bug = 0,
		.dma_event_q	= EVENTQ_3,
	}
};

static __init void dm365_evm_init(void)
{
	struct davinci_soc_info *soc_info = &davinci_soc_info;
	struct clk *aemif_clk;

	lan0_run = 0;
	lan1_run = 1;
	wlan_run = 0;
	spi0_run = 0;
	camera_run = 0;
	uart1_run = 0;

	v2r_parse_cmdline(saved_command_line);

	aemif_clk = clk_get(NULL, "aemif");
	if (IS_ERR(aemif_clk))	return;
	clk_prepare_enable(aemif_clk);

	// NAND & NOR init
	platform_add_devices(dm365_evm_nand_devices, ARRAY_SIZE(dm365_evm_nand_devices));
	//platform_add_devices(dm365_evm_nor_devices, ARRAY_SIZE(dm365_evm_nor_devices));

	init_i2c();

	// try to init camera
	if (camera_run) dm365_camera_configure();

	// set up UART1 GPIO
	if (uart1_run) {
	    davinci_cfg_reg(DM365_UART1_RXD);
	    davinci_cfg_reg(DM365_UART1_TXD);
	}

	// try to init UARTs
	davinci_serial_init(&uart_config);

	//dm365evm_mmc_configure();

	// try to init MMC0 (microSD)
	davinci_setup_mmc(0, &dm365evm_mmc_config);

	// try to init VoiceCodec
	dm365_init_vc(&dm365_evm_snd_data);

	// try to init USB
	dm365_usb_configure();

	// try to init LAN module
	if (lan0_run) {
	    dm365_ks8851_init();
	    davinci_init_spi(&dm365_evm_spi_udesc_KSZ8851, ARRAY_SIZE(ksz8851_snl_info), ksz8851_snl_info);
	}

	dm365evm_emac_configure();

	// try to init wlan
	if (wlan_run) {
	    gpio_request(59, "wlan-pwdn");
	    gpio_direction_output(59, 1);
	    msleep(20);
	    gpio_request(59, "wlan-reset");
	    gpio_direction_output(60, 1);
	    msleep(20);
	    gpio_direction_output(60, 0);
	    msleep(1000);
	    gpio_direction_output(60, 1);
	    davinci_setup_mmc(1, &dm365evm_mmc1_config);
	}

	// set USB prioruty, 
	// see http://e2e.ti.com/support/embedded/linux/f/354/t/94930.aspx 
	// and http://e2e.ti.com/support/dsp/davinci_digital_media_processors/f/100/t/150995.aspx
	// davinci_writel(0x0, 0x1c40040); //master priority1 register1 - to set USB

	return;
}

static void v2r_parse_cmdline(char * string)
{

    char *p;
    char *temp_string;
    char *temp_param;
    char *param_name;
    char *param_value;
    printk(KERN_INFO "Parse kernel cmdline:\n");
    temp_string = kstrdup(string, GFP_KERNEL);

    do
    {
	p = strsep(&temp_string, " ");
	if (p) {
	    // split param string into two parts
	    temp_param = kstrdup(p, GFP_KERNEL);
	    param_name = strsep(&temp_param, "=");
	    if (!param_name) continue;
	    param_value = strsep(&temp_param, " ");
	    if (!param_value) continue;


	    if (!strcmp(param_name, "wifi")) {
		if (!strcmp(param_value, "on")) {
		    printk(KERN_INFO "Wi-Fi board enabled\n");
		    wlan_run = 1;
		}
	    }

	    if (!strcmp(param_name, "lan0")) {
		if (!strcmp(param_value, "on")) {
		    printk(KERN_INFO "LAN enabled\n");
		    lan0_run = 1;
		}
	    }

	    if (!strcmp(param_name, "spi0")) {
		if (!strcmp(param_value, "on")) {
		    printk(KERN_INFO "SPI0 enabled\n");
		    spi0_run = 1;
		}
	    }

	    if (!strcmp(param_name, "uart1")) {
		if (!strcmp(param_value, "on")) {
		    printk(KERN_INFO "UART1 enabled\n");
		    uart1_run = 1;
		}
	    }

	    if (!strcmp(param_name, "camera")) {
		if (!strcmp(param_value, "ov2643")) {
		    printk(KERN_INFO "Use camera OmniVision OV2643\n");
		    camera_run = 1;
		}
		if (!strcmp(param_value, "ov5642")) {
		    printk(KERN_INFO "Use camera OmniVision OV5642\n");
		    camera_run = 1;
		}
		if (!strcmp(param_value, "ov7675")) {
		    printk(KERN_INFO "Use camera OmniVision OV7675\n");
		    camera_run = 1;
		}
		if (!strcmp(param_value, "ov9710")) {
		    printk(KERN_INFO "Use camera OmniVision OV9710\n");
		    camera_run = 1;
		}
	    }

	}

    } while(p);

}

MACHINE_START(DAVINCI_DM365_EVM, "DaVinci DM365 EVM")
	.atag_offset	= 0x100,
	.nr			= 0x00000793,
	.map_io		= dm365_evm_map_io,
	.init_irq	= davinci_irq_init,
	.init_time	= davinci_timer_init,
	.init_machine	= dm365_evm_init,
	.init_late	= davinci_init_late,
	.dma_zone_size	= SZ_128M,
	.restart	= davinci_restart,
MACHINE_END
