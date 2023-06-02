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
#include "MsTypes.h"
#include "ms_platform.h"

#include "hal_isp.h"
#define PAD16BITS  u32 :16
#define RSVD(ss,ee) u32 rsvd_##ss##_##ee[(ee) - (ss) + 1]

  //////////////////////////isp0//////////////////////////////
  typedef struct {
      u32 reg_en_isp                   :1 ;  //0x00
      u32 reg_en_sensor                :1 ;
      u32 reg_en_isp_obc               :1 ;
      //    h0000 h0000 3 3 reg_en_isp_wbg0 0 0 1 b0  rw  Enable White Balance (before Denoise)
      //    # 1'b0: Disable
      //    # 1'b1: Enable
      //     ( double buffer register ) ISP_TOP 0 1 1 1   r0a0  White
      //    h0000 h0000 4 4 reg_en_isp_wbg1 0 0 1 b0  rw  Enable White Balance (after Denoise)
      //    # 1'b0: Disable
      //    # 1'b1: Enable
      //     ( double buffer register ) ISP_TOP 0 1 1 1   r0a0  White
      u32 reg_en_isp_wbg0              :1; // [3]
      u32 reg_en_isp_wbg1              :1; // [4]
      u32 /* reserved */               :11;
      PAD16BITS;

      u32 reg_isp_sw_rstz               :1 ;  //0x01[0]
      u32 reg_sensor_sw_rstz            :1 ;
      u32 reg_sen_pwdn                  :1 ;
      u32 reg_en_isp_clk_sensor_delay   :1 ;
      u32 reg_en_isp_lsc                :1 ;
      u32                               :2 ;
      u32 reg_en_isp_denoise            :1 ; // [7]
      u32                               :4 ;
      u32 reg_en_sensor_clk             :1 ; // [12]
      u32                               :2 ;
      u32 reg_load_reg                  :1 ; // [15]
      PAD16BITS;

  }  __attribute__((packed, aligned(1))) isp_block_cfg;

  typedef struct {
      u32                                 :14 ;  //0x02
      u32 reg_db_batch_done               :1; // [14]
      u32 reg_db_batch_mode               :1; // [15]
      PAD16BITS;

      u32 reg_fire_mode                   :4 ;  //0x03
      u32 /* reserved */                  :4 ;
      u32 reg_sensor1_rst                 :1 ; // [8]
      u32 reg_sensor1_pwrdn               :1 ;
      u32 reg_sensor2_rst                 :1 ;
      u32 reg_sensor2_pwrdn               :1 ;
      u32 /* reserved */                  :4 ;
      PAD16BITS;
  }  __attribute__((packed, aligned(1))) isp_camsensor_ctl;

  typedef struct {
      u32 reg_isp_if_rmux_sel         :1 /* isp_dp source mux
                                            # 1'b0: from sensor_if
                                            # 1'b1: from RDMA
       */;  //0x04
      u32 /* reserved */              :3 ;
      u32 reg_isp_if_wmux_sel         :3 /* WDMA source mux
                                            # 2'd0: from sensor_if
                                            # 2'd1: from isp_dp output
                                            # 2'd2: from isp_dp down-sample
       */;
      u32 /* reserved */              :1;
      u32 reg_isp_if_rvs_sel          :1 /* isp_dp source mux
                                          # 1'b0: from rmux
                                          # 1'b1: from sensor freerun vs
                                         */;
      u32 reg_isp_icp_ack_tie1        :1 ;
      u32 /* reserved */              :5;
      u32 reg_isp_if_state_rst        :1 ;
      PAD16BITS;


      //0x14
      u32 reg_isp_if_src_sel :1; // isp_if source mux:
      #define ISP0_IF_SRC_PARALLEL  0x0	// 1'b0: from sensor
      #define ISP0_IF_SRC_CSI  0x01			// 1'b1: from csi
      u32 reg_isp2cmdq_vs_sel         :1; // [1]
      u32                             :2;
      u32 reg_isp_if_hs_mode_en       :1 ; /* ISP FIFO handshaking mode enable */
      u32 /* reserved */              :3;
      u32 reg_isp_if_dp_vc            :2 ; /* isp_dp crop virtual channel selection */
      u32 /* reserved */              :2;
      u32 reg_isp_if_wdma_vc          :2 ; /* WDMA crop virtual channel selection */
      u32 /* reserved */              :2;
      PAD16BITS;

      RSVD(0x06, 0x06);

      u32 reg_isp_sensor_mask             :1;  //0x07
      u32 reg_isp_if_de_mode              :1;
      u32                                 :14;
      PAD16BITS;
  }  __attribute__((packed, aligned(1))) isp_src_cfg;

  typedef struct {
      isp_block_cfg               block_cfg;
      isp_camsensor_ctl           camsensor_ctl;
      isp_src_cfg                 src_cfg;
} __attribute__((packed, aligned(1))) isp0_cfg_t;

