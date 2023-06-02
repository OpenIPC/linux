/**
	@brief Source file of kflow_ai_builtin.

	@file kflow_ai_builtin.c

	@ingroup kdrv_builtin

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/

#include <linux/of_device.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <asm/neon.h>
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include "kwrap/consts.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/cpu.h"
#include "kwrap/perf.h"
#include "ai_ioctl.h" //for AI_DRV_OPENCFG
#include "kdrv_ai.h"
#include "kflow_ai_builtin.h"
#include "kflow_ai_builtin_api.h"
#include "nvtmpp_init.h"
#ifdef CONFIG_MTD_SPINOR
#include <plat/nvt_flash.h>
#endif

// debug
#define DEBUG_AI_OUTBUF     0 // dump output buffer
#define DEBUG_AI_DET_RSLT   0 // detection result
#define DEBUG_AI_FLOW_MSG   0 // flow message

// perf
#define AI_PERF_TEST        0

// unit test
#define TEST_GET_QUE        0
#if TEST_GET_QUE
static UINT32 proc_net_cnt = 0;
#endif

#if DEBUG_AI_OUTBUF
#include "kwrap/file.h"
#define MAX_FILENAME_LEN    128
#endif

/**** TODO *****/
///> 1. [TODO] about _kflow_ai_builtin_map_mem()
//             maybe we could add a pa in struct, and there is no need to deal with the va mapping.

#define FLG_KFLOW_AI_BUILTIN_IDLE         	FLGPTN_BIT(0) //0x00000001
#define FLG_KFLOW_AI_BUILTIN_PROC_NET      	FLGPTN_BIT(1) //0x00000002
#define FLG_KFLOW_AI_BUILTIN_STOP         	FLGPTN_BIT(2) //0x00000004
#define FLG_KFLOW_AI_BUILTIN_STOP_DONE    	FLGPTN_BIT(3) //0x00000008

#define AI_FASTBOOT_MAP_VA(addr, old_base, new_base)  (new_base + (addr - old_base)) // real_va = new_base + (offset)

extern void nvt_bootts_add_ts(char *name);
/*-----------------------------------------------------------------------------*/
/* global Variables                                                            */
/*-----------------------------------------------------------------------------*/
ID            FLG_ID_KFLOW_AI_BUILTIN = 0;
THREAD_HANDLE KFLOW_AI_BUILTIN_TSK_ID = 0;
SEM_HANDLE    KFLOW_AI_BUILTIN_IN_SEM_ID = {0};
SEM_HANDLE    KFLOW_AI_BUILTIN_RSLT_SEM_ID = {0};
SEM_HANDLE    KFLOW_AI_BUILTIN_COMM_SEM_ID = {0};

static UINT32 g_new_base_va = 0;
static UINT32 g_old_base_va = 0;
static UINT32 g_new_io_base_va = 0;
static UINT32 g_old_io_base_va = 0;
static INT32  g_proc_period = 0;
KFLOW_AI_BUILTIN_OBJ kflow_ai_bt_obj = {0};
KFLOW_AI_BUILTIN_MEM_PARM rslt_buf[KFLOW_AI_BUILTIN_RSLTQ_MAX] = {0};
static NN_DATA g_fist_layer_imem[NN_SUPPORT_NET_MAX][NN_IMEM_NUM] = {0};
static PD_MEM pd_mem;
INT32 g_backbone_out_indexs[3][2] = {{171, 172}, {189, 190}, {228, 233}}; // process index for pdcnn

/*-----------------------------------------------------------------------------*/
/* External Functions                                                          */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
#if DEBUG_AI_OUTBUF
static int _kflow_ai_builtin_write_file(char *filename, void *data, size_t count, unsigned long long *offset)
{
	int ret = 0;
	int fd;

	fd = vos_file_open(filename, O_RDWR|O_CREAT, 0666);

	if ((VOS_FILE)(-1) == fd) {
		DBG_DUMP("%s fails to open file %s\n", __func__, filename);
		ret = -1;
		goto exit;
	}

	ret = vos_file_write(fd, (void *)data, count);

	if (ret <= 0) {
		DBG_DUMP("%s: Fail to write file %s(errno = %d)!\n", __func__, filename, ret);
		ret = -1;
		goto exit;
	}

exit:
	if ((VOS_FILE)(-1) != fd)
		vos_file_close(fd);

	return ret;
}

static void _kflow_ai_builtin_dump_out_buf(NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_idx, BOOL dump_to_file)
{
	char filename[MAX_FILENAME_LEN] = {0};
	char engname[16] = {0};
#if !CNN_25_MATLAB
	NN_DATA *p_sao;
	UINT32 omem_cnt;
	//NN_GEN_MODE_CTRL *p_mctrl = 0;
#else
	NN_IOMEM *p_io_mem = 0;
#endif
	UINT32 out_id = 0;

	if (p_mctrl == NULL) {
		DBG_ERR("p_mctrl is NULL\n");
		return;
	}

#if CNN_25_MATLAB
	p_io_mem = (NN_IOMEM *)p_job->p_io_info;
#else
	//p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
#endif

#if CNN_25_MATLAB
	for (out_id = 0; out_id < 3; out_id++) {
		if (p_io_mem->SAO[out_id].size > 0) {
			snprintf(filename, MAX_FILENAME_LEN-1, "/mnt/sd/ai_dumpbuf/%s_%u_OUT%u.bin", ENG_NAME, (unsigned int)proc_idx, (unsigned int)out_id);
			DBG_DUMP("write file: %s\n", filename);
			_kflow_cnn_write_file(filename, (void *)p_io_mem->SAO[out_id].va, p_io_mem->SAO[out_id].size, 0);
		}
	}
#else

	omem_cnt = p_mctrl->iomem.omem_cnt;
	p_sao    = (NN_DATA*)p_mctrl->iomem.omem_addr;

	if (p_mctrl->eng == NN_GEN_ENG_CNN || p_mctrl->eng == NN_GEN_ENG_CNN2) {
		snprintf(engname, MAX_FILENAME_LEN-1, "CNN");
	} else if (p_mctrl->eng == NN_GEN_ENG_NUE) {
		snprintf(engname, MAX_FILENAME_LEN-1, "NUE");
	} else if (p_mctrl->eng == NN_GEN_ENG_NUE2) {
		snprintf(engname, MAX_FILENAME_LEN-1, "NUE2");
	} else {
		DBG_ERR("invalid eng(%d), proc_idx(%u)\r\n", p_mctrl->eng, proc_idx);
	}

	for (out_id = 0; out_id < omem_cnt; out_id++) {
		if (p_sao[out_id].size > 0) {
			if (dump_to_file) {
				DBG_DUMP("dump to_file: proc_idx(%d)\n", proc_idx);
				snprintf(filename, MAX_FILENAME_LEN-1, "/mnt/sd/ai_dumpbuf/%s_%u_OUT%u.bin", engname, (unsigned int)proc_idx, (unsigned int)out_id);
				DBG_DUMP("write file: %s\n", filename);

				vos_cpu_dcache_sync(p_sao[out_id].va, ALIGN_CEIL_32(p_sao[out_id].size), VOS_DMA_FROM_DEVICE);

				_kflow_ai_builtin_write_file(filename, (void *)p_sao[out_id].va, p_sao[out_id].size, 0);
			} else {
				int i = 0;
				UINT8 *ptr;
				UINT32 va;

				va = (UINT32)ioremap(p_sao[out_id].pa, PAGE_ALIGN(p_sao[out_id].size));
				vos_cpu_dcache_sync(va, ALIGN_CEIL_32(p_sao[out_id].size), VOS_DMA_FROM_DEVICE);
				ptr = (UINT8 *)va;

				DBG_DUMP("dump to_console: proc_idx(%d) out_id(%d) ptr(0x%x) pa(0x%x)\n", proc_idx, out_id, (unsigned int)ptr, p_sao[out_id].pa);
				for (i = 0; i < 4; i++) {
					printk("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
						*(ptr+i), *(ptr+i+1), *(ptr+i+2), *(ptr+i+3), *(ptr+i+4), *(ptr+i+5), *(ptr+i+6), *(ptr+i+7),
						*(ptr+i+8), *(ptr+i+9), *(ptr+i+10), *(ptr+i+11), *(ptr+i+12), *(ptr+i+13), *(ptr+i+14), *(ptr+i+15));
				}
				iounmap((void *)va);
			}
		}
	}
#endif
}
#endif

