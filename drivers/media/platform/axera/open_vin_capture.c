// SPDX-License-Identifier: GPL-2.0
/*
 * open_vin_capture -- open V4L2 VIN/IFE capture video node for the Axera
 * AX630C (replaces the vendor ax_proton bypass/IFE-WDMA path). Epic #55 /
 * issue #59 (M2); ported to the mainline kernel by #83. Digital YUV422-8 over
 * MIPI CSI-2 -> IFE bypass (MODE10 / MODE3 whole-frame) -> IFE-WDMA -> DDR.
 *
 * CLEAN-ROOM: written from the behavioral specs
 *   docs/reference/deblob-scope/specs/spec-proton-bypass.md  ("spec §N" below)
 *   docs/reference/deblob-scope/specs/spec-cdma.md           (CDMA is optional;
 *     plain ordered writel() + explicit polls reproduce the vendor config)
 * plus this repo's open code and mainline 4.19 V4L2 sources. No vendor code.
 *
 * Register model (spec §0): every vendor register access is a plain
 * readl()/writel() at block_base + offset inside the 0x02400000 ISP/VIN
 * register file; RMW is read-modify-write in the driver (no hardware RMW);
 * enable / shadow-commit bits are set LAST (spec §4).
 *
 * Memory model: vb2 buffers come from a reserved-memory `shared-dma-pool`
 * named by `memory-region`, attached with of_reserved_mem_device_init(). A
 * small custom vb2 mem_ops allocates via dma_alloc_coherent() from that pool;
 * the buffer's dma_addr is exactly the physical DDR address the WDMA needs
 * ("writel(dma_addr >> 3, ...)", spec §3 -- THE GATE). There is no IOMMU on
 * this SoC, so bus == phys.
 *
 * WHAT THE MAINLINE PORT CHANGED (#83):
 *
 *   - The carveout is a DT reserved-memory node, not the carveout_base /
 *     carveout_size module parameters the 4.19 shell loader computed. The
 *     loader's whole `compute_mem_map` has no mainline counterpart.
 *   - The clock/reset window at 0x02500000 is a syscon phandle
 *     ("axera,isp-syscon"), not an ioremap of a literal address. It is the
 *     one window on this SoC with no clock or reset provider -- the vendor
 *     never wrote one either -- so the gate and reset sweeps below stay
 *     driver-owned register writes, and they are exactly the writes that are
 *     device-proven.
 *   - The VI and ISP-MM domain gates in the common syscon ARE modelled by the
 *     in-tree clock provider, and are taken here as `clocks =` consumers.
 *     That is not decoration: clk_disable_unused() gates an unclaimed clock
 *     on mainline, and mainline U-Boot leaves nothing enabled.
 *   - Subdev binding is the fwnode graph (v4l2_async_nf_add_fwnode_remote),
 *     not the platform-device-name match the vendor DT forced.
 *
 * Hardware-proven and shipping since #55 M3 (2026-09-02) on 4.19: 4K30 and
 * every sub-4K crop byte-identical to the vendor's frames, cold-boot clean.
 * The remaining TODO(bringup) comments mark values the spec could not pin
 * that still carry a best guess (each states how to confirm it).
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/iosys-map.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/videodev2.h>

#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-async.h>
#include <media/v4l2-fwnode.h>
#include <media/media-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-memops.h>

#include <linux/dma-buf.h>
#include <linux/scatterlist.h>

#include "ovc_golden_4k.h"

#define OVC_DRV_NAME		"open_vin_capture"
/* open_vin_csi2's pad layout: 0 = sink (bridge), 1 = source (to us). */
#define OVC_CSI2_SOURCE_PAD	1

/* ------------------------------------------------------------------------ */
/* Module parameters                                                        */
/* ------------------------------------------------------------------------ */

/*
 * ISP-top module gate masks (spec §2 "MODE10 bypass mask"). Device-confirmed
 * 2026-09-01 (see OVC_TOP_GATE_*): the status word 0x02400150 resets to
 * all-ones and the vendor clears bits 0,1,2,15 (status 0xffff7ff8); the SET
 * strobe is not needed (0 = skip). Clearing those four bits is the gate that
 * makes every SIF/IFE/WDMA config write take effect.
 */
static u32 bypass_set_mask;
module_param(bypass_set_mask, uint, 0444);
MODULE_PARM_DESC(bypass_set_mask, "ISP-top module gate SET strobe (0x02400154); 0 = skip");

static u32 bypass_clr_mask = 0x00008007;
module_param(bypass_clr_mask, uint, 0444);
MODULE_PARM_DESC(bypass_clr_mask, "ISP-top module gate CLR strobe (0x02400158); golden 0x8007");

/*
 * WDMA channel for single-plane packed YUV422. Channel 8 device-confirmed
 * (spec header: chn8 addr reg 0x024140d4 = phys>>3 during live 4K capture).
 */
static unsigned int wdma_chn = 8;
module_param(wdma_chn, uint, 0444);
MODULE_PARM_DESC(wdma_chn, "IFE-WDMA channel for the packed YUV422 plane");

/* ------------------------------------------------------------------------ */
/* Register map -- offsets inside the 0x02400000 ISP/VIN register file      */
/* (spec §0 naming: absolute MMIO = 0x02400000 + offset)                    */
/* ------------------------------------------------------------------------ */

#define OVC_REG_FILE_PHYS	0x02400000
#define OVC_REG_FILE_LEN	0xd4008

/* VIN/ISP interrupt controller bank 0, flat 0x10-stride groups (spec §5.1) */
#define OVC_INT_NGROUPS		10
#define OVC_INT_ENABLE(n)	(0x10 * (n) + 0x10)
#define OVC_INT_CLEAR(n)	(0x10 * (n) + 0x14)	/* W1C */
#define OVC_INT_RAW(n)		(0x10 * (n) + 0x18)
#define OVC_INT_MASKED(n)	(0x10 * (n) + 0x1c)
/* Second interrupt bank (the isp0-1 GIC line's view; the vendor-live
 * snapshot shows its enables all-ones at reset), same layout from +0xb0. */
#define OVC_INT1_ENABLE(n)	(0x10 * (n) + 0xb0)
#define OVC_INT1_CLEAR(n)	(0x10 * (n) + 0xb4)
#define OVC_INT1_MASKED(n)	(0x10 * (n) + 0xbc)
#define OVC_INT_GRP_FSOF	1	/* frame-start, bit0 for dev0 */
#define OVC_INT_GRP_FDONE	4	/* IFE WDMA frame-done */
/*
 * Frame-done bit: 1<<(wdma_chn+1) = bit9 for chn8; device-confirmed live
 * (group-4 enable 0x02400050 read 0x200 while the vendor stack streamed).
 */
#define OVC_INT_FDONE_BIT(chn)	BIT((chn) + 1)

/* SIF front-end (spec §1); dev_id = 0 (single HDMI-in path) */
#define OVC_SIF_DEV_ID		0
#define OVC_SIF_START		0x6404	/* [2:0] = 1<<dev begins capture */
#define OVC_SIF_STOP		0x6408
#define OVC_SIF_VBUS_CTRL(n)	(0x640c + 4 * (n))
#define OVC_SIF_VBUS_KEEP	0xFFFC0E0E
#define OVC_SIF_DEV(d)		(0x6500 + 0x80 * (d))
#define OVC_SIF_CTRL(d)		(OVC_SIF_DEV(d) + 0x00)	/* [2:0] arm */
#define OVC_SIF_ID(d)		(OVC_SIF_DEV(d) + 0x04)	/* = dev_id + 1 */
#define OVC_SIF_IN_FMT(d)	(OVC_SIF_DEV(d) + 0x10)
#define OVC_SIF_WIN0_START(d)	(OVC_SIF_DEV(d) + 0x14)
#define OVC_SIF_WIN0_SIZE(d)	(OVC_SIF_DEV(d) + 0x18)	/* W | (H<<16) */
#define OVC_SIF_WIN1_START(d)	(OVC_SIF_DEV(d) + 0x1c)
#define OVC_SIF_MIPI_CTRL(d)	(OVC_SIF_DEV(d) + 0x3c)	/* [0] = enable */
#define OVC_SIF_VC_MATCH(d, i)	(OVC_SIF_DEV(d) + 0x40 + 8 * (i))
#define OVC_SIF_DT_MATCH(d, i)	(OVC_SIF_DEV(d) + 0x44 + 8 * (i))

/* SIF_IN_FMT (spec §1.2): YUV422 ([2:0]=3), 8-bit ([6:4]=0), progressive
 * yuv-select [16]=1,[17]=1,[19]=0; preserve mask 0xFFF48E88.
 * ==> new = (old & KEEP) | 0x00030003 for CSI-2 DT 0x1E. */
#define OVC_SIF_IN_FMT_KEEP	0xFFF48E88
/* HW-corrected 2026-08-31 from the live vendor 4K capture register image
 * (docs/reference/deblob-scope/regdumps/): SIF IN_FMT settles to 0x40 for this
 * HDMI YUV422 path, not the spec-predicted 0x00030003. */
#define OVC_SIF_IN_FMT_YUV422_8	0x40
#define OVC_CSI2_DT_YUV422_8	0x1E
#define OVC_CSI2_DT_PARK	0x3A	/* never-matching DT for idle matcher */
#define OVC_SIF_WIN1_PARK	0x00200020

/*
 * IFE block. WDMA block base device-confirmed at absolute 0x02414000 =
 * file +0x14000 (spec header). The IFE-go register is at absolute file
 * offset 0x146dc (spec §2, device-confirmed bit0 set while streaming).
 *
 * ISP-top module gate ("MODE10 bypass mask", spec §2). DEVICE-CONFIRMED
 * 2026-09-01: the pair lives in the ISP TOP block (file +0x154 SET /
 * +0x158 CLR) with a readable status word at +0x150, NOT in the IFE block
 * (+0x14154 is a WDMA per-channel stride register). The status word resets
 * to all-ones; the vendor-live golden reads 0xffff7ff8, and writing the
 * 0x8007 clear-mask is exactly what makes SIF/IFE/WDMA config writes stick
 * (before it every datapath write reads back 0).
 */
