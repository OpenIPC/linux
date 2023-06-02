/*
 * arch/arm/plat-ambarella/generic/init.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/memblock.h>
#include <linux/export.h>
#include <linux/clk.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <asm/cacheflush.h>
#include <asm/system_info.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>
#include <mach/init.h>
#include <plat/clk.h>
#include <asm/hardware/cache-l2x0.h>

/* ==========================================================================*/
u64 ambarella_dmamask = DMA_BIT_MASK(32);
EXPORT_SYMBOL(ambarella_dmamask);

/* ==========================================================================*/
enum {
	AMBARELLA_IO_DESC_AHB_ID = 0,
	AMBARELLA_IO_DESC_APB_ID,
	AMBARELLA_IO_DESC_PPM_ID,
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_AXI)
	AMBARELLA_IO_DESC_AXI_ID,
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_DRAMC)
	AMBARELLA_IO_DESC_DRAMC_ID,
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_CRYPT)
	AMBARELLA_IO_DESC_CRYPT_ID,
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_AHB64)
	AMBARELLA_IO_DESC_AHB64_ID,
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_DBGBUS)
	AMBARELLA_IO_DESC_DBGBUS_ID,
	AMBARELLA_IO_DESC_DBGFMEM_ID,
#endif
	AMBARELLA_IO_DESC_FRAMEBUF_ID,
	AMBARELLA_IO_DESC_DSP_ID,
};

struct ambarella_mem_map_desc {
	char		name[8];
	struct map_desc	io_desc;
};

static struct ambarella_mem_map_desc ambarella_io_desc[] = {
	[AMBARELLA_IO_DESC_AHB_ID] = {
		.name		= "AHB",
		.io_desc	= {
			.virtual	= AHB_BASE,
			.pfn		= __phys_to_pfn(AHB_PHYS_BASE),
			.length		= AHB_SIZE,
#if defined(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)
			.type		= MT_DEVICE_NONSHARED,
#else
			.type		= MT_DEVICE,
#endif
			},
	},
	[AMBARELLA_IO_DESC_APB_ID] = {
		.name		= "APB",
		.io_desc	= {
			.virtual	= APB_BASE,
			.pfn		= __phys_to_pfn(APB_PHYS_BASE),
			.length		= APB_SIZE,
#if defined(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)
			.type		= MT_DEVICE_NONSHARED,
#else
			.type		= MT_DEVICE,
#endif
			},
	},
	[AMBARELLA_IO_DESC_PPM_ID] = {
		.name		= "PPM",	/*Private Physical Memory*/
		.io_desc		= {
			.virtual	= AHB_BASE - CONFIG_AMBARELLA_PPM_SIZE,
			.pfn		= __phys_to_pfn(DEFAULT_MEM_START),
			.length		= CONFIG_AMBARELLA_PPM_SIZE,
#if defined(CONFIG_AMBARELLA_PPM_UNCACHED)
			.type		= MT_DEVICE,
#else
			.type		= MT_MEMORY,
#endif
			},
	},
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_AXI)
	[AMBARELLA_IO_DESC_AXI_ID] = {
		.name		= "AXI",
		.io_desc	= {
			.virtual	= AXI_BASE,
			.pfn		= __phys_to_pfn(AXI_PHYS_BASE),
			.length		= AXI_SIZE,
			.type		= MT_DEVICE,
			},
	},
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_DRAMC)
	[AMBARELLA_IO_DESC_DRAMC_ID] = {
		.name		= "DRAMC",
		.io_desc	= {
			.virtual= DRAMC_BASE,
			.pfn	= __phys_to_pfn(DRAMC_PHYS_BASE),
			.length	= DRAMC_SIZE,
#if defined(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)
			.type	= MT_DEVICE_NONSHARED,
#else
			.type	= MT_DEVICE,
#endif
			},
	},
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_CRYPT)
	[AMBARELLA_IO_DESC_CRYPT_ID] = {
		.name		= "CRYPT",
		.io_desc	= {
			.virtual= CRYPT_BASE,
			.pfn	= __phys_to_pfn(CRYPT_PHYS_BASE),
			.length	= CRYPT_SIZE,
#if defined(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)
			.type	= MT_DEVICE_NONSHARED,
#else
			.type	= MT_DEVICE,
#endif
			},
	},
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_AHB64)
	[AMBARELLA_IO_DESC_AHB64_ID] = {
		.name		= "AHB64",
		.io_desc	= {
			.virtual= AHB64_BASE,
			.pfn	= __phys_to_pfn(AHB64_PHYS_BASE),
			.length	= AHB64_SIZE,
			.type	= MT_DEVICE,
			},
	},