static ER _kflow_ai_builtin_get_fastboot_dt(KFLOW_AI_BUILTIN_MAP_MEM_PARM *p_mem)
{
	char *path = "/fastboot/ai_buf";
	char *path2 = "/fastboot/mem_in_kerl";
	char *path3 = "/fastboot/user_set";
	struct device_node* dt_node;
	char node_name[30];
	KFLOW_AI_BUILTIN_MEM dt_mem = {0};
	KFLOW_AI_BUILTIN_KERL_START_MEM kerl_start = {0};

	memset((void *)p_mem, 0, sizeof(KFLOW_AI_BUILTIN_MAP_MEM_PARM));
	dt_node = of_find_node_by_path(path);
	if (!dt_node) {
		printk("Failed to find node by path: %s.\r\n", path);
		return E_SYS;
    }
    DBG_IND("[KFLOW_AI_BUILTIN] Found the node for %s.\r\n", path);

    /* kerl parm */
    snprintf(node_name, sizeof(node_name), "kerl_parm");
    if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&dt_mem, 2)) {
		printk("Failed to read kerl_parm\r\n");
		return E_SYS;
	}
	p_mem->kerl_parm.pa = dt_mem.addr;
	p_mem->kerl_parm.va = nvtmpp_sys_pa2va(p_mem->kerl_parm.pa);
	p_mem->kerl_parm.size = dt_mem.size;

	/* user model */
	snprintf(node_name, sizeof(node_name), "user_model");
    if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&dt_mem, 2)) {
		printk("Failed to read user_model\r\n");
		return E_SYS;
	}
	p_mem->user_model.pa = dt_mem.addr;
	p_mem->user_model.va = nvtmpp_sys_pa2va(p_mem->user_model.pa);
	p_mem->user_model.size = dt_mem.size;

	/* pd buff */
	snprintf(node_name, sizeof(node_name), "pd_buff");
    if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&dt_mem, 2)) {
		printk("Failed to read pd_buff\r\n");
		return E_SYS;
	}
	p_mem->pd_buff.pa = dt_mem.addr;
	p_mem->pd_buff.va = nvtmpp_sys_pa2va(p_mem->pd_buff.pa);
	p_mem->pd_buff.size = dt_mem.size;

	/* rslt buff */
	snprintf(node_name, sizeof(node_name), "fboot_rslt");
	if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&dt_mem, 2)) {
		printk("Failed to read pd_buff\r\n");
		return E_SYS;
	}
	p_mem->rslt_buff.pa = dt_mem.addr;
	p_mem->rslt_buff.va = nvtmpp_sys_pa2va(p_mem->rslt_buff.pa);
	p_mem->rslt_buff.size = dt_mem.size;

	/* io buff */
	snprintf(node_name, sizeof(node_name), "io_buff");
    if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&dt_mem, 2)) {
		printk("Failed to read io_buff\r\n");
		return E_SYS;
	}
	p_mem->io_buff.pa = dt_mem.addr;
	p_mem->io_buff.va = nvtmpp_sys_pa2va(p_mem->io_buff.pa);
	p_mem->io_buff.size = dt_mem.size;

	printk("[KFLOW_AI_BUILTIN] get_ai_fastboot_dt ===========\r\n");
	printk("    kerl_parm:  pa(0x%x) va(0x%x) size(0x%x)\r\n", p_mem->kerl_parm.pa, p_mem->kerl_parm.va, p_mem->kerl_parm.size);
	printk("    user_model: pa(0x%x) va(0x%x) size(0x%x)\r\n", p_mem->user_model.pa, p_mem->user_model.va, p_mem->user_model.size);
	printk("    pd_buff:    pa(0x%x) va(0x%x) size(0x%x)\r\n", p_mem->pd_buff.pa, p_mem->pd_buff.va, p_mem->pd_buff.size);
	printk("    fboot_rslt: pa(0x%x) va(0x%x) size(0x%x)\r\n", p_mem->rslt_buff.pa, p_mem->rslt_buff.va, p_mem->rslt_buff.size);
	printk("    io_buff:    pa(0x%x) va(0x%x) size(0x%x)\r\n", p_mem->io_buff.pa, p_mem->io_buff.va, p_mem->io_buff.size);

	/* mem_in_kerl */
	dt_node = of_find_node_by_path(path2);
	if (!dt_node) {
		printk("Failed to find node by path: %s.\r\n", path2);
		return E_SYS;
    }
    DBG_IND("[KFLOW_AI_BUILTIN] Found the node for %s.\r\n", path2);

	snprintf(node_name, sizeof(node_name), "kerl_parm");
    if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&kerl_start, 2)) {
		printk("Failed to read kerl_start\r\n");
		return E_SYS;
	}
	if (p_mem->kerl_parm.pa == kerl_start.pa) {
		g_old_base_va = kerl_start.va;
	} else {
		printk("starting mem not match! kerl_parm.pa(0x%x) kerl_start.pa(0x%x)\r\n", p_mem->kerl_parm.pa, kerl_start.pa);
		return E_SYS;
	}

	snprintf(node_name, sizeof(node_name), "io_buff");
    if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&kerl_start, 2)) {
		printk("Failed to read kerl_start\r\n");
		return E_SYS;
	}
	if (p_mem->io_buff.pa == kerl_start.pa) {
		g_old_io_base_va = kerl_start.va;
	} else {
		printk("starting mem not match! io_buff.pa(0x%x) kerl_start.pa(0x%x)\r\n", p_mem->io_buff.pa, kerl_start.pa);
		return E_SYS;
	}
	printk("mem_in_kerl: g_old_base_va(0x%x) g_old_io_base_va(0x%x)\r\n", g_old_base_va, g_old_io_base_va);

	/* user_set */
	dt_node = of_find_node_by_path(path3);
	if (!dt_node) {
		printk("Failed to find node by path: %s.\r\n", path3);
		return E_SYS;
	}
	DBG_IND("[KFLOW_AI_BUILTIN] Found the node for %s.\r\n", path3);

	snprintf(node_name, sizeof(node_name), "proc_period");
	if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&g_proc_period, 1)) {
		printk("Failed to read proc_period\r\n");
		return E_SYS;
	}
	if (g_proc_period <= 0) {
		g_proc_period = 1; // minimum proc period at least 1
	}

	return E_OK;
}

ER vendor_ais_get_net_info(NN_GEN_NET_INFO *p_info, UINT32 net_addr)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, model_head_size, mctrl_size;
	NN_GEN_MODE_CTRL *p_mctrl;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
#if CNN_528_PSW
	UINT32 *p_id_list;
#endif
	if ((p_info == NULL) || (net_addr == 0)) {
		DBG_ERR("null input...\r\n");
		return E_SYS;
	}

	p_head = (NN_GEN_MODEL_HEAD *)net_addr;
	proc_layer_num = p_head->mode_ctrl_num;
	model_head_size = ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD));
	mctrl_size = ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * proc_layer_num);
	p_mctrl = (NN_GEN_MODE_CTRL *)((UINT32)p_head + model_head_size);
#if CNN_528_PSW
	p_id_list = (UINT32 *)((UINT32)p_mctrl + mctrl_size);
#if CNN_25_MATLAB
	p_io_mem = (NN_IOMEM *)((UINT32)p_id_list + p_head->layer_id_list_size);
#endif
#else
	p_io_mem = (NN_IOMEM *)((UINT32)p_mctrl + mctrl_size);
#endif

	p_info->p_head      = p_head;
	p_info->p_mctrl     = p_mctrl;
#if CNN_25_MATLAB
	p_info->p_io_mem    = p_io_mem;
