#include <stdio.h>
#include <vendor_common.h>
#include "mem_hotplug.h"

#define SWAP4(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) & 0xFF000000) >> 24))

static int read_fdt(const char *path, unsigned int *p_addr, unsigned int *p_size)
{
	int fd;
	unsigned int reg[2] = {0};

	if ((fd = open(path, O_RDONLY)) < 0) {
		printf("unable to open %s\n", path);
		return -1;
	}

	if (read(fd, reg, sizeof(reg)) != sizeof(reg)) {
		printf("unable to read reg on %s\n", path);
		close(fd);
		return -1;
	}

	*p_addr = SWAP4(reg[0]);
	*p_size = SWAP4(reg[1]);
	close(fd);
	return 0;
}

static int get_mem_range(unsigned int *p_addr, unsigned int *p_size)
{
	unsigned int fdt_addr, fdt_size;
	unsigned int rtos_addr, rtos_size;
	unsigned int bridge_addr, bridge_size;
	//get fdt addr and size in nvt_memory_cfg
	if (read_fdt("/sys/firmware/devicetree/base/nvt_memory_cfg/fdt/reg", &fdt_addr, &fdt_size) != 0) {
		return -1;
	}
	if (read_fdt("/sys/firmware/devicetree/base/nvt_memory_cfg/rtos/reg", &rtos_addr, &rtos_size) != 0) {
		return -1;
	}
	if (read_fdt("/sys/firmware/devicetree/base/nvt_memory_cfg/bridge/reg", &bridge_addr, &bridge_size) != 0) {
		return -1;
	}

	*p_addr = fdt_addr;
	*p_size = fdt_size + rtos_size + bridge_size;
	return 0;
}

static int do_hotplug(unsigned int mem_begin, unsigned mem_size)
{
	HD_RESULT ret=0;
	VENDOR_LINUX_MEM_HOT_PLUG desc = {0};
	desc.start_addr = mem_begin;
	desc.size = mem_size;
	ret = vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_LINUX_HOT_PLUG, &desc);
	if(ret != 0){
		printf("nvtmem_hotplug_set fail ret:%d\n", (int)ret);
	}
	return 0;
}

HD_RESULT memory_hotplug(void)
{
	unsigned int mem_begin, mem_size;
	// read memory space from bridge memory
	if (get_mem_range(&mem_begin, &mem_size) != 0) {
		return HD_ERR_INIT;
	}
	// do hotplug
	if (do_hotplug(mem_begin, mem_size) != 0) {
		return HD_ERR_SYS;
	}
	return 0;
}
