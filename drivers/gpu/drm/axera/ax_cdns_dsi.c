// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright: 2017 Cadence Design Systems, Inc.
 *
 * Author: Boris Brezillon <boris.brezillon@bootlin.com>
 */

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_drv.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <video/mipi_display.h>

#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>

#include <linux/phy/phy.h>
#include "ax_mipi_dsi.h"
#include <linux/ax_display_hal.h>

#define IP_CONF				0x0
#define SP_HS_FIFO_DEPTH(x)		(((x) & GENMASK(30, 26)) >> 26)
#define SP_LP_FIFO_DEPTH(x)		(((x) & GENMASK(25, 21)) >> 21)
#define VRS_FIFO_DEPTH(x)		(((x) & GENMASK(20, 16)) >> 16)
#define DIRCMD_FIFO_DEPTH(x)		(((x) & GENMASK(15, 13)) >> 13)
#define SDI_IFACE_32			BIT(12)
#define INTERNAL_DATAPATH_32		(0 << 10)
#define INTERNAL_DATAPATH_16		(1 << 10)
#define INTERNAL_DATAPATH_8		(3 << 10)
#define INTERNAL_DATAPATH_SIZE		((x) & GENMASK(11, 10))
#define NUM_IFACE(x)			((((x) & GENMASK(9, 8)) >> 8) + 1)
#define MAX_LANE_NB(x)			(((x) & GENMASK(7, 6)) >> 6)
#define RX_FIFO_DEPTH(x)		((x) & GENMASK(5, 0))

#define MCTL_MAIN_DATA_CTL		0x4
#define TE_MIPI_POLLING_EN		BIT(25)
#define TE_HW_POLLING_EN		BIT(24)
#define DISP_EOT_GEN			BIT(18)
#define HOST_EOT_GEN			BIT(17)
#define DISP_GEN_CHECKSUM		BIT(16)
#define DISP_GEN_ECC			BIT(15)
#define BTA_EN				BIT(14)
#define READ_EN				BIT(13)
#define REG_TE_EN			BIT(12)
#define IF_TE_EN(x)			BIT(8 + (x))
#define TVG_SEL				BIT(6)
#define VID_EN				BIT(5)
#define IF_VID_SELECT(x)		((x) << 2)
#define IF_VID_SELECT_MASK		GENMASK(3, 2)
#define IF_VID_MODE			BIT(1)
#define LINK_EN				BIT(0)

#define MCTL_MAIN_PHY_CTL		0x8
#define HS_INVERT_DAT(x)		BIT(19 + ((x) * 2))
#define SWAP_PINS_DAT(x)		BIT(18 + ((x) * 2))
#define HS_INVERT_CLK			BIT(17)
#define SWAP_PINS_CLK			BIT(16)
#define HS_SKEWCAL_EN			BIT(15)
#define WAIT_BURST_TIME(x)		((x) << 10)
#define DATA_ULPM_EN(x)			BIT(6 + (x))
#define CLK_ULPM_EN			BIT(5)
#define CLK_CONTINUOUS			BIT(4)
#define DATA_LANE_EN(x)			BIT((x) - 1)

#define MCTL_MAIN_EN			0xc
#define DATA_FORCE_STOP			BIT(17)
#define CLK_FORCE_STOP			BIT(16)
#define IF_EN(x)			BIT(13 + (x))
#define DATA_LANE_ULPM_REQ(l)		BIT(9 + (l))
#define CLK_LANE_ULPM_REQ		BIT(8)
#define DATA_LANE_START(x)		BIT(4 + (x))
#define CLK_LANE_EN			BIT(3)
#define PLL_START			BIT(0)

#define MCTL_DPHY_CFG0			0x10
#define DPHY_C_RSTB			BIT(20)
#define DPHY_D_RSTB(x)			GENMASK(15 + (x), 16)
#define DPHY_PLL_PDN			BIT(10)
#define DPHY_CMN_PDN			BIT(9)
#define DPHY_C_PDN			BIT(8)
#define DPHY_D_PDN(x)			GENMASK(3 + (x), 4)
#define DPHY_ALL_D_PDN			GENMASK(7, 4)
#define DPHY_PLL_PSO			BIT(1)
#define DPHY_CMN_PSO			BIT(0)

#define MCTL_DPHY_TIMEOUT1		0x14
#define HSTX_TIMEOUT(x)			((x) << 4)
#define HSTX_TIMEOUT_MAX		GENMASK(17, 0)
#define CLK_DIV(x)			(x)
#define CLK_DIV_MAX			GENMASK(3, 0)

#define MCTL_DPHY_TIMEOUT2		0x18
#define LPRX_TIMEOUT(x)			(x)

#define MCTL_ULPOUT_TIME		0x1c
#define DATA_LANE_ULPOUT_TIME(x)	((x) << 9)
#define CLK_LANE_ULPOUT_TIME(x)		(x)

#define MCTL_3DVIDEO_CTL		0x20
#define VID_VSYNC_3D_EN			BIT(7)
#define VID_VSYNC_3D_LR			BIT(5)
#define VID_VSYNC_3D_SECOND_EN		BIT(4)
#define VID_VSYNC_3DFORMAT_LINE		(0 << 2)
#define VID_VSYNC_3DFORMAT_FRAME	(1 << 2)
#define VID_VSYNC_3DFORMAT_PIXEL	(2 << 2)
#define VID_VSYNC_3DMODE_OFF		0
#define VID_VSYNC_3DMODE_PORTRAIT	1
#define VID_VSYNC_3DMODE_LANDSCAPE	2

#define MCTL_MAIN_STS			0x24
#define MCTL_MAIN_STS_CTL		0x130
#define MCTL_MAIN_STS_CLR		0x150
#define MCTL_MAIN_STS_FLAG		0x170
#define HS_SKEWCAL_DONE			BIT(11)
#define IF_UNTERM_PKT_ERR(x)		BIT(8 + (x))
#define LPRX_TIMEOUT_ERR		BIT(7)
#define HSTX_TIMEOUT_ERR		BIT(6)
#define DATA_LANE_RDY(l)		BIT(2 + (l))
#define CLK_LANE_RDY			BIT(1)
#define PLL_LOCKED			BIT(0)

#define MCTL_DPHY_ERR			0x28
#define MCTL_DPHY_ERR_CTL1		0x148
#define MCTL_DPHY_ERR_CLR		0x168
#define MCTL_DPHY_ERR_FLAG		0x188
#define ERR_CONT_LP(x, l)		BIT(18 + ((x) * 4) + (l))
#define ERR_CONTROL(l)			BIT(14 + (l))
#define ERR_SYNESC(l)			BIT(10 + (l))
#define ERR_ESC(l)			BIT(6 + (l))

#define MCTL_DPHY_ERR_CTL2		0x14c
#define ERR_CONT_LP_EDGE(x, l)		BIT(12 + ((x) * 4) + (l))
#define ERR_CONTROL_EDGE(l)		BIT(8 + (l))
#define ERR_SYN_ESC_EDGE(l)		BIT(4 + (l))
#define ERR_ESC_EDGE(l)			BIT(0 + (l))

#define MCTL_LANE_STS			0x2c
#define PPI_C_TX_READY_HS		BIT(18)
#define DPHY_PLL_LOCK			BIT(17)
#define PPI_D_RX_ULPS_ESC(x)		(((x) & GENMASK(15, 12)) >> 12)
#define LANE_STATE_START		0
#define LANE_STATE_IDLE			1
#define LANE_STATE_WRITE		2
#define LANE_STATE_ULPM			3
#define LANE_STATE_READ			4
#define DATA_LANE_STATE(l, val)		\
	(((val) >> (2 + 2 * (l) + ((l) ? 1 : 0))) & GENMASK((l) ? 1 : 2, 0))