#endif
#if CNN_528_PSW
	p_info->p_id_list   = p_id_list;
#endif

	return E_OK;
}

ER _kflow_ai_builtin_map_mem(KFLOW_AI_BUILTIN_MAP_MEM_PARM *p_mem)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AI_LL_HEAD *p_ll_head;
	NN_DATA *p_sai, *p_sao;
	UINT32 process_index, proc_layer_num;
	UINT32 imem_cnt, omem_cnt, in_id, out_id;
	ER er = E_OK;

	if ((p_mem == NULL) || (p_mem->kerl_parm.va == 0)) {
		DBG_ERR("invalid memory input\r\n");
		return E_SYS;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("vendor_ais_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num  = p_head->mode_ctrl_num;

	// get new va_base
	g_new_base_va = p_mem->kerl_parm.va;
	g_new_io_base_va = p_mem->io_buff.va;

	// map mctrl and ll_head
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
#if DEBUG_AI_FLOW_MSG
				printk("map mctrl/ll_head, process_index(%d)\n", process_index);
#endif
				// map mctrl va
				p_mctrl[process_index].addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].addr, g_old_base_va, g_new_base_va);

				// map ll_head va
				p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl[process_index].addr;
				p_ll_head->parm_addr = AI_FASTBOOT_MAP_VA(p_ll_head->parm_addr, g_old_base_va, g_new_base_va);
			}
			break;

		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			DBG_ERR("<map mctrl/ll_head> invalid trig_src(%d), proc_idx(%u)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	// map iomem
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			// map layer 0, 1 for normal mode dumpbuf (debug)
			// and map out layer for postprocess by g_backbone_out_indexs
			if (process_index == 0 || process_index == 1 || p_mctrl[process_index].tot_trig_eng_times > 0 || 
				process_index == g_backbone_out_indexs[0][0] || process_index == g_backbone_out_indexs[0][1] ||
				process_index == g_backbone_out_indexs[1][0] || process_index == g_backbone_out_indexs[1][1] ||
				process_index == g_backbone_out_indexs[2][0] || process_index == g_backbone_out_indexs[2][1]) {
#if DEBUG_AI_FLOW_MSG
				printk("map iomem, process_index(%d)\n", process_index);
#endif
				// imem_addr
				p_mctrl[process_index].iomem.imem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.imem_addr, g_old_base_va, g_new_base_va);
				imem_cnt = p_mctrl->iomem.imem_cnt;
				p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;

				// sao_va
				for (in_id = 0; in_id < imem_cnt; in_id++) {
					if (p_sai[in_id].va != 0) {
#if DEBUG_AI_FLOW_MSG
						printk("\tmap sai, in_id(%d)\n", in_id);
#endif
						p_sai[in_id].va = AI_FASTBOOT_MAP_VA(p_sai[in_id].va, g_old_io_base_va, g_new_io_base_va);
					}
				}

				// omem_addr
				p_mctrl[process_index].iomem.omem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.omem_addr, g_old_base_va, g_new_base_va);
				omem_cnt = p_mctrl->iomem.omem_cnt;
				p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;

				// sao_va
				for (out_id = 0; out_id < omem_cnt; out_id++) {
					if (p_sao[out_id].va != 0) {
						if ((UINT32)OUT_BUF_ATTR_GET(&p_mctrl[process_index], NN_OUT_BUF_ATTR_PRESERVE)) {
#if DEBUG_AI_FLOW_MSG
							printk("\tmap sao, out_id(%d)\n", out_id);
#endif
							p_sao[out_id].va = AI_FASTBOOT_MAP_VA(p_sao[out_id].va, g_old_io_base_va, g_new_io_base_va);
						}
					}
				}
			}
			break;

		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			DBG_ERR("<map iomem> invalid trig_src(%d), proc_idx(%u)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	return E_OK;
}

NN_GEN_MODE_CTRL *vendor_ais_get_proclayer(UINT32 layer, KFLOW_AI_BUILTIN_MEM_PARM mem)
{

	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	er = vendor_ais_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return NULL;
	}
	p_mctrl = net_info.p_mctrl;

	return &p_mctrl[layer];
}

ER vendor_ais_create_1st_layer_addr(VENDOR_AIS_IMG_PARM *p_input_info, NN_DATA *p_1st_imem, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;
	NN_DATA *p_sai = p_1st_imem;

	if ((p_input_info == NULL) || (p_1st_imem == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...\r\n");
		return E_SYS;
	}
	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;

	if (p_input_info->pa == 0) {
		DBG_ERR("input source pa = NULL???\n");
		return E_SYS;
	}
	// p_input_info->va is optional, only used if layer 0 is CPU layer
	if ((p_1st_parm->eng == NN_GEN_ENG_CPU) && (p_input_info->va == 0)) {
		DBG_ERR("input source va = NULL??? (layer 0 is CPU layer)!\n");
		return E_SYS;
	}

	if (trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
		const UINT32 img_size = p_input_info->line_ofs * p_input_info->height;
		p_sai[BUF_IN_IDX].pa      = p_input_info->pa;
		if (nn_mode != NN_PREPROC) {
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
			p_sai[BUF_IN_IDX].va = p_input_info->va;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_RGB888_PLANAR) {
			//p_sai[BUF_IN_IDX].size = img_size * 3;
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX1].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX2].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX1].pa   = p_sai[BUF_IN_IDX].pa + (p_sai[BUF_IN_IDX].size);
			p_sai[NUE2_IN_IDX2].pa   = p_sai[NUE2_IN_IDX1].pa + (p_sai[NUE2_IN_IDX1].size);
			p_sai[BUF_IN_IDX].va = p_input_info->va;
			p_sai[NUE2_IN_IDX1].va	 = p_sai[BUF_IN_IDX].va + p_sai[BUF_IN_IDX].size;
            p_sai[NUE2_IN_IDX2].va	 = p_sai[NUE2_IN_IDX1].va + p_sai[NUE2_IN_IDX1].size;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV420) {
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX1].size = (p_input_info->line_ofs * p_input_info->height) >> 1;
			p_sai[NUE2_IN_IDX1].pa   = p_sai[BUF_IN_IDX].pa + (p_sai[BUF_IN_IDX].size);
			p_sai[BUF_IN_IDX].va = p_input_info->va;
			p_sai[NUE2_IN_IDX1].va	 = p_sai[BUF_IN_IDX].va + p_sai[BUF_IN_IDX].size;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			//p_sai[BUF_IN_IDX].size = img_size * 2;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_Y8) {
			//p_sai[BUF_IN_IDX].size = img_size;
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
			p_sai[BUF_IN_IDX].va = p_input_info->va;
		} else {
			p_sai[BUF_IN_IDX].size = img_size;
			p_sai[BUF_IN_IDX].va = p_input_info->va;
		}
	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return E_SYS;
	}

	return E_OK;
}

