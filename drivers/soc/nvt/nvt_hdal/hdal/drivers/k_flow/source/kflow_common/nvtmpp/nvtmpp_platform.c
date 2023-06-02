#if defined __KERNEL__
#include <linux/io.h>       // for ioremap
#include <linux/mm.h>
#include <linux/of_device.h>
#include <linux/kernel.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include <kwrap/task.h>
#include "nvtmpp_debug.h"
#include "kflow_common/nvtmpp.h"
#include "nvtmpp_platform.h"

void nvtmpp_dump_stack_by_pid(UINT32 pid)
{
	#if defined __LINUX
	struct task_struct *task_hdl;

	task_hdl = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (NULL == task_hdl) {
		DBG_ERR("Invalid pid %d\r\n", pid);
		return;
	}
	show_stack(task_hdl, NULL);
	#else
	DBG_ERR("Not support\r\n");
	#endif
}

void nvtmpp_dump_stack(void)
{
	return vos_dump_stack();
}

void nvtmpp_dumpmem(UINT32 pa, UINT32 length)
{
	#if defined __LINUX
	UINT32  va = 0;

	if ((pa & 0xF0000000) == 0xF0000000) {
		va = pa;
	} else {
		va = nvtmpp_sys_pa2va(pa);
		if (va == 0) {
			DBG_ERR("Invalid pa 0x%x\r\n", pa);
			return;
		}
	}
	DBG_DUMP("pa 0x%x, va= 0x%x\r\n", pa, va);
	debug_dumpmem(va, length);
	#else
	DBG_ERR("Not support\r\n");
	#endif
}

void nvtmpp_dumpmem2file(UINT32 pa, UINT32 length, char *filename)
{
	#if defined __LINUX
	UINT32  va = 0;
	void    *map_addr = NULL;

	pa &= 0xFFFFFFFC;
	if ((pa & 0xF0000000) == 0xF0000000) {
		map_addr = ioremap_cache(pa, length+16);
		if (NULL == map_addr) {
			printk("ioremap() failed, addr 0x%x, length %d\r\n", pa, length);
			return;
		}
		va = (UINT32)map_addr;
	} else {
		va = nvtmpp_sys_pa2va(pa);
		if (va == 0) {
			DBG_ERR("Invalid pa 0x%x\r\n", pa);
			return;
		}
	}
	DBG_DUMP("pa 0x%x, va= 0x%x\r\n", pa, va);
	debug_dumpmem2file(va, length, filename);
	if (0 != map_addr) {
		iounmap((void *)map_addr);
	}
	#else
	DBG_ERR("Not support\r\n");
	#endif
}

BOOL nvtmpp_vb_get_bridge_mem(MEM_RANGE *p_range)
{
#if defined(__KERNEL__)
	/* device node path - check it from /proc/device-tree/ */
    char *path = "/nvt_memory_cfg/bridge";
    struct device_node *dt_node;
	UINT32 reg[2] = {0};

    dt_node = of_find_node_by_path(path);

	if (!dt_node) {
		DBG_ERR("Failed to find node by path: %s.\r\n", path);
		return FALSE;
    }
	DBG_IND("Found the node for %s.\r\n", path);

	if (of_property_read_u32_array(dt_node, "reg", reg, 2)) {
		return FALSE;
	}
	p_range->addr = reg[0];
	p_range->size = reg[1];
	return TRUE;
#else
	DBG_ERR("not support.\n");
	return FALSE;
#endif
}

void *nvtmpp_vmalloc(unsigned long size)
{
#if defined(__KERNEL__)
	return vmalloc(size);
#else
    return malloc(size);
#endif
}

void nvtmpp_vfree(void *v_buff)
{
#if defined(__KERNEL__)
	vfree(v_buff);
#else
    free(v_buff);
#endif
}


