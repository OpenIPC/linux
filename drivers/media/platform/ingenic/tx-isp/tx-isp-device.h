/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_DEVICE_H__
#define __TX_ISP_DEVICE_H__

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <media/v4l2-device.h>
#include "tx-isp-common.h"
#include "tx-isp-regs.h"
#include "tx-isp-mdns.h"
#include "tx-isp-params.h"
#include "tx-isp-stats.h"

enum {
	TX_ISP_SUBDEV_VIC = 0,
	TX_ISP_SUBDEV_CSI = 1,
	TX_ISP_SUBDEV_CORE = 2,
	TX_ISP_SUBDEV_MAX = 3,
};

struct isp_irq_binding {
	struct tx_isp_device *owner;
	struct tx_isp_subdev *sd;
	int irq;
	bool requested;
	bool enabled;
};

struct tx_isp_device {
	struct platform_device *pdev;
	struct tx_isp_subdev *subdevs[TX_ISP_SUBDEV_MAX];
	unsigned int subdev_cnt;
	struct isp_irq_binding bindings[TX_ISP_SUBDEV_MAX];
	bool streaming;
	bool processed;
	bool dma_quiescence_failed;
	struct clk *cgu_isp;
	struct clk *isp_gate;
	struct clk *csi_gate;
	struct clk *cgu_vpu;
	struct clk *ivdc_gate;
	void __iomem *ivdc_base;
	struct reset_control *reset;
	void *dma_buffer;
	dma_addr_t dma_address;
	size_t dma_size;
	void *scratch_buf;
	dma_addr_t scratch_dma;
	size_t scratch_size;
	void *video_priv;
};

extern struct tx_isp_device *g_tx_isp_dev;

int tx_isp_request_irq(struct platform_device *pdev, struct tx_isp_device *isp);
void tx_isp_free_irq(struct tx_isp_device *isp);
void tx_isp_enable_irq(struct tx_isp_device *isp);
void tx_isp_disable_irq(struct tx_isp_device *isp);

int tx_isp_vic_probe(struct platform_device *pdev);
int tx_isp_vic_remove(struct platform_device *pdev);
int tx_isp_vic_configure(struct tx_isp_subdev *sd, unsigned int width,
			 unsigned int height, unsigned int port);
void tx_isp_vic_dma_arm(struct tx_isp_subdev *sd, dma_addr_t address);
void tx_isp_vic_dma_resume(struct tx_isp_subdev *sd);
void tx_isp_vic_dma_stop(struct tx_isp_subdev *sd);
void tx_isp_vic_select_port(struct tx_isp_subdev *sd, unsigned int port);
int tx_isp_vic_start(struct tx_isp_subdev *sd);
void tx_isp_vic_capture_trigger(struct tx_isp_subdev *sd, bool enable);
int tx_isp_vic_stop(struct tx_isp_subdev *sd);
irqreturn_t isp_vic_interrupt_service_routine(struct tx_isp_subdev *sd,
					      bool *handled);

int tx_isp_csi_probe(struct platform_device *pdev);
int tx_isp_csi_remove(struct platform_device *pdev);
int csi_phy_start(struct tx_isp_subdev *sd);
int csi_phy_stop(struct tx_isp_subdev *sd);
void tx_isp_csi_select_rate(struct tx_isp_subdev *sd, unsigned int clock_mhz);
int tx_isp_csi_configure(struct tx_isp_subdev *sd, unsigned int lanes,
			 unsigned int clock_mhz);


/* MSCA channel 0 writes the full frame and channels 1-2 scaled copies. */
#define T23_MSCA_OUTPUTS	3
#define T23_SCALED_OUTPUTS	(T23_MSCA_OUTPUTS - 1)

struct tx_isp_msca_target {
	u16 width;		/* 0 leaves the channel off */
	u16 height;
	dma_addr_t y;
	dma_addr_t uv;
	bool hflip;
	bool vflip;
};

#define T23_MSCA_MASKS		4
#define T23_MSCA_LINES		6
/* OSD blocks per input; the scaler has eight, programmed for each frame */
#define T23_MSCA_OSD_REGIONS	8

struct tx_isp_rect {
	u16 left;
	u16 top;
	u16 width;
	u16 height;
};

struct tx_isp_line {
	u16 x0;
	u16 y0;
	u16 x1;
	u16 y1;
	u16 width;
};

