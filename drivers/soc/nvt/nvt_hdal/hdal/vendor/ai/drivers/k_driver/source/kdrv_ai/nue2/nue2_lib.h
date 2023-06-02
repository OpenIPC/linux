#ifndef _NUE2_LIB_H
#define _NUE2_LIB_H

#include "kdrv_ai.h"
#include "kwrap/type.h"

/**
    Define NUE2 interrput enable.
*/
//@{
#define NUE2_INTE_FRMEND                 0x00000001
#define NUE2_INTE_DMAIN0END              0x00000002
#define NUE2_INTE_DMAIN1END              0x00000004
#define NUE2_INTE_DMAIN2END              0x00000008
#define NUE2_INTE_LLEND                  0x00000100
#define NUE2_INTE_LLERR                  0x00000200
#define NUE2_INTE_LLJOBEND               0x00000400  //CHIP:528
#define NUE2_INTE_SWRESET                0x00010000
#define NUE2_INTE_CHECKSUM_MISMATCH0     0x00020000  //CHIP:560
#define NUE2_INTE_CHECKSUM_MISMATCH1     0x00040000  //CHIP:560
#define NUE2_INTE_CHECKSUM_MISMATCH2     0x00080000  //CHIP:560
#define NUE2_INTE_ALL                    0x00000301
#define NUE2_INTE_ALL_528                0x00000301
#define NUE2_INTE_ALL_560                0x000F0301  //CHIP:560
//@}

/**
    Define NUE2 interrput status.
*/
//@{
#define NUE2_INT_FRMEND                 0x00000001
#define NUE2_INT_DMAIN0END              0x00000002
#define NUE2_INT_DMAIN1END              0x00000004
#define NUE2_INT_DMAIN2END              0x00000008
#define NUE2_INT_LLEND                  0x00000100
#define NUE2_INT_LLERR                  0x00000200
#define NUE2_INT_LLJOBEND               0x00000400  //CHIP:528
#define NUE2_INT_SWRESET                0x00010000
#define NUE2_INT_CHECKSUM_MISMATCH0     0x00020000  //CHIP:560
#define NUE2_INT_CHECKSUM_MISMATCH1     0x00040000  //CHIP:560
#define NUE2_INT_CHECKSUM_MISMATCH2     0x00080000  //CHIP:560
#define NUE2_INT_ALL                    0x0001030f
#define NUE2_INT_ALL_528                0x0001070f
#define NUE2_INT_ALL_560                0x000F070f  //CHIP:560
//@}

/**
    @name Define NUE2 function enable.
*/
//@{
#define NUE2_FUNC_YUV2RGB_EN              0x00000001  ///< Convert from YUV to RGB function enable
#define NUE2_FUNC_SUB_EN                  0x00000002  ///< Mean Subtraction function enable
#define NUE2_FUNC_PAD_EN                  0x00000004  ///< Padding interlace enable
#define NUE2_FUNC_HSV_EN                  0x00000008  ///< Convert from YUV to HSV function enable
#define NUE2_FUNC_ROTATE_EN               0x00000010  ///< Rotate function enable
#define NUE2_FUNC_ALL_EN                  0x0000001f  ///< all function enable
//@}

/**
    @name Define NUE2 checksum number.
*/
//@{
#define NUE2_CHKSUM_NUM_MAX				3 // CHIP:560
//@}

#define NUE2_CYCLE_TEST ENABLE
#define NUE2_CYCLE_LAYER DISABLE
/**
    NUE2 mode selection

    Select NUE2 mode
*/
typedef enum {
	NUE2_OPMODE_1              = 0,    ///< Operation of Mean Subtraction
	NUE2_OPMODE_2              = 1,    ///< Operation of HSV
	NUE2_OPMODE_3              = 2,    ///< Operation of Rotation
	NUE2_OPMODE_USERDEFINE     = 3,    ///< all manual mode
	NUE2_OPMODE_USERDEFINE_NO_INT = 4, ///< all manual mode without interrupt
	ENUM_DUMMY4WORD(NUE2_OPMODE)
} NUE2_OPMODE;


/**
    NUE2 In Format
*/
//@{
typedef enum {
	NUE2_YUV_420        = 0,
	NUE2_Y_CHANNEL      = 1,
	NUE2_UV_CHANNEL     = 2,
	NUE2_RGB_PLANNER    = 3,
	ENUM_DUMMY4WORD(NUE2_IN_FMT)
} NUE2_IN_FMT;
//@}

