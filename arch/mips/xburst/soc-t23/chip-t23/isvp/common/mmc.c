#include <linux/mmc/host.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <mach/jzmmc.h>

#include "board_base.h"

#ifdef CONFIG_JZMMC_V12_MMC0
static struct card_gpio tf_gpio = {
	.wp	    	= {GPIO_MMC_WP_N,	GPIO_MMC_WP_N_LEVEL},
	.cd			= {GPIO_MMC_CD_N,	GPIO_MMC_CD_N_LEVEL},
	.pwr		= {GPIO_MMC_PWR,	GPIO_MMC_PWR_LEVEL},
	.rst		= {GPIO_MMC_RST_N,	GPIO_MMC_RST_N_LEVEL},
};

/* common pdata for both tf_card and sdio wifi on fpga board */
struct jzmmc_platform_data tf_pdata = {
#if 1
	.removal  			= REMOVABLE,
	.sdio_clk			= 1,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA,
	.pm_flags			= 0,
	.recovery_info			= NULL,
	.gpio				= &tf_gpio,
	.max_freq                       = CONFIG_MMC0_MAX_FREQ,
#ifdef CONFIG_MMC0_PIO_MODE
	.pio_mode                       = 1,
#else
	.pio_mode                       = 0,
#endif
#else
	 .removal  			= MANUAL,
	 .sdio_clk			= 1,
	 .ocr_avail			= MMC_VDD_29_30 | MMC_VDD_30_31,
	 .capacity  			= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ,
	 .max_freq                       = CONFIG_MMC1_MAX_FREQ,
	 .recovery_info			= NULL,
	 .gpio				= NULL,
 #ifdef CONFIG_MMC1_PIO_MODE
	 .pio_mode			= 1,
 #else
	 .pio_mode			= 0,
 #endif
#ifdef CONFIG_BCM_PM_CORE
	 .private_init			= bcm_wlan_init,
#else
	 .private_init			= NULL,
#endif
#endif
};
#endif

#ifdef CONFIG_JZMMC_V12_MMC1
#ifdef CONFIG_BCM_PM_CORE
 extern int bcm_wlan_init(void);
#endif
 struct jzmmc_platform_data sdio_pdata = {
#if 0
	.removal  			= REMOVABLE,
	.sdio_clk			= 1,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA,
	.pm_flags			= 0,
	.recovery_info			= NULL,
	.gpio				= &tf_gpio,
	.max_freq                       = CONFIG_MMC0_MAX_FREQ,
#ifdef CONFIG_MMC0_PIO_MODE
	.pio_mode                       = 1,
#else
	.pio_mode                       = 0,
#endif
#else
	 .removal  			= MANUAL,
	 .sdio_clk			= 1,
	 .ocr_avail			= MMC_VDD_29_30 | MMC_VDD_30_31,
	 .capacity  			= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ,
	 .max_freq                       = CONFIG_MMC1_MAX_FREQ,
	 .recovery_info			= NULL,
	 .gpio				= NULL,
 #ifdef CONFIG_MMC1_PIO_MODE
	 .pio_mode			= 1,
 #else
	 .pio_mode			= 0,
 #endif
#ifdef CONFIG_BCM_PM_CORE
	 .private_init			= bcm_wlan_init,
#else
	 .private_init			= NULL,
#endif
#endif
 };
 #endif
