#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <comm/nvtmem.h>

#include "kwrap/type.h"
//#include "kwrap/mitype.h"
//#include "kwrap/error_no.h"
//#include "frammap/frammap_if.h"
#include <mach/fmem.h>
#include <plat-na51055/nvt-sramctl.h>
#include <kwrap/file.h>

#include "md_api.h"
#include "md_drv.h"
//#include "md_dbg.h"
#include "mdbc_lib.h"
#include "mdbc_reg.h"

#include "kdrv_md.h"
#include "kdrv_type.h"

#define MDBC_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))
#define MDBC_MEM_SIZE       60*1024*1024
#define MDBC_REG_NUMBERS    0x110
#define LINKED_LIST_EN 1
extern void md_test(MDBC_INPUT_INFO InInfo, MDBC_OUTPUT_INFO OutInfo);
extern int nvt_kdrv_md_api_test(PMD_MODULE_INFO pmodule_info, unsigned char argc, char** pargv);
unsigned int MDBC_ENGINE_HOUSE[MDBC_REG_NUMBERS/4 + 1] = {0x0};
//NT98520_MDBC_REG_STRUCT *p_mdbc_reg = NULL;
KDRV_MD_PARAM mdbc_parm = {0};

typedef struct _MDBC_OUT_PARAM
{
    UINT32 SAO0_Addr;
    UINT32 SAO1_Addr;
    UINT32 SAO2_Addr;
    UINT32 SAO3_Addr;
    UINT32 Size;
    UINT32 pat_count;
	UINT32 bg_model_num;
} MDBC_OUT_PARAM;

