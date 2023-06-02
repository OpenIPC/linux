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
#include <asm/signal.h>
#include <asm/cputype.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/glue-cache.h>
#include <asm/outercache.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <mach/fmem.h>
#include <mach/platform/board.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>

void debug_printk(const char *f, ...);
static int __init sanity_check_memhole(u32 *base);

/* MACROs defintion
 */
#define ALLOC_BSZ         SZ_8M
#define POOL_LIST(x)      g_page_info.list[x]
#define POOL_NODE_SZ(x)   g_page_info.node_sz[x]
#define POOL_PHY_START(x) g_page_info.phy_start[x]
#define MAX_PAGE_NUM      128 /* 1G bytes */
#define CACHE_ALIGN_MASK  0x1F  //cache line with 32 bytes

/*
 * Local variables declaration
 */
static g_page_info_t    g_page_info;
static struct meminfo mem_info, gmmem_info;
u32 page_array[2][MAX_PAGE_NUM] __initdata;
u32 reserve_blk_cnt[2] __initdata;
u32 nonlinear_cpuaddr_flush = 0;
u32 cpuaddr_flush = 0;
u32 va_not_32align = 0, length_not_32align = 0;
u32 debug_counter = 0;

#define FA726_START_ADDR    0x1000000

unsigned long phys_offset = FA726_START_ADDR;  //16M as the start address
EXPORT_SYMBOL(phys_offset);

#define DEBUG_WRONG_CACHE_API   0x0
#define DEBUG_WRONG_CACHE_VA    0x1
#define DEBUG_WRONG_CACHE_LEN   0x2

/* proc function
 */
static struct proc_dir_entry *fmem_proc_root = NULL;
static struct proc_dir_entry *resolve_proc = NULL;
static struct proc_dir_entry *counter_proc = NULL;

static int cpu_id = 0;

#ifdef CONFIG_PLATFORM_GM8210
/* pcie data abort */
static int fmem_pcie_abort_handler(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
    static unsigned long last_fail_addr = 0, fail_cnt = 0;
    u32 paddr = 0;

    /* user mode return unhandled */
    if ((regs->ARM_cpsr & 0xf) == 0)
        return 1;

    paddr = fmem_lookup_pa((u32)addr);
    if (paddr == 0xFFFFFFFF)
        return 1;

    if (paddr >= 0xA0000000) {
        if (last_fail_addr == addr) {
            fail_cnt ++;
        } else {
            last_fail_addr = addr;
            fail_cnt = 0;
        }

        /* fail to rescue */
        if (fail_cnt >= 1000)
            return 1;

        printk("%s, vaddr:0x%x(paddr:0x%x), fsr:0x%x, external abort on non-linefetch! \n",
                                                 __func__, (u32)addr, paddr, fsr);
        return 0;
    }

    return 1;
}
#endif /* CONFIG_PLATFORM_GM8210 */

static inline int __get_cpu_id(void)
{
    if (!cpu_id)
        cpu_id = (read_cpuid_id() >> 0x4) & 0xFFF;

    return cpu_id;
}

/* counter info
 */
static int proc_read_counter(char *page, char **start, off_t off, int count, int *eof,
                                    void *data)
{
    int len = 0;

    len += sprintf(page + len, "linear cpu_addr flush: %d \n", cpuaddr_flush);
    len += sprintf(page + len, "non-linear cpu_addr flush: %d \n", nonlinear_cpuaddr_flush);
    len += sprintf(page + len, "vaddr not cache alignment: %d \n", va_not_32align);
    len += sprintf(page + len, "len not cache alignment: %d \n", length_not_32align);
    len += sprintf(page + len, "debug_counter = 0x%x \n", debug_counter);

    return len;
}

/* debug counter */
int proc_write_debug_counter(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned char value[30];

    if (copy_from_user(value, buffer, count))
        return 0;

    sscanf(value, "%x\n", &debug_counter);


    printk("debug counter: 0x%x \n", debug_counter);

    return count;
}

/* address resolve
 */
int proc_resolve_pa(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned char value[30];
    unsigned int vaddr;
    phys_addr_t  paddr;

    if (copy_from_user(value, buffer, count))
        return 0;

    sscanf(value, "%x\n", &vaddr);
    paddr = fmem_lookup_pa(vaddr);

    printk("Resolve vaddr: 0x%x ---> paddr: 0x%x \n", vaddr, paddr);

    return count;
}

