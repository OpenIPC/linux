/*
 * Copyright (C) 2007-2008 Texas Instruments Inc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _DM355_IPIPE_HW_H
#define _DM355_IPIPE_HW_H

#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/io.h>
#include <mach/hardware.h>

#define IPIPE_IOBASE_VADDR 		IO_ADDRESS(0x01C71000)
#define IPIPEIF_IOBASE_VADDR		IO_ADDRESS(0x01C70100)

static inline u32 regr_ip(u32 offset)
{
    return __raw_readl(IPIPE_IOBASE_VADDR + offset);
}

static inline u32 regw_ip(u32 val, u32 offset)
{
    __raw_writel(val, IPIPE_IOBASE_VADDR + offset);
    return val;
}
static inline u32 regr_if(u32 offset)
{
    return __raw_readl(IPIPEIF_IOBASE_VADDR + offset);
}

static inline u32 regw_if(u32 val, u32 offset)
{
    __raw_writel(val, IPIPEIF_IOBASE_VADDR + offset);
    return val;
}
/*
static inline u32 regr_vpss(u32 offset)
{
    return davinci_readl(VPSS_BL_BASE + offset);
}

static inline u32 regw_vpss(u32 val, u32 offset)
{
    davinci_writel(val, VPSS_BL_BASE + offset);
    return val;
}
*/

#define SET_LOW_ADD     0x0000FFFF
#define SET_HIGH_ADD    0xFFFF0000


/* macro for bit set and clear */
#define SETBIT(reg, bit)   (reg = ((reg) | ((0x00000001)<<(bit))))
#define RESETBIT(reg, bit) (reg = ((reg) & (~(0x00000001<<(bit)))))
/* -to set vpss reg for ipipe- */
#define VPSS_PCR		0x0804
#define VPSS_MEMCTL		0x0818
#define VPSS_CLK   		0x0004



#define SDR_ENABLE		2/*int number for IRQ_EN*/
/* Internal RAM table addresses for defect correction */
#define DEF_COR_START_ADDR	0x0000
#define DEF_COR_END_ADDR	0x07FF
#define DEF_COR_SIZE		1024
#define MAX_SIZE_DFC			1024
#define MAX_SIZE_EEC			1024
#define MAX_SIZE_GAMMA			512

/* Internal RAM table addresses for gamma correction */
#define GAMMA_START_ADDR      0x0000
#define GAMMA_END_ADDR        0x03FF

/*Internal RAM table addresses for edge enhancement correction*/
#define EDGE_ENHANCE_START_ADDR		0x0200
#define EDGE_ENHANCE_END_ADDR		0x01FF

/* IPIPE Register Offsets from the base address */

#define IPIPE_EN 		0x0000
#define IPIPE_MODE 		0x0004
#define IPIPE_DPATHS 		0x0008
#define IPIPE_COLPAT		0x000C
#define IPIPE_VST		0x0010
#define IPIPE_VSZ 		0x0014
#define IPIPE_HST		0x0018
#define IPIPE_HSZ 		0x001C
/*gated clock enable*/
#define GCL_ARM			0x0024
#define GCL_CCD 		0x0028
#define GCL_SDR			0x002C
/* Internal Memory Access */
#define RAM_MODE		0x0030
#define RAM_ADR			0x0034
#define RAM_WDT			0x0038
#define RAM_RDT			0x003C
/* Interrupts */
#define IRQ_EN			0x0040
#define IRQ_RZA			0x0044
#define IRQ_RZB			0x0048
/* Defect Correction */
#define DFC_EN			0x004C
#define DFC_SEL			0x0050
#define DFC_ADR			0x0054
#define DFC_SIZE		0x0058
/* Programmable Noise Filter */
#define D2F_EN			0x005C
#define D2F_CFG			0x0060
#define D2F_THR			0x0064
#define D2F_STR			0x00E4
/* PreFilter */
#define PRE_EN			0x0164
#define PRE_TYP			0x0168
#define PRE_SHF			0x016C
#define PRE_GAIN		0x0170
#define PRE_THR_G		0x0174
#define PRE_THR_B		0x0178
#define PRE_THR_1		0x017C
/* White Balance */
#define WB2_DGN			0x0180
#define WB2_WG_R		0x0184
#define WB2_WG_GR		0x0188
#define WB2_WG_GB		0x018C
#define WB2_WG_B		0x0190

/* RGB to RGB conversion (include GAMMA correction) */
#define RGB_MUL_RR		0x01F4

