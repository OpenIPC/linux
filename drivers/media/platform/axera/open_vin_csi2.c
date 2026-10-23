// SPDX-License-Identifier: GPL-2.0
/*
 * open_vin_csi2 - V4L2 CSI-2 receiver subdev for the Axera AX630C MIPI CSI-2
 * host. Open replacement for the vendor ax_mipi_rx blob (epic #55, issue #57);
 * ported to the mainline kernel by #83.
 *
 * CLEAN-ROOM: written solely from the behavioral specification
 * docs/reference/deblob-scope/specs/spec-mipi-rx.md ("spec" below). Register
 * offsets and field names describe the silicon; spec section numbers are cited
 * at each sequence. No vendor code was consulted.
 *
 * Hardware (device-confirmed in the spec header):
 *   - CSI-2 protocol controller: CUSTOM Axera register map, DWC-derived error
 *     core only. Do NOT confuse with mainline dw-mipi-csi2 - offsets differ.
 *   - Two RX controllers, 0x02600000 (dev0) / 0x02602000 (dev1), stride
 *     0x2000, both inside this device's first reg bank. The NanoKVM-Pro uses
 *     dev0 only, fed by a fixed HDMI-to-MIPI bridge: 4 data lanes, digital
 *     YUV422-8, comboMode 4, DataLaneMap [0,1,3,4], ClkLane [2,5].
 *   - IRQ is ERROR-ONLY (spec section 5): no frame/packet interrupt exists in
 *     this block. Frame timing belongs to the downstream VIN capture driver.
 *
 * WHAT THE MAINLINE PORT CHANGED (#83), and nothing else did:
 *
 *   - The three shared global blocks are no longer ioremapped at a fixed
 *     physical address. isp_sys_glb (0x02500000) and the VI common syscon
 *     (0x02340000, the same window the clock/reset provider owns) arrive as
 *     syscon phandles and are reached through a shared regmap, so the two
 *     owners of those words share one lock. The D-PHY global file
 *     (0x023f0000) is a real second `reg` entry.
 *   - The dphyrx TLB clock gate (common syscon +0x24 bit 9) and its soft
 *     reset (+0x54 bit 7) are a CCF clock and a reset-controller line now,
 *     not hand-written SET/CLR strobes. Mainline U-Boot programs nothing and
 *     clk_disable_unused() would gate an unclaimed clock, so taking them
 *     properly is not cosmetic here - it is what makes the block live.
 *   - The MIPI RX pads are claimed through a pinctrl state (dphy_rx function
 *     over the twelve CDRX_* pads). On 4.19 the vendor boot chain's pad table
 *     left them muxed; nothing replays that on mainline.
 *   - Subdev binding is the fwnode graph. The sink endpoint's lane map comes
 *     from v4l2_fwnode_endpoint(); the source endpoint is what the capture
 *     driver's notifier matches.
 *
 * Shared-owner discipline (spec section 7): isp_sys_glb and the common syscon
 * are also touched by the in-tree axera clk/reset providers. Every write below
 * uses the hardware's SET/CLR and VALUE/MASK shadow registers - never a
 * read-modify-write of a whole shared word - so concurrent owners of *other*
 * bits are safe.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/spinlock.h>

#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#define OPENVIN_CSI2_NAME		"open_vin_csi2"

/* Fixed source configuration (spec header: device-confirmed). */
#define OPENVIN_LANE_NUM		4
#define OPENVIN_COMBO_MODE		4

/*
 * ---------------------------------------------------------------------------
 * CSI-2 protocol controller (reg[0] = 0x02600000, 0x4000; dev stride 0x2000)
 * Spec section 2. Offsets relative to the per-dev controller base.
 * ---------------------------------------------------------------------------
 */
#define CSI_DEV_STRIDE			0x2000

#define CSI_CTRL_SOFT_RESET		0x04	/* bits[1:0] csi2rx soft reset */
#define CSI_CTRL_DPHY_LANE_CONTROL	0x08	/* lane map + combo mode */
#define CSI_CTRL_ERR_IRQ_MASK_A		0x1c
#define CSI_CTRL_DPHY_ERR_IRQ_MASK	0x24
#define CSI_CTRL_ERR_STATUS0		0x28	/* W1C; spec section 5 bit map */
#define CSI_CTRL_ERR_IRQS_MASK		0x2c
#define CSI_CTRL_ERR_IRQS_MASK2		0x38	/* second per-ctrl copy */
#define CSI_CTRL_STREAM_CFG_LANES	0x40	/* N_LANES / lane-enable */
#define CSI_CTRL_ERR_STATUS1		0x4c	/* W1C; per-lane errsot hs */
#define CSI_CTRL_INFO_IRQS_MASK		0x50
#define CSI_CTRL_STREAM_CTRL		0x100	/* bit0 start, bit1 stop, bit4 srst */
#define CSI_CTRL_STREAM0_MONITOR_CTRL	0x10c
#define CSI_CTRL_STREAM0_MONITOR	0x110	/* bit4 toggled during init */
#define CSI_CTRL_STREAM0_MONITOR_LB	0x118

/* CSI_CTRL_DPHY_LANE_CONTROL fields (spec section 2, +0x08). Device-confirmed
 * final value for comboMode 4: 0x43210410. */
#define CSI_LANE_CONTROL_MASK		0x77770710
#define CSI_LANE_CONTROL_BASE		0x43210010	/* default lane map + bit4 */
#define CSI_LANE_CONTROL_COMBO(m)	((m) << 8)	/* bits[10:8] */

/* Mask/enable values, verbatim from spec section 2 init order. */
#define CSI_ERR_IRQ_MASK_A_VAL		0x7
#define CSI_DPHY_ERR_IRQ_MASK_VAL	0x6f
#define CSI_ERR_IRQS_MASK_VAL		0x00011ee1
#define CSI_INFO_IRQS_MASK_VAL		0x00111100
#define CSI_MONITOR_CTRL_MASK		0x333
#define CSI_MONITOR_CTRL_VAL		0x301
#define CSI_MONITOR_LB_VAL		0x00000d1f

#define CSI_STREAM_CTRL_START		BIT(0)
#define CSI_STREAM_CTRL_STOP		BIT(1)
#define CSI_STREAM_CTRL_SRST		BIT(4)

/*
 * N_LANES register (+0x40): table[LaneNum-1] | 0x10 (spec section 2).
 * The 4-lane value 0x1f is device-confirmed. The table itself is inferred as
 * a lane-enable bitmask {0x1, 0x3, 0x7, 0xf}.
 * TODO(bringup): confirm the 1/2/3-lane table entries on hardware if a
 * non-4-lane mode is ever needed (spec section 9 item 5).
 */
static const u32 csi_lane_table[4] = { 0x1, 0x3, 0x7, 0xf };
#define CSI_STREAM_CFG_LANES_OR		0x10
#define CSI_STREAM_CFG_LANES_MASK	0x1f

