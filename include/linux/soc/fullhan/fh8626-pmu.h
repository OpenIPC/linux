/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shared FH8626V100 PMU access, serialized with the clock provider.
 */
#ifndef _LINUX_SOC_FULLHAN_FH8626_PMU_H
#define _LINUX_SOC_FULLHAN_FH8626_PMU_H

#include <linux/types.h>

/**
 * fh8626_pmu_read() - Read one system-controller register.
 * @reg: register offset inside the system controller
 * @value: where to store the value
 *
 * Serialized with the clock provider.
 *
 * Return: 0 on success, or a negative errno.
 */
int fh8626_pmu_read(unsigned int reg, u32 *value);

/**
 * fh8626_pmu_write() - Write one system-controller register.
 * @reg: register offset inside the system controller
 * @value: value to write
 *
 * Serialized with the clock provider.
 *
 * Return: 0 on success, or a negative errno.
 */
int fh8626_pmu_write(unsigned int reg, u32 value);

/**
 * fh8626_pmu_update_bits() - Update bits in one system-controller register.
 * @reg: register offset inside the system controller
 * @mask: bits to change
 * @value: new values for the bits set in @mask
 *
 * Serialized with the clock provider.
 *
 * Return: 0 on success, or a negative errno.
 */
int fh8626_pmu_update_bits(unsigned int reg, u32 mask, u32 value);

/**
 * fh8626_pmu_reset() - Pulse one block reset and wait for it.
 * @bit: reset bit number inside the system controller
 *
 * Sleepable. Pulses one block reset and waits until it completes.
 *
 * Return: 0 on success, or a negative errno.
 */
int fh8626_pmu_reset(unsigned int bit);

#endif /* _LINUX_SOC_FULLHAN_FH8626_PMU_H */
