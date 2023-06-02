#ifndef __NUE2_SYS_VFY_h_
#define __NUE2_SYS_VFY_h_

#include "nue2_dbg.h"
#include "nue2_lib.h"

/**
    @name Define NUE2 function pointer
*/
//@{

#define NUE2_FPGA ENABLE

#if (NUE2_EMU_DBG_EN == DISABLE)
#define NUE2_DBG_STRIPE DISABLE
#else
#define NUE2_DBG_STRIPE ENABLE
#endif

#define NUE2_CHECK_REG_RW_BY_OFS(a, b, c) do { \
    *(reg_data + ((a)/sizeof(UINT32))) = (b);\
    nvt_dbg(ERR, "NUE2: 0x%x: 0x%x\r\n", (a), (UINT32) *(reg_data + ((a)/sizeof(UINT32))));\
    *(reg_data + ((a)/sizeof(UINT32))) = (c);\
    nvt_dbg(ERR, "NUE2: 0x%x: 0x%x\r\n", (a), (UINT32) *(reg_data + ((a)/sizeof(UINT32))));\
} while(0)\

#if defined(__FREERTOS)
#define NUE2_REG_DUMP(a)  do{nvt_dbg(ERR, "NUE2: 0x%08lx: 0x%08lx\r\n", (a), (UINT32) *(reg_data + ((a)/sizeof(UINT32))));} while(0)
#else
#define NUE2_REG_DUMP(a)  do{nvt_dbg(ERR, "NUE2: 0x%08x: 0x%08x\r\n", (a), (UINT32) *(reg_data + ((a)/sizeof(UINT32))));} while(0)
#endif

#define NUE2_REG_SET(a, b)  do{*(reg_data + ((a)/sizeof(UINT32))) = b;}while(0)

#define NUE2_DEBUG_LL DISABLE
#define NUE2_DEBUG_STRIP DISABLE

#define NUE2_STRIP_BEGIN  1
#define NUE2_STRIP_END    -1
#define NUE2_STRIP_NORM   0

typedef ER (* nue2_start_t)(VOID);
typedef ER (* nue2_ll_start_t)(VOID);
typedef void (* nue2_wait_frameend_t)(BOOL is_clr_flag);
typedef void (* nue2_wait_ll_frameend_t)(BOOL is_clr_flag);
typedef void (* nue2_engine_loop_frameend_t)(VOID *p_parm);
typedef void (* nue2_engine_loop_llend_t)(VOID *p_parm);
typedef void (* nue2_engine_debug_hook_t)(VOID *parm);
typedef void (* nue2_engine_debug_hook1_t)(VOID *parm);
typedef void (* nue2_engine_debug_hook2_t)(VOID *parm);
//@}

/*
    NUE2 hsv mode
*/
typedef enum {
    NUE2_NO_HSV     = 0,
    NUE2_HSV_8BIT   = 1,
    NUE2_HSV_9BIT   = 2,
    ENUM_DUMMY4WORD(NUE2_HSV_MODE)
} NUE2_HSV_MODE;

/**
    NUE2 sub mode

    Select NUE2 sub mode
*/
typedef enum {
    NUE2_DC_MODE                    = 0,    ///< DC mode for sub
    NUE2_PLANER_MODE                = 1,    ///< Planer mode for sub
    ENUM_DUMMY4WORD(NUE2_SUB_MODE)
} NUE2_SUB_MODE;

/**
    NUE2 DEBUG BURST

    Structure of information in NUE2 DEBUG burst
*/
//@{
typedef struct _NUE2_DEBUG_PARM {
    UINT8 in_burst_mode;
    UINT8 out_burst_mode;
} NUE2_DEBUG_PARM;
//@}

/**
    NUE2 AXI

    Structure of information in NUE2 AXI
*/
//@{
typedef struct _NUE2_AXI_PARM {
    UINT16 r_ostd_num;
    UINT16 w_ostd_num;
} NUE2_AXI_PARM;
//@}

/**
    NUE2 flow control

    Structure of information in NUE2 flow control
*/
//@{
typedef struct _NUE2_FLOW_CT_PARM {
    BOOL rand_ch_en;
    BOOL interrupt_en;
    UINT32 s_num;
    UINT32 rst_en;
    UINT64 *ll_buf;
	UINT64 *ll_buf_end;
    UINT8 is_terminate;
    UINT8 is_dma_test;
    UINT8 is_fill_reg_only;
    UINT8 is_ll_next_update;
	UINT32 cnt_is_hw_only;
	UINT32 cnt_hw_ll;
    UINT32 cnt_hw_no_ll;
    UINT32 cnt_single_hw;
	UINT8 is_reg_dump;
	UINT8 ll_test_2;
	UINT8 is_dump_ll_buf;
	UINT8 ll_big_buf;
	UINT8 auto_clk;
	UINT8 is_bit60;
	UINT32 ll_base_addr;
	UINT32 ll_fill_reg_num;
	UINT32 ll_fill_num;
	UINT8 is_switch_dram;
	UINT8 clk_en;
	UINT8 sram_down;
	UINT32 loop_time;
	UINT32 loop_mode;
} NUE2_FLOW_CT_PARM;

/**
    NUE2 Stripe parameter

    Parameter of NUE2 stripe
*/
//@{
typedef struct _NUE2_STRIP_PARM_STRUCT {
    UINT32 is_strip;
    UINT32 in_width;
    UINT32 in_height;
    UINT32 flip_mode;
    UINT8 sub_en;
    UINT8 sub_mode;
    UINT8 sub_planer_en;
    UINT32 s_num;
    UINT8 sub_dup_mode;
    UINT32 sub_in_width;
    UINT8 hsv_en;
    UINT8 hsv_out_mode;
    UINT8 hsv_en_mode;
    UINT8 in_fmt;
    UINT8 yuv_mode;
    UINT32 scl_width;
    UINT32 scl_height;
    UINT32 s_step;
    UINT32 s_posi;
} NUE2_STRIP_PARM;
//@}

/**
    NUE2 function register

    Register of function in NUE2 flow control
*/
//@{
typedef struct _NUE2_REG_FUNC_PARM {
    nue2_start_t nue2_start;
    nue2_ll_start_t nue2_ll_start;
    nue2_wait_frameend_t nue2_wait_frameend;
    nue2_wait_ll_frameend_t nue2_wait_ll_frameend;
    nue2_engine_loop_frameend_t nue2_engine_loop_frameend;
    nue2_engine_loop_llend_t nue2_engine_loop_llend;
	nue2_engine_debug_hook_t nue2_engine_debug_hook;
	nue2_engine_debug_hook1_t nue2_engine_debug_hook1;
	nue2_engine_debug_hook2_t nue2_engine_debug_hook2;
} NUE2_REG_FUNC_PARM;
//@}

/**
    NUE2 Entire Configuration

    Structure of NUE2 module parameters when one wants to configure this module.
*/
//@{
typedef struct _NUE2_TEST_PARM_STRUCT {
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
    NUE2_DEBUG_PARM dbg_parm;
    NUE2_AXI_PARM axi_parm;
	NUE2_FLOW_CT_PARM flow_ct;
    NUE2_STRIP_PARM strip_parm;
    NUE2_REG_FUNC_PARM reg_func;
} NUE2_TEST_PARM;
//@}

#endif // #ifndef __NUE2_SYS_VFY_h_

