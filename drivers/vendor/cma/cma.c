/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */
#include <linux/bsp_cma.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/securec.h>
#include <linux/initrd.h>
#include <linux/libfdt.h>
#include <linux/of_fdt.h>
#include <asm/early_ioremap.h>
#include <asm/sections.h>
#include <asm/tlbflush.h>

static u32 num_zones;
static struct cma_zone bsp_zone[ZONE_MAX];
static int use_bootargs;
static bool bsp_zones_held;
static bool bsp_zones_failed;	/* a zone CMA could not reserve was left out */

unsigned int get_cma_size(void)
{
	u32 i;
	u64 total = 0;

	for (i = 0; i < num_zones; i++)
		total += bsp_zone[i].nbytes;

	return (unsigned int)(total >> 20); /* unit M: shift right 20 bits */
}

int is_cma_address(phys_addr_t phys, unsigned long size)
{
	phys_addr_t start, end;
	u32 i;

	for (i = 0; i < num_zones; i++) {
		start = bsp_zone[i].phys_start;
		end = bsp_zone[i].phys_start + bsp_zone[i].nbytes;

		if ((phys >= start) && ((phys + size) <= end))
			return 1; /* Yes, found! */
	}

	return 0;
}
EXPORT_SYMBOL(is_cma_address);

static int __init bsp_mmz_parse_cmdline(char *s)
{
	char *line = NULL, *tmp = NULL;
	char tmpline[256];
	int ret;

	if (s == NULL) {
		pr_info("There is no cma zone!\n");
		return 0;
	}
	ret = strncpy_s(tmpline, sizeof(tmpline), s, sizeof(tmpline) - 1);
	if (ret) {
		printk("%s:strncpy_s failed\n", __func__);
		return ret;
	}

	tmpline[sizeof(tmpline) - 1] = '\0';
	tmp = tmpline;

	while ((line = strsep(&tmp, ":")) != NULL) {
		int i;
		char *argv[6];

		for (i = 0; (argv[i] = strsep(&line, ",")) != NULL;)
			if (++i == ARRAY_SIZE(argv))
				break;

		if (num_zones >= ZONE_MAX)
			return 0;
		bsp_zone[num_zones].pdev.coherent_dma_mask = DMA_BIT_MASK(64);
		if (i == 4) {
			strlcpy(bsp_zone[num_zones].name, argv[0], NAME_LEN_MAX);
			bsp_zone[num_zones].gfp = (uintptr_t)memparse(argv[1], NULL);
			bsp_zone[num_zones].phys_start = (uintptr_t)memparse(argv[2], NULL);
			bsp_zone[num_zones].nbytes = (uintptr_t)memparse(argv[3], NULL);
		}

		else if (i == 6) {
			strlcpy(bsp_zone[num_zones].name, argv[0], NAME_LEN_MAX);
			bsp_zone[num_zones].gfp = (uintptr_t)memparse(argv[1], NULL);
			bsp_zone[num_zones].phys_start = (uintptr_t)memparse(argv[2], NULL);
			bsp_zone[num_zones].nbytes = (uintptr_t)memparse(argv[3], NULL);
			bsp_zone[num_zones].alloc_type = (uintptr_t)memparse(argv[4], NULL);
			bsp_zone[num_zones].block_align = (uintptr_t)memparse(argv[5], NULL);
		} else {
			pr_err("ion parameter is not correct\n");
			continue;
		}

		num_zones++;
	}
	if (num_zones != 0)
		use_bootargs = 1;

	return 0;
}
early_param("mmz", bsp_mmz_parse_cmdline);

phys_addr_t get_zones_start(void)
{
	u32 i;
	phys_addr_t lowest_zone_base = memblock_end_of_DRAM();

	for (i = 0; i < num_zones; i++) {
		if (lowest_zone_base > bsp_zone[i].phys_start)
			lowest_zone_base = bsp_zone[i].phys_start;
	}

	return lowest_zone_base;
}
EXPORT_SYMBOL(get_zones_start);

