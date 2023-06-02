
#ifndef _IPE_ENG_BASE_H_
#define _IPE_ENG_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "kwrap/type.h"
#include "kwrap/nvt_type.h"

#define IPE_GET_APE_EN  0


/**
    @name Define IPE function enable.
*/
//@{
#define IPE_RGBLPF_EN                  0x00000001  ///< RGB LPF function enable
#define IPE_RGBGAMMA_EN                0x00000002  ///< RGB Gamma function enable
#define IPE_YCURVE_EN                  0x00000004  ///< Y curve function enable
#define IPE_CCR_EN                     0x00000008  ///< Color correction function enable
#define IPE_DEFOG_SUBOUT_EN            0x00000010  ///< Defog subimg output function enable
#define IPE_DEFOG_EN                   0x00000020  ///< Defog function enable
#define IPE_LCE_EN                     0x00000040  ///< Local contrast enhancement function enable
#define IPE_CST_EN                     0x00000080  ///< Color space transform function enable, RGB->YCC
#define IPE_CTRL_EN                    0x00000200  ///< Color control function enable
#define IPE_HADJ_EN                    0x00000400  ///< Hue adjustment function enable
#define IPE_CADJ_EN                    0x00000800  ///< Color component adjust function enable
#define IPE_CADJ_YENH_EN               0x00001000  ///< Color component edge enhance function enable, sub-function of IPE_CADJ_EN
#define IPE_CADJ_YCON_EN               0x00002000  ///< Color component Y contrast adjust function enable, sub-function of IPE_CADJ_EN
#define IPE_CADJ_CCON_EN               0x00004000  ///< Color component CbCr contrast adjust function enable, sub-function of IPE_CADJ_EN
#define IPE_YCTH_EN                    0x00008000  ///< Color component YCbCr substitution function enable, sub-function of IPE_CADJ_EN
#define IPE_CSTP_EN                    0x00010000  ///< Color space transform protect function enable
#define IPE_EDGE_DBG_EN                0x00020000  ///< Edge debug mode function enable
#define IPE_VACC_EN                    0x00080000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN0_EN           0x00100000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN1_EN           0x00200000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN2_EN           0x00400000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN3_EN           0x00800000  ///< Variation accumulation statistics function enable
#define IPE_VA_IDNEP_WIN4_EN          0x01000000  ///< Variation accumulation statistics function enable
#define IPE_PFR_EN                     0x02000000  ///< Purple fringe reductin function enable
//@}



#define IPE_DBG_NONE             0x00000000
#define IPE_DBG_VA               0x00000001
#define IPE_DBG_DEFOG            0x00000002
#define IPE_DBG_CCM              0x00000004
#define IPE_DBG_EEXT             0x00000008
#define IPE_DBG_DEFAULTON        0x10000000


extern UINT32 ipe_dbg_cmd;


/**
    IPE DRAM input SRAM update selection

\n Used for ipe_change_param().
*/
typedef enum {
	IPE_READ_LUT = 0,        ///< IPE read SRAM
	DMA_WRITE_LUT = 1,       ///< DMA write SRAM
	CPU_READ_LUT = 2,        ///< CPU read SRAM
	ENUM_DUMMY4WORD(IPE_RW_SRAM_ENUM)
} IPE_RW_SRAM_ENUM;

/**
    IPE Load from DRAM to Gamma SRAM update option

    Select Gamma load option
*/
typedef enum {
	LOAD_GAMMA_R = 0,          ///< Load Gamma R
	LOAD_GAMMA_RGB = 1,        ///< Load Gamma RGB
	LOAD_GAMMA_Y = 2,          ///< Load Y curve
	LOAD_GAMMA_R_Y = 3,        ///< Load Gamma R and then Y curve
	ENUM_DUMMY4WORD(IPE_RWGAM_OPT)
} IPE_RWGAM_OPT;

/**
    IPE structure - IPE stripe.
*/
//@{
typedef struct {
	UINT32    hn;        ///< Size of stripe except for last one, H unit : 16 pix, V unit : 1 line
	UINT32    hl;        ///< Size of last stripe, H unit : 16 pix, V unit : 1 line
	UINT32    hm;        ///< Number of total stripe - 1
	//IPE_OLAP_ENUM    Olap; ///< Overlap enable selection for multi-stripe mode
} STR_STRIPE_INFOR;
//@}



/**
    IPE structure - IPE size.
*/
//@{
typedef struct {
	UINT32    h_size;    ///< Horizontal image size, unit : pix
	UINT32    v_size;    ///< Vertical image size, uint : line
} IPE_IMG_SIZE;
//@}



/**
    IPE Direct to IFE2 enable

    Enable IPE direct to IFE2
\n Used for ipe_changeOutput().
*/
typedef enum {
	B_DISABLE = 0,         ///< disable
	B_ENABLE = 1,          ///< enable
	ENUM_DUMMY4WORD(IPE_ENABLE)
} IPE_ENABLE;


/**
    IPE parameter mode
*/
typedef enum {
	IPE_PARAM_AUTO_MODE  = 0,   ///< parameters using auto mode
	IPE_PARAM_USER_MODE  = 1,   ///< parameters using user mode
	ENUM_DUMMY4WORD(IPE_PARAM_MODE)
} IPE_PARAM_MODE;



/**
    IPE structure - IPE edge eext strength
*/
typedef struct {
	UINT32 eext_enh;            ///< Legal range : 0~63, enhance term for kernel
	INT32 eext_div;             ///< Legal range : -4~3, normalize term for kernel
} EEXT_KER_STR;

/**
    IPE structure - IPE edge eext strength
*/
typedef struct {
	EEXT_KER_STR ker_a;         ///< strength of kernal A
	EEXT_KER_STR ker_c;         ///< strength of kernal C
	EEXT_KER_STR ker_d;         ///< strength of kernal D
} IPE_EEXT_KER_STRENGTH;


/**
    IPE structure - IPE edge eext, strength of edge contrast and edge energy
*/
//@{
typedef struct {
	INT32 eext_div_con;         ///< Legal range : -8~7, normalize term for edge contrast
	INT32 eext_div_eng;         ///< Legal range : -8~7, normalize term for edge engergy
	UINT32 wt_con_eng;          ///< Legal range : 0~8, blending weight between edge engergy and edge contrast, 0: all contrast, 8: all edge energy
} IPE_EEXT_ENG_CON;