ER vendor_ais_update_1st_layer_parm(VENDOR_AIS_IMG_PARM *p_input_info, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;

	if ((p_input_info == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...r\n");
		return E_SYS;
	}
	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;

	if (trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
		//========== for first layer linked list mode ==========
		VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)p_1st_parm->addr;
		if (p_head == NULL) {
			printk("vendor_ais_update_1st_layer_parm: p_head NULL!!!\n");
			return E_SYS;
		}

		switch (p_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2:
		case AI_ENG_NUE:
		break;
		case AI_ENG_NUE2: {
			NUE2_LL_PARM *p_parm = (NUE2_LL_PARM*)p_head->parm_addr;
#if (NEW_AI_FLOW == 1)
            //calculate ideal cycle
            p_1st_parm->idea_cycle = p_input_info->width * p_input_info->height;
#endif
			p_parm->size0.bit.width = p_input_info->width;
			p_parm->size0.bit.height = p_input_info->height;
			p_parm->scale0.bit.h_rate = (((p_input_info->width-1)/(p_parm->scale_size.bit.h_scl_size-1))-1);
			p_parm->scale0.bit.v_rate = (((p_input_info->height-1)/(p_parm->scale_size.bit.v_scl_size-1))-1);
			p_parm->scale1.bit.h_sfact = (((p_input_info->width-1)*65536/(p_parm->scale_size.bit.h_scl_size-1)) & 0xffff);
			p_parm->scale1.bit.v_sfact = (((p_input_info->height-1)*65536/(p_parm->scale_size.bit.v_scl_size-1)) & 0xffff);
			switch (p_input_info->fmt) {
			case HD_VIDEO_PXLFMT_YUV420:
#if AI_SUPPORT_MULTI_FMT
#if defined(_BSP_NA51068_)
				// force to assign fmt in ll cmd (for 32x YUV420_NV21)
				{
					UINT64* tmp_cmd = (UINT64*)p_head->parm_addr;
					UINT32 tmp_idx = 0;
					// search the register which contain yuv_mode register
					while (1) {
						if ((tmp_cmd[tmp_idx] >> 63) == 1) {
							if (((tmp_cmd[tmp_idx] >> 32) & 0xFF) == 0x04) {
								// modify the format
								UINT32* p_reg_val = (UINT32*)(&tmp_cmd[tmp_idx]);
								if (p_input_info->fmt_type) {
									p_reg_val[0] = p_reg_val[0] | 0x200000;
								} else {
									p_reg_val[0] = p_reg_val[0] & 0xFFFDFFFFF;
								}
								break;
							} else {
								// find next cmd
								tmp_idx++;
							}
						} else if ((tmp_cmd[tmp_idx] >> 60) == 0) {
							// ll end
							break;
						} else {
							// find next cmd
							tmp_idx++;
						}
					}
				}
#endif
#endif
				p_parm->ilofs[0].bit.ofs = p_input_info->line_ofs;
				p_parm->ilofs[1].bit.ofs = p_input_info->line_ofs;
				//p_parm->in_ofs[2].line_ofs = 0;
				break;
			/*case HD_VIDEO_PXLFMT_YUV422_ONE:
				p_parm->in_fmt = NET_IMG_YUV422;
				break;*/
			case HD_VIDEO_PXLFMT_Y8:
				//p_parm->src_fmt = AI_PREPROC_SRC_YONLY;
				p_parm->ilofs[0].bit.ofs = p_input_info->line_ofs;
				break;
			case HD_VIDEO_PXLFMT_RGB888_PLANAR:
				//p_parm->src_fmt = AI_PREPROC_SRC_RGB;
				p_parm->ilofs[0].bit.ofs = p_input_info->line_ofs;
				p_parm->ilofs[1].bit.ofs = p_input_info->line_ofs;
				p_parm->ilofs[2].bit.ofs = p_input_info->line_ofs;
				break;
			default:
				DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->fmt);
				//p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
				break;
			}
		}
		break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
			break;
		}
		//========== by CCC 191004 ==========
#else
		DBG_ERR("first layer can't be linked list\r\n");
		return E_SYS;
#endif

	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return E_SYS;
	}

	return E_OK;
}

ER vendor_ais_proc_assign_input_addr(UINT32 proc_layer_num, NN_GEN_MODE_CTRL *p_mctrl, NN_DATA *p_imem_ofs)
{
	UINT32 process_index = 0, layer_index = 0;
	NN_DATA *p_sai;

	if ((proc_layer_num == 0) || (p_mctrl == NULL) || (p_imem_ofs == NULL)) {
		DBG_ERR("null input...\r\n");
		return E_SYS;
	}
	p_sai = p_imem_ofs;
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		layer_index = p_mctrl[process_index].layer_index;
		if (layer_index != 0) {
			continue;
		}
		if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
			//========== for first layer linked list mode ==========
			VENDOR_AI_LL_HEAD *p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl[process_index].addr;
			switch (p_ll_head->eng) {
			case AI_ENG_CNN:
			case AI_ENG_CNN2: {
					CNN_LL_PARM *p_parm = (CNN_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr = p_sai[BUF_IN_IDX].pa;
					//p_parm->input[1].bit.addr = p_sai[BUF_IN_IDX].va;
					//p_parm->input[2].bit.addr = p_sai[BUF_IN_IDX].va;
					//p_parm->interm_in.bit.addr = p_sai[NUE_ELT_IDX].pa;
				}
				break;
			case AI_ENG_NUE: {
					NUE_LL_PARM *p_parm = (NUE_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr = p_sai[BUF_IN_IDX].pa;
					//p_parm->elt_in.bit.addr = p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr = p_sai[NUE_ROI_IDX].va;
				}
				break;
			case AI_ENG_NUE2: {
					NUE2_LL_PARM *p_parm = (NUE2_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input[0].bit.addr = p_sai[NUE2_IN_IDX0].pa;
					p_parm->input[1].bit.addr = p_sai[NUE2_IN_IDX1].pa;
					p_parm->input[2].bit.addr = p_sai[NUE2_IN_IDX2].pa;
					//p_parm->elt_in.bit.addr = p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr = p_sai[NUE_ROI_IDX].va;
				}
				break;
			default:
				DBG_ERR("unknown engine type(%d) in ll\r\n", (int)p_ll_head->eng);
				break;
			}
//========== by CCC 191004 ==========
#else
			DBG_ERR("first layer can't be linked list\r\n");
			return E_SYS;
#endif
		} else {
			DBG_ERR("unknown first layer trigger source: %d\r\n", (int)p_mctrl[process_index].trig_src);
			return E_SYS;
		}
	}

	return E_OK;
}

ER vendor_ais_proc_input_init(NN_MODE mode, NN_DATA *p_1st_imem, KFLOW_AI_BUILTIN_MEM_PARM mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_imem_ofs, *p_model_data;
	NN_GEN_NET_INFO net_info = {0};
	ER ret = E_OK;

	ret = vendor_ais_get_net_info(&net_info, mem.va);
	if (ret != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	if (p_mctrl[0].mode != mode) {
		DBG_ERR("input mode(%d) isn't matched with first layer model mode(%d)\r\n", (int)mode, (int)p_mctrl[0].mode);
		return E_SYS;
	}

	///< update first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[0].iomem.imem_addr;
	p_imem_ofs = p_1st_imem;
	imem_cnt = p_mctrl[0].iomem.imem_cnt;
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("first layer imem count (%d) should be < %d\r\n", (int)imem_cnt, NN_IMEM_NUM);
		return E_SYS;
	}

	for (idx = 0; idx < imem_cnt; idx++) {
		if (p_imem_ofs[idx].size == 0) {
			continue;
		}
		memcpy(&p_model_data[idx], &p_imem_ofs[idx], sizeof(NN_DATA));
	}

	///< update first layer address of parameter
	vendor_ais_proc_assign_input_addr(proc_layer_num, p_mctrl, p_imem_ofs);

	return E_OK;
}

ER vendor_ais_net_input_init(VENDOR_AIS_IMG_PARM *p_input_info, UINT32 net_id)
{
		NN_GEN_MODE_CTRL *p_1st_mctrl;
		NN_MODE nn_mode;
		const UINT32 first_layer = 0;
		NN_DATA *p_1st_imem = g_fist_layer_imem[net_id];
		KFLOW_AI_BUILTIN_MEM_PARM *p_mem_parm = &kflow_ai_bt_obj.mem_parm.kerl_parm;

		SEM_WAIT(KFLOW_AI_BUILTIN_IN_SEM_ID);

		///< clear
		memset(&g_fist_layer_imem[net_id], 0, sizeof(g_fist_layer_imem[net_id]));

		///< get first mode control
		p_1st_mctrl = vendor_ais_get_proclayer(first_layer, *p_mem_parm);
		if (p_1st_mctrl == NULL) {
			DBG_ERR("vendor_ais_get_proclayer fail...\r\n");
			SEM_SIGNAL(KFLOW_AI_BUILTIN_IN_SEM_ID);
			return E_SYS;
		}
		nn_mode = p_1st_mctrl->mode;

		///< get first layer input address from streaming
		vendor_ais_create_1st_layer_addr(p_input_info, p_1st_imem, p_1st_mctrl);

		///< update first layer parameters based on streaming
		vendor_ais_update_1st_layer_parm(p_input_info, p_1st_mctrl);

		///< update first layer input address
		vendor_ais_proc_input_init(nn_mode, p_1st_imem, *p_mem_parm, net_id);

		// do cache flush
		vos_cpu_dcache_sync(p_1st_mctrl->addr, ALIGN_CEIL_32(p_1st_mctrl->size), VOS_DMA_TO_DEVICE);

		SEM_SIGNAL(KFLOW_AI_BUILTIN_IN_SEM_ID);
		return E_OK;
}

UINT32 _kflow_ai_builtin_get_yuv_size(VENDOR_AIS_IMG_PARM *p_input_info)
{
	const UINT32 img_size = p_input_info->line_ofs * p_input_info->height;
	UINT32 yuv_size;

	if (p_input_info->fmt == KDRV_IPP_BUILTIN_FMT_YUV420) {
		yuv_size = (img_size * 3 + 1) >> 1;
	} else if (p_input_info->fmt == KDRV_IPP_BUILTIN_FMT_Y8) {
		yuv_size = img_size;
	} else {
		DBG_ERR("invalid fmt(0x%x)\r\n", p_input_info->fmt);
		yuv_size = 0;
	}

	return yuv_size;
}

BOOL _kflow_ai_builtin_put_yuv(UINT32 proc_id, VENDOR_AIS_IMG_PARM *p_img_parm)
{
	KFLOW_AI_BUILTIN_YUVQ *p_obj;

	p_obj = &(kflow_ai_bt_obj.yuv_queue);

	DBG_IND("%s:(line%d) proc_id=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, proc_id, p_obj->front, p_obj->rear, p_obj->b_full);

	if ((p_obj->front == p_obj->rear) && (p_obj->b_full == TRUE)) {
		return FALSE;
	} else {
		p_obj->queue[p_obj->rear].va = p_img_parm->va;
		p_obj->queue[p_obj->rear].width = p_img_parm->width;
		p_obj->queue[p_obj->rear].height = p_img_parm->height;
		p_obj->queue[p_obj->rear].line_ofs = p_img_parm->line_ofs;
		p_obj->queue[p_obj->rear].timestamp = p_img_parm->timestamp;
		p_obj->queue[p_obj->rear].release_flag = p_img_parm->release_flag;

		p_obj->rear = (p_obj->rear + 1) % KFLOW_AI_BUILTIN_YUVQ_MAX;
		if (p_obj->front == p_obj->rear) { // Check Queue full
			p_obj->b_full = TRUE;
		}
		return TRUE;
	}
}

BOOL _kflow_ai_builtin_get_yuv(UINT32 proc_id, VENDOR_AIS_IMG_PARM *p_img_parm)
{
	KFLOW_AI_BUILTIN_YUVQ *p_obj;

	p_obj = &(kflow_ai_bt_obj.yuv_queue);

	DBG_IND("%s:(line%d) proc_id=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, proc_id, p_obj->front, p_obj->rear, p_obj->b_full);

	if ((p_obj->front == p_obj->rear) && (p_obj->b_full == FALSE)) {
		DBG_ERR("get yuv queue empty!\r\n");
		return FALSE;
	} else {
		p_img_parm->va = p_obj->queue[p_obj->front].va;
		p_img_parm->width = p_obj->queue[p_obj->front].width;
		p_img_parm->height = p_obj->queue[p_obj->front].height;
		p_img_parm->line_ofs = p_obj->queue[p_obj->front].line_ofs;
		p_img_parm->timestamp = p_obj->queue[p_obj->front].timestamp;
		p_img_parm->release_flag = p_obj->queue[p_obj->front].release_flag;
		p_obj->front = (p_obj->front + 1) % KFLOW_AI_BUILTIN_YUVQ_MAX;
		if (p_obj->front == p_obj->rear) { // Check Queue full
			p_obj->b_full = FALSE;
		}
		return TRUE;
	}
}

void _kflow_ai_builtin_trig(KFLOW_AI_BUILTIN_FMD_INFO *p_info, UINT32 reserved)
{
	VENDOR_AIS_IMG_PARM img_parm = {0};
	UINT32 proc_id = 0;
	UINT32 size = 0;
#if AI_PERF_TEST
	static int is_first_trig = 1;
	VOS_TICK ai_tick_begin, ai_tick_end;

	if (is_first_trig) {
		vos_perf_mark(&ai_tick_begin);
	}
#endif

	img_parm.fmt         = p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].fmt;
	img_parm.width       = p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].size.w;
	img_parm.height      = p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].size.h;
	img_parm.channel     = 2;
	img_parm.batch_num   = 0;
	img_parm.line_ofs    = p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].loff[0];
	img_parm.channel_ofs = 0;
	img_parm.batch_ofs   = 0;