/**
    NUE2 Subtraction Duplicate Rate
*/
//@{
typedef enum {
	NUE2_SUBDUP_RATE_1x        = 0,
	NUE2_SUBDUP_RATE_2x        = 1,
	NUE2_SUBDUP_RATE_4x        = 2,
	NUE2_SUBDUP_RATE_8x        = 3,
	ENUM_DUMMY4WORD(NUE2_SUBDUP_RATE)
} NUE2_SUBDUP_RATE;
//@}

/**
    NUE2 Rotate Degree
*/
//@{
typedef enum {
	NUE2_ROT_P90        = 0,
	NUE2_ROT_N90        = 1,
	NUE2_ROT_P180       = 2,
	ENUM_DUMMY4WORD(NUE2_ROT_DEG)
} NUE2_ROT_DEG;
//@}

/**
    NUE2 Input Buffer ID
*/
//@{
typedef enum {
	NUE2_IN0_BUF     = 0,
	NUE2_IN1_BUF     = 1,
	NUE2_IN2_BUF     = 2,
	ENUM_DUMMY4WORD(NUE2_IN_BUFID)
} NUE2_IN_BUFID;
//@}

/**
    NUE2 Output Buffer ID
*/
//@{
typedef enum {
	NUE2_OUT0_BUF     = 0,
	NUE2_OUT1_BUF     = 1,
	NUE2_OUT2_BUF     = 2,
	ENUM_DUMMY4WORD(NUE2_OUT_BUFID)
} NUE2_OUT_BUFID;
//@}

/**
    NUE2 Open Object.

    Open Object is used for opening NUE2 module.
    \n Used for NUE2_open()

*/
//@{
typedef struct _NUE2_OPENOBJ {
	UINT32 clk_sel;                             ///< support 220/192/160/120/80/60/48 Mhz
	VOID (*fp_isr_cb)(UINT32 int_status);       ///< isr callback function
} NUE2_OPENOBJ;
//@}

/**
    NUE2 output setting

    Structure of settings of output function
*/
//@{
typedef struct _NUE2_FUNC_EN {
	BOOL yuv2rgb_en;      ///< Convert from YUV to RGB function enable
	BOOL sub_en;          ///< Mean Subtraction function enable
	BOOL pad_en;          ///< Padding interlace enable
	BOOL hsv_en;          ///< Convert from YUV to HSV function enable
	BOOL rotate_en;       ///< Rotate function enable
} NUE2_FUNC_EN;
//@}

/**
    NUE2 Out Format
*/
//@{
typedef struct _NUE2_OUT_FMT {
	BOOL out_signedness;
} NUE2_OUT_FMT;
//@}

/**
    NUE2 Input Size
*/
//@{
typedef struct _NUE2_IN_SIZE {
	UINT32 in_width;
    UINT32 in_height;
} NUE2_IN_SIZE;
//@}

/**
    NUE2 DRAM input and output lineoffset.

    Structure of information in DRAM lineoffset.
*/
//@{
typedef struct _NUE2_DMAIO_LOFS {
	UINT32 in0_lofs;                 ///< input lineoffset of input data 0
	UINT32 in1_lofs;                 ///< input lineoffset of input data 1
	UINT32 in2_lofs;                 ///< input lineoffset of input data 2
	UINT32 out0_lofs;                ///< output lineoffset of output data 0
	UINT32 out1_lofs;                ///< output lineoffset of output data 1
	UINT32 out2_lofs;                ///< output lineoffset of output data 2
} NUE2_DMAIO_LOFS;
//@}

/**
    NUE2 DRAM input and output addresses.

    Structure of information in DRAM addresses.
*/
//@{
typedef struct _NUE2_LL_PRM {
        UINT32 ll_addr;             ///< LL address
        UINT32 ll_base_addr;        ///< LL base address
} NUE2_LL_PRM;
//@}

/**
    NUE2 DRAM input and output addresses.

    Structure of information in DRAM addresses.
*/
//@{
typedef struct _NUE2_DMAIO_ADDR {
	UINT32 addr_in0;                 ///< input starting address of input data 0
	UINT32 addr_in1;                 ///< input starting address of input data 1
	UINT32 addr_in2;                 ///< input starting address of input data 2
	UINT32 addr_out0;                ///< starting address output 0
	UINT32 addr_out1;                ///< starting address output 1
	UINT32 addr_out2;                ///< starting address output 2
	UINT8  dma_do_not_sync;          ///< 1: don't sync data 0: force to sync data
	UINT8  is_pa;                    ///< 1: physical address 0: virtual address
} NUE2_DMAIO_ADDR;
//@}