#define OVC_IFE_BLOCK		0x14000
#define OVC_TOP_GATE_STATUS	0x150
#define OVC_TOP_GATE_SET	0x154
#define OVC_TOP_GATE_CLR	0x158
#define OVC_TOP_GATE_GOLDEN	0xffff7ff8	/* status after the vendor bring-up */
#define OVC_IFE_GO		0x146dc	/* RMW keep 0xf81c, set bit0 (spec §2) */
#define OVC_IFE_GO_KEEP		0x0000f81c

/* IFE-WDMA per-channel control bank, stride 0x18 (spec §3) */
/*
 * Per-channel WDMA words, stride 0x18 (spec-ife-start §2.3, device-proven
 * 2026-09-01): +0x0c = SHADOW-LOAD TRIGGER (bit0, re-issued EVERY frame),
 * +0x10 = 16-bit sequence token (software writes the low half; the upper
 * half is the hardware's latched copy, so 0x1e2d1e2d in the vendor state
 * means "loaded"), +0x14 = buffer phys>>3, +0x18 = control word (from the
 * golden table), +0x1c = channel enable. The earlier spec called +0x18 the
 * commit; pulsing it never loaded anything -- the first strobe on +0x0c
 * put a full 4K frame in DDR.
 */
#define OVC_WDMA_TRIG(c)	(OVC_IFE_BLOCK + 0x18 * (c) + 0x0c)	/* bit0 */
#define OVC_WDMA_SEQ(c)		(OVC_IFE_BLOCK + 0x18 * (c) + 0x10)	/* low16 */
#define OVC_WDMA_ADDR(c)	(OVC_IFE_BLOCK + 0x18 * (c) + 0x14)	/* phys>>3 */
#define OVC_WDMA_ENABLE(c)	(OVC_IFE_BLOCK + 0x18 * (c) + 0x1c)	/* bit0 */
/* ISP-top data-source mux: readable mirror +0x16c, write-only enable +0x170
 * / disable +0x174 strobes (spec-ife-start §1.2). Vendor mirror = 0x30. */
#define OVC_TOP_MUX_EN		0x170
#define OVC_TOP_MUX_VAL		0x30
#define OVC_IFE_FLUSH		0x146e0	/* written 0xffffffff after the go RMW */
/* Per-channel format/geometry bank, stride 0x20, base (chn+0xf)<<5 (spec §3 sel 3) */
#define OVC_WDMA_FMT_BANK(c)	(OVC_IFE_BLOCK + (((c) + 0xf) << 5))
/* Packing/burst + WxH (spec §3 selector 2) */
#define OVC_WDMA_PACK(c)	(OVC_IFE_BLOCK + 0x18 * (c) + 0x3e8)
#define OVC_WDMA_PACK_KEEP	0xFFFC8880
#define OVC_WDMA_WH(c)		(OVC_IFE_BLOCK + 0x18 * (c) + 0x3ec)

/* Clock/reset controller = low 0x100 of the 0x02500000 window (spec §8).
 * Reached through the "axera,isp-syscon" phandle -- the one window on this
 * SoC that has no clock or reset provider, in the vendor tree or ours
 * (dt-bindings/reset/ax630c-reset.h says so). W1S/W1C pairs. */
#define OVC_CLK_MUX_RD		0x00
#define OVC_CLK_MUX_SET		0xC8
#define OVC_CLK_MUX_CLR		0xCC
/*
 * ISP clock-source MUX (spec-vin-write-enable §1). Three 3-bit source-select
 * fields at MUX_RD [10:8]/[7:5]/[4:2] (ISP domains 0/1/2), applied by strobing
 * the write-only CLR (0xCC) then SET (0xC8) registers. The vendor performs this
 * at ax_proton probe (ax_isp_clk_prepare); M1's gate-only bring-up (0xD0/0xD8)
 * omits it. Device-proven 2026-08-31: writing codes 5/5/3 drives MUX_RD
 * (0x02500000+0x00) to 0x5af, the live-vendor golden. This selects the ISP
 * clock SOURCE but is NOT sufficient on its own to make the SIF/IFE datapath
 * writable -- see ovc_clk_mux_apply() and spec-isp-clock-enable.md (the source
 * PLL / power domain that actually starts the clock, C0/C4, is still needed).
 */
#define OVC_CLK_MUX_NFIELDS	3
#define OVC_CLK_MUX_FIELD	0x7
#define OVC_CLK_MUX_READY	0x5af	/* golden MUX_RD after apply (device-proven) */
/*
 * ...but only bits [10:2] of it are the mux. MUX_RD[3:0] is the CSI deskew
 * lock status, which is 0xf only while a source is driving the link -- so an
 * idle board reads 0x5ac and comparing the whole word cries wolf on every
 * boot (measured 2026-09-10, first mainline run).
 */
#define OVC_CLK_MUX_FIELDS	0x7fc
#define OVC_CLK_GATE_A_SET	0xD0	/* bits [5:0] */
#define OVC_CLK_GATE_A_CLR	0xD4
#define OVC_CLK_GATE_B_SET	0xD8	/* bits [9:1] */
#define OVC_CLK_GATE_B_CLR	0xDC
#define OVC_CLK_GATE_A_MASK	0x3F
#define OVC_CLK_GATE_B_MASK	0x3FE
#define OVC_RST0_ASSERT		0xE0
#define OVC_RST0_DEASSERT	0xE4
#define OVC_RST1_ASSERT		0xE8
#define OVC_RST1_DEASSERT	0xEC
/* Read views of the two reset groups: bit set = line held in reset. Deep-off
 * boot reads 0xffffffff / 0x007fffff; the vendor streaming state reads 0 / 0.
 * (device-confirmed 2026-09-01) */
#define OVC_RST0_STATUS		0x0C
#define OVC_RST1_STATUS		0x10
#define OVC_RST0_NLINES		32
#define OVC_RST1_NLINES		23
#define OVC_RST1_SKIP_LINE	20	/* mux-domain kick line; never in the sweeps (spec) */
#define OVC_RST0_CSI_LINES	0x00001c3c	/* csirx0 pixel/ppi/prst/sys + deskew0/1 + dphyrx */

/* AXI-master quiesce ctrl/status regs, in the 0x02400000 ISP file
 * (spec-vin-reset step 4/9): write 0xFFFFFFFF to ctrl, poll status clear,
 * zero ctrl afterwards. IFE / ITP / YUV masters. */
#define OVC_AXI_IFE_CTRL	0x00184
#define OVC_AXI_IFE_STAT	0x00188
#define OVC_AXI_ITP_CTRL	0x80144
#define OVC_AXI_ITP_STAT	0x80148
#define OVC_AXI_YUV_CTRL	0xC0148
#define OVC_AXI_YUV_STAT	0xC014C

/*
 * Format limits -- 4096x2400, not 3840x2160 (#98).
 *
 * Both old ceilings were reachable by a real source and neither was admitted:
 * the bench host outputs 4096x2160 (DCI 4K) and pins that mode regardless of
 * EDID, and the 16:10 EDID this project ships (#61) advertises 3840x2400.
 *
 * Nothing in the datapath is width-baked. The CSI-2 receiver is
 * format-transparent and already clamped at 4096 (open_vin_csi2.c); the
 * golden ISP image parameterises exactly four words on geometry
 * (ovc_golden_4k.h), and those words hold width and height in separate
 * 16-bit halves, so 4096 costs no bit. The real bound is the capture pool,
 * and ovc_queue_setup() enforces it dynamically rather than through these
 * constants: 56 MiB holds three 4096x2160 YUYV frames (16.88 MiB each) and
 * two 4096x2400 ones (18.75 MiB).
 */
#define OVC_MIN_WIDTH		64
#define OVC_MAX_WIDTH		4096
#define OVC_MIN_HEIGHT		64
#define OVC_MAX_HEIGHT		2400
/* min_queued_buffers + 1: what vb2 insists on before REQBUFS succeeds at all.
 * Keep the two in step -- ovc_queue_setup uses this to decide whether the
 * carveout can serve a geometry, and it is a floor, not a margin. */
#define OVC_MIN_BUFFERS		3
/* Default to the confirmed source geometry (live vendor capture = 3840x2160). */
#define OVC_DEF_WIDTH		3840
#define OVC_DEF_HEIGHT		2160

/* ------------------------------------------------------------------------ */
/* Driver structures                                                        */
/* ------------------------------------------------------------------------ */

struct ovc_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

/*
 * The two common-syscon gates that feed the VI/ISP domain. They are real CCF
 * clocks (drivers/clk/axera), and taking them is what keeps
 * clk_disable_unused() from gating the block out from under us -- the vendor
 * kernel never had to care because its bootloader left them running.
 */
enum ovc_clk {
	OVC_CLK_VI,
	OVC_CLK_ISP_MM,
	OVC_NUM_CLKS,
};

static const char * const ovc_clk_names[OVC_NUM_CLKS] = {
	[OVC_CLK_VI] = "vi",
	[OVC_CLK_ISP_MM] = "isp_mm",
};

struct ovc_dev {
	struct device *dev;
	void __iomem *regs;	/* 0x02400000 ISP/VIN register file */
	struct regmap *clkrst;	/* 0x02500000 isp syscon (clock/reset window) */
	struct clk_bulk_data clks[OVC_NUM_CLKS];
	int irq;

	struct v4l2_device v4l2_dev;
	struct video_device vdev;
	struct media_pad pad;	/* sink pad for the CSI-2 subdev (M1) */
	struct media_device mdev;

	/* M1 link: the CSI-2 receiver, bound over the DT graph. */
	struct v4l2_async_notifier notifier;
	struct v4l2_subdev *csi2;	/* bound open_vin_csi2 subdev, or NULL */
	bool notifier_registered;

	struct mutex lock;	/* serializes ioctls + queue ops */
	spinlock_t irqlock;	/* protects buf_list / active / sequence */

	struct vb2_queue queue;
	struct list_head buf_list;	/* queued, not yet given to hw */
	struct ovc_buffer *active;	/* buffer the WDMA writes next */
	unsigned int sequence;

	struct v4l2_pix_format fmt;
	/* Size of the reserved-memory pool, for the queue_setup buffer cap. */
	resource_size_t carveout_size;
};

