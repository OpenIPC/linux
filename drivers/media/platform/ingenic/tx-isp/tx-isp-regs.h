/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_REGS_H__
#define __TX_ISP_REGS_H__

#include <linux/bits.h>
#include <linux/bitfield.h>

/* =========================================================================
 * 1. ISP Top / Global Control (sd->base)
 * ========================================================================= */
#define ISP_TOP_REG_00				0x0000 /* undocumented */
#define ISP_TOP_IMAGE_SIZE			0x0004 /* Input image resolution */
#define  ISP_TOP_IMAGE_WIDTH_MASK		GENMASK(31, 16)
#define  ISP_TOP_IMAGE_HEIGHT_MASK		GENMASK(15, 0)
#define  ISP_TOP_IMAGE_SIZE_VAL(w, h)		(((w) << 16) | ((h) & 0xffff))

#define ISP_TOP_BAYER_ORDER			0x0008 /* Sensor Bayer pattern order */
#define  ISP_TOP_BAYER_RGGB			0
#define  ISP_TOP_BAYER_BGGR			1
#define  ISP_TOP_BAYER_GBRG			2
#define  ISP_TOP_BAYER_GRBG			3

#define ISP_TOP_CTRL				0x000c /* ISP global top control */
#define  ISP_TOP_CTRL_ENABLE			BIT(0)
#define  ISP_TOP_CTRL_DEFAULT			0xb4742069 /* undocumented */

#define ISP_TOP_REG_10				0x0010 /* undocumented */
#define  ISP_TOP_REG_10_DEFAULT			0x000000f3 /* undocumented */

#define ISP_TOP_REG_1C				0x001c /* undocumented */
#define  ISP_TOP_REG_1C_DEFAULT			0x00000000 /* undocumented */

#define ISP_TOP_RESET_CTRL			0x0020 /* Soft reset control */
#define  ISP_TOP_RESET_RELEASE			0x00000000

#define ISP_TOP_RESET_TRIG			0x0024 /* Soft reset trigger */
#define  ISP_TOP_RESET_TRIG_REQ			BIT(0)

#define ISP_TOP_RESET_STAT			0x0028 /* Soft reset status */
#define  ISP_TOP_RESET_STAT_DONE		BIT(0)
#define  ISP_TOP_RESET_STAT_CLEAR		BIT(2)

#define ISP_TOP_REG_2C				0x002c /* undocumented */
#define  ISP_TOP_REG_2C_DEFAULT			0x00000000 /* undocumented */

#define ISP_TOP_REG_30				0x0030 /* undocumented */
#define  ISP_TOP_REG_30_DEFAULT			0x000000f3 /* undocumented */

#define ISP_TOP_REG_33C				0x033c /* undocumented */
#define  ISP_TOP_REG_33C_DEFAULT		0x20230219 /* undocumented */

/* =========================================================================
 * 2. ISP Core Interrupt Controller (sd->base)
 * ========================================================================= */
#define ISP_INT_MASK				0x00b0 /* Core interrupt enable mask */
#define  ISP_INT_MASK_ALL			0xffffffff

#define ISP_INT_STATUS				0x00b4 /* Core interrupt status */
#define  ISP_INT_MSCA_CH0_DONE			BIT(0)
#define  ISP_INT_MSCA_CH1_DONE			BIT(1)
#define  ISP_INT_MSCA_CH2_DONE			BIT(2)
#define  ISP_INT_ERR_MASK			GENMASK(9, 3)
#define  ISP_INT_ERR_BIT3			BIT(3)
#define  ISP_INT_ERR_BIT4			BIT(4)
#define  ISP_INT_ERR_BIT5			BIT(5)
#define  ISP_INT_ERR_BIT6			BIT(6)
#define  ISP_INT_ERR_BIT7			BIT(7)
#define  ISP_INT_ERR_BIT8			BIT(8)
#define  ISP_INT_ERR_BIT9			BIT(9)
#define  ISP_INT_FRAME_EVENT			BIT(12)
#define  ISP_INT_IPC_FRAME_DONE			BIT(13)
#define  ISP_INT_ADR				BIT(18)
#define  ISP_INT_DEFOG				BIT(20)
#define  ISP_INT_AE				BIT(26)
#define  ISP_INT_HIST				BIT(27)
#define  ISP_INT_AWB				BIT(30)
#define  ISP_INT_AF				BIT(31)

#define  ISP_CORE_STATS_IRQS			(ISP_INT_AE | ISP_INT_HIST | ISP_INT_AWB)

#define ISP_INT_CLEAR				0x00b8 /* Core interrupt clear (W1C) */

#define ISP_INT1_MASK				0x00c0 /* Interrupt bank 1 mask */
#define  ISP_INT1_BIT15				BIT(15) /* pipeline done interrupt */

#define ISP_INT1_CLEAR				0x00c4 /* Interrupt bank 1 clear (W1C) */

#define ISP_INT1_STATUS				0x00c8 /* Interrupt bank 1 status */
#define  ISP_INT1_STATUS_BIT15			BIT(15) /* pipeline done flag */

/* =========================================================================
 * 3. ISP IPC / Run Control (sd->base)
 * ========================================================================= */
#define ISP_IPC_TRIGGER				0x0800 /* Frame processing trigger */
#define  ISP_IPC_TRIGGER_START			BIT(0)
#define  ISP_IPC_TRIGGER_STOP			0x00000000

#define ISP_IPC_MODE				0x0804 /* Ingest mode and sensor port select */
#define  ISP_IPC_MODE_PORT1			BIT(6)
#define  ISP_IPC_MODE_DEFAULT			0x0000001c /* undocumented */

#define ISP_IPC_INPUT_STAT			0x084c /* Input pipeline status */

/* =========================================================================
 * 4. ISP Digital Gain & Black Level Correction (GIB) (sd->base)
 * ========================================================================= */
