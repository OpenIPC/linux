/****************************************************************
*****************************************************************/
#ifndef __HELIX_H__
#define __HELIX_H__

/****************************************************************
  VPU register map
*****************************************************************/
#define JZM_V2_TLB

#ifdef JZM_HUNT_SIM
#include "hunt.h"
#else
#ifndef __place_k0_data__
#define __place_k0_data__
#endif
#ifndef __place_k0_text__
#define __place_k0_text__
#endif
#endif

#define JPEG_BASE       0x13280000

/* #define	HID_SCH			 0x0 */
/* #define	HID_JRFD		 0x1 */
/* #define	HID_EMC			 0x3 */
/* #define	HID_EFE			 0x4 */
/* #define	HID_MCE			 0x5 */
/* #define HID_ODMA         0x6 */
/* #define	HID_DBLK		 0x7 */
/* #define	HID_VMAU		 0x8 */
/* #define	HID_SDE			 0x9 */
/* #define	HID_AUX			 0xA */
/* #define	HID_TCSM		 0xB */
/* #define	HID_IFA			 0xB //fixme */
/* #define	HID_JPGC		 0xE */
/* #define HID_SRAM         0xF */

#define	HID_SCH			 0x0
#define	HID_VDMA		 0x1
#define	HID_EFE			 0x2
#define	HID_JPGC		 0x3

//0x1328000 ==> 7->F
#define	HID_AUX			 0x7
#define	HID_TCSM		 0x7
#define	HID_SDE			 0x7
#define	HID_DBLK		 0x7
#define	HID_EMC			 0x7

/* #define	HID_JRFD		 0x7 */
/* #define	HID_MCE			 0x7 */
/* #define HID_ODMA         0x7 */
/* #define	HID_IFA			 0x7 //fixme */
/* #define HID_SRAM         0x7 */

#define VPU_MAX_MB_WIDTH     256

#define MSCOPE_START(mbnum)  write_reg(JPEG_BASE+0x24, mbnum)
#define MSCOPE_STOP()        write_reg(JPEG_BASE+0x28, 0)

/********************************************
  SCH (Scheduler)
*********************************************/
#define TCSM_FLUSH           (HID_TCSM<<16) //0xc0000 t41 change
#define REG_SCH_GLBC         0x00000
#define SCH_GLBC_SLDE        (0x1<<31)
#ifdef JZM_V2_TLB
#define SCH_TLBE_JPGC       (0x1<<26)
#define SCH_TLBE_DBLK       (0x1<<25)
#define SCH_TLBE_SDE        (0x1<<24)
#define SCH_TLBE_EFE        (0x1<<23)
#define SCH_TLBE_VDMA       (0x1<<22)
#define SCH_TLBE_MCE        (0x1<<21)
#else
#define SCH_GLBC_TLBE       (0x1<<30)
#define SCH_GLBC_TLBINV     (0x1<<29)
#endif
#define SCH_INTE_RESERR      (0x1<<29)
#define SCH_INTE_BSFULL      (0x1<<28)
#define SCH_INTE_ACFGERR     (0x1<<20)
#define SCH_INTE_TLBERR      (0x1<<18)
#define SCH_INTE_BSERR       (0x1<<17)
#define SCH_INTE_ENDF        (0x1<<16)
#define SCH_GLBC_HIMAP       (0x1<<15)
#define SCH_GLBC_HIAXI       (0x1<<9)
#define SCH_GLBC_EPRI0       (0x0<<7)
#define SCH_GLBC_EPRI1       (0x1<<7)
#define SCH_GLBC_EPRI2       (0x2<<7)
#define SCH_GLBC_EPRI3       (0x3<<7)
#define SCH_INTE_MASK	     (0x1f << 16)

#define REG_SCH_TLBA         0x00030

