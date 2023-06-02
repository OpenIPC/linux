////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/mutex.h>

#include "MsTypes.h"
#include "ms_platform.h"

#include "infinity3e_reg_isp0.h"
#include "infinity3e_reg_isp1.h"
#include "infinity3e_reg_isp2.h"
#include "infinity3e_reg_isp3.h"
#include "infinity3e_reg_isp4.h"
#include "infinity3e_reg_isp5.h"
#include "infinity3e_reg_isp6.h"
#include "infinity3e_reg_isp7.h"
#include "infinity3e_reg_isp8.h"
#include "infinity3e_reg_isp9.h"

#include <mdrv_isp_io.h>
#include <mdrv_isp_io_st.h>

#include "hal_isp.h"

#define PAD16BITS  u32 :16
#define RSVD(ss,ee) u32 rsvd_##ss##_##ee[(ee) - (ss) + 1]
typedef infinity3e_reg_isp0 isp0_cfg_t;

static volatile infinity3e_reg_isp0 *pISP_INT;

static volatile infinity3e_reg_isp3 *isp_miu_bases;
static volatile infinity3e_reg_isp8 *isp_rgbir_miu_bases;

//static volatile u32 *isp_afsts; // 0x35

typedef struct {
	u32   null:1;
	u32   blinking:1;
	u32   embedded:1;
	u32   :5;
	u32   :6;
	u32   YUV4228B:1;
	u32   :1;
	PAD16BITS;
	u32   :8; //16:23
	u32   :2;
	u32   RAW8:1;
	u32   RAW10:1;
	u32   RAW12:1;
	u32   :3;
	PAD16BITS;
	PAD16BITS;
	PAD16BITS;
}__attribute__((packed, aligned(1))) csi_dt;

typedef struct {
	//0x0000
	u32 :1;
	u32 reg_ecc_off   :1;
	u32 reg_lane_en   :4;
	u32 reg_rm_err_sot:1;
	u32 reg_fun_md    :2;
	u32 reg_vc_en     :4;
	u32 reg_rm_err_fs :1;
	u32 reg_uy_vy_sel :1;
	u32 :1;
	PAD16BITS;
#define CSI_LAN0_EN 0x01
#define CSI_LAN1_EN 0x02
#define CSI_LAN2_EN 0x04
#define CSI_LAN3_EN 0x08
#define CSI_VC0_EN 0x01
#define CSI_VC1_EN 0x02
#define CSI_VC3_EN 0x04
#define CSI_VC4_EN 0x08

	//0x0004
	u32 reg_vc0_hs_mode:3;
	u32 reg_vc1_hs_mode:3;
	u32 reg_vc2_hs_mode:3;
	u32 reg_vc3_hs_mode:3;
	u32 reg_debug_sel  :4;
	PAD16BITS;

	//0x0008
	u32 reg_vc0_vs_mode:3;
	u32 reg_vc1_vs_mode:3;
	u32 reg_vc2_vs_mode:3;
	u32 reg_vc3_vs_mode:3;
	u32 reg_debug_en   :1;
	u32 :3;
	PAD16BITS;
#define CSI_V_SYNC_MODE_FS 0x00
#define CSI_V_SYNC_MODE_FE 0x01

	//0x000C
	u32 reg_mac_err_int_mask:15;
	u32 :1;
	PAD16BITS;

	//0x0010
	u32 reg_mac_err_int_force:15;
	u32 :1;
	PAD16BITS;

	//0x0014
	u32 reg_mac_err_int_clr:15;
	u32 :1;
	PAD16BITS;

	//0x0018
	u32 reg_mac_rpt_int_mask:7;
	u32 :9;
	PAD16BITS;
#define CSI_LINE_INT    0x01
#define CSI_FRAME_INT  0x02
#define CSI_VC3_FE_INT  0x08
#define CSI_VC2_FE_INT  0x10
#define CSI_VC1_FE_INT  0x20
#define CSI_VC0_INT    0x40  //FS or FE depend on reg_mac_rpt_int_src[1] selection

	u32 reg_mac_rpt_int_force:7;
	u32 :9;
	PAD16BITS;

	u32 reg_mac_rpt_int_clr:7;
	u32 :9;
	PAD16BITS;

	u32 reg_phy_int_mask:10;
	u32 :6;
	PAD16BITS;

	u32 reg_phy_int_force:10;
	u32 :6;
	PAD16BITS;

	u32 reg_phy_int_clr:10;
	u32 :6;
	PAD16BITS;

	u32 reg_mac_err_int_src:15;
	u32 :1;
	PAD16BITS;

	u32 reg_mac_err_int_raw_src:15;
	u32 :1;
	PAD16BITS;

	u32 reg_mac_rpt_int_src:7;
	u32 :9;
	PAD16BITS;

	u32 reg_mac_rpt_int_raw_src:7;
	u32 :9;
	PAD16BITS;

	u32 reg_phy_int_src:10;
	u32 :6;
	PAD16BITS;

	u32 reg_phy_int_raw_src:10;
	u32 :6;
	PAD16BITS;

	u32 reg_frm_num:16;
	PAD16BITS;

	u32 reg_line_num:16;
	PAD16BITS;

	u32 reg_g8spd_wc:16;
	PAD16BITS;

	u32 reg_g8spd_dt:8;
	u32 :8;
	PAD16BITS;

	u32 reg_mac_idle:1;
	u32 :15;
	PAD16BITS;

	u32 reg_1frame_trig:1;
	u32 :15;
	PAD16BITS;

	u32 reg_ckg_csi_mac:6;
	u32 :2;
	u32 reg_ckg_csi_mac_lptx:5;
	u32 :3;
	PAD16BITS;

	u32 reg_ckg_ns:5;
	u32 :11;
	PAD16BITS;

	u32 reg_csi_mac_reserved:16;
	PAD16BITS;

	u32 reg_csi_mac_reserved1:16;
	PAD16BITS;

	u32 reg_csi_mac_reserved2:16;
	PAD16BITS;

	csi_dt reg_dt_en;

	u32 reg_sw_rst      :1;
	u32 reg_mac_en      :1;
	u32 reg_dont_care_dt:1;
	u32 :13;
	PAD16BITS;

	u32 reg_raw_l_sot_sel:2;
	u32 reg_sot_sel      :1;
	u32 reg_eot_sel      :1;
	u32 reg_rpt_fnum_cond:1;
	u32 reg_rpt_lnum_cond:1;
	u32 :10;
	PAD16BITS;
#define CSI_RPT_FS 0x0    //report frame start
#define CSI_RTP_FE 0x01  //report frame end

	u32 reg_clk_lane_fsm_sts_int_mask:16;
	PAD16BITS;

	u32 reg_clk_lane_fsm_sts_int_force:16;
	PAD16BITS;

	u32 reg_clk_lane_fsm_sts_int_clr:16;
	PAD16BITS;

	u32 reg_clk_lane_fsm_sts_int_src:16;
	PAD16BITS;

	u32 reg_clk_lane_fsm_sts_int_raw_src:16;
	PAD16BITS;

	u32 reg_data_lane0_fsm_sts_int_mask:16;
	PAD16BITS;

	u32 reg_data_lane0_fsm_sts_int_force:16;
	PAD16BITS;

	u32 reg_data_lane0_fsm_sts_int_clr:16;
	PAD16BITS;

	u32 reg_data_lane0_fsm_sts_int_src:16;
	PAD16BITS;

	u32 reg_data_lane0_fsm_sts_int_raw_src:16;
	PAD16BITS;

	u32 reg_data_lane1_fsm_sts_int_mask:16;
	PAD16BITS;

	u32 reg_data_lane1_fsm_sts_int_force:16;
	PAD16BITS;

	u32 reg_data_lane1_fsm_sts_int_clr:16;
	PAD16BITS;

	u32 reg_data_lane1_fsm_sts_int_src:16;
	PAD16BITS;

	u32 reg_data_lane1_fsm_sts_int_raw_src:16;
	PAD16BITS;
	} __attribute__((packed, aligned(1))) csi_mac;