struct cma_zone *get_cma_zone(const char *name)
{
	u32 i;

	if (name == NULL)
		return NULL;
	for (i = 0; i < num_zones; i++)
		if (strcmp(bsp_zone[i].name, name) == 0)
			break;

	if (i == num_zones)
		return NULL;

	return &bsp_zone[i];
}
EXPORT_SYMBOL(get_cma_zone);

struct device *get_cma_device(const char *name)
{
	u32 i;

	if (name == NULL)
		return NULL;

	for (i = 0; i < num_zones; i++)
		if (strcmp(bsp_zone[i].name, name) == 0)
			break;

	if (i == num_zones)
		return NULL;

	return &bsp_zone[i].pdev;
}
EXPORT_SYMBOL(get_cma_device);

#ifdef CONFIG_ARM
/*
 * Give the linear mapping of CMA pages the attributes of the mapping a
 * module is about to create for them (open_mmz maps MMBs non-cached), so
 * the same pages are not mapped cacheable and non-cached at once.  This is
 * arch/arm/mm/dma-mapping.c's __dma_remap(), which is static; it needs
 * init_mm and, on SMP, flush_tlb_kernel_range(), neither of which ARM
 * exports to modules.  dma_contiguous_remap() has already split the zones
 * that declare_heap_memory() reserved into pages, so there are ptes to
 * update.
 */
static int cma_update_pte(pte_t *pte, unsigned long addr, void *data)
{
	struct page *page = virt_to_page(addr);
	pgprot_t prot = *(pgprot_t *)data;

	set_pte_ext(pte, mk_pte(page, prot), 0);
	return 0;
}

int cma_dma_remap(struct page *page, size_t size, pgprot_t prot)
{
	unsigned long start;
	int ret;

	if (page == NULL || PageHighMem(page))
		return -EINVAL;

	start = (unsigned long)page_address(page);
	ret = apply_to_page_range(&init_mm, start, size, cma_update_pte, &prot);
	flush_tlb_kernel_range(start, start + size);
	return ret;
}
EXPORT_SYMBOL_GPL(cma_dma_remap);
#endif /* CONFIG_ARM */

int __init declare_heap_memory(void)
{
	u32 i, n;
	int ret = 0;

	/* bsp_memory_init() kept early allocations clear of the zones until now */
	if (bsp_zones_held) {
		memblock_set_bottom_up(false);
		bsp_zones_held = false;
	}
	if (use_bootargs == 0) {
		pr_info("cma zone is not set!\n");
		return ret;
	}

	for (i = 0, n = 0; i < num_zones; i++) {
		ret = dma_contiguous_reserve_area(bsp_zone[i].nbytes,
						  bsp_zone[i].phys_start, 0, &bsp_zone[i].pdev.cma_area, true);
		if (ret) {
			/*
			 * Memory something else holds already (a zone from the
			 * command line over reserved memory, say). The camera
			 * boots without this zone rather than not at all, as the
			 * panic here used to have it, rebooting in a loop.
			 */
			pr_err("hisi_mem: cma zone %s at 0x%llx, %lluM not reserved (%d), left out\n",
			       bsp_zone[i].name, (u64)bsp_zone[i].phys_start,
			       (u64)bsp_zone[i].nbytes >> 20, ret);
			bsp_zones_failed = true;
			continue;
		}

		bsp_zone[i].phys_start = cma_get_base(bsp_zone[i].pdev.cma_area);
		bsp_zone[i].nbytes = cma_get_size(bsp_zone[i].pdev.cma_area);
		if (n != i)
			bsp_zone[n] = bsp_zone[i];
		n++;
	}
	num_zones = n;
	if (!num_zones)
		use_bootargs = 0;

	return 0;
}

static int bsp_mmz_setup(struct reserved_mem *rmem)
{
	return 0;
}
RESERVEDMEM_OF_DECLARE(cma, "bsp-mmz", bsp_mmz_setup);

