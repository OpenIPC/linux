#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <kwrap/file.h>
#include "kwrap/mem.h"
#include "ive_proc.h"
#include "ive_main.h"
#include "ive_api.h"
#include "ive_dbg.h"
#include "ive_platform.h"

#if !defined(CONFIG_NVT_SMALL_HDAL)

UINT32 g_gen_filter_coeff[GEN_COEFF_NUM] = { 0,1,2,3,4,5,6,7,8,9};
INT32 g_lap_edge_filter[EDGE_COEFF_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
INT32 g_edge_x_filter[EDGE_COEFF_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
INT32 g_edge_y_filter[EDGE_COEFF_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
UINT32 g_neightbor_en[24] ={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

static void ive_test(unsigned char argc, char** pargv)
{

    IVE_OPENOBJ ive_open_param = {0};
    IVE_PARAM ive_param_obj = {0};

	int fd, fd2;
	int len = 0;
	int ret=0;
	int in_buf_size = 0, out_buf_size =0;

	struct vos_mem_cma_info_t in_buf = {0}, out_buf = {0};
	VOS_MEM_CMA_HDL in_buf_id;
	VOS_MEM_CMA_HDL out_buf_id;

    ive_open_param.FP_IVEISR_CB = NULL;
	ive_open_param.IVE_CLOCKSEL = 480;
	ive_open(&ive_open_param);

    ive_param_obj.InSize.uiWidth = 640;
	ive_param_obj.InSize.uiHeight = 480;

	ive_param_obj.uiOutDataSel = IMG_MEDIAN_8B;


	ive_param_obj.FuncEn.bGenFiltEn = TRUE;
	ive_param_obj.GenF.puiGenCoeff = g_gen_filter_coeff;


    ive_param_obj.FuncEn.bMednFiltEn = FALSE;
    ive_param_obj.uiMednMode = 0;


    ive_param_obj.FuncEn.bEdgeFiltEn =FALSE;
	ive_param_obj.EdgeF.uiEdgeMode = NO_DIR;
	ive_param_obj.EdgeF.pEdgeCoeff1 = g_edge_x_filter;
	ive_param_obj.EdgeF.pEdgeCoeff2 = g_edge_y_filter;
    ive_param_obj.EdgeF.uiAngSlpFact = 0;


	ive_param_obj.FuncEn.bMorphFiltEn = FALSE;
    ive_param_obj.MorphF.uiMorphOp = 0;
    ive_param_obj.uiMorphInSel = 0;


    ive_param_obj.MorphF.pbMorphNeighEn = g_neightbor_en;

    ive_param_obj.FuncEn.bThresLutEn = FALSE;


	DBG_ERR("ive_test \n") ;
    DBG_ERR("file open:%s\n", pargv[0]);
	//Allocate input memory

	 //get Y in buffer
	in_buf_size = ive_param_obj.InSize.uiWidth * ive_param_obj.InSize.uiHeight;
	if (0 != vos_mem_init_cma_info(&in_buf, VOS_MEM_CMA_TYPE_CACHE, in_buf_size)) { //VOS_MEM_CMA_TYPE_NONCACHE, VOS_MEM_CMA_TYPE_CACHE
		DBG_ERR("vos_mem_init_cma_info: init buffer fail. \r\n");
		return;
	} else {
		in_buf_id = vos_mem_alloc_from_cma(&in_buf);
		if (NULL == in_buf_id) {
			DBG_ERR("get buffer fail\n");
			return;
		}
	}

	DBG_ERR("in_buf.va_addr = 0x%08x\r\n", (UINT32)in_buf.vaddr);
	DBG_ERR("in_buf.phy_addr = 0x%08x\r\n", (UINT32)in_buf.paddr);


	ive_param_obj.DmaIn.uiInSaddr = (UINT32)in_buf.vaddr;
	ive_param_obj.DmaIn.uiInLofst = 640;

    //read input file
	fd = vos_file_open(pargv[0], O_RDONLY , 0);
	if ((VOS_FILE)(-1) == fd) {
		DBG_ERR("failed in file open:%s\n", pargv[0]);

		return;
	}

	len = vos_file_read(fd, (void *)in_buf.vaddr, in_buf.size);
	DBG_IND("Read %d bytes\r\n", len);
	vos_file_close(fd);


	//Allocate output memory
	ive_param_obj.DmaOut.uiOutLofst = 640;


	//get Y out buffer
	out_buf_size = ive_param_obj.DmaOut.uiOutLofst * ive_param_obj.InSize.uiHeight ;
	if (0 != vos_mem_init_cma_info(&out_buf, VOS_MEM_CMA_TYPE_CACHE, out_buf_size)) { //VOS_MEM_CMA_TYPE_NONCACHE, VOS_MEM_CMA_TYPE_CACHE
		DBG_ERR("vos_mem_init_cma_info: init buffer fail. \r\n");
		return;
	} else {
		out_buf_id = vos_mem_alloc_from_cma(&out_buf);
		if (NULL == out_buf_id) {
			DBG_ERR("get buffer fail\n");
			return;
		}
	}

    DBG_ERR("yout_buf_info.vaddr = 0x%08x\r\n", (UINT32)out_buf.vaddr);
    DBG_ERR("uiOutLofst = %d\r\n", ive_param_obj.DmaOut.uiOutLofst);
    DBG_ERR("out_buf_size = %d\r\n", out_buf_size);


	ive_param_obj.DmaOut.uiOutSaddr = (UINT32)out_buf.vaddr ;

	DBG_ERR("out_buf.vaddr = 0x%08x\r\n", (UINT32)out_buf.vaddr);
	DBG_ERR("out_buf.paddr = 0x%08x\r\n", (UINT32)out_buf.paddr);
	DBG_ERR("out_buf.size  = 0x%08x\r\n", (UINT32)out_buf.size);


	DBG_ERR("ive_param_obj FuncEn %d : \r\n",ive_param_obj.FuncEn.bGenFiltEn ) ;
	DBG_ERR("ive_param_obj bEdgeFiltEn %d : \r\n",ive_param_obj.FuncEn.bEdgeFiltEn ) ;
	DBG_ERR("ive_param_obj bMednFiltEn %d : \r\n",ive_param_obj.FuncEn.bMednFiltEn ) ;
	DBG_ERR("ive_param_obj bNonMaxEn %d : \r\n",ive_param_obj.FuncEn.bNonMaxEn ) ;
	DBG_ERR("ive_param_obj bThresLutEn %d : \r\n",ive_param_obj.FuncEn.bThresLutEn ) ;
	DBG_ERR("ive_param_obj bMorphFiltEn %d : \r\n",ive_param_obj.FuncEn.bMorphFiltEn ) ;
	DBG_ERR("ive_param_obj bIntegralEn %d : \r\n",ive_param_obj.FuncEn.bIntegralEn ) ;


	DBG_ERR("ive_param_obj.uiOutDataSel : %d  \r\n",ive_param_obj.uiOutDataSel ) ;
	DBG_ERR("ive_param_obj.MorphF.uiMorphOp : %d \r\n",ive_param_obj.MorphF.uiMorphOp ) ;
	DBG_ERR("ive_param_obj.uiMorphInSel : %d  \r\n",ive_param_obj.uiMorphInSel ) ;




	ive_setMode(&ive_param_obj);
	DBG_ERR("ive_setMode done\r\n");


	ive_start();
	DBG_ERR("ive start\r\n");

	ive_waitFrameEnd();
	DBG_ERR("ive get frame end\r\n");

	ive_pause();
	ive_close();

    //write output file
	fd2 = vos_file_open("/mnt/sd/y_out_test.bin", O_CREAT|O_WRONLY|O_SYNC , 0);
	if ((VOS_FILE)(-1) == fd2) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/y_out.bin");
		return;
	}

	len = vos_file_write(fd2, (void *) out_buf.vaddr, out_buf.size);
	DBG_ERR("Write %d bytes\r\n", len);
	vos_file_close(fd2);

	ret = vos_mem_release_from_cma(in_buf_id);
    if (ret != 0) {
        DBG_ERR("failed in release buffer\n");
        return;
    }

	ret = vos_mem_release_from_cma(out_buf_id);
    if (ret != 0) {
        DBG_ERR("failed in release buffer\n");
        return;
    }

}

#if (IVE_SYS_VFY_EN == DISABLE)
int nvt_kdrv_ipp_api_ive_test(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	DBG_ERR("IVE: Error , please set IVE_WORK_FLOW == DISABLE in ive_platform.h\r\n");
	return 0;
}
#endif

int nvt_ive_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -EINVAL;
	}

	ive_test(argc, pargv);

	return 0;
}

int nvt_ive_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;

	if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	DBG_IND("R REG 0x%lx\n", reg_addr);
	value = nvt_ive_drv_read_reg(pmodule_info, reg_addr);

	DBG_ERR("REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

int nvt_ive_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		DBG_ERR("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul (pargv[1], 0, &reg_value)) {
		DBG_ERR("invalid rag value:%s\n", pargv[1]);
 		return -EINVAL;

	}

	DBG_IND("W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_ive_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

#endif