#endif
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_DBGBUS)
	[AMBARELLA_IO_DESC_DBGBUS_ID] = {
		.name		= "DBGBUS",
		.io_desc	= {
			.virtual= DBGBUS_BASE,
			.pfn	= __phys_to_pfn(DBGBUS_PHYS_BASE),
			.length	= DBGBUS_SIZE,
			.type	= MT_DEVICE,
			},
	},
	[AMBARELLA_IO_DESC_DBGFMEM_ID] = {
		.name		= "DBGFMEM",
		.io_desc	= {
			.virtual= DBGFMEM_BASE,
			.pfn	= __phys_to_pfn(DBGFMEM_PHYS_BASE),
			.length	= DBGFMEM_SIZE,
			.type	= MT_DEVICE,
			},
	},
#endif
	[AMBARELLA_IO_DESC_DSP_ID] = {
		.name		= "IAV",
		.io_desc	= {
			.virtual	= 0x00000000,
			.pfn		= 0x00000000,
			.length		= 0x00000000,
			},
	},
	[AMBARELLA_IO_DESC_FRAMEBUF_ID] = {
		.name		= "FRAMEBUF",
		.io_desc	= {
			.virtual	= 0x00000000,
			.pfn		= 0x00000000,
			.length		= 0x00000000,
			},
	},
};


static int __init ambarella_dt_scan_iavmem(unsigned long node,
			const char *uname,  int depth, void *data)
{
	const char *type;
	__be32 *reg;
	unsigned long len;
	struct ambarella_mem_map_desc *iavmem_desc;

	type = of_get_flat_dt_prop(node, "device_type", NULL);
	if (type == NULL || strcmp(type, "iavmem") != 0)
		return 0;

	reg = of_get_flat_dt_prop(node, "reg", &len);
	if (WARN_ON(!reg || (len != 2 * sizeof(unsigned long))))
		return 0;

	iavmem_desc = &ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID];
	iavmem_desc->io_desc.pfn = __phys_to_pfn(be32_to_cpu(reg[0]));
	iavmem_desc->io_desc.length = be32_to_cpu(reg[1]);

	pr_info("Ambarella:   IAVMEM = 0x%08x[          ],0x%08x\n",
			be32_to_cpu(reg[0]), be32_to_cpu(reg[1]));

	return 1;
}

static int __init ambarella_dt_scan_fbmem(unsigned long node,
			const char *uname,  int depth, void *data)
{
	const char *type;
	__be32 *reg;
	unsigned long len;
	struct ambarella_mem_map_desc *fbmem_desc;

	type = of_get_flat_dt_prop(node, "device_type", NULL);
	if (type == NULL || strcmp(type, "fbmem") != 0)
		return 0;

	reg = of_get_flat_dt_prop(node, "reg", &len);
	if (WARN_ON(!reg || (len != 2 * sizeof(unsigned long))))
		return 0;

	fbmem_desc = &ambarella_io_desc[AMBARELLA_IO_DESC_FRAMEBUF_ID];
	fbmem_desc->io_desc.pfn = __phys_to_pfn(be32_to_cpu(reg[0]));
	fbmem_desc->io_desc.length = be32_to_cpu(reg[1]);

	pr_info("Ambarella:    FBMEM = 0x%08x[          ],0x%08x\n",
			be32_to_cpu(reg[0]), be32_to_cpu(reg[1]));

	return 1;
}

void __init ambarella_map_io(void)
{
	u32 i, iop, ios, iov;

	for (i = 0; i < AMBARELLA_IO_DESC_DSP_ID; i++) {
		iop = __pfn_to_phys(ambarella_io_desc[i].io_desc.pfn);
		ios = ambarella_io_desc[i].io_desc.length;
		iov = ambarella_io_desc[i].io_desc.virtual;
		if (ios > 0) {
			iotable_init(&(ambarella_io_desc[i].io_desc), 1);
			pr_info("Ambarella: %8s = 0x%08x[0x%08x],0x%08x %d\n",
				ambarella_io_desc[i].name, iop, iov, ios,
				ambarella_io_desc[i].io_desc.type);
		}
	}

	/* scan and hold the memory information for IAV */
	of_scan_flat_dt(ambarella_dt_scan_iavmem, NULL);
	/* scan and hold the memory information for FRAMEBUFFER */
	of_scan_flat_dt(ambarella_dt_scan_fbmem, NULL);
}

/* ==========================================================================*/
static struct proc_dir_entry *ambarella_proc_dir = NULL;

int __init ambarella_create_proc_dir(void)
{
	int ret_val = 0;

	ambarella_proc_dir = proc_mkdir("ambarella", NULL);
	if (!ambarella_proc_dir)
		ret_val = -ENOMEM;

	return ret_val;
}

