#if defined(__LINUX)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <mach/fmem.h>
#endif

#include "kwrap/cpu.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"
#include "nvtmpp_int.h"
#include "nvtmpp_module.h"
#include "nvtmpp_blk.h"
#include "nvtmpp_pool.h"
#include "nvtmpp_debug.h"
#include "nvtmpp_heap.h"


extern NVTMPP_VB_POOL_S *nvtmpp_vb_get_pool_by_mem_range_p(UINT32 mem_start, UINT32 mem_end);

#if defined(__LINUX)
long nvtmpp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int __user *argp = (int __user *)arg;

	DBG_IND("cmd = 0x%x\r\n", cmd);
	switch (cmd) {

	case NVTMPP_IOC_VB_CONF_SET: {
			NVTMPP_IOC_VB_CONF_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.rtn = nvtmpp_vb_set_conf((NVTMPP_VB_CONF_S *)&msg);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_CONF_GET: {
			NVTMPP_IOC_VB_CONF_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			msg.rtn = nvtmpp_vb_get_conf((NVTMPP_VB_CONF_S *)&msg);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_INIT: {
			NVTMPP_IOC_VB_INIT_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			msg.rtn = nvtmpp_vb_init();
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_EXIT: {
			NVTMPP_IOC_VB_INIT_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			msg.rtn = nvtmpp_vb_exit();
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_RELAYOUT: {
			NVTMPP_IOC_VB_INIT_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			msg.rtn = nvtmpp_vb_relayout();
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_CREATE_POOL: {
			NVTMPP_IOC_VB_CREATE_POOL_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.rtn = nvtmpp_vb_create_pool(msg.pool_name, msg.blk_size, msg.blk_cnt, msg.ddr);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_CREATE_FIXED_POOL: {
			NVTMPP_IOC_VB_CREATE_FIXPOOL_S msg;
			NVTMPP_VB_POOL_S              *p_pool;
			NVTMPP_VB_BLK                  blk;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			p_pool = (NVTMPP_VB_POOL_S *)nvtmpp_sys_create_fixed_pool(msg.pool_name, msg.blk_size, msg.blk_cnt, msg.ddr);
			blk = nvtmpp_vb_get_free_block_from_pool(p_pool, msg.blk_size);
			msg.rtn = nvtmpp_vb_blk2pa(blk);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_DESTROY_POOL: {
			NVTMPP_IOC_VB_DESTROY_POOL_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.rtn = nvtmpp_vb_destroy_pool(msg.pool);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_BLK: {
			NVTMPP_IOC_VB_GET_BLK_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.rtn = nvtmpp_vb_get_block(USER_MODULE, msg.pool, msg.blk_size, msg.ddr);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_REL_BLK: {
			NVTMPP_IOC_VB_REL_BLK_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.rtn = nvtmpp_vb_unlock_block(USER_MODULE, msg.blk);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_BLK_PA: {
			NVTMPP_IOC_VB_GET_BLK_PA_S msg;

			memset((void *)&msg, 0x00, sizeof(msg));
			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.rtn = nvtmpp_vb_blk2pa(msg.blk);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_CACHE_SYNC: {
			NVTMPP_IOC_VB_CACHE_SYNC_S msg = {0};

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			vos_cpu_dcache_sync((VOS_ADDR)msg.virt_addr, msg.size, msg.dma_dir);
		}
		break;
	case NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU: {
			NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU_S msg = {0};

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			if (msg.cpu_count <= 1) {
				vos_cpu_dcache_sync_by_cpu((VOS_ADDR)msg.virt_addr, msg.size, msg.dma_dir, msg.cpu_id);
			} else {
				vos_cpu_dcache_sync((VOS_ADDR)msg.virt_addr, msg.size, msg.dma_dir);
			}
		}
		break;
	case NVTMPP_IOC_VB_PA_TO_POOL: {
			NVTMPP_IOC_VB_PA_TO_POOL_S  msg = {0};
			NVTMPP_VB_POOL_S           *p_pool;
			UINT32                      va;

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			// assume minimum blk size
			va = nvtmpp_sys_pa2va(msg.blk_pa);
			if (va == 0) {
				return -EFAULT;
			}
			p_pool = nvtmpp_vb_get_pool_by_mem_range_p(va, va + sizeof(NVTMPP_VB_BLK_S));
			if (p_pool == NULL) {
				msg.pool = NVTMPP_VB_INVALID_POOL;
				msg.size = 0;
			} else {
				msg.pool = (NVTMPP_VB_POOL) p_pool;
				msg.size = p_pool->blk_size;
			}
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;

	case NVTMPP_IOC_VB_PA_TO_VA: {
			NVTMPP_IOC_VB_PA_TO_VA_S    msg = {0};
			UINT32                      va;

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			va = nvtmpp_sys_pa2va(msg.pa);
			if (va == 0) {
				return -EFAULT;
			}
			msg.va = va;
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_VA_TO_PA: {
			NVTMPP_IOC_VB_VA_TO_PA_S    msg = {0};
			UINT32                      pa;

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			pa = nvtmpp_sys_va2pa(msg.va);
			if (pa == 0) {
				return -EFAULT;
			}
			msg.pa = pa;
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_USER_VA_INFO: {
			NVTMPP_IOC_VB_GET_USER_VA_INFO_S    msg = {0};
			UINT32                              pa;

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			pa = fmem_lookup_pa((u32)msg.va);
			msg.rtn = NVTMPP_ER_OK;
			if (pa == 0xFFFFFFFF) {
				DBG_ERR("Err va = 0x%x\r\n", (int)msg.va);
				msg.rtn = NVTMPP_ER_PARM;
			}
			msg.pa = pa;
			msg.cached = 1;
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;

	case NVTMPP_IOC_VB_GET_BRIDGE_MEM: {
			MEM_RANGE mem_range = {0};
			NVTMPP_GET_SYSMEM_REGION_S msg = {0};

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}

			if (FALSE == nvtmpp_vb_get_bridge_mem(&mem_range)) {
				msg.rtn = NVTMPP_ER_POOL_UNEXIST;
			} else {
				msg.phys_addr = mem_range.addr;
				msg.size = mem_range.size;
				msg.rtn = NVTMPP_ER_OK;
			}

			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_MAX_FREE_BLK_SZ: {
			NVTMPP_IOC_VB_GET_MAX_FREE_S        msg = {0};

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.size = nvtmpp_vb_get_max_free_size(msg.ddr);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_COMM_POOL_RANGE: {
			MEM_RANGE                    mem_range = {0};
			NVTMPP_GET_COMM_POOL_RANGE_S msg = {0};

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			if (FALSE == nvtmpp_vb_get_comm_pool_range(msg.ddr, &mem_range)) {
				msg.rtn = NVTMPP_ER_POOL_UNEXIST;
			} else {
				msg.phys_addr = nvtmpp_sys_va2pa(mem_range.addr);
				msg.size = mem_range.size;
				msg.rtn = NVTMPP_ER_OK;
			}
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;

	case NVTMPP_IOC_VB_GET_FREE_SZ: {
			NVTMPP_IOC_VB_GET_FREE_S        msg = {0};

			if (unlikely(copy_from_user(&msg, argp, sizeof(msg)))) {
				return -EFAULT;
			}
			msg.size = nvtmpp_heap_get_free_size(msg.ddr);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				return -EFAULT;
			}
		}
		break;
	default:
		DBG_ERR("unknown cmd 0x%x\r\n", cmd);
		return -EINVAL;
	}
	return 0;
}
#else
int nvtmpp_ioctl (int fd, unsigned int cmd, void *p_arg)
{
	DBG_IND("cmd = 0x%x\r\n", cmd);
	switch (cmd) {
	case NVTMPP_IOC_VB_CONF_SET: {
			NVTMPP_IOC_VB_CONF_S *p_msg = (NVTMPP_IOC_VB_CONF_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_set_conf((NVTMPP_VB_CONF_S *)p_msg);
		}
		break;
	case NVTMPP_IOC_VB_CONF_GET: {
			NVTMPP_IOC_VB_CONF_S *p_msg = (NVTMPP_IOC_VB_CONF_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_get_conf((NVTMPP_VB_CONF_S *)p_msg);
		}
		break;
	case NVTMPP_IOC_VB_INIT: {
			NVTMPP_IOC_VB_INIT_S *p_msg = (NVTMPP_IOC_VB_INIT_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_init();
		}
		break;
	case NVTMPP_IOC_VB_EXIT: {
			NVTMPP_IOC_VB_INIT_S *p_msg = (NVTMPP_IOC_VB_INIT_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_exit();
		}
		break;
	case NVTMPP_IOC_VB_RELAYOUT: {
			NVTMPP_IOC_VB_INIT_S *p_msg = (NVTMPP_IOC_VB_INIT_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_relayout();
		}
		break;
	case NVTMPP_IOC_VB_CREATE_POOL: {
			NVTMPP_IOC_VB_CREATE_POOL_S *p_msg = (NVTMPP_IOC_VB_CREATE_POOL_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_create_pool(p_msg->pool_name, p_msg->blk_size, p_msg->blk_cnt, p_msg->ddr);
		}
		break;
	case NVTMPP_IOC_VB_CREATE_FIXED_POOL: {
			NVTMPP_IOC_VB_CREATE_FIXPOOL_S *p_msg = (NVTMPP_IOC_VB_CREATE_FIXPOOL_S *)p_arg;
			NVTMPP_VB_POOL_S               *p_pool;
			NVTMPP_VB_BLK                   blk;

			p_pool = (NVTMPP_VB_POOL_S *)nvtmpp_sys_create_fixed_pool(p_msg->pool_name, p_msg->blk_size, p_msg->blk_cnt, p_msg->ddr);
			blk = nvtmpp_vb_get_free_block_from_pool(p_pool, p_msg->blk_size);
			p_msg->rtn = nvtmpp_vb_blk2pa(blk);
		}
		break;
	case NVTMPP_IOC_VB_DESTROY_POOL: {
			NVTMPP_IOC_VB_DESTROY_POOL_S *p_msg = (NVTMPP_IOC_VB_DESTROY_POOL_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_destroy_pool(p_msg->pool);
		}
		break;
	case NVTMPP_IOC_VB_GET_BLK: {
			NVTMPP_IOC_VB_GET_BLK_S *p_msg = (NVTMPP_IOC_VB_GET_BLK_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_get_block(USER_MODULE, p_msg->pool, p_msg->blk_size, p_msg->ddr);
		}
		break;
	case NVTMPP_IOC_VB_REL_BLK: {
			NVTMPP_IOC_VB_REL_BLK_S *p_msg = (NVTMPP_IOC_VB_REL_BLK_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_unlock_block(USER_MODULE, p_msg->blk);
		}
		break;
	case NVTMPP_IOC_VB_GET_BLK_PA: {
			NVTMPP_IOC_VB_GET_BLK_PA_S *p_msg = (NVTMPP_IOC_VB_GET_BLK_PA_S *)p_arg;

			p_msg->rtn = nvtmpp_vb_blk2pa(p_msg->blk);
		}
		break;
	case NVTMPP_IOC_VB_CACHE_SYNC: {
			NVTMPP_IOC_VB_CACHE_SYNC_S *p_msg = (NVTMPP_IOC_VB_CACHE_SYNC_S *)p_arg;

			vos_cpu_dcache_sync((VOS_ADDR)p_msg->virt_addr, p_msg->size, p_msg->dma_dir);
		}
		break;
	case NVTMPP_IOC_VB_PA_TO_POOL: {
			NVTMPP_IOC_VB_PA_TO_POOL_S  *p_msg = (NVTMPP_IOC_VB_PA_TO_POOL_S *)p_arg;
			NVTMPP_VB_POOL_S           *p_pool;
			UINT32                      va;

			// assume minimum blk size
			va = nvtmpp_sys_pa2va(p_msg->blk_pa);
			if (va == 0) {
				DBG_ERR("Err pa = 0x%x\r\n", (int)p_msg->blk_pa);
				return -1;
			}
			p_pool = nvtmpp_vb_get_pool_by_mem_range_p(va, va + sizeof(NVTMPP_VB_BLK_S));
			if (p_pool == NULL) {
				p_msg->pool = NVTMPP_VB_INVALID_POOL;
				p_msg->size = 0;
			} else {
				p_msg->pool = (NVTMPP_VB_POOL) p_pool;
				p_msg->size = p_pool->blk_size;
			}
		}
		break;

	case NVTMPP_IOC_VB_PA_TO_VA: {
			NVTMPP_IOC_VB_PA_TO_VA_S    *p_msg = (NVTMPP_IOC_VB_PA_TO_VA_S *)p_arg;
			UINT32                      va;

			va = nvtmpp_sys_pa2va(p_msg->pa);
			if (va == 0) {
				DBG_ERR("Err pa = 0x%x\r\n", (int)p_msg->pa);
				return -1;
			}
			p_msg->va = va;
		}
		break;
	case NVTMPP_IOC_VB_VA_TO_PA: {
			NVTMPP_IOC_VB_VA_TO_PA_S    *p_msg = (NVTMPP_IOC_VB_VA_TO_PA_S *)p_arg;
			UINT32                      pa;

			pa = nvtmpp_sys_pa2va(p_msg->va);
			if (pa == 0) {
				DBG_ERR("Err va = 0x%x\r\n", (int)p_msg->va);
				return -1;
			}
			p_msg->pa = pa;
		}
		break;
	case NVTMPP_IOC_VB_GET_USER_VA_INFO: {
			NVTMPP_IOC_VB_GET_USER_VA_INFO_S    *p_msg = (NVTMPP_IOC_VB_GET_USER_VA_INFO_S *)p_arg;
			UINT32                              pa;

			pa = vos_cpu_get_phy_addr((unsigned int)p_msg->va);
			if (pa == VOS_ADDR_INVALID) {
				DBG_ERR("Err va = 0x%x\r\n", (int)p_msg->va);
				p_msg->rtn = NVTMPP_ER_PARM;
			} else {
				p_msg->rtn = NVTMPP_ER_OK;
			}
			p_msg->pa = pa;
			p_msg->cached = 1;
		}
		break;
	case NVTMPP_IOC_VB_GET_BRIDGE_MEM: {
			MEM_RANGE mem_range = {0};
			NVTMPP_GET_SYSMEM_REGION_S *p_msg = (NVTMPP_GET_SYSMEM_REGION_S *)p_arg;

			if (FALSE == nvtmpp_vb_get_bridge_mem(&mem_range)) {
				p_msg->rtn = NVTMPP_ER_POOL_UNEXIST;
			} else {
				p_msg->phys_addr = mem_range.addr;
				p_msg->size = mem_range.size;
				p_msg->rtn = NVTMPP_ER_OK;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_MAX_FREE_BLK_SZ: {
			NVTMPP_IOC_VB_GET_MAX_FREE_S       *p_msg = (NVTMPP_IOC_VB_GET_MAX_FREE_S *)p_arg;

			p_msg->size = nvtmpp_vb_get_max_free_size((unsigned int)p_msg->ddr);
		}
		break;
	case NVTMPP_IOC_VB_GET_COMM_POOL_RANGE: {
			MEM_RANGE                           mem_range = {0};
			NVTMPP_GET_COMM_POOL_RANGE_S       *p_msg = (NVTMPP_GET_COMM_POOL_RANGE_S *)p_arg;

			if (FALSE == nvtmpp_vb_get_comm_pool_range(p_msg->ddr, &mem_range)) {
				p_msg->rtn = NVTMPP_ER_POOL_UNEXIST;
			} else {
				p_msg->phys_addr = nvtmpp_sys_va2pa(mem_range.addr);
				p_msg->size = mem_range.size;
				p_msg->rtn = NVTMPP_ER_OK;
			}
		}
		break;
	case NVTMPP_IOC_VB_GET_FREE_SZ: {
			NVTMPP_IOC_VB_GET_FREE_S       *p_msg = (NVTMPP_IOC_VB_GET_FREE_S *)p_arg;

			p_msg->size = nvtmpp_heap_get_free_size((unsigned int)p_msg->ddr);
		}
		break;
	default:
		DBG_ERR("unknown cmd 0x%x\r\n", cmd);
		return -1;
	}
	return 0;
}

#endif