#define ISP_DGAIN_GAIN01			0x1030 /* Channels 0 and 1 digital gain */
#define ISP_DGAIN_GAIN23			0x1034 /* Channels 2 and 3 digital gain */
#define ISP_DGAIN_LUT_PARAM0			0x1038 /* GIB interpolation config word 0 */
#define ISP_DGAIN_LUT_PARAM1			0x103c /* GIB interpolation config word 1 */
#define ISP_DGAIN_BLC0				0x1060 /* Black level correction channel 0 */
#define ISP_DGAIN_BLC1				0x1064 /* Black level correction channel 1 */
#define ISP_DGAIN_BLC2				0x1068 /* Black level correction channel 2 */
#define ISP_DGAIN_BLC3				0x106c /* Black level correction channel 3 */
#define ISP_DGAIN_TRIGGER			0x1070 /* Digital gain parameter commit */

#define  ISP_DGAIN_GAIN_ONE			0x04000400 /* 1.0x gain for channel pair (Q10) */
#define  ISP_DGAIN_LUT_PARAM0_DEFAULT		0x0fff0fff /* undocumented */
#define  ISP_DGAIN_LUT_PARAM1_DEFAULT		0x00005430 /* undocumented */
#define  ISP_DGAIN_BLC_ONE			0x00000103 /* undocumented */
#define  ISP_DGAIN_BLC_PAIR_ONE			0x01030103 /* undocumented */
#define  ISP_DGAIN_TRIGGER_COMMIT		0x1

#define ISP_DGAIN_DEIR_LUT0			0x80000 /* GIB DEIR table 0 */
#define ISP_DGAIN_DEIR_LUT1			0x80080 /* GIB DEIR table 1 */
#define ISP_DGAIN_DEIR_LUT2			0x80100 /* GIB DEIR table 2 */

/* =========================================================================
 * 5. ISP White Balance Gain (sd->base)
 * ========================================================================= */
#define ISP_WB_TRIGGER				0x1800 /* White balance latch trigger strobe */
#define  ISP_WB_TRIGGER_COMMIT			0x1
#define ISP_WB_GAIN_R				0x1804 /* Red channel gain */
#define ISP_WB_GAIN_B				0x1808 /* Blue channel gain */
#define ISP_WB_GAIN_GR				0x180c /* Green-red channel gain */
#define ISP_WB_GAIN_GB				0x1810 /* Green-blue channel gain */
#define  ISP_WB_GAIN_TAG			0x04000000 /* undocumented gain tag */

/* =========================================================================
 * 6. ISP Lens Shading Correction (LSC) (sd->base)
 * ========================================================================= */
#define ISP_LSC_GRID_SIZE			0x3800 /* Mesh grid dimensions (height:width) */
#define ISP_LSC_CONTROL				0x3804 /* LSC control */
#define ISP_LSC_LUT_BASE			0x28000 /* Mesh gain correction lookup table */
#define ISP_LSC_LUT_TRIGGER			0x2800c /* LSC LUT commit strobe */
#define  ISP_LSC_LUT_TRIGGER_COMMIT		0x0

/* =========================================================================
 * 7. ISP Adaptive Dynamic Range (ADR) (sd->base)
 * ========================================================================= */
#define ISP_ADR_GRID_STEP			0x4000 /* Grid block step (y_step:x_step) */
#define ISP_ADR_GRID_Y0				0x4010 /* Vertical grid row boundary 0 */
#define ISP_ADR_GRID_Y1				0x4014 /* Vertical grid row boundary 1 */
#define ISP_ADR_GRID_HEIGHT			0x4018 /* Frame total height for grid */
#define ISP_ADR_GRID_X0				0x401c /* Horizontal grid column boundary 0 */
#define ISP_ADR_GRID_X1				0x4020 /* Horizontal grid column boundary 1 */
#define ISP_ADR_GRID_X2				0x4024 /* Horizontal grid column boundary 2 */
#define ISP_ADR_GRID_WIDTH			0x4028 /* Frame total width for grid */
#define ISP_ADR_MARGIN_Y			0x4454 /* Vertical statistical margin */
#define ISP_ADR_MARGIN_X			0x4458 /* Horizontal statistical margin */
#define ISP_ADR_DMA_CTRL			0x4490 /* ADR statistics DMA control */
#define  ISP_ADR_DMA_ENABLE			0x3
#define ISP_ADR_DMA_ADDR_BASE			0x4494 /* ADR statistics DMA buffer banks */
#define ISP_ADR_DMA_ADDR(bank)			(0x4494 + ((bank) * 4))

/* =========================================================================
 * 8. ISP Demosaic (DMSC) (sd->base)
 * ========================================================================= */
#define ISP_DMSC_REG_BASE			0x4800 /* DMSC register block start */
#define ISP_DMSC_REG_04				0x4804 /* DMSC status / configuration register */
#define ISP_DMSC_REG_08				0x4808 /* undocumented */
#define  ISP_DMSC_REG_08_DEFAULT		0x00c80078 /* undocumented */
#define ISP_DMSC_REG_48				0x4848 /* undocumented */
#define  ISP_DMSC_REG_48_DEFAULT		0x00640078 /* undocumented */
#define ISP_DMSC_REG_60				0x4860 /* undocumented */
#define  ISP_DMSC_REG_60_DEFAULT		0x005a005a /* undocumented */
#define ISP_DMSC_TRIGGER			0x499c /* DMSC parameter refresh commit */
#define  ISP_DMSC_REFRESH_COMMIT		0x1

/* =========================================================================
 * 9. ISP Colour Correction Matrix (CCM) (sd->base)
 * ========================================================================= */
#define ISP_CCM_TRIGGER				0x5000 /* CCM coefficient latch strobe */
#define  ISP_CCM_TRIGGER_STROBE			0x1
#define ISP_CCM_COEFF_BASE			0x5004 /* CCM packed coefficient register base */
#define ISP_CCM_COEFF(n)			(0x5004 + ((n) * 4))

/* =========================================================================
 * 10. ISP Defog / Dehaze (sd->base)
 * ========================================================================= */
#define ISP_DEFOG_BOUND_V_BASE			0x5800 /* Defog vertical boundaries base */
#define ISP_DEFOG_BOUND_H_BASE			0x5820 /* Defog horizontal boundaries base */
#define ISP_DEFOG_CONTROL			0x5b00 /* Defog module control */
#define ISP_DEFOG_MIN_THRES			0x5b04 /* Defog minimum threshold */
#define ISP_DEFOG_MAX_THRES			0x5b0c /* Defog maximum threshold */
#define ISP_DEFOG_DMA_CTRL			0x5b80 /* Defog statistics DMA control */
#define  ISP_DEFOG_DMA_ENABLE			0x3
#define ISP_DEFOG_DMA_ADDR_BASE			0x5b84 /* Defog statistics DMA buffer banks */
#define ISP_DEFOG_DMA_ADDR(bank)		(0x5b84 + ((bank) * 4))