static volatile infinity3e_reg_isp4 *isp_miu;

static volatile u32 *isp_vdos_pitch;
static volatile u32 *isp_vdos_addr;

static volatile void *RIUBASE_ISP[10] = {0};

// mload variable
dma_addr_t mload_dma_addr = 0;
u8  *mload_virt_addr = NULL;
static DEFINE_MUTEX(mloadMutex);
u32 pTableSize[eMLOAD_ID_NUM] = {	
	ALSC_TBL_SIZE, //eMLOAD_ID_ALSC_R_TBL = 0
	ALSC_TBL_SIZE, //eMLOAD_ID_ALSC_G_TBL = 1 
	ALSC_TBL_SIZE, //eMLOAD_ID_ALSC_B_TBL = 2
	GAMMA_TBL_SIZE, //eMLOAD_ID_LN_GMA12TO10_R = 3
	GAMMA_TBL_SIZE, //eMLOAD_ID_LN_GMA12TO10_G = 4
	GAMMA_TBL_SIZE, //eMLOAD_ID_LN_GMA12TO10_B = 5
	DEFECTPIX_TBL_SIZE, //eMLOAD_ID_DP_TBL = 6 
	0,
	0,
	0,
	GAMMA_TBL_SIZE, //eMLOAD_ID_LN_GMA10TO10_R = 10
	GAMMA_TBL_SIZE, // eMLOAD_ID_LN_GMA10TO10_G = 11
	GAMMA_TBL_SIZE, //eMLOAD_ID_LN_GMA10TO10_B = 12
	0,
	FPN_OFFSET_TBL_SIZE, //eMLOAD_ID_FPN_OFFSET	14
};

// memory offset in dma memory address
u32	tblOffset[eMLOAD_ID_NUM] = {
	offsetof(MLoadLayout,alsc_r),
	offsetof(MLoadLayout,alsc_g),
	offsetof(MLoadLayout,alsc_b),
	offsetof(MLoadLayout,gamma12to10_r),
	offsetof(MLoadLayout,gamma12to10_g),
	offsetof(MLoadLayout,gamma12to10_b),
	offsetof(MLoadLayout,dpc),
	0,
	0,
	0,
	offsetof(MLoadLayout,gamma10to10_r),
	offsetof(MLoadLayout,gamma10to10_g),
	offsetof(MLoadLayout,gamma10to10_b),
	0,
	offsetof(MLoadLayout,fpn),
};

static volatile u32 u4DefDisable[3] =
{
	//interrupt group 0
	ISP_SHIFTBITS(INT_DB_UPDATE_DONE)|ISP_SHIFTBITS(INT_SR_VREF_RISING)|ISP_SHIFTBITS(INT_SR_VREF_FALLING)|ISP_SHIFTBITS(INT_ISP_BUSY)|ISP_SHIFTBITS(INT_ISP_IDLE)|ISP_SHIFTBITS(INT_PAD_VSYNC_RISING)|ISP_SHIFTBITS(INT_PAD_VSYNC_FALLING),
	//interrupt group 1
	ISP_SHIFTBITS(INT2_VDOS_EVERYLINE)|ISP_SHIFTBITS(INT2_VDOS_LINE0)|ISP_SHIFTBITS(INT2_VDOS_LINE1)|ISP_SHIFTBITS(INT2_VDOS_LINE2),
	//interrupt group 2
	ISP_SHIFTBITS(INT3_AE_WIN1_DONE)|ISP_SHIFTBITS(INT3_MENULOAD_DONE)
	//|ISP_SHIFTBITS(INT3_SW_INT_OUTPUT_DONE)
	//|ISP_SHIFTBITS(INT3_SW_INT_INPUT_DONE)
	//|ISP_SHIFTBITS(INT3_HIT_LINE_COUNT1)|ISP_SHIFTBITS(INT3_HIT_LINE_COUNT2)|ISP_SHIFTBITS(INT3_HIT_LINE_COUNT3)
	|ISP_SHIFTBITS(INT3_AWB_ROW_DONE)|ISP_SHIFTBITS(INT3_HISTO_ROW_DONE)
	|ISP_SHIFTBITS(INT3_RESERVED0)|ISP_SHIFTBITS(INT3_RESERVED1)|ISP_SHIFTBITS(INT3_RESERVED2)
};

