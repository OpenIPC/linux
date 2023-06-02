#include "dis_platform.h"
#include "dis_api.h"
#include "dis_lib.h"


#define EMU_DIS DISABLE


#if (EMU_DIS == ENABLE)
#define DIS_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))
static UINT32 g_ipe_out_hsize_hw = 1920, g_ipe_out_vsize_hw = 1080;static UINT32 g_ipe_lineofs_hw = 480;
#endif

#if defined(__FREERTOS)
extern ER uart_getString(CHAR *pcString, UINT32 *pcBufferLen);
extern ER uart_getChar(CHAR *pcData);

#if defined(__FREERTOS)
extern char end[];
#define POOL_ID_APP_ARBIT		0
#define POOL_ID_APP_ARBIT2		1
extern UINT32 OS_GetMempoolAddr(UINT32 id);
extern UINT32 OS_GetMempoolSize(UINT32 id);
#endif


#endif


#if (EMU_DIS == ENABLE)

static int load_data(char* path, UINT32 addr, UINT32 size)
{
#if defined(__FREERTOS)
	FST_FILE file;
	unsigned int file_buf_size;
	INT32 fstatus;
#else
	mm_segment_t old_fs;
	struct file *fp;
	struct kstat statbuf = {0};
	int ret = 0;
#endif
	int len = 0;

#if defined(__FREERTOS)
    
    file_buf_size = FileSys_GetFileLen(path);
	
	file = FileSys_OpenFile(path, FST_OPEN_READ);
    if (file == 0) {
        nvt_dbg(ERR, "Invalid file: %s\r\n", path);
        return -1;
    }
	fstatus = FileSys_ReadFile(file, (UINT8 *)addr, (UINT32 *) &file_buf_size, 0, NULL);
	if (fstatus != 0) {        
		nvt_dbg(ERR, "%s:%dfail to FileSys_ReadFile.\r\n", __FILE__, __LINE__);
        return -1;
    }
	FileSys_CloseFile(file);
	len = file_buf_size;
#else
	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	ret = vfs_stat(path, &statbuf);
	if (0 == ret) {
		size = (unsigned int)statbuf.size;
	} else {
		nvt_dbg(ERR, "load_data_a: addr(0x%x) fail to get file size=0x%x\r\n", addr, size);
	}
	len = vfs_read(fp, (void*)addr, size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
#endif
			
	return len;
}

static int dump_data(char* path, UINT32 addr, UINT32 size)
{
#if defined(__FREERTOS)
	FST_FILE file;
	INT32 fstatus;
#else
	mm_segment_t old_fs;
	struct file *fp;
#endif
	int len = 0;

#if defined(__FREERTOS)
	file = FileSys_OpenFile(path, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
    if (file == 0) {
        nvt_dbg(ERR, "Invalid file: %s\r\n", path);
        return -1;
    }
    fstatus = FileSys_WriteFile(file, (UINT8 *)addr, (UINT32 *) &size, 0, NULL);
	if (fstatus < 0) {
		nvt_dbg(ERR, "%s:%dError to FileSys_WriteFile at %s\n", __FILE__, __LINE__, path);
		FileSys_CloseFile(file);
		return -1;
	}
	FileSys_CloseFile(file);
#else
	fp = filp_open(path, O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", path);
		return -1;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len = vfs_write(fp, (const char __user *)addr, size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
#endif
			
	return len;
}

void dis_test(UINT32 cur_egmap_addr, UINT32 ref_egmap_addr, UINT32 dis_outbuf_addr)
{
    DIS_OPENOBJ dis_openobj = {0};
    DIS_PARAM modeInfo = {0};
	UINT32 uiSize;

    dis_openobj.FP_DISISR_CB = NULL;
    dis_openobj.uiDisClockSel = 480;//220;
#if 1
	nvt_dbg(IND, "\r\n dis_open start!\r\n");
    dis_open(&dis_openobj);
	nvt_dbg(IND, "\r\n dis_open end!\r\n");
	
    modeInfo.uiWidth = g_ipe_out_hsize_hw;
    modeInfo.uiHeight= g_ipe_out_vsize_hw;
    modeInfo.uiInOfs = g_ipe_lineofs_hw;
    modeInfo.uiInAdd0 = cur_egmap_addr;
    modeInfo.uiInAdd1 = ref_egmap_addr;
    modeInfo.uiInAdd2 = ref_egmap_addr;

    modeInfo.uiOutAdd0 = dis_outbuf_addr;
    modeInfo.uiOutAdd1 = dis_outbuf_addr;
    modeInfo.uiIntEn = 0; //enable when process is unlocked

	uiSize  = modeInfo.uiInOfs * modeInfo.uiHeight;

#if 0
	if (cur_egmap_addr != 0) {
		dis_platform_dma_flush(cur_egmap_addr, uiSize);
	}
	if (ref_egmap_addr != 0) {
		dis_platform_dma_flush(ref_egmap_addr, uiSize);
	}
#endif
	
#if 1

    nvt_dbg(IND,"uiWidth = %d\r\n",modeInfo.uiWidth);
    nvt_dbg(IND,"uiHeight = %d\r\n",modeInfo.uiHeight);
    nvt_dbg(IND,"uiInOfs = %d\r\n",modeInfo.uiInOfs);

    nvt_dbg(IND,"uiInAdd0 = 0x%x\r\n",modeInfo.uiInAdd0);
    nvt_dbg(IND,"uiInAdd1 = 0x%x\r\n",modeInfo.uiInAdd1);
    nvt_dbg(IND,"uiInAdd2 = 0x%x\r\n",modeInfo.uiInAdd2);
    nvt_dbg(IND,"uiOutAdd0 = 0x%x\r\n",modeInfo.uiOutAdd0);
    nvt_dbg(IND,"uiOutAdd1 = 0x%x\r\n",modeInfo.uiOutAdd1);
#endif	

    dis_setMode(&modeInfo);
	nvt_dbg(IND, "\r\n dis_setMode end!\r\n");
#if defined(__FREERTOS)
#else
    dis_changeInterrupt(DIS_INT_FRM);
#endif
    dis_start();
	nvt_dbg(IND, "\r\n dis_start end!\r\n");
    dis_waitFrameEnd(FALSE);
	nvt_dbg(IND, "\r\n dis_waitFrameEnd end!\r\n");
    dis_pause();
	nvt_dbg(IND, "\r\n dis_pause end!\r\n");
    dis_close();
	nvt_dbg(IND, "\r\n dis_close end!\r\n");
#endif
}

int nvt_dis_api_write_pattern(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	int len = 0;

#if defined(__FREERTOS)
#else
		struct	vos_mem_cma_info_t buf_info = {0};
		VOS_MEM_CMA_HDL vos_mem_id;
#endif

	UINT32 mem_base = 0;
	UINT32 mem_end = 0;
	UINT32 mem_size = 0;//0x14800000;

	UINT32 egmap_size, dis_mv_size;
	UINT8 *p_cur_egmap_addr, *p_ref_egmap_addr, *p_dis_outbuf_addr;
	static MOTION_INFOR motresult[MVNUMMAX] = {0};
	UINT32 *p_ix_addr, *p_iy_addr, *p_valid_addr;
	
	UINT32 save_size;
	MDS_DIM mdsDim;
	UINT32	blkpMds;
	UINT32  i,j;

	UINT32 frm;
	UINT32 idxSt = 131;
	UINT32 idxEd = 570; 
	static char filename[100];
	UINT32 in_buf_size = 0;
	UINT32 out_buf_size = 0;
	
#if defined(__FREERTOS)

	/*UINT32 cStrLen = 256;	 
	CHAR cStartStr[256], cEndStr[256];
	INT32 iPatSNum = 0, iPatENum = 0;

	nvt_dbg(ERR, "******* DIS Verification*******\r\n");
    nvt_dbg(ERR, "Please enter the start No.: ");
    uart_getString(cStartStr, &cStrLen);
    iPatSNum = atoi(cStartStr);
    nvt_dbg(ERR, "\r\n");

    nvt_dbg(ERR, "Please enter the end No.: ");
    uart_getString(cEndStr, &cStrLen);
    iPatENum = atoi(cEndStr);
    nvt_dbg(ERR, "\r\n");

	idxSt = (UINT32) iPatSNum;
	idxEd = (UINT32) iPatENum;*/

#endif

	egmap_size	= (g_ipe_out_hsize_hw * g_ipe_out_vsize_hw)>>2;
	dis_mv_size = sizeof(MOTION_INFOR)*MVNUMMAX;
	in_buf_size = DIS_ALIGN((egmap_size*2 + dis_mv_size),4) ;
	out_buf_size = DIS_ALIGN((dis_mv_size),4) ;


#if defined(__FREERTOS)

	mem_base = OS_GetMempoolAddr(POOL_ID_APP_ARBIT2);
	mem_size = OS_GetMempoolSize(POOL_ID_APP_ARBIT2);
	nvt_dbg(IND, "mem_base = 0x%08X, mem_size = 0x%08X\r\n", mem_base,  mem_size );

	mem_end = mem_base + mem_size;
	dis_setBaseAddr(0xF0C50000);
	dis_create_resource();

#else
	mem_size = in_buf_size+out_buf_size;

	if (0 != vos_mem_init_cma_info(&buf_info, VOS_MEM_CMA_TYPE_CACHE, mem_size)) {        
		nvt_dbg(ERR, "vos_mem_init_cma_info: init buffer fail. \r\n");
		return -1;
    } else {
		vos_mem_id = vos_mem_alloc_from_cma(&buf_info);
		if (NULL == vos_mem_id) {
			nvt_dbg(ERR, "get buffer fail\n");
            return -1;
    	}
	}

	mem_base = (UINT32)buf_info.vaddr;
	mem_end = mem_base + mem_size;
#endif

	//Allocate Input memory
	p_cur_egmap_addr  = (UINT8 *)mem_base;
	p_ref_egmap_addr  = p_cur_egmap_addr + egmap_size;
	p_dis_outbuf_addr = p_ref_egmap_addr + egmap_size;

	mem_base = mem_base + in_buf_size;

	//Allocate Output memory
	p_ix_addr    = (UINT32 *)mem_base;
	p_iy_addr    = p_ix_addr + MVNUMMAX;
	p_valid_addr = p_iy_addr + MVNUMMAX;

	if ((UINT32 )(p_valid_addr + MVNUMMAX) > mem_end){
		nvt_dbg(ERR, "This pattern cost too many memory! mem_size = 0x%08X, buffer_size = 0x%08X\n", mem_size, in_buf_size+out_buf_size);
		return -1;
	}
		
	for ( frm= idxSt; frm <idxEd; frm++ ) {
    	static UINT32 frmcnt = 2;
		
		//Load input data

#if defined(__FREERTOS)
		snprintf(filename, 64, "A:\\DIS\\IN\\img_5m\\frame%ld_rawData.bin", frm);
#else
		snprintf(filename, 64, "/mnt/sd/DIS/IN/img_5m/frame%d_rawData.bin" , frm);
#endif
		nvt_dbg(ERR, "\r\nimage: %s................................\r\n", filename);
		
		len = load_data(filename, (UINT32)p_cur_egmap_addr, egmap_size);

#if defined(__FREERTOS)
		snprintf(filename, 64, "A:\\DIS\\IN\\img_5m\\frame%ld_rawData.bin", frm+1);
		
#else
		snprintf(filename, 64, "/mnt/sd/DIS/IN/img_5m/frame%d_rawData.bin" , frm+1);
#endif
		nvt_dbg(IND, "\r\nimage: %s................................\r\n", filename);
		
		len = load_data(filename, (UINT32)p_ref_egmap_addr, egmap_size);



		nvt_dbg(IND, "\r\n Read raw data end!\r\n");
		/* Do something after get data from file */
		dis_test((UINT32)p_cur_egmap_addr,(UINT32)p_ref_egmap_addr, (UINT32)p_dis_outbuf_addr);
		nvt_dbg(IND, "\r\n dis_test end!\r\n");
	    dis_getMotionVectors( motresult);
		nvt_dbg(IND, "\r\n dis_getMotionVectors end!\r\n");

		mdsDim = dis_getMDSDim();
		blkpMds = mdsDim.uiBlkNumH * mdsDim.uiMdsNum;
		save_size = blkpMds*mdsDim.uiBlkNumV*sizeof(UINT32);

		for (j = 0; j < mdsDim.uiBlkNumV; j++) {
			for (i = 0; i < blkpMds; i++) {			
				p_ix_addr[j*blkpMds+i] = motresult[j*blkpMds+i].iX;
				p_iy_addr[j*blkpMds+i] = motresult[j*blkpMds+i].iY;
				p_valid_addr[j*blkpMds+i]= motresult[j*blkpMds+i].bValid;
				}
			}

		nvt_dbg(IND, "\r\n start to save result!\r\n");

#if defined(__FREERTOS)
		snprintf(filename, 64, "A:\\DIS\\outBin\\api_main\\alg_%ld_iX_test.raw", frmcnt);
#else
    	snprintf(filename, 64, "/mnt/sd/DIS/outBin/api_main/alg_%d_iX_test.raw", frmcnt);
#endif

		len = dump_data(filename, (UINT32)p_ix_addr, save_size);


#if defined(__FREERTOS)
		snprintf(filename, 64, "A:\\DIS\\outBin\\api_main\\alg_%ld_iY_test.raw", frmcnt);
#else
		snprintf(filename, 64, "/mnt/sd/DIS/outBin/api_main/alg_%d_iY_test.raw", frmcnt);
#endif
		
		len = dump_data(filename, (UINT32)p_iy_addr, save_size);

#if defined(__FREERTOS)
		snprintf(filename, 64, "A:\\DIS\\outBin\\api_main\\alg_%ld_valid_test.raw", frmcnt);
#else
		snprintf(filename, 64, "/mnt/sd/DIS/outBin/api_main/alg_%d_valid_test.raw", frmcnt);
#endif
		
		len = dump_data(filename, (UINT32)p_valid_addr, save_size);


		frmcnt++;
	}

#if defined(__FREERTOS)
#else
	len = vos_mem_release_from_cma(vos_mem_id);
	if (len != 0) {
		nvt_dbg(ERR, "failed in release buffer\n");
		return -1;
	}
#endif

	return len;
	
}
#endif


int nvt_dis_api_write_reg(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -1;
	}

#if defined(__FREERTOS)
	if ((reg_addr = strtoul(p_argv[0], NULL, 16)) == 0) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", p_argv[0]);
		return -1;
	}

	if ((reg_value = strtoul(p_argv[1], NULL, 16)) == 0){
		nvt_dbg(ERR, "invalid rag value:%s\n", p_argv[1]);
		return -1;

	}
#else
	if (kstrtoul(p_argv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", p_argv[0]);
		return -1;
	}

	if (kstrtoul(p_argv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", p_argv[1]);
		return -1;

	}
#endif

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_dis_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_dis_api_read_reg(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -1;
	}

#if defined(__FREERTOS)
	if ((reg_addr = strtoul(p_argv[0], NULL, 16)) == 0) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", p_argv[0]);
		return -1;
	}
#else
	if (kstrtoul(p_argv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", p_argv[0]);
		return -1;
	}
#endif

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_dis_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

void emu_disMain(UINT32 emu_id)
{
#if (EMU_DIS == ENABLE)
	CHAR ch=0;

	nvt_dbg(ERR,"DIS Test Program\r\n");

	while (1) {
		nvt_dbg(ERR,"DIS test menu\r\n\r\n");
		nvt_dbg(ERR,"1. test dis\r\n");
		nvt_dbg(ERR,"z. back\r\n");
		nvt_dbg(ERR,"Input test item: ");

#if defined(__FREERTOS)
		uart_getChar(&ch);
#else
		nvt_dbg(ERR, "Error,TODO....2\r\n");
#endif
		switch (ch) {
		case '1':
#if defined(__FREERTOS)
			nvt_dis_api_write_pattern(0,0,0);
#else
			nvt_dbg(ERR, "Error,TODO....\r\n");
#endif
			break;
		case '2':
			break;
		case '3':
			break;
		case '4':
			break;
		case '5':
			break;
		case 'z':
			return;
		default:
			break;
		}
	}

#else
#endif
}