void fmem_proc_init(void)
{
    struct proc_dir_entry *p;

    p = create_proc_entry("fmem", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (p == NULL) {
        printk("%s, fail! \n", __func__);
        return;
    }

    fmem_proc_root = p;

    /* resolve, va->pa
     */
    resolve_proc = create_proc_entry("resolve", S_IRUGO, fmem_proc_root);
    if (resolve_proc == NULL)
        panic("FMEM: Fail to create proc resolve_proc!\n");
    resolve_proc->read_proc = NULL;
    resolve_proc->write_proc = proc_resolve_pa;

    /* counter
     */
    counter_proc = create_proc_entry("counter", S_IRUGO, fmem_proc_root);
    if (counter_proc == NULL)
        panic("FMEM: Fail to create proc counter_proc!\n");
    counter_proc->read_proc = (read_proc_t *)proc_read_counter;
    counter_proc->write_proc = proc_write_debug_counter;
}

/*
 * PAGE Functions
 */
static int __init page_array_cmp(const void *_a, const void *_b)
{
    u32 data_a, data_b;
	int cmp;

	data_a = *(u32 *)_a;    /* the address of page */
	data_b = *(u32 *)_b;

	cmp = page_to_phys((struct page *)data_a) - page_to_phys((struct page *)data_b);

	return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}

static int __init meminfo_cmp(const void *_a, const void *_b)
{
	const struct membank *a = _a, *b = _b;
	long cmp = bank_pfn_start(a) - bank_pfn_start(b);

	return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}

/* Parse early memory
 */
static void __init parse_early_mem(char *p)
{
	unsigned long size, nr_bank;
	phys_addr_t start;
	char *endp;

	start = PHYS_OFFSET;
	size  = memparse(p, &endp);
	if (*endp == '@')
		start = memparse(endp + 1, NULL);

    nr_bank = mem_info.nr_banks;
    mem_info.bank[nr_bank].start = start;
    mem_info.bank[nr_bank].size  = size;
    mem_info.nr_banks ++;
}

/* Parse early GM memory
 */
static void __init parse_early_gmmem(char *p)
{
	unsigned long size, nr_bank;
	char *endp;

	size  = memparse(p, &endp);
    nr_bank = gmmem_info.nr_banks;
    gmmem_info.bank[nr_bank].size  = size;

    gmmem_info.nr_banks ++;
}

#ifdef CONFIG_ARCH_SPARSEMEM_ENABLE
/* This function calculates the max MAX_PHYSMEM_BITS in memory.h
 *
 * MAX_PHYSMEM_BITS: The number of physical address bits required to address
 * the last byte of memory.
 */
unsigned int fmem_max_physmem_bits(void)
{
    unsigned int max_physmem_bits;
    unsigned long phyaddr;
    int nr_banks;

    nr_banks = mem_info.nr_banks;
	phyaddr = bank_pfn_end(&mem_info.bank[nr_banks - 1]) << PAGE_SHIFT;

	max_physmem_bits = get_order(phyaddr) + PAGE_SHIFT;


    return max_physmem_bits;
}
#endif /* CONFIG_ARCH_SPARSEMEM_ENABLE */

/* calculate how many blocks in the page array
 */
static int __init get_early_memblk_count(u32 *page_array)
{
    int i = 0;

    do {
        if (!page_array[i])
            break;
        i ++;
    } while (1);

    return i;
}

/* Purpose: to get whole pages from buddy system as possible for the 2nd DDR.
 * This function is called early once the buddy system is ready.
 */
void __init fmem_early_init(void)
{
    struct page *page;
    dma_addr_t  pfn;
    int i, idx, order, size, ddr0_cnt;

    memset(&reserve_blk_cnt[0], 0, sizeof(reserve_blk_cnt));

    order = get_order(ALLOC_BSZ);

    if (order > MAX_ORDER)
        panic("%s, max order: %d, alloc order: %d \n", __func__, MAX_ORDER, order);

    /* do nothing for single bank */
    if (mem_info.nr_banks == 1)
        goto ddr0_alloc;

    idx = 1;
    i = 0;
    do {
        /* allocate the first page and store all page pointer inside */
        page = alloc_pages_node(1, GFP_ATOMIC, order);
        if (!page)
            panic("%s, get null page for ddr%d! \n", __func__, idx);

        page_array[idx][i] = (u32)page;
        /* check if the physical of the allocated page is smaller than the 2nd bank */
        pfn = __phys_to_pfn(page_to_phys(page));
        if (pfn < bank_pfn_start(&mem_info.bank[1])) {
            page_array[idx][i] = 0x0;
            __free_pages(page, order);
            break;
        }
        /* keep page pointer */
        page_array[idx][i] = (u32)page;

        i ++;
        if (i >= MAX_PAGE_NUM)
            panic("%s, MAX_PAGE_NUM: %d is too small for ddr%d! \n", __func__, MAX_PAGE_NUM, idx);
    } while (1);

    //keep total blocks
    reserve_blk_cnt[idx] = i;

    /* sort the pages in ascendant order */
    sort(page_array[idx], i, sizeof(u32), page_array_cmp, NULL);

ddr0_alloc:
    idx = 0;
    /* allocate memory for node 0 */
    size = gmmem_info.bank[0].size ? gmmem_info.bank[0].size : DDR0_FMEM_SIZE;
    ddr0_cnt = (size + ALLOC_BSZ - 1) >> (order + PAGE_SHIFT);
    //keep total blocks
    reserve_blk_cnt[idx] = ddr0_cnt;

    i = 0;
    do {
        page = alloc_pages_node(0, GFP_ATOMIC, order);
        if (page == NULL) {
            page_array[idx][i] = 0x0;
            printk("%s, requested memory size: 0x%x is too large! \n", __func__, size);
            break;
        }

        page_array[idx][i] = (u32)page;
        i ++;
        if (i >= MAX_PAGE_NUM)
            panic("%s, MAX_PAGE_NUM: %d is too small for ddr%d! \n", __func__, MAX_PAGE_NUM, idx);
    } while (-- ddr0_cnt);

    /* sort the pages */
    sort(page_array[idx], i, sizeof(u32), page_array_cmp, NULL);

    return;
}

/*
 * Parse the memory tag from command line
 */
void __init fmem_fixup_memory(struct tag *tag, char **cmdline, struct meminfo *mi)
{
    char key[] = "mem=", *from, *command_line = NULL;
    struct tag  *t = tag;
    char gm_key[] = "gmmem=";
    int  i, tag_found = 0;

    memset(&mem_info, 0, sizeof(mem_info));
    memset(&gmmem_info, 0, sizeof(gmmem_info));

    if (tag->hdr.tag == ATAG_CORE) {
        for (; t->hdr.size; t = tag_next(t)) {
            if (t->hdr.tag == ATAG_CMDLINE) {
                command_line = &t->u.cmdline.cmdline[0];
                from = command_line;
                tag_found = 1;
                break;
            }
        }
    }

    /* not found the boot argument parameters from UBOOT */
    if (!tag_found) {
        command_line = *cmdline;
        from = *cmdline;
    }

    for (i = 0; i < strlen(command_line) - strlen(key); i ++) {
        if (memcmp(from, key, strlen(key)) == 0) {
			from += strlen(key);
			parse_early_mem(from);
		}
		else if (memcmp(from, gm_key, strlen(gm_key)) == 0)	{
		    from += strlen(gm_key);
			parse_early_gmmem(from);
		}
		else
			from ++;
    }


    sort(&mem_info.bank, mem_info.nr_banks, sizeof(mem_info.bank[0]), meminfo_cmp, NULL);
}

/*
 * @brief get the page with largest physical address and the page with smallest physical address.
 * @function void get_first_last_page(struct list_head *glist, page_node_t ** small_page,
                         page_node_t ** large_page)
 * @return None.
 */
void get_first_last_page(struct list_head *glist, page_node_t ** small_page,
                         page_node_t ** large_page)
{
    struct list_head *list;
    /* the list was sorted in ascendant sequence.
     * Get the page node with the smallest physical address
     */
    list = glist->next;
    *small_page = (page_node_t *)list_entry(list, page_node_t, list);
    /* get the page node with the largest physical address */
    list = glist->prev;
    *large_page = (page_node_t *)list_entry(list, page_node_t, list);
}

/*
 * @brief return the memory to the kernel from frammap
 *
 * @function int fmem_give_pages(int node_id, unsigned int give_sz)
 * @param node_id is the real node id instead of chunk id
 * @param node_id is the size you want to return to the kernel
 * @return the real size that gives to the kernel
 */
int fmem_give_pages(int bank, unsigned int give_sz)
{
    unsigned int ret_sz, size, tmp_sz;
    page_node_t *large_page, *small_page;
    struct page *page;
    u32 phyaddr;

    if (give_sz == 0)
        return 0;

    ret_sz = 0;

    do {
        get_first_last_page(&POOL_LIST(bank), &small_page, &large_page);
        size = (give_sz > large_page->size) ? large_page->size : give_sz;
        tmp_sz = size;

        /* calculate the last page. Because the page address is embedded in section. So
         * I am not sure whether we can get the page address by shift.
         */
        phyaddr = page_to_phys(large_page->page) + large_page->size - PAGE_SIZE;

        do {
            page = phys_to_page(phyaddr);
            ClearPageReserved(page);
            __free_page(page);

            /* No matter wether the page is freed, we need to decrease the page size
             */
            large_page->size -= PAGE_SIZE;
            phyaddr -= PAGE_SIZE;
        } while (tmp_sz -= PAGE_SIZE);

        /* free memory */
        if (!large_page->size) {
            list_del(&large_page->list);
            kfree(large_page);
        }

        give_sz -= size;
        POOL_NODE_SZ(bank) -= size;
        ret_sz += size;
    } while (give_sz != 0);

    return ret_sz;
}
EXPORT_SYMBOL(fmem_give_pages);

/*
 * @brief This function allocates ahead the memory in advance before fragementation happen
 *
 * @function int __init fmem_init(void)
 * @param none
 * @return none
 */
int __init fmem_init(void)
{
    int i, bank, order;
    struct page *page;
    page_node_t *page_node;
    u32 mem_sz[2], size, phyaddr;
    u32 phys_end;

    if (phys_end)   {}
    /* sanity check */
#ifdef CONFIG_ARCH_SPARSEMEM_ENABLE
    for (bank = 0; bank < mem_info.nr_banks; bank ++) {
        if (bank_phys_start(&mem_info.bank[bank]) & ((0x1 << SECTION_SIZE_BITS) - 1)) {
            panic("%s, the start address 0x%x in bank%d is not aligned to section size: 0x%x! \n",
                __func__, bank_phys_start(&mem_info.bank[bank]), bank, (0x1 << SECTION_SIZE_BITS));
        }
    }
    bank = mem_info.nr_banks - 1;
    phys_end = bank_phys_end(&mem_info.bank[bank]);
    if ((get_order(phys_end) + PAGE_SHIFT) != MAX_PHYSMEM_BITS)
        panic("%s, the memory end address is 0x%x, MAX_PHYSMEM_BITS should be %d(current: %d)! \n",
            __func__, phys_end, get_order(phys_end) + PAGE_SHIFT, MAX_PHYSMEM_BITS);
#endif

    /* macro test */
    for (bank = 0; bank < mem_info.nr_banks; bank ++) {
        phyaddr = bank_phys_start(&mem_info.bank[bank]);
        if (phyaddr != (__virt_to_phys(__phys_to_virt(phyaddr))))
            panic("%s, macro porting has bug! \n", __func__);
    }

    order = get_order(ALLOC_BSZ);

    for (bank = 0; bank < mem_info.nr_banks; bank ++) {
        sanity_check_memhole(page_array[bank]);
        /* count the active node */
        g_page_info.nr_node++;

        /*
         * Note: The arguments from boot command line is higher than the MACRO defintion in memory.h.
         */
        mem_sz[bank] = gmmem_info.bank[bank].size ?
                        gmmem_info.bank[bank].size : bank > 0 ? DDR1_FMEM_SIZE : DDR0_FMEM_SIZE;

        /* update 2nd DDR information.
         * If DDR1_FMEM_SIZE = -1 which means to allocate whole memory.
         */
        if ((bank > 0) && (DDR1_FMEM_SIZE == -1) && (mem_sz[bank] == DDR1_FMEM_SIZE))
            mem_sz[bank] = get_early_memblk_count(page_array[bank]) * ALLOC_BSZ;
    }

    for (bank = 0; bank < mem_info.nr_banks; bank ++) {
        INIT_LIST_HEAD(&POOL_LIST(bank));
        POOL_NODE_SZ(bank) = 0;

        /* allocate page_node for each page
         */
        for (i = 0; i < reserve_blk_cnt[bank]; i++) {
            page_node = (page_node_t *)kzalloc(sizeof(page_node_t), GFP_KERNEL);
            if (page_node == NULL)
                panic("%s: No memory for kmalloc! \n", __func__);
            INIT_LIST_HEAD(&page_node->list);
            page_node->page = (struct page *)page_array[bank][i];
            page_node->phy_start = page_to_phys(page_node->page);
            page_node->size = ALLOC_BSZ;

            /* can't be swap out and mmap issue which can be mapped to reserved pages only */
            page = page_node->page;
            size = page_node->size;
            phyaddr = page_node->phy_start;
            do {
                /* seperate the group pages into individual page and do the reservation prevent
                 * those pages from being swapped out.
                 */
                init_page_count(page);
                SetPageReserved(page);
                phyaddr += PAGE_SIZE;
                page = phys_to_page(phyaddr);
            } while (size -= PAGE_SIZE);

            POOL_NODE_SZ(bank) += page_node->size;

            /* add to the global DDR list */
            list_add_tail(&page_node->list, &POOL_LIST(bank));
        }

        /* free the rest memory */
        if (reserve_blk_cnt[bank]) {
            int ret;
            u32 free_sz;

            //record the start address of this pool
            POOL_PHY_START(bank) = page_to_phys((struct page *)page_array[bank][0]);
            free_sz = reserve_blk_cnt[bank] * ALLOC_BSZ - mem_sz[bank];
            if (free_sz != 0) {
                ret = fmem_give_pages(bank, free_sz);
                if (ret != free_sz)
                    panic("%s, bug in bank%d! \n", __func__, bank);
            }
            printk("FMEM: %d pages(0x%x bytes) from bank%d are reserved for Frammap. \n",
                mem_sz[bank] >> PAGE_SHIFT, mem_sz[bank], bank);
        }
    }

    printk("FMEM: Logical memory ends up at 0x%x, init_mm:0x%x(0x%x), PAGE_OFFSET:0x%x(phys:0x%x), \n",
                (u32)high_memory, (u32)init_mm.pgd, (u32)__pa((void *)init_mm.pgd),
                (u32)PAGE_OFFSET, (u32)__pa((void *)PAGE_OFFSET));

    if (1) {
        volatile unsigned int val;

        int cpu_id = (read_cpuid_id() >> 0x4) & 0xFFF;

        if (cpu_id == 0x726) {
            __asm__ __volatile__ ("mrc p15, 0, %0, c15, c0, 0\t\n":"=r"(val));

            printk("FMEM: FA726 Test and Debug Register: 0x%x \n", val);
        }
    }

#ifdef CONFIG_ARCH_SPARSEMEM_ENABLE
    printk("FMEM: SECTION_SIZE_BITS: %d, MAX_PHYSMEM_BITS: %d \n", SECTION_SIZE_BITS, MAX_PHYSMEM_BITS);
    if (MAX_PHYSMEM_BITS != fmem_max_physmem_bits())
        panic("%s, the correct MAX_PHYSMEM_BITS(%d) should be changed to %d! \n", __func__,
                MAX_PHYSMEM_BITS, fmem_max_physmem_bits());
#endif

    fmem_proc_init();

#ifdef CONFIG_PLATFORM_GM8210
    /* hook pcie function */
    hook_fault_code(8, fmem_pcie_abort_handler, SIGBUS, 0, "external abort on non-linefetch");
    hook_fault_code(10, fmem_pcie_abort_handler, SIGBUS, 0, "external abort on non-linefetch");
    printk("Fmem hooks external data-abort handler. \n");
#endif

    return 0;
}

core_initcall(fmem_init);

/*
 * @brief check if the address contains holes
 *
 * @function static sanity_check_memhole
 *
 * @param base of the first page
 * @return 0 on success, !0 on error
 */
static int __init sanity_check_memhole(u32 *base)
{
    struct page *page;
    dma_addr_t phy_addr, next_phy_addr;
    int i = 0;

    /* get first page */
    page = (struct page *)base[0];
    phy_addr = page_to_phys(page);
    next_phy_addr = phy_addr + ALLOC_BSZ;

    do {
        i ++;
        page = (struct page *)base[i];
        if (page == NULL)
            break;

        phy_addr = page_to_phys(page);
        if (phy_addr != next_phy_addr) {
            /* dump the content */
            printk("%s, the expected physical is 0x%x, but get 0x%x \n", __func__, next_phy_addr,
                    phy_addr);
            panic("%s, physical address has holes! \n", __func__);
        }
        next_phy_addr = phy_addr + ALLOC_BSZ;
    } while (1);

    return 0;
}

/*
 * @brief allocate the memory from linux kernel.
 * @function void *fmem_alloc_ex(size_t size, dma_addr_t * dma_handle, unsigned long flags, int ddr_id)
 * @parameter size indicates the memory size going to allocate
 * @parameter dma_handle indicates the physical address
 * @parameter flags indicates type of this allocated memory is cachable or non-cacheable or ....
 * @parameter ddr_id indicates which DDR the users want to allocate.
 * @return virutal memory if success or NULL for fail.
 */
void *fmem_alloc_ex(size_t size, dma_addr_t * dma_handle, unsigned long flags, int ddr_id)
{
    struct page *page;
    void *cpu_addr = NULL;
    struct page *end;

    if (ddr_id >= mem_info.nr_banks)
        return NULL;

    size = PAGE_ALIGN(size);
    if (size > ALLOC_BSZ) {
        printk("The allocated memory size 0x%x exceeds 0x%x bytes! \n", size, ALLOC_BSZ);
        return NULL;
    }

    page = alloc_pages_node(ddr_id, GFP_KERNEL, get_order(size));
    if (!page) {
        printk("alloc_pages fail! (requested %#x)", size);
        goto no_page;
    }

    *dma_handle = page_to_phys(page);
    if ((*dma_handle < bank_phys_start(&mem_info.bank[ddr_id])) ||
            (*dma_handle > bank_phys_end(&mem_info.bank[ddr_id]))) {
        __free_pages(page, get_order(size));
        goto no_page;
    }

    if ((flags == PAGE_COPY) || (flags == PAGE_SHARED))
        //cpu_addr = ioremap_cached(*dma_handle, size);
        cpu_addr = __va(*dma_handle);
    else if (flags == pgprot_writecombine(pgprot_kernel))
        cpu_addr = ioremap_wc(*dma_handle, size);
    else
        cpu_addr = ioremap_nocache(*dma_handle, size);

    if (cpu_addr) {
        end = page + (1 << get_order(size));
        do {
            init_page_count(page);
            SetPageReserved(page);
            page++;
            size -= PAGE_SIZE;
        } while (size != 0);

        /*
         * Free the otherwise unused pages.
         */
        while (page < end) {
            init_page_count(page);
            if (!PageReserved(page) && put_page_testzero(page))
                __free_page(page);
            page++;
        }
    } else {
        __free_pages(page, get_order(size));
        printk("__ioremap fail! (phy %#x)", *dma_handle);
    }

  no_page:
    return cpu_addr;
}
EXPORT_SYMBOL(fmem_alloc_ex);

/*
 * @brief free the memory which was allocated by fmem_alloc_ex().
 * @function void fmem_free_ex(size_t size, void *cpu_addr, dma_addr_t handle)
 * @parameter size indicates the memory size going to free
 * @parameter cpu_addr indicates the virtual memory going to free
 * @parameter handle indicates the physical memory going to free
 * @return None.
 */
void fmem_free_ex(size_t size, void *cpu_addr, dma_addr_t handle)
{
    struct page *page = pfn_to_page(handle >> PAGE_SHIFT);
    struct page *pg;
    unsigned int sz;

    pg = page;
    sz = size;

    if ((u32)__va(handle) != (u32)cpu_addr)
        __iounmap(cpu_addr);

    size = PAGE_ALIGN(size);

    do {
        ClearPageReserved(page);
        if (!PageReserved(page) && put_page_testzero(page))
            __free_page(page);
        page++;
    } while (size -= PAGE_SIZE);
}
EXPORT_SYMBOL(fmem_free_ex);

/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 *      its real physical address.
 *
 * @parm vaddr indicates any virtual address
 *
 * @return  >= 0 for success, 0xFFFFFFFF for fail
 */
phys_addr_t fmem_lookup_pa(unsigned int vaddr)
{
    pgd_t *pgdp;
    pmd_t *pmdp;
    pud_t *pudp;
    pte_t *ptep;
    phys_addr_t paddr = 0xFFFFFFFF;

    /* for speed up */
    if (virt_addr_valid(vaddr))
        return (phys_addr_t)__pa(vaddr);

    /*
     * The pgd is never bad, and a pmd always exists(as it's folded into the pgd entry)
     */
    if (vaddr >= TASK_SIZE) /* use init_mm as mmu to look up in kernel mode */
        pgdp = pgd_offset_k(vaddr);
    else
        pgdp = pgd_offset(current->mm, vaddr);

    if (unlikely(pgd_none(*pgdp))) {
        printk("fmem: 0x%x not mapped in pgd table! \n", vaddr);
        goto err;
    }

    /* because we don't have 3-level MMU, so pud = pgd. Here we are in order to meet generic Linux
     * version, pud is listed here.
     */
    pudp = pud_offset(pgdp, vaddr);
    if (unlikely(pud_none(*pudp))) {
        printk(KERN_INFO"fmem: 0x%x not mapped in pud table! \n", vaddr);
        goto err;
    }

    pmdp = pmd_offset(pudp, vaddr);
    if (unlikely(pmd_none(*pmdp))) {
        printk(KERN_INFO"fmem: 0x%x not mapped in pmd 0x%x! \n", vaddr, (u32)pmdp);
        goto err;
    }

    if (!pmd_bad(*pmdp)) {
        u32 v_addr;

        /* page mapping */
        ptep = pte_offset_kernel(pmdp, vaddr);
        if (pte_none(*ptep)) {
            printk(KERN_ERR"fmem: 0x%x not mapped in pte! \n", vaddr);
            goto err;
        }

        v_addr = (unsigned int)page_address(pte_page(*ptep)) + (vaddr & (PAGE_SIZE - 1));
        paddr = __pa(v_addr);
    } else {
        /* section mapping. The cases are:
         * 1. DDR direct mapping
         * 2. ioremap's vaddr, size and paddr all are 2M alignment.
         */
        if (vaddr & SECTION_SIZE)
            pmdp ++; /* move to pmd[1] */
        paddr = (pmd_val(*pmdp) & SECTION_MASK) | (vaddr & ~SECTION_MASK);
    }
err:
    return paddr;
}
EXPORT_SYMBOL(fmem_lookup_pa);

/*
 * @brief get the memory data information from frammap
 *
 * @function fmem_get_pageinfo(g_page_info_t **pg_info)
 * @param pg_info is the pointer wants to be assigned the info
 * @return none
 */
void fmem_get_pageinfo(g_page_info_t ** pg_info)
{
    *pg_info = &g_page_info;

    return;
}
EXPORT_SYMBOL(fmem_get_pageinfo);

/* __virt_to_phys macro porting arch/arm/include/mach/memory.h */
unsigned long fmem_virt_to_phys(unsigned int vaddr)
{
    unsigned long phys;

    if (phys_offset == FA726_START_ADDR) {
        int cpu_id = __get_cpu_id();

        if (cpu_id == 0x726)
            phys_offset = FA726_PHYS_OFFSET;
        else
            phys_offset = FA626_PHYS_OFFSET;
    }

    if (mem_info.nr_banks == 0) {
        phys = vaddr - PAGE_OFFSET + PHYS_OFFSET;

        printk("%s, someone calls va2pa early............................. \n", __func__);

        return phys;
    }

    if (mem_info.nr_banks == 1) {
        phys = vaddr - PAGE_OFFSET + PHYS_OFFSET;
    } else {
        u32 end_vaddr;

        end_vaddr = PAGE_OFFSET + bank_phys_size(&mem_info.bank[0]);
        phys = vaddr >= end_vaddr ? (vaddr - end_vaddr) + bank_phys_start(&mem_info.bank[1]) :
            (vaddr - PAGE_OFFSET) + bank_phys_start(&mem_info.bank[0]);
    }

    return phys;
}

/* __phys_to_virt macro porting in arch/arm/include/mach/memory.h */
unsigned int fmem_phys_to_virt(unsigned long phys)
{
    u32 vaddr;

    if (phys_offset == FA726_START_ADDR) {
        int cpu_id = __get_cpu_id();

        if (cpu_id == 0x726)
            phys_offset = FA726_PHYS_OFFSET;
        else
            phys_offset = FA626_PHYS_OFFSET;
    }

    if (mem_info.nr_banks == 0) {
         vaddr = phys - PHYS_OFFSET + PAGE_OFFSET;
         printk("%s, someone calls pa2va early............................. \n", __func__);
         return vaddr;
    }

    if (mem_info.nr_banks == 1) {
        vaddr = phys - PHYS_OFFSET + PAGE_OFFSET;
    } else {
        vaddr = (phys >= bank_phys_start(&mem_info.bank[1])) ?
            PAGE_OFFSET + bank_phys_size(&mem_info.bank[0]) + (phys - bank_phys_start(&mem_info.bank[1])) :
            PAGE_OFFSET + (phys - PHYS_OFFSET);
    }

    return vaddr;
}

/*
 * @This function is used to read CPU id and pci id
 * @Return value: 0 for success, -1 for fail
 */
int fmem_get_identifier(fmem_pci_id_t *pci_id, fmem_cpu_id_t *cpu_id)
{
    int ret = 0;
    u32 value;


    /* Read CR0-0 Identification Code Register(ID)
     * [31:24] IMP, [23:16]ARCH, [15:4] PART, [3:0] VER
     */
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0\t\n": "=r"(value));

    switch ((value >> 0x4) & 0xFFF) {
      case 0x626:
        /* the code should be 0x66056261 */
        *cpu_id = FMEM_CPU_FA626;
        break;
      case 0x726:
        *cpu_id = FMEM_CPU_FA726;
        break;
      default:
        *cpu_id = FMEM_CPU_UNKNOWN;
        panic("Unknow cpu id: 0x%x \n", value);
        break;
    }

    value = (ftpmu010_read_reg(0x04) >> 19) & 0x3;
    /* 01: Endpoint, 10: RootPoint */
    *pci_id = (value == 0x02) ? FMEM_PCI_HOST : FMEM_PCI_DEV0;

    return ret;
}
EXPORT_SYMBOL(fmem_get_identifier);

static void dev_release(struct device *dev) { return; }
#define DRIVER_NAME "fmem_sync"
static struct platform_device pseudo_dev = {
	.name = DRIVER_NAME,
	.id   = 0,
	.num_resources = 0,
	.dev  = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.release = dev_release,
	}
};