#define CLK_LANE_STATE_HS		2
#define CLK_LANE_STATE(val)		((val) & GENMASK(1, 0))

#define DSC_MODE_CTL			0x30
#define DSC_MODE_EN			BIT(0)

#define DSC_CMD_SEND			0x34
#define DSC_SEND_PPS			BIT(0)
#define DSC_EXECUTE_QUEUE		BIT(1)

#define DSC_PPS_WRDAT			0x38

#define DSC_MODE_STS			0x3c
#define DSC_PPS_DONE			BIT(1)
#define DSC_EXEC_DONE			BIT(2)

#define CMD_MODE_CTL			0x70
#define IF_LP_EN(x)			BIT(9 + (x))
#define IF_VCHAN_ID(x, c)		((c) << ((x) * 2))

#define CMD_MODE_CTL2			0x74
#define TE_TIMEOUT(x)			((x) << 11)
#define FILL_VALUE(x)			((x) << 3)
#define ARB_IF_WITH_HIGHEST_PRIORITY(x)	((x) << 1)
#define ARB_ROUND_ROBIN_MODE		BIT(0)

#define CMD_MODE_STS			0x78
#define CMD_MODE_STS_CTL		0x134
#define CMD_MODE_STS_CLR		0x154
#define CMD_MODE_STS_FLAG		0x174
#define ERR_IF_UNDERRUN(x)		BIT(4 + (x))
#define ERR_UNWANTED_READ		BIT(3)
#define ERR_TE_MISS			BIT(2)
#define ERR_NO_TE			BIT(1)
#define CSM_RUNNING			BIT(0)

#define DIRECT_CMD_SEND			0x80

#define DIRECT_CMD_MAIN_SETTINGS	0x84
#define TRIGGER_VAL(x)			((x) << 25)
#define CMD_LP_EN			BIT(24)
#define CMD_SIZE(x)			((x) << 16)
#define CMD_VCHAN_ID(x)			((x) << 14)
#define CMD_DATATYPE(x)			((x) << 8)
#define CMD_LONG			BIT(3)
#define WRITE_CMD			0
#define READ_CMD			1
#define TE_REQ				4
#define TRIGGER_REQ			5
#define BTA_REQ				6

#define DIRECT_CMD_STS			0x88
#define DIRECT_CMD_STS_CTL		0x138
#define DIRECT_CMD_STS_CLR		0x158
#define DIRECT_CMD_STS_FLAG		0x178
#define RCVD_ACK_VAL(val)		((val) >> 16)
#define RCVD_TRIGGER_VAL(val)		(((val) & GENMASK(14, 11)) >> 11)
#define READ_COMPLETED_WITH_ERR		BIT(10)
#define BTA_FINISHED			BIT(9)
#define BTA_COMPLETED			BIT(8)
#define TE_RCVD				BIT(7)
#define TRIGGER_RCVD			BIT(6)
#define ACK_WITH_ERR_RCVD		BIT(5)
#define ACK_RCVD			BIT(4)
#define READ_COMPLETED			BIT(3)
#define TRIGGER_COMPLETED		BIT(2)
#define WRITE_COMPLETED			BIT(1)
#define SENDING_CMD			BIT(0)

#define DIRECT_CMD_STOP_READ		0x8c

#define DIRECT_CMD_WRDATA		0x90

#define DIRECT_CMD_FIFO_RST		0x94

#define DIRECT_CMD_RDDATA		0xa0

#define DIRECT_CMD_RD_PROPS		0xa4
#define RD_DCS				BIT(18)
#define RD_VCHAN_ID(val)		(((val) >> 16) & GENMASK(1, 0))
#define RD_SIZE(val)			((val) & GENMASK(15, 0))

#define DIRECT_CMD_RD_STS		0xa8
#define DIRECT_CMD_RD_STS_CTL		0x13c
#define DIRECT_CMD_RD_STS_CLR		0x15c
#define DIRECT_CMD_RD_STS_FLAG		0x17c
#define ERR_EOT_WITH_ERR		BIT(8)
#define ERR_MISSING_EOT			BIT(7)
#define ERR_WRONG_LENGTH		BIT(6)
#define ERR_OVERSIZE			BIT(5)
#define ERR_RECEIVE			BIT(4)
#define ERR_UNDECODABLE			BIT(3)
#define ERR_CHECKSUM			BIT(2)
#define ERR_UNCORRECTABLE		BIT(1)
#define ERR_FIXED			BIT(0)

#define VID_MAIN_CTL			0xb0
#define VID_IGNORE_MISS_VSYNC		BIT(31)
#define VID_FIELD_SW			BIT(28)
#define VID_INTERLACED_EN		BIT(27)
#define RECOVERY_MODE(x)		((x) << 25)
#define RECOVERY_MODE_NEXT_HSYNC	0
#define RECOVERY_MODE_NEXT_STOP_POINT	2
#define RECOVERY_MODE_NEXT_VSYNC	3
#define REG_BLKEOL_MODE(x)		((x) << 23)
#define REG_BLKLINE_MODE(x)		((x) << 21)
#define REG_BLK_MODE_NULL_PKT		0
#define REG_BLK_MODE_BLANKING_PKT	1
#define REG_BLK_MODE_LP			2
#define SYNC_PULSE_HORIZONTAL		BIT(20)
#define SYNC_PULSE_ACTIVE		BIT(19)
#define BURST_MODE			BIT(18)
#define VID_PIXEL_MODE_MASK		GENMASK(17, 14)
#define VID_PIXEL_MODE_RGB565		(0 << 14)
#define VID_PIXEL_MODE_RGB666_PACKED	(1 << 14)
#define VID_PIXEL_MODE_RGB666		(2 << 14)
#define VID_PIXEL_MODE_RGB888		(3 << 14)
#define VID_PIXEL_MODE_RGB101010	(4 << 14)
#define VID_PIXEL_MODE_RGB121212	(5 << 14)
#define VID_PIXEL_MODE_YUV420		(8 << 14)
#define VID_PIXEL_MODE_YUV422_PACKED	(9 << 14)
#define VID_PIXEL_MODE_YUV422		(10 << 14)
#define VID_PIXEL_MODE_YUV422_24B	(11 << 14)
#define VID_PIXEL_MODE_DSC_COMP		(12 << 14)
#define VID_DATATYPE(x)			((x) << 8)
#define VID_VIRTCHAN_ID(iface, x)	((x) << (4 + (iface) * 2))
#define STOP_MODE(x)			((x) << 2)
#define START_MODE(x)			(x)

#define VID_VSIZE1			0xb4
#define VFP_LEN(x)			((x) << 12)
#define VBP_LEN(x)			((x) << 6)
#define VSA_LEN(x)			(x)

#define VID_VSIZE2			0xb8
#define VACT_LEN(x)			(x)

#define VID_HSIZE1			0xc0
#define HBP_LEN(x)			((x) << 16)
#define HSA_LEN(x)			(x)

#define VID_HSIZE2			0xc4
#define HFP_LEN(x)			((x) << 16)
#define HACT_LEN(x)			(x)

#define VID_BLKSIZE1			0xcc
#define BLK_EOL_PKT_LEN(x)		((x) << 15)
#define BLK_LINE_EVENT_PKT_LEN(x)	(x)

#define VID_BLKSIZE2			0xd0
#define BLK_LINE_PULSE_PKT_LEN(x)	(x)

#define VID_PKT_TIME			0xd8
#define BLK_EOL_DURATION(x)		(x)

