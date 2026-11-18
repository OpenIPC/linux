/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "osal_ax.h"


void *AX_OSAL_MEM_kmalloc(unsigned int size, unsigned int osal_gfp_flag)
{
    if (osal_gfp_flag == AX_OSAL_GFP_KERNEL) {
        return kmalloc(size, GFP_KERNEL);
    } else if (osal_gfp_flag == AX_OSAL_GFP_ATOMIC) {
        return kmalloc(size, GFP_ATOMIC);
    }

    return kmalloc(size, GFP_KERNEL);
}
EXPORT_SYMBOL(AX_OSAL_MEM_kmalloc);

void *AX_OSAL_MEM_kzalloc(unsigned int size, unsigned int osal_gfp_flag)
{
    if (osal_gfp_flag == AX_OSAL_GFP_KERNEL) {
        return kzalloc(size, GFP_KERNEL);
    } else if (osal_gfp_flag == AX_OSAL_GFP_ATOMIC) {
        return kzalloc(size, GFP_ATOMIC);
    }

    return kzalloc(size, GFP_KERNEL);
}
EXPORT_SYMBOL(AX_OSAL_MEM_kzalloc);

void AX_OSAL_MEM_kfree(const void *addr)
{
    kfree(addr);
}
EXPORT_SYMBOL(AX_OSAL_MEM_kfree);

void *AX_OSAL_MEM_vmalloc(unsigned int size)
{
    return vmalloc(size);
}
EXPORT_SYMBOL(AX_OSAL_MEM_vmalloc);

void AX_OSAL_MEM_vfree(const void *addr)
{
    vfree(addr);
}
EXPORT_SYMBOL(AX_OSAL_MEM_vfree);

int AX_OSAL_MEM_VirtAddrIsValid(unsigned long vm_start, unsigned long vm_end)
{
    struct vm_area_struct *pvma1;
    struct vm_area_struct *pvma2;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
    mmap_read_lock(current->mm);
#else
   down_read(&current->mm->mmap_sem);
#endif
    pvma1 = find_vma(current->mm, vm_start);
    if (NULL == pvma1) {
        printk(" user vaddr 1 is null. user add = 0x%lx\n", vm_start);
        goto badAddr;
    }

    pvma2 = find_vma(current->mm, vm_end - 1);
    if (NULL == pvma2) {
        printk(" user vaddr 2 is null. user add = 0x%lx\n", vm_start);
        goto badAddr;
    }

    if (pvma1 != pvma2) {
        printk(" user vaddr:[0x%lx,0x%lx) and user vaddr:[0x%lx,0x%lx) are not equal\n",
                       pvma1->vm_start, pvma1->vm_end, pvma2->vm_start, pvma2->vm_end);
        goto badAddr;
    }

    if (!(pvma1->vm_flags & VM_WRITE)) {
        printk("ERROR vma flag:0x%lx\n", pvma1->vm_flags);
        goto badAddr;
    }

    if (pvma1->vm_start > vm_start) {
        printk("cannot find corresponding vma, vm[%lx, %lx], user range[%lx,%lx]\n", pvma1->vm_start, pvma1->vm_end,
                       vm_start, vm_end);
        goto badAddr;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
    mmap_read_unlock(current->mm);
#else
    up_read(&current->mm->mmap_sem);
#endif
    return 0;
badAddr:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
    mmap_read_unlock(current->mm);
#else
    up_read(&current->mm->mmap_sem);
#endif
    return -1;
}
EXPORT_SYMBOL(AX_OSAL_MEM_VirtAddrIsValid);

unsigned long AX_OSAL_MEM_AddrMmap(void *file, unsigned long addr,
	unsigned long len, unsigned long prot, unsigned long flag, unsigned long offset)
{
    return vm_mmap((struct file *)file, addr, len, prot, flag, offset);
}
EXPORT_SYMBOL(AX_OSAL_MEM_AddrMmap);

int AX_OSAL_MEM_AddrMunmap(unsigned long start, unsigned int size)
{
    return vm_munmap(start, size);
}
EXPORT_SYMBOL(AX_OSAL_MEM_AddrMunmap);
