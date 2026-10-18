#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/cma.h>
#include <linux/io.h>
#include <linux/memblock.h>
#include <linux/mmzone.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <asm/early_ioremap.h>
#include <asm/memory.h>

#define NAME_LEN_MAX   64
#define ZONE_MAX       64

struct cma_zone {
    struct device pdev;
    char name[NAME_LEN_MAX];
    gfp_t gfp;
    phys_addr_t phys_start;
    phys_addr_t nbytes;
    u32 alloc_type;
    u32 block_align;
};

static u32 num_zones;
static struct cma_zone hisi_zone[ZONE_MAX];

static int use_bootargs;

unsigned int get_cma_size(void)
{
    int i;
    u64 total = 0;

    for (i = 0; i < num_zones; i++) {
        total += hisi_zone[i].nbytes;
    }

    /* unit is M */
    return (unsigned int)(total >> 20);
}

int is_hicma_address(phys_addr_t phys, unsigned long size)
{
    phys_addr_t start, end;
    int i;

    for (i = 0; i < num_zones; i++) {
        start = hisi_zone[i].phys_start;
        end = hisi_zone[i].phys_start + hisi_zone[i].nbytes;

        if ((phys >= start) && ((phys + size) <= end)) {
            /*
             * Yes, found!
             */
            return 1;
        }
    }

    return 0;
}
EXPORT_SYMBOL(is_hicma_address);

static int __init hisi_mmz_parse_cmdline(char *s)
{
    char *line, *tmp;
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

        hisi_zone[num_zones].pdev.coherent_dma_mask = DMA_BIT_MASK(64);
        if (i == 4) {
            strlcpy(hisi_zone[num_zones].name, argv[0], NAME_LEN_MAX);
            hisi_zone[num_zones].gfp = memparse(argv[1], NULL);
            hisi_zone[num_zones].phys_start = memparse(argv[2], NULL);
            hisi_zone[num_zones].nbytes = memparse(argv[3], NULL);
        }

        else if (i == 6) {
            strlcpy(hisi_zone[num_zones].name, argv[0], NAME_LEN_MAX);
            hisi_zone[num_zones].gfp = memparse(argv[1], NULL);
            hisi_zone[num_zones].phys_start = memparse(argv[2], NULL);
            hisi_zone[num_zones].nbytes = memparse(argv[3], NULL);
            hisi_zone[num_zones].alloc_type = memparse(argv[4], NULL);
            hisi_zone[num_zones].block_align = memparse(argv[5], NULL);
        } else {
            pr_err("hisi ion parameter is not correct\n");
            continue;
        }

        num_zones++;
    }
    if (num_zones != 0) {
        use_bootargs = 1;
    }

    return 0;
}
early_param("mmz", hisi_mmz_parse_cmdline);

#include <linux/memblock.h>
phys_addr_t hisi_get_zones_start(void)
{
    int i;
    phys_addr_t lowest_zone_base = memblock_end_of_DRAM();

    for (i = 0; i < num_zones; i++) {
        if (lowest_zone_base > hisi_zone[i].phys_start) {
            lowest_zone_base = hisi_zone[i].phys_start;
        }
    }

    return lowest_zone_base;
}
EXPORT_SYMBOL(hisi_get_zones_start);

struct cma_zone *hisi_get_cma_zone(const char *name)
{
    int i = 0;

    for (i = 0; i < num_zones; i++)
        if (strcmp(hisi_zone[i].name, name) == 0) {
            break;
        }

    if (i == num_zones) {
        return NULL;
    }

    return &hisi_zone[i];
}
EXPORT_SYMBOL(hisi_get_cma_zone);

struct device *hisi_get_cma_device(const char *name)
{
    int i = 0;

    for (i = 0; i < num_zones; i++)
        if (strcmp(hisi_zone[i].name, name) == 0) {
            break;
        }

    if (i == num_zones) {
        return NULL;
    }

    return &hisi_zone[i].pdev;
}
EXPORT_SYMBOL(hisi_get_cma_device);

int __init hisi_declare_heap_memory(void)
{
    int i;
    int ret = 0;

    if (use_bootargs == 0) {
        pr_info("cmz zone is not set!\n");
        return ret;
    }

    for (i = 0; i < num_zones; i++) {
        ret = dma_declare_contiguous(&hisi_zone[i].pdev,
                                     hisi_zone[i].nbytes, hisi_zone[i].phys_start, 0);
        if (ret) {
            panic("declare cma zone %s base: %lux size:%lux MB failed. ret:%d",
                  hisi_zone[i].name, (unsigned long)hisi_zone[i].phys_start,
                  (unsigned long)hisi_zone[i].nbytes >> 20, ret);
        }
        hisi_zone[i].phys_start = cma_get_base(hisi_zone[i].pdev.cma_area);
        hisi_zone[i].nbytes = cma_get_size(hisi_zone[i].pdev.cma_area);

        /* FIXME need to fix dma_declare_contiguous return value &&value type */
    }

    return ret;
}
EXPORT_SYMBOL(hisi_declare_heap_memory);

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
static int hisi_mmz_setup(struct reserved_mem *rmem)
{
    return 0;
}
RESERVEDMEM_OF_DECLARE(cma, "hisi-mmz", hisi_mmz_setup);