#ifdef JZM_V2_TLB
#define REG_SCH_TLBC        0x00050
#define SCH_TLBC_VPN        (0xFFFFF000)
#define SCH_TLBC_RIDX(idx)  (((idx) & 0xFF)<<4)
#define SCH_TLBC_INVLD      (0x1<<1)
#define SCH_TLBC_RETRY      (0x1<<0)

#define REG_SCH_TLBV        0x00054
#define SCH_TLBV_CNM(cnm)   (((cnm) & 0xFFF)<<16)
#define SCH_TLBV_GCN(gcn)   (((gcn) & 0xFFF)<<0)
#define SCH_TLBV_RCI_MC     (0x1<<30)
#define SCH_TLBV_RCI_EFE    (0x1<<31)
#endif

#define REG_SCH_STAT         0x00034
#define SCH_STAT_ORESERR     (0x1<<10)
#define SCH_STAT_BSERR       (0x1<<7)
#define SCH_STAT_JPGEND      (0x1<<4)
#define SCH_STAT_ACFGERR     (0x1<<2)
#define SCH_STAT_ENDFLAG     (0x1<<0)

#define REG_SCH_SLDE0        0x00040
#define REG_SCH_SLDE1        0x00044
#define REG_SCH_SLDE2        0x00048
#define REG_SCH_SLDE3        0x0004C
#define SCH_SLD_VTAG(val)    (((val) & 0xFFF)<<20)
#define SCH_SLD_MASK(val)    (((val) & 0xFFF)<<8)
#define SCH_SLD_VLD          (0x1<<0)

#define REG_SCH_SCHC         0x00060
#define SCH_CH1_PCH(ch)      (((ch) & 0x3)<<0)
#define SCH_CH2_PCH(ch)      (((ch) & 0x3)<<8)
#define SCH_CH3_PCH(ch)      (((ch) & 0x3)<<16)
#define SCH_CH4_PCH(ch)      (((ch) & 0x3)<<24)
#define SCH_CH1_PE           (0x1<<2)
#define SCH_CH2_PE           (0x1<<10)
#define SCH_CH3_PE           (0x1<<18)
#define SCH_CH4_PE           (0x1<<26)
#define SCH_CH1_GS0          (0x0<<3)
#define SCH_CH1_GS1          (0x1<<3)
#define SCH_CH2_GS0          (0x0<<11)
#define SCH_CH2_GS1          (0x1<<11)
#define SCH_CH3_GS0          (0x0<<19)
#define SCH_CH3_GS1          (0x1<<19)
#define SCH_CH4_GS0          (0x0<<27)
#define SCH_CH4_GS1          (0x1<<27)

#define REG_SCH_BND          0x00064
#define SCH_CH1_HID(hid)     (((hid) & 0xF)<<16)
#define SCH_CH2_HID(hid)     (((hid) & 0xF)<<20)
#define SCH_CH3_HID(hid)     (((hid) & 0xF)<<24)
#define SCH_CH4_HID(hid)     (((hid) & 0xF)<<28)
#define SCH_BND_G0F1         (0x1<<0)
#define SCH_BND_G0F2         (0x1<<1)
#define SCH_BND_G0F3         (0x1<<2)
#define SCH_BND_G0F4         (0x1<<3)
#define SCH_BND_G1F1         (0x1<<4)
#define SCH_BND_G1F2         (0x1<<5)
#define SCH_BND_G1F3         (0x1<<6)
#define SCH_BND_G1F4         (0x1<<7)
#define SCH_DEPTH(val)       (((val-1) & 0xF)<<8)

#define REG_SCH_SCHG0        0x00068
#define REG_SCH_SCHG1        0x0006C
#define REG_SCH_SCHE1        0x00070
#define REG_SCH_SCHE2        0x00074
#define REG_SCH_SCHE3        0x00078
#define REG_SCH_SCHE4        0x0007C