/**
    IPE structure - IPE edge kernel thickness
                        ker B_C     ker A_B
                        |           |
        iso_ker_robust  |           | iso_ker_thin
                        |           |
                    ker D        ker C

    input: ker A & ker B -> wt_ker_thin -> output: kerA_B
    input: ker B & ker C -> wt_ker_robust -> output: kerB_C
    input: ker A_B & ker C -> iso_ker_thin -> output: kerAB_C
    input: ker B_C & ker D -> iso_ker_robust -> output: kerBC_D
*/
//@{
typedef struct {
	UINT32 wt_ker_thin;         ///< Legal range : 0~8, Thin kernel weighting between A & B, 8: all thinner kernel A
	UINT32 wt_ker_robust;       ///< Legal range : 0~8, robust kernel weighting between B & C, 8: all thinner kernel B
	UINT32 iso_ker_thin;        ///< Legal range : 0~8, Thin kernel weighting between A_B & C, 8: all thinner kernel A_B
	UINT32 iso_ker_robust;      ///< Legal range : 0~8, robust kernel weighting between B_C & D, 8: all thinner kernel B_C

} IPE_KER_THICKNESS;


/**
    IPE structure - IPE region segmetation parameters
*/
//@{
typedef struct {
	UINT32 wt_low;              ///< Legal range : 0~16, the kernel weighting of flat region
	UINT32 wt_high;             ///< Legal range : 0~16, the kernel weighting of edge region
	UINT32 th_flat;             ///< Legal range : 0~1023, the threshold for flat region
	UINT32 th_edge;             ///< Legal range : 0~1023, the threshold for edge region

	UINT32 wt_low_hld;          ///< Legal range : 0~16, the kernel weighting of flat region in HLD mode
	UINT32 wt_high_hld;         ///< Legal range : 0~16, the kernel weighting of edge region in HLD mode
	UINT32 th_flat_hld;         ///< Legal range : 0~1023, the threshold for flat region in HLD mode
	UINT32 th_edge_hld;         ///< Legal range : 0~1023, the threshold for edge region in HLD mode
	UINT32 th_lum_hld;          ///< Legal range : 0~1023, the luma threshold in HLD mode

	//slope mode
	IPE_PARAM_MODE region_slope_mode;   ///< input sub-image sclae mode selection

	UINT16 slope_con_eng;       ///< Legal range : 0~65535,
	UINT16 slope_con_eng_hld;   ///< Legal range : 0~65535,

} IPE_REGION_PARAM;


typedef struct _IPE_REGION_STRENGTH_PARAM_ {
    IPE_ENABLE region_str_en;   ///< Enable region strength control
    UINT8      enh_thin;        ///< Strength for thin kernel
    UINT8      enh_robust;      ///< Strength for robust kernel
    INT16     slope_flat;      ///< Strength slope of the flat region
    INT16     slope_edge;      ///< Strength slope of the edge region
    UINT8      str_flat;        ///< Strength for flat region
    UINT8      str_edge;        ///< Strength for edge region
} IPE_REGION_STRENGTH_PARAM;


/**
    IPE structure - IPE edge statistic result
*/
//@{
typedef struct {
	BOOL overshoot_en;
	UINT32 wt_overshoot;
	UINT32 wt_undershoot;

	UINT32 th_overshoot;
	UINT32 th_undershoot;
	UINT32 th_undershoot_lum;
	UINT32 th_undershoot_eng;

	UINT32 slope_overshoot;         ///< Legal range : 0~32767,
	UINT32 slope_undershoot;        ///< Legal range : 0~32767,
	UINT32 slope_undershoot_lum;    ///< Legal range : 0~32767,
	UINT32 slope_undershoot_eng;    ///< Legal range : 0~32767,

	UINT32 clamp_wt_mod_lum;
	UINT32 clamp_wt_mod_eng;

	UINT32 strength_lum_eng;
	UINT32 norm_lum_eng;

} IPE_OVERSHOOT_PARAM;


/**
    IPE structure -
*/
//@{
typedef struct {

	UINT32 localmax_max;
	UINT32 coneng_max;
	UINT32 coneng_avg;

} IPE_EDGE_STCS_RSLT;

/**
    IPE structure - IPE luminance map
*/
//@{
typedef struct {
	UINT32     ethr_low;                   ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     ethr_high;                  ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     etab_low;                   ///< Legal range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32     etab_high;                  ///< Legal range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
} IPE_ESMAP_INFOR;


/**
    IPE edge mapping input selection

    Select intput data for edge mapping
\n Used for ipe_change_param().
*/
typedef enum {
	EIN_ENG = 0,            ///< Select edge energy for edge mapping input
	EIN_EEXT = 1,           ///< Select EEXT for edge mapping input
	EIN_AVG = 2,            ///< Select average of EEXT & edge energy for edge mapping input
	ENUM_DUMMY4WORD(IPE_EMAP_IN_ENUM)
} IPE_EMAP_IN_ENUM;

/**
    IPE structure - IPE edge map
*/
//@{
typedef struct {
	UINT32     ethr_low;                   ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     ethr_high;                  ///< Legal range : 0~1023, for edge mapping, if(Ein < EthrA){Eout=EDTabA[0]}, else if(EthrA <= Ein <EthrB){Eout apply EtabA and EDtabA}, else if(Ein >= EthrB){Eout apply EtabB and EDtabB}
	UINT32     etab_low;                   ///< Legal range : 0~6, for edge mapping, EDtabA table has 8 sections, each section width is (1 << etaA)
	UINT32     etab_high;                  ///< Legal range : 0~6, for edge mapping, EDtabB table has 8 sections, each section width is (1 << etaB)
	IPE_EMAP_IN_ENUM ein_sel;                ///< Edge mapping input selection
} IPE_EDGEMAP_INFOR;

/**
    IPE RGB LPF: kernal size selection

    Select kernel size of low-pass filter

*/
typedef enum {
	LPFSIZE_3X3 = 0,    ///< 3x3 filter size
	LPFSIZE_5X5 = 1,    ///< 5x5 filter size
	LPFSIZE_7X7 = 2,    ///< 7x7 filter size
	ENUM_DUMMY4WORD(IPE_LPF_SIZE_ENUM)
} IPE_LPF_SIZE_ENUM;


/**
    IPE structure - IPE RGB LPF
*/
//@{
typedef struct {
	UINT32 lpfw;              ///< LPF weighting
	UINT32 sonly_w;            ///< S-Only weighting
	UINT32 range_th0;          ///< Range threshold 0
	UINT32 range_th1;          ///< Range threshold 1
	IPE_LPF_SIZE_ENUM filt_size;
} STR_RGBLPF_INFOR;