#define VID_DPHY_TIME			0xdc
#define REG_WAKEUP_TIME(x)		((x) << 17)
#define REG_LINE_DURATION(x)		(x)

#define VID_ERR_COLOR1			0xe0
#define COL_GREEN(x)			((x) << 12)
#define COL_RED(x)			(x)

#define VID_ERR_COLOR2			0xe4
#define PAD_VAL(x)			((x) << 12)
#define COL_BLUE(x)			(x)

#define VID_VPOS			0xe8
#define LINE_VAL(val)			(((val) & GENMASK(14, 2)) >> 2)
#define LINE_POS(val)			((val) & GENMASK(1, 0))

#define VID_HPOS			0xec
#define HORIZ_VAL(val)			(((val) & GENMASK(17, 3)) >> 3)
#define HORIZ_POS(val)			((val) & GENMASK(2, 0))

#define VID_MODE_STS			0xf0
#define VID_MODE_STS_CTL		0x140
#define VID_MODE_STS_CLR		0x160
#define VID_MODE_STS_FLAG		0x180
#define VSG_RECOVERY			BIT(10)
#define ERR_VRS_WRONG_LEN		BIT(9)
#define ERR_LONG_READ			BIT(8)
#define ERR_LINE_WRITE			BIT(7)
#define ERR_BURST_WRITE			BIT(6)
#define ERR_SMALL_HEIGHT		BIT(5)
#define ERR_SMALL_LEN			BIT(4)
#define ERR_MISSING_VSYNC		BIT(3)
#define ERR_MISSING_HSYNC		BIT(2)
#define ERR_MISSING_DATA		BIT(1)
#define VSG_RUNNING			BIT(0)

#define VID_VCA_SETTING1		0xf4
#define BURST_LP			BIT(16)
#define MAX_BURST_LIMIT(x)		(x)

#define VID_VCA_SETTING2		0xf8
#define MAX_LINE_LIMIT(x)		((x) << 16)
#define EXACT_BURST_LIMIT(x)		(x)

#define TVG_CTL				0xfc
#define TVG_STRIPE_SIZE(x)		((x) << 5)
#define TVG_MODE_MASK			GENMASK(4, 3)
#define TVG_MODE_SINGLE_COLOR		(0 << 3)
#define TVG_MODE_VSTRIPES		(2 << 3)
#define TVG_MODE_HSTRIPES		(3 << 3)
#define TVG_STOPMODE_MASK		GENMASK(2, 1)
#define TVG_STOPMODE_EOF		(0 << 1)
#define TVG_STOPMODE_EOL		(1 << 1)
#define TVG_STOPMODE_NOW		(2 << 1)
#define TVG_RUN				BIT(0)

#define TVG_IMG_SIZE			0x100
#define TVG_NBLINES(x)			((x) << 16)
#define TVG_LINE_SIZE(x)		(x)

#define TVG_COLOR1			0x104
#define TVG_COL1_GREEN(x)		((x) << 12)
#define TVG_COL1_RED(x)			(x)

#define TVG_COLOR1_BIS			0x108
#define TVG_COL1_BLUE(x)		(x)

#define TVG_COLOR2			0x10c
#define TVG_COL2_GREEN(x)		((x) << 12)
#define TVG_COL2_RED(x)			(x)

#define TVG_COLOR2_BIS			0x110
#define TVG_COL2_BLUE(x)		(x)

#define TVG_STS				0x114
#define TVG_STS_CTL			0x144
#define TVG_STS_CLR			0x164
#define TVG_STS_FLAG			0x184
#define TVG_STS_RUNNING			BIT(0)

#define STS_CTL_EDGE(e)			((e) << 16)

#define DPHY_LANES_MAP			0x198
#define DAT_REMAP_CFG(b, l)		((l) << ((b) * 8))

#define DPI_IRQ_EN			0x1a0
#define DPI_IRQ_CLR			0x1a4
#define DPI_IRQ_STS			0x1a8
#define PIXEL_BUF_OVERFLOW		BIT(0)

#define DPI_CFG				0x1ac
#define DPI_CFG_FIFO_DEPTH(x)		((x) >> 16)
#define DPI_CFG_FIFO_LEVEL(x)		((x) & GENMASK(15, 0))

#define TEST_GENERIC			0x1f0
#define TEST_STATUS(x)			((x) >> 16)
#define TEST_CTRL(x)			(x)

#define ID_REG				0x1fc
#define REV_VENDOR_ID(x)		(((x) & GENMASK(31, 20)) >> 20)
#define REV_PRODUCT_ID(x)		(((x) & GENMASK(19, 12)) >> 12)
#define REV_HW(x)			(((x) & GENMASK(11, 8)) >> 8)
#define REV_MAJOR(x)			(((x) & GENMASK(7, 4)) >> 4)
#define REV_MINOR(x)			((x) & GENMASK(3, 0))

#define DSI_OUTPUT_PORT			0
#define DSI_INPUT_PORT(inputid)		(1 + (inputid))

#define DSI_HBP_FRAME_OVERHEAD		12
#define DSI_HSA_FRAME_OVERHEAD		14
#define DSI_HFP_FRAME_OVERHEAD		6
#define DSI_HSS_VSS_VSE_FRAME_OVERHEAD	4
#define DSI_BLANKING_FRAME_OVERHEAD	6
#define DSI_NULL_FRAME_OVERHEAD		6
#define DSI_EOT_PKT_SIZE		4

#define DSI_VFP_SIZE			1


static inline struct cdns_dsi *input_to_dsi(struct cdns_dsi_input *input)
{
	return container_of(input, struct cdns_dsi, input);
}

static inline struct cdns_dsi *to_cdns_dsi(struct mipi_dsi_host *host)
{
	return container_of(host, struct cdns_dsi, base);
}

static inline struct cdns_dsi_input *
bridge_to_cdns_dsi_input(struct drm_bridge *bridge)
{
	return container_of(bridge, struct cdns_dsi_input, bridge);
}

static int cdns_dsi_get_dphy_cfg(struct cdns_dphy *dphy,
				struct cdns_dphy_cfg *cfg,
				unsigned int dpi_htotal,
				unsigned int dpi_bpp,
				unsigned int dpi_hz,
				unsigned int dsi_htotal,
				unsigned int dsi_nlanes,
				unsigned int *dsi_hfp_ext)
{
	u64 dlane_bps, adj_dsi_htotal;

	DRM_INFO("%s dpi_htotal: %d, dpi_bpp: %d, dpi_hz: %d, dsi_htotal = %d, dsi_nlanes = %d\n",
		 __func__, dpi_htotal, dpi_bpp, dpi_hz, dsi_htotal, dsi_nlanes);

	memset(cfg, 0, sizeof(*cfg));

	cfg->nlanes = dsi_nlanes;

	/*
	 * Make sure DSI htotal is aligned on a lane boundary when calculating
	 * the expected data rate. This is done by extending HFP in case of
	 * misalignment.
	 */
	adj_dsi_htotal = dsi_htotal;
	if (dsi_htotal % dsi_nlanes)
		adj_dsi_htotal += dsi_nlanes - (dsi_htotal % dsi_nlanes);

	dlane_bps = (u64)dpi_hz * adj_dsi_htotal;

	/* data rate in bytes/sec is not an integer, refuse the mode. */
	if (do_div(dlane_bps, dsi_nlanes * dpi_htotal)) {
		DRM_ERROR("dlane_bps = %lld, dsi_nlanes = %d, dpi_htotal = %d\n",
			dlane_bps, dsi_nlanes, dpi_htotal);
		return -EINVAL;
	}

	/* data rate was in bytes/sec, convert to bits/sec. */
	dlane_bps *= 8;

	*dsi_hfp_ext = adj_dsi_htotal - dsi_htotal;

	dlane_bps = DIV_ROUND_DOWN_ULL((u64)dpi_hz * adj_dsi_htotal * 8,
				       dsi_nlanes * dpi_htotal);
	cfg->lane_bps = dlane_bps;

	return 0;
}