typedef struct {
    u32 reg_c_irq_mask      :16;//Interrupt Mask
    PAD16BITS;
    u32 reg_c_irq_force     :16;//Force Interrupt Enable
    PAD16BITS;
    u32 reg_c_irq_clr       :16;//Interrupt Clear
    PAD16BITS;
    u32 reg_irq_final_status:16;//Status of Interrupt on CPU side
    PAD16BITS;
    u32 reg_irq_raw_status:16;//Status
    PAD16BITS;
}isp_int1;

typedef struct {
    u32 reg_c_irq_mask:4;
    u32 :4;
    u32 reg_c_irq_force:4;
    u32 :4;
    PAD16BITS;

    u32 reg_c_irq_clr:4;
    u32 :12;
    PAD16BITS;

    u32 reg_irq_final_status:4;
    u32 :12;
    PAD16BITS;

    u32 reg_irq_raw_status:4;
    u32 :12;
    PAD16BITS;

} isp_int2;

typedef struct {
    u32 reg_c_irq_mask:8;        //New Interrupt Mask
    u32 :8;
    PAD16BITS;
    u32 reg_c_irq_force:8;       //New Force Interrupt Enable
    u32 reg_c_irq_clr:8;         //New Interrupt Clear
    PAD16BITS;
    u32 reg_irq_final_status:8;  //New Status of Interrupt on CPU side
    u32 :8;
    PAD16BITS;
    u32 reg_irq_raw_status:8;  //New Status of Interrupt on CPU side
    u32 :8;
    PAD16BITS;
} isp_int3;

//static volatile u32 *isp_cpu_int;
//static volatile u32 *isp_cpu_int2;
//static volatile u32 *isp_cpu_int3;
//
//static volatile u32 *isp_int_mask;
//static volatile u32 *isp_int_mask2;
//static volatile u32 *isp_int_mask3;
//
//static volatile u32 *isp_int_clear;
//static volatile u32 *isp_int_clear2;
//static volatile u32 *isp_int_clear3;

static volatile isp_int1  *pISP_INT1;
static volatile isp_int2  *pISP_INT2;
static volatile isp_int3  *pISP_INT3;

static volatile u32 *isp_vsync_pol;
static volatile u32 *isp_sif;

typedef struct {
    u32 u4Low :16;
    u32       :16;
    u32 u4High:10;
    u32       :22;
} ISP_STS_ADDR_T;

typedef struct {
    ISP_STS_ADDR_T *ae_statis_base ;
    ISP_STS_ADDR_T *awb_statis_base;
    ISP_STS_ADDR_T *dnr_fb_base_0  ;
    ISP_STS_ADDR_T *dnr_fb_base_1  ;
    ISP_STS_ADDR_T *mot_fb_base    ;
} ISP_MIU_STS_BASE;

static volatile ISP_MIU_STS_BASE  isp_miu_bases;
//static volatile u32 *isp_afsts; // 0x35