/*
 * DRAM size, allocator and OS share, decided by the kernel.
 *
 * The bootloader cannot be relied on to say how much DRAM there is: a vendor
 * U-Boot passes mem= from its own environment and no memory tag, which
 * leaves the device tree's 512 MiB placeholder. So the kernel measures it,
 * the way U-Boot's get_ram_size() does, and lays memory out itself:
 *
 *   DRAM      allocator  OS share  media zone
 *   64 MiB    hisi       32 MiB    the rest, a carve-out the kernel never sees
 *   128+ MiB  cma        32 MiB    the rest, a CMA area lent to the OS when idle
 *
 * A bootloader's plain mem= gives way to that. mmz_allocator=hisi|cma and
 * osmem=<size> on the command line override the defaults; an explicit mmz=
 * keeps today's meaning, with mem= as the bootloader passes it.
 *
 * The result is published read-only under /sys/module/hisi_mem/parameters
 * (dram and osmem in MiB, allocator, and mmz as the osal module takes it),
 * and the device tree's /memory is set to the measured size.
 */
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "hisi_mem."

#define HISI_OS_MIN	SZ_32M
/*
 * cma_declare_contiguous() rounds a fixed zone's base and size up to this
 * (PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order)); a zone has to stay
 * inside DRAM after the rounding, not only before it.
 */
#define HISI_ZONE_ALIGN	((phys_addr_t)PAGE_SIZE << max_t(unsigned int, MAX_ORDER - 1, pageblock_order))
/*
 * The page the probe writes, saving and restoring it: the last of the 32 MiB
 * OS share, above the kernel image and its device tree, and not yet handed
 * out this early in setup_arch().
 */
#define HISI_PROBE_OFF	(SZ_32M - PAGE_SIZE)

static phys_addr_t hisi_dram, hisi_os;
static char hisi_alloc_req[8];
static phys_addr_t hisi_os_req;

static unsigned int dram;
static unsigned int osmem;
static char allocator[8];
static char mmz[256];
module_param(dram, uint, 0444);
module_param(osmem, uint, 0444);
module_param_string(allocator, allocator, sizeof(allocator), 0444);
module_param_string(mmz, mmz, sizeof(mmz), 0444);

static int __init hisi_alloc_param(char *s)
{
	if (s && (!strcmp(s, "hisi") || !strcmp(s, "cma")))
		strlcpy(hisi_alloc_req, s, sizeof(hisi_alloc_req));
	else
		pr_warn("hisi_mem: mmz_allocator=%s ignored, not hisi or cma\n", s ? s : "");
	return 0;
}
early_param("mmz_allocator", hisi_alloc_param);

static int __init hisi_osmem_param(char *s)
{
	if (s)
		hisi_os_req = memparse(s, NULL);
	return 0;
}
early_param("osmem", hisi_osmem_param);

/*
 * Whether size is past the end of DRAM, seen from the probe page p: the
 * address p + size either wraps back onto p or holds no memory at all.
 * 1 if so, 0 if not, -1 if the pages could not be mapped.
 */
static int __init hisi_dram_ends_at(phys_addr_t p, phys_addr_t size)
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
static phys_addr_t __init hisi_probe_dram(void)
{
	phys_addr_t size;
	int end;

	/*
	 * Not past 256 MiB: the largest part of the family has 512 MiB, and
	 * an address beyond it may be outside the DDR window altogether.
	 */
	for (size = SZ_64M; size <= SZ_256M; size <<= 1) {
		end = hisi_dram_ends_at(PHYS_OFFSET + HISI_PROBE_OFF, size);
		if (end < 0)
			return 0;
		if (end)
			return size;
	}
	return SZ_512M;
}