/* =========================================================================
 * 11. ISP Colour Space Conversion (CSC) (sd->base)
 * ========================================================================= */
#define ISP_CSC_CONTROL				0x6000 /* CSC function enable and routing */
#define  ISP_CSC_CTRL_BT601			0x0000001f
#define ISP_CSC_OFFSET0				0x6004 /* CSC input luma offset */
#define ISP_CSC_COEFF0				0x6010 /* CSC coefficient word 0 (Y coeffs) */
#define ISP_CSC_COEFF1				0x6014 /* CSC coefficient word 1 (Cb coeffs) */
#define ISP_CSC_COEFF2				0x6018 /* CSC coefficient word 2 (Cr coeffs) */
#define ISP_CSC_OFFSET1				0x6020 /* CSC chroma offset */
#define ISP_CSC_CLIP				0x6030 /* CSC output dynamic range clamping */

/* =========================================================================
 * 12. ISP Luma Sharpening / Peaking (YSP) (sd->base)
 * ========================================================================= */
#define ISP_YSP_REG_BASE			0x7000 /* YSP register block start */
#define ISP_YSP_REFRESH				0x7090 /* YSP parameter refresh commit */
#define  ISP_YSP_REFRESH_COMMIT			0x1

/* =========================================================================
 * 13. ISP Temporal Noise Reduction (MDNS) (sd->base)
 * ========================================================================= */
#define ISP_MDNS_REG_TRIGGER			0x7804 /* MDNS hardware run/stop trigger */
#define  ISP_MDNS_TRIGGER_START			0x111
#define  ISP_MDNS_TRIGGER_STOP			0x110
#define ISP_MDNS_DEBUG_CONTROL			0x7808 /* MDNS debug / bypass control */
#define ISP_MDNS_FUNC_CONTROL			0x7810 /* MDNS functional enables */
#define  ISP_MDNS_FUNC_CTRL_BYPASS		0x101
#define  ISP_MDNS_FUNC_CTRL_ENABLE		0xd1111111
#define  ISP_MDNS_FUNC_CTRL_MASK		0xd1111111U
#define ISP_MDNS_FUNC_GROUP			0x7814 /* MDNS parameter group control */
#define  ISP_MDNS_FUNC_GROUP_DEFAULT		0x00f01100
#define  ISP_MDNS_FUNC_GROUP_MASK		0x00f1310fU
#define ISP_MDNS_FUNC_REFRESH			0x7818 /* MDNS parameter shadow latch trigger */
#define  ISP_MDNS_REFRESH_VAL			0x21
#define ISP_MDNS_PLANE_ADDR_BASE		0x7820 /* MDNS history DMA buffer base */
#define ISP_MDNS_DMA_CTRL0			0x7838 /* MDNS DMA allocation control 0 */
#define  ISP_MDNS_DMA_CTRL0_DEFAULT		0x0
#define ISP_MDNS_DMA_CTRL1			0x783c /* MDNS DMA allocation control 1 */
#define  ISP_MDNS_DMA_CTRL1_DEFAULT		0x1

/* =========================================================================
 * 14. ISP Luma Noise Reduction (YDNS) (sd->base)
 * ========================================================================= */
#define ISP_YDNS_REG_BASE			0x7af0 /* YDNS register block start */

/* =========================================================================
 * 15. ISP Brightness, Contrast, Saturation, Hue (BCSH) (sd->base)
 * ========================================================================= */
#define ISP_BCSH_REG_00				0x8000 /* BCSH configuration register 0 */
#define ISP_BCSH_REG_18				0x8018 /* BCSH configuration register 6 */
#define ISP_BCSH_REG_38				0x8038 /* BCSH configuration register 14 */
#define ISP_BCSH_REG_54				0x8054 /* BCSH configuration register 21 */
#define ISP_BCSH_REG_64				0x8064 /* BCSH configuration register 25 */
#define ISP_BCSH_REG_70				0x8070 /* BCSH configuration register 28 */

/* =========================================================================
 * 16. ISP Spatial Noise Reduction (SDNS) (sd->base)
 * ========================================================================= */
#define ISP_SDNS_REG_BASE			0x8800 /* SDNS register block start */
#define ISP_SDNS_REFRESH			0x8b4c /* SDNS parameter refresh commit */
#define  ISP_SDNS_REFRESH_COMMIT		0x1

/* =========================================================================
 * 17. ISP Gamma Lookup Tables (sd->base)
 * ========================================================================= */
#define ISP_GAMMA_LUT_BASE(c)			(0x40000 + ((c) * 0x8000))
#define ISP_GAMMA_LUT_R				0x40000 /* Red channel gamma curve LUT */
#define ISP_GAMMA_LUT_G				0x48000 /* Green channel gamma curve LUT */
#define ISP_GAMMA_LUT_B				0x50000 /* Blue channel gamma curve LUT */

/* =========================================================================
 * 18. ISP Auto Exposure Statistics (AE) (sd->base)
 * ========================================================================= */
#define ISP_AE_REG_BASE				0xa000 /* AE register block start */
#define ISP_AE_CONTROL				0xa004 /* AE statistics enable and mode */
#define  ISP_AE_CTRL_DEFAULT			0xf001f001 /* undocumented */
#define ISP_AE_ZONE_W_BASE			0xa008 /* AE zone horizontal dimensions base */
#define ISP_AE_ZONE_H_BASE			0xa018 /* AE zone vertical dimensions base */
#define ISP_AE_LUM_TH_FREQ			0xa028 /* AE luminance threshold and sampling frequency */
#define  ISP_AE_LUM_TH_FREQ_DEFAULT		0xaa1e /* undocumented */
#define ISP_AE_DMA_ADDR_BASE			0xa02c /* AE zone statistics DMA buffer banks */
#define ISP_AE_DMA_ZONE_ADDR(bank)		(0xa02c + ((bank) * 4))
#define ISP_AE_DMA_HIST_ADDR_BASE		0xa03c /* AE histogram statistics DMA banks */
#define ISP_AE_DMA_HIST_ADDR(bank)		(0xa03c + ((bank) * 4))
#define ISP_AE_DMA_CTRL				0xa04c /* AE statistics DMA control */
#define  ISP_AE_DMA_CTRL_DEFAULT		0x10010000 /* undocumented */

