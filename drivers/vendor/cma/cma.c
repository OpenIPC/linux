/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */
#include <linux/bsp_cma.h>
#include <linux/io.h>
#include <linux/mmzone.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/securec.h>
#include <linux/initrd.h>
#include <linux/libfdt.h>
#include <asm/early_ioremap.h>

static u32 num_zones;
static struct cma_zone bsp_zone[ZONE_MAX];
static int use_bootargs;
static bool bsp_zones_held;

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

int __init declare_heap_memory(void)
{
	u32 i;
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

	for (i = 0; i < num_zones; i++) {
		ret = dma_contiguous_reserve_area(bsp_zone[i].nbytes,
						  bsp_zone[i].phys_start, 0, &bsp_zone[i].pdev.cma_area, true);
		if (ret)
			panic("declare cma zone %s base: 0x%lx size:%lu MB failed. ret:%d",
			      bsp_zone[i].name, (unsigned long)bsp_zone[i].phys_start,
			      (unsigned long)bsp_zone[i].nbytes >> 20, ret);

		bsp_zone[i].phys_start = cma_get_base(bsp_zone[i].pdev.cma_area);
		bsp_zone[i].nbytes = cma_get_size(bsp_zone[i].pdev.cma_area);
	}

	return ret;
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
	phys_addr_t max, os;
} bsp_mem_parts[] __initconst = {
	{ "hisilicon,hi3516cv610", SZ_512M, SZ_32M },
	{ "hisilicon,hi3516cv608", SZ_512M, SZ_32M },
	{ "vendor,hi3519dv500", SZ_2G, SZ_128M },
	{ "vendor,hi3516dv500", SZ_2G, SZ_128M },
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

#define BSP_BLOBS(blob) \
	phys_addr_t blob[2][2] = { \
		{ BSP_FDT_PHYS, initial_boot_params ? fdt_totalsize(initial_boot_params) : 0 }, \
		{ IS_ENABLED(CONFIG_BLK_DEV_INITRD) ? phys_initrd_start : 0, \
		  IS_ENABLED(CONFIG_BLK_DEV_INITRD) ? phys_initrd_size : 0 }, \
	}

/*
 * Where a zone from start can end, at most at end: below what the bootloader
 * left in memory for the kernel -- its device tree, an initrd -- which it puts
 * where it likes, often at the top of RAM, and which memblock reserves before
 * the zone is declared. In whole units of CMA's alignment.
 */
static phys_addr_t __init bsp_zone_end(phys_addr_t start, phys_addr_t end)
{
	BSP_BLOBS(blob);
	int i;

	for (i = 0; i < ARRAY_SIZE(blob); i++)
		if (blob[i][1] && blob[i][0] < end && blob[i][0] + blob[i][1] > start)
			end = max(start, round_down(blob[i][0], BSP_ZONE_ALIGN));
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
	BSP_BLOBS(blob);
	phys_addr_t lo = *start, best = bsp_zone_end(lo, end);
	int i;

	for (i = 0; i < ARRAY_SIZE(blob); i++) {
		phys_addr_t from = ALIGN(blob[i][0] + blob[i][1], BSP_ZONE_ALIGN);
		phys_addr_t to;

		if (!blob[i][1] || from <= lo || from >= end)
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
	bool cma;
	int i, n;

	for (i = 0; i < ARRAY_SIZE(bsp_mem_parts); i++)
		if (of_flat_dt_is_compatible(root, bsp_mem_parts[i].compatible) > 0) {
			max = bsp_mem_parts[i].max;
			os_default = bsp_mem_parts[i].os;
			break;
		}
	if (!max)
		return;

	/*
	 * DDR starts on a 64 MiB boundary; the first usable byte is where the
	 * device tree's memory starts (above firmware the kernel must not touch).
	 */
	bsp_lo = memblock_start_of_DRAM();
	bsp_ddr = round_down(bsp_lo, SZ_64M);
#ifdef CONFIG_ARM64
	/*
	 * mem= waits in memory_limit here, as an amount of memory from the
	 * first usable byte; apply it now, as ARM32 already has.
	 */
	{
		phys_addr_t keep = min_t(phys_addr_t, arm64_memory_limit(),
					 memblock_end_of_DRAM() - bsp_lo);

		arm64_set_memory_limit(PHYS_ADDR_MAX);
		memblock_remove(bsp_lo + keep, PHYS_ADDR_MAX - (bsp_lo + keep));
		booted = bsp_lo + keep - bsp_ddr;
	}
#else
	booted = memblock_end_of_DRAM() - bsp_ddr;
#endif

	bsp_dram = bsp_probe_dram(max, os_default);
	if (!bsp_dram) {
		pr_warn("hisi_mem: DRAM size not measured, memory as the bootloader set it\n");
		return;
	}
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
			phys_addr_t start = ALIGN(bsp_zone[i].phys_start, BSP_ZONE_ALIGN);
			phys_addr_t zend = bsp_zone_end(start, end);

			if (start + ALIGN(bsp_zone[i].nbytes, BSP_ZONE_ALIGN) > zend) {
				pr_warn("hisi_mem: mmz zone %s at 0x%llx, %lluM, runs past the %lluM of DRAM or into the bootloader's device tree\n",
					bsp_zone[i].name, (u64)bsp_zone[i].phys_start,
					(u64)bsp_zone[i].nbytes >> 20, (u64)bsp_dram >> 20);
				if (start >= zend)
					continue;
				/* as CMA will place it, and only whole units of its alignment */
				bsp_zone[i].phys_start = start;
				bsp_zone[i].nbytes = round_down(zend - start, BSP_ZONE_ALIGN);
				if (!bsp_zone[i].nbytes)
					continue;
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

	if (!bsp_dram)
		return 0;
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
