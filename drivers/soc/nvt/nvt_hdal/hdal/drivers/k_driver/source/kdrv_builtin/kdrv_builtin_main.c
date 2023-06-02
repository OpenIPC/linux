#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <kwrap/verinfo.h>
#include "kwrap/debug.h"
#include <nvtmpp_init.h>
#include <kdrv_builtin.h>
#include "sie_init.h"
#include "isp_builtin.h"
#include "nvt_vdocdc_main.h"
#include "kdrv_ipp_builtin.h"
#include "bridge.h"
#include "vdoenc_builtin.h"
#include "audcap_builtin.h"
#include "kflow_ai_builtin.h"

extern int nvt_ime_builtin_module_init(void);
extern int nvt_ime_builtin_module_exit(void);
extern int nvt_sie_module_init(void);
extern int nvt_sie_module_exit(void);
extern int nvt_eac_module_init(void);
extern int nvt_eac_module_exit(void);
extern int nvt_dai_module_init(void);
extern int nvt_dai_module_exit(void);
extern int nvt_jpg_module_init(void);
extern int nvt_jpg_module_exit(void);
extern int nvt_ai_module_init(void);
extern void nvt_ai_module_exit(void);

extern void nvt_bootts_add_ts(char *name);

int __init kdrv_builtin_init(void)
{
	int ret = 0;

    printk("%s init module ==========\n",__func__);
    nvt_bootts_add_ts("builtin");

	if (kdrv_builtin_is_fastboot()) {
		kdrv_bridge_map(); //map bridge memory for all built-in
	}
	if (kdrv_builtin_is_fastboot()) {
		nvtmpp_init_mmz();
	}
	if ((ret=nvt_ime_builtin_module_init()) !=0 ) {
		return ret;
	}

	if ((ret=nvt_sie_module_init()) !=0 ) {
		return ret;
	}

	if ((ret=nvt_vdocdc_module_init()) !=0 ) {
		return ret;
	}

	if ((ret=nvt_eac_module_init()) !=0 ) {
		return ret;
	}

	if ((ret=nvt_dai_module_init()) !=0 ) {
		return ret;
	}

	if ((ret=nvt_jpg_module_init()) !=0 ) {
		return ret;
	}

	if (kdrv_builtin_is_fastboot()) {
		if ((ret=nvt_ai_module_init()) !=0 ) {
			return ret;
		}
	}
	if (kdrv_builtin_is_fastboot()) {
		NVTMPP_FASTBOOT_MEM_S *p_fastboot_mem;
		KDRV_IPP_BUILTIN_INIT_INFO ipp_init_info;
		KDRV_IPP_BUILTIN_IMG_INFO img_info[KDRV_IPP_BUILTIN_PATH_ID_MAX] = {0};
		VDOENC_BUILTIN_INIT_INFO venc_info[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
		SIE_BUILTIN_INIT_INFO sie_init_info;
		AUDCAP_BUILTIN_INIT_INFO acap_info;
		UINT32 i;
		#if (NVT_FAST_AI_FLOW)
		KFLOW_AI_BUILTIN_INIT_INFO ai_info;
		UINT32 pool_id;
		#endif
		unsigned int sensor_name_1 = 0, sensor_name_2 = 0;
		UINT32 vprc_src_dev = 0, vprc_src_path = 0;

		// get fastboot memory from dts
		if (nvtmpp_parse_fastboot_mem_dt() < 0) {
			return ret;
		}
		p_fastboot_mem = nvtmpp_get_fastboot_mem();

		// get sensor 1
		kdrv_bridge_get_tag(SENSOR_PRESET_NAME, &sensor_name_1);
		// get sensor 2
		kdrv_bridge_get_tag(SENSOR_PRESET_NAME_2, &sensor_name_2);

		// init isp
		#if (NVT_FAST_ISP_FLOW)
		isp_builtin_init();
		#else
		#if !defined(CONFIG_NVT_FAST_ISP_FLOW)
		printk("ERR: CONFIG_NVT_FAST_ISP_FLOW not define!!! \n");
		#endif
		#endif
		// init ipp
		memset((void *)&ipp_init_info, 0, sizeof(KDRV_IPP_BUILTIN_INIT_INFO));

		ipp_init_info.hdl_num = 2;
		ipp_init_info.hdl_info[0].name = "vdoprc0";
		ipp_init_info.hdl_info[0].src_sie_id_bit = 0x1;
		ipp_init_info.hdl_info[0].ctrl_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VPRC_CTRL].addr;
		ipp_init_info.hdl_info[0].ctrl_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VPRC_CTRL].size;
		ipp_init_info.hdl_info[0].shdr_ring_buf_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VCAP_SHDR_BUF].addr;
		ipp_init_info.hdl_info[0].shdr_ring_buf_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VCAP_SHDR_BUF].size;

		ipp_init_info.hdl_info[1].name = "vdoprc1";
		ipp_init_info.hdl_info[1].src_sie_id_bit = 0x2;
		ipp_init_info.hdl_info[1].ctrl_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VPRC1_CTRL].addr;
		ipp_init_info.hdl_info[1].ctrl_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VPRC1_CTRL].size;
		ipp_init_info.hdl_info[1].shdr_ring_buf_addr = 0;
		ipp_init_info.hdl_info[1].shdr_ring_buf_size = 0;

		if (sensor_name_1 != 0xFFFFFFFF) {
			ipp_init_info.valid_src_id_bit |= 0x1;
		}
		if (sensor_name_2 != 0xFFFFFFFF) {
			ipp_init_info.valid_src_id_bit |= 0x2;
		}
