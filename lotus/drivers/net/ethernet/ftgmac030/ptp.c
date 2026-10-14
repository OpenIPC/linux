// SPDX-License-Identifier: GPL-2.0
// Copyright (c) LOTUS. All rights reserved.

#include <linux/version.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "ftgmac030.h"


/**
 * ftgmac030_phc_adjfreq - adjust the frequency of the hardware clock
 * @ptp: ptp clock structure
 * @delta: Desired frequency change in parts per billion
 *
 * Adjust the frequency of the PHC cycle counter by the indicated delta from
 * the base frequency.
 */
static int ftgmac030_phc_adjfreq(struct ptp_clock_info *ptp, s32 delta)
{
	struct ftgmac030_ctrl *ctrl = container_of(ptp, struct ftgmac030_ctrl,
						 ptp_clock_info);
	bool neg_adj = false;
	u64 incns, adjustment, fract;

	if ((delta > ptp->max_adj) || (delta <= -1000000000))
		return -EINVAL;

	if (delta < 0) {
		neg_adj = true;
		delta = -delta;
	}

	// If delta = 2 ppb(ns/s) means there is a different 2ns
	// for each second. So we need to devide this 2ns evenly
	// into each period. Let's say phc_clock = 50 MHz.
	//
	// So, adjustment = 2ns / 50 MHz.
	//
	// because 50 MHz = 1,000,000,000ns / 20ns.
	//
	// Then, we get:
	//	adjustment = 2ns / (1,000,000,000ns / 20ns).
	//		   = (2ns * 20ns) / 1,000,000,000ns
	//
	// And finally we must adjust the incvalue to
	//
	//     incvalue = incvalue +(-) adjustment
	incns = ctrl->incval;
	adjustment = (incns * 1000000000ull) + ctrl->incval_nns;
	adjustment *= delta;
	adjustment = div_u64(adjustment, 1000000000ull);

	incns = (incns * 1000000000ull) + ctrl->incval_nns;
	incns = neg_adj ? (incns - adjustment) : (incns + adjustment);
	incns = div64_u64_rem(incns, 1000000000ull, &fract);

	// Must write offset 0x13C (PTP_NS_PERIOD) after update
	// offset 0x140(PTP_NNS_PERIOD).
	//
	// Don't change the sequence.
	if (fract < 1000000000ull)
		iow32(FTGMAC030_REG_PTP_NNS_PERIOD, (u32)fract);
	else
		e_err(drv, "fractional value larger than 999,999,999\n");

	iow32(FTGMAC030_REG_PTP_NS_PERIOD, (u32)incns);

	return 0;
}

/**
 * ftgmac030_phc_adjtime - Shift the time of the hardware clock
 * @ptp: ptp clock structure
 * @delta: Desired change in nanoseconds
 *
 * Adjust the timer by resetting the timecounter structure.
 */
static int ftgmac030_phc_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct ftgmac030_ctrl *ctrl = container_of(ptp, struct ftgmac030_ctrl,
						 ptp_clock_info);
	unsigned long flags;
	struct timespec64 ts;
	s64 now;

	spin_lock_irqsave(&ctrl->systim_lock, flags);

	now = (s64)ior32(FTGMAC030_REG_PTP_TM_SEC) * NSEC_PER_SEC;
	now += ior32(FTGMAC030_REG_PTP_TM_NSEC);

	spin_unlock_irqrestore(&ctrl->systim_lock, flags);

	now += delta;
	ts = ns_to_timespec64((const s64) now);

	spin_lock_irqsave(&ctrl->systim_lock, flags);
	// Don't change the sequence
	iow32(FTGMAC030_REG_PTP_TM_NSEC, ts.tv_nsec);
	iow32(FTGMAC030_REG_PTP_TM_SEC, ts.tv_sec);
	spin_unlock_irqrestore(&ctrl->systim_lock, flags);

	return 0;
}

/**
 * ftgmac030_phc_gettime - Reads the current time from the hardware clock
 * @ptp: ptp clock structure
 * @ts: timespec structure to hold the current time value
 *
 * Read the timecounter and return the correct value in ns after converting
 * it into a struct timespec.
 */
