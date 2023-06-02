#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/io.h>
#include <kdrv_builtin/kdrv_builtin.h>

// bridge memory description
#define BRIDGE_FOURCC 0x47445242 ///< MAKEFOURCC('B', 'R', 'D', 'G');
#define BRIDGE_MAX_OPT_CNT 128
// bridge memory description !! (DO NOT MODIFY ANY MEMBER IN BRIDGE_DESC and BRIDGE_BOOT_OPTION)
typedef struct _BRIDGE_BOOT_OPTION {
	unsigned int tag; //a fourcc tag
	unsigned int val; //the value
} BRIDGE_BOOT_OPTION;

typedef struct _BRIDGE_DESC {
	unsigned int bridge_fourcc;     ///< always BRIDGE_FOURCC
	unsigned int bridge_size;       ///< sizeof(BRIDGE_DESC) for check if struct match on rtos and linux
	unsigned int phy_addr;          ///< address of bridge memory described on fdt
	unsigned int phy_size;          ///< size of whole bridge memory described on fdt
	BRIDGE_BOOT_OPTION opts[BRIDGE_MAX_OPT_CNT]; ///< boot options from rtos
} BRIDGE_DESC;

//element's fourcc
#define JPG_FRAME_OFS      0x534F504A //MAKEFOURCC('J', 'P', 'O', 'S');
#define JPG_FRAME_SIZE     0x5A53504A //MAKEFOURCC('J', 'P', 'S', 'Z');

static BRIDGE_DESC *mp_bridge = NULL;

int kdrv_bridge_get_tag(unsigned int tag, unsigned int *p_val)
{
	int i;

	if (mp_bridge == NULL) {
		pr_err("mp_bridge is NULL\n");
		return -1;
	}

	for (i = 0; i < BRIDGE_MAX_OPT_CNT; i++) {
		if (mp_bridge->opts[i].tag == tag) {
			*p_val = mp_bridge->opts[i].val;
			return 0;
		} else if (mp_bridge->opts[i].tag == 0) {
			pr_err("unable to find tag: 0x%08X\n", tag);
			return -1;
		}
	}
	pr_err("bridge opts are full.\n");
	return -2;
}

int kdrv_bridge_map(void)
{
	u32 bridge[2] = {0};
	const char path[] = "/nvt_memory_cfg/bridge";
	struct device_node* of_node = of_find_node_by_path(path);

	if (!kdrv_builtin_is_fastboot()) {
		pr_err("unable to map bridge without fastboot mode\n");
		return 0;
	}

	if (mp_bridge) {
		pr_err("bridge has mapped\n");
		return 0;
	}

	if (!of_node) {
		pr_err("unable to find %s\n", path);
		return -1;
	}
	if (of_property_read_u32_array(of_node, "reg", bridge, 2) != 0) {
		pr_err("unable to read %s/reg\n", path);
		return -1;
	}

	mp_bridge = (BRIDGE_DESC *)ioremap_cache(bridge[0], bridge[1]);

	if (mp_bridge->bridge_fourcc != BRIDGE_FOURCC) {
		pr_err("invalid bridge_fourcc\n");
		iounmap((void *)mp_bridge);
		mp_bridge = NULL;
		return -1;
	}

	if (mp_bridge->bridge_size != sizeof(BRIDGE_DESC)) {
		pr_err("bridge_size not matched\n");
		iounmap((void *)mp_bridge);
		mp_bridge = NULL;
		return -1;
	}
	return 0;
}

int kdrv_bridge_unmap(void)
{
	if (mp_bridge) {
		iounmap((void *)mp_bridge);
		mp_bridge = NULL;
	}
	return 0;
}

#else
#endif