/**
    NUE2 FLIP Mode Parameters. CHIP:528

    Structure of information in NUE2 FLIP Mode parameters.
*/
//@{
typedef struct _NUE2_FLIP_PARM {
    UINT32 flip_mode;
} NUE2_FLIP_PARM;
//@}

/**
    NUE2 Subtraction Parameters.

    Structure of information in NUE2 Subtraction parameters.
*/
//@{
typedef struct _NUE2_SUB_PARM {
    BOOL sub_mode;
	UINT32 sub_in_width;
    UINT32 sub_in_height;
    UINT32 sub_coef0;
    UINT32 sub_coef1;
    UINT32 sub_coef2;
    NUE2_SUBDUP_RATE sub_dup;
    UINT32 sub_shift;
} NUE2_SUB_PARM;
//@}

/**
    NUE2 Mean Shift.

    Structure of information in NUE2 Mean Shift parameters.
*/
//@{
typedef struct _NUE2_MEAN_SHIFT_PARM {
	BOOL mean_shift_dir;
	UINT8 mean_shift;
	UINT32 mean_scale;
} NUE2_MEAN_SHIFT_PARM;
//@}


/**
    NUE2 Padding Parameters.

    Structure of information in NUE2 Padding parameters.
*/
//@{
typedef struct _NUE2_PAD_PARM {
	UINT32 pad_crop_x;
    UINT32 pad_crop_y;
    UINT32 pad_crop_width;
    UINT32 pad_crop_height;
    UINT32 pad_crop_out_x;
    UINT32 pad_crop_out_y;
    UINT32 pad_crop_out_width;
    UINT32 pad_crop_out_height;
    UINT32 pad_val0;
    UINT32 pad_val1;
    UINT32 pad_val2;
} NUE2_PAD_PARM;
//@}

/**
    NUE2 HSV Parameters.

    Structure of information in NUE2 HSV parameters.
*/
//@{
typedef struct _NUE2_HSV_PARM {
    BOOL hsv_out_mode;  // 0:8-bit, 1:9-bit
    UINT32 hsv_hue_shift;
} NUE2_HSV_PARM;
//@}

/**
    NUE2 Rotate Parameters.

    Structure of information in NUE2 Rotate parameters.
*/
//@{
typedef struct _NUE2_ROTATE_PARM {
    NUE2_ROT_DEG rotate_mode;
} NUE2_ROTATE_PARM;
//@}

/**
    NUE2 Scaling Parameters.

    Structure of information in NUE2 Scaling parameters.
*/
//@{
typedef struct _NUE2_SCALE_PARM {
    BOOL fact_update;
    BOOL h_filtmode;
    BOOL v_filtmode;
    UINT32 h_filtcoef;
    UINT32 v_filtcoef;
    UINT32 h_scl_size;
    UINT32 v_scl_size;
    UINT32 h_dnrate;
    UINT32 v_dnrate;
    UINT32 h_sfact;
    UINT32 v_sfact;
    UINT32 ini_h_dnrate;
    UINT32 ini_h_sfact;
    UINT32 final_h_dnrate;
    UINT32 final_h_sfact;
	UINT8  scale_h_mode;
	UINT8  scale_v_mode;
} NUE2_SCALE_PARM;
//@}

/**
    NUE2 checksum status Parameters.

    Structure of information in NUE2 checksum status parameters.
*/
typedef struct _NUE2_CHKSUM_ERR_STS_PARM { // CHIP:560
	BOOL is_chksum_err[NUE2_CHKSUM_NUM_MAX];
} NUE2_CHKSUM_ERR_STS_PARM;

/**
    NUE2 checksum Parameters.

    Structure of information in NUE2 checksum parameters.
*/
typedef struct _NUE2_CHKSUM_PARM { // CHIP:560
	UINT32 chksum[NUE2_CHKSUM_NUM_MAX];
} NUE2_CHKSUM_PARM;