static inline struct ovc_buffer *to_ovc_buffer(struct vb2_v4l2_buffer *vbuf)
{
	return container_of(vbuf, struct ovc_buffer, vb);
}

/* Plain ordered MMIO, per spec §0: no CDMA engine, RMW in software. */
static inline u32 ovc_rd(struct ovc_dev *ovc, u32 off)
{
	return readl(ovc->regs + off);
}

static inline void ovc_wr(struct ovc_dev *ovc, u32 off, u32 val)
{
	writel(val, ovc->regs + off);
}

/* clock/reset window (0x02500000) accessors, over the shared syscon regmap */
static inline void ovc_clkrst_wr(struct ovc_dev *ovc, u32 off, u32 val)
{
	regmap_write(ovc->clkrst, off, val);
}

static inline u32 ovc_clkrst_rd(struct ovc_dev *ovc, u32 off)
{
	unsigned int val = 0;

	regmap_read(ovc->clkrst, off, &val);
	return val;
}

/* new = (old & keep) | set  -- the vendor RMW idiom (spec §0.1, §4.3) */
static inline void ovc_rmw(struct ovc_dev *ovc, u32 off, u32 keep, u32 set)
{
	ovc_wr(ovc, off, (ovc_rd(ovc, off) & keep) | set);
}

/* ------------------------------------------------------------------------ */
/* vb2 mem_ops: dma_alloc_coherent over the declared carveout               */
/*                                                                          */
/* Modeled on mainline vb2-dma-contig / vb2-vmalloc, reduced to MMAP-mode   */
/* coherent allocations.  dma_alloc_coherent() on a device with a declared  */
/* coherent region allocates from that region first (4.19                   */
/* dma_alloc_from_dev_coherent; DMA_MEMORY_EXCLUSIVE = no fallback), so     */
/* buf->dma_addr is a physical DDR address inside the carveout -- exactly   */
/* what the WDMA address register consumes (spec §3.1).                     */
/* ------------------------------------------------------------------------ */

struct ovc_mem_buf {
	struct device *dev;
	void *vaddr;
	dma_addr_t dma_addr;
	unsigned long size;
	refcount_t refcount;
	struct vb2_vmarea_handler handler;
};

static void ovc_mem_put(void *buf_priv)
{
	struct ovc_mem_buf *buf = buf_priv;

	if (refcount_dec_and_test(&buf->refcount)) {
		dma_free_coherent(buf->dev, buf->size, buf->vaddr,
				  buf->dma_addr);
		kfree(buf);
	}
}

static void *ovc_mem_alloc(struct vb2_buffer *vb, struct device *dev,
			   unsigned long size)
{
	struct ovc_mem_buf *buf;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->size = size;
	buf->vaddr = dma_alloc_coherent(dev, size, &buf->dma_addr,
					GFP_KERNEL);
	if (!buf->vaddr) {
		dev_err(dev, "coherent alloc of %lu bytes failed (carveout full?)\n",
			size);
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = ovc_mem_put;
	buf->handler.arg = buf;
	refcount_set(&buf->refcount, 1);

	return buf;
}

static void *ovc_mem_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct ovc_mem_buf *buf = buf_priv;

	return buf->vaddr;
}

/* vb2-dma-contig convention: cookie points at the dma_addr_t */
static void *ovc_mem_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct ovc_mem_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static unsigned int ovc_mem_num_users(void *buf_priv)
{
	struct ovc_mem_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static int ovc_mem_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct ovc_mem_buf *buf = buf_priv;
	int ret;

	if (!buf)
		return -EINVAL;

	/*
	 * dma_mmap_* use vm_pgoff as an in-buffer offset, but vb2 encodes
	 * the buffer selector there (4.19 vb2_mmap does not clear it; the
	 * mainline dma-contig mmap op clears it the same way).
	 */
	vma->vm_pgoff = 0;

	/* Handled by dma_mmap_from_dev_coherent() for declared regions. */
	ret = dma_mmap_coherent(buf->dev, vma, buf->vaddr, buf->dma_addr,
				buf->size);
	if (ret) {
		pr_err(OVC_DRV_NAME ": mmap of coherent buffer failed: %d\n",
		       ret);
		return ret;
	}

	vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_private_data = &buf->handler;
	vma->vm_ops = &vb2_common_vm_ops;
	vma->vm_ops->open(vma);

	return 0;
}

/*
 * dma-buf export (VIDIOC_EXPBUF): the zero-copy hand-off to the open venc
 * (#60 M3). The buffer is physically contiguous and coherent, but the
 * carveout lies outside the kernel linear map (no struct pages), so the
 * generic dma_get_sgtable() exporter does not apply. This self-owned
 * exporter hands importers a one-entry sg_table that already carries the
 * bus address -- there is no IOMMU on this SoC and the memory is coherent,
 * so no dma_map_sg() is done on either side. The importer (our open VCMD
 * driver, HANTRO_IOCH_IMPORT_DMABUF) only reads sg_dma_address(); it never
 * touches sg_page(). Modeled on the 4.19 vb2-dma-contig exporter.
 */
struct ovc_dmabuf_attachment {
	struct sg_table sgt;
};

static int ovc_dmabuf_attach(struct dma_buf *dbuf,
			     struct dma_buf_attachment *att)
{
	struct ovc_mem_buf *buf = dbuf->priv;
	struct ovc_dmabuf_attachment *a;
	int ret;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;
	ret = sg_alloc_table(&a->sgt, 1, GFP_KERNEL);
	if (ret) {
		kfree(a);
		return ret;
	}
	a->sgt.sgl->offset = 0;
	a->sgt.sgl->length = buf->size;
	sg_dma_address(a->sgt.sgl) = buf->dma_addr;
	sg_dma_len(a->sgt.sgl) = buf->size;
	att->priv = a;
	return 0;
}

static void ovc_dmabuf_detach(struct dma_buf *dbuf,
			      struct dma_buf_attachment *att)
{
	struct ovc_dmabuf_attachment *a = att->priv;

	if (!a)
		return;
	sg_free_table(&a->sgt);
	kfree(a);
	att->priv = NULL;
}

static struct sg_table *ovc_dmabuf_map(struct dma_buf_attachment *att,
				       enum dma_data_direction dir)
{
	struct ovc_dmabuf_attachment *a = att->priv;

	return &a->sgt;
}

static void ovc_dmabuf_unmap(struct dma_buf_attachment *att,
			     struct sg_table *sgt, enum dma_data_direction dir)
{
}

static void ovc_dmabuf_release(struct dma_buf *dbuf)
{
	ovc_mem_put(dbuf->priv);
}

static int ovc_dmabuf_mmap(struct dma_buf *dbuf, struct vm_area_struct *vma)
{
	return ovc_mem_mmap(dbuf->priv, vma);
}

static int ovc_dmabuf_vmap(struct dma_buf *dbuf, struct iosys_map *map)
{
	struct ovc_mem_buf *buf = dbuf->priv;

	iosys_map_set_vaddr(map, buf->vaddr);
	return 0;
}

static const struct dma_buf_ops ovc_dmabuf_ops = {
	.attach		= ovc_dmabuf_attach,
	.detach		= ovc_dmabuf_detach,
	.map_dma_buf	= ovc_dmabuf_map,
	.unmap_dma_buf	= ovc_dmabuf_unmap,
	.release	= ovc_dmabuf_release,
	.mmap		= ovc_dmabuf_mmap,
	.vmap		= ovc_dmabuf_vmap,
};

static struct dma_buf *ovc_mem_get_dmabuf(struct vb2_buffer *vb,
					  void *buf_priv, unsigned long flags)
{
	struct ovc_mem_buf *buf = buf_priv;
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.ops = &ovc_dmabuf_ops;
	exp_info.size = buf->size;
	exp_info.flags = flags;
	exp_info.priv = buf;

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;

	/* the dma-buf holds its own reference; dropped in ovc_dmabuf_release */
	refcount_inc(&buf->refcount);
	return dbuf;
}

static const struct vb2_mem_ops ovc_mem_ops = {
	.alloc		= ovc_mem_alloc,
	.put		= ovc_mem_put,
	.vaddr		= ovc_mem_vaddr,
	.cookie		= ovc_mem_cookie,
	.num_users	= ovc_mem_num_users,
	.mmap		= ovc_mem_mmap,
	.get_dmabuf	= ovc_mem_get_dmabuf,
};

static dma_addr_t ovc_buf_dma_addr(struct vb2_buffer *vb)
{
	dma_addr_t *addr = vb2_plane_cookie(vb, 0);

	return *addr;
}

/* ------------------------------------------------------------------------ */
/* Hardware programming (spec order; enable/shadow bits LAST)               */
/* ------------------------------------------------------------------------ */

/*
 * ISP clock-domain MUX: field shifts and the vendor source-select CODES
 * (spec-vin-write-enable §1). Domains 0/1/2 at MUX_RD [10:8]/[7:5]/[4:2];
 * codes 5/5/3 reproduce the vendor's ISP source selection (~416/533/297 MHz).
 * These MUST be constants -- device-proven 2026-08-31 that writing 5/5/3 drives
 * MUX_RD to the golden 0x5af. Do NOT read the codes back from MUX_RD: once the
 * M1 CSI subdev is up it overwrites MUX_RD[3:0] with the CSI deskew-lock status
 * (0xf), so a read-and-restrobe reprograms the mux with garbage codes and can
 * wedge the ISP clock domain (observed: SIF/IFE fall back to 0xDEADBEEF).
 */
static const u8 ovc_clk_mux_shift[OVC_CLK_MUX_NFIELDS] = { 8, 5, 2 };
static const u8 ovc_clk_mux_code[OVC_CLK_MUX_NFIELDS]  = { 5, 5, 3 };

