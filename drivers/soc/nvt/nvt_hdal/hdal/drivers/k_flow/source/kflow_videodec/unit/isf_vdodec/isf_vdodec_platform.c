#include "kflow_videodec/isf_vdodec_platform.h"
#include <linux/uaccess.h>

BOOL isf_vdodec_copy_from_user(void *p_dest, void *p_src, UINT32 size)
{
	// copy from user
	if (unlikely(copy_from_user(p_dest, p_src, size))) {
		return FALSE;
	}

	return TRUE;
}