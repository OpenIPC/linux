/*
 * arch/arm/plat-ambarella/include/plat/remoteproc.h
 *
 * Author: Tzu-Jung Lee <tjlee@ambarella.com>
 *
 * Copyright (C) 2012-2012, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_REMOTEPROC_H
#define __PLAT_AMBARELLA_REMOTEPROC_H

#include <linux/remoteproc.h>

struct ambarella_rproc_pdata {
	const char *name;
	struct rproc *rproc;
	const char *firmware;
	int svq_tx_irq;
	int svq_rx_irq;
	int rvq_tx_irq;
	int rvq_rx_irq;
	const struct rproc_ops *ops;
	unsigned long buf_addr_pa;
	struct work_struct svq_work;
	struct work_struct rvq_work;
	struct resource_table *(*gen_rsc_table)(int *tablesz);
};

#endif /* __PLAT_AMBARELLA_REMOTEPROC_H */