/*
 * Apply the ISP clock-source mux (spec-vin-write-enable §1/§5 step 1): CLR then
 * SET each domain's 3-bit source-select field to the vendor code. Device-proven
 * to drive MUX_RD to 0x5af (verified below). NECESSARY BUT NOT SUFFICIENT for
 * datapath writability: on-hardware 2026-08-31, even with MUX_RD=0x5af + gates +
 * IFE reset, the SIF (0x2406xxx)/IFE (0x2414xxx) windows stay 0xDEADBEEF and the
 * clock-level readbacks 0x025000C0/C4 stay 0 -- an upstream ISP source-PLL /
 * power-domain enable (the C0/C4 producers) is still missing. Tracked in
 * spec-isp-clock-enable.md; this step stays because it is a correct prerequisite
 * the PLL bring-up builds on.
 */
static void ovc_clk_mux_apply(struct ovc_dev *ovc)
{
	u32 after;
	int i;

	for (i = 0; i < OVC_CLK_MUX_NFIELDS; i++) {
		unsigned int sh = ovc_clk_mux_shift[i];

		ovc_clkrst_wr(ovc, OVC_CLK_MUX_CLR, OVC_CLK_MUX_FIELD << sh);
		ovc_clkrst_wr(ovc, OVC_CLK_MUX_SET,
			      (u32)ovc_clk_mux_code[i] << sh);
	}

	after = ovc_clkrst_rd(ovc, OVC_CLK_MUX_RD);
	dev_info(ovc->dev, "clk-src mux applied: MUX_RD=%#06x (want %#06x in bits [10:2])\n",
		 after, OVC_CLK_MUX_READY);
	if ((after & OVC_CLK_MUX_FIELDS) !=
	    (OVC_CLK_MUX_READY & OVC_CLK_MUX_FIELDS))
		dev_warn(ovc->dev,
			 "clk-src mux: MUX_RD %#06x != golden %#06x -- clock-source not selected (spec-vin-write-enable §6)\n",
			 after, OVC_CLK_MUX_READY);
}

/*
 * Clock / reset bring-up, from spec §8.3 (vendor global-create path) +
 * spec-vin-write-enable §5: pulse the mux-domain reset, apply the clock-source
 * mux (the write-enable), ungate clocks, THEN release the datapath reset --
 * order preserved from the vendor's ax_isp_clk_prepare / VIN_glb_create.
 */
static void ovc_clkrst_init(struct ovc_dev *ovc)
{
	int i, t;

	/*
	 * Reload-safe: if the register file is already live (a previous
	 * instance, or a warm boot that kept the domain up) do not re-run
	 * the reset sweeps -- they would tear down a CSI receiver that is
	 * already locked (device-observed 2026-09-01).
	 */
	if (ovc_rd(ovc, 0) != 0xdeadbeef) {
		dev_info(ovc->dev, "ISP register file already live (file[0]=%#010x, rst0=%#010x); skipping clk/rst bring-up\n",
			 ovc_rd(ovc, 0), ovc_clkrst_rd(ovc, OVC_RST0_STATUS));
		return;
	}

	/* Pulse reset-group-1 bit20 (spec §8.3 step 1 / clock-source mux
	 * domain kick -- the rst1-bit20 pulse the vendor's ax_isp_clk_prepare
	 * issues right before the mux rate/source setup). */
	ovc_clkrst_wr(ovc, OVC_RST1_ASSERT, BIT(20));
	ovc_clkrst_wr(ovc, OVC_RST1_DEASSERT, BIT(20));

	/*
	 * Apply the ISP clock-source mux (spec-vin-write-enable §1): selects
	 * the ISP clock source (MUX_RD -> 0x5af). A necessary prerequisite for
	 * datapath writability but not sufficient on its own -- the source PLL /
	 * power-domain that starts the clock (0x025000C0/C4) is still missing
	 * (spec-isp-clock-enable.md). Runs before the gates/reset.
	 */
	ovc_clk_mux_apply(ovc);

	/*
	 * Ungate clocks: the vendor's streaming state is gate-A 0x3F /
	 * gate-B 0x3FE (device-confirmed 2026-09-01; the earlier "full mask
	 * un-DEADBEEFs the blocks" reading was wrong -- the reset release below
	 * is what does that). W1S registers.
	 * (spec §8.3 step 2; docs/reference/deblob-scope/specs/spec-vin-reset.md)
	 */
	ovc_clkrst_wr(ovc, OVC_CLK_GATE_A_SET, 0x3F);
	ovc_clkrst_wr(ovc, OVC_CLK_GATE_B_SET, 0x3FE);

	/*
	 * spec-vin-reset steps 1-2: per-bit DEASSERT of EVERY rst0 and rst1
	 * line (one W1S strobe per bit). DEVICE-PROVEN 2026-09-01: this is the
	 * step that takes the whole ISP register file out of 0xDEADBEEF (the
	 * deep-off boot holds all 32 + 23 lines; the first draft released only
	 * the IFE subset and the file stayed dead). No hang.
	 */
	for (i = 0; i < OVC_RST0_NLINES; i++)
		ovc_clkrst_wr(ovc, OVC_RST0_DEASSERT, BIT(i));
	for (i = 0; i < OVC_RST1_NLINES; i++)
		if (i != OVC_RST1_SKIP_LINE)
			ovc_clkrst_wr(ovc, OVC_RST1_DEASSERT, BIT(i));

	/* step 4: quiesce the three AXI masters (IFE / ITP / YUV) */
	ovc_wr(ovc, OVC_AXI_IFE_CTRL, 0xFFFFFFFF);
	ovc_wr(ovc, OVC_AXI_ITP_CTRL, 0xFFFFFFFF);
	ovc_wr(ovc, OVC_AXI_YUV_CTRL, 0xFFFFFFFF);
	for (t = 0; t < 51; t++) {
		if (!ovc_rd(ovc, OVC_AXI_IFE_STAT) &&
		    !ovc_rd(ovc, OVC_AXI_ITP_STAT) &&
		    !ovc_rd(ovc, OVC_AXI_YUV_STAT))
			break;
		udelay(200);
	}

	/*
	 * step 5: per-bit rst0 PULSE sweep (assert then deassert, each line).
	 * The 2026-08-31 hang came from pulsing lines while OTHER lines were
	 * still held; once everything is deasserted first (above) the sweep
	 * is device-proven clean. Steps 6-8 (the rst1 pulse under the
	 * 0x0440306C hold) are deliberately skipped: they cover the ITP/YUV
	 * rst1 domains, which the bypass path does not use, and the hold
	 * register lives in the MM/VPP domain, which is unclocked on an open
	 * boot -- merely reading it hangs the bus (device-proven).
	 */
	for (i = 0; i < OVC_RST0_NLINES; i++) {
		/* Leave the CSI receiver's own lines (pixel/ppi/presetn/sys
		 * 2-5, deskew0/1 10-11, dphyrx 12) to open_vin_csi2: pulsing
		 * them after M1 has locked kills the front end (device-observed
		 * 2026-09-01 when this module was reloaded after M1). They are
		 * already released by the deassert-all above. */
		if (BIT(i) & OVC_RST0_CSI_LINES)
			continue;
		ovc_clkrst_wr(ovc, OVC_RST0_ASSERT, BIT(i));
		ovc_clkrst_wr(ovc, OVC_RST0_DEASSERT, BIT(i));
	}

	/* step 9: release the AXI masters */
	ovc_wr(ovc, OVC_AXI_IFE_CTRL, 0);
	ovc_wr(ovc, OVC_AXI_ITP_CTRL, 0);
	ovc_wr(ovc, OVC_AXI_YUV_CTRL, 0);

	dev_info(ovc->dev, "clk/rst up: mux=%#06x rst0=%#010x rst1=%#010x file[0]=%#010x\n",
		 ovc_clkrst_rd(ovc, OVC_CLK_MUX_RD),
		 ovc_clkrst_rd(ovc, OVC_RST0_STATUS),
		 ovc_clkrst_rd(ovc, OVC_RST1_STATUS), ovc_rd(ovc, 0));
}

/*
 * Bypass-path static configuration: SIF front-end, IFE core, WDMA channel
 * and the small top/AXI blocks, programmed from OUR OWN device-observed
 * register image (ovc_golden_4k.h) with the geometry words recomputed for
 * the negotiated format. Device-proven 2026-09-01: with this image (and the
 * SIF arm sequence in ovc_sif_start) the SIF frame counter runs at the
 * source rate and the IFE event bits match the vendor state exactly; the
 * first draft's hand-derived SIF matcher / WDMA format programming did not
 * (matcher layout, WDMA mode word, stride bank and several IFE bits were
 * wrong or missing). All plain writes; the ISP-top gate must already be
 * open (ovc_ife_bypass_setup).
 */
static void ovc_sif_setup(struct ovc_dev *ovc)
{
	u32 w = ovc->fmt.width, h = ovc->fmt.height;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ovc_golden_4k); i++) {
		const struct ovc_reg_init *r = &ovc_golden_4k[i];
		u32 v = r->val;

		switch (r->geom) {
		case OVC_GEOM_WH:
			v = w | (h << 16);
			break;
		case OVC_GEOM_H:
			v = h;
			break;
		case OVC_GEOM_STRIDE8:
			v = ovc->fmt.bytesperline / 8;
			break;
		default:
			break;
		}
		ovc_wr(ovc, r->off, v);
	}
}

/* SIF start (spec §1.4): pre-clear CTRL/ID -> STOP -> START -> re-arm */
static void ovc_sif_start(struct ovc_dev *ovc)
{
	const unsigned int d = OVC_SIF_DEV_ID;

	ovc_wr(ovc, OVC_SIF_CTRL(d), 0);
	ovc_wr(ovc, OVC_SIF_ID(d), 0);
	ovc_rmw(ovc, OVC_SIF_STOP, ~(u32)7, BIT(d) & 7);
	ovc_rmw(ovc, OVC_SIF_START, ~(u32)7, BIT(d) & 7);
	ovc_rmw(ovc, OVC_SIF_CTRL(d), ~(u32)7, BIT(d) & 7);
	/* ID word: the vendor streaming state reads 0x00010001 (two halfword
	 * ids), not d+1 -- device-confirmed 2026-09-01; with this arm sequence
	 * the SIF/IFE event bits match the vendor's exactly. */
	ovc_wr(ovc, OVC_SIF_ID(d), 0x00010001);

	/*
	 * §1.4 note: START is asserted BEFORE the vbus is enabled at node
	 * level, so the vbus enable comes last.
	 * Golden (vendor streaming, regdumps/regfile-vendor-live.bin and
	 * geom/regfile-vendor-fake1080p.bin, offset 0x640c..0x6418):
	 * VBUS_CTRL[0] = 0x00000001 (enable, mux 0, shift 0), VBUS_CTRL[1..3]
	 * = 0 -- exactly this write. Resolved 2026-09-04.
	 */
	ovc_rmw(ovc, OVC_SIF_VBUS_CTRL(0), OVC_SIF_VBUS_KEEP, BIT(0));
}