/**
    IPE structure - IPE RGB LPF
*/
//@{
typedef struct {
	STR_RGBLPF_INFOR    lpf_r_info;  ///< LPF setting of R channel
	STR_RGBLPF_INFOR    lpf_g_info;  ///< LPF setting of G channel
	STR_RGBLPF_INFOR    lpf_b_info;  ///< LPF setting of B channel
} IPE_RGBLPF_PARAM;


/**
    IPE structure - IPE PFR color wetting
*/
//@{
typedef struct {
	BOOL enable;
	UINT32 color_u;
	UINT32 color_v;
	UINT32 color_wet_r;
	UINT32 color_wet_b;
	UINT32 cdiff_th;
	UINT32 cdiff_step;
	UINT32 cdiff_lut[5];
} IPE_PFR_COLOR_WET;

/**
    IPE structure - IPE PFR
*/
//@{
typedef struct {
	BOOL uv_filt_en;
	BOOL luma_level_en;
	UINT32 wet_out;
	UINT32 edge_th;
	UINT32 edge_strength;
	UINT32 luma_th;
	UINT32 *p_luma_lut;
	UINT32 color_wet_g;
	IPE_PFR_COLOR_WET color_wet_set[4];

} IPE_PFR_PARAM;



/**
    IPE CC range: precision selection

    Select precision 2.8 or 3.7 of color correction

*/
typedef enum {
	CCRANGE_2_9 = 0,    ///< Precision 2.9
	CCRANGE_3_8 = 1,    ///< Precision 3.8
	CCRANGE_4_7 = 1,    ///< Precision 4.7
	ENUM_DUMMY4WORD(IPE_CC_RANGE_ENUM)
} IPE_CC_RANGE_ENUM;

/**
    IPE CC output: weighting reference

    Select identity or gray for output weighting

*/
typedef enum {
	CC2_IDENTITY = 0,   ///< Identity matrix
	CC2_GRAY = 1,       ///< Gray matrix
	ENUM_DUMMY4WORD(IPE_CC2_SEL_ENUM)
} IPE_CC2_SEL_ENUM;

/**
    IPE color correction gamma selection

    Select pre or post gamma for color correction
*/
typedef enum {
	CC_PRE_GAM = 0,       ///< Select pre-gamma color correction
	CC_POST_GAM = 1,      ///< Select post-gamma color correction
	ENUM_DUMMY4WORD(IPE_CC_GAMSEL)
} IPE_CC_GAMSEL;

/**
    IPE color correction stab option

    Select input of color correction stab
*/
typedef enum {
	CC_MAX = 0,             ///< Select max
	CC_MIN = 1,             ///< Select min
	CC_MAX_MINUS_MIN = 2,   ///< Select max-min
	ENUM_DUMMY4WORD(IPE_CC_STABSEL)
} IPE_CC_STABSEL;

/**
    IPE color correction offset option

    Select mode of color correction offset
*/

typedef enum {
	CC_OFS_BYPASS = 0,      ///< Select bypass
	CC_OFS_Y_FULL = 1,      ///< Select offset for full YCC to RGB
	CC_OFS_Y_BTU  = 2,      ///< Select offset for Bt601/Bt709 YCC to RGB
	ENUM_DUMMY4WORD(IPE_CC_OFSSEL)
} IPE_CC_OFSSEL;


/**
    IPE structure - IPE CC.
*/
//@{
typedef struct {
	IPE_CC_RANGE_ENUM cc_range;   ///< Range of matrix coefficients,0: 3.8 format,1: 4.7format
	IPE_CC2_SEL_ENUM cc2_sel;     ///< Select the other data of CC weighting
	IPE_CC_GAMSEL cc_gam_sel;     ///< Select pre- or post-gamma CC
	IPE_CC_STABSEL cc_stab_sel;   ///< Select CC stab input
	IPE_CC_OFSSEL cc_ofs_sel;     ///< Select CC offset mode //sc
	INT16 *p_cc_coeff;            ///< Color correction matrix [rr rg rb gr gg gb br bg bb]
	UINT8 *p_fstab;               ///< Color correction stab
	UINT8 *p_fdtab;               ///< Color correction dtab
} IPE_CC_PARAM;


/**
    IPE color correction stab option

    Select input of color correction stab
*/
typedef enum {
	CST_NOOP = 0,             ///< No operation
	CST_MINUS128 = 1,         ///< out = in - 128
	ENUM_DUMMY4WORD(IPE_CST_OFFSEL)
} IPE_CST_OFFSEL;


/**
    IPE structure - IPE CST.
*/
//@{
typedef struct {
	INT16 *p_cst_coeff;            ///< Color space transform matrix [yr yg yb ur ug ub vr vg vb]
	IPE_CST_OFFSEL cstoff_sel;    ///< Operation when CST is off
} IPE_CST_PARAM;


/**
    IPE color control source option

    Select edge reference of color control vdet
*/
typedef enum {
	CCTRL_1STORDER = 0,     ///< 5x5 kernel B output
	CCTRL_E5 = 1,           ///< 5x5 kernel A output
	CCTRL_E7 = 2,           ///< 7x7 kernel output
	ENUM_DUMMY4WORD(IPE_CCTRL_SEL_ENUM)
} IPE_CCTRL_SEL_ENUM;

/**
    IPE structure - IPE noise effect.
*/
//@{
typedef struct {
	IPE_ENABLE yrand_en;     ///< Enable of random noise on Y
	IPE_ENABLE crand_en;     ///< Enable of random noise on C
	UINT32 yrand_level;    ///< Legal range : 0~7, 0 : NO Y random noise, 7 : MAX Y random noise level
	UINT32 crand_level;    ///< Legal range : 0~7, 0 : NO CbCr random noise, 7 : MAX CbCr random noise level
	BOOL yc_rand_rst;        ///< Legal range : 0~1, 0 : No reset for random pattern sequence, 1 : Frame start reset for random pattern seqeunce.
} IPE_CADJ_RAND_PARAM;
//@}


/**
    IPE YC thresholding: replacement option

    Select replacement with the original or a value specified by the register
*/
typedef enum {
	YCTH_ORIGINAL_VALUE = 0,
	YCTH_REGISTER_VALUE = 1,
	ENUM_DUMMY4WORD(IPE_YCTH_SEL_ENUM)
} IPE_YCTH_SEL_ENUM;


