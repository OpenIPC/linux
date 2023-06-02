#include "dispdev_platform.h"

#if defined __UITRON || defined __ECOS
#elif defined __FREERTOS
unsigned int dispdev_debug_level = NVT_DBG_WRN;
#else
#endif

PINMUX_LCDINIT dispdev_platform_get_disp_mode(UINT32 pin_func_id)
{
#if defined __UITRON || defined __ECOS
	return pinmux_getDispMode((PINMUX_FUNC_ID)pin_func_id);
#else
	return pinmux_get_dispmode((PINMUX_FUNC_ID) pin_func_id);
#endif
}

void dispdev_platform_set_pinmux(UINT32 pin_func_id, UINT32 pinmux)
{
#if defined __UITRON || defined __ECOS
	pinmux_setPinmux(pin_func_id, PINMUX_LCD_SEL_GPIO);
#else
	pinmux_set_config((PINMUX_FUNC_ID) pin_func_id, pinmux);
#endif
}

void dispdev_platform_delay_ms(UINT32 ms)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayMs(ms);
#elif defined __FREERTOS
	delay_us(ms * 1000);
#else
	msleep(ms);
	//mdelay(ms);
#endif
}

void dispdev_platform_delay_us(UINT32 us)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayUs(us);
#elif defined __FREERTOS
	delay_us(us);
#else
	ndelay(1000 * us);
#endif
}

UINT32 dispdev_platform_request_gpio(UINT32 id, CHAR *str)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return 0;
#else
	return gpio_request(id, str);
#endif
}

void dispdev_platform_set_gpio_ouput(UINT32 id, BOOL high)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	gpio_setDir(id, GPIO_DIR_OUTPUT);
	if (high)
		gpio_setPin(id);
	else
		gpio_clearPin(id);
#else
	gpio_direction_output(id, high);
#endif
}

void dispdev_platform_set_gpio_input(UINT32 id)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	gpio_setDir(id, GPIO_DIR_INPUT);
#else
	gpio_direction_input(id);
#endif
}

void dispdev_platform_free_gpio(UINT32 id)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
#else
	gpio_free(id);
#endif
}

#if defined __KERNEL__
EXPORT_SYMBOL(dispdev_platform_get_disp_mode);
EXPORT_SYMBOL(dispdev_platform_set_pinmux);
EXPORT_SYMBOL(dispdev_platform_delay_ms);
EXPORT_SYMBOL(dispdev_platform_delay_us);
EXPORT_SYMBOL(dispdev_platform_request_gpio);
EXPORT_SYMBOL(dispdev_platform_set_gpio_ouput);
EXPORT_SYMBOL(dispdev_platform_set_gpio_input);
EXPORT_SYMBOL(dispdev_platform_free_gpio);
#endif
/*
PDISPDEV_OBJ dispdev_get_lcd1_dev_obj(void)
{
#if defined DISPLCDSEL_IF8BITS_TYPE
#if (DISPLCDSEL_IF8BITS_TYPE == DISPLCDSEL_IF8BITS_LCD1)
	return dispdev_get_lcd1_if8bits_dev_obj();
#endif

#elif defined DISPLCDSEL_IFDSI_TYPE
#if (DISPLCDSEL_IFDSI_TYPE == DISPLCDSEL_IFDSI_LCD1)
	return dispdev_get_lcd1_ifdsi_dev_obj();
#endif

#elif defined DDISPLCDSEL_IFPARAL_LCD1
#if (DISPLCDSEL_IFPARAL_TYPE == DISPLCDSEL_IFPARAL_LCD1)
#endif

#endif
}

EXPORT_SYMBOL(dispdev_get_lcd1_dev_obj);
*/
