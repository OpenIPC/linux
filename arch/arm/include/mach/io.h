#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#ifdef CONFIG_ARCH_HI3519
#include <mach/hi3519_io.h>
#endif

#ifdef CONFIG_ARCH_HI3519V101
#include <mach/hi3519v101_io.h>
#endif

#if defined(CONFIG_ARCH_HI3559) || defined(CONFIG_ARCH_HI3556)
#include <mach/hi3559_io.h>
#endif

#ifdef CONFIG_ARCH_HI3516AV200
#include <mach/hi3516av200_io.h>
#endif

#ifdef CONFIG_ARCH_HI3536C
#include <mach/hi3536c_io.h>
#endif

#ifdef CONFIG_ARCH_HI3531D
#include <mach/hi3531d_io.h>
#endif

#ifdef CONFIG_ARCH_HI3520DV400
#include <mach/hi3520dv400_io.h>
#endif

#ifdef CONFIG_ARCH_HI3521D
#include <mach/hi3521d_io.h>
#endif

#endif
