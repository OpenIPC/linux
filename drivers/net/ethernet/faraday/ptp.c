/*******************************************************************************

  Intel PRO/0300 Linux driver
  Copyright(c) 1999 - 2013 Intel Corporation.
  
  Faraday FTGMAC030 PTP Linux driver
  Copyright(c) 2014-2016 Faraday Technology

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  e0300-devel Mailing List <e0300-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

/* PTP 1588 Hardware Clock (PHC)
 * Derived from PTP Hardware Clock driver for Intel 82576 and 82580 (igb)
 * Copyright (C) 2011 Richard Cochran <richardcochran@gmail.com>
 */
#include <linux/version.h>
#include <linux/export.h>
#include <linux/kernel.h>

#include "ftgmac030.h"


/**
 * ftgmac030_phc_adjfreq - adjust the frequency of the hardware clock
 * @ptp: ptp clock structure
 * @delta: Desired frequency change in parts per billion
 *
 * Adjust the frequency of the PHC cycle counter by the indicated delta from
 * the base frequency.
 **/
static int ftgmac030_phc_adjfreq(struct ptp_clock_info *ptp, s32 delta)
{
	struct ftgmac030 *priv = container_of(ptp, struct ftgmac030,
						     ptp_clock_info);
	bool neg_adj = false;
	u64 adjustment, fract;
	u32 incns, i;
	s32 incnns, adjnns;

	if (delta < 0) {
		neg_adj = true;
		delta = -delta;
	}

	if (delta > ptp->max_adj)
		return -EINVAL;

	netdev_info(priv->netdev, "adjfreq: %s%d ppb\n", neg_adj?"-":"+", delta);

	/* Get the increment value per period */
	incns = ioread32(priv->base + FTGMAC030_REG_PTP_NS_PERIOD);
	incnns = ioread32(priv->base + FTGMAC030_REG_PTP_NNS_PERIOD);
	/*
	 * If delta = 2 ppb(ns/s) means there is a different 2ns 
	 * for each second. So we need to devide this 2ns evenly
	 * into each period. Let's say phc_clock = 50 MHz.
	 *
	 * So, adjustment = 2ns / 50 MHz.
	 * 
	 * because 50 MHz = 1,000,000,000ns / 20ns.
	 *
	 * Then, we get:
	 * 	adjustment = 2ns / (1,000,000,000ns / 20ns).
	 *		   = (2ns * 20ns) / 1,000,000,000ns
	 *
	 * And finally we must adjust the incvalue to
	 *
	 *     incvalue = incvalue +(-) adjustment
	 */
	adjustment = incns;
	adjustment *= delta;
	adjustment = div_u64_rem(adjustment, 1000000000, (u32 *) &fract);
	/*
	 * fraction must be converted into binary format.
	 * example:  0.5  -> 0x80000000
	 *	     0.25 -> 0x40000000
	 */
	adjnns = 0;
	if (fract) {
		for (i=28; i>=0; i-=4) {
			int tmp;

			fract *= 16;
			tmp = div_u64(fract, 1000000000);
			adjnns = adjnns + (tmp << i);
			tmp = div_u64_rem(fract, 1000000000, (u32 *) &fract);
			if (!fract)
				break;
		}

	}

	if (neg_adj) {
		if (incnns - adjnns < 0) { /* case 40.5 - 1.8 = 38.7 */
			incns = incns - adjustment - 1;
		} else { /* case 40.8 - 1.5 = 39.3 */
			incns = incns - adjustment;
		}
		incnns = incnns - adjnns;
	} else {
		if ((u64)incnns + (u64)adjnns > 0xFFFFFFFFLL) { /* case 40.5 + 1.8 = 42.3 */
			incns = incns + adjustment + 1;
		} else { /* case 40.3 + 1.5 = 41.8 */
			incns = incns + adjustment;
		}
		incnns = incnns + adjnns;

	} 

	netdev_info(priv->netdev, "adjfreq: ns_period %d nns_period 0x%08x\n", incns, incnns);
	iowrite32(incns, priv->base + FTGMAC030_REG_PTP_NS_PERIOD);
	iowrite32(incnns, priv->base + FTGMAC030_REG_PTP_NNS_PERIOD);
	return 0;
}

/**
 * ftgmac030_phc_adjtime - Shift the time of the hardware clock
 * @ptp: ptp clock structure
 * @delta: Desired change in nanoseconds
 *
 * Adjust the timer by resetting the timecounter structure.
 **/
static int ftgmac030_phc_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct ftgmac030 *priv = container_of(ptp, struct ftgmac030,
						     ptp_clock_info);
	unsigned long flags;
	struct timespec ts;
	s64 now;

	spin_lock_irqsave(&priv->systim_lock, flags);

	ts.tv_sec = ioread32(priv->base + FTGMAC030_REG_PTP_TM_SEC);
	ts.tv_nsec = ioread32(priv->base + FTGMAC030_REG_PTP_TM_NSEC);

	spin_unlock_irqrestore(&priv->systim_lock, flags);

	now = timespec_to_ns(&ts);
	now += delta;

	ts = ns_to_timespec(now);
	spin_lock_irqsave(&priv->systim_lock, flags);
	iowrite32(ts.tv_sec, priv->base + FTGMAC030_REG_PTP_TM_SEC);
	iowrite32(ts.tv_nsec, priv->base + FTGMAC030_REG_PTP_TM_NSEC);
	spin_unlock_irqrestore(&priv->systim_lock, flags);

	return 0;
}

