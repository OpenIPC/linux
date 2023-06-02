#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H

#if __LINUX_ARM_ARCH__ < 6 && !defined(CONFIG_CPU_FMP626)
#error SMP not supported on pre-ARMv6 CPUs
#endif

#include <asm/processor.h>

/*
 * sev and wfe are ARMv6K extensions.  Uniprocessor ARMv6 may not have the K
 * extensions, so when running on UP, we have to patch these instructions away.
 */
#define ALT_SMP(smp, up)					\
	"9998:	" smp "\n"					\
	"	.pushsection \".alt.smp.init\", \"a\"\n"	\
	"	.long	9998b\n"				\
	"	" up "\n"					\
	"	.popsection\n"

#ifdef CONFIG_THUMB2_KERNEL
#define SEV		ALT_SMP("sev.w", "nop.w")
/*
 * For Thumb-2, special care is needed to ensure that the conditional WFE
 * instruction really does assemble to exactly 4 bytes (as required by
 * the SMP_ON_UP fixup code).   By itself "wfene" might cause the
 * assembler to insert a extra (16-bit) IT instruction, depending on the
 * presence or absence of neighbouring conditional instructions.
 *
 * To avoid this unpredictableness, an approprite IT is inserted explicitly:
 * the assembler won't change IT instructions which are explicitly present
 * in the input.
 */
#define WFE(cond)	ALT_SMP(		\
	"it " cond "\n\t"			\
	"wfe" cond ".n",			\
						\
	"nop.w"					\
)
#else
#define SEV		ALT_SMP("sev", "nop")
#define WFE(cond)	ALT_SMP("wfe" cond, "nop")
#endif

static inline void dsb_sev(void)
{
#if __LINUX_ARM_ARCH__ >= 7
	__asm__ __volatile__ (
		"dsb\n"
		SEV
	);
#elif defined(CONFIG_CPU_FMP626)
	__asm__ __volatile__ (
		"mcr	p15, 0, %0, c7, c10, 4\n"
		"cdp	p13, 0, c0, c0, c0, 1	@ set event\n"
		: : "r" (0)
	);
#else
	__asm__ __volatile__ (
		"mcr p15, 0, %0, c7, c10, 4\n"
		SEV
		: : "r" (0)
	);
#endif
}

/*
 * ARMv6 Spin-locking.
 *
 * We exclusively read the old value.  If it is zero, we may have
 * won the lock, so we try exclusively storing it.  A memory barrier
 * is required after we get a lock, and before we release it, because
 * V6 CPUs are assumed to have weakly ordered memory.
 *
 * Unlocked value: 0
 * Locked value: 1
 */

#define arch_spin_is_locked(x)		((x)->lock != 0)
#define arch_spin_unlock_wait(lock) \
	do { while (arch_spin_is_locked(lock)) cpu_relax(); } while (0)

#define arch_spin_lock_flags(lock, flags) arch_spin_lock(lock)

#ifdef CONFIG_CPU_FMP626
static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldc	p13, c0, [%1], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	teq	%0, #0\n"
"	cdpne	p13, 0, c0, c0, c0, 0	@ wait for event\n"
"	mcreq	p13, 0, %2, c0, c0, 0	@ data for strex\n"
"	stceq	p13, c0, [%1], {2}	@ strex to address\n"
"	mrceq	p13, 0, %0, c0, c0, 2	@ strex status\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"	ldc	p13, c0, [%1], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	teq	%0, #0\n"
"	mcreq	p13, 0, %2, c0, c0, 0	@ data for strex\n"
"	stceq	p13, c0, [%1], {2}	@ strex to address\n"
"	mrceq	p13, 0, %0, c0, c0, 2	@ strex status\n"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}
#else
static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
	WFE("ne")
"	strexeq	%0, %2, [%1]\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}
#endif

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	smp_mb();

	__asm__ __volatile__(
"	str	%1, [%0]\n"
	:
	: "r" (&lock->lock), "r" (0)
	: "cc");

	dsb_sev();
}

/*
 * RWLOCKS
 *
 *
 * Write locks are easy - we just set bit 31.  When unlocking, we can
 * just write zero since the lock is exclusively held.
 */