inline void HalInitRegs(void **pRegs)
{
	int i = 0;
	for (i = 0; i < 10; i++)
		RIUBASE_ISP[i] = (void*)pRegs[i];

	// disable double buffer
	*(volatile u32*)(RIUBASE_ISP[0] + 0x1*4) |= (1<<15);
	// init frame cnt
	*(volatile u32*)(RIUBASE_ISP[0] + 0x7a*4) = 0;

	// structured INT
	pISP_INT = (volatile infinity3e_reg_isp0*)(RIUBASE_ISP[0]);

	// statistics output address
	isp_miu_bases = (volatile infinity3e_reg_isp3*)(RIUBASE_ISP[3]);
	isp_rgbir_miu_bases = (volatile infinity3e_reg_isp8*)(RIUBASE_ISP[8]);
	// menuload
	isp_miu = (infinity3e_reg_isp4  *)(RIUBASE_ISP[4]);

	// vdos
	isp_vdos_pitch = (volatile u32*)(RIUBASE_ISP[6] + 0x21*4);
	isp_vdos_addr = (volatile u32*)(RIUBASE_ISP[6] + 0x28*4);

	// disable clock gating
	(*(u16*)(RIUBASE_ISP[0] + 0x64*4)) &= ~0x0004;

	mload_virt_addr = mloadInit(sizeof(MLoadLayout));
	pr_info("mload_size = %d\n", sizeof(MLoadLayout));
	pr_info("mload_virt_addr = %p\n", mload_virt_addr);
	pr_info("mload_dma_addr = %#x\n", mload_dma_addr);
}

inline void HalISPDisableInt()
{
	pISP_INT->reg_c_irq_mask  = 0xffff;
	pISP_INT->reg_c_irq_mask2  = 0x000f;
	pISP_INT->reg_c_irq_mask3  = 0x00ff;
}

inline void HalISPEnableInt()
{
	pISP_INT->reg_c_irq_mask  = u4DefDisable[0];
	pISP_INT->reg_c_irq_mask2  = u4DefDisable[1];
	pISP_INT->reg_c_irq_mask3  = u4DefDisable[2];

	pISP_INT->reg_c_irq_clr = 0xffff & ~(u4DefDisable[0]);
	pISP_INT->reg_c_irq_clr = 0;
	pISP_INT->reg_c_irq_clr2 = 0x00ff & ~(u4DefDisable[1]);
	pISP_INT->reg_c_irq_clr2 = 0;
	pISP_INT->reg_c_irq_clr3 = 0x00ff & ~(u4DefDisable[2]);
	pISP_INT->reg_c_irq_clr3 = 0;

}

#if 0
inline void HalISPMaskIntAll()
{
	pISP_INT->reg_c_irq_mask  = 0xffff;
	pISP_INT->reg_c_irq_mask2  = 0x000f;
	pISP_INT->reg_c_irq_mask3  = 0x00ff;

	pISP_INT->reg_c_irq_clr = 0xffff & ~(u4DefDisable[0]);
	pISP_INT->reg_c_irq_clr2 = 0x000f & ~(u4DefDisable[1]);
	pISP_INT->reg_c_irq_clr3 = 0x003f & ~(u4DefDisable[2]);
}
#endif

inline u32 HalISPGetIntStatus1()
{
	return (~pISP_INT->reg_c_irq_mask) & pISP_INT->reg_irq_final_status;
}

inline u32 HalISPGetIntStatus2()
{
	return (~pISP_INT->reg_c_irq_mask2) & pISP_INT->reg_irq_final_status2;
}

inline u32 HalISPGetIntStatus3()
{
	return (~pISP_INT->reg_c_irq_mask3) & pISP_INT->reg_irq_final_status3;
}

inline void HalISPMaskClear1(u32 clear)
{
	pISP_INT->reg_c_irq_clr |= clear;
	pISP_INT->reg_c_irq_clr = 0;
	//pISP_INT->reg_c_irq_mask = u4DefDisable[0];
}

inline void HalISPMaskClear2(u32 clear)
{
	pISP_INT->reg_c_irq_clr2 |= clear;
	pISP_INT->reg_c_irq_clr2 = 0;
	//pISP_INT->reg_c_irq_mask2 = u4DefDisable[1];
}

inline void HalISPMaskClear3(u32 clear)
{
	pISP_INT->reg_c_irq_clr3 |= clear;
	pISP_INT->reg_c_irq_clr3 = 0;
	//pISP_INT->reg_c_irq_mask3 = u4DefDisable[2];
}

inline void HalISPMaskInt1(u32 mask)
{
	pISP_INT->reg_c_irq_mask |= (mask|u4DefDisable[0]);
}

inline void HalISPMaskInt2(u32 mask)
{
	pISP_INT->reg_c_irq_mask2 |= (mask|u4DefDisable[1]);
}

inline void HalISPMaskInt3(u32 mask)
{
	pISP_INT->reg_c_irq_mask3 |= (mask|u4DefDisable[2]);
}