/* process outer cache */
static inline void __fmem_sync_outer_cache(void *vaddr, u32 len, enum dma_data_direction dir)
{
#ifdef CONFIG_OUTER_CACHE
    phys_addr_t  paddr;

    paddr = fmem_lookup_pa((unsigned int)vaddr);
    if (paddr == 0xFFFFFFFF)
        return;

    switch (dir) {
      case DMA_BIDIRECTIONAL:
        outer_flush_range(paddr, paddr + len);
        break;
      case DMA_TO_DEVICE:
        outer_clean_range(paddr, paddr + len);
        break;
      case DMA_FROM_DEVICE:
        outer_inv_range(paddr, paddr + len);
        break;
      case DMA_NONE:
        break;
    }
#endif
}

/* @this function is a data cache operation function,
 * @parm: vaddr: any virtual address
 * @parm: dir will be:
 * DMA_BIDIRECTIONAL = 0, it means flush operation.
 * DMA_TO_DEVICE = 1, it means clean operation.
 * DMA_FROM_DEVICE = 2, it means invalidate operation.
 * DMA_NONE = 3,
 */
void fmem_dcache_sync(void *vaddr, u32 len, enum dma_data_direction dir)
{
    struct device *dev = &pseudo_dev.dev;

    if (!valid_dma_direction(dir))
        panic("%s, invalid direction: %d \n", __func__, dir);

    /* kernel buffer may not cache line alignment, it only can use both clean/inv
     * for safe. Others we regard them as warning cases in coding.
     */
    if (dir != DMA_BIDIRECTIONAL) {
        if ((u32)vaddr & CACHE_ALIGN_MASK) {
            va_not_32align ++;
            if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_VA)) {
                printk("%s, warning, vaddr: 0x%x not cache alignment! \n", __func__, (u32)vaddr);
                dump_stack();
            }
        }

        if (len & CACHE_ALIGN_MASK) {
            length_not_32align ++;
            if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_LEN)) {
                printk("%s, warning, len: %d not cache alignment! \n", __func__, len);
                dump_stack();
            }
        }
    }

    if (virt_addr_valid(vaddr) && virt_addr_valid(vaddr + len - 1)) {
        switch (dir) {
          case DMA_BIDIRECTIONAL:
            dma_map_single(dev, vaddr, len, DMA_BIDIRECTIONAL);
            /* the purpose is for outer cache sync. dma_map_single() only did
             * outer_clean_range() in arch/arm/mm/dma-mapping.c (#554), why?
             */
            __fmem_sync_outer_cache(vaddr, len, dir);
            //dma_unmap_single(dev, __pa(vaddr), len, DMA_FROM_DEVICE);
            break;
          case DMA_TO_DEVICE:
            dma_map_single(dev, vaddr, len, DMA_TO_DEVICE);
            /* outer cache will invoke clean function. */
            break;
          case DMA_FROM_DEVICE:
            dma_map_single(dev, vaddr, len, DMA_FROM_DEVICE);
            /* outer cache will invoke invalidate function. */
            break;
          case DMA_NONE:
            break;
        }
        cpuaddr_flush ++;
    } else {
        switch (dir) {
          case DMA_BIDIRECTIONAL:
            dmac_flush_range(vaddr, vaddr + len);
            __fmem_sync_outer_cache(vaddr, len, dir);
            break;
          case DMA_TO_DEVICE:
            dmac_map_area(vaddr, len, DMA_TO_DEVICE);
            __fmem_sync_outer_cache(vaddr, len, dir);
            break;
          case DMA_FROM_DEVICE:
            dmac_map_area(vaddr, len, DMA_FROM_DEVICE);
            __fmem_sync_outer_cache(vaddr, len, dir);
            break;
          case DMA_NONE:
            break;
        }
        nonlinear_cpuaddr_flush ++;
        if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_API)) {
            printk("%s, warning, memory addr 0x%x not direct mapped! \n", __func__, (u32)vaddr);
            dump_stack();
        }
    }
}
EXPORT_SYMBOL(fmem_dcache_sync);