static int ftgmac030_phc_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct ftgmac030_ctrl *ctrl = container_of(ptp, struct ftgmac030_ctrl,
						 ptp_clock_info);
	unsigned long flags;

	spin_lock_irqsave(&ctrl->systim_lock, flags);
	ts->tv_sec = ior32(FTGMAC030_REG_PTP_TM_SEC);
	ts->tv_nsec = ior32(FTGMAC030_REG_PTP_TM_NSEC);
	spin_unlock_irqrestore(&ctrl->systim_lock, flags);

	return 0;
}

/**
 * ftgmac030_phc_settime - Set the current time on the hardware clock
 * @ptp: ptp clock structure
 * @ts: timespec containing the new time for the cycle counter
 *
 * Reset the timecounter to use a new base value instead of the kernel
 * wall timer value.
 */
static int ftgmac030_phc_settime(struct ptp_clock_info *ptp,
				 const struct timespec64 *ts)
{
	struct ftgmac030_ctrl *ctrl = container_of(ptp, struct ftgmac030_ctrl,
						   ptp_clock_info);
	unsigned long flags;

	// Reset the timecounter
	spin_lock_irqsave(&ctrl->systim_lock, flags);
	iow32(FTGMAC030_REG_PTP_TM_NSEC, ts->tv_nsec);
	iow32(FTGMAC030_REG_PTP_TM_SEC, ts->tv_sec);
	spin_unlock_irqrestore(&ctrl->systim_lock, flags);

	return 0;
}

/**
 * ftgmac030_phc_enable - enable or disable an ancillary feature
 * @ptp: ptp clock structure
 * @request: Desired resource to enable or disable
 * @on: Caller passes one to enable or zero to disable
 *
 * Enable (or disable) ancillary features of the PHC subsystem.
 * Currently, no ancillary features are supported.
 */
static int ftgmac030_phc_enable(struct ptp_clock_info __always_unused *ptp,
				struct ptp_clock_request __always_unused *request,
				int __always_unused on)
{
	return -EOPNOTSUPP;
}

static const struct ptp_clock_info ftgmac030_ptp_clock_info = {
	.owner		= THIS_MODULE,
	.n_alarm	= 0,
	.n_ext_ts	= 0,
	.n_per_out	= 0,
	.pps		= 0,
	.adjfreq	= ftgmac030_phc_adjfreq,
	.adjtime	= ftgmac030_phc_adjtime,
	.gettime64	= ftgmac030_phc_gettime,
	.settime64	= ftgmac030_phc_settime,
	.enable		= ftgmac030_phc_enable,
};

/**
 * ftgmac030_ptp_init - initialize PTP for devices which support it
 * @hw: board hwate structure
 *
 * This function performs the required steps for enabling PTP support.
 * If PTP support has already been loaded it simply calls the cyclecounter
 * init routine and exits.
 **/
void ftgmac030_ptp_init(struct ftgmac030_ctrl *ctrl)
{
	ctrl->ptp_clock = NULL;

	if (!(ctrl->flags & FLAG_HAS_HW_TIMESTAMP)) {
		e_info(drv, "PTP not enabled\n");
		return;
	}
	ctrl->ptp_clock_info = ftgmac030_ptp_clock_info;

	snprintf(ctrl->ptp_clock_info.name,
		 sizeof(ctrl->ptp_clock_info.name), "%pm",
		 ctrl->netdev->perm_addr);

	iow32(FTGMAC030_REG_PTP_NNS_PERIOD, ctrl->incval_nns);
	iow32(FTGMAC030_REG_PTP_NS_PERIOD, ctrl->incval);

	ctrl->ptp_clock_info.max_adj = 600000000;

	ctrl->ptp_clock = ptp_clock_register(&ctrl->ptp_clock_info,
					     &ctrl->pdev->dev);
	if (IS_ERR(ctrl->ptp_clock)) {
		ctrl->ptp_clock = NULL;
		e_err(drv, "ptp_clock_register failed\n");
	} else {
		e_info(drv, "registered PHC clock period max_adj %d\n",
		       ctrl->ptp_clock_info.max_adj);
	}
}

/**
 * ftgmac030_ptp_remove - disable PTP device and stop the overflow check
 * @hw: board hwate structure
 *
 * Stop the PTP support, and cancel the delayed work.
 **/
void ftgmac030_ptp_remove(struct ftgmac030_ctrl *ctrl)
{
	if (ctrl->ptp_clock) {
		ptp_clock_unregister(ctrl->ptp_clock);
		ctrl->ptp_clock = NULL;
		e_info(drv, "removed PHC\n");
	}
}
