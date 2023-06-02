/*
 * Novatek IR remote receiver - interface header file
 *
 * Copyright Novatek Microelectronics Corp. 2020. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

struct nvt_ir {
	void __iomem		*reg;
	struct rc_dev		*rc;
	struct clk		*clk;
	int			irq;
	spinlock_t		lock;
	spinlock_t		write_lock;
	u64			rc_type;
	u32			data_lo;
	u32			data_hi;
	struct delayed_work	decoder_work;
};

void nvt_ir_irq_cb(struct nvt_ir *ir);
void nvt_ir_enable(struct nvt_ir *ir);
void nvt_ir_disable(struct nvt_ir *ir);
void nvt_ir_protocol_config(struct nvt_ir *ir);
void nvt_ir_platform_init(struct nvt_ir *ir);
