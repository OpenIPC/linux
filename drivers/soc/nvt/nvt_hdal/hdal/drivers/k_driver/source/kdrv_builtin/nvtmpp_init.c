#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include "kwrap/spinlock.h"
#include "nvtmpp_init.h"

#define NVTMPP_DDR_MAX_NUM          3

/**
   DDR ID.

*/
typedef enum {
	NVTMPP_DDR_1                    =   0, ///< DDR1
	NVTMPP_DDR_2                    =   1, ///< DDR2
	NVTMPP_DDR_3                    =   2, ///< DDR3
	NVTMPP_DDR_MAX = NVTMPP_DDR_3,
	ENUM_DUMMY4WORD(NVTMPP_DDR)
} NVTMPP_DDR;

static NVTMPP_MMZ_INFO_S      mmz;           ///< ddr memory range
static NVTMPP_FASTBOOT_MEM_S  fastboot_mem;
static NVTMPP_FASTBOOT_MEM_S  *p_fboot_mem = &fastboot_mem;
static NVTMPP_LOCK_BLK_CB     fastboot_lock_cb = NULL;
static NVTMPP_UNLOCK_BLK_CB   fastboot_unlock_cb = NULL;
static VK_DEFINE_SPINLOCK(my_lock);

static NVTMPP_FBOOT_POOL_DTS_INFO_S fboot_pvpool_dts_info[FBOOT_POOL_CNT]= {
	{"vdoprc0.ctrl.work"    , "vprc_ctrl_0", 1},
	//{"vdoenc.ctrl.bs"       , "venc_ctrl_bs",0},
	{"vdoenc.o[0].max"      , "venc_max_0",  1},
	{"audcap.ctrl.work"     , "acap_ctrl_0", 1},
	{"vdocap0.ctrl.ring_buf", "vcap_shdr_ringbuf", 1},
	{"vdoenc.o[1].max"      , "venc_max_1",  1},
	{"vdoenc.o[2].max"      , "venc_max_2",  1},
	{"vdoenc.o[3].max"      , "venc_max_3",  1},
	{"vdoenc.o[4].max"      , "venc_max_4",  1},
	{"vdoenc.o[5].max"      , "venc_max_5",  1},
	{"vdoprc1.ctrl.work"    , "vprc_ctrl_1", 1},
};


#define NVTMPP_INITED_TAG       MAKEFOURCC('N', 'M', 'P', 'P') ///< a key value

/* return  > 0 for success, 0 for fail*/
UINT32  nvtmpp_sys_va2pa(UINT32 virt_addr)
{
	UINT32 i;

	for (i = 0; i <= NVTMPP_DDR_MAX; i++) {
		if (mmz.ddr_mem[i].va > 0 && virt_addr >= mmz.ddr_mem[i].va &&
			virt_addr < mmz.ddr_mem[i].va + mmz.ddr_mem[i].size) {
			return virt_addr-mmz.ddr_mem[i].va + mmz.ddr_mem[i].pa;
		}
	}
	DBG_ERR("Invalid virt_addr 0x%x\r\n", (int)virt_addr);
	return 0;

}
/* return  > 0 for success, 0 for fail*/
UINT32  nvtmpp_sys_pa2va(UINT32 phys_addr)
{
	UINT32 virt_addr = 0;
	UINT32 i;

	for (i = 0; i <= NVTMPP_DDR_MAX; i++) {
		if (mmz.ddr_mem[i].pa > 0 && phys_addr >= mmz.ddr_mem[i].pa &&
			phys_addr < mmz.ddr_mem[i].pa + mmz.ddr_mem[i].size) {
			return phys_addr-mmz.ddr_mem[i].pa + mmz.ddr_mem[i].va;
		}
	}
	return virt_addr;
}
static int nvtmpp_parse_mmz_dt_p(NVTMPP_MMZ_INFO_S *mmz)
{
    /* device node path - check it from /proc/device-tree/ */
    char *path = "/hdal-memory/media";
    struct device_node *dt_node;
	UINT32 reg[6] = {0};

    dt_node = of_find_node_by_path(path);
	if (!dt_node) {
		DBG_ERR("Failed to find node by path: %s.\r\n", path);
		return -1;
    }
	DBG_IND("Found the node for %s.\r\n", path);
	if (!of_property_read_u32_array(dt_node, "reg", reg, 6)) {
		;
	} else if(!of_property_read_u32_array(dt_node, "reg", reg, 4)) {
		;
	} else if (of_property_read_u32_array(dt_node, "reg", reg, 2)) {
		return -1;
	}
	dt_node = of_find_node_by_path("/hdal-memory/cfgs");
	if (dt_node) {
		of_property_read_u32_array(dt_node, "max_pools_cnt", (UINT32 *)&mmz->max_pools_cnt, 1);
	}
	DBG_IND("media memory: %x %x %x %x %x %x\r\n", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5]);
	mmz->ddr_mem[NVTMPP_DDR_1].pa   = reg[0];
	mmz->ddr_mem[NVTMPP_DDR_1].size = reg[1];
	mmz->ddr_mem[NVTMPP_DDR_2].pa   = reg[2];
	mmz->ddr_mem[NVTMPP_DDR_2].size = reg[3];
	mmz->ddr_mem[NVTMPP_DDR_3].pa   = reg[4];
	mmz->ddr_mem[NVTMPP_DDR_3].size = reg[5];
	return 0;
}