void CheckTxStatus(unsigned int port)
{
		// wait until Tx ready
    while (!(*(volatile unsigned int *)(port + 0x14) & 0x20));
      udelay(1);
}

static void _putchar(unsigned int base, char Ch)
{
    if(Ch != '\0'){
        CheckTxStatus(base);
        *(volatile unsigned int *)(base) = Ch;
    }

    if (Ch == '\n'){
        CheckTxStatus(base);
        *(volatile unsigned int *)(base) = '\r';
    }
}

static int debug_init = 1;
/* If we want to use this API, please enable arch/arm/kernel/head.S
 * GM_CONSOLE_PRINT definition
*/
void debug_printk(const char *f, ...)
{
    volatile unsigned int i, uart_va_base, uart_pa_base;
    va_list arg_ptr;
    char buffer[256];

    if (!debug_init) {
        volatile unsigned int val, ttbr;
        debug_init = 1;

        __asm__ __volatile__ ("mrc p15, 0, %0, c2, c0, 0\t\n":"=r"(ttbr));

        //ttbr = PAGE_OFFSET + (ttbr - PHYS_OFFSET);  //because the __va can't work at this moment (mem_info[] no data yet)
        
        if (__get_cpu_id() == 0x626) {
            ttbr = PAGE_OFFSET + (ttbr - FA626_PHYS_OFFSET);
            uart_va_base = UART_FTUART010_1_VA_BASE;
            uart_pa_base = UART_FTUART010_1_PA_BASE;
        } else {
            ttbr = PAGE_OFFSET + (ttbr - FA726_PHYS_OFFSET);
            uart_va_base = UART_FTUART010_0_VA_BASE;
            uart_pa_base = UART_FTUART010_0_PA_BASE;
        }
   
        val = *(u32 *)(ttbr + ((uart_va_base >> 20) << 2));
        //uart temp mapping
        if (!val) {
            *(u32 *)(ttbr + ((uart_va_base >> 20) << 2)) = (uart_pa_base | 0xC12);

            fmem_flush_kern_dcache_area((void *)(ttbr + ((uart_va_base >> 20) << 2)), PAGE_SIZE);
            
            //neet to flush dcache and tlb?
            __asm__ __volatile__ ("mov r0, #0\n"
                                  "mcr p15, 0, r0, c7, c10, 3\n"    /* Test and Clean DCache */
                                  "mcr p15, 0, r0, c7, c10, 4\n"    /* drain WB */
                                  "mcr p15, 0, r0, c8, c7, 0");	    /* invalidate TLB all */
        }

        va_start(arg_ptr, f);
        vsprintf(&buffer[0], f, arg_ptr);
        va_end(arg_ptr);
    
          //output the buffer
        i = 0;
        while (buffer[i]){
              _putchar(uart_va_base, buffer[i]);
              i++;
        }
    }
}