/*
 * DRAM size, allocator and OS share, decided by the kernel.
 *
 * The bootloader cannot be relied on to say how much DRAM there is: a vendor
 * U-Boot passes mem= from its own environment and no memory tag, and the
 * device tree carries a placeholder. So the kernel measures it, the way
 * U-Boot's get_ram_size() does, and lays memory out itself:
 *
 *   DRAM      allocator  OS share  media zone
 *   64 MiB    ot         default   the rest, a carve-out the kernel never sees
 *   128+ MiB  cma        default   the rest, a CMA area lent to the OS when idle
 *
 * A bootloader's plain mem= gives way to that. mmz_allocator=ot|cma and
 * osmem=<size> on the command line override the defaults; an explicit mmz=
 * keeps today's meaning, with mem= as the bootloader passes it, cut to the
 * measured DRAM.
 *
 * The result is published read-only under /sys/module/hisi_mem/parameters
 * (dram and osmem in MiB, allocator, and mmz as the mmz module takes it),
 * and the device tree's /memory is set to the measured size.
 */
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "hisi_mem."

/*
 * cma_declare_contiguous() rounds a fixed zone's base and size up to this
 * (PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order)); a zone has to stay
 * inside DRAM after the rounding, not only before it.
 */
#define BSP_ZONE_ALIGN	((phys_addr_t)PAGE_SIZE << max_t(unsigned int, MAX_ORDER - 1, pageblock_order))

/*
 * The parts this is for, by the device tree's root: the largest DRAM the
 * family carries, which bounds the probe (an address past it may be outside
 * the DDR window altogether), and the default OS share, whose last page the
 * probe writes -- above the kernel image and its device tree, and not yet
 * handed out this early. Other boards keep their memory as the bootloader set
 * it.
 */
static const struct {
	const char *compatible;
	phys_addr_t ddr, max, os;
} bsp_mem_parts[] __initconst = {
	{ "hisilicon,hi3516cv610", 0x40000000, SZ_512M, SZ_32M },
	{ "hisilicon,hi3516cv608", 0x40000000, SZ_512M, SZ_32M },
	{ "vendor,hi3519dv500", 0x40000000, SZ_2G, SZ_128M },
	{ "vendor,hi3516dv500", 0x40000000, SZ_2G, SZ_128M },
};

static phys_addr_t bsp_dram, bsp_os, bsp_ddr, bsp_lo;
static char bsp_alloc_req[8];
static phys_addr_t bsp_os_req;

static unsigned int dram;
static unsigned int osmem;
static char allocator[8];
static char mmz[256];
module_param(dram, uint, 0444);
module_param(osmem, uint, 0444);
module_param_string(allocator, allocator, sizeof(allocator), 0444);
module_param_string(mmz, mmz, sizeof(mmz), 0444);

static int __init bsp_alloc_param(char *s)
{
	if (s && (!strcmp(s, "ot") || !strcmp(s, "cma")))
		strlcpy(bsp_alloc_req, s, sizeof(bsp_alloc_req));
	else
		pr_warn("hisi_mem: mmz_allocator=%s ignored, not ot or cma\n", s ? s : "");
	return 0;
}
early_param("mmz_allocator", bsp_alloc_param);

static int __init bsp_osmem_param(char *s)
{
	if (s)
		bsp_os_req = memparse(s, NULL);
	return 0;
}
early_param("osmem", bsp_osmem_param);

/*
 * Whether size is past the end of DRAM, seen from the probe page p: the
 * address p + size either wraps back onto p or holds no memory at all.
 * 1 if so, 0 if not, -1 if the pages could not be mapped.
 */
