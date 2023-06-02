#if defined (__KERNEL__)
#include "sie_init_int.h"
#include <linux/of.h>

INT32 sie_init_plat_chk_node(CHAR *node)
{
	struct device_node *of_node;

	of_node = of_find_node_by_path(node);
	if (!of_node) {
		return E_SYS;
    }
	return E_OK;
}

INT32 sie_init_plat_read_dtsi_array(CHAR *node, CHAR *tag, UINT32 *buf, UINT32 num)
{
	struct device_node *of_node;

	of_node = of_find_node_by_path(node);
	if (!of_node) {
		DBG_ERR("Failed to find node\r\n");
		return E_SYS;
    }
	if (of_property_read_u32_array(of_node, tag, (UINT32 *)buf, num)) {
		return E_SYS;
	}
	return E_OK;
}

void *kdrv_sie_builtin_plat_malloc(UINT32 size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kzalloc(size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(size);
#endif

	return p_buf;
}

void kdrv_sie_builtin_plat_free(void *p_buf)
{
#if defined(__LINUX)
	kfree(p_buf);
#elif defined(__FREERTOS)
	free(p_buf);
#endif
}

#endif