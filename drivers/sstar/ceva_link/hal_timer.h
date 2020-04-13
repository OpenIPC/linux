#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include "ms_platform.h"
#include <linux/kernel.h>
int dsp_timer_hal_init(void);

void dump_bank_register(unsigned int Bank);
#endif