inline void HalISPClearInt(volatile u32 *clear, ISP_INT_STATUS int_num)
{
	*clear |= (0x01<<int_num);
}

#if 0
inline void HalISPClearIntAll()
{
	pISP_INT->reg_c_irq_mask = pISP_INT->reg_c_irq_clr;
	pISP_INT->reg_c_irq_clr = 0;
	pISP_INT->reg_c_irq_mask2 = pISP_INT->reg_c_irq_clr2;
	pISP_INT->reg_c_irq_clr2 = 0;
	pISP_INT->reg_c_irq_mask3 = pISP_INT->reg_c_irq_clr3;
	pISP_INT->reg_c_irq_clr3 = 0;
}
#endif

inline void HalISPClkEnable(bool enable)
{
	if (enable == false)
		*(volatile u32*)(RIUBASE_ISP[0] + 0x66*4) |=  (u16)0x04; // 0x35
	else
		*(volatile u32*)(RIUBASE_ISP[0] + 0x66*4) &= ~((u16)0x04);
}

inline u32 HalISPGetVsyncPol()
{
	return (pISP_INT->reg_sensor_vsync_polarity & 0x01);
}

inline u32 HalISPGetMIPI(void)
{
	return (pISP_INT->reg_isp_if_src_sel & 0x01);
}

inline void HalISPGetAFStat(volatile unsigned long *Stat)
{
	int i = 0;
	volatile u32*  isp_afsts = (volatile u32*)(RIUBASE_ISP[5] + 0x18*4);
	unsigned long value = 0, LSB = 0, MSB = 0;
	if (Stat)
		for (i = 0; i < 15*2; i+=2) {
			LSB = *(u32 *)(isp_afsts+i)&0xffff;
			MSB = *(u32 *)(isp_afsts+(i+1))&0xffff;
			value = LSB | (MSB << 16);
			Stat[i/2] = value;
		}
}

inline void HalISPGetVDOSPitch(volatile int *pPitch)
{
	if (pPitch)
		*pPitch = *(int *)isp_vdos_pitch;
}

inline void HalISPGetVDOSSize(volatile int *pSize, volatile int Pitch)
{
	if (pSize)
		*pSize = Pitch * 24;  // pitch * 3 * 8bytes
}

inline void HalISPGetVDOSData(volatile unsigned long *pAddr, volatile int Size)
{
	if (pAddr)
		memcpy((void *)pAddr, (void *)isp_vdos_addr, Size);
}

inline void HalISPSetAeBaseAddr(unsigned long Addr,unsigned int size)
{
	unsigned long MiuAddr = Chip_Phys_to_MIU(Addr);
	isp_miu_bases->reg_ae_statis_base = MiuAddr>>4;
	isp_miu_bases->reg_ae_statis_base_1 = MiuAddr>>20;
	//set dma upper bound
	MiuAddr = Chip_Phys_to_MIU(Addr+size);
	isp_miu_bases->reg_ae_max_wadr = MiuAddr >> 4;
	isp_miu_bases->reg_ae_max_wadr_1 = MiuAddr >> 20;
}

inline void HalISPSetAwbBaseAddr(volatile unsigned long Addr,unsigned int size)
{
	unsigned long MiuAddr = Chip_Phys_to_MIU(Addr);
	isp_miu_bases->reg_awb_statis_base = MiuAddr>>4;
	isp_miu_bases->reg_awb_statis_base_1 = MiuAddr>>20;
	//set dma upper bound
	MiuAddr = Chip_Phys_to_MIU(Addr+size);
	isp_miu_bases->reg_awb_max_wadr = MiuAddr >> 4;
	isp_miu_bases->reg_awb_max_wadr_1 = MiuAddr >> 20;
}

inline void HalISPSetAfBaseAddr(volatile unsigned long Addr,unsigned int size)
{
	volatile infinity3e_reg_isp5 *isp5 = (infinity3e_reg_isp5*) RIUBASE_ISP[5];
	unsigned long MiuAddr = Chip_Phys_to_MIU(Addr);
	isp5->reg_af_dma_addr = MiuAddr>>4;
	isp5->reg_af_dma_addr_1 = MiuAddr>>20;
    //set dma upper bound
    MiuAddr = Chip_Phys_to_MIU(Addr+size);
    isp5->reg_af_max_wadr = MiuAddr >> 4;
    isp5->reg_af_max_wadr_1 = MiuAddr >> 20;
}

//set motion detection statisic buffer address
inline void HalISPSetMotBaseAddr(volatile unsigned long Addr,unsigned int size)
{
	unsigned long MiuAddr = Chip_Phys_to_MIU(Addr);
	if(Addr & 0x0F)
	{
		pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
	}
	isp_miu_bases->reg_mot_fb_base = MiuAddr>>4;
	isp_miu_bases->reg_mot_fb_base_1 = MiuAddr>>20;
}

inline void HalISPSetHistoBaseAddr(unsigned long Addr,unsigned int size)
{
	unsigned long MiuAddr = Chip_Phys_to_MIU(Addr);
	if(Addr & 0x0F)
	{
		pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
	}
	isp_miu_bases->reg_histo_statis_base = MiuAddr>>4;
	isp_miu_bases->reg_histo_statis_base_1 = MiuAddr>>20;
    //set dma upper bound
    MiuAddr = Chip_Phys_to_MIU(Addr+size);
    isp_miu_bases->reg_histo_max_wadr = MiuAddr >> 4;
    isp_miu_bases->reg_histo_max_wadr_1 = MiuAddr >> 20;
}