typedef struct {
    u32 reg_isp_load_sram_id                  :4;//0x00 //Load to the needed sram id
    u32 reg_isp_sram_read_id                  :4;//Read the needed sram data
    u32 reg_isp_sram_rw                       :1;//Read or write srams.  1: write
    u32 reg_isp_miu2sram_en                   :1;//Enable miu loader. 1: enable
    u32 reg_isp_load_register_non_full        :1;//If it is full, the bit is 0  (16 entries)
    u32 reg_isp_load_done                     :1;//load done
    u32 reg_isp_load_wait_hardware_ready_en   :1;//wait hardware not busy
    u32 reg_isp_load_water_level              :2;//wait fifo space == 4, 1: wait space == 8, 2: wait space  == 12, 3: wait fifo empty (16)
    u32 reg_isp_load_sw_rst                   :1;//software reset
    PAD16BITS;

    u32 reg_isp_load_amount                   :16;//0x01 //How many 16-bits data minus one
    PAD16BITS;

    u32 reg_isp_sram_st_addr                  :16;//0x02 //Load data from the sram address
    PAD16BITS;

    u32 reg_isp_load_st_addr_l                :16; //0x03
    PAD16BITS;
    u32 reg_isp_load_st_addr_h                :16; //0x04 //Load data from MIU 16-byte address
    PAD16BITS;

    u32 reg_isp_sram_rd_addr                  :16; //0x05 //Read sram data from set address from set sram
    PAD16BITS;

    u32 reg_isp_sram_read_data                :16; //0x06 //Sram read data from corresponding address and sram
    PAD16BITS;

    u32 reg_isp_load_register_w1r             :1;//0x07 //Fire write register control into register queue
    u32 reg_isp_load_st_w1r                   :1;//Fire load sram
    u32 reg_isp_sram_read_w1r                 :1;//Fire read sram
    u32 reg_isp_load_start_error_clear_w1r    :1;//clear error flag
    u32                                       :12;
    PAD16BITS;

    u32 reg_isp_load_start_error              :1;//0x08 //Frame start when loading is not finished yet
    u32                                       :15;
    PAD16BITS;
} isp4_miu_mload_t;


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

static volatile isp4_miu_mload_t  *isp_miu;

static volatile u32 *isp_vdos_pitch;
static volatile u32 *isp_vdos_addr;

static volatile void *RIUBASE_ISP[8] = {0};

#if 1
static volatile u32 u4DefDisable[3] =
{
    ISP_SHIFTBITS(INT_DB_UPDATE_DONE)|ISP_SHIFTBITS(INT_SR_VREF_RISING)|ISP_SHIFTBITS(INT_SR_VREF_FALLING)|ISP_SHIFTBITS(INT_ISP_BUSY)|ISP_SHIFTBITS(INT_ISP_IDLE),
    ISP_SHIFTBITS(INT2_VDOS_EVERYLINE)|ISP_SHIFTBITS(INT2_VDOS_LINE0)|ISP_SHIFTBITS(INT2_VDOS_LINE1),
    ISP_SHIFTBITS(INT3_AE_WIN1_DONE)|ISP_SHIFTBITS(INT3_MENULOAD_DONE)|ISP_SHIFTBITS(INT3_SW_INT_INPUT_DONE)|ISP_SHIFTBITS(INT3_SW_INT_OUTPUT_DONE)
};
#else
static volatile u32 u4DefDisable[3] =
{
    ISP_SHIFTBITS(INT_DB_UPDATE_DONE)|ISP_SHIFTBITS(INT_SR_VREF_RISING)|ISP_SHIFTBITS(INT_SR_VREF_FALLING)
    |ISP_SHIFTBITS(INT_PAD_VSYNC_RISING)|ISP_SHIFTBITS(INT_PAD_VSYNC_FALLING)
    |ISP_SHIFTBITS(INT_ISP_FIFO_FULL)|ISP_SHIFTBITS(INT_ISP_BUSY) |ISP_SHIFTBITS(INT_ISP_IDLE)
    |ISP_SHIFTBITS(INT_WDMA_DONE)|ISP_SHIFTBITS(INT_RDMA_DONE) |ISP_SHIFTBITS(INT_WDMA_FIFO_FULL)
    |ISP_SHIFTBITS(INT_AF_DONE)|ISP_SHIFTBITS(INT_AWB_DONE) |ISP_SHIFTBITS(INT_AE_DONE)
    |ISP_SHIFTBITS(INT_ISPIF_VSYNC)
    ,
    ISP_SHIFTBITS(INT2_VDOS_EVERYLINE)|ISP_SHIFTBITS(INT2_VDOS_LINE0)|ISP_SHIFTBITS(INT2_VDOS_LINE1)|ISP_SHIFTBITS(INT2_VDOS_LINE2),
    ISP_SHIFTBITS(INT3_AE_WIN0_DONE)|ISP_SHIFTBITS(INT3_AE_WIN1_DONE)|ISP_SHIFTBITS(INT3_MENULOAD_DONE)|ISP_SHIFTBITS(INT3_SW_INT_INPUT_DONE)|ISP_SHIFTBITS(INT3_SW_INT_OUTPUT_DONE)
};
#endif

