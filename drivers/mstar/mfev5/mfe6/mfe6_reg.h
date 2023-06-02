
#ifndef _MFE6_REG_H_
#define _MFE6_REG_H_

#include <mhve_ios.h>

// IRQ's
#define IRQ_LESS_ROW_DONE       7
#define IRQ_NET_TRIGGER         6
#define IRQ_FS_FAIL             5
#define IRQ_TXIP_TIME_OUT       4
#define IRQ_BSPOBUF_FULL        3
#define IRQ_IMG_BUF_FULL        2
#define IRQ_MARB_BSPOBUF_FULL   1
#define IRQ_FRAME_DONE          0

#define CHECK_IRQ_STATUS(r,irq) ((r>>irq)&1)

typedef struct mfe6_reg {
    mhve_job    mjob;
    /* sw-coded data followed by mfe-output stream */
    void* coded_data;
    int   coded_bits;
    uchar bits_coded[256];
    int   bits_count;
    int   bits_delta;
    /* miu address for mfe-hw */
    uint  outbs_addr;
    int   outbs_size;
    /* ticks for profiling */
    uint  enc_cycles;
    uint  enc_bitcnt;
    uint  enc_sumpqs;
    /* mfe-regs bank definition */
    union {
        struct {
        ushort reg00_g_frame_start_sw:1;    // frame start (1T clk_mfe)
        ushort reg00_g_soft_rstz:1;         // software reset; 0/1: reset/not reset
        ushort reg00_g_enc_mode:2;          // 0/1/2/3: MPEG4/H263/H264/JPEG
  #define MFE_REG_ENC_MPG4          0
  #define MFE_REG_ENC_H263          1
  #define MFE_REG_ENC_H264          2
  #define MFE_REG_ENC_JPEG          3
        ushort reg00_g_frame_type:2;        // 0/1/2: I/P/B
  #define MFE_REG_ITYPE             0
  #define MFE_REG_PTYPE             1
  #define MFE_REG_BTYPE             2
        ushort reg00_g_ref_no:1;            // 0/1: 1 frame/2 frames
  #define MFE_REG_REF_NUM_ONE       0
  #define MFE_REG_REF_NUM_TWO       1
        ushort reg00_g_mbr_en:1;            // 0/1: disable/enable MB-level Rate control
        ushort reg00_g_qscale:6;            // frame level qscale: [H264]: 1 ~ 51; [MPEG4]: 1 ~ 31
        ushort reg00_g_rec_en:1;            // reconstruct enable
        ushort reg00_g_jpe_mst422_mode:1;   // YUV422 input buffer format; 0: YUYV, 1: MST422
        };
        ushort reg00;
    };
    union {
        struct {
        ushort reg01_g_pic_width:16;    // picture width
        };
        ushort reg01;
    };
    union {
        struct {
        ushort reg02_g_pic_height:16;   // picture height
        };
        ushort reg02;
    };
    union {
        struct {
        ushort reg03_g_er_mode:2;       // 0/1/2/3: mby/bs/mby+bs/off
        ushort reg03_g_er_mby:2;        // 0/1/2/3: every 1/2/4/8 mb row(s) (error resilence)
        ushort reg03_g_packed_mode:1;   // frame buffer format for 422 packed mode; 0/1: YVYU/YUYV
        ushort reg03_g_qmode:1;         // quantization method; 0/1: h263/mp4
        ushort reg03_g_tbc_mode:1;      // table mode; 0: SW control, 1: HW control
        ushort reg03_g_fldpic_en:1;     // field picture coding
        ushort reg03_g_dct_only_en:1;   // regmfe_g_dct_only_en
        ushort reg03_g_mstar_tile:1;
        ushort reg03_g_mstar_tile_field_split:1;
        ushort reg03_g_fldpic_idx:1;
        ushort reg03_g_fldpic_multislice_en:1;
        ushort reg03_g_cabac_en:1;
        ushort reg03_non_secure:1;
        };
        ushort reg03;
    };
    union {
        struct {
        ushort reg04_g_er_bs_th:16;         // er_bs mode threshold
        };
        ushort reg04;
    };
    union {
        struct {
        ushort reg05_g_inter_pref:16;       // inter prediction preference
        };
        ushort reg05;
    };
    union {
        struct {
        ushort reg06_g_cur_y_addr_lo:16;    // current luma base address
        };
        ushort reg06;
    };
    union {
        struct {
        ushort reg07_g_cur_y_addr_hi:8;     // current luma base address
        };
        ushort reg07;
    };
    union {
        struct {
        ushort reg08_g_cur_c_addr_lo:16;    // current chroma base address
        };
        ushort reg08;
    };
    union {
        struct {
        ushort reg09_g_cur_c_addr_hi:8;     // current chroma base address
        };
        ushort reg09;
    };
    union {
        struct {
        ushort reg0a_g_ref_y_addr0_lo:16;   // reference luma base address0
        };
        ushort reg0a;
    };
    union {
        struct {
        ushort reg0b_g_ref_y_addr0_hi:8;    // reference luma base address0
        };
        ushort reg0b;
    };
    union {
        struct {
        ushort reg0c_g_ref_y_addr1_lo:16;   // reference luma base address1
        };
        ushort reg0c;
    };
    union {
    struct {
        ushort reg0d_g_ref_y_addr1_hi:8;    // reference luma base address0
        };
        ushort reg0d;
    };
    union {
        struct {
        ushort reg0e_g_ref_c_addr0_lo:16;   // reference chroma base address0
        };
        ushort reg0e;
    };
    union {
        struct {
        ushort reg0f_g_ref_c_addr0_hi:8;    // reference chroma base address0
        };
        ushort reg0f;
    };
    union {
        struct {
        ushort reg10_g_ref_c_addr1_lo:16;   // reference chroma base address1
        };
        ushort reg10;
    };
    union {
        struct {
        ushort reg11_g_ref_c_addr1_hi:8;    // reference chroma base address1
        };
        ushort reg11;
    };
    union {
        struct {
        ushort reg12_g_rec_y_addr_lo:16;    // reconstructed luma base address
        };
        ushort reg12;
    };
    union {
        struct {
        ushort reg13_g_rec_y_addr_hi:8;     // reconstructed luma base address
        };
        ushort reg13;
    };
    union {
        struct {
        ushort reg14_g_rec_c_addr_lo:16;    // reconstructed chroma base address
        };
        ushort reg14;
    };
    union {
        struct {
        ushort reg15_g_rec_c_addr_hi:8;     // reconstructed chroma base address
        };
        ushort reg15;
    };
    union {
        struct {    // clock gating
        ushort gate_cry_crc_sram:1;
        ushort gate_qtab_dbfdc_dbqtb_sram:1;
        ushort gate_mcy_mcc_sram:1;
        ushort gate_res0_res1_sram:1;
        ushort gate_ieap:1;
        ushort gate_dct_idct:1;
        ushort gate_dbf:1;
        };
        ushort reg16;
    };
    union {
        struct {
        ushort reg17_s_auto_rst_wait_cnt:6;     // the waiting count for regen_soft_rstz and regen_fs_sw generation
        ushort reg17_g_sram1p_wp_type:1;        // "MFE 1p SRAM wrapper Type  'b1: Fix write-through problem 'b0: Original"
        ushort reg17_g_sram2p_wp_type:1;        // "MFE 2p SRAM wrapper Type  'b1: Fix write-through problem 'b0: Original"
        ushort reg17_g_clk_mfe_en:4;            // NOT used now.
        ushort reg17_g_mreq_sel:1;              // "1" D1 MIU clk gating; "0" dynamic MIU clk gating
        ushort reg17_g_mreq_always_active:1;    // "1" mreq always active; "0" make mreq active according to FSM. (let this be default)
        ushort reg17_g_clk_miu_d2_gate:1;       // b1: turn off miu clock of power-domain "dma" and sleep into d2 mode
        ushort reg17_g_clk_mfe_d2_gate:1;       // b1:  turn off mfe clock of power-domain "core" and sleep into d2 mode
        };
        ushort reg17;
    };
    // [JPEG]
    union {
        struct {
        ushort reg18_g_jpe_enc_mode:2;          // JPE encode mode; 2'b00/2'b01/2'b10/2'b11: 420/422/444/gray; current version supports 422 only
        ushort reg18_g_jpe_buffer_mode:1;       // JPE buffer mode; 0/1: double buffer mode/frame buffer mode
        ushort reg18_g_jpe_multibuf_mode:2;     // JPE multi-buffer mode; 0/1/2: 2/4/8 buffers
        ushort reg18_g_jpe_qfactor:4;           // JPE q factor; 0 ~ 15: (1 ~ 16)/4
        // (M1)JPE fsvs generation mode;
        // 0: pure sw
        // 1: sw+hw
        // 2: hw w/o auto-restart
        // 3: hw w/i auto-restart
        ushort reg18_g_jpe_fsvs_mode:2;         // (T8)JPE fsvs generation mode; 0/1/2: pure sw/sw+hw/hw
        ushort reg18_reserved:3;
        ushort reg18_g_jpe_turbo_en:1;          // 0: turbo mode off, 1: turbo mode enabled
        ushort reg18_g_viu_soft_rstz:1;         // viu software reset; 0/1: reset/not reset
        };
        ushort reg18;
    };
    // [MPEG4/H263]
    union {
        struct {
        ushort reg19_g_mp4_itlc:1;              // 0/1: MPEG4 progressive/interlaced mode
        ushort reg19_g_mp4_pskip_off:1;         // 0/1: MPEG4 enable/disable p skip mode
        ushort reg19_g_mp4_acp:2;               // [0]: 0/1: sw/hw acp selection; [1]: sw default value: 0/1: disable/enable acp; current version off
        ushort reg19_g_mp4_rounding_ctrl:1;     // mp4 rounding control specified as in spec
        ushort reg19_g_er_hec:1;                // 0/1: header extension code off/on
        ushort reg19_g_er_hec_t:3;              // HEC counter reset values
        ushort reg19_g_er_h263_unit:2;          // 0/1/2: unit is 1/2/4, for calculating gob_num.
        ushort reg19_g_mp4_direct_en:1;         // MPEG4 direct enable
        ushort reg19_g_mp4_direct_mvstore:1;    // [M]: enable storing of mv & skip_mb information to DRAM in P(or sometimes I) frame
        };
        ushort reg19;
    };
    union {
        struct {
        ushort reg1a_g_mp4_direct_pref:8;       // used in mp4 only, mp4 direct mode preference value
        ushort reg1a_g_mp4_direct_trb:3;        // used in mp4 only, mp4 direct mode trb (P0-B distance)
        ushort reg1a_g_mp4_direct_trd:3;        // used in mp4 only, mp4 direct mode trd (P0-P1 distance)
        };
        ushort reg1a;
    };
    union {
        struct {
        ushort reg1b_g_mp4_flddct_diff_thr:8;   // used in mp4 only, mp4 field dct difference threshold
        ushort reg1b_g_mp4_flddct_en:1;         // used in mp4 only, mp4 field dct enable
        };
        ushort reg1b;
    };
    // [IRQ & important IP status checkings]
    union {
        struct {
        ushort reg1c_g_irq_mask:8;              // 0/1: irq not-mask/mask
        ushort reg1c_g_irq_force:8;             // 0/1: set corresponding interrupt as usual/force corresponding interrupt
        };
        ushort reg1c;
    };
    union {
        struct {
        ushort reg1d_g_irq_clr0:1;              // 0/1: not clear interrupt/clear interrupt 0
        ushort reg1d_g_irq_clr1:1;              // 0/1: not clear interrupt/clear interrupt 1
        ushort reg1d_g_irq_clr2:1;              // 0/1: not clear interrupt/clear interrupt 2
        ushort reg1d_g_irq_clr3:1;              // 0/1: not clear interrupt/clear interrupt 3
        ushort reg1d_g_irq_clr4:1;              // 0/1: not clear interrupt/clear interrupt 4
        ushort reg1d_g_irq_clr5:1;              // 0/1: not clear interrupt/clear interrupt 5
        ushort reg1d_g_irq_clr6:1;              // 0/1: not clear interrupt/clear interrupt 6
        ushort reg1d_g_irq_clr7:1;              // 0/1: not clear interrupt/clear interrupt 7
        ushort reg1d_g_swrst_safe:1;            // to indicate there're no miu activities that need to pay attention to
        };
        ushort reg1d;
    };
    union {
        struct {
        // status of interrupt on CPU side
        // ({1'b0,net_trigger,fs_fail_irq, txip_time_out,early_bspobuf_full_irq/buf1_full,img_buf_full_irq,marb_bspobuf_ful/buf0_full,frame_done_irq})
        // [3] SW mode: early obuf full; HW mode: buf1 full
        // [1] SW mode: buf full; HW mode: buf0 full
        ushort reg1e_g_irq_cpu:8;
        // status of interrupt on IP side
        // ({1'b0,net_trigger,fs_fail_irq, txip_time_out,early_bspobuf_full_irq/buf1_full,img_buf_full_irq,marb_bspobuf_ful/buf0_full,frame_done_irq})
        // [3] SW mode: early obuf full; HW mode: buf1 full
        // [1] SW mode: buf full; HW mode: buf0 full
        ushort reg1e_g_irq_ip:8;
        };
        ushort reg1e;
    };
    union {
        struct {
        ushort reserved_reg1f;
        };
        ushort reg1f;
    };
    // [ME setting]
    union {
        struct {
        ushort reg20_s_me_4x4_disable:1;        // 4x4_disable
        ushort reg20_s_me_8x4_disable:1;        // 8x4_disable
        ushort reg20_s_me_4x8_disable:1;        // 4x8_disable
        ushort reg20_s_me_16x8_disable:1;       // 16x8_disable
        ushort reg20_s_me_8x16_disable:1;       // 8x16_disable
        ushort reg20_s_me_8x8_disable:1;        // 8x8_disable
        ushort reg20_s_me_16x16_disable:1;      // 16x16_disable
        ushort reg20_s_mesr_adapt:1;            // me search range auto-adaptive; 0/1: off/on
        ushort reg20_s_me_ref_en_mode:2;        // ref enable mode: 2'b01/2'b10/2'b11: ref0 enable/ref1 enable/ref0&1 enable
        };
        ushort reg20;
    };
    // [IME PIPELINE]
    union {
        struct {
        ushort reg21_s_ime_sr16:1;              // search range limited to (h,v) = (+/-16, +/-16); 0/1: search range 32/16
        ushort reg21_s_ime_umv_disable:1;       // 0/1: UMV enable/disable
        ushort reg21_s_ime_ime_wait_fme:1;      // 0/1: ime wait fme/fme wait ime
        ushort reg21_s_ime_boundrect_en:1;      // ime bounding rectangle enable (needed for level 3.0 and below)
        ushort reg21_s_ime_h264_p8x8_ctrl_en:1; // ime h264 max p8x8 count control enable
        ushort reg21_reserved:3;
        ushort reg21_s_ime_h264_p8x8_max:8;     // ime h264 max p8x8 count; value 0 is prohibited
                                                // Max P8x8 MB count = 16 * reg21_s_ime_h264_p8x8_max
        };
        ushort reg21;
    };
    union {
        struct {
        ushort reg22_s_ime_mesr_max_addr:8;     // me search range max depth
        ushort reg22_s_ime_mesr_min_addr:8;     // me search range min depth
        };
        ushort reg22;
    };
    union {
        struct {
        ushort reg23_s_ime_mvx_min:6;           // me mvx min; 0/.../62 --> -32/.../30
        ushort reg23_reserved:2;
        ushort reg23_s_ime_mvx_max:6;           // me mvx max; 0/.../62 --> -32/.../30
        };
        ushort reg23;
    };
    union {
        struct {
        ushort reg24_s_ime_mvy_min:6;           // me mvy min; 0/.../62 --> -32/.../30
        ushort reg24_reserved:2;
        ushort reg24_s_ime_mvy_max:6;           // me mvy max; 0/.../62 --> -32/.../30
        };
        ushort reg24;
    };
    // [FME pipeline]
    union {
        struct {
        ushort reg25_s_fme_quarter_disable:1;   // 0/1: Quarter fine-tune enable/disable
        ushort reg25_s_fme_half_disable:1;      // 0/1: Half fine-tune enable/disable
        ushort reg25_reserved:1;
        ushort reg25_s_fme_pmv_enable:1;        // 0/1: disable/enable Previous Skip MV mode
        ushort reg25_s_fme_mode_no:1;           // 0: one mode.  1: two mode.
        ushort reg25_s_fme_mode0_refno:1;       // 0: one ref. for mode0  1: two ref. for mode0
        ushort reg25_s_fme_mode1_refno:1;       // 0: one ref. for mode1  1: two ref. for mode1
        ushort reg25_s_fme_mode2_refno:1;       // 0: one ref. for mode2  1: two ref. for mode2
        ushort reg25_s_fme_skip:1;              // fme skip
        ushort reg25_s_fme_pipeline_on:1;       // 0/1: FME pipeline off/on
        };
        ushort reg25;
    };
    // MBR
    union {
        struct {
        ushort reg26_s_mbr_pqp_dlimit:2;        // previous qp diff limit
        ushort reg26_s_mbr_uqp_dlimit:2;        // upper qp diff limit
        ushort reg26_s_mbr_tmb_bits:12;         // target MB bits
        };
        ushort reg26;
    };
    union {
        struct {
        ushort reg27_s_mbr_frame_qstep:13;          // frame level qp's qstep
        ushort reg27_s_mbr_new_lambda:1;
        ushort reg27_s_mbr_tbl_woc_write:1;
        ushort reg27_s_mbr_tbl_woc_done_clr:1;
        };
        ushort reg27;
    };
    union {
        struct {
        ushort reg28_s_mbr_last_frm_avg_qp_lo:16;   // last frame average qp (status register)
        };
        ushort reg28;
    };
    union {
        struct {
        ushort reg29_s_mbr_last_frm_avg_qp_hi:8;    // last frame average qp (status register)
        ushort reg29_s_mbr_qp_cidx_offset:5;        // [H264] chroma qp index offset (+12). Spec range is [-12,12]
        };
        ushort reg29;
    };
    union {
        struct {
        ushort reg2a_s_mbr_qp_min:6;        // qp min
        ushort reg2a_s_mbr_qp_max:6;        // qp max
        ushort reg2a_s_mvdctl_ref0_offset:2;// H264 mvy offset adjustment for MCC if ref is frame 0: 0/1/2: 0/+2/-2
        ushort reg2a_s_mvdctl_ref1_offset:2;// H264 mvy offset adjustment for MCC if ref is frame 1: 0/1/2: 0/+2/-2
        };
        ushort reg2a;
    };
    // IEAP
    union {
        struct {
        ushort reg2b_s_ieap_last_mode:4;        // software control of the last mode of Intra4x4 mode 0 ~ 8
        ushort reg2b_s_ieap_constraint_intra:1; // software control constraint intra; 0/1: OFF/ON
        ushort reg2b_s_ieap_ccest_en:1;         // software control cost estimator; 0/1: OFF/ON
        ushort reg2b_s_ieap_ccest_thr:2;        // threshold of cost estimator set 0 ~ 3 for threshold 1 ~ 4
        ushort reg2b_s_ieap_drop_i16:1;         // software control stop-Intra16x16-mode; 1:w/o I16M, 0:w/i I16MB
        ushort reg2b_s_ieap_early_termination:1;
        };
        ushort reg2b;
    };
    // QUAN
    union {
        struct {
        ushort reg2c_s_quan_idx_last:6;     // the index of the last non-zero coefficient in the zig-zag order
        ushort reg2c_s_quan_idx_swlast:1;   // software control of the index of the last non-zero coefficient in the zig-zag order; 0/1: disable/enable
        ushort reg2c_reserved:8;
        ushort reg2c_g_ieap_sram_4x2_swap:1;
        };
        ushort reg2c;
    };
    // TXIP control & debug
    union {
        struct {
        ushort reg2d_s_txip_mbx:9;              // txip mbx
        ushort reg2d_s_txip_sng_mb:1;           // 0/1: disable/enable txip controller stop-and-go mechanism using
                                                //      (txip_mbx == reg71_g_debug_trig_mbx) & (txip_mby == reg72_g_debug_trig_mby)
        ushort reg2d_s_txip_sng_set:1;          // txip controller stop-and-go mechanism using this register bit:
                                                // 0/1: go/stop
        ushort reg2d_s_txip_dbf_full_halt_en:1; // txip controller stop-and-go mechanism using double buffer fullness as criterion:
                                                // 0/1: disable/enable
        ushort reg2d_s_txip_ack2fme_mode:1;
        };
        ushort reg2d;
    };
    union {
        struct {
        ushort reg2e_s_txip_mby:9;          // txip mby
        };
        ushort reg2e;
    };
    union {
        struct {
        ushort reg2f_s_txip_irfsh_mb_s0:13; // intra refresh mb start 0
        ushort reg2f_reserved:1;
        ushort reg2f_s_txip_irfsh_en:2;     // intra refresh enable bits: bit0: enable condition 0; bit 1: enable condition 1
        };
        ushort reg2f;
    };
    union {
        struct {
        ushort reg30_s_txip_irfsh_mb_e0:13; // intra refresh mb end 0
        };
        ushort reg30;
    };
    union {
        struct {
        ushort reg31_s_txip_irfsh_mb_s1:13; // intra refresh mb start 1
        };
        ushort reg31;
    };
    union {
        struct {
        ushort reg32_s_txip_irfsh_mb_e1:13; // intra refresh mb end 1
        ushort reg32_reserved:1;
        ushort reg32_s_txip_timeout_en:1;   // txip time out enable
        ushort reg32_s_txip_wait_mode:1;    // txip waiting mode to move to next MB; 0/1: idle count/cycle count
        };
        ushort reg32;
    };
    union {
        struct {
        ushort reg33_s_txip_idle_cnt:16;    // wait mode is 0: txip idle count (x 64T)/ wait mode is 1: txip total processing count (x 64T)
        };
        ushort reg33;
    };
    union {
        struct {
        ushort reg34_s_txip_timeout:16;     // txip timeout count (x 64T)
        };
        ushort reg34;
    };
    // [ECDB PIPELINE]
    // ECDB control & debug
    union {
        struct {
        ushort reg35_s_ecdb_mbx:13;         // ecdb mbx
        };
        ushort reg35;
    };
    union {
        struct {
        ushort reg36_s_ecdb_mby:13;         // ecdb mby
        };
        ushort reg36;
    };
    // MDC
    union {
        struct {
        ushort reg37_s_mdc_total_mb_bw:4;   // total mb bit width used in video_pkt
        ushort reg37_s_mdc_m4vpktpzero:1;   // MPEG4 video packet preceding zeros: 0/1: 16/17 zeros
        ushort reg37_s_mdc_m4timev:2;       // MPEG4 modulo time base: 0/1/2/3: 0/10/110/1110
        ushort reg37_s_mdc_m4iadcvlc_th:3;  // MPEG4 intra dc vlc threshold
        ushort reg37_s_mdc_m4vop_tinc_bw:4; // vop_time_increment bit width
        };
        ushort reg37;
    };
    union {
        struct {
        ushort reg38_s_mdc_m4vop_tinc:15;   //  vop_time_increment
        };
        ushort reg38;
    };
    union {
        struct {
        ushort reg39_s_mdc_gob_frame_id:2;          //  H263 gob frame id
        ushort reg39_s_mdc_h264_nal_ref_idc:2;      // nal_ref_idc
        ushort reg39_s_mdc_h264_nal_unit_type:1;    //  0/1: 1/5
        ushort reg39_s_mdc_h264_fnum_bits:2;        // H264 frame num bits
        ushort reg39_s_mdc_h264_dbf_control:1;      // dbf control present flag
        ushort reg39_s_mdc_h264_fnum_value:8;       // H264 frame num value
        };
        ushort reg39;
    };
    union {
        struct {
        ushort reg3a_s_mdc_h264_idr_pic_id:3;
        ushort reg3a_s_mdc_h264_disable_dbf_idc:2;
        ushort reg3a_s_mdc_h264_alpha:4;                // slice_alpha_c0_offset_div2
        ushort reg3a_s_mdc_h264_beta:4;                 // slice_beta_offset_div2
        ushort reg3a_s_mdc_h264_ridx_aor_flag:1;        // reference index active override flag
        ushort reg3a_miu_sel:2;
        };
        ushort reg3a;
    };
    // BSPOBUF/MVOBUF
    union {
        struct {
        ushort reg3b_s_bspobuf_set_adr:1;           // set bsp obuf start address(write one clear)
        ushort reg3b_s_mvobuf_set_adr:1;            // set mv obuf start address (write one clear)
        ushort reg3b_s_bspobuf_fifo_th:3;           // bsp obuf threshold
        ushort reg3b_s_mvobuf_fifo_th:3;            // mv obuf threshold
        ushort reg3b_s_bsp_fdc_skip:1;              // fdc skip enable; 0: fdc skip disable, 1: fdc skip enable
        ushort reg3b_reserved:5;
        ushort reg3b_s_obuf_toggle_obuf0_status:1;  // toggle buf0 status(write one clear)
        ushort reg3b_s_obuf_toggle_obuf1_status:1;  // toggle buf1 status(write one clear)
        };
        ushort reg3b;
    };
    union {
        struct {
        ushort reg3c_s_bspobuf_lo:16;       // bsp obuf start address
        };
        ushort reg3c;
    };
    union {
        struct {
        ushort reg3d_s_bspobuf_hi:13;       // bsp obuf address high
        ushort reg3d_reserved:1;
        ushort reg3d_s_obuf_id:2;           // 00: s0, 01: e0, 10: s1, 11: e1
        };
        ushort reg3d;
    };
    union {
        struct {
        ushort reg3e_s_obuf_write_id_adr:1; // write to this address to enable writing of bspobuf address
        };
        ushort reg3e;
    };
    union {
        struct {
        ushort reg3f_s_bspobuf_hw_en:1;                 // enable HW obuf automatic mechanism
        ushort reg3f_s_bspobuf_update_adr:1;            // update obuf address(write one clear)
        ushort reg3f_s_bspobuf_adr_rchk_sel:2;          // obuf adr read back check selection: 0/1/2/3: s0/e0/s1/e1
        ushort reg3f_s_bspobuf_adr_rchk_en:1;           // enable bspobuf adr read back check through regmfe_s_bspobuf_wptr
        ushort reg3f_reserved:3;
        ushort reg3f_s_bsp_fdc_offset:7;                // bsp's fdc offset
        };
        ushort reg3f;
    };
    union {
        struct {
        ushort reg40_s_mvobuf_saddr_lo:16;  // mv obuf start address
        };
        ushort reg40;
    };
    union {
        struct {
        ushort reg41_s_mvobuf_saddr_hi:13;  // mv obuf start address
        };
        ushort reg41;
    };
    union {
        struct {
        ushort reg42_s_bsp_bit_cnt_lo:16;   // encoded bit count (one frame)
        };
        ushort reg42;
    };
    union {
        struct {
        ushort reg43_s_bsp_bit_cnt_hi:8;    // encoded bit count (one frame)
        };
        ushort reg43;
    };
    union {
        struct {
        ushort reg44_s_bspobuf_wptr_lo:16;  // bspobuf write pointer (8 byte unit)
        };
        ushort reg44;
    };
    union {
        struct {
        ushort reg45_s_bspobuf_wptr_hi:13;  // bspobuf write pointer (8 byte unit)
        };
        ushort reg45;
    };
    // FDC
    union {
        struct {
        ushort reg46_s_fdc_bs:16;           // cpu to fdc bitstream data
        };
        ushort reg46;
    };
    union {
        struct {
        ushort reg47_s_fdc_bs_len:5;        // cpu to fdc bitstream len; 0 ~ 16
        ushort reg47_s_fdc_bs_count:10;     // cpu to fdc round count
        };
        ushort reg47;
    };
    // [Table Control]
    union {
        struct {
        ushort reg48_s_fdc_ack:1;           // fdc to cpu ack; 0/1: frame data pool not empty/frame data pool empty; 48x64 bits of space
        ushort reg48_s_fdc_done_clr:1;      // fdc done clear (write one clear)
        ushort reg48_s_fdc_done:1;          // fdc done; indicate to CPU that data has been written to internal buffer
        ushort reg48_reserved:8;
        ushort reg48_s_fdc_bs_vld:1;        // set for bitstream write out (write one clear)
        ushort reg48_s_tbc_en:1;            // set for table read & write ; 1: enable, 0: disable (write one clear)
        };
        ushort reg48;
    };
    union {
        struct {
        ushort reg49_s_tbc_rw:1;            // table mode; 0: read, 1: write
        ushort reg49_s_tbc_done_clr:1;      // table done clear (write one clear)
        ushort reg49_s_tbc_done:1;          // table done; indicate to CPU that (1) data has been written to table (2) table output is ready at reg4b_s_tbc_rdata
        ushort reg49_reserved:5;
        ushort reg49_s_tbc_adr:6;           // table address
        };
        ushort reg49;
    };
    union {
        struct {
        ushort reg4a_s_tbc_wdata:16;        // table write data
        };
        ushort reg4a;
    };
    union {
        struct {
        ushort reg4b_s_tbc_rdata:16;        // table read data
        };
        ushort reg4b;
    };
    // [Get Neighbor]
    union {
        struct {
        ushort reg4c_s_gn_saddr_lo:16;          // gn base adr low
        };
        ushort reg4c;
    };
    union {
        struct {
        ushort reg4d_s_gn_saddr_hi:13;          // gn base adr high
        ushort reg4d_reserved:2;
        ushort reg4d_s_gn_saddr_mode:1;         // 1: gn save data in one frame   0: gn save data in one row
        };
        ushort reg4d;
    };
    union {
        struct {
        ushort reg4e_s_gn_mvibuf_saddr_lo:16;   // mv ibuf start address low
        };
        ushort reg4e;
    };
    union {
        struct {
        ushort reg4f_s_gn_mvibuf_saddr_hi:13;   // mv ibuf start address high
        ushort reg4f_reserved:1;
        ushort reg4f_s_gn_bwr_mode:2;
        };
        ushort reg4f;
    };
    union {
        struct {
        ushort reg56_s_marb_mrpriority_sw:2;    // mfe2mi_rpriority software programmable
        ushort reg56_s_marb_mr_timeout_ref:1;   // miu read burst timeout count start point
        ushort reg56_s_marb_mr_nwait_mw:1;      // miu read not wait mi2mfe_wrdy
        ushort reg56_s_marb_mwpriority_sw:2;    // mfe2mi_wpriority software programmable
        ushort reg56_s_marb_mw_timeout_ref:1;   // miu write burst timeout count start point
        ushort reg56_s_marb_mw_nwait_mr:1;      // miu read not wait mi2mfe_wrdy
        ushort reg56_s_marb_mr_pending:4;       // max. pending read requests to miu
        ushort reg56_s_marb_32b_ad_nswap:1;     // 32bits miu address not swap. only for 32bits mode
        ushort reg56_s_marb_miu_wmode:1;        // 0/1: original miu protocol/new miu protocol(wd_en)
        ushort reg56_s_marb_rp_ordering:1;
        };
        ushort reg56;
    };
    union {
        struct {
        ushort reg58_s_marb_ubound_0_lo:16;     // MIU protect for MPEG4 BSP obuf
        };
        ushort reg58;
    };
    union {
        struct {
        ushort reg59_s_marb_ubound_0_hi:13;     // MIU protect for MPEG4 BSP obuf
        ushort reg59_reserved:3;
        };
        ushort reg59;
    };
    union {
        struct {
        ushort reg5a_s_marb_lbound_0_lo:16;     // MIU protect for MPEG4 BSP obuf
        };
        ushort reg5a;
    };
    union {
        struct {
        ushort reg5b_s_marb_lbound_0_hi:13;    // MIU protect for MPEG4 BSP obuf
        ushort reg5b_s_marb_miu_bound_en_0:1;
        };
        ushort reg5b;
    };
    union {
        struct {
        ushort reg5c_s_marb_ubound_1_lo:16;    // MIU Upper bound protect for MPEG4 MC obuf  rec
        };
        ushort reg5c;
    };
    union {
        struct {
        ushort reg5d_s_marb_ubound_1_hi:13;    // MIU Upper bound protect for MPEG4 MC obuf rec
        };
        ushort reg5d;
    };
    union {
        struct {
        ushort reg5e_s_marb_lbound_1_lo:16;    // MIU Lower bound protect for MPEG4 MC obuf
        };
        ushort reg5e;
    };
    union {
        struct {
        ushort reg5f_s_marb_lbound_1_hi:13;    // MIU Lower bound protect for MPEG4 MC obuf
        ushort reg5f_s_marb_miu_bound_en_1:1; //miu write protection, miu bound enable for write port 1
        };
        ushort reg5f;
    };
    union {
        struct {
        ushort reg60_s_marb_ubound_2_lo:16;    // MIU protect for MPEG4 MV obuf
        };
        ushort reg60;
    };
    union {
        struct {
        ushort reg61_s_marb_ubound_2_hi:13;    // MIU protect for MPEG4 MV obuf
        };
        ushort reg61;
    };
    union {
        struct {
        ushort reg62_s_marb_lbound_2_lo:16;    // MIU protect for MPEG4 MV obuf
        };
        ushort reg62;
    };
    union {
        struct {
        ushort reg63_s_marb_lbound_2_hi:13;    // MIU protect for MPEG4 MV obuf
        ushort reg63_s_marb_miu_bound_en_2:1;   // miu write protection, miu bound enable for write port 2
        };
        ushort reg63;
    };
    union {
        struct {
        ushort reg64_s_marb_ubound_3_lo:16;    // MIU protect for MPEG4 GN
        };
        ushort reg64;
    };
    union {
        struct {
        ushort reg65_s_marb_ubound_3_hi:13;    // MIU protect for MPEG4 GN
        };
        ushort reg65;
    };
    union {
        struct {
        ushort reg66_s_marb_lbound_3_lo:16;    // MIU protect for MPEG4 GN
        };
        ushort reg66;
    };
    union {
        struct {
        ushort reg67_s_marb_lbound_3_hi:13;    // MIU protect for MPEG4 GN
        ushort reg67_s_marb_miu_bound_en_3:1;
        ushort reg67_s_marb_miu_off:1;
        ushort reg67_s_marb_miu_bound_err:1;
        };
        ushort reg67;
    };
    union {
        struct {
        ushort reg68_s_marb_eimi_block:1;   // miu emi/imi block, 0: disable
        ushort reg68_s_marb_lbwd_mode:1;    // low bandwidth mode, 0: disable
        ushort reg68_s_marb_imi_burst_thd:5;// imi write burst bunch up threshold
        ushort reg68_s_marb_imi_timeout:3;  // imi write burst timeout
        ushort reg68_s_marb_imilast_thd:4;  // auto mfe2imi_last threshold
        ushort reg68_s_prfh_cryc_en:1;      // 0: disable prfh_cryc circuit, 1: enable prfh_cryc circuit
        ushort reg68_s_prfh_refy_en:1;      // 0: disable prfh_refy circuit, 1: enable prfh_refy circuit
        };
        ushort reg68;
    };
    union {
        struct {
        ushort reg69_g_pat_gen_init:16;     // pattern generation initial value
        };
        ushort reg69;
    };
    union {
    struct {
        ushort reg6a_s_bspobuf_idx:2;       // HW obuf index(0->1->2->3->0->1¡K)
        // SW obuf index(00->01->11->10->00...)
        ushort reg6a_s_obuf0_status:1;
        ushort reg6a_s_obuf1_status:1;
        // regmfe_s_bspobuf_adr_status:6
        // {reg6a_fifo_not_full, reg6a_fifo_not_empty, reg6a_enable_obufadr_update, reg6a_obufadr_update_cnt[2:0]}
        ushort reg6a_obufadr_update_cnt:3;
        ushort reg6a_enable_obufadr_update:1;
        ushort reg6a_fifo_not_empty:1;
        ushort reg6a_fifo_not_full:1;
        };
        ushort reg6a;
    };
    union {
        struct {
        ushort reg6b_s_marb_imi_saddr_lo:16;    // imi base address for low bandwidth mode (in 8-byte unit)
        };
        ushort reg6b;
    };
    union {
        struct {
        ushort reg6c_s_marb_imi_saddr_hi:13;
        ushort reg6c_s_marb_rimi_force:1;       // b1:force all read path to IMI   'b0:Normal
        ushort reg6c_s_marb_imi_cache_size:2;   // imi cache size (0: 64kB, 1:32kB, 2:16kB, 3:8kB)
        };
        ushort reg6c;
    };
    union {
        struct {
        ushort reg6d_s_marb_imipriority_thd:4;  // hardware mfe2imi_priority threshold
        ushort reg6d_s_marb_imipriority_sw:2;   // mfe2imi_priority software programmable
        ushort reg6d_reserved:2;
        ushort reg6d_s_prfh_cryc_idle_cnt:4;    // prfh idle count (x 16T) for prfh_cryc
        ushort reg6d_s_prfh_refy_idle_cnt:4;    // prfh idle count (x 16T) for prfh_refy
        };
        ushort reg6d;
    };
    union {
        struct {
        ushort reg6e_s_mbr_qstep_min:7;         // qstep min (note: max value of qstep_min is 128 because condition is qstep <= qstep_min)
        };
        ushort reg6e;
    };
    union {
        struct {
        ushort reg6f_s_mbr_qstep_max:13;        // qstep max
        };
        ushort reg6f;
    };
    union {
        struct {
        ushort reg70_g_debug_mode:7;            // debug mode
        ushort reg70_g_debug_trig_cycle:9;      // wait (8 * reg70_g_debug_trig_cycle) cycles
        };
        ushort reg70;
    };
    union {
        struct {
        ushort reg71_g_debug_trig_mbx:13;       // debug trigger mbx
        };
        ushort reg71;
    };
    union {
        struct {
        ushort reg72_g_debug_trig_mby:13;       // debug trigger mby
        };
        ushort reg72;
    };
    union {
        struct {
        ushort reg73_g_debug_trig:1;            // reg trigger (write one clear)
        ushort reg73_g_debug_trig_mode:2;       // debug trigger mode; 0/1/2/3: reg_trigger/3rd stage (mbx, mby)/frame start
        ushort reg73_g_debug_en:1;              // debug enable
        ushort reg73_g_crc_mode:4;              // 'h0: Disable,¡¥hc: bsp obuf, 'hd: mc obuf, 'hd: mc obuf
        ushort reg73_g_debug_tcycle_chk_en:1;   // enable total cycle check
        ushort reg73_g_debug_tcycle_chk_sel:1;  // select total cycle and report it on regmfe_g_crc_result[15:0]
        ushort reg73_g_sw_buffer_mode:1;        // 0/1: hw/sw buffer mode
        ushort reg73_g_sw_row_done:1;           // sw row done (1T clk_jpe) (write one clear)
        ushort reg73_g_sw_vs:1;                 // sw vsync (1T clk_jpe) (write one clear)
        ushort reg73_g_pat_gen_en:1;            // enable pattern generation
        ushort reg73_g_row_down_irq_en:1;       // row done irq enable; 0: disable, 1: enable
        ushort reg73_g_vs_irq_en:1;             // vs irq enable; 0: disable, 1: enable
        };
        ushort reg73;
    };
    union {
        struct {
        ushort reg74_g_debug_state0:16;         // "debug state for TXIP/ECDB submodule {txip2q_en, txip2iq_en, txip2mbr_en, txip2zmem_en, txip2dpcm_en,
                                                //  txip2mve_en, txip2mcobuf_en, txip2mbldr_en, ecdb2mdc_en,
                                                //  ecdb2rlc_en, ecdb2vlc_en, 5'd0}"
        };
        ushort reg74;
    };
    union {
        struct {
        ushort reg75_g_debug_state1;            // "debug state for ME submodule {3'd0, load_w4_ok, load_w3_ok, load_w2_ok, load_w1_ok, load_w0_ok,  2'd0,
                                                //  busy_ime, busy_fme, busy_mesr, busy_iacost, end_this_mb, init_this_mb"
        };
        ushort reg75;
    };
    union {
        struct {
        ushort reg76_g_crc_result0:16;  // CRC64[15..0]
        };
        ushort reg76;
    };
    union {
        struct {
        ushort reg77_g_crc_result1:16;  // CRC64[31..16]
        };
        ushort reg77;
    };
    union {
        struct {
        ushort reg78_g_crc_result2:16;  // CRC64[47..32]
        };
        ushort reg78;
    };
    union {
        struct {
        ushort reg79_g_crc_result3:16;  // CRC64[63..48]
        };
        ushort reg79;
    };
    union {
        struct {
        ushort reg7a_g_bist_fail0;
        };
        ushort reg7a;
    };
    union {
        struct {
        ushort reg7b_g_bist_fail1;
        };
        ushort reg7b;
    };
    union {
        struct {
        ushort reg7c_g_bist_fail2;
        };
        ushort reg7c;
    };
    union {
        struct {
        ushort reg7d_s_txip_eco0:1;            // 0: original, 1: fix mbldr_cry_done, mbldr_crc_done bug
        ushort reg7d_s_txip_eco1:1;            // reserved registers
        ushort reg7d_eco_mreq_stallgo:1;       // reg7d_eco_mreq_stallgo
        ushort reg7d_eco_marb_stallgo:1;       // reg7d_eco_marb_stallgo
        ushort reg7d_reserved1:1;
        ushort reg7d_eco_bsp_stuffing:1;
        ushort reg7d_eco_bsp_rdy_fix:1;
        ushort reg7d_eco_bsp_multi_slice_fix:1;
        ushort reg7d_reserved2:6;
        ushort reg7d_g_secure_obufadr:1;
        ushort reg7d_g_secure_miu_sel:1;
        };
        ushort reg7d;
    };
    union {
        struct {
        ushort reg80_s_pp_en:1;
        ushort reg80_s_pp_mw_burst_thd:5;
        ushort reg80_s_pp_mw_timeout:3;
        ushort reg80_s_pp_mw_timeout_ref:1;
        ushort reg80_s_pp_burst_split:2;
        ushort reg80_s_pp_32b_ad_nswap:1;
        };
        ushort reg80;
    };
    union {
        struct {
        ushort reg86_g_zmvmap_base_lo:16;
        };
        ushort reg86;
    };
    union {
        struct {
        ushort reg87_g_zmvmap_base_hi:8;
        ushort reg87_g_intra4_penalty:8;
        };
        ushort reg87;
    };
    union {
        struct {
        ushort reg88_g_intra16_penalty:8;
        ushort reg88_g_inter_penalty:8;
        };
        ushort reg88;
    };
    union {
        struct {
        ushort reg89_g_planar_penalty_luma:8;
        ushort reg89_g_planar_penalty_cbcr:8;
        };
        ushort reg89;
    };
    union {
        struct {
        ushort reg8b_g_mb_pitch:12;
        ushort reg8b_g_mb_pitch_en:1;
        };
        ushort reg8b;
    };
    union {
        struct {
        ushort reg90_g_capture_width_y:13;
        };
        ushort reg90;
    };
    union {
        struct {
        ushort reg91_g_capture_width_c:13;
        };
        ushort reg91;
    };
    union {
        struct {
        ushort regd2_g_jpd_hsk_en:1;
        ushort regd2_s_packed422_en:1;
        ushort regd2_s_packed422_endian:1;
        ushort regd2_s_packed422_yc_swap:1;
        ushort regd2_s_packed422_uv_swap:1;
        ushort regd2_s_packed422_delta:8;
        ushort regd2_g_jpd_hsk_shot:1;
        ushort regd2_g_jpd_hsk_mfe:1;
        ushort regd2_g_jpd_frame_done_mask:1;
        };
        ushort regd2;
    };
    union {
        struct {
        ushort regd6_g_yuvldr_en:1;
        ushort regd6_g_hevd_tile:1;
        ushort regd6_g_hevd_tile32:1;
        ushort regd6_g_yuv420_semi:1;
        ushort regd6_g_yuv420_semi_uv_swap:1;
        };
        ushort regd6;
    };
    union {
        struct {
        ushort regdb_s_sambc_td1k_rbw_max:8;
        ushort regdb_s_sambc_td1k_rbw_min:8;
        };
        ushort regdb;
    };
    union {
        struct {
        ushort regdc_s_sambc_td8k_rbw_max:8;
        ushort regdc_s_sambc_td8k_rbw_min:8;
        };
        ushort regdc;
    };
    union {
        struct {
        ushort regdd_s_sambc_td64k_rbw_max:8;
        ushort regdd_s_sambc_td64k_rbw_min:8;
        };
        ushort regdd;
    };
    union {
        struct {
        ushort regde_s_sambc_td1k_wbw_max:8;
        ushort regde_s_sambc_td1k_wbw_min:8;
        };
        ushort regde;
    };
    union {
        struct {
        ushort regdf_s_sambc_td8k_wbw_max:8;
        ushort regdf_s_sambc_td8k_wbw_min:8;
        };
        ushort regdf;
    };
    union {
        struct {
        ushort rege0_s_sambc_td64k_wbw_max:8;
        ushort rege0_s_sambc_td64k_wbw_min:8;
        };
        ushort rege0;
    };
    union {
        struct {
        ushort rege1_s_sambc_mask_en:4;
        ushort rege1_s_sambc_slowdown_mode:4;
        ushort rege1_s_sambc_bypass:8;
        };
        ushort rege1;
    };
    union {
        struct {
        ushort regf2_g_roi_en:1;
        ushort regf2_g_roi_range_sw_limit:1;
        ushort regf2_g_zeromv_en:1;
        ushort regf2_g_i16pln_en:1;
        };
        ushort regf2;
    };
    union {
        struct {
        ushort regf3_s_plnrldr_format:1;
  #define MFE_REG_PLNRLDR_420                   0
  #define MFE_REG_PLNRLDR_422                   1
        ushort regf3_s_plnrldr_rlast_thd:4;
        ushort regf3_s_plnrldr_mrpriority_sw:2;
        ushort regf3_s_plnrldr_mrpriority_thd:7;
        ushort regf3_s_plnrldr_mrpriority_type:1;
        ushort regf3_s_plnrldr_c_swap:1;
  #define MFE_REG_PLNRLDR_UV                    0
  #define MFE_REG_PLNRLDR_VU                    1
        };
        ushort regf3;
    };
    union {
        struct {
        ushort regf4_s_plnrldr_en:1;
        };
        ushort regf4;
    };
    union {
        struct {
        ushort regf5_s_mbr_last_frm_avg_qp_lo:16;
        };
        ushort regf5;
    };
    union {
        struct {
        ushort regf6_s_mbr_last_frm_avg_qp_hi:16;
        };
        ushort regf6;
    };
    union {
        struct {
        ushort regf7_s_mdc_h264_poc:16;
        };
        ushort regf7;
    };
    union {
        struct {
        ushort regf8_s_mdc_h264_poc_enable:1;
        ushort regf8_s_mdc_h264_poc_width:5;
        };
        ushort regf8;
    };
    union {
        struct {
        ushort regfb_g_roi_qmap_adr_lo:16;
        };
        ushort regfb;
    };
    union {
        struct {
        ushort regfc_g_roi_qmap_adr_hi:8;
        };
        ushort regfc;
    };
    union {
        struct {
        ushort regfd_reserved0:6;
        ushort regfd_eco:1;
        };
        ushort regfd;
    };
} mfe6_reg;

#endif//_MFE6_REG_H_