/**
    IPE structure - IPE Y threshold 1
*/
//@{
typedef struct {
	UINT32 y_th;               ///< Y threshold
	UINT32 edgeth;               ///< Edge threshold
	IPE_YCTH_SEL_ENUM hit_sel;   ///< Value selection when hit
	IPE_YCTH_SEL_ENUM nhit_sel;  ///< Value selection when not-hit
	UINT32 hit_value;          ///< The register value when hit
	UINT32 nhit_value;         ///< The register value when not-hit
} STR_YTH1_INFOR;


/**
    IPE structure - IPE Y threshold 2
*/
//@{
typedef struct {
	UINT32 y_th;               ///< Y threshold
	IPE_YCTH_SEL_ENUM hit_sel;   ///< Value selection when hit
	IPE_YCTH_SEL_ENUM nhit_sel;  ///< Value selection when not-hit
	UINT32 hit_value;          ///< The register value when hit
	UINT32 nhit_value;         ///< The register value when not-hit
} STR_YTH2_INFOR;

typedef enum {
	SATURATION = 0,         ///<
	ABS_MEAN = 1,           ///<
	ENUM_DUMMY4WORD(IPE_CCONTAB_SEL)
} IPE_CCONTAB_SEL;


/**
    IPE structure - IPE C threshold
*/
//@{
typedef struct {
	UINT32 edgeth;               ///< Edge threshold
	UINT32 yth_low;              ///< Low threshold of Y
	UINT32 yth_high;              ///< High threshold of Y
	UINT32 cbth_low;             ///< Low threshold of CB
	UINT32 cbth_high;             ///< High threshold of CB
	UINT32 crth_low;             ///< Low threshold of CR
	UINT32 crth_high;             ///< High threshold of CR
	IPE_YCTH_SEL_ENUM hit_sel;   ///< Value selection when hit
	IPE_YCTH_SEL_ENUM nhit_sel;  ///< Value selection when not-hit
	UINT32 cb_hit_value;        ///< The register value of CB when hit
	UINT32 cb_nhit_value;       ///< The register value of CB when not-hit
	UINT32 cr_hit_value;        ///< The register value of CR when hit
	UINT32 cr_nhit_value;       ///< The register value of CR when not-hit
} STR_CTH_INFOR;
//@}


/**
    IPE ycurve selection
*/
typedef enum {
	YCURVE_RGB_Y     = 0,       ///<
	YCURVE_RGB_V     = 1,       ///<
	YCURVE_YUV       = 2,       ///<
	ENUM_DUMMY4WORD(IPE_YCURVE_SEL)
} IPE_YCURVE_SEL;

/**
    IPE structure - IPE random input of gamma and y curve
*/
//@{
typedef struct {
	IPE_ENABLE gam_y_rand_en;  ///< Enable of random input
	IPE_ENABLE gam_y_rand_rst; ///< Reset of random input
	UINT8 gam_y_rand_shft;   ///< Shift of random input
} IPE_GAMYRAND;
//@}


/**
    IPE 3DCC rounding

    Select 3DCC rounding type
*/
typedef enum {
	ROUNDING = 0,             ///< rounding
	HALFTONE = 1,             ///< halftone rounding
	RANDOM = 2,               ///< random rounding
	ENUM_DUMMY4WORD(IPE_DEFOG_ROUNDOPT)
} IPE_DEFOG_ROUNDOPT;


/**
    IPE structure - IPE 3DCC rounding
*/
//@{
typedef struct {
	IPE_DEFOG_ROUNDOPT rand_opt;   ///< Rounding option
	IPE_ENABLE rand_rst;           ///< Reset of random input
} IPE_DEFOGROUND;
//@}


/**
    IPE edge debug mode selection
*/
typedef enum {
	DBG_EDGE_REGION    = 0,       ///<
	DBG_EDGE_WEIGHT    = 1,       ///<
	DBG_EDGE_STRENGTH  = 2,       ///<
	ENUM_DUMMY4WORD(IPE_EDGEDBG_SEL)
} IPE_EDGEDBG_SEL;


//@{
/**
    IPE burst length selection

    Select input/output burst length mode.
\n Used for ipe_Dram2Dram().
*/
typedef enum {
	IPE_BURST_32W    = 0,       ///< burst length 32 words
	IPE_BURST_16W    = 1,       ///< burst length 16 words
	ENUM_DUMMY4WORD(IPE_BURST_SEL)
} IPE_BURST_SEL;
//@}


/**
    IPE structure - IPE input/output burst length
*/
//@{
typedef struct _IPE_BURST_LENGTH {
	IPE_BURST_SEL   burstlen_in_y;  ///< Input burst length
	IPE_BURST_SEL   burstlen_in_c;  ///< Input burst length
	IPE_BURST_SEL   burstlen_out_y; ///< Output burst length
	IPE_BURST_SEL   burstlen_out_c; ///< Output burst length
} IPE_BURST_LENGTH;
//@}



/**
    IPE ETH ouput downsampling

    Select if ETH is downsampled at output
*/
typedef enum {
	ETH_OUT_ORIGINAL = 0,             ///< original size
	ETH_OUT_DOWNSAMPLED = 1,          ///< downsampled by 2x2
	ETH_OUT_BOTH = 2,                 ///< both origianl size and downsample size
	ENUM_DUMMY4WORD(IPE_ETH_OUTSEL)
} IPE_ETH_OUTSEL;

/**
    IPE ETH ouput format

    Select ETH output bitdepth
*/
typedef enum {
	ETH_OUT_2BITS = 0,             ///< 2 bits/pixel
	ETH_OUT_8BITS = 1,             ///< 8 bits/pixel
	ENUM_DUMMY4WORD(IPE_ETH_OUTFMT)
} IPE_ETH_OUTFMT;



/**
    IPE structure - Output information of IPE Edge Thresholding
*/
//@{
typedef struct {
	IPE_ETH_OUTSEL eth_outsel;   ///< If ETH output is downsampled
	IPE_ETH_OUTFMT eth_outfmt;   ///< ETH output format
} IPE_ETH_SIZE;
//@}


/**
    IPE ETH ouput position

    Select ETH output position when downsampling
*/
typedef enum {
	ETH_POS_EVEN = 0,             ///< even position
	ETH_POS_ODD = 1,              ///< odd position
	ENUM_DUMMY4WORD(IPE_ETH_POSSEL)
} IPE_ETH_POSSEL;


/**
    IPE structure - Parameters of IPE Edge Thresholding
*/
//@{
typedef struct {
	UINT32 eth_low;              ///< Low edge threshold value
	UINT32 eth_mid;              ///< Middle edge threshold value
	UINT32 eth_high;             ///< High edge threshold value
	IPE_ETH_POSSEL eth_hout;     ///< Horizontal position selection when downsampling
	IPE_ETH_POSSEL eth_vout;     ///< Vertical position selection when downsampling
} IPE_ETH_PARAM;
//@}