static unsigned int dpi_to_dsi_timing(unsigned int dpi_timing,
				      unsigned int dpi_bpp,
				      unsigned int dsi_pkt_overhead)
{
	unsigned int dsi_timing = DIV_ROUND_UP(dpi_timing * dpi_bpp, 8);

	if (dsi_timing < dsi_pkt_overhead)
		dsi_timing = 0;
	else
		dsi_timing -= dsi_pkt_overhead;

	return dsi_timing;
}

static unsigned long cdns_dphy_get_wakeup_time_ns(struct cdns_dphy *dphy)
{
	return dphy->ops->get_wakeup_time_ns(dphy);
}

int cdns_dsi_mode2cfg(struct cdns_dsi *dsi,
		      const struct drm_display_mode *mode,
		      struct cdns_dsi_cfg *dsi_cfg,
		      struct cdns_dphy_cfg *dphy_cfg,
		      bool mode_valid_check)
{
	struct cdns_dsi_output *output = &dsi->output;
	unsigned int tmp, dsi_hfp_ext = 0, dpi_hfp;
	bool sync_pulse = false;
	int bpp, nlanes, ret;
	unsigned long dsi_htotal = 0, dsi_hss_hsa_hse_hbp = 0;

	memset(dsi_cfg, 0, sizeof(*dsi_cfg));

	if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		sync_pulse = true;

	bpp = mipi_dsi_pixel_format_to_bpp(output->dev->format);
	nlanes = output->dev->lanes;

	if (mode_valid_check)
		tmp = mode->htotal -
		      (sync_pulse ? mode->hsync_end : mode->hsync_start);
	else
		tmp = mode->crtc_htotal -
		      (sync_pulse ?
		       mode->crtc_hsync_end : mode->crtc_hsync_start);

	if (sync_pulse) {
		dsi_cfg->hbp = dpi_to_dsi_timing(tmp, bpp, DSI_HBP_FRAME_OVERHEAD);
		dsi_hss_hsa_hse_hbp += dsi_cfg->hbp + DSI_HBP_FRAME_OVERHEAD;

		if (mode_valid_check)
			tmp = mode->hsync_end - mode->hsync_start;
		else
			tmp = mode->crtc_hsync_end - mode->crtc_hsync_start;

		dsi_cfg->hsa = dpi_to_dsi_timing(tmp, bpp,
						 DSI_HSA_FRAME_OVERHEAD);
		dsi_hss_hsa_hse_hbp += dsi_cfg->hsa + DSI_HSA_FRAME_OVERHEAD;
	} else {
		tmp *= CDNS_MODE_VIDEO_BURST_CLK_RATIO;
		dsi_cfg->hbp = dpi_to_dsi_timing(tmp, bpp, DSI_HBP_FRAME_OVERHEAD);
		dsi_cfg->hsa = 0;
	}

	dsi_cfg->hact = dpi_to_dsi_timing(mode_valid_check ?
					  mode->hdisplay : mode->crtc_hdisplay,
					  bpp, 0);

	if (!sync_pulse)
		dsi_cfg->hbp += dsi_cfg->hact*(CDNS_MODE_VIDEO_BURST_CLK_RATIO - 1) + 10;

	if (mode_valid_check)
		dpi_hfp = mode->hsync_start - mode->hdisplay;
	else
		dpi_hfp = mode->crtc_hsync_start - mode->crtc_hdisplay;

	dsi_cfg->hfp = dpi_to_dsi_timing(dpi_hfp, bpp, DSI_HFP_FRAME_OVERHEAD);

	if (mode_valid_check)
		dsi_htotal = mode->htotal;
	else
		dsi_htotal = mode->crtc_htotal;
	dsi_htotal = dpi_to_dsi_timing(dsi_htotal, bpp, 0);

	DRM_INFO("%s nlanes = %d, dsi_hss_hsa_hse_hbp = %ld dsi_htotal:%ld\n",
		 __func__, nlanes, dsi_hss_hsa_hse_hbp, dsi_htotal);

	if (mode_valid_check)
		ret = cdns_dsi_get_dphy_cfg(dsi->dphy, dphy_cfg,
					   mode->htotal, bpp,
					   mode->clock * 1000,
					   dsi_htotal, nlanes,
					   &dsi_hfp_ext);
	else
		ret = cdns_dsi_get_dphy_cfg(dsi->dphy, dphy_cfg,
					   mode->crtc_htotal, bpp,
					   mode->crtc_clock * 1000,
					   dsi_htotal, nlanes,
					   &dsi_hfp_ext);

	if (ret) {
		DRM_INFO("%s ret = %d\n", __func__, ret);
		return 0;
	}

	DRM_INFO("%s dsi_hfp_ext = %d\n", __func__, dsi_hfp_ext);
	if (!dsi_hfp_ext)
		dsi_hfp_ext = 0;
	if (sync_pulse)
		dsi_cfg->hfp += dsi_hfp_ext;
	else
		dsi_cfg->hfp = 0;
	dsi_htotal += dsi_hfp_ext;
	dsi_cfg->htotal = dsi_htotal;

	/*
	 * Make sure DPI(HFP) > DSI(HSS+HSA+HSE+HBP) to guarantee that the FIFO
	 * is empty before we start a receiving a new line on the DPI
	 * interface.
	 */
	if ((u64)dphy_cfg->lane_bps * dpi_hfp * nlanes <
	    (u64)dsi_hss_hsa_hse_hbp *
	    (mode_valid_check ? mode->clock : mode->crtc_clock) * 1000) {
		DRM_INFO("%s err\n", __func__);
		return -EINVAL;
	}

	DRM_INFO("%s dsi hfp = %d, hsa = %d, hbp = %d, hact = %d, htotal = %d\n",
		__func__, dsi_cfg->hfp, dsi_cfg->hsa, dsi_cfg->hbp, dsi_cfg->hact, dsi_cfg->htotal);

	return 0;
}

static int cdns_dsi_bridge_attach(struct drm_bridge *bridge)
{
	struct cdns_dsi_input *input = bridge_to_cdns_dsi_input(bridge);
	struct cdns_dsi *dsi = input_to_dsi(input);
	struct cdns_dsi_output *output = &dsi->output;

	if (!drm_core_check_feature(bridge->dev, DRIVER_ATOMIC)) {
		dev_err(dsi->base.dev,
			"cdns-dsi driver is only compatible with DRM devices supporting atomic updates");
		return -ENOTSUPP;
	}

	return drm_bridge_attach(bridge->encoder, output->bridge, bridge);
}

static void cdns_dsi_bridge_disable(struct drm_bridge *bridge)
{
	struct cdns_dsi_input *input = bridge_to_cdns_dsi_input(bridge);
	struct cdns_dsi *dsi = input_to_dsi(input);
	u32 val;

	val = readl(dsi->regs + MCTL_MAIN_DATA_CTL);
	val &= ~(IF_VID_SELECT_MASK | IF_VID_MODE | VID_EN | HOST_EOT_GEN |
		 DISP_EOT_GEN);
	writel(val, dsi->regs + MCTL_MAIN_DATA_CTL);

	val = readl(dsi->regs + MCTL_MAIN_EN) & ~IF_EN(input->id);
	writel(val, dsi->regs + MCTL_MAIN_EN);

	pm_runtime_put(dsi->base.dev);
	dsi->link_initialized = false;
}