struct tx_isp_msca_osd {
	dma_addr_t addr;	/* 0: off */
	struct tx_isp_rect rect;
	u32 stride;
};

/*
 * Overlays drawn by MSCA, in input pixels. Lines and OSD cover channel 0
 * only, and one pixel format applies to every OSD block of a frame.
 */
struct tx_isp_msca_overlay {
	struct tx_isp_rect mask[T23_MSCA_MASKS];	/* zero size: off */
	u32 mask_yuv;		/* Y << 16 | U << 8 | V */
	struct tx_isp_line line[T23_MSCA_LINES];	/* zero width: off */
	u32 line_yuv;		/* Y << 16 | U << 8 | V */
	u32 osd_format;		/* V4L2_PIX_FMT_* */
	struct tx_isp_msca_osd osd[T23_MSCA_OSD_REGIONS];
};

struct tx_isp_core_device {
	struct tx_isp_subdev sd;
	struct t23_isp_stats stats;
	u32 stats_irq;		/* statistics done bits of the current frame */
	struct tx_isp_params_state *params;	/* one per input */
	int last_params_input;			/* input currently loaded in HW */
	u32 hw_custom_mask;			/* params blocks active in HW */
	struct tx_isp_mdns_history history[2];
	bool mdns_enabled;
	unsigned int mdns_input;
	bool running;
	u32 input_control;
	u32 geometry;
	struct tx_isp_msca_target armed[T23_MSCA_OUTPUTS];
	u32 armed_mask;		/* MSCA channels writing the current frame */
	u32 done_mask;
	u32 bayer_order;
	u32 active_inputs;
};

int tx_isp_core_probe(struct platform_device *pdev);
int tx_isp_core_remove(struct platform_device *pdev);
int tx_isp_core_s_stream(struct tx_isp_subdev *sd, int enable);
int tx_isp_core_mdns_start(struct tx_isp_subdev *sd, unsigned int input);
void tx_isp_core_queue_ext_params(struct tx_isp_subdev *sd, unsigned int input,
				  const struct t23_isp_ext_params_cfg *ext,
				  size_t payload);
void tx_isp_core_apply_params(struct tx_isp_subdev *sd, unsigned int input);
void tx_isp_core_reset_params(struct tx_isp_subdev *sd, unsigned int input);
void tx_isp_core_mdns_frame_done(struct tx_isp_subdev *sd);
extern const s16 tx_isp_default_ccm[9];
int tx_isp_core_dma_arm(struct tx_isp_subdev *sd,
			const struct tx_isp_msca_target out[T23_MSCA_OUTPUTS],
			const struct tx_isp_msca_overlay *overlay,
			unsigned int input);
void tx_isp_core_dma_stop(struct tx_isp_subdev *sd, bool abort);
void tx_isp_core_park_channel(struct tx_isp_subdev *sd, unsigned int ch,
			      dma_addr_t scratch_dma);
int tx_isp_core_output_init(struct tx_isp_subdev *sd);
void tx_isp_core_restore_dmsc(struct tx_isp_subdev *sd);
void tx_isp_core_dmsc_defaults(u32 values[3]);
int t23_isp_configure_input(struct tx_isp_device *isp,
			    struct v4l2_subdev *sensor, unsigned int port,
			    u32 active_inputs);
void t23_isp_video_dma_done(struct tx_isp_device *isp, bool error);
void t23_isp_video_processed_event(struct tx_isp_device *isp, bool dma_done,
				 bool all_done, bool error);
void t23_isp_video_frame_event(struct tx_isp_device *isp);
u16 tx_isp_calc_gamma_entry(int j, u32 g);
int tx_isp_core_init_hardware(struct tx_isp_subdev *sd);
void tx_isp_core_set_blc_default(struct tx_isp_subdev *sd);
void tx_isp_core_set_dgain(struct tx_isp_subdev *sd, u32 dgain_q10);
void tx_isp_core_set_wb(struct tx_isp_subdev *sd, u32 r_gain_q8, u32 b_gain_q8);
int tx_isp_core_set_gamma(struct tx_isp_subdev *sd, u32 gamma_val);
void tx_isp_core_set_ccm(struct tx_isp_subdev *sd, const s16 ccm[9]);
void tx_isp_core_set_sharpness(struct tx_isp_subdev *sd, u32 sharpness);

int t23_isp_video_register(struct tx_isp_device *isp);
void t23_isp_video_unregister(void);

#endif
