/**
    NVT hdal memory operation handling
    Add a wrapper to handle the fmem memory handling api and version management API
    @file       nvtmem.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef __NVT_HDAL_NVTMEM_H
#define __NVT_HDAL_NVTMEM_H

#include <linux/mm.h>
#include <linux/list.h>
#include <mach/fmem.h>
#include <asm/io.h>
#include <asm/setup.h>

#define NVT_HDAL_MEM_VERSION	"0.0.1"

struct nvtmem_ver_info_t {
	struct platform_device	*pdev;
	int			id;
	unsigned int 		version;
	unsigned int 		patch_level;
	unsigned int 		sub_level;
	unsigned int		extra_ver;
	struct list_head	list;
};

/* @this function is used to do hdal layer version control sys registeration.
 * @parm: pdev:
 *		platform device node
 * @parm: version:
 * 		format 00.00.00.00
 * @return:
 *		< 0 : Failed
 *		>= 0: success
 */
int nvtmem_version_register(struct platform_device *pdev, char *version);

/* @this function is used to do hdal layer version control sys unregisteration.
 * @parm: pdev:
 *		platform device node
 * @return:
 *		< 0 : Failed
 *		>= 0: success
 */
int nvtmem_version_unregister(struct platform_device *pdev);

/* @this function is used to allocate buffer from cma.
 * @parm: nvt_fmem_info:
 *		You should use nvt_fmem_mem_info_init to initiate nvt_fmem_mem_info_t data structure
 * @return:
 *		Null : Failed
 *		Non-Null: success
 */
void* nvtmem_alloc_buffer(struct nvt_fmem_mem_info_t *nvt_fmem_info);

/* @this function is used to free buffer from cma.
 * @parm: handle:
 *		pointer to allocated handle
 * @return:
 *		< 0 : Failed
 *		>= 0: success
 */
int nvtmem_release_buffer(void *handle);

/* @this function is used to do virtual address to physical address transform.
 * @parm: vaddr:
 *		virtual address
 * @return:
 *		0xFFFFFFFF : Failed
 *		>= 0: success
 */
phys_addr_t nvtmem_va_to_pa(unsigned int vaddr);
#endif /* __NVT_HDAL_NVTMEM_H */