int nvtmpp_parse_fastboot_mem_dt(void)
{
    /* device node path - check it from /proc/device-tree/ */
	char *path = "/fastboot/hdal-mem";
	struct device_node *dt_node;
	char node_name[30];
	UINT32 i, /*path_id, */pool_id;
	NVTMPP_FBOOT_POOL_DTS_INFO_S *p_fboot_pool;
	int    ret;
	NVTMPP_FASTBOOT_MEM_S *p_mem;

	p_mem = &fastboot_mem;
	memset((void *)p_mem, 0, sizeof(NVTMPP_FASTBOOT_MEM_S));
	dt_node = of_find_node_by_path(path);
	if (!dt_node) {
		DBG_ERR("Failed to find node by path: %s.\r\n", path);
		return -1;
    }
	DBG_IND("Found the node for %s.\r\n", path);
	#if 1
	if (of_property_read_u32_array(dt_node, "comn_blk_cnt", (UINT32 *)&p_mem->comn_blk_cnt, 1)) {
		DBG_ERR("Failed to read comn_blk_cnt\r\n");
		return -1;
	}
	if (p_mem->comn_blk_cnt > FBOOT_COMNBLK_MAX_CNT) {
		p_mem->comn_blk_cnt = FBOOT_COMNBLK_MAX_CNT;
	}
	for (i = 0 ; i < p_mem->comn_blk_cnt; i++) {
		snprintf(node_name, sizeof(node_name), "comn_blk_%d", (int)i);
		if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->comn_blk[i], 2)) {
			DBG_ERR("Failed to read comn_blk_%d\r\n", i);
			return -1;
		}
		p_mem->comn_blk[i].addr = nvtmpp_sys_pa2va(p_mem->comn_blk[i].addr);
	}
	#else
	if (of_property_read_u32_array(dt_node, "vcap_blk_cnt", (UINT32 *)&p_mem->vcap_blk_cnt, 1)) {
		DBG_ERR("Failed to read vcap_blk_cnt\r\n");
		return -1;
	}
	if (p_mem->vcap_blk_cnt > FBOOT_VCAP_COMNBLK_MAX_CNT) {
		p_mem->vcap_blk_cnt = FBOOT_VCAP_COMNBLK_MAX_CNT;
	}
	for (i = 0 ; i < p_mem->vcap_blk_cnt; i++) {
		snprintf(node_name, sizeof(node_name), "vcap_blk_%d", (int)i);
		if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->vcap_blk[i], 2)) {
			DBG_ERR("Failed to read vcap_blk_0\r\n");
			return -1;
		}
		p_mem->vcap_blk[i].addr = nvtmpp_sys_pa2va(p_mem->vcap_blk[i].addr);
		p_mem->vcap_blk[i].ref_cnt = 1;
	}
	// vproc path1
	if (of_property_read_u32_array(dt_node, "vprc_blk_cnt", (UINT32 *)&p_mem->vprc_blk_cnt, 1)) {
		DBG_ERR("Failed to read vprc_blk_cnt\r\n");
		return -1;
	}
	if (p_mem->vprc_blk_cnt[0] > FBOOT_VPRC_COMNBLK_MAX_CNT) {
		p_mem->vprc_blk_cnt[0] = FBOOT_VPRC_COMNBLK_MAX_CNT;
	}
	for (i = 0 ; i < p_mem->vprc_blk_cnt[0]; i++) {
		snprintf(node_name, sizeof(node_name), "vprc_blk_%d", (int)i);
		if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->vprc_blk[0][i], 2)) {
			DBG_ERR("Failed to read vprc_blk_0\r\n");
			return -1;
		}
		p_mem->vprc_blk[0][i].addr = nvtmpp_sys_pa2va(p_mem->vprc_blk[0][i].addr);
		p_mem->vprc_blk[0][i].ref_cnt = 1;
	}
	// vproc path2, path3 ...
	for (path_id = 1; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
		snprintf(node_name, sizeof(node_name), "vprc_p%d_blk_cnt", (int)path_id+1);
		if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->vprc_blk_cnt[path_id], 1)) {
			break;
		}
		if (p_mem->vprc_blk_cnt[path_id] > FBOOT_VPRC_COMNBLK_MAX_CNT) {
			p_mem->vprc_blk_cnt[path_id] = FBOOT_VPRC_COMNBLK_MAX_CNT;
		}
		for (i = 0 ; i < p_mem->vprc_blk_cnt[path_id]; i++) {
			snprintf(node_name, sizeof(node_name), "vprc_p%d_blk_%d", (int)path_id+1, (int)i);
			if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->vprc_blk[path_id][i], 2)) {
				DBG_ERR("Failed to read %s\r\n", node_name);
				return -1;
			}
			p_mem->vprc_blk[path_id][i].addr = nvtmpp_sys_pa2va(p_mem->vprc_blk[path_id][i].addr);
			p_mem->vprc_blk[path_id][i].ref_cnt = 1;
		}
	}
	#endif
	// misc common pool type
	for (pool_id = 0; pool_id < FBOOT_MISC_CPOOL_MAX; pool_id++) {
		snprintf(node_name, sizeof(node_name), "misc_cpool%d_blk_cnt", (int)pool_id);
		if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->misc_cpool[pool_id].blk_cnt, 1)) {
			break;
		}
		snprintf(node_name, sizeof(node_name), "misc_cpool%d_pool_type", (int)pool_id);
		if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->misc_cpool[pool_id].pool_type, 1)) {
			DBG_ERR("Failed to read %s\r\n", node_name);
			return -1;
		}
		if (p_mem->misc_cpool[pool_id].blk_cnt > FBOOT_MISC_CPOOLBCK_MAX_CNT) {
			p_mem->misc_cpool[pool_id].blk_cnt = FBOOT_MISC_CPOOLBCK_MAX_CNT;
		}
		for (i = 0 ; i < p_mem->misc_cpool[pool_id].blk_cnt; i++) {
			snprintf(node_name, sizeof(node_name), "misc_cpool%d_blk_0", (int)pool_id);
			if (of_property_read_u32_array(dt_node, node_name, (UINT32 *)&p_mem->misc_cpool[pool_id].blk[i].addr, 2)) {
				DBG_ERR("Failed to read %s\r\n", node_name);
				return -1;
			}
			p_mem->misc_cpool[pool_id].blk[i].addr = nvtmpp_sys_pa2va(p_mem->misc_cpool[pool_id].blk[i].addr);
			p_mem->misc_cpool[pool_id].blk[i].ref_cnt = 1;
		}
	}
	p_fboot_pool = nvtmpp_get_fastboot_pvpool_dts_info();
	for (i = 0; i < FBOOT_POOL_CNT; i++) {
		ret = of_property_read_u32_array(dt_node, p_fboot_pool->dts_node, (UINT32 *)&p_mem->pv_pools[i], 2);
		if (ret && (!p_fboot_pool->optional)) {
			DBG_ERR("Failed to read %s\r\n", p_fboot_pool->dts_node);
			return -1;
		}
		if (!ret) {
			p_mem->pv_pools[i].addr = nvtmpp_sys_pa2va(p_mem->pv_pools[i].addr);
		} else {
			p_mem->pv_pools[i].addr = 0;
			p_mem->pv_pools[i].size = 0;
		}
		p_fboot_pool ++;
	}
	p_mem->init_tag = NVTMPP_INITED_TAG;

	#if 0
	{
		int path_id;

		DBG_DUMP("comn_blk_cnt = %d \r\n", p_mem->comn_blk_cnt);
		for (i = 0; i < p_mem->comn_blk_cnt; i++) {
			DBG_DUMP("comn_blk_%d addr = 0x%x size = 0x%x\r\n", i, p_mem->comn_blk[i].addr, p_mem->comn_blk[i].size);
		}
		for (path_id = 0; path_id < FBOOT_MISC_CPOOL_MAX; path_id++) {
			DBG_DUMP("misc_cpool%d_pool_type = 0x%x \r\n", path_id, p_mem->misc_cpool[path_id].pool_type);
			for (i = 0; i < p_mem->misc_cpool[path_id].blk_cnt; i++) {
				DBG_DUMP("misc_cpool%d_blk_%d addr = 0x%x size = 0x%x\r\n", path_id, i, p_mem->misc_cpool[path_id].blk[i].addr, p_mem->misc_cpool[path_id].blk[i].size);
			}
		}
		p_fboot_pool = nvtmpp_get_fastboot_pvpool_dts_info();
		for (i = 0; i < FBOOT_POOL_CNT; i++) {
			DBG_DUMP("%s  addr = 0x%x size = 0x%x\r\n", p_fboot_pool->dts_node, p_mem->pv_pools[i].addr, p_mem->pv_pools[i].size);
			p_fboot_pool ++;
		}
	}
	#endif
	#if 0
	{
		int path_id;

		DBG_DUMP("vcap_blk_cnt = %d \r\n", p_mem->vcap_blk_cnt);
		for (i = 0; i < p_mem->vcap_blk_cnt; i++) {
			DBG_DUMP("vcap_blk_%d addr = 0x%x size = 0x%x\r\n", i, p_mem->vcap_blk[i].addr, p_mem->vcap_blk[i].size);
		}
		for (path_id = 0; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
			DBG_DUMP("vprc%d_blk_cnt = %d \r\n", path_id, p_mem->vprc_blk_cnt[path_id]);
			for (i = 0; i < p_mem->vprc_blk_cnt[0]; i++) {
				DBG_DUMP("vprc%d_blk_%d addr = 0x%x size = 0x%x\r\n", path_id, i, p_mem->vprc_blk[path_id][i].addr, p_mem->vprc_blk[path_id][i].size);
			}
		}
		for (path_id = 0; path_id < FBOOT_MISC_CPOOL_MAX; path_id++) {
			DBG_DUMP("misc_cpool%d_pool_type = 0x%x \r\n", path_id, p_mem->misc_cpool[path_id].pool_type);
			for (i = 0; i < p_mem->misc_cpool[path_id].blk_cnt; i++) {
				DBG_DUMP("misc_cpool%d_blk_%d addr = 0x%x size = 0x%x\r\n", path_id, i, p_mem->misc_cpool[path_id].blk[i].addr, p_mem->misc_cpool[path_id].blk[i].size);
			}
		}
		p_fboot_pool = nvtmpp_get_fastboot_pvpool_dts_info();
		for (i = 0; i < FBOOT_POOL_CNT; i++) {
			DBG_DUMP("%s  addr = 0x%x size = 0x%x\r\n", p_fboot_pool->dts_node, p_mem->pv_pools[i].addr, p_mem->pv_pools[i].size);
			p_fboot_pool ++;
		}
	}
	#endif
	return 0;
}