#define DSA_SCH_CH1          (JPEG_BASE | REG_SCH_SCHE1)
#define DSA_SCH_CH2          (JPEG_BASE | REG_SCH_SCHE2)
#define DSA_SCH_CH3          (JPEG_BASE | REG_SCH_SCHE3)
#define DSA_SCH_CH4          (JPEG_BASE | REG_SCH_SCHE4)
/********************************************
  SW_RESET (VPU software reset)
*********************************************/
#define REG_CFGC_SW_RESET    0x00004
#define REG_CFGC_RST         (0x1<<1)
#define REG_CFGC_RST_CLR     (0x0<<1)
#define REG_CFGC_EARB_EMPT   (0x4)
/********************************************
  VDMA (VPU general-purpose DMA)
*********************************************/
#define REG_VDMA_LOCK        ((HID_VDMA<<16) + 0x0000) //0x10000
#define REG_VDMA_UNLK        ((HID_VDMA<<16) + 0x0004) //0x10004

#define REG_VDMA_TASKRG      ((HID_VDMA<<16) + 0x0008) //0x10008
#define REG_VDMA_TASKRG_T21  0X00084

#define VDMA_ACFG_RUN        (0x1)
#define VDMA_DESC_RUN        (0x3)
#define VDMA_ACFG_CLR        (0x8)
#define VDMA_ACFG_SAFE       (0x4)
#define VDMA_ACFG_DHA(a)     (((unsigned int)(a)) & 0xFFFFFF80)
#define VDMA_DESC_DHA(a)     (((unsigned int)(a)) & 0xFFFF0)

#define REG_CFGC_ACM_CTRL    0x00084
#define REG_CFGC_ACM_STAT    0x00088
#define REG_CFGC_ACM_DHA     0x0008C

#define REG_VDMA_TASKST      ((HID_VDMA<<16) + 0x000C) //0x1000C
#define VDMA_ACFG_ERR        (0x1<<3)
#define VDMA_ACFG_END        (0x1<<2)
#define VDMA_DESC_END        (0x1<<1)
#define VDMA_VPU_BUSY        (0x1<<0)

#define VDMA_DESC_EXTSEL     (0x1<<0)
#define VDMA_DESC_TLBSEL     (0x1<<1)
#define VDMA_DESC_LK         (0x1<<31)

#define VDMA_ACFG_VLD        (0x1<<31)
#define VDMA_ACFG_TERM       (0x1<<30)
#define VDMA_ACFG_IDX(a)     (((unsigned int)(a)) & 0xFFFFC)

#ifdef RW_REG_TEST
#define GEN_VDMA_ACFG(chn, reg, term, val)  write_reg(JPEG_BASE+(reg), val)
#else
#define GEN_VDMA_ACFG(chn, reg, term, val)			\
  ({*chn++ = val;						\
	  *chn++ = (VDMA_ACFG_VLD | (term) | VDMA_ACFG_IDX(JPEG_BASE + (reg))); \
  })
#endif

#define REG_EMC_CTRL         (HID_EMC<<16)
#define REG_EMC_FRM_SIZE     (REG_EMC_CTRL + 0x0000)
#define REG_EMC_BS_ADDR      (REG_EMC_CTRL + 0x0004)
#define REG_EMC_DBLK_ADDR    (REG_EMC_CTRL + 0x0008)
#define REG_EMC_RECON_ADDR   (REG_EMC_CTRL + 0x000c)
#define REG_EMC_MV_ADDR      (REG_EMC_CTRL + 0x0010)
#define REG_EMC_SE_ADDR      (REG_EMC_CTRL + 0x0014)
#define REG_EMC_QPT_ADDR     (REG_EMC_CTRL + 0x0018)
#define REG_EMC_RC_RADDR     (REG_EMC_CTRL + 0x001c)
#define REG_EMC_MOS_ADDR     (REG_EMC_CTRL + 0x0020)
#define REG_EMC_SLV_INIT     (REG_EMC_CTRL + 0x0024)
#define REG_EMC_DEBUG_INFO0  (REG_EMC_CTRL + 0x0028)
#define REG_EMC_DEBUG_INFO1  (REG_EMC_CTRL + 0x002c)
#define REG_EMC_CRC_INFO0    (REG_EMC_CTRL + 0x0030)
#define REG_EMC_CRC_INFO1    (REG_EMC_CTRL + 0x0034)
#define REG_EMC_CRC_INFO2    (REG_EMC_CTRL + 0x0038)
#define REG_EMC_CRC_INFO3    (REG_EMC_CTRL + 0x003c)
#define REG_EMC_BS_SIZE      (REG_EMC_CTRL + 0x0040)
#define REG_EMC_BS_STAT      (REG_EMC_CTRL + 0x0044)
#define REG_EMC_RC_WADDR     (REG_EMC_CTRL + 0x0048)
#define REG_EMC_CPX_ADDR     (REG_EMC_CTRL + 0x004c)
#define REG_EMC_MOD_ADDR     (REG_EMC_CTRL + 0x0050)
#define REG_EMC_SAD_ADDR     (REG_EMC_CTRL + 0x0054)
#define REG_EMC_NCU_ADDR     (REG_EMC_CTRL + 0x0058)