static int __init bsp_dram_ends_at(phys_addr_t p, phys_addr_t size)
{
	void __iomem *lo, *hi;
	u32 save_lo, save_hi;
	int end;

	lo = early_ioremap(p, PAGE_SIZE);
	hi = early_ioremap(p + size, PAGE_SIZE);
	if (!lo || !hi) {
		if (hi)
			early_iounmap(hi, PAGE_SIZE);
		if (lo)
			early_iounmap(lo, PAGE_SIZE);
		return -1;
	}
	save_lo = readl(lo);
	save_hi = readl(hi);
	writel(0x5a5aa5a5, lo);
	writel(0xa5a55a5a, hi);
	end = readl(lo) != 0x5a5aa5a5 || readl(hi) != 0xa5a55a5a;
	/* hi first: when the two alias, lo's value is the one to keep */
	writel(save_hi, hi);
	writel(save_lo, lo);
	early_iounmap(hi, PAGE_SIZE);
	early_iounmap(lo, PAGE_SIZE);
	return end;
}

/* The DRAM size, or 0 when it could not be measured. */
static phys_addr_t __init bsp_probe_dram(phys_addr_t max, phys_addr_t os)
{
	phys_addr_t size;
	int end;

	for (size = SZ_64M; size < max; size <<= 1) {
		end = bsp_dram_ends_at(bsp_ddr + os - PAGE_SIZE, size);
		if (end < 0)
			return 0;
		if (end)
			return size;
	}
	return max;
}

#ifdef CONFIG_ARM64
extern phys_addr_t __fdt_pointer;
#define BSP_FDT_PHYS	((phys_addr_t)__fdt_pointer)
#else
extern unsigned int __atags_pointer;
#define BSP_FDT_PHYS	((phys_addr_t)__atags_pointer)
#endif

/*
 * What a zone must stay clear of, as memblock reserves it before the zone is
 * declared: the kernel; the device tree and initrd the bootloader left where
 * it liked; the device tree's /memreserve/ entries and its static
 * /reserved-memory (OP-TEE's shared memory on the -tee boards, say).
 */
#define BSP_MAX_BLOBS	32
static phys_addr_t bsp_blobs[BSP_MAX_BLOBS][2] __initdata;
static int bsp_nblobs __initdata;
static bool bsp_blobs_full __initdata;

static void __init bsp_blob(phys_addr_t base, phys_addr_t size)
{
	if (!size)
		return;
	if (bsp_nblobs == BSP_MAX_BLOBS) {
		bsp_blobs_full = true;
		return;
	}
	bsp_blobs[bsp_nblobs][0] = base;
	bsp_blobs[bsp_nblobs][1] = size;
	bsp_nblobs++;
}

/* False when there were more than it holds: then no zone can be placed safely. */
static bool __init bsp_find_blobs(void)
{
	const void *fdt = initial_boot_params;
	const __be32 *prop;
	int node, child, i;

	bsp_nblobs = 0;
	bsp_blobs_full = false;
	bsp_blob(__pa_symbol(_text), __pa_symbol(_end) - __pa_symbol(_text));
	if (!fdt)
		return !bsp_blobs_full;
	bsp_blob(BSP_FDT_PHYS, fdt_totalsize(fdt));
#ifdef CONFIG_BLK_DEV_INITRD
	bsp_blob(phys_initrd_start, phys_initrd_size);
#endif
	for (i = 0; i < fdt_num_mem_rsv(fdt); i++) {
		u64 base, size;

		if (!fdt_get_mem_rsv(fdt, i, &base, &size))
			bsp_blob(base, size);
	}
	/*
	 * /reserved-memory as the kernel's own scan takes it
	 * (__reserved_mem_check_root): the root's cell counts and a ranges, or
	 * none of it is reserved at all.
	 */
	node = fdt_path_offset(fdt, "/reserved-memory");
	if (node < 0)
		return !bsp_blobs_full;
	prop = fdt_getprop(fdt, node, "#size-cells", NULL);
	if (!prop || be32_to_cpup(prop) != dt_root_size_cells)
		return !bsp_blobs_full;
	prop = fdt_getprop(fdt, node, "#address-cells", NULL);
	if (!prop || be32_to_cpup(prop) != dt_root_addr_cells)
		return !bsp_blobs_full;
	if (!fdt_getprop(fdt, node, "ranges", NULL) || !(dt_root_addr_cells + dt_root_size_cells))
		return !bsp_blobs_full;
	fdt_for_each_subnode(child, fdt, node) {
		const int t_len = (dt_root_addr_cells + dt_root_size_cells) * sizeof(__be32);
		const char *status = fdt_getprop(fdt, child, "status", NULL);
		const __be32 *reg;
		int len;

		if (status && strcmp(status, "okay") && strcmp(status, "ok"))
			continue;
		reg = fdt_getprop(fdt, child, "reg", &len);
		/* none: dynamic, placed later, bottom-up, in the OS's; malformed: ignored */
		if (!reg || !len || len % t_len)
			continue;
		for (; len >= t_len; len -= t_len) {
			u64 base = dt_mem_next_cell(dt_root_addr_cells, &reg);
			u64 size = dt_mem_next_cell(dt_root_size_cells, &reg);

			bsp_blob(base, size);
		}
	}
	return !bsp_blobs_full;
}

