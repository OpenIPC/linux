/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LOTUS_PLATFORM_H__
#define __LOTUS_PLATFORM_H__

#include <linux/kconfig.h>

#ifdef CONFIG_ARCH_XMFALCON
#include <../xmfalcon/include/platform.h>
#endif

#ifdef CONFIG_ARCH_XMORCA
#include <../xmorca/include/platform.h>
#endif

#endif /* End of __LOTUS_PLATFORM_H__ */
