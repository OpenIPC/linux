#if defined(__LINUX)
#include <linux/of.h>
#include <linux/slab.h>
#elif defined(__FREERTOS)
#include "comm/compiler.h"
#include "comm/libfdt.h"
#include <plat/rtosfdt.h>
#include <string.h>
#include <malloc.h>
#endif
#include "ipp_debug_int.h"
#include "kflow_videoprocess/ctl_ipp_util.h"
#include "ctl_ipp_util_int.h"

#define CTL_IPP_UTIL_CLOCK_DFT	(280)	/* select 280 for ipp to use pll_13 as default clk src */

void *ctl_ipp_util_os_malloc_wrap(UINT32 want_size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(want_size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(want_size);
#endif

	return p_buf;
}

void ctl_ipp_util_os_mfree_wrap(void *p_buf)
{
#if defined(__LINUX)
	kfree(p_buf);
#elif defined(__FREERTOS)
	free(p_buf);
#endif
}

static UINT32 ctl_ipp_util_chip_id = 0;
void ctl_ipp_util_init_chip_id(void)
{
	ctl_ipp_util_chip_id = nvt_get_chip_id();
}

UINT32 ctl_ipp_util_get_chip_id(void)
{
	return ctl_ipp_util_chip_id;
}

INT32 ctl_ipp_util_get_max_stripe(void)
{
	UINT32 chip = ctl_ipp_util_get_chip_id();

	if (chip == CHIP_NA51055) {
		return CTL_IPP_520_MAX_STRIPE_W;
	} else if (chip == CHIP_NA51084) {
		return CTL_IPP_528_MAX_STRIPE_W;
	} else if (chip == CHIP_NA51089) {
		return CTL_IPP_560_MAX_STRIPE_W;
	}
	DBG_WRN("unknown chip 0x%.8x\r\n", chip);

	return CTL_IPP_560_MAX_STRIPE_W;
}