/* =========================================================================
 * 19. ISP Auto White Balance Statistics (AWB) (sd->base)
 * ========================================================================= */
#define ISP_AWB_REG_BASE			0xb000 /* AWB register block start */
#define ISP_AWB_TRIGGER				0xb000 /* AWB statistics trigger strobe */
#define  ISP_AWB_TRIGGER_COMMIT			0x1
#define ISP_AWB_CONTROL				0xb004 /* AWB statistics enable and mode */
#define  ISP_AWB_CTRL_DEFAULT			0xf001f001 /* undocumented */
#define ISP_AWB_ZONE_W_BASE			0xb008 /* AWB zone horizontal dimensions base */
#define ISP_AWB_ZONE_H_BASE			0xb018 /* AWB zone vertical dimensions base */
#define ISP_AWB_COLOR_PARAM0			0xb028 /* AWB color parameter 0 */
#define  ISP_AWB_COLOR_PARAM0_DEFAULT		0x0122004b /* undocumented */
#define ISP_AWB_COLOR_PARAM1			0xb02c /* AWB color parameter 1 */
#define  ISP_AWB_COLOR_PARAM1_DEFAULT		0x00dc0023 /* undocumented */
#define ISP_AWB_COLOR_PARAM2			0xb030 /* AWB color parameter 2 */
#define  ISP_AWB_COLOR_PARAM2_DEFAULT		0x00c30100 /* undocumented */
#define ISP_AWB_COLOR_PARAM3			0xb034 /* AWB color parameter 3 */
#define  ISP_AWB_COLOR_PARAM3_DEFAULT		0x017c0100 /* undocumented */
#define ISP_AWB_LUM_TH_FREQ			0xb038 /* AWB luminance threshold and sampling */
#define  ISP_AWB_LUM_TH_FREQ_DEFAULT		0x0001f00a /* undocumented */
#define ISP_AWB_DMA_ADDR_BASE			0xb03c /* AWB statistics DMA buffer banks */
#define ISP_AWB_DMA_ADDR(bank)			(0xb03c + ((bank) * 4))
#define ISP_AWB_DMA_CTRL			0xb04c /* AWB statistics DMA control */
#define  ISP_AWB_DMA_CTRL_DEFAULT		0x0 /* undocumented */

/* =========================================================================
 * 20. ISP Auto Focus Statistics (AF) (sd->base)
 * ========================================================================= */
#define ISP_AF_TRIGGER				0xb800 /* AF statistics trigger strobe */
#define  ISP_AF_TRIGGER_COMMIT			0x1
#define ISP_AF_CONTROL				0xb804 /* AF statistics configuration */
#define  ISP_AF_CTRL_DEFAULT			0x80018003 /* undocumented */
#define ISP_AF_ZONE_W_BASE			0xb808 /* AF zone horizontal dimensions base */
#define ISP_AF_ZONE_H_BASE			0xb818 /* AF zone vertical dimensions base */
#define ISP_AF_THRES0				0xb828 /* AF metric threshold 0 */
#define  ISP_AF_THRES0_DEFAULT			0x0000c8f0 /* undocumented */
#define ISP_AF_THRES1				0xb82c /* AF metric threshold 1 */
#define ISP_AF_FILTER_BASE			0xb830 /* AF FIR/IIR filter coefficient base */
#define ISP_AF_DMA_ADDR_BASE			0xb8a8 /* AF statistics DMA buffer banks */
#define ISP_AF_DMA_ADDR(bank)			(0xb8a8 + ((bank) * 4))
#define ISP_AF_DMA_CTRL				0xb8b8 /* AF statistics DMA control */
#define  ISP_AF_DMA_CTRL_DEFAULT		0x000100dc /* undocumented */
#define ISP_AF_STATUS				0xb8bc /* AF statistics interrupt status */

/* =========================================================================
 * 21. MSCA Scaler and Output (sd->base)
 * ========================================================================= */
#define MSCA_SHD_TRIGGER			0xd010 /* Commit shadowed MSCA registers */
#define MSCA_CH_EN_MASK				0xd040 /* Channel enable bitmask */
#define  MSCA_CH_EN(ch)				BIT(ch)
#define MSCA_MASK_EN				0xd044 /* Mask enable register */
#define  MSCA_MASK_EN_CH(ch)			BIT(ch)
#define MSCA_OSD_EN				0xd048 /* OSD block enable register */
#define  MSCA_OSD_EN_BLOCK(n)			BIT(n)
#define MSCA_SCALE_CTL_MODE			0xd04c /* Scaler tap filter mode */
#define  MSCA_SCALE_CTL_MODE_V(ch)		BIT(ch)
#define  MSCA_SCALE_CTL_MODE_H(ch)		BIT(4 + (ch))
#define MSCA_REG_050				0xd050 /* Per-channel active mask, UV order, and mirror/flip */
#define  MSCA_REG_050_DEFAULT			GENMASK(2, 0) /* Per-channel active/clock enable (Ch0-Ch2) */
#define  MSCA_REG_050_UV_TYPE(ch)		BIT(12 + (ch))
#define  MSCA_REG_050_Y_MIRROR(ch)		BIT(16 + (ch))
#define  MSCA_REG_050_Y_FLIP(ch)		BIT(19 + (ch))
#define  MSCA_REG_050_UV_MIRROR(ch)		BIT(24 + (ch))
#define  MSCA_REG_050_UV_FLIP(ch)		BIT(27 + (ch))
#define  MSCA_REG_050_MIRROR(ch)		(MSCA_REG_050_Y_MIRROR(ch) | MSCA_REG_050_UV_MIRROR(ch))
#define  MSCA_REG_050_FLIP(ch)			(MSCA_REG_050_Y_FLIP(ch) | MSCA_REG_050_UV_FLIP(ch))
#define MSCA_REG_060				0xd060 /* IVDC inline link enable (shadowed; latches on frame commit) */
#define MSCA_IVDC_SYNC				MSCA_REG_060
#define MSCA_FCROP_POS				0xd090 /* Front crop origin */
#define MSCA_FCROP_SIZE				0xd094 /* Front crop dimensions */