inline void HalInitRegs(void **pRegs)
{
    int i = 0;
    for (i = 0; i < 8; i++)
        RIUBASE_ISP[i] = (void*)pRegs[i];

    // disable double buffer
    *(volatile u32*)(RIUBASE_ISP[0] + 0x1*4) |= (1<<15);
    // init frame cnt
    *(volatile u32*)(RIUBASE_ISP[0] + 0x7a*4) = 0;

    // structured INT
    pISP_INT1 = (volatile isp_int1*)(RIUBASE_ISP[0] + 0x28*4);
    pISP_INT2 = (volatile isp_int2*)(RIUBASE_ISP[0] + 0x2d*4);
    pISP_INT3 = (volatile isp_int3*)(RIUBASE_ISP[0] + 0x25*4);

    // polarity & interface
    isp_vsync_pol =
        (volatile u32*)(RIUBASE_ISP[0] + 0x10*4);
    isp_sif =
        (volatile u32*)(RIUBASE_ISP[0] + 0x05*4);

  // statistics output address
  isp_miu_bases.ae_statis_base = (ISP_STS_ADDR_T *)(RIUBASE_ISP[3] + 0x06*4);
  isp_miu_bases.awb_statis_base = (ISP_STS_ADDR_T *)(RIUBASE_ISP[3] + 0x08*4);
  isp_miu_bases.dnr_fb_base_0 = (ISP_STS_ADDR_T *)(RIUBASE_ISP[3] + 0x38*4);
  isp_miu_bases.dnr_fb_base_1 = (ISP_STS_ADDR_T *)(RIUBASE_ISP[3] + 0x3a*4);
  isp_miu_bases.mot_fb_base = (ISP_STS_ADDR_T *)(RIUBASE_ISP[3] + 0x3e*4);
  //isp_afsts = (volatile u32*)(RIUBASE_ISP[5] + 0x18*4); // 0x35

  // menuload
  isp_miu = (isp4_miu_mload_t  *)(RIUBASE_ISP[4]);

  // vdos
  isp_vdos_pitch = (volatile u32*)(RIUBASE_ISP[6] + 0x21*4);
  isp_vdos_addr = (volatile u32*)(RIUBASE_ISP[6] + 0x28*4);

  // disable clock gating
  (*(u16*)(RIUBASE_ISP[0] + 0x64*4)) &= ~0x0004;

}

inline void HalISPDisableInt()
{
    pISP_INT1->reg_c_irq_mask  = 0xffff;
    pISP_INT2->reg_c_irq_mask  = 0x000f;
    pISP_INT3->reg_c_irq_mask  = 0x00ff;
}

inline void HalISPEnableInt()
{
    pISP_INT1->reg_c_irq_mask  = u4DefDisable[0];
    pISP_INT2->reg_c_irq_mask  = u4DefDisable[1];
    pISP_INT3->reg_c_irq_mask  = u4DefDisable[2];

    pISP_INT1->reg_c_irq_clr = 0xffff & ~(u4DefDisable[0]);
    pISP_INT1->reg_c_irq_clr = 0;
    pISP_INT2->reg_c_irq_clr = 0x00ff & ~(u4DefDisable[1]);
    pISP_INT2->reg_c_irq_clr = 0;
    pISP_INT3->reg_c_irq_clr = 0x00ff & ~(u4DefDisable[2]);
    pISP_INT3->reg_c_irq_clr = 0;

}

inline void HalISPMaskIntAll()
{
    pISP_INT1->reg_c_irq_mask  = 0xffff;
    pISP_INT2->reg_c_irq_mask  = 0x000f;
    pISP_INT3->reg_c_irq_mask  = 0x00ff;

    pISP_INT1->reg_c_irq_clr = 0xffff & ~(u4DefDisable[0]);
    pISP_INT2->reg_c_irq_clr = 0x000f & ~(u4DefDisable[1]);
    pISP_INT3->reg_c_irq_clr = 0x003f & ~(u4DefDisable[2]);
}

inline u32 HalISPGetIntStatus1()
{
    return (~pISP_INT1->reg_c_irq_mask) & pISP_INT1->reg_irq_final_status;
}

inline u32 HalISPGetIntStatus2()
{
    return (~pISP_INT2->reg_c_irq_mask) & pISP_INT2->reg_irq_final_status;
}

inline u32 HalISPGetIntStatus3()
{
    return (~pISP_INT3->reg_c_irq_mask) & pISP_INT3->reg_irq_final_status;
}