/* Defect Correction */
#define DFC_EN                  0x004C
#define DFC_SEL                 0x0050
#define DFC_ADR                 0x0054
#define DFC_SIZ                 0x0058
/* Programmable Noise Filter */
#define D2F_EN                  0x005C
#define D2F_CFG                 0x0060
#define DFC_THR                 0x0064
#define D2F_STR                 0x00E4
/* PreFilter */
#define PRE_EN                  0x0164
#define PRE_TYP                 0x0168
#define PRE_SHF                 0x016C
#define PRE_GAIN                0x0170
#define PRE_THR_G               0x0174
#define PRE_THR_B               0x0178
#define PRE_THR_1               0x017C
/* White Balance */
#define WB2_DGN                 0x0180
#define WB2_WG_R                0x0184
#define WB2_WG_GR               0x0188
#define WB2_WG_GB               0x018C
#define WB2_WG_B                0x0190

/* RGB to RGB conversion (include GAMMA correction) */
#define RGB_MUL_RR              0x01F4
#define RGB_MUL_GR              0x01F8
#define RGB_MUL_BR              0x01FC
#define RGB_MUL_RG		0x0200
#define RGB_MUL_GG		0x0204
#define RGB_MUL_BG		0x0208
#define RGB_MUL_RB		0x020C

#define RGB_MUL_GB		0x0210
#define RGB_MUL_BB		0x0214
#define RGB_MUL_OR		0x0218
#define RGB_MUL_OG		0x021C
#define RGB_MUL_OB		0x0220
#define GMM_CFG			0x0224

/* RGB to YUV(YCbCr) conversion */
#define YUV_ADJ 		0x0228
#define YUV_MUL_RY 		0x022C
#define YUV_MUL_GY 		0x0230
#define YUV_MUL_BY 		0x0234
#define YUV_MUL_RCB 		0x0238
#define YUV_MUL_GCB 		0x023C
#define YUV_MUL_BCB 		0x0240
#define YUV_MUL_RCR 		0x0244
#define YUV_MUL_GCR 		0x0248
#define YUV_MUL_BCR 		0x024C
#define YUV_OFT_Y 		0x0250
#define YUV_OFT_CB 		0x0254
#define YUV_OFT_CR 		0x0258
#define YUV_Y_MIN 		0x025C
#define YUV_Y_MAX 		0x0260
#define YUV_C_MIN 		0x0264
#define YUV_C_MAX 		0x0268
#define YUV_PHS 		0x026C

/* Edge Enhancer */
#define YEE_EN			0x0270
#define YEE_EMF			0x0274
#define YEE_SHF			0x0278
#define YEE_MUL_00		0x027C
#define YEE_MUL_01		0x0280
#define YEE_MUL_02		0x0284
#define YEE_MUL_10		0x0288
#define YEE_MUL_11		0x028C
#define YEE_MUL_12		0x0290
#define YEE_MUL_20		0x0294
#define YEE_MUL_21		0x0298
#define YEE_MUL_22		0x029C

/* False Color Suppression */
#define FCS_EN			0x02A0
#define FCS_TYP			0x02A4
#define FCS_SHF_Y		0x02A8
#define FCS_SHF_C		0x02AC
#define FCS_THR 		0x02B0
#define FCS_SGN			0x02B4
#define FCS_LTH			0x02B8

/* Resizer */
#define RSZ_SEQ                 0x02BC
#define RSZ_AAL                 0x02C0

/* Resizer Rescale Parameters */
#define RSZ_EN_0	        0x02C4
#define RSZ_EN_1             	0x0334
/* offset of the registers to be added with base register of either RSZ0 or
 * RSZ1
 */
#define RSZ_MODE	        0x4
#define RSZ_I_VST               0x8
#define RSZ_I_VSZ               0xC
#define RSZ_I_HST               0x10
#define RSZ_O_VSZ               0x14
#define RSZ_O_HST               0x18
#define RSZ_O_HSZ		0x1C
#define RSZ_V_PHS               0x20
#define RSZ_V_PHS_O	        0x24
#define RSZ_V_DIF               0x28
#define RSZ_V_SIZ_O             0x2C
#define RSZ_H_PHS               0x30
#define RSZ_H_DIF               0x34
#define RSZ_H_TYP               0x38
#define RSZ_H_LSE               0x3C
#define RSZ_H_LPF             	0x40

/* Resizer RGB Conversion Parameters */
#define RSZ_RGB_EN              0x44
#define RSZ_RGB_TYP             0x48
#define RSZ_RGB_BLD             0x4C

/* Resizer External Memory Parameters */
#define RSZ_SDR_BAD_H           0x50
#define RSZ_SDR_BAD_L           0x54
#define RSZ_SDR_SAD_H           0x58
#define RSZ_SDR_SAD_L           0x5C
#define RSZ_SDR_OFT             0x60
#define RSZ_SDR_PTR_S           0x64
#define RSZ_SDR_PTR_E           0x68
#define RSZ_SDR_PTR_O           0x6C