typedef enum {
	VA_FLTR_MIRROR  = 0,   ///<
	VA_FLTR_INVERSE = 1,   ///<
	ENUM_DUMMY4WORD(IPE_VA_FLTR_SYMM_SEL)
} IPE_VA_FLTR_SYMM_SEL;

typedef enum {
	VA_FLTR_SIZE_1  = 0,   ///<
	VA_FLTR_SIZE_3  = 1,   ///<
	VA_FLTR_SIZE_5  = 2,   ///<
	VA_FLTR_SIZE_7  = 3,   ///<
	ENUM_DUMMY4WORD(IPE_VA_FLTR_SIZE_SEL)
} IPE_VA_FLTR_SIZE_SEL;


typedef struct {
	UINT8                   tap_a;     ///<
	INT8                    tap_b;      ///<
	INT8                    tap_c;      ///<
	INT8                    tap_d;      ///<
	IPE_VA_FLTR_SYMM_SEL    filt_symm;   ///<
	IPE_VA_FLTR_SIZE_SEL    fltr_size;   ///<
	UINT8                   div;      ///<
	UINT16                  th_low;      ///<
	UINT16                  th_high;      ///<
} IPE_VA_FLTR_PARAM;


typedef struct {
	IPE_VA_FLTR_PARAM       filt_h;
	IPE_VA_FLTR_PARAM       filt_v;
	BOOL                    linemax_en;   ///<
	BOOL                    cnt_en;       ///<
} IPE_VA_FLTR_GROUP_PARAM;


typedef struct {
	UINT32              win_stx;    ///< Variation accumulation - horizontal starting point
	UINT32              win_sty;    ///< Variation accumulation - vertical starting point
	UINT32              win_numx;    ///< Variation accumulation - horizontal window number
	UINT32              win_numy;    ///< Variation accumulation - vertical window number
	UINT32              win_szx;    ///< Variation accumulation - horizontal window size
	UINT32              win_szy;    ///< Variation accumulation - vertical window size
	UINT32              win_spx;    ///< Variation accumulation - horizontal window spacing
	UINT32              win_spy;    ///< Variation accumulation - vertical window spacing
} IPE_VA_WIN_PARAM;

typedef struct {
	BOOL                indep_va_en;
	UINT32              win_stx;    ///< horizontal window start
	UINT32              win_sty;    ///< vertical window start
	UINT32              win_szx;    ///< horizontal window size
	UINT32              win_szy;    ///< vertical window size
	BOOL                linemax_g1_en;
	BOOL                linemax_g2_en;
} IPE_INDEP_VA_PARAM;


typedef enum {
	VA_OUT_GROUP1  = 0,   ///<
	VA_OUT_BOTH    = 1,   ///<
	ENUM_DUMMY4WORD(IPE_VA_OUTSEL)
} IPE_VA_OUTSEL;


typedef struct {
	UINT32 *p_g1_h;
	UINT32 *p_g1_v;
	UINT32 *p_g2_h;
	UINT32 *p_g2_v;
	UINT32 *p_g1_h_cnt;
	UINT32 *p_g1_v_cnt;
	UINT32 *p_g2_h_cnt;
	UINT32 *p_g2_v_cnt;
} IPE_VA_RSLT;


typedef struct {
	BOOL va_en;
	UINT32 win_num_x;
	UINT32 win_num_y;
	IPE_VA_OUTSEL outsel;
	UINT32 address;
	UINT32 lineoffset;
} IPE_VA_SETTING;


typedef struct {
	UINT32              va_g1_h;
	UINT32              va_g1_v;
	UINT32              va_g2_h;
	UINT32              va_g2_v;
	UINT32              va_cnt_g1_h;
	UINT32              va_cnt_g1_v;
	UINT32              va_cnt_g2_h;
	UINT32              va_cnt_g2_v;
} IPE_INDEP_VA_WIN_RSLT;

/**
    IPE defog target method selection
*/
typedef enum {
	METHOD_A    = 0,       ///<
	METHOD_B    = 1,       ///<
	ENUM_DUMMY4WORD(IPE_DEFOG_METHOD_SEL)
} IPE_DEFOG_METHOD_SEL;

/**
    IPE structure - Defog statistics result
*/
typedef struct {
	UINT16              airlight[3];

} DEFOG_STCS_RSLT;



/**
    IPE mode selection

    Select ipe mode
*/
typedef enum {
	IPE_OPMODE_D2D             = 0,    ///< D2D
	IPE_OPMODE_IPP             = 1,    ///< direct mode, (IFE->)DCE->IPE->IME
	IPE_OPMODE_SIE_IPP         = 2,    ///< all direct mode, SIE->IFE->DCE->IPE->IME
	IPE_OPMODE_UNKNOWN         = 3,    ///< Undefined mode
	ENUM_DUMMY4WORD(IPE_OPMODE)
} IPE_OPMODE;


/**
    IPE output YCC format selection

    Select IPE YCC output format
\n Used for ipe_changeOutput().
*/
typedef enum {
	IPE_YCC444 = 0,         ///< Select YCC 424 format
	IPE_YCC422 = 1,         ///< Select YCC 422 format
	IPE_YCC420 = 2,          ///< Select YCC 420 format
	IPE_YONLY = 3,
	ENUM_DUMMY4WORD(IPE_INOUT_FORMAT)
} IPE_INOUT_FORMAT;

/**
    IPE DRAM output selection

    Select IPE output data type
\n Used for ipe_changeOutput().
*/
typedef enum {
	IPE_ORIGINAL = 0,           ///< original size, use both Y & C channels
	//IPE_SUBOUT = 1,             ///< subsampled out
	IPE_REGION = 2,             ///< edge region, use output C channel
	IPE_ETH = 3,                ///< edge thresholding, use output Y & C channel
	ENUM_DUMMY4WORD(IPE_DMAOSEL)
} IPE_DMAOSEL;

typedef struct {
    uint8_t stp_size_unit;      ///< stripe size unit
    uint8_t stp_overlap_size;   ///< stripe overlap size
} IPE_STP_CAL_LIMT;