/********************************************
  EFE (Encoder Front End)
*********************************************/
#define REG_EFE_CTRL         (HID_EFE<<16) //0x40000
#define EFE_TSE(en)          (((en) & 0x1)<<31)
#define EFE_FMVP(en)         (((en) & 0x1)<<30)
#define EFE_ID_X264          (0x0<<14)
#define EFE_ID_JPEG          (0x1<<14)
#define EFE_ID_VP8           (0x2<<14)
#define EFE_X264_QP(qp)      (((qp) & 0x3F)<<8)
#define EFE_VP8_QTB(qtb)     (((qtb) & 0x7f)<<22)
#define EFE_VP8_QIDX(qp)     (((qp) & 0x3F)<<8)
#define EFE_VP8_LF(lf)       ((lf & 0x3F)<<16)
#define EFE_HALN8_FLAG(en)   (((en) & 0x1)<<7)
#define EFE_STEP_MODE(en)    (((en) & 0x1)<<6)
#define EFE_DBLK_EN          (0x1<<5)
#define EFE_SLICE_TYPE(a)    (((a) & 0x1)<<4)
#define EFE_PLANE_TILE       (0x0<<2)
#define EFE_PLANE_420P       (0x1<<2)
#define EFE_PLANE_NV12       (0x2<<2)
#define EFE_PLANE_NV21       (0x3<<2)
#define EFE_EN               (0x1<<1)
#define EFE_RUN              (0x1<<0)

#define REG_EFE_GEOM         (REG_EFE_CTRL + 0x04) //0x40004
#define EFE_FST_MBY(mb)      (((mb) & 0xFF)<<24)
#define EFE_FST_MBX(mb)      (((0/*FIXME*/) & 0xFF)<<16)
#define EFE_LST_MBY(mb)      (((mb) & 0xFF)<<8)
#define EFE_LST_MBX(mb)      (((mb) & 0xFF)<<0)
#define EFE_JPGC_LST_MBY(mb) (((mb) & 0xFFFF)<<16)
#define EFE_JPGC_LST_MBX(mb) ((mb) & 0xFFFF)

#define REG_EFE_COEF_BA      (REG_EFE_CTRL + 0x000C)
#define REG_EFE_RAWY_SBA     (REG_EFE_CTRL + 0x0010)
#define REG_EFE_RAWC_SBA     (REG_EFE_CTRL + 0x0014)
#define REG_EFE_RAWU_SBA     (REG_EFE_CTRL + 0x0014)
#define REG_EFE_TOPMV_BA     (REG_EFE_CTRL + 0x0018)
#define REG_EFE_TOPPA_BA     (REG_EFE_CTRL + 0x001C)
#define REG_EFE_MECHN_BA     (REG_EFE_CTRL + 0x0020)
#define REG_EFE_MAUCHN_BA    (REG_EFE_CTRL + 0x0024)
#define REG_EFE_DBLKCHN_BA   (REG_EFE_CTRL + 0x0028)
#define REG_EFE_SDECHN_BA    (REG_EFE_CTRL + 0x002C)
#define REG_EFE_RAW_DBA      (REG_EFE_CTRL + 0x0030)
#define REG_EFE_RAWV_SBA     (REG_EFE_CTRL + 0x0034)

