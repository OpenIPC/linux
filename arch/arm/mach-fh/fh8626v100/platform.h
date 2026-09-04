#ifndef __FH8626V100_PLATFORM_H
#define __FH8626V100_PLATFORM_H

#include <linux/init.h>

void __init fh_timer_init_no_of(unsigned int iovbase, unsigned int irqno);
void __init fh_intc_init_no_of(unsigned int iovbase);

#endif