/**
    IPE structure - IPE size info.
*/
//@{
typedef struct {
	IPE_OPMODE ipe_mode;             ///< IPE operation mode
	IPE_IMG_SIZE in_size_y;           ///< Input image H V size, effective if StripeMode is IPE_SSTP or IPE_AUTOSTP. If StripeMode is IPE_MSTP, HSTRIPE and VSTRIPE must be input
	IPE_INOUT_FORMAT in_fmt;         ///< Input YCC format
	IPE_INOUT_FORMAT out_fmt;        ///< Output YCC format
	BOOL yc_to_dram_en;              ///< Enable of YC data to DRAM
	IPE_DMAOSEL dram_out_sel;          ///< DMA output channel data selection
	IPE_ETH_SIZE ethout_info;      ///< ETH output information
} IPE_STRPINFO;
//@}


/**
    IPE dram single output mode selection

    Select dram single output mode
*/
typedef enum {
	IPE_DRAM_OUT_NORMAL            = 0,    ///<
	IPE_DRAM_OUT_SINGLE            = 1,    ///<
	ENUM_DUMMY4WORD(IPE_DRAM_OUTPUT_MODE)
} IPE_DRAM_OUTPUT_MODE;

/**
    IPE structure - Dram single output channel enable
*/
//@{
typedef struct {
	BOOL single_out_y_en;
	BOOL single_out_c_en;
	BOOL single_out_va_en;
	//BOOL single_out_defog_en;
	BOOL single_out_lce_en;
} IPE_DRAM_SINGLE_OUT_EN;

/**
    IPE output YCC subsample selection

    Select IPE YCC 422 420 subsample method
\n Used for ipe_changeOutput().
*/
typedef enum {
	YCC_DROP_RIGHT = 0,         ///< When 422 or 420 format is chosen, drop right pixel for subsample
	YCC_DROP_LEFT = 1,          ///< When 422 or 420 format is chosen, drop left pixel for subsample
	ENUM_DUMMY4WORD(IPE_HSUB_SEL_ENUM)
} IPE_HSUB_SEL_ENUM;



/**
    IPE structure - IPE output info.
*/
//@{
typedef struct {
	BOOL yc_to_ime_en;                      ///< Enable of YC data to IME
	BOOL yc_to_dram_en;                     ///< Enable of YC data to DRAM
	BOOL va_to_dram_en;                     ///< Enable of VA data to DRAM, combine with VACC
	BOOL defog_to_dram_en;                  ///< Enable of Defog data to DRAM
	IPE_DRAM_OUTPUT_MODE dram_out_mode;     ///< IPE DRAM output mode selection, 0: normal, 1: single out mode
	IPE_DRAM_SINGLE_OUT_EN single_out_en;   ///< IPE DRAM single output channel enable
	IPE_INOUT_FORMAT yc_format;             ///< IPE DRAM output source data selection
	IPE_HSUB_SEL_ENUM sub_h_sel;            ///< IPE output YCC subsample selection
	UINT32 addr_y;                          ///< Legal range : multiples of 4(word aligned), DRAM output address, data type selected by dram_out_sel
	UINT32 addr_c;                          ///< Legal range : multiples of 4(word aligned), DRAM output address, data type selected by dram_out_sel
	UINT32 addr_va;                         ///< Legal range : multiples of 4(word aligned), VA DRAM output address
	UINT32 addr_defog;                      ///< Legal range : multiples of 4(word aligned), Defog DRAM output address
	UINT32 lofs_y;                          ///< Legal range : multiples of 4(word aligned), DRAM output lineoffset, data type selected by dram_out_sel
	UINT32 lofs_c;                          ///< Legal range : multiples of 4(word aligned), DRAM output lineoffset, data type selected by dram_out_sel
	UINT32 lofs_va;                         ///< Legal range : multiples of 4(word aligned), VA DRAM output lineoffset
	UINT32 lofs_defog;                      ///< Legal range : multiples of 4(word aligned), Defog DRAM output lineoffset

	IPE_DMAOSEL dram_out_sel;               ///< DMA output channel data selection
	IPE_ETH_SIZE ethout_info;               ///< ETH output information
	IPE_VA_OUTSEL va_outsel;                ///< VA output information

} IPE_OUTYCINFO;


extern IPE_VA_SETTING va_ring_setting[2];


//----------------------------------------------
extern void ipe_eng_set_horizontal_stripe_buf_reg(UINT32 set_hn, UINT32 set_hl, UINT32 set_hm);

extern void ipe_eng_set_vertical_stripe_buf_reg(UINT32 set_vl);

extern void ipe_eng_clear_frame_end(void);

extern void ipe_eng_wait_frame_end(BOOL is_clr_flg_sel);

extern void ipe_eng_clear_frame_start(void);

extern void ipe_eng_wait_frame_start(BOOL is_clr_flg_sel);

extern void ipe_eng_clear_dma_end(void);

extern void ipe_eng_wait_dma_end(BOOL is_clr_flg_sel);

extern void ipe_eng_clear_gamma_in_end(void);

extern void ipe_eng_wait_gamma_in_end(void);

extern void ipe_eng_clear_linked_list_job_end(void);

extern void ipe_eng_wait_linked_list_job_end(BOOL is_clr_flg_sel);

extern void ipe_eng_clear_linked_list_done(void);

extern void ipe_eng_wait_linked_list_done(BOOL is_clr_flg_sel);

extern void ipe_eng_get_stripe_cal_limt(IPE_STP_CAL_LIMT *p_get_info);

extern ER ipe_eng_cal_stripe(IPE_STRPINFO *p_strp_info);

extern void ipe_eng_set_enable_feature_buf_reg(UINT32 features);

extern void ipe_eng_set_disable_feature_buf_reg(UINT32 features);

extern void ipe_eng_set_op_mode_buf_reg(UINT32 mode);

extern void ipe_eng_set_dram_out_mode_buf_reg(UINT32 mode);

extern void ipe_eng_set_dram_single_channel_enable_buf_reg(BOOL out_y_en, BOOL out_c_en, BOOL out_va_en, BOOL out_lce_en);

extern void ipe_eng_set_output_switch_buf_reg(BOOL to_ime_en, BOOL to_dram_en);

extern void ipe_eng_set_dma_out_sel_buf_reg(UINT32 sel);

extern void ipe_eng_set_hovlp_opt_buf_reg(UINT32 opt);

extern void ipe_eng_set_in_format_buf_reg(UINT32 imat);

extern void ipe_eng_set_out_format_buf_reg(UINT32 omat, UINT32 sub_h_sel);

extern void ipe_eng_set_feature_buf_reg(UINT32 features);

extern void ipe_eng_set_linked_list_addr_buf_reg(UINT32 addr);

extern void ipe_eng_set_dma_in_addr_buf_reg(UINT32 addr_in_y, UINT32 addr_in_c);