static void ovc_sif_stop(struct ovc_dev *ovc)
{
	/* §1.4: STOP then a blunt 40ms settle (vendor uses no status poll) */
	ovc_rmw(ovc, OVC_SIF_STOP, ~(u32)7, BIT(OVC_SIF_DEV_ID) & 7);
	msleep(40);
}

/*
 * ISP-top module gate (spec §2 "MODE10 bypass masks"), SET/CLR strobes with
 * a readable status word. Must run before any SIF/IFE/WDMA config write --
 * device-proven 2026-09-01: until the 0x8007 clear lands, every datapath
 * register write reads back 0.
 */
static void ovc_ife_bypass_setup(struct ovc_dev *ovc)
{
	u32 st;

	if (bypass_set_mask)
		ovc_wr(ovc, OVC_TOP_GATE_SET, bypass_set_mask);
	ovc_wr(ovc, OVC_TOP_GATE_CLR, bypass_clr_mask);
	st = ovc_rd(ovc, OVC_TOP_GATE_STATUS);
	dev_dbg(ovc->dev, "isp-top gate status %#010x (golden %#010x)\n",
		st, OVC_TOP_GATE_GOLDEN);
	if (st != OVC_TOP_GATE_GOLDEN)
		dev_warn(ovc->dev, "isp-top gate != golden: datapath writes may not stick\n");
}

/*
 * THE GATE (spec §3, device-confirmed): hand a DDR buffer to the WDMA.
 * MODE3 whole-frame = single partition, offset 0, so the vb2 dma_addr is
 * the value programmed (>>3: register holds phys[34:3], 8-byte aligned).
 * Shadow-commit bit0 latches the new address at the next frame boundary
 * (spec §3.2); set LAST per §4.
 */
static void ovc_wdma_program_buffer(struct ovc_dev *ovc, dma_addr_t dma_addr)
{
	const unsigned int c = wdma_chn;

	ovc_wr(ovc, OVC_WDMA_ADDR(c), lower_32_bits(dma_addr >> 3));
	/* per-frame token (low 16 bits) + the shadow-load strobe */
	ovc_rmw(ovc, OVC_WDMA_SEQ(c), 0xffff0000, ovc->sequence & 0xffff);
	ovc_rmw(ovc, OVC_WDMA_TRIG(c), ~(u32)BIT(0), BIT(0));
}

static void ovc_wdma_enable(struct ovc_dev *ovc, bool on)
{
	const unsigned int c = wdma_chn;

	ovc_rmw(ovc, OVC_WDMA_ENABLE(c), ~(u32)BIT(0), on ? BIT(0) : 0);
}

/* IFE core go (spec §2): RMW keep 0x0000f81c, set bit0 */
static void ovc_ife_go(struct ovc_dev *ovc, bool on)
{
	ovc_rmw(ovc, OVC_IFE_GO, OVC_IFE_GO_KEEP, on ? BIT(0) : 0);
}

/* Interrupts (spec §5.2/§5.4): ack first, then enable frame-done bit */
static void ovc_irq_setup(struct ovc_dev *ovc)
{
	unsigned int n;

	/* deterministic baseline: disable + ack every group of BOTH banks */
	for (n = 0; n < OVC_INT_NGROUPS; n++) {
		ovc_wr(ovc, OVC_INT_ENABLE(n), 0);
		ovc_wr(ovc, OVC_INT_CLEAR(n), 0xFFFFFFFF);
		ovc_wr(ovc, OVC_INT1_ENABLE(n), 0);
		ovc_wr(ovc, OVC_INT1_CLEAR(n), 0xFFFFFFFF);
	}

	/* group-4 frame-done for our channel (device-confirmed 0x200) */
	ovc_rmw(ovc, OVC_INT_ENABLE(OVC_INT_GRP_FDONE), ~(u32)0,
		OVC_INT_FDONE_BIT(wdma_chn));
}

static void ovc_irq_teardown(struct ovc_dev *ovc)
{
	ovc_wr(ovc, OVC_INT_ENABLE(OVC_INT_GRP_FDONE), 0);
	ovc_wr(ovc, OVC_INT_CLEAR(OVC_INT_GRP_FDONE), 0xFFFFFFFF);
}

/* ------------------------------------------------------------------------ */
/* Frame-done ISR                                                           */
/*                                                                          */
/* Bank-0 GIC line (DT interrupts index 0 = GIC_SPI 27; device-confirmed    */
/* ax_proton_intt on hwirq 59/60). The line can fire ~3x/frame when FSOF    */
/* is also enabled; we enable only group-4 frame-done and demux on its      */
/* status bit (spec §5.4).                                                  */
/* ------------------------------------------------------------------------ */

static irqreturn_t ovc_isr(int irq, void *priv)
{
	struct ovc_dev *ovc = priv;
	struct ovc_buffer *done = NULL;
	u32 status;

	bool other = false;
	unsigned int n;

	spin_lock(&ovc->irqlock);

	/*
	 * The GIC line is shared by every group of both interrupt banks.
	 * Ack anything pending that is not ours so the level line drops
	 * (otherwise the kernel sees an unhandled storm and disables IRQ 35
	 * -- "nobody cared", device-observed 2026-09-01).
	 */
	for (n = 0; n < OVC_INT_NGROUPS; n++) {
		u32 s;

		if (n == OVC_INT_GRP_FDONE)
			continue;
		s = ovc_rd(ovc, OVC_INT_MASKED(n));
		if (s) {
			ovc_wr(ovc, OVC_INT_CLEAR(n), s);
			other = true;
		}
		s = ovc_rd(ovc, OVC_INT1_MASKED(n));
		if (s) {
			ovc_wr(ovc, OVC_INT1_CLEAR(n), s);
			other = true;
		}
	}

	status = ovc_rd(ovc, OVC_INT_MASKED(OVC_INT_GRP_FDONE));
	if (!(status & OVC_INT_FDONE_BIT(wdma_chn))) {
		/* not ours; W1C anything latched in our group and bail */
		if (status)
			ovc_wr(ovc, OVC_INT_CLEAR(OVC_INT_GRP_FDONE), status);
		spin_unlock(&ovc->irqlock);
		return (status || other) ? IRQ_HANDLED : IRQ_NONE;
	}

	/* W1C ack (spec §5.4) */
	ovc_wr(ovc, OVC_INT_CLEAR(OVC_INT_GRP_FDONE), status);

	/*
	 * The WDMA just finished writing 'active'. If another buffer is
	 * queued, rotate: program its address + shadow-commit (latched at
	 * the upcoming frame boundary, spec §3.2) and complete 'active'.
	 * On underrun the hardware re-writes 'active' next frame and the
	 * frame is dropped (sequence still advances).
	 *
	 * TODO(bringup): this assumes frame-done N fires before the frame
	 * N+1 shadow latch (FSOF). Verify with checklist #2/#3 (addr reg
	 * toggling between two queued buffers, shadow bit0 pulsing); if
	 * the latch precedes frame-done, rotation must be re-keyed off
	 * FSOF (group 1 bit0).
	 */
	ovc->sequence++;
	if (ovc->active && !list_empty(&ovc->buf_list)) {
		struct ovc_buffer *next;

		next = list_first_entry(&ovc->buf_list, struct ovc_buffer,
					list);
		list_del(&next->list);
		ovc_wdma_program_buffer(ovc,
			ovc_buf_dma_addr(&next->vb.vb2_buf));
		done = ovc->active;
		ovc->active = next;
	} else if (ovc->active) {
		/*
		 * Underrun: the hardware only writes a frame after a shadow
		 * load, so re-arm on the same buffer (the frame is dropped)
		 * to keep frame-done coming -- device-proven 2026-09-01: one
		 * strobe == exactly one frame in DDR.
		 */
		ovc_wdma_program_buffer(ovc,
			ovc_buf_dma_addr(&ovc->active->vb.vb2_buf));
	}

	spin_unlock(&ovc->irqlock);

	if (done) {
		struct vb2_v4l2_buffer *vbuf = &done->vb;

		vbuf->vb2_buf.timestamp = ktime_get_ns();
		vbuf->sequence = ovc->sequence - 1;
		vbuf->field = V4L2_FIELD_NONE;
		vb2_set_plane_payload(&vbuf->vb2_buf, 0, ovc->fmt.sizeimage);
		vb2_buffer_done(&vbuf->vb2_buf, VB2_BUF_STATE_DONE);
	}

	return IRQ_HANDLED;
}

/* ------------------------------------------------------------------------ */
/* vb2 queue ops                                                            */
/* ------------------------------------------------------------------------ */

static int ovc_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			   unsigned int *nplanes, unsigned int sizes[],
			   struct device *alloc_devs[])
{
	struct ovc_dev *ovc = vb2_get_drv_priv(vq);

	if (*nplanes)
		return sizes[0] < ovc->fmt.sizeimage ? -EINVAL : 0;