#ifdef CONFIG_CPU_FMP626
static inline void arch_write_lock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldc	p13, c0, [%1], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	teq	%0, #0\n"
"	cdpne	p13, 0, c0, c0, c0, 0	@ wait for event\n"
"	mcreq	p13, 0, %2, c0, c0, 0	@ data for strex\n"
"	stceq	p13, c0, [%1], {2}	@ strex to address\n"
"	mrceq	p13, 0, %0, c0, c0, 2	@ strex status\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	smp_mb();
}

static inline int arch_write_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldc	p13, c0, [%1], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	teq	%0, #0\n"
"	mcreq	p13, 0, %2, c0, c0, 0	@ data for strex\n"
"	stceq	p13, c0, [%1], {2}	@ strex to address\n"
"	mrceq	p13, 0, %0, c0, c0, 2	@ strex status\n"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}
#else
static inline void arch_write_lock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
	WFE("ne")
"	strexeq	%0, %2, [%1]\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	smp_mb();
}

static inline int arch_write_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&rw->lock), "r" (0x80000000)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}
#endif

static inline void arch_write_unlock(arch_rwlock_t *rw)
{
	smp_mb();

	__asm__ __volatile__(
	"str	%1, [%0]\n"
	:
	: "r" (&rw->lock), "r" (0)
	: "cc");

	dsb_sev();
}

/* write_can_lock - would write_trylock() succeed? */
#define arch_write_can_lock(x)		((x)->lock == 0)

/*
 * Read locks are a bit more hairy:
 *  - Exclusively load the lock value.
 *  - Increment it.
 *  - Store new lock value if positive, and we still own this location.
 *    If the value is negative, we've already failed.
 *  - If we failed to store the value, we want a negative result.
 *  - If we failed, try again.
 * Unlocking is similarly hairy.  We may have multiple read locks
 * currently active.  However, we know we won't have any write
 * locks.
 */
#ifdef CONFIG_CPU_FMP626
static inline void arch_read_lock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	__asm__ __volatile__(
"1:	ldc	p13, c0, [%2], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	adds	%0, %0, #1\n"
"	mcrpl	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stcpl	p13, c0, [%2], {2}	@ strex to address\n"
"	mrcpl	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	cdpmi	p13, 0, c0, c0, c0, 0	@ wait for event\n"
"	rsbpls	%0, %1, #0\n"
"	bmi	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
}

static inline void arch_read_unlock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	smp_mb();

	__asm__ __volatile__(
"1:	ldc	p13, c0, [%2], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	sub	%0, %0, #1\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stc	p13, c0, [%2], {2}	@ strex to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	teq	%1, #0\n"
"	bne	1b\n"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	if (tmp == 0)
		dsb_sev();
}

static inline int arch_read_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2 = 1;

	__asm__ __volatile__(
"1:	ldc	p13, c0, [%2], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	adds	%0, %0, #1\n"
"	mcrpl	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stcpl	p13, c0, [%2], {2}	@ strex to address\n"
"	mrcpl	p13, 0, %1, c0, c0, 2	@ strex status\n"
	: "=&r" (tmp), "+r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
	return tmp2 == 0;
}
#else
static inline void arch_read_lock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	adds	%0, %0, #1\n"
"	strexpl	%1, %0, [%2]\n"
	WFE("mi")
"	rsbpls	%0, %1, #0\n"
"	bmi	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
}

static inline void arch_read_unlock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2;

	smp_mb();

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	sub	%0, %0, #1\n"
"	strex	%1, %0, [%2]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	if (tmp == 0)
		dsb_sev();
}

static inline int arch_read_trylock(arch_rwlock_t *rw)
{
	unsigned long tmp, tmp2 = 1;

	__asm__ __volatile__(
"1:	ldrex	%0, [%2]\n"
"	adds	%0, %0, #1\n"
"	strexpl	%1, %0, [%2]\n"
	: "=&r" (tmp), "+r" (tmp2)
	: "r" (&rw->lock)
	: "cc");

	smp_mb();
	return tmp2 == 0;
}
#endif

/* read_can_lock - would read_trylock() succeed? */
#define arch_read_can_lock(x)		((x)->lock < 0x80000000)

#define arch_read_lock_flags(lock, flags) arch_read_lock(lock)
#define arch_write_lock_flags(lock, flags) arch_write_lock(lock)

#define arch_spin_relax(lock)	cpu_relax()
#define arch_read_relax(lock)	cpu_relax()
#define arch_write_relax(lock)	cpu_relax()

#endif /* __ASM_SPINLOCK_H */