NVTMPP_FASTBOOT_MEM_S *nvtmpp_get_fastboot_mem(void)
{
	if (p_fboot_mem->init_tag != NVTMPP_INITED_TAG) {
		return NULL;
	}
	return p_fboot_mem;
}

UINT32 nvtmpp_fastboot_spin_lock(void)
{
	unsigned long flags;
	vk_spin_lock_irqsave(&my_lock, flags);
	return flags;
}

void nvtmpp_fastboot_spin_unlock(UINT32 flags)
{
	vk_spin_unlock_irqrestore(&my_lock, flags);
}

void nvtmpp_reg_fastboot_lock_cb(NVTMPP_LOCK_BLK_CB lock_cb)
{
	fastboot_lock_cb = lock_cb;
}

void nvtmpp_reg_fastboot_unlock_cb(NVTMPP_UNLOCK_BLK_CB unlock_cb)
{
	fastboot_unlock_cb = unlock_cb;
}

int nvtmpp_lock_fastboot_blk(UINT32 blk_addr)
{
	UINT32                i, ret = -1 /*, path_id*/;
	unsigned long         flags;

	//DBG_ERR("0x%x\r\n", blk_addr);
	flags = nvtmpp_fastboot_spin_lock();
	if (fastboot_lock_cb != NULL) {
		ret = fastboot_lock_cb(blk_addr);
		goto unlock_exit;
	}
	#if 1
	for (i = 0; i < fastboot_mem.comn_blk_cnt; i++) {
		if (blk_addr == fastboot_mem.comn_blk[i].addr) {
			fastboot_mem.comn_blk[i].ref_cnt ++;
			ret = 0;
			goto unlock_exit;
		}
	}
	#else
	for (i = 0; i < fastboot_mem.vcap_blk_cnt; i++) {
		if (blk_addr == fastboot_mem.vcap_blk[i].addr) {
			fastboot_mem.vcap_blk[i].ref_cnt ++;
			ret = 0;
			goto unlock_exit;
		}
	}
	for (path_id = 1 ; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
		for (i = 0; i < fastboot_mem.vprc_blk_cnt[path_id]; i++) {
			if (blk_addr == fastboot_mem.vprc_blk[path_id][i].addr) {
				fastboot_mem.vprc_blk[path_id][i].ref_cnt ++;
				ret = 0;
				goto unlock_exit;
			}
		}
	}
	#endif
unlock_exit:
	nvtmpp_fastboot_spin_unlock(flags);
	return ret;
}