	/*
	 * Never promise more buffers than the carveout holds -- otherwise vb2
	 * tries the allocation and dma_alloc_coherent logs a failure for every
	 * start. This, not OVC_MAX_WIDTH/HEIGHT, is what actually bounds the
	 * envelope.
	 *
	 * THE COST OF A BUFFER IS NOT ITS PAGE-ALIGNED SIZE (#98). A declared
	 * coherent region is a bitmap allocator, and dma_alloc_from_dev_coherent
	 * calls bitmap_find_free_region() with get_order(size) -- so every
	 * buffer costs a POWER-OF-TWO number of pages, aligned to itself. A
	 * 4096x2160 YUYV frame is 16.88 MiB and costs 32 MiB; 3840x2160 is
	 * 15.82 MiB and costs 16. Computing the cap from PAGE_ALIGN() instead
	 * promised three buffers the pool could not hold and failed inside vb2
	 * with nothing pointing at the reason -- measured on hardware, which is
	 * also where the 16 MiB step showed up: 3840x2160 allocates three
	 * buffers out of 56 MiB and 3840x2400 cannot allocate ONE.
	 *
	 * "Cannot allocate one" is literal: vb2 refuses the whole REQBUFS
	 * unless it gets min_queued_buffers + 1 = 3, so three is the floor for
	 * any geometry at all, not a comfort margin.
	 */
	if (ovc->carveout_size) {
		unsigned long cost = PAGE_SIZE << get_order(ovc->fmt.sizeimage);
		unsigned int max = div_u64(ovc->carveout_size, cost);

		if (max < OVC_MIN_BUFFERS) {
			dev_err(ovc->dev,
				"%ux%u: %u B/frame costs %lu MiB of the %llu MiB pool (order-rounded), so at most %u buffers -- vb2 needs %u\n",
				ovc->fmt.width, ovc->fmt.height,
				ovc->fmt.sizeimage, cost >> 20,
				(u64)ovc->carveout_size >> 20, max,
				OVC_MIN_BUFFERS);
			return -ENOMEM;
		}
		if (*nbuffers > max)
			*nbuffers = max;
	}

	*nplanes = 1;
	sizes[0] = ovc->fmt.sizeimage;
	return 0;
}

static int ovc_buf_prepare(struct vb2_buffer *vb)
{
	struct ovc_dev *ovc = vb2_get_drv_priv(vb->vb2_queue);
	dma_addr_t dma = ovc_buf_dma_addr(vb);

	if (vb2_plane_size(vb, 0) < ovc->fmt.sizeimage)
		return -EINVAL;

	/* WDMA address register holds phys[34:3]: 8-byte aligned (spec §3) */
	if (WARN_ON_ONCE(dma & 7))
		return -EINVAL;

	vb2_set_plane_payload(vb, 0, ovc->fmt.sizeimage);
	return 0;
}

static void ovc_buf_queue(struct vb2_buffer *vb)
{
	struct ovc_dev *ovc = vb2_get_drv_priv(vb->vb2_queue);
	struct ovc_buffer *buf = to_ovc_buffer(to_vb2_v4l2_buffer(vb));
	unsigned long flags;

	spin_lock_irqsave(&ovc->irqlock, flags);
	list_add_tail(&buf->list, &ovc->buf_list);
	spin_unlock_irqrestore(&ovc->irqlock, flags);
}

/* Hand every queued buffer back to vb2 in @state (start failure / stop). */
static void ovc_return_buffers(struct ovc_dev *ovc, enum vb2_buffer_state state)
{
	struct ovc_buffer *buf, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&ovc->irqlock, flags);
	if (ovc->active) {
		vb2_buffer_done(&ovc->active->vb.vb2_buf, state);
		ovc->active = NULL;
	}
	list_for_each_entry_safe(buf, tmp, &ovc->buf_list, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
	spin_unlock_irqrestore(&ovc->irqlock, flags);
}

/*
 * M1 fan-out: start/stop the receiver with our stream. -ENOIOCTLCMD (no
 * subdev bound) is not an error -- the receiver may be running standalone
 * (bench mode).
 */
static int ovc_csi2_stream(struct ovc_dev *ovc, bool on)
{
	struct v4l2_subdev *sd = ovc->csi2;
	int ret;

	if (!sd)
		return 0;

	/*
	 * Format is NOT pushed to the receiver. It programs no geometry of any
	 * kind -- it forwards whatever the bridge emits -- and a set_fmt call
	 * with a NULL subdev state is a NULL dereference on a modern subdev.
	 * The graph carries the format; the hardware does not care.
	 */
	ret = v4l2_subdev_call(sd, video, s_stream, on);
	return (ret == -ENOIOCTLCMD) ? 0 : ret;
}

/* Block new AXI requests and wait for outstanding transfers to drain. */
static int ovc_axi_quiesce(struct ovc_dev *ovc)
{
	u32 busy;
	int ret;

	ovc_wr(ovc, OVC_AXI_IFE_CTRL, U32_MAX);
	ovc_wr(ovc, OVC_AXI_ITP_CTRL, U32_MAX);
	ovc_wr(ovc, OVC_AXI_YUV_CTRL, U32_MAX);
	ret = read_poll_timeout(ovc_rd, busy, !busy, 200, 10000,
				false, ovc, OVC_AXI_IFE_STAT);
	if (!ret)
		ret = read_poll_timeout(ovc_rd, busy, !busy, 200, 10000,
					false, ovc, OVC_AXI_ITP_STAT);
	if (!ret)
		ret = read_poll_timeout(ovc_rd, busy, !busy, 200, 10000,
					false, ovc, OVC_AXI_YUV_STAT);
	return ret;
}

static void ovc_axi_resume(struct ovc_dev *ovc)
{
	ovc_wr(ovc, OVC_AXI_IFE_CTRL, 0);
	ovc_wr(ovc, OVC_AXI_ITP_CTRL, 0);
	ovc_wr(ovc, OVC_AXI_YUV_CTRL, 0);
}

static int ovc_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct ovc_dev *ovc = vb2_get_drv_priv(vq);
	unsigned long flags;
	int ret;

	ret = pm_runtime_resume_and_get(ovc->dev);
	if (ret < 0) {
		ovc_return_buffers(ovc, VB2_BUF_STATE_QUEUED);
		return ret;
	}

	/* A quick restart may cancel autosuspend while AXI is still quiesced. */
	ovc_axi_resume(ovc);
	/* Receiver first: no point programming the datapath without a link. */
	ret = ovc_csi2_stream(ovc, true);
	if (ret) {
		dev_err(ovc->dev, "CSI-2 receiver start failed: %d\n", ret);
		ovc_return_buffers(ovc, VB2_BUF_STATE_QUEUED);
		pm_runtime_mark_last_busy(ovc->dev);
		pm_runtime_put_autosuspend(ovc->dev);
		return ret;
	}

	/*
	 * Full bring-up order per spec §4.1/§4.2 and the §1.4/§2 notes:
	 * static config first, every enable/commit bit last:
	 *   ISP-top gate (must precede every datapath write) -> SIF front-end
	 *   -> WDMA channel config -> first buffer addr + shadow -> IFE-go ->
	 *   WDMA channel enable -> IRQ enable -> SIF START (begins capture)
	 *   -> vbus.
	 */
	ovc_ife_bypass_setup(ovc);
	ovc_sif_setup(ovc);

	/* data-source mux enable strobe (write-only; mirror +0x16c = 0x30) */
	ovc_wr(ovc, OVC_TOP_MUX_EN, OVC_TOP_MUX_VAL);
	/* go RMW under the vendor's keep-mask, then the flush word */
	ovc_ife_go(ovc, true);
	ovc_wr(ovc, OVC_IFE_FLUSH, 0xffffffff);
	ovc_wdma_enable(ovc, true);
	ovc_irq_setup(ovc);
	ovc_sif_start(ovc);

	/*
	 * First buffer + shadow-load strobe LAST, once the SIF is streaming:
	 * the vendor issues address + trigger per frame from its scheduler
	 * after pipe start, and a strobe issued before SIF start is lost
	 * (device-observed 2026-09-01: 0 frames vs 1 frame per strobe).
	 */
	spin_lock_irqsave(&ovc->irqlock, flags);
	ovc->sequence = 0;
	ovc->active = list_first_entry(&ovc->buf_list, struct ovc_buffer,
				       list);
	list_del(&ovc->active->list);
	ovc_wdma_program_buffer(ovc,
		ovc_buf_dma_addr(&ovc->active->vb.vb2_buf));
	spin_unlock_irqrestore(&ovc->irqlock, flags);

	return 0;
}

static void ovc_stop_streaming(struct vb2_queue *vq)
{
	struct ovc_dev *ovc = vb2_get_drv_priv(vq);

	/* The frame-done ISR must not re-arm WDMA while we stop it. */
	disable_irq(ovc->irq);
	/* reverse order: stop the source, then the datapath, then the IRQ */
	ovc_sif_stop(ovc);
	/* vendor stop case: clear the shadow-load trigger */
	ovc_rmw(ovc, OVC_WDMA_TRIG(wdma_chn), ~(u32)BIT(0), 0);
	ovc_wdma_enable(ovc, false);
	ovc_ife_go(ovc, false);
	ovc_irq_teardown(ovc);

	/* Receiver last, mirroring start (it was brought up first). */
	ovc_csi2_stream(ovc, false);
	if (ovc_axi_quiesce(ovc))
		dev_warn(ovc->dev, "AXI drain timed out at STREAMOFF\n");
	ovc_return_buffers(ovc, VB2_BUF_STATE_ERROR);
	enable_irq(ovc->irq);
	pm_runtime_mark_last_busy(ovc->dev);
	pm_runtime_put_autosuspend(ovc->dev);
}

static const struct vb2_ops ovc_vb2_ops = {
	.queue_setup		= ovc_queue_setup,
	.buf_prepare		= ovc_buf_prepare,
	.buf_queue		= ovc_buf_queue,
	.start_streaming	= ovc_start_streaming,
	.stop_streaming		= ovc_stop_streaming,
};

/* ------------------------------------------------------------------------ */
/* V4L2 ioctl ops                                                           */
/* ------------------------------------------------------------------------ */

/* The bypass WDMA packing is Y0 U Y1 V (YUYV), byte-identical to the vendor
 * pool frames and to what libkvm's open venc/soft-JPEG consume. The 2026-09-01
 * "UYVY, byte 1 is luma" reading came from frames stored through the reset
 * value of WDMA 0x142f8 (12-bit samples << 4): the odd byte then carried the
 * luma's upper nibble. Fixed in the golden table (2026-09-02). */
static const u32 ovc_pix_formats[] = {
	V4L2_PIX_FMT_YUYV,
};

