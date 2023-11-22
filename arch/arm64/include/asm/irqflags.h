/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_IRQFLAGS_H
#define __ASM_IRQFLAGS_H

#include <asm/alternative.h>
#include <asm/barrier.h>
#include <asm/ptrace.h>
#include <asm/sysreg.h>

#ifdef CONFIG_SSTAR_IRQ_DEBUG_TRACE
extern void sstar_irq_disable_debug(void);
extern void sstar_irq_enable_debug(void);
#endif

/*
 * Aarch64 has flags for masking: Debug, Asynchronous (serror), Interrupts and
 * FIQ exceptions, in the 'daif' register. We mask and unmask them in 'dai'
 * order:
 * Masking debug exceptions causes all other exceptions to be masked too/
 * Masking SError masks irq, but not debug exceptions. Masking irqs has no
 * side effects for other flags. Keeping to this order makes it easier for
 * entry.S to know which exceptions should be unmasked.
 *
 * FIQ is never expected, but we mask it when we disable debug exceptions, and
 * unmask it at all other times.
 */

/*
 * CPU interrupt mask handling.
 */
static inline void arch_local_irq_enable(void)
{
#ifdef CONFIG_SSTAR_IRQ_DEBUG_TRACE
	sstar_irq_enable_debug();
#endif

	if (system_has_prio_mask_debugging()) {
		u32 pmr = read_sysreg_s(SYS_ICC_PMR_EL1);

		WARN_ON_ONCE(pmr != GIC_PRIO_IRQON && pmr != GIC_PRIO_IRQOFF);
	}

	asm volatile(ALTERNATIVE(
		"msr	daifclr, #2		// arch_local_irq_enable",
		__msr_s(SYS_ICC_PMR_EL1, "%0"),
		ARM64_HAS_IRQ_PRIO_MASKING)
		:
		: "r" ((unsigned long) GIC_PRIO_IRQON)
		: "memory");

	pmr_sync();

#ifdef CONFIG_SSTAR_IRQ_DEBUG_TRACE
	sstar_irq_disable_debug();
#endif
}

static inline void arch_local_irq_disable(void)
{
	if (system_has_prio_mask_debugging()) {
		u32 pmr = read_sysreg_s(SYS_ICC_PMR_EL1);

		WARN_ON_ONCE(pmr != GIC_PRIO_IRQON && pmr != GIC_PRIO_IRQOFF);
	}

	asm volatile(ALTERNATIVE(
		"msr	daifset, #2		// arch_local_irq_disable",
		__msr_s(SYS_ICC_PMR_EL1, "%0"),
		ARM64_HAS_IRQ_PRIO_MASKING)
		:
		: "r" ((unsigned long) GIC_PRIO_IRQOFF)
		: "memory");
}

#define local_fiq_enable()	asm("msr	daifclr, #1" : : : "memory")
#define local_fiq_disable()	asm("msr	daifset, #1" : : : "memory")

#define local_async_enable()	asm("msr	daifclr, #4" : : : "memory")
#define local_async_disable()	asm("msr	daifset, #4" : : : "memory")

/*
 * Save the current interrupt enable state.
 */
static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;

	asm volatile(ALTERNATIVE(
		"mrs	%0, daif",
		__mrs_s("%0", SYS_ICC_PMR_EL1),
		ARM64_HAS_IRQ_PRIO_MASKING)
		: "=&r" (flags)
		:
		: "memory");

	return flags;
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	int res;

	asm volatile(ALTERNATIVE(
		"and	%w0, %w1, #" __stringify(PSR_I_BIT),
		"eor	%w0, %w1, #" __stringify(GIC_PRIO_IRQON),
		ARM64_HAS_IRQ_PRIO_MASKING)
		: "=&r" (res)
		: "r" ((int) flags)
		: "memory");

	return res;
}

static inline int arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	flags = arch_local_save_flags();

	/*
	 * There are too many states with IRQs disabled, just keep the current
	 * state if interrupts are already disabled/masked.
	 */
	if (!arch_irqs_disabled_flags(flags))
		arch_local_irq_disable();

#ifdef CONFIG_SSTAR_IRQ_DEBUG_TRACE
	sstar_irq_disable_debug();
#endif

	return flags;
}

/*
 * restore saved IRQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
#ifdef CONFIG_SSTAR_IRQ_DEBUG_TRACE
	sstar_irq_enable_debug();
#endif

	asm volatile(ALTERNATIVE(
		"msr	daif, %0",
		__msr_s(SYS_ICC_PMR_EL1, "%0"),
		ARM64_HAS_IRQ_PRIO_MASKING)
		:
		: "r" (flags)
		: "memory");

	pmr_sync();
}

/*
 * save and restore debug state
 */
#define local_dbg_save(flags)						\
	do {								\
		typecheck(unsigned long, flags);			\
		asm volatile(						\
		"mrs    %0, daif		// local_dbg_save\n"	\
		"msr    daifset, #8"					\
		: "=r" (flags) : : "memory");				\
	} while (0)

#define local_dbg_restore(flags)					\
	do {								\
		typecheck(unsigned long, flags);			\
		asm volatile(						\
		"msr    daif, %0		// local_dbg_restore\n"	\
		: : "r" (flags) : "memory");				\
	} while (0)
#endif /* __ASM_IRQFLAGS_H */