/*
 * ---------------------------------------------------------------------------
 * isp_sys_glb - csirx clock gates, soft resets, deskew lock.
 * Reached through the "axera,isp-syscon" phandle (0x02500000).
 * Spec section 3. SET/CLR pairs: write 1 to SET asserts, 1 to CLR deasserts.
 * VALUE/MASK pairs: write field mask to MASK, then value to VALUE.
 *
 * This window has no clock or reset provider of its own, in the vendor tree or
 * ours (dt-bindings/reset/ax630c-reset.h says so explicitly), so these gates
 * and resets stay driver-owned register writes. They are single-bit SET/CLR
 * strobes, which is why that is safe.
 * ---------------------------------------------------------------------------
 */
#define ISP_GLB_CSIRX_STATUS		0xc4	/* deskew status: bits[1:0]==3 locked */
#define ISP_GLB_CSIRX_LOCK_MASK		0x3
#define ISP_GLB_CSIRX_LOCKED		0x3

#define ISP_GLB_CLK_SEL_VAL		0xc8	/* csirx_cfg_clk_sel bits[1:0] */
#define ISP_GLB_CLK_SEL_MASK		0xcc
#define ISP_GLB_CSIRX_CFG_CLK_SEL_MASK	0x3
#define ISP_GLB_CSIRX_CFG_CLK_SEL_VAL	0x3	/* spec section 6a step 14 */

#define ISP_GLB_CLK_EB0_SET		0xd0
#define ISP_GLB_CLK_EB0_CLR		0xd4
#define ISP_GLB_CFG_PHY_CLK_EB		BIT(0)
#define ISP_GLB_DPHY_RX_REF_CLK_EB	BIT(1)

#define ISP_GLB_CLK_EB1_SET		0xd8
#define ISP_GLB_CLK_EB1_CLR		0xdc
#define ISP_GLB_SYS_PIXEL_CLK_EB(d)	BIT(4 + (d))
#define ISP_GLB_CSIRX_PCLK_EB(d)	BIT(7 + (d))

#define ISP_GLB_SWRST_SET		0xe0	/* write 1: assert reset */
#define ISP_GLB_SWRST_CLR		0xe4	/* write 1: deassert reset */
#define ISP_GLB_RST_PIXEL(d)		BIT(2 + 4 * (d))
#define ISP_GLB_RST_PPI_RX_BYTE(d)	BIT(3 + 4 * (d))
#define ISP_GLB_RST_PRESETN(d)		BIT(4 + 4 * (d))
#define ISP_GLB_RST_SYS(d)		BIT(5 + 4 * (d))
#define ISP_GLB_RST_DESKEW(d)		BIT(10 + (d))
#define ISP_GLB_RST_DPHYRX		BIT(12)

#define ISP_GLB_CSI_CTRL_SEL_VAL	0x218
#define ISP_GLB_CSI_CTRL_SEL_MASK	0x21c

/*
 * csi_ctrl_sel (spec-dphy-writes section 3): a masked-write pair, 0x21c =
 * mask/write-enable, 0x218 = value; field [1:0] = controller-0 select,
 * [5:4] = controller-1 select. For "4 lanes on one PHY" (lane_divide_mode
 * 0, our HDMI case) the vendor selects mode 4 = {mask 3, value 0} then
 * {mask 0x30, value 0x30}, once, at the top of the D-PHY global init.
 */

/*
 * ---------------------------------------------------------------------------
 * D-PHY global config - reg[1] = 0x023f0000, lane swap, HS-RX timing, PHY en.
 * Spec section 4 + spec-dphy-writes sections 2-4 (instruction-level list).
 * Every config register is a write-only SET (0xc0 + 8k) / CLR (+4) pair with
 * a readable mirror at 0x34 + (SET - 0xc8) / 2. Vendor idiom: CLR(mask) then
 * SET(value << shift).
 * ---------------------------------------------------------------------------
 */
#define DPHY_LANE_CFG_SET		0xc8	/* lane swap fields, databus16, 1d2c */
#define DPHY_LANE_CFG_CLR		0xcc
#define DPHY_DPDN_SWAP_MASK		0x0000003f
#define DPHY_SWAP_FIELD			0x7
#define DPHY_C1_SWAP_SHIFT		6
#define DPHY_C0_SWAP_SHIFT		9
#define DPHY_D3_SWAP_SHIFT		12
#define DPHY_D2_SWAP_SHIFT		15
#define DPHY_D1_SWAP_SHIFT		18
#define DPHY_D0_SWAP_SHIFT		21
#define DPHY_DATABUS16_SEL		BIT(24)
#define DPHY_1D2C_EN			BIT(25)

/*
 * Physical lane slot feeding each logical lane {d0, d1, d2, d3, c0, c1}.
 * Board wiring for the LT6911UXC bridge on the NanoKVM-Pro: clock on
 * physical slot 2, data on 0,1,3,4. DEVICE-CONFIRMED 2026-09-01: the vendor
 * streaming state reads 0x0005c540 in the +0x34 mirror; a naive 0..5 map
 * reads 0x00053940 and produces a locked PHY that forwards only short
 * packets (no pixel data).
 *
 * On mainline this array is the FALLBACK: openvin_parse_endpoint() rebuilds
 * it from the sink endpoint's data-lanes/clock-lanes, which say exactly the
 * same thing in the device tree where a board fact belongs.
 */
#define OPENVIN_NUM_PHY_LANES		6
static const u8 openvin_lane_swap_default[OPENVIN_NUM_PHY_LANES] = {
	0, 1, 3, 4, 2, 5
};
static const u8 openvin_lane_swap_shift[OPENVIN_NUM_PHY_LANES] = {
	DPHY_D0_SWAP_SHIFT, DPHY_D1_SWAP_SHIFT, DPHY_D2_SWAP_SHIFT,
	DPHY_D3_SWAP_SHIFT, DPHY_C0_SWAP_SHIFT, DPHY_C1_SWAP_SHIFT,
};

#define DPHY_MODE_SET			0xd0	/* mirror +0x38; vendor 0x820 (MIPI) */
#define DPHY_MODE_CLR			0xd4
#define DPHY_MODE_FIELD_MASK		0x00000fff
#define DPHY_MODE_MIPI			0x00000820
#define DPHY_PRE_TIME_SET		0xd8	/* mirror +0x3c: 4 x 8-bit pre-times */
#define DPHY_PRE_TIME_CLR		0xdc
#define DPHY_PRE_TIME_VAL		8	/* all four groups, fixed */
#define DPHY_DESKEW_DBG_SET		0xe8	/* mirror +0x44: deskew reset */
#define DPHY_DESKEW_DBG_CLR		0xec
#define DPHY_DESKEW_DBG_4LANE		0x3c00

#define DPHY_PHY_EN			0x110	/* write-1-to-enable, per lane */
#define DPHY_PHY_DISABLE		0x114	/* write-1-to-disable mirror */
#define DPHY_PHY_ALL_LANES		0x3f	/* clk pair + 4 data lanes */
#define DPHY_PHY_DATA_LANES		0x3c	/* 4 data lanes (bits 5:2) */