#if AI_SUPPORT_MULTI_FMT
	img_parm.fmt_type    = 0;
#endif
	img_parm.release_flag = p_info->release_flg;
	size = _kflow_ai_builtin_get_yuv_size(&img_parm);

	// check addr and size validation
	if (p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].addr[0] == 0 || size == 0) {
		DBG_ERR("invalid input, addr(0x%x) size(0x%x)\r\n",
			p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].addr[0], size);
		return;
	}
	img_parm.va          = p_info->out_img[KDRV_IPP_BUILTIN_PATH_ID_3].addr[0];
	img_parm.pa          = nvtmpp_sys_va2pa(img_parm.va);
#if DEBUG_AI_FLOW_MSG
	printk("trig: pa(0x%x) va(0x%x) size(0x%x)\r\n", img_parm.pa, img_parm.va, size);
#endif

	if ((kflow_ai_bt_obj.trig_cnt % g_proc_period) != 0) { // do proc_net every N times
		goto skip_trig;
	}

	if (kflow_ai_bt_obj.stop_trig) {
		goto skip_trig;
	}

	vendor_ais_net_input_init(&img_parm, proc_id);

	_kflow_ai_builtin_put_yuv(proc_id, &img_parm);

	set_flg(FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_PROC_NET);

skip_trig:
	if (img_parm.release_flag) {
		nvtmpp_unlock_fastboot_blk(img_parm.va);
		SEM_WAIT(KFLOW_AI_BUILTIN_COMM_SEM_ID);
		kflow_ai_bt_obj.yuv_unlock_cnt++;
		if (kflow_ai_bt_obj.yuv_unlock_cnt == 2) {
			kflow_ai_bt_obj.stop_trig = 1;
		}
		SEM_SIGNAL(KFLOW_AI_BUILTIN_COMM_SEM_ID);
	}
	kflow_ai_bt_obj.trig_cnt++;

#if AI_PERF_TEST
	if (is_first_trig) {
		vos_perf_mark(&ai_tick_end);
		DBG_DUMP("[AI_BUILTIN] trig() - process %u us\r\n", (unsigned int)vos_perf_duration(ai_tick_begin, ai_tick_end));
		is_first_trig = 0;
	}
#endif
	return;
}

THREAD_DECLARE(kflow_ai_builtin_task, arglist)
{
	FLGPTN uiFlag = 0;
	UINT32 proc_id = 0;
	int is_first_frame = 1;
#if AI_PERF_TEST
	VOS_TICK ai_tick_begin, ai_tick_end;
#endif
	DBG_IND("[KFLOW_AI_BUILTIN] kflow_ai_builtin_task() start\r\n");

	THREAD_ENTRY(); //kent_tsk();

	clr_flg(FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_IDLE);

	// coverity[no_escape]
	while (!THREAD_SHOULD_STOP) {
		set_flg(FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_IDLE);

		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_PROC_NET | FLG_KFLOW_AI_BUILTIN_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		clr_flg(FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_IDLE);

		if (uiFlag & FLG_KFLOW_AI_BUILTIN_STOP) {
			break;
		}

		if (uiFlag & FLG_KFLOW_AI_BUILTIN_PROC_NET) {
			// process network
			if (is_first_frame) {
				nvt_bootts_add_ts("ai");
#if AI_PERF_TEST
				vos_perf_mark(&ai_tick_begin);
#endif
			}
			kflow_ai_builtin_proc_net(&kflow_ai_bt_obj.mem_parm, proc_id);
			if (is_first_frame) {
				nvt_bootts_add_ts("ai");
#if AI_PERF_TEST
				vos_perf_mark(&ai_tick_end);
				DBG_DUMP("[AI_BUILTIN] proc_net() - process %u us\r\n", (unsigned int)vos_perf_duration(ai_tick_begin, ai_tick_end));
#endif
				is_first_frame = 0;
			}
		}
	} // end of while loop

	nvt_bootts_add_ts("ai_trans"); // log transition time of ai fastboot (end time of builtin)

	set_flg(FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_STOP_DONE);

	THREAD_RETURN(0);
}