/*
 * Where a zone from start can end, at most at end: below what the bootloader
 * left in memory for the kernel -- its device tree, an initrd -- which it puts
 * where it likes, often at the top of RAM, and which memblock reserves before
 * the zone is declared. In whole units of CMA's alignment.
 */
static phys_addr_t __init bsp_zone_end(phys_addr_t start, phys_addr_t end)
{
	int i;

	for (i = 0; i < bsp_nblobs; i++)
		if (bsp_blobs[i][0] < end && bsp_blobs[i][0] + bsp_blobs[i][1] > start)
			end = max(start, round_down(bsp_blobs[i][0], BSP_ZONE_ALIGN));
	return end;
}

/*
 * The largest span of [*start, end) clear of those: a vendor U-Boot that sees
 * less DRAM than the board has puts them at the top of what it sees, part way
 * into the zone, and the span above them can be the larger. Returns the
 * span's end, and its start in *start.
 */
static phys_addr_t __init bsp_zone_span(phys_addr_t *start, phys_addr_t end)
{
	phys_addr_t lo = *start, best = bsp_zone_end(lo, end);
	int i;

	for (i = 0; i < bsp_nblobs; i++) {
		phys_addr_t from = ALIGN(bsp_blobs[i][0] + bsp_blobs[i][1], BSP_ZONE_ALIGN);
		phys_addr_t to;

		if (from <= lo || from >= end)
			continue;
		to = bsp_zone_end(from, end);
		if (to - from > best - *start) {
			*start = from;
			best = to;
		}
	}
	return best;
}

/*
 * A carve-out from start is not the kernel's to keep anything in, nor above
 * it: mmz refuses a zone inside the span of the kernel's memory. An initrd
 * the bootloader put up there is not used, since arm64 would add its pages
 * back to the kernel's memory (arm64_memblock_init).
 */
static void __init bsp_carve_out(phys_addr_t start)
{
#ifdef CONFIG_BLK_DEV_INITRD
	if (phys_initrd_size && phys_initrd_start + phys_initrd_size > start) {
		pr_warn("hisi_mem: the initrd at 0x%llx is in the carve-out, and is not used\n",
			(u64)phys_initrd_start);
		phys_initrd_size = 0;
	}
#endif
}

/*
 * Keep early allocations out of the zones until declare_heap_memory() reserves
 * them for CMA: on arm64 the page tables and the memory map are taken from the
 * top of memory, where the zones are, before dma_contiguous_reserve() runs.
 * Bottom-up, they come from just above the kernel, in the OS share, instead.
 * (Reserving the zones and freeing them again would also free whatever the
 * kernel reserved inside them meanwhile.)
 */
static void __init bsp_hold_zones(void)
{
	memblock_set_bottom_up(true);
	bsp_zones_held = true;
}

