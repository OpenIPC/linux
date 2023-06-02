#ifndef _EMU_H26X_JOB_H_
#define _EMU_H26X_JOB_H_

#include "kwrap/type.h"

#include "emu_h26x_mem.h"
// for sw driver //
#include "h26x_def.h"
#include "h26x.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define DIRECT_MODE	(1)	// 0: link-list mode, 1 : direct mode //

#define H26X_JOB_MAX	(1)

typedef enum{
	Start = 0,
	WR,
	RD,
	RD_select,
	Sub_cmd,
	Finish = 15
}LINKLIST_CMD;

typedef struct _h26x_ver_item_t_{
	unsigned int rand_seed;
	unsigned int rec_out_dis;
	unsigned int src_out_en;
	unsigned int rnd_sw_rest;
	unsigned int rnd_slc_hdr;
	unsigned int rnd_bs_buf;
	unsigned int rnd_bs_buf_32b;
    unsigned int stable_bs_len;
	unsigned int rotate_en;
	unsigned int src_cbcr_iv;
	unsigned int cmp_bs_en;
	unsigned int cmp_colmv_en;
	unsigned int rnd_sram_en;
	unsigned int bw_heavy;
	unsigned int ddr_brst_en;
	unsigned int dram_range;
	unsigned int dram2;
	unsigned int write_prot;
	unsigned int gating;
	unsigned int clksel;
	unsigned int lofs;
	unsigned int job_order;
	unsigned int src_out_only;
}h26x_ver_item_t;

typedef struct _h26x_job_t_{
	unsigned int idx1;         // for mallocate order
    unsigned int idx2;         // for hw linklist order
	unsigned int codec;			// 0 : 265 enc,  1 : 264 enc, 2 : 265 dec , 3 : 264 dec

	unsigned int apb_addr;
	unsigned int ctx_addr;
	unsigned int llc_addr;		// for debug only //

	unsigned int check1_addr;
	unsigned int check2_addr;


	unsigned int start_folder_idx;
	unsigned int end_folder_idx;
	unsigned int start_pat_idx;
	unsigned int end_pat_idx;
	unsigned int end_frm_num;
	unsigned int src_out_only_en;

	unsigned int is_finish;

	h26x_mem_t mem;
	h26x_mem_t mem_bak;

	// for swrest //
	UINT32 tmp_rec_y_addr;
	UINT32 tmp_rec_c_addr;
	UINT32 tmp_colmv_addr;
	UINT32 tmp_sideinfo_addr;
    UINT32 tmp_sideinfo_addr2[H26X_MAX_TILE_NUM-1];
	UINT32 tmp_rec_y_addr2[H26X_MAX_TILE_NUM-1];
	UINT32 tmp_rec_c_addr2[H26X_MAX_TILE_NUM-1];
	UINT32 tmp_bs_addr;
	UINT32 tmp_bs_len;
	UINT32 tmp_tmnr_mtout_addr;
	UINT32 tmp_tmnr_rec_y_addr;
	UINT32 tmp_tmnr_rec_c_addr;
	UINT32 tmp_osg_gcac_addr0;
	UINT32 tmp_osg_gcac_addr1;
	// for sw compare//
	UINT32 picbs_addr;
}h26x_job_t;

typedef struct _h26x_ctrl_t_{
	h26x_ver_item_t ver_item;

	h26x_job_t job[H26X_JOB_MAX];

	unsigned int job_num;
	unsigned int llc_addr[H26X_JOB_MAX];
	unsigned int llc_size;
	unsigned int wrap_va;
    int list_order[H26X_JOB_MAX];
	int cur_job_idx;
}h26x_ctrl_t;

typedef struct _h26x_srcd2d_t_{
    unsigned int is_hevc;
    unsigned int src_d2d_en;
    unsigned int src_d2d_mode;
    unsigned int src_cmp_luma_en;
    unsigned int src_cmp_crma_en;
    unsigned int src_rotate;
    unsigned int src_y_addr;
    unsigned int src_c_addr;
    unsigned int src_y_lineoffset;
    unsigned int src_c_lineoffset;
    unsigned int sdec[45];
}h26x_srcd2d_t;

void h26x_job_ctrl_init(h26x_ctrl_t *p_ctrl, h26x_mem_t *p_mem, h26x_mem_t *p_mem2);
void h26x_job_ctrl_close(h26x_ctrl_t *p_ctrl);
h26x_job_t *h26x_job_add(unsigned int codec, h26x_ctrl_t *p_ctrl);
BOOL h26x_job_excute(h26x_ctrl_t *p_ctrl, h26x_srcd2d_t *p_src_d2d);
void h26xEmuCloseTestBandWidth(void);
BOOL h26x_job_check_result(h26x_ctrl_t *p_ctrl, UINT32 interrupt, unsigned int rec_out_en);

#if 0 //d2d
BOOL emu_codec_open(UINT32 uiVirMemAddr1 , UINT32 uiSize1,UINT32 uiVirMemAddr2 , UINT32 uiSize2, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d, UINT32 job_num);
void emu_codec_run( h26x_srcd2d_t *p_src_d2d);
BOOL emu_codec_check(void);
#endif

//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif	// _EMU_H26X_JOB_H_