int nvtmpp_unlock_fastboot_blk(UINT32 blk_addr)
{
	UINT32                i, ret = -1 /*, path_id*/;
	unsigned long         flags;

	//DBG_ERR("0x%x\r\n", blk_addr);
	flags = nvtmpp_fastboot_spin_lock();
	if (fastboot_unlock_cb != NULL) {
		ret = fastboot_unlock_cb(blk_addr);
		goto unlock_exit;
	}
	#if 1
	for (i = 0; i < fastboot_mem.comn_blk_cnt; i++) {
		if (blk_addr == fastboot_mem.comn_blk[i].addr) {
			if (fastboot_mem.comn_blk[i].ref_cnt > 0) {
				fastboot_mem.comn_blk[i].ref_cnt --;
			}
			ret = 0;
			goto unlock_exit;
		}
	}
	#else
	for (i = 0; i < fastboot_mem.vcap_blk_cnt; i++) {
		if (blk_addr == fastboot_mem.vcap_blk[i].addr) {
			if (fastboot_mem.vcap_blk[i].ref_cnt > 0) {
				fastboot_mem.vcap_blk[i].ref_cnt --;
			}
			ret = 0;
			goto unlock_exit;
		}
	}
	for (path_id = 1 ; path_id < FBOOT_VPRC_MAX_PATH; path_id++) {
		for (i = 0; i < fastboot_mem.vprc_blk_cnt[path_id]; i++) {
			if (blk_addr == fastboot_mem.vprc_blk[path_id][i].addr) {
				if (fastboot_mem.vprc_blk[path_id][i].ref_cnt > 0) {
					fastboot_mem.vprc_blk[path_id][i].ref_cnt --;
				}
				ret = 0;
				goto unlock_exit;
			}
		}
	}
	#endif
unlock_exit:
	nvtmpp_fastboot_spin_unlock(flags);
	return ret;
}