//extern void ipe_eng_set_dma_in_defog_addr_buf_reg(UINT32 addr);
extern void ipe_eng_set_dma_in_lce_addr_buf_reg(UINT32 addr);;

extern void ipe_eng_set_dma_in_gamma_lut_addr_buf_reg(UINT32 addr);

extern void ipe_eng_set_dma_in_ycurve_lut_addr_buf_reg(UINT32 addr);

extern void ipe_eng_set_dma_in_offset_buf_reg(UINT32 ofsi_y, UINT32 ofsi_c);

//extern void ipe_eng_set_dma_in_defog_offset_buf_reg(UINT32 ofsi);
extern void ipe_eng_set_dma_in_lce_offset_buf_reg(UINT32 ofsi);


extern void ipe_eng_set_dma_in_rand_buf_reg(UINT32 en, UINT32 reset);

extern void ipe_eng_set_dma_out_addr_y_buf_reg(UINT32 addr_y);

extern void ipe_eng_set_dma_out_addr_c_buf_reg(UINT32 addr_c);

extern void ipe_eng_set_dma_out_offset_y_buf_reg(UINT32 ofso_y);

extern void ipe_eng_set_dma_out_offset_c_buf_reg(UINT32 ofso_c);

extern void ipe_eng_set_interrupt_enable_buf_reg(UINT32 int_en);

extern void ipe_eng_set_tone_remap_buf_reg(UINT16 *p_tonemap_lut);

extern void ipe_eng_set_edge_extract_sel_buf_reg(UINT32 gam_sel, UINT32 ch_sel);

extern void ipe_eng_set_edge_extract_kernel_b_buf_reg(INT16 *p_eext_ker, UINT32 eext_div_b, UINT8 eext_enh_b);

extern void ipe_eng_set_eext_kerstrength_buf_reg(IPE_EEXT_KER_STRENGTH *p_ker_str);

extern void ipe_eng_set_eext_engcon_buf_reg(IPE_EEXT_ENG_CON *p_eext_engcon);

extern void ipe_eng_set_ker_thickness_buf_reg(IPE_KER_THICKNESS *p_ker_thickness);

extern void ipe_eng_set_ker_thickness_hld_buf_reg(IPE_KER_THICKNESS *p_ker_thickness_hld);

extern void ipe_eng_set_region_buf_reg(IPE_REGION_PARAM *p_region);

extern void ipe_eng_set_region_strength_ctrl_buf_reg(IPE_REGION_STRENGTH_PARAM *p_region_str);

extern void ipe_eng_set_region_slope_buf_reg(UINT16 slope, UINT16 slope_hld);

extern void ipe_eng_set_overshoot_buf_reg(IPE_OVERSHOOT_PARAM *p_overshoot);

extern void ipe_get_edge_stcs_buf_reg(IPE_EDGE_STCS_RSLT *stcs_result);

extern void ipe_eng_set_esmap_thresholds_buf_reg(IPE_ESMAP_INFOR *p_esmap);

extern void ipe_eng_set_estab_buf_reg(UINT8 *p_estab);

extern void ipe_eng_set_edgemap_thresholds_buf_reg(IPE_EDGEMAP_INFOR *p_edmap);

extern void ipe_eng_set_edgemap_tab_buf_reg(UINT8 *p_edtab);

//extern void ipe_eng_set_rgblpf_buf_reg(IPE_RGBLPF_PARAM *p_lpf_info);

//extern void ipe_eng_set_pfr_general_buf_reg(IPE_PFR_PARAM *p_pfr_info);

//extern void ipe_eng_set_pfr_color_buf_reg(UINT32 idx, IPE_PFR_COLOR_WET *p_pfr_color_info);

//extern void ipe_eng_set_pfr_luma_buf_reg(UINT32 luma_th, UINT32 *luma_table);

extern void ipe_eng_set_color_correct_buf_reg(IPE_CC_PARAM *p_cc_param);

extern void ipe_eng_set_fstab_buf_reg(UINT8 *p_fstab_lut);

extern void ipe_eng_set_fdtab_buf_reg(UINT8 *p_fdtab_lut);

extern void ipe_eng_set_cst_buf_reg(IPE_CST_PARAM *p_cst);

extern void ipe_eng_set_int_sat_offset_buf_reg(INT16 int_ofs, INT16 sat_ofs);

extern void ipe_eng_set_hue_rotate_enable_buf_reg(BOOL hue_rot_en);

extern void ipe_eng_set_hue_c2g_enable_buf_reg(BOOL c2g_en);

extern void ipe_eng_set_color_suppress_buf_reg(IPE_CCTRL_SEL_ENUM cctrl_sel, UINT8 vdetdiv);

extern void ipe_eng_set_cctrl_hue_buf_reg(UINT8 *p_hue_tbl);

extern void ipe_eng_set_cctrl_int_buf_reg(INT8 *p_int_tbl);

extern void ipe_eng_set_cctrl_sat_buf_reg(INT8 *p_sat_tbl);

extern void ipe_eng_set_cctrl_edge_buf_reg(UINT8 *p_edge_tbl);

extern void ipe_eng_set_cctrl_dds_tab_buf_reg(UINT8 *p_dds_tbl);

extern void ipe_eng_set_edge_enhance_buf_reg(UINT32 y_enh_p, UINT32 y_enh_n);

extern void ipe_eng_set_edge_invert_buf_reg(BOOL b_einv_p, BOOL b_einv_n);

extern void ipe_eng_set_y_contrast_buf_reg(UINT32 con_y);
extern void ipe_eng_set_yc_rand_buf_reg(IPE_CADJ_RAND_PARAM *p_ycrand);

extern void ipe_eng_set_yfix_th_buf_reg(STR_YTH1_INFOR *p_yth1, STR_YTH2_INFOR *p_yth2);

extern void ipe_eng_set_yc_mask_buf_reg(UINT8 mask_y, UINT8 mask_cb, UINT8 mask_cr);

extern void ipe_eng_set_cbcr_offset_buf_reg(UINT32 cb_ofs, UINT32 cr_ofs);

extern void ipe_eng_set_cbcr_contrast_buf_reg(UINT32 con_c);

extern void ipe_eng_set_cbcr_contab_sel_buf_reg(IPE_CCONTAB_SEL ccontab_sel);

extern void ipe_eng_set_cbcr_contab_buf_reg(UINT16 *p_ccontab);

extern void ipe_eng_set_cbcr_fixth_buf_reg(STR_CTH_INFOR *p_cth);

extern void ipe_eng_set_cstp_buf_reg(UINT32 ratio);