#define REG_EFE_ROI_MAX_QP       (REG_EFE_CTRL + 0x0040)
#define REG_EFE_ROI_BASE_INFO0   (REG_EFE_CTRL + 0x0044)
#define REG_EFE_ROI_BASE_INFO1   (REG_EFE_CTRL + 0x0048)
#define REG_EFE_ROI_POS_INFO0    (REG_EFE_CTRL + 0x004C)
#define REG_EFE_ROI_POS_INFO1    (REG_EFE_CTRL + 0x0050)
#define REG_EFE_ROI_POS_INFO2    (REG_EFE_CTRL + 0x0054)
#define REG_EFE_ROI_POS_INFO3    (REG_EFE_CTRL + 0x0058)
#define REG_EFE_ROI_POS_INFO4    (REG_EFE_CTRL + 0x005C)
#define REG_EFE_ROI_POS_INFO5    (REG_EFE_CTRL + 0x0060)
#define REG_EFE_ROI_POS_INFO6    (REG_EFE_CTRL + 0x0064)
#define REG_EFE_ROI_POS_INFO7    (REG_EFE_CTRL + 0x0068)
#define REG_EFE_QP_GEN_TAB       (REG_EFE_CTRL + 0x006C)

#define REG_EFE_QPG_CTRL         (REG_EFE_CTRL + 0x0074)
#define REG_EFE_QPG_CFG0         (REG_EFE_CTRL + 0x0078)
#define REG_EFE_QPG_CFG1         (REG_EFE_CTRL + 0x007C)
#define REG_EFE_QPG_CFG2         (REG_EFE_CTRL + 0x0080)
#define REG_EFE_QPG_CFG3         (REG_EFE_CTRL + 0x0084)
#define REG_EFE_QPG_CFG4         (REG_EFE_CTRL + 0x0088)
#define REG_EFE_QPG_CFG5         (REG_EFE_CTRL + 0x008C)
#define REG_EFE_QPG_CFG6         (REG_EFE_CTRL + 0x0090)
#define REG_EFE_QPG_RGNC_A       (REG_EFE_CTRL + 0x0094)
#define REG_EFE_QPG_RGNC_B       (REG_EFE_CTRL + 0x0098)
#define REG_EFE_QPG_RGNC_C       (REG_EFE_CTRL + 0x009C)
#define REG_EFE_QPG_RGNC_D       (REG_EFE_CTRL + 0x00A0)
#define REG_EFE_QP_SUM           (REG_EFE_CTRL + 0x00A4)

#define	REG_EFE_EIGEN_CFG0	 (REG_EFE_CTRL + 0x00C0)
#define	REG_EFE_EIGEN_CFG1	 (REG_EFE_CTRL + 0x00C4)
#define	REG_EFE_EIGEN_CFG2	 (REG_EFE_CTRL + 0x00C8)
#define	REG_EFE_EIGEN_CFG3	 (REG_EFE_CTRL + 0x00CC)
#define	REG_EFE_EIGEN_CFG4	 (REG_EFE_CTRL + 0x00D0)
#define	REG_EFE_EIGEN_CFG5	 (REG_EFE_CTRL + 0x00D4)
#define	REG_EFE_EIGEN_CFG6	 (REG_EFE_CTRL + 0x00D8)
#define	REG_EFE_DIFFY_CFG	 (REG_EFE_CTRL + 0x00DC)
#define	REG_EFE_DIFFU_CFG	 (REG_EFE_CTRL + 0x00E0)
#define	REG_EFE_DIFFV_CFG	 (REG_EFE_CTRL + 0x00E4)