void __init bsp_memory_init(void)
{
	unsigned long root = of_get_flat_dt_root();
	phys_addr_t max = 0, os_default = 0, booted, end;
#ifdef CONFIG_ARM64
	phys_addr_t keep;
#endif
	bool cma;
	int i, n;

	for (i = 0; i < ARRAY_SIZE(bsp_mem_parts); i++)
		if (of_flat_dt_is_compatible(root, bsp_mem_parts[i].compatible) > 0) {
			bsp_ddr = bsp_mem_parts[i].ddr;
			max = bsp_mem_parts[i].max;
			os_default = bsp_mem_parts[i].os;
			break;
		}
	if (!max)
		return;

	/*
	 * The DDR base is the part's; the first usable byte is where the device
	 * tree's memory starts, above firmware the kernel must not touch (ATF;
	 * OP-TEE's whole first 80 MiB on the -tee boards).
	 */
	bsp_lo = memblock_start_of_DRAM();
	/* the probe writes the last page of the default OS share: the kernel's */
	if (bsp_lo < bsp_ddr || bsp_lo > bsp_ddr + os_default - PAGE_SIZE) {
		pr_warn("hisi_mem: memory from 0x%llx, not this part's DDR; as the bootloader set it\n",
			(u64)bsp_lo);
		return;
	}
#ifdef CONFIG_ARM64
	/* mem= waits in memory_limit, as an amount from the first usable byte */
	keep = min_t(phys_addr_t, arm64_memory_limit(), memblock_end_of_DRAM() - bsp_lo);
	booted = bsp_lo + keep - bsp_ddr;
#else
	booted = memblock_end_of_DRAM() - bsp_ddr;
#endif

	bsp_dram = bsp_probe_dram(max, os_default);
	if (!bsp_dram) {
		pr_warn("hisi_mem: DRAM size not measured, memory as the bootloader set it\n");
		return;
	}
	if (!bsp_find_blobs()) {
		pr_warn("hisi_mem: more than %d reserved regions to keep a zone clear of; memory as the bootloader set it\n",
			BSP_MAX_BLOBS);
		bsp_dram = 0;
		return;
	}
#ifdef CONFIG_ARM64
	/* measured: mem= is applied here, as ARM32 already has, and not again */
	arm64_set_memory_limit(PHYS_ADDR_MAX);
	memblock_remove(bsp_lo + keep, PHYS_ADDR_MAX - (bsp_lo + keep));
#endif
	end = bsp_ddr + bsp_dram;

	if (use_bootargs) {
		/* mmz_allocator=ot: the zones are a carve-out, which CMA must not take */
		bool carve = !strcmp(bsp_alloc_req, "ot");

		/*
		 * An explicit mmz=: the zone and mem= as the bootloader gave them,
		 * except where they claim memory the board does not have. Past the
		 * end of DRAM the address wraps onto the kernel's own pages, so a
		 * zone or a mem= reaching there corrupts it as soon as it is used.
		 */
		if (bsp_ddr + booted > end) {
			pr_warn("hisi_mem: mem=%lluM is more than the %lluM of DRAM, cut to it\n",
				(u64)booted >> 20, (u64)bsp_dram >> 20);
			memblock_remove(end, bsp_ddr + booted - end);
		}
		for (i = 0, n = 0; i < num_zones; i++) {
			/*
			 * Whole units of CMA's alignment, which it reserves only at
			 * an aligned base; at or above the first usable byte, and
			 * within DRAM, clear of what is reserved there.
			 */
			phys_addr_t start = ALIGN(max(bsp_zone[i].phys_start, bsp_lo),
						  BSP_ZONE_ALIGN);
			phys_addr_t zend = min(round_down(bsp_zone[i].phys_start +
							  bsp_zone[i].nbytes, BSP_ZONE_ALIGN),
					       bsp_zone_end(start, end));

			if (start != bsp_zone[i].phys_start ||
			    zend != bsp_zone[i].phys_start + bsp_zone[i].nbytes) {
				pr_warn("hisi_mem: mmz zone %s at 0x%llx, %lluM: outside the %lluM of DRAM from 0x%llx, over reserved memory, or not in whole %lluM units\n",
					bsp_zone[i].name, (u64)bsp_zone[i].phys_start,
					(u64)bsp_zone[i].nbytes >> 20, (u64)bsp_dram >> 20,
					(u64)bsp_lo, (u64)BSP_ZONE_ALIGN >> 20);
				if (start >= zend)
					continue;
				bsp_zone[i].phys_start = start;
				bsp_zone[i].nbytes = zend - start;
			}
			/*
			 * DRAM, as just measured, whatever mem= said: CMA hands
			 * a zone's pages to the page allocator, and only memory
			 * memblock knows of has them. A carve-out is kept out.
			 */
			if (!carve)
				memblock_add(bsp_zone[i].phys_start, bsp_zone[i].nbytes);
			else {	/* mem= counts from the first usable byte, and may reach in */
				memblock_remove(bsp_zone[i].phys_start, bsp_zone[i].nbytes);
				bsp_carve_out(bsp_zone[i].phys_start);
			}
			if (n != i)
				bsp_zone[n] = bsp_zone[i];
			n++;
		}
		num_zones = n;
		if (num_zones) {
			booted = min(booted, bsp_dram);
			bsp_os = get_zones_start() - bsp_ddr;
			strlcpy(allocator, carve ? "ot" : "cma", sizeof(allocator));
			pr_info("hisi_mem: DRAM %lluM (probed), %s, zones from mmz=, mem=%lluM as passed\n",
				(u64)bsp_dram >> 20, allocator, (u64)booted >> 20);
			if (carve)
				use_bootargs = 0;	/* published, not declared */
			else
				bsp_hold_zones();
			return;
		}
		use_bootargs = 0;
	}

	cma = bsp_alloc_req[0] ? !strcmp(bsp_alloc_req, "cma") : bsp_dram > SZ_64M;
	bsp_os = os_default;
	/* a part with less DRAM than usual still gets a zone, half of it */
	if (bsp_os + SZ_8M > bsp_dram)
		bsp_os = ALIGN(bsp_dram / 2, BSP_ZONE_ALIGN);
	if (bsp_os_req) {
		if (bsp_os_req >= SZ_32M &&
		    ALIGN(bsp_os_req, BSP_ZONE_ALIGN) + SZ_8M <= bsp_dram)
			bsp_os = ALIGN(bsp_os_req, BSP_ZONE_ALIGN);
		else
			pr_warn("hisi_mem: osmem=%lluM ignored, out of range for %lluM DRAM\n",
				(u64)bsp_os_req >> 20, (u64)bsp_dram >> 20);
	}

	if (booted != bsp_dram && booted != bsp_os)
		pr_info("hisi_mem: %lluM of memory from the bootloader ignored, DRAM is %lluM\n",
			(u64)booted >> 20, (u64)bsp_dram >> 20);

	memblock_remove(bsp_lo, PHYS_ADDR_MAX - bsp_lo);
	if (cma) {
		memblock_add(bsp_lo, end - bsp_lo);
		strlcpy(bsp_zone[0].name, "anonymous", NAME_LEN_MAX);
		bsp_zone[0].pdev.coherent_dma_mask = DMA_BIT_MASK(64);
		bsp_zone[0].phys_start = bsp_ddr + bsp_os;
		bsp_zone[0].nbytes = bsp_zone_span(&bsp_zone[0].phys_start, end) -
				     bsp_zone[0].phys_start;
		if (!bsp_zone[0].nbytes) {
			/* the bootloader's blobs leave no whole unit for it */
			pr_warn("hisi_mem: DRAM %lluM (probed), no room for a media zone; all of it is the OS's\n",
				(u64)bsp_dram >> 20);
			/* nothing published: userspace keeps to the bootloader's layout */
			bsp_dram = 0;
			return;
		}
		/* below the zone is the OS's; above it too, under cma */
		bsp_os = bsp_zone[0].phys_start - bsp_ddr;
		num_zones = 1;
		use_bootargs = 1;
		bsp_hold_zones();
	} else {
		memblock_add(bsp_lo, bsp_ddr + bsp_os - bsp_lo);
		bsp_carve_out(bsp_ddr + bsp_os);
	}
	strlcpy(allocator, cma ? "cma" : "ot", sizeof(allocator));
	pr_info("hisi_mem: DRAM %lluM (probed), %s, OS %lluM, mmz=anonymous,0,0x%llx,%lluM\n",
		(u64)bsp_dram >> 20, allocator, (u64)bsp_os >> 20, (u64)(bsp_ddr + bsp_os),
		(u64)(cma ? bsp_zone[0].nbytes : bsp_dram - bsp_os) >> 20);
}