extern void ipe_eng_set_ycurve_sel_buf_reg(IPE_YCURVE_SEL ycurve_sel);

extern void ipe_eng_set_gam_y_rand_buf_reg(IPE_GAMYRAND *p_gamy_rand);

//extern void ipe_eng_set_defog_rand_buf_reg(IPE_DEFOGROUND *p_defog_rnd);

extern void ipe_eng_set_edge_dbg_sel(IPE_EDGEDBG_SEL mode_sel);

extern void ipe_eng_set_eth_size_buf_reg(IPE_ETH_SIZE *p_eth);

extern void ipe_eng_set_eth_param_buf_reg(IPE_ETH_PARAM *p_eth);

extern void ipe_eng_set_dma_out_va_addr_buf_reg(UINT32 addr);

extern void ipe_eng_set_dma_out_va_offset_buf_reg(UINT32 ofso);

extern void ipe_eng_set_va_out_sel_buf_reg(BOOL en);

extern void ipe_eng_set_va_filter_g1_buf_reg(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1);

extern void ipe_eng_set_va_filter_g2_buf_reg(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2);

extern void ipe_eng_set_va_mode_enable_buf_reg(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1, IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2);

extern void ipe_eng_set_va_win_info_buf_reg(IPE_VA_WIN_PARAM *p_va_win);

extern void ipe_eng_set_va_indep_win_buf_reg(IPE_INDEP_VA_PARAM  *p_indep_va_win_info, UINT32 win_idx);

extern ER ipe_eng_check_va_lofs(IPE_VA_WIN_PARAM *p_va_win, IPE_OUTYCINFO *p_out_info);

extern ER ipe_eng_check_va_win_info(IPE_VA_WIN_PARAM *p_va_win, IPE_IMG_SIZE size);

//extern void ipe_eng_set_dma_out_defog_addr_buf_reg(UINT32 sao);
extern void ipe_eng_set_dma_out_lce_addr_buf_reg(UINT32 sao);


//extern void ipe_eng_set_dma_out_defog_offset_buf_reg(UINT32 ofso);
extern void ipe_eng_set_dma_out_lce_offset_buf_reg(UINT32 ofso);


//extern void ipe_eng_set_dfg_subimg_size_buf_reg(IPE_IMG_SIZE subimg_size);
extern void ipe_eng_set_lce_subimg_size_buf_reg(IPE_IMG_SIZE subimg_size);


//extern void ipe_eng_set_dfg_subout_param_buf_reg(UINT32 blk_sizeh, UINT32 blk_cent_h_fact, UINT32 blk_sizev, UINT32 blk_cent_v_fact);
extern void ipe_eng_set_lce_subout_param_buf_reg(UINT32 blk_sizeh, UINT32 blk_cent_h_fact, UINT32 blk_sizev, UINT32 blk_cent_v_fact);


//extern void ipe_eng_set_dfg_input_bld_buf_reg(UINT8 *input_bld_wt);

//extern void ipe_eng_set_dfg_scal_factor_buf_reg(UINT32 scalfact_h, UINT32 scalfact_v);
extern void ipe_eng_set_lce_scal_factor_buf_reg(UINT32 scalfact_h, UINT32 scalfact_v);


//extern void ipe_eng_set_dfg_scal_filtcoeff_buf_reg(UINT8 *p_filtcoef);
extern void ipe_eng_set_lce_scal_filtcoeff_buf_reg(UINT8 *p_filtcoef);


//extern void ipe_eng_set_dfg_scalg_edgeinterplut_buf_reg(UINT8 *p_interp_diff_lut);


//extern void ipe_eng_set_defog_scal_edgeinterp_buf_reg(UINT8 wdist, UINT8 wout, UINT8 wcenter, UINT8 wsrc);

//extern void ipe_eng_set_defog_atmospherelight_buf_reg(UINT16 val0, UINT16 val1, UINT16 val2);

//extern void ipe_eng_set_defog_fog_protect_buf_reg(BOOL self_cmp_en, UINT16 min_diff);

//extern void ipe_eng_set_defog_fog_mod_buf_reg(UINT16 *p_fog_mod_lut);

//extern void ipe_eng_set_defog_fog_target_lut_buf_reg(UINT16 *p_fog_target_lut);

//extern void ipe_eng_set_defog_strength_buf_reg(IPE_DEFOG_METHOD_SEL modesel, UINT8 fog_ratio, UINT8 dgain, UINT8 gain_th);

//extern void ipe_eng_set_defog_outbld_lum_lut_buf_reg(BOOL lum_bld_ref, UINT8 *p_outbld_lum_lut);

//extern void ipe_eng_set_defog_outbld_diff_lut_buf_reg(UINT8 *p_outbld_diff_lut);

//extern void ipe_eng_set_defog_outbld_local_buf_reg(UINT32 local_en, UINT32 air_nfactor);

//extern void ipe_eng_set_defog_stcs_pcnt_buf_reg(UINT32 pixelcnt);

extern void ipe_eng_set_lce_diff_param_buf_reg(UINT8 wt_avg, UINT8 wt_pos, UINT8 wt_neg);

extern void ipe_eng_set_lce_lum_lut_buf_reg(UINT8 *p_lce_lum_lut);

//extern void ipe_eng_set_rgb_lpf_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_gamma_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_ycurve_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_color_correction_enable_buf_reg(UINT32 set_en);

//extern void ipe_eng_set_defog_subout_enable_buf_reg(UINT32 set_en);
extern void ipe_eng_set_lce_subout_enable_buf_reg(UINT32 set_en);

//extern void ipe_eng_set_defog_enable_buf_reg(UINT32 set_en);
extern void ipe_eng_set_lce_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cst_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_color_ctrl_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_hue_adj_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cadj_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cadj_y_edge_enh_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cadj_y_cb_ctrl_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cadj_uv_cb_ctrl_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cadj_y_uv_th_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_cst_prot_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_edge_dbg_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_vacc_enable_buf_reg(UINT32 set_en);

extern void ipe_eng_set_vacc_win_enable_buf_reg(UINT32 set0_en, UINT32 set1_en, UINT32 set2_en, UINT32 set3_en, UINT32 set4_en);

//extern void ipe_eng_set_pfr_enable_buf_reg(UINT32 set_en);

#if (defined (_NVT_EMULATION_) == ON)
//extern void ipe_update_opmode(IPE_OPMODE ipe_opmode_set);
//extern ER ipe_set_mode_fix_state(VOID);
extern BOOL ipe_end_time_out_status;
#endif



#ifdef __cplusplus
}
#endif

#endif