#define	REG_EFE_SKIN_CTRL	 (REG_EFE_CTRL + 0x00AC)
#define	REG_EFE_SKIN_PTHD0	 (REG_EFE_CTRL + 0x00B0)
#define	REG_EFE_SKIN_PTHD1	 (REG_EFE_CTRL + 0x00B4)
#define	REG_EFE_SKIN_PTHD2	 (REG_EFE_CTRL + 0x00B8)
#define	REG_EFE_SKIN_QP_OFST (REG_EFE_CTRL + 0x00BC)
#define	REG_EFE_SKIN_PARAM0	 (REG_EFE_CTRL + 0x00E8)
#define	REG_EFE_SKIN_PARAM1	 (REG_EFE_CTRL + 0x00EC)
#define	REG_EFE_SKIN_PARAM2	 (REG_EFE_CTRL + 0x00F0)

#define REG_EFE_RAW_STRD     (REG_EFE_CTRL + 0x0038)
#define EFE_RAW_STRDY(y)     (((y) & 0xFFFF)<<16)
#define EFE_RAW_STRDC(c)     (((c) & 0xFFFF)<<0)

#define REG_EFE_DBG_INFO     (REG_EFE_CTRL + 0x003C)
#define EFE_DBG_EN           (0x1<<31)
#define EFE_DBG_BP_MBX(x)    (((x) & 0xFFF)<<0)
#define EFE_DBG_BP_MBY(y)    (((y) & 0xFFF)<<16)

#define REG_EFE_MVRP         (REG_EFE_CTRL + 0x0100)
#define REG_EFE_SSAD         (REG_EFE_CTRL + 0x0108)
#define REG_EFE_DCS          (REG_EFE_CTRL + 0x010C)
#define EFE_DCS_CLR(th)      (0x1<<(th & 0xF))
#define EFE_DCS_EN(en)       (((en) & 0x1)<<16)
#define EFE_DCS_RT(rt)       (((rt) & 0xF)<<20)
#define EFE_DCS_OTH(oth)     (((oth) & 0xF)<<24)
#define REG_EFE_STAT         (REG_EFE_CTRL + 0x0110)
#define REG_EFE_CQP_OFST     (REG_EFE_CTRL + 0x0120)

#define EFE_RC_QPO_CFG(c4, c3, c2, c1)		\
( ((c4) & 0x3F)<<18 |                           \
  ((c3) & 0x3F)<<12 |                           \
  ((c2) & 0x3F)<<6 |                            \
  ((c1) & 0x3F)<<0                              \
)

#define REG_EFE_RC_MINFO    (REG_EFE_CTRL + 0x0128)
#define EFE_RC_MB_EN        (0x1<<0)
#define EFE_RC_MBGP_NUM(a)  (((a) & 0x3fff)<<1)
#define EFE_RC_MAD_CFG_RDY  (0x1<<15)
#define REG_EFE_RC_BINFO0   (REG_EFE_CTRL + 0x012C)
#define EFE_RC_BU_EN        (0x1<<0)
#define EFE_RC_BU_NUM(a)    (((a) & 0x7f)<<1)
#define EFE_RC_BU_CFG(a)    (((a) & 0x1)<<8)
#define EFE_RC_SLICE_TP(a)  (((a) & 0x1)<<10)
#define REG_EFE_RC_BINFO1   (REG_EFE_CTRL + 0x0130)
#define EFE_RC_BU_SIZE(a)   (((a) & 0x3fff)<<0)
#define EFE_RC_BU_LSIZE(a)  (((a) & 0x3fff)<<14)
#define REG_EFE_RC_GP0THD   (REG_EFE_CTRL + 0x0134)
#define REG_EFE_RC_GP1THD   (REG_EFE_CTRL + 0x0138)
#define REG_EFE_RC_GP2THD   (REG_EFE_CTRL + 0x013C)
#define REG_EFE_RC_TBS      (REG_EFE_CTRL + 0x0140)
#define REG_EFE_RC_BNQA0    (REG_EFE_CTRL + 0x0144)
#define REG_EFE_RC_BPQA0    (REG_EFE_CTRL + 0x0148)
#define REG_EFE_RC_BNQA1    (REG_EFE_CTRL + 0x014C)
#define REG_EFE_RC_BPQA1    (REG_EFE_CTRL + 0x0150)
#define REG_EFE_RC_MNQCS    (REG_EFE_CTRL + 0x0154)
#define REG_EFE_RC_MPQCS    (REG_EFE_CTRL + 0x0158)
#define REG_EFE_RC_MTBQ     (REG_EFE_CTRL + 0x015C)
#define REG_EFE_RC_MRFQ     (REG_EFE_CTRL + 0x0160)
#define REG_EFE_RC_MAMIN    (REG_EFE_CTRL + 0x0164)
#define REG_EFE_RC_MAMAX    (REG_EFE_CTRL + 0x0168)
#define REG_EFE_RC_BBS      (REG_EFE_CTRL + 0x016C)

