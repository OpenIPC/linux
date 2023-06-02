/*
 * hi_cma.c
 *
 * Copyright (c) 2019 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/hi_cma.h>

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
		pr_info("cma zone is not set!\n");
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

static int hisi_mmz_setup(struct reserved_mem *rmem)
{
	return 0;
}
RESERVEDMEM_OF_DECLARE(cma, "hisi-mmz", hisi_mmz_setup);