inline void HalISPSetRgbIRBaseAddr(unsigned long Addr,unsigned int size)
{
	unsigned long MiuAddr = Chip_Phys_to_MIU(Addr);
	if(Addr & 0x0F)
	{
		pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
	}
	isp_rgbir_miu_bases->reg_miu_base_addr = MiuAddr>>4;
	isp_rgbir_miu_bases->reg_miu_base_addr_1 = MiuAddr>>20;
    //set dma upper bound
    MiuAddr = Chip_Phys_to_MIU(Addr+size);
    isp_rgbir_miu_bases->reg_rgbir_awb_max_addr = MiuAddr >> 4;
    isp_rgbir_miu_bases->reg_rgbir_awb_max_addr_1 = MiuAddr >> 20;
}

inline void HalISPSetDnrFbAddr(unsigned long Addr, int id)
{
	if(Addr & 0x0F){
		pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
	}
	Addr = Chip_Phys_to_MIU(Addr);
	if(id==0) {
		isp_miu_bases->reg_dnr_fb_base_0 = (Addr>>4) & 0xFFFF;
		isp_miu_bases->reg_dnr_fb_base_0_1 = (Addr>>20) & 0xFFF;
	} else if(id==1) {
		isp_miu_bases->reg_dnr_fb_base_1 = (Addr>>4) & 0xFFFF;
		isp_miu_bases->reg_dnr_fb_base_1_1 = (Addr>>20) & 0xFFF;
	}
	pr_debug("set DNR fb addr = 0x%X\n",(uint32_t)Addr);
}

inline void HalISPSetDnrUbound(unsigned long Addr)
{
	if(Addr & 0x0F)
    {
        pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
    }
	Addr = Chip_Phys_to_MIU(Addr);
    isp_miu_bases->reg_dnr_wdma_uaddr = (Addr>>4) & 0xFFFF;
    isp_miu_bases->reg_dnr_wdma_uaddr_1 = (Addr>>20) & 0xFFF;
}
#if 1
inline void waitMloadDone(void)
{
	int i,time_out = 1, us_count = 50000;

	for(i = 0; i < us_count; i++) {
		if(isp_miu->reg_isp_load_done) {
			time_out = 0;
			break;
		}
		udelay(1);
	}
	if (time_out) {
		pr_err("[%s] Wait %d us, TimeOut: %d, Done:%d, Status: %d, full:%d\n", __FUNCTION__,
				us_count, time_out,
				isp_miu->reg_isp_load_done,
				isp_miu->reg_isp_load_start_error,
				isp_miu->reg_isp_load_register_non_full);
	}
}

//Loading Data From DRAM to SRAMs
inline void HalISPMLoadWriteData(ISP_MLOAD_ID Sram_Id, volatile unsigned long Addr, size_t size)
{
	u32 k;

	Addr = Chip_Phys_to_MIU(Addr);

	//printk(KERN_INFO "[ISP] HalISPMLoadWriteData addr:0x%08x\n", (u32)Addr);

	mutex_lock(&mloadMutex);
	//Enable the engine by turning on the register
	isp_miu->reg_isp_miu2sram_en = 1;
	//Set SW reset as 0 and 1
	isp_miu->reg_isp_load_sw_rst = 0;
	isp_miu->reg_isp_load_sw_rst = 1;
	isp_miu->reg_isp_load_wait_hardware_ready_en = 1;
	isp_miu->reg_isp_load_water_level = 0;

	//Set SW read write mode as 1 (write)
	isp_miu->reg_isp_sram_rw     = 1;// 1:write
	//Set SRAM ID
	isp_miu->reg_isp_load_sram_id = Sram_Id;
	//Set MIU 16-byte start address
	isp_miu->reg_isp_load_st_addr = (Addr>>4) & 0xFFFF;
	isp_miu->reg_isp_load_st_addr_1 = (Addr>>20) & 0xFFFF;
	//Set data amount (2-byte)
	//The number should be set as length - 1
	isp_miu->reg_isp_load_amount    = size - 1;
	//Set destination SRAM start SRAM address (2-byte)
	isp_miu->reg_isp_sram_st_addr   = 0x0000;
	//Set SRAM loading parameter by setting write-one-clear
	isp_miu->reg_isp_load_register_w1r = 1;
	//write 0, HW is not working, same as dummy command
	for(k = 0; k < 32; k++) {
		isp_miu->reg_isp_load_register_w1r = 0;
	}
	//Enable the engine by turning on the register
	//isp_miu->reg_isp_miu2sram_en = 1;
	//Fire Loading by setting write-one-clear
	isp_miu->reg_isp_load_st_w1r = 1;

	waitMloadDone();
	isp_miu->reg_isp_miu2sram_en = 0;
		

	mutex_unlock(&mloadMutex);
}

//Loading All table From DRAM to SRAMs
inline void HalISPMLoadWriteAllTable(volatile unsigned long Addr)
{
	u32 i,k;
	unsigned long tbl_addr = 0;

	Addr = Chip_Phys_to_MIU(Addr);

	mutex_lock(&mloadMutex);
	//Enable the engine by turning on the register
	isp_miu->reg_isp_miu2sram_en = 1;
	//Set SW reset as 0 and 1
	isp_miu->reg_isp_load_sw_rst = 0; //this register is active low, set 0 to reset
	isp_miu->reg_isp_load_sw_rst = 1;
	//Set SW read write mode as 1 (write)

	for(i = 0; i < eMLOAD_ID_NUM; i++) {
		if(pTableSize[i]==0)
		{
			continue;
		}
		// get offset
		tbl_addr = Addr + tblOffset[i];

		pr_debug("MLOAD[%d] tbl_addr = %#lX \n", i, tbl_addr);

		isp_miu->reg_isp_sram_rw     = 1;// 1:write
		//Set SRAM ID
		isp_miu->reg_isp_load_sram_id = i;
		//Set MIU 16-byte start address
		isp_miu->reg_isp_load_st_addr = (tbl_addr>>4) & 0xFFFF;
		isp_miu->reg_isp_load_st_addr_1 = (tbl_addr>>20) & 0xFFFF;
		//Set data amount (2-byte)
		//The number should be set as length - 1
		isp_miu->reg_isp_load_amount    = pTableSize[i] - 1;
		//Set destination SRAM start SRAM address (2-byte)
		isp_miu->reg_isp_sram_st_addr   = 0x0000;
		//Set SRAM loading parameter by setting write-one-clear
		isp_miu->reg_isp_load_register_w1r = 1;
		//write 0, HW is not working, same as dummy command
		for(k = 0; k < 32; k++) {
			isp_miu->reg_isp_load_register_w1r = 0;
		}
	}

	//Fire Loading by setting write-one-clear
	isp_miu->reg_isp_load_st_w1r = 1;
	
	waitMloadDone();
	isp_miu->reg_isp_miu2sram_en = 0;

	mutex_unlock(&mloadMutex);
}

