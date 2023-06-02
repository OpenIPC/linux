#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/mmzone.h>
#include <linux/memory.h>
#include <linux/kallsyms.h>
#include <linux/nodemask.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <asm/cputype.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/glue-cache.h>
#include <asm/outercache.h>

/* 
 * cache functions.
 * The following functions come from arch/arm/include/asm/glue-cache.h
 */
static inline u32 get_cpu_id(void)
{
    static u32 cpu_id = 0;
    
    if (!cpu_id)
       cpu_id = (read_cpuid_id() >> 0x4) & 0xFFF;
    
    return cpu_id;
}
void inline fmem_flush_icache_all(void)
{
    extern void fa726te_flush_icache_all(void);
    extern void fa_flush_icache_all(void);
    
    if (get_cpu_id() == 0x726)
        fa726te_flush_icache_all();
    else
        fa_flush_icache_all();
}

void inline fmem_flush_kern_cache_all(void)
{
    extern void fa726te_flush_kern_cache_all(void);
    extern void fa_flush_kern_cache_all(void);
    
    if (get_cpu_id() == 0x726)
        fa726te_flush_kern_cache_all();
    else
        fa_flush_kern_cache_all();
}

void inline fmem_flush_user_cache_all(void)
{
    extern void fa726te_flush_user_cache_all(void);
    extern void fa_flush_user_cache_all(void);
    
    if (get_cpu_id() == 0x726)
        fa726te_flush_user_cache_all();
    else
        fa_flush_user_cache_all();
}

void inline fmem_flush_user_cache_range(unsigned long start, unsigned long end, unsigned int flags)
{
    extern void fa726te_flush_user_cache_range(unsigned long, unsigned long, unsigned int);
    extern void fa_flush_user_cache_range(unsigned long, unsigned long, unsigned int);
    
    if (get_cpu_id() == 0x726)
        fa726te_flush_user_cache_range(start, end, flags);
    else
        fa_flush_user_cache_range(start, end, flags);
}

void inline fmem_coherent_kern_range(unsigned long start, unsigned long end)
{
    extern void fa726te_coherent_kern_range(unsigned long, unsigned long);
    extern void fa_coherent_kern_range(unsigned long, unsigned long);
    
    if (get_cpu_id() == 0x726)
        fa726te_coherent_kern_range(start, end);
    else
        fa_coherent_kern_range(start, end);
}

void inline fmem_coherent_user_range(unsigned long start, unsigned long end)
{
    extern void fa726te_coherent_user_range(unsigned long, unsigned long);
    extern void fa_coherent_user_range(unsigned long, unsigned long);
    
    if (get_cpu_id() == 0x726)
        fa726te_coherent_user_range(start, end);
    else
        fa_coherent_user_range(start, end);
}

void inline fmem_flush_kern_dcache_area(void *addr, size_t size)
{
    extern void fa726te_flush_kern_dcache_area(void *, size_t);
    extern void fa_flush_kern_dcache_area(void *, size_t);
    
    if (get_cpu_id() == 0x726)
        fa726te_flush_kern_dcache_area(addr, size);
    else
        fa_flush_kern_dcache_area(addr, size);
}

void inline fmem_dma_map_area(const void *start, size_t size, int dir)
{
    extern void fa726te_dma_map_area(const void *, size_t, int);
    extern void fa_dma_map_area(const void *, size_t, int);
    
    if (get_cpu_id() == 0x726)
        fa726te_dma_map_area(start, size, dir);
    else
        fa_dma_map_area(start, size, dir);
}

void inline fmem_dma_unmap_area(const void *start, size_t size, int dir)
{
    extern void fa726te_dma_unmap_area(const void *, size_t, int);
    extern void fa_dma_unmap_area(const void *, size_t, int);
    
    if (get_cpu_id() == 0x726)
        fa726te_dma_unmap_area(start, size, dir);
    else
        fa_dma_unmap_area(start, size, dir);
}

void inline fmem_dma_flush_range(const void *start, const void *end)
{
    extern void fa726te_dma_flush_range(const void *, const void *);
    extern void fa_dma_flush_range(const void *, const void *);
    
    if (get_cpu_id() == 0x726)
        fa726te_dma_flush_range(start, end);
    else
        fa_dma_flush_range(start, end);
}