#if 0 //tmp for migration
		ipp_init_info.hdl_num = 1;
		//ipp_init_info.hdl_info[0].name = "builtin_vprc1";
		ipp_init_info.hdl_info[0].src_sie_id_bit = 0x1;
		for (path_id = 0; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
			if (fastboot_mem.vprc_blk_cnt[path_id] >= FBOOT_VPRC_COMNBLK_MIN_CNT) {
				ipp_init_info.hdl_info[0].blk_addr[path_id][0] = fastboot_mem.vprc_blk[path_id][0].addr;
				ipp_init_info.hdl_info[0].blk_addr[path_id][1] = fastboot_mem.vprc_blk[path_id][1].addr;
				ipp_init_info.hdl_info[0].blk_size[path_id][0] = fastboot_mem.vprc_blk[path_id][0].size;
				ipp_init_info.hdl_info[0].blk_size[path_id][1] = fastboot_mem.vprc_blk[path_id][1].size;
			}
		}
		ipp_init_info.hdl_info[0].ctrl_blk_addr = fastboot_mem.pv_pools[FBOOT_POOL_VPRC_CTRL].addr;
		ipp_init_info.hdl_info[0].ctrl_blk_size = fastboot_mem.pv_pools[FBOOT_POOL_VPRC_CTRL].size;
		ipp_init_info.hdl_info[0].shdr_ring_buf_addr = fastboot_mem.pv_pools[FBOOT_POOL_VCAP_SHDR_BUF].addr;
		ipp_init_info.hdl_info[0].shdr_ring_buf_size = fastboot_mem.pv_pools[FBOOT_POOL_VCAP_SHDR_BUF].size;