/*
 * ---------------------------------------------------------------------------
 * VI common syscon (0x02340000) - the D-PHY analog power triple.
 *
 * The dphyrx TLB clock gate (+0x24 bit 9) and its soft reset (+0x54 bit 7)
 * that this driver used to strobe by hand now arrive as a CCF clock and a
 * reset line from the provider that owns this window; only the power triple
 * is left, because no framework describes it.
 *
 * DEVICE-CONFIRMED 2026-09-01 (/dev/mem on a base-only boot vs the vendor
 * streaming state): power_off = {status 0x1e8, SET 0x1ec, CLR 0x1f0},
 * power_ready = {status 0x1f4, SET 0x1f8, CLR 0x1fc}. An early draft's
 * "CLR ready" write went to 0x1f8, which is the SET strobe: the ISP pixel
 * clocks then stayed dead (isp_sys_glb +0xc4 = 0 instead of 0xf0f).
 * ---------------------------------------------------------------------------
 */
#define CGLB_DPHY_POWER_OFF_STATUS	0x1e8
#define CGLB_DPHY_POWER_OFF_SET		0x1ec
#define CGLB_DPHY_POWER_OFF_CLR		0x1f0	/* spec section 6a step 18 */
#define CGLB_DPHY_POWER_READY_STATUS	0x1f4
#define CGLB_DPHY_POWER_READY_SET	0x1f8
#define CGLB_DPHY_POWER_READY_CLR	0x1fc
#define CGLB_DPHY_POWER_BIT		BIT(0)

/* Deskew lock poll (spec section 5: retry ~20x with delay). */
#define OPENVIN_LOCK_POLL_US		1000
#define OPENVIN_LOCK_TIMEOUT_US		20000

/* ErrorStatus0 bit meanings (spec section 5). Index = bit number. */
static const char *const csi_err0_names[32] = {
	[0]  = "packet/protocol error",
	[4]  = "stream0 FIFO overflow",
	[5]  = "stream1 FIFO overflow",
	[6]  = "stream2 FIFO overflow",
	[7]  = "stream3 FIFO overflow",
	[8]  = "truncated header",
	[9]  = "truncated long packet: wrong byte count",
	[10] = "truncated long packet: no payload",
	[11] = "reserved/invalid short packet",
	[12] = "invalid access to configuration register space",
	[16] = "resynchronization FIFO overflow (DPHY<->protocol)",
	[17] = "data-id error in header",
	[18] = "ECC error corrected",
	[19] = "unrecoverable ECC error",
	/*
	 * TODO(bringup): the spec lists a CRC-error condition in the same
	 * status word but does not pin its bit position; identify it from a
	 * live error dump and name it here.
	 */
};

/*
 * Private V4L2 controls exposing M1 health telemetry (PHY lock + error
 * counters) on the subdev node, so link bring-up is checkable from userspace
 * without the capture pipeline.
 * TODO(upstream): request an official control range before submission (#87).
 * Cosmetic for the port -- nothing in the shipped stack reads these.
 */
#define OPENVIN_CID_BASE		(V4L2_CID_USER_BASE + 0x10a0)
#define OPENVIN_CID_LINK_LOCKED		(OPENVIN_CID_BASE + 0)
#define OPENVIN_CID_ERR0_EVENTS		(OPENVIN_CID_BASE + 1)
#define OPENVIN_CID_ERR1_EVENTS		(OPENVIN_CID_BASE + 2)

enum openvin_pads {
	OPENVIN_PAD_SINK,	/* from the HDMI->MIPI bridge */
	OPENVIN_PAD_SOURCE,	/* to the VIN capture block (M2) */
	OPENVIN_NUM_PADS,
};

struct openvin_csi2 {
	struct device *dev;

	void __iomem *csi;		/* CSI-2 protocol controller, dev0 */
	void __iomem *dphy;		/* D-PHY global file, reg[1] */
	struct regmap *isp_glb;		/* isp_sys_glb syscon */
	struct regmap *cglb;		/* VI common syscon */

	struct clk *tlb_clk;		/* dphyrx TLB gate */
	struct reset_control *tlb_rst;	/* dphyrx TLB soft reset */

	int irq;

	struct v4l2_subdev sd;
	struct media_pad pads[OPENVIN_NUM_PADS];
	struct v4l2_ctrl_handler ctrls;

	/* Standalone-mode owner of the subdev (M1 bring-up without M2). */
	struct v4l2_device v4l2_dev;
	bool standalone;

	u8 lane_swap[OPENVIN_NUM_PHY_LANES];

	struct mutex lock;		/* start/stop */
	bool streaming;

	/* IRQ telemetry (spec section 5): counters + last latched status. */
	spinlock_t err_lock;
	u64 err0_events;
	u64 err1_events;
	u32 last_err0;
	u32 last_err1;
	u64 irq_count;
};

static bool standalone;
module_param(standalone, bool, 0444);
MODULE_PARM_DESC(standalone,
	"Own a private v4l2_device and expose /dev/v4l-subdev* without a bridge (bench mode). Default 0: register with v4l2-async for the open_vin_capture bridge, which starts/stops the receiver with its stream.");

static bool start_on_probe;
module_param(start_on_probe, bool, 0444);
MODULE_PARM_DESC(start_on_probe,
	"Run the full RX bring-up at probe time (bench milestone: prove PHY lock without the capture pipeline).");

static inline struct openvin_csi2 *sd_to_openvin(struct v4l2_subdev *sd)
{
	return container_of(sd, struct openvin_csi2, sd);
}

/* --------------------------------------------------------------------------
 * Register helpers. Plain writes for the CSI controller and the D-PHY file
 * (sole owner); SET/CLR or VALUE/MASK shadow writes only for the shared
 * syscon windows.
 */

static inline u32 csi_rd(struct openvin_csi2 *priv, u32 off)
{
	return readl(priv->csi + off);
}

static inline void csi_wr(struct openvin_csi2 *priv, u32 off, u32 val)
{
	writel(val, priv->csi + off);
}

static inline void csi_rmw(struct openvin_csi2 *priv, u32 off, u32 mask,
			   u32 val)
{
	csi_wr(priv, off, (csi_rd(priv, off) & ~mask) | (val & mask));
}

static inline u32 isp_glb_rd(struct openvin_csi2 *priv, u32 off)
{
	unsigned int val = 0;

	regmap_read(priv->isp_glb, off, &val);
	return val;
}

static inline void isp_glb_wr(struct openvin_csi2 *priv, u32 off, u32 val)
{
	regmap_write(priv->isp_glb, off, val);
}

static inline void dphy_wr(struct openvin_csi2 *priv, u32 off, u32 val)
{
	writel(val, priv->dphy + off);
}

