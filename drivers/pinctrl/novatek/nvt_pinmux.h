#ifndef __NVT_PINMUX_H__
#define __NVT_PINMUX_H__

#ifdef CONFIG_PINCTRL_NA51000
#include "na51000_pinmux.h"
#endif

#ifdef CONFIG_PINCTRL_NA51055
#include "na51055/na51055_pinmux.h"
#endif

#ifdef CONFIG_PINCTRL_NA51068
#include "na51068/top_int.h"
#include "na51068/na51068_pinmux.h"
#endif

#ifdef CONFIG_PINCTRL_NA51089
#include "na51089/na51089_pinmux.h"
#endif

#endif