#endif
		kdrv_ipp_builtin_init(&ipp_init_info);

		// init codec
		for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
			ret = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_ENC_EN);
			if (ret == 1) {
				vprc_src_dev = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_DEV);
				vprc_src_path = VdoEnc_builtin_get_dtsi_param(i, BUILTIN_VDOENC_DTSI_PARAM_SRC_VPRC_PATH);
				if (vprc_src_dev == 0) {
					img_info[vprc_src_path] = kdrv_ipp_builtin_get_path_info("vdoprc0", vprc_src_path);
				} else if (vprc_src_dev == 1) {
					img_info[vprc_src_path] = kdrv_ipp_builtin_get_path_info("vdoprc1", vprc_src_path);
				}
				venc_info[i].width = img_info[vprc_src_path].size.w;
				venc_info[i].height = img_info[vprc_src_path].size.h;
				venc_info[i].fmt = (UINT32)img_info[vprc_src_path].fmt;
			}
		}
		venc_info[BUILTIN_VDOENC_PATH_ID_0].max_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_0_MAX].addr;
		venc_info[BUILTIN_VDOENC_PATH_ID_0].max_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_0_MAX].size;
		venc_info[BUILTIN_VDOENC_PATH_ID_1].max_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_1_MAX].addr;
		venc_info[BUILTIN_VDOENC_PATH_ID_1].max_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_1_MAX].size;
		venc_info[BUILTIN_VDOENC_PATH_ID_2].max_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_2_MAX].addr;
		venc_info[BUILTIN_VDOENC_PATH_ID_2].max_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_2_MAX].size;
		venc_info[BUILTIN_VDOENC_PATH_ID_3].max_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_3_MAX].addr;
		venc_info[BUILTIN_VDOENC_PATH_ID_3].max_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_3_MAX].size;
		venc_info[BUILTIN_VDOENC_PATH_ID_4].max_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_4_MAX].addr;
		venc_info[BUILTIN_VDOENC_PATH_ID_4].max_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_4_MAX].size;
		venc_info[BUILTIN_VDOENC_PATH_ID_5].max_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_5_MAX].addr;
		venc_info[BUILTIN_VDOENC_PATH_ID_5].max_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VENC_5_MAX].size;

		if ((ret=VdoEnc_builtin_init(venc_info)) !=0 ) {
			return ret;
		}

		// init sie
		sie_init_info.ring_buf_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_VCAP_SHDR_BUF].addr;
		sie_init_info.ring_buf_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_VCAP_SHDR_BUF].size;

		sie_init_info.sie_id_bit = 0x0;
		if (sensor_name_1 != 0xFFFFFFFF) {
			sie_init_info.sie_id_bit = 0x1;
		}
		// get sensor 2
		kdrv_bridge_get_tag(SENSOR_PRESET_NAME_2, &sensor_name_2);
		if (sensor_name_2 != 0xFFFFFFFF) {
			unsigned int isp_sensor_path_2 = 0;
			kdrv_bridge_get_tag(ISP_PATH_2, &isp_sensor_path_2);
			sie_init_info.sie_id_bit |= 1 << isp_sensor_path_2;
		}
		sie_builtin_init(&sie_init_info);

		acap_info.ctrl_blk_addr = p_fastboot_mem->pv_pools[FBOOT_POOL_ACAP_CTRL].addr;
		acap_info.ctrl_blk_size = p_fastboot_mem->pv_pools[FBOOT_POOL_ACAP_CTRL].size;
		audcap_builtin_init(&acap_info);

		// init ai
		#if (NVT_FAST_AI_FLOW)
		for (pool_id = 0; pool_id < FBOOT_MISC_CPOOL_MAX; pool_id++) {
			if (p_fastboot_mem->misc_cpool[pool_id].blk_cnt > FBOOT_MISC_CPOOLBCK_MAX_CNT) {
				p_fastboot_mem->misc_cpool[pool_id].blk_cnt = FBOOT_MISC_CPOOLBCK_MAX_CNT;
			}
			for (i = 0; i < p_fastboot_mem->misc_cpool[pool_id].blk_cnt; i++) {
				// check pool_type(0x6a) for CNN_POOL
				if (p_fastboot_mem->misc_cpool[pool_id].pool_type == 0x6a &&
					p_fastboot_mem->misc_cpool[pool_id].blk[i].addr != 0) {
					ai_info.blk_addr = p_fastboot_mem->misc_cpool[pool_id].blk[i].addr;
					ai_info.blk_size = p_fastboot_mem->misc_cpool[pool_id].blk[i].size;
				}
			}
		}
		kflow_ai_builtin_init(&ai_info);
		#endif
	}

	if (kdrv_builtin_is_fastboot()) {
		kdrv_bridge_unmap();
	}
    printk("%s init module ==========\n",__func__);
    nvt_bootts_add_ts("builtin");

	return ret;
}

void __exit kdrv_builtin_exit(void)
{
	
	nvt_ime_builtin_module_exit();
	nvt_sie_module_exit();
	nvt_dai_module_exit();
	nvt_eac_module_exit();
	nvt_jpg_module_exit();
	nvt_vdocdc_module_exit();
	if (kdrv_builtin_is_fastboot()) {
		nvt_ai_module_exit();
	}
	#if (NVT_FAST_ISP_FLOW)
	isp_builtin_uninit();
	#endif

	VdoEnc_BuiltIn_TskStop_H26X();
	VdoEnc_BuiltIn_TskStop_JPEG();
	VdoEnc_BuiltIn_Uninstall_ID();
	#if (NVT_FAST_AI_FLOW)
	kflow_ai_builtin_exit();
	#endif
	if (kdrv_builtin_is_fastboot()) {
		nvtmpp_exit_mmz();
	}
}

#ifdef __KDRV_BUILTIN_USE_INITCALL
fs_initcall(kdrv_builtin_init);
__exitcall(kdrv_builtin_exit);
#else
module_init(kdrv_builtin_init);
module_exit(kdrv_builtin_exit);
#endif

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv built-in driver");
MODULE_LICENSE("GPL");

#else
#endif
