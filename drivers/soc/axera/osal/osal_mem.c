/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/cache.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kexec.h>
#include <linux/libfdt.h>
#include <linux/mman.h>
#include <linux/nodemask.h>
#include <linux/memblock.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include <asm/barrier.h>
#include <asm/cputype.h>
#include <asm/fixmap.h>
#include <linux/kasan.h>
#ifdef CONFIG_ARM64
#include <asm/kernel-pgtable.h>
#endif
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>
#include <asm/ptdump.h>
#include <asm/tlbflush.h>
#include <linux/version.h>

#include "osal_ax.h"
#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"

unsigned long AX_OSAL_DEV_usr_virt_to_phys(unsigned long virt)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	unsigned int cacheable = 0;
	unsigned int val = 0;
	unsigned long page_addr = 0;
	unsigned long page_offset = 0;
	unsigned long phys_addr = 0;

	if (virt > TASK_SIZE) {
		printk("Illegal user address\n");
		return 0;
	}

	if (virt & 0x3) {
		pr_info("invalid virt addr 0x%08lx[not 4 bytes align]\n", virt);
		return 0;
	}

	if (virt >= PAGE_OFFSET) {
		pr_info("invalid user space virt addr 0x%08lx\n", virt);
		return 0;
	}

	pgd = pgd_offset(current->mm, virt);

	if (pgd_none(*pgd)) {
		pr_info("no mapped in pg,dvirt addr 0x%08lx\n", virt);
		return 0;
	}

	p4d = p4d_offset(pgd, virt);
	if (p4d_none(*p4d)) {
		pr_info("no mapped in p4d,dvirt addr 0x%08lx\n", virt);
		return 0;
	}

	pud = pud_offset(p4d, virt);

	if (pud_none(*pud)) {
		pr_info(" not mapped in pud! addr 0x%08lx\n", virt);
		return 0;
	}

	pmd = pmd_offset(pud, virt);

	if (pmd_none(*pmd)) {
		pr_info(" not mapped in pmd! addr 0x%08lx\n", virt);
		return 0;
	}

	pte = pte_offset_map(pmd, virt);

	if (pte_none(*pte)) {
		pr_info(" not mapped in pte!addr 0x%08lx\n", virt);
		pte_unmap(pte);
		return 0;
	}

	page_addr = (pte_val(*pte) & PHYS_MASK) & PAGE_MASK;
	page_offset = virt & ~PAGE_MASK;
	phys_addr = page_addr | page_offset;
#ifdef CONFIG_ARM64
	/* bit[4:2] */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	val = (pte_val(*pte) & 0x1c) >> 2;
	if (val == 0 || val == 1) {
		cacheable = 1;
	}
#else
	if (pte_val(*pte) & (1 << 4)) {
		cacheable = 1;
	}
#endif

#else
	if (pte_val(*pte) & (1 << 3)) {
		cacheable = 1;
	}
#endif
	/*
	 * phys_addr: the lowest bit indicates its cache attribute
	 * 1: cacheable
	 * 0: uncacheable
	 */
	phys_addr |= cacheable;

	pte_unmap(pte);

	return phys_addr;
}

EXPORT_SYMBOL(AX_OSAL_DEV_usr_virt_to_phys);

unsigned long AX_OSAL_DEV_kernel_virt_to_phys(void *virt)
{
#ifdef CONFIG_ARM64
	unsigned long vaddr = (unsigned long)virt;
        unsigned long offset = vaddr & ~PAGE_MASK;
	if (vaddr <= TASK_SIZE) {
		printk("Illegal kernel address\n");
		return 0;
	} else if (__is_lm_address(vaddr)) {
		return virt_to_phys(virt);
	} else if (is_vmalloc_addr(virt)) {
		return page_to_phys(vmalloc_to_page(virt)) | offset;
	} else {
		printk("unknow kernel address\n");
		return 0;
	}
#endif
}
EXPORT_SYMBOL(AX_OSAL_DEV_kernel_virt_to_phys);