static int __init bsp_memory_publish(void)
{
	struct device_node *np;
	struct property *prop;
	__be32 *reg;
	int i, ac, sc, cells;
	size_t len = 0;

	/*
	 * Nothing to publish when the kernel did not lay memory out, or when
	 * every zone it laid out failed to be reserved: no zone is then the
	 * media's, and a carve-out's "anonymous" range would claim memory that
	 * is the kernel's or someone else's. Userspace keeps the bootloader's.
	 */
	if (!bsp_dram || (bsp_zones_failed && !num_zones)) {
		allocator[0] = '\0';	/* what load_hisilicon looks at first */
		return 0;
	}
	dram = bsp_dram >> 20;
	osmem = bsp_os >> 20;
	if (num_zones) {
		/* the zones as declared, which is what mmz looks up; whole entries only */
		for (i = 0; i < num_zones; i++) {
			char entry[NAME_LEN_MAX + 40];
			int n = snprintf(entry, sizeof(entry), "%s%s,0,0x%llx,%lluM",
					 i ? ":" : "", bsp_zone[i].name, (u64)bsp_zone[i].phys_start,
					 (u64)bsp_zone[i].nbytes >> 20);

			if (n >= sizeof(entry) || len + n >= sizeof(mmz)) {
				pr_warn("hisi_mem: mmz zones from %s on not published, too long\n",
					bsp_zone[i].name);
				break;
			}
			memcpy(mmz + len, entry, n + 1);
			len += n;
		}
	} else {
		snprintf(mmz, sizeof(mmz), "anonymous,0,0x%llx,%lluM",
			 (u64)(bsp_ddr + bsp_os), (u64)(bsp_dram - bsp_os) >> 20);
	}

	np = of_find_node_by_path("/memory");
	if (!np) {
		pr_warn("hisi_mem: no /memory node, its size not updated\n");
		return 0;
	}
	ac = of_n_addr_cells(np);
	sc = of_n_size_cells(np);
	cells = ac + sc;
	prop = cells <= 4 ? kzalloc(sizeof(*prop) + cells * sizeof(*reg), GFP_KERNEL) : NULL;
	if (prop) {
		u64 base = bsp_lo, size = bsp_ddr + bsp_dram - bsp_lo;

		reg = (__be32 *)(prop + 1);
		for (i = ac - 1; i >= 0; i--, base >>= 32)
			reg[i] = cpu_to_be32((u32)base);
		for (i = cells - 1; i >= ac; i--, size >>= 32)
			reg[i] = cpu_to_be32((u32)size);
		prop->name = "reg";
		prop->length = cells * sizeof(*reg);
		prop->value = reg;
		if (of_update_property(np, prop)) {
			pr_warn("hisi_mem: /memory not updated to the measured %uM\n", dram);
			kfree(prop);
		}
	} else {
		pr_warn("hisi_mem: /memory not updated to the measured %uM\n", dram);
	}
	of_node_put(np);
	return 0;
}
core_initcall(bsp_memory_publish);