struct proc_dir_entry *get_ambarella_proc_dir(void)
{
	return ambarella_proc_dir;
}
EXPORT_SYMBOL(get_ambarella_proc_dir);


/* ==========================================================================*/
void __init ambarella_init_machine(void)
{
	int ret_val = 0;

#if defined(CONFIG_PLAT_AMBARELLA_LOWER_ARM_PLL)
	amba_rct_writel(SCALER_ARM_ASYNC_REG, 0xF);
#endif

	ret_val = ambarella_create_proc_dir();
	BUG_ON(ret_val != 0);

	ret_val = ambarella_clk_init();
	BUG_ON(ret_val != 0);

	ret_val = ambarella_init_fio();
	BUG_ON(ret_val != 0);

	ret_val = ambarella_init_fb();
	BUG_ON(ret_val != 0);

	ret_val = ambarella_init_pm();
	BUG_ON(ret_val != 0);

	ret_val = ambarella_init_audio();
	BUG_ON(ret_val != 0);
	l2x0_of_init((1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT)
			| (1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT),  ~0);

	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

void ambarella_restart_machine(char mode, const char *cmd)
{
	local_irq_disable();
	local_fiq_disable();
	flush_cache_all();
	amba_rct_writel(SOFT_OR_DLL_RESET_REG, 0x2);
	amba_rct_writel(SOFT_OR_DLL_RESET_REG, 0x3);
}

/* ==========================================================================*/

static int __init parse_system_revision(char *p)
{
	system_rev = simple_strtoul(p, NULL, 0);
	return 0;
}
early_param("system_rev", parse_system_revision);

u32 ambarella_phys_to_virt(u32 paddr)
{
	int					i;
	u32					phystart;
	u32					phylength;
	u32					phyoffset;
	u32					vstart;

	for (i = 0; i < ARRAY_SIZE(ambarella_io_desc); i++) {
		phystart = __pfn_to_phys(ambarella_io_desc[i].io_desc.pfn);
		phylength = ambarella_io_desc[i].io_desc.length;
		vstart = ambarella_io_desc[i].io_desc.virtual;
		if ((paddr >= phystart) && (paddr < (phystart + phylength))) {
			phyoffset = paddr - phystart;
			return (vstart + phyoffset);
		}
	}

	return __amb_raw_phys_to_virt(paddr);
}
EXPORT_SYMBOL(ambarella_phys_to_virt);

u32 ambarella_virt_to_phys(u32 vaddr)
{
	int					i;
	u32					phystart;
	u32					vlength;
	u32					voffset;
	u32					vstart;

	for (i = 0; i < ARRAY_SIZE(ambarella_io_desc); i++) {
		phystart = __pfn_to_phys(ambarella_io_desc[i].io_desc.pfn);
		vlength = ambarella_io_desc[i].io_desc.length;
		vstart = ambarella_io_desc[i].io_desc.virtual;
		if ((vaddr >= vstart) && (vaddr < (vstart + vlength))) {
			voffset = vaddr - vstart;
			return (phystart + voffset);
		}
	}

	return __amb_raw_virt_to_phys(vaddr);
}
EXPORT_SYMBOL(ambarella_virt_to_phys);

u32 get_ambarella_fbmem_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_FRAMEBUF_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_fbmem_phys);

u32 get_ambarella_fbmem_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_FRAMEBUF_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_fbmem_size);

u32 get_ambarella_iavmem_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_iavmem_phys);

u32 get_ambarella_iavmem_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_DSP_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_iavmem_size);

u32 get_ambarella_ppm_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_ppm_phys);

u32 get_ambarella_ppm_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_ppm_virt);

u32 get_ambarella_ppm_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_PPM_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_ppm_size);

u32 get_ambarella_ahb_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_AHB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_ahb_phys);

u32 get_ambarella_ahb_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_AHB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_ahb_virt);

u32 get_ambarella_ahb_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_AHB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_ahb_size);

u32 get_ambarella_apb_phys(void)
{
	return __pfn_to_phys(
		ambarella_io_desc[AMBARELLA_IO_DESC_APB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ambarella_apb_phys);

u32 get_ambarella_apb_virt(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_APB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ambarella_apb_virt);

u32 get_ambarella_apb_size(void)
{
	return ambarella_io_desc[AMBARELLA_IO_DESC_APB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ambarella_apb_size);

u32 ambarella_get_poc(void)
{
	return amba_rct_readl(SYS_CONFIG_REG);
}
EXPORT_SYMBOL(ambarella_get_poc);


