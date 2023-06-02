#ifndef __HISILICON_CORE_H
#define __HISILICON_CORE_H

#include <linux/reboot.h>

#define MPCORE_COH_MAGIC 0x434f484d
#define COH_MAGIC        (0x0500)
#define COH_FUN_BASE     (0x0504)
#define COH_FUN_PARAM    (0x0508)

#define OFFSET_LOWPOWER_CTRL        (0x109c)

extern void fhca7_set_cpu_jump(int cpu, void *jump_addr);
extern int fhca7_get_cpu_jump(int cpu);
extern void secondary_startup(void);

extern void fhca7_cpu_die(unsigned int cpu);
extern int fhca7_cpu_kill(unsigned int cpu);
extern void fhca7_set_cpu(int cpu, bool enable);

extern void fhca7_wakeup_cpu(unsigned int cpu);
extern void fhca7_shutdown_cpu(unsigned int cpu);

#endif