static inline void cglb_wr(struct openvin_csi2 *priv, u32 off, u32 val)
{
	regmap_write(priv->cglb, off, val);
}

/* VALUE/MASK shadow write into isp_sys_glb (spec section 0 access idiom). */
static void isp_glb_field_wr(struct openvin_csi2 *priv, u32 val_off,
			     u32 mask_off, u32 mask, u32 val)
{
	isp_glb_wr(priv, mask_off, mask);
	isp_glb_wr(priv, val_off, val & mask);
}

static bool openvin_link_locked(struct openvin_csi2 *priv)
{
	return (isp_glb_rd(priv, ISP_GLB_CSIRX_STATUS) &
		ISP_GLB_CSIRX_LOCK_MASK) == ISP_GLB_CSIRX_LOCKED;
}

/* --------------------------------------------------------------------------
 * D-PHY global init -- spec-dphy-writes section 3, rows 1-33, resolved for
 * dev 0 / MIPI / 4 lanes on one PHY. Exact vendor order; every D-PHY config
 * write is CLR(field mask) then SET(value). One pass.
 */
static void openvin_dphy_glb_init(struct openvin_csi2 *priv)
{
	unsigned int i;

	/* rows 1-4: csi_ctrl_sel(dev0, mode 4): {mask 3, val 0}, {mask 0x30, val 0x30} */
	isp_glb_wr(priv, ISP_GLB_CSI_CTRL_SEL_MASK, 0x3);
	isp_glb_wr(priv, ISP_GLB_CSI_CTRL_SEL_VAL, 0x0);
	isp_glb_wr(priv, ISP_GLB_CSI_CTRL_SEL_MASK, 0x30);
	isp_glb_wr(priv, ISP_GLB_CSI_CTRL_SEL_VAL, 0x30);

	/* row 5: the thirteen plain words 0x00..0x30 are zeroed */
	for (i = 0; i <= 0x30; i += 4)
		dphy_wr(priv, i, 0);

	/* rows 6-17: lane swap, one 3-bit field per logical lane */
	for (i = 0; i < OPENVIN_NUM_PHY_LANES; i++) {
		dphy_wr(priv, DPHY_LANE_CFG_CLR,
			DPHY_SWAP_FIELD << openvin_lane_swap_shift[i]);
		dphy_wr(priv, DPHY_LANE_CFG_SET,
			(u32)priv->lane_swap[i] << openvin_lane_swap_shift[i]);
	}
	/* row 18: 1d2c_en = 0 (single clock) */
	dphy_wr(priv, DPHY_LANE_CFG_CLR, DPHY_1D2C_EN);
	/* rows 19-20: dpdn_swap = 0 */
	dphy_wr(priv, DPHY_LANE_CFG_CLR, DPHY_DPDN_SWAP_MASK);
	dphy_wr(priv, DPHY_LANE_CFG_SET, 0);
	/* row 21: databus16_sel = 0 */
	dphy_wr(priv, DPHY_LANE_CFG_CLR, DPHY_DATABUS16_SEL);

	/* rows 22-29: HS-RX pre-times, clk grp1, clk grp0, data grp1, data grp0 */
	dphy_wr(priv, DPHY_PRE_TIME_CLR, 0x00ff0000);
	dphy_wr(priv, DPHY_PRE_TIME_SET, DPHY_PRE_TIME_VAL << 16);
	dphy_wr(priv, DPHY_PRE_TIME_CLR, 0xff000000);
	dphy_wr(priv, DPHY_PRE_TIME_SET, DPHY_PRE_TIME_VAL << 24);
	dphy_wr(priv, DPHY_PRE_TIME_CLR, 0x000000ff);
	dphy_wr(priv, DPHY_PRE_TIME_SET, DPHY_PRE_TIME_VAL);
	dphy_wr(priv, DPHY_PRE_TIME_CLR, 0x0000ff00);
	dphy_wr(priv, DPHY_PRE_TIME_SET, DPHY_PRE_TIME_VAL << 8);

	/* rows 30-31: the MIPI mode word */
	dphy_wr(priv, DPHY_MODE_CLR, DPHY_MODE_FIELD_MASK);
	dphy_wr(priv, DPHY_MODE_SET, DPHY_MODE_MIPI);

	/* row 32: required settle before the D-PHY reset release */
	usleep_range(2000, 2500);

	/* row 33: release the D-PHY soft reset (isp_sys_glb, deassert = CLR) */
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_DPHYRX);
}

/* --------------------------------------------------------------------------
 * CSI-2 protocol controller init - spec section 2, strict order (steps 1-14).
 */
static void openvin_csi_ctrl_init(struct openvin_csi2 *priv)
{
	u32 lanes;

	/* 1. Lane map + combo mode (device-confirmed result 0x43210410). */
	csi_rmw(priv, CSI_CTRL_DPHY_LANE_CONTROL, CSI_LANE_CONTROL_MASK,
		CSI_LANE_CONTROL_BASE |
		CSI_LANE_CONTROL_COMBO(OPENVIN_COMBO_MODE));

	/* 2. N_LANES / lane-enable (device-confirmed result 0x1f). */
	lanes = csi_lane_table[OPENVIN_LANE_NUM - 1] | CSI_STREAM_CFG_LANES_OR;
	csi_rmw(priv, CSI_CTRL_STREAM_CFG_LANES, CSI_STREAM_CFG_LANES_MASK,
		lanes);

	/* 3-9. IRQ masks and stream monitor config, before any reset. */
	csi_wr(priv, CSI_CTRL_ERR_IRQ_MASK_A, CSI_ERR_IRQ_MASK_A_VAL);
	csi_wr(priv, CSI_CTRL_DPHY_ERR_IRQ_MASK, CSI_DPHY_ERR_IRQ_MASK_VAL);
	csi_wr(priv, CSI_CTRL_ERR_IRQS_MASK, CSI_ERR_IRQS_MASK_VAL);
	csi_wr(priv, CSI_CTRL_ERR_IRQS_MASK2, CSI_ERR_IRQS_MASK_VAL);
	csi_wr(priv, CSI_CTRL_INFO_IRQS_MASK, CSI_INFO_IRQS_MASK_VAL);
	csi_rmw(priv, CSI_CTRL_STREAM0_MONITOR_CTRL, CSI_MONITOR_CTRL_MASK,
		CSI_MONITOR_CTRL_VAL);
	/* Stream monitor bit4 toggle (spec section 2 step 8). */
	csi_rmw(priv, CSI_CTRL_STREAM0_MONITOR, BIT(4), BIT(4));
	csi_wr(priv, CSI_CTRL_STREAM0_MONITOR_LB, CSI_MONITOR_LB_VAL);

	/* 10-11. Stream soft-reset pulse, then settle. */
	csi_rmw(priv, CSI_CTRL_STREAM_CTRL, CSI_STREAM_CTRL_SRST,
		CSI_STREAM_CTRL_SRST);
	udelay(10);
	csi_rmw(priv, CSI_CTRL_STREAM_CTRL, CSI_STREAM_CTRL_SRST, 0);
	udelay(100);

	/*
	 * 12-13. Clear the stop bit, set the start bit. Device-confirmed
	 * running state: +0x100 == 0x1. Vendor semantics observed 2026-09-04:
	 * 1 = running, 2 = stopped, the value is retained across clock gating,
	 * and no srst pulse is ever issued.
	 */
	csi_rmw(priv, CSI_CTRL_STREAM_CTRL, CSI_STREAM_CTRL_STOP, 0);
	csi_rmw(priv, CSI_CTRL_STREAM_CTRL, CSI_STREAM_CTRL_START,
		CSI_STREAM_CTRL_START);

	/* 14. Clear any latched error/interrupt status, last. */
	csi_wr(priv, CSI_CTRL_ERR_STATUS0, 0xffffffff);
	csi_wr(priv, CSI_CTRL_ERR_STATUS1, 0xffffffff);
}

