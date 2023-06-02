#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <mach/platform.h>
#include <mach/io.h>


#define __io_address(n) (IOMEM(IO_ADDRESS(n)))

#define gpio_write(x, y) writel((x), __io_address(GPIO_BASE + (y) * 4))

#endif