static void cdns_dsi_hs_init(struct cdns_dsi *dsi)
{
	u32 status;
	u32 val;

	val = readl(dsi->regs + MCTL_MAIN_STS);

	if (val & PLL_LOCKED) {
		/* Activate the PLL and wait until it's locked. */
		writel(PLL_LOCKED, dsi->regs + MCTL_MAIN_STS_CLR);

		WARN_ON_ONCE(readl_poll_timeout(dsi->regs + MCTL_MAIN_STS, status,
						status & PLL_LOCKED, 100, 100));
	}
}

static void cdns_dsi_init_link(struct cdns_dsi *dsi)
{
	struct cdns_dsi_output *output = &dsi->output;
	unsigned long sysclk = 0, sysclk_period, ulpout;
	u32 val;
	int i;

	if (dsi->link_initialized)
		return;

	val = 0;
	for (i = 1; i < output->dev->lanes; i++)
		val |= DATA_LANE_EN(i);

	if (!(output->dev->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS))
		val |= CLK_CONTINUOUS;

	if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		val |= WAIT_BURST_TIME(0xf);

	writel(val, dsi->regs + MCTL_MAIN_PHY_CTL);

	/* ULPOUT should be set to 1ms and is expressed in sysclk cycles. */
	sysclk = clk_get_rate(dsi->dsi_sys_clk);
	if (!sysclk) {
		DRM_ERROR("dsi sys clk not enabled\n");
		return;
	}
	sysclk_period = DIV_ROUND_DOWN_ULL(NSEC_PER_SEC, sysclk);
	ulpout = DIV_ROUND_UP(NSEC_PER_MSEC, sysclk_period);
	writel(CLK_LANE_ULPOUT_TIME(ulpout) | DATA_LANE_ULPOUT_TIME(ulpout),
	       dsi->regs + MCTL_ULPOUT_TIME);

	writel(LINK_EN, dsi->regs + MCTL_MAIN_DATA_CTL);

	val = CLK_LANE_EN | PLL_START;
	for (i = 0; i < output->dev->lanes; i++)
		val |= DATA_LANE_START(i);

	writel(val, dsi->regs + MCTL_MAIN_EN);

	dsi->link_initialized = true;
}

void cdns_dsi_bridge_enable(struct drm_bridge *bridge)
{
	struct cdns_dsi_input *input = bridge_to_cdns_dsi_input(bridge);
	struct cdns_dsi *dsi = input_to_dsi(input);
	struct cdns_dsi_output *output = &dsi->output;
	struct drm_display_mode *mode;
	unsigned long tx_byte_period, lane_bps;
	struct cdns_dsi_cfg dsi_cfg;
	u32 tmp, reg_wakeup, div;
	int nlanes;
	struct cdns_dphy_cfg dphy_cfg;
	u16 blkline_event_pck_len, blkeol_pck_len;

	if (ax_display_get_bootlogo_mode() == AX_DISP_OUT_MODE_DSI_DPI_VIDEO)
		return;

	if (WARN_ON(pm_runtime_get_sync(dsi->base.dev) < 0))
		return;

	mode = &bridge->encoder->crtc->state->adjusted_mode;
	nlanes = output->dev->lanes;

	WARN_ON_ONCE(cdns_dsi_mode2cfg(dsi, mode, &dsi_cfg, &dphy_cfg, false));

	cdns_dsi_hs_init(dsi);
	cdns_dsi_init_link(dsi);

	writel(HBP_LEN(dsi_cfg.hbp) | HSA_LEN(dsi_cfg.hsa), dsi->regs + VID_HSIZE1);
	writel(HFP_LEN(dsi_cfg.hfp) | HACT_LEN(dsi_cfg.hact), dsi->regs + VID_HSIZE2);

	writel(VBP_LEN(mode->crtc_vtotal - mode->crtc_vsync_end) |
	       VFP_LEN(DSI_VFP_SIZE) |
	       VSA_LEN(mode->crtc_vsync_end - mode->crtc_vsync_start),
	       dsi->regs + VID_VSIZE1);
	writel(mode->crtc_vdisplay, dsi->regs + VID_VSIZE2);

	tmp = dsi_cfg.htotal -
	      (dsi_cfg.hsa + DSI_BLANKING_FRAME_OVERHEAD +
	       DSI_HSA_FRAME_OVERHEAD);
	writel(BLK_LINE_PULSE_PKT_LEN(tmp), dsi->regs + VID_BLKSIZE2);

	writel(MAX_LINE_LIMIT(tmp - DSI_NULL_FRAME_OVERHEAD),
		dsi->regs + VID_VCA_SETTING2);
	if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		tmp = dsi_cfg.htotal -
	     		(DSI_HSS_VSS_VSE_FRAME_OVERHEAD + DSI_BLANKING_FRAME_OVERHEAD);
		writel(BLK_LINE_EVENT_PKT_LEN(tmp), dsi->regs + VID_BLKSIZE1);
	} else if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		writel(MAX_LINE_LIMIT(tmp - DSI_NULL_FRAME_OVERHEAD),
		       dsi->regs + VID_VCA_SETTING2);
		tmp = dsi_cfg.htotal * CDNS_MODE_VIDEO_BURST_CLK_RATIO;
		blkline_event_pck_len = tmp - 4;
		blkeol_pck_len = blkline_event_pck_len - (dsi_cfg.hact + DSI_BLANKING_FRAME_OVERHEAD + dsi_cfg.hbp + DSI_NULL_FRAME_OVERHEAD);
		writel((BLK_EOL_PKT_LEN(blkeol_pck_len)|BLK_LINE_EVENT_PKT_LEN(blkline_event_pck_len)), dsi->regs + VID_BLKSIZE1);
		tmp = DIV_ROUND_UP((blkeol_pck_len + 6), nlanes);
		writel(BLK_EOL_DURATION(tmp), dsi->regs + VID_PKT_TIME);/*0xd8 tmp code*/
	}

	if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		tmp = DIV_ROUND_UP(dsi_cfg.htotal, nlanes) - DIV_ROUND_UP(dsi_cfg.hsa, nlanes);
		lane_bps = dphy_cfg.lane_bps;
	} else if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		tmp = DIV_ROUND_UP(dsi_cfg.htotal, nlanes) * CDNS_MODE_VIDEO_BURST_CLK_RATIO;
		lane_bps = dphy_cfg.lane_bps * CDNS_MODE_VIDEO_BURST_CLK_RATIO;
	}

	if (!(output->dev->mode_flags & MIPI_DSI_MODE_EOT_PACKET))
		tmp -= DIV_ROUND_UP(DSI_EOT_PKT_SIZE, nlanes);

	tx_byte_period = DIV_ROUND_DOWN_ULL((u64)NSEC_PER_SEC * 8, lane_bps);
	DRM_INFO("%s lane_bps = %ld, tx_byte_period = %ld\n", __func__, dphy_cfg.lane_bps, tx_byte_period);

	reg_wakeup = cdns_dphy_get_wakeup_time_ns(dsi->dphy) / tx_byte_period;

	writel((REG_WAKEUP_TIME(reg_wakeup) | REG_LINE_DURATION(tmp)), dsi->regs + VID_DPHY_TIME);


	/*
	 * HSTX and LPRX timeouts are both expressed in TX byte clk cycles and
	 * both should be set to at least the time it takes to transmit a
	 * frame.
	 */
	tmp = NSEC_PER_SEC / drm_mode_vrefresh(mode);
	tmp /= tx_byte_period;

	for (div = 0; div <= CLK_DIV_MAX; div++) {
		if (tmp <= HSTX_TIMEOUT_MAX)
			break;

		tmp >>= 1;
	}

	if (tmp > HSTX_TIMEOUT_MAX)
		tmp = HSTX_TIMEOUT_MAX;


	writel(CLK_DIV(div) | HSTX_TIMEOUT(tmp), dsi->regs + MCTL_DPHY_TIMEOUT1);
	writel(LPRX_TIMEOUT(tmp), dsi->regs + MCTL_DPHY_TIMEOUT2);

	if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO) {
		switch (output->dev->format) {
		case MIPI_DSI_FMT_RGB888:
			tmp = VID_PIXEL_MODE_RGB888 |
			      VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_24);
			break;

		case MIPI_DSI_FMT_RGB666:
			tmp = VID_PIXEL_MODE_RGB666 |
			      VID_DATATYPE(MIPI_DSI_PIXEL_STREAM_3BYTE_18);
			break;

		case MIPI_DSI_FMT_RGB666_PACKED:
			tmp = VID_PIXEL_MODE_RGB666_PACKED |
			      VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_18);
			break;

		case MIPI_DSI_FMT_RGB565:
			tmp = VID_PIXEL_MODE_RGB565 |
			      VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_16);
			break;

		default:
			dev_err(dsi->base.dev, "Unsupported DSI format\n");
			return;
		}

		if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
			tmp |= SYNC_PULSE_ACTIVE | SYNC_PULSE_HORIZONTAL;
			tmp |= REG_BLKLINE_MODE(REG_BLK_MODE_BLANKING_PKT) |
			REG_BLKEOL_MODE(REG_BLK_MODE_BLANKING_PKT) |
			RECOVERY_MODE(RECOVERY_MODE_NEXT_HSYNC) |
			VID_IGNORE_MISS_VSYNC;
		} else if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
			tmp |= BURST_MODE;
			tmp |= REG_BLKLINE_MODE(REG_BLK_MODE_NULL_PKT) |
			REG_BLKEOL_MODE(REG_BLK_MODE_NULL_PKT) |
			RECOVERY_MODE(1) |
			VID_IGNORE_MISS_VSYNC;
		}

		writel(tmp, dsi->regs + VID_MAIN_CTL);
	}

	tmp = readl(dsi->regs + MCTL_MAIN_DATA_CTL);
	tmp &= ~(IF_VID_SELECT_MASK | HOST_EOT_GEN | IF_VID_MODE);

	if (!(output->dev->mode_flags & MIPI_DSI_MODE_EOT_PACKET))
		tmp |= HOST_EOT_GEN;

	if (output->dev->mode_flags & MIPI_DSI_MODE_VIDEO)
		tmp |= IF_VID_MODE | IF_VID_SELECT(input->id) | VID_EN;

	writel(tmp, dsi->regs + MCTL_MAIN_DATA_CTL);

	tmp = readl(dsi->regs + MCTL_MAIN_EN) | IF_EN(input->id);
	writel(tmp, dsi->regs + MCTL_MAIN_EN);
}