/* Macro for resizer */
#define IPIPE_RESIZER_0(i)  (IPIPE_IOBASE_VADDR + RSZ_EN_0 + i)
#define IPIPE_RESIZER_1(i)  (IPIPE_IOBASE_VADDR + RSZ_EN_1 + i)

/* Masking fields */
#define IPIPE_MODE_WRT 		(1 << 1)
#define IPIPE_DPATHS_FMT	(3 << 0)
#define IPIPE_DPATHS_BYPASS	(1 << 2)
#define IPIPE_COLPAT_ELEP	(3 << 0)
#define IPIPE_COLPAT_ELOP	(3 << 2)
#define IPIPE_COLPAT_OLEP	(3 << 4)
#define IPIPE_COLPAT_OLOP	(3 << 6)
#define IPIPE_D2F_CFG_SPR	(3 << 0)
#define IPIPE_D2F_CFG_SHF       (3 << 2)
#define IPIPE_D2F_CFG_TYP       (1 << 4)
#define IPIPE_PRE_TYP_SEL1	(1 << 1)
#define IPIPE_PRE_TYP_EN0       (1 << 2)
#define IPIPE_PRE_TYP_EN1       (1 << 3)
#define IPIPE_GMM_CFG_BYPG      (1 << 1)
#define IPIPE_GMM_CFG_BYPB      (1 << 2)
#define IPIPE_GMM_CFG_TBL       (1 << 4)
#define IPIPE_GMM_CFG_SIZ       (3 << 5)
#define IPIPE_YUV_ADJ_CTR	(0Xff << 0)
#define IPIPE_YUV_ADJ_BRT       (0Xff << 8)
#define IPIPE_YUV_PHS_LPF	(1 << 1)
#define IPIPE_RSZ_SEQ_TMM	(1 << 1)
#define IPIPE_RSZ_SEQ_HRV       (1 << 2)
#define IPIPE_RSZ_SEQ_VRV       (1 << 3)
#define IPIPE_RSZ_SEQ_CRV       (1 << 3)
#define IPIPE_RSZ_RGB_TYP_MSK0  (1 << 1)
#define IPIPE_RSZ_RGB_TYP_MSK1  (1 << 2)

/* BIT FIELDS */

#define IPIPE_DPATHS_BYPASS_SHIFT	 2
#define IPIPE_COLPAT_ELOP_SHIFT		 2
#define IPIPE_COLPAT_OLEP_SHIFT		 4
#define IPIPE_COLPAT_OLOP_SHIFT		 6
#define IPIPE_D2F_CFG_SHF_SHIFT          2
#define IPIPE_D2F_CFG_TYP_SHIFT          4
#define IPIPE_PRE_TYP_SEL1_SHIFT         1
#define IPIPE_PRE_TYP_EN0_SHIFT          2
#define IPIPE_PRE_TYP_EN1_SHIFT          3
#define IPIPE_GMM_CFG_BYPG_SHIFT         1
#define IPIPE_GMM_CFG_BYPB_SHIFT         2
#define IPIPE_GMM_CFG_TBL_SHIFT          4
#define IPIPE_GMM_CFG_SIZ_SHIFT          5
#define IPIPE_YUV_ADJ_BRT_SHIFT          8
#define IPIPE_YUV_PHS_LPF_SHIFT          1
#define IPIPE_RSZ_SEQ_TMM_SHIFT          1
#define IPIPE_RSZ_SEQ_HRV_SHIFT          2
#define IPIPE_RSZ_SEQ_VRV_SHIFT          3
#define IPIPE_RSZ_SEQ_CRV_SHIFT          3
#define IPIPE_RSZ_RGB_TYP_MSK0_SHIFT     1
#define IPIPE_RSZ_RGB_TYP_MSK1_SHIFT     2

/* IPIPEIF Register Offsets from the base address */
#define IPIPEIF_ENABLE 	0x00
#define IPIPEIF_GFG 	0x04
#define IPIPEIF_PPLN	0x08
#define IPIPEIF_LPFR	0x0C
#define IPIPEIF_HNUM 	0x10
#define IPIPEIF_VNUM    0x14
#define IPIPEIF_ADDRU   0x18
#define IPIPEIF_ADDRL   0x1C
#define IPIPEIF_ADOFS   0x20
#define IPIPEIF_RSZ   	0x24
#define IPIPEIF_GAIN   0x28

#endif /* End of #ifdef __KERNEL__ */
#endif  /* End of #ifdef _DM355_IPIPE_HW_H */