NVTMPP_FBOOT_POOL_DTS_INFO_S *nvtmpp_get_fastboot_pvpool_dts_info(void)
{
	return fboot_pvpool_dts_info;
}

UINT32 nvtmpp_get_fastboot_blk(UINT32 blk_size)
{
	UINT32 i;

	NVTMPP_FASTBOOT_BLK_S emtpy_blk = {0};
	NVTMPP_FASTBOOT_BLK_S *p_candidate_blk = &emtpy_blk;
	NVTMPP_FASTBOOT_BLK_S *p_blk = 0;

	//DBG_ERR("blk_size=0x%x\r\n", (int)blk_size);
	emtpy_blk.size = 0xFFFFFFFF;
	if (p_fboot_mem->init_tag != NVTMPP_INITED_TAG) {
		return 0;
	}
	for (i = 0 ; i < p_fboot_mem->comn_blk_cnt; i++) {
		p_blk = &p_fboot_mem->comn_blk[i];
		if (p_blk->ref_cnt == 0 && blk_size <= p_blk->size && p_blk->size < p_candidate_blk->size) {
			p_candidate_blk = p_blk;
			//DBG_ERR("p_candidate_blk addr=0x%x, size=0x%x\r\n", p_candidate_blk->addr, p_candidate_blk->size);
		}
	}
	p_candidate_blk->ref_cnt = 1;
	//DBG_ERR("p_candidate_blk addr=0x%x, size=0x%x\r\n", (int)p_candidate_blk->addr, (int)p_candidate_blk->size);
	return p_candidate_blk->addr;
}