int nvt_md_api_write_reg(PMD_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul (pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
 		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_md_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

char in_mdbc_file1[256] = "//mnt//sd//mdbc//input//dram_in_y.bin";
char in_mdbc_file2[256] = "//mnt//sd//mdbc//input//dram_in_uv.bin";
char in_mdbc_file3[256] = "//mnt//sd//mdbc//input//dram_in_uv_pre.bin";
char in_mdbc_file4[256] = "//mnt//sd//mdbc//input//dram_in_bgYUV.bin";
char in_mdbc_file5[256] = "//mnt//sd//mdbc//input//dram_in_var1.bin";
char in_mdbc_file6[256] = "//mnt//sd//mdbc//input//dram_in_var2.bin";
char out_mdbc_file1[256] = "//mnt//sd//mdbc//output//dram_out_s1.bin";
char out_mdbc_file2[256] = "//mnt//sd//mdbc//output//dram_out_bgYUV.bin";
char out_mdbc_file3[256] = "//mnt//sd//mdbc//output//dram_out_var1.bin";
char out_mdbc_file4[256] = "//mnt//sd//mdbc//output//dram_out_var2.bin";
char in_file1[256];
char in_file2[256];
char in_file3[256];
char in_file4[256];
char in_file5[256];
char in_file6[256];
char out_file1[256];
char out_file2[256];
char out_file3[256];
char out_file4[256];
int nvt_md_api_write_pattern(PMD_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
    int len = 0;
    int ret;
    //mm_segment_t old_fs;
	//struct file *fp;
	int fd;
	UINT32 width, height, size, bg_model_num;

    //frammap_buf_t in_buf_info ={0};
    //frammap_buf_t out_buf_info ={0};
    struct nvt_fmem_mem_info_t in_buf_info1 = {0};
    struct nvt_fmem_mem_info_t in_buf_info2 = {0};
    struct nvt_fmem_mem_info_t in_buf_info3 = {0};
    struct nvt_fmem_mem_info_t in_buf_info4 = {0};
    struct nvt_fmem_mem_info_t in_buf_info5 = {0};
    struct nvt_fmem_mem_info_t in_buf_info6 = {0};
	struct nvt_fmem_mem_info_t out_buf_info1 = {0};
	struct nvt_fmem_mem_info_t out_buf_info2 = {0};
	struct nvt_fmem_mem_info_t out_buf_info3 = {0};
	struct nvt_fmem_mem_info_t out_buf_info4 = {0};
    void *hdl_in_buf_info1 = NULL;
    void *hdl_in_buf_info2 = NULL;
    void *hdl_in_buf_info3 = NULL;
    void *hdl_in_buf_info4 = NULL;
    void *hdl_in_buf_info5 = NULL;
    void *hdl_in_buf_info6 = NULL;
    void *hdl_out_buf_info1 = NULL;
    void *hdl_out_buf_info2 = NULL;
    void *hdl_out_buf_info3 = NULL;
    void *hdl_out_buf_info4 = NULL;
    UINT32 in_addr1 = 0,in_addr2 = 0,in_addr3 = 0,in_addr4 = 0,in_addr5 = 0,in_addr6 = 0;
    UINT32 out_addr1 = 0,out_addr2 = 0,out_addr3 = 0,out_addr4 = 0;
    MDBC_INPUT_INFO     InInfo;
    MDBC_OUTPUT_INFO    OutInfo;

    width = 60;
    height = 60;
    size = (width * height);
    bg_model_num = 8;

    // input Y buffer
	ret = nvt_fmem_mem_info_init(&in_buf_info1, NVT_FMEM_ALLOC_CACHE, size, NULL);
	if (ret >= 0) {
		hdl_in_buf_info1 = nvtmem_alloc_buffer(&in_buf_info1);
		in_addr1 = (UINT32)in_buf_info1.vaddr;
	}
    // input UV buffer
    ret = nvt_fmem_mem_info_init(&in_buf_info2, NVT_FMEM_ALLOC_CACHE, size/2, NULL);
	if (ret >= 0) {
		hdl_in_buf_info2 = nvtmem_alloc_buffer(&in_buf_info2);
		in_addr2 = (UINT32)in_buf_info2.vaddr;
	}
    // input pre UV buffer
    ret = nvt_fmem_mem_info_init(&in_buf_info3, NVT_FMEM_ALLOC_CACHE, size/2, NULL);
	if (ret >= 0) {
		hdl_in_buf_info3 = nvtmem_alloc_buffer(&in_buf_info3);
		in_addr3 = (UINT32)in_buf_info3.vaddr;
	}
    // input BgYUV
    ret = nvt_fmem_mem_info_init(&in_buf_info4, NVT_FMEM_ALLOC_CACHE, size*5*bg_model_num, NULL);
	if (ret >= 0) {
		hdl_in_buf_info4 = nvtmem_alloc_buffer(&in_buf_info4);
		in_addr4 = (UINT32)in_buf_info4.vaddr;
	}
    // input Var1
    ret = nvt_fmem_mem_info_init(&in_buf_info5, NVT_FMEM_ALLOC_CACHE, size*6, NULL);
	if (ret >= 0) {
		hdl_in_buf_info5 = nvtmem_alloc_buffer(&in_buf_info5);
		in_addr5 = (UINT32)in_buf_info5.vaddr;
	}
    // input Var2
    ret = nvt_fmem_mem_info_init(&in_buf_info6, NVT_FMEM_ALLOC_CACHE, MDBC_ALIGN(size,16)*12, NULL);
	if (ret >= 0) {
		hdl_in_buf_info6 = nvtmem_alloc_buffer(&in_buf_info6);
		in_addr6 = (UINT32)in_buf_info6.vaddr;
	}
    // output S1
    ret = nvt_fmem_mem_info_init(&out_buf_info1, NVT_FMEM_ALLOC_CACHE, MDBC_ALIGN(size,8)/8, NULL);
	if (ret >= 0) {
		hdl_out_buf_info1 = nvtmem_alloc_buffer(&out_buf_info1);
		out_addr1 = (UINT32)out_buf_info1.vaddr;
	}
    // output BgYUV
    ret = nvt_fmem_mem_info_init(&out_buf_info2, NVT_FMEM_ALLOC_CACHE, size*5*bg_model_num, NULL);
	if (ret >= 0) {
		hdl_out_buf_info2 = nvtmem_alloc_buffer(&out_buf_info2);
		out_addr2 = (UINT32)out_buf_info2.vaddr;
	}
    // output Var1
    ret = nvt_fmem_mem_info_init(&out_buf_info3, NVT_FMEM_ALLOC_CACHE, size*6, NULL);
	if (ret >= 0) {
		hdl_out_buf_info3 = nvtmem_alloc_buffer(&out_buf_info3);
		out_addr3 = (UINT32)out_buf_info3.vaddr;
	}
    // output Var2
    ret = nvt_fmem_mem_info_init(&out_buf_info4, NVT_FMEM_ALLOC_CACHE, MDBC_ALIGN(size,16)*12, NULL);
	if (ret >= 0) {
		hdl_out_buf_info4 = nvtmem_alloc_buffer(&out_buf_info4);
		out_addr4 = (UINT32)out_buf_info4.vaddr;
	}

    InInfo.uiInAddr0 = in_addr1;
    InInfo.uiInAddr1 = in_addr2;
    InInfo.uiInAddr2 = in_addr3;
    InInfo.uiInAddr3 = in_addr4;
    InInfo.uiInAddr4 = in_addr5;
    InInfo.uiInAddr5 = in_addr6;
    InInfo.uiLofs0   = 60;
    InInfo.uiLofs1   = 60;
    OutInfo.uiOutAddr0 = out_addr1;
    OutInfo.uiOutAddr1 = out_addr2;
    OutInfo.uiOutAddr2 = out_addr3;
    OutInfo.uiOutAddr3 = out_addr4;

#if 0
	//Allocate Input memory
    in_buf_info.size = CNN_ALIGN(((in_w * in_h * in_bit) >> 3), 4) * in_ch;
    in_buf_info.align = 64;      ///< address alignment
    in_buf_info.name = "ai_t_in";
    in_buf_info.alloc_type = ALLOC_CACHEABLE;
    frm_get_buf_ddr(DDR_ID0, &in_buf_info);
	nvt_dbg(IND, "in_buf_info.va_addr = 0x%08x, in_buf_info.phy_addr = 0x%08x\r\n",(UINT32)in_buf_info.va_addr, (UINT32)in_buf_info.phy_addr);

	//Allocate Output memory
    out_buf_info.size = CNN_ALIGN(((out_w * out_h * out_bit) >> 3), 4) * out_ch;
    out_buf_info.align = 64;      ///< address alignment
    out_buf_info.name = "ai_t_out";
    out_buf_info.alloc_type = ALLOC_CACHEABLE;
    frm_get_buf_ddr(DDR_ID0, &out_buf_info);
	nvt_dbg(IND, "out_buf_info.va_addr = 0x%08x, out_buf_info.phy_addr = 0x%08x\r\n",(UINT32)out_buf_info.va_addr, (UINT32)out_buf_info.phy_addr);
#endif

    DBG_IND("MDBC Read input file\n");
	//Load input data y
/*
	fp = filp_open(in_mdbc_file1, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_mdbc_file1);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(fp, (char __user *)in_addr1, size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(in_mdbc_file1, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", in_mdbc_file1);
		return -1;
	}
	len = vos_file_read(fd, (void *)in_addr1, size);
	vos_file_close(fd);

    //Load input data uv
/*
	fp = filp_open(in_mdbc_file2, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_mdbc_file2);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(fp, (char __user *)in_addr2, size/2, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(in_mdbc_file2, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", in_mdbc_file2);
		return -1;
	}
	len = vos_file_read(fd, (void *)in_addr2, size/2);
	vos_file_close(fd);

    //Load input data pre uv
/*
	fp = filp_open(in_mdbc_file3, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_mdbc_file3);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(fp, (char __user *)in_addr3, size/2, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(in_mdbc_file3, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", in_mdbc_file3);
		return -1;
	}
	len = vos_file_read(fd, (void *)in_addr3, size/2);
	vos_file_close(fd);

    //Load input data bgYUV
/*
	fp = filp_open(in_mdbc_file4, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_mdbc_file4);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(fp, (char __user *)in_addr4, size*5*bg_model_num, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(in_mdbc_file4, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", in_mdbc_file4);
		return -1;
	}
	len = vos_file_read(fd, (void *)in_addr4, size*5*bg_model_num);
	vos_file_close(fd);

    //Load input data Var1
/*
	fp = filp_open(in_mdbc_file5, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_mdbc_file5);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(fp, (char __user *)in_addr5, size*6, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(in_mdbc_file5, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", in_mdbc_file5);
		return -1;
	}
	len = vos_file_read(fd, (void *)in_addr5, size*6);
	vos_file_close(fd);

    //Load input data Var2
/*
	fp = filp_open(in_mdbc_file6, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", in_mdbc_file6);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_read(fp, (char __user *)in_addr6, MDBC_ALIGN(size,16)*12, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(in_mdbc_file6, O_RDONLY, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "failed in file open:%s\r\n", in_mdbc_file6);
		return -1;
	}
	len = vos_file_read(fd, (void *)in_addr6, MDBC_ALIGN(size,16)*12);
	vos_file_close(fd);

	////////// Do something after get data from file //////////
	md_test(InInfo,OutInfo);

	//kfree(pbuffer);

    DBG_IND("MDBC Write output file\n");
    //Write output data S1
/*
	fp = filp_open(out_mdbc_file1, O_CREAT|O_WRONLY|O_SYNC , 0);
	if (IS_ERR_OR_NULL(fp)) {
	    DBG_ERR("failed in file open:%s\n", out_mdbc_file1);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_write(fp, (const char __user *)out_addr1, MDBC_ALIGN(size,8)/8, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(out_mdbc_file1, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "open %s failure\r\n", out_mdbc_file1);
		return -1;
	}

	len = vos_file_write(fd, (void *)out_addr1, MDBC_ALIGN(size,8)/8);
	vos_file_close(fd);

    //Write output data bgYuv
/*
	fp = filp_open(out_mdbc_file2, O_CREAT|O_WRONLY|O_SYNC , 0);
	if (IS_ERR_OR_NULL(fp)) {
	    DBG_ERR("failed in file open:%s\n", out_mdbc_file2);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_write(fp, (const char __user *)out_addr2, size*5*bg_model_num, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(out_mdbc_file2, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "open %s failure\r\n", out_mdbc_file2);
		return -1;
	}

	len = vos_file_write(fd, (void *)out_addr2, size*5*bg_model_num);
	vos_file_close(fd);

    //Write output data Var1
/*
	fp = filp_open(out_mdbc_file3, O_CREAT|O_WRONLY|O_SYNC , 0);
	if (IS_ERR_OR_NULL(fp)) {
	    DBG_ERR("failed in file open:%s\n", out_mdbc_file3);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_write(fp, (const char __user *)out_addr3, size*6, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(out_mdbc_file3, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "open %s failure\r\n", out_mdbc_file3);
		return -1;
	}

	len = vos_file_write(fd, (void *)out_addr3, size*6);
	vos_file_close(fd);

    //Write output data Var2
/*
	fp = filp_open(out_mdbc_file4, O_CREAT|O_WRONLY|O_SYNC , 0);
	if (IS_ERR_OR_NULL(fp)) {
	    DBG_ERR("failed in file open:%s\n", out_mdbc_file4);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len = vfs_write(fp, (const char __user *)out_addr4, MDBC_ALIGN(size,16)*12, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
*/
	fd = vos_file_open(out_mdbc_file4, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		nvt_dbg(ERR, "open %s failure\r\n", out_mdbc_file4);
		return -1;
	}

	len = vos_file_write(fd, (void *)out_addr4, MDBC_ALIGN(size,16)*12);
	vos_file_close(fd);

    DBG_IND("MDBC Release buffer\n");
    ret = nvtmem_release_buffer(hdl_in_buf_info1);
    ret = nvtmem_release_buffer(hdl_in_buf_info2);
    ret = nvtmem_release_buffer(hdl_in_buf_info3);
    ret = nvtmem_release_buffer(hdl_in_buf_info4);
    ret = nvtmem_release_buffer(hdl_in_buf_info5);
    ret = nvtmem_release_buffer(hdl_in_buf_info6);
	ret = nvtmem_release_buffer(hdl_out_buf_info1);
	ret = nvtmem_release_buffer(hdl_out_buf_info2);
	ret = nvtmem_release_buffer(hdl_out_buf_info3);
	ret = nvtmem_release_buffer(hdl_out_buf_info4);
    //frm_free_buf_ddr(in_buf_info.va_addr);
	//frm_free_buf_ddr(out_buf_info.va_addr);

	return len;
}

void md_test(MDBC_INPUT_INFO InInfo, MDBC_OUTPUT_INFO OutInfo)
{
#if 0
	MDBC_OPENOBJ ObjCB = {0};
	MDBC_PARAM mdbcPara = {0};

	ObjCB.FP_MDBCISR_CB = NULL;
    ObjCB.uiMdbcClockSel = 240;
	mdbc_open(&ObjCB);
	DBG_IND("MDBC open\n");

	mdbcPara.mode = NORM;
    mdbcPara.controlEn.update_nei_en = 1;
    mdbcPara.controlEn.deghost_en    = 1;
    mdbcPara.controlEn.roi_en0       = 0;
    mdbcPara.controlEn.roi_en1       = 0;
    mdbcPara.controlEn.roi_en2       = 0;
    mdbcPara.controlEn.roi_en3       = 0;
    mdbcPara.controlEn.roi_en4       = 0;
    mdbcPara.controlEn.roi_en5       = 0;
    mdbcPara.controlEn.roi_en6       = 0;
    mdbcPara.controlEn.roi_en7       = 0;
    mdbcPara.controlEn.chksum_en     = 0;
    mdbcPara.controlEn.bgmw_save_bw_en = 0;

    mdbcPara.InInfo  = InInfo;
    mdbcPara.OutInfo = OutInfo;

    mdbcPara.Size.uiMdbcWidth  = 60;
    mdbcPara.Size.uiMdbcHeight = 60;

    mdbcPara.MdmatchPara.lbsp_th    = 0x48;
    mdbcPara.MdmatchPara.d_colour   = 0xb;
    mdbcPara.MdmatchPara.r_colour   = 0xf;
    mdbcPara.MdmatchPara.d_lbsp     = 0xf;
    mdbcPara.MdmatchPara.r_lbsp     = 0x5;
    mdbcPara.MdmatchPara.model_num  = 8;
    mdbcPara.MdmatchPara.t_alpha    = 0x82;
    mdbcPara.MdmatchPara.dw_shift   = 0;
    mdbcPara.MdmatchPara.dlast_alpha= 0x1a7;
    mdbcPara.MdmatchPara.min_match  = 4;
    mdbcPara.MdmatchPara.dlt_alpha  = 0x17a;
    mdbcPara.MdmatchPara.dst_alpha  = 0x23d;
    mdbcPara.MdmatchPara.uv_thres   = 2;
    mdbcPara.MdmatchPara.s_alpha    = 0xd8;
    mdbcPara.MdmatchPara.dbg_lumDiff= 0x22b4;
    mdbcPara.MdmatchPara.dbg_lumDiff_en = 1;

    mdbcPara.MorPara.th_ero     = 6;
    mdbcPara.MorPara.th_dil     = 6;
    mdbcPara.MorPara.mor_sel0   = 2;
    mdbcPara.MorPara.mor_sel1   = 0;
    mdbcPara.MorPara.mor_sel2   = 2;
    mdbcPara.MorPara.mor_sel3   = 0;

    mdbcPara.UpdPara.minT           = 0x3e;
    mdbcPara.UpdPara.maxT           = 0x4a;
    mdbcPara.UpdPara.maxFgFrm       = 0x6c;
    mdbcPara.UpdPara.deghost_dth    = 0x1d;
    mdbcPara.UpdPara.deghost_sth    = 0x65;
    mdbcPara.UpdPara.stable_frm     = 0x3b;
    mdbcPara.UpdPara.update_dyn     = 0x82;
    mdbcPara.UpdPara.va_distth      = 32;
    mdbcPara.UpdPara.t_distth       = 24;
    mdbcPara.UpdPara.dbg_frmID      = 2;
    mdbcPara.UpdPara.dbg_frmID_en   = 1;
    mdbcPara.UpdPara.dbg_rnd        = 0x243;
    mdbcPara.UpdPara.dbg_rnd_en     = 1;

	DBG_IND("MDBC setMode\n");
	if(mdbc_setMode(&mdbcPara) != E_OK)
	{
		nvt_dbg(ERR,"mdbc_setMode error ..\r\n");
	}
	mdbc_start();
	mdbc_waitFrameEnd(FALSE);
	mdbc_pause();
#endif	
}

int nvt_kdrv_md_api_test(PMD_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
    int len = 0;
#if 0	
    int ret;
	//mm_segment_t old_fs;
	//struct file *fp;
	int fd;
	struct nvt_fmem_mem_info_t in_buf_info1 = {0};
	struct nvt_fmem_mem_info_t out_buf_info1 = {0};
    void *hdl_in_buf_info1 = NULL;
    void *hdl_out_buf_info1 = NULL;
	char io_path[64];
	char tmp[100];
	unsigned int tmp_val0=0, tmp_val1=0, i;
    UINT32 in_addr1 = 0,in_addr2 = 0,in_addr3 = 0,in_addr4 = 0,in_addr5 = 0,in_addr6 = 0;
    UINT32 out_addr1 = 0,out_addr2 = 0,out_addr3 = 0,out_addr4 = 0;
	UINT32 pat_count=0,pat_start=0,pat_end=0;
	UINT32 MDBC_Used_Start_Address,MDBC_Used_Start_Address_out;
    MDBC_INPUT_INFO     InInfo;
    MDBC_OUTPUT_INFO    OutInfo;
	UINT32 width, height, size, bg_model_num;
	KDRV_MD_OPENCFG md_open_obj;
	KDRV_MD_TRIGGER_PARAM kdrv_trig_param;
	#if LINKED_LIST_EN
		UINT32 LL_addr = 0,LL_Used_Address,arr_id,phyAdddr;
		UINT32 pat_num;
		MDBC_OUT_PARAM output_array[20] = {0};
	#endif

	INT32 chip = 0, engine = KDRV_CV_ENGINE_MD, channel = 0;
    INT32 id = KDRV_DEV_ID(chip, engine, channel);

	if (kstrtouint (pargv[0], 10, &pat_start)) {
		nvt_dbg(ERR, "invalid pat_start value:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtouint (pargv[1], 10, &pat_end)) {
		nvt_dbg(ERR, "invalid pat_end value:%s\n", pargv[1]);
 		return -EINVAL;

	}
	#if LINKED_LIST_EN
		pat_num   = pat_end - pat_start +1;
	#endif
	ret = nvt_fmem_mem_info_init(&in_buf_info1, NVT_FMEM_ALLOC_CACHE, MDBC_MEM_SIZE, NULL);
	if (ret >= 0) {
		hdl_in_buf_info1 = nvtmem_alloc_buffer(&in_buf_info1);
		#if LINKED_LIST_EN
			LL_addr = (UINT32)in_buf_info1.vaddr;
			MDBC_Used_Start_Address = MDBC_ALIGN(LL_addr+(8*68*20),4);
			LL_Used_Address = LL_addr;
		#endif
	}
	ret = nvt_fmem_mem_info_init(&out_buf_info1, NVT_FMEM_ALLOC_CACHE, MDBC_MEM_SIZE, NULL);
	if (ret >= 0) {
		hdl_out_buf_info1 = nvtmem_alloc_buffer(&out_buf_info1);
		#if LINKED_LIST_EN
			MDBC_Used_Start_Address_out = MDBC_ALIGN((UINT32)out_buf_info1.vaddr,4);
		#endif
	}
	for (pat_count = pat_start; pat_count <= pat_end; pat_count++){

		snprintf(io_path, 64, "//mnt//sd//MDBCP//RI//%d.dat", pat_count);
/*
		fp = filp_open(io_path, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", io_path);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		// parsing registers
		for (i=0;i<(MDBC_REG_NUMBERS/4+1);i++){
			memset(tmp, 0, 14*sizeof(char));
			len = vfs_read(fp, (void*)tmp, 4*sizeof(char), &fp->f_pos);
			if (len == 0)
			break;
			kstrtouint(tmp, 16, &tmp_val0);
			vfs_read(fp, (void*)tmp, 8*sizeof(char), &fp->f_pos);
			kstrtouint(tmp, 16, &tmp_val1);
			vfs_read(fp, (void*)tmp, 2*sizeof(char), &fp->f_pos);
			//nvt_dbg(IND, "read reg file:ofs=%04X val=%08X\n", tmp_val0, tmp_val1);
			MDBC_ENGINE_HOUSE[i] = tmp_val1;
		}
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(io_path, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", io_path);
			return -1;
		}
		for (i=0;i<(MDBC_REG_NUMBERS/4+1);i++){
			memset(tmp, 0, 14*sizeof(char));
			//len = vfs_read(fp, (void*)tmp, 4*sizeof(char), &fp->f_pos);
			len = vos_file_read(fd, (void *)tmp, 4*sizeof(char));
			if (len == 0)
			break;
			kstrtouint(tmp, 16, &tmp_val0);
			//vfs_read(fp, (void*)tmp, 8*sizeof(char), &fp->f_pos);
			vos_file_read(fd, (void *)tmp, 8*sizeof(char));
			kstrtouint(tmp, 16, &tmp_val1);
			//vfs_read(fp, (void*)tmp, 2*sizeof(char), &fp->f_pos);
			vos_file_read(fd, (void *)tmp, 2*sizeof(char));
			//nvt_dbg(IND, "read reg file:ofs=%04X val=%08X\n", tmp_val0, tmp_val1);
			MDBC_ENGINE_HOUSE[i] = tmp_val1;
		}
		vos_file_close(fd);
		p_mdbc_reg = (NT98520_MDBC_REG_STRUCT*)((unsigned int)MDBC_ENGINE_HOUSE);

		width = p_mdbc_reg->MDBC_Register_0048.bit.WIDTH<<1;
		height = p_mdbc_reg->MDBC_Register_0048.bit.HEIGHT<<1;
		size = (width * height);
		bg_model_num = p_mdbc_reg->MDBC_Register_0050.bit.BG_MODEL_NUM;
		DBG_IND("width: %d\n",width);
		DBG_IND("height: %d\n",height);
		DBG_IND("bg_model_num: %d\n",bg_model_num);

		// input Y buffer
		#if LINKED_LIST_EN
			mdbc_parm.uiLLAddr = LL_addr;
		#else
			MDBC_Used_Start_Address = MDBC_ALIGN((UINT32)in_buf_info1.vaddr,4);  //byte align
			MDBC_Used_Start_Address_out = MDBC_ALIGN((UINT32)out_buf_info1.vaddr,4);  //byte align
		#endif
		in_addr1 = MDBC_Used_Start_Address;
		in_addr2 = MDBC_ALIGN(in_addr1+size,4);   // input UV buffer
		in_addr3 = MDBC_ALIGN(in_addr2+size/2,4); // input pre UV buffer
		in_addr4 = MDBC_ALIGN(in_addr3+size/2,4); // input BgYUV
		in_addr5 = MDBC_ALIGN(in_addr4+size*5*bg_model_num,4);   // input Var1
		in_addr6 = MDBC_ALIGN(in_addr5+size*6,4);   // input Var2
		MDBC_Used_Start_Address = MDBC_ALIGN(in_addr6+MDBC_ALIGN(size,16)*12,4);
		// output S1
		out_addr1 = MDBC_Used_Start_Address_out;
		out_addr2 = MDBC_ALIGN(out_addr1+MDBC_ALIGN(size,8)/8,4);   // output BgYUV
		out_addr3 = MDBC_ALIGN(out_addr2+size*5*bg_model_num,4); // output Var1
		out_addr4 = MDBC_ALIGN(out_addr3+size*6,4); // output Var2
		MDBC_Used_Start_Address_out = MDBC_ALIGN(out_addr4+MDBC_ALIGN(size,16)*12,4);
		#if LINKED_LIST_EN
			arr_id = pat_count - pat_start;
			output_array[arr_id].SAO0_Addr = out_addr1;
			output_array[arr_id].SAO1_Addr = out_addr2;
			output_array[arr_id].SAO2_Addr = out_addr3;
			output_array[arr_id].SAO3_Addr = out_addr4;
			output_array[arr_id].Size = size;
			output_array[arr_id].pat_count = pat_count;
			output_array[arr_id].bg_model_num = bg_model_num;
			DBG_IND("output_array[%d].SAO0_Addr = 0x%08x\r\n", arr_id,output_array[arr_id].SAO0_Addr);
			DBG_IND("output_array[%d].SAO1_Addr = 0x%08x\r\n", arr_id,output_array[arr_id].SAO1_Addr);
			DBG_IND("output_array[%d].SAO2_Addr = 0x%08x\r\n", arr_id,output_array[arr_id].SAO2_Addr);
			DBG_IND("output_array[%d].SAO3_Addr = 0x%08x\r\n", arr_id,output_array[arr_id].SAO3_Addr);
			DBG_IND("output_array[%d].Size = %d\r\n", arr_id,output_array[arr_id].Size);
			DBG_IND("output_array[%d].pat_count = %d\r\n", arr_id,output_array[arr_id].pat_count);
			DBG_IND("output_array[%d].bg_model_num = %d\r\n", arr_id,output_array[arr_id].bg_model_num);
			p_mdbc_reg->MDBC_Register_0010.bit.DRAM_SAI0 = fmem_lookup_pa(in_addr1)>>2;
			DBG_IND("MDBC_ENGINE_HOUSE[4] = 0x%08x\r\n", MDBC_ENGINE_HOUSE[4]);
			p_mdbc_reg->MDBC_Register_0014.bit.DRAM_SAI1 = fmem_lookup_pa(in_addr2)>>2;
			p_mdbc_reg->MDBC_Register_0020.bit.DRAM_SAI2 = fmem_lookup_pa(in_addr3)>>2;
			p_mdbc_reg->MDBC_Register_0024.bit.DRAM_SAI3 = fmem_lookup_pa(in_addr4)>>2;
			p_mdbc_reg->MDBC_Register_0028.bit.DRAM_SAI4 = fmem_lookup_pa(in_addr5)>>2;
			p_mdbc_reg->MDBC_Register_002c.bit.DRAM_SAI5 = fmem_lookup_pa(in_addr6)>>2;
			p_mdbc_reg->MDBC_Register_0034.bit.DRAM_SAO0 = fmem_lookup_pa(out_addr1)>>2;
			p_mdbc_reg->MDBC_Register_0038.bit.DRAM_SAO1 = fmem_lookup_pa(out_addr2)>>2;
			p_mdbc_reg->MDBC_Register_003c.bit.DRAM_SAO2 = fmem_lookup_pa(out_addr3)>>2;
			p_mdbc_reg->MDBC_Register_0040.bit.DRAM_SAO3 = fmem_lookup_pa(out_addr4)>>2;
			DBG_IND("MDBC_ENGINE_HOUSE[16] = 0x%08x\r\n", MDBC_ENGINE_HOUSE[16]);
			MDBC_ENGINE_HOUSE[28] += (32<<16) + (24<<24);
		#endif

		InInfo.uiInAddr0 = in_addr1;
		InInfo.uiInAddr1 = in_addr2;
		InInfo.uiInAddr2 = in_addr3;
		InInfo.uiInAddr3 = in_addr4;
		InInfo.uiInAddr4 = in_addr5;
		InInfo.uiInAddr5 = in_addr6;
		InInfo.uiLofs0   = p_mdbc_reg->MDBC_Register_0018.bit.DRAM_OFSI0<<2;
		InInfo.uiLofs1   = p_mdbc_reg->MDBC_Register_001c.bit.DRAM_OFSI1<<2;
		OutInfo.uiOutAddr0 = out_addr1;
		OutInfo.uiOutAddr1 = out_addr2;
		OutInfo.uiOutAddr2 = out_addr3;
		OutInfo.uiOutAddr3 = out_addr4;

		//Load input data
		DBG_IND("MDBC Read input file\n");
		//Load input data y
		sprintf(in_file1, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_y.bin", pat_count);
/*
		fp = filp_open(in_file1, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_file1);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_read(fp, (char __user *)in_addr1, size, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(in_file1, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", in_file1);
			return -1;
		}
		vos_file_read(fd, (void *)in_addr1, size);
		vos_file_close(fd);

		//Load input data uv
		sprintf(in_file2, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_uv.bin", pat_count);
/*
		fp = filp_open(in_file2, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_file2);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_read(fp, (char __user *)in_addr2, size/2, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(in_file2, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", in_file2);
			return -1;
		}
		vos_file_read(fd, (void *)in_addr2, size/2);
		vos_file_close(fd);

		//Load input data pre uv
		sprintf(in_file3, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_uv_pre.bin", pat_count);
/*
		fp = filp_open(in_file3, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_file3);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_read(fp, (char __user *)in_addr3, size/2, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(in_file3, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", in_file3);
			return -1;
		}
		vos_file_read(fd, (void *)in_addr3, size/2);
		vos_file_close(fd);

		//Load input data bgYUV
		sprintf(in_file4, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_bgYUV.bin", pat_count);
/*
		fp = filp_open(in_file4, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_file4);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_read(fp, (char __user *)in_addr4, size*5*bg_model_num, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(in_file4, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", in_file4);
			return -1;
		}
		vos_file_read(fd, (void *)in_addr4, size*5*bg_model_num);
		vos_file_close(fd);

		//Load input data Var1
		sprintf(in_file5, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_var1.bin", pat_count);
/*
		fp = filp_open(in_file5, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_file5);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_read(fp, (char __user *)in_addr5, size*6, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(in_file5, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", in_file5);
			return -1;
		}
		vos_file_read(fd, (void *)in_addr5, size*6);
		vos_file_close(fd);

		//Load input data Var2
		sprintf(in_file6, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_var2.bin", pat_count);
/*
		fp = filp_open(in_file6, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			nvt_dbg(ERR, "failed in file open:%s\n", in_file6);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_read(fp, (char __user *)in_addr6, MDBC_ALIGN(size,16)*12, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(in_file6, O_RDONLY, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "failed in file open:%s\r\n", in_file6);
			return -1;
		}
		vos_file_read(fd, (void *)in_addr6, MDBC_ALIGN(size,16)*12);
		vos_file_close(fd);

#if LINKED_LIST_EN
		DBG_IND("LL_Used_Address = 0x%08x\r\n", LL_Used_Address);
		for(i=0;i<((MDBC_REG_NUMBERS / 4) -3);i++) {
			if(i!=2){
				//SET_32BitsValue(LL_Used_Address, GET_32BitsValue(_MDBC_REG_BASE_ADDR + 4*i));
				//SET_32BitsValue(LL_Used_Address + 4, 0x80000000 + 0xf000 + i*4);  //UPD
				*((UINT64*)LL_Used_Address) = mdbc_ll_upd_cmd(0xF, i*4, MDBC_ENGINE_HOUSE[i]);
				LL_Used_Address += 8;
				if((i+1)%12 == 0) {
					phyAdddr = fmem_lookup_pa(LL_Used_Address + 16);
					//SET_32BitsValue(LL_Used_Address, ((phyAdddr&0xffffff)<<8));
					//SET_32BitsValue(LL_Used_Address + 4, 0x40000000 + (phyAdddr>>24));  //Next_UPD
					*((UINT64*)LL_Used_Address) = mdbc_ll_nextupd_cmd(phyAdddr);
					LL_Used_Address += 16;
				}
			}
		}
		if(pat_count == pat_end) {
			//SET_32BitsValue(LL_Used_Address, ((pat_count - pat_start)%256));//NULL
			*((UINT64*)LL_Used_Address) = mdbc_ll_null_cmd(((pat_count - pat_start+1)%256));
		} else {
			phyAdddr = fmem_lookup_pa(LL_Used_Address + 16);
			//SET_32BitsValue(LL_Used_Address, ((phyAdddr&0xffffff)<<8) + ((pat_count - pat_start)%256));
			//SET_32BitsValue(LL_Used_Address + 4, 0x20000000 + (phyAdddr>>24));  //Next_LL
			*((UINT64*)LL_Used_Address) = mdbc_ll_nextll_cmd(phyAdddr, ((pat_count - pat_start)%256));
			LL_Used_Address += 16;
		}

#else
		/////////// md d2d flow ///////////
		md_open_obj.clock_sel = 240;
		kdrv_md_set(id, KDRV_MD_PARAM_OPENCFG, (void *)&md_open_obj);

		if(kdrv_md_open(chip, engine) != 0) {
			DBG_ERR("set opencfg fail!\r\n");
			return -EFAULT;
		}

		//kdrv_ai_set(id, KDRV_AI_PARAM_ISR_CB, (void *)&KDRV_AI_ISR_CB);

		mdbc_parm.mode = p_mdbc_reg->MDBC_Register_0004.bit.MDBC_MODE;
		mdbc_parm.controlEn.update_nei_en = p_mdbc_reg->MDBC_Register_0004.bit.BC_UPDATE_NEI_EN;
		mdbc_parm.controlEn.deghost_en    = p_mdbc_reg->MDBC_Register_0004.bit.BC_DEGHOST_EN;
		mdbc_parm.controlEn.roi_en0       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN0;
		mdbc_parm.controlEn.roi_en1       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN1;
		mdbc_parm.controlEn.roi_en2       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN2;
		mdbc_parm.controlEn.roi_en3       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN3;
		mdbc_parm.controlEn.roi_en4       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN4;
		mdbc_parm.controlEn.roi_en5       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN5;
		mdbc_parm.controlEn.roi_en6       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN6;
		mdbc_parm.controlEn.roi_en7       = p_mdbc_reg->MDBC_Register_0004.bit.ROI_EN7;
		mdbc_parm.controlEn.chksum_en     = p_mdbc_reg->MDBC_Register_0004.bit.CHKSUM_EN;
		mdbc_parm.controlEn.bgmw_save_bw_en = p_mdbc_reg->MDBC_Register_0004.bit.BGMW_SAVE_BW_EN;

		mdbc_parm.InInfo.uiInAddr0 = InInfo.uiInAddr0;
		mdbc_parm.InInfo.uiInAddr1 = InInfo.uiInAddr1;
		mdbc_parm.InInfo.uiInAddr2 = InInfo.uiInAddr2;
		mdbc_parm.InInfo.uiInAddr3 = InInfo.uiInAddr3;
		mdbc_parm.InInfo.uiInAddr4 = InInfo.uiInAddr4;
		mdbc_parm.InInfo.uiInAddr5 = InInfo.uiInAddr5;
		mdbc_parm.InInfo.uiLofs0   = InInfo.uiLofs0;
		mdbc_parm.InInfo.uiLofs1   = InInfo.uiLofs1;
		mdbc_parm.OutInfo.uiOutAddr0 = OutInfo.uiOutAddr0;
		mdbc_parm.OutInfo.uiOutAddr1 = OutInfo.uiOutAddr1;
		mdbc_parm.OutInfo.uiOutAddr2 = OutInfo.uiOutAddr2;
		mdbc_parm.OutInfo.uiOutAddr3 = OutInfo.uiOutAddr3;

		mdbc_parm.Size.uiMdbcWidth  = width;
		mdbc_parm.Size.uiMdbcHeight = height;

		mdbc_parm.MdmatchPara.lbsp_th    = p_mdbc_reg->MDBC_Register_004c.bit.LBSP_TH;//0x48;
		mdbc_parm.MdmatchPara.d_colour   = p_mdbc_reg->MDBC_Register_004c.bit.D_COLOUR;//0xb;
		mdbc_parm.MdmatchPara.r_colour   = p_mdbc_reg->MDBC_Register_004c.bit.R_COLOUR;//0xf;
		mdbc_parm.MdmatchPara.d_lbsp     = p_mdbc_reg->MDBC_Register_004c.bit.D_LBSP;//0xf;
		mdbc_parm.MdmatchPara.r_lbsp     = p_mdbc_reg->MDBC_Register_004c.bit.R_LBSP;//0x5;
		mdbc_parm.MdmatchPara.model_num  = bg_model_num;
		mdbc_parm.MdmatchPara.t_alpha    = p_mdbc_reg->MDBC_Register_0050.bit.T_ALPHA;//0x82;
		mdbc_parm.MdmatchPara.dw_shift   = p_mdbc_reg->MDBC_Register_0050.bit.DW_SHIFT;//0;
		mdbc_parm.MdmatchPara.dlast_alpha= p_mdbc_reg->MDBC_Register_0050.bit.D_LAST_ALPHA;//0x1a7;
		mdbc_parm.MdmatchPara.min_match  = p_mdbc_reg->MDBC_Register_0054.bit.BC_MIN_MATCH;//4;
		mdbc_parm.MdmatchPara.dlt_alpha  = p_mdbc_reg->MDBC_Register_0054.bit.DLT_ALPHA;//0x17a;
		mdbc_parm.MdmatchPara.dst_alpha  = p_mdbc_reg->MDBC_Register_0054.bit.DST_ALPHA;//0x23d;
		mdbc_parm.MdmatchPara.uv_thres   = p_mdbc_reg->MDBC_Register_0058.bit.BC_UV_THRES;//2;
		mdbc_parm.MdmatchPara.s_alpha    = p_mdbc_reg->MDBC_Register_0058.bit.S_ALPHA;//0xd8;
		mdbc_parm.MdmatchPara.dbg_lumDiff= p_mdbc_reg->MDBC_Register_005c.bit.DBG_LUM_DIFF;//0x22b4;
		mdbc_parm.MdmatchPara.dbg_lumDiff_en = p_mdbc_reg->MDBC_Register_005c.bit.DBG_LUM_DIFF_EN;//1;

		mdbc_parm.MorPara.th_ero     = p_mdbc_reg->MDBC_Register_0064.bit.TH_ERO;//6;
		mdbc_parm.MorPara.th_dil     = p_mdbc_reg->MDBC_Register_0064.bit.TH_DIL;//6;
		mdbc_parm.MorPara.mor_sel0   = p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL0;//KDRV_Erode;//2;
		mdbc_parm.MorPara.mor_sel1   = p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL1;//KDRV_Median;//0;
		mdbc_parm.MorPara.mor_sel2   = p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL2;//KDRV_Erode;//2;
		mdbc_parm.MorPara.mor_sel3   = p_mdbc_reg->MDBC_Register_0064.bit.MOR_SEL3;//KDRV_Median;//0;

		mdbc_parm.UpdPara.minT           = p_mdbc_reg->MDBC_Register_0068.bit.BC_MIN_T;//0x3e;
		mdbc_parm.UpdPara.maxT           = p_mdbc_reg->MDBC_Register_0068.bit.BC_MAX_T;//0x4a;
		mdbc_parm.UpdPara.maxFgFrm       = p_mdbc_reg->MDBC_Register_0068.bit.BC_MAX_FG_FRM;//0x6c;
		mdbc_parm.UpdPara.deghost_dth    = p_mdbc_reg->MDBC_Register_006c.bit.BC_DEGHOST_DTH;//0x1d;
		mdbc_parm.UpdPara.deghost_sth    = p_mdbc_reg->MDBC_Register_006c.bit.BC_DEGHOST_STH;//0x65;
		mdbc_parm.UpdPara.stable_frm     = p_mdbc_reg->MDBC_Register_0070.bit.BC_STABLE_FRAME;//0x3b;
		mdbc_parm.UpdPara.update_dyn     = p_mdbc_reg->MDBC_Register_0070.bit.BC_UPDATE_DYN;//0x82;
		mdbc_parm.UpdPara.va_distth      = 32;
		mdbc_parm.UpdPara.t_distth       = 24;
		mdbc_parm.UpdPara.dbg_frmID      = p_mdbc_reg->MDBC_Register_0074.bit.DBG_FRM_ID;//2;
		mdbc_parm.UpdPara.dbg_frmID_en   = p_mdbc_reg->MDBC_Register_0074.bit.DBG_FRM_ID_EN;//1;
		mdbc_parm.UpdPara.dbg_rnd        = p_mdbc_reg->MDBC_Register_0078.bit.DBG_RND;//0x243;
		mdbc_parm.UpdPara.dbg_rnd_en     = p_mdbc_reg->MDBC_Register_0078.bit.DBG_RND_EN;//1;

		kdrv_md_set(id, KDRV_MD_PARAM_ALL, (void *)&mdbc_parm);

		kdrv_trig_param.is_nonblock = 0;
		kdrv_trig_param.time_out_ms = 0;
		DBG_IND("~~~Kdrv_trigger start!\r\n");
		kdrv_md_trigger(id, &kdrv_trig_param, NULL, NULL);
		DBG_IND("~~~Kdrv_trigger end!\r\n");

		kdrv_md_close(chip,engine);
		/////////// md d2d flow ///////////

		DBG_IND("MDBC Write output file\n");
		//Write output data S1
		sprintf(out_file1, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_s1.bin", pat_count);
/*
		fp = filp_open(out_file1, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file1);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_write(fp, (const char __user *)out_addr1, MDBC_ALIGN(size,8)/8, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file1, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file1);
			return -1;
		}

		vos_file_write(fd, (void *)out_addr1, MDBC_ALIGN(size,8)/8);
		vos_file_close(fd);

		//Write output data bgYuv
		sprintf(out_file2, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_bgYUV.bin", pat_count);
/*
		fp = filp_open(out_file2, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file2);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_write(fp, (const char __user *)out_addr2, size*5*bg_model_num, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file2, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file2);
			return -1;
		}

		vos_file_write(fd, (void *)out_addr2, size*5*bg_model_num);
		vos_file_close(fd);

		//Write output data Var1
		sprintf(out_file3, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var1.bin", pat_count);
/*
		fp = filp_open(out_file3, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file3);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_write(fp, (const char __user *)out_addr3, size*6, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file3, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file3);
			return -1;
		}

		vos_file_write(fd, (void *)out_addr3, size*6);
		vos_file_close(fd);

		//Write output data Var2
		sprintf(out_file4, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var2.bin", pat_count);
/*
		fp = filp_open(out_file4, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file4);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		len = vfs_write(fp, (const char __user *)out_addr4, MDBC_ALIGN(size,16)*12, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file4, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file4);
			return -1;
		}

		vos_file_write(fd, (void *)out_addr4, MDBC_ALIGN(size,16)*12);
		vos_file_close(fd);
#endif
	}

#if LINKED_LIST_EN
	md_open_obj.clock_sel = 240;
	kdrv_md_set(id, KDRV_MD_PARAM_OPENCFG, (void *)&md_open_obj);

	if(kdrv_md_open(chip, engine) != 0) {
		DBG_ERR("set opencfg fail!\r\n");
		return -EFAULT;
	}

	kdrv_md_set(id, KDRV_MD_PARAM_ALL, (void *)&mdbc_parm);

	kdrv_trig_param.is_nonblock = 1;
	kdrv_trig_param.time_out_ms = 0;
	DBG_IND("~~~Kdrv_trigger start!\r\n");
	kdrv_md_trigger(id, &kdrv_trig_param, NULL, NULL);
	DBG_IND("~~~Kdrv_trigger end!\r\n");

	kdrv_md_close(chip,engine);

	DBG_IND("MDBC Write output file\n");

	for(arr_id = 0; arr_id<pat_num; arr_id++) {
		//Write output data S1
		DBG_IND("output_array[%d].SAO0_Addr = 0x%08x\r\n", arr_id,output_array[arr_id].SAO0_Addr);
		sprintf(out_file1, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_s1.bin", output_array[arr_id].pat_count);
/*
		fp = filp_open(out_file1, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file1);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_write(fp, (const char __user *)output_array[arr_id].SAO0_Addr, MDBC_ALIGN(output_array[arr_id].Size,8)/8, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file1, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file1);
			return -1;
		}

		vos_file_write(fd, (void *)output_array[arr_id].SAO0_Addr, MDBC_ALIGN(output_array[arr_id].Size,8)/8);
		vos_file_close(fd);

		//Write output data bgYuv
		sprintf(out_file2, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_bgYUV.bin", output_array[arr_id].pat_count);
/*
		fp = filp_open(out_file2, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file2);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_write(fp, (const char __user *)output_array[arr_id].SAO1_Addr, output_array[arr_id].Size*5*output_array[arr_id].bg_model_num, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file2, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file2);
			return -1;
		}

		vos_file_write(fd, (void *)output_array[arr_id].SAO1_Addr, output_array[arr_id].Size*5*output_array[arr_id].bg_model_num);
		vos_file_close(fd);

		//Write output data Var1
		sprintf(out_file3, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var1.bin", output_array[arr_id].pat_count);
/*
		fp = filp_open(out_file3, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file3);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		vfs_write(fp, (const char __user *)output_array[arr_id].SAO2_Addr, output_array[arr_id].Size*6, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file3, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file3);
			return -1;
		}

		vos_file_write(fd, (void *)output_array[arr_id].SAO2_Addr, output_array[arr_id].Size*6);
		vos_file_close(fd);

		//Write output data Var2
		sprintf(out_file4, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var2.bin", output_array[arr_id].pat_count);
/*
		fp = filp_open(out_file4, O_CREAT|O_WRONLY|O_SYNC , 0);
		if (IS_ERR_OR_NULL(fp)) {
			DBG_ERR("failed in file open:%s\n", out_file4);
			return -EFAULT;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		len = vfs_write(fp, (const char __user *)output_array[arr_id].SAO3_Addr, MDBC_ALIGN(output_array[arr_id].Size,16)*12, &fp->f_pos);
		filp_close(fp, NULL);
		set_fs(old_fs);
*/
		fd = vos_file_open(out_file4, O_CREAT|O_WRONLY|O_SYNC, 0);
		if ((VOS_FILE)(-1) == fd) {
			nvt_dbg(ERR, "open %s failure\r\n", out_file4);
			return -1;
		}

		vos_file_write(fd, (void *)output_array[arr_id].SAO3_Addr, MDBC_ALIGN(output_array[arr_id].Size,16)*12);
		vos_file_close(fd);
	}

#endif

	ret = nvtmem_release_buffer(hdl_in_buf_info1);
	ret = nvtmem_release_buffer(hdl_out_buf_info1);
#endif	
	return len;
}

int nvt_md_api_read_reg(PMD_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_md_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}