/* --------------------------------------------------------------------------
 * Full RX bring-up - spec section 6a (ax_mipi_rx_start behavioral order).
 * Hard ordering constraints (spec section 6a tail): resets deasserted ->
 * clocks enabled -> D-PHY powered -> D-PHY enabled -> controller init/stream.
 */
static int openvin_rx_start(struct openvin_csi2 *priv)
{
	u32 val;
	int ret;

	/*
	 * Step 1 (fastboot check) is skipped: on an open-stack boot nothing
	 * has touched the RX.
	 */

	/* Steps 2-5: deassert the csirx0 sub-resets (CLR = deassert). */
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_PIXEL(0));
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_PPI_RX_BYTE(0));
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_PRESETN(0));
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_SYS(0));

	/*
	 * Step 5: deassert both deskew resets (4 lanes -> deskew0 + deskew1),
	 * then step 6: the D-PHY debug-ctrl deskew reset release (CLR 0x3c00
	 * for 4 lanes) -- spec-dphy-writes section 5 rows 5-6.
	 */
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_DESKEW(0));
	isp_glb_wr(priv, ISP_GLB_SWRST_CLR, ISP_GLB_RST_DESKEW(1));
	dphy_wr(priv, DPHY_DESKEW_DBG_CLR, DPHY_DESKEW_DBG_4LANE);

	/*
	 * Step 7: the common-syscon dphyrx TLB soft-reset deassert (row 7).
	 * On 4.19 this was a raw CLR strobe of +0x5c bit 7; it is a reset
	 * line now and lands on exactly the same bit.
	 */
	ret = reset_control_deassert(priv->tlb_rst);
	if (ret) {
		dev_err(priv->dev, "dphyrx TLB reset deassert failed: %d\n",
			ret);
		return ret;
	}

	/* Step 8: settle. */
	udelay(10);

	/* Steps 9-10: csirx0 pclk + pixel clock enables (SET = enable). */
	isp_glb_wr(priv, ISP_GLB_CLK_EB1_SET, ISP_GLB_CSIRX_PCLK_EB(0));
	isp_glb_wr(priv, ISP_GLB_CLK_EB1_SET, ISP_GLB_SYS_PIXEL_CLK_EB(0));

	/* Step 14: csirx cfg clock select. */
	isp_glb_field_wr(priv, ISP_GLB_CLK_SEL_VAL, ISP_GLB_CLK_SEL_MASK,
			 ISP_GLB_CSIRX_CFG_CLK_SEL_MASK,
			 ISP_GLB_CSIRX_CFG_CLK_SEL_VAL);

	/* Steps 15-16: PHY config clock + D-PHY RX reference clock. */
	isp_glb_wr(priv, ISP_GLB_CLK_EB0_SET, ISP_GLB_CFG_PHY_CLK_EB);
	isp_glb_wr(priv, ISP_GLB_CLK_EB0_SET, ISP_GLB_DPHY_RX_REF_CLK_EB);

	/*
	 * Step 17: dphyrx TLB clock enable. The 4.19 driver strobed the
	 * common syscon's gate word by hand, twice (here and again after the
	 * controller init); the CCF gate is refcounted, so one enable is the
	 * whole of it.
	 */
	ret = clk_prepare_enable(priv->tlb_clk);
	if (ret) {
		dev_err(priv->dev, "dphyrx TLB clock enable failed: %d\n", ret);
		return ret;
	}

	/*
	 * Steps 15-18 (spec-dphy-writes section 5): D-PHY power-up -- release
	 * power_off, settle, release power_ready, settle. Both are CLR strobes
	 * of the (status, SET, CLR) triples.
	 */
	cglb_wr(priv, CGLB_DPHY_POWER_OFF_CLR, CGLB_DPHY_POWER_BIT);
	udelay(100);
	cglb_wr(priv, CGLB_DPHY_POWER_READY_CLR, CGLB_DPHY_POWER_BIT);
	udelay(100);

	/* Step 19: disable all PHY lanes before the analog config. */
	dphy_wr(priv, DPHY_PHY_DISABLE, DPHY_PHY_ALL_LANES);

	/* Step 20: the D-PHY global init (lane swap, timing, mode, reset). */
	openvin_dphy_glb_init(priv);

	/* Step 23: enable clock + data lanes (4-lane: 0x3f). */
	dphy_wr(priv, DPHY_PHY_EN, DPHY_PHY_ALL_LANES);

	/* Step 24: data-lane disable/enable toggle. */
	dphy_wr(priv, DPHY_PHY_DISABLE, DPHY_PHY_DATA_LANES);
	dphy_wr(priv, DPHY_PHY_EN, DPHY_PHY_DATA_LANES);

	/* Step 25: CSI-2 protocol controller init, ends with stream start. */
	openvin_csi_ctrl_init(priv);

	/*
	 * Deskew/lock status is valid only after step 25 (spec section 6a).
	 * Poll it. Lock needs a live source on the bridge, so a timeout is a
	 * warning, not a failure: the link comes up when the host starts
	 * driving HDMI.
	 */
	ret = read_poll_timeout(isp_glb_rd, val,
				(val & ISP_GLB_CSIRX_LOCK_MASK) ==
				ISP_GLB_CSIRX_LOCKED,
				OPENVIN_LOCK_POLL_US, OPENVIN_LOCK_TIMEOUT_US,
				false, priv, ISP_GLB_CSIRX_STATUS);
	if (ret)
		dev_warn(priv->dev,
			 "link not locked after start (status 0x%08x) - no source?\n",
			 val);
	else
		dev_dbg(priv->dev, "link locked (deskew status 0x%08x)\n",
			val);

	return 0;
}

/* --------------------------------------------------------------------------
 * Teardown - spec section 6d (mipi_rx_stop behavioral order).
 */