#define MSCA_GEOM_HI				GENMASK(28, 16)
#define MSCA_GEOM_LO				GENMASK(11, 0)
#define MSCA_GEOM(hi, lo)			(FIELD_PREP(MSCA_GEOM_HI, hi) | FIELD_PREP(MSCA_GEOM_LO, lo))

#define MSCA_CH_BASE(ch)			(0xd100 + (ch) * 0x100)
#define MSCA_CH_OFF_FCROP_POS			0x00
#define MSCA_CH_OFF_SCALE_SIZE			0x04
#define MSCA_CH_OFF_CROP_POS			0x08
#define MSCA_CH_OFF_CROP_SIZE			0x0c
#define MSCA_CH_OFF_VSTEP			0x10
#define MSCA_CH_OFF_HSTEP			0x14
#define MSCA_CH_OFF_OUT				0x28
#define MSCA_CH_OFF_Y_FIFO			0x30
#define MSCA_CH_OFF_REG_34			0x34
#define MSCA_CH_OFF_Y_DONE			0x38
#define MSCA_CH_OFF_Y_DONE_EMPTY		0x3c
#define MSCA_CH_OFF_Y_STRIDE			0x40
#define MSCA_CH_OFF_Y_RESET			0x44
#define MSCA_CH_OFF_REG_4C			0x4c
#define MSCA_CH_OFF_UV_FIFO			0x50
#define MSCA_CH_OFF_REG_54			0x54
#define MSCA_CH_OFF_UV_DONE			0x58
#define MSCA_CH_OFF_UV_DONE_EMPTY		0x5c
#define MSCA_CH_OFF_UV_STRIDE			0x60
#define MSCA_CH_OFF_UV_RESET			0x64
#define MSCA_CH_OFF_REG_6C			0x6c
#define MSCA_CH_OFF_SCALE_PARA0			0x70
#define MSCA_CH_OFF_SCALE_PARA1			0x74

#define MSCA_CH_FCROP_POS(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_FCROP_POS)
#define MSCA_CH_SCALE_SIZE(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_SCALE_SIZE)
#define MSCA_CH_CROP_POS(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_CROP_POS)
#define MSCA_CH_CROP_SIZE(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_CROP_SIZE)
#define MSCA_CH_VSTEP(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_VSTEP)
#define MSCA_CH_HSTEP(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_HSTEP)
#define  MSCA_STEP				GENMASK(18, 0)
#define MSCA_CH_OUT(ch)				(MSCA_CH_BASE(ch) + MSCA_CH_OFF_OUT)
#define  MSCA_CH_OUT_FMT			GENMASK(1, 0)
#define  MSCA_CH_OUT_STROBE			BIT(6)
#define MSCA_Y_FIFO(ch)				(MSCA_CH_BASE(ch) + MSCA_CH_OFF_Y_FIFO)
#define MSCA_CH_REG_34(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_REG_34)
#define MSCA_Y_DONE(ch)				(MSCA_CH_BASE(ch) + MSCA_CH_OFF_Y_DONE)
#define MSCA_Y_DONE_EMPTY(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_Y_DONE_EMPTY)
#define MSCA_Y_STRIDE(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_Y_STRIDE)
#define MSCA_Y_RESET(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_Y_RESET)
#define MSCA_CH_REG_4C(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_REG_4C)
#define MSCA_UV_FIFO(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_UV_FIFO)
#define MSCA_CH_REG_54(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_REG_54)
#define MSCA_UV_DONE(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_UV_DONE)
#define MSCA_UV_DONE_EMPTY(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_UV_DONE_EMPTY)
#define MSCA_UV_STRIDE(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_UV_STRIDE)
#define MSCA_UV_RESET(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_UV_RESET)
#define MSCA_CH_REG_6C(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_REG_6C)
#define MSCA_CH_SCALE_PARA0(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_SCALE_PARA0)
#define MSCA_CH_SCALE_PARA1(ch)			(MSCA_CH_BASE(ch) + MSCA_CH_OFF_SCALE_PARA1)
#define  MSCA_DONE_ADDR				GENMASK(31, 3)
#define  MSCA_DONE_TAG				GENMASK(2, 0)
#define  MSCA_DONE_EMPTY			BIT(0)

#define MSCA_SCALE_PARA0_X0			GENMASK(9, 0)
#define MSCA_SCALE_PARA0_X1			GENMASK(25, 16)
#define MSCA_SCALE_PARA1_Y0			GENMASK(7, 0)
#define MSCA_SCALE_PARA1_Y1			GENMASK(15, 8)
#define MSCA_SCALE_PARA1_SLOPE			GENMASK(24, 16)
#define MSCA_SCALE_PARA1_SHIFT			GENMASK(31, 28)
#define MSCA_SCALE_PARA0_DEFAULT			(FIELD_PREP(MSCA_SCALE_PARA0_X0, 150) | \
						 FIELD_PREP(MSCA_SCALE_PARA0_X1, 450))
#define MSCA_SCALE_PARA1_DEFAULT			(FIELD_PREP(MSCA_SCALE_PARA1_Y0, 0x80) | \
						 FIELD_PREP(MSCA_SCALE_PARA1_Y1, 0x80) | \
						 FIELD_PREP(MSCA_SCALE_PARA1_SHIFT, 8))

#define MSCA_MASK(ch, n)			(0xd400 + (ch) * 0x40 + (n) * 0x10)
#define MSCA_MASK_POS(ch, n)			(MSCA_MASK(ch, n) + 0x00)
#define MSCA_MASK_SIZE(ch, n)			(MSCA_MASK(ch, n) + 0x04)
#define MSCA_MASK_COLOR(ch, n)			(MSCA_MASK(ch, n) + 0x08)
#define  MSCA_MASK_COLOR_Y			GENMASK(23, 16)
#define  MSCA_MASK_COLOR_U			GENMASK(15, 8)
#define  MSCA_MASK_COLOR_V			GENMASK(7, 0)

