/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_MDNS_H__
#define __TX_ISP_MDNS_H__

#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/media/ingenic/t23-isp-config.h>

#define TX_ISP_MDNS_PLANES 9

enum tx_isp_mdns_profile {
	TX_ISP_MDNS_SC1A4T_LINEAR_DAY,
};

struct tx_isp_mdns_plane {
	u32 offset;
	u32 stride;
	u32 size;
};

struct tx_isp_mdns_layout {
	struct tx_isp_mdns_plane plane[TX_ISP_MDNS_PLANES];
	u32 size;
};

/* One allocation per sensor; callers serialize changes with the stream lock. */
struct tx_isp_mdns_history {
	struct tx_isp_mdns_layout layout;
	struct device *dev;
	void *cpu;
	dma_addr_t dma;
	u32 width;
	u32 height;
	bool active;
	bool valid;
};

extern const u16 tx_isp_mdns_address_regs[TX_ISP_MDNS_PLANES];

/* Compact history layout for TX_ISP_MDNS_SC1A4T_LINEAR_DAY. */
int tx_isp_mdns_layout(u32 width, u32 height,
		       struct tx_isp_mdns_layout *layout);
int tx_isp_mdns_alloc(struct device *dev, struct tx_isp_mdns_history *history,
		     u32 width, u32 height);
int tx_isp_mdns_free(struct tx_isp_mdns_history *history);
int tx_isp_mdns_invalidate(struct tx_isp_mdns_history *history);
/* Pipeline must be idle. Gain is log2(total gain) in Q16; zero means 1x. */
int tx_isp_mdns_configure_bypassed(void __iomem *base, u32 width, u32 height,
				 enum tx_isp_mdns_profile profile,
				 u32 gain_log2_q16);
/* Pipeline must be idle. Gain is log2(total gain) in Q16; zero means 1x. */
/* A non-NULL @cfg replaces the profile; the grid still follows the frame. */
int tx_isp_mdns_start(struct tx_isp_mdns_history *history, void __iomem *base,
		      enum tx_isp_mdns_profile profile, u32 gain_log2_q16,
		      const struct t23_isp_mdns_config *cfg);
int tx_isp_mdns_check_config(const struct t23_isp_mdns_config *cfg);
void tx_isp_mdns_frame_done(struct tx_isp_mdns_history *history, bool error);
int tx_isp_mdns_bypass(struct tx_isp_mdns_history *history, void __iomem *base);

#endif
