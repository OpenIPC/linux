#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#ifdef CONFIG_ARCH_HI3516A
#include <mach/hi3516a_io.h>
#endif

#ifdef CONFIG_ARCH_HI3518EV20X
#include <mach/hi3518ev20x_io.h>
#endif

#ifdef CONFIG_ARCH_HI3516CV100
/* cv100 reuses cv200's mach file (mach-hi3518ev20x.c) — share its IO map.
 * The IOCH1 range (0x10000000-0x100E0000) doesn't cover cv100's VIC at
 * 0x10140000, but the iotable_init() entry is a virt-mapping cache hint;
 * absence just means ioremap() is called per-driver, which still works. */
#include <mach/hi3518ev20x_io.h>
#endif

#ifdef CONFIG_ARCH_HI3536DV100
#include <mach/hi3536dv100_io.h>
#endif

#ifdef CONFIG_ARCH_HI3521A
#include <mach/hi3521a_io.h>
#endif

#ifdef CONFIG_ARCH_HI3531A
#include <mach/hi3531a_io.h>
#endif

#ifdef CONFIG_ARCH_HI3516CV500
#include <mach/hi3516cv500_io.h>
#endif

#ifdef CONFIG_ARCH_HI3516DV300
#include <mach/hi3516dv300_io.h>
#endif

#ifdef CONFIG_ARCH_HI3556V200
#include <mach/hi3556v200_io.h>
#endif

#ifdef CONFIG_ARCH_HI3559V200
#include <mach/hi3559v200_io.h>
#endif

#ifdef CONFIG_ARCH_HI3519AV100
#include <mach/hi3519av100_io.h>
#endif

#endif
