/*
 * This header provides constants for binding novatek,na*-gpio.
 *
 * The first cell in Novetek's GPIO specifier is the GPIO ID. The macros below
 * provide names for this.
 *
 * The second cell contains standard flag values specified in gpio.h.
 */

#ifndef _DT_BINDINGS_GPIO_NVT_GPIO_H
#define _DT_BINDINGS_GPIO_NVT_GPIO_H

#include <dt-bindings/gpio/gpio.h>

/* na51055 platform */
#define C_GPIO(pin)			(pin)
#define P_GPIO(pin)			(pin + 0x20)
#define S_GPIO(pin)			(pin + 0x40)
#define L_GPIO(pin)			(pin + 0x60)
#define D_GPIO(pin)			(pin + 0x80)
#define H_GPIO(pin)			(pin + 0xA0)
#define A_GPIO(pin)			(pin + 0xC0)
#define DSI_GPIO(pin)			(pin + 0xE0)

/* na51068 platform */
#define GPIO_0(pin)			(pin)
#define GPIO_1(pin)			(pin + 0x20)
#define GPIO_2(pin)			(pin + 0x40)
#define GPIO_3(pin)			(pin + 0x60)

#define GPIO_RISING_EDGE		0x1
#define GPIO_FALLING_EDGE		0x0

#define GPIO_INTERRUPT		0x1
#define GPIO_POLLING		0x0

#define GPIO_HIGH		0x1
#define GPIO_LOW		0x0
#endif