#define MSCA_LINE(n)				(0xd4c0 + (n) * 0x10)
#define MSCA_LINES				6
#define MSCA_LINE_START(n)			(MSCA_LINE(n) + 0x00)
#define MSCA_LINE_END(n)			(MSCA_LINE(n) + 0x04)
#define MSCA_LINE_COLOR(n)			(MSCA_LINE(n) + 0x08)
#define  MSCA_LINE_COLOR_YUV			GENMASK(23, 0)
#define  MSCA_LINE_COLOR_WIDTH			GENMASK(31, 24)
#define MSCA_LINE_CTRL(n)			(MSCA_LINE(n) + 0x0c)
#define  MSCA_LINE_CTRL_CH_MASK			GENMASK(2, 0)
#define  MSCA_LINE_CTRL_DDA_EN			BIT(5)
#define  MSCA_LINE_CTRL_ALPHA			GENMASK(15, 8)
#define  MSCA_LINE_CTRL_SLOPE			GENMASK(24, 16)
#define  MSCA_LINE_CTRL_OCTANT			GENMASK(27, 25)

#define MSCA_OSD_CTRL				0xd750
#define  MSCA_OSD_CTRL_TYPE			GENMASK(1, 0)
#define  MSCA_OSD_CTRL_TYPE_32BIT		0
#define  MSCA_OSD_CTRL_TYPE_1555		1
#define  MSCA_OSD_CTRL_DEFAULT			GENMASK(6, 4) /* undocumented */
#define  MSCA_OSD_CTRL_NO_ALPHA			BIT(8)
#define  MSCA_OSD_CTRL_ALPHA_MASK		GENMASK(15, 8)
#define MSCA_OSD_ORDER				0xd754
#define  MSCA_OSD_ORDER_BGRA			0
#define  MSCA_OSD_ORDER_ARGB			13
#define MSCA_OSD_BLOCK(n)			(0xd760 + (n) * 0x10)
#define MSCA_OSD_BLOCKS				8
#define MSCA_OSD_POS(n)				(MSCA_OSD_BLOCK(n) + 0x00)
#define MSCA_OSD_SIZE(n)			(MSCA_OSD_BLOCK(n) + 0x04)
#define MSCA_OSD_ADDR(n)			(MSCA_OSD_BLOCK(n) + 0x08)
#define MSCA_OSD_STRIDE(n)			(MSCA_OSD_BLOCK(n) + 0x0c)

#define MSCA_SHD_CTRL				0xe000
#define  MSCA_SHD_CTRL_DEFAULT_BEGIN		0x00000101 /* undocumented */
#define  MSCA_SHD_CTRL_DEFAULT_END		0x01010102 /* undocumented */
#define MSCA_SHD_FIFO				0xe004
#define MSCA_SHD_STAT				0xe00c
#define  MSCA_SHD_STAT_BUSY			GENMASK(31, 1)
#define MSCA_SHD_DMA				0xe100
#define  MSCA_SHD_DMA_START			BIT(0)
#define  MSCA_SHD_DMA_BUSY			BIT(1)
#define  MSCA_SHD_DMA_COUNT			GENMASK(19, 4)
#define MSCA_SHD_VCOEF(ch)			(0x800 + (ch) * 0x60)
#define MSCA_SHD_HCOEF(ch)			(0x830 + (ch) * 0x60)
#define  MSCA_COEF_LO				GENMASK(12, 0)
#define  MSCA_COEF_HI				GENMASK(28, 16)

/* =========================================================================
 * 22. IVDC (ivdc_base)
 * ========================================================================= */
#define IVDC_RESET				0x0000 /* Software reset */
#define IVDC_ISP_CTRL				0x0004 /* ISP interface control */
#define IVDC_VPU_CTRL				0x0008 /* VPU interface control */
#define IVDC_ISP_SIZE				0x0010 /* ISP input resolution */
#define IVDC_ISP_ALN				0x0014 /* ISP alignment configuration */
#define IVDC_VPU0_SIZE				0x0018 /* VPU channel 0 size */
#define IVDC_VPU1_SIZE				0x001c /* VPU channel 1 size */
#define IVDC_STRIDE				0x0020 /* Line stride */
#define IVDC_VPU_FRAME_ID			0x0024 /* VPU frame identifier */
#define IVDC_RAM_MAX_LINE			0x0028 /* RAM maximum line threshold */
#define IVDC_RAM_BASE_ADDR			0x002c /* RAM base buffer address */
#define IVDC_DDR_MAX_LINE			0x0030 /* DDR maximum line threshold */
#define IVDC_DDR_Y_BASE_ADDR			0x0034 /* DDR Y component base address */
#define IVDC_DDR_UV_BASE_ADDR			0x0038 /* DDR UV component base address */
#define IVDC_FIFO_BASE_ADDR			0x003c /* FIFO base buffer address */
#define IVDC_INTC_ENABLE			0x0040 /* Interrupt controller enable */
#define IVDC_INTC_STATE				0x0044 /* Interrupt controller status */
#define IVDC_DDR_HALF_FULL_THRD			0x0048 /* DDR half-full threshold */
#define IVDC_ISP_HALF_DONE_THRD			0x004c /* ISP half-done threshold */
#define IVDC_VPU_HALF_DONE_THRD			0x0050 /* VPU half-done threshold */
#define IVDC_TOP_INFO1				0x005c /* Top stream trigger control */
#define IVDC_TOP_INFO				0x0060 /* Top status / FIFO state */
#define IVDC_ISP_INFO				0x0064 /* ISP pipeline status */
#define IVDC_VPU_INFO0				0x0068 /* VPU channel 0 status */
#define IVDC_VPU_INFO1				0x006c /* VPU channel 1 status */
#define IVDC_VPU0_START				0x0070 /* VPU channel 0 DMA start */
#define IVDC_VPU0_CLR_FIFO			0x0074 /* VPU channel 0 FIFO clear */
#define IVDC_VPU0_RUN_FIFO			0x0078 /* VPU channel 0 FIFO run */
#define IVDC_VPU0_FIFO_STATE			0x0080 /* VPU channel 0 FIFO status */
#define IVDC_VPU1_START				0x0088 /* VPU channel 1 DMA start */
#define IVDC_VPU1_CLR_FIFO			0x008c /* VPU channel 1 FIFO clear */
#define IVDC_VPU1_RUN_FIFO			0x0090 /* VPU channel 1 FIFO run */
#define IVDC_VPU1_FIFO_STATE			0x0098 /* VPU channel 1 FIFO status */
#define IVDC_ISP_STOP_THRD			0x00a4 /* ISP stop threshold */
#define IVDC_ISP_DISCARD_THRD			0x00a8 /* ISP discard threshold */
#define IVDC_IDMA_ENABLE			0x00b0 /* Internal DMA enable */
#define IVDC_IDMA_Y_ADDR			0x00b4 /* Internal DMA Y plane address */
#define IVDC_IDMA_UV_ADDR			0x00b8 /* Internal DMA UV plane address */
#define IVDC_IDMA_STRIDE			0x00bc /* Internal DMA line stride */
#define IVDC_VPU0_END_CFG0			0x00c0 /* VPU channel 0 end config 0 */
#define IVDC_VPU1_END_CFG0			0x00c4 /* VPU channel 1 end config 0 */
#define IVDC_VPU0_END_CFG1			0x00c8 /* VPU channel 0 end config 1 */
#define IVDC_VPU1_END_CFG1			0x00cc /* VPU channel 1 end config 1 */
#define IVDC_TIMEOUT_ENABLE			0x00d0 /* Timeout monitor enable */
#define IVDC_TIMEOUT_WAIT_THRD			0x00d4 /* Timeout wait threshold */
#define IVDC_TIMEOUT_WORK_THRD			0x00d8 /* Timeout work threshold */
#define IVDC_MAX_DDR_LINE			0x00ec /* DDR maximum line reached status */
#define  IVDC_MAX_DDR_LINE_Y_MASK		GENMASK(14, 0)
#define  IVDC_MAX_DDR_LINE_UV_MASK		GENMASK(30, 16)