static const struct drm_bridge_funcs cdns_dsi_bridge_funcs = {
	.attach = cdns_dsi_bridge_attach,
	.disable = cdns_dsi_bridge_disable,
	.enable = cdns_dsi_bridge_enable,
};

static int cdns_dsi_attach(struct mipi_dsi_host *host,
			   struct mipi_dsi_device *dev)
{
	struct cdns_dsi *dsi = to_cdns_dsi(host);
	struct cdns_dsi_output *output = &dsi->output;
	struct cdns_dsi_input *input = &dsi->input;
	struct drm_bridge *bridge;
	struct drm_panel *panel;
	struct device_node *np;
	int ret;

	/*
	 * We currently do not support connecting several DSI devices to the
	 * same host. In order to support that we'd need the DRM bridge
	 * framework to allow dynamic reconfiguration of the bridge chain.
	 */
	if (output->dev)
		return -EBUSY;

	/*
	 * The host <-> device link might be described using an OF-graph
	 * representation, in this case we extract the device of_node from
	 * this representation, otherwise we use dsidev->dev.of_node which
	 * should have been filled by the core.
	 */
	np = of_graph_get_remote_node(dsi->base.dev->of_node, DSI_OUTPUT_PORT,
				      dev->channel);
	if (!np)
		np = of_node_get(dev->dev.of_node);

	panel = of_drm_find_panel(np);
	if (!IS_ERR(panel)) {
		bridge = drm_panel_bridge_add(panel, DRM_MODE_CONNECTOR_DSI);
	} else {
		bridge = of_drm_find_bridge(dev->dev.of_node);
		if (!bridge)
			bridge = ERR_PTR(-EINVAL);
	}

	of_node_put(np);

	if (IS_ERR(bridge)) {
		ret = PTR_ERR(bridge);
		dev_err(host->dev, "failed to add DSI device %s (err = %d)",
			dev->name, ret);
		return ret;
	}

	output->dev = dev;
	output->bridge = bridge;
	output->panel = panel;

	/*
	 * The DSI output has been properly configured, we can now safely
	 * register the input to the bridge framework so that it can take place
	 * in a display pipeline.
	 */
	drm_bridge_add(&input->bridge);

	return 0;
}

static int cdns_dsi_detach(struct mipi_dsi_host *host,
			   struct mipi_dsi_device *dev)
{
	struct cdns_dsi *dsi = to_cdns_dsi(host);
	struct cdns_dsi_output *output = &dsi->output;
	struct cdns_dsi_input *input = &dsi->input;

	drm_bridge_remove(&input->bridge);
	if (output->panel)
		drm_panel_bridge_remove(output->bridge);

	return 0;
}

static irqreturn_t cdns_dsi_interrupt(int irq, void *data)
{
	struct cdns_dsi *dsi = data;
	irqreturn_t ret = IRQ_NONE;
	u32 flag, ctl;

	flag = readl(dsi->regs + DIRECT_CMD_STS_FLAG);
	if (flag) {
		ctl = readl(dsi->regs + DIRECT_CMD_STS_CTL);
		ctl &= ~flag;
		writel(ctl, dsi->regs + DIRECT_CMD_STS_CTL);
		complete(&dsi->direct_cmd_comp);
		ret = IRQ_HANDLED;
	}

	return ret;
}