//#define ISP_SENSOR_MASK
//Read one u16 Data From SRAMs for debug
inline void HalISPMLoadReadData(ISP_MLOAD_ID Sram_Id, volatile unsigned short *table, size_t size)
{
	int i, k;
#if defined(ISP_SENSOR_MASK)
	volatile infinity3e_reg_isp0 *isp0 = (volatile infinity3e_reg_isp0*)RIUBASE_ISP[0];
#endif
	mutex_lock(&mloadMutex);

#if defined(ISP_SENSOR_MASK)
	// only for gamma10to10 read
	switch (Sram_Id){
		case eMLOAD_ID_LN_GMA10TO10_R:
		case eMLOAD_ID_LN_GMA10TO10_G:
		case eMLOAD_ID_LN_GMA10TO10_B:
			isp0->reg_isp_sensor_mask = 1;
			break;
		default:
			break;
	}
#endif

	isp_miu->reg_isp_miu2sram_en = 1;
	isp_miu->reg_isp_sram_rw       = 0;

	for(i = 0; i < size; i++) {
		isp_miu->reg_isp_sram_read_id  = Sram_Id;

		isp_miu->reg_isp_sram_rd_addr  = i;
		isp_miu->reg_isp_sram_read_w1r = 1;

		//write 0, HW is not working, same as dummy command
		for(k = 0; k < 32; k++) {
			isp_miu->reg_isp_sram_read_w1r = 0;
		}

		//Read data from register, only 16 bits data
		table[i] = isp_miu->reg_isp_sram_read_data;
		//pr_info("table[%d] = %#x, %p \n", i, table[i], &table[i]);
	}

	isp_miu->reg_isp_sram_rw       = 1;
	isp_miu->reg_isp_miu2sram_en   = 0;

#if defined(ISP_SENSOR_MASK)
	if(isp0->reg_isp_sensor_mask)
		isp0->reg_isp_sensor_mask = 0;
#endif

	mutex_unlock(&mloadMutex);
}

#endif

inline u8 HalISPGetFrameDoneCount(void)
{
	//u16 val = *(u16*)(RIUBASE_ISP[0] + 0x7A*4);
	//return  (val >> 8) & 0x7F;
	return pISP_INT->reg_isp_frm_done_cnt;
}

inline void IspReset(void)
{
	volatile infinity3e_reg_isp0 *isp0 = (volatile infinity3e_reg_isp0*)RIUBASE_ISP[0];
	u32 fifo_mask = isp0->reg_isp_sensor_mask;
	isp0->reg_isp_sensor_mask = 1;//fifo gating
	isp0->reg_isp_sw_rstz = 0;
	isp0->reg_sensor_sw_rstz = 0;
	udelay(1);
	isp0->reg_sensor_sw_rstz = 1;
	isp0->reg_isp_sw_rstz = 1;
	isp0->reg_isp_sensor_mask = fifo_mask;//fifo gating
}

inline void IspInputEnable(u32 enable)
{
	isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
	//isp0->reg_isp_sensor_mask = enable?0:1;
	if(enable)
	{
		isp0->reg_isp_sensor_mask = 0;
		isp0->reg_isp_icp_ack_tie1 = 0;
	}
	else
	{
		isp0->reg_isp_sensor_mask = 1;
		isp0->reg_isp_icp_ack_tie1 = 1;
	}
}

inline void IspAsyncEnable(u32 enable)
{
	isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
	isp0->reg_isp_icp_ack_tie1 = enable?1:0;
}

inline void HalIspSetAEDgain(u32 enable,u32 gain)
{
	volatile infinity3e_reg_isp1 *isp1 = (volatile infinity3e_reg_isp1*) RIUBASE_ISP[1];
	isp1->reg_isp_ae_en = enable?1:0;
	isp1->reg_isp_ae_gain = gain;
}

typedef struct {
	u32 val                             :13; //0x32
	u32 /* reserved */                  :3;
										 u32 padding                         :16;
} __attribute__((packed, aligned(1))) isp_ccm_coeff;


void HalISPSetYUVCCM(const s16 *ccm_coeff)
{
	int n=0;
	volatile isp_ccm_coeff* coeff = (volatile isp_ccm_coeff*)(((char*)RIUBASE_ISP[0]) + 0x3d*4);
	for(n=0;n<9;++n)
	{
		coeff[n].val = ccm_coeff[n]&0x1FFF;
	}
}

void HalISPGetImageSize(u32* width,u32* high)
{
	*width = 1920;
	*high = 1080;
}

void HalISPGetAeBlkSize(u32* x,u32* y)
{
	*x = 15;
	*y = 12;
}

u32 HalISPGetRotEn(void)
{
	return 0;
}

