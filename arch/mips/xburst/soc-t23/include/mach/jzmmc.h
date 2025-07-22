#ifndef __JZ_MMC_H__
#define __JZ_MMC_H__

#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
#include <soc/gpio.h>
#include <gpio.h>

#define MMC_BOOT_AREA_PROTECTED	(0x1234)	/* Can not modified the area protected */
#define MMC_BOOT_AREA_OPENED	(0x4321)	/* Can modified the area protected */

enum {
	DONTCARE = 0,
	NONREMOVABLE,
	REMOVABLE,
	MANUAL,
};

struct mmc_partition_info {
	char				name[32];
	unsigned int			saddr;
	unsigned int			len;
	int				type;
};

struct mmc_recovery_info {
	struct mmc_partition_info	*partition_info;
	unsigned int			partition_num;
	unsigned int			permission;
	unsigned int			protect_boundary;
};

struct jzmmc_pin {
	short				num;
#define LOW_ENABLE			0
#define HIGH_ENABLE			1
	short 				enable_level;
};

struct card_gpio {
	struct jzmmc_pin 		wp;
	struct jzmmc_pin 		cd;
	struct jzmmc_pin 		pwr;
	struct jzmmc_pin 		rst;
};

/**
 * struct jzmmc_platform_data is a struct which defines board MSC informations
 * @removal: This shows the card slot's type:
 *	REMOVABLE/IRREMOVABLE/MANUAL (Tablet card/Phone card/build-in SDIO).
 * @sdio_clk: SDIO device's clock can't use Low-Power-Mode.
 * @ocr_mask: This one shows the voltage that host provide.
 * @capacity: Shows the host's speed capacity and bus width.
 * @max_freq: The max freqency of mmc host.
 *
 * @recovery_info: Informations that Android recovery mode uses.
 * @gpio: Slot's gpio information including pins of write-protect, card-detect and power.
 * @pio_mode: Indicate that whether the MSC host use PIO mode.
 * @private_init: Board private initial function, mostly for SDIO devices.
 */
struct jzmmc_platform_data {
	unsigned short			removal;
	unsigned short			sdio_clk;
	unsigned int			ocr_avail;
	unsigned int			capacity;
	unsigned int			pm_flags;
	unsigned int			max_freq;
	struct mmc_recovery_info	*recovery_info;
	struct card_gpio		*gpio;
	unsigned int			pio_mode;
	int				(*private_init)(void);
	unsigned int type;
};

static void inline clk32k_control(int on_off)
{
	if (on_off) {
		jz_gpio_set_func(GPIO_PB(18), GPIO_FUNC_1);
	} else {
		jz_gpio_set_func(GPIO_PB(18), GPIO_INPUT);
	}
}
#define pwm_enable_clk32k()  clk32k_control(1)
#define pwm_disable_clk32k() clk32k_control(0)

extern int jzmmc_manual_detect(int index, int on);
extern int jzmmc_clk_ctrl(int index, int on);

#endif