static void openvin_rx_stop(struct openvin_csi2 *priv)
{
	/* Assert the D-PHY soft reset. */
	isp_glb_wr(priv, ISP_GLB_SWRST_SET, ISP_GLB_RST_DPHYRX);

	/* Reference + config PHY clocks off (CLR = disable). */
	isp_glb_wr(priv, ISP_GLB_CLK_EB0_CLR, ISP_GLB_DPHY_RX_REF_CLK_EB);
	isp_glb_wr(priv, ISP_GLB_CLK_EB0_CLR, ISP_GLB_CFG_PHY_CLK_EB);

	/* All PHY lanes off. */
	dphy_wr(priv, DPHY_PHY_DISABLE, DPHY_PHY_ALL_LANES);

	/* D-PHY power-down cycle (ready then off, with settles). */
	cglb_wr(priv, CGLB_DPHY_POWER_READY_SET, CGLB_DPHY_POWER_BIT);
	udelay(100);
	cglb_wr(priv, CGLB_DPHY_POWER_OFF_SET, CGLB_DPHY_POWER_BIT);
	udelay(100);

	/* Deskew reset asserted, TLB clock off. */
	isp_glb_wr(priv, ISP_GLB_SWRST_SET, ISP_GLB_RST_DESKEW(0));
	clk_disable_unprepare(priv->tlb_clk);

	/* Stream stop, last (spec section 6d). */
	csi_rmw(priv, CSI_CTRL_STREAM_CTRL, CSI_STREAM_CTRL_START, 0);
	csi_rmw(priv, CSI_CTRL_STREAM_CTRL, CSI_STREAM_CTRL_STOP,
		CSI_STREAM_CTRL_STOP);
}

/* --------------------------------------------------------------------------
 * Error IRQ - spec section 5. Error-only: W1C both status words, count,
 * decode. No frame interrupt exists on this block.
 */
static irqreturn_t openvin_csi2_irq(int irq, void *arg)
{
	struct openvin_csi2 *priv = arg;
	unsigned long flags;
	unsigned long err0_bits;
	u32 err0, err1;
	unsigned int bit;

	err0 = csi_rd(priv, CSI_CTRL_ERR_STATUS0);
	if (err0)
		csi_wr(priv, CSI_CTRL_ERR_STATUS0, err0);
	err1 = csi_rd(priv, CSI_CTRL_ERR_STATUS1);
	if (err1)
		csi_wr(priv, CSI_CTRL_ERR_STATUS1, err1);

	if (!err0 && !err1)
		return IRQ_NONE;

	spin_lock_irqsave(&priv->err_lock, flags);
	priv->irq_count++;
	if (err0) {
		priv->last_err0 = err0;
		priv->err0_events += hweight32(err0);
	}
	if (err1) {
		priv->last_err1 = err1;
		priv->err1_events += hweight32(err1);
	}
	spin_unlock_irqrestore(&priv->err_lock, flags);

	err0_bits = err0;
	for_each_set_bit(bit, &err0_bits, 32)
		dev_dbg_ratelimited(priv->dev, "csi error: %s (bit %u)\n",
				    csi_err0_names[bit] ?: "unknown", bit);
	if (err1)
		dev_dbg_ratelimited(priv->dev,
				    "dphy errsot hs, lanes 0x%08x\n", err1);

	return IRQ_HANDLED;
}

/* --------------------------------------------------------------------------
 * V4L2 subdev ops
 */

static int openvin_csi2_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct openvin_csi2 *priv = sd_to_openvin(sd);
	int ret = 0;

	mutex_lock(&priv->lock);
	if (enable && !priv->streaming) {
		enable_irq(priv->irq);
		ret = openvin_rx_start(priv);
		if (ret)
			disable_irq(priv->irq);
		else
			priv->streaming = true;
	} else if (!enable && priv->streaming) {
		disable_irq(priv->irq);
		openvin_rx_stop(priv);
		priv->streaming = false;
	}
	mutex_unlock(&priv->lock);

	return ret;
}

static int openvin_csi2_log_status(struct v4l2_subdev *sd)
{
	struct openvin_csi2 *priv = sd_to_openvin(sd);
	unsigned long flags;
	u64 err0_events, err1_events, irq_count;
	u32 last_err0, last_err1;

	spin_lock_irqsave(&priv->err_lock, flags);
	err0_events = priv->err0_events;
	err1_events = priv->err1_events;
	irq_count = priv->irq_count;
	last_err0 = priv->last_err0;
	last_err1 = priv->last_err1;
	spin_unlock_irqrestore(&priv->err_lock, flags);

	mutex_lock(&priv->lock);
	v4l2_info(sd, "streaming: %d\n", priv->streaming);
	/*
	 * VIN gates the shared ISP/CSI clock bank while idle. Telemetry must
	 * not touch the stopped controller or wake capture just for status.
	 */
	if (!priv->streaming)
		goto counters;
	v4l2_info(sd, "deskew/lock status: 0x%08x (%s)\n",
		  isp_glb_rd(priv, ISP_GLB_CSIRX_STATUS),
		  openvin_link_locked(priv) ? "LOCKED" : "not locked");
	v4l2_info(sd, "lane control: 0x%08x, n_lanes: 0x%08x, stream ctrl: 0x%08x\n",
		  csi_rd(priv, CSI_CTRL_DPHY_LANE_CONTROL),
		  csi_rd(priv, CSI_CTRL_STREAM_CFG_LANES),
		  csi_rd(priv, CSI_CTRL_STREAM_CTRL));
	v4l2_info(sd, "live ErrorStatus0: 0x%08x, ErrorStatus1: 0x%08x\n",
		  csi_rd(priv, CSI_CTRL_ERR_STATUS0),
		  csi_rd(priv, CSI_CTRL_ERR_STATUS1));
counters:
	mutex_unlock(&priv->lock);
	v4l2_info(sd, "error irqs: %llu (status0 events: %llu, status1 events: %llu)\n",
		  irq_count, err0_events, err1_events);
	v4l2_info(sd, "last ErrorStatus0: 0x%08x, last ErrorStatus1: 0x%08x\n",
		  last_err0, last_err1);

	return 0;
}

static void openvin_default_fmt(struct v4l2_mbus_framefmt *fmt)
{
	fmt->code = MEDIA_BUS_FMT_UYVY8_1X16;
	fmt->width = 1920;
	fmt->height = 1080;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
}

static int openvin_csi2_init_state(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *state)
{
	unsigned int pad;

	for (pad = 0; pad < OPENVIN_NUM_PADS; pad++)
		openvin_default_fmt(v4l2_subdev_state_get_format(state, pad));

	return 0;
}

static int openvin_csi2_enum_mbus_code(struct v4l2_subdev *sd,
				       struct v4l2_subdev_state *state,
				       struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index > 0)
		return -EINVAL;
	code->code = MEDIA_BUS_FMT_UYVY8_1X16;
	return 0;
}