#define PCIE_WIN_SIZE   (128 << 20)
#define PCIE_WIN_MASK   (~(PCIE_WIN_SIZE - 1))
#define PCIE_WIN_BASE   0xE8000000
static u32 ddr_win_base = 0, ddr_win_end = 0;

/* @this function is used to set outbound window of EP
 * @parm: phy_addr: start physical address
 * @parm: size: window size
 * return value: 0 for success, -1 for fail.
 * Note: This function is only called in RC.
 */
int fmem_set_ep_outbound_win(u32 phy_addr, u32 size)
{
    static u32 remote_pcie_vbase = 0;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    if (pci_id != FMEM_PCI_HOST)
        return -1;  /* only RC can configure EP's BAR */

    if (ftpmu010_get_attr(ATTR_TYPE_EPCNT) == 0)
        return 0;

    ddr_win_base = phy_addr & PCIE_WIN_MASK;
    ddr_win_end = ddr_win_base + PCIE_WIN_SIZE;
    if ((phy_addr + size) >= ddr_win_end) {
        printk("%s, ddr_win_base: 0x%x, ddr_win_end: 0x%x  \n", __func__, ddr_win_base, ddr_win_end);
        printk("input phy_addr: 0x%x, end: 0x%x is out of range! \n", phy_addr, phy_addr + size);
        return -1;
    }

    if (!remote_pcie_vbase) {
        u32 pcie_pbase = PCIE_PLDA_PA_BASE - 0x98000000 + 0xE0000000;

        remote_pcie_vbase = (u32)ioremap_nocache(pcie_pbase, PAGE_SIZE);
        if (!remote_pcie_vbase)
            panic("%s fail! \n", __func__);
    }

    iowrite32(ddr_win_base, remote_pcie_vbase + 0xE8);

    return 0;
}
EXPORT_SYMBOL(fmem_set_ep_outbound_win);

