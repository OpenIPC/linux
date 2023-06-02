/**
 * History:
 *    2012/09/17 - [Tzu-Jung Lee] created file
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

#ifndef __PLAT_AMBARELLA_REMOTEPROC_CFG_H
#define __PLAT_AMBARELLA_REMOTEPROC_CFG_H


/*
 * [Copy from the virtio_ring.h]
 *
 * The standard layout for the ring is a continuous chunk of memory which looks
 * like this.  We assume num is a power of 2.
 *
 * struct vring
 * {
 *	// The actual descriptors (16 bytes each)
 *	struct vring_desc desc[num];
 *
 *	// A ring of available descriptor heads with free-running index.
 *	__u16 avail_flags;
 *	__u16 avail_idx;
 *	__u16 available[num];
 *	__u16 used_event_idx;
 *
 *	// Padding to the next align boundary.
 *	char pad[];
 *
 *	// A ring of used descriptor heads with free-running index.
 *	__u16 used_flags;
 *	__u16 used_idx;
 *	struct vring_used_elem used[num];
 *	__u16 avail_event_idx;
 * };
 */

/*
 * Calculation of memory usage for a descriptior rings with n buffers:
 *
 *   (16 * n) + 2 + 2 + (2 * n) + PAD + 2 + 2 + (8 * n) + 2
 *
 *   = (26 * n) + 10 + PAD( < 4K)
 *
 * EX:
 *   A RPMSG bus with 4096 buffers has two descriptor rings with 2048
 *   descriptors each. And each of the them needs:
 *
 *   26 * 2K + 10 + 4K = 56 K + 10 bytes
 */

#define RPMSG_VRING_BASE		(CONFIG_RPMSG_VRING_BASE)
#define RPMSG_NUM_BUFS			(CONFIG_RPMSG_NUM_BUFS)
#define RPMSG_BUF_SIZE			(CONFIG_RPMSG_BUF_SIZE)

#define RPMSG_TOTAL_BUF_SPACE		(RPMSG_NUM_BUFS * RPMSG_BUF_SIZE)

#ifdef CONFIG_MACH_GINKGO

#define VRING_C0_TO_C1			(0x21000000)
#define VRING_C1_TO_C0			(0x21020000)
#define VRING_C0_AND_C1_BUF		(0x20000000)
#define VRING_IRQ_C0_TO_C1_KICK		(106)
#define VRING_IRQ_C0_TO_C1_ACK		(107)
#define VRING_IRQ_C1_TO_C0_KICK		(108)
#define VRING_IRQ_C1_TO_C0_ACK		(109)

#elif defined(CONFIG_MACH_HYACINTH_0) || defined(CONFIG_MACH_HYACINTH_1)
/*
 * On A8, we allocate 4096 4K-byte buffers for each RPMSG bus.
 * The buffers are partitioned into two dedicated halves for TX and RX.
 * Besides the 16 MB buffer space, each RPMSG bus also requires two
 * descriptor rings with 56 KB each.
 *
 * Two make it simple for the configuration and debugging,
 * we allocate 64 MB in total for the 3 RPMSG buses.
 * The first 16 MB are used for the 3 * 2 descrpitor rings.
 * And the reset of 48 MB are partitioned for the 3 buses.
 */
#define VRING_CA9_B_TO_A		(RPMSG_VRING_BASE + 0x00000000)
#define VRING_CA9_A_TO_B		(RPMSG_VRING_BASE + 0x00010000)

#define VRING_ARM11_TO_CA9_A		(RPMSG_VRING_BASE + 0x00020000)
#define VRING_CA9_A_TO_ARM11		(RPMSG_VRING_BASE + 0x00030000)

#define VRING_ARM11_TO_CA9_B		(RPMSG_VRING_BASE + 0x00040000)
#define VRING_CA9_B_TO_ARM11		(RPMSG_VRING_BASE + 0x00050000)

#define VRING_BUF_CA9_A_AND_B		(RPMSG_VRING_BASE + RPMSG_TOTAL_BUF_SPACE * 1)

#define VRING_BUF_CA9_A_AND_ARM11	(RPMSG_VRING_BASE + RPMSG_TOTAL_BUF_SPACE * 2)

#define VRING_BUF_CA9_B_AND_ARM11	(RPMSG_VRING_BASE + RPMSG_TOTAL_BUF_SPACE * 3)

#define VRING_CA9_A_TO_B_CLNT_IRQ	(106)
#define VRING_CA9_A_TO_B_HOST_IRQ	(107)

#define VRING_CA9_B_TO_A_CLNT_IRQ	(108)
#define VRING_CA9_B_TO_A_HOST_IRQ	(109)

#define VRING_CA9_A_TO_ARM11_CLNT_IRQ	(110)
#define VRING_CA9_A_TO_ARM11_HOST_IRQ	(111)

#define VRING_ARM11_TO_CA9_A_CLNT_IRQ	(112)
#define VRING_ARM11_TO_CA9_A_HOST_IRQ	(113)

#define VRING_CA9_B_TO_ARM11_CLNT_IRQ	(114)
#define VRING_CA9_B_TO_ARM11_HOST_IRQ	(115)

#define VRING_ARM11_TO_CA9_B_CLNT_IRQ	(116)
#define VRING_ARM11_TO_CA9_B_HOST_IRQ	(117)
#endif /* CONFIG_MACH_XXX */

#endif /* __PLAT_AMBARELLA_REMOTEPROC_CFG_H */