u32 HalISPVDOSInit(void){

	isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
	infinity3e_reg_isp6 *isp6 = (infinity3e_reg_isp6*)RIUBASE_ISP[6];


	isp0->reg_vdos_sblock_blk = 3;
	isp0->reg_vdos_x_st = 800;
	isp0->reg_vdos_y_st = 100;
	isp0->reg_vdos_sblock_width = 170;
	isp0->reg_vdos_sblock_height = 170;
	isp0->reg_isp_icp_ack_tie1 = 0;

	isp0->reg_isp_wdma_crop_start_x = 0;
	isp0->reg_isp_wdma_crop_start_y = 0;

	isp0->reg_isp_wdma_crop_width=1021;  // 170 * 3 + 512 -1 = 1021
	isp0->reg_isp_wdma_crop_high=2;


	isp0->reg_load_reg = 0;
	isp0->reg_vdos_yc_en = 0;
	isp6->reg_isp_wdma_align_en = 1;

  #if 0
	isp6->reg_isp_rdma_pitch =  ((1021+7)>>3)*1;
	isp6->reg_isp_wdma_pitch =  ((1021+7)>>3)*1;
  #elif 0
  isp6->reg_isp_rdma_pitch =  (((1021+32)>>5)<<5)>>3;
	isp6->reg_isp_wdma_pitch =  (((1021+32)>>5)<<5)>>3;
	#elif 1
  isp6->reg_isp_rdma_pitch =  (((1021+8)>>3)<<3)>>3;
	isp6->reg_isp_wdma_pitch =  (((1021+8)>>3)<<3)>>3;
  #endif

	isp0->reg_vdos_en =1;
	return 0;
}

#define LIMIT_BITS(x, bits) ((x) > ((1<<(bits))-1) ? (((1<<(bits))-1)) : (x))

u32 HalISPWdmaTrigger(isp_ioctl_trigger_wdma_attr wdma_attr)
{

	int wdma_buf;

	isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
	infinity3e_reg_isp6 *isp6 = (infinity3e_reg_isp6*)RIUBASE_ISP[6];

	////////////////////////////////////////////////////////// Reset WDMA
	isp6->reg_isp_wdma_en = 0;
	isp6->reg_isp_wdma_wreq_rst = 1;
	isp0->reg_dbgwr_swrst = 0;
	isp6->reg_isp_wdma_wreq_rst = 0;
	isp0->reg_dbgwr_swrst = 1;
	///////////////////////////////////////////////////////////

	isp6->reg_isp_rdma_en=0;
	isp6->reg_isp_wdma_en=0;

	wdma_buf = wdma_attr.buf_addr_phy >> 4;
	isp6->reg_isp_wdma_base = (wdma_buf & 0xffff);
	isp6->reg_isp_wdma_base_1 = (wdma_buf >> 16) & 0x7ff;


	isp0->reg_isp_wdma_crop_start_x = wdma_attr.x;
	isp0->reg_isp_wdma_crop_start_y = wdma_attr.y;
	isp0->reg_isp_wdma_crop_width = wdma_attr.width;
	isp0->reg_isp_wdma_crop_high = wdma_attr.height;

	switch (wdma_attr.wdma_path) {
		case ISP_WDMA_SIF:
			pr_debug("--------ISP_WDMA_SIF--------\n");
			isp0->reg_isp_if_wmux_sel = 0;
			isp0->reg_dspl2wdma_en = 0;

			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS
			isp6->reg_isp_wdma_pitch =  ((wdma_attr.width + 7) >> 3) *1;
			break;
		case ISP_WDMA_ISP:
			pr_debug("--------ISP_WDMA_ISP--------\n");
			isp0->reg_isp_if_wmux_sel = 1;
			isp0->reg_dspl2wdma_en = 0;

			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS
			isp6->reg_isp_wdma_pitch =  ((wdma_attr.width + 7) >> 3) *1;
			break;
		case ISP_WDMA_ISPDS:
			pr_debug("--------ISP_WDMA_ISPDS--------\n");
			isp0->reg_isp_if_wmux_sel = 2;
			isp0->reg_dspl2wdma_en = 1;

			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS
			isp6->reg_isp_wdma_pitch =  ((wdma_attr.width + 7) >> 3) *1;
			break;
		case ISP_WDMA_ISPSWAPYC:
			pr_debug("--------ISP_WDMA_ISPSWAPYC--------\n");
			isp0->reg_isp_if_wmux_sel = 3;
			isp0->reg_dspl2wdma_en = 0;

			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS
			isp6->reg_isp_wdma_pitch =  ((wdma_attr.width + 7) >> 3) *1;
			break;
			// FIXJASON
		case ISP_WDMA_VDOS:
			printk("--------ISP_WDMA_VDOS--------\n");
			HalISPVDOSInit();
			isp0->reg_isp_if_wmux_sel = 4;
			isp0->reg_dspl2wdma_en = 0;

			//isp6->reg_isp_wdma_mode = 0; //PITCH_8BITS
			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS

			break;
		case ISP_WDMA_DEFAULT_SIF: //Use ISP crop range
			pr_debug("--------ISP_WDMA_DEFAULT_SIF--------\n");
			isp0->reg_isp_wdma_crop_start_x = isp0->reg_isp_crop_start_x;
			isp0->reg_isp_wdma_crop_start_y = isp0->reg_isp_crop_start_y;
			isp0->reg_isp_wdma_crop_width = isp0->reg_isp_crop_width;
			isp0->reg_isp_wdma_crop_high = isp0->reg_isp_crop_high;

			isp0->reg_isp_if_wmux_sel = 0;
			isp0->reg_dspl2wdma_en = 0;

			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS
			isp6->reg_isp_wdma_pitch =  ((wdma_attr.width + 7) >> 3) *1;
			break;
		case ISP_WDMA_DEFAULT_YC:  //Use ISP crop range
			pr_debug("--------ISP_WDMA_DEFAULT_YC--------\n");
			isp0->reg_isp_wdma_crop_start_x = 0;
			isp0->reg_isp_wdma_crop_start_y = 0;
			isp0->reg_isp_wdma_crop_width = isp0->reg_isp_crop_width;
			isp0->reg_isp_wdma_crop_high = isp0->reg_isp_crop_high;

			isp0->reg_isp_if_wmux_sel = 3;
			isp0->reg_dspl2wdma_en = 0;

			isp6->reg_isp_wdma_mode = 2; //PITCH_16BITS
			isp6->reg_isp_wdma_pitch =  ((wdma_attr.width + 7) >> 3) *1;
			break;
		default:
			return -1;
	}




	isp6->reg_isp_wdma_auto = 1;

	isp6->reg_isp_wdma_en = 1;


	isp6->reg_isp_wdma_wreq_hpri_set=1;
	isp6->reg_isp_wdma_wreq_thrd = LIMIT_BITS(0x0, 3);
	isp6->reg_isp_wdma_wreq_hpri = LIMIT_BITS(0x1,4);
	isp6->reg_isp_wdma_wreq_max = LIMIT_BITS(0xf, 4);

	isp6->reg_isp_wdma_trigger_mode = 1;
	isp6->reg_isp_wdma_trigger = 1;   //Trigger WDMA

	return 0;
}

