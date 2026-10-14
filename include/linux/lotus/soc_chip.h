#ifndef SOC_CHIP_H
#define SOC_CHIP_H

#include <linux/lotus/kapi.h>
#include <linux/init.h>

/* For getting soc chip id in the early stage */
unsigned int __init early_get_chipid(void);

#endif /* SOC_CHIP_H */
