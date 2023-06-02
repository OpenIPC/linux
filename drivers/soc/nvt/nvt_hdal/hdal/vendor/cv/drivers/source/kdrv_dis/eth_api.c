//=============================================================================
// include
//=============================================================================
#include "dis_platform.h"
#include "eth_api.h"
#include "eth_lib.h"
#include "kdrv_eth.h"


//=============================================================================
// define
//=============================================================================
#ifdef SIE_ETH
extern CTL_SIE_ISP_ETH_PARAM g_eth_param;
extern CTL_SIE_ISP_ETH_PARAM g_eth_get_param;
#else
extern CTL_IPP_ISP_ETH g_eth_param;
extern CTL_IPP_ISP_ETH g_rdy_eth;
#endif
extern ETH_IN_BUFFER_INFO g_eth_in_buffer;
extern UINT32             g_eth_out_lofs;
extern UINT32             g_eth_frame_cnt;



#ifdef DEBUG_KDRV_ETH
int exam_dis_eth_ipe_cb(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	UINT32 i = 0;
	#ifdef SIE_ETH
	static CTL_SIE_ISP_ETH_PARAM eth_set_param = {0,0,100,{0,0,100,100},{ENABLE, 0, 0, 0, 0, 28,48,128},0};
	#else
    static CTL_IPP_ISP_ETH eth_set_param = {ENABLE, 0, 0, 0, 0, {0, 0}, {0, 0}, 28, 48, 128, {0, 0}};
	#endif
	UINT32 buf_size = 0;	
	struct nvt_fmem_mem_info_t eth_bufinfo[ETH_BUFFER_NUM] ={0};
	void *hdl_buf = NULL;
	UINT32 out_lofs;

	int len = 0;
	g_eth_param = eth_set_param;

	for (i = 0; i < ETH_BUFFER_NUM; i++) {
		#ifdef SIE_ETH
		buf_size = dis_eth_util_ethsize(1920, 1080, g_eth_param.eth_info.out_bit_sel, g_eth_param.eth_info.out_sel, &out_lofs);
		#else
		buf_size = dis_eth_util_ethsize(1920, 1080, g_eth_param.out_bit_sel, g_eth_param.out_sel, &out_lofs);
		#endif
		
		len = nvt_fmem_mem_info_init(&eth_bufinfo[i], NVT_FMEM_ALLOC_CACHE, buf_size, NULL);
		if (len >= 0) {
			hdl_buf = nvtmem_alloc_buffer(&eth_bufinfo[i]);
		}
		else {
			DBG_ERR("get eth buffer fail\n");
			return 0;
		}
		g_eth_in_buffer.buf_addr[i] = (UINT32)eth_bufinfo[i].vaddr;
		g_eth_in_buffer.buf_size = eth_bufinfo[i].size;
	}
    DBG_IND("eth_in_info: enable = %d, h_out_sel = %d, v_out_sel = %d, out_bit_sel = %d, out_sel = %d,th_high = %d, th_mid = %d, th_low = %d\n\r ",eth_set_param.enable, eth_set_param.h_out_sel, eth_set_param.v_out_sel, eth_set_param.out_bit_sel, eth_set_param.out_sel, eth_set_param.th_high, eth_set_param.th_mid, eth_set_param.th_low);
	DBG_IND("eth_in_addr: uiInAdd0 = 0x%x, uiInAdd1 = 0x%x, buf_size = 0x%x\n\r ",g_eth_in_buffer.buf_addr[0],g_eth_in_buffer.buf_addr[1], g_eth_in_buffer.buf_size);

	dis_eth_api_reg();

	return 0;
}
int exam_dis_eth_ipe_cb_un(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{

	dis_eth_api_unreg();

	return 0;
}
int vendor_dis_eth_ipe_cb(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	UINT32 i = 0;
    UINT32 out_lofs;
	KDRV_ETH_IN_PARAM                eth_input_info;
	KDRV_ETH_IN_BUFFER_INFO          eth_in_addr;

	UINT32 buf_size = 0;	
	struct nvt_fmem_mem_info_t eth_bufinfo[ETH_BUFFER_NUM] ={0};
	void *hdl_buf = NULL;
	int len = 0;
	INT32 chip = 1, engine = KDRV_CV_ENGINE_DIS, channel = 0;
	INT32 id = KDRV_DEV_ID(chip, engine, channel);

	for (i = 0; i < ETH_BUFFER_NUM; i++) {
		buf_size = dis_eth_util_ethsize(1920, 1080, 0, 0, &out_lofs);
		len = nvt_fmem_mem_info_init(&eth_bufinfo[i], NVT_FMEM_ALLOC_CACHE, buf_size, NULL);
		if (len >= 0) {
			hdl_buf = nvtmem_alloc_buffer(&eth_bufinfo[i]);
		}
		else {
			DBG_ERR("get eth buffer fail\n");
			return 0;
		}
	}

	eth_input_info.enable = 1;
	eth_input_info.h_out_sel = 0;
	eth_input_info.v_out_sel = 0;
	eth_input_info.out_bit_sel = 0;
	eth_input_info.out_sel = 0;

	eth_input_info.th_low  = 28;
	eth_input_info.th_mid  = 48;
	eth_input_info.th_high = 128;
	
	eth_in_addr.ui_inadd = (UINT32)eth_bufinfo[0].vaddr;
	eth_in_addr.buf_size = eth_bufinfo[0].size;

    DBG_ERR("eth_in_info: enable = %d, h_out_sel = %d, v_out_sel = %d, out_bit_sel = %d, out_sel = %d,th_high = %d, th_mid = %d, th_low = %d\n\r ",eth_input_info.enable, eth_input_info.h_out_sel, eth_input_info.v_out_sel, eth_input_info.out_bit_sel, eth_input_info.out_sel, eth_input_info.th_high, eth_input_info.th_mid, eth_input_info.th_low);
	DBG_ERR("eth_in_addr: uiInAdd0 = 0x%x, uiInAdd1 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",eth_in_addr.ui_inadd, (UINT32)eth_bufinfo[1].vaddr, eth_in_addr.buf_size, eth_in_addr.frame_cnt);
	

	/******** dis d2d flow *********/
	/*set call back funcion*/
	
	kdrv_dis_set(id, KDRV_DIS_ETH_BUFFER_IN, (void *)&eth_in_addr);
	kdrv_dis_set(id, KDRV_DIS_ETH_PARAM_IN, (void *)&eth_input_info);

	/******** dis d2d flow *********/	
	return 0;
}
int vendor_dis_eth_ipe_cb_un(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	KDRV_ETH_IN_PARAM                eth_input_info = {0};

	INT32 chip = 1, engine = KDRV_CV_ENGINE_DIS, channel = 0;
	INT32 id = KDRV_DEV_ID(chip, engine, channel);

	/******** dis d2d flow *********/
	/*set call back funcion*/
	kdrv_dis_set(id, KDRV_DIS_ETH_PARAM_IN, (void *)&eth_input_info);

	/******** dis d2d flow *********/	
	return 0;
}
int vendor_dis_eth_get(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv)
{
	KDRV_ETH_OUT_PARAM               eth_out_info = {0};
	KDRV_ETH_IN_BUFFER_INFO          eth_out_addr = {0};
	KDRV_ETH_IN_PARAM                eth_in_info  = {0};
	KDRV_DIS_OPENCFG dis_open_obj;
	
	INT32 chip = 2, engine = KDRV_CV_ENGINE_DIS, channel = 0;
    INT32 id = KDRV_DEV_ID(chip, engine, channel);
	#ifdef DUMP_ETH
	static char filename[100];
	struct file *p_fp;
	mm_segment_t old_fs;
	#endif
	/******** dis d2d flow *********/
	DBG_IND("dis d2d flow start\n");
	/*open*/
	dis_open_obj.dis_clock_sel = 480;
    kdrv_dis_set(id, KDRV_DIS_PARAM_OPENCFG, (void *)&dis_open_obj);
	if(kdrv_dis_open(chip, engine) != 0) {
        DBG_ERR("set opencfg fail!\r\n");
		return -EFAULT;
	}
	DBG_ERR("dis opened\r\n");

	kdrv_dis_get(id, KDRV_DIS_ETH_BUFFER_IN, (void *)&eth_out_addr);
	kdrv_dis_get(id, KDRV_DIS_ETH_PARAM_IN, (void *)&eth_in_info);
	kdrv_dis_get(id, KDRV_DIS_ETH_PARAM_OUT, (void *)&eth_out_info);

    DBG_ERR("eth_in_info: enable = %d, h_out_sel = %d, v_out_sel = %d, out_bit_sel = %d, out_sel = %d,th_high = %d, th_mid = %d, th_low = %d\n\r ",eth_in_info.enable, eth_in_info.h_out_sel, eth_in_info.v_out_sel, eth_in_info.out_bit_sel, eth_in_info.out_sel, eth_in_info.th_high, eth_in_info.th_mid, eth_in_info.th_low);
	DBG_ERR("eth_out_addr: uiInAdd0 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",eth_out_addr.ui_inadd, eth_out_addr.buf_size, eth_out_addr.frame_cnt);
	DBG_ERR("eth_out_info: out_lofs = %d, w = %d, h = %d\n\r ",eth_out_info.out_lofs, eth_out_info.out_size.w, eth_out_info.out_size.h);

	/*close*/
	kdrv_dis_close(chip,engine);
    DBG_IND("dis d2d flow end\n");
	/******** dis d2d flow *********/
	#ifdef DUMP_ETH
	sprintf(filename, "/mnt/sd/DIS/outBin/IPE_out/alg_eth_%d_0x%x.raw", eth_out_addr.frame_cnt, eth_out_addr.ui_inadd);
	p_fp = filp_open(filename, O_CREAT|O_WRONLY|O_SYNC , 0);
	if (IS_ERR_OR_NULL(p_fp)) {
		DBG_ERR("failed in file open:%s\n", filename);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	vfs_write(p_fp, (char*)eth_out_addr.ui_inadd, eth_out_addr.buf_size, &p_fp->f_pos);
	filp_close(p_fp, NULL);
	set_fs(old_fs);		
	#endif
	return 0;
}
#endif