/* =========================================================================
 * 23. VIC (Video Ingest Controller) (vic_base)
 * ========================================================================= */
#define VIC_CONTROL_REG				0x0000 /* VIC capture control */
#define  VIC_CTRL_ENABLE			BIT(0)
#define  VIC_CTRL_RESET_BIT1			BIT(1)
#define  VIC_CTRL_RESET_BIT2			BIT(2)

#define VIC_IMAGE_SIZE				0x0004 /* Ingest image resolution */
#define  VIC_IMAGE_WIDTH_MASK			GENMASK(31, 16)
#define  VIC_IMAGE_HEIGHT_MASK			GENMASK(15, 0)

#define VIC_ENABLE_REG				0x0008 /* Sub-module enable flags */
#define  VIC_ENABLE_DEFAULT			0x00000004 /* undocumented */

#define VIC_CONF_REG				0x000c /* Ingest configuration mode */
#define  VIC_CONF_DEFAULT			0x00000002 /* undocumented */

#define VIC_INPUT_REG				0x0014 /* Ingest input interface selection */
#define  VIC_INPUT_DEFAULT			0x00000001 /* undocumented */

#define VIC_LINE_STRIDE				0x0100 /* Ingest line stride */
#define VIC_DVP_TIMING0				0x0104 /* DVP interface timing parameter 0 */
#define VIC_DVP_TIMING1				0x0108 /* DVP interface timing parameter 1 */
#define VIC_CSI_CONF				0x010c /* CSI interface configuration */
#define  VIC_CSI_CONF_DEFAULT			(0x2b << 12) /* undocumented */
#define VIC_CSI_SIZE				0x0110 /* CSI virtual channel 0 size */
#define VIC_CSI_SIZE1				0x0114 /* CSI virtual channel 1 size */
#define VIC_CSI_SIZE2				0x0118 /* CSI virtual channel 2 size */
#define VIC_CSI_SIZE3				0x011c /* CSI virtual channel 3 size */
#define VIC_FRM_CTRL				0x01a0 /* Frame control configuration */
#define VIC_FRM_CYCLE				0x01a4 /* Frame blanking cycle timing */
#define  VIC_FRM_CYCLE_DEFAULT			0x000a000a /* undocumented */
#define VIC_FIFO_TH_0				0x01a8 /* FIFO threshold 0 */
#define  VIC_FIFO_TH_0_DEFAULT			0x4440 /* undocumented */
#define VIC_FIFO_TH_1				0x01ac /* FIFO threshold 1 */
#define  VIC_FIFO_TH_1_DEFAULT			0x4440 /* undocumented */
#define VIC_FIFO_TH_2				0x01b0 /* FIFO threshold 2 */
#define  VIC_FIFO_TH_2_DEFAULT			0x10 /* undocumented */

#define VIC_STATUS_REG_0			0x01e0 /* VIC interrupt status bank 0 */
#define  VIC_INT0_FRAME_DONE			BIT(0)
#define  VIC_INT0_FRAME_START			BIT(4)
#define  VIC_INT0_FRAME_ASFIFO_OVF		BIT(9)
#define  VIC_INT0_HOR_ERR_CH0			BIT(10)
#define  VIC_INT0_HOR_ERR_CH1			BIT(11)
#define  VIC_INT0_HOR_ERR_CH2			BIT(12)
#define  VIC_INT0_HOR_ERR_CH3			BIT(13)
#define  VIC_INT0_VER_ERR_CH0			BIT(14)
#define  VIC_INT0_VER_ERR_CH1			BIT(15)
#define  VIC_INT0_VER_ERR_CH2			BIT(16)
#define  VIC_INT0_VER_ERR_CH3			BIT(17)
#define  VIC_INT0_HVF_ERR			BIT(18)
#define  VIC_INT0_DVP_HCOMP_ERR			BIT(19)
#define  VIC_INT0_DMA_SYFIFO_OVF		BIT(20)
#define  VIC_INT0_CONTROL_LIMIT_ERR		BIT(21)
#define  VIC_INT0_IMAGE_SYFIFO_OVF		BIT(22)
#define  VIC_INT0_MIPI_FID_ASFIFO_OVF		BIT(23)
#define  VIC_INT0_MIPI_CH0_HCOMP_ERR		BIT(24)
#define  VIC_INT0_MIPI_CH1_HCOMP_ERR		BIT(25)
#define  VIC_INT0_MIPI_CH2_HCOMP_ERR		BIT(26)
#define  VIC_INT0_MIPI_CH3_HCOMP_ERR		BIT(27)
#define  VIC_INT0_MIPI_CH0_VCOMP_ERR		BIT(28)
#define  VIC_INT0_MIPI_CH1_VCOMP_ERR		BIT(29)
#define  VIC_INT0_MIPI_CH2_VCOMP_ERR		BIT(30)
#define  VIC_INT0_MIPI_CH3_VCOMP_ERR		BIT(31)

