/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#include <linux/goke_cma.h>
#include <linux/io.h>
#include <linux/mmzone.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <asm/early_ioremap.h>
#include <asm/memory.h>

static u32 num_zones;
static struct cma_zone zone[ZONE_MAX];
static int use_bootargs;

unsigned int get_cma_size(void)
{
	int i;
	u64 total = 0;

	for (i = 0; i < num_zones; i++) {
		total += zone[i].nbytes;
	}

	/* unit is M */
	return (unsigned int)(total >> 20);
}

int is_cma_address(phys_addr_t phys, unsigned long size)
{
	phys_addr_t start, end;
	int i;

	for (i = 0; i < num_zones; i++) {
		start = zone[i].phys_start;
		end = zone[i].phys_start + zone[i].nbytes;

		if ((phys >= start) && ((phys + size) <= end)) {
			/*
			 * Yes, found!
			 */
			return 1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(is_cma_address);

static int __init mmz_parse_cmdline(char *s)
{
	char *line = NULL;
	char *tmp = NULL;
	char tmpline[256];

	if (s == NULL) {
		pr_info("There is no cma zone!\n");
		return 0;
	}
	strncpy(tmpline, s, sizeof(tmpline));
	tmpline[sizeof(tmpline) - 1] = '\0';
	tmp = tmpline;

	while ((line = strsep(&tmp, ":")) != NULL) {
		int i;
		char *argv[6];

		for (i = 0; (argv[i] = strsep(&line, ",")) != NULL;)
			if (++i == ARRAY_SIZE(argv)) {
				break;
			}

		zone[num_zones].pdev.coherent_dma_mask = DMA_BIT_MASK(64);
		if (i == 4) {
			strlcpy(zone[num_zones].name, argv[0], NAME_LEN_MAX);
			zone[num_zones].gfp = memparse(argv[1], NULL);
			zone[num_zones].phys_start = memparse(argv[2], NULL);
			zone[num_zones].nbytes = memparse(argv[3], NULL);
		}

		else if (i == 6) {
			strlcpy(zone[num_zones].name, argv[0], NAME_LEN_MAX);
			zone[num_zones].gfp = memparse(argv[1], NULL);
			zone[num_zones].phys_start = memparse(argv[2], NULL);
			zone[num_zones].nbytes = memparse(argv[3], NULL);
			zone[num_zones].alloc_type = memparse(argv[4], NULL);
			zone[num_zones].block_align = memparse(argv[5], NULL);
		} else {
			pr_err("ion parameter is not correct\n");
			continue;
		}

		num_zones++;
	}
	if (num_zones != 0) {
		use_bootargs = 1;
	}

	return 0;
}
early_param("mmz", mmz_parse_cmdline);

phys_addr_t goke_get_zones_start(void)
{
	int i;
	phys_addr_t lowest_zone_base = memblock_end_of_DRAM();

	for (i = 0; i < num_zones; i++) {
		if (lowest_zone_base > zone[i].phys_start) {
			lowest_zone_base = zone[i].phys_start;
		}
	}

	return lowest_zone_base;
}
EXPORT_SYMBOL(goke_get_zones_start);

struct cma_zone *goke_get_cma_zone(const char *name)
{
	int i = 0;

	for (i = 0; i < num_zones; i++)
		if (strcmp(zone[i].name, name) == 0) {
			break;
		}

	if (i == num_zones) {
		return NULL;
	}

	return &zone[i];
}
EXPORT_SYMBOL(goke_get_cma_zone);

struct device *goke_get_cma_device(const char *name)
{
	int i = 0;

	for (i = 0; i < num_zones; i++)
		if (strcmp(zone[i].name, name) == 0) {
			break;
		}

	if (i == num_zones) {
		return NULL;
	}

	return &zone[i].pdev;
}
EXPORT_SYMBOL(goke_get_cma_device);

int __init goke_declare_heap_memory(void)
{
	int i;
	int ret = 0;

	if (use_bootargs == 0) {
		pr_info("cma zone is not set!\n");
		return ret;
	}

	for (i = 0; i < num_zones; i++) {
		ret = dma_declare_contiguous(&zone[i].pdev,
					     zone[i].nbytes, zone[i].phys_start, 0);
		if (ret) {
			panic("declare cma zone %s base: %lux size:%lux MB failed. ret:%d",
			      zone[i].name, (unsigned long)zone[i].phys_start,
			      (unsigned long)zone[i].nbytes >> 20, ret);
		}
		zone[i].phys_start = cma_get_base(zone[i].pdev.cma_area);
		zone[i].nbytes = cma_get_size(zone[i].pdev.cma_area);

		/* FIXME need to fix dma_declare_contiguous return value &&value type */
	}

	return ret;
}
EXPORT_SYMBOL(goke_declare_heap_memory);

static int mmz_setup(struct reserved_mem *rmem)
{
	return 0;
}
RESERVEDMEM_OF_DECLARE(cma, "mmz", mmz_setup);

/*
 * DRAM size, allocator and OS share, decided by the kernel.
 *
 * The bootloader cannot be relied on to say how much DRAM there is: a vendor
 * U-Boot passes mem= from its own environment and no memory tag, which
 * leaves the device tree's 512 MiB placeholder. So the kernel measures it,
 * the way U-Boot's get_ram_size() does, and lays memory out itself:
 *
 *   DRAM      allocator  OS share  media zone
 *   64 MiB    gk         32 MiB    the rest, a carve-out the kernel never sees
 *   128+ MiB  cma        32 MiB    the rest, a CMA area lent to the OS when idle
 *
 * A bootloader's plain mem= gives way to that. mmz_allocator=gk|cma and
 * osmem=<size> on the command line override the defaults; an explicit mmz=
 * keeps today's meaning, with mem= as the bootloader passes it.
 *
 * The result is published read-only under /sys/module/goke_mem/parameters
 * (dram and osmem in MiB, allocator, and mmz as the osal module takes it),
 * and the device tree's /memory is set to the measured size.
 */
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "goke_mem."

#define GOKE_OS_MIN	SZ_32M
/*
 * cma_declare_contiguous() rounds a fixed zone's base and size up to this
 * (PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order)); a zone has to stay
 * inside DRAM after the rounding, not only before it.
 */
#define GOKE_ZONE_ALIGN	((phys_addr_t)PAGE_SIZE << max_t(unsigned int, MAX_ORDER - 1, pageblock_order))
/*
 * The page the probe writes, saving and restoring it: the bootcount page,
 * which every board device tree of this family reserves no-map at
 * base + 0x1f20000, below the 32 MiB OS share and out of the kernel image.
 */
#define GOKE_PROBE_OFF	0x1f20000

static phys_addr_t goke_dram, goke_os;
static char goke_alloc_req[8];
static phys_addr_t goke_os_req;

static unsigned int dram;
static unsigned int osmem;
static char allocator[8];
static char mmz[256];
module_param(dram, uint, 0444);
module_param(osmem, uint, 0444);
module_param_string(allocator, allocator, sizeof(allocator), 0444);
module_param_string(mmz, mmz, sizeof(mmz), 0444);

static int __init goke_alloc_param(char *s)
{
	if (s && (!strcmp(s, "gk") || !strcmp(s, "cma")))
		strlcpy(goke_alloc_req, s, sizeof(goke_alloc_req));
	else
		pr_warn("goke_mem: mmz_allocator=%s ignored, not gk or cma\n", s ? s : "");
	return 0;
}
early_param("mmz_allocator", goke_alloc_param);

static int __init goke_osmem_param(char *s)
{
	if (s)
		goke_os_req = memparse(s, NULL);
	return 0;
}
early_param("osmem", goke_osmem_param);

/*
 * Whether size is past the end of DRAM, seen from the probe page p: the
 * address p + size either wraps back onto p or holds no memory at all.
 * 1 if so, 0 if not, -1 if the pages could not be mapped.
 */
static int __init goke_dram_ends_at(phys_addr_t p, phys_addr_t size)
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
static phys_addr_t __init goke_probe_dram(void)
{
	phys_addr_t size;
	int end;

	/*
	 * Not past 256 MiB: the largest part of the family has 512 MiB, and
	 * an address beyond it may be outside the DDR window altogether.
	 */
	for (size = SZ_64M; size <= SZ_256M; size <<= 1) {
		end = goke_dram_ends_at(PHYS_OFFSET + GOKE_PROBE_OFF, size);
		if (end < 0)
			return 0;
		if (end)
			return size;
	}
	return SZ_512M;
}

void __init goke_memory_init(void)
{
	phys_addr_t base = PHYS_OFFSET, booted = memblock_end_of_DRAM() - base;
	bool cma;

	goke_dram = goke_probe_dram();
	if (!goke_dram) {
		pr_warn("goke_mem: DRAM size not measured, memory as the bootloader set it\n");
		return;
	}

	if (use_bootargs) {
		/*
		 * An explicit mmz=: the zone and mem= as the bootloader gave them,
		 * except where they claim memory the board does not have. Past the
		 * end of DRAM the address wraps onto the kernel's own pages, so a
		 * zone or a mem= reaching there corrupts it as soon as it is used.
		 */
		phys_addr_t end = base + goke_dram;
		int i, n = 0;

		if (base + booted > end) {
			pr_warn("goke_mem: mem=%lluM is more than the %lluM of DRAM, cut to it\n",
				(u64)booted >> 20, (u64)goke_dram >> 20);
			memblock_remove(end, base + booted - end);
		}
		for (i = 0; i < num_zones; i++) {
			phys_addr_t start = ALIGN(zone[i].phys_start, GOKE_ZONE_ALIGN);

			if (start + ALIGN(zone[i].nbytes, GOKE_ZONE_ALIGN) > end) {
				pr_warn("goke_mem: mmz zone %s at 0x%llx, %lluM, runs past the %lluM of DRAM\n",
					zone[i].name, (u64)zone[i].phys_start,
					(u64)zone[i].nbytes >> 20, (u64)goke_dram >> 20);
				if (start >= end)
					continue;
				/* as CMA will place it, and only whole units of its alignment */
				zone[i].phys_start = start;
				zone[i].nbytes = round_down(end - start, GOKE_ZONE_ALIGN);
				if (!zone[i].nbytes)
					continue;
			}
			if (n != i)
				zone[n] = zone[i];
			n++;
		}
		num_zones = n;
		if (!num_zones) {
			use_bootargs = 0;
			goto auto_layout;
		}
		booted = memblock_end_of_DRAM() - base;
		goke_os = goke_get_zones_start() - base;
		pr_info("goke_mem: DRAM %lluM (probed), cma, zones from mmz=, mem=%lluM as passed\n",
			(u64)goke_dram >> 20, (u64)booted >> 20);
		strlcpy(allocator, "cma", sizeof(allocator));
		return;
	}

auto_layout:
	cma = goke_alloc_req[0] ? !strcmp(goke_alloc_req, "cma") : goke_dram > SZ_64M;
	goke_os = GOKE_OS_MIN;
	if (goke_os_req) {
		if (goke_os_req >= GOKE_OS_MIN &&
		    ALIGN(goke_os_req, GOKE_ZONE_ALIGN) + SZ_8M <= goke_dram)
			goke_os = ALIGN(goke_os_req, GOKE_ZONE_ALIGN);
		else
			pr_warn("goke_mem: osmem=%lluM ignored, out of range for %lluM DRAM\n",
				(u64)goke_os_req >> 20, (u64)goke_dram >> 20);
	}

	if (booted != goke_dram && booted != goke_os)
		pr_info("goke_mem: %lluM of memory from the bootloader ignored, DRAM is %lluM\n",
			(u64)booted >> 20, (u64)goke_dram >> 20);

	memblock_remove(base, memblock_end_of_DRAM() - base);
	if (cma) {
		memblock_add(base, goke_dram);
		strlcpy(zone[0].name, "anonymous", NAME_LEN_MAX);
		zone[0].pdev.coherent_dma_mask = DMA_BIT_MASK(64);
		zone[0].phys_start = base + goke_os;
		zone[0].nbytes = goke_dram - goke_os;
		num_zones = 1;
		use_bootargs = 1;
	} else {
		memblock_add(base, goke_os);
	}
	strlcpy(allocator, cma ? "cma" : "gk", sizeof(allocator));
	pr_info("goke_mem: DRAM %lluM (probed), %s, OS %lluM, mmz=anonymous,0,0x%llx,%lluM\n",
		(u64)goke_dram >> 20, allocator, (u64)goke_os >> 20,
		(u64)(base + goke_os), (u64)(goke_dram - goke_os) >> 20);
}

static int __init goke_memory_publish(void)
{
	struct device_node *np;
	struct property *prop;
	__be32 *reg;
	size_t len = 0;
	int i;

	if (!goke_dram)
		return 0;
	dram = goke_dram >> 20;
	osmem = goke_os >> 20;
	if (num_zones) {
		/* the zones as declared, which is what osal looks up; whole entries only */
		for (i = 0; i < num_zones; i++) {
			char entry[NAME_LEN_MAX + 40];
			int n = snprintf(entry, sizeof(entry), "%s%s,0,0x%llx,%lluM",
					 i ? ":" : "", zone[i].name, (u64)zone[i].phys_start,
					 (u64)zone[i].nbytes >> 20);

			if (n >= sizeof(entry) || len + n >= sizeof(mmz)) {
				pr_warn("goke_mem: mmz zones from %s on not published, too long\n",
					zone[i].name);
				break;
			}
			memcpy(mmz + len, entry, n + 1);
			len += n;
		}
	} else {
		snprintf(mmz, sizeof(mmz), "anonymous,0,0x%llx,%lluM",
			 (u64)(PHYS_OFFSET + goke_os), (u64)(goke_dram - goke_os) >> 20);
	}

	np = of_find_node_by_path("/memory");
	if (!np)
		return 0;
	prop = kzalloc(sizeof(*prop) + 2 * sizeof(*reg), GFP_KERNEL);
	if (prop) {
		reg = (__be32 *)(prop + 1);
		reg[0] = cpu_to_be32(PHYS_OFFSET);
		reg[1] = cpu_to_be32(goke_dram);
		prop->name = "reg";
		prop->length = 2 * sizeof(*reg);
		prop->value = reg;
		of_update_property(np, prop);
	}
	of_node_put(np);
	return 0;
}
core_initcall(goke_memory_publish);