/**
    NUE2 Entire Configuration

    Structure of NUE2 module parameters when one wants to configure this module.
*/
//@{
typedef struct _NUE2_PARM_STRUCT {
    NUE2_FUNC_EN func_en;
	NUE2_IN_FMT infmt;
	NUE2_OUT_FMT outfmt;
	NUE2_IN_SIZE insize;                    ///< input size
	NUE2_DMAIO_LOFS dmaio_lofs;             ///< DRAM input and output lineoffset
	NUE2_DMAIO_ADDR dmaio_addr;             ///< DRAM input and output addresses
	NUE2_FLIP_PARM flip_parm;
    NUE2_SCALE_PARM scale_parm;
	NUE2_SUB_PARM sub_parm;
	NUE2_PAD_PARM pad_parm;
	NUE2_HSV_PARM hsv_parm;
	NUE2_ROTATE_PARM rotate_parm;
	NUE2_MEAN_SHIFT_PARM mean_shift_parm;   ///< scale shift in 528 now
} NUE2_PARM;
//@}


//interrupt
extern ER       nue2_change_interrupt(UINT32 int_en);
extern UINT32   nue2_get_int_enable(VOID);
extern VOID     nue2_clr_intr_status(UINT32 status);
extern UINT32   nue2_get_intr_status(VOID);
//basic function
extern ER       nue2_open(NUE2_OPENOBJ *p_obj_cb);
extern BOOL     nue2_is_opened(VOID);
extern ER       nue2_close(VOID);
extern VOID     nue2_wait_frameend(BOOL is_clr_flag);
extern BOOL     nue2_isenable(VOID);
extern VOID     nue2_set_dmain_lladdr(UINT32 addr0);
extern UINT32   nue2_get_dmain_lladdr(VOID);
extern ER       nue2_ll_pause(VOID);                        
extern ER       nue2_ll_start(VOID);                        
extern VOID     nue2_wait_ll_frameend(BOOL is_clr_flag);    
extern ER       nue2_ll_setmode(NUE2_LL_PRM *ll_parm);
extern ER       nue2_ll_isr_start(VOID);
extern VOID 	nue2_create_resource(VOID *parm, UINT32 clk_freq);
extern VOID 	nue2_release_resource(VOID);                           
extern VOID 	nue2_set_base_addr(UINT32 addr);
extern UINT32 	nue2_get_base_addr(VOID);
//Engine info
extern NUE2_OPMODE 		nue2_get_drvmode(VOID);
extern UINT32 			nue2_get_int_enable(VOID);
extern UINT32 			nue2_get_intr_status(VOID);
extern NUE2_DMAIO_LOFS 	nue2_get_dmaio_lofs(VOID);
extern NUE2_DMAIO_ADDR 	nue2_get_dmaio_addr(VOID);
//Function info
extern NUE2_IN_FMT 		nue2_get_infmt(VOID);
extern NUE2_FUNC_EN 	nue2_get_func_en(VOID);
extern NUE2_OUT_FMT 	nue2_get_outfmt(VOID);
extern NUE2_IN_SIZE 	nue2_get_insize(VOID);
extern NUE2_SCALE_PARM 	nue2_get_scale_parm(VOID);
extern NUE2_SUB_PARM 	nue2_get_sub_parm(VOID);
extern NUE2_PAD_PARM 	nue2_get_pad_parm(VOID);
extern NUE2_HSV_PARM 	nue2_get_hsv_parm(VOID);
extern NUE2_ROTATE_PARM nue2_get_rotate_parm(VOID);
extern UINT32 			nue2_init(VOID);
extern UINT32 			nue2_uninit(VOID);
#if NUE2_CYCLE_TEST
extern UINT64 			nue2_dump_total_cycle(UINT32 debug_layer);
#endif
extern VOID 			nue2_loop_frameend(VOID);
extern VOID 			nue2_reset_status(VOID);
extern VOID     		nue2_reset(VOID);
extern ER       		nue2_pause(VOID);
extern VOID 			nue2_wait_sw_reset(BOOL is_clr_flag);
extern VOID 			nue2_dma_abort(VOID);
extern VOID 			nue2_dma_abort_exit(VOID);

#if defined(_BSP_NA51089_)
VOID nue2_lib_set_chksum_en(BOOL is_en);
NUE2_CHKSUM_ERR_STS_PARM nue2_lib_get_output_chksum_err_sts_parm(VOID);
VOID nue2_lib_clr_output_chksum_err_sts_parm(VOID);
NUE2_CHKSUM_PARM nue2_lib_get_output_chksum_parm(VOID);
VOID nue2_lib_fill_output_chksum_parm(NUE2_CHKSUM_PARM chksum_parm);
#endif

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
extern ER       		nue2_isr_start(VOID);
extern ER 				nue2_start(void);
#endif

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
extern ER				nue2_setmode(NUE2_OPMODE mode, NUE2_PARM *p_parm);
#endif

#endif