void __init hisi_memory_init(void)
{
	phys_addr_t base = PHYS_OFFSET, booted = memblock_end_of_DRAM() - base;
	bool cma;

	hisi_dram = hisi_probe_dram();
	if (!hisi_dram) {
		pr_warn("hisi_mem: DRAM size not measured, memory as the bootloader set it\n");
		return;
	}

	if (use_bootargs) {
		/*
		 * An explicit mmz=: the zone and mem= as the bootloader gave them,
		 * except where they claim memory the board does not have. Past the
		 * end of DRAM the address wraps onto the kernel's own pages, so a
		 * zone or a mem= reaching there corrupts it as soon as it is used.
		 */
		phys_addr_t end = base + hisi_dram;
		int i, n = 0;

		if (base + booted > end) {
			pr_warn("hisi_mem: mem=%lluM is more than the %lluM of DRAM, cut to it\n",
				(u64)booted >> 20, (u64)hisi_dram >> 20);
			memblock_remove(end, base + booted - end);
		}
		for (i = 0; i < num_zones; i++) {
			phys_addr_t start = ALIGN(hisi_zone[i].phys_start, HISI_ZONE_ALIGN);

			if (start + ALIGN(hisi_zone[i].nbytes, HISI_ZONE_ALIGN) > end) {
				pr_warn("hisi_mem: mmz zone %s at 0x%llx, %lluM, runs past the %lluM of DRAM\n",
					hisi_zone[i].name, (u64)hisi_zone[i].phys_start,
					(u64)hisi_zone[i].nbytes >> 20, (u64)hisi_dram >> 20);
				if (start >= end)
					continue;
				/* as CMA will place it, and only whole units of its alignment */
				hisi_zone[i].phys_start = start;
				hisi_zone[i].nbytes = round_down(end - start, HISI_ZONE_ALIGN);
				if (!hisi_zone[i].nbytes)
					continue;
			}
			if (n != i)
				hisi_zone[n] = hisi_zone[i];
			n++;
		}
		num_zones = n;
		if (!num_zones) {
			use_bootargs = 0;
			goto auto_layout;
		}
		booted = memblock_end_of_DRAM() - base;
		hisi_os = hisi_get_zones_start() - base;
		pr_info("hisi_mem: DRAM %lluM (probed), cma, zones from mmz=, mem=%lluM as passed\n",
			(u64)hisi_dram >> 20, (u64)booted >> 20);
		strlcpy(allocator, "cma", sizeof(allocator));
		return;
	}

auto_layout:
	cma = hisi_alloc_req[0] ? !strcmp(hisi_alloc_req, "cma") : hisi_dram > SZ_64M;
	hisi_os = HISI_OS_MIN;
	if (hisi_os_req) {
		if (hisi_os_req >= HISI_OS_MIN &&
		    ALIGN(hisi_os_req, HISI_ZONE_ALIGN) + SZ_8M <= hisi_dram)
			hisi_os = ALIGN(hisi_os_req, HISI_ZONE_ALIGN);
		else
			pr_warn("hisi_mem: osmem=%lluM ignored, out of range for %lluM DRAM\n",
				(u64)hisi_os_req >> 20, (u64)hisi_dram >> 20);
	}

	if (booted != hisi_dram && booted != hisi_os)
		pr_info("hisi_mem: %lluM of memory from the bootloader ignored, DRAM is %lluM\n",
			(u64)booted >> 20, (u64)hisi_dram >> 20);

	memblock_remove(base, memblock_end_of_DRAM() - base);
	if (cma) {
		memblock_add(base, hisi_dram);
		strlcpy(hisi_zone[0].name, "anonymous", NAME_LEN_MAX);
		hisi_zone[0].pdev.coherent_dma_mask = DMA_BIT_MASK(64);
		hisi_zone[0].phys_start = base + hisi_os;
		hisi_zone[0].nbytes = hisi_dram - hisi_os;
		num_zones = 1;
		use_bootargs = 1;
	} else {
		memblock_add(base, hisi_os);
	}
	strlcpy(allocator, cma ? "cma" : "hisi", sizeof(allocator));
	pr_info("hisi_mem: DRAM %lluM (probed), %s, OS %lluM, mmz=anonymous,0,0x%llx,%lluM\n",
		(u64)hisi_dram >> 20, allocator, (u64)hisi_os >> 20,
		(u64)(base + hisi_os), (u64)(hisi_dram - hisi_os) >> 20);
}

static int __init hisi_memory_publish(void)
{
	struct device_node *np;
	struct property *prop;
	__be32 *reg;
	size_t len = 0;
	int i;

	if (!hisi_dram)
		return 0;
	dram = hisi_dram >> 20;
	osmem = hisi_os >> 20;
	if (num_zones) {
		/* the zones as declared, which is what osal looks up; whole entries only */
		for (i = 0; i < num_zones; i++) {
			char entry[NAME_LEN_MAX + 40];
			int n = snprintf(entry, sizeof(entry), "%s%s,0,0x%llx,%lluM",
					 i ? ":" : "", hisi_zone[i].name, (u64)hisi_zone[i].phys_start,
					 (u64)hisi_zone[i].nbytes >> 20);

			if (n >= sizeof(entry) || len + n >= sizeof(mmz)) {
				pr_warn("hisi_mem: mmz zones from %s on not published, too long\n",
					hisi_zone[i].name);
				break;
			}
			memcpy(mmz + len, entry, n + 1);
			len += n;
		}
	} else {
		snprintf(mmz, sizeof(mmz), "anonymous,0,0x%llx,%lluM",
			 (u64)(PHYS_OFFSET + hisi_os), (u64)(hisi_dram - hisi_os) >> 20);
	}

	np = of_find_node_by_path("/memory");
	if (!np)
		return 0;
	prop = kzalloc(sizeof(*prop) + 2 * sizeof(*reg), GFP_KERNEL);
	if (prop) {
		reg = (__be32 *)(prop + 1);
		reg[0] = cpu_to_be32(PHYS_OFFSET);
		reg[1] = cpu_to_be32(hisi_dram);
		prop->name = "reg";
		prop->length = 2 * sizeof(*reg);
		prop->value = reg;
		of_update_property(np, prop);
	}
	of_node_put(np);
	return 0;
}
core_initcall(hisi_memory_publish);