inline void HalISPMaskClear1(u32 clear)
{
    pISP_INT1->reg_c_irq_clr |= clear;
    pISP_INT1->reg_c_irq_clr = 0;
    //pISP_INT1->reg_c_irq_mask &= ~(clear & ~u4DefDisable[0]);
    pISP_INT1->reg_c_irq_mask = u4DefDisable[0];
}

inline void HalISPMaskClear2(u32 clear)
{
    pISP_INT2->reg_c_irq_clr |= clear;
    pISP_INT2->reg_c_irq_clr = 0;
    //pISP_INT2->reg_c_irq_mask &= ~(clear & ~u4DefDisable[1]);
    pISP_INT2->reg_c_irq_mask = u4DefDisable[1];
}

inline void HalISPMaskClear3(u32 clear)
{
    pISP_INT3->reg_c_irq_clr |= clear;
    pISP_INT3->reg_c_irq_clr = 0;
    //pISP_INT3->reg_c_irq_mask &= ~(clear & ~u4DefDisable[2]);
    pISP_INT3->reg_c_irq_mask = u4DefDisable[2];
}

inline void HalISPMaskInt1(u32 mask)
{
    pISP_INT1->reg_c_irq_mask |= (mask|u4DefDisable[0]);
}

inline void HalISPMaskInt2(u32 mask)
{
    pISP_INT2->reg_c_irq_mask |= (mask|u4DefDisable[1]);
}

inline void HalISPMaskInt3(u32 mask)
{
    pISP_INT3->reg_c_irq_mask |= (mask|u4DefDisable[2]);
}

inline void HalISPClearInt(volatile u32 *clear, ISP_INT_STATUS int_num)
{
    *clear |= (0x01<<int_num);
}

inline void HalISPClearIntAll()
{
    pISP_INT1->reg_c_irq_mask = pISP_INT1->reg_c_irq_clr;
    pISP_INT1->reg_c_irq_clr = 0;
    pISP_INT2->reg_c_irq_mask = pISP_INT2->reg_c_irq_clr;
    pISP_INT2->reg_c_irq_clr = 0;
    pISP_INT3->reg_c_irq_mask = pISP_INT3->reg_c_irq_clr;
    pISP_INT3->reg_c_irq_clr = 0;
}

inline void HalISPClkEnable(bool enable)
{
    if (enable == false)
        *(volatile u32*)(RIUBASE_ISP[0] + 0x66*4) |=  (u16)0x04; // 0x35
    else
        *(volatile u32*)(RIUBASE_ISP[0] + 0x66*4) &= ~((u16)0x04);
}

inline u32 HalISPGetVsyncPol()
{
    return (((*isp_vsync_pol) >> VSYNC_BIT) & 0x01);
}