/* ----------------------------------------------------------------------------
 * processor functions.
 * The following functions come from arch/arm/include/asm/glue-proc.h
 * ----------------------------------------------------------------------------
 */
 
/*
 * Set up any processor specifics
 */ 
void inline cpu_fmem_proc_init(void)
{
    extern void cpu_fa726te_proc_init(void);
    extern void cpu_fa626te_proc_init(void);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_proc_init();
    else
        cpu_fa626te_proc_init();         
}

/*
 * Disable any processor specifics
 */
void inline cpu_fmem_proc_fin(void)
{
    extern void cpu_fa726te_proc_fin(void);
    extern void cpu_fa626te_proc_fin(void);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_proc_fin();
    else
        cpu_fa626te_proc_fin();      
}

/*
 * Special stuff for a reset
 */
void inline cpu_fmem_reset(unsigned long addr)
{
    volatile int loop = 100;
    extern void cpu_fa726te_reset(unsigned long);
    extern void cpu_fa626te_reset(unsigned long);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_reset(addr);
    else
        cpu_fa626te_reset(addr);     

    /* never return */
    while (loop == 100) {};    
}

/*
 * Idle the processor
 */
int inline cpu_fmem_do_idle(void)
{
    extern int cpu_fa726te_do_idle(void);
    extern int cpu_fa626te_do_idle(void);
    
    if (get_cpu_id() == 0x726)
        return cpu_fa726te_do_idle();
    else
        return cpu_fa626te_do_idle();    
}

/*
 * clean a virtual address range from the
 * D-cache without flushing the cache.
 */
void inline cpu_fmem_dcache_clean_area(void *addr, int size)
{
    extern void cpu_fa726te_dcache_clean_area(void *addr, int size);
    extern void cpu_fa626te_dcache_clean_area(void *addr, int size);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_dcache_clean_area(addr, size);
    else
        cpu_fa626te_dcache_clean_area(addr, size);         
}

/*
 * Set the page table
 */
void inline cpu_fmem_switch_mm(unsigned long pgd_phys, struct mm_struct *mm)
{
    extern void cpu_fa726te_switch_mm(unsigned long, struct mm_struct *);
    extern void cpu_fa626te_switch_mm(unsigned long, struct mm_struct *);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_switch_mm(pgd_phys, mm);
    else
        cpu_fa626te_switch_mm(pgd_phys, mm);
}

/*
 * Set a possibly extended PTE.  Non-extended PTEs should
 * ignore 'ext'.
 */
#ifdef CONFIG_ARM_LPAE 
void inline cpu_fmem_set_pte_ext(pte_t *ptep, pte_t pte)
{
    extern void cpu_fa726te_set_pte_ext(pte_t *ptep, pte_t pte);
    extern void cpu_fa626te_set_pte_ext(pte_t *ptep, pte_t pte);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_set_pte_ext(ptep, pte);
    else
        cpu_fa626te_set_pte_ext(ptep, pte);       
}
#else
void inline cpu_fmem_set_pte_ext(pte_t *ptep, pte_t pte, unsigned int ext)
{
    extern void cpu_fa726te_set_pte_ext(pte_t *ptep, pte_t pte, unsigned int ext);
    extern void cpu_fa626te_set_pte_ext(pte_t *ptep, pte_t pte, unsigned int ext);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_set_pte_ext(ptep, pte, ext);
    else
        cpu_fa626te_set_pte_ext(ptep, pte, ext);
}
#endif /* CONFIG_ARM_LPAE */

/* Suspend/resume */
void inline cpu_fmem_do_suspend(void *ptr)
{
#ifdef CONFIG_PM_SLEEP    
    extern void cpu_fa726te_do_suspend(void *);
    extern void cpu_fa626te_do_suspend(void *);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_do_suspend(ptr);
    else
        cpu_fa626te_do_suspend(ptr);    
#endif        
}

/* Suspend/resume */
void inline cpu_fmem_do_resume(void *ptr)
{
#ifdef CONFIG_PM_SLEEP    
    extern void cpu_fa726te_do_resume(void *);
    extern void cpu_fa626te_do_resume(void *);
    
    if (get_cpu_id() == 0x726)
        cpu_fa726te_do_resume(ptr);
    else
        cpu_fa626te_do_resume(ptr);
#endif        
}