/* @this function is used to translate local axi address to pcie address
 * @parm: axi_phy_addr indicates local axi address
 * return value: PCIe address. 0xFFFFFFFF if fail.
 * Note: This function is only called in RC.
 */
u32 fmem_get_pcie_addr(u32 axi_phy_addr)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    if (pci_id != FMEM_PCI_HOST)
        return 0xFFFFFFFF;

    if ((axi_phy_addr < ddr_win_base) || (axi_phy_addr >= ddr_win_end))
        return 0xFFFFFFFF;

    return (axi_phy_addr - ddr_win_base) + PCIE_WIN_BASE;
}
EXPORT_SYMBOL(fmem_get_pcie_addr);

/* @this function is used to translate pcie to local axi address
 * @parm: pcie_phy_addr indicates pcie address
 * return value: local axi address. 0xFFFFFFFF if fail.
 * Note: This function is only called in RC.
 */
u32 fmem_get_axi_addr(u32 pcie_phy_addr)
{
    u32 axi_phy_addr;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    if (pci_id != FMEM_PCI_HOST)
        return 0xFFFFFFFF;

    if (pcie_phy_addr < PCIE_WIN_BASE)
        return 0xFFFFFFFF;

    axi_phy_addr = (pcie_phy_addr - PCIE_WIN_BASE) + ddr_win_base;
    if ((axi_phy_addr < ddr_win_base) || (axi_phy_addr >= ddr_win_end))
        return 0xFFFFFFFF;

    return axi_phy_addr;
}
EXPORT_SYMBOL(fmem_get_axi_addr);

EXPORT_SYMBOL(debug_printk);
EXPORT_SYMBOL(fmem_virt_to_phys);
EXPORT_SYMBOL(fmem_phys_to_virt);

