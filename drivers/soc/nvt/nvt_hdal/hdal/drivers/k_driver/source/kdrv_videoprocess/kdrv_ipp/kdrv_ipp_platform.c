#include "kwrap/error_no.h"
#include "kwrap/cpu.h"
#include "kdrv_ipp_int_dbg.h"
#include "kdrv_ipp_int_util.h"
#if defined(__LINUX)
#include <linux/of.h>
#elif defined(__FREERTOS)
#include "comm/compiler.h"
#include "comm/libfdt.h"
#include <plat/rtosfdt.h>
#endif


UINT32 kdrv_ipp_util_get_dtsi_clock(void)
{
#define KDRV_IPP_UTIL_CLOCK_DFT	(240000000)

#if defined(__LINUX)
	struct device_node* of_node = of_find_node_by_path("/kdrv_ipp");
	UINT32 clk = KDRV_IPP_UTIL_CLOCK_DFT;

	if (of_node) {
		if (of_property_read_u32(of_node, "clock-frequency", &clk) != 0) {
			DBG_WRN("cannot find /kdrv_ipp/clock-frequency\r\n");
		}
	} else {
		DBG_WRN("cannot find node /kdrv_ipp\r\n");
		clk = KDRV_IPP_UTIL_CLOCK_DFT;
	}

	/* return clk unit is MHz */
	return (clk / 1000000);

#elif defined(__FREERTOS)
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();
	int len;
	int nodeoffset;
	const void *nodep;  /* property node pointer */
	UINT32 clk = KDRV_IPP_UTIL_CLOCK_DFT;

	// get linux space
	if (p_fdt != NULL) {
		nodeoffset = fdt_path_offset(p_fdt, "/kdrv_ipp");
		if (nodeoffset < 0) {
			DBG_WRN("failed to offset for  %s = %d \r\n", "/kdrv_ipp", nodeoffset);
		} else {
			nodep = fdt_getprop(p_fdt, nodeoffset, "clock-frequency", &len);
			if (len == 0 || nodep == NULL) {
				DBG_WRN("failed to access clock-frequency.\r\n");
			} else {
				clk = be32_to_cpu(*(unsigned int *)nodep);
			}
		}
	} else {
		DBG_WRN("null fdt_base\r\n");
	}

	/* return clk unit is MHz */
	return (clk / 1000000);
#endif
}