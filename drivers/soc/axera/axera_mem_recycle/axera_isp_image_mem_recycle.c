#include <linux/module.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/slab.h>

#ifdef CONFIG_AXERA_ISP_IMAGE_MEM_RECYCLE

static phys_addr_t s_isp_addr_start = 0;
static phys_addr_t s_isp_addr_end = 0;

int ax_in_isp_image_mem_range(const phys_addr_t paddr)
{
	if (paddr >= s_isp_addr_start && paddr <= s_isp_addr_end){
		return 1;
	}

	return 0;
}
EXPORT_SYMBOL(ax_in_isp_image_mem_range);

void ax_free_isp_image_mem(const phys_addr_t addr_start, const unsigned int size)
{
	void *vir_start;
	unsigned long ret;

	if (!PAGE_ALIGNED(addr_start)){
		pr_err("%s fatal: addr_start: 0x%08llx is not page aligned, size: 0x%08x", __FUNCTION__, addr_start, size);
		dump_stack();
		return;
	}

	if (!PAGE_ALIGNED(size)){
		pr_err("%s fatal: ax_free_isp_image_mem size: 0x%08x is not page aligned, addr_start: 0x%08x", __FUNCTION__, size, addr_start);
		dump_stack();
		return;
	}

	if ((addr_start >= s_isp_addr_start) && ((addr_start+size) <= s_isp_addr_end)){
		vir_start = phys_to_virt(addr_start);
		ret = free_reserved_area(vir_start, vir_start + size, 0xffff, "isp image");
		pr_info("isp image memory freed pages: %ld!!!\n", ret);
	}else{
		pr_err("%s: invalid addr <0x%x> size <%x>\n", __FUNCTION__, addr_start, size);
	}
}
EXPORT_SYMBOL(ax_free_isp_image_mem);

static int __init ax_isp_image_mem_init(void)
{
	struct device_node *of_node;
	unsigned int addrs[4];
	int len;

	of_node = of_find_compatible_node(NULL, NULL, "axera, isp_image");
	if (!of_node) {
		pr_err("%s:can't find memory_dump tree node\n", __FUNCTION__);
		return -ENODEV;
	}
	pr_info("%s: of_find_compatible_node 'axera, isp_image' success!!!", __FUNCTION__);

	len = of_property_read_variable_u32_array(of_node, "reg", addrs, 4, ARRAY_SIZE(addrs));
	pr_debug("addr[0]:0x%x,addr[1]:0x%x,addr[2]:0x%x,addr[3]:0x%x", addrs[0], addrs[1], addrs[2], addrs[3]);
	if (len == 4){
		s_isp_addr_start = addrs[1];
		s_isp_addr_end   = addrs[1] + addrs[3];
	}else{
		pr_err("%s: invalid isp_image address!", __FUNCTION__);
	}

	return 0;
}

early_initcall(ax_isp_image_mem_init);

#else
int ax_in_isp_image_mem_range(const phys_addr_t paddr)
{
	return 0;
}
EXPORT_SYMBOL(ax_in_isp_image_mem_range);

void ax_free_isp_image_mem(const phys_addr_t addr_start, const unsigned int size)
{
	return;
}
EXPORT_SYMBOL(ax_free_isp_image_mem);

#endif
