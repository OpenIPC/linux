#include <linux/platform_device.h>

#include <linux/gpio_keys.h>
#include <linux/input.h>
#include "board_base.h"

struct gpio_keys_button __attribute__((weak)) board_buttons[] = {
#ifdef GPIO_HOME
	{
		.gpio		= GPIO_HOME,
		.code   	= KEY_HOME,
		.desc		= "home key",
		.active_low	= ACTIVE_LOW_HOME,
		.type		= EV_KEY,
#ifdef WAKEUP_HOME
		.wakeup         = WAKEUP_HOME,
#endif
#ifdef CAN_DISABLE_HOME
		.can_disable	= CAN_DISABLE_HOME,
#endif
	},
#endif /* GPIO_HOME */
#ifdef GPIO_MENU
	{
		.gpio		= GPIO_MENU,
		.code   	= KEY_MENU,
		.desc		= "menu key",
		.active_low	= ACTIVE_LOW_MENU,
		.type		= EV_KEY,
#ifdef WAKEUP_MENU
		.wakeup         = WAKEUP_MENU,
#endif
#ifdef CAN_DISABLE_MENU
		.can_disable	= CAN_DISABLE_MENU,
#endif
	},
#endif /* GPIO_MENU */
#ifdef GPIO_BACK
	{
		.gpio		= GPIO_BACK,
		.code   	= KEY_BACK,
		.desc		= "back key",
		.active_low	= ACTIVE_LOW_BACK,
		.type		= EV_KEY,
#ifdef WAKEUP_BACK
		.wakeup         = WAKEUP_BACK,
#endif
#ifdef CAN_DISABLE_BACK
		.can_disable	= CAN_DISABLE_BACK,
#endif
	},
#endif /* GPIO_BACK */
#ifdef GPIO_VOLUMEDOWN
	{
		.gpio		= GPIO_VOLUMEDOWN,
		.code   	= KEY_VOLUMEDOWN,
		.desc		= "volum down key",
		.active_low	= ACTIVE_LOW_VOLUMEDOWN,
		.type		= EV_KEY,
#ifdef WAKEUP_VOLUMEDOWN
		.wakeup         = WAKEUP_VOLUMEDOWN,
#endif
#ifdef CAN_DISABLE_VOLUMEDOWN
		.can_disable	= CAN_DISABLE_VOLUMEDOWN,
#endif
	},
#endif /* GPIO_VOLUMEDOWN */
#ifdef GPIO_VOLUMEUP
	{
		.gpio		= GPIO_VOLUMEUP,
		.code   	= KEY_VOLUMEUP,
		.desc		= "volum up key",
		.active_low	= ACTIVE_LOW_VOLUMEUP,
		.type		= EV_KEY,
#ifdef WAKEUP_VOLUMEUP
		.wakeup         = WAKEUP_VOLUMEUP,
#endif
#ifdef CAN_DISABLE_VOLUMEUP
		.can_disable	= CAN_DISABLE_VOLUMEUP,
#endif
	},
#endif /* GPIO_VOLUMEUP */
#ifdef GPIO_POWER
		{
		.gpio           = GPIO_POWER,
		.code           = KEY_POWER,
		.desc           = "power key",
		.active_low     = ACTIVE_LOW_POWER,
		.type		= EV_KEY,
#ifdef WAKEUP_POWER
		.wakeup         = WAKEUP_POWER,
#endif
#ifdef CAN_DISABLE_POWER
		.can_disable	= CAN_DISABLE_POWER,
#endif
	},
#endif /* GPIO_POWER */
#ifdef GPIO_WAKEUP
		{
		.gpio           = GPIO_WAKEUP,
		.code           = KEY_WAKEUP,
		.desc           = "wakeup key",
		.active_low     = ACTIVE_LOW_WAKEUP,
		.type		= EV_KEY,
#ifdef WAKEUP_WAKEUP
		.wakeup         = WAKEUP_WAKEUP,
#endif
#ifdef CAN_DISABLE_WAKEUP
		.can_disable	= CAN_DISABLE_WAKEUP,
#endif
	},
#endif /* GPIO_WAKEUP */
#ifdef GPIO_PIR
		{
		.gpio           = GPIO_PIR,
		.code           = KEY_F13,
		.desc           = "PIR key(key code:F13)",
		.active_low     = ACTIVE_LOW_PIR,
		.type		= EV_KEY,
#ifdef WAKEUP_PIR
		.wakeup         = WAKEUP_PIR,
#endif
#ifdef CAN_DISABLE_PIR
		.can_disable	= CAN_DISABLE_PIR,
#endif
	},
#endif /* GPIO_PIR */
};

static struct gpio_keys_platform_data board_button_data = {
	.buttons	= board_buttons,
	.nbuttons	= ARRAY_SIZE(board_buttons),
};

struct platform_device jz_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
                .platform_data	= &board_button_data,
	}
};