inline u32 HalISPGetMIPI(void)
{
    return (*isp_sif) & 0x01;
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

inline void HalISPSetAeBaseAddr(volatile unsigned long Addr)
{
    Addr = Chip_Phys_to_MIU(Addr);
    isp_miu_bases.ae_statis_base->u4Low = Addr>>4;
    isp_miu_bases.ae_statis_base->u4High = Addr>>20;

    pr_debug("[ISP] AeBaseAddr 0x%08x, H:0x%04x, L:0x%04x\n", (unsigned int)Addr, (unsigned int)isp_miu_bases.ae_statis_base->u4High,
        (unsigned int)isp_miu_bases.ae_statis_base->u4Low);
}

inline void HalISPSetAwbBaseAddr(volatile unsigned long Addr)
{
    Addr = Chip_Phys_to_MIU(Addr);
    isp_miu_bases.awb_statis_base->u4Low = Addr>>4;
    isp_miu_bases.awb_statis_base->u4High = Addr>>20;

    pr_debug("[ISP] AwbBaseAddr 0x%08x, H:0x%04x, L:0x%04x\n", (unsigned int)Addr, (unsigned int)isp_miu_bases.awb_statis_base->u4High,
        (unsigned int)isp_miu_bases.awb_statis_base->u4Low);
}

//set motion detection statisic buffer address
inline void HalISPSetMotBaseAddr(volatile unsigned long Addr)
{
    if(Addr & 0x0F)
    {
        pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
    }
    Addr = Chip_Phys_to_MIU(Addr);
    isp_miu_bases.mot_fb_base->u4Low = Addr>>4;
    isp_miu_bases.mot_fb_base->u4High = Addr>>20;
    pr_debug("set motion base addr = 0x%X\n",(uint32_t)Addr);
}

inline void HalISPSetDnrFbAddr(unsigned long Addr, int id)
{
    volatile ISP_STS_ADDR_T *base=0;
    if(Addr & 0x0F){
        pr_err("%s : Buffer address is not 16 bytes aligment.\r\n",__FUNCTION__);
    }
    Addr = Chip_Phys_to_MIU(Addr);
    if(id==0)
        base = isp_miu_bases.dnr_fb_base_0;
    else if(id==1)
        base = isp_miu_bases.dnr_fb_base_1;

    base->u4Low = (Addr>>4) & 0xFFFF;
    base->u4High = (Addr>>20) & 0xFFF;

    pr_debug("set DNR fb addr = 0x%X\n",(uint32_t)Addr);
}


//Loading Data From DRAM to SRAMs
inline void HalISPMLoadWriteData(volatile unsigned int Sram_Id, volatile unsigned long Addr)
{
    Addr = Chip_Phys_to_MIU(Addr);

    printk(KERN_INFO "[ISP] HalISPMLoadWriteData addr:0x%08x\n", (u32)Addr);

    //Enable the engine by turning on the register
    isp_miu->reg_isp_miu2sram_en = 1;
    //Set SW reset as 0 and 1
    isp_miu->reg_isp_load_sw_rst = 0;
    isp_miu->reg_isp_load_sw_rst = 1;
    //Set SW read write mode as 1 (write)
    isp_miu->reg_isp_sram_rw     = 1;// 1:write
    //Set SRAM ID
    isp_miu->reg_isp_load_sram_id = Sram_Id;
    //Set MIU 16-byte start address
    isp_miu->reg_isp_load_st_addr_l = (Addr>>4) & 0xFFFF;
    isp_miu->reg_isp_load_st_addr_h = (Addr>>20) & 0xFFFF;
    //Set data amount (2-byte)
    //The number should be set as length - 1
    isp_miu->reg_isp_load_amount    = IQ_LEN_ALSC_GAIN - 1;
    //Set destination SRAM start SRAM address (2-byte)
    isp_miu->reg_isp_sram_st_addr   = 0x0000;
    //Set SRAM loading parameter by setting write-one-clear
    isp_miu->reg_isp_load_register_w1r = 1;
    isp_miu->reg_isp_load_register_w1r = 0;
    //Fire Loading by setting write-one-clear
    isp_miu->reg_isp_load_st_w1r = 1;


}

//Loading All table From DRAM to SRAMs
inline void HalISPMLoadWriteAllTable(volatile unsigned long Addr, volatile unsigned long *offset)
{

    u32 pTableSize[IQ_MEM_NUM] = {IQ_LEN_ALSC_GAIN, IQ_LEN_ALSC_GAIN, IQ_LEN_ALSC_GAIN,
                                  IQ_LEN_GAMMA_12TO10, IQ_LEN_GAMMA_12TO10, IQ_LEN_GAMMA_12TO10,
                                  IQ_LEN_DEFECT_PIXEL,
                                  IQ_LEN_GAMMA_10TO12, IQ_LEN_GAMMA_10TO12, IQ_LEN_GAMMA_10TO12,
                                  IQ_LEN_GAMMA_CORRECT, IQ_LEN_GAMMA_CORRECT, IQ_LEN_GAMMA_CORRECT};

    u32 i, k;
    unsigned long tbl_addr;

    Addr = Chip_Phys_to_MIU(Addr);

    //Enable the engine by turning on the register
    isp_miu->reg_isp_miu2sram_en = 1;
    //Set SW reset as 0 and 1
    isp_miu->reg_isp_load_sw_rst = 0; //this register is active low, set 0 to reset
    isp_miu->reg_isp_load_sw_rst = 1;
    //Set SW read write mode as 1 (write)

    for(i = 0; i < IQ_MEM_NUM; i++) {
        tbl_addr = Addr + offset[i];

        isp_miu->reg_isp_sram_rw     = 1;// 1:write
        //Set SRAM ID
        isp_miu->reg_isp_load_sram_id = i;
        //Set MIU 16-byte start address
        isp_miu->reg_isp_load_st_addr_l = (tbl_addr>>4) & 0xFFFF;
        isp_miu->reg_isp_load_st_addr_h = (tbl_addr>>20) & 0xFFFF;
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

}

//Read one u16 Data From SRAMs for debug
inline void HalISPMLoadReadData(volatile unsigned int Sram_Id, volatile unsigned short Offset, volatile unsigned short *Data)
{
    //Enable the engine by turning on the register
    isp_miu->reg_isp_miu2sram_en = 1;

    //Set sram read write mode to 0 (read)
    isp_miu->reg_isp_sram_rw     = 0;
    //Set SRAM ID
    isp_miu->reg_isp_load_sram_id = Sram_Id;

    //Set destination SRAM read 2-byte address
    isp_miu->reg_isp_sram_rd_addr   = Offset;

    //Fire data read by setting write-one-clear
    isp_miu->reg_isp_load_register_w1r = 1;
    isp_miu->reg_isp_load_register_w1r = 0;
    //Fire again due to CDC problems
    isp_miu->reg_isp_sram_read_w1r = 1;

    *Data = isp_miu->reg_isp_sram_read_data;

}

inline u8 HalISPGetFrameDoneCount(void)
{
    u16 val = *(u16*)(RIUBASE_ISP[0] + 0x7A*4);
    return  (val >> 8) & 0x7F;
}

inline void IspReset(void)
{
    isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
    isp0->block_cfg.reg_isp_sw_rstz = 0;
    isp0->block_cfg.reg_sensor_sw_rstz = 0;
    udelay(1);
    isp0->block_cfg.reg_sensor_sw_rstz = 1;
    isp0->block_cfg.reg_isp_sw_rstz = 1;
}

inline void IspDisable(void)
{
	*(volatile u32*)(RIUBASE_ISP[0]) = 0;
	IspReset();
}

inline void IspInputEnable(u32 enable)
{
    isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
    isp0->src_cfg.reg_isp_sensor_mask = enable?0:1;
}

inline void IspAsyncEnable(u32 enable)
{
    isp0_cfg_t *isp0 = (isp0_cfg_t*)RIUBASE_ISP[0];
    isp0->src_cfg.reg_isp_icp_ack_tie1 = enable?1:0;
}

inline void HalISPSetOBC(int u4OBC_a, int u4OBC_b)
{
    *(volatile u32*)(RIUBASE_ISP[1] + 0x00*4) &= ~(0x0ff);
    *(volatile u32*)(RIUBASE_ISP[1] + 0x00*4) |= (u4OBC_a&(0x0ff));
    *(volatile u32*)(RIUBASE_ISP[1] + 0x05*4) &= ~(0x0fffff);
    *(volatile u32*)(RIUBASE_ISP[1] + 0x05*4) |= (u4OBC_b&(0x0fffff));
}

typedef struct {
    u32 val                             :13; //0x32
    u32 /* reserved */                  :3;
    u32 padding                         :16;
} __attribute__((packed, aligned(1))) isp_ccm_coeff;

void HalISPSetRGBCCM(const s16 *ccm_coeff)
{
	int n=0;
	volatile isp_ccm_coeff* coeff = (volatile isp_ccm_coeff*)(((char*)RIUBASE_ISP[0]) + 0x33*4);
	for(n=0;n<9;++n)
	{
		coeff[n].val = ccm_coeff[n]&0x1FFF;
	}
}

void HalISPSetYUVCCM(const s16 *ccm_coeff)
{
	int n=0;
	volatile isp_ccm_coeff* coeff = (volatile isp_ccm_coeff*)(((char*)RIUBASE_ISP[0]) + 0x3d*4);
	for(n=0;n<9;++n)
	{
		coeff[n].val = ccm_coeff[n]&0x1FFF;
	}
}

///////////////////////////////  CSI //////////////////////////////////
typedef struct
{
    csi_mac *reg;
}hal_csi_handle;

inline void* HalCsi_Open(void* reg_base)
{
    hal_csi_handle *hnd = kzalloc(sizeof(hal_csi_handle),GFP_KERNEL);
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
#if 1
    hal_csi_handle *hnd = (hal_csi_handle*)handle;
    //enable frame start and virtual channel 0 frame end interrupt
    //hnd->reg->reg_mac_rpt_int_mask =  0x7F&(~(CSI_FRAME_INT|CSI_VC0_INT));
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
#endif

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