/****************************************************************
  JPGC (jpeg codec)
*****************************************************************/
#define REG_JPGC_CTRL        (HID_JPGC<<16) //0xE0000
#define REG_JPGC_TRIG        (REG_JPGC_CTRL + 0x0000)
#define REG_JPGC_GLBI        (REG_JPGC_CTRL + 0x0004)
#define REG_JPGC_STAT        (REG_JPGC_CTRL + 0x0008)
#define JPGC_STAT_ENDF       (0x1<<31)
#define REG_JPGC_BSA         (REG_JPGC_CTRL + 0x000C)
#define REG_JPGC_P0A         (REG_JPGC_CTRL + 0x0010)
#define REG_JPGC_P1A         (REG_JPGC_CTRL + 0x0014)
#define REG_JPGC_P2A         (REG_JPGC_CTRL + 0x0018)
#define REG_JPGC_P3A         (REG_JPGC_CTRL + 0x001C)
#define REG_JPGC_NMCU        (REG_JPGC_CTRL + 0x0028)
#define REG_JPGC_NRSM        (REG_JPGC_CTRL + 0x002C)
#define REG_JPGC_P0C         (REG_JPGC_CTRL + 0x0030)
#define REG_JPGC_P1C         (REG_JPGC_CTRL + 0x0034)
#define REG_JPGC_P2C         (REG_JPGC_CTRL + 0x0038)
#define REG_JPGC_P3C         (REG_JPGC_CTRL + 0x003C)
#define REG_JPGC_WIDTH       (REG_JPGC_CTRL + 0x0040)
#define REG_JPGC_MCUS        (REG_JPGC_CTRL + 0x0064)
#define REG_JPGC_ZIGM0       (REG_JPGC_CTRL + 0x1000)
#define REG_JPGC_ZIGM1       (REG_JPGC_CTRL + 0x1100)
#define REG_JPGC_HUFB        (REG_JPGC_CTRL + 0x1200)
#define REG_JPGC_HUFM        (REG_JPGC_CTRL + 0x1300)
#define REG_JPGC_QMEM        (REG_JPGC_CTRL + 0x1400)
#define REG_JPGC_HUFE        (REG_JPGC_CTRL + 0x1800)
#define REG_JPGC_HUFS        (REG_JPGC_CTRL + 0x1800)

#define JPGC_CORE_OPEN      (0x1<<0)
#define JPGC_BS_TRIG        (0x1<<1)
#define JPGC_PP_TRIG        (0x1<<2)
#define JPGC_TERM           (0x1<<3)
#define JPGC_RSTER_MD       (0x1<<8)

#define CPM_HELIX_SR		28
#define CPM_HELIX_STP		27
#define CPM_HELIX_ACK		26

#endif /*__HELIX_H__*/