static void ovc_fill_pix_format(struct v4l2_pix_format *pix)
{
	pix->width = clamp_t(u32, pix->width, OVC_MIN_WIDTH, OVC_MAX_WIDTH) & ~1U;
	pix->height = clamp_t(u32, pix->height, OVC_MIN_HEIGHT, OVC_MAX_HEIGHT);
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	/* packed 4:2:2, stride == width (spec target path: zero ISP) */
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;
}

static int ovc_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	strscpy(cap->driver, OVC_DRV_NAME, sizeof(cap->driver));
	strscpy(cap->card, "AX630C open VIN/IFE capture", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 OVC_DRV_NAME);
	/* capabilities / device_caps are filled from vdev->device_caps by the
	 * core after this returns; setting them here would be dead stores. */
	return 0;
}

static int ovc_enum_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(ovc_pix_formats))
		return -EINVAL;
	f->pixelformat = ovc_pix_formats[f->index];
	return 0;
}

static int ovc_g_fmt_vid_cap(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct ovc_dev *ovc = video_drvdata(file);

	f->fmt.pix = ovc->fmt;
	return 0;
}

static int ovc_try_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ovc_pix_formats); i++)
		if (f->fmt.pix.pixelformat == ovc_pix_formats[i])
			break;
	if (i == ARRAY_SIZE(ovc_pix_formats))
		f->fmt.pix.pixelformat = ovc_pix_formats[0];

	ovc_fill_pix_format(&f->fmt.pix);
	return 0;
}

static int ovc_s_fmt_vid_cap(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct ovc_dev *ovc = video_drvdata(file);
	int ret;

	if (vb2_is_busy(&ovc->queue))
		return -EBUSY;

	ret = ovc_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	/*
	 * Userspace states the source geometry (the HDMI->MIPI bridge is not
	 * queryable from here); start_streaming pushes it to the bound CSI-2
	 * subdev via set_fmt so the media graph carries the same format on
	 * both ends (ovc_csi2_stream). Same contract as the open capture
	 * stack has had since #17.
	 */
	ovc->fmt = f->fmt.pix;
	return 0;
}

static int ovc_enum_input(struct file *file, void *priv,
			  struct v4l2_input *i)
{
	if (i->index)
		return -EINVAL;
	i->type = V4L2_INPUT_TYPE_CAMERA;
	strscpy(i->name, "MIPI CSI-2 (HDMI-in)", sizeof(i->name));
	return 0;
}

static int ovc_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int ovc_s_input(struct file *file, void *priv, unsigned int i)
{
	return i ? -EINVAL : 0;
}

static int ovc_enum_framesizes(struct file *file, void *priv,
			       struct v4l2_frmsizeenum *fsize)
{
	unsigned int i;

	if (fsize->index)
		return -EINVAL;
	for (i = 0; i < ARRAY_SIZE(ovc_pix_formats); i++)
		if (fsize->pixel_format == ovc_pix_formats[i])
			break;
	if (i == ARRAY_SIZE(ovc_pix_formats))
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = OVC_MIN_WIDTH;
	fsize->stepwise.max_width = OVC_MAX_WIDTH;
	fsize->stepwise.step_width = 2;
	fsize->stepwise.min_height = OVC_MIN_HEIGHT;
	fsize->stepwise.max_height = OVC_MAX_HEIGHT;
	fsize->stepwise.step_height = 1;
	return 0;
}

static const struct v4l2_ioctl_ops ovc_ioctl_ops = {
	.vidioc_querycap		= ovc_querycap,
	.vidioc_enum_fmt_vid_cap	= ovc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= ovc_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= ovc_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= ovc_try_fmt_vid_cap,
	.vidioc_enum_input		= ovc_enum_input,
	.vidioc_g_input			= ovc_g_input,
	.vidioc_s_input			= ovc_s_input,
	.vidioc_enum_framesizes		= ovc_enum_framesizes,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations ovc_fops = {
	.owner		= THIS_MODULE,
	.open		= v4l2_fh_open,
	.release	= vb2_fop_release,
	.unlocked_ioctl	= video_ioctl2,
	.read		= vb2_fop_read,
	.mmap		= vb2_fop_mmap,
	.poll		= vb2_fop_poll,
};

/* ------------------------------------------------------------------------ */
/* Probe / remove                                                           */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* M1 subdev binding (v4l2-async)                                            */
/* ------------------------------------------------------------------------ */

static inline struct ovc_dev *notifier_to_ovc(struct v4l2_async_notifier *n)
{
	return container_of(n, struct ovc_dev, notifier);
}

static int ovc_notifier_bound(struct v4l2_async_notifier *notifier,
			      struct v4l2_subdev *sd,
			      struct v4l2_async_connection *asc)
{
	struct ovc_dev *ovc = notifier_to_ovc(notifier);
	int ret;

	if (sd->entity.num_pads <= OVC_CSI2_SOURCE_PAD) {
		dev_err(ovc->dev, "subdev %s has no source pad %d\n",
			sd->name, OVC_CSI2_SOURCE_PAD);
		return -EINVAL;
	}

	ret = media_create_pad_link(&sd->entity, OVC_CSI2_SOURCE_PAD,
				    &ovc->vdev.entity, 0,
				    MEDIA_LNK_FL_ENABLED |
				    MEDIA_LNK_FL_IMMUTABLE);
	if (ret) {
		dev_err(ovc->dev, "link %s:%d -> %s:0 failed: %d\n", sd->name,
			OVC_CSI2_SOURCE_PAD, ovc->vdev.name, ret);
		return ret;
	}

	ovc->csi2 = sd;
	dev_info(ovc->dev, "bound CSI-2 subdev %s (%s)\n", sd->name,
		 dev_name(sd->dev));
	return 0;
}

static void ovc_notifier_unbind(struct v4l2_async_notifier *notifier,
				struct v4l2_subdev *sd,
				struct v4l2_async_connection *asc)
{
	struct ovc_dev *ovc = notifier_to_ovc(notifier);

	ovc->csi2 = NULL;
	dev_info(ovc->dev, "CSI-2 subdev %s unbound\n", sd->name);
}

static int ovc_notifier_complete(struct v4l2_async_notifier *notifier)
{
	struct ovc_dev *ovc = notifier_to_ovc(notifier);

	/* /dev/v4l-subdevN for the receiver (log_status, controls). */
	return v4l2_device_register_subdev_nodes(&ovc->v4l2_dev);
}

static const struct v4l2_async_notifier_operations ovc_notifier_ops = {
	.bound		= ovc_notifier_bound,
	.unbind		= ovc_notifier_unbind,
	.complete	= ovc_notifier_complete,
};

/*
 * The graph edge. This node has one port with one endpoint, whose remote is
 * the CSI-2 receiver's source endpoint; v4l2_async_nf_add_fwnode_remote()
 * resolves that remote and the notifier binds whichever subdev registers for
 * it. Replaces the 4.19 V4L2_ASYNC_MATCH_DEVNAME on "2600000.mipi_rx", which
 * existed only because the vendor DT had no ports at all.
 */
static int ovc_notifier_init(struct ovc_dev *ovc)
{
	struct v4l2_async_connection *asc;
	struct fwnode_handle *ep;
	int ret;

	ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(ovc->dev), 0, 0,
					     FWNODE_GRAPH_ENDPOINT_NEXT);
	if (!ep) {
		dev_info(ovc->dev,
			 "no port/endpoint: running without a CSI-2 subdev\n");
		return 0;
	}

	v4l2_async_nf_init(&ovc->notifier, &ovc->v4l2_dev);
	ovc->notifier.ops = &ovc_notifier_ops;

	asc = v4l2_async_nf_add_fwnode_remote(&ovc->notifier, ep,
					      struct v4l2_async_connection);
	fwnode_handle_put(ep);
	if (IS_ERR(asc)) {
		ret = PTR_ERR(asc);
		dev_err(ovc->dev, "async connection add failed: %d\n", ret);
		goto err_cleanup;
	}

	ret = v4l2_async_nf_register(&ovc->notifier);
	if (ret) {
		dev_err(ovc->dev, "async notifier register failed: %d\n", ret);
		goto err_cleanup;
	}

	ovc->notifier_registered = true;
	return 0;

err_cleanup:
	v4l2_async_nf_cleanup(&ovc->notifier);
	return ret;
}

static int ovc_runtime_suspend(struct device *dev)
{
	struct ovc_dev *ovc = dev_get_drvdata(dev);
	int ret;

	/*
	 * STREAMOFF has stopped WDMA and the CSI PHY. Drain IRQs and AXI
	 * before gating the internal ISP clocks or their common parents.
	 */
	disable_irq(ovc->irq);
	ret = ovc_axi_quiesce(ovc);
	if (ret) {
		/* Keep requests blocked until the next STREAMON. */
		enable_irq(ovc->irq);
		dev_warn_ratelimited(dev, "AXI masters busy; keeping VIN clocks on\n");
		return -EBUSY;
	}
	ovc_clkrst_wr(ovc, OVC_CLK_GATE_B_CLR, OVC_CLK_GATE_B_MASK);
	ovc_clkrst_wr(ovc, OVC_CLK_GATE_A_CLR, OVC_CLK_GATE_A_MASK);
	clk_bulk_disable_unprepare(OVC_NUM_CLKS, ovc->clks);
	return 0;
}

static int ovc_runtime_resume(struct device *dev)
{
	struct ovc_dev *ovc = dev_get_drvdata(dev);
	int ret;

	ret = clk_bulk_prepare_enable(OVC_NUM_CLKS, ovc->clks);
	if (ret)
		return ret;
	/*
	 * Clock gating retains mux/register state. Never repeat the global
	 * reset sweep here: it also covers the CSI receiver's reset lines.
	 */
	ovc_clkrst_wr(ovc, OVC_CLK_GATE_A_SET, OVC_CLK_GATE_A_MASK);
	ovc_clkrst_wr(ovc, OVC_CLK_GATE_B_SET, OVC_CLK_GATE_B_MASK);
	ovc_axi_resume(ovc);
	enable_irq(ovc->irq);
	return 0;
}

static const struct dev_pm_ops ovc_pm_ops = {
	RUNTIME_PM_OPS(ovc_runtime_suspend, ovc_runtime_resume, NULL)
};

