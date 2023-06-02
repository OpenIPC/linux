/**
 * system/src/rpclnt/rpclnt.h
 *
 * History:
 *    2012/08/15 - [Tzu-Jung Lee] created file
 *
 * Copyright 2008-2015 Ambarella Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __RPCLNT_H__
#define __RPCLNT_H__

struct rpclnt
{
	struct work_struct	svq_work;
	struct work_struct	rvq_work;

	int			inited;
	char			*name;

	void			*svq;
	int			svq_tx_irq;
	int			svq_rx_irq;
	int			svq_num_bufs;
	u32			svq_buf_phys;
	u32			svq_vring_phys;
	int			svq_vring_algn;

	void			*rvq;
	int			rvq_tx_irq;
	int			rvq_rx_irq;
	int			rvq_num_bufs;
	u32			rvq_buf_phys;
	u32			rvq_vring_phys;
	int			rvq_vring_algn;
};

extern struct rpclnt *rpclnt_sync(const char *bus_name);

#endif /* __RPCLNT_H__ */