static int openvin_csi2_set_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *format;
	unsigned int pad;

	/*
	 * The receiver is format-transparent: it forwards whatever the fixed
	 * HDMI->MIPI bridge emits (digital YUV422-8) and programs no geometry
	 * anywhere. Only the frame size is negotiable, and it is carried so
	 * the media graph reads correctly; geometry enforcement is the capture
	 * driver's job.
	 */
	fmt->format.code = MEDIA_BUS_FMT_UYVY8_1X16;
	fmt->format.width = clamp_t(u32, fmt->format.width, 64, 4096);
	fmt->format.height = clamp_t(u32, fmt->format.height, 64, 2400);
	fmt->format.field = V4L2_FIELD_NONE;
	fmt->format.colorspace = V4L2_COLORSPACE_SRGB;

	/* Sink format propagates to the source pad: nothing in between. */
	for (pad = 0; pad < OPENVIN_NUM_PADS; pad++) {
		format = v4l2_subdev_state_get_format(state, pad);
		*format = fmt->format;
	}

	return 0;
}

static const struct v4l2_subdev_core_ops openvin_csi2_core_ops = {
	.log_status = openvin_csi2_log_status,
};

static const struct v4l2_subdev_video_ops openvin_csi2_video_ops = {
	.s_stream = openvin_csi2_s_stream,
};

static const struct v4l2_subdev_pad_ops openvin_csi2_pad_ops = {
	.enum_mbus_code = openvin_csi2_enum_mbus_code,
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = openvin_csi2_set_fmt,
};

static const struct v4l2_subdev_ops openvin_csi2_subdev_ops = {
	.core = &openvin_csi2_core_ops,
	.video = &openvin_csi2_video_ops,
	.pad = &openvin_csi2_pad_ops,
};

static const struct v4l2_subdev_internal_ops openvin_csi2_internal_ops = {
	.init_state = openvin_csi2_init_state,
};

/* --------------------------------------------------------------------------
 * Controls: read-only volatile health telemetry.
 */
static int openvin_csi2_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct openvin_csi2 *priv =
		container_of(ctrl->handler, struct openvin_csi2, ctrls);
	unsigned long flags;

	switch (ctrl->id) {
	case OPENVIN_CID_LINK_LOCKED:
		mutex_lock(&priv->lock);
		ctrl->val = priv->streaming && openvin_link_locked(priv);
		mutex_unlock(&priv->lock);
		return 0;
	case OPENVIN_CID_ERR0_EVENTS:
		spin_lock_irqsave(&priv->err_lock, flags);
		*ctrl->p_new.p_s64 = priv->err0_events;
		spin_unlock_irqrestore(&priv->err_lock, flags);
		return 0;
	case OPENVIN_CID_ERR1_EVENTS:
		spin_lock_irqsave(&priv->err_lock, flags);
		*ctrl->p_new.p_s64 = priv->err1_events;
		spin_unlock_irqrestore(&priv->err_lock, flags);
		return 0;
	}
	return -EINVAL;
}

static const struct v4l2_ctrl_ops openvin_csi2_ctrl_ops = {
	.g_volatile_ctrl = openvin_csi2_g_volatile_ctrl,
};