int nvtmpp_init_mmz(void)
{
	UINT32             i;

	if (nvtmpp_parse_mmz_dt_p(&mmz) < 0) {
		return -1;
	}
	if (mmz.ddr_mem[NVTMPP_DDR_1].size == 0) {
		DBG_ERR("Invalid mmz pa = 0x%x, size = 0x%x\r\n", mmz.ddr_mem[NVTMPP_DDR_1].pa, mmz.ddr_mem[NVTMPP_DDR_1].size);
		return -1;
	}
	for (i = 0; i <= NVTMPP_DDR_MAX; i++) {
		if (0 != mmz.ddr_mem[i].pa && 0 != mmz.ddr_mem[i].size) {
			mmz.ddr_mem[i].va = (UINT32)ioremap_cache(mmz.ddr_mem[i].pa, mmz.ddr_mem[i].size);
			if (0 == mmz.ddr_mem[i].va) {
				DBG_ERR("ioremap() failed\r\n");
				return -1;
			}
			mmz.ddr_mem[i].size      = mmz.ddr_mem[i].size;
			DBG_DUMP("nvtmpp_init_mmz: ddr%d pa = 0x%x va = 0x%x , size = 0x%x\n",
				 (int)i+1, (int)mmz.ddr_mem[i].pa,
			     (int)mmz.ddr_mem[i].va, (int)mmz.ddr_mem[i].size);
		}
	}
	return 0;
}

void nvtmpp_exit_mmz(void)
{
	UINT32 i;

	for (i = 0; i <= NVTMPP_DDR_MAX; i++) {
		if (NULL != (void *)mmz.ddr_mem[i].va) {
			iounmap((void *)mmz.ddr_mem[i].va);
		}
	}
}

NVTMPP_MMZ_INFO_S *nvtmpp_get_mmz(void)
{
	return &mmz;
}

EXPORT_SYMBOL(nvtmpp_init_mmz);
EXPORT_SYMBOL(nvtmpp_exit_mmz);
EXPORT_SYMBOL(nvtmpp_get_mmz);
EXPORT_SYMBOL(nvtmpp_sys_va2pa);
EXPORT_SYMBOL(nvtmpp_sys_pa2va);
EXPORT_SYMBOL(nvtmpp_get_fastboot_mem);
EXPORT_SYMBOL(nvtmpp_reg_fastboot_lock_cb);
EXPORT_SYMBOL(nvtmpp_reg_fastboot_unlock_cb);
EXPORT_SYMBOL(nvtmpp_lock_fastboot_blk);
EXPORT_SYMBOL(nvtmpp_unlock_fastboot_blk);
EXPORT_SYMBOL(nvtmpp_fastboot_spin_lock);
EXPORT_SYMBOL(nvtmpp_fastboot_spin_unlock);
EXPORT_SYMBOL(nvtmpp_get_fastboot_pvpool_dts_info);
EXPORT_SYMBOL(nvtmpp_get_fastboot_blk);

#endif