#define VIC_STATUS_REG_1			0x01e4 /* VIC interrupt status bank 1 */
#define  VIC_INT1_DMA_CH0_FRAME_DONE		BIT(0)
#define  VIC_INT1_DMA_CH1_FRAME_DONE		BIT(1)
#define  VIC_INT1_DMA_ARB_TRANS_DONE_OVF	BIT(2)
#define  VIC_INT1_DMA_CHID_OVF			BIT(3)

#define VIC_MASK_REG_0				0x01e8 /* VIC interrupt mask bank 0 */
#define VIC_MASK_REG_1				0x01ec /* VIC interrupt mask bank 1 */
#define VIC_CLEAR_REG_0				0x01f0 /* VIC interrupt clear bank 0 (W1C) */
#define VIC_CLEAR_REG_1				0x01f4 /* VIC interrupt clear bank 1 (W1C) */

#define VIC_AUTO_SWITCH_CTRL			0x0200 /* Sensor ping-pong switch control */
#define  VIC_AUTO_SWITCH_EN			BIT(31)
#define  VIC_AUTO_SWITCH_ACTIVE			BIT(16)
#define  VIC_AUTO_SWITCH_PORT0			BIT(0)

#define VIC_PORT_SEL				0x0204 /* Active sensor input port select */
#define  VIC_PORT_SEL_PORT0			0x00000000
#define  VIC_PORT_SEL_PORT1			0xffffffff

#define VIC_AUTO_SWITCH_TIME			0x0208 /* Auto switch cycle duration */
#define VIC_PORT_STATUS				0x020c /* Active sensor port status */

#define VIC_DMA_CONTROL				0x0300 /* VIC DMA control */
#define  VIC_DMA_CTRL_ENABLE			BIT(31)
#define  VIC_DMA_CTRL_AUTO			BIT(16)
#define  VIC_DMA_CTRL_FMT_RAW10			BIT(5)

#define VIC_DMA_RESOLUTION			0x0304 /* VIC DMA buffer dimensions */
#define  VIC_DMA_RES_WIDTH_MASK			GENMASK(31, 16)
#define  VIC_DMA_RES_HEIGHT_MASK		GENMASK(15, 0)

#define VIC_DMA_RESET				0x0308 /* VIC DMA reset trigger */
#define  VIC_DMA_RESET_TRIG			BIT(0)

#define VIC_DMA_STRIDE				0x0310 /* VIC DMA Y/ch0 plane line stride */
#define VIC_DMA_STRIDE1				0x0314 /* VIC DMA UV/ch1 plane line stride */
#define VIC_DMA_BANK(n)				(0x0318 + 4 * (n))
#define VIC_DMA_CURRENT				0x0380 /* Active DMA buffer address pointer */

/* =========================================================================
 * 24. CSI-2 Host Controller (sd->base)
 * ========================================================================= */
#define CSI2_VERSION				0x0000 /* Controller version */
#define CSI2_N_LANES				0x0004 /* Number of active data lanes */
#define  CSI2_N_LANES_MASK			GENMASK(1, 0)

#define CSI2_PHY_SHUTDOWNZ			0x0008 /* D-PHY shutdown control (active low) */
#define  CSI2_PHY_SHUTDOWNZ_BIT			BIT(0)

#define CSI2_DPHY_RSTZ				0x000c /* D-PHY reset control (active low) */
#define  CSI2_DPHY_RSTZ_BIT			BIT(0)

#define CSI2_RESETN				0x0010 /* CSI-2 core controller reset (active low) */
#define  CSI2_RESETN_BIT			BIT(0)

#define CSI2_PHY_STATE				0x0014 /* D-PHY line state status */
#define  CSI2_PHY_STOPSTATEDATA(n)		BIT(4 + (n))
#define  CSI2_PHY_RXCLKACTIVEHS			BIT(8)
#define  CSI2_PHY_RXULPSCLKNOT			BIT(9)
#define  CSI2_PHY_STOPSTATECLK			BIT(10)

#define CSI2_DATA_IDS_1				0x0018 /* Data ID configuration 1 */
#define CSI2_DATA_IDS_2				0x001c /* Data ID configuration 2 */
#define CSI2_ERR1				0x0020 /* Packet / ECC error status register 1 */
#define CSI2_ERR2				0x0024 /* CRC / sync error status register 2 */
#define CSI2_MSK1				0x0028 /* Error mask register 1 */
#define CSI2_MSK2				0x002c /* Error mask register 2 */
#define CSI2_PHY_TST_CTRL0			0x0030 /* D-PHY test interface control 0 */
#define  CSI2_PHY_TESTCLR			BIT(0)
#define  CSI2_PHY_TESTCLK			BIT(1)

#define CSI2_PHY_TST_CTRL1			0x0034 /* D-PHY test interface control 1 */
#define  CSI2_PHY_TESTEN			BIT(16)

#define CSI2_TIMEOUT_REG			0x0100 /* Reception timeout threshold */
#define  CSI2_TIMEOUT_DEFAULT			0x00000003 /* undocumented */

/* =========================================================================
 * 25. Ingenic T23 D-PHY Receiver (csi->phy)
 * ========================================================================= */
#define DPHY_PWR_REG				0x0000 /* D-PHY power control */
#define  DPHY_PWR_ON				0x0000007d /* undocumented */

#define DPHY_RST_REG				0x0128 /* D-PHY reset control */
#define  DPHY_RST_RELEASE			0x0000003f /* undocumented */

#define DPHY_CLK_SETTLE				0x0160 /* Clock lane settle time */
#define DPHY_DATA0_SETTLE			0x01e0 /* Data lane 0 settle time */
#define DPHY_DATA1_SETTLE			0x0260 /* Data lane 1 settle time */
#define DPHY_LANE_SETTLE(lane)			(0x0160 + (lane) * 0x80)
#define  DPHY_SETTLE_MASK			GENMASK(3, 0)

#endif /* __TX_ISP_REGS_H__ */