static const struct v4l2_ctrl_config openvin_csi2_ctrl_cfgs[] = {
	{
		.ops = &openvin_csi2_ctrl_ops,
		.id = OPENVIN_CID_LINK_LOCKED,
		.name = "CSI-2 Link Locked",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &openvin_csi2_ctrl_ops,
		.id = OPENVIN_CID_ERR0_EVENTS,
		.name = "CSI-2 Error Events",
		.type = V4L2_CTRL_TYPE_INTEGER64,
		.min = 0,
		.max = S64_MAX,
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &openvin_csi2_ctrl_ops,
		.id = OPENVIN_CID_ERR1_EVENTS,
		.name = "D-PHY ErrSotHS Events",
		.type = V4L2_CTRL_TYPE_INTEGER64,
		.min = 0,
		.max = S64_MAX,
		.step = 1,
		.def = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* --------------------------------------------------------------------------
 * Probe / remove
 */

/*
 * The sink endpoint is where the board's lane wiring belongs. data-lanes /
 * clock-lanes name the PHYSICAL lane slot feeding each logical lane, which is
 * exactly what the D-PHY swap fields take; an absent or malformed endpoint
 * falls back to the device-confirmed constant, loudly.
 */
static void openvin_parse_endpoint(struct openvin_csi2 *priv)
{
	struct v4l2_fwnode_endpoint vep = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	struct fwnode_handle *ep;
	unsigned int i, used = 0;
	int ret;

	memcpy(priv->lane_swap, openvin_lane_swap_default,
	       sizeof(priv->lane_swap));

	ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(priv->dev),
					     OPENVIN_PAD_SINK, 0,
					     FWNODE_GRAPH_ENDPOINT_NEXT);
	if (!ep) {
		dev_warn(priv->dev,
			 "no sink endpoint; using the default lane map\n");
		return;
	}

	ret = v4l2_fwnode_endpoint_parse(ep, &vep);
	fwnode_handle_put(ep);
	if (ret) {
		dev_warn(priv->dev,
			 "sink endpoint parse failed (%d); using the default lane map\n",
			 ret);
		return;
	}

	if (vep.bus.mipi_csi2.num_data_lanes != OPENVIN_LANE_NUM) {
		dev_warn(priv->dev,
			 "sink endpoint declares %u data lanes, this receiver is wired for %u; using the default lane map\n",
			 vep.bus.mipi_csi2.num_data_lanes, OPENVIN_LANE_NUM);
		return;
	}

	for (i = 0; i < OPENVIN_LANE_NUM; i++) {
		u8 phys = vep.bus.mipi_csi2.data_lanes[i];

		if (phys >= OPENVIN_NUM_PHY_LANES) {
			dev_warn(priv->dev,
				 "data-lane %u is physical slot %u, out of range; using the default lane map\n",
				 i, phys);
			return;
		}
		priv->lane_swap[i] = phys;
		used |= BIT(phys);
	}

	if (vep.bus.mipi_csi2.clock_lane >= OPENVIN_NUM_PHY_LANES) {
		dev_warn(priv->dev,
			 "clock-lane %u out of range; using the default lane map\n",
			 vep.bus.mipi_csi2.clock_lane);
		memcpy(priv->lane_swap, openvin_lane_swap_default,
		       sizeof(priv->lane_swap));
		return;
	}
	priv->lane_swap[4] = vep.bus.mipi_csi2.clock_lane;
	used |= BIT(vep.bus.mipi_csi2.clock_lane);

	/*
	 * c1 is the second clock slot of the 1d2c ("one D-PHY, two
	 * controllers") mode this board does not use. It still has to name a
	 * slot, and the vendor names the one nothing else claims.
	 */
	for (i = 0; i < OPENVIN_NUM_PHY_LANES; i++) {
		if (!(used & BIT(i))) {
			priv->lane_swap[5] = i;
			break;
		}
	}

	dev_info(priv->dev,
		 "lane map from DT: d[%u %u %u %u] c[%u %u]\n",
		 priv->lane_swap[0], priv->lane_swap[1], priv->lane_swap[2],
		 priv->lane_swap[3], priv->lane_swap[4], priv->lane_swap[5]);
}

static int openvin_csi2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct openvin_csi2 *priv;
	struct resource *res;
	unsigned int i;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->dev = dev;
	priv->standalone = standalone;
	mutex_init(&priv->lock);
	spin_lock_init(&priv->err_lock);

	/*
	 * reg[0] = the CSI-2 protocol controller bank (both devs; this driver
	 * drives dev0 at +0x0000), reg[1] = the D-PHY global file. On 4.19 the
	 * second was an ioremap of a literal address.
	 */
	priv->csi = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(priv->csi))
		return PTR_ERR(priv->csi);

	priv->dphy = devm_platform_ioremap_resource_byname(pdev, "dphy");
	if (IS_ERR(priv->dphy))
		return dev_err_probe(dev, PTR_ERR(priv->dphy),
				     "no D-PHY global reg\n");

	priv->isp_glb = syscon_regmap_lookup_by_phandle(dev->of_node,
							"axera,isp-syscon");
	if (IS_ERR(priv->isp_glb))
		return dev_err_probe(dev, PTR_ERR(priv->isp_glb),
				     "axera,isp-syscon\n");

	priv->cglb = syscon_regmap_lookup_by_phandle(dev->of_node,
						     "axera,common-syscon");
	if (IS_ERR(priv->cglb))
		return dev_err_probe(dev, PTR_ERR(priv->cglb),
				     "axera,common-syscon\n");

	priv->tlb_clk = devm_clk_get(dev, "dphyrx_tlb");
	if (IS_ERR(priv->tlb_clk))
		return dev_err_probe(dev, PTR_ERR(priv->tlb_clk),
				     "dphyrx_tlb clock\n");

	priv->tlb_rst = devm_reset_control_get_exclusive(dev, "dphyrx_tlb");
	if (IS_ERR(priv->tlb_rst))
		return dev_err_probe(dev, PTR_ERR(priv->tlb_rst),
				     "dphyrx_tlb reset\n");

	openvin_parse_endpoint(priv);

	/* Error IRQ for controller 0. Controller 1 is unused on this board;
	 * its "csictrl1" line is left unclaimed. */
	priv->irq = platform_get_irq_byname(pdev, "csictrl0");
	if (priv->irq < 0)
		return priv->irq;
	ret = devm_request_irq(dev, priv->irq, openvin_csi2_irq, 0,
			       OPENVIN_CSI2_NAME, priv);
	if (ret)
		return ret;
	/* Enabled only while streaming (balanced in s_stream). */
	disable_irq(priv->irq);

	v4l2_subdev_init(&priv->sd, &openvin_csi2_subdev_ops);
	priv->sd.internal_ops = &openvin_csi2_internal_ops;
	priv->sd.owner = THIS_MODULE;
	priv->sd.dev = dev;
	priv->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(priv->sd.name, sizeof(priv->sd.name), OPENVIN_CSI2_NAME);
	v4l2_set_subdevdata(&priv->sd, priv);

	priv->sd.entity.function = MEDIA_ENT_F_VID_IF_BRIDGE;
	priv->pads[OPENVIN_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	priv->pads[OPENVIN_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&priv->sd.entity, OPENVIN_NUM_PADS,
				     priv->pads);
	if (ret)
		return ret;

	v4l2_ctrl_handler_init(&priv->ctrls,
			       ARRAY_SIZE(openvin_csi2_ctrl_cfgs));
	for (i = 0; i < ARRAY_SIZE(openvin_csi2_ctrl_cfgs); i++)
		v4l2_ctrl_new_custom(&priv->ctrls,
				     &openvin_csi2_ctrl_cfgs[i], NULL);
	if (priv->ctrls.error) {
		ret = priv->ctrls.error;
		goto err_entity;
	}
	priv->sd.ctrl_handler = &priv->ctrls;

	ret = v4l2_subdev_init_finalize(&priv->sd);
	if (ret)
		goto err_ctrls;

	if (priv->standalone) {
		/*
		 * Bench mode: own a private v4l2_device so the subdev node
		 * exists without the capture bridge.
		 */
		ret = v4l2_device_register(dev, &priv->v4l2_dev);
		if (ret)
			goto err_subdev;
		ret = v4l2_device_register_subdev(&priv->v4l2_dev, &priv->sd);
		if (ret)
			goto err_v4l2_dev;
		ret = v4l2_device_register_subdev_nodes(&priv->v4l2_dev);
		if (ret)
			goto err_v4l2_dev;
	} else {
		ret = v4l2_async_register_subdev(&priv->sd);
		if (ret)
			goto err_subdev;
	}

	platform_set_drvdata(pdev, priv);

	dev_info(dev,
		 "AX630C CSI-2 RX subdev (dev0 @%pa, %u lanes, combo mode %u)%s\n",
		 &res->start, OPENVIN_LANE_NUM, OPENVIN_COMBO_MODE,
		 priv->standalone ? ", standalone" : "");

	if (start_on_probe) {
		ret = openvin_csi2_s_stream(&priv->sd, 1);
		if (ret)
			dev_warn(dev, "start_on_probe failed: %d\n", ret);
	}

	return 0;

err_v4l2_dev:
	v4l2_device_unregister(&priv->v4l2_dev);
err_subdev:
	v4l2_subdev_cleanup(&priv->sd);
err_ctrls:
	v4l2_ctrl_handler_free(&priv->ctrls);
err_entity:
	media_entity_cleanup(&priv->sd.entity);
	return ret;
}

static void openvin_csi2_remove(struct platform_device *pdev)
{
	struct openvin_csi2 *priv = platform_get_drvdata(pdev);

	openvin_csi2_s_stream(&priv->sd, 0);

	if (priv->standalone) {
		v4l2_device_unregister_subdev(&priv->sd);
		v4l2_device_unregister(&priv->v4l2_dev);
	} else {
		v4l2_async_unregister_subdev(&priv->sd);
	}
	v4l2_subdev_cleanup(&priv->sd);
	v4l2_ctrl_handler_free(&priv->ctrls);
	media_entity_cleanup(&priv->sd.entity);
}

static const struct of_device_id openvin_csi2_of_match[] = {
	{ .compatible = "axera,ax630c-csi2-rx" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, openvin_csi2_of_match);

static struct platform_driver openvin_csi2_driver = {
	.probe = openvin_csi2_probe,
	.remove = openvin_csi2_remove,
	.driver = {
		.name = OPENVIN_CSI2_NAME,
		.of_match_table = openvin_csi2_of_match,
	},
};
module_platform_driver(openvin_csi2_driver);

MODULE_AUTHOR("open-nanokvm-pro contributors");
MODULE_DESCRIPTION("Open V4L2 CSI-2 receiver subdev for AX630C (ax_mipi_rx replacement, #57/#83)");
MODULE_LICENSE("GPL v2");