ER _kflow_ai_builtin_tsk_start(void)
{
	THREAD_CREATE(KFLOW_AI_BUILTIN_TSK_ID, kflow_ai_builtin_task, NULL, "kflow_ai_builtin_task");
	if (KFLOW_AI_BUILTIN_TSK_ID == 0) {
		printk("Invalid KFLOW_AI_BUILTIN_TSK_ID\r\n");
		return E_SYS;
	}
	THREAD_RESUME(KFLOW_AI_BUILTIN_TSK_ID);
	DBG_IND("[KFLOW_AI_BUILTIN] task start ...\r\n");

	return E_OK;
}

ER _kflow_ai_builtin_tsk_stop(void)
{
	FLGPTN uiFlag;

	DBG_IND("[KFLOW_AI_BUILTIN] tskstop wait idle ..\r\n");
	wai_flg(&uiFlag, FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_IDLE, TWF_ORW);
	DBG_IND("[KFLOW_AI_BUILTIN] tskstop wait idle OK\r\n");

	set_flg(FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_STOP);
	DBG_IND("[KFLOW_AI_BUILTIN] tskstop wait stop_done ..\r\n");
	wai_flg(&uiFlag, FLG_ID_KFLOW_AI_BUILTIN, FLG_KFLOW_AI_BUILTIN_STOP_DONE, TWF_ORW | TWF_CLR);
	DBG_IND("[KFLOW_AI_BUILTIN] tskstop wait stop_done OK\r\n");

	return E_OK;
}