static ssize_t cdns_dsi_transfer(struct mipi_dsi_host *host,
				 const struct mipi_dsi_msg *msg)
{
	struct cdns_dsi *dsi = to_cdns_dsi(host);
	u32 cmd, sts, val, wait = WRITE_COMPLETED, ctl = 0;
	struct mipi_dsi_packet packet;
	int ret, i, tx_len, rx_len;

	ret = pm_runtime_get_sync(host->dev);
	if (ret < 0)
		return ret;

	cdns_dsi_init_link(dsi);

	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret)
		goto out;

	tx_len = msg->tx_buf ? msg->tx_len : 0;
	rx_len = msg->rx_buf ? msg->rx_len : 0;

	/* For read operations, the maximum TX len is 2. */
	if (rx_len && tx_len > 2) {
		ret = -ENOTSUPP;
		goto out;
	}

	/* TX len is limited by the CMD FIFO depth. */
	if (tx_len > dsi->direct_cmd_fifo_depth) {
		ret = -ENOTSUPP;
		goto out;
	}

	/* RX len is limited by the RX FIFO depth. */
	if (rx_len > dsi->rx_fifo_depth) {
		ret = -ENOTSUPP;
		goto out;
	}

	cmd = CMD_SIZE(tx_len) | CMD_VCHAN_ID(msg->channel) |
	      CMD_DATATYPE(msg->type);

	if (msg->flags & MIPI_DSI_MSG_USE_LPM)
		cmd |= CMD_LP_EN;

	if (mipi_dsi_packet_format_is_long(msg->type))
		cmd |= CMD_LONG;

	if (rx_len) {
		cmd |= READ_CMD;
		wait = READ_COMPLETED_WITH_ERR | READ_COMPLETED;
		ctl = READ_EN | BTA_EN;
	} else if (msg->flags & MIPI_DSI_MSG_REQ_ACK) {
		cmd |= BTA_REQ;
		wait = ACK_WITH_ERR_RCVD | ACK_RCVD;
		ctl = BTA_EN;
	}

	writel(readl(dsi->regs + MCTL_MAIN_DATA_CTL) | ctl,
	       dsi->regs + MCTL_MAIN_DATA_CTL);

	writel(cmd, dsi->regs + DIRECT_CMD_MAIN_SETTINGS);

	for (i = 0; i < tx_len; i += 4) {
		const u8 *buf = msg->tx_buf;
		int j;

		val = 0;
		for (j = 0; j < 4 && j + i < tx_len; j++)
			val |= (u32)buf[i + j] << (8 * j);

		writel(val, dsi->regs + DIRECT_CMD_WRDATA);
	}

	/* Clear status flags before sending the command. */
	writel(wait, dsi->regs + DIRECT_CMD_STS_CLR);

	writel(wait, dsi->regs + DIRECT_CMD_STS_CTL);

	reinit_completion(&dsi->direct_cmd_comp);
	writel(0, dsi->regs + DIRECT_CMD_SEND);

	ret = wait_for_completion_timeout(&dsi->direct_cmd_comp,
				    msecs_to_jiffies(1000));
	if (!ret) {
		DRM_ERROR("wait dsi0 direct cmd done timeout\n");
		ret = -ETIMEDOUT;
		goto out;
	}

	sts = readl(dsi->regs + DIRECT_CMD_STS);
	writel(wait, dsi->regs + DIRECT_CMD_STS_CLR);

	writel(0, dsi->regs + DIRECT_CMD_STS_CTL);

	writel(readl(dsi->regs + MCTL_MAIN_DATA_CTL) & ~ctl,
	       dsi->regs + MCTL_MAIN_DATA_CTL);

	/* We did not receive the events we were waiting for. */
	if (!(sts & wait)) {
		ret = -ETIMEDOUT;
		DRM_ERROR("timeout and not receive the events, sts:0x%x, wait:0x%x\n", sts, wait);
		goto out;
	}

	/* 'READ' or 'WRITE with ACK' failed. */
	if (sts & (READ_COMPLETED_WITH_ERR | ACK_WITH_ERR_RCVD)) {
		ret = -EIO;
		DRM_ERROR("read or write with ack err, sts:0x%x\n", sts);
		goto out;
	}

	for (i = 0; i < rx_len; i += 4) {
		u8 *buf = msg->rx_buf;
		int j;

		val = readl(dsi->regs + DIRECT_CMD_RDDATA);
		for (j = 0; j < 4 && j + i < rx_len; j++)
			buf[i + j] = val >> (8 * j);
	}

out:
	pm_runtime_put(host->dev);
	return ret;
}

static const struct mipi_dsi_host_ops cdns_dsi_ops = {
	.attach = cdns_dsi_attach,
	.detach = cdns_dsi_detach,
	.transfer = cdns_dsi_transfer,
};

void cdns_dsi_resume(struct device *dev)
{
	struct cdns_dsi *dsi = dev_get_drvdata(dev);
	reset_control_deassert(dsi->dsi_p_rst);
	reset_control_deassert(dsi->dsi_txpix_rst);
	reset_control_deassert(dsi->dsi_txesc_rst);
	reset_control_deassert(dsi->dsi_sys_rst);
	reset_control_deassert(dsi->dphy2dsi_rst);
	reset_control_deassert(dsi->dsi_rx_esc_rst);
	clk_prepare_enable(dsi->comm_dphytx_tlb_clk);
	clk_prepare_enable(dsi->dsi_txesc_clk);
	clk_prepare_enable(dsi->pll_ref_clk);
	clk_prepare_enable(dsi->dphytx_esc_clk);
	clk_prepare_enable(dsi->dsi_sys_clk);
	clk_prepare_enable(dsi->dsi_hs_clk);
	clk_prepare_enable(dsi->dsi_p_clk);
}
EXPORT_SYMBOL_GPL(cdns_dsi_resume);

void cdns_dsi_suspend(struct device *dev)
{
	struct cdns_dsi *dsi = dev_get_drvdata(dev);
	clk_disable_unprepare(dsi->dsi_p_clk);
	clk_disable_unprepare(dsi->dsi_sys_clk);
	clk_disable_unprepare(dsi->dsi_txesc_clk);
	clk_disable_unprepare(dsi->dsi_hs_clk);
	clk_disable_unprepare(dsi->pll_ref_clk);
	clk_disable_unprepare(dsi->dphytx_esc_clk);
	clk_disable_unprepare(dsi->comm_dphytx_tlb_clk);

	reset_control_assert(dsi->dsi_p_rst);
	reset_control_assert(dsi->dsi_txpix_rst);
	reset_control_assert(dsi->dsi_txesc_rst);
	reset_control_assert(dsi->dsi_sys_rst);
	reset_control_assert(dsi->dphy2dsi_rst);
	reset_control_assert(dsi->dsi_rx_esc_rst);
	reset_control_assert(dsi->dphytx_rst);

	dsi->link_initialized = false;
}
EXPORT_SYMBOL_GPL(cdns_dsi_suspend);

static unsigned long cdns_dphy_ref_get_wakeup_time_ns(struct cdns_dphy *dphy)
{
	/* Default wakeup time is 800 ns (in a simulated environment). */
	return 2000;
}
/*
 * This is the reference implementation of DPHY hooks. Specific integration of
 * this IP may have to re-implement some of them depending on how they decided
 * to wire things in the SoC.
 */
static const struct cdns_dphy_ops ref_dphy_ops = {
	.get_wakeup_time_ns = cdns_dphy_ref_get_wakeup_time_ns,
};

static const struct of_device_id cdns_dphy_of_match[] = {
	{ .compatible = "axera,dphy", .data = &ref_dphy_ops },
	{ /* sentinel */ },
};