/**
 * ftgmac030_phc_gettime - Reads the current time from the hardware clock
 * @ptp: ptp clock structure
 * @ts: timespec structure to hold the current time value
 *
 * Read the timecounter and return the correct value in ns after converting
 * it into a struct timespec.
 **/
static int ftgmac030_phc_gettime(struct ptp_clock_info *ptp, struct timespec *ts)
{
	struct ftgmac030 *priv = container_of(ptp, struct ftgmac030,
						     ptp_clock_info);
	unsigned long flags;

	spin_lock_irqsave(&priv->systim_lock, flags);
	ts->tv_sec = ioread32(priv->base + FTGMAC030_REG_PTP_TM_SEC);
	ts->tv_nsec = ioread32(priv->base + FTGMAC030_REG_PTP_TM_NSEC);
	spin_unlock_irqrestore(&priv->systim_lock, flags);

	return 0;
}

/**
 * ftgmac030_phc_settime - Set the current time on the hardware clock
 * @ptp: ptp clock structure
 * @ts: timespec containing the new time for the cycle counter
 *
 * Reset the timecounter to use a new base value instead of the kernel
 * wall timer value.
 **/
static int ftgmac030_phc_settime(struct ptp_clock_info *ptp,
			      const struct timespec *ts)
{
	struct ftgmac030 *priv = container_of(ptp, struct ftgmac030,
						     ptp_clock_info);
	unsigned long flags;

	/* reset the timecounter */
	spin_lock_irqsave(&priv->systim_lock, flags);
	iowrite32(ts->tv_sec, priv->base + FTGMAC030_REG_PTP_TM_SEC);
	iowrite32(ts->tv_nsec, priv->base + FTGMAC030_REG_PTP_TM_NSEC);
	spin_unlock_irqrestore(&priv->systim_lock, flags);

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
 **/
static int ftgmac030_phc_enable(struct ptp_clock_info __always_unused *ptp,
			     struct ptp_clock_request __always_unused *request,
			     int __always_unused on)
{
	return -EOPNOTSUPP;
}

static void ftgmac030_systim_overflow_work(struct work_struct *work)
{
	struct ftgmac030 *priv = container_of(work, struct ftgmac030,
						     systim_overflow_work.work);
	struct timespec ts;

	priv->ptp_clock_info.gettime(&priv->ptp_clock_info, &ts);

	netdev_dbg(priv->netdev, "SYSTIM overflow check at %ld.%09lu\n", 
		   ts.tv_sec, ts.tv_nsec);

	schedule_delayed_work(&priv->systim_overflow_work,
			      FTGMAC030_SYSTIM_OVERFLOW_PERIOD);
}

static const struct ptp_clock_info ftgmac030_ptp_clock_info = {
	.owner		= THIS_MODULE,
	.n_alarm	= 0,
	.n_ext_ts	= 0,
	.n_per_out	= 0,
	.pps		= 0,
	.adjfreq	= ftgmac030_phc_adjfreq,
	.adjtime	= ftgmac030_phc_adjtime,
	.gettime	= ftgmac030_phc_gettime,
	.settime	= ftgmac030_phc_settime,
	.enable		= ftgmac030_phc_enable,
};

/**
 * ftgmac030_ptp_init - initialize PTP for devices which support it
 * @adapter: board private structure
 *
 * This function performs the required steps for enabling PTP support.
 * If PTP support has already been loaded it simply calls the cyclecounter
 * init routine and exits.
 **/
void ftgmac030_ptp_init(struct ftgmac030 *adapter)
{
	u32 incns;

	adapter->ptp_clock_info = ftgmac030_ptp_clock_info;

	snprintf(adapter->ptp_clock_info.name,
		 sizeof(adapter->ptp_clock_info.name), "%pm",
		 adapter->netdev->perm_addr);

	/* Get the increment value per period */
	incns = ioread32(adapter->base + FTGMAC030_REG_PTP_NS_PERIOD);

	/* Prevent from adjust the increment ns value per
	 * period to less than 1.
	 * If ptp_clock freq is 25 MHz, period is 40 ns,
	 * then max adjustment is 39 ns.
	 * Or in freq is 39 ns * 25MHz = 975 MHz
	 */
	adapter->ptp_clock_info.max_adj = (incns - 1) *
					   div_u64(1000000000, incns);

	INIT_DELAYED_WORK(&adapter->systim_overflow_work,
			  ftgmac030_systim_overflow_work);

	schedule_delayed_work(&adapter->systim_overflow_work,
			      FTGMAC030_SYSTIM_OVERFLOW_PERIOD);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,6,0)
	adapter->ptp_clock = ptp_clock_register(&adapter->ptp_clock_info);
#else
	adapter->ptp_clock = ptp_clock_register(&adapter->ptp_clock_info,
						adapter->dev);
#endif
	if (IS_ERR(adapter->ptp_clock)) {
		adapter->ptp_clock = NULL;
		netdev_err(adapter->netdev, "ptp_clock_register failed\n");
	} else {
		netdev_info(adapter->netdev, "registered PHC clock\n");
		netdev_info(adapter->netdev, "period %dns max_adj %d\n",
					      incns,
					      adapter->ptp_clock_info.max_adj);
	}
}

/**
 * ftgmac030_ptp_remove - disable PTP device and stop the overflow check
 * @adapter: board private structure
 *
 * Stop the PTP support, and cancel the delayed work.
 **/
void ftgmac030_ptp_remove(struct ftgmac030 *adapter)
{
	cancel_delayed_work_sync(&adapter->systim_overflow_work);

	if (adapter->ptp_clock) {
		ptp_clock_unregister(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		netdev_info(adapter->netdev, "removed PHC\n");
	}
}
