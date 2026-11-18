/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/fs.h>
#include <asm/uaccess.h>

#include "osal_ax.h"

void *AX_OSAL_FS_filp_open(const char * filename, int flags, int mode)
{
	struct file *filp = filp_open(filename, flags, mode);
	return (IS_ERR(filp)) ? NULL : filp;
}

EXPORT_SYMBOL(AX_OSAL_FS_filp_open);

void AX_OSAL_FS_filp_close(void * filp)
{
	if (filp) {
		filp_close(filp, NULL);
	}
	return;
}

EXPORT_SYMBOL(AX_OSAL_FS_filp_close);

int AX_OSAL_FS_filp_write(char * buf, int len, void * k_file)
{
	int writelen = 0;
	struct file *filp;

	if (k_file == NULL) {
		return -ENOENT;
	}

	filp = (struct file *)k_file;

	writelen = kernel_write(filp, buf, len, &filp->f_pos);
	return writelen;
}

EXPORT_SYMBOL(AX_OSAL_FS_filp_write);

int AX_OSAL_FS_filp_read(char * buf, int len, void * k_file)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_FS_filp_read);