void kflow_ai_builtin_install_id(void)
{
	OS_CONFIG_FLAG(FLG_ID_KFLOW_AI_BUILTIN);
	OS_CONFIG_SEMPHORE(KFLOW_AI_BUILTIN_IN_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(KFLOW_AI_BUILTIN_RSLT_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(KFLOW_AI_BUILTIN_COMM_SEM_ID, 0, 1, 1);
}

void kflow_ai_builtin_uninstall_id(void)
{
	rel_flg(FLG_ID_KFLOW_AI_BUILTIN);
	SEM_DESTROY(KFLOW_AI_BUILTIN_IN_SEM_ID);
	SEM_DESTROY(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
	SEM_DESTROY(KFLOW_AI_BUILTIN_COMM_SEM_ID);
}

ER kflow_ai_builtin_assign_rslt_mem(UINT32 proc_id, KFLOW_AI_BUILTIN_MEM_PARM *p_rslt_buff)
{
	UINT32 i;
	UINT32 rsltq_max = KFLOW_AI_BUILTIN_RSLTQ_MAX;
	UINT32 tmp_va = p_rslt_buff->va;
	UINT32 tmp_pa = p_rslt_buff->pa;
	UINT32 end_pa = p_rslt_buff->pa + p_rslt_buff->size;
	UINT32 end_va = p_rslt_buff->va + p_rslt_buff->size;
	UINT32 pd_rslt_sz = PD_RSLT_SIZE;

	/*printk("=======assign_rslt_mem============\r\n");
	printk("start(0x%x) end(0x%x) size(0x%x)\r\n", tmp_addr, end_addr, p_rslt_buff->size);*/

	for (i = 0; i < rsltq_max; i++)
    {
		rslt_buf[i].pa = tmp_pa;
		rslt_buf[i].va = tmp_va;
		rslt_buf[i].size = pd_rslt_sz;
		//printk("i(%u), pa(0x%x) va(0x%x) size(0x%x)\r\n", i, rslt_buf[i].pa, rslt_buf[i].va, rslt_buf[i].size);

		tmp_pa += pd_rslt_sz;
		if (tmp_pa > end_pa) {
			DBG_ERR("check pa error! tmp_pa(0x%x) > end_pa(0x%x)\r\n", tmp_pa, end_pa);
		}

		tmp_va += pd_rslt_sz;
		if (tmp_va > end_va) {
			DBG_ERR("check va error! tmp_va(0x%x) > end_va(0x%x)\r\n", tmp_va, end_va);
		}
    }

	return E_OK;
}

ER kflow_ai_builtin_alloc_rslt_que_mem(UINT32 proc_id, UINT32 que_max_num)
{
	UINT32 buf_size = sizeof(KFLOW_AI_BUILTIN_RSLT_INFO) * que_max_num;
	PKFLOW_AI_BUILTIN_RSLTQ p_obj;

	if (buf_size == 0) {
		DBG_ERR("[KFLOW_AI_BUILTIN] alloc_que_mem fail\r\n");
		return E_SYS;
	}

	/* alloc rslt queue mem */
	p_obj = &(kflow_ai_bt_obj.rslt_queue);
	p_obj->queue = (KFLOW_AI_BUILTIN_RSLT_INFO *)vmalloc(buf_size);

	/* alloc rslt buffer mem */

	return E_OK;
}

void kflow_ai_builtin_free_rslt_que_mem(UINT32 proc_id)
{
	PKFLOW_AI_BUILTIN_RSLTQ p_obj;

	p_obj = &(kflow_ai_bt_obj.rslt_queue);
	if (p_obj->queue != NULL) {
		vfree(p_obj->queue);
	}
}

BOOL kflow_ai_builtin_put_rslt(UINT32 proc_id, VENDOR_AIS_IMG_PARM *p_img_parm, PD_MEM *p_pd_mem)
{
	PKFLOW_AI_BUILTIN_RSLTQ p_obj;
	UINT32 rslt_start_va;

	SEM_WAIT(KFLOW_AI_BUILTIN_RSLT_SEM_ID);

	p_obj = &(kflow_ai_bt_obj.rslt_queue);
	rslt_start_va = kflow_ai_bt_obj.mem_parm.rslt_buff.va;

	if ((p_obj->front == p_obj->rear) && (p_obj->b_full == TRUE)) {
		//DBG_ERR("result lock queue is full!\r\n");
		SEM_SIGNAL(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
		return FALSE;
	} else {
		// copy out_result to rslt_buf
		if (p_pd_mem->out_result.size > rslt_buf[p_obj->rear].size) {
			DBG_ERR("r(%u) rslt_buf(0x%x) not enough! out_size(0x%x)\r\n",
					p_obj->rear, rslt_buf[p_obj->rear].size, p_pd_mem->out_result.size);
			return FALSE;
		}
		memcpy((void *)rslt_buf[p_obj->rear].va, (void *)p_pd_mem->out_result.va, p_pd_mem->out_result.size);

		// assign result info
		if (rslt_buf[p_obj->rear].va < rslt_start_va) {
			DBG_ERR("invalid rslt mem! rslt_buf[%u].va(0x%x) > rslt_start_va(0x%x)\r\n",
					p_obj->rear, rslt_buf[p_obj->rear].va, rslt_start_va);
			return FALSE;
		} else {
			// we will store va_ofs here, then normal mode must add rslt_start_addr back before returning to user.
			p_obj->queue[p_obj->rear].out_result.va = rslt_buf[p_obj->rear].va - rslt_start_va;
		}
		p_obj->queue[p_obj->rear].out_result.pa = rslt_buf[p_obj->rear].pa;
		p_obj->queue[p_obj->rear].out_result.size = p_pd_mem->out_result.size;
		p_obj->queue[p_obj->rear].out_result.blk = p_pd_mem->out_result.blk;
		p_obj->queue[p_obj->rear].out_num = p_pd_mem->out_num;
		p_obj->queue[p_obj->rear].timestamp = p_img_parm->timestamp;
		/*printk("put_rslt: r=%u, pa=0x%x, va=0x%x, size=0x%x, blk=0x%x\r\n",
				p_obj->rear,
				p_obj->queue[p_obj->rear].out_result.pa,
				p_obj->queue[p_obj->rear].out_result.va,
				p_obj->queue[p_obj->rear].out_result.size,
				p_obj->queue[p_obj->rear].out_result.blk);*/

		p_obj->rear = (p_obj->rear + 1) % KFLOW_AI_BUILTIN_RSLTQ_MAX;
		if (p_obj->front == p_obj->rear) { // Check Queue full
			p_obj->b_full = TRUE;
		}
	}
	SEM_SIGNAL(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
	return TRUE;
}

BOOL kflow_ai_builtin_get_rslt(UINT32 proc_id, KFLOW_AI_BUILTIN_RSLT_INFO *builtin_rslt_info)
{
	KFLOW_AI_BUILTIN_RSLTQ *p_obj;

	SEM_WAIT(KFLOW_AI_BUILTIN_RSLT_SEM_ID);

	p_obj = &(kflow_ai_bt_obj.rslt_queue);

	DBG_IND("%s:(line%d) proc_id=%d, f=%d, r=%d, full=%d\r\n", __func__, __LINE__, proc_id, p_obj->front, p_obj->rear, p_obj->b_full);

	if ((p_obj->front == p_obj->rear) && (p_obj->b_full == FALSE)) {
		SEM_SIGNAL(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
		DBG_ERR("get rslt queue empty!\r\n");
		return FALSE;
	} else {
		builtin_rslt_info->out_result.pa = p_obj->queue[p_obj->front].out_result.pa;
		builtin_rslt_info->out_result.va = p_obj->queue[p_obj->front].out_result.va;
		builtin_rslt_info->out_result.size = p_obj->queue[p_obj->front].out_result.size;
		builtin_rslt_info->out_result.blk = p_obj->queue[p_obj->front].out_result.blk;
		builtin_rslt_info->out_num = p_obj->queue[p_obj->front].out_num;
		builtin_rslt_info->timestamp = p_obj->queue[p_obj->front].timestamp;

		p_obj->front = (p_obj->front + 1) % KFLOW_AI_BUILTIN_RSLTQ_MAX;

		if (p_obj->front == p_obj->rear) { // Check Queue full
			p_obj->b_full = FALSE;
		}
		SEM_SIGNAL(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
		return TRUE;
	}
}

UINT32 kflow_ai_builtin_how_many_in_rsltq(UINT32 proc_id)
{
	UINT32 front, rear, full, sq = 0;
	UINT32 rsltq_max = KFLOW_AI_BUILTIN_RSLTQ_MAX;
	KFLOW_AI_BUILTIN_RSLTQ *p_obj;

	SEM_WAIT(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
	p_obj = &(kflow_ai_bt_obj.rslt_queue);
	front = p_obj->front;
	rear = p_obj->rear;
	full = p_obj->b_full;
	SEM_SIGNAL(KFLOW_AI_BUILTIN_RSLT_SEM_ID);
	if (front < rear) {
		sq = rear - front;
	} else if (front > rear) {
		sq = rsltq_max - (front - rear);
	} else if (front == rear && full == TRUE) {
		sq = rsltq_max;
	} else {
		sq = 0;
	}

	DBG_IND("%s:(line%d) sq=%d\r\n", __func__, __LINE__, sq);

	return sq;
}

/* HDAL will call this api to get builtin mem */
ER kflow_ai_builtin_get_builtin_mem(KFLOW_AI_BUILTIN_MAP_MEM_PARM *p_mem)
{
	p_mem->kerl_parm = kflow_ai_bt_obj.mem_parm.kerl_parm;
	p_mem->user_model = kflow_ai_bt_obj.mem_parm.user_model;
	p_mem->io_buff = kflow_ai_bt_obj.mem_parm.io_buff;
	return E_OK;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER kflow_ai_builtin_init(KFLOW_AI_BUILTIN_INIT_INFO *info)
{
	UINT32 id;
	AI_DRV_OPENCFG cfg;
	ER ret = E_OK;
#if AI_PERF_TEST
	VOS_TICK ai_tick_begin, ai_tick_end;
	vos_perf_mark(&ai_tick_begin);
#endif

	cfg.opencfg.clock_sel = 0;
	cfg.net_id = 0;
	kdrv_ai_config_flow(1); //config to new AI flow

	/* CNN */
	cfg.engine = AI_ENG_CNN;
	id = KDRV_DEV_ID(0, KDRV_AI_ENGINE_CNN, 0);
	kdrv_ai_builtin_open(0, KDRV_AI_ENGINE_CNN);

	/* NUE */
	cfg.engine = AI_ENG_NUE;
	id = KDRV_DEV_ID(0, KDRV_AI_ENGINE_NUE, 0);
	kdrv_ai_builtin_open(0, KDRV_AI_ENGINE_NUE);

	/* NUE2 */
	cfg.engine = AI_ENG_NUE2;
	id = KDRV_DEV_ID(0, KDRV_AI_ENGINE_NUE2, 0);
	kdrv_ai_builtin_open(0, KDRV_AI_ENGINE_NUE2);

	kflow_ai_builtin_install_id();

	/* start task */
	if ((ret = _kflow_ai_builtin_tsk_start()) != E_OK) {
		return ret;
	}

	/* init mem by dtsi */
	printk("kflow_ai_builtin_init: cnn_pool blk_va(0x%x) size(0x%x)\r\n", info->blk_addr, info->blk_size);
	if ((ret = _kflow_ai_builtin_get_fastboot_dt(&kflow_ai_bt_obj.mem_parm)) != E_OK) {
		return ret;
	}

	// check validity of memory
	if (kflow_ai_bt_obj.mem_parm.kerl_parm.va != info->blk_addr) {
		DBG_ERR("memory alloc not match, kerl_va(0x%x) blk_addr(0x%x), please check dtsi\r\n",
			kflow_ai_bt_obj.mem_parm.kerl_parm.va, info->blk_addr);
		return E_SYS;
	}

	/* fix va in parm */
	if ((ret = _kflow_ai_builtin_map_mem(&kflow_ai_bt_obj.mem_parm)) != E_OK) {
		return ret;
	}

	/* alloc rslt queue mem */
	if ((ret = kflow_ai_builtin_alloc_rslt_que_mem(0, KFLOW_AI_BUILTIN_RSLTQ_MAX)) != E_OK) {
		return ret;
	}

	/* assign rslt buffer mem */
	kflow_ai_builtin_assign_rslt_mem(0, &kflow_ai_bt_obj.mem_parm.rslt_buff);

#ifdef CONFIG_MTD_SPINOR
	/* wait for preload finish */
	ret = (ER)nvt_check_preload_finish();
	if (ret != 1) { // 1: OK, -1: NG
		DBG_ERR("check preload finish fail(%d)\r\n", ret);
		return ret;
	}
#endif

	/* register for pd postproc */
	kdrv_ipp_builtin_reg_fmd_cb((KDRV_IPP_BUILTIN_FMD_CB)&_kflow_ai_builtin_trig);
	pd_set_mem(&pd_mem, (UINT32)kflow_ai_bt_obj.mem_parm.pd_buff.pa, (UINT32)kflow_ai_bt_obj.mem_parm.pd_buff.va);

	/* variable reset */
	SEM_WAIT(KFLOW_AI_BUILTIN_COMM_SEM_ID);
	kflow_ai_bt_obj.stop_trig = 0;
	SEM_SIGNAL(KFLOW_AI_BUILTIN_COMM_SEM_ID);

#if AI_PERF_TEST
	vos_perf_mark(&ai_tick_end);
	DBG_DUMP("[KFLOW_AI_BUILTIN] init() - process %u us\r\n", (unsigned int)vos_perf_duration(ai_tick_begin, ai_tick_end));
#endif

	return E_OK;
}

ER kflow_ai_builtin_exit(void)
{
	/* stop trigger */
	SEM_WAIT(KFLOW_AI_BUILTIN_COMM_SEM_ID);
	kflow_ai_bt_obj.stop_trig = 1;
	SEM_SIGNAL(KFLOW_AI_BUILTIN_COMM_SEM_ID);

	/* stop task */
	_kflow_ai_builtin_tsk_stop();

	kflow_ai_builtin_uninstall_id();

	kdrv_ai_builtin_close(0, KDRV_AI_ENGINE_CNN);
	kdrv_ai_builtin_close(0, KDRV_AI_ENGINE_NUE);
	kdrv_ai_builtin_close(0, KDRV_AI_ENGINE_NUE2);

	return E_OK;
}

/* linear trigger all jobs */
ER kflow_ai_builtin_proc_net(KFLOW_AI_BUILTIN_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_IMG_PARM img_parm = {0};
	UINT32 process_index = 0;
	UINT32 proc_layer_num;
	ER er = E_OK;
	UINT32 backbone_out_va[6];
	NN_IOMEM *cls_prob_32_mem, *bbox_pred_32_mem, *cls_prob_16_mem, *bbox_pred_16_mem, *cls_prob_8_mem, *bbox_pred_8_mem;
	NN_DATA *p_sao_cls32;
	NN_DATA *p_sao_bbox32;
	NN_DATA *p_sao_cls16;
	NN_DATA *p_sao_bbox16;
	NN_DATA *p_sao_cls8;
	NN_DATA *p_sao_bbox8;

	if ((p_mem == NULL) || (p_mem->kerl_parm.va == 0)) {
		DBG_ERR("invalid memory input\r\n");
		return E_SYS;
	}

	_kflow_ai_builtin_get_yuv(net_id, &img_parm);

	er = vendor_ais_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num  = p_head->mode_ctrl_num;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
				UINT32 id, engine;
				KDRV_AI_TRIG_MODE mode = AI_TRIG_MODE_LL;
				KDRV_AI_LL_INFO info = { (KDRV_AI_LL_HEAD *)(p_mctrl[process_index].addr), p_mctrl[process_index].tot_trig_eng_times };
				KDRV_AI_TRIGGER_PARAM trig_parm = {0};

				if (p_mctrl[process_index].eng == NN_GEN_ENG_CNN || p_mctrl[process_index].eng == NN_GEN_ENG_CNN2) {
					engine = KDRV_AI_ENGINE_CNN;
				} else if (p_mctrl[process_index].eng == NN_GEN_ENG_NUE) {
					engine = KDRV_AI_ENGINE_NUE;
				} else if (p_mctrl[process_index].eng == NN_GEN_ENG_NUE2) {
					engine = KDRV_AI_ENGINE_NUE2;
				} else {
					DBG_ERR("invalid eng(%d), proc_idx(%u)\r\n", p_mctrl[process_index].eng, process_index);
				}
				// only support blocking mode
				id = KDRV_DEV_ID(0, engine, 0);
				kdrv_ai_builtin_set(id, KDRV_AI_PARAM_MODE_INFO, &mode);
				kdrv_ai_builtin_set(id, KDRV_AI_PARAM_LL_INFO, &info);
				kdrv_ai_builtin_trigger(id, &trig_parm, NULL, NULL);

#if DEBUG_AI_OUTBUF
				_kflow_ai_builtin_dump_out_buf(&p_mctrl[process_index], process_index, 0);
#endif
			}
			break;
		case NN_GEN_TRIG_COMMON:
#if DEBUG_AI_FLOW_MSG
			printk("%s:%d proc_idx(%d) eng(%d) trig_src(%d) => skip\n", __func__, __LINE__, process_index, p_mctrl[process_index].eng, p_mctrl[process_index].trig_src);
#endif
			break;
		case NN_GEN_TRIG_APP_AI_DRV:
		default:
			DBG_ERR("invalid trig_src(%d), proc_idx(%u)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	if (img_parm.release_flag) {
		nvtmpp_unlock_fastboot_blk(img_parm.va);
		SEM_WAIT(KFLOW_AI_BUILTIN_COMM_SEM_ID);
		kflow_ai_bt_obj.yuv_unlock_cnt++;
		if (kflow_ai_bt_obj.yuv_unlock_cnt == 2) {
			kflow_ai_bt_obj.stop_trig = 1;
		}
		SEM_SIGNAL(KFLOW_AI_BUILTIN_COMM_SEM_ID);
	}

	/* pd postproc */
	cls_prob_32_mem  = &p_mctrl[g_backbone_out_indexs[0][0]].iomem;
	bbox_pred_32_mem = &p_mctrl[g_backbone_out_indexs[0][1]].iomem;
	cls_prob_16_mem  = &p_mctrl[g_backbone_out_indexs[1][0]].iomem;
	bbox_pred_16_mem = &p_mctrl[g_backbone_out_indexs[1][1]].iomem;
	cls_prob_8_mem   = &p_mctrl[g_backbone_out_indexs[2][0]].iomem;
	bbox_pred_8_mem  = &p_mctrl[g_backbone_out_indexs[2][1]].iomem;

	p_sao_cls32 = (NN_DATA *)cls_prob_32_mem->omem_addr;
	p_sao_bbox32 = (NN_DATA *)bbox_pred_32_mem->omem_addr;
	p_sao_cls16 = (NN_DATA *)cls_prob_16_mem->omem_addr;
	p_sao_bbox16 = (NN_DATA *)bbox_pred_16_mem->omem_addr;
	p_sao_cls8 = (NN_DATA *)cls_prob_8_mem->omem_addr;
	p_sao_bbox8 = (NN_DATA *)bbox_pred_8_mem->omem_addr;

	vos_cpu_dcache_sync(p_sao_cls32[0].va, p_sao_cls32[0].size, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(p_sao_bbox32[0].va, p_sao_bbox32[0].size, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(p_sao_cls16[0].va, p_sao_cls16[0].size, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(p_sao_bbox16[0].va, p_sao_bbox16[0].size, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(p_sao_cls8[0].va, p_sao_cls8[0].size, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(p_sao_bbox8[0].va, p_sao_bbox8[0].size, VOS_DMA_FROM_DEVICE);

	backbone_out_va[0] = p_sao_cls32[0].va;
	backbone_out_va[1] = p_sao_bbox32[0].va;
	backbone_out_va[2] = p_sao_cls16[0].va;
	backbone_out_va[3] = p_sao_bbox16[0].va;
	backbone_out_va[4] = p_sao_cls8[0].va;
	backbone_out_va[5] = p_sao_bbox8[0].va;

	kernel_neon_begin();

	er = pd_postprocess(&pd_mem, backbone_out_va);
#if DEBUG_AI_DET_RSLT
	pd_print_results(&pd_mem, SENSOR_MAIN_SIZE_W, SENSOR_MAIN_SIZE_H);
#endif
	kernel_neon_end();

	/* add rslt to queue */
	kflow_ai_builtin_put_rslt(net_id, &img_parm, &pd_mem);

#if TEST_GET_QUE
	proc_net_cnt++;
	if (proc_net_cnt > 30) {
		UINT32 i = 0;
		KFLOW_AI_BUILTIN_RSLT_INFO rslt_info = {0};
		while(kflow_ai_builtin_how_many_in_rsltq(net_id) > 0) {
			kflow_ai_builtin_get_rslt(net_id, &rslt_info);
			printk("get_rslt: i(%u), pa(0x%x) va(0x%x) sz(0x%x) blk(0x%x) out_num(%u) ts(0x%x)\r\n",
					i,
					rslt_info.out_result.pa,
					rslt_info.out_result.va,
					rslt_info.out_result.size,
					rslt_info.out_result.blk,
					rslt_info.out_num,
					rslt_info.timestamp);
			i++;
		}
		proc_net_cnt = 0;
	}
#endif

	return er;
}