static int cdns_dsi_drm_probe(struct platform_device *pdev)
{
	struct cdns_dsi *dsi;
	struct cdns_dsi_input *input;
	struct resource *res;
	int ret, irq;
	u32 val;

	dsi = devm_kzalloc(&pdev->dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;

	platform_set_drvdata(pdev, dsi);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dsi->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(dsi->regs))
		return PTR_ERR(dsi->regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	dsi->dispc_sys_regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(dsi->dispc_sys_regs))
		return PTR_ERR(dsi->dispc_sys_regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	dsi->dphytx_regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(dsi->dphytx_regs))
		return PTR_ERR(dsi->dphytx_regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	dsi->comm_sys_regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(dsi->comm_sys_regs))
		return PTR_ERR(dsi->comm_sys_regs);

	dsi->dsi_p_clk = devm_clk_get(&pdev->dev, "dsi_pclk");
	if (IS_ERR(dsi->dsi_p_clk))
		return PTR_ERR(dsi->dsi_p_clk);

	dsi->dsi_sys_clk = devm_clk_get(&pdev->dev, "dsi_sys_clk");
	if (IS_ERR(dsi->dsi_sys_clk))
		return PTR_ERR(dsi->dsi_sys_clk);

	dsi->dsi_txesc_clk = devm_clk_get(&pdev->dev, "dsi_txesc_clk");
	if (IS_ERR(dsi->dsi_txesc_clk))
		return PTR_ERR(dsi->dsi_txesc_clk);

	dsi->dsi_hs_clk = devm_clk_get(&pdev->dev, "dphy_hs_clk");
	if (IS_ERR(dsi->dsi_hs_clk))
		return PTR_ERR(dsi->dsi_hs_clk);

	dsi->pll_ref_clk = devm_clk_get(&pdev->dev, "dphy_ref_clk");
	if (IS_ERR(dsi->pll_ref_clk)) {
		return PTR_ERR(dsi->dsi_hs_clk);
	}

	dsi->dphytx_esc_clk = devm_clk_get(&pdev->dev, "dphy_esc_clk");
	if (IS_ERR(dsi->dphytx_esc_clk))
		return PTR_ERR(dsi->dphytx_esc_clk);

	dsi->comm_dphytx_tlb_clk = devm_clk_get(&pdev->dev, "comm_dphytx_tlb_clk");
	if (IS_ERR(dsi->comm_dphytx_tlb_clk))
		return PTR_ERR(dsi->comm_dphytx_tlb_clk);

	dsi->dsi_p_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_dsi_prst");
	if (IS_ERR(dsi->dsi_p_rst))
		return PTR_ERR(dsi->dsi_p_rst);

	dsi->dsi_txpix_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_txpix_rst");
	if (IS_ERR(dsi->dsi_txpix_rst))
		return PTR_ERR(dsi->dsi_txpix_rst);

	dsi->dsi_txesc_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_txesc_rst");
	if (IS_ERR(dsi->dsi_txesc_rst))
		return PTR_ERR(dsi->dsi_txesc_rst);

	dsi->dsi_sys_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_sys_rst");
	if (IS_ERR(dsi->dsi_sys_rst))
		return PTR_ERR(dsi->dsi_sys_rst);

	dsi->dsi_rx_esc_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_dsi_rx_esc_rst");
	if (IS_ERR(dsi->dsi_rx_esc_rst))
		return PTR_ERR(dsi->dsi_rx_esc_rst);

	dsi->dphytx_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_dphytx_rst");
	if (IS_ERR(dsi->dphytx_rst))
		return PTR_ERR(dsi->dphytx_rst);

	dsi->dphy2dsi_rst = devm_reset_control_get_optional_exclusive(&pdev->dev,
								"dispc_dphy2dsi_rst");
	if (IS_ERR(dsi->dphy2dsi_rst))
		return PTR_ERR(dsi->dphy2dsi_rst);

	if (ax_display_get_bootlogo_mode() != AX_DISP_OUT_MODE_DSI_DPI_VIDEO) {
		reset_control_deassert(dsi->dsi_p_rst);
		reset_control_deassert(dsi->dsi_txpix_rst);
		reset_control_deassert(dsi->dsi_txesc_rst);
		reset_control_deassert(dsi->dsi_sys_rst);
		reset_control_deassert(dsi->dphy2dsi_rst);
		reset_control_deassert(dsi->dsi_rx_esc_rst);
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	dsi->dphy = devm_kzalloc(&pdev->dev, sizeof(struct cdns_dphy), GFP_KERNEL);
	if (!dsi->dphy)
		return -ENOMEM;

	dsi->dphy->ops = &ref_dphy_ops;
	dsi->dphy->regs = dsi->dphytx_regs;
	dsi->dphy->power_regs =dsi->comm_sys_regs;

	if (ax_display_get_bootlogo_mode() != AX_DISP_OUT_MODE_DSI_DPI_VIDEO) {
		ret = clk_prepare_enable(dsi->dsi_p_clk);
		if (ret) {
			DRM_ERROR("enable dsi p clk failed\n");
			goto err_remove_dphy;
		}
	}

	val = readl(dsi->regs + ID_REG);
	if (REV_VENDOR_ID(val) != 0xcad) {
		dev_err(&pdev->dev, "invalid vendor id\n");
		ret = -EINVAL;
		goto err_disable_pclk;
	}

	val = readl(dsi->regs + IP_CONF);
	dsi->direct_cmd_fifo_depth = 1 << (DIRCMD_FIFO_DEPTH(val) + 2);
	dsi->rx_fifo_depth = RX_FIFO_DEPTH(val);

	init_completion(&dsi->direct_cmd_comp);

	input = &dsi->input;
	/*
	 * We only support the DPI input, so force input->id to
	 * CDNS_DPI_INPUT.
	 */
	input->id = CDNS_DPI_INPUT;
	input->bridge.funcs = &cdns_dsi_bridge_funcs;
	input->bridge.of_node = pdev->dev.of_node;
	if (ax_display_get_bootlogo_mode() != AX_DISP_OUT_MODE_DSI_DPI_VIDEO) {
		/* Mask all interrupts before registering the IRQ handler. */
		writel(0, dsi->regs + MCTL_MAIN_STS_CTL);
		writel(0, dsi->regs + MCTL_DPHY_ERR_CTL1);
		writel(0, dsi->regs + CMD_MODE_STS_CTL);
		writel(0, dsi->regs + DIRECT_CMD_STS_CTL);
		writel(0, dsi->regs + DIRECT_CMD_RD_STS_CTL);
		writel(0, dsi->regs + VID_MODE_STS_CTL);
		writel(0, dsi->regs + TVG_STS_CTL);
		writel(0, dsi->regs + DPI_IRQ_EN);
	}

	ret = devm_request_irq(&pdev->dev, irq, cdns_dsi_interrupt, 0,
			       dev_name(&pdev->dev), dsi);
	if (ret)
		goto err_disable_pclk;

	pm_runtime_enable(&pdev->dev);
	dsi->base.dev = &pdev->dev;
	dsi->base.ops = &cdns_dsi_ops;

	ret = mipi_dsi_host_register(&dsi->base);
	if (ret)
		goto err_disable_runtime_pm;

	return 0;

err_disable_runtime_pm:
	pm_runtime_disable(&pdev->dev);

err_disable_pclk:
	clk_disable_unprepare(dsi->dsi_p_clk);

err_remove_dphy:
	if (dsi->dphy->ops->remove)
		dsi->dphy->ops->remove(dsi->dphy);
	return ret;
}

static int cdns_dsi_drm_remove(struct platform_device *pdev)
{
	struct cdns_dsi *dsi = platform_get_drvdata(pdev);

	if (dsi->dphy->ops->remove)
		dsi->dphy->ops->remove(dsi->dphy);

	clk_disable_unprepare(dsi->dsi_p_clk);

	mipi_dsi_host_unregister(&dsi->base);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static const struct of_device_id cdns_dsi_of_match[] = {
	{ .compatible = "cdns,dsi" },
	{ },
};
MODULE_DEVICE_TABLE(of, cdns_dsi_of_match);

int cdns_mipi_dsi_bind(struct platform_device *pdev, struct drm_encoder *encoder)
{
	int ret;
	struct cdns_dsi *dsi;

	ret = cdns_dsi_drm_probe(pdev);
	if (ret)
		return ret;

	dsi = platform_get_drvdata(pdev);

	return drm_bridge_attach(encoder, &dsi->input.bridge, NULL);
}

EXPORT_SYMBOL_GPL(cdns_mipi_dsi_bind);

void cdns_mipi_dsi_unbind(struct platform_device *pdev)
{
	cdns_dsi_drm_remove(pdev);
}
EXPORT_SYMBOL_GPL(cdns_mipi_dsi_unbind);

MODULE_AUTHOR("Boris Brezillon <boris.brezillon@bootlin.com>");
MODULE_DESCRIPTION("Cadence DSI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cdns-dsi");