static int ovc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ovc_dev *ovc;
	struct device_node *rmem_np;
	struct resource *res;
	struct vb2_queue *q;
	unsigned int i;
	int irq, ret;

	ovc = devm_kzalloc(dev, sizeof(*ovc), GFP_KERNEL);
	if (!ovc)
		return -ENOMEM;
	ovc->dev = dev;

	/* The DT reg covers 0x02400000 + 0x100000; the register file proper
	 * is 0xd4008 long (spec §0 naming note). Map what DT provides. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ovc->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(ovc->regs))
		return PTR_ERR(ovc->regs);

	/* Clock/reset window (spec §8): a syscon phandle, shared with whoever
	 * else ever describes that window. */
	ovc->clkrst = syscon_regmap_lookup_by_phandle(dev->of_node,
						      "axera,isp-syscon");
	if (IS_ERR(ovc->clkrst))
		return dev_err_probe(dev, PTR_ERR(ovc->clkrst),
				     "axera,isp-syscon\n");

	for (i = 0; i < OVC_NUM_CLKS; i++)
		ovc->clks[i].id = ovc_clk_names[i];
	ret = devm_clk_bulk_get(dev, OVC_NUM_CLKS, ovc->clks);
	if (ret)
		return dev_err_probe(dev, ret, "VI/ISP domain clocks\n");

	/* Bank-0 line (DT index 0 = GIC_SPI 27, device-confirmed §5.5). */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	ovc->irq = irq;

	ret = dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	/*
	 * Capture-buffer carveout: a `shared-dma-pool` reserved-memory node
	 * named by `memory-region`. dma_alloc_coherent() then comes out of it
	 * with no fallback to the page allocator, so a full pool fails loudly
	 * instead of handing out memory the WDMA cannot reach. On 4.19 this
	 * was dma_declare_coherent_memory() over numbers a shell loader
	 * computed from the board id.
	 */
	ret = of_reserved_mem_device_init(dev);
	if (ret)
		return dev_err_probe(dev, ret, "capture memory-region\n");

	rmem_np = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (rmem_np) {
		struct reserved_mem *rmem = of_reserved_mem_lookup(rmem_np);

		if (rmem)
			ovc->carveout_size = rmem->size;
		of_node_put(rmem_np);
	}
	dev_info(dev, "capture carveout %pa bytes\n", &ovc->carveout_size);

	/*
	 * The VI/ISP domain gates, before the first register access. Nothing
	 * on mainline enables them for us.
	 */
	ret = clk_bulk_prepare_enable(OVC_NUM_CLKS, ovc->clks);
	if (ret) {
		dev_err(dev, "VI/ISP clock enable failed: %d\n", ret);
		goto err_rmem;
	}

	mutex_init(&ovc->lock);
	spin_lock_init(&ovc->irqlock);
	INIT_LIST_HEAD(&ovc->buf_list);

	ovc->fmt.pixelformat = ovc_pix_formats[0];
	ovc->fmt.width = OVC_DEF_WIDTH;
	ovc->fmt.height = OVC_DEF_HEIGHT;
	ovc_fill_pix_format(&ovc->fmt);

	/*
	 * VIN domain clock/reset bring-up (spec §8.3) at probe -- the open
	 * boot has no ax_proton to do it. Note: the sensor MCLKs in the DT
	 * clocks property are sensor-side only and irrelevant for the HDMI
	 * bypass path (spec §8.1), so no devm_clk_get here.
	 * Must run before mipi_rx/D-PHY bring-up (spec §8.2 ordering:
	 * proton-first is FORCED -- the reset pulse covers the RX front-end).
	 */
	ovc_clkrst_init(ovc);
	/* Open the ISP-top module gate now so the register file is live for
	 * everything that follows (probe-time int-ctrl programming included). */
	ovc_ife_bypass_setup(ovc);

	ret = devm_request_irq(dev, irq, ovc_isr, 0, OVC_DRV_NAME, ovc);
	if (ret) {
		dev_err(dev, "request_irq(%d) failed: %d\n", irq, ret);
		goto err_clk;
	}
	ovc_irq_setup(ovc);
	ovc_irq_teardown(ovc);
	platform_set_drvdata(pdev, ovc);
	pm_runtime_set_active(dev);
	pm_runtime_get_noresume(dev);
	pm_runtime_set_autosuspend_delay(dev, 1000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);

	/* Media controller: the graph is csi2 (source pad 1) -> video0 (pad 0). */
	ovc->mdev.dev = dev;
	strscpy(ovc->mdev.model, "AX630C VIN capture", sizeof(ovc->mdev.model));
	snprintf(ovc->mdev.bus_info, sizeof(ovc->mdev.bus_info),
		 "platform:%s", dev_name(dev));
	media_device_init(&ovc->mdev);
	ovc->v4l2_dev.mdev = &ovc->mdev;

	ret = v4l2_device_register(dev, &ovc->v4l2_dev);
	if (ret) {
		media_device_cleanup(&ovc->mdev);
		goto err_pm;
	}

	q = &ovc->queue;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_READ;
	q->drv_priv = ovc;
	q->buf_struct_size = sizeof(struct ovc_buffer);
	q->ops = &ovc_vb2_ops;
	q->mem_ops = &ovc_mem_ops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	/* OVC_MIN_BUFFERS is this + 1; ovc_queue_setup depends on the pair. */
	q->min_queued_buffers = OVC_MIN_BUFFERS - 1;
	q->dev = dev;
	q->lock = &ovc->lock;
	ret = vb2_queue_init(q);
	if (ret)
		goto err_v4l2;

	/*
	 * Media pad: this video node is the sink of the M1 open_vin_csi2
	 * subdev; the link itself is made when the notifier binds it (see
	 * ovc_notifier_bound and csi2_devname).
	 */
	ovc->pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&ovc->vdev.entity, 1, &ovc->pad);
	if (ret)
		goto err_v4l2;

	ovc->vdev.fops = &ovc_fops;
	ovc->vdev.ioctl_ops = &ovc_ioctl_ops;
	ovc->vdev.release = video_device_release_empty;
	ovc->vdev.v4l2_dev = &ovc->v4l2_dev;
	ovc->vdev.queue = q;
	ovc->vdev.lock = &ovc->lock;
	/*
	 * REQUIRED since 5.4, and its absence is a WARN plus -EINVAL out of
	 * __video_register_device with nothing that names the field
	 * (v4l2-dev.c: `WARN_ON(type != VFL_TYPE_SUBDEV && !vdev->device_caps)`).
	 * On 4.19 only vidioc_querycap filled this in, which is why the port
	 * did not carry it. Measured on hardware 2026-09-10.
	 */
	ovc->vdev.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
				V4L2_CAP_READWRITE;
	strscpy(ovc->vdev.name, OVC_DRV_NAME, sizeof(ovc->vdev.name));
	video_set_drvdata(&ovc->vdev, ovc);

	ret = video_register_device(&ovc->vdev, VFL_TYPE_VIDEO, -1);
	if (ret) {
		dev_err(dev, "video_register_device failed: %d\n", ret);
		goto err_entity;
	}

	ret = media_device_register(&ovc->mdev);
	if (ret) {
		dev_err(dev, "media_device_register failed: %d\n", ret);
		goto err_vdev;
	}

	platform_set_drvdata(pdev, ovc);
	dev_info(dev, "registered %s as /dev/video%d (WDMA chn %u)\n",
		 OVC_DRV_NAME, ovc->vdev.num, wdma_chn);

	/*
	 * Last: the async notifier for the CSI-2 subdev, over this node's own
	 * `port { endpoint }`. If open_vin_csi2 is already registered this
	 * binds synchronously from here (module order does not matter).
	 * Registered after the video node so the media link has both ends.
	 */
	ret = ovc_notifier_init(ovc);
	if (ret)
		goto err_mdev;

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
	return 0;

err_mdev:
	media_device_unregister(&ovc->mdev);
err_vdev:
	video_unregister_device(&ovc->vdev);
err_entity:
	media_entity_cleanup(&ovc->vdev.entity);
err_v4l2:
	v4l2_device_unregister(&ovc->v4l2_dev);
	media_device_cleanup(&ovc->mdev);
err_pm:
	pm_runtime_disable(dev);
	pm_runtime_put_noidle(dev);
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_set_suspended(dev);
err_clk:
	clk_bulk_disable_unprepare(OVC_NUM_CLKS, ovc->clks);
err_rmem:
	of_reserved_mem_device_release(dev);
	return ret;
}

static void ovc_remove(struct platform_device *pdev)
{
	struct ovc_dev *ovc = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int ret;

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		dev_err(dev, "cannot resume VIN for removal: %d\n", ret);
	pm_runtime_disable(dev);

	if (ovc->notifier_registered) {
		v4l2_async_nf_unregister(&ovc->notifier);
		v4l2_async_nf_cleanup(&ovc->notifier);
	}
	media_device_unregister(&ovc->mdev);
	video_unregister_device(&ovc->vdev);
	media_entity_cleanup(&ovc->vdev.entity);
	v4l2_device_unregister(&ovc->v4l2_dev);
	media_device_cleanup(&ovc->mdev);
	if (ret >= 0) {
		disable_irq(ovc->irq);
		clk_bulk_disable_unprepare(OVC_NUM_CLKS, ovc->clks);
		pm_runtime_put_noidle(dev);
	}
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_set_suspended(dev);
	of_reserved_mem_device_release(&pdev->dev);
}

static const struct of_device_id ovc_of_match[] = {
	{ .compatible = "axera,ax630c-vin" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ovc_of_match);

static struct platform_driver ovc_driver = {
	.probe	= ovc_probe,
	.remove	= ovc_remove,
	.driver	= {
		.name		= OVC_DRV_NAME,
		.of_match_table	= ovc_of_match,
		.pm		= pm_ptr(&ovc_pm_ops),
	},
};
module_platform_driver(ovc_driver);

MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS("DMA_BUF");	/* VIDIOC_EXPBUF: the zero-copy hand-off to the encoder */
MODULE_AUTHOR("open-nanokvm-pro contributors");
MODULE_DESCRIPTION("Open V4L2 VIN/IFE capture node for AX630C (ax_proton bypass replacement, #59/#83)");
