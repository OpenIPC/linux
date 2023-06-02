#ifndef _IFE_INT_REGISTER_H_
#define _IFE_INT_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif


#if defined (__LINUX)

#include "mach/rcw_macro.h"

#include "plat/top.h"
#include "kwrap/type.h"
#include "kwrap/nvt_type.h"

//=========================================================================
#elif defined (__FREERTOS)


#include "rcw_macro.h"
#include "plat/top.h"
#include "kwrap/type.h"
#include "kwrap/nvt_type.h"

#else


#endif


typedef struct
{

    union
    {
        struct
        {
            unsigned ife_sw_rst             :  1;            //bits : 0
            unsigned ife_start              :  1;            //bits : 1
            unsigned ife_load_start         :  1;            //bits : 2
            unsigned ife_load_fd            :  1;            //bits : 3
            unsigned ife_load_frmstart      :  1;            //bits : 4
            unsigned ife_global_load_en     :  1;            //bits : 5
            unsigned                        : 22;
            unsigned ife_ll_fire            :  1;            //bits : 28
            unsigned ife_ll_terminate       :  1;            //bits : 29
        } bit;
        UINT32 word;
    } ife_register_0; // 0x00;

    union
    {
        struct
        {
            unsigned ife_mode               :  2;            //bits : 1..0
            unsigned                        :  2;
			unsigned inbit_16_fmt_sel       :  1;            //bits : 4
            unsigned inbit_depth            :  2;            //bits : 6..5
            unsigned outbit_depth           :  2;            //bits : 8..7
            unsigned cfapat                 :  3;            //bits : 11..9
            unsigned ife_filt_mode          :  1;            //bits : 12
            unsigned outl_en            :  1;            //bits : 13
            unsigned filter_en          :  1;            //bits : 14
            unsigned cgain_en           :  1;            //bits : 15
            unsigned vig_en             :  1;            //bits : 16
            unsigned gbal_en            :  1;            //bits : 17
            unsigned ife_binning            :  3;            //bits : 20..18
            unsigned bayer_format           :  1;            //bits : 21
            unsigned rgbir_rb_nrfill    :  1;            //bits : 22
            unsigned bilat_th_en        :  1;            //bits : 23
            unsigned f_nrs_en           :  1;            //bits : 24
            unsigned f_cg_en            :  1;            //bits : 25
            unsigned f_fusion_en        :  1;            //bits : 26
            unsigned f_fusion_fnum      :  2;            //bits : 28..27
            unsigned f_fc_en            :  1;            //bits : 29
            unsigned mirror_en              :  1;            //bits : 30
            unsigned r_decode_en        :  1;            //bits : 31
        } bit;
        UINT32 word;
    } ife_register_1; // 0x04;

    union
    {
        struct
        {
            unsigned inte_frmend            :  1;            //bits : 0
            unsigned 					    :  1;            //bits : 1
            unsigned inte_ife_r_dec2_err    :  1;            //bits : 2
            unsigned inte_llend             :  1;            //bits : 3
            unsigned inte_llerror           :  1;            //bits : 4
            unsigned inte_llerror2          :  1;            //bits : 5
            unsigned inte_lljobend          :  1;            //bits : 6
            unsigned inte_ife_bufovfl       :  1;            //bits : 7
        } bit;
        UINT32 word;
    } ife_register_2; // 0x08;

    union
    {
        struct
        {
            unsigned int_frmend             :  1;            //bits : 0
            unsigned 					    :  1;            //bits : 1
            unsigned int_ife_r_dec2_err     :  1;            //bits : 2
            unsigned int_llend              :  1;            //bits : 3
            unsigned int_llerror            :  1;            //bits : 4
            unsigned int_llerror2           :  1;            //bits : 5
            unsigned int_lljobend           :  1;            //bits : 6
            unsigned int_ife_bufovfl        :  1;            //bits : 7
        } bit;
        UINT32 word;
    } ife_register_3; // 0x0c;

    union
    {
        struct
        {
            unsigned ife_busy               :  1;            //bits : 0
        } bit;
        UINT32 word;
    } ife_register_4; // 0x10;

    union
    {
        struct
        {
            unsigned ife_hn                 : 11;            //bits : 10..0
            unsigned                        :  1;
            unsigned ife_hl                 : 11;            //bits : 22..12
            unsigned                        :  1;
            unsigned ife_hm                 :  4;            //bits : 27..24
        } bit;
        UINT32 word;
    } ife_register_5; // 0x14;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_6; // 0x18;

    union
    {
        struct
        {
        	unsigned ife_dmach_idle           :1;              //bits : 0 
			unsigned ife_dmach_dis            :1;              //bits : 1
        } bit;
        UINT32 word;
    } ife_register_7; // 0x1c;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned width                  : 14;            //bits : 15..2
            unsigned                        :  1;
            unsigned height                 : 15;            //bits : 31..17
        } bit;
        UINT32 word;
    } ife_register_8; // 0x20;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned crop_width             : 14;            //bits : 15..2
            unsigned                        :  1;
            unsigned crop_height            : 15;            //bits : 31..17
        } bit;
        UINT32 word;
    } ife_register_9; // 0x24;

    union
    {
        struct
        {
            unsigned crop_hpos              : 16;            //bits : 15..0
            unsigned crop_vpos              : 16;            //bits : 31..16
        } bit;
        UINT32 word;
    } ife_register_10; // 0x28;

    union
    {
        struct
        {
        	unsigned ife_line_count			: 16;
			unsigned sie2_line_count		: 16;
        } bit;
        UINT32 word;
    } ife_register_11; // 0x2c;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram_sai0              : 30;            //bits : 31..2
        } bit;
        UINT32 word;
    } ife_register_12; // 0x30;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram_ofsi0             : 14;            //bits : 15..2
            unsigned h_start_shift          :  4;            //bits : 19..16
        } bit;
        UINT32 word;
    } ife_register_13; // 0x34;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram_sai1              : 30;            //bits : 31..2
        } bit;
        UINT32 word;
    } ife_register_14; // 0x38;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram_ofsi1             : 14;            //bits : 15..2
        } bit;
        UINT32 word;
    } ife_register_15; // 0x3c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_16; // 0x40;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_17; // 0x44;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_18; // 0x48;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_19; // 0x4c;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram                   : 30;            //bits : 31..2
        } bit;
        UINT32 word;
    } ife_register_20; // 0x50;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram_sao               : 30;            //bits : 31..2
        } bit;
        UINT32 word;
    } ife_register_21; // 0x54;

    union
    {
        struct
        {
            unsigned                        :  2;
            unsigned dram_ofso              : 14;            //bits : 15..2
        } bit;
        UINT32 word;
    } ife_register_22; // 0x58;

    union
    {
        struct
        {
            unsigned input_burst_mode       :  1;            //bits : 0
            unsigned                        :  3;
            unsigned output_burst_mode      :  1;            //bits : 4
            unsigned                        :  7;
            unsigned dmaloop_line           : 11;            //bits : 22..12
            unsigned                        :  1;
            unsigned dmaloop_en             :  1;            //bits : 24
            unsigned dmaloop_ctrl           :  1;            //bits : 25            
        } bit;
        UINT32 word;
    } ife_register_23; // 0x5c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_24; // 0x60;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_25; // 0x64;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_26; // 0x68;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_27; // 0x6c;

    union
    {
        struct
        {
            unsigned cgain_inv              :  1;            //bits : 0
            unsigned cgain_hinv             :  1;            //bits : 1
            unsigned cgain_range            :  1;            //bits : 2
            unsigned ife_f_cgain_range      :  1;            //bits : 3
            unsigned                        :  4;
            unsigned cgain_mask             : 12;            //bits : 19..8
        } bit;
        UINT32 word;
    } ife_register_28; // 0x70;

    union
    {
        struct
        {
            unsigned ife_cgain_r            : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_cgain_gr           : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_29; // 0x74;

    union
    {
        struct
        {
            unsigned ife_cgain_gb           : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_cgain_b            : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_30; // 0x78;

    union
    {
        struct
        {
            unsigned ife_cgain_ir           : 10;            //bits : 9..0
        } bit;
        UINT32 word;
    } ife_register_31; // 0x7c;

    union
    {
        struct
        {
            unsigned ife_f_p0_cgain_r       : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p0_cgain_gr      : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_32; // 0x80;

    union
    {
        struct
        {
            unsigned ife_f_p0_cgain_gb      : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p0_cgain_b       : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_33; // 0x84;

    union
    {
        struct
        {
            unsigned ife_f_p0_cgain_ir      : 10;            //bits : 9..0
        } bit;
        UINT32 word;
    } ife_register_34; // 0x88;

    union
    {
        struct
        {
            unsigned ife_f_p1_cgain_r       : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p1_cgain_gr      : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_35; // 0x8c;

    union
    {
        struct
        {
            unsigned ife_f_p1_cgain_gb      : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p1_cgain_b       : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_36; // 0x90;

    union
    {
        struct
        {
            unsigned ife_f_p1_cgain_ir      : 10;            //bits : 9..0
        } bit;
        UINT32 word;
    } ife_register_37; // 0x94;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_38; // 0x98;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_39; // 0x9c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_40; // 0xa0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_41; // 0xa4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_42; // 0xa8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_43; // 0xac;

    union
    {
        struct
        {
            unsigned ife_cofs_r             : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_cofs_gr            : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_44; // 0xb0;

    union
    {
        struct
        {
            unsigned ife_cofs_gb            : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_cofs_b             : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_45; // 0xb4;

    union
    {
        struct
        {
            unsigned ife_cofs_ir            : 10;            //bits : 9..0
        } bit;
        UINT32 word;
    } ife_register_46; // 0xb8;

    union
    {
        struct
        {
            unsigned ife_f_p0_cofs_r        : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p0_cofs_gr       : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_47; // 0xbc;

    union
    {
        struct
        {
            unsigned ife_f_p0_cofs_gb       : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p0_cofs_b        : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_48; // 0xc0;

    union
    {
        struct
        {
            unsigned ife_f_p0_cofs_ir       : 10;            //bits : 9..0
        } bit;
        UINT32 word;
    } ife_register_49; // 0xc4;

    union
    {
        struct
        {
            unsigned ife_f_p1_cofs_r        : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p1_cofs_gr       : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_50; // 0xc8;

    union
    {
        struct
        {
            unsigned ife_f_p1_cofs_gb       : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_f_p1_cofs_b        : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_51; // 0xcc;

    union
    {
        struct
        {
            unsigned ife_f_p1_cofs_ir       : 10;            //bits : 9..0
        } bit;
        UINT32 word;
    } ife_register_52; // 0xd0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_53; // 0xd4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_54; // 0xd8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_55; // 0xdc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_56; // 0xe0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_57; // 0xe4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_58; // 0xe8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_59; // 0xec;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_60; // 0xf0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_61; // 0xf4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_62; // 0xf8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_63; // 0xfc;

    union
    {
        struct
        {
            unsigned ife_clamp_th           : 12;            //bits : 11..0
            unsigned ife_clamp_mul          :  8;            //bits : 19..12
            unsigned ife_clamp_dlt          : 12;            //bits : 31..20
        } bit;
        UINT32 word;
    } ife_register_64; // 0x100;

    union
    {
        struct
        {
            unsigned ife_bilat_w            :  4;            //bits : 3..0
            unsigned ife_rth_w              :  4;            //bits : 7..4
            unsigned ife_bilat_th1          : 10;            //bits : 17..8
            unsigned ife_bilat_th2          : 10;            //bits : 27..18
            unsigned ife_bilat_cen_sel      :  1;            //bits : 28
        } bit;
        UINT32 word;
    } ife_register_65; // 0x104;

    union
    {
        struct
        {
            unsigned ife_s_weight0          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_s_weight1          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_s_weight2          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_s_weight3          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_66; // 0x108;

    union
    {
        struct
        {
            unsigned ife_s_weight4          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_s_weight5          :  5;            //bits : 12..8
        } bit;
        UINT32 word;
    } ife_register_67; // 0x10c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c0_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_68; // 0x110;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c0_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_69; // 0x114;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c0_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_70; // 0x118;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_71; // 0x11c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c1_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_72; // 0x120;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c1_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_73; // 0x124;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c1_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_74; // 0x128;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_75; // 0x12c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c2_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_76; // 0x130;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c2_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_77; // 0x134;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c2_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_78; // 0x138;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_79; // 0x13c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c3_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_80; // 0x140;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c3_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_81; // 0x144;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_a_c3_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_82; // 0x148;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_83; // 0x14c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c0_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_84; // 0x150;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c0_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_85; // 0x154;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c0_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_86; // 0x158;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_87; // 0x15c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c1_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_88; // 0x160;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c1_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_89; // 0x164;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c1_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_90; // 0x168;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_91; // 0x16c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c2_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_92; // 0x170;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c2_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_93; // 0x174;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c2_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_94; // 0x178;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_95; // 0x17c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_0         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c3_1         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_96; // 0x180;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_2         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c3_3         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_97; // 0x184;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_4         : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_rth_b_c3_5         : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_98; // 0x188;

    union
    {
        struct
        {
            unsigned ife_outl_bright_ofs    : 12;            //bits : 11..0
            unsigned ife_outl_dark_ofs      : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_99; // 0x18c;

    union
    {
        struct
        {
            unsigned ife_outlth_bri0        : 12;            //bits : 11..0
            unsigned ife_outlth_dark0       : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_100; // 0x190;

    union
    {
        struct
        {
            unsigned ife_outlth_bri1        : 12;            //bits : 11..0
            unsigned ife_outlth_dark1       : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_101; // 0x194;

    union
    {
        struct
        {
            unsigned ife_outlth_bri2        : 12;            //bits : 11..0
            unsigned ife_outlth_dark2       : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_102; // 0x198;

    union
    {
        struct
        {
            unsigned ife_outlth_bri3        : 12;            //bits : 11..0
            unsigned ife_outlth_dark3       : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_103; // 0x19c;

    union
    {
        struct
        {
            unsigned ife_outlth_bri4        : 12;            //bits : 11..0
            unsigned ife_outlth_dark4       : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_104; // 0x1a0;

    union
    {
        struct
        {
            unsigned ife_outl_avg_mode      :  1;            //bits : 0
            unsigned                        :  7;
            unsigned ife_outl_weight        :  8;            //bits : 15..8
            unsigned ife_outl_cnt1          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_outl_cnt2          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_105; // 0x1a4;

    union
    {
        struct
        {
            unsigned ife_ord_range_bri      :  3;            //bits : 2..0
            unsigned                        :  1;
            unsigned ife_ord_range_dark     :  3;            //bits : 6..4
            unsigned                        :  1;
            unsigned ife_ord_protect_th     : 10;            //bits : 17..8
            unsigned                        :  2;
            unsigned ife_ord_blend_w        :  8;            //bits : 27..20
        } bit;
        UINT32 word;
    } ife_register_106; // 0x1a8;

    union
    {
        struct
        {
            unsigned ife_ord_bri_wlut0      :  4;            //bits : 3..0
            unsigned ife_ord_bri_wlut1      :  4;            //bits : 7..4
            unsigned ife_ord_bri_wlut2      :  4;            //bits : 11..8
            unsigned ife_ord_bri_wlut3      :  4;            //bits : 15..12
            unsigned ife_ord_bri_wlut4      :  4;            //bits : 19..16
            unsigned ife_ord_bri_wlut5      :  4;            //bits : 23..20
            unsigned ife_ord_bri_wlut6      :  4;            //bits : 27..24
            unsigned ife_ord_bri_wlut7      :  4;            //bits : 31..28
        } bit;
        UINT32 word;
    } ife_register_107; // 0x1ac;

    union
    {
        struct
        {
            unsigned ife_ord_dark_wlut0     :  4;            //bits : 3..0
            unsigned ife_ord_dark_wlut1     :  4;            //bits : 7..4
            unsigned ife_ord_dark_wlut2     :  4;            //bits : 11..8
            unsigned ife_ord_dark_wlut3     :  4;            //bits : 15..12
            unsigned ife_ord_dark_wlut4     :  4;            //bits : 19..16
            unsigned ife_ord_dark_wlut5     :  4;            //bits : 23..20
            unsigned ife_ord_dark_wlut6     :  4;            //bits : 27..24
            unsigned ife_ord_dark_wlut7     :  4;            //bits : 31..28
        } bit;
        UINT32 word;
    } ife_register_108; // 0x1b0;

    union
    {
        struct
        {
            unsigned ife_gbal_edge_protect_en:  1;            //bits : 0
            unsigned                        :  7;
            unsigned ife_gbal_diff_thr_str  : 10;            //bits : 17..8
            unsigned                        :  2;
            unsigned ife_gbal_diff_w_max    :  4;            //bits : 23..20
        } bit;
        UINT32 word;
    } ife_register_109; // 0x1b4;

    union
    {
        struct
        {
            unsigned ife_gbal_edge_thr_1    : 10;            //bits : 9..0
            unsigned                        :  6;
            unsigned ife_gbal_edge_thr_0    : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_110; // 0x1b8;

    union
    {
        struct
        {
            unsigned ife_gbal_edge_w_max    :  8;            //bits : 7..0
            unsigned ife_gbal_edge_w_min    :  8;            //bits : 15..8
        } bit;
        UINT32 word;
    } ife_register_111; // 0x1bc;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_112; // 0x1c0;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_113; // 0x1c4;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_114; // 0x1c8;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_115; // 0x1cc;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_116; // 0x1d0;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_117; // 0x1d4;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_118; // 0x1d8;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_119; // 0x1dc;

    union
    {
        struct
        {
            unsigned ife_rth_a_c0_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c0_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_120; // 0x1e0;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_121; // 0x1e4;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_122; // 0x1e8;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_123; // 0x1ec;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_124; // 0x1f0;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_125; // 0x1f4;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_126; // 0x1f8;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_127; // 0x1fc;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_128; // 0x200;

    union
    {
        struct
        {
            unsigned ife_rth_a_c1_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c1_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_129; // 0x204;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_130; // 0x208;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_131; // 0x20c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_132; // 0x210;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_133; // 0x214;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_134; // 0x218;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_135; // 0x21c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_136; // 0x220;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_137; // 0x224;

    union
    {
        struct
        {
            unsigned ife_rth_a_c2_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c2_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_138; // 0x228;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_139; // 0x22c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_140; // 0x230;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_141; // 0x234;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_142; // 0x238;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_143; // 0x23c;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_144; // 0x240;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_145; // 0x244;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_146; // 0x248;

    union
    {
        struct
        {
            unsigned ife_rth_a_c3_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_a_c3_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_147; // 0x24c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_148; // 0x250;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_149; // 0x254;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_150; // 0x258;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_151; // 0x25c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_152; // 0x260;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_153; // 0x264;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_154; // 0x268;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_155; // 0x26c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c0_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c0_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_156; // 0x270;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_157; // 0x274;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_158; // 0x278;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_159; // 0x27c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_160; // 0x280;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_161; // 0x284;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_162; // 0x288;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_163; // 0x28c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_164; // 0x290;

    union
    {
        struct
        {
            unsigned ife_rth_b_c1_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c1_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_165; // 0x294;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_166; // 0x298;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_167; // 0x29c;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_168; // 0x2a0;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_169; // 0x2a4;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_170; // 0x2a8;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_171; // 0x2ac;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_172; // 0x2b0;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_173; // 0x2b4;

    union
    {
        struct
        {
            unsigned ife_rth_b_c2_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c2_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_174; // 0x2b8;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_175; // 0x2bc;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_176; // 0x2c0;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_177; // 0x2c4;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_178; // 0x2c8;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_179; // 0x2cc;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_180; // 0x2d0;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_181; // 0x2d4;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_182; // 0x2d8;

    union
    {
        struct
        {
            unsigned ife_rth_b_c3_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_rth_b_c3_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_183; // 0x2dc;

    union
    {
        struct
        {
            unsigned ife_distvgtx_c0        : 14;            //bits : 13..0
            unsigned                        :  2;
            unsigned ife_distvgty_c0        : 14;            //bits : 29..16
            unsigned ife_distgain           :  2;            //bits : 31..30
        } bit;
        UINT32 word;
    } ife_register_184; // 0x2e0;

    union
    {
        struct
        {
            unsigned ife_distvgtx_c1        : 14;            //bits : 13..0
            unsigned                        :  2;
            unsigned ife_distvgty_c1        : 14;            //bits : 29..16
        } bit;
        UINT32 word;
    } ife_register_185; // 0x2e4;

    union
    {
        struct
        {
            unsigned ife_distvgtx_c2        : 14;            //bits : 13..0
            unsigned                        :  2;
            unsigned ife_distvgty_c2        : 14;            //bits : 29..16
        } bit;
        UINT32 word;
    } ife_register_186; // 0x2e8;

    union
    {
        struct
        {
            unsigned ife_distvgtx_c3        : 14;            //bits : 13..0
            unsigned                        :  2;
            unsigned ife_distvgty_c3        : 14;            //bits : 29..16
        } bit;
        UINT32 word;
    } ife_register_187; // 0x2ec;

    union
    {
        struct
        {
            unsigned ife_distvgxdiv         : 12;            //bits : 11..0
            unsigned ife_distvgydiv         : 12;            //bits : 23..12
        } bit;
        UINT32 word;
    } ife_register_188; // 0x2f0;

    union
    {
        struct
        {
            unsigned distdthr_en             :  1;            //bits : 0
            unsigned ife_vig_fisheye_gain_en :  1;            //bits : 1
            unsigned                         :  2;
            unsigned distdthr_rst            :  1;            //bits : 4
            unsigned                         :  3;
            unsigned distthr                 : 10;            //bits : 17..8
            unsigned ife_vig_fisheye_slope   :  4;            //bits : 21..18
            unsigned ife_vig_fisheye_radius  : 10;            //bits : 31..22
        } bit;
        UINT32 word;
    } ife_register_189; // 0x2f4;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_190; // 0x2f8;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_191; // 0x2fc;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_192; // 0x300;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_193; // 0x304;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_194; // 0x308;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_195; // 0x30c;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_196; // 0x310;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_197; // 0x314;

    union
    {
        struct
        {
            unsigned ife_vig_c0_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c0_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_198; // 0x318;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_199; // 0x31c;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_200; // 0x320;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_201; // 0x324;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_202; // 0x328;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_203; // 0x32c;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_204; // 0x330;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_205; // 0x334;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_206; // 0x338;

    union
    {
        struct
        {
            unsigned ife_vig_c1_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c1_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_207; // 0x33c;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_208; // 0x340;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_209; // 0x344;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_210; // 0x348;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_211; // 0x34c;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_212; // 0x350;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_213; // 0x354;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_214; // 0x358;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_215; // 0x35c;

    union
    {
        struct
        {
            unsigned ife_vig_c2_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c2_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_216; // 0x360;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut00   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut01   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_217; // 0x364;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut02   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut03   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_218; // 0x368;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut04   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut05   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_219; // 0x36c;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut06   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut07   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_220; // 0x370;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut08   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut09   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_221; // 0x374;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut10   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut11   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_222; // 0x378;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut12   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut13   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_223; // 0x37c;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut14   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut15   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_224; // 0x380;

    union
    {
        struct
        {
            unsigned ife_vig_c3_lut16   : 10;            //bits : 9..0
            unsigned                      :  6;
            unsigned ife_vig_c3_lut17   : 10;            //bits : 25..16
        } bit;
        UINT32 word;
    } ife_register_225; // 0x384;

    union
    {
        struct
        {
            unsigned ife_gbal_ofs_lut_00    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_01    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_02    :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_03    :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_226; // 0x388;

    union
    {
        struct
        {
            unsigned ife_gbal_ofs_lut_04    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_05    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_06    :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_07    :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_227; // 0x38c;

    union
    {
        struct
        {
            unsigned ife_gbal_ofs_lut_08    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_09    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_10    :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_11    :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_228; // 0x390;

    union
    {
        struct
        {
            unsigned ife_gbal_ofs_lut_12    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_13    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_14    :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_gbal_ofs_lut_15    :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_229; // 0x394;

    union
    {
        struct
        {
            unsigned ife_gbal_ofs_lut_16    :  6;            //bits : 5..0
        } bit;
        UINT32 word;
    } ife_register_230; // 0x398;

    union
    {
        struct
        {
            unsigned ife_rbluma00           :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbluma01           :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbluma02           :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbluma03           :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_231; // 0x39c;

    union
    {
        struct
        {
            unsigned ife_rbluma04           :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbluma05           :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbluma06           :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbluma07           :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_232; // 0x3a0;

    union
    {
        struct
        {
            unsigned ife_rbluma08           :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbluma09           :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbluma10           :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbluma11           :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_233; // 0x3a4;

    union
    {
        struct
        {
            unsigned ife_rbluma12           :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbluma13           :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbluma14           :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbluma15           :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_234; // 0x3a8;

    union
    {
        struct
        {
            unsigned ife_rbluma16           :  5;            //bits : 4..0
        } bit;
        UINT32 word;
    } ife_register_235; // 0x3ac;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_236; // 0x3b0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_237; // 0x3b4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_238; // 0x3b8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_239; // 0x3bc;

    union
    {
        struct
        {
            unsigned ife_rbratio00          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio01          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio02          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio03          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_240; // 0x3c0;

    union
    {
        struct
        {
            unsigned ife_rbratio04          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio05          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio06          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio07          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_241; // 0x3c4;

    union
    {
        struct
        {
            unsigned ife_rbratio08          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio09          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio10          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio11          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_242; // 0x3c8;

    union
    {
        struct
        {
            unsigned ife_rbratio12          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio13          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio14          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio15          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_243; // 0x3cc;

    union
    {
        struct
        {
            unsigned ife_rbratio16          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio17          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio18          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio19          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_244; // 0x3d0;

    union
    {
        struct
        {
            unsigned ife_rbratio20          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio21          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio22          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio23          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_245; // 0x3d4;

    union
    {
        struct
        {
            unsigned ife_rbratio24          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio25          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio26          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio27          :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_246; // 0x3d8;

    union
    {
        struct
        {
            unsigned ife_rbratio28          :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_rbratio29          :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_rbratio30          :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_rbratio31          :  5;            //bits : 28..24
            unsigned rbratio_mode           :  2;            //bits : 30..29
        } bit;
        UINT32 word;
    } ife_register_247; // 0x3dc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_248; // 0x3e0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_249; // 0x3e4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_250; // 0x3e8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_251; // 0x3ec;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_252; // 0x3f0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_253; // 0x3f4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_254; // 0x3f8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_255; // 0x3fc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_256; // 0x400;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_257; // 0x404;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_258; // 0x408;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_259; // 0x40c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_260; // 0x410;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_261; // 0x414;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_262; // 0x418;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_263; // 0x41c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_264; // 0x420;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_265; // 0x424;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_266; // 0x428;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_267; // 0x42c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_268; // 0x430;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_269; // 0x434;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_270; // 0x438;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_271; // 0x43c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_272; // 0x440;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_273; // 0x444;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_274; // 0x448;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_275; // 0x44c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_276; // 0x450;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_277; // 0x454;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_278; // 0x458;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_279; // 0x45c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_280; // 0x460;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_281; // 0x464;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_282; // 0x468;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_283; // 0x46c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_284; // 0x470;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_285; // 0x474;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_286; // 0x478;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_287; // 0x47c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_288; // 0x480;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_289; // 0x484;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_290; // 0x488;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_291; // 0x48c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_292; // 0x490;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_293; // 0x494;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_294; // 0x498;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_295; // 0x49c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_296; // 0x4a0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_297; // 0x4a4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_298; // 0x4a8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_299; // 0x4ac;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_300; // 0x4b0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_301; // 0x4b4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_302; // 0x4b8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_303; // 0x4bc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_304; // 0x4c0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_305; // 0x4c4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_306; // 0x4c8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_307; // 0x4cc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_308; // 0x4d0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_309; // 0x4d4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_310; // 0x4d8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_311; // 0x4dc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_312; // 0x4e0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_313; // 0x4e4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_314; // 0x4e8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_315; // 0x4ec;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_316; // 0x4f0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_317; // 0x4f4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_318; // 0x4f8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_319; // 0x4fc;

    union
    {
        struct
        {
            unsigned ife_f_nrs_ord_en            :  1;            //bits : 0
            unsigned ife_f_nrs_bilat_en          :  1;            //bits : 1
            unsigned ife_f_nrs_gbilat_en         :  1;            //bits : 2
            unsigned                             : 13;
            unsigned ife_f_nrs_bilat_strength    :  4;            //bits : 19..16
            unsigned ife_f_nrs_gbilat_strength   :  4;            //bits : 23..20
            unsigned ife_f_nrs_gbilat_weight     :  4;            //bits : 27..24
        } bit;
        UINT32 word;
    } ife_register_320; // 0x500;

    union
    {
        struct
        {
            unsigned ife_f_nrs_ord_range_bri:  3;            //bits : 2..0
            unsigned                        :  1;
            unsigned ife_f_nrs_ord_range_dark:  3;            //bits : 6..4
            unsigned                        :  1;
            unsigned ife_f_nrs_ord_diff_thr : 10;            //bits : 17..8
        } bit;
        UINT32 word;
    } ife_register_321; // 0x504;

    union
    {
        struct
        {
            unsigned ife_f_nrs_ord_dark_wlut0:  4;            //bits : 3..0
            unsigned ife_f_nrs_ord_dark_wlut1:  4;            //bits : 7..4
            unsigned ife_f_nrs_ord_dark_wlut2:  4;            //bits : 11..8
            unsigned ife_f_nrs_ord_dark_wlut3:  4;            //bits : 15..12
            unsigned ife_f_nrs_ord_dark_wlut4:  4;            //bits : 19..16
            unsigned ife_f_nrs_ord_dark_wlut5:  4;            //bits : 23..20
            unsigned ife_f_nrs_ord_dark_wlut6:  4;            //bits : 27..24
            unsigned ife_f_nrs_ord_dark_wlut7:  4;            //bits : 31..28
        } bit;
        UINT32 word;
    } ife_register_322; // 0x508;

    union
    {
        struct
        {
            unsigned ife_f_nrs_ord_bri_wlut0:  4;            //bits : 3..0
            unsigned ife_f_nrs_ord_bri_wlut1:  4;            //bits : 7..4
            unsigned ife_f_nrs_ord_bri_wlut2:  4;            //bits : 11..8
            unsigned ife_f_nrs_ord_bri_wlut3:  4;            //bits : 15..12
            unsigned ife_f_nrs_ord_bri_wlut4:  4;            //bits : 19..16
            unsigned ife_f_nrs_ord_bri_wlut5:  4;            //bits : 23..20
            unsigned ife_f_nrs_ord_bri_wlut6:  4;            //bits : 27..24
            unsigned ife_f_nrs_ord_bri_wlut7:  4;            //bits : 31..28
        } bit;
        UINT32 word;
    } ife_register_323; // 0x50c;

    union
    {
        struct
        {
            unsigned ife_f_nrs_bilat_lut_ofs0   :  8;            //bits : 7..0
            unsigned ife_f_nrs_bilat_lut_ofs1   :  8;            //bits : 15..8
            unsigned ife_f_nrs_bilat_lut_ofs2   :  8;            //bits : 23..16
            unsigned ife_f_nrs_bilat_lut_ofs3   :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_324; // 0x510;

    union
    {
        struct
        {
            unsigned ife_f_nrs_bilat_lut_ofs4   :  8;            //bits : 7..0
            unsigned ife_f_nrs_bilat_lut_ofs5   :  8;            //bits : 15..8
        } bit;
        UINT32 word;
    } ife_register_325; // 0x514;

    union
    {
        struct
        {
            unsigned ife_f_nrs_bilat_lut_w0     :  5;            //bits : 4..0
            unsigned ife_f_nrs_bilat_lut_w1     :  5;            //bits : 9..5
            unsigned ife_f_nrs_bilat_lut_w2     :  5;            //bits : 14..10
            unsigned ife_f_nrs_bilat_lut_w3     :  5;            //bits : 19..15
            unsigned ife_f_nrs_bilat_lut_w4     :  5;            //bits : 24..20
            unsigned ife_f_nrs_bilat_lut_w5     :  5;            //bits : 29..25
        } bit;
        UINT32 word;
    } ife_register_326; // 0x518;

    union
    {
        struct
        {
            unsigned ife_f_nrs_bilat_lut_r1     : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_nrs_bilat_lut_r2     : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_327; // 0x51c;

    union
    {
        struct
        {
            unsigned ife_f_nrs_bilat_lut_r3     : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_nrs_bilat_lut_r4     : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_328; // 0x520;

    union
    {
        struct
        {
            unsigned ife_f_nrs_bilat_lut_r5     : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_nrs_bilat_lut_th1    :  2;            //bits : 17..16
            unsigned ife_f_nrs_bilat_lut_th2    :  2;            //bits : 19..18
            unsigned ife_f_nrs_bilat_lut_th3    :  2;            //bits : 21..20
            unsigned ife_f_nrs_bilat_lut_th4    :  2;            //bits : 23..22
            unsigned ife_f_nrs_bilat_lut_th5    :  2;            //bits : 25..24
        } bit;
        UINT32 word;
    } ife_register_329; // 0x524;

    union
    {
        struct
        {
            unsigned ife_f_nrs_gbilat_lut_ofs0   :  8;            //bits : 7..0
            unsigned ife_f_nrs_gbilat_lut_ofs1   :  8;            //bits : 15..8
            unsigned ife_f_nrs_gbilat_lut_ofs2   :  8;            //bits : 23..16
            unsigned ife_f_nrs_gbilat_lut_ofs3   :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_330; // 0x528;

    union
    {
        struct
        {
            unsigned ife_f_nrs_gbilat_lut_ofs4   :  8;            //bits : 7..0
            unsigned ife_f_nrs_gbilat_lut_ofs5   :  8;            //bits : 15..8
        } bit;
        UINT32 word;
    } ife_register_331; // 0x52c;

    union
    {
        struct
        {
            unsigned ife_f_nrs_gbilat_lut_w0     :  5;            //bits : 4..0
            unsigned ife_f_nrs_gbilat_lut_w1     :  5;            //bits : 9..5
            unsigned ife_f_nrs_gbilat_lut_w2     :  5;            //bits : 14..10
            unsigned ife_f_nrs_gbilat_lut_w3     :  5;            //bits : 19..15
            unsigned ife_f_nrs_gbilat_lut_w4     :  5;            //bits : 24..20
            unsigned ife_f_nrs_gbilat_lut_w5     :  5;            //bits : 29..25
        } bit;
        UINT32 word;
    } ife_register_332; // 0x530;

    union
    {
        struct
        {
            unsigned ife_f_nrs_gbilat_lut_r1     : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_nrs_gbilat_lut_r2     : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_333; // 0x534;

    union
    {
        struct
        {
            unsigned ife_f_nrs_gbilat_lut_r3     : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_nrs_gbilat_lut_r4     : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_334; // 0x538;

    union
    {
        struct
        {
            unsigned ife_f_nrs_gbilat_lut_r5     : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_nrs_gbilat_lut_th1    :  2;            //bits : 17..16
            unsigned ife_f_nrs_gbilat_lut_th2    :  2;            //bits : 19..18
            unsigned ife_f_nrs_gbilat_lut_th3    :  2;            //bits : 21..20
            unsigned ife_f_nrs_gbilat_lut_th4    :  2;            //bits : 23..22
            unsigned ife_f_nrs_gbilat_lut_th5    :  2;            //bits : 25..24
        } bit;
        UINT32 word;
    } ife_register_335; // 0x53c;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_336; // 0x540;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_337; // 0x544;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_338; // 0x548;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_339; // 0x54c;

    union
    {
        struct
        {
            unsigned ife_f_fu_ym_sel        :  2;            //bits : 1..0
            unsigned                        :  2;
            unsigned ife_f_fu_bcn_sel       :  2;            //bits : 5..4
            unsigned                        :  2;
            unsigned ife_f_fu_bcd_sel       :  2;            //bits : 9..8
            unsigned                        :  2;
            unsigned ife_f_fu_mode          :  2;            //bits : 13..12
            unsigned                        :  2;
            unsigned ife_f_fu_evratio       :  8;            //bits : 23..16
        } bit;
        UINT32 word;
    } ife_register_340; // 0x550;

    union
    {
        struct
        {
            unsigned ife_f_fu_bcnl_p0       : 12;            //bits : 11..0
            unsigned ife_f_fu_bcnl_range    :  4;            //bits : 15..12
            unsigned ife_f_fu_bcnl_p1       : 12;            //bits : 27..16
            unsigned                        :  3;
            unsigned ife_f_fu_bcnl_wedge    :  1;            //bits : 31
        } bit;
        UINT32 word;
    } ife_register_341; // 0x554;

    union
    {
        struct
        {
            unsigned ife_f_fu_bcns_p0       : 12;            //bits : 11..0
            unsigned ife_f_fu_bcns_range    :  4;            //bits : 15..12
            unsigned ife_f_fu_bcns_p1       : 12;            //bits : 27..16
            unsigned                        :  3;
            unsigned ife_f_fu_bcns_wedge    :  1;            //bits : 31
        } bit;
        UINT32 word;
    } ife_register_342; // 0x558;

    union
    {
        struct
        {
            unsigned ife_f_fu_bcdl_p0       : 12;            //bits : 11..0
            unsigned ife_f_fu_bcdl_range    :  4;            //bits : 15..12
            unsigned ife_f_fu_bcdl_p1       : 12;            //bits : 27..16
            unsigned                        :  3;
            unsigned ife_f_fu_bcdl_wedge    :  1;            //bits : 31
        } bit;
        UINT32 word;
    } ife_register_343; // 0x55c;

    union
    {
        struct
        {
            unsigned ife_f_fu_bcds_p0       : 12;            //bits : 11..0
            unsigned ife_f_fu_bcds_range    :  4;            //bits : 15..12
            unsigned ife_f_fu_bcds_p1       : 12;            //bits : 27..16
            unsigned                        :  3;
            unsigned ife_f_fu_bcds_wedge    :  1;            //bits : 31
        } bit;
        UINT32 word;
    } ife_register_344; // 0x560;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_345; // 0x564;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_346; // 0x568;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_347; // 0x56c;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lumthr     : 12;            //bits : 11..0
            unsigned ife_f_fu_mc_diff_ratio :  2;            //bits : 13..12
        } bit;
        UINT32 word;
    } ife_register_348; // 0x570;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lut_dwp0   :  5;            //bits : 4..0
            unsigned ife_f_fu_mc_lut_dwp1   :  5;            //bits : 9..5
            unsigned ife_f_fu_mc_lut_dwp2   :  5;            //bits : 14..10
            unsigned ife_f_fu_mc_lut_dwp3   :  5;            //bits : 19..15
            unsigned ife_f_fu_mc_lut_dwp4   :  5;            //bits : 24..20
            unsigned ife_f_fu_mc_lut_dwp5   :  5;            //bits : 29..25
        } bit;
        UINT32 word;
    } ife_register_349; // 0x574;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lut_dwp6   :  5;            //bits : 4..0
            unsigned ife_f_fu_mc_lut_dwp7   :  5;            //bits : 9..5
            unsigned ife_f_fu_mc_lut_dwp8   :  5;            //bits : 14..10
            unsigned ife_f_fu_mc_lut_dwp9   :  5;            //bits : 19..15
            unsigned ife_f_fu_mc_lut_dwp10  :  5;            //bits : 24..20
            unsigned ife_f_fu_mc_lut_dwp11  :  5;            //bits : 29..25
        } bit;
        UINT32 word;
    } ife_register_350; // 0x578;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lut_dwp12  :  5;            //bits : 4..0
            unsigned ife_f_fu_mc_lut_dwp13  :  5;            //bits : 9..5
            unsigned ife_f_fu_mc_lut_dwp14  :  5;            //bits : 14..10
            unsigned ife_f_fu_mc_lut_dwp15  :  5;            //bits : 19..15
            unsigned ife_f_fu_mc_lut_dwd    :  5;            //bits : 24..20
        } bit;
        UINT32 word;
    } ife_register_351; // 0x57c;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lut_dwn0   :  5;            //bits : 4..0
            unsigned ife_f_fu_mc_lut_dwn1   :  5;            //bits : 9..5
            unsigned ife_f_fu_mc_lut_dwn2   :  5;            //bits : 14..10
            unsigned ife_f_fu_mc_lut_dwn3   :  5;            //bits : 19..15
            unsigned ife_f_fu_mc_lut_dwn4   :  5;            //bits : 24..20
            unsigned ife_f_fu_mc_lut_dwn5   :  5;            //bits : 29..25
        } bit;
        UINT32 word;
    } ife_register_352; // 0x580;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lut_dwn6   :  5;            //bits : 4..0
            unsigned ife_f_fu_mc_lut_dwn7   :  5;            //bits : 9..5
            unsigned ife_f_fu_mc_lut_dwn8   :  5;            //bits : 14..10
            unsigned ife_f_fu_mc_lut_dwn9   :  5;            //bits : 19..15
            unsigned ife_f_fu_mc_lut_dwn10  :  5;            //bits : 24..20
            unsigned ife_f_fu_mc_lut_dwn11  :  5;            //bits : 29..25
        } bit;
        UINT32 word;
    } ife_register_353; // 0x584;

    union
    {
        struct
        {
            unsigned ife_f_fu_mc_lut_dwn12  :  5;            //bits : 4..0
            unsigned ife_f_fu_mc_lut_dwn13  :  5;            //bits : 9..5
            unsigned ife_f_fu_mc_lut_dwn14  :  5;            //bits : 14..10
            unsigned ife_f_fu_mc_lut_dwn15  :  5;            //bits : 19..15
        } bit;
        UINT32 word;
    } ife_register_354; // 0x588;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_355; // 0x58c;

    union
    {
        struct
        {
            unsigned ife_f_ds0_th           : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_ds0_step         :  8;            //bits : 23..16
            unsigned ife_f_ds0_lb           :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_356; // 0x590;

    union
    {
        struct
        {
            unsigned ife_f_ds1_th           : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_ds1_step         :  8;            //bits : 23..16
            unsigned ife_f_ds1_lb           :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_357; // 0x594;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_358; // 0x598;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_359; // 0x59c;

    union
    {
        struct
        {
            unsigned ife_f_sc_kp0           : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_sc_kp1           : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_360; // 0x5a0;

    union
    {
        struct
        {
            unsigned ife_f_sc_kp2           : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_sc_en            :  1;            //bits : 16
        } bit;
        UINT32 word;
    } ife_register_361; // 0x5a4;

    union
    {
        struct
        {
            unsigned ife_f_sc_sp0           : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_sc_sp1           : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_362; // 0x5a8;

    union
    {
        struct
        {
            unsigned ife_f_sc_sp2           : 12;            //bits : 11..0
            unsigned                        :  4;
            unsigned ife_f_sc_sp3           : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_363; // 0x5ac;

    union
    {
        struct
        {
            unsigned ife_f_sc_sb0           :  3;            //bits : 2..0
            unsigned                        :  1;
            unsigned ife_f_sc_sb1           :  3;            //bits : 6..4
            unsigned                        :  1;
            unsigned ife_f_sc_sb2           :  3;            //bits : 10..8
            unsigned                        :  1;
            unsigned ife_f_sc_sb3           :  3;            //bits : 14..12
        } bit;
        UINT32 word;
    } ife_register_364; // 0x5b0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_365; // 0x5b4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_366; // 0x5b8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_367; // 0x5bc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_368; // 0x5c0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_369; // 0x5c4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_370; // 0x5c8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_371; // 0x5cc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_372; // 0x5d0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_373; // 0x5d4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_374; // 0x5d8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_375; // 0x5dc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_376; // 0x5e0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_377; // 0x5e4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_378; // 0x5e8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_379; // 0x5ec;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_380; // 0x5f0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_381; // 0x5f4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_382; // 0x5f8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_383; // 0x5fc;

    union
    {
        struct
        {
            unsigned ife_f_fc_ym_sel        :  2;            //bits : 1..0
            unsigned ife_f_fc_yvweight      :  4;            //bits : 5..2
        } bit;
        UINT32 word;
    } ife_register_384; // 0x600;

    union
    {
        struct
        {
            unsigned ife_f_fc_yweight_lut0  :  8;            //bits : 7..0
            unsigned ife_f_fc_yweight_lut1  :  8;            //bits : 15..8
            unsigned ife_f_fc_yweight_lut2  :  8;            //bits : 23..16
            unsigned ife_f_fc_yweight_lut3  :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_385; // 0x604;

    union
    {
        struct
        {
            unsigned ife_f_fc_yweight_lut4  :  8;            //bits : 7..0
            unsigned ife_f_fc_yweight_lut5  :  8;            //bits : 15..8
            unsigned ife_f_fc_yweight_lut6  :  8;            //bits : 23..16
            unsigned ife_f_fc_yweight_lut7  :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_386; // 0x608;

    union
    {
        struct
        {
            unsigned ife_f_fc_yweight_lut8  :  8;            //bits : 7..0
            unsigned ife_f_fc_yweight_lut9  :  8;            //bits : 15..8
            unsigned ife_f_fc_yweight_lut10 :  8;            //bits : 23..16
            unsigned ife_f_fc_yweight_lut11 :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_387; // 0x60c;

    union
    {
        struct
        {
            unsigned ife_f_fc_yweight_lut12 :  8;            //bits : 7..0
            unsigned ife_f_fc_yweight_lut13 :  8;            //bits : 15..8
            unsigned ife_f_fc_yweight_lut14 :  8;            //bits : 23..16
            unsigned ife_f_fc_yweight_lut15 :  8;            //bits : 31..24
        } bit;
        UINT32 word;
    } ife_register_388; // 0x610;

    union
    {
        struct
        {
            unsigned ife_f_fc_yweight_lut16 :  8;            //bits : 7..0
        } bit;
        UINT32 word;
    } ife_register_389; // 0x614;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut0    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut1    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut2    :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut3    :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_390; // 0x618;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut4    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut5    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut6    :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut7    :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_391; // 0x61c;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut8    :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut9    :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut10   :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut11   :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_392; // 0x620;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut12   :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut13   :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut14   :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut15   :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_393; // 0x624;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut16   :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut17   :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut18   :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut19   :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_394; // 0x628;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut20   :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut21   :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut22   :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut23   :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_395; // 0x62c;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut24   :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut25   :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut26   :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut27   :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_396; // 0x630;

    union
    {
        struct
        {
            unsigned ife_f_fc_index_lut28   :  6;            //bits : 5..0
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut29   :  6;            //bits : 13..8
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut30   :  6;            //bits : 21..16
            unsigned                        :  2;
            unsigned ife_f_fc_index_lut31   :  6;            //bits : 29..24
        } bit;
        UINT32 word;
    } ife_register_397; // 0x634;

    union
    {
        struct
        {
            unsigned ife_f_fc_split_lut0    :  2;            //bits : 1..0
            unsigned ife_f_fc_split_lut1    :  2;            //bits : 3..2
            unsigned ife_f_fc_split_lut2    :  2;            //bits : 5..4
            unsigned ife_f_fc_split_lut3    :  2;            //bits : 7..6
            unsigned ife_f_fc_split_lut4    :  2;            //bits : 9..8
            unsigned ife_f_fc_split_lut5    :  2;            //bits : 11..10
            unsigned ife_f_fc_split_lut6    :  2;            //bits : 13..12
            unsigned ife_f_fc_split_lut7    :  2;            //bits : 15..14
            unsigned ife_f_fc_split_lut8    :  2;            //bits : 17..16
            unsigned ife_f_fc_split_lut9    :  2;            //bits : 19..18
            unsigned ife_f_fc_split_lut10   :  2;            //bits : 21..20
            unsigned ife_f_fc_split_lut11   :  2;            //bits : 23..22
            unsigned ife_f_fc_split_lut12   :  2;            //bits : 25..24
            unsigned ife_f_fc_split_lut13   :  2;            //bits : 27..26
            unsigned ife_f_fc_split_lut14   :  2;            //bits : 29..28
            unsigned ife_f_fc_split_lut15   :  2;            //bits : 31..30
        } bit;
        UINT32 word;
    } ife_register_398; // 0x638;

    union
    {
        struct
        {
            unsigned ife_f_fc_split_lut16   :  2;            //bits : 1..0
            unsigned ife_f_fc_split_lut17   :  2;            //bits : 3..2
            unsigned ife_f_fc_split_lut18   :  2;            //bits : 5..4
            unsigned ife_f_fc_split_lut19   :  2;            //bits : 7..6
            unsigned ife_f_fc_split_lut20   :  2;            //bits : 9..8
            unsigned ife_f_fc_split_lut21   :  2;            //bits : 11..10
            unsigned ife_f_fc_split_lut22   :  2;            //bits : 13..12
            unsigned ife_f_fc_split_lut23   :  2;            //bits : 15..14
            unsigned ife_f_fc_split_lut24   :  2;            //bits : 17..16
            unsigned ife_f_fc_split_lut25   :  2;            //bits : 19..18
            unsigned ife_f_fc_split_lut26   :  2;            //bits : 21..20
            unsigned ife_f_fc_split_lut27   :  2;            //bits : 23..22
            unsigned ife_f_fc_split_lut28   :  2;            //bits : 25..24
            unsigned ife_f_fc_split_lut29   :  2;            //bits : 27..26
            unsigned ife_f_fc_split_lut30   :  2;            //bits : 29..28
            unsigned ife_f_fc_split_lut31   :  2;            //bits : 31..30
        } bit;
        UINT32 word;
    } ife_register_399; // 0x63c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut00   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut01   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_400; // 0x640;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut02   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut03   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_401; // 0x644;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut04   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut05   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_402; // 0x648;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut06   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut07   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_403; // 0x64c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut08   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut09   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_404; // 0x650;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut10   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut11   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_405; // 0x654;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut12   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut13   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_406; // 0x658;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut14   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut15   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_407; // 0x65c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut16   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut17   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_408; // 0x660;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut18   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut19   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_409; // 0x664;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut20   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut21   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_410; // 0x668;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut22   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut23   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_411; // 0x66c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut24   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut25   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_412; // 0x670;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut26   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut27   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_413; // 0x674;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut28   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut29   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_414; // 0x678;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut30   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut31   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_415; // 0x67c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut32   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut33   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_416; // 0x680;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut34   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut35   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_417; // 0x684;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut36   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut37   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_418; // 0x688;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut38   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut39   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_419; // 0x68c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut40   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut41   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_420; // 0x690;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut42   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut43   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_421; // 0x694;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut44   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut45   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_422; // 0x698;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut46   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut47   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_423; // 0x69c;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut48   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut49   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_424; // 0x6a0;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut50   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut51   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_425; // 0x6a4;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut52   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut53   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_426; // 0x6a8;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut54   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut55   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_427; // 0x6ac;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut56   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut57   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_428; // 0x6b0;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut58   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut59   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_429; // 0x6b4;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut60   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut61   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_430; // 0x6b8;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut62   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_f_fc_val_lut63   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_431; // 0x6bc;

    union
    {
        struct
        {
            unsigned ife_f_fc_val_lut64     : 12;            //bits : 11..0
        } bit;
        UINT32 word;
    } ife_register_432; // 0x6c0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_433; // 0x6c4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_434; // 0x6c8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_435; // 0x6cc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_436; // 0x6d0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_437; // 0x6d4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_438; // 0x6d8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_439; // 0x6dc;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_440; // 0x6e0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_441; // 0x6e4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_442; // 0x6e8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_443; // 0x6ec;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_444; // 0x6f0;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_445; // 0x6f4;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_446; // 0x6f8;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_447; // 0x6fc;

    union
    {
        struct
        {
            unsigned ife_r_degamma_en       :  1;            //bits : 0
            unsigned                        :  3;
            unsigned ife_r_dith_en          :  1;            //bits : 4
            unsigned                        :  3;
            unsigned ife_r_segbitno         :  2;            //bits : 9..8
            unsigned                        :  2;
            unsigned ife_r_dith_rst         :  1;            //bits : 12
        } bit;
        UINT32 word;
    } ife_register_448; // 0x700;

    union
    {
        struct
        {
            unsigned ife_r_dct_qtbl0_idx    :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_r_dct_qtbl1_idx    :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_r_dct_qtbl2_idx    :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_r_dct_qtbl3_idx    :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_449; // 0x704;

    union
    {
        struct
        {
            unsigned ife_r_dct_qtbl4_idx    :  5;            //bits : 4..0
            unsigned                        :  3;
            unsigned ife_r_dct_qtbl5_idx    :  5;            //bits : 12..8
            unsigned                        :  3;
            unsigned ife_r_dct_qtbl6_idx    :  5;            //bits : 20..16
            unsigned                        :  3;
            unsigned ife_r_dct_qtbl7_idx    :  5;            //bits : 28..24
        } bit;
        UINT32 word;
    } ife_register_450; // 0x708;

    union
    {
        struct
        {
        } bit;
        UINT32 word;
    } ife_register_451; // 0x70c;

    union
    {
        struct
        {
            unsigned ife_r_out_rand_init1  :  4;            //bits : 3..0
            unsigned ife_r_out_rand_init2  : 15;            //bits : 18..4
        } bit;
        UINT32 word;
    } ife_register_452; // 0x710;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl00   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl01   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_453; // 0x714;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl02   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl03   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_454; // 0x718;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl04   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl05   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_455; // 0x71c;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl06   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl07   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_456; // 0x720;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl08   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl09   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_457; // 0x724;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl10   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl11   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_458; // 0x728;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl12   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl13   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_459; // 0x72c;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl14   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl15   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_460; // 0x730;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl16   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl17   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_461; // 0x734;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl18   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl19   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_462; // 0x738;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl20   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl21   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_463; // 0x73c;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl22   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl23   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_464; // 0x740;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl24   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl25   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_465; // 0x744;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl26   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl27   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_466; // 0x748;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl28   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl29   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_467; // 0x74c;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl30   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl31   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_468; // 0x750;

    union
    {
        struct
        {
            unsigned ife_r_degamma_tbl32   : 12;            //bits : 11..0
            unsigned                      :  4;
            unsigned ife_r_degamma_tbl33   : 12;            //bits : 27..16
        } bit;
        UINT32 word;
    } ife_register_469; // 0x754;

} NT98560_IFE_REG_STRUCT;


/*
#define IFE_REG_ADDR(ofs)        (_IFE_REG_BASE_ADDR+(ofs))
#define IFE_SETREG(ofs, value)   OUTW((_IFE_REG_BASE_ADDR + ofs), value)
#define IFE_GETREG(ofs)          INW(_IFE_REG_BASE_ADDR + ofs)
*/
/*
    ife_sw_rst       :    [0x0, 0x1],           bits : 0
    ife_start        :    [0x0, 0x1],           bits : 1
    ife_load_start   :    [0x0, 0x1],           bits : 2
    ife_load_fd      :    [0x0, 0x1],           bits : 3
    ife_load_frmstart:    [0x0, 0x1],           bits : 4
    ife_ll_fire      :    [0x0, 0x1],           bits : 28
    ife_ll_terminate :    [0x0, 0x1],           bits : 29
*/
#define FILTER_OPERATION_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(FILTER_OPERATION_CONTROL_REGISTER)
REGDEF_BIT(ife_sw_rst,        1)
REGDEF_BIT(ife_start,        1)
REGDEF_BIT(ife_load_start,        1)
REGDEF_BIT(ife_load_fd,        1)
REGDEF_BIT(ife_load_frmstart,        1)
REGDEF_BIT(ife_global_load_en,        1)
REGDEF_BIT(,        22)
REGDEF_BIT(ife_ll_fire,        1)
REGDEF_BIT(ife_ll_terminate,        1)
REGDEF_END(FILTER_OPERATION_CONTROL_REGISTER)


/*
    IFE_MODE           :    [0x0, 0x3],         bits : 1_0
    INBIT_16_FMT_SEL   :    [0x0, 0x1],         bits : 4
    INBIT_DEPTH        :    [0x0, 0x3],         bits : 6_5
    OUTBIT_DEPTH       :    [0x0, 0x3],         bits : 8_7
    CFAPAT             :    [0x0, 0x7],         bits : 11_9
    IFE_FILT_MODE      :    [0x0, 0x1],         bits : 12
    IFE_OUTL_EN        :    [0x0, 0x1],         bits : 13
    IFE_FILTER_EN      :    [0x0, 0x1],         bits : 14
    IFE_CGAIN_EN       :    [0x0, 0x1],         bits : 15
    IFE_VIG_EN         :    [0x0, 0x1],         bits : 16
    IFE_GBAL_EN        :    [0x0, 0x1],         bits : 17
    IFE_BINNING        :    [0x0, 0x7],         bits : 20_18
    BAYER_FORMAT       :    [0x0, 0x1],         bits : 21
    IFE_RGBIR_RB_NRFILL:    [0x0, 0x1],         bits : 22
    IFE_BILAT_TH_EN    :    [0x0, 0x1],         bits : 23
    IFE_F_NRS_EN       :    [0x0, 0x1],         bits : 24
    IFE_F_CG_EN        :    [0x0, 0x1],         bits : 25
    IFE_F_FUSION_EN    :    [0x0, 0x1],         bits : 26
    IFE_F_FUSION_FNUM  :    [0x0, 0x3],         bits : 28_27
    IFE_F_FC_EN        :    [0x0, 0x1],         bits : 29
    MIRROR_EN          :    [0x0, 0x1],         bits : 30
    IFE_R_DECODE_EN    :    [0x0, 0x1],         bits : 31
*/
#define CONTROL_REGISTER_OFS 0x0004
REGDEF_BEGIN(CONTROL_REGISTER)
REGDEF_BIT(ife_mode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(inbit_16_fmt_sel,	1)
REGDEF_BIT(inbit_depth,        2)
REGDEF_BIT(outbit_depth,        2)
REGDEF_BIT(cfapat,        3)
REGDEF_BIT(filt_mode,        1)
REGDEF_BIT(outl_en,        1)
REGDEF_BIT(filter_en,        1)
REGDEF_BIT(cgain_en,        1)
REGDEF_BIT(vig_en,        1)
REGDEF_BIT(gbal_en,        1)
REGDEF_BIT(binning,        3)
REGDEF_BIT(bayer_fmt,        1)
REGDEF_BIT(rgbir_rb_nrfill,        1)
REGDEF_BIT(bilat_th_en,        1)
REGDEF_BIT(f_nrs_en,        1)
REGDEF_BIT(f_cg_en,        1)
REGDEF_BIT(f_fusion_en,        1)
REGDEF_BIT(f_fusion_fnum,        2)
REGDEF_BIT(f_fc_en,        1)
REGDEF_BIT(mirror_en,        1)
REGDEF_BIT(r_decode_en,        1)
REGDEF_END(CONTROL_REGISTER)


/*
    inte_frmend         :    [0x0, 0x1],            bits : 0
    inte_ife_r_dec2_err :    [0x0, 0x1],            bits : 2
    inte_llend          :    [0x0, 0x1],            bits : 3
    inte_llerror        :    [0x0, 0x1],            bits : 4
    inte_llerror2       :    [0x0, 0x1],            bits : 5
    inte_lljobend       :    [0x0, 0x1],            bits : 6
    inte_ife_bufovfl    :    [0x0, 0x1],            bits : 7
    inte_ife_ringbuf_err:    [0x0, 0x1],            bits : 8
*/
#define FILTER_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(FILTER_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(inte_frmend,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(inte_ife_r_dec2_err,        1)
REGDEF_BIT(inte_llend,        1)
REGDEF_BIT(inte_llerror,        1)
REGDEF_BIT(inte_llerror2,        1)
REGDEF_BIT(inte_lljobend,        1)
REGDEF_BIT(inte_ife_bufovfl,        1)
REGDEF_BIT(inte_ife_ringbuf_err,        1)
REGDEF_END(FILTER_INTERRUPT_ENABLE_REGISTER)


/*
    int_frmend         :    [0x0, 0x1],         bits : 0
    int_ife_r_dec2_err :    [0x0, 0x1],         bits : 2
    int_llend          :    [0x0, 0x1],         bits : 3
    int_llerror        :    [0x0, 0x1],         bits : 4
    int_llerror2       :    [0x0, 0x1],         bits : 5
    int_lljobend       :    [0x0, 0x1],         bits : 6
    int_ife_bufovfl    :    [0x0, 0x1],         bits : 7
    int_ife_ringbuf_err:    [0x0, 0x1],         bits : 8
*/
#define FILTER_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(FILTER_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(int_frmend,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(int_ife_r_dec2_err,        1)
REGDEF_BIT(int_llend,        1)
REGDEF_BIT(int_llerror,        1)
REGDEF_BIT(int_llerror2,        1)
REGDEF_BIT(int_lljobend,        1)
REGDEF_BIT(int_ife_bufovfl,        1)
REGDEF_BIT(int_ife_ringbuf_err,        1)
REGDEF_END(FILTER_INTERRUPT_STATUS_REGISTER)


/*
    ife_busy:    [0x0, 0x1],            bits : 0
    reserved:    [0x0, 0x7fffffff],         bits : 31_1
*/
#define DEBUG_REGISTER_OFS 0x0010
REGDEF_BEGIN(DEBUG_REGISTER)
REGDEF_BIT(ife_busy,        1)
REGDEF_BIT(reserved,        31)
REGDEF_END(DEBUG_REGISTER)


/*
    ife_hn :    [0x0, 0x3ff],           bits : 9_0
    ife_hl :    [0x0, 0x3ff],           bits : 21_12
    ife_hm :    [0x0, 0xf],         bits : 27_24
*/
#define STRIPE_REGISTER_OFS 0x0014
REGDEF_BEGIN(STRIPE_REGISTER)
REGDEF_BIT(ife_hn,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_hl,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_hm,        4)
REGDEF_END(STRIPE_REGISTER)

/*
    ife_dmach_idle :    [0x0, 0x1],          bits : 0
    ife_dmach_dis :    [0x0, 0x1],           bits : 1
*/
#define IFE_DMA_DISABLE_REGISTER_OFS 0x001c
REGDEF_BEGIN(IFE_DMA_DISABLE_REGISTER)
REGDEF_BIT(ife_dmach_idle,        1)
REGDEF_BIT(ife_dmach_dis,		  1)
REGDEF_END(IFE_DMA_DISABLE_REGISTER)


/*
    width :    [0x0, 0x3fff],           bits : 15_2
    height:    [0x0, 0x7fff],           bits : 31_17
*/
#define SOURCE_SIZE_REGISTER_0_OFS 0x0020
REGDEF_BEGIN(SOURCE_SIZE_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(width,        14)
REGDEF_BIT(,        1)
REGDEF_BIT(height,        15)
REGDEF_END(SOURCE_SIZE_REGISTER_0)


/*
    crop_width :    [0x0, 0x3fff],          bits : 15_2
    crop_height:    [0x0, 0x7fff],          bits : 31_17
*/
#define SOURCE_SIZE_REGISTER_1_OFS 0x0024
REGDEF_BEGIN(SOURCE_SIZE_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(crop_width,        14)
REGDEF_BIT(,        1)
REGDEF_BIT(crop_height,        15)
REGDEF_END(SOURCE_SIZE_REGISTER_1)


/*
    crop_hpos:    [0x0, 0xffff],            bits : 15_0
    crop_vpos:    [0x0, 0xffff],            bits : 31_16
*/
#define SOURCE_SIZE_REGISTER_2_OFS 0x0028
REGDEF_BEGIN(SOURCE_SIZE_REGISTER_2)
REGDEF_BIT(crop_hpos,        16)
REGDEF_BIT(crop_vpos,        16)
REGDEF_END(SOURCE_SIZE_REGISTER_2)

/*
    ife_line_count :    [0x0, 0xffff],            bits : 15_0
    sie2_line_count:    [0x0, 0xffff],            bits : 31_16
*/
#define DMALOOP_ERROR_LINE_COUNT_REGISTER_OFS 0x002C
REGDEF_BEGIN(DMALOOP_ERROR_LINE_COUNT_REGISTER)
REGDEF_BIT(ife_line_count,        16)
REGDEF_BIT(sie2_line_count,       16)
REGDEF_END(DMALOOP_ERROR_LINE_COUNT_REGISTER)

/*
    dram_sai0:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define SOURCE_ADDRESS_REGISTER_0_OFS 0x0030
REGDEF_BEGIN(SOURCE_ADDRESS_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sai0,        30)
REGDEF_END(SOURCE_ADDRESS_REGISTER_0)


/*
    dram_ofsi0   :    [0x0, 0x3fff],            bits : 15_2
    h_start_shift:    [0x0, 0xf],           bits : 19_16
*/
#define SOURCE_LINE_OFFSET_REGISTER_0_OFS 0x0034
REGDEF_BEGIN(SOURCE_LINE_OFFSET_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofsi0,        14)
REGDEF_BIT(h_start_shift,        4)
REGDEF_END(SOURCE_LINE_OFFSET_REGISTER_0)


/*
    dram_sai1:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define SOURCE_ADDRESS_REGISTER_1_OFS 0x0038
REGDEF_BEGIN(SOURCE_ADDRESS_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sai1,        30)
REGDEF_END(SOURCE_ADDRESS_REGISTER_1)


/*
    dram_ofsi1:    [0x0, 0x3fff],           bits : 15_2
*/
#define SOURCE_LINE_OFFSET_REGISTER_1_OFS 0x003c
REGDEF_BEGIN(SOURCE_LINE_OFFSET_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofsi1,        14)
REGDEF_END(SOURCE_LINE_OFFSET_REGISTER_1)

/*
    dram_saill:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define SOURCE_ADDRESS_REGISTER_2_OFS 0x0050
REGDEF_BEGIN(SOURCE_ADDRESS_REGISTER_2)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_saill,        30)
REGDEF_END(SOURCE_ADDRESS_REGISTER_2)


/*
    dram_sao:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define DESTINATION_ADDRESS_REGISTER_OFS 0x0054
REGDEF_BEGIN(DESTINATION_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sao,        30)
REGDEF_END(DESTINATION_ADDRESS_REGISTER)


/*
    dram_ofso:    [0x0, 0x3fff],            bits : 15_2
*/
#define DESTINATION_LINE_OFFSET_REGISTER_OFS 0x0058
REGDEF_BEGIN(DESTINATION_LINE_OFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofso,        14)
REGDEF_END(DESTINATION_LINE_OFFSET_REGISTER)


/*
    input_burst_mode :    [0x0, 0x1],           bits : 0
    output_burst_mode:    [0x0, 0x1],           bits : 4
    dmaloop_line     :    [0x0, 0x3ff],         bits : 21_12
    dmaloop_en       :    [0x0, 0x1],           bits : 24
    dmaloop_ctrl     :    [0x0, 0x1],           bits : 25    
*/
#define DRAM_SETTINGS_OFS 0x005c
REGDEF_BEGIN(DRAM_SETTINGS)
REGDEF_BIT(input_burst_mode,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(output_burst_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(dmaloop_line,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(dmaloop_en,        1)
REGDEF_BIT(dmaloop_ctrl,		1)
REGDEF_END(DRAM_SETTINGS)

/*
    cgain_inv        :    [0x0, 0x1],           bits : 0
    cgain_hinv       :    [0x0, 0x1],           bits : 1
    cgain_range      :    [0x0, 0x1],           bits : 2
    ife_f_cgain_range:    [0x0, 0x1],           bits : 3
    cgain_mask       :    [0x0, 0xfff],         bits : 19_8
*/
#define COLOR_GAIN_REGISTER_0_OFS 0x0070
REGDEF_BEGIN(COLOR_GAIN_REGISTER_0)
REGDEF_BIT(cgain_inv,        1)
REGDEF_BIT(cgain_hinv,        1)
REGDEF_BIT(cgain_range,        1)
REGDEF_BIT(ife_f_cgain_range,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(cgain_mask,        12)
REGDEF_END(COLOR_GAIN_REGISTER_0)


/*
    ife_cgain_r :    [0x0, 0x3ff],          bits : 9_0
    ife_cgain_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_GAIN_REGISTER_1_OFS 0x0074
REGDEF_BEGIN(COLOR_GAIN_REGISTER_1)
REGDEF_BIT(ife_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_1)


/*
    ife_cgain_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_cgain_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_GAIN_REGISTER_2_OFS 0x0078
REGDEF_BEGIN(COLOR_GAIN_REGISTER_2)
REGDEF_BIT(ife_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_2)


/*
    ife_cgain_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_GAIN_REGISTER_3_OFS 0x007c
REGDEF_BEGIN(COLOR_GAIN_REGISTER_3)
REGDEF_BIT(ife_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_3)


/*
    ife_f_p0_cgain_r :    [0x0, 0x3ff],         bits : 9_0
    ife_f_p0_cgain_gr:    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_4_OFS 0x0080
REGDEF_BEGIN(COLOR_GAIN_REGISTER_4)
REGDEF_BIT(ife_f_p0_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_4)


/*
    ife_f_p0_cgain_gb:    [0x0, 0x3ff],         bits : 9_0
    ife_f_p0_cgain_b :    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_5_OFS 0x0084
REGDEF_BEGIN(COLOR_GAIN_REGISTER_5)
REGDEF_BIT(ife_f_p0_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_5)


/*
    ife_f_p0_cgain_ir:    [0x0, 0x3ff],         bits : 9_0
*/
#define COLOR_GAIN_REGISTER_6_OFS 0x0088
REGDEF_BEGIN(COLOR_GAIN_REGISTER_6)
REGDEF_BIT(ife_f_p0_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_6)


/*
    ife_f_p1_cgain_r :    [0x0, 0x3ff],         bits : 9_0
    ife_f_p1_cgain_gr:    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_7_OFS 0x008c
REGDEF_BEGIN(COLOR_GAIN_REGISTER_7)
REGDEF_BIT(ife_f_p1_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_7)


/*
    ife_f_p1_cgain_gb:    [0x0, 0x3ff],         bits : 9_0
    ife_f_p1_cgain_b :    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_8_OFS 0x0090
REGDEF_BEGIN(COLOR_GAIN_REGISTER_8)
REGDEF_BIT(ife_f_p1_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_8)


/*
    ife_f_p1_cgain_ir:    [0x0, 0x3ff],         bits : 9_0
*/
#define COLOR_GAIN_REGISTER_9_OFS 0x0094
REGDEF_BEGIN(COLOR_GAIN_REGISTER_9)
REGDEF_BIT(ife_f_p1_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_9)

/*
    ife_cofs_r :    [0x0, 0x3ff],           bits : 9_0
    ife_cofs_gr:    [0x0, 0x3ff],           bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_0_OFS 0x00b0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_0)
REGDEF_BIT(ife_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_0)


/*
    ife_cofs_gb:    [0x0, 0x3ff],           bits : 9_0
    ife_cofs_b :    [0x0, 0x3ff],           bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_1_OFS 0x00b4
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_1)
REGDEF_BIT(ife_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_1)


/*
    ife_cofs_ir:    [0x0, 0x3ff],           bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_2_OFS 0x00b8
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_2)
REGDEF_BIT(ife_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_2)


/*
    ife_f_p0_cofs_r :    [0x0, 0x3ff],          bits : 9_0
    ife_f_p0_cofs_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_3_OFS 0x00bc
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_3)
REGDEF_BIT(ife_f_p0_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_3)


/*
    ife_f_p0_cofs_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_f_p0_cofs_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_4_OFS 0x00c0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_4)
REGDEF_BIT(ife_f_p0_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_4)


/*
    ife_f_p0_cofs_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_5_OFS 0x00c4
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_5)
REGDEF_BIT(ife_f_p0_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_5)


/*
    ife_f_p1_cofs_r :    [0x0, 0x3ff],          bits : 9_0
    ife_f_p1_cofs_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_6_OFS 0x00c8
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_6)
REGDEF_BIT(ife_f_p1_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_6)


/*
    ife_f_p1_cofs_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_f_p1_cofs_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_7_OFS 0x00cc
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_7)
REGDEF_BIT(ife_f_p1_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_7)


/*
    ife_f_p1_cofs_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_8_OFS 0x00d0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_8)
REGDEF_BIT(ife_f_p1_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_8)

/*
    ife_clamp_th :    [0x0, 0xfff],         bits : 11_0
    ife_clamp_mul:    [0x0, 0xff],          bits : 19_12
    ife_clamp_dlt:    [0x0, 0xfff],         bits : 31_20
*/
#define OUTPUT_FILTER_REGISTER_OFS 0x0100
REGDEF_BEGIN(OUTPUT_FILTER_REGISTER)
REGDEF_BIT(ife_clamp_th,        12)
REGDEF_BIT(ife_clamp_mul,        8)
REGDEF_BIT(ife_clamp_dlt,        12)
REGDEF_END(OUTPUT_FILTER_REGISTER)


/*
    ife_bilat_w  :    [0x0, 0xf],           bits : 3_0
    ife_rth_w    :    [0x0, 0xf],           bits : 7_4
    ife_bilat_th1:    [0x0, 0x3ff],         bits : 17_8
    ife_bilat_th2:    [0x0, 0x3ff],         bits : 27_18
    ife_bilat_cen_sel:[0x0, 0x1],           bits : 28
*/
#define RANGE_FILTER_REGISTER_0_OFS 0x0104
REGDEF_BEGIN(RANGE_FILTER_REGISTER_0)
REGDEF_BIT(ife_bilat_w,        4)
REGDEF_BIT(ife_rth_w,        4)
REGDEF_BIT(ife_bilat_th1,        10)
REGDEF_BIT(ife_bilat_th2,        10)
REGDEF_BIT(ife_bilat_cen_sel,	 1)
REGDEF_END(RANGE_FILTER_REGISTER_0)


/*
    IFE_S_WEIGHT0:    [0x0, 0x1f],          bits : 4_0
    IFE_S_WEIGHT1:    [0x0, 0x1f],          bits : 12_8
    IFE_S_WEIGHT2:    [0x0, 0x1f],          bits : 20_16
    IFE_S_WEIGHT3:    [0x0, 0x1f],          bits : 28_24
*/
#define SPATIAL_FILTER_REGISTER_1_OFS 0x0108
REGDEF_BEGIN(SPATIAL_FILTER_REGISTER_1)
REGDEF_BIT(ife_spatial_weight0,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight1,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight2,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight3,        5)
REGDEF_END(SPATIAL_FILTER_REGISTER_1)


/*
    IFE_S_WEIGHT4:    [0x0, 0x1f],          bits : 4_0
    IFE_S_WEIGHT5:    [0x0, 0x1f],          bits : 12_8
*/
#define SPATIAL_FILTER_REGISTER_2_OFS 0x010c
REGDEF_BEGIN(SPATIAL_FILTER_REGISTER_2)
REGDEF_BIT(ife_spatial_weight4,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight5,        5)
REGDEF_END(SPATIAL_FILTER_REGISTER_2)


/*
    ife_rth_nlm_c0_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_1_OFS 0x0110
REGDEF_BEGIN(RANGE_FILTER_REGISTER_1)
REGDEF_BIT(ife_rth_nlm_c0_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_1)


/*
    ife_rth_nlm_c0_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_2_OFS 0x0114
REGDEF_BEGIN(RANGE_FILTER_REGISTER_2)
REGDEF_BIT(ife_rth_nlm_c0_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_2)


/*
    ife_rth_nlm_c0_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_3_OFS 0x0118
REGDEF_BEGIN(RANGE_FILTER_REGISTER_3)
REGDEF_BIT(ife_rth_nlm_c0_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_4_OFS 0x011c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_4)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_4)


/*
    ife_rth_nlm_c1_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_5_OFS 0x0120
REGDEF_BEGIN(RANGE_FILTER_REGISTER_5)
REGDEF_BIT(ife_rth_nlm_c1_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_5)


/*
    ife_rth_nlm_c1_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_6_OFS 0x0124
REGDEF_BEGIN(RANGE_FILTER_REGISTER_6)
REGDEF_BIT(ife_rth_nlm_c1_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_6)


/*
    ife_rth_nlm_c1_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_7_OFS 0x0128
REGDEF_BEGIN(RANGE_FILTER_REGISTER_7)
REGDEF_BIT(ife_rth_nlm_c1_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_7)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_8_OFS 0x012c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_8)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_8)


/*
    ife_rth_nlm_c2_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_9_OFS 0x0130
REGDEF_BEGIN(RANGE_FILTER_REGISTER_9)
REGDEF_BIT(ife_rth_nlm_c2_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_9)


/*
    ife_rth_nlm_c2_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_10_OFS 0x0134
REGDEF_BEGIN(RANGE_FILTER_REGISTER_10)
REGDEF_BIT(ife_rth_nlm_c2_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_10)


/*
    ife_rth_nlm_c2_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_11_OFS 0x0138
REGDEF_BEGIN(RANGE_FILTER_REGISTER_11)
REGDEF_BIT(ife_rth_nlm_c2_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_11)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_12_OFS 0x013c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_12)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_12)


/*
    ife_rth_nlm_c3_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_13_OFS 0x0140
REGDEF_BEGIN(RANGE_FILTER_REGISTER_13)
REGDEF_BIT(ife_rth_nlm_c3_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_13)


/*
    ife_rth_nlm_c3_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_14_OFS 0x0144
REGDEF_BEGIN(RANGE_FILTER_REGISTER_14)
REGDEF_BIT(ife_rth_nlm_c3_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_14)


/*
    ife_rth_nlm_c3_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_15_OFS 0x0148
REGDEF_BEGIN(RANGE_FILTER_REGISTER_15)
REGDEF_BIT(ife_rth_nlm_c3_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_15)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_16_OFS 0x014c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_16)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_16)


/*
    ife_rth_bilat_c0_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_17_OFS 0x0150
REGDEF_BEGIN(RANGE_FILTER_REGISTER_17)
REGDEF_BIT(ife_rth_bilat_c0_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_17)


/*
    ife_rth_bilat_c0_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_18_OFS 0x0154
REGDEF_BEGIN(RANGE_FILTER_REGISTER_18)
REGDEF_BIT(ife_rth_bilat_c0_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_18)


/*
    ife_rth_bilat_c0_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_19_OFS 0x0158
REGDEF_BEGIN(RANGE_FILTER_REGISTER_19)
REGDEF_BIT(ife_rth_bilat_c0_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_19)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_20_OFS 0x015c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_20)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_20)


/*
    ife_rth_bilat_c1_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_21_OFS 0x0160
REGDEF_BEGIN(RANGE_FILTER_REGISTER_21)
REGDEF_BIT(ife_rth_bilat_c1_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_21)


/*
    ife_rth_bilat_c1_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_22_OFS 0x0164
REGDEF_BEGIN(RANGE_FILTER_REGISTER_22)
REGDEF_BIT(ife_rth_bilat_c1_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_22)


/*
    ife_rth_bilat_c1_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_23_OFS 0x0168
REGDEF_BEGIN(RANGE_FILTER_REGISTER_23)
REGDEF_BIT(ife_rth_bilat_c1_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_23)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_24_OFS 0x016c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_24)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_24)


/*
    ife_rth_bilat_c2_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_25_OFS 0x0170
REGDEF_BEGIN(RANGE_FILTER_REGISTER_25)
REGDEF_BIT(ife_rth_bilat_c2_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_25)


/*
    ife_rth_bilat_c2_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_26_OFS 0x0174
REGDEF_BEGIN(RANGE_FILTER_REGISTER_26)
REGDEF_BIT(ife_rth_bilat_c2_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_26)


/*
    ife_rth_bilat_c2_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_27_OFS 0x0178
REGDEF_BEGIN(RANGE_FILTER_REGISTER_27)
REGDEF_BIT(ife_rth_bilat_c2_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_27)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_28_OFS 0x017c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_28)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_28)


/*
    ife_rth_bilat_c3_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_29_OFS 0x0180
REGDEF_BEGIN(RANGE_FILTER_REGISTER_29)
REGDEF_BIT(ife_rth_bilat_c3_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_29)


/*
    ife_rth_bilat_c3_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_30_OFS 0x0184
REGDEF_BEGIN(RANGE_FILTER_REGISTER_30)
REGDEF_BIT(ife_rth_bilat_c3_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_30)


/*
    ife_rth_bilat_c3_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_31_OFS 0x0188
REGDEF_BEGIN(RANGE_FILTER_REGISTER_31)
REGDEF_BIT(ife_rth_bilat_c3_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_31)


/*
    ife_outl_bright_ofs:    [0x0, 0xfff],           bits : 11_0
    ife_outl_dark_ofs  :    [0x0, 0xfff],           bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_0_OFS 0x018c
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_0)
REGDEF_BIT(ife_outl_bright_ofs,        12)
REGDEF_BIT(ife_outl_dark_ofs,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_0)


/*
    ife_outlth_bri0 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark0:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_1_OFS 0x0190
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_1)
REGDEF_BIT(ife_outlth_bri0,        12)
REGDEF_BIT(ife_outlth_dark0,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_1)


/*
    ife_outlth_bri1 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark1:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_2_OFS 0x0194
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_2)
REGDEF_BIT(ife_outlth_bri1,        12)
REGDEF_BIT(ife_outlth_dark1,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_2)


/*
    ife_outlth_bri2 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark2:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_3_OFS 0x0198
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_3)
REGDEF_BIT(ife_outlth_bri2,        12)
REGDEF_BIT(ife_outlth_dark2,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_3)


/*
    ife_outlth_bri3 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark3:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_4_OFS 0x019c
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_4)
REGDEF_BIT(ife_outlth_bri3,        12)
REGDEF_BIT(ife_outlth_dark3,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_4)


/*
    ife_outlth_bri4 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark4:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_5_OFS 0x01a0
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_5)
REGDEF_BIT(ife_outlth_bri4,        12)
REGDEF_BIT(ife_outlth_dark4,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_5)


/*
    ife_outl_avg_mode:    [0x0, 0x1],           bits : 0
    ife_outl_weight  :    [0x0, 0xff],          bits : 15_8
    ife_outl_cnt1    :    [0x0, 0x1f],          bits : 20_16
    ife_outl_cnt2    :    [0x0, 0x1f],          bits : 28_24
*/
#define OUTLIER_THRESHOLD_REGISTER_6_OFS 0x01a4
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_6)
REGDEF_BIT(ife_outl_avg_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(ife_outl_weight,        8)
REGDEF_BIT(ife_outl_cnt1,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_outl_cnt2,        5)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_6)


/*
    IFE_ORD_RANGE_BRI :    [0x0, 0x7],          bits : 2_0
    ife_ord_range_dark:    [0x0, 0x7],          bits : 6_4
    ife_ord_protect_th:    [0x0, 0x3ff],            bits : 17_8
    IFE_ORD_BLEND_W   :    [0x0, 0xff],         bits : 27_20
*/
#define OUTLIER_ORDER_REGISTER_0_OFS 0x01a8
REGDEF_BEGIN(OUTLIER_ORDER_REGISTER_0)
REGDEF_BIT(ife_ord_range_bright,     3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_ord_range_dark,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_ord_protect_th,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_ord_blend_weight,      8)
REGDEF_END(OUTLIER_ORDER_REGISTER_0)


/*
    ife_ord_bright_weight_lut0 (0~8):    [0x0, 0xf],            bits : 3_0
    ife_ord_bright_weight_lut1 (0~8):    [0x0, 0xf],            bits : 7_4
    ife_ord_bright_weight_lut2 (0~8):    [0x0, 0xf],            bits : 11_8
    ife_ord_bright_weight_lut3 (0~8):    [0x0, 0xf],            bits : 15_12
    ife_ord_bright_weight_lut4 (0~8):    [0x0, 0xf],            bits : 19_16
    ife_ord_bright_weight_lut5 (0~8):    [0x0, 0xf],            bits : 23_20
    ife_ord_bright_weight_lut6 (0~8):    [0x0, 0xf],            bits : 27_24
    ife_ord_bright_weight_lut7 (0~8):    [0x0, 0xf],            bits : 31_28
*/
#define OUTLIER_ORDER_REGISTER_1_OFS 0x01ac
REGDEF_BEGIN(OUTLIER_ORDER_REGISTER_1)
REGDEF_BIT(ife_ord_bright_weight_lut0,        4)
REGDEF_BIT(ife_ord_bright_weight_lut1,        4)
REGDEF_BIT(ife_ord_bright_weight_lut2,        4)
REGDEF_BIT(ife_ord_bright_weight_lut3,        4)
REGDEF_BIT(ife_ord_bright_weight_lut4,        4)
REGDEF_BIT(ife_ord_bright_weight_lut5,        4)
REGDEF_BIT(ife_ord_bright_weight_lut6,        4)
REGDEF_BIT(ife_ord_bright_weight_lut7,        4)
REGDEF_END(OUTLIER_ORDER_REGISTER_1)


/*
    ife_ord_dark_weight_lut0 (0~8):    [0x0, 0xf],          bits : 3_0
    ife_ord_dark_weight_lut1 (0~8):    [0x0, 0xf],          bits : 7_4
    ife_ord_dark_weight_lut2 (0~8):    [0x0, 0xf],          bits : 11_8
    ife_ord_dark_weight_lut3 (0~8):    [0x0, 0xf],          bits : 15_12
    ife_ord_dark_weight_lut4 (0~8):    [0x0, 0xf],          bits : 19_16
    ife_ord_dark_weight_lut5 (0~8):    [0x0, 0xf],          bits : 23_20
    ife_ord_dark_weight_lut6 (0~8):    [0x0, 0xf],          bits : 27_24
    ife_ord_dark_weight_lut7 (0~8):    [0x0, 0xf],          bits : 31_28
*/
#define OUTLIER_ORDER_REGISTER_2_OFS 0x01b0
REGDEF_BEGIN(OUTLIER_ORDER_REGISTER_2)
REGDEF_BIT(ife_ord_dark_weight_lut0,        4)
REGDEF_BIT(ife_ord_dark_weight_lut1,        4)
REGDEF_BIT(ife_ord_dark_weight_lut2,        4)
REGDEF_BIT(ife_ord_dark_weight_lut3,        4)
REGDEF_BIT(ife_ord_dark_weight_lut4,        4)
REGDEF_BIT(ife_ord_dark_weight_lut5,        4)
REGDEF_BIT(ife_ord_dark_weight_lut6,        4)
REGDEF_BIT(ife_ord_dark_weight_lut7,        4)
REGDEF_END(OUTLIER_ORDER_REGISTER_2)


/*
    ife_gbal_edge_protect_en:    [0x0, 0x1],            bits : 0
    ife_gbal_diff_thr_str   :    [0x0, 0x3ff],          bits : 17_8
    ife_gbal_diff_w_max     :    [0x0, 0xf],            bits : 23_20
*/
#define GBAL_REGISTER_0_OFS 0x01b4
REGDEF_BEGIN(GBAL_REGISTER_0)
REGDEF_BIT(ife_gbal_edge_protect_en,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(ife_gbal_diff_thr_str,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_diff_w_max,        4)
REGDEF_END(GBAL_REGISTER_0)


/*
    ife_gbal_edge_thr_1:    [0x0, 0x3ff],           bits : 9_0
    ife_gbal_edge_thr_0:    [0x0, 0x3ff],           bits : 25_16
*/
#define GBAL_REGISTER_1_OFS 0x01b8
REGDEF_BEGIN(GBAL_REGISTER_1)
REGDEF_BIT(ife_gbal_edge_thr_1,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_gbal_edge_thr_0,        10)
REGDEF_END(GBAL_REGISTER_1)


/*
    ife_gbal_edge_w_max:    [0x0, 0xff],            bits : 7_0
    ife_gbal_edge_w_min:    [0x0, 0xff],            bits : 15_8
*/
#define GBAL_REGISTER_2_OFS 0x01bc
REGDEF_BEGIN(GBAL_REGISTER_2)
REGDEF_BIT(ife_gbal_edge_w_max,        8)
REGDEF_BIT(ife_gbal_edge_w_min,        8)
REGDEF_END(GBAL_REGISTER_2)


/*
    ife_rth_nlm_c0_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_32_OFS 0x01c0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_32)
REGDEF_BIT(ife_rth_nlm_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_32)


/*
    ife_rth_nlm_c0_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_33_OFS 0x01c4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_33)
REGDEF_BIT(ife_rth_nlm_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_33)


/*
    ife_rth_nlm_c0_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_34_OFS 0x01c8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_34)
REGDEF_BIT(ife_rth_nlm_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_34)


/*
    ife_rth_nlm_c0_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_35_OFS 0x01cc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_35)
REGDEF_BIT(ife_rth_nlm_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_35)


/*
    ife_rth_nlm_c0_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_36_OFS 0x01d0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_36)
REGDEF_BIT(ife_rth_nlm_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_36)


/*
    ife_rth_nlm_c0_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_37_OFS 0x01d4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_37)
REGDEF_BIT(ife_rth_nlm_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_37)


/*
    ife_rth_nlm_c0_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_38_OFS 0x01d8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_38)
REGDEF_BIT(ife_rth_nlm_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_38)


/*
    ife_rth_nlm_c0_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_39_OFS 0x01dc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_39)
REGDEF_BIT(ife_rth_nlm_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_39)


/*
    ife_rth_nlm_c0_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_40_OFS 0x01e0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_40)
REGDEF_BIT(ife_rth_nlm_c0_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_40)


/*
    ife_rth_nlm_c1_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_41_OFS 0x01e4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_41)
REGDEF_BIT(ife_rth_nlm_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_41)


/*
    ife_rth_nlm_c1_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_42_OFS 0x01e8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_42)
REGDEF_BIT(ife_rth_nlm_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_42)


/*
    ife_rth_nlm_c1_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_43_OFS 0x01ec
REGDEF_BEGIN(RANGE_FILTER_REGISTER_43)
REGDEF_BIT(ife_rth_nlm_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_43)


/*
    ife_rth_nlm_c1_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_44_OFS 0x01f0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_44)
REGDEF_BIT(ife_rth_nlm_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_44)


/*
    ife_rth_nlm_c1_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_45_OFS 0x01f4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_45)
REGDEF_BIT(ife_rth_nlm_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_45)


/*
    ife_rth_nlm_c1_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_46_OFS 0x01f8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_46)
REGDEF_BIT(ife_rth_nlm_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_46)


/*
    ife_rth_nlm_c1_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_47_OFS 0x01fc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_47)
REGDEF_BIT(ife_rth_nlm_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_47)


/*
    ife_rth_nlm_c1_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_48_OFS 0x0200
REGDEF_BEGIN(RANGE_FILTER_REGISTER_48)
REGDEF_BIT(ife_rth_nlm_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_48)


/*
    ife_rth_nlm_c1_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_49_OFS 0x0204
REGDEF_BEGIN(RANGE_FILTER_REGISTER_49)
REGDEF_BIT(ife_rth_nlm_c1_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_49)


/*
    ife_rth_nlm_c2_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_50_OFS 0x0208
REGDEF_BEGIN(RANGE_FILTER_REGISTER_50)
REGDEF_BIT(ife_rth_nlm_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_50)


/*
    ife_rth_nlm_c2_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_51_OFS 0x020c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_51)
REGDEF_BIT(ife_rth_nlm_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_51)


/*
    ife_rth_nlm_c2_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_52_OFS 0x0210
REGDEF_BEGIN(RANGE_FILTER_REGISTER_52)
REGDEF_BIT(ife_rth_nlm_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_52)


/*
    ife_rth_nlm_c2_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_53_OFS 0x0214
REGDEF_BEGIN(RANGE_FILTER_REGISTER_53)
REGDEF_BIT(ife_rth_nlm_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_53)


/*
    ife_rth_nlm_c2_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_54_OFS 0x0218
REGDEF_BEGIN(RANGE_FILTER_REGISTER_54)
REGDEF_BIT(ife_rth_nlm_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_54)


/*
    ife_rth_nlm_c2_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_55_OFS 0x021c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_55)
REGDEF_BIT(ife_rth_nlm_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_55)


/*
    ife_rth_nlm_c2_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_56_OFS 0x0220
REGDEF_BEGIN(RANGE_FILTER_REGISTER_56)
REGDEF_BIT(ife_rth_nlm_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_56)


/*
    ife_rth_nlm_c2_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_57_OFS 0x0224
REGDEF_BEGIN(RANGE_FILTER_REGISTER_57)
REGDEF_BIT(ife_rth_nlm_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_57)


/*
    ife_rth_nlm_c2_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_58_OFS 0x0228
REGDEF_BEGIN(RANGE_FILTER_REGISTER_58)
REGDEF_BIT(ife_rth_nlm_c2_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_58)


/*
    ife_rth_nlm_c3_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_59_OFS 0x022c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_59)
REGDEF_BIT(ife_rth_nlm_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_59)


/*
    ife_rth_nlm_c3_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_60_OFS 0x0230
REGDEF_BEGIN(RANGE_FILTER_REGISTER_60)
REGDEF_BIT(ife_rth_nlm_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_60)


/*
    ife_rth_nlm_c3_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_61_OFS 0x0234
REGDEF_BEGIN(RANGE_FILTER_REGISTER_61)
REGDEF_BIT(ife_rth_nlm_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_61)


/*
    ife_rth_nlm_c3_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_62_OFS 0x0238
REGDEF_BEGIN(RANGE_FILTER_REGISTER_62)
REGDEF_BIT(ife_rth_nlm_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_62)


/*
    ife_rth_nlm_c3_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_63_OFS 0x023c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_63)
REGDEF_BIT(ife_rth_nlm_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_63)


/*
    ife_rth_nlm_c3_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_64_OFS 0x0240
REGDEF_BEGIN(RANGE_FILTER_REGISTER_64)
REGDEF_BIT(ife_rth_nlm_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_64)


/*
    ife_rth_nlm_c3_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_65_OFS 0x0244
REGDEF_BEGIN(RANGE_FILTER_REGISTER_65)
REGDEF_BIT(ife_rth_nlm_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_65)


/*
    ife_rth_nlm_c3_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_66_OFS 0x0248
REGDEF_BEGIN(RANGE_FILTER_REGISTER_66)
REGDEF_BIT(ife_rth_nlm_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_66)


/*
    ife_rth_nlm_c3_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_67_OFS 0x024c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_67)
REGDEF_BIT(ife_rth_nlm_c3_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_67)


/*
    ife_rth_bilat_c0_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_68_OFS 0x0250
REGDEF_BEGIN(RANGE_FILTER_REGISTER_68)
REGDEF_BIT(ife_rth_bilat_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_68)


/*
    ife_rth_bilat_c0_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_69_OFS 0x0254
REGDEF_BEGIN(RANGE_FILTER_REGISTER_69)
REGDEF_BIT(ife_rth_bilat_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_69)


/*
    ife_rth_bilat_c0_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_70_OFS 0x0258
REGDEF_BEGIN(RANGE_FILTER_REGISTER_70)
REGDEF_BIT(ife_rth_bilat_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_70)


/*
    ife_rth_bilat_c0_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_71_OFS 0x025c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_71)
REGDEF_BIT(ife_rth_bilat_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_71)


/*
    ife_rth_bilat_c0_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_72_OFS 0x0260
REGDEF_BEGIN(RANGE_FILTER_REGISTER_72)
REGDEF_BIT(ife_rth_bilat_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_72)


/*
    ife_rth_bilat_c0_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_73_OFS 0x0264
REGDEF_BEGIN(RANGE_FILTER_REGISTER_73)
REGDEF_BIT(ife_rth_bilat_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_73)


/*
    ife_rth_bilat_c0_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_74_OFS 0x0268
REGDEF_BEGIN(RANGE_FILTER_REGISTER_74)
REGDEF_BIT(ife_rth_bilat_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_74)


/*
    ife_rth_bilat_c0_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_75_OFS 0x026c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_75)
REGDEF_BIT(ife_rth_bilat_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_75)


/*
    ife_rth_bilat_c0_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_76_OFS 0x0270
REGDEF_BEGIN(RANGE_FILTER_REGISTER_76)
REGDEF_BIT(ife_rth_bilat_c0_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_76)


/*
    ife_rth_bilat_c1_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_77_OFS 0x0274
REGDEF_BEGIN(RANGE_FILTER_REGISTER_77)
REGDEF_BIT(ife_rth_bilat_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_77)


/*
    ife_rth_bilat_c1_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_78_OFS 0x0278
REGDEF_BEGIN(RANGE_FILTER_REGISTER_78)
REGDEF_BIT(ife_rth_bilat_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_78)


/*
    ife_rth_bilat_c1_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_79_OFS 0x027c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_79)
REGDEF_BIT(ife_rth_bilat_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_79)


/*
    ife_rth_bilat_c1_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_80_OFS 0x0280
REGDEF_BEGIN(RANGE_FILTER_REGISTER_80)
REGDEF_BIT(ife_rth_bilat_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_80)


/*
    ife_rth_bilat_c1_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_81_OFS 0x0284
REGDEF_BEGIN(RANGE_FILTER_REGISTER_81)
REGDEF_BIT(ife_rth_bilat_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_81)


/*
    ife_rth_bilat_c1_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_82_OFS 0x0288
REGDEF_BEGIN(RANGE_FILTER_REGISTER_82)
REGDEF_BIT(ife_rth_bilat_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_82)


/*
    ife_rth_bilat_c1_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_83_OFS 0x028c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_83)
REGDEF_BIT(ife_rth_bilat_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_83)


/*
    ife_rth_bilat_c1_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_84_OFS 0x0290
REGDEF_BEGIN(RANGE_FILTER_REGISTER_84)
REGDEF_BIT(ife_rth_bilat_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_84)


/*
    ife_rth_bilat_c1_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_85_OFS 0x0294
REGDEF_BEGIN(RANGE_FILTER_REGISTER_85)
REGDEF_BIT(ife_rth_bilat_c1_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_85)


/*
    ife_rth_bilat_c2_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_86_OFS 0x0298
REGDEF_BEGIN(RANGE_FILTER_REGISTER_86)
REGDEF_BIT(ife_rth_bilat_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_86)


/*
    ife_rth_bilat_c2_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_87_OFS 0x029c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_87)
REGDEF_BIT(ife_rth_bilat_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_87)


/*
    ife_rth_bilat_c2_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_88_OFS 0x02a0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_88)
REGDEF_BIT(ife_rth_bilat_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_88)


/*
    ife_rth_bilat_c2_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_89_OFS 0x02a4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_89)
REGDEF_BIT(ife_rth_bilat_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_89)


/*
    ife_rth_bilat_c2_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_90_OFS 0x02a8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_90)
REGDEF_BIT(ife_rth_bilat_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_90)


/*
    ife_rth_bilat_c2_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_91_OFS 0x02ac
REGDEF_BEGIN(RANGE_FILTER_REGISTER_91)
REGDEF_BIT(ife_rth_bilat_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_91)


/*
    ife_rth_bilat_c2_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_92_OFS 0x02b0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_92)
REGDEF_BIT(ife_rth_bilat_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_92)


/*
    ife_rth_bilat_c2_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_93_OFS 0x02b4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_93)
REGDEF_BIT(ife_rth_bilat_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_93)


/*
    ife_rth_bilat_c2_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_94_OFS 0x02b8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_94)
REGDEF_BIT(ife_rth_bilat_c2_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_94)


/*
    ife_rth_bilat_c3_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_95_OFS 0x02bc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_95)
REGDEF_BIT(ife_rth_bilat_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_95)


/*
    ife_rth_bilat_c3_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_96_OFS 0x02c0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_96)
REGDEF_BIT(ife_rth_bilat_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_96)


/*
    ife_rth_bilat_c3_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_97_OFS 0x02c4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_97)
REGDEF_BIT(ife_rth_bilat_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_97)


/*
    ife_rth_bilat_c3_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_98_OFS 0x02c8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_98)
REGDEF_BIT(ife_rth_bilat_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_98)


/*
    ife_rth_bilat_c3_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_99_OFS 0x02cc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_99)
REGDEF_BIT(ife_rth_bilat_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_99)


/*
    ife_rth_bilat_c3_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_100_OFS 0x02d0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_100)
REGDEF_BIT(ife_rth_bilat_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_100)


/*
    ife_rth_bilat_c3_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_101_OFS 0x02d4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_101)
REGDEF_BIT(ife_rth_bilat_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_101)


/*
    ife_rth_bilat_c3_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_102_OFS 0x02d8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_102)
REGDEF_BIT(ife_rth_bilat_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_102)


/*
    ife_rth_bilat_c3_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_103_OFS 0x02dc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_103)
REGDEF_BIT(ife_rth_bilat_c3_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_103)


/*
    ife_distvgtx_c0:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c0:    [0x0, 0x3fff],          bits : 29_16
    ife_distgain   :    [0x0, 0x3],         bits : 31_30
*/
#define VIGNETTE_SETTING_REGISTER_0_OFS 0x02e0
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_0)
REGDEF_BIT(ife_distvgtx_c0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c0,        14)
REGDEF_BIT(ife_distgain,        2)
REGDEF_END(VIGNETTE_SETTING_REGISTER_0)


/*
    ife_distvgtx_c1:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c1:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_1_OFS 0x02e4
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_1)
REGDEF_BIT(ife_distvgtx_c1,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c1,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_1)


/*
    ife_distvgtx_c2:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c2:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_2_OFS 0x02e8
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_2)
REGDEF_BIT(ife_distvgtx_c2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c2,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_2)


/*
    ife_distvgtx_c3:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c3:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_3_OFS 0x02ec
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_3)
REGDEF_BIT(ife_distvgtx_c3,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c3,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_3)


/*
    ife_distvgxdiv:    [0x0, 0xfff],            bits : 11_0
    ife_distvgydiv:    [0x0, 0xfff],            bits : 23_12
*/
#define VIGNETTE_SETTING_REGISTER_4_OFS 0x02f0
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_4)
REGDEF_BIT(ife_distvgxdiv,        12)
REGDEF_BIT(ife_distvgydiv,        12)
REGDEF_END(VIGNETTE_SETTING_REGISTER_4)


/*
    distdthr_en :    [0x0, 0x1],            bits : 0
    distdthr_rst:    [0x0, 0x1],            bits : 4
    distthr     :    [0x0, 0x3ff],          bits : 17_8
*/
#define VIGNETTE_SETTING_REGISTER_5_OFS 0x02f4
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_5)
REGDEF_BIT(distdthr_en,        1)
REGDEF_BIT(ife_vig_fisheye_gain_en, 1)
REGDEF_BIT(,        2)
REGDEF_BIT(distdthr_rst,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(distthr,        10)
REGDEF_BIT(ife_vig_fisheye_slope,        4)
REGDEF_BIT(ife_vig_fisheye_radius,      10)
REGDEF_END(VIGNETTE_SETTING_REGISTER_5)


/*
    ife_vig_c0_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_0_OFS 0x02f8
REGDEF_BEGIN(VIGNETTE_REGISTER_0)
REGDEF_BIT(ife_vig_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_0)


/*
    ife_vig_c0_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_1_OFS 0x02fc
REGDEF_BEGIN(VIGNETTE_REGISTER_1)
REGDEF_BIT(ife_vig_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_1)


/*
    ife_vig_c0_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_2_OFS 0x0300
REGDEF_BEGIN(VIGNETTE_REGISTER_2)
REGDEF_BIT(ife_vig_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_2)


/*
    ife_vig_c0_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_3_OFS 0x0304
REGDEF_BEGIN(VIGNETTE_REGISTER_3)
REGDEF_BIT(ife_vig_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_3)


/*
    ife_vig_c0_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_4_OFS 0x0308
REGDEF_BEGIN(VIGNETTE_REGISTER_4)
REGDEF_BIT(ife_vig_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_4)


/*
    ife_vig_c0_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_5_OFS 0x030c
REGDEF_BEGIN(VIGNETTE_REGISTER_5)
REGDEF_BIT(ife_vig_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_5)


/*
    ife_vig_c0_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_6_OFS 0x0310
REGDEF_BEGIN(VIGNETTE_REGISTER_6)
REGDEF_BIT(ife_vig_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_6)


/*
    ife_vig_c0_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_7_OFS 0x0314
REGDEF_BEGIN(VIGNETTE_REGISTER_7)
REGDEF_BIT(ife_vig_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_7)


/*
    ife_vig_c0_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_8_OFS 0x0318
REGDEF_BEGIN(VIGNETTE_REGISTER_8)
REGDEF_BIT(ife_vig_c0_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_8)


/*
    ife_vig_c1_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_9_OFS 0x031c
REGDEF_BEGIN(VIGNETTE_REGISTER_9)
REGDEF_BIT(ife_vig_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_9)


/*
    ife_vig_c1_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_10_OFS 0x0320
REGDEF_BEGIN(VIGNETTE_REGISTER_10)
REGDEF_BIT(ife_vig_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_10)


/*
    ife_vig_c1_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_11_OFS 0x0324
REGDEF_BEGIN(VIGNETTE_REGISTER_11)
REGDEF_BIT(ife_vig_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_11)


/*
    ife_vig_c1_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_12_OFS 0x0328
REGDEF_BEGIN(VIGNETTE_REGISTER_12)
REGDEF_BIT(ife_vig_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_12)


/*
    ife_vig_c1_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_13_OFS 0x032c
REGDEF_BEGIN(VIGNETTE_REGISTER_13)
REGDEF_BIT(ife_vig_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_13)


/*
    ife_vig_c1_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_14_OFS 0x0330
REGDEF_BEGIN(VIGNETTE_REGISTER_14)
REGDEF_BIT(ife_vig_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_14)


/*
    ife_vig_c1_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_15_OFS 0x0334
REGDEF_BEGIN(VIGNETTE_REGISTER_15)
REGDEF_BIT(ife_vig_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_15)


/*
    ife_vig_c1_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_16_OFS 0x0338
REGDEF_BEGIN(VIGNETTE_REGISTER_16)
REGDEF_BIT(ife_vig_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_16)


/*
    ife_vig_c1_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_17_OFS 0x033c
REGDEF_BEGIN(VIGNETTE_REGISTER_17)
REGDEF_BIT(ife_vig_c1_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_17)


/*
    ife_vig_c2_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_18_OFS 0x0340
REGDEF_BEGIN(VIGNETTE_REGISTER_18)
REGDEF_BIT(ife_vig_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_18)


/*
    ife_vig_c2_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_19_OFS 0x0344
REGDEF_BEGIN(VIGNETTE_REGISTER_19)
REGDEF_BIT(ife_vig_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_19)


/*
    ife_vig_c2_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_20_OFS 0x0348
REGDEF_BEGIN(VIGNETTE_REGISTER_20)
REGDEF_BIT(ife_vig_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_20)


/*
    ife_vig_c2_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_21_OFS 0x034c
REGDEF_BEGIN(VIGNETTE_REGISTER_21)
REGDEF_BIT(ife_vig_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_21)


/*
    ife_vig_c2_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_22_OFS 0x0350
REGDEF_BEGIN(VIGNETTE_REGISTER_22)
REGDEF_BIT(ife_vig_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_22)


/*
    ife_vig_c2_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_23_OFS 0x0354
REGDEF_BEGIN(VIGNETTE_REGISTER_23)
REGDEF_BIT(ife_vig_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_23)


/*
    ife_vig_c2_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_24_OFS 0x0358
REGDEF_BEGIN(VIGNETTE_REGISTER_24)
REGDEF_BIT(ife_vig_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_24)


/*
    ife_vig_c2_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_25_OFS 0x035c
REGDEF_BEGIN(VIGNETTE_REGISTER_25)
REGDEF_BIT(ife_vig_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_25)


/*
    ife_vig_c2_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_26_OFS 0x0360
REGDEF_BEGIN(VIGNETTE_REGISTER_26)
REGDEF_BIT(ife_vig_c2_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_26)


/*
    ife_vig_c3_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_27_OFS 0x0364
REGDEF_BEGIN(VIGNETTE_REGISTER_27)
REGDEF_BIT(ife_vig_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_27)


/*
    ife_vig_c3_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_28_OFS 0x0368
REGDEF_BEGIN(VIGNETTE_REGISTER_28)
REGDEF_BIT(ife_vig_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_28)


/*
    ife_vig_c3_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_29_OFS 0x036c
REGDEF_BEGIN(VIGNETTE_REGISTER_29)
REGDEF_BIT(ife_vig_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_29)


/*
    ife_vig_c3_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_30_OFS 0x0370
REGDEF_BEGIN(VIGNETTE_REGISTER_30)
REGDEF_BIT(ife_vig_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_30)


/*
    ife_vig_c3_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_31_OFS 0x0374
REGDEF_BEGIN(VIGNETTE_REGISTER_31)
REGDEF_BIT(ife_vig_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_31)


/*
    ife_vig_c3_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_32_OFS 0x0378
REGDEF_BEGIN(VIGNETTE_REGISTER_32)
REGDEF_BIT(ife_vig_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_32)


/*
    ife_vig_c3_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_33_OFS 0x037c
REGDEF_BEGIN(VIGNETTE_REGISTER_33)
REGDEF_BIT(ife_vig_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_33)


/*
    ife_vig_c3_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_34_OFS 0x0380
REGDEF_BEGIN(VIGNETTE_REGISTER_34)
REGDEF_BIT(ife_vig_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_34)


/*
    ife_vig_c3_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_35_OFS 0x0384
REGDEF_BEGIN(VIGNETTE_REGISTER_35)
REGDEF_BIT(ife_vig_c3_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_35)


/*
    ife_gbal_ofs_lut_00:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_01:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_02:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_03:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_3_OFS 0x0388
REGDEF_BEGIN(GBAL_REGISTER_3)
REGDEF_BIT(ife_gbal_ofs_lut_00,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_01,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_02,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_03,        6)
REGDEF_END(GBAL_REGISTER_3)


/*
    ife_gbal_ofs_lut_04:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_05:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_06:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_07:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_4_OFS 0x038c
REGDEF_BEGIN(GBAL_REGISTER_4)
REGDEF_BIT(ife_gbal_ofs_lut_04,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_05,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_06,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_07,        6)
REGDEF_END(GBAL_REGISTER_4)


/*
    ife_gbal_ofs_lut_08:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_09:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_10:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_11:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_5_OFS 0x0390
REGDEF_BEGIN(GBAL_REGISTER_5)
REGDEF_BIT(ife_gbal_ofs_lut_08,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_09,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_11,        6)
REGDEF_END(GBAL_REGISTER_5)


/*
    ife_gbal_ofs_lut_12:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_13:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_14:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_15:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_6_OFS 0x0394
REGDEF_BEGIN(GBAL_REGISTER_6)
REGDEF_BIT(ife_gbal_ofs_lut_12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_15,        6)
REGDEF_END(GBAL_REGISTER_6)


/*
    ife_gbal_ofs_lut_16:    [0x0, 0x3f],            bits : 5_0
*/
#define GBAL_REGISTER_7_OFS 0x0398
REGDEF_BEGIN(GBAL_REGISTER_7)
REGDEF_BIT(ife_gbal_ofs_lut_16,        6)
REGDEF_END(GBAL_REGISTER_7)


/*
    ife_rbluma00:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma01:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma02:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma03:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_0_OFS 0x039c
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_0)
REGDEF_BIT(ife_rbluma00,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma01,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma02,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma03,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_0)


/*
    ife_rbluma04:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma05:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma06:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma07:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_1_OFS 0x03a0
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_1)
REGDEF_BIT(ife_rbluma04,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma05,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma06,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma07,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_1)


/*
    ife_rbluma08:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma09:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma10:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma11:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_2_OFS 0x03a4
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_2)
REGDEF_BIT(ife_rbluma08,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma09,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma10,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma11,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_2)


/*
    ife_rbluma12:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma13:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma14:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma15:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_3_OFS 0x03a8
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_3)
REGDEF_BIT(ife_rbluma12,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma13,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma14,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma15,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_3)


/*
    ife_rbluma16:    [0x0, 0x1f],           bits : 4_0
*/
#define RB_LUMINANCE_REGISTER_4_OFS 0x03ac
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_4)
REGDEF_BIT(ife_rbluma16,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_4)

/*
    ife_rbratio00:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio01:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio02:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio03:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_0_OFS 0x03c0
REGDEF_BEGIN(RB_RATIO_REGISTER_0)
REGDEF_BIT(ife_rbratio00,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio01,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio02,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio03,        5)
REGDEF_END(RB_RATIO_REGISTER_0)


/*
    ife_rbratio04:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio05:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio06:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio07:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_1_OFS 0x03c4
REGDEF_BEGIN(RB_RATIO_REGISTER_1)
REGDEF_BIT(ife_rbratio04,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio05,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio06,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio07,        5)
REGDEF_END(RB_RATIO_REGISTER_1)


/*
    ife_rbratio08:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio09:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio10:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio11:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_2_OFS 0x03c8
REGDEF_BEGIN(RB_RATIO_REGISTER_2)
REGDEF_BIT(ife_rbratio08,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio09,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio10,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio11,        5)
REGDEF_END(RB_RATIO_REGISTER_2)


/*
    ife_rbratio12:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio13:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio14:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio15:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_3_OFS 0x03cc
REGDEF_BEGIN(RB_RATIO_REGISTER_3)
REGDEF_BIT(ife_rbratio12,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio13,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio14,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio15,        5)
REGDEF_END(RB_RATIO_REGISTER_3)


/*
    ife_rbratio16:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio17:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio18:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio19:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_4_OFS 0x03d0
REGDEF_BEGIN(RB_RATIO_REGISTER_4)
REGDEF_BIT(ife_rbratio16,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio17,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio18,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio19,        5)
REGDEF_END(RB_RATIO_REGISTER_4)


/*
    ife_rbratio20:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio21:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio22:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio23:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_5_OFS 0x03d4
REGDEF_BEGIN(RB_RATIO_REGISTER_5)
REGDEF_BIT(ife_rbratio20,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio21,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio22,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio23,        5)
REGDEF_END(RB_RATIO_REGISTER_5)


/*
    ife_rbratio24:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio25:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio26:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio27:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_6_OFS 0x03d8
REGDEF_BEGIN(RB_RATIO_REGISTER_6)
REGDEF_BIT(ife_rbratio24,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio25,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio26,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio27,        5)
REGDEF_END(RB_RATIO_REGISTER_6)


/*
    ife_rbratio28     :    [0x0, 0x1f],         bits : 4_0
    ife_rbratio29     :    [0x0, 0x1f],         bits : 12_8
    ife_rbratio30     :    [0x0, 0x1f],         bits : 20_16
    ife_rbratio31     :    [0x0, 0x1f],         bits : 28_24
    rbratio_mode (0~2):    [0x0, 0x3],          bits : 30_29
*/
#define RB_RATIO_REGISTER_7_OFS 0x03dc
REGDEF_BEGIN(RB_RATIO_REGISTER_7)
REGDEF_BIT(ife_rbratio28,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio29,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio30,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio31,        5)
REGDEF_BIT(rbratio_mode,        2)
REGDEF_END(RB_RATIO_REGISTER_7)

/*
    IFE_F_NRS_O_EN            :    [0x0, 0x1],          bits : 0
    IFE_F_NRS_B_EN            :    [0x0, 0x1],          bits : 1
    IFE_F_NRS_G_EN            :    [0x0, 0x1],          bits : 2
    IFE_F_NRS_B_STRENGTH (0~8):    [0x0, 0xf],          bits : 19_16
    IFE_F_NRS_G_STRENGTH (0~8):    [0x0, 0xf],          bits : 23_20
    IFE_F_NRS_G_WEIGHT        :    [0x0, 0xf],          bits : 27_24
*/
#define NRS_REGISTER_OFS 0x0500
REGDEF_BEGIN(NRS_REGISTER)
REGDEF_BIT(ife_f_nrs_ord_en,        1)
REGDEF_BIT(ife_f_nrs_bilat_en,        1)
REGDEF_BIT(ife_f_nrs_gbilat_en,        1)
REGDEF_BIT(,       13)
REGDEF_BIT(ife_f_nrs_bilat_strength,        4)
REGDEF_BIT(ife_f_nrs_gbilat_strength,        4)
REGDEF_BIT(ife_f_nrs_gbilat_weight,        4)
REGDEF_END(NRS_REGISTER)


/*
    IFE_F_NRS_ORD_RANGE_BRI :    [0x0, 0x7],            bits : 2_0
    ife_f_nrs_ord_range_dark:    [0x0, 0x7],            bits : 6_4
    ife_f_nrs_ord_diff_thr  :    [0x0, 0x3ff],          bits : 17_8
*/
#define RHE_NRS_S_REGISTER_OFS 0x0504
REGDEF_BEGIN(RHE_NRS_S_REGISTER)
REGDEF_BIT(ife_f_nrs_ord_range_bright,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_nrs_ord_range_dark,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_nrs_ord_diff_thr,        10)
REGDEF_END(RHE_NRS_S_REGISTER)


/*
    ife_f_nrs_ord_dark_weight_lut0 (0~8):    [0x0, 0xf],            bits : 3_0
    ife_f_nrs_ord_dark_weight_lut1 (0~8):    [0x0, 0xf],            bits : 7_4
    ife_f_nrs_ord_dark_weight_lut2 (0~8):    [0x0, 0xf],            bits : 11_8
    ife_f_nrs_ord_dark_weight_lut3 (0~8):    [0x0, 0xf],            bits : 15_12
    ife_f_nrs_ord_dark_weight_lut4 (0~8):    [0x0, 0xf],            bits : 19_16
    ife_f_nrs_ord_dark_weight_lut5 (0~8):    [0x0, 0xf],            bits : 23_20
    ife_f_nrs_ord_dark_weight_lut6 (0~8):    [0x0, 0xf],            bits : 27_24
    ife_f_nrs_ord_dark_weight_lut7 (0~8):    [0x0, 0xf],            bits : 31_28
*/
#define RHE_NRS_ORDER_LUT_DARK_WEIGHT_OFS 0x0508
REGDEF_BEGIN(RHE_NRS_ORDER_LUT_DARK)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut0,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut1,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut2,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut3,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut4,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut5,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut6,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut7,        4)
REGDEF_END(RHE_NRS_ORDER_LUT_DARK_WEIGHT)


/*
    ife_f_nrs_ord_bright_weight_lut0 (0~8):    [0x0, 0xf],          bits : 3_0
    ife_f_nrs_ord_bright_weight_lut1 (0~8):    [0x0, 0xf],          bits : 7_4
    ife_f_nrs_ord_bright_weight_lut2 (0~8):    [0x0, 0xf],          bits : 11_8
    ife_f_nrs_ord_bright_weight_lut3 (0~8):    [0x0, 0xf],          bits : 15_12
    ife_f_nrs_ord_bright_weight_lut4 (0~8):    [0x0, 0xf],          bits : 19_16
    ife_f_nrs_ord_bright_weight_lut5 (0~8):    [0x0, 0xf],          bits : 23_20
    ife_f_nrs_ord_bright_weight_lut6 (0~8):    [0x0, 0xf],          bits : 27_24
    ife_f_nrs_ord_bright_weight_lut7 (0~8):    [0x0, 0xf],          bits : 31_28
*/
#define RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT_OFS 0x050c
REGDEF_BEGIN(RHE_NRS_ORDER_LUT_BRI)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut0,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut1,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut2,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut3,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut4,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut5,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut6,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut7,        4)
REGDEF_END(RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT)


/*
    IFE_F_NRS_B_LUT_OFS0:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_B_LUT_OFS1:    [0x0, 0xff],           bits : 15_8
    IFE_F_NRS_B_LUT_OFS2:    [0x0, 0xff],           bits : 23_16
    IFE_F_NRS_B_LUT_OFS3:    [0x0, 0xff],           bits : 31_24
*/
#define RHE_NRS_BILATERAL_LUT_OFFSET0_OFS 0x0510
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_OFFSET0)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset0,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset1,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset2,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset3,        8)
REGDEF_END(RHE_NRS_BILATERAL_LUT_OFFSET0)


/*
    IFE_F_NRS_B_LUT_OFS4:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_B_LUT_OFS5:    [0x0, 0xff],           bits : 15_8
*/
#define RHE_NRS_BILATERAL_LUT_OFFSET1_OFS 0x0514
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_OFFSET1)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset4,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset5,        8)
REGDEF_END(RHE_NRS_BILATERAL_LUT_OFFSET1)


/*
    IFE_F_NRS_B_LUT_W0:    [0x0, 0x1f],         bits : 4_0
    IFE_F_NRS_B_LUT_W1:    [0x0, 0x1f],         bits : 9_5
    IFE_F_NRS_B_LUT_W2:    [0x0, 0x1f],         bits : 14_10
    IFE_F_NRS_B_LUT_W3:    [0x0, 0x1f],         bits : 19_15
    IFE_F_NRS_B_LUT_W4:    [0x0, 0x1f],         bits : 24_20
    IFE_F_NRS_B_LUT_W5:    [0x0, 0x1f],         bits : 29_25
*/
#define RHE_NRS_BILATERAL_LUT_WEIGHT_OFS 0x0518
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_WEIGHT)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight0,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight1,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight2,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight3,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight4,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight5,        5)
REGDEF_END(RHE_NRS_BILATERAL_LUT_WEIGHT)


/*
    IFE_F_NRS_B_LUT_RANGE1:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_B_LUT_RANGE2:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_BILATERAL_LUT_RANGE0_OFS 0x051c
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE0)
REGDEF_BIT(ife_f_nrs_bilat_lut_range1,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_range2,        12)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE0)


/*
    IFE_F_NRS_B_LUT_RANGE3:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_B_LUT_RANGE4:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_BILATERAL_LUT_RANGE1_OFS 0x0520
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE1)
REGDEF_BIT(ife_f_nrs_bilat_lut_range3,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_range4,        12)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE1)


/*
    IFE_F_NRS_B_LUT_RANGE5 :    [0x0, 0xfff],           bits : 11_0
    IFE_F_NRS_B_LUT_TH1:    [0x0, 0x3],         bits : 17_16
    IFE_F_NRS_B_LUT_TH2:    [0x0, 0x3],         bits : 19_18
    IFE_F_NRS_B_LUT_TH3:    [0x0, 0x3],         bits : 21_20
    IFE_F_NRS_B_LUT_TH4:    [0x0, 0x3],         bits : 23_22
    IFE_F_NRS_B_LUT_TH5:    [0x0, 0x3],         bits : 25_24
*/
#define RHE_NRS_BILATERAL_LUT_RANGE2_OFS 0x0524
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE2)
REGDEF_BIT(ife_f_nrs_bilat_lut_range5,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_th1,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th2,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th3,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th4,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th5,        2)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE2)


/*
    IFE_F_NRS_G_LUT_OFS0:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_G_LUT_OFS1:    [0x0, 0xff],           bits : 15_8
    IFE_F_NRS_G_LUT_OFS2:    [0x0, 0xff],           bits : 23_16
    IFE_F_NRS_G_LUT_OFS3:    [0x0, 0xff],           bits : 31_24
*/
#define RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS 0x0528
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_OFFSET0)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset0,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset1,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset2,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset3,        8)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_OFFSET0)


/*
    IFE_F_NRS_G_LUT_OFS4:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_G_LUT_OFS5:    [0x0, 0xff],           bits : 15_8
*/
#define RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS 0x052c
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_OFFSET1)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset4,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset5,        8)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_OFFSET1)


/*
    IFE_F_NRS_G_LUT_W0:    [0x0, 0x1f],         bits : 4_0
    IFE_F_NRS_G_LUT_W1:    [0x0, 0x1f],         bits : 9_5
    IFE_F_NRS_G_LUT_W2:    [0x0, 0x1f],         bits : 14_10
    IFE_F_NRS_G_LUT_W3:    [0x0, 0x1f],         bits : 19_15
    IFE_F_NRS_G_LUT_W4:    [0x0, 0x1f],         bits : 24_20
    IFE_F_NRS_G_LUT_W5:    [0x0, 0x1f],         bits : 29_25
*/
#define RHE_NRS_GBILATERAL_LUT_WEIGHT_OFS 0x0530
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_WEIGHT)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight0,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight1,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight2,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight3,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight4,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight5,        5)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_WEIGHT)


/*
    IFE_F_NRS_G_LUT_RANGE1:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_G_LUT_RANGE2:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE0_OFS 0x0534
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE0)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range1,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range2,        12)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE0)


/*
    IFE_F_NRS_G_LUT_RANGE3:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_G_LUT_RANGE4:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE1_OFS 0x0538
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE1)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range3,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range4,        12)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE1)


/*
    IFE_F_NRS_G_LUT_RANGE5 :    [0x0, 0xfff],           bits : 11_0
    IFE_F_NRS_G_LUT_TH1:    [0x0, 0x3],         bits : 17_16
    IFE_F_NRS_G_LUT_TH2:    [0x0, 0x3],         bits : 19_18
    IFE_F_NRS_G_LUT_TH3:    [0x0, 0x3],         bits : 21_20
    IFE_F_NRS_G_LUT_TH4:    [0x0, 0x3],         bits : 23_22
    IFE_F_NRS_G_LUT_TH5:    [0x0, 0x3],         bits : 25_24
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE2_OFS 0x053c
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range5,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th1,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th2,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th3,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th4,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th5,        2)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE2)

/*
    IFE_F_FUSION_YM_SEL       :    [0x0, 0x3],          bits : 1_0
    IFE_F_FUSION_BCN_SEL (0~2):    [0x0, 0x3],          bits : 5_4
    IFE_F_FUSION_BCD_SEL (0~2):    [0x0, 0x3],          bits : 9_8
    ife_f_fusion_mode         :    [0x0, 0x3],          bits : 13_12
    ife_f_fusion_evratio      :    [0x0, 0xff],         bits : 23_16
*/
#define RHE_FUSION_REGISTER_OFS 0x0550
REGDEF_BEGIN(RHE_FUSION_REGISTER)
REGDEF_BIT(ife_f_fusion_ymean_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_normal_blend_curve_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_diff_blend_curve_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_mode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_evratio,        8)
REGDEF_END(RHE_FUSION_REGISTER)


/*
    IFE_F_FUSION_BCNL_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCNL_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCNL_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCNL_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS 0x0554
REGDEF_BEGIN(RHE_FUSION_LONG_EXPOSURE_NORMAL_BLENDING_CURVE)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE)


/*
    IFE_F_FUSION_BCNS_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCNS_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCNS_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCNS_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS 0x0558
REGDEF_BEGIN(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE)


/*
    IFE_F_FUSION_BCDL_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCDL_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCDL_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCDL_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS 0x055c
REGDEF_BEGIN(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE)


/*
    IFE_F_FUSION_BCDS_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCDS_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCDS_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCDS_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS 0x0560
REGDEF_BEGIN(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE)

/*
    ife_f_fusion_mc_lumthr    :    [0x0, 0xfff],            bits : 11_0
    ife_f_fusion_mc_diff_ratio:    [0x0, 0x3],          bits : 13_12
*/
#define RHE_FUSION_MOTION_COMPENSATION_OFS 0x0570
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION)
REGDEF_BIT(ife_f_fusion_mc_lumthr,        12)
REGDEF_BIT(ife_f_fusion_mc_diff_ratio,        2)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION)


/*
    ife_f_fusion_mc_lut_positive_diff_weight0 (0~16):    [0x0, 0x1f],           bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight1 (0~16):    [0x0, 0x1f],           bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight2 (0~16):    [0x0, 0x1f],           bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight3 (0~16):    [0x0, 0x1f],           bits : 19_15
    ife_f_fusion_mc_lut_positive_diff_weight4 (0~16):    [0x0, 0x1f],           bits : 24_20
    ife_f_fusion_mc_lut_positive_diff_weight5 (0~16):    [0x0, 0x1f],           bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_0_OFS 0x0574
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_0)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight0,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight1,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight2,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight3,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight4,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight5,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_0)


/*
    ife_f_fusion_mc_lut_positive_diff_weight6 (0~16) :    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight7 (0~16) :    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight8 (0~16) :    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight9 (0~16) :    [0x0, 0x1f],          bits : 19_15
    ife_f_fusion_mc_lut_positive_diff_weight10 (0~16):    [0x0, 0x1f],          bits : 24_20
    ife_f_fusion_mc_lut_positive_diff_weight11 (0~16):    [0x0, 0x1f],          bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_1_OFS 0x0578
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_1)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight6,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight7,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight8,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight9,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight10,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight11,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_1)


/*
    ife_f_fusion_mc_lut_positive_diff_weight12 (0~16):    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight13 (0~16):    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight14 (0~16):    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight15 (0~16):    [0x0, 0x1f],          bits : 19_15
    IFE_F_FUSION_MC_LUT_DWD (0~16)  :    [0x0, 0x1f],           bits : 24_20
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS 0x057c
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_2)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight12,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight13,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight14,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight15,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_difflumth_diff_weight,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_2)


/*
    ife_f_fusion_mc_lut_negative_diff_weight0 (0~16):    [0x0, 0x1f],           bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight1 (0~16):    [0x0, 0x1f],           bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight2 (0~16):    [0x0, 0x1f],           bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight3 (0~16):    [0x0, 0x1f],           bits : 19_15
    ife_f_fusion_mc_lut_negative_diff_weight4 (0~16):    [0x0, 0x1f],           bits : 24_20
    ife_f_fusion_mc_lut_negative_diff_weight5 (0~16):    [0x0, 0x1f],           bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_3_OFS 0x0580
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_3)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight0,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight1,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight2,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight3,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight4,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight5,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_3)


/*
    ife_f_fusion_mc_lut_negative_diff_weight6 (0~16) :    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight7 (0~16) :    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight8 (0~16) :    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight9 (0~16) :    [0x0, 0x1f],          bits : 19_15
    ife_f_fusion_mc_lut_negative_diff_weight10 (0~16):    [0x0, 0x1f],          bits : 24_20
    ife_f_fusion_mc_lut_negative_diff_weight11 (0~16):    [0x0, 0x1f],          bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_4_OFS 0x0584
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_4)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight6,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight7,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight8,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight9,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight10,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight11,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_4)


/*
    ife_f_fusion_mc_lut_negative_diff_weight12 (0~16):    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight13 (0~16):    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight14 (0~16):    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight15 (0~16):    [0x0, 0x1f],          bits : 19_15
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS 0x0588
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight12,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight13,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight14,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight15,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_5)

/*
    IFE_F_DS0_TH  :    [0x0, 0xfff],            bits : 11_0
    IFE_F_DS0_STEP:    [0x0, 0xff],         bits : 23_16
    IFE_F_DS0_LB  :    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION_OFS 0x0590
REGDEF_BEGIN(RHE_FUSION_PATH0_DSR)
REGDEF_BIT(ife_f_dark_sat_reduction0_th,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_dark_sat_reduction0_step,        8)
REGDEF_BIT(ife_f_dark_sat_reduction0_low_bound,        8)
REGDEF_END(RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION)


/*
    IFE_F_DS1_TH  :    [0x0, 0xfff],            bits : 11_0
    IFE_F_DS1_STEP:    [0x0, 0xff],         bits : 23_16
    IFE_F_DS1_LB  :    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION_OFS 0x0594
REGDEF_BEGIN(RHE_FUSION_PATH1_DSR)
REGDEF_BIT(ife_f_dark_sat_reduction1_th,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_dark_sat_reduction1_step,        8)
REGDEF_BIT(ife_f_dark_sat_reduction1_low_bound,        8)
REGDEF_END(RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION)

/*
    ife_f_fusion_short_exposure_compress_knee_point0:    [0x0, 0xfff],          bits : 11_0
    ife_f_fusion_short_exposure_compress_knee_point1:    [0x0, 0xfff],          bits : 27_16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0_OFS 0x05a0
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_knee_point0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_knee_point1,        12)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0)


/*
    ife_f_fusion_short_exposure_compress_knee_point2:    [0x0, 0xfff],          bits : 11_0
    IFE_f_fusion_short_exposure_compress_en :    [0x0, 0x1],            bits : 16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1_OFS 0x05a4
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_knee_point2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_enable,        1)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1)


/*
    ife_f_fusion_short_exposure_compress_subtract_point0:    [0x0, 0xfff],          bits : 11_0
    ife_f_fusion_short_exposure_compress_subtract_point1:    [0x0, 0xfff],          bits : 27_16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2_OFS 0x05a8
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point1,        12)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2)


/*
    ife_f_fusion_short_exposure_compress_subtract_point2:    [0x0, 0xfff],          bits : 11_0
    ife_f_fusion_short_exposure_compress_subtract_point3:    [0x0, 0xfff],          bits : 27_16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3_OFS 0x05ac
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point3,        12)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3)


/*
    ife_f_fusion_short_exposure_compress_shift_bit0:    [0x0, 0x7],         bits : 2_0
    ife_f_fusion_short_exposure_compress_shift_bit1:    [0x0, 0x7],         bits : 6_4
    ife_f_fusion_short_exposure_compress_shift_bit2:    [0x0, 0x7],         bits : 10_8
    ife_f_fusion_short_exposure_compress_shift_bit3:    [0x0, 0x7],         bits : 14_12
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4_OFS 0x05b0
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit0,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit1,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit2,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit3,        3)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4)

/*
    IFE_F_FCURVE_YM_SEL (0~2)  :    [0x0, 0x3],         bits : 1_0
    ife_f_fcurve_yvweight (0~8):    [0x0, 0xf],         bits : 5_2
*/
#define RHE_FCURVE_CTRL_OFS 0x0600
REGDEF_BEGIN(RHE_FCURVE_CTRL)
REGDEF_BIT(ife_f_fcurve_ymean_select,        2)
REGDEF_BIT(ife_f_fcurve_yvweight,        4)
REGDEF_END(RHE_FCURVE_CTRL)


/*
    ife_f_fcurve_yweight_lut0:    [0x0, 0xff],          bits : 7_0
    ife_f_fcurve_yweight_lut1:    [0x0, 0xff],          bits : 15_8
    ife_f_fcurve_yweight_lut2:    [0x0, 0xff],          bits : 23_16
    ife_f_fcurve_yweight_lut3:    [0x0, 0xff],          bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER0_OFS 0x0604
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER0)
REGDEF_BIT(ife_f_fcurve_yweight_lut0,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut1,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut2,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut3,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER0)


/*
    ife_f_fcurve_yweight_lut4:    [0x0, 0xff],          bits : 7_0
    ife_f_fcurve_yweight_lut5:    [0x0, 0xff],          bits : 15_8
    ife_f_fcurve_yweight_lut6:    [0x0, 0xff],          bits : 23_16
    ife_f_fcurve_yweight_lut7:    [0x0, 0xff],          bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER1_OFS 0x0608
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER1)
REGDEF_BIT(ife_f_fcurve_yweight_lut4,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut5,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut6,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut7,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER1)


/*
    ife_f_fcurve_yweight_lut8 :    [0x0, 0xff],         bits : 7_0
    ife_f_fcurve_yweight_lut9 :    [0x0, 0xff],         bits : 15_8
    ife_f_fcurve_yweight_lut10:    [0x0, 0xff],         bits : 23_16
    ife_f_fcurve_yweight_lut11:    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER2_OFS 0x060c
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER2)
REGDEF_BIT(ife_f_fcurve_yweight_lut8,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut9,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut10,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut11,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER2)


/*
    ife_f_fcurve_yweight_lut12:    [0x0, 0xff],         bits : 7_0
    ife_f_fcurve_yweight_lut13:    [0x0, 0xff],         bits : 15_8
    ife_f_fcurve_yweight_lut14:    [0x0, 0xff],         bits : 23_16
    ife_f_fcurve_yweight_lut15:    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER3_OFS 0x0610
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER3)
REGDEF_BIT(ife_f_fcurve_yweight_lut12,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut13,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut14,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut15,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER3)


/*
    ife_f_fcurve_yweight_lut16:    [0x0, 0xff],         bits : 7_0
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER4_OFS 0x0614
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER4)
REGDEF_BIT(ife_f_fcurve_yweight_lut16,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER4)


/*
    ife_f_fcurve_index_lut0:    [0x0, 0x3f],            bits : 5_0
    ife_f_fcurve_index_lut1:    [0x0, 0x3f],            bits : 13_8
    ife_f_fcurve_index_lut2:    [0x0, 0x3f],            bits : 21_16
    ife_f_fcurve_index_lut3:    [0x0, 0x3f],            bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER0_OFS 0x0618
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER0)
REGDEF_BIT(ife_f_fcurve_index_lut0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut3,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER0)


/*
    ife_f_fcurve_index_lut4:    [0x0, 0x3f],            bits : 5_0
    ife_f_fcurve_index_lut5:    [0x0, 0x3f],            bits : 13_8
    ife_f_fcurve_index_lut6:    [0x0, 0x3f],            bits : 21_16
    ife_f_fcurve_index_lut7:    [0x0, 0x3f],            bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER1_OFS 0x061c
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER1)
REGDEF_BIT(ife_f_fcurve_index_lut4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut7,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER1)


/*
    ife_f_fcurve_index_lut8 :    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut9 :    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut10:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut11:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER2_OFS 0x0620
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER2)
REGDEF_BIT(ife_f_fcurve_index_lut8,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut9,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut11,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER2)


/*
    ife_f_fcurve_index_lut12:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut13:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut14:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut15:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER3_OFS 0x0624
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER3)
REGDEF_BIT(ife_f_fcurve_index_lut12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut15,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER3)


/*
    ife_f_fcurve_index_lut16:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut17:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut18:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut19:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER4_OFS 0x0628
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER4)
REGDEF_BIT(ife_f_fcurve_index_lut16,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut17,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut18,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut19,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER4)


/*
    ife_f_fcurve_index_lut20:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut21:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut22:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut23:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER5_OFS 0x062c
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER5)
REGDEF_BIT(ife_f_fcurve_index_lut20,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut21,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut22,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut23,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER5)


/*
    ife_f_fcurve_index_lut24:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut25:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut26:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut27:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER6_OFS 0x0630
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER6)
REGDEF_BIT(ife_f_fcurve_index_lut24,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut25,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut26,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut27,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER6)


/*
    ife_f_fcurve_index_lut28:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut29:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut30:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut31:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER7_OFS 0x0634
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER7)
REGDEF_BIT(ife_f_fcurve_index_lut28,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut29,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut30,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut31,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER7)


/*
    ife_f_fcurve_split_lut0 :    [0x0, 0x3],            bits : 1_0
    ife_f_fcurve_split_lut1 :    [0x0, 0x3],            bits : 3_2
    ife_f_fcurve_split_lut2 :    [0x0, 0x3],            bits : 5_4
    ife_f_fcurve_split_lut3 :    [0x0, 0x3],            bits : 7_6
    ife_f_fcurve_split_lut4 :    [0x0, 0x3],            bits : 9_8
    ife_f_fcurve_split_lut5 :    [0x0, 0x3],            bits : 11_10
    ife_f_fcurve_split_lut6 :    [0x0, 0x3],            bits : 13_12
    ife_f_fcurve_split_lut7 :    [0x0, 0x3],            bits : 15_14
    ife_f_fcurve_split_lut8 :    [0x0, 0x3],            bits : 17_16
    ife_f_fcurve_split_lut9 :    [0x0, 0x3],            bits : 19_18
    ife_f_fcurve_split_lut10:    [0x0, 0x3],            bits : 21_20
    ife_f_fcurve_split_lut11:    [0x0, 0x3],            bits : 23_22
    ife_f_fcurve_split_lut12:    [0x0, 0x3],            bits : 25_24
    ife_f_fcurve_split_lut13:    [0x0, 0x3],            bits : 27_26
    ife_f_fcurve_split_lut14:    [0x0, 0x3],            bits : 29_28
    ife_f_fcurve_split_lut15:    [0x0, 0x3],            bits : 31_30
*/
#define RHE_FCURVE_SPLIT_REGISTER0_OFS 0x0638
REGDEF_BEGIN(RHE_FCURVE_SPLIT_REGISTER0)
REGDEF_BIT(ife_f_fcurve_split_lut0,        2)
REGDEF_BIT(ife_f_fcurve_split_lut1,        2)
REGDEF_BIT(ife_f_fcurve_split_lut2,        2)
REGDEF_BIT(ife_f_fcurve_split_lut3,        2)
REGDEF_BIT(ife_f_fcurve_split_lut4,        2)
REGDEF_BIT(ife_f_fcurve_split_lut5,        2)
REGDEF_BIT(ife_f_fcurve_split_lut6,        2)
REGDEF_BIT(ife_f_fcurve_split_lut7,        2)
REGDEF_BIT(ife_f_fcurve_split_lut8,        2)
REGDEF_BIT(ife_f_fcurve_split_lut9,        2)
REGDEF_BIT(ife_f_fcurve_split_lut10,        2)
REGDEF_BIT(ife_f_fcurve_split_lut11,        2)
REGDEF_BIT(ife_f_fcurve_split_lut12,        2)
REGDEF_BIT(ife_f_fcurve_split_lut13,        2)
REGDEF_BIT(ife_f_fcurve_split_lut14,        2)
REGDEF_BIT(ife_f_fcurve_split_lut15,        2)
REGDEF_END(RHE_FCURVE_SPLIT_REGISTER0)


/*
    ife_f_fcurve_split_lut16:    [0x0, 0x3],            bits : 1_0
    ife_f_fcurve_split_lut17:    [0x0, 0x3],            bits : 3_2
    ife_f_fcurve_split_lut18:    [0x0, 0x3],            bits : 5_4
    ife_f_fcurve_split_lut19:    [0x0, 0x3],            bits : 7_6
    ife_f_fcurve_split_lut20:    [0x0, 0x3],            bits : 9_8
    ife_f_fcurve_split_lut21:    [0x0, 0x3],            bits : 11_10
    ife_f_fcurve_split_lut22:    [0x0, 0x3],            bits : 13_12
    ife_f_fcurve_split_lut23:    [0x0, 0x3],            bits : 15_14
    ife_f_fcurve_split_lut24:    [0x0, 0x3],            bits : 17_16
    ife_f_fcurve_split_lut25:    [0x0, 0x3],            bits : 19_18
    ife_f_fcurve_split_lut26:    [0x0, 0x3],            bits : 21_20
    ife_f_fcurve_split_lut27:    [0x0, 0x3],            bits : 23_22
    ife_f_fcurve_split_lut28:    [0x0, 0x3],            bits : 25_24
    ife_f_fcurve_split_lut29:    [0x0, 0x3],            bits : 27_26
    ife_f_fcurve_split_lut30:    [0x0, 0x3],            bits : 29_28
    ife_f_fcurve_split_lut31:    [0x0, 0x3],            bits : 31_30
*/
#define RHE_FCURVE_SPLIT_REGISTER1_OFS 0x063c
REGDEF_BEGIN(RHE_FCURVE_SPLIT_REGISTER1)
REGDEF_BIT(ife_f_fcurve_split_lut16,        2)
REGDEF_BIT(ife_f_fcurve_split_lut17,        2)
REGDEF_BIT(ife_f_fcurve_split_lut18,        2)
REGDEF_BIT(ife_f_fcurve_split_lut19,        2)
REGDEF_BIT(ife_f_fcurve_split_lut20,        2)
REGDEF_BIT(ife_f_fcurve_split_lut21,        2)
REGDEF_BIT(ife_f_fcurve_split_lut22,        2)
REGDEF_BIT(ife_f_fcurve_split_lut23,        2)
REGDEF_BIT(ife_f_fcurve_split_lut24,        2)
REGDEF_BIT(ife_f_fcurve_split_lut25,        2)
REGDEF_BIT(ife_f_fcurve_split_lut26,        2)
REGDEF_BIT(ife_f_fcurve_split_lut27,        2)
REGDEF_BIT(ife_f_fcurve_split_lut28,        2)
REGDEF_BIT(ife_f_fcurve_split_lut29,        2)
REGDEF_BIT(ife_f_fcurve_split_lut30,        2)
REGDEF_BIT(ife_f_fcurve_split_lut31,        2)
REGDEF_END(RHE_FCURVE_SPLIT_REGISTER1)


/*
    ife_f_fcurve_val_lut0:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut1:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER0_OFS 0x0640
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER0)
REGDEF_BIT(ife_f_fcurve_val_lut0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut1,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER0)


/*
    ife_f_fcurve_val_lut2:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut3:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER1_OFS 0x0644
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER1)
REGDEF_BIT(ife_f_fcurve_val_lut2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut3,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER1)


/*
    ife_f_fcurve_val_lut4:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut5:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER2_OFS 0x0648
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER2)
REGDEF_BIT(ife_f_fcurve_val_lut4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut5,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER2)


/*
    ife_f_fcurve_val_lut6:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut7:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER3_OFS 0x064c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER3)
REGDEF_BIT(ife_f_fcurve_val_lut6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut7,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER3)


/*
    ife_f_fcurve_val_lut8:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut9:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER4_OFS 0x0650
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER4)
REGDEF_BIT(ife_f_fcurve_val_lut8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut9,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER4)


/*
    ife_f_fcurve_val_lut10:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut11:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER5_OFS 0x0654
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER5)
REGDEF_BIT(ife_f_fcurve_val_lut10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut11,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER5)


/*
    ife_f_fcurve_val_lut12:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut13:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER6_OFS 0x0658
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER6)
REGDEF_BIT(ife_f_fcurve_val_lut12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut13,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER6)


/*
    ife_f_fcurve_val_lut14:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut15:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER7_OFS 0x065c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER7)
REGDEF_BIT(ife_f_fcurve_val_lut14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut15,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER7)


/*
    ife_f_fcurve_val_lut16:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut17:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER8_OFS 0x0660
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER8)
REGDEF_BIT(ife_f_fcurve_val_lut16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut17,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER8)


/*
    ife_f_fcurve_val_lut18:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut19:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER9_OFS 0x0664
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER9)
REGDEF_BIT(ife_f_fcurve_val_lut18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut19,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER9)


/*
    ife_f_fcurve_val_lut20:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut21:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER10_OFS 0x0668
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER10)
REGDEF_BIT(ife_f_fcurve_val_lut20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut21,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER10)


/*
    ife_f_fcurve_val_lut22:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut23:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER11_OFS 0x066c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER11)
REGDEF_BIT(ife_f_fcurve_val_lut22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut23,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER11)


/*
    ife_f_fcurve_val_lut24:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut25:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER12_OFS 0x0670
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER12)
REGDEF_BIT(ife_f_fcurve_val_lut24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut25,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER12)


/*
    ife_f_fcurve_val_lut26:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut27:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER13_OFS 0x0674
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER13)
REGDEF_BIT(ife_f_fcurve_val_lut26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut27,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER13)


/*
    ife_f_fcurve_val_lut28:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut29:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER14_OFS 0x0678
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER14)
REGDEF_BIT(ife_f_fcurve_val_lut28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut29,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER14)


/*
    ife_f_fcurve_val_lut30:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut31:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER15_OFS 0x067c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER15)
REGDEF_BIT(ife_f_fcurve_val_lut30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut31,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER15)


/*
    ife_f_fcurve_val_lut32:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut33:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER16_OFS 0x0680
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER16)
REGDEF_BIT(ife_f_fcurve_val_lut32,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut33,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER16)


/*
    ife_f_fcurve_val_lut34:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut35:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER17_OFS 0x0684
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER17)
REGDEF_BIT(ife_f_fcurve_val_lut34,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut35,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER17)


/*
    ife_f_fcurve_val_lut36:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut37:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER18_OFS 0x0688
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER18)
REGDEF_BIT(ife_f_fcurve_val_lut36,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut37,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER18)


/*
    ife_f_fcurve_val_lut38:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut39:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER19_OFS 0x068c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER19)
REGDEF_BIT(ife_f_fcurve_val_lut38,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut39,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER19)


/*
    ife_f_fcurve_val_lut40:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut41:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER20_OFS 0x0690
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER20)
REGDEF_BIT(ife_f_fcurve_val_lut40,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut41,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER20)


/*
    ife_f_fcurve_val_lut42:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut43:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER21_OFS 0x0694
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER21)
REGDEF_BIT(ife_f_fcurve_val_lut42,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut43,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER21)


/*
    ife_f_fcurve_val_lut44:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut45:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER22_OFS 0x0698
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER22)
REGDEF_BIT(ife_f_fcurve_val_lut44,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut45,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER22)


/*
    ife_f_fcurve_val_lut46:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut47:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER23_OFS 0x069c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER23)
REGDEF_BIT(ife_f_fcurve_val_lut46,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut47,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER23)


/*
    ife_f_fcurve_val_lut48:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut49:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER24_OFS 0x06a0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER24)
REGDEF_BIT(ife_f_fcurve_val_lut48,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut49,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER24)


/*
    ife_f_fcurve_val_lut50:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut51:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER25_OFS 0x06a4
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER25)
REGDEF_BIT(ife_f_fcurve_val_lut50,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut51,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER25)


/*
    ife_f_fcurve_val_lut52:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut53:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER26_OFS 0x06a8
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER26)
REGDEF_BIT(ife_f_fcurve_val_lut52,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut53,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER26)


/*
    ife_f_fcurve_val_lut54:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut55:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER27_OFS 0x06ac
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER27)
REGDEF_BIT(ife_f_fcurve_val_lut54,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut55,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER27)


/*
    ife_f_fcurve_val_lut56:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut57:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER28_OFS 0x06b0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER28)
REGDEF_BIT(ife_f_fcurve_val_lut56,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut57,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER28)


/*
    ife_f_fcurve_val_lut58:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut59:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER29_OFS 0x06b4
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER29)
REGDEF_BIT(ife_f_fcurve_val_lut58,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut59,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER29)


/*
    ife_f_fcurve_val_lut60:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut61:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER30_OFS 0x06b8
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER30)
REGDEF_BIT(ife_f_fcurve_val_lut60,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut61,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER30)


/*
    ife_f_fcurve_val_lut62:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut63:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER31_OFS 0x06bc
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER31)
REGDEF_BIT(ife_f_fcurve_val_lut62,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut63,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER31)


/*
    ife_f_fcurve_val_lut64:    [0x0, 0xfff],            bits : 11_0
*/
#define RHE_FCURVE_VALUE_REGISTER32_OFS 0x06c0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER32)
REGDEF_BIT(ife_f_fcurve_val_lut64,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER32)

/*
    ife_r_degamma_en:    [0x0, 0x1],            bits : 0
    ife_r_dith_en   :    [0x0, 0x1],            bits : 4
    ife_r_segbitno  :    [0x0, 0x3],            bits : 9_8
    ife_r_dith_rst  :    [0x0, 0x1],            bits : 12
*/
#define RDE_CONTROL_OFS 0x0700
REGDEF_BEGIN(RDE_CONTROL)
REGDEF_BIT(ife_r_degamma_en,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dith_en,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_segbitno,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_r_dith_rst,        1)
REGDEF_END(RDE_CONTROL)


/*
    ife_r_dct_qtbl0_idx:    [0x0, 0x1f],            bits : 4_0
    ife_r_dct_qtbl1_idx:    [0x0, 0x1f],            bits : 12_8
    ife_r_dct_qtbl2_idx:    [0x0, 0x1f],            bits : 20_16
    ife_r_dct_qtbl3_idx:    [0x0, 0x1f],            bits : 28_24
*/
#define DCT_QTBL_REGISTER0_OFS 0x0704
REGDEF_BEGIN(DCT_QTBL_REGISTER0)
REGDEF_BIT(ife_r_dct_qtbl0_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl1_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl2_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl3_idx,        5)
REGDEF_END(DCT_QTBL_REGISTER0)


/*
    ife_r_dct_qtbl4_idx:    [0x0, 0x1f],            bits : 4_0
    ife_r_dct_qtbl5_idx:    [0x0, 0x1f],            bits : 12_8
    ife_r_dct_qtbl6_idx:    [0x0, 0x1f],            bits : 20_16
    ife_r_dct_qtbl7_idx:    [0x0, 0x1f],            bits : 28_24
*/
#define DCT_QTBL_REGISTER1_OFS 0x0708
REGDEF_BEGIN(DCT_QTBL_REGISTER1)
REGDEF_BIT(ife_r_dct_qtbl4_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl5_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl6_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl7_idx,        5)
REGDEF_END(DCT_QTBL_REGISTER1)


/*
    ife_r_out_rand2_init1:    [0x0, 0xf],           bits : 3_0
    ife_r_out_rand2_init2:    [0x0, 0x7fff],            bits : 18_4
*/
#define DITHERING_INITIAL_REGISTER1_OFS 0x0710
REGDEF_BEGIN(DITHERING_INITIAL_REGISTER1)
REGDEF_BIT(ife_r_out_rand2_init1,        4)
REGDEF_BIT(ife_r_out_rand2_init2,        15)
REGDEF_END(DITHERING_INITIAL_REGISTER1)


/*
    ife_r_degamma_tbl0:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl1:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER0_OFS 0x0714
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER0)
REGDEF_BIT(ife_r_degamma_tbl0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl1,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER0)


/*
    ife_r_degamma_tbl2:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl3:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER1_OFS 0x0718
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER1)
REGDEF_BIT(ife_r_degamma_tbl2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl3,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER1)


/*
    ife_r_degamma_tbl4:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl5:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER2_OFS 0x071c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER2)
REGDEF_BIT(ife_r_degamma_tbl4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl5,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER2)


/*
    ife_r_degamma_tbl6:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl7:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER3_OFS 0x0720
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER3)
REGDEF_BIT(ife_r_degamma_tbl6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl7,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER3)


/*
    ife_r_degamma_tbl8:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl9:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER4_OFS 0x0724
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER4)
REGDEF_BIT(ife_r_degamma_tbl8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl9,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER4)


/*
    ife_r_degamma_tbl10:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl11:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER5_OFS 0x0728
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER5)
REGDEF_BIT(ife_r_degamma_tbl10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl11,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER5)


/*
    ife_r_degamma_tbl12:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl13:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER6_OFS 0x072c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER6)
REGDEF_BIT(ife_r_degamma_tbl12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl13,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER6)


/*
    ife_r_degamma_tbl14:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl15:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER7_OFS 0x0730
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER7)
REGDEF_BIT(ife_r_degamma_tbl14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl15,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER7)


/*
    ife_r_degamma_tbl16:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl17:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER8_OFS 0x0734
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER8)
REGDEF_BIT(ife_r_degamma_tbl16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl17,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER8)


/*
    ife_r_degamma_tbl18:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl19:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER9_OFS 0x0738
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER9)
REGDEF_BIT(ife_r_degamma_tbl18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl19,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER9)


/*
    ife_r_degamma_tbl20:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl21:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER10_OFS 0x073c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER10)
REGDEF_BIT(ife_r_degamma_tbl20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl21,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER10)


/*
    ife_r_degamma_tbl22:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl23:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER11_OFS 0x0740
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER11)
REGDEF_BIT(ife_r_degamma_tbl22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl23,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER11)


/*
    ife_r_degamma_tbl24:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl25:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER12_OFS 0x0744
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER12)
REGDEF_BIT(ife_r_degamma_tbl24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl25,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER12)


/*
    ife_r_degamma_tbl26:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl27:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER13_OFS 0x0748
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER13)
REGDEF_BIT(ife_r_degamma_tbl26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl27,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER13)


/*
    ife_r_degamma_tbl28:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl29:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER14_OFS 0x074c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER14)
REGDEF_BIT(ife_r_degamma_tbl28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl29,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER14)


/*
    ife_r_degamma_tbl30:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl31:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER15_OFS 0x0750
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER15)
REGDEF_BIT(ife_r_degamma_tbl30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl31,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER15)


/*
    IFE_R_DEGAMMA_TBL32:    [0x0, 0xfff],           bits : 11_0
*/
#define DEGAMMA_TBL_REGISTER16_OFS 0x0754
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER16)
REGDEF_BIT(ife_r_degamma_tbl32,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER16)



#endif