///////////////////////////////  CSI //////////////////////////////////
typedef struct
{
	volatile csi_mac *reg;
}hal_csi_handle;

inline void* HalCsi_Open(void* reg_base)
{
	hal_csi_handle *hnd = kzalloc(sizeof(hal_csi_handle),GFP_KERNEL);
	if(hnd)
		hnd->reg = (csi_mac*) reg_base;
	return (void*) hnd;
}

inline void HalCsi_Close(void **handle)
{
	kfree(*handle);
	*handle = NULL;
}

inline void HalCsi_RegInit(void* handle)
{
	hal_csi_handle *hnd = (hal_csi_handle*)handle;
	//enable frame start and virtual channel 0 frame end interrupt
	hnd->reg->reg_mac_rpt_int_mask =  0x7F&(~(CSI_VC0_INT)); //enable vc0 interrupt only
	hnd->reg->reg_rpt_fnum_cond = CSI_RPT_FS;

	//enable virtual channel 0
	hnd->reg->reg_lane_en = CSI_LAN0_EN | CSI_LAN1_EN;
	hnd->reg->reg_vc_en = CSI_VC0_EN;
	hnd->reg->reg_vc0_vs_mode = CSI_V_SYNC_MODE_FE;
	//hnd->reg->reg_vc0_vs_mode = CSI_V_SYNC_MODE_FS;

	//CSI h-sync bug fix
	hnd->reg->reg_vc0_hs_mode = 0x03;

	//CSI pattern gen test setting
	hnd->reg->reg_dt_en.blinking = 0x0;
	//hnd->reg->reg_dt_en.blinking = 0x1;
	hnd->reg->reg_sw_rst = 0;
	hnd->reg->reg_mac_en= 0x1;
	hnd->reg->reg_dont_care_dt = 0x0;
	hnd->reg->reg_mac_err_int_mask = 0x7FFF; //enable/disable csi error report interrupt
}

inline void HalCsi_RegUninit(void* handle)
{
	hal_csi_handle *hnd = (void*)handle;
	//mask all rpt interrupt
	hnd->reg->reg_mac_rpt_int_mask = 0x7F;
	//disable all virtual channels
	hnd->reg->reg_vc_en = 0;
}

inline u32 HalCsi_GetRptIntStatus(void* handle)
{
	hal_csi_handle *hnd = (void*)handle;
	return hnd->reg->reg_mac_rpt_int_src;
}

inline void HalCsi_MaskRptInt(void* handle,u32 mask)
{
	hal_csi_handle *hnd = (void*)handle;
	hnd->reg->reg_mac_rpt_int_mask |= mask;
}

//HAL , clear report interrupt
inline void HalCsi_ClearRptInt(void* handle, u32 clear)
{
	hal_csi_handle *hnd = (void*)handle;
	hnd->reg->reg_mac_rpt_int_clr |=  clear;
	hnd->reg->reg_mac_rpt_int_clr = 0;
	hnd->reg->reg_mac_rpt_int_mask &= ~clear;
}

inline u32 HalCsi_GetErrorIntStatus(void* handle)
{
	hal_csi_handle *hnd = (void*)handle;
	return hnd->reg->reg_mac_err_int_src;
}

inline void HalCsi_MaskErrorInt(void* handle,u32 mask)
{
	hal_csi_handle *hnd = (void*)handle;
	hnd->reg->reg_mac_err_int_mask |= mask;
}

//HAL , clear report interrupt
inline void HalCsi_ClearErrorInt(void* handle, u32 clear)
{
	hal_csi_handle *hnd = (void*)handle;
	hnd->reg->reg_mac_err_int_clr |=  clear;
	hnd->reg->reg_mac_err_int_clr = 0;
	hnd->reg->reg_mac_err_int_mask &= ~clear;
}

inline void HalCsi_ErrIntMaskSet(void* handle, u32 mask)
{
	hal_csi_handle *hnd = (void*)handle;
	hnd->reg->reg_mac_err_int_mask = mask & 0x7FFF;
}

inline u32 HalCsi_ErrIntMaskGet(void* handle)
{
	hal_csi_handle *hnd = (void*)handle;
	return hnd->reg->reg_mac_err_int_mask;
}

u8 * mloadInit(size_t mloadLayoutSize)
{
	return (u8*)dma_alloc_coherent(NULL, mloadLayoutSize, &mload_dma_addr, GFP_KERNEL);
}

void mloadDeInit(void *dmaBuf, size_t mloadLayoutSize)
{
	if(mload_dma_addr)
		dma_free_coherent(NULL, mloadLayoutSize, dmaBuf, mload_dma_addr);
}
